#ident	"@(#)in.nbd.c	1.19"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1993 Lachman Technology, Inc.
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
/*
 *
 * MODIFICATION HISTORY
 * WHEN		WHO	ID	WHAT
 * 04 Dec 97	stuarth	S001	ul97-33846 - add code in getendpoint() to set
 *				TCP_NODELAY on TCP endpoints - conditionally
 *				compiled code in #ifdef ... #endif
 *
 */
/*      SCCS IDENTIFICATION        */
/*
 * NetBIOS Daemon
 *
 * The NetBIOS daemon performs two functions:
 *	- initialize NetBIOS
 *	- provide NetBIOS with transport endpoints when needed
 * Several parameters are taken from the environment.  These are:
 *	NB_ADDR		address of this host (dot format)
 *	NB_SCOPE	NetBIOS scope name (dot format, e.g. lachman.com)
 *	NB_DFLTNCB	Default maximum pending commands per user
 *	NB_DFLTSSN	Default maximum sessions per user
 *	NB_MAXNAME	Maximum names per user
 *	NB_MAXNCB	Maximum ncbs per user
 *	NB_MAXSSN	Maximum sessions per user
 *	NB_NAMESEARCH - comma separated name search order - options are
 *		resolv  - call daemon for name resolution
 *		simul 	- simultaneous broadcast + name resolution
 *		bc    	- broadcast
 * 		valid combinations are
 *		resolv
 *		bc
 *		resolv,bc
 *		bc,resolv
 *		simul
 *
 * The daemon extracts relevant network configuration information 
 * and passes the information down to the kernel via an ioctl.
 * After doing the initialization ioctl and linking in the inital set of
 * endpoints, the daemon does a getmsg to wait for a message from the driver.
 * The message is an array of integers.  The first integer indicates whether
 * the driver needs more endpoints (1=needs more, 0=doesn't).  The remaining
 * integers in the array are indexes of endpoints that the driver is done
 * with.  The daemon unlinks these endpoints.
 */

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/tiuser.h>
#include <sys/tihdr.h>
#include <sys/nb/nbuser.h>
#include <sys/nb/nb_const.h>
#include <sys/nb/nb_ioctl.h>
#include <sys/nb/nbtpi.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/sockio.h>
#include <string.h>
#include <ctype.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <net/if.h>
#ifndef	UNIXWARE2_1
#include <net/if_dl.h>
#endif
#include <netinet/in_f.h>
#include <net/route.h>
#include <netinet/in_var.h>
#include <netinet/ip_str.h>
#include <thread.h>
#include <netdb.h>
#include <syslog.h>
#include <locale.h>
#include "nb_msg.h"
#include "in.nbd.h"
nl_catd catd;


/* void *tserver(struct nb_dns *nb_dns); */
void *tserver(void *arg);
void *sigserver(void *arg);
void namethread();
void sigthread();

char *name		= NULL;
struct nb_ifinfo *addrp = NULL;		/* where it starts */
struct nb_ifinfo *vp 	= NULL; 	/* the write pointer */

/* NetBIOS device fd */
int nbfd;


char *buf;			/* generic buffer for ioctls */
int bufsz = 0;

			/* WINS_CLI vvvvv */
char	*namesearch[] = {"wins", "resolv", "bc"};
int	ordertable[] = {
	NB_SO_PASS1_WINS,
	NB_SO_PASS1_RESOLV,
	NB_SO_PASS1_BC,
	NB_SO_PASS2_WINS,
	NB_SO_PASS2_RESOLV,
	NB_SO_PASS2_BC,
	NB_SO_PASS3_WINS,
	NB_SO_PASS3_RESOLV,
	NB_SO_PASS3_BC,
	};
			/* WINS_CLI ^^^^^ */

#ifdef DEBUG_NB

void
print_name(uchar_t *p)
{
	int	i;
	for (i = 0; i < NBNAMSZ - 1 ; i++, p++)
		if (isprint(*p))
			putc(*p, stderr);
		else
			putc('^', stderr);
	
	fprintf(stderr, " x%2.2x", (*p) & 0xff);
}
#endif
/*
 * TCP keep alive idle timer value:
 *      kpalive < 0, don't use keep alives
 *      kpalive == 0, turn on keep alives, use TCP default idle time
 *      kpalive > 0, turn on keep alives, set idle time to kpalive
 *              (after ensuring it is at least TCPTV_MIN_KEEPIDLE)
 */
int kpalive;

/* buffer for TPI primitives */
char pbuf[256];

extern int t_errno;
extern int errno;

extern	int	do_addrs(char *str);


/* errors generic to the process, if quit is set the process exits */

