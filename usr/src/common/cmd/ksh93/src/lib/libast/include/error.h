#ident	"@(#)ksh93:src/lib/libast/include/error.h	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * option, error and message formatter external definitions
 */

#ifndef _ERROR_H
#define _ERROR_H

#include <ast.h>
#include <option.h>
#include <errno.h>

#if _DLL_INDIRECT_DATA && _DLL
#undef	errno
#define errno	(*_ast_getdll()->_ast_errno)
#endif

#ifndef	error_info
#if _DLL_INDIRECT_DATA && !_DLL
#define error_info	(*_error_info_)
#else
#define error_info	_error_info_
#endif
#endif

#define ERROR_debug(n)	(-(n))
#define ERROR_exit(n)	((n)+ERROR_ERROR)
#define ERROR_system(n)	(((n)+ERROR_ERROR)|ERROR_SYSTEM)
#define ERROR_usage(n)	((((n)?2:0)+ERROR_ERROR)|ERROR_USAGE)
#define ERROR_warn(n)	(ERROR_WARNING)

#define ERROR_dictionary(t)	(t)
#if _hdr_locale && _lib_setlocale
#define ERROR_translate(t,m)	((error_info.translate&&(ast.locale.set&LC_SET_MESSAGES))?(*error_info.translate)(t,m):(t))
#else
#define ERROR_translate(t,m)	(t)
#endif

#define ERROR_INFO	0		/* info message -- no err_id	*/
#define ERROR_WARNING	1		/* warning message		*/
#define ERROR_ERROR	2		/* error message -- no err_exit	*/
#define ERROR_FATAL	3		/* error message with err_exit	*/
#define ERROR_NOEXEC	EXIT_NOEXEC	/* shell convention		*/
#define ERROR_NOENT	EXIT_NOTFOUND	/* shell convention		*/
#define ERROR_PANIC	ERROR_LEVEL	/* panic message with err_exit	*/

#define ERROR_LEVEL	000377		/* level portion of status	*/
#define ERROR_SYSTEM	000400		/* report system errno message	*/
#define ERROR_OUTPUT	001000		/* next arg is error fd		*/
#define ERROR_SOURCE	002000		/* next 2 args are FILE,LINE	*/
#define ERROR_USAGE	004000		/* usage message		*/
#define ERROR_PROMPT	010000		/* omit trailing newline	*/
#define ERROR_NOID	020000		/* omit err_id			*/
#define ERROR_LIBRARY	040000		/* library routine error	*/

#define ERROR_INTERACTIVE	000001	/* context is interactive	*/
#define ERROR_SILENT		000002	/* context is silent		*/

#define errorpush(p,f)	(*(p)=*ERROR_CONTEXT_BASE,*ERROR_CONTEXT_BASE=error_info.empty,error_info.context=(p),error_info.flags=(f))
#define errorpop(p)	(*ERROR_CONTEXT_BASE=*(p))

#define ERROR_CONTEXT_BASE	((Error_context_t*)&error_info.context)

#define ERROR_CONTEXT \
	Error_context_t* context;	/* prev context stack element	*/ \
	int	errors;			/* >= ERROR_ERROR count		*/ \
	int	flags;			/* context flags		*/ \
	int	line;			/* input|output line number	*/ \
	int	warnings;		/* ERROR_WARNING count		*/ \
	char*	file;			/* input|output file name	*/ \
	char*	id			/* command id			*/

typedef struct errorcontext Error_context_t;

struct errorcontext			/* context stack element	*/
{
	ERROR_CONTEXT;
};

typedef struct				/* error state			*/
{
	int	fd;			/* write(2) fd			*/

	void	(*exit)(int);		/* error exit			*/
	ssize_t	(*write)(int, const void*, size_t); /* error output	*/

	/* the rest are implicitly initialized				*/

	int	clear;			/* default clear ERROR_* flags	*/
	int	core;			/* level>=core -> core dump	*/
	int	indent;			/* debug trace indent level	*/
	int	init;			/* initialized			*/
	int	last_errno;		/* last reported errno		*/
	int	mask;			/* multi level debug trace mask	*/
	int	set;			/* default set ERROR_* flags	*/
	int	trace;			/* debug trace level		*/

	char*	version;		/* ERROR_SOURCE command version	*/

	int	(*auxilliary)(Sfio_t*, int, int); /* aux info to append	*/

	ERROR_CONTEXT;			/* top of context stack		*/

	Error_context_t	empty;		/* empty context stack element	*/

	unsigned long	time;		/* debug time trace		*/

	char*	(*translate)(const char*, int);	/* format translator	*/

} Error_info_t;

extern Error_info_t	error_info;	/* global error state		*/

#ifndef errno
extern int	errno;			/* system call error status	*/
#endif

extern void	error(int, ...);
extern void	errorv(const char*, int, va_list);
extern void	liberror(const char*, int, ...);

#endif
