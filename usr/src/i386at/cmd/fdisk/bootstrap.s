	.ident	"@(#)fdisk:i386at/cmd/fdisk/bootstrap.s	1.2.1.3"
	.ident  "$Header$"

	.file	"bootstrap.s"

	.text	

	.set	DEFAULT_DRIVE, 0x80
	.set	SECTOR_SIZE, 512
	.set	RETRIES, 5			/ max I/O error retries.

	.set	ORIGINAL_CODE, 0x7C00
	.set	SANITY_WORD, ORIGINAL_CODE + 510
	.set	SANITY_VAL, 0xAA55
	.set	BOOTSTRAP_CODE, 0x600
	.set	BOOTSTRAP_STACK, ORIGINAL_CODE

	.set	BOOTSTRAP_SIZE, 446
	.set	PARTITION_TABLE, BOOTSTRAP_CODE + BOOTSTRAP_SIZE
	.set	PART_ENTRY_SIZE, 16
	.set	BOOTIND, 0
	.set	ACTIVE_PART, 128
	.set	RELSECT, 8

_start:
	/ Reset segment registers (except %cs) to zero, to simplify things
	/ NOTE: Previous versions of this masterboot, and many other
	/	masterboot implementations disable interrupts while loading
	/	the segment registers. Except for keeping the %ss:%esp pair
	/	atomic, this should not be necessary.
	/
	xor	%eax, %eax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss		/ MUST immediately precede %esp load
	data16 / use 32-bit data-size
	movl	$BOOTSTRAP_STACK, %esp	/ MUST immediately follow %ss load

	/ When we boot, the firmware loads and runs this code at address 0x7c00.
	/ This is the same place we need to load the next bootstrap stage.
	/ Therefore, we need to move ourselves to a new address; this must be
	/ 0x600, in order for the partition table to end up in the standard
	/ place.
	/
	data16 / use 32-bit data-size
	movl	$ORIGINAL_CODE, %esi
	data16 / use 32-bit data-size
	movl	$BOOTSTRAP_CODE, %edi
	data16 / use 32-bit data-size
	movl	$SECTOR_SIZE, %ecx
	cld
	rep; smovb

	/ Reset %cs to zero and jump to the copied code.

	/ NOTE: Even though the assembler generates a 32-bit offset for the
	/ 2nd operand to this instruction, and the CPU only expects 16 bits,
	/ it's harmless since we just jump over the extra bytes.
	ljmp	$0, $reset_cs
reset_cs:

	/ The Plug and Play BIOS Specification and other sources indicate
	/ that this code will be entered with the drive number in %dl.
	/ However, it's not clear that older BIOSes always pass this in,
	/ so we do some sanity checking. If it doesn't look like a valid
	/ drive number, we use DEFAULT_DRIVE instead.

	testb	$0x80, %dl		/ Valid hard drives have 0x80 set
	jnz	drive_OK
bad_drive:
	movb	$DEFAULT_DRIVE, %dl	/ Use default drive if %dl not valid
drive_OK:
	addr16 / use 32-bit address-size
	movb	%dl, drive		/ Save drive number for later use

	/ To use the alternate monitor output on a Cirrus Video card,
	/ uncomment the following:
/enable_cirrus_video:
/	movb	$0x12, %ah
/	movb	$0x92, %bl
/	movb	$0x02, %al
/	int	$0x10

	/ The following blank-line output supposedly made a previous version
	/ of this code work on some machines that were otherwise failing
	/ (Toshiba laptop, IBM PowerPoint?). However, there's no good
	/ explanation as to why this should be. Other masterboots (DOS,
	/ OpenServer) do not do this. It is likely that the real problem
	/ was something else (most likely some missing data-size prefixes
	/ which have since been fixed), so this code is disabled, pending
	/ further testing.
	/ 
/	data16 / use 32-bit data-size
/	movl	$blankline, %esi
/	data16 / use 32-bit data-size
/	call	dispmsg

					/ Get the boot drive parameters
	movb	$8, %ah			/ Call the BIOS to get drive parms
	addr16 / use 32-bit address-size
	movb	drive, %dl
	int	$0x13
	jnc	gotparm			/ jump if no error

	data16 / use 32-bit data-size
	movl	$hdperr, %esi
	jmp	error

gotparm:
	andb	$0x3f, %cl		/ extract spt value
	movb	%cl, %al
	addr16 / use 32-bit address-size
	movb	%al, spt		/ and save it
	incb	%dh			/ max head + 1 for # heads
					/	(tracks per cyl)
	mulb	%dh			/ compute sectors per cyl
	addr16 / use 32-bit address-size
	mov	%eax, spc		/ and save it

	/ Search for the active partition within the partition table,
	/ which was loaded from the same sector along with this code.
	/
	data16 / use 32-bit data-size
	movl	$PARTITION_TABLE, %esi	/ point to start of partition table
	data16 / use 32-bit data-size
	movl	$4, %ecx

