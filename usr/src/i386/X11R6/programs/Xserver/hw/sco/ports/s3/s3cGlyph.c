/*
 *	@(#)s3cGlyph.c	6.1	3/20/96	10:23:15
 *
 * 	Copyright (C) Xware, 1991-1992.
 *
 * 	The information in this file is provided for the exclusive use
 *	of the licensees of Xware. Such users have the right to use, 
 *	modify, and incorporate this code into other products for 
 *	purposes authorized by the license agreement provided they 
 *	include this notice and the associated copyright notice with 
 *	any such product.
 *
 *	Copyright (C) The Santa Cruz Operation, 1993
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *	The information in this file is provided for the exclusive use
 *	of the licensees of SCO. Such users have the right to use, 
 *	modify, and incorporate this code into other products for 
 *	purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such
 *	product.
 *
 * Modification History
 *
 * S011, 17-May-93, staceyc
 * 	multiheaded S3 card support
 * S010, 11-May-93, staceyc
 * 	include file clean up, replace calls to assembler code with calls
 * 	to C code
 * S009	Wed Nov 18 10:11:23 PST 1992	hiramc@sco.COM
 *	There was a comma instead of semi-colon to end a statement.
 *	Must have been a typo error, reset it to ;
 *	And remove all mod comments prior to 1992.
 * S008	Thu Aug 27 10:14:51 PDT 1992	hiramc@sco.COM
 *	Include misc.h before font includes to get pointer defined
 * X007 24-Jul-92 hiramc@sco.com
 *	Don't draw zero width or height fonts
 * X006 01-Jan-92 kevin@xware.com
 *      updated copyright notice.
 */

#include "s3cConsts.h"
#include "s3cMacros.h"
#include "s3cDefs.h"
#include "s3cProcs.h"

/*
 *  s3cSetupStretchGlyphBlit() -- Setup Stretch Glyph Blit
 *
 *	This routine setup the 86c911 for stretch glyph blit.
 *
 *	Note: This routine is coded in assembly in s3cGBlit.s
 */

static void
s3cSetupStretchGlyphBlit(
	int		rop,
	int		fg,
	int		writeplanemask)
{
	S3C_CLEAR_QUEUE(5);
	S3C_PLNWENBL(writeplanemask);
	S3C_SETMODE(S3C_M_CPYRCT);
	S3C_SETFN0(S3C_FNCOLOR0, S3C_FNNOP);
	S3C_SETFN1(S3C_FNCOLOR1, rop);
	S3C_SETCOL1(fg);
}


/*
 *  s3cStretchGlyphBlit() -- Stretch Glyph Blit
 *
 *	This routine will stretch blit a glyph using the 86c911 blit engine.
 */

static void
s3cStretchGlyphBlit(
	int 		x0,
	int 		y0,
	int 		x1,
	int 		y1,
	int 		lx,
	int 		ly,
	int 		readplanemask,
	int		command)
{
	S3C_CLEAR_QUEUE(8);
	S3C_PLNRENBL(readplanemask);
	S3C_SETX0(x0);
	S3C_SETY0(y0);
	S3C_SETX1(x1);
	S3C_SETY1(y1);
	S3C_SETLX(lx);
	S3C_SETLY(ly);
	S3C_COMMAND(command);
}

/*
 *  s3cDrawMonoGlyphs() -- Draw Mono Glyphs
 *
 *	This routine will draw a list glyphs on the display in either the 
 *	4 or 8 bit display modes.
 *
 */

