#ident "@(#)protocol.c	1.7"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1996 Computer Associates International, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <signal.h>
#include <errno.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <net/if.h>
#include <sys/sockio.h>
#include <netdb.h>
#include <stropts.h>
#include <poll.h>
#include <syslog.h>
#include <sys/stat.h>
#include <time.h>
#include <arpa/dhcp.h>
#include <aas/aas.h>
#include "dhcpd.h"
#include "proto.h"
#include "pathnames.h"

static u_char magic_cookie[] = OPT_COOKIE;

/*
 * The Request structure stores information about a client request.
 */

typedef struct Request {
	struct dhcp *pkt;	/* the request packet */
	int pkt_len;		/* length of packet */
	int pkt_buf_size;	/* size of packet buffer */
	/*
	 * ifinfo contains information about the interface on
	 * which the packet was received, including the server's
	 * address(es) on that interface.
	 */
	struct ifreq_all ifinfo;
	/*
	 * The following things are taken from the options portion of the
	 * request (those that are pointers just point into the packet).
	 * For each of these, there is a corresponding bit in the
	 * present field which indicates that the option was present in
	 * the request.
	 */
	u_short present;	/* fields present -- see below */
	u_char msg_type;	/* DHCP message type */
	u_char opt_overload;	/* option overload value */
	struct in_addr req_addr;	/* requested address */
	u_long lease;		/* requested lease time */
	u_char *client_id;	/* client identifier */
	int client_id_len;	/* length of above */
	struct in_addr server_id;	/* server identifier */
	u_char *param_req;	/* parameter request list */
	int param_req_len;	/* length of above */
	u_short max_msg_size;	/* maximum message size */
	char *opt_boot_file;	/* boot file (from option) */
	int opt_boot_file_len;	/* length of above */
	/*
	 * The following point to matched database entries.  If there is
	 * no match for a particular item or that item is not specified
	 * in the packet, the corresponding pointer is NULL.
	 */
	Client *client;
	UserClass *user_class;
	VendorClass *vendor_class;
	Subnet *subnet;
	/*
	 * The following are set based on the configuration for this
	 * client and the boot file name supplied by the client.
	 * These represent the boot file being supplied.
	 */
	char *tftpdir, *bootdir, *bootfile;
} Request;

static Request request;

/*
 * Values for present field
 */

#define PR_MSG_TYPE		0x0001
#define PR_OPT_OVERLOAD		0x0002
#define PR_REQ_ADDR		0x0004
#define PR_LEASE		0x0008
#define PR_CLIENT_ID		0x0010
#define PR_SERVER_ID		0x0020
#define PR_PARAM_REQ		0x0040
#define PR_MAX_MSG_SIZE		0x0080
#define PR_BOOT_FILE		0x0100

/*
 * The PRESENT macro checks the above bits.
 */

#define PRESENT(req, fld)	((req)->present & (fld))

/*
 * The SATOIN macro gives a pointer to the sin_addr portion of a sockaddr_in
 * given a sockaddr pointer.
 */

#define SATOIN(sa)	(&((struct sockaddr_in *) (sa))->sin_addr)

static int endpt;		/* UDP endpoint we are listening on */

/*
 * Client and sevrer ports
 */

static u_short bootps_port;
static u_short bootpc_port;

/*
 * Macros for padding and unpadding lease values
 */

#define LEASE_PAD(lease)	\
	((u_long) ((double) (lease) * server_params.lease_pad + 0.5))

#define LEASE_UNPAD(lease)	\
	((u_long) ((double) (lease) / server_params.lease_pad + 0.5))

/*
 * aas_conn references the AAS connection.  If NULL, no connection exists.
 */

static AasConnection *aas_conn = NULL;

/*
 * ip_fd -- file descriptor for IP device
 */

static int ip_fd;

/*
 * config_time -- modification time on the configuration file
 */

static time_t config_time;

/*
 * PKT_BUF_SIZE is the initial size of the packet buffer.  It must
 * be large enough to hold any reasonable request from a client.
 * 2K is way more than enough.
 */

#define PKT_BUF_SIZE	2048

/* Local functions */
static void check_lost_aas_conn(void);

/*
 * merge_options -- merge options into a merged list
 * merge_options is used to construct the list of options that correspond
 * to a client request.  The given list of options is merged into the
 * given merged list.  If a given option is already present in the merged
 * list, the new setting is ignored.  Therefore, lists must be merged in
 * decreasing order of precedence.  *merged must be NULL on the first call
 * for a given merged list.
 */

static void
merge_options(OptionSetting **merged, OptionSetting *list)
{
	OptionSetting *osp, *mosp, **link;

	for (osp = list; osp; osp = osp->next) {
		link = merged;
		for (mosp = *merged; mosp; mosp = mosp->merge) {
			if (mosp->opt == osp->opt) {
				break;
			}
			link = &mosp->merge;
		}
		/*
		 * If mosp is NULL, the option isn't in the merged list yet,
		 * so add it at the end.
		 */
		if (!mosp) {
			osp->merge = NULL;
			*link = osp;
		}
	}
}

/*
 * proc_options -- scan an option area for important things and fill
 * in Request structure.
 * Returns 1 if ok or 0 if options area is malformed.
 */

static int
proc_options(Request *req, u_char *optbuf, int len)
{
	u_char *p, *ep, *valp, val;
	int optlen, totlen;

	for (p = optbuf, ep = optbuf + len; p < ep; p += totlen) {
		/*
		 * Process PAD and END separately, since they don't have
		 * a length byte.
		 */
		switch (*p) {
		case OPT_PAD:
			totlen = 1;
			continue;
		case OPT_END:
			return 1;
		}

		/*
		 * Make sure length fits in remaining space (add 2 to count
		 * the code & length bytes).
		 */
		
		optlen = *(p + 1);
		totlen = optlen + 2;
		if (totlen > ep - p) {
			report(LOG_WARNING, "Incomplete option; ignoring...");
			break;
		}
		valp = p + 2;
		val = *valp;
		
		switch (*p) {

		case OPT_MSG_TYPE:
			if (optlen != 1) {
				report(LOG_WARNING, "Bad length for message type option; discarding...");
				return 0;
			}
			/*
			 * The actual message type value is checked in
			 * process_request.
			 */
			req->present |= PR_MSG_TYPE;
			req->msg_type = val;
			if (debug > 1) {
				report(LOG_INFO, "  Message type: %d", val);
			}
			break;
		
		case OPT_OVERLOAD:
			if (optlen != 1) {
				report(LOG_WARNING, "Bad length for option overload; discarding...");
				return 0;
			}
			if (val != OVERLOAD_FILE && val != OVERLOAD_SNAME
			    && val != OVERLOAD_BOTH) {
				report(LOG_WARNING, "Bad value for option overload; discarding...");
				return 0;
			}
			req->present |= PR_OPT_OVERLOAD;
			req->opt_overload = val;
			if (debug > 1) {
				report(LOG_INFO, "  Option overload value: %d",
					val);
			}
			break;
		
		case OPT_REQ_ADDR:
			if (optlen != sizeof(req->req_addr)) {
				report(LOG_WARNING, "Bad length for requested IP address; discarding...");
				return 0;
			}
			req->present |= PR_REQ_ADDR;
			memcpy(&req->req_addr, valp, optlen);
			if (debug > 1) {
				report(LOG_INFO, "  Requested address: %s",
					inet_ntoa(req->req_addr));
			}
			break;
		
		case OPT_LEASE_TIME:
			if (optlen != sizeof(req->lease)) {
				report(LOG_WARNING, "Bad length for lease time; ignoring...");
				break;
			}
			req->present |= PR_LEASE;
			memcpy(&req->lease, valp, optlen);
			req->lease = ntohl(req->lease);
			if (debug > 1) {
				report(LOG_INFO, "  Lease: %lu", req->lease);
			}
			break;
		
		case OPT_CLIENT_ID:
			if (optlen < MIN_CLIENT_ID_LEN) {
				report(LOG_WARNING, "Bad length for client identifier; discarding...");
				return 0;
			}
			req->present |= PR_CLIENT_ID;
			req->client_id = valp;
			req->client_id_len = optlen;
			if (debug) {
				report(LOG_INFO, "  Client ID: 0x%s",
					binary2string(valp, optlen));
			}
			break;

		case OPT_SERVER_ID:
			if (optlen != sizeof(req->server_id)) {
				report(LOG_WARNING, "Bad length for server identifier; discarding...");
				return 0;
			}
			req->present |= PR_SERVER_ID;
			memcpy(&req->server_id, valp, optlen);
			if (debug > 1) {
				report(LOG_INFO, "  Server ID: %s",
					inet_ntoa(req->server_id));
			}
			break;
		
		case OPT_PARAM_REQ:
			if (optlen < 1) {
				report(LOG_WARNING, "Bad length for parameter request list; ignoring...");
				break;
			}
			req->present |= PR_PARAM_REQ;
			req->param_req = valp;
			req->param_req_len = optlen;
			if (debug > 1) {
				report(LOG_INFO, "  Parameter request list: 0x%s",
					binary2string(valp, optlen));
			}
			break;

		case OPT_MAX_DHCP_MSG_SIZE:
			if (optlen != sizeof(req->max_msg_size)) {
				report(LOG_WARNING, "Bad length for maximum DHCP message size; ignoring...");
				break;
			}
			req->present |= PR_MAX_MSG_SIZE;
			memcpy(&req->max_msg_size, valp, optlen);
			req->max_msg_size = ntohs(req->max_msg_size);
			if (debug > 1) {
				report(LOG_INFO, "  Maximum message size: %d",
					req->max_msg_size);
			}
			break;

		case OPT_BOOT_FILE:
			/*
			 * Since this option is an ASCII string, we have
			 * to remove trailing nulls.
			 */
			for (; optlen > 0; optlen--) {
				if (p[optlen - 1] != '\0') {
					break;
				}
			}
			if (optlen < 1) {
				report(LOG_WARNING, "Bad length for boot file; ignoring...");
				break;
			}
			req->present |= PR_BOOT_FILE;
			req->opt_boot_file = (char *) valp;
			req->opt_boot_file_len = optlen;
			if (debug > 1) {
				report(LOG_WARNING, "  Boot file (option): %.*s",
					optlen, valp);
			}
			break;
		
		case OPT_USER_CLASS:
			/*
			 * Since this option is an ASCII string, we have
			 * to remove trailing nulls.
			 */
			for (; optlen > 0; optlen--) {
				if (valp[optlen - 1] != '\0') {
					break;
				}
			}
			if (optlen < 1) {
				report(LOG_WARNING, "Bad length for user class; ignoring...");
				break;
			}
			if (debug > 1) {
				report(LOG_WARNING, "  User class: %.*s",
					optlen, valp);
			}
			/*
			 * For this option, we just look up the user class
			 * and save a pointer to it if it's in the database.
			 */
			req->user_class = lookup_user_class((char *) valp,
				optlen);
			if (!req->user_class) {
				report(LOG_WARNING, "Unknown user class %.*s",
					optlen, valp);
			}
			break;
		
		case OPT_VENDOR_CLASS:
			if (optlen < 1) {
				report(LOG_WARNING, "Bad length for vendor class.");
				return 0;
			}
			if (debug > 1) {
				report(LOG_INFO, "  Vendor class: 0x%s",
					binary2string(valp, optlen));
			}
			/*
			 * For this option, we just look up the vendor class
			 * and save a pointer to it if it's in the database.
			 */
			req->vendor_class = lookup_vendor_class(valp, optlen);
			if (!req->vendor_class) {
				report(LOG_WARNING, "Unknown vendor class 0x%s",
					binary2string(valp, optlen));
			}
			break;
		}
	}

	/*
	 * The spec says that there must be an END at the end, but
	 * we don't really need to check for it.
	 */

	return 1;
}

