#ident	"@(#)pcintf:bridge/p_termout.c	1.1.1.4"
#include	"sccs.h"
SCCSID(@(#)p_termout.c	6.7	LCC);	/* Modified: 2/20/92 14:30:46 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

/*			Terminal Output Process				*/

#include "sysconfig.h"

#include <errno.h>
#include <signal.h>

#if defined(ETHNETPCI)
#	define	NDATA_ADDR	&ndata
#	ifdef	ETHLOCUS
#		include <eth.h>		/* LOCUS Ethernet structs/constants */
#	endif	/* ETHLOCUS */
#	ifdef	ETH3BNET		/* Ethernet 3b2 - Sys V */
#		include <ni.h>		/* AT&T Ethernet structs/constants */
#	endif	/* ETH3BNET */
#endif /* ETHNETPCI */

#ifdef TCA
/* I don't believe this is actually used [ hjb 05/16/85 ] */
#define	NDATA_ADDR	&ndata
#endif /* TCA */

#ifdef RS232PCI
#if	defined(SYS5) || defined(ULTRIX)
#include	<termio.h>
#endif	/* SYS5  || ULTRIX */
#endif /* RS232PCI */

#ifdef	BERKELEY42
#include	<sys/time.h>
#endif

#ifdef	AIX_RT
#include	<pty.h>
#endif	/* AIX_RT */

#include <lmf.h>

#include "pci_types.h"
#include "common.h"
#include "const.h"
#include "log.h"

#if defined(MIN)
#	undef	MIN
#endif
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))

int		main		PROTO((int, char **));
LOCAL int	ackWait		PROTO((int, struct ackcontrol *, unsigned char *));
#if defined(ETHNETPCI) || defined(TCA)
LOCAL int	check_ack	PROTO((void));
#endif

extern void	pckframe	PROTO((struct output *, int, int, unsigned char, int, int, int, int, int, int, int, int, int, int, struct stat *));

/*			    Global Structures				*/

extern FILE	
	    *logFile;			/* Pointer to Error logfile */
static unsigned short
            AckExpected = 0,            /* RD: next ack we expect */
            NextFrameSend = 0;          /* Sequence of next frame to send */
static struct ackcontrol
	    InitControl;		/* used for init in sig_catcher() */
static int	
	    initial_stream_size,	/* RD: stream size from init */
	    pty_read_flg,		/* RD: indicates if we read the PTY */
            semaphore,                  /* RD: semaphore for piggy back acks */
	    stream_size,		/* RD: requested stream size from PC */
	    rd_reinitialized = FALSE,	/* RD: indicate if reinitilized */
	    r_cnt;			/* RD: adjusted read count */

int
	    to_seqnum,			/* Sequence number of last frame */
	    rexmttime,			/* Timeout for this retransmit */
#if	defined(ETHNETPCI) || defined(TCA)
	    rexmtcnt,			/* Counter for retransmissions */
	    rexmtdone,			/* flag for when alarm signal is rcvd */
#ifdef	NOTIMEOUTS
	    rexmtign,			/* number of ignored excessive rexmts */
#endif	/* NOTIMEOUTS */
#endif	/* ETHNETPCI || TCA */
#ifdef ETHNETPCI
	    netdesc,			/* File descriptor of ethernet */
#endif /* ETHNETPCI */
#ifdef RS232PCI
	    ttydesc,			/* File descriptor of tty input */
	    sent,			/* Outstanding character sent count */
	    brg_seqnum,			/* Dummy variable */
	    swap_how,			/* Dummy variable */
#endif /* RS232PCI */
#ifdef	TCA
	    tcadesc,
#endif	/* TCA */
	    pipedesc,			/* File descriptor of pipe */
	    ptydesc,			/* File descriptor of pty */
	    length;			/* Length of last output frame */


#ifdef RS232PCI
struct	termio
	    ttymodes;			/* TTY modes structure */
#endif
char
	    copyright[] =		"PC BRIDGE TO UNIX FILE SERVER\
COPYRIGHT (C) 1984, LOCUS COMPUTING CORPORATION.  ALL RIGHTS RESERVED.  USE OF\
THIS SOFTWARE IS GRANTED ONLY UNDER LICENSE FROM LOCUS COMPUTING CORPORATION.\
ANY UNAUTHORIZED USE IS STRICTLY PROHIBITED.";

