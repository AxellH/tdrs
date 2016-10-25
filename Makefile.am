AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/ext/cppzmq -I$(top_srcdir)/ext/zyrecpp $(LIBZMQ_CFLAGS) $(LIBZYRE_CFLAGS) $(BOOST_CPPFLAGS) $(CRYPTOPP_CPPFLAGS)
ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = tdrs

tdrs_LDFLAGS = $(BOOST_PROGRAM_OPTIONS_LDFLAGS) $(CRYPTOPP_LDFLAGS)
tdrs_LDADD = $(LIBZMQ_LIBS) $(LIBZYRE_LIBS) $(BOOST_PROGRAM_OPTIONS_LIBS) $(CRYPTOPP_LIBS)

tdrs_SOURCES = \
  src/main.cpp \
  src/hub.cpp \
  src/hub_chain_client.cpp \
  src/hub_discovery_service_listener.cpp \
  src/tdrs.hpp
