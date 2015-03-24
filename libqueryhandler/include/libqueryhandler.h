#ifndef LIBQUERYHANDLER_H_INCLUDED
#define LIBQUERYHANDLER_H_INCLUDED

#include <stdbool.h>

#define HTTP_API_HELP_PATH "help"
#define HTTP_API_VERSION_PATH "version"
#define HTTP_API_STATEMENT_PATH "statement"
#define HTTP_API_LANGUAGE_PATH "language"
#define HTTP_API_MCLIENT_PATH "mclient"
#define HTTP_API_FORMAT_PATH "format"

/*
 * For now use short option "h" for help. Later we might use 
 * it for the host value in a redirect/proxy setup.
 */
#define HTTP_QUERY_VERSION 'v'
#define HTTP_QUERY_HELP 'h'
#define HTTP_QUERY_STATEMENT 's'
#define HTTP_QUERY_LANGUAGE 'l'
#define HTTP_QUERY_MCLIENT 'm'
#define HTTP_QUERY_FORMAT 'f'

#define QUERY_LANGUAGE_UNKNOWN -1
#define QUERY_LANGUAGE_MAL 0
#define QUERY_LANGUAGE_SQL 1

#define HTTP_API_HANDLE_ERROR -1
#define HTTP_API_HANDLE_HELP 0
#define HTTP_API_HANDLE_VERSION 1
#define HTTP_API_HANDLE_STATEMENT 2
#define HTTP_API_HANDLE_MCLIENT 3
#define HTTP_API_HANDLE_NOTFOUND 4
#define HTTP_API_HANDLE_BADREQUEST 5

#define ACCEPT_HTML "text/html"
#define ACCEPT_XML "text/xml"
#define ACCEPT_JSON "application/json"
#define ACCEPT_CSV "text/csv"
#define ACCEPT_ALL "*/*"

#define QUERY_FORMAT_HTML "html"
#define QUERY_FORMAT_XML "xml"
#define QUERY_FORMAT_CSV "csv"
#define QUERY_FORMAT_JSON "json"

#define RETURN_HTML 0
#define RETURN_XML 1
#define RETURN_JSON 2
#define RETURN_CSV 3

#define EMPTY ""

typedef int db_resultset;

struct connection_info_struct
{
    int connectiontype;
    char *answerstring;
    struct MHD_PostProcessor *postprocessor; 
    int answercode;
};

struct url_query_statements {
    int statements_found;
    int statements_error;
    bool query_version;
    bool query_help;
    int query_language;
    char * query_statement;
    bool query_mclient;
    int query_format;
};

struct request_header_list {
    int headers_found;
    int headers_error;
    char * host;
    char * user_agent;
    char * accept;
    char * etag;
};

void run_query_handler();
#endif /* LIBQUERYHANDLER_H_INCLUDED */
