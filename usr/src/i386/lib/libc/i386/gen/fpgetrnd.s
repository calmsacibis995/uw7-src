/ fpgetrnd() returns the 80[23]87 coprocessor's current rounding mode.

	.file	"fpgetrnd.s"
	.ident	"@(#)libc-i386:gen/fpgetrnd.s	1.6"
	.text
	.set	FPRC, 0xC00	/ FPRC (rounding control) from <sys/fp.h>
	.align	4
_fwdef_(`fpgetrnd'):
	pushl	%ecx
	fstcw	0(%esp)
	movl	0(%esp),%eax
	andl	$FPRC,%eax
	popl	%ecx
	ret	
	.align	4
	.size	fpgetrnd,.-fpgetrnd
