
/*
 * @(#) mgaScreen.c 11.1 97/10/22
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
 *      SCO Modifications
 *
 *	S006	Thu Sep  4 13:55:41 PDT 1997	hiramc@sco.COM
 *	- no need for the ioctl on SW_VGA12 in gemini
 *      S005    30 Oct 95   kylec@sco.com
 *		Add multihead support.
 *	S004	Thu Jun  1 16:56:14 PDT 1995	brianm@sco.com
 *		Merged in new code from Matrox.
 *      S003    Fri Nov  4 16:08:07 PST 1994    brianm@sco.com
 *              Moved the memory sizing to a routine which only
 *              actually counts the memory once.  I suyspect counting
 *              memory caused a problem with getting confused about how much
 *              memory was actually available.
 *      S002    Wed Jun 15 08:31:49 PDT 1994    hiramc@sco.COM
 *              Remove code that was saving the state of the VGA adapter.
 *              Rework order of talking to the registers, may have
 *              eliminated the problem of hanging the system all the time.
 *      S001    Wed May 25 16:42:33 PDT 1994    hiramc@sco.COM
 *              Try using ioctl to tell console driver we are entering
 *              graphics mode.  Will need to come back here and
 *              take out code that is trying to save the state of the
 *              VGA adapter.
 */

/*
 * mgaScreen.c
 *
 * Template for machine dependent screen procedures
 */
#include <sys/types.h>

#ifdef usl
#include <sys/kd.h>
#else
#include <sys/console.h>
#endif /* usl */

#include "X.h" 
#include "Xproto.h"
#include "misc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "regionstr.h"
#include "ddxScreen.h"
#include "scoext.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbGlyph.h"
#include "nfb/nfbProcs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "mgaScrStr.h"

#define WINDOWSIZE (7 * 1024)
#define VGASIZE (19 * WINDOWSIZE)
#define MAXMEM (5 * 1024 * 1024)  /* don't look passed 5 meg */
#define MINMEM (2 * 1024 * 1024)  /* must be at least 2 meg */

#ifdef DELETE_ME
typedef struct
{
    unsigned char r, b, g, x;
} dacVals;

vid *vidtab;                            /* default video table */
static dacVals mgaDac[256];             /* place to hold graphics color table */
static unsigned char mgaDacValid = FALSE;       /* is mgaDac[] valid to use ? */
static Bool firstTime = TRUE;
#endif /* DELETE_ME */

/* table to help set up color table for true color visuals */
static unsigned char mgaLut[256] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
	30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
	50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
	60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
	70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
	90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
	100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
	110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
	120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
	130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
	140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
	150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
	160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
	170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
	180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
	190, 191, 192, 193, 194, 195, 196, 197, 198, 199,
	200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
	210, 211, 212, 213, 214, 215, 216, 217, 218, 219,
	220, 221, 222, 223, 224, 225, 226, 227, 228, 229,
	230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247, 248, 249,
	250, 251, 252, 253, 254, 255
};

/* fuction to set the clipping rectangle */
/* note the linearized y values */

void mgaSetClipRegions(BoxRec *pbox, int nbox, DrawablePtr pDraw)
{
    mgaPrivatePtr mgaPriv = MGA_PRIVATE_DATA(pDraw->pScreen);
#ifdef DEBUG_PRINT
ErrorF("in mgaSetClipRegions\n");
#endif
    if(nbox)
    {
	mgaPriv->clipXL = pbox->x1;
	mgaPriv->clipXR = pbox->x2 - 1;
	mgaPriv->clipYT = (pbox->y1 * mgaPriv->pstride) + mgaPriv->ydstorg;
	mgaPriv->clipYB = ((pbox->y2 - 1) * mgaPriv->pstride) +
			  mgaPriv->ydstorg;
    }
    else
    {
	mgaPriv->clipXL = 0;
	mgaPriv->clipXR = mgaPriv->width - 1;
	mgaPriv->clipYT = 0;
	mgaPriv->clipYB = ((mgaPriv->height - 1) * mgaPriv->pstride) +
			  mgaPriv->ydstorg;
    }

#ifdef DEBUG_PRINT
ErrorF("out of mgaSetClipRegions\n");
#endif
}

/*
 * mgaBlankScreen - blank or unblank the screen
 *      on - blank the screen if true, unblank the screen if false
 *      pScreen - which screen to blank
 */
