/*
 *	@(#) scoKbd.c 11.2 97/11/13 
 *
 *	Copyright (C) The Santa Cruz Operation, 1990-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * scoKbd.c
 *	Functions for retrieving data from a keyboard.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	??? Oct ?? ??:??:?? PST 1990	mikep@sco.com
 *	- Created file from sunKbd.c
 *
 *	S001	Sat Feb 09 22:34:14 PST 1991	mikep@sco.com
 *	- Added Lock key functionality, scancode translations and
 *	deleted SUNJUNK
 *
 *	S002	Mon Feb 11 21:29:24 PST 1991	mikep@sco.com
 *      - Added autoRepeat handling.
 *	- Replaced sysKbCtrl with pPriv->ctrl
 *	- Deleted scoBlock and scoWakeup Handlers.
 *
 *	S003	Tue Feb 12 13:58:57 PST 1991	mikep@sco.com
 *	- Fixed the bell. (Implemented KDMKTONE in 3.2v3)
 *
 *	S004	Thu Feb 28 01:43:27 PST 1991	buckm@sco.com
 *	- Use stdin if GetCrtName() returns null.
 *
 *	S005	Fri Mar 08 16:56:17 PST 1991	mikep@sco.com
 *	- Added #ifdef v3 and put old 3.2v2 bell code.
 *
 *	S006	Thu Mar 28 13:54:00 PST 1991	staceyc@sco.com
 * 	- added/reworked Xsight.cfg stuff
 *
 *	S007	Tue Apr 02 16:51:29 PST 1991	mikep@sco.com
 *	- Added screen switch structures.  Very little code as
 *	of yet.
 *
 *	S008	Tue Apr 02 22:10:25 PST 1991	mikep@sco.com
 *	- Change mwStatusRecord to scoStatusRecord.  Removed mw.h
 *	- Removed mwScreenAccessAllowed
 *	- Made modifications necessary for new screen switching
 *
 *	S009	Mon Apr 08 02:36:56 PDT 1991	mikep@sco.com
 *	- Completely rewrote AltSysReq code.  These routines
 *	need to do a lookup in the modifier table.  See FIX ME!
 *
 *	S010	Mon Apr 08 18:02:39 PDT 1991	mikep@sco.com
 *	- Added calls to ignore clients while handling AltSysReq.
 *
 *	S011	Wed Apr 10 18:51:38 PDT 1991	buckm@sco.com
 *	- Force keyboard fd to be stdin.
 *	- Don't close keyboard fd on DEVICE_CLOSEs.
 *
 *	SAAA	Wed May 08 02:31:05 PDT 1991	mikep@sco.com
 *	- Hack to get Merge linked in.  This file was just convienent
 *	This code should disappear.
 *
 *	S012	Wed May 08 18:39:55 PDT 1991	buckm@sco.com
 *	- Back to ifdef'ing KDMKTONE code on v3 in scoKbdBell;
 *	it was a no-op in v2, not an error.
 *
 *	S013	Thu May 09 12:19:15 PDT 1991	buckm@sco.com
 *	- No need for screen active checks here.
 *
 *	S014	Fri May 10 17:14:58 PDT 1991	staceyc@sco.com
 *	- Move call to screen switch filter up a bit.
 *
 *	S015	Thu May 30 09:45:29 PDT 1991	buckm@sco.com
 *	- If stdin is not the console, then try dup'ing stdout.
 *	If we were started in the background, the shell may have
 *	re-directed our stdin to /dev/null for us.
 *
 *	S016	Wed Aug 07 13:25:41 PDT 1991	mikep@sco.com
 *	- Added XTEST Extension.
 *
 *	S017	Thu Aug 08 16:17:43 PDT 1991	mikep@sco.com
 *	- Modified Lock key processing to switch off of Keysyms (yea!!!!)
 *
 *	S018	Thu Sep 19 13:43:34 PDT 1991	mikep@sco.com
 *	- Ignore right shifts in the translation table as well.
 *
 *	S019	Fri Sep 27 16:26:55 1991	staceyc@sco.com
 *	- pass keysyms to screen switch filter
 *
 *	S020	Mon Sep 30 21:34:54 PDT 1991	mikep@sco.com
 *	- Don't allow Alt-SysReq when the KEYBOARD is grabbed.
 *	Should we nix screen switching as well?
 *
 *	S021	Sat Oct 12 13:12:30 PDT 1991	mikep@sco.com
 *	- lastkey needs to be a KeyCode.  (was a char which was bad
 *	for key code past 128.
 *
 *	S022	Sun Oct 27 14:13:52 PST 1991	mikep@sco.com
 *	- Yikes! Don't forget to put a timestamp on our keyboard events!!! 
 *
 *	S023	Tue Sep 29 17:16:09 PDT 1992	mikep@sco.com
 *	- Use 3.2v4 Bell style.  Note this won't work on 3.2v2 but it saves
 *	  the server from a busy loop.
 *	- Call mieqEnqueue() now instead of passing events directly to DIX.
 *      - Change the name of scoKbdProcessEvent to scoKbdEnqueueEvent
 *	  since that's what it does now.
 *	- Remove dead code left over from sunKbd.c
 *
 *	S024	Tue Nov 17 18:27:26 PST 1992	hess@sco.com
 *	- check for XK_y as well as XK_Y after user hits AltSysReq, this test
 *    	was failing on intl keyboards.
 *
 *	S025	Wed Nov 25 09:26:30 PST 1992	buckm@sco.com
 *	- The initial Crt mucking from scoKbdOpen() is now unified
 *	  over in scoInitCrt() in scoInit.c.
 *
 *	S026	Sat Dec 05 13:13:41 PST 1992	mikep@sco.com
 *	- Add support Xsight Extension Screen Switch and AltSys-Req
 *	  controls.  The check on whether the keyboard is grabbed or
 *	  not is still very questionable, in any case, start processing
 *	  the event when the server doesn't want it.  This isn't completely
 *	  implemented yet.
 *
 *	S027	Thu Dec 17 15:35:47 PST 1992	mikep@sco.com
 *	- Support the Shift_Lock key
 *
 *	S028	Fri Jan 15 12:39:23 PST 1993	mikep@sco.com
 *	- Our granularity is only 1/10th of a second, so make sure that
 *	anything > 0 makes a noise.
 *	
 *      S029    Mon Jul 12 10:22:35 PDT 1993    davidw@sco.com
 *      - Removed y/n confirmation for Alt-SysReq (S024), exit with 
 *	  Return now.  Updated scoPopMessage exit prompt message.
 *	  Added multi-lined strings ability using "\n" embedding.
 *
 *      S030    Tue Jul 20 09:55:11 PDT 1993    davidw@sco.com
 *      - Added XPG4 Message Catalogue support - cataloged some messages.
 *
 *	S031	Thu Aug 26 18:56:26 PDT 1993	mikep@sco.com
 *	- Fixed a typo found in code review
 *
 *      S032    Fri Aug 27 13:02:32 PDT 1993    davidw@sco.com
 *      - Add XK_KP_Enter to Alt_Sysreq confirmation keys.
 *
 *      S033    Tue Sep 21 13:02:32 PDT 1993    edb@sco.com
 *	- Use scancode API
 *
 *      S034	Mon Oct 10 10:02:33 1994	kylec@sco.com
 *	- Add scoKeysymToKeycode() - this function allows us to 
 *	  remove any hardcoded scancode values used by Xsco.
 *	  Fixes to BUG SCO-59-4154 and BUG SCO-59-4155.
 *	- Scancode translations (scoScanTranslate) should no
 *	  longer be accessed in scoKbdEnqueueEvent().  Use
 *	  of this table is moved to scoIo.c.  BUG SCO-59-4153.
 *
 *	S035	Tue Feb 27 14:45:37 PST 1996	hiramc@sco.COM
 *	- Enabled kbd and mouse processing on gemini, changes not
 *	  marked, too extensive.
 *
 * 	S036, Tue Sep 24 13:13:43 PDT 1996, kylec
 * 	- Re-wrote scoKbdEnqueueEvent()
 * 	- Misc fixes in KB101Map[]
 *	- Set up default built in scancode translations in
 *	  defaultScanTranslate[].
 *
 *      S037, Thu Oct  3 18:16:54 PDT 1996, kylec
 *	- Re-wrote alot of the code.
 *	- Added support for XKB extension.
 *	- Removed alot of unused code.
 *
 *	S038, Mon Oct  7 11:35:31 PDT 1996, kylec
 *	- Get default XKEYBOARD extension config data from .Xsco.cfg.
 *
 *	S039, Tue Oct 15 13:23:34 PDT 1996, kylec@sco.com
 *	- Allow Ctrl+Alt+XK_BackSpace or XK_Terminate_Server keystrokes
 *	  to terminate server.
 *	- Disable Alt-SysReq functionality.
 *	- Confirmation dialog is no longer displayed.
 *
 *	S040, Tue Jan 21 16:08:30 PST 1997, kyle@sco.com
 *	- Don't restore keyboard unless graphics screen is active.
 *	S041	Fri Jun 20 10:40:34 PDT 1997	hiramc@sco.COM
 *	- fixing memory leak on reset cycle
 *	S042	Wed Jul  2 13:23:00 PDT 1997	hiramc@sco.COM
 *	- that fix caused many days of delays to BL10, removed
 *	S043	Thu Nov 13 15:56:38 PST 1997	hiramc@sco.COM
 *	- now have -noexit command line argument to disable
 *	- the Ctrl-Alt-Backspace key.
 *	
 */


