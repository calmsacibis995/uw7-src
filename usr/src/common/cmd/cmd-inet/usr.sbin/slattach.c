#ident	"@(#)slattach.c	1.4"
#ident	"$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */


#include <dial.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/termio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/stropts.h>
#include <netinet/in.h>
#include <net/route.h>
#include <netinet/ip_str.h>
#include <sys/sysmacros.h>
#include <netinet/slip.h>
#include <sys/wait.h>
#include <sys/dlpi.h>
#include <sys/dlpi_ether.h>
#include <sys/asyc.h>

#ifndef bcopy
#define bcopy(s1, s2, n) memcpy((void *)(s2), (void *)(s1), (n))
#endif
#ifndef bzero
#define bzero(s, n) memset((void *)(s), 0, (n))
#endif

#define TTY_MINCHARS	40
#define TTY_WAITTIME	1
#define DATABUFSIZE	256

int             alldone;
extern int      errno, sys_nerr;
extern char    *sys_errlist[];

int baudrate =
#if defined(m88k)
	B9600 << 16;
#else
	B9600;
#endif

#define IFNAME_PREF	"sl"
#define MAX_IFACES	64
#define	MIN_MTU		42		/* tcp/ip header + 2 */

char *getifname();

/* flags */
int	cflg = 0;	/* <> 0 ==> turn on tcp/ip header compression */
int	eflg = 0;	/* <> 0 ==> auto detect use of tcp/ip header comp. */
int 	fflg = 0;	/* <> 0 ==> turn on flow control */
int	iflg = 0;	/* <> 0 ==> don't send any ICMP packets */
int	dflg = 0;	/* <> 0 ==> dial the remote system */
int	vflg = 0;	/* <> 0 ==> yack while bringing up the interface. */

char            devname[80];


#define USAGE() fprintf(stderr, "usage: slattach [{+|-}{c|e|i|v|m mtu} ...] ttyname source-name destination-name [baudrate]\n")

extern int Uerror;
extern int conn();
extern void undial();
int	dialfd = 0;

void
un_dial(sig)
	int	sig;
{
	if (vflg) {
		printf("undial sig=%d \n",sig);
	}
	if (dialfd)
		undial(dialfd);
	exit(1);
}

sig_hup(sig)
	int	sig;
{
	if (vflg) {
		printf("sig_hup sig=%d \n",sig);
	}
	exit(1);
}

