.ident	"@(#)libcg:i386/lib/libcg/i386/cgmemloc.s	1.1"

/
/ int
/ cg_memloc(caddr_t addr, size_t  len, cgid_t *vec)
/       Determine physical location of pages.
/
/ Calling/Exit State:
/       On success 0 is returned, and on failure errno is returned.
/

	.file   "cgmemloc.s"

	.text

	.globl	_cerror

	.globl	cg_memloc

	.type 	cg_memloc,@function
cg_memloc:
	MCOUNT                  / subroutine entry counter if profiling
	movl	$CGMEMLOC,%eax
	lcall	$0x7,$0
	jc 	_cerror
	ret
