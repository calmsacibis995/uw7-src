#ident	"@(#)ksh93:src/cmd/ksh93/include/shell.h	1.2"
#pragma prototyped
#ifndef SH_INTERACTIVE
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * Interface definitions for shell command language
 *
 */

#include	<cmd.h>
#include	<hash.h>
#include	<history.h>
#ifdef _SH_PRIVATE
#   include	"name.h"
#else
#   include	<nval.h>
#endif /* _SH_PRIVATE */

#define SH_VERSION	960106

#undef NOT_USED
#define NOT_USED(x)	(&x,1)

/* options */
typedef unsigned long	Shopt_t;
#define sh_isoption(x)	(sh.options & (x))
#define sh_onoption(x)	(sh.options |= (x))
#define sh_offoption(x)	(sh.options &= ~(x))

#define SH_CFLAG	(1<<0)
#define SH_HISTORY	(1<<1) /* used also as a state */
#define	SH_ERREXIT	(1<<2) /* used also as a state */
#define	SH_VERBOSE	(1<<3) /* used also as a state */
#define SH_MONITOR	(1<<4)/* used also as a state */
#define	SH_INTERACTIVE	(1<<5) /* used also as a state */
#define	SH_RESTRICTED	(1L<<6)
#define	SH_XTRACE	(1L<<7)
#define	SH_KEYWORD	(1L<<8)
#define SH_NOUNSET	(1L<<9)
#define SH_NOGLOB	(1L<<10)
#define SH_ALLEXPORT	(1L<<11)
#define SH_IGNOREEOF	(1L<<13)
#define SH_NOCLOBBER	(1L<<14)
#define SH_MARKDIRS	(1L<<15)
#define SH_BGNICE	(1L<<16)
#define SH_VI		(1L<<17)
#define SH_VIRAW	(1L<<18)
#define	SH_TFLAG	(1L<<19)
#define SH_TRACKALL	(1L<<20)
#define	SH_SFLAG	(1L<<21)
#define	SH_NOEXEC	(1L<<22)
#define SH_GMACS	(1L<<24)
#define SH_EMACS	(1L<<25)
#define SH_PRIVILEGED	(1L<<26)
#define SH_NOLOG	(1L<<28)
#define SH_NOTIFY	(1L<<29)
#define SH_DICTIONARY	(1L<<30)

/* The following type is used for error messages */

/* error messages */
extern const char	e_defpath[];
extern const char	e_found[];
extern const char	e_found_id[];
extern const char	e_found2[];
extern const char	e_found2_id[];
extern const char	e_nospace[];
extern const char	e_nospace_id[];
extern const char	e_format[];
extern const char	e_format_id[];
extern const char 	e_number[];
extern const char 	e_number_id[];
extern const char	e_restricted[];
extern const char	e_restricted_id[];
extern const char	e_version[];
extern const char	e_recursive[];
extern const char	e_recursive_id[];

typedef struct sh_scope
{
	struct sh_scope	*par_scope;
	int		argc;
	char		**argv;
	char		*cmdname;
	Hashtab_t	*var_tree;
} Shscope_t;

/*
 * Saves the state of the shell
 */

typedef struct sh_static
{
	int		inlineno;	/* line number of current input file */
	Shopt_t		options;	/* set -o options */
	Hashtab_t	*var_tree;	/* for shell variables */
	Hashtab_t	*fun_tree;	/* for shell functions */
	Hashtab_t	*alias_tree;	/* for alias names */
	Hashtab_t	*bltin_tree;	/* for builtin commands */
	int		exitval;	/* most recent exit value */
	Sfio_t		**sftable;	/* pointer to stream pointer table */
	unsigned char	*fdstatus;	/* pointer to file status table */
	const char	*pwd;		/* present working directory */
	History_t	*hist_ptr;	/* history file pointer */
        unsigned int	trapnote;	/* set when trap/signal is pending */
        char		subshell;	/* set for virtual subshell */
        char		universe;
        int		(*waitevent)(int,long);
        Shscope_t	*topscope;	/* pointer to top-level scope */
#ifdef _SH_PRIVATE
	_SH_PRIVATE
#endif /* _SH_PRIVATE */
} Shell_t;

/* flags for sh_parse */
#define SH_NL		1	/* Treat new-lines as ; */
#define SH_EOF		2	/* EOF causes syntax error */

extern void		sh_subfork(void);
extern int		sh_init(int,char*[]);
extern int 		sh_eval(Sfio_t*,int);
extern void 		sh_delay(double);
extern void		*sh_parse(Sfio_t*,int);
extern int 		sh_trap(const char*,int);
extern int 		sh_fun(Namval_t*,Namval_t*);
extern int		sh_funscope(int,char*[],int(*)(void*),void*,int);
extern int		sh_main(int, char*[], void(*)(int));
extern void		sh_menu(Sfio_t*, int, char*[]);
extern int		sh_addbuiltin(const char*, int(*)(int, char*[],void*), void*);
extern char		*sh_fmtq(const char*);
extern double		sh_strnum(const char*, char**, int);
extern int		sh_access(const char*,int);
extern int 		sh_close(int);
extern int		sh_dup(int);
extern void 		sh_exit(int);
extern int		sh_open(const char*, int, ...);
extern int		sh_openmax(void);
extern ssize_t 		sh_read(int, void*, size_t);
extern ssize_t 		sh_write(int, const void*, size_t);
extern off_t		sh_seek(int, off_t, int);
extern int 		sh_pipe(int[]);
extern void		*sh_waitnotify(int(*)(int,long));
extern Shscope_t	*sh_getscope(int,int);
extern Shscope_t	*sh_setscope(Shscope_t*);
#ifdef SHOPT_DYNAMIC
    extern void		**sh_getliblist(void);
#endif /* SHOPT_DYNAMIC */

#ifndef _SH_PRIVATE
#   define access(a,b)	sh_access(a,b)
#   define close(a)	sh_close(a)
#   define exit(a)	sh_exit(a)
#   define pipe(a)	sh_pipe(a)
#   define read(a,b,c)	sh_read(a,b,c)
#   define write(a,b,c)	sh_write(a,b,c)
#   define open		sh_open
#   define lseek	sh_seek
#   define dup		sh_dup
#endif /* !_SH_PRIVATE */

#define SH_SIGSET	4
#define SH_EXITSIG	0400	/* signal exit bit */
#define SH_EXITMASK	(SH_EXITSIG-1)	/* normal exit status bits */
#define SH_RUNPROG	-1022	/* needs to be negative and < 256 */


#define sh_bltin_table()	(sh.bltin_tree)
#define sh_sigcheck() do{if(sh.trapnote&SH_SIGSET)sh_exit(SH_EXITSIG);} while(0)

extern Shell_t sh;

#endif /* SH_INTERACTIVE */
