/* $XFree86: xc/programs/Xserver/hw/xfree86/etc/ati.test.c,v 3.11 1995/05/27 03:12:08 dawes Exp $ */
/* ati.test.c -- Gather information about ATI video adapters.
 * Created: Sun Aug  9 10:15:01 1992
 * Author: Rickard E. Faith, faith@cs.unc.edu
 * Copyright 1992 Rickard E. Faith; All Rights Reserved.
 * May be distributed freely for any purpose as long as the
 * copyright notice and the warranty disclaimer are kept.
 * This programme comes with ABSOLUTELY NO WARRANTY.
 *
 * Log
 * - Heavily modified in 1994 and 1995 by Marc Aurele La France, tsi@ualberta.ca
 *
 * Please see main driver (ati_driver.c) for acknowledgements.
 */
/* $XConsortium: ati.test.c /main/7 1995/11/12 20:16:47 kaleb $ */

#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include "regati.h"

#ifdef linux

#define IOPL_ON		iopl(3)
#define IOPL_OFF	iopl(0)

#elif defined(MACH386)

static int iopl_fd;
#define IOPL_ON		iopl_fd = open("/dev/iopl", 0)
#define IOPL_OFF	close(iopl_fd);

#elif defined(__bsdi__)

static int console_fd;
#define IOPL_ON								\
	{								\
		console_fd = open("/dev/kbd", O_RDONLY | O_NDELAY, 0);	\
		ioctl(console_fd, PCCONENABIOPL, 0);			\
	}
#define IOPL_OFF							\
	{								\
		ioctl(console_fd, PCCONDISABIOPL, 0);			\
		close(console_fd);					\
	}

#elif defined(USL) || defined(SYSV386)

/* #include <where sysi86 is defined>.h */
#define IOPL_ON		sysi86(SI86V86, V86SC_IOPL, PS_IOPL)
#define IOPL_OFF	sysi86(SI86V86, V86SC_IOPL, 0)

#else

#define IOPL_ON		/* Nothing */
#define IOPL_OFF	/* Nothing */

#endif

static __inline__ void outb(const unsigned short port,
			    const unsigned char value)
{
   __asm__ volatile ("outb %0,%1"
		     ::"a" ((char) value),"d" ((unsigned short) port));
}
static __inline__ void outw(const unsigned short port,
			    const unsigned short value)
{
   __asm__ volatile ("outw %0,%1"
		     ::"a" ((short) value),"d" ((unsigned short) port));
}
static __inline__ void outl(const unsigned short port,
			    const unsigned long value)
{
   __asm__ volatile ("outl %0,%1"
		     ::"a" ((long) value),"d" ((unsigned short) port));
}

static __inline__ unsigned char inb(const unsigned short port)
{
   unsigned char _v;
   __asm__ volatile ("inb %1,%0"
		     :"=a" (_v):"d" ((unsigned short) port));
   return _v;
}
static __inline__ unsigned short inw(const unsigned short port)
{
   unsigned short _v;
   __asm__ volatile ("inw %1,%0"
		     :"=a" (_v):"d" ((unsigned short) port));
   return _v;
}
static __inline__ unsigned long inl(const unsigned short port)
{
   unsigned long _v;
   __asm__ volatile ("inl %1,%0"
		     :"=a" (_v):"d" ((unsigned short) port));
   return _v;
}

static int ATIExtReg = 0x01CE;

#define GetReg(Register, Index)		\
	(				\
		outb(Register, Index),	\
		inb(Register + 1)	\
	)
#define PutReg(Register, Index, Register_Value)	\
	outw(Register, ((Register_Value) << 8) | (Index))

#define FALSE 0
#define TRUE  1

#define ATI_CHIP_NONE      0
#define ATI_CHIP_18800     1
#define ATI_CHIP_18800_1   2
#define ATI_CHIP_28800_2   3
#define ATI_CHIP_28800_4   4
#define ATI_CHIP_28800_5   5
#define ATI_CHIP_28800_6   6
#define ATI_CHIP_38800     7	/* Mach8 */
#define ATI_CHIP_68800     8	/* Mach32 */
#define ATI_CHIP_68800_3   9	/* Mach32 */
#define ATI_CHIP_68800_6  10	/* Mach32 */
#define ATI_CHIP_68800LX  11	/* Mach32 */
#define ATI_CHIP_68800AX  12	/* Mach32 */
#define ATI_CHIP_88800    13	/* Mach64 */
#define ATI_CHIP_88800CX  14	/* Mach64 */
#define ATI_CHIP_88800GX  15	/* Mach64 */
static unsigned char ATIChip = ATI_CHIP_NONE;
static const char *ChipNames[] =
{
	"Unknown",
	"ATI 18800",
	"ATI 18800-1",
	"ATI 28800-2",
	"ATI 28800-4",
	"ATI 28800-5",
	"ATI 28800-6",
	"ATI 38800",
	"ATI 68800",
	"ATI 68800-3",
	"ATI 68800-6",
	"ATI 68800LX",
	"ATI 68800AX",
	"ATI 88800",
	"ATI 88800CX",
	"ATI 88800GX"
};