main(argc, argv)
	char           *argv[];
{
	register int    stream, ip;
	int     	try = 1;        /* # of times to try to get ifname */
	struct termios  term;
	register char  *dev;
	int             pid, ifmetric;
	int		baud = -1;
	int		mtu = 0;	/* only change mtu if mtu != 0 */
	struct stat	statbuf;
	struct strioctl	iocb;
	struct ifreq	ifr;
	struct sockaddr_in	src_addr, dst_addr;
	char		mod_name[80];
	char		flagt;
	char		*source, *destination;
	char		*progname;
	void		getaddr();
	char            *ifname = NULL;
	char		service[] = "uucico";
	CALL		call;
	CALL_EXT	call_ext;

	progname = strrchr(argv[0], '/');
	progname = progname ? progname + 1 : argv[0];

	for (;argc > 1 && ((flagt = *argv[1]) == '-' || flagt == '+');
							argc--, argv++) {
		switch (argv[1][1]) {
		case 'c':
			if (flagt == '-') {
				cflg = 0;
			}
			else {
				cflg = 1;
			}
			break;
		case 'e':
			if (flagt == '-') {
				eflg = 0;
			}
			else {
				eflg = 1;
			}
			break;
		case 'f':
			if (flagt == '-') {
				fflg = 0;
			}
			else {
				fflg = 1;
			}
			break;
		case 'i':
			if (flagt == '-') {
				iflg = 0;
			}
			else {
				iflg = 1;
			}
			break;
		case 'm':
			if (--argc <= 1) {
				USAGE();
				exit(1);
			}
			argv++;
			if ((mtu = atoi(argv[1])) < MIN_MTU) {
				fprintf(stderr, "mtu too small, using default. min_mtu = %d\n", MIN_MTU);
				mtu = 0;
			}
			break;
		case 'd':
			if ((flagt == '-') || (flagt == '+')) {
				dflg = 1;
			}
			else {
				dflg = 0;
			}
			break;
		case 'v':
			if (flagt == '-') {
				vflg = 0;
			}
			else {
				vflg = 1;
			}
			break;
		case '?':
			USAGE();
			exit(1);
		}
	}

	switch (argc) {
	default:
		USAGE();
		exit(1);
	case 5:
#if defined(m88k)
		baudrate = getbaud(argv[4]) << 16;
		baud = getbaudrate((baudrate >> 16) & 0xffff);
#else
		baudrate = getbaud(argv[4]);
		baud = getbaudrate(baudrate);
#endif
		if (baudrate == 0) {
			fprintf(stderr, "%s: invalid baud rate (%s)\n",
					progname, argv[4]);
			exit(1);
		}
		/* Fall through */
	case 4:
		getaddr(argv[3], &dst_addr);
		getaddr(argv[2], &src_addr);
		source = argv[2];
		destination = argv[3];
		dev = argv[1];
		break;
	}

	if (dflg) {

		/* fork so we'are not a process group leader */ 
		if (fork()) {
			int status, ret;
			while (1) {
				ret = wait(&status);
				if (ret < 0 && errno != EINTR) {
					perror("wait");
					exit(3);
				}
				if (ret < 0) continue;
				if (WIFEXITED(status))
				  exit(WEXITSTATUS(status));
				if (WIFSIGNALED(status))
				  exit(4);
			}
		}

		sigset(SIGTERM,	un_dial);
		sigset(SIGHUP,	un_dial);
		sigset(SIGQUIT, un_dial);
		sigset(SIGINT, un_dial);

		/* create new session so that the tty we open will be
		 * our control terminal. We do this because we want
		 * to get modem SIGHUP signal. 
		 */	
		if(setsid()<0) {
			perror("setsid");
			exit(1);
		}

tryagain:
		fprintf(stderr, "Dialing uucp system name %s\n",dev);

		call.attr = NULL;               /* termio attributes */
		call.baud = 0;                  /* unused */
		call.speed = baud;              /* any speed if -1 */
		call.line = NULL;
		call.telno = dev;		/* give name, not number */
		call.modem = 0;                 /* modem control not required */
		call.device = (char *)&call_ext;
		call.dev_len = 0;               /* unused */

		call_ext.service = service;
		call_ext.class = NULL;
		call_ext.protocol = NULL;

		if ((dialfd = dial(call)) < 0) {
			fprintf(stderr, "Dial failure using uucp system '%s' = %d",
				 dev,
				 dialfd);

			 if (dialfd == INTRPT)
				 goto tryagain;
			 exit(2);
		}

		stream = dialfd; 
	} else {
		if (strncmp("/dev/", dev, 5)) {
			sprintf(devname, "/dev/%s", dev);
			dev = devname;
		}
		if ((stream = open(dev, O_RDWR)) < 0) {
			perror("tty open");
			exit(2);
		}
	}
	if ((ip = open("/dev/ip", O_RDWR)) < 0) {
		perror("can't open /dev/ip.");
		exit(1);
	}

	/* ditch any modules that were auto pushed onto the tty */
	while (ioctl(stream, I_LOOK, mod_name) >= 0) {
		if (vflg) {
			printf("popping module %s\n", mod_name);
		}
		if (ioctl(stream, I_POP, mod_name) < 0) {
			perror("ioctl I_POP failed");
			exit(1);
		}
	}
	/* push on the slip module */
	if (vflg) {
		printf("pushing module slip\n");
	}
	if (ioctl(stream, I_PUSH, "slip") < 0) {
		perror("ioctl I_PUSH failed");
		exit(1);
	}

	if (mtu) {
		iocb.ic_cmd = S_MTU;
		iocb.ic_timout = 15;
		iocb.ic_len = sizeof(mtu);
		iocb.ic_dp = (caddr_t)&mtu;
		if (ioctl(stream, I_STR, &iocb) < 0) {
			perror("ioctl I_STR (S_MTU) failed");
			exit(1);
		}
	}

	if (cflg) {
		iocb.ic_cmd = S_COMPRESSON;
		iocb.ic_timout = 15;
		iocb.ic_len = 0;
		if (ioctl(stream, I_STR, &iocb) < 0) {
			perror("ioctl I_STR (S_COMPRESSON) failed");
			exit(1);
		}
	}
	if (eflg & !cflg) {
		iocb.ic_cmd = S_COMPRESSAON;
		iocb.ic_timout = 15;
		iocb.ic_len = 0;
		if (ioctl(stream, I_STR, &iocb) < 0) {
			perror("ioctl I_STR (S_COMPRESSAON) failed");
			exit(1);
		}
	}
	if (iflg) {
		iocb.ic_cmd = S_NOICMP;
		iocb.ic_timout = 15;
		iocb.ic_len = 0;
		if (ioctl(stream, I_STR, &iocb) < 0) {
			perror("ioctl I_STR (S_NOICMP) failed");
			exit(1);
		}
	}

	/* set the line speed in the ifstats structure */
	iocb.ic_cmd = S_SETSPEED;
        iocb.ic_timout = 15;
        iocb.ic_len = sizeof(baud);
        iocb.ic_dp = (char *)&baud;
        if (ioctl(stream, I_STR, &iocb) < 0) {
                perror("ioctl I_STR (S_SETSPEED) failed");
                exit(1);
        }

	/* set up line */
	if (ioctl(stream, TCGETA, &term) < 0) {
		perror("TCGETA");
		exit(1);
	}
	term.c_cflag = baudrate | HUPCL | CREAD | CS8 ;

	term.c_iflag = IGNBRK;
	term.c_oflag = term.c_lflag = 0;

	if (ioctl(stream, TCSETS, &term) < 0) {
		perror("TCSETS");
		exit(1);
	}

	/* flush any trash which may be lurking about to trounce on ip */
	

	if (ioctl(stream, I_FLUSH, FLUSHRW) < 0) {
		perror("ioctl I_FLUSH failed");
		exit(1);
	}
	/* bind the stream to the IP sap */
	if (do_bind(stream) < 0)
		exit(1);
	/* link SLIP under IP */
	if ((ifmetric = ioctl(ip, I_LINK, stream)) < 0) {
		perror("can't 'I_LINK' slip under ip");
		exit(1);
	}
	close(stream);

	/* name the interface */
again:	ifname = getifname(ip);
	strcpy(ifr.ifr_name, ifname);
	ifr.ifr_metric = ifmetric;
	ifr.ifr_metric |= (IFF_POINTOPOINT | IFF_RUNNING |
                        IFF_WANTIOCTLS) << 16;
	iocb.ic_cmd = SIOCSIFNAME;
	iocb.ic_timout = 15;
	iocb.ic_len = sizeof(ifr);
	iocb.ic_dp = (caddr_t)&ifr;
	if (ioctl(ip, I_STR, &iocb) < 0) {
		if(try--){
			goto again;
		}
		perror("ioctl I_STR (SIOCSIFNAME) failed");
		exit(1);
	}

	iocb.ic_cmd = SIOCSIFDSTADDR;
	bcopy((caddr_t)&dst_addr, (caddr_t)&ifr.ifr_dstaddr, sizeof(dst_addr));
	if (ioctl(ip, I_STR, &iocb) < 0) {
		perror("ioctl I_STR (SIOCSIFDSTADDR) failed");
		exit(1);
	}

	iocb.ic_timout = 15;
	iocb.ic_len = sizeof(ifr);
	iocb.ic_dp = (caddr_t)&ifr;
	iocb.ic_cmd = SIOCSIFADDR;
	bcopy((caddr_t)&src_addr, (caddr_t)&ifr.ifr_addr, sizeof(src_addr));
	if (ioctl(ip, I_STR, &iocb) < 0) {
		perror("ioctl I_STR (SIOCSIFADDR) failed");
		exit(1);
	}

	if (vflg) {
		printf("%s: src=%s, dest=%s ", progname,
			source, destination);
		if (mtu)
			printf(", mtu=%d", mtu);
		if (iflg)
			printf(", no ICMP");
		if (cflg)
			printf(", compession on");
		else if (eflg)
			printf(", auto compression on");
		else
			printf(", no compression");
		printf(", baudrate=%d.\n", baud);
	}

	/* This is ioctl will return when the line is hungup */
	iocb.ic_cmd = SIOCLOWER;
	iocb.ic_timout = -1;
	iocb.ic_len = sizeof(ifr);
	if (ioctl(ip, I_STR, &iocb) < 0) {
		perror("ioctl I_STR (SIOCLOWER) failed");
		exit(1);
	}

	if (ioctl(ip, I_UNLINK, ifmetric) < 0) {
		perror("I_UNLINK");
	}
	if (vflg) {
		printf("undial called\n");
	}
	undial(dialfd);
	fprintf(stderr, "slattach exiting.\n");
	exit(0);
}

