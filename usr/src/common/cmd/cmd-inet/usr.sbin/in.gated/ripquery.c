#ident	"@(#)ripquery.c	1.4"
#ident	"$Header$"

/*
 * Public Release 3
 * 
 * $Id$
 */

/*
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1996, 1997 The Regents of the University of Michigan
 * All Rights Reserved
 *  
 * Royalty-free licenses to redistribute GateD Release
 * 3 in whole or in part may be obtained by writing to:
 * 
 * 	Merit GateDaemon Project
 * 	4251 Plymouth Road, Suite C
 * 	Ann Arbor, MI 48105
 *  
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS OF THE
 * UNIVERSITY OF MICHIGAN AND MERIT DO NOT WARRANT THAT THE
 * FUNCTIONS CONTAINED IN THE SOFTWARE WILL MEET LICENSEE'S REQUIREMENTS OR
 * THAT OPERATION WILL BE UNINTERRUPTED OR ERROR FREE. The Regents of the
 * University of Michigan and Merit shall not be liable for
 * any special, indirect, incidental or consequential damages with respect
 * to any claim by Licensee or any third party arising from use of the
 * software. GateDaemon was originated and developed through release 3.0
 * by Cornell University and its collaborators.
 * 
 * Please forward bug fixes, enhancements and questions to the
 * gated mailing list: gated-people@gated.merit.edu.
 * 
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1990,1991,1992,1993,1994,1995 by Cornell University.
 *     All rights reserved.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * GateD is based on Kirton's EGP, UC Berkeley's routing
 * daemon	 (routed), and DCN's HELLO routing Protocol.
 * Development of GateD has been supported in part by the
 * National Science Foundation.
 * 
 * ------------------------------------------------------------------------
 * 
 * Portions of this software may fall under the following
 * copyrights:
 * 
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms are
 * permitted provided that the above copyright notice and
 * this paragraph are duplicated in all such forms and that
 * any documentation, advertising materials, and other
 * materials related to such distribution and use
 * acknowledge that the software was developed by the
 * University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote
 * products derived from this software without specific
 * prior written permission.  THIS SOFTWARE IS PROVIDED
 * ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */


#define	INCLUDE_TIME
#define	INCLUDE_CTYPE
#define	MALLOC_OK

#include "include.h"
#define	RIPCMDS
#include "inet.h"
#include "rip.h"

/* macros to select internet address given pointer to a struct sockaddr */

/* result is u_int32 */
#define sock_inaddr(x) (((struct sockaddr_in *)(x))->sin_addr)

/* result is struct in_addr */
#define in_addr_ofs(x) (((struct sockaddr_in *)(x))->sin_addr)

/* Calculate the natural netmask for a given network */
#define	inet_netmask(net) (IN_CLASSA(net) ? IN_CLASSA_NET :\
			 (IN_CLASSB(net) ? IN_CLASSB_NET :\
			  (IN_CLASSC(net) ? IN_CLASSC_NET : 0)))

#define WTIME	5			/* Time to wait for responses */
#define STIME	1			/* Time to wait between packets */

static int s;
static char packet[RIP_PKTSIZE];
static int cmds_request[] = {
    RIPCMD_REQUEST,
    RIPCMD_POLL,
    0 };
static int cmds_poll[] = {
    RIPCMD_POLL,
    RIPCMD_REQUEST,
    0 };
static int *cmds = cmds_poll;
static int dflag;
static int nflag;
const char *version = "1.25.2.6";
static int vers = RIP_VERSION_2;
static struct rip_authinfo auth_info;

/* List of destinations to query */

static int n_dests = 0;

struct dest {
    struct dest *d_forw;
    struct dest *d_back;

    struct in_addr d_dest;
    struct in_addr d_mask;    
} ;

static struct dest dests = { &dests, &dests };



