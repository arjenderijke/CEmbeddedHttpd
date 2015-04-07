check_PROGRAMS = \
  testmock

TESTS = $(check_PROGRAMS)

testmock_SOURCES = testmock.c
testmock_CPPFLAGS = -I$(top_srcdir)/libqueryhandler/include
testmock_LDADD = $(top_builddir)/libqueryhandler/libqueryhandler.la
