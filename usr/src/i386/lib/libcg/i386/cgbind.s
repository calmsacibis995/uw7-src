.ident	"@(#)libcg:i386/lib/libcg/i386/cgbind.s	1.1"


/
/ int
/ cg_bind(idtype_t idtype, id_t id, cgid_t cgid, int flags, cgid_t *ocgid, int *oflags)
/	Bind a process/LWP to a CPU-group.
/       
/
/ Calling/Exit State:
/       On success 0 is returned, and on failure errno is returned.
/

	.file   "cgbind.s"

	.text

	.globl  _cerror

	.globl  cg_bind

	.type cg_bind,@function
cg_bind:
	MCOUNT                  / subroutine entry counter if profiling
	movl	$CGBIND,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret

