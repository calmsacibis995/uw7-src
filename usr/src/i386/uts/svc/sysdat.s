	.ident	"@(#)kern-i386:svc/sysdat.s	1.1.2.1"
	.ident	"$Header$"
	.file	"svc/sysdat.s"
 

include(KBASE/svc/asm.m4)
include(assym_include)

	.set hrestime, _A_KVSYSDAT
	.globl hrestime
	.type hrestime, "object"
	.size hrestime, _A_TIMESTR_SIZ
