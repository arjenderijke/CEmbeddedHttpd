#if !HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <microhttpd.h>
#include <uriparser/Uri.h>

#include "libqueryhandler.h"

#define DEFAULT_PORT 8888
#define MAX_GET_REQUEST_LENGTH 2147483648
#define DEFAULT_USER "monetdb"
#define DEFAULT_PASSWORD "monetdb"
#define ENABLE_AUTH true
#define DEFAULT_AUTH "basic"
#define DEFAULT_VERSION "0.0.1"

#define POSTBUFFERSIZE  512
#define MAXQUERYSIZE     20
#define MAXANSWERSIZE   512

#define GET             0
#define POST            1
#define MAXCLIENTS      2
static unsigned int    nr_of_uploading_clients = 0;

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

static char *
getVersion()
{
    char * version = DEFAULT_VERSION;
    return version;
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
     * [TODO]: read settings from file
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
     * [TODO]: authenticate against database
     */
    if ((strcmp (username, "arjen") != 0) ||
	(strcmp (password, "arjen") != 0)) {  
	return MHD_NO;
    } else {
	return MHD_YES;
    }
}

static int 
mock_database(const char * username, const char * query, db_resultset * query_result)
{
    /*
     * [TODO]: login to database with user "username". Run query "query"
     *         and get resultset in "query_result"
     */
    *query_result = 1;
    return 0;
}

static int
mock_render_json(char ** result_json, const db_resultset * query_result)
{
    char * renderjson = "{ \"column\" : \"value\" }";
    
    if (*query_result == 1) {
	*result_json = (char *) malloc(strlen(renderjson) * sizeof(char));
	sprintf(*result_json, renderjson);
    }
    
    return 0;
}

static int
mock_render_csv(char ** result_csv, const db_resultset * query_result)
{
    char * rendercsv = "column\n"
	"value\n";
    
    if (*query_result == 1) {
	*result_csv = (char *) malloc(strlen(rendercsv) * sizeof(char));
	sprintf(*result_csv, rendercsv);
    }
    
    return 0;
}

static int
mock_render_xml(char ** result_xml, const db_resultset * query_result)
{
    char * renderxml = "<xml>"
	"<value />";
    
    if (*query_result == 1) {
	*result_xml = (char *) malloc(strlen(renderxml) * sizeof(char));
	sprintf(*result_xml, renderxml);
    }
    
    return 0;
}

static int
mock_render_html(char ** result_html, const db_resultset * query_result)
{
    char * renderhtml = "<html>"
	"</html>";
    
    if (*query_result == 1) {
	*result_html = (char *) malloc(strlen(renderhtml) * sizeof(char));
	sprintf(*result_html, renderhtml);
    }
    
    return 0;
}

static int
mock_cache(const char ** tag, const char * page)
{
    char * mocktag = "ToCache";
    *tag = (char *) malloc(strlen(mocktag) * sizeof(char));
    strcpy(*tag, mocktag);
    return 0;
}

static int
help_page(char ** result_page, const int returntype)
{
    char * helppage_html = "<html>"
	"<title>CEmbeddedHttpd Help</title>"
	"<body>"
	"<h1>Helptext</h1>"
	"<p>help</p>"
	"</body></html>\n";

    switch (returntype) {
    case RETURN_XML:
    case RETURN_JSON:
    case RETURN_CSV:
    default:
	// RETURN_HTML
	*result_page = (char *) malloc(strlen(helppage_html) * sizeof(char));
	sprintf(*result_page, helppage_html);
    }

    return 0;
}

