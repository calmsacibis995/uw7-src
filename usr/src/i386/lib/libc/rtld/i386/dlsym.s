	.ident	"@(#)rtld:i386/dlsym.s	1.2"
	.file	"dlsym.s"

/ void *dlsym(void *handle, const char *name);
/ Assembly language interface to dlsym - determines address
/ of function calling dlsym and passes it to the real
/ dlsym implementation:
/ void *_rt_real_dlsym(void *caller, void *handle, const char *name);

	.weak	dlsym
	.type	_dlsym,@function
	.text
	.set	dlsym,_dlsym
	.globl	_dlsym
	.align	16
_dlsym:
	pushl	8(%esp)		/ name
	pushl	8(%esp)		/ handle
	pushl	8(%esp)		/ return address
	call	_rt_real_dlsym	/ we assume _rt_real_dlsym is
				/ compiled with -Bsymbolic
				/ and so the call does not need to go
				/ through the GOT/PLT
	addl	$12,%esp
	ret	
	.align	4
	.size	_dlsym,.-_dlsym
	.text
