.ident	"@(#)rtld:i386/syscalls.s	1.9"

	.text

	.globl	_rtclose
_fgdef_(_rtclose):
	MCOUNT
	movl	$CLOSE,%eax
	lcall	$0x7,$0
	jc	.rterror
	xorl	%eax,%eax
	ret
	.text


	.globl	_rtexit
_fgdef_(_rtexit):
	MCOUNT
	movl	$EXIT,%eax
	lcall	$0x7,$0

	.globl	_rtcompegid
_fgdef_(_rtcompegid):
	MCOUNT
	movl	$GETGID,%eax
	lcall	$0x7,$0
	cmpl	%edx,%eax 
	je	.gidsareequal
	movl	$0,%eax
	ret
.gidsareequal:
	movl	$1,%eax
	ret

	.globl	_rtcompeuid
_fgdef_(_rtcompeuid): 
	MCOUNT
	movl	$GETUID,%eax
	lcall	$0x7,$0
	cmpl	%edx,%eax 
	jne	.uidsnotequal
	ret
.uidsnotequal:
	movl	$-1,%eax
	ret

	.globl	_rtgetpid
_fgdef_(_rtgetpid):
	MCOUNT
	movl	$GETPID,%eax
	lcall	$0x7,$0
	ret

	.globl	_rtkill
_fgdef_(_rtkill):
	MCOUNT
	movl	$KILL,%eax
	lcall	$0x7,$0
	jc	.rterror
	xorl	%eax,%eax
	ret


	.globl	_rtmmap
_fgdef_(_rtmmap):
	MCOUNT
	movl	$MMAP,%eax
	lcall	$0x7,$0
	jc	.rterror
	ret

	.globl	_rtmprotect
_fgdef_(_rtmprotect):
	MCOUNT
	movl	$MPROTECT,%eax
	lcall	$0x7,$0
	jc	.rterror
	xorl	%eax,%eax
	ret

	.globl	_rtmunmap
_fgdef_(_rtmunmap):
	MCOUNT
	movl	$MUNMAP,%eax
	lcall	$0x7,$0
	jc	.rterror
	xorl	%eax,%eax
	ret

	.globl	_rtopen
_fgdef_(_rtopen):
	MCOUNT
	movl	$OPEN,%eax
	lcall	$0x7,$0
	jc	.rterror
	ret

_m4_ifdef_(`GEMINI_ON_OSR5',`
_m4_define_(`ERESTART', 152)
',`
_m4_define_(`ERESTART', 91)
')
	.globl	_rtwrite
_fgdef_(_rtwrite):
	MCOUNT
	movl	$WRITE,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je	_rtwrite
	jmp	.rterror
.noerror:
	ret


	.globl	_rtsysconfig
_fgdef_(_rtsysconfig):
	MCOUNT
	movl	$SYSCONFIG,%eax
	lcall	$0x7,$0
	jc	.rterror
	ret

/ procpriv(int cmd, priv_t *privp, int count)
	.globl	_rtprocpriv
_fgdef_(_rtprocpriv):
        MCOUNT
        movl    $PROCPRIV,%eax
        lcall   $0x7,$0
        jc      .rterror
        ret

	.globl	_rtsecsys
_fgdef_(_rtsecsys):
        MCOUNT
        movl    $SECSYS,%eax
        lcall   $0x7,$0
        jc      .rterror
        ret

	.globl	_rtfxstat
_fgdef_(_rtfxstat):
	MCOUNT
	movl	$FXSTAT,%eax
	lcall	$0x7,$0
	jc	.rterror
	xorl	%eax,%eax
	ret

	.globl	_rtxstat
_fgdef_(_rtxstat):
	MCOUNT
	movl	$XSTAT,%eax
	lcall	$0x7,$0
	jc	.rterror
	xorl	%eax,%eax
	ret

	.globl	_rtsysi86
_fgdef_(_rtsysi86):
	MCOUNT
	movl	$SYSI86,%eax
	lcall	$0x7,$0
	jc	.rterror
	ret

.rterror:
	// rtld ignores errno...don't bother setting it
	movl	$-1,%eax
	ret
