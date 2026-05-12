#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ceo/io_reactor.h"
#include "ceo/task_registry.h"
#include "comms/primitives.h"
#include "refract/bootstrap.h"
#include "refract/dispatch.h"
#include "refract/schema_registry.h"
#include "referee/referee.h"
#include "referee_sqlite/sqlite_store.h"
#include "viz/artifacts.h"
#include "vizier/routing.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <variant>

#include <nlohmann/json.hpp>

#if defined(HAVE_READLINE)
#include <readline/history.h>
#include <readline/readline.h>
#endif

#include <unistd.h>

#include "parser/conch_command.h"
#include "parser/json_parser.h"

using iris::refract::SchemaRegistry;
using iris::refract::TypeSummary;
using iris::refract::DispatchEngine;
using iris::refract::OperationScope;
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

struct AliasEntry {
  std::string name;
  ObjectID object_id{};
};

struct IoHandleEntry {
  std::string name;
  iris::conduit::IoHandle handle{};
};

struct IoAliasRecord {
  std::string name;
  iris::conduit::IoHandleKind kind{iris::conduit::IoHandleKind::Channel};
  std::uint64_t handle_id{0};
  bool active{false};
  std::uint64_t created_at{0};
};

struct SessionAlias {
  std::vector<std::string> tokens;
  std::string operation;
};

void print_route_for(iris::refract::SchemaRegistry& registry, referee::TypeID type_id);
void maybe_spawn_concho(iris::refract::SchemaRegistry& registry,
                        referee::SqliteStore& store,
                        const referee::ObjectID& artifact_id);
bool cmd_task_profile(iris::ceo::TaskRegistry& registry,
                      SchemaRegistry& schema,
                      SqliteStore& store);
bool cmd_task_trace(iris::ceo::TaskRegistry& registry,
                    SchemaRegistry& schema,
                    SqliteStore& store,
                    const std::vector<std::string>& args);

const std::vector<SessionAlias>& session_command_aliases() {
  static const std::vector<SessionAlias> aliases = {
    { { "alias" }, "alias_set" },
    { { "aliases" }, "aliases_list" },
    { { "bundle", "export" }, "bundle_export" },
    { { "bundle", "import" }, "bundle_import" },
    { { "let" }, "alias_set" },
    { { "var" }, "alias_set_persistent" },
    { { "call" }, "call" },
    { { "caps", "clear" }, "caps_clear" },
    { { "caps", "grant" }, "caps_grant" },
    { { "caps", "revoke" }, "caps_revoke" },
    { { "caps" }, "caps_list" },
    { { "debug", "dispatch" }, "debug_dispatch" },
    { { "debug", "graph" }, "debug_graph" },
    { { "define", "type" }, "define_type" },
    { { "demo", "v1" }, "demo_v1" },
    { { "edge" }, "edge_add" },
    { { "edges" }, "edges_list" },
    { { "emit" }, "emit_viz" },
    { { "emit", "viz" }, "emit_viz" },
    { { "exit" }, "exit" },
    { { "quit" }, "exit" },
    { { "find", "type" }, "find_type" },
    { { "help" }, "help" },
    { { "io", "alias" }, "io_alias" },
    { { "io", "aliases" }, "io_aliases" },
    { { "io", "await" }, "io_await" },
    { { "io", "close" }, "io_close" },
    { { "io", "handles" }, "io_handles" },
    { { "io", "open" }, "io_open" },
    { { "io", "recv" }, "io_recv" },
    { { "io", "send" }, "io_send" },
    { { "io", "unalias" }, "io_unalias" },
    { { "kill" }, "task_kill" },
    { { "ls" }, "types_list" },
    { { "migrate", "apply" }, "migrate_apply" },
    { { "migrate", "list" }, "migrate_list" },
    { { "migrate", "verify" }, "migrate_verify" },
    { { "new" }, "new_object" },
    { { "namespace" }, "namespace" },
    { { "ns" }, "namespace" },
    { { "objects" }, "objects_list" },
    { { "ops" }, "ops" },
    { { "ps" }, "ps" },
    { { "route", "type" }, "route_type" },
    { { "route" }, "route_object" },
    { { "show", "type" }, "show_type" },
    { { "show" }, "show_object" },
    { { "start" }, "start" },
    { { "task", "list" }, "task_list" },
    { { "task", "spawn" }, "task_spawn" }
  };
  return aliases;
}

bool matches_prefix(const std::vector<std::string>& tokens,
                    const std::vector<std::string>& prefix) {
  if (tokens.size() < prefix.size()) return false;
  for (size_t i = 0; i < prefix.size(); ++i) {
    if (tokens[i] != prefix[i]) return false;
  }
  return true;
}

std::optional<std::string> resolve_session_operation(const iris::parser::CommandAst& cmd) {
  if (cmd.name.empty()) return std::nullopt;
  std::vector<std::string> tokens;
  tokens.reserve(cmd.args.size() + 1);
  tokens.push_back(cmd.name);
  for (const auto& arg : cmd.args) tokens.push_back(arg);

  std::optional<std::string> resolved;
  size_t best_match = 0;
  for (const auto& alias : session_command_aliases()) {
    if (!matches_prefix(tokens, alias.tokens)) continue;
    if (alias.tokens.size() > best_match) {
      best_match = alias.tokens.size();
      resolved = alias.operation;
    }
  }
  return resolved;
}

std::optional<ObjectRef> latest_ref(SqliteStore& store, const ObjectID& id, std::string* err_out);
std::optional<ObjectID> parse_object_id_or_alias(
    const std::string& token,
    const std::unordered_map<std::string, ObjectID>& session_aliases,
    SqliteStore& store,
    SchemaRegistry& schema,
    std::string* err_out);
bool handle_alias_assignment_command(const iris::parser::AliasAssignmentCommand& command,
                                     SchemaRegistry& registry,
                                     SqliteStore& store,
                                     std::unordered_map<std::string, ObjectID>& session_aliases);
struct TypeListOptions;
void cmd_ls(SchemaRegistry& registry,
            SqliteStore& store,
            const TypeListOptions& options);
bool handle_types_list_command(SchemaRegistry& registry,
                               SqliteStore& store,
                               const iris::parser::TypesListCommand& command,
                               const std::string& current_namespace);
bool handle_namespace_family_command(SchemaRegistry& registry,
                                     std::string& current_namespace,
                                     const iris::parser::NamespaceCommand& command);
void cmd_objects(SchemaRegistry& registry, SqliteStore& store);
void cmd_define_type(SchemaRegistry& registry, const std::vector<std::string>& tokens);
void cmd_new_object(SchemaRegistry& registry, SqliteStore& store, const std::string& line);
void cmd_find_type(SchemaRegistry& registry, const std::string& type_name);
void cmd_show_type(SchemaRegistry& registry, const std::string& type_name);
void cmd_ops(SchemaRegistry& registry, const std::vector<std::string>& args);
bool handle_schema_command(SchemaRegistry& registry,
                           SqliteStore& store,
                           const iris::parser::SchemaCommand& command);
void cmd_caps_list(const std::set<std::string>& session_caps);
bool cmd_caps_grant(std::set<std::string>& session_caps, const std::vector<std::string>& args);
bool cmd_caps_revoke(std::set<std::string>& session_caps, const std::vector<std::string>& args);
bool cmd_caps_clear(std::set<std::string>& session_caps, const std::vector<std::string>& args);
bool handle_caps_family_command(std::set<std::string>& session_caps,
                                const iris::parser::CapsCommand& command);
void cmd_show(SchemaRegistry& registry, SqliteStore& store, const ObjectID& id);
void cmd_edges(SqliteStore& store, const ObjectID& id);
bool handle_object_command(SchemaRegistry& registry,
                           SqliteStore& store,
                           const std::unordered_map<std::string, ObjectID>& session_aliases,
                           const iris::parser::ObjectCommand& command);
bool cmd_call(SchemaRegistry& registry,
              SqliteStore& store,
              const ObjectID& id,
              const std::string& op_name,
              const std::vector<std::string>& args,
              const std::unordered_map<std::string, ObjectID>& session_aliases,
              const std::set<std::string>& granted_caps);
bool handle_call_command(SchemaRegistry& registry,
                         SqliteStore& store,
                         const std::unordered_map<std::string, ObjectID>& session_aliases,
                         const std::set<std::string>& granted_caps,
                         const iris::parser::CallCommand& command);
bool cmd_task_spawn(iris::ceo::TaskRegistry& registry,
                    SchemaRegistry& schema,
                    SqliteStore& store,
                    const std::unordered_map<std::string, ObjectID>& session_aliases,
                    const std::vector<std::string>& args);
bool cmd_task_list(iris::ceo::TaskRegistry& registry);
bool cmd_debug_dispatch(SchemaRegistry& registry,
                        SqliteStore& store,
                        const std::unordered_map<std::string, ObjectID>& session_aliases,
                        const std::vector<std::string>& args);
bool cmd_debug_graph(SchemaRegistry& registry,
                     SqliteStore& store,
                     const std::unordered_map<std::string, ObjectID>& session_aliases,
                     const std::vector<std::string>& args);
bool handle_task_family_command(iris::ceo::TaskRegistry& ceo_registry,
                                SchemaRegistry& registry,
                                SqliteStore& store,
                                const std::unordered_map<std::string, ObjectID>& session_aliases,
                                std::vector<TaskEntry>& tasks,
                                std::uint64_t& next_task_id,
                                const iris::parser::TaskCommand& command,
                                const std::set<std::string>& session_caps);
bool cmd_io(iris::conduit::IoExecutor& executor,
            iris::conduit::IoHandleStore& handle_store,
            std::unordered_map<std::string, iris::conduit::IoHandle>& handles,
            std::unordered_map<std::string, iris::conduit::IoHandle>& aliases,
            std::uint64_t& next_handle_id,
            SchemaRegistry& registry,
            SqliteStore& store,
            const std::set<std::string>& granted_caps,
            const std::vector<std::string>& args);
bool handle_io_family_command(iris::conduit::IoExecutor& executor,
                              iris::conduit::IoHandleStore& handle_store,
                              std::unordered_map<std::string, iris::conduit::IoHandle>& handles,
                              std::unordered_map<std::string, iris::conduit::IoHandle>& aliases,
                              std::uint64_t& next_handle_id,
                              SchemaRegistry& registry,
                              SqliteStore& store,
                              const std::set<std::string>& granted_caps,
                              const iris::parser::IoCommand& command);
void cmd_route_type(SchemaRegistry& registry, const std::string& type_name);
void cmd_route_object(SchemaRegistry& registry,
                      SqliteStore& store,
                      const std::unordered_map<std::string, ObjectID>& session_aliases,
                      const std::string& token);
void cmd_edge(SqliteStore& store,
              SchemaRegistry& registry,
              const std::unordered_map<std::string, ObjectID>& session_aliases,
              const std::vector<std::string>& tokens);
void cmd_emit_viz(SchemaRegistry& registry,
                  SqliteStore& store,
                  const std::unordered_map<std::string, ObjectID>& session_aliases,
                  const std::vector<std::string>& tokens);
void cmd_demo_v1(SchemaRegistry& registry,
                 SqliteStore& store,
                 iris::ceo::TaskRegistry& ceo_registry,
                 iris::ceo::TaskComms& ceo_comms,
                 std::unordered_map<std::string, ObjectID>& session_aliases);
void cmd_aliases_list();
std::string join_tokens(const std::vector<std::string>& tokens, size_t start);
std::string make_prompt(const std::string& current_namespace);

struct TypeListOptions {
  std::optional<std::string> filter;
  bool regex_mode = false;
  bool namespaces_only = false;
  bool recursive = false;
  bool include_objects = false;
  std::string current_namespace;
};

bool handle_types_list(SchemaRegistry& registry,
                       SqliteStore& store,
                       const std::vector<std::string>& args,
                       const std::string& current_namespace) {
  TypeListOptions options;
  options.current_namespace = current_namespace;

  bool bad_args = false;
  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i] == "--regex") {
      options.regex_mode = true;
      continue;
    }
    if (args[i] == "--namespaces") {
      options.namespaces_only = true;
      continue;
    }
    if (args[i] == "--recursive") {
      options.recursive = true;
      continue;
    }
    if (args[i] == "--objects") {
      options.include_objects = true;
      continue;
    }
    if (!options.filter.has_value()) {
      options.filter = args[i];
    } else {
      std::cout << "error: unexpected argument\n";
      bad_args = true;
      break;
    }
  }
  if (bad_args) return true;
  if (!args.empty() && !options.filter.has_value() && options.regex_mode) {
    std::cout << "error: --regex requires a pattern\n";
    return true;
  }
  cmd_ls(registry, store, options);
  return true;
}

bool handle_types_list_command(SchemaRegistry& registry,
                               SqliteStore& store,
                               const iris::parser::TypesListCommand& command,
                               const std::string& current_namespace) {
  return handle_types_list(registry, store, command.args, current_namespace);
}

bool handle_namespace_command(SchemaRegistry& registry,
                              std::string& current_namespace,
                              const std::vector<std::string>& args) {
  if (args.size() > 1) {
    std::cout << "error: usage: namespace [<name>|/|.|..]\n";
    return true;
  }

  auto typesR = registry.list_types();
  if (!typesR) {
    std::cout << "error: " << typesR.error->message << "\n";
    return true;
  }

  std::set<std::string> namespaces;
  for (const auto& summary : typesR.value.value()) {
    if (summary.namespace_name.empty()) continue;
    size_t start = 0;
    for (;;) {
      auto pos = summary.namespace_name.find("::", start);
      if (pos == std::string::npos) {
        namespaces.insert(summary.namespace_name);
        break;
      }
      namespaces.insert(summary.namespace_name.substr(0, pos));
      start = pos + 2;
    }
  }

  if (args.empty() || args[0] == ".") {
    std::cout << "namespace "
              << (current_namespace.empty() ? "/" : current_namespace) << "\n";
    return true;
  }

  const auto& target = args[0];
  if (target == "/" || target == "root") {
    current_namespace.clear();
    std::cout << "namespace /\n";
    return true;
  }

  if (target == "..") {
    if (current_namespace.empty()) {
      std::cout << "namespace /\n";
      return true;
    }
    auto pos = current_namespace.rfind("::");
    if (pos == std::string::npos) {
      current_namespace.clear();
    } else {
      current_namespace.erase(pos);
    }
    std::cout << "namespace "
              << (current_namespace.empty() ? "/" : current_namespace) << "\n";
    return true;
  }

  std::vector<std::string> candidates;
  if (target.rfind("::", 0) == 0) {
    candidates.push_back(target.substr(2));
  } else {
    if (!current_namespace.empty()) {
      candidates.push_back(current_namespace + "::" + target);
    }
    candidates.push_back(target);
  }

  for (const auto& candidate : candidates) {
    if (!candidate.empty() && namespaces.find(candidate) != namespaces.end()) {
      current_namespace = candidate;
      std::cout << "namespace " << current_namespace << "\n";
      return true;
    }
  }

  std::cout << "error: namespace not found\n";
  return true;
}

bool handle_namespace_family_command(SchemaRegistry& registry,
                                     std::string& current_namespace,
                                     const iris::parser::NamespaceCommand& command) {
  return handle_namespace_command(registry, current_namespace, command.args);
}

bool handle_caps_command(std::set<std::string>& session_caps,
                         const std::vector<std::string>& args) {
  if (args.empty()) {
    cmd_caps_list(session_caps);
    return true;
  }
  const auto& action = args[0];
  if (action == "grant") {
    cmd_caps_grant(session_caps, args);
    return true;
  }
  if (action == "revoke") {
    cmd_caps_revoke(session_caps, args);
    return true;
  }
  if (action == "clear") {
    cmd_caps_clear(session_caps, args);
    return true;
  }
  std::cout << "error: usage: caps [grant|revoke|clear]\n";
  return true;
}

bool handle_caps_family_command(std::set<std::string>& session_caps,
                                const iris::parser::CapsCommand& command) {
  return handle_caps_command(session_caps, command.args);
}

void cmd_aliases_list() {
  std::vector<std::pair<std::string, std::string>> rows;
  rows.reserve(session_command_aliases().size());
  for (const auto& alias : session_command_aliases()) {
    rows.emplace_back(join_tokens(alias.tokens, 0), alias.operation);
  }
  if (rows.empty()) {
    std::cout << "no aliases\n";
    return;
  }
  std::sort(rows.begin(), rows.end(),
            [](const auto& a, const auto& b) {
              if (a.first != b.first) return a.first < b.first;
              return a.second < b.second;
            });
  std::cout << "aliases\n";
  for (const auto& row : rows) {
    std::cout << "  " << row.first << " -> " << row.second << "\n";
  }
}

bool handle_start_command(SchemaRegistry& registry,
                          SqliteStore& store,
                          const std::unordered_map<std::string, ObjectID>& session_aliases,
                          const std::set<std::string>& session_caps,
                          std::vector<TaskEntry>& tasks,
                          std::uint64_t& next_task_id,
                          const std::vector<std::string>& args) {
  if (args.size() != 1) return false;
  std::string err;
  auto id = parse_object_id_or_alias(args[0], session_aliases, store, registry, &err);
  if (!id.has_value()) {
    std::cout << "error: " << err << "\n";
    return true;
  }
  bool ok = cmd_call(registry, store, id.value(), "start", {}, session_aliases, session_caps);
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
  return true;
}

bool handle_ps_command(const std::vector<TaskEntry>& tasks) {
  if (tasks.empty()) {
    std::cout << "no tasks\n";
    return true;
  }
  for (const auto& task : tasks) {
    std::cout << task.id << " " << task.target.id.to_hex() << " " << task.state << "\n";
  }
  return true;
}

bool handle_kill_command(std::vector<TaskEntry>& tasks,
                         const std::vector<std::string>& args) {
  if (args.size() != 1) return false;
  auto it = std::find_if(tasks.begin(), tasks.end(),
                         [&](const TaskEntry& t) { return t.id == args[0]; });
  if (it == tasks.end()) {
    std::cout << "error: task not found\n";
    return true;
  }
  std::cout << "killed " << it->id << "\n";
  tasks.erase(it);
  return true;
}

bool handle_task_command(iris::ceo::TaskRegistry& ceo_registry,
                         SchemaRegistry& registry,
                         SqliteStore& store,
                         const std::unordered_map<std::string, ObjectID>& session_aliases,
                         const std::vector<std::string>& args) {
  if (args.empty()) {
    std::cout << "error: usage: task <spawn|list|profile|trace>\n";
    return true;
  }
  if (args[0] == "spawn") {
    cmd_task_spawn(ceo_registry, registry, store, session_aliases, args);
    return true;
  }
  if (args[0] == "list") {
    cmd_task_list(ceo_registry);
    return true;
  }
  if (args[0] == "profile") {
    cmd_task_profile(ceo_registry, registry, store);
    return true;
  }
  if (args[0] == "trace") {
    cmd_task_trace(ceo_registry, registry, store, args);
    return true;
  }
  std::cout << "error: usage: task <spawn|list|profile|trace>\n";
  return true;
}

bool handle_task_family_command(iris::ceo::TaskRegistry& ceo_registry,
                                SchemaRegistry& registry,
                                SqliteStore& store,
                                const std::unordered_map<std::string, ObjectID>& session_aliases,
                                std::vector<TaskEntry>& tasks,
                                std::uint64_t& next_task_id,
                                const iris::parser::TaskCommand& command,
                                const std::set<std::string>& session_caps) {
  switch (command.kind) {
    case iris::parser::TaskCommandKind::Start:
      return handle_start_command(registry, store, session_aliases, session_caps,
                                  tasks, next_task_id, command.args);
    case iris::parser::TaskCommandKind::Ps:
      return handle_ps_command(tasks);
    case iris::parser::TaskCommandKind::Kill:
      return handle_kill_command(tasks, command.args);
    case iris::parser::TaskCommandKind::Task:
      return handle_task_command(ceo_registry, registry, store, session_aliases, command.args);
  }
  return false;
}

referee::Result<void> migrate_list(SchemaRegistry& registry,
                                   SqliteStore& store,
                                   const std::string& target);
referee::Result<void> migrate_apply(SchemaRegistry& registry,
                                    SqliteStore& store,
                                    const std::string& target);
referee::Result<void> migrate_verify(SchemaRegistry& registry,
                                     SqliteStore& store,
                                     const std::string& target);
referee::Result<void> export_bundle(SchemaRegistry& registry,
                                    SqliteStore& store,
                                    const std::string& path);
referee::Result<void> import_bundle(SchemaRegistry& registry,
                                    SqliteStore& store,
                                    const std::string& path);

bool handle_session_operation(const std::string& line,
                              const iris::parser::CommandAst& parsed,
                              const std::string& op,
                              SchemaRegistry& registry,
                              SqliteStore& store,
                              std::string& current_namespace,
                              std::unordered_map<std::string, ObjectID>& session_aliases,
                              std::set<std::string>& session_caps,
                              std::vector<TaskEntry>& tasks,
                              std::uint64_t& next_task_id,
                              iris::ceo::TaskRegistry& ceo_registry,
                              iris::conduit::IoExecutor& io_executor,
                              iris::conduit::IoHandleStore& io_handle_store,
                              std::unordered_map<std::string, iris::conduit::IoHandle>& io_handles,
                              std::unordered_map<std::string, iris::conduit::IoHandle>& io_handle_aliases,
                              std::uint64_t& next_io_handle_id,
                              iris::ceo::TaskComms& ceo_comms) {
  if (op == "types_list") {
    return handle_types_list(registry, store, parsed.args, current_namespace);
  }
  if (op == "namespace") {
    return handle_namespace_command(registry, current_namespace, parsed.args);
  }
  if (op == "bundle_export") {
    if (parsed.args.size() != 2 || parsed.args[0] != "export") {
      std::cout << "error: usage: bundle export <path>\n";
      return true;
    }
    auto exportR = export_bundle(registry, store, parsed.args[1]);
    if (!exportR) {
      std::cout << "error: " << exportR.error->message << "\n";
      return true;
    }
    std::cout << "bundle export ok\n";
    return true;
  }
  if (op == "bundle_import") {
    if (parsed.args.size() != 2 || parsed.args[0] != "import") {
      std::cout << "error: usage: bundle import <path>\n";
      return true;
    }
    auto importR = import_bundle(registry, store, parsed.args[1]);
    if (!importR) {
      std::cout << "error: " << importR.error->message << "\n";
      return true;
    }
    std::cout << "bundle import ok\n";
    return true;
  }
  if (op == "migrate_list") {
    if (parsed.args.size() != 2 || parsed.args[0] != "list") {
      std::cout << "error: usage: migrate list <TypeName|DefinitionID>\n";
      return true;
    }
    auto listR = migrate_list(registry, store, parsed.args[1]);
    if (!listR) {
      std::cout << "error: " << listR.error->message << "\n";
    }
    return true;
  }
  if (op == "migrate_apply") {
    if (parsed.args.size() != 2 || parsed.args[0] != "apply") {
      std::cout << "error: usage: migrate apply <TypeName|DefinitionID>\n";
      return true;
    }
    auto applyR = migrate_apply(registry, store, parsed.args[1]);
    if (!applyR) {
      std::cout << "error: " << applyR.error->message << "\n";
      return true;
    }
    std::cout << "migrate apply ok\n";
    return true;
  }
  if (op == "migrate_verify") {
    if (parsed.args.size() != 2 || parsed.args[0] != "verify") {
      std::cout << "error: usage: migrate verify <TypeName|DefinitionID>\n";
      return true;
    }
    auto verifyR = migrate_verify(registry, store, parsed.args[1]);
    if (!verifyR) {
      std::cout << "error: " << verifyR.error->message << "\n";
      return true;
    }
    std::cout << "migrate verify ok\n";
    return true;
  }
  if (op == "aliases_list") {
    if (!parsed.args.empty()) {
      std::cout << "error: usage: aliases\n";
      return true;
    }
    cmd_aliases_list();
    return true;
  }
  if (op == "objects_list") {
    cmd_objects(registry, store);
    return true;
  }
  if (op == "debug_dispatch") {
    std::vector<std::string> args = parsed.args;
    if (!args.empty() && args[0] == "dispatch") {
      args.erase(args.begin());
    }
    cmd_debug_dispatch(registry, store, session_aliases, args);
    return true;
  }
  if (op == "debug_graph") {
    std::vector<std::string> args = parsed.args;
    if (!args.empty() && args[0] == "graph") {
      args.erase(args.begin());
    }
    cmd_debug_graph(registry, store, session_aliases, args);
    return true;
  }
  if (op == "define_type") {
    if (parsed.args.size() >= 2 && parsed.args[0] == "type") {
      std::vector<std::string> tokens;
      tokens.reserve(parsed.args.size() + 1);
      tokens.push_back(parsed.name);
      tokens.insert(tokens.end(), parsed.args.begin(), parsed.args.end());
      cmd_define_type(registry, tokens);
      return true;
    }
    return false;
  }
  if (op == "new_object") {
    cmd_new_object(registry, store, line);
    return true;
  }
  if (op == "find_type") {
    if (parsed.args.size() >= 2 && parsed.args[0] == "type") {
      cmd_find_type(registry, parsed.args[1]);
      return true;
    }
    return false;
  }
  if (op == "show_type") {
    if (parsed.args.size() == 2 && parsed.args[0] == "type") {
      cmd_show_type(registry, parsed.args[1]);
      return true;
    }
    return false;
  }
  if (op == "show_object") {
    if (parsed.args.size() == 1) {
      std::string err;
      auto id = parse_object_id_or_alias(parsed.args[0], session_aliases, store, registry, &err);
      if (!id.has_value()) {
        std::cout << "error: " << err << "\n";
        return true;
      }
      cmd_show(registry, store, id.value());
      return true;
    }
    return false;
  }
  if (op == "ops") {
    if (parsed.args.empty()) return false;
    cmd_ops(registry, parsed.args);
    return true;
  }
  if (op == "caps_list" || op == "caps_grant" || op == "caps_revoke" || op == "caps_clear") {
    return handle_caps_command(session_caps, parsed.args);
  }
  if (op == "edges_list") {
    if (parsed.args.size() != 1) return false;
    std::string err;
    auto id = parse_object_id_or_alias(parsed.args[0], session_aliases, store, registry, &err);
    if (!id.has_value()) {
      std::cout << "error: " << err << "\n";
      return true;
    }
    cmd_edges(store, id.value());
    return true;
  }
  if (op == "call") {
    if (parsed.args.size() < 2) return false;
    std::string err;
    auto id = parse_object_id_or_alias(parsed.args[0], session_aliases, store, registry, &err);
    if (!id.has_value()) {
      std::cout << "error: " << err << "\n";
      return true;
    }
    std::vector<std::string> args;
    if (parsed.args.size() > 2) {
      args.assign(parsed.args.begin() + 2, parsed.args.end());
    }
    cmd_call(registry, store, id.value(), parsed.args[1], args, session_aliases, session_caps);
    return true;
  }
  if (op == "start") {
    return handle_start_command(registry, store, session_aliases, session_caps,
                                tasks, next_task_id, parsed.args);
  }
  if (op == "ps") {
    return handle_ps_command(tasks);
  }
  if (op == "task_kill") {
    return handle_kill_command(tasks, parsed.args);
  }
  if (op == "task_list" || op == "task_spawn") {
    return handle_task_command(ceo_registry, registry, store, session_aliases, parsed.args);
  }
  if (op == "io_alias" || op == "io_aliases" || op == "io_await" || op == "io_close"
      || op == "io_handles" || op == "io_open" || op == "io_recv" || op == "io_send"
      || op == "io_unalias") {
    cmd_io(io_executor, io_handle_store, io_handles, io_handle_aliases,
           next_io_handle_id, registry, store, session_caps, parsed.args);
    return true;
  }
  if (op == "route_type") {
    if (parsed.args.size() == 2 && parsed.args[0] == "type") {
      cmd_route_type(registry, parsed.args[1]);
      return true;
    }
    std::cout << "error: usage: route <ObjectID> | route type <TypeName>\n";
    return true;
  }
  if (op == "route_object") {
    if (parsed.args.size() == 1) {
      cmd_route_object(registry, store, session_aliases, parsed.args[0]);
      return true;
    }
    std::cout << "error: usage: route <ObjectID> | route type <TypeName>\n";
    return true;
  }
  if (op == "edge_add") {
    std::vector<std::string> tokens;
    tokens.reserve(parsed.args.size() + 1);
    tokens.push_back(parsed.name);
    tokens.insert(tokens.end(), parsed.args.begin(), parsed.args.end());
    cmd_edge(store, registry, session_aliases, tokens);
    return true;
  }
  if (op == "emit_viz") {
    std::vector<std::string> tokens;
    tokens.reserve(parsed.args.size() + 1);
    tokens.push_back(parsed.name);
    tokens.insert(tokens.end(), parsed.args.begin(), parsed.args.end());
    cmd_emit_viz(registry, store, session_aliases, tokens);
    return true;
  }
  if (op == "demo_v1") {
    cmd_demo_v1(registry, store, ceo_registry, ceo_comms, session_aliases);
    return true;
  }
  return false;
}