#define ATI_BOARD_NONE    0
#define ATI_BOARD_EGA     1
#define ATI_BOARD_BASIC   2
#define ATI_BOARD_V3      3
#define ATI_BOARD_V4      4
#define ATI_BOARD_V5      5
#define ATI_BOARD_PLUS    6
#define ATI_BOARD_XL      7
#define ATI_BOARD_NONISA  8
#define ATI_BOARD_MACH8   9
#define ATI_BOARD_MACH32 10
#define ATI_BOARD_MACH64 11
static unsigned char ATIBoard = ATI_BOARD_NONE;
static unsigned char ATIVGABoard = ATI_BOARD_NONE;
static const char *BoardNames[] =
{
	"Unknown",
	"EGA Wonder800+",
	"VGA Basic16",
	"VGA Wonder V3",
	"VGA Wonder V4",
	"VGA Wonder V5",
	"VGA Wonder+",
	"VGA Wonder XL or XL24",
	"VGA Wonder VLB or PCI",
	"Mach8",
	"Mach32",
	"Mach64"
};

#define ATI_DAC_ATI68830 0
#define ATI_DAC_SC11483  1
#define ATI_DAC_ATI68875 2
#define ATI_DAC_GENERIC  3
#define ATI_DAC_BT481    4
#define ATI_DAC_ATI68860 5
#define ATI_DAC_STG1700  6
#define ATI_DAC_SC15021  7
static unsigned char ATIDac = ATI_DAC_GENERIC;
static const char *DACNames[] =
{
	"ATI 68830",
	"Sierra 11483",
	"ATI 68875",
	"Brooktree 476",
	"Brooktree 481",
	"ATI 68860",
	"STG 1700",
	"Sierra 15021"
};

static unsigned short int ChipType = 0, ChipClass = 0, ChipRevision = 0;

/*
 * Because the only practical standard C library is an inadequate lowest
 * common denominator...
 */
static void *
findsubstring(const void * needle, const int needle_length,
	      const void * haystack, int haystack_length)
{
	const unsigned char * haystack_pointer;
	const unsigned char * needle_pointer = needle;
	int compare_length;

	haystack_length -= needle_length;
	for (haystack_pointer = haystack;
	     haystack_length >= 0;
	     haystack_pointer++, haystack_length--)
		for (compare_length = 0; ; compare_length++)
		{
			if (compare_length >= needle_length)
				return (void *) haystack_pointer;
			if (haystack_pointer[compare_length] !=
			    needle_pointer[compare_length])
				break;
		}

	return (void *) 0;
}

/*
 * Extract values out of a 68800's CHIP_ID register.
 */
static void
ATIMach32ChipID(void)
{
	int IO_Value = inw(CHIP_ID);
	ChipType = IO_Value & (CHIP_CODE_0 | CHIP_CODE_1);
	ChipClass = (IO_Value & CHIP_CLASS) >> 10;
	ChipRevision = (IO_Value & CHIP_REV) >> 12;
	if (IO_Value == 0xFFFF)
		IO_Value = 0;
	switch (IO_Value & (CHIP_CODE_0 | CHIP_CODE_1))
	{
		case 0x0000:
			ATIChip = ATI_CHIP_68800_3;
			break;

		case 0x02F7:
			ATIChip = ATI_CHIP_68800_6;
			break;

		case 0x0177:
			ATIChip = ATI_CHIP_68800LX;
			break;

		case 0x0017:
			ATIChip = ATI_CHIP_68800AX;
			break;

		default:
			ATIChip = ATI_CHIP_68800;
			break;
	}
}

/*
 * Extract values out of an 88800's CONFIG_CHIP_ID register.
 */
static void
ATIMach64ChipID(void)
{
	unsigned int IO_Value = inl(CONFIG_CHIP_ID);
	ChipType     = (IO_Value & 0xFFFF        )      ;
	ChipClass    = (IO_Value & CFG_CHIP_CLASS) >> 16;
	ChipRevision = (IO_Value & CFG_CHIP_REV  ) >> 24;
	switch (ChipType)
	{
		case 0x0057:
			ATIChip = ATI_CHIP_88800CX;
			break;

		case 0x00D7:
			ATIChip = ATI_CHIP_88800GX;
			break;

		default:
			ATIChip = ATI_CHIP_88800;
			break;
	}
}

