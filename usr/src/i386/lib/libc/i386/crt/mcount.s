        .file   "mcount.s"

	.ident	"@(#)libc-i386:crt/mcount.s	1.8"

	.globl _mcount
	
	/ If compiling with -p the definition for _mcount will be
	/ resolved with the one in /usr/ccs/lib/libprof.a

_fgdef_(_mcount):
        ret

