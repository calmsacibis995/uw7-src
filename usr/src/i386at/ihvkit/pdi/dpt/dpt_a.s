#ident	"@(#)ihvkit:pdi/dpt/dpt_a.s	1.1"
/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)uts-x86at:io/hba/dpt_a.s	1.1"
	.ident	"$Header$"

	.type	scsi_send_cmd,@function
	.text
	.globl	scsi_send_cmd
	.align	4
scsi_send_cmd:
	pushl	%ebp
	movl	%esp,%ebp
        pushl   %eax
        
	movw    0x08(%ebp),%dx
	addb    $8,%dl

dptHA_wait_not_busy:
	inb     (%dx)
	testb   $1,%al
	jnz     dptHA_wait_not_busy
	subb    $6,%dl

        movb    0x0F(%ebp),%al
        outb    (%dx)
        incw    %dx
        movb    0x0E(%ebp),%al
        outb    (%dx)
        incw    %dx
        movb    0x0D(%ebp),%al
        outb    (%dx)
        incw    %dx
        movb    0x0C(%ebp),%al
        outb    (%dx)
        incw    %dx
        incw    %dx
        movb    0x10(%ebp),%al
        outb    (%dx)

        popl    %eax
	leave	
	ret	

	.align	4
	.size	scsi_send_cmd,.-scsi_send_cmd
