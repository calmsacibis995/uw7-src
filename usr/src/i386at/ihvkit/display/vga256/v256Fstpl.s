	.ident	"@(#)ihvkit:display/vga256/v256Fstpl.s	1.2"
	.file 	"v256Fstpl.s"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

#define	__ASSEMBLER__
#include "v256as.h"

/*
 * MODULE: v256Fstpl.s
 * 
 * DESCRIPTION:
 *
 *	Assembly language support for the stippling code.
 *
 * NOTES:
 *
 *	These routines are passed in pointers to a bitmap representing
 * the stipple to be drawn on screen.  A count of the number of bytes
 * in the stipple is passed.  Thus we end up processing 8 times the
 * count pixels on the screen.
 * The destination pointer is not guaranteed to be aligned to long
 * word boundaries.
 * 
 * CAVEATS:
 *	This code assumes each pixel is a byte wide.  
 *
 *	No checking is done for crossings of the vga segments.  Its
 * the responsibility of the caller to appropriately side-step
 * disaster. 
 *
 *	These routines work only for GXcopy mode.
 */

/*
 *
 * v256FOpaqueStipple(unsigned long *dstPtr,  char *srcBits,
 * 		int nSrcBytes); 
 *
 * Opaque stippler for the vga256 display library.  
 * 
 * Expected State:
 *	On entry to this function, the expanded stipple array
 * `cfb8StippleXor' should be set up with the correct foreground and
 * background colors.
 *
 * Strategy:
 * 	The strategy here is to use the expanded stipple array in
 * `cfb8StippleXor[]' to provide us with the appropriate foreground
 * and background pixels to write to screen.  We run up to the first
 * longword boundary and then try to do long word aligned accesses
 * till we exhaust the source bitmap.
 * 
 * Register Usage:
 * 	
 *	%eax 	:  pixels targeted for the destination aligned to long words
 *	%edx	:  pixels from the expanded stipple array
 *	%ecl	:  shift count for misaligned destinations
 *	%ech	:  holds stipple bits (8 stipple bits)
 *	%ebx	:  temporary
 * 	%edi	:  virtual address of destination
 *	%esi	:  pointer to stipple bits
 */

#define	dstPtr		8(%ebp)
#define	srcBitsPtr	12(%ebp)
#define nSrcBytes	16(%ebp)
#define nPixels		16(%ebp)
	/* We reuse the same stack location as `nSrcBytes' */
/*
 * locals to this procedure
 */
#define savedBits	-16(%ebp)
#define shiftCount	-20(%ebp)

#ifdef	PROFILING
	.align	4
	.data
.OSProfileTemp:	.long	0
#endif	/* PROFILING */

	.globl	v256FOpaqueStipple
	.text
	.align	4
v256FOpaqueStipple:
	/*
 	 * Prologue
	 */
	pushl	%ebp
	movl	%esp,		%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx

	/*
	 * Reserve space for locals
	 */
	subl	$16,		%esp

#ifdef	PROFILING
	movl	$.OSProfileTemp,	%edx
	call	_mcount
#endif	/* PROFILING */

	cld

	movl	nSrcBytes,	%ecx
	jcxz	.OSReturn

	movl	srcBitsPtr,	%esi
	movl	dstPtr,		%edi

	/* No more use of %ebp : reuse as a gen register */

	shll	$3,		%ecx
	movl	%ecx,		%ebp

	/* get the first source byte */
	xorl	%eax,		%eax	/* clear higher bits */
	lodsb
	movl	%eax,		%ebx	/* save this byte */

	/* get the expanded stipple pixels */
	andl	$0x0F,		%eax
	movl	cfb8StippleXor(,%eax,4),	%eax

	/* run up to the long word */
	movl	%edi,		%ecx
	andl	$0x03,		%ecx
	movl	%ecx,		%edx
 	
	/* adjust the number of pixels to be drawn */
	negl	%ecx
	addl	$4,		%ecx
	subl	%ecx,		%ebp

	shll	$3,		%ecx	/* convert to shift count */
	
	jmp	*.OSStartStippleCodeTable(,%edx,4)

.OSByteLoop:
	subl	$4,		%ebp
	jle	.OSEndSection

	xorl	%eax,		%eax
	lodsb
	movl	%eax,		%ebx

	/* Stipple the lower 4 bits */
	andl	$0x0F,		%eax

	movl	cfb8StippleXor(,%eax,4),	%eax

	shrdl	%eax,		%edx	/* overlap with the previous
					 * word of stipple pixels
					 */
	xchgl	%eax,		%edx	/* tuck away a copy in %edx */
	stosl			/* store 4 pixels */
