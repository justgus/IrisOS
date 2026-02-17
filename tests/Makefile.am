AM_CXXFLAGS = -Wall -Wextra -Wpedantic
AM_CPPFLAGS = -I$(top_srcdir)/src $(CHECK_CFLAGS)
AM_TESTS_ENVIRONMENT = CK_FORK=no

ASAN_CXXFLAGS = -O0 -g -fsanitize=address,undefined -fno-omit-frame-pointer
ASAN_LDFLAGS = -fsanitize=address,undefined

asan-check: check
	@$(MAKE) clean
	@$(MAKE) AM_CXXFLAGS="$(AM_CXXFLAGS) $(ASAN_CXXFLAGS)" \
		AM_LDFLAGS="$(AM_LDFLAGS) $(ASAN_LDFLAGS)" \
		check

TESTS = test_referee_core test_service_ipc test_refract_registry
check_PROGRAMS = $(TESTS)

test_referee_core_SOURCES = test_referee_core.cc
test_referee_core_LDADD = $(CHECK_LIBS) $(top_builddir)/src/libreferee.la

test_service_ipc_SOURCES = test_service_ipc.cc
test_service_ipc_LDADD = $(CHECK_LIBS) $(top_builddir)/src/libreferee.la

test_refract_registry_SOURCES = test_refract_registry.cc
test_refract_registry_LDADD = $(CHECK_LIBS) $(top_builddir)/src/libreferee.la
