#ident	"@(#)OSRcmds:sh/defs.h	1.1"
#pragma comment(exestr, "@(#) defs.h 25.3 95/03/09 ")
/*
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1995 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)sh:defs.h	1.5.1.1" */
/*
 *	UNIX shell
 */

/*
 * Modification History
 *
 *	L000	scol!markhe	15th March, 1992
 *	- added prototypes for new functions in string.c and new message
 *	S001	sco!vinces	16th March 1995
 *	- added logical path processing to argument syntax
 */

/* error exits from various parts of shell */
#define 	ERROR		1
#define 	SYNBAD		2
#define 	SIGFAIL 	2000
#define	 	SIGFLG		0200

/* command tree */
#define 	FPRS		0x0100
#define 	FINT		0x0200
#define 	FAMP		0x0400
#define 	FPIN		0x0800
#define 	FPOU		0x1000
#define 	FPCL		0x2000
#define 	FCMD		0x4000
#define 	COMMSK		0x00F0
#define		CNTMSK		0x000F

#define 	TCOM		0x0000
#define 	TPAR		0x0010
#define 	TFIL		0x0020
#define 	TLST		0x0030
#define 	TIF			0x0040
#define 	TWH			0x0050
#define 	TUN			0x0060
#define 	TSW			0x0070
#define 	TAND		0x0080
#define 	TORF		0x0090
#define 	TFORK		0x00A0
#define 	TFOR		0x00B0
#define		TFND		0x00C0

/* execute table */
#define 	SYSSET		1
#define 	SYSCD		2
#define 	SYSEXEC		3

#ifdef RES	/*	include login code	*/
#define 	SYSLOGIN	4
#else
#define 	SYSNEWGRP 	4
#endif

#define 	SYSTRAP		5
#define 	SYSEXIT		6
#define 	SYSSHFT 	7
#define 	SYSWAIT		8
#define 	SYSCONT 	9
#define 	SYSBREAK	10
#define 	SYSEVAL 	11
#define 	SYSDOT		12
#define 	SYSRDONLY 	13
#define 	SYSTIMES 	14
#define 	SYSXPORT	15
#define 	SYSNULL 	16
#define 	SYSREAD 	17
#define		SYSTST		18

#ifndef RES	/*	exclude umask code	*/
#define 	SYSUMASK 	20
#define 	SYSULIMIT 	21
#endif

#define 	SYSECHO		22
#define		SYSHASH		23
#define		SYSPWD		24
#define 	SYSRETURN	25
#define		SYSUNS		26
#define		SYSMEM		27
#define		SYSTYPE  	28
#define		SYSGETOPT	29

/* used for input and output of shell */
#define 	INIO 		59

/* command to ulimit to get # of files that can be opened */
#define		NUM_FILES	4

/*io nodes*/
#define 	USERIO		10
#define 	IOUFD		15
#define 	IODOC		16
#define 	IOPUT		32
#define 	IOAPP		64
#define 	IOMOV		128
#define 	IORDW		256
#define		IOSTRIP		512
#define 	INPIPE		0
#define 	OTPIPE		1

/* arg list terminator */
#define 	ENDARGS		0

#include	"mac.h"
#include	"mode.h"
#include	"name.h"
#include	<signal.h>


/*	error catching */
extern int 		errno;

/* getopt */

extern int		optind;
extern char 		*optarg;

/* result type declarations */

#define 	alloc 		malloc

extern unsigned char				*alloc();
extern unsigned char				*make();
extern unsigned char				*movstr();
extern unsigned char				*movstrn();
extern int					strperm();	/* L000 */
extern char					*permtostr();	/* L000 */
extern struct trenod	*cmd();
extern struct trenod	*makefork();
extern struct namnod	*lookup();
extern struct namnod	*findnam();
extern struct dolnod	*useargs();
extern float			expr();
extern unsigned char				*catpath();
extern unsigned char				*getpath();
extern unsigned char				*nextpath();
extern unsigned char				**scan();
extern unsigned char				*mactrim();
extern unsigned char				*macro();
extern unsigned char				*execs();
extern unsigned char				*copyto();
extern int				exname();
extern unsigned char				*staknam();
extern int				printnam();
extern int				printro();
extern int				printexp();
extern unsigned char				readc();
extern unsigned char				nextc();
extern unsigned char				skipc();
extern unsigned char				**setenv();
extern long				time();

