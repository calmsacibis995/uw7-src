.ident	"@(#)libc-i386:sys/lwpwait.s	1.3"


/
/ int
/ lwp_wait(lwpid_t wait_for, lwpid_t *departed_lwp)
/	The function waits for the termination of a sibling LWP.
/
/ Calling/Exit State:
/	On successful completion, the ID of the LWP is returned in an 
/	out parameter and 0 is returned. On failure errno is returned.
/
/ Remarks:
/	If "wait_for" is set to zero, then the calling context will wait for 
/	the termination any OTHER context in the process. If not, the 
/	specific LWP is waited for.
/
	.file	"lwpwait.s"
	
	.text



_fwdef_(`_lwp_wait'):
	MCOUNT
	movl	$LWPWAIT,%eax
	lcall	$0x7,$0
	jae 	.noerror		/ all OK - normal return
	cmpb	$ERESTART,%al	/  else, if ERRESTART
	je	_lwp_wait		/    then loop
	ret			/ else return errno

.noerror:
        movl	8(%esp),%ecx	/ departed lwp id is returned in edx 
        testl	%ecx,%ecx
        jz	.return
        movl	%edx,(%ecx)
.return:
	ret
