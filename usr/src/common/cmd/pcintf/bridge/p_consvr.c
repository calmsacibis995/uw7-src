#ident	"@(#)pcintf:bridge/p_consvr.c	1.1.1.4"
#include	"sccs.h"
SCCSID(@(#)p_consvr.c	6.13	LCC);	/* Modified: 11:19:09 12/3/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

/*		PC Interface Connection Server			*/

#include "sysconfig.h"

#include <sys/types.h>
#include <sys/stat.h>
#if defined(SYS5_4)
#	include <sys/filio.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <malloc.h>

#if defined(BERK42FILE) && !defined(LOCUS)
#	include <wait.h>
#endif	/* BERK42FILE && !LOCUS */

#if !defined(BERKELEY42) || defined(AIX_RT) && !defined(LOCUS)
#	include <termio.h>
#endif	/* !BERKELEY42 || AIX_RT && !LOCUS */

#if defined(SYS19)
#	include <ioctl42.h>
#else
#	if defined(RIDGE)
#		include <bsdioctl.h>
#	else
#		include <sys/ioctl.h>
#	endif	/* RIDGE */
#endif	/* SYS19 */

#if defined(ISC386)
#define FIONBIO	(TIOC | 12)	/* non-blocking mode */
#endif

#include <lmf.h>

#include "pci_proto.h"
#include "pci_types.h"
#include "common.h"
#include "const.h"
#include "flip.h"
#include "log.h"
#include "version.h"

#define UNIX
#include "serial.h"
#define PRINT_CS  /**/

#define	PNAME		"pciconsvr"

char
	CopyRight[] =
"PC INTERFACE UNIX FILE SERVER.  COPYRIGHT (C) 1984, LOCUS COMPUTING CORP.\n\
ALL RIGHTS RESERVED.  USE OF THIS SOFTWARE IS GRANTED ONLY UNDER LICENSE\n\
FROM LOCUS COMPUTING CORPORATION.  UNAUTHORIZED USE IS STRICTLY PROHIBITED.\n";


/*
   struct pcNex: Connection table entry.  One is used for
			each currently active server/PC pair.
*/

struct pcNex {
	struct pcNex *next;		/* next structure in table */
	struct pcNex *nextHash;		/* next structure in hash list */
	char	pcAddr[SZNADDR];	/* Destination address of connection */
	char	pcAddr2[SZNADDR];	/* Source of copy violator */	
	char	pcSerial[16];		/* serial number of pc software	*/
	short	probeCount;		/* Set when daemon receives PROBE msg */
	short	conNumber;		/* Conenction number */
	pid_t	svrPid;			/* Process id of PC Interface server */
	pid_t	emPid;			/* Process id of Emulation login */
	short	flags;			/* Flags for connection */
	char	emPty[20];		/* Name of login pty */
};

#define CTF_INDICT	0x01		/* Set when there may be a cp viol */
#define CTF_EM_CLOSE	0x02		/* Set when safe to signal svr */

struct pcNex *conTable = NULL;		/* List of current connections */
struct pcNex *conFree = NULL;

struct pcNex *conHash[256];	/* Hash pointers */

static struct ni2
	mapSvrHeader,		/* Only one map server address in IP */
	myHeader;		/* This connection server's address */

long consvr_here_time = 30;
long consvr_here_last = 0;

#define SCHED_INTERVAL	30	/* Check connections this often */
#define	PROBE_TIMEOUT	5	/* Max missed probes before disconnect */

#define	HASH_COMMENT	'#'

int
	noDisconnect,		/* Don't disconnect on probe timeout */
	ConSvrPid;		/* The PID of the Consvr */
static	int	line_pos = 0;	/* for feature reading */

long
	dosLogCtl;		/* Dos Server debug level */

char
	*netDev = NETDEV,	/* Network interface device */
	fqNetDev[32],		/* Fully qualified netDev name */
	*logArg,		/* Debug argument string */
	*dosLogArg,		/* Dos server debug argument */
	*attr_String = "F0";	/* pointer to feature string */

char *myname;

netIntFace	intFaceList[MAX_NET_INTFACE];

int	numIntFace = 0;			/* The # of configured interfaces */


typedef struct pcNex	pcNex;
typedef char		*ref;

int		main		PROTO((int, char **));
LOCAL int	getFeature	PROTO((FILE *, char *));
LOCAL void	connectService	PROTO((void));
LOCAL void	newConnection	PROTO((struct input *, struct output *, long));
LOCAL void	conProbe	PROTO((struct input *, struct output *, int));
LOCAL void	disConnect	PROTO((struct input *, struct output *, long));
LOCAL void	sendSiteAttr	PROTO((struct input *, struct output *, long));
LOCAL void	securityCheck	PROTO((struct input *, long));
LOCAL void	closeShop	PROTO((void));
LOCAL void	toMapSvr	PROTO((unsigned char));
LOCAL void	schedOn		PROTO((void));
LOCAL struct pcNex *newPort	PROTO((int *));
LOCAL void	cleanUp		PROTO((void));
LOCAL void	sched		PROTO((void));
LOCAL void	ageConnects	PROTO((void));
LOCAL void	catchSignal	PROTO((int));
LOCAL void	checkCode	PROTO((void));
LOCAL void	setnameAddr	PROTO((struct nameAddr *));
LOCAL void	killSvr		PROTO((struct pcNex *, int));
LOCAL void	initConTable	PROTO((void));
LOCAL struct pcNex	*allocCTSlot	PROTO((void));
LOCAL void		freeCTSlot	PROTO((struct pcNex *));
LOCAL char	       *readFeatureFile	PROTO((char *));
LOCAL void		addCTIndex	PROTO((struct pcNex *));
LOCAL void		rmvCTIndex	PROTO((struct pcNex *));
LOCAL struct pcNex	*getCTAddr	PROTO((struct sockaddr_in *,int));
LOCAL struct pcNex	*getCTSerial	PROTO((SERI_ST *));
LOCAL struct pcNex	*getCTSvrPID	PROTO((int));

#if !defined(DONT_START_GETTY)
LOCAL void	s_login		PROTO((struct input *, struct output *, int));
LOCAL void	k_login		PROTO((struct input *, struct output *, int));

#	if !defined(BERKELEY42)
LOCAL void	addut		PROTO((char *));
LOCAL void	rmut		PROTO((char *));
#	endif	/* BERKELEY42 */
#endif   /* DONT_START_GETTY */

#if defined(UDP42)
extern struct hostent	*myhostent	PROTO((void));
#endif	/* UDP42 */

#if defined(SecureWare)
extern int		getluid		PROTO((void));
#endif


main(argc, argv)
int	argc;
char	*argv[];
{
int
	netdesc = -1,		/* Network descriptor */
#if defined(SecureWare)
	luid,			/* login user id -- should not be set */
#endif
	argN;			/* Current argument number */
char
	*arg;			/* Current argument */
register char *i;		/*temp for sin_zero init */

	/* determine machine byte ordering */
	byteorder_init();

	/* initialize NLS */
	nls_init();

	/* Save Consvr PID to check after forking for Dossvr */ 
	ConSvrPid = getpid();

	myname = argv[0];
	if (*myname == '\0')
		myname = "unknown";

	/* Open logfile to record unexpected occurences */
	logOpen(CONSVR_LOG, 0);

	for (argN = 1; argN < argc; argN++) {
		arg = argv[argN];

		if (*arg != '-') {
			fatal(lmf_get_message("P_CONSVR3", 
				"pciconsvr: Bad usage\n"));
			exit(1);
		}

		switch (arg[1]) {
		case 'x':
			noDisconnect = 1;
			break;

		case 'F':
			attr_String = readFeatureFile(&arg[2]);
			break;

		case 'D':
			dbgSet(strtol(&arg[2], NULL, 16));
			logArg = &arg[2];
			break;

		case 'L':
			dosLogCtl = strtol(&arg[2], NULL, 16);
			dosLogArg = &arg[2];
			break;


		case 'N':			/* Network device */
			netDev = &arg[2];
			if (netDev == '\0')
				netDev = NETDEV;
			else if (*netDev != '/') {
				sprintf(fqNetDev, "/dev/%s", netDev);
				netDev = fqNetDev;
			}
			break;

		case 'n':		/* Network descriptor */
			netdesc = atoi(&arg[2]);
			log("netdesc on %d\n",netdesc);
			break;

		case 'I':		/* Interface list */
			numIntFace = get_interface_list(&arg[2]);
			break;

		case 'T':
			consvr_here_time = atoi(&arg[2]);
			if (consvr_here_time < -1 || consvr_here_time > 3600)
				consvr_here_time = 30;
			break;
		}
	}
	fputs(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("P_CONSVR10",
		"PC-Interface Release %1.%2.%3\n"),
		"%d%d%d", VERS_MAJOR, VERS_MINOR, VERS_SUBMINOR),
		stdout);
	fflush(stdout);

	/* Standard in/out/err not needed */
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);

#if defined(SecureWare)
	if ((luid = getluid()) != -1 || errno != EPERM)
		fatal(lmf_format_string((char *)NULL, 0,
				lmf_get_message("P_CONSVR_LUID",
				"luid is already set to %1\n"), "%d", luid));
#endif

	log("PC Interface Connection Server Start\n");

	if (netdesc >= 0)
		netUse(netdesc);
	else if (netUse(netOpen(netDev, PCI_CONSVR_PORT)) < 0)
		fatal(lmf_get_message("P_CONSVR1",
			"Can't open network - Bye\n"));


	/* Enable signals */
	signal(SIG_CHILD,  catchSignal);	/* Server death */
	signal(SIG_DBG1, catchSignal);  /* Change debug level */
	signal(SIG_DBG2, catchSignal);  /* Change dos server debug level */
	signal(SIGTERM, catchSignal);	/* Go Away */

	/* Detach myself from my invoker */
#ifdef	SYS5
	setpgrp();
#endif	/* SYS5 */

	/* Initialize the connection table */
	initConTable();

	/*
	   This is where the map servers listen out
	   Create header for each type of map server
	*/
	if (numIntFace == 0)
		netaddr(intFaceList, &numIntFace, 0);
	mapSvrHeader.dst_sin.sin_family = AF_INET;
	mapSvrHeader.dst_sin.sin_port = htons(PCI_MAPSVR_PORT);
#define destin mapSvrHeader.dst_sin.sin_zero
	for (i = destin; i < destin + 8; i++)
 	    *i = 0;
#undef destin

	toMapSvr(CONSVR_HERE);
	schedOn();

	/* Checksum the code befor we start */
	checkCode();

	/* Everything is initialized - Start main connection service loop */
	connectService();

	log("Connection service ended - Bye\n");
	return 0;
}


