/*
 *  @(#) wd33Font.c 11.1 97/10/22
 *
 * Copyright (C) 1991-1993 The Santa Cruz Operation, Inc.
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
 *   wd33Font.c      Draw terminal emulator font
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Tue 17-Aug-1993	edb@sco.com
 *              New for wd33 driver
 *      S001    Thu 18-Aug-1993 edb@sco.com
 *              Changed memory allocation
 *      S002    Tue 31-Aug-1993 edb@sco.com
 *		Fix screen switch bug - fonts did not get reloaded
 */

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "miscstruct.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "scrnintstr.h"
#include "regionstr.h"
#include "windowstr.h"


#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbRop.h"	
#include "nfb/nfbGlyph.h"

#include "wd33Defs.h"
#include "wd33ScrStr.h"
#include "wd33Procs.h"
#include "wdBitswap.h"

static glMemSeg * LoadFont( wdScrnPrivPtr wdPriv, nfbFontPSPtr pPS ,int font_id);

extern int nextFontId;

#define FONT_GLYPH_ID -1

#ifndef agaII

/*
 * wd33DrawFontText8() - Draw Terminal emulator fonts
 *
 *
 *      If this font is not loaded yet
 *      we load all glyphs of this font at once
 */

void
wd33DrawFontText8(
        BoxPtr pbox,
        unsigned char *chars,
        unsigned int count,
        nfbFontPSPtr pPS,
        unsigned long fg,
        unsigned long bg,
        unsigned char alu,
        unsigned long planemask,
        unsigned char transparent,
        DrawablePtr pDraw)
{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
        int curBlock = wdPriv->curRegBlock;
	int i;
        int index;
	glMemSeg *memseg;
	glMemSeg *memseg_first;
        int font_id;
        BoxRec box = *pbox;
        int width,height;
        int x1,y1;

        font_id = pPS->private.val;
        width   = pPS->pFontInfo->width;
        height  = pbox->y2 - pbox->y1;
        x1      = pbox->x1;
        y1      = pbox->y1;

#ifdef DEBUG_PRINT
   ErrorF("DrawFontText8( font_id=%d,count=%d,width=%d,height=%d,fg=%x,bg=%x,alu=%x,pmask=%x\n",
                              font_id,count,width,height,fg,bg,alu,planemask);
#endif
        if( font_id <=0 ) font_id = nextFontId + 1;                                /* S002 vvv*/

        index = GL_HASH( font_id, FONT_GLYPH_ID );
        memseg = wdPriv->glHashTab[index];
        while( memseg != NULL) {
           if(memseg->fontId == font_id && memseg->glyphId == FONT_GLYPH_ID ) break;
             memseg = memseg->next;
        }

        if( memseg == NULL )
        {
            if( (memseg = LoadFont( wdPriv,pPS,font_id )) == NULL)
            {
                  genDrawFontText(pbox, chars, count, pPS,
                        fg, bg, alu, planemask, transparent, pDraw);
                  return;
            }
#ifdef DEBUG_PRINT
            ErrorF("               font_id %d loaded\n", font_id);
#endif
        }
        if( pPS->private.val <= 0) 
              pPS->private.val = ++nextFontId;                                     /* S002 ^^^*/

        memseg_first = memseg ;

        WAITFOR_BUFF( 8 );
        WRITE_2_REG( ENG_2, FOREGR_IND_0    , fg );
        WRITE_2_REG( ENG_2, BACKGR_IND_0    , bg );
        WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , planemask);
        WRITE_2_REG( ENG_2, TRANSPMASK_IND_0, 1 << memseg->planeNr );
        WAITFOR_BUFF( 8 );
        WRITE_2_REG( ENG_2, TRANSPCOL_IND_0 , wdPriv->planeMaskMask );
        WRITE_REG(   ENG_1, DIM_X_IND     , width  - 1 );
        WRITE_REG(   ENG_1, DIM_Y_IND     , height - 1 );
        WRITE_REG(   ENG_1, RASTEROP_IND  , alu << 8 );
        WRITE_REG(   ENG_1, CNTRL_2_IND , wdPriv->bit11_10 | BIT_6_5 |
                                     (transparent ? MONO_TRANSP_ENAB : 0) );

	for (i = 0; i < count; ++i, chars++)
	{
            memseg = memseg_first + *chars;

            WAITFOR_BUFF( 6 );
            WRITE_REG( ENG_1, SOURCE_X      , memseg->xCache );
            WRITE_REG( ENG_1, SOURCE_Y      , memseg->yCache );
            WRITE_REG( ENG_1, DEST_X        , x1 );
            WRITE_REG( ENG_1, DEST_Y        , y1 );
            WRITE_REG( ENG_1, CNTRL_1_IND , BITBLIT | SRC_IS_MONO_COMP );
            x1 += width;
	}
        wdPriv->curRegBlock = curBlock;
}

/*
 *   Allocate memory in cache and load all glyphs in font
 */

static glMemSeg *
LoadFont(
        wdScrnPrivPtr wdPriv,
        nfbFontPSPtr pPS,
        int font_id)
{
        nfbFontInfoPtr nfbFontInfo = pPS->pFontInfo;
        int width,height,count,stride;
        int index;
        int i;
        unsigned char **imagePtr = nfbFontInfo->ppBits;
        glPlane  *plane;
        glMemSeg *memseg, *memseg_first;

        if( !nfbFontInfo->isTE8Font )
                 return( NULL );
        
        width  = nfbFontInfo->width;
        height = nfbFontInfo->height;
        count  = nfbFontInfo->count;
        stride = nfbFontInfo->stride;

        if( (memseg = wd33AllocGlyphMem( wdPriv, width,height ,count )) == NULL)
             return NULL;

        /*
         *  We assume a contiguous list of memsegs
         */
        memseg_first = memseg;

        for( i=0; i<count; i++, memseg++ )
        {
              memseg->fontId  = font_id;
              memseg->glyphId = FONT_GLYPH_ID;
  
              wd33LoadGlyph( wdPriv, width, height, imagePtr[i], stride,
                      memseg->xCache, memseg->yCache, memseg->planeNr);
        }

        index = GL_HASH( font_id, FONT_GLYPH_ID );
        memseg--;
        memseg->next    = wdPriv->glHashTab[index] ;
        wdPriv->glHashTab[index] = memseg_first;

        return( memseg_first );
}

#endif  /* agaII */
