#ident "@(#)ntp_io.c	1.4"

/*
 * xntp_io.c - input/output routines for xntpd.  The socket-opening code
 *	       was shamelessly stolen from ntpd.
 */
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#ifndef SYS_WINNT
#if !defined(VMS)
#include <sys/param.h>
#endif /* VMS */
#include <sys/time.h>
#if !defined(__bsdi__) && !defined(apollo)
#include <netinet/in.h>
#endif
#endif /* SYS_WINNT */
#if defined(SYS_BSDI) || defined(SYS_NETBSD) || defined(SYS_FREEBSD) || defined(SYS_AIX)
#include <sys/ioctl.h>
#endif
#ifdef SYS_UXPV
#include <sys/sockio.h>
#endif

#if	_BSDI_VERSION >= 199510
#include "ifaddrs.h"
#endif
#include "ntpd.h"
#include "ntp_select.h"
#include "ntp_io.h"
#include "ntp_refclock.h"
#include "ntp_if.h"
#include "ntp_stdlib.h"

#if defined(VMS)    /* most likely UCX-specific */

#include <UCX$INETDEF.H>

/* "un*x"-compatible names for some items in UCX$INETDEF.H */
#define ifreq		IFREQDEF
#define ifr_name	IFR$T_NAME
#define ifr_addr        IFR$R_DUMMY.IFR$T_ADDR
#define ifr_broadaddr   IFR$R_DUMMY.IFR$T_BROADADDR
#define ifr_flags       IFR$R_DUMMY.IFR$R_DUMMY_1_OVRL.IFR$W_FLAGS
#define IFF_UP		IFR$M_IFF_UP
#define IFF_BROADCAST	IFR$M_IFF_BROADCAST
#define IFF_LOOPBACK	IFR$M_IFF_LOOPBACK

/* structure used in SIOCGIFCONF request (after [KSR] OSF/1) */
struct ifconf {
	int ifc_len;			/* size of buffer */
	union {
		caddr_t ifcu_buf;
		struct ifreq *ifcu_req;
	} ifc_ifcu;
};
#define ifc_buf ifc_ifcu.ifcu_buf	/* buffer address */
#define ifc_req ifc_ifcu.ifcu_req	/* array of structures returned */

#endif /* VMS+UCX */

#if defined(MCAST) && !defined(IP_ADD_MEMBERSHIP)
#undef MCAST
#endif

#if defined(BSD)&&!defined(sun)&&!defined(SYS_SINIXM)&&!defined(SYS_MIPS)
#if BSD >= 199006
#define HAVE_VARIABLE_IFR_LENGTH
#endif
#endif

#if defined(USE_TTY_SIGPOLL)||defined(USE_UDP_SIGPOLL)
#if defined(SYS_AIX)&&defined(_IO)
#undef _IO
#endif
#include <stropts.h>
#endif

/*
 * We do asynchronous input using the SIGIO facility.  A number of
 * recvbuf buffers are preallocated for input.  In the signal
 * handler we poll to see which sockets are ready and read the
 * packets from them into the recvbuf's along with a time stamp and
 * an indication of the source host and the interface it was received
 * through.  This allows us to get as accurate receive time stamps
 * as possible independent of other processing going on.
 *
 * We watch the number of recvbufs available to the signal handler
 * and allocate more when this number drops below the low water
 * mark.  If the signal handler should run out of buffers in the
 * interim it will drop incoming frames, the idea being that it is
 * better to drop a packet than to be inaccurate.
 */

/*
 * Block the interrupt, for critical sections.
 */
#if defined(HAVE_SIGNALED_IO)
#define BLOCKIO()   ((void) block_sigio())
#define UNBLOCKIO() ((void) unblock_sigio())
#else
#define BLOCKIO()
#define UNBLOCKIO()
#endif

/*
 * recvbuf memory management
 */
#define	RECV_INIT	10	/* 10 buffers initially */
#define	RECV_LOWAT	3	/* when we're down to three buffers get more */
#define	RECV_INC	5	/* get 5 more at a time */
#define	RECV_TOOMANY	30	/* this is way too many buffers */

/*
 * Memory allocation
 */
u_long full_recvbufs;	/* number of recvbufs on fulllist */
u_long free_recvbufs;	/* number of recvbufs on freelist */

static	struct recvbuf *freelist;	/* free buffers */
static	struct recvbuf *fulllist;	/* lifo buffers with data */
static	struct recvbuf *beginlist;	/* fifo buffers with data */

u_long total_recvbufs;	/* total recvbufs currently in use */
u_long lowater_additions;	/* number of times we have added memory */

static	struct recvbuf initial_bufs[RECV_INIT];	/* initial allocation */


/*
 * Other statistics of possible interest
 */
u_long packets_dropped;	/* total number of packets dropped on reception */
u_long packets_ignored;	/* packets received on wild card interface */
u_long packets_received;	/* total number of packets received */
u_long packets_sent;	/* total number of packets sent */
u_long packets_notsent;	/* total number of packets which couldn't be sent */

u_long handler_calls;	/* number of calls to interrupt handler */
u_long handler_pkts;	/* number of pkts received by handler */
u_long io_timereset;	/* time counters were reset */

/*
 * Interface stuff
 */
#define	MAXINTERFACES	192		/* much better for big gateways with IP/X.25 and more ... */
struct interface *any_interface;	/* pointer to default interface */
struct interface *loopback_interface;	/* point to loopback interface */
static	struct interface inter_list[MAXINTERFACES];
static	int ninterfaces;

#ifdef REFCLOCK
/*
 * Refclock stuff.  We keep a chain of structures with data concerning
 * the guys we are doing I/O for.
 */
static	struct refclockio *refio;
#endif

/*
 * File descriptor masks etc. for call to select
 */
fd_set activefds;
int maxactivefd;

/*
 * Imported from ntp_timer.c
 */
extern u_long current_time;

#ifndef SYS_WINNT
extern int errno;
#else
extern long units_per_tick;
#endif /* SYS_WINNT */
extern int debug;

static	int	create_sockets	P((u_int));
static	int	open_socket	P((struct sockaddr_in *, int, int));
static	void	close_socket	P((int));
#ifdef HAVE_SIGNALED_IO
static  int	init_clock_sig	P(());
static  void	init_socket_sig P((int));
static  void	set_signal	P(());
static  RETSIGTYPE sigio_handler P((int));
static  void	block_sigio	P((void));
static  void	unblock_sigio	P(());
#endif
#ifndef STREAMS_TLI
#ifndef SYS_WINNT
extern	char	*inet_ntoa	P((struct in_addr));
#else
extern char FAR * PASCAL FAR inet_ntoa P((struct in_addr));
#endif /* SYS_WINNT */
#endif /* STREAMS_TLI */

/*
 * init_io - initialize I/O data structures and call socket creation routine
 */
void
init_io()
{
	register int i;

#ifdef SYS_WINNT
 	WORD wVersionRequested;
	WSADATA wsaData;
#endif /* SYS_WINNT */

	/*
	 * Init buffer free list and stat counters
	 */
	freelist = 0;
	for (i = 0; i < RECV_INIT; i++) {
		initial_bufs[i].next = freelist;
		freelist = &initial_bufs[i];
	}

	fulllist = 0;
	free_recvbufs = total_recvbufs = RECV_INIT;
	full_recvbufs = lowater_additions = 0;
	packets_dropped = packets_received = 0;
	packets_ignored = 0;
	packets_sent = packets_notsent = 0;
	handler_calls = handler_pkts = 0;
	io_timereset = 0;
	loopback_interface = 0;

#ifdef REFCLOCK
	refio = 0;
#endif

#if defined(HAVE_SIGNALED_IO)
	(void) set_signal();
#endif

#ifdef SYS_WINNT
	wVersionRequested = MAKEWORD(1,1);
	if (WSAStartup(wVersionRequested, &wsaData)) {
		syslog(LOG_ERR, "No useable winsock.dll: %m");
		exit(1);
	}
#endif /* SYS_WINNT */

	/*
	 * Create the sockets
	 */
	BLOCKIO();
	(void) create_sockets(htons(NTP_PORT));
	UNBLOCKIO();

#ifdef DEBUG
	if (debug)
		printf("init_io: maxactivefd %d\n", maxactivefd);
#endif
}

