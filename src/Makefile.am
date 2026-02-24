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
   vizier/routing.h \
   vizier/routing.cc \
   viz/artifacts.h \
   viz/artifacts.cc \
   services/service.h \
   services/service.cc \
   refract/bootstrap.h \
   refract/bootstrap.cc \
   refract/schema_registry.h \
   refract/schema_registry.cc \
   referee_sqlite/sqlite_store.h \
   referee_sqlite/sqlite_store.cc

libreferee_la_CPPFLAGS = $(SQLITE_CFLAGS)
libreferee_la_LIBADD = $(SQLITE_LIBS)

include_HEADERS = referee/referee.h \
   services/service.h \
   refract/schema_registry.h \
   ceo/task_registry.h \
   ceo/io_reactor.h \
   exec/waitables.h \
   exec/await.h \
   parser/types.h \
   parser/tokenizer.h \
   parser/parser.h \
   parser/conch_command.h \
   parser/json_parser.h \
   parser/python_parser.h \
   parser/cpp_parser.h \
   parser/xml_parser.h \
   comms/primitives.h \
   vizier/routing.h \
   viz/artifacts.h
