#ident	"@(#)ksh93:src/lib/libast/include/wait.h	1.1"
#pragma prototyped
/*
 * ast POSIX wait/exit support
 */

#ifndef _WAIT_H
#define _WAIT_H

#include <ast.h>
#include <ast_wait.h>

#if _sys_wait
#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:hide wait waitpid
#else
#define wait		______wait
#define waitpid		______waitpid
#endif
#include <sys/wait.h>
#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:nohide wait waitpid
#else
#undef	wait
#undef	waitpid
#endif
#endif

#ifndef WNOHANG
#define WNOHANG		1
#endif

#ifndef WUNTRACED
#define WUNTRACED	2
#endif

#if !_ok_wif
#undef	WIFEXITED
#undef	WEXITSTATUS
#undef	WIFSIGNALED
#undef	WTERMSIG
#undef	WIFSTOPPED
#undef	WSTOPSIG
#undef	WTERMCORE
#endif

#ifndef WIFEXITED
#define WIFEXITED(x)	(!((x)&((1<<(EXIT_BITS-1))-1)))
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(x)	(((x)>>EXIT_BITS)&((1<<EXIT_BITS)-1))
#endif

#ifndef WIFSIGNALED
#define WIFSIGNALED(x)	(((x)&((1<<(EXIT_BITS-1))-1))!=0)
#endif

#ifndef WTERMSIG
#define WTERMSIG(x)	((x)&((1<<(EXIT_BITS-1))-1))
#endif

#ifndef WIFSTOPPED
#define WIFSTOPPED(x)	(((x)&((1<<EXIT_BITS)-1))==((1<<(EXIT_BITS-1))-1))
#endif

#ifndef WSTOPSIG
#define WSTOPSIG(x)	WEXITSTATUS(x)
#endif

#ifndef WTERMCORE
#define WTERMCORE(x)	((x)&(1<<(EXIT_BITS-1)))
#endif

extern pid_t		wait(int*);
extern pid_t		waitpid(pid_t, int*, int);

#endif