/*
 * create_sockets - create a socket for each interface plus a default
 *		    socket for when we don't know where to send
 */
static int
create_sockets(port)
	u_int port;
{
#if	_BSDI_VERSION >= 199510
	int num_if, i, j;
	struct ifaddrs *ifaddrs, *ifap, *lp;
	struct sockaddr_in resmask;
#else	/* _BSDI_VERSION >= 199510 */
#ifdef STREAMS_TLI
	struct strioctl	ioc;
#endif /* STREAMS_TLI */
#ifndef SYS_WINNT
	char	buf[MAXINTERFACES*sizeof(struct ifreq)];
	struct	ifconf	ifc;
	struct	ifreq	ifreq, *ifr;
#endif /* SYS_WINNT */
	int n, i, j, vs, size;
	struct sockaddr_in resmask;
#endif	/* _BSDI_VERSION >= 199510 */

#ifdef DEBUG
	if (debug)
		printf("create_sockets(%d)\n", ntohs(port));
#endif

	/*
	 * create pseudo-interface with wildcard address
	 */
	inter_list[0].sin.sin_family = AF_INET;
	inter_list[0].sin.sin_port = port;
	inter_list[0].sin.sin_addr.s_addr = htonl(INADDR_ANY);
	(void) strncpy(inter_list[0].name, "wildcard",
	     sizeof(inter_list[0].name));
	inter_list[0].mask.sin_addr.s_addr = htonl(~0);
	inter_list[0].received = 0;
	inter_list[0].sent = 0;
	inter_list[0].notsent = 0;
	inter_list[0].flags = INT_BROADCAST;

#if	_BSDI_VERSION >= 199510
	if (getifaddrs(&ifaddrs, &num_if) < 0) {
		syslog(LOG_ERR, "getifaddrs: %m");
		exit(1);
	}

	i = 1;

	for (ifap = ifaddrs, lp = ifap + num_if; ifap < lp; ifap++) {
		struct sockaddr_in *sin;

		if (ifap->ifa_addr->sa_family != AF_INET)
			continue;

		if ((ifap->ifa_flags & IFF_UP) == 0)
			continue;

		inter_list[i].flags = 0;
		if (ifap->ifa_flags & IFF_BROADCAST)
			inter_list[i].flags |= INT_BROADCAST;

		(void)strcpy(inter_list[i].name, ifap->ifa_name);

		sin = (struct sockaddr_in *)ifap->ifa_addr;
		inter_list[i].sin = *sin;
		inter_list[i].sin.sin_port = port;

		if (ifap->ifa_flags & IFF_LOOPBACK) {
			inter_list[i].flags = INT_LOOPBACK;
			if (loopback_interface == NULL
			    || ntohl(sin->sin_addr.s_addr) != 0x7f000001)
				loopback_interface = &inter_list[i];
		}

		if (inter_list[i].flags & INT_BROADCAST) {
			sin = (struct sockaddr_in *)ifap->ifa_broadaddr;
			inter_list[i].bcast = *sin;
			inter_list[i].bcast.sin_port = port;
		}

		if (ifap->ifa_flags & (IFF_LOOPBACK|IFF_POINTOPOINT)) {
			inter_list[i].mask.sin_addr.s_addr = 0xffffffff;
		} else {
			sin = (struct sockaddr_in *)ifap->ifa_netmask;
			inter_list[i].mask = *sin;
		}
		inter_list[i].mask.sin_family = AF_INET;
		inter_list[i].mask.sin_len = sizeof *sin;

		/*
		 * look for an already existing source interface address.  If
		 * the machine has multiple point to point interfaces, then
		 * the local address may appear more than once.
		 */
		for (j=0; j < i; j++)
			if (inter_list[j].sin.sin_addr.s_addr ==
			    inter_list[i].sin.sin_addr.s_addr) {
				if (inter_list[j].flags & INT_LOOPBACK)
					inter_list[j] = inter_list[i];
				break;
			}
		if (j == i)
			i++;
	}
	free(ifaddrs);
#else	/* _BSDI_VERSION >= 199510 */
#ifndef SYS_WINNT
#ifdef USE_STREAMS_DEVICE_FOR_IF_CONFIG
	if ((vs = open("/dev/ip", O_RDONLY)) < 0) {
#else /* ! USE_STREAMS_DEVICE_FOR_IF_CONFIG */
	if ((vs = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
#endif /* USE_STREAMS_DEVICE_FOR_IF_CONFIG */
		syslog(LOG_ERR, "vs=socket(AF_INET, SOCK_DGRAM) %m");
		exit(1);
	}

	i = 1;

	ifc.ifc_len = sizeof(buf);
#ifdef STREAMS_TLI
	ioc.ic_cmd = SIOCGIFCONF;
	ioc.ic_timout = 0;
	ioc.ic_dp = (caddr_t)buf;
	ioc.ic_len = sizeof(buf);
	if(ioctl(vs, I_STR, &ioc) < 0 ||
	    ioc.ic_len < sizeof(struct ifreq)) {
		syslog(LOG_ERR, "get interface configuration: %m");
		exit(1);
	}
#ifdef SIZE_RETURNED_IN_BUFFER
	ifc.ifc_len = ioc.ic_len - sizeof(int);
	ifc.ifc_buf = buf + sizeof(int);
#else /* ! SIZE_RETURNED_IN_BUFFER */
	ifc.ifc_len = ioc.ic_len;
	ifc.ifc_buf = buf;
#endif /* SIZE_RETURNED_IN_BUFFER */

#else /* ! STREAMS_TLI */
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if (ioctl(vs, SIOCGIFCONF, (char *)&ifc) < 0) {
		syslog(LOG_ERR, "get interface configuration: %m");
		exit(1);
	}
#endif /* STREAMS_TLI */

	for(n = ifc.ifc_len, ifr = ifc.ifc_req; n > 0;
	    ifr = (struct ifreq *)((char *)ifr + size)) {
		size = sizeof(*ifr);

#ifdef HAVE_VARIABLE_IFR_LENGTH
		if (ifr->ifr_addr.sa_len > sizeof(ifr->ifr_addr))
			size += ifr->ifr_addr.sa_len - sizeof(struct sockaddr);
#endif
		n -= size;
#ifdef VMS /* VMS+UCX */
		if(((struct sockaddr *)&(ifr->ifr_addr))->sa_family != AF_INET)
#else
		if (ifr->ifr_addr.sa_family != AF_INET)
#endif /* VMS+UCX */
			continue;
		ifreq = *ifr;
#ifdef STREAMS_TLI
		ioc.ic_cmd = SIOCGIFFLAGS;
		ioc.ic_timout = 0;
		ioc.ic_dp = (caddr_t)&ifreq;
		ioc.ic_len = sizeof(struct ifreq);
		if(ioctl(vs, I_STR, &ioc)) {
#else /* ! STREAMS_TLI */
		if (ioctl(vs, SIOCGIFFLAGS, (char *)&ifreq) < 0) {
#endif /* STREAMS_TLI */
			syslog(LOG_ERR, "get interface flags: %m");
			continue;
		}
		if ((ifreq.ifr_flags & IFF_UP) == 0)
			continue;
		inter_list[i].flags = 0;
		if (ifreq.ifr_flags & IFF_BROADCAST)
			inter_list[i].flags |= INT_BROADCAST;
#if !defined(SUN_3_3_STINKS)
#if defined(SYS_HPUX) && (SYS_HPUX < 8)
		if (ifreq.ifr_flags & IFF_LOCAL_LOOPBACK)
#else
		if (ifreq.ifr_flags & IFF_LOOPBACK)
#endif
		{
			inter_list[i].flags |= INT_LOOPBACK;
			if (loopback_interface == 0)
				loopback_interface = &inter_list[i];
		}
#endif

#ifdef STREAMS_TLI
		ioc.ic_cmd = SIOCGIFADDR;
		ioc.ic_timout = 0;
		ioc.ic_dp = (caddr_t)&ifreq;
		ioc.ic_len = sizeof(struct ifreq);
		if(ioctl(vs, I_STR, &ioc)) {
#else /* ! STREAMS_TLI */
		if (ioctl(vs, SIOCGIFADDR, (char *)&ifreq) < 0) {
#endif /* STREAMS_TLI */
			syslog(LOG_ERR, "get interface addr: %m");
			continue;
		}

		(void)strncpy(inter_list[i].name, ifreq.ifr_name,
		    sizeof(inter_list[i].name));
		inter_list[i].sin = *(struct sockaddr_in *)&ifreq.ifr_addr;
		inter_list[i].sin.sin_family = AF_INET;
		inter_list[i].sin.sin_port = port;

#if defined(SUN_3_3_STINKS)
		/*
		 * Oh, barf!  I'm too disgusted to even explain this
		 */
		if (SRCADR(&inter_list[i].sin) == 0x7f000001) {
			inter_list[i].flags |= INT_LOOPBACK;
			if (loopback_interface == 0)
				loopback_interface = &inter_list[i];
		}
#endif
		if (inter_list[i].flags & INT_BROADCAST) {
#ifdef STREAMS_TLI
			ioc.ic_cmd = SIOCGIFBRDADDR;
			ioc.ic_timout = 0;
			ioc.ic_dp = (caddr_t)&ifreq;
			ioc.ic_len = sizeof(struct ifreq);
			if(ioctl(vs, I_STR, &ioc)) {
#else /* ! STREAMS_TLI */
			if (ioctl(vs, SIOCGIFBRDADDR, (char *)&ifreq) < 0) {
#endif /* STREAMS_TLI */
				syslog(LOG_ERR, "SIOCGIFBRDADDR fails");
				exit(1);
			}
#ifndef ifr_broadaddr
			inter_list[i].bcast =
				*(struct sockaddr_in *)&ifreq.ifr_addr;
#else
			inter_list[i].bcast =
				*(struct sockaddr_in *)&ifreq.ifr_broadaddr;
#endif
			inter_list[i].bcast.sin_family = AF_INET;
			inter_list[i].bcast.sin_port = port;
		}
#ifdef STREAMS_TLI
		ioc.ic_cmd = SIOCGIFNETMASK;
		ioc.ic_timout = 0;
		ioc.ic_dp = (caddr_t)&ifreq;
		ioc.ic_len = sizeof(struct ifreq);
		if(ioctl(vs, I_STR, &ioc)) {
#else /* ! STREAMS_TLI */
		if (ioctl(vs, SIOCGIFNETMASK, (char *)&ifreq) < 0) {
#endif /* STREAMS_TLI */
			syslog(LOG_ERR, "SIOCGIFNETMASK fails");
			exit(1);
		}
		inter_list[i].mask = *(struct sockaddr_in *)&ifreq.ifr_addr;

		/*
		 * look for an already existing source interface address.  If
		 * the machine has multiple point to point interfaces, then
		 * the local address may appear more than once.
		 */
		for (j=0; j < i; j++)
			if (inter_list[j].sin.sin_addr.s_addr ==
			    inter_list[i].sin.sin_addr.s_addr) {
				break;
			}
		if (j == i)
			i++;
	}
	close(vs);
#else /* SYS_WINNT */
	/* don't know how to get information about network interfaces on Win/NT
	 * one socket bound to wildcard interface is good enough for now
	 */
	 i = 1;
#endif /* SYS_WINNT */
#endif  /* _BSDI_VERSION >= 199510 */
	ninterfaces = i;

	maxactivefd = 0;
	FD_ZERO(&activefds);
			
	for (i = 0; i < ninterfaces; i++) {
		inter_list[i].fd = open_socket(&inter_list[i].sin,
		    inter_list[i].flags & INT_BROADCAST, 0);
	}

	/*
	 * Now that we have opened all the sockets, turn off the reuse flag for
	 * security.
	 */
	for (i = 0; i < ninterfaces; i++) {
		int off = 0;
		if (setsockopt(inter_list[i].fd, SOL_SOCKET, SO_REUSEADDR,
				(char *)&off, sizeof(off))) {
			syslog(LOG_ERR, "setsockopt SO_REUSEADDR off fails: %m");
		}
	}

#if defined(MCAST) && !defined(sun) && !defined(SYS_BSDI) && !defined(SYS_DECOSF1) && !defined(SYS_44BSD)
	/*
	 * enable possible multicast reception on the broadcast socket
	 */
	inter_list[0].bcast.sin_addr.s_addr = htonl(INADDR_ANY);
	inter_list[0].bcast.sin_family = AF_INET;
	inter_list[0].bcast.sin_port = port;
#endif /* MCAST */

	/*
	 * Blacklist all bound interface addresses
	 */
	resmask.sin_addr.s_addr = ~0L;
	for (i = 1; i < ninterfaces; i++)
		restrict(RESTRICT_FLAGS, &inter_list[i].sin, &resmask,
		    RESM_NTPONLY|RESM_INTERFACE, RES_IGNORE);

	any_interface = &inter_list[0];
#ifdef DEBUG
	if (debug > 2) {
		printf("create_sockets: ninterfaces=%d\n", ninterfaces);
		for (i = 0; i < ninterfaces; i++) {
			printf("interface %d:  fd=%d,  bfd=%d,  name=%.8s,  flags=0x%x\n",
				i,
				inter_list[i].fd,
				inter_list[i].bfd,
				inter_list[i].name,
				inter_list[i].flags);
			/* Leave these as three printf calls. */
			printf("              sin=%s",
				inet_ntoa((inter_list[i].sin.sin_addr)));
			if(inter_list[i].flags & INT_BROADCAST)
				printf("  bcast=%s,",
					inet_ntoa((inter_list[i].bcast.sin_addr)));
			printf("  mask=%s\n",
				inet_ntoa((inter_list[i].mask.sin_addr)));
		}
	}
#endif
	return ninterfaces;
}


/*
 * io_setbclient - open the broadcast client sockets
 */
void
io_setbclient()
{
	int i;

	for (i = 1; i < ninterfaces; i++) {
		if (!(inter_list[i].flags & INT_BROADCAST))
			continue;
		if (inter_list[i].flags & INT_BCASTOPEN)
			continue;
#ifdef	SYS_SOLARIS
		inter_list[i].bcast.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
#if !defined(SYS_DOMAINOS) && !defined(SYS_LINUX)
		inter_list[i].bfd = open_socket(&inter_list[i].bcast, 0, 1);
		inter_list[i].flags |= INT_BCASTOPEN;
#endif
	}
}


/*
 * io_multicast_add() - add multicast group address
 */
void
io_multicast_add(addr)
	u_long addr;
{
#ifdef MCAST
	struct ip_mreq mreq;
	int i = ninterfaces;	/* Use the next interface */
	u_long haddr = ntohl(addr);
	struct in_addr iaddr;
	int s;
	struct sockaddr_in *sinp;

	iaddr.s_addr = addr;

	if (!IN_CLASSD(haddr))
	{	syslog(LOG_ERR,
		    "cannot add multicast address %s as it is not class D",
		    inet_ntoa(iaddr));
		return;
	}

	for (i=0; i<ninterfaces; i++) {
		/* Already have this address */
		if (inter_list[i].sin.sin_addr.s_addr == addr) return;
		/* found a free slot */
		if (inter_list[i].sin.sin_addr.s_addr == 0 &&
		    inter_list[i].fd <= 0 && inter_list[i].bfd <= 0 &&
		    inter_list[i].flags == 0) break;
	}
	sinp = &(inter_list[i].sin);

	memset((char *)&mreq, 0, sizeof(mreq));
	memset((char *)&inter_list[i], 0, sizeof inter_list[0]);
	sinp->sin_family = AF_INET;
	sinp->sin_addr = iaddr;
	sinp->sin_port = htons(123);

	s = open_socket(sinp, 0, 1);
	/* Try opening a socket for the specified class D address */
	/* This works under SunOS 4.x, but not OSF1 .. :-( */
	if (s < 0) {
		memset((char *)&inter_list[i], 0, sizeof inter_list[0]);
		i = 0;
		/* HACK ! -- stuff in an address */
		inter_list[i].bcast.sin_addr.s_addr = addr;
		syslog(LOG_ERR, "...multicast address %s using wildcard socket",
		    inet_ntoa(iaddr));
	}
	else {
		inter_list[i].fd = s;
		inter_list[i].bfd = -1;
		(void) strncpy(inter_list[i].name, "multicast",
		sizeof(inter_list[i].name));
		inter_list[i].mask.sin_addr.s_addr = htonl(~0);
	}

	/*
	 * enable reception of multicast packets
	 */
	mreq.imr_multiaddr = iaddr;
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(inter_list[i].fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
	    (char *)&mreq, sizeof(mreq)) == -1)
		syslog(LOG_ERR,
		"setsockopt IP_ADD_MEMBERSHIP fails: %m for %x / %x (%s)",
			mreq.imr_multiaddr, mreq.imr_interface.s_addr,
			inet_ntoa(iaddr));
	inter_list[i].flags |= INT_MULTICAST;
	if (i >= ninterfaces) ninterfaces = i+1;	
#else /* MCAST */
	struct in_addr iaddr;
	iaddr.s_addr = addr;
	syslog(LOG_ERR, "cannot add multicast address %s as no MCAST support",
	    inet_ntoa(iaddr));
#endif /* MCAST */
}

/*
 * io_unsetbclient - close the broadcast client sockets
 */
void
io_unsetbclient()
{
        int i;

	for (i = 1; i < ninterfaces; i++) {
		if (!(inter_list[i].flags & INT_BCASTOPEN))
			continue;
		close_socket(inter_list[i].bfd);
		inter_list[i].bfd = -1;
		inter_list[i].flags &= ~INT_BCASTOPEN;
        }
}


/*
 * io_multicast_del() - delete multicast group address
 */
void
io_multicast_del(addr)
	u_long addr;
{
#ifdef MCAST
        int i;
	struct ip_mreq mreq;
	struct sockaddr_in sinaddr;

	if (!IN_CLASSD(addr)) {
		sinaddr.sin_addr.s_addr = addr;
		syslog(LOG_ERR,
		    "invalid multicast address %s", ntoa(&sinaddr));
		return;
	}

	/*
	 * Disable reception of multicast packets
	 */
	mreq.imr_multiaddr.s_addr = addr;
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	for (i = 0; i < ninterfaces; i++) {
		if (!(inter_list[i].flags & INT_MULTICAST))
			continue;
		if (!(inter_list[i].fd < 0))
			continue;
		if (addr != inter_list[i].sin.sin_addr.s_addr)
			continue;
		if (i != 0) {
			/* we have an explicit fd, so we can slose it */
			close_socket(inter_list[i].fd);
			memset((char *)&inter_list[i], 0, sizeof inter_list[0]);
			inter_list[i].fd = -1;
			inter_list[i].bfd = -1;
		} else {
			/* We are sharing "any address" port :-(  Don't close it! */
			if (setsockopt(inter_list[i].fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
				(char *)&mreq, sizeof(mreq)) == -1)
				syslog(LOG_ERR, "setsockopt IP_DROP_MEMBERSHIP fails: %m");
			/* This is **WRONG** -- there may be others ! */
			/* There should be a count of users ... */
			inter_list[i].flags &= ~INT_MULTICAST;
		}
        }
#else /* MCAST */
	syslog(LOG_ERR, "this function requires multicast kernel");
#endif /* MCAST */
}


/*
 * open_socket - open a socket, returning the file descriptor
 */
static int
open_socket(addr, flags, turn_off_reuse)
	struct sockaddr_in *addr;
	int flags;
	int turn_off_reuse;
{
	int fd;
	int on = 1, off = 0;

	/* create a datagram (UDP) socket */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		syslog(LOG_ERR, "socket(AF_INET, SOCK_DGRAM, 0) failed: %m");
		exit(1);
		/*NOTREACHED*/
	}

	/* set SO_REUSEADDR since we will be binding the same port
	   number on each interface */
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
			  (char *)&on, sizeof(on))) {
		syslog(LOG_ERR, "setsockopt SO_REUSEADDR on fails: %m");
	}

	/*
	 * bind the local address.
	 */
	if (bind(fd, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
		char buff[160];
		sprintf(buff,
	"bind() fd %d, family %d, port %d, addr %08lx, in_classd=%d flags=%d fails: %%m",
		    fd, addr->sin_family, (int)ntohs(addr->sin_port),
		    (u_long)ntohl(addr->sin_addr.s_addr),
		    IN_CLASSD(ntohl(addr->sin_addr.s_addr)), flags);
		syslog(LOG_ERR, buff);
#ifndef SYS_WINNT
		close(fd);
#else
		closesocket(fd);
#endif

		/*
		 * soft fail if opening a class D address
		 */
		if (IN_CLASSD(ntohl(addr->sin_addr.s_addr)))
			return -1;
		exit(1);
	}
#ifdef DEBUG
	if (debug)
		printf("bind() fd %d, family %d, port %d, addr %08lx, flags=%d\n",
			fd,
			addr->sin_family,
			(int)ntohs(addr->sin_port),
			(u_long)ntohl(addr->sin_addr.s_addr),
			flags);
#endif
	if (fd > maxactivefd)
		maxactivefd = fd;
	FD_SET(fd, &activefds);

#ifdef HAVE_SIGNALED_IO
        init_socket_sig(fd);
#else /* HAVE_SIGNALED_IO */

	/*
	 * set non-blocking,
	 */
#ifndef SYS_WINNT
#if defined(O_NONBLOCK)
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
		syslog(LOG_ERR, "fcntl(O_NONBLOCK) fails: %m");
		exit(1);
		/*NOTREACHED*/
	}
#else /* O_NONBLOCK */
#if defined(FNDELAY)
	if (fcntl(fd, F_SETFL, FNDELAY) < 0) {
		syslog(LOG_ERR, "fcntl(FNDELAY) fails: %m");
		exit(1);
		/*NOTREACHED*/
	}
#else /* FNDELAY */
#if defined(VMS) /* VMS+UCX */
	if (ioctl(fd,FIONBIO,&1) < 0) {
		syslog(LOG_ERR, "ioctl(FIONBIO) fails: %m");
		exit(1);
	}
#else
Need non blocking I/O
#endif /* VMS+UCX */
#endif /* FNDELAY */
#endif /* O_NONBLOCK */
#else /* SYS_WINNT */
	if (ioctlsocket(fd,FIONBIO,(u_long *) &on) == SOCKET_ERROR) {
		syslog(LOG_ERR, "ioctlsocket(FIONBIO) fails: %m");
		exit(1);
	}
#endif /* SYS_WINNT */
#endif /* HAVE_SIGNALED_IO */

	/*
	 *  Turn off the SO_REUSEADDR socket option.  It apparently
	 *  causes heartburn on systems with multicast IP installed.
	 *  On normal systems it only gets looked at when the address
	 *  is being bound anyway..
	 */
	if (turn_off_reuse)
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
			(char *)&off, sizeof(off))) {
			syslog(LOG_ERR, "setsockopt SO_REUSEADDR off fails: %m");
		}

#ifdef SO_BROADCAST
	/* if this interface can support broadcast, set SO_BROADCAST */
	if (flags & INT_BROADCAST) {
		if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST,
		    (char *)&on, sizeof(on))) {
			syslog(LOG_ERR, "setsockopt(SO_BROADCAST): %m");
		}
	}
