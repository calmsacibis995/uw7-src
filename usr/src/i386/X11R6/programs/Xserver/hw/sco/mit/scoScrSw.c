/*
 *	@(#)scoScrSw.c	11.3	11/17/97	12:08:07
 *
 *	Copyright (C) 1991-1997 The Santa Cruz Operation, Inc.
 *
 *	The information in this file is provided for the exclusive use of the
 *	licensees of The Santa Cruz Operation, Inc.  Such users have the right
 *	to use, modify, and incorporate this code into other products for
 *	purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 */
/*
 *	S000, 15-MAR-91, hiramc, SCO-000-000
 *		brought over from mwzdevs.c
 *	S001, 02-APR-91, mikep@sco.com
 *		Rewrote most of this file for new Screen Switch
 *		interface.
 *	S002, 03-APR-91, mikep@sco.com
 *		Added blank and unblank screen routines to the SS
 *		interface.
 *	S003, 04-APR-91, mikep@sco.com
 *		Added POSIX safe signal handling.
 *	S004, 05-APR-91, mikep@sco.com
 *		Fixed BlankScreen calls to do Unblank as well
 *	S005, 17-APR-91, mikep@sco.com
 *		Removed Blank screen routine and called pScreen 
 *		ScreenSave instead.
 *	S006, 20-APR-91, mikep@sco.com
 *		Modfications suggested in code review.  Don't let the
 *		called set the switch fd and don't call ioctl twice.
 *	S007, 24-APR-91, mikep@sco.com
 *		Changed scoLeaveVtProcMode() to not call GiveUp(),
 *		since GiveUp() calls scoLeaveVtProcMode(), and that
 *		wouldn't get us anywhere.
 *	S008, 03-MAY-91, mikep@sco.com
 *		Major changes to signal handlers and screen switch routines
 *		Everything is now called from WaitForSomething instead of
 *		ProcessInput events as was done before.  This guarrantees
 *		us that they will always get called after all pending events 
 *		have been handled.
 *		Added CursorOn routines to SS Rec.  Needed for clients like
 *		mwm who grab the cursor.
 *		Don't unset VtProcMode if we never got there.
 *		The console driver will put us into text mode after a
 *		screen switch.  Force us back into some graphics mode.
 *	S009, 04-MAY-91, mikep@sco.com
 *		It seems the event driver is much happier if we resume()
 *		before the VT_ACKACQ ioctl.  To be consitent we suspend()
 *		after the VT_RELDISP.  So it looks like this:
 *
 *		release the screen
 *			suspend the event queue
 *			resume the event queue
 *		acquire the screen
 *	S010, 06-MAY-91, buckm@sco.com
 *		Flush event queue just before suspending.  This keeps
 *		select from reporting events that can't be read, and
 *		besides, who wants to save outdated events for later?
 *	S011, 06-MAY-91, mikep@sco.com
 *		Nix the VT_AUTO fail message.  Even if it did,
 *		what's the user going to do about it?   Most the time
 *		it's because we never got to VT_PROCESS.
 *	S012, 06-MAY-91, mikep@sco.com
 *		Remove some stuff which was conflicting with scoMerge.c
 *	S013, 08-MAY-91, buckm@sco.com
 *		Suspend event driver before VT_RELDISP.  This breaks
 *		the symmetry of S009, but is consistent in another way.
 *	S014, 08-MAY-91, mikep@sco.com
 *		Massage code to allow Merge extension to call 
 *		scoReleaseScreen() and scoAcquireScreen().
 *		Created scoSetSSFD() instead of having SetVtProcMode do 
 *		two things.  Added some more comments.
 *	S015, 09-MAY-91, buckm@sco.com
 *		Don't change the state of screenActive in scoInitSS.
 *		Shouldn't ever ignore a caught screen switch signal;
 *		just say NO.  Move screenActive changing closer to
 *		the vt ioctls.  The screen switch signal handlers
 *		set separate flags, don't modify screenActive, and
 *		never just ignore the signal.  scoScreenActive does
 *		not do any ioctls, just returns screenActive.
 *		Add scoBecomeActiveScreen routine for getting to a
 *		known state after entering VtProc mode.
 * 	S016, 09-May-91, staceyc@sco.com
 *		Reworked screen switch filter.  Added ioctl to
 * 		notice if alt depressed on screen acquire.
 *	S017, 10-May-91, staceyc@sco.com
 *		Fixed typo(!) in screen switch filter.  Made check for
 *		depressed alt more comprehensive.
 *	S018, 10-May-91, buckm@sco.com
 *		Add scoUnblankScreen(), called from WaitForSomething().
 *	S019, 11-May-91, buckm@sco.com
 *		In scoBecomeActiveScreen, change pause to sleep
 *		to avoid waiting forever because of race condition.
 *	S020, 11-May-91, mikep@sco.com
 *		There's still the window in which a SIG_ACQ may arrive
 *		between the time we release the screen and enter select()
 *		in WaitForSomething().   By turning on the event driver
 *		in the signal handler, we know that select will at least
 *		wake up when it sees a key press.
 *		Note: We still need to call ev_resume in scoAcquireScreen()
 *		for Merge.  It don't hurt nothing.
 *	S021, 11-May-91, mikep@sco.com
 *		Turned on Merge keyboard code.
 *	S022, 30-May-91, staceyc@sco.com
 * 		Major reworking of screen switch code to deal with
 *		scoScreenSwitchKeys.
 *		Most of checkMods() and all of scoScreenSwitchFilter
 *		rewritten.
 *      S023, 14-Jun-91, buckm@sco.com
 *		Add scoSetText(), called from scoRestoreVid().
 *	S024, 26-Aug-91, mikep@sco.com
 *		Rewrote most of file to deal with the scoSysInfoRec and
 *		multiple screens.
 *      S025, 11-Sep-91, buckm@sco.com
 *      	Get rid of SW_CG320 call; we let the ddx drivers
 *		take care of it if they need to.
 *      S026, 12-Sep-91, mikep@sco.com
 *		Call miPointerOn() instead of CursorOn from sysInfo.
 *		This simplifies our interface.
 *      S027, 12-Sep-91, mikep@sco.com
 *		Check the value of runSwitched.  This allows a ddx layer to 
 *		keep on running after the user has switched away or zoomed.
 *		Note this will only work if every screen on the system 
 *		supports this feature.  If one doesn't, they all have to 
 *		go away.
 *      S028, 24-Sep-91, buckm@sco.com
 *      	Our screen number is available in scoSysInfo now,
 *		so use it in scoScreenSwitchFilter.
 *	S029, 26-Sep-91, staceyc@sco.com
 *		Move all data structures and routines needing those data
 *		structures into this file.  Complete rewrite of screen
 *		switching code.
 *      S030, 27-Sep-91, buckm@sco.com
 *      	Fix a few things in S029.
 *      S031, 29-Sep-91, edb@sco.com
 *              Turn off cursor before we go into Text mode
 *              The evc screen memory does'nt like to be touched afterwars 
 *	S032, 1-Oct-91, mikep@sco.com
 *		Send correct scancodes for Alt_R and Ctrl_R keys.  This
 *		forces the X server to see the right keys.  This involves
 *		sending the E0 leader before hand.
 *		Allow for Ctrl-Shift-KP_Multiply to switch screens just like
 *		Ctrl-PrintScreen
 *	S033, 4-Oct-91, staceyc@sco.com
 *		check state of leds before and after screen switch and try
 *		to get them in a reasonable state
 *	S034, 21-Oct-91, mikep@sco.com
 *		If Merge dies while zoom is may screw up the event queue.
 *		If this is the case, bail-out.  It's not clear what needs
 *		to be done to recover.
 *	S035, 20-Aug-91, mikep@sco.com
 *		Seems to be a race condition between DOS, xcrt and the server.
 *		What if xcrt exits so fast that the server trys to do a 
 *		VT_PROCESS before DOS has really released it?  Let's just
 *		retry a couple of times.  Seems to solve the problem.
 *	S036, 19-Nov-91, mikep@sco.com
 *		Turning off the cursor must be done after we have created
 *		the cover window.  Sigh.
 *	S037, 24-Jun-93, buckm@sco.com
 *		Check global flag scoRunSwitched in sco{Acquire,Release}Screen.
 *		Add scoWaitUntilScreenActive().
 *	S038, 10-Aug-93, buckm@sco.com
 *		Change mipointer sprite on/off interface.
 *	S039, Mon Oct 10 10:52:32 1994, kylec@sco.com
 *		- Pass mapcodes to scoSSQueKey(), not scancodes.
 *		  BUG SCO-59-4154.
 *		- Use scoKeysymToKeycode() to get mapcodes for
 *		  keysyms.  No longer need to use hardcoded scancode
 *		  values.  BUG SCO-59-4155.
 *	S040, Thu Aug 29 04:57:15 GMT 1996, kylec@sco.com
 *		- Fix misc problems with running server while in
 *		  text mode.
 *	S041, Tue Sep 24 13:00:09 PDT 1996, kylec@sco.com
 *		- Don't send simulated key events.  Led state is automatically
 *		  restored by vtlmgr.  Non-locking modifier state reset
 *		  when enable devices.
 *	S042, Tue Oct 15 10:06:09 PDT 1996, kylec@sco.com
 *		- Fix misc problems with sprite after screen switch.
 *	S043, Mon Nov  3 13:56:49 PST 1997, hiramc@sco.COM
 *		- fixup scoWaitUntilScreenActive() since it is now used
 *		- properly in dix/window.c and dix/main.c
 *	S044, Mon Nov  17 9:56:49 PST 1997, brianr@sco.com
 *		- replaces S043
 *		- scoWaitUntilScreenActive() waits for the signal and
 *		  kicks off its own acquire.
 */

