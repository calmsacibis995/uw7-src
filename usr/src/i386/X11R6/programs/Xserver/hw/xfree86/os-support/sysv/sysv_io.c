/* $XFree86: $ */
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
/* $XConsortium: sysv_io.c /main/2 1995/11/13 06:21:49 kaleb $ */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"

#include "compiler.h"

#include "xf86Procs.h"
#include "xf86_OSlib.h"

void xf86SoundKbdBell(loudness, pitch, duration)
int loudness;
int pitch;
int duration;
{
	if (loudness && pitch)
	{
#ifdef KDMKTONE
		/*
		 * If we have KDMKTONE use it to avoid putting the server
		 * to sleep
		 */
		ioctl(xf86Info.consoleFd, KDMKTONE,
		      ((1193190 / pitch) & 0xffff) |
		      (((unsigned long)duration *
			loudness / 50) << 16));
#else
		ioctl(xf86Info.consoleFd, KIOCSOUND, 1193180 / pitch);
		usleep(xf86Info.bell_duration * loudness * 20);
		ioctl(xf86Info.consoleFd, KIOCSOUND, 0);
#endif
	}
}

void xf86SetKbdLeds(leds)
int leds;
{
#ifdef KBIO_SETMODE
	ioctl(xf86Info.consoleFd, KBIO_SETMODE, KBM_AT);
	ioctl(xf86Info.consoleFd, KDSETLED, leds);
	ioctl(xf86Info.consoleFd, KBIO_SETMODE, KBM_XT);
#endif
}

void xf86MouseInit()
{
	return;
}

int xf86MouseOn()
{
	if ((xf86Info.mseFd = open(xf86Info.mseDevice, O_RDWR | O_NDELAY)) < 0)
	{
		FatalError("Cannot open mouse (%s)\n", strerror(errno));
	}

	xf86SetupMouse();

	/* Flush any pending input */
	ioctl(xf86Info.mseFd, TCFLSH, 0);

	return(xf86Info.mseFd);
}