#endif /* SO_BROADCAST */

#if !defined(SYS_WINNT) && !defined(VMS)
#ifdef DEBUG
	if (debug > 1)
		printf("flags for fd %d: 0%o\n", fd,
		    fcntl(fd, F_GETFL, 0));
#endif
#endif /* SYS_WINNT || VMS */

	return fd;
}


/*
 * closesocket - close a socket and remove from the activefd list
 */
static void
close_socket(fd)
	int fd;
{
	int i, newmax;

#ifndef SYS_WINNT
	(void) close(fd);
#else
	closesocket(fd);
#endif /* SYS_WINNT */
	FD_CLR(fd, &activefds);

	if (fd >= maxactivefd) {
		newmax = 0;
		for (i = 0; i < maxactivefd; i++)
			if (FD_ISSET(i, &activefds))
				newmax = i;
		maxactivefd = newmax;
	}
}



/*
 * findbcastinter - find broadcast interface corresponding to address
 */
struct interface *
findbcastinter(addr)
	struct sockaddr_in *addr;
{
#ifdef SIOCGIFCONF
	register int i;
	register u_long netnum;

	netnum = NSRCADR(addr);
	for (i = 1; i < ninterfaces; i++) {
		if (!(inter_list[i].flags & INT_BROADCAST))
			continue;
		if (NSRCADR(&inter_list[i].bcast) == netnum)
			return &inter_list[i];
		if ((NSRCADR(&inter_list[i].sin) & NSRCADR(&inter_list[i].mask))
		    == (netnum & NSRCADR(&inter_list[i].mask)))
			return &inter_list[i];
	}
#endif /* SIOCGIFCONF */
	return any_interface;
}


