/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft 
/	Corporation and should be treated as Confidential.

	.file	"waitsem.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/waitsem.s	1.2"
	.ident  "$Header$"


	.globl	waitsem
	.globl	_cerror

waitsem:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$WAITSEM,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
