#ident	"@(#)Space.c	1.2"
#ident	"$Header$"

#include <sys/emap.h>	/* channel mapping include file from XENIX */
#include <sys/nmap.h>	/* channel mapping include file from XENIX */

#include "config.h" 	/* to get NEMAP tunaebale from mtune */

struct	emap	emap[NEMAP];	/* channel mapping data struct table */
struct	nxmap	nxmap[NEMAP];	/* channel mapping data struct table */


