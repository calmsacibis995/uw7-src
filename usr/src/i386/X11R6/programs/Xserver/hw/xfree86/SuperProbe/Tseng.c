/* $XFree86: xc/programs/Xserver/hw/xfree86/SuperProbe/Tseng.c,v 3.4 1995/11/16 11:04:26 dawes Exp $ */
/*
 * (c) Copyright 1993,1994 by David Wexelblat <dwex@xfree86.org>
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
 * DAVID WEXELBLAT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of David Wexelblat shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from David Wexelblat.
 *
 */

/* $XConsortium: Tseng.c /main/5 1995/11/16 10:10:46 kaleb $ */

#include "Probe.h"

static Word Ports[] = {0x000, 0x000, 0x000, 0x3BF, 0x3CB, 0x3CD, 
		       ATR_IDX, ATR_REG_R, SEQ_IDX, SEQ_REG};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_Tseng __STDCARGS((int));

Chip_Descriptor Tseng_Descriptor = {
	"Tseng",
	Probe_Tseng,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	MemProbe_Tseng,
};

Bool Probe_Tseng(Chipset)
int *Chipset;
{
	Bool result = FALSE;
	Byte old, old1, ver;

	/* Add CRTC to enabled ports */
	Ports[0] = CRTC_IDX;
	Ports[1] = CRTC_REG;
	Ports[2] = inp(MISC_OUT_R) & 0x01 ? 0x3D8 : 0x3B8;
	EnableIOPorts(NUMPORTS, Ports);

	old = inp(0x3BF);
	old1 = inp(Ports[2]);
	outp(0x3BF, 0x03);
	outp(Ports[2], 0xA0);
	if (tstrg(0x3CD, 0x3F)) 
	{
		result = TRUE;
		if (testinx2(CRTC_IDX, 0x33, 0x0F))
		{
			if (tstrg(0x3CB, 0x33))
			{
				ver = rdinx(0x217A, 0xEC);
				switch (ver >> 4)
				{
				case 0x00:
					*Chipset = CHIP_ET4000W32;
					break;
				case 0x01:
					*Chipset = CHIP_ET4000W32I;
					break;
				case 0x02:
					*Chipset = CHIP_ET4KW32P_A;
					break;
				case 0x03:
					*Chipset = CHIP_ET4KW32I_B;
					break;
				case 0x05:
					*Chipset = CHIP_ET4KW32P_B;
					break;
				case 0x06:
					*Chipset = CHIP_ET4KW32P_D;
					break;
				case 0x07:
					*Chipset = CHIP_ET4KW32P_C;
					break;
				case 0x0B:
					*Chipset = CHIP_ET4KW32I_C;
					break;
				default:
					Chip_data = ver >> 4;
					*Chipset = CHIP_TSENG_UNK;
					break;
				}
			}
			else
			{
				*Chipset = CHIP_ET4000;
			}
		}
		else
		{
			*Chipset = CHIP_ET3000;
		}
	}
	outp(Ports[2], old1);
	outp(0x3BF, old);

	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

static int MemProbe_Tseng(Chipset)
int Chipset;
{
	Byte Save[2];
	int Mem = 0;

	EnableIOPorts(NUMPORTS, Ports);

	/* 
	 * Unlock 
	 */
	Save[0] = inp(0x3BF);
	Save[1] = inp(vgaIOBase + 8);
	outp(0x3BF, 0x03);
	outp(vgaIOBase + 8, 0xA0);

	/*
	 * Check
	 */
	switch (Chipset)
	{
	case CHIP_ET3000:
		Mem = 512;
		break;
	case CHIP_ET4000:
		switch (rdinx(CRTC_IDX, 0x37) & 0x0B)
		{
		case 0x03:
		case 0x05:
			Mem = 256;
			break;
		case 0x0A:
			Mem = 512;
			break;
		case 0x0B:
			Mem = 1024;
			break;
		}
		break;
	case CHIP_ET4000W32:
	case CHIP_ET4000W32I:
	case CHIP_ET4KW32P_A:
	case CHIP_ET4KW32I_B:
	case CHIP_ET4KW32I_C:
	case CHIP_ET4KW32P_B:
	case CHIP_ET4KW32P_C:
	case CHIP_ET4KW32P_D:
		switch (rdinx(CRTC_IDX, 0x37) & 0x09)
		{
		case 0x00:
			Mem = 2048;
			break;
		case 0x01:
			Mem = 4096;
			break;
		case 0x08:
			Mem = 512;
			break;
		case 0x09:
			Mem = 1024;
			if ((Chipset != CHIP_ET4000W32) &&
			   (rdinx(CRTC_IDX, 0x32) & 0x80))
			    Mem = 2048;
			break;
		}
	}

	/* 
	 * Lock 
	 */
	outp(vgaIOBase + 8, Save[1]);
	outp(0x3BF, Save[0]);
	DisableIOPorts(NUMPORTS, Ports);

	return(Mem);
}
