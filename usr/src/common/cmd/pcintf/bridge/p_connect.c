#ident	"@(#)pcintf:bridge/p_connect.c	1.1.1.4"
#include	"sccs.h"
SCCSID(@(#)p_connect.c	6.18	LCC);	/* Modified: 11:14:43 12/3/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

/* History:
   [1/26/88 PD] Peter Dobson - corrected log message to print out correct
	values.
   [2/26/88 PD] added code to make RS232 em recieve loop exit if DOSOUT program
	has quit.
   [3/9/90 SCAHNG] made DOSSVR eat the ACK from PC BRIDGE, if emulation session
		has gone. 
   [3/23/90 SCHANG] made environment variables HOME, LOGNAME & USER of DOSSVR
 	to be same as the values defined in /etc/passwd file.
*/

/*			 PC-INTERFACE  Configuration Manager		*/

#include "sysconfig.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <pwd.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#if defined(STREAMS_PT) && defined(RS232PCI)
#	include <stropts.h>
#endif

#if defined(SecureWare)
#	include <sys/security.h>
#	include <sys/audit.h>
#	undef hdr		/* defined in <sys/audit.h> */
#	include <prot.h>
#elif defined(SHADOW_PASSWD)
#	include	<shadow.h>
#endif

#ifdef LOCUS
#include	<uinfo.h>
#endif /* LOCUS */

#ifdef	RIDGE
#include	<bsdioctl.h>
#endif	/* RIDGE */

#ifdef RS232PCI
#ifdef	XENIX
#include	<ioctl.h>
#endif	  /* XENIX  */
#include	<termio.h>		/* TTY structs/constants */
#endif   /* RS232PCI  */

#ifdef RLOCK      /* record locking */
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <rlock.h>			/* record locking defs */
#endif /* RLOCK */

#include <lmf.h>

#include "pci_proto.h"
#include "pci_types.h"
#include "common.h"
#include "const.h"
#include "dossvr.h"
#include "flip.h"
#include "log.h"

#ifdef	LOCUS	/* Locus site info stuff */
#include	"pw_gecos.h"
#endif	/* LOCUS */

#define	TI_CHECK_INTERVAL	5	/* log a check every 5 packets */
#define	TI_DUP_WINDOW		128	/* size of the window to forget about dups */

#define	pcDbg(dbgArgs)	debug(0x40, dbgArgs)

void		main			PROTO((int, char **));
LOCAL int	getVers			PROTO((char *));
LOCAL void	sendMap			PROTO((struct output *));
LOCAL void	setnameAddr		PROTO((struct nameAddr *));
LOCAL void	do_connect_1		PROTO((void));
LOCAL void	do_connect_2		PROTO((void));
LOCAL void	updateenviron		PROTO((char *, char *));

#if defined(RESTRICT_USER)
LOCAL int	restrictUser		PROTO((char *));
#endif	  /* RESTRICT_USER  */

#if defined(RS232PCI)
LOCAL void	dosSvrAlive		PROTO((int));
extern int	get_tty			PROTO((int, struct termio *));
extern int	set_tty			PROTO((int, struct termio *));
extern void	rawify_tty		PROTO((struct termio *));
#endif    /* RS232PCI  */

#if defined(UDP42)
extern void		localhostaddr	PROTO((struct sockaddr_in *));
extern struct hostent	*myhostent	PROTO((void));
#endif	/* UDP42 */

#if defined(LOCUS)
LOCAL void		get_valid_sites	PROTO((char *));
LOCAL int		get_valid_shell	PROTO((char *));
extern struct pw_gecos	*parseGecos	PROTO((char *));
#endif     /* LOCUS   */

#if defined(STREAMS_PT)
extern int		grantpt		PROTO((int));
extern int		unlockpt	PROTO((int));
extern char		*ptsname	PROTO((int));
#endif

extern void	pckframe	PROTO((struct output *, int, int, unsigned char, int, int, int, int, int, int, int, int, int, int, struct stat *));

extern char	*crypt		PROTO((char *, char *));

extern	int
	    rd_flag;			/* reliable delivery? true or false */

#if	!defined(RS232PCI)

static	int rd_rec_data,		/* If true RDR will receive data */
	    semaphore,			/* reliable delivery semaphore id */
	    stream_size;		/* stream size sent from PC */

static	struct rd_shared_mem
	    *sh_mem;			/* pointer to shared memory segment  */

static shmid_t	rd_shmid;		/* reliable delivery shared mem id */

#endif	    /* Xenix or Eth3bnet - reliable delivery stuff   */


char	*mydevname=NULL;		/* name of device to talk on */
extern char *ttyname();

#if defined(LOCUS) && defined(ETHNETPCI)
static char 
		txperms[MAXSITE + 1];
#endif     /* LOCUS && ETHNETPCI */

extern	int	print_desc[NPRINT];	/* print file descriptors */
extern	char	*print_name[NPRINT];	/* print file names */

int	
	    bridge = INITIALIZED,	/* Indicates bridge access is active */
	    swap_how,			/* Byte order for auto-sense flipping */
	    emulator = INITIALIZED,	/* Indicates term emulation is active */
	    versNum,			/* Version number of this server */
	    stopGate = 0,               /* Reentry gate for stopService */

	    netdesc = -1,		/* File descriptor of net device */
	    pipedesc,			/* File descriptor of control pipe */
	    ptydesc,			/* File descriptor of pseudo TTY */
	    length,			/* Length of last configuration frame */
	    accepted = -1,		/* Number bytes accepted on PTY write */
	    files_open,			/* Number of files open on Bridge PC */
	    connseqnum,			/* seq number for fake connect */
	    preconnect = 0,		/* flag for fake connect */
	    ti_seqnum,			/* Last emulator input seq number */
	ti_check,			/* counter to check input characters */
	    termoutpid,			/* Id of terminal output process */
	    descriptors[2];		/* File descriptors associated w/pipe */
int	oldserial = 1;			/* nonzero->use old algorithm */

unsigned char	brg_seqnum = 0xff;	/* last bridge frame sequence number */

#ifdef	RS232PCI
int	noDisconnect=1;			/* Don't disconnect on probe timeout */
#else	/* RS232PCI */
int	noDisconnect=0;			/* Disconnect on probe timeout */
#endif	/* RS232PCI */

char	
	    cwd[MAX_CWD],		/* Contains current working directory */
	    ptydevice[MAX_FN_TOTAL+1],	/* Contains line in /etc/brgptys file */
	    cntrlpty[MAX_FN_TOTAL + 1],	/* Name of controlling PTY device */
#if defined(AIX_RT)
	    slvpty[MAX_FN_TOTAL + 1],	/* Name of slave PTY device */
#else
	    *slvpty,			/* Name of slave PTY device */
#endif
	    copyright[] =		"PC BRIDGE TO UNIX FILE SERVER\
COPYRIGHT (C) 1984, LOCUS COMPUTING CORPORATION.  ALL RIGHTS RESERVED.  USE OF\
THIS SOFTWARE IS GRANTED ONLY UNDER LICENSE FROM LOCUS COMPUTING CORPORATION.\
ANY UNAUTHORIZED USE IS STRICTLY PROHIBITED.";

struct  ni2 ndata;                      /* Ethernet device header structure */
					/* (dummy struct when not ethernet) */
#ifdef ETHNETPCI
struct  ni2 ntmp;                       /* Ethernet device header structure */

#if defined(UDP42) || defined(UDP41C)
static struct ni2 conSvrHeader;
#endif /* UDP42 || UDP41C  */

#endif	  /* ETHNETPCI  */

#ifdef RS232PCI
struct	termio
	    ttysave,			/* Saves TTY modes for later */
	    ttymodes;			/* TTY modes structure */

#ifdef RS232_7BIT
int
	    using_7_bits = 0;		/* Serial line only supports 7 bits */
#endif /* RS232_7BIT */
#endif   /* RS232PCI  */

static struct	input				/* Input buffer for RS-232 */
#if defined(UDP42) || defined(UDP41C)
	    cibuf,			/* Input buffer for Consvr comm */
#endif /* UDP42  || UDP41C */
	    in;

static struct	output
#if defined(UDP42) || defined(UDP41C)
	    conbuf,			/* Output buffer for Consvr comm */
#endif /* UDP42 || UDP41C  */
	    out;			/* Output buffer for RS-232 */

#if defined(UDP42) || defined(UDP41C)
static long login_pid;
#endif /* UDP42 || UDP41C  */

char
	*logStr;			/* Log level string (shows in ps) */


#define	NULLstat	(struct stat *)0

/* problem in excelan code not returning the correct family class
   after a receive. The START parameter is used in the address compare
   after a receive takes place in the main loop.
*/
#ifdef XLN_BUG
#define START 4 
#else
#define START 0
#endif /* XLN_BUG */

/*
 *  main()  arguments:  (any order)
  -ny   ethernet network descriptor y.        : only when ETHNETPCI
  -Dn   debug level n          default 0
  -tdev talk on file "dev"
#ifdef RS232PCI
  -7	use only 7 bits in file protocol
#endif
 */

int  savePid;

struct passwd
	*pptr;			/* Pointer to entry in password file */

#if !defined(RS232PCI)
#	if defined(SecureWare)

struct pr_passwd *prptr = NULL;

#	elif defined(SHADOW_PASSWD)

struct spwd *spptr = NULL;	/* pointer to shadow password entry */

int sVr3_1 = 0;	/* true if on SVR3.1 or later, i.e. /etc/shadow exists */

#	endif
#endif

char *myname;

void main(argc, argv)
int argc;
char *argv[];
{
#if defined(RS232PCI) || defined(STREAMS_PT)
    int		status;		/* return value from library/system call */
#endif
#if defined(RS232PCI)
    char	ptybuf[10];	/* Character buffer for terminal emulation */
#   if defined(STREAMS_PT)
    char	*shell_path;	/* user's login shell */
    char	*shell_name;	/* last component of user's login shell */
    char	*shell_env[6];
    char	shell_arg0[20];
    char	env_buffer[6][120];
    int		stdin_fd;	/* stdin for user's login shell process */
#   endif
#else
    char *pw_attempt;	/* password from bridge before encryption */
    char *cryptPass;	/* password from bridge after encryption */
    char *password;	/* password from /etc/passwd or /etc/shadow */
#   if defined(SecureWare)
    int savemask;	/* to restore umask after set_auth_parameters() call */
#   endif

#   if defined(SHADOW_PASSWD)
    struct stat statbuf;	/* dummy buffer for stat system call */
#   endif	/* SHADOW_PASSWD */

    struct emhead
        *rcvhead = (struct emhead *) in.text;
    struct emhead
	*sndhead = (struct emhead *) out.text;
    struct ackcontrol
	control;                /* RD: communication structure onto PIPE */
    int send_ack,               /* RD: need to send ack local protocol   */
	accept,                 /* RD: accpet data f;ag for xmt to pty   */
	old_state = 0;          /* RD: old state protocol was in         */
    unsigned short
        sequence;               /* RD:temp variable                      */
    char    
 	loginname[129];		/* temporary storage space for login name */
#endif	/* !RS232PCI */

    register int 
	len,			/* Temporary length of string */
	i;			/* Loop counter for loading source addresses */

    int
	savePid;

    char
	c;			/* Character for writing onto PIPE */

    FILE 
	*fptr;			/* File pointer into PTY configuration table */

    char
	*ptr,			/* Pointer to character array */
	*ptyGetty = NULL,	/* Pointer to getty command from pciptys */
	*txtptr,		/* Pointer into input text buffer */
	fname[120];             /* a file name */

char
	errArg[8],		/* Error descriptor */
	pipeArg[8],		/* Ack pipe descriptor */
	ptyArg[8],		/* Pty descriptor */
	debugArg[8],		/* Debug level */
#if defined(ETHNETPCI)
	rdArg[15],		/* reliable delivery */
	ssArg[15],		/* stream size (RD only) */
	naddrArg[32],		/* Network address (eth) */
#endif	/* ETHNETPCI */
	swapArg[8],		/* Byte swapping code */
	descArg[8];		/* Network (eth or 232) descriptor */
int
	argN;			/* Current argument number */
char
	*arg;			/* Current argument */

#ifdef VERSION_MATCHING
char	*version_number = NULL;	/* version number from consvr */
#endif

#ifdef	RIDGE
int off = 0;
#endif	/* RIDGE */

#ifdef XENIX
struct stat sig_buf;
#endif /* XENIX */

#if defined(UDP42) || defined(UDP41C)
struct sockaddr_in vPort;
#endif /* UDP42 || UDP41C  */

	/* determine machine byte ordering */
	byteorder_init();

#if defined(SHADOW_PASSWD) && defined(ETHNETPCI)
	sVr3_1 = !stat("/etc/shadow", &statbuf);    /* determine system level */
#endif
	myname = argv[0];
	if (*myname == '\0')
		myname = "unknown";

	/* Extract version number of this server from its name */
	versNum = getVers(argv[0]);

	/* Decode arguments */
	for (argN = 1; argN < argc; argN++) {
		arg = argv[argN];

		if (*arg != '-')
			continue;

		switch (arg[1]) {
		case 'D':		/* Log level */
			logStr = &arg[2];
			dbgSet(strtol(logStr, (char **)NULL, 16));
			break;

		case 'n':		/* Network descriptor */
			netdesc = atoi(&arg[2]);
			log("netdesc on %d\n",netdesc);
			break;

		case 't':
			mydevname = arg+2;
			break;

		case '-':
			break;	/* dummy placeholder */
		case 'x':
			log("probe timeouts are disabled\n");
			noDisconnect = 1;
			break;
		case 'P':
			pipedesc = atoi(&arg[2]);
			log("pipedesc is %d\n",pipedesc);
			if (read(pipedesc,&in,sizeof in) != sizeof in) {
				fatal(lmf_format_string((char *) NULL, 0, 
					lmf_get_message("P_CONNECT1",
					"error reading conninfo pipe:%1\n"), 
					"%d", errno));
			}
			close(pipedesc);
			log("about to inputswap\n");
			swap_how = input_swap(&in, in.hdr.pattern);
			brg_seqnum = in.hdr.seq;
			preconnect = 1;
			break;

#ifdef VERSION_MATCHING
		case 'V':		/* Version number from consvr */
			version_number = &arg[2];
			break;
#endif
#if defined(RS232PCI) && defined(RS232_7BIT)
		case '7':
			using_7_bits++;
			break;
#endif /* RS232PCI && RS232_7BIT */
		}
	}

	/* Initialize print file table */
	for (i = 0; i < NPRINT; i++) {
		print_desc[i] = -1;
		print_name[i] = NULL;
	}

	if (dbgCheck(~0)) {
		logOpen(DOSSVR_LOG, getpid());
		ulog("Debug channels = 0x%x\n", dbgEnable);
	}

#ifdef	SIGXFSZ				/* Ignore the execeed ulimit signal */
    signal(SIGXFSZ, SIG_IGN);		/* because errno will be set to EFBIG */
#endif	/* SIGXFSZ */
    signal(SIGALRM, sig_catcher);	/* Catch the alarm signal. Someone is */
					/* sending this signal which kills */
					/* the dossvr. [ WU 6/6/88 ] */
    signal(SIGTERM, sig_catcher);	/* Catch signal from pcidaemon */
#if	defined(XENIX) && defined(RS232PCI)
    signal(SIGINT, SIG_IGN);		/* Ignore signal from tty (for now) */
#else
    signal(SIGINT, sig_catcher);	/* Catch signal from tty (break) */
#endif
    signal(SIGHUP, sig_catcher);	/* Catch signal from tty (disconnect) */
    signal(SIG_DBG1, sig_catcher);      /* Catch signal to toggle logs */
    signal(SIG_DBG2, sig_catcher);      /* Same */
    signal(SIG_CHILD, sig_catcher);	/* Catch signal if pcitermout dies */
    signal(SIGSYS, sig_catcher);	/* Catch signal when em shell dies */

#ifdef	SYS5
    /* Set up my own process group so later when I do a kill(0,15) it won't
       kill other DOSSVR's  */ 
    setpgrp();
#endif	  /* SYS5  */

	/* initialize NLS */
	nls_init();

	/* initialize translation tables */
	table_init();

/* 
 * Configure ethernet device (port/descriptor inherited from daemon).
 */

#ifdef ETHNETPCI

#if defined(UDP42) || defined(UDP41C)


	/* get our own address into vPort.sin_addr.s_addr */
	localhostaddr(&vPort);
	vPort.sin_family = AF_INET;
	vPort.sin_port = htons(PCI_CONSVR_PORT);

	/* Save ethernet source address, type, and port for output frames */
	nAddrCpy(conSvrHeader.dst, &vPort);

#else /* !UDP42 || !UDP41C  */

    while ((ioctl(netdesc, NIGETA, &lp)) < 0) {
	if (errno == EINTR)
	    continue;
	fatal(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("P_CONNECT4","Error %1 on NIGETA; Bye!\n"),
		"%d", errno));
    }

/* 
 * Set-up desired configuration in status structure for ioctl.
 */
    lp.rcvb_sz = MAX_FRAME;			/* Maximum packet size */
    lp.rcvq_sz = 2;                     /* Number of buffers */

    if (ioctl(netdesc, NISETA, &lp) < 0)
	fatal(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("P_CONNECT5","Error %1 on NISETA; Bye!\n"),
		"%d" , errno));

#endif	/* UDP42 || UDP41C  */

    netUse(netdesc);

#endif   /* ETHNETPCI  */

#ifdef RS232PCI

    if (netdesc == -1) netdesc = STDIN;
    if (get_tty(netdesc, &ttysave) < 0)
	fatal(lmf_format_string((char *) NULL, 0,
		lmf_get_message("P_CONNECT6","Error %1 on get_tty\n"),
		"%d", errno));

    ttymodes = ttysave;

    if(!mydevname) mydevname = ttyname(netdesc);
/* Set up TTY driver for eight bit "raw" mode */
    rawify_tty(&ttymodes);

    if (set_tty(netdesc, &ttymodes) < 0)
	fatal(lmf_format_string((char *) NULL, 0,
		lmf_get_message("P_CONNECT7","Error %1 on STDIN set_tty\n"),
		"%d", errno));

    /* Give network (tty) descriptor to network interface */
    netUse(netdesc);

	/* Get current working directory from password file */
	    if ((pptr = getpwuid(getuid())) == (struct passwd *)NULL)
		fatal(lmf_get_message("P_CONNECT8",
			"Error getting current working directory\n"));

#   if defined(STREAMS_PT)
	    shell_path = pptr->pw_shell;	/* save login shell for later */
#   endif

	    /* Start bridge in home directory */
	    if (chdir(pptr->pw_dir) < 0) {
		dosSvrAlive(LOGIN_FAILED);
		stopService(1,0);
	    }
	    if (!preconnect) dosSvrAlive(SUCCESS);

    strcpy(cwd, pptr->pw_dir);

    bridge = RUNNING;

#ifdef	XENIX
if  (stat("/usr/pci/config.sig", &sig_buf) != 0) 
    signal(SIGINT, sig_catcher);	/* now catch breaks from tty */
#endif

#endif   /* RS232PCI  */


    /* Initialize virtual file descriptor cache */
    vfInit();

#ifdef RLOCK    /* Record locking from Starlan */

	/* Attach to the shared table segments and the server lock set */
	log("p_connect.c: Attaching to share table segment.\n");
	if (rlockAttach()) {
		log("p_connect.c: can't initialize, %s\n",
				rlockEString(rlockErr));
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("P_CONNECT9",
			"Can't initialize record lock data, %1 \n"),
			"%s", rlockEString(rlockErr)));
	}

