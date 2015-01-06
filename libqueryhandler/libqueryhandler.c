#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <microhttpd.h>

#include "libqueryhandler.h"

#define PORT 8888

static int
print_out_key (void *cls, enum MHD_ValueKind kind, const char *key,
               const char *value)
{
    printf ("%s: %s\n", key, value);
    return MHD_YES;
}

int
handle_request (void *cls, struct MHD_Connection *connection,
		const char *url, const char *method,
		const char *version, const char *upload_data,
		size_t *upload_data_size, void **con_cls)
{
    printf ("New %s request for %s using version %s\n", method, url, version);

    MHD_get_connection_values (connection, MHD_HEADER_KIND, print_out_key,
			       NULL);

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

void run_query_handler() {
    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
			       &handle_request, NULL, MHD_OPTION_END);
    if (NULL == daemon)
	return 1;

    getchar ();

    MHD_stop_daemon (daemon);
}
