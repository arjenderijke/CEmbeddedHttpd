check_PROGRAMS = \
  testmock \
  queryhandler

TESTS = $(check_PROGRAMS)

testmock_SOURCES = testmock.c
testmock_CPPFLAGS = -I$(top_srcdir)/libqueryhandler/include
testmock_LDADD = $(top_builddir)/libqueryhandler/libqueryhandler.la

queryhandler_SOURCES = queryhandler.c
queryhandler_CPPFLAGS = -I$(top_srcdir)/libqueryhandler/include -I$(top_srcdir)/libqueryhandler
queryhandler_LDADD = $(top_builddir)/libqueryhandler/libqueryhandler.la

CLEANFILES = cunit_testall-Listing.xml \
             cunit_testall-Results.xml \
             testmock \
             queryhandler