#endif /* RLOCK */

	if (preconnect) {
		log("preconnect,do_connect\n");
		do_connect_2();
	}

/*
 * Service configuration requests ,bridge packets,
 * or terminal emulator packets.
 */ 
     for (;;)
     {
getrequest:
	swap_how = rcvPacket(&in);


#ifdef ETHNETPCI

    /* Store destination address of PC */


	if (bridge == INITIALIZED) {
	    for (i = 0; i < DST; i++)
		ndata.dst[i] = in.net.src[i];
	}
	else {

	/* Only the connected PC may use this port! */
	    for (i = START; i < START /*DST-1*/; i++) { /* HOOK */
		if (ndata.dst[i] != in.net.src[i]) {
		    log("Bad source %s\n",
			nAddrFmt((unsigned char *)in.net.src));
		    pckframe(&out, CONF, (int)in.hdr.seq, in.hdr.req, NEW,
			INVALID_FUNCTION, NO_DES, NO_CNT, NO_CNT, NO_MODE,
			NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
		    goto getrequest;
		}
	    }
	}

#endif   /* ETHNETPCI  */

/*
 * Retransmit previous response upon receipt of previous sequence number.
 */
	if (in.pre.reset) {
	    brg_seqnum = 0;
	    ti_seqnum  = 0;
	ti_check = 0;
	}
	else if ((in.pre.select == CONF || in.pre.select == DAEMON)
	     &&  in.hdr.seq == brg_seqnum)
	{
	    log("reXmt due to seqnum\n");
	    reXmt(&out, length);
	    goto getrequest;
	}

	switch (in.pre.select) {
	    /* Respond to some messages normally handled by other servers */
	    case DAEMON:
		if (in.hdr.req == CONNECT) {
		    do_connect_1();
		    /* NOTREACHED:  restarts with correct server */
		}
		else if (in.hdr.req == SEND_MAP)
		    sendMap(&out);
		else if (in.hdr.req == DISCONNECT)
		    stopService(2,0);
		else if (in.hdr.req == PROBE) {
			break;	/* no response for this one */
		}
		else
		    break;

		length = xmtPacket(&out, &ndata, swap_how);
		break;

    /*
     * Request to establish PC-DOS Bridge session (only in ethernet mode.)
     */
	    case CONF:

#if	defined(ETHNETPCI)

		if (in.hdr.req == EST_BRIDGE) {
		    brg_seqnum = in.hdr.seq;
		    if (bridge == RUNNING) {
			log("Dup bridge req\n");
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				DUPLICATE_CONNECTION, NO_DES, NO_CNT, NO_CNT,
				NO_MODE, NO_SIZE, NO_OFF, NO_ATTR, NO_DATE,
				NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			break;
		    }

		    log("Establish bridge\n");

		    /* save the login name for later use */
		    strncpy(loginname, in.text, 128);

		/* Execute login sequence: find entry from passwd file */
#if defined(SecureWare)
		    umask(savemask = umask(2));
		    set_auth_parameters(argc, argv);
		    umask(savemask);
#endif
		    pw_attempt = in.text + in.hdr.b_cnt;
		    if (
#ifndef	ALLOW_ROOT_LOGIN
				(strcmp(in.text, "root")) == 0 ||
#endif	/* ALLOW_ROOT_LOGIN */
				!(pptr = getpwnam(in.text))
#if defined(SecureWare)
				|| !(prptr = getprpwnam(in.text))
#elif defined(SHADOW_PASSWD)
				|| (sVr3_1 && !(spptr = getspnam(in.text)))
#endif
				) {
#if defined(SecureWare)
			cryptPass = bigcrypt(pw_attempt, "//");
#else
			cryptPass = crypt(pw_attempt, "//");
#endif
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			break;
		    }

		/* Match password */
#if defined(SecureWare)
		    password = prptr->ufld.fd_encrypt;
#elif defined(SHADOW_PASSWD)
		    password = (sVr3_1) ? spptr->sp_pwdp : pptr->pw_passwd;
#else
		    password = pptr->pw_passwd;
#endif

		    if (strlen(password) == 0) {
			if (strlen(pw_attempt) == 0)
			    goto good_passwd;
			else
			    goto bad_passwd;
		    }

#if defined(SecureWare)
		    cryptPass = bigcrypt(pw_attempt, password);
#else
		    cryptPass = crypt(pw_attempt, password);
#endif
	   	    if (!strcmp(cryptPass, password))
		    	goto good_passwd;
bad_passwd:
		pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW, LOGIN_FAILED,
			NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE, NO_OFF,
			NO_ATTR, NO_DATE, NULLstat);
		length = xmtPacket(&out, &ndata, swap_how);
		break;
good_passwd:
#ifdef	RESTRICT_USER
		/* Check RESTRICTFILE for this username, and disallow login
		   if name is found */
		if (restrictUser(in.text))
			goto bad_passwd;
#endif	  /* RESTRICT_USER  */
#ifdef LOCUS	/* Check site permissions and fail if user is not allowed */
		/* to login to this site.  Should have better error codes */
		/* that explain the cause of failure.			  */

		get_valid_sites(txperms);
		if (txperms[site(0L)] == 0)  {	/* must have 0L for xenix */
		    pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW, 
		    	LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE, 
		    	NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
		    length = xmtPacket(&out, &ndata, swap_how);
		    break;
		}
		if (setxperm(txperms, MAXSITE) < 0)
			log("Can't set site permissions\n");
		/* Check the shell to make sure it is a valid shell (fixes 
		 * bug # P12245).  If the user's shell is not listed, do 
		 * not allow login.
		 */
		if (get_valid_shell(pptr->pw_shell) == FALSE) {
		    pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW, 
		    	LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE, 
		    	NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
		    length = xmtPacket(&out, &ndata, swap_how);
		    break;
		}
#endif     /* LOCUS   */
		/* Start bridge in home directory */
		    if (chdir(pptr->pw_dir) < 0) {
			char *p;

			if (p = strdup("/tmp"))
				pptr->pw_dir = p;
			else if (pptr->pw_dir && strlen(pptr->pw_dir) != 0)
				strcpy(pptr->pw_dir, "/");
		    }
		    if (chdir(pptr->pw_dir) < 0)  {
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			break;
		    }

		/* Get current working directory, uid, gid from password file */
		    strcpy(cwd, pptr->pw_dir);

#ifdef SETUTMP
	/* need to set utmp entry so pci user looks like a user */
		    set_utmp(pptr);
#endif	/* SETUTMP */

		    logChown((uid_t)pptr->pw_uid, (gid_t)pptr->pw_gid);

#if defined(SecureWare)
		    if (setluid(pptr->pw_uid)) {
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			fatal(lmf_format_string(NULL, 0,
				lmf_get_message("P_CONNECT_LUID",
				"setluid(%1) failed -- errno %2\n"), 
				"%d%d", pptr->pw_uid, errno));
		    }
#endif

#ifdef LOCUS
		    {
			int sz;
			char uinfo[UINFOSIZ];
	
			/*
			 *  Record user name, uid, and tty
			 */
			sz = sprintf(uinfo, "NAME=%s%cUID=%d%cTTY=/dev/console%c",
				pptr->pw_name, '\0', pptr->pw_uid, '\0', '\0');
			(void)usrinfo(SETUINFO, uinfo, sz);
		    }
#endif	/* LOCUS */

#ifdef BSDGROUPS
#	ifdef USE_SETBASESG
		    /*
		     * FUTURE: When there is time (or we get a bug report),
		     * the following initgroups() call (in this USE_SETBASESG
		     * section) should be replaced with setbasesg().  I can't
		     * do it now, because it uses the "pwd" functions, and
		     * effectively overwrites data referenced by "pptr".  The
		     * fix should be straight-forward, but I don't really
		     * have the time to do it right (set up a set of general
		     * functions to allocate/free the field pointers, dealing
		     * with "no memory" errors gracefully), and I don't want
		     * to introduce a hacked-up solution.  For the time being,
		     * the use of initgroups() is reasonable.
		     *
		     * (void)setbasesg(pptr->pw_name, NULL);
		     */

		    (void)initgroups(pptr->pw_name, pptr->pw_gid);
#	else
		    (void)initgroups(pptr->pw_name, pptr->pw_gid);
#	endif
#endif
		    setgid((gid_t)pptr->pw_gid);
		    setuid((uid_t)pptr->pw_uid);

		    /* update environment variables */
		    updateenviron(pptr->pw_dir, loginname);

		/* Bridge session established */
		    bridge = RUNNING;
		    pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW, SUCCESS,
			NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE, NO_OFF,
			NO_ATTR, NO_DATE, NULLstat);
		    length = xmtPacket(&out, &ndata, swap_how);
		    break;
		}

#endif	  /* ETHNETPCI */

#ifdef RS232PCI
		if (in.hdr.req == PROBE) {
		    brg_seqnum = in.hdr.seq;
		    log("Bridge: probe\n");
		    pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW, SUCCESS,
			NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE, NO_OFF,
			NO_ATTR, NO_DATE, NULLstat);
		    length = xmtPacket(&out, &ndata, swap_how);
		    break;	
		}

