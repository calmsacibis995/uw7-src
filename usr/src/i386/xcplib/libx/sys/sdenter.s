/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft
/	Corporation and should be treated as Confidential.	 

	.file	"sdenter.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/sdenter.s	1.2"
	.ident  "$Header$"


	.globl	sdenter
	.globl	_cerror

sdenter:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SDENTER,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
