/
/	@(#)s3cBlkIn.S	6.2	3/20/96	12:20:32
/
/ 	Copyright (C) Xware, 1991-1992.
/
/ 	The information in this file is provided for the exclusive use
/	of the licensees of Xware. Such users have the right to use, 
/	modify, and incorporate this code into other products for 
/	purposes authorized by the license agreement provided they 
/	include this notice and the associated copyright notice with 
/	any such product.
/
/
/ Modification History:
/
/ S001, 20-Mar-96, hiramc
/	remove the lines with .def .val .scl .type .endef
/	directives to enable assemble as -b elf
/ S000, 17-May-93, staceyc
/ 	this file is now "pre-processed" by sed
/ X002 01-Jan-92 kevin@xware.com
/	updated copyright notice.
/ X001 14-Dec-91 kevin@xware.com
/	removed gating output by Q capacity, added checking for data available.
/ X000 29-Nov-91 kevin@xware.com
/	moved from 'C' in s3cSlab.c, initial source adopted from s3cBlkOut.s.
/ 

	.file	"s3cBlkIn.s"

/*									*/
/*	void								*/
/*	s3cBlockInW( unsigned short *data, int n )			*/
/*									*/
/*	This routine destroys the contents of %eax, %ecx & %edx		*/
/*	All other registers are preserved.				*/
/*									*/
/*	Moving this routine to assembly speeded up x11perf:		*/
/*									*/
/*		-getimage100	40%					*/
/*		-getimage500	42%					*/
/*									*/

	.globl	S3CNAMEBlockInW
.align 4
	movw	%ax, %ax	/* Aligning "nop" */
S3CNAMEBlockInW:
	xchgl	%edi, 4(%esp)	/* get "values" */
	movl	8(%esp), %ecx	/* get "n" */
/* Bottom half of %edx is I/O addr of the queue status port	0x9AE8 */
/* Top half of %edx is I/O addr of the variable-data port	0xE2E8 */
	movl	$0xE2E89AE8, %edx

.testQ1:			/* Now %dx points to the queue status port */
	inw	(%dx)		/* Read The Queue Status Port 		*/
	andw	$0x0100, %ax	/* same as logical negation, but set flags */
	jz	.testQ1		/* If no data is available		*/

	rorl	$16, %edx	/* Point %dx at the VAR-DATA I/O port 	*/

	rep
	insw			/* (%dx), (%edi) */
	movl	4(%esp), %edi
	ret

