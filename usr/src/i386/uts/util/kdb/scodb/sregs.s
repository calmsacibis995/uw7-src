	.ident	"@(#)kern-i386:util/kdb/scodb/sregs.s	1.1"
/
/	Copyright (C) The Santa Cruz Operation, 1989-1995.
/		All Rights Reserved.
/	This Module contains Proprietary Information of
/	The Santa Cruz Operation, and should be treated as Confidential.
/ 

/
/ Modification History
/
/ L000	nadeem		19oct92
/ - change ggdt() and gidt() to return the base of the GDT/IDT registers,
/   rather than to fill in the address passed with the GDT/IDT base and limit.
/ L001	nadeem		2feb95
/ - added new support routine db_search_page() for use by db_search()
/   function.  db_search_page() searches a specified page for a specified
/   value.
/

/
/	gdbr(n)		return DRn
/	sdbr(n,	 v)	DRn := v
/	ggdt()		return Global Descriptor Table register
/	gidt()		return Interrupt Descriptor Table register
/	gldt()		return Local Descriptor Table register
/	gcr0()		return CR0
/	gtr6()		return TR6
/	gtr7()		return TR7
/
/	g_esp()		return ESP
/	g_ebp()		return EBP
/

/#define	ARG1	BPARGBAS
/#define	ARG2	BPARGBAS+4
/#define	ARG3	BPARGBAS+8
/#define	ARG4	BPARGBAS+12

	.set	ARG1,	 8	/ offset of first argument using bp as base
	.set	ARG2,	 12	/ offset of second argument using bp as base
	.set	ARG3,	 16	/ offset of third argument using bp as base
	.set	ARG4,	 20	/ offset of fourth argument using bp as base


	.data			/ L000 v
dscr_save:	.long	0, 0	/ for use with SGDT/SIDT instructions

	.text			/ L000 ^

		



/-----------------------------------------------------------------------------
/- gdbr(n) : return DRn
	.globl	gdbr
gdbr:
	push	%ebp
	mov	%esp,	%ebp
	mov	ARG1(%ebp),	%eax
	pop	%ebp
	cmp	$0,	%eax
	jne	gndbr0
	movl	%db0,	%eax	/ DR0
	ret
gndbr0:	cmp	$1,	%eax
	jne	gndbr1
	movl	%db1,	%eax	/ DR1
	ret
gndbr1:	cmp	$2,	%eax
	jne	gndbr2
	movl	%db2,	%eax	/ DR2
	ret
gndbr2:	cmp	$3,	%eax
	jne	gndbr3
	movl	%db3,	%eax	/ DR3
	ret
gndbr3:	cmp	$6,	%eax
	jne	gndbr6
	movl	%db6,	%eax	/ DR6
	ret
gndbr6:	cmp	$7,	%eax
	jne	gndbr7
	movl	%db7,	%eax	/ DR7
	ret
gndbr7:	mov	$-1,	%eax
	ret
/-----------------------------------------------------------------------------





/-----------------------------------------------------------------------------
/- sdbr(n, v) : DRn := v
	.globl	sdbr
sdbr:
	push	%ebp
	mov	%esp,	%ebp
	mov	ARG1(%ebp),	%ecx
	mov	ARG2(%ebp),	%eax
	cmp	$0,	%ecx
	jne	sndbr0
	movl	%eax,	%db0			/ set DR0
	jmp	sdbrout
sndbr0:	cmp	$1,	%ecx
	jne	sndbr1
	movl	%eax,	%db1			/ set DR1
	jmp	sdbrout
sndbr1:	cmp	$2,	%ecx
	jne	sndbr2
	movl	%eax,	%db2			/ set DR2
	jmp	sdbrout
sndbr2:	cmp	$3,	%ecx
	jne	sndbr3
	movl	%eax,	%db3			/ set DR3
	jmp	sdbrout
sndbr3:	cmp	$6,	%ecx
	jne	sndbr6
	movl	%eax,	%db6			/ set DR6
	jmp	sdbrout
sndbr6:	cmp	$7,	%ecx
	jne	sndbr7
	movl	%eax,	%db7			/ set DR7
	jmp	sdbrout
sndbr7:
sdbrout:
	pop	%ebp
	ret