#define 	attrib(n,f)		(n->namflg |= f)
#define 	round(a,b)		(((int)(((char *)(a)+b)-1))&~((b)-1))
#define 	closepipe(x)	(close(x[INPIPE]), close(x[OTPIPE]))
#define 	eq(a,b)			(cf(a,b)==0)
#define 	max(a,b)		((a)>(b)?(a):(b))
#define 	assert(x)		;

/* temp files and io */
extern int				output;
extern int				ioset;
extern struct ionod		*iotemp;	/* files to be deleted sometime */
extern struct ionod		*fiotemp;	/* function files to be deleted sometime */
extern struct ionod		*iopend;	/* documents waiting to be read at NL */
extern struct fdsave	fdmap[];
extern int savpipe;

/* substitution */
extern int				dolc;
extern unsigned char				**dolv;
extern struct dolnod	*argfor;
extern struct argnod	*gchain;

/* stak stuff */
#include		"stak.h"

/* string constants */
extern unsigned char				atline[];
extern unsigned char				readmsg[];
extern unsigned char				colon[];
extern unsigned char				minus[];
extern unsigned char				nullstr[];
extern unsigned char				sptbnl[];
extern unsigned char				unexpected[];
extern unsigned char				endoffile[];
extern unsigned char				synmsg[];
extern unsigned char				openmax[];
#ifdef VPIX
extern char                             vpix[];
extern char                             vpixflag[];
extern char                             dotcom[];
extern char                             dotexe[];
extern char                             dotbat[];
#endif

/* name tree and words */
extern struct sysnod	reserved[];
extern int				no_reserved;
extern struct sysnod	commands[];
extern int				no_commands;

extern int				wdval;
extern int				wdnum;
extern int				fndef;
extern int				nohash;
extern struct argnod	*wdarg;
extern int				wdset;
extern BOOL				reserv;

/* prompting */
extern unsigned char				stdprompt[];
extern unsigned char				supprompt[];
extern unsigned char				profile[];
extern unsigned char				sysprofile[];

/* built in names */
extern struct namnod	fngnod;
extern struct namnod	cdpnod;
extern struct namnod	ifsnod;
extern struct namnod	homenod;
extern struct namnod	mailnod;
extern struct namnod	pathnod;
#ifdef VPIX
extern struct namnod    dpathnod;
#endif

extern struct namnod	ps1nod;
extern struct namnod	ps2nod;
extern struct namnod	mchknod;
extern struct namnod	acctnod;
extern struct namnod	mailpnod;

/* special names */
extern unsigned char				flagadr[];
extern unsigned char				*pcsadr;
extern unsigned char				*pidadr;
extern unsigned char				*cmdadr;

extern unsigned char				defpath[];

/* names always present */
extern unsigned char				mailname[];
extern unsigned char				homename[];
extern unsigned char				pathname[];
#ifdef VPIX
extern char                             	dpathname[];
extern unsigned char                           	*vpixdirname;
#endif

extern unsigned char				cdpname[];
extern unsigned char				ifsname[];
extern unsigned char				ps1name[];
extern unsigned char				ps2name[];
extern unsigned char				mchkname[];
extern unsigned char				acctname[];
extern unsigned char				mailpname[];

/* transput */
extern unsigned char				tmpout[];
extern unsigned char				*tmpnamo;
extern int				serial;

#define		TMPNAM 		7

extern struct fileblk	*standin;

#define 	input		(standin->fdes)
#define 	eof			(standin->feof)