#define NEED_EVENTS

#include "sco.h"
#include "scoModKeys.h"
#include <stdio.h>
#include <errno.h>
#include <stropts.h>
#include <sys/page.h>
#include <sys/sysmacros.h>
#include <sys/param.h>
#include <sys/event.h>

#if defined(usl)
#include <sys/vt.h>
#include <sys/kd.h>
#include <xsconfig/config.h>
#else
#include <sys/vtkd.h>
#include <mouse.h>
#endif	/* usl */

#ifndef v3						/* S005 */
#include <sys/times.h>
#endif

#include "Xproto.h"
#include "keysym.h"
#include "input.h"
#include "inputstr.h"
#include "pckeys.h"
#include "xsrv_msgcat.h"

#ifdef XKB
#include <X11/extensions/XKB.h>
#include <X11/extensions/XKBstr.h>
#include <X11/extensions/XKBsrv.h>
extern Bool noXkbExtension;
#endif

#if defined(usl)
extern Bool NoCtrlAltExit;	/* 0 == exit OK, 1 no exit allowed S043	*/
#else
extern scoEventQueue *scoQueue;
extern Bool XsightScreenSwitch, XsightAltSysReq;
#endif	/* ! usl */

extern void	ProcessInputEvents();
extern void	miPointerPosition();
extern void	scoLoadConfiguration();
static void 	scoKbdBell();
static void 	scoKbdCtrl();
static void     scoKbdEnqueueEvent();

