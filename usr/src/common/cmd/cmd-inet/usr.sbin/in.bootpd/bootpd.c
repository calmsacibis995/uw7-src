#ifndef _BLURB_
#define _BLURB_
/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

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
#endif /* _BLURB_ */

#ident	"@(#)bootpd.c	1.3"

#ifndef lint
static char rcsid[] = "$Header$";
#endif


/*
 * BOOTP (bootstrap protocol) server daemon.
 *
 * Answers BOOTP request packets from booting client machines.
 * See [SRI-NIC]<RFC>RFC951.TXT for a description of the protocol.
 * See [SRI-NIC]<RFC>RFC1048.TXT for vendor-information extensions.
 * See RFC 1395 for option tags 14-17.
 * See accompanying man page -- bootpd.8
 *
 * HISTORY
 *	See ./Changes
 *
 * BUGS
 *	See ./ToDo
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

#ifdef SVR4
# include <fcntl.h>		/* for O_RDONLY, etc */
# define	SETSID
# include <memory.h>
/* Yes, memcpy is OK here (no overlapped copies). */
# define bcopy(a,b,c)    memcpy(b,a,c)
# define bzero(p,l)      memset(p,0,l)
# define bcmp(a,b,c)     memcmp(a,b,c)
# define index           strchr
#endif

#include <arpa/bootp.h>
#include "hash.h"
#include "bootpd.h"
#include "dovend.h"
#include "getif.h"
#include "hwaddr.h"
#include "readfile.h"
#include "report.h"
#include "tzone.h"
#include "patchlevel.h"
#include "pathnames.h"

/* Local definitions: */
#define MAXPKT			(3*512) /* Maximum packet size */

/*
 * Externals, forward declarations, and global variables
 */

#ifdef	__STDC__
#define P(args) args
#else
#define P(args) ()
#endif

extern void dumptab P((char*));

PRIVATE void catcher P((int));
PRIVATE int  chk_access P((char *, long *));
#ifdef VEND_CMU
PRIVATE void dovend_cmu P((struct bootp *, struct host *));
#endif
PRIVATE void dovend_rfc1048 P((struct bootp *, struct host *, long));
PRIVATE void handle_reply P((void));
PRIVATE void handle_request P((void));
PRIVATE void sendreply P((int forward, long dest_override));
PRIVATE void usage P((void));

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

/*
 * If slave mode is set, bootpd listens at the specified port (or the already
 * bound port in inetd mode), where dhcpd forwards BOOTP packets.  bootpd
 * then sends the packets back to dhcpd, which sends them out on the network
 * (this is done so that the source port is bootps (67)).  slave_dest
 * points to the beginning of the buffer allocated for packets (before
 * pktbuf) -- this is where the destination address is stored.
 * slave_fowarder is the address that we send packets to in slave mode.
 */

int slave = 0;
u_short slave_port = 0;
struct sockaddr_in *slave_dest;
struct sockaddr_in slave_forwarder;

/*
 * General
 */

int s;					/* Socket file descriptor */
char *pktbuf;			/* Receive packet buffer */
int pktlen;
char *progname;
char *chdir_path;
char hostname[MAXHOSTNAMELEN];	/* System host name */
struct in_addr my_ip_addr;

/* Flags set by signal catcher. */
PRIVATE int do_readtab = 0;
PRIVATE int do_dumptab = 0;

/*
 * Globals below are associated with the bootp database file (bootptab).
 */

