/*
 * @(#) mgaDefs.h 11.1 97/10/22
 *
 * Copyright (C) 1994-1995 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right
 * to use, modify, and incorporate this code into other products for purposes
 * authorized by the license agreement provided they include this notice
 * and the associated copyright notice with any such product.  The
 * information in this file is provided "AS IS" without warranty.
 *
 */

/*
 *	S000	Thu Jun  1 16:52:52 PDT 1995	brianm@sco.com
 *	- New code from Matrox.
 */

#ifndef _mgadefs_h_
#define _mgadefs_h_

extern unsigned long mgaALU[16];

/* mga source or destination window def (I hope) */

typedef struct _mgaSDWin
{
	unsigned char window[0x1c00];
} mgaSDWin, *mgaSDWinPtr;

/*** As per Titan (Drawing Engine) specification 1.0 ***/

typedef struct _mgaTitanDrawRegs
{
	unsigned long dwgctl;          /* 0x000 */
	unsigned long maccess;         /* 0x004 */
	unsigned long mctlwtst;        /* 0x008 */
	unsigned long unused0;         /* 0x00C */
	unsigned long dst0;            /* 0x010 */
	unsigned long dst1;            /* 0x014 */
	unsigned long zmsk;            /* 0x018 */
	unsigned long plnwt;           /* 0x01C */
	unsigned long bcol;            /* 0x020 */
	unsigned long fcol;            /* 0x024 */
	unsigned long unused1;         /* 0x028 */
	unsigned long srcblt;          /* 0x02C */
	unsigned long src0;            /* 0x030 */
	unsigned long src1;            /* 0x034 */
	unsigned long src2;            /* 0x038 */
	unsigned long src3;            /* 0x03C */
	unsigned long xystrt;          /* 0x040 */
	unsigned long xyend;           /* 0x044 */
	unsigned long unused2;         /* 0x048 */
	unsigned long unused3;         /* 0x04C */
	unsigned long shift;           /* 0x050 */
	unsigned long unused4;         /* 0x054 */
	unsigned long sgn;             /* 0x058 */
	unsigned long len;             /* 0x05C */
	unsigned long ar0;             /* 0x060 */
	unsigned long ar1;             /* 0x064 */
	unsigned long ar2;             /* 0x068 */
	unsigned long ar3;             /* 0x06C */
	unsigned long ar4;             /* 0x070 */
	unsigned long ar5;             /* 0x074 */
	unsigned long ar6;             /* 0x078 */
	unsigned long unused5[4];      /* 0x07C-88 */
	unsigned long pitch;           /* 0x08C */
	unsigned long ydst;            /* 0x090 */
	unsigned long ydstorg;         /* 0x094 */
	unsigned long ytop;            /* 0x098 */
	unsigned long ybot;            /* 0x09C */
	unsigned long cxleft;          /* 0x0A0 */
	unsigned long cxright;         /* 0x0A4 */
	unsigned long fxleft;          /* 0x0A8 */
	unsigned long fxright;         /* 0x0AC */
	unsigned long xdst;            /* 0x0B0 */
	unsigned long unused6[3];      /* 0x0B4-BC */
	unsigned long dr0;             /* 0x0C0 */
	unsigned long dr1;             /* 0x0C4 */
	unsigned long dr2;             /* 0x0C8 */
	unsigned long dr3;             /* 0x0CC */
	unsigned long dr4;             /* 0x0D0 */
	unsigned long dr5;             /* 0x0D4 */
	unsigned long dr6;             /* 0x0D8 */
	unsigned long dr7;             /* 0x0DC */
	unsigned long dr8;             /* 0x0E0 */
	unsigned long dr9;             /* 0x0E4 */
	unsigned long dr10;            /* 0x0E8 */
	unsigned long dr11;            /* 0x0EC */
	unsigned long dr12;            /* 0x0F0 */
	unsigned long dr13;            /* 0x0F4 */
	unsigned long dr14;            /* 0x0F8 */
	unsigned long dr15;            /* 0x0FC */
} magaTitanDrawRegs;

/*** As per Titan (Host Interface VGA/CRTC) specification 0.2 ***/

typedef struct _vgaRegs
{
	unsigned char crtc_addr0;      /* io space 0x3b0 */
	unsigned char crtc_data0;      /* io space 0x3b1 */
	unsigned char crtc_addr1;      /* io space 0x3b2 */
	unsigned char crtc_data1;      /* io space 0x3b3 */
	unsigned char crtc_addr2;      /* io space 0x3b4 */
	unsigned char crtc_data2;      /* io space 0x3b5 */
	unsigned char crtc_addr3;      /* io space 0x3b6 */
	unsigned char crtc_data3;      /* io space 0x3b7 */
	unsigned char her_mode;        /* io space 0x3b8 */
	unsigned char her_lp_set;      /* io space 0x3b9 */
	unsigned char misc_istat1;     /* io space 0x3ba */
	unsigned char her_lp_clr;      /* io space 0x3bb */
	unsigned char unused0[3];      /* io space 0x3bc-0x3be */
	unsigned char her_conf;        /* io space 0x3bf */
	unsigned char attr_addr;       /* io space 0x3c0 */
	unsigned char attr_data;       /* io space 0x3c1 */
	unsigned char misc_istat0;     /* io space 0x3c2 */
	unsigned char vga_subsys;      /* io space 0x3c3 */
	unsigned char seq_addr;        /* io space 0x3c4 */
	unsigned char seq_data;        /* io space 0x3c5 */
	unsigned char unused1;         /* io space 0x3c6 */
	unsigned char dac_status;      /* io space 0x3c7 */
	unsigned char unused2[2];      /* io space 0x3c8-3c9 */
	unsigned char feat_ctl_r;      /* io space 0x3ca */
	unsigned char unused3;         /* io space 0x3cb */
	unsigned char misc_out;        /* io space 0x3cc */
	unsigned char unused4;         /* io space 0x3cd */
	unsigned char gctl_addr;       /* io space 0x3ce */
	unsigned char gctl_data;       /* io space 0x3cf */
	unsigned char crtc_addr4;      /* io space 0x3d0 */
	unsigned char crtc_data4;      /* io space 0x3d1 */
	unsigned char crtc_addr5;      /* io space 0x3d2 */
	unsigned char crtc_data5;      /* io space 0x3d3 */
	unsigned char crtc_addr6;      /* io space 0x3d4 */
	unsigned char crtc_data6;      /* io space 0x3d5 */
	unsigned char crtc_addr7;      /* io space 0x3d6 */
	unsigned char crtc_data7;      /* io space 0x3d7 */
	unsigned char cga_mode;        /* io space 0x3d8 */
	unsigned char cga_col_sl;      /* io space 0x3d9 */
	unsigned char misc_istat11;    /* io space 0x3da */
	unsigned char cga_lp_clr;      /* io space 0x3db */
	unsigned char cga_lp_set;      /* io space 0x3dc */
	unsigned char unused7;         /* io space 0x3dd */
	unsigned char aux_addr;        /* io space 0x3de */
	unsigned char aux_data;        /* io space 0x3df */
	unsigned char unused8[32];     /* io space 0x3e0-3ff */
} vgaRegs, *vgaRegsPtr;

