/* $XConsortium: ati_driver.c /main/9 1996/01/12 12:16:31 kaleb $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/ati/ati_driver.c,v 3.23 1996/01/12 14:37:46 dawes Exp $ */
/*
 * Copyright 1994 and 1995 by Marc Aurele La France (TSI @ UQV), tsi@ualberta.ca
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Marc Aurele La France not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Marc Aurele La France makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * MARC AURELE LA FRANCE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL MARC AURELE LA FRANCE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*************************************************************************/

/*
 * Author:  Marc Aurele La France (TSI @ UQV), tsi@ualberta.ca
 *
 * This is the ATI VGA Wonder driver for XFree86.  Contributions to the
 * previous versions of this driver by the following people are hereby
 * acknowledged:
 *
 * Thomas Roell, roell@informatik.tu-muenchen.de
 * Per Lindqvist, pgd@compuram.bbt.se
 * Doug Evans, dje@cygnus.com
 * Rik Faith, faith@cs.unc.edu
 * Arthur Tateishi, ruhtra@turing.toronto.edu
 * Alain Hebert, aal@broue.rot.qc.ca
 *
 * ... and, doubtless, many others whom I do not know about.
 *
 * Additional acknowledgements are due to:
 *
 * Ton van Rosmalen, ton@stack.urc.tue.nl, for debugging assistance on V3 and
 *    V5 boards.
 * David Chambers, davidc@netcom.com, for providing an old V3 board.
 * William Shubert, wms@ssd.intel.com, for work in supporting Mach64 boards.
 * ATI, Robert Wolff, David Dawes and Mark Weaver, for a Mach32 memory probe.
 * Hans Nasten, nasten@everyware.se, for debugging assistance on Mach8 boards.
 *
 * The driver is intended to support ATI VGA Wonder series of boards and its
 * OEM counterpart, the VGA1024 series.  It will also work with Mach32 and
 * Mach64 boards but will not use their accelerated features.
 *
 * The ATI x8800 chips use special registers for their extended features.
 * These registers are accessible through an index I/O port and a data I/O
 * port.  BIOS initialization stores the index port number in the Graphics
 * register bank (0x03CE), indices 0x50 and 0x51.  Unfortunately, these
 * registers are write-only (a.k.a. black holes).  On all but Mach64 boards,
 * the index port number can be found in the short integer at offset 0x10 in
 * the BIOS.  For Mach64's, this driver will use 0x01CE as the index port
 * number.  The data port number is one more than the index port number (i.e.
 * 0x01CF).  These ports differ slightly in their I/O behaviour from the normal
 * VGA ones:
 *
 *    write:  outw(0x01CE, (data << 8) | index);
 *    read:   outb(0x01CE, index);  data = inb(0x01CF);
 *
 * Two consecutive byte-writes will not work.  Furthermore an index written to
 * 0x01CE is usable only once.  Note also that the setting of ATI extended
 * registers (especially those with clock selection bits) should be bracketed
 * by a sequencer reset.
 *
 * The number of these extended VGA registers varies by chipset.  The 18800
 * series have 16, the 28800 series have 32, while Mach32's and Mach64's have
 * 64.  The last 16 on each have almost identical definitions.  Thus, the BIOS
 * (and this driver) sets up an indexing scheme whereby the last 16 extended
 * VGA registers are accessed at indices 0xB0 through 0xBF on all chipsets.
 *
 * Boards prior to V5 use 4 crystals.  Boards V5 and later use a clock
 * generator chip.  V3 and V4 boards differ when it comes to choosing clock
 * frequencies.
 *
 * VGA Wonder V3/V4 Board Clock Frequencies
 * R E G I S T E R S
 * 1CE(*)    3C2     3C2    Frequency
 * B2h/BEh
 * Bit 6/4  Bit 3   Bit 2   (MHz)
 * ------- ------- -------  -------
 *    0       0       0     50.175
 *    0       0       1     56.644
 *    0       1       0     Spare 1
 *    0       1       1     44.900
 *    1       0       0     44.900
 *    1       0       1     50.175
 *    1       1       0     Spare 2
 *    1       1       1     36.000
 *
 * (*):  V3 uses index B2h, bit 6;  V4 uses index BEh, bit 4
 *
 * V5, PLUS, XL and XL24 usually have an ATI 18810 clock generator chip, but
 * some have an ATI 18811-0, and it's quite conceivable that some exist with
 * ATI 18811-1's or ATI 18811-2's.  Mach32 boards are known to use any one of
 * these clock generators.  The possibilities for Mach64 boards also include
 * the newer programmable 18818 chips.  BIOS initialization can set the 18818
 * to one of two slightly different sets of frequencies.  Mach32 and Mach64
 * boards also use a different dot clock ordering.  ATI says there is no
 * reliable way for the driver to determine which clock generator is on the
 * board (the BIOS is tailored to each board), but the driver will do its best
 * to do so anyway.
 *
 * VGA Wonder V5/PLUS/XL/XL24 Board Clock Frequencies
 * R E G I S T E R S
 *   1CE     1CE     3C2     3C2    Frequency
 *   B9h     BEh                     (MHz)   18811-0  18811-1
 *  Bit 1   Bit 4   Bit 3   Bit 2    18810   18812-0  18811-2        18818
 * ------- ------- ------- -------  -------  -------  -------  ----------------
 *    0       0       0       0      30.240   30.240  135.000     (*3)     (*3)
 *    0       0       0       1      32.000   32.000   32.000  110.000  110.000
 *    0       0       1       0      37.500  110.000  110.000  126.000  126.000
 *    0       0       1       1      39.000   80.000   80.000  135.000  135.000
 *    0       1       0       0      42.954   42.954  100.000   50.350   25.175
 *    0       1       0       1      48.771   48.771  126.000   56.644   28.322
 *    0       1       1       0        (*1)   92.400   92.400   63.000   31.500
 *    0       1       1       1      36.000   36.000   36.000   72.000   36.000
 *    1       0       0       0      40.000   39.910   39.910     (*3)     (*3)
 *    1       0       0       1      56.644   44.900   44.900   80.000   80.000
 *    1       0       1       0      75.000   75.000   75.000   75.000   75.000
 *    1       0       1       1      65.000   65.000   65.000   65.000   65.000
 *    1       1       0       0      50.350   50.350   50.350   40.000   40.000
 *    1       1       0       1      56.640   56.640   56.640   44.900   44.900
 *    1       1       1       0        (*2)     (*3)     (*3)   49.500   49.500
 *    1       1       1       1      44.900   44.900   44.900   50.000   50.000
 *
 * (*1) External 0 (supposedly 16.657 Mhz)
 * (*2) External 1 (supposedly 28.322 MHz)
 * (*3) This setting doesn't seem to generate anything
 *
 * Mach32 and Mach64 Board Clock Frequencies
 * R E G I S T E R S
 *   1CE     1CE     3C2     3C2    Frequency
 *   B9h     BEh                     (MHz)   18811-0  18811-1
 *  Bit 1   Bit 4   Bit 3   Bit 2    18810   18812-0  18811-2        18818
 * ------- ------- ------- -------  -------  -------  -------  ----------------
 *    0       0       0       0      42.954   42.954  100.000   50.350   25.175
 *    0       0       0       1      48.771   48.771  126.000   56.644   28.322
 *    0       0       1       0        (*1)   92.400   92.400   63.000   31.500
 *    0       0       1       1      36.000   36.000   36.000   72.000   36.000
 *    0       1       0       0      30.240   30.240  135.000     (*3)     (*3)
 *    0       1       0       1      32.000   32.000   32.000  110.000  110.000
 *    0       1       1       0      37.500  110.000  110.000  126.000  126.000
 *    0       1       1       1      39.000   80.000   80.000  135.000  135.000
 *    1       0       0       0      50.350   50.350   50.350   40.000   40.000
 *    1       0       0       1      56.640   56.640   56.640   44.900   44.900
 *    1       0       1       0        (*2)     (*3)     (*3)   49.500   49.500
 *    1       0       1       1      44.900   44.900   44.900   50.000   50.000
 *    1       1       0       0      40.000   39.910   39.910     (*3)     (*3)
 *    1       1       0       1      56.644   44.900   44.900   80.000   80.000
 *    1       1       1       0      75.000   75.000   75.000   75.000   75.000
 *    1       1       1       1      65.000   65.000   65.000   65.000   65.000
 *
 * (*1) External 0 (supposedly 16.657 Mhz)
 * (*2) External 1 (supposedly 28.322 MHz)
 * (*3) This setting doesn't seem to generate anything
 *
 * Note that, to reduce confusion, this driver masks out the different clock
 * ordering.
 *
 * For all boards, these frequencies can be divided by 1 or 2.  For all boards,
 * except Mach32 and Mach64, frequencies can also be divided by 3 or 4.
 *
 *      Register 1CE, index B8h
 *       Bit 7    Bit 6
 *      -------  -------
 *         0        0           Divide by 1
 *         0        1           Divide by 2
 *         1        0           Divide by 3
 *         1        1           Divide by 4
 *
 * There is some question as to whether or not bit 1 of index 0xB9 can
 * be used for clock selection on a V4 board.  This driver makes it
 * available only if the "undoc_clocks" option (itself undocumented :-)) is
 * specified in XF86Config.
 */

/*************************************************************************/

/*
 * These are X and server generic header files.
 */
#include "X.h"
#include "input.h"
#include "screenint.h"

/*
 * These are XFree86-specific header files.
 */
#include "compiler.h"
#include "xf86Version.h"
#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#include "xf86_Config.h"
#include "vga.h"
#include "regati.h"

#ifdef XFreeXDGA
#include "X.h"
#include "Xproto.h"
#include "extnsionst.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif

#ifndef __inline__
#define __inline__ inline
#endif

#ifndef inline
#define inline     /* Nothing */
#endif

/*
 * Driver data structures.
 */
typedef struct
{
        vgaHWRec std;               /* good old IBM VGA */

        unsigned char             a3,         a6, a7,
                                  ab, ac, ad, ae,
                      b0, b1, b2, b3,     b5, b6,
                      b8, b9, ba,         bd, be, bf;
} vgaATIRec, *vgaATIPtr;

/*
 * Forward definitions for the functions that make up the driver.  See the
 * definitions of these functions for the real scoop.
 */
static Bool     ATIProbe();
static char *   ATIIdent();
static void     ATIEnterLeave();
static Bool     ATIInit();
static Bool     ATIValidMode();
static void *   ATISave();
static void     ATIRestore();
static void     ATIAdjust();
static void     ATISaveScreen();
static void     ATIGetMode();
/*
 * These are the bank select functions.  They are defined in bank.s.
 */
extern void     ATISetRead();
extern void     ATISetWrite();
extern void     ATISetReadWrite();
/*
 * Bank selection functions for V3 boards.  These are also defined in bank.s.
 */
extern void     ATIV3SetRead();
extern void     ATIV3SetWrite();
extern void     ATIV3SetReadWrite();
/*
 * Bank selection functions for V4 and V5 boards.  Defined in bank.s.
 */
extern void     ATIV4V5SetRead();
extern void     ATIV4V5SetWrite();
extern void     ATIV4V5SetReadWrite();
/*
 * This data structure defines the driver itself.  The data structure is
 * initialized with the functions that make up the driver and some data that
 * defines how the driver operates.
 */
