/*
 * @(#) mgaCursor.c 11.1 97/10/22
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

/*
 * mgaCursor.c
 *
 * mga cursor routines
 */

#include "X.h"
#include "servermd.h"
#include "cursorstr.h"
#include "scrnintstr.h"

#ifdef usl
#include "mi/mi.h"
#include "mi/mipointer.h"
#include "mi/misprite.h"
#else
#include "../../mi/mipointer.h"
#include "../../mi/misprite.h"
#endif /* usl */

#include "mgaDefs.h"
#include "mgaScrStr.h"

/*
 * Array to help convert cursor data from source and mask bitmaps.
 * The bits in each byte need to be reversed if BITMAP_BIT_ORTDER == LSBFirst.
 */

#if (BITMAP_BIT_ORDER == LSBFirst)

static unsigned char revBits[256] =
{
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0, 
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4, 
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc, 
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa, 
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6, 
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1, 
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9, 
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd, 
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb, 
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7, 
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff 
};
#endif

/*
 * Realize the cursor for the mga.  Generate a cursor image which is:
 * an unsigned chars hotx and hoty,
 * followed by 256 unsigned chars of cursor data.
 * For overly large cursors we could choose to retain a portion
 * containing the hot spot, but for now we will just retain the upper-left
 * corner, and move the hot spot inside.
 */
Bool
mgaRealizeCursor(pScr, pCurs)
	ScreenPtr pScr;
	CursorPtr pCurs;
{
	mgaPrivatePtr pMga = MGA_PRIVATE_DATA(pScr);
	unsigned char *image, *psrc, *pmsk, *mimage;
	int w, wlast, h, stride;
	int hotx, hoty, th;
	register int i, j, srcbits, mskbits;
#ifdef DEBUG_PRINT
ErrorF("in mgaRealizeCursor\n");
#endif
	h = min(pCurs->bits->height, pMga->cursorHeight);
	w = min(pCurs->bits->width,  pMga->cursorWidth);

	hotx = min(pCurs->bits->xhot, (pMga->cursorHeight - 1));
	hoty = min(pCurs->bits->yhot,  (pMga->cursorWidth  - 1));

	if ((image = (unsigned char *)xalloc(258)) == NULL)
		return FALSE;

	pCurs->devPriv[pScr->myNum] = (pointer)image;

	psrc = (unsigned char *)pCurs->bits->source;
	pmsk = (unsigned char *)pCurs->bits->mask;
	stride = PixmapBytePad(pCurs->bits->width, 1);

	/* generate the sprite image */

	w = (w + 7) >> 3;

	*image++ = hotx;
	*image++ = hoty;

	mimage = image + 128;

	for(th = 0; th < h; ++th)
	{
		for (i = 0; i < w; ++i)
		{
#if (BITMAP_BIT_ORDER == LSBFirst)
		    *image++ = revBits[psrc[i]];
		    *mimage++ = revBits[pmsk[i]];
#else
		    *image++ = psrc[i];
		    *mimage++ = pmsk[i];
#endif
		}
	/* fill in remainder of width with transparent */
		for(; i < 4; ++i)
		{
		    *image++ = 0xff;
		    *mimage++ = 0;
		}
		psrc += stride;
		pmsk += stride;
	}
	/* fill in remainder of heitht with transparent rows */
	for(; th < 32 ; ++th)
	{
	    for(i = 0; i < 4; ++i)
	    {
		*image++ = 0xff;
		*mimage++ = 0;
	    }
	}
#ifdef DEBUG_PRINT
ErrorF("out of mgaRealizeCursor\n");
#endif

	return TRUE;
}

Bool
mgaUnrealizeCursor(pScr, pCurs)
	ScreenPtr pScr;
	CursorPtr pCurs;
{
#ifdef DEBUG_PRINT
ErrorF("in mgaUnrealizeCursor\n");
#endif
	xfree(pCurs->devPriv[pScr->myNum]);
	pCurs->devPriv[pScr->myNum] = NULL;
#ifdef DEBUG_PRINT
ErrorF("out of mgaUnrealizeCursor\n");
#endif
	return TRUE;
}