#define misc_outr misc_out
#define misc_outw misc_istat0
#define feat_ctl0 misc_istat1
#define feat_ctl1 misc_istat11

typedef struct _mgaTitanRegs
{

	magaTitanDrawRegs drawSetup;	/* 0-ff */
	magaTitanDrawRegs drawGo;	/* 100-1ff */

/*** As per Titan (Host Interface Non-VGA) specification 0.2 ***/
                              
	unsigned long srcpage;         /* 0x200 */
	unsigned long dstpage;         /* 0x204 */
	unsigned long bytaccdata;      /* 0x208 */
	unsigned long adrgen;          /* 0x20c */
	unsigned long fifostatus;      /* 0x210 */
	unsigned long status;          /* 0x214 */
	unsigned long iclear;          /* 0x218 */
	unsigned long ien;             /* 0x21c */
	unsigned long unused0[2];      /* 0x220,224 */
	unsigned long intsts;	       /* 0x228 */
	unsigned long unused1[5];      /* 0x22c-23c */
	unsigned long rst;             /* 0x240 */
	unsigned long test;            /* 0x244 */
	unsigned long rev;             /* 0x248 */
	unsigned long unused2;         /* 0x24c */
	unsigned long config;          /* 0x250 */
	unsigned long opmode;          /* 0x254 */
	unsigned long unused3;         /* 0x258 */
	unsigned long crtc_ctrl;       /* 0x25c */
	unsigned char unused4[0x3b0 - 0x260]; /* 260-3af, offset to vga regs */
	vgaRegs vga;
} mgaTitanRegs, *mgaTitanRegsPtr;

/*** As per Dubic specification 0.2 ***/

typedef struct _mgaDubicRegs
{
	unsigned char dub_sel;      /* 0x00   BYTE ACCESSES ONLY ***/
	unsigned char unused0[3];
	unsigned char ndx_ptr;      /* 0x04   BYTE ACCESSES ONLY ***/
	unsigned char unused1[3];
	unsigned char data;         /* 0x08   BYTE ACCESSES ONLY ***/
	unsigned char unused2[3];
	unsigned char dub_mous;     /* 0x0c   BYTE ACCESSES ONLY ***/
	unsigned char unused3[3];
	unsigned char mouse0;       /* 0x10   BYTE ACCESSES ONLY ***/
	unsigned char unused4[3];
	unsigned char mouse1;       /* 0x14   BYTE ACCESSES ONLY ***/
	unsigned char unused5[3];
	unsigned char mouse2 ;      /* 0x18   BYTE ACCESSES ONLY ***/
	unsigned char unused6[3];
	unsigned char mouse3;       /* 0x1c   BYTE ACCESSES ONLY ***/
	unsigned char unused7[3];

} mgaDubicRegs, *mgaDubicRegsPtr;

/*****************************************************************************

 RAMDAC rgisters

*/

/*** DIRECT ***/

typedef struct _mgaBt481
{
	unsigned char wadr_pal;			/* 0x00 */
	unsigned char unused0[3];
	unsigned char col_pal;			/* 0x04 */
	unsigned char unused1[3];
	unsigned char pix_rd_msk;		/* 0x08 */
	unsigned char unused2[3];
	unsigned char radr_pal;			/* 0x0c */
	unsigned char unused3[3];
	unsigned char wadr_ovl;			/* 0x10 */
	unsigned char unused4[3];
	unsigned char col_ovl;			/* 0x14 */
	unsigned char unused5[3];
	unsigned char cmd_rega;			/* 0x18 */
	unsigned char unused6[3];
	unsigned char radr_ovl;			/* 0x1c */
	unsigned char unused7[3];

} mgaBt481, *mgaBt481Ptr;

/*** DIRECT ***/

typedef struct _mgaBt482
{
	unsigned char wadr_pal;			/* 0x00 */
	unsigned char unused0[3];
	unsigned char col_pal;			/* 0x04 */
	unsigned char unused1[3];
	unsigned char pix_rd_msk;		/* 0x08 */
	unsigned char unused2[3];
	unsigned char radr_pal;			/* 0x0c */
	unsigned char unused3[3];
	unsigned char wadr_ovl;			/* 0x10 */
	unsigned char unused4[3];
	unsigned char col_ovl;			/* 0x14 */
	unsigned char unused5[3];
	unsigned char cmd_rega;			/* 0x18 */
	unsigned char unused6[3];
	unsigned char radr_ovl;			/* 0x1c */
	unsigned char unused7[3];
} mgaBt482, *mgaBt482Ptr;


/*** DIRECT ***/

typedef struct _mgaBt484
{
	unsigned char wadr_pal;			/* 0x00 */
	unsigned char unused0[3];
	unsigned char col_pal;			/* 0x04 */
	unsigned char unused1[3];
	unsigned char pix_rd_msk;		/* 0x08 */
	unsigned char unused2[3];
	unsigned char radr_pal;			/* 0x0c */
	unsigned char unused3[3];
	unsigned char wadr_ovl;			/* 0x10 */
	unsigned char unused4[3];
	unsigned char col_ovl;			/* 0x14 */
	unsigned char unused5[3];
	unsigned char cmd_reg0;			/* 0x18 */
	unsigned char unused6[3];
	unsigned char radr_ovl;			/* 0x1c */
	unsigned char unused7[3];
	unsigned char cmd_reg1;			/* 0x20 */
	unsigned char unused8[3];
	unsigned char cmd_reg2;			/* 0x24 */
	unsigned char unused9[3];
	unsigned char status;			/* 0x28 */
	unsigned char unuseda[3];
	unsigned char cur_ram; 			/* 0x2c */
	unsigned char unusedb[3];
	unsigned char cur_xlow;			/* 0x30 */
	unsigned char unusedc[3];
	unsigned char cur_xhi; 			/* 0x34 */
	unsigned char unusedd[3];
	unsigned char cur_ylow;			/* 0x38 */
	unsigned char unusede[3];
	unsigned char cur_yhi; 			/* 0x3c */
	unsigned char unusedf[3];
} mgaBt484, *mgaBt484Ptr;


