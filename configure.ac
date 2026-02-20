AC_INIT([referee],[0.1.0],[you@example.com])
AM_INIT_AUTOMAKE([foreign subdir-objects color-tests])
AC_CONFIG_SRCDIR([src/referee/referee.h])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CXX
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

# C++ standard
AX_CXX_COMPILE_STDCXX([20],[noext],[mandatory])

# SQLite
PKG_CHECK_MODULES([SQLITE],[sqlite3])

# Readline (optional)
AC_CHECK_HEADERS([readline/readline.h readline/history.h],
  [have_readline_headers=yes],
  [have_readline_headers=no])
AC_CHECK_LIB([readline],[readline],
  [have_readline_lib=yes],
  [have_readline_lib=no])
if test "x$have_readline_headers" = "xyes" && test "x$have_readline_lib" = "xyes"; then
  AC_DEFINE([HAVE_READLINE],[1],[Define if readline is available])
  READLINE_LIBS="-lreadline"
else
  AC_MSG_WARN([readline not found; conch history disabled])
  READLINE_LIBS=""
fi
AC_SUBST([READLINE_LIBS])

# Unit test framework (Check)
PKG_CHECK_MODULES([CHECK],[check])

AC_CONFIG_FILES([
  Makefile
  bin/Makefile
  src/Makefile
  tests/Makefile
])
AC_OUTPUT
