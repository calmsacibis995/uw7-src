/*
 *	@(#)ctMacros.h	11.1	10/22/97	12:34:53
 *	@(#) ctMacros.h 62.1 97/03/21 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */

#ifndef _CT_MACROS_H
#define _CT_MACROS_H

#ident "@(#) $Id: ctMacros.h 62.1 97/03/21 "

#include "ctRegs.h"

/*******************************************************************************

			Miscellaneous Macros

*******************************************************************************/

#ifdef __USLC__

__asm void CT_OUTD(port, value)
{
%       mem     port, value;
        movl    port, %edx
        movl    value, %eax
        outl    (%dx)
}
#pragma asm partial_optimization CT_OUTD

__asm unsigned long CT_IND(port)
{
%       mem     port;
        movl    port, %edx
        inl     (%dx)
}
#pragma asm partial_optimization CT_IND

#else		/*	__USLC__	*/

#define	CT_OUTD	outd
#define	CT_IND	ind

#endif		/*	__USLC__	*/

extern unsigned char *ct_membase;

/*
 * CT_SROUT():
 *
 * Program a Graphics Controller Register (SRxx) indexed by 'index' with
 * 'value'.
 */
#define CT_SROUT(index, value) {					\
	outb(CT_SRX, (unsigned char)((index) & 0x000000ffL));		\
	outb(CT_SR, (unsigned char)((value) & 0x000000ffL));		\
}

/*
 * CT_SRIN():
 *
 * Read a Graphics Controller Register (SRxx) indexed by 'index' with 'value'.
 */
#define CT_SRIN(index, rval) {						\
	outb(CT_SRX, (unsigned char)((index) & 0x000000ffL));		\
	rval = inb(CT_SR);						\
}


/*
 * CT_CROUT():
 *
 * Program a Graphics Controller Register (CRxx) indexed by 'index' with
 * 'value'.
 */
#define CT_CROUT(index, value) {					\
	outb(CT_CRX, (unsigned char)((index) & 0x000000ffL));		\
	outb(CT_CR, (unsigned char)((value) & 0x000000ffL));		\
}

/*
 * CT_CRIN():
 *
 * Read a Graphics Controller Register (CRxx) indexed by 'index' with 'value'.
 */
#define CT_CRIN(index, rval) {						\
	outb(CT_CRX, (unsigned char)((index) & 0x000000ffL));		\
	rval = inb(CT_CR);						\
}


/*
 * CT_GROUT():
 *
 * Program a Graphics Controller Register (GRxx) indexed by 'index' with
 * 'value'.
 */
#define CT_GROUT(index, value) {					\
	outb(CT_GRX, (unsigned char)((index) & 0x000000ffL));		\
	outb(CT_GR, (unsigned char)((value) & 0x000000ffL));		\
}

/*
 * CT_GRIN():
 *
 * Read a Graphics Controller Register (GRxx) indexed by 'index' with 'value'.
 */
#define CT_GRIN(index, rval) {						\
	outb(CT_GRX, (unsigned char)((index) & 0x000000ffL));		\
	rval = inb(CT_GR);						\
}

/*
 * CT_XROUT():
 *
 * Program an Extension Register (XRxx) indexed by 'index' with 'value'.
 */
#define CT_XROUT(index, value) {					\
	outb(CT_XRX, (unsigned char)((index) & 0x000000ffL));		\
	outb(CT_XR, (unsigned char)((value) & 0x000000ffL));		\
}

/*
 * CT_XRIN():
 *
 * Read an Extension Register (XRxx) indexed by 'index' returning 'rval'.
 */
#define CT_XRIN(index, rval) {						\
	outb(CT_XRX, (unsigned char)((index) & 0x000000ffL));		\
	rval = inb(CT_XR);						\
}

/*
 * CT_FRIN():
 *
 * Read an Flat Panel Register (FRxx) indexed by 'index' returning 'rval'.
 */