error(msg, quit, errno)
char *msg;
int quit;
int errno;	/* errno has to be prtected from faults in MSGSTR */
{
#ifdef	DEBUG_NB
	fprintf(stderr, "error - quit = %d, errno = %d\n",
			quit, errno);
#endif
	syslog(LOG_ERR, "%s: ", name);
	if (t_errno && t_errno != TSYSERR)
		syslog(LOG_ERR, "%s t_errno:%d\n", msg, t_errno);
	else
		syslog(LOG_ERR, "%s errno:%d\n", msg, errno);
	if (quit){
		unlink(_PATH_PID);
		exit(quit);
	}
}

/* errors specific to the thread , if quit is set the thread exits */

terror(msg, quit, errno)
char *msg;
int quit;
int errno;
{
	syslog(LOG_ERR, "%s: ", name);
	if (t_errno && t_errno != TSYSERR)
		syslog(LOG_ERR, "%s t_errno:%d\n", msg, t_errno);
	else
		syslog(LOG_ERR, "%s errno:%d\n", msg, errno);
	if (quit)
		thr_exit(NULL);
}

/* convert string to lower case */

cvtlow(char *str)
{
	while(*str)
		*str++ = tolower(*str);
}



/*
 * return a pointer to the environment variable pname
 */

char *
getparam(pname)
char *pname;
{
	char *p;
	char *getenv();

	if (!(p = getenv(pname))) {
		syslog(LOG_ERR,MSGSTR(MSG_NBD_NOPARAM,
		   "%s: parameter %s not set\n"), name, pname);
		exit(1);
	}
	return p;
}

/* 
 * return a value from an environment variable,
 * checking limits if required 
 */
int
getparamval(pname, minv, maxv, checkv)
char *pname;
int minv, maxv;
char checkv;
{
	char *p;
	char *getenv();
	int val;

	if (!(p = getenv(pname))) {
		syslog(LOG_ERR, MSGSTR(MSG_NBD_NOPARAM,
		   "%s: parameter %s not set\n"), name, pname);
		exit(1);
	}

	if (sscanf(p, "%ld", &val) != 1) {
		syslog(LOG_ERR, MSGSTR(MSG_NBD_INVPARAM,
		   "%s: Invalid parameter: %s=%s\n"),name,pname,p);
		exit(1);
	}

	if (checkv && (val < minv || val > maxv)) {
		syslog(LOG_ERR, MSGSTR(MSG_NBD_INVPARAM_RANGE,
		   "%s: Invalid parameter: %s=%d not in range %d - %d\n"), 
	   	   name, pname, val, minv, maxv);
		exit(1);
	}

	return val;
}

negotiate(fd, name, level, value)
int fd;
int name;
int level;
int value;
{
	union T_primitives *prim;
	struct opthdr *ohdr;
	struct strbuf sb;
	int len = sizeof(struct opthdr) + OPTLEN(sizeof(int));
	int flags = 0;
	int err;

	sb.maxlen = sizeof(pbuf);
	sb.len = len + sizeof(struct T_optmgmt_req);
	sb.buf = pbuf;

	prim = (union T_primitives *) pbuf;
	prim->type = T_OPTMGMT_REQ;
	prim->optmgmt_req.MGMT_flags = T_NEGOTIATE;
	prim->optmgmt_req.OPT_offset = sizeof(struct T_optmgmt_req);
	prim->optmgmt_req.OPT_length = len;

	ohdr = (struct opthdr *) (pbuf + sizeof(struct T_optmgmt_req));
	ohdr->level = level;
	ohdr->name = name;
	ohdr->len = OPTLEN(sizeof(int));
	*((int *) OPTVAL(ohdr)) = value;

	if (putmsg(fd, &sb, (struct strbuf *) 0, 0) < 0) {
		err=errno;
		error(MSGSTR(MSG_NBD_ERR_PUTM_O,"putmsg for optmgmt failed"),
		   0, err);
		return 0;
	}

	if (getmsg(fd, &sb, (struct strbuf *) 0, &flags) < 0) {
		err=errno;
		error(MSGSTR(MSG_NBD_ERR_GETM_O,"getmsg for optmgmt failed")
		   ,0, err);
		return 0;
	}

	switch (prim->type) {
	case T_OPTMGMT_ACK:
		ohdr = (struct opthdr *) (pbuf + prim->optmgmt_ack.OPT_offset);
		return prim->optmgmt_ack.OPT_length == len
		    && ohdr->level == level
		    && ohdr->name == name
		    && ohdr->len == OPTLEN(sizeof(int))
		    && *((int *) OPTVAL(ohdr)) == value;
	case T_ERROR_ACK:
		syslog(LOG_WARNING, MSGSTR(MSG_NBD_OPTMGT,
		   "%s: optmgmt failed: error (%d)\n"),name,
		   prim->error_ack.UNIX_error);
		return 0;
	default:
		syslog(LOG_WARNING, MSGSTR(MSG_NBD_PRIM,
		   "%s: unexpected primitive (%d) received\n")
		   ,name , prim->type);
		return 0;
	}
}

