AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/ext/cppzmq $(LIBZMQ_CFLAGS) $(BOOST_CPPFLAGS)
ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = tdrs

tdrs_LDFLAGS = $(BOOST_PROGRAM_OPTIONS_LDFLAGS)
tdrs_LDADD = $(LIBZMQ_LIBS) $(BOOST_PROGRAM_OPTIONS_LIBS)

tdrs_SOURCES = \
  src/main.cpp \
  src/hub.cpp \
  src/tdrs.hpp
