/*
 *	@(#)s3cMono.c	6.1	3/20/96	10:23:25
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
 * S024, 30-Jun-93, staceyc
 * 	stop using s3c symbols for multiheaded cards
 * S023, 17-May-93, staceyc
 * 	multiheaded S3 card support
 * S022, 11-May-93, staceyc
 * 	include file clean up, replace calls to assembler code with calls
 * 	to C code
 * S021	Mon Apr 05 14:42:44 PDT 1993	hiramc@sco.COM
 *	changed queue usage for 86C80[15], 86C928
 * S020	Thu Sep 24 08:46:54 PDT 1992	hiramc@sco.COM
 *	Change the bit swap stuff to reference the array
 *	in ddxMisc.c
 * X019	Wed Sep 09 08:15:03 PDT 1992	hiramc@sco.COM
 *	Add bit swap stuff, remove all mod history before 1992
 * X018 02-Aug-92 kevin@xware.com
 *      fixed bug #2 in work around for potentental 1280 clipping problem.
 * X017 19-Jul-92 kevin@xware.com
 *      fixed bug in work around for potentental 1280 clipping problem.
 * X016 07-Feb-92 kevin@xware.com
 *      added work around for potentental 1280 clipping problem.
 * X015 01-Jan-92 kevin@xware.com
 *      added s3cOffsetBlockOutW() and s3cOffsetBlockB() functions to fix
 *	problems where startx != 0.
 * X014 01-Jan-92 kevin@xware.com
 *      updated copyright notice.
 */

#include "s3cConsts.h"
#include "s3cMacros.h"
#include "s3cDefs.h"
#include "s3cProcs.h"

/*
 *  s3cFixedBlit() -- Fixed Blit
 *
 *	This routine will blit a mono image using the 86c911 blit engine.
 */

static void
s3cFixedBlit(
	int	x,
	int	y,
	int	lx,
	int	ly,
	int	xmax,
	int	rop,
	int	fg,
	int	planemask,
	int	command)
{
	S3C_CLEAR_QUEUE(6);
	S3C_PLNWENBL(planemask);
	S3C_PLNRENBL(S3C_RPLANES);
	S3C_SETMODE(S3C_M_VAR);
	S3C_SETFN0(S3C_FNCOLOR0, S3C_FNNOP);
	S3C_SETFN1(S3C_FNCOLOR1, rop);
	S3C_SETCOL1(fg);

	S3C_CLEAR_QUEUE(5);		/*	was 6, S021	*/
	S3C_SETX0(x);
	S3C_SETY0(y);
    	S3C_SETLX(lx);
	S3C_SETLY(ly);
	S3C_SETXMAX(xmax);

	S3C_WAIT_FOR_IDLE();		/*	S021	*/
	S3C_COMMAND(command);
}


/*
 *  s3cOpaqueFixedBlit() -- Opaque Fixed Blit
 *
 *	This routine will blit a opaque mono image using the 86c911 blit
 *	engine.
 */

static void
s3cOpaqueFixedBlit(
	int	x,
	int	y,
	int	lx,
	int	ly,
	int	xmax,
	int	rop,
	int	bg,
	int	fg,
	int	planemask,
	int	command)
{
	S3C_CLEAR_QUEUE(7);
	S3C_PLNWENBL(planemask);
	S3C_PLNRENBL(S3C_RPLANES);
	S3C_SETMODE(S3C_M_VAR);
	S3C_SETFN0(S3C_FNCOLOR0, rop);
	S3C_SETFN1(S3C_FNCOLOR1, rop);
	S3C_SETCOL0(bg);
	S3C_SETCOL1(fg);

	S3C_CLEAR_QUEUE(5);			/*	was 5, S021	*/
	S3C_SETX0(x);
	S3C_SETY0(y);
    	S3C_SETLX(lx);
	S3C_SETLY(ly);
	S3C_SETXMAX(xmax);

	S3C_WAIT_FOR_IDLE();
	S3C_COMMAND(command);
}

/*
 *  s3cOffsetBlockOutW() -- Offset Block Out Word
 *
 *	This routine will write a block of words to the 86c911
 *	shifting the image the offset in bits specified.
 *
 */

