#ident	"@(#)kern-i386at:svc/svc.cf/Space.c	1.2.3.1"
#ident	"$Header$"

#include <config.h>

/* Time variables for XENIX-style ftime() system call. */
int Timezone = TIMEZONE;	/* tunable time zone for XENIX ftime() */
int Dstflag = DSTFLAG;		/* tunable daylight time flag for XENIX */

/*
 * FPKIND: type of floating-point support: 0 = auto-detect,
 *	  1 = software emulator, 2 = Intel287 coproc, 3 = Intel387 coproc
 */
int fp_kind = FPKIND;

/*
 * Tuneable to enable/disable memory to be configured above 4G
 * on quad p6 with Orion chipset.
 */
unsigned int p6splitmem = 0;
