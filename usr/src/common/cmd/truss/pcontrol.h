/*		copyright	"%c%" 	*/

#ident	"@(#)truss:common/cmd/truss/pcontrol.h	1.2.10.2"
#ident  "$Header$"

/* include file for process management */

/* requires:
 *	<sys/types.h>
 *	<sys/fault.h>
 *	<sys/syscall.h>
 *	<signal.h>
 *	<stdio.h>
 */
#include <sys/procfs.h>

#define	CONST	const
#define	VOID	void

#define	TRUE	1
#define	FALSE	0

/* definition of the process (program) table */
typedef struct {
	char	cntrl;		/* if TRUE then controlled process */
	char	child;		/* TRUE :: process created by fork() */
	char	state;		/* state of the process, see flags below */
	char	sig;		/* if dead, signal which caused it */
	char	rc;		/* exit code if process terminated normally */
	int	asfd;		/* /proc/<upid>/as filedescriptor */
	int	ctlfd;		/* /proc/<upid>/ctl filedescriptor */
	int	statusfd;	/* /proc/<upid>/status filedescriptor */
	pid_t	upid;		/* UNIX process ID */
	pstatus_t why;		/* from /proc -- status values when stopped */
	long	sysaddr;	/* address of most recent syscall instruction */
	sigset_t sigmask;	/* signals which stop the process */
	fltset_t faultmask;	/* faults which stop the process */
	sysset_t sysentry;	/* system calls which stop process on entry */
	sysset_t sysexit;	/* system calls which stop process on exit */
	char	setsig;		/* set signal mask before continuing */
	char	sethold;	/* set signal hold mask before continuing */
	char	setfault;	/* set fault mask before continuing */
	char	setentry;	/* set sysentry mask before continuing */
	char	setexit;	/* set sysexit mask before continuing */
	char	setregs;	/* set registers before continuing */
} process_t;

extern const char * const regname[NGREG];	/* register name strings */

/* shorthand for register array */
#define	REG	why.pr_lwp.pr_context.uc_mcontext.gregs

/* state values */
#define	PS_NULL	0	/* no process in this table entry */
#define	PS_RUN	1	/* process running */
#define	PS_STEP	2	/* process running single stepped */
#define	PS_STOP	3	/* process stopped */
#define	PS_LOST	4	/* process lost to control (EBADF) */
#define	PS_DEAD	5	/* process terminated */

/* machine-specific stuff */

struct	argdes	{	/* argument descriptor for system call (Psyscall) */
	int	value;		/* value of argument given to system call */
	char *	object;		/* pointer to object in controlling process */
	char	type;		/* AT_BYVAL, AT_BYREF */
	char	inout;		/* AI_INPUT, AI_OUTPUT, AT_INOUT */
	short	len;		/* if AT_BYREF, length of object in bytes */
};

struct	sysret	{	/* return values from system call (Psyscall) */
	int	rerrno;		/* syscall error number */
	greg_t	r0;		/* %r0 from system call */
	greg_t	r1;		/* %r1 from system call */
};

/* values for type */
#define	AT_BYVAL	0
#define	AT_BYREF	1

/* values for inout */
#define	AI_INPUT	0
#define	AI_OUTPUT	1
#define	AI_INOUT	2

/* maximum number of syscall arguments */
#define	MAXARGS		8

/* maximum size in bytes of a BYREF argument */
#define	MAXARGL		(4*1024)


/* external data used by the package */
extern int debugflag;		/* for debugging */
extern char * procdir;		/* "/proc" */

/*
 * Function prototypes for routines in the process control package.
 */

extern	int	Pcreate(process_t *, char **);
extern	int	Pgrab(process_t *, pid_t, int);
extern	int	Preopen(process_t *, int);
extern	int	Prelease(process_t *);
extern	int	Pwait(process_t *);
extern	int	Pstop(process_t *);
extern	int	Pstatus(process_t *, int, unsigned);
extern	int	Pgetsysnum(process_t *);
extern	int	Psetsysnum(process_t *, int);
extern	int	Pgetregs(process_t *);
extern	int	Pgetareg(process_t *, int);
extern	int	Pputregs(process_t *);
extern	int	Pputareg(process_t *, int);
extern	int	Psetrun(process_t *, int, int);
extern	int	Pstart(process_t *, int);
extern	int	Pterm(process_t *);
extern	int	Pread(process_t *P, ulong_t addr, void *buf, size_t nbyte);
extern	int	Pwrite(process_t *P, ulong_t addr, const void *buf, size_t nbyte);
extern	int	Psignal(process_t *, int, int);
extern	int	Pfault(process_t *, int, int);
extern	int	Psysentry(process_t *, int, int);
extern	int	Psysexit(process_t *, int, int);
extern	struct sysret Psyscall(process_t *, int, int, struct argdes *);
extern	int	Pctl(process_t *P, ulong_t op, void *argp, size_t arglen);
extern	int	Pset(process_t *P, ulong_t flags);
extern	int	Preset(process_t *P, ulong_t flags);
extern	int	Pcred(int pid, prcred_t *prcredp);
extern	int	is_empty(const long *, unsigned);
extern	void	mysleep(unsigned int);


/*
 * Test for empty set.
 * is_empty() should not be called directly.
 */
#define	isemptyset(sp)	is_empty((long *)(sp), sizeof(*(sp))/sizeof(long))