#endif   /* RS232PCI  */

    /* Request to establish terminal emulator session */
    /* If reliable delivery set rd_flag to true	      */

		else if (in.hdr.req == EST_SHELL) {
#if	!defined(RS232PCI)
		    if (in.hdr.t_cnt > 0) {
			log("Connector using reliable delivery.\n");
			stream_size = in.hdr.t_cnt;
			log ("Stream size: %d\n", stream_size);
			rd_flag = 1;
			old_state = -1; /* undefined state */
		if (emulator != RUNNING) { /* init sem and shm first time */
                    if ((semaphore=rdsem_init()) == -1) {
			  log("Can't get rd semaphore.\n");
			  pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			  length = xmtPacket(&out, &ndata, swap_how);
			  break;
		    }
		    /* initialize and attach to the shared memory */
		    rd_rec_data = FALSE;	/* don't receive yet */
		    if ((rd_shmid = rd_sdinit()) < 0) {
			log("Can't get shared memory: errno %d\n", errno);
			rdsem_unlnk(semaphore);		/* remove semaphore */
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			break;
		    }

		    if ((sh_mem = (struct rd_shared_mem *) rd_sdenter(rd_shmid))
				== 0) {

			log("Can't enter shared memory: errno %d\n", errno);
			rdsem_unlnk(semaphore);	/* remove semaphore */
			rd_shmdel(rd_shmid);	/* remove shared memory */
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			break;
		    }
		} /* emulator != running */
		    sh_mem->FrameExpected = 0;
		    sh_mem->kick_ack = 0;

		    /*
		     * We are passing sh_mem which is a ptr to the
		     * shared memory that we need to dettach.
		     */
		    }  /* if hdr.t_cnt > 0 */
		else
#endif
		    rd_flag = 0;


		    brg_seqnum = in.hdr.seq;

#if	defined(ETHNETPCI)
		    if (emulator == RUNNING) {
			log("Dup shell req\n");
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW, 
			DUPLICATE_CONNECTION, NO_DES, NO_CNT, NO_CNT, NO_MODE, 
			NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);

			if (rd_flag) {	/* we need init termout */
				if (kill(termoutpid, SIGUSR2) < 0)
					log("Couldn't signal termout.\n");
		    		sh_mem->FrameExpected = 0; /* init shared mem */
		    		sh_mem->kick_ack = 0;
 				control.code = RD_INITTERM;  /* command code */
 				control.num = 1;    /* just to make sure */
 				if (write(pipedesc, &control, 
 					sizeof(struct ackcontrol)) < 0)
 					log("PIPE (INITTERM) write error: %d\n",
 					     errno);
 				log("Wrote INITTERM to pipe.\n");
			}
			break;
		    }
