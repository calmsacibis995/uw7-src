#ident "@(#)bootpgw.c	1.2"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1994 Lachman Technology, Inc.
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
/*      SCCS IDENTIFICATION        */
/*
 * bootpgw.c - BOOTP GateWay
 * This program forwards BOOTP Request packets to a BOOTP server.
 */

/************************************************************************
          Copyright 1988, 1991 by Carnegie Mellon University

                          All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted, provided
that the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation, and that the name of Carnegie Mellon University not be used
in advertising or publicity pertaining to distribution of the software
without specific, written prior permission.

CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
************************************************************************/

#ifndef lint
static char rcsid[] = "/proj/tcp/6.0/lcvs/usr/common/usr/src/cmd/net/bootpd/bootpgw.c,v 6.3 1996/01/08 21:28:11 aes Exp";
#endif

/*
 * BOOTPGW is typically used to forward BOOTP client requests from
 * one subnet to a BOOTP server on a different subnet.
 */

/*
 * CHANGES TO SUPPORT DHCP
 * bootpgw has been modified so that it can correctly function as a relay
 * agent for DHCP messages as well as regular BOOTP messages.  Specifically,
 * the following changes have been made:
 *
 * + bootpgw now sets the (new) IP_RECVIFINDEX option on the UDP endpoint,
 *   which causes UDP to send the index of the receiving interface as an
 *   option with each received packet.  This allows the giaddr field to
 *   be filled in with the relay agent's address on the client's interface,
 *   which was not possible before.  DHCP depends on the giaddr field
 *   correctly identifying the client's subnet.
 *
 * + In order for the above to work, TLI must be used instead of sockets
 *   on the GIS platform, since, unlike the SCO version, GIS's recvmsg
 *   does not return options.  The sockets interface is still used in the
 *   SCO version because inetd only supports sockets-based servers (GIS has
 *   tlid, which is a TLI version of inetd).  The file endpt.c contains
 *   sockets and TLI versions of several generic endpoint routines, which
 *   this code now calls instead of the original socket functions.
 *   Also, poll is used instead of select since it works in both the sockets
 *   and TLI worlds.
 *
 * + To support interfaces with mutliple addresses, the -a option has been
 *   added.  This allows the specification of the address to use for filling
 *   in giaddr when an interface has more than one address.  Any number
 *   of -a options can be given (up to 32).
 *
 * + bootpgw now recognizes the BROADCAST (0x8000) flag in the flags field
 *   (a.k.a the "unused" field) of the BOOTP/DHCP packet.  If this flag is
 *   set in a BOOTREPLY, the packet is sent to the limited broadcast address
 *   (255.255.255.255).  This requires the use of the (also new)
 *   IP_BROADCAST_IF option, which allows the specification of the interface
 *   to which to direct packets sent to 255.255.255.255.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>	/* inet_ntoa */

#ifndef	NO_UNISTD
#include <unistd.h>
#endif
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <syslog.h>
#include <paths.h>
#include <stropts.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/sockio.h>


#include <arpa/bootp.h>
#include "getif.h"
#include "hwaddr.h"
#include "report.h"
#include "endpt.h"
#include "patchlevel.h"

/* Local definitions: */
#define MAXPKT			(3*512) /* Maximum packet size */
#define TRUE 1
#define FALSE 0
#define get_network_errmsg get_errmsg
typedef unsigned char byte;

/*
 * The "unused" field in the original BOOTP packet definition
 * is now the "flags" field
 */

#define bp_flags	bp_unused
#define BROADCAST_FLAG	0x8000



/*
 * Externals, forward declarations, and global variables
 */

#ifdef	__STDC__
#define P(args) args
#else
#define P(args) ()
#endif

static void usage P((void));
static void handle_reply P((void));
static void handle_request P((void));

#undef	P

/*
 * IP port numbers for client and server obtained from /etc/services
 */

u_short bootps_port, bootpc_port;


/*
 * Internet socket and interface config structures
 */

struct sockaddr_in bind_addr;	/* Listening */
struct sockaddr_in recv_addr;	/* Packet source */
struct sockaddr_in send_addr;	/*  destination */


/*
 * option defaults
 */
int debug = 0;			/* Debugging flag (level) */
struct timeval actualtimeout = { /* fifteen minutes */
	15 * 60L,   /* tv_sec */
	0          /* tv_usec */
};
u_int maxhops = 4;		/* Number of hops allowed for requests. */
u_int minwait = 3;		/* Number of seconds client must wait before
						   its bootrequest packets are forwarded. */

