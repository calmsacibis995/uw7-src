.ident	"@(#)libc-i386:sys/lwpinfo.s	1.1"

/
/ int
/ _lwp_info(struct lwp_info *buffer)
/	Returns the utime and stime of the calling LWP in an out parameter.
/
/ Calling/Exit State:
/	On success 0 is returned, and on failure errno is returned.
/
	
	.file	"lwpinfo.s"
	
	.text


_fwdef_(`_lwp_info'):
	MCOUNT
	movl	$LWPINFO,%eax
	lcall	$0x7,$0
	ret
