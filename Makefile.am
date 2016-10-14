AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/ext/cppzmq $(LIBZMQ_CFLAGS)

bin_PROGRAMS = tdrs

tdrs_LDADD = $(LIBZMQ_LIBS)

tdrs_SOURCES = \
  src/main.cpp \
  src/hub.cpp \
  src/tdrs.hpp
