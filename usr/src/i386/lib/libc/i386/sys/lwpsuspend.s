.ident	"@(#)libc-i386:sys/lwpsuspend.s	1.3"


/
/ int
/ _lwp_suspend(lwpid_t suspendid)
/	Suspend the specified LWP. If the ID is valid, when the caller
/	returns, the target LWP is suspended.
/
/ Calling/Exit State:
/	On success 0 is returned and on failure errno is returned.
/
	.file	"lwpsuspend.s"
	
	.text



_fwdef_(`_lwp_suspend'):
	MCOUNT
	movl	$LWPSUSPEND,%eax
	lcall	$0x7,$0
	jae 	.noerror		/ all OK - normal return
	cmpb	$ERESTART,%al	/  else, if ERRESTART
	je	_lwp_suspend		/    then loop
					/ return errno
.noerror:
	ret
