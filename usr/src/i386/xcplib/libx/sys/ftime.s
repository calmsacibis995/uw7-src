/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft 
/	Corporation and should be treated as Confidential.

	.file	"ftime.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/ftime.s	1.2"
	.ident  "$Header$"


	.globl	ftime
	.globl	_cerror

ftime:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$FTIME,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
