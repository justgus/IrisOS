#include "services/service_commands.h"

#include "services/capability_context.h"

#include <nlohmann/json.hpp>

namespace iris::service {

namespace {

void print_host(const ServiceHostSnapshot& host, std::ostream& output) {
  output << "service " << host.record.descriptor.name << " " << host.record.descriptor.id.to_hex()
         << " state=" << service_host_state_name(host.state)
         << " bound=" << (host.bound ? "yes" : "no")
         << " healthy=" << (host.healthy ? "yes" : "no")
         << " generation=" << host.record.generation << "\n";
}

} // namespace

void ServiceCommands::execute(const parser::ServiceCommand& command, referee::SqliteStore& store,
                              referee::ObjectID sender, std::ostream& output) {
  const auto& args = command.args;
  if (args.size() == 1 && args[0] == "list") {
    for (const auto& host : hosts_.list()) print_host(host, output);
    return;
  }
  if (args.size() == 2 && args[0] == "create" && args[1] == "memory") {
    MemoryService service(referee::ObjectID::random());
    auto created = hosts_.create(service.descriptor());
    if (!created) output << "error: " << created.error->message << "\n";
    else output << "service created: memory\n";
    return;
  }
  const bool control = args.size() == 2 && (args[0] == "show" || args[0] == "bind"
      || args[0] == "unbind" || args[0] == "start" || args[0] == "stop" || args[0] == "restart");
  const bool request = (args.size() == 3 || args.size() == 4) && args[0] == "request";
  if (!control && !request) {
    output << "usage: service list | create memory | show/bind/unbind/start/stop/restart <name>"
              " | request <name> <endpoint> [quoted-json]\n";
    return;
  }
  auto descriptor = hosts_.registry().resolve_by_name(args[1]);
  if (!descriptor) {
    output << "error: " << descriptor.error->message << "\n";
    return;
  }
  const auto id = descriptor.value->id;
  if (args[0] == "show") {
    auto host = hosts_.inspect(id);
    if (!host) output << "error: " << host.error->message << "\n";
    else print_host(host.value.value(), output);
    return;
  }
  if (request) {
    try {
      CapabilityContextStore contexts(store);
      CapabilityContextAuthorizer authorizer(contexts);
      IpcService ipc(hosts_.registry(), &authorizer);
      auto envelope = make_request_to_object(sender, id, {}, {});
      envelope.endpoint = Endpoint{args[2], std::nullopt, {}};
      bool declared = false;
      for (const auto& endpoint : descriptor.value->endpoints) {
        if (endpoint.name == args[2]) {
          envelope.message_type = endpoint.type.value_or(referee::TypeID{});
          declared = true;
          break;
        }
      }
      if (!declared) {
        output << "error: service endpoint not declared\n";
        return;
      }
      if (args.size() == 4) {
        envelope.payload_cbor = nlohmann::json::to_cbor(nlohmann::json::parse(args[3]));
      }
      auto response = ipc.send_request(envelope, std::chrono::milliseconds(100));
      if (!response) output << "error: " << response.error->message << "\n";
      else output << nlohmann::json::from_cbor(response.value->payload_cbor).dump() << "\n";
    } catch (const std::exception& ex) {
      output << "error: " << ex.what() << "\n";
    }
    return;
  }
  referee::Result<void> result;
  if (args[0] == "bind") {
    if (descriptor.value->type != kMemoryServiceType) {
      output << "error: no built-in handler for service type\n";
      return;
    }
    auto handler = std::make_unique<MemoryService>(id);
    result = hosts_.bind(id, *handler);
    if (result) handlers_[id.to_hex()] = std::move(handler);
  } else if (args[0] == "unbind") {
    result = hosts_.unbind(id);
    if (result) handlers_.erase(id.to_hex());
  } else if (args[0] == "start") result = hosts_.start(id);
  else if (args[0] == "stop") result = hosts_.stop(id);
  else result = hosts_.restart(id);
  if (!result) output << "error: " << result.error->message << "\n";
  else output << "service " << args[0] << ": " << args[1] << "\n";
}

} // namespace iris::service