vgaVideoChipRec ATI =
{
        ATIProbe,               /* Probe */
        ATIIdent,               /* Ident */
        ATIEnterLeave,          /* EnterLeave */
        ATIInit,                /* Init */
        ATIValidMode,           /* ValidMode */
        ATISave,                /* Save */
        ATIRestore,             /* Restore */
        ATIAdjust,              /* Adjust */
        ATISaveScreen,          /* SaveScreen */
        ATIGetMode,             /* GetMode */
        (void (*)())NoopDDA,    /* FbInit */
        ATISetRead,             /* SetRead */
        ATISetWrite,            /* SetWrite */
        ATISetReadWrite,        /* SetReadWrite */
        0x10000,                /* Mapped memory window size (64k) */
        0x10000,                /* Video memory bank size (64k) */
        16,                     /* Shift factor to get bank number */
        0xFFFF,                 /* Bit mask for address within a bank */
        0x00000, 0x10000,       /* Boundaries for reads within a bank */
        0x00000, 0x10000,       /* Boundaries for writes within a bank */
        TRUE,                   /* Uses two banks */
        VGA_DIVIDE_VERT,        /* Divide interlaced vertical timings */
        {0,},                   /* Options are set by ATIProbe */
        16,                     /* Virtual X rounding */
        FALSE,                  /* No linear frame buffer */
        0,                      /* Linear frame buffer base address */
        0,                      /* Linear frame buffer size */
        FALSE,                  /* No support for 16 bits per pixel (yet) */
        FALSE,                  /* No support for 32 bits per pixel (yet) */
        NULL,                   /* List of builtin modes */
        1,                      /* Clock scaling factor */
};

/*
 * This is a convenience macro, so that entries in the driver structure can
 * simply be dereferenced with 'new->xxx'.
 */
#define new ((vgaATIPtr)vgaNewVideoState)

/*
 * This structure is used by ATIProbe in an attempt to define a default video
 * mode when the user has not specified any modes in XF86Config.
 */
static DisplayModeRec DefaultMode;

/*
 * Define a table to map mode flag values to XF86Config tokens.
 */
typedef struct
{
      int flag, token;
} TokenTabRec, *TokenTabPtr;

static TokenTabRec TokenTab[] =
{
      {V_PHSYNC,    TT_PHSYNC},
      {V_NHSYNC,    TT_NHSYNC},
      {V_PVSYNC,    TT_PVSYNC},
      {V_NVSYNC,    TT_NVSYNC},
      {V_PCSYNC,    TT_PCSYNC},
      {V_NCSYNC,    TT_NCSYNC},
      {V_INTERLACE, TT_INTERLACE},
      {V_DBLSCAN,   TT_DBLSCAN},
      {V_CSYNC,     TT_CSYNC},
      {0,           0}
};

/*
 * This driver needs non-standard I/O ports.  The first two are determined by
 * ATIProbe and are initialized to their most probable values here.
 */
static unsigned ATI_IOPorts[] =
{
        /* ATI VGA Wonder extended registers */
        0x01CE, 0x01CF,

        /* 8514/A registers */
        GP_STAT, SUBSYS_CNTL,

        /* Mach8 registers */
        EXT_FIFO_STATUS, CLOCK_SEL,

        /* Mach32 registers */
        MEM_BNDRY, MEM_CFG, MISC_OPTIONS,

        /* Mach64 registers */
        BUS_CNTL, GEN_TEST_CNTL, CONFIG_CNTL, CRTC_GEN_CNTL, MEM_INFO
};
#define Num_ATI_IOPorts (sizeof(ATI_IOPorts) / sizeof(ATI_IOPorts[0]))
short ATIExtReg = 0x01CE;       /* Used by bank.s;  Must be short */
extern unsigned char ATIB2Reg;  /* The B2 mirror in bank.s */

/*
 * I/O port list needed by ATIProbe.
 */
static unsigned Probe_IOPorts[] =
{
        /* VGA Graphics registers */
        GRAX, GRAD,

        /* 8514/A registers */
        ERR_TERM, GP_STAT, SUBSYS_CNTL,
        WRT_MASK, RD_MASK, CUR_X, CUR_Y, PIX_TRANS, FRGD_COLOR,

        /* Mach8 registers */
        ROM_ADDR_1, DESTX_DIASTP, CONFIG_STATUS_1, EXT_FIFO_STATUS,
        CLOCK_SEL, GE_OFFSET_LO, GE_OFFSET_HI, GE_PITCH, DP_CONFIG,

        /* Mach32 registers */
        READ_SRC_X, CHIP_ID, MISC_OPTIONS,
        MEM_BNDRY, R_EXT_GE_CONFIG, EXT_GE_CONFIG,
        DEST_X_START, DEST_X_END, DEST_Y_END, ALU_FG_FN,

        /* Mach64 registers */
        SCRATCH_REG0, MEM_INFO, CONFIG_STATUS_0, CONFIG_CHIP_ID,
        BUS_CNTL, GEN_TEST_CNTL
};
#define Num_Probe_IOPorts (sizeof(Probe_IOPorts) / sizeof(Probe_IOPorts[0]))

/*
 * Handy macros to read and write registers.
 */
#define GetReg(Register, Index)                                 \
        (                                                       \
                outb(Register, Index),                          \
                inb(Register + 1)                               \
        )
#define PutReg(Register, Index, Register_Value)                 \
        outw(Register, ((Register_Value) << 8) | (Index))
#define ATIGetExtReg(Index)                                     \
        GetReg(ATIExtReg, Index)
#define ATIPutExtReg(Index, Register_Value)                     \
        PutReg(ATIExtReg, Index, Register_Value)

static unsigned char Chip_Has_SUBSYS_CNTL = FALSE;

#define ATI_CHIP_NONE      0
#define ATI_CHIP_18800     1
#define ATI_CHIP_18800_1   2
#define ATI_CHIP_28800_2   3
#define ATI_CHIP_28800_4   4
#define ATI_CHIP_28800_5   5
#define ATI_CHIP_28800_6   6
#define ATI_CHIP_68800     7    /* Mach32 */
#define ATI_CHIP_68800_3   8    /* Mach32 */
#define ATI_CHIP_68800_6   9    /* Mach32 */
#define ATI_CHIP_68800LX  10    /* Mach32 */
#define ATI_CHIP_68800AX  11    /* Mach32 */
#define ATI_CHIP_88800    12    /* Mach64 */
#define ATI_CHIP_88800CX  13    /* Mach64 */
#define ATI_CHIP_88800GX  14    /* Mach64 */
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
#define ATI_BOARD_V3      1
#define ATI_BOARD_V4      2
#define ATI_BOARD_V5      3
#define ATI_BOARD_PLUS    4
#define ATI_BOARD_XL      5
#define ATI_BOARD_NONISA  6
#define ATI_BOARD_MACH8   7
#define ATI_BOARD_MACH32  8
#define ATI_BOARD_MACH64  9
static unsigned char ATIBoard = ATI_BOARD_NONE;
static unsigned char ATIVGABoard = ATI_BOARD_NONE;
static const char *BoardNames[] =
{
        "Unknown",
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

#define ATI_CLOCK_NONE     0    /* Must be zero */
#define ATI_CLOCK_CRYSTALS 1    /* Must be one */
#define ATI_CLOCK_18810    2
#define ATI_CLOCK_18811_0  3
#define ATI_CLOCK_18811_1  4
#define ATI_CLOCK_18818_A  5
#define ATI_CLOCK_18818_B  6
static unsigned char ATIClock = ATI_CLOCK_NONE;
static const char *ClockNames[] =
{
        "unknown",
        "crystals",
        "ATI 18810",
        "ATI 18811-0",
        "ATI 18811-1",
        "ATI 18818 (primary BIOS setting)",
        "ATI 18818 (alternate BIOS setting)"
};

static unsigned short int ChipType = 0, ChipClass = 0, ChipRevision = 0;

/*
 * The driver will attempt to match the clocks to one of the following
 * specifications.
 */
static const int
ATICrystalFrequencies[] =
{
         50175,  56644,      0,  44900,  44900,  50175,      0,  36000,
            -1
},
ATI18810Frequencies[] =
{
         30240,  32000,  37500,  39000,  42954,  48771,      0,  36000,
         40000,  56644,  75000,  65000,  50350,  56640,      0,  44900
},
ATI188110Frequencies[] =
{
         30240,  32000, 110000,  80000,  42954,  48771,  92400,  36000,
         39910,  44900,  75000,  65000,  50350,  56640,      0,  44900
},
ATI188111Frequencies[] =
{
        135000,  32000, 110000,  80000, 100000, 126000,  92400,  36000,
         39910,  44900,  75000,  65000,  50350,  56640,      0,  44900
},
ATI18818AFrequencies[] =
{
             0, 110000, 126000, 135000,  50350,  56644,  63000,  72000,
             0,  80000,  75000,  65000,  40000,  44900,  49500,  50000
},
ATI18818BFrequencies[] =
{
             0, 110000, 126000, 135000,  25175,  28322,  31500,  36000,
             0,  80000,  75000,  65000,  40000,  44900,  49500,  50000
},
*ClockLine[] =
{
        NULL,
        ATICrystalFrequencies,
        ATI18810Frequencies,
        ATI188110Frequencies,
        ATI188111Frequencies,
        ATI18818AFrequencies,
        ATI18818BFrequencies,
        NULL
};

/*
 * The driver will reject XF86Config clocks lines that start with, or are an
 * initial subset of, one of the following.
 */
static const int
ATIVGAClocks[] =
{
         25175,  28322,
            -1
},
ATIPre_2_1_1_Clocks_A[] =       /* Based on 18810 */
{
         18000,  22450,  25175,  28320,  36000,  44900,  50350,  56640,
         30240,  32000,  37500,  39000,  40000,  56644,  75000,  65000,
            -1
},
ATIPre_2_1_1_Clocks_B[] =       /* Based on 18811-0 */
{
         18000,  22450,  25175,  28320,  36000,  44900,  50350,  56640,
         30240,  32000, 110000,  80000,  39910,  44900,  75000,  65000,
            -1
},
ATIPre_2_1_1_Clocks_C[] =       /* Based on 18811-1 (or -2) */
{
         18000,  22450,  25175,  28320,  36000,  44900,  50350,  56640,
        135000,  32000, 110000,  80000,  39910,  44900,  75000,  65000,
            -1
},
ATIPre_2_1_1_Clocks_D[] =       /* Based on 18818 (primary) */
{
         36000,  25000,  20000,  22450,  72000,  50000,  40000,  44900,
             0, 110000, 126000, 135000,      0,  80000,  75000,  65000,
            -1
},
ATIPre_2_1_1_Clocks_E[] =       /* Based on 18818 (alternate) */
{
         18000,  25000,  20000,  22450,  36000,  50000,  40000,  44900,
             0, 110000, 126000, 135000,      0,  80000,  75000,  65000,
            -1
},
*InvalidClockLine[] =
{
        NULL,
        ATIVGAClocks,
        ATIPre_2_1_1_Clocks_A,
        ATIPre_2_1_1_Clocks_B,
        ATIPre_2_1_1_Clocks_C,
        ATIPre_2_1_1_Clocks_D,
        ATIPre_2_1_1_Clocks_E,
        NULL
};

static const unsigned char Clock_Maps[][16] =
{
        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15},
        { 4,  5,  6,  7, 12, 13, 14, 15,  0,  1,  2,  3,  8,  9, 10, 11}
};
#define Number_Of_Clock_Maps (sizeof(Clock_Maps) / sizeof(Clock_Maps[0]))

static unsigned char saved_b9_bit_1 = 0;

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
                for (compare_length = 0;  ;  compare_length++)
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
 * ATIIdent --
 *
 * Returns a string name for this driver or NULL.
 */
static char *
ATIIdent(const unsigned int n)
{
        const char *chipsets[] = {"vgawonder"};

        if (n >= (sizeof(chipsets) / sizeof(char *)))
                return (NULL);
        else
                return ((char *)chipsets[n]);
}

#define ATIV3Delay                                              \
        {                                                       \
                int counter;                                    \
                for (counter = 0;  counter < 512;  counter++)   \
                        /* (void) inb(GENS1(vgaIOBase)) */;     \
        }

/*
 * ATIModifyExtReg --
 *
 * This routine is called to modify certain bits in an ATI extended VGA
 * register while preserving its other bits.  The routine will not write the
 * register if it turns out its value would not change.
 */
