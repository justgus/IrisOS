// Audit counterexamples, not acceptance tests: output records the audited behavior.
#include "referee/referee.h"
#include "referee_sqlite/sqlite_store.h"
#include "refract/bootstrap.h"
#include "refract/dispatch.h"
#include "refract/schema_registry.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace referee;
using namespace iris::refract;

template <typename T>
T require(Result<T> result) {
  if (!result) throw std::runtime_error(result.error->message);
  return std::move(*result.value);
}

void require(Result<void> result) {
  if (!result) throw std::runtime_error(result.error->message);
}

int main(int argc, char** argv) {
  if (argc != 2 || std::filesystem::exists(argv[1])) {
    std::cerr << "Pass a new, nonexistent directory for disposable audit stores.\n";
    return 2;
  }
  try {
    const std::filesystem::path root(argv[1]);
    std::filesystem::create_directories(root);
    SqliteStore memory(SqliteConfig{.filename = ":memory:"});
    require(memory.open());
    const auto id = ObjectID::random();
    require(memory.create_object_with_id(id, TypeID{1}, {}, Bytes{1}));
    auto duplicate = memory.create_object_with_id(id, TypeID{1}, {}, Bytes{2});
    std::cout << "duplicate_identity_accepted=" << bool(duplicate) << '\n';
    std::cout << "duplicate_latest_payload=" << unsigned(require(memory.get_latest(id)).payload_cbor.at(0)) << '\n';
    std::cout << "object_id_text_length=" << id.to_hex().size() << '\n';

    bool exception = false;
    try { (void)cbor_from_json_string("{"); }
    catch (...) { exception = true; }
    std::cout << "public_cbor_helper_exception_escaped=" << exception << '\n';

    SchemaRegistry schema(memory);
    require(bootstrap_core_schema(schema));
    GenericRegistry generics(schema, memory);
    GenericInstance unknown;
    unknown.base_type = TypeID{0xDEADBEEF};
    std::cout << "unknown_generic_base_accepted=" << bool(generics.register_instance(unknown)) << '\n';
    GenericInstance list;
    list.base_type = TypeID{0x4352415400000002ULL};
    std::cout << "list_without_type_argument_accepted=" << bool(generics.register_instance(list)) << '\n';

    TypeDefinition packet;
    packet.type_id = TypeID{0xF001};
    packet.name = "AuditPacket";
    packet.kind = "packet";
    packet.packet_byte_order = "invalid-byte-order";
    packet.packet_fields.push_back({"field", TypeID{0x1002}, 0});
    std::cout << "zero_width_invalid_endian_packet_accepted=" << bool(schema.register_definition(packet)) << '\n';

    TypeDefinition callable;
    callable.type_id = TypeID{0xF002};
    callable.name = "AuditCallable";
    OperationDefinition operation;
    operation.name = "takes_string";
    operation.signature.params = {{"value", TypeID{0x1001}, false}};
    callable.operations = {operation};
    require(schema.register_definition(callable));
    DispatchEngine dispatch(schema);
    std::cout << "inconsistent_argument_type_count_accepted="
              << bool(dispatch.resolve(callable.type_id, operation.name, OperationScope::Object,
                                       {TypeID{0x1002}, TypeID{0x1002}}, 1)) << '\n';

    // Simulate a torn final object record, then write after reopening.
    const auto tail_base = (root / "tail").string();
    ObjectID first_id, lost_id, later_id;
    {
      SqliteStore store(SqliteConfig{.filename = tail_base});
      require(store.open());
      first_id = require(store.create_object(TypeID{1}, {}, Bytes{1})).ref.id;
      lost_id = require(store.create_object(TypeID{1}, {}, Bytes{2})).ref.id;
      require(store.close());
    }
    const auto tail_file = tail_base + ".segments/segments/objects.seg";
    std::filesystem::resize_file(tail_file, std::filesystem::file_size(tail_file) - 1);
    {
      SqliteStore store(SqliteConfig{.filename = tail_base});
      auto opened = store.open();
      std::cout << "torn_tail_first_reopen_ok=" << bool(opened) << '\n';
      require(opened);
      std::cout << "torn_tail_prior_record_present=" << bool(store.get_latest(first_id)) << '\n';
      std::cout << "torn_tail_incomplete_record_present=" << bool(store.get_latest(lost_id)) << '\n';
      later_id = require(store.create_object(TypeID{1}, {}, Bytes{3})).ref.id;
      require(store.close());
    }
    {
      SqliteStore store(SqliteConfig{.filename = tail_base});
      auto opened = store.open();
      std::cout << "torn_tail_second_reopen_ok=" << bool(opened) << '\n';
      if (!opened) std::cout << "torn_tail_second_reopen_error=" << opened.error->message << '\n';
      else std::cout << "torn_tail_later_record_present=" << bool(store.get_latest(later_id)) << '\n';
    }

    const auto flip_base = (root / "bitflip").string();
    ObjectID flip_id;
    {
      SqliteStore store(SqliteConfig{.filename = flip_base});
      require(store.open());
      flip_id = require(store.create_object(TypeID{1}, {}, Bytes{0x01})).ref.id;
      require(store.close());
    }
    {
      std::fstream file(flip_base + ".segments/segments/objects.seg",
                        std::ios::binary | std::ios::in | std::ios::out);
      file.seekp(-1, std::ios::end);
      file.put('\x02');
      if (!file) throw std::runtime_error("audit bit flip failed");
    }
    {
      SqliteStore store(SqliteConfig{.filename = flip_base});
      require(store.open());
      std::cout << "undetected_changed_payload=" << unsigned(require(store.get_latest(flip_id)).payload_cbor.at(0)) << '\n';
    }
  } catch (const std::exception& error) {
    std::cerr << "Audit setup failed: " << error.what() << '\n';
    return 1;
  }
}
