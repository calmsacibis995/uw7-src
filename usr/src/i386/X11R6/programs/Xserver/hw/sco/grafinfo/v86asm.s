/
/	@(#) v86asm.s 12.1 95/05/09 
/
/	Copyright (C) The Santa Cruz Operation, 1992.
/	This Module contains Proprietary Information of
/	The Santa Cruz Operation, and should be treated as Confidential.
/

/
/	SCO MODIFICATION HISTORY
/
/	S000	Mon Nov 02 09:02:29 PST 1992	buckm@sco.com
/	-  Created.
/

	.text

	.align	4
	.globl	EnterV86Mode
EnterV86Mode:
	push	%ebp
	movl	%esp,%ebp
	iret			/ enter V86 mode through TSS
	xor	%eax,%eax	/ clear return value
	cmp	%esp,%ebp	/ check for error code on stack
	je	ok
err:	pop	%eax		/ get error code
ok:	leave
	ret
