/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft
/	Corporation and should be treated as Confidential.	  

	.file	"unexecseg.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/unexecseg.s	1.2"
	.ident  "$Header$"


	.globl	unexecseg
	.globl	_cerror

unexecseg:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$UNEXECSEG,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
