#ident	"@(#)ihvkit:display/vga256/devices/gd54xx/font.c	1.2"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */
#include "Xmd.h"
#include "sidep.h"
#include "sys/types.h"
#include "sys/kd.h"
#include "vtio.h"
#include "v256.h"
#include "sys/inline.h"
#include "font.h"
#include <stdio.h>

#include "gd54xx_256.h"

extern SI_1_1_Functions v256SIFunctions;
extern SIFunctions oldfns;

#define FREE(p)	(p ? free(p) : 0)

typedef struct gd_font {
    int w;		/* width of glyphs in pixels */
    int h;		/* height of glyphs in pixels */
    int ascent;		/* distance from baseline to top of glyph */
	int descent;	/* distance from baseline to bottom of glyph*/
    int lsize;		/* no. of bytes stored for each line of a glyph */
    int size;		/* no. of bytes stored for each glyph */
} gd_font_rec;

gd_font_rec gd_fonts[FONT_COUNT];

SIBool gd54xx_font_check();
SIBool gd54xx_font_download();
SIBool gd54xx_font_stplblt();
SIBool gd54xx_font_free();

extern unsigned char inverted_byte[];
extern unsigned char gd54xx_rops[];


/*
 *  gd54xx_font_check(num, info)  : Check whether we can download a font
 *
 *    Input:
 *       int num	  : index of the font to be downloaded
 *       SIFontInfoP info : basic info about font
 */
SIBool
gd54xx_font_check(num, info)
int num;
SIFontInfoP info;
{
	/*
	 * Also check font downloadability by the software, because
	 * software may be used for some cases where the hardware function
	 * cannot be used.
	 */
	if ((*oldfns.si_font_check)(num,info) == SI_SUCCEED)
	{
    	if ( !(info->SFflag & SFTerminalFont) ||
			OUTSIDE(info->SFnumglyph, 0, FONT_NUMGLYPHS) ||
			OUTSIDE(info->SFmax.SFwidth, 1, FONT_MAXWIDTH) ||
			OUTSIDE(info->SFlascent + info->SFldescent, 1, FONT_MAXHEIGHT) ||
			OUTSIDE(num, 0, (FONT_COUNT - 1)))
		{
			return (SI_FAIL);
		}
    	return (SI_SUCCEED);
	}
	else
	{
		return (SI_FAIL);
	}
}


/*
 *  gd54xx_font_download(index, glyph_info_p, glyph_p)  
 *				- download the glyphs for a font into off-screen memory
 *
 *    Input:
 *        int index		   			: the index for the downloaded font
 *        SIFontInfoP glyph_info_p	: basic info about font
 *        SIGlyphP glyph_p		    : the glyphs themselves
 */