struct  ni2 ndata;                      /* Ethernet driver header */

#ifdef ETHNETPCI
struct output 
	    out;			/* Output buffer for data */

#ifndef	RS232PCI
struct emhead    
	    *rdhead;                     /* RD: reliable delivery header */
#endif

unsigned char 
            tcbn = TCB_UK;               /* RD: tcbn from connect */

#ifdef	ETHLOCUS
struct	lpstatus
	lp;
#endif	/* ETHLOCUS */

#ifdef	ETH3BNET                             /* Ethernet 3b2 - Sys V */
NI_PORT
	lp;
#endif

#endif /* ETHNETPCI */

#ifdef TCA
struct output 
	    out;			/* Output buffer for data */
#endif /* TCA */

#ifdef RS232PCI
char
	    out[MAX_READ];

#ifdef RS232_7BIT
int
	    using_7_bits = 0;		/* Serial line only supports 7 bits */
#endif /* RS232_7BIT */
#endif /* RS232PCI */

extern int rd_flag;                /* Normal vs reliable delivery defined */

char
	*logArg;			/* Debug argument pointer */

#if	defined(RS232PCI) || defined(TCA)
#define	REXMTTIME	20		/* initial rexmt timeout period */
#else
/*
 *	Please note that on 3b2 with 42 extensions I increased this time to
 *	3. Maybe on other UDP/IP systems with RD this time needs to be increased
 *	check it out and see! JD
 */
#define	REXMTTIME	2		/* initial rexmt timeout period */
#endif	/* RS232PCI || TCA */

#define REXMTCNT	2		/* Starting count not max see TRIES */

char *myname;

#if	!defined(RS232PCI)
struct rd_shared_mem			/* RD: shared memory pointer */
	*sh_mem;

#ifdef  RD3				/* XENIX Sys 3 - Rel. Del. */
char
	*rd_sdopen(),
	*rd_shmid;			/* shared mem id passed from dossvr */
#else
int
	rd_shmid;
#endif
#endif	  /* Xenix or Eth3bnet - reliable delivery  */
unsigned short FrameExpected; 
/*
 * Note FrameExpected is a copy of sh_mem->FrameExpected. This is so that
 * the contents of the shared memory can be used in system calls without
 * holding the shared memory open for undue long periods. B.F.
 */


 
/*
 * main() -
 */
 
main(argc, argv)
int argc;
char *argv[];
{

register int
#if defined(ETHLOCUS) || defined(ETH3BNET)
	i,				/* Loop counter */
#endif
	swap_how,
	status;				/* Contains return code from sys call */
struct  ackcontrol
	rcontrol;                       /* RD: from pipe inband control data */
unsigned char
	control = RFNM;			/* Contains inband control data */
int
	argN;
char
	*arg;
#ifndef	RS232PCI
struct emhead 
        *sendhead = (struct emhead *) out.text;
#endif

	/* determine machine byte ordering */
	byteorder_init();

	/* initialize NLS */
	nls_init();

	rd_flag = 0;			/* init reliable delivery flag */
	myname = argv[0];
	if (*myname == '\0')
		myname = "unknown";

	for (argN = 1; argN < argc; argN++) {
		arg = argv[argN];
		if (*arg != '-')
			continue;

		switch(arg[1]) {
		default:		/* Unknown */
			break;

		case 'e':		/* Error descriptor (for logFile) */
			logDOpen(atoi(&arg[2]));
			break;

		case 'p':		/* Pipe descriptor */
			pipedesc = atoi(&arg[2]);
			break;

		case 'y':		/* Pty descriptor */
			ptydesc = atoi(&arg[2]);
			break;

		case 'D':		/* Debug level */
			logArg = &arg[2];
			dbgSet(strtol(logArg, NULL, 16));
			break;

		case 's':		/* Byte swap code */
			swap_how = atoi(&arg[2]);
			break;

#ifdef	ETHNETPCI
		case 'a':		/* Ethernet address */
			nAddrGet(&arg[2], ndata.dst);
			/* Prevent net address from showing id ps output */
			arg[2] = '\0';
			break;

		case 'n':		/* Network descriptor */
			netdesc = atoi(&arg[2]);
			break;

		case 'R':
			rd_flag = 1;

/*
 * Reliable delivery has been asked for, but can we do it?
 */
#if	defined(RD3)			/* XENIX Sys 3 - Rel. Del. */
			if ((semaphore = rdsem_open()) == -1)
			     return 1;        /* can't do reliable delivery */ 
			if ((rd_shmid = rd_sdopen()) < 0)
			     fatal(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("P_TERMOUT1",
				"termout: shmat: errno: %1\n"),
				"%d", errno));
#else	                             /* Ethernet 3b2 - Sys V */
			semaphore = atoi(&arg[2]);
		log("termout: semaphore: %d\n", semaphore);
			if ((rd_shmid = rd_sdopen()) < 0)
				fatal(lmf_get_message("P_TERMOUT2",
					"termout: no shared memory.\n"));
#endif
			sh_mem = (struct rd_shared_mem *)rd_sdenter(rd_shmid);
			if (sh_mem == 0)
			     fatal(lmf_get_message("P_TERMOUT3",
				"Couldn't enter segment.\n"));

                        rd_flag = 1;
			sh_mem->FrameExpected = FrameExpected = 0;
			break;

		case 'S':		/* stream size variable */
			if (rd_flag) { 	/* only valid for reliable delivery */
				stream_size = atoi(&arg[2]);
				initial_stream_size = stream_size;
				log("termout: Stream size: %d\n", stream_size);
			}
			break;

#endif	/* ETHNETPCI */
#ifdef	TCA
		case 't':		/* Tty descriptor */
			tcadesc = atoi(&arg[2]);
			break;
#endif  /* TCA */
#ifdef	RS232PCI
		case 't':		/* Tty descriptor */
			ttydesc = atoi(&arg[2]);
			break;
#endif  /* RS232PCI */
		}
	}


