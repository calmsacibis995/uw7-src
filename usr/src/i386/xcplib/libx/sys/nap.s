/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft
/	Corporation and should be treated as Confidential.

	.file	"nap.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/nap.s	1.2"
	.ident  "$Header$"


	.globl	nap
	.globl	_cerror

nap:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$NAP,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
