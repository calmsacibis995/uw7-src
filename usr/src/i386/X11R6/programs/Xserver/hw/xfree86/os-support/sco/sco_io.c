/* $XFree86: $ */
/*
 * Copyright 1993 by David McCullough <davidm@stallion.oz.au>
 * Copyright 1993 by David Dawes <dawes@physics.su.oz.au>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of David McCullough and David Dawes 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  David McCullough
 * and David Dawes makes no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * DAVID MCCULLOUGH AND DAVID DAWES DISCLAIMS ALL WARRANTIES WITH REGARD TO 
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL DAVID MCCULLOUGH OR DAVID DAWES BE LIABLE FOR 
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER 
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF 
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: sco_io.c /main/2 1995/11/13 06:08:41 kaleb $ */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"

#include "compiler.h"

#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "xf86_Config.h"

void xf86SoundKbdBell(loudness, pitch, duration)
int loudness;
int pitch;
int duration;
{
	if (loudness && pitch)
	{
		ioctl(xf86Info.consoleFd, KIOCSOUND, 1193180 / pitch);
		usleep(duration * loudness * 20);
		ioctl(xf86Info.consoleFd, KIOCSOUND, 0);
	}
}

void xf86SetKbdLeds(leds)
int leds;
{
	/*
	 * sleep the first time through under SCO.  There appears to be a
	 * timing problem in the driver which causes the keyboard to be lost.
	 * This sleep stops it from occurring.  The sleep could proably be
	 * a lot shorter as even trace can fix the problem.  You may
	 * prefer a usleep(100).
	 */
	static int once = 1;

	if (once)
	{
		sleep(1);
		once = 0;
	}
	ioctl(xf86Info.consoleFd, KDSETLED, leds );
}

void xf86MouseInit()
{
	if ((xf86Info.mseFd = open(xf86Info.mseDevice, O_RDWR | O_NDELAY)) < 0)
	{
		FatalError("Cannot open mouse (%s)\n", strerror(errno));
	}
}

int xf86MouseOn()
{
	xf86SetupMouse();

	/* Flush any pending input */
	ioctl(xf86Info.mseFd, TCFLSH, 0);

	return(xf86Info.mseFd);
}

int xf86MouseOff(doclose)
Bool doclose;
{
	if (xf86Info.mseFd >= 0)
	{
		if (xf86Info.mseType == P_LOGI)
		{
			write(xf86Info.mseFd, "U", 1);
			xf86SetMouseSpeed(xf86Info.baudRate, 1200,
				  	  xf86MouseCflags[P_LOGI]);
		}
		if (doclose)
		{
			close(xf86Info.mseFd);
		}
	}
	return(xf86Info.mseFd);
}