/* Enable SIG_DBG1 for debug toggle */
    signal(SIGALRM, sig_catcher);
    signal(SIG_DBG1, sig_catcher);
#if  !defined(RS232)
	/*
	 * Now BSD4.2 systems will get SIGUSR2
	 */
    signal(SIGUSR2, sig_catcher);	/* used to reset termout from dossvr */
#endif

    if (dbgCheck(~0) && logFile == NULL)
	logOpen(DOSOUT_LOG, getpid());

#if	defined(ETHNETPCI) || defined(TCA)

#ifdef ETHNETPCI
#if !defined(UDP42) && !defined(UDP41C)
/* Get source address from driver */
    while ((ioctl(netdesc, NIGETA, &lp)) < 0) {
	if (errno == EINTR)
	    continue;
	fatal(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("P_TERMOUT5",
		"NISETA Error: %1\n"),
		"%d", errno));
    }

#ifdef	ETHLOCUS
    for (i = 0; i < SRC; i++)
	ndata.src[i] = lp.netid[i];

/* Load bridge domain constant */
    ndata.type[0] = PCIAID1;
    ndata.type[1] = PCIAID2;

#endif	/* ETHLOCUS */

#ifdef	ETH3BNET                             /* Ethernet 3b2 - Sys V */

    for (i = 0; i < SRC; i++)
	ndata.src[i] = lp.srcaddr[i];

    ndata.type[0] = 0x76;
    ndata.type[1] = 0x77;
#endif	/* ETH3BNET */
#endif /* !UDP42 && !UDP41C */

    /* Let the network interface know which descriptor to use */
    netUse(netdesc);

#endif /* ETHNETPCI */

/*
 * Read on the PTY and send the first frame.
 */
       