typedef struct _mgaBt485
{
	unsigned char wadr_pal;			/* 0x00 */
	unsigned char unused0[3];
	unsigned char col_pal;			/* 0x04 */
	unsigned char unused1[3];
	unsigned char pix_rd_msk;		/* 0x08 */
	unsigned char unused2[3];
	unsigned char radr_pal;			/* 0x0c */
	unsigned char unused3[3];
	unsigned char wadr_ovl;			/* 0x10 */
	unsigned char unused4[3];
	unsigned char col_ovl;			/* 0x14 */
	unsigned char unused5[3];
	unsigned char cmd_reg0;			/* 0x18 */
	unsigned char unused6[3];
	unsigned char radr_ovl;			/* 0x1c */
	unsigned char unused7[3];
	unsigned char cmd_reg1;			/* 0x20 */
	unsigned char unused8[3];
	unsigned char cmd_reg2;			/* 0x24 */
	unsigned char unused9[3];
	unsigned char status;			/* 0x28 */
	unsigned char unuseda[3];
	unsigned char cur_ram; 			/* 0x2c */
	unsigned char unusedb[3];
	unsigned char cur_xlow;			/* 0x30 */
	unsigned char unusedc[3];
	unsigned char cur_xhi; 			/* 0x34 */
	unsigned char unusedd[3];
	unsigned char cur_ylow;			/* 0x38 */
	unsigned char unusede[3];
	unsigned char cur_yhi; 			/* 0x3c */
	unsigned char unusedf[3];
} mgaBt485, *mgaBt485Ptr;

#define cmd_reg3 status				/* 0x28 */


/*****************************************************************************/

/*** VIEWPOINT REGISTER DIRECT ***/

typedef struct _mgaViewPoint
{
	unsigned char wadr_pal;			/* 0x00 */
	unsigned char unused0[3];
	unsigned char col_pal;			/* 0x04 */
	unsigned char unused1[3];
	unsigned char pix_rd_msk;		/* 0x08 */
	unsigned char unused2[3];
	unsigned char radr_pal;			/* 0x0c */
	unsigned char unused3[3];
	unsigned char unused4[8];		/* 0x10-14 */
	unsigned char index;			/* 0x18 */
	unsigned char unused5[3];
	unsigned char data;			/* 0x1c */
	unsigned char unused6[3];
} mgaViewPoint, *mgaViewPointPtr;

/*** TVP3026 REGISTER DIRECT ***/

typedef struct _mgaTVP3026
{
	unsigned char index;  			/* 0x00 *//* same as wadr_pal */
	unsigned char unused0[3];
	unsigned char col_pal;			/* 0x04 */
	unsigned char unused1[3];
	unsigned char pix_rd_msk;		/* 0x08 */
	unsigned char unused2[3];
	unsigned char radr_pal;			/* 0x0c */
	unsigned char unused3[3];
	unsigned char cur_col_addr;	/* 0x10 */
	unsigned char unused4[3];
	unsigned char cur_col_data;	/* 0x14 */
	unsigned char unused5[19];
	unsigned char data;  			/* 0x28 */
	unsigned char unused6[3];
	unsigned char cur_ram; 			/* 0x2c */
	unsigned char unused7[3];
        unsigned char cur_xlow;			/* 0x30 */
	unsigned char unused8[3];
	unsigned char cur_xhi; 			/* 0x34 */
	unsigned char unused9[3];
	unsigned char cur_ylow;			/* 0x38 */
	unsigned char unuseda[3];
	unsigned char cur_yhi; 			/* 0x3c */
	unsigned char unusedb[3];
} mgaTVP3026, *mgaTVP3026Ptr;

typedef union _mgaDacs
{
    mgaBt481 bt481;
    mgaBt482 bt482;
    mgaBt484 bt484;
    mgaBt485 bt485;
    mgaViewPoint vpoint;
    mgaTVP3026 tvp3026;
} mgaDacs, *mgaDacsPtr;

typedef struct _mgaRegs
{
    mgaSDWin srcwin;
    mgaTitanRegs titan;
    mgaSDWin dstwin;
    mgaDacs  ramdacs;
    unsigned char unused0[0x80 - sizeof(mgaDacs)];
    mgaDubicRegs dubic;
    unsigned char unused1[0x80 - sizeof(mgaDubicRegs)];
    unsigned char viwic[0x80];	/* place holder 'till I know what this is */
    unsigned char clkgen[0x80];	/* place holder 'till I know what this is */
    unsigned char expdev[0x200];/* place holder 'till I know what this is */

} mgaRegs, *mgaRegsPtr;


/*****************************************************************************

 DUBIC INDEX TO REGISTERS (NDX_PTR)

 These are the index values to access the Dubic indexed registers via NDX_PTR

*/

/*** As per Dubic specification 0.2 ***/

#define DUBIC_DUB_CTL         0x00
#define DUBIC_KEY_COL         0x01
#define DUBIC_KEY_MSK         0x02
#define DUBIC_DBX_MIN         0x03
#define DUBIC_DBX_MAX         0x04
#define DUBIC_DBY_MIN         0x05
#define DUBIC_DBY_MAX         0x06
#define DUBIC_OVS_COL         0x07
#define DUBIC_CUR_X           0x08
#define DUBIC_CUR_Y           0x09
#define DUBIC_DUB_CTL2        0x0A
#define DUBIC_CUR_COL0        0x0c
#define DUBIC_CUR_COL1        0x0d
#define DUBIC_CRC_CTL         0x0e
#define DUBIC_CRC_DAT         0x0f

/*****************************************************************************/

/* DUBIC FIELDS */

#define DUBIC_DB_SEL_M		   0x00000001
#define DUBIC_DB_SEL_A  		0
#define DUBIC_DB_SEL_DBA  		0x00000000
#define DUBIC_DB_SEL_DBB  		0x00000001

#define DUBIC_DB_EN_M			0x00000002
#define DUBIC_DB_EN_A			1
#define DUBIC_DB_EN_OFF			0x00000000
#define DUBIC_DB_EN_ON			0x00000002

