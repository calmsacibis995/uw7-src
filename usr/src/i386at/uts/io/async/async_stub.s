	.ident	"@(#)kern-i386at:io/async/async_stub.s	1.1"
	.ident	"$Header$"
/
/ This file contains the stubs for async.
/
include(../../util/mod/stub.m4)

MODULE(async, STUB_LOADONLY)
WSTUB(async, aio_intersect, mod_zero)
WSTUB(async, aio_as_free, mod_zero)
END_MODULE(async)