static void
ATIModifyExtReg(const unsigned char Index,
                int Current_Value,
                const unsigned char Current_Mask,
                unsigned char New_Value)
{
        /* Possibly retrieve the current value */
        if (Current_Value < 0)
                Current_Value = ATIGetExtReg(Index);

        /* Compute new value */
        New_Value &= (unsigned char)(~Current_Mask);
        New_Value |= Current_Value & Current_Mask;

        /* Check if value will be changed */
        if (Current_Value != New_Value)
        {
                /*
                 * The following is taken from ATI's VGA Wonder programmer's
                 * reference manual which says that this is needed to "ensure
                 * the proper state of the 8/16 bit ROM toggle".  I suspect a
                 * timing glitch appeared in the 18800 after its die was cast.
                 * 18800-1 and later chips do not exhibit this problem.
                 */
                if ((ATIChip == ATI_CHIP_18800) && (Index == 0xB2) &&
                   ((New_Value ^ 0x40) & Current_Value & 0x40))
                {
                        unsigned char misc = inb(R_GENMO);
                        unsigned char bb = ATIGetExtReg(0xBB);
                        outb(GENMO, (misc & 0xF3) | 0x04 | ((bb & 0x10) >> 1));
                        Current_Value &= (unsigned char)(~0x40);
                        ATIPutExtReg(0xB2, Current_Value);
                        ATIV3Delay;
                        outb(GENMO, misc);
                        ATIV3Delay;
                        if (Current_Value != New_Value)
                                ATIPutExtReg(0xB2, New_Value);
                }
                else
                        ATIPutExtReg(Index, New_Value);
        }
}

/*
 * ATIMapClock --
 *
 * This function is called to mask out the different clock ordering used on
 * Mach32 and Mach64 boards.
 */
static __inline__ int
ATIMapClock(int Clock)
{
        if (ATIChip >= ATI_CHIP_68800)
        {
                /* Invert the 0x04 bit */
                Clock ^= 0x04;
        }

        return Clock;
}

#define ATIUnmapClock(no)               ATIMapClock(no)

/*
 * ATIClockSelect --
 *
 * This function selects the dot-clock with index 'no'.  This is done by
 * setting bits in various registers (generic VGA uses two bits in the
 * Miscellaneous Output Register to select from 4 clocks).  Care is taken to
 * protect any other bits in these registers by fetching their values and
 * masking off the other bits.
 */
static Bool
ATIClockSelect(int no)
{
        static unsigned char saved_misc,
                saved_b2, saved_b5, saved_b8, saved_b9, saved_be;
        unsigned char misc;

        switch(no)
        {
                case CLK_REG_SAVE:
                        /*
                         * Here all of the registers that can be affected by
                         * clock setting are saved into static variables.
                         */
                        saved_misc = inb(R_GENMO);
                        saved_b5 = ATIGetExtReg(0xB5);
                        saved_b8 = ATIGetExtReg(0xB8);

                        if (ATIChip == ATI_CHIP_18800)
                                saved_b2 = ATIGetExtReg(0xB2);
                        else
                        {
                                saved_be = ATIGetExtReg(0xBE);
                                if ((ATIBoard != ATI_BOARD_V4) ||
                                    (OFLG_ISSET(OPTION_UNDOC_CLKS,
                                        &vga256InfoRec.options)))
                                        saved_b9 = ATIGetExtReg(0xB9);
                        }

                        /*
                         * Ensure clock interface is properly configured.
                         */
                        ATIModifyExtReg(0xB5, saved_b5, 0x7F, 0x00);

                        break;

                case CLK_REG_RESTORE:
                        /*
                         * Here all the previously saved registers are
                         * restored.
                         */
                        ATISaveScreen(SS_START);

                        if (ATIChip == ATI_CHIP_18800)
                                ATIModifyExtReg(0xB2, -1, 0x00, saved_b2);
                        else
                        {
                                ATIModifyExtReg(0xBE, -1, 0x00, saved_be);
                                if ((ATIBoard != ATI_BOARD_V4) ||
                                    (OFLG_ISSET(OPTION_UNDOC_CLKS,
                                        &vga256InfoRec.options)))
                                        ATIModifyExtReg(0xB9, -1, 0x00,
                                                saved_b9);
                        }
                        ATIModifyExtReg(0xB5, -1, 0x00, saved_b5);
                        ATIModifyExtReg(0xB8, -1, 0x00, saved_b8);

                        outb(GENMO, saved_misc);
                        ATISaveScreen(SS_FINISH);

                        break;

                default:
                        /*
                         * Possibly, remap clock number.
                         */
                        no = ATIMapClock(no);

                        /*
                         * On boards with crystals, switching to one of the
                         * spare assignments doesn't do anything (i.e. the
                         * previous setting remains in effect).  So, disable
                         * their selection.
                         */
                        if (((no & 0x03) == 0x02) &&
                           ((ATIChip == ATI_CHIP_18800) ||
                            (ATIBoard == ATI_BOARD_V4)))
                                return (FALSE);

                        /*
                         * Set the generic two low-order bits of the clock
                         * select.
                         */
                        misc = (inb(R_GENMO) & 0xF3) | ((no << 2) & 0x0C);

                        /*
                         * Set the high order bits.
                         */
                        if (ATIChip == ATI_CHIP_18800)
                                ATIModifyExtReg(0xB2, -1, 0xBF, (no << 4));
                        else
                        {
                                ATIModifyExtReg(0xBE, -1, 0xEF, (no << 2));
                                if ((ATIBoard != ATI_BOARD_V4) ||
                                    (OFLG_ISSET(OPTION_UNDOC_CLKS,
                                        &vga256InfoRec.options)))
                                {
                                        no >>= 1;
                                        ATIModifyExtReg(0xB9, -1, 0xFD,
                                                (no >> 1) ^ saved_b9_bit_1);
                                }
                        }

                        /*
                         * Set clock divider bits.
                         */
                        ATIModifyExtReg(0xB8, -1, 0x00, (no << 3) & 0xC0);

                        /*
                         * Must set miscellaneous output register last.
                         */
                        outb(GENMO, misc);

                        break;
        }
        return (TRUE);
}

/*
 * ATIMatchClockLine --
 *
 * This function tries to match the XF86Config clocks to one of an array of
 * clock lines.  It returns a clock line number (or 0).
 */
static int
ATIMatchClockLine(const int **Clock_Line, const unsigned int Number_Of_Clocks,
                  const int Calibration_Clock_Number, const int Clock_Map)
{
        int Clock_Chip = 0, Clock_Chip_Index = 0;
        int Number_Of_Matching_Clocks = 0;
        int Minimum_Gap = CLOCK_TOLERANCE + 1;

        /* If checking for XF86Config clock order, skip crystals */
        if (Clock_Map)
                Clock_Chip_Index++;

        for (;  Clock_Line[++Clock_Chip_Index];  )
        {
                int Maximum_Gap = 0, Clock_Count = 0, Clock_Index = 0;

                for (;  Clock_Index < Number_Of_Clocks;  Clock_Index++)
                {
                        int Gap, XF86Config_Clock, Specification_Clock;

                        Specification_Clock =
                                Clock_Line[Clock_Chip_Index]
                                          [Clock_Maps[Clock_Map][Clock_Index]];
                        if (Specification_Clock < 0)
                                break;
                        if (!Specification_Clock ||
                           (Specification_Clock > vga256InfoRec.maxClock))
                                continue;

                        XF86Config_Clock = vga256InfoRec.clock[Clock_Index];
                        if (!XF86Config_Clock ||
                           (XF86Config_Clock > vga256InfoRec.maxClock))
                                continue;

                        Gap = abs(XF86Config_Clock - Specification_Clock);
                        if (Gap >= Minimum_Gap)
                                goto skip_this_clock_generator;
                        if (!Gap)
                        {
                                if (Clock_Index == Calibration_Clock_Number)
                                        continue;
                        }
                        else if (Gap > Maximum_Gap)
                                Maximum_Gap = Gap;
                        Clock_Count++;
                }

                if (Clock_Count <= Number_Of_Matching_Clocks)
                        continue;
                Number_Of_Matching_Clocks = Clock_Count;
                Clock_Chip = Clock_Chip_Index;
                if (!(Minimum_Gap = Maximum_Gap))
                        break;
        skip_this_clock_generator: ;
        }

        return Clock_Chip;
}

/*
 * ATIGetClocks --
 *
 * This function is called by ATIProbe and handles the XF86Config clocks line
 * (or lack thereof).
 */
