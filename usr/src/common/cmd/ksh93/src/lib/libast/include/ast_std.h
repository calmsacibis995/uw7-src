#ident	"@(#)ksh93:src/lib/libast/include/ast_std.h	1.1"
#pragma prototyped
/*
 * Advanced Software Technology Department
 * AT&T Bell Laboratories
 *
 * a union of the following standard headers that works
 *
 *	<limits.h>
 *	<stdarg.h>
 *	<stddef.h>
 *	<stdlib.h>
 *	<sys/types.h>
 *	<string.h>
 *	<unistd.h>
 *	<fcntl.h>
 *	<locale.h>
 *
 * the following ast implementation specific headers are also included
 * these do not stomp on the std namespace
 *
 *	<ast_botch.h>
 *	<ast_fcntl.h>
 *	<ast_hdr.h>
 *	<ast_lib.h>
 *	<ast_types.h>
 *	<ast_unistd.h>
 *
 * NOTE: the C++ versions of most ANSI and POSIX files are fubar
 */

#ifndef _AST_STD_H
#define _AST_STD_H

#include <ast_hdr.h>

#ifndef	_ast_info
#if _DLL_INDIRECT_DATA && !_DLL
#define _ast_info	(*_ast_state)
#else
#define _ast_info	_ast_state
#endif
#endif

#ifdef	_SFSTDIO_H
#define _SKIP_SFSTDIO_H
#else
#define _SFSTDIO_H
#define FILE	int
#if defined(__STDPP__directive) && defined(__STDPP__hide)
#if !_std_def_calloc
__STDPP__directive pragma pp:hide calloc
#endif
#if !_std_def_free
__STDPP__directive pragma pp:hide free
#endif
#if !_std_def_malloc
__STDPP__directive pragma pp:hide malloc
#endif
#if !_std_def_realloc
__STDPP__directive pragma pp:hide realloc
#endif
__STDPP__directive pragma pp:hide bcopy bzero execl execle execlp execv
__STDPP__directive pragma pp:hide execvp getcwd putenv setenv setpgrp sleep
__STDPP__directive pragma pp:hide spawnve spawnveg strdup
__STDPP__directive pragma pp:hide vfprintf vprintf vsprintf
#else
#if !_std_def_calloc
#define calloc		______calloc
#endif
#if !_std_def_free
#define free		______free
#endif
#if !_std_def_malloc
#define malloc		______malloc
#endif
#if !_std_def_realloc
#define realloc		______realloc
#endif
#define bcopy		______bcopy
#define bzero		______bzero
#define execl		______execl
#define execle		______execle
#define execlp		______execlp
#define execv		______execv
#define execvp		______execvp
#define getcwd		______getcwd
#define putenv		______putenv
#define setenv		______setenv
#define setpgrp		______setpgrp
#define sleep		______sleep
#define spawnve		______spawnve
#define spawnveg	______spawnveg
#define strdup		______strdup
#define vfprintf	______vfprintf
#define vprintf		______vprintf
#define vsprintf	______vsprintf
#endif
#endif

#if defined(__STDPP__directive) && defined(__STDPP__note)
__STDPP__directive pragma pp:note clock_t dev_t div_t gid_t ino_t ldiv_t mode_t
__STDPP__directive pragma pp:note nlink_t off_t pid_t ptrdiff_t size_t ssize_t
__STDPP__directive pragma pp:note time_t uid_t wchar_t
#endif

#include <sys/types.h>
#include <stdarg.h>

#if defined(__STDPP__directive) && defined(__STDPP__initial)
__STDPP__directive pragma pp:initial
#endif
#include <limits.h>
#if defined(__STDPP__directive) && defined(__STDPP__initial)
__STDPP__directive pragma pp:noinitial
#endif

#if defined(_std_stddef) || defined(__STDC__) || defined(__cplusplus)

#include <stddef.h>

#endif

#ifndef offsetof
#define offsetof(type,member) ((size_t)&(((type*)0)->member))
#endif

#if !defined(__cplusplus) && (defined(_std_stdlib) || defined(__STDC__))

#include <stdlib.h>

#else

#define EXIT_FAILURE	1
#define EXIT_SUCCESS	0
#define MB_CUR_MAX	1
#define RAND_MAX	32767

#endif

