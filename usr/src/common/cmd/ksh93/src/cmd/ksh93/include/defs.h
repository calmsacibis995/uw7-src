#ident	"@(#)ksh93:src/cmd/ksh93/include/defs.h	1.2"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * Shell interface private definitions
 *
 */

/* get rid of mount prototype */
#define mount	_AST__mount

#include	<ast.h>
#include	<sfio.h>
#include	<error.h>
#include	"FEATURE/options"
#include	<hash.h>
#include	<history.h>
#include	"fault.h"
#include	"argnod.h"

/*
 * note that the first few fields have to be the same as for
 * Shscoped_t in <shell.h>
 */
struct sh_scoped
{
	struct sh_scoped *prevst;	/* pointer to previous state */
	int		dolc;
	char		**dolv;
	char		*cmdname;
	Hashtab_t	*save_tree;	/* var_tree for calling function */
	struct slnod	*staklist;	/* link list of function stacks */
	struct sh_scoped *self;		/* pointer to copy of this scope*/
	int		states;
	int		breakcnt;
	int		execbrk;
	int		loopcnt;
	int		firstline;
	long		optindex;
	long		optnum;
	long		tmout;		/* value for TMOUT */ 
	short		optchar;
	short		opterror;
	int		ioset;
	short		trapmax;
	char		*trap[SH_DEBUGTRAP+1];
	char		**trapcom;
	char		**otrapcom;
	void		*timetrap;
};

struct limits
{
	int		open_max;	/* maximum number of file descriptors */
	int		clk_tck;	/* number of ticks per second */
	int		child_max;	/* maxumum number of children */
	int		ngroups_max;	/* maximum number of process groups */
	unsigned char	posix_version;	/* posix version number */
	unsigned char	posix_jobcontrol;/* non-zero for job control systems */
	unsigned char	fs3d;		/* non-zero for 3-d file system */
};

#define _SH_PRIVATE \
	struct sh_scoped st;		/* scoped information */ \
	struct limits	lim;		/* run time limits */ \
	Sfio_t		*heredocs;	/* current here-doc file */ \
	int		**fdptrs;	/* pointer to file numbers */ \
	int		savexit; \
	char		*lastarg; \
	char		*lastpath;	/* last alsolute path found */ \
	Hashtab_t	*track_tree;	/* for tracked aliases*/ \
	Namval_t	*bltin_nodes;	/* pointer to built-in variables */ \
	Hashtab_t	*var_base;	/* global level variables */ \
	Sfio_t		*outpool;	/* ouput stream pool */ \
	long		timeout;	/* read timeout */ \
	int		curenv;		/* current subshell number */ \
	int		jobenv;		/* subshell number for jobs */ \
	int		nextprompt;	/* next prompt is PS<nextprompt> */ \
	Namval_t	*bltin_cmds;	/* pointer to built-in commands */ \
	Namval_t	*posix_fun;	/* points to last name() function */ \
	int		infd;		/* input file descriptor */ \
	char		*outbuff;	/* pointer to output buffer */ \
	char		*errbuff;	/* pointer to stderr buffer */ \
	char		*prompt;	/* pointer to prompt string */ \
	char		*shname;	/* shell name */ \
	char		*comdiv;	/* points to sh -c argument */ \
	char		*prefix;	/* prefix for compound assignment */ \
	sigjmp_buf	*jmplist;	/* longjmp return stack */ \
	char		**sigmsg;	/* points to signal messages */ \
	int		oldexit; \
	uid_t 		userid,euserid;	/* real and effective user id */ \
	gid_t 		groupid,egroupid;/* real and effective group id */ \
	pid_t		pid;		/* process id of shell */ \
	pid_t		bckpid;		/* background process id */ \
	pid_t		cpid; \
	long		ppid;		/* parent process id of shell */ \
	int		topfd; \
	int		sigmax;		/* maximum number of signals */ \
	unsigned char	*sigflag;	/* pointer to signal states */ \
	char		intrap; \
	char		login_sh; \
	char		lastbase; \
	char		forked;	\
	char		binscript; \
	char		used_pos;	/* used postional parameter */\
	unsigned char	lastsig;	/* last signal received */ \
	char		*readscript;	/* set before reading a script */ \
	int		*inpipe;	/* input pipe pointer */ \
	int		*outpipe;	/* output pipe pointer */ \
	int		cpipe[2]; \
	int		coutpipe; \
	struct argnod	*envlist; \
	struct dolnod	*arglist; \
	int		fn_depth; \
	int		dot_depth; \
	long		nforks; \
	char		ifstable[256];

#include	<shell.h>


/* error exits from various parts of shell */
#define	NIL(type)	((type)0)

#define new_of(type,x)	((type*)malloc((unsigned)sizeof(type)+(x)))

#define exitset()	(sh.savexit=sh.exitval)


/* states */
/* low numbered states are same as options */
#define SH_NOFORK	0x1	/* set when fork not necessary, not a state */
#define SH_COMPLETE	0x1	/* set for command completion */
#define SH_TTYWAIT	0x40	/* waiting for keyboard input */ 
#define	SH_FORKED	0x80	/* set when process has been forked */
#define	SH_PROFILE	0x100	/* set when processing profiles */
#define SH_NOALIAS	0x200	/* do not expand non-exported aliases */
#define SH_NOTRACK	0x400	/* set to disable sftrack() function */
#define SH_STOPOK	0x800	/* set for stopable builtins */
#define SH_GRACE	0x1000	/* set for timeout grace period */
#define SH_TIMING	0x2000	/* set while timing pipelines */
#define SH_DEFPATH	0x4000	/* set when using default path */
#define SH_INIT		0x8000	/* set when initializing the shell */

extern Namval_t		*sh_assignok(Namval_t*,int);
extern int 		sh_echolist(Sfio_t*, int, char**);
extern struct argnod	*sh_endword(int);
extern char 		**sh_envgen(void);
extern void 		sh_envnolocal(Namval_t*);
extern char		*sh_etos(double,int);
extern char		*sh_ftos(double,int);
extern double		sh_arith(const char*);
extern pid_t 		sh_fork(int,int*);
extern char 		*sh_mactrim(char*,int);
extern int 		sh_macexpand(struct argnod*,struct argnod**);
extern void 		sh_machere(Sfio_t*, Sfio_t*, char*);
extern char 		*sh_mactry(char*);
extern int 		sh_readline(char**,int,int,long);
extern void		sh_reinit(char*[]);
extern Sfio_t		*sh_sfeval(char*[]);
extern Hashtab_t	*sh_subfuntree(void);
extern void		sh_subtmpfile(void);
extern char 		*sh_substitute(const char*,const char*,char*);
extern int		sh_trace(char*[],int);
extern void		sh_trim(char*);
extern void		sh_userinit(void);
extern int 		sh_whence(char**,int);

#define	sh_isstate(x)	(sh.st.states&((int)(x)))
#define	sh_onstate(x)	(sh.st.states |= ((int)(x)))
#define	sh_setstate(x)	(sh.st.states = ((int)(x)))
#define	sh_offstate(x)	(sh.st.states &= ~((int)(x)))

extern time_t		sh_mailchk;
