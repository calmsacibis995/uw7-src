/*
 *	@(#)s3cImage.c	6.1	3/20/96	10:23:18
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
 * S017, 30-Jun-93, staceyc
 * 	more multiheaded fixes
 * S016, 28-May-93, staceyc
 * 	full bankswitched support for gxcopy operations, ifdeffed out for
 *	BL4
 * S015, 27-May-93, staceyc
 * 	limited support for direct memory access using SVGA style bank
 * S014, 17-May-93, staceyc
 * 	multiheaded S3 card support
 * S013, 11-May-93, staceyc
 * 	include file clean up, replace calls to assembler code with calls
 * 	to C code
 * S012	Mon Apr 05 14:29:41 PDT 1993	hiramc@sco.COM
 *	Changed queue usage for 86C80[15], 86C928 after Kevin's code
 * S011	Wed Sep 23 08:50:04 PDT 1992	hiramc@sco.COM
 *	Remove SourceValidate calls, remove all mod comments
 *	prior to 1992.
 * X010	Tue Jun 02 11:31:58 PDT 1992	hiramc@sco.COM
 *	Bug in software cursor when copying areas.
 * X009 08-Jan-92 kevin@xware.com
 *      fixed bugs in S3CNAME(DrawImage)(), s3cReadImage(), s3cN2BlockOutW(),
 *	and s3cN2BlockInW() in the 4 bit display modes.
 * X008 01-Jan-92 kevin@xware.com
 *      update copyright notice.
 */

#include "s3cConsts.h"
#include "s3cMacros.h"
#include "s3cDefs.h"
#include "s3cProcs.h"

/*
 *  s3cN2BlockOutW() -- Nibble to Block Out Word
 *
 *	This routine will write a block of words to the 86c911
 *	in sets of four pixels from the 8 bit image format.
 *
 */

static void
s3cN2BlockOutW(
	unsigned char		*image,
	int			words)
{
	int			n;
	int			nibbles;
	register unsigned short	word;

	nibbles = words << 2;

	for ( n = 0; n < nibbles ; n+=4 )	
	{
		word  = ( image[n+0] << 4  ) & 0x00F0;
		word |= ( image[n+1] << 0  ) & 0x000F; 
		word |= ( image[n+2] << 12 ) & 0xF000;
		word |= ( image[n+3] << 8  ) & 0x0F00;
		S3C_OUT(S3C_VARDATA, word); 
	}
}


/*
 *  s3cN2BlockInW() -- Nibble to Block In Word
 *
 *	This routine will read a block of words from the 86c911
 *	and will store the sets of four pixels in the 8 bit image
 *	format.
 *
 */

static void
s3cN2BlockInW(
	unsigned char		*image,
	int			words)
{
	int			n;
	int			nibbles;
	register unsigned short	word;

	nibbles = words << 2;

	S3C_WAIT_FOR_DATA();
	
	for ( n = 0; n < nibbles ; n+=4 )	
	{
		word = S3C_INW(S3C_VARDATA); 
		image[n+0] = ( word & 0x00F0 ) >> 4;
		image[n+1] = ( word & 0x000F ) >> 0;
		image[n+2] = ( word & 0xF000 ) >> 12;
		image[n+3] = ( word & 0x0F00 ) >> 8;
	}
}


/*
 *  s3cB2BlockOutW() -- Byte to Block Out Word
 *
 *	This routine will write a block of words to the 86c911
 *	in the pairs of 1/2 pixels from the 16 bit image format.
 *
 */

static void
s3cB2BlockOutW(
	unsigned char		*image,
	int			words)
{
	int			b;
	int			bytes;
	register unsigned short	word;

	bytes = words << 2;

	S3C_CLEAR_QUEUE(8);
	
	for ( b = 0; b < bytes ; b+=4 )	
	{
		word  = image[b] & 0x00FF; 
		word |= ( image[b+2] & 0x00FF ) << 8; 
		S3C_OUT(S3C_VARDATA, word); 
	}
}


/*
 *  s3cB2BlockInW() -- Byte to Block In Word
 *
 *	This routine will read a block of words from the 86c911
 *	and will store the pairs of 1/2 pixels in the 16 bit 
 *	image format.
 *
 */

static void
s3cB2BlockInW(
	unsigned char		*image,
	int			words)
{
	int			b;
	int			bytes;
	register unsigned short	word;

	bytes = words << 2;

	S3C_WAIT_FOR_DATA();
	
	for ( b = 0; b < bytes ; b+=4 )	
	{
		word = S3C_INW(S3C_VARDATA); 
		image[b]   =   word & 0x00FF; 
		image[b+2] = ( word & 0xFF00 ) >> 8;
	}
}