/* XXX ELIMINATE getrecvbufs (almost) identical to ntpdate.c, ntptrace.c, ntp_io.c */
/*
 * getrecvbufs - get receive buffers which have data in them
 *
 * ***N.B. must be called with SIGIO blocked***
 */
struct recvbuf *
getrecvbufs()
{
	struct recvbuf *rb;

#ifdef DEBUG
	if (debug > 4)
		printf("getrecvbufs: %ld handler interrupts, %ld frames\n",
		    handler_calls, handler_pkts);
#endif

	if (full_recvbufs == 0) {
#ifdef DEBUG
		if (debug > 4)
			printf("getrecvbufs called, no action here\n");
#endif
		return (struct recvbuf *)0;	/* nothing has arrived */
	}
	
	/*
	 * Get the fulllist chain and mark it empty
	 */
#ifdef DEBUG
	if (debug > 4)
		printf("getrecvbufs returning %ld buffers\n", full_recvbufs);
#endif
	rb = beginlist;
	fulllist = 0;
	full_recvbufs = 0;

	/*
	 * Check to see if we're below the low water mark.
	 */
	if (free_recvbufs <= RECV_LOWAT) {
		register struct recvbuf *buf;
		register int i;

		if (total_recvbufs >= RECV_TOOMANY)
			syslog(LOG_ERR, "too many recvbufs allocated (%d)",
			    total_recvbufs);
		else {
			buf = (struct recvbuf *)
			    emalloc(RECV_INC*sizeof(struct recvbuf));
			for (i = 0; i < RECV_INC; i++) {
				buf->next = freelist;
				freelist = buf;
				buf++;
			}

			free_recvbufs += RECV_INC;
			total_recvbufs += RECV_INC;
			lowater_additions++;
		}
	}

	/*
	 * Return the chain
	 */
	return rb;
}