struct sg_spds {
	char            *pstr; 
	int		rate;
}               spds[] = {
#ifdef B50
	{"50", B50},
#endif
#ifdef B75
	{"75", B75},
#endif
#ifdef B110
	{"110", B110},
#endif
#ifdef B134
	{"134", B134},
#endif
#ifdef B150
	{"150", B150},
#endif
#ifdef B200
	{"200", B200},
#endif
#ifdef B300
	{"300", B300},
#endif
#ifdef B600
	{"600", B600},
#endif
#ifdef B1200
	{"1200", B1200},
#endif
#ifdef B1800
	{"1800", B1800},
#endif
#ifdef B2000
	{"2000", B2000},
#endif
#ifdef B2400
	{"2400", B2400},
#endif
#ifdef B4800
	{"4800", B4800},
#endif
#ifdef B9600
	{"9600", B9600},
	{"9.6", B9600},
#endif
#if defined(B19200) || defined(EXTA)
#if !defined(B19200)
#define B19200 EXTA
#endif
	{"19200", B19200},
	{"19.2", B19200},
#endif
#if defined(B38400) || defined(EXTB)
#if !defined(B38400)
#define B38400 EXTB
#endif
	{"38400", B38400},
	{"38.4", B38400},
#endif
#ifdef B57600
	{"57600", B57600},
	{"57.6", B57600},
#endif
#ifdef B115200
	{"115200", B115200},
	{"115.2", B115200},
#endif
	{(char *)NULL, 0}
};