/*
 *  s3cPixelBlit() -- Pixel Blit
 *
 *	This routine will set up the 86c911 for a through the plane
 *	image transfer. 
 */

static void
s3cPixelBlit(
	int		x,
	int		y,
	int		lx,
	int		ly,
	int		rop,
	int		mode,
	int		readplanemask,
	int		writeplanemask,
	int		command)
{
	S3C_CLEAR_QUEUE(8);		/*	was 4, S012	*/
	S3C_PLNWENBL(writeplanemask);
	S3C_PLNRENBL(readplanemask);
	S3C_SETMODE(mode);
	S3C_SETFN1(S3C_FNVAR, rop);
	S3C_SETX0(x);
	S3C_SETY0(y);
    	S3C_SETLX(lx);
	S3C_SETLY(ly);

	S3C_WAIT_FOR_IDLE();
	S3C_COMMAND(command);

}

#define S3C_SET_BANK(s3cPriv, offset) \
{ \
	S3C_OUTB(s3cPriv->crx, S3C_S3R5); \
	S3C_OUTB(s3cPriv->crd, offset); \
}

static void
s3cDrawMemImage(
BoxRec *pbox,
unsigned char *image,
unsigned int stride,
s3cPrivateData_t *s3cPriv)
{
	unsigned long ul_address, lr_address, ul_best, lr_best;
	unsigned long offset, fb_size;
	unsigned char *fb, *fb_base, *ip;
	int width, lx, ly, pitch, final_y;
	unsigned long last_y, partial_y0;
	long partial_width;
	BoxRec box;

	pitch = s3cPriv->width;
	fb_base = s3cPriv->fb_base;
	fb_size = s3cPriv->fb_size;
	box = *pbox;
	lx = box.x2 - box.x1;
	--box.x2;
	--box.y2;
	final_y = box.y2;
	last_y = box.y1;

	S3C_WAIT_FOR_IDLE();
	while (last_y <= final_y)
	{
		ul_address = box.y1 * pitch + box.x1;
		lr_address = box.y2 * pitch + box.x2;
		ul_best = (ul_address >> 16) << 16;
		lr_best = ul_best + fb_size;

		if (lr_address < lr_best)
		{
			last_y = final_y;
			partial_width = 0;
		}
		else
		{
			last_y = lr_best / pitch;
			partial_y0 = last_y * pitch;
			partial_width = lr_best - partial_y0 - box.x1;
			if (partial_width > lx)
				partial_width = 0;
			else
			{
				if (partial_width < 0)
					partial_width = 0;
				--last_y;
			}
		}
		box.y2 = last_y;

		ul_address -= ul_best;
		fb = fb_base + ul_address;
		ly = box.y2 - box.y1 + 1;
		offset = (ul_best >> 16) & 0x0f;
		ip = image;
		S3C_SET_BANK(s3cPriv, offset);
		while (ly--)
		{
			memcpy(fb, ip, lx);
			fb += pitch;
			ip += stride;
		}

		if (partial_width > 0)
		{
			fb = fb_base + partial_y0 + partial_width;
			memcpy(fb, ip, partial_width);
			S3C_SET_BANK(s3cPriv, offset + 1);
			memcpy(fb_base, ip + partial_width, lx - partial_width);
			++last_y;
		}
		++last_y;
		image += (last_y - box.y1) * stride;
		box.y1 = last_y;
		box.y2 = final_y;
	}
}

/*
 *  S3CNAME(DrawImage)() -- Draw Image
 *
 *	This routine will draw an image on the display in either the 
 *	4 or 8 bit display modes.
 *
 */

void 
S3CNAME(DrawImage)(
	BoxPtr 			pbox,
	unsigned char 		*image,
	unsigned int 		stride,
	unsigned char 		alu,     
	unsigned long 		planemask,
	DrawablePtr 		pDraw)
{
	int 			lx;
	int			ly;
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);

#if 0
	if (alu == GXcopy && s3cPriv->depth == 8 && (planemask & 0xff) == 0xff)
	{
		s3cDrawMemImage(pbox, image, stride, s3cPriv);
		return;
	}