static int
version_page(char ** result_page, const int returntype)
{
    char * version = getVersion();
    char * versionHTML = "<html>"
	"<title>CEmbeddedHttpd Version</title>"
	"<body>"
	"<p>Version %s</p>"
	"</body></html>\n";
    char * versionXML = "<xml>"
	"<version>%s</version>\n";
    char * versionJSON = "{ \"version\" : \"%s\" }\n";
    char * versionCSV = "version\n%s\n";

    switch (returntype) {
    case RETURN_XML:
	*result_page = (char *) malloc((strlen(versionXML) + strlen(version)) * sizeof(char));
	sprintf(*result_page, versionXML, version);
	break;
    case RETURN_JSON:
	*result_page = (char *) malloc((strlen(versionJSON) + strlen(version)) * sizeof(char));
	sprintf(*result_page, versionJSON, version);
	break;
    case RETURN_CSV:
	*result_page = (char *) malloc((strlen(versionCSV) + strlen(version)) * sizeof(char));
	sprintf(*result_page, versionCSV, version);
	break;
    default:
	// RETURN_HTML
	*result_page = (char *) malloc((strlen(versionHTML) + strlen(version)) * sizeof(char));
	sprintf(*result_page, versionHTML, version);
    }

    return 0;
}

static int
error_page(char ** result_page, const int status_code, const int returntype, const bool quiet)
{
    char * errorHTML = "<html>"
	"<title>CEmbeddedHttpd Version</title>"
	"<body>"
	"<p>Version %s</p>"
	"</body></html>\n";
    char * errorXML = "<xml>"
	"<version>%s</version>\n";
    char * errorJSON = "{ \"version\" : \"%s\" }\n";
    char * errorCSV = "version\n%s\n";
    char error_code[3] = "";
    sprintf(error_code, "%i", status_code);
    
    if (quiet) {
	*result_page = (char *) malloc(strlen(EMPTY) * sizeof(char));
	sprintf(*result_page, EMPTY);
    } else {
	/*
	 * result depends on mime type
	 */
	switch (returntype) {
	case RETURN_XML:
	    *result_page = (char *) malloc((strlen(errorXML) + strlen(error_code)) * sizeof(char));
	    sprintf(*result_page, errorXML, error_code);
	    break;
	case RETURN_JSON:
	    *result_page = (char *) malloc((strlen(errorJSON) + strlen(error_code)) * sizeof(char));
	    sprintf(*result_page, errorJSON, error_code);
	    break;
	case RETURN_CSV:
	    *result_page = (char *) malloc((strlen(errorCSV) + strlen(error_code)) * sizeof(char));
	    sprintf(*result_page, errorCSV, error_code);
	    break;
	default:
	    // RETURN_HTML
	    *result_page = (char *) malloc((strlen(errorHTML) + strlen(error_code)) * sizeof(char));
	    sprintf(*result_page, errorHTML, error_code);
	}
    }
    
    return MHD_YES;
}

static void
setContentTypeHeader(const struct MHD_Response * response, const int returntype)
{
    switch (returntype) {
    case RETURN_XML:
	MHD_add_response_header (response,
				 MHD_HTTP_HEADER_CONTENT_TYPE,
				 ACCEPT_XML);
	break;
    case RETURN_JSON:
	MHD_add_response_header (response,
				 MHD_HTTP_HEADER_CONTENT_TYPE,
				 ACCEPT_JSON);
	break;
    case RETURN_CSV:
	MHD_add_response_header (response,
				 MHD_HTTP_HEADER_CONTENT_TYPE,
				 ACCEPT_CSV);
	break;
    case RETURN_HTML:
    default:
	MHD_add_response_header (response,
				 MHD_HTTP_HEADER_CONTENT_TYPE,
				 ACCEPT_HTML);
    }

    return;
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
    if (rhl->etag != NULL) {
	free(rhl->etag);
    }
}

static int
handle_accept (void *cls, const struct sockaddr * addr, socklen_t addrlen)
{
    char * client_ip;
    int can_connect;
    char * hostname = "localhost";
    client_ip = (char *) malloc(INET_ADDRSTRLEN * sizeof(char));
    inet_ntop(addr->sa_family, &(((struct sockaddr_in *)addr)->sin_addr),
	      client_ip, INET_ADDRSTRLEN);
    printf("debug: check accept %s\n", client_ip);

    /*
     * [TODO]: handle reverse dns lookups
     */
    can_connect = authorize(hostname, strlen(hostname), NULL);
    free(client_ip);
    return can_connect;
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
    if (strcmp(key, MHD_HTTP_HEADER_ETAG) == 0) {
	rhl->etag = malloc(strlen(value) * sizeof(char));
	strcpy(rhl->etag, value);
    }

    printf ("debug: %s: %s\n", key, value);
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
	    // [TODO]: handle malloc error
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
    printf ("debug: %s: %s\n", key, value);
    return MHD_YES;
}

