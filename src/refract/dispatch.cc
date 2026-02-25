#include "refract/dispatch.h"

#include <deque>
#include <sstream>
#include <unordered_set>

namespace iris::refract {

namespace {

struct Candidate {
  OperationDefinition operation;
  referee::TypeID owner{};
  std::size_t depth{0};
  std::size_t type_penalty{0};
  std::size_t optional_penalty{0};
};

bool has_base_type(referee::TypeID type,
                   referee::TypeID base,
                   const DispatchEngine::InheritanceResolver& resolver) {
  if (!resolver) return false;
  std::deque<referee::TypeID> queue;
  std::unordered_set<std::uint64_t> visited;
  queue.push_back(type);
  visited.insert(type.v);

  while (!queue.empty()) {
    auto current = queue.front();
    queue.pop_front();
    for (const auto& parent : resolver(current)) {
      if (parent.v == base.v) return true;
      if (visited.insert(parent.v).second) queue.push_back(parent);
    }
  }

  return false;
}

bool matches_arity(const OperationDefinition& op, std::size_t arg_count) {
  std::size_t required = 0;
  for (const auto& param : op.signature.params) {
    if (!param.optional) ++required;
  }
  if (arg_count < required) return false;
  if (arg_count > op.signature.params.size()) return false;
  return true;
}

std::string format_operation(const Candidate& cand) {
  std::ostringstream os;
  os << cand.operation.name << "(";
  for (std::size_t i = 0; i < cand.operation.signature.params.size(); ++i) {
    if (i > 0) os << ", ";
    const auto& param = cand.operation.signature.params[i];
    os << "0x" << std::hex << param.type.v << std::dec;
    if (param.optional) os << "?";
  }
  os << ") owner=0x" << std::hex << cand.owner.v << std::dec;
  return os.str();
}

} // namespace

DispatchEngine::DispatchEngine(SchemaRegistry& registry,
                               InheritanceResolver resolver)
  : registry_(registry),
    resolver_(std::move(resolver)) {}

referee::Result<DispatchMatch> DispatchEngine::resolve(
    referee::TypeID target_type,
    std::string_view name,
    OperationScope scope,
    const std::vector<referee::TypeID>& arg_types,
    std::size_t arg_count,
    bool include_inherited) {
  std::deque<std::pair<referee::TypeID, std::size_t>> queue;
  std::unordered_set<std::uint64_t> visited;
  std::vector<Candidate> matches;

  queue.push_back({ target_type, 0 });
  visited.insert(target_type.v);

  while (!queue.empty()) {
    auto [current, depth] = queue.front();
    queue.pop_front();

    auto defR = registry_.get_latest_definition_by_type(current);
    if (!defR) return referee::Result<DispatchMatch>::err(defR.error->message);
    if (!defR.value->has_value()) {
      return referee::Result<DispatchMatch>::err("definition not found");
    }

    const auto& def = defR.value->value().definition;
    for (const auto& op : def.operations) {
      if (op.scope != scope) continue;
      if (op.name != name) continue;
      if (!matches_arity(op, arg_count)) continue;

      Candidate cand;
      cand.operation = op;
      cand.owner = current;
      cand.depth = depth;
      cand.optional_penalty = op.signature.params.size() - arg_count;

      if (!arg_types.empty() && arg_types.size() == arg_count) {
        bool ok = true;
        for (std::size_t i = 0; i < arg_count; ++i) {
          const auto& arg_type = arg_types[i];
          const auto& param_type = op.signature.params[i].type;
          if (arg_type.v == param_type.v) {
            continue;
          }
          if (has_base_type(arg_type, param_type, resolver_)) {
            cand.type_penalty += 1;
            continue;
          }
          ok = false;
          break;
        }
        if (!ok) continue;
      }

      matches.push_back(std::move(cand));
    }

    if (include_inherited && resolver_) {
      for (const auto& parent : resolver_(current)) {
        if (visited.insert(parent.v).second) {
          queue.push_back({ parent, depth + 1 });
        }
      }
    }
  }

  if (matches.empty()) {
    return referee::Result<DispatchMatch>::err("no matching operation");
  }

  auto better = [](const Candidate& a, const Candidate& b) {
    if (a.type_penalty != b.type_penalty) return a.type_penalty < b.type_penalty;
    if (a.optional_penalty != b.optional_penalty) return a.optional_penalty < b.optional_penalty;
    return a.depth < b.depth;
  };

  Candidate best = matches.front();
  for (std::size_t i = 1; i < matches.size(); ++i) {
    if (better(matches[i], best)) best = matches[i];
  }

  std::vector<Candidate> ties;
  for (const auto& cand : matches) {
    if (!better(best, cand) && !better(cand, best)) {
      ties.push_back(cand);
    }
  }

  if (ties.size() > 1) {
    std::ostringstream os;
    os << "ambiguous operation:";
    for (const auto& cand : ties) {
      os << " " << format_operation(cand) << ";";
    }
    return referee::Result<DispatchMatch>::err(os.str());
  }

  DispatchMatch out;
  out.operation = std::move(best.operation);
  out.owner_type = best.owner;
  out.depth = best.depth;
  return referee::Result<DispatchMatch>::ok(std::move(out));
}

} // namespace iris::refract
