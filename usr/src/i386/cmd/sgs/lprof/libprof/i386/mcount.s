	.file	"mcount.s"
	.ident	"@(#)lprof:libprof/i386/mcount.s	1.2"

/ Each caller has reserved a unique pointer to refer to its
/ calls to _mcount.  The address of this pointer is in %edx.
/ The first time for any call to _mcount, the pointer is set to
/ refer to the (long) counter member of its own Cnt structure
/ The next entry in the structure (+4 bytes) is a pointer to
/ the associated SO entry.
/ For subsequent calls, we increment the counter and set
/ _curr_SO to reflect the most recently referenced SO.

	.comm	_curr_SO, 4, 4

	.globl	_mcount
_mcount:
	movl	(%edx), %eax	/ pointer to counter in %edx
	testl	%eax, %eax
	jz	.init		/ first time for this counter
	incl	(%eax)
	movl	4(%eax), %edx
	movl	%edx, _curr_SO	/ note most recent SO
	ret

.init:
	pushl	%edx
	call	_mcountNewent	/ passed pointer and address
	popl	%edx
	ret

	.type	_mcount, "function"
	.size	_mcount, .-_mcount
