#ident	"@(#)OSRcmds:ksh/include/jobs.h	1.1"
#pragma comment(exestr, "@(#) jobs.h 25.3 93/01/20 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1993.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*									*/
/*		   Copyright (C) AT&T, 1984-1992.			*/
/*			All Rights Reserved				*/
/*									*/
/*	  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.		*/
/*	    The copyright notice above does not evidence any		*/
/*	   actual or intended publication of such source code.		*/
/*									*/

/*
 * Modification History
 *
 *	L000	scol!markhe	11 Nov 92
 *	- changed type of message variables from 'const char' to MSG
 */

/*
 *	UNIX shell
 *	S. R. Bourne
 *	rewritten by David Korn
 *
 */

#define LOBYTE	0377
#define MAXMSG	25
#define JOBTTY	2

#include	"sh_config.h"
#ifdef JOBS
#   include	"terminal.h"
#   undef ESCAPE

#   ifdef FIOLOOKLD
	/* Ninth edition */
	extern int tty_ld, ntty_ld;
#	define OTTYDISC	tty_ld
#	define NTTYDISC	ntty_ld
#	define killpg(pgrp,sig)		kill(-(pgrp),sig)
#   endif	/* FIOLOOKLD */
#   define job_nonstop()	(st.states &= ~MONITOR)
#else
#   undef SIGTSTP
#   undef SIGCHLD
#   undef SIGCLD
#   undef MONITOR
#   define MONITOR	0
#   define job_nonstop()
#   define job_set(x)
#   define job_reset(x)
#endif	/* JOBS */

#if defined(CHILD_MAX) && (CHILD_MAX>256)
#   define MAXJ	(CHILD_MAX-1)
#else
#   define MAXJ	256
#endif /* CHILD_MAX */

/* JBYTES is the number of char's needed for MAXJ bits */
#define JBYTES		(1+((MAXJ-1)/(8)))

struct process
{
	struct process *p_nxtjob;	/* next job structure */
	struct process *p_nxtproc;	/* next process in current job */
	pid_t		p_pid;		/* process id */
	pid_t		p_pgrp;		/* process group */
	pid_t		p_fgrp;		/* process group when stopped */
	short		p_job;		/* job number of process */
	unsigned short	p_exit;		/* exit value or signal number */
	unsigned char	p_flag;		/* flags - see below */
#ifdef JOBS
	off_t		p_name;		/* history file offset for command */
	struct termios	p_stty;		/* terminal state for job */
#endif /* JOBS */
};

/* Process states */

#define P_RUNNING	01
#define P_STOPPED	02
#define P_NOTIFY	04
#define P_SIGNALLED	010
#define P_STTY		020
#define P_DONE		040
#define P_COREDUMP	0100
#define P_BYNUM		0200

struct jobs
{
	struct process	*pwlist;	/* head of process list */
	pid_t		curpgid;	/* current process gid id */
	pid_t		parent;		/* set by fork() */
	pid_t		mypid;		/* process id of shell */
	pid_t		mypgid;		/* process group id of shell */
	pid_t		mytgid;		/* terminal group id of shell */
	int		numpost;	/* number of posted jobs */
#ifdef JOBS
	int		suspend;	/* suspend character */
	int		linedisc;	/* line dicipline */
	char		jobcontrol;	/* turned on for real job control */
#endif /* JOBS */
	char		pipeflag;	/* set for pipelines */
	char		waitall;	/* wait for all pids of pipeline */
	char		waitsafe;	/* wait will not block */
	unsigned char	freejobs[JBYTES];	/* free jobs numbers */
};

extern struct jobs job;

#ifdef JOBS
extern MSG	e_jobusage;				/* L000 begin */
extern MSG	e_kill;
extern MSG	e_Done;
extern MSG	e_Running;
extern MSG	e_coredump;
extern MSG	e_killcolon;
extern MSG	e_no_proc;
extern MSG	e_no_job;
extern MSG	e_running;
extern MSG	e_nlspace;
extern MSG	e_ambiguous;
#ifdef SIGTSTP
   extern MSG	e_no_start;
   extern MSG	e_terminate;
   extern MSG	e_no_jctl;
#endif /* SIGTSTP */
#ifdef NTTYDISC
   extern MSG	e_newtty;
   extern MSG	e_oldtty;				/* L000 end */
#endif /* NTTYDISC */
#endif	/* JOBS */

/*
 * The following are defined in jobs.c
 */

#ifdef PROTO
    extern void job_clear(void);
    extern void job_bwait(char*[]);
    extern int	job_walk(int(*)(),int,char*[]);
    extern int	job_kill(struct process*,int);
#else
    extern void job_clear();
    extern int	job_post();
    extern void job_bwait();
    extern int	job_walk();
    extern int	job_kill();
#endif /* PROTO */
extern void	job_wait();
extern int	job_post();

#ifdef JOBS
#   ifdef PROTO
	extern void	job_init(int);
	extern int	job_close(void);
	extern int	job_list(struct process*,int);
#   else
	extern void	job_init();
	extern int	job_close();
	extern int	job_list();
#   endif /* PROTO */
#else
#	define job_init(flag)
#	define job_close()	(0)
#endif	/* JOBS */

#ifdef SIGTSTP
#   ifdef PROTO
	extern int	job_switch(struct process*,int);
#   else
	extern int	job_switch();
#   endif /* PROTO */
#endif /* SIGTSTP */
