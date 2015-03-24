typedef int db_resultset;

int mock_authenticate(const char * username, const char * password);
int mock_database(const char * username, const char * query, db_resultset * query_result);
int mock_render_json(char ** result_json, const db_resultset * query_result);
int mock_render_csv(char ** result_csv, const db_resultset * query_result);
int mock_render_xml(char ** result_xml, const db_resultset * query_result);
int mock_render_html(char ** result_html, const db_resultset * query_result);
int mock_cache(const char ** tag, const char * page);
