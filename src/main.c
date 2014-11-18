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

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>

#define PORT 8888

const char cfgdir[FILENAME_MAX] = CFGDIR;

static int
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
    const char *page = "<html><body>Hello, browser!</body></html>";
    struct MHD_Response *response;
    int ret;
  
    response =
	MHD_create_response_from_buffer (strlen (page), (void *) page, 
					 MHD_RESPMEM_PERSISTENT);
    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);

    return ret;
}

int main(int argc, char **argv) 
{
    bool versionflag = false;
    int c;

    while ((c = getopt (argc, argv, "v")) != -1) {
	switch (c) {
	case 'v':
	    versionflag = true;
	    break;
	case '?':
	    if (isprint (optopt)) {
		fprintf (stderr, "Unknown option `-%c'.\n", optopt);
	    }
	    return 1;
	default:
	    abort ();
	}
    }

    if (versionflag) {
	printf("version\n");
    }
    return 0;
}

/*
  int
  main ()
  {
  struct MHD_Daemon *daemon;

  daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
  &answer_to_connection, NULL, MHD_OPTION_END);
  if (NULL == daemon)
  return 1;

  getchar ();

  MHD_stop_daemon (daemon);
  return 0;
  }
*/
