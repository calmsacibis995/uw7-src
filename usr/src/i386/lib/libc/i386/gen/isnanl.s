	.file	"isnanl.s"

	.ident	"@(#)libc-i386:gen/isnanl.s	1.2"

/	int isnanl(srcD)
/	long double srcD;
/
/	This routine returns 1 if the argument is a NaN
/		     returns 0 otherwise.

	.set	DMAX_EXP,0x7fff

	.text
	.align	4

_fwdef_(`isnanl'):
	MCOUNT
	movl	10(%esp),%eax
	andl	$0x7fff0000,%eax	/ bits 62-52
	cmpl	$[DMAX_EXP<<16],%eax
	jne	.false

	movl	8(%esp),%eax
	andl	$0xffffffff,%eax	/ bits 51-32
	orl	4(%esp),%eax		/ bits 31-0
	cmpl	$0x80000000,%eax
	jz	.false			/ all fraction bits are 0 excect MSB
					/ its an infinity
	movl	$1,%eax
	ret
.false:
	movl	$0,%eax
	ret
