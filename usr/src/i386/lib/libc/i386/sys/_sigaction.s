.ident	"@(#)libc-i386:sys/_sigaction.s	1.3"

/ SYS library -- _sigaction
/ error = _sigaction(sig, act, oact, handler);

	
	.file "_sigaction.s"
	
	.text

	.globl	__sigaction
	.globl	_cerror
_m4_ifdef_(`GEMINI_ON_OSR5',`
	.globl	_sigacthandler
',`')

_fgdef_(__sigaction):
	_prologue_
	MCOUNT
	movl	$SIGACTION,%eax
_m4_ifdef_(`GEMINI_ON_OSR5',`
	movl	_daref_(_sigacthandler),%edx
',`')
	_epilogue_
	lcall	$0x7,$0
	jc	_cerror
	ret

_m4_ifdef_(`GEMINI_ON_OSR5',`
_sigacthandler:
	addl	$4,%esp		/ remove args to user interrupt routine
	lcall	$0xF,$0		/ return to kernel to return to user
',`')