static void
ATIGetClocks()
{
        int Number_Of_Documented_Clocks, Number_Of_Undivided_Clocks;
        int Number_Of_Dividers, Number_Of_Clocks;
        int Calibration_Clock_Number, Calibration_Clock_Value;
        int Clock_Index, Specification_Clock, Clock_Map;

        /*
         * Determine the number of clock values the board should be able to
         * generate and the dot clock to use for probe calibration.
         */
probe_clocks:
        Number_Of_Dividers = 4;
        if ((ATIChip == ATI_CHIP_18800) || (ATIBoard == ATI_BOARD_V4))
        {
                Number_Of_Documented_Clocks = 8;
                /* Actually, any undivided clock will do */
                Calibration_Clock_Number = 7;
                Calibration_Clock_Value = 36000;
        }
        else
        {
                Number_Of_Documented_Clocks = 16;
                Calibration_Clock_Number = 10 /* or 11 */;
                Calibration_Clock_Value = 75000 /* or 65000 */;
                if (ATIChip >= ATI_CHIP_68800)
                        Number_Of_Dividers = 2;
        }
        Number_Of_Undivided_Clocks = Number_Of_Documented_Clocks;
        if ((OFLG_ISSET(OPTION_UNDOC_CLKS, &vga256InfoRec.options)) &&
            (ATIBoard == ATI_BOARD_V4))
                Number_Of_Undivided_Clocks <<= 1;
        Number_Of_Clocks = Number_Of_Undivided_Clocks * Number_Of_Dividers;

        /*
         * Respect any XF86Config clocks line.  Well, that's the theory,
         * anyway.  In practice, however, the regular use of probed values is
         * widespread, at times causing otherwise inexplicable results.  So,
         * attempt to normalize the clocks to known (specification) values.
         */
        if ((!vga256InfoRec.clocks) || xf86ProbeOnly ||
            (OFLG_ISSET(OPTION_PROBE_CLKS, &vga256InfoRec.options)))
        {
                /*
                 * Probe the board for clock values.  Note that vgaGetClocks
                 * cannot be used for this purpose because it assumes clock
                 * 1 is 28.322 MHz.  Instead call xf86GetClocks directly
                 * passing it slighly different parameters.
                 */
                xf86GetClocks(Number_Of_Clocks, ATIClockSelect,
                        vgaProtect, (void (*)())vgaSaveScreen,
                        GENS1(vgaIOBase), 0x08,
                        Calibration_Clock_Number, Calibration_Clock_Value,
                        &vga256InfoRec);

                /* Tell user clocks were probed, instead of supplied */
                OFLG_CLR(XCONFIG_CLOCKS, &vga256InfoRec.xconfigFlag);

                /*
                 * Attempt to match probed clocks to a known specification.
                 */
                ATIClock =
                        ATIMatchClockLine(ClockLine,
                                Number_Of_Documented_Clocks,
                                Calibration_Clock_Number, 0);

                if ((ATIChip == ATI_CHIP_18800) || (ATIBoard == ATI_BOARD_V4))
                {
                        /*
                         * V3 and V4 boards don't have clock chips.
                         */
                        if (ATIClock > ATI_CLOCK_CRYSTALS)
                                ATIClock = ATI_CLOCK_NONE;
                }
                else
                {
                        /*
                         * All others don't have crystals.
                         */
                        if (ATIClock == ATI_CLOCK_CRYSTALS)
                                ATIClock = ATI_CLOCK_NONE;
                }
        }
        else
        {
                /*
                 * Allow for an initial subset of specification clocks.  Can't
                 * allow for any more than that though...
                 */
                if (Number_Of_Clocks > vga256InfoRec.clocks)
                {
                        Number_Of_Clocks = vga256InfoRec.clocks;
                        if (Number_Of_Undivided_Clocks > Number_Of_Clocks)
                        {
                                Number_Of_Undivided_Clocks = Number_Of_Clocks;
                                if (Number_Of_Documented_Clocks >
                                    Number_Of_Clocks)
                                        Number_Of_Documented_Clocks =
                                                Number_Of_Clocks;
                        }
                }
                else if (Number_Of_Clocks < vga256InfoRec.clocks)
                        vga256InfoRec.clocks = Number_Of_Clocks;

                /*
                 * Attempt to match clocks to a known specification.
                 */
                ATIClock =
                        ATIMatchClockLine(ClockLine,
                                Number_Of_Documented_Clocks, -1, 0);

                if (ATIClock == ATI_CLOCK_NONE)
                {
                        /*
                         * Reject certain clock lines that are obviously wrong.
                         * This includes the standard VGA clocks, and clock
                         * lines that could have been used with the pre-2.1.1
                         * driver.
                         */
                        if (ATIMatchClockLine(InvalidClockLine,
                                              Number_Of_Clocks, -1, 0))
                                vga256InfoRec.clocks = 0;
                        else if ((ATIChip != ATI_CHIP_18800) &&
                                 (ATIBoard != ATI_BOARD_V4))
                        /*
                         * Check for clocks that are specified in the wrong
                         * order.  This is meant to catch those who are trying
                         * to use the clock order intended for the accelerated
                         * servers.
                         */
                        for (Clock_Map = 1;
                             Clock_Map < Number_Of_Clock_Maps;
                             Clock_Map++)
                                if (ATIMatchClockLine(ClockLine,
                                        Number_Of_Documented_Clocks,
                                        -1, Clock_Map))
                                {
                                        ErrorF("XF86Config clocks ordering"
                                               " incorrect.  Clocks will be"
                                               " probed.\nSee README.ati for"
                                               " more information.\n");
                                        vga256InfoRec.clocks = 0;
                                        goto probe_clocks;
                                 }
                }
                else
                /*
                 * Ensure crystals are not matched to clock chips, and vice
                 * versa.
                 */
                if ((ATIChip == ATI_CHIP_18800) || (ATIBoard == ATI_BOARD_V4))
                {
                        if (ATIClock > ATI_CLOCK_CRYSTALS)
                                vga256InfoRec.clocks = 0;
                }
                else
                {
                        if (ATIClock == ATI_CLOCK_CRYSTALS)
                                vga256InfoRec.clocks = 0;
                }

                if (!vga256InfoRec.clocks)
                {
                        ErrorF("Invalid or obsolete XF86Config clocks line"
                               " rejected.\nClocks will be probed.  See"
                               " README.ati for more information.\n");
                        goto probe_clocks;
                }
        }

        switch (ATIClock)
        {
                case ATI_CLOCK_NONE:
                        ErrorF("Unknown clock generator detected.\n");
                        return; /* Don't touch the clocks */

                case ATI_CLOCK_CRYSTALS:
                        ErrorF("This board uses crystals to generate dot clock"
                               " frequencies.\n");
                        break;

                default:
                        ErrorF("%s or similar clock chip detected.\n",
                                ClockNames[ATIClock]);
                        break;
        }

        /*
         * Replace the undivided documented clocks with specification values.
         */
        for (Clock_Index = 0;
             Clock_Index < Number_Of_Documented_Clocks;
             Clock_Index++)
        {
                /*
                 * Don't replace clocks that are probed, documented, or set by
                 * the user to zero.  The exception is that we need to override
                 * the user's value for the spare settings on a crystal-based
                 * board.
                 */
                Specification_Clock = ClockLine[ATIClock][Clock_Index];
                if (Specification_Clock < 0)
                        break;
                if (!vga256InfoRec.clock[Clock_Index])
                        continue;
                if ((!Specification_Clock) && (ATIClock != ATI_CLOCK_CRYSTALS))
                        continue;
                vga256InfoRec.clock[Clock_Index] = Specification_Clock;
        }

        /*
         * Adjust the divided clocks.
         */
        for (Clock_Index = Number_Of_Undivided_Clocks;
             Clock_Index < Number_Of_Clocks;
             Clock_Index++)
        {
                vga256InfoRec.clock[Clock_Index] =
                        vga256InfoRec.clock[Clock_Index %
                                Number_Of_Undivided_Clocks] /
                        ((Clock_Index / Number_Of_Undivided_Clocks) + 1);
        }
}

/*
 * Set variables whose value is based on an 68800's CHIP_ID register.
 */
static void
ATIMach32ChipID(void)
{
        int IO_Value = inw(CHIP_ID);
        ChipType = IO_Value & (CHIP_CODE_0 | CHIP_CODE_1);
        ChipClass    = (IO_Value & CHIP_CLASS) >> 10;
        ChipRevision = (IO_Value & CHIP_REV  ) >> 12;
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
 * Set variables whose value is based on an 88800's CONFIG_CHIP_ID register.
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

typedef unsigned short Colour;  /* The correct spelling should be OK :-) */

/*
 * Bit patterns which are extremely unlikely to show up when reading from
 * nonexistant memory (which normally shows up as either all bits set or all
 * bits clear).
 */
static const Colour Test_Pixel[] = {0x5AA5, 0x55AA, 0xA55A, 0xCA53};

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
        unsigned short corrected_MISC_OPTIONS;
        Bool AllPixelsOK;

        /* Save register values to be modified */
        saved_CLOCK_SEL = inw(CLOCK_SEL);
        saved_MEM_BNDRY = inw(MEM_BNDRY);
        saved_MISC_OPTIONS = inw(MISC_OPTIONS);
        corrected_MISC_OPTIONS = saved_MISC_OPTIONS & ~MEM_SIZE_ALIAS;
        saved_EXT_GE_CONFIG = inw(R_EXT_GE_CONFIG);

        /* Wait for enough FIFO entries */
        ATIWaitQueue(7);

        /* Enable accelerator */
        outw(CLOCK_SEL, saved_CLOCK_SEL | DISABPASSTHRU);

        /* Make accelerator and VGA share video memory */
        outw(MEM_BNDRY, saved_MEM_BNDRY & ~(MEM_PAGE_BNDRY | MEM_BNDRY_ENA));

        /* Prevent video memory wrap */
        outw(MISC_OPTIONS, corrected_MISC_OPTIONS | MEM_SIZE_4M);

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
#               define TestPixel               \
                        Test_Case[Case_Number].Coordinates[Pixel_Number]
#               define ForEachTestPixel        \
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
#               undef ForEachTestPixel
#               undef TestPixel
        }

        /* Restore what was changed and correct MISC_OPTIONS register */
        ATIWaitQueue(4);
        outw(EXT_GE_CONFIG, saved_EXT_GE_CONFIG);
        corrected_MISC_OPTIONS |=
                Test_Case[Case_Number].Miscellaneous_Options_Setting;
        if (corrected_MISC_OPTIONS != saved_MISC_OPTIONS)
                outw(MISC_OPTIONS, corrected_MISC_OPTIONS);
        outw(MEM_BNDRY, saved_MEM_BNDRY);

        /* Re-enable VGA passthrough */
        outw(CLOCK_SEL, saved_CLOCK_SEL & ~DISABPASSTHRU);

        /* Wait for activity to die down */
        ProbeWaitIdleEmpty();

        /* Tell ATIProbe the REAL story */
        return Test_Case[Case_Number].videoRamSize;
}

/*
 * ATIProbe --
 *
 * This is the function that makes a yes/no decision about whether or not a
 * chipset supported by this driver is present or not.  The server will call
 * each driver's probe function in sequence, until one returns TRUE or they all
 * fail.
 */
