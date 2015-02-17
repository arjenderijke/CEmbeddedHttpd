#ifndef LIBQUERYHANDLER_H_INCLUDED
#define LIBQUERYHANDLER_H_INCLUDED

#include <stdbool.h>

#define HTTP_API_HELP_PATH "help"
#define HTTP_API_VERSION_PATH "version"
#define HTTP_API_STATEMENT_PATH "statement"
#define HTTP_API_LANGUAGE_PATH "language"
#define HTTP_API_MCLIENT_PATH "mclient"

#define HTTP_QUERY_VERSION 'v'
#define HTTP_QUERY_STATEMENT 's'
#define HTTP_QUERY_LANGUAGE 'l'
#define HTTP_QUERY_MCLIENT 'm'

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

struct connection_info_struct
{
    int connectiontype;
    char *answerstring;
    struct MHD_PostProcessor *postprocessor; 
};

struct url_query_statements {
    int statements_found;
    int statements_error;
    bool query_version;
    int query_language;
    char * query_statement;
    bool query_mclient;
};

struct request_header_list {
    int headers_found;
    int headers_error;
    char * host;
    char * user_agent;
    char * accept;
};

void run_query_handler();
#endif /* LIBQUERYHANDLER_H_INCLUDED */
