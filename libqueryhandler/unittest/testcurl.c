#include <stdio.h>
#include <string.h>
#include "CUnit/Basic.h"
#include "curl/curl.h"

#include "libqueryhandler.h"
#include "libqueryhandler.c"
#include <microhttpd.h>

struct CBC
{
  char *buf;
  size_t pos;
  size_t size;
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

int initCurl(void)
{
    return 0;

}
 
int cleanCurl(void)
{
    return 0;
}

void testConnect(void)
{
    struct MHD_Daemon *daemon;
    CURL *c;
    char buf[2048];
    struct CBC cbc;
    CURLcode errornum;
    int excess_found = 0;

    cbc.buf = buf;
    cbc.size = 2048;
    cbc.pos = 0;

    int listen_port = getPort();

    daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, listen_port,
			       &handle_accept, NULL,
			       &handle_request, NULL,
			       MHD_OPTION_NOTIFY_COMPLETED, &request_completed,
			       NULL, MHD_OPTION_END);

    if (daemon == NULL)
	return;

    c = curl_easy_init ();
    curl_easy_setopt (c, CURLOPT_URL, "http://127.0.0.1:8888/hello_world");
    curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
    curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
    curl_easy_setopt (c, CURLOPT_DEBUGFUNCTION, &curlExcessFound);
    curl_easy_setopt (c, CURLOPT_DEBUGDATA, &excess_found);
    curl_easy_setopt (c, CURLOPT_VERBOSE, 1);
    curl_easy_setopt (c, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
    curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 150L);
    curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1);
    if (CURLE_OK != (errornum = curl_easy_perform (c))) {
	fprintf (stderr,
		 "curl_easy_perform failed: `%s'\n",
		 curl_easy_strerror (errornum));
	curl_easy_cleanup (c);
	MHD_stop_daemon (daemon);
	return;
    }
    curl_easy_cleanup (c);
    MHD_stop_daemon (daemon);

    CU_ASSERT(cbc.pos == 0);
    CU_ASSERT(excess_found == 0);
}

int main()
{
    CU_pSuite pSuite = NULL;
 
    if (CU_initialize_registry() != CUE_SUCCESS)
	return CU_get_error();
 
    pSuite = CU_add_suite("CurlFuncs", initCurl, cleanCurl);
    if (pSuite == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }
 
    if (CU_add_test(pSuite, "test with curl", testConnect) == NULL) {
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
