	.file	"effBlkOut.s"
/*									*/
/*	@(#) effBlkOut.s 11.1 97/10/22
/*									*/
/* Modification History							*/
/*									*/
/* S002, 20-Aug-91, staceyc                                             */
/*	removed across plane block out     				*/
/* S001, 05-Aug-91, staceyc						*/
/* 	added across planes block out					*/
/* S000, 25-Jun-91, staceyc						*/
/* 	incorporated from ibm source					*/

/*									*/
/*	void								*/
/*	effBlockOutW( unsigned short *values, int n )			*/
/*									*/
/*	This routine destroys the contents of %eax, %ecx & %edx		*/
/*	All other registers are preserved.				*/
/*									*/

	.def	effBlockOutW;	.val	effBlockOutW;
	.scl	2;	.type	055;	.endef
	.globl	effBlockOutW
.text
.align 4
	movw	%ax, %ax	/* Aligning "nop" */
effBlockOutW:
	xchgl	%esi, 4(%esp)	/* get "values" */
	xchgl	%ebx, 8(%esp)	/* get "n" */
/* Bottom half of %edx is I/O addr of the queue status port	0xDAE8 */
/* Top half of %edx is I/O addr of the variable-data port	0xE2E8 */
	movl	$0xE2E8DAE8, %edx

.testQ1:	/* Now %dx points to the queue status port */
	inw	(%dx)		/* Read The Queue Status Port */
	xorb	$0xFF, %al	/* same as logical negation, but set flags */
	jz	.testQ1		/* If no slots are available, spin & burn */

	bsfl	%eax, %ecx
	rorl	$16, %edx	/* Point %dx at the VAR-DATA I/O port */
	subb	$8, %cl		/* The number of empty slots -- NEGATED !! */
	negb	%cl		/* The number of empty slots !! */
	subl	%ecx, %ebx	/* The number of words remaining after this */
	jle	.final_batch
	rep
	outsw	/* (%dx), (%esi) */
	rorl	$16, %edx	/* Point %dx at the Queue Status I/O port */
	jmp	.testQ1

.final_batch:
	add	%ebx, %ecx	/* The number of words left !! */
	jz	.done1		/* Are there any words left ?? */
	rep
	outsw	/* (%dx), (%esi) */
.done1:
	movl	4(%esp), %esi
	movl	8(%esp), %ebx
	ret
	.def	effBlockOutW;	.val	.;	.scl	-1;	.endef