#ifdef	_SKIP_SFSTDIO_H
#undef	_SKIP_SFSTDIO_H
#else
#undef	_SFSTDIO_H
#undef	FILE
#if defined(__STDPP__directive) && defined(__STDPP__hide)
#if !_std_def_calloc
__STDPP__directive pragma pp:nohide calloc
#endif
#if !_std_def_free
__STDPP__directive pragma pp:nohide free
#endif
#if !_std_def_malloc
__STDPP__directive pragma pp:nohide malloc
#endif
#if !_std_def_realloc
__STDPP__directive pragma pp:nohide realloc
#endif
__STDPP__directive pragma pp:nohide bcopy bzero execl execle execlp execv
__STDPP__directive pragma pp:nohide execvp getcwd putenv setenv setpgrp sleep
__STDPP__directive pragma pp:nohide strdup spawnve spawnveg
__STDPP__directive pragma pp:nohide vfprintf vprintf vsprintf
#else
#if !_std_def_calloc
#undef	calloc	
#endif
#if !_std_def_free
#undef	free	
#endif
#if !_std_def_malloc
#undef	malloc	
#endif
#if !_std_def_realloc
#undef	realloc	
#endif
#undef	bcopy
#undef	bzero
#undef	execl
#undef	execle
#undef	execlp
#undef	execv
#undef	execvp
#undef	getcwd
#undef	putenv
#undef	setenv
#undef	setpgrp
#undef	sleep
#undef	spawnve
#undef	spawnveg
#undef	strdup
#undef	vfprintf
#undef	vprintf
#undef	vsprintf
#endif
#endif

#if defined(__STDPP__directive) && defined(__STDPP__note)
#if !noticed(dev_t)
typedef short dev_t;
#endif
#if !noticed(div_t)
typedef struct { int quot; int rem; } div_t;
#endif
#if !noticed(gid_t)
typedef unsigned short gid_t;
#endif
#if !noticed(ino_t)
typedef unsigned long ino_t;
#endif
#if !noticed(ldiv_t)
typedef struct { long quot; long rem; } ldiv_t;
#endif
#if !noticed(mode_t)
typedef unsigned short mode_t;
#endif
#if !noticed(nlink_t)
typedef short nlink_t;
#endif
#if !noticed(off_t)
typedef long off_t;
#endif
#if !noticed(pid_t)
typedef int pid_t;
#endif
#if !noticed(ptrdiff_t)
typedef long ptrdiff_t;
#endif
#if !noticed(size_t)
typedef unsigned int size_t;
#endif
#if !noticed(ssize_t)
typedef int ssize_t;
#endif
#if !noticed(uid_t)
typedef unsigned short uid_t;
#endif
#if !noticed(wchar_t)
typedef unsigned short wchar_t;
#endif
#else
#include <ast_types.h>
#endif

struct stat;

#if defined(__cplusplus) || !defined(_std_stdlib) && !defined(__STDC__)

/* <stdlib.h> */

extern double		atof(const char*);
extern int		atoi(const char*);
extern long		atol(const char*);
extern double		strtod(const char*, char**);
extern long		strtol(const char*, char**, int);
extern unsigned long	strtoul(const char*, char**, int);

extern int		rand(void);
extern void		srand(unsigned int);

extern void		abort(void);
extern int		atexit(void(*)(void));
extern void		exit(int);
extern char*		getenv(const char*);
extern int		system(const char*);

extern void*		bsearch(const void*, const void*, size_t, size_t,
		 		int(*)(const void*, const void*));
extern void		qsort(void*, size_t, size_t,
				int(*)(const void*, const void*));

extern int		abs(int);
extern div_t		div(int, int);
extern long		labs(long);
extern ldiv_t		ldiv(long, long);

extern int		mblen(const char*, size_t);
extern int		mbtowc(wchar_t*, const char*, size_t);
extern int		wctomb(char*, wchar_t);
extern size_t		mbstowcs(wchar_t*, const char*, size_t);
extern size_t		wcstombs(char*, const wchar_t*, size_t);

#endif

#if !_std_def_calloc
extern void*		calloc(size_t, size_t);
#endif
#if !_std_def_free
extern void		free(void*);
#endif
#if !_std_def_malloc
extern void*		malloc(size_t);
#endif
#if !_std_def_realloc
extern void*		realloc(void*, size_t);
#endif

/* <string.h> */