/*
 * get_ifinfo -- do an SIOCGIFALL to get information for the interface
 * with the given index.  Returns 1 if ok, or 0 if an error occurs.
 */

static int
get_ifinfo(u_long ifindex, struct ifreq_all *ifinfo)
{
	struct strioctl sioc;

	sioc.ic_cmd = SIOCGIFALL;
	sioc.ic_timout = -1;
	sioc.ic_len = sizeof(struct ifreq_all);
	sioc.ic_dp = (char *) ifinfo;

	ifinfo->if_number = ifindex;

	if (ioctl(ip_fd, I_STR, &sioc) == -1) {
		report(LOG_ERR, "SIOCGIFALL failed: %m");
		return 0;
	}

	return 1;
}

/*
 * scan_request -- extract information from a request packet and
 * set up the Request structure.
 * Returns 1 if ok, or 0 if there's something wrong with the request.
 */

static int
scan_request(Request *req)
{
	int i, option_len;

	/*
	 * op must be BOOTREQUEST
	 */
	
	if (req->pkt->op != BOOTREQUEST) {
		report(LOG_WARNING, "Unexpected DHCP op %d; discarding...",
			req->pkt->op);
		return 0;
	}

	req->present = 0;

	option_len = req->pkt_len - DHCP_FIXED_LEN;

	/*
	 * Check the magic cookie in the options area.
	 */
	
	if (option_len < OPT_COOKIE_LEN
	    || memcmp(req->pkt->options, magic_cookie, OPT_COOKIE_LEN) != 0) {
		report(LOG_WARNING, "Magic cookie not present; assuming BOOTP");
		return 1;
	}

	/*
	 * Process options in options area.
	 */
	
	if (!proc_options(req, &req->pkt->options[OPT_COOKIE_LEN],
	    option_len - OPT_COOKIE_LEN)) {
		return 0;
	}

	/*
	 * If option overload was specified, look in file and/or sname fields.
	 */
	
	if (PRESENT(req, PR_OPT_OVERLOAD)) {
		if (req->opt_overload == OVERLOAD_FILE
		    || req->opt_overload == OVERLOAD_BOTH) {
			if (!proc_options(req, req->pkt->file, DHCP_FILE_LEN)) {
				return 0;
			}
		}
		if (req->opt_overload == OVERLOAD_SNAME
		    || req->opt_overload == OVERLOAD_BOTH) {
			if (!proc_options(req, req->pkt->sname,
			    DHCP_SNAME_LEN)) {
				return 0;
			}
		}
	}

	/*
	 * Look up the client in the database.  If there was no client
	 * identifier option, use the hardware type & address.
	 */
	
	if (PRESENT(req, PR_CLIENT_ID)) {
		req->client = lookup_client(req->client_id[0],
			&req->client_id[1], req->client_id_len - 1);
	}
	else {
		if (req->pkt->hlen < 1 || req->pkt->hlen > DHCP_CHADDR_LEN) {
			report(LOG_WARNING, "Illegal hardware address length; discarding...");
			return 0;
		}
		req->client = lookup_client(req->pkt->htype,
			req->pkt->chaddr, req->pkt->hlen);
	}
	if (req->client && debug > 1) {
		report(LOG_INFO, "Found client in database.");
	}

	/*
	 * Look up the subnet, using ciaddr if set, or giaddr if set, or our
	 * address on the receiving interface.  If we have more than one
	 * address on that interface, use the first one that matches a subnet
	 * entry.
	 */
	
	if (req->pkt->ciaddr.s_addr != 0) {
		req->subnet = lookup_subnet(&req->pkt->ciaddr);
	}
	else if (req->pkt->giaddr.s_addr != 0) {
		req->subnet = lookup_subnet(&req->pkt->giaddr);
	}
	else {
		for (i = 0; i < req->ifinfo.if_naddr; i++) {
			if (req->subnet = lookup_subnet(
			  SATOIN(&req->ifinfo.addrs[i].addr))) {
				break;
			}
		}
	}
	if (req->subnet) {
		struct in_addr a;
		a.s_addr = ntohl(req->subnet->subnet.s_addr);
		if (debug > 1) {
			report(LOG_INFO, "Request matches subnet %s",
				inet_ntoa(a));
		}
	}

	return 1;
}

/*
 * aas_client_id -- construct the client ID that we give to the
 * address server.  This client ID is composed of the subnet number
 * in network order, followed by the DHCP client ID (or hardware type
 * and address).  A pointer to a static AasClientId structure which
 * points to a static buffer is returned.
 */

static AasClientId *
aas_client_id(Request *req)
{
	static u_char idbuf[MAX_OPT_LEN + sizeof(struct in_addr)];
	static AasClientId id = { idbuf, 0 };
	struct in_addr subnet;

	subnet.s_addr = htonl(req->subnet->subnet.s_addr);
	memcpy(idbuf, &subnet, sizeof(struct in_addr));
	if (PRESENT(req, PR_CLIENT_ID)) {
		memcpy(&idbuf[sizeof(struct in_addr)], req->client_id,
			req->client_id_len);
		id.len = req->client_id_len + sizeof(struct in_addr);
	}
	else {
		idbuf[sizeof(struct in_addr)] = req->pkt->htype;
		memcpy(&idbuf[sizeof(struct in_addr) + 1], req->pkt->chaddr,
			req->pkt->hlen);
		id.len = req->pkt->hlen + 1 + sizeof(struct in_addr);
	}

	return &id;
}

/*
 * open_aas_conn -- open a connection to the addres server
 * A pointer to the AasConenction structure is stored in aas_conn.
 * Returns 1 if successful or 0 if not.
 */

int
open_aas_conn(void)
{
	AasServer aserver;
	struct in_addr addr;
	char password[MAX_PASSWORD_LEN];	

	memset(&aserver, 0, sizeof(aserver));
	aserver.server.addr = &addr;
	aserver.server.len = sizeof(addr);
	aserver.password = (char *)password;
	aserver.password_len = MAX_PASSWORD_LEN;

	if (aas_get_server(NULL, &aserver) != AAS_SUCCESS) {
		report(LOG_ERR, "Unable to get address server configuration: %s",
			aas_strerror(aas_errno));
		return 0;
	}	

	if (aas_open(&aserver, AAS_MODE_BLOCK, &aas_conn) != AAS_SUCCESS) {
		if (aserver.addr_type == AF_INET) {
			report(LOG_ERR, "Unable to open connection to remote address server: %s",
				aas_strerror(aas_errno));
		}
		else {
			report(LOG_ERR, "Unable to open connection to local address server: %s",
				aas_strerror(aas_errno));
		}
		return 0;
	}

	return 1;
}

/*
 * check_lost_aas_conn -- check error code and determine if we have
 * lost our connection to the address server.  If so, the connection
 * is closed and aas_conn is set to NULL so that a new connection will
 * be attempted when another request comes in.
 */

static void
check_lost_aas_conn(void)
{
	if (aas_errno == AAS_LOST_CONNECTION) {
		aas_close(aas_conn);
		aas_conn = NULL;
	}
}

