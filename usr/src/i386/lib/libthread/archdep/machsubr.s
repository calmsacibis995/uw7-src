.ident	"@(#)libthread:i386/lib/libthread/archdep/machsubr.s	1.4.1.3"

/* Copyright (c) 1991 by Sun Microsystems, Inc.  */

	.file	"machsubr.s"
	.text

/* void *_thr_flush_and_tell()				*/
/*	Return the frame pointer (for compatibility)	*/
/*							*/
/* Calling/Exit state:					*/
/*	Called without holding any locks.		*/

	.globl	_thr_flush_and_tell
	.align	4
_fgdef_(_thr_flush_and_tell):
	movl	%ebp, %eax
	ret

/* void *_thr_getsp()					*/
/*	Return the current stack pointer (for debugging)*/
/*							*/
/* Calling/Exit state:					*/
/*	Called without holding any locks.		*/

	.globl	_thr_getsp
	.align	4
_fgdef_(_thr_getsp):
	leal	4(%esp), %eax
	ret

/* void *_thr_getfp()					*/
/*	Return the current frame pointer (for debugging)*/
/*							*/
/* Calling/Exit state:					*/
/*	Called without holding any locks.		*/

	.globl	_thr_getfp
	.align	4
_fgdef_(_thr_getfp):
	movl	%ebp, %eax
	ret

/* void *_thr_getpc()					*/
/*	Return the current pc of getpc()'s caller	*/
/*							*/
/* Calling/Exit state:					*/
/*	Called without holding any locks.		*/

	.globl	_thr_getpc
	.align	4
_fgdef_(_thr_getpc):
	movl	0(%esp), %eax
	ret

/* void *_thr_caller()					*/
/*	Return the address of our caller's caller.	*/
/*							*/
/* Calling/Exit state:					*/
/*	Called without holding any locks.		*/

	.globl	_thr_caller
	.align	4
_fgdef_(_thr_caller):
	movl	4(%ebp), %eax
	ret

/* void _thr_debug_notify(void *thread_map, enum thread change type) */
/* Inform debugger about changes in thread state. 	*/
/* The debugger can set a breakpoint here		*/
/* to find out about state change events.		*/

	.globl	_thr_debug_notify
	.align	4
	.text
_fgdef_(_thr_debug_notify):
	ret