/*
 * scan the list for repeat broadcast addresses
 * ignore repeats and pass the address list down to the kernel
 *
 * scaning is left until this point to reduce copies.
 */
void
addrs_to_kernel(int naddr)
{
	int addrlen;
	struct nb_ainit *na;
	struct nb_ifinfo *ip, *jp, *wp;
	struct strioctl sioc;
	int errn;
	int i;

	addrlen = sizeof(int) + naddr*(sizeof(struct nb_ifinfo));

	/* reuse existing buffer if possible */

	if(bufsz < addrlen) {
		free(buf, bufsz);
		bufsz = addrlen;
		bufsz = MAX(bufsz, MINBUF);
		if((buf = (char *)malloc(bufsz)) == NULL) {
			perror(name);
			unlink(_PATH_PID);
			exit(1);
		}
	} 
		
	na = (struct nb_ainit *) buf;

	/* ip scans down addr list */
	/* wp is the write pointer into the buffer */
	/* jp scans down the buffered list */

	wp = na->na_if;

	for(i = 0, ip = addrp;i < naddr; i++, ip++) {
		for(jp = na->na_if; jp < wp;  jp++) {
			if(ip->i_baddr == jp->i_baddr)
				break;
		}
		if (jp >= wp) {/* we didn't find it already - copy it */
			memcpy(wp, ip, sizeof(struct nb_ifinfo));
			wp++;
		}
	}

	na->na_naddr = wp - na->na_if;

#ifdef DEBUG_NB
	for(i=0, ip=na->na_if; i<na->na_naddr; i++, ip++) {
		/*
		 * don't concatenate the printfs into one call
		 * as the static storage used by inet_ntoa gets 
		 * overwritten and gives the impression that the 
		 * local and broadcast addresses are identical !
		 */
		fprintf(stderr,"\naddr:%s ", inet_ntoa(ip->i_addr));
		fprintf(stderr," broadcast:%s mtu %d\n",
		
		inet_ntoa(ip->i_baddr),
		ip->i_mtu);
        }
#endif
	/* do init address list ioctl */
	sioc.ic_cmd = NB_IOC_AINIT;
	sioc.ic_timout = -1;
	sioc.ic_len = addrlen;
	sioc.ic_dp = (char *) na;
	if (ioctl(nbfd, I_STR, &sioc) < 0){
		errn = errno;
		error(MSGSTR(MSG_NBD_ERR_AINIT,
		   "Address initialization failed"), 1, errn);
	}
}

