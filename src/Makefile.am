AM_CXXFLAGS = -Wall -Wextra -Wpedantic -Werror

lib_LTLIBRARIES = libreferee.la
libreferee_la_SOURCES = \
   referee/referee.h \
   referee/referee.cc \
   ceo/task_registry.h \
   ceo/task_registry.cc \
   ceo/io_reactor.h \
   ceo/io_reactor.cc \
   exec/await.h \
   exec/await.cc \
   exec/waitables.h \
   exec/waitables.cc \
   parser/types.h \
   parser/tokenizer.h \
   parser/tokenizer.cc \
   parser/parser.h \
   parser/parser.cc \
   parser/conch_command.h \
   parser/conch_command.cc \
   parser/conch_grammar.h \
   parser/conch_grammar.cc \
   parser/conch_batch.h \
   parser/conch_batch.cc \
   parser/json_parser.h \
   parser/json_parser.cc \
   parser/python_parser.h \
   parser/python_parser.cc \
   parser/cpp_parser.h \
   parser/cpp_parser.cc \
   parser/xml_parser.h \
   parser/xml_parser.cc \
   comms/primitives.h \
   comms/primitives.cc \
   comms/transport_session.h \
   comms/transport_session.cc \
   machine/scalars.h \
   machine/primitives.cc \
   machine/buffers.h \
   machine/buffers.cc \
   machine/definitions.h \
   machine/definitions.cc \
   machine/topology.h \
   machine/topology.cc \
   machine/inventory.h \
   machine/inventory.cc \
   machine/authority.h \
   machine/authority.cc \
   machine/refract.h \
   machine/refract.cc \
   conch/session_growth.h \
   conch/session_growth.cc \
   vizier/routing.h \
   vizier/routing.cc \
   viz/artifacts.h \
   viz/artifacts.cc \
   services/capability_context.h \
   services/capability_context.cc \
   services/service.h \
   services/service.cc \
   refract/bootstrap.h \
   refract/bootstrap.cc \
   refract/dispatch.h \
   refract/dispatch.cc \
   refract/operation_registry.h \
   refract/operation_registry.cc \
   refract/schema_registry.h \
   refract/schema_registry.cc \
   referee_sqlite/sqlite_store.h \
   referee_sqlite/sqlite_store.cc

libreferee_la_CPPFLAGS = $(SQLITE_CFLAGS) $(NLOHMANN_JSON_CFLAGS)
libreferee_la_LIBADD = $(SQLITE_LIBS)

include_HEADERS = referee/referee.h \
   services/service.h \
   refract/dispatch.h \
   refract/operation_registry.h \
   refract/schema_registry.h \
   ceo/task_registry.h \
   ceo/io_reactor.h \
   exec/waitables.h \
   exec/await.h \
   parser/types.h \
   parser/tokenizer.h \
   parser/parser.h \
   parser/conch_command.h \
   parser/conch_grammar.h \
   parser/conch_batch.h \
   parser/json_parser.h \
   parser/python_parser.h \
   parser/cpp_parser.h \
   parser/xml_parser.h \
   comms/primitives.h \
   comms/transport_session.h \
   machine/scalars.h \
   machine/buffers.h \
   machine/definitions.h \
   machine/topology.h \
   machine/inventory.h \
   machine/authority.h \
   machine/refract.h \
   conch/session_growth.h \
   vizier/routing.h \
   viz/artifacts.h \
   services/capability_context.h