Bool
mgaBlankScreen(on, pScreen)
	int on;
	ScreenPtr pScreen;
{
	mgaPrivatePtr mgaPriv = MGA_PRIVATE_DATA(pScreen);
	VOLATILE register mgaTitanRegsPtr titan = &mgaPriv->regs->titan;
	unsigned char tbyte;

#ifdef DEBUG_PRINT
	ErrorF("mgaBlankScreen(on=%d)\n", on);
#endif /* DEBUG_PRINT */

	titan->vga.seq_addr = 1;
	tbyte = titan->vga.seq_data;

	if(on)
	    tbyte |= 0x20;
	else
	    tbyte &= 0xdf;

	titan->vga.seq_data = tbyte;

	return(TRUE);
}

/*
 * mgaSetText(pScreen) - set screen into text mode
 */
void
mgaSetText(pScreen)
	ScreenPtr pScreen;
{
    register int i;
    grafData *grafinfo = DDX_GRAFINFO(pScreen);
    mgaPrivatePtr mgaPriv = MGA_PRIVATE_DATA(pScreen);
    VOLATILE register mgaTitanRegsPtr titan = &mgaPriv->regs->titan;
    VOLATILE mgaDacsPtr dac = &mgaPriv->regs->ramdacs;
	VOLATILE unsigned long *p = (unsigned long *)mgaPriv->fbBase;
    unsigned long *sp;
    int count;
    unsigned char tmpByte;

    /*  make sure not in vga mode, not really necessary */
#ifdef DEBUG_PRINT
ErrorF("in mgaSetText\n");
#endif

    titan->config &= ~0x400;

    mgaBlankScreen(1, pScreen); /* */
    /* restore screen contents */

    WAIT_FOR_DONE(titan);       /* just in case, wait for done */

    titan->drawSetup.maccess = 0;                       /* memory access */
    titan->drawSetup.plnwt = 0xffffffff;                /* plane mask */

    /* save graphics color table in case we switch back */

    dac->bt485.radr_pal = 0;

	/* reset  the titan (atlas) chip */

  titan->vga.misc_outw = 0x67;

  if (mgaPriv->dactype == Info_Dac_TVP3026)
  {
   /* reset extended crtc start  address */
   titan->vga.aux_addr = 0x0a;
   titan->vga.aux_data = 0x80;
    

   /* Remove all synch */
   titan->vga.crtc_addr6 = 0x11;
   titan->vga.crtc_data6 = 0x40;
   
   titan->vga.crtc_addr6 = 0x01;
   titan->vga.crtc_data6 = 0x00;
   titan->vga.crtc_addr6 = 0x00;
   titan->vga.crtc_data6 = 0x00;
   titan->vga.crtc_addr6 = 0x06;
   titan->vga.crtc_data6 = 0x00;

/*------ reset exterm hsync and vsync polarity (no inversion ------*/
      dac->tvp3026.index = TVP3026_GEN_CTL;
      tmpByte = dac->tvp3026.data;
      /* Put hsync = negative, vsync = negative */
      tmpByte = tmpByte & 0xfc;
      dac->tvp3026.index = TVP3026_GEN_CTL;
      dac->tvp3026.data = tmpByte;
    }


    /* reset the dac to vga mode */
    switch(mgaPriv->dactype)
    {
	case Info_Dac_BT482:
	    dac->bt482.cmd_rega = 1;
	    dac->bt482.wadr_pal = BT482_CMD_REGB;
	    dac->bt482.pix_rd_msk = 0;
	    dac->bt482.wadr_pal = BT482_CUR_REG;
	    dac->bt482.pix_rd_msk = 0x10;
	    dac->bt482.cmd_rega = 0;
	    break;
	case Info_Dac_BT485:
	    dac->bt485.cmd_reg0 = 0x80;
	    dac->bt485.wadr_pal = 0xff;
	    dac->bt485.cmd_reg1 = 0;
	    dac->bt485.cmd_reg2 = 0;
	    dac->bt485.cmd_reg3 = 0;
	    dac->bt485.cmd_reg0 = 0;
	    break;
	case Info_Dac_ViewPoint:
	    dac->vpoint.index = VPOINT_RESET;
	    dac->vpoint.data = 0;
	    dac->vpoint.index = VPOINT_GEN_CTL;
	    dac->vpoint.data = 0;
	case Info_Dac_TVP3026:
	    dac->tvp3026.index = TVP3026_GEN_IO_CTL;
	    dac->tvp3026.data = 0x01;
	    dac->tvp3026.index = TVP3026_GEN_IO_DATA;
	    dac->tvp3026.data = 0x01;
	    dac->tvp3026.index = TVP3026_MISC_CTL;
	    dac->tvp3026.data = 0x04;
	    dac->tvp3026.index = TVP3026_GEN_CTL;
	    dac->tvp3026.data = 00;
	    dac->tvp3026.index = TVP3026_CLK_SEL;
	    dac->tvp3026.data = 00;
	    dac->tvp3026.index = TVP3026_TRUE_COLOR_CTL;
	    dac->tvp3026.data = 0x80;
	    dac->tvp3026.index = TVP3026_MUX_CTL;
	    dac->tvp3026.data = 0x98;
	    dac->tvp3026.index = TVP3026_MCLK_CTL;
	    dac->tvp3026.data = 0x18;

	    break;
    }

    titan->rst = TITAN_SOFTRESET_SET;
    mgaDelay(2);
    titan->rst = TITAN_SOFTRESET_CLR;
    mgaDelay(2);
    
   /* reset the crtc gen */

  if (mgaPriv->dactype != Info_Dac_TVP3026)
   {
    titan->vga.seq_addr = 0;
    titan->vga.seq_data = 3;
    mgaDelay(2);
   }
    /* if vlb bus, and isa bit was set, set isa bit in config */

    if(mgaPriv->isvlb && mgaPriv->isabit)
    {
	titan->test = 0x8d000000L;  /* enable writing to isa bit */
	titan->config |= 0x10000000; /* set isa bit */
	mgaDelay(1);
	titan->test = 0;
    }

    /* enable vga mode */
    if (mgaPriv->vgaEnabled)
        titan->config |= 0x400;

    /* if vlb bus, unmap. NOTE! THIS IS UNDOCUMENTED ANYWHERE but matrox code */
    if(mgaPriv->isvlb)
    {
	if(mgaPriv->regs == mgaPriv->mapBase)
	  *((unsigned long *)(&mgaPriv->fbBase[0x2010])) = 0x80000000 | 0xac000;
	outb(0x46e8, 8);
    }

    /* this should just call int10(3) */
    if (mgaPriv->vgaEnabled)
        grafExec(grafinfo, "SetText", NULL);

#ifdef DEBUG_PRINT
ErrorF("out of mgaSetText\n");
#endif
}

