#ident	"@(#)pcintf:bridge/pci_proto.h	1.1"
/*
**	@(#)pci_proto.h	1.3	10/17/91	15:01:10
**	Copyright 1991  Locus Computing Corporation
**
**	PROTO and CONST macros are defined here until support for non-ANSI-
**	compliant C compilers is dropped.
**
**	LOCAL is also defined here for convenience.
**
**	Included are some prototypes for common library routines & system
**	calls not found in standard places.
*/

#if !defined(_PCI_PROTO_H)
#	define	_PCI_PROTO_H


#if defined(PROTO)
#	undef	PROTO
#endif

#if defined(CONST)
#	undef	CONST
#endif

#if defined(__STDC__)
#	define	PROTO(p)	p
#	define	CONST		const
#else
#	define	PROTO(p)	()
#	define	CONST
#endif


/* NDEBUG should be defined in make.mstr by default */
#if defined(NDEBUG)
#	define	LOCAL		static
#else
#	define	LOCAL
#endif


#if defined(__STDC__)
#	include <stdlib.h>
#else
extern int	atoi();
extern void	exit();
extern char	*getenv();
extern long	strtol();
#endif


#if defined(POSIX)
#	include <sys/stat.h>
#	include <sys/wait.h>
#	include <fcntl.h>
#	include <signal.h>
#	include <time.h>
#	include <unistd.h>
#elif defined(USE_PROTOTYPES_H)
#	include <prototypes.h>
#else
#	include <sys/types.h>
#	include <sys/stat.h>
extern int	access		PROTO((const char *, int));
extern unsigned	alarm		PROTO((unsigned));
extern int	chdir		PROTO((const char *));
extern int	chmod		PROTO((const char *, mode_t));
extern int	chown		PROTO((const char *, uid_t, gid_t));
extern int	close		PROTO((int));
extern int	creat		PROTO((const char *, mode_t));
extern int	dup		PROTO((int));
extern int	execl		PROTO((const char *, const char *, ...));
extern int	execv		PROTO((const char *, char * const *));
extern int	execve		PROTO((const char *, char * const *,
							char * const *));
extern int	execvp		PROTO((const char *, char * const *));
extern void	_exit		PROTO((int));
extern int	fcntl		PROTO((int, int, ...));
extern pid_t	fork		PROTO((void));
extern int	fstat		PROTO((int, struct stat *));
extern gid_t	getgid		PROTO((void));
extern pid_t	getpid		PROTO((void));
extern pid_t	getppid		PROTO((void));
extern uid_t	getuid		PROTO((void));
extern int	ioctl		PROTO((int, int, ...));
extern int	isatty		PROTO((int));
extern int	kill		PROTO((pid_t, int));
extern int	link		PROTO((const char *, const char *));
extern off_t	lseek		PROTO((int, off_t, int));
extern int	mkdir		PROTO((const char *, mode_t));
extern int	open		PROTO((const char *, int, .../* mode_t */));
extern int	pipe		PROTO((int *));
extern int	read		PROTO((int, void *, unsigned));
extern int	rmdir		PROTO((const char *));
extern int	setgid		PROTO((gid_t));
extern pid_t	setpgrp		PROTO((void));
extern int	setuid		PROTO((uid_t));
extern unsigned	sleep		PROTO((unsigned));
extern int	stat		PROTO((const char *, struct stat *));
extern char	*ttyname	PROTO((int));
extern void	tzset		PROTO((void));
#if !defined(ISC386)
extern mode_t	umask		PROTO((mode_t));
#endif
extern int	unlink		PROTO((const char *));
extern pid_t	wait		PROTO((int *));
extern int	write		PROTO((int, const void *, unsigned));
#endif

/*
**  X/Open defines putenv(), but doesn't specify a header file for its proto-
**  type.  In practice, SVR4 (at least) puts it in <stdlib.h>, subject to the
**  feature control macros __STDC__, _POSIX_SOURCE and _XOPEN_SOURCE.  It is
**  also being considered as part of a supplement to POSIX.
*/
extern int	putenv		PROTO((char *));

#endif	/* _PCI_PROTO_H */
