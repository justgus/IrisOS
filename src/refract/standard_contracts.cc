#include "refract/standard_contracts.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace iris::refract {
namespace {
constexpr referee::TypeID kString{0x1001}, kU64{0x1002}, kF64{0x1008};
constexpr referee::TypeID primitives[] = {{0x1001}, {0x1002}, {0x1003}, {0x1004},
                                         {0x1005}, {0x1006}, {0x1007}, {0x1008}};
referee::ObjectID implementation_id(StandardContract kind, referee::TypeID type) {
  referee::ObjectID id{};
  id.bytes[0] = 0x27; id.bytes[1] = 0x27; id.bytes[7] = static_cast<unsigned>(kind) + 1;
  for (int i = 0; i < 8; ++i) id.bytes[8 + i] = (type.v >> (8 * i)) & 0xff;
  return id;
}
OperationDefinition operation(StandardContract kind, referee::TypeID type) {
  OperationDefinition op;
  op.name = kind == StandardContract::Add ? "add" : kind == StandardContract::Compare ? "compare" : "to_string";
  op.scope = kind == StandardContract::Add ? OperationScope::Class : OperationScope::Object;
  if (kind == StandardContract::Add) op.signature.params = {{"left", type, false}, {"right", type, false}};
  if (kind == StandardContract::Compare) op.signature.params = {{"other", type, false}};
  op.signature.outputs = {{"result", kind == StandardContract::ToString ? kString
                                   : kind == StandardContract::Compare ? kF64 : type, false}};
  op.conformances = {{standard_contract_key(kind), {{"Self", type}}}};
  op.binding = OperationBinding{implementation_id(kind, type), 1, ExecutionRoute::Local, ""};
  return op;
}
bool same_signature(const OperationDefinition& a, const OperationDefinition& b) {
  auto same = [](const auto& x, const auto& y) {
    if (x.size() != y.size()) return false;
    for (std::size_t i = 0; i < x.size(); ++i)
      if (x[i].type != y[i].type || x[i].optional != y[i].optional) return false;
    return true;
  };
  return a.scope == b.scope && same(a.signature.params, b.signature.params) && same(a.signature.outputs, b.signature.outputs);
}

nlohmann::json scalar(const ContractValue& v) {
  auto j = nlohmann::json::from_cbor(v.payload);
  const auto& x = j.at("value");
  if (v.type.v == 0x1007 && x.is_array()) {
    std::ostringstream hex;
    for (const auto& byte : x) {
      if (!byte.is_number_unsigned() || byte.get<std::uint64_t>() > 255)
        throw std::invalid_argument("invalid byte array value");
      hex << std::hex << std::setw(2) << std::setfill('0') << byte.get<unsigned>();
    }
    return hex.str();
  }
  if (v.type.v == 0x1001 || v.type.v == 0x1004 || v.type.v == 0x1007) {
    if (!x.is_string()) throw std::invalid_argument("expected string value");
    if (v.type.v == 0x1004) {
      const auto s = x.get<std::string>();
      if (s.size() != 32 || s.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
        throw std::invalid_argument("invalid ObjectID value");
      return referee::ObjectID::from_hex(s).to_hex();
    }
  } else if (v.type.v == 0x1003) {
    if (!x.is_boolean()) throw std::invalid_argument("expected boolean value");
  } else if (v.type == kF64) {
    if (!x.is_number() || !std::isfinite(x.get<double>())) throw std::invalid_argument("expected finite F64 value");
  } else if (!x.is_number_unsigned()) throw std::invalid_argument("expected unsigned value");
  return x;
}
ContractValue output(referee::TypeID type, const nlohmann::json& value) {
  return {type, nlohmann::json::to_cbor({{"value", value}})};
}

referee::Result<ContractValues> execute(
    StandardContract kind, referee::TypeID type, const std::optional<ContractValue>& receiver,
    const ContractValues& inputs) {
  using Result = referee::Result<ContractValues>;
  try {
    if (kind == StandardContract::Add) {
      const auto a = scalar(inputs.at(0)), b = scalar(inputs.at(1));
      if (type == kU64) {
        const auto left = a.get<std::uint64_t>(), right = b.get<std::uint64_t>();
        if (left > UINT64_MAX - right) return Result::err(referee::ErrorCode::InvalidArgument, "U64 addition overflow");
        return Result::ok({output(type, left + right)});
      }
      const auto sum = a.get<double>() + b.get<double>();
      if (!std::isfinite(sum)) return Result::err(referee::ErrorCode::InvalidArgument, "non-finite F64 sum");
      return Result::ok({output(type, sum)});
    }
    const auto self = scalar(receiver.value());
    if (kind == StandardContract::Compare) {
      const auto other = scalar(inputs.at(0));
      double order = 0;
      if (type == kF64) {
        const auto a = self.get<double>(), b = other.get<double>(); order = a == b ? 0 : a < b ? -1 : 1;
      } else { order = self == other ? 0 : self < other ? -1 : 1; }
      return Result::ok({output(kF64, order)});
    }
    std::ostringstream text;
    if (type.v == 0x1005) text << "0x" << std::hex << self.get<std::uint64_t>();
    else if (self.is_string()) text << self.get<std::string>();
    else if (self.is_boolean()) text << (self.get<bool>() ? "true" : "false");
    else if (type == kF64) text << self.get<double>();
    else text << self.get<std::uint64_t>();
    return Result::ok({output(kString, text.str())});
  } catch (const std::exception& ex) {
    return Result::err(referee::ErrorCode::InvalidArgument, ex.what());
  }
}
} // namespace

ContractKey standard_contract_key(StandardContract kind) {
  auto id = referee::ObjectID::from_hex("27000000000000000000000000000001");
  id.bytes[15] = static_cast<unsigned>(kind) + 1;
  return {id, 1};
}
std::vector<OperationContract> standard_operation_contracts() {
  std::vector<OperationContract> out;
  for (const auto kind : {StandardContract::ToString, StandardContract::Compare, StandardContract::Add}) {
    OperationContract c; c.key = standard_contract_key(kind); c.type_parameters = {"Self"};
    if (kind == StandardContract::ToString) {
      c.name = "to_string"; c.category = "conversion";
      c.outputs = {{"text", kString, "", false}};
      c.documentation.summary = "Return receiver text using the primitive display convention; do not mutate the receiver. F64 values must be finite.";
    } else if (kind == StandardContract::Compare) {
      c.name = "compare"; c.category = "comparison";
      c.inputs = {{"other", {}, "Self", false}}; c.outputs = {{"order", kF64, "", false}};
      c.documentation.summary = "Compare the receiver with other: return F64 -1, 0 or 1. F64 values must be finite. Inputs are not mutated.";
    } else {
      c.name = "add"; c.category = "arithmetic"; c.scope = OperationScope::Class;
      c.inputs = {{"left", {}, "Self", false}, {"right", {}, "Self", false}};
      c.outputs = {{"result", {}, "Self", false}};
      c.documentation.summary = "Return left + right as a separate Self value. U64 overflow and non-finite F64 inputs/results fail with InvalidArgument. No assignment or input mutation.";
    }
    out.push_back(std::move(c));
  }
  return out;
}

referee::Result<BootstrapResult> bootstrap_standard_contracts(SchemaRegistry& schema, OperationCatalog& catalog) {
  using Result = referee::Result<BootstrapResult>;
  for (const auto& contract : standard_operation_contracts()) {
    auto stored = catalog.register_contract(contract);
    if (!stored) return Result::err(*stored.error);
  }
  BootstrapResult result;
  for (const auto type : primitives) {
    auto prior = schema.get_latest_definition_by_type(type);
    if (!prior) return Result::err(*prior.error);
    auto def = prior.value->definition;
    bool changed = false;
    for (const auto kind : {StandardContract::ToString, StandardContract::Compare, StandardContract::Add}) {
      if (kind == StandardContract::Add && type != kU64 && type != kF64) continue;
      auto expected = operation(kind, type);
      auto found = def.operations.end();
      for (auto it = def.operations.begin(); it != def.operations.end(); ++it) {
        if (it->name == expected.name && it->scope == expected.scope) {
          if (!same_signature(*it, expected))
            return Result::err(referee::ErrorCode::FailedPrecondition,
                               "existing standard operation conflicts: " + def.name + "::" + expected.name);
          // Older Bytes definitions repeat core operations. Adopt one compatible declaration;
          // do not manufacture duplicate explicit claims or rewrite the legacy declarations.
          if (found != def.operations.end()) {
            if (it->binding || !it->conformances.empty())
              return Result::err(referee::ErrorCode::FailedPrecondition, "duplicate explicit standard conformance");
            continue;
          }
          found = it;
        }
      }
      if (found == def.operations.end()) { def.operations.push_back(expected); changed = true; continue; }
      bool claimed = false;
      for (const auto& claim : found->conformances) {
        if (claim.contract == expected.conformances[0].contract) {
          if (claim.substitutions != expected.conformances[0].substitutions)
            return Result::err(referee::ErrorCode::FailedPrecondition, "conflicting standard conformance");
          claimed = true;
        }
      }
      if (found->binding && (found->binding->implementation != expected.binding->implementation ||
          found->binding->version != 1 || found->binding->route != ExecutionRoute::Local))
        return Result::err(referee::ErrorCode::FailedPrecondition, "existing implementation binding conflicts");
      if (!claimed) { found->conformances.push_back(expected.conformances[0]); changed = true; }
      if (!found->binding) { found->binding = expected.binding; changed = true; }
    }
    if (!changed) { ++result.existing; continue; }
    if (def.version == UINT64_MAX) return Result::err(referee::ErrorCode::FailedPrecondition, "schema version exhausted");
    ++def.version; def.supersedes_definition_id = prior.value->ref.id;
    auto stored = schema.register_definition(def);
    if (!stored) return Result::err(*stored.error);
    ++result.inserted;
  }
  return Result::ok(result);
}

referee::Result<void> bind_standard_contracts(ContractDispatcher& dispatcher) {
  for (const auto type : primitives) {
    for (const auto kind : {StandardContract::ToString, StandardContract::Compare, StandardContract::Add}) {
      if (kind == StandardContract::Add && type != kU64 && type != kF64) continue;
      auto bound = dispatcher.bind(implementation_id(kind, type), 1,
          [kind, type](const auto& receiver, const auto& inputs) { return execute(kind, type, receiver, inputs); });
      if (!bound) return bound;
    }
  }
  return referee::Result<void>::ok();
}
} // namespace iris::refract
