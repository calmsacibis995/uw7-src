/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86_OSlib.h,v 3.21 1996/01/10 05:39:47 dawes Exp $ */
/*
 * Copyright 1990, 1991 by Thomas Roell, Dinkelscherben, Germany
 * Copyright 1992 by David Dawes <dawes@XFree86.org>
 * Copyright 1992 by Jim Tsillas <jtsilla@damon.ccs.northeastern.edu>
 * Copyright 1992 by Rich Murphey <Rich@Rice.edu>
 * Copyright 1992 by Robert Baron <Robert.Baron@ernst.mach.cs.cmu.edu>
 * Copyright 1992 by Orest Zborowski <obz@eskimo.com>
 * Copyright 1993 by Vrije Universiteit, The Netherlands
 * Copyright 1993 by David Wexelblat <dwex@XFree86.org>
 * Copyright 1994 by Holger Veit <veit@gmd.de>
 * Copyright 1994, 1995 by The XFree86 Project, Inc
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the above listed copyright holders 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  The above listed
 * copyright holders make no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THE ABOVE LISTED COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDERS BE 
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY 
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER 
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: xf86_OSlib.h /main/13 1996/01/10 11:12:07 kaleb $ */

#ifndef _XF86_OSLIB_H
#define _XF86_OSLIB_H

#include <X11/Xos.h>
#include <X11/Xfuncproto.h>

#ifdef __EMX__
#define NO_INLINE
#endif
#include "compiler.h"

#if defined(MACH386) || defined(__OSF__)
# undef NULL
#endif /* MACH386 || __OSF__ */

#include <stdio.h>
#include <ctype.h>

/**************************************************************************/
/* SYSV386 (SVR3, SVR4)                                                   */
/**************************************************************************/
#if defined(SYSV) || defined(SVR4)
# if defined(sun) && defined(i386) && defined(SVR4)
#  /* Fix for Solaris ANSI compilation */
#  define __EXTENSIONS__
# endif
# include <sys/ioctl.h>
# include <signal.h>
# include <termio.h>
# include <sys/stat.h>
# include <sys/types.h>
# if defined(SCO) || defined(ISC)
# include <sys/param.h>
# endif

# include <errno.h>

# if defined(_NEED_SYSI86)
#  include <sys/immu.h>
#  if !(defined (sun) && defined (i386) && defined (SVR4))
#    include <sys/region.h>
#  endif
#  include <sys/proc.h>
#  include <sys/tss.h>
#  include <sys/sysi86.h>
#  if defined(SVR4) && !defined(sun)
#   include <sys/seg.h>
#  endif /* SVR4 && !sun */
#  include <sys/v86.h>
#  if defined(sun) && defined (i386) && defined (SVR4)
#    include <sys/psw.h>
#  endif
# endif /* _NEED_SYSI86 */

#if defined(HAS_SVR3_MMAPDRV)
# include <sys/sysmacros.h>
# if !defined(_NEED_SYSI86)
#  include <sys/immu.h>
#  include <sys/region.h>
# endif
# include <sys/mmap.h>		/* MMAP driver header */
#endif

# define HAS_USL_VTS
# if !defined(sun)
#  include <sys/emap.h>
# endif
# if defined(SCO)
#  include <sys/vtkd.h>
#  include <sys/console.h>
#  include <sys/keyboard.h>
#  include <sys/vid.h>
#  define LED_CAP 0x01
#  define LED_NUM 0x02
#  define LED_SCR 0x04
# else /* SCO */
#  include <sys/at_ansi.h>
#  include <sys/kd.h>
#  include <sys/vt.h>
# endif /* SCO */

# if !defined(VT_ACKACQ)
#  define VT_ACKACQ 2
# endif /* !VT_ACKACQ */

# if defined(SCO)
#  include <sys/sysmacros.h>
# endif /* SCO */

# ifdef SVR4
#  include <sys/mman.h>
#  if !(defined(sun) && defined (i386) && defined (SVR4))
#    define DEV_MEM "/dev/pmem"
#  endif
#  define CLEARDTR_SUPPORT
#  define POSIX_TTY
# endif /* SVR4 */

