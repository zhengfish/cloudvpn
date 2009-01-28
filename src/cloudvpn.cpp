
/*
 * CloudVPN
 *
 * This program is a free software: You can redistribute and/or modify it
 * under the terms of GNU GPLv3 license, or any later version of the license.
 * The program is distributed in a good hope it will be useful, but without
 * any warranty - see the aforementioned license for more details.
 * You should have received a copy of the license along with this program;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#include "cloudvpn.h"

#include "sq.h"
#include "log.h"
#include "comm.h"
#include "conf.h"
#include "poll.h"
#include "iface.h"
#include "route.h"
#include "utils.h"
#include "status.h"
#include "security.h"
#include "timestamp.h"

#include <unistd.h>

int g_terminate = 0;

int run_cloudvpn (int argc, char**argv)
{
	int ret = 0;
	int heartbeat_usec = 50000; //20Hz is ok by default
	uint64_t last_beat = 0;

	Log_info ("cloudvpn: starting");
	Log (FATAL + 1, "You are using CloudVPN, which is Free software.");
	Log (FATAL + 1, "For more information please see the GNU GPL license,");
	Log (FATAL + 1, "which you should have received along with this program.");

	setup_sighandler();

	/*
	 * initialization
	 */

	if (!config_parse (argc, argv) ) {
		Log_error ("failed to parse config, terminating.");
		ret = 1;
		goto failed_config;
	}

	if (!config_get_int ("heartbeat", heartbeat_usec) )
		heartbeat_usec = 50000;
	Log_info ("heartbeat is set to %d usec", heartbeat_usec);

	timestamp_update(); //get initial timestamp

	status_init();

	route_init();

	squeue_init();

	if (poll_init() ) {
		Log_fatal ("poll initialization failed");
		ret = 2;
		goto failed_poll;
	}

	if (iface_create() ) {
		Log_fatal ("local interface initialization failed");
		ret = 3;
		goto failed_iface;
	}

	if (do_memlock() ) {
		Log_fatal ("locking process to memory failed");
		ret = 4;
		goto failed_comm;
	}

	if (comm_init() ) {
		Log_fatal ("communication initialization failed");
		ret = 5;
		goto failed_comm;
	}

	if (do_local_security() ) {
		Log_fatal ("local security failed");
		ret = 6;
		goto failed_sec;
	}

	/*
	 * main loop
	 */

	Log_info ("initialization complete, entering main loop");

	last_beat = 0; //update immediately.

	while (!g_terminate) {

		timestamp_update();

		if ( (timestamp() - last_beat) 
			< (unsigned int) heartbeat_usec) {
			//poll more stuff
			poll_wait_for_event (heartbeat_usec
			                     - timestamp()
			                     + last_beat);
			continue;
		}

		last_beat = timestamp();

		route_update();
		comm_periodic_update();

		route_update();
		status_try_export();
	}

	/*
	 * deinitialization
	 */

	Log_info ("shutting down");

failed_sec:

	comm_shutdown();

failed_comm:

	iface_destroy();

failed_iface:

	if (poll_deinit() )
		Log_warn ("poll_deinit somehow failed!");

failed_poll:
failed_config:
	if (!ret) Log_info ("cloudvpn: exiting gracefully");
	else Log_error ("cloudvpn: exiting with code %d", ret);
	return ret;
}

void kill_cloudvpn (int signum)
{
	Log_info ("cloudvpn: killed by signal %d, will terminate", signum);
	g_terminate = 1;
}

