	.file	"cerror.s"
	.ident	"@(#)libc-i386:crt/cerror.s	1.11"

/ C return sequence which sets errno, returns -1.

_m4_define_(`EINTR', 4)

_m4_ifdef_(`GEMINI_ON_OSR5',`
/
/create a table mapping OSR5 error numbers to Gemini versions
/
_m4_define_(`ELOOP', 90)
_m4_define_(`ESTRPIPE', 92)
_m4_define_(`ENOTEMPTY', 93)
_m4_define_(`ENOTSOCK', 95)
_m4_define_(`EDESTADDRREQ', 96)
_m4_define_(`EMSGSIZE', 97)
_m4_define_(`EPROTOTYPE', 98)
_m4_define_(`ENOPROTOOPT', 99)
_m4_define_(`EPROTONOSUPPORT', 120)
_m4_define_(`ESOCKTNOSUPPORT', 121)
_m4_define_(`EOPNOTSUPP', 122)
_m4_define_(`EPFNOSUPPORT', 123)
_m4_define_(`EAFNOSUPPORT', 124)
_m4_define_(`EADDRINUSE', 125)
_m4_define_(`EADDRNOTAVAIL', 126)
_m4_define_(`ENETDOWN', 127)
_m4_define_(`ENETUNREACH', 128)
_m4_define_(`ENETRESET', 129)
_m4_define_(`ECONNABORTED', 130)
_m4_define_(`ECONNRESET', 131)
_m4_define_(`ENOBUFS', 132)
_m4_define_(`EISCONN', 133)
_m4_define_(`ENOTCONN', 134)
_m4_define_(`ESHUTDOWN', 143)
_m4_define_(`ETOOMANYREFS', 144)
_m4_define_(`ETIMEDOUT', 145)
_m4_define_(`ECONNREFUSED', 146)
_m4_define_(`EHOSTDOWN', 147)
_m4_define_(`EHOSTUNREACH', 148)
_m4_define_(`EWOULDBLOCK', 11)
_m4_define_(`EALREADY', 149)
_m4_define_(`EINPROGRESS', 150)
_m4_define_(`_SCO_EWOULDBLOCK', 90)
_m4_define_(`_SCO_ESTRPIPE', 153)
	.section	.rodata,"a"
.err_map:
	/ table begins with value 90, _SCO_EWOULDBLOCK
	.byte EWOULDBLOCK
	.byte EINPROGRESS
	.byte EALREADY
	.byte ENOTSOCK
	.byte EDESTADDRREQ
	.byte EMSGSIZE
	.byte EPROTOTYPE
	.byte EPROTONOSUPPORT
	.byte ESOCKTNOSUPPORT
	.byte EOPNOTSUPP
	.byte EPFNOSUPPORT
	.byte EAFNOSUPPORT
	.byte EADDRINUSE
	.byte EADDRNOTAVAIL
	.byte ENETDOWN
	.byte ENETUNREACH
	.byte ENETRESET
	.byte ECONNABORTED
	.byte ECONNRESET
	.byte 109
	.byte EISCONN
	.byte ENOTCONN
	.byte ESHUTDOWN
	.byte ETOOMANYREFS
	.byte ETIMEDOUT
	.byte ECONNREFUSED
	.byte EHOSTDOWN
	.byte EHOSTUNREACH
	.byte ENOPROTOOPT
	.byte 119
	.byte 120
	.byte 121
	.byte 122
	.byte 123
	.byte 124
	.byte 125
	.byte 126
	.byte 127
	.byte 128
	.byte 129
	.byte 130
	.byte 131
	.byte 132
	.byte 133
	.byte 134
	.byte 135
	.byte 136
	.byte 137
	.byte 138
	.byte 139
	.byte 140
	.byte 141
	.byte 142
	.byte 143
	.byte 144
	.byte ENOTEMPTY
	.byte 146
	.byte 147
	.byte 148
	.byte 149
	.byte ELOOP
	.byte 151
	.byte 152
	.byte ESTRPIPE
',`')

	.globl	_cerror
	.text

_fgdef_(_cerror):
	_prologue_
	/
	/ If the error is ERESTART, then this is a system call that wants
	/ to EINTR out on ERESTART.  We replace ERESTART with EINTR.
	/
	cmpl	$ERESTART, %eax
	jne	.L1
	movl	$EINTR, %eax
_m4_ifdef_(`GEMINI_ON_OSR5',`
	jmp	.Lend
',`')
.L1:
_m4_ifdef_(`GEMINI_ON_OSR5',`
	/ If running on OSR5, map OSR5 errnos to Gemini versions.
	/ First, get ourselves within range of the errnos that
	/ may need mapping.
	cmpl	$_SCO_EWOULDBLOCK, %eax
	jl	.Lend
	cmpl	$_SCO_ESTRPIPE, %eax
	jg	.Lend

	/ Within range; now check for cases that need to be mapped.
	movzbl	.err_map@GOTOFF-_SCO_EWOULDBLOCK (%ebx,%eax),%eax
.Lend:
',`')
_m4_ifdef_(`_REENTRANT',`
	/
	/ (*__thr_errno()) = err
	/
	pushl	%eax
	call	_fref_(__thr_errno)
	popl	%ecx
	movl	%ecx,(%eax)
',`_m4_ifdef_(`DSHLIB',`
	movl	_daref_(errno),%ecx
	movl	%eax,(%ecx)
',`
	movl	%eax,errno
')')
	movl	$-1,%edx	/ in case calling function returns
				/ long long
	movl	$-1,%eax
	_epilogue_
	ret
	.size	_cerror,.-_cerror
