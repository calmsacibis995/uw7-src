/*
 *	@(#)ctMacros.h	11.1	10/22/97	12:10:46
 *	@(#) ctMacros.h 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1994-1996.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */
/*
 *	SCO Modifications
 *
 *	S002 Wed Jun 01 10:32:56 PDT 1994	hiramc@sco.COM
 *	- Must have different _asm directives for the Microsoft
 *		compiler
 *	S001 Tue May 31 14:10:45 PDT 1994	hiramc@sco.COM
 *	- Use the OUTD, IND macros only on USL compiler.
 */

#ifndef _CT_MACROS_H
#define _CT_MACROS_H

#ident "@(#) $Id$"

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

#define CT_ENABLE_BITBLT() {						\
	unsigned char xx_val;						\
	CT_XRIN(0x03, xx_val);						\
	CT_XROUT(0x03, (xx_val | 0x02));				\
}

#define CT_DISABLE_BITBLT() {						\
	unsigned char xx_val;						\
	CT_XRIN(0x03, xx_val);						\
	CT_XROUT(0x03, (xx_val & ~0x02));				\
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
	/*	Microsoft asm	S002 */
	/*_asm cmp    eax, 0	*/
#define CT_FLUSH_BITBLT(ptr)	\
{	\
	_asm mov    eax, ptr	\
	_asm mov    eax,[eax]	\
	_asm cmp    eax, 0	\
}
#endif	/*	__USLC__	*/

#define CT_BITBLT_WAIT_VALUE	 0x01000000

/*
 * CT_WAIT_FOR_IDLE():
 * 
 * Wait for the BitBlt engine to be idle. Idle when bit-20 of the BitBlt Control
 * register (DR04) is unset. Read the high 16 bits of the register and compare
 * with the high 16 bits of the CT_ACTIVEBIT value.
 */
#define CT_IS_BUSY() (CT_IND(CT_BLTCTRL) & CT_ACTIVEBIT)

#define CT_WAIT_FOR_IDLE() {						\
	unsigned long xx_wait_for_idle_count = 0L;			\
	while (CT_IS_BUSY()) {						\
		if(xx_wait_for_idle_count++ > CT_BITBLT_WAIT_VALUE) {	\
			CT(BitBltTimeoutError)();			\
		}							\
	}								\
	CT_RESET_DEBUG_INFO();						\
}

#ifdef NOT_NEEDED
/*
 * CT_WAIT_FOR_BUFFER():
 * 
 * Wait for the BitBlt engine FIFO to be available. As new data is written to
 * the FIFO, dwords already in the FIFO are processed. It is possible to fill
 * the FIFO faster than the engine can process data (particularly in doing
 * READ/MODIFY/WRITE operations with mono image data). This situation results
 * in a system crash! When the FIFO is full, the Buffer Status Bit (DR04[24])
 * is 0. When the buffer status bit is set (1), there is at least one available
 * dword in the buffer. 
 */
#define CT_IS_FULL() ((CT_IND(CT_BLTCTRL) & CT_BUFFERBIT) == 0x00000000L)

#define CT_WAIT_FOR_BUFFER() {						\
	unsigned long xx_wait_for_buffer_count = 0L;			\
	while (CT_IS_FULL()) {						\
		if(xx_wait_for_buffer_count++ > CT_BITBLT_WAIT_VALUE) {	\
			CT(BitBltTimeoutError)();			\
		}							\
	}								\
}
#else /* NOT_NEEDED */
#define CT_WAIT_FOR_BUFFER()
#endif /* NOT_NEEDED */

/*
 * CT_SET_BLTOFFSET():
 *
 * Set offset register (DR00) to:
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
	outw(CT_BLTOFFSET, (unsigned short)((sstride) & 0x00000fffL));	\
	outw(CT_BLTOFFSET+2, (unsigned short)((dstride) & 0x00000fffL));\
}

/*
 * CT_SET_BLTPATSRC():
 *
 * Set pattern rop register (DR01) to:
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
	CT_OUTD(CT_BLTSTIP, ((src) & 0x001fffff));			\
}
#define CT_PATTERN_SEED(y) (						\
	(((y) << 16) & 0x00070000)					\
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
	CT_OUTD(CT_BLTCTRL, (control));					\
}

/*
 * CT_SET_BLTSRC():
 *
 * Set BitBlt source register (DR06) to:
 *
 * 	0-20	Source address of byte-aligned block
 *	21-31	RESERVED
 */
#define CT_SET_BLTSRC(src) CT_OUTD(CT_BLTSRC, ((src) & 0x001fffffL));

/*
 * CT_SET_BLTDST():
 *
 * Set BitBlt destination register (DR06) to:
 *
 * 	0-20	Destination address of byte-aligned block
 *	21-31	RESERVED
 */
#define CT_SET_BLTDST(dst) CT_OUTD(CT_BLTDST, ((dst) & 0x001fffffL));

/*
 * CT_START_BITBLT():
 *
 * Set command register (DR07) to:
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
	unsigned long xx_val = (((((w) * 1) & 0x00000fffL)) |		\
				(((h) & 0x00000fffL) << 16));		\
	CT_SET_DEBUG_INFO(xx_val);					\
	CT_OUTD(CT_BLTCMD, xx_val);					\
}
#endif /* (CT_BITS_PER_PIXEL == 8) */
#if (CT_BITS_PER_PIXEL == 16)
#define CT_START_BITBLT(w, h) {						\
	unsigned long xx_val = (((((w) * 2) & 0x00000fffL)) |		\
				(((h) & 0x00000fffL) << 16));		\
	CT_SET_DEBUG_INFO(xx_val);					\
	CT_OUTD(CT_BLTCMD, xx_val);					\
}
#endif /* (CT_BITS_PER_PIXEL == 16) */
#if (CT_BITS_PER_PIXEL == 24)
#define CT_START_BITBLT(w, h) {						\
	unsigned long xx_val = (((((w) * 3) & 0x00000fffL)) |		\
				(((h) & 0x00000fffL) << 16));		\
	CT_SET_DEBUG_INFO(xx_val);					\
	CT_OUTD(CT_BLTCMD, xx_val);					\
}
#endif /* (CT_BITS_PER_PIXEL == 24) */