typedef unsigned short Colour;	/* The correct spelling should be OK :-) */

/*
 * Bit patterns which are extremely unlikely to show up when reading from
 * nonexistant memory (which normally shows up as either all bits set or all
 * bits clear).
 */
static const Colour Test_Pixel[] = {0x5aa5, 0x55aa, 0xa55a, 0xca53};

#define NUMBER_OF_TEST_PIXELS (sizeof(Test_Pixel) / sizeof(Test_Pixel[0]))

static const struct
{
	int videoRamSize;
	int Miscellaneous_Options_Setting;
	struct
	{
		short x, y;
	}
	Coordinates[NUMBER_OF_TEST_PIXELS + 1];
}
Test_Case[] =
{
	/*
	 * Given the engine settings used, only a 4M card will have enough
	 * memory to back up the 1025th line of the display.  Since the pixel
	 * coordinates are zero-based, line 1024 will be the first one which
	 * is only backed on 4M cards.
	 *
	 * <Mark_Weaver@brown.edu>:
	 * In case memory is being wrapped, (0,0) and (0,1024) to make sure
	 * they can each hold a unique value.
	 */
	{4096, MEM_SIZE_4M, {{0,0}, {0,1024}, {-1,-1}}},

	/*
	 * This card has 2M or less.  On a 1M card, the first 2M of the card's
	 * memory will have even doublewords backed by physical memory and odd
	 * doublewords unbacked.
	 *
	 * Pixels 0 and 1 of a row will be in the zeroth doubleword, while
	 * pixels 2 and 3 will be in the first.  Check both pixels 2 and 3 in
	 * case this is a pseudo-1M card (one chip pulled to turn a 2M card
	 * into a 1M card).
	 *
	 * <Mark_Weaver@brown.edu>:
	 * I don't have a 1M card, so I'm taking a stab in the dark.  Maybe
	 * memory wraps every 512 lines, or maybe odd doublewords are aliases
	 * of their even doubleword counterparts.  I try everything here.
	 */
	{2048, MEM_SIZE_2M, {{0,0}, {0,512}, {2,0}, {3,0}, {-1,-1}}},

	/*
	 * This is a either a 1M card or a 512k card.  Test pixel 1, since it
	 * is an odd word in an even doubleword.
	 *
	 * <Mark_Weaver@brown.edu>:
	 * This is the same idea as the test above.
	 */
	{1024, MEM_SIZE_1M, {{0,0}, {0,256}, {1,0}, {-1,-1}}},

	/*
	 * Assume it is a 512k card by default, since that is the minimum
	 * configuration.
	 */
	{512, MEM_SIZE_512K, {{-1,-1}}}
};

#define NUMBER_OF_TEST_CASES (sizeof(Test_Case) / sizeof(Test_Case[0]))

/*
 * ATIMach32ReadPixel --
 *
 * Return the colour of the specified screen location.  Called from
 * ATIMach32videoRam routine below.
 */
static Colour
ATIMach32ReadPixel(const short int X, const short int Y)
{
	Colour Pixel_Colour;

	/* Wait for idle engine */
	ProbeWaitIdleEmpty();

	/* Set up engine for pixel read */
	ATIWaitQueue(7);
	outw(RD_MASK, (unsigned short)(~0));
	outw(DP_CONFIG, FG_COLOR_SRC_BLIT | DATA_WIDTH | DRAW | DATA_ORDER);
	outw(CUR_X, X);
	outw(CUR_Y, Y);
	outw(DEST_X_START, X);
	outw(DEST_X_END, X + 1);
	outw(DEST_Y_END, Y + 1);

	/* Wait for data to become ready */
	ATIWaitQueue(16);
	WaitDataReady();

	/* Read pixel colour */
	Pixel_Colour = inw(PIX_TRANS);
	ProbeWaitIdleEmpty();
	return Pixel_Colour;
}

/*
 * ATIMach32WritePixel --
 *
 * Set the colour of the specified screen location.  Called from
 * ATIMach32videoRam routine below.
 */
