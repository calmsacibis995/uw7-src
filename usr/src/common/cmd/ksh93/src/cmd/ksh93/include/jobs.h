#ident	"@(#)ksh93:src/cmd/ksh93/include/jobs.h	1.1"
#pragma prototyped
#ifndef JOB_NFLAG
/*
 *	Interface to job control for shell
 *	written by David Korn
 *
 */

#define JOBTTY	2

#include	<ast.h>
#include	<sfio.h>
#ifndef SIGINT
#   include	<signal.h>
#endif /* !SIGINT */
#include	"FEATURE/options"

#undef JOBS
#if defined(SIGCLD) && !defined(SIGCHLD)
#   define SIGCHLD	SIGCLD
#endif
#ifdef SIGCHLD
#   define JOBS	1
#   include	"terminal.h"
#   ifdef FIOLOOKLD
	/* Ninth edition */
	extern int tty_ld, ntty_ld;
#	define OTTYDISC	tty_ld
#	define NTTYDISC	ntty_ld
#   endif	/* FIOLOOKLD */
#else
#   undef SIGTSTP
#   undef SH_MONITOR
#   define SH_MONITOR	0
#   define job_set(x)
#   define job_reset(x)
#endif

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
	int		p_env;		/* subshell environment number */
#ifdef JOBS
	off_t		p_name;		/* history file offset for command */
	struct termios	p_stty;		/* terminal state for job */
#endif /* JOBS */
};

struct jobs
{
	struct process	*pwlist;	/* head of process list */
	pid_t		curpgid;	/* current process gid id */
	pid_t		parent;		/* set by fork() */
	pid_t		mypid;		/* process id of shell */
	pid_t		mypgid;		/* process group id of shell */
	pid_t		mytgid;		/* terminal group id of shell */
	int		numpost;	/* number of posted jobs */
	short		fd;		/* tty descriptor number */
#ifdef JOBS
	int		suspend;	/* suspend character */
	int		linedisc;	/* line dicipline */
#endif /* JOBS */
	char		in_critical;	/* set when in critical region */
	char		jobcontrol;	/* turned on for real job control */
	char		waitsafe;	/* wait will not block */
	char		waitall;	/* wait for all jobs in pipe */
	char		toclear;	/* job table needs clearing */
	unsigned char	*freejobs;	/* free jobs numbers */
};

/* flags for joblist */
#define JOB_LFLAG	1
#define JOB_NFLAG	2
#define JOB_PFLAG	4
#define JOB_NLFLAG	8

extern struct jobs job;

#ifdef JOBS
extern const char	e_jobusage[];
extern const char	e_jobusage_id[];
extern const char	e_kill[];
extern const char	e_kill_id[];
extern const char	e_done[];
extern const char	e_done_id[];
extern const char	e_running[];
extern const char	e_running_id[];
extern const char	e_coredump[];
extern const char	e_coredump_id[];
extern const char	e_killcolon[];
extern const char	e_no_proc[];
extern const char	e_no_proc_id[];
extern const char	e_no_job[];
extern const char	e_no_job_id[];
extern const char	e_jobsrunning[];
extern const char	e_jobsrunning_id[];
extern const char	e_nlspace[];
extern const char	e_access[];
extern const char	e_access_id[];
extern const char	e_terminate[];
extern const char	e_terminate_id[];
extern const char	e_no_jctl[];
extern const char	e_no_jctl_id[];
#ifdef SIGTSTP
   extern const char	e_no_start[];
   extern const char	e_no_start_id[];
#endif /* SIGTSTP */
#ifdef NTTYDISC
   extern const char	e_newtty[];
   extern const char	e_newtty_id[];
   extern const char	e_oldtty[];
   extern const char	e_oldtty_id[];
#endif /* NTTYDISC */
#endif	/* JOBS */

/*
 * The following are defined in jobs.c
 */

extern void	job_clear(void);
extern void	job_bwait(char**);
extern int	job_walk(Sfio_t*,int(*)(struct process*,int),int,char*[]);
extern int	job_kill(struct process*,int);
extern void	job_wait(pid_t);
extern int	job_post(pid_t,pid_t);
extern int	job_chksave(pid_t);
#ifdef JOBS
	extern void	job_init(int);
	extern int	job_close(void);
	extern int	job_list(struct process*,int);
	extern int	job_terminate(struct process*,int);
	extern int	job_switch(struct process*,int);
#else
#	define job_init(flag)
#	define job_close()	(0)
#endif	/* JOBS */


#endif /* !JOB_NFLAG */
