/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/i128/i128reg.h,v 3.1 1995/12/16 08:19:52 dawes Exp $ */
/*
 * Copyright 1994 by Robin Cutshaw <robin@XFree86.Org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Robin Cutshaw not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Robin Cutshaw makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ROBIN CUTSHAW DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ROBIN CUTSHAW BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: i128reg.h /main/2 1995/12/17 08:13:20 kaleb $ */


struct i128pci {
    unsigned long devicevendor;
    unsigned long statuscommand;
    unsigned long classrev;
    unsigned long bhlc;
    unsigned long base0;
    unsigned long base1;
    unsigned long base2;
    unsigned long base3;
    unsigned long base4;
    unsigned long base5;
    unsigned long rsvd0;
    unsigned long rsvd1;
    unsigned long baserom;
    unsigned long rsvd2;
    unsigned long rsvd3;
    unsigned long lgii;
};

struct i128io {
    unsigned long rbase_g;
    unsigned long rbase_w;
    unsigned long rbase_a;
    unsigned long rbase_b;
    unsigned long rbase_i;
    unsigned long rbase_e;
    unsigned long id;
    unsigned long config1;
    unsigned long config2;
    unsigned long rsvd1;
    unsigned long soft_sw;
};

struct i128mem {
    unsigned char *mw0_ad;
    unsigned char *mw1_ad;
    unsigned char *xyw_ada;
    unsigned char *xyw_adb;
    unsigned long *rbase_g;
    unsigned long *rbase_w;
    unsigned long *rbase_a;
    unsigned long *rbase_b;
    unsigned long *rbase_i;
    unsigned char *rbase_g_b;  /* special byte pointer for ramdac registers */
};

#define I128_DEVICE_ID1		0x2309105D
#define I128_DEVICE_ID2		0x2339105D

/* RBASE_I register offsets */

#define GINTP 0x0000
#define GINTM 0x0004

/* RBASE_G register offsets  (divided by four for double word indexing */

#define WR_ADR   0x0000     /* use rbase_g_b for byte indexing */
#define PAL_DAT  0x0004     /* use rbase_g_b for byte indexing */
#define PEL_MASK 0x0008     /* use rbase_g_b for byte indexing */
#define RD_ADR   0x000C     /* use rbase_g_b for byte indexing */
#define INDEX_TI 0x0018     /* TI  ramdac use rbase_g_b for byte indexing */
#define DATA_TI  0x001C     /* TI  ramdac use rbase_g_b for byte indexing */
#define IDXL_I   0x0010     /* IBM ramdac use rbase_g_b for byte indexing */
#define IDXH_I   0x0014     /* IBM ramdac use rbase_g_b for byte indexing */
#define DATA_I   0x0018     /* IBM ramdac use rbase_g_b for byte indexing */
#define IDXCTL_I 0x001C     /* IBM ramdac use rbase_g_b for byte indexing */
#define INT_VCNT 0x0020/4
#define INT_HCNT 0x0024/4
#define DB_ADR   0x0028/4
#define DB_PTCH  0x002C/4
#define CRT_HAC  0x0030/4
#define CRT_HBL  0x0034/4
#define CRT_HFP  0x0038/4
#define CRT_HS   0x003C/4
#define CRT_VAC  0x0040/4
#define CRT_VBL  0x0044/4
#define CRT_VFP  0x0048/4
#define CRT_VS   0x004C/4
#define CRT_BORD 0x0050/4
#define CRT_ZOOM 0x0054/4
#define CRT_1CON 0x0058/4
#define CRT_2CON 0x005C/4


/* RBASE_W register offsets  (divided by four for double word indexing */

#define MW0_CTRL 0x0000/4
#define MW0_AD   0x0004/4
#define MW0_SZ   0x0008/4   /* 2MB = 0x9, 4MB = 0xA, 8MB = 0xB */
#define MW0_PGE  0x000C/4
#define MW0_ORG  0x0010/4
#define MW0_MSRC 0x0018/4
#define MW0_WKEY 0x001C/4
#define MW0_KDAT 0x0020/4
#define MW0_MASK 0x0024/4

/* raster operations */

#define MIX_CLEAR          0x00
#define MIX_NOR            0x01
#define MIX_AND_INVERTED   0x02
#define MIX_COPY_INVERTED  0x03
#define MIX_AND_REVERSE    0x04
#define MIX_INVERT         0x05
#define MIX_XOR            0x06
#define MIX_NAND           0x07
#define MIX_AND            0x08
#define MIX_EQUIV          0x09
#define MIX_NOOP           0x0A
#define MIX_OR_INVERTED    0x0B
#define MIX_COPY           0x0C
#define MIX_OR_REVERSED    0x0D
#define MIX_OR             0x0E
#define MIX_SET            0x0F

typedef struct {
	unsigned char r, b, g;
} LUTENTRY;

#define RGB8_PSEUDO      (-1)
#define RGB16_565         0
#define RGB16_555         1
#define RGB32_888         2