/*
 * check_server_id_if -- check given server ID against the addresses
 * in the given ifreq_all.  Returns 1 if ID matches, or 0 if not.
 */

static int
check_server_id_if(struct in_addr *server_id, struct ifreq_all *ifinfo)
{
	int i;

	for (i = 0; i < ifinfo->if_naddr; i++) {
		if (server_id->s_addr ==
		    SATOIN(&ifinfo->addrs[i].addr)->s_addr) {
			return 1;
		}
	}

	return 0;
}

/*
 * check_server_id -- check server ID against all of our addresses
 * Returns 1 if the given server ID matches, or 0 if not.
 * If unable to obtain interface information, 0 is returned.
 */

static int
check_server_id(Request *req)
{
	struct strioctl sioc;
	int naddrs, i;
	struct ifreq *ifreq, ifr;

	/*
	 * First check against the interface the packet came in on.
	 */
	
	if (check_server_id_if(&req->server_id, &req->ifinfo)) {
		return 1;
	}

	/*
	 * If that didn't match, get all of our addresses and
	 * check against that list.
	 */

	/*
	 * Get the number of addresses.
	 */

	sioc.ic_cmd = SIOCGIFANUM;
	sioc.ic_dp = (char *)&ifr;
	sioc.ic_len = sizeof(struct ifreq);
	sioc.ic_timout = -1;

	if (ioctl(ip_fd, I_STR, &sioc) < 0) {
		report(LOG_ERR, "SIOCGIFANUM failed: %m");
		return 0;
	}
	naddrs = ifr.ifr_naddr;

	if (!(ifreq = tbl_alloc(struct ifreq, naddrs))) {
		malloc_error("check_server_id");
		return 0;
	}

	/*
	 * Get addresses.
	 */

	sioc.ic_cmd = SIOCGIFCONF;
	sioc.ic_timout = -1;
	sioc.ic_len = naddrs * sizeof(struct ifreq);
	sioc.ic_dp = (char *) ifreq;
	if (ioctl(ip_fd, I_STR, &sioc) == -1) {
		report(LOG_ERR, "SIOCGIFCONF failed: %m");
		free(ifreq);
		return 0;
	}

	for (i = 0; i < naddrs; i++) {
		if (req->server_id.s_addr == ((struct sockaddr_in *)
		    &ifreq[i].ifr_addr)->sin_addr.s_addr) {
			free(ifreq);
			return 1;
		}
	}

	free(ifreq);
	return 0;
}

/*
 * store_option -- store a DHCP option in an option buffer
 * *ptr points to the next available byte in the option buffer.
 * *optlen gives the total length of the options in the option buffer.
 * size is the size of the option buffer.  *ptr and *optlen are updated.
 * Returns 1 if the option fits, or 0 if not.
 */

static int
store_option(u_char code, void *value, u_char len, u_char **ptr, int *optlen,
	int size)
{
	int totlen;

	totlen = len + 2;
	if (totlen + *optlen > size) {
		return 0;
	}
	*(*ptr)++ = code;
	*(*ptr)++ = len;
	memcpy(*ptr, value, len);
	*ptr += len;
	*optlen += totlen;
	if (debug > 1) {
		report(LOG_INFO, "Stored option %d: 0x%s",
			code, binary2string(value, len));
	}

}

/*
 * store_1byte_option -- call store_option to store a 1-byte option
 */

static int
store_1byte_option(u_char code, u_char value, u_char **ptr, int *optlen,
	int size)
{
	return store_option(code, &value, 1, ptr, optlen, size);
}

/*
 * store_4byte_option -- call store_option to store a 4-byte integer option
 * The value is converted to net order before storing.
 */

static int
store_4byte_option(u_char code, u_long value, u_char **ptr, int *optlen,
	int size)
{
	value = htonl(value);
	return store_option(code, &value, 4, ptr, optlen, size);
}

/*
 * send_reply -- send a DHCP reply message
 */

static void
send_reply(Request *req)
{
	struct sockaddr_in dest;
	struct in_addr *ifaddr;

	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;

	/*
	 * Make sure the packet is at least DHCP_MIN_LEN bytes.
	 * If it's shorter, the extra bytes are set to the PAD option.
	 * We assume that the packet buffer is at least DHCP_MIN_LEN bytes
	 * in length.
	 */
	
	if (req->pkt_len < DHCP_MIN_LEN) {
		memset(&req->pkt->options[req->pkt_len - DHCP_FIXED_LEN],
			OPT_PAD, DHCP_MIN_LEN - req->pkt_len);
		req->pkt_len = DHCP_MIN_LEN;
	}

	req->pkt->flags = ntohs(req->pkt->flags);

	/*
	 * See if we're sending to a gateway.
	 */
	
	if (req->pkt->giaddr.s_addr != 0) {
		if (debug > 1) {
			report(LOG_INFO, "Sending reply to relay agent %s",
				inet_ntoa(req->pkt->giaddr));
		}
		dest.sin_addr = req->pkt->giaddr;
		dest.sin_port = bootps_port;
		/*
		 * Gateway must broadcast if we're sending a DHCPNAK.
		 */
		if (req->msg_type == DHCPNAK) {
			req->pkt->flags |= DHCP_BROADCAST;
		}
	}
	else {
		dest.sin_port = bootpc_port;

		/*
		 * Decide how we need to send this reply.
		 */
		if (req->msg_type == DHCPNAK
		    || (req->pkt->ciaddr.s_addr == 0
		     && (req->pkt->flags & DHCP_BROADCAST))) {
			/*
			 * Broadcasting -- set destination & set
			 * broadcast interface.
			 */
			dest.sin_addr.s_addr = htonl(INADDR_BROADCAST);
			ifaddr = SATOIN(&req->ifinfo.addrs[0].addr);
			if (set_endpt_opt(endpt, IPPROTO_IP, IP_BROADCAST_IF,
			    ifaddr, sizeof(struct in_addr)) == -1) {
				report(LOG_ERR, "Unable to set broadcast interface to %s.",
				  inet_ntoa(*ifaddr));
				return;
			}
		}
		else if (req->pkt->yiaddr.s_addr != 0) {
			/*
			 * Send it to yiaddr.  To do this, we need to make
			 * an ARP entry first, in case the client is
			 * just starting up doesn't have this address yet
			 * (and therefore wouldn't respond to an ARP request).
			 */
			dest.sin_addr = req->pkt->yiaddr;
			setarp(&req->pkt->yiaddr, req->pkt->chaddr,
				req->pkt->hlen);
		}
		else {
			/*
			 * If there is no yiaddr we must be responding
			 * to a DHCPINFORM, which goes to ciaddr.
			 */
			dest.sin_addr = req->pkt->ciaddr;
		}

		if (debug > 1) {
			report(LOG_INFO, "Sending reply to %s",
				inet_ntoa(dest.sin_addr));
		}
	}

	req->pkt->flags = htons(req->pkt->flags);

	(void) send_packet(endpt, &dest, req->pkt, req->pkt_len);
}

/*
 * get_time_offset -- return a pointer to the value of the server's time offset
 * (UTC - local time).  The value is in network order.
 */

static void *
get_time_offset(void)
{
	static long offset;
	struct tm *tm;
	time_t t;

	/*
	 * XXX should we always return timezone?
	 * XXX is west negative or positive?
	 */

	tzset();
	time(&t);
	tm = localtime(&t);
	if (tm->tm_isdst) {
		offset = altzone;
	}
	else {
		offset = timezone;
	}

	/*
	 * West of UTC is negative.
	 */
	offset = -offset;

	offset = htonl(offset);
	return &offset;
}

/*
 * get_boot_file_size -- return a pointer to the boot file size
 * Returns NULL if unable to determine size, if boot file doesn't exist,
 * or if bootfile is NULL.  If tftpdir is set, it is prepended to the
 * boot file path.
 */

static void *
get_boot_file_size(Request *req)
{
	static u_short size;
	struct stat st;
	char *buf;
	int len;

	if (!req->bootfile) {
		return NULL;
	}

	len = 0;
	if (req->tftpdir) {
		len = strlen(req->tftpdir) + 1;	/* 1 more for the slash */
	}
	if (req->bootdir) {
		len += strlen(req->bootdir) + 1;  /*1 more for the slash */
	}
	len += strlen(req->bootfile) + 1;	/* 1 more for the null byte */

	if (!(buf = malloc(len))) {
		malloc_error("get_boot_file_size");
		return NULL;
	}

	buf[0] = '\0';
	if (req->tftpdir) {
		strcat(buf, req->tftpdir);
		strcat(buf, "/");
	}
	if (req->bootdir) {
		strcat(buf, req->bootdir);
		strcat(buf, "/");
	}
	strcat(buf, req->bootfile);

	if (stat(buf, &st) == -1) {
		report(LOG_WARNING, "Unable to get status for boot file %s: %m",
			buf);
		free(buf);
		return NULL;
	}

	free(buf);

	/*
	 * The boot file size option is only 16 bits, so don't return anything
	 * if it's bigger than 65535.
	 */
	
	if (st.st_size > 65535) {
		return NULL;
	}

	size = st.st_size;
	size = htons(size);
	return &size;
}

/*
 * get_broadcast_addr -- get the default broadcast address for the subnet
 * Returns a pointer to the broadcast address in network order.
 * If unable to determine the broadcast address, NULL is returned.
 * The address returned is (subnet, -1).
 */