/* Print an IP address as a dotted quad.  We could use inet_ntoa, but */
/* that breaks with GCC on a Sun4 */
static char *
pr_ntoa __PF1(addr, u_int32)
{
    static char buf[18];
    register char *bp = buf;
    register byte *cp = (byte *) &addr;
    register int i = sizeof (struct in_addr);
    register int c;

    while (i--) {
	if ((c = *cp++)) {
	    register int leading = 0;
	    register int power = 100;

	    do {
		if (c >= power) {
		    *bp++ = '0' + c/power;
		    c %= power;
		    leading = 1;
		} else if (leading || power == 1) {
		    *bp++ = '0';
		}
	    } while (power /= 10) ;
	} else {
	    *bp++ = '0';
	}
	if (i) {
	    *bp++ = '.';
	}
    }
    *bp = (char) 0;
    return buf;
}


/*
 *	Trace RIP packets
 */
static void
ripq_trace __PF4(dir, const char *,
		 who, struct sockaddr_in *,
		 cp, void_t,
		 size, register int)
{
    register struct rip *rpmsg = (struct rip *) cp;
    register struct rip_netinfo *n;
    register const char *cmd = "Invalid";

    /* XXX - V2 extensions */

    if (rpmsg->rip_cmd && rpmsg->rip_cmd < RIPCMD_MAX) {
	cmd = trace_state(rip_cmd_bits, rpmsg->rip_cmd);
    }
    (void) fprintf(stderr, "RIP %s %s+%d vers %d, cmd %s, length %d",
		   dir,
		   pr_ntoa(who->sin_addr.s_addr),
		   ntohs(who->sin_port),
		   rpmsg->rip_vers,
		   cmd,
		   size);

    switch (rpmsg->rip_cmd) {
#ifdef	RIPCMD_POLL
	case RIPCMD_POLL:
#endif	/* RIPCMD_POLL */
	case RIPCMD_REQUEST:
	case RIPCMD_RESPONSE:
	    (void) fprintf(stderr, "\n");
	    size -= 4 * sizeof(char);
	    n = (struct rip_netinfo *) ((void_t) (rpmsg + 1));
	    for (; size > 0; n++, size -= sizeof(struct rip_netinfo)) {
		u_int family = ntohs(n->rip_family);
		metric_t metric = ntohl((u_int32) n->rip_metric);
		
		if (size < sizeof(struct rip_netinfo)) {
		    break;
		}
		switch (family) {
		case RIP_AF_INET:
		    if (rpmsg->rip_vers > 1) {
			(void) fprintf(stderr,
				       "\tnet %15s/",
				       pr_ntoa(n->rip_dest));
			(void) fprintf(stderr,
				       "%-15s  ",
				       pr_ntoa(n->rip_dest_mask));
			(void) fprintf(stderr,
				       "router %-15s  metric %2d  tag %#04X\n",
				       pr_ntoa(n->rip_router),
				       metric,
				       ntohs((u_int16) n->rip_tag));
		    } else {
			(void) fprintf(stderr,
				       "\tnet %-15s  metric %2d\n",
				       pr_ntoa(n->rip_dest),
				       metric);
		    }
		    break;

		case RIP_AF_UNSPEC:
		    if (metric == RIP_METRIC_UNREACHABLE) {
			(void) fprintf(stderr,
				       "\trouting table request\n");
			break;
		    }
		    goto bogus;

		case RIP_AF_AUTH:
		    if (rpmsg->rip_vers > 1
			&& n == (struct rip_netinfo *) ((void_t) (rpmsg + 1))) {
			struct rip_authinfo *auth = (struct rip_authinfo *) n;
			int auth_type = ntohs(auth->auth_type);

			switch (auth_type) {
			case RIP_AUTH_NONE:
			    (void) fprintf(stderr,
					   "\tAuthentication: None\n");
			    break;

			case RIP_AUTH_SIMPLE:
			    (void) fprintf(stderr,
					   "\tAuthentication: %.*s\n",
					   RIP_AUTH_SIZE, (char *) auth->auth_data);
			    break;

			case RIP_AUTH_MD5:
			    {
				struct rip_trailer *rtp;
				
				(void) fprintf(stderr,
					       "\tAuthentication: MD5 Digest: %08x.%08x.%08x.%08x Sequence: ",
					       auth->auth_data[0],
					       auth->auth_data[1],
					       auth->auth_data[2],
					       auth->auth_data[3]);
				if (size % sizeof (struct rip_netinfo) >= sizeof (*rtp)) {
				    rtp = (struct rip_trailer *) (n + size / sizeof (struct rip_netinfo));
				    size -= sizeof (struct rip_trailer);
				    (void) fprintf(stderr,
						   "%08x (%T)",
						   ntohl(rtp->auth_sequence),
						   ntohl(rtp->auth_sequence) - time_boot);
				} else {
				    (void) fprintf(stderr,
						   "???");
				}
				(void) fprintf(stderr, "\n");
			    }
			    break;

			default:
			    (void) fprintf(stderr,
					   "\tInvalid auth type: %d\n",
					   auth_type);
			}
			break;
		    }
		    /* Fall through */

		default:
		bogus:
		    (void) fprintf(stderr,
				   "\tInvalid family: %d\n",
				   family);
		}
	    }
	    (void) fprintf(stderr,
			   "RIP %s end of packet", dir);
	    break;
	case RIPCMD_TRACEON:
	    (void) fprintf(stderr, ", file %*s", size, (char *) (rpmsg + 1));
	    break;
#ifdef	RIPCMD_POLLENTRY
	case RIPCMD_POLLENTRY:
	    n = (struct rip_netinfo *) ((void_t) (rpmsg + 1));
	    (void) fprintf(stderr, ", net %s",
			   pr_ntoa(n->rip_dest));
	    break;
#endif	/* RIPCMD_POLLENTRY */
	default:
	    (void) fprintf(stderr, "\n");
	    break;
    }
    (void) fprintf(stderr, "\n");
}


