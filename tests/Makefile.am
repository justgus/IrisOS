AM_CXXFLAGS = -Wall -Wextra -Wpedantic
AM_CPPFLAGS = -I$(top_srcdir)/src $(CHECK_CFLAGS)

TESTS = test_referee_core test_service_ipc
check_PROGRAMS = $(TESTS)

test_referee_core_SOURCES = test_referee_core.cc
test_referee_core_LDADD = $(CHECK_LIBS) $(top_builddir)/src/libreferee.la

test_service_ipc_SOURCES = test_service_ipc.cc
test_service_ipc_LDADD = $(CHECK_LIBS) $(top_builddir)/src/libreferee.la
