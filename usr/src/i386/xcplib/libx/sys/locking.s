/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft
/	Corporation and should be treated as Confidential.	  

	.file	"locking.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/locking.s	1.2"
	.ident  "$Header$"


	.globl	locking	
	.globl	_cerror

locking:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$XLOCKING,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