#define CT_FRIN(index, rval) {						\
	outb(CT_FRX, (unsigned char)((index) & 0x000000ffL));		\
	rval = inb(CT_FR);						\
}

#if CT_BITS_PER_PIXEL == 8
/*
 * CT_SET_COLOR():
 *
 *	unsigned int index;
 *	unsigned char r;
 *	unsigned char g;
 *	unsigned char b;
 */
#define CT_SET_COLOR(index, r, g, b) {					\
	outb(CT_DACX, (unsigned char)(index));				\
	outb(CT_DACDATA, (r));						\
	outb(CT_DACDATA, (g));						\
	outb(CT_DACDATA, (b));						\
}
#endif /* CT_BITS_PER_PIXEL == 8 */

/*******************************************************************************

			BitBlt Engine Macros

*******************************************************************************/
#define ENABLE_BLT	0x02
#if CT_BITS_PER_PIXEL == 8
#define CT_BLT_MODE	0x00
#endif
#if CT_BITS_PER_PIXEL == 16
#define CT_BLT_MODE	0x10
#endif
#if CT_BITS_PER_PIXEL == 24
#define CT_BLT_MODE	0x20
#endif

#define CT_ENABLE_BITBLT() {						\
	unsigned char xx_val;						\
	CT_XRIN(0x20, xx_val);						\
	xx_val |= CT_BLT_MODE;						\
	xx_val &= ~ENABLE_BLT;						\
	CT_XROUT(0x20, xx_val);						\
}

#define CT_DISABLE_BITBLT() {						\
	unsigned char xx_val;						\
	CT_XRIN(0x20, xx_val);                                          \
	CT_XROUT(0x20, (xx_val | ENABLE_BLT));				\
}

#ifdef CT_BITBLT_DEBUG
#define CT_SET_DEBUG_INFO(cmd) {					\
	CT(BitBltDumpRegisters)(cmd, __FILE__, __LINE__);		\
}
#define CT_RESET_DEBUG_INFO() {						\
	CT(BitBltInfoReset)();						\
}
#else /* CT_BITBLT_DEBUG */
#define CT_SET_DEBUG_INFO(cmd)
#define CT_RESET_DEBUG_INFO()
#endif /* CT_BITBLT_DEBUG */

/*
 * Flush the CPU FIFO by doing a CPU read. Just pass in a pointer to mapped,
 * linear memory.
 */
#ifdef __USLC__

__asm void CT_FLUSH_BITBLT(ptr)
{
%       mem     ptr;
        movl    ptr, %eax
        movl    (%eax), %eax
	cmpl	%eax, 0
}

#else	/*	__USLC__	*/
	/*	Microsoft asm	*/
	/*_asm cmp    eax, 0	*/
#define CT_FLUSH_BITBLT(ptr)	\
{	\
	_asm mov    eax, ptr	\
	_asm mov    eax,[eax]	\
	_asm cmp    eax, 0	\
}
#endif	/*	__USLC__	*/

#define CT_BITBLT_WAIT_VALUE	 0x01000000

#define MAPREG_L(x)	\
	*(unsigned *)((unsigned char *)ct_membase + (x))

#define MAPREG_S(x)	\
	*(unsigned short *)((unsigned char *)ct_membase + (x))

#define MAPREG_C(x)	\
	*(unsigned char *)((unsigned char *)ct_membase + (x))

/*
 * CT_WAIT_FOR_IDLE():
 * 
 * Wait for the BitBlt engine to be idle. Idle when bit-20 of the BitBlt Control
 * register (DR04) is unset. Read the high 16 bits of the register and compare
 * with the high 16 bits of the CT_ACTIVEBIT value.
 */
#define CT_IS_BUSY()	(outb(CT_XRX, 0x20), inb(CT_XR) & 0x1)