extern int				peekc;
extern int				peekn;
extern unsigned char				*comdiv;
extern unsigned char				devnull[];

/* flags */
#define		noexec		01
#define		sysflg		01
#define		intflg		02
#define		prompt		04
#define		setflg		010
#define		errflg		020
#define		ttyflg		040
#define		forked		0100
#define		oneflg		0200
#define		rshflg		0400
#define		waiting		01000
#define		stdflg		02000
#define		STDFLG		's'
#define		execpr		04000
#define		readpr		010000
#define		keyflg		020000
#define		hashflg		040000
#define		logflg		0100000				/* S001 */
#define		nofngflg	0200000
#define		exportflg	0400000

extern long				flags;
extern int				rwait;	/* flags read waiting */

/* error exits from various parts of shell */
#include	<setjmp.h>
extern jmp_buf			subshell;
extern jmp_buf			errshell;

/* fault handling */
#include	"brkincr.h"

extern unsigned			brkincr;
#define 	MINTRAP		0
#define 	MAXTRAP		20

#define 	TRAPSET		2
#define 	SIGSET		4
#define 	SIGMOD		8
#define 	SIGCAUGHT	16

extern void				fault();
extern BOOL				trapnote;
extern unsigned char				*trapcom[];
extern BOOL				trapflg[];

/* name tree and words */
extern unsigned char				**environ;
extern unsigned char				numbuf[];
extern unsigned char				export[];
extern unsigned char				duperr[];
extern unsigned char				readonly[];

/* execflgs */
extern int				exitval;
extern int				retval;
extern BOOL				execbrk;
extern int				loopcnt;
extern int				breakcnt;
extern int				funcnt;

/* messages */
extern unsigned char				mailmsg[];
extern unsigned char				coredump[];
extern unsigned char				badopt[];
extern unsigned char				badparam[];
extern unsigned char				unset[];
extern unsigned char				badsub[];
extern unsigned char				nospace[];
extern unsigned char				nostack[];
extern unsigned char				notfound[];
extern unsigned char				badtrap[];
extern unsigned char				baddir[];
extern unsigned char				badshift[];
extern unsigned char				restricted[];
extern unsigned char				execpmsg[];
extern unsigned char				notid[];
extern unsigned char 				badulimit[];
extern unsigned char				wtfailed[];
extern unsigned char				badcreate[];
extern unsigned char				nofork[];
extern unsigned char				noswap[];
extern unsigned char				piperr[];
extern unsigned char				badopen[];
extern unsigned char				badnum[];
extern unsigned char				arglist[];
extern unsigned char				txtbsy[];
extern unsigned char				toobig[];
extern unsigned char				badexec[];
extern unsigned char				badfile[];
extern unsigned char				badreturn[];
extern unsigned char				badexport[];
extern unsigned char				badunset[];
extern unsigned char				nohome[];
extern unsigned char				badperm[];
extern unsigned char				mssgargn[];
extern unsigned char				libacc[];
extern unsigned char				libbad[];
extern unsigned char				libscn[];
extern unsigned char				libmax[];
extern unsigned char				badumask[];	/* L000 */
extern unsigned char                             emultihop[];
extern unsigned char                             nulldir[];
extern unsigned char                             enotdir[];
extern unsigned char                             enoent[];
extern unsigned char                             eacces[];
extern unsigned char                             enolink[];

/*	'builtin'nerror messages	*/

extern unsigned char				btest[];
extern unsigned char				badop[];

/*	fork constant	*/

#define 	FORKLIM 	32

extern address			end[];

#include	"ctype.h"

extern int				wasintr;	/* used to tell if break or delete is hit
				   					 *  while executing a wait
									 */
extern int				eflag;


/*
 * Find out if it is time to go away.
 * `trapnote' is set to SIGSET when fault is seen and
 * no trap has been set.
 */

#define		sigchk()	if (trapnote & SIGSET)	\
							exitsh(exitval ? exitval : SIGFAIL)

#define 	exitset()	retval = exitval