static void *
get_broadcast_addr(Request *req)
{
	static struct in_addr addr;

	if (!req->subnet) {
		return NULL;
	}

	addr.s_addr = req->subnet->subnet.s_addr | ~req->subnet->mask.s_addr;
	addr.s_addr = htonl(addr.s_addr);
	return &addr;
}

/*
 * store_options -- store options in the given buffer
 * req points to the request structure (some information is retrieved
 * from there when generating default option values).
 * optbuf points to the area in which the options are to be stored.  len is
 * the length of this area.  options is an array of OptionSetting structures
 * that refer to the options begin set.  nopts is the number of elements in
 * options.  When an option is stored, the corresponding element in options
 * is set to NULL.  An option may be skipped if there isn't enough room in
 * the buffer.  An END option is stored after the last option in the buffer,
 * and the remainder of the buffer is filled with PAD bytes.  *done is set to
 * 1 if all options have been stored, or 0 if some didn't fit.  The return
 * value of the function is the number of bytes of actual options stored in
 * the buffer (not counting the PAD bytes at the end).
 */

static int
store_options(Request *req, u_char *optbuf, int len, OptionSetting **options,
	int nopts, int *done)
{
	int i, optlen, optspace;
	OptionSetting *osp;
	void *optval;
	u_char *p;

	/*
	 * Set *done to 1 initially.  If we skip something, it will be
	 * changed to 0.
	 */
	
	*done = 1;

	/*
	 * Reserve one byte for the END option.
	 */

	optspace = len - 1;

	p = optbuf;
	optlen = 0;

	for (i = 0; i < nopts; i++) {
		osp = options[i];
		if (!(optval = osp->val)) {
			/*
			 * Option was set to "auto"
			 */
			switch (osp->opt->code) {
			case OPT_TIME_OFFSET:
				optval = get_time_offset();
				break;
			case OPT_BOOT_SIZE:
				optval = get_boot_file_size(req);
				break;
			case OPT_BCAST_ADDR:
				optval = get_broadcast_addr(req);
				break;
			}
			if (!optval) {
				options[i] = NULL;
				continue;
			}
		}
		if (store_option(osp->opt->code, optval, osp->size, &p,
		    &optlen, optspace)) {
			options[i] = NULL;
		}
		else {
			/*
			 * This option didn't fit.
			 */
			*done = 0;
		}
	}

	*p = OPT_END;
	optlen++;

	if (optlen < len) {
		memset(&optbuf[optlen], OPT_PAD, len - optlen);
	}

	return optlen;
}

/*
 * send_dhcpack -- send a DHCPACK or DHCPOFFER message
 */

static void
send_dhcpack(Request *req, struct in_addr *addr, u_long lease)
{
	u_char *p, msg_type, o, *optval;
	int optlen, optspace, optsize, i, nopts, max_optsize, nsize, done;
	struct in_addr *server_id;
	struct dhcp *pkt = req->pkt;
	Subnet *subnet = req->subnet;
	u_long timer;
	char *bfp;
	int bootdir_len, bootfile_len, tot_len;
	OptionSetting *config_opts, *osp;
	u_char overload_file, overload_sname, *overload_ptr;
	static OptionSetting *config_optmap[NUM_OPTS];
	static u_char opt_included[MAX_OPT_CODE + 1];
	static OptionSetting *opt_list[MAX_OPT_CODE + 1];
	static char bootfile_buf[DHCP_FILE_LEN + 1];

	pkt->op = BOOTREPLY;
	pkt->hops = 0;
	pkt->secs = 0;
	if (req->msg_type == DHCPDISCOVER) {
		pkt->ciaddr.s_addr = 0;
		msg_type = DHCPOFFER;
		if (debug) {
			report(LOG_INFO, "Sending DHCPOFFER: address %s lease %lu",
				inet_ntoa(*addr), lease);
		}
	}
	else {
		msg_type = DHCPACK;
		if (debug) {
			if (addr) {
				report(LOG_INFO, "Sending DHCPACK: address %s lease %lu",
					inet_ntoa(*addr), lease);
			}
			else {
				report(LOG_INFO, "Sending DHCPACK");
			}
		}
	}
	if (addr) {
		pkt->yiaddr = *addr;
	}

	/*
	 * Create a merged list of configured options.
	 */
	
	config_opts = NULL;
	if (req->client) {
		merge_options(&config_opts, req->client->options);
	}
	if (req->user_class) {
		merge_options(&config_opts, req->user_class->options);
	}
	if (req->vendor_class) {
		merge_options(&config_opts, req->vendor_class->options);
	}
	if (req->subnet) {
		merge_options(&config_opts, req->subnet->options);
	}
	merge_options(&config_opts, global_options);

	/*
	 * Create a table to map an option number to the option setting.
	 */
	
	for (i = 0; i < NUM_OPTS; i++) {
		config_optmap[i] = NULL;
	}
	for (osp = config_opts; osp; osp = osp->merge) {
		config_optmap[osp->opt->code] = osp;
	}

	/*
	 * The boot server address goes in the header, so we can
	 * take care of that now.
	 */
	
	if (osp = config_optmap[OPT_BOOT_SERVER]) {
		memcpy(&pkt->siaddr, osp->val, sizeof(struct in_addr));
	}
	else {
		pkt->siaddr.s_addr = 0;
	}

	/*
	 * Make a list of options we're going to include.  This is
	 * based on the parameter request list (if present in the request)
	 * and the list of configured options.  We also add up the total
	 * size of the options we're including.
	 */
	
	for (i = 0; i <= MAX_OPT_CODE; i++) {
		opt_included[i] = 0;
	}
	nopts = 0;
	optsize = 0;
	if (PRESENT(req, PR_PARAM_REQ)) {
		for (i = 0; i < req->param_req_len; i++) {
			o = req->param_req[i];
			if (opt_included[o]) {
				continue;
			}
			/*
			 * To include an option, there must be a
			 * configured setting for it, or it must
			 * have a default value.
			 */
			if ((osp = config_optmap[o])
			    || (osp = option_defaults[o])) {
				opt_list[nopts++] = osp;
				/*
				 * Space required for this option is
				 * the size of the value plus 1 for the
				 * code and 1 for the length byte.
				 */
				optsize += osp->size + 2;
				opt_included[o] = 1;
			}
		}
	}
	for (osp = config_opts; osp; osp = osp->merge) {
		/*
		 * Add it if it's not already included.
		 */
		if (osp->opt->code <= MAX_OPT_CODE
		    && !opt_included[osp->opt->code]) {
			opt_list[nopts++] = osp;
			optsize += osp->size + 2;
			opt_included[osp->opt->code] = 1;
		}
	}

	/*
	 * Determine the boot file name.
	 */

	bootfile_buf[DHCP_FILE_LEN] = '\0';
	if (!(osp = config_optmap[OPT_BOOT_FILE_NAME])) {
		if (pkt->file[0] != '\0') {
			memcpy(bootfile_buf, pkt->file, DHCP_FILE_LEN);
			req->bootfile = bootfile_buf;
		}
		else if (PRESENT(req, PR_BOOT_FILE)) {
			if (req->opt_boot_file_len > DHCP_FILE_LEN) {
				req->bootfile = NULL;
			}
			else {
				memcpy(bootfile_buf, req->opt_boot_file,
					req->opt_boot_file_len);
				req->bootfile = bootfile_buf;
			}
		}
		else  {
			req->bootfile = NULL;
		}
	}
	else {
		req->bootfile = (char *) osp->val;
	}

	/*
	 * IMPORTANT: This function constructs the reply packet in the
	 * buffer that contains the request packet, so we need to
	 * make sure that we process or copy all information that we
	 * reference directly from the packet.  This includes client ID,
	 * parameter request list, and boot file.  This processing must
	 * be completed by this point (since we're about to start storing
	 * options, which will overwrite the client's options).
	 */

	p = &pkt->options[OPT_COOKIE_LEN];
	optlen = OPT_COOKIE_LEN;

	/*
	 * Determine the amount of space we currently have for storing
	 * options.  This is the lesser of the maximum allowable size (either
	 * the default or that specified by the client) and the space
	 * we currently have allocated.
	 */

	if (PRESENT(req, PR_MAX_MSG_SIZE)) {
		max_optsize = MSGSIZETODHCPSIZE(req->max_msg_size)
			- DHCP_FIXED_LEN;
		/*
		 * Make sure it's at least the minimum value.
		 */
		if (max_optsize < DHCP_OPTION_LEN) {
			max_optsize = DHCP_OPTION_LEN;
		}
	}
	else {
		max_optsize = DHCP_OPTION_LEN;
	}
	optspace = req->pkt_buf_size - DHCP_FIXED_LEN;
	if (optspace > max_optsize) {
		optspace = max_optsize;
	}

	/*
	 * First, store the "essential" options.  We don't need to check
	 * the return codes from the store_* routines since we know these
	 * options can fit in the minimal packet.
	 */

	(void) store_1byte_option(OPT_MSG_TYPE, msg_type, &p, &optlen,
		optspace);

	if (PRESENT(req, PR_SERVER_ID)) {
		server_id = &req->server_id;
	}
	else {
		server_id = SATOIN(&req->ifinfo.addrs[0].addr);
	}
	(void) store_option(OPT_SERVER_ID, server_id, sizeof(struct in_addr),
		&p, &optlen, optspace);
	
	if (addr) {
		(void) store_4byte_option(OPT_LEASE_TIME, lease,
			&p, &optlen, optspace);
		/*
		 * Store T1 & T2 values if non-infinite lease.
		 */
		if (subnet && lease != LEASE_INFINITY) {
			timer = ((double) lease * (double) subnet->t1)
				/ 1000.0 + 0.5;
			(void) store_4byte_option(OPT_T1, timer,
				&p, &optlen, optspace);
			timer = ((double) lease * (double) subnet->t2)
				/ 1000.0 + 0.5;
			(void) store_4byte_option(OPT_T2, timer,
				&p, &optlen, optspace);
		}
	}

	/*
	 * Store the subnet mask.
	 */
	
	/*
	 * We use store_4byte_option here because the mask is stored
	 * in host order.
	 */

	(void) store_4byte_option(OPT_SUBNET_MASK, subnet->mask.s_addr,
		&p, &optlen, optspace);

	/*
	 * If the request matched a user class in the database, we
	 * have to include the class name.  We know this will fit even
	 * if it's the maximum length (255).
	 */
	
	if (req->user_class) {
		(void) store_option(OPT_USER_CLASS, req->user_class->class,
			req->user_class->class_len, &p, &optlen, optspace);
	}

	/*
	 * If we are providing a boot file name, store it in the header.
	 */

	memset(pkt->file, 0, DHCP_FILE_LEN);
	if (req->bootfile) {
		if (osp = config_optmap[OPT_TFTP_DIR]) {
			req->tftpdir = (char *) osp->val;
		}
		else {
			req->tftpdir = NULL;
		}
		bootfile_len = strlen(req->bootfile);
		tot_len = bootfile_len;
		if (osp = config_optmap[OPT_BOOT_DIR]) {
			req->bootdir = (char *) osp->val;
			bootdir_len = strlen(req->bootdir);
			tot_len += bootdir_len;
			if (req->bootfile[0] != '/') {
				tot_len++;
			}
		}
		else {
			req->bootdir = NULL;
		}
		if (tot_len > DHCP_FILE_LEN) {
			report(LOG_WARNING, "Boot file path %s/%s too long.",
				req->bootdir, req->bootfile);
			req->bootfile = NULL;
		}
		else {
			if (req->bootdir) {
				memcpy((char *) pkt->file, req->bootdir,
					bootdir_len);
				bfp = (char *) &pkt->file[bootdir_len];
				if (req->bootfile[0] != '/') {
					*bfp++ = '/';
				}
			}
			else {
				bfp = (char *) pkt->file;
			}
			memcpy(bfp, req->bootfile, bootfile_len);
		}
	}

	/*
	 * See if we have enough room for the remaining options (1 is
	 * subtracted to leave room for an END option) .  If the total option
	 * size won't fit in the packet, we can enlarge it up to the maximum
	 * size (default of 576 - 24 or the client's max msg size - 24).  If
	 * we still don't have enough space, we may be able to use option
	 * overloading if we're configured to do so.
	 */
	
	if (optlen + optsize > optspace - 1) {
		/*
		 * If we're not at the maximum yet, we can enlarge the buffer.
		 */
		if (optspace < max_optsize) {
			/*
			 * Enlarge it to the amount we need, if possible.
			 */
			nsize = DHCP_FIXED_LEN + optlen + optsize + 1;
			if (pkt = (struct dhcp *) realloc(pkt, nsize)) {
				req->pkt = pkt;
				req->pkt_buf_size = nsize;
				optspace = nsize - DHCP_FIXED_LEN;
				p = &pkt->options[optlen];
			}
			else {
				/*
				 * If it failed, just continue with what
				 * we already have.
				 */
				pkt = req->pkt;
				report(LOG_WARNING, "Unable to increase packet buffer size.");
			}
		}
	}

	/*
	 * If we will be overloading, save room for the overload
	 * option and remember where it is.  We only use overload if we
	 * are configured to do so.
	 */
	
	if (server_params.option_overload
	    && (optlen + optsize > optspace - 1)) {
		overload_ptr = p;
		p += 3;
		optlen += 3;
	}
	else {
		overload_ptr = NULL;
	}

	/*
	 * Store options in the options buffer.
	 */
	
	optlen += store_options(req, p, optspace - optlen, opt_list, nopts,
		&done);

	/*
	 * If we didn't store them all and we're allowed to overload,
	 * try to store options in the file and sname fields.
	 */
	
	overload_file = 0;
	overload_sname = 0;

	if (!done && server_params.option_overload) {
		/*
		 * Only use the file field if we aren't returning a boot file.
		 */
		if (!req->bootfile) {
			if (store_options(req, pkt->file, DHCP_FILE_LEN,
			    opt_list, nopts, &done) > 0) {
				overload_file = 1;
			}
		}
		if (!done) {
			if (store_options(req, pkt->sname, DHCP_SNAME_LEN,
			    opt_list, nopts, &done) > 0) {
				overload_sname = 1;
			}
		}
		/*
		 * Store the overload option.  If we didn't actually
		 * store anything, put PADs there.
		 */
		if (overload_file || overload_sname) {
			overload_ptr[0] = OPT_OVERLOAD;
			overload_ptr[1] = 1;
		}
		if (overload_file && !overload_sname) {
			overload_ptr[2] = OVERLOAD_FILE;
		}
		else if (overload_sname && !overload_file) {
			overload_ptr[2] = OVERLOAD_SNAME;
		}
		else if (overload_sname && overload_file) {
			overload_ptr[2] = OVERLOAD_BOTH;
		}
		else {
			memset(overload_ptr, OPT_PAD, 3);
		}
	}

	/*
	 * Print a warning if there were options that didn't fit.
	 */
	
	if (!done) {
		for (i = 0; i < nopts; i++) {
			if (osp = opt_list[i]) {
				report(LOG_WARNING, "Option %d did not fit in packet.",
					osp->opt->code);
			}
		}
	}

	req->pkt_len = DHCP_FIXED_LEN + optlen;
	send_reply(req);
}