#define DUBIC_IMOD_M          0x0000000c
#define DUBIC_IMOD_A          2
#define DUBIC_IMOD_32         0x00000000
#define DUBIC_IMOD_16         0x00000004
#define DUBIC_IMOD_8          0x00000008

#define DUBIC_LVID_M          0x00000070
#define DUBIC_LVID_A          4
#define DUBIC_LVID_OFF        0x00000000
#define DUBIC_LVID_COL_EQ     0x00000010
#define DUBIC_LVID_COL_GE     0x00000020
#define DUBIC_LVID_COL_LE     0x00000030
#define DUBIC_LVID_DB         0x00000040
#define DUBIC_LVID_COL_EQ_DB  0x00000050
#define DUBIC_LVID_COL_GE_DB  0x00000060
#define DUBIC_LVID_COL_LE_DB  0x00000070

#define DUBIC_FBM_M           0x00000380
#define DUBIC_FBM_A           7

#define DUBIC_START_BK_M      0x00000c00
#define DUBIC_START_BK_A      10

#define DUBIC_VSYNC_POL_M     0x00001000
#define DUBIC_VSYNC_POL_A     12

#define DUBIC_HSYNC_POL_M     0x00002000
#define DUBIC_HSYNC_POL_A     13

#define DUBIC_DACTYPE_M       0x0000c000
#define DUBIC_DACTYPE_A       14

#define DUBIC_INT_EN_M        0x00010000
#define DUBIC_INT_EN_A        16

#define DUBIC_GENLOCK_M       0x00040000
#define DUBIC_GENLOCK_A       18

#define DUBIC_BLANK_SEL_M     0x00080000
#define DUBIC_BLANK_SEL_A     19

#define DUBIC_SYNC_DEL_M      0x00f00000
#define DUBIC_SYNC_DEL_A      20

#define DUBIC_VGA_EN_M        0x01000000
#define DUBIC_VGA_EN_A        24

#define DUBIC_SRATE_M         0x7e000000
#define DUBIC_SRATE_A         25

#define DUBIC_BLANKDEL_M      0x80000000
#define DUBIC_BLANKDEL_A      31

#define DUBIC_CSYNCEN_M       0x00000001
#define DUBIC_CSYNCEN_A       0

#define DUBIC_SYNCEN_M        0x00000002
#define DUBIC_SYNCEN_A        1

#define DUBIC_LASEREN_M       0x00000004
#define DUBIC_LASEREN_A       2

#define DUBIC_LASERSCL_M      0x00000018
#define DUBIC_LASERSCL_A      3

#define DUBIC_LVIDFIELD_M     0x00000020
#define DUBIC_LVIDFIELD_A     5

#define DUBIC_CLKSEL_M        0x00000040
#define DUBIC_CLKSEL_A        6

#define DUBIC_LDCLKEN_M       0x00000080
#define DUBIC_LDCLKEN_A       7


/*****************************************************************************

 TITAN Drawing Engine field masks and values as per Titan specification 1.0

*/

#define TITAN_OPCOD_M                        0x0000000f
#define TITAN_OPCOD_A                        0
#define TITAN_OPCOD_LINE_OPEN                0x00000000
#define TITAN_OPCOD_AUTOLINE_OPEN            0x00000001
#define TITAN_OPCOD_LINE_CLOSE               0x00000002
#define TITAN_OPCOD_AUTOLINE_CLOSE           0x00000003
#define TITAN_OPCOD_TRAP                     0x00000004
#define TITAN_OPCOD_BITBLT                   0x00000008
#define TITAN_OPCOD_ILOAD                    0x00000009
#define TITAN_OPCOD_IDUMP                    0x0000000a

#define TITAN_ATYPE_M                        0x00000030
#define TITAN_ATYPE_A                        4
#define TITAN_ATYPE_RPL                      0x00000000
#define TITAN_ATYPE_RSTR                     0x00000010
#define TITAN_ATYPE_ANTI                     0x00000020
#define TITAN_ATYPE_ZI                       0x00000030

#define TITAN_BLOCKM_M                       0x00000040
#define TITAN_BLOCKM_A                       6
#define TITAN_BLOCKM_OFF                     0x00000000
#define TITAN_BLOCKM_ON                      0x00000040

#define TITAN_LINEAR_M                       0x00000080
#define TITAN_LINEAR_A                       7
#define TITAN_LINEAR_OFF                     0x00000000
#define TITAN_LINEAR_ON                      0x00000080  /*** spec 2.5 ***/

#define TITAN_BOP_M                          0x000f0000
#define TITAN_BOP_A                          16
#define TITAN_BOP_CLEAR                      0x00000000
#define TITAN_BOP_NOT_D_OR_S                 0x00010000
#define TITAN_BOP_D_AND_NOTS                 0x00020000
#define TITAN_BOP_NOTS                       0x00030000
#define TITAN_BOP_NOTD_AND_S                 0x00040000
#define TITAN_BOP_NOTD                       0x00050000
#define TITAN_BOP_D_XOR_S                    0x00060000
#define TITAN_BOP_NOT_D_AND_S                0x00070000
#define TITAN_BOP_D_AND_S                    0x00080000
#define TITAN_BOP_NOT_D_XOR_S                0x00090000
#define TITAN_BOP_D                          0x000a0000
#define TITAN_BOP_D_OR_NOTS                  0x000b0000
#define TITAN_BOP_S                          0x000c0000
#define TITAN_BOP_NOTD_OR_S                  0x000d0000
#define TITAN_BOP_D_OR_S                     0x000e0000
#define TITAN_BOP_SET                        0x000f0000

#define TITAN_TRANS_M                        0x00f00000
#define TITAN_TRANS_A                        20

#define TITAN_ALPHADIT_M                     0x01000000
#define TITAN_ALPHADIT_A                     24
#define TITAN_ALPHADIT_FCOL                  0x00000000
#define TITAN_ALPHADIT_RED                   0x01000000

#define TITAN_BLTMOD_M                       0x06000000
#define TITAN_BLTMOD_A                       25
#define TITAN_BLTMOD_BMONO                   0x00000000
#define TITAN_BLTMOD_BPLAN                   0x02000000
#define TITAN_BLTMOD_BFCOL                   0x04000000
#define TITAN_BLTMOD_BUCOL                   0x06000000

#define TITAN_ZDRWEN_M                       0x02000000
#define TITAN_ZDRWEN_A                       25
#define TITAN_ZDRWEN_OFF                     0x00000000
#define TITAN_ZDRWEN_ON                      0x02000000