void
S3CNAME(DrawMonoGlyphs)(
	nfbGlyphInfo 			*glyph_info,
	unsigned int 			nglyphs,
	unsigned long 			fg,
	unsigned char 			alu,
	unsigned long 			planemask,
	unsigned long 			font_id,
	DrawablePtr 			pDrawable)
{
	int				ng;
	int				lx;
	int				ly;
	int				i;
	int				hash_table_size;
	int				bucket_size;
	BoxPtr 				pbox;
	s3cGlGlyphInfo_t 		glyph_entry;
	BoxRec 				off_screen_box;
	ScreenPtr 			pScreen = pDrawable->pScreen;
	s3cFontData_t 			*s3cFontData;
	s3cGlHashHead_t 		*bucket;
	s3cGlHashHead_t			**hash_table;
	s3cGlCachePosition_t 		*cache_positions;
	unsigned int			glyph_id;
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pDrawable->pScreen);

	s3cFontData = &S3C_PRIVATE_DATA(pScreen)->font_data;
	S3C_VALIDATE_SCREEN(s3cPriv, pDrawable->pScreen,
	    pDrawable->pScreen->myNum);

	hash_table_size = s3cFontData->hash_table_size;
	hash_table = s3cFontData->hash_table;

	s3cSetupStretchGlyphBlit(S3CNAME(RasterOps)[alu], fg, planemask);

	for ( ng = 0; ng < nglyphs; ++ng, ++glyph_info )
	{
		pbox = &glyph_info->box;

		lx = pbox->x2 - pbox->x1;
		if( 0 == lx ) continue;	/* don't do zero widths  X007 */
		ly = pbox->y2 - pbox->y1;
		if( 0 == ly ) continue;	/* don't do zero heights  X007 */

		glyph_id = glyph_info->glyph_id;	/*	S009	*/
			
		bucket = 
		    hash_table[S3C_GL_HASH(font_id, glyph_id, hash_table_size)];
		cache_positions = bucket->cache_positions;
		bucket_size = bucket->bucket_size;

		/*
		 * hopefully short linear search through bucket for glyph
		 */

		i = 0;
		while ( i < bucket_size
		    	&& !( cache_positions->exists 
				&& cache_positions->byteOffset == glyph_id 
				&& cache_positions->font_id == font_id )) 
		{
			cache_positions++;
			i++;
		}

		/*
		 * check to see if glyph was already cached
		 */
		 
		if ( i < bucket_size )
		{
			/*
			 * blit the cached glyph to the screen
			 */

			s3cStretchGlyphBlit(
				cache_positions->coords.x, 
				cache_positions->coords.y,
				pbox->x1, pbox->y1, lx - 1, ly - 1,
		    		cache_positions->plane, 
				S3C_BLIT_XP_YP_Y);
		}
		else
		{
			/*
			 * if not, see if we can cache this glyph
			 */

			if (S3CNAME(GlAddCacheEntry)(pScreen, font_id, glyph_id, 
				lx, ly, &glyph_entry) )
			{
				/*
				 * cache position has been allocated, 
				 * now draw glyph to the off-screen
				 * cache area
				 */

				off_screen_box.x1 = glyph_entry.coords.x;
				off_screen_box.y1 = glyph_entry.coords.y;
				off_screen_box.x2 = off_screen_box.x1 + lx;
				off_screen_box.y2 = off_screen_box.y1 + ly;

				S3CNAME(DrawOpaqueMonoImage)(&off_screen_box, 
					glyph_info->image, 0, 
					glyph_info->stride, 
					~0, 0, GXcopy, 
					glyph_entry.plane, pDrawable);

				/*
				 * blit the newly cached glyph to the screen
				 */
				 
				s3cSetupStretchGlyphBlit(S3CNAME(RasterOps)[alu], 
					fg, planemask);

				s3cStretchGlyphBlit(
					glyph_entry.coords.x, 
					glyph_entry.coords.y,
					pbox->x1, pbox->y1, lx - 1, ly - 1,
		  		  	glyph_entry.plane, 
					S3C_BLIT_XP_YP_Y);
			}
			else
			{
				/*
				 * unable to allocate a cache position, 
				 * use draw mono image to write to screen, 
				 * hopefully this will not happen too often!
				 */

				S3CNAME(DrawMonoImage)(&glyph_info->box, 
					glyph_info->image, 0, 
					glyph_info->stride, 
					fg, alu, planemask, pDrawable);

				s3cSetupStretchGlyphBlit(S3CNAME(RasterOps)[alu], 
					fg, planemask);
			}
		}
	}		
}


/*
 *  s3cDrawMonoGlyphs16() -- Draw Mono Glyphs 16
 *
 *	This routine will draw a list glyphs on the display in the 16 bit
 *	display modes.
 *
 */