KeyCode 	scoKeysymToKeycode ();

#ifdef XTESTEXT1                /* S016 */
/*
 * defined in xtest1dd.c
 */
extern int      on_steal_input;
extern Bool     XTestStealKeyData();
#endif /* XTESTEXT1 */

#define ModifierDown(k) ((keyc->state & (k)) == (k))
#define KeyPressed(k) 	(keyc->down[k >> 3] & (1 << (k & 7)))

#define X_CAPS_LOCK	0x01
#define X_SHIFT_LOCK	0x02
#define X_NUM_LOCK	0x04
#define X_SCROLL_LOCK	0x08
#define X_KANA_LOCK	0x10

#define X_LOCK_KEY(st, lk) \
(((st) & (lk)) ? (((st) &= ~(lk)) && 0) : ((st) |= (lk)))

#define CONFIRM_EXIT_MSG 	"To exit, press <Enter>.\n" \
"To cancel, press any other key."

static KeySym scoDefaultKeyboardMap[] =
{
    XK_Escape,	      NoSymbol, /* 1: */  
    XK_1,	      XK_exclam, /* 2: */          
    XK_2,	      XK_at,    /* 3: */          
    XK_3,	      XK_numbersign, /* 4: */          
    XK_4,	      XK_dollar, /* 5: */          
    XK_5,	      XK_percent, /* 6: */          
    XK_6,	      XK_asciicircum, /* 7: */          
    XK_7,	      XK_ampersand, /* 8: */          
    XK_8,	      XK_asterisk, /* 9: */          
    XK_9,	      XK_parenleft, /* 10: */         
    XK_0,	      XK_parenright, /* 11: */         
    XK_minus,	      XK_underscore, /* 12: */ 
    XK_equal,	      XK_plus,  /* 13: */ 
    XK_BackSpace,     NoSymbol, /* 14: */         
    XK_Tab,	      NoSymbol, /* 15: */         
    XK_Q,	      NoSymbol, /* 16: */         
    XK_W,	      NoSymbol, /* 17: */         
    XK_E,	      NoSymbol, /* 18: */         
    XK_R,	      NoSymbol, /* 19: */         
    XK_T,	      NoSymbol, /* 20: */         
    XK_Y,	      NoSymbol, /* 21: */         
    XK_U,	      NoSymbol, /* 22: */         
    XK_I,	      NoSymbol, /* 23: */         
    XK_O,	      NoSymbol, /* 24: */         
    XK_P,	      NoSymbol, /* 25: */         
    XK_bracketleft,   XK_braceleft, /* 26: */         
    XK_bracketright,  XK_braceright, /* 27: */     
    XK_Return,	      NoSymbol, /* 28: */ 
    XK_Control_L,     NoSymbol, /* 29: */         
    XK_A,	      NoSymbol, /* 30: */         
    XK_S,	      NoSymbol, /* 31: */         
    XK_D,	      NoSymbol, /* 32: */         
    XK_F,	      NoSymbol, /* 33: */         
    XK_G,	      NoSymbol, /* 34: */         
    XK_H,	      NoSymbol, /* 35: */         
    XK_J,	      NoSymbol, /* 36: */         
    XK_K,	      NoSymbol, /* 37: */         
    XK_L,	      NoSymbol, /* 38: */         
    XK_semicolon,     XK_colon, /* 39: */         
    XK_quoteright,    XK_quotedbl, /* 40: */         
    XK_quoteleft,     XK_asciitilde, /* 41: */         
    XK_Shift_L,	      NoSymbol, /* 42: */ 
    XK_backslash,     XK_bar,   /* 43: */         
    XK_Z,	      NoSymbol, /* 44: */         
    XK_X,	      NoSymbol, /* 45: */         
    XK_C,	      NoSymbol, /* 46: */         
    XK_V,	      NoSymbol, /* 47: */         
    XK_B,	      NoSymbol, /* 48: */         
    XK_N,	      NoSymbol, /* 49: */         
    XK_M,	      NoSymbol, /* 50: */         
    XK_comma,	      XK_less,  /* 51: */ 
    XK_period,	      XK_greater, /* 52: */ 
    XK_slash,	      XK_question, /* 53: */ 
    XK_Shift_R,	      NoSymbol, /* 54: */ 
    XK_KP_Multiply,   NoSymbol, /* 55: */         
    XK_Alt_L,	      NoSymbol, /* 56: */ 
    XK_space,	      NoSymbol, /* 57: */ 
    XK_Caps_Lock,     NoSymbol, /* 58: */         
    XK_F1,	      NoSymbol, /* 59: */         
    XK_F2,	      NoSymbol, /* 60: */         
    XK_F3,	      NoSymbol, /* 61: */         
    XK_F4,	      NoSymbol, /* 62: */         
    XK_F5,	      NoSymbol, /* 63: */         
    XK_F6,	      NoSymbol, /* 64: */         
    XK_F7,	      NoSymbol, /* 65: */         
    XK_F8,	      NoSymbol, /* 66: */         
    XK_F9,	      NoSymbol, /* 67: */         
    XK_F10,	      NoSymbol, /* 68: */         
    XK_Num_Lock,      NoSymbol, /* 69: */         
    XK_Scroll_Lock,   NoSymbol, /* 70: */         
    XK_KP_Home,	      XK_KP_7,  /* 71: */ 
    XK_KP_Up,	      XK_KP_8,  /* 72: */ 
    XK_KP_Prior,      XK_KP_9,  /* 73: */         
    XK_KP_Subtract,   NoSymbol, /* 74: */         
    XK_KP_Left,	      XK_KP_4,  /* 75: */ 
    XK_KP_Begin,      XK_KP_5,  /* 76: */         
    XK_KP_Right,      XK_KP_6,  /* 77: */         
    XK_KP_Add,	      NoSymbol, /* 78: */ 
    XK_KP_End,	      XK_KP_1,  /* 79: */ 
    XK_KP_Down,	      XK_KP_2,  /* 80: */ 
    XK_KP_Next,	      XK_KP_3,  /* 81: */ 
    XK_KP_Insert,     XK_KP_0,  /* 82: */         
    XK_KP_Delete,     XK_KP_Decimal, /* 83: */         
    NoSymbol,	      NoSymbol, /* 84: */ 
    NoSymbol,	      NoSymbol, /* 85: */ 
    NoSymbol,	      NoSymbol, /* 86: */ 
    XK_F11,	      NoSymbol, /* 87: */         
    XK_F12,	      NoSymbol, /* 88: */         
    NoSymbol,	      NoSymbol, /* 89: */ 
    NoSymbol,	      NoSymbol, /* 90: */ 
    NoSymbol,	      NoSymbol, /* 91: */ 
    NoSymbol,	      NoSymbol, /* 92: */ 
    NoSymbol,	      NoSymbol, /* 93: */ 
    NoSymbol,	      NoSymbol, /* 94: */ 
    NoSymbol,	      NoSymbol, /* 95: */ 
    NoSymbol,	      NoSymbol, /* 96: */ 
    NoSymbol,	      NoSymbol, /* 97: */ 
    NoSymbol,	      NoSymbol, /* 98: */ 
    NoSymbol,	      NoSymbol, /* 99: */ 
    NoSymbol,	      NoSymbol, /* 100: */
    NoSymbol,	      NoSymbol, /* 101: */
    NoSymbol,	      NoSymbol, /* 102: */
    NoSymbol,	      NoSymbol, /* 103: */
    NoSymbol,	      NoSymbol, /* 104: */
    NoSymbol,	      NoSymbol, /* 105: */
    NoSymbol,	      NoSymbol, /* 106: */
    NoSymbol,	      NoSymbol, /* 107: */
    NoSymbol,	      NoSymbol, /* 108: */
    NoSymbol,	      NoSymbol, /* 109: */
    NoSymbol,	      NoSymbol, /* 100: */
    NoSymbol,	      NoSymbol, /* 111: */
    NoSymbol,	      NoSymbol, /* 112: */
    NoSymbol,	      NoSymbol, /* 113: */
    NoSymbol,	      NoSymbol, /* 114: */
    NoSymbol,	      NoSymbol, /* 115: */
    NoSymbol,	      NoSymbol, /* 116: */
    NoSymbol,	      NoSymbol, /* 117: */
    NoSymbol,	      NoSymbol, /* 118: */
    NoSymbol,	      NoSymbol, /* 119: */
    NoSymbol,	      NoSymbol, /* 120: */
    NoSymbol,	      NoSymbol, /* 121: */
    NoSymbol,	      NoSymbol, /* 122: */
    NoSymbol,	      NoSymbol, /* 123: */
    NoSymbol,	      NoSymbol, /* 124: */
    NoSymbol,	      NoSymbol, /* 125: */
    NoSymbol,	      NoSymbol, /* 126: */
    NoSymbol,	      NoSymbol, /* 127: */
    XK_Control_R,     NoSymbol, /* 128: */       
    XK_Alt_R,	      NoSymbol, /* 129: */
    XK_Insert,	      NoSymbol, /* 130: */
    XK_Delete,	      NoSymbol, /* 131: */
    XK_Home,	      NoSymbol, /* 132: */
    XK_End,	      NoSymbol, /* 133: */       
    XK_Prior,	      NoSymbol, /* 134: */
    XK_Next,	      NoSymbol, /* 135: */
    XK_Right,	      NoSymbol, /* 136: */
    XK_Left,	      NoSymbol, /* 137: */
    XK_Up,	      NoSymbol, /* 138: */       
    XK_Down,	      NoSymbol, /* 139: */
    XK_KP_Divide,     NoSymbol, /* 140: */       
    XK_KP_Enter,      NoSymbol, /* 141: */       
    NoSymbol,	      NoSymbol, /* 142: */
    XK_Sys_Req,       XK_Print, /* 143: */
    XK_Break,	      XK_Pause, /* 144: */
    NoSymbol,	      NoSymbol, /* 145: */

};                              /* scoDefaultKeyboardMap */