if (rd_flag) {
	/*
	 *	Under BSD, PTY reads can return 1024 bytes (or more).
	 *	Since the text area of the packet is 1024 bytes, this
	 *	leaves no room for the emhead structure.  The following code
	 *	ensures that the buffer will not be overrun.  (We have always
	 *	gotten away with this, since no Sys V port to date
	 *	has returned anywhere near 1024 bytes on a PTY read.
	 * 	(Why the -1 is needed isn't known; maybe a DOS side bug.
	 *	Without the -1, we stop getting acks.)
	 */
	r_cnt = MIN(stream_size, MAX_OUTPUT - sizeof (struct emhead) - 1);

    log("RD: Reading from pty!\n");
    rdhead = (struct emhead *)out.text;/* RD: init pointer into r.d. header */

#ifdef	AIX_RT
	/* 
	 * The following code keeps sending ioctl to pty util data 
	 * becomes available at the pty.
	 * This is a work around for the pty bug.  When a process tries to
	 * read data from pty before there are any data available.  The 
	 * process will be put to sleep.  But when data becomes available
	 * the pty/kernel does not wake up or send signal (SIGPTY) to the 
	 * process that was put to sleep.  This cause the process to hang
	 * forever.  
	 */
	do  
		ioctl(ptydesc, PTYIOR, &status);
	while (status <= 0);

#endif	/* AIX_RT */
/*
 * get stuff from pty and put behind emheader info
 */
    while ((status = read(ptydesc, out.text + sizeof (struct emhead), 
                          r_cnt)) < 0) {
	int saved_errno;

	if (errno == EINTR)
	    continue;
	saved_errno = errno;
	rdsem_unlnk(semaphore);		/* remove semaphore */
	rd_shmdel(rd_shmid);		/* remove shared memory */
	fatal(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("P_TERMOUT6", "PTY Read error: %1\n"),
		"%d", saved_errno));
    }
    r_cnt -= status;			/* adjust r_cnt for next read */
}
else {
#ifdef	AIX_RT
    /* Wait till data becomes available at the pty port */
    do  
	ioctl(ptydesc, PTYIOR, &status);
    while (status <= 0);
#endif	/* AIX_RT */

    /*  read the data */
    while ((status = read(ptydesc, out.text, sizeof out.text)) < 0) {
	int saved_errno;

	if (errno == EINTR)
	    continue;
	saved_errno = errno;
	rdsem_unlnk(semaphore);		/* remove semaphore */
	rd_shmdel(rd_shmid);		/* remove shared memory */
	fatal(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("P_TERMOUT7","PTY Read error: %1\n"),
		"%d", saved_errno));
    }
}

    /* Quit on end of file */
    if (status == 0 && r_cnt != 0) {
	rdsem_unlnk(semaphore);		/* remove semaphore */
	rd_shmdel(rd_shmid);		/* remove shared memory */
	log("termout: PTY EOF; Bye\n");
	return 0;
    }

if (rd_flag)
    log("pty read: %d chars: %.*s", status, 
               status, out.text+sizeof( struct emhead));
else
    log("pty read: %d chars: %.*s", status, status, out.text);
    log("\nSending to: %s\n", nAddrFmt((unsigned char *)ndata.dst));

if (rd_flag) {
    if (check_ack() == FAILURE) {
	     log("No piggy backed ack.\n");
	     pckRD( rdhead, RD_DATA, NextFrameSend, 0, 0, 
	            tcbn, RD_VERSION );
    }
    else
    {
	     log("Piggy backing %d\n",FrameExpected);
	     pckRD( rdhead, RD_PB, NextFrameSend, FrameExpected++, 0, 
	            tcbn, RD_VERSION );
             log("New FrameExpected = %d\n",FrameExpected);
	     sh_mem->FrameExpected = FrameExpected;
             rdsem_relinq(semaphore); 
    }
    pckframe(&out, SHELL, ++to_seqnum, (unsigned char)NULL, NEW, SUCCESS, NO_DES, NO_CNT, 
             status+sizeof (struct emhead), NO_MODE, NO_SIZE, 
             NO_OFF, NO_ATTR, NO_DATE, (struct stat *)NULL);
    log("XXX: Sending data to pc(1), cnt:%d,Frame=%d, tcbn=%d, data=%.*s\n",
	       status,rdhead->dnum, rdhead->tcbn, status,
               out.text+sizeof(struct emhead));
}
else
    pckframe(&out, SHELL, ++to_seqnum, (unsigned char)NULL, NEW, SUCCESS, NO_DES, NO_CNT, 
             status, NO_MODE, NO_SIZE, NO_OFF, NO_ATTR, NO_DATE,
	     (struct stat *)NULL);

    length = xmtPacket(&out, NDATA_ADDR, swap_how);

    rexmtcnt = REXMTCNT;
    rexmttime = REXMTTIME;
    rexmtdone = 0;
    alarm(rexmttime);