#define CT_WAIT_FOR_IDLE() {						\
	unsigned long xx_wait_for_idle_count = 0L;			\
	while (CT_IS_BUSY()) {						\
		if(xx_wait_for_idle_count++ > CT_BITBLT_WAIT_VALUE) {	\
			CT(BitBltTimeoutError)();			\
		}							\
	}								\
	CT_RESET_DEBUG_INFO();						\
}

#define CT_WAIT_FOR_BUFFER()

/*
 * CT_SET_BLTOFFSET():
 *
 * Set offset register (BR00) to:
 *
 *	0-11	source stride in bytes
 *	12-15	RESERVED
 *	16-27	destination stride in bytes
 *	28-31	RESERVED
 *
 * NOTE: assume parameters are passed as 32-bit integers. Source and destination
 * strides are passed in bytes.
 */
#define CT_SET_BLTOFFSET(sstride, dstride) {				\
	MAPREG_S(CT_BLTOFFSET) = (unsigned short)((sstride) & 0x00001fffL); \
	MAPREG_S(CT_BLTOFFSET + 2) = (unsigned short)((dstride) & 0x00001fffL);\
}

/*
 * CT_SET_BLTPATSRC():
 *
 * Set pattern rop register (BR05) to:
 *
 *	0-20	address of onboard 8x8 pattern
 *	21-31	RESERVED
 *
 * NOTE: For an 8BPP pattern, the pattern address MUST be aligned on a 64 byte
 * boundary. For a 16BPP pattern, the pattern MUST be aligned on a 128 byte
 * boundary. For monochrome patterns, the pattern MUST be aligned on an 8 byte
 * boundary.
 */
#if (CT_BITS_PER_PIXEL == 8) || (CT_BITS_PER_PIXEL == 16)
#define CT_SET_BLTPATSRC(src) {						\
	MAPREG_L(CT_BLTSTIP) = ((src) & 0x001fffff);			\
}
#define CT_PATTERN_SEED(y) (						\
	(((y) << 20) & 0x00700000)					\
)
#endif /* (CT_BITS_PER_PIXEL == 8) || (CT_BITS_PER_PIXEL == 16) */
#if (CT_BITS_PER_PIXEL == 24)
/*
 * NOTE: The BitBlt engine does not support 24-bit pattern operations.
 */
#define CT_SET_BLTPATSRC(src)
#define CT_PATTERN_SEED(y) (0)
#endif /* (CT_BITS_PER_PIXEL == 24) */

/*
 * CT_SET_BLTCTRL():
 *
 * Set control register (DR04) to:
 *
 *	0-7	MS Windows rop
 *	8	y-direction (0-bottom_to_top, 1-top_to_bottom)
 *	9	x-direction (0-right_to_left, 1-left_to_right)
 *	10	src data (0-image, 1-fg_color)
 *	11	src depth (0-color, 1-mono font expansion)
 *	12	pattern depth (0-color, 1-mono)
 *	13	bg (0-opaque, 1-transparent)
 *	14-15	blt (0,0-screen->screen, 1,0-mem->screen)
 *	16-18	pattern seed (tile y-offset)
 *	19	pattern type (0-bitmap, 1-solid)
 *	20	status (READ-ONLY, 0-idle, 1-active)
 *	21-23	RESERVED
 *	24-27	buffer status ?
 *	28-31	RESERVED
 */
#define CT_SET_BLTCTRL(control) {					\
	MAPREG_L(CT_BLTCTRL) = control;					\
}

/*
 * CT_SET_BLTSRC():
 *
 * Set BitBlt source register (DR06) to:
 *
 * 	0-20	Source address of byte-aligned block
 *	21-31	RESERVED
 */
#define CT_SET_BLTSRC(src) MAPREG_L(CT_BLTSRC) = ((src) & 0x007fffffL);

/*
 * CT_SET_BLTDST():
 *
 * Set BitBlt destination register (DR06) to:
 *
 * 	0-20	Destination address of byte-aligned block
 *	21-31	RESERVED
 */
#define CT_SET_BLTDST(dst) MAPREG_L(CT_BLTDST) = ((dst) & 0x007fffffL);