/*
 * The following stores a list of addresses given on the command line with -a.
 * This list is used to decide among multiple addresses on an interface
 * when filling in the giaddr field.  If an interface has multiple addresses,
 * the first one that was given on the command line is used.  If none were
 * given on the command line, the first address is used.
 */

#define MAX_ADDRESSES	32	/* this should be way more than enough */
struct in_addr addresses[MAX_ADDRESSES];
int num_addresses;

/*
 * General
 */

int s;					/* Socket file descriptor */
char *pktbuf;			/* Receive packet buffer */
int pktlen;
char *progname;
char *servername;
long server_ipa;		/* Real server IP address, network order. */

u_long ifindex;			/* index of receiving interface */

int ip_fd;			/* file descriptor of open IP device */




/*
 * Initialization such as command-line processing is done and then the
 * main server loop is started.
 */

void
main(argc, argv)
	int argc;
	char **argv;
{
	struct timeval *timeout;
	struct bootp *bp;
	struct servent *servp;
	struct hostent *hep;
	int n, ba_len, ra_len;
	int standalone;
	int c;
	int on;
	int poll_timeout;
	struct pollfd pfd;
	struct in_addr addr;
	extern int optind;
	extern char *optarg;

	progname = strrchr(argv[0],'/');
	if (progname) progname++;
	else progname = argv[0];

	/*
	 * Initialize logging.
	 */
	report_init(0);	/* uses progname */

	/*
	 * Log startup
	 */
	report(LOG_INFO, "version %s.%d", VERSION, PATCHLEVEL);

	/* Get space for receiving packets and composing replies. */
	pktbuf = malloc(MAXPKT);
	if (!pktbuf) {
		report(LOG_ERR, "malloc failed");
		exit(1);
	}
	bp = (struct bootp *) pktbuf;

	/*
	 * Check to see if a socket was passed to us from inetd.
	 *
	 * Use is_endpt() to determine if descriptor 0 is indeed a socket
	 * (and thus we are probably a child of inetd) or if it is instead
	 * something else and we are running standalone.
	 */
	s = 0;
	ba_len = sizeof(bind_addr);
	bzero((char *) &bind_addr, ba_len);
	errno = 0;
	standalone = TRUE;
	if (is_endpt(s, &bind_addr)) {
		/*
		 * Descriptor 0 is a socket.  Assume we are a child of inetd.
		 */
		if (bind_addr.sin_family == AF_INET) {
			standalone = FALSE;
			bootps_port = ntohs(bind_addr.sin_port);
		} else {
			/* Some other type of socket? */
			report(LOG_INFO, "getsockname: not an INET socket");
		}
	}

	/*
	 * Set defaults that might be changed by option switches.
	 */
	timeout = &actualtimeout;

	num_addresses = 0;

	/*
	 * Read switches.
	 */
	while ((c = getopt(argc, argv, "dD:t:w:ish:a:")) != EOF) {
		switch (c) {

		case 'd':		/* debug level */
			/*
			 * Backwards-compatible behavior:
			 * no parameter, so just increment the debug flag.
			 */
			debug++;
			break;
		case 'D':
			if ((sscanf(optarg, "%d", &n) != 1) || (n < 0)) {
				fprintf(stderr,
					"%s: invalid debug level: %s\n", 
					progname, optarg);
				break;
			}
			debug = n;
			break;

		case 'h':		/* hop count limit */
			if ((sscanf(optarg, "%d", &n) != 1) ||
				(n < 0) || (n > 16) )
			{
				fprintf(stderr,
				"bootpgw: invalid hop count limit: %s\n",
				optarg);
				break;
			}
			maxhops = (u_int)n;
			break;

		case 'i':		/* inetd mode */
			standalone = FALSE;
			break;

		case 's':		/* standalone mode */
			standalone = TRUE;
			break;

		case 't':		/* timeout */
			if ((sscanf(optarg, "%d", &n) != 1) || (n < 0)) {
				fprintf(stderr,
				"%s: invalid timeout specification: %s\n", 
				progname, optarg);
				break;
			}
			actualtimeout.tv_sec = (long) (60 * n);
			/*
			 * If the actual timeout is zero, pass a NULL pointer
			 * to select so it blocks indefinitely, otherwise,
			 * point to the actual timeout value.
			 */
			timeout = (n > 0) ? &actualtimeout : NULL;
			break;

		case 'w':		/* wait time */
			if ((sscanf(optarg, "%d", &n) != 1) ||
				(n < 0) || (n > 60) )
			{
				fprintf(stderr,
					"bootpgw: invalid wait time: %s\n",
						optarg);
				break;
			}
			minwait = (u_int)n;
			break;
		
		case 'a':
			if (num_addresses == MAX_ADDRESSES) {
				fprintf(stderr, "bootpgw: too many -a options (max is %d)\n",
					MAX_ADDRESSES);
				break;
			}
			if (!inet_aton(optarg, &addr)) {
				if (!(hep = gethostbyname(optarg))) {
					fprintf(stderr, "bootpgw: unknown host %s\n",
						optarg);
					break;
				}
				bcopy(hep->h_addr, (char *) &addr,
					sizeof(addr));
			}
			addresses[num_addresses++] = addr;
			break;

		default:
			usage();
			break;

		} /* switch */
	} /* while*/

	argc -= optind;
	argv += optind;

	/* Make sure server name argument is suplied. */
	servername = argv[0];
	if (!servername) {
		fprintf(stderr, "bootpgw: missing server name\n");
		usage();
	}

	/*
	 * Get address of real bootp server.
	 */
	if (isdigit(servername[0]))
		server_ipa = inet_addr(servername);
	else {
		hep = gethostbyname(servername);
		if (!hep) {
			fprintf(stderr, "bootpgw: can't get addr for %s\n", servername);
			exit(1);
		}
		bcopy(hep->h_addr, (char*)&server_ipa, sizeof(server_ipa));
	}

	if (standalone) {
		/*
		 * Go into background and disassociate from controlling terminal.
		 * XXX - This is not the POSIX way (Should use setsid). -gwr
		 */
		if (debug < 3) {
			if (fork())
				exit(0);
#if 0
			/* XXX - This closes the syslog socket! */
			for (n = 0; n < 10; n++)
				(void) close(n);
#endif
#if	0	/* This is unnecessary. -gwr */
			(void) open("/", O_RDONLY);
			(void) dup2(0, 1);
			(void) dup2(0, 2);
#endif
#ifdef SETSID
            if (setsid() < 0)
                perror("setsid");
#endif
#ifdef TIOCNOTTY
			n = open("/dev/tty", O_RDWR);
			if (n >= 0) {
				ioctl(n, TIOCNOTTY, (char *) 0);
				(void) close(n);
			}
#endif
		} /* if debug < 3 */

		/*
		 * Nuke any timeout value
		 */
		timeout = NULL;

		/*
		 * Here, bootpd would do:
		 *	chdir
		 *	tzone_init
		 *	rdtab_init
		 *	readtab
		 */

		/*
		 * Get server's listening port number.
		 */
		servp = getservbyname("bootps", "udp");
		if (servp) {
			bootps_port = ntohs((u_short) servp->s_port);
		} else {
			bootps_port = (u_short) IPPORT_BOOTPS;
			report(LOG_ERR,
				   "udp/bootps: unknown service -- assuming port %d",
				   bootps_port);
		}

		/*
		 * Create & bind UDP endpoint.
		 */

		if ((s = get_endpt(htons(bootps_port))) == -1) {
			exit(1);
		}

	} /* if standalone */

	/*
	 * Set option to receive interface index with packets.
	 */
	
	on = 1;
	if (set_endpt_opt(s, IPPROTO_IP, IP_RECVIFINDEX, &on,
	    sizeof(int)) == -1) {
		report(LOG_ERR, "Unable to set IP_RECVIFINDEX option.");
		exit(1);
	}

	/*
	 * Get destination port number so we can reply to client
	 */
	servp = getservbyname("bootpc", "udp");
	if (servp) {
		bootpc_port = ntohs(servp->s_port);
	} else {
		report(LOG_ERR,
			   "udp/bootpc: unknown service -- assuming port %d",
			   IPPORT_BOOTPC);
		bootpc_port = (u_short) IPPORT_BOOTPC;
	}

	/*
	 * Open an IP device for doing ioctl's
	 */

	if ((ip_fd = open(_PATH_IP, O_RDONLY)) == -1) {
		report(LOG_ERR, "Unable to open IP driver %s: %s",
			_PATH_IP, get_errmsg());
		exit(1);
	}

	/* no signal catchers */

	/*
	 * Set up data structure for poll.  We use poll instead of select
	 * because poll works with both sockets and TLI endpoints, and this
	 * program now has to work both ways (see "CHANGES TO SUPPORT DHCP"
	 * above).
	 */
	
	pfd.fd = s;
	pfd.events = POLLIN | POLLPRI;

	if (timeout) {
		poll_timeout = timeout->tv_sec * 1000;
	}
	else {
		poll_timeout = -1;
	}

	/*
	 * Process incoming requests.
	 */
	for (;;) {
		if (poll(&pfd, 1, poll_timeout) == -1) {
			if (errno != EINTR) {
				report(LOG_ERR, "poll: %s", get_errmsg());
			}
			continue;
		}
		if (pfd.revents == 0) {
			report(LOG_INFO, "exiting after %ld minutes of inactivity",
				   actualtimeout.tv_sec / 60);
			exit(0);
		}
		ra_len = sizeof(recv_addr);
		n = recv_packet(s, pktbuf, MAXPKT, &recv_addr, &ifindex);
		if (n <= 0) {
			continue;
		}

		if (debug > 3) {
			report(LOG_INFO, "recvd pkt from IP addr %s",
				   inet_ntoa(recv_addr.sin_addr));
		}

		if (n < sizeof(struct bootp)) {
			if (debug) {
				report(LOG_INFO, "received short packet");
			}
			continue;
		}
		pktlen = n;

		switch (bp->bp_op) {
		case BOOTREQUEST:
			handle_request();
			break;
		case BOOTREPLY:
			handle_reply();
			break;
		}
	}
}




