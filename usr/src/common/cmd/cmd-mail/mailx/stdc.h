#ident	"@(#)stdc.h	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/stdc.h	1.4.5.3"
#ident "@(#)stdc.h	1.16 'attmail mail(1) command'"
/*
    This header file #include's all of the appropriate
    standard C library header files used by the mail programs.
*/
#ifndef __STDC_H
# define __STDC_H

#include	<errno.h>
#include	<fcntl.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<sys/types.h>
#include	<sys/param.h>
#include	<errno.h>
#include	<pwd.h>
#include	<signal.h>
#include	<string.h>
#include	<grp.h>
#include	<unistd.h>
#include	<time.h>
#include	<sys/stat.h>
#include	<setjmp.h>
#include	<sys/utsname.h>

/* The following typedefs must be used in SVR4 */
#ifdef SVR3
# ifndef sun
typedef unsigned short gid_t;
typedef unsigned short uid_t;
typedef int pid_t;
typedef int mode_t;
# endif /* sun */
#endif /* SVR3 */

#ifdef __STDC__
#define ARGS(x)	x
#else
#define ARGS(x) (/* nothing */)
#endif

#ifdef SVR3
# define	YESSTR	41		/* affirmative response for yes/no queries */
# define	NOSTR	42		/* negative response for yes/no queries */
extern char *nl_langinfo ARGS((int));	/* get a string from the database	*/
#else
# include <langinfo.h>
#endif

#ifdef __STDC__
# include <stdarg.h>
# include <locale.h>
# include <stddef.h>
# include <stdlib.h>
# include <ulimit.h>
# include <wait.h>
# define VOID void	/* VOID* -> void* */
#else /* __STDC__ */
# include <varargs.h>

typedef int sig_atomic_t;

# define const		/* nothing */
# define volatile	/* nothing */
# define VOID char	/* VOID* -> char* */

extern	unsigned	alarm();
extern	long	atol();
extern	int	chmod();
extern	int	close();
extern	char	*ctime();
extern	int	errno;
extern	int	execl();
extern	int	execvp();
extern	void	exit();
extern	void	_exit();
extern	int	fork();
extern	void	free();
extern	long	ftell();
extern	char	*getcwd();
extern	gid_t	getegid();
extern	char	*getenv();
extern	uid_t	geteuid();
extern	gid_t	getgid();
extern	struct group *getgrnam();
extern	char	*getlogin();
extern	struct passwd *getpwent();
extern	struct passwd *getpwnam();
extern	struct passwd *getpwuid();
extern	uid_t	getuid();
extern	char	*malloc();
extern	char	*memcpy();
extern	char	*memmove();
extern	char	*mktemp();
extern	void	perror();
extern	void	qsort();
extern	char	*realloc();
extern	int	rename();
extern	char	*setlocale();
extern	void	setpwent();
extern	unsigned	sleep();
extern	char	*strchr();
extern	char	*strcpy();
extern	char	*strerror();
extern	char	*strncpy();
extern	char	*strpbrk();
extern	char	*strrchr();
extern	char	*strstr();
extern	time_t	time();
extern	char	*tempnam();
extern	FILE	*tmpfile();
extern	int	unlink();
extern	int	wait();

# ifndef MAXPATHLEN
#  define MAXPATHLEN 1024
# endif /* !MAXPATHLEN */

# ifndef FILENAME_MAX
#  define FILENAME_MAX 1024
# endif /* !FILENAME_MAX */

/* arguments for access(2) defined in <unistd.h> */
# ifndef R_OK
#  define	R_OK	4	/* Test for Read permission */
#  define	W_OK	2	/* Test for Write permission */
#  define	X_OK	1	/* Test for eXecute permission */
#  define	F_OK	0	/* Test for existence of File */
# endif /* !R_OK */

/* arguments for fseek(3) defined in <stdio.h> */
# ifndef SEEK_SET
#  define SEEK_SET	0
#  define SEEK_CUR	1
#  define SEEK_END	2
# endif

/* arguments for ulimit(2) defined in <ulimit.h> */
# ifndef UL_GFILLIM
#  define UL_GFILLIM      1       /* get file limit */
#  define UL_SFILLIM      2       /* set file limit */
#  define UL_GMEMLIM      3       /* get process size limit */
#  define UL_GDESLIM      4       /* get file descriptor limit */
# endif /* !UL_GFILLIM */

#ifndef LC_ALL
# define LC_ALL 6
#endif
#endif /* !__STDC__ */

extern	char	*optarg;	/* for getopt */
extern	int	optind;

#ifdef SVR3
   struct utimbuf {
	time_t	actime;
	time_t	modtime;
   };
#else /* SVR3 */
# include	<utime.h>
#endif /* SVR3 */

#ifdef SVR4_1
# include <pfmt.h>
#else /* SVR4_1 */
# define MM_ERROR	0
# define MM_HALT	1
# define MM_WARNING	2
# define MM_INFO	3
# define MM_NOSTD	0x100
# define MM_NOGET	0x200
# define MM_ACTION	0x400
# define MM_CONSOLE	0x800

extern int pfmt ARGS((FILE *, long, const char *, ...));
extern int lfmt ARGS((FILE *out, long error_class, const char *fmt, ...));
extern int vpfmt ARGS((FILE *, long, const char *, va_list));
extern const char *setcat ARGS((const char *));
extern int setlabel ARGS((const char *));
extern int addsev ARGS((int, const char *msg));
extern char *gettxt ARGS((const char *id, const char *msg));
#endif /* SVR4_1 */

#ifndef MAXLABEL
# define MAXLABEL	25
#endif

#endif /* __STDC_H */
