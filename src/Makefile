all: cembeddedhttpd

cembeddedhttpd: main.c
	gcc -g -O0 -o $@ main.c

clean:
	-rm cembeddedhttpd

.PHONY: all clean