extern void*		memchr(const void*, int, size_t);
extern int		memcmp(const void*, const void*, size_t);
extern void*		memcpy(void*, const void*, size_t);
extern void*		memmove(void*, const void*, size_t);
extern void*		memset(void*, int, size_t);
extern char*		strcat(char*, const char*);
extern char*		strchr(const char*, int);
extern int		strcmp(const char*, const char*);
extern int		strcoll(const char*, const char*);
extern char*		strcpy(char*, const char*);
extern size_t		strcspn(const char*, const char*);
extern size_t		strlen(const char*);
extern char*		strncat(char*, const char*, size_t);
extern int		strncmp(const char*, const char*, size_t);
extern char*		strncpy(char*, const char*, size_t);
extern char*		strpbrk(const char*, const char*);
extern char*		strrchr(const char*, int);
extern size_t		strspn(const char*, const char*);
extern char*		strstr(const char*, const char*);
extern char*		strtok(char*, const char*);
extern size_t		strxfrm(char*, const char*, size_t);

#if defined(__STDPP__directive) && defined(__STDPP__ignore)

__STDPP__directive pragma pp:ignore "libc.h"
__STDPP__directive pragma pp:ignore "malloc.h"
__STDPP__directive pragma pp:ignore "memory.h"
__STDPP__directive pragma pp:ignore "stdlib.h"
__STDPP__directive pragma pp:ignore "string.h"

#else

#ifndef _libc_h
#define _libc_h
#endif
#ifndef _libc_h_
#define _libc_h_
#endif
#ifndef __libc_h
#define __libc_h
#endif
#ifndef __libc_h__
#define __libc_h__
#endif
#ifndef _LIBC_H
#define _LIBC_H
#endif
#ifndef _LIBC_H_
#define _LIBC_H_
#endif
#ifndef __LIBC_H
#define __LIBC_H
#endif
#ifndef __LIBC_H__
#define __LIBC_H__
#endif
#ifndef _LIBC_INCLUDED
#define _LIBC_INCLUDED
#endif
#ifndef __LIBC_INCLUDED
#define __LIBC_INCLUDED
#endif
#ifndef _H_LIBC
#define _H_LIBC
#endif
#ifndef __H_LIBC
#define __H_LIBC
#endif

#ifndef _malloc_h
#define _malloc_h
#endif
#ifndef _malloc_h_
#define _malloc_h_
#endif
#ifndef __malloc_h
#define __malloc_h
#endif
#ifndef __malloc_h__
#define __malloc_h__
#endif
#ifndef _MALLOC_H
#define _MALLOC_H
#endif
#ifndef _MALLOC_H_
#define _MALLOC_H_
#endif
#ifndef __MALLOC_H
#define __MALLOC_H
#endif
#ifndef __MALLOC_H__
#define __MALLOC_H__
#endif
#ifndef _MALLOC_INCLUDED
#define _MALLOC_INCLUDED
#endif
#ifndef __MALLOC_INCLUDED
#define __MALLOC_INCLUDED
#endif
#ifndef _H_MALLOC
#define _H_MALLOC
#endif
#ifndef __H_MALLOC
#define __H_MALLOC
#endif

#ifndef _memory_h
#define _memory_h
#endif
#ifndef _memory_h_
#define _memory_h_
#endif
#ifndef __memory_h
#define __memory_h
#endif
#ifndef __memory_h__
#define __memory_h__
#endif
#ifndef _MEMORY_H
#define _MEMORY_H
#endif
#ifndef _MEMORY_H_
#define _MEMORY_H_
#endif
#ifndef __MEMORY_H
#define __MEMORY_H
#endif
#ifndef __MEMORY_H__
#define __MEMORY_H__
#endif
#ifndef _MEMORY_INCLUDED
#define _MEMORY_INCLUDED
#endif
#ifndef __MEMORY_INCLUDED
#define __MEMORY_INCLUDED
#endif
#ifndef _H_MEMORY
#define _H_MEMORY
#endif
#ifndef __H_MEMORY
#define __H_MEMORY
#endif

#ifndef _stdlib_h
#define _stdlib_h
#endif
#ifndef _stdlib_h_
#define _stdlib_h_
#endif
#ifndef __stdlib_h
#define __stdlib_h
#endif
#ifndef __stdlib_h__
#define __stdlib_h__
#endif
#ifndef _STDLIB_H
#define _STDLIB_H
#endif
#ifndef _STDLIB_H_
#define _STDLIB_H_
#endif
#ifndef __STDLIB_H
#define __STDLIB_H
#endif
#ifndef __STDLIB_H__
#define __STDLIB_H__
#endif
#ifndef _STDLIB_INCLUDED
#define _STDLIB_INCLUDED
#endif
#ifndef __STDLIB_INCLUDED
#define __STDLIB_INCLUDED
#endif
#ifndef _H_STDLIB
#define _H_STDLIB
#endif
#ifndef __H_STDLIB
#define __H_STDLIB
#endif

