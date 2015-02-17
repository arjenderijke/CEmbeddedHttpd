#if !HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <microhttpd.h>
#include <uriparser/Uri.h>

#include "libqueryhandler.h"

#define DEFAULT_PORT 8888
#define MAX_GET_REQUEST_LENGTH 2147483648
#define DEFAULT_USER "monetdb"
#define DEFAULT_PASSWORD "monetdb"
#define ENABLE_AUTH true
#define DEFAULT_AUTH "basic"

static int
getPort()
{
    int port = DEFAULT_PORT;
    return port;
}

static bool
getEnableAuth()
{
    bool enable_auth = ENABLE_AUTH;
    return enable_auth;
}

static char *
getAuth()
{
    char * auth = DEFAULT_AUTH;
    return auth;
}

static char *
getServerUsername()
{
    char * server_user = DEFAULT_USER;
    return server_user;
}

static char *
getServerPassword()
{
    char * server_password = DEFAULT_PASSWORD;
    return server_password;
}

static int
authorize(const char * host, const int hostname_len, const char * username)
{
    char * hostname;
    int allow;
    hostname = malloc((hostname_len + 1) * sizeof(char));
    strncpy(hostname, host, hostname_len);
    hostname[hostname_len] = '\0';

    /*
     * TODO: read settings from file
     */
    if (username == NULL) {
	if (strcmp(hostname, "localhost") == 0) {
	    allow = MHD_YES;
	} else {
	    allow = MHD_NO;
	}
    } else {
	if ((strcmp(hostname, "localhost") == 0) &&
	    (strcmp(username, "arjen") == 0)) {
	    allow = MHD_YES;
	} else {
	    allow = MHD_NO;
	}
    }
    free(hostname);
    return allow;
}

static int
mock_authenticate(const char * username, const char * password)
{
    /*
     * TODO: authenticate against database
     */
    if ((strcmp (username, "arjen") != 0) ||
	(strcmp (password, "arjen") != 0)) {  
	return MHD_NO;
    } else {
	return MHD_YES;
    }
}

static int 
mock_database(const char * username, const char * query)
{
    return 0;
}

static int
mock_render_json(char ** result_json)
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
mock_cache(const char * tag, const char * page)
{
    return 0;
}

static int
help_page(char ** result_page)
{
    *result_page = (char *) malloc(100 * sizeof(char));

    sprintf(*result_page, "<html><body>Hello, browser! help</body></html>");
    return 0;
}

