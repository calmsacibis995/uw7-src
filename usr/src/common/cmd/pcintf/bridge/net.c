#ident	"@(#)pcintf:bridge/net.c	1.1.1.3"
#include	"sccs.h"
SCCSID(@(#)net.c	6.7	LCC);	/* Modified: 15:47:51 1/6/92 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

/*
 * History:
 *	[10/25/88 efh] Eugene Hu - added fix to myhostent from 2.8.6 which
 *		checks for NULL return by gethostbyname().
 */

#include "sysconfig.h"

#if defined(BSD43)
#	include <sys/ioctl.h>
#	if defined(SYS5_4)
#		include <sys/sockio.h>
#	endif
#endif		/* BSD43 */

#ifdef		MICOM
#include	<interlan/il_errno.h>
#endif

#if		defined(ISC386)
#include	<sys/sioctl.h>
#include	<net/errno.h>
#undef ENAMETOOLONG
#undef ENOTEMPTY
#endif		/* ISC386 */

#include	<fcntl.h>

#ifdef SYS19
#include 	<h/errno42.h>
#endif
#ifdef EXL316CMC
#include	<net/errno.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <memory.h>

#ifdef	RS232PCI
#include	<termio.h>
#endif	  /* RS232PCI  */

#ifdef STREAMNET
#include <stropts.h>
#endif

#include <lmf.h>

#include "pci_proto.h"
#include "pci_types.h"
#include "common.h"
#include "flip.h"
#include "log.h"

#define	r232dbg(X)	debug(DBG_RS232,X)

/*			External Variables			*/

static int
	theNetDesc = -1,	/* Current network file descriptor */
	savedNetDesc = -1;	/* Saved network descriptor */

#ifdef	TLI
extern int t_errno;		/* System errno var for TLI calls */
#endif	/* TLI */

#ifdef	MICOM
static 	int 	micomPort;
#endif


#define	MAXRRETRY	3	/* Maximum receive retries */
#define	MAXXRETRY	3	/* Maximum xmit retries */
#define MAXORETRY	5	/* Maximum open retries */

#if defined(UDP42)
void			hostaddr	PROTO((struct sockaddr_in *));
void			localhostaddr	PROTO((struct sockaddr_in *));
struct hostent		*myhostent	PROTO((void));
LOCAL struct hostent	*localhostent	PROTO((void));

#	if !defined(TLI)
extern int		socket		PROTO((int, int, int));
extern int		bind		PROTO((int, struct sockaddr *, int));
extern int		sendto		PROTO((int, char *, int, int,
						struct sockaddr *, int));
extern int		recvfrom	PROTO((int, char *, int, int,
						struct sockaddr *, int *));

#		if defined(BSD43)
extern int		setsockopt	PROTO((int, int, int, char *, int));
#		endif	/* BSD43 */
#	endif	/* !TLI */
#endif	/* UDP42 */

#if defined(RS232PCI)
LOCAL void	nak			PROTO((void));

#	if defined(RS232_PKTDBG)
LOCAL void	rs232_log_packet	PROTO((char *, unsigned char *, int));
#	endif /* RS232_PKTDBG */

#	if defined(TIMEOUT)
extern void	timeout_on	PROTO((int, struct termio *));
extern void	timeout_off	PROTO((int, struct termio *));
#	endif /* TIMEOUT */
#endif /* RS232PCI */

#if defined(TLI)
extern char	*t_alloc	PROTO((int, int, int));
extern int	t_bind		PROTO((int, struct t_bind *, struct t_bind *));
extern int	t_close		PROTO((int));
extern int	t_free		PROTO((void *, int));
extern int	t_open		PROTO((char *, int, struct t_info *));
extern int	t_optmgmt	PROTO((int, struct t_optmgmt *, struct t_optmgmt *));
extern int	t_rcvudata	PROTO((int, struct t_unitdata *, int *));
extern int	t_rcvuderr	PROTO((int, struct t_uderr *));
extern int	t_sndudata	PROTO((int, struct t_unitdata *));
extern int	t_sync		PROTO((int));
#endif


/*
   netSave: Tell network driver what descriptor we're talking on
*/

int				/* Returns current network file descriptor */
netSave()
{
	return savedNetDesc = theNetDesc;
}


/*
   netUse: Tell network driver what descriptor to talk on
*/

int				/* Returns new network file descriptor */
netUse(newNetDesc)
int
	newNetDesc;		/* New network file descriptor */
{
#ifdef	TLI
	if (newNetDesc < 0) {
		log("netUse: Invalid net descriptor\n");
		exit(1);
	}
	if (newNetDesc != theNetDesc && newNetDesc != savedNetDesc) {
		if (t_sync(newNetDesc) < 0) {
			log("netUse: t_sync failed t_errno %d errno %d\n",
				t_errno, errno);
			exit(1);
		}
	}
#endif	/* TLI */

	return theNetDesc = newNetDesc;
}



/* Start of network dependent code */


#ifdef	UDP42

#ifndef TLI

/*
   netOpen:  Get access to a network port
*/

int				/* Returns file descriptor of network port */
				/* < 0 signals error */
netOpen(netDev, portNum)
char
	*netDev;		/* Name of network interface device */
int
	portNum;		/* The port to assign */
{
int
	retryCount,		/* Used for opens and binds */
	netDesc;		/* Network file descriptor */
struct sockaddr_in
	vPort;			/* Network driver virtual port structure */
int 	one = 1;


#ifdef MICOM
	micomPort = portNum;
#endif

	for (retryCount = 0; retryCount < MAXORETRY;) {
		if ((netDesc = socket(AF_INET, SOCK_DGRAM, 0)) != -1)
			break; 
		switch(errno) {
		case ENOBUFS:
			(void) sleep(10);
			break;
		case EISCONN:
		case ENOTCONN:
		case EADDRINUSE:
		case EADDRNOTAVAIL:
		default:
			log("netOpen: socket failed: %d\n",errno);
			return -1;
		}
		retryCount++;
	}
	vPort.sin_family = AF_INET;
	vPort.sin_addr.s_addr = INADDR_ANY;
	vPort.sin_port = htons(portNum);
	if (bind(netDesc, &vPort, sizeof vPort) < 0) {
		log("netOpen: can't bind socket %d\n", errno);
		return -1;
	}

#ifdef BSD43
/* This is for 4.3BSD Unix systems which must have socket broadcast permissions
   explicitly set with a setsockopt() system call */

	if (setsockopt(netDesc,SOL_SOCKET,SO_BROADCAST,
			&one,sizeof(one)) < 0)
		log("SETSOCKOPT failed; errno: %d.\n",errno);

#endif /* BSD43 */

	return theNetDesc = netDesc;
}

/*
   rcvPacket():	Read packet from ethernet device into buffer;
		Swap bytes if necessary.  Retry on transient
		errors.
*/

int
rcvPacket(inPkt)
register struct input
	*inPkt;			/* Pointer to input buffer */
{
int
	nRead,				/* Return from read()	*/
	retryCount,			/* Number of times retried */
	fromLength,
	flipCode;			/* Byte ordering code */

	for (retryCount = 0; retryCount < MAXRRETRY;) {
		fromLength = SZNADDR;
		if ((nRead = recvfrom(theNetDesc, &inPkt->pre, MAX_FRAME,
			0, inPkt->net.src, &fromLength)) > 0) {
			flipCode = input_swap(inPkt, inPkt->hdr.pattern);
			logPacket(inPkt, PLOG_RCV, LOG_SOME);
			return flipCode;
		}

		switch(errno) {
		case EINTR:		/* Retry forever */
			continue;

#ifdef MICOM
		/* apparent bug in MICOM TCP	*/
		case ENOTCONN:	close(theNetDesc);
				netOpen("", micomPort);
				continue;
#endif
		case ENOTSOCK:
		case EBADF:
		case EFAULT:
		case EWOULDBLOCK:
		default:
			log("rcvPacket: UDP Read err: %d\n", errno);
		}

		retryCount++;
	}
	fatal(lmf_get_message("NET1",
		"rcvPacket: UDP Retry limit exceeded\n"));
}


struct ni2
	lastXmtHeader;		/* Remember address for reXmt() */

/*
   xmtPacket():	Sends a packet onto the ethernet. Returns the length
  		of the packet sent, or 0 on error.
*/

int
xmtPacket(outPacket, niHeader, flipCode)
register struct output
	*outPacket;		/* Outgoing packet */
struct ni2
	*niHeader;		/* Ni driver header */
register int
	flipCode;		/* Flipping sense pattern */
{
register int
	pktLength,		/* Packet length */
	pktSent,		/* Number of bytes successfully transmitted */
	retryCount;		/* Number of times retried */

	pktLength = HEADER + outPacket->hdr.t_cnt;

	/* If the length is invalid send an error messsge */
	if (pktLength > MAX_FRAME) {
		log("xmtPacket: Packet too big: %d\n", pktLength);
		outPacket->hdr.res = INTERNAL_ERRORS;
		pktLength = HEADER;
	}

	logPacket((struct input *)outPacket, PLOG_XMT, LOG_SOME);
	output_swap(outPacket, flipCode);
	lastXmtHeader = *niHeader;

	/* Write the packet onto the ethernet device */
	for (retryCount = 0; retryCount < MAXXRETRY;) {
		pktSent = sendto(theNetDesc, &outPacket->pre, pktLength, 0, 
				niHeader->dst, SZNADDR);

		if (pktSent == pktLength)
			return pktLength;

		switch(errno) {
		case EINTR:	/* Retry forever */
			continue;

		case ENOTSOCK:
		case EBADF:
		case EFAULT:
		case EWOULDBLOCK:
		default:
			log("xmtPacket: UDP Write err: %d\n", errno);
			return 0;
		}

		retryCount++;
	}
}


/*****************************************************************************
 *  reXmt: retransmit last packet
 */

reXmt(rePacket, reLength)
struct output
	*rePacket;
int
	reLength;
{
unsigned
	retryCount,
	reSent;

	log("reXmt: seq %d\n", rePacket->hdr.seq);

	/* Send the packet */
	for (retryCount = 0; retryCount < MAXXRETRY;) {
		reSent = sendto(theNetDesc, &rePacket->pre, reLength, 0, 
				lastXmtHeader.dst, SZNADDR);

		if (reSent == reLength)
			return reLength;

		switch(errno) {
		case EINTR:	/* Retry forever */
			continue;

		case ENOTSOCK:
		case EBADF:
		case EFAULT:
		case EWOULDBLOCK:
		default:
			log("reXmt: UDP Write err: %d\n", errno);
			return 0;
		}

		retryCount++;
	}
}

#else	/* TLI */

/*
   netOpen:  Get access to a network port
*/

int				/* Returns file descriptor of network port */
				/* < 0 signals error */
netOpen(netDev, portNum)
char
	*netDev;		/* Name of network interface device */
int
	portNum;		/* The port to assign */
{
int
	retryCount,		/* Used for opens and binds */
	netDesc;		/* Network file descriptor */
struct t_bind 
	*req, *ret;		/* Bind request and return addresses */
struct t_bind 
	retstruct, reqstruct;
struct t_info 
	tinfo;			/* Returns default parms of transport protcol */
struct sockaddr_in
	vPort;			/* Network driver virtual port structure */
char    pad[10000];
int 	one = 1;
int	memret;

#ifdef BROADCAST_OPTION
#ifdef BELLTLI
struct t_optmgmt *topt;
struct opthdr *oh;
#else
struct  t_optmgmt reqopt;
struct  t_optmgmt retopt;
static char    optbuf[10000];
struct inetopt netopt;
#endif
#endif

	for (retryCount = 0; retryCount < MAXORETRY;) {
		if ((netDesc = t_open(netDev, O_RDWR, &tinfo)) >= 0)
			break; 
		else if ((netDesc = t_open("/dev/inet/udp", O_RDWR, &tinfo)) >= 0)
			break;
		log("netOpen: open failed: errno %d\n", errno);
		if (t_errno == TSYSERR)
		switch(errno) {
		case ENOBUFS:
			(void) sleep(10);
			break;
		case EISCONN:
		case ENOTCONN:
		case EADDRINUSE:
		case EADDRNOTAVAIL:
		default:
			log("netOpen: open failed: errno %d t_errno %d\n",
					errno, t_errno);
			return -1;
		}
		retryCount++;
	}

	if (tinfo.tsdu > 0 &&  tinfo.tsdu < MAX_FRAME)  {
		log("netOpen: open returned max data < MAX_FRAME\n");
		fatal(lmf_get_message("NET2",
			"netOpen: open: max data < MAX_FRAME -- PCI Aborting"));
	}

	memset((char *)&vPort, '\0', sizeof(struct sockaddr_in));
	vPort.sin_family = AF_INET;
	vPort.sin_port = htons(portNum);

	/* Allocate bind structures */
	req = &reqstruct;
	ret = &retstruct;

	req->addr.len = SZNADDR;
	req->addr.buf = (char *)&vPort;
	/* memcpy(req->addr.buf, (char *)&vPort, SZNADDR); */
	req->qlen = 0;

	ret->addr.maxlen = SZNADDR;
	ret->addr.buf = (char *)&vPort;

	if (t_bind(netDesc, req, ret) < 0) { 
		log("netOpen: can't bind endpoint: t_errno %d errno %d\n",
				t_errno, errno);
	        t_close(netDesc);
		return -1;
	}

	vPort = *(struct sockaddr_in *)ret->addr.buf;
	memret = memcmp( ret->addr.buf, req->addr.buf, sizeof(struct sockaddr_in));
	if ( memret ) {
	        t_close(netDesc);
		log("netOpen: bind of wrong address\n");
		return -1;
	}

#ifdef BROADCAST_OPTION
#ifdef BELLTLI
	if ((topt = (struct t_optmgmt *) t_alloc(netDesc, T_OPTMGMT, T_ALL)) == NULL) {
		log("netOpen: t_alloc() failed: t_errno=%d errno=%d\n",
			t_errno, errno);
		return(-1);
	}
	oh = (struct opthdr *)topt->opt.buf;
	oh->level = SOL_SOCKET;
	oh->name = SO_BROADCAST;
	oh->len = OPTLEN(sizeof(int));
	*((int *)OPTVAL(oh)) = 1;
	topt->opt.len = sizeof(struct opthdr) + OPTLEN(sizeof(int));
	topt->flags = T_NEGOTIATE;
	if (t_optmgmt(netDesc, topt, topt) < 0) {
		log("netOpen: t_optmgmt() failed: t_errno %d errno %d\n",
			t_errno, errno);
		t_close(netDesc);
		t_free(topt, T_OPTMGMT);
		return(-1);
	}
	t_free(topt, T_OPTMGMT);
#else
	/* Option management for debugging */
	netopt.len = (short)(IOPT_LEADIN_LEN + sizeof(long));
	netopt.name = (ushort)SO_BROADCAST;
	netopt.level = (ushort)SOL_SOCKET;
	netopt.value.ival = 1;

	reqopt.opt.len = sizeof(struct inetopt);
	reqopt.opt.buf = (char *)&netopt;
	reqopt.flags = (long)T_NEGOTIATE;

	retopt.opt.maxlen = 10000;
	retopt.opt.buf = &optbuf[0];
	if (t_optmgmt(netDesc, &reqopt, &retopt) < 0) {
		log("netOpen: t_optmgmt() failed: t_errno %d errno %d\n",
			t_errno, errno);
		return -1;
	}
#endif
#endif

	return theNetDesc = netDesc;
}


/*
   rcvPacket():	Read packet from ethernet device into buffer;
		Swap bytes if necessary.  Retry on transient
		errors.
*/

int
rcvPacket(inPkt)
register struct input
	*inPkt;			/* Pointer to input buffer */
{
int
	nRead,				/* Return from read()	*/
	retryCount,			/* Number of times retried */
	fromLength,
	flipCode;			/* Byte ordering code */

	int    rcvFlags;
	struct t_unitdata *ud;		/* datagram unit */
	struct t_uderr *uderr;		/* datagram error unit */
	struct t_unitdata udstruct;
	struct t_uderr uderrstruct;

	ud = &udstruct;
	uderr = &uderrstruct;

	rcvFlags = 0;
	ud->opt.maxlen = 0;
	ud->addr.maxlen = (unsigned int)SZNADDR;
	ud->udata.maxlen = (unsigned int)MAX_FRAME;
	ud->addr.buf = (char *)&inPkt->net.src_sin;
	ud->udata.buf = (char *)&inPkt->pre;

	for (retryCount = 0; retryCount < MAXRRETRY;) {
		if ((t_rcvudata(theNetDesc, ud, &rcvFlags)) < 0) {

			if (t_errno == TSYSERR && errno == EINTR)
				continue;

			log("rcvPacket: t_rcvudata() failed: t_errno = %d, errno = %d\n", 
				t_errno, errno);
			if (t_errno == TLOOK) {
			    /* Error on previous datagram */
			    log("rcvPacket: error on previous datagram sent\n");
			    if (t_rcvuderr(theNetDesc, uderr) < 0) {
				log("rcvPacket: error on t_rcvuderr()\n");
			    }

			    log("rcvPacket: bad datagram: ud_error = %d\n", 
				uderr->error);
			    continue;

			}

			retryCount++;
			continue;
		}

		flipCode = input_swap(inPkt, inPkt->hdr.pattern);
		logPacket(inPkt, PLOG_RCV, LOG_SOME);
		return flipCode;

	}
	fatal(lmf_get_message("NET3",
		"rcvPacket: Retry limit exceeded\n"));
}


struct ni2
	lastXmtHeader;		/* Remember address for reXmt() */

/*
   xmtPacket():	Sends a packet onto the ethernet. Returns the length
  		of the packet sent, or 0 on error.
*/

int
xmtPacket(outPacket, niHeader, flipCode)
register struct output
	*outPacket;		/* Outgoing packet */
struct ni2
	*niHeader;		/* Ni driver header */
register int
	flipCode;		/* Flipping sense pattern */
{
register int
	pktLength,		/* Packet length */
	pktSent,		/* Number of bytes successfully transmitted */
	retryCount;		/* Number of times retried */

	struct t_unitdata *ud;		/* datagram unit */

	if ((ud = (struct t_unitdata *)t_alloc(theNetDesc, T_UNITDATA, T_OPT))
	    == (struct t_unitdata *)NULL) {
		log("xmtPacket: theNetDesc = %d\n", theNetDesc);
		log("xmtPacket: alloc of unit-data struct failed: t_error %d\n",
				t_errno);
		return 0;
	}

	pktLength = HEADER + outPacket->hdr.t_cnt;

	/* If the length is invalid send an error messsge */
	if (pktLength > MAX_FRAME) {
		log("xmtPacket: Packet too big: %d\n", pktLength);
		outPacket->hdr.res = INTERNAL_ERRORS;
		pktLength = HEADER;
	}

	logPacket((struct input *)outPacket, PLOG_XMT, LOG_SOME);
	output_swap(outPacket, flipCode);
	lastXmtHeader = *niHeader;

	/* Assign the dst sockaddr struct to the addr.buf for UDP */
	ud->addr.len = SZNADDR;
	ud->addr.buf = (char *)&niHeader->dst_sin;

	/* No special options */
	ud->opt.len = 0;

	/* Packet goes in udata */
	ud->udata.len = (unsigned int)pktLength;
	ud->udata.buf = (char *)&outPacket->pre;

	/* Write the packet onto the ethernet device */
	for (retryCount = 0; retryCount < MAXXRETRY;) {
		if ((t_sndudata(theNetDesc, ud)) < 0) {
			log("xmtPacket: t_sndudata() failed: t_errno = %d\n", 
				t_errno);

			if (t_errno == TSYSERR) {
			log("xmtPacket: t_sndudata() failed: errno = %d\n", 
				errno);
			switch(errno) {
			case EINTR:		/* Retry forever */
				log("xmtPacket: Interrupted system call\n");
				continue;
			}
			}


			log("xmtPacket: send of datagram failed\n");
			switch(t_errno) {
			    case TFLOW:	/* Retry forever */
				continue;
			    default:
				log("xmtPacket: UDP Write err: %d\n", t_errno);
				ud->addr.buf = NULL;
				ud->udata.buf = NULL;
				(void)t_free((char *)ud, T_UNITDATA);
				return 0;
			}
		}
		else {
			ud->addr.buf = NULL;
			ud->udata.buf = NULL;
			(void)t_free((char *)ud, T_UNITDATA);
			return pktLength;
		}
	}
}


/*****************************************************************************
 *  reXmt: retransmit last packet
 */

reXmt(rePacket, reLength)
struct output
	*rePacket;
int
	reLength;
{
unsigned
	retryCount,
	reSent;

	struct t_unitdata *ud;		/* datagram unit */

	if ((ud = (struct t_unitdata *)t_alloc(theNetDesc, T_UNITDATA, T_OPT))
	    == (struct t_unitdata *)NULL) {
		log("reXmtPacket: alloc of unit-data structure failed\n");
		return 0;
	}

	/* Assign the dst sockaddr struct to the addr.buf for UDP */
	ud->addr.len = SZNADDR;
	ud->addr.buf = (char *)&lastXmtHeader.dst_sin;

	/* No special options */
	ud->opt.len = 0;

	/* Packet goes in udata */
	ud->udata.len = (unsigned int)reLength;
	ud->udata.buf = (char *)&rePacket->pre;

	log("reXmt: seq %d\n", rePacket->hdr.seq);

	/* Send the packet */
	for (retryCount = 0; retryCount < MAXXRETRY;) {
		if ((t_sndudata(theNetDesc, ud)) < 0) {
			if (t_errno == TSYSERR) {
			log("reXmtPacket: t_sndudata() failed: errno = %d\n", 
				errno);
			switch(errno) {
			case EINTR:		/* Retry forever */
				log("reXmtPacket: Interrupted system call\n");
				continue;
			}
			}


			log("reXmtPacket: send of datagram failed\n");
			switch(t_errno) {
			    case TFLOW:	/* Retry forever */
				continue;
			    default:
				log("reXmtPacket: UDP Write err: %d\n", t_errno);
				ud->addr.buf = NULL;
				ud->udata.buf = NULL;
				(void)t_free((char *)ud, T_UNITDATA);
				return 0;
			}
		}
		else {
			ud->addr.buf = NULL;
			ud->udata.buf = NULL;
			(void)t_free((char *)ud, T_UNITDATA);
			return reLength;
		}
	}
}

#endif 	/* TLI */


/*
   nAddrFmt: Format a network address for human consumption
		Can be called several times in the same printf
		argument list.
*/

#define		NNADDRMSG	4

static int	nAddrIndx;
static char	nAddrMsg[NNADDRMSG][256];

char *
nAddrFmt(nAddr)
unsigned char
	*nAddr;
{
	nAddrIndx = ++nAddrIndx % NNADDRMSG;

	sprintf(nAddrMsg[nAddrIndx], "%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x",
		nAddr[0], nAddr[1], nAddr[2], nAddr[3],
		nAddr[4], nAddr[5], nAddr[6], nAddr[7]);
	return nAddrMsg[nAddrIndx];
}

/*
 *  nAddrGet: Read an ethernet address (%x.%x.%x.%x.%x.%x.%x.%x)
 */

nAddrGet(addrStr, nAddr)
char
	*addrStr;
register char
	*nAddr;
{
register int
	i;			/* Count bytes of Ethernet address */

	nAddrClr(nAddr);

	for (i = 0; i < SZNADDR; i++) {
		if (!isxdigit(*addrStr))
			return 1;
		*nAddr++ = (char)strtol(addrStr, &addrStr, 16);
		if (*addrStr != '\0')
			addrStr++;
	}

	return 0;
}


void hostaddr(sa)
register struct sockaddr_in
	*sa;
{
static struct hostent
	*uName; 

	if (!uName)
		uName = myhostent();

	memcpy(&sa->sin_addr.s_addr, uName->h_addr, uName->h_length);

}


void localhostaddr(sa)
register struct sockaddr_in
	*sa;
{
static struct hostent
	*localuName; 

	if (!localuName)
		localuName = localhostent();

	memcpy(&sa->sin_addr.s_addr, localuName->h_addr, localuName->h_length);

}


void netaddr(intFaceListP, intFaceNumP, mode)
netIntFace	*intFaceListP;
int	*intFaceNumP;
int mode;
{
	int n;
	static struct hostent *uName; 
	long	*inAddrP;
	long	*inSubnetMaskP;
#ifdef BSD43
	struct ifreq *ifr;
	struct sockaddr_in *ifr_in;
	char buf[BUFSIZ];

#	if !defined(SYS5_4)
	struct ifconf *ifc;
#	endif

#	if defined(STREAMNET)
	struct strioctl sic;
#	else
	struct ifconf ifc_data;
#	endif
#endif /* BSD43 */

	if (!uName)
		uName = myhostent();

#ifdef BSD43
	if (mode & BCAST43) {
		
#	ifdef STREAMNET
		sic.ic_cmd = SIOCGIFCONF;
		sic.ic_len = sizeof (buf);
		sic.ic_dp = (char *)buf;
#		if !defined(SYS5_4)
		ifc = (struct ifconf *)buf;
		ifc->ifc_len = sizeof (buf) - sizeof (struct ifconf);
#			if defined(BELLTLI)
		sic.ic_len = ifc->ifc_len;
		sic.ic_dp += sizeof(struct ifconf);
		ifc->ifc_buf = sic.ic_dp;
#			endif	/* BELLTLI */
#		endif	/* !SYS5_4 */
		if (ioctl(theNetDesc, I_STR, (char *)&sic) < 0)
#	else	/* !STREAMNET */
		ifc = &ifc_data;
		ifc->ifc_len = sizeof (buf);
		ifc->ifc_buf = buf;
		if (ioctl(theNetDesc, SIOCGIFCONF, (char *)ifc) < 0)
#	endif	/* STREAMNET */
		{
			log("netaddr: ioctl(SIOCGIFCONF) failed: errno %d\n",
				errno);
		}
		*intFaceNumP = 0;

#	if defined(SYS5_4)
		ifr = (struct ifreq *)buf;
#	else
		ifr = ifc->ifc_req;
#	endif	/* SYS5_4 */

#	if defined(SYS5_4) || defined(BELLTLI)
		n =   sic.ic_len / sizeof(*ifr);
#	else
		n = ifc->ifc_len / sizeof(*ifr);
#	endif	/* SYS5_4 || BELLTLI */

		for (; n > 0; n--, ifr++) {

			ifr_in = (struct sockaddr_in *)&ifr->ifr_addr;

			/* Copy the local address */

			intFaceListP->localAddr.s_addr =ifr_in->sin_addr.s_addr;
			log("netaddr: local address: %s\n",
			  nAddrFmt((unsigned char *)&intFaceListP->localAddr));

			ifr_in = (struct sockaddr_in *)&ifr->ifr_broadaddr;

			/* Determine the broadcast address */
#	ifdef STREAMNET
			sic.ic_cmd = SIOCGIFBRDADDR;
			sic.ic_len = sizeof (struct ifreq);
			sic.ic_dp = (char *)ifr;
			if (ioctl(theNetDesc, I_STR, &sic) < 0)
#	else
			if (ioctl(theNetDesc, SIOCGIFBRDADDR, ifr) < 0)
#	endif	/* STREAMNET */
			{
				log("netaddr:SIOCGIFBRDADDR failed; errno %d\n",
					errno);
				continue;
			}

			intFaceListP->broadAddr.s_addr =ifr_in->sin_addr.s_addr;

			log("netaddr: broadcast address: %s\n",
			   nAddrFmt((unsigned char *)&intFaceListP->broadAddr));

			if (mode & USESUBNETS) {
			/* Determine the subnet mask */

#	ifdef STREAMNET
				sic.ic_cmd = SIOCGIFNETMASK;
				sic.ic_len = sizeof (struct ifreq);
				sic.ic_dp = (char *)ifr;
				if (ioctl(theNetDesc, I_STR, &sic) < 0)
#	else
				if (ioctl(theNetDesc, SIOCGIFNETMASK, ifr) < 0)
#	endif	/* STREAMNET */
				{
					log("netaddr:SIOCGIFNETMASK failed; errno %d\n",
						errno);
					continue;
				}

				intFaceListP->subnetMask.s_addr =
					ifr_in->sin_addr.s_addr;
			} else {
				inAddrP = (long *) &(intFaceListP->localAddr);
				inSubnetMaskP = (long *) &(intFaceListP->subnetMask);
				*inAddrP = ntohl(*inAddrP);
				if (IN_CLASSA(*inAddrP))
					*inSubnetMaskP = IN_CLASSA_NET;
				else if (IN_CLASSB(*inAddrP))
					*inSubnetMaskP = IN_CLASSB_NET;
				else if (IN_CLASSC(*inAddrP))
					*inSubnetMaskP = IN_CLASSC_NET;

				 *inAddrP = htonl(*inAddrP);
				 *inSubnetMaskP = htonl(*inSubnetMaskP);
			}

			log("netaddr: subnet mask: %s\n",
			  nAddrFmt((unsigned char *)&intFaceListP->subnetMask));

			++intFaceListP;
			++*intFaceNumP;
		}
	} else
#endif /* BSD43 */
	       {
		*intFaceNumP = 1;

		memcpy(&intFaceListP->localAddr, uName->h_addr, uName->h_length);
		memcpy(&intFaceListP->broadAddr, uName->h_addr, uName->h_length);

		inAddrP = (long *) &(intFaceListP->broadAddr);
		inSubnetMaskP = (long *) &(intFaceListP->subnetMask);
		*inAddrP = ntohl(*inAddrP);

		if (IN_CLASSA(*inAddrP)) {
			 *inAddrP |= ~IN_CLASSA_NET;
			 *inSubnetMaskP = IN_CLASSA_NET;
		} else if (IN_CLASSB(*inAddrP)) {
			 *inAddrP |= ~IN_CLASSB_NET;
			 *inSubnetMaskP = IN_CLASSB_NET;
		} else if (IN_CLASSC(*inAddrP)) {
			 *inAddrP |= ~IN_CLASSC_NET;
			 *inSubnetMaskP = IN_CLASSC_NET;
		}

		 *inAddrP = htonl(*inAddrP);
		 *inSubnetMaskP = htonl(*inSubnetMaskP);

		log("netaddr: broadcast address: %s\n",
			nAddrFmt((unsigned char *)&intFaceListP->broadAddr));
		log("netaddr: subnet mask: %s\n",
			nAddrFmt((unsigned char *)&intFaceListP->subnetMask));
	}
}


struct hostent *
myhostent()
{
struct hostent	*tmpentry;
static struct hostent
	myhostentry;
static int
	gotmyhostent;
extern struct hostent
	*gethostbyname();

	if (!gotmyhostent) {
		tmpentry = gethostbyname(myhostname());
		if (tmpentry == (struct hostent *) NULL)
			fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("NET4", 
		"Cannot find hostname '%1' in /etc/hosts.  PCI aborting.\n"),
			"%s" ,myhostname()));
		else {
			myhostentry = *tmpentry;
			gotmyhostent++;
		}
	}

	return &myhostentry;
}