/*
   readFeatureFile: read the feature file
*/

static int
gchar(fp)
FILE	*fp;
{
	int	cchar;

	cchar = getc(fp);
	if (cchar >= 0) {
		if (cchar == '\n')
			line_pos = 0;
		else
			line_pos ++;
	}
	return(cchar);
}

static int
eatLine(fp)
FILE	*fp;
{
	int	cchar;	/* current character */

	do {
		cchar = gchar(fp);
		if (cchar < 0)
			return(0);
	} while (cchar != '\n');

	return(cchar);
}

int
getFeature(fp, buf)
FILE	*fp;
char	*buf;
{
	int	cchar;	/* current character */

	do {
	   cchar = gchar(fp);		/* get next character */
	   if (cchar < 0)
		return(0);		/* stop reading on error */
	   if ((cchar == HASH_COMMENT) && (line_pos == 1)) {
		if (!eatLine(fp))	/* error */
			return(0);
		cchar = ' ';		/* make this a space */
	   }
	} while (isspace(cchar));

	buf[0] = (char)cchar;
	cchar = gchar(fp);
	if ((cchar < 0) || (isspace(cchar)))
			return(-1);
	buf[1] = (char)cchar;

	buf[2] = '\0';
	return(1);
}


char *
readFeatureFile(fn)
char	*fn;
{
	FILE	*fp;
	char	*fb, fbuf[3];
	int	slen;
	int	i;

	fp = fopen(fn, "r");

	if (!fp) {
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("P_CONSVR4",
			"%1: error opening feature file %2\n"),
			"%s%s", PNAME, fn));
		exit(1);
	}

	fb = malloc(MAX_FSTR_LEN);
	if (!fb) {
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("P_CONSVR5",
			"%1: error allocating RAM for feature string\n"),
			"%s", PNAME));
		fclose(fp);
		return(NULL);
	}

	strcpy(fb, "F0");
	slen = strlen(fb);

	while (i = getFeature(fp, fbuf)) {
		if (i < 0) {	/* error */
			fatal(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("P_CONSVR6",
				"%1: error in feature file %2\n"),
				"%s%s", PNAME, fn));
			fclose(fp);
			free(fb);
			return(NULL);
		}

		/* room to add this? */

		if ((slen + strlen(fbuf)) >= (size_t)MAX_FSTR_LEN) {
		   fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("P_CONSVR5",
			"%1: error allocating RAM for feature string\n"),
			"%s", PNAME));
			fclose(fp);
			free(fb);
			return(NULL);
		}

		strcat(fb, fbuf);
		slen += strlen(fbuf);
	}
	fclose(fp);
	return(realloc(fb, strlen(fb) + 1));
}

