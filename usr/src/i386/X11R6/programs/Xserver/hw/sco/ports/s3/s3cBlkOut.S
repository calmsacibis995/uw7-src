/
/	@(#)s3cBlkOut.S	6.2	3/20/96	12:20:33
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
/ S004, 20-Mar-96, hiramc
/	remove the lines with .def .val .scl .type .endef
/	directives to enable assemble as -b elf
/ S003, 17-May-93, staceyc
/	this file is now "pre-processed" by sed
/ S005 27-Apr-93 hiramc@sco.com
/	changed queue usage per new requirements for 86C801, 86C805, and 86C928.
/	from kevin's update
/ X004 01-Jan-92 kevin@xware.com
/	updated copyright notice.
/ X003 14-Dec-91 kevin@xware.com
/	removed gating output by Q capacity.
/ X002 01-Nov-91 kevin@xware.com
/	added s3cBlockOutB() function for KLUDGEs in s3cMono.c (temporary).
/ X001 23-Oct-91 kevin@xware.com
/	changed Q stat I/O address from 0xDAE8 to 0x9AE8.
/ X000 23-Oct-91 kevin@xware.com
/	initial source adopted from SCO's sample 8514a source (eff).
/ 

	.file	"s3cBlkOut.s"
/*									*/
/*	@(#) effBlkOut.s 1.3 91/08/20 						*/
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
/*	s3cBlockOutW( unsigned short *values, int n )			*/
/*									*/
/*	This routine destroys the contents of %eax, %ecx & %edx		*/
/*	All other registers are preserved.				*/
/*									*/

	.globl	S3CNAMEBlockOutW
.align 4
	movw	%ax, %ax	/* Aligning "nop" */
S3CNAMEBlockOutW:
	xchgl	%esi, 4(%esp)	/* get "values" */
	movl	8(%esp), %ecx	/* get "n" */
	movl	$0xE2E8, %edx	/*	addr of variable-data port	S005 */
				/*	code removed waiting for status S005 */
	rep
	outsw			/* (%dx), (%esi) */
	movl	4(%esp), %esi
	ret

/									*/
/	void								*/
/	s3cBlockOut( unsigned char *values, int n )			*/
/									*/
/	This routine destroys the contents of %eax, %ecx & %edx		*/
/	All other registers are preserved.				*/
/									*/

	.globl	S3CNAMEBlockOutB
.align 4
	movw	%ax, %ax	/* Aligning "nop" */
S3CNAMEBlockOutB:
	xchgl	%esi, 4(%esp)	/* get "values" */
	movl	8(%esp), %ecx	/* get "n" */
	movl	$0xE2E8, %edx	/*	addr of variable-data port S005 */
				/*	code removed waiting for status S005 */
	rep
	outsb			/* (%dx), (%esi) */
	movl	4(%esp), %esi
	ret
