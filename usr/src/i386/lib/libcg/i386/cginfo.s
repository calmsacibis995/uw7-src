.ident	"@(#)libcg:i386/lib/libcg/i386/cginfo.s	1.1"

/
/ int
/ cg_info(cgid_t cgid, cpugroup_info_t *infop)
/       Returns information about a single CPU-group in the system.
/
/ Calling/Exit State:
/       On success 0 is returned, and on failure errno is returned.
/

	.file   "cginfo.s"

	.text

	.globl	_cerror

	.globl	cg_info

	.type 	cg_info,@function
cg_info:
	MCOUNT                  / subroutine entry counter if profiling
	movl	$CGINFO,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret

