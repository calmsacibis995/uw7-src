#ident	"@(#)OSRcmds:csh/sh.init.c	1.1"
#pragma comment(exestr, "@(#) sh.init.c 25.3 95/03/09 ")
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

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/* #ident	"@(#)csh:sh.init.c	1.2" */

/*
 *	@(#) sh.init.c 1.1 88/03/29 csh:sh.init.c
 */
/*	@(#)sh.init.c	2.1	SCCS id keyword	*/
/* Copyright (c) 1980 Regents of the University of California */

/*
 * Modification History
 *	L000	scol!markhe	16th March, 1992
 *	- let umask take upto 2 arguments
 *	L001	scol!gregw	3rd August 1994
 *	- added code for unsetenv. Code from BSD-4.4.
 *	S002	sco!vinces	15th Feb 1995
 *	- added support to cd and pwd for logical path processing of symbolic
 *	  links. Brought pwd into the list of built in functions for this shell
 *	  (in conjunction with changes made and noted in sh.func.c).
 */

#include "sh.local.h"
/*
 * C shell
 */

extern	int await();
extern	int chngd();
extern	int doalias();
extern	int dobreak();
extern	int docontin();
extern	int doecho();
extern	int doelse();
extern	int doend();
extern	int doendif();
extern	int doendsw();
extern	int doexit();
extern	int doforeach();
extern	int doglob();
extern	int dogoto();
extern	int dohash();
extern	int dohist();
extern	int doif();
extern	int dolet();
#ifdef	LOGIN
extern	int dologin();
#endif
extern	int dologout();
extern	int donewgrp();
extern	int donice();
extern	int donohup();
extern	int doonintr();
extern	int dorepeat();
extern	int doset();
#ifndef V6
extern	int dosetenv();
#endif
extern	int dounsetenv();
extern	int dosource();
extern	int doswbrk();
extern	int doswitch();
extern	int dotime();
#ifndef V6
extern	int doumask();
#endif
extern	int dowhile();
extern	int dozip();
extern	int execash();
#ifdef VFORK
extern	int hashstat();
#endif
extern	int goodbye();
extern	int shift();
extern	int showall();
extern	int unalias();
extern	int dounhash();
extern	int unset();
extern	int dopwd();						/* S002 */

#define INF	1000

struct	biltins {
	char	*bname;
	/* REPLACED WITH BELOW int	(*bfunct)(); */
	int	(*bfunct)();
	short	minargs, maxargs;
} bfunc[] = {
	"@",		dolet,		0,	INF,
	"alias",	doalias,	0,	INF,
#ifdef debug
	"alloc",	showall,	0,	1,
#endif
	"break",	dobreak,	0,	0,
	"breaksw",	doswbrk,	0,	0,
	"case",		dozip,		0,	1,
	"cd",		chngd,		0,	2,		/* S002 */
	"chdir",	chngd,		0,	2,		/* S002 */
	"continue",	docontin,	0,	0,
	"default",	dozip,		0,	0,
	"echo",		doecho,		0,	INF,
	"else",		doelse,		0,	INF,
	"end",		doend,		0,	0,
	"endif",	dozip,		0,	0,
	"endsw",	dozip,		0,	0,
	"exec",		execash,	1,	INF,
	"exit",		doexit,		0,	INF,
	"foreach",	doforeach,	3,	INF,
	"glob",		doglob,		0,	INF,
	"goto",		dogoto,		1,	1,
#ifdef VFORK
	"hashstat",	hashstat,	0,	0,
#endif
	"history",	dohist,		0,	0,
	"if",		doif,		1,	INF,
#ifdef	LOGIN
	"login",	dologin,	0,	1,
#endif
	"logout",	dologout,	0,	0,
	"newgrp",	donewgrp,	0,	1,		/* M002 */
	"nice",		donice,		0,	INF,
	"nohup",	donohup,	0,	INF,
	"onintr",	doonintr,	0,	2,
	"pwd",		dopwd,		0,	1,		/* S002 */
	"rehash",	dohash,		0,	0,
	"repeat",	dorepeat,	2,	INF,
	"set",		doset,		0,	INF,
#ifndef V6
	"setenv",	dosetenv,	2,	2,
#endif
	"shift",	shift,		0,	1,
	"source",	dosource,	1,	1,
	"switch",	doswitch,	1,	INF,
	"time",		dotime,		0,	INF,
#ifndef V6
	"umask",	doumask,	0,	2,		/* L000 */
#endif
	"unalias",	unalias,	1,	INF,
	"unhash",	dounhash,		0,	0,
	"unset",	unset,		1,	INF,
	"unsetenv",	dounsetenv,	1,	INF,		/* L001 */
	"wait",		await,		0,	0,
	"while",	dowhile,	1,	INF,
	0,		0,		0,	0,
};

#define	ZBREAK		0
#define	ZBRKSW		1
#define	ZCASE		2
#define	ZDEFAULT 	3
#define	ZELSE		4
#define	ZEND		5
#define	ZENDIF		6
#define	ZENDSW		7
#define	ZEXIT		8
#define	ZFOREACH	9
#define	ZGOTO		10
#define	ZIF		11
#define	ZLABEL		12
#define	ZLET		13
#define	ZSET		14
#define	ZSWITCH		15
#define	ZTEST		16
#define	ZTHEN		17
#define	ZWHILE		18

struct srch {
	char	*s_name;
	short	s_value;
} srchn[] = {
	"@",		ZLET,
	"break",	ZBREAK,
	"breaksw",	ZBRKSW,
	"case",		ZCASE,
	"default", 	ZDEFAULT,
	"else",		ZELSE,
	"end",		ZEND,
	"endif",	ZENDIF,
	"endsw",	ZENDSW,
	"exit",		ZEXIT,
	"foreach", 	ZFOREACH,
	"goto",		ZGOTO,
	"if",		ZIF,
	"label",	ZLABEL,
	"set",		ZSET,
	"switch",	ZSWITCH,
	"while",	ZWHILE,
	0,		0,
};

char	*mesg[] = {
	0,
	"Hangup",
	0,
	"Quit",
	"Illegal instruction",
	"Trace/BPT trap",
	"IOT trap",
	"EMT trap",
	"Floating exception",
	"Killed",
	"Bus error",
	"Segmentation violation",
	"Bad system call",
	0,
	"Alarm clock",
	"Terminated",
};
