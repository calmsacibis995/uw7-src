/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/lnx_init.c,v 3.3 1995/06/24 10:29:11 dawes Exp $ */
/*
 * Copyright 1992 by Orest Zborowski <obz@Kodak.com>
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Orest Zborowski and David Wexelblat 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  Orest Zborowski
 * and David Wexelblat make no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * OREST ZBOROWSKI AND DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL OREST ZBOROWSKI OR DAVID WEXELBLAT BE LIABLE 
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: lnx_init.c /main/4 1995/11/13 05:58:38 kaleb $ */

#include "X.h"
#include "Xmd.h"
#include "input.h"
#include "scrnintstr.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Procs.h"
#include "xf86_OSlib.h"

static Bool KeepTty = FALSE;
static int VTnum = -1;
static int activeVT = -1;

extern void xf86VTRequest(
#if NeedFunctionPrototypes
	int
#endif
);

void xf86OpenConsole()
{
    int i, fd;
    struct vt_mode VT;
    char vtname[11];
    struct vt_stat vts;

    if (serverGeneration == 1) 
    {
	/* check if we're run with euid==0 */
	if (geteuid() != 0)
	{
	    FatalError("xf86OpenConsole: Server must be suid root\n");
	}

	/*
	 * setup the virtual terminal manager
	 */
	if (VTnum != -1)
	{
	    xf86Info.vtno = VTnum;
	}
	else 
	{
	    if ((fd = open("/dev/console",O_WRONLY,0)) < 0) 
	    {
		FatalError(
		    "xf86OpenConsole: Cannot open /dev/console (%s)\n",
		    strerror(errno));
	    }
	    if ((ioctl(fd, VT_OPENQRY, &xf86Info.vtno) < 0) ||
		(xf86Info.vtno == -1))
	    {
		FatalError("xf86OpenConsole: Cannot find a free VT\n");
	    }
	    close(fd);
	}
	ErrorF("(using VT number %d)\n\n", xf86Info.vtno);

	sprintf(vtname,"/dev/tty%d",xf86Info.vtno); /* /dev/tty1-64 */

	xf86Config(FALSE); /* Read XF86Config */

	if (!KeepTty)
	{
	    setpgrp();
	}

	if ((xf86Info.consoleFd = open(vtname, O_RDWR|O_NDELAY, 0)) < 0)
	{
	    FatalError("xf86OpenConsole: Cannot open %s (%s)\n",
		       vtname, strerror(errno));
	}

	/* change ownership of the vt */
	chown(vtname, getuid(), getgid());

	/*
	 * the current VT device we're running on is not "console", we want
	 * to grab all consoles too
	 */
	chown("/dev/console", getuid(), getgid());
	chown("/dev/tty0", getuid(), getgid());

	/*
	 * Linux doesn't switch to an active vt after the last close of a vt,
	 * so we do this ourselves by remembering which is active now.
	 */
	if (ioctl(xf86Info.consoleFd, VT_GETSTATE, &vts) == 0)
	{
	    activeVT = vts.v_active;
	}

	if (!KeepTty)
	{
	    /*
	     * Detach from the controlling tty to avoid char loss
	     */
	    if ((i = open("/dev/tty",O_RDWR)) >= 0)
	    {
		ioctl(i, TIOCNOTTY, 0);
		close(i);
	    }
	}

	/*
	 * now get the VT
	 */
	if (ioctl(xf86Info.consoleFd, VT_ACTIVATE, xf86Info.vtno) != 0)
	{
	    ErrorF("xf86OpenConsole: VT_ACTIVATE failed\n");
	}
	if (ioctl(xf86Info.consoleFd, VT_WAITACTIVE, xf86Info.vtno) != 0)
	{
	    ErrorF("xf86OpenConsole: VT_WAITACTIVE failed\n");
	}
	if (ioctl(xf86Info.consoleFd, VT_GETMODE, &VT) < 0) 
	{
	    FatalError ("xf86OpenConsole: VT_GETMODE failed\n");
	}

	signal(SIGUSR1, xf86VTRequest);

	VT.mode = VT_PROCESS;
	VT.relsig = SIGUSR1;
	VT.acqsig = SIGUSR1;
	if (ioctl(xf86Info.consoleFd, VT_SETMODE, &VT) < 0) 
	{
	    FatalError("xf86OpenConsole: VT_SETMODE VT_PROCESS failed\n");
	}
	if (ioctl(xf86Info.consoleFd, KDSETMODE, KD_GRAPHICS) < 0)
	{
	    FatalError("xf86OpenConsole: KDSETMODE KD_GRAPHICS failed\n");
	}
    }
    else 
    {
	/* serverGeneration != 1 */
	/*
	 * now get the VT
	 */
	if (ioctl(xf86Info.consoleFd, VT_ACTIVATE, xf86Info.vtno) != 0)
	{
	    ErrorF("xf86OpenConsole: VT_ACTIVATE failed\n");
	}
	if (ioctl(xf86Info.consoleFd, VT_WAITACTIVE, xf86Info.vtno) != 0)
	{
	    ErrorF("xf86OpenConsole: VT_WAITACTIVE failed\n");
	}
    }
    return;
}

void xf86CloseConsole()
{
    struct vt_mode   VT;

#if 0
    ioctl(xf86Info.consoleFd, VT_ACTIVATE, xf86Info.vtno);
    ioctl(xf86Info.consoleFd, VT_WAITACTIVE, 0);
#endif
    ioctl(xf86Info.consoleFd, KDSETMODE, KD_TEXT);  /* Back to text mode ... */
    if (ioctl(xf86Info.consoleFd, VT_GETMODE, &VT) != -1)
    {
	VT.mode = VT_AUTO;
	ioctl(xf86Info.consoleFd, VT_SETMODE, &VT); /* set dflt vt handling */
    }
    /*
     * Perform a switch back to the active VT when we were started
     */
    if (activeVT >= 0)
    {
	ioctl(xf86Info.consoleFd, VT_ACTIVATE, activeVT);
	activeVT = -1;
    }
    close(xf86Info.consoleFd);                /* make the vt-manager happy */
    return;
}

int xf86ProcessArgument (argc, argv, i)
int argc;
char *argv[];
int i;
{
	/*
	 * Keep server from detaching from controlling tty.  This is useful 
	 * when debugging (so the server can receive keyboard signals.
	 */
	if (!strcmp(argv[i], "-keeptty"))
	{
		KeepTty = TRUE;
		return(1);
	}
	if ((argv[i][0] == 'v') && (argv[i][1] == 't'))
	{
		if (sscanf(argv[i], "vt%2d", &VTnum) == 0)
		{
			UseMsg();
			VTnum = -1;
			return(0);
		}
		return(1);
	}
	return(0);
}

void xf86UseMsg()
{
	ErrorF("vtXX                   use the specified VT number\n");
	ErrorF("-keeptty               ");
	ErrorF("don't detach controlling tty (for debugging only)\n");
	return;
}