static void
s3cOffsetBlockOutW(
	unsigned char		*image,
	int			words,
	int			offset)
{
	int			b;
	int			bytes;
	int			noffset;
	register unsigned short hibits;
	register unsigned short lobits;
	register unsigned short	word;

	bytes = words << 1;
	noffset = 8 - offset;

	for ( b = 0; b < bytes ; b+=2 )	
	{
		hibits = DDXBITSWAP(image[b]) << offset;	/* X019 */
		lobits = DDXBITSWAP(image[b+1]) >> noffset;	/* X019 */
		word = ( hibits | lobits ) & 0x00FF;

		hibits = DDXBITSWAP(image[b+1]) << offset;	/* X019 */
		lobits = DDXBITSWAP(image[b+2]) >> noffset;	/* X019 */
		word |= ( ( hibits | lobits ) << 8 ) & 0xFF00;

		S3C_OUT(S3C_VARDATA, word);
	}
}

/*
 *  s3cOffsetBlockOutB() -- Offset Block Out Byte
 *
 *	This routine will write a block of bytes to the 86c911
 *	shifting the image the offset in bits specified.
 *
 *	NOTE: this routine is only called when the stride is a 
 *	single byte.
 */

static void
s3cOffsetBlockOutB(
	unsigned char		*image,
	int			bytes,
	int			offset)
{
	while ( bytes-- )
	{
		S3C_OUTB(S3C_VARDATA, DDXBITSWAP(*image++) << offset);
	}			/*	^^^ X019	*/
}

#if	BITMAP_BIT_ORDER == LSBFirst		/*	X019	vvv	*/
/*
 *	When we need to do bit swapping, these two routines are called instead
 *	of the assembly language versions s3cBlockOutW and s3cBlockOutB
 *	in s3cBlkOut.s
 */
void
S3CNAME(CBlockOutW)( unsigned char *values, int words )
{
	int bytes;
	register int i;
	register unsigned short *ap_p;
	register unsigned char t1;
	register unsigned char t2;
	unsigned short *ap_values;

	bytes = words << 1;

	ap_p=ap_values= (unsigned short *)ALLOCATE_LOCAL(words * sizeof(short));
	i = words;
	while (i--)
	{
		t1 = DDXBITSWAP(*values++);
		t2 = DDXBITSWAP(*values++);
		*ap_p++ = (t2 << 8) | t1;
	}

	S3CNAME(BlockOutW)( ap_values, words );
	DEALLOCATE_LOCAL((char *)ap_values);

	return;
}	/*	S3CNAME(CBlockOutW)( unsigned char *values, int words )	*/

void
S3CNAME(CBlockOutB)( unsigned char *values, int n )
{
	register int i;
	register unsigned char *ap_p;
	register unsigned char tmp;
	unsigned char *ap_values;

	ap_p = ap_values = (unsigned char *)ALLOCATE_LOCAL(n * sizeof(char));
	i = n;
	while (i--)
	{
		tmp = *values++;
		*ap_p++ = DDXBITSWAP(tmp);
	}

	S3CNAME(BlockOutB)( ap_values, n );
	DEALLOCATE_LOCAL((char *)ap_values);

	return;
}	/*	S3CNAME(CBlockOutB)( unsigned char *values, int n )	*/
#else
/*
 *	When we do NOT need to do bit swapping, reset all references
 *	to the above two routines to be to the assembly language versions
 */
#define	S3CNAME(CBlockOutW)	S3CNAME(BlockOutW)
#define	S3CNAME(CBlockOutB)	S3CNAME(BlockOutB)
#endif						/*	X019	^^^	*/

/*
 *  S3CNAME(DrawMonoImage)() -- Draw Mono Image
 *
 *	This routine will draw a mono image on the display in either the 
 *	4 or 8 bit display modes.
 *
 */

