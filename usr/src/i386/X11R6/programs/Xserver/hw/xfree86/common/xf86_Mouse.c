/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86_Mouse.c,v 3.7 1996/01/30 15:25:59 dawes Exp $ */
/*
 *
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 * Copyright 1993 by David Dawes <dawes@physics.su.oz.au>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Thomas Roell and David Dawes not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  Thomas Roell
 * and David Dawes makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THOMAS ROELL AND DAVID DAWES DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THOMAS ROELL OR DAVID DAWES BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: xf86_Mouse.c /main/10 1996/01/30 15:16:12 kaleb $ */

/*
 * [JCH-96/01/21] Added fourth button support for P_GLIDEPOINT mouse protocol.
 */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"

#include "compiler.h"

#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "xf86_Config.h"


#ifndef MOUSE_PROTOCOL_IN_KERNEL
/*
 * List of mouse types supported by xf86MouseProtocol()
 *
 * For systems which do the mouse protocol translation in the kernel,
 * this list should be defined in the appropriate *_io.c file under
 * xf86/os-support.
 */
Bool xf86SupportedMouseTypes[] =
{
	TRUE,	/* Microsoft */
	TRUE,	/* MouseSystems */
	TRUE,	/* MMSeries */
	TRUE,	/* Logitech */
	TRUE,	/* BusMouse */
	TRUE,	/* MouseMan */
	TRUE,	/* PS/2 */
	TRUE,	/* Hitachi Tablet */
	TRUE,	/* ALPS GlidePoint */
};

int xf86NumMouseTypes = sizeof(xf86SupportedMouseTypes) /
			sizeof(xf86SupportedMouseTypes[0]);

/*
 * termio[s] c_cflag settings for each mouse type.
 *
 * For systems which do the mouse protocol translation in the kernel,
 * this list should be defined in the appropriate *_io.c under
 * xf86/os-support if it is required.
 */

unsigned short xf86MouseCflags[] =
{
	(CS7                   | CREAD | CLOCAL | HUPCL ),   /* MicroSoft */
	(CS8 | CSTOPB          | CREAD | CLOCAL | HUPCL ),   /* MouseSystems */
	(CS8 | PARENB | PARODD | CREAD | CLOCAL | HUPCL ),   /* MMSeries */
	(CS8 | CSTOPB          | CREAD | CLOCAL | HUPCL ),   /* Logitech */
	0,						     /* BusMouse */
	(CS7                   | CREAD | CLOCAL | HUPCL ),   /* MouseMan,
                                                              [CHRIS-211092] */
	0,						     /* PS/2 */
	(CS8                   | CREAD | CLOCAL | HUPCL ),   /* mmhitablet */
	(CS7                   | CREAD | CLOCAL | HUPCL ),   /* GlidePoint */
};
#endif /* ! MOUSE_PROTOCOL_IN_KERNEL */


/*
 * xf86MouseSupported --
 *	Returns true if OS supports mousetype
 */

Bool
xf86MouseSupported(mousetype)
     int mousetype;
{
    if (mousetype < 0 || mousetype >= xf86NumMouseTypes)
    {
	return(FALSE);
    }
    return(xf86SupportedMouseTypes[mousetype]);
}

/*
 * xf86SetupMouse --
 *	Sets up the mouse parameters
 */

