/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/os2/os2_VTsw.c,v 3.0 1996/01/30 15:26:30 dawes Exp $ */
/*
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 * Modified 1996 by Sebastien Marineau <marineau@genie.uottawa.ca>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Wexelblat not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  David Wexelblat makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID WEXELBLAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "X.h"
#include "input.h"
#include "scrnintstr.h"

#define I_NEED_OS2_H
#define INCL_WINSWITCHLIST
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

BOOL SwitchedToWPS=FALSE;
CARD32 LastSwitchTime;
HSWITCH GetDesktopSwitchHandle();
/*
 * Added OS/2 code to handle switching back to WPS
 */

Bool xf86VTSwitchPending()
{
	return(xf86Info.vtRequestsPending ? TRUE : FALSE);
}

Bool xf86VTSwitchAway()
{
        HSWITCH hSwitch;
        APIRET rc;

        xf86Info.vtRequestsPending=FALSE;
        hSwitch=GetDesktopSwitchHandle();
        ErrorF("xf86-OS/2: Switching to desktop. Handle: %d\n",hSwitch);
        if(hSwitch==NULLHANDLE) return(FALSE);
        rc=WinSwitchToProgram(hSwitch);
        SwitchedToWPS=TRUE;
        LastSwitchTime=GetTimeInMillis();
        usleep(300000);
	return(TRUE);
}

Bool xf86VTSwitchTo()
{
        xf86Info.vtRequestsPending=FALSE;
        SwitchedToWPS=FALSE;
        ErrorF("Switching back to server. \n");
	return(TRUE);
}

HSWITCH GetDesktopSwitchHandle()
{

  PSWBLOCK pswb;
  ULONG ulcEntries;
  ULONG usSize;
  HSWITCH hSwitch;
/* Get the switch list information */

  ulcEntries=WinQuerySwitchList(0, NULL, 0);
  usSize=sizeof(SWBLOCK)+sizeof(HSWITCH)+(ulcEntries+4)*(long)sizeof(SWENTRY);
  /* Allocate memory for list */
  if ((pswb=malloc((unsigned)usSize)) != NULL)
  {
    /* Put the info in the list */
    ulcEntries=WinQuerySwitchList(0, pswb, (USHORT)(usSize-sizeof(SWENTRY)));

    /* Return first entry in list, usually the desktop */

    hSwitch=pswb->aswentry[0].hswitch;
    if (pswb)
    free(pswb);
    return(hSwitch);
    }
return(NULLHANDLE);
}
