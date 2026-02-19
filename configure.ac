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

# Unit test framework (Check)
PKG_CHECK_MODULES([CHECK],[check])

AC_CONFIG_FILES([
  Makefile
  bin/Makefile
  src/Makefile
  tests/Makefile
])
AC_OUTPUT
