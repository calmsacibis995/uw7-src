/*		copyright	"%c%" 	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)csh:common/cmd/csh/owait.h	1.1.2.4"
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
 * This file holds definitions relevent to the wait system call.
 * Some of the options here are available only through the ``wait3''
 * entry point; the old entry point with one argument has more fixed
 * semantics, never returning status of unstopped children, hanging until
 * a process terminates if any are outstanding, and never returns
 * detailed information about process resource utilization (<vtimes.h>).
 */

#ifndef _sys_wait_h
#define _sys_wait_h

/*
 * Structure of the information in the first word returned by both
 * wait and wait3.  If w_stopval==WSTOPPED, then the second structure
 * describes the information returned, else the first.  See WUNTRACED below.
 */
union wait	{
	int	w_status;		/* used in syscall */
	/*
	 * Terminated process status.
	 */
	struct {
#if defined(vax) || defined(i386)
		unsigned short	w_Termsig:7;	/* termination signal */
		unsigned short	w_Coredump:1;	/* core dump indicator */
		unsigned short	w_Retcode:8;	/* exit code if w_termsig==0 */
#else
		unsigned short	w_Fill1:16;	/* high 16 bits unused */
		unsigned short	w_Retcode:8;	/* exit code if w_termsig==0 */
		unsigned short	w_Coredump:1;	/* core dump indicator */
		unsigned short	w_Termsig:7;	/* termination signal */
#endif
	} w_T;
	/*
	 * Stopped process status.  Returned
	 * only for traced children unless requested
	 * with the WUNTRACED option bit.
	 */
	struct {
#if defined(vax) || defined(i386)
		unsigned short	w_Stopval:8;	/* == W_STOPPED if stopped */
		unsigned short	w_Stopsig:8;	/* signal that stopped us */
#else
		unsigned short	w_Fill2:16;	/* high 16 bits unused */
		unsigned short	w_Stopsig:8;	/* signal that stopped us */
		unsigned short	w_Stopval:8;	/* == W_STOPPED if stopped */
#endif
	} w_S;
};
#define	w_termsig	w_T.w_Termsig
#define w_coredump	w_T.w_Coredump
#define w_retcode	w_T.w_Retcode
#define w_stopval	w_S.w_Stopval
#define w_stopsig	w_S.w_Stopsig


#define	WSTOPPED	0177	/* value of s.stopval if process is stopped */
#define	WCONTFLG	0177777 /* value of w.w_status due to SIGCONT, sysV */

/*
 * Option bits for the second argument of wait3.  WNOHANG causes the
 * wait to not hang if there are no stopped or terminated processes, rather
 * returning an error indication in this case (pid==0).  WUNTRACED
 * indicates that the caller should receive status about untraced children
 * which stop due to signals.  If children are stopped and a wait without
 * this option is done, it is as though they were still running... nothing
 * about them is returned.
 */
#ifdef WNOHANG
#undef WNOHANG
#endif
#define WNOHANG		1	/* dont hang in wait */

#ifdef WUNTRACED
#undef WUNTRACED
#endif
#define WUNTRACED	2	/* tell about stopped, untraced children */

#ifdef WIFSTOPPED
#undef WIFSTOPPED
#endif
#define WIFSTOPPED(x)	((x).w_stopval == WSTOPPED)

#ifdef WIFSIGNALED
#undef WIFSIGNALED
#endif
#define WIFSIGNALED(x)	((x).w_stopval != WSTOPPED && (x).w_termsig != 0)

#ifdef WIFEXITED
#undef WIFEXITED
#endif
#define WIFEXITED(x)	((x).w_stopval != WSTOPPED && (x).w_termsig == 0)

#ifdef KERNEL
/* 
 * Arguments to wait4() system call, included here so it may be called by
 * other routines in the kernel 
 */
struct wait4_args {
	int pid;
	union wait *status;
	int options;
	struct rusage *rusage;
};
#endif KERNEL

#endif /*!_sys_wait_h*/