#endif	/* ETHNETPCI || TCA */
#ifdef RS232PCI

    while ((status = read(ptydesc, out, sizeof out / 2)) < 0) {
	if (errno == EINTR)
	    continue;
	fatal(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("P_TERMOUT8","PTY Read error: %1\n"),
		"%d", errno));
    }

    /* Quit on PTY end of file */
    if (status == 0) {
	log("termout: PTY EOF; Bye\n");
	return 0;
    }

    sent = stuff(out, status);

    debug(0, ("sent %d status %d length %d; chars: %.*s\n", sent,
	status, length, sent, out));

    while ((status = write(ttydesc, out, sent)) < 0) {
        if (errno == EINTR)
	    continue;
        fatal(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("P_TERMOUT9","TTY Write error: %1\n"),
		"%d", errno));
    }

    sent = status;

    if (sent >= MAX_BURST) {
	while ((write(ttydesc, SYNCNULL, 2)) < 0) {
	    if (errno == EINTR)
		continue;
	    fatal(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("P_TERMOUT10","TTY Write error: %1\n"),
		"%d", errno));
	}

	debug(0, ("Sending flow ctl: sent %d\n", sent));

    /* Set timer for resending of "SYNC2 NULL" */
	alarm(7);
    }

    rexmttime = REXMTTIME;

#endif /* RS232PCI */


/*
 * Read control stream from input process.  If RFNM does not arrive
 * within timeout period retransmit the last frame.
 */
    for (;;) {

#if	defined(ETHNETPCI) || defined(TCA)

    /* If RFNM doesn't arrive retransmit the previous frame */

	if (ackWait(pipedesc, &rcontrol, &control) < 0) {
	    alarm(0);
	    if (rd_flag) {
		    if (rd_reinitialized) {
			rd_reinitialized = FALSE;	/* normal state */
			rexmttime = REXMTTIME;	/* reset timer/counter */
			rexmtcnt = REXMTCNT;
			goto rd_start_again;	/* I know - but no other way */
		    }
		    log("Resend seq %d\n", NextFrameSend);
	    } else
		log("Resend seq %d\n", to_seqnum);
	    if (rexmtcnt > TRIES) {
#ifndef	NOTIMEOUTS
		fatal(lmf_get_message("P_TERMOUT11","Too many reXMITS\n"));
#else	/* NOTIMEOUTS */
		rexmtign++;
		log("Ignor reXMT limit for %dth time\n", rexmtign);
		rexmtcnt=1;
		rexmttime=REXMTTIME-1;
#endif	/* NOTIMEOUTS */
	    }

	    ++rexmtcnt;
	    rexmttime += rexmttime;
            if (rd_flag)
		sendhead->code = RD_DATA; /* no piggy backed acks allowed */
	    reXmt(&out, length);
	    rexmtdone = 0;
	    alarm(rexmttime);
	    continue;
	} else if (rd_flag) {

            log("RDT: code ack, code %d\n",rcontrol.code); 
            if ( rcontrol.code == RD_ACK &&
                 rdtest( rcontrol.num, AckExpected) == 0) {
		log("RDT: Good ack:%d\n",AckExpected); 
		AckExpected++;
	        NextFrameSend++;
	        tcbn=rcontrol.tcbn;
	    } else {
/*
 *	If the ack you actually receive is less than the AckExpected then
 *	we'll just chuck it away. Basically AckExpected only gets incremented
 *	when the correct ack has been received. The problem showed up on a
 *	3b2 with 42 extensions. What happened was the PC would timeout and 
 *	rexmt an ack but by this time the ack had already been processed.
 *	This caused a lockstep problem and things got bery bery slow.
 */
		if (rcontrol.code == RD_DATA ||
		    (rdtest(rcontrol.num, AckExpected) == -1))
			continue;
		log("RDT: BAD ACK, got:%d, wnt:%d\n",rcontrol.num,AckExpected);
		alarm(0);
		rexmtdone = 1;		/* force retransmission */
		continue;
	    }
	}

    /* Reset timer/counter */
	rexmttime = REXMTTIME;
	rexmtcnt = REXMTCNT;
	alarm(0);
	rexmtdone = 0;

#endif	/* ETHNETPCI || TCA */

#ifdef RS232PCI
	if (sent >= MAX_BURST) {
	    while ((status = ackWait(pipedesc, &rcontrol, &control)) < 0) {
		if (errno == EINTR)
		    continue;
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("P_TERMOUT12",
			"PIPE Read error; %1\n"), 
			"%d", errno));
	    }

    /* Is this really Flow Control? */
	if (control != SYNC_2)
	    continue;
		    
	debug(0, ("Got flow control\n"));

	/* Reset state */
	    alarm(0);
	    sent = 0;
	}