#define TITAN_ZLTE_M                         0x04000000
#define TITAN_ZLTE_A                         26
#define TITAN_ZLTE_LT                        0x00000000
#define TITAN_ZLTE_LTE                       0x04000000

#define TITAN_TRANSC_M                       0x40000000   /* spec 2.2 */
#define TITAN_TRANSC_A                       30
#define TITAN_TRANSC_OPAQUE                  0x00000000
#define TITAN_TRANSC_TRANSPARENT             0x40000000

#define TITAN_AFOR_M                         0x08000000
#define TITAN_AFOR_A                         27
#define TITAN_AFOR_ALU                       0x00000000
#define TITAN_AFOR_FCOL                      0x08000000

#define TITAN_HBGR_M                         0x08000000
#define TITAN_HBGR_A                         27

#define TITAN_ABAC_M                         0x10000000
#define TITAN_ABAC_A                         28
#define TITAN_ABAC_DEST                      0x00000000
#define TITAN_ABAC_BCOL                      0x10000000

#define TITAN_HCPRS_M                        0x10000000
#define TITAN_HCPRS_A                        28
#define TITAN_HCPRS_SRC32                    0x00000000
#define TITAN_HCPRS_SRC24                    0x10000000

#define TITAN_PATTERN_M                      0x20000000
#define TITAN_PATTERN_A                      29
#define TITAN_PATTERN_OFF                    0x00000000
#define TITAN_PATTERN_ON                     0x20000000

#define TITAN_PWIDTH_M                       0x00000003
#define TITAN_PWIDTH_A                       0
#define TITAN_PWIDTH_PW8                     0x00000000
#define TITAN_PWIDTH_PW16                    0x00000001
#define TITAN_PWIDTH_PW32                    0x00000002
#define TITAN_PWIDTH_PW32I                   0x00000003

#define TITAN_FBC_M                          0x0000000c
#define TITAN_FBC_A                          2
#define TITAN_FBC_SBUF                       0x00000000
#define TITAN_FBC_DBUFA                      0x00000008
#define TITAN_FBC_DBUFB                      0x0000000c

#define TITAN_ZCOL_M                         0x0000000f
#define TITAN_ZCOL_A                         0

#define TITAN_PLNZMSK_M                      0x000000f0
#define TITAN_PLNZMSK_A                      4

#define TITAN_ZTEN_M                         0x00000100
#define TITAN_ZTEN_A                         8
#define TITAN_ZTEN_OFF                       0x00000000
#define TITAN_ZTEN_ON                        0x00000100

#define TITAN_ZCOLBLK_M                      0x00000200
#define TITAN_ZCOLBLK_A                      9

#define TITAN_FUNCNT_M                       0x0000007f
#define TITAN_FUNCNT_A                       0
#define TITAN_X_OFF_M                        0x0000000f
#define TITAN_X_OFF_A                        0
#define TITAN_Y_OFF_M                        0x00000070
#define TITAN_Y_OFF_A                        4
#define TITAN_FUNOFF_M                       0x003f0000
#define TITAN_FUNOFF_A                       16

#define TITAN_SDYDXL_M                       0x00000001
#define TITAN_SDYDXL_A                       0
#define TITAN_SDYDXL_Y_MAJOR                 0x00000000
#define TITAN_SDYDXL_X_MAJOR                 0x00000001

#define TITAN_SCANLEFT_M                     0x00000001
#define TITAN_SCANLEFT_A                     0
#define TITAN_SCANLEFT_OFF                   0x00000000
#define TITAN_SCANLEFT_ON                    0x00000001

#define TITAN_SDXL_M                         0x00000002
#define TITAN_SDXL_A                         1
#define TITAN_SDXL_POS                       0x00000000
#define TITAN_SDXL_NEG                       0x00000002

#define TITAN_SDY_M                          0x00000004
#define TITAN_SDY_A                          2
#define TITAN_SDY_POS                        0x00000000
#define TITAN_SDY_NEG                        0x00000004

#define TITAN_SDXR_M                         0x00000020
#define TITAN_SDXR_A                         5
#define TITAN_SDXR_POS                       0x00000000
#define TITAN_SDXR_NEG                       0x00000020

#define TITAN_YLIN_M                         0x00008000
#define TITAN_YLIN_A                         15

#define TITAN_AR0_M                          0x0003ffff
                                             
/*****************************************************************************/

/*****************************************************************************

 TITAN HostInterface field masks and values as per Host Interface spec 0.20

*/

#define TITAN_FIFOCOUNT_M                    0x0000007f
#define TITAN_FIFOCOUNT_A                    0

#define TITAN_BFULL_M                        0x00000100
#define TITAN_BFULL_A                        8

#define TITAN_BEMPTY_M                       0x00000200
#define TITAN_BEMPTY_A                       9

#define TITAN_BYTEACCADDR_M                  0x007f0000
#define TITAN_BYTEACCADDR_A                  16

#define TITAN_ADDRGENSTATE_M                 0x3f000000
#define TITAN_ADDRGENSTATE_A                 24

#define TITAN_BFERRISTS_M                    0x00000001
#define TITAN_BFERRISTS_A                    0

#define TITAN_DMATCISTS_M                    0x00000002
#define TITAN_DMATCISTS_A                    1

#define TITAN_PICKISTS_M                     0x00000004
#define TITAN_PICKISTS_A                     2

#define TITAN_VSYNCSTS_M                     0x00000008
#define TITAN_VSYNCSTS_A                     3
#define TITAN_VSYNCSTS_SET      	     0x00000008
#define TITAN_VSYNCSTS_CLR		     0x00000000

#define TITAN_BYTEFLAG_M                     0x00000f00
#define TITAN_BYTEFLAG_A                     8

#define TITAN_DWGENGSTS_M                    0x00010000
#define TITAN_DWGENGSTS_A                    16
#define TITAN_DWGENGSTS_BUSY                 0x00010000
#define TITAN_DWGENGSTS_IDLE                 0x00000000

#define TITAN_BFERRICLR_M                    0x00000001
#define TITAN_BFERRICLR_A                    0

#define TITAN_DMATCICLR_M                    0x00000002
#define TITAN_DMATCICLR_A                    1

#define TITAN_PICKICLR_M                     0x00000004
#define TITAN_PICKICLR_A                     2

#define TITAN_BFERRIEN_M                     0x00000001
#define TITAN_BFERRIEN_A                     0