#ifndef _string_h
#define _string_h
#endif
#ifndef _string_h_
#define _string_h_
#endif
#ifndef __string_h
#define __string_h
#endif
#ifndef __string_h__
#define __string_h__
#endif
#ifndef _STRING_H
#define _STRING_H
#endif
#ifndef _STRING_H_
#define _STRING_H_
#endif
#ifndef __STRING_H
#define __STRING_H
#endif
#ifndef __STRING_H__
#define __STRING_H__
#endif
#ifndef _STRING_INCLUDED
#define _STRING_INCLUDED
#endif
#ifndef __STRING_INCLUDED
#define __STRING_INCLUDED
#endif
#ifndef _H_STRING
#define _H_STRING
#endif
#ifndef __H_STRING
#define __H_STRING
#endif

#endif

#include <ast_fcntl.h>

/* <unistd.h> */

#ifdef _WIN32
#include <unistd.h>
#else
#include <ast_unistd.h>
#endif
#include <ast_botch.h>

#ifndef STDIN_FILENO
#define	STDIN_FILENO	0
#define	STDOUT_FILENO	1
#define	STDERR_FILENO	2
#endif

#ifndef NULL
#define	NULL		0
#endif

#ifndef SEEK_SET
#define	SEEK_SET	0
#define	SEEK_CUR	1
#define	SEEK_END	2
#endif

#ifndef	F_OK
#define	F_OK		0
#define	X_OK		1
#define	W_OK		2
#define	R_OK		4
#endif

extern void		_exit(int);
extern int		access(const char*, int);
extern unsigned		alarm(unsigned);
extern int		chdir(const char*);
extern int		chown(const char*, uid_t, gid_t);
extern int		close(int);
extern size_t		confstr(int, char*, size_t);
extern int		dup(int);
extern int		dup2(int, int);
extern int		execl(const char*, const char*, ...);
extern int		execle(const char*, const char*, ...);
extern int		execlp(const char*, const char*, ...);
extern int		execv(const char*, char* const[]);
extern int		execve(const char*, char* const[], char* const[]);
extern int		execvp(const char*, char* const[]);
extern pid_t		fork(void);
extern long		fpathconf(int, int);
extern char*		getcwd(char*, size_t);
extern gid_t		getegid(void);
extern uid_t		geteuid(void);
extern gid_t		getgid(void);
extern int		getgroups(int, gid_t[]);
extern char*		getlogin(void);
extern pid_t		getpgrp(void);
extern pid_t		getpid(void);
extern pid_t		getppid(void);
extern uid_t		getuid(void);
extern int		isatty(int);
extern int		link(const char*, const char*);
extern off_t		lseek(int, off_t, int);
extern long		pathconf(const char*, int);
extern int		pause(void);
extern int		pipe(int[]);
extern ssize_t		read(int, void*, size_t);
extern int		rmdir(const char*);
extern int		setgid(gid_t);
extern int		setpgid(pid_t, pid_t);
extern pid_t		setsid(void);
extern int		setuid(uid_t);
extern unsigned		sleep(unsigned int);
extern pid_t		spawnve(const char*, char* const[], char* const[]);
extern pid_t		spawnveg(const char*, char* const[], char* const[], pid_t);
extern long		sysconf(int);
extern pid_t		tcgetpgrp(int);
extern int		tcsetpgrp(int, pid_t);
extern char*		ttyname(int);
extern int		unlink(const char*);
extern ssize_t		write(int, const void*, size_t);

#ifndef _WIN32

/*
 * yes, we don't trust anyone's interpretation but our own
 */

#undef	confstr
#define confstr		_ast_confstr
#undef	fpathconf
#define fpathconf	_ast_fpathconf
#undef	pathconf
#define pathconf	_ast_pathconf
#undef	sysconf
#define sysconf		_ast_sysconf

extern size_t		confstr(int, char*, size_t);
extern long		fpathconf(int, int);
extern long		pathconf(const char*, int);
extern long		sysconf(int);

#endif

#if defined(__STDPP__directive) && defined(__STDPP__ignore)

__STDPP__directive pragma pp:ignore "unistd.h"
__STDPP__directive pragma pp:ignore "sys/unistd.h"

#else