# ifdef ISC
#  include <termios.h>
#  define POSIX_TTY
# endif

# if defined(sun) && defined (i386) && defined (SVR4)
#  define USE_VT_SYSREQ
#  define VT_SYSREQ_DEFAULT TRUE
# endif

# if defined(ATT) && !defined(i386)
#  define i386 /* note defined in ANSI C mode */
# endif /* ATT && !i386 */

# if (defined(ATT) || defined(SVR4)) && !(defined(sun) && defined (i386) && defined (SVR4))
#  define XQUEUE
#  include <sys/xque.h>
# endif /* ATT || SVR4 */

/* Hack on SVR3 and SVR4 to avoid linking in Xenix or BSD support */
#if defined (sun) && defined (i386) && defined (SVR4)
extern int xf86_solx86usleep(unsigned long);
# define usleep(usec) xf86_solx86usleep(usec) 
#else
# define usleep(usec) syscall(3112, (usec) / 1000 + 1)
#endif /* sun && i386 && SVR4 */

# ifdef SYSV
#  if !defined(ISC) || defined(ISC202) || defined(ISC22)
#   define NEED_STRERROR
#  endif
# endif

#ifndef NULL
# define NULL 0
#endif

#endif /* SYSV || SVR4 */

/**************************************************************************/
/* Linux                                                                  */
/**************************************************************************/
#if defined(linux)
# include <sys/ioctl.h>
# include <signal.h>
# include <termio.h>

# include <errno.h>
extern int errno;

# include <sys/stat.h>

# define HAS_USL_VTS
# include <sys/mman.h>
# include <sys/kd.h>
# include <sys/vt.h>
# define LDGMAP GIO_SCRNMAP
# define LDSMAP PIO_SCRNMAP
# define LDNMAP LDSMAP

# define CLEARDTR_SUPPORT
# define USE_VT_SYSREQ

# define POSIX_TTY

#endif /* linux */

/**************************************************************************/
/* LynxOS AT                                                              */
/**************************************************************************/
#if defined(Lynx)
 
# include <termio.h>
# include <sys/ioctl.h>
# include <sys/param.h>
# include <signal.h>
# include <sys/kd.h>
# include <sys/vt.h>
# include <sys/stat.h>

# include <errno.h>
extern int errno;
 
/* smem_create et.al. to access physical memory */ 
# include <smem.h>
 
/* keyboard types */
# define KB_84		1
# define KB_101 	2
# define KB_OTHER	3

/* atc drivers ignores argument to VT_RELDISP ioctl */
# define VT_ACKACQ	2

# include <termios.h>
# define POSIX_TTY

#endif /* Lynx */

/**************************************************************************/
/* 386BSD and derivatives,  BSD/386                                       */
/**************************************************************************/

#if defined(__386BSD__) && (defined(__FreeBSD__) || defined(__NetBSD__))
# undef __386BSD__
#endif

#ifdef CSRG_BASED
# include <sys/ioctl.h>
# include <signal.h>

# include <termios.h>
# define termio termios
# define POSIX_TTY

# include <errno.h>
extern int errno;

# if !defined(LINKKIT)
  /* Don't need this stuff for the Link Kit */
#  if defined(__bsdi__)
#   include <i386/isa/pcconsioctl.h>
#   define CONSOLE_X_MODE_ON PCCONIOCRAW
#   define CONSOLE_X_MODE_OFF PCCONIOCCOOK
#   define CONSOLE_X_BELL PCCONIOCBEEP
#  else /* __bsdi__ */
#   ifdef CODRV_SUPPORT
#    define COMPAT_CO011
#    if defined(__FreeBSD__) || defined(__NetBSD__)
#     include <machine/ioctl_pc.h>
#    else
#     include <sys/ioctl_pc.h>
#    endif /* __FreeBSD__ || __NetBSD__ */
#   endif /* CODRV_SUPPORT */
#   ifdef SYSCONS_SUPPORT
#    define COMPAT_SYSCONS
#    if defined(__FreeBSD__) || defined(__NetBSD__)
#     include <machine/console.h>
#    else
#     include <sys/console.h>
#    endif /* __FreeBSD__ || __NetBSD__ */
#   endif /* SYSCONS_SUPPORT */
#   if defined(PCVT_SUPPORT)
#    if !defined(SYSCONS_SUPPORT)
      /* no syscons, so include pcvt specific header file */
