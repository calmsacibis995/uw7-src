	.ident "@(#)e3Bio.s	3.1"

/ 
/       Copyright (C) The Santa Cruz Operation, 1993-1994.
/       This Module contains Proprietary Information of
/       The Santa Cruz Operation and should be treated
/       as Confidential.

/
/	System V STREAMS TCP - Release 4.0
/
/	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
/
/	All Rights Reserved.
/
/	The copyright above and this notice must be preserved in all
/	copies of this source code.  The copyright above does not
/	evidence any actual or intended publication of this source
/	code.
/
/	This is unpublished proprietary trade secret source code of
/	Lachman Associates.  This source code may not be copied,
/	disclosed, distributed, demonstrated or licensed except as
/	expressly authorized by Lachman Associates.
/
/	System V STREAMS TCP was jointly developed by Lachman
/	Associates and Convergent Technologies.
/
	.set	PTR,    8
	.set	LEN,   12
	.set	PORT,  16
	.set	STREG, 20
	.set	TYPE16,24
	.set	OADDR, 28
	.set	DPRDY, 0x80

	.globl	e3Bioout
e3Bioout:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edx
	pushl	%esi
	pushl	%ecx
	pushl	%ebx
	cld
	movl	PTR(%ebp),%esi
	testb	$1,TYPE16(%ebp)
	jnz	ogot16
otop:
	cmpl	$0,LEN(%ebp)
	jg	odprdy
odone:
	popl	%ebx
	popl	%ecx
	popl	%esi
	popl	%edx
	leave
	ret
odprdy:
	movw	STREG(%ebp),%dx
oloop:
	inb	(%dx)
	testb	$DPRDY,%al
	je	oloop

	movl	$8,%ecx
	subl	%ecx,LEN(%ebp)
	jge	output
	movl	LEN(%ebp),%ecx
	addl	$8,%ecx
output:
	movw	PORT(%ebp),%dx
	rep	
	outsb
	jmp	otop

ogot16:
	testb	$1,OADDR(%ebp)
	jz	oloop2
	movw	STREG(%ebp),%dx
olp1:
	inb	(%dx)
	testb	$DPRDY,%al
	je	olp1

	subl	$1,LEN(%ebp)
	movw	PORT(%ebp),%dx
	outsb
oloop2:
	movl	LEN(%ebp),%ebx
	cmp	$1,%ebx
	jle	odone16
	movw	STREG(%ebp),%dx
olp2:
	inb	(%dx)
	testb	$DPRDY,%al
	je	olp2

	cmp	$16,%ebx
	jle	ol1
	mov	$16,%ebx
ol1:
	testb	$1,%bl
	jz	ol2
	dec	%ebx
ol2:
	mov	%ebx,%ecx
	shrl	$1,%ecx
	movw	PORT(%ebp),%dx
	rep
	outsw
	subl	%ebx,LEN(%ebp)
	jmp	oloop2
odone16:
	cmp	$0,%ebx
	jz	odone
	movw	STREG(%ebp),%dx
olp3:
	inb	(%dx)
	testb	$DPRDY,%al
	je	olp3
	movw	PORT(%ebp),%dx
	outsb
	jmp	odone


	.globl	e3Bioin
e3Bioin:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edx
	pushl	%edi
	pushl	%ecx
	push	%ebx
	cld
	movl	PTR(%ebp),%edi
	testb	$1,TYPE16(%ebp)
	jnz	igot16
itop:
	cmpl	$0,LEN(%ebp)
	jg	idprdy
idone:
	popl	%ebx
	popl	%ecx
	popl	%edi
	popl	%edx
	leave
	ret
idprdy:
	movw	STREG(%ebp),%dx
iloop:
	inb	(%dx)
	testb	$DPRDY,%al
	je	iloop

	movl	$8,%ecx
	subl	%ecx,LEN(%ebp)
	jge	input
	movl	LEN(%ebp),%ecx
	addl	$8,%ecx
input:
	movw	PORT(%ebp),%dx
	rep	
	insb
	jmp	itop

igot16:
	movl	LEN(%ebp),%ebx
	testb	$1,%bl
	jz	il1
	inc	%ebx
il1:
	shrl	%ebx
iloop1:
	cmp	$0,%ebx
	jz	idone

	movw	STREG(%ebp),%dx
ilp1:
	inb	(%dx)
	testb	$DPRDY,%al
	je	ilp1

	mov	%ebx,%ecx
	cmp	$8,%ecx
	jle	il2
	mov	$8,%ecx
il2:
	subl	%ecx,%ebx
	movw	PORT(%ebp),%dx
	rep
	insw
	jmp	iloop1
