/*
 *	@(#)sco.h	6.3	1/26/96	13:47:37
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1991.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/*-
 * sco.h --
 *	Internal declarations for the sco ddx interface
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *"$XConsortium: sco.h,v 5.7 89/12/06 09:37:35 rws Exp $ SPRITE (Berkeley)"
 */

/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	??? Oct ?? ??:??:?? PST 1990	mikep@sco.com
 *	- Created from sun.h
 *
 *	S001	Mon Feb 11 21:43:19 PST 1991	mikep@sco.com
 *	- Removed autoRepeat references
 *
 *	S002	Tue Apr 02 15:00:36 PST 1991	mikep@sco.com
 *	-  Added screen switch structure.  Removed Sun garbage from
 *	keyboard structure.  Removed Sun Cursor Private structure.
 *	Remove inclusion of mfb.h to not conflict with cfb.h in
 *	mwData.h.  This is a lazy hack since both files should be able
 *      to be included in the same source.
 *
 *	S003	Tue Apr 02 22:02:05 PST 1991	mikep@sco.com
 *	- Broke down and added scoStatusRecord and friends from mw.h
 *
 *	S004	Wed Apr 03 12:38:45 PST 1991	mikep@sco.com
 *	- Extended the screen switch structure to include blank and
 *	unblank screen.
 *
 *	S005	Fri Apr 05 22:28:43 PST 1991	mikep@sco.com
 *	- Deleted LocalMode garbage.  Added AltSysReq define.
 *	- Remove UnblankScreen since blank could do it already.
 *
 *	S006	Mon Apr 08 17:56:26 PDT 1991	mikep@sco.com
 *	- Added AllScreens define for calls to OS layer.
 *
 *	S007	Thu Apr 18 20:15:50 PDT 1991	mikep@sco.com
 *	- Removed BlankScreen from SS struct since it's already
 *	in the pScreen struct.
 *
 *	S008	Thu May 02 20:38:52 PDT 1991	mikep@sco.com
 *	- Added Cursor control to SS struct.
 *	- Move status information into scoOs.h
 *
 *	S009	Wed May 08 12:54:25 PDT 1991	mikep@sco.com
 *	- Removed mi.h
 *
 *	S010	Tue Jun 11 18:01:26 PDT 1991	mikep@sco.com
 *	- Removed information which is in scoext.h
 *
 *      S011    Fri Jun 14 02:35:33 PDT 1991	buckm@sco.com
 *      - Get rid of scoVidFd and scoGrafData.
 *
 *      S012    Tue Oct 01 02:06:52 PDT 1991	buckm@sco.com
 *      - Get rid of monitorResolution.
 *
 *	S013	Tue Sep 29 22:16:13 PDT 1992	mikep@sco.com
 *	- Trim Keyboard and Pointer private structures to only
 *	  include the data we need.  Also rearrange to put the
 *	  function pointers at the top.
 *	- Change function names to EnqueueEvent since that's what
 *	  they do now.
 *
 *	S014	Fri Jan 26 13:41:16 PST 1996	hiramc@sco.COM
 *	- begin modifications to build server on Unixware
 *
 * 	S015	Mon Sep 30 15:16:57 PDT 1996	kylec@sco.com
 *	- Re-define KbPrivRec.  Remove unused fields.
 *        Add some new ones.
 */

#ifndef _SCO_H_
#define _SCO_H_

#include    <errno.h>
extern int  errno;
#include    <sys/param.h>
#include    <sys/types.h>
#include    <sys/time.h>
#include    <sys/file.h>
#include    <sys/fcntl.h>
#include    <sys/event.h>

#if defined(usl)
#undef _POSIX_SOURCE
#include    <sys/types.h>
#include    <string.h>
#include    <unistd.h>
#define _POSIX_SOURCE
#include    <signal.h>
#include    <sys/kd.h>
#include    <sys/proc.h>
#include    <sys/seg.h>
#include    <sys/stat.h>
#include    <sys/vt.h>
#else 
#include    <sys/signal.h>
#include    <sys/console.h>
#include    <mouse.h>
#endif /* usl */

#include    "scoIo.h"
#include    "X.h"
#include    "Xproto.h"
#include    "scrnintstr.h"
#include    "screenint.h"
#ifdef NEED_EVENTS
#include    "inputstr.h"
#endif /* NEED_EVENTS */
#include    "input.h"
#include    "cursorstr.h"
#include    "cursor.h"
#include    "pixmapstr.h"
#include    "pixmap.h"
#include    "windowstr.h"
#include    "gc.h"
#include    "gcstruct.h"
#include    "regionstr.h"
#include    "colormap.h"
#include    "miscstruct.h"
#include    "dix.h"

#include    "scoOs.h"
#include    "scoext.h"						/* S010 */

/*
 * Data private to any sco keyboard.
 *	EnqueueEvent processes a single event and gives it to MI
 *	devPrivate is private to the specific keyboard.
 *	keep EnqueueEvent first since it's used in the most time criticle
 *	manner.
 */
typedef struct kbPrivate {
    void    	  (*EnqueueEvent)();	/* Function to process an event S013 */
    int	    	  fd;	    	    	/* File descriptor for controlling kybd */
    int		  offset;		/* to be added to device keycodes */

    unsigned int  lockState;
    KeyCode	  lastKey;      	/* Keep track for auto-repeat */

    KeybdCtrl	  ctrl;    	    	/* Current control structure (for
 					 * keyclick, bell duration, auto-
 					 * repeat, etc.) */
} KbPrivRec, *KbPrivPtr;

#define	MIN_KEYCODE	8	/* necessary to avoid the mouse buttons */
#define AT_KBD		2
#define PC_KBD		3

/*
 * Data private to any sco pointer device.
 *	keep EnqueueEvent first since it's used in the most time criticle
 *	manner.
 */
typedef struct ptrPrivate {
    void    	  (*EnqueueEvent)();	/* Function to process an event S013 */
    int	    	  fd;	    	    	/* Descriptor to device */
    pointer    	  devPrivate;	    	/* Field private to device */
} PtrPrivRec, *PtrPrivPtr;

extern char *scoVidModeStr;

#define tvminus(tv, tv1, tv2)	/* tv = tv1 - tv2 */ \
		if ((tv1).tv_usec < (tv2).tv_usec) { \
			(tv1).tv_usec += 1000000; \
			(tv1).tv_sec -= 1; \
		} \
		(tv).tv_usec = (tv1).tv_usec - (tv2).tv_usec; \
		(tv).tv_sec = (tv1).tv_sec - (tv2).tv_sec;

#define tvplus(tv, tv1, tv2)	/* tv = tv1 + tv2 */ \
		(tv).tv_sec = (tv1).tv_sec + (tv2).tv_sec; \
		(tv).tv_usec = (tv1).tv_usec + (tv2).tv_usec; \
		if ((tv).tv_usec > 1000000) { \
			(tv).tv_usec -= 1000000; \
			(tv).tv_sec += 1; \
		}

/*-
 * TVTOMILLI(tv)
 *	Given a struct timeval, convert its time into milliseconds...
 */
#define TVTOMILLI(tv)	(((tv).tv_usec/1000)+((tv).tv_sec*1000))


#endif /* _SCO_H_ */
