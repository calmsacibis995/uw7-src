/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft
/	Corporation and should be treated as Confidential.

	.file	"sdleave.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/sdleave.s	1.2"
	.ident  "$Header$"


	.globl	sdleave
	.globl	_cerror

sdleave:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SDLEAVE,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