void
S3CNAME(DrawMonoImage)(
    	BoxPtr 			pbox,
    	unsigned char 		*image,
    	unsigned int 		startx,
    	unsigned int 		stride,
    	unsigned long 		fg,
    	unsigned char 		alu,
    	unsigned long 		planemask,
    	DrawablePtr 		pDraw)
{
	int 			lx;
	int			ly;
	int			i;
	int			words;
	int			start_byte;
	int			offset_bits;
	unsigned char		*ip;
	s3cPrivateData_t	*s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);
	lx = pbox->x2 - pbox->x1;
	ly = pbox->y2 - pbox->y1;
	if ( stride > 1 )
	{
		if ( ( pbox->x1 < 1024 )
		    && ( lx = ( pbox->x2 > 1024 ? 1024 : pbox->x2 ) - pbox->x1))
		{
			ip = image;
	
			s3cFixedBlit(pbox->x1, pbox->y1, lx - 1, ly - 1,
				(pbox->x1 + lx - 1), S3CNAME(RasterOps)[alu],
				fg, planemask, S3C_WRITE_X_Y_DATA);

			words = ( lx + 15 ) >> 4;
	
			if ( startx == 0 )
			{
				if ( stride == words << 1 )
					S3CNAME(CBlockOutW)(ip, words * ly);
				else
					for (i = 0; i < ly; ++i, ip += stride)
						S3CNAME(CBlockOutW)(ip, words);
			}
			else
			{
				start_byte = startx / 8;
				offset_bits = startx % 8;

				if ( offset_bits == 0 )
					for (i = 0; i < ly; ++i, ip += stride)
						S3CNAME(CBlockOutW)(&ip[start_byte], 
							words);
				else	
					for (i = 0; i < ly; ++i, ip += stride)
						s3cOffsetBlockOutW(
							&ip[start_byte], 
							words, offset_bits);
			}
			startx += 1024 - pbox->x1;
		}

		if ( ( pbox->x2 > 1024 ) 
		    && ( lx = pbox->x2 - ( pbox->x1 < 1024 ? 1024 : pbox->x1 )))
		{
			ip = image;
	
			s3cFixedBlit(( pbox->x1 < 1024 ? 1024 : pbox->x1 ), 
				pbox->y1, lx - 1, ly - 1,
				((pbox->x1 < 1024 ? 1024 : pbox->x1) + lx - 1),
				S3CNAME(RasterOps)[alu],
				fg, planemask, S3C_WRITE_X_Y_DATA);

			words = ( lx + 15 ) >> 4;
	
			if ( startx == 0 )
			{
				if ( stride == words << 1 )
					S3CNAME(CBlockOutW)(ip, words * ly);
				else
					for (i = 0; i < ly; ++i, ip += stride)
						S3CNAME(CBlockOutW)(ip, words);
			}
			else
			{
				start_byte = startx / 8;
				offset_bits = startx % 8;

				if ( offset_bits == 0 )
					for (i = 0; i < ly; ++i, ip += stride)
						S3CNAME(CBlockOutW)(&ip[start_byte], 
						words);
				else	
					for (i = 0; i < ly; ++i, ip += stride)
						s3cOffsetBlockOutW(
							&ip[start_byte], 
							words, offset_bits);
			}
		}
	}
	else
	{
		if ( ( pbox->x1 < 1024 )
		    && ( lx = ( pbox->x2 > 1024 ? 1024 : pbox->x2 ) - pbox->x1))
		{
			s3cFixedBlit(pbox->x1, pbox->y1, lx - 1, ly - 1,
				(pbox->x1 + lx - 1), S3CNAME(RasterOps)[alu],
				fg, planemask, S3C_WRITE_X_Y_BYTE);

			if ( startx == 0 )
				S3CNAME(CBlockOutB)(image, ly);
			else
				s3cOffsetBlockOutB(image, ly, startx);
			startx += 1024 - pbox->x1;
		}

		if ( ( pbox->x2 > 1024 ) 
		    && ( lx = pbox->x2 - ( pbox->x1 < 1024 ? 1024 : pbox->x1 )))
		{
			s3cFixedBlit(( pbox->x1 < 1024 ? 1024 : pbox->x1 ), 
				pbox->y1, lx - 1, ly - 1,
				((pbox->x1 < 1024 ? 1024 : pbox->x1) + lx - 1),
				S3CNAME(RasterOps)[alu],
				fg, planemask, S3C_WRITE_X_Y_BYTE);

			if ( startx == 0 )
				S3CNAME(CBlockOutB)(image, ly);
			else
				s3cOffsetBlockOutB(image, ly, startx);
		}
	}
	S3C_CLEAR_QUEUE(1);
	S3C_SETXMAX(s3cPriv->card_clip.x);
}


