check_PROGRAMS = \
  testmock \
  queryhandler

if HAVE_CURL
  check_PROGRAMS += testcurl \
    testhttp10 \
    testpost \
    testbasic
endif

TESTS = $(check_PROGRAMS)

testmock_SOURCES = testmock.c
testmock_CPPFLAGS = -I$(top_srcdir)/libqueryhandler/include
testmock_LDADD = $(top_builddir)/libqueryhandler/libqueryhandler.la

queryhandler_SOURCES = queryhandler.c
queryhandler_CPPFLAGS = -I$(top_srcdir)/libqueryhandler/include -I$(top_srcdir)/libqueryhandler
queryhandler_LDADD = $(top_builddir)/libqueryhandler/libqueryhandler.la

testcurl_SOURCES = testcurl.c
testcurl_CPPFLAGS = -I$(top_srcdir)/libqueryhandler/include -I$(top_srcdir)/libqueryhandler
testcurl_LDADD = $(top_builddir)/libqueryhandler/libqueryhandler.la

testhttp10_SOURCES = testhttp10.c
testhttp10_CPPFLAGS = -I$(top_srcdir)/libqueryhandler/include -I$(top_srcdir)/libqueryhandler
testhttp10_LDADD = $(top_builddir)/libqueryhandler/libqueryhandler.la

testpost_SOURCES = testpost.c
testpost_CPPFLAGS = -I$(top_srcdir)/libqueryhandler/include -I$(top_srcdir)/libqueryhandler
testpost_LDADD = $(top_builddir)/libqueryhandler/libqueryhandler.la

testbasic_SOURCES = testbasic.c
testbasic_CPPFLAGS = -I$(top_srcdir)/libqueryhandler/include -I$(top_srcdir)/libqueryhandler
testbasic_LDADD = $(top_builddir)/libqueryhandler/libqueryhandler.la

CLEANFILES = cunit_testall-Listing.xml \
             cunit_testall-Results.xml \
             testmock \
             testcurl \
             queryhandler \
             testhttp10 \
             testpost \
             testbasic
