/*
 *	@(#)s3cBres.c	6.1	3/20/96	10:23:03
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
 *
 * Modification History
 *
 * S009, 17-May-93, staceyc
 * 	support for multiheaded S3 cards
 * S008, 11-May-93, staceyc
 * 	include file clean up, replace calls to assembler code with calls
 * 	to C code
 * S007	Mon Jan 18 12:02:47 PST 1993 hiramc@sco.COM
 *		Pull over bresnaham parameter checks from the effBres.c
 * X006 01-Jan-92 kevin@xware.com
 *      updated copyright notice.
 * X005 31-Dec-91 kevin@xware.com
 *	added support for 16 bit 64K color modes.
 * X004 06-Dec-91 kevin@xware.com
 *	modified for style consistency, cosmetic only.
 * X003 30-Nov-91 kevin@xware.com
 *	changed declaration of s3cLine to static (cosmetic only).
 * X002 30-Nov-91 kevin@xware.com
 *	changed some hardcoded values to use values defined in s3cConsts.h.
 * X001 30-Nov-91 kevin@xware.com
 *	moved I/O bound section of s3cFillZeroSeg() to new assembly function
 *	s3cLine() in s3cLine.s.
 * X000 23-Oct-91 kevin@xware.com
 *	initial source adopted from SCO's sample 8514a source (eff).
 * 
 */

#include "s3cConsts.h"
#include "s3cMacros.h"
#include "s3cDefs.h"
#include "s3cProcs.h"

static void
s3cLine(
	int		x,
	int		y,
	int		e,
	int		e1,
	int		e2,
	int		len,
	int		rop,
	int		fg,
	int		planemask,
	int		command)
{

	S3C_CLEAR_QUEUE(5);
	S3C_PLNWENBL(planemask);
	S3C_PLNRENBL(S3C_RPLANES);
	S3C_SETMODE(S3C_M_ONES);
	S3C_SETFN1(S3C_FNCOLOR1, rop);
	S3C_SETCOL1(fg);

	S3C_CLEAR_QUEUE(7);
	S3C_SETX0(x);
	S3C_SETY0(y);
	S3C_SETET(e);
	S3C_SETK1(e1);
	S3C_SETK2(e2);
	S3C_SETLX(len);

	S3C_COMMAND(command);
}

/*
 *  s3cFillZeroSeg() -- Fill Solid Zero Width Segment
 *
 *	This routine will draw solid zero width line on the display 
 *	in either the 4 or 8 bit display modes.
 *
 */

void
S3CNAME(FillZeroSeg)(
	GCPtr		pGC,
	DrawablePtr	pDraw,
	int 		signdx,
	int 		signdy,
	int 		axis,
	int 		x,
	int 		y,
	int 		e,
	int 		e1,
	int 		e2,
	int 		len)
{
	unsigned short 	command;
	int 		cx;
	int		cy;
	unsigned char 	alu;
	unsigned long 	planemask;
	unsigned int 	fg;
	nfbGCPrivPtr 	pGCPriv = NFB_GC_PRIV(pGC);
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	/*
         * e (0x92E8) and e1 (0x8AE8) ports have 12 bits and a sign bit,
         * e2 (0x8EE8) has 13 bits and a sign bit, use gen to draw any
         * line that would have its error terms truncated if they were
         * written to these registers
         */
        if (e > 0xfff || e1 > 0xfff || e < -0x1000 || e1 < -0x1000 ||
            e2 > 0x1fff || e2 < -0x2000)
        {
                genSolidZeroSeg(pGC, pDraw, signdx, signdy, axis, x, y, e, e1,
                    e2, len);
                return;
        }

	alu = pGCPriv->rRop.alu;
	planemask = pGCPriv->rRop.planemask;
	fg = pGCPriv->rRop.fg;

	if (signdx == -1)
		cx = x - len;
	else
		cx = x;
	if (signdy == -1)
		cy = y - len;
	else
		cy = y;

	if (axis == X_AXIS)
		command = S3C_LINE_XN_YN_X;
	else
		command = S3C_LINE_XN_YN_Y;
	if (signdx > 0)
		command |= S3C_CMD_YN_XP_X;
	if (signdy > 0)
		command |= S3C_CMD_YP_XN_X;

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);

	s3cLine( x, y, e, e1, e2, len, 
		S3CNAME(RasterOps)[alu], fg, planemask, command);
}


/*
 *  s3cFillZeroSeg16() -- Fill Solid Zero Width Segment 16
 *
 *	This routine will draw solid zero width line on the display 
 *	in 16 bit display modes.
 *
 */

void
S3CNAME(FillZeroSeg16)(
	struct _GC	*pGC,
	DrawablePtr	pDraw,
	int 		signdx,
	int 		signdy,
	int 		axis,
	int 		x,
	int 		y,
	int 		e,
	int 		e1,
	int 		e2,
	int 		len)
{
	unsigned short 	command;
	int 		cx;
	int		cy;
	unsigned char 	alu;
	unsigned long 	planemask;
	unsigned int 	fg;
	nfbGCPrivPtr 	pGCPriv = NFB_GC_PRIV(pGC);
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	if (len <= 0)
		return;

	alu = pGCPriv->rRop.alu;
	planemask = pGCPriv->rRop.planemask;
	fg = pGCPriv->rRop.fg;

	if (signdx == -1)
		cx = x - len;
	else
		cx = x;
	if (signdy == -1)
		cy = y - len;
	else
		cy = y;

	if (axis == X_AXIS)
		command = S3C_LINE_XN_YN_X;
	else
		command = S3C_LINE_XN_YN_Y;
	if (signdx > 0)
		command |= S3C_CMD_YN_XP_X;
	if (signdy > 0)
		command |= S3C_CMD_YP_XN_X;

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);

	s3cLine(x, y, e, e1, e2, len, S3CNAME(RasterOps)[alu], fg, planemask,
	    command);

	s3cLine(x + 1024, y, e, e1, e2, len, S3CNAME(RasterOps)[alu], fg >> 8,
	    planemask >> 8, command);
}
