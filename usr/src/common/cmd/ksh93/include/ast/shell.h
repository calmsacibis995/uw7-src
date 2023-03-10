#ident	"@(#)ksh93:include/ast/shell.h	1.1"
/***************************************************************
*                                                              *
*                      AT&T - PROPRIETARY                      *
*                                                              *
*        THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF        *
*                    AT&T BELL LABORATORIES                    *
*         AND IS NOT TO BE DISCLOSED OR USED EXCEPT IN         *
*            ACCORDANCE WITH APPLICABLE AGREEMENTS             *
*                                                              *
*                Copyright (c) 1995 AT&T Corp.                 *
*              Unpublished & Not for Publication               *
*                     All Rights Reserved                      *
*                                                              *
*       The copyright notice above does not evidence any       *
*      actual or intended publication of such source code      *
*                                                              *
***************************************************************/

/* : : generated by proto : : */
                  
#ifndef SH_INTERACTIVE
#if !defined(__PROTO__)
#include <prototyped.h>
#endif

/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * Interface definitions for shell command language
 *
 */

#include	<cmd.h>
#include	<hash.h>
#ifdef _SH_PRIVATE
#   include	"name.h"
#else
#   include	<nval.h>
#endif /* _SH_PRIVATE */

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
#define SH_PHYSICAL	(1L<<27)
#define SH_NOLOG	(1L<<28)
#define SH_NOTIFY	(1L<<29)
#define SH_DICTIONARY	(1L<<30)

/* The following type is used for error messages */

/* error messages */
extern __MANGLE__ const char	e_defpath[];
extern __MANGLE__ const char	e_found[];
extern __MANGLE__ const char	e_found_id[];
extern __MANGLE__ const char	e_nospace[];
extern __MANGLE__ const char	e_nospace_id[];
extern __MANGLE__ const char	e_format[];
extern __MANGLE__ const char	e_format_id[];
extern __MANGLE__ const char 	e_number[];
extern __MANGLE__ const char 	e_number_id[];
extern __MANGLE__ const char	e_restricted[];
extern __MANGLE__ const char	e_restricted_id[];
extern __MANGLE__ const char	e_version[];
extern __MANGLE__ const char	e_recursive[];
extern __MANGLE__ const char	e_recursive_id[];

/*
 * Saves the state of the shell
 */

typedef struct sh_static
{
	int		inlineno;
	Sfio_t		*heredocs;	/* current here-doc file */ 
	Shopt_t		options;
	Hashtab_t	*var_tree;	/* for shell variables */
	Hashtab_t	*fun_tree;	/* for shell functions */
	Hashtab_t	*alias_tree;	/* for alias names */
	Namval_t	*bltin_nodes;	/* pointer to built-in variables */
	int		**fdptrs;	/* pointer to file numbers */
	Sfio_t		**sftable;	/* pointer to stream pointer table */
	unsigned char	*fdstatus;	/* pointer to file status table */
	int		subshell;	/* set for virtual subshell */
	unsigned int	trapnote;
	char		*lastpath;	/* last alsolute path found */
	int		exitval;
	char		*lastarg;
	const char	*pwd;		/* present working directory */
	int		savexit;
#ifdef _SH_PRIVATE
	_SH_PRIVATE
#endif /* _SH_PRIVATE */
} Shell_t;

/* flags for sh_parse */
#define SH_NL		1	/* Treat new-lines as ; */
#define SH_EOF		2	/* EOF causes syntax error */

extern __MANGLE__ void		sh_subfork __PROTO__((void));
extern __MANGLE__ int		sh_init __PROTO__((int,char*[]));
extern __MANGLE__ void 		sh_eval __PROTO__((Sfio_t*,int));
extern __MANGLE__ void 		sh_delay __PROTO__((double));
extern __MANGLE__ union anynode	*sh_parse __PROTO__((Sfio_t*,int));
extern __MANGLE__ int 		sh_trap __PROTO__((const char*,int));
extern __MANGLE__ int 		sh_fun __PROTO__((Namval_t*,Namval_t*));
extern __MANGLE__ void		sh_menu __PROTO__((Sfio_t*, int, char*[]));
extern __MANGLE__ int		sh_addbuiltin __PROTO__((const char*, int(*)(int, char*[])));
extern __MANGLE__ char		*sh_fmtq __PROTO__((const char*));
extern __MANGLE__ double		sh_strnum __PROTO__((const char*, char**));
extern __MANGLE__ int		sh_access __PROTO__((const char*,int));
extern __MANGLE__ int 		sh_close __PROTO__((int));
extern __MANGLE__ void 		sh_exit __PROTO__((int));
extern __MANGLE__ int		sh_open __PROTO__((const char*, int, ...));
extern __MANGLE__ ssize_t 		sh_read __PROTO__((int, __V_*, size_t));
extern __MANGLE__ ssize_t 		sh_write __PROTO__((int, const __V_*, size_t));
extern __MANGLE__ off_t		sh_seek __PROTO__((int, off_t, int));
extern __MANGLE__ int 		sh_pipe __PROTO__((int[]));
#ifdef SHOPT_DYNAMIC
    extern __MANGLE__ __V_		**sh_getliblist __PROTO__((void));
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


#define sh_sigcheck() do{if(sh.trapnote&SH_SIGSET)sh_exit(SH_EXITSIG);} while(0)

extern __MANGLE__ Shell_t sh;
extern __MANGLE__ int	errno;

#endif /* SH_INTERACTIVE */
