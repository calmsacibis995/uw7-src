.ident	"@(#)libcg:i386/lib/libcg/i386/cgprocs.s	1.1"

/
/ int
/ cg_processors(cgid_t cgid, prstatus_t selector, unsigned np, processorid_t *parray)
/       Returns processors in a CPU-group.
/
/ Calling/Exit State:
/       On success 0 is returned, and on failure errno is returned.
/

	.file   "cgprocs.s"

	.text

	.globl	_cerror

	.globl	cg_processors

	.type cg_processors,@function
cg_processors:
	MCOUNT                  / subroutine entry counter if profiling
	movl	$CGPROCS,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret

