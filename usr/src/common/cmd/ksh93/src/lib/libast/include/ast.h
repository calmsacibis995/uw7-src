#ident	"@(#)ksh93:src/lib/libast/include/ast.h	1.1"
#pragma prototyped
/*
 * Advanced Software Technology Library
 * AT&T Bell Laboratories
 *
 * std + posix + ast
 */

#ifndef _AST_H
#define _AST_H

#ifndef _AST_STD_H
#include <ast_std.h>
#endif

#ifndef _SFIO_H
#include <sfio.h>
#endif

#ifndef	ast
#define ast	_ast_info
#endif

#ifndef PATH_MAX
#define PATH_MAX	1024
#endif

/*
 * workaround botched headers that assume <stdio.h>
 */

#ifndef FILE
#define FILE	Sfio_t
#endif

/*
 * exit() support -- this matches shell exit codes
 */

#define EXIT_BITS	8			/* # exit status bits	*/

#define EXIT_USAGE	2			/* usage exit code	*/
#define EXIT_NOTFOUND	((1<<(EXIT_BITS-1))-1)	/* command not found	*/
#define EXIT_NOEXEC	((1<<(EXIT_BITS-1))-2)	/* other exec error	*/

#define EXIT_CODE(x)	((x)&((1<<EXIT_BITS)-1))
#define EXIT_CORE(x)	(EXIT_CODE(x)|(1<<EXIT_BITS)|(1<<(EXIT_BITS-1)))
#define EXIT_TERM(x)	(EXIT_CODE(x)|(1<<EXIT_BITS))

/*
 * NOTE: for compatibility the following work for EXIT_BITS={7,8}
 */

#define EXITED_CORE(x)	(((x)&((1<<EXIT_BITS)|(1<<(EXIT_BITS-1))))==((1<<EXIT_BITS)|(1<<(EXIT_BITS-1)))||((x)&((1<<(EXIT_BITS-1))|(1<<(EXIT_BITS-2))))==((1<<(EXIT_BITS-1))|(1<<(EXIT_BITS-2))))
#define EXITED_TERM(x)	((x)&((1<<EXIT_BITS)|(1<<(EXIT_BITS-1))))

/*
 * pathcanon() flags
 */

#define PATH_PHYSICAL	01
#define PATH_DOTDOT	02
#define PATH_EXISTS	04

/*
 * pathaccess() flags
 */

#define PATH_READ	004
#define PATH_WRITE	002
#define PATH_EXECUTE	001
#define	PATH_REGULAR	010
#define PATH_ABSOLUTE	020

/*
 * pathcheck() info
 */

typedef struct
{
	unsigned long	date;
	char*		feature;
	char*		host;
	char*		user;
} Pathcheck_t;

/*
 * strmatch() flags
 */

#define STR_MAXIMAL	01
#define STR_LEFT	02
#define STR_RIGHT	04

/*
 * common macros
 */

#define elementsof(x)	(sizeof(x)/sizeof(x[0]))
#define newof(p,t,n,x)	((p)?(t*)realloc((char*)(p),sizeof(t)*(n)+(x)):(t*)calloc(1,sizeof(t)*(n)+(x)))
#define oldof(p,t,n,x)	((p)?(t*)realloc((char*)(p),sizeof(t)*(n)+(x)):(t*)malloc(sizeof(t)*(n)+(x)))
#define roundof(x,y)	(((x)+(y)-1)&~((y)-1))
#define streq(a,b)	(*(a)==*(b)&&!strcmp(a,b))
#define strneq(a,b,n)	(*(a)==*(b)&&!strncmp(a,b,n))

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
#define NiL		0
#define NoP(x)		(void)(x)
#else
#define NiL		((char*)0)
#define NoP(x)		(&x,1)
#endif

#if !defined(NoN)
#define NoN(x)		int _STUB_ ## x;
#if !defined(_STUB_)
#define _STUB_
#endif
#endif

#define NOT_USED(x)	NoP(x)

typedef int (*Ast_confdisc_t)(const char*, const char*, const char*);

extern char*		astconf(const char*, const char*, const char*);
extern Ast_confdisc_t	astconfdisc(Ast_confdisc_t);
extern void		astconflist(Sfio_t*, const char*, int);
extern off_t		astcopy(int, int, off_t);
extern int		astquery(int, const char*, ...);
extern void		astwinsize(int, int*, int*);

extern int		chresc(const char*, char**);
extern int		chrtoi(const char*);
extern char*		fmtbase(long, int, int);
extern char*		fmtdev(struct stat*);
extern char*		fmtelapsed(unsigned long, int);
extern char*		fmterror(int);
extern char*		fmtesc(const char*);
extern char*		fmtfs(struct stat*);
extern char*		fmtgid(int);
extern char*		fmtmatch(const char*);
extern char*		fmtmode(int, int);
extern char*		fmtperm(int);
extern char*		fmtperm_pos(int);
extern char*		fmtre(const char*);
extern char*		fmtsignal(int);
extern char*		fmttime(const char*, time_t);
extern char*		fmtuid(int);
extern void		mematoe(void*, const void*, size_t);
extern void*		memdup(const void*, size_t);
extern void		memetoa(void*, const void*, size_t);
extern char*		pathaccess(char*, const char*, const char*, const char*, int);
extern char*		pathbin(void);
extern char*		pathcanon(char*, int);
extern char*		pathcat(char*, const char*, int, const char*, const char*);
extern int		pathcd(const char*, const char*);
extern int		pathcheck(const char*, const char*, Pathcheck_t*);
extern int		pathgetlink(const char*, char*, int);
extern char*		pathkey(char*, char*, const char*, const char*);
extern char*		pathpath(char*, const char*, const char*, int);
extern char*		pathprobe(char*, char*, const char*, const char*, const char*, int);
extern char*		pathrepl(char*, const char*, const char*);
extern char*		pathtemp(char*, const char*, const char*);
extern int		pathsetlink(const char*, const char*);
extern char*		pathshell(void);
extern char*		setenviron(const char*);
extern int		sigcritical(int);
extern char*		strcopy(char*, const char*);
extern char*		strdup(const char*);
extern unsigned long	strelapsed(const char*, char**, int);
extern char*		strerror(int);
extern int		stresc(char*);
extern long		streval(const char*, char**, long(*)(const char*, char**));
extern long		strexpr(const char*, char**, long(*)(const char*, char**, void*), void*);
extern int		strgid(const char*);
extern int		strgrpmatch(const char*, const char*, int*, int, int);
extern void*		strlook(const void*, int, const char*);
extern int		strmatch(const char*, const char*);
extern int		strmode(const char*);
extern int		stropt(const char*, const void*, int, int(*)(void*, const void*, int, const char*), void*);
extern int		strperm(const char*, char**, int);
extern char*		strsignal(int);
extern void		strsort(char**, int, int(*)(const char*, const char*));
extern char*		strsubmatch(const char*, const char*, int);
extern char*		strtape(const char*, char**);
extern long		strton(const char*, char**, char*, int);
extern int		struid(const char*);

/*
 * C library global data symbols not prototyped by <unistd.h>
 */

#ifndef environ
#if _DLL_INDIRECT_DATA && _DLL
#define	environ		(*_ast_getdll()->_ast_environ)
#else
extern char**		environ;
#endif
#endif

#endif
