/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft
/	Corporation and should be treated as Confidential.

	.file	"sdgetv.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/sdgetv.s	1.2"
	.ident  "$Header$"


	.globl	sdgetv
	.globl	_cerror

sdgetv:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SDGETV,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