void
xf86SetupMouse()
{
#if !defined(MOUSE_PROTOCOL_IN_KERNEL) || defined(MACH386)
      /*
      ** The following lines take care of the Logitech MouseMan protocols.
      **
      ** NOTE: There are different versions of both MouseMan and TrackMan!
      **       Hence I add another protocol P_LOGIMAN, which the user can
      **       specify as MouseMan in his XF86Config file. This entry was
      **       formerly handled as a special case of P_MS. However, people
      **       who don't have the middle button problem, can still specify
      **       Microsoft and use P_MS.
      **
      ** By default, these mice should use a 3 byte Microsoft protocol
      ** plus a 4th byte for the middle button. However, the mouse might
      ** have switched to a different protocol before we use it, so I send
      ** the proper sequence just in case.
      **
      ** NOTE: - all commands to (at least the European) MouseMan have to
      **         be sent at 1200 Baud.
      **       - each command starts with a '*'.
      **       - whenever the MouseMan receives a '*', it will switch back
      **	 to 1200 Baud. Hence I have to select the desired protocol
      **	 first, then select the baud rate.
      **
      ** The protocols supported by the (European) MouseMan are:
      **   -  5 byte packed binary protocol, as with the Mouse Systems
      **      mouse. Selected by sequence "*U".
      **   -  2 button 3 byte MicroSoft compatible protocol. Selected
      **      by sequence "*V".
      **   -  3 button 3+1 byte MicroSoft compatible protocol (default).
      **      Selected by sequence "*X".
      **
      ** The following baud rates are supported:
      **   -  1200 Baud (default). Selected by sequence "*n".
      **   -  9600 Baud. Selected by sequence "*q".
      **
      ** Selecting a sample rate is no longer supported with the MouseMan!
      ** Some additional lines in xf86Config.c take care of ill configured
      ** baud rates and sample rates. (The user will get an error.)
      **               [CHRIS-211092]
      */

  
      if (xf86Info.mseType == P_LOGIMAN)
        {
          xf86SetMouseSpeed(1200, 1200, xf86MouseCflags[P_LOGIMAN]);
          write(xf86Info.mseFd, "*X", 2);
          xf86SetMouseSpeed(1200, xf86Info.baudRate,
			    xf86MouseCflags[P_LOGIMAN]);
        }
      else if (xf86Info.mseType != P_BM && xf86Info.mseType != P_PS2) 
	{
	  xf86SetMouseSpeed(9600, xf86Info.baudRate,
			    xf86MouseCflags[xf86Info.mseType]);
	  xf86SetMouseSpeed(4800, xf86Info.baudRate, 
			    xf86MouseCflags[xf86Info.mseType]);
	  xf86SetMouseSpeed(2400, xf86Info.baudRate,
			    xf86MouseCflags[xf86Info.mseType]);
	  xf86SetMouseSpeed(1200, xf86Info.baudRate,
			    xf86MouseCflags[xf86Info.mseType]);

	  if (xf86Info.mseType == P_LOGI)
	    {
	      write(xf86Info.mseFd, "S", 1);
	      xf86SetMouseSpeed(xf86Info.baudRate, xf86Info.baudRate,
                                xf86MouseCflags[P_MM]);
	    }

	  if (xf86Info.mseType == P_MMHIT)
	  {
	    char speedcmd;

	    /*
	     * Initialize Hitachi PUMA Plus - Model 1212E to desired settings.
	     * The tablet must be configured to be in MM mode, NO parity,
	     * Binary Format.  xf86Info.sampleRate controls the sensativity
	     * of the tablet.  We only use this tablet for it's 4-button puck
	     * so we don't run in "Absolute Mode"
	     */
	    write(xf86Info.mseFd, "z8", 2);	/* Set Parity = "NONE" */
	    usleep(50000);
	    write(xf86Info.mseFd, "zb", 2);	/* Set Format = "Binary" */
	    usleep(50000);
	    write(xf86Info.mseFd, "@", 1);	/* Set Report Mode = "Stream" */
	    usleep(50000);
	    write(xf86Info.mseFd, "R", 1);	/* Set Output Rate = "45 rps" */
	    usleep(50000);
	    write(xf86Info.mseFd, "I\x20", 2);	/* Set Incrememtal Mode "20" */
	    usleep(50000);
	    write(xf86Info.mseFd, "E", 1);	/* Set Data Type = "Relative */
	    usleep(50000);

	    /* These sample rates translate to 'lines per inch' on the Hitachi
	       tablet */
	    if      (xf86Info.sampleRate <=   40) speedcmd = 'g';
	    else if (xf86Info.sampleRate <=  100) speedcmd = 'd';
	    else if (xf86Info.sampleRate <=  200) speedcmd = 'e';
	    else if (xf86Info.sampleRate <=  500) speedcmd = 'h';
	    else if (xf86Info.sampleRate <= 1000) speedcmd = 'j';
	    else                                  speedcmd = 'd';
	    write(xf86Info.mseFd, &speedcmd, 1);
	    usleep(50000);

	    write(xf86Info.mseFd, "\021", 1);	/* Resume DATA output */
	  }
	  else
	  {
	    if      (xf86Info.sampleRate <=   0)  write(xf86Info.mseFd, "O", 1);
	    else if (xf86Info.sampleRate <=  15)  write(xf86Info.mseFd, "J", 1);
	    else if (xf86Info.sampleRate <=  27)  write(xf86Info.mseFd, "K", 1);
	    else if (xf86Info.sampleRate <=  42)  write(xf86Info.mseFd, "L", 1);
	    else if (xf86Info.sampleRate <=  60)  write(xf86Info.mseFd, "R", 1);
	    else if (xf86Info.sampleRate <=  85)  write(xf86Info.mseFd, "M", 1);
	    else if (xf86Info.sampleRate <= 125)  write(xf86Info.mseFd, "Q", 1);
	    else                                  write(xf86Info.mseFd, "N", 1);
	  }
        }

#ifdef CLEARDTR_SUPPORT
      if (xf86Info.mseType == P_MSC && (xf86Info.mouseFlags & MF_CLEAR_DTR))
        {
          int val = TIOCM_DTR;
          ioctl(xf86Info.mseFd, TIOCMBIC, &val);
        }
      if (xf86Info.mseType == P_MSC && (xf86Info.mouseFlags & MF_CLEAR_RTS))
        {
          int val = TIOCM_RTS;
          ioctl(xf86Info.mseFd, TIOCMBIC, &val);
        }
#endif
#endif /* !MOUSE_PROTOCOL_IN_KERNEL || MACH386 */
}
 
