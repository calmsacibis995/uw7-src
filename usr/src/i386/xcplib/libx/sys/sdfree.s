/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft
/	Corporation and should be treated as Confidential.	  

	.file	"sdfree.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/sdfree.s	1.2"
	.ident  "$Header$"


	.globl	sdfree
	.globl	_cerror

sdfree:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SDFREE,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
