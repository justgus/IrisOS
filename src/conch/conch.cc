#include "refract/schema_registry.h"
#include "referee/referee.h"
#include "referee_sqlite/sqlite_store.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using iris::refract::SchemaRegistry;
using iris::refract::TypeSummary;
using referee::ObjectID;
using referee::ObjectRef;
using referee::SqliteConfig;
using referee::SqliteStore;
using referee::TypeID;

namespace {

struct TaskEntry {
  std::string id;
  ObjectRef target;
  std::string state;
};

std::vector<std::string> split_ws(const std::string& line) {
  std::istringstream iss(line);
  std::vector<std::string> out;
  std::string tok;
  while (iss >> tok) out.push_back(tok);
  return out;
}

std::string type_display_name(const TypeSummary& summary) {
  if (summary.namespace_name.empty()) return summary.name;
  return summary.namespace_name + "::" + summary.name;
}

std::optional<TypeSummary> find_type_summary(const std::vector<TypeSummary>& types,
                                             const std::string& name,
                                             std::string* err_out) {
  std::optional<TypeSummary> match;
  for (const auto& t : types) {
    auto full = type_display_name(t);
    if (t.name == name || full == name) {
      if (match.has_value()) {
        if (err_out) *err_out = "ambiguous type name";
        return std::nullopt;
      }
      match = t;
    }
  }
  if (!match.has_value() && err_out) *err_out = "type not found";
  return match;
}

std::optional<ObjectID> parse_object_id(const std::string& token, std::string* err_out) {
  if (token.size() != 32) {
    if (err_out) *err_out = "ObjectID must be 32 hex chars";
    return std::nullopt;
  }
  for (char c : token) {
    if (!std::isxdigit(static_cast<unsigned char>(c))) {
      if (err_out) *err_out = "ObjectID contains non-hex characters";
      return std::nullopt;
    }
  }
  try {
    return ObjectID::from_hex(token);
  } catch (const std::exception& ex) {
    if (err_out) *err_out = ex.what();
    return std::nullopt;
  }
}

std::optional<ObjectRef> latest_ref(SqliteStore& store, const ObjectID& id, std::string* err_out) {
  auto recR = store.get_latest(id);
  if (!recR) {
    if (err_out) *err_out = recR.error->message;
    return std::nullopt;
  }
  if (!recR.value->has_value()) {
    if (err_out) *err_out = "object not found";
    return std::nullopt;
  }
  return recR.value->value().ref;
}

void print_help() {
  std::cout << "Commands:\n";
  std::cout << "  ls\n";
  std::cout << "  show <ObjectID>\n";
  std::cout << "  edges <ObjectID>\n";
  std::cout << "  find type <TypeName>\n";
  std::cout << "  call <ObjectID> <opName> [args...]\n";
  std::cout << "  start <ObjectID>\n";
  std::cout << "  ps\n";
  std::cout << "  kill <TaskID>\n";
  std::cout << "  help\n";
  std::cout << "  exit\n";
}

void cmd_ls(SchemaRegistry& registry, SqliteStore& store) {
  auto typesR = registry.list_types();
  if (!typesR) {
    std::cout << "error: " << typesR.error->message << "\n";
    return;
  }
  if (typesR.value->empty()) {
    std::cout << "no types registered\n";
    return;
  }

  for (const auto& summary : typesR.value.value()) {
    std::cout << "type " << type_display_name(summary) << " (0x" << std::hex << summary.type_id.v
              << std::dec << ")\n";
    auto listR = store.list_by_type(summary.type_id);
    if (!listR) {
      std::cout << "  error: " << listR.error->message << "\n";
      continue;
    }
    if (listR.value->empty()) {
      std::cout << "  (no objects)\n";
      continue;
    }
    for (const auto& rec : listR.value.value()) {
      std::cout << "  " << rec.ref.id.to_hex() << " v" << rec.ref.ver.v << "\n";
    }
  }
}

void cmd_find_type(SchemaRegistry& registry, const std::string& name) {
  auto typesR = registry.list_types();
  if (!typesR) {
    std::cout << "error: " << typesR.error->message << "\n";
    return;
  }
  std::string err;
  auto match = find_type_summary(typesR.value.value(), name, &err);
  if (!match.has_value()) {
    std::cout << "error: " << err << "\n";
    return;
  }
  std::cout << "type " << type_display_name(*match) << " id=0x" << std::hex << match->type_id.v
            << std::dec << " def=" << match->definition_id.to_hex() << "\n";
}

void cmd_show(SchemaRegistry& registry, SqliteStore& store, const ObjectID& id) {
  auto recR = store.get_latest(id);
  if (!recR) {
    std::cout << "error: " << recR.error->message << "\n";
    return;
  }
  if (!recR.value->has_value()) {
    std::cout << "error: object not found\n";
    return;
  }

  const auto& rec = recR.value->value();
  std::cout << "object " << rec.ref.id.to_hex() << " v" << rec.ref.ver.v << "\n";
  std::cout << "type 0x" << std::hex << rec.type.v << std::dec << "\n";
  std::cout << "definition " << rec.definition_id.to_hex() << "\n";
  std::cout << "created_at_ms " << rec.created_at_unix_ms << "\n";

  try {
    auto json = referee::json_string_from_cbor(rec.payload_cbor);
    std::cout << "payload " << json << "\n";
  } catch (const std::exception& ex) {
    std::cout << "payload <unparseable> (" << ex.what() << ")\n";
  }

  auto defR = registry.get_definition_by_type(rec.type);
  if (!defR) {
    std::cout << "refract error: " << defR.error->message << "\n";
    return;
  }
  if (!defR.value->has_value()) {
    std::cout << "refract: definition not found\n";
    return;
  }

  const auto& def = defR.value->value().definition;
  std::cout << "refract " << def.namespace_name << "::" << def.name << " v" << def.version << "\n";
  if (!def.fields.empty()) {
    std::cout << "fields\n";
    for (const auto& field : def.fields) {
      std::cout << "  " << field.name << " type=0x" << std::hex << field.type.v << std::dec;
      if (field.required) std::cout << " required";
      if (field.default_json.has_value()) std::cout << " default=" << field.default_json.value();
      std::cout << "\n";
    }
  }
  if (!def.operations.empty()) {
    std::cout << "operations\n";
    for (const auto& op : def.operations) {
      std::cout << "  " << op.name << "(";
      for (size_t i = 0; i < op.signature.params.size(); ++i) {
        const auto& param = op.signature.params[i];
        if (i > 0) std::cout << ", ";
        std::cout << param.name;
        if (param.optional) std::cout << "?";
      }
      std::cout << ")";
      if (op.signature.return_type.has_value()) {
        std::cout << " -> 0x" << std::hex << op.signature.return_type->v << std::dec;
      }
      std::cout << "\n";
    }
  }
}

void cmd_edges(SqliteStore& store, const ObjectID& id) {
  std::string err;
  auto ref = latest_ref(store, id, &err);
  if (!ref.has_value()) {
    std::cout << "error: " << err << "\n";
    return;
  }

  auto outR = store.edges_from(*ref);
  if (!outR) {
    std::cout << "error: " << outR.error->message << "\n";
    return;
  }
  auto inR = store.edges_to(*ref);
  if (!inR) {
    std::cout << "error: " << inR.error->message << "\n";
    return;
  }

  std::cout << "edges from " << ref->id.to_hex() << "\n";
  if (outR.value->empty()) {
    std::cout << "  (none)\n";
  } else {
    for (const auto& edge : outR.value.value()) {
      std::cout << "  -> " << edge.to.id.to_hex() << " v" << edge.to.ver.v
                << " name=" << edge.name << " role=" << edge.role << "\n";
    }
  }
  std::cout << "edges to " << ref->id.to_hex() << "\n";
  if (inR.value->empty()) {
    std::cout << "  (none)\n";
  } else {
    for (const auto& edge : inR.value.value()) {
      std::cout << "  <- " << edge.from.id.to_hex() << " v" << edge.from.ver.v
                << " name=" << edge.name << " role=" << edge.role << "\n";
    }
  }
}

std::optional<iris::refract::OperationDefinition> find_operation(
    const iris::refract::TypeDefinition& def, const std::string& name) {
  for (const auto& op : def.operations) {
    if (op.name == name) return op;
  }
  return std::nullopt;
}

bool validate_call_args(const iris::refract::OperationDefinition& op, size_t arg_count,
                        std::string* err_out) {
  size_t required = 0;
  for (const auto& param : op.signature.params) {
    if (!param.optional) ++required;
  }
  if (arg_count < required) {
    if (err_out) *err_out = "missing required args";
    return false;
  }
  if (arg_count > op.signature.params.size()) {
    if (err_out) *err_out = "too many args";
    return false;
  }
  return true;
}

bool cmd_call(SchemaRegistry& registry, SqliteStore& store, const ObjectID& id,
              const std::string& op_name, size_t arg_count) {
  auto recR = store.get_latest(id);
  if (!recR) {
    std::cout << "error: " << recR.error->message << "\n";
    return false;
  }
  if (!recR.value->has_value()) {
    std::cout << "error: object not found\n";
    return false;
  }
  auto defR = registry.get_definition_by_type(recR.value->value().type);
  if (!defR) {
    std::cout << "error: " << defR.error->message << "\n";
    return false;
  }
  if (!defR.value->has_value()) {
    std::cout << "error: definition not found\n";
    return false;
  }
  const auto& def = defR.value->value().definition;
  auto op = find_operation(def, op_name);
  if (!op.has_value()) {
    std::cout << "error: operation not found\n";
    return false;
  }
  std::string err;
  if (!validate_call_args(op.value(), arg_count, &err)) {
    std::cout << "error: " << err << "\n";
    return false;
  }
  std::cout << "call ok\n";
  return true;
}

} // namespace

