#ident	"@(#)libc-i386:gen/tcsendbreak.c	1.1"

#ifdef __STDC__
	#pragma weak tcsendbreak = _tcsendbreak
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <sys/termios.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

/* 
 * send zeros for 0.25 seconds, if duration is 0
 * If duration is not 0, ioctl(fildes, TCSBRK, 0) is used also to
 * make sure that a break is sent. This is for POSIX compliance.
 */

/* This is not quite right as originally written.  The problem is 
 * that in the asy driver, the DDI/DKI timeout() routine is called.
 * timeout() does not delay -- it returns immediately to the caller,
 * Hence, this routine, which PCTS and the FIPS tests assumes will 
 * require 0.25 seconds to return, returns immediately.  Of course, 
 * when the next call occurs (PCTS makes 40 calls, back to back), the
 * asy driver is still in the process of sending zeroes.  So, some 
 * breaks get dropped on the floor, and the PCTS tests only see about
 * 18 of the 40 breaks.  Timing tests with this routine show that 
 * the ioctl takes less than a clock tick.  So, the solution is to 
 * use the setitimer(3C) call to sleep for the 0.25 second.
 * A better formed solution will swipe code from sleep(3C).
 */

asm
int
msec_sleep(long period)
{
%con	period;
	pushl 	period
	call 	mynap
	popl	%ecx
	jmp	end
mynap:
	movl	$3112,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
end:
}


/*ARGSUSED*/
int tcsendbreak (fildes, duration)
int fildes;
int duration;
{
	int ret;
	int saved_errno;	

	ret=ioctl(fildes,TCSBRK,0);
	saved_errno=errno;
	/* According to documentation, the only reason for 
	 * nap(2) to return -1 is if the sleep is interrupted 
	 * by a signal.  Just to be safe, check that the errno
	 * is really EINTR.
	 */
	if (msec_sleep(250) == -1){
		if (errno!=EINTR)	
			return(-1);
	}
	errno=saved_errno;
	return(ret);
}

