#ident	"@(#)ihvkit:display/vga256/devices/gd54xx/hwfuncs.c	1.2"

/*
 *	Copyright (c) 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

#define TSSBITMAP 1			/* so KDENABIO works... */
#define	VPIX	  1			/* so KIOCINFO works... */

#ifndef VGA_PAGE_SIZE 
#define VGA_PAGE_SIZE 	(64 * 1024)
#endif

#include "Xmd.h"
#include "sidep.h"
#include "miscstruct.h"
#include "sys/types.h"
#include <fcntl.h>
#include "sys/kd.h"
#include <stdio.h>
#include "vtio.h"
#include "v256.h"
#include "v256spreq.h"
#include "sys/inline.h"
#include "vgaregs.h"
#include "newfill.h"

#include <sys/param.h>
#include <errno.h>

#include "gd54xx_256.h"

SIBool gd54xx_tile_poly_fillrect();
SIBool gd54xx_solid_poly_fillrect();
SIBool gd54xx_stipple_poly_fillrect();
SIBool gd54xx_big_tile_poly_fillrect();

extern SI_1_1_Functions v256SIFunctions;
extern SIFunctions oldfns;

const unsigned char inverted_byte[256] = {
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,
    0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
    0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
    0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,
    0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
    0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
    0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,
    0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
    0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1,
    0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9,
    0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
    0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED,
    0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
    0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
    0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7,
    0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF,
    0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

/* The order: see X.h for more info
 *
 * GXclear GXand GXandReverse GXcopy GXandInverted GXnoop GXxor GXor GXnor 
 * GXequiv GXinvert GXorReverse GXcopyInverted GXorInverted GXnand GXset
 */
unsigned char gd54xx_rops[16] = 
		{ 0, 0x05, 0x09, 0x0d, 0x50, 0x06, 0x59, 0x6d, 
		  0x90, 0x95, 0x0b, 0xad, 0xd0, 0xd6, 0xda, 0x0e };

int gd54xx_rop;

#if (defined(__DEBUG__))
int gd54xx_stplblt_debug = TRUE;
#endif

/*
 * gd54xx_poly_fillrect(x_origin,y_origin,count,rect_p)
 *			-for filling rectangles with solid color,tile,stipple
 *
 *	Input:
 *		SIint32		x_origin, y_origin -- 
 *								origin relative to which rectangles
 *								should be filled/tiled/stippled.
 *		SIint32		count	 -- No. of rectangles to be filled
 *		SIRectOutlineP	*rect_p - Pointer to the rectangle
 *
 */
SIBool
gd54xx_poly_fillrect ( SIint32 x_origin, SIint32 y_origin, SIint32 count,
			 SIRectOutlineP rect_p )
{
	if (v256_gs->mode == GXnoop)
		return (SI_SUCCEED);

	/*
	 * we can do only certain types of FILL's - check it out.
	 */
	if ( ((v256_gs->fill_mode == SGFillTile) ||
		 (v256_gs->fill_mode == SGFillStipple) ||
		 (v256_gs->fill_mode == SGFillSolidFG) ||
		 (v256_gs->fill_mode == SGFillSolidBG)) &&
		 ((v256_gs->mode == GXcopy) || (v256_gs->mode == GXor) ||
		 (v256_gs->mode == GXxor) || (v256_gs->mode == GXand)) &&
		 (v256_gs->pmask == 0xFF) )
	{
		switch (v256_gs->fill_mode)
		{
			case SGFillTile : 
				gd54xx_tile_poly_fillrect(x_origin,y_origin,count,rect_p);
				return (SI_SUCCEED);
				break;
			case SGFillSolidFG :
			case SGFillSolidBG : 
				gd54xx_solid_poly_fillrect(x_origin,y_origin,count,rect_p);
				return (SI_SUCCEED);
				break;
			case SGFillStipple :
				gd54xx_stipple_poly_fillrect(x_origin,y_origin, count,rect_p);
				return (SI_SUCCEED);
				break;
			default :
				FALLBACK(si_poly_fillrect,(x_origin,y_origin,count,rect_p));
				break;
		}
	}
	else
	{
		FALLBACK(si_poly_fillrect,(x_origin,y_origin,count,rect_p));
	}
	
}



SIBool
gd54xx_tile_poly_fillrect ( SIint32 x_origin, SIint32 y_origin, SIint32 count,
			 SIRectOutlineP rect_p )
{
	int i,j;								/*temp*/
	int repeat,bytes_read = 0;				/*for local use */
	unsigned char tile_data_byte;
	unsigned char *tile_data_start_p;		/*Pointer to tile data */
	unsigned short *off_screen_p;			/*pointer to off screen memory to be
										  		used for downloading the tile */
	unsigned short tile_data_half_word;		/*tile data read */
	int source_pitch;
	int source_x, source_y;					/*x & y position (in pixels)
												in tile to read */
	int source_width,source_height;			/*width and height of source tile */
	int source_address;
	int destination_address;
    unsigned char rop;
    unsigned char flag;
	int old_source_x = -1;					/* illegal value */
	int old_source_y = -1;					/* illegal value */


	/*
	 *Sanity Checks..
	 */
	if ( (v256_gs->fill_mode != SGFillTile) || 
		(v256_gs->pmask != 0xFF) )
	{
#if (defined(__DEBUG__))
	fprintf(stderr,"Tile fill called with invalid parameters.Exiting..");
	exit (0);
#endif
		FALLBACK(si_poly_fillrect,(x_origin,y_origin,count,rect_p));
	}

	
	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	if (v256_gs->raw_tile.BbitsPerPixel != V256_PLANES)
	{
		return (SI_SUCCEED);
	}
	
#if (defined(__DEBUG__))
	if (gd54xx_tile_poly_fillrect_debug)
	{
		printf("\n(gd54xx_tile_poly_fillrect)");
		printf("\n\t x_origin  %d",x_origin);
		printf("\n\t y_origin  %d",y_origin);
		printf("\n\t count     %d",count);
		printf("\n\t width,height %d %d",rect_p->width,rect_p->height);
	}
#endif

	/*
	 *Loop through the list of rectangles
	 */
	do
	{
		register int destination_lx,destination_ty;
		register int destination_rx,destination_by;
		register int width, height;

		/*
		 *Get the destination region bounds
		 */
		destination_lx = rect_p->x + x_origin;
		destination_ty = rect_p->y + y_origin;
		width = rect_p->width;
		height = rect_p->height;
		destination_rx = destination_lx + width - 1;
		destination_by = destination_ty + height - 1;
	
		/* 
		 * Check destination_x ,destination_y to be within the screen width 
		 * & height.
		 */
	
		 if ((destination_lx > destination_rx) ||
			 (destination_ty > destination_by) ||
			 (destination_lx > v256_clip_x2) || 
			 (destination_ty > v256_clip_y2) ||
			 (destination_rx < v256_clip_x1) || 
			 (destination_by < v256_clip_y1) ||
			 (width <= 0) || (height <= 0))
		 {
			rect_p ++;
			continue;
		 }
	
   		/*
		 * Clip if destination_lx or destination_ty are out of screen but
		 * destination_x + width and destination_y + height are inside
		 * the screen.
		 */

		 if (destination_lx < v256_clip_x1)
		 {
			 destination_lx = v256_clip_x1;
	 	 }
	
		 if (destination_ty < v256_clip_y1)
		 {
			 destination_ty = v256_clip_y1;
		 }
	
	
   		/* 
		 * Clip if destination_rx or destination_by are out
		 * of the screen width and height
		 */
	
		 if (destination_rx > v256_clip_x2 )
		 {
		 	 destination_rx = v256_clip_x2;
		 }
	
   	 	 if (destination_by > v256_clip_y2)
		 {
			 destination_by = v256_clip_y2;
		 }

		width = (destination_rx - destination_lx + 1);
		height = (destination_by - destination_ty + 1);

		if ( (width < 1) || (height < 1) )
		{
			rect_p ++;
			continue;
		}


		/*
		 *Check for tile width and height to be greater than or equal to 32
		 *and less than 8.If so, this will be handled by the hardware
		 *function - gd54xx_big_tile_poly_fillrect.
		 */
		source_width = v256_gs->raw_tile.Bwidth;
		source_height = v256_gs->raw_tile.Bheight;
		
		if (((source_width > 8) && (source_width <= MAX_TILE_WIDTH)) || 
			((source_height > 8) && (source_height <= MAX_TILE_HEIGHT)) )
		{
			gd54xx_big_tile_poly_fillrect(x_origin,y_origin,count,rect_p);
			return (SI_SUCCEED); 
		}

		/*
		 *Check for tile width and height to be power of 2, less than 8
		 */
		if ((source_width & (source_width - 1)) || 
			(source_height & (source_height - 1)) )
		{
			FALLBACK(si_poly_fillrect,(x_origin,y_origin,count,rect_p));	
		} 
		else
		{
			/*
			 *Download the tile to offscreen area. 
			 *Tile data bytes can be accessed through pointer raw_tile_data
			 *If tile not at 8 pixel boundary, rotate the tile data and then
			 *download
			 */
			source_x = 
				(destination_lx - (v256_gs->raw_tile.BorgX)) & 
				(source_width - 1);	
			source_y = 
				(destination_ty - (v256_gs->raw_tile.BorgY)) &
				(source_height - 1);	
	
			if (source_x < 0) 
			{
				source_x += source_width;
			}
		
			if (source_y < 0) 
			{
				source_y += source_height;
			}
			if (old_source_x != source_x || old_source_y != source_y)
			{
				old_source_x = source_x;
				old_source_y = source_y;
	

				/*
				 *Program the BLIT ENGINE to write into off-screen memory
				 *For downloading the tile write 64 bytes of data sequentially
				 *in the off-screen memory.These 64 bytes will be used for tile
				 *fill using 8x8 tile.Rotation of tile data is being done before
				 *downloading
				 */
	
				off_screen_p = (unsigned short *)v256_fb;
			
				U_WIDTH(63);
				U_HEIGHT(0);
				U_DEST_PITCH(64);
				U_DEST_ADDR((vendorInfo.virtualX)*(vendorInfo.virtualY));
			
				/*Here mode is BitBlt from system source to display copy*/
				U_BLTMODE(4);
				
				U_ROP(0x0d);			/*source copy*/
				BLT_START(2);
			
				tile_data_start_p = v256_gs->raw_tile_data;
		
				/*
				 *Repetitions required to get 8x8 tile if the source
				 *tile is smaller than 8x8.
				 */	
				repeat = 8/source_height;	
		
				do
				{	
					for (i = 0 ; i < source_height ; i++)
					{
						unsigned char *tmp_row_p = tile_data_start_p +
							((source_y + i) % source_height)*
							((source_width+3)&~3);
		
						/*If the tile width is less than 8 ,repeat the 
						 *tile data to get a width of 8
					 	*/
						register int tmp_repeat = 8/source_width;	
			
						do
						{	
							for(j = 0; j < source_width ; j++)
							{
								if (bytes_read == 1)
								{
									tile_data_half_word = 
										(0x00ff & tile_data_byte) |
										(((unsigned short) (*(tmp_row_p + 
										((source_x + j) % source_width))))<<8);
									bytes_read = 0;
									*off_screen_p = tile_data_half_word;
								}
								else
								{
									tile_data_byte = 
										(*(tmp_row_p +
										((source_x + j) % source_width)));
									bytes_read = 1;
								}
							}
			
						}
						while ( --tmp_repeat > 0);
					}
				}
				while ( --repeat > 0);
			
	
				WAIT_FOR_ENGINE_IDLE();
			}
		}
	
	
		/*
		 *Now the tile is in offscreen-memory
		 *source address - offscreen memory
		 */ 
		source_address = ((vendorInfo.virtualX)*(vendorInfo.virtualY));
		rop = gd54xx_rops[v256_gs->mode];
	
	
		/*
		 *Program the BLIT ENGINE for tiling.We do bliting of tile data
		 *in off-screen memory to the destination on screen
		 */
		U_DEST_PITCH(v256_slbytes);
		U_BLTMODE(0x40); 		/* always increment */
		U_ROP(rop);

		U_WIDTH(width - 1);
		U_HEIGHT(height - 1);
		destination_address = destination_lx + 
					(destination_ty * v256_slbytes); /* destination address */
		U_SRC_ADDR(source_address);
		U_DEST_ADDR(destination_address);

		BLT_START(2);

		WAIT_FOR_ENGINE_IDLE();

		rect_p ++;
	}
	while ( --count > 0);

	return (SI_SUCCEED);
}


SIBool
gd54xx_solid_poly_fillrect ( SIint32 xorg, SIint32 yorg, SIint32 cnt,
			 SIRectOutlineP prect )
{
	unsigned short *fill;
    int	srcaddr, destaddr;
    int i, j, k;
    unsigned char rop;
    unsigned char flag;

	/*
	 *Sanity checks...
	 */
	if ((v256_gs->fill_mode != SGFillSolidFG  &&
		v256_gs->fill_mode != SGFillSolidBG) || 
		(v256_gs->pmask != 0xFF) )
	{
		FALLBACK(si_poly_fillrect, (xorg, yorg, cnt, prect));
	}

	/*
	 * The color tile data is cached only if enough video Ram is
	 * available .If color tile data not cached we cannot use this 
	 * function.
	 */
	if ( vendorInfo.videoRam < 1024 )
	{
		FALLBACK(si_poly_fillrect, (xorg, yorg, cnt, prect));
	}
	/*
	 *Now program the BLIT ENGINE to do a write of tile data from
	 *off-screen to on-screen.Tile data (64 bytes) for each of the
	 *256 colors is put in the off-screen memory at gd54xx_init.
	 *This data is then indexed using the variable v256_src.
	 */
	srcaddr = TILE_DB_START + v256_src*TILE_DATA_SIZE;
	rop = gd54xx_rops[v256_gs->mode];

	U_SRC_PITCH(v256_slbytes);
	U_DEST_PITCH(v256_slbytes);
	U_BLTMODE(0x40); 		/* always increment */
	U_ROP(rop);

	for (i=0; i<cnt; i++, prect++)
	{
		register int x1 = prect->x + xorg;
		register int y1 = prect->y + yorg;
		register int w  = prect->width;
		register int h = prect->height;
		register int x2 = x1 + w - 1;
		register int y2 = y1 + h - 1;

		if ((w <= 0) || (h <= 0))
			continue;

		/* check if within clip area */
		if ((x1>x2) || (y1>y2) || (x1>v256_clip_x2) || 
			(x2<v256_clip_x1) || (y1>v256_clip_y2) || 
			(y2<v256_clip_y1))
		{
			continue; /* box is out of clip area */
		}

		/* Now, clip the box */
		if (x1<v256_clip_x1) x1 = v256_clip_x1;
		if (y1<v256_clip_y1) y1 = v256_clip_y1;
		if (x2>v256_clip_x2) x2 = v256_clip_x2;
		if (y2>v256_clip_y2) y2 = v256_clip_y2;

		/*
		 * Adjust width and height to the clip region
		 */
		w = x2 - x1 + 1;
		h = y2 - y1 + 1;

		if ( (w < 1) || (h < 1) )
		{
			continue;
		}

		U_WIDTH(w-1);
		U_HEIGHT(h-1);
		destaddr = x1 + (y1 * v256_slbytes); /* destination address */
		U_SRC_ADDR(srcaddr);
		U_DEST_ADDR(destaddr);

		BLT_START(2);
	
		WAIT_FOR_ENGINE_IDLE();

	}
	return(SI_SUCCEED);
}


SIBool
gd54xx_stipple_poly_fillrect (SIint32 x_origin, SIint32 y_origin, SIint32 count,
			 SIRectOutlineP rect_p )
{
	int i,j;									/*temp*/
	int repeat,bytes_read = 0;					/*for local use */
	unsigned char stipple_data;					/*Stipple data */
	unsigned char data_byte;					/*For temporary storage of
											  	  stipple data*/
	unsigned short *off_screen_p;				/*pointer to off screen memory
												  to be used for downloading 
												  the stipple */
	unsigned short stipple_data_half_word;		/*stipple data read */
	int source_pitch;
	int source_x, source_y;						/*x & y position (in pixels)
												  in tile to read */
	int source_width,source_height;				/*width and height of source
												  stipple */
	int source_address;
	int destination_address;
    unsigned char rop;
    unsigned char flag;

	unsigned char saved_extended_mode; 			/* saved parameters */
	unsigned char saved_mode; 					/* saved parameters */
	unsigned char saved_blt_mode; 				/* saved parameters */
	unsigned char saved_set_reset;				/* saved parameters */
	unsigned char saved_set_reset_enable;		/* saved parameters */
	
	unsigned char graphics_engine_mode;			/* saved parameters */

	int old_source_x  = -1;						/* illegal value */
	int old_source_y  = -1;						/* illegal value */


	/*
	 *Sanity Checks..
	 */
	if ((v256_gs->fill_mode != SGFillStipple) || 
		(v256_gs->pmask != 0xFF) )
	{
		FALLBACK(si_poly_fillrect,(x_origin,y_origin,count,rect_p));
	}
	
	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	if (v256_gs->raw_stipple.BbitsPerPixel != 1)
	{
		FALLBACK(si_poly_fillrect,(x_origin,y_origin,count,rect_p));
	}
	

#if (defined(__DEBUG__))
	if (gd54xx_stipple_poly_fillrect_debug)
	{
		printf("\n(gd54xx_stipple_poly_fillrect)");
		printf("\n\t x_origin  %d",x_origin);
		printf("\n\t y_origin  %d",y_origin);
		printf("\n\t count     %d",count);
		printf("\n\t width,height %d %d",rect_p->width,rect_p->height);
	}
#endif


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
	 * Enable extended write modes
	 */
	ENABLE_EX_WRITE_MODES(0x04);


	/*
	 * read the graphics engine mode.
	 */
	outb(0x3CE, 0x05);
	graphics_engine_mode = inb(0x3CF);

	/*
	 * Save the foreground and the background color registers.
	 * (register GR0 for bg & GR1 for fg)
	 */

	/* save the values first */
	outb(0x3CE, 0x00);
	saved_set_reset = inb(0x3CF);
	outb(0x3CE, 0x01);
	saved_set_reset_enable = inb(0x3CF);


	/*
	 *Loop through the list of rectangles
	 */
	do
	{
		register int destination_lx,destination_ty;
		register int destination_rx,destination_by;
		register int width = rect_p->width, height = rect_p->height;

	 	/*
		/*
	 	 *Get the destination region bounds
	 	 */
		destination_lx = rect_p->x + x_origin;
		destination_ty = rect_p->y + y_origin;
		destination_rx = destination_lx + width - 1;
		destination_by = destination_ty + height - 1;

		/* 
	 	 * Check destination_x ,destination_y to be within the screen width 
	 	 * & height.
	 	 */

	 	if ((destination_lx > destination_rx) ||
			 (destination_ty > destination_by) ||
			 (destination_lx > v256_clip_x2) || 
			 (destination_ty > v256_clip_y2) ||
			 (destination_rx < v256_clip_x1) || 
			 (destination_by < v256_clip_y1) ||
			 (width <= 0) || (height <= 0))
	 	{
			rect_p ++;
			continue;
	 	}

    	/*
	   	 * Clip if destination_lx or destination_ty are out of screen but
	 	 * destination_x + width and destination_y + height are inside
	 	 * the screen.
	 	 */

	 	if ( destination_lx < v256_clip_x1)
	 	{
	 		destination_lx = v256_clip_x1;
	 	}

	 	if ( destination_ty < v256_clip_y1)
	 	{
	 		destination_ty = v256_clip_y1;
	 	}


    	/* 
	 	 * Clip if destination_rx or destination_by are out
	 	 * of the screen width and height
	 	 */

	 	if (destination_rx > v256_clip_x2 )
	 	{
	 		 destination_rx = v256_clip_x2;
	 	}

     	if (destination_by > v256_clip_y2)
	 	{
			 destination_by = v256_clip_y2;
	 	}

		width = (destination_rx - destination_lx + 1);
		height = (destination_by - destination_ty + 1);

		if ( (width < 1) || (height < 1) )
		{
			rect_p ++;
			continue;
		}

		/*
 		 *Check for stipple width and height to be power of 2, less than 8
 		 */
		source_width = v256_gs->raw_stipple.Bwidth;
		source_height = v256_gs->raw_stipple.Bheight;
	
		if ((source_width & (source_width - 1)) || 
			(source_height & (source_height - 1)) ||
			(source_width > 8) || (source_height > 8))
		{
			FALLBACK(si_poly_fillrect,(x_origin,y_origin,count,rect_p));	
		} 
		else
		{
	
			/*
		 	 *Download the stipple to offscreen area selected above.
		 	 *Stipple data bytes can be accessed through the pointer 
			 *raw_stipple_data in v256 graphics state.
		 	 *If stipple not at 8 pixel boundry, repeat the stipple data before
		 	 *downloading.
		 	 */
			source_x = 
				(destination_lx - (v256_gs->raw_stipple.BorgX)) % source_width;	
			source_y = 
				(destination_ty - (v256_gs->raw_stipple.BorgY)) % source_height;	
	
			if (source_x < 0) source_x += source_width;
			if (source_y < 0) source_y += source_height;
		
			if (old_source_x != source_x || old_source_y != source_y)
			{
				old_source_x = source_x;
				old_source_y = source_y;
				/*
				 *Program the BLIT ENGINE to write into off-screen memory.
				 *For downloading the stipple write 8 bytes of data 
				 *sequentially in the off-screen memory.These 8 bytes will
				 * be used for stipple fill using 8x8 stipple.
				 *Rotation of stipple data is done before downloading
				 */

				off_screen_p = (unsigned short *)v256_fb;
				U_WIDTH(7);
				U_HEIGHT(0);
				U_DEST_PITCH(8);
				U_DEST_ADDR((vendorInfo.virtualX)*(vendorInfo.virtualY));
			
				/*Here mode is BitBlt from system source to display copy*/
				U_BLTMODE(4);
			
				U_ROP(0x0d);
				BLT_START(2);
	
				repeat = 8/source_height;				/*repetitions required 
														 to get a 8x8 stipple*/ 
				do
				{	
					for ( i = 0; i < source_height ; i++ )
					{
						register  int tmp_repeat = 8/source_width;
						register int tmp_stpl_row_no = 
									(((source_y + i ) % source_height)*4);
						unsigned char tmp_data_byte = 
									v256_gs->raw_stpl_data[tmp_stpl_row_no];
												
	
						for ( j = 0 ; j < tmp_repeat ; j++ )
						{
							data_byte = tmp_data_byte | 
										((tmp_data_byte) << (source_width*j));
						}
					
						data_byte = inverted_byte[data_byte];
	
						if (source_x)
						{
							register int k;
							for ( k =0 ; k < source_x ; k++ )
							{
								unsigned char most_sig_bit;
								most_sig_bit = data_byte & 0x80;
								data_byte = data_byte << 1 |
											(most_sig_bit >> 7);
							}
						}	
	
						bytes_read ++;
	
						if (bytes_read == 2)
						{
							stipple_data_half_word = 
								(stipple_data_half_word & 0x00ff)|
							((unsigned short)(data_byte) << 8);
							*off_screen_p = stipple_data_half_word;
							bytes_read = 0;
						}
						else
						{
							stipple_data_half_word = 0x00ff &
													 (unsigned short)data_byte;
						}
					}	
				}
				while ( --repeat > 0);
	
				WAIT_FOR_ENGINE_IDLE();
			}
		}

		/*
	 	 *Now the stipple is in offscreen-memory
	 	 *source address - offscreen memory
	 	 */ 
		source_address = ((vendorInfo.virtualX)*(vendorInfo.virtualY));
		rop = gd54xx_rops[v256_gs->mode];


		/*
	 	 *Program the BLIT ENGINE for stippling.We do bliting of stipple data
	 	 *in off-screen memory to the destination on screen
	 	 */
		U_DEST_PITCH(v256_slbytes);

		if (v256_gs->stp_mode != SGOPQStipple)
		{
			U_WRITE_MODE((graphics_engine_mode & ~7) | 0x4); /* write mode 4 */
	
			U_BLTMODE(0xc8);		/* direction increment, enable color
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
	
			U_BLTMODE(0xc0);		/* direction increment, enable color
									   expand, dont enable transparency, source
									   video memory, destination video memory */
			/*
			 *For opaque stippling the register GR0 and GR1 should be
			 *set to background and foreground colors.
			 */
			U_BLT_FG_LOW(v256_gs->fg);
			U_BLT_BG_LOW(v256_gs->bg);

		}


		U_ROP(rop);
		
		U_WIDTH(width - 1);
		U_HEIGHT(height - 1);
		destination_address = destination_lx + 
					(destination_ty * v256_slbytes); /* destination address */
		U_SRC_ADDR(source_address);
		U_DEST_ADDR(destination_address);

		BLT_START(2);


		WAIT_FOR_ENGINE_IDLE();

		rect_p ++;
	}
	while ( --count > 0);

	/*
	 * Restore the register states
	 */
	ENABLE_EX_WRITE_MODES(saved_extended_mode);
	U_WRITE_MODE(graphics_engine_mode);
	U_BLTMODE(0x00);
	U_BLT_FG_LOW(saved_set_reset_enable);
	U_BLT_BG_LOW(saved_set_reset);

	return (SI_SUCCEED);
}

		
/*
 * scr->scr to bitblt
 */

/* 
 *    gd5426_ss_bitblt(sx, sy, dx, dy, w, h)    -- Moves pixels from one screen
 *                        position to another using the
 *                        ROP from the setdrawmode call.
 *
 *    Input:
 *        int    sx    -- X position (in pixels) of source
 *        int    sy    -- Y position (in pixels) of source
 *        int    dx    -- X position (in pixels) of destination
 *        int    dy    -- Y position (in pixels) of destination
 *        int    w    -- Width (in pixels) of area to move
 *        int    h    -- Height (in pixels) of area to move
 */
SIBool
gd54xx_ss_bitblt(sx, sy, dx, dy, w, h)
int    sx, sy, dx, dy;
int    w, h;
{
    int	srcaddr, destaddr;
    register int i, j, k;
    register BYTE *p,*q;
    DDXPointRec pt1, pt2;
    int	rop;
    unsigned char flag;

    /*
     * flag to indicate the direction of the bitblt
     * 		= 0	from top left to bottom right
     *		= 1	from bottom right to top left
     */
    SIBool    direction;

	/*
	 * check if we have anything to do at all
	 */
	if ( (v256_gs->mode == GXnoop) || ((w == 0) || (h == 0)) )
	{
		return (SI_SUCCEED);
	}

	/*
	 * 8/10/93: there doesn't seem to be an easy way to set plane mask
	 * and still use hardware bitblting; If all the planes are not
	 * enabled, use the software version
	 */
	if (v256_gs->pmask != MASK_ALL)
	{
		FALLBACK(si_ss_bitblt,(sx, sy, dx, dy, w, h));
	}

	/*
	 * compute if the source and destination regions overlap
	 * and if is there a necessity to do a backward copy
	 *
	 * direction = 1  copy from top-left to right-bottom
	 * 	     = 0  copy from right-bottom to top-left
	 */
	direction = 1;

	j = OFFSET(sx,sy);
	k = OFFSET(dx,dy);
	if ( (j < k) && k < (j + h*v256_slbytes + w)) 
	{
	    /* Yes, there is an overlap; now decide if we have to
	     * 	copy from right-bottom to top-left
	     */
	    if ( (dx>sx) || ((dx+w)>sx) )  {
		direction = 0;			
	    }
	}

	/* is v256_gs->mode guaranteed to be between 0-15 ?? hope so */
	rop = gd54xx_rops[v256_gs->mode];

	U_WIDTH(w-1);
	U_HEIGHT(h-1);
	U_SRC_PITCH(v256_slbytes);
	U_DEST_PITCH(v256_slbytes);

	/* source  and destination address */
	srcaddr =  sx + (sy * v256_slbytes);
	destaddr = dx + (dy * v256_slbytes);

	/* if reverse direction, ie: from bottom-right to top-left, give
	 * the bottom-right as the source and destination addresses
	 */
	if (!direction) {
		srcaddr = sx + w - 1 + ( (sy + h - 1) * v256_slbytes);	
		destaddr = dx + w - 1 + ( (dy + h - 1) * v256_slbytes);	
	}

	U_SRC_ADDR(srcaddr);
	U_DEST_ADDR(destaddr);

	if (direction) {
		U_BLTMODE(0);	 /* increment */
	}
	else {
		U_BLTMODE(1);	/* decrement */
	}

	U_ROP(rop);
	BLT_START(2);

	/*
	 * temporary: wait until the command is done; the most optimum way
	 * would be to continue without waiting, but to check (if the 
	 * accelerator is ready to accept commands) at the begining of the
	 * functions that access the frame buffer. For this purpose,
	 * vendorInfo structure has a function ptr, "HWstatus" .... Once
	 * these checks are put in the main libvga256, then you can remove
	 * this while loop here
	 */
	WAIT_FOR_ENGINE_IDLE();

	return(SI_SUCCEED);
}


/*
 *    gd54xx_ms_stplblt(src, sx, sy, dx, dy, w, h, plane, opaque) 
 *                -- Stipple the screen using the bitmap in src.
 *
 *    Input:
 *        SIbitmapP    src    -- pointer to source data        
 *        int        sx    -- X position (in pixels) of source
 *        int        sy    -- Y position (in pixels) of source
 *        int        dx    -- X position (in pixels) of destination
 *        int        dy    -- Y position (in pixels) of destination
 *        int        w    -- Width (in pixels) of area to move
 *        int        h    -- Height (in pixels) of area to move
 *        int        plane    -- which source plane
 *        int        opaque    -- Opaque/regular stipple (if non-zero)
 *
 *	calls to cfb8check(opaque)stipple have been included in case 
 *	the forcetype was set for stipple type and later restore to 
 *	original values correnponding to the previous graphics state.
 */
SIBool
gd54xx_ms_stplblt(source_p, source_x, source_y, destination_x, destination_y,
					width, height, plane, forcetype)
SIbitmapP source_p;
SIint32 source_x, source_y;
SIint32 destination_x, destination_y;
SIint32 width, height, plane, forcetype;
{
	unsigned char *source_row_start_p;  	/*start of each row in bitmap */
	unsigned short *const destination_p = 
		((unsigned short *) v256_fb);   	/*pointer to frame buffer */
	int source_pitch;						/*bitmap width rounded to 4 bytes */
    unsigned char rop;						/*raster operation */
	int blit_mode;							/*value of the blit mode register */
	int is_opaque;							/* == 1 for opaque stippling, 0 for
										 	 *transparent stippling */
	int destination_address;				/*offset of start location on 
										 	 *the screen */
	int destination_rx,destination_by;		/*inclusive right,bottom point*/
    unsigned char flag;						/*temporary */
	int half_words_per_scanline;
	int bytes_per_scanline;
	int blt_height = height;				/*To save height for local use */
	int rem_width=0,blt_width=0;
	int total_count_of_half_words;			/*total number of half words in
										 	 *the bitmap */
	unsigned char saved_extended_mode; 		/*saved parameters */
	unsigned char saved_mode; 				/*saved parameters */
	unsigned char saved_blt_mode; 			/*saved parameters */
	unsigned char saved_set_reset;			/*saved parameters */
	unsigned char saved_set_reset_enable;	/*saved parameters */
	
	char graphics_engine_mode;

	/*
	 * Sanity check
	 */
	if (source_x < 0 || source_y < 0 || width <= 0 || height <= 0)
	{
		return (SI_SUCCEED);
	}

    /* 
	 * determine if we can use hardware or not... 
	 */
    if (source_p->BbitsPerPixel != 1)
	{
		FALLBACK(si_ms_stplblt,(source_p,source_x,source_y,
				destination_x,destination_y,width,height,plane,forcetype));
    }
	/*
	 * The hardware cannot be used if we have a rops other than copy,
	 * or ,and.
	 */
	if ((v256_gs->mode != GXcopy) && (v256_gs->mode != GXand) && 
		(v256_gs->mode != GXor))
	{
		FALLBACK(si_ms_stplblt,(source_p,source_x,source_y,
				destination_x,destination_y,width,height,plane,forcetype));
    }
	/* 
	 * Check destination_x ,destination_y to be within the screen width 
	 * & height.
	 */
	 destination_rx = destination_x + width - 1;
	 destination_by = destination_y + height - 1;

	 if (destination_x > v256_clip_x2 || 
		 destination_y > v256_clip_y2 ||
		 destination_rx < v256_clip_x1 || 
		 destination_by < v256_clip_y1)
	 {
		 return (SI_SUCCEED);
	 }

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
	 *We cannot handle cases where the width is less than a byte
	 *So such cases can be passed to the software.
	 */
	if (width <= 8)
	{
		FALLBACK(si_ms_stplblt,(source_p,source_x,source_y,
				destination_x,destination_y,width,height,plane,forcetype));
	}

	/*
	 *For the case where source_x is not at a byte boundry
	 *do the stipling of the incomplete byte using software
	 */
	if (source_x & 7)
	{
		blt_width = ((source_x + 7) & ~7) - source_x;	
		v256_ms_stplblt(source_p,source_x,source_y,
				destination_x,destination_y,blt_width,height,plane,forcetype);
		width -= blt_width;
		source_x += blt_width;
		destination_x += blt_width;
	}

	/*
	 * Again, if with is less than a byte, use software
	 */
	if (width <= 8)
	{
		FALLBACK(si_ms_stplblt,(source_p,source_x,source_y,
				destination_x,destination_y,width,height,plane,forcetype));
	}


    /*
	 * Clip if destination_x or destination_y are out of screen but
	 * destination_x + width and destination_y + height are inside
	 * the screen.
	 */

	 if (destination_x < v256_clip_x1)
	 {
		destination_x = v256_clip_x1;
	 }

	 if (destination_y < v256_clip_y1)
	 {
	 	destination_y = v256_clip_y1;
	 }


    /* 
	 * Clip if destination_x + width or destination_y + height are out
	 * of the screen width and height
	 */

	 if (destination_rx > v256_clip_x2 )
	 {
		 destination_rx = v256_clip_x2;
	 }

     if (destination_by > v256_clip_y2)
	 {
		 destination_by = v256_clip_y2;
	 }

	 width = destination_rx - destination_x + 1;
	 height = destination_by - destination_y + 1;

	/*
	 *Call the software function if the width is less than 8
	 * i.e. we have less than a byte to write per scanline
	 */
	bytes_per_scanline = ((width + 7) & ~7) >> 3;
	if (bytes_per_scanline <= 1)
	{
		FALLBACK(si_ms_stplblt,(source_p,source_x,source_y,
				destination_x,destination_y,width,height,plane,forcetype));
	}
	
	/*
	 *Destination address - on screen
	 */
    destination_address = destination_x + (destination_y * v256_slbytes);


	/*Save the states of the registers before being programmed
	 *for the BLIT operation
	 */
	outb(0x3ce, 0x0b);
	saved_extended_mode = inb(0x3cf);

	outb(0x3ce, 0x05);
	saved_mode = inb(0x3cf);

	outb(0x3ce, 0x30);
	saved_blt_mode = inb(0x3cf);

	outb(0x3CE, 0x00);
	saved_set_reset = inb(0x3CF);

	outb(0x3CE, 0x01);
	saved_set_reset_enable = inb(0x3CF);

	/*
	 * Enable extended write modes
	 */
	ENABLE_EX_WRITE_MODES(0x04);

	/*
	 *We do BLIT operation for only those cases where the
	 *the number bytes ( after rounding to nearest byte boundry)
	 *to write are even
	 */
	if (bytes_per_scanline & 1)
	{
		rem_width = width - (width & ~15);
		width = (width & ~15);
	}

    /*
	 * Program the BitBLT engine registers
	 */
	U_WIDTH(width - 1);
	U_HEIGHT(height - 1);
	U_DEST_PITCH(v256_slbytes);
	U_DEST_ADDR(destination_address);
	U_SRC_ADDR(0);

	
	/* 
	 * check if stipple mode setting in graphics state has to 
	 * be overridden if so setup new stipple arrays
	 */

	if (forcetype )
	{
		if (is_opaque)
		{
			/*
		     * opaque stippling
		     */
		    cfb8CheckOpaqueStipple(v256_gs->mode,
					   v256_gs->fg,
					   v256_gs->bg,
					   v256_gs->pmask);
		}
		else 
	    {
			/*
		     * transparent stippling
		     */
		    cfb8CheckStipple (v256_gs->mode,
				      v256_gs->fg,
				      v256_gs->pmask);
		}
	}

	/*
	 * If stipple_type is not zero the request is for a type of stippling
	 * different from the Graphics State. If forcetype is SGOPQStipple
	 * then perform opaque stippling, else perform transparent stippling.
	 */

	/*
	 * read the graphics engine mode.
	 */
	outb(0x3CE, 0x05);
	graphics_engine_mode = inb(0x3CF);
	
	if (!is_opaque)
	{
		U_WRITE_MODE((graphics_engine_mode & ~7) | 0x4); /* write mode 4 */

		U_BLTMODE(0x8C);			 /*direction increment, enable color
								   	   expand, enable transparency, source
								   	   system memory, destination video memory*/
		/*
	 	 *For transparent stippling the register GR1 should be
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

		U_BLTMODE(0x84);		/* direction increment, enable color
								   expand, dont enable transparency, source
								   system memory, destination video memory */
		/*
		 *For opaque stippling the register GR0 and GR1 should be
		 *set to background and foreground colors.
		 */
		U_BLT_FG_LOW(v256_gs->fg);
		U_BLT_BG_LOW(v256_gs->bg);

	}

	/* is v256_gs->mode guaranteed to be between 0-15 ?? hope so */
	rop = gd54xx_rops[v256_gs->mode];
    U_ROP(rop);

    BLT_START(2);

    /*
	 * Bitmap Width in bytes 
	 */ 

    source_pitch = ((source_p->Bwidth + 31) & ~31) >> 3; 
	source_row_start_p = 
		((unsigned char *) source_p->Bptr) + 
		(source_y * source_pitch) + (source_x >> 3);
  
	half_words_per_scanline = bytes_per_scanline >> 1 ;
	total_count_of_half_words = half_words_per_scanline * height;
	
#if (defined(__DEBUG__))

	if (gd54xx_stplblt_debug)
	{
		printf("\n sx, sy %d %d",source_x,source_y);
		printf("\n dx, dy %d %d",destination_x,destination_y);
		printf("\n width,height %d %d",width,height);
		printf("\n fg bg %x %x",v256_gs->fg,v256_gs->bg);
		printf("\n rop %x",rop);
		printf("\n Bwidth %d",source_p->Bwidth);
		printf("\n half_words_per_scanline %d", half_words_per_scanline);
	}
	
#endif /* __DEBUG__ */

	/*
	 * Write the stipple bytes out to the blit engine.
	 */
	do
    {
		register int tmp_half_words_per_scanline = half_words_per_scanline;

		register unsigned short *source_short_p = 
					(unsigned short *) source_row_start_p;

		do 
		{

			unsigned short source_bits = *source_short_p++;
			unsigned short half_word = 
				inverted_byte[(source_bits & 0xFF)] | 
				((inverted_byte[(source_bits & 0xFF00U) >> 8U]) << 8);

			*destination_p = half_word;
			
		}
		while ( --tmp_half_words_per_scanline > 0);

		source_row_start_p += source_pitch;

	}
	while ( --height > 0);

	/*
	 * Write a multiple of 4 bytes to complete the operation.
	 */

	if (total_count_of_half_words & 1)
	{
		*destination_p = 0x0000;
	}

	/*
	 * Wait for the blit operation to complete.
	 */
	WAIT_FOR_ENGINE_IDLE();

	/*
	 * restore stipple arrays to correspond to graphics state if 
	 * required
	 */

	if (forcetype)
	{
		if(v256_gs->stp_mode == SGOPQStipple)
	    {
			/*
		     * opaque stippling
		     */
		    cfb8CheckOpaqueStipple( v256_gs->mode,
					   v256_gs->fg,
					   v256_gs->bg,
					   v256_gs->pmask);
		}
	    else 
	    {
		    /*
		     * transparent stippling
		     */
		    cfb8CheckStipple (  v256_gs->mode,
				      v256_gs->fg,
				      v256_gs->pmask);
		}
	}

	/*
	 * Restore the register states
	 */
	ENABLE_EX_WRITE_MODES(saved_extended_mode);
	U_WRITE_MODE(graphics_engine_mode);
	U_BLTMODE(0x00);
	U_BLT_FG_LOW(saved_set_reset_enable);
	U_BLT_BG_LOW(saved_set_reset);
	
	/*
	 *If number of bytes to be written to display are odd
	 *the last byte is to be written using software function
	 */	
	if (rem_width)
	{
		source_x += width;
		destination_x += width;
		v256_ms_stplblt(source_p,source_x,source_y,
			destination_x,destination_y,rem_width,blt_height,plane,forcetype);
	}

	return (SI_SUCCEED);

}

#ifdef NOT_USEFUL
/*
 *  gd54xx_ms_bitblt(src, sx, sy, dx, dy, w, h)
 *	-- Moves pixels from memory to the screen using the ROP 
 * 	from the setdrawmode call.
 *
 *  Input:
 *      SIbitmapP  src    -- pointer to source data        
 *      int        sx    -- X position (in pixels) of source
 *      int        sy    -- Y position (in pixels) of source
 *      int        dx    -- X position (in pixels) of destination
 *      int        dy    -- Y position (in pixels) of destination
 *      int        w    -- Width (in pixels) of area to move
 *      int        h    -- Height (in pixels) of area to move
 * The hardware function for memory to screen bitblt is comparable
 * to the software ( vga256) function so we retain the software function
 * for blitting.
 */
SIBool
gd54xx_ms_bitblt(source_p, source_x, source_y,
				destination_x, destination_y, width, height)
SIbitmapP      source_p;
SIint32        source_x, source_y, destination_x, destination_y;
SIint32        width, height;
{
	int i,j;
	unsigned char *source_byte_p;
	unsigned char *source_data_p;
	unsigned short *source_short_p;
	unsigned long *source_word_p;
	unsigned short *destination_p;
    int rop;
	int destaddr;
	int source_pitch;
    unsigned char flag;
	unsigned int word_count,word_rem;
	unsigned char rem_byte;
	unsigned short rem_short;

    /*
     * check if we have anything to do at all
     */

    if ((v256_gs->mode == GXnoop) || 
		((width <= 0) || (height <= 0)))
	{
		return (SI_SUCCEED);
	}

    if (source_p->BbitsPerPixel != V256_PLANES)   /* only handle screen depth */
    {
        return(SI_SUCCEED);
    }


    /*
	 * Calling SI function if all the planes are not masked
     */
    if ((v256_gs->pmask != MASK_ALL ||
		 v256_gs->mode != GXcopy)) 
	{
		FALLBACK(si_ms_bitblt,(source_p,source_x,source_y,
			destination_x,destination_y,width,height));
    }

	/* 
	 * Check destination_x ,destination_y to be within the screen width 
	 * & height.
	 */

	 if (destination_x > v256_clip_x2 || 
		 destination_y > v256_clip_y2 ||
		 destination_x + width <= v256_clip_x1 || 
		 destination_y + height <= v256_clip_y1)
	 {
		 return (SI_SUCCEED);
	 }
    /*
	 * Clip if destination_x or destination_y are out of screen but
	 * destination_x + width and destination_y + height are inside
	 * the screen.
	 */

	 if ( destination_x < 0 && (destination_x + width -1) > v256_clip_x1)
	 {
	 destination_x = v256_clip_x1;
	 }

	 if ( destination_y < 0 && (destination_x + height -1) > v256_clip_y1)
	 {
	 destination_y = v256_clip_y1;
	 }


    /* 
	 * Clip if destination_x + width or destination_y + height are out
	 * of the screen width and height
	 */

	 if ((destination_x + width -1) > v256_clip_x2 )
	 {
		 width = (v256_clip_x2 - destination_x + 1);
	 }

     if ((destination_y + height) > v256_clip_y2)
	 {
		 height = (v256_clip_y2 - destination_y + 1);
	 }
	
	/* is v256_gs->mode guaranteed to be between 0-15 ?? hope so */
	/*rop = gd54xx_rops[v256_gs->mode];
	*/
	rop = 0x0d;
	
    destaddr = destination_x + (destination_y * v256_slbytes);
    /*
	 * Program the BitBLT engine registers
	 */

		U_WIDTH(width - 1);
		U_HEIGHT(height -1);
		U_DEST_PITCH(v256_slbytes);
		U_DEST_ADDR(destaddr);
	
	/*Here mode is BitBlt from system source to display copy*/
		U_BLTMODE(4);
		
	    U_ROP(rop);
	    BLT_START(2);


    /*Bitmap Width in bytes */
    source_pitch = ((source_p -> Bwidth + 3) & ~3);
	source_byte_p = ((unsigned char *) source_p->Bptr) +
		(source_y * source_pitch) + source_x;
  
	destination_p=((unsigned short *)v256_fb);

      word_count = width >> 2;
      word_rem = width & 0x3;

      switch (word_rem)
	{

	case 0:
	  for (i = 0; i < height; i++)
	    {
	      source_word_p = (unsigned long *) source_byte_p;
	      for (j = 0; j < word_count; j++)
		  {
		  source_short_p = (unsigned short *) source_word_p;
		  *destination_p = *source_short_p;
		  source_short_p ++;
		  *destination_p = *source_short_p;
		  source_word_p ++;
		  }

	      source_byte_p += source_pitch;
	    }
	  break;

	case 1:		/* One byte extra */
	  for (i = 0; i < height; i++)
	    {
	      source_word_p = (unsigned long *) source_byte_p;
	      for (j = 0; j < word_count; j++)
		  {
		  source_short_p = (unsigned short *) source_word_p;
		  *destination_p = *source_short_p;
		  source_short_p ++;
		  *destination_p = *source_short_p;
		  source_word_p ++;
		  }
          rem_byte = *(source_byte_p + (word_count << 2));
	      *destination_p = (0x00ff & rem_byte);
	      *destination_p = 0x0000;

	      source_byte_p += source_pitch;
	    }
	  break;

	case 2:		/* Two bytes extra */
	  for (i = 0; i < height; i++)
	    {
	      source_word_p = (unsigned long *) source_byte_p;
	      for (j = 0; j < word_count; j++)
		  {
		  source_short_p = (unsigned short *) source_word_p;
		  *destination_p = *source_short_p;
		  source_short_p ++;
		  *destination_p = *source_short_p;
		  source_word_p ++;
		  }
          rem_short = *((unsigned short *)(source_byte_p + (word_count << 2)));
	      *destination_p = rem_short;
	      *destination_p = 0x0000;

	      source_byte_p += source_pitch;
	    }
	  break;

	case 3:

	  for (i = 0; i < height; i++)
	    {
	      source_word_p = (unsigned long *) source_byte_p;
	      for (j = 0; j < word_count; j++)
		  {
		  source_short_p = (unsigned short *) source_word_p;
		  *destination_p = *source_short_p;
		  source_short_p ++;
		  *destination_p = *source_short_p;
		  source_word_p ++;
		  }

          rem_short = *((unsigned short *)(source_byte_p + (word_count << 2)));
	      *destination_p = rem_short;
          rem_byte = *(source_byte_p + (word_count << 2) + 2);
	      *destination_p = (0x00ff & rem_byte);
	      source_byte_p += source_pitch;
	    }
	  break;

	}

	WAIT_FOR_ENGINE_IDLE();

	return (SI_SUCCEED);
}

/*
 *  gd54xx_sm_bitblt(dst, sx, sy, dx, dy, w, h)
 *	-- Moves pixels from the screen to memory 
 *
 *  Input:
 *      SIbitmapP   dst -- pointer to destination buffer
 *      int     sx  -- X position (in pixels) of source
 *      int     sy  -- Y position (in pixels) of source
 *      int     dx  -- X position (in pixels) of destination
 *      int     dy  -- Y position (in pixels) of destination
 *      int     w   -- Width (in pixels) of area to move
 *      int     h   -- Height (in pixels) of area to move
 *  The hardware function for screen to memory bitblt is comparable
 *  to the software ( vga256) function, so we retain the software
 *  function.
 */
SIBool
gd54xx_sm_bitblt(destination_p, source_x, source_y, 
						destination_x, destination_y, width, height)
SIbitmapP      destination_p;
SIint32        source_x, source_y, destination_x, destination_y;
SIint32        width, height;
{
	int j;
	unsigned char *destination_start_p;
	unsigned short *destination_short_p;
	unsigned long *destination_word_p;
	volatile unsigned short *read_p;
    int rop;
	int srcaddr;
	int destination_pitch;
    unsigned char flag;
	unsigned int word_count,word_rem;
	unsigned short dummy;
	unsigned long word;


    /*
     * check if we have anything to do at all
     */

    if ( height <= 0 || width <= 0
	|| OUTSIDE(destination_x, 0, destination_p->Bwidth-width)
	|| OUTSIDE(destination_y, 0, destination_p->Bheight-height)) {
	return(SI_SUCCEED);		/* SI_FAIL for the last 2 cases? */
    }

    if ( destination_p->BbitsPerPixel != V256_PLANES)	/* only handle
															screen depth */
    {
        return(SI_SUCCEED);
    }


    /*
     * Unlike ms_bitblt, in no case the v256 code is faster.
     * Fallback only if a plane mask is involved or the ROP depends on
     * destination data.
     */
    if ( v256_gs->pmask != MASK_ALL ||
		(v256_gs->mode != GXcopy && 
		v256_gs->mode != GXcopyInverted)) 
		{ 
			FALLBACK(si_sm_bitblt,(destination_p,source_x,source_y,
						destination_x,destination_y,width,height));
		}
	/* 
	 * Check source_x ,source_y to be within the screen width 
	 * & height.
	 */

	 if ( source_x > v256_clip_x2 || 
		  source_y > v256_clip_y2 ||
		  (source_x + width <= v256_clip_x1) ||
		  (source_y + height <= v256_clip_y1))
	 {
		 return (SI_SUCCEED);
	 }

	/*
	 *Just check for engine idle
	 */
	WAIT_FOR_ENGINE_IDLE();

    /*
	 * Clip if source_x or source_y are out of screen but
	 * source_x + width and source_y + height are inside
	 * the screen.
	 */
	 if ( (source_x < 0) && ((source_x + width - 1) > v256_clip_x1))
	 {
		source_x = v256_clip_x1;
	 }

	 if ( (source_y < 0) && ((source_y + height - 1) > v256_clip_y1))
	 {
	 	source_y = v256_clip_y1;
	 }


    /* 
	 * Clip if source_x + width or source_y + height are out
	 * of the screen width and height
	 */

	 if ( (source_x + width - 1) > v256_clip_x2 )
	 {
		 width = (v256_clip_x2 - source_x + 1);
	 }

     if ( (source_y + height - 1) > v256_clip_y2)
	 {
		 height = (v256_clip_y2 - source_y + 1);
	 }
	
	/* is v256_gs->mode guaranteed to be between 0-15 ?? hope so */
	/*rop = gd54xx_rops[v256_gs->mode];
	*/
	rop = 0x0d;
	
    srcaddr = source_x + (source_y * v256_slbytes);

    /*
	 * Program the BitBLT engine registers
	 */

		U_WIDTH(width - 1);
		U_HEIGHT(height -1);
		U_SRC_PITCH(v256_slbytes);
		U_SRC_ADDR(srcaddr);
	
	/*Here mode is BitBlt from system source to display copy*/
		U_BLTMODE(2);
		
	    U_ROP(rop);
	    BLT_START(2);


    /*
	 * Bitmap Width in Bytes 
	 */
    destination_pitch = ((destination_p -> Bwidth + 3) & ~3);
  
	destination_start_p = (((unsigned char *)destination_p->Bptr) 
					+ (destination_y * destination_pitch) + destination_x);
	
	read_p = ((unsigned short *)v256_fb);

    word_count = width >> 2;
    word_rem = width & 0x3;

      switch (word_rem)
	{

	case 0:
		do
	    {
		  j=word_count -1;
		  destination_word_p = (unsigned long *)destination_start_p;
	      do 
		  {
		  	word = (unsigned long)*read_p;
		  	word = word | (((unsigned long)*read_p) << 16);
			*destination_word_p = word;
			destination_word_p ++;
		  }
		  while ( --j > 0);
		  	word = (unsigned long)*read_p;
		  	word = word | (((unsigned long)*read_p) << 16);
			*destination_word_p = word;
	      destination_start_p += destination_pitch;
	    }
		while ( --height > 0);
	  break;

	case 1:		/* One byte extra */
	  do 
	    { 
		  j=word_count;
		  destination_word_p = (unsigned long *)destination_start_p;
	      do 
		  {
		  	word = (unsigned long)*read_p;
		  	word = word | (((unsigned long)*read_p) << 16);
			*destination_word_p = word;
			destination_word_p ++;
		  }
		  while ( --j > 0);
		  word = (unsigned long)*read_p;
	      dummy = *read_p;	
		  *destination_word_p = 
			 ((*destination_word_p) & 0xffffff00) | (0x000000ff & word); 
	      destination_start_p += destination_pitch;
	    }
		while ( --height > 0);
	  break;

	case 2:		/* Two bytes extra */
	  do 
	    {
		  j=word_count;
		  destination_word_p = (unsigned long *)destination_start_p;
	      do 
		  {
		  	word = (unsigned long)*read_p;
		  	word = word | (((unsigned long)*read_p) << 16);
			*destination_word_p = word;
			destination_word_p ++;
		  }
		  while ( --j > 0);
		  word = (unsigned long)*read_p;
		  dummy = *read_p;
		  *destination_word_p = 
			 ((*destination_word_p) & 0xffff0000) | (0x0000ffff & word); 

	      destination_start_p += destination_pitch;
	    }
		while ( --height > 0);
	  break;

	case 3:

	  do
	    {
		  j=word_count;
		  destination_word_p = (unsigned long *)destination_start_p;
	      do
		  {
		  	word = (unsigned long)*read_p;
		  	word = word | (((unsigned long)*read_p) << 16);
			*destination_word_p = word;
			destination_word_p ++;
		  }
		  while ( --j >0);
		  word = (unsigned long)*read_p;
		  word = word | (((unsigned long)*read_p) << 16);
		  *destination_word_p = 
			 ((*destination_word_p) & 0xff000000) | (0x00ffffff & word); 

	      destination_start_p += destination_pitch;
	    }
		while ( --height >0);
	  break;

	}


	WAIT_FOR_ENGINE_IDLE();

	return (SI_SUCCEED);

}

/*
 *    gd54xx_select_state(indx, flag, state)    -- set the current state
 *                        to that specified by indx.
 *
 *    Input:
 *        int        indx    -- index into graphics states
 */
SIBool
gd54xx_select_state(indx)
int indx;
{
    SIBool r;

    r = (*(oldfns.si_select_state))(indx);

      /* values commonly used by BLT software */
    gd54xx_rop = (v256_gs->mode << 8) & 0xF00;

#if 0
      /* set up hardware */
    SET_BLT_INDEX();
    U_BLT_BG(v256_gs->bg);
#endif

    return r;
}
#endif /* NOT_USEFUL */

/*############################################################################*/
/*
 * SI 1.0 functions
 */

SIBool
gd54xx_1_0_poly_fillrect(SIint32 cnt, SIRectP prect)
{

	SIRectOutlineP new_prect,tmp_new_prect;
	SIRectP tmp_old_prect;
	SIint32 tmp_count;

	if ((new_prect = 
		(SIRectOutlineP)malloc(cnt*sizeof(SIRectOutline))) == NULL)
	{
		return SI_FAIL;
	}


	tmp_count = cnt;
	tmp_new_prect = new_prect;
	tmp_old_prect = prect;

	while (tmp_count --)
	{
		tmp_new_prect->x = tmp_old_prect->ul.x;
		tmp_new_prect->y = tmp_old_prect->ul.y;
		tmp_new_prect->width = tmp_old_prect->lr.x - tmp_old_prect->ul.x;
		tmp_new_prect->height = tmp_old_prect->lr.y - tmp_old_prect->ul.y;
	
		tmp_new_prect++;
		tmp_old_prect++;
	}

	if ((gd54xx_poly_fillrect(0,0,cnt,new_prect)) == SI_FAIL)
	{
		return SI_FAIL;
	}

	free (new_prect);

	return SI_SUCCEED;

}


SIBool
gd54xx_big_tile_poly_fillrect ( SIint32 x_origin, SIint32 y_origin,
									SIint32 count, SIRectOutlineP rect_p )
{
	int i,j;							/*temp*/
	int repeat,bytes_read = 0;			/*for local use */
	unsigned char tile_data_byte;
	unsigned char *tile_data_start_p;	/*Pointer to tile data */
	unsigned short *off_screen_p;		/*pointer to off screen memory to be
										  used for downloading the tile */
	unsigned short tile_data_half_word;		/*tile data read */
	int source_pitch;
	int source_x, source_y;		/*x & y position (in pixels) in tile to read */
	int source_width,source_height;		/*width and height of source tile */
	int source_address;
	int destination_address;
	int current_destination_address;
    unsigned char rop;
    unsigned char flag;
	int tile_width;
	int tile_height;
	int partial_tile_width;
	int partial_tile_height;
	int no_of_full_tile_widths;
	int no_of_full_tile_heights;


	if ( (v256_gs->fill_mode != SGFillTile) || 
		(v256_gs->pmask != 0xFF) )
	{
		FALLBACK(si_poly_fillrect,(x_origin,y_origin,count,rect_p));
	}
	
	/*
	 * Big tile can be downloaded only if Ram is enough.
	 */
	if ( vendorInfo.videoRam < 1024 )
	{
		FALLBACK(si_poly_fillrect,(x_origin,y_origin,count,rect_p));
	}

	/*
	 *Sanity Checks..
	 */
	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	if (v256_gs->raw_tile.BbitsPerPixel != V256_PLANES)
	{
		FALLBACK(si_poly_fillrect,(x_origin,y_origin,count,rect_p));
	}
#if (defined(__DEBUG__))
	printf("\n(gd54xx_big_tile_poly_fillrect)");
	printf("\n\t x_origin  %d",x_origin);
	printf("\n\t y_origin  %d",y_origin);
	printf("\n\t count     %d",count);
	printf("\n\t width,height %d %d",rect_p->width,rect_p->height);
#endif
	/*
	 *Tile's width and height.
	 */
	source_width = v256_gs->raw_tile.Bwidth;
	source_height = v256_gs->raw_tile.Bheight;

	if (((source_width*source_height + 3) & ~3) > BIG_TILE_MAX_SIZE ||
		(source_width > MAX_TILE_WIDTH) || (source_height > MAX_TILE_HEIGHT) )
	{
		FALLBACK(si_poly_fillrect,(x_origin,y_origin,count,rect_p));	
	} 
	else
	{
		register int i=0,j,l;

		/*
		 *Download the tile to offscreen area. 
		 *Program the BLIT ENGINE to write into off-screen memory
		 *For downloading the tile writes bytes of data sequentially
		 *in the off-screen memory.These bytes will be used for tile
		 *fill tile.Rotation of tile data is being done before
		 *downloading.
		 */

		off_screen_p = (unsigned short *)v256_fb;

		U_WIDTH(BLT_MAX_WIDTH - 1);
		U_HEIGHT(((BIG_TILE_MAX_SIZE)/BLT_MAX_WIDTH) - 1);
		U_DEST_PITCH(BLT_MAX_WIDTH);
		U_DEST_ADDR(BIG_TILE_OFFSCREEN_ADDRESS);
	
		/*Here mode is BitBlt from system source to display copy*/
		U_BLTMODE(4);
		
		U_ROP(0x0d);			/*source copy*/
		BLT_START(2);
	
		tile_data_start_p = v256_gs->raw_tile_data;
		for (l = 0 ; l < MAX_TILE_HEIGHT ; l++,i++)
		{
			unsigned char *tmp_row_p = tile_data_start_p +
				((i) % source_height)* ((source_width+3)&~3);
			for(j = 0; j < MAX_TILE_WIDTH ; j++)
			{
				if (bytes_read == 1)
				{
					tile_data_half_word = 
						(0x00ff & tile_data_byte) |
						(((unsigned short) (*(tmp_row_p + 
						((j) % source_width))))<<8);
					bytes_read = 0;
					*off_screen_p = tile_data_half_word;
				}
				else
				{
					tile_data_byte = 
						(*(tmp_row_p + ((j) % source_width)));
					bytes_read = 1;
				}
			}
		}

		WAIT_FOR_ENGINE_IDLE();
	}	

	tile_width = (MAX_TILE_WIDTH/source_width)*source_width;
	tile_height = (MAX_TILE_HEIGHT/source_height)*source_height;

	/*
	 *Loop through the list of rectangles
	 */
	do
	{
		register int destination_lx,destination_ty;
		register int destination_rx,destination_by;
		register int width, height;
		register int reduced_tile_width,reduced_tile_height;

		/*
		 *Get the destination region bounds
		 */
		destination_lx = rect_p->x + x_origin;
		destination_ty = rect_p->y + y_origin;
		width = rect_p->width;
		height = rect_p->height;
		destination_rx = destination_lx + width - 1;
		destination_by = destination_ty + height - 1;
	
		/* 
		 * Check destination_x ,destination_y to be within the screen width 
		 * & height.
		 */
	
		 if ((destination_lx > destination_rx) ||
			 (destination_ty > destination_by) ||
			 (destination_lx > v256_clip_x2) || 
			 (destination_ty > v256_clip_y2) ||
			 (destination_rx < v256_clip_x1) || 
			 (destination_by < v256_clip_y1) ||
			 (width <= 0) || (height <= 0))
		 {
			rect_p ++;
			continue;
		 }
	
   		/*
		 * Clip if destination_lx or destination_ty are out of screen but
		 * destination_x + width and destination_y + height are inside
		 * the screen.
		 */

		 if (destination_lx < v256_clip_x1)
		 {
			 destination_lx = v256_clip_x1;
	 	 }
	
		 if (destination_ty < v256_clip_y1)
		 {
			 destination_ty = v256_clip_y1;
		 }
	
	
   		/* 
		 * Clip if destination_rx or destination_by are out
		 * of the screen width and height
		 */
	
		 if (destination_rx > v256_clip_x2 )
		 {
		 	 destination_rx = v256_clip_x2;
		 }
	
   	 	 if (destination_by > v256_clip_y2)
		 {
			 destination_by = v256_clip_y2;
		 }
	
		width = (destination_rx - destination_lx + 1);
		height = (destination_by - destination_ty + 1);

		if ( (width < 1) || (height < 1) )
		{
			rect_p ++;
			continue;
		}

		source_x = 
			(destination_lx - (v256_gs->raw_tile.BorgX)) % source_width;	
		source_y = 
			(destination_ty - (v256_gs->raw_tile.BorgY)) % source_height;	
		if (source_x < 0) 
		{
			source_x += source_width;
		}
	
		if (source_y < 0) 
		{
			source_y += source_height;
		}

		reduced_tile_width = tile_width - source_x;
		reduced_tile_height = tile_height - source_y;

		/*
		 *Now the tile is in offscreen-memory
		 *source address - offscreen memory
		 */ 
		source_address = BIG_TILE_OFFSCREEN_ADDRESS + source_x +
						 (source_y*MAX_TILE_WIDTH);
		rop = gd54xx_rops[v256_gs->mode];
		destination_address = destination_lx + 
					(destination_ty * v256_slbytes); /* destination address */
	
		/*
		 *Program the BLIT ENGINE for tiling.We do bliting of tile data
		 *in off-screen memory to the destination on screen
		 */
		U_SRC_PITCH(MAX_TILE_WIDTH);
		U_DEST_PITCH(v256_slbytes);
		U_BLTMODE(0x00); 		/* always increment */
		U_ROP(rop);

		if ( width <= reduced_tile_width )
		{ 
			register int tmp_repeat;
			register int tmp_partial_height;

			if ( height <= reduced_tile_height )
			{
				U_WIDTH(width - 1);
				U_HEIGHT(height - 1);
				
				U_SRC_ADDR(source_address);
				U_DEST_ADDR(destination_address);
				
				BLT_START(2);
				
				WAIT_FOR_ENGINE_IDLE();
				rect_p ++;
				continue;
			}

			U_WIDTH(width - 1);
			U_HEIGHT(reduced_tile_height - 1);
			
			U_SRC_ADDR(source_address);
			U_DEST_ADDR(destination_address);
			destination_address += (reduced_tile_height*v256_slbytes);
			
			BLT_START(2);
			
			WAIT_FOR_ENGINE_IDLE();

			height -= reduced_tile_height;
			tmp_repeat = height/tile_height;
			tmp_partial_height = height%tile_height;
			source_address = BIG_TILE_OFFSCREEN_ADDRESS + source_x; 

			for ( i = 0; i < tmp_repeat; i++ )
			{
				U_WIDTH(width - 1);
				U_HEIGHT(tile_height - 1);
				
				U_SRC_ADDR(source_address);
				U_DEST_ADDR(destination_address);
				destination_address += (tile_height*v256_slbytes);
				
				BLT_START(2);
				
				WAIT_FOR_ENGINE_IDLE();
			}

			if ( tmp_partial_height )
			{
				U_WIDTH(width - 1);
				U_HEIGHT(tmp_partial_height - 1);
				
				U_SRC_ADDR(source_address);
				U_DEST_ADDR(destination_address);
				
				BLT_START(2);
				
				WAIT_FOR_ENGINE_IDLE();
			}

			rect_p ++;
			continue;

		}

		U_DEST_PITCH(v256_slbytes);
		U_BLTMODE(0x00); 		/* always increment */
		U_ROP(rop);
		U_SRC_ADDR(source_address);

		current_destination_address = destination_address;

		if ( source_x )
		{
			if ( height <= reduced_tile_height )
			{
				reduced_tile_height = height;
			}
			if ( source_y )
			{
				U_WIDTH(reduced_tile_width - 1);
				U_HEIGHT(reduced_tile_height - 1);
				U_SRC_ADDR(source_address);
				U_DEST_ADDR(current_destination_address);
				current_destination_address += 
					(reduced_tile_height*v256_slbytes);
		
				BLT_START(2);
		
				WAIT_FOR_ENGINE_IDLE();
				height -= reduced_tile_height;
			}

			no_of_full_tile_heights = height/tile_height;
			partial_tile_height = height % tile_height;
			source_address = BIG_TILE_OFFSCREEN_ADDRESS + source_x ;

			U_SRC_ADDR(source_address);

			for ( j = 0; j < no_of_full_tile_heights; j++)
			{
				U_WIDTH(reduced_tile_width - 1);
				U_HEIGHT(tile_height - 1);
				
				U_SRC_ADDR(source_address);
				U_DEST_ADDR(current_destination_address);
				current_destination_address += (tile_height*v256_slbytes);
		
				BLT_START(2);
		
				WAIT_FOR_ENGINE_IDLE();
			}

			if ( partial_tile_height )
			{
				U_WIDTH(reduced_tile_width - 1);
				U_HEIGHT(partial_tile_height - 1);
				
				U_SRC_ADDR(source_address);
				U_DEST_ADDR(current_destination_address);
		
				BLT_START(2);
		
				WAIT_FOR_ENGINE_IDLE();
			}

			destination_address += reduced_tile_width;
			current_destination_address = destination_address;
			width -= reduced_tile_width;

		}

		if ( source_y )
		{
			no_of_full_tile_widths = width/tile_width;
			partial_tile_width = width % tile_width;
			source_address = BIG_TILE_OFFSCREEN_ADDRESS + 
				(source_y*MAX_TILE_WIDTH);

			if ( (!source_x) && (height <= reduced_tile_height) )
			{
				reduced_tile_height = height;
			}

			for ( j = 0; j < no_of_full_tile_widths; j++)
			{
				U_WIDTH(tile_width - 1);
				U_HEIGHT(reduced_tile_height - 1);
				
				U_SRC_ADDR(source_address);
				U_DEST_ADDR(current_destination_address);
				current_destination_address += tile_width;
		
				BLT_START(2);
		
				WAIT_FOR_ENGINE_IDLE();
			}

			if ( partial_tile_width )
			{
				U_WIDTH(partial_tile_width - 1);
				U_HEIGHT(reduced_tile_height - 1);
				
				U_SRC_ADDR(source_address);
				U_DEST_ADDR(current_destination_address);
		
				BLT_START(2);
		
				WAIT_FOR_ENGINE_IDLE();
			}

			destination_address += reduced_tile_height*v256_slbytes;
			current_destination_address = destination_address;

			if ( !source_x )
			{
				height -= reduced_tile_height;
			}


		}
		
		if ( (height <= 0) || (width <= 0) )
		{
			rect_p++;
			continue;
		}

		no_of_full_tile_widths = width/tile_width;
		no_of_full_tile_heights = height/tile_height;
		partial_tile_width = width % tile_width;
		partial_tile_height = height % tile_height;
		source_address = BIG_TILE_OFFSCREEN_ADDRESS;

		U_SRC_PITCH(MAX_TILE_WIDTH);
		U_DEST_PITCH(v256_slbytes);
		U_BLTMODE(0x00); 		/* always increment */
		U_ROP(rop);

		for ( i = 0; i < no_of_full_tile_widths; i++)
		{
			for ( j = 0; j < no_of_full_tile_heights; j++)
			{
				U_WIDTH(tile_width - 1);
				U_HEIGHT(tile_height - 1);
				
				U_SRC_ADDR(source_address);
				U_DEST_ADDR(current_destination_address);
				current_destination_address += (tile_height*v256_slbytes);
		
				BLT_START(2);
		
				WAIT_FOR_ENGINE_IDLE();
			}

			if ( partial_tile_height )
			{
				U_WIDTH(tile_width - 1);
				U_HEIGHT(partial_tile_height - 1);
				
				U_SRC_ADDR(source_address);
				U_DEST_ADDR(current_destination_address);
		
				BLT_START(2);
		
				WAIT_FOR_ENGINE_IDLE();
			}

			current_destination_address = destination_address + 
				(tile_width*(i+1));

		}

		if ( partial_tile_width )
		{
			for ( j = 0; j < no_of_full_tile_heights; j++)
			{
				U_WIDTH(partial_tile_width - 1);
				U_HEIGHT(tile_height - 1);
				
				U_SRC_ADDR(source_address);
				U_DEST_ADDR(current_destination_address);
				current_destination_address += (tile_height*v256_slbytes);
		
				BLT_START(2);
		
				WAIT_FOR_ENGINE_IDLE();
			}

			if ( partial_tile_height )
			{
				U_WIDTH(partial_tile_width - 1);
				U_HEIGHT(partial_tile_height - 1);
				
				U_SRC_ADDR(source_address);
				U_DEST_ADDR(current_destination_address);
		
				BLT_START(2);
		
				WAIT_FOR_ENGINE_IDLE();
			}
		}

		rect_p ++;
	}
	while ( --count > 0);

	return (SI_SUCCEED);
}

SIBool
gd54xx_FALLBACK_1_0_poly_fillrect ( SIint32 x_origin, SIint32 y_origin,
									SIint32 count, SIRectOutlineP rect_p )
{

	SIRectP new_prect,tmp_new_prect;
	SIRectOutlineP tmp_old_prect;
	SIint32 tmp_count;

	if ((new_prect = 
		(SIRectP)malloc(count*sizeof(SIRect))) == NULL)
	{
		return SI_FAIL;
	}


	tmp_count = count;
	tmp_new_prect = new_prect;
	tmp_old_prect = rect_p;

	while (tmp_count --)
	{
		tmp_new_prect->ul.x = tmp_old_prect->x;
		tmp_new_prect->ul.y = tmp_old_prect->y;
		tmp_new_prect->lr.x = tmp_old_prect->x + tmp_old_prect->width;
		tmp_new_prect->lr.y = tmp_old_prect->y + tmp_old_prect->height;
	
		tmp_new_prect++;
		tmp_old_prect++;
	}


	if (v256_1_0_fill_rect(count,new_prect) == SI_FAIL)
	{
		return (SI_FAIL);
	}

	free (new_prect);

	return (SI_SUCCEED);
}
/*
 * Local Variables:
 * tab-width: 4
 * End:
 */
