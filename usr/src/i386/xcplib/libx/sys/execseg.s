/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft
/	Corporation and should be treated as Confidential.

	.file	"execseg.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/execseg.s	1.2"
	.ident  "$Header$"


	.globl	execseg
	.globl	_cerror

execseg:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$EXECSEG,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