static void
mgaSetSpritePos(dac, dactype, x, y, hotx, hoty)
	register VOLATILE mgaDacsPtr dac;
	int dactype;
	int x, y;
	unsigned int hotx, hoty;
{
    unsigned long xp, yp;

#ifdef DEBUG_PRINT
ErrorF("in mgaSetSpritePos\n");
#endif
    /* calc the position */
    xp = x + (32 - hotx);	
    yp = y + (32 - hoty);

    /* set position based on dac type, at this point can't get a bad type */

    switch(dactype)
    {
	case Info_Dac_BT481:
	case Info_Dac_BT482:
	    {
		register mgaBt482Ptr dp = (mgaBt482Ptr) dac;
		dp->wadr_pal = BT482_CUR_XLOW;
		dp->pix_rd_msk = xp;
		dp->wadr_pal = BT482_CUR_XHI;
		dp->pix_rd_msk = xp >> 8;
		dp->wadr_pal = BT482_CUR_YLOW;
		dp->pix_rd_msk = yp;
		dp->wadr_pal = BT482_CUR_YHI;
		dp->pix_rd_msk = yp >> 8;
	    }
	    break;
	case Info_Dac_BT484:
	case Info_Dac_BT485:
	    {
		register mgaBt485Ptr dp = (mgaBt485Ptr) dac;
		dp->cur_xlow = xp;
		dp->cur_xhi = xp >> 8;
		dp->cur_ylow = yp;
		dp->cur_yhi = yp >> 8;
	    }
	    break;
	case Info_Dac_ViewPoint:
	    {
		register mgaViewPointPtr dp = (mgaViewPointPtr) dac;
		dp->index = VPOINT_CUR_XLOW;
		dp->data = xp;
		dp->index = VPOINT_CUR_XHI;
		dp->data = xp >> 8;
		dp->index = VPOINT_CUR_YLOW;
		dp->data = yp;
		dp->index = VPOINT_CUR_YHI;
		dp->data = yp >> 8;
	    }
	    break;
	case Info_Dac_TVP3026:
	    {
		register mgaTVP3026Ptr dp = (mgaTVP3026Ptr)dac;
		dp->cur_xlow = xp;
		dp->cur_xhi = xp >> 8;
		dp->cur_ylow = yp;
		dp->cur_yhi = yp >> 8;
	    }
	    break;
    }
#ifdef DEBUG_PRINT
ErrorF("out of mgaSetSpritePos\n");
#endif
}

static void
mgaSetSpriteColor(dac, dactype, fr, fg, fb, br, bg, bb)
	register VOLATILE mgaDacsPtr dac;
	int dactype;
	unsigned short fr, fg, fb, br, bg, bb;
{
    unsigned char temp;

    /* set cursor color as per dac type, can't have a bad one */

#ifdef DEBUG_PRINT
ErrorF("in mgaSetSpriteColor\n");
#endif
    switch(dactype)
    {
	case Info_Dac_BT481:
	case Info_Dac_BT482:
	    {
		register mgaBt482Ptr dp = (mgaBt482Ptr)dac;
		dp->wadr_pal = BT482_CUR_REG;
		temp = dp->pix_rd_msk;
		dp->pix_rd_msk = 0;
		dp->col_ovl = br >> 8;
		dp->col_ovl = bg >> 8;
		dp->col_ovl = bb >> 8;
		dp->col_ovl = fr >> 8;
		dp->col_ovl = fg >> 8;
		dp->col_ovl = fb >> 8;
		dp->wadr_pal = BT482_CUR_REG;
		dp->pix_rd_msk = temp;
	    }
	    break;
	case Info_Dac_BT484:
	case Info_Dac_BT485:
	    {
		register mgaBt485Ptr dp = (mgaBt485Ptr) dac;
		dp->wadr_ovl = BT485_OFF_CUR_COL;
		dp->col_ovl = br >> 8;
		dp->col_ovl = bg >> 8;
		dp->col_ovl = bb >> 8;
		dp->col_ovl = fr >> 8;
		dp->col_ovl = fg >> 8;
		dp->col_ovl = fb >> 8;
	    }
	    break;
	case Info_Dac_ViewPoint:
	    {
		register mgaViewPointPtr dp = (mgaViewPointPtr)dac;
		dp->index = VPOINT_CUR_COL0_RED;
		dp->data = br >> 8;
		dp->index = VPOINT_CUR_COL0_GREEN;
		dp->data = bg >> 8;
		dp->index = VPOINT_CUR_COL0_BLUE;
		dp->data = bb >> 8;
		dp->index = VPOINT_CUR_COL1_RED;
		dp->data = fr >> 8;
		dp->index = VPOINT_CUR_COL1_GREEN;
		dp->data = fg >> 8;
		dp->index = VPOINT_CUR_COL1_BLUE;
		dp->data = fb >> 8;
	    }
	    break;
	case Info_Dac_TVP3026:
	    {
		register mgaTVP3026Ptr dp = (mgaTVP3026Ptr)dac;
		dp->cur_col_addr = TVP3026_OFF_CUR_COL;
		dp->cur_col_data = br >> 8;
		dp->cur_col_data = bg >> 8;
		dp->cur_col_data = bb >> 8;
		dp->cur_col_data = fr >> 8;
		dp->cur_col_data = fg >> 8;
		dp->cur_col_data = fb >> 8;
	    }
	    break;
    }
#ifdef DEBUG_PRINT
ErrorF("out of mgaSetSpriteColor\n");
#endif
}

