#ident	"@(#)ktool:i386at/ktool/scodb/pfe.c	1.1"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

#include	<stdio.h>
#include	<varargs.h>

#define		NERRN		(sizeof(sys_errn)/sizeof(sys_errn[0]))
char *sys_errn[] = {
	0,
	"EPERM",
	"ENOENT",
	"ESRCH",
	"EINTR",
	"EIO",
	"ENXIO",
	"E2BIG",
	"ENOEXEC",
	"EBADF",
	"ECHILD",
	"EAGAIN",
	"ENOMEM",
	"EACCES",
	"EFAULT",
	"ENOTBLK",
	"EBUSY",
	"EEXIST",
	"EXDEV",
	"ENODEV",
	"ENOTDIR",
	"EISDIR",
	"EINVAL",
	"ENFILE",
	"EMFILE",
	"ENOTTY",
	"ETXTBSY",
	"EFBIG",
	"ENOSPC",
	"ESPIPE",
	"EROFS",
	"EMLINK",
	"EPIPE",
	"EDOM",
	"ERANGE",
	"ENOMSG",
	"EIDRM",
	"ECHRNG",
	"EL2NSYNC",
	"EL3HLT",
	"EL3RST",
	"ELNRNG",
	"EUNATCH",
	"ENOCSI",
	"EL2HLT",
	"EDEADLK",
	"ENOLCK",
	0,
	0,
	0,
	"EBADE",
	"EBADR",
	"EXFULL",
	"ENOANO",
	"EBADRQC",
	"EBADSLT",
	"EDEADLOCK",
	"EBFONT",
	0,
	0,
	"ENOSTR",
	"ENODATA",
	"ETIME",
	"ENOSR",
	"ENONET",
	"ENOPKG",
	"EREMOTE",
	"ENOLINK",
	"EADV",
	"ESRMNT",
	"ECOMM",
	"EPROTO",
	0,
	0,
	"EMULTIHOP",
	"ELBIN",
	"EDOTDOT",
	"EBADMSG",
	"ENAMETOOLONG",
	0,
	"ENOTUNIQ",
	"EBADFD",
	"EREMCHG",
	"ELIBACC",
	"ELIBBAD",
	"ELIBSCN",
	"ELIBMAX",
	"ELIBEXEC",
	0,
	"ENOSYS",
	"EWOULDBLOCK",
	"EINPROGRESS",
	"EALREADY",
	"ENOTSOCK",
	"EDESTADDRREQ",
	"EMSGSIZE",
	"EPROTOTYPE",
	"EPROTONOSUPPORT",
	"ESOCKTNOSUPPORT",
	"EOPNOTSUPP",
	"EPFNOSUPPORT",
	"EAFNOSUPPORT",
	"EADDRINUSE",
	"EADDRNOTAVAIL",
	"ENETDOWN",
	"ENETUNREACH",
	"ENETRESET",
	"ECONNABORTED",
	"ECONNRESET",
	0,
	"EISCONN",
	"ENOTCONN",
	"ESHUTDOWN",
	"ETOOMANYREFS",
	"ETIMEDOUT",
	"ECONNREFUSED",
	"EHOSTDOWN",
	"EHOSTUNREACH",
	"ENOPROTOOPT",
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	"EUCLEAN",
	0,
	"ENOTNAM",
	"ENAVAIL",
	"EISNAM",
	"EREMOTEIO",
	"EINIT",
	"EREMDEV",
	0,
	0,
	"ENOTEMPTY",
};

/*
*	pferror(exitv, fmt, args...)
*
*	given a printf format and arguments, print them and an
*	error message similar to perror().
*	if (exitv != 0) then exit after printing with status exitv
*/
pferror(va_alist)
	va_dcl
{
	char *fmt, *s, *s2;
	int exitv;
	va_list args;
	extern char *sys_errlist[];
	extern int errno, sys_nerr;

	va_start(args);
	exitv = va_arg(args, int);
	fmt = va_arg(args, char *);
	vfprintf(stderr, fmt, args);
	if (*fmt)
		fprintf(stderr, ": ");
	if (errno < NERRN)
		s = sys_errn[errno];
	else
		s = "unknown error";
	s2 = (errno < sys_nerr) ? sys_errlist[errno] : 0;
	fprintf(stderr, "%s (%d%s%s)\n", s, errno, s2 ? " = " : "", s2);
	if (exitv)
		exit(exitv);
}

#ifdef STDALONE
main() {
	int d, cnt;
	char bf[10];

	d = 0;
	cnt = 10;
	close(d);
	read(d, bf, cnt);
	pferror(1, "read failed on desc %d for %d bytes", d, cnt);
}
#endif