#define KB101_MIN_KEY  		0
#define KB101_MAX_KEY 		143
#define KB101_GLYPHS_PER_KEY 	2		

static KeySymsRec scoDefaultKeySyms =
{
    scoDefaultKeyboardMap,
    KB101_MIN_KEY + MIN_KEYCODE,
    KB101_MAX_KEY + MIN_KEYCODE,
    KB101_GLYPHS_PER_KEY
};



static ScanTranslation scoDefaultTranslations[] = {

    /*  XT mode */
    0xe01d, 128,		/* Control_R */
    0xe038, 129,		/* Alt_R */
    0xe052, 130,		/* Insert */
    0xe053, 131,		/* Delete */
    0xe047, 132,		/* Home */
    0xe04f, 133,		/* End */
    0xe049, 134,		/* Prior */
    0xe051, 135,		/* Next */
    0xe04d, 136,		/* Right */
    0xe04b, 137,		/* Left */
    0xe048, 138,		/* Up */
    0xe050, 139,		/* Down */
    0xe035, 140,		/* KP_Divide */
    0xe01c, 141,		/* KP_Enter */
    0xe037, 143,                /* Sys_Req / Print */
    0xe11d, 0xe1,		/* Lead in control key before pause */
    0xe145, 144,                /* Pause key */
    0xe046, 144,                /* Pause key when ctrl key is pressed */
    0xe02a, 0,                  /* IGNORE simulated left shift */
    0xe036, 0,                  /* IGNORE simulated right shift */

#if defined(AT_TRANSLATIONS)
    /* AT mode */
    0xe07c, 143,                /* Sys_Req / Print (AT mode) */
    0xe114, 0xe1,               /* Lead in control key before pause (AT mode) */
    0xe177, 144,                /* Pause key (AT mode) */
#endif

};

