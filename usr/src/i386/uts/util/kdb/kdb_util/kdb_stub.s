	.ident	"@(#)kern-i386:util/kdb/kdb_util/kdb_stub.s	1.2.2.1"
	.ident	"$Header$"
/
/ This file contains the stubs for kdb_util.
/
include(KBASE/util/mod/stub.m4)

MODULE(kdb_util, STUB_LOADONLY)
WSTUB(kdb_util, kdb_printf, mod_zero)
WSTUB(kdb_util, kdb_check_aborted, mod_zero)
END_MODULE(kdb_util)