#include <stdio.h>
#if defined(usl)
#include <uwareIo.h>
#define VT_FALSE        0
#define VT_TRUE         1
#define	CTRL	CNTRL
#endif
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/page.h>
#include <sys/sysmacros.h>
#if defined(usl)
#include <sys/vt.h>
#include <sys/kd.h>
#else
#include <sys/vtkd.h>
#include <sys/keyboard.h>
#endif
#include <sys/param.h>
#include <sys/event.h>
#include <sys/vid.h>
#include "X.h"
#include "Xproto.h"
#include "sco.h"
#include "keysym.h"
#include "pckeys.h"

#define SIG_REL	SIGUSR1
#define SIG_ACQ	SIGUSR2

extern Bool scoRunSwitched;					/* S037 */
extern int scoKeysymToKeycode(KeySym); 				/* S039 */ 

static void scoSSQueKey();
static void scoSSCheckMods();
static int scoSSSetNewVT(int screen);

typedef struct scoSSLED_t {     /* S033 */
    unsigned int led_mask;
} scoSSLED_t;

#define SCO_SS_LED_COUNT (sizeof(scoSSLED) / sizeof(scoSSLED[0]))
static scoSSLED_t scoSSLED[] = { {CLKED}, {NLKED}, {SLKED}};

typedef struct scoSSKeyTrans_t {
    KeySym lkey;
    KeySym rkey;
    char char_rep;
    int checked_since_acquire;
    unsigned long mod_mask;
} scoSSKeyTrans_t;

#define SCO_SS_KEY_TRANS_SIZE (sizeof(scoSSKeyTrans) / sizeof(scoSSKeyTrans[0]))
static scoSSKeyTrans_t scoSSKeyTrans[] = {
    {XK_Alt_L,		XK_Alt_R,	'a', 1,	ALT},
    {XK_Shift_L,	XK_Shift_R,	's', 1, SHIFT},
    {XK_Control_L,	XK_Control_R,	'c', 1, CTRL}};

static unsigned long scoSSCurrentModKeys = 0;
static unsigned long scoSSMask = 0;
static unsigned int scoSSLEDState = 0;
static int scoSSLEDStatus = -1;
#define SCO_MAX_SWKEYS	3				/* S030 */
static char scoScreenSwitchKeys[SCO_MAX_SWKEYS + 1];	/* null terminated */

/*
 *  scoScreenSwitchFilter()
 *
 * Return 1 if screen switch detected, otherwise, return 0.
 */
int
scoScreenSwitchFilter(keypressed, key)
    int keypressed;
    KeySym key;
{
    int i;
    int screen;
    unsigned long currmods = 0;

    i = 0;
    while (i < SCO_SS_KEY_TRANS_SIZE &&
           key != scoSSKeyTrans[i].lkey && key != scoSSKeyTrans[i].rkey)
        ++i;
    if (i < SCO_SS_KEY_TRANS_SIZE)
    {
        if (! scoSSKeyTrans[i].checked_since_acquire && ! keypressed)
        {
            (void)ioctl(scoSysInfo.consoleFd, KDGKBSTATE,
                        &currmods);
            if (! (currmods & scoSSKeyTrans[i].mod_mask))
            {
                scoSSKeyTrans[i].checked_since_acquire = 1;

                /* S039 !!! */
                scoSSQueKey(
                            scoKeysymToKeycode(scoSSKeyTrans[i].lkey),
                            DEVE_KBTUP);
                scoSSQueKey(
                            scoKeysymToKeycode(scoSSKeyTrans[i].rkey),
                            DEVE_KBTUP);
                /* S039 ^^^ */
            }
        }
        if (keypressed)
            scoSSCurrentModKeys |= scoSSKeyTrans[i].mod_mask;
        else
            scoSSCurrentModKeys &= ~scoSSKeyTrans[i].mod_mask;
        return 0;
    }
    if (key >= XK_F1 && key <= XK_F12 &&
        scoSSCurrentModKeys == scoSSMask && keypressed)	/* S030 */
    {
        screen = key - XK_F1;   /* S030 */
        (void)scoSSSetNewVT(screen);
        return 1;
    }
    /*
     * Here's some trivia: control-printscreen is meant to
     * screen switch to the next tty and wrap at the end.
     *
     * S032
     * Still more trivia: control-printscreen == control-shift-KP_*
     */
    if (keypressed && 
        ((scoSSCurrentModKeys & CTRL) && key == XK_Print) || 
        ((scoSSCurrentModKeys & SHFCTL) && key == XK_KP_Multiply))
    {
        screen = scoSysInfo.currentVT + 1;
        if (scoSSSetNewVT(screen) == -1)
            (void)scoSSSetNewVT(0);
        return 1;
    }
    return 0;                   /* S030 */
}