SIBool
gd54xx_font_download(index, glyph_info_p, glyph_p)
int index;
SIFontInfoP glyph_info_p;
SIGlyphP glyph_p;
{
	int i,j,flag;						/*temp.*/
    int width, height;				/*Max. width and max. height of glyphs in
									  the font to be downloaded*/
	int num_of_glyphs;				/*Number of glyphs in the font */
    int bitmap_pitch;
	int bytes_per_glyph_line;		/*Width rounded to nearest byte*/
	unsigned char *glyph_bitmap_row_p;
	unsigned char *glyph_bitmap_p;	/*Pointer to the glyph bitmap*/
	int font_off_screen_address;	/*Address from where the font glyphs will
									  be downloaded in the off-screen memory*/
	int glyph_off_screen_address;	/*Address where a particular glyph will
									  downloaded*/
	unsigned short glyph_data_half_word;
	unsigned short *off_screen_p;


	/*
	 *Also do the sofware download for the cases where we have to call
	 *the software to do the font stplblt.
	 */
	if ((*oldfns.si_font_download)(index,glyph_info_p,glyph_p) == SI_FAIL)
	{
		return (SI_FAIL);
	}

    gd_fonts[index].w = width = glyph_info_p->SFmax.SFwidth;
    gd_fonts[index].h = height = glyph_info_p->SFmax.SFascent + 
							   		glyph_info_p->SFmax.SFdescent;
    gd_fonts[index].ascent = glyph_info_p->SFmax.SFascent;
    gd_fonts[index].descent = glyph_info_p->SFmax.SFdescent;
    num_of_glyphs = glyph_info_p->SFnumglyph;

    /* 
	 *Convert each glyph bitmap to a blit-ready image:
     */

    bytes_per_glyph_line = ((width + 7) & ~7) >> 3;		/* bytes in each line 
														   (rounded down) */
    gd_fonts[index].lsize = bytes_per_glyph_line;		/* size of each glyph's
														   scanline */
    gd_fonts[index].size = bytes_per_glyph_line * height;	/* size of each 
														   resulting glyph */
	/*
	 *According to the index number the fonts will be downloaded to
	 *the off-screen memory.Each of the 8 fonts have been assigned
	 *8K slots in the off-screen memory.Out of this 8K each glyph
	 *has been assigned 32 bytes.Thus supporting a maximum of 256
	 *glyphs per font.The max. width and height of the gyphs is 16x16
	 */

	/*
	 *Program the BLIT ENGINE to write 2xBheight bytes per glyph,since
	 *we know that max. width of the glyph will always be 16.The remaining
	 *bytes of the 32 bytes of the glyph will not be written.This also
	 *makes BLIT operation easier because the BLIT ENGINE must transfer
	 *16 bits at a time.
	 */

	off_screen_p = (unsigned short *)v256_fb;
	font_off_screen_address = FONT_DB_START + index*FONT_MEMORY_SIZE;

	j = 0;

	do
	{
		register int write_more = 0;
		register int num_of_bytes = bytes_per_glyph_line;
		register int bytes_read = 0;
		register int total_bytes;

		i = glyph_p->SFglyph.Bheight;
		total_bytes = i*bytes_per_glyph_line;
		U_HEIGHT(0);
		U_DEST_ADDR(font_off_screen_address + j*FONT_MAXSIZE);
	
		if ((((total_bytes + 1)&~1) >> 1) & 0x01)
		{
			write_more = 1;
			U_WIDTH(((total_bytes + 1) & ~1) + 1);
			U_DEST_PITCH(((total_bytes + 1) & ~1) + 2);
		}
		else
		{
			U_WIDTH(((total_bytes + 1) & ~1) - 1);
			U_DEST_PITCH((total_bytes + 1) & ~1);
		}

		/*Here mode is BitBlt from system source to display copy*/
		U_BLTMODE(4);
   		U_ROP(0x0d);			/*source copy*/
		BLT_START(2);

		glyph_bitmap_row_p = (unsigned char *)(glyph_p->SFglyph.Bptr);

		do
		{
			register int num_of_bytes = bytes_per_glyph_line;
			glyph_bitmap_p = glyph_bitmap_row_p;

			do
			{
				if (bytes_read == 1)
				{
					glyph_data_half_word =
						(glyph_data_half_word & 0x00ff) |
						(((unsigned short)(inverted_byte[*glyph_bitmap_p]))
						 << 8);	
					*off_screen_p = glyph_data_half_word;
					glyph_bitmap_p ++;
					bytes_read = 0;
				}
				else
				{
					glyph_data_half_word = 
						(unsigned short)(inverted_byte[*glyph_bitmap_p]);
					*glyph_bitmap_p ++;
					bytes_read = 1;
				}
			}
			while ( --num_of_bytes > 0);	

			glyph_bitmap_row_p +=4;			/*Since we know that the max.
											  bitmap width for a font of
											  16x16 will be 4 bytes*/
		}
		while ( --i > 0);
		
		/*
		 *For the case where we have odd number of bytes to transfer,
		 *the last byte ,expanded to 16 bits is transfered.This happens
		 *only for the case when we have to transfer 1 byte per glyphline
		 *and total line are odd.
		 */	
		if (total_bytes & 1)
		{
			*off_screen_p = 0x00ff & glyph_data_half_word;
		}

		/*
		 *Total number of bytes in the blit operation should be
		 *a multiple of 4 for BLIT to come to an end.
		 */

		if (write_more)
		{
			*off_screen_p = 0x0000;
		}

		WAIT_FOR_ENGINE_IDLE();

		glyph_p ++;j ++;
	}
	while ( --num_of_glyphs > 0);
			
    return(SI_SUCCEED);
}

