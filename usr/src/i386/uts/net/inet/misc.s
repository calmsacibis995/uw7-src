	.ident	"@(#)misc.s	1.4"
/
/ Copyrighted as an unpublished work.
/ (c) Copyright 1987-1995 Computer Associates International, Inc.
/ All rights reserved.
/
/ RESTRICTED RIGHTS
/
/ These programs are supplied under a license.  They may be used,
/ disclosed, and/or copied only as permitted under such license
/ agreement.  Any copy must contain the above copyright notice and
/ this restricted rights notice.  Use, copying, and/or disclosure
/ of the programs is strictly prohibited unless otherwise provided
/ in the license agreement.
/
/ SCCS IDENTIFICATION
/
/ This file contains routines that should be written in assembler because
/ they get called frequently.  On some systems, such as SVR4, these can
/ be inline, but on others, such as SCO UNIX, they must be here.
	.file "misc.s"

	.globl ntohl
	.globl ntohs
	.globl htonl
	.globl htons

	.text
/
/	swapping for longs
/
ntohl:
htonl:
	movl	4(%esp), %eax	
	xchgb	%ah, %al		/ swap 2
	rorl	$16, %eax		/ rotate
	xchgb	%ah, %al		/ swap the other 2
	clc				/ clean up the carry flag
	ret
/
/	swapping for shorts
/

ntohs:
htons:
	movl	4(%esp), %eax	
	xchgb	%ah, %al		/ swap 2
	clc				/ clean up the carry flag
	ret
