.ident	"@(#)libc-i386:sys/lwpcontinue.s	1.1"


/
/ int
/ _lwp_continue(lwpid_t continueid)
/	Get the target LWP going.
/
/ Calling/Exit State:
/	On success 0 returned, and on failure errno is returned.
/
	.file	"lwpcontinue.s"
	
	.text


_fwdef_(`_lwp_continue'):
	MCOUNT
	movl	$LWPCONTINUE,%eax
	lcall	$0x7,$0
	ret