#endif	  /* ETHNETPCI */

		    log("Start Shell\n");

#ifdef RS232PCI
		    if (emulator == RUNNING) {
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				SUCCESS, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			goto runemulator;
		    }
#endif   /* RS232PCI  */

#ifdef UNISOFT
    		    if(bridge != RUNNING) {
			/* This dossvr was started for an Emulator session;
			   on the UniPlus system, we can't allow this because
			   problems with the remlogin program.  So, we dis-
			   allow the emulator session and stopService for this
			   dossvr.  
			*/
			log("No bridge session; Can't start EM.\n");
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			stopService(3,0);
			break;
		    }
#endif   /* UNISOFT  */
		/* Open PIPE to output process */
		    if ((pipe(descriptors)) < 0) {
			log("Can't pipe(): %d\n", errno);
#if	!defined(RS232PCI)
			rdsem_unlnk(semaphore);	/* remove semaphore */
			rd_sdleave((char *)sh_mem);	/* detach shared mem */
			rd_shmdel(rd_shmid);	/* remove shared memory */
#endif
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			break;
		    }

		/* Search for an available PTY */
#ifdef	AIX_RT 
		    if ((fptr = popen(PENABLE, "r")) == (FILE *)NULL) {
#else	/* ! AIX_RT */
		    if ((fptr = fopen(PCIPTYS, "r")) == (FILE *)NULL) {
#endif	/* AIX_RT */
			close(descriptors[0]);
			close(descriptors[1]); /* [1/26/88 PD] */
			log("Can't open %s: %d\n", PCIPTYS, errno);
#if	!defined(RS232PCI)
			rdsem_unlnk(semaphore);	/* remove semaphore */
			rd_sdleave((char *)sh_mem);	/* detach shared mem */
			rd_shmdel(rd_shmid);	/* remove shared memory */
#endif
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			break;
		    }

		/* Search PTY table for available PTY device */
		    ptydesc = -1;
		    while (fgets(ptydevice, MAX_FN_TOTAL, fptr)) {
#ifdef	AIX_RT
			if (strncmp(ptydevice, "pts", 3))
				continue;
			/* Save the slave pty name */
			strcpy(slvpty, ptydevice);
			/* change 'pts' to 'ptc' */
			ptydevice[2] = 'c';
			len = strlen(ptydevice);
			ptydevice[len-1] = '\0';
			strcpy(cntrlpty, "/dev/");
			strcat(cntrlpty, ptydevice);
			if ((ptydesc = open(cntrlpty, O_RDWR)) >= 0) {
			    /* Sleep for 5 sec for getty to get ready */
			    sleep(5);
			    break;
			}
#else	/* +AIX_RT- */
			len = strlen(ptydevice);
			ptydevice[len-1] = '\0';	/* zap newline */
			if (ptr = strchr(ptydevice, ':')) {
			    *ptr++ = '\0';
			    if (ptyGetty = strchr(ptr, ':')) {
				*ptyGetty++ = '\0';
				while (isspace(*ptyGetty))
				    ptyGetty++;
				if (!(*ptyGetty))
				    ptyGetty = NULL;
#if defined(RS232PCI)
				if (ptyGetty)	/* no consvr to run getty */
				    continue;	/*     so skip this pty   */
#endif
			    }
			}
			strcpy(cntrlpty, "/dev/");
			strcat(cntrlpty, ptydevice);
			if ((ptydesc = open(cntrlpty, O_RDWR)) >= 0) {
#if defined(STREAMS_PT)
			    signal(SIG_CHILD, SIG_DFL);
			    status = grantpt(ptydesc);
			    signal(SIG_CHILD, sig_catcher);
			    if (status != -1 && unlockpt(ptydesc) != -1
						 && (slvpty = ptsname(ptydesc)))
				break;
			    close(ptydesc);
			    ptydesc = -1;
#else	/* STREAMS_PT */
			    slvpty = ptr;		/* slave device name */
			    break;
#endif	/* STREAMS_PT */
			}
#endif 	/* AIX_RT */
		    }
#ifdef	AIX_RT
		    pclose(fptr);
#else	/* AIX_RT */
		    fclose(fptr);
#endif	/* AIX_RT */

		/* Was PTY available? */
		    if (ptydesc < 0) {
			log("No PTYs available\n");
#if	!defined(RS232PCI)
			rdsem_unlnk(semaphore);	/* remove semaphore */
			rd_sdleave((char *)sh_mem);	/* detach shared mem */
			rd_shmdel(rd_shmid);	/* remove shared memory */
#endif
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);

		    /* Clean-up from login attempt */
			close(descriptors[0]);
			close(descriptors[1]);
			break;
		    }

#ifdef	RIDGE
		    /* Turn off packet mode - should be off by default! */
		    ioctl(ptydesc, TIOCPKT, &off);
#endif /* RIDGE */

#if defined(STREAMS_PT) && defined(RS232PCI)

	if (fork() == 0)	{    /* child */

		/* close all file descriptors except the logfile */
		for (i = uMaxDescriptors() - 1; i >= 0; i--) 
			if (!logFile || i != fileno(logFile))
				close(i);

		setpgrp();
		
		if ((stdin_fd = open(slvpty, O_RDONLY)) != -1) {

			ioctl(stdin_fd, I_PUSH, "ptem");
			ioctl(stdin_fd, I_PUSH, "ldterm");
			ioctl(stdin_fd, I_PUSH, "ttcompat");

			if (open(slvpty, O_WRONLY) != -1) {

				dup2(1, 2);

				/*
				**  signals set to SIG_IGN should be restored
				**  to SIG_DFL before spawning user shell
				*/

				/* prepend '-' indicating a login shell */
				if (shell_name = strrchr(shell_path, '/'))
					shell_name++;
				else
					shell_name = shell_path;
				strcpy(shell_arg0, "-");
				strcat(shell_arg0, shell_name);

				strcpy(env_buffer[0],"LOGNAME=");
				strcat(env_buffer[0], getenv("LOGNAME"));
				shell_env[0]= env_buffer[0];
 
				strcpy(env_buffer[1],"PATH=");
				strcat(env_buffer[1], getenv("PATH"));
				shell_env[1]= env_buffer[1];
 
				strcpy(env_buffer[2],"TZ=");
				strcat(env_buffer[2], getenv("TZ"));
				shell_env[2]= env_buffer[2];
 
				strcpy(env_buffer[3],"TERM=");
				strcat(env_buffer[3], getenv("TERM"));
				shell_env[3]= env_buffer[3];
 
				strcpy(env_buffer[4],"HOME=");
				strcat(env_buffer[4], getenv("HOME"));
				shell_env[4]= env_buffer[4];
 
				shell_env[5]= NULL;
				execle(shell_path, shell_arg0, NULL, shell_env);
			} /* open */
		} /* open */

		pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
		length = xmtPacket(&out, &ndata, swap_how);
		fatal(lmf_format_string(NULL, 0, 
					lmf_get_message("P_CONNECT_NOSHELL",
					"can't exec user shell %1\n"),
					"%s", shell_path));

	}   /* end child */

#elif !defined(DONT_START_GETTY) && defined(UDP42)
		    if (ptyGetty) {

			/* Ask consvr to startup login process */
			strcpy(conbuf.text, slvpty);
			i = strlen(slvpty) + 1;
			i += sprintf(&conbuf.text[i], ptyGetty, slvpty) + 1;
			pckframe(&conbuf, DAEMON, brg_seqnum, S_LOGIN, NEW, 0,
				0, 2, i, 0, 0, getpid(), 0, 0, NULLstat);

			/* Send request to consvr */
	        	xmtPacket(&conbuf, &conSvrHeader, NOFLIP);

			/* Now wait for response from getty-to-be */
			do {
			    rcvPacket(&cibuf);
			    /* Is this the packet I've been waiting for? */
			    if (nAddrEq(cibuf.net.src, conSvrHeader.dst))
				break;
			} while(cibuf.hdr.req != S_LOGIN);

			if (cibuf.hdr.req != S_LOGIN) {
			    /* Something is amis here, let's just forget */
			    /* the whole thing!				 */
			    /* Clean-up from login attempt		 */
#if	!defined(RS232PCI)
			    rdsem_unlnk(semaphore);	/* remove semaphore */
			    rd_sdleave((char *)sh_mem);	/* detach shared mem */
			    rd_shmdel(rd_shmid);	/* remove shared mem */
#endif
			    close(descriptors[0]);
			    close(descriptors[1]);
			    /* we'll just drop the packet and let the pc
			    retry with it's current request */
			    break;
			}

			/* This is our response from the getty-to-be */
			if (cibuf.hdr.res != SUCCESS) {
			    log("Can't start login\n");
#if	!defined(RS232PCI)
			    rdsem_unlnk(semaphore);	/* remove semaphore */
			    rd_sdleave((char *)sh_mem);	/* detach shared mem */
			    rd_shmdel(rd_shmid);	/* remove shared mem */
#endif
			    pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			    length = xmtPacket(&out, &ndata, swap_how);

			    /* Clean-up from login attempt */
			    close(descriptors[0]);
			    close(descriptors[1]);
			    break;
			}
			login_pid = cibuf.hdr.offset;
		    }