#ifndef _unistd_h
#define _unistd_h
#endif
#ifndef _unistd_h_
#define _unistd_h_
#endif
#ifndef __unistd_h
#define __unistd_h
#endif
#ifndef __unistd_h__
#define __unistd_h__
#endif
#ifndef _UNISTD_H
#define _UNISTD_H
#endif
#ifndef _UNISTD_H_
#define _UNISTD_H_
#endif
#ifndef __UNISTD_H
#define __UNISTD_H
#endif
#ifndef __UNISTD_H__
#define __UNISTD_H__
#endif
#ifndef _UNISTD_INCLUDED
#define _UNISTD_INCLUDED
#endif
#ifndef __UNISTD_INCLUDED
#define __UNISTD_INCLUDED
#endif
#ifndef _H_UNISTD
#define _H_UNISTD
#endif
#ifndef __H_UNISTD
#define __H_UNISTD
#endif
#ifndef _SYS_UNISTD_H
#define _SYS_UNISTD_H
#endif

#endif

#if defined(__cplusplus)

#if defined(__STDPP__directive) && defined(__STDPP__ignore)

__STDPP__directive pragma pp:ignore "sysent.h"

#else

#ifndef _sysent_h
#define _sysent_h
#endif
#ifndef _sysent_h_
#define _sysent_h_
#endif
#ifndef __sysent_h
#define __sysent_h
#endif
#ifndef __sysent_h__
#define __sysent_h__
#endif
#ifndef _SYSENT_H
#define _SYSENT_H
#endif
#ifndef _SYSENT_H_
#define _SYSENT_H_
#endif
#ifndef __SYSENT_H
#define __SYSENT_H
#endif
#ifndef __SYSENT_H__
#define __SYSENT_H__
#endif
#ifndef _SYSENT_INCLUDED
#define _SYSENT_INCLUDED
#endif
#ifndef __SYSENT_INCLUDED
#define __SYSENT_INCLUDED
#endif
#ifndef _H_SYSENT
#define _H_SYSENT
#endif
#ifndef __H_SYSENT
#define __H_SYSENT
#endif

#endif

#endif

#include <ast_lib.h>

/* locale stuff */

#if _hdr_locale

#include <locale.h>

#if _sys_localedef

#include <sys/localedef.h>

#endif

#endif

#undef	setlocale
extern char*		_ast_setlocale(int, const char*);
#ifdef LC_ALL
#define setlocale	_ast_setlocale
#else
#define setlocale(a,b)	_ast_setlocale(0,b)
#endif

#undef	strcoll
#if _std_strcoll
#define strcoll		_ast_info.collate
#else
#define strcoll		strcmp
#endif

#define LC_SET_COLLATE	(1<<0)
#define LC_SET_CTYPE	(1<<1)
#define LC_SET_MESSAGES	(1<<2)
#define LC_SET_MONETARY	(1<<3)
#define LC_SET_NUMERIC	(1<<4)
#define LC_SET_TIME	(1<<5)

typedef struct
{

	struct
	{
	unsigned int	serial;
	unsigned int	set;
	}		locale;

	long		tmp_long;
	size_t		tmp_size;
	int		tmp_int;
	short		tmp_short;
	char		tmp_char;
	wchar_t		tmp_wchar;

	int		(*collate)(const char*, const char*);

	void*		tmp_pointer;

} _Ast_info_t;

extern _Ast_info_t	_ast_info;

/* stuff from std headers not used by ast, e.g., <stdio.h> */

extern void*		memzero(void*, size_t);
extern int		remove(const char*);
extern int		rename(const char*, const char*);

/* direct macro access for bsd crossover */

#if !defined(memcpy) && !defined(_lib_memcpy) && defined(_lib_bcopy)
extern void		bcopy(void*, void*, size_t);
#define memcpy(t,f,n)	(bcopy(f,t,n),(t))
#endif

#if !defined(memzero)
#if defined(_lib_bzero)
extern void		bzero(void*, size_t);
#if defined(FD_ZERO)
#undef	FD_ZERO
#define FD_ZERO(p)	memzero(p,sizeof(*p))
#endif
#define memzero(b,n)	(bzero(b,n),(b))
#else
#define memzero(b,n)	memset(b,0,n)
#endif
#endif

#if !defined(remove) && !defined(_lib_remove)
extern int		unlink(const char*);
#define remove(p)	unlink(p)
#endif

#if !defined(strchr) && !defined(_lib_strchr) && defined(_lib_index)
extern char*		index(const char*, int);
#define strchr(s,c)	index(s,c)
#endif

#if !defined(strrchr) && !defined(_lib_strrchr) && defined(_lib_rindex)
extern char*		rindex(const char*, int);
#define strrchr(s,c)	rindex(s,c)
#endif

/* and now introducing prototypes botched by the standard(s) */

#undef	getpgrp
#define	getpgrp()	_ast_getpgrp()
extern int		_ast_getpgrp(void);

#endif
