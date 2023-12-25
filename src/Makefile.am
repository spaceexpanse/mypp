lib_LTLIBRARIES = libmypp.la
myppdir = $(includedir)/mypp

libmypp_la_CXXFLAGS = \
  $(MARIADB_CFLAGS) $(GLOG_CFLAGS)
libmypp_la_LIBADD = \
  $(MARIADB_LIBS) $(GLOG_LIBS)
libmypp_la_SOURCES = \
  mypp.cpp
mypp_HEADERS = \
  mypp.hpp
