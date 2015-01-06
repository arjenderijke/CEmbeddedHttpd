#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#ifndef CFGDIR
#define CFGDIR "/etc"
#endif

#include "libexamples.h"
#include "libqueryhandler.h"

const char cfgdir[FILENAME_MAX] = CFGDIR;

int main(int argc, char ** argv) 
{
    bool versionflag = false;
    bool exampleflag = false;
    bool queryflag = false;
    int c;
    char * cvalue = NULL;

    while ((c = getopt (argc, argv, "e:vm")) != -1) {
	switch (c) {
	case 'v':
	    versionflag = true;
	    break;
	case 'e':
	    exampleflag = true;
	    cvalue = optarg;
	    break;
	case 'm':
	    queryflag = true;
	    break;
	case '?':
	    if (optopt == 'e') {
		fprintf (stderr, "Option -%c requires an argument.\n", optopt);
	    } else { 
		if (isprint (optopt)) {
		    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
		} else {
		    fprintf (stderr,
			     "Unknown option character `\\x%x'.\n",
			     optopt);
		}
	    }
	    return 1;
	default:
	    abort ();
	}
    }

    if (versionflag) {
	printf("version\n");
    }

    if (exampleflag) {
	run_example(1);
    }

    if (queryflag) {
	run_query_handler();
    }

    return 0;
}
