/*	Copyright (c) 1993 UNIVEL */

#ident	"@(#)Space.c	10.1"
#ident	"$Header$"

#include	<sys/types.h>
#include	<sys/stream.h>
#include	<sys/sr.h>


extern int	sr_insert_age();		/* update table with new source
						 * route information only if
						 * if the entry has aged sr_age
						 * ticks
						 */
extern int	sr_insert_latest();		/* update table with latest
						 * source route information
						 */

int	(*sr_insert)() = sr_insert_latest;	/* use the latest source route
						 * information
						 */

#define		ARPT_KILLC	(20 * 60)	/* from ARP - kill completed
						 * entry in 20 mins.
						 */

int	sr_age = ARPT_KILLC;			/* used only if sr_insert is
						 * set to sr_insert_age
						 */

int	sr_broadcast = SINGLE_ROUTE_BCAST;	/* change to ALL_ROUTE_BCAST if
						 * needed
						 */