char *bootptab = _PATH_CONFIG;
char *bootpd_dump = _PATH_DUMP;



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
	int n;
	size_t ba_len, ra_len;
	int nfound;
	fd_set readfds;
	int standalone;
	int c, val;
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
	/*
	 * In case we're going to run in slave mode, we allocate an
	 * extra sizeof(struct sockaddr_in) bytes, where we will store
	 * the packet's destination address & port.
	 */
	pktbuf = malloc(MAXPKT + sizeof(struct sockaddr_in));
	if (!pktbuf) {
		report(LOG_ERR, "malloc failed");
		exit(1);
	}
	slave_dest = (struct sockaddr_in *) pktbuf;
	pktbuf += sizeof(struct sockaddr_in);
	bp = (struct bootp *) pktbuf;

	/*
	 * Check to see if a socket was passed to us from inetd.
	 *
	 * Use getsockname() to determine if descriptor 0 is indeed a socket
	 * (and thus we are probably a child of inetd) or if it is instead
	 * something else and we are running standalone.
	 */
	s = 0;
	ba_len = sizeof(bind_addr);
	bzero((char *) &bind_addr, ba_len);
	errno = 0;
	standalone = TRUE;
	if (getsockname(s, (struct sockaddr *) &bind_addr, &ba_len) == 0) {
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

	/*
	 * Read switches.
	 */
	while ((c = getopt(argc, argv, "c:p:dD:ist:SP:")) != EOF) {
		switch (c) {

		case 'p':		/* chdir_path */
		case 'c':		/* chdir_path */
			if (*optarg != '/') {
				fprintf(stderr,
				"bootpd: invalid chdir specification: %s\n",
				optarg);
				break;
			}
			chdir_path = optarg;
			break;

		case 'd':		/* debug level */
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

		case 'i':		/* inetd mode */
			standalone = FALSE;
			break;

		case 's':		/* standalone mode */
			standalone = TRUE;
			break;

		case 't':		/* timeout */
			if ((sscanf(optarg, "%d", &n) != 1) || (n < 0)) {
				fprintf(stderr,
						"%s: invalid timeout specification: %s\n", progname, optarg);
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
		case 'S':
			slave = 1;
			break;
		case 'P':
			if (servp = getservbyname(optarg, "udp")) {
				slave_port = ntohs((u_short) servp->s_port);
			}
			else if (sscanf(optarg, "%d", &val) == 1
			    && val > 0 && val <= 65535) {
				slave_port = val;
			}
			else {
				fprintf(stderr, "%s: invalid port: %s\n",
					progname, optarg);
			}
			break;
		case '?':
		default:
			usage();
			break;

		} /* switch */
	} /* while */

	argc -= optind;
	argv += optind;

	/*
	 * Override default file names if specified on the command line.
	 */
	if (argc > 0)
		bootptab = argv[0];

	if (argc > 1)
		bootpd_dump = argv[1];

	/*
	 * Get my hostname and IP address.
	 */
	if (gethostname(hostname, sizeof(hostname)) == -1) {
		fprintf(stderr, "bootpd: can't get hostname\n");
		exit(1);
	}
	hep = gethostbyname(hostname);
	if (!hep) {
		fprintf(stderr, "Can not get my IP address\n");
		exit(1);
	}
	bcopy(hep->h_addr, (char*)&my_ip_addr, sizeof(my_ip_addr));

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

	} /* if standalone (1st) */

	/* Set the cwd (i.e. to /tftpboot) */
	if (chdir_path) {
		if (chdir(chdir_path) < 0)
			report(LOG_ERR, "%s: chdir failed", chdir_path);
	}

	/* Get the timezone. */
	tzone_init();

	/* Allocate hash tables. */
	rdtab_init();

	/*
	 * Read the bootptab file.
	 */
	readtab(1); /* force read */

	if (standalone) {

		/*
		 * Create a socket.
		 */
		if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			report(LOG_ERR, "socket: %s", get_network_errmsg());
			exit(1);
		}

		/*
		 * Get server's listening port number
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
		 * Bind socket to BOOTPS port, unless we're running in
		 * slave mode, in which case we use the specified port.
		 */
		bind_addr.sin_family = AF_INET;
		bind_addr.sin_addr.s_addr = INADDR_ANY;
		if (slave) {
			if (slave_port == 0) {
				servp = getservbyname("bootps-alt", "udp");
				if (servp) {
					slave_port = ntohs((u_short) servp->s_port);
				} else {
					slave_port = (u_short) IPPORT_BOOTPS_ALT;
					report(LOG_ERR,
				   		"udp/bootps: unknown service -- assuming port %d",
						 slave_port);
				}
			}
			bind_addr.sin_port = htons(slave_port);
		} else {
			bind_addr.sin_port = htons(bootps_port);
		}
		if (bind(s, (struct sockaddr *) &bind_addr,
				 sizeof(bind_addr)) < 0)
		{
			report(LOG_ERR, "bind: %s", get_network_errmsg());
			exit(1);
		}

	} /* if standalone (2nd)*/

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
	 * Set up things for slave mode.
	 */

	if (slave) {
		/*
		 * If we're running from inetd, we can't use the port
		 * that was handed to us as the bootps port, since it's
		 * some random alternate port.  In that case, we
		 * get the real bootps port.
		 */
		if (!standalone) {
			servp = getservbyname("bootps", "udp");
			if (servp) {
				bootps_port = ntohs((u_short) servp->s_port);
			} else {
				bootps_port = (u_short) IPPORT_BOOTPS;
				report(LOG_ERR, "udp/bootps: unknown service -- assuming port %d", bootps_port);
			}
		}
		memset(&slave_forwarder, 0, sizeof(struct sockaddr_in));
		slave_forwarder.sin_family = AF_INET;
		slave_forwarder.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		slave_forwarder.sin_port = htons(bootps_port);
	}

	/*
	 * Set up signals to read or dump the table.
	 */
	if ((int) sigset(SIGHUP, catcher) < 0) {
		report(LOG_ERR, "signal: %s", get_errmsg());
		exit(1);
	}
	if ((int) sigset(SIGUSR1, catcher) < 0) {
		report(LOG_ERR, "signal: %s", get_errmsg());
		exit(1);
	}

	/*
	 * Process incoming requests.
	 */
	FD_ZERO(&readfds);
	for (;;) {
		FD_SET(s, &readfds);
		nfound = select(s + 1, &readfds, NULL, NULL, timeout);
		if (nfound < 0) {
			if (errno != EINTR) {
				report(LOG_ERR, "select: %s", get_errmsg());
			}
			/*
			 * Call readtab() or dumptab() here to avoid the
			 * dangers of doing I/O from a signal handler.
			 */
			if (do_readtab) {
				do_readtab = 0;
				readtab(1); /* force read */
			}
			if (do_dumptab) {
				do_dumptab = 0;
				dumptab(bootpd_dump);
			}
			continue;
		}
		if (!FD_ISSET(s, &readfds)) {
			report(LOG_INFO, "exiting after %ld minutes of inactivity",
				   actualtimeout.tv_sec / 60);
			exit(0);
		}
		ra_len = sizeof(recv_addr);
		n = recvfrom(s, pktbuf, MAXPKT, 0,
					 (struct sockaddr *) &recv_addr, &ra_len);
		if (n <= 0) {
			continue;
		}

		if (debug > 1) {
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

		readtab(0);	/* maybe re-read bootptab */

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

PRIVATE void
usage()
{
	fprintf(stderr,
			"usage:  bootpd [-D level] [-c dir] [-d] [-i] [-s] [-t timeout] [configfile [dumpfile]]\n");
	fprintf(stderr, "\t -c dir\tset current directory\n");
	fprintf(stderr, "\t -p dir\tset current directory (obsolete)\n");
	fprintf(stderr, "\t -d\tincrement debug level (obsolete)\n");
	fprintf(stderr, "\t -D n\tset debug level\n");
	fprintf(stderr, "\t -i\tforce inetd mode (run as child of inetd) (obsolete)\n");
	fprintf(stderr, "\t -s\tforce standalone mode (run without inetd) (obsolete)\n");
	fprintf(stderr, "\t -t n\tset inetd exit timeout to n minutes\n");
	fprintf(stderr, "\t -S\trun in DHCP slave mode\n");
	fprintf(stderr, "\t -P port\tspecify DHCP slave mode port\n");
	exit(1);
}

/* Signal catchers */
PRIVATE void
catcher(sig)
	int sig;
{
	if (sig == SIGHUP)
		do_readtab = 1;
	if (sig == SIGUSR1)
		do_dumptab = 1;
}

/*
 * Look in bootp request for magic tag and host name and lookup IP address.
 * Set *result.  Return non-zero on failure.
 */
int
lookup_by_vend_name(bp, result)
    struct bootp *bp;
    u_long *result;
{
	char byname_magic[] = BYNAME_MAGIC;
    int i, n, gotbyname = 0;
	char *hostptr = NULL;

    /* verify vend is rfc1048 */
    if (bcmp(bp->bp_vend, vm_rfc1048, 4))
		return(-1);

    /* Parse vend area looking for magic tag and host name. */
	for(i=4; i<BP_VEND_LEN; i++)
	switch (bp->bp_vend[i]) {
		  case TAG_PAD:
			break;
		  case BYNAME_TAG:
			n = strlen(byname_magic);       /* magic tag length */
			if (bp->bp_vend[++i] != n)
			  return(-1);
			if (bcmp(bp->bp_vend+i+1, byname_magic, n))
			  return(-1);
			gotbyname++;
			i += n;	/* n == bp_vend[++i] */
		  case TAG_HOST_NAME:
			hostptr = (char *)bp->bp_vend+i+2;
			i += bp->bp_vend[i+1]+1;
			break;
		  case TAG_END:
			i = BP_VEND_LEN;	/* skip remaining */
			break;
		  default: /* unused tag */
			i += bp->bp_vend[i+1]+1;	/* skip this tag */
			break;
	}

    /* lookup hostname if we should */
    if (gotbyname && hostptr)
		return(lookup_ipa(hostptr, result));
    return(-1);
}




/*
 * Process BOOTREQUEST packet.
 *
 * Note:  This version of the bootpd.c server never forwards
 * a request to another server.  That is the job of a gateway
 * program such as the "bootpgw" program included here.
 *
 * (Also this version does not interpret the hostname field of
 * the request packet;  it COULD do a name->address lookup and
 * forward the request there.)
 */
PRIVATE void
handle_request()
{
	struct bootp *bp = (struct bootp *) pktbuf;
	struct host *hp = NULL;
	struct host dummyhost;
	long bootsize = 0;
	unsigned hlen, hashcode;
	long dest;
#ifdef	CHECK_FILE_ACCESS
	char realpath[1024];
	char *path;
	int n;
#endif

	/* XXX - SLIP init: Set bp_ciaddr = recv_addr here? */

	/*
	 * If the servername field is set, compare it against us.
	 * If we're not being addressed, ignore this request.
	 * If the server name field is null, throw in our name.
	 */
	if (strlen(bp->bp_sname)) {
		if (strcmp(bp->bp_sname, hostname)) {
			if (debug)
				report(LOG_INFO, "\
ignoring request for server %s from client at %s address %s",
					   bp->bp_sname, netname(bp->bp_htype),
					   haddrtoa(bp->bp_chaddr, bp->bp_hlen));
			/* XXX - Is it correct to ignore such a request? -gwr */
			return;
		}
	} else {
		strcpy(bp->bp_sname, hostname);
	}

	bp->bp_op = BOOTREPLY;
	if (bp->bp_ciaddr.s_addr == 0) {
		/*
		 * client doesnt know his IP address,
		 * search by hardware address.
		 */
		if (debug) {
			report(LOG_INFO, "request from %s address %s",
				   netname(bp->bp_htype),
				   haddrtoa(bp->bp_chaddr, bp->bp_hlen));
		}
		hlen = haddrlength(bp->bp_htype);
		if (hlen != bp->bp_hlen) {
			report(LOG_NOTICE, "bad addr len from from %s address %s",
				   netname(bp->bp_htype),
				   haddrtoa(bp->bp_chaddr, hlen));
		}

		dummyhost.htype = bp->bp_htype;
		bcopy(bp->bp_chaddr, dummyhost.haddr, hlen);
		hashcode = hash_HashFunction(bp->bp_chaddr, hlen);
		hp = (struct host *) hash_Lookup(hwhashtable, hashcode, hwlookcmp,
										 &dummyhost);
		if (hp == NULL) {
			/* if vend area includes host name, use it to find IP addr. */
			if(!lookup_by_vend_name(bp, &dummyhost.iaddr.s_addr)) {
				(bp->bp_yiaddr).s_addr = dummyhost.iaddr.s_addr;
				hashcode = hash_HashFunction((u_char *)
											 &dummyhost.iaddr.s_addr, 4);
				hp = (struct host *) hash_Lookup(iphashtable, hashcode,
												 iplookcmp, &dummyhost);
				/* if host not found, check for generic subnet entry. */
				if (hp == NULL)
					hp=lookup_subnet(dummyhost.iaddr.s_addr);
			}
		}
		else
			(bp->bp_yiaddr).s_addr = hp->iaddr.s_addr;

		if (hp == NULL) {
			/*
			 * XXX - Add dynamic IP address assignment?
			 */
			if (debug)
				report(LOG_INFO, "unknown client %s address %s",
					   netname(bp->bp_htype),
					   haddrtoa(bp->bp_chaddr, bp->bp_hlen));
			return;	/* not found */
		}

	} else {
		
		/*
		 * search by IP address.
		 */
		if (debug) {
			report(LOG_INFO, "request from IP addr %s",
				   inet_ntoa(bp->bp_ciaddr));
		}
		dummyhost.iaddr.s_addr = bp->bp_ciaddr.s_addr;
		hashcode = hash_HashFunction((u_char *) &(bp->bp_ciaddr.s_addr), 4);
		hp = (struct host *) hash_Lookup(iphashtable, hashcode, iplookcmp,
										 &dummyhost);

		/* if host not found, check for generic subnet entry. */
		if (hp == NULL)
			hp=lookup_subnet(bp->bp_ciaddr.s_addr);

		if (hp == NULL) {
			report(LOG_NOTICE,
				   "IP address not found: %s", inet_ntoa(bp->bp_ciaddr));
			return;
		}
	}

	if (debug) {
		report(LOG_INFO, "found %s (%s)", inet_ntoa(hp->iaddr),
			   hp->hostname->string);
	}

	/*
	 * If a specific TFTP server address was specified in the bootptab file,
	 * fill it in, otherwise zero it.
	 */
	(bp->bp_siaddr).s_addr = (hp->flags.bootserver) ?
		hp->bootserver.s_addr : 0L;

#ifdef	STANFORD_PROM_COMPAT
	/*
	 * Stanford bootp PROMs (for a Sun?) have no way to leave
	 * the boot file name field blank (because the boot file
	 * name is automatically generated from some index).
	 * As a work-around, this little hack allows those PROMs to
	 * specify "sunboot14" with the same effect as a NULL name.
	 * (The user specifies boot device 14 or some such magic.)
	 */
	if (strcmp(bp->bp_file, "sunboot14") == 0)
		bp->bp_file[0] = '\0';	/* treat it as unspecified */
#endif

	/*
	 * Fill in the client's proper bootfile.
	 *
	 * If the client specifies an absolute path, try that file with a
	 * ".host" suffix and then without.  If the file cannot be found, no
	 * reply is made at all.
	 *
	 * If the client specifies a null or relative file, use the following
	 * table to determine the appropriate action:
	 *
	 *  Homedir      Bootfile    Client's file
	 * specified?   specified?   specification   Action
	 * -------------------------------------------------------------------
	 *      No          No          Null         Send null filename
	 *      No          No          Relative     Discard request
	 *      No          Yes         Null         Send if absolute else null
	 *      No          Yes         Relative     Discard request     *XXX
	 *      Yes         No          Null         Send null filename
	 *      Yes         No          Relative     Lookup with ".host"
	 *      Yes         Yes         Null         Send home/boot or bootfile
	 *      Yes         Yes         Relative     Lookup with ".host" *XXX
	 *
	 */

	/*
	 * XXX - I think the above policy is too complicated.  When the
	 * boot file is missing, it is not obvious why bootpd will not
	 * respond to client requests.  Define CHECK_FILE_ACCESS if you
	 * want the original complicated policy, otherwise bootpd will
	 * no longer check for existence of the boot file. -gwr
	 */

#ifdef	CHECK_FILE_ACCESS

	if (hp->flags.tftpdir) {
		strcpy(realpath, hp->tftpdir->string);
		path = &realpath[strlen(realpath)];
	} else {
		path = realpath;
	}

	if (bp->bp_file[0]) {
		/*
		 * The client specified a file.
		 */
		if (bp->bp_file[0] == '/') {
			strcpy(path, bp->bp_file);		/* Absolute pathname */
		} else {
			if (hp->flags.homedir) {
				strcpy(path, hp->homedir->string);
				strcat(path, "/");
				strcat(path, bp->bp_file);
			} else {
				report(LOG_NOTICE,
					   "requested file \"%s\" not found: hd unspecified",
					   bp->bp_file);
				return;
			}
		}
	} else {
		/*
		 * No file specified by the client.
		 */
		if (hp->flags.bootfile && ((hp->bootfile->string)[0] == '/')) {
			strcpy(path, hp->bootfile->string);
		} else if (hp->flags.homedir && hp->flags.bootfile) {
			strcpy(path, hp->homedir->string);
			strcat(path, "/");
			strcat(path, hp->bootfile->string);
		} else {
			bzero(bp->bp_file, sizeof(bp->bp_file));
			goto skip_file;	/* Don't bother trying to access the file */
		}
	}

	/*
	 * First try to find the file with a ".host" suffix
	 */
	n = strlen(path);
	strcat(path, ".");
	strcat(path, hp->hostname->string);
	if (chk_access(realpath, &bootsize) < 0) {
		path[n] = 0;			/* Try it without the suffix */
		if (chk_access(realpath, &bootsize) < 0) {
			if (bp->bp_file[0]) {
				/*
				 * Client wanted specific file
				 * and we didn't have it.
				 */
				report(LOG_NOTICE,
					   "requested file not found: \"%s\"", path);
				return;
			} else {
				/*
				 * Client didn't ask for a specific file and we couldn't
				 * access the default file, so just zero-out the bootfile
				 * field in the packet and continue processing the reply.
				 */
				bzero(bp->bp_file, sizeof(bp->bp_file));
				goto skip_file;
			}
		}
	}
	strcpy(bp->bp_file, path);

 skip_file:  ;


#else	/* CHECK_FILE_ACCESS */

	/*
	 * This implements a simple response policy, where bootpd
	 * will fail to respond only if it knows nothing about
	 * the client that sent the request.  This plugs in the
	 * boot file name but does not demand that it exist.
	 */

	if (bp->bp_file[0] == '\0') {
		/*
		 * The client did not specify a file.
		 * Set the file name if it is known.
		 */
		if (hp->flags.bootfile) {
			char *p = bp->bp_file;
			int space = BP_FILE_LEN;
			int n;

			/* If tftpdir is set, append it. */
			if (hp->flags.tftpdir) {
				n = strlen(hp->tftpdir->string);
				if ((n + 1) < space) {
					strcpy(p, hp->tftpdir->string);
					p += n;
					space -= n;
					*p++ = '/';
				}
			}

			/* If homedir is set, append it. */
			if (hp->flags.homedir) {
				n = strlen(hp->homedir->string);
				if ((n + 1) < space) {
					strcpy(p, hp->homedir->string);
					p += n;
					space -= n;
					*p++ = '/';
				}
			}

			/* Finally, append the boot file name. */
			n = strlen(hp->bootfile->string);
			if ((n + 1) < space) {
				strcpy(p, hp->bootfile->string);
				p += n;
				space -= n;
				*p = '\0';
			}
		}
	}

	/* Determine boot file size if requested. */
	if (hp->flags.bootsize_auto) {
		if (bp->bp_file[0] == '\0' ||
			chk_access(bp->bp_file, &bootsize) < 0)
		{
			report(LOG_ERR, "can not determine boot file size for %s",
				   hp->hostname->string);
		}
	}

#endif	/* CHECK_FILE_ACCESS */


	

	if (debug > 1) {
		report(LOG_INFO, "vendor magic field is %d.%d.%d.%d",
			   (int) ((bp->bp_vend)[0]),
			   (int) ((bp->bp_vend)[1]),
			   (int) ((bp->bp_vend)[2]),
			   (int) ((bp->bp_vend)[3]));
	}

	/*
	 * If this host isn't set for automatic vendor info then copy the
	 * specific cookie into the bootp packet, thus forcing a certain
	 * reply format.  Only force reply format if user specified it.
	 */
	if (hp->flags.vm_cookie) {
		/* Slam in the user specified magic number. */
		bcopy(hp->vm_cookie, bp->bp_vend, 4);
	}

	/*
	 * Figure out the format for the vendor-specific info.
	 * Note that bp->bp_vend may have been set above.
	 */
	if (!bcmp(bp->bp_vend, vm_rfc1048, 4)) {
		/* RFC1048 conformant bootp client */
		dovend_rfc1048(bp, hp, bootsize);
		if (debug > 1) {
			report(LOG_INFO, "sending reply (with RFC1048 options)");
		}
	}
#ifdef VEND_CMU
	else if (!bcmp(bp->bp_vend, vm_cmu, 4)) {
		dovend_cmu(bp, hp);
		if (debug > 1) {
			report(LOG_INFO, "sending reply (with CMU options)");
		}
	}
#endif
	else {
		if (debug > 1) {
			report(LOG_INFO, "sending reply (with no options)");
		}
	}

	dest = (hp->flags.reply_addr) ?
		hp->reply_addr.s_addr : 0L;

	/* not forwarded */
	sendreply(0, dest);
}



/*
 * Process BOOTREPLY packet.
 */
PRIVATE void
handle_reply()
{
	if (debug) {
		report(LOG_INFO, "processing boot reply");
	}
	/* forwarded, no destination override */
	sendreply(1, 0);
}



/*
 * Send a reply packet to the client.  'forward' flag is set if we are
 * not the originator of this reply packet.
 */
PRIVATE void
sendreply(forward, dst_override)
	int forward;
	long dst_override;
{
	struct bootp *bp = (struct bootp *) pktbuf;
	struct in_addr dst;
	u_short port = bootpc_port;

	/*
	 * If the destination address was specified explicitly
	 * (i.e. the broadcast address for HP compatiblity)
	 * then send the response to that address.  Otherwise,
	 * act in accordance with RFC951:
	 *   If the client IP address is specified, use that
	 * else if gateway IP address is specified, use that
	 * else make a temporary arp cache entry for the client's
	 * NEW IP/hardware address and use that.
	 */
	if (dst_override) {
		dst.s_addr = dst_override;
		if (debug > 1) {
			report(LOG_INFO, "reply address override: %s",
				   inet_ntoa(dst));
		}
	} else if (bp->bp_ciaddr.s_addr) {
		dst = bp->bp_ciaddr;
	} else if (bp->bp_giaddr.s_addr && forward == 0) {
		dst = bp->bp_giaddr;
		port = bootps_port;
		if (debug > 1) {
			report(LOG_INFO, "sending reply to gateway %s",
				   inet_ntoa(dst));
		}
	} else {
		dst = bp->bp_yiaddr;
		if (debug > 1)
			report(LOG_INFO, "setarp %s - %s",
				   inet_ntoa(dst),
				   haddrtoa(bp->bp_chaddr, bp->bp_hlen));
		setarp(s, &dst, bp->bp_chaddr, bp->bp_hlen);
	}

	if ((forward == 0) &&
		(bp->bp_siaddr.s_addr == 0))
	{
		struct ifreq *ifr;
		struct in_addr siaddr;
		/*
		 * If we are originating this reply, we
		 * need to find our own interface address to
		 * put in the bp_siaddr field of the reply.
		 * If this server is multi-homed, pick the
		 * 'best' interface (the one on the same net
		 * as the client).  Of course, the client may
		 * be on the other side of a BOOTP gateway...
		 */
		ifr = getif(s, &dst);
		if (ifr) {
			struct sockaddr_in *sip;
			sip = (struct sockaddr_in *) &(ifr->ifr_addr);
			siaddr = sip->sin_addr;
		} else {
			/* Just use my "official" IP address. */
			siaddr = my_ip_addr;
		}

		/* XXX - No need to set bp_giaddr here. */

		/* Finally, set the server address field. */
		bp->bp_siaddr = siaddr;
	}

	/* Set up socket address for send. */
	send_addr.sin_family = AF_INET;
	send_addr.sin_port = htons(port);
	send_addr.sin_addr = dst;

	/* Send reply with same size packet as request used. */

	/*
	 * If we're running in slave mode, we store the ultimate destination
	 * at the beginning of the packet and send the packet to the
	 * forwarder.
	 */
	
	if (slave) {
		*slave_dest = send_addr;
		if (sendto(s, slave_dest, pktlen+sizeof(struct sockaddr_in), 0,
				   (struct sockaddr *) &slave_forwarder,
				   sizeof(slave_forwarder)) < 0)
		{
			report(LOG_ERR, "sendto: %s", get_network_errmsg());
		}
	}
	else {
		if (sendto(s, pktbuf, pktlen, 0,
				   (struct sockaddr *) &send_addr,
				   sizeof(send_addr)) < 0)
		{
			report(LOG_ERR, "sendto: %s", get_network_errmsg());
		}
	}
} /* sendreply */


/* nmatch() - now in getif.c */
/* setarp() - now in hwaddr.c */


/*
 * This call checks read access to a file.  It returns 0 if the file given
 * by "path" exists and is publically readable.  A value of -1 is returned if
 * access is not permitted or an error occurs.  Successful calls also
 * return the file size in bytes using the long pointer "filesize".
 *
 * The read permission bit for "other" users is checked.  This bit must be
 * set for tftpd(8) to allow clients to read the file.
 */

PRIVATE int
chk_access(path, filesize)
	char *path;
	long *filesize;
{
	struct stat st;

	if ((stat(path, &st) == 0) && (st.st_mode & (S_IREAD >> 6))) {
		*filesize = (long) st.st_size;
		return 0;
	} else {
		return -1;
	}
}


/*
 * Now in dumptab.c :
 *	dumptab()
 *	dump_host()
 *	list_ipaddresses()
 */

#ifdef VEND_CMU

/*
 * Insert the CMU "vendor" data for the host pointed to by "hp" into the
 * bootp packet pointed to by "bp".
 */

PRIVATE void
dovend_cmu(bp, hp)
	struct bootp *bp;
	struct host *hp;
{
	struct cmu_vend *vendp;
	struct in_addr_list *taddr;

	/*
	 * Initialize the entire vendor field to zeroes.
	 */
	bzero(bp->bp_vend, sizeof(bp->bp_vend));

	/*
	 * Fill in vendor information. Subnet mask, default gateway,
	 * domain name server, ien name server, time server
	 */
	vendp = (struct cmu_vend *) bp->bp_vend;
	strcpy(vendp->v_magic, (char*)vm_cmu);
	if (hp->flags.subnet_mask) {
		(vendp->v_smask).s_addr = hp->subnet_mask.s_addr;
		(vendp->v_flags) |= VF_SMASK;
		if (hp->flags.gateway) {
			(vendp->v_dgate).s_addr = hp->gateway->addr->s_addr;
		}
	}
	if (hp->flags.domain_server) {
		taddr = hp->domain_server;
		if (taddr->addrcount > 0) {
			(vendp->v_dns1).s_addr = (taddr->addr)[0].s_addr;
			if (taddr->addrcount > 1) {
				(vendp->v_dns2).s_addr = (taddr->addr)[1].s_addr;
			}
		}
	}
	if (hp->flags.name_server) {
		taddr = hp->name_server;
		if (taddr->addrcount > 0) {
			(vendp->v_ins1).s_addr = (taddr->addr)[0].s_addr;
			if (taddr->addrcount > 1) {
				(vendp->v_ins2).s_addr = (taddr->addr)[1].s_addr;
			}
		}
	}
	if (hp->flags.time_server) {
		taddr = hp->time_server;
		if (taddr->addrcount > 0) {
			(vendp->v_ts1).s_addr = (taddr->addr)[0].s_addr;
			if (taddr->addrcount > 1) {
				(vendp->v_ts2).s_addr = (taddr->addr)[1].s_addr;
			}
		}
	}
	/* Log message now done by caller. */
} /* dovend_cmu */

#endif /* VEND_CMU */



/*
 * Insert the RFC1048 vendor data for the host pointed to by "hp" into the
 * bootp packet pointed to by "bp".
 */

PRIVATE void
dovend_rfc1048(bp, hp, bootsize)
	struct bootp *bp;
	struct host *hp;
	long bootsize;
{
	int bytesleft, len;
	byte *vp;
	char *tmpstr;

	static char noroom[] = "%s: No room for \"%s\" option";
#define	NEED(LEN, MSG) do                       \
		if (bytesleft < (LEN)) {         	    \
			report(LOG_NOTICE, noroom,          \
				   hp->hostname->string, MSG);  \
			return;                             \
		} while (0)

	vp = bp->bp_vend;
	bytesleft = sizeof(bp->bp_vend);	/* Initial vendor area size */
	bcopy(vm_rfc1048, vp, 4);		/* Copy in the magic cookie */
	vp += 4;
	bytesleft -= 4;

	if (hp->flags.subnet_mask) {
		/* always enough room here. */
		*vp++ = TAG_SUBNET_MASK;			/* -1 byte  */
		*vp++ = 4;					/* -1 byte  */
		insert_u_long(hp->subnet_mask.s_addr, &vp);	/* -4 bytes */
		bytesleft -= 6;					/* Fix real count */
		if (hp->flags.gateway) {
			(void) insert_ip(TAG_GATEWAY,
							 hp->gateway,
							 &vp, &bytesleft);
		}
	}
	if (hp->flags.bootsize) {
		/* always enough room here */
		bootsize = (hp->flags.bootsize_auto) ?
			((bootsize + 511) / 512) : (hp->bootsize);	/* Round up */
		*vp++ = TAG_BOOT_SIZE;
		*vp++ = 2;
		*vp++ = (byte) ((bootsize >> 8) & 0xFF);
		*vp++ = (byte) (bootsize & 0xFF);
		bytesleft -= 4;		/* Tag, length, and 16 bit blocksize */
	}

	/*
	 * This one is special: Remaining options go in the ext file.
	 * Only the subnet_mask, bootsize, and gateway should precede.
	 */
    if (hp->flags.exten_file) {
		/*
		 * Check for room for exten_file.  Add 3 to account for
		 * TAG_EXTEN_FILE, length, and TAG_END.
		 */
		len = strlen(hp->exten_file->string) + 1;
		NEED((len + 3), "ef");
		*vp++ = TAG_EXTEN_FILE;
		*vp++ = (byte) (len & 0xFF);
		bcopy(hp->exten_file->string, vp, len);
		vp += len;
		*vp++ = TAG_END;
		bytesleft -= len + 3;
		return;		/* no more options here. */
	}

	/*
	 * The remaining options are inserted by the following
	 * function (which is shared with bootpef.c).
	 * Keep back one byte for the TAG_END.
	 */
	len = dovend_rfc1497(hp, vp, bytesleft-1);
	vp += len;
	bytesleft -= len;

	/* There should be at least one byte left. */
	NEED(1, "(end)");
	*vp++ = TAG_END;
	bytesleft--;

	/* Log message done by caller. */
	if (bytesleft > 0) {
		/*
		 * Zero out any remaining part of the vendor area.
		 */
		bzero(vp, bytesleft);
	}
#undef	NEED
} /* dovend_rfc1048 */


/*
 * Now in readfile.c:
 * 	hwlookcmp()
 *	iplookcmp()
 */

/* haddrtoa() - now in hwaddr.c */
/*
 * Now in dovend.c:
 * insert_ip()
 * insert_generic()
 * insert_u_long()
 */

/* get_errmsg() - now in report.c */

/*
 * Local Variables:
 * tab-width: 4
 * End:
 */