#if (CT_BITS_PER_PIXEL == 8)
#define CT_SET_BLTFGCOLOR(fg) {						\
	CT_OUTD(CT_BLTFG,						\
		(((fg) & 0x000000ffL) |					\
		 (((fg) << 8) & 0x0000ff00L) |				\
		 (((fg) << 16) & 0x00ff0000L) |				\
		 (((fg) << 24) & 0xff000000L)));			\
}
#define CT_SET_BLTBGCOLOR(bg) {						\
	CT_OUTD(CT_BLTBG,						\
		(((bg) & 0x000000ffL) |					\
		 (((bg) << 8) & 0x0000ff00L) |				\
		 (((bg) << 16) & 0x00ff0000L) |				\
		 (((bg) << 24) & 0xff000000L)));			\
}
#endif /* (CT_BITS_PER_PIXEL == 8) */
#if (CT_BITS_PER_PIXEL == 16)
#define CT_SET_BLTFGCOLOR(fg) {						\
	CT_OUTD(CT_BLTFG,						\
		(((fg) & 0x0000ffffL) | (((fg) << 16) & 0xffff0000)));	\
}
#define CT_SET_BLTBGCOLOR(bg) {						\
	CT_OUTD(CT_BLTBG,						\
		(((bg) & 0x0000ffffL) | (((bg) << 16) & 0xffff0000)));	\
}
#endif /* (CT_BITS_PER_PIXEL == 16) */
#if (CT_BITS_PER_PIXEL == 24)
/*
 * NOTE: The BitBlt engine does not support 24-bit solid fills using foreground
 * or background colors!
 */
#define CT_SET_BLTFGCOLOR(fg) 
#define CT_SET_BLTBGCOLOR(bg)
#endif /* (CT_BITS_PER_PIXEL == 24) */

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
__asm void CT_SET_BLTDATA(baddr, psrc, ndwords) {
%       mem     baddr, psrc, ndwords;
	movl	%edi, %eax	/* save EDI */
	movl	%esi, %ebx	/* save ESI */
	movl	baddr, %edi	/* load base address into EDI */
	movl	psrc, %esi	/* load src pointer into ESI */
	movl	ndwords, %ecx	/* load number of dwords in src into ECX */
	rep ;	smovl		/* move ECX dwords from ESI to ES:[EDI] */
	movl	%eax, %edi	/* restore EDI */
	movl	%ebx, %esi	/* restore ESI */
}

#else	/*	__USLC__	*/
	/*	Microsoft asm	S002 */
#define CT_SET_BLTDATA(baddr, psrc, ndwords) \
{	\
	_asm mov	eax, edi	\
	_asm mov	ebx, esi	\
	_asm mov	edi, baddr	\
	_asm mov	esi, psrc	\
	_asm mov	ecx, ndwords	\
	_asm rep	movsd	\
	_asm mov	edi, eax	\
	_asm mov	esi, ebx	\
}

#endif	/*	__USLC__	*/

/*
 * CT_EXPAND_BYTESn():
 *
 * Write 1-bit data to the mapped memory address. We don't have to increment
 * the destination pointer here since any write to a valid 64300/301 memory
 * address, either in VGA space or linear address space (if enabled), will be
 * recognized as BitBlt source data and will be routed to the correct address by
 * the BitBlt engine. The BitBlt engine must receive an even number of dwords,
 * and the source data must be written in 32-bit dwords. If there are less than
 * 32 bits in width of source data, zero-pad the values.
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

#if 0
	/*
	 * Will this code be faster than the shifting and or'ing, above?
	 */
	unsigned char *pdst = (unsigned char *)ctPriv->fbPointer;

#define CT_EXPAND_BYTES1(pdst, psrc) {					\
	(pdst)[0] = MSBIT_SWAP((psrc)[0]);				\
	(pdst)[1] = 0;							\
	(pdst)[2] = 0;							\
	(pdst)[3] = 0;							\
}

#define CT_EXPAND_BYTES2(pdst, psrc) {					\
	(pdst)[0] = MSBIT_SWAP((psrc)[0]);				\
	(pdst)[1] = MSBIT_SWAP((psrc)[1]);				\
	(pdst)[2] = 0;							\
	(pdst)[3] = 0;							\
}

#define CT_EXPAND_BYTES3(pdst, psrc) {					\
	(pdst)[0] = MSBIT_SWAP((psrc)[0]);				\
	(pdst)[1] = MSBIT_SWAP((psrc)[1]);				\
	(pdst)[2] = MSBIT_SWAP((psrc)[2]);				\
	(pdst)[3] = 0;							\
}

#define CT_EXPAND_BYTES4(pdst, psrc) {					\
	(pdst)[0] = MSBIT_SWAP((psrc)[0]);				\
	(pdst)[1] = MSBIT_SWAP((psrc)[1]);				\
	(pdst)[2] = MSBIT_SWAP((psrc)[2]);				\
	(pdst)[3] = MSBIT_SWAP((psrc)[3]);				\
}
#endif


#endif /* _CT_MACROS_H */