int main(int argc, char** argv) {
  std::string db_path = "referee.db";

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--db" && i + 1 < argc) {
      db_path = argv[++i];
      continue;
    }
    if (arg == "--help" || arg == "-h") {
      std::cout << "Usage: conch [--db <path>]\n";
      return 0;
    }
    std::cout << "unknown argument: " << arg << "\n";
    return 1;
  }

  SqliteStore store(SqliteConfig{ .filename=db_path });
  if (!store.open()) {
    std::cout << "error: failed to open db\n";
    return 1;
  }
  if (!store.ensure_schema()) {
    std::cout << "error: failed to ensure schema\n";
    return 1;
  }

  SchemaRegistry registry(store);
  std::vector<TaskEntry> tasks;
  std::uint64_t next_task_id = 1;

  std::string line;
  while (std::cout << "conch> " && std::getline(std::cin, line)) {
    auto tokens = split_ws(line);
    if (tokens.empty()) continue;

    const auto& cmd = tokens[0];
    if (cmd == "exit" || cmd == "quit") break;
    if (cmd == "help") {
      print_help();
      continue;
    }
    if (cmd == "ls") {
      cmd_ls(registry, store);
      continue;
    }
    if (cmd == "find" && tokens.size() >= 3 && tokens[1] == "type") {
      cmd_find_type(registry, tokens[2]);
      continue;
    }
    if (cmd == "show" && tokens.size() == 2) {
      std::string err;
      auto id = parse_object_id(tokens[1], &err);
      if (!id.has_value()) {
        std::cout << "error: " << err << "\n";
        continue;
      }
      cmd_show(registry, store, id.value());
      continue;
    }
    if (cmd == "edges" && tokens.size() == 2) {
      std::string err;
      auto id = parse_object_id(tokens[1], &err);
      if (!id.has_value()) {
        std::cout << "error: " << err << "\n";
        continue;
      }
      cmd_edges(store, id.value());
      continue;
    }
    if (cmd == "call" && tokens.size() >= 3) {
      std::string err;
      auto id = parse_object_id(tokens[1], &err);
      if (!id.has_value()) {
        std::cout << "error: " << err << "\n";
        continue;
      }
      cmd_call(registry, store, id.value(), tokens[2], tokens.size() - 3);
      continue;
    }
    if (cmd == "start" && tokens.size() == 2) {
      std::string err;
      auto id = parse_object_id(tokens[1], &err);
      if (!id.has_value()) {
        std::cout << "error: " << err << "\n";
        continue;
      }
      bool ok = cmd_call(registry, store, id.value(), "start", 0);
      if (ok) {
        std::ostringstream os;
        os << "task-" << std::setw(4) << std::setfill('0') << next_task_id++;
        TaskEntry entry;
        entry.id = os.str();
        entry.target = *latest_ref(store, id.value(), nullptr);
        entry.state = "running";
        tasks.push_back(std::move(entry));
        std::cout << "started " << tasks.back().id << "\n";
      }
      continue;
    }
    if (cmd == "ps") {
      if (tasks.empty()) {
        std::cout << "no tasks\n";
        continue;
      }
      for (const auto& task : tasks) {
        std::cout << task.id << " " << task.target.id.to_hex() << " " << task.state << "\n";
      }
      continue;
    }
    if (cmd == "kill" && tokens.size() == 2) {
      auto it = std::find_if(tasks.begin(), tasks.end(),
                             [&](const TaskEntry& t) { return t.id == tokens[1]; });
      if (it == tasks.end()) {
        std::cout << "error: task not found\n";
        continue;
      }
      std::cout << "killed " << it->id << "\n";
      tasks.erase(it);
      continue;
    }

    std::cout << "error: unknown command\n";
  }

  (void)store.close();
  return 0;
}