/*
   connectService: Create, maintain and delete bridge connections
*/

void connectService()
{
struct input
	inPacket;		/* Input buffer */
struct output
	outPacket;		/* Output buffer */
int
	flipCode;		/* How to flip bytes in output packets */

	for (;;) {
		flipCode = rcvPacket(&inPacket);
		outPacket.hdr.res = SUCCESS;
		outPacket.hdr.stat = NEW;
		outPacket.hdr.seq = inPacket.hdr.seq;
		outPacket.hdr.req = inPacket.hdr.req;
		outPacket.pre.select = DAEMON;
		outPacket.hdr.t_cnt = 0;
#ifdef	ETHPORTS
		outPacket.net.head[PCIPORTOFF] = PCI_CONSVR_PORT;
#endif	/* ETHPORTS */

		/* Store address of destination PC */

		nAddrCpy(myHeader.dst, inPacket.net.src);

		/* Is packet destined for DAEMON stream? */
		if (inPacket.pre.select != DAEMON) {
			log("Bad sel %d\n", inPacket.pre.select);
			outPacket.hdr.res = INVALID_FUNCTION;
			xmtPacket(&outPacket, &myHeader, flipCode);
			continue;
		}

		switch (inPacket.hdr.req) {
		case CONNECT:
			newConnection(&inPacket, &outPacket, flipCode);
			break;

		case PROBE:
			conProbe(&inPacket, &outPacket, flipCode);
			break;

		case SECURITY_CHECK:
			securityCheck(&inPacket, flipCode);
			break;

		case DISCONNECT:
			disConnect(&inPacket, &outPacket, flipCode);
			checkCode();
			break;

#ifndef	DONT_START_GETTY
		case S_LOGIN:
			s_login(&inPacket, &outPacket, flipCode);
			break;

		case K_LOGIN:
			k_login(&inPacket, &outPacket, flipCode);
			break;
#endif   /* DONT_START_GETTY */

		case GET_SITE_ATTR:
			sendSiteAttr(&inPacket, &outPacket, flipCode);
			break;

		default:
			log("Bad req %d\n", inPacket.hdr.req);
			outPacket.hdr.res = INVALID_FUNCTION;
			xmtPacket(&outPacket, &myHeader, flipCode);
		}
	}
}


/*
   newConnection: Establish new PC Interface service
*/

void newConnection(conPacket, rspPacket, flipCode)
register struct input	*conPacket;	/* Packet requesting connection */
register struct output	*rspPacket;	/* Response packet */
long flipCode;				/* Flipping code */
{
	int
		svrDesc,		/* Servers network port descriptor */
		newSvrPid;		/* New server pid */
#ifdef	VERSION_MATCHING
	register struct connect_text
		*connText;		/* pointer to text area of pkt. */
	struct stat svrStatBuf;		/* For doing stat() on dossvr */
	char	versionArg[16];		/* Version number */
#endif	/* VERSION_MATCHING */
	int
		savenetDesc;		/* holds previous net desc  */
	char
		svrProg[64],		/* Name of server program */
		debugArg[16],		/* Debug level argument */
		netArg[16],		/* Network descriptor number */
		*versionArgP;		/* pointer to Version argument */
	register struct pcNex
		*ctSlot;		/* conTable table slot (old AND new) */
	int	conNum = -1;		/* Connection number */

	/* Check for connection number in packet */
	if (conPacket->hdr.t_cnt > 22)
		conNum = conPacket->text[22] & 0xff;

	log("Connect req from %s, con num %d\n",
		nAddrFmt((unsigned char *)conPacket->net.src), conNum);

	/* Look up requestor's address in conTable */
	if ((ctSlot = getCTAddr((struct sockaddr_in *)conPacket->net.src,
		conNum)) != 0)
		/* If old server from this client still exists, kill it */
		killSvr(ctSlot,  SIGTERM);

	/* Allocate a new network endpoint/socket/port/etc */
	savenetDesc = netSave();
	if ((ctSlot = newPort(&svrDesc)) == NULL) {
		log("...No net ports\n");
		rspPacket->hdr.res = FAILURE;
		rspPacket->hdr.b_cnt = 0;
		xmtPacket(rspPacket, &myHeader, flipCode);
		return;
	}
	netUse(svrDesc);

#ifdef VERSION_MATCHING
	connText = (struct connect_text *) conPacket->text;
	log("PC version: %d.%d.%d \n", (int) connText->vers_major,
	    (int) connText->vers_minor, (int) connText->vers_submin);
#endif	/* VERSION_MATCHING */

    if (conPacket->hdr.versNum == 0) {	/* do only for "statndard" startups */

#ifdef VERSION_MATCHING
	/* get version into server name */
	sprintf(svrProg, "%s/versions/%d.%d/%s", PCIDIR,
		(int) connText->vers_major, 
		(int) connText->vers_minor, PCIDOSSVR);

	/* Check the existence of the server. If it's mode says that it is */
	/* a directory then it is a hidden directory but the load module   */
	/* is not there							   */

	if ((stat(svrProg, &svrStatBuf) < 0) || (S_ISDIR(svrStatBuf.st_mode)))
	{
		rspPacket->hdr.res = FAILURE;
		rspPacket->hdr.b_cnt = 1; /* version matching failure. */
		xmtPacket(rspPacket, &myHeader, flipCode);
		log("cannot access svrProg: %s\n", svrProg); /* JD */
		netUse(savenetDesc); /* restore our port */
		close(svrDesc);	/* close child's port */
		return;
	}
#else	/* !VERSION_MATCHING */
	sprintf(svrProg, "%s/%s", PCIBIN, PCIDOSSVR); 
#endif	/* VERSION_MATCHING */
#ifdef IBM_SERCHK
	if (serchk(conPacket,flipCode) == 0) { /* bad serial# for version */
		rspPacket->hdr.res = FAILURE;
		rspPacket->hdr.b_cnt = 1; /* version matching failure. */
		xmtPacket(rspPacket, &myHeader, flipCode);
		netUse(savenetDesc); /* restore our port */
		close(svrDesc);	/* close child's port */
		return;
	}
#endif /* IBM_SERCHK */
    }

	if ((newSvrPid = fork()) < 0) {
		log("...Can't fork\n");
		rspPacket->hdr.res = FAILURE;
		rspPacket->hdr.b_cnt = 0;
	}
	else if (newSvrPid == 0) {
		if (conPacket->hdr.versNum != 0)	/* cmd line "-s<num>" */
			sprintf(svrProg, "%s/%s%d", PCIBIN, PCIDOSSVR,
				conPacket->hdr.versNum);
		sprintf(debugArg, "-D%04x", dosLogCtl);
		sprintf(netArg, "-n%d", svrDesc);
#ifdef VERSION_MATCHING
		sprintf(versionArg, "-V%d.%d",
			(int) connText->vers_major, 
			(int) connText->vers_minor);
		versionArgP = versionArg;
#else
		versionArgP = (char *)0;
#endif
		logClose();
		close(savenetDesc);	/* disconnect from parent's port */

		execl(svrProg, strrchr(svrProg, '/') + 1, debugArg, netArg,
			versionArgP, (char *)0);
		/* All dressed up, no place to go */
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("P_CONSVR2",
			"Can't exec DOS server `%1': %2\n"),
			"%s%d", svrProg, errno));
	} else {
		/* Set-up the parent: build and store connection table entry */
		nAddrCpy(ctSlot->pcAddr, conPacket->net.src);
		serialCpy(ctSlot->pcSerial, conPacket->text);
		ctSlot->svrPid = newSvrPid;
		ctSlot->emPid = 0;
		ctSlot->probeCount = 0;
		ctSlot->conNumber = conNum;
#ifdef VERSION_MATCHING
		if (connText->vers_major < 4 ||
		    (connText->vers_major == 4 && connText->vers_minor == 0))
			ctSlot->flags = 0;
		else
#endif
			ctSlot->flags = CTF_EM_CLOSE;

		/* Index the new conTable entry */
		addCTIndex(ctSlot);

		/* Tell PC whom to talk to */
		rspPacket->hdr.res = SUCCESS;
	}

	xmtPacket(rspPacket, &myHeader, flipCode);

	netUse(savenetDesc); /* restore our port */
	close(svrDesc);	/* close child's port */
}