static int
version_page(char ** result_page)
{
    char * version = "Version 0.0.1";
    *result_page = (char *) malloc(100 * sizeof(char));

    sprintf(*result_page, "<html><body>Hello, browser! %s</body></html>", version);
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

static void
free_headers(const struct request_header_list * rhl)
{
    if (rhl->host != NULL) {
	free(rhl->host);
    }
    if (rhl->user_agent != NULL) {
	free(rhl->user_agent);
    }
    if (rhl->accept != NULL) {
	free(rhl->accept);
    }
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
	rhl->accept = malloc(strlen(value) * sizeof(char));
	strcpy(rhl->accept, value);
    }
    if (strcmp(key, MHD_HTTP_HEADER_HOST) == 0) {
	rhl->host = malloc(strlen(value) * sizeof(char));
	strcpy(rhl->host, value);
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
	    uqs->query_statement = malloc(strlen(value) * sizeof(char));
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
    char *page = NULL;
    struct MHD_Response *response;
    int ret;
  
    int use_request_handler = HTTP_API_HANDLE_ERROR;
    int return_code = MHD_HTTP_OK;

    char * username = NULL;
    char * password = NULL;
    char * query = NULL;
    char * tag = NULL;
    int hostname_len = 0;
    
    struct url_query_statements uqs = { 0, 0, false, QUERY_LANGUAGE_SQL, NULL, false };
    struct request_header_list rhl = { 0, 0, NULL, NULL, NULL };

    if (*con_cls == NULL) {
	*con_cls = connection;
	return MHD_YES;
    }

    printf ("New %s request for %s using version %s\n", method, url, version);

    MHD_get_connection_values (connection, MHD_HEADER_KIND, handle_httpd_headers,
			       &rhl);
    printf("headercount: %i\n", rhl.headers_found);

    /*
     * First check authorization.
     * Do not do anything unless a connection is allowed.
     * Here we only check hostname/ip-address.
     */
    hostname_len = strlen(rhl.host) - strlen(strstr(rhl.host, ":"));

    if (authorize(rhl.host, hostname_len, username) == MHD_NO) {
	return_code = MHD_HTTP_UNAUTHORIZED;
	error_page(&page, return_code);

	/*
	 * If a resultpage has been created, we should return it
	 * instead of continuing with the request.
	 */
	if (page != NULL) {
	    free_headers(&rhl);
	    response =
		MHD_create_response_from_buffer (strlen (page), (void *) page, 
						 MHD_RESPMEM_MUST_FREE);
	    ret = MHD_queue_response (connection, return_code, response);
	    MHD_destroy_response (response);

	    return ret;
	}
    }
    
    /*
     * Second, check some parts of the request.
     * This is more important than checking the users password.
     * We will not allow anything unless these criteria are met.
     */
    if (strcmp(version, MHD_HTTP_VERSION_1_1) != 0) {
	return_code = MHD_HTTP_HTTP_VERSION_NOT_SUPPORTED;
	error_page(&page, return_code);
    }

    if ((strcmp(method, MHD_HTTP_METHOD_GET) != 0) &&
	(strcmp(method, MHD_HTTP_METHOD_POST) != 0)) {
	return_code = MHD_HTTP_METHOD_NOT_ALLOWED;
	error_page(&page, return_code);
    }

    /*
     * If a resultpage has been created, we should return it
     * instead of continuing with the request.
     */
    if (page != NULL) {
	free_headers(&rhl);
	response =
	    MHD_create_response_from_buffer (strlen (page), (void *) page, 
					     MHD_RESPMEM_MUST_FREE);
	ret = MHD_queue_response (connection, return_code, response);
	MHD_destroy_response (response);

	return ret;
    }

    /*
     * Third, check authentification.
     * Only allow a authentificated user to continue.
     * Also check authorization of user/hostname combination.
     */
    username = MHD_basic_auth_get_username_password (connection, &password);
    if ((username != NULL) &&
	(mock_authenticate(username, password) == 0)) {
	if (authorize(rhl.host, hostname_len, username) == MHD_NO) {
	    return_code = MHD_HTTP_UNAUTHORIZED;
	    error_page(&page, return_code);
	}
    } else {
	return_code = MHD_HTTP_UNAUTHORIZED;
	error_page(&page, return_code);
    }

    /*
     * If a resultpage has been created, we should return it
     * instead of continuing with the request.
     */
    if (page != NULL) {
	if (username != NULL) free(username);
	if (password != NULL) free(password);
	
	free_headers(&rhl);
	response =
	    MHD_create_response_from_buffer (strlen (page), (void *) page, 
					     MHD_RESPMEM_MUST_FREE);
	ret = MHD_queue_basic_auth_fail_response (connection,
						  "monetdb",
						  response);
	MHD_destroy_response (response);

	return ret;
    }
    
    /*
     * Now try to handle the actual request
     */
    handle_request_uri(url, &use_request_handler);

    switch(use_request_handler) {
    case HTTP_API_HANDLE_HELP:
	printf ("help\n");
	help_page(&page);
	break;
    case HTTP_API_HANDLE_VERSION:
	printf ("version\n");
	version_page(&page);
	break;
    case HTTP_API_HANDLE_MCLIENT:
	return_code = MHD_HTTP_NOT_IMPLEMENTED;
	error_page(&page, return_code);
	break;
    case HTTP_API_HANDLE_NOTFOUND:
	/*
	 * If path is not found, stop processing and
	 * return an error.
	 */
	return_code = MHD_HTTP_NOT_FOUND;
	error_page(&page, return_code);
	break;
    case HTTP_API_HANDLE_BADREQUEST:
	/*
	 * If problem with path, stop processing and
	 * return an error.
	 */
	return_code = MHD_HTTP_BAD_REQUEST;
	error_page(&page, return_code);
	break;
    case HTTP_API_HANDLE_STATEMENT:
	printf ("statement\n");
	break;
    default:
	/*
	 * Unknown problem, stop processing
	 */
	printf ("error\n");
	return_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
	error_page(&page, return_code);
    }

    /*
     * If a resultpage has been created, we should return it
     * instead of continuing with the request.
     */
    if (page != NULL) {
	free_headers(&rhl);
	response =
	    MHD_create_response_from_buffer (strlen (page), (void *) page, 
					     MHD_RESPMEM_MUST_FREE);
	ret = MHD_queue_response (connection, return_code, response);
	MHD_destroy_response (response);

	return ret;
    }

    MHD_get_connection_values (connection, MHD_GET_ARGUMENT_KIND, handle_query_parameters,
			       &uqs);

    printf("querycount: %i\n", uqs.statements_found);

    if(mock_database(username, query) == 0) {
	if (mock_render_json(&page) == 0) {
	    // TODO: handle no caching
	    if (mock_cache(tag, page) == 0) {
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

    if (uqs.query_statement != NULL) {
	free(uqs.query_statement);
    }

    free_headers(&rhl);
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
    int listen_port = getPort();

    daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, listen_port, NULL,
			       NULL, &handle_request, NULL, MHD_OPTION_END);
    if (NULL == daemon)
	return 1;

    getchar ();

    MHD_stop_daemon (daemon);
}