struct hostent *
localhostent()
{
struct hostent	*tmpentry;
static struct hostent
	localhostentry;
static int
	gotlocalhostent;
extern struct hostent
	*gethostbyname();

	if (!gotlocalhostent) {
		tmpentry = gethostbyname("localhost");
		if (tmpentry == (struct hostent *) NULL)
			fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("NET4", 
		"Cannot find hostname '%1' in /etc/hosts.  PCI aborting.\n"),
			"%s" ,"localhost"));
		else {
			localhostentry = *tmpentry;
			gotlocalhostent++;
		}
	}

	return &localhostentry;
}

#endif	/* UDP42 */



#ifdef	RS232PCI

#ifdef RS232_7BIT
extern int using_7_bits;
#endif /* RS232_7BIT */

/*****************************************************************************
 *    rcvPacket() -
 *        is the receive portion of the DATALINK LAYER between
 *        UNIX and a remote PC.  It uses an RS-232 terminal line
 *        at the physical layer.  Getframe() searches for the start
 *        of a data frame by looking for a  SYNC NULL character
 *        sequence.  Once it finds this sequence it reads  four
 *        more "PROTECTED" bytes which contain the checksum and
 *        frame count.  These four bytes are not to be "destuffed"
 *        and are not examined for SYNC's.  If the frame count in
 *        the header is too large it waits in a "tight" loop
 *        looking for the begining of the next frame, awaiting the
 *        sending PC to timeout.  If at any time the pattern SYNC NULL
 *        is encountered all previous state information is purged
 *        and the new data is taken as an entirely new request.
 *        Getframe() writes the data frame into a buffer whose address
 *        is supplied by the caller and it returns a frame byte count.
 *
 */

