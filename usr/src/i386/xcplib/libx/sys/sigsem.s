/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft
/	Corporation and should be treated as Confidential.

	.file	"sigsem.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/sigsem.s	1.2"
	.ident  "$Header$"


	.globl	sigsem
	.globl	_cerror

sigsem:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SIGSEM,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