#if defined(AT_TRANSLATIONS)
static int scoNumDefaultTranslations = 23;
#else
static int scoNumDefaultTranslations = 20;
#endif

static CARD8 *scoDefaultModifierMap = 0;


/*
 *-----------------------------------------------------------------------
 * scoSetKeyboardDefaults --
 *	Sets up default mapping for keyboard in absence of .Xsco.cfg.
 *
 * Results:
 *	Calculates scoDefaultModifierMap from scoDefaultKeyboardMap.
 *	Default KeySyms and scancode translations are hard coded
 * 	in table above (see scoDefaultKeyboardMap, and defaultScanTranslations).
 *
 *-----------------------------------------------------------------------
 */

void
scoSetKeyboardDefaults ()
{
    extern CARD8 *scoDefaultModifierMap;
    register int i, j;

    if (!scoDefaultModifierMap)
        scoDefaultModifierMap = (CARD8 *)Xalloc(MAP_LENGTH * sizeof(CARD8));

    for (i=0; i<MAP_LENGTH; i++)
    {
        scoDefaultModifierMap[i] = NoSymbol;
    }

    for (i=0, j=MIN_KEYCODE; i<=KB101_MAX_KEY; i+=KB101_GLYPHS_PER_KEY, j++)
    {
        switch(scoDefaultKeyboardMap[i])
        {
        case XK_Caps_Lock:
            scoDefaultModifierMap[j] = LockMask;
            break;

        case XK_Shift_L:
        case XK_Shift_R:
            scoDefaultModifierMap[j] = ShiftMask;
            break;

        case XK_Control_L:
        case XK_Control_R:
            scoDefaultModifierMap[j] = ControlMask;
            break;

        case XK_Alt_L:
        case XK_Alt_R:
            scoDefaultModifierMap[j] = Mod1Mask;
            break;

        case XK_Num_Lock:
            scoDefaultModifierMap[j] = Mod2Mask;
            break;

        case XK_Mode_switch:
            scoDefaultModifierMap[j] = Mod3Mask;
            break;

        default:
            break;
        }
    }

}



/*
 *-----------------------------------------------------------------------
 * scoKbdCtrl --
 *	Funtion to control keyboard, leds, repeat keys, etc.
 *
 * Results:
 *	Programs leds if their state changes.
 *
 *-----------------------------------------------------------------------
 */

static void
scoKbdCtrl (DeviceIntPtr pKeyboard,
            KeybdCtrl *ctrl)
{
    static unsigned int lastGeneration = (unsigned int)-1;
    KbPrivPtr		pPriv;

#ifdef DEBUG
    ErrorF("scoKbdCtrl()\n");
#endif

    pPriv = (KbPrivPtr) pKeyboard->public.devicePrivate;

    if ((pPriv->ctrl.leds != ctrl->leds) ||
        (lastGeneration != serverGeneration))
    {
#ifdef DEBUG
        ErrorF("LEDS(%#x)\n", ctrl->leds);
#endif
        pPriv->ctrl.leds = ctrl->leds;
        ioctl(pPriv->fd, KDSETLED, pPriv->ctrl.leds);
    }

    lastGeneration = serverGeneration;

}


