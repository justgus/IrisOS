#include "refract/contract_dispatch.h"

#include <algorithm>
#include <deque>
#include <set>
#include <tuple>

namespace iris::refract {
referee::Result<bool> ContractDispatcher::subtype(referee::TypeID actual, referee::TypeID expected) {
  if (actual == expected) return referee::Result<bool>::ok(true);
  std::deque<referee::TypeID> queue{actual};
  std::set<std::uint64_t> visited{actual.v};
  while (!queue.empty()) {
    const auto current = queue.front(); queue.pop_front();
    auto parents = schema_.list_supertypes(current);
    if (!parents) {
      if (parents.error->code == referee::ErrorCode::NotFound) continue;
      return referee::Result<bool>::err(*parents.error);
    }
    for (const auto parent : *parents.value) {
      if (parent == expected) return referee::Result<bool>::ok(true);
      if (visited.insert(parent.v).second) queue.push_back(parent);
    }
  }
  return referee::Result<bool>::ok(false);
}

referee::Result<ConformingOperation> ContractDispatcher::resolve(
    referee::TypeID target, ContractKey contract, const std::vector<referee::TypeID>& inputs) {
  using Result = referee::Result<ConformingOperation>;
  if (target.v == 0 || std::any_of(inputs.begin(), inputs.end(), [](auto t) { return t.v == 0; }))
    return Result::err(referee::ErrorCode::InvalidArgument, "complete nonzero input types required");
  OperationRegistry operations(schema_);
  auto candidates = operations.list_conformances(target, catalog_, contract);
  if (!candidates) return Result::err(*candidates.error);
  std::optional<ConformingOperation> best;
  std::tuple<std::size_t, std::size_t, std::size_t> best_rank{};
  bool ambiguous = false;
  for (const auto& c : *candidates.value) {
    const auto& params = c.operation.signature.params;
    const auto required = std::count_if(params.begin(), params.end(), [](const auto& p) { return !p.optional; });
    if (inputs.size() < static_cast<std::size_t>(required) || inputs.size() > params.size()) continue;
    std::size_t penalty = 0;
    bool match = true;
    for (std::size_t i = 0; i < inputs.size(); ++i) {
      auto compatible = subtype(inputs[i], params[i].type);
      if (!compatible) return Result::err(*compatible.error);
      if (!*compatible.value) { match = false; break; }
      if (inputs[i] != params[i].type) ++penalty;
    }
    if (!match) continue;
    const auto rank = std::tuple{penalty, params.size() - inputs.size(), c.depth};
    if (!best || rank < best_rank) { best = c; best_rank = rank; ambiguous = false; }
    else if (rank == best_rank) ambiguous = true;
  }
  if (!best) return Result::err(referee::ErrorCode::NotFound, "no compatible contract implementation");
  if (ambiguous) return Result::err(referee::ErrorCode::FailedPrecondition, "ambiguous contract implementation");
  return Result::ok(std::move(*best));
}

referee::Result<void> ContractDispatcher::bind(
    referee::ObjectID implementation, std::uint64_t version, ContractCallback callback) {
  if (implementation == referee::ObjectID{} || version == 0 || !callback)
    return referee::Result<void>::err(referee::ErrorCode::InvalidArgument, "invalid implementation binding");
  auto key = std::pair{implementation.to_hex(), version};
  if (callbacks_.count(key)) return referee::Result<void>::err(referee::ErrorCode::AlreadyExists,
                                                             "implementation already bound");
  callbacks_.emplace(std::move(key), std::move(callback));
  return referee::Result<void>::ok();
}

referee::Result<ContractValues> ContractDispatcher::invoke(
    referee::TypeID target, ContractKey contract, const ContractValues& inputs,
    const std::vector<std::string>& capabilities, const std::optional<ContractValue>& receiver) {
  using Result = referee::Result<ContractValues>;
  std::vector<referee::TypeID> types;
  for (const auto& value : inputs) types.push_back(value.type);
  auto resolved = resolve(target, contract, types);
  if (!resolved) return Result::err(*resolved.error);
  const auto& op = resolved.value->operation;
  if ((op.scope == OperationScope::Object) != receiver.has_value() || (receiver && receiver->type != target))
    return Result::err(referee::ErrorCode::InvalidArgument, "receiver does not match operation scope/type");
  for (const auto& required : op.required_capabilities) {
    if (std::find(capabilities.begin(), capabilities.end(), required) == capabilities.end())
      return Result::err(referee::ErrorCode::FailedPrecondition, "missing capability: " + required);
  }
  const auto& binding = *op.binding;
  if (binding.route != ExecutionRoute::Local)
    return Result::err(referee::ErrorCode::Unsupported, "delegated execution route is not bound locally");
  auto callback = callbacks_.find({binding.implementation.to_hex(), binding.version});
  if (callback == callbacks_.end()) return Result::err(referee::ErrorCode::FailedPrecondition,
                                                      "implementation requires explicit runtime binding");
  try {
    auto result = callback->second(receiver, inputs);
    if (!result) return result;
    const auto& outputs = op.signature.outputs;
    const auto required = std::count_if(outputs.begin(), outputs.end(), [](const auto& p) { return !p.optional; });
    if (result.value->size() < static_cast<std::size_t>(required) || result.value->size() > outputs.size())
      return Result::err(referee::ErrorCode::FailedPrecondition, "implementation output count mismatch");
    for (std::size_t i = 0; i < result.value->size(); ++i) {
      if ((*result.value)[i].type != outputs[i].type)
        return Result::err(referee::ErrorCode::FailedPrecondition, "implementation output type mismatch");
    }
    return result;
  } catch (const std::exception& ex) {
    return Result::err(referee::ErrorCode::Internal, ex.what());
  } catch (...) {
    return Result::err(referee::ErrorCode::Internal, "implementation threw");
  }
}

referee::Result<ContractValues> ContractDispatcher::invoke_object(
    referee::ObjectID object, referee::SqliteStore& store, ContractKey contract,
    const ContractValues& inputs, const std::vector<std::string>& capabilities) {
  auto record = store.get_latest(object);
  if (!record) return referee::Result<ContractValues>::err(*record.error);
  return invoke(record.value->type, contract, inputs, capabilities,
                ContractValue{record.value->type, record.value->payload_cbor});
}
} // namespace iris::refract