#endif /* RS232PCI */

    /* Read PTY for output from the shell packetize it and send to PC */
#if	defined(ETHNETPCI) || defined(TCA)
rd_start_again:
	rd_reinitialized = FALSE;		/* normal state */
        status = -1;                           
if (rd_flag) {
	log("RDT: PTY read count (r_cnt): %d\n", r_cnt);
	pty_read_flg = FALSE;	/* indicates we haven't read the PTY */
	if (r_cnt > 0) {	/* if greater than zero let's read */

#ifdef	AIX_RT
		/* Wait till data becomes available at the pty port */
		do  
			ioctl(ptydesc, PTYIOR, &status);
		while (status <= 0);
#endif	/* AIX_RT */

		/*  read the data */
            while ((status = read(ptydesc, out.text + sizeof (struct emhead),
                          r_cnt )) < 0) {
		int saved_errno;

#if	defined(RIDGE) || defined(ULTRIX)
		if (errno == EINTR || errno == EIO)
              	    continue;
#else
		if (errno == EINTR)
              	    continue;
		saved_errno = errno;
#endif	/* !RIDGE || !ULTRIX */
		rdsem_unlnk(semaphore);		/* remove semaphore */
		rd_shmdel(rd_shmid);		/* remove shared memory */
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("P_TERMOUT13",
			"PTY Read error: %1\n"),
			"%d", saved_errno));
	    }
	pty_read_flg = TRUE;	/* we have read something from the PTY */
	r_cnt -= status;
	}
}
else {
#ifdef	AIX_RT
	/* Wait till data becomes available at the pty port */
	do  
		ioctl(ptydesc, PTYIOR, &status);
	while (status <= 0);
#endif	/* AIX_RT */

	/* read the data */
	while ((status = read(ptydesc, out.text, sizeof out.text)) < 0) 

#endif	/* ETHNETPCI || TCA */
#ifdef RS232PCI
	while ((status = read(ptydesc, out, sizeof out / 2)) < 0) 
#endif /* RS232PCI */
	{
	    int saved_errno;

	    if (errno == EINTR)
		continue;
	    saved_errno = errno;

#if	!defined(RS232PCI) 
                                 /* RD3 - XENIX Sys 3, Rel Del. */

	    rdsem_unlnk(semaphore);		/* remove semaphore */
	    rd_shmdel(rd_shmid);		/* remove shared memory */
#endif
	    fatal(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("P_TERMOUT14","PTY Read error: %1\n"),
		"%d", saved_errno));
	}
#if	defined(ETHNETPCI) || defined(TCA)
}
#endif	/* ETHNETPCI || TCA */

	/* Quit on EOF from pty */
#if	!defined(RS232PCI)
                                    /* RD3 - XENIX Sys 3, Rel Del. */
    	if (status == 0 && r_cnt != 0) {
		rdsem_unlnk(semaphore);		/* remove semaphore */
		rd_shmdel(rd_shmid);		/* remove shared memory */
		log("termout: PTY EOF; Bye\n");
		return 0;
    	}
#else
    	if (status == 0) {
		log("termout: PTY EOF; Bye\n");
		return 0;
	}
#endif

#if	defined(ETHNETPCI) || defined(TCA)

if (rd_flag) {
    if (pty_read_flg)
	log("RDT Data: status %d chars: %.*s\n", status, status,
             out.text+sizeof (struct emhead));
}
else
	debug(0, ("Data: status %d len %d; chars: %.*s\n", status,
	    length, length, out.text));

#endif	/* ETHNETPCI || TCA */
#ifdef	RS232PCI
	debug(0, ("Data: status %d len %d; chars: %.*s\n", status,
	    length, length, out));
#endif	/* RS232PCI */


#if	defined(ETHNETPCI) || defined(TCA)

