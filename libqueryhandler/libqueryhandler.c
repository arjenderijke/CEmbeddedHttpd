#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <microhttpd.h>
#include <uriparser/Uri.h>

#include "libqueryhandler.h"

#define PORT 8888

static int
handle_request_uri(const char *url, int *use_request_handler)
{
    UriParserStateA state;
    UriUriA uri;
    
    state.uri = &uri;
    if (uriParseUriA(&state, url) != URI_SUCCESS) {
	/* Failure */
	uriFreeUriMembersA(&uri);
	return MHD_NO;
    }

    if (uri.absolutePath == 1) {
	if (uri.pathHead != NULL) {
	    if (uri.pathHead->next == NULL) {
		if (strcmp(uri.pathHead->text.first, HTTP_API_HELP_PATH) == 0) {
		    *use_request_handler = HTTP_API_HANDLE_HELP;
		}
		if (strcmp(uri.pathHead->text.first, HTTP_API_VERSION_PATH) == 0) {
		    *use_request_handler = HTTP_API_HANDLE_VERSION;
		}
		if (strcmp(uri.pathHead->text.first, HTTP_API_STATEMENT_PATH) == 0) {
		    *use_request_handler = HTTP_API_HANDLE_STATEMENT;
		}
	    } else {
		/* 
		 * More than one path is an error
		 */
		*use_request_handler = HTTP_API_HANDLE_ERROR;
	    }
	} else {
	    /*
	     * Assume that query parameters exists
	     */
	    *use_request_handler = HTTP_API_HANDLE_STATEMENT;
	}
    } else {
	/*
	 * Relative paths is an error
	 */
	*use_request_handler = HTTP_API_HANDLE_ERROR;
    }

    uriFreeUriMembersA(&uri);
    return MHD_YES;
}

static int
print_out_key (void *cls, enum MHD_ValueKind kind, const char *key,
               const char *value)
{
    printf ("%s: %s\n", key, value);
    return MHD_YES;
}

static int
print_out_value (void *cls, enum MHD_ValueKind kind, const char *key,
		 const char *value)
{
    char query_key;
    struct url_query_statements * uqs = (struct url_query_statement *) cls;
    uqs->statements_found++;
    char * query_statement;

    if (strlen(key) > 1) {
	uqs->statements_error++;
    } else {
	query_key = key[0];
	switch (query_key) {
	case HTTP_QUERY_VERSION:
	    uqs->query_version = true;
	    break;
	case HTTP_QUERY_STATEMENT:
	    // TODO: handle malloc error
	    uqs->query_statement = malloc(strlen(value));
	    strcpy(uqs->query_statement, value);
	    break;
	case HTTP_QUERY_LANGUAGE:
	    if (strcmp(value, "mal") == 0) {
		uqs->query_language = QUERY_LANGUAGE_MAL;
		break;
	    }
	    if (strcmp(value, "sql") == 0) {
		uqs->query_language = QUERY_LANGUAGE_SQL;
		break;
	    }
	    uqs->query_language = QUERY_LANGUAGE_UNKNOWN;
	    uqs->statements_error++;
	    break;
	case HTTP_QUERY_MCLIENT:
	    uqs->query_mclient = true;
	    break;
	default:
	    uqs->statements_error++;
	}
    }
    printf ("%s: %s\n", key, value);
    return MHD_YES;
}

int
handle_request (void *cls, struct MHD_Connection *connection,
		const char *url, const char *method,
		const char *version, const char *upload_data,
		size_t *upload_data_size, void **con_cls)
{
    int use_request_handler = 0;
    int return_code = 0;

    struct url_query_statements uqs = { 0, 0, false, QUERY_LANGUAGE_SQL, NULL, false };

    printf ("New %s request for %s using version %s\n", method, url, version);

    if (strcmp(version, MHD_HTTP_VERSION_1_1) == 0) {
	handle_request_uri(url, &use_request_handler);
    } else {
	return_code = MHD_HTTP_HTTP_VERSION_NOT_SUPPORTED;
    }

    switch(use_request_handler) {
    case HTTP_API_HANDLE_HELP:
	printf ("help\n");
	break;
    case HTTP_API_HANDLE_VERSION:
	printf ("version\n");
	break;
    case HTTP_API_HANDLE_STATEMENT:
	printf ("statement\n");
	break;
    default:
	printf ("error\n");
    }

    MHD_get_connection_values (connection, MHD_HEADER_KIND, print_out_key,
			       NULL);

    MHD_get_connection_values (connection, MHD_GET_ARGUMENT_KIND, print_out_value,
			       &uqs);

    printf("querycount: %i\n", uqs.statements_found);

    if (uqs.query_statement != NULL) {
	free(uqs.query_statement);
    }

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