check_active_part:	
	addr16 / use 32-bit address-size
	cmpb	$ACTIVE_PART, BOOTIND(%esi)
	je	active_partition_found
	data16 / use 32-bit data-size
	addl	$PART_ENTRY_SIZE, %esi
	loop	check_active_part

	data16 / use 32-bit data-size
	movl	$nopart, %esi
	jmp	error

active_partition_found:
	xor	%ecx, %ecx		/ initialize retry count
	movb	$RETRIES, %cl
retry:
	push	%ecx

	addr16 / use 32-bit address-size
	mov	RELSECT(%esi), %eax	/ get start sector of partition
	addr16 / use 32-bit address-size
	mov	[RELSECT+2](%esi), %edx
	addr16 / use 32-bit address-size
	div	spc			/ divide to get cyl
	mov	%eax, %ecx		/ cyl to cx (high 16 bits always zero)
	mov	%edx, %eax		/ remainder from divide to ax
	cltd / (cwtd)			/ zero %dx
	addr16 / use 32-bit address-size
	div	spt			/ divide to get track (head) in al
	movb	%al, %dh		/ move head to dh for BIOS call
	xchgb	%ch, %cl		/ move cyl:0-7 to ch for BIOS call
	shlb	$6, %cl			/ extract cyl:8-9
	orb	%dl, %cl		/ combine with sector into cl for BIOS
	incb	%cl			/ BIOS wants sector number one-based
	data16 / use 32-bit data-size
	movl	$ORIGINAL_CODE, %ebx	/ pickup the destination phys addr
	movb	$1, %al			/ sectors to read
	movb	$2, %ah			/ read sectors function
	addr16 / use 32-bit address-size
	movb	drive, %dl

	int	$0x13			/ BIOS disk support

	jnc	check_sanity		/ if no error, continue

	movb	$0, %ah			/ reset controller
	int	$0x13

	pop	%ecx

	loop	retry			/ retry several times

	/ Maybe it's really a bogus drive number. If we're not using the
	/ default drive, switch to it now and try again.
	addr16 / use 32-bit address-size
	cmpb	$DEFAULT_DRIVE, drive
	data16 / use 32-bit data-size
	jne	bad_drive

	data16 / use 32-bit data-size
	movl	$ioerr, %esi
	jmp	error

check_sanity:
	pop	%ecx

	/ cmpw	$SANITY_VAL, SANITY_WORD
	data16 / use 32-bit data-size
	movl	$SANITY_VAL, %eax
	addr16 / use 32-bit address-size
	cmp	%eax, SANITY_WORD
	jne	sanity_check_failed

	/ Jump to the next-stage bootstrap, which we just loaded.
	/ Entry conditions: %cs=%ss=0, %ip=%sp=7c00, %ds:%si->boot partition,
	/			%dl=drive number
	data16 / use 32-bit data-size
	movl	$ORIGINAL_CODE, %eax
	jmp	*%eax			/ Execute OS-specific bootstrap

sanity_check_failed:
	data16 / use 32-bit data-size
	movl	$sanityerr, %esi
	/ FALL THROUGH

error:
	data16 / use 32-bit data-size
	call	dispmsg

	/ We'd like to use INT 18, but too many BIOSes do funky things,
	/ like saying they can't load BASIC, or clearing the screen and
	/ losing our error messages. So, we'll just spin here.
	jmp	.

badp2:
	data16 / use 32-bit data-size
	movl	$hdperr, %esi
	jmp	error

/	----------------------------------------------------
/ 	dispmsg:		put null-terminated string at si to console
/
dispmsg:
	movb	$0xF, %ah	/ Get Current Video Mode
	int	$0x10		/ in order to get active page number into bh
dispchar:
	cld
	lodsb
	orb	%al,%al
	jz	done		/ zero is end of the string

	movb	$0xE, %ah	/ Write TTY function
	int	$0x10
	jmp	dispchar
done:
	data16 / use 32-bit data-size
	ret			

/
/	program variables
/
spt:		.value	0		/ disk sectors per track
spc:		.value	0		/ disk sectors per cylinder
drive:		.byte	0		/ drive number

/
/	various	error messages
/
ioerr:		.string "Error loading operating system\r\n"
hdperr:		.string "Cannot determine disk geometry\r\n"
nopart:		.string	"No active partition\r\n"
sanityerr:	.string "Missing operating system\r\n"

/blankline:	.string	"\r\n"
