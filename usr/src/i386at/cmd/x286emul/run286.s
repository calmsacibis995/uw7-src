	.file   "run286.s"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft 
/	Corporation and should be treated as Confidential.

	.ident	"@(#)x286emul:run286.s	1.1"


/ This file implements the 386 interface routine that runs 286 code.
/ The entry point "run286" is called from i286exec() in C code.
/
/ run286(stackpointer, entrypoint)
/   ushort *stackpointer;       /* 286 initial stack pointer */
/   ulong entrypoint;           /* 286 entry point CS/IP */
/
/ The 286 stack has the exec args on it.
/ We switch to the 286 stack,
/    push on the 286 entry point CS/IP,
/    clear the registers,
/    and return.
/
/ The 286 stack looks like this:
/
/ (one word per line)

/            <-- 0xFFFC
/       .
/       .
/       strings
/       NULL
/       .
/       .
/       environment string ptrs
/       NULL
/       .
/       .
/       exec arg string ptrs
/       argc                  <-- stackpointer


	.globl  Stacksel	/ ushort stack segment selector

	.globl  run286
run286:

	pushl   %ebp             / for debug stack tracing
	movl    %esp, %ebp

	/ get args
	movl    8(%ebp), %ecx    / stack pointer
	movl    12(%ebp), %eax   / entry point CS/IP

	/ load 286 stack segment register and pointer
	movw    Stacksel, %bx
	movw    %bx, %ds
	movw    %bx, %es
	movw    %bx, %ss
		/ interrupt cannot occur here
	movw    %cx, %sp

	/ push entry point on 286 stack
	pushl   %eax

	/ clear registers for 286 code
	subl    %eax, %eax
	movl    %eax, %ebx
	movl    %eax, %ecx
	movl    %eax, %edx
	movl    %eax, %ebp
	movl    %eax, %esi
	movl    %eax, %edi
	pushw   %ax
	popfw

	/ return to 286 program entry point
	data16          / for 16-bit IP
	lret