getendpoint(dev, port, qlen, bcast, quit, keep)
char *dev;
int port;
int qlen, bcast, quit, keep;
{
	int fd;
	union T_primitives *prim;
#define breq prim->bind_req
#define back prim->bind_ack
#define err prim->error_ack
	struct sockaddr_in *sin;
	struct strbuf sb;
	int flags;
	int errn;

	if ((fd = open(dev, O_RDWR)) < 0) {
		error(dev, quit, errno);
		return;
	}
	prim = (union T_primitives *) pbuf;
	prim->type = T_BIND_REQ;
	sb.buf = pbuf;
	if (port) {
		breq.ADDR_length = sizeof(struct sockaddr_in);
		breq.ADDR_offset = sizeof(struct T_bind_req);
		breq.CONIND_number = qlen;
		sin = (struct sockaddr_in *) (pbuf + sizeof(struct T_bind_req));
		sin->sin_family = AF_INET;
		sin->sin_addr.s_addr = htonl(INADDR_ANY);
		sin->sin_port = port;
		sb.len = sizeof(struct T_bind_req) + sizeof(struct sockaddr_in);
	}
	else {
		breq.ADDR_length = 0;
		breq.ADDR_offset = 0;
		breq.CONIND_number = 0;
		sb.len = sizeof(struct T_bind_req);
	}
	if (putmsg(fd, &sb, (struct strbuf *) 0, 0) < 0) {
		errn = errno;
		error(MSGSTR(MSG_NBD_ERR_PUTM_B,"putmsg for bind failed"),
		   quit, errn);
		close(fd);
		return;
	}
	sb.maxlen = sizeof(pbuf);
	flags = 0;
	if (getmsg(fd, &sb, (struct strbuf *) 0, &flags) < 0) {
		errn = errno;
		error(MSGSTR(MSG_NBD_ERR_GETM_B,"getmsg for bind failed"),
		   quit, errn);
		close(fd);
		return;
	}
	switch (prim->type) {
	case T_BIND_ACK:
		if (port) {
			sin = (struct sockaddr_in *) (pbuf + back.ADDR_offset);
			if (sin->sin_port != port) {
				syslog(LOG_ERR, MSGSTR(MSG_NBD_WRONGADDR,
				   "%s: Bound to wrong address\n") ,name );
				if (quit)
					exit(quit);
				close(fd);
				return;
			}
		}
		break;
	case T_ERROR_ACK:
		t_errno = err.TLI_error;
#ifdef	DEBUG_NB
		fprintf(stderr,"Bind for %s fails t_errno= %d, port %d, broadcast %d, qlen %d\n",
			dev, t_errno, port, bcast, qlen);
#endif
		error(MSGSTR(MSG_NBD_ERR_BIND,"Bind failed"), 
		   quit, err.UNIX_error);
		close(fd);
		return;
	default:
		syslog(LOG_ERR, MSGSTR(MSG_NBD_PRIM,
		   "%s: unexpected primitive (%d) received\n"),name,prim->type);
		if (quit)
			exit(quit);
		close(fd);
		return;
	}

	if (bcast && !negotiate(fd, SO_BROADCAST, SOL_SOCKET, 1)) {
		syslog(LOG_ERR, MSGSTR(MSG_NBD_BCOPT,
		   "%s: Couldn't set broadcast option\n"), name);
		if (quit)
			exit(quit);
		close(fd);
		return;
	}

	if (keep >= 0 && !negotiate(fd, SO_KEEPALIVE, SOL_SOCKET, 1))
		syslog(LOG_ERR, MSGSTR(MSG_NBD_KPAOPT,
		   "%s: Couldn't set keepalive option\n"), name);

	if (keep > 0 && !negotiate(fd, TCP_KEEPIDLE, IPPROTO_TCP, keep))
		syslog(LOG_ERR, MSGSTR(MSG_NBD_KPAIDL,
		   "%s: Couldn't set keepalive idle time\n"),name);
#if	!defined(UNIXWARE2_1)
/* ul97-33846	UnixWare 7 only */
	/*
	 * If the device is TCP, negotiate TCP_NODELAY to so that TCP
	 * does not wait for acks when there's data queued on the stream
	 * waiting to be sent to the client
	 */
	if (!(strcmp(dev, _PATH_TCP)))
	{
	    if (!negotiate(fd, TCP_NODELAY, IPPROTO_TCP, 1))
	    {
#ifdef	SOMETIME	/* we need to get this message in the message
			   catalogues sometime			*/
		syslog(LOG_ERR, MSGSTR(MSG_NBD_NODELAY,
		    "%s: Could not set TCP_NODELAY\n"), name);
#else
		fprintf(stderr, "%s: Could not set TCP_NODELAY\n", name);
#endif
	    }
	}
#endif	/* ul97-33846 & !UNIXWARE2_1 */

	if (ioctl(nbfd, I_LINK, fd) < 0)
		if (errno != ENOSPC || quit){
			errn = errno;
			error(MSGSTR(MSG_NBD_ERR_LINK,"Link failed"), 
			   quit, errn);
		}
	close(fd);
}

linkalot()
{
	int i;

	for (i = 0; i < NENDPOINTS; i++)
		getendpoint(_PATH_TCP, 0, 0, 0, 0, kpalive);
}

/* 
 * reloads the address table 
 */
load_addrs()
{

	char *str;
	int naddr;


#ifdef DEBUG_NB
	fprintf(stderr,"reloading addresses \n");
#endif

	syslog(LOG_NOTICE, MSGSTR(MSG_NBD_ADDR,
	   "%s Reloading interface information\n"),name);


	/* maybe it would be better here to reparse the 
	 * config file although if we do that then maybe we
 	 * ought to be able to reconfigure anything whilst 
	 * netBIOS is up
	 */

	str = getparam("NB_ADDR");
	if((naddr = do_addrs(str)) == 0) {
		syslog(LOG_ERR, MSGSTR(MSG_NBD_INVADDR,
		   "%s:Invalid address: %s\n"), name, str);
		exit(1);
	}

	addrs_to_kernel(naddr);

#ifdef DEBUG_NB
	fprintf(stderr,"done reloading addresses\n");
#endif

}

