#ident	"@(#)kern-i386at:util/util.cf/Space.c	1.7.1.1"
#ident	"$Header$"

#include <config.h>
#include <sys/types.h>

char putbuf[PUTBUFSZ];
int putbufsz = PUTBUFSZ;
int sanity_clk = SANITYCLK;
short maxlink = MAXLINK;