static int
scoSSSetNewVT(
              int screen)
{
    int ret;

    if (screen == scoSysInfo.currentVT)
        return 0;
    if (ioctl(scoSysInfo.consoleFd, VT_ACTIVATE, screen) < 0)
        ret = -1;
    else
        ret = 1;

    return ret;
}

void
scoSetFinalSwitchMask()
{
    int i, key_no;

    scoSSMask = 0;
    for (i = 0; i < SCO_MAX_SWKEYS; ++i)
    {
        key_no = 0;
        while (key_no < SCO_SS_KEY_TRANS_SIZE &&
               scoScreenSwitchKeys[i] != scoSSKeyTrans[key_no].char_rep)
            ++key_no;
        if (key_no < SCO_SS_KEY_TRANS_SIZE)
            scoSSMask |= scoSSKeyTrans[key_no].mod_mask;
    }
}

void
scoInitScreenSwitch()
{
    SetSWSequence("ac");        /* S030 */
}

void
SetServerSWSequence(string)
    char *string;
{
    int i;

    if (! string || ! *string)
        return;

    for (i = 0; i < SCO_MAX_SWKEYS; i++)
        scoScreenSwitchKeys[i] = 0;

    for (i = 0 ; *string && i < SCO_MAX_SWKEYS; string++, i++)
    {
        switch(*string)
        {
          case 'a':
          case 'A': 
            scoScreenSwitchKeys[i] = 'a';
            break;
          case 's':
          case 'S': 
            scoScreenSwitchKeys[i] = 's';
            break;
          case 'c':
          case 'C': 
            scoScreenSwitchKeys[i] = 'c';
            break;
        }
        if (! scoScreenSwitchKeys[i])
            break;
    }
    scoSetFinalSwitchMask();
}

