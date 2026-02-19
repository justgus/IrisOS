AM_CXXFLAGS = -Wall -Wextra -Wpedantic -Werror
AM_CPPFLAGS = -I$(top_srcdir)/src $(SQLITE_CFLAGS)

bin_PROGRAMS = conch

conch_SOURCES = $(top_srcdir)/src/conch_shell/conch.cc
conch_LDADD = $(top_builddir)/src/libreferee.la $(SQLITE_LIBS)
