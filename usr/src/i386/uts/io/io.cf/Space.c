#ident	"@(#)kern-i386:io/io.cf/Space.c	1.9"

#include <config.h>	/* to collect tunable parameters */
#include <sys/types.h>

int nstrpush = NSTRPUSH;
int strmsgsz = STRMSGSZ;
int strctlsz = STRCTLSZ;
int strthresh = STRTHRESH;
int strnsched = STRNSCHED;

major_t maxmajor = MAXMAJOR;
minor_t maxminor = MAXMINOR;

int console_security = CONSOLE_SECURITY;