static Bool
ATIProbe()
{
#       define Signature        " 761295520"
#       define Signature_Size   10
#       define BIOS_DATA_SIZE   (0x80 + Signature_Size)
        unsigned char BIOS_Data[BIOS_DATA_SIZE];
#       define Signature_Offset 0x30
#       define BIOS_Signature   (BIOS_Data + Signature_Offset)
        unsigned char *Signature_Found;
        unsigned int IO_Value, IO_Value2, Index;
        int MachvideoRam = 0;
        int VGAWondervideoRam = 0;
        unsigned char Offset = 0x80;
        const int videoRamSizes[] =
                {0, 256, 512, 1024, 2*1024, 4*1024, 6*1024, 8*1024, 12*1024,
                 8*1024, 0};
        TokenTabPtr TokenEntry;
        int mode_flags;

        /*
         * Get out if this isn't the driver the user wants.
         */
        if (vga256InfoRec.chipset &&
            StrCaseCmp(vga256InfoRec.chipset, ATIIdent(0)))
        {
                static char *chipsets[] = {"ati", "mach8", "mach32", "mach64"};
                int i;

                /* Check for some other chipset names that need changing */
                for (i = 0;  StrCaseCmp(vga256InfoRec.chipset, chipsets[i]);  )
                        if (++i >= (sizeof(chipsets) / sizeof(chipsets[0])))
                                return (FALSE);
                ErrorF("ChipSet specification changed from \"%s%s",
                       chipsets[i], "\" to \"vgawonder\".\n");
                ErrorF("See README.ati for more information.\n");
                OFLG_CLR(XCONFIG_CHIPSET, &vga256InfoRec.xconfigFlag);
                if (vga256InfoRec.clocks)
                {
                        vga256InfoRec.clocks = 0;
                        if (!OFLG_ISSET(OPTION_PROBE_CLKS,
                                        &vga256InfoRec.options))
                                ErrorF("Clocks will be probed.\n");
                }
        }

        /*
         * Get BIOS data this driver will use.
         */
        if (xf86ReadBIOS(vga256InfoRec.BIOSbase, 0, BIOS_Data,
                         sizeof(BIOS_Data)) != sizeof(BIOS_Data))
                return (FALSE);

        /*
         * Get out if this is the wrong driver for installed chipset.
         */
        Signature_Found =
                findsubstring(Signature, Signature_Size,
                        BIOS_Data, sizeof(BIOS_Data));
        if (!Signature_Found)
                return (FALSE);

        /*
         * Enable the I/O ports needed for probing.
         */
        xf86ClearIOPortList(vga256InfoRec.scrnIndex);
        xf86AddIOPorts(vga256InfoRec.scrnIndex,
                Num_Probe_IOPorts, Probe_IOPorts);
        xf86EnableIOPorts(vga256InfoRec.scrnIndex);

        /*
         * Save register value to be modified, just in case there is no 8514/A
         * compatible accelerator.
         */
        IO_Value = inw(SUBSYS_STAT);

        /*
         * Determine if an 8514-compatible accelerator is present, making sure
         * it's not in some weird state.
         */
        outw(SUBSYS_CNTL, GPCTRL_RESET | CHPTEST_NORMAL);
        outw(SUBSYS_CNTL, GPCTRL_ENAB | CHPTEST_NORMAL |
                RVBLNKFLG | RPICKFLAG | RINVALIDIO | RGPIDLE);

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

                /*
                 * Don't leave any Mach8 or Mach32 in 8514/A mode.
                 */
                IO_Value2 = inw(CLOCK_SEL);
                outw(CLOCK_SEL, IO_Value2);
                ProbeWaitIdleEmpty();

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
                /* ATI accelerator detected */
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

        if (ATIBoard != ATI_BOARD_NONE)
                Chip_Has_SUBSYS_CNTL = TRUE;
        else
        {
                /*
                 * Restore register clobbered by 8514 reset attempt.
                 */
                outw(SUBSYS_CNTL, IO_Value);

                /*
                 * Determine if a Mach64 is present, making sure it's not in
                 * some weird state.
                 */
                IO_Value = inl(BUS_CNTL);
                outl(BUS_CNTL, (IO_Value &
                        ~(BUS_ROM_DIS | BUS_FIFO_ERR_INT_EN |
                          BUS_HOST_ERR_INT_EN)) |
                        BUS_FIFO_ERR_INT | BUS_HOST_ERR_INT);
                IO_Value = inl(GEN_TEST_CNTL) &
                        (GEN_OVR_OUTPUT_EN | GEN_OVR_POLARITY | GEN_CUR_EN |
                         GEN_BLOCK_WR_EN);
                outl(GEN_TEST_CNTL, IO_Value);
                outl(GEN_TEST_CNTL, IO_Value | GEN_GUI_EN);

                IO_Value = inl(SCRATCH_REG0);
                outl(SCRATCH_REG0, 0x55555555);          /* Test odd bits */
                if (inl(SCRATCH_REG0) == 0x55555555)
                {
                        outl(SCRATCH_REG0, 0xAAAAAAAA);  /* Test even bits */
                        if (inl(SCRATCH_REG0) == 0xAAAAAAAA)
                        {
                                /* A Mach64 has been detected */
                                IO_Value2 = inl(CONFIG_STATUS_0);
                                if ((IO_Value2 & (CFG_VGA_EN | CFG_CHIP_EN)) !=
                                    (CFG_VGA_EN | CFG_CHIP_EN))
                                {
                                        ErrorF("Mach64 detected but VGA"
                                               " Wonder capability cannot be"
                                               " enabled.\n");
                                        outl(SCRATCH_REG0, IO_Value);
                                        xf86DisableIOPorts(
                                                vga256InfoRec.scrnIndex);
                                        return (FALSE);
                                }
                                ATIBoard = ATI_BOARD_MACH64;
                                ATIDac = (IO_Value2 & CFG_INIT_DAC_TYPE) >> 9;
                                MachvideoRam =
                                        videoRamSizes[(inl(MEM_INFO) &
                                                CTL_MEM_SIZE) + 2];

                                ATIMach64ChipID();
                        }
                }
                outl(SCRATCH_REG0, IO_Value);
        }

        if (ATIBoard == ATI_BOARD_MACH32)
        {
                IO_Value = inw(CONFIG_STATUS_1);
                if (IO_Value & (_8514_ONLY | CHIP_DIS))
                {
                        ErrorF("Mach32 detected but VGA Wonder capability"
                               " cannot be enabled.\n");
                        xf86DisableIOPorts(vga256InfoRec.scrnIndex);
                        return (FALSE);
                }

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

        else if ((ATIBoard <= ATI_BOARD_MACH8) &&
                 (Signature_Found == BIOS_Signature))
        {
                /* This is a Mach8 or VGA Wonder board of some kind */
                if ((BIOS_Data[0x43] >= '1') && (BIOS_Data[0x43] <= '6'))
                        ATIChip = BIOS_Data[0x43] - '0';

                switch (BIOS_Data[0x43])
                {
                        case '1':       /* ATI_CHIP_18800 */
                                Offset = 0xB0;
                                ATIVGABoard = ATI_BOARD_V3;
                                /* Reset a few things for V3 boards */
                                ATI.ChipSetRead = ATIV3SetRead;
                                ATI.ChipSetWrite = ATIV3SetWrite;
                                ATI.ChipSetReadWrite = ATIV3SetReadWrite;
                                ATI.ChipUse2Banks = FALSE;
                                break;

                        case '2':       /* ATI_CHIP_18800_1 */
                                Offset = 0xB0;
                                if (BIOS_Data[0x42] & 0x10)
                                        ATIVGABoard = ATI_BOARD_V5;
                                else
                                        ATIVGABoard = ATI_BOARD_V4;
                                /* Reset a few things for V4 and V5 boards */
                                ATI.ChipSetRead = ATIV4V5SetRead;
                                ATI.ChipSetWrite = ATIV4V5SetWrite;
                                ATI.ChipSetReadWrite = ATIV4V5SetReadWrite;
                                break;

                        case '3':       /* ATI_CHIP_28800_2 */
                        case '4':       /* ATI_CHIP_28800_4 */
                        case '5':       /* ATI_CHIP_28800_5 */
                        case '6':       /* ATI_CHIP_28800_6 */
                                Offset = 0xA0;
                                ATIVGABoard = ATI_BOARD_PLUS;
                                if (BIOS_Data[0x44] & 0x80)
                                {
                                        ATIVGABoard = ATI_BOARD_XL;
                                        ATIDac = ATI_DAC_SC11483;
                                }
                                break;

                        case 'a':       /* It seems this Mach32 has suffered */
                        case 'b':       /* a frontal lobotomy...             */
                        case 'c':
                                ATIVGABoard = ATI_BOARD_NONISA;
                                ATIMach32ChipID();
                                ProbeWaitIdleEmpty();
                                if (inw(SUBSYS_STAT) != 0xFFFF)
                                        Chip_Has_SUBSYS_CNTL = TRUE;
                                break;

                        case ' ':       /* A crippled Mach64? */
                                ATIVGABoard = ATI_BOARD_NONISA;
                                ATIMach64ChipID();
                                break;

                        default:
                                break;
                }

                if (ATIBoard == ATI_BOARD_NONE)
                        ATIBoard = ATIVGABoard;
        }

        /*
         * Set up extended register addressing.
         */
        if ((ATIChip < ATI_CHIP_88800) &&
            (Signature_Found == BIOS_Signature))
        {
                /*
                 * Pick up extended register index I/O port number.
                 */
                ATIExtReg = *((short *)(BIOS_Data + 0x10)) & 0x0FFF;
        }
        PutReg(GRAX, 0x50, ATIExtReg & 0xFF);
        PutReg(GRAX, 0x51, Offset | (ATIExtReg >> 8));
        ATI_IOPorts[0] = ATIExtReg;
        ATI_IOPorts[1] = ATIExtReg + 1;

        /*
         * Probe I/O ports are no longer needed.
         */
        xf86DisableIOPorts(vga256InfoRec.scrnIndex);

        /*
         * Set up I/O ports to be used by this driver.
         */
        xf86ClearIOPortList(vga256InfoRec.scrnIndex);
        xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_VGA_IOPorts, VGA_IOPorts);
        xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_ATI_IOPorts, ATI_IOPorts);

        ATIEnterLeave(ENTER);           /* Unlock registers */

        /*
         * Sometimes, the BIOS lies about the chip.
         */
        if ((ATIChip >= ATI_CHIP_28800_4) &&
                (ATIChip <= ATI_CHIP_28800_6))
        {
                IO_Value = ATIGetExtReg(0xAA) & 0x0F;
                if ((IO_Value < 7) && (IO_Value > ATIChip))
                        ATIChip = IO_Value;
        }

        ErrorF("%s graphics controller detected.\n", ChipNames[ATIChip]);
        if ((ATIChip >= ATI_CHIP_68800) && (ATIChip != ATI_CHIP_68800_3))
        {
                ErrorF("Chip type %04X", ChipType);
                if (!(ChipType & ~(CHIP_CODE_0 | CHIP_CODE_1)))
                        ErrorF(" (%c%c)",
                                ((ChipType & CHIP_CODE_1) >> 5) + 0x41,
                                ((ChipType & CHIP_CODE_0)     ) + 0x41);
                ErrorF(", class %d, revision %d.\n", ChipClass, ChipRevision);
        }
        if (ATIBoard == ATI_BOARD_NONE)
                ErrorF("Unknown chip descriptor in BIOS:  '%.1s' (0x%02X).\n",
                        &BIOS_Data[0x43], BIOS_Data[0x43]);
        ErrorF("%s or similar RAMDAC detected.\n", DACNames[ATIDac]);
        ErrorF("This is a %s video adapter.\n", BoardNames[ATIBoard]);

        /* From now on, ignore Mach8 accelerator */
        if (ATIBoard == ATI_BOARD_MACH8)
                ATIBoard = ATIVGABoard;

        if ((OFLG_ISSET(OPTION_UNDOC_CLKS, &vga256InfoRec.options)) &&
            (ATIBoard == ATI_BOARD_V4))
        {
                /*
                 * Remember initial setting of undocumented clock selection
                 * bit.
                 */
                saved_b9_bit_1 = ATIGetExtReg(0xB9) & 0x02;
        }

        /*
         * Normalize any XF86Config videoRam value.
         */
        for (Index = 0;  videoRamSizes[++Index];  )
                if (vga256InfoRec.videoRam < videoRamSizes[Index])
                        break;
        vga256InfoRec.videoRam = videoRamSizes[Index - 1];

        /*
         * The default videoRam value is what the accelerator (if any) thinks
         * it has.  Also, allow the user to override the accelerator's value.
         */
        if (vga256InfoRec.videoRam == 0)
        {
                /* Normalization might have zeroed XF86Config videoRam value */
                OFLG_CLR(XCONFIG_VIDEORAM, &vga256InfoRec.xconfigFlag);
                vga256InfoRec.videoRam = MachvideoRam;
        }
        else
                MachvideoRam = vga256InfoRec.videoRam;

        /*
         * Find out how much video memory the VGA Wonder side thinks it has.
         */
        if (ATIChip < ATI_CHIP_28800_2)
        {
                IO_Value = ATIGetExtReg(0xBB);
                if (IO_Value & 0x20)
                        VGAWondervideoRam = 512;
                else
                        VGAWondervideoRam = 256;
                if (MachvideoRam > 512)
                        MachvideoRam = 512;
        }
        else
        {
                IO_Value = ATIGetExtReg(0xB0);
                if (IO_Value & 0x08)
                        VGAWondervideoRam = 1024;
                else if (IO_Value & 0x10)
                        VGAWondervideoRam = 512;
                else
                        VGAWondervideoRam = 256;
                if (MachvideoRam > 1024)
                        MachvideoRam = 1024;
        }

        /*
         * If there's no accelerator, default videoRam to what the VGA Wonder
         * side believes.
         */
        if (!vga256InfoRec.videoRam)
                vga256InfoRec.videoRam = VGAWondervideoRam;
        else
        {
                /*
                 * After BIOS initialization, the accelerator (if any) and the
                 * VGA won't necessarily agree on the amount of video memory,
                 * depending on whether or where the memory boundary is
                 * configured.  Any discrepancy will be resolved by ATIInit.
                 *
                 * However, it's possible that there is more video memory than
                 * VGA Wonder can architecturally handle.  If so, spit out a
                 * warning.
                 */
                if (MachvideoRam < vga256InfoRec.videoRam)
                        ErrorF("Virtual resolutions requiring more than %d"
                               " kB\n of video memory might not function"
                               " correctly.\n", MachvideoRam);
        }

        /*
         * Set the maximum allowable dot-clock frequency (in kHz).
         */
        vga256InfoRec.maxClock = 80000;

        /*
         * Determine available dot clock frequencies.
         */
        ATIGetClocks();

        /*
         * If user did not specify any modes, attempt to create a default mode.
         * Its timings will be taken from the mode in effect on driver entry.
         */
        if (vga256InfoRec.modes == NULL)
        {
                /*
                 * This duplicates vgaProbe's needmem variable.
                 */
#               if defined(MONOVGA) && !defined(BANKEDMONOVGA)
#                       define needmem (ATI.ChipMapSize << 3)
#               elif defined(MONOVGA) || defined(XF86VGA16)
#                       define needmem (vga256InfoRec.videoRam << 11)
#               else
#                       define needmem ((vga256InfoRec.videoRam << 13) / \
                                vgaBitsPerPixel)
