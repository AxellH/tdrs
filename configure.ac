AC_INIT([tdrs], [0.0.1], [houston@weltraum.co], [tdrs], [https://weltraum.co])
AC_PREREQ([2.68])
AC_LANG([C++])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

m4_include([m4/ax_cxx_compile_stdcxx.m4])
AX_CXX_COMPILE_STDCXX(11, noext, mandatory)

AM_INIT_AUTOMAKE([subdir-objects foreign -Wall -Werror])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CXX([g++])
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(LIBZMQ, libzmq)
PKG_CHECK_MODULES(LIBZYRE, libzyre)
BOOST_REQUIRE
BOOST_PROGRAM_OPTIONS
MULE_CHECK_CRYPTOPP

AC_OUTPUT