static void
ripq_query __PF2(host, char *,
		 cmd, int)
{
    struct sockaddr_in router;
    struct rip *msg = (struct rip *) ((void_t) packet);
    struct rip_netinfo *nets = (struct rip_netinfo *) ((void_t) (msg + 1));
    struct rip_authinfo *auth = (struct rip_authinfo *) 0;
    int rc;

    bzero((char *) &router, sizeof(router));
    if (!inet_aton(host, &router.sin_addr)) {
	struct hostent *hp = gethostbyname(host);

	if (!hp) {
	    (void) printf("%s: unknown\n", host);
	    exit(1);
	} else {
	    (void) bcopy(hp->h_addr, (char *) &router.sin_addr, (size_t) hp->h_length);
	}
    }
    router.sin_family = AF_INET;
    router.sin_port = task_get_port((trace *) 0,
				    "router", "udp",
				    htons(RIP_PORT));
    msg->rip_cmd = cmd;
    msg->rip_vers = vers;
    if (vers > 1
	&& ntohs(auth_info.auth_type) != RIP_AUTH_NONE) {
	/* Reserve space for authentication */

	auth = (struct rip_authinfo *) nets++;
    }

    /* Fill in the request */
    if (n_dests) {
	register struct dest *dp;
	
	/* Query specific nets */

	for (dp = dests.d_forw; dp != &dests; dp = dp->d_forw) {
	    bzero((caddr_t) nets, sizeof *nets);
	    nets->rip_family = htons(RIP_AF_INET);
	    nets->rip_dest = dp->d_dest.s_addr;
	    if (dp->d_mask.s_addr && vers < 2) {
		(void) fprintf(stderr,
			       "destination with mask not possible with RIPv1 or later\n");
		exit(1);
	    }
	    nets->rip_dest_mask = dp->d_mask.s_addr;
	    nets++;
	}
    } else {
	bzero((caddr_t) nets, sizeof *nets);
	nets->rip_family = htons(AF_UNSPEC);
	nets->rip_metric = htonl((u_int32) RIP_METRIC_UNREACHABLE);
	nets++;
    }

    if (auth) {
	switch (ntohs(auth_info.auth_type)) {
	case RIP_AUTH_NONE:
	    break;

	case RIP_AUTH_SIMPLE:
	    bcopy((caddr_t) &auth_info, (caddr_t) auth, sizeof *auth);
	    break;

	case RIP_AUTH_MD5:
	    {
		struct rip_trailer *rtp = (struct rip_trailer *) nets;

		/* Init field and add digest */
		bcopy((caddr_t) &auth_info, (caddr_t) auth, sizeof *auth);

		rtp->auth_sequence = htonl(0);
		rtp++;

		/* Calculate and add digest */
		md5_cksum((byte *) packet,
			  (caddr_t) rtp - packet,
			  (caddr_t) rtp - packet,
			  auth->auth_data,
			  (u_int32 *) 0);
		nets = (struct rip_netinfo *) rtp;
	    }
	    break;
	}
    }

    if (dflag) {
	ripq_trace("SEND", &router, (void_t) msg, (caddr_t) nets - packet);
    }
    NON_INTR(rc,
	     sendto(s,
		    packet,
		    (caddr_t) nets - packet,
		    0,
		    (struct sockaddr *) & router,
		    sizeof(router)));
    if (rc < 0) {
	perror(host);
    }
}