#     if defined(__FreeBSD__) || defined(__NetBSD__)
#      include <machine/pcvt_ioctl.h>
#     else
#      include <sys/pcvt_ioctl.h>
#     endif /* __FreeBSD__ || __NetBSD__ */
#    else /* pcvt and syscons: hard-code the ID magic */
#     define VGAPCVTID _IOWR('V',113, struct pcvtid)
      struct pcvtid {
	char name[16];
	int rmajor, rminor;
      };
#    endif /* PCVT_SUPPORT && SYSCONS_SUPPORT */
#   endif /* PCVT_SUPPORT */
    /* Include these definitions in case ioctl_pc.h didn't get included */
#   ifndef CONSOLE_X_MODE_ON
#    define CONSOLE_X_MODE_ON _IO('t',121)
#   endif
#   ifndef CONSOLE_X_MODE_OFF
#    define CONSOLE_X_MODE_OFF _IO('t',122)
#   endif
#   ifndef CONSOLE_X_BELL
#    define CONSOLE_X_BELL _IOW('t',123,int[2])
#   endif
#  endif /* __bsdi__ */
# endif /* !LINKKIT */

# include <sys/types.h>
# include <sys/mman.h>
# include <sys/stat.h>

# if defined(__bsdi__)
#  include <i386/isa/vgaioctl.h>
# endif /* __bsdi__ */

# define CLEARDTR_SUPPORT

# if defined(SYSCONS_SUPPORT) || defined(PCVT_SUPPORT)
#  define USE_VT_SYSREQ
# endif

# ifndef NULL
#   define NULL 0
# endif

#endif /* CSRG_BASED */

/**************************************************************************/
/* Mach and OSF/1                                                         */
/**************************************************************************/
#if defined(MACH386) || defined(__OSF__)
# include <sys/ioctl.h>

# include <signal.h>

# include <errno.h>
extern int errno;

# if defined(__OSF__)
#  include <sys/param.h>
#  include <machine/kd.h>
# else /* __OSF__ */
#  if !defined(__STDC__)
#   define __STDC__ 1
#   include <i386at/kd.h>
#   include <i386at/kd_queue.h>
#   undef __STDC__
#  else /* !__STDC__ */
#   include <i386at/kd.h>
#   include <i386at/kd_queue.h>
#  endif /* !__STDC__ */
#  include <sys/file.h>
#  define SEEK_SET L_SET
# endif /* __OSF__ */

# ifdef MACH386
#  define NEED_STRERROR
# endif

# include <sys/mman.h>
# include <sys/stat.h>
# define MOUSE_PROTOCOL_IN_KERNEL

#endif /* MACH386 || __OSF__ */

/**************************************************************************/
/* Minix                                                                  */
/**************************************************************************/
#if defined(MINIX)
# include <sys/ioctl.h>
# include <signal.h>

# include <termios.h>
# define termio termios
# define POSIX_TTY

# include <errno.h>

# include <assert.h>
# include <limits.h>
# include <sys/memio.h>
# include <sys/kbdio.h>

# include <sys/stat.h>

#endif /* MINIX */

/**************************************************************************/
/* Amoeba                                                                 */
/**************************************************************************/
#if defined(AMOEBA)
# define port am_port_t
# include <amoeba.h>
# include <cmdreg.h>
# include <stderr.h>
# include <ampolicy.h>
# include <proc.h>
# include <signal.h>
# include <server/iop/iop.h>
# include <errno.h>
# undef port

# undef _POSIX_SOURCE    /* to get the BSD-compatible symbols */
# include <sys/stat.h>

  /* keyboard types */
# define KB_84                   1
# define KB_101                  2
# define KB_OTHER                3

extern capability iopcap;
# define MOUSE_PROTOCOL_IN_KERNEL

#endif /* AMOEBA */

/**************************************************************************/
/* OS/2                                                                   */
/**************************************************************************/
/* currently OS/2 with EMX/GCC compiler only */
#if defined(__EMX__)
# include <signal.h>
# include <errno.h>
# include <sys/stat.h>