/*
   conProbe: Keep connection alive
	     Check for copy protection violators
*/

void conProbe(prbPacket, rspPacket, flipCode)
struct input *prbPacket;
struct output *rspPacket;
{
	register struct pcNex *ctSlot;	/* conTable slot for probing client */
	int conNum = -1;		/* Connection Number */

	/* Check for connection number in packet */
	if (prbPacket->hdr.t_cnt > 0)
		conNum = prbPacket->text[0] & 0xff;

	log("Probe from %s, con num %d\n",
		nAddrFmt((unsigned char *)prbPacket->net.src), conNum);

	if ((ctSlot = getCTAddr((struct sockaddr_in *)prbPacket->net.src,
		conNum)) == NULL)
		return;

	ctSlot->probeCount = 0;
	/* 
	   If there is a connection here, we want to kill
	   any new one trying to use the same serial no.
	*/
	if (ctSlot->flags & CTF_INDICT) {
		rspPacket->pre.select = UNSOLICITED;
		rspPacket->hdr.req = PC_CRASH;

		/* Kill the pc without a connection  here */
		nAddrCpy(myHeader.dst, ctSlot->pcAddr2);
		xmtPacket(rspPacket, &myHeader, flipCode);
		log("PC_CRASH to %s\n",
			nAddrFmt((unsigned char *)ctSlot->pcAddr2));
		ctSlot->flags &= ~CTF_INDICT;
	}
}


/*
   disConnect: Destroy connection
*/

void disConnect(disPacket, rspPacket, flipCode)
struct input	*disPacket;
struct output	*rspPacket;
long flipCode;
{
	struct pcNex *ctSlot;		/* Slot of session to disconnect */
	int conNum = -1;		/* Connection Number */

	/* Check for connection number in packet */
	if (disPacket->hdr.t_cnt > 0)
		conNum = disPacket->text[0] & 0xff;

	log("Disconnect req from %s, con num %d\n",
		nAddrFmt((unsigned char *)disPacket->net.src), conNum);

	ctSlot = getCTAddr((struct sockaddr_in *)disPacket->net.src, conNum);
	if (ctSlot != NULL) 
		killSvr(ctSlot, SIGTERM);
	else
		log("...Unknown connection\n");
	rspPacket->hdr.res = NO_SESSION;
	rspPacket->hdr.req = disPacket->hdr.req;
	rspPacket->hdr.seq = disPacket->hdr.seq;
	rspPacket->hdr.stat = NEW;
	rspPacket->hdr.t_cnt = 0;
	rspPacket->pre.select = DAEMON;
	xmtPacket(rspPacket, &myHeader, flipCode);
}


/*
	sendSiteAttr() - send attributes of site
*/

void sendSiteAttr(inPack, outPack, flipCode)
struct input	*inPack;
struct output	*outPack;

long
	flipCode;
{

	log("Site attr req from %s\n",
		nAddrFmt((unsigned char *)inPack->net.src));

	outPack->hdr.res = SUCCESS;
	outPack->hdr.req = inPack->hdr.req;
	outPack->hdr.seq = inPack->hdr.seq;
	outPack->hdr.stat = NEW;
	outPack->hdr.t_cnt = (unsigned short)strlen(attr_String) + 1;
	strcpy(outPack->text, attr_String);
	outPack->pre.select = DAEMON;
	xmtPacket(outPack, &myHeader, flipCode);
}

/*
   securityCheck:

   The two things that we are trying to catch are: multiple copies
   of the same pci diskette being run at the same time and attempts
   to foil this protection by modifying the unique information recorded
   on each diskette.  We will detect multiple copies even if the servers
   are on different machines.

   The tests are performed as follows:
   The map server takes care of invalid serial numbers so that all that
   is handled here is multiple uses of a copy.
   The only thing to check is whether there is a current connection
   already for this serial number.  If not do nothing.  If there is then 
   we set the indict member in the connection table  entry for the pc with
   this number.  The net address of the new pc is added to the connection
   table as pcAddr2.  If a probe arrives on the old connection we can
   halt both or one of them.
*/

