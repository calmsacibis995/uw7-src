#ident	"@(#)OSRcmds:ksh/include/sh_config.h	1.1"
#pragma comment(exestr, "@(#) sh_config.h 25.4 94/10/24 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1994.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	Modification History
 *
 *	L000	scol!markhe	4 Nov 92
 *	- generated originally by using install/config then hand modified
 *	  with the following changes (from scol!blf and scol!hughd):
 *		o  define DEVFD - uses /dev/fd but fails tolerably
 *		   if not installed
 *		o  invented and #define'd HAS_EACCESS:  SCO UNIX has
 *		   an eaccess system call
 *		o  renamed TIC_SEC to HZ and define Hz as extern (actually
 *		   declared in sh/defs.h)
 *		o  removed SUID_EXEC, suid shell scripts are insecure
 *	- added VPIX for dos support
 *	- turn on C-Shell {...,...} expansions (BRACEPAT)
 *	- added cast to killpg().  A unary minus promotes a short to an
 *	  integer giving a long/short mismatch warning during compliation
 *	  of sh/jobs.c
 *	- NOTE: when the compiler correctly handles `const char *name[]' then
 *		the `#define const ' line should be removed
 *
 *	L001	scol!markhe	12 Nov 92
 *	- turned on POSIX
 *	L002	scol!anthonys	24 Aug 94
 *	- enable full SVR4 style ulimit functionality.
 *	L003	scol!ianw	24 Oct 94
 *	- Removed killpg() #define, unnecessary as it is now in libc.a.
 *	- Added NOBUF #define.
 */
#ifndef _sh_config_
#define	_sh_config_	1
/*
 * This has been generated from install/config
 * The information is based on the compile time environment.
 * It may be necessary to change values of some parameters for cross
 *  development environments.
 */

#include	<sys/types.h>

extern	int	Hz;		/* value from gethz() */	/* L000 */

#define _sys_acct_	1
#define _dirent_ 1
#define _sys_dirent_	1
#define _execargs_ 1
#define _fcntl_ 1
#define _sys_fcntl_	1
#define _sys_file_	1
#define _sys_ioctl_	1
#define _locale_ 1
#define _mnttab_ 1
#define _sys_resource_	1					/* L002 */
#define _sgtty_ 1
#define _sys_times_	1
#define _termio_ 1
#define _sys_termio_	1
#define _termios_ 1
#define _sys_termios_	1
#define _sys_wait_	1
#define _unistd_ 1
#define _sys_unistd_	1
#define _sys_utsname_	1
#define _bin_grep_	1
#define _usr_bin_lpr_	1
#define _usr_bin_lp_	1
#define _usr_bin_vi_	1
#define _bin_newgrp_	1
#define _poll_	1
#define const /* empty */
#define VOID	void
#define _sys_timeb_	1
#undef _sys_file_
#define SIG_NORESTART	1
#define signal	sigset
extern VOID (*sigset())();
#define sig_begin()
#define sigrelease(s)
#define getpgid(a)	getpgrp()
#define NFILE	32
#define sh_rand(x) ((x),rand())
#define PIPE_ERR	29
#define PROTO	1
#define SETJMP	setjmp
#define LONGJMP	longjmp
#define MULTIGROUPS	0
#define SHELLMAGIC	1
#define PATH_MAX	1024
#define HZ	100						/* L000 */
#define NOBUF	1						/* L003 */
#define LSTAT	1
#define SYSCALL	1
#define _sys_Time_
#define MNTTAB	"/etc/mnttab"
#define ECHO_N	1
#define JOBS	1
#define WINSIZE	1
#define _sys_ptem_	1
#define _sys_stream_	1
#define _SELECT5_	1
#define	VPIX		1					/* L000 */
#define	BRACEPAT	1					/* L000 */
#define	DEVFD		1					/* L000 */
#define	POSIX		1					/* L001 */
#define ESH	1
#define JOBS	1
#define NEWTEST	1
#define OLDTEST	1
#define VSH	1
#define	HAS_EACCESS	1		/* eaccess exists	   L000 */
#endif
