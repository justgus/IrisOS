#pragma once

extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "refract/operation_catalog.h"
#include <filesystem>
#include <unistd.h>

namespace contract_test {
using namespace referee;
using namespace iris::refract;

template<class T> void ok(const Result<T>& result) {
  ck_assert_msg(static_cast<bool>(result), "%s", result.error ? result.error->message.c_str() : "unknown error");
}
template<class T> void error(const Result<T>& result, ErrorCode code) {
  ck_assert_msg(!result, "expected failure");
  ck_assert_int_eq(static_cast<int>(result.error->code), static_cast<int>(code));
}
inline OperationContract sample() {
  OperationContract c;
  c.key.id = ObjectID::from_hex("27000000000000000000000000000001");
  c.name = "add"; c.category = "arithmetic"; c.scope = OperationScope::Class;
  c.type_parameters = {"Self"};
  c.inputs = {{"left", {}, "Self", false}, {"right", {}, "Self", false}};
  c.outputs = {{"result", {}, "Self", false}};
  c.documentation.summary = "Return the sum without modifying either input.";
  return c;
}
struct Disk {
  Disk() {
    char path[] = "./.contract-test-XXXXXX";
    const auto* created = ::mkdtemp(path);
    ck_assert_ptr_nonnull(created); directory = created;
  }
  ~Disk() { std::filesystem::remove_all(directory); }
  std::string path() const { return directory + "/store"; }
  std::string directory;
};
inline int run(TCase* tc, const char* name) {
  auto* suite = suite_create(name);
  suite_add_tcase(suite, tc);
  auto* runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);
  const auto failures = srunner_ntests_failed(runner);
  srunner_free(runner);
  return failures ? 1 : 0;
}
}
