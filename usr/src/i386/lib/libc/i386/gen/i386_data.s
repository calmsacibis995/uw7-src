	.file   "i386_data.s"

	.ident	"@(#)libc-i386:gen/i386_data.s	1.5"

/ This file contains
/ the definition of the
/ global symbols errno and _siguhandler
/ 
/ int errno;

	.globl	errno
	.comm	errno,4

	.section .rodata
	.align	4
	.globl	_siguhandler
_dgdef2_(_siguhandler,128):	/ only used by (old) libucb.a's
	.zero	128		/ 32 * 4 (cannot increase this)
