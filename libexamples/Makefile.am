SUBDIRS = include

lib_LTLIBRARIES = libexamples.la
libexamples_la_SOURCES = examples.c
libexamples_la_CPPFLAGS = -I$(top_srcdir)/libexamples/include -I$(top_srcdir)/common
libexamples_la_LIBADD = ../common/libmicro.la