static void
mgaEnableSprite(dac, dactype, on)
register VOLATILE mgaDacsPtr dac;
int dactype, on;
{
    unsigned char temp;
    /* turn on/off cursor, can't have a bad dac type */
#ifdef DEBUG_PRINT
ErrorF("in mgaEnableSprite\n");
#endif

    switch(dactype)
    {
	case Info_Dac_BT481:
	case Info_Dac_BT482:
	    {
		register mgaBt482Ptr dp = (mgaBt482Ptr)dac;
		dp->wadr_pal = BT482_CUR_REG;
		temp = dp->pix_rd_msk & ~(BT482_CUR_MODE_M | BT482_CUR_EN_M);
		if(on)
		    dp->pix_rd_msk = temp | BT482_CUR_MODE_3;
		else
		    dp->pix_rd_msk = temp | BT482_CUR_MODE_DIS;
	    }
	    break;
	case Info_Dac_BT484:
	case Info_Dac_BT485:
	    {
		register mgaBt485Ptr dp = (mgaBt485Ptr)dac;
		temp = dp->cmd_reg2 & ~BT485_CUR_MODE_M;
		if(on)
		    dp->cmd_reg2 = temp | BT485_CUR_MODE_3;
		else
		    dp->cmd_reg2 = temp | BT485_CUR_MODE_DIS;
	    }
	    break;
	case Info_Dac_ViewPoint:
	    {
		register mgaViewPointPtr dp = (mgaViewPointPtr)dac;
		dp->index = VPOINT_CUR_CTL;
		if(on)
		    dp->data = 0x50;
		else
		    dp->data = 0x10;
	    }
	    break;
	case Info_Dac_TVP3026:
	    {
		register mgaTVP3026Ptr dp = (mgaTVP3026Ptr)dac;
		dp->index = TVP3026_CURSOR_CTL;
		temp = dp->data;
		if(on)
		    dp->data = temp | 0x03; /* X windows Cursor */
		else
		    dp->data = temp & 0xfc; /* Cursor Off */
	   }
	   break;
     }
#ifdef DEBUG_PRINT
ErrorF("out of mgaEnableSprite\n");
#endif
}

