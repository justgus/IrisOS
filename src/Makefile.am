AM_CXXFLAGS = -Wall -Wextra -Wpedantic -Werror

lib_LTLIBRARIES = libreferee.la
libreferee_la_SOURCES = \
   referee/referee.h \
   referee/referee.cc \
   referee_sqlite/sqlite_store.h \
   referee_sqlite/sqlite_store.cc

libreferee_la_CPPFLAGS = $(SQLITE_CFLAGS)
libreferee_la_LIBADD = $(SQLITE_LIBS)

include_HEADERS = referee/referee.h