/*
 * send_dhcpnak -- send a DHCPNAK message
 */

static void
send_dhcpnak(Request *req)
{
	u_char *p;
	int optlen, optspace;
	struct in_addr *server_id;

	if (debug) {
		report(LOG_INFO, "Sending DHCPNAK");
	}

	req->pkt->op = BOOTREPLY;
	req->pkt->hops = 0;
	req->pkt->secs = 0;
	req->pkt->ciaddr.s_addr = 0;
	req->pkt->yiaddr.s_addr = 0;
	req->pkt->siaddr.s_addr = 0;

	/*
	 * send_reply looks at msg_type to tell that this is a DHCPNAK.
	 */
	
	req->msg_type = DHCPNAK;

	p = &req->pkt->options[OPT_COOKIE_LEN];
	optlen = OPT_COOKIE_LEN;
	optspace = DHCP_OPTION_LEN - 1;

	/*
	 * We know there is enough room for the options we're adding
	 * here.
	 */
	
	(void) store_1byte_option(OPT_MSG_TYPE, DHCPNAK, &p, &optlen, optspace);
	if (PRESENT(req, PR_SERVER_ID)) {
		server_id = &req->server_id;
	}
	else {
		server_id = SATOIN(&req->ifinfo.addrs[0].addr);
	}
	(void) store_option(OPT_SERVER_ID, server_id, sizeof(struct in_addr),
		&p, &optlen, optspace);
	
	*p = OPT_END;
	optlen++;

	req->pkt_len = DHCP_FIXED_LEN + optlen;
	send_reply(req);
}

/*
 * address_known -- see if an address is in the database.
 * Returns 1 if it is, 0 if not, or -1 if some error occurs.
 */

static int
address_known(AasConnection *conn, char *pool, struct in_addr *addr)
{
	AasAddr min_addr, max_addr;
	AasAddrInfo *addrs;
	AasTime server_time;
	u_long num_addrs;

	min_addr.addr = addr;
	min_addr.len = sizeof(struct in_addr);
	max_addr.addr = addr;
	max_addr.len = sizeof(struct in_addr);

	if (aas_get_addr_info(conn, pool, AAS_ATYPE_INET,
	    &min_addr, &max_addr, &addrs, &num_addrs, &server_time)
	    == AAS_SUCCESS) {
		free(addrs);
		return num_addrs == 1;
	}
	else {
		report(LOG_ERR, "Request for address information failed: %s",
			aas_strerror(aas_errno));
		check_lost_aas_conn();
		return -1;
	}
}

/*
 * clientid2string -- return a string representation of the client ID in
 * the given request.  If no client was specified, the hardware type & address
 * are used.
 */

char *
clientid2string(Request *req)
{
	static char buf[550];
	u_char type;
	int len;
	u_char *p;

	if (PRESENT(req, PR_CLIENT_ID)) {
		type = req->client_id[0];
		p = &req->client_id[1];
		len = req->client_id_len - 1;
	}
	else {
		type = req->pkt->htype;
		p = req->pkt->chaddr;
		len = req->pkt->hlen;
	}
	sprintf(buf, "%d:%s", type, binary2string(p, len));
	return buf;
}

/*
 * proc_* -- process various types of DHCP messages
 */