static void 
mgaLoadSprite(pMga, image)
	register mgaPrivatePtr pMga;
	unsigned char *image;
{
	register VOLATILE mgaDacsPtr dac = (mgaDacsPtr)&pMga->regs->ramdacs;
	int i;
	unsigned char temp,temp1,temp2,temp3,temp4,temp5;
#ifdef DEBUG_PRINT
       ErrorF("in mgaLoadSprite\n"); 
#endif
	if (image == NULL)
	{
		pMga->cursorHotx = 0;
		pMga->cursorHoty = 0;
	}
	else
	{
		pMga->cursorHotx = *image++;
		pMga->cursorHoty = *image++;
	}

	/* note unused area of sprite */

	/* disable sprite */

	mgaEnableSprite(dac, pMga->dactype, 0);

	if(image == NULL)
		return;		/* if no image, just leave the cursor off */

	/* else load the sprite */
	/* we can't have a bad dac type here */

	switch(pMga->dactype)
	{
	  case Info_Dac_BT481:
	  case Info_Dac_BT482:
	    {
		register mgaBt482Ptr dp = (mgaBt482Ptr)dac;
		dp->wadr_pal = BT482_CUR_REG;
		temp = dp->pix_rd_msk;
		dp->pix_rd_msk = temp | BT482_CUR_CR3_RAM;
		dp->wadr_pal = 0;

		for(i = 0; i < 256; ++i)
		    dp->col_ovl = *image++;

		dp->wadr_pal = BT482_CUR_REG;
		dp->pix_rd_msk = temp;
	    }
	    break;
	  case Info_Dac_BT484:
	  case Info_Dac_BT485:
	    {
		unsigned char reg0, reg3;
		register mgaBt485Ptr dp = (mgaBt485Ptr)dac;

		reg0 = dp->cmd_reg0;
		dp->cmd_reg0  = reg0 | BT485_IND_REG3_M;
		dp->wadr_pal = 1;
		reg3 = dp->cmd_reg3;
		reg3 &= ~(BT485_CUR_SEL_M | BT485_CUR_MSB_M);

		dp->wadr_pal = 1;
		dp->cmd_reg3 = reg3;
		dp->wadr_pal = 0;

		for(i = 0; i < 256; ++i)
		    dp->cur_ram = *image++;
		dp->cmd_reg0 = reg0;
	    }
	    break;
	  case Info_Dac_ViewPoint:
	    {
		register mgaViewPointPtr dp = (mgaViewPointPtr)dac;
		unsigned char pixels;

		dp->index = VPOINT_CUR_CTL;
		dp->data = 10;
		dp->index = VPOINT_CUR_RAM_LSB;
		dp->data = 0;
		dp->index = VPOINT_CUR_RAM_MSB;
		dp->data = 0;
		dp->index = VPOINT_CUR_RAM_DATA;

		for(i = 0; i < 256; ++i)
		{
		    pixels = *image++;
		    dp->data = (pixels << 6) | ((pixels << 2) & 0x30) |
			       ((pixels >> 2) & 0x0c) | ((pixels  >> 6) & 0x3);
		}
		dp->index = VPOINT_SPRITE_X;
		dp->data = 31;
		dp->index = VPOINT_SPRITE_Y;
		dp->data = 31;
	    }
	    break;
 	case Info_Dac_TVP3026:
	    {
		register mgaTVP3026Ptr dp = (mgaTVP3026Ptr)dac;
		/* Hide cursor */
		temp1 = dp->cur_xlow;
		temp2 = dp->cur_xhi;
		temp3 = dp->cur_ylow;
		temp4 = dp->cur_yhi;
		dp->cur_xlow = 0;
		dp->cur_xhi  = 0;
		dp->cur_ylow = 0;
		dp->cur_yhi  = 0;

		/* Transfer 1st 256 bytes */
		dp->index = TVP3026_CURSOR_CTL;
		temp5 = dp->data;
		temp5 = temp5 & 0xf0;
			dp->index = TVP3026_CURSOR_CTL;
		dp->data = temp5;  /* CCR[3:2] = 00 */
		dp->index = 0;   /* address RAM cursor to 0 */
		for (i = 0; i<128;i+=4)
		{
			dp->cur_ram = *image++;
			dp->cur_ram = *image++;
			dp->cur_ram = *image++;
			dp->cur_ram = *image++;
			dp->cur_ram = 0;
			dp->cur_ram = 0;
			dp->cur_ram = 0;
			dp->cur_ram = 0;
		}
		/* Transfer 2nd 256 bytes */
		temp5 = temp5 | 0x04;                /* CCR[3:2] = 01 */
		dp->index = TVP3026_CURSOR_CTL;
		dp->data = temp5;
		dp->index = 0;   /* address RAM cursor to 0 */
		for (i=0; i<256; i++)
			dp->cur_ram = 0;

		/* Transfer 3rd 256 bytes (Start of second PLAN)  */
		temp5 = temp5 & 0xf0;                
		temp5 = temp5 | 0x08;                /* CCR[3:2] = 10 */
		dp->index = TVP3026_CURSOR_CTL;
		dp->data = temp5;
		dp->index = 0;   /* address RAM cursor to 0 */
		for (i=128; i<256; i+=4)
		{
			dp->cur_ram = *image++;
			dp->cur_ram = *image++;
			dp->cur_ram = *image++;
			dp->cur_ram = *image++;
			dp->cur_ram = 0;
			dp->cur_ram = 0;
			dp->cur_ram = 0;
			dp->cur_ram = 0;
		}

		/* Transfer 4th 256 bytes */
		temp5 = temp5 & 0x0c;                /* CCR[3:2] = 11 */
		dp->index = TVP3026_CURSOR_CTL;
		dp->data = temp5;
		dp->index = 0;   /* address RAM cursor to 0 */
		for (i=0; i<256; i++)
			dp->cur_ram = 0;

	    }
	    break;
   }
	/* enable sprite */
	mgaEnableSprite(dac, pMga->dactype, 1);
#ifdef DEBUG_PRINT
       ErrorF("out of mgaLoadSprite\n"); 
#endif
}


/*
 * mgaSetCursor() - Install and position the given cursor.
 */