/*
 *  S3CNAME(DrawMonoImage16)() -- Draw Mono Image 16
 *
 *	This routine will draw a mono image on the display in the 16 bit
 *	display modes.
 *
 */

void
S3CNAME(DrawMonoImage16)(
    	BoxPtr 			pbox,
    	unsigned char 		*image,
    	unsigned int 		startx,
    	unsigned int 		stride,
    	unsigned long 		fg,
    	unsigned char 		alu,
    	unsigned long 		planemask,
    	DrawablePtr 		pDraw)
{
	int 			lx;
	int			ly;
	int			i;
	int			words;
	int			start_byte;
	int			offset_bits;
	unsigned char		*ip;
	s3cPrivateData_t	*s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);
	lx = pbox->x2 - pbox->x1;
	ly = pbox->y2 - pbox->y1;
	if ( stride > 1 )
	{
		ip = image;

		s3cFixedBlit(pbox->x1, pbox->y1, ( lx - 1), (ly - 1),
			(pbox->x1 + lx - 1), S3CNAME(RasterOps)[alu],
			fg, planemask, S3C_WRITE_X_Y_DATA);

		words = ( lx + 15 ) >> 4;
	
		if ( startx == 0 )
		{
			if ( stride == words << 1 )
				S3CNAME(CBlockOutW)(ip, words * ly);
			else
				for (i = 0; i < ly; ++i, ip += stride)
					S3CNAME(CBlockOutW)(ip, words);
		}
		else
		{
			start_byte = startx / 8;
			offset_bits = startx % 8;

			if ( offset_bits == 0 )
				for (i = 0; i < ly; ++i, ip += stride)
					S3CNAME(CBlockOutW)(&ip[start_byte], words);
			else	
				for (i = 0; i < ly; ++i, ip += stride)
					s3cOffsetBlockOutW(&ip[start_byte], 
						words, offset_bits);
		}
	
		ip = image;

		s3cFixedBlit(pbox->x1 + 1024, pbox->y1, lx - 1, ly - 1,
			pbox->x1 + 1024 + lx - 1, S3CNAME(RasterOps)[alu],
			fg >> 8, planemask >> 8, S3C_WRITE_X_Y_DATA);

		if ( startx == 0 )
		{
			if ( stride == words << 1 )
				S3CNAME(CBlockOutW)(ip, words * ly);
			else
				for (i = 0; i < ly; ++i, ip += stride)
					S3CNAME(CBlockOutW)(ip, words);
		}
		else
		{
			if ( offset_bits == 0 )
				for (i = 0; i < ly; ++i, ip += stride)
					S3CNAME(CBlockOutW)(&ip[start_byte], words);
			else	
				for (i = 0; i < ly; ++i, ip += stride)
					s3cOffsetBlockOutW(&ip[start_byte], 
						words, offset_bits);
		}
	}
	else
	{
		s3cFixedBlit(pbox->x1, pbox->y1, lx - 1, ly - 1,
			pbox->x1 + lx - 1, S3CNAME(RasterOps)[alu],
			fg, planemask, S3C_WRITE_X_Y_BYTE);

		if ( startx == 0 )
			S3CNAME(CBlockOutB)(image, ly);
		else
			s3cOffsetBlockOutB(image, ly, startx);
	
		s3cFixedBlit(pbox->x1 + 1024, pbox->y1, lx - 1, ly - 1,
			pbox->x1 + 1024 + lx - 1, S3CNAME(RasterOps)[alu],
			fg >> 8, planemask >> 8, S3C_WRITE_X_Y_BYTE);

		if ( startx == 0 )
			S3CNAME(CBlockOutB)(image, ly);
		else
			s3cOffsetBlockOutB(image, ly, startx);
	}
	S3C_CLEAR_QUEUE(1);
	S3C_SETXMAX(s3cPriv->card_clip.x);
}


/*
 *  S3CNAME(DrawOpaqueMonoImage)() -- Draw Opaque Mono Image
 *
 *	This routine will draw an opaque mono image on the display in either
 *	the 4 or 8 bit display modes.
 *
 */