main(argc, argv)
int argc;
char *argv[];
{
	struct servent	*sp;
	struct nb_init *ni;
	struct strbuf sb;
	int scopelen, flags, i, first, more;
	int *ip, *eip;
	long val;
	char *str;
	struct strioctl sioc;
	int tpifd, xnetfd;
	int naddr;
	int addrlen;
	int errn;
	sigset_t blkset; 
	sigset_t oset;
	FILE *fp;

	int		index = 0;
	char		*temp = NULL;
	char		*string = NULL;
	unsigned long	wins1 = 0;
	unsigned long	wins2 = 0;


        setlocale(LC_ALL,"");
        catd = catopen(MF_NB, MC_FLAGS);

	name = argv[0];

#ifndef	DEBUG_NB
#ifdef	UNIXWARE2_1
	openlog((const char *)basename(name), LOG_PID | LOG_NOWAIT | LOG_CONS, LOG_DAEMON);
#else
	openlog(name, LOG_PID | LOG_NOWAIT | LOG_CONS, LOG_DAEMON);
#endif
#endif	/* DEBUG_NB */


	str = getparam("NB_SCOPE");
	if ((scopelen = strlen(str)) > NB_DOMAIN_MAX) {
		syslog(LOG_ERR, MSGSTR(MSG_NBD_SCOPELEN,
		   "%s: scope name too long\n"), name);
		exit(1);
	}


	/* figure out the addresses to be used before initialising.
	 * - better to stop before initialisation than after it 
	 */

	str = getparam("NB_ADDR");
	if((naddr = do_addrs(str)) == 0) {
		syslog(LOG_ERR, MSGSTR(MSG_NBD_INVADDR,
		   "%s:Invalid address: %s\n"), name, str);
		exit(1);
	}

	bufsz = sizeof(struct nb_init) + scopelen;

	bufsz = MAX(bufsz, MINBUF);

	if((buf = (char *)malloc(bufsz)) == NULL) {
		perror(name);
		exit(1);
	}

	bzero(buf, bufsz);

	ni = (struct nb_init *) buf;

	str = getparam("NB_SCOPE");
	memcpy((char *) (ni + 1), str, scopelen);
	ni->ni_scope_len = scopelen;

	ni->ni_dfltncb = getparamval("NB_DFLTNCB", 0, 255, 1);

	ni->ni_dfltssn = getparamval("NB_DFLTSSN", 0, NB_ABS_MAX_SESSIONS, 1);

	ni->ni_maxname = getparamval("NB_MAXNAME", 0, 255, 1);

	ni->ni_maxncb  = getparamval("NB_MAXNCB", 0, 65535, 1);

	ni->ni_maxssn  = getparamval("NB_MAXSSN", 0, NB_ABS_MAX_SESSIONS, 1);

	kpalive        = getparamval("NB_KPALIVE", 0, 0, 0);

	if (kpalive > 0 && kpalive < TCPTV_MIN_KEEPIDLE) {
		syslog(LOG_ERR, MSGSTR(MSG_NBD_KPATIM_MIN,
		   "%s: Keep alive time less than minimum\n"),name);
		syslog(LOG_ERR, MSGSTR(MSG_NBD_KPAADJ,
		   "\tadjusted up to %d seconds\n"),TCPTV_MIN_KEEPIDLE);
		kpalive = TCPTV_MIN_KEEPIDLE;
	}

	str = getparam("NB_NAMESEARCH");

	cvtlow(str);

	if (strncmp(str,"simul", 5) == 0)
		ni->ni_sorder = NB_SO_PASS1_RESOLV | NB_SO_PASS1_BC;
	else {

				/* WINS_CLI vvvvv */
	    while ((string = strtok_r(str, ",", &temp)) != (char *)NULL)
	    {
		str = (char *) NULL;
#ifdef	DEBUG_NB
		fprintf(stderr, "next fragment of string = %s\n", string);
#endif
		for (i = 0; i < 3; i++)
		{
		    if (strcmp (string, namesearch[i]) == 0)
		    {
			ni->ni_sorder |= ordertable[index + i];
#ifdef	DEBUG_NB
			fprintf(stderr, "sorder = 0x%x, i = %d, index = %d\n",
				ni->ni_sorder, i, index);
#endif
			index += 3;
			break;
		    }
		}
		if (i == 3)
		{
		    error(MSGSTR(MSG_NBD_ERR_SRC, 
		   	"Invalid NetBIOS parameter: NB_NAMESEARCH\n"), 1, EINVAL);
		}
	    }

		/* WINS_CLI  ^^^^^^*/
	}

	if (!ni->ni_sorder)
		error(MSGSTR(MSG_NBD_ERR_SRC, 
		   "Invalid NetBIOS parameter: NB_NAMESEARCH\n"), 1, EINVAL);

				/* WINS_CLI vvvvv */
	str = getparam ("NB_WINS_PRIMARY");
	if ((ni->ni_wins1 = inet_addr(str)) == -1)
	{
		syslog(LOG_ERR, MSGSTR(MSG_NBD_INVADDR,
		   "%s:Invalid address: %s\n"), name, str);
		exit(1);
	}

	str = getparam ("NB_WINS_SECONDARY");
	if ((ni->ni_wins2 = inet_addr(str)) == -1)
	{
		syslog(LOG_ERR, MSGSTR(MSG_NBD_INVADDR,
		   "%s:Invalid address: %s\n"), name, str);
		exit(1);
	}
	if ((ni->ni_wins1 == 0) && (ni->ni_wins2 != 0))
	{
	/*
	 * The user has specified a NULL WINS1 address and a non-NULL
	 * WINS2 address, so use WINS2 as WINS1
	 * stuarth 15 Sep 97
	 */
#ifdef	DEBUG_NB
		fprintf(stderr,"Invalid combination of NB_WINS_PRIMARY zero & NB_WINS_SECONDARY non-zero\nUsing %s as NB_WINS_PRIMARY\n", str);
#endif
		ni->ni_wins1 = ni->ni_wins2;
		ni->ni_wins2 = 0;
	}
	/*
	 * If WINS_PRIMARY and WINS_SECONDARY are same, just use WINS_PRIMARY
	 */
	if (ni->ni_wins1 && (ni->ni_wins1 == ni->ni_wins2))
		ni->ni_wins2 = 0;
#ifdef	DEBUG_NB
	fprintf(stderr,"Final wins1 = 0x%x, wins2 = 0x%x\n", ni->ni_wins1,
		ni->ni_wins2);
	fprintf(stderr,"Search order before manipulation 0x%x\n",
		ni->ni_sorder);
#endif
	/*
	 * If both WINS addresses are 0 AND wins has been specified in
	 * NB_NAMESEARCH, then take it out
	 */
	if (ni->ni_sorder & 
		(NB_SO_PASS1_WINS | NB_SO_PASS2_WINS | NB_SO_PASS3_WINS))
	{
	    if (ni->ni_wins1 == 0)
	    {	
#ifdef	DEBUG_NB
		fprintf(stderr,"wins specified in search order, but no valid NB_WINS_PRIMARY address specified - continuing\n");
#endif
		/*
		 * Mask out the WINS bits from the search order
		 */
		ni->ni_sorder &= ~(NB_SO_PASS1_WINS | NB_SO_PASS2_WINS |
				    NB_SO_PASS3_WINS);
		/* If no search order left, it's an error		*/
		if (!ni->ni_sorder)
			error(MSGSTR(MSG_NBD_ERR_SRC, 
			   "Invalid NetBIOS parameter: NB_NAMESEARCH\n"), 1, EINVAL);
		/* 
		 * If search order pass 1 AND/OR pass 2 masks 0, 
		 * shift search order to pack the masks
		 */
		if ((ni->ni_sorder & NB_SO_PASS2) == 0)
		    ni->ni_sorder = (ni->ni_sorder & NB_SO_PASS1) +
			    ((ni->ni_sorder & NB_SO_PASS3) >> NB_SO_SHIFT);
		if ((ni->ni_sorder & NB_SO_PASS1) == 0)			
		    ni->ni_sorder = ni->ni_sorder >> NB_SO_SHIFT;
	    }
	}
	else		
	{
	/*
	 * MR ul97-32936
	 * WINS NOT specified in search order. If  WINS_PRIMARY non-zero
	 * add WINS to the search order
	 */
	    if (ni->ni_wins1 != 0)
	    {
		ni->ni_sorder = ((ni->ni_sorder << NB_SO_SHIFT) 
					| NB_SO_PASS1_WINS);
	    }
	}
#ifdef	DEBUG_NB
	fprintf(stderr, "Final search order = 0x%x\n", ni->ni_sorder);
#endif

		/* WINS_CLI  ^^^^^^*/

	/* this one for endpoint requests */

	if ((nbfd = open(_PATH_NBD, O_RDWR)) < 0){
		errn = errno;
		error(MSGSTR(MSG_NBD_ERR_DOPEN, 
		   "Can't open NetBIOS daemon device"), 1, errn);
	}

	/* do init ioctl */
	sioc.ic_cmd = NB_IOC_INIT;
	sioc.ic_timout = -1;
	sioc.ic_len = sizeof(struct nb_init) + scopelen;
	sioc.ic_dp = (char *) ni;
	if (ioctl(nbfd, I_STR, &sioc) < 0){
		errn = errno;
		error(MSGSTR(MSG_NBD_ERR_INIT,"Initialization failed"), 
		   1, errn);
	}

	addrs_to_kernel(naddr);

	/* link in initial endpoints */
	sp = getservbyname("nb-ns", "udp");
	if (sp == NULL) {
		errn = errno;
		error(MSGSTR(MSG_NBD_ERR_NS, "can't find nb-ns udp service"),
		   1, errn);
	}
	getendpoint(_PATH_UDP, sp->s_port, 0, 1, 1, -1);

	sp = getservbyname("nb-dgm", "udp");
	if (sp == NULL) {
		errn = errno;
		error(MSGSTR(MSG_NBD_ERR_DG,"can't find nb-dgm udp service")
		   ,1, errn);
	}
	getendpoint(_PATH_UDP, sp->s_port, 0, 1, 1, -1);

	sp = getservbyname("nb-ssn", "tcp");
	if (sp == NULL){
		errn = errno;
		error(MSGSTR(MSG_NBD_ERR_SN,"can't find nb-ssn tcp service"),
		   1, errn);
	}
	getendpoint(_PATH_TCP, sp->s_port, 4, 0, 1, -1);

	/* link in some tcp endpoints to use for sessions */
	linkalot();

	/* open dev for TPI */
	if ((tpifd = open(_PATH_NB, O_RDWR)) < 0){
		errn = errno;
		error(MSGSTR(MSG_NBD_ERR_OPEN, "Can't open NetBIOS device"),
		   1, errn);
	}

	sioc.ic_cmd = NB_IOC_TPI;
	sioc.ic_timout = -1;
	sioc.ic_len = 0;
	sioc.ic_dp = (char *) 0;
	if (ioctl(tpifd, I_STR, &sioc) < 0){
		errn = errno;
		error(MSGSTR(MSG_NBD_ERR_TPI, "TPI initialization failed"),
		   1, errn);
	}

	/*
	 * Cool so far - go into background & wait for messages
	 * from driver.
	 */
	if (fork())
		exit();

	setpgrp();
	close(0);
	close(1);

	/* dump the pid somewhere so others can find it */

	fp = fopen(_PATH_PID, "w");
	fprintf(fp, "%d", getpid());
	fclose(fp);

	/* block all signals in the main thread, all 
	 * spawned threads will inheirit this signal mask
	 *
	 * signals are handled by a signal handling thread 
	 */

	sigfillset(&blkset);
	sigdelset(&blkset, SIGSEGV);
	thr_sigsetmask(SIG_BLOCK, &blkset, &oset);
	sigthread(buf);

	/* main thread waits for messages from driver */
	sb.maxlen = bufsz;
	sb.buf = buf;
	first = 1;
	for (;;) {
		flags = 0;
		/* if getmsg fails, something is majorly hosed */
		if ((i = getmsg(nbfd, (struct strbuf *) 0, &sb, &flags)) < 0){
			error("getmsg", 1, errno);
		}
		if (sb.len <= 0) {
			/*
			 * This means either there is a non-data message
			 * on the queue, there is a zero-length data message
			 * on the queue, or a hangup was received.
			 * None of these are tolerable.
			 */
			syslog(LOG_ERR, MSGSTR(MSG_NBD_GETMSG,
			   "%s: getmsg returned length=%d\n"),name,sb.len);
			syslog(LOG_ERR, MSGSTR(MSG_NBD_EXIT,
			   "%s: exiting\n"), name);
			exit(1);
		}

		
		ip = (int *) buf;
		eip = ip + (sb.len / sizeof(int));
		/* see if driver wants more endpoints */
		if (first)
			more = *ip++;
		else
			more = 0;

		if (more == NB_CMD_DNS) /* its a DNS request */
			namethread(buf);
		else {			/* its a tpe request */
			/* 
			 * first refers to whether there is more 
			 * data in a follow up message
			 * 
			 */
			first = (i != MOREDATA);
			/* do unlinks */
			for (; ip < eip; ip++) {
				if (ioctl(nbfd, I_UNLINK, *ip) < 0){
					errn = errno;
					error(MSGSTR(MSG_NBD_ERR_UNLINK,
					   "unlink failed"), 0, errn);
				}
			}
			if (more)
				linkalot();
		}
	}
}

