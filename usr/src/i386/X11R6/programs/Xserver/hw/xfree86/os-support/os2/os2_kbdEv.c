/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/os2/os2_kbdEv.c,v 3.2 1996/01/30 15:26:34 dawes Exp $ */
/*
 * (c) Copyright 1994 by Holger Veit
 *			<Holger.Veit@gmd.de>
 * Modified 1996 Sebastien Marineau <marineau@genie.uottawa.ca>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * HOLGER VEIT  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of Holger Veit shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Holger Veit.
 *
 */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "inputstr.h"
#include "scrnintstr.h"

#define I_NEED_OS2_H
#define INCL_KBD
#undef RT_FONT	/* must discard this */
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "atKeynames.h"

/* Attention! these lines copied from ../../common/xf86Events.c */
#define XE_POINTER 1
#define XE_KEYBOARD 2

#ifdef XTESTEXT1

#define	XTestSERVER_SIDE
#include "xtestext1.h"
extern short xtest_mousex;
extern short xtest_mousey;
extern int   on_steal_input;          
extern Bool  XTestStealKeyData();
extern void  XTestStealMotionData();

#define ENQUEUE(ev, code, direction, dev_type) \
  (ev)->u.u.detail = (code); \
  (ev)->u.u.type   = (direction); \
  if (!on_steal_input ||  \
      XTestStealKeyData((ev)->u.u.detail, (ev)->u.u.type, dev_type, \
			xtest_mousex, xtest_mousey)) \
  mieqEnqueue((ev))

#define MOVEPOINTER(dx, dy, time) \
  if (on_steal_input) \
    XTestStealMotionData(dx, dy, XE_POINTER, xtest_mousex, xtest_mousey); \
  miPointerDeltaCursor (dx, dy, time)

#else /* ! XTESTEXT1 */

#define ENQUEUE(ev, code, direction, dev_type) \
  (ev)->u.u.detail = (code); \
  (ev)->u.u.type   = (direction); \
  mieqEnqueue((ev))

#define MOVEPOINTER(dx, dy, time) \
  miPointerDeltaCursor (dx, dy, time)

#endif
/* end of include */


int last_status;
int lastShiftState;
extern BOOL SwitchedToWPS;

static void os2PostKbdEvent();

int os2KbdQueueQuery()
{
    KBDKEYINFO keybuf;
    APIRET rc;
    
	keybuf.chScan=0;
	keybuf.fsState=0;
	keybuf.fbStatus=0;
	rc = KbdPeek(&keybuf,xf86Info.consoleFd);
	if(rc!=0){
	     ErrorF("Kbd returned bad rc=%d\n",rc);
	     return (-1);        /* We may have lost focus? */
	}
	if((keybuf.fbStatus!=0)) return(0); /* We have something in queue */
	   
return (1);

}


