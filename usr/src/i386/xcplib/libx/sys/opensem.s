/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft
/	Corporation and should be treated as Confidential.	 

	.file	"opensem.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/opensem.s	1.2"
	.ident  "$Header$"


	.globl	opensem
	.globl	_cerror

opensem:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$OPENSEM,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
