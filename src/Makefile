all: cembeddedhttpd

cembeddedhttpd: main.c
	gcc -g -O0 -o $@ main.c

clean:
	-rm cembeddedhttpd

check: all
	./cembeddedhttpd | grep "Hello from .*cembeddedhttpd!"
	@echo "*** ALL TESTS PASSED ***"

install:
	install -d $(prefix)/bin
	install -m 0755 cembeddedhttpd $(prefix)/bin

uninstall:
	-rm $(prefix)/bin/cembeddedhttpd

.PHONY: all clean check install uninstall
