	.ident	"@(#)kern-i386:util/kdb/scodb/entry.s	1.1"
/
/	Copyright (C) The Santa Cruz Operation, 1989-1992.
/		All Rights Reserved.
/	This Module contains Proprietary Information of
/	The Santa Cruz Operation, and should be treated as Confidential.
/
/
/	Modification History:
/
/	L000	scol!nadeem	2jul92
/	- fixed bug whereby eipsave was defined globally in this module
/	  as well as being defined as a per-processor variable in vuifile.
/	  Deleted the definition in this module.
/

	.text

////////////////////////////////////////////////////////////////////////
	.globl	debugsetup
debugsetup:
	/
	/ set up interrupt return frame
	/
	pop	eipsave			/ take off return address
	pushf				/	EFL
	cli
	push	%cs			/	CS
	push	eipsave			/	EIP

	/
	/ use xor/movw sequence to push high-word-zeroed selectors
	/ to look right
	/
	push	%ss			/ SS		  7
	push	%esp			/ ESP		  6
	push	16(%esp)		/ EFL		  5
	push	%cs			/ CS		  4
	push	eipsave			/ EIP		  3
	push	%ds			/ DS		  2
	push	%es			/ ES		  1
	pusha				/ EAX-EDI	  0 to -7
	push	$0			/ TRAPNO	  -8

	leal	32(%esp), %eax
	push	%eax			/ REGP (address of eax)
	call	scodb			/ scodb_debug(REGP)
	add	$8,	%esp		/ restore stack after call

	popa

/ver_gs:
/	pop	%eax
/	verr	%eax
/	jnz	ver_fs		/ failed
/	movw	%ax,	%gs
/
/ver_fs:
/	pop	%eax
/	verr	%eax
/	jnz	ver_es		/ failed
/	movw	%ax,	%fs

ver_es:
	pop	%eax
	verr	%ax
	jnz	ver_ds		/ failed
	movw	%ax,	%es

ver_ds:
	pop	%eax
	verr	%ax
	jnz	restore		/ failed
	movw	%ax,	%ds

	/
	/ restore registers, and
	/ fix stack frame by discarding everything (TRAP to SS)
	/ up to interrupt return frame
	/
restore:
	add	$20,	%esp

	/
	/ return through interrupt frame pops off and sets EIP, CS, EFL
	/
	iret
////////////////////////////////////////////////////////////////////////