#endif	/* !DONT_START_GETTY && UDP42 */

		/* Execute terminal emulation output process */
		    if ((termoutpid = fork()) < 0) {
			log("Can't fork: %d\n", errno);

#if	!defined(RS232PCI)
			rdsem_unlnk(semaphore);	/* remove semaphore */
			rd_sdleave((char *)sh_mem);	/* detach shared mem */
			rd_shmdel(rd_shmid);	/* remove shared memory */
#endif
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);
			close(descriptors[0]);
			close(descriptors[1]);
			break;
		    }

		    else if (termoutpid != 0) {
		    /* Parent */
			close(descriptors[0]);
			pipedesc = descriptors[1];
		    }
		    else {
		    /* Child */
			close(descriptors[1]);
			pipedesc = descriptors[0];

			/* Pass destination network address */
#ifdef	ETHNETPCI
			sprintf(naddrArg, "-a%s",
			    nAddrFmt((unsigned char *)in.net.src));
			sprintf(descArg, "-n%d", netdesc);
#endif   /* ETHNETPCI  */
			sprintf(descArg, "-t%d", netdesc);

			if (logFile != NULL)
				sprintf(errArg, "-e%d", fileno(logFile));
			sprintf(pipeArg, "-p%d", pipedesc);
			sprintf(ptyArg, "-y%d", ptydesc);
			sprintf(debugArg, "-D%04x", dbgEnable);
			sprintf(swapArg, "-s%d", swap_how);

		    /* Execute terminal output process */

#ifdef VERSION_MATCHING
			if (version_number)
				sprintf(fname, "%s/versions/%s/%s", PCIDIR,
					version_number, PCIOUT);
			else
#endif
			     if (versNum == 0)
				sprintf(fname, "%s/%s", PCIBIN, PCIOUT);
			else
				sprintf(fname, "%s/%s%d", PCIBIN, PCIOUT,
					versNum);
#ifdef	ETHNETPCI
			if (!rd_flag)
			   execl(fname, strrchr(fname, '/') + 1, errArg,
				debugArg, descArg, naddrArg, pipeArg,
				ptyArg, swapArg, (char *)NULL);
			else {
			   sprintf(rdArg, "-R%d", semaphore);
			   sprintf(ssArg, "-S%d", stream_size);
			   stream_size = 0;	/* reset for later code */

			   execl(fname, strrchr(fname, '/') + 1, errArg,
				debugArg, descArg, naddrArg, pipeArg,
				ptyArg, swapArg, rdArg, ssArg, (char *)NULL);
			}

#endif	  /* ETHNETPCI  */
			execl(fname, strrchr(fname, '/') + 1, errArg,
				debugArg, descArg, pipeArg,
				ptyArg, swapArg, (char *)NULL);
#if	!defined(RS232PCI)
			rdsem_unlnk(semaphore);	/* remove semaphore */
			rd_sdleave((char *)sh_mem);	/* detach shared mem */
			rd_shmdel(rd_shmid);	/* remove shared memory */
#endif
			pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW,
				LOGIN_FAILED, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
			length = xmtPacket(&out, &ndata, swap_how);

		    /* Clean-up from login attempt */
			close(ptydesc);
			close(descriptors[0]);
			fatal(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("P_CONNECT11",
				"Can't exec %1: %2\n"), 
				"%s%d", fname, errno));
		    }


		/* Send response to original EST_SHELL request */
		    emulator = RUNNING;
		    ti_seqnum = 0;

		    pckframe(&out, CONF, brg_seqnum, in.hdr.req, NEW, SUCCESS,
			NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE, NO_OFF,
			NO_ATTR, NO_DATE, NULLstat);
		    length = xmtPacket(&out, &ndata, swap_how);
#if	defined(ETHNETPCI)
		    break;
#endif	  /* ETHNETPCI */
#ifdef RS232PCI

		/* Read raw stream and write unstuffed chars to pty */
runemulator:

		    for (;;) {

			if ((status = read(netdesc, &ptybuf[0], 1)) < 0) {
			    log("TTY Read err: %d\n", errno);
			    continue;
			}
			debug(0, ("Input %c\n", ptybuf[0]));

		    /* Scan stream for termination characters */
			if (ptybuf[0] == SYNC) {
			    if ((status = read(netdesc, &ptybuf[1], 1)) < 0) {
				log("TTY Read err: %d\n", errno);
				continue;
			    }

			/* Break out of Emulator mode inot bridge mode */
			    if (ptybuf[1] != SYNC)
				break;
			    else if ((status = write(ptydesc, &ptybuf[0], 1)) < 0) {
				log("PTY Write err: %d\n", errno);
				continue;
			    }
			}
			else if (ptybuf[0] == SYNC_2) {
			    if ((status = read(netdesc, &ptybuf[1], 1)) < 0) {
				log("TTY Read err: %d\n", errno);
				continue;
			    }
			    if (ptybuf[1] == SYNC_2) {
			        if ((status = write(ptydesc, &ptybuf[0], 1)) < 0) {
				    log("PTY Write err: %d\n", errno);
				    continue;
				}
			    }

			/* Recognized Flow Control character */
			    else {

				debug(0, ("got flow ctl\n"));

				c = SYNC_2;

				while (write(pipedesc, &c, 1) < 0) {
				    if (errno == EINTR)
					continue;
				    log("PIPE Write error: %d\n", errno);
				    break;
				}
			    }
			}
			else {
			    /* give it up, if dosout has exited.  Take data
			       as file service packets (they won't be accepted
			       because of sync/null and checksum.)
			       [2/26/88 PD] */
			    if (termoutpid == 0)
				break;
			    if ((status = write(ptydesc, &ptybuf[0], 1)) < 0) {
				if (errno == EINTR)
				    continue;
				log("PTY Write error: %d\n", errno);
				break;
			    }
			    debug(0, ("Got input %c\n", ptybuf[0]));
			}
		    }
#endif   /* RS232PCI  */
		}

	/* Request to terminate terminal emulator session */
		else if (in.hdr.req == TERM_SHELL) {
		    ti_seqnum = in.hdr.seq;
		    log("Stop Shell\n");


		/* Terminate terminal output process */
		    if(savePid = termoutpid) {
			log("Sending SIGTERM to dosout\n");
			kill(savePid, SIGTERM);
#ifndef XENIX	/* Causes file service dossvr to die: wasn't here in 2_8_1 */
		        u_wait((int *) 0);
#endif	/* XENIX */
		        termoutpid = 0;
		    }

#if	!defined(RS232PCI)
		    log("Deleting IPC members.\n");
		    rd_sdleave((char *)sh_mem);	/* detach shared mem */
		    rd_shmdel(rd_shmid);	/* delete "segment" */
		    rdsem_unlnk(semaphore);	/* get rid of semaphore */  
#endif
		/* Close PTY device and pipe */
		close(ptydesc);
		close(pipedesc);

		/* Send response frame */
		    pckframe(&out, CONF, ti_seqnum, in.hdr.req, NEW, SUCCESS,
			NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE, NO_OFF,
			NO_ATTR, NO_DATE, NULLstat);
		    length = xmtPacket(&out, &ndata, swap_how);

		    emulator = INITIALIZED;
		    ti_seqnum = 0;

		    /* If bridge was established only for emulator session,
			the bridge is terminated on a Stop Shell request */

		    if (bridge != RUNNING)
			stopService(3,0);
		}
		break;

	    case BRIDGE:
		if (in.hdr.req == PROBE)
			break;
		if (bridge != RUNNING) {
		    pckframe(&out, BRIDGE, (int)in.hdr.seq, in.hdr.req, NEW,
			NO_SESSION, NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE,
			NO_OFF, NO_ATTR, NO_DATE, NULLstat);
#ifdef ETHNETPCI
		/* Store destination address of PC */
		    for (i = START; i < DST; i++)
			ntmp.dst[i] = in.net.src[i];
#endif   /* ETHNETPCI  */
		    length = xmtPacket(&out, &ndata, swap_how);
		    break;
		}

	    /* Invoke the PC-DOS Bridge server */
		server(&in);
		break;
	

	    case SHELL:

#ifdef ETHNETPCI
	    /* Store destination address of PC */
		for (i = 0; i < DST; i++)
		    ntmp.dst[i] = in.net.src[i];
#endif   /* ETHNETPCI  */

	    /* Is a terminal emulator session established */
		if (!emulator) {
#if	!defined(RS232PCI)
		    /* no need to respond if this is an ACK */
		    if (rcvhead->code == RD_ACK)
			break;
#endif

		    pckframe(&out, SHELL, (int)in.hdr.seq, in.hdr.req, NEW,
			NO_SESSION, NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE,
			NO_OFF, NO_ATTR, NO_DATE, NULLstat);

		    length = xmtPacket(&out, &ndata, swap_how);
		    break;
		}