static void
ATIMach32WritePixel(const short int X, const short int Y,
		    const Colour Pixel_Colour)
{
	/* Set up engine for pixel write */
	ATIWaitQueue(9);
	outw(WRT_MASK, (unsigned short)(~0));
	outw(DP_CONFIG, FG_COLOR_SRC_FG | DRAW | READ_WRITE);
	outw(ALU_FG_FN, MIX_FN_PAINT);
	outw(FRGD_COLOR, Pixel_Colour);
	outw(CUR_X, X);
	outw(CUR_Y, Y);
	outw(DEST_X_START, X);
	outw(DEST_X_END, X + 1);
	outw(DEST_Y_END, Y + 1);
}

/*
 * ATIMach32videoRam --
 *
 * Determine the amount of video memory installed on an 68800-6 based adapter.
 * This is done because these chips exhibit a bug that causes their
 * MISC_OPTIONS register to report 1M rather than the true amount of memory.
 *
 * This routine is adapted from a similar routine in mach32mem.c written by
 * Robert Wolff, David Dawes and Mark Weaver.
 */
static int
ATIMach32videoRam(void)
{
	unsigned short saved_CLOCK_SEL, saved_MEM_BNDRY,
		saved_MISC_OPTIONS, saved_EXT_GE_CONFIG;
	Colour saved_Pixel[NUMBER_OF_TEST_PIXELS];
	int Case_Number, Pixel_Number;
	unsigned char AllPixelsOK;

	/* Save register values to be modified */
	saved_CLOCK_SEL = inw(CLOCK_SEL);
	saved_MEM_BNDRY = inw(MEM_BNDRY);
	saved_MISC_OPTIONS = inw(MISC_OPTIONS);
	saved_EXT_GE_CONFIG = inw(R_EXT_GE_CONFIG);

	/* Wait for enough FIFO entries */
	ATIWaitQueue(7);

	/* Enable accelerator */
	outw(CLOCK_SEL, saved_CLOCK_SEL | DISABPASSTHRU);

	/* Make the accelerator and VGA share video memory */
	outw(MEM_BNDRY, saved_MEM_BNDRY & ~(MEM_PAGE_BNDRY | MEM_BNDRY_ENA));

	/* Prevent video memory wrap */
	outw(MISC_OPTIONS, saved_MISC_OPTIONS | MEM_SIZE_4M);

	/*
	 * Set up the drawing engine for a pitch of 1024 at 16 bits per pixel.
	 * No need to mess with the CRT because the results of this test are
	 * not intended to be seen.
	 */
	outw(EXT_GE_CONFIG, PIX_WIDTH_16BPP | ORDER_16BPP_565 |
		MONITOR_8514 | ALIAS_ENA);
	outw(GE_PITCH, 1024 >> 3);
	outw(GE_OFFSET_HI, 0);
	outw(GE_OFFSET_LO, 0);

	for (Case_Number = 0;
	     Case_Number < (NUMBER_OF_TEST_CASES - 1);
	     Case_Number++)
	{
		/* Reduce redundancy as per Mark_Weaver@brown.edu */
#		define TestPixel               \
			Test_Case[Case_Number].Coordinates[Pixel_Number]
#		define ForEachTestPixel        \
			for (Pixel_Number = 0; \
			     TestPixel.x >= 0; \
			     Pixel_Number++)

		/* Save pixel colours that will be clobbered */
		ForEachTestPixel
			saved_Pixel[Pixel_Number] =
				ATIMach32ReadPixel(TestPixel.x, TestPixel.y);

		/* Write test patterns */
		ForEachTestPixel
			ATIMach32WritePixel(TestPixel.x, TestPixel.y,
				Test_Pixel[Pixel_Number]);

		/* Test for lost pixels */
		AllPixelsOK = TRUE;
		ForEachTestPixel
			if (ATIMach32ReadPixel(TestPixel.x, TestPixel.y) !=
			    Test_Pixel[Pixel_Number])
			{
				AllPixelsOK = FALSE;
				break;
			}

		/* Restore clobbered pixels */
		ForEachTestPixel
			ATIMach32WritePixel(TestPixel.x, TestPixel.y,
				saved_Pixel[Pixel_Number]);

		/* End test on success */
		if (AllPixelsOK)
			break;

		/* Completeness */
#		undef ForEachTestPixel
#		undef TestPixel
	}

	/* Restore what was changed */
	ATIWaitQueue(4);
	outw(EXT_GE_CONFIG, saved_EXT_GE_CONFIG);
	outw(MISC_OPTIONS, saved_MISC_OPTIONS);
	outw(MEM_BNDRY, saved_MEM_BNDRY);

	/* Re-enable VGA passthrough */
	outw(CLOCK_SEL, saved_CLOCK_SEL & ~DISABPASSTHRU);

	/* Wait for activity to die down */
	ProbeWaitIdleEmpty();

	/* Tell caller the REAL story */
	return Test_Case[Case_Number].videoRamSize;
}

