.ident	"@(#)libc-i386:sys/waitid.s	1.3"

/ C library -- waitid

/ error = waitid(idtype,id,&info,options)

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.file	"waitid.s"
	
	.text


	.globl  _cerror

_fwdef_(`waitid'):
	MCOUNT
	movl	$WAITID,%eax
	lcall	$0x7,$0
	jae 	.noerror		/ all OK - normal return
	cmpb	$ERESTART,%al	/  else, if ERRESTART
	je	waitid		/    then loop
	jmp 	_cerror		/  otherwize, error

.noerror:
	ret
')