/*
 * Handle an incoming routing packet.
 */
static void
ripq_input __PF3(from, struct sockaddr_in *,
		to, struct in_addr *,
		size, int)
{
    register struct rip *msg = (struct rip *) ((void_t) packet);
    struct rip_netinfo *n;
    struct hostent *hp;

    if (dflag) {
	ripq_trace("RECV", from, (void_t) msg, size);
    }
    if (msg->rip_cmd != RIPCMD_RESPONSE)
	return;
    if (nflag
	|| !(hp = gethostbyaddr((char *) &from->sin_addr, sizeof(struct in_addr), AF_INET))) {
	(void) printf("%d bytes from %s",
		      size,
		      pr_ntoa(from->sin_addr.s_addr));
    } else {
	(void) printf("%d bytes from %s(%s)",
		      size,
		      hp->h_name,
		      pr_ntoa(from->sin_addr.s_addr));
    }
    if (to) {
	(void) printf(" to %s",
		      pr_ntoa(to->s_addr));
    }
    printf(" version %u:\n",
	   msg->rip_vers);
    size -= sizeof(int);
    n = (struct rip_netinfo *) ((void_t) (msg + 1));
    while (size > 0) {
	if (size < sizeof(struct rip_netinfo))
	    break;
	switch (msg->rip_vers) {
	case 0:
	    break;

	default:
	    GNTOHS(n->rip_tag);
	    /* Fall through */

	case 1:
	    GNTOHS(n->rip_family);
	    GNTOHL(n->rip_metric);
	    break;
	}
	switch (n->rip_family) {
	case RIP_AF_INET:
	    if (msg->rip_vers > 1) {
		(void) printf("\t%15s/",
			      pr_ntoa(n->rip_dest));
		(void) printf("%-15s  ",
			      pr_ntoa(n->rip_dest_mask));
		(void) printf("router %-15s  metric %2d  tag %#04X\n",
			      pr_ntoa(n->rip_router),
			      n->rip_metric,
			      n->rip_tag);
	    } else {
		/* XXX - Print a couple per line */
		(void) printf("\t%-15s  metric %2d\n",
			      pr_ntoa(n->rip_dest),
			      n->rip_metric);
	    }
	    break;

	case RIP_AF_AUTH:
	    break;

	default:
	    (void) printf("\tInvalid family: %d\n",
			  n->rip_family);
	    break;
	}
	size -= sizeof(struct rip_netinfo), n++;
    }
}


