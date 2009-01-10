
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

#ifndef _CVPN_COMM_H
#define _CVPN_COMM_H

#include "iface.h"
#include "utils.h"
#include "sq.h"

#include <stdint.h>

#include <openssl/ssl.h>

#include <map>
#include <set>
#include <queue>
#include <string>
using namespace std;

class connection
{
public:
	int id; //not meant to be modified.
	int fd; //set to -1 if there's no socket

	void index();
	void deindex();

	void set_fd (int i) {
		if (i < 0) return;
		deindex();
		fd = i;
		index();
	}

	void unset_fd() {
		deindex();
		fd = -1;
	}

	int state;

#define cs_inactive 0
#define cs_retry_timeout 1
#define cs_connecting 2
#define cs_ssl_connecting 3
#define cs_accepting 4
#define cs_closing 5
#define cs_active 6

	uint64_t last_retry; //last connection retry

	unsigned int ping; //cached ping
	uint8_t sent_ping_id;
	uint64_t sent_ping_time;
	//ping is on the way, if sent_ping_time==last_ping

	//all routes the peer reported
	class remote_route
	{
	public:
		unsigned int ping, dist;
		remote_route (unsigned int p, unsigned int d) {
			ping = p;
			dist = d;
		}
	};
	map<hwaddr, remote_route> remote_routes;

	explicit inline connection (int ID) {
		id = ID;
		fd = -1;
		ping = 1; //measure the distance at least
		ssl = 0; //point at nothing.
		bio = 0;
		last_ping = 0;
		cached_header.type = 0;
		sending_from_data_q = false;
		route_overflow = false;
	}

	connection (); //this is supposed to fail, always use c(ID)

	/*
	 * packet handling/sending functions. Those handle the endianiness.
	 */

	void handle_packet (void*buf, int len);
	void handle_broadcast_packet (uint32_t id, void*buf, int len);
	void handle_route_set (uint8_t*data, int n);
	void handle_route_diff (uint8_t*data, int n);
	void handle_ping (uint8_t id);
	void handle_pong (uint8_t id);
	void handle_route_request ();

	void write_packet (void*buf, int len);
	void write_broadcast_packet (uint32_t id, void*buf, int len);
	void write_route_set (uint8_t*data, int n);
	void write_route_diff (uint8_t*data, int n);
	void write_ping (uint8_t id);
	void write_pong (uint8_t id);
	void write_route_request ();

	/*
	 * those functions are called by polling interface to do specific stuff
	 */

	squeue recv_q;
	deque<pbuffer> proto_q, data_q;
	bool sending_from_data_q;

	struct {
		uint8_t type;
		uint8_t special;
		uint16_t size;
	} cached_header;

	void try_parse_input();

	pbuffer& new_proto ();
	pbuffer& new_data ();

	bool try_read();
	bool try_write(); //both called by try_data(); dont use directly

	void try_data();

	void try_accept();
	void try_connect();
	void try_ssl_connect();
	void try_close();

	void start_connect();
	void start_accept();
	void send_ping();

	void activate();
	void disconnect();
	void reset(); //hard socket disconnect.

	int handle_ssl_error (int);

	/*
	 * direct poll interface
	 */

	void poll_simple();
	void poll_read();
	void poll_write();

	/*
	 * update the stuff
	 */

	void periodic_update();

	/*
	 * address that we should try to reconnect
	 */

	string address;

	/*
	 * operation timings
	 */

	static int timeout;
	static int keepalive;
	static int retry;
	uint64_t last_ping;

	/*
	 * SSL data
	 */

	SSL*ssl; //should be 0 when not used
	BIO*bio;

	int alloc_ssl();
	void dealloc_ssl();

	/*
	 * queue management
	 */

	static int mtu;
	static int max_waiting_data_packets;
	static int max_waiting_proto_packets;
	static int max_remote_routes;

	inline bool can_write_data() {
		return data_q.size() < max_waiting_data_packets;
	}

	inline bool can_write_proto() {
		return proto_q.size() < max_waiting_proto_packets;
	}

	/*
	 * route information size management
	 */

	bool route_overflow;
	void handle_route_overflow();
};

void comm_listener_poll (int fd);

int comm_init();
int comm_shutdown();

void comm_periodic_update();

void comm_broadcast_route_update (uint8_t*data, int n);

map<int, int>& comm_connection_index();
map<int, connection>& comm_connections();
set<int>& comm_listeners();

#endif