/*
 * CT_START_BITBLT():
 *
 * Set command register (BR08) to:
 *
 * 	0-11	w (image bytes-per-scanline)
 *	12-15	RESERVED
 *	16-27	h (image number of scanlines)
 *	28-31	RESERVED
 *
 * NOTE: assume parameters are passed as 32-bit integers. w is passed in
 * pixels and h is passed in scanlines.
 */
#if (CT_BITS_PER_PIXEL == 8)
#define CT_START_BITBLT(w, h) {						\
	unsigned long xx_val = (((((w) * 1) & 0x00001fffL)) |		\
				(((h) & 0x00001fffL) << 16));		\
	CT_SET_DEBUG_INFO(xx_val);					\
	MAPREG_L(CT_BLTCMD) = xx_val;					\
}
#endif /* (CT_BITS_PER_PIXEL == 8) */
#if (CT_BITS_PER_PIXEL == 16)
#define CT_START_BITBLT(w, h) {						\
	unsigned long xx_val = (((((w) * 2) & 0x00001fffL)) |		\
				(((h) & 0x00001fffL) << 16));		\
	CT_SET_DEBUG_INFO(xx_val);					\
	MAPREG_L(CT_BLTCMD) = xx_val;					\
}
#endif /* (CT_BITS_PER_PIXEL == 16) */
#if (CT_BITS_PER_PIXEL == 24)
#define CT_START_BITBLT(w, h) {						\
	unsigned long xx_val = (((((w) * 3) & 0x00001fffL)) |		\
				(((h) & 0x00001fffL) << 16));		\
	CT_SET_DEBUG_INFO(xx_val);					\
	MAPREG_L(CT_BLTCMD) = xx_val;					\
}
#endif /* (CT_BITS_PER_PIXEL == 24) */

#if (CT_BITS_PER_PIXEL == 8)
#define CT_SET_BLTFGCOLOR(fg) {						\
	MAPREG_L(CT_BLTFG) = 						\
		((fg) & 0x000000ffL);					\
}
#define CT_SET_BLTBGCOLOR(bg) {						\
	MAPREG_L(CT_BLTBG) = 						\
		((bg) & 0x000000ffL);					\
}
#endif /* (CT_BITS_PER_PIXEL == 8) */
#if (CT_BITS_PER_PIXEL == 16)
#define CT_SET_BLTFGCOLOR(fg) {						\
	MAPREG_L(CT_BLTFG) = 						\
		((fg) & 0x0000ffffL);					\
}
#define CT_SET_BLTBGCOLOR(bg) {						\
	MAPREG_L(CT_BLTBG) = 						\
		((bg) & 0x0000ffffL);					\
}
#endif /* (CT_BITS_PER_PIXEL == 16) */
#if (CT_BITS_PER_PIXEL == 24)
#define CT_SET_BLTFGCOLOR(fg) {						\
	MAPREG_L(CT_BLTFG) = 						\
		((fg) & 0x00ffffffL);					\
}
#define CT_SET_BLTBGCOLOR(bg) {						\
	MAPREG_L(CT_BLTBG) = 						\
		((bg) & 0x00ffffffL);					\
}
#endif /* (CT_BITS_PER_PIXEL == 24) */

#define CT_BLT_DATAPORT(x)	\
	(unsigned long *)((unsigned char *)(x) + CT_BLTDATA)

/*
 * CT_SET_BLTDATA():
 *
 * Write source data to the BitBlt engine using 32-bit moves.
 *
 *	unsigned char *addr;
 *	void *psrc;
 *	int ndwords;
 *
 * NOTE: Any write to a valid 64300/301 memory address, either in VGA space or 
 * linear address space (if enabled), will be recognized as BitBlt source data
 * and will be routed to the correct address by the BitBlt engine.
 */