if (rd_flag) {
/*
 * Put emheader together, then put emulation header together
 */
        if (check_ack() == FAILURE) {
	     log("No piggy back ack.\n");
	         pckRD( rdhead, RD_DATA, NextFrameSend, 0, 0, 
	             tcbn, RD_VERSION );
	}
        else 
        {
	     log("Piggy Backing %d\n", FrameExpected);
	     pckRD( rdhead, RD_PB, NextFrameSend, FrameExpected++, 0, 
			tcbn, RD_VERSION );
             log("New FrameAck = %d\n",FrameExpected);
	     sh_mem->FrameExpected = FrameExpected;
             rdsem_relinq(semaphore); 
        }
	if (!pty_read_flg)	/* nothing was read therefore status is zero */
		status = 0;
	pckframe(&out, SHELL, ++to_seqnum, (unsigned char)NULL, NEW, SUCCESS, NO_DES, NO_CNT, 
                 status+sizeof (struct emhead), NO_MODE, NO_SIZE, NO_OFF, 
                 NO_ATTR, NO_DATE, (struct stat *)NULL);
        out.text[status + sizeof (struct emhead)] = '\0'; /* Null terminate */
        log("RDT: Sending data to pc(2), Frame=%d, data:=%.*s\n",
	           rdhead->dnum,status, 
                   out.text+sizeof(struct emhead));

}
else {
	pckframe(&out, SHELL, ++to_seqnum, (unsigned char)NULL, NEW, SUCCESS, NO_DES, NO_CNT, 
                 status, NO_MODE, NO_SIZE, NO_OFF, NO_ATTR, NO_DATE,
		 (struct stat *)NULL);
	out.text[status] = '\0';		/* Null terminated */
}

		length = xmtPacket(&out, NDATA_ADDR, swap_how);
		rexmtdone = 0;
		alarm(rexmttime);

#endif	/* ETHNETPCI || TCA */
#ifdef RS232PCI

	length = stuff(out, status);

	while ((status = write(ttydesc, out, length)) < 0) {
	    if (errno == EINTR)
		continue;
	    fatal(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("P_TERMOUT15","TTY Write error: %1\n"),
		"%d", errno));
	}

	sent += status;

	if (sent >= MAX_BURST) {
	    if ((write(ttydesc, SYNCNULL, 2)) < 0) {
		if (errno == EINTR)
		    continue;
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("P_TERMOUT16",
			"TTY Write error: %1\n"),
			"%d", errno));
	    }

	    debug(0, ("Send flow ctl\n"));

	/* Set timer for resending of "SYNC2 NULL" */
	    alarm(7);
	}

#endif /* RS232PCI */

    }  /* end of for (;;) */  
}



/*
 * sig_catcher() -		Signal catcher.
 */

void
sig_catcher(signum)
register int signum;
{
    switch (signum) {
	case SIG_DBG1:
	    newLogs(DOSOUT_LOG, getpid(), NULL, NULL);
	    if (logArg != NULL)
		sprintf(logArg, "%04x", dbgEnable);
	    break;

#if	!defined(RS232PCI)
                         /* RD3 - XENIX Sys 3, Rel Del. */
	case SIGUSR2 :
		/* we will now see if we should reset */
	    log("RDT: received SIGUSR2\n");
	    do {
		if (read(pipedesc, &InitControl, sizeof(struct ackcontrol)) < 0)
		    log("RDT: pipe read error during SIGUSR2\n");

		if (InitControl.code == RD_INITTERM) {
		    AckExpected = 0;	/* re-init termout variables */
		    NextFrameSend = 0;
		    FrameExpected = 0;
                    r_cnt = 0;
		    stream_size = initial_stream_size; /* re-init stream size */
		    rexmttime = REXMTTIME;	/* reset timer/counter */
		    rexmtcnt = REXMTCNT;
		    rd_reinitialized = TRUE;
		    log("RDT: re-initialized\n");
		}
		} while (InitControl.code != RD_INITTERM);
	    break;
#endif
	case SIGALRM:

#if	defined(ETHNETPCI) || defined(TCA)
	    rexmtdone++;
#endif	/* ETHNETPCI || TCA */
#ifdef RS232PCI
	    while ((write(ttydesc, SYNCNULL, 2)) < 0) {
		if (errno == EINTR)
		    continue;
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("P_TERMOUT17",
			"TTY Write error: %1\n"),
			"%d", errno));
	    }

	    debug(0, ("Alarm: Resend flow ctl\n"));

	/* Set timer for resending of "SYNC2 NULL" */
	    alarm(7);
