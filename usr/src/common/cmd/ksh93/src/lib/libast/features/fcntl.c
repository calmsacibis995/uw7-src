#ident	"@(#)ksh93:src/lib/libast/features/fcntl.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * generate POSIX fcntl.h
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <ast_lib.h>

#define ioctl		______ioctl
#define printf		______printf

#include "FEATURE/fcntl.lcl"
#include "FEATURE/unistd.lcl"

#undef	ioctl
#undef	printf

#include "FEATURE/tty"

extern int		printf(const char*, ...);

main()
{
	int		f_local = 0;
	int		f_lck = 0;
	int		f_lk = 0;
	int		o_local = 2;
	char		tmp[64];

	printf("#pragma prototyped\n");
	printf("\n");
#if _hdr_lcl_fcntl
	printf("#if defined(__STDPP__directive) && defined(__STDPP__hide)\n");
	printf("__STDPP__directive pragma pp:hide chmod creat fcntl mkdir mkfifo mknod open umask\n");
	printf("#else\n");
	printf("#define chmod	______chmod\n");
	printf("#define creat	______creat\n");
	printf("#define fcntl	______fcntl\n");
	printf("#define mkdir	______mkdir\n");
	printf("#define mkfifo	______mkfifo\n");
	printf("#define mknod	______mknod\n");
	printf("#define open	______open\n");
	printf("#define umask	______umask\n");
	printf("#endif \n");
	printf("\n");
#if defined(S_IRUSR)
	printf("#include <ast_fs.h>	/* <fcntl.h> includes <sys/stat.h>! */\n");
#endif
	printf("#include <fcntl.h>\n");
	printf("\n");
	printf("#if defined(_AST_H) || defined(_POSIX_SOURCE)\n");
	printf("#define _AST_mode_t	mode_t\n");
	printf("#else\n");
	printf("#define _AST_mode_t	int\n");
	printf("#endif\n");
	printf("#if defined(__STDPP__directive) && defined(__STDPP__hide)\n");
	printf("__STDPP__directive pragma pp:nohide chmod creat fcntl mkdir mkfifo mknod open umask\n");
	printf("extern int	creat(const char*, _AST_mode_t);\n");
	printf("extern int	fcntl(int, int, ...);\n");
	printf("extern int	open(const char*, int, ...);\n");
	printf("#else\n");
	printf("#ifdef	creat\n");
	printf("#undef	creat\n");
	printf("extern int	creat(const char*, _AST_mode_t);\n");
	printf("#endif \n");
	printf("#ifdef	fcntl\n");
	printf("#undef	fcntl\n");
	printf("extern int	fcntl(int, int, ...);\n");
	printf("#endif \n");
	printf("#ifdef	open\n");
	printf("#undef	open\n");
	printf("extern int	open(const char*, int, ...);\n");
	printf("#endif \n");
	printf("\n");
	printf("#undef	_AST_mode_t\n");
	printf("#undef	chmod\n");
	printf("#undef	mkdir\n");
	printf("#undef	mkfifo\n");
	printf("#undef	mknod\n");
	printf("#undef	umask\n");
	printf("#endif \n");
	printf("\n");
#endif

#ifndef	FD_CLOEXEC
	printf("#define FD_CLOEXEC	1\n");
	printf("\n");
#endif

#ifndef	F_DUPFD
#define NEED_F	1
#else
	if (F_DUPFD > f_local) f_local = F_DUPFD;
#endif
#ifndef	F_GETFD
#define NEED_F	1
#else
	if (F_GETFD > f_local) f_local = F_GETFD;
#endif
#ifndef	F_GETFL
#define NEED_F	1
#else
	if (F_GETFL > f_local) f_local = F_GETFL;
#endif
#ifndef	F_GETLK
#define NEED_F	1
	f_lk++;
#else
	if (F_GETLK > f_local) f_local = F_GETLK;
#endif
#ifndef	F_RDLCK
#define NEED_F	1
#define NEED_LCK	1
#else
	if (F_RDLCK > f_lck) f_lck = F_RDLCK;
#endif
#ifndef	F_SETFD
#define NEED_F	1
#else
	if (F_SETFD > f_local) f_local = F_SETFD;
#endif
#ifndef	F_SETFL
#define NEED_F	1
#else
	if (F_SETFL > f_local) f_local = F_SETFL;
#endif
#ifndef	F_SETLK
#define NEED_F	1
	f_lk++;
#else
	if (F_SETLK > f_local) f_local = F_SETLK;
#endif
#ifndef	F_SETLKW
#define NEED_F	1
	f_lk++;
#else
	if (F_SETLKW > f_local) f_local = F_SETLKW;
#endif
#ifndef	F_UNLCK
#define NEED_F	1
#define NEED_LCK	1
#else
	if (F_UNLCK > f_lck) f_lck = F_UNLCK;
#endif
#ifndef	F_WRLCK
#define NEED_F	1
#define NEED_LCK	1
#else
	if (F_WRLCK > f_lck) f_lck = F_WRLCK;
#endif

#if	NEED_F
	printf("#define fcntl		_ast_fcntl\n");
#if	_lib_fcntl
	printf("#define _lib_fcntl	1\n");
#endif
	printf("#define _ast_F_LOCAL	%d\n", f_local + 1);
#ifndef	F_DUPFD
	printf("#define F_DUPFD		%d\n", ++f_local);
#endif
#ifndef	F_GETFD
	printf("#define F_GETFD		%d\n", ++f_local);
#endif
#ifndef	F_GETFL
	printf("#define F_GETFL		%d\n", ++f_local);
#endif
#ifndef	F_GETLK
	printf("#define F_GETLK		%d\n", ++f_local);
#endif
#ifndef	F_SETFD
	printf("#define F_SETFD		%d\n", ++f_local);
#endif
#ifndef	F_SETFL
	printf("#define F_SETFL		%d\n", ++f_local);
#endif
#ifndef	F_SETLK
	printf("#define F_SETLK		%d\n", ++f_local);
#endif
#ifndef	F_SETLKW
	printf("#define F_SETLKW	%d\n", ++f_local);
#endif
#if	NEED_LCK
	printf("\n");
#ifndef	F_RDLCK
	printf("#define F_RDLCK		%d\n", f_lck++);
#endif
#ifndef	F_WRLCK
	printf("#define F_WRLCK		%d\n", f_lck++);
#endif
#ifndef	F_UNLCK
	printf("#define F_UNLCK		%d\n", f_lck++);
#endif
#endif
	printf("\n");
	if (f_lck == 3)
	{
		printf("struct flock\n");
		printf("{\n");
		printf("	short	l_type;\n");
		printf("	short	l_whence;\n");
		printf("	off_t	l_start;\n");
		printf("	off_t	l_len;\n");
		printf("	short	l_pid;\n");
		printf("};\n");
		printf("\n");
	}
	printf("\n");
#endif

#ifndef	O_APPEND
#define NEED_O	1
#else
	if (O_APPEND > o_local) o_local = O_APPEND;
#endif
#ifndef	O_CREAT
#define NEED_O	1
#else
	if (O_CREAT > o_local) o_local = O_CREAT;
#endif
#ifndef	O_EXCL
#define NEED_O	1
#else
	if (O_EXCL > o_local) o_local = O_EXCL;
#endif
#ifndef	O_NOCTTY
#ifdef	TIOCNOTTY
#define NEED_O	1
#endif
#else
	if (O_NOCTTY > o_local) o_local = O_NOCTTY;
#endif
#ifndef	O_NONBLOCK
#ifndef	O_NDELAY
#define NEED_O	1
#endif
#else
	if (O_NONBLOCK > o_local) o_local = O_NONBLOCK;
#endif
#ifndef	O_RDONLY
#define NEED_O	1
#endif
#ifndef	O_RDWR
#define NEED_O	1
#endif
#ifndef	O_TRUNC
#define NEED_O	1
#else
	if (O_TRUNC > o_local) o_local = O_TRUNC;
#endif
#ifndef	O_WRONLY
#define NEED_O	1
#endif

#if	NEED_O
	printf("#define open		_ast_open\n");
	printf("#define _ast_O_LOCAL	0%o\n", o_local<<1);
#ifndef	O_RDONLY
	printf("#define O_RDONLY	0\n");
#endif
#ifndef	O_WRONLY
	printf("#define O_WRONLY	1\n");
#endif
#ifndef	O_RDWR
	printf("#define O_RDWR		2\n");
#endif
#ifndef	O_APPEND
	printf("#define O_APPEND	0%o\n", o_local <<= 1);
#endif
#ifndef	O_CREAT
	printf("#define O_CREAT		0%o\n", o_local <<= 1);
#endif
#ifndef	O_EXCL
	printf("#define O_EXCL		0%o\n", o_local <<= 1);
#endif
#ifndef	O_NOCTTY
#ifdef	TIOCNOTTY
	printf("#define O_NOCTTY	0%o\n", o_local <<= 1);
#endif
#endif
#ifndef	O_NONBLOCK
#ifndef	O_NDELAY
	printf("#define O_NONBLOCK	0%o\n", o_local <<= 1);
#endif
#endif
#ifndef	O_TRUNC
	printf("#define O_TRUNC		0%o\n", o_local <<= 1);
#endif
#endif
#ifndef	O_ACCMODE
	printf("#define O_ACCMODE	(O_RDONLY|O_WRONLY|O_RDWR)\n");
#endif
#ifndef	O_NOCTTY
#ifndef	TIOCNOTTY
	printf("#define O_NOCTTY	0\n");
#endif
#endif
#ifndef	O_NONBLOCK
#ifdef	O_NDELAY
	printf("#define O_NONBLOCK	O_NDELAY\n");
#endif
#endif
#if	NEED_F || NEED_O
	printf("\n");
#if	NEED_F
	printf("extern int	fcntl(int, int, ...);\n");
#endif
#if	NEED_O
	printf("extern int	open(const char*, int, ...);\n");
#endif
#endif

	return(0);
}
