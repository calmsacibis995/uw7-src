	.ident	"@(#)kern-i386:util/lsign.s	1.2.2.1"
        .file   "util/lsign.s"

include(KBASE/svc/asm.m4)

/
/ int
/ lsign(dl_t)
/	This function returns the value of the sign bit of
/	the 64-bit argument (assumes POSITIVE == 0, NEGATIVE == 1).
/
/ Calling State/Exit State:
/	This function assumes caller has provided sufficient locking
/	on the arguments prior to the call.
/
ENTRY(lsign)
	movl	SPARG1, %eax
	roll	%eax
	andl	$1, %eax
	ret
	SIZE(lsign)
