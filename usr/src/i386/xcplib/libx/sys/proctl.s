/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft
/	Corporation and should be treated as Confidential.

	.file	"proctl.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/proctl.s	1.2"
	.ident  "$Header$"


	.globl	proctl
	.globl	_cerror

proctl:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$PROCTL,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
