.ident	"@(#)libc-i386:sys/sleep.s	1.2"


/
/ 
/ unsigned sleep(unsigned seconds)
/	Implements the sleep() function.
/
/ Calling/Exit State:
/	Returns the number of seconds not slept.
/
_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.file	"sleep.s"
	
	.text


_fwdef_(`sleep'):
	MCOUNT
	movl	$SLEEP,%eax
	lcall	$0x7,$0
	ret
')
