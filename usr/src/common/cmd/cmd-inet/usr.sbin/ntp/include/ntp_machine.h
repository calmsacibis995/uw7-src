#ident "@(#)ntp_machine.h	1.2"

/*
 * Collect all machine dependent idiosyncrasies in one place.
 */

#ifndef __ntp_machine
#define __ntp_machine

/*
    Various options.
    They can defined with the DEFS directive in the Config file if they
    are not defined here.
 
WHICH NICE

  HAVE_ATT_NICE     - Use att nice(priority_change)
  HAVE_BSD_NICE     - Use bsd setprioirty(which, who, priority)
  HAVE_NO_NICE      - Don't have (or use) either

KERNEL MUCKING - If you porting to a new system see xntpd/ntp_unixclock.c and
		 util/tickadj.c to see what these do. This is very system 
	         dependent stuff!!!
		 
  HAVE_LIBKVM       - Use libkvm to read kernal memory
  HAVE_READKMEM     - Use read to read kernal memory 
  NOKMEM	    - Don't read kmem
  HAVE_N_UN         - Have u_nn nlist struct.

WHICH SETPGRP TO USE - Not needed if NTP_POSIX_SOURCE is defined since you
		       better of setsid!

  HAVE_ATT_SETPGRP  - setpgrp(void) instead of setpgrp(int, int)


Signaled IO -  Signled IO defines. 

  HAVE_SIGNALED_IO  - Enable signaled io. Assumes you are going to use SIGIO
		      for tty and udp io.
  USE_UDP_SIGPOLL   - Use SIGPOLL on socket io. This assumes that the
		      sockets routines are defined on top of streams.
  USE_TTY_SIGPOLL   - Use SIGPOLL on tty io. This assumes streams.
  USE_FSETOWNCTTY   - some systems require the terminal being a CTTY
		      for the F_SETOWN fcntl.
  UDP_BACKWARDS_SETOWN - SunOS 3.5 or Ultirx 2.0 system.
	      

WHICH TERMINAL MODEL TO USE - I would assume HAVE_TERMIOS if 
		      NTP_POSIX_SOURCE was set but can't.  The 
		      posix tty driver is too restrictive on most systems.
		      It is defined if you define STREAMS.

  We do not put these defines in the ntp_machine.h as some systems
  offer multiple interfaces and refclock configuration likes to
  peek into the configuration defines for tty model restrictions.
  Thus all tty definitions should be in the files in the machines directory.

  HAVE_TERMIOS      - Use POSIX termios.h
  HAVE_SYSV_TTYS    - Use SYSV termio.h
  HAVE_BSD_TTYS     - Use BSD stty.h

THIS MAKES PORTS TO NEW SYSTEMS EASY - You only have to wory about
		                       kernel mucking.

  NTP_POSIX_SOURCE  - Use POSIX functions over bsd functions and att functions.
		      This is NOT the same as _POSIX_SOURCE.
		      It is much weaker!


STEP SLEW OR TWO STEP - The Default is to step.

  SLEWALWAYS  	    - setttimeofday can not be used to set the time of day at 
		      all. 
  STEP_SLEW 	    - setttimeofday can not set the seconds part of time
		      time use setttimeofday to set the seconds part of the
		      time and the slew the seconds.
  FORCE_NTPDATE_STEP - even if SLEWALWAYS is defined, force a step of
		      of the systemtime (via settimeofday()). Only takes
		      affect if STEP_SLEW isn't defined.

WHICH TIMEOFDAY()

  SYSV_TIMEOFDAY    - [sg]ettimeofday(struct timeval *) as opposed to BSD
                      [sg]ettimeofday(struct timeval *, struct timezone *)

INFO ON NEW KERNEL PLL SYS CALLS

  NTP_SYSCALLS_STD  - use the "normal" ones
  NTP_SYSCALL_GET   - SYS_ntp_gettime id
  NTP_SYSCALL_ADJ   - SYS_ntp_adjtime id
  NTP_SYSCALLS_LIBC - ntp_adjtime() and ntp_gettime() are in libc.

HOW TO GET IP INTERFACE INFORMATION

  Some UNIX V.4 machines implement a sockets library on top of
  streams. For these systems, you must use send the SIOCGIFCONF down
  the stream in an I_STR ioctl. This ususally also implies
  USE_STREAMS_DEVICE FOR IF_CONFIG. Dell UNIX is a notable exception.

  STREAMS_TLI - use ioctl(I_STR) to implement ioctl(SIOCGIFCONF)

WHAT DOES IOCTL(SIOCGIFCONF) RETURN IN THE BUFFER

  UNIX V.4 machines implement a sockets library on top of streams.
  When requesting the IP interface configuration with an ioctl(2) calll,
  an array of ifreq structures are placed in the provided buffer.  Some
  implementations also place the length of the buffer information in
  the first integer position of the buffer.  
  
  SIZE_RETURNED_IN_BUFFER - size integer is in the buffer

WILL IOCTL(SIOCGIFCONF) WORK ON A SOCKET

  Some UNIX V.4 machines do not appear to support ioctl() requests for the
  IP interface configuration on a socket.  They appear to require the use
  of the streams device instead.

  USE_STREAMS_DEVICE_FOR_IF_CONFIG - use the /dev/ip device for configuration

MISC  

  USE_PROTOTYPES    - Prototype functions
  DOSYNCTODR        - Resync TODR clock  every hour.
  RETSIGTYPE        - Define signal function type.
  NO_SIGNED_CHAR_DECL - No "signed char" see include/ntp.h
  LOCK_PROCESS      - Have plock.
  UDP_WILDCARD_DELIVERY
		    - these systems deliver broadcast packets to the wildcard
		      port instead to a port bound to the interface bound
		      to the correct broadcast address - are these
		      implementations broken or did the spec change ?

DEFINITIONS FOR SYSTEM && PROCESSOR
  STR_SYSTEM        - value of system variable
  STR_PROCESSOR     - value of processor variable

You could just put the defines on the DEFS line in machines/<os> file.
I don't since there are lots of different types of compilers that a system might
have, some that can do proto typing and others that cannot on the same system.
I get a chance to twiddle some of the configuration parameters at compile
time based on compiler/machine combinations by using this include file.
See convex, aix and sun configurations see how complex it get.
  
Note that it _is_ considered reasonable to add some system-specific defines
to the machine/<os> file if it would be too inconvenient to puzzle them out
in this file.
  
*/