#endif /* RS232PCI */

	    break;
    }
    signal(signum, sig_catcher);
}
 

/*
   ackWait: Read an acknowledgement from the dossvr pipe
	    EXCEPT for RELIABLE DELIVERY which uses this
	    entry point to set the flow control variable
*/

LOCAL int
ackWait(ackDesc, rdackPtr, ackPtr )
int
	ackDesc;			/* Descriptor from which ack is read */
struct ackcontrol *rdackPtr;            /* reliable delivery pointer */
unsigned char
	*ackPtr;			/* Return ack code here */
{  
int count;

#if defined(BERKELEY42) && !defined(ULTRIX)
int
	selBits,			/* Select descriptor bit mask */
	selRet;				/* Select() return */
struct timeval
	ackTime;			/* PCI termout retransmit time limit */

	/* Set up the time timeval */
	ackTime.tv_sec = rexmttime;
	ackTime.tv_usec = 0;

	/* Set up selBits for the descriptor ackDesc */
	selBits = 1 << ackDesc;

	/* Wait for ack to become available */
	selRet = select(ackDesc + 1, &selBits, (int *)NULL, (int *)NULL,
		&ackTime);	

	/* < 0 for no ack available, > 0 for ack ready */
	if (selRet <= 0)
		return -1;
	else {
		read(ackDesc, ackPtr, 1);
		return 1;
	}
#else
	if (rd_flag) {
#if	defined(ETHNETPCI) || defined(TCA)
	    if (rexmtdone)
		return -1;
#endif	/* ETHNETPCI || TCA */
            count = read(ackDesc, rdackPtr, sizeof (struct ackcontrol));
	    log("RDT:Read from pipe cnt:%d\n",count);
            if (count<0) {
		int saved_errno;

		saved_errno = errno;
		log("ERROR CODE:%d\n",saved_errno);
		if (saved_errno != EINTR)
			fatal(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("P_TERMOUT18",
				"IO ERROR ON PIPE. errno: %1\n"),
				"%d", saved_errno));
		return -1;
		}
	    stream_size = rdackPtr->ssiz;		/* get stream size */
	    r_cnt = MIN(stream_size, MAX_OUTPUT - sizeof (struct emhead) - 1);
	    log("RDT: Stream size from pipe: %d   r_cnt: %d\n", stream_size, r_cnt);
	} else
	    read(ackDesc, ackPtr, 1);

	return 1;

#endif /* BERKELEY42 */
}

/*****************************************************************************/
/*                                                                           */
/* Function: check_ack()                                                     */
/*                                                                           */
/* Purpose: Check to see if the termout process has to send an ack for data  */
/*          received by the dossvr process running reliable delivery emul.   */
/*                                                                           */
/* Parameters: none                                                          */
/*                                                                           */
/* Returns: SUCCESS = Need to send Ack. FAILURE = No Ack required            */
/*                                                                           */
/* Calls:   rdsem_access()                                                   */
/*                                                                           */
/* Data Examined: sh_mem->kick_ack, part of a shared memory segment          */
/*                                                                           */
/* Data Modified: FrameExpected, a copy of sh_mem->FrameExpected             */
/*                                                                           */
/* Side Affects: A semaphore is used here. If check_ack() can get the        */ 
/*               semaphore, the semaphore is checked for value. If the value */
/*               is true then the semaphore indicates need to ack. Therefore */
/*               when this function returns SUCCESS, the semaphore is re-    */
/*               tained by the caller who must relinquish it upon completion */
/*               of its own processing.                                      */
/*****************************************************************************/

#if defined(ETHNETPCI) || defined(TCA)
LOCAL int
check_ack()
{
                               /* RD3 - XENIX Sys 3, Rel Del. */
	if (rdsem_access(semaphore)==-1)
		return( FAILURE );   /* access denied */
/*
 * We have the shared memory access. Now check the value of kick_ack.
 */
        if (sh_mem->kick_ack != 1)
        {
		rdsem_relinq( semaphore );
		return( FAILURE );
        }
/* 
 * Indicate in the shared memory that the ack has been kicked out the door 
 * to the PC.
 */
	sh_mem->kick_ack = 0;
	FrameExpected = sh_mem->FrameExpected-1;
	return( SUCCESS );    /* semaphore now owned */
}
#endif	/* ETHNETPCI || TCA */
