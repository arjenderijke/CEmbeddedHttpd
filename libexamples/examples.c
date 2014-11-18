#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <microhttpd.h>

#include "libexamples.h"
#include "libmicro.h"

#define PORT 8888

void run_example(const int example) {
    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
				   &answer_example3, NULL, MHD_OPTION_END);
    if (NULL == daemon)
	return 1;

    getchar ();

    MHD_stop_daemon (daemon);
}