int
main __PF2(argc, int,
	   argv, char **)
{
    int c, cc, fdbits, errflg = 0, *cmd = cmds;
    static struct timeval *wait_time, long_time = {WTIME, 0}, short_time = {STIME, 0};
    static struct sockaddr_in from;
#if	defined(SCM_RIGHTS) && !defined(sun)
    int on = 1;
    struct in_addr *to;
    static byte control[BUFSIZ];
    static struct iovec iovec = { (caddr_t) packet, sizeof (packet) };
    static struct msghdr msghdr = {
	(caddr_t) &from, sizeof (from),
	&iovec, 1,
	(caddr_t) control, sizeof(control),
	0 } ;
#else	/* defined(SCM_RIGHTS) && !defined(sun) */
    int fromlen = sizeof (from);
#endif	/* defined(SCM_RIGHTS) && !defined(sun) */

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == (u_int) -1) {
	perror("socket");
	exit(2);
    }
#if	defined(SCM_RIGHTS) && !defined(sun)
    if (setsockopt(s, IPPROTO_IP, IP_RECVDSTADDR, &on, sizeof (on)) < 0) {
	perror("setsockopt(IP_RECVDSTADDR)");
	exit (2);
    }
#endif	/* defined(SCM_RIGHTS) && !defined(sun) */

    auth_info.auth_family = htons(RIP_AF_AUTH);
    auth_info.auth_type = htons(RIP_AUTH_NONE);
    auth_info.auth_data[0] = auth_info.auth_data[1]
	= auth_info.auth_data[2] = auth_info.auth_data[3]
	    = (u_int32) 0;

    while ((c = getopt(argc, argv, "a:dnprvw:125:N:")) != EOF) {
	switch (c) {
	case 'a':
	case '5':
	    if (htons(auth_info.auth_type) != RIP_AUTH_NONE) {
		(void) fprintf(stderr,
			       "Authentication already specified!\n");
			       errflg++;
			       break;
	    } else {
		int len;
		char *cp = optarg;

		for (len = 0; *cp; len++) {
		    if (!isprint(*cp)) {
			break;
		    }
		    cp++;
		}
		if (*cp || !len || len > RIP_AUTH_SIZE) {
		    (void) fprintf(stderr,
				   "Invalid Authentication: %s\n",
				   optarg);
		    errflg++;
		    break;
		}
		strncpy((caddr_t) auth_info.auth_data, optarg, RIP_AUTH_SIZE);
		switch (c) {
		case 'a':
		    auth_info.auth_type = htons(RIP_AUTH_SIMPLE);
		    break;

		case '5':
		    auth_info.auth_type = htons(RIP_AUTH_MD5);
		    break;

		default:
		    assert(FALSE);
		}
	    }
	    break;

	case 'd':
	    dflag++;
	    break;

	case 'n':
	    nflag++;
	    break;
	    
	case 'p':
	    cmd = cmds_poll;
	    break;

	case 'r':
	    cmd = cmds_request;
	    break;

	case 'v':
	    (void) fprintf(stderr, "RIPQuery %s\n", version);
	    break;

	case 'w':
	    long_time.tv_sec = atoi(optarg);
	    (void) fprintf(stderr, "Wait time set to %d\n", long_time.tv_sec);
	    break;

	case 'N':
	    if (n_dests < 23) {
		struct in_addr dest, mask;
		struct dest *dp;
		register char *cp = (char *) strchr(optarg, '/');

		if (cp) {
		    *cp = (char ) 0;

		    if (!inet_aton(optarg, &dest)) {
			cp = optarg;
		    BadDest:
			(void) fprintf(stderr,
				       "unable to parse destination: %s\n",
				       cp);
			exit(1);
		    }
		    if (!inet_aton(++cp, &mask)) {
			goto BadDest;
		    }
		    vers = RIP_VERSION_2;
		} else {
		    if (!inet_aton(optarg, &dest)) {
			cp = optarg;
			goto BadDest;
		    }
		    mask.s_addr = 0;
		}
		dp = (struct dest *) calloc(1, sizeof *dp);
		dp->d_dest = dest;	/* struct copy */
		dp->d_mask = mask;	/* struct copy */
		INSQUE(dp, dests.d_back);
		n_dests++;
	    } else {
		(void) fprintf(stderr,
			       "too many destinations specified\n");
		exit(1);
	    }
	    break;
		

	case '1':
	    vers = RIP_VERSION_1;
	    break;

	case '2':
	    vers = RIP_VERSION_2;
	    break;

	case '?':
	    errflg++;
	    break;
	}
    }

    if (errflg || (optind >= argc)) {
	(void) printf("usage: ripquery [ -a ] [ -d ] [ -n ] [ -p ] [ -r ] [ -v ] [ -1 ] [ -2 ] [ -w time] [ -N dest[/mask] ] hosts...\n");
	exit(1);
    }
    setnetent(1);

    for (; optind < argc; optind++) {
      retry:
	ripq_query(argv[optind], *cmd);
	fdbits = 1 << s;
	wait_time = &long_time;
	for (;;) {
	    cc = select(s + 1, (struct fd_set *) & fdbits, (struct fd_set *) 0, (struct fd_set *) 0, wait_time);
	    if (cc == 0) {
		if (wait_time == &short_time) {
		    break;
		}
		if (*(++cmd)) {
		    goto retry;
		} else {
		    break;
		}
	    } else if (cc < 0) {
		perror("select");
		(void) close(s);
		exit(1);
	    } else {
		wait_time = &short_time;
#if	defined(SCM_RIGHTS) && !defined(sun)
		msghdr.msg_namelen = sizeof (from);
		msghdr.msg_controllen = sizeof (control) ;
		msghdr.msg_flags = 0;
		NON_INTR(cc, recvmsg(s, &msghdr, 0));
#else	/* defined(SCM_RIGHTS) && !defined(sun) */
		NON_INTR(cc, recvfrom(s,
				      packet,
				      sizeof (packet),
				      0,
				      (struct sockaddr *) &from,
				      (void_t) &fromlen));
#endif	/* defined(SCM_RIGHTS) && !defined(sun) */
		if (cc <= 0) {
		    if (cc < 0) {
			perror("recvmsg");
			(void) close(s);
			exit(1);
		    }
		    continue;
		}
#if	defined(SCM_RIGHTS) && !defined(sun)
#define	ENOUGH_CMSG(cmsg, size)	((cmsg)->cmsg_len >= ((size) + sizeof(struct cmsghdr)))

		if (msghdr.msg_flags & MSG_TRUNC) {
		    (void) fprintf(stderr, "message from %s truncated to %d bytes\n",
				   pr_ntoa(from.sin_addr.s_addr),
				   cc);
#ifdef	notdef
		    continue;
#endif	/* notdef */
		}

		to = (struct in_addr *) 0;
		if (msghdr.msg_controllen >= sizeof (struct cmsghdr)
		    && !(msghdr.msg_flags & MSG_CTRUNC)) {
		    struct cmsghdr *cmsg;

		    for (cmsg = CMSG_FIRSTHDR(&msghdr);
			 cmsg && cmsg->cmsg_len >= sizeof (struct cmsghdr);
			 cmsg = CMSG_NXTHDR(&msghdr, cmsg)) {

			if (cmsg->cmsg_level == IPPROTO_IP
			    && cmsg->cmsg_type == IP_RECVDSTADDR
			    && ENOUGH_CMSG(cmsg, sizeof (struct in_addr))) {
			    to = (struct in_addr *) CMSG_DATA(cmsg);
			}
		    }
		}

		ripq_input(&from, to, cc);
#else	/* defined(SCM_RIGHTS) && !defined(sun) */
		ripq_input(&from, (struct in_addr *) 0, cc);
#endif	/* defined(SCM_RIGHTS) && !defined(sun) */
	    }
	}
    }

    endnetent();
    return 0;
}