#define TITAN_DMATCIEN_M                     0x00000002
#define TITAN_DMATCIEN_A                     1

#define TITAN_PICKIEN_M                      0x00000004
#define TITAN_PICKIEN_A                      2

#define TITAN_VSYNCIEN_M                     0x00000008
#define TITAN_VSYNCIEN_A                     3

#define TITAN_SOFTRESET_M                    0x00000001
#define TITAN_SOFTRESET_A                    0
#define TITAN_SOFTRESET_SET                  0x00000001
#define TITAN_SOFTRESET_CLR                  0x00000000

#define TITAN_VGATEST_M                      0x00000001
#define TITAN_VGATEST_A                      0

#define TITAN_ROBITWREN_M                    0x00000100
#define TITAN_ROBITWREN_A                    8

#define TITAN_CHIPREV_M                      0x0000000f
#define TITAN_CHIPREV_A                      0


#define TITAN_NODUBIC_M                      0x00000010
#define TITAN_NODUBIC_A                      4


#define TITAN_CONFIG_M                       0x00000003   /*** BYTE ACCESS ONLY ***/
#define TITAN_CONFIG_A                       0
#define TITAN_CONFIG_8                       0x00000000
#define TITAN_CONFIG_16                      0x00000001
#define TITAN_CONFIG_16N                     0x00000003

#define TITAN_DRIVERDY_M                     0x00000100
#define TITAN_DRIVERDY_A                     8

#define TITAN_BIOSEN_M                       0x00000200
#define TITAN_BIOSEN_A                       9

#define TITAN_VGAEN_M                        0x00000400
#define TITAN_VGAEN_A                        10

#define TITAN_LEVELIRQ_M                     0x00000800
#define TITAN_LEVELIRQ_A                     11

#define TITAN_EXPDEV_M                       0x00010000
#define TITAN_EXPDEV_A                       16

#define TITAN_MAPSEL_M                       0x07000000
#define TITAN_MAPSEL_A                       24
#define TITAN_MAPSEL_DISABLED                0x00000000
#define TITAN_MAPSEL_BASE_1                  0x01000000
#define TITAN_MAPSEL_BASE_2                  0x02000000
#define TITAN_MAPSEL_BASE_3                  0x03000000
#define TITAN_MAPSEL_BASE_4                  0x04000000
#define TITAN_MAPSEL_BASE_5                  0x05000000
#define TITAN_MAPSEL_BASE_6                  0x06000000
#define TITAN_MAPSEL_BASE_7                  0x07000000

#define TITAN_POSEIDON_M                     0x08000000
#define TITAN_POSEIDON_A                     27

#define TITAN_PCI_M                          0x08000000
#define TITAN_PCI_A                          27
#define TITAN_ISA_M                          0x10000000
#define TITAN_ISA_A                          28
#define TITAN_ISA_ISA_BUS                    0x10000000
#define TITAN_ISA_WIDE_BUS                   0x00000000

#define TITAN_PSEUDODMA_M                    0x00000001
#define TITAN_PSEUDODMA_A                    0

#define TITAN_DMAACT_M                       0x00000002
#define TITAN_DMAACT_A                       1

#define TITAN_DMAMOD_M                       0x0000000c
#define TITAN_DMAMOD_A                       2
#define TITAN_DMAMOD_GENERAL_WR              0x00000000
#define TITAN_DMAMOD_BLIT_WR                 0x00000004
#define TITAN_DMAMOD_VECTOR_WR               0x00000008
#define TITAN_DMAMOD_BLIT_RD                 0x0000000c

#define TITAN_NOWAIT_M                       0x00000010
#define TITAN_NOWAIT_A                       4

#define TITAN_MOUSEEN_M                      0x00000100
#define TITAN_MOUSEEN_A                      8

#define TITAN_MOUSEMAP_M                     0x00000200
#define TITAN_MOUSEMAP_A                     9

#define TITAN_ZTAGEN_M                       0x00000400
#define TITAN_ZTAGEN_A                       10

#define TITAN_VBANK0_M                       0x00000800
#define TITAN_VBANK0_A                       11

#define TITAN_RFHCNT_M                       0x000f0000
#define TITAN_RFHCNT_A                       16

#define TITAN_DST1_200MHZ_M                  0x00010000
#define TITAN_DST1_200MHZ_A                  16

#define TITAN_FBM_M                          0x00700000
#define TITAN_FBM_A                          20

#define TITAN_HYPERPG_M                      0x03000000
#define TITAN_HYPERPG_A                      24
#define TITAN_HYPERPG_NOHYPER                0x00000000
#define TITAN_HYPERPG_SELHYPER               0x01000000
#define TITAN_HYPERPG_ALLHYPER               0x02000000
#define TITAN_HYPERPG_RESERVED               0x03000000

#define TITAN_TRAM_M                         0x04000000
#define TITAN_TRAM_A                         26
#define TITAN_TRAM_256X8                     0x00000000
#define TITAN_TRAM_256X16                    0x04000000

#define TITAN_CRTCBPP_M                      0x00000003
#define TITAN_CRTCBPP_A                      0
#define TITAN_CRTCBPP_8                      0x00000000
#define TITAN_CRTCBPP_16                     0x00000001
#define TITAN_CRTCBPP_32                     0x00000002

#define TITAN_ALW_M                          0x00000004
#define TITAN_ALW_A                          2

#define TITAN_INTERLACE_M                    0x00000018
#define TITAN_INTERLACE_A                    3
#define TITAN_INTERLACE_OFF                  0x00000000
#define TITAN_INTERLACE_768                  0x00000008
#define TITAN_INTERLACE_1024                 0x00000010
#define TITAN_INTERLACE_1280                 0x00000018

#define TITAN_VIDEODELAY0_M                  0x00000020
#define TITAN_VIDEODELAY0_A                  5
#define TITAN_VIDEODELAY1_M                  0x00000200
#define TITAN_VIDEODELAY1_A                  9
#define TITAN_VIDEODELAY2_M                  0x00000400
#define TITAN_VIDEODELAY2_A                  10

#define TITAN_VSCALE_M                       0x00030000
#define TITAN_VSCALE_A                       16

#define TITAN_SYNCDEL_M                      0x000C0000
#define TITAN_SYNCDEL_A                      18

