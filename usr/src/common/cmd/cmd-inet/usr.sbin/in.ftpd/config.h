#ident	"@(#)config.h	1.6"

/*
 * Configuration file for UnixWare
 * $Id$
 */
#define HAVE_SYMLINK
#undef  F_SETOWN
#define HAVE_REGEX_H
#define HAVE_DIRENT
#define HAVE_D_NAMLEN
#undef  HAVE_FLOCK
#define HAVE_FTW
#define HAVE_GETCWD
#define HAVE_GETRLIMIT
#undef  HAVE_PSTAT
#define HAVE_STATVFS
#define HAVE_ST_BLKSIZE
#define HAVE_SYSINFO
#define HAVE_SYSCONF
#undef  HAVE_UT_UT_HOST
#define HAVE_VPRINTF
#define L_INCR	SEEK_CUR
#define REGEXEC
#ifndef UXW
#define SPT_TYPE SPT_NONE
#endif /* UXW */
#define SHADOW_PASSWORD
#define SVR4
#define USG
#define UNIXWARE
#define USE_VAR
#define USE_ETC

#ifdef UXW
#if SPT_TYPE == SPT_SCO
#define _KMEMUSER
#define MERGE386
#define BUG386B1
#define _PATH_KMEM	"/dev/kmem"
#endif /* SPT_SCO */
#endif /* UXW */

#include <limits.h>

#ifndef NCARGS
#ifdef _POSIX_ARG_MAX
#define NCARGS _POSIX_ARG_MAX
#else
#ifdef ARG_MAX
#define NCARGS ARG_MAX
#endif
#endif
#endif

#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef UXW
/*
 * ensure _PATH_WTMP is consistent with WTMPX_FILE in logwtmp.c
 */
#define _PATH_WTMP	WTMP_FILE

/*
 * define here to avoid the need for sigfix.c
 */
#define delay_signaling()
#define enable_signaling()

/*
 * define here rather than changing source files to deal with privileges
 */
#include "../security.h"
#define execv(arg1, arg2)	CLR_MAXPRIVS_FOR_EXEC execv(arg1, arg2)
#define seteuid(arg)		CLR_WORKPRIVS_NON_ADMIN(seteuid(arg))

/*
 * needed to support long passwords
 */
#define crypt(k,s)	bigcrypt(k,s)

/*
 * work around problems in libsocket caused by calling vfork() followed by
 * the child calling close()
 */
#define vfork	fork
#endif /* UXW */

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif  

#ifndef FACILITY
#define FACILITY LOG_DAEMON
#endif

typedef void	SIGNAL_TYPE;

/* 
 * Top level config file... you'll probably not need to modify any of this.
 * $Id$
 * In the future, a lot more definable features will be here (and this
 * will all make sense...)
 */

/*
 * allow "upload" keyword in ftpaccess
 */

#define UPLOAD

/*
 * allow "overwrite" keyword in ftpaccess.
 */

#define OVERWRITE

/*
 * allow "allow/deny" for individual users.
 */

#define HOST_ACCESS

/*
 * log failed login attempts
 */

#define LOG_FAILED

/*
 * log login attempts that fail because of class connection
 * limits.  Busy servers may want to prevent this logging
 * since it can fill up the log file and put a high load on
 * syslog.
 */
#define LOG_TOOMANY

/*
 * allow use of private file.  (for site group and site gpass)
 */

#undef NO_PRIVATE

/*
 * Try once more on failed DNS lookups (to allow far away connections 
 * which might resolve slowly)
 */

#define	DNS_TRYAGAIN

/*
 * ANON_ONLY 
 * Permit only anonymous logins... disables all other type
 */

#undef ANON_ONLY

/*
 * PARANOID
 * Disable "questionable" functions
 */

#undef PARANOID

/*
 * SKEY
 * Add SKEY support -- REQUIRES SKEY libraries
 */

#undef SKEY

#define realpath realpath_on_steroids   /* hack to work around unistd.h */
