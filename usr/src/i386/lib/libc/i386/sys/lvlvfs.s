/ C library -- lvlvfs
.ident	"@(#)libc-i386:sys/lvlvfs.s	1.2"

/ lvlvfs(const char *path, int cmd, level_t *hilevelp)

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.globl	_cerror

_fwdef_(`lvlvfs'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$LVLVFS,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
