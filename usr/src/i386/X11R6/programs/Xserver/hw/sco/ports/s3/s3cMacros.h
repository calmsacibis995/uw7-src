/*
 *	@(#)s3cMacros.h	6.1	3/20/96	10:23:22
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
 *
 * Modification History
 *
 * S008, 30-Jun-93, staceyc
 * 	multiheaded include file now comes in from defs.h
 * S007, 17-May-93, staceyc
 * 	multiheaded S3 card support
 * S006	Mon Apr 05 10:13:54 PDT 1993	hiramc@sco.COM
 *	add macro S3C_WAIT_FOR_IDLE() as per Kevin's changes
 * X005 02-Jan-92 kevin@xware.com
 *      replaces S3C_CURSOR_DATA() macro with S3C_HWCURSOR_DATA() and 
 *	S3C_SWCURSOR_DATA() macros, and removed unused macros.
 * X004 01-Jan-92 kevin@xware.com
 *      updated copyright notice.
 * X003 14-Dec-91 kevin@xware.com
 *	added macro S3C_WAIT_FOR_DATA().
 * X002 08-Dec-91 kevin@xware.com
 *	embedded s3cReadCacheEntry() function into s3cDrawMonoGlyphs(),
 *	moved hash macro to global location.
 * X001 03-Dec-91 kevin@xware.com
 *	changed name of S3C_QSTATADD to S3C_STAT.
 * X000 23-Oct-91 kevin@xware.com
 *	initial source adopted from SCO's sample 8514a source (eff).
 * 
 */

#ifndef _S3C_MACROS_H
#define _S3C_MACROS_H

/*
 * Here is how to concatenate tokens when one of the tokens is a
 * preprocessor define.  See K&R II or the comp.lang.c FAQ to find
 * out why this has to be done this way.  This is ANSI only code.
 */
#define S3C_TYPE_0(head,body) head##body
#define S3C_TYPE_1(head,body) S3C_TYPE_0(head,body)
#define S3CNAME(body) S3C_TYPE_1(S3CHEAD,body)

#ifdef USE_INLINE_CODE
#define S3C_OUT(port, val16) outpw((port), (val16))
#define S3C_OUTB(port, val8) outp((port), (val8))
#define S3C_INB(addr) inp(addr)
#define S3C_INW(addr) (0x0000ffff & inpw(addr))
#else
#define S3C_OUT(port, val16) outw((port), (val16))
#define S3C_OUTB(port, val8) outb((port), (val8))
#define S3C_INB(addr) inb(addr)
#define S3C_INW(addr) inw(addr)
#endif	/*	USE_INLINE_CODE	*/
#define S3C_QSTAT() S3C_INW(S3C_STAT)

#define S3C_CLEAR_QUEUE(n) \
{ \
	register int mask; \
	mask = 0x100 >> (n); \
	while ( mask & S3C_QSTAT() ) \
		;\
}

#define S3C_WAIT_FOR_DATA() \
{ \
	while ( !(S3C_STAT_DATA & S3C_QSTAT()) ) \
		; \
}

				/*	S006	vvv	*/
#define S3C_WAIT_FOR_IDLE() \
{ \
	while( S3C_STAT_BUSY & S3C_QSTAT() ) \
		; \
}
				/*	S006	^^^	*/


#define S3C_SETMODE(mode) S3C_OUT(S3C_SEC_DECODE, (mode))
#define S3C_SETET(c) S3C_OUT(S3C_ERROR_ACC, (c))
#define S3C_SETK1(c) S3C_OUT(S3C_K1, (c))
#define S3C_SETK2(c) S3C_OUT(S3C_K2, (c))

#define S3C_SETCOL0(c) S3C_OUT(S3C_COLOR0, (c))
#define S3C_SETCOL1(c) S3C_OUT(S3C_COLOR1, (c))

#define S3C_SETX0(c) S3C_OUT(S3C_X0, ((c) & 0x07FF))
#define S3C_SETY0(c) S3C_OUT(S3C_Y0, ((c) & 0x07FF))
#define S3C_SETX1(c) S3C_OUT(S3C_X1, ((c) & 0x07FF))
#define S3C_SETY1(c) S3C_OUT(S3C_Y1, ((c) & 0x07FF))

#define S3C_SETLX(c) S3C_OUT(S3C_LX, (c))
#define S3C_SETLY(c) S3C_OUT(S3C_SEC_DECODE, (c))

#define S3C_PLNWENBL(c) S3C_OUT(S3C_PLANE_WE, (c))
#define S3C_PLNRENBL(c) S3C_OUT(S3C_PLANE_RE, (c))

#define S3C_SETFN0(c, alu) S3C_OUT(S3C_FUNC0, ((c) | (alu)))
#define S3C_SETFN1(c, alu) S3C_OUT(S3C_FUNC1, ((c) | (alu)))

#define S3C_SETPAT0(c) S3C_OUT(S3C_SEC_DECODE, (((c) & 0x001E) | 0x8000))
#define S3C_SETPAT1(c) S3C_OUT(S3C_SEC_DECODE, (((c) & 0x001E) | 0x9000))

#define S3C_SETYMIN(c) S3C_OUT(S3C_SEC_DECODE, (((c) & 0x0FFF) | 0x1000))
#define S3C_SETXMIN(c) S3C_OUT(S3C_SEC_DECODE, (((c) & 0x0FFF) | 0x2000))
#define S3C_SETYMAX(c) S3C_OUT(S3C_SEC_DECODE, (((c) & 0x0FFF) | 0x3000))
#define S3C_SETXMAX(c) S3C_OUT(S3C_SEC_DECODE, (((c) & 0x0FFF) | 0x4000))

#define S3C_COMMAND(c) S3C_OUT(S3C_CMD, (c))

#define S3C_PRIVATE_DATA(pScrn) \
((s3cPrivateData_t *)((pScrn)->devPrivates[S3CNAME(ScreenPrivateIndex)].ptr))

#define S3C_HWCURSOR_DATA(pScrn) (&(S3C_PRIVATE_DATA(pScrn)->hw_cursor))
#define S3C_SWCURSOR_DATA(pScrn) (&(S3C_PRIVATE_DATA(pScrn)->sw_cursor))

/*
 * IMPORTANT: the S3C_HASH_TABLE_SIZE must be n to the power of 2 
 * and >= 4096 for this optimized the S3C_GL_HASH() macro to work. 
 *
 * NOTE: this macro works pretty well because of the random nature
 * of the font byteOffset wich is passed as argument b.
 */

#define S3C_GL_HASH(a, b, max)	( ( a + b ) & ( max - 1 ) )

#endif /* _S3C_MACROS_H */

