/* 
 *	@(#) effMacros.h 11.1 97/10/22
 *
 * SCO Modification History
 *
 * S012, 19-Jan-92, chrissc
 *	added macros for ATI clip registers.
 * S011, 20-Nov-92, staceyc
 * 	remove all remnants of third party hardware mods
 * S010, 04-Sep-92, staceyc
 * 	assert macro added
 * S009, 26-Sep-91, staceyc
 * 	macros for saving VGA 8514 DAC state
 * S008, 28-Aug-91, staceyc
 * 	reworked command queue use
 * S007, 01-Aug-91, staceyc
 * 	code to access cursor screen private data
 * S006, 28-Jun-91, staceyc
 * 	added screen private access macro
 * S005, 26-Jun-91, staceyc
 * 	replace clearqueue debugging call with inline busy loop
 * S004, 21-Jun-91, staceyc
 * 	added code for Bres line drawing
 * S003, 21-Jun-91, staceyc
 * 	added some macros for bitblt
 * S002, 20-Jun-91, staceyc
 * 	added code to help with debugging
 * S001, 18-Jun-91, staceyc
 * 	get read/draw image and cmap code happening
 * S000, 14-Jun-91, staceyc
 * 	created
 */

#ifndef _EFF_MACROS_H
#define _EFF_MACROS_H

#define EFF_OUT(port, val16) outw((port), (val16))
#define EFF_OUTB(port, val8) outb((port), (val8))
#define EFF_INB(addr) inb(addr)
#define EFF_INW(addr) inw(addr)
#define EFF_CRTCMODE(c) outb(0x22E8, (c))
#define EFF_QSTAT() EFF_INW(EFF_QSTATADD)
#define EFF_CLEAR_QUEUE(n) \
{ \
	register int mask; \
	mask = 0x100 >> (n); \
	while (mask & EFF_QSTAT()); \
}
#define	EFF_GPBUSY 0x0200	/* graphics processor busy (qstat reg) */
#define	EFF_GPDONE() \
{ \
	while ((EFF_GPBUSY|0x01) & EFF_QSTAT()) \
		; \
}

#define EFF_SETMODE(mode) EFF_OUT(EFF_SEC_DECODE, (mode))
#define EFF_SETET(c) EFF_OUT(EFF_ERROR_ACC, (c))
#define EFF_SETK1(c) EFF_OUT(EFF_K1, (c))
#define EFF_SETK2(c) EFF_OUT(EFF_K2, (c))

#define EFF_SETCOL0(c) EFF_OUT(EFF_COLOR0, (c))
#define EFF_SETCOL1(c) EFF_OUT(EFF_COLOR1, (c))

#define EFF_SETX0(c) EFF_OUT(EFF_X0, ((c) & 0x07FF))
#define EFF_SETY0(c) EFF_OUT(EFF_Y0, ((c) & 0x07FF))
#define EFF_SETX1(c) EFF_OUT(EFF_X1, ((c) & 0x07FF))
#define EFF_SETY1(c) EFF_OUT(EFF_Y1, ((c) & 0x07FF))

#define EFF_SETLX(c) EFF_OUT(EFF_LX, (c))
#define EFF_SETLY(c) EFF_OUT(EFF_SEC_DECODE, (c))

#define EFF_PLNWENBL(c) EFF_OUT(EFF_PLANE_WE, (c))
#define EFF_PLNRENBL(c) EFF_OUT(EFF_PLANE_RE, (c))

#define EFF_SETFN0(c, alu) EFF_OUT(EFF_FUNC0, ((c) | (alu)))
#define EFF_SETFN1(c, alu) EFF_OUT(EFF_FUNC1, ((c) | (alu)))

#define EFF_SETPAT0(c) EFF_OUT(EFF_SEC_DECODE, (((c) & 0x001E) | 0x8000))
#define EFF_SETPAT1(c) EFF_OUT(EFF_SEC_DECODE, (((c) & 0x001E) | 0x9000))

#define EFF_SETYMIN(c) EFF_OUT(EFF_SEC_DECODE, (((c) & 0x0FFF) | 0x1000))
#define EFF_SETXMIN(c) EFF_OUT(EFF_SEC_DECODE, (((c) & 0x0FFF) | 0x2000))
#define EFF_SETYMAX(c) EFF_OUT(EFF_SEC_DECODE, (((c) & 0x0FFF) | 0x3000))
#define EFF_SETXMAX(c) EFF_OUT(EFF_SEC_DECODE, (((c) & 0x0FFF) | 0x4000))

#define ATI_SETYMIN(c) EFF_OUT(ATI_EXT_SCISSOR_T, (((c) & 0x0FFF)))  /* S012 */
#define ATI_SETXMIN(c) EFF_OUT(ATI_EXT_SCISSOR_L, (((c) & 0x0FFF)))  /* S012 */
#define ATI_SETYMAX(c) EFF_OUT(ATI_EXT_SCISSOR_B, (((c) & 0x0FFF)))  /* S012 */
#define ATI_SETXMAX(c) EFF_OUT(ATI_EXT_SCISSOR_R, (((c) & 0x0FFF)))  /* S012 */

#define EFF_COMMAND(c) EFF_OUT(EFF_QSTATADD, (c))

#define EFF_PRIVATE_DATA(pScrn) \
	((effPrivateData_t *)((pScrn)->devPrivates[effScreenPrivateIndex].ptr))

#define EFF_CURSOR_DATA(pScrn) (&(EFF_PRIVATE_DATA(pScrn)->cursor))

#define EFF_ASSERT(bool_cond) (!(bool_cond) ? \
	(ErrorF("EFF: ASSERTION failed in %s at line %d\n", __FILE__, \
	    __LINE__), 1) : \
	0)

#endif /* _EFF_MACROS_H */

