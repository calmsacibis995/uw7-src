	.ident	"@(#)libc-i386:fp/fpsetmask.s	1.1"
	.file	"fpsetmask.s"
	.text
	.align	16
_fwdef_(`fpsetmask'):
	subl	$28,%esp	// 28 bytes to hold FPU regs (CW and SW)
	fnstenv	(%esp)
	movl	32(%esp),%ecx	// arg to fpsetmask
	andl	$63,%ecx
	notl	%ecx
	andl    %ecx,4(%esp)	// clear bits in low order byte of SW
	movl	(%esp),%eax	
	andl	$-64,(%esp)
	notl	%eax
	andl	$63,%eax	// for return of old mask in %eax
	andl	$63,%ecx	// set mask to
	orl	%ecx,(%esp)	//     show changes
	fldenv	(%esp)		// reload the changed reg set
	addl	$28,%esp
	ret	