void
proc_dhcpdiscover(Request *req)
{
	AasAddr req_addr, min_addr, max_addr, *req_addr_p;
	AasAddr addr;
	struct in_addr req_in_addr, min_in_addr, max_in_addr;
	struct in_addr alloc_in_addr;
	AasClientInfo cinfo;
	AasTime server_time;
	u_long lease, age;
	int ret;

	if (debug) {
		report(LOG_INFO, "Request is DHCPDISCOVER.");
	}

	/*
	 * If the client is configured with a fixed address,
	 * offer an infinite lease of that address.  Otherwise,
	 * if the subnet is configured for dynamic allocation,
	 * we will get an address from the address server.
	 * Otherwise, we do nothing.
	 */
	
	if (req->client && req->client->addr.s_addr != 0) {
		/*
		 * send_dhcpack is smart enough to actually send
		 * a DHCPOFFER in response to a DHCPDISCOVER.
		 */
		send_dhcpack(req, &req->client->addr, LEASE_INFINITY);
		return;
	}
	else if (!req->subnet) {
		if (debug) {
			report(LOG_INFO, "No information for client's subnet; ignoring request.");
		}
		return;
	}
	else if (!req->subnet->pool) {
		if (debug) {
			report(LOG_INFO, "Subnet %s has no address pool assigned; ignoring request.",
				inet_ntoa(req->subnet->subnet));
		}
		return;
	}

	if (!aas_conn && !open_aas_conn()) {
		return;
	}

	/*
	 * Determine the lease time to offer.
	 */
	
	if (PRESENT(req, PR_LEASE)) {
		if (req->lease > req->subnet->lease_max) {
			lease = req->subnet->lease_max;
		}
		else {
			lease = req->lease;
		}
	}
	else {
		lease = req->subnet->lease_dflt;
	}

	/*
	 * Before allocating an address, we need to see if the client
	 * already has one.  If the client already has an address, we
	 * either return the time remaining in that lease or, if the
	 * client is requesting a specific lease, offer an appropriate lease.
	 */
	
	if ((ret = aas_get_client_info(aas_conn, req->subnet->pool,
	    AAS_ATYPE_INET, DHCP_SERVICE_NAME,
	    aas_client_id(req), &cinfo, &server_time)) == AAS_SUCCESS) {
		if ((cinfo.flags & (AAS_ADDR_INUSE | AAS_ADDR_TEMP |
		    AAS_ADDR_DISABLED)) == AAS_ADDR_INUSE) {
			/*
			 * If the client requested a lease, offer
			 * an appropriate lease (determined above).
			 */
			if (PRESENT(req, PR_LEASE)) {
				send_dhcpack(req, (struct in_addr *)
					cinfo.addr.addr, lease);
				free(cinfo.addr.addr);
				return;
			}
			else {
				/*
				 * Compute the actual lease time by factoring
				 * out the padding.
				 */
				if (lease != LEASE_INFINITY) {
					/*
					 * Yes, this will not be completely
					 * correct if we are within lease_pad
					 * % of the largest possible lease.
					 * This only affects lease times >
					 * 130 years, so I think we're ok.
					 */
					lease = LEASE_UNPAD(cinfo.lease_time);
				}
				/*
				 * Make sure the "unpadded" lease hasn't
				 * expired.
				 */
				age = server_time - cinfo.alloc_time;
				if (lease > age) {
					/*
					 * Subtract the amount of time since
					 * allocation to get the remaining
					 * time.
					 */
					lease -= age;
					/*
					 * Send a DHCPACK with the current
					 * lease info.
					 */
					send_dhcpack(req, (struct in_addr *)
						cinfo.addr.addr, lease);
					free(cinfo.addr.addr);
					return;
				}
			}
		}
		else {
			free(cinfo.addr.addr);
		}
	}
	else if (aas_errno != AAS_UNKNOWN_CLIENT) {
		report(LOG_WARNING, "Request to get client information failed: %s",
			aas_strerror(aas_errno));
		check_lost_aas_conn();
		return;
	}

	/*
	 * Client did not already have an address allocated or the client
	 * specified a lease time (which means we extend the current lease
	 * if any), so we need to do an allocation request.
	 */

	/*
	 * Set up address structures.
	 */
	
	req_addr.addr = &req_in_addr;
	req_addr.len = sizeof(struct in_addr);
	min_addr.addr = &min_in_addr;
	min_addr.len = sizeof(struct in_addr);
	max_addr.addr = &max_in_addr;
	max_addr.len = sizeof(struct in_addr);

	min_in_addr.s_addr = req->subnet->subnet.s_addr + 1;
	min_in_addr.s_addr = htonl(min_in_addr.s_addr);
	max_in_addr.s_addr = req->subnet->subnet.s_addr
		+ ~req->subnet->mask.s_addr - 1;
	max_in_addr.s_addr = htonl(max_in_addr.s_addr);

	addr.addr = &alloc_in_addr;
	addr.len = sizeof(alloc_in_addr);

	/*
	 * If the request contains a requested address, use it.
	 */

	if (PRESENT(req, PR_REQ_ADDR)) {
		req_in_addr = req->req_addr;
		req_addr_p = &req_addr;
	}
	else {
		req_addr_p = NULL;
	}
	/*
	 * Loop trying to allocate addresses, just in case the ping below
	 * determines that the address is in use.
	 */
	for (;;) {
		if (aas_alloc(aas_conn, req->subnet->pool, AAS_ATYPE_INET,
		    req_addr_p, &min_addr, &max_addr,
		    AAS_ALLOC_PREFER_PREV | AAS_ALLOC_TEMP,
		    server_params.lease_res, DHCP_SERVICE_NAME,
		    aas_client_id(req), &addr) != AAS_SUCCESS) {
			report(LOG_WARNING,
			    "Address allocation failed for pool %s: %s",
			    req->subnet->pool, aas_strerror(aas_errno));
			check_lost_aas_conn();
			return;
		}
		/*
		 * If configured to do so, attempt to ping the address.
		 * If it's in use, disable it.
		 */
		if (!server_params.address_probe
		    || !ping((struct in_addr *) addr.addr)) {
			break;
		}
		report(LOG_WARNING, "Address %s is already in use -- disabling",
			inet_ntoa(*((struct in_addr *) addr.addr)));
		if (aas_disable(aas_conn, req->subnet->pool,
		    AAS_ATYPE_INET,
		    &addr, 1) != AAS_SUCCESS) {
			report(LOG_WARNING,
				"Unable to disable address %s in pool %s: %s",
				inet_ntoa(req->req_addr),
				req->subnet->pool,
				aas_strerror(aas_errno));
			check_lost_aas_conn();
			return;
		}
	}

	/*
	 * Send the DHCPOFFER (send_dhcpack knows to do this).
	 */
	
	send_dhcpack(req, (struct in_addr *) addr.addr, lease);
}