static int
setReturnContent(const char * accept)
{
    if (strcmp(accept, ACCEPT_HTML) == 0) return RETURN_HTML;
    if (strcmp(accept, ACCEPT_XML) == 0) return RETURN_XML;
    if (strcmp(accept, ACCEPT_JSON) == 0) return RETURN_JSON;
    if (strcmp(accept, ACCEPT_CSV) == 0) return RETURN_CSV;
    
    /*
     * In all other cases, return html
     */
    return RETURN_HTML;
}

static int
handle_post_parameters (void *cls, enum MHD_ValueKind kind, const char *key,
			 const char *value)
{
    printf ("debug: %s: %s\n", key, value);
    return MHD_YES;
}

static int 
iterate_post (void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
              const char *filename, const char *content_type,
              const char *transfer_encoding, const char *data, 
	      uint64_t off, size_t size)
{
    struct connection_info_struct *con_info = coninfo_cls;

    /*
     * [TODO]: concat parts of post data, set responsed if it failes
     *         and make sure the memory of answerstring is freed
     */
    printf("debug: key : %s\n", key);
    if (strcmp (key, "query") == 0) {
	if ((size > 0) && (size <= MAXQUERYSIZE)) {
	    char *answerstring;
	    answerstring = malloc (MAXANSWERSIZE);
	    if (!answerstring) return MHD_NO;
      
	    snprintf (answerstring, MAXANSWERSIZE, "%s", data);
	    con_info->answerstring = answerstring;      
	} else {
	  con_info->answerstring = NULL;
	}
	return MHD_NO;
    }

    return MHD_YES;
}

void request_completed (void *cls, struct MHD_Connection *connection, 
     		        void **con_cls,
                        enum MHD_RequestTerminationCode toe)
{
  struct connection_info_struct *con_info = *con_cls;

  if (con_info == NULL) {
      return;
  }
  
  if (con_info->connectiontype == POST) {
      if (con_info->postprocessor != NULL) {
	  MHD_destroy_post_processor (con_info->postprocessor);        
	  if (con_info->answerstring) {
	      free (con_info->answerstring);
	  }
      }
  }
  
  free (con_info);
  *con_cls = NULL;   
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
    int return_content = RETURN_HTML;
    int quiet_error = true;
    
    char * username = NULL;
    char * password = NULL;
    char * query = NULL;
    char * tag = NULL;
    int hostname_len = 0;

    db_resultset query_result = 0;
    // [TODO]: read setting from config
    bool handle_http_cache = true;
    
    struct url_query_statements uqs = { 0, 0, false, QUERY_LANGUAGE_SQL, NULL, false };
    struct request_header_list rhl = { 0, 0, NULL, NULL, NULL, NULL };

    printf ("debug: New %s request for %s using version %s\n", method, url, version);

    MHD_get_connection_values (connection, MHD_HEADER_KIND, handle_httpd_headers,
			       &rhl);
    printf("debug: headercount: %i\n", rhl.headers_found);
    return_content = setReturnContent(rhl.accept);

    /*
     * First, check some parts of the request.
     * This is more important than checking the users password.
     * We will not allow anything unless these criteria are met.
     */
    if (strcmp(version, MHD_HTTP_VERSION_1_1) != 0) {
	return_code = MHD_HTTP_HTTP_VERSION_NOT_SUPPORTED;
	error_page(&page, return_code, return_content, quiet_error);
    }