/*
 * called from Xsight extensions.
 */
char *
GetSWSequence(len)
    int *len;
{
    *len = strlen(scoScreenSwitchKeys);

    return scoScreenSwitchKeys;
}

/*
 * Set up signals to catch screen switches and tell the 
 * kernel we're in control.
 */
scoEnterVtProcMode()
{
    struct vt_mode smode;
    struct sigaction sigrel, sigacq; /* S003 */
    extern void sco_rel_screen();
    extern void sco_acq_screen();
    int i, ioctl_ret;

    smode.mode = VT_PROCESS;
    smode.waitv = 0;		/* not implemented */
    smode.relsig = SIG_REL;
    smode.acqsig = SIG_ACQ;
    smode.frsig  = SIGINT;      /* not implemented */

    for (i = 0; i < 5; i++)     /* S035 vvvv */
    {
	if ((ioctl_ret = ioctl(scoSysInfo.consoleFd, VT_SETMODE, &smode)) >= 0)
        {
	    break;
        }
	ErrorF("Retry VT_SETMODE....\n");
	nap(1000);
    }
    if (ioctl_ret < 0 ) 
    {
	ErrorF("Set VT_PROCESS fail.\n");
	GiveUp( 1 );
	/* NOT REACHED */
    } /* S035 ^^^^ */

    sigrel.sa_handler = sco_rel_screen; /* S003 vvvv */
    sigfillset(&sigrel.sa_mask);
    sigrel.sa_flags = 0;

    sigacq.sa_handler = sco_acq_screen;
    sigfillset(&sigacq.sa_mask);
    sigacq.sa_flags = 0;

    sigaction(SIG_REL, &sigrel, NULL);
    sigaction(SIG_ACQ, &sigacq, NULL); /* S003 ^^^^ */
    /* signal(SIGINT, catchkillsigs); */

    return;
}

/*
 *  Let someone else handle screen switching
 */
scoLeaveVtProcMode()
{
    struct vt_mode smode;
    struct sigaction sigrel, sigacq; /* S003 vvvv */

    if(scoSysInfo.consoleFd < 0) /* S008 */
        return;

    sigrel.sa_handler = SIG_DFL;
    sigemptyset(&sigrel.sa_mask);
    sigrel.sa_flags = 0;

    sigacq.sa_handler = SIG_DFL;
    sigemptyset(&sigrel.sa_mask);
    sigacq.sa_flags = 0;


    sigaction(SIG_REL, &sigrel, NULL);
    sigaction(SIG_ACQ, &sigacq, NULL); /* S003 ^^^^ */
    /* signal(SIGINT, catchkillsigs); */

    smode.mode = VT_AUTO;
    smode.waitv = 0;		/* not implemented */
    smode.relsig = SIG_REL;
    smode.acqsig = SIG_ACQ;
    smode.frsig  = SIGINT;      /* not implemented */

    if (ioctl(scoSysInfo.consoleFd, VT_SETMODE, &smode) < 0 ) {
#ifdef DEBUG                    /* S011 */
        ErrorF("Set VT_AUTO fail.\n");
#else
        return;
#endif
    }
}