/*
 *-----------------------------------------------------------------------
 * scoKbdProc --
 *	Handle the initialization, etc. of a keyboard.
 *
 *-----------------------------------------------------------------------
 */

int
scoKbdProc (DeviceIntPtr pKeyboard, /* Keyboard to manipulate */
            int what)              /* What to do to it */
{
  KeySymsRec *keySyms;
  CARD8 *modMap;
  int i;
  xEvent		xE;
  KeyClassRec *keyc = pKeyboard->key;
  KbPrivPtr	pPriv = (KbPrivPtr)pKeyboard->public.devicePrivate;
#ifdef XKB
  XkbComponentNamesPtr xkbNames;
#endif

  switch (what) 
    {
    case DEVICE_INIT: 

#ifdef DEBUG
      ErrorF("DEVICE_INIT(keyboard)\n");
#endif
      pPriv = (KbPrivPtr)Xalloc(sizeof(KbPrivRec));
      pPriv->EnqueueEvent = scoKbdEnqueueEvent;
      pPriv->fd = scoSysInfo.consoleFd;
      pPriv->lockState = 0;
      pPriv->offset = 0;
      pPriv->ctrl = defaultKeyboardControl; /* struct copy */

      pKeyboard->public.devicePrivate = (pointer)pPriv;

      SaveTtyMode(pPriv->fd);

      /* load keyboard config and use defaults if .Xsco.cfg is missing */
      scoLoadConfiguration(); /* read .Xsco.cfg */
      scoGetKeyboardMappings(&keySyms, &modMap,
#ifdef XKB
                             &xkbNames,
#endif
                             &scoDefaultKeySyms,
                             scoDefaultModifierMap,
                             scoDefaultTranslations,
                             scoNumDefaultTranslations);

      /* ensure that the keycodes on the wire are >= MIN_KEYCODE */
      if (keySyms->minKeyCode < MIN_KEYCODE) {
        int offset = MIN_KEYCODE - keySyms->minKeyCode;

        keySyms->minKeyCode += offset;
        keySyms->maxKeyCode += offset;
        pPriv->offset = offset;
      }

#ifdef XKB
      if (noXkbExtension)
        {
          InitKeyboardDeviceStruct((DevicePtr)pKeyboard,
                                   keySyms, modMap,
                                   scoKbdBell, scoKbdCtrl);
        }
      else
        {
          XkbInitKeyboardDeviceStruct(pKeyboard, 
                                      xkbNames, keySyms, modMap, 
                                      scoKbdBell, scoKbdCtrl);
        }
#else
      InitKeyboardDeviceStruct((DevicePtr)pKeyboard, keySyms,
                               modMap, scoKbdBell, scoKbdCtrl);
#endif /* XKB */

      break;


    case DEVICE_ON: 

#ifdef DEBUG
      ErrorF("DEVICE_ON(keyboard)\n");
#endif
      xE.u.keyButtonPointer.time = NewInputEventTime();
      for (i = keyc->curKeySyms.minKeyCode;
           i < keyc->curKeySyms.maxKeyCode; i++) {
        if (KeyPressed(i)) {
          xE.u.u.detail = i;
          xE.u.u.type = KeyRelease;
          (*pKeyboard->public.processInputProc)(&xE, pKeyboard, 1);
        }
      }
      SetTtyRawMode(pPriv->fd);
      XqueEnable();
      pKeyboard->public.on = TRUE;
      break;

    case DEVICE_CLOSE: 

#ifdef DEBUG
      ErrorF("DEVICE_CLOSE(keyboard)\n");
#endif

    case DEVICE_OFF: 

#ifdef DEBUG
      ErrorF("DEVICE_OFF(keyboard)\n");
#endif
      pKeyboard->public.on = FALSE;
      XqueDisable();
      if (scoScreenActive())    /* S040 */
        RestoreTtyMode(pPriv->fd);
#ifdef NOT
	if ( pKeyboard->public.devicePrivate ) {	/*	S041	*/
		xfree ( pKeyboard->public.devicePrivate );	/* S041 */
	}							/* S041 */
#endif
      break;

    }
  return (Success);
}


/*
 *-----------------------------------------------------------------------
 * scoKbdEnqueueEvent -- Convert a keycode into an X event and pass it
 *			 off to mi.
 *
 *-----------------------------------------------------------------------
 */