void xf86KbdEvents()
{
    KBDKEYINFO keybuf;
    APIRET rc;
    int scan,down;
    static int last;
    USHORT ModState;

    while(1) {
		/* Let's init the key struct */
	keybuf.chScan=0;
	keybuf.fsState=0;
	keybuf.fbStatus=0;

	rc = KbdCharIn(&keybuf, 1, xf86Info.consoleFd);
	if (rc != 0){
		ErrorF("Keyboard driver rc=%d\n",rc);
	    return;
	}
	if ((keybuf.fbStatus == 0))
	    return;
	scan = keybuf.chScan;
	ModState=(0xFF83 &
		(lastShiftState ^ keybuf.fsState));

        /* Check to see if we need toreenable the server */

	/* the separate cursor keys return 0xe0/scan */
	if (keybuf.fbStatus & 0x02) {
	    switch (scan) {

/* BUG ALERT: IBM has in its keyboard driver a 122 key keyboard, which
 * uses the "server generated scancodes" from atKeynames.h as real scan codes.
 * We wait until some poor guy with such a keyboard will break the whole
 * card house though...
 */
	    case KEY_KP_7: scan = KEY_Home; break;	/* curs home */
	    case KEY_KP_8: scan = KEY_Up;  break;	/* curs up */
	    case KEY_KP_9: scan = KEY_PgUp; break;	/* curs pgup */
	    case KEY_KP_4: scan = KEY_Left; break;	/* curs left */
	    case KEY_KP_5: scan = KEY_Begin; break;	/* curs begin */
	    case KEY_KP_6: scan = KEY_Right; break;	/* curs right */
	    case KEY_KP_1: scan = KEY_End; break;	/* curs end */
	    case KEY_KP_2: scan = KEY_Down; break;	/* curs down */
	    case KEY_KP_3: scan = KEY_PgDown; break;	/* curs pgdown */
	    case KEY_KP_0: scan = KEY_Insert; break;	/* curs insert */
	    case KEY_KP_Decimal: scan = KEY_Delete; break; /* curs delete */
	    case KEY_Enter: scan = KEY_KP_Enter; break;	/* keypad enter */
	    case KEY_LCtrl: scan = KEY_RCtrl; break;	/* right ctrl */
	    case KEY_KP_Multiply: scan = KEY_Print; break; /* print */
	    case KEY_Slash: scan = KEY_KP_Divide; break;   /* keyp divide */
	    case KEY_Alt: scan = KEY_AltLang; break;	/* right alt */
	    case KEY_ScrollLock: scan = KEY_Break; break;  /* curs break */
	    case 0x5b: scan = KEY_LMeta; break;
	    case 0x5c: scan = KEY_RMeta; break;
	    case 0x5d: scan = KEY_Menu; break;
	    }
	}

	/* Check to see if the shift key status has changed. If so, then
	   send the keycodes. The default handling does not send the
	   scancodes for these.....Damn those smart keyboard drivers     */


	if(ModState){
		down=ModState & keybuf.fsState;
		switch (ModState){
		    case 0x01:os2PostKbdEvent(KEY_ShiftR,down);break;
		    case 0x02:os2PostKbdEvent(KEY_ShiftL,down);break;
		    case 0x10:os2PostKbdEvent(KEY_ScrollLock,down);break;
		    case 0x20:os2PostKbdEvent(KEY_NumLock,down);break;
		    case 0x40:os2PostKbdEvent(KEY_CapsLock,down);break;
		    case 0x80:os2PostKbdEvent(KEY_Insert,down);break;
		    case 0x100:os2PostKbdEvent(KEY_LCtrl,down);break;
		    case 0x200:os2PostKbdEvent(KEY_Alt,down);break;
		    case 0x400:os2PostKbdEvent(KEY_RCtrl,down);break;
		    case 0x800:os2PostKbdEvent(KEY_AltLang,down);break;
		    case 0x1000:os2PostKbdEvent(KEY_ScrollLock,down);break;
		    case 0x2000:os2PostKbdEvent(KEY_NumLock,down);break;
		    case 0x4000:os2PostKbdEvent(KEY_CapsLock,down);break;
		}
	}

	down = (keybuf.fbStatus & 0x40) ? TRUE : FALSE;
	if(scan!=0) os2PostKbdEvent(scan, down);
	last_status = keybuf.fbStatus;
	lastShiftState = keybuf.fsState;
    }
}

/*
 * xf86PostKbdEvent --
 *	Translate the raw hardware KbdEvent into an XEvent, and tell DIX
 *	about it. Scancode preprocessing and so on is done ...
 *
 *  OS/2 specific xf86PostKbdEvent(key) has been moved from common/xf86Events.c
 *  as some things differ, and I didnït want to scatter this routine with
 *  ifdefs further (hv).
 */

    

    