#               endif

                /*
                 * Get current timings.
                 */
                ATIGetMode(&DefaultMode);

                /*
                 * Check if generated mode can be used.
                 */
                if ((DefaultMode.SynthClock / 1000) >
                   ((vga256InfoRec.maxClock / 1000) / ATI.ChipClockScaleFactor))
                        ErrorF("Default %dx%d mode not used:  required dot "
                               "clock greater than maxClock.\n",
                                DefaultMode.HDisplay, DefaultMode.VDisplay);
                else
                if ((DefaultMode.HDisplay * DefaultMode.VDisplay) > needmem)
                        ErrorF("Default %dx%d mode not used:  insufficient "
                               "video memory.\n",
                                DefaultMode.HDisplay, DefaultMode.VDisplay);
                else
                {
                        DefaultMode.prev = DefaultMode.next =
                                ATI.ChipBuiltinModes = &DefaultMode;
                        DefaultMode.name = "Default mode";
                        ErrorF("The following default video mode will be"
                               " used:\n"
                               " Dot clock:           %7.3fMHz\n"
                               " Horizontal timings:  %4d %4d %4d %4d\n"
                               " Vertical timings:    %4d %4d %4d %4d\n"
                               " Flags:              ",
                               DefaultMode.SynthClock / 1000.0,
                               DefaultMode.HDisplay,
                               DefaultMode.HSyncStart,
                               DefaultMode.HSyncEnd,
                               DefaultMode.HTotal,
                               DefaultMode.VDisplay,
                               DefaultMode.VSyncStart,
                               DefaultMode.VSyncEnd,
                               DefaultMode.VTotal);
                        mode_flags = DefaultMode.Flags;
                        for (TokenEntry = TokenTab;
                             TokenEntry->flag;
                             TokenEntry++)
                                if (mode_flags & TokenEntry->flag)
                                {
                                        ErrorF(" %s",
                                               xf86TokenToString(TimingTab,
                                                        TokenEntry->token));
                                        mode_flags &= ~TokenEntry->flag;
                                        if (!mode_flags)
                                                break;
                                }
                        ErrorF("\n");
                }

                /*
                 * Completeness.
                 */
#               undef needmem
        }

        /*
         * Set chipset name.
         */
        vga256InfoRec.chipset = ATIIdent(0);

        /*
         * Tell monochrome and 16-colour servers banked operation is
         * supported.
         */
        vga256InfoRec.bankedMono = TRUE;

        /*
         * Indicate supported options.
         */
        OFLG_SET(OPTION_PROBE_CLKS, &ATI.ChipOptionFlags);
        OFLG_SET(OPTION_CSYNC,      &ATI.ChipOptionFlags);
        if (ATIBoard == ATI_BOARD_V4)
                OFLG_SET(OPTION_UNDOC_CLKS, &ATI.ChipOptionFlags);

        /*
         * Our caller doesn't necessarily get back to us.  So, remove its
         * privileges until it does.
         */
        ATIEnterLeave(LEAVE);

#ifndef MONOVGA
#ifdef XFreeXDGA
	vga256InfoRec.directMode = XF86DGADirectPresent;
#endif
#endif

        /*
         * Return success.
         */
        return (TRUE);
}

/*
 * ATIEnterLeave --
 *
 * This function is called when the virtual terminal on which the server is
 * running is entered or left, as well as when the server starts up and is shut
 * down.  Its function is to obtain and relinquish I/O permissions for the SVGA
 * device.  This includes unlocking access to any registers that may be
 * protected on the chipset, and locking those registers again on exit.
 */
static void
ATIEnterLeave(const Bool enter)
{
        static unsigned char saved_a6, saved_ab,
                saved_b1, saved_b4, saved_b5, saved_b6,
                saved_b8, saved_b9, saved_be;
        static unsigned short saved_clock_sel, saved_misc_options,
                saved_mem_bndry, saved_mem_cfg;
        static unsigned int saved_config_cntl, saved_crtc_gen_cntl,
                saved_mem_info, saved_gen_test_cntl;

        static Bool entered = LEAVE;
        unsigned int tmp;

#ifndef MONOVGA
#ifdef XFreeXDGA
	if (vga256InfoRec.directMode&XF86DGADirectGraphics && !enter)
		return;
#endif
#endif

        if (enter == entered)
                return;
        entered = enter;

        if (enter == ENTER)
        {
                xf86EnableIOPorts(vga256InfoRec.scrnIndex);

                if (Chip_Has_SUBSYS_CNTL)
                {
                        /* Save register values to be modified */
                        saved_clock_sel = inw(CLOCK_SEL);
                        if (ATIChip >= ATI_CHIP_68800)
                        {
                                saved_misc_options = inw(MISC_OPTIONS);
                                saved_mem_bndry = inw(MEM_BNDRY);
                                saved_mem_cfg = inw(MEM_CFG);
                        }

                        /* Reset the 8514/A and disable all interrupts */
                        outw(SUBSYS_CNTL, GPCTRL_RESET | CHPTEST_NORMAL);
                        outw(SUBSYS_CNTL, GPCTRL_ENAB | CHPTEST_NORMAL |
                                RVBLNKFLG | RPICKFLAG | RINVALIDIO | RGPIDLE);

                        /* Ensure VGA is enabled */
                        outw(CLOCK_SEL, saved_clock_sel & ~DISABPASSTHRU);
                        if (ATIChip >= ATI_CHIP_68800)
                        {
                                outw(MISC_OPTIONS, saved_misc_options &
                                        ~(DISABLE_VGA | DISABLE_DAC));

                                /* Disable any video memory boundary */
                                outw(MEM_BNDRY, saved_mem_bndry &
                                        ~(MEM_PAGE_BNDRY | MEM_BNDRY_ENA));

                                /* Disable direct video memory aperture */
                                outw(MEM_CFG, saved_mem_cfg &
                                        ~(MEM_APERT_SEL | MEM_APERT_PAGE |
                                                MEM_APERT_LOC));
                        }

                        /* Wait for all activity to die down */
                        ProbeWaitIdleEmpty();
                }
                else if (ATIChip >= ATI_CHIP_88800)
                {
                        /* Save register values to be modified */
                        saved_config_cntl = inl(CONFIG_CNTL);
                        saved_crtc_gen_cntl = inl(CRTC_GEN_CNTL);
                        saved_mem_info = inl(MEM_INFO);

                        /* Reset everything */
                        tmp = inl(BUS_CNTL);
                        outl(BUS_CNTL, (tmp &
                                ~(BUS_ROM_DIS | BUS_FIFO_ERR_INT_EN |
                                        BUS_HOST_ERR_INT_EN)) |
                                BUS_FIFO_ERR_INT | BUS_HOST_ERR_INT);
                        saved_gen_test_cntl = inl(GEN_TEST_CNTL) &
                                (GEN_OVR_OUTPUT_EN | GEN_OVR_POLARITY |
                                 GEN_CUR_EN | GEN_BLOCK_WR_EN);
                        tmp = saved_gen_test_cntl & ~GEN_CUR_EN;
                        outl(GEN_TEST_CNTL, tmp);
                        outl(GEN_TEST_CNTL, tmp | GEN_GUI_EN);

                        /* Ensure VGA aperture is enabled */
                        outl(CONFIG_CNTL, saved_config_cntl &
                                ~(CFG_MEM_AP_SIZE | CFG_VGA_DIS |
                                        CFG_MEM_VGA_AP_EN));
                        outl(CRTC_GEN_CNTL,
                                saved_crtc_gen_cntl & ~CRTC_EXT_DISP_EN);
                        outl(MEM_INFO, saved_mem_info &
                                ~(CTL_MEM_BNDRY | CTL_MEM_BNDRY_EN));
                }

                /*
                 * Ensure all registers are read/write and disable all non-VGA
                 * emulations.
                 */
                saved_b1 = ATIGetExtReg(0xB1);
                ATIModifyExtReg(0xB1, saved_b1, 0xFC, 0x00);
                saved_b4 = ATIGetExtReg(0xB4);
                ATIModifyExtReg(0xB4, saved_b4, 0x00, 0x00);
                saved_b5 = ATIGetExtReg(0xB5);
                ATIModifyExtReg(0xB5, saved_b5, 0xBF, 0x00);
                saved_b6 = ATIGetExtReg(0xB6);
                ATIModifyExtReg(0xB6, saved_b6, 0xDD, 0x00);
                saved_b8 = ATIGetExtReg(0xB8);
                ATIModifyExtReg(0xB8, saved_b8, 0xC0, 0x00);
                saved_b9 = ATIGetExtReg(0xB9);
                ATIModifyExtReg(0xB9, saved_b9, 0x7F, 0x00);
                if (ATIChip != ATI_CHIP_18800)
                {
                        saved_be = ATIGetExtReg(0xBE);
                        ATIModifyExtReg(0xBE, saved_be, 0xFA, 0x01);
                        if (ATIChip >= ATI_CHIP_28800_2)
                        {
                                saved_a6 = ATIGetExtReg(0xA6);
                                ATIModifyExtReg(0xA6, saved_a6, 0x7F, 0x00);
                                saved_ab = ATIGetExtReg(0xAB);
                                ATIModifyExtReg(0xAB, saved_ab, 0xE7, 0x00);
                        }
                }

                vgaIOBase = (inb(R_GENMO) & 0x01) ?
                        ColourIOBase : MonochromeIOBase;

                /*
                 * There's a bizarre interaction here.  If bit 0x80 of CRTC[17]
                 * is on, then CRTC[3] is read-only.  If bit 0x80 of CRTC[3] is
                 * off, then CRTC[17] is write-only (or a read attempt actually
                 * returns bits from C/EGA's light pen position).  This means
                 * that if both conditions are met, CRTC[17]'s value on server
                 * entry cannot be retrieved.
                 */

                tmp = GetReg(CRTX(vgaIOBase), 0x03);
                if ((tmp & 0x80) ||
                    ((outb(CRTD(vgaIOBase), tmp | 0x80),
                        tmp = inb(CRTD(vgaIOBase))) & 0x80))
                {
                        /* CRTC[16-17] should be readable */
                        tmp = GetReg(CRTX(vgaIOBase), 0x11);
                        if (tmp & 0x80)         /* Unprotect CRTC[0-7] */
                                outb(CRTD(vgaIOBase), tmp & 0x7F);
                }
                else
                {
                        /*
                         * Could not make CRTC[17] readable, so unprotect
                         * CRTC[0-7] replacing VSyncEnd with zero.  This zero
                         * will be replaced after acquiring the needed access.
                         */
                        unsigned int VSyncEnd, VBlankStart, VBlankEnd;
                        unsigned char crt07, crt09;

                        PutReg(CRTX(vgaIOBase), 0x11, 0x20);
                        /* Make CRTC[16-17] readable */
                        PutReg(CRTX(vgaIOBase), 0x03, tmp | 0x80);
                        /* Make vertical synch pulse as wide as possible */
                        crt07 = GetReg(CRTX(vgaIOBase), 0x07);
                        crt09 = GetReg(CRTX(vgaIOBase), 0x09);
                        VBlankStart =
                                (((crt09 & 0x20) << 4) |
                                 ((crt07 & 0x08) << 5) |
                                 GetReg(CRTX(vgaIOBase), 0x15)) + 1;
                        VBlankEnd =
                                (VBlankStart & 0x380) |
                                (GetReg(CRTX(vgaIOBase), 0x16) & 0x7F);
                        if (VBlankEnd <= VBlankStart)
                                VBlankEnd += 0x80;
                        VSyncEnd =
                                (((crt07 & 0x80) << 2) |
                                 ((crt07 & 0x04) << 6) |
                                 GetReg(CRTX(vgaIOBase), 0x10)) + 0x0F;
                        if (VSyncEnd >= VBlankEnd)
                                VSyncEnd = VBlankEnd - 1;
                        PutReg(CRTX(vgaIOBase), 0x11,
                                (VSyncEnd & 0x0F) | 0x20);
                }
        }
        else
        {
                vgaIOBase = (inb(R_GENMO) & 0x01) ?
                        ColourIOBase : MonochromeIOBase;

                /* Protect CRTC[0-7] */
                tmp = GetReg(CRTX(vgaIOBase), 0x11);
                outb(CRTD(vgaIOBase), tmp | 0x80);

                /*
                 * Restore emulation and protection bits in ATI extended
                 * registers.
                 */
                ATIModifyExtReg(0xB1, -1, 0xFC, saved_b1);
                ATIModifyExtReg(0xB4, -1, 0x00, saved_b4);
                ATIModifyExtReg(0xB5, -1, 0xBF, saved_b5);
                ATIModifyExtReg(0xB6, -1, 0xDD, saved_b6);
                ATIModifyExtReg(0xB8, -1, 0xC0, saved_b8 & 0x03);
                ATIModifyExtReg(0xB9, -1, 0x7F, saved_b9);
                if (ATIChip != ATI_CHIP_18800)
                {
                        ATIModifyExtReg(0xBE, -1, 0xFA, saved_be);
                        if (ATIChip >= ATI_CHIP_28800_2)
                        {
                                ATIModifyExtReg(0xA6, -1, 0x7F, saved_a6);
                                ATIModifyExtReg(0xAB, -1, 0xE7, saved_ab);
                        }
                }

                if (Chip_Has_SUBSYS_CNTL)
                {
                        /* Reset the 8514/A and disable all interrupts */
                        outw(SUBSYS_CNTL, GPCTRL_RESET | CHPTEST_NORMAL);
                        outw(SUBSYS_CNTL, GPCTRL_ENAB | CHPTEST_NORMAL |
                                RVBLNKFLG | RPICKFLAG | RINVALIDIO | RGPIDLE);

                        /* Restore modified accelerator registers */
                        outw(CLOCK_SEL, saved_clock_sel);
                        if (ATIChip >= ATI_CHIP_68800)
                        {
                                outw(MISC_OPTIONS, saved_misc_options);
                                outw(MEM_BNDRY, saved_mem_bndry);
                                outw(MEM_CFG, saved_mem_cfg);
                        }

                        /* Wait for all activity to die down */
                        ProbeWaitIdleEmpty();
                }
                else if (ATIChip >= ATI_CHIP_88800)
                {
                        /* Reset everything */
                        tmp = inl(BUS_CNTL);
                        outl(BUS_CNTL, (tmp &
                                ~(BUS_ROM_DIS | BUS_FIFO_ERR_INT_EN |
                                        BUS_HOST_ERR_INT_EN)) |
                                 BUS_FIFO_ERR_INT | BUS_HOST_ERR_INT);
                        outl(GEN_TEST_CNTL, saved_gen_test_cntl);
                        outl(GEN_TEST_CNTL, saved_gen_test_cntl | GEN_GUI_EN);

                        /* Restore registers */
                        outl(CONFIG_CNTL, saved_config_cntl);
                        outl(CRTC_GEN_CNTL, saved_crtc_gen_cntl);
                        outl(MEM_INFO, saved_mem_info);
                }

                xf86DisableIOPorts(vga256InfoRec.scrnIndex);
        }
}