if (rd_flag) {
#if	!defined(RS232PCI)
/******************************************************************************/
/*                                                                            */
/*                    RELIABLE DELIVERY CODE                                  */
/*                                                                            */
/* Most of the receive logic for the protocol is contained in the confines of */
/* this macro ifdef block.                                                    */
/*                                                                            */
/******************************************************************************/

	log("RDR packet       dnum=%d\n",rcvhead->dnum);
	log("                 anum=%d\n",rcvhead->anum);
	log("                 tcbn=%d\n",rcvhead->tcbn);
	log("                 version=%d\n",rcvhead->version);
	log("                 code=%d\n",rcvhead->code);
	log("                 options=%d\n",rcvhead->options);
	log("                 strsiz=%d\n",rcvhead->strsiz);
	if (rcvhead->code != RD_ACK)
	      log("   %c,%c,%c,%c\n",in.text[sizeof(struct emhead)],
		in.text[sizeof(struct emhead)+1], 
		in.text[sizeof(struct emhead)+2], 
		in.text[sizeof(struct emhead)+3]);

		stream_size = rcvhead->strsiz; /* get stream size */

		if (rcvhead->code == RD_ACK || rcvhead->code == RD_PB)
		{
                   control.code = RD_ACK;
                   control.tcbn = rcvhead->tcbn;
		   control.num  = rcvhead->anum;
		   control.ssiz = (unsigned short)stream_size;
							/* setup stream size */
		   stream_size = 0;	        /* reset stream size */
		   if (rcvhead->anum == 0)
			rd_rec_data = TRUE;	/* start after first ack */
		   log("RDR Was ACK, sending it to TERMOUT via pipe.\n");
                   if (write(pipedesc, &control, sizeof (struct ackcontrol)) < 0)
			log("PIPE Write err: %d\n",errno);
		   if (rcvhead->code != RD_PB)
		        break;
                }
		if (rd_rec_data && (rcvhead->code == RD_DATA || 
				    rcvhead->code == RD_PB)      &&
		     (rdsem_access( semaphore ) != -1)) 
		{
  		      log("In Loop: dnum = %d, FE = %d\n", rcvhead->dnum,
			sh_mem->FrameExpected);
                      accept = FALSE;
		      send_ack = FALSE;
		      sequence = 0;
                      if (sh_mem->kick_ack == 0)
		      {
			  if (rcvhead->dnum > sh_mem->FrameExpected)
			  {
			      log("RDR state 0\n");
			      log("    dropped %d\n",sh_mem->FrameExpected);
			      log("    but can accept this one.\n");
			      accept = FALSE;
                              send_ack = TRUE;
			      sequence = sh_mem->FrameExpected-1; /* ack last */
			      old_state = 0;
                          }
			  else if (rcvhead->dnum == sh_mem->FrameExpected)
			  {
			      log("RDR state 1\n");
			      log("    got what was expected.\n");
			      accept = TRUE;
			      sh_mem->kick_ack = 1;
			      sh_mem->FrameExpected++;
			      old_state = 1;
                          }
			  else
			  {
			      log("RDR state 2\n");
			      log("     duplicate packet.\n");
			      sequence = rcvhead->dnum;
			      accept = FALSE;
			      send_ack = TRUE;
			      old_state = 2;
                          }
                      }
                      else           /* there is a need to ack a prev pkt */
		      {
			  send_ack = TRUE;
			  sh_mem->kick_ack = 0;
			  if (rcvhead->dnum > sh_mem->FrameExpected)
			  {
			      log("RDR state 3\n");
			      log("     lock step detected.\n");
			      sequence = sh_mem->FrameExpected-1;
			      accept = TRUE;
			      old_state = 3;
                          }
			  else if (rcvhead->dnum == sh_mem->FrameExpected)
			  {
			      log("RDR state 4\n");
			      log("    got what was expected.\n");
			      accept = TRUE;
			      sh_mem->kick_ack = TRUE;
			      sequence = rcvhead->dnum-1;
			      sh_mem->FrameExpected++;
			      old_state = 4;
                          }
			  else
                 	  {
			      log("RDR state 5\n"); 
			      log("    duplicate packet.\n");
			      sequence = rcvhead->dnum;
			      accept = FALSE;
			      old_state = 5;
                          }
                      }
		      rdsem_relinq( semaphore );
		      if (send_ack == TRUE)
		      {
			  log("RDR sending ack: %d\n",sequence);
	                  pckRD( sndhead, RD_ACK, 0, sequence, 0, 
			         rcvhead->tcbn, RD_VERSION );
		          pckframe(&out, SHELL, in.hdr.seq, in.hdr.req, 
			               ACK, SUCCESS, NO_DES, NO_CNT, 
				       sizeof (struct emhead), NO_MODE, 
				       NO_SIZE, NO_OFF, NO_ATTR, 
				       NO_DATE, NULLstat);
                          xmtPacket(&out, &ndata, swap_how);
                      } 
		      if (accept)
		      {
		          log("RDR Accept data, send to pty.\n");
/* The alarm breaks us out of blocked writes that result in deadlocks */
			alarm(2);
		          if ((len=write(ptydesc,in.text+sizeof (struct emhead),
                              in.hdr.t_cnt-sizeof (struct emhead))) < 
                              in.hdr.t_cnt-sizeof (struct emhead) &&
			      errno != EINTR)
		                log("PTY Write err: %d\n",errno);
			alarm(0);
/*
 * Sometimes stream size info comes in on a RD_DATA packet. This has to
 * be transferred to termout. It is already handled by piggy back but this
 * case is only for DATA only, when accepted.
 */
			  if (rcvhead->code==RD_DATA && stream_size>0) {
		   log("RDR: GOT STREAM size info on data packet.\n");
                   		control.code = RD_DATA;
                   		control.tcbn = rcvhead->tcbn;
		   		control.num  = rcvhead->anum;
		   		control.ssiz = (unsigned short)stream_size; 	
                   		if (write(pipedesc, &control, 
					sizeof (struct ackcontrol)) < 0)
					log("PIPE Write err: %d\n",errno);
		          }

                      }
		}
	     break;
#endif
}
else {
	    /* Pipe flow control to terminal output process */
		if((in.hdr.stat == RFNM) || (in.hdr.stat == 5))
		{
		    c = RFNM;
		    debug(0, ("Got input: send flow ctl\n"));

		    if (write(pipedesc, &c, 1) < 0)
			log("PIPE Write err: %d\n", errno);
		break;		/* don't even try to give text from this to pty.. */
		}

#ifdef SYMETRIC

	    /* Is this a duplicate input frame? */
		if (ti_seqnum != in.hdr.seq-1) {
		    if (acceptence < 0) {

		    /* Lost ACK: re-ACK input frame */
			pckframe(&out, SHELL, in.hdr.seq, in.hdr.req, ACK,
				SUCCESS, NO_DES, NO_CNT, NO_CNT, NO_MODE,
				NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);

			length = xmtPacket(&out, &ndata, swap_how);
			break;
		    }
		    else {
		    /* Partial acceptence of data */
			txptr = in.text + accepted;
			in.hdr.t_cnt -= accepted;
		    }
		}
		else {
		    ++ti_seqnum;
		    txtptr = in.text;
		}

#endif   /* SYMETRIC  */

/* we don't look at stat except to see if it is an ack.. */
/* at this point, acks are weeded out - this must be text */
/* we really should check, you know */

/* **** Note ***** Note ***** Note */

/* you may not like wahat this does - I know I don't and I wrote it */
/* the test for duplicate packets looks to see if the currently    */
/* received packet is less than what we expected. if it isn't it must be */
/* at least what we want, in which case we use it. otherwise, it must be */
/* a duplicate packet of some sort. the trouble is that sequence numbers */
/* wrap around at 255 to 0. imagine we want 255 and miss it and get 0 ... */
/* we say "is 0 less than 255?" and we chuck it because it must be a dup */
/* this works for any wrapped pair...  how do we fix this? easy, we have a */
/* "window" of duplicate checking. Anything outside this window is assumed */
/* too old to be a dup of a recent packet and must therefore be a wrapped */
/* packet. we accept it for what it is (if only parents could do the same */
/* with their children) and re-synchronize on it */


	if (++ti_check == TI_CHECK_INTERVAL)
	{
		pcDbg(("ti_check: ti_seqnum %d, actually %d\n",
			ti_seqnum, in.hdr.seq));
		ti_check = 0;	/* reset this */
	}


#if	!(defined(RIDGE) || defined(IX370))
	/*  This is taken out for Ridge(and 370's) due to bug in the PC 
	    bridge where
	    sequence numbers are not reset upon subsequent entries in to 
 	    terminal emulation
	*/
	if ((ti_seqnum != in.hdr.seq) && ((ti_seqnum - in.hdr.seq) <= TI_DUP_WINDOW))
	{
		pcDbg(("em dup %d - dropped\n", in.hdr.seq));
		break;
	}
#endif	/* RIDGE */

 if (ti_seqnum != in.hdr.seq)
	{
		pcDbg(("break in em sequence\n"));
		ti_seqnum = in.hdr.seq;	/* re-sync */
	}

	ti_seqnum = (ti_seqnum + 1) & 0xFF;	/* byte from PC */


	    /* Send any data thru PTY into shell */
		txtptr = in.text;
		if (in.hdr.t_cnt) {
		    if ((len = write(ptydesc, txtptr, in.hdr.t_cnt)) != in.hdr.t_cnt) {

#ifdef SYMETRIC

/*
 * For partial success on write save the amount of data actually written,
 * and do not ACK frame.
 */
			if (accepted < 0)
			    log("PTY Write err: %d\n", errno);
			else {
			    accepted = (accepted < 0) ? len : accepted + len;
			    debug(0, ("Partial input: %d| %s\n", in.hdr.t_cnt,
				in.text));
			}
			break;

#else   /* ~SYMETRIC  */

			log("PTY Write err: %d\n", errno);

#endif   /* SYMETRIC  */
		    }

#ifdef SYMETRIC
		/* Acknowledge input frame and reset acceptence flag */
		    pckframe(&out, SHELL, in.hdr.seq, in.hdr.req, ACK, SUCCESS,
			NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE, NO_OFF,
			NO_ATTR, NO_DATE, NULLstat);

		    length = xmtPacket(&out, &ndata, swap_how);

		    accepted = -1;

#endif   /* SYMETRIC  */

		    debug(0, ("input data: %.*s\n", in.hdr.t_cnt, in.text));
		}

}  /* else not reliable delivery */
                   break;

	    default:

		log("Bad select: %d\n", in.pre.select);
		pckframe(&out, CONF, (int)in.hdr.seq, in.hdr.req, NEW,
			INVALID_FUNCTION, NO_DES, NO_CNT, NO_CNT, NO_MODE,
			NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);

#ifdef ETHNETPCI

	    /* Store destination address of PC */
		for (i = 0; i < DST; i++)
		    ntmp.dst[i] = in.net.src[i];

#endif   /* ETHNETPCI  */
		length = xmtPacket(&out, &ndata, swap_how);

		break;
	}/*endswitch*/
    }/*end for(;;)*/
}			/* main() */



/*
   getVers: Extract version number from program name
*/

getVers(progName)
char
	*progName;
{
	register char
		*versPart;		/* Version number in program name */

	/* Find the first digit in program name */
	if ((versPart = strpbrk(progName, "0123456789")) == (char *)NULL)
		return 0;

	/* Avoid the "232" part of the rs232 servers name */
	/* This is a kludgey hack! */
	if (versPart[0] == '2' && versPart[1] == '3' && versPart[2] == '2')
		versPart += 3;

	return atoi(versPart);
}


