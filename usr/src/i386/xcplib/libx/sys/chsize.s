/		copyright	"%c%"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft
/	Corporation and should be treated as Confidential.

	.file	"chsize.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/chsize.s	1.2"
	.ident  "$Header$"


	.globl	chsize
	.globl	_cerror

chsize:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$CHSIZE,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