/* XXX ELIMINATE freerecvbuf (almost) identical to ntpdate.c, ntptrace.c, ntp_io.c */
/*
 * freerecvbuf - make a single recvbuf available for reuse
 */
void
freerecvbuf(rb)
	struct recvbuf *rb;
{
	BLOCKIO();
	rb->next = freelist;
	freelist = rb;
	free_recvbufs++;
	UNBLOCKIO();
}


/* XXX ELIMINATE sendpkt similar in ntpq.c, ntpdc.c, ntp_io.c, ntptrace.c */
/*
 * sendpkt - send a packet to the specified destination. Maintain a
 * send error cache so that only the first consecutive error for a
 * destination is logged.
 */
void
sendpkt(dest, inter, ttl, pkt, len)
	struct sockaddr_in *dest;
	struct interface *inter;
	int ttl;
	struct pkt *pkt;
	int len;
{
	int cc, slot;
#ifdef SYS_WINNT
	DWORD err;
#endif /* SYS_WINNT */
	/*
	 * Send error cache. Empty slots have port == 0
	 * Set ERRORCACHESIZE to 0 to disable
	 */
	struct cache {
		u_short	port;
		struct	in_addr addr;
	};

#ifndef ERRORCACHESIZE
#define ERRORCACHESIZE 8
#endif
#if ERRORCACHESIZE > 0
	static struct cache badaddrs[ERRORCACHESIZE];
#else
#define badaddrs ((struct cache *)0)		/* Only used in empty loops! */
#endif

#ifdef DEBUG
	if (debug)
		printf("%ssendpkt(fd=%d %s, %s, ttl=%d, %d)\n",
			(ttl >= 0) ? "\tMCAST\t*****" : "",
			inter->fd, ntoa(dest),
			ntoa(&inter->sin), ttl, len);
#endif

#ifdef MCAST
	/* for the moment we use the bcast option to set multicast ttl */
	if (ttl >= 0 && ttl != inter->last_ttl) {
	    char mttl = ttl;

	    /* set the multicast ttl for outgoing packets */
	    if (setsockopt(inter->fd, IPPROTO_IP, IP_MULTICAST_TTL,
			   &mttl, sizeof(mttl)) == -1) {
		syslog(LOG_ERR, "setsockopt IP_MULTICAST_TTL fails: %m");
	    }
	    else inter->last_ttl = ttl;
	}
#endif /* MCAST */

	for (slot = ERRORCACHESIZE; --slot >= 0; )
		if (badaddrs[slot].port == dest->sin_port &&
		    badaddrs[slot].addr.s_addr == dest->sin_addr.s_addr)
			break;
			
	cc = sendto(inter->fd, (char *)pkt, len, 0, (struct sockaddr *)dest,
	    sizeof(struct sockaddr_in));
	if (cc == -1) {
		inter->notsent++;
		packets_notsent++;
#ifndef SYS_WINNT
		if (errno != EWOULDBLOCK && errno != ENOBUFS && slot < 0) {
#else
		err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK && err != WSAENOBUFS && slot <0) {
#endif /* SYS_WINNT */
			/*
			 * Remember this, if there's an empty slot
			 */
			for (slot = ERRORCACHESIZE; --slot >= 0; )
				if (badaddrs[slot].port == 0) {
					badaddrs[slot].port = dest->sin_port;
					badaddrs[slot].addr = dest->sin_addr;
					break;
				}
			syslog(LOG_ERR, "sendto(%s): %m", ntoa(dest));
		}
	} else {
		inter->sent++;
		packets_sent++;
		/*
		 * He's not bad any more
		 */
		if (slot >= 0)
			badaddrs[slot].port = 0;
	}
}

/*
 * fdbits - generate ascii representation of fd_set (FAU debug support)
 * HFDF format - highest fd first.
 */
static char *
fdbits(count, set)
  int count;
  fd_set *set;
{
  static char buffer[256];
  char * buf = buffer;

  count = (count < 256) ? count : 255;

  while (count >= 0) {
   *buf++ = FD_ISSET(count, set) ? '#' : '-';
   count--;
  }
  *buf = '\0';

  return buffer;
}

/*
 * input_handler - receive packets asynchronously
 */
