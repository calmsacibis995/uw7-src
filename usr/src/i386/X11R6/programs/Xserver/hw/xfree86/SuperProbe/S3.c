/* $XFree86: xc/programs/Xserver/hw/xfree86/SuperProbe/S3.c,v 3.7 1995/07/17 12:39:39 dawes Exp $ */
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

/* $XConsortium: S3.c /main/6 1995/11/13 11:13:16 kaleb $ */

#include "Probe.h"

static Word Ports[] = {0x000, 0x000};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_S3 __STDCARGS((int));

Chip_Descriptor S3_Descriptor = {
	"S3",
	Probe_S3,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	FALSE,
	MemProbe_S3,
};

Bool Probe_S3(Chipset)
int *Chipset;
{
	Bool result = FALSE;
	Byte old, tmp, rev;

	/* Add CRTC to enabled ports */
	Ports[0] = CRTC_IDX;
	Ports[1] = CRTC_REG;
	EnableIOPorts(NUMPORTS, Ports);
	old = rdinx(CRTC_IDX, 0x38);
	wrinx(CRTC_IDX, 0x38, 0x00);
	if (!testinx2(CRTC_IDX, 0x35, 0x0F))
	{
		wrinx(CRTC_IDX, 0x38, 0x48);
		if (testinx2(CRTC_IDX, 0x35, 0x0F))
		{
			result = TRUE;
			rev = rdinx(CRTC_IDX, 0x30);
			switch (rev & 0xF0)
			{
			case 0x80:
				switch (rev & 0x0F)
				{
				case 0x01:
					*Chipset = CHIP_S3_911;
					break;
				case 0x02:
					*Chipset = CHIP_S3_924;
					break;
				default:
					Chip_data = rev;
					*Chipset = CHIP_S3_UNKNOWN;
					break;
				}
				break;
			case 0xA0:
				tmp = rdinx(CRTC_IDX, 0x36);
				switch (tmp & 0x03)
				{
				case 0x00:
				case 0x01:
					/* EISA or VLB - 805 */
					switch (rev & 0x0F)
					{
					case 0x00:
						*Chipset = CHIP_S3_805B;
						break;
					case 0x01:
						Chip_data = rev;
						*Chipset = CHIP_S3_UNKNOWN;
						break;
					case 0x02:
					case 0x03:
					case 0x04:
						*Chipset = CHIP_S3_805C;
						break;
					case 0x05:
						*Chipset = CHIP_S3_805D;
						break;
					case 0x08:
						*Chipset = CHIP_S3_805I;
						break;
					default:
						/* Call >0x05 D step for now */
						*Chipset = CHIP_S3_805D;
						break;
					}
					break;
				case 0x03:
					/* ISA - 801 */
					switch (rev & 0x0F)
					{
					case 0x00:
						*Chipset = CHIP_S3_801B;
						break;
					case 0x01:
						Chip_data = rev;
						*Chipset = CHIP_S3_UNKNOWN;
						break;
					case 0x02:
					case 0x03:
					case 0x04:
						*Chipset = CHIP_S3_801C;
						break;
					case 0x05:
						*Chipset = CHIP_S3_801D;
						break;
					case 0x08:
						*Chipset = CHIP_S3_801I;
						break;
					default:
						/* Call >0x05 D step for now */
						*Chipset = CHIP_S3_801D;
						break;
					}
					break;
				default:
					Chip_data = rev;
					*Chipset = CHIP_S3_UNKNOWN;
					break;
				}
				break;
			case 0x90:
				switch (rev & 0x0F)
				{
				case 0x00:
				case 0x01:
					/*
					 * Contradictory documentation -
					 * one says 0, the other says 1.
					 */
					*Chipset = CHIP_S3_928D;
					break;
				case 0x02:
				case 0x03:
					Chip_data = rev;
					*Chipset = CHIP_S3_UNKNOWN;
					break;
				case 0x04:
				case 0x05:
					*Chipset = CHIP_S3_928E;
					break;
				default:
					/* Call >0x05 E step for now */
					*Chipset = CHIP_S3_928E;
				}
				break;
			case 0xB0:
				/*
				 * Don't know anything more about this
				 * just yet.
				 */
				*Chipset = CHIP_S3_928P;
				break;
			case 0xC0:
				*Chipset = CHIP_S3_864;
				break;
			case 0xD0:
				*Chipset = CHIP_S3_964;
				break;
			case 0xE0: {
			   Byte chip_id_high, chip_id_low, chip_rev;
			   chip_id_high = rdinx(CRTC_IDX, 0x2d);
			   chip_id_low  = rdinx(CRTC_IDX, 0x2e);
			   chip_rev     = rdinx(CRTC_IDX, 0x2f);
			   if      (chip_id_low==0x80) 
			      *Chipset = CHIP_S3_866;
			   else if (chip_id_low==0x90) 
			      *Chipset = CHIP_S3_868;
			   else if (chip_id_low==0x10) 
			      *Chipset = CHIP_S3_Trio32;
			   else if (chip_id_low==0x11)
			      if ((chip_rev&0x40) == 0x40)
				 *Chipset = CHIP_S3_Trio64V;
			      else
				 *Chipset = CHIP_S3_Trio64;
			   else if (chip_id_low==0xf0) 
			      *Chipset = CHIP_S3_968;
			   else {
			      Chip_data = rev;
			      Chip_data = (Chip_data << 8) | chip_id_high;
			      Chip_data = (Chip_data << 8) | chip_id_low;
			      Chip_data = (Chip_data << 8) | chip_rev;
			      *Chipset = CHIP_S3_UNKNOWN;
			   }
			   break;				 
			}
			default:
				Chip_data = rev;
				*Chipset = CHIP_S3_UNKNOWN;
				break;
			}
		}
	}
	wrinx(CRTC_IDX, 0x38, old);
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

static int MemProbe_S3(Chipset)
int Chipset;
{
	Byte config, old;
	int Mem = 0;

	EnableIOPorts(NUMPORTS, Ports);

	old = rdinx(CRTC_IDX, 0x38);
	wrinx(CRTC_IDX, 0x38, 0x00);
	if (!testinx2(CRTC_IDX, 0x35, 0x0F))
	{
		wrinx(CRTC_IDX, 0x38, 0x48);
		if (testinx2(CRTC_IDX, 0x35, 0x0F))
		{
			config = rdinx(CRTC_IDX, 0x36);
			if ((config & 0x20) != 0)
			{
				Mem = 512;
			}
			else
			{
				if ((Chipset == CHIP_S3_911) || 
				    (Chipset == CHIP_S3_924))
				{
					Mem = 1024;
				}
				else
				{
					switch((config & 0xC0) >> 6)
					{
					case 0:
						Mem = 4096;
						break;
					case 1:
						Mem = 3072;
						break;
					case 2:
						Mem = 2048;
						break;
					case 3:
						Mem = 1024;
						break;
					}
				}
			}
		}
	}
	wrinx(CRTC_IDX, 0x38, old);

	DisableIOPorts(NUMPORTS, Ports);
	return(Mem);
}
