/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/posix_tty.c,v 3.4 1995/01/28 17:05:03 dawes Exp $ */
/*
 * Copyright 1993 by David Dawes <dawes@physics.su.oz.au>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Dawes 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.
 * David Dawes makes no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * DAVID DAWES DISCLAIMS ALL WARRANTIES WITH REGARD TO 
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL DAVID DAWES BE LIABLE FOR 
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER 
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF 
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: posix_tty.c /main/4 1995/11/13 06:15:12 kaleb $ */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"

#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "xf86_Config.h"

static Bool not_a_tty = FALSE;

void xf86SetMouseSpeed(old, new, cflag)
int old;
int new;
unsigned cflag;
{
	struct termios tty;
	char *c;

	if (not_a_tty)
		return;

	if (tcgetattr(xf86Info.mseFd, &tty) < 0)
	{
		not_a_tty = TRUE;
		ErrorF("Warning: unable to get status of mouse fd (%s)\n",
		       strerror(errno));
		return;
	}

	/* this will query the initial baudrate only once */
	if (xf86Info.oldBaudRate < 0) { 
	   switch (cfgetispeed(&tty)) 
	      {
	      case B9600: 
		 xf86Info.oldBaudRate = 9600;
		 break;
	      case B4800: 
		 xf86Info.oldBaudRate = 4800;
		 break;
	      case B2400: 
		 xf86Info.oldBaudRate = 2400;
		 break;
	      case B1200: 
	      default:
		 xf86Info.oldBaudRate = 1200;
		 break;
	      }
	}

	tty.c_iflag = IGNBRK | IGNPAR;
	tty.c_oflag = 0;
	tty.c_lflag = 0;
	tty.c_cflag = (tcflag_t)cflag;
	tty.c_cc[VTIME] = 0;
	tty.c_cc[VMIN] = 1;

	switch (old)
	{
	case 9600:
		cfsetispeed(&tty, B9600);
		cfsetospeed(&tty, B9600);
		break;
	case 4800:
		cfsetispeed(&tty, B4800);
		cfsetospeed(&tty, B4800);
		break;
	case 2400:
		cfsetispeed(&tty, B2400);
		cfsetospeed(&tty, B2400);
		break;
	case 1200:
	default:
		cfsetispeed(&tty, B1200);
		cfsetospeed(&tty, B1200);
	}

	if (tcsetattr(xf86Info.mseFd, TCSADRAIN, &tty) < 0)
	{
		xf86FatalError("Unable to set status of mouse fd (%s)\n",
			       strerror(errno));
	}

	switch (new)
	{
	case 9600:
		c = "*q";
		cfsetispeed(&tty, B9600);
		cfsetospeed(&tty, B9600);
		break;
	case 4800:
		c = "*p";
		cfsetispeed(&tty, B4800);
		cfsetospeed(&tty, B4800);
		break;
	case 2400:
		c = "*o";
		cfsetispeed(&tty, B2400);
		cfsetospeed(&tty, B2400);
		break;
	case 1200:
	default:
		c = "*n";
		cfsetispeed(&tty, B1200);
		cfsetospeed(&tty, B1200);
	}

	if (xf86Info.mseType == P_LOGIMAN || xf86Info.mseType == P_LOGI)
	{
		if (write(xf86Info.mseFd, c, 2) != 2)
		{
			xf86FatalError("Unable to write to mouse fd (%s)\n",
				       strerror(errno));
		}
	}
	usleep(100000);

	if (tcsetattr(xf86Info.mseFd, TCSADRAIN, &tty) < 0)
	{
		xf86FatalError("Unable to set status of mouse fd (%s)\n",
			       strerror(errno));
	}
}