scoReleaseScreen(client)
    ClientPtr client;
{
    int i;
    ScreenPtr pScreen;
    scoScreenInfo *pScoScreen;
    Bool grabScreen = !scoRunSwitched; /* S027 S037 */

#if !defined(usl)
    if(!scoSysInfo.switchEnabled) {
	ioctl(scoSysInfo.consoleFd, VT_RELDISP, VT_FALSE); /* S015 */
	return;
    }
#endif

    for (i = scoSysInfo.numScreens - 1; i >= 0; i--)
    {
	pScoScreen = scoSysInfo.scoScreens[i];
	pScreen = pScoScreen->pScreen;

	/* S002, S003, S005  Call pScreen SaveScreen first */
	(*(pScreen->SaveScreen))(pScreen, SCREEN_SAVER_ON);

	/* 
	 * This must be done here since in R5 the above window will
	 * inherit the grab cursor
	 */
	miPointerSpriteEnable(pScreen, FALSE); /* S038 */

	/* This could change the graphics state */
	if (pScoScreen->exposeScreen)
	    scoSaveScreen(pScreen);

	(*(pScoScreen->SaveGState))(pScreen);

	/* Go to text mode */
	(*(pScoScreen->SetText))(pScreen);

	/* 						 	   S027
	 * If we can't run while switched, then grab the screen.  
	 * If runSwitched is FALSE for any screen, then we must do the grab.
	 */
	if (!grabScreen)        /* S027 */
	    grabScreen = !(pScoScreen->runSwitched); /* S027 */

    }

    if (grabScreen)             /* S027 */
    {
	/* suspend X request processing. */
	if(client)              /* S014 */
	    OnlyListenToOneClient(client); /* Merge is doing a grab */
	else
	    DontListenToAnybody();
    }
    
#if !defined(usl)
    /* flush and suspend event driver.  before the RELDISP	   S013 */
    ev_flush();                 /* S010 */
    ev_suspend();               /* S009 */
#endif /*	! usl	*/

    scoSysInfo.screenActive = FALSE; /* S015 */

    /* S033 */
    scoSSLEDStatus = ioctl(scoSysInfo.consoleFd, KDGETLED, &scoSSLEDState);

#if !defined(usl)
    ioctl(scoSysInfo.consoleFd, VT_RELDISP, VT_TRUE);
#endif /* usl */
}

