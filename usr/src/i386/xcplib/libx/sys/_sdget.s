/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft
/	Corporation and should be treated as Confidential.
/
/	The _sdget() routine is called from libx/port/sys/sdget.c
/

	.file	"_sdget.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/_sdget.s	1.2"
	.ident  "$Header$"


	.globl	_sdget
	.globl	_cerror

_sdget:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SDGET,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