#ifdef	RS232PCI

/*
   dosSvrAlive: Tell PC that the RS232 server is waiting
*/

void dosSvrAlive(status)
int
	status;
{
	extern int flipBytes;	/* byte ordering flag */

	pckframe(&out, CONF, 0, RS232_ALIVE, NEW, status, NO_DES, NO_CNT,
		NO_CNT, NO_MODE, NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);

	/* The following delay is to workaround a timing problem in the
	   bridge in which very fast UNIX hosts return this packet before
	   the bridge is ready to handle it */
	sleep(1);

	length = xmtPacket(&out, &ndata, flipBytes);
}
#endif    /* RS232PCI  */


/*
   sendMap: Construct response to a map request
*/

void sendMap(respPkt)
struct output
	*respPkt;
{
	setnameAddr((struct nameAddr *)respPkt->text);
	pckframe(respPkt, DAEMON, in.hdr.seq, SEND_MAP, NEW, 0, 0,
		1, sizeof(struct nameAddr),
		0, 0, 0, 0, 0, NULLstat);
}


void setnameAddr(myName)
struct nameAddr
	*myName;
{
#ifdef	UDP42
	static struct hostent
		*uName = 0;
#endif   /* UDP42  */

	strncpy(myName->name, myhostname(), sizeof myName->name);
}

/* We got a CONNECT.  Restart using the appropriate server. */
/* All state is lost; better hope that this was just about */
/* the first message. */
void do_connect_1()
{
	int kidpid;
	int pipedes[2];
	char svrProg[64];			/* Name of server program */
	char debugArg[16];			/* Debug level argument */
	char netArg[16];			/* Network descriptor number */
	char netDev[64];			/* Network device name */
	char pipeArg[8];			/* Ack pipe descriptor */
	char disArg[8];				/* nodisconnect arg */
	int myerrno;
#ifdef VERSION_MATCHING
	struct connect_text *connText;
	struct stat svrStatBuf;			/* For doing stat() on dossvr */
	char	versionArg[16];			/* Version number */
#endif /* VERSION_MATCHING */

	if (pipe(pipedes) < 0) {
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("PP_CONSVR7",
			"Can't create pipe:errno %1\n"),
			"%d",errno));
	}

	log("pipe[0] = %d, pipe[1]= %d\n",pipedes[0],pipedes[1]);


	log("forking kid to hand back CONNECT message\n");
	if ((kidpid = fork()) < 0 ) {
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("PP_CONSVR8",
			"Can't fork:errno %1\n"),
			"%d", errno));
	}

	if (kidpid == 0) {
		int pcnt;
		/* this one is the kid */
		/* give the server the input packet and go away */
		output_swap((struct output *)&in,swap_how);
		if ((pcnt=write(pipedes[1],&in,sizeof in)) !=  sizeof in) {
			fatal(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("PP_CONSVR9",
				"Can't write connmsg to pipe:errno %1,cnt %2, pcnt %3\n"),
				"%d%d%d", myerrno = errno,sizeof in,pcnt));
		}
		close(pipedes[1]);
		exit(0);
	}

	/* this code runs in the parent */

#ifdef VERSION_MATCHING
	connText = (struct connect_text *) in.text;
	log("PC version: %d.%d.%d \n", (int) connText->vers_major,
	    (int) connText->vers_minor, (int) connText->vers_submin);
#endif /* VERSION_MATCHING */

/*
* Here is where we look for a matching server. If none that is executable
* then inform the user about that.
*/
#ifdef VERSION_MATCHING
	/* get version into server name */
	sprintf(svrProg, "%s/versions/%d.%d/%s", PCIDIR,
		(int) connText->vers_major, 
		(int) connText->vers_minor, PCIDOSSVR);

	/* Check the existence of the server. If its mode says that it	*/
	/* is a directory then it is a hidden directory but the load	*/
	/* module is not there.						*/

	if ((stat(svrProg, &svrStatBuf) < 0)
	|| (S_ISDIR(svrStatBuf.st_mode))) {
		pckframe(&out, DAEMON, (int)in.hdr.seq, in.hdr.req, 
			 NEW, FAILURE, NO_DES, NO_CNT, NO_CNT,
			 NO_MODE, NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, 
			 NULLstat);
		out.hdr.b_cnt = 1;	/* version error */
		length = xmtPacket(&out, &ndata, swap_how);
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("PP_CONSVR10",
			"Can't access DOS server `%1'\n"),
			"%s", svrProg));
	}
#else	/* !VERSION_MATCHING */
	sprintf(svrProg, "%s/%s", PCIBIN, PCIDOSSVR);
#endif	/* VERSION_MATCHING */

	if (in.hdr.versNum != 0)	/* cmd line "-s<svr number>" */
		sprintf(svrProg+strlen(svrProg), "%d", in.hdr.versNum);


#ifdef	IBM_SERCHK
	if (serchk(&in,swap_how) == 0) {
		/* bad serial# for version */
		pckframe(&out, DAEMON, (int)in.hdr.seq, in.hdr.req, 
			 NEW, FAILURE, NO_DES, NO_CNT, NO_CNT,
			 NO_MODE, NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, 
			 NULLstat);
		out.hdr.b_cnt = 1;	/* version error */
		length = xmtPacket(&out, &ndata, swap_how);
		fatal(lmf_get_message("PP_CONSVR11",
			"Bad serial number for version\n"));
	}
#endif	/* IBM_SERCHK */

	sprintf(debugArg, "-D%s", logStr);
	sprintf(netArg, "-n%d", netdesc);
	sprintf(netDev,"-t%s",mydevname);
	sprintf(pipeArg,"-P%d",pipedes[0]);
	sprintf(disArg,noDisconnect?"-x":"--");
#ifdef VERSION_MATCHING
	sprintf(versionArg, "-V%d.%d",
		(int) connText->vers_major, 
		(int) connText->vers_minor);
#endif	/* VERSION_MATCHING */
	log("execing %s with %s %s",svrProg, debugArg, netDev);
#ifdef	VERSION_MATCHING
	log(" %s",versionArg);
#endif	/* VERSION_MATCHING */
	log(" %s %s %s\n", disArg, pipeArg, netArg);

	logClose();

	execl(svrProg, PCIDOSSVR, debugArg, netDev,
#ifdef	VERSION_MATCHING
		versionArg,
#endif	/* VERSION_MATCHING */
		disArg, pipeArg, netArg, (char *) 0);

	myerrno = errno;
	pckframe(&out, DAEMON, (int)in.hdr.seq, in.hdr.req, NEW, INVALID_FUNCTION, NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE, NO_OFF, NO_ATTR, NO_DATE, NULLstat);
	length = xmtPacket(&out, &ndata, swap_how);
	/* All dressed up, no place to go */
	fatal(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("PP_CONSVR12",
		"Can't exec DOS server `%1': %2\n"),
		"%s%d", svrProg, myerrno));
}

/* Processing after we get restarted after a CONNECT */
void do_connect_2()
{
	pckframe(&out, DAEMON, in.hdr.seq, CONNECT, NEW, 0, 0, 0, 0,
		0, 0, 0, 0, 0, NULLstat);
	length = xmtPacket(&out, &ndata, swap_how);
}


/*
 * sig_catcher() -		Signal catcher.
 */

void
sig_catcher(signum)
register int signum;
{
    int
	pid,			/* Process id of children */
	cstat;			/* Contains status of child termination */

    switch (signum) {
	case SIG_DBG1:
	case SIG_DBG2:
	    newLogs(DOSSVR_LOG,getpid(),(long *)NULL,(struct dbg_struct *)NULL);
	    if (logStr != (char *)NULL)
		sprintf(logStr, "%04x", dbgEnable);
	    break;

	case SIGINT:
	case SIGTERM:
	case SIGHUP:
	    log("Signal %d - Quit\n", signum);
	    stopService(3,0);
	    break;

	case SIG_CHILD:
	    pid = u_wait(&cstat);
	    childExit(pid, cstat);
	    break;

	case SIGALRM:
	    break;		/* No action needed */

	case SIGSYS:
	    if (termoutpid)
		kill(termoutpid, SIGHUP);
	    break;
    }
    signal(signum, sig_catcher);
}


/*
   stopService: Shut down dos server and termout process, if any.  Kill all
		children.
*/

void stopService(dum1, dum2)
int dum1, dum2; /* these are not used. Other versions stopService do. */
{

	/* The re-entry gate may have a hole in it.  If the hole turns out
	 * to matter, someone better find a way to plug it.
	 */
	if (stopGate++) { 
		stopGate--;
		return;
	}
	/* Clean-up tmp files (pipes, etc...) */
	unlink_on_termsig();

#ifdef RLOCK  /* record locking */
	/* Close all dossvr's files, removing locks */
	/* and removing entry in the global open file table */
	close_all();	
#endif  /* RLOCK */

	/* Kill terminal emulation output process, if it exists */
	if ((savePid = termoutpid) != 0) {
#if	!defined(RS232PCI)
		rdsem_unlnk(semaphore);		/* remove semaphore */
		rd_sdleave((char *)sh_mem);	/* detach shared memory */
		rd_shmdel(rd_shmid);		/* remove shared memory */
#endif
		signal(SIG_CHILD, SIG_IGN);
		log("Kill dosout %d\n", savePid);
		kill(savePid, SIGTERM);
		u_wait((int *) 0);
	}

#ifdef	RS232PCI
	/* Reset tty modes */
	drain_tty(netdesc);
	set_tty(netdesc, &ttysave);
#endif	  /* RS232PCI  */
	
	/* Kill all children and wait for their termination status */
	kill_uexecs();
	while (wait((int *)NULL) != -1);

	/* Exit normally */
	log("stopService %d,%d:exit(0)\n",dum1,dum2);
	exit(0);
}