/*
 * ATIRestore --
 *
 * This function restores a video mode.  It basically writes out all of the
 * registers that have previously been saved in the vgaATIRec data structure.
 *
 * Note that "Restore" is slightly incorrect.  This function is also used when
 * the server enters/changes video modes.  The mode definitions have previously
 * been initialized by the Init() function, below.
 */
static void
ATIRestore(vgaATIPtr restore)
{
        unsigned char ae, b2, b8, b9, be;

        /*
         * Unlock registers.
         */
        ATIEnterLeave(ENTER);

        /*
         * Get (back) to bank 0.
         */
        b2 = ATIGetExtReg(0xB2);
        if (ATIChip == ATI_CHIP_18800)
        {
                if (b2 & 0x1E)
                        ATIPutExtReg(0xB2, b2 & 0xE1);
        }
        else
        {
                be = ATIGetExtReg(0xBE);
                if (b2)
                        ATIPutExtReg(0xB2, 0);
                ATIB2Reg = 0;
                if (ATIChip >= ATI_CHIP_28800_2)
                {
                        ae = ATIGetExtReg(0xAE);
                        if (ae & 0x0F)
                        {
                                ae &= 0xF0;
                                ATIPutExtReg(0xAE, ae);
                        }
                }
        }

        /*
         * Restore ATI registers.
         *
         * A special case - when using an external clock-setting program,
         * clock selection bits must not be changed.  This condition can
         * be checked by the condition:
         *
         *      if (restore->std.NoClock >= 0)
         *              restore clock-select bits.
         */

        b8 = ATIGetExtReg(0xB8);
        b9 = ATIGetExtReg(0xB9);
        if (restore->std.NoClock < 0)
        {
                /*
                 * Retrieve current setting of clock select bits.
                 */
                restore->b8 = b8;
                if (ATIChip == ATI_CHIP_18800)
                        restore->b2 = (restore->b2 & 0xBF) | (b2 & 0x40);
                else
                {
                        restore->be = (restore->be & 0xEF) | (be & 0x10);
                        if ((ATIBoard != ATI_BOARD_V4) ||
                            (OFLG_ISSET(OPTION_UNDOC_CLKS,
                                &vga256InfoRec.options)))
                                restore->b9 = (restore->b9 & 0xFD) |
                                        (b9 & 0x02);
                }
        }

        ATISaveScreen(SS_START);

        if (ATIChip == ATI_CHIP_18800)
                ATIModifyExtReg(0xB2, b2, 0x00, restore->b2);
        else
        {
                ATIModifyExtReg(0xBE, be, 0x00, restore->be);
                if (ATIChip >= ATI_CHIP_28800_2)
                {
                        ATIModifyExtReg(0xBF, -1, 0x00, restore->bf);
                        ATIModifyExtReg(0xA3, -1, 0x00, restore->a3);
                        ATIModifyExtReg(0xA6, -1, 0x00, restore->a6);
                        ATIModifyExtReg(0xA7, -1, 0x00, restore->a7);
                        ATIModifyExtReg(0xAB, -1, 0x00, restore->ab);
                        ATIModifyExtReg(0xAC, -1, 0x00, restore->ac);
                        ATIModifyExtReg(0xAD, -1, 0x00, restore->ad);
                        ATIModifyExtReg(0xAE, ae, 0x00, restore->ae);
                }
        }
        ATIModifyExtReg(0xB0, -1, 0x00, restore->b0);
        ATIModifyExtReg(0xB1, -1, 0x00, restore->b1);
        ATIModifyExtReg(0xB3, -1, 0x00, restore->b3);
        ATIModifyExtReg(0xB5, -1, 0x00, restore->b5);
        ATIModifyExtReg(0xB6, -1, 0x00, restore->b6);
        ATIModifyExtReg(0xB8, b8, 0x00, restore->b8);
        ATIModifyExtReg(0xB9, b9, 0x00, restore->b9);
        ATIModifyExtReg(0xBA, -1, 0x00, restore->ba);
        ATIModifyExtReg(0xBD, -1, 0x00, restore->bd);

#if 0
        outb(GENMO, restore->std.MiscOutReg);
        ATISaveScreen(SS_FINISH);
#endif

        /*
         * Restore the generic VGA registers.
         */
        vgaHWRestore((vgaHWPtr)restore);
}

/*
 * ATISave --
 *
 * This function saves the video state.  It reads all of the SVGA registers
 * into the vgaATIRec data structure.  There is in general no need to mask out
 * bits here - just read the registers.
 */
static void *
ATISave(vgaATIPtr save)
{
        unsigned char ae, b2;   /* The oddballs */

        /*
         * Unlock registers.
         */
        ATIEnterLeave(ENTER);

        /*
         * Get back to bank zero.
         */
        b2 = ATIGetExtReg(0xB2);
        if (ATIChip == ATI_CHIP_18800)
        {
                if (b2 & 0x1E)
                {
                        b2 &= 0xE1;
                        ATIPutExtReg(0xB2, b2);
                }
        }
        else
        {
                if (b2)
                {
                        ATIPutExtReg(0xB2, 0);
                        b2 = 0;
                }
                ATIB2Reg = 0;
                if (ATIChip >= ATI_CHIP_28800_2)
                {
                        if ((ae = ATIGetExtReg(0xAE)) & 0x0F)
                        {
                                ae &= 0xF0;
                                ATIPutExtReg(0xAE, ae);
                        }
                }
        }

        /*
         * vgaHWSave creates the data structure and fills in the generic VGA
         * portion.
         */
        save = (vgaATIPtr)vgaHWSave((vgaHWPtr)save, sizeof(vgaATIRec));

        /*
         * Save ATI-specific registers.
         */
        save->b0 = ATIGetExtReg(0xB0);
        save->b1 = ATIGetExtReg(0xB1);
        save->b2 = b2;
        save->b3 = ATIGetExtReg(0xB3);
        save->b5 = ATIGetExtReg(0xB5);
        save->b6 = ATIGetExtReg(0xB6);
        save->b8 = ATIGetExtReg(0xB8);
        save->b9 = ATIGetExtReg(0xB9);
        save->ba = ATIGetExtReg(0xBA);
        save->bd = ATIGetExtReg(0xBD);
        if (ATIChip != ATI_CHIP_18800)
        {
                save->be = ATIGetExtReg(0xBE);
                if (ATIChip >= ATI_CHIP_28800_2)
                {
                        save->bf = ATIGetExtReg(0xBF);
                        save->a3 = ATIGetExtReg(0xA3);
                        save->a6 = ATIGetExtReg(0xA6);
                        save->a7 = ATIGetExtReg(0xA7);
                        save->ab = ATIGetExtReg(0xAB);
                        save->ac = ATIGetExtReg(0xAC);
                        save->ad = ATIGetExtReg(0xAD);
                        save->ae = ae;
                }
        }

        return ((void *) save);
}

/*
 * ATIInit --
 *
 * This is the most important function (after the Probe function).  This
 * function fills in the vgaATIRec with all of the register values needed to
 * enable a video mode.
 */