void
input_handler(cts)
	l_fp *cts;
{
	register int i, n;
	register struct recvbuf *rb;
	register int doing;
	register int fd;
	struct timeval tvzero;
	int fromlen;
	l_fp ts;
	fd_set fds;
	int first = 1;

	handler_calls++;
	ts = *cts;

	/*
	 * Do a poll to see who has data
	 */
again:
	fds = activefds;
	tvzero.tv_sec = tvzero.tv_usec = 0;
	n = select(maxactivefd+1, &fds, (fd_set *)0, (fd_set *)0, &tvzero);

	/*
	 * If nothing to do, just return.  If an error occurred, complain
	 * and return.  If we've got some, freeze a timestamp.
	 */
	if (n == 0)
		return;
	else if (n == -1) {
#ifndef SYS_WINNT
		int err = errno;
#else
		DWORD err = WSAGetLastError();
#endif /* SYS_WINNT */

		/*
		 * extended FAU debugging output
		 */
		syslog(LOG_ERR, "select(%d, %s, 0L, 0L, &0.000000) error: %m",
				 maxactivefd+1, fdbits(maxactivefd, &activefds));
#ifndef SYS_WINNT
		if (err == EBADF) {
#else
		if (err = WSAEBADF) {
#endif /* SYS_WINNT */
		  int i, b;

		  fds = activefds;
		  for (i = 0; i <= maxactivefd; i++)
#ifndef SYS_WINNT
		    if (FD_ISSET(i, &fds) && (read(i, &b, 0) == -1))
#else
			if (FD_ISSET(i, &fds) && (!ReadFile((HANDLE)i, &b, 0, NULL, NULL)))
#endif /* SYS_WINNT */
		      syslog(LOG_ERR, "Bad file descriptor %d", i);
		}
		return;
	}
	if (!first)get_systime(&ts);
	first = 0;
	handler_pkts += n;

#ifdef REFCLOCK
	/*
	 * Check out the reference clocks first, if any
	 */
	if (refio != 0) {
		register struct refclockio *rp;

		for (rp = refio; rp != 0 && n > 0; rp = rp->next) {
			fd = rp->fd;
			if (FD_ISSET(fd, &fds)) {
				n--;
				if (free_recvbufs == 0) {
					char buf[RX_BUFF_SIZE];

#ifndef SYS_WINNT
					(void) read(fd, buf, sizeof buf);
#else
					(void) ReadFile((HANDLE)fd, buf, (DWORD)sizeof buf, NULL, NULL);
#endif /* SYS_WINNT */
					packets_dropped++;
					continue;
				}

				rb = freelist;
				freelist = rb->next;
				free_recvbufs--;

				i = (rp->datalen == 0
				    || rp->datalen > sizeof(rb->recv_space))
				    ? sizeof(rb->recv_space) : rp->datalen;

#ifndef SYS_WINNT				
				rb->recv_length =
					read(fd, (char *)&rb->recv_space, i);
#else
					ReadFile((HANDLE)fd, (char *)&rb->recv_space, (DWORD)i,
					 (LPDWORD)&(rb->recv_length), NULL);
#endif /* SYS_WINNT */

				if (rb->recv_length == -1) {
					syslog(LOG_ERR, "clock read fd %d: %m",	fd);
					rb->next = freelist;
					freelist = rb;
					free_recvbufs++;
					continue;
				}
	
				/*
				 * Got one.  Mark how and when it got here,
				 * put it on the full list and do bookkeeping.
				 */
				rb->recv_srcclock = rp->srcclock;
				rb->dstadr = 0;
				rb->fd = fd;
				rb->recv_time = ts;
				rb->receiver = rp->clock_recv;

				if (fulllist == 0) {
					beginlist = rb;
					rb->next = 0;
				} else {
					rb->next = fulllist->next;
					fulllist->next = rb;
				}
				fulllist = rb;
				full_recvbufs++;

				rp->recvcount++;
				packets_received++;
			}
		}
	}
#endif

	/*
	 * Loop through the interfaces looking for data to read.
	 */
	for (i = ninterfaces-1; i >= 0 && n > 0; i--) {
		for (doing = 0; doing < 2 && n > 0; doing++) {
			if (doing == 0) {
				fd = inter_list[i].fd;
			} else {
				if (!(inter_list[i].flags & INT_BCASTOPEN))
					break;
				fd = inter_list[i].bfd;
			}
			if (fd < 0) continue;
			if (FD_ISSET(fd, &fds)) {
				n--;

				/*
				 * Get a buffer and read the frame.  If we
				 * haven't got a buffer, or this is received
				 * on the wild card socket, just dump the
				 * packet.
				 */
#ifndef SYS_WINNT
				if (!(free_recvbufs && i == 0 &&
				    inter_list[i].flags & INT_MULTICAST)) {
#else
				/* For Win/NT since we are creating just one socket,
				 * i is bound to be 0, and INT_MULTICAST flag is not set
				 */
				if (!free_recvbufs) {
#endif /* SYS_WINNT */
#ifdef UDP_WILDCARD_DELIVERY
				/*
				 * these guys manage to put properly addressed
				 * packets into the wildcard queue
				 */
				if (free_recvbufs == 0) {
#else
					if (i == 0 || free_recvbufs == 0) {
#endif
						char buf[RX_BUFF_SIZE];
						struct sockaddr from;
						fromlen = sizeof from;
						(void) recvfrom(fd, buf,
						    sizeof(buf), 0,
						    &from, (size_t *)&fromlen);
#ifdef DEBUG
	if (debug)
		printf("ignore/drop on %d(%lu) fd=%d from %s\n",
		    i, free_recvbufs, fd,
		    inet_ntoa(((struct sockaddr_in *) &from)->sin_addr));
#endif
						if (i == 0)
							packets_ignored++;
						else
							packets_dropped++;
						continue;
					}
				}
	
				rb = freelist;
				freelist = rb->next;
				free_recvbufs--;
	
				fromlen = sizeof(struct sockaddr_in);
				rb->recv_length = recvfrom(fd,
				    (char *)&rb->recv_space,
				    sizeof(rb->recv_space), 0,
				    (struct sockaddr *)&rb->recv_srcadr,
				    (size_t *)&fromlen);
				if (rb->recv_length == -1) {
					syslog(LOG_ERR, "recvfrom: %m");
					rb->next = freelist;
					freelist = rb;
					free_recvbufs++;
#ifdef DEBUG
	if (debug)
		printf("input_handler: fd=%d dropped (bad recvfrom)\n", fd);
#endif
					continue;
				}
#ifdef DEBUG
	if (debug)
		printf("input_handler: fd=%d length %d from %08lx %s\n",
		    fd, rb->recv_length,
		    (u_long)ntohl(rb->recv_srcadr.sin_addr.s_addr) &
		    0x00000000ffffffff, inet_ntoa(rb->recv_srcadr.sin_addr));
#endif

				/*
				 * Got one.  Mark how and when it got here,
				 * put it on the full list and do bookkeeping.
				 */
				rb->dstadr = &inter_list[i];
				rb->fd = fd;
				rb->recv_time = ts;
				rb->receiver = receive;
	

				if (fulllist == 0) {
					beginlist = rb;
					rb->next = 0;
				} else {
					rb->next = fulllist->next;
					fulllist->next = rb;
				}
				fulllist = rb;
				full_recvbufs++;
	
				inter_list[i].received++;
				packets_received++;
			}
		}
	}
	/*
	 * Done everything from that select.  Poll again.
	 */
	goto again;
}


/*
 * findinterface - utility used by other modules to find an interface
 *		   given an address.
 */
struct interface *
findinterface(addr)
	struct sockaddr_in *addr;
{
	register int i;
	register u_long saddr;

	/*
	 * Just match the address portion.
	 */
	saddr = addr->sin_addr.s_addr;
	for (i = 0; i < ninterfaces; i++) {
		if (inter_list[i].sin.sin_addr.s_addr == saddr)
			return &inter_list[i];
	}
	return (struct interface *)0;
}


/*
 * io_clr_stats - clear I/O module statistics
 */
void
io_clr_stats()
{
	packets_dropped = 0;
	packets_ignored = 0;
	packets_received = 0;
	packets_sent = 0;
	packets_notsent = 0;

	handler_calls = 0;
	handler_pkts = 0;
	io_timereset = current_time;
}


#ifdef REFCLOCK
/*
 * This is a hack so that I don't have to fool with these ioctls in the
 * pps driver ... we are already non-blocking and turn on SIGIO thru
 * another mechanisim
 */
int
io_addclock_simple(rio)
	struct refclockio *rio;
{
	BLOCKIO();
	/*
	 * Stuff the I/O structure in the list and mark the descriptor
	 * in use.  There is a harmless (I hope) race condition here.
	 */
	rio->next = refio;
	refio = rio;

	if (rio->fd > maxactivefd)
		maxactivefd = rio->fd;
	FD_SET(rio->fd, &activefds);
	UNBLOCKIO();
	return 1;
}

/*
 * io_addclock - add a reference clock to the list and arrange that we
 *               get SIGIO interrupts from it.
 */
int
io_addclock(rio)
        struct refclockio *rio;
{
	BLOCKIO();
        /*
         * Stuff the I/O structure in the list and mark the descriptor
         * in use.  There is a harmless (I hope) race condition here.
         */
        rio->next = refio;
        refio = rio;

#ifdef HAVE_SIGNALED_IO
	if (init_clock_sig(rio)) { 
		refio = rio->next;
		UNBLOCKIO();
		return 0;
	}
#endif

        if (rio->fd > maxactivefd)
                maxactivefd = rio->fd;
        FD_SET(rio->fd, &activefds);

	UNBLOCKIO();
        return 1;
}

/*
 * io_closeclock - close the clock in the I/O structure given
 */
void
io_closeclock(rio)
	struct refclockio *rio;
{
	/*
	 * Remove structure from the list
	 */
	if (refio == rio) {
		refio = rio->next;
	} else {
		register struct refclockio *rp;

		for (rp = refio; rp != 0; rp = rp->next)
			if (rp->next == rio) {
				rp->next = rio->next;
				break;
			}
		
		if (rp == 0) {
			/*
			 * Internal error.  Report it.
			 */
			syslog(LOG_ERR,
			    "internal error: refclockio structure not found");
			return;
		}
	}

	/*
	 * Close the descriptor.  close_socket does the right thing despite
	 * the misnomer.
	 */
	close_socket(rio->fd);
}
#endif	/* REFCLOCK */

/*
 * SIGPOLL and SIGIO ROUTINES.
 */
#ifdef HAVE_SIGNALED_IO
/*
 * Some systems (MOST) define SIGPOLL==SIGIO others SIGIO==SIGPOLL a few
 * have seperate SIGIO and SIGPOLL signals.  This code checks for the
 * SIGIO==SIGPOLL case at compile time.
 * Do not defined USE_SIGPOLL or USE_SIGIO.
 * these are interal only to ntp_io.c!
 */
#if defined(USE_SIGPOLL)
#undef USE_SIGPOLL
#endif
#if defined(USE_SIGIO)
#undef USE_SIGIO
#endif

#if defined(USE_TTY_SIGPOLL)||defined(USE_UDP_SIGPOLL)
#define USE_SIGPOLL
#endif

#if !defined(USE_TTY_SIGPOLL)||!defined(USE_UDP_SIGPOLL)
#define USE_SIGIO
#endif

#if defined(USE_SIGIO)&&defined(USE_SIGPOLL)
#if SIGIO==SIGPOLL
#define USE_SIGIO
#undef USE_SIGPOLL
#endif /* SIGIO==SIGPOLL */
#endif /* USE_SIGIO && USE_SIGIO */


/*
 * TTY instialzation routeins.
 */
#ifndef USE_TTY_SIGPOLL
/*
 * Spical cases first!
 */
#if defined(SYS_HPUX)
#define CLOCK_DONE
static int
init_clock_sig(rio)
	struct refclockio *rio;
{
	int pgrp, on = 1;
	
	/* DO NOT ATTEMPT TO MAKE CLOCK-FD A CTTY: not portable, unreliable */
        pgrp = getpid();
        if (ioctl(rio->fd, FIOSSAIOOWN, (char *)&pgrp) == -1) {
                syslog(LOG_ERR, "ioctl(FIOSSAIOOWN) fails for clock I/O: %m");
                exit(1);
                /*NOTREACHED*/
        }

        /*
         * set non-blocking, async I/O on the descriptor
         */
        if (ioctl(rio->fd, FIOSNBIO, (char *)&on) == -1) {
                syslog(LOG_ERR, "ioctl(FIOSNBIO) fails for clock I/O: %m");
                exit(1);
                /*NOTREACHED*/
        }

        if (ioctl(rio->fd, FIOSSAIOSTAT, (char *)&on) == -1) {
                syslog(LOG_ERR, "ioctl(FIOSSAIOSTAT) fails for clock I/O: %m");
                exit(1);
                /*NOTREACHED*/
        }
	return 0;	
}
#endif /* SYS_HPUX */
#if defined(SYS_AIX)&&!defined(_BSD)
/*
 * SYSV compatibility mode under AIX.
 */
#define CLOCK_DONE
static int
init_clock_sig(rio)
	struct refclockio *rio;
{
	int pgrp, on = 1;

	/* DO NOT ATTEMPT TO MAKE CLOCK-FD A CTTY: not portable, unreliable */
        if (ioctl(rio->fd, FIOASYNC, (char *)&on) == -1) {
                syslog(LOG_ERR, "ioctl(FIOASYNC) fails for clock I/O: %m");
		return 1;
        }
        pgrp = -getpid();
        if (ioctl(rio->fd, FIOSETOWN, (char*)&pgrp) == -1) {
                syslog(LOG_ERR, "ioctl(FIOSETOWN) fails for clock I/O: %m");
		return 1;
        }

	if (fcntl(rio->fd, F_SETFL, FNDELAY|FASYNC) < 0) {
                syslog(LOG_ERR, "fcntl(FNDELAY|FASYNC) fails for clock I/O: %m");
		return 1;
        }
	return 0;
}
#endif /* AIX && !BSD */
#ifndef  CLOCK_DONE
static int
init_clock_sig(rio)
	struct refclockio *rio;
{
	/* DO NOT ATTEMPT TO MAKE CLOCK-FD A CTTY: not portable, unreliable */
#if defined(TIOCSCTTY) && defined(USE_FSETOWNCTTY)
	/*
	 * there are, however, always exceptions to the rules
	 * one is, that OSF accepts SETOWN on TTY fd's only, iff they are
	 * CTTYs. SunOS and HPUX do not semm to have this restriction.
	 * another question is: how can you do multiple SIGIO from several
	 * ttys (as they all should be CTTYs), wondering...
	 *
	 * kd 95-07-16
	 */
	if (ioctl(rio->fd, TIOCSCTTY, 0) == -1) {
		syslog(LOG_ERR, "ioctl(TIOCSCTTY, 0) fails for clock I/O: %m");
		return 1;
	}
#endif /* TIOCSCTTY && USE_FSETOWNCTTY */

        if (fcntl(rio->fd, F_SETOWN, getpid()) == -1) {
                syslog(LOG_ERR, "fcntl(F_SETOWN) fails for clock I/O: %m");
                return 1;
        }

        if (fcntl(rio->fd, F_SETFL, FNDELAY|FASYNC) < 0) {
                syslog(LOG_ERR,
                    "fcntl(FNDELAY|FASYNC) fails for clock I/O: %m");
                return 1;
        }
	return 0;	
}
#endif /* CLOCK_DONE */
#else /* !USE_TTY_SIGPOLL */
int
static init_clock_sig(rio)
	struct refclockio *rio;
{
	/* DO NOT ATTEMPT TO MAKE CLOCK-FD A CTTY: not portable, unreliable */
	if (ioctl(rio->fd, I_SETSIG, S_INPUT) < 0) {
                syslog(LOG_ERR,
                    "ioctl(I_SETSIG, S_INPUT) fails for clock I/O: %m");
		return 1;
	}
        return 0;
}
#endif /* !USE_TTY_SIGPOLL  */



#ifndef USE_UDP_SIGPOLL
/*
 * Socket SIGPOLL initialization routines.
 * Special cases first!
 */
#if defined(SYS_HPUX) || defined(SYS_LINUX)
#define SOCKET_DONE
static void
init_socket_sig(fd)
	int fd;
{
        int pgrp, on = 1;

        /*
         * Big difference here for HP-UX ... why can't life be easy ?
         */
        if (ioctl(fd, FIOSNBIO, (char *)&on) == -1) {
                syslog(LOG_ERR, "ioctl(FIOSNBIO) fails: %m");
                exit(1);
                /*NOTREACHED*/
        }

        if (ioctl(fd, FIOASYNC, (char *)&on) == -1) {
                syslog(LOG_ERR, "ioctl(FIOASYNC) fails: %m");
                exit(1);
                /*NOTREACHED*/
        }

#if (SYS_HPUX > 7)
        pgrp = getpid();
#else
        pgrp = -getpid();
#endif
        if (ioctl(fd, SIOCSPGRP, (char *)&pgrp) == -1) {
                syslog(LOG_ERR, "ioctl(SIOCSPGRP) fails: %m");
                exit(1);
                /*NOTREACHED*/
        }
}
#endif /* SYS_HPUX */
#if defined(SYS_AIX)&&!defined(_BSD)
/*
 * SYSV compatibility mod under AIX
 */
#define SOCKET_DONE
static void
init_socket_sig(fd)
	int fd;
{
	int pgrp, on = 1;

        if (ioctl(fd, FIOASYNC, (char *)&on) == -1) {
                syslog(LOG_ERR, "ioctl(FIOASYNC) fails: %m");
                exit(1);
                /*NOTREACHED*/
        }
        pgrp = -getpid();
        if (ioctl(fd, FIOSETOWN, (char*)&pgrp) == -1) {
                syslog(LOG_ERR, "ioctl(FIOSETOWN) fails: %m");
                exit(1);
                /*NOTREACHED*/
        }

	if (fcntl(fd, F_SETFL, FNDELAY|FASYNC) < 0) {
                syslog(LOG_ERR, "fcntl(FNDELAY|FASYNC) fails: %m");
                exit(1);
                /*NOTREACHED*/
        }
}
#endif /* AIX && !BSD */
#if defined(UDP_BACKWARDS_SETOWN)
/*
 * SunOS 3.5 and Ultrix 2.0
 */
#define SOCKET_DONE
static void
init_socket_sig(fd)
	int fd;
{
        /*
         * The way Sun did it as recently as SunOS 3.5.  Only
         * in the case of sockets, of course, just to confuse
         * the issue.  Don't they even bother to test the stuff
         * they send out?  Ibid for Ultrix 2.0
         */
        if (fcntl(fd, F_SETOWN, -getpid()) == -1)
        {
                syslog(LOG_ERR, "fcntl(F_SETOWN) fails: %m");
                exit(1);
        }
	/*
         * set non-blocking, async I/O on the descriptor
         */
        if (fcntl(fd, F_SETFL, FNDELAY|FASYNC) < 0) {
                syslog(LOG_ERR, "fcntl(FNDELAY|FASYNC) fails: %m");
                exit(1);
                /*NOTREACHED*/
        }
}
#endif /* UDP_BACKWARDS_SETOWN */
#ifndef SOCKET_DONE
static void
init_socket_sig(fd)
	int fd;
{
        if (fcntl(fd, F_SETOWN, getpid()) == -1)
        {
                syslog(LOG_ERR, "fcntl(F_SETOWN) fails: %m");
                exit(1);
        }
	/*
         * set non-blocking, async I/O on the descriptor
         */
        if (fcntl(fd, F_SETFL, FNDELAY|FASYNC) < 0) {
                syslog(LOG_ERR, "fcntl(FNDELAY|FASYNC) fails: %m");
                exit(1);
                /*NOTREACHED*/
        }
}
#endif /* SOCKET_DONE */
#else /* !USE_UDP_SIGPOLL */
static void
init_socket_sig(fd)
	int fd;
{
	if (ioctl(fd, I_SETSIG, S_INPUT) < 0) {
                syslog(LOG_ERR,
                    "ioctl(I_SETSIG, S_INPUT) fails for socket I/O: %m");
                exit(1);
        }
}
#endif /* USE_UDP_SIGPOLL */

static RETSIGTYPE
sigio_handler(sig)
int sig;
{
	l_fp ts;

#ifdef SYS_SVR4
        /* This should not be necessary for a signal previously set with
         * sigset().
         */
#  if defined(USE_SIGIO)
        (void) sigset(SIGIO, sigio_handler);
#  endif
#  if defined(USE_SIGPOLL)
        (void) sigset(SIGPOLL, sigio_handler);
#  endif
#endif /* SYS_SVR4 */

	get_systime(&ts);
	(void)input_handler(&ts);
}

/*
 * Signal support routines.
 */
#ifdef NTP_POSIX_SOURCE
static void
set_signal()
{
    int n;
    struct sigaction vec;

    sigemptyset(&vec.sa_mask);

#ifdef USE_SIGIO
    sigaddset(&vec.sa_mask, SIGIO);
#endif
#ifdef USE_SIGPOLL
    sigaddset(&vec.sa_mask, SIGPOLL);
#endif
    vec.sa_flags = 0;

#if defined(USE_SIGIO)
    vec.sa_handler =  sigio_handler;

    while (1) {
        n = sigaction(SIGIO, &vec, NULL);
        if (n == -1 && errno == EINTR) continue;
        break;
    }

    if (n == -1) {
        perror("sigaction");
        exit(1);
    }
#endif
#if defined(USE_SIGPOLL)
    vec.sa_handler =  sigio_handler;

    while (1) {
        n = sigaction(SIGPOLL, &vec, NULL);
        if (n == -1 && errno == EINTR) continue;
        break;
    }

    if (n == -1) {
        perror("sigaction");
        exit(1);
    }
#endif
}

void
block_io_and_alarm()
{
	sigset_t set;

	sigemptyset(&set);
#if defined(USE_SIGIO)
	sigaddset(&set, SIGIO);
#endif
#if defined(USE_SIGPOLL)
	sigaddset(&set, SIGPOLL);
#endif
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_BLOCK, &set, NULL);
}

static void
block_sigio()
{
	sigset_t set;

	sigemptyset(&set);
#if defined(USE_SIGIO)
	sigaddset(&set, SIGIO);
#endif
#if defined(USE_SIGPOLL)
	sigaddset(&set, SIGPOLL);
#endif
	sigprocmask(SIG_BLOCK, &set, NULL);
}

void
unblock_io_and_alarm()
{
	sigset_t unset;

	sigemptyset(&unset);

#if defined(USE_SIGIO)
	sigaddset(&unset, SIGIO);
#endif
#if defined(USE_SIGPOLL)
	sigaddset(&unset, SIGPOLL);
#endif
	sigaddset(&unset, SIGALRM);
	sigprocmask(SIG_UNBLOCK, &unset, NULL);
}

static
void
unblock_sigio()
{
	sigset_t unset;

	sigemptyset(&unset);

#if defined(USE_SIGIO)
	sigaddset(&unset, SIGIO);
#endif
#if defined(USE_SIGPOLL)
	sigaddset(&unset, SIGPOLL);
#endif
	sigprocmask(SIG_UNBLOCK, &unset, NULL);
}

void
wait_for_signal()
{
	sigset_t old;

	sigprocmask(SIG_UNBLOCK, NULL, &old);

#if defined(USE_SIGIO)
	sigdelset(&old, SIGIO);
#endif
#if defined(USE_SIGPOLL)
	sigdelset(&old, SIGPOLL);
#endif
	sigdelset(&old, SIGALRM);

	sigsuspend(&old);
}

#else
/*
 * Must be an old bsd system.
 * We assume there is no SIGPOLL.
 */

void
block_io_and_alarm()
{
	int mask;

	mask = sigmask(SIGIO)|sigmask(SIGALRM);
	(void)sigblock(mask);
}

void
block_sigio()
{
	int mask;

	mask = sigmask(SIGIO);
	(void)sigblock(mask);
}

static void
set_signal()
{
	(void) signal_no_reset(SIGIO,   sigio_handler);
}

void
unblock_io_and_alarm()
{
	int mask, omask;

	mask = sigmask(SIGIO)|sigmask(SIGALRM);
	omask = sigblock(0);
	omask &= ~mask;
	(void)sigsetmask(omask);
}

void
unblock_sigio()
{
	int mask, omask;

	mask = sigmask(SIGIO);
	omask = sigblock(0);
	omask &= ~mask;
	(void)sigsetmask(omask);
}

void
wait_for_signal()
{
	int mask, omask;
	
	mask = sigmask(SIGIO)|sigmask(SIGALRM);
        omask = sigblock(0);
        omask &= ~mask;
	sigpause(omask);
}
#endif /* NTP_POSIX_SOURCE */
#endif /* HAVE_SIGNALED_IO */

void
kill_asyncio()
{
  int i;

  BLOCKIO();
  for (i = 0; i <= maxactivefd; i++)
    (void)close_socket(i);
}
