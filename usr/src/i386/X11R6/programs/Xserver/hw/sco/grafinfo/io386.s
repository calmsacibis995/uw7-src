/
/	@(#) io386.s 12.1 95/05/09 
/
/	Copyright (C) The Santa Cruz Operation, 1991-1992.
/	This Module contains Proprietary Information of
/	The Santa Cruz Operation, and should be treated as Confidential.
/

/
/	SCO MODIFICATION HISTORY
/
/	S000	Fri Feb 01 19:52:32 PST 1991	pavelr@sco.com
/	-  Created File
/	S001	Thu Aug 27 14:02:30 PDT 1992	buckm@sco.com
/	- Streamline existing functions.
/	- Add inpd and outpd.
/	- Add EnterV86Mode.
/	S002	Mon Nov 02 09:00:59 PST 1992	buckm@sco.com
/	- Move EnterV86Mode to its own file.
/

	.text

	.align	4
	.globl	inp
inp:
	movl	4(%esp),%edx
	subl	%eax,%eax
	inb	(%dx)
	ret

	.align	4
	.globl	outp
outp:
	movl	4(%esp),%edx
	movl	8(%esp),%eax
	outb	(%dx)
	ret

	.align	4
	.globl	inpw
inpw:
	movl	4(%esp),%edx
	subl	%eax,%eax
	inw	(%dx)
	ret

	.align	4
	.globl	outpw
outpw:
	movl	4(%esp),%edx
	movl	8(%esp),%eax
	outw	(%dx)
	ret

	.align	4
	.globl	inpd
inpd:
	movl	4(%esp),%edx
	subl	%eax,%eax
	inl	(%dx)
	ret

	.align	4
	.globl	outpd
outpd:
	movl	4(%esp),%edx
	movl	8(%esp),%eax
	outl	(%dx)
	ret