    if ((strcmp(method, MHD_HTTP_METHOD_GET) != 0) &&
	(strcmp(method, MHD_HTTP_METHOD_POST) != 0)) {
	return_code = MHD_HTTP_METHOD_NOT_ALLOWED;
	error_page(&page, return_code, return_content, quiet_error);
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
	setContentTypeHeader(response, return_content);
	ret = MHD_queue_response (connection, return_code, response);
	MHD_destroy_response (response);

	return ret;
    }

    /*
     * Second, check authentification.
     * Only allow a authentificated user to continue.
     * Also check authorization of user/hostname combination.
     */
    username = MHD_basic_auth_get_username_password (connection, &password);
    hostname_len = strlen(rhl.host) - strlen(strstr(rhl.host, ":"));
    if ((username != NULL) &&
	(mock_authenticate(username, password) == MHD_YES)) {
	if (authorize(rhl.host, hostname_len, username) == MHD_NO) {
	    return_code = MHD_HTTP_UNAUTHORIZED;
	    error_page(&page, return_code, return_content, quiet_error);
	}
    } else {
	return_code = MHD_HTTP_UNAUTHORIZED;
	error_page(&page, return_code, return_content, quiet_error);
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
	setContentTypeHeader(response, return_content);
	ret = MHD_queue_basic_auth_fail_response (connection,
						  "monetdb",
						  response);
	MHD_destroy_response (response);

	return ret;
    }

    /*
     * Only start with setting up postprocessor after checking
     * for requirements. Otherwise the error responses will not
     * be created correct
     */
    if (*con_cls == NULL) {
	struct connection_info_struct *con_info;

	con_info = malloc (sizeof (struct connection_info_struct));
	if (con_info == NULL) return MHD_NO;
	con_info->answerstring = NULL;

	if (strcmp(method, "POST") == 0) {
	    /*
	     * [TODO]: handle situation where there was a request
	     *         without the correct contenttype. In that case
	     *         the next function will return NULL
	     */
	    con_info->postprocessor =
		MHD_create_post_processor (connection, POSTBUFFERSIZE,
					   iterate_post, (void *) con_info);

	    if (con_info->postprocessor == NULL) {
		free (con_info);
		return MHD_NO;
	    }
	    nr_of_uploading_clients++;
	    con_info->connectiontype = POST;
	    con_info->answercode = return_code;
	    //con_info->answerstring = 
	} else {
	    con_info->connectiontype = GET;
	}

	*con_cls = (void *) con_info;
	return MHD_YES;
    }

    /*
     * [TODO]: use MHD_OPTION_URI_LOG_CALLBACK to enable callback
     *         to check for original length of uri and return
     *         414 if too large for get request
     */
    
    /*
     * Now try to handle the actual request
     */
    handle_request_uri(url, &use_request_handler);

    switch(use_request_handler) {
    case HTTP_API_HANDLE_HELP:
	printf ("debug: help\n");
	help_page(&page, return_content);
	break;
    case HTTP_API_HANDLE_VERSION:
	printf ("debug: version\n");
	version_page(&page, return_content);
	break;
    case HTTP_API_HANDLE_MCLIENT:
	return_code = MHD_HTTP_NOT_IMPLEMENTED;
	error_page(&page, return_code, return_content, quiet_error);
	break;
    case HTTP_API_HANDLE_NOTFOUND:
	/*
	 * If path is not found, stop processing and
	 * return an error.
	 */
	return_code = MHD_HTTP_NOT_FOUND;
	error_page(&page, return_code, return_content, quiet_error);
	break;
    case HTTP_API_HANDLE_BADREQUEST:
	/*
	 * If problem with path, stop processing and
	 * return an error.
	 */
	return_code = MHD_HTTP_BAD_REQUEST;
	error_page(&page, return_code, return_content, quiet_error);
	break;
    case HTTP_API_HANDLE_STATEMENT:
	printf ("debug: statement\n");
	break;
    default:
	/*
	 * Unknown problem, stop processing
	 */
	printf ("debug: error\n");
	return_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
	error_page(&page, return_code, return_content, quiet_error);
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
	setContentTypeHeader(response, return_content);
	ret = MHD_queue_response (connection, return_code, response);
	MHD_destroy_response (response);

	return ret;
    }

