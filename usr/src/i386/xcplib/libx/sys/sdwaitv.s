/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft 
/	Corporation and should be treated as Confidential.	   

	.file	"sdwaitv.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/sdwaitv.s	1.2"
	.ident  "$Header$"


	.globl	sdwaitv
	.globl	_cerror

sdwaitv:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SDWAITV,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
