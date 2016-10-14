AC_INIT([tdrs], [0.0.1], [houston@weltraum.co], [tdrs], [https://weltraum.co])
AC_PREREQ([2.68])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([subdir-objects foreign -Wall -Werror])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CXX([g++])

PKG_CHECK_MODULES(LIBZMQ, libzmq)

AC_OUTPUT