/* I would have liked to have this included here always, but
 * it causes clashes for BYTE and BOOL with Xmd.h, which is too dangerous. 
 * So I'll include it in place where I know it does no harm.
 */
#if defined(I_NEED_OS2_H)
# undef BOOL
# undef BYTE
# include <os2.h>
#endif

  /* keyboard types */
# define KB_84                   1
# define KB_101                  2
/* could detect more keyboards */
# define KB_OTHER                3

  /* LEDs */
#  define LED_CAP 0x40
#  define LED_NUM 0x20
#  define LED_SCR 0x10

  /* mouse driver */
# define OSMOUSE_ONLY
# define MOUSE_PROTOCOL_IN_KERNEL

#endif

/**************************************************************************/
/* Generic                                                                */
/**************************************************************************/

/* 
 * Hack originally for ISC 2.2 POSIX headers, but may apply elsewhere,
 * and it's safe, so just do it.
 */
#if !defined(O_NDELAY) && defined(O_NONBLOCK)
# define O_NDELAY O_NONBLOCK
#endif /* !O_NDELAY && O_NONBLOCK */

#if !defined(MAXHOSTNAMELEN)
# define MAXHOSTNAMELEN 32
#endif /* !MAXHOSTNAMELEN */

#if !defined(X_NOT_POSIX)
# if defined(_POSIX_SOURCE)
#  include <limits.h>
# else
#  define _POSIX_SOURCE
#  include <limits.h>
#  undef _POSIX_SOURCE
# endif /* _POSIX_SOURCE */
#endif /* !X_NOT_POSIX */
#if !defined(PATH_MAX)
# if defined(MAXPATHLEN)
#  define PATH_MAX MAXPATHLEN
# else
#  define PATH_MAX 1024
# endif /* MAXPATHLEN */
#endif /* !PATH_MAX */

#ifdef NEED_STRERROR
# ifndef strerror
extern char *sys_errlist[];
extern int sys_nerr;
#  define strerror(n) \
     ((n) >= 0 && (n) < sys_nerr) ? sys_errlist[n] : "unknown error"
# endif /* !strerror */
#endif /* NEED_STRERROR */

#ifndef VT_SYSREQ_DEFAULT
#define VT_SYSREQ_DEFAULT FALSE
#endif

#ifdef OSMOUSE_ONLY
# ifndef MOUSE_PROTOCOL_IN_KERNEL
#  define MOUSE_PROTOCOL_IN_KERNEL
# endif
#endif

/* The Region arg to xf86[Un]Map* */
#define NUM_REGIONS 3
#define VGA_REGION 0
#define LINEAR_REGION 1
#define EXTENDED_REGION 2

#ifndef NO_OSLIB_PROTOTYPES
/*
 * This is to prevent re-entrancy to FatalError() when aborting.
 * Anything that can be called as a result of AbortDDX() should use this
 * instead of FatalError(). (xf86Exiting gets set to TRUE the first time
 * AbortDDX() is called.)
 */

extern Bool xf86Exiting;

#define xf86FatalError(a, b) \
	if (xf86Exiting) { \
		ErrorF(a, b); \
		return; \
	} else FatalError(a, b)

/***************************************************************************/
/* Prototypes                                                              */
/***************************************************************************/

#include <X11/Xfuncproto.h>

_XFUNCPROTOBEGIN

/* xf86_Util.c */
extern int StrCaseCmp(
#if NeedFunctionPrototypes
	char *,
	char *
#endif
);

