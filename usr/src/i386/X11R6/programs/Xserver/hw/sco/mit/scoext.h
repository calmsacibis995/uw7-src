/*
 *	@(#)scoext.h	11.2	11/17/97	12:03:03
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1997.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Tue Jun 11 16:53:08 PDT 1991	mikep@sco.com
 *	- Completely revamped from sco.h.  This should only be the info
 * 	needed outside of the os/sco/{i386/mips} directory.
 *
 *	S001	Fri Jun 14 00:02:23 PDT 1991	buckm@sco.com
 *	- Add declaration of scoCursorOn().
 *
 *	S002	Mon Jul 15 15:37:16 PDT 1991	buckm@sco.com
 *	- Get rid of scoCursorOn() and scoCursorInit().
 *	- Add new cursor routines.
 *
 *	S003	Mon Aug 26 15:40:12 PDT 1991	mikep@sco.com
 *	- Put in new scoScreenInfo, scoSysInfoRec structures.
 *
 *	S004	Sat Sep 21 23:48:31 PDT 1991	mikep@sco.com
 *	- Add runSwitched, and version.  Remove CursorOn.
 *
 *	S005	Mon Nov 18 17:42:53 PST 1991	mikep@sco.com
 *	- Yikes! _scoSysInfoRec.numScreens needs to be an int!!!!!
 *
 *	S006	Wed Sep 09 20:08:15 PDT 1992	mikep@sco.com
 *	- Up XSCO_VERSION to 5.0
 *
 *	S007	Tue Sep 15 20:00:06 PDT 1992	mikep@sco.com
 *	- Add ddxBitSwap[], change scoCloseScreen() to a Bool
 *
 *	S008	Sat Oct 10 13:18:53 PDT 1992	mikep@sco.com
 *	- Add per-screen cursor type so that NFB knows when to
 *	  wrap.
 *
 *	S009	Wed Nov 11 14:09:38 PST 1992	hiramc@sco.COM
 *	- ddxBitSwap is an UNSIGNED char, not just a char
 *
 *	S010	Thu Mar 11 16:17:22 PST 1993	mikep@sco.com
 *	- add MSBIT_SWAP macro
 *
 *      S011    Mon Dec 19 14:57:37 PST 1994    kylec@sco.COM
 *      - BUG FIX SCO-59-4718 -  Add GetImage() to scoScreenPriv.
 *      GetImage() in ScreenRec is temporarly replaced with NoopDDA
 *      when we switch away from the running X server to another multiscreen.
 *      The correct GetImage() is restored from scoScreenPriv when
 *      we switch back to the running X server.
 *
 *	S012 	Mon Nov 17 09:38:22 EST 1997	brianr@sco.com
 *	- Take out fields added to wrap SaveScreen and HandleExposures.
 *	- add volatile to scoSysInfoRec declaration
 *      
 */

#ifndef _SCOEXT_H_
#define _SCOEXT_H_


#include    "X.h"
#include    "Xproto.h"
#include    "scrnintstr.h"
#include    "screenint.h"
#include    "dix.h"

/*
 * Per screen info for SCO system layer
 */

typedef struct _scoScreenInfo {
    ScreenPtr	pScreen;           	/* Screen to switch */
    void	(*SetGraphics)();  	/* Set graphics mode */
    void	(*SetText)();		/* Set text mode */
    void    	(*SaveGState)();	/* Save graphics state */
    void    	(*RestoreGState)();	/* Restore graphics state */
    void    	(*CloseScreen)();	/* Close screen routine */
    Bool	exposeScreen;		/* Send expose event on switch */
    Bool	isConsole;		/* Is this the console? */
    Bool	runSwitched;		/* Can the server continue to run 
					when the user switched multiuscreens? */
    float	version;		/* Linkkit version number */
} scoScreenInfo;

typedef volatile struct _scoSysInfoRec	/* S012 */
{
    int numScreens;             /* How many screens */
    int consoleFd;              /* Fd of console */
    int currentVT;              /* Multiscreen number */
    Bool screenActive;          /* Do we have the screen? */
    Bool switchEnabled;         /* Are we screen switching yet? */
#if defined(usl)
    void *pPointer;
    void *pKeyboard;
    Bool switchReqPending;      /* Caught screen switch signal? */
    Bool inputPending;          /* Synthetic input events? */
    Bool reset;                 /* Server is reseting */
    int defaultMode;            /*  */
#endif /* usl */
    scoScreenInfo *scoScreens[MAXSCREENS];
    char cursorType[MAXSCREENS]; /* HW or SOFT cursor */	/* S008 */
} scoSysInfoRec;

typedef struct _scoScrnPriv {
    void        (*CloseScreen)();
    void        (*GetImage)();          /* S011 */
#ifdef NOTNOW				/* S012 */
    Bool	(*SaveScreen)();
    void	(*HandleExposures)();
#endif
} scoScrnPriv, *scoScrnPrivPtr;

extern scoSysInfoRec scoSysInfo;

extern unsigned char ddxBitSwap[];

#if BITMAP_BIT_ORDER == LSBFirst
#define MSBIT_SWAP(byte)  ddxBitSwap[byte]
#else
#define MSBIT_SWAP(byte)  byte
#endif

/*
 * Cursor functions
 */
extern void scoSWCursorInitialize();
extern Bool scoDCInitialize ();
extern Bool scoSpriteInitialize ();
extern Bool scoPointerInitialize();

/*
 * Initialization
 */
extern Bool	scoCloseScreen();
extern void	SetTimeSinceLastInputEvent();
extern void	scoSysInfoInit(ScreenPtr pScreen, scoScreenInfo *pSysInfo);


#define 	SSFD		-1
#define		SCOMAGIC_F	NoopDDA	
#define		SCOMAGIC_I	-1
#define		SCOMAGIC_P	NULL

#define		SCO_HW_CURSOR	0
#define		SCO_SOFT_CURSOR 1

#define		OLD_BIT_ORDER_VERSION	4.1
#define		XSCO_VERSION	5.0

#define SCO_PRIVATE_DATA(pScreen) \
((scoScrnPrivPtr)((pScreen)->devPrivates[scoScreenPrivateIndex].ptr))

#endif /* _SCOEXT_H_ */
