#ident	"@(#)debugger:libedit/common/sh_config.h	1.5"
#ifndef _sh_config_
#define _sh_config_     1
/*
 * This has been generated from install/config
 * The information is based on the compile time environment.
 * It may be necessary to change values of some parameters for cross
 *  development environments.
 */

#include        <sys/types.h>

#define _sys_acct_      1
#define _dirent_ 1
#define _sys_dirent_    1
#define _fcntl_ 1
#define _sys_fcntl_     1
#define _sys_file_      1
#define _sys_filio_     1
#define _sys_ioctl_     1
#define _locale_ 1
#define _mnttab_ 1
#define _sys_mnttab_    1
#define _poll_ 1
#define _sys_poll_      1
#define _sys_ptem_      1
#define _sys_stream_    1
#define _sgtty_ 1
#define _time_ 1
#define _sys_time_      1
#define _sys_times_     1
#define _termio_ 1
#define _sys_termio_    1
#define _sys_termios_   1
#define _wait_ 1
#define _sys_wait_      1
#define _unistd_ 1
#define _sys_unistd_ 1
#define _sys_utsname_   1
#define _bin_grep_      1
#define _usr_bin_lp_    1
#define _usr_bin_vi_    1
#define _bin_newgrp_    1
#define _sys_resource_  1

#ifdef XT_SUPPORTED
#define _sys_jioctl_    1
#endif

#ifdef __STDC__
#define VOID    void
#define PROTO 1
#else
#ifdef PROTO
#undef PROTO
#endif
#define VOID	char
#endif
#define SIG_NORESTART   1
/* extern VOID (*sigset())(); */
#undef _sys_file_
#define signal  sigset
#define sig_begin()
#define sigrelease      sigrelse
#define NFILE   32
#define sh_rand() rand()
#define PIPE_ERR        29
#define SHELLMAGIC      1
#define PATH_MAX        1024
#define HZ      100             /* 100 ticks/second of the clock */
#define	FIONREAD        _IOR('f', 127, int)	/* get # bytes to read */
#define LSTAT   1
#define SYSCALL 1
#define DEVFD   1
#define MNTTAB  "/etc/mnttab"
#define JOBS    1
#define SELECT5 1
#define ESH     1
#define JOBS    1
#define NEWTEST 1
#define OLDTEST 1
#define VSH     1

#ifndef GEMINI_ON_OSR5
#define OLDTERMIO 1
#else
#undef MULTIBYTE
#endif

#endif