static void
os2PostKbdEvent(scanCode, down)
     unsigned scanCode;
     Bool down;
{
    KeyClassRec *keyc = ((DeviceIntPtr)xf86Info.pKeyboard)->key;
    Bool        updateLeds = FALSE;
    Bool        UsePrefix = FALSE;
    Bool        Direction = FALSE;
    xEvent      kevent;
    KeySym      *keysym;
    int         keycode;
    static int  lockkeys = 0;

    if (xf86Info.serverNumLock &&
        ((!xf86Info.numLock && !ModifierDown(ShiftMask)) ||
        (xf86Info.numLock && ModifierDown(ShiftMask)))) {
        /*
         * Hardwired numlock handling ... (Some applications break if they have
         * these keys double defined, like twm)
         */
	switch (scanCode) {
	case KEY_KP_7:        scanCode = KEY_Home;      break;  /* curs home */
	case KEY_KP_8:        scanCode = KEY_Up;        break;  /* curs up */
	case KEY_KP_9:        scanCode = KEY_PgUp;      break;  /* curs pgup */
	case KEY_KP_4:        scanCode = KEY_Left;      break;  /* curs left */
	case KEY_KP_5:        scanCode = KEY_Begin;     break;  /* curs begin */
	case KEY_KP_6:        scanCode = KEY_Right;     break;  /* curs right */
	case KEY_KP_1:        scanCode = KEY_End;       break;  /* curs end */
	case KEY_KP_2:        scanCode = KEY_Down;      break;  /* curs down */
	case KEY_KP_3:        scanCode = KEY_PgDown;    break;  /* curs pgdown */
	case KEY_KP_0:        scanCode = KEY_Insert;    break;  /* curs insert */
	case KEY_KP_Decimal:  scanCode = KEY_Delete;    break;  /* curs delete */
	}
    }

    /*
     * and now get some special keysequences
     */
    if ((ModifierDown(ControlMask | AltMask)) ||
        (ModifierDown(ControlMask | AltLangMask)))
    {
	switch (scanCode) {
	case KEY_BackSpace:
	    if (!xf86Info.dontZap) GiveUp(0);
	return;
        }
    }

    /* CTRL-ESC is std OS/2 hotkey for going back to PM and popping up
     * window list... handled by keyboard driver if you tell it.
     */
    if (ModifierDown(ControlMask) && scanCode==KEY_Escape) {
        if(!SwitchedToWPS) xf86Info.vtRequestsPending=TRUE;
        return;
    } else if (ModifierDown(AltLangMask|AltMask) && scanCode==KEY_Escape) {
	/*notyet*/
    }

  /*
   * Now map the scancodes to real X-keycodes ...
   */
  keycode = scanCode + MIN_KEYCODE;
  keysym = (keyc->curKeySyms.map +
	    keyc->curKeySyms.mapWidth * 
	    (keycode - keyc->curKeySyms.minKeyCode));
  ErrorF("Keyboard keycode %d, keysym =%di down %d\n",keycode,*keysym,down);

  /*
   * Filter autorepeated caps/num/scroll lock keycodes.
   */
#define CAPSFLAG 0x01
#define NUMFLAG 0x02
#define SCROLLFLAG 0x04
#define MODEFLAG 0x08
  if( down ) {
    switch( keysym[0] ) {
        case XK_Caps_Lock :
          if (lockkeys & CAPSFLAG)
              return;
	  else
	      lockkeys |= CAPSFLAG;
          break;

        case XK_Num_Lock :
          if (lockkeys & NUMFLAG)
              return;
	  else
	      lockkeys |= NUMFLAG;
          break;

        case XK_Scroll_Lock :
          if (lockkeys & SCROLLFLAG)
              return;
	  else
	      lockkeys |= SCROLLFLAG;
          break;
    }
    if (keysym[1] == XF86XK_ModeLock)
    {
      if (lockkeys & MODEFLAG)
          return;
      else
          lockkeys |= MODEFLAG;
    }
      
  }
  else {
    switch( keysym[0] ) {
        case XK_Caps_Lock :
            lockkeys &= ~CAPSFLAG;
            break;

        case XK_Num_Lock :
            lockkeys &= ~NUMFLAG;
            break;

        case XK_Scroll_Lock :
            lockkeys &= ~SCROLLFLAG;
            break;
    }
    if (keysym[1] == XF86XK_ModeLock)
      lockkeys &= ~MODEFLAG;
  }

  /*
   * LockKey special handling:
   * ignore releases, toggle on & off on presses.
   * Don't deal with the Caps_Lock keysym directly, but check the lock modifier
   */
  if (keyc->modifierMap[keycode] & LockMask ||
      keysym[0] == XK_Scroll_Lock ||
      keysym[1] == XF86XK_ModeLock ||
      keysym[0] == XK_Num_Lock)
    {
      Bool flag;

      if (!down) return;
      if (KeyPressed(keycode)) {
	down = !down;
	flag = FALSE;
      }
      else
	flag = TRUE;

      if (keyc->modifierMap[keycode] & LockMask)   xf86Info.capsLock   = flag;
      if (keysym[0] == XK_Num_Lock)    xf86Info.numLock    = flag;
      if (keysym[0] == XK_Scroll_Lock) xf86Info.scrollLock = flag;
      if (keysym[1] == XF86XK_ModeLock)   xf86Info.modeSwitchLock = flag;
      updateLeds = TRUE;
    }
	
  /*
   * check for an autorepeat-event
   */
  if ((down && KeyPressed(keycode)) &&
      (xf86Info.autoRepeat != AutoRepeatModeOn || keyc->modifierMap[keycode]))
    return;

  xf86Info.lastEventTime = kevent.u.keyButtonPointer.time = GetTimeInMillis();

  /*
   * normal, non-keypad keys
   */
  if (scanCode < KEY_KP_7 || scanCode > KEY_KP_Decimal) {
    /*
     * magic ALT_L key on AT84 keyboards for multilingual support
     */
    if (xf86Info.kbdType == KB_84 &&
	ModifierDown(AltMask) &&
	keysym[2] != NoSymbol)
      {
	UsePrefix = TRUE;
	Direction = TRUE;
      }
  }




  /*
   * And now send these prefixes ...
   * NOTE: There cannot be multiple Mode_Switch keys !!!!
   */
  if (UsePrefix)
    {
      ENQUEUE(&kevent,
	      keyc->modifierKeyMap[keyc->maxKeysPerModifier*7],
	      (Direction ? KeyPress : KeyRelease),
	      XE_KEYBOARD);
      ENQUEUE(&kevent, keycode, (down ? KeyPress : KeyRelease), XE_KEYBOARD);
      ENQUEUE(&kevent,
	      keyc->modifierKeyMap[keyc->maxKeysPerModifier*7],
	      (Direction ? KeyRelease : KeyPress),
	      XE_KEYBOARD);
    }
  else 
    {
      ENQUEUE(&kevent, keycode, (down ? KeyPress : KeyRelease), XE_KEYBOARD);
    }

  if (updateLeds) xf86KbdLeds();
}


