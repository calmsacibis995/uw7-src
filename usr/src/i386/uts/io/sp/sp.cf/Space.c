#ident	"@(#)Space.c	1.2"
#ident	"$Header$"

#include <config.h>	/* to collect tunable parameters */
#include <sys/types.h>
#include <sys/stream.h>

/*
 * WARNING: Original code had this defined in sp.c. It is still defined there.
 * We define it here so we can declare sp_sp.
 */
struct sp {
	queue_t *sp_rdq;		/* this stream's read queue */
	queue_t *sp_ordq;		/* other stream's read queue */
};

struct sp	sp_sp[SP_UNITS];
int		spcnt = SP_UNITS;
