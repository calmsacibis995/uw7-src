#ident	"@(#)ihvkit:display/vga256/fFillCopy.s	1.1"

/*
 *	Copyright (c) 1991, 1992 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 * $Header$
 */

/*
 *
 * This routine implements a fast Solid Fill in GXcopy mode.
 * no segment checking is done.
 *
 * SYNTAX:
 * unchar * fastFillSolidGXcopy(pdst,fill,hcount,count,width,widthPitch);
 * 
 */

#define pdst       %ebx
#define fill       %ecx
#define fillw      %cx
#define fillb      %cl
#define count      %edx
#define hcount     %edi
#define width      24(%ebp)
#define widthPitch %esi
#define tmp        %eax

.text
	.align 4
.globl fastFillSolidGXcopy

fastFillSolidGXcopy:
	pushl %ebp
	movl %esp,%ebp
	pushl %edi
	pushl %esi
	pushl %ebx
	movl 8(%ebp),pdst
	movl 12(%ebp),fill
	movl 16(%ebp),hcount
	movl 20(%ebp),count
	movl 28(%ebp),widthPitch
	orl hcount,hcount
	jz .finish
	orl count,count
	jz .finish
	cmpl $3,count
	jg .blockloop
	je .tribbleloop
	cmpl $2,count
	je .wordloop
/*
 * do a fast vertical line
 */
	.align 4
.byteloop:
	movb fillb,(pdst)
	leal 1(widthPitch,pdst),pdst
	decl hcount
	jnz .byteloop
	jmp .finish

	.align 4
.wordloop:
	movw fillw,(pdst)
	leal 2(widthPitch,pdst),pdst
	decl hcount
	jnz .wordloop
	jmp .finish

	.align 4
.tribbleloop:
	movw fillw,(pdst)
	movb fillb,2(pdst)
	leal 3(widthPitch,pdst),pdst
	decl hcount
	jnz .tribbleloop
	jmp .finish

.blockloop:
	testl $1,pdst
	jz .alignword
	movb fillb,(pdst)
	incl pdst
	decl count
.alignword:
	testl $2,pdst
	jz .aligneddword
	movw fillw,(pdst)
	leal 2(pdst),pdst
	leal -2(count),count
.aligneddword:
	movl count,tmp
	shrl $5,tmp
	jz .fixupdword

	.align 4
.dwordloop:
	movl fill,0(pdst)			
	movl fill,4(pdst)			
	movl fill,8(pdst)			
	movl fill,12(pdst)			
	movl fill,16(pdst)			
	movl fill,20(pdst)			
	movl fill,24(pdst)			
	movl fill,28(pdst)			
	leal 32(pdst),pdst
	decl tmp
	jnz .dwordloop

.fixupdword:
	movl count,tmp
	andl $28,tmp
	leal (tmp,pdst),pdst
	movl .jumptab(tmp),tmp
	jmp *tmp

	.align 4
.jumptab: .long .Lnoop, .L0, .L1, .L2, .L3, .L4, .L5, .L6

.L6:	movl fill,-28(pdst)
.L5:	movl fill,-24(pdst)
.L4:	movl fill,-20(pdst)
.L3:	movl fill,-16(pdst)
.L2:	movl fill,-12(pdst)
.L1:	movl fill,-8(pdst)
.L0:	movl fill,-4(pdst)
.Lnoop: 
	
	test $2,count
	jz .fixupbyte
	movw fillw,(pdst)
	leal 2(pdst),pdst
.fixupbyte:
	test $1,count
	jz .enditeration
	movb fillb,(pdst)
	incl pdst

.enditeration:
	leal (widthPitch,pdst),pdst
	movl width,count
	decl hcount
	jnz .blockloop

.finish:
	movl pdst,%eax
	leal -12(%ebp),%esp
	popl %ebx
	popl %esi
	popl %edi
	leave
	ret


