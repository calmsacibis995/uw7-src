	.ident	"@(#)kern-i386at:svc/oemsup.s	1.1.2.1"
	.file	"svc/oemsup.s"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft 
/	Corporation and should be treated as Confidential.

/
/
/ *** oemsup.s - miscellaneous oem specific routines
/
/	Routines are:
/		oem_fclex - clear 287/387 exception

include(KBASE/svc/asm.m4)
include(assym_include)

/ ******************************************************************
/
/ 	oem_fclex - clear 287/387 exception 
/
/ ******************************************************************

ENTRY(oem_fclex)
	/ The NDP's (287's) busy line is only held active while actually
	/ executing, but hardware that sits between the 2 chips latches
	/ the NDP error line and feeds it to the 286/386 busy line.  Since this
	/ prevents normal coprocessor communication, we must clear the NDP
	/ BUSY latch before attempting to examine the NDP status word.
	/ Else the 286 thinks the NDP is busy and hangs on any NDP access.
	movw	$0x0f0,%dx		/ outb(0xf0, 0)
	subb	%al,%al
	outb	(%dx)
	ret

	SIZE(oem_fclex)
