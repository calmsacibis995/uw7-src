#ident	"@(#)kern-i386at:svc/sysdump.cf/Stubs.c	1.1.1.1"
#ident	"$Header$"

#include <sys/types.h>

#define NOPAGE	((ppid_t)-1)

void sysdump_init(void) {}
void sysdump(void) {}
ppid_t dkvtoppid32(caddr_t vaddr, int engnum) { return NOPAGE; }
ppid_t pae_dkvtoppid(caddr_t vaddr, int engnum) { return NOPAGE; }
