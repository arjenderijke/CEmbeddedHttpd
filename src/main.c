#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#ifndef CFGDIR
#define CFGDIR "/etc"
#endif

const char cfgdir[FILENAME_MAX] = CFGDIR;

int main(int argc, char * argv[])
{
	printf("Hello from %s!\n", argv[0]);
	return 0;
}