/*
 * mgaSetGraphics(pScreen) - set screen into graphics mode
 */
void
mgaSetGraphics(pScreen)
	ScreenPtr pScreen;
{
   nfbScrnPriv *devPriv = NFB_SCREEN_PRIV(pScreen);
	register int count;
    unsigned long blkMode, TramDword, InfoHardware;
    int i, isatlas, athena;
    grafData *grafinfo = DDX_GRAFINFO(pScreen);
    mgaPrivatePtr mgaPriv = MGA_PRIVATE_DATA(pScreen);
    VOLATILE register mgaTitanRegsPtr titan = &mgaPriv->regs->titan;
    mgaDacsPtr dac = &mgaPriv->regs->ramdacs;
    unsigned long *sp, dst0, dst1, dacinfo;
	register unsigned long tword;
	register unsigned char tbyte;
#ifdef DEBUG_PRINT
ErrorF("in mgaSetGraphics\n");	
#endif
   /* if vlb bus, map it */ 
    if(mgaPriv->isvlb)
    {
      outb(0x46e8, 0);  /* allow writing to upper mem area then map regs */
      if(mgaPriv->regs == mgaPriv->mapBase)
	*((unsigned long *)(&mgaPriv->fbBase[0x2010])) = 0xac000;

	/* if isa bit set, clear isa bit in config */
      if(mgaPriv->isabit = titan->config & 0x10000000)
      {
	titan->test = 0x8d000000L;  /* enable writing to isa bit */
	titan->config &= ~0x10000000; /* clear isa bit */
	mgaDelay(1);
	titan->test = 0;
      }
    }

    mgaBlankScreen( 1, pScreen);    /*      1 == blank screen       */

    if (titan->config & 0x400)
    {
        /*  Signal the console driver we will be in graphics mode */
        mgaPriv->vgaEnabled = 1;
#if ! defined(usl)			/*	S006	*/
        ioctl(1, SW_VGA12, 0);
#endif
        grafExec(grafinfo, "SetGraphics", NULL);
    }
    else
        mgaPriv->vgaEnabled = 0;

    mgaPriv->regs->dubic.ndx_ptr = 0x8; /* shut off dubic ints */
    mgaPriv->regs->dubic.dub_sel = 0;

    titan->ien = 0;
    titan->iclear = 0xf;

    isatlas = titan->rev & 0x7f; 
    athena = isatlas==0x2;
    
    /* figure out ydstorg if > 2 meg of memory needed for this res */
    /* first get pixel location of bank switch (2m / bytes ber pixel) */
    /* then round down to end of previous line */
    /* then subtract to get remainder of line */

    if(isatlas && ((mgaPriv->height * mgaPriv->bstride) > 2097152L))
    {
	int pt, l, offset;
	pt = 2097152L / mgaPriv->bpp;   /* get pixel loc of bank change */
	l = pt / mgaPriv->pstride;      /* get number of lines before change */
	mgaPriv->ydstorg = (pt - (mgaPriv->pstride * l)); /* get remainder */
    }
    

    /* reset the board */

    titan->config &= ~0x400;
    titan->vga.seq_addr = 0;
    titan->vga.seq_data = 1;
    mgaDelay(1);
    titan->vga.misc_outw = 0x2f;
    titan->rst = TITAN_SOFTRESET_SET;
    mgaDelay(2);
    titan->rst = TITAN_SOFTRESET_CLR;
    mgaDelay(2);

      /*** program the opmode fields ***/

    GetMGAConfiguration(mgaPriv->regs, &dst0, &dst1, &dacinfo);
    
    tword = TITAN_NOWAIT_M;

			    /***        15.625*40.0/64.0 DO 8.8       ***/
    tword |= (((((0x0fa0*0x2800)/0x4000)>>8) << TITAN_RFHCNT_A) &
	      TITAN_RFHCNT_M); /*** FIXED ***/

    /* if(isatlas) /* only use fbm 2 'till further notice */
	tword |= ((2 << TITAN_FBM_A) & TITAN_FBM_M);
#if 0
    else
	tword |= ((6 << TITAN_FBM_A) & TITAN_FBM_M);
#endif

    tword |= ((((dst1 & TITAN_DST1_HYPERPG_M) >>
		TITAN_DST1_HYPERPG_A) << TITAN_HYPERPG_A) & TITAN_HYPERPG_M);

      /*** ATTENTION bit inverse par rapport a DST1 ***/
    tword |= ((((~dst1 & TITAN_DST1_TRAM_M) >> TITAN_DST1_TRAM_A) <<
		TITAN_TRAM_A) & TITAN_TRAM_M);

    titan->opmode = tword;
  
    tword = titan->config;

    /*** clear fields to be modified in config ***/

    tword &= ~TITAN_EXPDEV_M;

      /*** program the fields ***/

    tword |= ((((dst1 & TITAN_DST1_EXPDEV_M) >> TITAN_DST1_EXPDEV_A) <<
	       TITAN_EXPDEV_A) & TITAN_EXPDEV_M);
      
    titan->config = tword;

/*      get mga configruation stuff */

//    GetMGAConfiguration(mgaPriv->regs, &dst0, &dst1, &dacinfo);
    mgaPriv->dactype = dacinfo;         /* save dac type info */

    if (athena)
{

/*----------------------------------------------------------
* blkModeSupported
*
*  Check if board support 8-bit block mode.
*     Condition: i) chip set ATHENA
*               ii) IMP+ PCB rev 2 or more
*              iii) 
*
* Return:
*   1   : 8-bit block mode supported
*   0   : 8-bit block mode not supported
* 
*----------------------------------------------------------*/

   /* Test IMP+ with PCB rev < 2, we read TramDword to find IMP+ /p4 */
   TramDword = titan->opmode;
   TramDword &= TITAN_TRAM_M;

   /* PCB rev < 2 and not a /p4 */
   if((( titan->rev & 0xf) < 2) && !TramDword)
      blkMode = 0;
   else
      blkMode = 1;

#ifdef DEBUG_PRINT
ErrorF("blkMode = %d\n",blkMode);	
#endif
    /*------ Strap added for ATHENA ------------------------------*/
    if ( blkMode )
        {
        InfoHardware = titan->opmode;

        if(dst0 & TITAN_DST0_BLKMODE_M)
           InfoHardware = InfoHardware & 0xf7ffffff;  /* bit 27=0: VRAM 4 bit block mode */
        else
            InfoHardware = InfoHardware | 0x08000000;  /* bit 27=1: VRAM 8 bit block mode */
        titan->opmode = InfoHardware;
        }

    /*------ Strap added in ATHENA For max clock dac support ----*/

    InfoHardware = titan->config;

    if(!((dst1 & TITAN_DST1_200MHZ_M) >> TITAN_DST1_200MHZ_A))
       InfoHardware = InfoHardware & 0xfffffffb;  /* bit 2=0: Board supports regular (135MHz/170MHz) operation */
    else
       InfoHardware = InfoHardware | 0x00000004;  /* bit 2=1: Board supports 200MHz operation */

       titan->config = InfoHardware;
}

    if(isatlas)
    {
	titan->config |= TITAN_NODUBIC_M;
	tbyte = titan->vga.misc_outr;
	
 if (dacinfo != Info_Dac_TVP3026)
	programme_reg_icd(mgaPriv->regs, 3, 0x065AC3D);

	titan->vga.misc_outw = tbyte;
	mgaDelay(10);
	tword = *(unsigned long *)mgaPriv->regs->dstwin.window;
	if(tword == 0x0518102b)
	{
	    mgaPriv->regs->dstwin.window[0x40] = 1; /* ??????????? */
	    mgaPriv->regs->dstwin.window[4] |= 2; /* ??????????? */
	}
    }
    else
    {
	titan->config &= ~TITAN_NODUBIC_M;
	tbyte = titan->vga.misc_outr;
	programme_reg_icd(mgaPriv->regs, 3, 0x0063AC44);
	titan->vga.misc_outw = tbyte;
	mgaDelay(10);
    }
    /* load the graphics display parameters */

    MGAVidInit(mgaPriv->regs, mgaPriv->ydstorg, mgaPriv->vidtab);
    WAIT_FOR_DONE(titan);       /* just in case, wait for done */


    titan->drawSetup.maccess = 0;                       /* memory access */
    titan->drawSetup.plnwt = 0xffffffff;                /* plane mask */

    count = mgaCountMem( mgaPriv );     /* S003 Only count memory once. */

    if(count == MAXMEM)         /* found too much mem ? */
    {
	ErrorF("MGA Warning: Too much memory found\n");
	count = MINMEM;         /* set to known minamum */
    }

   mgaPriv->offscreenSize = count - ((mgaPriv->bstride * mgaPriv->height) +
				      (mgaPriv->ydstorg * mgaPriv->bpp) +
				      (10 * WINDOWSIZE));       /* font space */

    /* there is not enough memory if offscreensize < byte stride */
    if(mgaPriv->offscreenSize < mgaPriv->bstride)
    {
	mgaSetText(pScreen);    /* put us back in text mode */
	mgaBlankScreen(0, pScreen); /* unblank the screen */
	return;
    }

    /* we can support two loaded fonts of sizes of up to 32x32 */
    /* compute the font location offsets in bytes */

    mgaPriv->font[0].loc = (count - (10 * WINDOWSIZE)); /* font location 0 */
    mgaPriv->font[1].loc = (count - (5 * WINDOWSIZE));  /* font location 1 */

    /* program the dacs */
    mgaPriv->regs->ramdacs.bt484.wadr_pal = 0;

    if(mgaPriv->depth == 8)     /* setup color table saved by mgaSetText */
    {
        int i;  /*      a 3:3:2 (RGB) LUT       */
        for(i = 0; i < 256; ++i)
        {
            mgaPriv->regs->ramdacs.bt484.col_pal =
                mgaLut[(((i>>5)&0x7)*0x2466)>>8];
            mgaPriv->regs->ramdacs.bt484.col_pal =
                mgaLut[(((i>>2)&0x7)*0x2466)>>8];
            mgaPriv->regs->ramdacs.bt484.col_pal =
                mgaLut[(i&0x3)*85];
        }
    }
    else if(mgaPriv->depth == 16)       /* setup 15 bits of color */
    {
	int i;          /*      a 5:5:5 (RGB) LUT contigously indexed   */
  if (mgaPriv->dactype != Info_Dac_TVP3026)
	for(i = 0; i < 256; ++i)
	{
	      mgaPriv->regs->ramdacs.bt484.col_pal =
						mgaLut[((i&0x1f)*0x0833)>>8];
	      mgaPriv->regs->ramdacs.bt484.col_pal =
						mgaLut[((i&0x1f)*0x0833)>>8];
	      mgaPriv->regs->ramdacs.bt484.col_pal =
						mgaLut[((i&0x1f)*0x0833)>>8];
	}
   else
        for(i = 0; i < 256; ++i)   
	{
	      mgaPriv->regs->ramdacs.bt484.col_pal = mgaLut[i];
	      mgaPriv->regs->ramdacs.bt484.col_pal = mgaLut[i];
	      mgaPriv->regs->ramdacs.bt484.col_pal = mgaLut[i];
	}
    }
    else
    {
	int i;
	for(i = 0; i < 256; ++i)                /* setup 24 bits of color */
	{
	      mgaPriv->regs->ramdacs.bt484.col_pal = mgaLut[i];
	      mgaPriv->regs->ramdacs.bt484.col_pal = mgaLut[i];
	      mgaPriv->regs->ramdacs.bt484.col_pal = mgaLut[i];
	}
    }

/* let the server do this after painting the background */
/*    mgaBlankScreen(0, pScreen); /* unblank the screen */

  /* the registers loaded here should not be loaded elswhere */

    titan->drawSetup.maccess    = mgaPriv->depth >> 4;
    titan->drawSetup.pitch      = mgaPriv->pstride;
    titan->drawSetup.ydstorg    = mgaPriv->ydstorg;

   (*devPriv->LoadColormap)(devPriv->installedCmap);

#ifdef DEBUG_PRINT
ErrorF("out of mgaSetGraphics\n");	
#endif
}

