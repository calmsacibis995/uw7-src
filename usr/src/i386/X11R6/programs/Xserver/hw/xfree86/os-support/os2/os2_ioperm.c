/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/os2/os2_ioperm.c,v 3.0 1996/01/30 15:26:33 dawes Exp $ */
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



#define INCL_32
#define INCL_DOS
#include <os2.h>
#include <stdio.h>

/*
 * To access I/O ports under OS/2, we use the xf86sup.sys driver.
 * For the moment, we use a function which basically grants IO priviledge
 * to the whole server. NOTE: Once the server is running, we should 
 * change this to use inline IO functions through the callgate returned by 
 * the fastio$ driver.
 */

int ioEnabled=FALSE;
ULONG action;
char *ioDrvPath = "/dev/fastio$";
USHORT callgate[3]={0,0,0};

void xf86ClearIOPortList(ScreenNum)
int ScreenNum;
{
	return;
}

/* ARGSUSED */
void xf86AddIOPorts(ScreenNum, NumPorts, Ports)
int ScreenNum;
int NumPorts;
unsigned *Ports;
{
	return;

}

void xf86EnableIOPorts(ScreenNum)
int ScreenNum;
{

HFILE hfd;
	ULONG dlen;


	/* no need to call multiple times */
	if (ioEnabled) return;
	
	if (DosOpen((PSZ)ioDrvPath, (PHFILE)&hfd, (PULONG)&action,
	   (ULONG)0, FILE_SYSTEM, FILE_OPEN,
	   OPEN_SHARE_DENYNONE|OPEN_FLAGS_NOINHERIT|OPEN_ACCESS_READONLY,
	   (ULONG)0) != 0) {
		ErrorF("Error opening fastio$ driver...\n");
		ErrorF("Please install xf86sup.sys in config.sys!\n");
		return;
	}
	ErrorF("xf86-OS/2: Granting IOPL to X-server\n");

	callgate[0] = callgate[1] = 0;

/* Get callgate from driver for fast io to ports and other stuff */

	if (DosDevIOCtl(hfd, (ULONG)0x76, (ULONG)0x64,
		NULL, 0, NULL,
		(ULONG*)&callgate[2], sizeof(USHORT), &dlen) != 0) {
		ErrorF("IOCTL to fastio$ failed!\n");
		DosClose(hfd);
		return;
	}

/* Calling callgate with function 13 sets IOPL for the program */

	asm volatile ("movl $13,%%ebx;.byte 0xff,0x1d;.long _callgate"
			: /*no outputs */ 
			: /*no inputs */
			: "eax","ebx","ecx","edx","cc");

	ioEnabled = TRUE;
        DosClose(hfd);
	return;
}

void xf86DisableIOPorts(ScreenNum)
int ScreenNum;
{
HFILE hfd;
	ULONG dlen;


	/* no need to call multiple times */
	if (!ioEnabled) return;
	
	if (DosOpen((PSZ)ioDrvPath, (PHFILE)&hfd, (PULONG)&action,
	   (ULONG)0, FILE_SYSTEM, FILE_OPEN,
	   OPEN_SHARE_DENYNONE|OPEN_FLAGS_NOINHERIT|OPEN_ACCESS_READONLY,
	   (ULONG)0) != 0) {
		ErrorF("Error opening fastio$ driver...\n");
		ErrorF("Please install xf86sup.sys in config.sys!\n");
		return;
	}
	ErrorF("xf86-OS/2: Setting X-server back to ring 3\n");

	callgate[0] = callgate[1] = 0;

/* Get callgate from driver for fast io to ports and other stuff */

	if (DosDevIOCtl(hfd, (ULONG)0x76, (ULONG)0x64,
		NULL, 0, NULL,
		(ULONG*)&callgate[2], sizeof(USHORT), &dlen) != 0) {
		ErrorF("IOCTL to fastio$ failed!\n");
		DosClose(hfd);
		return;
	}

/* Function 14 of callgate brings program back to ring 3 */

	asm volatile ("movl $14,%%ebx;.byte 0xff,0x1d;.long _callgate"
			: /*no outputs */ 
			: /*no inputs */
			: "eax","ebx","ecx","edx","cc");
	ioEnabled=FALSE;
        DosClose(hfd);
	return;

}

void xf86DisableIOPrivs()
{
	return;
}
