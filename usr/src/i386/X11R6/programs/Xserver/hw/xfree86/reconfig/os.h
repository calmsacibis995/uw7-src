/* $XFree86: xc/programs/Xserver/hw/xfree86/reconfig/os.h,v 3.3 1995/01/28 16:07:32 dawes Exp $ */





/* $XConsortium: os.h /main/3 1995/11/13 06:28:56 kaleb $ */

/* from <X11/Xosdefs.h> */
#ifdef NOSTDHDRS
#define X_NOT_STDC_ENV
#endif
#if defined(USL) && defined(SYSV)
#define X_NOT_STDC_ENV
#endif
#if defined(SYSV) && defined(i386)
#define X_NOT_STDC_ENV
#endif

/* from <X11/Xlibint.h> */
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#include <string.h>
#else
char *malloc(), *realloc(), *calloc();
#ifdef SYSV
#include <string.h>
#else
#include <strings.h>
#endif
#endif

#if defined(MACH) || defined(__bsdi__)
#include <ctype.h>
#endif