    if (strcmp(method, MHD_HTTP_METHOD_GET) == 0) {
	MHD_get_connection_values (connection, MHD_GET_ARGUMENT_KIND,
				   handle_query_parameters,
				   &uqs);

	printf("debug: querycount: %i\n", uqs.statements_found);
    }

    if (strcmp(method, MHD_HTTP_METHOD_POST) == 0) {
	//if (nr_of_uploading_clients >= MAXCLIENTS) 
        //return send_page(connection, busypage, MHD_HTTP_SERVICE_UNAVAILABLE);
	printf("debug: handle post\n");
	struct connection_info_struct *con_info;
	
	MHD_get_connection_values (connection, MHD_GET_ARGUMENT_KIND,
				   handle_post_parameters,
				   NULL);

	con_info = *con_cls;

	if (*upload_data_size != 0) {
	    MHD_post_process (con_info->postprocessor, upload_data,
			      *upload_data_size);
	    *upload_data_size = 0;

	    return MHD_YES;
        } else {
	    /*
	     * [TODO]: make sure answerstring is initialized
	     */
	    if (con_info->answerstring != NULL) {
		query = con_info->answerstring;
		printf("debug: post query: %s\n", query);
	    }
	}
    }

    if(mock_database(username, query, &query_result) == 0) {
	/*
	 * Query succeeds. default return_code
	 */
	switch(return_content) {
	case RETURN_XML:
	    if (mock_render_xml(&page, &query_result) != 0) {
		return_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
	    }
	    break;
	case RETURN_JSON:
	    if (mock_render_json(&page, &query_result) != 0) {
		return_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
	    }
	    break;
	case RETURN_CSV:
	    if (mock_render_csv(&page, &query_result) != 0) {
		return_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
	    }
	    break;
	case RETURN_HTML:
	default:
	    if (mock_render_html(&page, &query_result) != 0) {
		return_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
	    }
	}

	if (return_code != MHD_HTTP_INTERNAL_SERVER_ERROR) { 
	    if (handle_http_cache) {
		if (mock_cache(&tag, page) == 0) {
		    if ((rhl.etag != NULL) &&
			(strcmp(rhl.etag, tag) == 0)) {
			return_code = MHD_HTTP_NOT_MODIFIED;
			free(page);
			page = (char *) malloc(strlen(EMPTY) *
					       sizeof(char));
			sprintf(page, EMPTY);
		    }
		} else {
		    return_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
		}
	    }
	}
    } else {
	return_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
    }

    if (uqs.query_statement != NULL) {
	free(uqs.query_statement);
    }
    free_headers(&rhl);

    if ((return_code != MHD_HTTP_OK) &&
	(return_code != MHD_HTTP_NOT_MODIFIED)) {
	if (page != NULL) free(page);
	error_page(&page, return_code, return_content, quiet_error);
    }
    response =
	MHD_create_response_from_buffer (strlen (page), (void *) page, 
					 MHD_RESPMEM_MUST_FREE);

    if (tag != NULL) {
	if (MHD_add_response_header (response, MHD_HTTP_HEADER_ETAG,
				     tag) == MHD_NO) {

	    printf("debug: add etag header failed\n");
	}
	free(tag);
    }

    setContentTypeHeader(response, return_content);
    ret = MHD_queue_response (connection, return_code, response);
    MHD_destroy_response (response);

    return ret;
}

void run_query_handler() {
    struct MHD_Daemon *daemon;
    int listen_port = getPort();

    daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, listen_port,
			       &handle_accept, NULL,
			       &handle_request, NULL,
			       MHD_OPTION_NOTIFY_COMPLETED, &request_completed,
			       NULL, MHD_OPTION_END);

    if (NULL == daemon)
	return;

    getchar ();

    MHD_stop_daemon (daemon);
}