#define TITAN_DST0_RESERVED1_M               0x0000ffff
#define TITAN_DST0_RESERVED1_A               0
#define TITAN_DST0_PCBREV_M                  0x000f0000
#define TITAN_DST0_PCBREV_A                  16
#define TITAN_DST0_PRODUCT_M                 0x00f00000
#define TITAN_DST0_PRODUCT_A                 20
#define TITAN_DST0_RAMBANK_M                 0xff000000
#define TITAN_DST0_RAMBANK_A                 24
#define TITAN_DST1_RAMBANK_M                 0x00000001
#define TITAN_DST1_RAMBANK_A                 0
#define TITAN_DST1_RAMBANK0_M                0x00000008
#define TITAN_DST1_RAMBANK0_A                3
#define TITAN_DST1_RAMSPEED_M                0x00000006
#define TITAN_DST1_RAMSPEED_A                1
#define TITAN_DST1_RESERVED1_M               0x0007fff8
#define TITAN_DST1_RESERVED1_A               3
#define TITAN_DST1_HYPERPG_M                 0x00180000
#define TITAN_DST1_HYPERPG_A                 19
#define TITAN_DST1_EXPDEV_M                  0x00200000
#define TITAN_DST1_EXPDEV_A                  21
#define TITAN_DST1_TRAM_M                    0x00400000
#define TITAN_DST1_TRAM_A                    22
#define TITAN_DST1_RESERVED2_M               0xff800000
#define TITAN_DST1_RESERVED2_A               23

/*** INDIRECT ***/
#define BT481_RD_MSK				0x00
#define BT481_OVL_MSK 			0x01
#define BT481_CMD_REGB			0x02
#define BT481_CUR_REG			0x03
#define BT481_CUR_XLOW			0x04
#define BT481_CUR_XHI			0x05
#define BT481_CUR_YLOW			0x06
#define BT481_CUR_YHI			0x07

/*** INDIRECT ***/
#define BT482_RD_MSK				0x00
#define BT482_OVL_MSK 			0x01
#define BT482_CMD_REGB			0x02
#define BT482_CUR_REG			0x03
#define BT482_CUR_XLOW			0x04
#define BT482_CUR_XHI			0x05
#define BT482_CUR_YLOW			0x06
#define BT482_CUR_YHI			0x07
/* Bt482 FIELDS */
#define BT482_EXT_REG_M			0x01
#define BT482_EXT_REG_A			0x00
#define BT482_EXT_REG_EN		0x01
#define BT482_EXT_REG_DIS		0x00
#define BT482_CUR_SEL_M			0x20
#define BT482_CUR_SEL_A			0x05
#define BT482_CUR_SEL_INT		0x00
#define BT482_CUR_SEL_EXT		0x20
#define BT482_DISP_MODE_M		0x10
#define BT482_DISP_MODE_A		0x04
#define BT482_DISP_MODE_I		0x10
#define BT482_DISP_MODE_NI		0x00
#define BT482_CUR_CR3_M			0x08
#define BT482_CUR_CR3_A			0x03
#define BT482_CUR_CR3_RAM		0x08
#define BT482_CUR_CR3_PAL		0x00
#define BT482_CUR_EN_M			0x04
#define BT482_CUR_EN_A			0x02
#define BT482_CUR_EN_ON			0x00
#define BT482_CUR_EN_OFF		0x04
#define BT482_CUR_MODE_M		0x03
#define BT482_CUR_MODE_A		0x00
#define BT482_CUR_MODE_DIS		0x00
#define BT482_CUR_MODE_1		0x01
#define BT482_CUR_MODE_2		0x02
#define BT482_CUR_MODE_3		0x03
/* Bt482 ADRESSE OFFSET FOR EXT. REG. */
#define BT482_OFF_CUR_COL		0x11

/*****************************************************************************/

/* Bt485 FIELDS */

/* Command register 0 */
#define BT485_IND_REG3_M		0x80
#define BT485_IND_REG3_A		0x07
#define BT485_IND_REG3_EN		0x80
#define BT485_IND_REG3_DIS		0x00

/* Command register 2 */
#define BT485_DISP_MODE_M		0x08
#define BT485_DISP_MODE_A		0x03
#define BT485_DISP_MODE_I		0x08
#define BT485_DISP_MODE_NI		0x00
#define BT485_CUR_MODE_M		0x03
#define BT485_CUR_MODE_A		0x00
#define BT485_CUR_MODE_DIS		0x00
#define BT485_CUR_MODE_1		0x01
#define BT485_CUR_MODE_2		0x02
#define BT485_CUR_MODE_3		0x03

/* Command register 3 (indirecte) */
#define BT485_CUR_SEL_M			0x04
#define BT485_CUR_SEL_A			0x02
#define BT485_CUR_SEL_32		0x00
#define BT485_CUR_SEL_64		0x04
#define BT485_CUR_MSB_M			0x03
#define BT485_CUR_MSB_A			0x00
#define BT485_CUR_MSB_00		0x00
#define BT485_CUR_MSB_01		0x01
#define BT485_CUR_MSB_10		0x02
#define BT485_CUR_MSB_11		0x03

/* Bt485 ADR OFFSET FOR EXT. reg cmd3 */
#define BT485_OFF_CUR_COL		0x01
#define TVP3026_OFF_CUR_COL		0x01


#define BT484_ID_M            0xc0
#define BT484_ID              0x40
#define BT485_ID_M            0xc0
#define BT485_ID              0x80
#define ATT20C505_ID_M        0x70
#define ATT20C505_ID          0x50

/*** VIEWPOINT REGISTER INDIRECT ***/

