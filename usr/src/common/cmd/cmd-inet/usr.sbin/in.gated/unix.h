#ident	"@(#)unix.h	1.5"
#ident	"$Header$"

/*
 * Public Release 3
 * 
 * $Id$
 */

/*
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1996, 1997 The Regents of the University of Michigan
 * All Rights Reserved
 *  
 * Royalty-free licenses to redistribute GateD Release
 * 3 in whole or in part may be obtained by writing to:
 * 
 * 	Merit GateDaemon Project
 * 	4251 Plymouth Road, Suite C
 * 	Ann Arbor, MI 48105
 *  
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS OF THE
 * UNIVERSITY OF MICHIGAN AND MERIT DO NOT WARRANT THAT THE
 * FUNCTIONS CONTAINED IN THE SOFTWARE WILL MEET LICENSEE'S REQUIREMENTS OR
 * THAT OPERATION WILL BE UNINTERRUPTED OR ERROR FREE. The Regents of the
 * University of Michigan and Merit shall not be liable for
 * any special, indirect, incidental or consequential damages with respect
 * to any claim by Licensee or any third party arising from use of the
 * software. GateDaemon was originated and developed through release 3.0
 * by Cornell University and its collaborators.
 * 
 * Please forward bug fixes, enhancements and questions to the
 * gated mailing list: gated-people@gated.merit.edu.
 * 
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1990,1991,1992,1993,1994,1995 by Cornell University.
 *     All rights reserved.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * GateD is based on Kirton's EGP, UC Berkeley's routing
 * daemon	 (routed), and DCN's HELLO routing Protocol.
 * Development of GateD has been supported in part by the
 * National Science Foundation.
 * 
 * ------------------------------------------------------------------------
 * 
 * Portions of this software may fall under the following
 * copyrights:
 * 
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms are
 * permitted provided that the above copyright notice and
 * this paragraph are duplicated in all such forms and that
 * any documentation, advertising materials, and other
 * materials related to such distribution and use
 * acknowledge that the software was developed by the
 * University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote
 * products derived from this software without specific
 * prior written permission.  THIS SOFTWARE IS PROVIDED
 * ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */


/* system function type declarations */

#ifdef	MALLOC_OK
PROTOTYPE(free,
	  extern void,
	  (caddr_t));
#endif	/* MALLOC_OK */
PROTOTYPE(send,
	  extern int,
	  (int s,
	   caddr_t,
	   int,
	   int));
PROTOTYPE(getpeername,
	  extern int,
	  (int,
	   struct sockaddr *,
	   int *));
PROTOTYPE(fcntl,
	  extern int,
	  (int,
	   int,
	   int));
PROTOTYPE(setsockopt,
	  extern int,
	  (int,
	   int,
	   int,
	   caddr_t,
	   int));
PROTOTYPE(close,
	  extern int,
	  (int));
PROTOTYPE(connect,
	  extern int,
	  (int,
	   struct sockaddr *,
	   int));
PROTOTYPE(accept,
	  extern int,
	  (int,
	   struct sockaddr *,
	   int *));
PROTOTYPE(bind, 
	  extern int,
	  (int,
	   struct sockaddr *,
	   int));
PROTOTYPE(listen,
	  extern int,
	  (int,
	   int));
PROTOTYPE(recv,
	  extern int,
	  (int,
	   caddr_t,
	   int,
	   int));
PROTOTYPE(qsort,
	  extern void,
	  (caddr_t,
	   u_int,
	   int,
	   _PROTOTYPE(compare,
		      int,
		      (const VOID_T,
		       const VOID_T))));
#ifdef	INCLUDE_TIME
PROTOTYPE(gettimeofday,
	  extern int,
	  (struct timeval *,
	   struct timezone *));
#endif
PROTOTYPE(sendto,
	  extern int,
	  (int,
	   caddr_t,
	   int,
	   int,
	   struct sockaddr *,
	   int));
PROTOTYPE(ioctl,
	  extern int,
	  (int,
	   unsigned long,
	   caddr_t));
#ifdef	INCLUDE_NLIST
PROTOTYPE(nlist,
	  extern int,
	  (const char *,
	   NLIST_T *));
#endif
PROTOTYPEV(open,
	  extern int,
	  (const char *,
	   int,
	   ...));
