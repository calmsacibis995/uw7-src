.ident	"@(#)libcg:i386/lib/libcg/i386/cgids.s	1.1"

/
/ int
/ cg_ids(cgstatus_t selector, unsigned int ncgids, cgid_t *cgid_array)
/       Returns the CPU-groups in the system.
/
/ Calling/Exit State:
/       On success 0 is returned, and on failure errno is returned.
/

	.file   "cgids.s"

	.text

	.globl  _cerror

	.globl  cg_ids

	.type   cg_ids,@function
cg_ids:
	MCOUNT                  / subroutine entry counter if profiling
	movl	$CGIDS,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret

