AM_CXXFLAGS = -Wall -Wextra -Wpedantic
AM_CPPFLAGS = -I$(top_srcdir)/src $(CHECK_CFLAGS)
AM_TESTS_ENVIRONMENT = CK_FORK=no

TESTS = test_referee_core test_service_ipc test_refract_registry test_refract_bootstrap test_ceo_tasks test_exec_waitables test_exec_integration test_vizier_routing test_viz_artifacts test_phase3_integration test_comms_primitives
check_PROGRAMS = $(TESTS)

test_referee_core_SOURCES = test_referee_core.cc
test_referee_core_LDADD = $(CHECK_LIBS) $(top_builddir)/src/libreferee.la

test_service_ipc_SOURCES = test_service_ipc.cc
test_service_ipc_LDADD = $(CHECK_LIBS) $(top_builddir)/src/libreferee.la

test_refract_registry_SOURCES = test_refract_registry.cc
test_refract_registry_LDADD = $(CHECK_LIBS) $(top_builddir)/src/libreferee.la

test_refract_bootstrap_SOURCES = test_refract_bootstrap.cc
test_refract_bootstrap_LDADD = $(CHECK_LIBS) $(top_builddir)/src/libreferee.la

test_ceo_tasks_SOURCES = test_ceo_tasks.cc
test_ceo_tasks_LDADD = $(CHECK_LIBS) $(top_builddir)/src/libreferee.la

test_exec_waitables_SOURCES = test_exec_waitables.cc
test_exec_waitables_LDADD = $(CHECK_LIBS) $(top_builddir)/src/libreferee.la

test_exec_integration_SOURCES = test_exec_integration.cc
test_exec_integration_LDADD = $(CHECK_LIBS) $(top_builddir)/src/libreferee.la

test_vizier_routing_SOURCES = test_vizier_routing.cc
test_vizier_routing_LDADD = $(CHECK_LIBS) $(top_builddir)/src/libreferee.la

test_viz_artifacts_SOURCES = test_viz_artifacts.cc
test_viz_artifacts_LDADD = $(CHECK_LIBS) $(top_builddir)/src/libreferee.la

test_phase3_integration_SOURCES = test_phase3_integration.cc
test_phase3_integration_LDADD = $(CHECK_LIBS) $(top_builddir)/src/libreferee.la

test_comms_primitives_SOURCES = test_comms_primitives.cc
test_comms_primitives_LDADD = $(CHECK_LIBS) $(top_builddir)/src/libreferee.la