/*
 * Print "usage" message and exit
 */

static void
usage()
{
	fprintf(stderr,
			"usage:  bootpgw [-d] [-c dir] [-D level] [-i] [-s] [-t timeout] [-a address] server\n");
	fprintf(stderr, "\t -d\tincrement debug level\n");
	fprintf(stderr, "\t -D n\tset debug level\n");
	fprintf(stderr, "\t -p dir\tset default transfer diretory (obsolete)\n");
	fprintf(stderr, "\t -c dir\tset default transfer diretory (obsolete)\n");
	fprintf(stderr, "\t -h n\tset max hop count\n");
	fprintf(stderr, "\t -i\tforce inetd mode (run as child of inetd) (obsolete)\n");
	fprintf(stderr, "\t -s\tforce standalone mode (run without inetd) (obsolete)\n");
	fprintf(stderr, "\t -t n\tset inetd exit timeout to n minutes\n");
	fprintf(stderr, "\t -w n\tset min wait time (secs)\n");
	fprintf(stderr, "\t -a n\tspecify an address to use for interfaces with multiple addresses\n");
	exit(1);
}



/*
 * get_if_addr -- return the IP address for a given interface index
 * get_if_addr does a SIOCGIFALL to obtain the information for the
 * given interface and returns the appropriate address.  If the interface
 * has multiple addresses, we use the first one that matches an address
 * given with the -a option, or, if none match (or no -a options were
 * present), the first address is used.
 * Returns 1 if ok, or 0 if the ioctl fails.
 */

