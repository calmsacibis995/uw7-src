#ident	"@(#)kern-i386:svc/fpe.cf/Stubs.c	1.3"
#ident	"$Header$"

#include <sys/types.h>

vaddr_t EM80387;

void fpesetvec(void) {}
boolean_t fpeclean(void) { return B_FALSE; }
