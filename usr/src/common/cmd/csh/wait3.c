/*		copyright	"%c%" 	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)csh:common/cmd/csh/wait3.c	1.2.7.1"
#ident  "$Header$"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
 *      Compatibility lib for BSD's wait3(). It is not
 *      binary compatible, since BSD's WNOHANG and WUNTRACED
 *      carry different #define values.
 *	
 *	This functions is renamed to wait9() due to inclusion of
 *	wait3 into global namespace. Namespace change was made for
 *	unix95 compliance.
 */

#include <errno.h>
#include <sys/time.h>
#include <sys/times.h>
#include <wait.h>
#include <sys/param.h>
#include "resource.h"

/*
 * Since sysV does not support rusage as in BSD, an approximate approach
 * is:
 *      ...
 *      call times
 *      call waitid
 *      if ( a child is found )
 *              call times again
 *              rusage ~= diff in the 2 times call
 *      ...
 * 
 */

wait9(status, options, rp)
        int	*status;
        int     options;
	struct	rusage	*rp;
 
{
        struct  tms     before_tms;
        struct  tms     after_tms;
        siginfo_t       info;
        int             error;

	if (rp)
		memset((void *)rp, 0, sizeof(struct rusage));
	memset((void *)&info, 0, sizeof(siginfo_t));
        if ( gettimes(&before_tms) < 0 ) 
                return (-1);            /* errno is set by gettimes() */

	/*
	 * BSD's wait3() only supports WNOHANG & WUNTRACED
	 */
	options |= (WNOHANG|WUNTRACED|WEXITED|WSTOPPED|WTRAPPED|WCONTINUED);
        error = waitid(P_ALL, 0, &info, options);
        if ( error == 0 ) {
                clock_t	diffu;  /* difference in usertime (ticks) */
                clock_t	diffs;  /* difference in systemtime (ticks) */

                if ( (options & WNOHANG) && (info.si_pid == 0) )
                        return (0);     /* no child found */

		if (rp) {
                	if ( gettimes(&after_tms) < 0 )
                        	return (-1);    /* errno set by gettimes() */
                	/*
                 	 * The system/user time is an approximation only !!!
                 	 */
                	diffu = after_tms.tms_cutime - before_tms.tms_cutime;
                	diffs = after_tms.tms_cstime - before_tms.tms_cstime;
                	rp->ru_utime.tv_sec = diffu/HZ;
			rp->ru_utime.tv_usec = ((diffu % HZ) * 1000000) / HZ;
                	rp->ru_stime.tv_sec = diffs/HZ;
			rp->ru_stime.tv_usec = ((diffs % HZ) * 1000000) / HZ;
		}
               	*status = wstat(info.si_code, info.si_status);
                return (info.si_pid);

        } else {
                return (-1);            /* error number is set by waitid() */
        }
}

/*
 * Convert the status code to old style wait status
 */
wstat(code, status)
        int     code;
        int     status;
{
        register stat = (status & 0377);

        switch (code) {
                case CLD_EXITED:
                        stat <<= 8;
                        break;
                case CLD_DUMPED:
                        stat |= WCOREFLG;
                        break;
                case CLD_KILLED:
                        break;
                case CLD_TRAPPED:
                case CLD_STOPPED:
                        stat <<= 8;
                        stat |= WSTOPFLG;
                        break;
		case CLD_CONTINUED:
                        stat = WCONTFLG;
                        break;
        }
        return (stat);
}
