/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft
/	Corporation and should be treated as Confidential.

	.file	"nbwaitsem.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/nbwaitsem.s	1.2"
	.ident  "$Header$"


	.globl	nbwaitsem
	.globl	_cerror

nbwaitsem:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$NBWAITSEM,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