/* 
 * namethread spawns a thread which will wait on upcoming 
 * name resolution requests. Since multiple requests may 
 * be being processed simultaneously, when a request is received
 * another thread is spawned which will then wait on the 
 * stream. 
 *
 */
void
namethread(char *buf)
{
	int errn;
	struct nb_dns *nb_dns;

	/* Be careful with sb, its used repeatedly in the
	 * main loop - copy data before passing it to the 
	 * new thread. 
 	 *
	 * The nameserver thread must free the memory 
	 * before it exits.
	 */

	nb_dns = (struct nb_dns *)malloc(sizeof(struct nb_dns));
	if(nb_dns == NULL)
		return;

	bcopy(buf, (char *)nb_dns, sizeof(struct nb_dns));

	errn = thr_create((void *)NULL, (size_t)0, tserver, (void *)nb_dns, 
		  THR_BOUND | THR_DETACHED, NULL);

	if(errn)
		error("thr_create", 0, errn);
}

/* 
 * sigthread spawns a thread which will deal with signals on 
 * behalf of the daemon.
 */

void
sigthread()
{
	int errn;

	errn = thr_create((void *)NULL, (size_t)0, sigserver, (void *)NULL, 
		  THR_BOUND | THR_DETACHED, NULL);

	if(errn)
		error("thr_create", 0, errn);

}

