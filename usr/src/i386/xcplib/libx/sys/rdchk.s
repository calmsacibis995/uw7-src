/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft
/	Corporation and should be treated as Confidential.	  

	.file	"rdchk.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/rdchk.s	1.2"
	.ident  "$Header$"


	.globl	rdchk
	.globl	_cerror

rdchk:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$RDCHK,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