void securityCheck(conPacket, flipCode)
register struct input *conPacket;
long flipCode;
{
	register struct pcNex *ctSlot;	/* conTable entry relevant to check */
	struct seriEthSt *seriEthP;

	log("Security check from %s\n",
		nAddrFmt((unsigned char *)conPacket->net.src));

	/* Get pointer to serialization structure in security check request */
	seriEthP = (struct seriEthSt *)conPacket->text;

	/* Get connection with this serial number */
	if ((ctSlot = getCTSerial(&seriEthP->serNum)) == NULL)
		return;

	/* Potential duplicate bridge if addresses don't match */
	if (!nAddrEq(ctSlot->pcAddr, seriEthP->serAddr)) {
		/* Indict and save address of bad guy */
		ctSlot->flags |= CTF_INDICT;
		nAddrCpy(ctSlot->pcAddr2, seriEthP->serAddr);
#ifdef	VERBOSE_LOG
		log("Security indict from %s \n", nAddrFmt(seriEthP->serAddr));
#endif
	}
}


/*
   closeShop: Tell map servers this site is closing down
*/

void closeShop()
{
	/* Stop the clock */
	alarm(0);

	/* Send bye-bye message twice to make sure everyone hears */
	toMapSvr(CONSVR_BYE);
	sleep(5);
	toMapSvr(CONSVR_BYE);
}

/*
   toMapSvr: Send message to Map Server multicast address; text of
		message is always a nameAddr for this consvr;
		mapSvrReq is the request code passed.
*/

#if defined(__STDC__)
void toMapSvr(unsigned char mapSvrReq)
#else
void toMapSvr(mapSvrReq)
unsigned char
	mapSvrReq;
#endif
{
	int	i;			/* Loop counter */
	struct nameAddr	*myName;
	struct output	outPacket;

	myName = (struct nameAddr *) outPacket.text;

	/* Store system name and network address */ 

	memset(myName, 0, sizeof(struct nameAddr));
	setnameAddr(myName);

	outPacket.hdr.req = mapSvrReq;
	outPacket.hdr.t_cnt = sizeof(struct nameAddr);
	outPacket.hdr.time = (unsigned short)consvr_here_time;

	for (i = 0; i < numIntFace; ++i) {
		memcpy(myName->address, &(intFaceList[i].localAddr), SZHADDR);
		memcpy(&(mapSvrHeader.dst_sin.sin_addr.s_addr),
			&(intFaceList[i].broadAddr), SZHADDR);
		if ((xmtPacket(&outPacket, &mapSvrHeader, NOFLIP) < 0)
		    && (errno == EACCES)) {
				xmtPacket(&outPacket, &mapSvrHeader, NOFLIP);
		}
	}
}


/*
   schedOn: Start periodic monitoring activities
*/

void schedOn()
{
	consvr_here_last = consvr_here_time;
	signal(SIGALRM, catchSignal);
	alarm(SCHED_INTERVAL);
}


/*
   newPort:  Allocate a unique ethernet port number.
*/

struct pcNex *
newPort(toNetDesc)
int *toNetDesc;			/* Put net device descriptor here */
{
	register int netDesc;		/* File descriptor of net dev */
	struct pcNex *ctSlot;

	if ((ctSlot = allocCTSlot()) != NULL) {
		if ((netDesc = netOpen(netDev, ANY_PORT)) < 0) {
			log("...Can't open net for dossvr; errno %d\n", errno);
			return (struct pcNex *)NULL;
		}
		*toNetDesc = netDesc;
		return ctSlot;
	}
	return (struct pcNex *)NULL;
}


/*
   cleanUp: cleans up before exiting.
*/

void
cleanUp()
{
	register struct pcNex *conScan;

	signal(SIG_CHILD, SIG_IGN);

	for (conScan = conTable; conScan != NULL; conScan = conScan->next) {
		if (conScan->svrPid > 0)
			killSvr(conScan, SIGTERM);
	}
}


/*
   sched: Periodic housekeeping
*/

void sched()
{
	ageConnects();
	if (consvr_here_time > 0 &&
	    (consvr_here_last -= SCHED_INTERVAL) <= 0) {
		consvr_here_last = consvr_here_time;
		toMapSvr(CONSVR_HERE);
	}
	alarm(SCHED_INTERVAL);
}


/*
   ageConnects: Age connection table
*/

void ageConnects()
{
	register struct pcNex *conScan;

	/*
	   Check all connections to determine if any have NOT
	   received PROBE messages within the last period.  If
	   a connection misses two PROBE messages in a row, delete it!
	*/
	for (conScan = conTable; conScan != NULL; conScan = conScan->next) {
		/* Is there a connection on this port? */
		if (conScan->svrPid == 0)
			continue;

		/* Probe timeout reached? */
		if (++conScan->probeCount < PROBE_TIMEOUT)
			continue;

		if (noDisconnect) {
			log("Probe timeout - DON'T disconnect %s\n",
				nAddrFmt((unsigned char *)conScan->pcAddr));
		} else {
			log("Probe timeout - disconnect %s pid %d\n",
				nAddrFmt((unsigned char *)conScan->pcAddr),
				conScan->svrPid);
			killSvr(conScan, SIGTERM);
		}
	}
}


/*
   catchSignal: Signal catcher.
*/