void *
sigserver(void *arg)
{
	sigset_t waitset; 
	sigset_t oset;
	int sig;

	sigemptyset(&waitset);			/* all signals */
	sigaddset(&waitset, SIGUSR1);	
	sigaddset(&waitset, SIGTERM);	
	thr_sigsetmask(SIG_UNBLOCK, &waitset, &oset);

	while(1){
		sig = sigwait(&waitset);
		switch(sig) {
			case SIGUSR1:		/* reload the address table */
				load_addrs();
				break;
			case SIGTERM:
				syslog(LOG_WARNING, MSGSTR(MSG_NBD_EXITSIG,
				   "%s exiting -- SIGTERM\n"),name);
				unlink(_PATH_PID);
				exit(SIGTERM);
				break;
			default:
				fprintf(stderr,"caught signal %d\n", sig);
				break;
		}
	}
}

int
inrange(char c)
{
	if(isupper(c))
		return 1;
	if(isdigit(c))
		return 1;
	if(c == '-')
		return 1;

	return 0;
}

/* 
 * nbtodns
 * Converts a netbios name pointed to by p, to a DNS encoded name
 * at wp. The calling routine is responsible for memory allocation
 * for the dns name. 
 *
 * Return values
 *  0 = OK
 *  1 = error
 */

#define DNS_NAMELEN  (NB_ENAMESIZE + 1)