int
rcvPacket(inPkt)			/* Returns byte count of frame	*/
struct input
	*inPkt;				/* Pointer to input buffer	*/
{
register int
#ifdef RS232_7BIT
	i,				/* Loop counter in SYNC search	*/
#endif /* RS232_7BIT */
	bytes,				/* Bytes returned in read call	*/
	count,
	framesize;			/* Flag & temporary storage	*/
int
	flipCode;			/* Byte flipping code */
unsigned short
	tmpshort;

extern ttymodes;
char
	buf[2 * MAX_FRAME];		/* Lowest layer input buffer	*/
extern int flipBytes;			/* Byte ordering flag */

/* */
	while (0)
	{
resync:
		nak();
#ifdef	TIMEOUT
		timeout_on();
#endif	/* TIMEOUT */
	}

	count = 0;

	r232dbg(("receive: resync: count = %d\n", count));

	/* Find begining of frame by searching for SYN-NULL sequence. */
	for (;;) {
		if ((bytes = read(theNetDesc, buf, 1)) <= 0) {
			if (errno != EINTR)
				log("receive: TTY Read err: %d\n", errno);
			continue;
		}

#ifdef RS232_7BIT
		if (using_7_bits)
			buf[0] &= 0x7f;
#endif /* RS232_7BIT */

		r232dbg(("want %#x (SYNC), got %#x\n", SYNC, buf[0] & 0xff));
		r232dbg(("bytes = %#x; count = %#x\n", bytes, count));

		if (buf[0] == SYNC) {
			if ((bytes = read(theNetDesc, &buf[1], 1)) <= 0) {
				if (errno != EINTR)
					log("receive: TTY Read err: %d\n",
						errno);
				continue;
			}

#ifdef RS232_7BIT
			if (using_7_bits)
				buf[1] &= 0x7f;
#endif /* RS232_7BIT */

			r232dbg(("want %#x (NULL), got %#x\n", NULL,
				buf[1] & 0xff));
			r232dbg(("bytes = %#x; count = %#x\n", bytes, count));

			if (buf[1] == NULL) {
				count = 2;
				break;
			}
		}
	}

#ifdef	TIMEOUT
	timeout_on(theNetDesc, &ttymodes);
#endif	/* TIMEOUT */
	/* Read in PROTECTED portion of frame */
	while (count < PROTECTED) {
		r232dbg(("SyncNull: bytes = %#x count = %#x\n", bytes, count));

		bytes = read(theNetDesc, &buf[count], PROTECTED - count);
		if (bytes > 0) {
#ifdef RS232_7BIT
			if (using_7_bits) {
				for (i = count; i < count+bytes; i++)
					buf[i] &= 0x7f;
			}
#endif /* RS232_7BIT */
			r232dbg(("count %d; bytes %d;\n", count, bytes));
			count += bytes;
		}
	}

	/* Get FRAME SIZE from protected portion of header. */
	if (flipBytes)
	    dosflipm(((struct rs232 *)buf)->f_cnt, tmpshort);
	framesize = ((struct rs232 *)buf)->f_cnt;
#ifdef RS232_7BIT
	if (using_7_bits)
		framesize = ((framesize & 0x7f00) >> 1) | (framesize & 0x7f);
#endif /* RS232_7BIT */

	r232dbg(("framesize %#x\n", framesize));

	/* Do range check on frame size */
	if ((framesize > 2*MAX_FRAME) || (framesize == 0))
		goto resync;

	/* Read in rest of frame */
	while (count < framesize) {
		bytes = read(theNetDesc, &buf[count], framesize - count);
		if (bytes <= 0) {
			if (errno == EINTR) {
				/* It is unlikely that any data has been lost;
				 * just restart the read.
				 */
				continue;
			}
			log("receive: TTY Read err: %d\n", errno);
			goto resync;
		}
#ifdef RS232_7BIT
		if (using_7_bits) {
			for (i = count; i < count+bytes; i++)
				buf[i] &= 0x7f;
		}
#endif /* RS232_7BIT */
		r232dbg(("bytes %d; count %d; chars: %.*s\n", bytes, count,
			bytes, buf));
		count += bytes;
	}

#ifdef RS232_PKTDBG
	rs232_log_packet("Received", buf, count);
#endif /* RS232_PKTDBG */

	if (flipBytes)
	    dosflipm(((struct rs232 *)buf)->chks, tmpshort);

	if (((
#ifdef RS232_7BIT
	      using_7_bits ? 0x7f7f :
#endif /* RS232_7BIT */
				      0xffff) & chksum(buf, count)) !=
		((struct rs232 *)buf)->chks) {
		log("chksum: expected %x, got %x\n",
			((struct rs232 *)buf)->chks,
			((
#ifdef RS232_7BIT
			  using_7_bits ? 0x7f7f :
#endif /* RS232_7BIT */
						  0xffff) & chksum(buf,count)));
		goto resync;
	}

	/* Unstuff input buffer */
	count = unstuff(buf, (char *)inPkt, count);

	/* Now do proper range check on frame size */
	if ((count > MAX_FRAME) || (count == 0)) {
		log("bad framesize: %d\n", count);
		goto resync;
	}

#ifdef RS232_7BIT
	/* Convert to 8 bit data if needed */
	if (using_7_bits)
		count = convert_from_7_bits((unsigned char *)inPkt, count);
#endif /* RS232_7BIT */

#ifdef	TIMEOUT
	timeout_off(theNetDesc, &ttymodes);
#endif	/* TIMEOUT */
	flipCode = input_swap(inPkt, inPkt->hdr.pattern);

#ifdef RS232_PKTDBG
	rs232_log_packet("Processed", (char *)inPkt, count);
#endif /* RS232_PKTDBG */

	logPacket(inPkt, PLOG_RCV, LOG_SOME);
	return flipCode;
}