/-----------------------------------------------------------------------------

/
/	scodb_set_de()
/
/	Set the Debugging Extensions bit in CR4
/

	.set	CR4_DE, 8

	.globl	scodb_set_de			/ L002v
scodb_set_de:
	movl	%cr4, %eax
	testl	$CR4_DE, %eax
	jnz	de_done
	orl	$CR4_DE, %eax
	movl	%eax, %cr4
de_done:
	ret					/ L002^



/-----------------------------------------------------------------------------
/- ggdt : load memory with contents of GDT
/-        returns argument
/-
	.globl	ggdt
ggdt:						/ L000 v
	sgdt	dscr_save			/ copy gdtr to addr
	mov	dscr_save+2, %eax		/ return the GDT base
	ret
/-----------------------------------------------------------------------------




/-----------------------------------------------------------------------------
/- gidt() : load memory with contents of GDT
/-        returns argument
/-
	.globl	gidt
gidt:
	sidt	dscr_save			/ copy idtr to addr
	mov	dscr_save+2, %eax		/ return the IDT base
	ret					/ L000 ^
/-----------------------------------------------------------------------------





/-----------------------------------------------------------------------------
/- gldt() : returns LDT
	.globl	gldt
gldt:
	sldt	%ax
	ret
/-----------------------------------------------------------------------------





/-----------------------------------------------------------------------------
/- gcr0() : returns CR0
	.globl	gcr0
gcr0:
	movl	%cr0,%eax
	ret
/-----------------------------------------------------------------------------



/-----------------------------------------------------------------------------
/- gtr6() : returns TR6
	.globl	gtr6
gtr6:
	movl	%tr6,%eax
	ret
/-----------------------------------------------------------------------------




/-----------------------------------------------------------------------------
/- gtr7() : returns TR7
	.globl	gtr7
gtr7:
	movl	%tr7,%eax
	ret
/-----------------------------------------------------------------------------




/-----------------------------------------------------------------------------
/- g_esp() : returns ESP
	.globl	g_esp
g_esp:
	mov	%esp,	%eax
	ret
/-----------------------------------------------------------------------------

/-----------------------------------------------------------------------------
/- g_ebp() : returns EBP
	.globl	g_ebp
g_ebp:
	mov	%ebp,	%eax
	ret
/-----------------------------------------------------------------------------

								/ L001v
/
/	db_search_page(start address, count, mask, value)
/
/	Search a page for a dword value.  The search start from the start
/	address for a specified number of bytes, although the page is
/	treated as a number of dwords.  A mask is also specified which
/	is and-ed with the memory dwords before being compared against the
/	value being searched for.
/
/	Returns 0 if no match, else the address of the first match.


	.align	4
	.globl	db_search_page
db_search_page:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi
	pushl	%esi
	cld
	movl	ARG2(%ebp), %ecx	/ %ecx = byte count
	shrl	$2, %ecx		/ convert byte count to dword count
	movl	ARG3(%ebp), %edx	/ %edx = mask
	cmpl	$-1, %edx		/ if mask != -1, execute slow loop
	jne	logical_cmp

	movl	ARG1(%ebp), %edi	/ %edi = start address
	movl	ARG4(%ebp), %eax	/ %eax = value to search for
	repnz				/ repeat compare (%edi)++ against %eax 
	scasl				/   until a match or %ecx == 0
	jne	notfound
	leal	-4(%edi), %eax		/ found - return the matching address
exit:
	popl	%esi
	popl	%edi
	popl	%ebp
	ret

notfound:
	xorl	%eax, %eax		/ not found - return 0
	popl	%esi
	popl	%edi
	popl	%ebp
	ret

	/
	/ This is the "slow" loop which is called whenever mask != -1.
	/

logical_cmp:
	movl	ARG1(%ebp), %esi	/ %esi = start address
	movl	ARG4(%ebp), %edi	/ %edi = value to search for

	.align	4
logical_loop:
	lodsl				/ (%esi)++ => %eax
	andl	%edx, %eax
	cmpl	%eax, %edi
	je	found2
	decl	%ecx
	jne	logical_loop
	jmp	notfound		/ not found - return 0
found2:
	leal	-4(%esi), %eax		/ found - return matching address
	jmp	exit

								/ L001^