int 
nbtodns(char *p, char *wp)
{
	int i,c;
	char *pp = p;
	char tflag = 0;
	char *fb = NULL;
	int len = NB_NAMELEN;
	int cnt;

	/* figure out if it needs converting */

	if(!isupper(*p))
		tflag++;
	else{
		for(pp = p, cnt = 0;cnt < NB_NAMELEN; cnt++, pp++){
			if(!inrange(*pp)){
				if ((*pp == NULL) || (*pp == ' ')){
					if (fb == NULL)
						fb = pp;
				}
				else {
					tflag++;
					break;
				}
			}
			else if(fb != NULL){	/* valid character - after  */
				tflag++;	/* a space or blank */
				break;	
			}
		}
	}

	if(!tflag){
		if(fb != NULL)
			len = fb - p;
		
		bcopy(p, wp, len);
		*(wp+len) = 0;
		return 0;
	}

	/* encoding is according to the RFC */

        for (i = 0; i < NB_NAMESIZE; i++) {
                c = *p++;
                *wp++ = ((c >> 4) & 0xf) + 'A';
                *wp++ = (c & 0xf) + 'A';
        }
	*wp = 0;

	return 0;
}

/*
 * tserver - the start function for a new thread 
 */
void *
tserver(void *arg)
{
	struct strbuf sb;
	int i, flags;
	char dnsname[DNS_NAMELEN];
	struct hostent *host = NULL;
	struct in_addr addr;
	struct strioctl strioc;
	struct nb_dns *nb_dns = (struct nb_dns *)arg;


	char	nbname[NB_NAMELEN + 1];
	/*
	 * Try the text name up to the first NULL or space character
	 */
	bcopy(nb_dns->d_name, nbname, NB_NAMELEN);
	nbname[NB_NAMELEN] = '\0';
	for (i = 0; i < NB_NAMELEN; i++)
	{
	    if (nbname[i] == '\0' || nbname[i] == ' ')
	    {
		nbname[i] = '\0';
		break;
	    }
	}
#ifdef DEBUG_NB
	fprintf(stderr, "netbios name %s\n", nbname);
#endif

	host = gethostbyname(nbname);

	if (host == NULL) 
	{
	/* 
	 * Now try the encoded form of the name (depends on how nbtodns()
	 * returns the name - may be RFC encoded)
	 */

	nbtodns(nb_dns->d_name, dnsname);

#ifdef DEBUG_NB
	fprintf(stderr, "netbios name ");
	print_name((uchar_t *) nb_dns->d_name);
	fprintf(stderr, " -> dns name %s\n", dnsname);
#endif

	host = gethostbyname(dnsname);

	}

	if ( (host == NULL) || 
	   (host->h_length != sizeof(struct in_addr)) ||
	   (host->h_addrtype != AF_INET)) {
		nb_dns->d_len	= 0;
		nb_dns->d_addr	= 0;
	}
	else {
		nb_dns->d_len = sizeof(struct in_addr);
		nb_dns->d_addr= *(ulong*)*host->h_addr_list;
	}

	strioc.ic_cmd	= NB_IOC_DNS;
	strioc.ic_timout= -1;		/* infinite */
	strioc.ic_len	= sizeof(struct nb_dns);
	strioc.ic_dp	= (char *)nb_dns;

	if(ioctl(nbfd, I_STR, &strioc) == -1)
		syslog(LOG_ERR, MSGSTR(MSG_NBD_ERR_IOC,
		   "%s: ioctl for DNS failed\n"), name);

cleanup:
	free(nb_dns);
	thr_exit(NULL);
}
