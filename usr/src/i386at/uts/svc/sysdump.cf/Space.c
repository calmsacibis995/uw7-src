#ident	"@(#)kern-i386at:svc/sysdump.cf/Space.c	1.2"

#include <config.h>	/* to collect tunable parameters */
#include <sys/types.h>

int sysdump_selective = SYSDUMP_SELECTIVE;
int sysdump_poll = SYSDUMP_POLL;
