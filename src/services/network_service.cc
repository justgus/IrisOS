#include "services/network_service.h"

#include "services/request_validation.h"

#include <new>
#include <stdexcept>

namespace iris::service {
namespace {
nlohmann::json session_metadata(const comms::Session& session) {
  return {{"id", session.id().to_hex()}, {"transport", session.transport_id().to_hex()},
          {"first_endpoint", session.first_endpoint_id().to_hex()},
          {"second_endpoint", session.second_endpoint_id().to_hex()},
          {"state", session.state() == comms::SessionState::Open ? "open" : "closed"}};
}
}

NetworkService::NetworkService(referee::ObjectID id, refract::SchemaRegistry& schemas,
                               const machine::MachineLeaseRegistry& leases,
                               comms::Protocol protocol, comms::Transport transport,
                               std::size_t maximum_sessions)
    : desc_{id, kNetworkServiceType, "network", {},
            {{"network.open", kNetworkOpenType, {}},
             {"network.round_trip", kNetworkRoundTripType, {}},
             {"network.close", kNetworkCloseType, {}},
             {"network.inspect", kNetworkInspectType, {}}}},
      schemas_(schemas), leases_(leases), protocol_(std::move(protocol)),
      transport_(std::move(transport)), maximum_sessions_(maximum_sessions) {}

referee::Result<void> NetworkService::authorize(referee::ObjectID sender) const {
  const auto* resource = std::get_if<comms::AuthorizedTransportResource>(&transport_.resource());
  if (!resource || sender != resource->owner_id) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition,
                                      "authorized transport owner required");
  }
  auto use = leases_.authorize_use(resource->lease_id, sender, resource->capability_context_id);
  if (!use) return use;
  if (transport_.semantics() != comms::TransportSemantics::Stream ||
      !comms::check_compatibility(protocol_, transport_, leases_).compatible) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition,
                                      "incompatible network transport");
  }
  return referee::Result<void>::ok();
}

referee::Result<MessageEnvelope> NetworkService::handle_message(const MessageEnvelope& request) {
  try {
    auto operation = detail::operation_for(desc_, request);
    if (!operation) return referee::Result<MessageEnvelope>::err(*operation.error);
    auto payload = detail::object_payload(request.payload_cbor);
    if (!payload) return referee::Result<MessageEnvelope>::err(*payload.error);
    auto auth = authorize(request.sender);
    if (!auth) return referee::Result<MessageEnvelope>::err(*auth.error);
    const auto& input = *payload.value;
    if (*operation.value == kNetworkOpenType) {
      if (input.size() != 2) throw std::invalid_argument("expected two endpoint IDs");
      auto first = referee::ObjectID::from_hex(input.at("first_endpoint").get<std::string>());
      auto second = referee::ObjectID::from_hex(input.at("second_endpoint").get<std::string>());
      if (sessions_.size() >= maximum_sessions_) {
        return referee::Result<MessageEnvelope>::err(referee::ErrorCode::FailedPrecondition,
                                                     "session limit exceeded");
      }
      auto id = referee::ObjectID::random();
      while (sessions_.contains(id.to_hex())) id = referee::ObjectID::random();
      comms::Session session(id, {first}, {second}, transport_.id());
      auto opened = session.open();
      if (!opened) return referee::Result<MessageEnvelope>::err(*opened.error);
      auto response = make_response(request, desc_.id, kNetworkResponseType,
                                     nlohmann::json::to_cbor(session_metadata(session)));
      sessions_.emplace(id.to_hex(), std::move(session));
      return referee::Result<MessageEnvelope>::ok(std::move(response));
    }
    const auto expected_arguments = *operation.value == kNetworkRoundTripType ? 2U : 1U;
    if (input.size() != expected_arguments) throw std::invalid_argument("unexpected session arguments");
    auto id = referee::ObjectID::from_hex(input.at("id").get<std::string>()).to_hex();
    auto found = sessions_.find(id);
    if (found == sessions_.end()) {
      return referee::Result<MessageEnvelope>::err(referee::ErrorCode::NotFound,
                                                   "network session not found");
    }
    // Stage any lifecycle mutation until response construction succeeds.
    auto session = found->second;
    auto output = session_metadata(session);
    if (*operation.value == kNetworkCloseType) {
      auto closing = session.begin_close();
      if (!closing) return referee::Result<MessageEnvelope>::err(*closing.error);
      auto closed = session.finish_close();
      if (!closed) return referee::Result<MessageEnvelope>::err(*closed.error);
      output = session_metadata(session);
    } else if (*operation.value == kNetworkRoundTripType) {
      const auto& bytes = input.at("payload");
      if (!bytes.is_binary() || bytes.get_binary().size() > comms::kMaximumFramePayload) {
        throw std::invalid_argument("payload must be a bounded CBOR byte string");
      }
      std::vector<machine::Byte> data;
      data.reserve(bytes.get_binary().size());
      for (auto byte : bytes.get_binary()) data.emplace_back(byte);
      auto packet = comms::execute_registered_packet_round_trip(
          schemas_, leases_, protocol_, transport_, session,
          machine::Packet(machine::Blob(std::move(data))));
      if (!packet) return referee::Result<MessageEnvelope>::err(*packet.error);
      referee::Bytes result;
      result.reserve(packet.value->size());
      for (auto byte : packet.value->payload().bytes()) result.push_back(byte.value());
      output["payload"] = nlohmann::json::binary(std::move(result));
    }
    auto response = make_response(request, desc_.id, kNetworkResponseType,
                                   nlohmann::json::to_cbor(output));
    if (*operation.value == kNetworkCloseType) found->second = std::move(session);
    return referee::Result<MessageEnvelope>::ok(std::move(response));
  } catch (const std::bad_alloc&) {
    return referee::Result<MessageEnvelope>::err(referee::ErrorCode::IoError,
                                                 "network allocation failed");
  } catch (const std::exception& ex) {
    return referee::Result<MessageEnvelope>::err(referee::ErrorCode::InvalidArgument, ex.what());
  }
}

} // namespace iris::service