void
proc_dhcprequest(Request *req)
{
	struct in_addr addr;
	AasAddr req_addr, alloc_addr;
	struct in_addr alloc_in_addr;
	char abuf1[16], abuf2[16];
	u_long lease, age, padded;
	AasClientId *id;
	AasClientInfo cinfo;
	AasTime server_time;
	int ret;
	Subnet *subnet = req->subnet;
	struct dhcp *pkt = req->pkt;

	if (debug) {
		report(LOG_INFO, "Request is DHCPREQUEST.");
	}

	/*
	 * Compute the lease time if info is available.
	 */
	
	if (subnet) {
		if (PRESENT(req, PR_LEASE)) {
			if (req->lease > subnet->lease_max) {
				lease = subnet->lease_max;
			}
			else {
				lease = req->lease;
			}
		}
		else {
			lease = subnet->lease_dflt;
		}
	}

	/*
	 * The way we handle this request depends on the client's state,
	 * which is determined by the settings of various things in
	 * the request.
	 */
	
	if (PRESENT(req, PR_SERVER_ID)) {
		/*
		 * Client is in SELECTING state.
		 * Check the server ID.  If it's us, process the request.
		 * If not, see if we have an address on hold for this client.
		 * If we do, release is (because it's choosing another server).
		 */
		if (!check_server_id(req)) {
			if (debug) {
				report(LOG_INFO, "Received request for non-matching server ID %s.",
					inet_ntoa(req->server_id));
			}
			/*
			 * Connect to the address sevrer.
			 */
			if (!aas_conn && !open_aas_conn()) {
				return;
			}
			/*
			 * See if we reserved an address for the client.  If
			 * so, release it.
			 */
			id = aas_client_id(req);
			if ((ret = aas_get_client_info(aas_conn,
			    subnet->pool, AAS_ATYPE_INET, DHCP_SERVICE_NAME,
			    id, &cinfo, &server_time))
			    == AAS_SUCCESS) {
				if ((cinfo.flags
				  & (AAS_ADDR_INUSE | AAS_ADDR_TEMP))
				  == (AAS_ADDR_INUSE | AAS_ADDR_TEMP)) {
					if (debug) {
						report(LOG_INFO, "Client %s has chosen another server; releasing %s",
						  binary2string(id->id,
						    id->len),
						  inet_ntoa(*((struct in_addr *) cinfo.addr.addr)));
					}
					if (aas_free(aas_conn,
					  subnet->pool, AAS_ATYPE_INET,
					  &cinfo.addr, DHCP_SERVICE_NAME,
					  id) != AAS_SUCCESS) {
						report(LOG_WARNING,
						  "Unable to release address %s in pool %s: %s",
						  inet_ntoa(*((struct in_addr *) cinfo.addr.addr)),
						  subnet->pool,
						  aas_strerror(aas_errno));
						check_lost_aas_conn();
					}
				}
			}
			else if (aas_errno != AAS_UNKNOWN_CLIENT) {
				report(LOG_WARNING,
				 "Request to get client information failed: %s",
					aas_strerror(aas_errno));
				check_lost_aas_conn();
				return;
			}
			return;
		}
		/*
		 * RFC1541 says that the client MUST NOT use the
		 * Requested IP Address option in a DHCPREQUEST.  In
		 * subsequent drafts this was changed to say that
		 * the client MUST use the Requested IP Address option
		 * to specify the address in SELECTING state.  To be
		 * compatible with both, we use the requested address if
		 * it's there, or ciaddr otherwise.
		 * Since this code was originally written to use req_addr,
		 * if ciaddr is being used, we just set req_addr to ciaddr.
		 */
		if (!PRESENT(req, PR_REQ_ADDR)) {
			if (pkt->ciaddr.s_addr != 0) {
				req->req_addr = pkt->ciaddr;
				pkt->ciaddr.s_addr = 0;
			}
			else {
				report(LOG_WARNING, "Request from client in SELECTING state does not contain a requested address.");
				return;
			}
		}
		/*
		 * If the client has a fixed address configured, verify that
		 * it's the one being requested and send an ACK, otherwise
		 * send a NAK.
		 */
		if (req->client && req->client->addr.s_addr != 0) {
			if (req->req_addr.s_addr == req->client->addr.s_addr) {
				send_dhcpack(req, &req->req_addr,
					LEASE_INFINITY);
			}
			else {
				send_dhcpnak(req);
			}
			return;
		}
		/*
		 * Make sure there's a subnet and it has an address
		 * pool assigned.
		 */
		if (!subnet) {
			report(LOG_WARNING, "Request received from SELECTING client on unknown subnet.");
			return;
		}
		if (!subnet->pool) {
			addr.s_addr = htonl(subnet->subnet.s_addr);
			report(LOG_WARNING, "No address pool assigned for subnet %s.",
				inet_ntoa(addr));
			return;
		}
		req_addr.addr = &req->req_addr;
	}
	else {
		/*
		 * Client is in either INIT-REBOOT, RENEWING, REBINDING
		 * state.
		 */
		/*
		 * We use either the requested address or ciaddr.
		 * Since the code below was originally written to use
		 * req_addr, if we are using ciaddr, we just set req_addr
		 * to ciaddr.
		 */
		if (!PRESENT(req, PR_REQ_ADDR)) {
			if (pkt->ciaddr.s_addr != 0) {
				req->req_addr = pkt->ciaddr;
			}
			else {
				report(LOG_WARNING, "No address specified in DHCPREQUEST.");
				return;
			}
		}
		/*
		 * If the client has a fixed address configured, verify that
		 * it's the one being requested and send an ACK, otherwise
		 * send a NAK.
		 */
		if (req->client && req->client->addr.s_addr != 0) {
			if (req->req_addr.s_addr == req->client->addr.s_addr) {
				send_dhcpack(req, &req->req_addr,
					LEASE_INFINITY);
			}
			else {
				send_dhcpnak(req);
			}
			return;
		}
		/*
		 * Make sure we know about the client's subnet.
		 */
		if (!subnet) {
			return;
		}
		/*
		 * If the requested address doesn't match the subnet,
		 * send a NAK.
		 */
		if (lookup_subnet(&req->req_addr) != subnet) {
			send_dhcpnak(req);
			return;
		}
		if (!subnet->pool) {
			return;
		}
		/*
		 * Connect to the address sevrer.
		 */
		if (!aas_conn && !open_aas_conn()) {
			return;
		}
		/*
		 * See if we have any information about the client.
		 */
		if ((ret = aas_get_client_info(aas_conn, req->subnet->pool,
		    AAS_ATYPE_INET, DHCP_SERVICE_NAME,
		    aas_client_id(req), &cinfo, &server_time)) == AAS_SUCCESS) {
			/*
			 * If the client is requesting a different address
			 * than we have on record AND the requested address
			 * is in our database, we know the client's information
			 * is wrong, so we send a NAK.  If the requested
			 * address isn't in our database, we do nothing.
			 */
			if (((struct in_addr *) cinfo.addr.addr)->s_addr
			    != req->req_addr.s_addr) {
				free(cinfo.addr.addr);
				if (address_known(aas_conn, subnet->pool,
				    &req->req_addr) == 1) {
					send_dhcpnak(req);
				}
				return;
			}
		}
		else if (aas_errno == AAS_UNKNOWN_CLIENT) {
			/*
			 * If we don't know about this client AND
			 * the address being requested isn't in our
			 * database, we ignore the request.  Otherwise,
			 * fall through and try to allocate it.
			 */
			if (!address_known(aas_conn, subnet->pool,
			    &req->req_addr) == 1) {
				return;
			}
		}
		else {
			report(LOG_WARNING, "Request to get client information failed: %s",
				aas_strerror(aas_errno));
			check_lost_aas_conn();
			return;
		}
		req_addr.addr = &req->req_addr;
	}

	/*
	 * Do the allocation.  If the allocation fails because the address
	 * isn't available, send a NAK; for other failures, no response is
	 * sent.
	 */

	if (!aas_conn && !open_aas_conn()) {
		return;
	}

	req_addr.len = sizeof(struct in_addr);
	/*
	 * Can't pad the lease if that would make it LEASE_INFINITY or
	 * greater.
	 */
	if (lease <= LEASE_UNPAD(LEASE_INFINITY - 1)) {
		padded = LEASE_PAD(lease);
	}
	else {
		padded = lease;
	}
	/*
	 * Allocate memory for address
	 */
	alloc_addr.addr = &alloc_in_addr;
	alloc_addr.len = sizeof(alloc_in_addr);
	if (aas_alloc(aas_conn, subnet->pool, AAS_ATYPE_INET, &req_addr,
	    NULL, NULL, AAS_ALLOC_SPECIFIC, padded, DHCP_SERVICE_NAME,
	    aas_client_id(req), &alloc_addr) == AAS_SUCCESS) {
		send_dhcpack(req, (struct in_addr *) alloc_addr.addr, lease);
	}
	else if (aas_errno == AAS_NO_ADDRS_AVAIL) {
		report(LOG_INFO, "Denying request for address %s -- address not available",
			inet_ntoa(req->req_addr));
		send_dhcpnak(req);
	}
	else if (errno != AAS_NO_SUCH_ADDR) {
		report(LOG_ERR, "Address allocation request failed: %s",
			aas_strerror(aas_errno));
		check_lost_aas_conn();
	}
}

void
proc_dhcpdecline(Request *req)
{
	AasAddr addr;
	AasClientInfo cinfo;
	AasTime server_time;
	int ret;

	if (debug) {
		report(LOG_INFO, "Request is DHCPDECLINE.");
	}

	/*
	 * We must know about the subnet and it must have a pool
	 * defined.
	 */
	
	if (!req->subnet || !req->subnet->pool) {
		return;
	}

	/*
	 * Make sure the request specifies the address being declined.
	 */
	
	if (!PRESENT(req, PR_REQ_ADDR)) {
		if (req->pkt->ciaddr.s_addr == 0) {
			report(LOG_WARNING, "DHCPDECLINE contains no address.");
			return;
		}
		else {
			req->req_addr = req->pkt->ciaddr;
		}
	}

	/*
	 * Verify that the server ID is one of our addresses.
	 */
	
	if (!PRESENT(req, PR_SERVER_ID)) {
		report(LOG_WARNING, "DHCPDECLINE contains no server ID.");
		return;
	}

	if (!check_server_id(req)) {
		return;
	}

	if (!aas_conn && !open_aas_conn()) {
		return;
	}

	if ((ret = aas_get_client_info(aas_conn, req->subnet->pool,
	    AAS_ATYPE_INET, DHCP_SERVICE_NAME,
	    aas_client_id(req), &cinfo, &server_time)) == AAS_SUCCESS) {
		if (!(cinfo.flags & AAS_ADDR_INUSE)
		    || ((struct in_addr *) cinfo.addr.addr)->s_addr
		    != req->req_addr.s_addr) {
			report(LOG_WARNING, "Received DHCPDECLINE for %s but address is not allocated to client.",
				inet_ntoa(req->req_addr));
			return;
		}
	}
	else if (aas_errno == AAS_UNKNOWN_CLIENT) {
		return;
	}
	else {
		report(LOG_ERR, "Request for client information failed: %s",
			aas_strerror(aas_errno));
		check_lost_aas_conn();
		return;
	}

	report(LOG_WARNING, "Client %s declined address %s; marking unavailable.",
		clientid2string(req), inet_ntoa(req->req_addr));
	
	/*
	 * Tell the address server to disable the address.
	 */

	addr.addr = &req->req_addr;
	addr.len = sizeof(struct in_addr);

	if (aas_disable(aas_conn, req->subnet->pool, AAS_ATYPE_INET,
	    &addr, 1) != AAS_SUCCESS) {
		report(LOG_WARNING, "Unable to disable address %s in pool %s: %s",
			inet_ntoa(req->req_addr),
			req->subnet->pool, aas_strerror(aas_errno));
		check_lost_aas_conn();
	}
}

