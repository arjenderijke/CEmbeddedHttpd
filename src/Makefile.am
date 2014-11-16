bin_PROGRAMS = cembeddedhttpd
cembeddedhttpd_SOURCES = main.c

check_SCRIPTS = greptest.sh
TESTS = $(check_SCRIPTS)

greptest.sh:
	echo './cembeddedhttpd | grep "Hello"' > greptest.sh
	chmod +x greptest.sh

CLEANFILES = greptest.sh
 