int
get_if_addr(u_long ifindex, struct in_addr *addr)
{
	struct strioctl sioc;
	struct ifreq_all ifinfo;
	struct sockaddr_in *sin;
	int i, j;

	sioc.ic_cmd = SIOCGIFALL;
	sioc.ic_timout = -1;
	sioc.ic_len = sizeof(struct ifreq_all);
	sioc.ic_dp = (char *) &ifinfo;

	ifinfo.if_number = ifindex;

	if (ioctl(ip_fd, I_STR, &sioc) == -1) {
		report(LOG_ERR, "SIOCGIFALL failed: %s", get_errmsg());
		return 0;
	}

	for (i = 0; i < num_addresses; i++) {
		for (j = 0; j < ifinfo.if_naddr; j++) {
			sin = (struct sockaddr_in *) &ifinfo.addrs[j].addr;
			if (sin->sin_addr.s_addr == addresses[i].s_addr) {
				*addr = addresses[i];
				return 1;
			}
		}
	}

	/*
	 * None were specifically selected, so use the first one.
	 */

	*addr = ((struct sockaddr_in *) &ifinfo.addrs[0].addr)->sin_addr;
}

/*
 * Process BOOTREQUEST packet.
 *
 * Note, this just forwards the request to a real server.
 */
static void
handle_request()
{
	struct bootp *bp = (struct bootp *) pktbuf;
	struct ifreq *ifr;
	u_short secs, hops;

	/* XXX - SLIP init: Set bp_ciaddr = recv_addr here? */

	if (debug) {
		report(LOG_INFO, "request from %s",
			   inet_ntoa(recv_addr.sin_addr));
	}

	/* Has the client been waiting long enough? */
	secs = ntohs(bp->bp_secs);
	if (secs < minwait)
		return;

	/* Has this packet hopped too many times? */
	hops = ntohs(bp->bp_hops);
	if (++hops > maxhops) {
		report(LOG_NOTICE, "reqest from %s reached hop limit",
			   inet_ntoa(recv_addr.sin_addr));
		return;
	}
	bp->bp_hops = htons(hops);

	/*
	 * Here one might discard a request from the same subnet as the
	 * real server, but we can assume that the real server will send
	 * a reply to the client before it waits for minwait seconds.
	 */

	/* If gateway address is not set, put in local interface addr. */
	if (bp->bp_giaddr.s_addr == 0) {
		/*
		 * Due to a miracle of modern technology, we are now able
		 * to tell where a packet came from because UDP gives us
		 * the if_index of the receiving interface.  get_if_addr
		 * translates this to an IP address.
		 */
		get_if_addr(ifindex, &bp->bp_giaddr);

		/*
		 * XXX - DHCP says to insert a subnet mask option into the
		 * options area of the request (if vendor magic == std).
		 */
	}

	/* Set up socket address for send. */
	send_addr.sin_family = AF_INET;
	send_addr.sin_port = htons(bootps_port);
	send_addr.sin_addr.s_addr = server_ipa;

	/* Send reply with same size packet as request used. */
	(void) send_packet(s, &send_addr, pktbuf, pktlen);
}



