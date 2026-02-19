AM_CXXFLAGS = -Wall -Wextra -Wpedantic -Werror

bin_PROGRAMS = conch

conch_SOURCES = \
   conch_shell/conch.cc
conch_LDADD = libreferee.la

lib_LTLIBRARIES = libreferee.la
libreferee_la_SOURCES = \
   referee/referee.h \
   referee/referee.cc \
   ceo/task_registry.h \
   ceo/task_registry.cc \
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
   ceo/task_registry.h