scoAcquireScreen(client)
    ClientPtr client;
{
    int i;
    ScreenPtr pScreen;
    scoScreenInfo *pScoScreen;
    Bool ungrabScreen = !scoRunSwitched; /* S027 S037 */
    unsigned int led, led_mask;
#ifdef XMERGE                   /* S021 */
    char buf[5];
#endif

#if !defined(usl)
    Bool resumed = TRUE;        /* S034 */


    /* S034 vvv
     * If Merge dies we may lose the event queue all together. 
     * We could try to jump start it, but that's not easy.
     */
    if(ev_resume() == -1)       /* -2 is O.K. */
	resumed = FALSE;        /* deal with it later */		/* S034 ^^^ */


    if (ioctl(scoSysInfo.consoleFd, VT_RELDISP, VT_ACKACQ) < 0)	/* S006 */
        FatalError("Can't acquire screen now!!\n");
#endif /*	! usl	*/



#ifdef XMERGE                   /* S021 */
    if( GetMergeScreenSwitchSeq(buf, scoSysInfo.consoleFd))
        SetServerSWSequence(buf);
#endif

    for (i = scoSysInfo.numScreens - 1; i >= 0; i--)
    {
	pScoScreen = scoSysInfo.scoScreens[i];
	pScreen = pScoScreen->pScreen;

	(*(pScoScreen->SetGraphics))(pScreen);

	(*(pScoScreen->RestoreGState))(pScreen);

	if (pScoScreen->exposeScreen)
	    scoRestoreScreen(pScreen);

	/* S002, S003, S005  Finally unblank via pScreen screen saver */
	(*(pScreen->SaveScreen))(pScreen, SCREEN_SAVER_OFF);

	miPointerSpriteEnable(pScreen, TRUE); /* S038 */

	/* All screens must agree */				/* S027 */
	if (!ungrabScreen)      /* S027 */
	    ungrabScreen = !(pScoScreen->runSwitched); /* S027 */
    }

    if (ungrabScreen)           /* S027 */
    {
	if(client)              /* S014 */
	    ListenToAllClients(); /* Undo the merge grab */
	else
	    PayAttentionToClientsAgain();
    }

#if !defined(usl)
    /*
     * set num, scroll, and caps lock keys to current state as
     * indicated by the LEDs					S033
     */
    if (scoSSLEDStatus >= 0)
	if (ioctl(scoSysInfo.consoleFd, KDGETLED, &led) >= 0)
	    for (i = 0; i < SCO_SS_LED_COUNT; ++i)
	    {
		led_mask = scoSSLED[i].led_mask;
		if ((led & led_mask) != (scoSSLEDState & led_mask))
		{
		    /* S039 !!!! */
                    unsigned int mapcode = 0;
                    switch(scoSSLED[i].led_mask)
                    {
                      case CLKED:
                        mapcode = scoKeysymToKeycode(XK_Caps_Lock);
                        break;
                      case NLKED:
                        mapcode = scoKeysymToKeycode(XK_Num_Lock);
                        break;
                      case SLKED:
                        mapcode = scoKeysymToKeycode(XK_Scroll_Lock);
                        break;
                    }
                    scoSSQueKey(mapcode, DEVE_KBTDOWN);
                    scoSSQueKey(mapcode, DEVE_KBTUP);
		    /* S039 ^^^^ */
		}
	    }

    scoSSCheckMods();


    /* S034 vvv
     * We've lost the event queue, but the only thing wrong is input.
     * So shutdown as gracefully as possible.
     */
    if (!resumed)
    {
        ErrorF("Fatal Error: Lost the the event queue.  Probably due to a Merge problem.\n");
        GiveUp( 1 );
    } /* S034 ^^^ */
#endif /*	! usl	*/

    scoSysInfo.screenActive = TRUE; /* S014 */

}

/*
 * Use this fd for all Screen Switch ioctls			S014
 */
void
scoSetSSFD(int fd)
{
    scoSysInfo.consoleFd = fd;
}

/* S015
 * Become the active screen.
 * If we are not already the active screen, sleep until we are.
 * Use following scoEnterVtProcMode() to get to an initial known
 * screen-switching state.
 */
scoBecomeActiveScreen()
{
    while (ioctl(scoSysInfo.consoleFd, VT_RELDISP, VT_ACKACQ) < 0)
        sleep(2);               /* S019 */
    scoStatusRecord &= ~CatchScreenAcquireSignal;
    scoSysInfo.screenActive = TRUE;
}

/* S037
 * Wait until this screen is active.
 * If we are not already the active screen, sleep until we are,
 * then acquire the screen.
 * Use after screen-switching state has been established.
 */
scoWaitUntilScreenActive()
{
    if (!scoSysInfo.screenActive)
    {
#if defined(usl)				/*	vvv	S044	*/
	extern void scoVTSwitch();

	while (!scoSysInfo.switchReqPending)
		sleep(2);			
	scoVTSwitch();			
#else						/*	^^^	S044 */
        while (!(scoStatusRecord & CatchScreenAcquireSignal))
            sleep(2);
        scoStatusRecord &= ~CatchScreenAcquireSignal;
        scoAcquireScreen(NULL);
#endif						/*	S044	*/
    }
}

/* S015
 * Are we the active screen?
 */
int
scoScreenActive()
{
    return scoSysInfo.screenActive;
}

void sco_rel_screen()
{
    scoStatusRecord |= CatchScreenReleaseSignal; /* S015 */
}