static void
PrintRegisters(const unsigned short Port, const unsigned char Start_Index,
	       const unsigned char End_Index, const char * Name,
	       const unsigned short GenS1)
{
	int Index;

	printf("\n\n Current %s register values:", Name);
	for (Index = Start_Index;  Index < End_Index;  Index++)
	{
		if((Index % 4) == 0)
		{
			if ((Index % 16) == 0)
				printf("\n 0x%02X:", Index);
			printf(" ");
		}
		if (Port == ATTRX)
			(void) inb(GenS1);	/* Reset flip-flop */
		printf("%02X", GetReg(Port, Index));
	}

	if (Port == ATTRX)
	{
		(void) inb(GenS1);		/* Reset flip-flop */
		outb(ATTRX, 0x20);		/* Turn on PAS bit */
	}
}

int
main(int argc, char **argv)
{
#	define Signature	" 761295520"
#	define Signature_Size	10
#	define BIOS_DATA_SIZE	(0x80 + Signature_Size)
	unsigned char		BIOS_Data[BIOS_DATA_SIZE];
#	define Signature_Offset	0x30
#	define BIOS_Signature	(BIOS_Data + Signature_Offset)
	unsigned char * Signature_Found;
	int fd;
	int Index, IO_Value = 0, IO_Value2;
	int MachvideoRam = 0;
	int VGAWondervideoRam = 0;
	const int videoRamSizes[] =
		{0, 256, 512, 1024, 2*1024, 4*1024, 6*1024, 8*1024, 12*1024,
		 8*1024, 0};
	unsigned char * ptr;
	unsigned char misc;
	unsigned char Reset_Before_Probe = FALSE;
	unsigned char Offset = 0x80;

	/* Parse options */
	for ( ;  argc > 1;  argc--, argv++)
	{
		if ((argv[1][0] == '-') && (argv[1][1] == 'r'))
			Reset_Before_Probe = TRUE;
	}

	if ((fd = open("/dev/mem", O_RDONLY)) < 0 ||
	    lseek(fd, 0xc0000, SEEK_SET) < 0 ||
	    read(fd, BIOS_Data, BIOS_DATA_SIZE) != BIOS_DATA_SIZE)
	{
		if (fd >= 0)
			close(fd);
		printf(" Failed to read VGA BIOS.  Cannot detect ATI card.\n");
		return 2;
	}
	close(fd);

	/* Look for signature in BIOS */
	Signature_Found =
		findsubstring(Signature, Signature_Size,
			BIOS_Data, BIOS_DATA_SIZE);
	if (!Signature_Found)
		printf(" ATI Signature not found in BIOS.\n");
	else if (Signature_Found != BIOS_Signature)
		printf(" ATI Signature found at offset 0x%04x in BIOS.\n",
			Signature_Found - BIOS_Data);

	IOPL_ON;				/* Enable I/O ports */

	/*
	 * Determine if an 8514/A compatible accelerator is present.
	 */
	if (Reset_Before_Probe)
	{
		/*
		 * Save register value to be modified, just in case there is no
		 * 8514/A accelerator.
		 */
		IO_Value = inw(SUBSYS_STAT);

		/*
		 * Ensure any 8514/A compatible adapter is not in a weird
		 * state.
		 */
		outw(SUBSYS_CNTL, GPCTRL_RESET | CHPTEST_NORMAL);
		outw(SUBSYS_CNTL, GPCTRL_ENAB | CHPTEST_NORMAL |
			RVBLNKFLG | RPICKFLAG | RINVALIDIO | RGPIDLE);
	}

	IO_Value2 = inw(ERR_TERM);
	outw(ERR_TERM, 0x5A5A);
	ProbeWaitIdleEmpty();
	if (inw(ERR_TERM) == 0x5A5A)
	{
		outw(ERR_TERM, 0x2525);
		ProbeWaitIdleEmpty();
		if (inw(ERR_TERM) == 0x2525)
			ATIBoard = ATI_BOARD_MACH8;
	}
	outw(ERR_TERM, IO_Value2);

	if (ATIBoard != ATI_BOARD_NONE)
	{
		/* Some kind of 8514/A detected */
		ATIBoard = ATI_BOARD_NONE;

		if (Reset_Before_Probe)
		{
			/*
			 * Don't leave any Mach8 or Mach32 in 8514/A mode.
			 */
			IO_Value2 = inw(CLOCK_SEL);
			outw(CLOCK_SEL, IO_Value2);
			ProbeWaitIdleEmpty();
		}

		IO_Value2 = inw(ROM_ADDR_1);
		outw(ROM_ADDR_1, 0x5555);
		ProbeWaitIdleEmpty();
		if (inw(ROM_ADDR_1) == 0x5555)
		{
			outw(ROM_ADDR_1, 0x2A2A);
			ProbeWaitIdleEmpty();
			if (inw(ROM_ADDR_1) == 0x2A2A)
				ATIBoard = ATI_BOARD_MACH8;
		}
		outw(ROM_ADDR_1, IO_Value2);
	}

	if (ATIBoard != ATI_BOARD_NONE)
	{
		/* ATI Mach8 or Mach32 accelerator detected */
		outw(DESTX_DIASTP, 0xAAAA);
		ProbeWaitIdleEmpty();
		if (inw(READ_SRC_X) == 0x02AA)
			ATIBoard = ATI_BOARD_MACH32;

		outw(DESTX_DIASTP, 0x5555);
		ProbeWaitIdleEmpty();
		if (inw(READ_SRC_X) == 0x0555)
		{
			if (ATIBoard != ATI_BOARD_MACH32)
				ATIBoard = ATI_BOARD_NONE;
		}
		else
		{
			if (ATIBoard != ATI_BOARD_MACH8)
				ATIBoard = ATI_BOARD_NONE;
		}
	}

	if (ATIBoard == ATI_BOARD_NONE)
	{
		/*
		 * Determine if a Mach64 is present.
		 */
		if (Reset_Before_Probe)
		{
			/*
			 * Restore register clobbered by 8514 reset attempt.
			 */
			outw(SUBSYS_CNTL, IO_Value);

			/*
			 * Ensure any Mach64 is not in some weird state.
			 */
			IO_Value = inl(BUS_CNTL);
			outl(BUS_CNTL, (IO_Value &
				~(BUS_ROM_DIS | BUS_FIFO_ERR_INT_EN |
				  BUS_HOST_ERR_INT_EN)) |
				BUS_FIFO_ERR_INT | BUS_HOST_ERR_INT);
			IO_Value = inl(GEN_TEST_CNTL) &
				(GEN_OVR_OUTPUT_EN | GEN_OVR_POLARITY |
				 GEN_CUR_EN | GEN_BLOCK_WR_EN);
			outl(GEN_TEST_CNTL, IO_Value);
			outl(GEN_TEST_CNTL, IO_Value | GEN_GUI_EN);
		}

		IO_Value = inl(SCRATCH_REG0);
		outl(SCRATCH_REG0, 0x55555555);		/* Test odd bits */
		if (inl(SCRATCH_REG0) == 0x55555555)
		{
			outl(SCRATCH_REG0, 0xAAAAAAAA);	/* Test even bits */
			if (inl(SCRATCH_REG0) == 0xAAAAAAAA)
			{
				/* Mach64 detected */
				IO_Value2 = inl(CONFIG_STATUS_0);
				if ((IO_Value2 & (CFG_VGA_EN | CFG_CHIP_EN)) !=
				    (CFG_VGA_EN | CFG_CHIP_EN))
					printf("Mach64 detected but VGAWonder"
					       " capability cannot be"
					       " enabled.\n");
				ATIBoard = ATI_BOARD_MACH64;
				ATIDac = (IO_Value2 & CFG_INIT_DAC_TYPE) >> 9;
				MachvideoRam =
					videoRamSizes[(inl(MEM_INFO) & 0x0007) +
						2];

				ATIMach64ChipID();
			}
		}
		outl(SCRATCH_REG0, IO_Value);
	}

	else if (ATIBoard == ATI_BOARD_MACH32)
	{
		IO_Value = inw(CONFIG_STATUS_1);
		if (IO_Value & (_8514_ONLY | CHIP_DIS))
			printf("Mach32 detected but VGA Wonder capability"
			       " cannot be enabled.\n");
		ATIDac = (IO_Value & DACTYPE) >> 9;

		ATIMach32ChipID();

		MachvideoRam =
			videoRamSizes[((inw(MISC_OPTIONS) & MEM_SIZE_ALIAS) >>
				2) + 2];

		/*
		 * The 68800-6 doesn't necessarily report the correct video
		 * memory size.
		 */
		if ((ATIChip == ATI_CHIP_68800_6) && (MachvideoRam == 1024))
			MachvideoRam = ATIMach32videoRam();

	}
	else if (ATIBoard == ATI_BOARD_MACH8)
		MachvideoRam =
			videoRamSizes[((inw(CONFIG_STATUS_1) & MEM_INSTALLED) >>
				5) + 2];

	if (ATIBoard <= ATI_BOARD_MACH8)
	if (Signature_Found == BIOS_Signature)
	if (BIOS_Data[0x40] == '3')
	{
		if (BIOS_Data[0x41] == '2')
			ATIVGABoard = ATI_BOARD_EGA;
		else if (BIOS_Data[0x41] == '3')
			ATIVGABoard = ATI_BOARD_BASIC;
		else if (BIOS_Data[0x41] == '1')
		{
			/* This is a VGA Wonder or Mach8 board of some kind */
			if ((BIOS_Data[0x43] >= '1') &&
			    (BIOS_Data[0x43] <= '6'))
				ATIChip = BIOS_Data[0x43] - '0';

			switch (BIOS_Data[0x43])
			{
				case '1':	/* ATI_CHIP_18800	*/
					ATIVGABoard = ATI_BOARD_V3;
					Offset = 0xB0;
					break;

				case '2':	/* ATI_CHIP_18800_1	*/
					if (BIOS_Data[0x42] & 0x10)
						ATIVGABoard = ATI_BOARD_V5;
					else
						ATIVGABoard = ATI_BOARD_V4;
					Offset = 0xB0;
					break;

				case '3':	/* ATI_CHIP_28800_2	*/
				case '4':	/* ATI_CHIP_28800_4	*/
				case '5':	/* ATI_CHIP_28800_5	*/
				case '6':	/* ATI_CHIP_28800_6	*/
					ATIVGABoard = ATI_BOARD_PLUS;
					if (BIOS_Data[0x44] & 0x80)
					{
						ATIVGABoard = ATI_BOARD_XL;
						ATIDac = ATI_DAC_SC11483;
					}
					Offset = 0xA0;
					break;

				case 'a':	/* This Mach32 has suffered */
				case 'b':	/* a lobotomy, it seems...  */
				case 'c':
					ATIVGABoard = ATI_BOARD_NONISA;
					ATIMach32ChipID();
					break;

				case ' ':	/* A crippled Mach64? */
					ATIVGABoard = ATI_BOARD_NONISA;
					ATIMach64ChipID();
					break;

				default:
					break;
			}
		}

		if (ATIBoard == ATI_BOARD_NONE)
			ATIBoard = ATIVGABoard;
	}

	if ((ATIChip < ATI_CHIP_88800) &&
	    (Signature_Found == BIOS_Signature))
	{
		/*
		 * Pick up extended register port number.
		 */
		ATIExtReg = *((short *)(BIOS_Data + 0x10)) & 0x0FFF;
	}
	if (Reset_Before_Probe)
	{
		PutReg(GRAX, 0x50, ATIExtReg & 0xFF);
		PutReg(GRAX, 0x51, Offset | (ATIExtReg >> 8));
	}

	/*
	 * Sometimes, the BIOS lies about the chip.
	 */
	if ((ATIChip >= ATI_CHIP_28800_4) ||
		(ATIChip <= ATI_CHIP_28800_6))
	{
		IO_Value = GetReg(ATIExtReg, 0xAA) & 0x0F;
		if ((IO_Value < 7) && (IO_Value > ATIChip))
			ATIChip = IO_Value;
	}

	printf("%s graphics controller detected.\n", ChipNames[ATIChip]);
	if ((ATIChip >= ATI_CHIP_68800) && (ATIChip != ATI_CHIP_68800_3))
	{
		printf("Chip type %04X", ChipType);
		if (!(ChipType & ~(CHIP_CODE_0 | CHIP_CODE_1)))
			printf(" (%c%c)",
			       ((ChipType & CHIP_CODE_1) >> 5) + 0x41,
			       ((ChipType & CHIP_CODE_0)     ) + 0x41);
		printf(", class %d, revision %d.\n", ChipClass, ChipRevision);
	}
	if (ATIBoard == ATI_BOARD_NONE)
		printf("Unknown chip descriptor in BIOS:  '%.1s' (0x%02X)\n",
			&BIOS_Data[0x43], BIOS_Data[0x43]);
	printf("%s or similar RAMDAC detected.\n", DACNames[ATIDac]);
	printf("This is a %s video adapter.\n\n", BoardNames[ATIBoard]);

	if ((ATIChip < ATI_CHIP_88800) &&
	    (Signature_Found == BIOS_Signature))
	{
		printf("   Signature code:                \"%c%c\"\n",
		       BIOS_Data[0x40], BIOS_Data[0x41]);
		printf("\n   BIOS version:                  %d.%d\n",
		       BIOS_Data[0x4C], BIOS_Data[0x4D]);

		printf("\n   Byte at offset 0x42 =          0x%02X\n",
		       BIOS_Data[0x42]);
		printf("   8 and 16 bit ROM supported:    %s\n",
		       BIOS_Data[0x42] & 0x01 ? "Yes" : "No");
		printf("   Mouse chip present:            %s\n",
		       BIOS_Data[0x42] & 0x02 ? "Yes" : "No");
		printf("   Inport compatible mouse port:  %s\n",
		       BIOS_Data[0x42] & 0x04 ? "Yes" : "No");
		printf("   Micro Channel supported:       %s\n",
		       BIOS_Data[0x42] & 0x08 ? "Yes" : "No");
		printf("   Clock chip present:            %s\n",
		       BIOS_Data[0x42] & 0x10 ? "Yes" : "No");
		printf("   Uses C000:0000 to D000:FFFF:   %s\n",
		       BIOS_Data[0x42] & 0x80 ? "Yes" : "No");

		printf("\n   Byte at offset 0x44 =          0x%02X\n",
		       BIOS_Data[0x44]);
		printf("   Supports 70Hz non-interlaced:  %s\n",
		       BIOS_Data[0x44] & 0x01 ? "No" : "Yes");
		printf("   Supports Korean characters:    %s\n",
		       BIOS_Data[0x44] & 0x02 ? "Yes" : "No");
		printf("   Uses 45Mhz memory clock:       %s\n",
		       BIOS_Data[0x44] & 0x04 ? "Yes" : "No");
		printf("   Supports zero wait states:     %s\n",
		       BIOS_Data[0x44] & 0x08 ? "Yes" : "No");
		printf("   Uses paged ROMs:               %s\n",
		       BIOS_Data[0x44] & 0x10 ? "Yes" : "No");
		printf("   8514/A hardware on board:      %s\n",
		       BIOS_Data[0x44] & 0x40 ? "No" : "Yes");
		printf("   32K colour DAC on board:       %s\n\n",
		       BIOS_Data[0x44] & 0x80 ? "Yes" : "No");
	}

	/*
	 * Find out how much video memory the VGA Wonder side thinks it has.
	 */
	if (ATIChip < ATI_CHIP_28800_2)
	{
		IO_Value = GetReg(ATIExtReg, 0xBB);
		if (IO_Value & 0x20)
			VGAWondervideoRam = 512;
		else
			VGAWondervideoRam = 256;
	}
	else
	{
		IO_Value = GetReg(ATIExtReg, 0xB0);
		if (IO_Value & 0x08)
			VGAWondervideoRam = 1024;
		else if (IO_Value & 0x10)
			VGAWondervideoRam = 512;
		else
			VGAWondervideoRam = 256;
	}

	if (MachvideoRam)
		printf("Accelerator video RAM:  %dkB\n", MachvideoRam);
	printf("VGA video RAM:  %dkB\n", VGAWondervideoRam);

	printf("The extended registers (ATIExtReg) are at 0x%04X.\n",
		ATIExtReg);

	ptr = BIOS_Data;
	printf("\n BIOS data at 0xC0000:");
	for (Index = 0;  Index < BIOS_DATA_SIZE;  Index++, ptr++)
	{
		if ((Index % 4) == 0)
		{
			if ((Index % 16) == 0)
				printf("\n 0xC00%02X:", Index);
			printf(" ");
		}
		printf("%02X", *ptr);
	}

	misc = inb(R_GENMO);
	printf("\n\n Current miscellaneous output register value: 0x%02X.",
		misc);

	if (misc & 0x01)
	{
		PrintRegisters(CRTX(ColourIOBase), 0, 64,
			"colour CRT controller", 0);
		PrintRegisters(ATTRX, 0, 32, "attribute controller",
			GENS1(ColourIOBase));
	}
	else
	{
		PrintRegisters(CRTX(MonochromeIOBase), 0, 64,
			"monochrome CRT controller", 0);
		PrintRegisters(ATTRX, 0, 32, "attribute controller",
			GENS1(MonochromeIOBase));
	}

	PrintRegisters(GRAX, 0, 16, "graphics controller", 0);
	PrintRegisters(SEQX, 0, 8, "sequencer", 0);

	PrintRegisters(ATIExtReg, 2 << 6, (2 + 1) << 6, "extended", 0);

	printf("\n\n");

	IOPL_OFF;				/* Turn off I/O permissions */

	return 0;

}
