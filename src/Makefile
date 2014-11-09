all: cembeddedhttpd

CFLAGS = -g -O0

cembeddedhttpd: main.c
	$(CC) -DCFGDIR="\"$(sysconfdir)\"" $(CPPFLAGS) $(CFLAGS) -o $@ main.c

clean:
	-rm cembeddedhttpd

check: all
	./cembeddedhttpd | grep "Hello from .*cembeddedhttpd!"
	@echo "*** ALL TESTS PASSED ***"

install:
	install -d $(DESTDIR)$(bindir)
	install -m 0755 cembeddedhttpd $(DESTDIR)$(bindir)

uninstall:
	-rm $(DESTDIR)$(bindir)/cembeddedhttpd

.PHONY: all clean check install uninstall
