#ident	"@(#)ihvkit:display/vga256/fFillXor.s	1.1"

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
 * This routine implements a fast Solid Fill in GXxor mode.
 * no segment checking is done.
 *
 * SYNTAX:
 * unchar * fastFillSolidGXxor(pdst,fill,hcount,count,width,widthPitch);
 * 
 *  (7/27/90 TR)
 */

#define pdst        %ebx
#define fill        %ecx
#define fillw       %cx
#define fillb       %cl
#define count       %edx
#define hcount      %edi
#define width       24(%ebp)
#define widthPitch  %esi
#define tmp         %eax

.text
	.align 4
.globl fastFillSolidGXxor

fastFillSolidGXxor:
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
.byteloop:
	decl hcount
	js .finish
	xorb fillb,(pdst)
	leal 1(widthPitch,pdst),pdst
	jmp .byteloop

.wordloop:
	decl hcount
	js .finish
	xorw fillw,(pdst)
	leal 2(widthPitch,pdst),pdst
	jmp .wordloop

.tribbleloop:
	decl hcount
	js .finish
	xorw fillw,(pdst)
	xorb fillb,2(pdst)
	leal 3(widthPitch,pdst),pdst
	jmp .tribbleloop

.finish:
	movl pdst,%eax
	leal -12(%ebp),%esp
	popl %ebx
	popl %esi
	popl %edi
	leave
	ret

.blockloop:
	decl hcount
	js .finish

	testl $1,pdst
	jz .alignword
	xorb fillb,(pdst)
	incl pdst
	decl count
.alignword:
	testl $2,pdst
	jz .aligneddword
	xorw fillw,(pdst)
	addl $2,pdst
	subl $2,count
.aligneddword:
	movl count,tmp
	shrl $5,tmp

.dwordloop:
	decl tmp
	js .fixupdword
	xorl fill,0(pdst)			
	xorl fill,4(pdst)			
	xorl fill,8(pdst)			
	xorl fill,12(pdst)			
	xorl fill,16(pdst)			
	xorl fill,20(pdst)			
	xorl fill,24(pdst)			
	xorl fill,28(pdst)			
	addl $32,pdst
	jmp .dwordloop

.fixupdword:
	movl count,tmp
	andl $28,tmp
	addl tmp,pdst
	movl .jumptab(tmp),tmp
	jmp *tmp
.jumptab: .long .Lnoop, .L0, .L1, .L2, .L3, .L4, .L5, .L6
.L6:	xorl fill,-28(pdst)
.L5:	xorl fill,-24(pdst)
.L4:	xorl fill,-20(pdst)
.L3:	xorl fill,-16(pdst)
.L2:	xorl fill,-12(pdst)
.L1:	xorl fill,-8(pdst)
.L0:	xorl fill,-4(pdst)
.Lnoop: 
	
	test $2,count
	jz .fixupbyte
	xorw fillw,(pdst)
	addl $2,pdst
.fixupbyte:
	test $1,count
	jz .enditeration
	xorb fillb,(pdst)
	incl pdst

.enditeration:
	addl widthPitch,pdst
	movl width,count
	jmp .blockloop

