/* $XFree86: xc/programs/Xserver/hw/xfree86/SuperProbe/ATIMach.c,v 3.5 1995/01/28 15:46:49 dawes Exp $ */
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

/* $XConsortium: ATIMach.c /main/7 1995/11/13 11:11:44 kaleb $ */

#include "Probe.h"

static Word Ports[] = {ROM_ADDR_1,DESTX_DIASTP,READ_SRC_X,
		       CONFIG_STATUS_1,MISC_OPTIONS,GP_STAT,
		       SCRATCH_REG0, MEM_INFO};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_ATIMach __STDCARGS((int));

Chip_Descriptor ATIMach_Descriptor = {
	"ATI_Mach",
	Probe_ATIMach,
	Ports,
	NUMPORTS,
	TRUE,
	FALSE,
	FALSE,
	MemProbe_ATIMach,
};

#define WaitIdleEmpty() { int i; \
			  for (i=0; i < 100000; i++) \
				if (!(inpw(GP_STAT) & (GPBUSY | 1))) \
					break; \
			}

Bool Probe_ATIMach(Chipset)
int *Chipset;
{
	Long tmp;
	static int chip = -1;
	static Bool Already_Called = FALSE;

	if (Already_Called)
	{
		if (chip != -1)
			*Chipset = chip;
		return (chip != -1);
	}
	Already_Called = TRUE;

	EnableIOPorts(NUMPORTS, Ports);

	/*
	 * Check for 8514/A registers.  Don't read BIOS, or an attached 8514
	 * Ultra won't be detected (the slave SVGA's BIOS is in the normal SVGA
	 * place).
	 */
	tmp = inpw(ROM_ADDR_1);
	outpw(ROM_ADDR_1, 0x5555);
	WaitIdleEmpty();
	if (inpw(ROM_ADDR_1) == 0x5555)
	{
		outpw(ROM_ADDR_1, 0x2A2A);
		WaitIdleEmpty();
		if (inpw(ROM_ADDR_1) == 0x2A2A)
			chip = CHIP_8514;
	}
	outpw(ROM_ADDR_1, tmp);
	if (chip != -1)
	{
		/*
		 * An 8514 accelerator is really present;  now figure
		 * out which one.
		 */
		outpw(DESTX_DIASTP, 0xAAAA);
		WaitIdleEmpty();
		if (inpw(READ_SRC_X) != 0x02AA)
			chip = CHIP_MACH8;
		else
			chip = CHIP_MACH32;
		outpw(DESTX_DIASTP, 0x5555);
		WaitIdleEmpty();
		if (inpw(READ_SRC_X) != 0x0555)
		{
			if (chip != CHIP_MACH8)
				/*
				 * Something bizarre is happening.
				 */
				chip = -1;
		}
		else
		{
			if (chip != CHIP_MACH32)
				/*
				 * Something bizarre is happening.
				 */
				chip = -1;
		}
	}

	if (chip == -1)
	{
		/* 
		 * Check for a Mach64.
		 */
		tmp = inpl(SCRATCH_REG0);
		outpl(SCRATCH_REG0, 0x55555555);	 /* Test odd bits */
		if (inpl(SCRATCH_REG0) == 0x55555555)
		{
			outpl(SCRATCH_REG0, 0xAAAAAAAA); /* Test even bits */
			if (inpl(SCRATCH_REG0) == 0xAAAAAAAA)
				chip = CHIP_MACH64;
		}
		outpl(SCRATCH_REG0, tmp);
	}

	if (chip != -1)
	{
		*Chipset = chip;
	}

	DisableIOPorts(NUMPORTS, Ports);
	return(chip != -1);
}

static int MemProbe_ATIMach(Chipset)
int Chipset;
{
	static int Mem = 0;
	static Bool Already_Called = FALSE;

	if (Already_Called)
		return (Mem);
	Already_Called = TRUE;

	EnableIOPorts(NUMPORTS, Ports);
	if (Chipset == CHIP_MACH8)
	{
		if (inpw(CONFIG_STATUS_1) & 0x0020)
		{
			Mem = 1024;
		}
		else
		{
			Mem = 512;
		}
	}
	else if (Chipset == CHIP_MACH32)
	{
		switch ((inpw(MISC_OPTIONS) & 0x000C) >> 2)
		{
		case 0x00:
			Mem = 512;
			break;
		case 0x01:
			Mem = 1024;
			break;
		case 0x02:
			Mem = 2048;
			break;
		case 0x03:
			Mem = 4096;
			break;
		}
	}
	else if (Chipset == CHIP_MACH64)
	{
		switch (inpl(MEM_INFO) & 0x00000007)
		{
		case 0x00:
			Mem = 512;
			break;
		case 0x01:
			Mem = 1024;
			break;
		case 0x02:
			Mem = 2048;
			break;
		case 0x03:
			Mem = 4096;
			break;
		case 0x04:
			Mem = 6144;
			break;
		case 0x05:
			Mem = 8192;
			break;
		case 0x06:
			Mem = 12288;
			break;
		case 0x07:
			Mem = 8192;
			break;
		}
	}
	DisableIOPorts(NUMPORTS, Ports);
	return(Mem);
}