void
proc_dhcprelease(Request *req)
{
	AasAddr addr;

	if (debug) {
		report(LOG_INFO, "Request is DHCPRELEASE");
	}

	/*
	 * We must know about the subnet and it must have a pool
	 * defined.
	 */
	
	if (!req->subnet || !req->subnet->pool) {
		return;
	}

	/*
	 * Make sure the request specifies the address being released.
	 */
	
	if (req->pkt->ciaddr.s_addr == 0) {
		report(LOG_WARNING, "DHCPRELEASE contains no address.");
		return;
	}

	/*
	 * Verify that the server ID is one of our addresses.
	 * Note: some clients don't send the server ID so we don't
	 * require it, although the RFC says that it MUST be included.
	 */
	
	if (PRESENT(req, PR_SERVER_ID)) {
		if (!check_server_id(req)) {
			return;
		}
	}

	if (!aas_conn && !open_aas_conn()) {
		return;
	}

	addr.addr = &req->pkt->ciaddr;
	addr.len = sizeof(struct in_addr);

	if (debug) {
		report(LOG_INFO, "Releasing address %s",
			inet_ntoa(req->pkt->ciaddr));
	}

	if (aas_free(aas_conn, req->subnet->pool, AAS_ATYPE_INET, &addr,
	    DHCP_SERVICE_NAME, aas_client_id(req)) != AAS_SUCCESS) {
		report(LOG_WARNING, "Unable to release address %s in pool %s: %s",
			inet_ntoa(req->pkt->ciaddr), req->subnet->pool,
			aas_strerror(aas_errno));
		check_lost_aas_conn();
	}
}

void
proc_dhcpinform(Request *req)
{
	if (debug) {
		report(LOG_INFO, "Requst is DHCPINFORM");
	}

	/*
	 * Make sure yiaddr is 0 since send_reply looks at it.
	 */

	req->pkt->yiaddr.s_addr = 0;
	send_dhcpack(req, NULL, 0);
}

/*
 * report_received -- log a message containing information about a received
 * request.
 */

void
report_received(Request *req)
{
	static char buf[1024];
	char *p;
	struct dhcp *pkt = req->pkt;
	int hlen;

	if ((hlen = pkt->hlen) > DHCP_CHADDR_LEN) {
		hlen = DHCP_CHADDR_LEN;
	}
	sprintf(buf, "Req: IF %s XID 0x%x client HW %d:%s",
		inet_ntoa(*SATOIN(&req->ifinfo.addrs[0].addr)),
		ntohl(pkt->xid),
		pkt->htype, binary2string(pkt->chaddr, hlen));
	p = buf + strlen(buf);
	if (pkt->ciaddr.s_addr != 0) {
		sprintf(p, " IP %s", inet_ntoa(pkt->ciaddr));
		p += strlen(p);
	}
	if (pkt->giaddr.s_addr != 0) {
		sprintf(p, " GW %s", inet_ntoa(pkt->giaddr));
	}
	report(LOG_INFO, "%s", buf);
}

/*
 * process_request -- receive & process a request on the DHCP port
 */

static void
process_request(void)
{
	int len;
	u_long ifindex;
	struct sockaddr_in from;
	struct sockaddr_in *dest;

	if ((len = recv_packet(endpt, request.pkt, request.pkt_buf_size,
	    &from, &ifindex)) <= 0) {
		return;
	}

	/*
	 * If this is from bootpd, just send it out.
	 */
	
	if (from.sin_addr.s_addr == bootpd_addr.sin_addr.s_addr
	    && from.sin_port == bootpd_addr.sin_port) {
		dest = (struct sockaddr_in *) request.pkt;
		if (debug) {
			report(LOG_INFO, "Forwarding BOOTP reply to %s:%d",
				inet_ntoa(dest->sin_addr),
				ntohs(dest->sin_port));
		}
		(void) send_packet(endpt, dest, dest + 1,
			len - sizeof(struct sockaddr_in));
		return;
	}

	/*
	 * Get the interface information.
	 */

	if (!get_ifinfo(ifindex, &request.ifinfo)) {
		return;
	}

	if (debug) {
		report_received(&request);
	}

	/*
	 * If the configuration is invalid, discard the request.
	 */
	
	if (!config_ok) {
		report(LOG_WARNING, "Discarding request due to invalid configuration.");
		return;
	}

	request.pkt_len = len;
	request.client = NULL;
	request.user_class = NULL;
	request.vendor_class = NULL;
	request.subnet = NULL;

	/*
	 * Extract important stuff from the request & fill in Request
	 * structure.  scan_request fails if the request is malformed.
	 */
	
	if (!scan_request(&request)) {
		return;
	}

	/*
	 * If this is a BOOTP request and we were set up to forward to
	 * bootpd, do it.
	 */
	
	if (!PRESENT(&request, PR_MSG_TYPE)) {
		if (bootp_fwd) {
			if (debug) {
				report(LOG_INFO, "Forwarding request to bootpd at %s:%d.",
					inet_ntoa(bootpd_addr.sin_addr),
					ntohs(bootpd_addr.sin_port));
			}
			(void) send_packet(endpt, &bootpd_addr, request.pkt,
				len);
		}
		else {
			if (debug) {
				report(LOG_INFO, "Discarding BOOTP request.");
			}
		}
		return;
	}

	/*
	 * Do message type-specific processing.
	 */

	switch (request.msg_type) {

	case DHCPDISCOVER:
		proc_dhcpdiscover(&request);
		break;
	
	case DHCPREQUEST:
		proc_dhcprequest(&request);
		break;
	
	case DHCPDECLINE:
		proc_dhcpdecline(&request);
		break;
	
	case DHCPRELEASE:
		proc_dhcprelease(&request);
		break;
	
	case DHCPINFORM:
		proc_dhcpinform(&request);
		break;
	
	default:
		report(LOG_WARNING, "Unexpected DHCP message type %d received; discarding...",
			request.msg_type);
		return;
	}

	return;
}

static void
reconfigure(void)
{
	if (!config_file_changed()) {
		return;
	}

	report(LOG_INFO, "Re-reading configuration file...");

	/*
	 * We have to close the AAS connection because the new configuration
	 * may specify a different AAS location.
	 */

	if (aas_conn) {
		aas_close(aas_conn);
		aas_conn = NULL;
	}

	configure();
}

/*
 * proto -- do DHCP on given port
 * proto listens for and processes requests sent to the given UDP port.
 * If no packets are received for a period of timeout minutes, we exit.
 * If timeout is 0, we don't time out.
 * This function also looks for activity on the address sevrer connection
 * if there is a request waiting for something from the address server.
 * proto exits if unable to set up an endpoint to listen on.
 */

void
proto(int timeout)
{
	struct pollfd pfd;
	struct servent *sp;
	int on;

	/*
	 * Get port numbers.
	 */
	
	if (sp = getservbyname("bootps", "udp")) {
		bootps_port = sp->s_port;
	}
	else {
		report(LOG_WARNING, "Unknown service \"bootps\"; using default port %d",
			IPPORT_BOOTPS);
		bootps_port = htons((u_short) IPPORT_BOOTPS);
	}

	if (sp = getservbyname("bootpc", "udp")) {
		bootpc_port = sp->s_port;
	}
	else {
		report(LOG_WARNING, "Unknown service \"bootpc\"; using default port %d",
			IPPORT_BOOTPC);
		bootpc_port = htons((u_short) IPPORT_BOOTPC);
	}

	/*
	 * Create a UDP endpoint if we're running standalone.
	 */

	if (standalone) {
		if ((endpt = get_endpt(bootps_port)) == -1) {
			done(1);
		}
	}
	else {
		endpt = 0;
	}

	/*
	 * Set options to receive interface index with received packets
	 * and to enable broadcasting.
	 */
	
	on = 1;
	if (set_endpt_opt(endpt, IPPROTO_IP, IP_RECVIFINDEX, &on,
	    sizeof(int)) == -1) {
		report(LOG_ERR, "Unable to set IP_RECVIFINDEX option.");
		done(1);
	}
	if (set_endpt_opt(endpt, SOL_SOCKET, SO_BROADCAST, &on,
	    sizeof(int)) == -1) {
		report(LOG_ERR, "Unable to set SO_BROADCAST option.");
		done(1);
	}

	/*
	 * Allocate a buffer for receiving packets.
	 */
	
	if (!(request.pkt = malloc(PKT_BUF_SIZE))) {
		malloc_error("Unable to allocate buffer for receiving packets.");
		done(1);
	}

	request.pkt_buf_size = PKT_BUF_SIZE;

	/*
	 * Open an IP device for doing ioctl's
	 */

	if ((ip_fd = open(_PATH_IP, O_RDONLY)) == -1) {
		report(LOG_ERR, "Unable to open IP driver %s: %m", _PATH_IP);
		done(1);
	}

	/*
	 * Initialize the option_defaults table.
	 */
	
	init_option_defaults();

	/*
	 * Convert timeout from minutes to milliseconds
	 */
	
	if (timeout == 0) {
		timeout = -1;
	}
	else {
		timeout *= 60000;
	}

	/*
	 * Set up pollfd structure.
	 */

	pfd.fd = endpt;
	pfd.events = POLLIN | POLLPRI;

	/*
	 * Wait for & process requests and activity on the address
	 * server connection.
	 */
	
	for (;;) {
		if (poll(&pfd, 1, timeout) == -1) {
			if (errno == EINTR) {
				continue;
			}
			else if (errno == EAGAIN) {
				report(LOG_WARNING, "poll: %m; retrying...");
				sleep(5);
				continue;
			}
			else {
				report(LOG_ERR, "poll failed: %m; exiting");
				done(1);
			}
		}

		/*
		 * If no packet was received, we timed out, so we're
		 * outta here.
		 */
		
		if (pfd.revents == 0) {
			report(LOG_INFO, "Exiting after %d minutes of inactivity.",
				timeout/60000);
			break;
		}

		/*
		 * Re-read the configuration file if it has changed.
		 */
		
		reconfigure();

		process_request();
	}

	done(0);
}