#endif

	lx = pbox->x2 - pbox->x1;
	ly = pbox->y2 - pbox->y1;

	if (stride == 0)
		stride = lx;

	s3cPixelBlit(pbox->x1, pbox->y1, lx - 1, ly - 1, 
		S3CNAME(RasterOps)[alu], S3C_M_ONES, S3C_RPLANES, planemask, 
		S3C_WRITE_Z_DATA);

	if ( s3cPriv->depth == 8 )
	{
		if ( ( lx == stride ) && !( lx & 1 ) )
		{
			S3CNAME(BlockOutW)(image, ((lx * ly) + 1) >> 1);
		}
		else
		{
			for ( lx = lx + 1 >> 1; ly--; image += stride )
				S3CNAME(BlockOutW)(image, lx);
		}
	}
	else
	{
		if ( ( lx == stride ) && !( lx & 3 ) )
		{
			s3cN2BlockOutW(image, ((lx * ly) + 3) >> 2);
		}
		else
		{	
			for ( lx = lx + 3 >> 2; ly--; image += stride )
				s3cN2BlockOutW(image, lx);
		}
	}
}


/*
 *  S3CNAME(DrawImage16)() -- Draw Image 16
 *
 *	This routine will draw an image on the display in the 16 bit
 *	bit display modes.
 *
 */

void 
S3CNAME(DrawImage16)(
	BoxPtr 			pbox,
	unsigned char 		*image,
	unsigned int 		stride,
	unsigned char 		alu,     
	unsigned long 		planemask,
	DrawablePtr 		pDraw)
{
	int			y;
	int 			lx;
	int			ly;
	int			point;
	int			odd_width;
	int			last_word;
	char			*ip;
	s3cPrivateData_t	*s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);
	lx = pbox->x2 - pbox->x1;
	ly = pbox->y2 - pbox->y1;
	if ( stride == 0 )
		stride = lx << 1;

	point = lx + ly == 2;
	odd_width = lx & 1;

	s3cPixelBlit(pbox->x1, pbox->y1, lx - 1, ly - 1, 
		S3CNAME(RasterOps)[alu], S3C_M_ONES, S3C_RPLANES, planemask, 
		S3C_WRITE_Z_DATA);

	ip = image;

	if ( ( lx == ( stride >> 1 ) ) && !odd_width )
	{
		s3cB2BlockOutW(ip, ((lx * ly) + 1) >> 1);
	}
	else
	{	
		for ( y = 0; y < ly; y++, ip += stride )
			s3cB2BlockOutW(ip, lx + 1 >> 1);
	}

	s3cPixelBlit(pbox->x1 + 1024, pbox->y1, lx - 1, ly - 1, 
		S3CNAME(RasterOps)[alu], S3C_M_ONES, S3C_RPLANES, planemask >> 8, 
		S3C_WRITE_Z_DATA);

	ip = image + 1;

	if ( ( lx == ( stride >> 1 ) ) && !odd_width )
	{
		s3cB2BlockOutW(ip, ((lx * ly) + 1) >> 1);
	}
	else
	{	
		for ( y = 0; y < ly; y++, ip += stride )
			s3cB2BlockOutW(ip, lx + 1 >> 1);
	}
}


/*
 *  s3cReadImage() -- Read Image
 *
 *	This routine will read an image from the display in either the
 *	4 or 8 bit display modes.
 *
 */