void
catchSignal(sigNum)
register int
	sigNum;
{
int
	exitPid,
	exitStat;
register struct pcNex	*ctSlot;

	/* If this is not the actual consvr, return; this is a pre-exec'ed
	   dossvr */
	if (getpid() != ConSvrPid)
		return;

	switch (sigNum) {
	case SIG_DBG1:
		if (newLogs(CONSVR_LOG, 0, &dosLogCtl, NULL) & CHG_CHILD) {
			if (dosLogArg != NULL)
				sprintf(dosLogArg, "%04x", dosLogCtl);
			log("dossvr logs %x\n", dosLogCtl);
		} else {
			if (logArg != NULL)
				sprintf(logArg, "%04x", dbgEnable);
			log("my logs %x\n", dbgEnable);
		}
		break;

	case SIG_DBG2:
		dosLogCtl ^= 1;
		if (logArg != NULL)
			sprintf(dosLogArg, "%04x", dosLogCtl);
		log("dossvr logs %x\n", dosLogCtl);
		break;

	case SIGALRM:
		sched();
		break;

	case SIG_CHILD:

#ifdef	BERKELEY42
		while ((exitPid = wait3(&exitStat,WNOHANG,0))> 0) {
			if ((ctSlot = getCTSvrPID(exitPid)) == NULL)
				break;
			if (ctSlot->svrPid == exitPid) {
				/* Remove entry from conTable index */
				rmvCTIndex(ctSlot);
				ctSlot->svrPid = 0;
				if(ctSlot->emPid)
					kill(ctSlot->emPid, SIGHUP);
			}
#ifndef	DONT_START_GETTY
			if (ctSlot->emPid == exitPid) {
				rmut(ctSlot->emPty);
				ctSlot->emPid = 0;
				exitStat = 0;
				if ((ctSlot->flags & CTF_EM_CLOSE) &&
				    ctSlot->svrPid)
					kill(ctSlot->svrPid, SIGSYS);
			}
#endif   /* DONT_START_GETTY */
			if (ctSlot->svrPid == 0 && ctSlot->emPid == 0)
				freeCTSlot((struct pcNex *)ctSlot);

			if (exitStat != 0)
				log("%d server exit status %d:%d\n", exitPid,
				((exitStat >> 8) & 0xff), exitStat & 0xff);
		}
#else	/* !BERKELEY42 */
		exitPid = wait(&exitStat);

		if ((ctSlot = getCTSvrPID(exitPid)) == NULL)
			break;
		if (ctSlot->svrPid == exitPid) {
			/* Remove entry from conTable index */
			rmvCTIndex(ctSlot);
			ctSlot->svrPid = 0;
			if(ctSlot->emPid)
				kill(ctSlot->emPid, SIGHUP);
		}
#ifndef	DONT_START_GETTY
		if (ctSlot->emPid == exitPid) {
			rmut(ctSlot->emPty);
			ctSlot->emPid = 0;
			exitStat = 0;
			if ((ctSlot->flags & CTF_EM_CLOSE) &&
			    ctSlot->svrPid)
				kill(ctSlot->svrPid, SIGSYS);
		}
#endif   /* DONT_START_GETTY */
		if (ctSlot->svrPid == 0 && ctSlot->emPid == 0)
			freeCTSlot((struct pcNex *)ctSlot);

		if (exitStat != 0)
			log("%d server exit status %d:%d\n", exitPid,
			((exitStat >> 8) & 0xff), exitStat & 0xff);
#endif	/* !BERKELEY42 */
		break;

	case SIGTERM:
		cleanUp();
		closeShop();
		log("Quit signalled - Bye\n");
		exit(0);
	}

	signal(sigNum, catchSignal);
}

void checkCode()
{
#ifdef	CHECK_CODE
long
	cSum = 0;
long
	*textp;
#ifdef NEVER
extern
	LabelzZ();

asm("LabelzZ:");

	for (textp = (long *)main; textp < (long *)LabelzZ; textp++)
		cSum += *textp;
#define CSUM 0xa54c0e23
#ifdef PRINT_CS
	log("checksum = %lx; want %lx\n", cSum, CSUM);
#else	/* NEVER */
	if (cSum != CSUM)
		exit(1);
#endif	/* !PRINT_CS */
#endif	/* NEVER */
#endif	/* CHECK_CODE */

}


void setnameAddr(myName)
struct nameAddr
	*myName;
{
	char *p;

	static struct hostent  *uName = 0;

	p = myhostname();
	strncpy(myName->name, p, sizeof myName->name);
	if (!uName)
		uName = myhostent();
	nAddrCpy(myName->address, uName->h_addr);
}

#ifndef	DONT_START_GETTY
#	include <utmp.h>
#	if !defined(BERKELEY42) && !defined(AIX) && !defined(SYS5_4)
extern void setutent(),endutent(),pututline();
extern struct utmp *getutline();
#	endif	/* !BERKELEY42 && !AIX && !SYS5_4 */

struct	utmp wtmp,*ut;
char	wtmpf[]	= WTMP_FILE;
char	utmp[] = UTMP_FILE;
#define SCPYN(a, b)	strncpy(a, b, sizeof (a))
#define SCMPN(a, b)	strncmp(a, b, sizeof (a))

#ifndef BERKELEY42
void addut(entry)
char *entry;
{
#ifndef LOCUS
	char *cp;
	int entryLength;
#endif

	log("addut: parameter - %s\n", entry);
	strcpy(wtmp.ut_user, "consvr");
	strcpy(wtmp.ut_line, entry);

#ifndef LOCUS
	/* use the last three bytes of the dev name as the ID field */
	entryLength = strlen(entry);
	if (entryLength > 3)
		cp = entry + entryLength - 3;
	else
		cp = entry;
	strcpy(wtmp.ut_id, cp);

	log("addut: wtmp.ut_id = %s \n", wtmp.ut_id);
#endif
#ifdef LOCUS
	SCPYN(wtmp.ut_host, "PCI");
#endif

	wtmp.ut_pid = getpid();
	wtmp.ut_type = INIT_PROCESS;
	wtmp.ut_time = time((long *) 0);

	pututline(&wtmp);

	endutent();

	return;
}
#endif

void rmut(cleanup)
char *cleanup;
{
	register f;
	int found;
	char pty[20];
#ifndef LOCUS
	char *cp;
	int entryLength;
#endif

	found = 0;
#ifdef	BERKELEY42
	f = open(utmp, 2);
	if (f >= 0) {
		while(read(f, (char *)&wtmp, sizeof (wtmp)) == sizeof (wtmp)) {
			if (SCMPN(wtmp.ut_line, cleanup) || wtmp.ut_name[0]==0)
				continue;
			lseek(f, -(long)sizeof (wtmp), 1);
			SCPYN(wtmp.ut_name, "");
			SCPYN(wtmp.ut_host, "");
			time(&wtmp.ut_time);
			write(f, (char *)&wtmp, sizeof (wtmp));
			found++;
		}
		close(f);
	}
#else	/* !BERKELEY42 */
	strcpy(wtmp.ut_line, cleanup);

#ifndef LOCUS
	/* use the last three bytes of the dev name as the ID field */
	entryLength = strlen(cleanup);
	if (entryLength > 3)
		cp = cleanup + entryLength - 3;
	else
		cp = cleanup;
	strcpy(wtmp.ut_id, cp);
#endif


	wtmp.ut_type = LOGIN_PROCESS;
	setutent();
	if ((ut = getutline(&wtmp)) != (struct utmp *) NULL) {
		time(&(ut->ut_time));
#if !defined(SYS5) || defined(LOCUS)
		SCPYN(ut->ut_host, "");
#endif	/* !SYS5 */
		SCPYN(ut->ut_user, "");

		ut->ut_type = DEAD_PROCESS;

		found++;
		wtmp = *ut;
		pututline(&wtmp);
	}
	endutent();

#endif	/* BERKELEY42 */

	if (found) {
#ifdef	BERKELEY42
		f = open(wtmpf, 1);
#else	/* !BERKELEY42 */
		f = open(wtmpf, O_WRONLY | O_APPEND);
#endif	/* BERKELEY42 */
		if (f >= 0) {
			SCPYN(wtmp.ut_line, cleanup);
			SCPYN(wtmp.ut_name, "");
#ifndef SYS5
			SCPYN(wtmp.ut_host, "");
#endif	/* !SYS5 */
			time(&wtmp.ut_time);
			lseek(f, (long)0, 2);
			write(f, (char *)&wtmp, sizeof (wtmp));
			close(f);
		}
	}
	strcpy(pty, "/dev/");
	strcat(pty, cleanup);
	chmod(pty, 0666);
	chown(pty, 0, 0);
	pty[0] = 'p';
	chmod(pty, 0666);
	chown(pty, 0, 0);

}