void sco_acq_screen()
{
    scoStatusRecord |= CatchScreenAcquireSignal; /* S015 */

#if !defined(usl)
    /* re-gain event driver and wake up select() */
    ev_resume();                /* S020 */
#endif /*	! usl	*/

}

								/* S017 vvvv */

/*
 *	scoSSCheckMods - Check the modifiers after a switch
 *
 * Check the modifier keys to see if any of them changed state
 * after a screen switch.
 *
 */
static void
scoSSCheckMods()
{
    unsigned char currMods;
    int ret, direction;
    int i;

    ret = ioctl(scoSysInfo.consoleFd, KDGKBSTATE, &currMods);
    if (ret < 0)
        return;
    /*
     * fake the release of all modifier keys
     */
    for (i = 0; i < SCO_SS_KEY_TRANS_SIZE; ++i)
    {
        /* S039 !!! */
        scoSSQueKey(scoKeysymToKeycode(scoSSKeyTrans[i].lkey),
                    DEVE_KBTUP);
        scoSSQueKey(scoKeysymToKeycode(scoSSKeyTrans[i].rkey),
                    DEVE_KBTUP);
        /* S039 ^^^ */
    }
    /*
     * fake press of left modifier keys that are currently set
     */
    for (i = 0; i < SCO_SS_KEY_TRANS_SIZE; ++i)
    {
        if (currMods & scoSSKeyTrans[i].mod_mask)
            direction = DEVE_KBTDOWN;
        else
            direction = DEVE_KBTUP;
        /* S039 !!!! */
        scoSSQueKey(scoKeysymToKeycode(scoSSKeyTrans[i].lkey),
                    direction);
        scoSSQueKey(scoKeysymToKeycode(scoSSKeyTrans[i].rkey),
                    direction);
        /* S039 ^^^^ */
    }

    for (i = 0; i < SCO_SS_KEY_TRANS_SIZE; ++i)
        scoSSKeyTrans[i].checked_since_acquire = 0;
}

/*
 *	scoSSQueKey - Queue a key up on the event queue
 *
 *	Take a key event and put it on the input queue.  This is done
 *	so that we can fake any key events we might have lost during
 *	screen switching.
 */
static
void
scoSSQueKey(key, up_down_state)
    unsigned int key;	/* S039 */
    int up_down_state;
{

#if defined(usl)
    scoProcessKbdEvent(key, up_down_state);
#else
    devEvent pE;

    pE.deve_key = key;

    pE.deve_direction = up_down_state;
    pE.deve_type = DEVE_BUTTON;
    pE.deve_device = DEVE_DKB;
    pE.deve_time = GetTimeInMillis();

    ProcessEvent(&pE);
#endif /* usl */
}

#if defined(usl)
void
scoVTSwitch()
{
  scoSysInfo.switchReqPending = FALSE;

  if(!scoSysInfo.switchEnabled)
    {
      return;
    }

  else if (scoSysInfo.reset)
    {
      if (!scoSysInfo.screenActive)
        scoSysInfo.screenActive = scoVTSwitchTo();
    }

  else if (scoScreenActive())
    {
      DisableDevice((DeviceIntPtr)scoSysInfo.pKeyboard);
      DisableDevice((DeviceIntPtr)scoSysInfo.pPointer);
      scoReleaseScreen(NULL);
      scoVTSwitchAway();
    }

  else
    {
      scoVTSwitchTo();
      EnableDevice((DeviceIntPtr)scoSysInfo.pKeyboard);
      EnableDevice((DeviceIntPtr)scoSysInfo.pPointer);
      scoAcquireScreen(NULL);
    }
}

int
scoVTSwitchAway()
{
  if (ioctl(scoSysInfo.consoleFd, VT_RELDISP, 1) < 0)
    return(FALSE);
  else
    return(TRUE);
}

int
scoVTSwitchTo()
{
  if (ioctl(scoSysInfo.consoleFd, VT_RELDISP, VT_ACKACQ) < 0)
    return(FALSE);
  else
    return(TRUE);
}

#endif	/* usl */
