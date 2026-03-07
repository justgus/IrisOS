extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "refract/bootstrap.h"
#include "refract/schema_registry.h"
#include "referee/referee.h"
#include "referee_sqlite/sqlite_store.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <unistd.h>

using namespace referee;
using namespace iris::refract;

namespace {

template <typename T>
const char* result_message(const Result<T>& r) {
  return r.error.has_value() ? r.error->message.c_str() : "ok";
}

std::string run_conch_script(const std::string& script) {
  char db_template[] = "/tmp/iris-conch-db-XXXXXX";
  int db_fd = ::mkstemp(db_template);
  if (db_fd >= 0) {
    ::close(db_fd);
  }

  char script_template[] = "/tmp/iris-conch-script-XXXXXX";
  int script_fd = ::mkstemp(script_template);
  if (script_fd >= 0) {
    ::close(script_fd);
  }

  {
    std::ofstream out(script_template);
    out << script;
  }

  std::ostringstream cmd;
  cmd << "../bin/conch --db " << db_template << " < " << script_template;

  std::string output;
  FILE* pipe = ::popen(cmd.str().c_str(), "r");
  if (pipe) {
    char buffer[256];
    while (std::fgets(buffer, sizeof(buffer), pipe)) {
      output.append(buffer);
    }
    ::pclose(pipe);
  }

  ::unlink(script_template);
  ::unlink(db_template);
  return output;
}

} // namespace

START_TEST(test_conch_define_and_instantiate)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  TypeDefinition def{};
  def.type_id = TypeID{0xC0C1ULL};
  def.name = "Widget";
  def.namespace_name = "ConchTest";
  def.version = 1;
  def.fields.push_back(FieldDefinition{ "label", TypeID{0x1001ULL}, true, std::nullopt });

  auto regR = registry.register_definition(def);
  ck_assert_msg(regR, "register_definition failed: %s", result_message(regR));

  auto typesR = registry.list_types();
  ck_assert_msg(typesR, "list_types failed: %s", result_message(typesR));
  bool found = false;
  for (const auto& summary : typesR.value.value()) {
    if (summary.namespace_name == "ConchTest" && summary.name == "Widget") {
      found = true;
      ck_assert_uint_eq(summary.type_id.v, def.type_id.v);
      break;
    }
  }
  ck_assert_msg(found, "expected Widget type in registry list");

  nlohmann::json payload;
  payload["label"] = "alpha";
  auto cbor = nlohmann::json::to_cbor(payload);
  auto createR = store.create_object(def.type_id, regR.value->ref.id, cbor);
  ck_assert_msg(createR, "create_object failed: %s", result_message(createR));

  auto recR = store.get_latest(createR.value->ref.id);
  ck_assert_msg(recR, "get_latest failed: %s", result_message(recR));
  ck_assert_msg(recR.value->has_value(), "expected object record");
  ck_assert_uint_eq(recR.value->value().type.v, def.type_id.v);

  auto decoded = nlohmann::json::from_cbor(recR.value->value().payload_cbor);
  ck_assert_str_eq(decoded.value("label", "").c_str(), "alpha");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_conch_io_commands)
{
  std::ostringstream script;
  script << "let a=new Demo::PropulsionSynth name:=alpha\n";
  script << "let b=new Demo::PropulsionSynth name:=beta\n";
  script << "task spawn a\n";
  script << "task spawn b\n";
  script << "caps grant kernel.io\n";
  script << "io open channel 1 2\n";
  script << "io send io-0001 010203\n";
  script << "io await io-0002 2\n";
  script << "io recv io-0002 8\n";
  script << "io close io-0002\n";
  script << "exit\n";

  auto output = run_conch_script(script.str());
  ck_assert_msg(output.find("io channel io-0001 io-0002") != std::string::npos,
                "expected io channel output");
  ck_assert_msg(output.find("io send ready=true") != std::string::npos,
                "expected io send output");
  ck_assert_msg(output.find("io recv 010203") != std::string::npos,
                "expected io recv output");
  ck_assert_msg(output.find("io closed io-0002") != std::string::npos,
                "expected io close output");
}
END_TEST

START_TEST(test_conch_io_requires_caps)
{
  std::ostringstream script;
  script << "let a=new Demo::PropulsionSynth name:=alpha\n";
  script << "let b=new Demo::PropulsionSynth name:=beta\n";
  script << "task spawn a\n";
  script << "task spawn b\n";
  script << "io open channel 1 2\n";
  script << "exit\n";

  auto output = run_conch_script(script.str());
  ck_assert_msg(output.find("error: missing capability: kernel.io") != std::string::npos,
                "expected missing capability error");
}
END_TEST

Suite* conch_authoring_suite(void) {
  Suite* s = suite_create("ConchAuthoring");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_conch_define_and_instantiate);
  tcase_add_test(tc, test_conch_io_commands);
  tcase_add_test(tc, test_conch_io_requires_caps);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = conch_authoring_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
