/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Procs.h,v 3.2 1995/01/28 17:03:36 dawes Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: xf86Procs.h /main/3 1995/11/12 19:21:53 kaleb $ */

#ifndef _XF86PROCS_H
#define _XF86PROCS_H

#include <X11/Xfuncproto.h>
#include "xf86.h"
#include "xf86Priv.h"

_XFUNCPROTOBEGIN


/* xf86Config.c */

extern void xf86Config();

extern Bool xf86LookupMode(
#if NeedFunctionPrototypes
	DisplayModePtr,		/* target */
	ScrnInfoPtr		/* driver */
#endif 
);

extern void xf86VerifyOptions(
#if NeedFunctionPrototypes
	OFlagSet *,		/* allowedOptions */
	ScrnInfoPtr		/* driver */
#endif
);

extern void xf86DeleteMode(
#if NeedFunctionPrototypes
	ScrnInfoPtr,		/* infoptr */
	DisplayModePtr		/* dispmp */
#endif
);

extern char *xf86TokenToString(
#if NeedFunctionPrototypes
	SymTabPtr,		/* table */
	int			/* token */
#endif
);

extern int xf86StringToToken(
#if NeedFunctionPrototypes
	SymTabPtr,		/* table */
	char *			/* string */
#endif
);

/* xf86Cursor.c */

extern void xf86InitViewport(
#if NeedFunctionPrototypes
	ScrnInfoPtr		/* pScr */
#endif 
);

extern void xf86SetViewport(
#if NeedFunctionPrototypes
	ScreenPtr,		/* pScreen */
	int,			/* x */
	int			/* y */
#endif 
);

extern void xf86ZoomViewport(
#if NeedFunctionPrototypes
	ScreenPtr,		/* pScreen */
	int			/* zoom */
#endif 
);


/* xf86Events.c */

extern void ProcessInputEvents();

extern void xf86PostKbdEvent(
#if NeedFunctionPrototypes
	unsigned		/* key */
#endif 
);

extern void xf86PostMseEvent(
#if NeedFunctionPrototypes
	int,			/* buttons */
	int,			/* dx */
	int			/* dy */
#endif
);

extern void xf86Block(
#if NeedFunctionPrototypes
	pointer,		/* blockData */
	OSTimePtr,		/* pTimeout */
	pointer			/* pReadmask */
#endif
);

extern void xf86Wakeup(
#if NeedFunctionPrototypes
	pointer,		/* blockData */
	int,			/* err */
	pointer			/* pReadmask */
#endif
);

extern void xf86VTRequest(
#if NeedFunctionPrototypes
	int			/* signo */
#endif
);

extern void xf86SigHandler(
#if NeedFunctionPrototypes
	int		       /* signo */
#endif
);

/* xf86Io.c */

extern void xf86KbdLeds();

extern void xf86InitKBD(
#if NeedFunctionPrototypes
	Bool			/* init */
#endif
);

#ifdef NEED_EVENTS
extern void xf86KbdBell(
#if NeedFunctionPrototypes
	int,			/* loudness */
	DeviceIntPtr, 		/* pKeyboard */
	pointer, 		/* ctrl */
	int
#endif
);
#endif

extern void xf86KbdCtrl(
#if NeedFunctionPrototypes
	DevicePtr,		/* pKeyboard */
	KeybdCtrl *		/* ctrl */
#endif
);

extern int  xf86KbdProc(
#if NeedFunctionPrototypes
	DevicePtr,		/* pKeyboard */
	int			/* what */
#endif
);

extern void xf86KbdEvents();

extern void xf86MseCtrl(
#if NeedFunctionPrototypes
	DevicePtr,		/* pPointer */
	PtrCtrl*		/* ctrl */
#endif
);

extern int  xf86MseProc(
#if NeedFunctionPrototypes
	DevicePtr,		/* pPointer */
	int			/* what */
#endif
);

extern void xf86MseEvents();

/* xf86_Mouse.c */

extern Bool xf86MouseSupported(
#if NeedFunctionPrototypes
	int			/* mousetype */
#endif
);

extern void xf86SetupMouse(
#if NeedFunctionPrototypes
	void
#endif
);

extern void xf86MouseProtocol(
#if NeedFunctionPrototypes
	unsigned char *,	/* rBuf */
	int			/* nBytes */
#endif
);


/* xf86Kbd.c */

extern void xf86KbdGetMapping(
#if NeedFunctionPrototypes
	KeySymsRec *,		/* pKeySyms */
	CARD8 *			/* pModMap */
#endif
);

_XFUNCPROTOEND

#endif /* _XF86PROCS_H */


