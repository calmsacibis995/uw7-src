	.ident	"@(#)kern-i386:proc/obj/coff_stub.s	1.2"
	.ident	"$Header$"
/
/ This file contains unloadable DLM stubs
/ for the coff exec type module.
/
include(../../util/mod/stub.m4)

MODULE(coff, STUB_UNLOADABLE)
USTUB(coff, getcoffshlibs, nopkg, 4)
USTUB(coff, coffexec_err, nopkg, 2)
END_MODULE(coff)