#ifdef __USLC__
#if 0
__asm void CT_SET_BLTDATA(baddr, psrc, ndwords) {
%       mem     baddr, psrc, ndwords;
	pushl	%edi
	pushl	%esi
	movl	baddr, %edi	/* load base address into EDI */
	movl	psrc, %esi	/* load src pointer into ESI */
	movl	ndwords, %ecx	/* load number of dwords in src into ECX */
bltloop:
	movl	(%esi), %eax
	movl	%eax, (%edi)
	addl	$4, %esi
	loop bltloop
	/* rep ;	smovl	 * move ECX dwords from ESI to ES:[EDI] */
	popl	%esi
	popl	%edi
}
#else
#define CT_SET_BLTDATA(pdst, psrc, ndwords) {				\
	unsigned long	*pdstp;						\
	unsigned long	*psrcp;						\
	unsigned	i;						\
									\
	pdstp = CT_BLT_DATAPORT(pdst);					\
	psrcp = (unsigned long *)psrc;					\
									\
	for (i = 0 ; i < ndwords ; i++, psrcp++)			\
		*pdstp = *psrcp;					\
	}
#endif

#else	/*	__USLC__	*/
	/*	Microsoft asm	*/
#define CT_SET_BLTDATA(baddr, psrc, ndwords) \
{	\
	_asm mov	eax, edi		\
	_asm mov	ebx, esi		\
	_asm mov	edi, baddr		\
	_asm mov	esi, psrc		\
	_asm mov	ecx, ndwords		\
	_asm rep	mov [esi], [edi]	\
	_asm mov	edi, eax		\
	_asm mov	esi, ebx		\
}

#endif	/*	__USLC__	*/

#define CT_MONO_ALIGN_BIT	1
#define CT_MONO_ALIGN_BYTE	2
#define CT_MONO_ALIGN_WORD	3
#define CT_MONO_ALIGN_DWORD	4
#define CT_MONO_ALIGN_QWORD	5

#define CT_SET_MONO_CONTROL(align, skip, right, left)			\
	MAPREG_L(CT_BLTMONOCTL) = 					\
		(((align) << 24) |					\
		((skip) << 16) |					\
		((right) << 8) |					\
		(left))

#define	CT_BLTMONOCTL	0x40000C	/* BR03 BitBlt monochrome src cntrl */

/*
 * CT_EXPAND_BYTESn():
 *
 * Write 1-bit data to the mapped memory address. We don't have to increment
 * the destination pointer here since any write to a valid 64300/301 memory
 * address, either in VGA space or linear address space (if enabled), will be
 * recognized as BitBlt source data and will be routed to the correct address by
 * the BitBlt engine. The BitBlt engine must receive an even number of dwords,
 * and the source data must be written in 64-bit dwords. If there are less than
 * 64 bits in width of source data, zero-pad the values.
 */
#define CT_EXPAND_BYTES1(pdst, psrc) {					\
	*(pdst) =   CT_CHAR2LONG((psrc)[0]);				\
}

#define CT_EXPAND_BYTES2(pdst, psrc) {					\
	*(pdst) = ((CT_CHAR2LONG((psrc)[1]) <<  8) |			\
		    CT_CHAR2LONG((psrc)[0]));				\
}

#define CT_EXPAND_BYTES3(pdst, psrc) {					\
	*(pdst) = ((CT_CHAR2LONG((psrc)[2]) << 16) |			\
		   (CT_CHAR2LONG((psrc)[1]) <<  8) |			\
		    CT_CHAR2LONG((psrc)[0]));				\
}

#define CT_EXPAND_BYTES4(pdst, psrc) {					\
	*(pdst) = ((CT_CHAR2LONG((psrc)[3]) << 24) |			\
		   (CT_CHAR2LONG((psrc)[2]) << 16) |			\
		   (CT_CHAR2LONG((psrc)[1]) <<  8) |			\
		    CT_CHAR2LONG((psrc)[0]));				\
}

#define CT_EXPAND_ROUNDED(pdst)	{					\
	*(pdst) = 0;							\
}

#endif /* _CT_MACROS_H */
