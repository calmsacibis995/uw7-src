	.ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/bootstrap.s	1.1"
	.ident  "$Header$"

	.file	"bootstrap.s"


	.text	

	.set	BOOT_BEGIN, 0x7C00
	.set	STACK_SP, 0x7C00
	.set	VIDEO, 0x10
	.set	KBD, 0x16
	.set	TTY, 14
	.set	BOOT, 0x19


	.globl	boot_code

	.align	4
boot_code:
	jmp	begin		/ Jump to the start of the program
	nop

	.byte	0,0,0,0,0,0,0,0	/ OEM name and version
	.value	512	      	/ Bytes/sector
	.byte	1		/ Sectors/allocation unit
	.value	1		/ Reserved
	.byte	2		/ Number of FATs
	.value	256		/ Root directory entries
	.value	0		/ Total number of sectors
	.byte	0		/ Media descriptor
	.value	0		/ Sectors/FAT
	.value	0	 	/ Sectors/track
	.value	0		/ Number of heads
	.long	0		/ Hidden sectors
	.long	0		/ Total sectors
	.byte	0		/ Drive number
	.byte	0		/ Reserved
	.byte	0		/ Boot signature (0x29)
	.long	0xbabe		/ Volume ID
	.byte	0,0,0,0,0,0,0,0	/ Volume label
	.byte	0,0,0		/   "      "
	.byte	0,0,0,0,0,0,0,0	/ Filesystem type

begin:
	cli			/ No interrupts
	movw	%cs, %ax	/ Set segment registers equal to the code seg.
	movw	%ax, %ss
  	movw	%ax, %es
	movw	%ax, %ds
	movw	$STACK_SP, %ax
	movw	%ax, %sp
	sti			/ Restore interrupts

	data16
	addr16
	mov	$message, %esi
	movb	$1, %bl		/ select a normal video attribute
dispchar:
	cld
	lodsb
	orb	%al,%al
	jz	done		/ zero is end of the string

	movb	$TTY, %ah	/ Write TTY function
	int	$VIDEO
	jmp	dispchar
done:
	xorb	%ah, %ah	/ Wait for the next keystroke
	int	$KBD
	int 	$BOOT 		/ Try booting the system again.
	.byte 	0xEA		/ Jump to reset vector
	.value	0xFFF0
	.value	0xF000


message:
	.string		"\r\nNon System disk\r\nReplace and press any key\r\n"

	.value	0		/ reserve space for boot sector signature
