#include <stdio.h>
#include <string.h>
#include "CUnit/Basic.h"
#include "curl/curl.h"
#include <stdbool.h>

#include "libqueryhandler.h"
#include "libqueryhandler.c"
#include <microhttpd.h>

static int oneone;

struct CBC
{
    char *buf;
    size_t pos;
    size_t size;
};

struct return_headers
{
    int header_count;
    int content_length;
    bool has_date;
    char * etag;
};

static size_t
copyBuffer (void *ptr, size_t size, size_t nmemb, void *ctx)
{
    struct CBC *cbc = ctx;

    if (cbc->pos + size * nmemb > cbc->size)
	return 0;                   /* overflow */
    memcpy (&cbc->buf[cbc->pos], ptr, size * nmemb);
    cbc->pos += size * nmemb;
    return size * nmemb;
}

static int
curlExcessFound(CURL *c, curl_infotype type, char *data, size_t size, void *cls)
{
    static const char *excess_found = "Excess found";
    const size_t str_size = strlen (excess_found);

    if (CURLINFO_TEXT == type
	&& size >= str_size
	&& 0 == strncmp(excess_found, data, str_size))
	*(int *)cls = 1;
    return 0;
}

static size_t header_callback(char *buffer, size_t size,
                              size_t nitems, void *userdata)
{
    int etag_len;
    /* received header is nitems * size long in 'buffer' NOT ZERO TERMINATED */
    /* 'userdata' is set with CURLOPT_WRITEDATA */
    ((struct return_headers *) userdata)->header_count++;
    if (strncmp(buffer, "Date: ", strlen("Date: ")) == 0) {
	((struct return_headers *) userdata)->has_date = true;
    }
    if (strncmp(buffer, "Content-Length: ", strlen("Content-Length: ")) == 0) {
	((struct return_headers *) userdata)->content_length =
	    atoi(&buffer[strlen("Content-Length: ")]);
    }
    if (strncmp(buffer, "ETag: ", strlen("ETag: ")) == 0) {
	etag_len = strlen(buffer) - strlen("ETag: ") - 2;
	((struct return_headers *) userdata)->etag =
	    malloc(etag_len * sizeof(char));
	strncpy(((struct return_headers *) userdata)->etag,
		&buffer[strlen("ETag: ")],
		etag_len);
    }
    return nitems * size;
}

int initCurl(void)
{
    return 0;

}
 
int cleanCurl(void)
{
    return 0;
}

void testPost(void)
{
    struct MHD_Daemon *daemon;
    CURL *c;
    char buf[2048];
    struct CBC cbc;
    CURLcode errornum;
    int excess_found = 0;
    long resp;
    char *ct;
    struct return_headers rh = { 0, 0, false, NULL };

    cbc.buf = buf;
    cbc.size = 2048;
    cbc.pos = 0;

    int listen_port = getPort();
    CU_ASSERT(listen_port == 8888);

    daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, listen_port,
			       &handle_accept, NULL,
			       &handle_request, NULL,
			       MHD_OPTION_NOTIFY_COMPLETED, &request_completed,
			       NULL, MHD_OPTION_END);

    if (daemon == NULL)
	return;

    c = curl_easy_init ();
    curl_easy_setopt (c, CURLOPT_URL, "http://localhost:8888");
    curl_easy_setopt (c, CURLOPT_POSTFIELDS, "");
    curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
    curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
    curl_easy_setopt (c, CURLOPT_DEBUGFUNCTION, &curlExcessFound);
    curl_easy_setopt (c, CURLOPT_DEBUGDATA, &excess_found);
    curl_easy_setopt (c, CURLOPT_VERBOSE, 1);
    curl_easy_setopt (c, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
    curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 150L);
    curl_easy_setopt (c, CURLOPT_HEADERDATA, &rh);
    curl_easy_setopt (c, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt (c, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt (c, CURLOPT_USERPWD, "arjen:arjen");
    if (oneone)
	curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    else
	curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
    curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1);

    if ((errornum = curl_easy_perform (c)) != CURLE_OK) {
	CU_ASSERT(errornum != 0);

	curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &resp);
	CU_ASSERT(resp == MHD_HTTP_HTTP_VERSION_NOT_SUPPORTED);

	curl_easy_getinfo(c, CURLINFO_CONTENT_TYPE, &ct);
	CU_ASSERT_PTR_EQUAL(ct, NULL);

#if DEBUG
	fprintf (stderr,
		 "curl_easy_perform failed: `%s'\n",
		 curl_easy_strerror (errornum));
#endif
	curl_easy_cleanup (c);
	MHD_stop_daemon (daemon);
	return;
    }

    curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &resp);
    CU_ASSERT(resp == MHD_HTTP_OK);

    curl_easy_getinfo(c, CURLINFO_CONTENT_TYPE, &ct);
    CU_ASSERT_STRING_EQUAL(ct, ACCEPT_HTML);

    CU_ASSERT(rh.header_count == 6);
    CU_ASSERT_TRUE(rh.has_date);
    CU_ASSERT(rh.content_length == 14);
    CU_ASSERT_STRING_EQUAL(rh.etag, "ToCache");
    
    if (rh.etag != NULL) free(rh.etag);

    curl_easy_cleanup (c);
    MHD_stop_daemon (daemon);

    CU_ASSERT(cbc.pos == 14);
    CU_ASSERT(excess_found == 0);
}