/*****************************************************************************
 *  xmtPacket()  takes an output frame and sends it to the appropriate
 *          device driver (rs232) for delivery.
 *
 */

int
xmtPacket(outPacket, niUnused, flipCode)
register struct output
	*outPacket;
struct ni2
	*niUnused;			/* place holder */
register int
	flipCode;
{
register int
	status;			/* Number of characters written */
int
	retryCount;
long
	length;
#ifdef RS232_7BIT
short
	tmpshort;
#endif /* RS232_7BIT */

	length = outPacket->hdr.t_cnt + HEADER;
	outPacket->rs232.syn = SYNC;
	outPacket->rs232.null = NULL;
	outPacket->rs232.f_cnt = (unsigned short)length;
	outPacket->rs232.chks = chksum((char *)outPacket, length);

	logPacket((struct input *)outPacket, PLOG_XMT, LOG_SOME);
	output_swap(outPacket, flipCode);

#ifdef RS232_7BIT
	/* Handle 7 bit output */
	if (using_7_bits) {
		length = convert_to_7_bits((unsigned char *)outPacket, length);
		outPacket->rs232.f_cnt = (unsigned short)(((length&0x3f80)
							<< 1) | (length&0x7f));
		outPacket->rs232.chks =
				chksum((char *)outPacket, length) & 0x7f7f;
		if (flipCode & SFLIP) {
		    dosflipm(outPacket->rs232.f_cnt, tmpshort);
		    dosflipm(outPacket->rs232.chks, tmpshort);
		}
	}
#endif /* RS232_7BIT */

#ifdef RS232_PKTDBG
	rs232_log_packet("Sending", outPacket, length);
#endif /* RS232_PKTDBG */

	/* Send response */
	for (retryCount = 0; retryCount < MAXXRETRY; retryCount++) {
		if ((status = write(theNetDesc, outPacket, length)) == length)
			break;
		log("send: TTY Write err: %d\n", errno);
	}

	return length;
}


