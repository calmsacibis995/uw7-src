/*		copyright	"%c%" 	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)csh:common/cmd/csh/sh.init.c	1.4.5.5"
#ident  "$Header$"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

#include "sh.h"
#include "sh.tconst.h"

/*
 * C shell
 */

extern	int doalias();
extern	int dobg();
extern	int dobreak();
extern	int dochngd();
extern	int docontin();
extern	int dodirs();
extern	int doecho();
extern	int doelse();
extern	int doend();
extern	int doendif();
extern	int doendsw();
extern	int doeval();
extern	int doexit();
extern	int dofg();
extern	int doforeach();
extern	int doglob();
extern	int dogoto();
extern	int dohash();
extern	int dohist();
extern	int doif();
extern	int dojobs();
extern	int dokill();
extern	int dolet();
extern	int dolimit();
extern	int dologin();
extern	int dologout();
#ifdef NEWGRP
extern	int donewgrp();
#endif
extern	int donice();
extern	int donotify();
extern	int donohup();
extern	int doonintr();
extern	int dopopd();
extern	int dopushd();
extern	int dorepeat();
extern	int doset();
extern	int dosetenv();
extern	int dosource();
extern	int dostop();
extern	int dosuspend();
extern	int doswbrk();
extern	int doswitch();
extern	int dotime();
extern	int dounlimit();
extern	int doumask();
extern	int dowait();
extern	int dowhile();
extern	int dozip();
extern	int execash();
extern	int goodbye();
extern	int hashstat();
extern	int shift();
#ifdef OLDMALLOC
extern	int showall();
#endif
extern	int unalias();
extern	int dounhash();
extern	int unset();
extern	int dounsetenv();

#define	INF	1000

struct	biltins bfunc[] = {
	S_AT,		dolet,		0,	INF,
	S_alias,	doalias,	0,	INF,
#ifdef OLDMALLOC
	S_alloc,	showall,	0,	1,
#endif
	S_bg,		dobg,		0,	INF,
	S_break,	dobreak,	0,	0,
	S_breaksw,	doswbrk,	0,	0,
#ifdef IIASA
	S_bye,		goodbye,	0,	0,
#endif
	S_case,	dozip,		0,	1,
	S_cd,		dochngd,	0,	1,
	S_chdir,	dochngd,	0,	1,
	S_continue,	docontin,	0,	0,
	S_default,	dozip,		0,	0,
	S_dirs,	dodirs,		0,	1,
	S_echo,	doecho,		0,	INF,
	S_else,	doelse,		0,	INF,
	S_end,		doend,		0,	0,
	S_endif,	dozip,		0,	0,
	S_endsw,	dozip,		0,	0,
	S_eval,	doeval,		0,	INF,
	S_exec,	execash,	1,	INF,
	S_exit,	doexit,		0,	INF,
	S_fg,		dofg,		0,	INF,
	S_foreach,	doforeach,	3,	INF,
#ifdef IIASA
	S_gd,		dopushd,	0,	1,
#endif
	S_glob,	doglob,		0,	INF,
	S_goto,	dogoto,		1,	1,
	S_hashstat,	hashstat,	0,	0,
	S_history,	dohist,		0,	2,
	S_if,		doif,		1,	INF,
	S_jobs,	dojobs,		0,	1,
	S_kill,	dokill,		1,	INF,
	S_limit,	dolimit,	0,	3,
	S_login,	dologin,	0,	1,
	S_logout,	dologout,	0,	0,
#ifdef NEWGRP
	S_newgrp,	donewgrp,	1,	1,
#endif
	S_nice,	donice,		0,	INF,
	S_nohup,	donohup,	0,	INF,
	S_notify,	donotify,	0,	INF,
	S_onintr,	doonintr,	0,	2,
	S_popd,	dopopd,		0,	1,
	S_pushd,	dopushd,	0,	1,
#ifdef IIASA
	S_rd,		dopopd,		0,	1,
#endif
	S_rehash,	dohash,		0,	0,
	S_repeat,	dorepeat,	2,	INF,
	S_set,		doset,		0,	INF,
	S_setenv,	dosetenv,	0,	2,
	S_shift,	shift,		0,	1,
	S_source,	dosource,	1,	2,
	S_stop,	dostop,		1,	INF,
	S_suspend,	dosuspend,	0,	0,
	S_switch,	doswitch,	1,	INF,
	S_time,		dotime,		0,	INF,
	S_umask,	doumask,	0,	1,
	S_unalias,	unalias,	1,	INF,
	S_unhash,	dounhash,	0,	0,
	S_unlimit,	dounlimit,	0,	INF,
	S_unset,	unset,		1,	INF,
	S_unsetenv,	dounsetenv,	1,	INF,
	S_wait,		dowait,		0,	0,
	S_while,	dowhile,	1,	INF,
};
int nbfunc = sizeof bfunc / sizeof *bfunc;

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

struct srch srchn[] = {
	S_AT,		ZLET,
	S_break,	ZBREAK,
	S_breaksw,	ZBRKSW,
	S_case,		ZCASE,
	S_default, 	ZDEFAULT,
	S_else,		ZELSE,
	S_end,		ZEND,
	S_endif,	ZENDIF,
	S_endsw,	ZENDSW,
	S_exit,		ZEXIT,
	S_foreach, 	ZFOREACH,
	S_goto,		ZGOTO,
	S_if,		ZIF,
	S_label,	ZLABEL,
	S_set,		ZSET,
	S_switch,	ZSWITCH,
	S_while,	ZWHILE
};
int nsrchn = sizeof srchn / sizeof *srchn;

struct	mesg mesg[] = {
	0,	0,
	S_HUP,	"Hangup",			/* 1 */
	S_INT,	"Interrupt",			/* 2 */	
	S_QUIT,	"Quit",				/* 3 */
	S_ILL,	"Illegal Instruction",		/* 4 */
	S_TRAP,	"Trace/Breakpoint Trap",	/* 5 */
	S_ABRT,	"Abort",			/* 6 */
	S_EMT,	"Emulation Trap",		/* 7 */
	S_FPE,	"Arithmetic Exception",		/* 8 */
	S_KILL,	"Killed",			/* 9 */
	S_BUS,	"Bus Error",			/* 10 */
	S_SEGV,	"Segmentation Fault",		/* 11 */
	S_SYS,	"Bad System Call",		/* 12 */
	S_PIPE,	"Broken Pipe",			/* 13 */
	S_ALRM,	"Alarm Clock",			/* 14 */
	S_TERM,	"Terminated",			/* 15 */
	S_USR1,	"User Signal 1",		/* 16 */
	S_USR2,	"User Signal 2",		/* 17 */
	S_CHLD,	"Child Status Changed",		/* 18 */
	S_LOST,	"Power-Fail/Restart",		/* 19  Not used in SysV */
	S_WINCH,"Window Size Change",		/* 20 */
	S_URG,	"Urgent Socket Condition",	/* 21 */
	S_IO,	"Pollable Event",		/* 22 */
	S_STOP,	"Stopped (signal)",		/* 23 */
	S_TSTP,	"Stopped (user)",		/* 24 */
	S_CONT,	"Continued",			/* 25 */
	S_TTIN, "Stopped (tty input)",		/* 26 */
	S_TTOU, "Stopped (tty output)",		/* 27 */
	S_VTALRM,"Virtual Timer Expired",	/* 28 */
	S_PROF,	"Profiling Timer Expired",	/* 29 */
	S_XCPU,	"Cpu Limit Exceeded",		/* 30 */
	S_XFSZ, "File Size Limit Exceeded",	/* 31 */
	0,	"Signal 32"		
};
