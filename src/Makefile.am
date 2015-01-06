bin_PROGRAMS = cembeddedhttpd
cembeddedhttpd_SOURCES = main.c
cembeddedhttpd_CPPFLAGS = -I$(top_srcdir)/libexamples/include -I$(top_srcdir)/libqueryhandler/include
cembeddedhttpd_LDADD = ../libexamples/libexamples.la ../libqueryhandler/libqueryhandler.la

check_SCRIPTS = greptest.sh
TESTS = $(check_SCRIPTS)

greptest.sh:
	echo './cembeddedhttpd | grep "Hello"' > greptest.sh
	chmod +x greptest.sh

CLEANFILES = greptest.sh