#define	ARGVSIZE	20	/* 19 args to getty, plus null arg terminator */

void s_login(in, out, flip)
struct input *in;
struct output *out;
{
	register struct pcNex *ctSlot;
	register int svrPid, i;
	char *argv[ARGVSIZE];		/* args for execv() call on getty */
	char *argp;
	char *loginProg;
	char stdio[30];
	int std_in;
#ifdef	BERKELEY42
	struct sgttyb b;
#else
	struct termio b;
#endif
#if !defined(LOCUS) && !defined(SYS5)
	int tt;
#endif

#if	!defined(EXL316) && !defined(AIX_RT)
#if defined(CCI) || defined(BERKELEY42)
	static int on = 0;
#else
	static int on = 1;
#endif
#endif	/* !EXL316 and !AIX_RT */

	static char whitestr[] = " \t";		/* whitespace */

	svrPid = in->hdr.offset;

	out->hdr.res = SUCCESS;
	out->hdr.stat = NEW;
	out->hdr.req = S_LOGIN;

	/* Look up conTable slot by consvr PID */
	if ((ctSlot = getCTSvrPID(svrPid)) == NULL) {
		/* Send error return */
		log("s_login: dossvr unknown\n");
		out->hdr.res = LOGIN_FAILED;
		xmtPacket(out, &myHeader, flip);
		return;
	}

	if(ctSlot->emPid) {
		/* Send successful return */
		log("s_login: login already exists\n");
		xmtPacket(out, &myHeader, flip);
		return;
	}

	strcpy(ctSlot->emPty, in->text);
	if (loginProg = strtok(&in->text[strlen(ctSlot->emPty)+1], whitestr)) {
		/* build argument array for execv() call */
		i = 0;
		if (argp = strrchr(loginProg, '/'))
			argp++;
		else
			argp = loginProg;
		while (argp && i < ARGVSIZE-1) {
			argv[i++] = argp;
			argp = strtok(NULL, whitestr);
		}
		argv[i] = NULL;
	} else {
		/* Send error return */
		log("s_login: bad getty command string\n");
		out->hdr.res = LOGIN_FAILED;
		xmtPacket(out, &myHeader, flip);
		return;
	}

	if(ctSlot->emPid = fork()) {	/* Parent! */
		if(ctSlot->emPid < 0) {
			/* Send error return */
			log("s_login: Can't fork login process\n");
			out->hdr.res = LOGIN_FAILED;
			xmtPacket(out, &myHeader, flip);
			return;
		}
		/* Don't return a packet, let child do it */
		return;
	} else {	/***** CHILD PROCESS *****/

		/* Cancel any alarms */
		alarm(0);

		if (access(loginProg, 1) < 0) {
			/* Send error return */
			log("s_login: can't execute \"%s\"\n", loginProg);
			out->hdr.res = LOGIN_FAILED;
			xmtPacket(out, &myHeader, flip);
			exit(255);
		}

#ifndef BERKELEY42
		addut(ctSlot->emPty);
#endif

		/* Construct name of tty */
		if (ctSlot->emPty[0] == '/')
			strcpy(stdio, ctSlot->emPty);
		else {
			strcpy(stdio, "/dev/");
			strcat(stdio, ctSlot->emPty);
		}

		close(0);
		close(1);
		close(2);

#if	defined(LOCUS)
		setsid();
#elif	defined(SYS5)
		setpgrp();
#else
		setpgrp(0, getpid());
		/* disassociate us from any controlling tty */
		if ((tt = open("/dev/tty", 2)) >= 0)
		{
			ioctl(tt, TIOCNOTTY, 0);
			close(tt);
		}
#endif

		/* getty re-opens stdin, stdout and stderr, and sets up	*/
		/* tty modes, so we don't need to do it		*/

		if (strcmp(argv[0], "getty")) {
			if ((std_in = open(stdio, 2)) < 0) {
				/* Send error return */
				log("open of %s failed, errno %d\n", stdio, errno);
				out->hdr.res = LOGIN_FAILED;
				xmtPacket(out, &myHeader, flip);
				exit(255);
			}
#ifdef	BERKELEY42
			ioctl(std_in,TIOCGETP,&b);
			b.sg_flags = CRMOD|XTABS|ANYP|ECHO;
			ioctl(std_in,TIOCSETP,&b);
#else	/* !BERKELEY42 */
			ioctl(std_in,TCGETA,&b);
			b.c_iflag &= ~(INLCR | ICRNL | BRKINT);
			b.c_iflag |= IXON;
			b.c_oflag |= OPOST;
			b.c_oflag &= ~(OLCUC | ONLCR | OCRNL | ONOCR | ONLRET);
			b.c_lflag &= ~(ICANON | ECHO | ISIG);
			b.c_cc[VMIN] 	 = '\01';
			b.c_cc[VTIME] 	 = '\0';
			ioctl(std_in,TCSETAW,&b);
#endif	/* !BERKELEY42 */
#if	!defined(EXL316) && !defined(AIX_RT)
			ioctl(std_in, FIONBIO, &on);
#ifdef BERKELEY42
			ioctl(std_in, FIOASYNC, &on);
#endif
#endif	/* !EXL316 and !AIX_RT */
			dup(0);
			dup(0);
		}

		/* Send success packet */
		out->hdr.offset = getpid();
		xmtPacket(out, &myHeader, flip);

		/* close all file descriptors above stderr */
		for (i = 3; i < uMaxDescriptors(); i++)
			close(i);

		execv(loginProg, argv);

		_exit(errno);
	}
}