referee::Result<ObjectID> create_object(SchemaRegistry& registry, SqliteStore& store,
                                        const std::string& expr);
referee::Result<ObjectID> create_demo_object(SchemaRegistry& registry, SqliteStore& store,
                                             const std::string& type_name,
                                             const nlohmann::json& payload);
referee::Result<ObjectID> demo_start(SchemaRegistry& registry, SqliteStore& store,
                                     const ObjectID& demo_id);
referee::Result<void> demo_expand(SchemaRegistry& registry, SqliteStore& store,
                                  const ObjectID& summary_id, std::uint64_t level);
bool parse_bool(std::string_view v, bool* out);
bool parse_int(std::string_view v, std::int64_t* out);
bool parse_double(std::string_view v, double* out);
bool parse_u64(std::string_view v, std::uint64_t* out);
std::string bytes_to_hex(const std::vector<std::uint8_t>& bytes);
bool parse_hex_bytes(std::string_view v, std::vector<std::uint8_t>* out, std::string* err_out);
bool value_to_json(const iris::parser::ValueNode& node, nlohmann::json* out,
                   std::string* err_out);

std::optional<std::string> read_line(const char* prompt) {
#if defined(HAVE_READLINE)
  if (!::isatty(STDIN_FILENO)) {
    std::string line;
    if (!std::getline(std::cin, line)) return std::nullopt;
    return line;
  }
  std::cout << prompt;
  std::cout.flush();
  rl_already_prompted = 1;
  char* input = readline("");
  if (!input) return std::nullopt;
  if (*input != '\0') add_history(input);
  std::string line(input);
  std::free(input);
  return line;
#else
  if (::isatty(STDIN_FILENO)) {
    std::cout << prompt;
    std::cout.flush();
  }
  std::string line;
  if (!std::getline(std::cin, line)) return std::nullopt;
  return line;
#endif
}

std::string join_tokens(const std::vector<std::string>& tokens, size_t start) {
  std::ostringstream os;
  for (size_t i = start; i < tokens.size(); ++i) {
    if (i > start) os << ' ';
    os << tokens[i];
  }
  return os.str();
}

std::string strip_quotes(std::string value) {
  if (value.size() >= 2) {
    char first = value.front();
    char last = value.back();
    if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
      return value.substr(1, value.size() - 2);
    }
  }
  return value;
}

std::string trim_copy(std::string value) {
  auto not_space = [](unsigned char c) { return !std::isspace(c); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
  return value;
}

bool parse_kv_payload(const std::string& text, nlohmann::json* payload, std::string* err_out) {
  size_t i = 0;
  auto fail = [&](const std::string& msg) {
    if (err_out) *err_out = msg;
    return false;
  };

  while (i < text.size()) {
    while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
    if (i >= text.size()) break;

    size_t name_start = i;
    while (i < text.size() && !std::isspace(static_cast<unsigned char>(text[i]))
           && text[i] != ':' && text[i] != '=') {
      ++i;
    }
    if (i == name_start) return fail("expected field:=value");
    std::string field = text.substr(name_start, i - name_start);

    while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
    if (i >= text.size() || text[i] != ':') return fail("expected field:=value");
    ++i;
    while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
    if (i >= text.size() || text[i] != '=') return fail("expected field:=value");
    ++i;

    while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
    if (i >= text.size()) return fail("expected field:=value");

    std::string value;
    char quote = text[i];
    if (quote == '"' || quote == '\'') {
      ++i;
      while (i < text.size()) {
        char c = text[i];
        if (c == quote) {
          ++i;
          break;
        }
        if (c == '\\' && i + 1 < text.size()) {
          value.push_back(text[i + 1]);
          i += 2;
          continue;
        }
        value.push_back(c);
        ++i;
      }
      if (i > text.size() || (i == text.size() && (text.empty() || text.back() != quote))) {
        return fail("unterminated quoted value");
      }
    } else {
      size_t value_start = i;
      while (i < text.size() && !std::isspace(static_cast<unsigned char>(text[i]))) ++i;
      value = text.substr(value_start, i - value_start);
    }

    bool b = false;
    std::int64_t n = 0;
    if (parse_bool(std::string_view(value), &b)) {
      (*payload)[field] = b;
    } else if (parse_int(std::string_view(value), &n)) {
      (*payload)[field] = n;
    } else {
      (*payload)[field] = value;
    }
  }
  return true;
}

bool value_to_json(const iris::parser::ValueNode& node, nlohmann::json* out,
                   std::string* err_out) {
  if (!out) return false;
  auto fail = [&](const std::string& msg) {
    if (err_out) *err_out = msg;
    return false;
  };

  const auto& value = node.value;
  if (std::holds_alternative<std::monostate>(value)) {
    *out = nullptr;
    return true;
  }
  if (std::holds_alternative<bool>(value)) {
    *out = std::get<bool>(value);
    return true;
  }
  if (std::holds_alternative<double>(value)) {
    *out = std::get<double>(value);
    return true;
  }
  if (std::holds_alternative<std::string>(value)) {
    *out = std::get<std::string>(value);
    return true;
  }
  if (std::holds_alternative<iris::parser::ValueArray>(value)) {
    nlohmann::json arr = nlohmann::json::array();
    const auto& items = std::get<iris::parser::ValueArray>(value);
    for (const auto& item : items) {
      nlohmann::json child;
      if (!value_to_json(item, &child, err_out)) return false;
      arr.push_back(std::move(child));
    }
    *out = std::move(arr);
    return true;
  }
  if (std::holds_alternative<iris::parser::ValueObject>(value)) {
    nlohmann::json obj = nlohmann::json::object();
    const auto& items = std::get<iris::parser::ValueObject>(value);
    for (const auto& [key, child_node] : items) {
      nlohmann::json child;
      if (!value_to_json(child_node, &child, err_out)) return false;
      obj[key] = std::move(child);
    }
    *out = std::move(obj);
    return true;
  }
  return fail("unsupported json value");
}

referee::Result<void> parse_new_expr(const std::string& expr,
                                     std::string* type_name_out,
                                     nlohmann::json* payload_out) {
  std::string err;
  auto s = trim_copy(expr);
  if (s.rfind("new", 0) != 0) {
    return referee::Result<void>::err("usage: new <TypeName> field:=value ...");
  }
  s = trim_copy(s.substr(3));
  if (s.empty()) {
    return referee::Result<void>::err("usage: new <TypeName> field:=value ...");
  }

  if (s.rfind("--json", 0) == 0) {
    auto json_text = trim_copy(s.substr(6));
    json_text = strip_quotes(json_text);
    auto parsed = iris::parser::parse_json(json_text);
    if (!parsed.errors.empty()) {
      const auto& err0 = parsed.errors.front();
      std::ostringstream os;
      os << "json parse error: " << err0.message
         << " at " << err0.line << ":" << err0.column;
      return referee::Result<void>::err(os.str());
    }
    if (!parsed.value.has_value()) {
      return referee::Result<void>::err("json parse error");
    }
    nlohmann::json j;
    if (!value_to_json(parsed.value.value(), &j, &err)) {
      return referee::Result<void>::err(err.empty() ? "json parse error" : err);
    }
    auto type_name = j.value("type", "");
    if (type_name.empty()) {
      return referee::Result<void>::err("json missing type");
    }
    *type_name_out = type_name;
    if (j.contains("payload")) {
      *payload_out = j.at("payload");
    } else {
      *payload_out = nlohmann::json::object();
    }
    return referee::Result<void>::ok();
  }

  size_t i = 0;
  while (i < s.size() && !std::isspace(static_cast<unsigned char>(s[i]))) ++i;
  *type_name_out = s.substr(0, i);
  if (type_name_out->empty()) {
    return referee::Result<void>::err("usage: new <TypeName> field:=value ...");
  }
  auto rest = trim_copy(s.substr(i));
  *payload_out = nlohmann::json::object();
  if (rest.empty()) return referee::Result<void>::ok();

  if (!parse_kv_payload(rest, payload_out, &err)) {
    return referee::Result<void>::err(err);
  }
  return referee::Result<void>::ok();
}

bool field_has_constraint(const iris::refract::FieldDefinition& field,
                          iris::refract::FieldConstraintKind kind) {
  for (const auto& constraint : field.constraints) {
    if (constraint.kind == kind) return true;
  }
  return false;
}

bool json_value_is_empty(const nlohmann::json& value) {
  if (value.is_string()) return value.get_ref<const std::string&>().empty();
  if (value.is_array() || value.is_object()) return value.empty();
  return false;
}

referee::Result<void> validate_payload_constraints(const iris::refract::TypeDefinition& def,
                                                   const nlohmann::json& payload) {
  if (!payload.is_object()) {
    return referee::Result<void>::err("payload must be a JSON object");
  }

  for (const auto& field : def.fields) {
    const bool required = field_has_constraint(field, iris::refract::FieldConstraintKind::Required);
    auto it = payload.find(field.name);
    bool present = it != payload.end() && !it->is_null();

    if (required && !present) {
      return referee::Result<void>::err("missing required field '" + field.name + "'");
    }
    if (!present) continue;

    if (field_has_constraint(field, iris::refract::FieldConstraintKind::NonEmpty)
        && json_value_is_empty(*it)) {
      return referee::Result<void>::err("field '" + field.name + "' must be non-empty");
    }
  }

  return referee::Result<void>::ok();
}

std::string type_display_name(const TypeSummary& summary) {
  if (summary.namespace_name.empty()) return summary.name;
  return summary.namespace_name + "::" + summary.name;
}

bool namespace_in_scope(std::string_view current_namespace, std::string_view candidate_namespace) {
  if (current_namespace.empty()) return true;
  if (candidate_namespace == current_namespace) return true;
  if (candidate_namespace.size() <= current_namespace.size()) return false;
  if (candidate_namespace.compare(0, current_namespace.size(), current_namespace) != 0) return false;
  return candidate_namespace.substr(current_namespace.size(), 2) == "::";
}

std::optional<std::string> direct_child_namespace(std::string_view current_namespace,
                                                  std::string_view candidate_namespace) {
  if (candidate_namespace.empty()) return std::nullopt;

  std::string_view remainder = candidate_namespace;
  if (!current_namespace.empty()) {
    if (candidate_namespace.size() <= current_namespace.size()) return std::nullopt;
    if (candidate_namespace.compare(0, current_namespace.size(), current_namespace) != 0) {
      return std::nullopt;
    }
    if (candidate_namespace.substr(current_namespace.size(), 2) != "::") return std::nullopt;
    remainder = candidate_namespace.substr(current_namespace.size() + 2);
  }

  auto pos = remainder.find("::");
  return std::string(remainder.substr(0, pos));
}

std::string relative_type_name(std::string_view current_namespace, const TypeSummary& summary) {
  if (current_namespace.empty() || summary.namespace_name != current_namespace) {
    return type_display_name(summary);
  }
  return summary.name;
}

bool match_pattern(std::string_view value, std::string_view pattern, bool regex_mode,
                   std::string* err_out);

bool listing_matches(const std::string& value,
                     const std::optional<std::string>& filter,
                     bool regex_mode,
                     bool* matched,
                     std::string* err_out) {
  if (!filter.has_value()) {
    *matched = true;
    return true;
  }
  *matched = match_pattern(value, *filter, regex_mode, err_out);
  if (!*matched && err_out && !err_out->empty()) return false;
  return true;
}

std::string glob_to_regex(std::string_view pattern) {
  std::string out;
  out.reserve(pattern.size() * 2);
  out.push_back('^');
  for (char c : pattern) {
    switch (c) {
      case '*': out.append(".*"); break;
      case '?': out.push_back('.'); break;
      case '.': case '+': case '(': case ')': case '[': case ']':
      case '{': case '}': case '^': case '$': case '|': case '\\':
        out.push_back('\\');
        out.push_back(c);
        break;
      default:
        out.push_back(c);
        break;
    }
  }
  out.push_back('$');
  return out;
}

bool match_pattern(std::string_view value, std::string_view pattern, bool regex_mode, std::string* err_out) {
  try {
    std::string expr = regex_mode ? std::string(pattern) : glob_to_regex(pattern);
    std::regex re(expr);
    return std::regex_match(value.begin(), value.end(), re);
  } catch (const std::exception& ex) {
    if (err_out) *err_out = ex.what();
    return false;
  }
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

std::optional<ObjectID> parse_object_id_or_alias(
    const std::string& token,
    const std::unordered_map<std::string, ObjectID>& session_aliases,
    SqliteStore& store,
    SchemaRegistry& registry,
    std::string* err_out) {
  std::string id_err;
  auto parsed = parse_object_id(token, &id_err);
  if (parsed.has_value()) return parsed;
  if (token.size() == 32 && !id_err.empty()) {
    if (err_out) *err_out = id_err;
    return std::nullopt;
  }

  std::string name = token;
  if (!name.empty() && name.front() == '@') name = name.substr(1);
  if (name.empty()) {
    if (err_out) *err_out = "empty alias";
    return std::nullopt;
  }
  auto it = session_aliases.find(name);
  if (it != session_aliases.end()) return it->second;

  auto typesR = registry.list_types();
  if (!typesR) {
    if (err_out) *err_out = typesR.error->message;
    return std::nullopt;
  }
  std::optional<TypeSummary> alias_type;
  for (const auto& summary : typesR.value.value()) {
    if (summary.namespace_name == "Conch" && summary.name == "Alias") {
      alias_type = summary;
      break;
    }
  }
  if (!alias_type.has_value()) {
    if (err_out) *err_out = "alias type not registered";
    return std::nullopt;
  }

  auto listR = store.list_by_type(alias_type->type_id);
  if (!listR) {
    if (err_out) *err_out = listR.error->message;
    return std::nullopt;
  }
  for (const auto& rec : listR.value.value()) {
    try {
      auto json = nlohmann::json::from_cbor(rec.payload_cbor);
      if (json.value("name", "") == name) {
        auto oid_text = json.value("object_id", "");
        if (oid_text.empty()) continue;
        return parse_object_id(oid_text, err_out);
      }
    } catch (const std::exception&) {
      continue;
    }
  }

  if (name.size() >= 4 && name.size() < 32 && name.front() != '@') {
    bool hex_only = true;
    for (char c : name) {
      if (!std::isxdigit(static_cast<unsigned char>(c))) {
        hex_only = false;
        break;
      }
    }
    if (hex_only) {
      std::optional<ObjectID> match;
      bool ambiguous = false;
      for (const auto& summary : typesR.value.value()) {
        auto objsR = store.list_by_type(summary.type_id);
        if (!objsR) {
          if (err_out) *err_out = objsR.error->message;
          return std::nullopt;
        }
        for (const auto& rec : objsR.value.value()) {
          auto hex = rec.ref.id.to_hex();
          if (hex.rfind(name, 0) == 0) {
            if (match.has_value() && match->to_hex() != hex) {
              ambiguous = true;
              break;
            }
            match = rec.ref.id;
          }
        }
        if (ambiguous) break;
      }
      if (ambiguous) {
        if (err_out) *err_out = "ambiguous ObjectID prefix";
        return std::nullopt;
      }
      if (match.has_value()) return match;
      if (err_out) *err_out = "ObjectID prefix not found";
      return std::nullopt;
    }
  }

  if (err_out) *err_out = "alias not found";
  return std::nullopt;
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

std::pair<std::string, std::string> split_type_name(const std::string& full) {
  auto pos = full.find("::");
  if (pos == std::string::npos) return {"", full};
  return {full.substr(0, pos), full.substr(pos + 2)};
}

std::uint64_t fnv1a_64(std::string_view input) {
  std::uint64_t hash = 1469598103934665603ULL;
  for (unsigned char c : input) {
    hash ^= c;
    hash *= 1099511628211ULL;
  }
  if (hash == 0) hash = 1;
  return hash;
}

std::optional<TypeSummary> resolve_type(SchemaRegistry& registry,
                                        const std::string& name,
                                        std::string* err_out) {
  auto typesR = registry.list_types();
  if (!typesR) {
    if (err_out) *err_out = typesR.error->message;
    return std::nullopt;
  }
  return find_type_summary(typesR.value.value(), name, err_out);
}

struct OperationListing {
  iris::refract::OperationDefinition operation;
  TypeID owner{};
  std::size_t depth{0};
};

std::vector<TypeID> resolve_base_types(SchemaRegistry& registry,
                                       TypeID type_id,
                                       std::string* err_out) {
  auto basesR = registry.list_base_types(type_id);
  if (!basesR) {
    if (err_out) *err_out = basesR.error->message;
    return {};
  }
  return basesR.value.value();
}

std::vector<TypeID> resolve_interface_types(SchemaRegistry& registry,
                                            TypeID type_id,
                                            std::string* err_out) {
  auto interfacesR = registry.list_interface_types(type_id);
  if (!interfacesR) {
    if (err_out) *err_out = interfacesR.error->message;
    return {};
  }
  return interfacesR.value.value();
}

referee::Result<std::vector<OperationListing>> list_operations_with_inheritance(
    SchemaRegistry& registry,
    TypeID root_type,
    bool include_inherited) {
  std::vector<OperationListing> out;
  std::deque<std::pair<TypeID, std::size_t>> queue;
  std::unordered_set<std::uint64_t> visited;

  queue.push_back({ root_type, 0 });
  visited.insert(root_type.v);

  while (!queue.empty()) {
    auto [current, depth] = queue.front();
    queue.pop_front();

    auto defR = registry.get_latest_definition_by_type(current);
    if (!defR) {
      return referee::Result<std::vector<OperationListing>>::err(defR.error->message);
    }
    if (!defR.value->has_value()) {
      return referee::Result<std::vector<OperationListing>>::err("definition not found");
    }

    const auto& def = defR.value->value().definition;
    for (const auto& op : def.operations) {
      OperationListing entry;
      entry.operation = op;
      entry.owner = current;
      entry.depth = depth;
      out.push_back(std::move(entry));
    }

    if (include_inherited) {
      auto parentsR = registry.list_supertypes(current);
      if (!parentsR) {
        return referee::Result<std::vector<OperationListing>>::err(parentsR.error->message);
      }
      for (const auto& base : parentsR.value.value()) {
        if (visited.insert(base.v).second) {
          queue.push_back({ base, depth + 1 });
        }
      }
    }
  }

  return referee::Result<std::vector<OperationListing>>::ok(std::move(out));
}

std::string type_display_name_for(const std::vector<TypeSummary>& types, TypeID type_id) {
  for (const auto& t : types) {
    if (t.type_id.v == type_id.v) return type_display_name(t);
  }
  std::ostringstream os;
  os << "0x" << std::hex << type_id.v << std::dec;
  return os.str();
}

void print_type_line(const char* label,
                     const std::vector<TypeSummary>& types,
                     const std::vector<TypeID>& type_ids) {
  if (type_ids.empty()) return;
  std::cout << label;
  for (const auto& type_id : type_ids) {
    std::cout << " " << type_display_name_for(types, type_id);
  }
  std::cout << "\n";
}

std::string format_signature(const iris::refract::OperationDefinition& op) {
  std::ostringstream os;
  os << "(";
  for (size_t i = 0; i < op.signature.params.size(); ++i) {
    if (i > 0) os << ", ";
    const auto& param = op.signature.params[i];
    if (!param.name.empty()) os << param.name << ":";
    os << "0x" << std::hex << param.type.v << std::dec;
    if (param.optional) os << "?";
  }
  os << ")";
  if (!op.signature.outputs.empty()) {
    os << " -> ";
    if (op.signature.outputs.size() > 1) os << "(";
    for (size_t i = 0; i < op.signature.outputs.size(); ++i) {
      if (i > 0) os << ", ";
      const auto& out = op.signature.outputs[i];
      if (!out.name.empty()) {
        os << out.name << ":";
      }
      os << "0x" << std::hex << out.type.v << std::dec;
      if (out.optional) os << "?";
    }
    if (op.signature.outputs.size() > 1) os << ")";
  }
  return os.str();
}

struct DispatchCandidate {
  OperationListing listing;
  std::size_t type_penalty{0};
  std::size_t optional_penalty{0};
};

bool matches_arity(const iris::refract::OperationDefinition& op, std::size_t arg_count) {
  std::size_t required = 0;
  for (const auto& param : op.signature.params) {
    if (!param.optional) ++required;
  }
  if (arg_count < required) return false;
  if (arg_count > op.signature.params.size()) return false;
  return true;
}

bool has_base_type(TypeID type_id,
                   TypeID base_id,
                   SchemaRegistry& registry,
                   std::string* err_out) {
  if (type_id.v == base_id.v) return true;
  std::deque<TypeID> queue;
  std::unordered_set<std::uint64_t> visited;
  queue.push_back(type_id);
  visited.insert(type_id.v);

  while (!queue.empty()) {
    auto current = queue.front();
    queue.pop_front();

    auto defR = registry.get_latest_definition_by_type(current);
    if (!defR) {
      if (err_out) *err_out = defR.error->message;
      return false;
    }
    if (!defR.value->has_value()) {
      return false;
    }
    auto basesR = registry.list_supertypes(current);
    if (!basesR) {
      if (err_out) *err_out = basesR.error->message;
      return false;
    }
    for (const auto& base : basesR.value.value()) {
      if (base.v == base_id.v) return true;
      if (visited.insert(base.v).second) queue.push_back(base);
    }
  }
  return false;
}

std::string format_capabilities(const iris::refract::OperationDefinition& op) {
  if (op.required_capabilities.empty()) return "";
  std::ostringstream os;
  os << " [caps: ";
  for (size_t i = 0; i < op.required_capabilities.size(); ++i) {
    if (i > 0) os << ", ";
    os << op.required_capabilities[i];
  }
  os << "]";
  return os.str();
}

constexpr TypeID kTypeU64{0x1002ULL};
constexpr TypeID kTypeBytes{0x1007ULL};
constexpr TypeID kTypeKernelIo{0x4B494F5000000001ULL};
constexpr TypeID kTypeKernelIoChannel{0x4B494F5000000002ULL};
constexpr TypeID kTypeKernelIoDatagram{0x4B494F5000000003ULL};

std::string format_task_state(iris::ceo::TaskState state) {
  return iris::ceo::to_string(state);
}

const char* format_task_mode(iris::ceo::TaskMode mode) {
  switch (mode) {
    case iris::ceo::TaskMode::Inline: return "inline";
    case iris::ceo::TaskMode::Service: return "service";
  }
  return "unknown";
}

std::string format_duration_ms(std::uint64_t ns) {
  return std::to_string(ns / 1000000ULL);
}

std::string io_handle_name(std::uint64_t id) {
  std::ostringstream os;
  os << "io-" << std::setw(4) << std::setfill('0') << id;
  return os.str();
}

const char* io_kind_name(iris::conduit::IoHandleKind kind) {
  switch (kind) {
    case iris::conduit::IoHandleKind::Channel: return "channel";
    case iris::conduit::IoHandleKind::Datagram: return "datagram";
    case iris::conduit::IoHandleKind::ByteStream: return "stream";
  }
  return "unknown";
}

std::optional<iris::conduit::IoHandleKind> io_kind_from_string(const std::string& text) {
  if (text == "channel") return iris::conduit::IoHandleKind::Channel;
  if (text == "datagram") return iris::conduit::IoHandleKind::Datagram;
  if (text == "stream") return iris::conduit::IoHandleKind::ByteStream;
  return std::nullopt;
}

bool parse_task_mode(const std::string& token, iris::ceo::TaskMode* out) {
  if (token == "inline") {
    *out = iris::ceo::TaskMode::Inline;
    return true;
  }
  if (token == "service") {
    *out = iris::ceo::TaskMode::Service;
    return true;
  }
  return false;
}

bool has_required_capabilities(const iris::refract::OperationDefinition& op,
                               const std::set<std::string>& granted,
                               std::string* err_out) {
  if (op.required_capabilities.empty()) return true;
  for (const auto& required : op.required_capabilities) {
    if (granted.find(required) == granted.end()) {
      if (err_out) *err_out = "missing capability: " + required;
      return false;
    }
  }
  return true;
}

void cmd_caps_list(const std::set<std::string>& caps) {
  if (caps.empty()) {
    std::cout << "caps: (none)\n";
    return;
  }
  std::cout << "caps\n";
  for (const auto& cap : caps) {
    std::cout << "  " << cap << "\n";
  }
}

bool cmd_caps_grant(std::set<std::string>& caps, const std::vector<std::string>& args) {
  if (args.size() < 2) {
    std::cout << "error: usage: caps grant <cap> [cap...]\n";
    return false;
  }
  for (size_t i = 1; i < args.size(); ++i) {
    caps.insert(args[i]);
  }
  return true;
}

bool cmd_caps_revoke(std::set<std::string>& caps, const std::vector<std::string>& args) {
  if (args.size() < 2) {
    std::cout << "error: usage: caps revoke <cap> [cap...]\n";
    return false;
  }
  for (size_t i = 1; i < args.size(); ++i) {
    caps.erase(args[i]);
  }
  return true;
}

bool cmd_caps_clear(std::set<std::string>& caps, const std::vector<std::string>& args) {
  if (args.size() != 1) {
    std::cout << "error: usage: caps clear\n";
    return false;
  }
  caps.clear();
  return true;
}

std::string format_task_list(const std::vector<iris::ceo::TaskRecord>& tasks) {
  std::ostringstream os;
  for (const auto& task : tasks) {
    os << "task " << task.id << " " << format_task_state(task.state)
       << " object=" << task.object_id.to_hex();
    if (!task.name.empty()) os << " name=" << task.name;
    os << "\n";
  }
  return os.str();
}

bool cmd_task_spawn(iris::ceo::TaskRegistry& registry,
                    SchemaRegistry& schema,
                    SqliteStore& store,
                    const std::unordered_map<std::string, ObjectID>& session_aliases,
                    const std::vector<std::string>& args) {
  if (args.size() < 2) {
    std::cout << "error: usage: task spawn <ObjectID> [name] [inline|service]\n";
    return false;
  }
  std::string err;
  auto id = parse_object_id_or_alias(args[1], session_aliases, store, schema, &err);
  if (!id.has_value()) {
    std::cout << "error: " << err << "\n";
    return false;
  }

  std::string name;
  iris::ceo::TaskMode mode = iris::ceo::TaskMode::Inline;
  if (args.size() >= 3) {
    if (parse_task_mode(args[2], &mode)) {
      name = "";
    } else {
      name = args[2];
    }
  }
  if (args.size() >= 4) {
    if (!parse_task_mode(args[3], &mode)) {
      std::cout << "error: invalid task mode\n";
      return false;
    }
  }
  if (args.size() > 4) {
    std::cout << "error: usage: task spawn <ObjectID> [name] [inline|service]\n";
    return false;
  }

  auto taskR = registry.spawn_task(id.value(), std::nullopt, name, mode);
  if (!taskR) {
    std::cout << "error: " << taskR.error->message << "\n";
    return false;
  }
  std::cout << "task " << taskR.value->id << " " << format_task_state(taskR.value->state) << "\n";
  return true;
}

bool cmd_task_list(iris::ceo::TaskRegistry& registry) {
  auto listR = registry.list_tasks();
  if (!listR) {
    std::cout << "error: " << listR.error->message << "\n";
    return false;
  }
  if (listR.value->empty()) {
    std::cout << "no tasks\n";
    return true;
  }
  std::cout << format_task_list(listR.value.value());
  return true;
}

bool cmd_task_profile(iris::ceo::TaskRegistry& registry,
                      SchemaRegistry& schema,
                      SqliteStore& store) {
  auto snapshot = registry.profile_snapshot();
  iris::viz::Table table;
  table.columns = {"task_id", "state", "mode", "name", "run_ms",
                   "wait_ms", "runs", "waits", "cancels"};
  for (const auto& profile : snapshot.tasks) {
    std::vector<std::string> row;
    row.push_back(std::to_string(profile.id));
    row.push_back(format_task_state(profile.state));
    row.push_back(format_task_mode(profile.mode));
    row.push_back(profile.name);
    row.push_back(format_duration_ms(profile.running_ns));
    row.push_back(format_duration_ms(profile.waiting_ns));
    row.push_back(std::to_string(profile.run_count));
    row.push_back(std::to_string(profile.wait_count));
    row.push_back(std::to_string(profile.cancel_count));
    table.rows.push_back(std::move(row));
  }

  auto idR = iris::viz::create_table(schema, store, table);
  if (!idR) {
    std::cout << "error: " << idR.error->message << "\n";
    return false;
  }
  auto id = idR.value.value();
  std::cout << "created Viz::Table " << id.to_hex() << "\n";
  print_route_for(schema, iris::viz::kTypeVizTable);
  maybe_spawn_concho(schema, store, id);
  return true;
}

bool cmd_task_trace(iris::ceo::TaskRegistry& registry,
                    SchemaRegistry& schema,
                    SqliteStore& store,
                    const std::vector<std::string>& args) {
  if (args.size() == 2 && args[1] == "clear") {
    registry.clear_trace();
    std::cout << "task trace cleared\n";
    return true;
  }
  if (args.size() != 1) {
    std::cout << "error: usage: task trace [clear]\n";
    return false;
  }

  auto snapshot = registry.trace_snapshot();
  iris::viz::Table table;
  table.columns = {"seq", "task_id", "from", "to", "at_ms"};
  for (const auto& event : snapshot.events) {
    std::vector<std::string> row;
    row.push_back(std::to_string(event.seq));
    row.push_back(std::to_string(event.id));
    row.push_back(format_task_state(event.from));
    row.push_back(format_task_state(event.to));
    row.push_back(format_duration_ms(event.timestamp_ns));
    table.rows.push_back(std::move(row));
  }

  auto idR = iris::viz::create_table(schema, store, table);
  if (!idR) {
    std::cout << "error: " << idR.error->message << "\n";
    return false;
  }
  auto id = idR.value.value();
  std::cout << "created Viz::Table " << id.to_hex() << "\n";
  print_route_for(schema, iris::viz::kTypeVizTable);
  maybe_spawn_concho(schema, store, id);
  if (snapshot.dropped_events > 0) {
    std::cout << "warning: dropped " << snapshot.dropped_events << " trace events\n";
  }
  return true;
}

bool resolve_io_handle(const std::unordered_map<std::string, iris::conduit::IoHandle>& handles,
                       const std::unordered_map<std::string, iris::conduit::IoHandle>& aliases,
                       const std::string& token,
                       iris::conduit::IoHandle* out,
                       std::string* err_out) {
  auto it = handles.find(token);
  if (it != handles.end()) {
    if (out) *out = it->second;
    return true;
  }
  auto alias = aliases.find(token);
  if (alias == aliases.end()) {
    if (err_out) *err_out = "unknown handle";
    return false;
  }
  if (out) *out = alias->second;
  return true;
}

void print_io_outcome(const iris::exec::AwaitOutcome& outcome) {
  if (!outcome.resumed.empty()) {
    std::cout << "resumed";
    for (auto id : outcome.resumed) std::cout << " " << id;
    std::cout << "\n";
  }
  if (!outcome.canceled.empty()) {
    std::cout << "canceled";
    for (auto id : outcome.canceled) std::cout << " " << id;
    std::cout << "\n";
  }
}

std::optional<IoAliasRecord> parse_io_alias_record(const referee::ObjectRecord& rec) {
  try {
    auto json = nlohmann::json::from_cbor(rec.payload_cbor);
    auto name = json.value("name", "");
    auto kind_text = json.value("kind", "");
    auto handle_id = json.value("handle_id", 0ULL);
    auto active = json.value("active", false);
    auto kind = io_kind_from_string(kind_text);
    if (name.empty() || !kind.has_value()) return std::nullopt;
    IoAliasRecord out;
    out.name = name;
    out.kind = kind.value();
    out.handle_id = handle_id;
    out.active = active;
    out.created_at = rec.created_at_unix_ms;
    return out;
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

referee::Result<void> persist_io_alias(SqliteStore& store,
                                       SchemaRegistry& registry,
                                       const std::string& name,
                                       const iris::conduit::IoHandle& handle,
                                       bool active) {
  auto typesR = registry.list_types();
  if (!typesR) return referee::Result<void>::err(typesR.error->message);
  std::optional<TypeSummary> alias_type;
  for (const auto& summary : typesR.value.value()) {
    if (summary.namespace_name == "Conch" && summary.name == "IoHandleAlias") {
      alias_type = summary;
      break;
    }
  }
  if (!alias_type.has_value()) {
    return referee::Result<void>::err("io handle alias type not registered");
  }

  nlohmann::json payload;
  payload["name"] = name;
  payload["kind"] = io_kind_name(handle.kind);
  payload["handle_id"] = handle.id;
  payload["active"] = active;
  auto cbor = nlohmann::json::to_cbor(payload);
  auto createR = store.create_object(alias_type->type_id, alias_type->definition_id, cbor);
  if (!createR) return referee::Result<void>::err(createR.error->message);
  return referee::Result<void>::ok();
}

void load_io_aliases(SqliteStore& store,
                     SchemaRegistry& registry,
                     std::unordered_map<std::string, iris::conduit::IoHandle>& aliases) {
  auto typesR = registry.list_types();
  if (!typesR) return;
  std::optional<TypeSummary> alias_type;
  for (const auto& summary : typesR.value.value()) {
    if (summary.namespace_name == "Conch" && summary.name == "IoHandleAlias") {
      alias_type = summary;
      break;
    }
  }
  if (!alias_type.has_value()) return;

  auto listR = store.list_by_type(alias_type->type_id);
  if (!listR || listR.value->empty()) return;

  std::unordered_map<std::string, IoAliasRecord> latest;
  for (const auto& rec : listR.value.value()) {
    auto parsed = parse_io_alias_record(rec);
    if (!parsed.has_value()) continue;
    auto it = latest.find(parsed->name);
    if (it == latest.end() || parsed->created_at >= it->second.created_at) {
      latest[parsed->name] = parsed.value();
    }
  }

  for (const auto& kv : latest) {
    const auto& record = kv.second;
    if (!record.active) continue;
    aliases.emplace(record.name,
                    iris::conduit::IoHandle{record.kind, record.handle_id});
  }
}

void list_io_aliases(SqliteStore& store,
                     SchemaRegistry& registry,
                     const std::unordered_map<std::string, iris::conduit::IoHandle>& aliases,
                     iris::conduit::IoHandleStore& handle_store) {
  auto typesR = registry.list_types();
  if (!typesR) {
    std::cout << "error: " << typesR.error->message << "\n";
    return;
  }
  std::optional<TypeSummary> alias_type;
  for (const auto& summary : typesR.value.value()) {
    if (summary.namespace_name == "Conch" && summary.name == "IoHandleAlias") {
      alias_type = summary;
      break;
    }
  }
  if (!alias_type.has_value()) {
    std::cout << "error: io handle alias type not registered\n";
    return;
  }

  auto listR = store.list_by_type(alias_type->type_id);
  if (!listR) {
    std::cout << "error: " << listR.error->message << "\n";
    return;
  }
  if (listR.value->empty() || aliases.empty()) {
    std::cout << "no io aliases\n";
    return;
  }

  for (const auto& kv : aliases) {
    const auto& handle = kv.second;
    bool alive = false;
    if (handle.kind == iris::conduit::IoHandleKind::Channel) {
      alive = handle_store.find_channel(handle) != nullptr;
    } else if (handle.kind == iris::conduit::IoHandleKind::Datagram) {
      alive = handle_store.find_datagram(handle) != nullptr;
    } else if (handle.kind == iris::conduit::IoHandleKind::ByteStream) {
      alive = handle_store.find_stream(handle) != nullptr;
    }
    std::cout << kv.first << " " << io_kind_name(handle.kind)
              << " id=" << handle.id;
    if (!alive) std::cout << " stale";
    std::cout << "\n";
  }
}

bool cmd_io(iris::conduit::IoExecutor& executor,
            iris::conduit::IoHandleStore& handle_store,
            std::unordered_map<std::string, iris::conduit::IoHandle>& handles,
            std::unordered_map<std::string, iris::conduit::IoHandle>& aliases,
            std::uint64_t& next_handle_id,
            SchemaRegistry& registry,
            SqliteStore& store,
            const std::set<std::string>& session_caps,
            const std::vector<std::string>& args) {
  if (args.empty()) {
    std::cout << "error: usage: io <open|send|recv|await|close|handles|aliases|alias|unalias>\n";
    return false;
  }
  DispatchEngine engine(registry);

  if (args[0] == "handles") {
    if (handles.empty()) {
      std::cout << "no io handles\n";
      return true;
    }
    for (const auto& kv : handles) {
      std::cout << kv.first << " " << io_kind_name(kv.second.kind)
                << " id=" << kv.second.id << "\n";
    }
    return true;
  }

  if (args[0] == "aliases") {
    list_io_aliases(store, registry, aliases, handle_store);
    return true;
  }

  if (args[0] == "open") {
    if (args.size() != 4) {
      std::cout << "error: usage: io open <channel|datagram> <taskA> <taskB>\n";
      return false;
    }
    std::uint64_t a = 0;
    std::uint64_t b = 0;
    if (!parse_u64(args[2], &a) || !parse_u64(args[3], &b)) {
      std::cout << "error: invalid task id\n";
      return false;
    }
    if (args[1] == "channel") {
      auto matchR = engine.resolve(kTypeKernelIo, "open_channel", OperationScope::Class,
                                   {kTypeU64, kTypeU64}, 2, true);
      if (!matchR) {
        std::cout << "error: " << matchR.error->message << "\n";
        return false;
      }
      std::string cap_err;
      if (!has_required_capabilities(matchR.value->operation, session_caps, &cap_err)) {
        std::cout << "error: " << cap_err << "\n";
        return false;
      }
      auto openR = executor.open_channel(matchR.value.value(), a, b);
      if (!openR) {
        std::cout << "error: " << openR.error->message << "\n";
        return false;
      }
      auto name_a = io_handle_name(next_handle_id++);
      auto name_b = io_handle_name(next_handle_id++);
      handles.emplace(name_a, openR.value->first);
      handles.emplace(name_b, openR.value->second);
      std::cout << "io channel " << name_a << " " << name_b << "\n";
      return true;
    }
    if (args[1] == "datagram") {
      auto matchR = engine.resolve(kTypeKernelIo, "open_datagram", OperationScope::Class,
                                   {kTypeU64, kTypeU64}, 2, true);
      if (!matchR) {
        std::cout << "error: " << matchR.error->message << "\n";
        return false;
      }
      std::string cap_err;
      if (!has_required_capabilities(matchR.value->operation, session_caps, &cap_err)) {
        std::cout << "error: " << cap_err << "\n";
        return false;
      }
      auto openR = executor.open_datagram(matchR.value.value(), a, b);
      if (!openR) {
        std::cout << "error: " << openR.error->message << "\n";
        return false;
      }
      auto name_a = io_handle_name(next_handle_id++);
      auto name_b = io_handle_name(next_handle_id++);
      handles.emplace(name_a, openR.value->first);
      handles.emplace(name_b, openR.value->second);
      std::cout << "io datagram " << name_a << " " << name_b << "\n";
      return true;
    }
    std::cout << "error: usage: io open <channel|datagram> <taskA> <taskB>\n";
    return false;
  }

  if (args[0] == "send") {
    if (args.size() != 3) {
      std::cout << "error: usage: io send <handle> <hexbytes>\n";
      return false;
    }
    iris::conduit::IoHandle handle{};
    std::string err;
    if (!resolve_io_handle(handles, aliases, args[1], &handle, &err)) {
      std::cout << "error: " << err << "\n";
      return false;
    }
    std::vector<std::uint8_t> bytes;
    if (!parse_hex_bytes(args[2], &bytes, &err)) {
      std::cout << "error: " << err << "\n";
      return false;
    }
    if (handle.kind == iris::conduit::IoHandleKind::Channel) {
      auto matchR = engine.resolve(kTypeKernelIoChannel, "send", OperationScope::Object,
                                   {kTypeBytes}, 1, true);
      if (!matchR) {
        std::cout << "error: " << matchR.error->message << "\n";
        return false;
      }
      std::string cap_err;
      if (!has_required_capabilities(matchR.value->operation, session_caps, &cap_err)) {
        std::cout << "error: " << cap_err << "\n";
        return false;
      }
      auto sendR = executor.send_channel(matchR.value.value(), handle, bytes);
      if (!sendR) {
        std::cout << "error: " << sendR.error->message << "\n";
        return false;
      }
      std::cout << "io send ready=" << (sendR.value->ready ? "true" : "false") << "\n";
      print_io_outcome(sendR.value->outcome);
      return true;
    }
    if (handle.kind == iris::conduit::IoHandleKind::Datagram) {
      auto matchR = engine.resolve(kTypeKernelIoDatagram, "send", OperationScope::Object,
                                   {kTypeBytes}, 1, true);
      if (!matchR) {
        std::cout << "error: " << matchR.error->message << "\n";
        return false;
      }
      std::string cap_err;
      if (!has_required_capabilities(matchR.value->operation, session_caps, &cap_err)) {
        std::cout << "error: " << cap_err << "\n";
        return false;
      }
      auto sendR = executor.send_datagram(matchR.value.value(), handle, bytes);
      if (!sendR) {
        std::cout << "error: " << sendR.error->message << "\n";
        return false;
      }
      std::cout << "io send ready=" << (sendR.value->ready ? "true" : "false") << "\n";
      print_io_outcome(sendR.value->outcome);
      return true;
    }
    std::cout << "error: unsupported handle kind\n";
    return false;
  }

  if (args[0] == "await") {
    if (args.size() != 3) {
      std::cout << "error: usage: io await <handle> <taskId>\n";
      return false;
    }
    iris::conduit::IoHandle handle{};
    std::string err;
    if (!resolve_io_handle(handles, aliases, args[1], &handle, &err)) {
      std::cout << "error: " << err << "\n";
      return false;
    }
    std::uint64_t task_id = 0;
    if (!parse_u64(args[2], &task_id)) {
      std::cout << "error: invalid task id\n";
      return false;
    }
    if (handle.kind == iris::conduit::IoHandleKind::Channel) {
      auto matchR = engine.resolve(kTypeKernelIoChannel, "await_readable", OperationScope::Object,
                                   {kTypeU64}, 1, true);
      if (!matchR) {
        std::cout << "error: " << matchR.error->message << "\n";
        return false;
      }
      std::string cap_err;
      if (!has_required_capabilities(matchR.value->operation, session_caps, &cap_err)) {
        std::cout << "error: " << cap_err << "\n";
        return false;
      }
      auto waitR = executor.await_channel(matchR.value.value(), handle, task_id);
      if (!waitR) {
        std::cout << "error: " << waitR.error->message << "\n";
        return false;
      }
      std::cout << "io await ready=" << (waitR.value->ready ? "true" : "false") << "\n";
      print_io_outcome(waitR.value->outcome);
      return true;
    }
    if (handle.kind == iris::conduit::IoHandleKind::Datagram) {
      auto matchR = engine.resolve(kTypeKernelIoDatagram, "await_readable", OperationScope::Object,
                                   {kTypeU64}, 1, true);
      if (!matchR) {
        std::cout << "error: " << matchR.error->message << "\n";
        return false;
      }
      std::string cap_err;
      if (!has_required_capabilities(matchR.value->operation, session_caps, &cap_err)) {
        std::cout << "error: " << cap_err << "\n";
        return false;
      }
      auto waitR = executor.await_datagram(matchR.value.value(), handle, task_id);
      if (!waitR) {
        std::cout << "error: " << waitR.error->message << "\n";
        return false;
      }
      std::cout << "io await ready=" << (waitR.value->ready ? "true" : "false") << "\n";
      print_io_outcome(waitR.value->outcome);
      return true;
    }
    std::cout << "error: unsupported handle kind\n";
    return false;
  }

  if (args[0] == "recv") {
    if (args.size() < 2 || args.size() > 3) {
      std::cout << "error: usage: io recv <handle> [max_bytes]\n";
      return false;
    }
    iris::conduit::IoHandle handle{};
    std::string err;
    if (!resolve_io_handle(handles, aliases, args[1], &handle, &err)) {
      std::cout << "error: " << err << "\n";
      return false;
    }
    if (handle.kind == iris::conduit::IoHandleKind::Channel) {
      if (args.size() != 3) {
        std::cout << "error: usage: io recv <handle> <max_bytes>\n";
        return false;
      }
      std::uint64_t max_bytes = 0;
      if (!parse_u64(args[2], &max_bytes)) {
        std::cout << "error: invalid max_bytes\n";
        return false;
      }
      auto matchR = engine.resolve(kTypeKernelIoChannel, "recv", OperationScope::Object,
                                   {kTypeU64}, 1, true);
      if (!matchR) {
        std::cout << "error: " << matchR.error->message << "\n";
        return false;
      }
      std::string cap_err;
      if (!has_required_capabilities(matchR.value->operation, session_caps, &cap_err)) {
        std::cout << "error: " << cap_err << "\n";
        return false;
      }
      auto recvR = executor.recv_channel(matchR.value.value(), handle, max_bytes);
      if (!recvR) {
        std::cout << "error: " << recvR.error->message << "\n";
        return false;
      }
      std::cout << "io recv " << bytes_to_hex(recvR.value.value()) << "\n";
      return true;
    }
    if (handle.kind == iris::conduit::IoHandleKind::Datagram) {
      if (args.size() != 2) {
        std::cout << "error: usage: io recv <handle>\n";
        return false;
      }
      auto matchR = engine.resolve(kTypeKernelIoDatagram, "recv", OperationScope::Object, {}, 0, true);
      if (!matchR) {
        std::cout << "error: " << matchR.error->message << "\n";
        return false;
      }
      std::string cap_err;
      if (!has_required_capabilities(matchR.value->operation, session_caps, &cap_err)) {
        std::cout << "error: " << cap_err << "\n";
        return false;
      }
      auto recvR = executor.recv_datagram(matchR.value.value(), handle);
      if (!recvR) {
        std::cout << "error: " << recvR.error->message << "\n";
        return false;
      }
      if (!recvR.value->has_value()) {
        std::cout << "io recv (none)\n";
        return true;
      }
      std::cout << "io recv " << bytes_to_hex(recvR.value->value()) << "\n";
      return true;
    }
    std::cout << "error: unsupported handle kind\n";
    return false;
  }

  if (args[0] == "close") {
    if (args.size() != 2) {
      std::cout << "error: usage: io close <handle>\n";
      return false;
    }
    iris::conduit::IoHandle handle{};
    std::string err;
    if (!resolve_io_handle(handles, aliases, args[1], &handle, &err)) {
      std::cout << "error: " << err << "\n";
      return false;
    }
    if (handle.kind == iris::conduit::IoHandleKind::Channel) {
      auto matchR = engine.resolve(kTypeKernelIoChannel, "close", OperationScope::Object, {}, 0, true);
      if (!matchR) {
        std::cout << "error: " << matchR.error->message << "\n";
        return false;
      }
      std::string cap_err;
      if (!has_required_capabilities(matchR.value->operation, session_caps, &cap_err)) {
        std::cout << "error: " << cap_err << "\n";
        return false;
      }
      auto closeR = executor.close_channel(matchR.value.value(), handle);
      if (!closeR) {
        std::cout << "error: " << closeR.error->message << "\n";
        return false;
      }
    } else if (handle.kind == iris::conduit::IoHandleKind::Datagram) {
      auto matchR = engine.resolve(kTypeKernelIoDatagram, "close", OperationScope::Object, {}, 0, true);
      if (!matchR) {
        std::cout << "error: " << matchR.error->message << "\n";
        return false;
      }
      std::string cap_err;
      if (!has_required_capabilities(matchR.value->operation, session_caps, &cap_err)) {
        std::cout << "error: " << cap_err << "\n";
        return false;
      }
      auto closeR = executor.close_datagram(matchR.value.value(), handle);
      if (!closeR) {
        std::cout << "error: " << closeR.error->message << "\n";
        return false;
      }
    } else {
      std::cout << "error: unsupported handle kind\n";
      return false;
    }
    handles.erase(args[1]);
    std::cout << "io closed " << args[1] << "\n";
    return true;
  }

  if (args[0] == "alias") {
    if (args.size() != 3) {
      std::cout << "error: usage: io alias <handle> <name>\n";
      return false;
    }
    iris::conduit::IoHandle handle{};
    std::string err;
    if (!resolve_io_handle(handles, aliases, args[1], &handle, &err)) {
      std::cout << "error: " << err << "\n";
      return false;
    }
    if (aliases.find(args[2]) != aliases.end()) {
      std::cout << "error: alias already exists\n";
      return false;
    }
    auto persistR = persist_io_alias(store, registry, args[2], handle, true);
    if (!persistR) {
      std::cout << "error: " << persistR.error->message << "\n";
      return false;
    }
    aliases.emplace(args[2], handle);
    std::cout << "io alias " << args[2] << " " << io_kind_name(handle.kind)
              << " id=" << handle.id << "\n";
    return true;
  }

  if (args[0] == "unalias") {
    if (args.size() != 2) {
      std::cout << "error: usage: io unalias <name>\n";
      return false;
    }
    auto it = aliases.find(args[1]);
    if (it == aliases.end()) {
      std::cout << "error: alias not found\n";
      return false;
    }
    auto persistR = persist_io_alias(store, registry, args[1], it->second, false);
    if (!persistR) {
      std::cout << "error: " << persistR.error->message << "\n";
      return false;
    }
    aliases.erase(it);
    std::cout << "io unalias " << args[1] << "\n";
    return true;
  }

  std::cout << "error: usage: io <open|send|recv|await|close|handles|aliases|alias|unalias>\n";
  return false;
}

bool handle_io_family_command(iris::conduit::IoExecutor& executor,
                              iris::conduit::IoHandleStore& handle_store,
                              std::unordered_map<std::string, iris::conduit::IoHandle>& handles,
                              std::unordered_map<std::string, iris::conduit::IoHandle>& aliases,
                              std::uint64_t& next_handle_id,
                              SchemaRegistry& registry,
                              SqliteStore& store,
                              const std::set<std::string>& granted_caps,
                              const iris::parser::IoCommand& command) {
  cmd_io(executor, handle_store, handles, aliases, next_handle_id, registry, store,
         granted_caps, command.args);
  return true;
}

const char* scope_label(OperationScope scope) {
  return scope == OperationScope::Class ? "class" : "object";
}

void print_operations(SchemaRegistry& registry,
                      const std::vector<TypeSummary>& types,
                      TypeID type_id,
                      std::optional<OperationScope> scope_filter,
                      bool include_inherited) {
  auto listR = list_operations_with_inheritance(registry, type_id, include_inherited);
  if (!listR) {
    std::cout << "error: " << listR.error->message << "\n";
    return;
  }

  std::map<OperationScope, std::map<std::string, std::vector<OperationListing>>> grouped;
  for (const auto& entry : listR.value.value()) {
    if (scope_filter.has_value() && entry.operation.scope != scope_filter.value()) continue;
    grouped[entry.operation.scope][entry.operation.name].push_back(entry);
  }

  if (grouped.empty()) return;

  std::cout << "operations\n";
  for (OperationScope scope : { OperationScope::Class, OperationScope::Object }) {
    auto scope_it = grouped.find(scope);
    if (scope_it == grouped.end()) continue;
    std::cout << "  " << scope_label(scope) << "\n";
    for (const auto& [name, overloads] : scope_it->second) {
      if (overloads.size() == 1) {
        const auto& entry = overloads.front();
        std::cout << "    " << name << format_signature(entry.operation)
                  << format_capabilities(entry.operation);
        if (entry.depth > 0) {
          std::cout << " [from " << type_display_name_for(types, entry.owner) << "]";
        }
        std::cout << "\n";
        continue;
      }
      std::cout << "    " << name << "\n";
      for (const auto& entry : overloads) {
        std::cout << "      " << format_signature(entry.operation)
                  << format_capabilities(entry.operation);
        if (entry.depth > 0) {
          std::cout << " [from " << type_display_name_for(types, entry.owner) << "]";
        }
        std::cout << "\n";
      }
    }
  }
}

bool parse_bool(std::string_view v, bool* out) {
  if (v == "true") { *out = true; return true; }
  if (v == "false") { *out = false; return true; }
  return false;
}

bool parse_int(std::string_view v, std::int64_t* out) {
  if (v.empty()) return false;
  char* end = nullptr;
  std::string tmp(v);
  long long val = std::strtoll(tmp.c_str(), &end, 10);
  if (!end || *end != '\0') return false;
  *out = (std::int64_t)val;
  return true;
}

bool parse_double(std::string_view v, double* out) {
  if (v.empty()) return false;
  char* end = nullptr;
  std::string tmp(v);
  double val = std::strtod(tmp.c_str(), &end);
  if (!end || *end != '\0') return false;
  *out = val;
  return true;
}

bool parse_u64(std::string_view v, std::uint64_t* out) {
  if (v.empty()) return false;
  std::string tmp(v);
  char* end = nullptr;
  int base = 10;
  if (tmp.rfind("0x", 0) == 0 || tmp.rfind("0X", 0) == 0) {
    base = 16;
    tmp = tmp.substr(2);
  }
  if (tmp.empty()) return false;
  unsigned long long val = std::strtoull(tmp.c_str(), &end, base);
  if (!end || *end != '\0') return false;
  *out = static_cast<std::uint64_t>(val);
  return true;
}

std::string hex_u64(std::uint64_t v) {
  std::array<char, 19> buf{};
  std::snprintf(buf.data(), buf.size(), "0x%016llx",
                static_cast<unsigned long long>(v));
  return std::string(buf.data());
}

std::string bytes_to_hex(const std::vector<std::uint8_t>& bytes) {
  static const char* kHex = "0123456789abcdef";
  std::string out;
  out.reserve(bytes.size() * 2);
  for (auto b : bytes) {
    out.push_back(kHex[(b >> 4) & 0xF]);
    out.push_back(kHex[b & 0xF]);
  }
  return out;
}

bool parse_hex_bytes(std::string_view v, std::vector<std::uint8_t>* out, std::string* err_out) {
  if (!out) return false;
  out->clear();
  if (v.empty()) {
    if (err_out) *err_out = "expected hex bytes";
    return false;
  }
  std::string tmp(v);
  if (tmp.rfind("0x", 0) == 0 || tmp.rfind("0X", 0) == 0) {
    tmp = tmp.substr(2);
  } else if (tmp.rfind("hex:", 0) == 0 || tmp.rfind("HEX:", 0) == 0) {
    tmp = tmp.substr(4);
  }
  if (tmp.empty()) {
    if (err_out) *err_out = "expected hex bytes";
    return false;
  }
  if (tmp.size() % 2 != 0) {
    if (err_out) *err_out = "hex bytes must be an even-length string";
    return false;
  }

  out->reserve(tmp.size() / 2);
  for (size_t i = 0; i < tmp.size(); i += 2) {
    char hi = tmp[i];
    char lo = tmp[i + 1];
    if (!std::isxdigit(static_cast<unsigned char>(hi)) ||
        !std::isxdigit(static_cast<unsigned char>(lo))) {
      if (err_out) *err_out = "invalid hex byte sequence";
      return false;
    }
    auto byte = static_cast<std::uint8_t>(std::strtoul(tmp.substr(i, 2).c_str(), nullptr, 16));
    out->push_back(byte);
  }
  return true;
}

struct BundleObjectRecord {
  ObjectID id{};
  referee::Version version{};
  TypeID type{};
  ObjectID definition_id{};
  referee::Bytes payload_cbor{};
  std::uint64_t created_at_ms{0};
};

struct BundleEdgeRecord {
  ObjectRef from{};
  ObjectRef to{};
  std::string name;
  std::string role;
  referee::Bytes props_cbor{};
  std::uint64_t created_at_ms{0};
};

std::string object_id_hex(const ObjectID& id) {
  return id.to_hex();
}

bool parse_object_id_hex(const std::string& text, ObjectID* out, std::string* err_out) {
  try {
    if (out) *out = ObjectID::from_hex(text);
    return true;
  } catch (const std::exception& ex) {
    if (err_out) *err_out = ex.what();
    return false;
  }
}

bool record_less(const BundleObjectRecord& a, const BundleObjectRecord& b) {
  auto a_hex = object_id_hex(a.id);
  auto b_hex = object_id_hex(b.id);
  if (a_hex != b_hex) return a_hex < b_hex;
  if (a.version.v != b.version.v) return a.version.v < b.version.v;
  if (a.type.v != b.type.v) return a.type.v < b.type.v;
  if (a.definition_id != b.definition_id) return object_id_hex(a.definition_id) < object_id_hex(b.definition_id);
  return a.created_at_ms < b.created_at_ms;
}

bool edge_less(const BundleEdgeRecord& a, const BundleEdgeRecord& b) {
  auto a_from = object_id_hex(a.from.id);
  auto b_from = object_id_hex(b.from.id);
  if (a_from != b_from) return a_from < b_from;
  if (a.from.ver.v != b.from.ver.v) return a.from.ver.v < b.from.ver.v;
  auto a_to = object_id_hex(a.to.id);
  auto b_to = object_id_hex(b.to.id);
  if (a_to != b_to) return a_to < b_to;
  if (a.to.ver.v != b.to.ver.v) return a.to.ver.v < b.to.ver.v;
  if (a.name != b.name) return a.name < b.name;
  if (a.role != b.role) return a.role < b.role;
  return a.created_at_ms < b.created_at_ms;
}

referee::Result<void> export_bundle(SchemaRegistry& registry,
                                    SqliteStore& store,
                                    const std::string& path) {
  constexpr TypeID kTypeDefinition{iris::refract::kTypeDefinitionType};

  auto typesR = registry.list_types();
  if (!typesR) return referee::Result<void>::err(typesR.error->message);

  std::vector<BundleObjectRecord> definitions;
  std::vector<BundleObjectRecord> objects;

  auto defR = store.list_by_type(kTypeDefinition);
  if (!defR) return referee::Result<void>::err(defR.error->message);
  for (const auto& rec : defR.value.value()) {
    BundleObjectRecord out;
    out.id = rec.ref.id;
    out.version = rec.ref.ver;
    out.type = rec.type;
    out.definition_id = rec.definition_id;
    out.payload_cbor = rec.payload_cbor;
    out.created_at_ms = rec.created_at_unix_ms;
    definitions.push_back(std::move(out));
  }

  std::map<std::string, BundleObjectRecord> latest_objects;
  for (const auto& summary : typesR.value.value()) {
    if (summary.type_id.v == kTypeDefinition.v) continue;
    auto listR = store.list_by_type(summary.type_id);
    if (!listR) return referee::Result<void>::err(listR.error->message);
    for (const auto& rec : listR.value.value()) {
      auto key = rec.ref.id.to_hex();
      auto it = latest_objects.find(key);
      if (it == latest_objects.end() || rec.ref.ver.v > it->second.version.v) {
        BundleObjectRecord out;
        out.id = rec.ref.id;
        out.version = rec.ref.ver;
        out.type = rec.type;
        out.definition_id = rec.definition_id;
        out.payload_cbor = rec.payload_cbor;
        out.created_at_ms = rec.created_at_unix_ms;
        latest_objects[key] = std::move(out);
      }
    }
  }
  for (auto& [_, rec] : latest_objects) objects.push_back(std::move(rec));

  std::vector<BundleEdgeRecord> edges;
  auto collect_edges = [&](const BundleObjectRecord& rec) -> referee::Result<void> {
    auto edgesR = store.edges_from(ObjectRef{rec.id, rec.version});
    if (!edgesR) return referee::Result<void>::err(edgesR.error->message);
    for (const auto& edge : edgesR.value.value()) {
      BundleEdgeRecord out;
      out.from = edge.from;
      out.to = edge.to;
      out.name = edge.name;
      out.role = edge.role;
      out.props_cbor = edge.props_cbor;
      out.created_at_ms = edge.created_at_unix_ms;
      edges.push_back(std::move(out));
    }
    return referee::Result<void>::ok();
  };

  for (const auto& rec : definitions) {
    auto r = collect_edges(rec);
    if (!r) return r;
  }
  for (const auto& rec : objects) {
    auto r = collect_edges(rec);
    if (!r) return r;
  }

  std::sort(definitions.begin(), definitions.end(), record_less);
  std::sort(objects.begin(), objects.end(), record_less);
  std::sort(edges.begin(), edges.end(), edge_less);

  nlohmann::json root;
  root["format"] = "iris.referee.bundle";
  root["version"] = 1;

  root["definitions"] = nlohmann::json::array();
  for (const auto& rec : definitions) {
    nlohmann::json entry;
    entry["id"] = object_id_hex(rec.id);
    entry["version"] = rec.version.v;
    entry["type_id"] = rec.type.v;
    entry["definition_id"] = object_id_hex(rec.definition_id);
    entry["payload_cbor_hex"] = bytes_to_hex(rec.payload_cbor);
    entry["created_at_ms"] = rec.created_at_ms;
    root["definitions"].push_back(std::move(entry));
  }

  root["objects"] = nlohmann::json::array();
  for (const auto& rec : objects) {
    nlohmann::json entry;
    entry["id"] = object_id_hex(rec.id);
    entry["version"] = rec.version.v;
    entry["type_id"] = rec.type.v;
    entry["definition_id"] = object_id_hex(rec.definition_id);
    entry["payload_cbor_hex"] = bytes_to_hex(rec.payload_cbor);
    entry["created_at_ms"] = rec.created_at_ms;
    root["objects"].push_back(std::move(entry));
  }

  root["edges"] = nlohmann::json::array();
  for (const auto& edge : edges) {
    nlohmann::json entry;
    entry["from_id"] = object_id_hex(edge.from.id);
    entry["from_version"] = edge.from.ver.v;
    entry["to_id"] = object_id_hex(edge.to.id);
    entry["to_version"] = edge.to.ver.v;
    entry["name"] = edge.name;
    entry["role"] = edge.role;
    entry["props_cbor_hex"] = bytes_to_hex(edge.props_cbor);
    entry["created_at_ms"] = edge.created_at_ms;
    root["edges"].push_back(std::move(entry));
  }

  std::ofstream out(path);
  if (!out) return referee::Result<void>::err("failed to open bundle file");
  out << root.dump(2) << "\n";
  if (!out) return referee::Result<void>::err("failed to write bundle file");
  return referee::Result<void>::ok();
}

referee::Result<void> import_bundle(SchemaRegistry& registry,
                                    SqliteStore& store,
                                    const std::string& path) {
  std::ifstream in(path);
  if (!in) return referee::Result<void>::err("failed to open bundle file");
  nlohmann::json root;
  try {
    in >> root;
  } catch (const std::exception& ex) {
    return referee::Result<void>::err(ex.what());
  }

  if (root.value("format", "") != "iris.referee.bundle") {
    return referee::Result<void>::err("invalid bundle format");
  }
  if (root.value("version", 0) != 1) {
    return referee::Result<void>::err("unsupported bundle version");
  }

  auto txnR = store.begin();
  if (!txnR) return referee::Result<void>::err(txnR.error->message);

  auto fail = [&](const std::string& msg) {
    store.rollback();
    return referee::Result<void>::err(msg);
  };

  auto edge_exists = [&](const ObjectRef& from,
                         const ObjectRef& to,
                         const std::string& name,
                         const std::string& role,
                         const referee::Bytes& props) -> referee::Result<bool> {
    auto outR = store.edges_from(from);
    if (!outR) return referee::Result<bool>::err(outR.error->message);
    for (const auto& edge : outR.value.value()) {
      if (edge.to.id == to.id
          && edge.to.ver.v == to.ver.v
          && edge.name == name
          && edge.role == role
          && edge.props_cbor == props) {
        return referee::Result<bool>::ok(true);
      }
    }
    return referee::Result<bool>::ok(false);
  };

  if (root.contains("definitions")) {
    for (const auto& item : root.at("definitions")) {
      std::string id_text = item.value("id", "");
      std::string def_text = item.value("definition_id", "");
      std::string payload_hex = item.value("payload_cbor_hex", "");
      if (id_text.empty() || def_text.empty()) return fail("definition missing id");
      ObjectID id{};
      ObjectID def_id{};
      std::string err;
      if (!parse_object_id_hex(id_text, &id, &err)) return fail("definition id invalid");
      if (!parse_object_id_hex(def_text, &def_id, &err)) return fail("definition definition_id invalid");
      if (id != def_id) return fail("definition id mismatch");
      auto existingR = store.get_latest(id);
      if (!existingR) return fail(existingR.error->message);
      if (existingR.value->has_value()) continue;
      referee::Bytes payload;
      if (!parse_hex_bytes(payload_hex, &payload, &err)) return fail(err);
      auto createR = store.create_object_with_id(id, iris::refract::kTypeDefinitionType,
                                                 def_id, payload);
      if (!createR) return fail(createR.error->message);
    }
  }

  if (root.contains("objects")) {
    for (const auto& item : root.at("objects")) {
      std::string id_text = item.value("id", "");
      std::string def_text = item.value("definition_id", "");
      std::string payload_hex = item.value("payload_cbor_hex", "");
      if (id_text.empty() || def_text.empty()) return fail("object missing id");
      ObjectID id{};
      ObjectID def_id{};
      std::string err;
      if (!parse_object_id_hex(id_text, &id, &err)) return fail("object id invalid");
      if (!parse_object_id_hex(def_text, &def_id, &err)) return fail("object definition_id invalid");
      auto existingR = store.get_latest(id);
      if (!existingR) return fail(existingR.error->message);
      if (existingR.value->has_value()) continue;
      auto defR = store.get_latest(def_id);
      if (!defR) return fail(defR.error->message);
      if (!defR.value->has_value()) return fail("definition not found for object");
      auto type_id = TypeID{item.value("type_id", 0ULL)};
      referee::Bytes payload;
      if (!parse_hex_bytes(payload_hex, &payload, &err)) return fail(err);
      auto createR = store.create_object_with_id(id, type_id, def_id, payload);
      if (!createR) return fail(createR.error->message);
    }
  }

  if (root.contains("edges")) {
    for (const auto& item : root.at("edges")) {
      ObjectID from_id{};
      ObjectID to_id{};
      std::string err;
      if (!parse_object_id_hex(item.value("from_id", ""), &from_id, &err)) {
        return fail("edge from_id invalid");
      }
      if (!parse_object_id_hex(item.value("to_id", ""), &to_id, &err)) {
        return fail("edge to_id invalid");
      }
      referee::Version from_ver{item.value("from_version", 0ULL)};
      referee::Version to_ver{item.value("to_version", 0ULL)};
      auto fromR = store.get_object(ObjectRef{from_id, from_ver});
      if (!fromR) return fail(fromR.error->message);
      if (!fromR.value->has_value()) return fail("edge from object not found");
      auto toR = store.get_object(ObjectRef{to_id, to_ver});
      if (!toR) return fail(toR.error->message);
      if (!toR.value->has_value()) return fail("edge to object not found");

      std::string name = item.value("name", "");
      std::string role = item.value("role", "");
      referee::Bytes props;
      std::string props_hex = item.value("props_cbor_hex", "");
      if (!props_hex.empty()) {
        if (!parse_hex_bytes(props_hex, &props, &err)) return fail(err);
      }
      auto existsR = edge_exists(ObjectRef{from_id, from_ver},
                                 ObjectRef{to_id, to_ver},
                                 name, role, props);
      if (!existsR) return fail(existsR.error->message);
      if (existsR.value.value()) continue;
      auto edgeR = store.add_edge(ObjectRef{from_id, from_ver},
                                  ObjectRef{to_id, to_ver},
                                  name, role, props);
      if (!edgeR) return fail(edgeR.error->message);
    }
  }

  auto commitR = store.commit();
  if (!commitR) return referee::Result<void>::err(commitR.error->message);
  (void)registry;
  return referee::Result<void>::ok();
}

struct MigrationStep {
  iris::refract::DefinitionRecord from;
  iris::refract::DefinitionRecord to;
  std::optional<std::string> hook;
};

struct MigrationTarget {
  TypeSummary summary{};
  iris::refract::DefinitionRecord latest;
  std::string display;
};

struct MigrationRecordPayload {
  TypeID type_id{};
  ObjectID from_definition_id{};
  ObjectID to_definition_id{};
  ObjectID from_object_id{};
  referee::Version from_object_version{};
  ObjectID to_object_id{};
  referee::Version to_object_version{};
  std::optional<std::string> hook;
  std::string status;
  std::optional<std::string> note;
};

std::optional<TypeSummary> find_type_summary_by_id(const std::vector<TypeSummary>& types,
                                                   TypeID type_id) {
  for (const auto& summary : types) {
    if (summary.type_id.v == type_id.v) return summary;
  }
  return std::nullopt;
}

referee::Result<MigrationTarget> resolve_migration_target(SchemaRegistry& registry,
                                                          const std::string& token) {
  auto typesR = registry.list_types();
  if (!typesR) return referee::Result<MigrationTarget>::err(typesR.error->message);

  std::string err;
  auto def_id = parse_object_id(token, &err);
  if (def_id.has_value()) {
    auto defR = registry.get_definition_by_id(def_id.value());
    if (!defR) return referee::Result<MigrationTarget>::err(defR.error->message);
    if (!defR.value->has_value()) {
      return referee::Result<MigrationTarget>::err("definition not found");
    }
    const auto& def = defR.value->value();
    auto summary = find_type_summary_by_id(typesR.value.value(), def.definition.type_id);
    if (!summary.has_value()) {
      return referee::Result<MigrationTarget>::err("type summary not found");
    }
    MigrationTarget target;
    target.summary = summary.value();
    target.latest = def;
    target.display = type_display_name(target.summary);
    return referee::Result<MigrationTarget>::ok(std::move(target));
  }
  if (token.size() == 32 && !err.empty()) {
    return referee::Result<MigrationTarget>::err(err);
  }

  std::vector<TypeSummary> matches;
  for (const auto& summary : typesR.value.value()) {
    auto full = type_display_name(summary);
    if (summary.name == token || full == token) {
      matches.push_back(summary);
    }
  }
  if (matches.empty()) {
    return referee::Result<MigrationTarget>::err("type not found");
  }
  TypeID type_id = matches.front().type_id;
  for (const auto& match : matches) {
    if (match.type_id.v != type_id.v) {
      return referee::Result<MigrationTarget>::err("ambiguous type name");
    }
  }

  auto defR = registry.get_latest_definition_by_type(type_id);
  if (!defR) return referee::Result<MigrationTarget>::err(defR.error->message);
  if (!defR.value->has_value()) return referee::Result<MigrationTarget>::err("definition not found");

  MigrationTarget target;
  target.summary = matches.front();
  target.summary.definition_id = defR.value->value().ref.id;
  target.summary.name = defR.value->value().definition.name;
  target.summary.namespace_name = defR.value->value().definition.namespace_name;
  target.summary.preferred_renderer = defR.value->value().definition.preferred_renderer;
  target.latest = defR.value->value();
  target.display = type_display_name(target.summary);
  return referee::Result<MigrationTarget>::ok(std::move(target));
}

referee::Result<std::vector<MigrationStep>> build_migration_steps(
    SchemaRegistry& registry,
    const iris::refract::DefinitionRecord& latest) {
  auto chainR = registry.list_supersedes_chain(latest.ref.id);
  if (!chainR) return referee::Result<std::vector<MigrationStep>>::err(chainR.error->message);

  std::vector<MigrationStep> steps;
  auto current = latest;
  for (const auto& link : chainR.value.value()) {
    MigrationStep step;
    step.from = link.prior;
    step.to = current;
    step.hook = link.migration_hook;
    steps.push_back(std::move(step));
    current = link.prior;
  }
  return referee::Result<std::vector<MigrationStep>>::ok(std::move(steps));
}

std::string migration_record_key(const ObjectID& from_object_id,
                                 const ObjectID& to_definition_id) {
  return from_object_id.to_hex() + ":" + to_definition_id.to_hex();
}

bool parse_migration_record_payload(const referee::Bytes& payload_cbor,
                                    MigrationRecordPayload* out,
                                    std::string* err_out) {
  if (!out) return false;
  try {
    auto j = nlohmann::json::from_cbor(payload_cbor);
    auto type_id = TypeID{j.value("type_id", 0ULL)};
    std::string from_def_text = j.value("from_definition_id", "");
    std::string to_def_text = j.value("to_definition_id", "");
    std::string from_obj_text = j.value("from_object_id", "");
    std::string to_obj_text = j.value("to_object_id", "");
    if (type_id.v == 0 || from_def_text.empty() || to_def_text.empty()
        || from_obj_text.empty() || to_obj_text.empty()) {
      if (err_out) *err_out = "migration record missing required fields";
      return false;
    }
    std::string err;
    auto from_def = parse_object_id(from_def_text, &err);
    if (!from_def.has_value()) {
      if (err_out) *err_out = "invalid from_definition_id: " + err;
      return false;
    }
    auto to_def = parse_object_id(to_def_text, &err);
    if (!to_def.has_value()) {
      if (err_out) *err_out = "invalid to_definition_id: " + err;
      return false;
    }
    auto from_obj = parse_object_id(from_obj_text, &err);
    if (!from_obj.has_value()) {
      if (err_out) *err_out = "invalid from_object_id: " + err;
      return false;
    }
    auto to_obj = parse_object_id(to_obj_text, &err);
    if (!to_obj.has_value()) {
      if (err_out) *err_out = "invalid to_object_id: " + err;
      return false;
    }

    out->type_id = type_id;
    out->from_definition_id = from_def.value();
    out->to_definition_id = to_def.value();
    out->from_object_id = from_obj.value();
    out->to_object_id = to_obj.value();
    out->from_object_version = referee::Version{j.value("from_object_version", 0ULL)};
    out->to_object_version = referee::Version{j.value("to_object_version", 0ULL)};
    if (j.contains("hook")) out->hook = j.at("hook").get<std::string>();
    out->status = j.value("status", "");
    if (j.contains("note")) out->note = j.at("note").get<std::string>();
    return true;
  } catch (const std::exception& ex) {
    if (err_out) *err_out = ex.what();
    return false;
  }
}

referee::Result<TypeSummary> resolve_migration_record_type(SchemaRegistry& registry) {
  auto typesR = registry.list_types();
  if (!typesR) return referee::Result<TypeSummary>::err(typesR.error->message);
  std::string err;
  auto summary = find_type_summary(typesR.value.value(), "Refract::MigrationRecord", &err);
  if (!summary.has_value()) {
    return referee::Result<TypeSummary>::err(err);
  }
  return referee::Result<TypeSummary>::ok(summary.value());
}

referee::Result<std::unordered_map<std::string, MigrationRecordPayload>> load_migration_records(
    SchemaRegistry& registry,
    SqliteStore& store,
    TypeID type_id) {
  auto typeR = resolve_migration_record_type(registry);
  if (!typeR) return referee::Result<std::unordered_map<std::string, MigrationRecordPayload>>::err(typeR.error->message);

  auto listR = store.list_by_type(typeR.value->type_id);
  if (!listR) {
    return referee::Result<std::unordered_map<std::string, MigrationRecordPayload>>::err(listR.error->message);
  }

  std::unordered_map<std::string, MigrationRecordPayload> out;
  for (const auto& rec : listR.value.value()) {
    MigrationRecordPayload payload;
    std::string err;
    if (!parse_migration_record_payload(rec.payload_cbor, &payload, &err)) {
      return referee::Result<std::unordered_map<std::string, MigrationRecordPayload>>::err(err);
    }
    if (payload.type_id.v != type_id.v) continue;
    out.emplace(migration_record_key(payload.from_object_id, payload.to_definition_id), payload);
  }
  return referee::Result<std::unordered_map<std::string, MigrationRecordPayload>>::ok(std::move(out));
}

referee::Result<void> migrate_list(SchemaRegistry& registry,
                                   SqliteStore& store,
                                   const std::string& target) {
  auto targetR = resolve_migration_target(registry, target);
  if (!targetR) return referee::Result<void>::err(targetR.error->message);

  auto stepsR = build_migration_steps(registry, targetR.value->latest);
  if (!stepsR) return referee::Result<void>::err(stepsR.error->message);

  std::cout << "migrations " << targetR.value->display << "\n";
  std::cout << "latest " << targetR.value->latest.ref.id.to_hex()
            << " v" << targetR.value->latest.definition.version << "\n";

  if (stepsR.value->empty()) {
    std::cout << "  (none)\n";
    return referee::Result<void>::ok();
  }

  auto listR = store.list_by_type(targetR.value->summary.type_id);
  if (!listR) return referee::Result<void>::err(listR.error->message);

  std::unordered_map<std::string, std::size_t> counts;
  for (const auto& rec : listR.value.value()) {
    counts[rec.definition_id.to_hex()]++;
  }

  for (const auto& step : stepsR.value.value()) {
    auto from_hex = step.from.ref.id.to_hex();
    std::cout << "  " << from_hex << " v" << step.from.definition.version
              << " -> " << step.to.ref.id.to_hex() << " v" << step.to.definition.version;
    if (step.hook.has_value()) {
      std::cout << " hook=" << step.hook.value();
    } else {
      std::cout << " hook=<none>";
    }
    auto count_it = counts.find(from_hex);
    std::size_t count = (count_it != counts.end()) ? count_it->second : 0;
    std::cout << " objects=" << count << "\n";
  }
  return referee::Result<void>::ok();
}

referee::Result<void> migrate_apply(SchemaRegistry& registry,
                                    SqliteStore& store,
                                    const std::string& target) {
  auto targetR = resolve_migration_target(registry, target);
  if (!targetR) return referee::Result<void>::err(targetR.error->message);

  auto stepsR = build_migration_steps(registry, targetR.value->latest);
  if (!stepsR) return referee::Result<void>::err(stepsR.error->message);
  if (stepsR.value->empty()) {
    std::cout << "no migrations\n";
    return referee::Result<void>::ok();
  }

  std::unordered_map<std::string, MigrationStep> step_by_from;
  for (const auto& step : stepsR.value.value()) {
    step_by_from.emplace(step.from.ref.id.to_hex(), step);
  }

  auto recordsR = load_migration_records(registry, store, targetR.value->summary.type_id);
  if (!recordsR) return referee::Result<void>::err(recordsR.error->message);
  auto record_map = std::move(recordsR.value.value());

  auto listR = store.list_by_type(targetR.value->summary.type_id);
  if (!listR) return referee::Result<void>::err(listR.error->message);

  auto recordTypeR = resolve_migration_record_type(registry);
  if (!recordTypeR) return referee::Result<void>::err(recordTypeR.error->message);

  auto beginR = store.begin();
  if (!beginR) return referee::Result<void>::err(beginR.error->message);

  std::size_t migrated = 0;
  for (const auto& rec : listR.value.value()) {
    if (rec.definition_id == targetR.value->latest.ref.id) continue;
    auto current = rec;
    while (current.definition_id != targetR.value->latest.ref.id) {
      auto step_it = step_by_from.find(current.definition_id.to_hex());
      if (step_it == step_by_from.end()) {
        (void)store.rollback();
        return referee::Result<void>::err("missing migration step for definition");
      }
      const auto& step = step_it->second;
      auto key = migration_record_key(current.ref.id, step.to.ref.id);
      auto record_it = record_map.find(key);
      if (record_it != record_map.end()) {
        auto nextR = store.get_latest(record_it->second.to_object_id);
        if (!nextR) {
          (void)store.rollback();
          return referee::Result<void>::err(nextR.error->message);
        }
        if (!nextR.value->has_value()) {
          (void)store.rollback();
          return referee::Result<void>::err("migration record target missing");
        }
        current = nextR.value->value();
        continue;
      }

      auto createR = store.create_object(targetR.value->summary.type_id,
                                         step.to.ref.id,
                                         current.payload_cbor);
      if (!createR) {
        (void)store.rollback();
        return referee::Result<void>::err(createR.error->message);
      }

      nlohmann::json payload;
      payload["type_id"] = targetR.value->summary.type_id.v;
      payload["from_definition_id"] = current.definition_id.to_hex();
      payload["to_definition_id"] = step.to.ref.id.to_hex();
      payload["from_object_id"] = current.ref.id.to_hex();
      payload["from_object_version"] = current.ref.ver.v;
      payload["to_object_id"] = createR.value->ref.id.to_hex();
      payload["to_object_version"] = createR.value->ref.ver.v;
      if (step.hook.has_value()) payload["hook"] = step.hook.value();
      payload["status"] = "applied";

      auto record_cbor = nlohmann::json::to_cbor(payload);
      auto recordCreateR = store.create_object(recordTypeR.value->type_id,
                                               recordTypeR.value->definition_id,
                                               record_cbor);
      if (!recordCreateR) {
        (void)store.rollback();
        return referee::Result<void>::err(recordCreateR.error->message);
      }

      MigrationRecordPayload record_payload;
      record_payload.type_id = targetR.value->summary.type_id;
      record_payload.from_definition_id = current.definition_id;
      record_payload.to_definition_id = step.to.ref.id;
      record_payload.from_object_id = current.ref.id;
      record_payload.from_object_version = current.ref.ver;
      record_payload.to_object_id = createR.value->ref.id;
      record_payload.to_object_version = createR.value->ref.ver;
      record_payload.hook = step.hook;
      record_payload.status = "applied";
      record_map.emplace(key, std::move(record_payload));

      current = createR.value.value();
      migrated++;
    }
  }

  auto commitR = store.commit();
  if (!commitR) return referee::Result<void>::err(commitR.error->message);

  std::cout << "migrated " << migrated << " objects\n";
  return referee::Result<void>::ok();
}

referee::Result<void> migrate_verify(SchemaRegistry& registry,
                                     SqliteStore& store,
                                     const std::string& target) {
  auto targetR = resolve_migration_target(registry, target);
  if (!targetR) return referee::Result<void>::err(targetR.error->message);

  auto stepsR = build_migration_steps(registry, targetR.value->latest);
  if (!stepsR) return referee::Result<void>::err(stepsR.error->message);
  if (stepsR.value->empty()) {
    std::cout << "no migrations\n";
    return referee::Result<void>::ok();
  }

  std::unordered_map<std::string, MigrationStep> step_by_from;
  for (const auto& step : stepsR.value.value()) {
    step_by_from.emplace(step.from.ref.id.to_hex(), step);
  }

  auto recordsR = load_migration_records(registry, store, targetR.value->summary.type_id);
  if (!recordsR) return referee::Result<void>::err(recordsR.error->message);
  auto record_map = std::move(recordsR.value.value());

  auto listR = store.list_by_type(targetR.value->summary.type_id);
  if (!listR) return referee::Result<void>::err(listR.error->message);

  std::unordered_map<std::string, std::size_t> totals;
  std::unordered_map<std::string, std::size_t> missing;

  for (const auto& rec : listR.value.value()) {
    if (rec.definition_id == targetR.value->latest.ref.id) continue;
    auto current = rec;
    while (current.definition_id != targetR.value->latest.ref.id) {
      auto from_hex = current.definition_id.to_hex();
      totals[from_hex]++;
      auto step_it = step_by_from.find(from_hex);
      if (step_it == step_by_from.end()) {
        missing[from_hex]++;
        break;
      }
      const auto& step = step_it->second;
      auto key = migration_record_key(current.ref.id, step.to.ref.id);
      auto record_it = record_map.find(key);
      if (record_it == record_map.end()) {
        missing[from_hex]++;
        break;
      }
      auto nextR = store.get_latest(record_it->second.to_object_id);
      if (!nextR || !nextR.value->has_value()) {
        missing[from_hex]++;
        break;
      }
      current = nextR.value->value();
    }
  }

  std::cout << "migration verification " << targetR.value->display << "\n";
  std::size_t total_missing = 0;
  for (const auto& step : stepsR.value.value()) {
    auto from_hex = step.from.ref.id.to_hex();
    std::size_t total = 0;
    std::size_t miss = 0;
    auto total_it = totals.find(from_hex);
    if (total_it != totals.end()) total = total_it->second;
    auto miss_it = missing.find(from_hex);
    if (miss_it != missing.end()) miss = miss_it->second;
    total_missing += miss;
    std::cout << "  " << from_hex << " v" << step.from.definition.version
              << " -> " << step.to.ref.id.to_hex() << " v" << step.to.definition.version
              << " missing=" << miss << " of " << total << "\n";
  }

  if (total_missing > 0) {
    return referee::Result<void>::err("migration verification failed");
  }
  return referee::Result<void>::ok();
}

const nlohmann::json* payload_value(const nlohmann::json& payload) {
  if (payload.is_object()) {
    auto it = payload.find("value");
    if (it != payload.end()) return &(*it);
  }
  return &payload;
}

bool read_string_value(const nlohmann::json& payload, std::string* out, std::string* err_out) {
  const auto* value = payload_value(payload);
  if (value->is_string()) {
    *out = value->get<std::string>();
    return true;
  }
  if (err_out) *err_out = "expected string value";
  return false;
}

bool read_bool_value(const nlohmann::json& payload, bool* out, std::string* err_out) {
  const auto* value = payload_value(payload);
  if (value->is_boolean()) {
    *out = value->get<bool>();
    return true;
  }
  if (value->is_number_integer()) {
    *out = value->get<int>() != 0;
    return true;
  }
  if (err_out) *err_out = "expected bool value";
  return false;
}

bool read_u64_value(const nlohmann::json& payload, std::uint64_t* out, std::string* err_out) {
  const auto* value = payload_value(payload);
  if (value->is_number_unsigned()) {
    *out = value->get<std::uint64_t>();
    return true;
  }
  if (value->is_number_integer()) {
    auto v = value->get<std::int64_t>();
    if (v < 0) {
      if (err_out) *err_out = "expected unsigned value";
      return false;
    }
    *out = static_cast<std::uint64_t>(v);
    return true;
  }
  if (value->is_string()) {
    return parse_u64(value->get<std::string>(), out);
  }
  if (err_out) *err_out = "expected u64 value";
  return false;
}

bool read_double_value(const nlohmann::json& payload, double* out, std::string* err_out) {
  const auto* value = payload_value(payload);
  if (value->is_number()) {
    *out = value->get<double>();
    return true;
  }
  if (value->is_string()) {
    return parse_double(value->get<std::string>(), out);
  }
  if (err_out) *err_out = "expected f64 value";
  return false;
}

bool read_object_id_value(const nlohmann::json& payload, referee::ObjectID* out,
                          std::string* err_out) {
  const auto* value = payload_value(payload);
  if (value->is_string()) {
    auto hex = value->get<std::string>();
    if (hex.size() == 32) {
      *out = referee::ObjectID::from_hex(hex);
      return true;
    }
  }
  if (err_out) *err_out = "expected object id hex value";
  return false;
}

bool read_bytes_value(const nlohmann::json& payload, std::string* out, std::string* err_out) {
  const auto* value = payload_value(payload);
  if (value->is_string()) {
    *out = value->get<std::string>();
    return true;
  }
  if (value->is_array()) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(value->size());
    for (const auto& item : *value) {
      if (!item.is_number_integer() && !item.is_number_unsigned()) {
        if (err_out) *err_out = "invalid byte array";
        return false;
      }
      auto v = item.get<int>();
      if (v < 0 || v > 255) {
        if (err_out) *err_out = "invalid byte value";
        return false;
      }
      bytes.push_back(static_cast<std::uint8_t>(v));
    }
    *out = bytes_to_hex(bytes);
    return true;
  }
  if (err_out) *err_out = "expected bytes value";
  return false;
}

void print_help() {
  std::cout << "Commands:\n";
  std::cout << "  ls\n";
  std::cout << "  ls --objects [pattern]\n";
  std::cout << "  ls --recursive [pattern]\n";
  std::cout << "  ls --recursive --objects [pattern]\n";
  std::cout << "  ls --namespaces [pattern]\n";
  std::cout << "  ls [pattern]\n";
  std::cout << "  ls --regex <pattern>\n";
  std::cout << "  ls --regex --namespaces <pattern>\n";
  std::cout << "  namespace [<name>|/|.|..]\n";
  std::cout << "  ns [<name>|/|.|..]\n";
  std::cout << "  objects\n";
  std::cout << "  let <name>=<expr>\n";
  std::cout << "  let .\n";
  std::cout << "  var <name>=<expr>\n";
  std::cout << "  var .\n";
  std::cout << "  alias <name>=<expr>\n";
  std::cout << "  show <ObjectID>\n";
  std::cout << "  show type <TypeName>\n";
  std::cout << "  ops <TypeName> [--class|--object] [--declared]\n";
  std::cout << "  edges <ObjectID>\n";
  std::cout << "  find type <TypeName>\n";
  std::cout << "  define type <TypeName> fields <field>:<type>[?],...\n";
  std::cout << "  define type --json <spec>\n";
  std::cout << "  new <TypeName> field:=value ...\n";
  std::cout << "  new --json <spec>\n";
  std::cout << "  call <ObjectID> <opName> [args...]\n";
  std::cout << "  caps\n";
  std::cout << "  caps grant <cap> [cap...]\n";
  std::cout << "  caps revoke <cap> [cap...]\n";
  std::cout << "  caps clear\n";
  std::cout << "  start <ObjectID>\n";
  std::cout << "  ps\n";
  std::cout << "  kill <TaskID>\n";
  std::cout << "  task spawn <ObjectID> [name] [inline|service]\n";
  std::cout << "  task list\n";
  std::cout << "  task profile\n";
  std::cout << "  task trace [clear]\n";
  std::cout << "  debug dispatch <TypeName|ObjectID> <opName> "
               "[--class|--object] [--declared] [argType...]\n";
  std::cout << "  debug graph <ObjectID>\n";
  std::cout << "  io open <channel|datagram> <taskA> <taskB>\n";
  std::cout << "  io send <handle> <hexbytes>\n";
  std::cout << "  io await <handle> <taskId>\n";
  std::cout << "  io recv <handle> [max_bytes]\n";
  std::cout << "  io close <handle>\n";
  std::cout << "  io handles\n";
  std::cout << "  io aliases\n";
  std::cout << "  io alias <handle> <name>\n";
  std::cout << "  io unalias <name>\n";
  std::cout << "  edge <fromObjectID> <toObjectID> <name> [role]\n";
  std::cout << "  migrate list <TypeName|DefinitionID>\n";
  std::cout << "  migrate apply <TypeName|DefinitionID>\n";
  std::cout << "  migrate verify <TypeName|DefinitionID>\n";
  std::cout << "  emit viz <textlog|metric|table|tree|panel> [args...]\n";
  std::cout << "    [--produced-by <id>] [--progress-of <id>] [--diagnostic-of <id>] [--role <role>]\n";
  std::cout << "  demo v1\n";
  std::cout << "  route <ObjectID>\n";
  std::cout << "  route type <TypeName>\n";
  std::cout << "  note: ObjectID prefixes (>=4 hex chars) resolve when unambiguous\n";
  std::cout << "  help\n";
  std::cout << "  exit\n";
}

void cmd_ls(SchemaRegistry& registry, SqliteStore& store, const TypeListOptions& options) {
  auto typesR = registry.list_types();
  if (!typesR) {
    std::cout << "error: " << typesR.error->message << "\n";
    return;
  }
  if (typesR.value->empty()) {
    std::cout << "no types registered\n";
    return;
  }

  std::vector<TypeSummary> types = typesR.value.value();
  std::sort(types.begin(), types.end(),
            [](const TypeSummary& a, const TypeSummary& b) {
              auto a_name = type_display_name(a);
              auto b_name = type_display_name(b);
              if (a_name != b_name) return a_name < b_name;
              return a.type_id.v < b.type_id.v;
            });

  if (options.recursive) {
    std::set<std::string> namespaces;
    for (const auto& summary : types) {
      if (summary.namespace_name.empty()) continue;
      if (!namespace_in_scope(options.current_namespace, summary.namespace_name)) continue;
      if (summary.namespace_name == options.current_namespace) continue;

      size_t offset = options.current_namespace.empty() ? 0 : options.current_namespace.size() + 2;
      std::string_view remaining(summary.namespace_name.c_str() + offset,
                                 summary.namespace_name.size() - offset);
      size_t start = 0;
      while (start < remaining.size()) {
        auto pos = remaining.find("::", start);
        size_t len = pos == std::string_view::npos ? remaining.size() : pos;
        auto prefix = std::string(remaining.substr(0, len));
        if (!prefix.empty()) {
          if (options.current_namespace.empty()) {
            namespaces.insert(prefix);
          } else {
            namespaces.insert(options.current_namespace + "::" + prefix);
          }
        }
        if (pos == std::string_view::npos) break;
        start = pos + 2;
      }
    }

    bool any = false;
    for (const auto& ns : namespaces) {
      bool matched = false;
      std::string err;
      if (!listing_matches(ns, options.filter, options.regex_mode, &matched, &err)) {
        std::cout << "error: " << err << "\n";
        return;
      }
      if (!matched) continue;
      any = true;
      std::cout << "namespace " << ns << "\n";
    }

    if (!options.namespaces_only) {
      for (const auto& summary : types) {
        if (!namespace_in_scope(options.current_namespace, summary.namespace_name)) continue;
        auto display = type_display_name(summary);
        bool matched = false;
        std::string err;
        if (!listing_matches(display, options.filter, options.regex_mode, &matched, &err)) {
          std::cout << "error: " << err << "\n";
          return;
        }
        if (!matched) continue;
        any = true;
        std::cout << "type " << display << " (0x" << std::hex << summary.type_id.v
                  << std::dec << ")\n";
        if (!options.include_objects) continue;

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

    if (!any) {
      std::cout << (options.namespaces_only ? "no namespaces\n" : "no entries\n");
    }
    return;
  }

  std::set<std::string> namespaces;
  for (const auto& summary : types) {
    auto child = direct_child_namespace(options.current_namespace, summary.namespace_name);
    if (child.has_value()) namespaces.insert(child.value());
  }

  bool any = false;
  for (const auto& ns : namespaces) {
    bool matched = false;
    std::string err;
    if (!listing_matches(ns, options.filter, options.regex_mode, &matched, &err)) {
      std::cout << "error: " << err << "\n";
      return;
    }
    if (!matched) continue;
    any = true;
    std::cout << "namespace " << ns << "\n";
  }

  if (!options.namespaces_only) {
    for (const auto& summary : types) {
      if (summary.namespace_name != options.current_namespace) continue;
      if (options.current_namespace.empty() && !options.include_objects) continue;

      auto display = relative_type_name(options.current_namespace, summary);
      bool matched = false;
      std::string err;
      if (!listing_matches(display, options.filter, options.regex_mode, &matched, &err)) {
        std::cout << "error: " << err << "\n";
        return;
      }
      if (!matched) continue;
      any = true;
      std::cout << "type " << display << " (0x" << std::hex << summary.type_id.v
                << std::dec << ")\n";
      if (!options.include_objects) continue;

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

  if (!any) {
    std::cout << (options.namespaces_only ? "no namespaces\n" : "no entries\n");
  }
}

void cmd_objects(SchemaRegistry& registry, SqliteStore& store) {
  auto typesR = registry.list_types();
  if (!typesR) {
    std::cout << "error: " << typesR.error->message << "\n";
    return;
  }
  bool any = false;
  for (const auto& summary : typesR.value.value()) {
    auto listR = store.list_by_type(summary.type_id);
    if (!listR) {
      std::cout << "error: " << listR.error->message << "\n";
      return;
    }
    for (const auto& rec : listR.value.value()) {
      any = true;
      std::cout << rec.ref.id.to_hex() << " type=" << type_display_name(summary)
                << " v" << rec.ref.ver.v << "\n";
    }
  }
  if (!any) std::cout << "no objects\n";
}

std::string make_prompt(const std::string& current_namespace) {
  if (current_namespace.empty()) return "conch> ";
  return "conch:" + current_namespace + "> ";
}

std::optional<iris::refract::FieldDefinition> parse_field_spec(
    SchemaRegistry& registry, const std::string& token, std::string* err_out) {
  auto pos = token.find(':');
  if (pos == std::string::npos) {
    if (err_out) *err_out = "field spec missing ':'";
    return std::nullopt;
  }
  std::string name = token.substr(0, pos);
  std::string type_name = token.substr(pos + 1);
  if (name.empty() || type_name.empty()) {
    if (err_out) *err_out = "field spec missing name or type";
    return std::nullopt;
  }
  bool required = true;
  if (!name.empty() && name.back() == '?') {
    required = false;
    name.pop_back();
  }

  std::string err;
  auto type_summary = resolve_type(registry, type_name, &err);
  if (!type_summary.has_value()) {
    if (err_out) *err_out = "unknown field type: " + err;
    return std::nullopt;
  }

  iris::refract::FieldDefinition field;
  field.name = name;
  field.type = type_summary->type_id;
  field.required = required;
  return field;
}

std::optional<iris::refract::FieldConstraint> parse_field_constraint_json(
    const nlohmann::json& item,
    std::string* err_out) {
  std::string kind = item.is_string() ? item.get<std::string>() : item.value("kind", "");
  if (kind == "required") {
    return iris::refract::FieldConstraint{ iris::refract::FieldConstraintKind::Required };
  }
  if (kind == "non_empty") {
    return iris::refract::FieldConstraint{ iris::refract::FieldConstraintKind::NonEmpty };
  }
  if (err_out) *err_out = "unknown field constraint kind";
  return std::nullopt;
}

std::optional<iris::refract::RelationshipConstraint> parse_relationship_constraint_json(
    const nlohmann::json& item,
    std::string* err_out) {
  if (!item.is_object()) {
    if (err_out) *err_out = "relationship constraint must be an object";
    return std::nullopt;
  }

  std::string kind = item.value("kind", "");
  if (!item.contains("value")) {
    if (err_out) *err_out = "relationship constraint missing value";
    return std::nullopt;
  }

  iris::refract::RelationshipConstraint constraint{};
  if (kind == "min_occurs") {
    constraint.kind = iris::refract::RelationshipConstraintKind::MinOccurs;
  } else if (kind == "max_occurs") {
    constraint.kind = iris::refract::RelationshipConstraintKind::MaxOccurs;
  } else {
    if (err_out) *err_out = "unknown relationship constraint kind";
    return std::nullopt;
  }

  constraint.value = item.at("value").get<std::uint64_t>();
  return constraint;
}

std::optional<iris::refract::TypeDefinition> parse_define_inline(
    SchemaRegistry& registry, const std::vector<std::string>& tokens, std::string* err_out) {
  if (tokens.size() < 5) {
    if (err_out) *err_out = "usage: define type <TypeName> fields <field>:<type>[?],...";
    return std::nullopt;
  }
  auto type_name = tokens[2];
  if (tokens[3] != "fields") {
    if (err_out) *err_out = "missing fields clause";
    return std::nullopt;
  }
  auto field_text = join_tokens(tokens, 4);
  std::vector<std::string> field_specs;
  std::string current;
  for (char c : field_text) {
    if (c == ',') {
      if (!current.empty()) field_specs.push_back(current);
      current.clear();
      continue;
    }
    if (c != ' ' && c != '\t') current.push_back(c);
  }
  if (!current.empty()) field_specs.push_back(current);
  if (field_specs.empty()) {
    if (err_out) *err_out = "no fields defined";
    return std::nullopt;
  }

  auto [ns, name] = split_type_name(type_name);
  iris::refract::TypeDefinition def{};
  def.name = name;
  def.namespace_name = ns;
  def.version = 1;

  for (const auto& spec : field_specs) {
    std::string err;
    auto field = parse_field_spec(registry, spec, &err);
    if (!field.has_value()) {
      if (err_out) *err_out = err;
      return std::nullopt;
    }
    def.fields.push_back(field.value());
  }

  std::string full = ns.empty() ? name : ns + "::" + name;
  def.type_id = referee::TypeID{fnv1a_64(full)};
  return def;
}

std::optional<iris::refract::TypeDefinition> parse_define_json(
    SchemaRegistry& registry, const std::string& json_text, std::string* err_out) {
  try {
    auto j = nlohmann::json::parse(json_text);
    iris::refract::TypeDefinition def{};
    def.name = j.value("name", "");
    def.namespace_name = j.value("namespace", "");
    def.version = j.value("version", 1ULL);
    if (j.contains("supersedes_definition_id")) {
      std::string err;
      auto supersedes = parse_object_id(j.at("supersedes_definition_id").get<std::string>(), &err);
      if (!supersedes.has_value()) {
        if (err_out) *err_out = "invalid supersedes_definition_id: " + err;
        return std::nullopt;
      }
      def.supersedes_definition_id = supersedes.value();
    }
    if (j.contains("migration_hook")) {
      def.migration_hook = j.at("migration_hook").get<std::string>();
    }
    if (j.contains("kind")) def.kind = j.at("kind").get<std::string>();
    if (def.name.empty()) {
      if (err_out) *err_out = "json missing name";
      return std::nullopt;
    }

    if (j.contains("fields")) {
      for (const auto& item : j.at("fields")) {
        std::string field_name = item.value("name", "");
        std::string field_type = item.value("type", "");
        bool required = item.value("required", false);
        if (field_name.empty() || field_type.empty()) {
          if (err_out) *err_out = "field missing name or type";
          return std::nullopt;
        }
        std::string err;
        auto type_summary = resolve_type(registry, field_type, &err);
        if (!type_summary.has_value()) {
          if (err_out) *err_out = "unknown field type: " + err;
          return std::nullopt;
        }
        iris::refract::FieldDefinition field;
        field.name = field_name;
        field.type = type_summary->type_id;
        field.required = required;
        if (item.contains("constraints")) {
          for (const auto& constraint_item : item.at("constraints")) {
            auto constraint = parse_field_constraint_json(constraint_item, err_out);
            if (!constraint.has_value()) return std::nullopt;
            field.constraints.push_back(constraint.value());
          }
        }
        def.fields.push_back(std::move(field));
      }
    }

    if (j.contains("relationships")) {
      for (const auto& item : j.at("relationships")) {
        std::string role = item.value("role", "");
        std::string cardinality = item.value("cardinality", "");
        std::string target_name = item.value("target", "");
        if (role.empty() || target_name.empty()) {
          if (err_out) *err_out = "relationship missing role or target";
          return std::nullopt;
        }
        iris::refract::RelationshipSpec rel;
        rel.role = role;
        rel.cardinality = cardinality;
        rel.target = target_name;
        if (item.contains("constraints")) {
          for (const auto& constraint_item : item.at("constraints")) {
            auto constraint = parse_relationship_constraint_json(constraint_item, err_out);
            if (!constraint.has_value()) return std::nullopt;
            rel.constraints.push_back(constraint.value());
          }
        }
        def.relationships.push_back(std::move(rel));
      }
    }

    if (j.contains("enum_value_type")) {
      std::string err;
      auto type_summary = resolve_type(registry, j.at("enum_value_type").get<std::string>(), &err);
      if (!type_summary.has_value()) {
        if (err_out) *err_out = "unknown enum value type: " + err;
        return std::nullopt;
      }
      def.enum_value_type = type_summary->type_id;
      def.has_enum_value_type = true;
    }
    if (j.contains("enum_values")) {
      for (const auto& item : j.at("enum_values")) {
        std::string value_name = item.value("name", "");
        if (value_name.empty()) {
          if (err_out) *err_out = "enum value missing name";
          return std::nullopt;
        }
        iris::refract::EnumValueDefinition value;
        value.name = value_name;
        if (item.contains("value_json")) {
          value.value_json = item.at("value_json").get<std::string>();
        } else if (item.contains("value")) {
          value.value_json = item.at("value").dump();
        } else {
          if (err_out) *err_out = "enum value missing value";
          return std::nullopt;
        }
        def.enum_values.push_back(std::move(value));
      }
    }
    if (j.contains("packet_byte_order")) {
      def.packet_byte_order = j.at("packet_byte_order").get<std::string>();
    }
    if (j.contains("packet_fields")) {
      for (const auto& item : j.at("packet_fields")) {
        std::string field_name = item.value("name", "");
        std::string field_type = item.value("type", "");
        auto bit_width = item.value("bit_width", 0U);
        if (field_name.empty() || field_type.empty() || bit_width == 0U) {
          if (err_out) *err_out = "packet field missing name, type, or bit_width";
          return std::nullopt;
        }
        std::string err;
        auto type_summary = resolve_type(registry, field_type, &err);
        if (!type_summary.has_value()) {
          if (err_out) *err_out = "unknown packet field type: " + err;
          return std::nullopt;
        }
        iris::refract::PacketFieldDefinition field;
        field.name = field_name;
        field.type = type_summary->type_id;
        field.bit_width = bit_width;
        def.packet_fields.push_back(std::move(field));
      }
    }

    if (j.contains("collection_kind")) {
      def.collection_kind = j.at("collection_kind").get<std::string>();
    }
    if (j.contains("collection_elements")) {
      for (const auto& item : j.at("collection_elements")) {
        std::string role = item.value("role", "");
        std::string element_type = item.value("type", "");
        if (element_type.empty()) {
          if (err_out) *err_out = "collection element missing type";
          return std::nullopt;
        }
        std::string err;
        auto type_summary = resolve_type(registry, element_type, &err);
        if (!type_summary.has_value()) {
          if (err_out) *err_out = "unknown collection element type: " + err;
          return std::nullopt;
        }
        iris::refract::CollectionElementDefinition element;
        element.role = role;
        element.type = type_summary->type_id;
        def.collection_elements.push_back(std::move(element));
      }
    } else if (j.contains("element_type")) {
      std::string err;
      auto type_summary = resolve_type(registry, j.at("element_type").get<std::string>(), &err);
      if (!type_summary.has_value()) {
        if (err_out) *err_out = "unknown collection element type: " + err;
        return std::nullopt;
      }
      iris::refract::CollectionElementDefinition element;
      element.role = "element";
      element.type = type_summary->type_id;
      def.collection_elements.push_back(std::move(element));
    }

    if (j.contains("type_id")) {
      def.type_id = referee::TypeID{j.at("type_id").get<std::uint64_t>()};
    } else {
      std::string full = def.namespace_name.empty()
        ? def.name
        : def.namespace_name + "::" + def.name;
      def.type_id = referee::TypeID{fnv1a_64(full)};
    }
    return def;
  } catch (const std::exception& ex) {
    if (err_out) *err_out = ex.what();
    return std::nullopt;
  }
}

void cmd_define_type(SchemaRegistry& registry, const std::vector<std::string>& tokens) {
  std::string err;
  iris::refract::TypeDefinition def{};
  bool ok = false;

  if (tokens.size() >= 3 && tokens[2] == "--json") {
    auto json_text = strip_quotes(join_tokens(tokens, 3));
    auto parsed = parse_define_json(registry, json_text, &err);
    if (parsed.has_value()) {
      def = std::move(parsed.value());
      ok = true;
    }
  } else {
    auto parsed = parse_define_inline(registry, tokens, &err);
    if (parsed.has_value()) {
      def = std::move(parsed.value());
      ok = true;
    }
  }

  if (!ok) {
    std::cout << "error: " << err << "\n";
    return;
  }

  auto existing = registry.get_definition_by_type(def.type_id);
  if (!existing) {
    std::cout << "error: " << existing.error->message << "\n";
    return;
  }
  if (existing.value->has_value()) {
    std::cout << "error: type already exists\n";
    return;
  }

  auto reg = registry.register_definition(def);
  if (!reg) {
    std::cout << "error: " << reg.error->message << "\n";
    return;
  }
  std::string display = def.namespace_name.empty()
    ? def.name
    : def.namespace_name + "::" + def.name;
  std::cout << "defined type " << display
            << " id=0x" << std::hex << def.type_id.v << std::dec
            << " def=" << reg.value->ref.id.to_hex() << "\n";
}

void cmd_new_object(SchemaRegistry& registry, SqliteStore& store, const std::string& line) {
  try {
    auto createR = create_object(registry, store, line);
    if (!createR) {
      std::cout << "error: " << createR.error->message << "\n";
      return;
    }
    std::cout << "created " << createR.value->to_hex() << "\n";
  } catch (const std::exception& ex) {
    std::cout << "error: " << ex.what() << "\n";
  }
}

bool handle_object_command(SchemaRegistry& registry,
                           SqliteStore& store,
                           const std::unordered_map<std::string, ObjectID>& session_aliases,
                           const iris::parser::ObjectCommand& command) {
  if (command.kind == iris::parser::ObjectCommandKind::New) {
    cmd_new_object(registry, store, command.expression);
    return true;
  }

  std::string err;
  auto id = parse_object_id_or_alias(command.target, session_aliases, store, registry, &err);
  if (!id.has_value()) {
    std::cout << "error: " << err << "\n";
    return true;
  }

  if (command.kind == iris::parser::ObjectCommandKind::Show) {
    cmd_show(registry, store, id.value());
    return true;
  }
  if (command.kind == iris::parser::ObjectCommandKind::Edges) {
    cmd_edges(store, id.value());
    return true;
  }
  return false;
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

std::string format_field_constraint(const iris::refract::FieldConstraint& constraint) {
  switch (constraint.kind) {
    case iris::refract::FieldConstraintKind::Required:
      return "required";
    case iris::refract::FieldConstraintKind::NonEmpty:
      return "non_empty";
  }
  return "required";
}

std::string format_relationship_constraint(
    const iris::refract::RelationshipConstraint& constraint) {
  switch (constraint.kind) {
    case iris::refract::RelationshipConstraintKind::MinOccurs:
      return "min_occurs=" + std::to_string(constraint.value);
    case iris::refract::RelationshipConstraintKind::MaxOccurs:
      return "max_occurs=" + std::to_string(constraint.value);
  }
  return "min_occurs=0";
}

std::string format_field_constraints(const iris::refract::FieldDefinition& field) {
  if (field.constraints.empty()) return "";
  std::ostringstream os;
  os << " constraints=";
  for (size_t i = 0; i < field.constraints.size(); ++i) {
    if (i > 0) os << ",";
    os << format_field_constraint(field.constraints[i]);
  }
  return os.str();
}

std::string format_relationship_constraints(const iris::refract::RelationshipSpec& rel) {
  if (rel.constraints.empty()) return "";
  std::ostringstream os;
  os << " constraints=";
  for (size_t i = 0; i < rel.constraints.size(); ++i) {
    if (i > 0) os << ",";
    os << format_relationship_constraint(rel.constraints[i]);
  }
  return os.str();
}

void cmd_show_type(SchemaRegistry& registry, const std::string& name) {
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
  auto defR = registry.get_definition_by_type(match->type_id);
  if (!defR) {
    std::cout << "error: " << defR.error->message << "\n";
    return;
  }
  if (!defR.value->has_value()) {
    std::cout << "error: definition not found\n";
    return;
  }

  const auto& def = defR.value->value().definition;
  std::cout << "type " << type_display_name(*match) << " v" << def.version << "\n";
  if (def.kind.has_value()) {
    std::cout << "kind " << def.kind.value() << "\n";
  }
  std::string inheritance_err;
  auto base_types = resolve_base_types(registry, match->type_id, &inheritance_err);
  if (!inheritance_err.empty()) {
    std::cout << "error: " << inheritance_err << "\n";
    return;
  }
  auto interface_types = resolve_interface_types(registry, match->type_id, &inheritance_err);
  if (!inheritance_err.empty()) {
    std::cout << "error: " << inheritance_err << "\n";
    return;
  }
  print_type_line("base types", typesR.value.value(), base_types);
  print_type_line("interface types", typesR.value.value(), interface_types);
  if (!def.type_params.empty()) {
    std::cout << "type params";
    for (const auto& param : def.type_params) {
      std::cout << " " << param;
    }
    std::cout << "\n";
  }
  if (def.has_enum_value_type) {
    std::cout << "enum value type=0x" << std::hex << def.enum_value_type.v << std::dec << "\n";
  }
  if (!def.enum_values.empty()) {
    std::cout << "enum values\n";
    for (const auto& value : def.enum_values) {
      std::cout << "  " << value.name << " value=" << value.value_json << "\n";
    }
  }
  if (def.packet_byte_order.has_value()) {
    std::cout << "packet byte order " << def.packet_byte_order.value() << "\n";
  }
  if (!def.packet_fields.empty()) {
    std::cout << "packet fields\n";
    for (const auto& field : def.packet_fields) {
      std::cout << "  " << field.name << " type=0x" << std::hex << field.type.v
                << std::dec << " bits=" << field.bit_width << "\n";
    }
  }
  if (def.collection_kind.has_value()) {
    std::cout << "collection kind " << def.collection_kind.value() << "\n";
  }
  if (!def.collection_elements.empty()) {
    std::cout << "collection elements\n";
    for (const auto& element : def.collection_elements) {
      std::cout << "  " << element.role << " type=0x" << std::hex << element.type.v
                << std::dec << "\n";
    }
  }
  if (!def.relationships.empty()) {
    std::cout << "relationships\n";
    for (const auto& rel : def.relationships) {
      std::cout << "  role=" << rel.role;
      if (!rel.cardinality.empty()) std::cout << " card=" << rel.cardinality;
      if (!rel.target.empty()) std::cout << " target=" << rel.target;
      std::cout << format_relationship_constraints(rel);
      std::cout << "\n";
    }
  }
  if (!def.fields.empty()) {
    std::cout << "fields\n";
    for (const auto& field : def.fields) {
      std::cout << "  " << field.name << " type=0x" << std::hex << field.type.v << std::dec;
      if (field.required) std::cout << " required";
      if (field.default_json.has_value()) std::cout << " default=" << field.default_json.value();
      std::cout << format_field_constraints(field);
      std::cout << "\n";
    }
  }
  print_operations(registry, typesR.value.value(), match->type_id, std::nullopt, true);
}

void cmd_ops(SchemaRegistry& registry, const std::vector<std::string>& args) {
  if (args.empty()) {
    std::cout << "error: ops <type> [--class|--object] [--declared]\n";
    return;
  }

  auto typesR = registry.list_types();
  if (!typesR) {
    std::cout << "error: " << typesR.error->message << "\n";
    return;
  }

  std::string err;
  auto match = find_type_summary(typesR.value.value(), args[0], &err);
  if (!match.has_value()) {
    std::cout << "error: " << err << "\n";
    return;
  }

  std::optional<OperationScope> scope_filter;
  bool include_inherited = true;

  for (size_t i = 1; i < args.size(); ++i) {
    const auto& token = args[i];
    if (token == "--class") {
      scope_filter = OperationScope::Class;
      continue;
    }
    if (token == "--object") {
      scope_filter = OperationScope::Object;
      continue;
    }
    if (token == "--declared") {
      include_inherited = false;
      continue;
    }
    std::cout << "error: unknown option '" << token << "'\n";
    return;
  }

  print_operations(registry, typesR.value.value(), match->type_id, scope_filter, include_inherited);
}

bool handle_schema_command(SchemaRegistry& registry,
                           SqliteStore& store,
                           const iris::parser::SchemaCommand& command) {
  (void)store;
  switch (command.kind) {
    case iris::parser::SchemaCommandKind::DefineType:
      cmd_define_type(registry, command.tokens);
      return true;
    case iris::parser::SchemaCommandKind::FindType:
      cmd_find_type(registry, command.type_name);
      return true;
    case iris::parser::SchemaCommandKind::ShowType:
      cmd_show_type(registry, command.type_name);
      return true;
    case iris::parser::SchemaCommandKind::Ops:
      cmd_ops(registry, command.args);
      return true;
  }
  return false;
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
      if (!op.signature.outputs.empty()) {
        std::cout << " -> ";
        if (op.signature.outputs.size() > 1) std::cout << "(";
        for (size_t i = 0; i < op.signature.outputs.size(); ++i) {
          if (i > 0) std::cout << ", ";
          const auto& out = op.signature.outputs[i];
          if (!out.name.empty()) {
            std::cout << out.name;
          } else {
            std::cout << "result";
          }
          if (out.optional) std::cout << "?";
        }
        if (op.signature.outputs.size() > 1) std::cout << ")";
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

bool cmd_debug_dispatch(SchemaRegistry& registry,
                        SqliteStore& store,
                        const std::unordered_map<std::string, ObjectID>& session_aliases,
                        const std::vector<std::string>& args) {
  if (args.size() < 2) {
    std::cout << "error: usage: debug dispatch <TypeName|ObjectID> <opName> "
                 "[--class|--object] [--declared] [argType...]\n";
    return false;
  }

  std::string target_token = args[0];
  std::string op_name = args[1];
  iris::refract::OperationScope scope = iris::refract::OperationScope::Object;
  bool include_inherited = true;
  std::vector<TypeID> arg_types;

  for (size_t i = 2; i < args.size(); ++i) {
    const auto& token = args[i];
    if (token == "--class") {
      scope = iris::refract::OperationScope::Class;
      continue;
    }
    if (token == "--object") {
      scope = iris::refract::OperationScope::Object;
      continue;
    }
    if (token == "--declared") {
      include_inherited = false;
      continue;
    }
    std::string err;
    auto summary = resolve_type(registry, token, &err);
    if (!summary.has_value()) {
      std::cout << "error: " << err << "\n";
      return false;
    }
    arg_types.push_back(summary->type_id);
  }

  std::optional<ObjectID> object_id;
  auto alias_it = session_aliases.find(target_token);
  if (alias_it != session_aliases.end()) {
    object_id = alias_it->second;
  } else {
    std::string err;
    auto parsed = parse_object_id(target_token, &err);
    if (parsed.has_value()) object_id = parsed.value();
  }

  auto typesR = registry.list_types();
  if (!typesR) {
    std::cout << "error: " << typesR.error->message << "\n";
    return false;
  }
  const auto& types = typesR.value.value();

  TypeID target_type{};
  std::string target_display;
  if (object_id.has_value()) {
    auto recR = store.get_latest(object_id.value());
    if (!recR) {
      std::cout << "error: " << recR.error->message << "\n";
      return false;
    }
    if (!recR.value->has_value()) {
      std::cout << "error: object not found\n";
      return false;
    }
    target_type = recR.value->value().type;
    target_display = type_display_name_for(types, target_type);
  } else {
    std::string err;
    auto summary = find_type_summary(types, target_token, &err);
    if (!summary.has_value()) {
      std::cout << "error: " << err << "\n";
      return false;
    }
    target_type = summary->type_id;
    target_display = type_display_name(*summary);
  }

  auto listR = list_operations_with_inheritance(registry, target_type, include_inherited);
  if (!listR) {
    std::cout << "error: " << listR.error->message << "\n";
    return false;
  }

  std::cout << "dispatch trace " << target_display << " op=" << op_name
            << " scope=" << (scope == iris::refract::OperationScope::Class ? "class" : "object")
            << " args=" << arg_types.size();
  if (!include_inherited) std::cout << " declared";
  std::cout << "\n";

  if (!arg_types.empty()) {
    std::cout << "arg types";
    for (const auto& type_id : arg_types) {
      std::cout << " " << type_display_name_for(types, type_id);
    }
    std::cout << "\n";
  }

  std::vector<DispatchCandidate> matches;
  for (const auto& entry : listR.value.value()) {
    if (entry.operation.scope != scope) continue;
    if (entry.operation.name != op_name) continue;

    const auto& op = entry.operation;
    if (!matches_arity(op, arg_types.size())) {
      std::cout << "  skip " << type_display_name_for(types, entry.owner)
                << " depth=" << entry.depth << " " << op.name << format_signature(op)
                << " reason=arity\n";
      continue;
    }

    std::size_t type_penalty = 0;
    bool type_ok = true;
    if (!arg_types.empty()) {
      for (std::size_t i = 0; i < arg_types.size(); ++i) {
        const auto& arg_type = arg_types[i];
        const auto& param_type = op.signature.params[i].type;
        if (arg_type.v == param_type.v) continue;
        std::string err;
        if (has_base_type(arg_type, param_type, registry, &err)) {
          type_penalty += 1;
          continue;
        }
        if (!err.empty()) {
          std::cout << "error: " << err << "\n";
          return false;
        }
        type_ok = false;
        break;
      }
    }
    if (!type_ok) {
      std::cout << "  skip " << type_display_name_for(types, entry.owner)
                << " depth=" << entry.depth << " " << op.name << format_signature(op)
                << " reason=type-mismatch\n";
      continue;
    }

    DispatchCandidate cand;
    cand.listing = entry;
    cand.type_penalty = type_penalty;
    cand.optional_penalty = op.signature.params.size() - arg_types.size();
    matches.push_back(cand);

    std::cout << "  match " << type_display_name_for(types, entry.owner)
              << " depth=" << entry.depth << " " << op.name << format_signature(op)
              << " type_penalty=" << type_penalty
              << " optional_penalty=" << cand.optional_penalty << "\n";
  }

  if (matches.empty()) {
    std::cout << "no matching operation\n";
    return true;
  }

  auto better = [](const DispatchCandidate& a, const DispatchCandidate& b) {
    if (a.type_penalty != b.type_penalty) return a.type_penalty < b.type_penalty;
    if (a.optional_penalty != b.optional_penalty) return a.optional_penalty < b.optional_penalty;
    return a.listing.depth < b.listing.depth;
  };

  DispatchCandidate best = matches.front();
  for (std::size_t i = 1; i < matches.size(); ++i) {
    if (better(matches[i], best)) best = matches[i];
  }

  std::vector<DispatchCandidate> ties;
  for (const auto& cand : matches) {
    if (!better(best, cand) && !better(cand, best)) ties.push_back(cand);
  }

  if (ties.size() > 1) {
    std::cout << "ambiguous operation:";
    for (const auto& cand : ties) {
      const auto& op = cand.listing.operation;
      std::cout << " " << type_display_name_for(types, cand.listing.owner)
                << " " << op.name << format_signature(op) << ";";
    }
    std::cout << "\n";
    return true;
  }

  const auto& op = best.listing.operation;
  std::cout << "selected " << type_display_name_for(types, best.listing.owner)
            << " depth=" << best.listing.depth << " " << op.name << format_signature(op)
            << " type_penalty=" << best.type_penalty
            << " optional_penalty=" << best.optional_penalty << "\n";
  return true;
}

bool cmd_debug_graph(SchemaRegistry& registry,
                     SqliteStore& store,
                     const std::unordered_map<std::string, ObjectID>& session_aliases,
                     const std::vector<std::string>& args) {
  if (args.size() != 1) {
    std::cout << "error: usage: debug graph <ObjectID>\n";
    return false;
  }

  std::string err;
  auto id = parse_object_id_or_alias(args[0], session_aliases, store, registry, &err);
  if (!id.has_value()) {
    std::cout << "error: " << err << "\n";
    return false;
  }

  auto recR = store.get_latest(id.value());
  if (!recR) {
    std::cout << "error: " << recR.error->message << "\n";
    return false;
  }
  if (!recR.value->has_value()) {
    std::cout << "error: object not found\n";
    return false;
  }

  auto typesR = registry.list_types();
  if (!typesR) {
    std::cout << "error: " << typesR.error->message << "\n";
    return false;
  }
  const auto& types = typesR.value.value();

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
  std::cout << "graph " << recR.value->value().ref.id.to_hex()
            << " type " << type_display_name_for(types, def.type_id)
            << " v" << def.version << "\n";

  std::unordered_map<std::string, std::vector<iris::refract::RelationshipSpec>> relationships;
  if (!def.relationships.empty()) {
    std::cout << "relationships\n";
    for (const auto& rel : def.relationships) {
      relationships[rel.role].push_back(rel);
      std::cout << "  role=" << rel.role;
      if (!rel.cardinality.empty()) std::cout << " card=" << rel.cardinality;
      if (!rel.target.empty()) std::cout << " target=" << rel.target;
      std::cout << "\n";
    }
  } else {
    std::cout << "relationships (none)\n";
  }

  auto outR = store.edges_from(recR.value->value().ref);
  if (!outR) {
    std::cout << "error: " << outR.error->message << "\n";
    return false;
  }
  auto inR = store.edges_to(recR.value->value().ref);
  if (!inR) {
    std::cout << "error: " << inR.error->message << "\n";
    return false;
  }

  std::cout << "edges from " << recR.value->value().ref.id.to_hex() << "\n";
  if (outR.value->empty()) {
    std::cout << "  (none)\n";
  } else {
    for (const auto& edge : outR.value.value()) {
      std::string target_display = "<unknown>";
      auto targetR = store.get_latest(edge.to.id);
      if (targetR && targetR.value->has_value()) {
        target_display = type_display_name_for(types, targetR.value->value().type);
      }
      std::string note;
      auto rel_it = relationships.find(edge.role);
      if (rel_it == relationships.end()) {
        note = "unexpected-role";
      } else if (!rel_it->second.empty()) {
        bool target_ok = false;
        for (const auto& rel : rel_it->second) {
          std::string target_err;
          if (rel.target.empty()) {
            target_ok = true;
            break;
          }
          auto summary = find_type_summary(types, rel.target, &target_err);
          if (!summary.has_value()) continue;
          if (targetR && targetR.value->has_value()
              && summary->type_id.v == targetR.value->value().type.v) {
            target_ok = true;
            break;
          }
        }
        if (!target_ok) note = "target-mismatch";
      }
      std::cout << "  -> " << edge.to.id.to_hex() << " v" << edge.to.ver.v
                << " type=" << target_display << " name=" << edge.name
                << " role=" << edge.role;
      if (!note.empty()) std::cout << " (" << note << ")";
      std::cout << "\n";
    }
  }

  std::cout << "edges to " << recR.value->value().ref.id.to_hex() << "\n";
  if (inR.value->empty()) {
    std::cout << "  (none)\n";
  } else {
    for (const auto& edge : inR.value.value()) {
      std::string source_display = "<unknown>";
      auto sourceR = store.get_latest(edge.from.id);
      if (sourceR && sourceR.value->has_value()) {
        source_display = type_display_name_for(types, sourceR.value->value().type);
      }
      std::cout << "  <- " << edge.from.id.to_hex() << " v" << edge.from.ver.v
                << " type=" << source_display << " name=" << edge.name
                << " role=" << edge.role << "\n";
    }
  }
  return true;
}

bool cmd_call(SchemaRegistry& registry, SqliteStore& store, const ObjectID& id,
              const std::string& op_name, const std::vector<std::string>& args,
              const std::unordered_map<std::string, ObjectID>& session_aliases,
              const std::set<std::string>& granted_caps) {
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
  DispatchEngine engine(registry);
  auto matchR = engine.resolve(def.type_id, op_name, OperationScope::Object, {}, args.size(), true);
  if (!matchR) {
    std::cout << "error: " << matchR.error->message << "\n";
    return false;
  }
  std::string cap_err;
  if (!has_required_capabilities(matchR.value->operation, granted_caps, &cap_err)) {
    std::cout << "error: " << cap_err << "\n";
    return false;
  }

  constexpr referee::TypeID kTypeString{0x1001ULL};
  constexpr referee::TypeID kTypeU64{0x1002ULL};
  constexpr referee::TypeID kTypeBool{0x1003ULL};
  constexpr referee::TypeID kTypeObjectID{0x1004ULL};
  constexpr referee::TypeID kTypeTypeID{0x1005ULL};
  constexpr referee::TypeID kTypeVersion{0x1006ULL};
  constexpr referee::TypeID kTypeBytes{0x1007ULL};
  constexpr referee::TypeID kTypeF64{0x1008ULL};

  struct CoreValue {
    std::string text;
    std::uint64_t u64{0};
    bool b{false};
    double f64{0.0};
  };

  auto read_core_value = [&](referee::TypeID type_id,
                             const nlohmann::json& payload,
                             CoreValue* out,
                             std::string* err_out) -> bool {
    if (!out) return false;
    if (type_id.v == kTypeString.v) {
      return read_string_value(payload, &out->text, err_out);
    }
    if (type_id.v == kTypeU64.v || type_id.v == kTypeVersion.v || type_id.v == kTypeTypeID.v) {
      return read_u64_value(payload, &out->u64, err_out);
    }
    if (type_id.v == kTypeBool.v) {
      return read_bool_value(payload, &out->b, err_out);
    }
    if (type_id.v == kTypeObjectID.v) {
      referee::ObjectID v{};
      if (!read_object_id_value(payload, &v, err_out)) return false;
      out->text = v.to_hex();
      return true;
    }
    if (type_id.v == kTypeF64.v) {
      return read_double_value(payload, &out->f64, err_out);
    }
    if (type_id.v == kTypeBytes.v) {
      return read_bytes_value(payload, &out->text, err_out);
    }
    if (err_out) *err_out = "unsupported core type";
    return false;
  };

  auto resolve_alias_only = [&](const std::string& token,
                                std::string* err_out) -> std::optional<ObjectID> {
    std::string name = token;
    if (!name.empty() && name.front() == '@') name = name.substr(1);
    if (name.empty()) {
      if (err_out) *err_out = "empty alias";
      return std::nullopt;
    }
    auto it = session_aliases.find(name);
    if (it != session_aliases.end()) return it->second;

    auto typesR = registry.list_types();
    if (!typesR) {
      if (err_out) *err_out = typesR.error->message;
      return std::nullopt;
    }
    std::optional<TypeSummary> alias_type;
    for (const auto& summary : typesR.value.value()) {
      if (summary.namespace_name == "Conch" && summary.name == "Alias") {
        alias_type = summary;
        break;
      }
    }
    if (!alias_type.has_value()) {
      if (err_out) *err_out = "alias type not registered";
      return std::nullopt;
    }

    auto listR = store.list_by_type(alias_type->type_id);
    if (!listR) {
      if (err_out) *err_out = listR.error->message;
      return std::nullopt;
    }
    for (const auto& rec : listR.value.value()) {
      try {
        auto json = nlohmann::json::from_cbor(rec.payload_cbor);
        if (json.value("name", "") == name) {
          auto oid_text = json.value("object_id", "");
          if (oid_text.empty()) continue;
          return parse_object_id(oid_text, err_out);
        }
      } catch (const std::exception&) {
        continue;
      }
    }
    if (err_out) *err_out = "alias not found";
    return std::nullopt;
  };

  auto try_core_op = [&]() -> bool {
    if (op_name != "to_string" && op_name != "print" && op_name != "render" && op_name != "compare") {
      return false;
    }

    nlohmann::json payload;
    try {
      payload = nlohmann::json::from_cbor(recR.value->value().payload_cbor);
    } catch (const std::exception& ex) {
      std::cout << "error: payload decode failed: " << ex.what() << "\n";
      return true;
    }

    auto type_id = recR.value->value().type;
    std::string err;
    CoreValue self_value;
    if (!read_core_value(type_id, payload, &self_value, &err)) {
      if (err == "unsupported core type") return false;
      std::cout << "error: " << err << "\n";
      return true;
    }

    if (op_name == "compare") {
      if (args.size() != 1) {
        std::cout << "error: compare expects 1 arg\n";
        return true;
      }
      CoreValue other_value;
      bool resolved_object = false;
      std::string resolve_err;
      auto parsed_id = parse_object_id(args[0], nullptr);
      if (parsed_id.has_value()) {
        resolved_object = true;
      } else if (!args[0].empty() && (args[0].front() == '@'
                                      || session_aliases.find(args[0]) != session_aliases.end())) {
        parsed_id = resolve_alias_only(args[0], &resolve_err);
        if (parsed_id.has_value()) resolved_object = true;
      } else {
        auto alias_id = resolve_alias_only(args[0], nullptr);
        if (alias_id.has_value()) {
          parsed_id = alias_id;
          resolved_object = true;
        }
      }

      if (resolved_object) {
        auto otherR = store.get_latest(parsed_id.value());
        if (!otherR) {
          std::cout << "error: " << otherR.error->message << "\n";
          return true;
        }
        if (!otherR.value->has_value()) {
          std::cout << "error: object not found\n";
          return true;
        }
        const auto& other_rec = otherR.value->value();
        if (other_rec.type.v != type_id.v) {
          std::cout << "error: compare expects matching type\n";
          return true;
        }
        nlohmann::json other_payload;
        try {
          other_payload = nlohmann::json::from_cbor(other_rec.payload_cbor);
        } catch (const std::exception& ex) {
          std::cout << "error: payload decode failed: " << ex.what() << "\n";
          return true;
        }
        if (!read_core_value(type_id, other_payload, &other_value, &err)) {
          std::cout << "error: " << err << "\n";
          return true;
        }
      } else {
        if (type_id.v == kTypeString.v || type_id.v == kTypeObjectID.v || type_id.v == kTypeBytes.v) {
          other_value.text = strip_quotes(args[0]);
        } else if (type_id.v == kTypeBool.v) {
          if (!parse_bool(args[0], &other_value.b)) {
            std::cout << "error: invalid bool arg\n";
            return true;
          }
        } else if (type_id.v == kTypeU64.v || type_id.v == kTypeVersion.v || type_id.v == kTypeTypeID.v) {
          if (!parse_u64(args[0], &other_value.u64)) {
            std::cout << "error: invalid u64 arg\n";
            return true;
          }
        } else if (type_id.v == kTypeF64.v) {
          if (!parse_double(args[0], &other_value.f64)) {
            std::cout << "error: invalid f64 arg\n";
            return true;
          }
        }
      }

      double order = 0.0;
      if (type_id.v == kTypeString.v || type_id.v == kTypeObjectID.v || type_id.v == kTypeBytes.v) {
        if (self_value.text == other_value.text) order = 0.0;
        else if (self_value.text < other_value.text) order = -1.0;
        else order = 1.0;
      } else if (type_id.v == kTypeBool.v) {
        order = (self_value.b == other_value.b) ? 0.0 : (self_value.b ? 1.0 : -1.0);
      } else if (type_id.v == kTypeU64.v || type_id.v == kTypeVersion.v || type_id.v == kTypeTypeID.v) {
        order = (self_value.u64 == other_value.u64) ? 0.0 : (self_value.u64 < other_value.u64 ? -1.0 : 1.0);
      } else if (type_id.v == kTypeF64.v) {
        order = (self_value.f64 == other_value.f64) ? 0.0 : (self_value.f64 < other_value.f64 ? -1.0 : 1.0);
      }
      std::cout << "result " << order << "\n";
      std::cout << "call ok\n";
      return true;
    }

    if (op_name == "print" || op_name == "render") {
      std::cout << self_value.text << "\n";
      std::cout << "call ok\n";
      return true;
    }

    if (op_name == "to_string") {
      if (type_id.v == kTypeU64.v || type_id.v == kTypeVersion.v) {
        std::cout << "result " << self_value.u64 << "\n";
      } else if (type_id.v == kTypeBool.v) {
        std::cout << "result " << (self_value.b ? "true" : "false") << "\n";
      } else if (type_id.v == kTypeTypeID.v) {
        std::cout << "result " << hex_u64(self_value.u64) << "\n";
      } else if (type_id.v == kTypeF64.v) {
        std::cout << "result " << self_value.f64 << "\n";
      } else {
        std::cout << "result " << self_value.text << "\n";
      }
      std::cout << "call ok\n";
      return true;
    }

    return false;
  };

  if (try_core_op()) return true;

  if (def.namespace_name == "Demo" && def.name == "PropulsionSynth" && op_name == "start") {
    auto demoR = demo_start(registry, store, id);
    if (!demoR) {
      std::cout << "error: " << demoR.error->message << "\n";
      return false;
    }
    std::cout << "summary " << demoR.value->to_hex() << "\n";
  }
  if (def.namespace_name == "Demo" && def.name == "Summary" && op_name == "expand") {
    std::uint64_t level = 1;
    if (!args.empty()) {
      std::int64_t parsed = 0;
      if (!parse_int(args[0], &parsed) || parsed <= 0) {
        std::cout << "error: invalid expand level\n";
        return false;
      }
      level = static_cast<std::uint64_t>(parsed);
    }
    auto expandR = demo_expand(registry, store, id, level);
    if (!expandR) {
      std::cout << "error: " << expandR.error->message << "\n";
      return false;
    }
  }
  std::cout << "call ok\n";
  return true;
}

bool handle_call_command(SchemaRegistry& registry,
                         SqliteStore& store,
                         const std::unordered_map<std::string, ObjectID>& session_aliases,
                         const std::set<std::string>& granted_caps,
                         const iris::parser::CallCommand& command) {
  std::string err;
  auto id = parse_object_id_or_alias(command.target, session_aliases, store, registry, &err);
  if (!id.has_value()) {
    std::cout << "error: " << err << "\n";
    return true;
  }
  cmd_call(registry, store, id.value(), command.operation, command.args, session_aliases,
           granted_caps);
  return true;
}

referee::Result<ObjectID> create_object(SchemaRegistry& registry, SqliteStore& store,
                                        const std::string& expr) {
  std::string type_name;
  nlohmann::json payload = nlohmann::json::object();
  auto parseR = parse_new_expr(expr, &type_name, &payload);
  if (!parseR) return referee::Result<ObjectID>::err(parseR.error->message);

  std::string err;
  auto type_summary = resolve_type(registry, type_name, &err);
  if (!type_summary.has_value()) {
    return referee::Result<ObjectID>::err(err);
  }

  auto defR = registry.get_definition_by_id(type_summary->definition_id);
  if (!defR) return referee::Result<ObjectID>::err(defR.error->message);
  if (!defR.value->has_value()) {
    return referee::Result<ObjectID>::err("definition not found");
  }

  auto validateR = validate_payload_constraints(defR.value->value().definition, payload);
  if (!validateR) return referee::Result<ObjectID>::err(validateR.error->message);

  auto cbor = nlohmann::json::to_cbor(payload);
  auto createR = store.create_object(type_summary->type_id, type_summary->definition_id, cbor);
  if (!createR) {
    return referee::Result<ObjectID>::err(createR.error->message);
  }
  return referee::Result<ObjectID>::ok(createR.value->ref.id);
}

referee::Result<void> persist_alias(SqliteStore& store, SchemaRegistry& registry,
                                    const std::string& name, const ObjectID& object_id) {
  auto typesR = registry.list_types();
  if (!typesR) return referee::Result<void>::err(typesR.error->message);
  std::optional<TypeSummary> alias_type;
  for (const auto& summary : typesR.value.value()) {
    if (summary.namespace_name == "Conch" && summary.name == "Alias") {
      alias_type = summary;
      break;
    }
  }
  if (!alias_type.has_value()) return referee::Result<void>::err("alias type not registered");

  nlohmann::json payload;
  payload["name"] = name;
  payload["object_id"] = object_id.to_hex();
  auto cbor = nlohmann::json::to_cbor(payload);
  auto createR = store.create_object(alias_type->type_id, alias_type->definition_id, cbor);
  if (!createR) return referee::Result<void>::err(createR.error->message);
  return referee::Result<void>::ok();
}

void cmd_list_aliases(SqliteStore& store, SchemaRegistry& registry,
                      const std::unordered_map<std::string, ObjectID>& session_aliases,
                      bool persistent) {
  if (!persistent) {
    if (session_aliases.empty()) {
      std::cout << "no aliases\n";
      return;
    }
    for (const auto& it : session_aliases) {
      std::cout << it.first << " = " << it.second.to_hex() << "\n";
    }
    return;
  }

  auto typesR = registry.list_types();
  if (!typesR) {
    std::cout << "error: " << typesR.error->message << "\n";
    return;
  }
  std::optional<TypeSummary> alias_type;
  for (const auto& summary : typesR.value.value()) {
    if (summary.namespace_name == "Conch" && summary.name == "Alias") {
      alias_type = summary;
      break;
    }
  }
  if (!alias_type.has_value()) {
    std::cout << "error: alias type not registered\n";
    return;
  }
  auto listR = store.list_by_type(alias_type->type_id);
  if (!listR) {
    std::cout << "error: " << listR.error->message << "\n";
    return;
  }
  if (listR.value->empty()) {
    std::cout << "no aliases\n";
    return;
  }
  for (const auto& rec : listR.value.value()) {
    try {
      auto json = nlohmann::json::from_cbor(rec.payload_cbor);
      auto name = json.value("name", "");
      auto oid = json.value("object_id", "");
      if (!name.empty() && !oid.empty()) {
        std::cout << name << " = " << oid << "\n";
      }
    } catch (const std::exception&) {
      continue;
    }
  }
}

void cmd_alias_assignment(const std::string& line,
                          const std::string& keyword,
                          bool persistent,
                          SchemaRegistry& registry,
                          SqliteStore& store,
                          std::unordered_map<std::string, ObjectID>& session_aliases) {
  auto rest = trim_copy(line.substr(keyword.size()));
  if (rest == ".") {
    cmd_list_aliases(store, registry, session_aliases, persistent);
    return;
  }
  auto eq = rest.find('=');
  if (eq == std::string::npos) {
    std::cout << "error: expected name=expression\n";
    return;
  }
  auto name = trim_copy(rest.substr(0, eq));
  auto expr = trim_copy(rest.substr(eq + 1));
  if (name.empty() || expr.empty()) {
    std::cout << "error: expected name=expression\n";
    return;
  }

  ObjectID id{};
  if (expr.rfind("new ", 0) == 0) {
    try {
      auto createR = create_object(registry, store, expr);
      if (!createR) {
        std::cout << "error: " << createR.error->message << "\n";
        return;
      }
      id = createR.value.value();
    } catch (const std::exception& ex) {
      std::cout << "error: " << ex.what() << "\n";
      return;
    }
  } else {
    std::string err;
    auto resolved = parse_object_id_or_alias(expr, session_aliases, store, registry, &err);
    if (!resolved.has_value()) {
      std::cout << "error: " << err << "\n";
      return;
    }
    id = resolved.value();
  }

  if (persistent) {
    auto persistR = persist_alias(store, registry, name, id);
    if (!persistR) {
      std::cout << "error: " << persistR.error->message << "\n";
      return;
    }
  } else {
    session_aliases[name] = id;
  }
  std::cout << name << " = " << id.to_hex() << "\n";
}

bool handle_alias_assignment_command(const iris::parser::AliasAssignmentCommand& command,
                                     SchemaRegistry& registry,
                                     SqliteStore& store,
                                     std::unordered_map<std::string, ObjectID>& session_aliases) {
  if (command.list_aliases) {
    cmd_list_aliases(store, registry, session_aliases, command.persistent);
    return true;
  }

  ObjectID id{};
  if (command.expression.rfind("new ", 0) == 0) {
    try {
      auto createR = create_object(registry, store, command.expression);
      if (!createR) {
        std::cout << "error: " << createR.error->message << "\n";
        return true;
      }
      id = createR.value.value();
    } catch (const std::exception& ex) {
      std::cout << "error: " << ex.what() << "\n";
      return true;
    }
  } else {
    std::string err;
    auto resolved = parse_object_id_or_alias(command.expression, session_aliases, store,
                                             registry, &err);
    if (!resolved.has_value()) {
      std::cout << "error: " << err << "\n";
      return true;
    }
    id = resolved.value();
  }

  if (command.persistent) {
    auto persistR = persist_alias(store, registry, command.name, id);
    if (!persistR) {
      std::cout << "error: " << persistR.error->message << "\n";
      return true;
    }
  } else {
    session_aliases[command.name] = id;
  }
  std::cout << command.name << " = " << id.to_hex() << "\n";
  return true;
}

struct EmitFlags {
  std::vector<std::string> positional;
  std::optional<std::string> produced_by;
  std::optional<std::string> progress_of;
  std::optional<std::string> diagnostic_of;
  std::string role{"artifact"};
};

bool parse_emit_flags(const std::vector<std::string>& tokens, size_t start,
                      EmitFlags* out, std::string* err_out) {
  EmitFlags flags;
  for (size_t i = start; i < tokens.size(); ++i) {
    const auto& tok = tokens[i];
    if (tok == "--produced-by" || tok == "--progress-of" || tok == "--diagnostic-of") {
      if (i + 1 >= tokens.size()) {
        if (err_out) *err_out = "missing id after " + tok;
        return false;
      }
      const auto& value = tokens[++i];
      if (tok == "--produced-by") flags.produced_by = value;
      if (tok == "--progress-of") flags.progress_of = value;
      if (tok == "--diagnostic-of") flags.diagnostic_of = value;
      continue;
    }
    if (tok == "--role") {
      if (i + 1 >= tokens.size()) {
        if (err_out) *err_out = "missing role after --role";
        return false;
      }
      flags.role = tokens[++i];
      continue;
    }
    if (!tok.empty() && tok[0] == '-') {
      if (err_out) *err_out = "unknown option: " + tok;
      return false;
    }
    flags.positional.push_back(tok);
  }
  if (out) *out = std::move(flags);
  return true;
}

bool add_edge_named(SqliteStore& store, const ObjectID& from_id, const ObjectID& to_id,
                    const std::string& name, const std::string& role,
                    std::string* err_out) {
  auto from_ref = latest_ref(store, from_id, err_out);
  if (!from_ref.has_value()) return false;
  auto to_ref = latest_ref(store, to_id, err_out);
  if (!to_ref.has_value()) return false;
  referee::Bytes props;
  auto edgeR = store.add_edge(from_ref.value(), to_ref.value(), name, role, props);
  if (!edgeR) {
    if (err_out) *err_out = edgeR.error->message;
    return false;
  }
  return true;
}

void print_route_for(iris::refract::SchemaRegistry& registry, referee::TypeID type_id) {
  auto routeR = iris::vizier::route_for_type_id(registry, type_id);
  if (!routeR.has_value()) {
    std::cout << "route: none\n";
    return;
  }
  std::cout << "route: " << routeR->concho << "\n";
}

void maybe_spawn_concho(iris::refract::SchemaRegistry& registry,
                        referee::SqliteStore& store,
                        const referee::ObjectID& artifact_id) {
  auto conchoR = iris::vizier::spawn_concho_for_artifact(registry, store, artifact_id);
  if (!conchoR) {
    std::cout << "error: " << conchoR.error->message << "\n";
    return;
  }
  if (!conchoR.value->has_value()) {
    std::cout << "concho: none\n";
    return;
  }
  std::cout << "concho: " << conchoR.value->value().to_hex() << "\n";
}

referee::Result<void> add_edge_or_err(SqliteStore& store, const ObjectID& from_id,
                                      const ObjectID& to_id, const std::string& name,
                                      const std::string& role) {
  std::string err;
  if (!add_edge_named(store, from_id, to_id, name, role, &err)) {
    return referee::Result<void>::err(err);
  }
  return referee::Result<void>::ok();
}

referee::Result<ObjectID> create_demo_object(SchemaRegistry& registry, SqliteStore& store,
                                             const std::string& type_name,
                                             const nlohmann::json& payload) {
  std::string err;
  auto type_summary = resolve_type(registry, type_name, &err);
  if (!type_summary.has_value()) {
    return referee::Result<ObjectID>::err(err);
  }
  auto cbor = nlohmann::json::to_cbor(payload);
  auto createR = store.create_object(type_summary->type_id, type_summary->definition_id, cbor);
  if (!createR) {
    return referee::Result<ObjectID>::err(createR.error->message);
  }
  return referee::Result<ObjectID>::ok(createR.value->ref.id);
}

referee::Result<void> emit_demo_artifacts(SchemaRegistry& registry, SqliteStore& store,
                                          const ObjectID& owner_id,
                                          const std::string& prefix,
                                          double metric_value) {
  iris::viz::TextLog log;
  log.lines = {prefix + " online", prefix + " nominal"};
  auto logR = iris::viz::create_text_log(registry, store, log);
  if (!logR) return referee::Result<void>::err(logR.error->message);

  iris::viz::Metric metric;
  metric.name = "thrust";
  metric.value = metric_value;
  auto metricR = iris::viz::create_metric(registry, store, metric);
  if (!metricR) return referee::Result<void>::err(metricR.error->message);

  iris::viz::Table table;
  table.columns = {"module", "status"};
  table.rows = {{"ionizer", "ok"}, {"coolant", "nominal"}};
  auto tableR = iris::viz::create_table(registry, store, table);
  if (!tableR) return referee::Result<void>::err(tableR.error->message);

  auto linkR = add_edge_or_err(store, owner_id, logR.value.value(), "produced", "artifact");
  if (!linkR) return linkR;
  maybe_spawn_concho(registry, store, logR.value.value());

  linkR = add_edge_or_err(store, owner_id, metricR.value.value(), "produced", "artifact");
  if (!linkR) return linkR;
  maybe_spawn_concho(registry, store, metricR.value.value());

  linkR = add_edge_or_err(store, owner_id, tableR.value.value(), "produced", "artifact");
  if (!linkR) return linkR;
  maybe_spawn_concho(registry, store, tableR.value.value());

  return referee::Result<void>::ok();
}

referee::Result<ObjectID> demo_start(SchemaRegistry& registry, SqliteStore& store,
                                     const ObjectID& demo_id) {
  auto artifactsR = emit_demo_artifacts(registry, store, demo_id, "PropulsionSynth", 0.78);
  if (!artifactsR) return referee::Result<ObjectID>::err(artifactsR.error->message);

  nlohmann::json payload;
  payload["title"] = "PropulsionSynth Summary";
  payload["level"] = 0;
  auto summaryR = create_demo_object(registry, store, "Demo::Summary", payload);
  if (!summaryR) return summaryR;

  auto linkR = add_edge_or_err(store, demo_id, summaryR.value.value(), "summary", "root");
  if (!linkR) return referee::Result<ObjectID>::err(linkR.error->message);

  return summaryR;
}

referee::Result<void> demo_expand(SchemaRegistry& registry, SqliteStore& store,
                                  const ObjectID& summary_id, std::uint64_t level) {
  std::size_t count = static_cast<std::size_t>(level) + 1;
  for (std::size_t i = 0; i < count; ++i) {
    nlohmann::json payload;
    std::ostringstream title;
    title << "Detail L" << level << "-" << (i + 1);
    payload["title"] = title.str();
    payload["level"] = level;
    payload["index"] = static_cast<std::uint64_t>(i + 1);
    auto detailR = create_demo_object(registry, store, "Demo::Detail", payload);
    if (!detailR) return referee::Result<void>::err(detailR.error->message);

    auto linkR = add_edge_or_err(store, summary_id, detailR.value.value(), "summarizes", "detail");
    if (!linkR) return linkR;

    iris::viz::TextLog log;
    log.lines = {title.str(), "thrusters calibrated"};
    auto logR = iris::viz::create_text_log(registry, store, log);
    if (!logR) return referee::Result<void>::err(logR.error->message);

    iris::viz::Metric metric;
    metric.name = "thrust";
    metric.value = 0.6 + (0.05 * static_cast<double>(i));
    auto metricR = iris::viz::create_metric(registry, store, metric);
    if (!metricR) return referee::Result<void>::err(metricR.error->message);

    linkR = add_edge_or_err(store, detailR.value.value(), logR.value.value(), "produced", "artifact");
    if (!linkR) return linkR;
    maybe_spawn_concho(registry, store, logR.value.value());

    linkR = add_edge_or_err(store, detailR.value.value(), metricR.value.value(), "produced", "artifact");
    if (!linkR) return linkR;
    maybe_spawn_concho(registry, store, metricR.value.value());
  }
  return referee::Result<void>::ok();
}

void cmd_emit_viz(SchemaRegistry& registry, SqliteStore& store,
                  const std::unordered_map<std::string, ObjectID>& session_aliases,
                  const std::vector<std::string>& tokens) {
  if (tokens.size() < 3 || tokens[1] != "viz") {
    std::cout << "usage: emit viz <textlog|metric|table|tree|panel> [args...]\n";
    return;
  }

  const auto& kind = tokens[2];
  EmitFlags flags;
  std::string err;
  if (!parse_emit_flags(tokens, 3, &flags, &err)) {
    std::cout << "error: " << err << "\n";
    return;
  }
  if (kind == "textlog" || kind == "log") {
    iris::viz::TextLog log;
    if (!flags.positional.empty()) {
      for (const auto& line : flags.positional) log.lines.push_back(line);
    } else {
      log.lines = {"hello", "world"};
    }
    auto idR = iris::viz::create_text_log(registry, store, log);
    if (!idR) {
      std::cout << "error: " << idR.error->message << "\n";
      return;
    }
    auto id = idR.value.value();
    std::cout << "created Viz::TextLog " << id.to_hex() << "\n";
    print_route_for(registry, iris::viz::kTypeVizTextLog);
    maybe_spawn_concho(registry, store, id);
    if (flags.produced_by.has_value()) {
      auto from_id = parse_object_id_or_alias(flags.produced_by.value(), session_aliases,
                                              store, registry, &err);
      if (!from_id.has_value()
          || !add_edge_named(store, from_id.value(), id, "produced", flags.role, &err)) {
        std::cout << "error: " << err << "\n";
      }
    }
    if (flags.progress_of.has_value()) {
      auto from_id = parse_object_id_or_alias(flags.progress_of.value(), session_aliases,
                                              store, registry, &err);
      if (!from_id.has_value()
          || !add_edge_named(store, from_id.value(), id, "progress", flags.role, &err)) {
        std::cout << "error: " << err << "\n";
      }
    }
    if (flags.diagnostic_of.has_value()) {
      auto from_id = parse_object_id_or_alias(flags.diagnostic_of.value(), session_aliases,
                                              store, registry, &err);
      if (!from_id.has_value()
          || !add_edge_named(store, from_id.value(), id, "diagnostic", flags.role, &err)) {
        std::cout << "error: " << err << "\n";
      }
    }
    return;
  }
  if (kind == "metric") {
    iris::viz::Metric metric;
    metric.name = "load";
    metric.value = 0.42;
    if (flags.positional.size() >= 1) metric.name = flags.positional[0];
    if (flags.positional.size() >= 2) {
      double parsed = 0.0;
      if (!parse_double(flags.positional[1], &parsed)) {
        std::cout << "error: invalid metric value\n";
        return;
      }
      metric.value = parsed;
    }
    auto idR = iris::viz::create_metric(registry, store, metric);
    if (!idR) {
      std::cout << "error: " << idR.error->message << "\n";
      return;
    }
    auto id = idR.value.value();
    std::cout << "created Viz::Metric " << id.to_hex() << "\n";
    print_route_for(registry, iris::viz::kTypeVizMetric);
    maybe_spawn_concho(registry, store, id);
    if (flags.produced_by.has_value()) {
      auto from_id = parse_object_id_or_alias(flags.produced_by.value(), session_aliases,
                                              store, registry, &err);
      if (!from_id.has_value()
          || !add_edge_named(store, from_id.value(), id, "produced", flags.role, &err)) {
        std::cout << "error: " << err << "\n";
      }
    }
    if (flags.progress_of.has_value()) {
      auto from_id = parse_object_id_or_alias(flags.progress_of.value(), session_aliases,
                                              store, registry, &err);
      if (!from_id.has_value()
          || !add_edge_named(store, from_id.value(), id, "progress", flags.role, &err)) {
        std::cout << "error: " << err << "\n";
      }
    }
    if (flags.diagnostic_of.has_value()) {
      auto from_id = parse_object_id_or_alias(flags.diagnostic_of.value(), session_aliases,
                                              store, registry, &err);
      if (!from_id.has_value()
          || !add_edge_named(store, from_id.value(), id, "diagnostic", flags.role, &err)) {
        std::cout << "error: " << err << "\n";
      }
    }
    return;
  }
  if (kind == "table") {
    iris::viz::Table table;
    table.columns = {"name", "value"};
    table.rows = {{"alpha", "1"}, {"beta", "2"}};
    auto idR = iris::viz::create_table(registry, store, table);
    if (!idR) {
      std::cout << "error: " << idR.error->message << "\n";
      return;
    }
    auto id = idR.value.value();
    std::cout << "created Viz::Table " << id.to_hex() << "\n";
    print_route_for(registry, iris::viz::kTypeVizTable);
    maybe_spawn_concho(registry, store, id);
    if (flags.produced_by.has_value()) {
      auto from_id = parse_object_id_or_alias(flags.produced_by.value(), session_aliases,
                                              store, registry, &err);
      if (!from_id.has_value()
          || !add_edge_named(store, from_id.value(), id, "produced", flags.role, &err)) {
        std::cout << "error: " << err << "\n";
      }
    }
    if (flags.progress_of.has_value()) {
      auto from_id = parse_object_id_or_alias(flags.progress_of.value(), session_aliases,
                                              store, registry, &err);
      if (!from_id.has_value()
          || !add_edge_named(store, from_id.value(), id, "progress", flags.role, &err)) {
        std::cout << "error: " << err << "\n";
      }
    }
    if (flags.diagnostic_of.has_value()) {
      auto from_id = parse_object_id_or_alias(flags.diagnostic_of.value(), session_aliases,
                                              store, registry, &err);
      if (!from_id.has_value()
          || !add_edge_named(store, from_id.value(), id, "diagnostic", flags.role, &err)) {
        std::cout << "error: " << err << "\n";
      }
    }
    return;
  }
  if (kind == "tree") {
    iris::viz::Tree tree;
    tree.label = flags.positional.empty() ? "root" : flags.positional[0];
    auto idR = iris::viz::create_tree(registry, store, tree);
    if (!idR) {
      std::cout << "error: " << idR.error->message << "\n";
      return;
    }
    auto id = idR.value.value();
    std::cout << "created Viz::Tree " << id.to_hex() << "\n";
    print_route_for(registry, iris::viz::kTypeVizTree);
    maybe_spawn_concho(registry, store, id);
    if (flags.produced_by.has_value()) {
      auto from_id = parse_object_id_or_alias(flags.produced_by.value(), session_aliases,
                                              store, registry, &err);
      if (!from_id.has_value()
          || !add_edge_named(store, from_id.value(), id, "produced", flags.role, &err)) {
        std::cout << "error: " << err << "\n";
      }
    }
    if (flags.progress_of.has_value()) {
      auto from_id = parse_object_id_or_alias(flags.progress_of.value(), session_aliases,
                                              store, registry, &err);
      if (!from_id.has_value()
          || !add_edge_named(store, from_id.value(), id, "progress", flags.role, &err)) {
        std::cout << "error: " << err << "\n";
      }
    }
    if (flags.diagnostic_of.has_value()) {
      auto from_id = parse_object_id_or_alias(flags.diagnostic_of.value(), session_aliases,
                                              store, registry, &err);
      if (!from_id.has_value()
          || !add_edge_named(store, from_id.value(), id, "diagnostic", flags.role, &err)) {
        std::cout << "error: " << err << "\n";
      }
    }
    return;
  }
  if (kind == "panel") {
    iris::viz::Panel panel;
    panel.title = flags.positional.empty() ? "Panel" : flags.positional[0];
    auto idR = iris::viz::create_panel(registry, store, panel);
    if (!idR) {
      std::cout << "error: " << idR.error->message << "\n";
      return;
    }
    auto id = idR.value.value();
    std::cout << "created Viz::Panel " << id.to_hex() << "\n";
    print_route_for(registry, iris::viz::kTypeVizPanel);
    maybe_spawn_concho(registry, store, id);
    if (flags.produced_by.has_value()) {
      auto from_id = parse_object_id_or_alias(flags.produced_by.value(), session_aliases,
                                              store, registry, &err);
      if (!from_id.has_value()
          || !add_edge_named(store, from_id.value(), id, "produced", flags.role, &err)) {
        std::cout << "error: " << err << "\n";
      }
    }
    if (flags.progress_of.has_value()) {
      auto from_id = parse_object_id_or_alias(flags.progress_of.value(), session_aliases,
                                              store, registry, &err);
      if (!from_id.has_value()
          || !add_edge_named(store, from_id.value(), id, "progress", flags.role, &err)) {
        std::cout << "error: " << err << "\n";
      }
    }
    if (flags.diagnostic_of.has_value()) {
      auto from_id = parse_object_id_or_alias(flags.diagnostic_of.value(), session_aliases,
                                              store, registry, &err);
      if (!from_id.has_value()
          || !add_edge_named(store, from_id.value(), id, "diagnostic", flags.role, &err)) {
        std::cout << "error: " << err << "\n";
      }
    }
    return;
  }

  std::cout << "error: unknown viz artifact\n";
}

void cmd_demo_v1(SchemaRegistry& registry, SqliteStore& store,
                 iris::ceo::TaskRegistry& ceo_registry,
                 iris::ceo::TaskComms& ceo_comms,
                 std::unordered_map<std::string, ObjectID>& session_aliases) {
  nlohmann::json payload;
  payload["name"] = "PropulsionSynth";
  auto demoR = create_demo_object(registry, store, "Demo::PropulsionSynth", payload);
  if (!demoR) {
    std::cout << "error: " << demoR.error->message << "\n";
    return;
  }
  auto demo_id = demoR.value.value();
  session_aliases["demo"] = demo_id;
  std::cout << "demo " << demo_id.to_hex() << "\n";

  auto demo_task = ceo_registry.create_task(demo_id, std::nullopt, "demo", iris::ceo::TaskMode::Service);
  if (!demo_task) {
    std::cout << "error: " << demo_task.error->message << "\n";
    return;
  }
  auto start_demo = ceo_registry.start_task(demo_task.value->id);
  if (!start_demo) {
    std::cout << "error: " << start_demo.error->message << "\n";
    return;
  }

  auto summaryR = demo_start(registry, store, demo_id);
  if (!summaryR) {
    std::cout << "error: " << summaryR.error->message << "\n";
    return;
  }
  auto summary_id = summaryR.value.value();
  session_aliases["summary"] = summary_id;
  std::cout << "summary " << summary_id.to_hex() << "\n";

  auto summary_task = ceo_registry.create_task(summary_id, demo_task.value->id,
                                               "summary", iris::ceo::TaskMode::Inline);
  if (!summary_task) {
    std::cout << "error: " << summary_task.error->message << "\n";
    return;
  }
  auto start_summary = ceo_registry.start_task(summary_task.value->id);
  if (!start_summary) {
    std::cout << "error: " << start_summary.error->message << "\n";
    return;
  }

  auto channelR = ceo_comms.open_channel(demo_task.value->id, summary_task.value->id);
  if (!channelR) {
    std::cout << "error: " << channelR.error->message << "\n";
    return;
  }
  auto& demo_to_summary = channelR.value->first;
  auto& summary_to_demo = channelR.value->second;

  iris::comms::Bytes boot = {0x56, 0x31};
  auto sendR = demo_to_summary.send(boot);
  if (!sendR.ready) {
    std::cout << "error: demo comms send failed\n";
    return;
  }
  auto waitR = summary_to_demo.wait_readable(summary_task.value->id);
  if (!waitR.ready) {
    std::cout << "error: summary comms wait failed\n";
    return;
  }
  auto recv = summary_to_demo.recv(8);
  if (recv.empty()) {
    std::cout << "error: summary comms recv failed\n";
    return;
  }

  ceo_comms.close_channel(demo_to_summary);

  auto expandR = demo_expand(registry, store, summary_id, 2);
  if (!expandR) {
    std::cout << "error: " << expandR.error->message << "\n";
    return;
  }
  std::cout << "expanded summary level 2\n";
  cmd_edges(store, demo_id);

  auto stop_summary = ceo_registry.stop_task(summary_task.value->id);
  if (!stop_summary) {
    std::cout << "error: " << stop_summary.error->message << "\n";
    return;
  }
  auto stop_demo = ceo_registry.stop_task(demo_task.value->id);
  if (!stop_demo) {
    std::cout << "error: " << stop_demo.error->message << "\n";
    return;
  }
}

void cmd_edge(SqliteStore& store, SchemaRegistry& registry,
              const std::unordered_map<std::string, ObjectID>& session_aliases,
              const std::vector<std::string>& tokens) {
  if (tokens.size() < 4) {
    std::cout << "usage: edge <fromObjectID> <toObjectID> <name> [role]\n";
    return;
  }
  std::string err;
  auto from_id = parse_object_id_or_alias(tokens[1], session_aliases, store, registry, &err);
  if (!from_id.has_value()) {
    std::cout << "error: " << err << "\n";
    return;
  }
  auto to_id = parse_object_id_or_alias(tokens[2], session_aliases, store, registry, &err);
  if (!to_id.has_value()) {
    std::cout << "error: " << err << "\n";
    return;
  }
  const std::string& name = tokens[3];
  std::string role = tokens.size() >= 5 ? tokens[4] : "artifact";
  if (!add_edge_named(store, from_id.value(), to_id.value(), name, role, &err)) {
    std::cout << "error: " << err << "\n";
    return;
  }
  std::cout << "edge added\n";
  if (name == "produced" || name == "progress" || name == "diagnostic") {
    maybe_spawn_concho(registry, store, to_id.value());
  }
}

void cmd_route_type(SchemaRegistry& registry, const std::string& type_name) {
  std::string err;
  auto summary = resolve_type(registry, type_name, &err);
  if (!summary.has_value()) {
    std::cout << "error: " << err << "\n";
    return;
  }
  auto route = iris::vizier::route_for_type(summary.value());
  if (!route.has_value()) {
    std::cout << "route: none\n";
    return;
  }
  std::cout << "route: " << route->concho << "\n";
}

void cmd_route_object(SchemaRegistry& registry, SqliteStore& store,
                      const std::unordered_map<std::string, ObjectID>& session_aliases,
                      const std::string& token) {
  std::string err;
  auto id = parse_object_id_or_alias(token, session_aliases, store, registry, &err);
  if (!id.has_value()) {
    std::cout << "error: " << err << "\n";
    return;
  }
  auto recR = store.get_latest(id.value());
  if (!recR) {
    std::cout << "error: " << recR.error->message << "\n";
    return;
  }
  if (!recR.value->has_value()) {
    std::cout << "error: object not found\n";
    return;
  }
  auto route = iris::vizier::route_for_type_id(registry, recR.value->value().type);
  if (!route.has_value()) {
    std::cout << "route: none\n";
    return;
  }
  std::cout << "route: " << route->concho << "\n";
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
  auto bootstrapR = iris::refract::bootstrap_core_schema(registry);
  if (!bootstrapR) {
    std::cout << "error: bootstrap failed: " << bootstrapR.error->message << "\n";
    return 1;
  }
  auto catalogR = iris::refract::bootstrap_core_catalog(registry, store);
  if (!catalogR) {
    std::cout << "error: bootstrap catalog failed: " << catalogR.error->message << "\n";
    return 1;
  }
#if !defined(HAVE_READLINE)
  if (::isatty(STDIN_FILENO)) {
    std::cout << "note: readline not available; history disabled\n";
  }
#endif
  std::vector<TaskEntry> tasks;
  iris::ceo::TaskRegistry ceo_registry;
  iris::ceo::TaskComms ceo_comms(ceo_registry);
  iris::ceo::IoReactor ceo_reactor(ceo_registry);
  iris::conduit::IoHandleStore io_handle_store;
  iris::conduit::IoExecutor io_executor(ceo_registry, ceo_comms, ceo_reactor, io_handle_store);
  std::unordered_map<std::string, iris::conduit::IoHandle> io_handles;
  std::unordered_map<std::string, iris::conduit::IoHandle> io_handle_aliases;
  std::uint64_t next_io_handle_id = 1;
  std::unordered_map<std::string, ObjectID> session_aliases;
  std::set<std::string> session_caps;
  std::string current_namespace;
  std::uint64_t next_task_id = 1;

  load_io_aliases(store, registry, io_handle_aliases);

  for (;;) {
    auto prompt = make_prompt(current_namespace);
    auto line_opt = read_line(prompt.c_str());
    if (!line_opt.has_value()) break;
    const auto& line = line_opt.value();
    if (line.empty()) {
      if (::isatty(STDIN_FILENO)) {
        std::cout << "\n";
      }
      continue;
    }
    auto parsed = iris::parser::parse_conch_command(line);
    if (!parsed.errors.empty()) {
      for (const auto& err : parsed.errors) {
        std::cout << "parse error: " << err.message
                  << " at " << err.line << ":" << err.column << "\n";
      }
      continue;
    }
    if (parsed.name.empty()) continue;

    auto session_op = resolve_session_operation(parsed);

    const auto& cmd = parsed.name;
    if (auto alias_command = parsed.get_if<iris::parser::AliasAssignmentCommand>()) {
      handle_alias_assignment_command(*alias_command, registry, store, session_aliases);
      continue;
    }
    if (cmd == "let") {
      cmd_alias_assignment(line, "let", false, registry, store, session_aliases);
      continue;
    }
    if (cmd == "var") {
      cmd_alias_assignment(line, "var", true, registry, store, session_aliases);
      continue;
    }
    if (cmd == "alias") {
      cmd_alias_assignment(line, "alias", false, registry, store, session_aliases);
      continue;
    }
    if (cmd == "exit" || cmd == "quit") break;
    if (cmd == "help") {
      print_help();
      continue;
    }
    if (auto list_command = parsed.get_if<iris::parser::TypesListCommand>()) {
      handle_types_list_command(registry, store, *list_command, current_namespace);
      continue;
    }
    if (auto namespace_command = parsed.get_if<iris::parser::NamespaceCommand>()) {
      handle_namespace_family_command(registry, current_namespace, *namespace_command);
      continue;
    }
    if (auto schema_command = parsed.get_if<iris::parser::SchemaCommand>()) {
      handle_schema_command(registry, store, *schema_command);
      continue;
    }
    if (auto object_command = parsed.get_if<iris::parser::ObjectCommand>()) {
      handle_object_command(registry, store, session_aliases, *object_command);
      continue;
    }
    if (auto call_command = parsed.get_if<iris::parser::CallCommand>()) {
      handle_call_command(registry, store, session_aliases, session_caps, *call_command);
      continue;
    }
    if (auto task_command = parsed.get_if<iris::parser::TaskCommand>()) {
      handle_task_family_command(ceo_registry, registry, store, session_aliases,
                                 tasks, next_task_id, *task_command, session_caps);
      continue;
    }
    if (auto io_command = parsed.get_if<iris::parser::IoCommand>()) {
      handle_io_family_command(io_executor, io_handle_store, io_handles, io_handle_aliases,
                               next_io_handle_id, registry, store, session_caps, *io_command);
      continue;
    }
    if (auto caps_command = parsed.get_if<iris::parser::CapsCommand>()) {
      handle_caps_family_command(session_caps, *caps_command);
      continue;
    }
    if (session_op.has_value()) {
      if (handle_session_operation(line, parsed, session_op.value(), registry, store,
                                   current_namespace, session_aliases, session_caps, tasks,
                                   next_task_id,
                                   ceo_registry, io_executor, io_handle_store,
                                   io_handles, io_handle_aliases, next_io_handle_id, ceo_comms)) {
        continue;
      }
    }
    if (cmd == "ls") {
      handle_types_list(registry, store, parsed.args, current_namespace);
      continue;
    }
    if (cmd == "objects") {
      cmd_objects(registry, store);
      continue;
    }
    std::cout << "error: unknown command\n";
  }

  (void)store.close();
  return 0;
}