/*
 * Process BOOTREPLY packet.
 */
static void
handle_reply()
{
	struct bootp *bp = (struct bootp *) pktbuf;
	struct sockaddr_in *sip;
	struct in_addr ifaddr;
	struct ifreq *ifr;
	int on;

	if (debug) {
		report(LOG_INFO, "   reply for %s",
			   inet_ntoa(bp->bp_yiaddr));
	}

	/* Set up socket address for send to client. */
	send_addr.sin_family = AF_INET;
	send_addr.sin_port = htons(bootpc_port);

	/*
	 * If the broadcast flag is set in the packet, we have to
	 * broadcast the reply to the client.
	 */
	
	if (htons(bp->bp_flags) & BROADCAST_FLAG) {
		/*
		 * Determine the interface to use for broadcasting.
		 * If giaddr is set, use that.  If not, use yiaddr.
		 * If neither of these is set, we can't tell where to
		 * send the packet, so we just drop it.
		 */
		if (bp->bp_giaddr.s_addr != 0) {
			ifaddr = bp->bp_giaddr;
		}
		else if (ifr = getif(s, &(bp->bp_yiaddr))) {
			ifaddr = ((struct sockaddr_in *) &ifr->ifr_addr)->sin_addr;
		}
		else {
			report(LOG_WARNING, "Unable to determine network for reply with BROADCAST set.");
			return;
		}
		send_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
		/*
		 * Set the broadcast option so we can broadcast.
		 */
		on = 1;
		if (set_endpt_opt(s, SOL_SOCKET, SO_BROADCAST, &on,
		    sizeof(int)) == -1) {
			report(LOG_ERR, "Unable to set broadcast option: %s",
				get_errmsg());
			return;
		}
		/*
		 * Use the interface address determined above to direct the
		 * broadcast to the appropriate interface.
		 */
		if (set_endpt_opt(s, IPPROTO_IP, IP_BROADCAST_IF,
		    &ifaddr, sizeof(struct in_addr)) == -1) {
			report(LOG_ERR, "Unable to set broadcast interface to %s: %s",
				inet_ntoa(ifaddr), get_errmsg());
			return;
		}
	}
	else {
		send_addr.sin_addr = bp->bp_yiaddr;
		/* Create an ARP cache entry for the client. */
		if (debug > 1)
			report(LOG_INFO, "setarp %s - %s",
				   inet_ntoa(bp->bp_yiaddr),
				   haddrtoa(bp->bp_chaddr, bp->bp_hlen));
		setarp(s, &bp->bp_yiaddr, bp->bp_chaddr, bp->bp_hlen);
	}

	/* Send reply with same size packet as request used. */
	(void) send_packet(s, &send_addr, pktbuf, pktlen);
}

/*
 * Local Variables:
 * tab-width: 4
 * End:
 */