/* OS-support layer */
extern void xf86OpenConsole(
#if NeedFunctionPrototypes
	void
#endif
);
extern void xf86CloseConsole(
#if NeedFunctionPrototypes
	void
#endif
);
extern Bool xf86VTSwitchPending(
#if NeedFunctionPrototypes
	void
#endif
);
extern Bool xf86VTSwitchAway(
#if NeedFunctionPrototypes
	void
#endif
);
extern Bool xf86VTSwitchTo(
#if NeedFunctionPrototypes
	void
#endif
);
extern Bool xf86LinearVidMem(
#if NeedFunctionPrototypes
	void
#endif
);
extern pointer xf86MapVidMem(
#if NeedFunctionPrototypes
	int,
	int,
	pointer,
	unsigned long
#endif
);
extern void xf86UnMapVidMem(
#if NeedFunctionPrototypes
	int,
	int,
	pointer,
	unsigned long
#endif
);
extern void xf86MapDisplay(
#if NeedFunctionPrototypes
	int,
	int
#endif
);
extern void xf86UnMapDisplay(
#if NeedFunctionPrototypes
	int,
	int
#endif
);
extern int xf86ReadBIOS(
#if NeedFunctionPrototypes
	unsigned long,
	unsigned long,
	unsigned char *,
	int
#endif
);
extern void xf86ClearIOPortList(
#if NeedFunctionPrototypes
	int
#endif
);
extern void xf86AddIOPorts(
#if NeedFunctionPrototypes
	int,
	int,
	unsigned *
#endif
);
void xf86EnableIOPorts(
#if NeedFunctionPrototypes
	int
#endif
);
void xf86DisableIOPorts(
#if NeedFunctionPrototypes
	int
#endif
);
void xf86DisableIOPrivs(
#if NeedFunctionPrototypes
	void
#endif
);
extern Bool xf86DisableInterrupts(
#if NeedFunctionPrototypes
	void
#endif
);
extern void xf86EnableInterrupts(
#if NeedFunctionPrototypes
	void
#endif
);
extern int xf86ProcessArgument(
#if NeedFunctionPrototypes
	int,
	char **,
	int
#endif
);
extern void xf86UseMsg(
#if NeedFunctionPrototypes
	void
#endif
);
extern void xf86SoundKbdBell(
#if NeedFunctionPrototypes
	int,
	int,
	int
#endif
);
extern void xf86SetKbdLeds(
#if NeedFunctionPrototypes
	int
#endif
);
extern int xf86GetKbdLeds(
#if NeedFunctionPrototypes
	void
#endif
);
extern void xf86SetKbdRepeat(
#if NeedFunctionPrototypes
	char
#endif
);
extern void xf86KbdInit(
#if NeedFunctionPrototypes
	void
#endif
);
extern int xf86KbdOn(
#if NeedFunctionPrototypes
	void
#endif
);
extern int xf86KbdOff(
#if NeedFunctionPrototypes
	void
#endif
);
extern void xf86KbdEvents(
#if NeedFunctionPrototypes
	void
#endif
);
extern void xf86SetMouseSpeed(
#if NeedFunctionPrototypes
	int,
	int,
	unsigned
#endif
);
extern void xf86MouseInit(
#if NeedFunctionPrototypes
	void
#endif
);
extern int xf86MouseOn(
#if NeedFunctionPrototypes
	void
#endif
);
extern int xf86MouseOff(
#if NeedFunctionPrototypes
	Bool
#endif
);
extern void xf86MouseEvents(
#if NeedFunctionPrototypes
	void
#endif
);
extern int  xf86XqueKbdProc(
#if NeedFunctionPrototypes
	DevicePtr,
	int
#endif
);
extern int  xf86XqueMseProc(
#if NeedFunctionPrototypes
	DevicePtr,
	int
#endif
);
extern void xf86XqueEvents(
#if NeedFunctionPrototypes
	void
#endif
);


/* These are privates */
extern void xf86InitPortLists(
#if NeedFunctionPrototypes
	unsigned **,
	int *,
	Bool *,
	Bool *,
	int
#endif
);
extern Bool xf86CheckPorts(
#if NeedFunctionPrototypes
	unsigned,
	unsigned **,
	int *,
	Bool *,
	int
#endif
);
extern int  xf86OsMouseProc(
#if NeedFunctionPrototypes
	DevicePtr,
	int
#endif
);
extern void xf86OsMouseEvents(
#if NeedFunctionPrototypes
	void
#endif
);
extern void xf86OsMouseOption(
#if NeedFunctionPrototypes
	int,
	pointer /* gets cast to LexPtr later, saves include file hassles */
#endif
);

_XFUNCPROTOEND
#endif /* NO_OSLIB_PROTOTYPES */

#endif /* _XF86_OSLIB_H */