static Bool
ATIInit(DisplayModePtr mode)
{
        int saved_mode_flags;

        /*
         * Unlock registers.
         */
        ATIEnterLeave(ENTER);

        /*
         * The VGA Wonder boards have a bit that multiplies all vertical
         * timing values by 2.  This feature is only used if it's actually
         * needed (i.e. when VTotal > 1024).  If the feature is needed, fake
         * out an interlaced mode and let vgaHWInit divide things by two.
         * Note that this prevents the (incorrect) use of this feature with
         * interlaced modes.
         */
        saved_mode_flags = mode->Flags;
        if (mode->VTotal > 1024)
                mode->Flags |= V_INTERLACE;

        /*
         * This will allocate the data structure and initialize all of the
         * generic VGA registers.
         */
        if (!vgaHWInit(mode,sizeof(vgaATIRec)))
        {
                mode->Flags = saved_mode_flags;
                return (FALSE);
        }

        /*
         * Override a few things.
         */
#       if !defined(MONOVGA) && !defined(XF86VGA16)
                new->std.Sequencer[4] = 0x0A;   /* instead of 0x0E */
                new->std.Graphics[5] = 0x00;    /* instead of 0x40 */
                new->std.Attribute[16] = 0x01;  /* instead of 0x41 */
                if ((ATIChip == ATI_CHIP_18800) &&
                    (vga256InfoRec.videoRam == 256))
                        new->std.CRTC[19] = vga256InfoRec.displayWidth >> 3;
                new->std.CRTC[23] = 0xE3;       /* instead of 0xC3 */
#       endif
        if (saved_mode_flags != mode->Flags)
        {
                /* Use "double vertical timings" bit */
                new->std.CRTC[23] |= 0x04;
                mode->Flags = saved_mode_flags;
        }

        /*
         * Set up ATI registers.
         */
#       if defined(MONOVGA) || defined(XF86VGA16)
                if (ATIChip <= ATI_CHIP_18800_1)
                        new->b0 = 0x00;
                else
                {
                        new->b0 = 0x00;
                        if (vga256InfoRec.videoRam > 512)
                                new->b0 |= 0x08;
                        else if (vga256InfoRec.videoRam > 256)
                                new->b0 |= 0x10;
                }
#       else
                new->b0 = 0x20;
                if (vga256InfoRec.videoRam > 512)
                        new->b0 |= 0x08;
                else if (vga256InfoRec.videoRam > 256)
                        new->b0 |= 0x10;
                else if (ATIChip <= ATI_CHIP_18800_1)
                        new->b0 |= 0x06;
#       endif
        new->b1 = (ATIGetExtReg(0xB1) & 0x04)       ;
        new->b3 = (ATIGetExtReg(0xB3) & 0x20)       ;
        new->b5 = 0;
#       if defined(MONOVGA) || defined(XF86VGA16)
                new->b6 = 0x40;
#       else
                new->b6 = 0x04;
#       endif
        if (vga256InfoRec.videoRam > 256)
                new->b6 |= 0x01;
        new->b8 = (ATIGetExtReg(0xB8) & 0xC0)       ;
        new->b9 = (ATIGetExtReg(0xB9) & 0x7F)       ;
        new->ba = 0;
        new->bd = (ATIGetExtReg(0xBD) & 0x02)       ;
        if (ATIChip == ATI_CHIP_18800)
                new->b2 = (ATIGetExtReg(0xB2) & 0xC0)       ;
        else
        {
                new->b2 = 0;
                new->be = (ATIGetExtReg(0xBE) & 0x30) | 0x09;
                if (ATIChip >= ATI_CHIP_28800_2)
                {
                        new->bf = (ATIGetExtReg(0xBF) & 0x5F)       ;
                        new->a3 = (ATIGetExtReg(0xA3) & 0x67)       ;
                        new->a6 = (ATIGetExtReg(0xA6) & 0x38) | 0x04;
                        new->a7 = (ATIGetExtReg(0xA7) & 0xFE)       ;
                        new->ab = (ATIGetExtReg(0xAB) & 0xE7)       ;
                        new->ac = (ATIGetExtReg(0xAC) & 0x8E)       ;
                        new->ad = 0;
                        new->ae = (ATIGetExtReg(0xAE) & 0xF0)       ;
                }
        }
        if (mode->Flags & V_INTERLACE)  /* Enable interlacing */
                if (ATIChip == ATI_CHIP_18800)
                        new->b2 |= 0x01;
                else
                        new->be |= 0x02;
        if (mode->Flags & V_DBLSCAN)
                new->b1 |= 0x08;        /* Enable double scanning */
        if ((OFLG_ISSET(OPTION_CSYNC, &vga256InfoRec.options)) ||
            (mode->Flags & (V_CSYNC | V_PCSYNC)))
                new->bd |= 0x08;        /* Enable composite synch */
        if (mode->Flags & V_NCSYNC)
                new->bd |= 0x09;        /* Invert composite synch polarity */

        if (new->std.NoClock >= 0)
        {
                /*
                 * Set clock select bits, possibly remapping them.
                 */
                int Clock = ATIMapClock(mode->Clock);

                /*
                 * Set generic clock select bits just in case.
                 */
                new->std.MiscOutReg = (new->std.MiscOutReg & 0xF3) |
                        ((Clock << 2) & 0x0C);

                /*
                 * Set ATI clock select bits.
                 */
                if (ATIChip == ATI_CHIP_18800)
                        new->b2 = (new->b2 & 0xBF) | ((Clock << 4) & 0x40);
                else
                {
                        new->be = (new->be & 0xEF) | ((Clock << 2) & 0x10);
                        if ((ATIBoard != ATI_BOARD_V4) ||
                            (OFLG_ISSET(OPTION_UNDOC_CLKS,
                                &vga256InfoRec.options)))
                        {
                                Clock >>= 1;
                                new->b9 = ((new->b9 & 0xFD) |
                                        ((Clock >> 1) & 0x02)) ^ saved_b9_bit_1;
                        }
                }

                /*
                 * Set clock divider bits.
                 */
                new->b8 = (new->b8 & 0x3F) | ((Clock << 3) & 0xC0);

                /*
                 * Modes using the higher clock frequencies need a non-zero
                 * Display Enable Skew.  The following number has been
                 * empirically determined to be between 1054 and 1342
                 * non-inclusively.
                 */
#               define Display_Enable_Skew_Threshold 1250

                /*
                 * Set a reasonable value for Display Enable Skew.
                 */
                new->std.CRTC[3] |=
                        (vga256InfoRec.clock[mode->Clock] /
                                Display_Enable_Skew_Threshold) & 0x60;
        }
        return (TRUE);
}

/*
 * ATIAdjust --
 *
 * This function is used to initialize the SVGA Start Address - the first
 * displayed location in the video memory.  This is used to implement the
 * virtual window.
 */
static void
ATIAdjust(const unsigned int x, const unsigned int y)
{
        int Base = (y * vga256InfoRec.displayWidth + x) >> 3;

        /*
         * Unlock registers.
         */
        ATIEnterLeave(ENTER);

        PutReg(CRTX(vgaIOBase), 0x0C, (Base & 0x00FF00) >> 8);
        PutReg(CRTX(vgaIOBase), 0x0D, (Base &   0x00FF)     );

        if (ATIChip <= ATI_CHIP_18800_1)
                ATIModifyExtReg(0xB0, -1, 0x3F, Base >> 10);
        else
        {
                ATIModifyExtReg(0xB0, -1, 0xBF, Base >> 10);
                ATIModifyExtReg(0xA3, -1, 0xEF, Base >> 13);

                /*
                 * I don't know if this also applies to Mach64's, but give it
                 * a shot...
                 */
                if (ATIChip >= ATI_CHIP_68800)
                        ATIModifyExtReg(0xAD, -1, 0xF3, Base >> 16);
        }
}

/*
 * ATIGetMode --
 *
 * This function will read the current SVGA register settings and produce a
 * filled-in DisplayModeRec containing the current mode.
 */
static void
ATIGetMode(DisplayModePtr mode)
{
        int ShiftCount = 0;
        unsigned char misc;
        unsigned char crt00, crt01, crt04, crt05, crt06, crt07, crt09,
                crt10, crt11, crt12, crt17;
        unsigned char b1, b2, b5, b8, b9, bd, be;

        /*
         * Unlock registers.
         */
        ATIEnterLeave(ENTER);

        /*
         * First, get the needed register values.
         */
        misc = inb(R_GENMO);
        vgaIOBase = ((misc & 0x01) * (ColourIOBase - MonochromeIOBase)) +
                MonochromeIOBase;

        crt00 = GetReg(CRTX(vgaIOBase), 0x00);
        crt01 = GetReg(CRTX(vgaIOBase), 0x01);
        crt04 = GetReg(CRTX(vgaIOBase), 0x04);
        crt05 = GetReg(CRTX(vgaIOBase), 0x05);
        crt06 = GetReg(CRTX(vgaIOBase), 0x06);
        crt07 = GetReg(CRTX(vgaIOBase), 0x07);
        crt09 = GetReg(CRTX(vgaIOBase), 0x09);
        crt10 = GetReg(CRTX(vgaIOBase), 0x10);
        crt11 = GetReg(CRTX(vgaIOBase), 0x11);
        crt12 = GetReg(CRTX(vgaIOBase), 0x12);
        crt17 = GetReg(CRTX(vgaIOBase), 0x17);

        b1 = ATIGetExtReg(0xB1);
        b5 = ATIGetExtReg(0xB5);
        b8 = ATIGetExtReg(0xB8);
        b9 = ATIGetExtReg(0xB9);
        bd = ATIGetExtReg(0xBD);
        if (ATIChip == ATI_CHIP_18800)
                b2 = ATIGetExtReg(0xB2);
        else
                be = ATIGetExtReg(0xBE);

        /*
         * Set clock number.
         */
        mode->Clock = (b8 & 0xC0) >> 3;                 /* Clock divider */
        if (ATIChip == ATI_CHIP_18800)
                mode->Clock |= (b2 & 0x40) >> 4;
        else
        {
                if ((ATIBoard != ATI_BOARD_V4) ||
                    OFLG_ISSET(OPTION_UNDOC_CLKS, &vga256InfoRec.options))
                {
                        mode->Clock |= ((b9 ^ saved_b9_bit_1) & 0x02) << 1;
                        mode->Clock <<= 1;
                }
                mode->Clock |= (be & 0x10) >> 2;
        }
        mode->Clock |= (misc & 0x0C) >> 2;              /* VGA clock select */
        mode->Clock = ATIUnmapClock(mode->Clock);       /* Reverse mapping */
        mode->SynthClock = vga256InfoRec.clock[mode->Clock];

        /*
         * Set horizontal display end.
         */
        mode->CrtcHDisplay = mode->HDisplay = (crt01 + 1) << 3;

        /*
         * Set horizontal synch pulse start.
         */
        mode->CrtcHSyncStart = mode->HSyncStart = crt04 << 3;

        /*
         * Set horizontal synch pulse end.
         */
        crt05 = (crt04 & 0xE0) | (crt05 & 0x1F);
        if (crt05 <= crt04)
                crt05 += 0x20;
        mode->CrtcHSyncEnd = mode->HSyncEnd = crt05 << 3;

        /*
         * Set horizontal total.
         */
        mode->CrtcHTotal = mode->HTotal = (crt00 + 5) << 3;

        /*
         * Set vertical display end.
         */
        mode->CrtcVDisplay = mode->VDisplay =
                (((crt07 & 0x40) << 3) | ((crt07 & 0x02) << 7) | crt12) + 1;

        /*
         * Set vertical synch pulse start.
         */
        mode->CrtcVSyncStart = mode->VSyncStart =
                (((crt07 & 0x80) << 2) | ((crt07 & 0x04) << 6) | crt10);

        /*
         * Set vertical synch pulse end.
         */
        mode->VSyncEnd = (mode->VSyncStart & 0x3F0) | (crt11 & 0x0F);
        if (mode->VSyncEnd <= mode->VSyncStart)
                mode->VSyncEnd += 0x10;
        mode->CrtcVSyncEnd = mode->VSyncEnd;

        /*
         */
        mode->CrtcVTotal = mode->VTotal =
                (((crt07 & 0x20) << 4) | ((crt07 & 0x01) << 8) | crt06) + 2;

        mode->CrtcVAdjusted = TRUE;

        /*
         * Set flags.
         */
        if (misc & 0x40)
                mode->Flags = V_NHSYNC;
        else
                mode->Flags = V_PHSYNC;
        if (misc & 0x80)
                mode->Flags |= V_NVSYNC;
        else
                mode->Flags |= V_PVSYNC;
        if (ATIChip == ATI_CHIP_18800)
        {
                if (b2 & 0x01)
                        mode->Flags |= V_INTERLACE;
        }
        else
        {
                if (be & 0x02)
                        mode->Flags |= V_INTERLACE;
        }
        if ((b1 & 0x08) || (crt09 & 0x80))
                mode->Flags |= V_DBLSCAN;
        if ((bd & 0x09) == 0x09)
                mode->Flags |= V_NCSYNC;
        else if (bd & 0x08)
                mode->Flags |= V_PCSYNC;

        /*
         * Adjust vertical timings.
         */
        if (mode->Flags & V_INTERLACE)
                ShiftCount++;
        if (mode->Flags & V_DBLSCAN)
                ShiftCount--;
        if (b1 & 0x40)
                ShiftCount--;
        if (crt17 & 0x04)
                ShiftCount++;
        if (ShiftCount > 0)
        {
                mode->VDisplay <<= ShiftCount;
                mode->VSyncStart <<= ShiftCount;
                mode->VSyncEnd <<= ShiftCount;
                mode->VTotal <<= ShiftCount;
        }
        else if (ShiftCount < 0)
        {
                mode->VDisplay >>= -ShiftCount;
                mode->VSyncStart >>= -ShiftCount;
                mode->VSyncEnd >>= -ShiftCount;
                mode->VTotal >>= -ShiftCount;
        }
}

/*
 * ATISaveScreen --
 *
 * This performs a sequencer reset.
 */
static void
ATISaveScreen(const Bool start)
{
        static Bool started = SS_FINISH;

        if (start == started)
                return;
        started = start;

        if (start == SS_START)                  /* Start synchronous reset */
                PutReg(SEQX, 0x00, 0x02);
        else                                    /* End synchronous reset */
                PutReg(SEQX, 0x00, 0x03);
}

/*
 * ATIValidMode --
 *
 * This is only a dummy place-holder for now.
 */
static Bool
ATIValidMode(DisplayModePtr mode)
{
        return (TRUE);
}
