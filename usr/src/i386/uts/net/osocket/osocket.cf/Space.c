#ident	"@(#)Space.c	1.2"
#ident	"$Header$"

/* Enhanced Application Compatibility Support */

#include <sys/osocket.h>

#define ONSOCK 100

int num_osockets = ONSOCK;
struct osocket *osocket_tab[ONSOCK];
char osoc_domainbuf[OMAXHOSTNAMELEN] = { 0 };

/* Enhanced Application Compatibility Support */