void
S3CNAME(DrawOpaqueMonoImage)(
	BoxPtr 			pbox,
	unsigned char 		*image,
	unsigned int 		startx,
	unsigned int 		stride,
	unsigned long 		fg,
	unsigned long 		bg,
	unsigned char 		alu,
	unsigned long 		planemask,
	DrawablePtr 		pDraw)
{
	int 			lx;
	int			ly;
	int			i;
	int			words;
	int			start_byte;
	int			offset_bits;
	unsigned char		*ip;
	s3cPrivateData_t	*s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);
	lx = pbox->x2 - pbox->x1;
	ly = pbox->y2 - pbox->y1;
	if ( stride > 1 )
	{
		if ( ( pbox->x1 < 1024 )
		    && ( lx = ( pbox->x2 > 1024 ? 1024 : pbox->x2 ) - pbox->x1))
		{
			ip = image;
	
			s3cOpaqueFixedBlit(pbox->x1, pbox->y1, lx - 1, ly - 1,
				(pbox->x1 + lx - 1), S3CNAME(RasterOps)[alu],
				bg, fg, planemask, S3C_WRITE_X_Y_DATA);

			words = ( lx + 15 ) >> 4;
	
			if ( startx == 0 )
			{
				if ( stride == words << 1 )
					S3CNAME(CBlockOutW)(ip, words * ly);
				else
					for (i = 0; i < ly; ++i, ip += stride)
						S3CNAME(CBlockOutW)(ip, words);
			}
			else
			{
				start_byte = startx / 8;
				offset_bits = startx % 8;

				if ( offset_bits == 0 )
					for (i = 0; i < ly; ++i, ip += stride)
						S3CNAME(CBlockOutW)(
							&ip[start_byte], words);
				else	
					for (i = 0; i < ly; ++i, ip += stride)
						s3cOffsetBlockOutW(
							&ip[start_byte], 
							words, offset_bits);
			}
			startx += 1024 - pbox->x1;
		}

		if ( ( pbox->x2 > 1024 ) 
		    && ( lx = pbox->x2 - ( pbox->x1 < 1024 ? 1024 : pbox->x1 )))
		{
			ip = image;
	
			s3cOpaqueFixedBlit(( pbox->x1 < 1024 ? 1024 : pbox->x1),
				pbox->y1, lx - 1, ly - 1,
				((pbox->x1 < 1024 ? 1024 : pbox->x1) + lx - 1),
				S3CNAME(RasterOps)[alu],
				bg, fg, planemask, S3C_WRITE_X_Y_DATA);

			words = ( lx + 15 ) >> 4;
	
			if ( startx == 0 )
			{
				if ( stride == words << 1 )
					S3CNAME(CBlockOutW)(ip, words * ly);
				else
					for (i = 0; i < ly; ++i, ip += stride)
						S3CNAME(CBlockOutW)(ip, words);
			}
			else
			{
				start_byte = startx / 8;
				offset_bits = startx % 8;

				if ( offset_bits == 0 )
					for (i = 0; i < ly; ++i, ip += stride)
						S3CNAME(CBlockOutW)(&ip[start_byte], 
							words);
				else	
					for (i = 0; i < ly; ++i, ip += stride)
						s3cOffsetBlockOutW(
							&ip[start_byte], 
							words, offset_bits);
			}
		}
	}
	else
	{
		if ( ( pbox->x1 < 1024 )
		    && ( lx = ( pbox->x2 > 1024 ? 1024 : pbox->x2 ) - pbox->x1))
		{
			s3cOpaqueFixedBlit(pbox->x1, pbox->y1, lx - 1, ly - 1,
				(pbox->x1 + lx - 1), S3CNAME(RasterOps)[alu],
				bg, fg, planemask, S3C_WRITE_X_Y_BYTE);

			if ( startx == 0 )
				S3CNAME(CBlockOutB)(image, ly);
			else
				s3cOffsetBlockOutB(image, ly, startx);
			startx += 1024 - pbox->x1;
		}

		if ( ( pbox->x2 > 1024 ) 
		    && ( lx = pbox->x2 - ( pbox->x1 < 1024 ? 1024 : pbox->x1 )))
		{
			s3cOpaqueFixedBlit(( pbox->x1 < 1024 ? 1024 : pbox->x1),
				pbox->y1, lx - 1, ly - 1,
				((pbox->x1 < 1024 ? 1024 : pbox->x1) + lx - 1),
				S3CNAME(RasterOps)[alu],
				bg, fg, planemask, S3C_WRITE_X_Y_BYTE);

			if ( startx == 0 )
				S3CNAME(CBlockOutB)(image, ly);
			else
				s3cOffsetBlockOutB(image, ly, startx);
		}
	}
	S3C_CLEAR_QUEUE(1);
	S3C_SETXMAX(s3cPriv->card_clip.x);
}


