/*
 *	@(#) scoXTest1.c 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Tue Jul 30 22:06:48 PDT 1991	mikep@sco.com
 *	- Created file.
 *	S001	Thu Feb 11 16:27:41 PST 1993	mikep@sco.com
 *	- Need to call back to mi here to make sure events get
 *	processed.  
 *	S002	Sat Feb 13 14:07:20 PST 1993	mikep@sco.com
 *	- Speed up XTestGenerateEvent() by avoiding too much 
 *	function call overhead.
 *
 */

#include "sco.h"
#include "dixstruct.h"
#define	XTestSERVER_SIDE
#include "extensions/xtestext1.h"

extern CARD32 lastEventTime;

void
XTestGetPointerPos(fmousex, fmousey)
        short *fmousex, *fmousey;
{
    miPointerPosition(fmousex, fmousey);
}

void
XTestJumpPointer(jx, jy, dev_type)
        int     jx;
        int     jy;
        int     dev_type;
{
    miPointerAbsoluteCursor(jx, jy, GetTimeInMillis());
    mieqProcessInputEvents();				/* S001 */
    miPointerUpdate();
}

void
XTestGenerateEvent(dev_type, keycode, keystate, mousex, mousey)
        int     dev_type;
        int     keycode;
        int     keystate;
        int     mousex;
        int     mousey;

{
	devEvent dE;

#ifdef DEBUG
	ErrorF("dev_type=%d, keycode=%d, keystate=%d, mousex=%d, mousey=%d\n",
		dev_type, keycode, keystate, mousex, mousey);
#endif

	lastEventTime = dE.deve_time  = GetTimeInMillis();

	miPointerAbsoluteCursor(mousex, mousey, lastEventTime);  /* S002 */
	dE.deve_x = mousex;
	dE.deve_y = mousey;

	dE.deve_type = DEVE_BUTTON;
	dE.deve_device = dev_type;

	if (keystate == XTestKEY_DOWN) 
	    dE.deve_direction = DEVE_KBTDOWN;
	else
	    dE.deve_direction = DEVE_KBTUP;

	/* Convert Keysyms back to scancodes.  This may not work all the time.
	 */
	if(dev_type == DEVE_DKB)
	    dE.deve_key = keycode - 7;
	else
	    dE.deve_key = keycode - 1;


	ProcessEvent(&dE);

	mieqProcessInputEvents();				/* S001 */
	miPointerUpdate();					/* S001 */

}
