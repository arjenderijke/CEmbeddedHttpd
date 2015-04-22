if HAVE_UNIT
  UNIT_DIR = unittest
endif

SUBDIRS = include $(UNIT_DIR)

lib_LTLIBRARIES = libqueryhandler.la
libqueryhandler_la_SOURCES = libqueryhandler.c libquerymock.c
libqueryhandler_la_CPPFLAGS = -I$(top_srcdir)/libqueryhandler/include
