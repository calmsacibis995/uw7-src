/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/os2/os2_init.c,v 3.2 1996/01/30 15:26:31 dawes Exp $ */
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

#include "X.h"
#include "Xmd.h"
#include "input.h"
#include "scrnintstr.h"

#include "compiler.h"

#define I_NEED_OS2_H
#define INCL_DOSFILEMGR
#define INCL_KBD
#define INCL_VIO
#include "xf86.h"
#include "xf86Procs.h"
#include "xf86_OSlib.h"

VIOMODEINFO OriginalVideoMode;

void xf86OpenConsole()
{
    if (serverGeneration == 1)
    {
	HKBD fd;
	ULONG dummy;
	KBDHWID hwid;
	APIRET rc;

	ErrorF("xf86-OS/2: Console opened\n");
	OriginalVideoMode.cb=sizeof(VIOMODEINFO);
	rc=VioGetMode(&OriginalVideoMode,(HVIO)0);
	if(rc!=0) ErrorF("xf86-OS/2: Could not get original video mode. RC=%d\n",rc);
	xf86Info.consoleFd = -1;

	/* grab the keyboard */
	rc = KbdGetFocus(0,0);
	if (rc != 0)
		FatalError("xf86OpenConsole: cannot grab kbd focus, rc=%d\n",rc);
	

	/* open the keyboard */
	rc = KbdOpen(&fd);
	if (rc != 0)
		FatalError("xf86OpenConsole: cannot open keyboard, rc=%d\n",rc);
	xf86Info.consoleFd = fd;

	ErrorF("xf86-OS/2: Keyboard opened\n");

	/* assign logical keyboard */
	KbdFreeFocus(0);
	rc = KbdGetFocus(0,fd);
	if (rc != 0)
		FatalError("xf86OpenConsole: cannot set local kbd focus, rc=%d\n",rc);

	rc = KbdSetCp(0,0,fd);
	if(rc != 0)
		FatalError("xf86OpenCOnsole: cannot set keyboard codepage, rc=%d\n",rc);
	rc = KbdGetHWID(&hwid, fd);
	if (rc == 0) {
		switch (hwid.idKbd) {
		default:
		case 0xab54: /* 88/89 key */
		case 0:	/*unknown*/
		case 1: /*real AT 84 key*/
			xf86Info.kbdType = KB_84; break;
		case 0xab85: /* 122 key */
			FatalError("OS/2 has detected an extended 122key keyboard: unsupported!\n");
		case 0xab41: /* 101/102 key */
			xf86Info.kbdType = KB_101; break;
		}				
	} else
		xf86Info.kbdType = KB_84; /*defensive*/

	xf86Config(FALSE); /* Read XF86Config */
    }
    return;
}

void xf86CloseConsole()
{
	if (xf86Info.consoleFd != -1) {
		KbdClose(xf86Info.consoleFd);
	}
	VioSetMode(&OriginalVideoMode,(HVIO)0);
	return;
}

/* ARGSUSED */
int xf86ProcessArgument (argc, argv, i)
int argc;
char *argv[];
int i;
{
	return(0);
}

void xf86UseMsg()
{
	return;
}