/*
 * (Univel/Novell) Unixware2 SVR4 on intel x86 processor
 */
#if defined(SYS_UNIXWARE2)
#undef HAVE_ATT_SETPGRP
#define HAVE_ATT_NICE
#define USE_PROTOTYPES
#define NTP_POSIX_SOURCE
#define HAVE_ATT_NICE
#define HAVE_READKMEM 
#define USE_TTY_SIGPOLL
#define USE_UDP_SIGPOLL
#define UDP_WILDCARD_DELIVERY
#undef SIZE_RETURNED_IN_BUFFER
#undef STREAMS_TLI
#undef USE_STREAMS_DEVICE_FOR_IF_CONFIG
#undef HAVE_SIGNALED_IO
#define STREAM
#define STREAMS
#undef STEP_SLEW 		/* TWO step */
#define LOCK_PROCESS
#define NO_SIGNED_CHAR_DECL 
#undef SYSV_TIMEOFDAY
#define	RETSIGTYPE void
#include <sys/sockio.h>
#include <sys/types.h>
#ifndef STR_SYSTEM
#define STR_SYSTEM "UNIX/Unixware2"
#endif
#endif

#ifdef STREAM			/* STREAM implies TERMIOS */
#ifndef HAVE_TERMIOS
#define HAVE_TERMIOS
#endif
#endif

#ifndef	RETSIGTYPE
#if defined(NTP_POSIX_SOURCE)
#define	RETSIGTYPE	void
#else
#define	RETSIGTYPE	int
#endif
#endif

#ifdef	NTP_SYSCALLS_STD
#ifndef	NTP_SYSCALL_GET
#define	NTP_SYSCALL_GET	235
#endif
#ifndef	NTP_SYSCALL_ADJ
#define	NTP_SYSCALL_ADJ	236
#endif
#endif	/* NTP_SYSCALLS_STD */

#if	!defined(HAVE_ATT_NICE) \
	&& !defined(HAVE_BSD_NICE) \
	&& !defined(HAVE_NO_NICE) \
	&& !defined(SYS_WINNT)
	ERROR You_must_define_one_of_the_HAVE_xx_NICE_defines
#endif

/*
 * use only one tty model - no use in initialising
 * a tty in three ways
 * HAVE_TERMIOS is preferred over HAVE_SYSV_TTYS over HAVE_BSD_TTYS
 */
#ifdef HAVE_TERMIOS
#undef HAVE_BSD_TTYS
#undef HAVE_SYSV_TTYS
#endif

#ifdef HAVE_SYSV_TTYS
#undef HAVE_BSD_TTYS
#endif

#if !defined(SYS_WINNT) && !defined(VMS)
#if	!defined(HAVE_SYSV_TTYS) \
	&& !defined(HAVE_BSD_TTYS) \
	&& !defined(HAVE_TERMIOS)
   	ERROR no_tty_type_defined
#endif
#endif /* SYS_WINNT || VMS */


#if !defined(XNTP_BIG_ENDIAN) && !defined(XNTP_LITTLE_ENDIAN)

# if defined(XNTP_AUTO_ENDIAN)
#  include <netinet/in.h>

#  if BYTE_ORDER == BIG_ENDIAN
#   define XNTP_BIG_ENDIAN
#  endif
#  if BYTE_ORDER == LITTLE_ENDIAN
#   define XNTP_LITTLE_ENDIAN
#  endif

# else	/* AUTO */

#  ifdef	WORDS_BIGENDIAN
#   define	XNTP_BIG_ENDIAN	1
#  else
#   define	XNTP_LITTLE_ENDIAN	1
#  endif

# endif	/* AUTO */

#endif	/* !BIG && !LITTLE */

/*
 * Byte order woes.  The DES code is sensitive to byte order.  This
 * used to be resolved by calling ntohl() and htonl() to swap things
 * around, but this turned out to be quite costly on Vaxes where those
 * things are actual functions.  The code now straightens out byte
 * order troubles on its own, with no performance penalty for little
 * end first machines, but at great expense to cleanliness.
 */
#if !defined(XNTP_BIG_ENDIAN) && !defined(XNTP_LITTLE_ENDIAN)
	/*
	 * Pick one or the other.
	 */
	BYTE_ORDER_NOT_DEFINED_FOR_AUTHENTICATION
#endif

#if defined(XNTP_BIG_ENDIAN) && defined(XNTP_LITTLE_ENDIAN)
	/*
	 * Pick one or the other.
	 */
	BYTE_ORDER_NOT_DEFINED_FOR_AUTHENTICATION
#endif


#endif /* __ntp_machine */