void
S3CNAME(DrawMonoGlyphs16)(
	nfbGlyphInfo 			*glyph_info,
	unsigned int 			nglyphs,
	unsigned long 			fg,
	unsigned char 			alu,
	unsigned long 			planemask,
	unsigned long 			font_id,
	DrawablePtr 			pDrawable)
{
	int				ng;
	int				lx;
	int				ly;
	int				i;
	int				hash_table_size;
	int				bucket_size;
	BoxPtr 				pbox;
	s3cGlGlyphInfo_t 		glyph_entry;
	BoxRec 				off_screen_box;
	ScreenPtr 			pScreen = pDrawable->pScreen;
	s3cFontData_t 			*s3cFontData;
	s3cGlHashHead_t 		*bucket;
	s3cGlHashHead_t			**hash_table;
	s3cGlCachePosition_t 		*cache_positions;
	unsigned int			glyph_id;
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pDrawable->pScreen);

	s3cFontData = &S3C_PRIVATE_DATA(pScreen)->font_data;
	S3C_VALIDATE_SCREEN(s3cPriv, pDrawable->pScreen,
	    pDrawable->pScreen->myNum);

	hash_table_size = s3cFontData->hash_table_size;
	hash_table = s3cFontData->hash_table;

	for ( ng = 0; ng < nglyphs; ++ng, ++glyph_info )
	{
		pbox = &glyph_info->box;

		lx = pbox->x2 - pbox->x1;
		if( 0 == lx ) continue;	/* don't do zero widths  X007 */
		ly = pbox->y2 - pbox->y1;
		if( 0 == ly ) continue;	/* don't do zero heights  X007 */

		glyph_id = glyph_info->glyph_id;	/*	S009	*/
			
		bucket = 
		    hash_table[S3C_GL_HASH(font_id, glyph_id, hash_table_size)];
		cache_positions = bucket->cache_positions;
		bucket_size = bucket->bucket_size;

		/*
		 * hopefully short linear search through bucket for glyph
		 */

		i = 0;
		while ( i < bucket_size
		    	&& !( cache_positions->exists 
				&& cache_positions->byteOffset == glyph_id 
				&& cache_positions->font_id == font_id )) 
		{
			cache_positions++;
			i++;
		}

		/*
		 * check to see if glyph was already cached
		 */
		 
		if ( i < bucket_size )
		{
			/*
			 * blit the cached glyph to the screen
			 */

			s3cSetupStretchGlyphBlit(S3CNAME(RasterOps)[alu], 
				fg, planemask);

			s3cStretchGlyphBlit(
				cache_positions->coords.x, 
				cache_positions->coords.y,
				pbox->x1, pbox->y1, lx - 1, ly - 1,
		    		cache_positions->plane, 
				S3C_BLIT_XP_YP_Y);

			s3cSetupStretchGlyphBlit(S3CNAME(RasterOps)[alu], 
				fg >> 8, planemask >> 8);

			s3cStretchGlyphBlit(
				cache_positions->coords.x, 
				cache_positions->coords.y,
				pbox->x1+1024, pbox->y1, lx - 1, ly - 1,
		    		cache_positions->plane, 
				S3C_BLIT_XP_YP_Y);
		}
		else
		{
			/*
			 * if not, see if we can cache this glyph
			 */

			if (S3CNAME(GlAddCacheEntry)(pScreen, font_id, glyph_id, 
				lx, ly, &glyph_entry) )
			{
				/*
				 * cache position has been allocated, 
				 * now draw glyph to the off-screen
				 * cache area
				 */

				off_screen_box.x1 = glyph_entry.coords.x;
				off_screen_box.y1 = glyph_entry.coords.y;
				off_screen_box.x2 = off_screen_box.x1 + lx;
				off_screen_box.y2 = off_screen_box.y1 + ly;

				S3CNAME(DrawOpaqueMonoImage16)(&off_screen_box, 
					glyph_info->image, 0, 
					glyph_info->stride, 
					~0, 0, GXcopy, 
					glyph_entry.plane, pDrawable);

				/*
				 * blit the newly cached glyph to the screen
				 */
				 
				s3cSetupStretchGlyphBlit(S3CNAME(RasterOps)[alu], 
					fg, planemask);

				s3cStretchGlyphBlit(
					glyph_entry.coords.x, 
					glyph_entry.coords.y,
					pbox->x1, pbox->y1, lx - 1, ly - 1,
		  		  	glyph_entry.plane, 
					S3C_BLIT_XP_YP_Y);

				s3cSetupStretchGlyphBlit(S3CNAME(RasterOps)[alu], 
					fg >> 8, planemask >> 8);

				s3cStretchGlyphBlit(
					glyph_entry.coords.x, 
					glyph_entry.coords.y,
					pbox->x1+1024, pbox->y1, lx - 1, ly - 1,
		  		  	glyph_entry.plane, 
					S3C_BLIT_XP_YP_Y);
			}
			else
			{
				/*
				 * unable to allocate a cache position, 
				 * use draw mono image to write to screen, 
				 * hopefully this will not happen too often!
				 */

				S3CNAME(DrawMonoImage16)(&glyph_info->box, 
					glyph_info->image, 0, 
					glyph_info->stride, 
					fg, alu, planemask, pDrawable);
			}
		}
	}		
}
