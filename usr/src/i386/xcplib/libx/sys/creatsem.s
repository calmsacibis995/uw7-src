/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft
/	Corporation and should be treated as Confidential.

	.file	"creatsem.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/creatsem.s	1.2"
	.ident  "$Header$"


	.globl	creatsem
	.globl	_cerror

creatsem:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$CREATSEM,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
