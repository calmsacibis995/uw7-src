/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/std_mouse.c,v 3.3 1995/01/28 17:05:04 dawes Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany
 * Copyright 1993 by David Dawes <dawes@physics.su.oz.au>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Thomas Roell and David Dawes 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  Thomas Roell and
 * David Dawes makes no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THOMAS ROELL AND DAVID DAWES DISCLAIMS ALL WARRANTIES WITH REGARD TO 
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL THOMAS ROELL OR DAVID DAWES BE LIABLE FOR 
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER 
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF 
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: std_mouse.c /main/3 1995/11/13 06:15:17 kaleb $ */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"

#include "xf86_OSlib.h"
#include "xf86Procs.h"
#include "xf86_Config.h"

int xf86MouseOff(doclose)
Bool doclose;
{
	int oldfd;

	if ((oldfd = xf86Info.mseFd) >= 0)
	{
		if (xf86Info.mseType == P_LOGI)
		{
			write(xf86Info.mseFd, "U", 1);
		}
		if (xf86Info.oldBaudRate > 0) {
			xf86SetMouseSpeed(xf86Info.baudRate,
					  xf86Info.oldBaudRate,
				  	  xf86MouseCflags[xf86Info.mseType]);
		}
		close(xf86Info.mseFd);
		oldfd = xf86Info.mseFd;
		xf86Info.mseFd = -1;
	}
	return(oldfd);
}