#ifndef MOUSE_PROTOCOL_IN_KERNEL
void
xf86MouseProtocol(rBuf, nBytes)
     unsigned char *rBuf;
     int nBytes;
{
  int                  i, buttons, dx, dy;
  static int           pBufP = 0;
  static unsigned char pBuf[8];

  static unsigned char proto[9][5] = {
    /*  hd_mask hd_id   dp_mask dp_id   nobytes */
    { 	0x40,	0x40,	0x40,	0x00,	3 	},  /* MicroSoft */
    {	0xf8,	0x80,	0x00,	0x00,	5	},  /* MouseSystems */
    {	0xe0,	0x80,	0x80,	0x00,	3	},  /* MMSeries */
    {	0xe0,	0x80,	0x80,	0x00,	3	},  /* Logitech */
    {	0xf8,	0x80,	0x00,	0x00,	5	},  /* BusMouse */
    { 	0x40,	0x40,	0x40,	0x00,	3 	},  /* MouseMan
                                                       [CHRIS-211092] */
    {	0xc0,	0x00,	0x00,	0x00,	3	},  /* PS/2 mouse */
    {	0xe0,	0x80,	0x80,	0x00,	3	},  /* MM_HitTablet */
    { 	0x40,	0x40,	0x40,	0x00,	3 	},  /* GlidePoint */
  };
  
  for ( i=0; i < nBytes; i++) {
    /*
     * Hack for resyncing: We check here for a package that is:
     *  a) illegal (detected by wrong data-package header)
     *  b) invalid (0x80 == -128 and that might be wrong for MouseSystems)
     *  c) bad header-package
     *
     * NOTE: b) is a voilation of the MouseSystems-Protocol, since values of
     *       -128 are allowed, but since they are very seldom we can easily
     *       use them as package-header with no button pressed.
     * NOTE/2: On a PS/2 mouse any byte is valid as a data byte. Furthermore,
     *         0x80 is not valid as a header byte. For a PS/2 mouse we skip
     *         checking data bytes.
     *         For resyncing a PS/2 mouse we require the two most significant
     *         bits in the header byte to be 0. These are the overflow bits,
     *         and in case of an overflow we actually lose sync. Overflows
     *         are very rare, however, and we quickly gain sync again after
     *         an overflow condition. This is the best we can do. (Actually,
     *         we could use bit 0x08 in the header byte for resyncing, since
     *         that bit is supposed to be always on, but nobody told
     *         Microsoft...)
     */
    if (pBufP != 0 && xf86Info.mseType != P_PS2 &&
	((rBuf[i] & proto[xf86Info.mseType][2]) != proto[xf86Info.mseType][3]
	 || rBuf[i] == 0x80))
      {
	pBufP = 0;          /* skip package */
      }

    if (pBufP == 0 &&
	(rBuf[i] & proto[xf86Info.mseType][0]) != proto[xf86Info.mseType][1])
      {
	/*
	 * Hack for Logitech MouseMan Mouse - Middle button
	 *
	 * Unfortunately this mouse has variable length packets: the standard
	 * Microsoft 3 byte packet plus an optional 4th byte whenever the
	 * middle button status changes.
	 *
	 * We have already processed the standard packet with the movement
	 * and button info.  Now post an event message with the old status
	 * of the left and right buttons and the updated middle button.
	 */

        /*
	 * Even worse, different MouseMen and TrackMen differ in the 4th
         * byte: some will send 0x00/0x20, others 0x01/0x21, or even
         * 0x02/0x22, so I have to strip off the lower bits. [CHRIS-211092]
         *
         * [JCH-96/01/21]
         * HACK for ALPS "fourth button". (It's bit 0x10 of the "fourth byte"
         * and it is activated by tapping the glidepad with the finger! 8^)
         * We map it to bit bit3, and the reverse map in xf86Events just has
         * to be extended so that it is identified as Button 4. The lower
         * half of the reverse-map may remain unchanged.
	 */

	/*
	 * The order of tests in the following expression
	 * is an attempt to optimize wrt the likeliness of the
	 * various cases, think twice before simplifying.
	 */

	if (   (   (char)(rBuf[i] & ~0x23) != 0
		&& (   (char) (rBuf[i] & ~0x33) != 0
		    || xf86Info.mseType != P_GLIDEPOINT))
	    || (   xf86Info.mseType != P_MS
		&& xf86Info.mseType != P_LOGIMAN
		&& xf86Info.mseType != P_GLIDEPOINT)) continue;

	buttons =  ((int)(rBuf[i] & 0x20) >> 4)
		  | (xf86Info.lastButtons & 0x05);
	if (xf86Info.mseType == P_GLIDEPOINT)
	    buttons |= ((int)(rBuf[i] & 0x10) >> 1);
	xf86PostMseEvent(buttons, 0, 0);

	continue;            /* skip package */
      }


    pBuf[pBufP++] = rBuf[i];
    if (pBufP != proto[xf86Info.mseType][4]) continue;

    /*
     * assembly full package
     */
    switch(xf86Info.mseType) {
      
    case P_LOGIMAN:	    /* MouseMan / TrackMan   [CHRIS-211092] */
    case P_MS:              /* Microsoft */
      if (xf86Info.chordMiddle)
	buttons = (((int) pBuf[0] & 0x30) == 0x30) ? 2 :
		  ((int)(pBuf[0] & 0x20) >> 3)
		  | ((int)(pBuf[0] & 0x10) >> 4);
      else {
        buttons = (xf86Info.lastButtons & 2)
		  | ((int)(pBuf[0] & 0x20) >> 3)
		  | ((int)(pBuf[0] & 0x10) >> 4);
      }
      dx = (char)(((pBuf[0] & 0x03) << 6) | (pBuf[1] & 0x3F));
      dy = (char)(((pBuf[0] & 0x0C) << 4) | (pBuf[2] & 0x3F));
      break;

    case P_GLIDEPOINT:      /* ALPS GlidePoint */
       buttons =  (xf86Info.lastButtons & (8 + 2))
		| ((int)(pBuf[0] & 0x20) >> 3)
		| ((int)(pBuf[0] & 0x10) >> 4);
      dx = (char)(((pBuf[0] & 0x03) << 6) | (pBuf[1] & 0x3F));
      dy = (char)(((pBuf[0] & 0x0C) << 4) | (pBuf[2] & 0x3F));
      break;

    case P_MSC:             /* Mouse Systems Corp */
      buttons = (~pBuf[0]) & 0x07;
      dx =    (char)(pBuf[1]) + (char)(pBuf[3]);
      dy = - ((char)(pBuf[2]) + (char)(pBuf[4]));
      break;
      
    case P_MMHIT:           /* MM_HitTablet */
      buttons = pBuf[0] & 0x07;
      if (buttons != 0)
        buttons = 1 << (buttons - 1);
      dx = (pBuf[0] & 0x10) ?   pBuf[1] : - pBuf[1];
      dy = (pBuf[0] & 0x08) ? - pBuf[2] :   pBuf[2];
      break;

    case P_MM:              /* MM Series */
    case P_LOGI:            /* Logitech Mice */
      buttons = pBuf[0] & 0x07;
      dx = (pBuf[0] & 0x10) ?   pBuf[1] : - pBuf[1];
      dy = (pBuf[0] & 0x08) ? - pBuf[2] :   pBuf[2];
      break;
      
    case P_BM:              /* BusMouse */
      buttons = (~pBuf[0]) & 0x07;
      dx =   (char)pBuf[1];
      dy = - (char)pBuf[2];
      break;

    case P_PS2:		    /* PS/2 mouse */
      buttons = (pBuf[0] & 0x04) >> 1 |       /* Middle */
	        (pBuf[0] & 0x02) >> 1 |       /* Right */
		(pBuf[0] & 0x01) << 2;        /* Left */
      dx = (pBuf[0] & 0x10) ?    pBuf[1]-256  :  pBuf[1];
      dy = (pBuf[0] & 0x20) ?  -(pBuf[2]-256) : -pBuf[2];
      break;
    }

    xf86PostMseEvent(buttons, dx, dy);
    pBufP = 0;
  }
}
#endif /* MOUSE_PROTOCOL_IN_KERNEL */