void testPostRoot(void)
{
    struct MHD_Daemon *daemon;
    CURL *c;
    char buf[2048];
    struct CBC cbc;
    CURLcode errornum;
    int excess_found = 0;
    long resp;
    char *ct;
    struct return_headers rh = { 0, 0, false, NULL };

    cbc.buf = buf;
    cbc.size = 2048;
    cbc.pos = 0;

    int listen_port = getPort();
    CU_ASSERT(listen_port == 8888);

    daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, listen_port,
			       &handle_accept, NULL,
			       &handle_request, NULL,
			       MHD_OPTION_NOTIFY_COMPLETED, &request_completed,
			       NULL, MHD_OPTION_END);

    if (daemon == NULL)
	return;

    c = curl_easy_init ();
    curl_easy_setopt (c, CURLOPT_URL, "http://localhost:8888/");
    curl_easy_setopt (c, CURLOPT_POSTFIELDS, "");
    curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
    curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
    curl_easy_setopt (c, CURLOPT_DEBUGFUNCTION, &curlExcessFound);
    curl_easy_setopt (c, CURLOPT_DEBUGDATA, &excess_found);
    curl_easy_setopt (c, CURLOPT_VERBOSE, 1);
    curl_easy_setopt (c, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
    curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 150L);
    curl_easy_setopt (c, CURLOPT_HEADERDATA, &rh);
    curl_easy_setopt (c, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt (c, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt (c, CURLOPT_USERPWD, "arjen:arjen");
    if (oneone)
	curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    else
	curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
    curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1);

    if ((errornum = curl_easy_perform (c)) != CURLE_OK) {
	CU_ASSERT(errornum != 0);

	curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &resp);
	CU_ASSERT(resp == MHD_HTTP_HTTP_VERSION_NOT_SUPPORTED);

	curl_easy_getinfo(c, CURLINFO_CONTENT_TYPE, &ct);
	CU_ASSERT_PTR_EQUAL(ct, NULL);

#if DEBUG
	fprintf (stderr,
		 "curl_easy_perform failed: `%s'\n",
		 curl_easy_strerror (errornum));
#endif
	curl_easy_cleanup (c);
	MHD_stop_daemon (daemon);
	return;
    }

    curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &resp);
    CU_ASSERT(resp == MHD_HTTP_OK);

    curl_easy_getinfo(c, CURLINFO_CONTENT_TYPE, &ct);
    CU_ASSERT_STRING_EQUAL(ct, ACCEPT_HTML);

    CU_ASSERT(rh.header_count == 6);
    CU_ASSERT_TRUE(rh.has_date);
    CU_ASSERT(rh.content_length == 14);
    CU_ASSERT_STRING_EQUAL(rh.etag, "ToCache");

    if (rh.etag != NULL) free(rh.etag);

    curl_easy_cleanup (c);
    MHD_stop_daemon (daemon);

    CU_ASSERT(cbc.pos == 14);
    CU_ASSERT(excess_found == 0);
}

int main()
{
    oneone = 1;

    CU_pSuite pSuite = NULL;
 
    if (CU_initialize_registry() != CUE_SUCCESS)
	return CU_get_error();
 
    pSuite = CU_add_suite("CurlFuncs", initCurl, cleanCurl);
    if (pSuite == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test post", testPost) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test post root", testPostRoot) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }
    /*
    if (CU_add_test(pSuite, "test http 1.0 auth", testHttp10Auth) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test http 1.0 auth root", testHttp10AuthRoot) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test http 1.0 wrong password", testHttp10User) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test http 1.0 wrong password root", testHttp10UserRoot) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test http 1.0 wrong user", testHttp10Password) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test http 1.0 wrong user root", testHttp10PasswordRoot) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test http 1.0  both", testHttp10Both) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test with http 10 both root", testHttp10BothRoot) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test http 1.0  wrong", testHttp10Wrong) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test with http 10 wrong root", testHttp10WrongRoot) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }
    */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    /*
      CU_set_output_filename( "cunit_testall" );
      CU_list_tests_to_file();
      CU_automated_run_tests();
    */
    CU_cleanup_registry();
    return CU_get_error();
}