int	getbaud(wantbaud)
	char	*wantbaud;
{
	register struct sg_spds *sp;

	sp = spds;

	while (sp->pstr != (char *)NULL){
		if(strcmp(wantbaud, sp->pstr) == 0){
			return sp->rate;
		}
		sp++;
	}

	return 0;
}

void
getaddr(s, sin)
	char	*s;
	struct sockaddr_in	*sin;
{
	struct hostent	*hp;

	bzero(sin, sizeof(struct sockaddr_in));
	sin->sin_len = sizeof(struct sockaddr_in);

	if (inet_aton(s, &(sin->sin_addr)) != 0) {
		sin->sin_family = AF_INET;
        } else {
                hp = gethostbyname(s);
                if (!hp) {
		  	fprintf(stderr, "%s: bad value\n", s);
                        exit(1);
                }
                memcpy(&(sin->sin_addr), hp->h_addr_list[0], sizeof(sin->sin_addr));
		sin->sin_family = hp->h_addrtype;
        }
	return;
}


char *
getifname(fd)
        int     fd;
{
        struct strioctl ioc;
	struct ifreq *ir = NULL;
        struct ifreq *ifreqp;
        int max_ifnum = -1;
        int ifnum;
        int i;
        int ifname_pref_len;
        char *ifname;
	int total;
	int loopcount = 0;

        ifname_pref_len = strlen(IFNAME_PREF);

	total = MAX_IFACES;
	do {
		if ((ir = (struct ifreq *)realloc(ir, 
				sizeof(struct ifreq) * total)) == NULL) {
			perror("getifname(): realloc");
			exit(-1);
		}

		ioc.ic_cmd = SIOCGIFCONF;
		ioc.ic_timout = 15;
		ioc.ic_len = sizeof(struct ifreq) * total;
		ioc.ic_dp = (void *)ir;
		/*
		 * the ioctl returns the total number of interfaces
		 * ioc.ic_len / sizeof(struct ifreq) is the actual
		 * number of entries returned.
		 */
		if ((total = ioctl(fd, I_STR, (char *)&ioc)) < 0) {
			perror("ioctl SIOCGIFCONF failed");
			exit(-2);
		}
	} while ((ioc.ic_len / sizeof(struct ifreq)) < total && 
		loopcount++ < 3);

        ifreqp = ir;
        for (i = 0; i < ioc.ic_len / sizeof(struct ifreq); i++, ifreqp++) {
                if (ifreqp->ifr_addr.sa_family != AF_INET) {
                        continue;
                }
                if(strncmp(IFNAME_PREF,ifreqp->ifr_name,ifname_pref_len) == 0) {
                        if (isdigit(ifreqp->ifr_name[ifname_pref_len])){
			  ifnum = atoi(&(ifreqp->ifr_name[ifname_pref_len]));
                           max_ifnum = MAX(max_ifnum, ifnum);
                        }
                }
        }

        if ((ifname = (char *)malloc(IFNAMSIZ)) == (char *)NULL) {
                perror("malloc");
                exit(-1);
        }
        sprintf(ifname, "%s%d", IFNAME_PREF, max_ifnum+1);
        free(ir);
        return ifname;
}