void 
mgaSetCursor(pScr, pCurs, x, y)
	register ScreenPtr pScr;
	register CursorPtr pCurs;
	int x, y;
{
	register mgaPrivatePtr pMga = MGA_PRIVATE_DATA(pScr);
#ifdef DEBUG_PRINT
ErrorF("in mgaSetCursor\n");
#endif
	if (pCurs == NULL) {
		/* just load an empty cursor */
		mgaLoadSprite(pMga, NULL);
		return;
	}

	/* load image */
	mgaLoadSprite(pMga, pCurs->devPriv[pScr->myNum]);

	/* set color */
	mgaSetSpriteColor(&pMga->regs->ramdacs, pMga->dactype,
		pCurs->foreRed, pCurs->foreGreen, pCurs->foreBlue,
		pCurs->backRed, pCurs->backGreen, pCurs->backBlue );

	/* set position */
	mgaSetSpritePos(&pMga->regs->ramdacs, pMga->dactype, x, y,
			pMga->cursorHotx, pMga->cursorHoty);
#ifdef DEBUG_PRINT
ErrorF("out of mgaSetCursor\n");
#endif
}

/*
 * mgaMoveCursor() - Move the cursor.
 */
Bool
mgaMoveCursor(pScr, x, y)
	ScreenPtr pScr;
	int x, y;
{
	register mgaPrivatePtr pMga = MGA_PRIVATE_DATA(pScr);
#ifdef DEBUG_PRINT
ErrorF("in mgaMoveCursor\n");
#endif
	mgaSetSpritePos(&pMga->regs->ramdacs, pMga->dactype, x, y,
			pMga->cursorHotx, pMga->cursorHoty);
#ifdef DEBUG_PRINT
ErrorF("out of mgaMoveCursor\n");
#endif
}

/*
 * mgaRecolorCursor() - set the foreground and background cursor colors.
 *			We do not have to re-realize the cursor.
 */
mgaRecolorCursor(pScr, pCurs, displayed)
	ScreenPtr pScr;
	register CursorPtr pCurs;
	Bool displayed;
{
	mgaPrivatePtr pMga = MGA_PRIVATE_DATA(pScr);

#ifdef DEBUG_PRINT
ErrorF("in mgaRecolorCursor\n");
#endif
	if (displayed)
		mgaSetSpriteColor(&pMga->regs->ramdacs, pMga->dactype,
			pCurs->foreRed, pCurs->foreGreen, pCurs->foreBlue,
			pCurs->backRed, pCurs->backGreen, pCurs->backBlue );
#ifdef DEBUG_PRINT
ErrorF("out of mgaRecolorCursor\n");
#endif
}

/*
 * mgaCursorOn() - turn the cursor on and off.
 */
void
mgaCursorOn(on, pScr)
	int on;
	ScreenPtr pScr;
{
	mgaPrivatePtr pMga = MGA_PRIVATE_DATA(pScr);
	
#ifdef DEBUG_PRINT
ErrorF("in mgaCursorOn\n");
#endif
mgaEnableSprite(&pMga->regs->ramdacs, pMga->dactype, on);
#ifdef DEBUG_PRINT
ErrorF("out of mgaCursorOn\n");
#endif

}

/*
 * mgaCursorInitialize() - Init the cursor and register movement routines.
 *			   We can do a better job than miRecolorCursor,
 *			   so replace it after pointer init.
 */

static miPointerSpriteFuncRec mgaPointerFuncs = {
	&mgaRealizeCursor,
	&mgaUnrealizeCursor,
	&mgaSetCursor,
	&mgaMoveCursor,
};

void
mgaCursorInitialize(pScr)
	register ScreenPtr pScr;
{
	mgaPrivatePtr pMga = MGA_PRIVATE_DATA(pScr);
#ifdef DEBUG_PRINT
ErrorF("in mgaCursorInitialize\n");
#endif
	if(scoPointerInitialize(pScr, &mgaPointerFuncs, TRUE) == 0) /* S001 */
		FatalError("mga: Cannot initialize cursor.\n");

	pScr->RecolorCursor = &mgaRecolorCursor;

	if(pMga->dactype == Info_Dac_ViewPoint)
	{
	    pMga->cursorHeight = 64;
	    pMga->cursorWidth = 64;
	}
	else
	{
	    pMga->cursorHeight = 32;
	    pMga->cursorWidth = 32;
	}
	mgaLoadSprite(pMga, NULL);
#ifdef DEBUG_PRINT
ErrorF("out of mgaCursorInitialize\n");
#endif
}

/*
 * mgaRestoreCursor() - restore cursor after screen switch.
 */
void
mgaRestoreCursor(pScr)
	register ScreenPtr pScr;
{
}