.OSMidByteLoop:
	subl	$4,		%ebp
	jle	.OSEndSection

	/* Stipple the upper 4 bits */
	shrb	$0x04,		%bl

	movl	cfb8StippleXor(,%ebx,4),	%eax

	shrdl	%eax,		%edx
	xchgl	%eax,		%edx
	stosl			/* store 4 pixels */

	/* finished processing a byte from the stipple */	
	jmp	.OSByteLoop
		
	/*
	 *  Finish off the last 1-4 pixels.  %edx has the stipple
	 * pixels to be written out
	 */
	
	.align	4
.OSEndSection:
.OSAlignedEndSection:
	addl	$4,		%ebp
	jmp	*.OSEndStippleCodeTable(,%ebp,4)

	.align	4
.OSAlignedByteLoop:
	subl	$4,	%ebp
	jle	.OSAlignedEndSection
	lodsb
	movb	%al,	%bl
	andl	$0x0F,	%eax
	movl	(%edx,%eax,4),	%eax
	stosl
.OSAlignedMidByteLoop:
	subl	$4,	%ebp
	jle	.OSAlignedEndSection
	shrl	$4,	%ebx
	movl	(%edx,%ebx,4),	%eax
	stosl
	jmp	.OSAlignedByteLoop		

	/*
	 * Epilogue
	 */
	.align	4
.OSReturn:
	addl	$16,		%esp
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret


/*
 * Code Tables
 */
	.data
.OSStartStippleCodeTable:
	.long	.OSStart4, .OSStart3, .OSStart2, .OSStart1
.OSEndStippleCodeTable:
	.long	.OSEnd0, .OSEnd1, .OSEnd2, .OSEnd3, .OSEnd4

/*
 * Write out the starting pixels.  %eax has the pixels to be written
 * out.  %edi points to the destination.  The code will leave %edi
 * aligned to a longword boundary.
 *
 * 	-----------------
 *	| 0 | 1 | 2 | 3 |	OFFSETS of %edi
 *	-----------------
 * 	LSB 		MSB (Screen Right)
 *	-----------------
 *	| 4 | 3 | 2 | 1 |	# PIXELS TO BE WRITTEN
 *	-----------------
 */

	.align	4
.OSStart4:	/* write out four pixels */
	stosl
	leal	cfb8StippleXor,	%edx
	jmp	.OSAlignedMidByteLoop
	.align	4
.OSStart3:	/* write out 3 pixels */
	movl	%eax,		%edx
	movb	%al,		0(%edi)
	shrl	$8,		%eax
	movw	%ax,		1(%edi)	
	addl	$3,		%edi
	jmp	.OSMidByteLoop
	.align	4
.OSStart2:
	movl	%eax,		%edx
	movw	%ax,		0(%edi)
	addl	$2,		%edi
	jmp	.OSMidByteLoop
	.align	4
.OSStart1:	/* write one pixel */
	movl	%eax,		%edx
	movb	%al,		0(%edi)
	incl	%edi
	jmp	.OSMidByteLoop
			
/*
 * The last few pixels on the line.  %edi will be long word aligned at
 * this point.  Note that the MSB's of %edx need to be written out.
 *
 *	-----------------
 *	| 0 | 1 | 2 | 3 |  OFFSETS
 *	-----------------
 *	LSB		MSB
 *      -----------------
 *	| 1 | 2 | 3 | 4 |  PIXELS TO BE WRITTEN
 *	-----------------
 */
	.align	4
.OSEnd4:	/* write out 4 pixels */
	shrb	$4,		%bl

	movl	cfb8StippleXor(,%ebx,4),	%eax
	stosl
	jmp	.OSReturn
	.align	4
.OSEnd3:	/* write out 3 pixels */
	shrl	$8,		%edx
	movw	%dx,		0(%edi)
	shrl	$16,		%edx
	movb	%dl,		2(%edi)
	jmp	.OSReturn
	.align	4
.OSEnd2:	/* write out 2 pixels */
	shrl	$16,		%edx
	movw	%dx,		0(%edi)
	jmp	.OSReturn
	.align	4
.OSEnd1:
	shrl	$24,		%edx
	movb	%dl,		0(%edi)
	jmp	.OSReturn
	.align	4
.OSEnd0:	/* write out nothing */
	jmp	.OSReturn


/*
 * Local Variables:
 * mode: text
 * eval: (auto-fill-mode 1)
 * End:
 */