#define VPOINT_CUR_XLOW       0x00
#define VPOINT_CUR_XHI        0x01
#define VPOINT_CUR_YLOW       0x02
#define VPOINT_CUR_YHI        0x03
#define VPOINT_SPRITE_X       0x04
#define VPOINT_SPRITE_Y       0x05
#define VPOINT_CUR_CTL        0x06
#define VPOINT_CUR_RAM_LSB    0x08
#define VPOINT_CUR_RAM_MSB    0x09
#define VPOINT_CUR_RAM_DATA   0x0a
#define VPOINT_WIN_XSTART_LSB 0x10
#define VPOINT_WIN_XSTART_MSB 0x11
#define VPOINT_WIN_XSTOP_LSB  0x12
#define VPOINT_WIN_XSTOP_MSB  0x13
#define VPOINT_WIN_YSTART_LSB 0x14
#define VPOINT_WIN_YSTART_MSB 0x15
#define VPOINT_WIN_YSTOP_LSB  0x16
#define VPOINT_WIN_YSTOP_MSB  0x17
#define VPOINT_MUX_CTL1       0x18
#define VPOINT_MUX_CTL2       0x19
#define VPOINT_INPUT_CLK      0x1a
#define VPOINT_OUTPUT_CLK     0x1b
#define VPOINT_PAL_PAGE       0x1c
#define VPOINT_GEN_CTL        0x1d
#define VPOINT_OVS_RED        0x20
#define VPOINT_OVS_GREEN      0x21
#define VPOINT_OVS_BLUE       0x22
#define VPOINT_CUR_COL0_RED   0x23
#define VPOINT_CUR_COL0_GREEN 0x24
#define VPOINT_CUR_COL0_BLUE  0x25
#define VPOINT_CUR_COL1_RED   0x26
#define VPOINT_CUR_COL1_GREEN 0x27
#define VPOINT_CUR_COL1_BLUE  0x28
#define VPOINT_AUX_CTL        0x29
#define VPOINT_GEN_IO_CTL     0x2a
#define VPOINT_GEN_IO_DATA    0x2b
#define VPOINT_KEY_RED_LOW    0x32
#define VPOINT_KEY_RED_HI     0x33
#define VPOINT_KEY_GREEN_LOW  0x34
#define VPOINT_KEY_GREEN_HI   0x35
#define VPOINT_KEY_BLUE_LOW   0x36
#define VPOINT_KEY_BLUE_HI    0x37
#define VPOINT_KEY_CTL        0x38
#define VPOINT_SENSE_TEST     0x3a
#define VPOINT_TEST_DATA      0x3b
#define VPOINT_CRC_LSB        0x3c
#define VPOINT_CRC_MSB        0x3d
#define VPOINT_CRC_CTL        0x3e
#define VPOINT_ID             0x3f
#define VPOINT_RESET          0xff

/*** TVP3026 REGISTER INDIRECT ***/

#define TVP3026_SILICON_REV    0x01
#define TVP3026_CURSOR_CTL     0x06
#define TVP3026_LATCH_CTL      0x0f
#define TVP3026_TRUE_COLOR_CTL 0x18
#define TVP3026_MUX_CTL        0x19
#define TVP3026_CLK_SEL        0x1a
#define TVP3026_PAL_PAGE       0x1c
#define TVP3026_GEN_CTL        0x1d
#define TVP3026_MISC_CTL       0x1e
#define TVP3026_GEN_IO_CTL     0x2a
#define TVP3026_GEN_IO_DATA    0x2b
#define TVP3026_PLL_ADDR       0x2c
#define TVP3026_PIX_CLK_DATA   0x2d
#define TVP3026_MEM_CLK_DATA   0x2e
#define TVP3026_LOAD_CLK_DATA  0x2f

#define TVP3026_KEY_RED_LOW    0x32
#define TVP3026_KEY_RED_HI     0x33
#define TVP3026_KEY_GREEN_LOW  0x34
#define TVP3026_KEY_GREEN_HI   0x35
#define TVP3026_KEY_BLUE_LOW   0x36
#define TVP3026_KEY_BLUE_HI    0x37
#define TVP3026_KEY_CTL        0x38
#define TVP3026_MCLK_CTL       0x39
#define TVP3026_SENSE_TEST     0x3a
#define TVP3026_TEST_DATA      0x3b
#define TVP3026_CRC_LSB        0x3c
#define TVP3026_CRC_MSB        0x3d
#define TVP3026_CRC_CTL        0x3e
#define TVP3026_ID             0x3f
#define TVP3026_RESET          0xff

/* structs used by video timing routines */

typedef struct
   {
   char         IdString[32];           /* "Matrox MGA Setup file" */
   short        Revision;               /* .inf file revision */

   short        BoardPtr[7];		/* offset of board wrt start */
                                            /* -1 = board not there */
   }header;

typedef struct
   {
   long         MapAddress;             /* board address */
   short        BitOperation8_16;       /* BIT8, BIT16, BITNARROW16 */
   char         DmaEnable;              /* 0 = enable ; 1 = disable */
   char         DmaChannel;             /* channel number. 0 = disabled */
   char         DmaType;                /* 0 = ISA, 1 = B, 2 = C */
   char         DmaXferWidth;           /* 0 = 16, 1 = 32 */
   char         MonitorName[64];        /* as in MONITORM.DAT file */
   short        MonitorSupport[8];      /* NA, NI, I */
   short        NumVidparm;             /* up to 24 vidparm structures */
   }general_info;

typedef struct
   {
   long         PixClock;
   short        HDisp;
   short        HFPorch;
   short        HSync;
   short        HBPorch;
   short        HOvscan;
   short        VDisp;
   short        VFPorch;
   short        VSync;
   short        VBPorch;
   short        VOvscan;
   short        OvscanEnable;
   short        InterlaceEnable;
   short        HsyncPol;                /* 0 : Negative   1 : Positive */
   short        VsyncPol;                /* 0 : Negative   1 : Positive */
   }Vidset;


typedef struct
   {
   short        Resolution;             /* RES640, RES800 ... RESPAL */
   short        PixWidth;               /* 8, 16, 32 */
   Vidset       VidsetPar[3];		/* for zoom X1, X2, X4 */
   }Vidparm;

typedef struct
{
    header hdr;
    general_info info;
    Vidparm parms[24];
} combined;

typedef struct
{
   char name[26];
   unsigned long valeur;
}vid;

/* Dac defs */

#define Info_Dac_M                  0x0000000f
#define Info_Dac_A                  0

#define Info_Dac_BT481              0x0 
#define Info_Dac_ViewPoint          0x1
#define Info_Dac_BT484              0x2
#define Info_Dac_Chameleon          0x3
#define Info_Dac_BT482              0x4
#define Info_Dac_BT485              0x6
#define Info_Dac_ATT                0x8
#define Info_Dac_Sierra             0xc
#define Info_Dac_TVP3026            0x9


/******* ProductType *******/
#define BOARD_MGA_RESERVED   0x07

#define TITAN_DST1_ABOVE1280_M     0x00000010
#define TITAN_DST1_NOMUXES_M       0x00020000
#define TITAN_DST0_BLKMODE_M       0x00080000

/* wait macros */

/* wait for n locations free in fifo */

#define MGA_WAIT(t, n) while(((t)->fifostatus & 0x3f) < (n))

/* wait for idle state */

#define WAIT_FOR_DONE(t) MGA_WAIT((t), 32); \
			 while((t)->status & TITAN_DWGENGSTS_BUSY)


#endif /* _mgadefs_h_ */