PROTOTYPE(read,
	  extern int,
	  (int,
	   caddr_t,
	   int));
PROTOTYPE(lseek,
	  extern off_t,
	  (int,
	   off_t,
	   int));
PROTOTYPE(gethostname,
	  extern int,
	  (char *,
	   int));
PROTOTYPE(fork,
	  extern int,
	  (void));
PROTOTYPE(exit,
	  extern void,
	  (int));
PROTOTYPE(getdtablesize,
	  extern int,
	  (void));
PROTOTYPE(getpid,
	  extern int,
	  (void));
PROTOTYPE(openlog,
	  extern void,
	  (const char *,
	   int,
	   int));
PROTOTYPE(setlogmask,
	  extern void,
	  (int));
PROTOTYPE(srandom,
	  extern void,
	  (int));
PROTOTYPE(random,
	  extern long,
	  (void));
PROTOTYPE(chdir,
	  extern int,
	  (const char *));
PROTOTYPE(abort,
	  extern void,
	  (void));
PROTOTYPE(fputs,
	  extern int,
	  (const char *,
	   FILE *));
PROTOTYPE(kill,
	  extern int,
	  (int,
	   int));
#ifdef	INCLUDE_TIME
PROTOTYPE(setitimer,
	  extern int,
	  (int,
	   struct itimerval *,
	   struct itimerval *));
#endif
PROTOTYPE(recvmsg,
	  extern int,
	  (int,
	   struct msghdr *,
	   int));
#ifdef	INCLUDE_TIME
PROTOTYPE(select,
	  extern int,
	  (int,
	   fd_set *,
	   fd_set *,
	   fd_set *,
	   struct timeval *));
#endif
PROTOTYPE(sigblock,
	  extern int,
	  (int));
PROTOTYPE(sigsetmask,
	  extern int,
	  (int));
#ifdef	INCLUDE_WAIT
PROTOTYPE(wait3,
	  extern int,
	  (union wait *,
	   int,
	   caddr_t /* XXX */));
#endif
PROTOTYPE(sigvec,
	  extern int,
	  (int,
	   struct sigvec *,
	   struct sigvec *));
PROTOTYPE(socket,
	  extern int,
	  (int,
	   int,
	   int));
PROTOTYPE(sleep,
	  extern void,
	  (unsigned));
PROTOTYPE(fclose,
	  extern int,
	  (FILE *));
#ifdef	INCLUDE_STAT
PROTOTYPE(stat,
	  extern int,
	  (const char *,
	   struct stat *));
#endif
PROTOTYPE(setlinebuf,
	  extern int,
	  (FILE *));
PROTOTYPE(setbuf,
	  extern void,
	  (FILE *,
	   caddr_t));
PROTOTYPE(setvbuf,
	  extern int,
	  (FILE *,
	   caddr_t,
	   int,
	   size_t));
PROTOTYPE(fflush,
	  extern int,
	  (FILE *));
PROTOTYPE(fputc,
	  extern int,
	  (char c,
	   FILE *));
PROTOTYPEV(syslog,
	   extern int,
	   (int,
	    const char *,
	    ...));
#ifdef	MALLOC_OK
PROTOTYPE(calloc,
	  extern caddr_t,
	  (unsigned,
	   size_t));
PROTOTYPE(malloc,
	  extern caddr_t,
	  (size_t));
#endif	/* MALLOC_OK */
#ifdef	notdef
PROTOTYPE(alloca,
	  extern caddr_t,
	  (int));
#endif
PROTOTYPE(atoi,
	  extern int,
	  (const char *));
#ifdef	notdef
PROTOTYPE(index,
	  extern char *,
	  (char *,
	   char));
PROTOTYPE(rindex,
	  extern char *,
	  (char *,
	   char));
#endif
PROTOTYPE(getwd,
	  extern char *,
	  (char *));
PROTOTYPE(sethostent,
	  extern,
	  (int));
PROTOTYPE(endhostent,
	  extern,
	  (void));
PROTOTYPE(endnetent,
	  extern,
	  (void));
PROTOTYPE(setnetent,
	  extern,
	  (int));
PROTOTYPE(getenv,
	  extern char *,
	  (const char *));