void childExit(pid, status)
register int
	pid,				/* Pid of dead child process */
	status;				/* Exit status */
{
	if (pid == termoutpid) {
		termoutpid = 0;
		log("Termout %d; exit %d:%d\n", pid, ((status >> 8) & 0xff),
			(status & 0xff));
#if	!defined(RS232PCI)
		log("Deleting IPC members.\n");
		rd_sdleave((char *)sh_mem);	/* detach shared mem */
		rd_shmdel(rd_shmid);	/* delete "segment" */
		rdsem_unlnk(semaphore);	/* get rid of semaphore */  
#endif
		close(ptydesc);
		close(pipedesc);
		emulator = INITIALIZED;
#if	defined(UDP42) && !defined(DONT_START_GETTY)

		/* Send a close errror to the PC */
		pckframe(&conbuf, SHELL, brg_seqnum, 0, NEW,
			NO_SESSION, NO_DES, NO_CNT, NO_CNT, NO_MODE, NO_SIZE,
			NO_OFF, NO_ATTR, NO_DATE, NULLstat);

		xmtPacket(&conbuf, &ndata, swap_how);

		/* Tell CONSVR dosout quit so login on pty can be killed! */
		pckframe(&conbuf, DAEMON, brg_seqnum, (unsigned char)K_LOGIN,
			 NEW, 0, getpid(),
			 0, 0, 0, 0, login_pid, 0, 0, NULLstat);
		xmtPacket(&conbuf, &conSvrHeader, NOFLIP);
		do {
			rcvPacket(&cibuf);
			/* Is this the packet I've been waiting for? */
			if (nAddrEq(cibuf.net.src, conSvrHeader.dst))
				break;
		    } while(cibuf.hdr.req != (unsigned char)K_LOGIN);

		    if (cibuf.hdr.req != (unsigned char)K_LOGIN) {
			/* Something is amis here, let's just forget 	*/
                        /* the whole thing!                         	*/
			/* we'll just drop the packet and let the pc
			   retry with it's current request */
		    }
#endif   /* UDP42  && !DONT_START_GETTY */
	}
}

#ifdef	RESTRICT_USER

/*
** restrictUser: Opens and searches RESTRICTFILE for name.
**
**	RESTRICTFILE needs to be opened every time because it can change
**	between invocations.
**
**	Returns 1 if name is found
**		0 otherwise
**
**	If RESTRICTFILE can't be opened for some reason, 0 is returned.
*/
#define	NL		'\n'

int
restrictUser(name)
	char *name;
{
	static char buf[BUFSIZ];
	register char *bp, *np;
	register int fd, i;

	if ((fd = open(RESTRICTFILE, O_RDONLY)) < 0)
		return(0);
	while ((i = read(fd, buf, BUFSIZ)) > 0) {
		bp = &buf[i-1];
		for (i = 0; *bp != NL && bp >= buf; i++)
			bp--;	/* move back to end of last name */
		*bp = NULL;
		if (lseek(fd, (long) -i, 1) < 0) {
			close(fd);
			return(0);
		}
		bp = buf;
		while (np = name, *bp) {
			while (*np == *bp && *bp)
				np++, bp++;
			if ((*np == NULL) && (*bp == NL || *bp == NULL)) {
				close(fd);
				return(1);
			}
			else
				while (*bp != NL && *bp)
					bp++;
			bp++;
		}
	}
	close(fd);
	return(0);
}
#endif	  /* RESTRICT_USER  */

#ifdef  LOCUS
#define VALID_SITE 1
#define INVALID_SITE 0
    /* get_valid_sites() places 1's in txperms[] for valid sites and */
    /* 0's for invalid sites.  If valid sites cannot be determined,  */
    /* all sites are considered to be valid.			     */
void get_valid_sites(txperms)
char txperms[];		/* array in which we set the site permissions */
{
	register int i;
	struct	passwd *pwd;
	struct	pw_gecos *pwGecos;
	char *a_grp;		/* access group as determined from passwd */

/*	pwd = getpwuid(pptr->pw_uid);  this call trashes pptr */
	pwd = pptr;
	    /* site 0 is NEVER valid */
	txperms[0] = INVALID_SITE;
	/* default to all sites are valid */
	for (i = 1; i < MAXSITE; ++i) txperms[i] = VALID_SITE;
	if (pwd == (struct passwd *)NULL) return;

	/* Now parse the pw_gecos field to get the site access perm. */
	pwGecos = parseGecos(pwd->pw_gecos);
	
	a_grp = pwGecos->siteAccessPerm;
	    /* Check for empty string which implies no restrictions */
	if (*a_grp != '\0') {
		    /* try to get access sites from library call */
		switch (get_access_sites(a_grp, txperms)) {
		case 0:
			break;	    /* successful */
		case 1:
			log("Cannot open /etc/sitegroup");
			break;
		case 2:
			log("Access group %s is unknown\n", a_grp);
			break;
		}
	}
}

/*  get_valid_shell()
 *
 *  Reads "/etc/shells" and returns true if shell is listed, else returns FALSE
 */
get_valid_shell( shell )
char *shell;
{
	char line[MAX_FN_TOTAL];
	FILE *shfile;

	if ((shfile=fopen("/etc/shells","r")) == (FILE *)NULL) {
		if (strcmp(shell,"/bin/sh") == 0)
			return(TRUE);
		else if (strcmp(shell,"/bin/csh") == 0)
			return(TRUE);
		else
			return (FALSE);
	}

	while (fgets(line,MAX_FN_TOTAL,shfile) != (char *)NULL)
	{	if( line[0] == '#' ) continue; /* speeds up comment lines */
		line[strlen(line)-1] = '\0';   /* replace '\n' */
		if (strcmp(shell,line) == 0)
			return (TRUE);
	}
	return (FALSE);
}	
#endif     /* LOCUS   */

#ifdef	SETUTMP

#include <utmp.h>

struct utmp *getutent();
/*
 * set_utmp		This routine sets the entry in the utmp file that
 *			corresponds to the PCI line.  This will make the
 *			PCI user look like a "real" user.
 *
 *	Entry:
 *		ptr	Pointer to a passwd table entry
 *
 *	Returns:
 *		Nothing
 */
set_utmp(ptr)
register struct passwd *ptr;
{
	int	pid;		     /* utmp key is either my pid or parent's */
	int	ppid = getppid();
	int	fd;
	struct	utmp	*uptr, ubuf;

	strncpy(ubuf.ut_user, ptr->pw_name,		/* get user name */
		sizeof(ubuf.ut_user));
	strncpy(ubuf.ut_line, (mydevname + sizeof("/dev/")-1),
		sizeof(ubuf.ut_line));
	ubuf.ut_pid = pid = getpid();
	ubuf.ut_type = USER_PROCESS;			/* entry type */
	time(&ubuf.ut_time);				/* set time */

	while ((uptr = getutent()) != (struct utmp *)NULL) {
		if ((uptr->ut_pid == pid) || (uptr->ut_pid == ppid)) {
			ubuf.ut_id[0] = uptr->ut_id[0];
			ubuf.ut_id[1] = uptr->ut_id[1];
			ubuf.ut_id[2] = uptr->ut_id[2];
			ubuf.ut_id[3] = uptr->ut_id[3];
			pututline(&ubuf);	/* write the record */
			endutent();
		/* now write to wtmp */
			if ((fd = open(WTMP_FILE, 2)) != -1) {
				lseek(fd, 0L, 2);
				write(fd, &ubuf, sizeof(ubuf));
				close(fd);
			}
			if (chmod(mydevname, 0600) < 0)
				log("set_utmp: chmod, dev: %s, errno: %d\n",
				    mydevname, errno);
			if (chown(mydevname, ptr->pw_uid, ptr->pw_gid) < 0)
				log("set_utmp: chown, dev: %s, errno: %d\n",
				    mydevname, errno);
			return;
		}
	}
	endutent();
	log("set_utmp: unable to find utmp entry.\n");
}
#endif	/* SETUTMP */

/* adjust HOME, LOGNAME & USER environment variables */
#define	HOME		"HOME="
#define	LOGNAME		"LOGNAME="
#define	USER		"USER="
void updateenviron(path, name)
char	*path;			/* home path */
char	*name;			/* login name */
{
	extern 	char	**environ;
	char	**envp;
	int 	homelen=strlen(HOME), 
    		lognamelen=strlen(LOGNAME), 
    		userlen=strlen(USER),
		pathlen=strlen(path),
    		namelen=strlen(name);
	int 	addmore, addhome=1, addlogname=1, adduser=1;
	int	j;

	/* adjusting existing environment variables */
	j = 0;
	while(environ[j]) {
	    	if (strncmp(environ[j], HOME, homelen) == 0) {
			if(environ[j] = malloc(homelen+pathlen+1))
		    		sprintf(environ[j], "%s%s", HOME, path);
			addhome = 0;
	    	}
	    	else if (strncmp(environ[j], LOGNAME, lognamelen) == 0) {
			if(environ[j] = malloc(lognamelen+namelen+1))
		    		sprintf(environ[j], "%s%s", LOGNAME, name);
		 	addlogname = 0;
	    	}
	    	else if (strncmp(environ[j], USER, userlen) == 0) {
			if(environ[j] = malloc(userlen+namelen+1))
		    		sprintf(environ[j], "%s%s", USER, name);
			adduser = 0;
	    	}
	    	j ++;
	}

	addmore = addhome + addlogname + adduser;
	log("Add %d more environments, %d %d %d\n", addmore, addhome, addlogname, adduser);
	if (addmore && 
	    (envp = (char **) malloc(sizeof(char *)*(j+addmore+1)))) {
		/* move the pointers to environment variables to envp */
		j = 0;
		while(environ[j]) {
			envp[j] = environ[j];
			j ++;
		}
		/* add new environment variables */
	        if (addhome) 
		    	if(envp[j] = malloc(homelen+pathlen+1)) {
		    		sprintf(envp[j], "%s%s", HOME, path);
		    		j ++;
		    	}
	    	if (addlogname) 
		    	if(envp[j] = malloc(lognamelen+namelen+1)) {
		    		sprintf(envp[j], "%s%s", LOGNAME, name);
		    		j ++;
		    	}
	        if (adduser) 
		    	if(envp[j] = malloc(userlen+namelen+1)) {
		    		sprintf(envp[j], "%s%s", USER, name);
		    		j ++;
	            	}
	    	envp[j] = (char *) NULL;
		environ = envp;
	}
}