void
S3CNAME(ReadImage)(
	BoxPtr 			pbox,
	unsigned char 		*image,
	unsigned int 		stride,
	DrawablePtr		pDraw)
{
	int			y;
	int			lx;
	int			ly;
	int			point;
	int			odd_width;
	int			last_word;
	register unsigned short	word;
	s3cPrivateData_t	*s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);
	lx = pbox->x2 - pbox->x1;
	ly = pbox->y2 - pbox->y1;
	if ( stride == 0 )
		stride = lx;

	point = lx + ly == 2;

	if ( s3cPriv->depth == 8 )
	{
		odd_width = lx & 1;

		s3cPixelBlit(pbox->x1, pbox->y1, 
			lx + odd_width - 1, ly - 1,
			S3C_FNREPLACE, S3C_M_DEPTH, S3C_ALLPLANES,S3C_ALLPLANES,
			S3C_READ_Z_DATA);

		if ( point )
		{
			word = S3C_INW(S3C_VARDATA);
			*image = word & 0x00FF;
		}
		else
		{
			if ( lx == stride && !odd_width )
			{
				S3CNAME(BlockInW)(image, (lx * ly) >> 1);
			}
			else
			{
				if ( !odd_width )
				{
					for (y = 0; y < ly; ++y, image+=stride)
						S3CNAME(BlockInW)(image,
						    lx >> 1);
				}
				else
				{
					last_word = lx - 1;
		
					for ( y = 0; y < ly; ++y, image+=stride)
					{
						S3CNAME(BlockInW)(image,
						    lx >> 1);
						image[last_word] = 
							S3C_INW(S3C_VARDATA) 
								& 0xFF;
					}
				}
			}
		}
	}
	else
	{
		odd_width = lx & 3;

		s3cPixelBlit(pbox->x1, pbox->y1, 
			( odd_width ? lx + ( 4 - odd_width ) : lx ) - 1, ly - 1,
			S3C_FNREPLACE, S3C_M_DEPTH, S3C_ALLPLANES,S3C_ALLPLANES,
			S3C_READ_Z_DATA);

		if ( point )
		{
			word = S3C_INW(S3C_VARDATA);
			*image = ( word & 0x00F0 ) >> 4;
		}
		else
		{
			if ( lx == stride && !odd_width )
			{
				s3cN2BlockInW(image, (lx * ly) >> 2);
			}
			else
			{
				if ( !odd_width )
				{
					for ( y = 0; y < ly; ++y, image+=stride)
						s3cN2BlockInW(image, lx >> 2);
				}
				else
				{
					last_word = lx - odd_width;
		
					for ( y = 0; y < ly; ++y, image+=stride)
					{
						s3cN2BlockInW(image, lx >> 2);
						word = S3C_INW(S3C_VARDATA);
						switch ( odd_width )
						{
						case 3:
						    image[last_word+2] = 
							( word & 0xF000 ) >> 12;

						case 2:	
						    image[last_word+1] = 
							( word & 0x000F ) >> 0;

						case 1:
						    image[last_word+0] = 
							( word & 0x00F0 ) >> 4;
						}	
					}
				}
			}
		}
	}
}


/*
 *  s3cReadImage16() -- Read Image 16
 *
 *	This routine will read an image from the display in the 16 bit
 *	display modes.
 *
 */

void
S3CNAME(ReadImage16)(
	BoxPtr 			pbox,
	unsigned char 		*image,
	unsigned int 		stride,
	DrawablePtr		pDraw)
{
	int			x;
	int			y;
	int			lx;
	int			ly;
	int			point;
	int			odd_width;
	int			last_word;
	char			*ip;
	register unsigned short	word;
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);

	lx = pbox->x2 - pbox->x1;
	ly = pbox->y2 - pbox->y1;
	if ( stride == 0 )
		stride = lx << 1;

	point = lx + ly == 2;
	odd_width = lx & 1;

	s3cPixelBlit(pbox->x1, pbox->y1, ( lx - 1 + odd_width), ly - 1, 
		S3C_FNREPLACE, S3C_M_DEPTH, S3C_ALLPLANES, S3C_ALLPLANES, 
		S3C_READ_Z_DATA);

	ip = image;

	if ( point )
	{
		word = S3C_INW(S3C_VARDATA);
		*ip = word & 0x00FF;
	}
	else
	{
		if ( lx == ( stride >> 1 )  && !odd_width )
		{
			s3cB2BlockInW(ip, (lx * ly) >> 1);
		}
		else
		{
			if ( !odd_width )
			{
				for ( y = 0; y < ly; ++y, ip+=stride)
					s3cB2BlockInW(ip, lx >> 1);
			}
			else
			{
				last_word = ( lx - 1 ) << 1;
	
				for ( y = 0; y < ly; ++y, ip+=stride)
				{
					s3cB2BlockInW(ip, lx >> 1);
					ip[last_word] = (char)
						S3C_INW(S3C_VARDATA);
				}
			}
		}
	}

	s3cPixelBlit(pbox->x1 + 1024, pbox->y1, ( lx - 1 + odd_width), ly - 1, 
		S3C_FNREPLACE, S3C_M_DEPTH, S3C_ALLPLANES, S3C_ALLPLANES, 
		S3C_READ_Z_DATA);

	ip = image + 1;

	if ( point )
	{
		word = S3C_INW(S3C_VARDATA);
		*ip = word & 0x00FF;
	}
	else
	{
		if ( lx == ( stride >> 1 ) && !odd_width )
		{
			s3cB2BlockInW(ip, (lx * ly) >> 1);
		}
		else
		{
			if ( !odd_width )
			{
				for ( y = 0; y < ly; ++y, ip+=stride)
					s3cB2BlockInW(ip, lx >> 1);
			}
			else
			{
				for ( y = 0; y < ly; ++y, ip+=stride)
				{
					s3cB2BlockInW(ip, lx >> 1);
					ip[last_word] = (char)
						S3C_INW(S3C_VARDATA);
				}
			}
		}
	}
}
