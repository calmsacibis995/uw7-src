.ident	"@(#)libcg:i386/lib/libcg/i386/cgcurrent.s	1.1"

/
/ int
/ cg_current(idtype_t idtype, id_t id)
/       Get the CG id of target.
/
/ Calling/Exit State:
/       On success 0 is returned, and on failure errno is returned.
/

	.file   "cgcurrent.s"

	.text

	.globl	_cerror

	.globl	cg_current

	.type	cg_current,@function
cg_current:
	MCOUNT                  / subroutine entry counter if profiling
	movl	$CGCURRENT,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret

