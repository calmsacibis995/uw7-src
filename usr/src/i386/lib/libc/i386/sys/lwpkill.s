.ident	"@(#)libc-i386:sys/lwpkill.s	1.1"

/
/ int
/ _lwp_kill(lwpid_t lwpid, int sig)
/	The _lwp_kill(2) system call. Posts the signal to the specified
/	LWP.
/
/ Calling/Exit State:
/	On success 0 is returned and on failure errno is returned.
/


	.file	"lwpkill.s"

	.text


_fwdef_(`_lwp_kill'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$LWPKILL,%eax
	lcall	$0x7,$0
	ret
