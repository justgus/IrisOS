#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "refract/bootstrap.h"
#include "refract/schema_registry.h"
#include "referee/referee.h"
#include "referee_sqlite/sqlite_store.h"
#include "viz/artifacts.h"
#include "vizier/routing.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#if defined(HAVE_READLINE)
#include <readline/history.h>
#include <readline/readline.h>
#endif

#include <unistd.h>

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

struct AliasEntry {
  std::string name;
  ObjectID object_id{};
};

referee::Result<ObjectID> create_object(SchemaRegistry& registry, SqliteStore& store,
                                        const std::string& expr);
bool parse_bool(std::string_view v, bool* out);
bool parse_int(std::string_view v, std::int64_t* out);
bool parse_double(std::string_view v, double* out);

std::vector<std::string> split_ws(const std::string& line) {
  std::istringstream iss(line);
  std::vector<std::string> out;
  std::string tok;
  while (iss >> tok) out.push_back(tok);
  return out;
}

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
    auto j = nlohmann::json::parse(json_text);
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

std::string type_display_name(const TypeSummary& summary) {
  if (summary.namespace_name.empty()) return summary.name;
  return summary.namespace_name + "::" + summary.name;
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

void print_help() {
  std::cout << "Commands:\n";
  std::cout << "  ls\n";
  std::cout << "  ls --namespaces [pattern]\n";
  std::cout << "  ls [pattern]\n";
  std::cout << "  ls --regex <pattern>\n";
  std::cout << "  ls --regex --namespaces <pattern>\n";
  std::cout << "  objects\n";
  std::cout << "  let <name>=<expr>\n";
  std::cout << "  let .\n";
  std::cout << "  var <name>=<expr>\n";
  std::cout << "  var .\n";
  std::cout << "  alias <name>=<expr>\n";
  std::cout << "  show <ObjectID>\n";
  std::cout << "  show type <TypeName>\n";
  std::cout << "  edges <ObjectID>\n";
  std::cout << "  find type <TypeName>\n";
  std::cout << "  define type <TypeName> fields <field>:<type>[?],...\n";
  std::cout << "  define type --json <spec>\n";
  std::cout << "  new <TypeName> field:=value ...\n";
  std::cout << "  new --json <spec>\n";
  std::cout << "  call <ObjectID> <opName> [args...]\n";
  std::cout << "  start <ObjectID>\n";
  std::cout << "  ps\n";
  std::cout << "  kill <TaskID>\n";
  std::cout << "  edge <fromObjectID> <toObjectID> <name> [role]\n";
  std::cout << "  emit viz <textlog|metric|table|tree|panel> [args...]\n";
  std::cout << "    [--produced-by <id>] [--progress-of <id>] [--diagnostic-of <id>] [--role <role>]\n";
  std::cout << "  route <ObjectID>\n";
  std::cout << "  route type <TypeName>\n";
  std::cout << "  note: ObjectID prefixes (>=4 hex chars) resolve when unambiguous\n";
  std::cout << "  help\n";
  std::cout << "  exit\n";
}

void cmd_ls(SchemaRegistry& registry, SqliteStore& store,
            const std::optional<std::string>& filter,
            bool regex_mode,
            bool namespaces_only) {
  auto typesR = registry.list_types();
  if (!typesR) {
    std::cout << "error: " << typesR.error->message << "\n";
    return;
  }
  if (typesR.value->empty()) {
    std::cout << "no types registered\n";
    return;
  }

  if (namespaces_only) {
    std::set<std::string> namespaces;
    for (const auto& summary : typesR.value.value()) {
      if (!summary.namespace_name.empty()) namespaces.insert(summary.namespace_name);
    }
    if (namespaces.empty()) {
      std::cout << "no namespaces\n";
      return;
    }
    for (const auto& ns : namespaces) {
      if (filter.has_value()) {
        std::string err;
        if (!match_pattern(ns, *filter, regex_mode, &err)) {
          if (!err.empty()) {
            std::cout << "error: " << err << "\n";
            return;
          }
          continue;
        }
      }
      std::cout << ns << "\n";
    }
    return;
  }

  for (const auto& summary : typesR.value.value()) {
    auto display = type_display_name(summary);
    if (filter.has_value()) {
      std::string err;
      if (!match_pattern(display, *filter, regex_mode, &err)) {
        if (!err.empty()) {
          std::cout << "error: " << err << "\n";
          return;
        }
        continue;
      }
    }
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
  iris::refract::TypeDefinition def;
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
    iris::refract::TypeDefinition def;
    def.name = j.value("name", "");
    def.namespace_name = j.value("namespace", "");
    def.version = j.value("version", 1ULL);
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
        def.fields.push_back(std::move(field));
      }
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
  iris::refract::TypeDefinition def;
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

void cmd_show_type(SchemaRegistry& registry, const std::string& name) {
  std::string err;
  auto match = resolve_type(registry, name, &err);
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
#if !defined(HAVE_READLINE)
  if (::isatty(STDIN_FILENO)) {
    std::cout << "note: readline not available; history disabled\n";
  }
#endif
  std::vector<TaskEntry> tasks;
  std::unordered_map<std::string, ObjectID> session_aliases;
  std::uint64_t next_task_id = 1;

  for (;;) {
    auto line_opt = read_line("conch> ");
    if (!line_opt.has_value()) break;
    const auto& line = line_opt.value();
    if (line.empty()) {
      if (::isatty(STDIN_FILENO)) {
        std::cout << "\n";
      }
      continue;
    }
    auto tokens = split_ws(line);
    if (tokens.empty()) continue;

    const auto& cmd = tokens[0];
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
    if (cmd == "ls") {
      bool regex_mode = false;
      bool namespaces_only = false;
      std::optional<std::string> filter;

      bool bad_args = false;
      for (size_t i = 1; i < tokens.size(); ++i) {
        if (tokens[i] == "--regex") {
          regex_mode = true;
          continue;
        }
        if (tokens[i] == "--namespaces") {
          namespaces_only = true;
          continue;
        }
        if (!filter.has_value()) {
          filter = tokens[i];
        } else {
          std::cout << "error: unexpected argument\n";
          bad_args = true;
          break;
        }
      }
      if (bad_args) continue;
      if (tokens.size() > 1 && !filter.has_value() && regex_mode) {
        std::cout << "error: --regex requires a pattern\n";
        continue;
      }
      cmd_ls(registry, store, filter, regex_mode, namespaces_only);
      continue;
    }
    if (cmd == "objects") {
      cmd_objects(registry, store);
      continue;
    }
    if (cmd == "define" && tokens.size() >= 3 && tokens[1] == "type") {
      cmd_define_type(registry, tokens);
      continue;
    }
    if (cmd == "new") {
      cmd_new_object(registry, store, line);
      continue;
    }
    if (cmd == "find" && tokens.size() >= 3 && tokens[1] == "type") {
      cmd_find_type(registry, tokens[2]);
      continue;
    }
    if (cmd == "show" && tokens.size() == 3 && tokens[1] == "type") {
      cmd_show_type(registry, tokens[2]);
      continue;
    }
    if (cmd == "show" && tokens.size() == 2) {
      std::string err;
      auto id = parse_object_id_or_alias(tokens[1], session_aliases, store, registry, &err);
      if (!id.has_value()) {
        std::cout << "error: " << err << "\n";
        continue;
      }
      cmd_show(registry, store, id.value());
      continue;
    }
    if (cmd == "edges" && tokens.size() == 2) {
      std::string err;
      auto id = parse_object_id_or_alias(tokens[1], session_aliases, store, registry, &err);
      if (!id.has_value()) {
        std::cout << "error: " << err << "\n";
        continue;
      }
      cmd_edges(store, id.value());
      continue;
    }
    if (cmd == "call" && tokens.size() >= 3) {
      std::string err;
      auto id = parse_object_id_or_alias(tokens[1], session_aliases, store, registry, &err);
      if (!id.has_value()) {
        std::cout << "error: " << err << "\n";
        continue;
      }
      cmd_call(registry, store, id.value(), tokens[2], tokens.size() - 3);
      continue;
    }
    if (cmd == "start" && tokens.size() == 2) {
      std::string err;
      auto id = parse_object_id_or_alias(tokens[1], session_aliases, store, registry, &err);
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
    if (cmd == "edge") {
      cmd_edge(store, registry, session_aliases, tokens);
      continue;
    }
    if (cmd == "emit") {
      cmd_emit_viz(registry, store, session_aliases, tokens);
      continue;
    }
    if (cmd == "route") {
      if (tokens.size() == 3 && tokens[1] == "type") {
        cmd_route_type(registry, tokens[2]);
        continue;
      }
      if (tokens.size() == 2) {
        cmd_route_object(registry, store, session_aliases, tokens[1]);
        continue;
      }
      std::cout << "error: usage: route <ObjectID> | route type <TypeName>\n";
      continue;
    }

    std::cout << "error: unknown command\n";
  }

  (void)store.close();
  return 0;
}
