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

void testVersion(void)
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
    curl_easy_setopt (c, CURLOPT_URL, "http://localhost:8888/version");
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
    CU_ASSERT(rh.content_length == 84);
    CU_ASSERT_PTR_EQUAL(rh.etag, NULL);

    if (rh.etag != NULL) free(rh.etag);

    curl_easy_cleanup (c);
    MHD_stop_daemon (daemon);

    CU_ASSERT(cbc.pos == 84);
    CU_ASSERT(excess_found == 0);
}

void testVersionRoot(void)
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
    curl_easy_setopt (c, CURLOPT_URL, "http://localhost:8888/version/");
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
	CU_ASSERT(resp == MHD_HTTP_BAD_REQUEST);

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

void testNoParams(void)
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
    curl_easy_setopt (c, CURLOPT_URL, "http://localhost:8888/?");
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
	CU_ASSERT(resp == MHD_HTTP_BAD_REQUEST);

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

void testVersionShort(void)
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
    curl_easy_setopt (c, CURLOPT_URL, "http://localhost:8888/?v");
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
    CU_ASSERT(rh.content_length == 84);
    CU_ASSERT_PTR_EQUAL(rh.etag, NULL);

    if (rh.etag != NULL) free(rh.etag);

    curl_easy_cleanup (c);
    MHD_stop_daemon (daemon);

    CU_ASSERT(cbc.pos == 84);
    CU_ASSERT(excess_found == 0);
}

void testVerShortIs(void)
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
    curl_easy_setopt (c, CURLOPT_URL, "http://localhost:8888/?v=");
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
    CU_ASSERT(rh.content_length == 84);
    CU_ASSERT_PTR_EQUAL(rh.etag, NULL);

    if (rh.etag != NULL) free(rh.etag);

    curl_easy_cleanup (c);
    MHD_stop_daemon (daemon);

    CU_ASSERT(cbc.pos == 84);
    CU_ASSERT(excess_found == 0);
}

void testVerShortIsVal(void)
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
    curl_easy_setopt (c, CURLOPT_URL, "http://localhost:8888/?v=1");
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
    CU_ASSERT(rh.content_length == 84);
    CU_ASSERT_PTR_EQUAL(rh.etag, NULL);

    if (rh.etag != NULL) free(rh.etag);

    curl_easy_cleanup (c);
    MHD_stop_daemon (daemon);

    CU_ASSERT(cbc.pos == 84);
    CU_ASSERT(excess_found == 0);
}

void testVerShortIsStr1(void)
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
    curl_easy_setopt (c, CURLOPT_URL, "http://localhost:8888/?v=version");
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
    CU_ASSERT(rh.content_length == 84);
    CU_ASSERT_PTR_EQUAL(rh.etag, NULL);

    if (rh.etag != NULL) free(rh.etag);

    curl_easy_cleanup (c);
    MHD_stop_daemon (daemon);

    CU_ASSERT(cbc.pos == 84);
    CU_ASSERT(excess_found == 0);
}

void testVerShortIsStr2(void)
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
    curl_easy_setopt (c, CURLOPT_URL, "http://localhost:8888/?v=hello");
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
    CU_ASSERT(rh.content_length == 84);
    CU_ASSERT_PTR_EQUAL(rh.etag, NULL);

    if (rh.etag != NULL) free(rh.etag);

    curl_easy_cleanup (c);
    MHD_stop_daemon (daemon);

    CU_ASSERT(cbc.pos == 84);
    CU_ASSERT(excess_found == 0);
}

void testHelp(void)
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
    curl_easy_setopt (c, CURLOPT_URL, "http://localhost:8888/help");
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
    CU_ASSERT(rh.content_length == 89);
    CU_ASSERT_PTR_EQUAL(rh.etag, NULL);

    if (rh.etag != NULL) free(rh.etag);

    curl_easy_cleanup (c);
    MHD_stop_daemon (daemon);

    CU_ASSERT(cbc.pos == 89);
    CU_ASSERT(excess_found == 0);
}

void testHelpShort(void)
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
    curl_easy_setopt (c, CURLOPT_URL, "http://localhost:8888/?h");
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
    CU_ASSERT(rh.content_length == 89);
    CU_ASSERT_PTR_EQUAL(rh.etag, NULL);

    if (rh.etag != NULL) free(rh.etag);

    curl_easy_cleanup (c);
    MHD_stop_daemon (daemon);

    CU_ASSERT(cbc.pos == 89);
    CU_ASSERT(excess_found == 0);
}

void testHelpShortIs(void)
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
    curl_easy_setopt (c, CURLOPT_URL, "http://localhost:8888/?h=");
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
    CU_ASSERT(rh.content_length == 89);
    CU_ASSERT_PTR_EQUAL(rh.etag, NULL);

    if (rh.etag != NULL) free(rh.etag);

    curl_easy_cleanup (c);
    MHD_stop_daemon (daemon);

    CU_ASSERT(cbc.pos == 89);
    CU_ASSERT(excess_found == 0);
}

void testHelpShortIsVal(void)
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
    curl_easy_setopt (c, CURLOPT_URL, "http://localhost:8888/?h=1");
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
    CU_ASSERT(rh.content_length == 89);
    CU_ASSERT_PTR_EQUAL(rh.etag, NULL);

    if (rh.etag != NULL) free(rh.etag);

    curl_easy_cleanup (c);
    MHD_stop_daemon (daemon);

    CU_ASSERT(cbc.pos == 89);
    CU_ASSERT(excess_found == 0);
}

void testHelpShortIsStr1(void)
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
    curl_easy_setopt (c, CURLOPT_URL, "http://localhost:8888/?h=help");
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
    CU_ASSERT(rh.content_length == 89);
    CU_ASSERT_PTR_EQUAL(rh.etag, NULL);

    if (rh.etag != NULL) free(rh.etag);

    curl_easy_cleanup (c);
    MHD_stop_daemon (daemon);

    CU_ASSERT(cbc.pos == 89);
    CU_ASSERT(excess_found == 0);
}

void testHelpShortIsStr2(void)
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
    curl_easy_setopt (c, CURLOPT_URL, "http://localhost:8888/?h=hello");
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
    CU_ASSERT(rh.content_length == 89);
    CU_ASSERT_PTR_EQUAL(rh.etag, NULL);

    if (rh.etag != NULL) free(rh.etag);

    curl_easy_cleanup (c);
    MHD_stop_daemon (daemon);

    CU_ASSERT(cbc.pos == 89);
    CU_ASSERT(excess_found == 0);
}

int main()
{
    oneone = 1;

    CU_pSuite pSuite = NULL;
 
    if (CU_initialize_registry() != CUE_SUCCESS)
	return CU_get_error();
 
    pSuite = CU_add_suite("Basic urls", initCurl, cleanCurl);
    if (pSuite == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test version", testVersion) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test version root", testVersionRoot) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test missing params", testNoParams) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test version short", testVersionShort) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test ver short eq", testVerShortIs) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test ver short eq val", testVerShortIsVal) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test ver short eq str1", testVerShortIsStr1) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test ver short eq str2", testVerShortIsStr2) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test help", testHelp) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test help short", testHelpShort) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test help short eq", testHelpShortIs) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test help short eq val", testHelpShortIsVal) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test help short eq str1", testHelpShortIsStr1) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

    if (CU_add_test(pSuite, "test help short eq str2", testHelpShortIsStr2) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }

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