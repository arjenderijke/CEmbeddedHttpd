#if !HAVE_CONFIG_H
#include <config.h>
#endif

#include "libquerymock.h"
#include <microhttpd.h>

int
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

int 
mock_database(const char * username, const char * query, db_resultset * query_result)
{
    /*
     * [TODO]: login to database with user "username". Run query "query"
     *         and get resultset in "query_result"
     */
    *query_result = 1;
    return 0;
}

int
mock_render_json(char ** result_json, const db_resultset * query_result)
{
    char * renderjson = "{ \"column\" : \"value\" }";
    
    if (*query_result == 1) {
	*result_json = (char *) malloc(strlen(renderjson) * sizeof(char));
	sprintf(*result_json, renderjson);
    }
    
    return 0;
}

int
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

int
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

int
mock_render_html(char ** result_html, const db_resultset * query_result)
{
    char * renderhtml = "<html>"
	"</html>\n";
    
    if (*query_result == 1) {
	*result_html = (char *) malloc(strlen(renderhtml) * sizeof(char));
	sprintf(*result_html, renderhtml);
    }
    
    return 0;
}

int
mock_cache(const char ** tag, const char * page)
{
    char * mocktag = "ToCache";
    *tag = (char *) malloc(strlen(mocktag) * sizeof(char));
    strcpy(*tag, mocktag);
    return 0;
}