/*
 *  gd54xx_font_stplblt(index,x_start,y_start,glyph_count,glyphs_p,forcetype)
 *                : stipple glyphs in a downloaded font.
 *
 *    Input:
 *       int index				: font index to stipple from
 *       int x_start,y_start	: position to stipple to (at baseline of font)
 *       int glyph_count		: number of glyphs to stipple
 *       SIint16 *glyphs_p 		: list of glyph indices to stipple
 *       int forcetype  		: Opaque or regular stipple (if non-zero)
 */
SIBool
gd54xx_font_stplblt(index, x_start, y_start, glyph_count, glyphs_p, forcetype)
SIint32 index;
SIint32 x_start, y_start;
SIint32 glyph_count, forcetype;
SIint16 *glyphs_p;
{
	int flag;
	int is_opaque;							/*stippling-opaque or transparent*/
	unsigned char rop;						/*raster operation*/
	int clipped_x,clipped_y;				/*(x,y)from where the glyphs should
											  be written*/
	int width,height;						/*For local use*/
	int clipped_width,clipped_height;		/*For local use*/
	int hidden_width = 0,no_of_hidden_glyphs;	/*Used for x-clipping*/
	int hidden_height = 0;
	int hidden_start,left_over_end;
	int left_over_start;
	int destination_address;
	int source_font_start;
	int source_glyph_address;
	int bytes_per_glyph_line;

	unsigned char saved_extended_mode; 		/* saved parameters */
	unsigned char saved_mode; 				/* saved parameters */
	unsigned char saved_blt_mode; 			/* saved parameters */
	unsigned char saved_set_reset;			/* saved parameters */
	unsigned char saved_set_reset_enable;	/* saved parameters */
	unsigned char graphics_engine_mode;		/* saved parameters */

	/*
	 * We do font stippling only if the fill-style is SGFillSolid
	 * and rops are GXcopy GXand and GXor.
	 */
	if (v256_gs->fill_mode != SGFillSolidFG)
	{
		FALLBACK(si_font_stplblt,
			(index,x_start,y_start,glyph_count,glyphs_p,forcetype));
	}

	if ((v256_gs->mode != GXcopy) && (v256_gs->mode != GXand) && 
		(v256_gs->mode != GXor))
	{
		FALLBACK(si_font_stplblt,
			(index,x_start,y_start,glyph_count,glyphs_p,forcetype));
    }


#if (defined(__DEBUG__))
	if (gd54xx_font_stplblt_debug)
	{
		printf("\n(gd54xx_font_stplblt)");
		printf("\n\t x_origin  %d",x_start);
		printf("\n\t y_origin  %d",y_start);
		printf("\n\t count     %d",glyph_count);
		printf("\n\t forcetype %d",forcetype);
	}
#endif
	/*
	 *Initialize variables for local use.
	 */
    clipped_height = height = gd_fonts[index].h;
    clipped_width = width = gd_fonts[index].w;
	bytes_per_glyph_line = gd_fonts[index].lsize;
	
	/*
	 *Initial values,before clipping.
	 */
    clipped_y = y_start - gd_fonts[index].ascent;
    clipped_x = x_start;

	/*
	 *Sanity checks before going ahead.
	 */
	if ((clipped_x > v256_clip_x2) ||
		(clipped_y > v256_clip_y2) ||
    	((clipped_x + glyph_count*width - 1) < v256_clip_x1) ||
		((y_start + gd_fonts[index].descent -1) < v256_clip_y1))
	{
		return (SI_SUCCEED);
	}

	/*
	 *Opaque stippling or transparent
	 */
     if (forcetype == SGOPQStipple || 
	     (forcetype == 0 && v256_gs->stp_mode == SGOPQStipple))
     {
	  	is_opaque = 1;
     }	
     else
     {
	  	is_opaque = 0;
	 }

    /*
     *Check for y-clipping : these clipping adjustments affect every
     *glyph blit.
     */
    if ((clipped_y + clipped_height - 1) > v256_clip_y2) 
	{
		clipped_height = v256_clip_y2 - clipped_y + 1;
    }
    if (clipped_y < v256_clip_y1) 
	{
		hidden_height = v256_clip_y1 - clipped_y;
		clipped_height -= hidden_height;
		clipped_y = v256_clip_y1;
    }

    if (clipped_height <= 0)
	{
		return (SI_SUCCEED);
	}
	
    /*
     *Check for x-clipping : these affect start glyph & glyph_count, and set up
     *variables for partial-width blits.
     */
    if (clipped_x < v256_clip_x1) 
	{
		hidden_width = v256_clip_x1 - clipped_x;
		no_of_hidden_glyphs = hidden_width /width;		/*no. of glyphs TOTALLY
														  clipped */
		glyph_count -= no_of_hidden_glyphs;		  		/*skip all totally
														  clipped glyphs */
		hidden_start = 
			hidden_width - no_of_hidden_glyphs*width;   /* and this much of 1st
														   glyph */
		glyphs_p += no_of_hidden_glyphs;
		clipped_x = v256_clip_x1;
		clipped_width = width - hidden_start;			/*The visible width*/
		x_start += no_of_hidden_glyphs*width;
    } 
	else 
	{
		hidden_start = 0;
	}

	/*
	 *We cannot handle the cases where there is a hidden start.
	 *Such cases can be given to the software.
	 */
	if (hidden_start)
	{
		(*oldfns.si_font_stplblt)(index,x_start,y_start,1,glyphs_p,forcetype);
		glyphs_p ++;
		glyph_count --;
		clipped_x += clipped_width;
	}

	/*
	 *Hardware is required only if we have more glyphs.
	 */
	if (glyph_count <= 0)
	{
		return (SI_SUCCEED);
	}
	if (clipped_x > v256_clip_x2)
	{
		return (SI_SUCCEED);
	}

    if ((clipped_x + glyph_count*width - 1) > v256_clip_x2) 
	{
		hidden_width = clipped_x + glyph_count*width - 1 - v256_clip_x2;
		no_of_hidden_glyphs = 
			(hidden_width + width - 1) / width;		/* # of glyphs clipped */
		glyph_count -= no_of_hidden_glyphs;
		left_over_end = 
			no_of_hidden_glyphs*width - hidden_width;	/*pixel width left*/
		left_over_start = clipped_x + glyph_count*width;
    }
	else 
	{
		left_over_end = 0;
	}

    if (glyph_count <= 0 )
	{
		if (left_over_end)
		{
			if ((*oldfns.si_font_stplblt)
				(index,left_over_start,y_start,1,glyphs_p,forcetype) == SI_FAIL)
			{
				return (SI_FAIL);
			}
		}
		return (SI_SUCCEED);
    }


	if (glyph_count <= 0)
	{
		return (SI_SUCCEED);
	}

	/*
	 *Destination on the screen.
	 */
	destination_address = clipped_x + (clipped_y * v256_slbytes);

	/*
	 *Save the states of the registers before being programmed
	 *for the BLIT operation
	 */
	outb(0x3ce, 0x0b);
	saved_extended_mode = inb(0x3cf);

	outb(0x3ce, 0x05);
	saved_mode = inb(0x3cf);

	outb(0x3ce, 0x30);
	saved_blt_mode = inb(0x3cf);

	/*
	 * read the graphics engine mode.
	 */
	outb(0x3CE, 0x05);
	graphics_engine_mode = inb(0x3CF);

	/*
	 * Update the foreground and the background color registers.
	 * (register GR0 for bg & GR1 for fg)
	 */

	/* save the values first */
	outb(0x3CE, 0x00);
	saved_set_reset = inb(0x3CF);
	outb(0x3CE, 0x01);
	saved_set_reset_enable = inb(0x3CF);

	/*
 	 *Program the BLIT ENGINE for stippling.
	 *We do bliting of stippled glyph data
 	 *in off-screen memory to the destination on screen
 	 */
	/*
	 * Enable extended write modes
	 */
	ENABLE_EX_WRITE_MODES(0x04);


	if (!is_opaque)
	{
		U_WRITE_MODE((graphics_engine_mode & ~7) | 0x4); /* write mode 4 */

		U_BLTMODE(0x88);		/* direction increment, enable color
								   expand, enable transparency, source
								   video memory,destination video memory */
		/*
 		 *For transparent stippling the register GR0 should be
		 *programmed to the foreground.
		 */
		U_BLT_FG_LOW(v256_gs->fg);   

		/*
 		 * Update the BLT Transparent Color Registers
 		 * The color set should not be the same as foreground color.
 		 */
		U_BLT_BG_LOW(~v256_gs->fg);
		U_BLT_TRANS_COLOR_LOW(~v256_gs->fg);
		U_BLT_TRANS_COLOR_HIGH(~v256_gs->fg);


		/*
 		 * Update the BLT Transparent Color Mask Registers
 		 * The mask value should be the same as in 
 		 */
		U_BLT_TRANS_COLOR_MASK_LOW(0);
		U_BLT_TRANS_COLOR_MASK_HIGH(0);


	}
	else /* opaque stippling */
	{
		U_WRITE_MODE((graphics_engine_mode & ~7) | 0x5); /* write mode 5 */

		U_BLTMODE(0x80);		/* direction increment, enable color
								   expand, dont enable transparency, source
								   video memory, destination video memory */
		/*
		 *For opaque stippling the register GR0 and GR1 should be
		 *set to background and foreground colors.
		 */
		U_BLT_FG_LOW(v256_gs->fg);
		U_BLT_BG_LOW(v256_gs->bg);

	}

	/*
	 *Raster operation.
	 */
	rop = gd54xx_rops[v256_gs->mode];
	U_ROP(rop);

	source_font_start = FONT_DB_START + index*FONT_MEMORY_SIZE;

		do
		{
			/*
			 * y-clipped cases can be handled.
			 */
			source_glyph_address = source_font_start +
				(*glyphs_p)*32 + hidden_height*bytes_per_glyph_line;
	
			U_WIDTH(width - 1);
			U_HEIGHT(clipped_height - 1);
			U_SRC_ADDR(source_glyph_address);
			U_DEST_ADDR(destination_address);
			U_DEST_PITCH(v256_slbytes);
			
			BLT_START(2);
	
			WAIT_FOR_ENGINE_IDLE();

			destination_address += width;
			glyphs_p ++;
		}
		while ( --glyph_count > 0);

	/*
	 * Restore the register states
	 */
	ENABLE_EX_WRITE_MODES(saved_extended_mode);
	U_WRITE_MODE(graphics_engine_mode);
	U_BLTMODE(0x00);
	U_BLT_FG_LOW(saved_set_reset_enable);
	U_BLT_BG_LOW(saved_set_reset);

	/*
	 *We cannot take care of all the leftover cases
	 */
	if (left_over_end)
	{
		if ((*oldfns.si_font_stplblt)
			(index,left_over_start,y_start,1,glyphs_p,forcetype) == SI_FAIL)
		{
			return (SI_FAIL);
		}
	}
				
	return (SI_SUCCEED);
}

/*
 *    gd54xx_font_free(num) : Free data structures associated with a
 *                downloaded font.
 *
 *    Input:
 *       int num : index of font
 */
SIBool
gd54xx_font_free(num)
SIint32 num;
{
	/*
	 * Free the fonts downloaded by the software ,also.
	 */
	if ((*oldfns.si_font_free)(num) == SI_FAIL)
	{
		return (SI_FAIL);
	}

    return(SI_SUCCEED);
}