static void
scoKbdEnqueueEvent (pKeyboard, keycode, down)
DeviceIntPtr pKeyboard;
KeyCode keycode;
Bool down;
{
    xEvent 			xE;
    KeyClassRec 		*keyc;
    KbPrivPtr			pPriv;
    KeySym 			keySym;
    extern InputInfo    	inputInfo;          

    if ((keycode == 0) || (!pKeyboard->public.on))
        return;

    pPriv = (KbPrivPtr) pKeyboard->public.devicePrivate;
    keyc = pKeyboard->key;


#ifdef XTESTEXT1                   /* S016 */
    if (on_steal_input)
    {
        int x, y;
        miPointerPosition(&x, &y);
        /* 
         * XTestStealKeyData() returns true if this event should be sent on
         * to DIX.  Note this means that the LEDS won't change if we are in
         * pure steal input mode (should they?).  The auto repeat and Alt Sys
         * Req code also gets skipped.   Does it really matter?
         */
        if(! XTestStealKeyData(keycode,
                               down ? KeyPress : KeyRelease, 
                               DEVE_DKB, x, y))
            return;
    }
#endif /* XTESTEXT1 */

#ifdef XKB
    if (noXkbExtension)
    {
#endif        
        keySym = *(keyc->curKeySyms.map + keyc->curKeySyms.mapWidth * 
                   (keycode - keyc->curKeySyms.minKeyCode));

        
        if (down)                  /* if KeyPress */
        {
#ifdef ALTSYS_REQ
            if (scoStatusRecord & AltSysRequest)
            {
                return;
            }
#endif            
                
            /*
             *  Autorepeat is implemented in the keyboard.  So, watch
             *  for repeating makes (press events).  If we see two makes
             *  in a row, we need to check it against the KB control
             *  structure.
             */
                
            if (keycode == pPriv->lastKey)
            {
                int i, mask;

                if (pPriv->ctrl.autoRepeat == FALSE)
                    return;

                /*
                 * Is this key repeating?  autoRepeats is a
                 * bitmap representing every key on the keyboard
                 */
                i = (pPriv->lastKey >> 3);
                mask = (1 << (pPriv->lastKey & 7));
                if ((mask & pPriv->ctrl.autoRepeats[i]) == 0)
                    return;
            }
                
            switch (keySym)        /* handle locking keys */
            {

#ifdef ALTSYS_REQ
            case XK_Sys_Req: 
                
                if (inputInfo.keyboard->sync.state == NOT_GRABBED)
                    keycode = 0;   /* ignore */

                break;
#endif

#ifndef NO_TERMINATE_KEYS

            case XK_BackSpace:

                /* Allow BackSpace keystroke to terminate server. */
                /* Backspace requires keyboard state 0xC to be set. */
                /* This state usually maps to modifiers Ctrl + Alt. */
                /* Can be changed via xsconfig.*/
                
#define     BACKSPACE_MODS	0xC /* Ctrl + Alt usually */

                if ((keyc->state & BACKSPACE_MODS) != BACKSPACE_MODS)
                    break;

                /* else fall through */
#endif
                
            case XK_Terminate_Server:

                if (inputInfo.keyboard->sync.state == NOT_GRABBED) {
		    if ( ! NoCtrlAltExit )	/*	S043	*/
                    	GiveUp(1);
		}

                break;

                
            case XK_Caps_Lock: 
                
                if (pPriv->lastKey == keycode)
                    keycode = 0;   /* ignore KeyPress */
                else if (X_LOCK_KEY(pPriv->lockState, X_CAPS_LOCK))
                {
                    pPriv->ctrl.leds |= LED_CAP;
                    ioctl(pPriv->fd, KDSETLED, pPriv->ctrl.leds);
                }
                else
                {
                    pPriv->ctrl.leds &= ~LED_CAP;
                    ioctl(pPriv->fd, KDSETLED, pPriv->ctrl.leds);
                    down = 0;      /* simulate KeyRelease */
                }

                break;


            case XK_Shift_Lock:

                if (pPriv->lastKey == keycode)
                    keycode = 0;   /* ignore KeyPress */
                else if (!X_LOCK_KEY(pPriv->lockState, X_SHIFT_LOCK))
                {
                    down = 0;      /* simulate KeyRelease */
                }
                break;     


            case XK_Num_Lock:

                if (pPriv->lastKey == keycode)
                    keycode = 0;   /* ignore KeyPress */
                else if (X_LOCK_KEY(pPriv->lockState, X_NUM_LOCK))
                {
                    pPriv->ctrl.leds |= LED_NUM;
                    ioctl(pPriv->fd, KDSETLED, pPriv->ctrl.leds);
                }
                else
                {
                    pPriv->ctrl.leds &= ~LED_NUM;
                    ioctl(pPriv->fd, KDSETLED, pPriv->ctrl.leds);
                    down = 0;      /* simulate KeyRelease */
                }
                break;

                    
            case XK_Scroll_Lock: 

                if (pPriv->lastKey == keycode)
                    keycode = 0;   /* ignore KeyPress */
                else if (X_LOCK_KEY(pPriv->lockState, X_SCROLL_LOCK))
                {
                    pPriv->ctrl.leds |= LED_SCR;
                    ioctl(pPriv->fd, KDSETLED, pPriv->ctrl.leds);
                }
                else
                {
                    pPriv->ctrl.leds &= ~LED_SCR;
                    ioctl(pPriv->fd, KDSETLED, pPriv->ctrl.leds);
                    down = 0;      /* simulate KeyRelease */
                }
                break;

            case XK_Kana_Lock:

                if (pPriv->lastKey == keycode)
                    keycode = 0;   /* ignore KeyPress */
                else if (X_LOCK_KEY(pPriv->lockState, X_KANA_LOCK))
                {
                    pPriv->ctrl.leds |= LED_KANA;
                    ioctl(pPriv->fd, KDSETLED, pPriv->ctrl.leds);
                }
                else
                {
                    pPriv->ctrl.leds &= ~LED_KANA;
                    ioctl(pPriv->fd, KDSETLED, pPriv->ctrl.leds);
                    down = 0;      /* simulate KeyRelease */
                }
                break;

            default:
                break;

            }

            pPriv->lastKey = keycode;
                
        }

        else                       /* KeyRelease */
        {
#ifdef ALTSYS_REQ
            /* Deal with SysReq */
            if (scoStatusRecord & AltSysRequest)
            {
                scoStatusRecord &= ~AltSysRequest;
                scoHideMessage();
                PayAttentionToClientsAgain(AllScreens);

                switch (keySym)
                {
                case XK_Return:
                case XK_KP_Enter:
                    GiveUp (15);
                    break;

                default:
                    return;
                }
            }
#endif
                
            switch (keySym)
            {
            case XK_Caps_Lock: 
            case XK_Shift_Lock: 
            case XK_Num_Lock: 
            case XK_Scroll_Lock: 
            case XK_Kana_Lock: 

                keycode = 0;       /* ignore */
                break;

#ifdef ALTSYS_REQ
            case XK_Sys_Req: 
                
                if (inputInfo.keyboard->sync.state == NOT_GRABBED)
                {
                    /* BUG? do we need to check XsightAltSysReq? */
                    scoStatusRecord |= AltSysRequest;
                    DontListenToAnybody(AllScreens); 
                    scoPopMessage(MSGSYS(XSRV_EXIT, CONFIRM_EXIT_MSG));
                }
                break;
#endif

            default:
                break;
            }

        }
#ifdef XKB
    }
#endif        

    if (keycode)
    {
        xE.u.keyButtonPointer.time = NewInputEventTime();
        xE.u.u.detail = keycode;
        xE.u.u.type = down ? KeyPress : KeyRelease;
        mieqEnqueue(&xE);
    }

    if (!down)
        pPriv->lastKey = 0;        /* clear last key on release */

}