getbaudrate(b)
int b;
{
	switch (b) {
#if defined(B50)
	case B50:
		return 50;
#endif
#if defined(B75)
	case B75:
		return 75;
#endif
#if defined(B110)
	case B110:
		return 110;
#endif
#if defined(B134)
	case B134:
		return 134;
#endif
#if defined(B150)
	case B150:
		return B150;
#endif
#if defined(B200)
	case B200:
		return 200;
#endif
#if defined(B300)
	case B300:
		return 300;
#endif
#if defined(B600)
	case B600:
		return 600;
#endif
#if defined(B1200)
	case B1200:
		return 1200;
#endif
#if defined(B2400)
	case B2400:
		return 2400;
#endif
#if defined(B4800)
	case B4800:
		return 4800;
#endif
#if defined(B9600)
	case B9600:
		return 9600;
#endif
#if defined(EXTA)
	case EXTA:
		return 19200;
#else
#if defined(B19200)
	case B19200:
		return 19200;
#endif
#endif
#if defined(EXTB)
	case EXTB:
		return 38400;
#else
#if defined(B38400)
	case B38400:
		return 38400;
#endif
#if defined(B57600)
	case B57600:
		return 57600;
#endif
#if defined(B115200)
	case B115200:
		return 115200;
#endif
#endif
	case B0:
	default:
		return 0xffff;
	}
}

int
do_bind(stream)
int	stream;
{
	struct strbuf	ctlbuf;
	dl_bind_req_t	*bind_reqp;
	dl_bind_ack_t	*bind_ackp;
	dl_error_ack_t	*error_ackp;
	union DL_primitives	dl_prim;
	int	flags;

	bind_reqp = (dl_bind_req_t *)&dl_prim;
	bind_reqp->dl_primitive = DL_BIND_REQ;
	bind_reqp->dl_sap = IP_SAP;
	bind_reqp->dl_max_conind = 0;
	bind_reqp->dl_service_mode = 0;
	bind_reqp->dl_conn_mgmt = 0;
	bind_reqp->dl_xidtest_flg = 0;
	ctlbuf.len = sizeof(dl_bind_req_t);
	ctlbuf.buf = (char *)bind_reqp;
	if (vflg)
		printf("Binding to IP_SAP\n");
	if (putmsg(stream, &ctlbuf, NULL, 0) < 0) {
		perror("putmsg(DL_BIND_REQ) failed");
		return -1;
	}

	ctlbuf.maxlen = sizeof(union DL_primitives);
	ctlbuf.len = 0;
	ctlbuf.buf = (char *)&dl_prim;
	flags = 0;
	if (getmsg(stream, &ctlbuf, NULL, &flags) < 0) {
		perror("getmsg(DL_BIND_REQ) failed");
		return -1;
	}

	switch (dl_prim.dl_primitive) {
	case DL_BIND_ACK:
		if (ctlbuf.len < sizeof(dl_bind_ack_t)) {
			perror("DL_BIND_ACK error");
			return -1;
		}
		break;

	case DL_ERROR_ACK:
		if (ctlbuf.len < sizeof(dl_error_ack_t)) {
			perror("DL_ERROR_ACK/DL_BIND_ACK error");
			return -1;
		}

		error_ackp = (dl_error_ack_t *)&dl_prim;
		switch (error_ackp->dl_errno) {
		case DL_SYSERR:
			perror("DL_SYSERR/DL_BIND_ACK");
			return -1;

		default:
			perror("DL_ERROR_ACK/DL_BIND_ACK");
			return -1;
		}

	default:
		perror("unexpected message type");
		return -1;
	}
	return 0;
}