void k_login(in, out, flip)
struct input *in;
register struct output *out;
{
	register struct pcNex *ctSlot;

	/* Build success response */
	out->hdr.res = SUCCESS;
	out->hdr.stat = NEW;
	out->hdr.req = K_LOGIN;

	/* Get connection table slot */
	if ((ctSlot = getCTSvrPID(in->hdr.offset)) == NULL)
		out->hdr.res = LOGIN_FAILED;
	else
		if(ctSlot->emPid == 0)
			/* Dosout already gone - return success */
			log("k_login: login already killed\n");
		else
			/* Dsoout is running - kill it & return success */
			kill(ctSlot->emPid, SIGHUP);

	/* Send reply */
	xmtPacket(out, &myHeader, flip);
}
#endif   /* DONT_START_GETTY */


void
killSvr(ctSlot, sigNum)
struct pcNex		*ctSlot;	/* conTable slot of server to kill */
int			sigNum;		/* Signal to send */
{
int			emPID;		/* Stable copy of dosout PID */
int			svrPID;		/* Ditto for consvr */

	/* Remove conTable slot from index so it won't be seen any longer */
	rmvCTIndex(ctSlot);

	/* Kill dossvr process */
	if ((svrPID = ctSlot->svrPid) > 0)
		kill(svrPID, sigNum);

	/* Kill em process, if it exists */
	if ((emPID = ctSlot->emPid) > 0)
		kill(emPID, SIGHUP);

	/* Don't let this go unnoticed */
	log("SIG%d to %d; SIG%d to %d\n", sigNum, svrPID, SIGHUP, emPID);
}



/* ----- Connection Table Index Functions ----- */

/*
   addCTIndex:			Index a new conTable entry
   rmvCTIndex:			Remove a conTable entry from index
   getCTAddr:			Lookp up a conTable entry by client address
   getCTSerial:			Look up an entry by serial number
   getCTSvrPID:			Look up an entry by consvr PID
*/


void
addCTIndex(ctSlot)
struct pcNex *ctSlot;	/* Index this conTable slot */
{
	struct sockaddr_in *sa;
	int hashIdx;

	sa = (struct sockaddr_in *)ctSlot->pcAddr;
	hashIdx = ntohl (sa->sin_addr.s_addr) & 0xff;
	log ("addCTIndex: hash = %d\n", hashIdx);

	/* Add slot to hash list */
	ctSlot->nextHash = conHash[hashIdx];
	conHash[hashIdx] = ctSlot;
}


void
rmvCTIndex(ctSlot)
struct pcNex		*ctSlot;	/* Remove this slot from index */
{
	register struct pcNex *ct;
	struct sockaddr_in *sa;
	int hashIdx;

	sa = (struct sockaddr_in *)ctSlot->pcAddr;
	hashIdx = ntohl (sa->sin_addr.s_addr) & 0xff;
	log ("rmvCTIndex: hash = %d\n", hashIdx);

	/* Remove the entry from the indexes */
	if (conHash[hashIdx] == ctSlot)
		conHash[hashIdx] = ctSlot->nextHash;
	else {
		for (ct = conHash[hashIdx]; ct != NULL; ct = ct->nextHash) {
			if (ct->nextHash == ctSlot) {
				ct->nextHash = ctSlot->nextHash;
				break;
			}
		}
	}
	ctSlot->nextHash = NULL;
}


struct pcNex *
getCTAddr(netAddr, conNum)
struct sockaddr_in *netAddr;	/* Look up connection on this address */
int conNum;
{
	register struct pcNex *ct;
	int hashIdx;

	hashIdx = ntohl (netAddr->sin_addr.s_addr) & 0xff;
	log ("getCTAddr: hash = %d\n", hashIdx);

	/* Find the entry from the indexes */
	for (ct = conHash[hashIdx]; ct != NULL; ct = ct->nextHash) {
		if (nAddrEq(ct->pcAddr, netAddr) && conNum == ct->conNumber)
			return ct;
	}
	return (struct pcNex *)NULL;
}


struct pcNex *
getCTSerial(serNum)
register SERI_ST *serNum;	/* Search for this net address */
{
	register struct pcNex *conScan;	/* Scan current connection table */

	/* Look for an existing connection with this serial number */
	for (conScan = conTable; conScan != NULL; conScan = conScan->next) {
		/* Does serial number match? */
		if (serialEq(conScan->pcSerial, serNum))
			return conScan;
	}
	return (struct pcNex *)NULL;
}


struct pcNex *
getCTSvrPID(svrPID)
register int svrPID;		/* Search for this net address */
{
	register struct pcNex *conScan;	/* Scan current connection table */

	/* Look for an existing connection with this serial number */
	for (conScan = conTable; conScan != NULL; conScan = conScan->next) {
		if (conScan->svrPid == svrPID || conScan->emPid == svrPID)
			return conScan;
	}

	return (struct pcNex *)NULL;
}



/* ----- Connection Table Allocation and De-allocation ----- */

/*
   initConTable:		Initialize conTable[]
*/

void
initConTable()
{
	register int i;

	conTable = NULL;
	conFree = NULL;
	for (i = 0; i < 256; i++)
		conHash[i] = NULL;
}


/*
   allocCTSlot:	Allocate a free conTable entry to caller
*/

struct pcNex *
allocCTSlot()
{
	struct pcNex *freeSlot;	/* Newly allocated entry */
	static int max_entries = 0;

	/* Take first free slot on list... */
	if ((freeSlot = conFree) == NULL) {
		freeSlot = (struct pcNex *)calloc(1, sizeof(struct pcNex));
		if (freeSlot == NULL)
			fatal("consvr: Can't allocate connection table entry\n");
		log("allocCTSlot: max entries = %d\n", ++max_entries);
	} else
		conFree = freeSlot->next;

	/* Return pointer to allocated slot */
	memset((char *)freeSlot, 0, sizeof (pcNex));

	/* Put slot on active list */
	freeSlot->next = conTable;
	conTable = freeSlot;
	return freeSlot;
}


/*
   freeCTSlot:		Release a no-longer-used conTable entry
*/

void
freeCTSlot(freeSlot)
struct pcNex *freeSlot;		/* Slot to de-allocate */
{
	register struct pcNex *ct;

	/* Remove from hash list */
	rmvCTIndex(freeSlot);

	/* Remove from conTable list */
	if (freeSlot == conTable)
		conTable = freeSlot->next;
	else {
		for (ct = conTable; ct != NULL; ct = ct->next) {
			if (ct->next == freeSlot) {
				ct->next = freeSlot->next;
				break;
			}
		}
	}

	/* Clear out old conTable slot */
	memset((char *)freeSlot, 0, sizeof *freeSlot);

	/* Put old slot on free list */
	freeSlot->next = conFree;
	conFree = freeSlot;
}