/*
 *-----------------------------------------------------------------------
 * scoRestoreKbd --
 *	REMOVE THIS FUNCTION !!!!!
 *
 *-----------------------------------------------------------------------
 */

int
scoRestoreKbd()
{

#if !defined(usl)
    DevicePtr pKeyboard, pPointer;
    int i;

    if(sysKbPriv.fd >= 0) {

	if(scoStatusRecord & TtyScancodeMode) { /* S008 */
	    scoStatusRecord &= ~TtyScancodeMode; /* S008 */
            sc_exit( sysKbPriv.fd ); /* S033 */
	    ioctl(sysKbPriv.fd, KDSKBMODE, K_XLATE);
	}

	if(scoStatusRecord & TtyModeChanged) { /* S008 */
	    scoStatusRecord &= ~TtyModeChanged; /* S008 */
	    RestoreTtyMode(sysKbPriv.fd);
	}
    }
#endif /*	! usl	*/
}


/*
 *-----------------------------------------------------------------------
 * scoKbdBell --
 *	Ring the terminal/keyboard bell
 *
 * Results:
 *	Ring the keyboard bell for an amount of time proportional to
 *	"loudness."
 *
 *-----------------------------------------------------------------------
 */

static void
scoKbdBell(int loudness,           /* Percentage of full volume */
           DeviceIntPtr pKeyboard, /* Keyboard to ring */
           pointer ctrl,
           int unused )
{
    KeybdCtrl*      kctrl = (KeybdCtrl*) ctrl;
    KbPrivPtr	  pPriv = (KbPrivPtr) pKeyboard->public.devicePrivate;

    if (loudness == 0 || kctrl->bell == 0) {
        return;
    }

    if (loudness && kctrl->bell_pitch) {
        (void)ioctl(pPriv->fd, KDMKTONE,
                    ((1193190 / kctrl->bell_pitch) & 0xffff) |
                    (((ulong)kctrl->bell_duration * loudness / 50) << 16));
    }

}


/*
 *-----------------------------------------------------------------------
 * LegalModifier
 *	Returns true is key generates make and break sequences.
 *
 * Results:
 *	Always returns True.
 *
 *-----------------------------------------------------------------------
 */

Bool
LegalModifier(unsigned int key, DevicePtr pDev)
{
    return TRUE;
}


/*
 *-----------------------------------------------------------------------
 * MergeHack --
 *  	Hack to get Merge Extensions linked in.
 *
 *-----------------------------------------------------------------------
 */

MergeHack()	
{
    GetSWSequence();
}



/*
 *-----------------------------------------------------------------------
 * scoKeysymToKeycode -- 
 * 	Returns a keysym's keycode.  If there is no keycode associated
 * 	with the keysym, then returns 0.
 *
 *-----------------------------------------------------------------------
 */

KeyCode
scoKeysymToKeycode (KeySym keysym)
{
    extern InputInfo inputInfo;
    register int mapcode;
    KeySym *kMap = inputInfo.keyboard->key->curKeySyms.map;	 
    int kMapWidth = inputInfo.keyboard->key->curKeySyms.mapWidth;
    int minKeyCode = inputInfo.keyboard->key->curKeySyms.minKeyCode;
    int maxKeyCode = inputInfo.keyboard->key->curKeySyms.maxKeyCode;

    for (mapcode = minKeyCode; mapcode < maxKeyCode; mapcode++)
    {
	if (kMap[mapcode * kMapWidth] == keysym)
	{
            return (mapcode+1);
	}
    }
    return 0;
}
