#ident	"@(#)ihvkit:display/include/os.h	1.1"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/* $Header$ */
/* $XConsortium: os.h,v 1.44 91/07/18 23:01:12 keith Exp $ */

#ifndef OS_H
#define OS_H
#include "misc.h"

#ifdef INCLUDE_ALLOCA_H
#include <alloca.h>
#endif

#define NullFID ((FID) 0)

#define SCREEN_SAVER_ON   0
#define SCREEN_SAVER_OFF  1
#define SCREEN_SAVER_FORCER 2

#ifndef MAX_REQUEST_SIZE
#define MAX_REQUEST_SIZE 65535
#endif

typedef pointer	FID;
typedef struct _FontPathRec *FontPathPtr;
typedef struct _NewClientRec *NewClientPtr;

extern unsigned long *Xalloc(), *Xrealloc();
extern void Xfree();

#ifndef NO_ALLOCA
/*
 * os-dependent definition of local allocation and deallocation
 * If you want something other than Xalloc/Xfree for ALLOCATE/DEALLOCATE
 * LOCAL then you add that in here.
 */
#ifdef __HIGHC__

extern char *alloca();

#if HCVERSION < 21003
#define ALLOCATE_LOCAL(size)	alloca((int)(size))
#pragma on(alloca);
#else /* HCVERSION >= 21003 */
#define	ALLOCATE_LOCAL(size)	_Alloca((int)(size))
#endif /* HCVERSION < 21003 */

#define DEALLOCATE_LOCAL(ptr)  /* as nothing */

#endif /* defined(__HIGHC__) */


#ifdef __GNUC__
#define alloca __builtin_alloca
#endif

/*
 * warning: old mips alloca (pre 2.10) is unusable, new one is builtin
 * Test is easy, the new one is named __builtin_alloca and comes
 * from alloca.h which #defines alloca.
 */
#if defined(vax) || defined(sun) || defined(apollo) || defined(stellar) || defined(alloca)
/*
 * Some System V boxes extract alloca.o from /lib/libPW.a; if you
 * decide that you don't want to use alloca, you might want to fix 
 * ../os/4.2bsd/Imakefile
 */
#ifndef alloca
char *alloca();
#endif
#define ALLOCATE_LOCAL(size) alloca((int)(size))
#define DEALLOCATE_LOCAL(ptr)  /* as nothing */
#endif /* who does alloca */

#endif /* NO_ALLOCA */

#if defined(CAHILL_MALLOC) || defined(DEBUG_MALLOC)
#define Xalloc(len)		debug_Xalloc(__FILE__,__LINE__,(len))
#define Xcalloc(len)		debug_Xcalloc(__FILE__,__LINE__,(len))
#define Xrealloc(ptr,len)	debug_Xrealloc(__FILE__,__LINE__,(ptr),(len))
#define Xfree(ptr)		debug_Xfree(__FILE__,__LINE__,(ptr))
#define Xstrdup(ptr)		debug_Xstrdup(__FILE__,__LINE__,(ptr))
#endif

#ifndef ALLOCATE_LOCAL
#define ALLOCATE_LOCAL(size) Xalloc((unsigned long)(size))
#define DEALLOCATE_LOCAL(ptr) Xfree((pointer)(ptr))
#endif /* ALLOCATE_LOCAL */


#define xalloc(size) Xalloc(((unsigned long)(size)))
#define xrealloc(ptr, size) Xrealloc(((pointer)(ptr)), ((unsigned long)(size)))
#define xfree(ptr) Xfree(((pointer)(ptr)))
#define xstrdup(ptr) Xstrdup(((pointer)(ptr)))

#ifndef X_NOT_STDC_ENV
#include <string.h>
#else
#ifdef SYSV
#include <string.h>
#else
#include <strings.h>
#endif
#endif

extern int		ReadRequestFromClient();
extern void		CloseDownConnection();
extern FontPathPtr	ExpandFontNamePattern();
extern FID		FiOpenForRead();
extern void		CreateWellKnownSockets();
extern int		SetDefaultFontPath();
extern void		FreeFontRecord();
extern int		SetFontPath();
extern void		ErrorF();
extern void		Error();
extern void		FatalError();
extern void		ProcessCommandLine();
extern void		FlushAllOutput();
extern void		FlushIfCriticalOutputPending();
#if defined(CAHILL_MALLOC) || defined(DEBUG_MALLOC)
extern void		debug_Xfree();
extern unsigned long	*debug_Xalloc();
extern unsigned long	*debug_Xcalloc();
extern unsigned long	*debug_Xrealloc();
extern char		*debug_Xstrdup();
#else
extern void		Xfree();
extern unsigned long	*Xalloc();
extern unsigned long	*Xcalloc();
extern unsigned long	*Xrealloc();
extern char 		*Xstrdup();
#endif
extern long		GetTimeInMillis();

#endif /* OS_H */