/*
 *  S3CNAME(DrawOpaqueMonoImage16)() -- Draw Opaque Mono Image 16
 *
 *	This routine will draw an opaque mono image on the display in the
 *	16 bit display modes.
 *
 */

void
S3CNAME(DrawOpaqueMonoImage16)(
	BoxPtr 			pbox,
	unsigned char 		*image,
	unsigned int 		startx,
	unsigned int 		stride,
	unsigned long 		fg,
	unsigned long 		bg,
	unsigned char 		alu,
	unsigned long 		planemask,
	DrawablePtr 		pDraw)
{
	int 			lx;
	int			ly;
	int			i;
	int			words;
	int			start_byte;
	int			offset_bits;
	unsigned char		*ip;
	s3cPrivateData_t	*s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);
	lx = pbox->x2 - pbox->x1;
	ly = pbox->y2 - pbox->y1;
	ip = image;
	if ( stride > 1 )
	{
		s3cOpaqueFixedBlit(pbox->x1, pbox->y1, lx - 1, ly - 1,
			pbox->x1 + lx - 1, S3CNAME(RasterOps)[alu],
			bg, fg, planemask, S3C_WRITE_X_Y_DATA);

		words = ( lx + 15 ) >> 4;
	
		if ( startx == 0 )
		{
			if ( stride == words << 1 )
				S3CNAME(CBlockOutW)(ip, words * ly);
			else
				for (i = 0; i < ly; ++i, ip += stride)
					S3CNAME(CBlockOutW)(ip, words);
		}
		else
		{
			start_byte = startx / 8;
			offset_bits = startx % 8;

			if ( offset_bits == 0 )
				for (i = 0; i < ly; ++i, ip += stride)
					S3CNAME(CBlockOutW)(&ip[start_byte], words);
			else	
				for (i = 0; i < ly; ++i, ip += stride)
					s3cOffsetBlockOutW(&ip[start_byte], 
						words, offset_bits);
		}
	
		ip = image;

		s3cOpaqueFixedBlit(pbox->x1 + 1024, pbox->y1, lx - 1, ly - 1,
			pbox->x1 + 1024 + lx - 1, S3CNAME(RasterOps)[alu],
			bg >> 8, fg >> 8, planemask >> 8, S3C_WRITE_X_Y_DATA);

		if ( startx == 0 )
		{
			if ( stride == words << 1 )
				S3CNAME(CBlockOutW)(ip, words * ly);
			else
				for (i = 0; i < ly; ++i, ip += stride)
					S3CNAME(CBlockOutW)(ip, words);
		}
		else
		{
			if ( offset_bits == 0 )
				for (i = 0; i < ly; ++i, ip += stride)
					S3CNAME(CBlockOutW)(&ip[start_byte], words);
			else	
				for (i = 0; i < ly; ++i, ip += stride)
					s3cOffsetBlockOutW(&ip[start_byte], 
						words, offset_bits);
		}
	}
	else
	{
		s3cOpaqueFixedBlit(pbox->x1, pbox->y1, lx - 1, ly - 1,
			pbox->x1 + lx - 1, S3CNAME(RasterOps)[alu],
			bg, fg, planemask, S3C_WRITE_X_Y_BYTE);

		if ( startx == 0 )
			S3CNAME(CBlockOutB)(image, ly);
		else
			s3cOffsetBlockOutB(image, ly, startx);

		s3cOpaqueFixedBlit(pbox->x1 + 1024, pbox->y1, lx - 1, ly - 1,
			pbox->x1 + 1024 + lx - 1, S3CNAME(RasterOps)[alu],
			bg >> 8, fg >> 8, planemask >> 8, S3C_WRITE_X_Y_BYTE);

		if ( startx == 0 )
			S3CNAME(CBlockOutB)(image, ly);
		else
			s3cOffsetBlockOutB(image, ly, startx);
	}
	S3C_CLEAR_QUEUE(1);
	S3C_SETXMAX(s3cPriv->card_clip.x);
}
