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
mock_authenticate()
{
    return 0;
}

static int 
mock_database()
{
    return 0;
}

static int
mock_render_json()
{
    return 0;
}

static int
mock_render_xml()
{
    return 0;
}

static int
mock_render_text()
{
    return 0;
}

static int
mock_cache()
{
    return 0;
}

static int
error_page(char ** result_page, const int status_code)
{
    *result_page = (char *) malloc(100 * sizeof(char));

    sprintf(*result_page, "<html><body>Hello, browser! %i</body></html>", status_code);
    return 0;
}

static int
result_page(char ** result_page, const int status_code)
{
    *result_page = (char *) malloc(100 * sizeof(char));

    sprintf(*result_page, "<html><body>Hello, browser! %i</body></html>", status_code);
    return 0;
}

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
		if (strcmp(uri.pathHead->text.first, HTTP_API_MCLIENT_PATH) == 0) {
		    *use_request_handler = HTTP_API_HANDLE_MCLIENT;
		}
		if (*use_request_handler == HTTP_API_HANDLE_ERROR) {
		    *use_request_handler = HTTP_API_HANDLE_NOTFOUND;
		}
	    } else {
		/* 
		 * More than one path is an error
		 */
		*use_request_handler = HTTP_API_HANDLE_BADREQUEST;
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
	*use_request_handler = HTTP_API_HANDLE_BADREQUEST;
    }

    uriFreeUriMembersA(&uri);
    return MHD_YES;
}

static int
handle_httpd_headers (void *cls, enum MHD_ValueKind kind, const char *key,
		      const char *value)
{
    struct request_header_list * rhl = (struct request_header_list *) cls;
    rhl->headers_found++;
    if (strcmp(key, MHD_HTTP_HEADER_ACCEPT) == 0) {
	rhl->accept = malloc(strlen(value));
	strcpy(rhl->accept, value);
    }
    printf ("%s: %s\n", key, value);
    return MHD_YES;
}

static int
handle_query_parameters (void *cls, enum MHD_ValueKind kind, const char *key,
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
    // TODO: check that memory of page is released by response function
    char *page;
    char *page2 = "help";
    struct MHD_Response *response;
    int ret;
  
    int use_request_handler = HTTP_API_HANDLE_ERROR;
    int return_code = MHD_HTTP_OK;

    struct url_query_statements uqs = { 0, 0, false, QUERY_LANGUAGE_SQL, NULL, false };
    struct request_header_list rhl = { 0, 0, NULL, NULL, NULL };

    printf ("New %s request for %s using version %s\n", method, url, version);

    if (strcmp(version, MHD_HTTP_VERSION_1_1) != 0) {
	return_code = MHD_HTTP_HTTP_VERSION_NOT_SUPPORTED;

	error_page(&page, return_code);
	response =
	    MHD_create_response_from_buffer (strlen (page), (void *) page, 
					     MHD_RESPMEM_MUST_FREE);
	ret = MHD_queue_response (connection, return_code, response);
	MHD_destroy_response (response);

	return ret;
    }

    if ((strcmp(method, MHD_HTTP_METHOD_GET) != 0) &&
	(strcmp(method, MHD_HTTP_METHOD_POST) != 0)) {
	return_code = MHD_HTTP_METHOD_NOT_ALLOWED;

	error_page(&page, return_code);
	response =
	    MHD_create_response_from_buffer (strlen (page), (void *) page, 
					     MHD_RESPMEM_MUST_FREE);
	ret = MHD_queue_response (connection, return_code, response);
	MHD_destroy_response (response);

	return ret;
    }

    handle_request_uri(url, &use_request_handler);

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
    case HTTP_API_HANDLE_MCLIENT:
	return_code = MHD_HTTP_NOT_IMPLEMENTED;
	break;
    case HTTP_API_HANDLE_NOTFOUND:
	return_code = MHD_HTTP_NOT_FOUND;
	break;
    case HTTP_API_HANDLE_BADREQUEST:
	return_code = MHD_HTTP_BAD_REQUEST;
	break;
    default:
	printf ("error\n");
    }

    /*
     * If path is not found, stop processing and
     * return an error.
     */
    if (return_code == MHD_HTTP_NOT_FOUND) {
	error_page(&page, return_code);
	response =
	    MHD_create_response_from_buffer (strlen (page), (void *) page, 
					     MHD_RESPMEM_MUST_FREE);
	ret = MHD_queue_response (connection, return_code, response);
	MHD_destroy_response (response);

	return ret;
    }

    /*
     * If problem with path, stop processing and
     * return an error.
     */
    if (return_code == MHD_HTTP_BAD_REQUEST) {
	error_page(&page, return_code);

	response =
	    MHD_create_response_from_buffer (strlen (page), (void *) page, 
					     MHD_RESPMEM_MUST_FREE);
	ret = MHD_queue_response (connection, return_code, response);
	MHD_destroy_response (response);

	return ret;
    }

    MHD_get_connection_values (connection, MHD_HEADER_KIND, handle_httpd_headers,
			       &rhl);

    MHD_get_connection_values (connection, MHD_GET_ARGUMENT_KIND, handle_query_parameters,
			       &uqs);

    printf("querycount: %i\n", uqs.statements_found);
    printf("headercount: %i\n", rhl.headers_found);

    if (mock_authenticate() == 0) {
	if(mock_database() == 0) {
	    if (mock_render_json() == 0) {
		if (mock_cache() == 0) {
		    /*
		     * Query succeeds. default return_code
		     */
		} else {
		    return_code = MHD_HTTP_NOT_MODIFIED;
		}
	    } else {
		return_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
	    }
	} else {
	    return_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
	}
    } else {
	return_code = MHD_HTTP_UNAUTHORIZED;
    }

    if (uqs.query_statement != NULL) {
	free(uqs.query_statement);
    }

    if (rhl.host != NULL) {
	free(rhl.host);
    }
    if (rhl.user_agent != NULL) {
	free(rhl.user_agent);
    }
    if (rhl.accept != NULL) {
	free(rhl.accept);
    }

    result_page(&page, return_code);
    response =
	MHD_create_response_from_buffer (strlen (page), (void *) page, 
					 MHD_RESPMEM_MUST_FREE);
    ret = MHD_queue_response (connection, return_code, response);
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
