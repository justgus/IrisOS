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

# Readline
AC_CHECK_HEADERS([readline/readline.h readline/history.h], [],
  [AC_MSG_ERROR([readline headers not found])])
AC_CHECK_LIB([readline],[readline], [READLINE_LIBS="-lreadline"],
  [AC_MSG_ERROR([libreadline not found])])
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
