/
/ This file contains the stubs for ccnv.
/
	.ident	"@(#)kern-i386:util/ccnv/ccnv_stub.s	1.3.1.1"
include(../../util/mod/stub.m4)

MODULE(ccnv, STUB_UNLOADABLE)
USTUB(ccnv, ccnv_unix2dos, mod_enoload,4)
USTUB(ccnv, ccnv_dos2unix, mod_enoload,4)
USTUB(ccnv, dos2unixfn, mod_enoload,3)
USTUB(ccnv, unix2dosfn, mod_enoload,3)
END_MODULE(ccnv)