/*
 * mgaSaveGState(pScreen) - save graphics info before screen switch
 */
void
mgaSaveGState(pScreen)
	ScreenPtr pScreen;
{
#ifdef DEBUG_PRINT
	ErrorF("mgaSaveGState()\n");
#endif /* DEBUG_PRINT */
}

/*
 * mgaRestoreGState(pScreen) - restore graphics info from mgaSaveGState()
 */
void
mgaRestoreGState(pScreen)
	ScreenPtr pScreen;
{
#ifdef DEBUG_PRINT
	ErrorF("mgaRestoreGState()\n");
#endif /* DEBUG_PRINT */
}


/*
 *      S003
 *      Count the memory on the card only once.  I think is responsible
 *      for the memory sizing not working.  It would get called multiple times
 *      usually on a screen switch.
 */

static int
mgaCountMem( mgaPriv )
mgaPrivatePtr mgaPriv;
{
    static int count = -1;
    register unsigned long saved;
    VOLATILE register mgaTitanRegsPtr titan = &mgaPriv->regs->titan;
    VOLATILE unsigned long *p = (unsigned long *)mgaPriv->fbBase;

    if (count != -1)
	return count;

/*      Beware.  The Microsoft compiler turned this for loop into:
 *      mov esi,0x55555555
 *      mov esi,0xAAAAAAAA
 *      effectively a NoOp,
 *      until the register declarations were set so that *p wasn't
 *      simply a register, and Microsoft doesn't honor volatile
 *      properly.
 */
    /* size memory, try each loc twice */

    for(count = 0; count < MAXMEM;)     /* MAXMEM is upper bound */
    {
	int i;
	titan->srcpage = count;

	i = 2;
	saved = *p;
	do
	{
		*p = 0x55555555;
	} while((*p != 0x55555555) && i--);

	if(*p != 0x55555555)
	{
	    *p = saved;
	    break;
	}

	i = 2;
	do
	{
		*p = 0xAAAAAAAA;
	} while((*p != 0xAAAAAAAA) && i--);

	if(*p != 0xAAAAAAAA)
	{
	    *p = saved;
	    break;
	}
	*p = saved;
	count += (1024 * 64);
    }
    return count;
}
