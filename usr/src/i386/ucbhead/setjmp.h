/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)ucb:i386/ucbhead/setjmp.h	1.3"
#ident	"$Header$"

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
 * 4.3BSD setjmp compatibility header
 *
 * 4.3BSD setjmp/longjmp is equivalent to SVR4 sigsetjmp/siglongjmp -
 * 4.3BSD _setjmp/_longjmp is equivalent to SVR4 setjmp/longjmp
 */

#ifndef _SETJMP_H
#define _SETJMP_H

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/siginfo.h>
#include <sys/tss.h>
#include <sys/ucontext.h>

#define _JBLEN		((sizeof(ucontext_t)+sizeof(int)-1)/sizeof(int))
#define _SIGJBLEN	_JBLEN

typedef int jmp_buf[_JBLEN];

#define sigjmp_buf 	jmp_buf

#define	_setjmp(jbuf) 		sigsetjmp(jbuf, (int)0)
#define _longjmp(jbuf, val)	siglongjmp(jbuf, val)
#define setjmp(jbuf)		sigsetjmp(jbuf, (int)1) 	
#define longjmp(jbuf, val)	siglongjmp(jbuf, val)

#if defined(__STDC__)

#if __STDC__ == 0	/* non-ANSI standard compilation */
extern int	sigsetjmp(sigjmp_buf, int);
extern void	siglongjmp(sigjmp_buf, int);

#else
extern int	sigsetjmp();
extern void	siglongjmp();
#endif  /* __STDC__ */

#endif

#endif  /* _SETJMP_H */