/*****************************************************************************
 *  reXmt: retransmit last packet
 */

reXmt(rePacket, reLength)
struct output
	*rePacket;
int
	reLength;
{
	log("reXmt: seq %d\n", rePacket->hdr.seq);

	for (;;) {
		if (write(theNetDesc, rePacket, reLength) == reLength)
			return reLength;

		if (errno == EINTR)
			continue;

		log("reXmt: TTY write err: %d", errno);
		return 0;
	}
}

void nak()
{
extern unsigned char	brg_seqnum;		/* Current sequence number */
extern int swap_how;		/* How to swap bytes */
struct
	output retryPkt;
struct ni2
	*niUnused;			/* place holder */
/* */
	memset(&retryPkt.hdr, 0, sizeof(struct header));

	retryPkt.hdr.seq = ~brg_seqnum;
	retryPkt.hdr.res = FAILURE;
	xmtPacket(&retryPkt, niUnused, swap_how);

	return;
}

#ifdef RS232_PKTDBG

rs232_log_packet(desc, pkt, len)
char *desc;
unsigned char *pkt;
register int len;
{
	register int i;

	r232dbg(("%s packet, len = %d\ndata:\n", desc, len));
	while (len > 0) {
		for (i = 0; i < 16 && len-- > 0; i++)
			r232dbg((" %02x", *pkt++));
		r232dbg(("\n"));
	}
}
#endif /* RS232_PKTDBG */

#endif	  /* RS232PCI  */
