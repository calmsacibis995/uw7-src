#ident	"@(#)kern-i386:util/kdb/scodb/bkp_extra.c	1.1.1.1"
#ident  "$Header$"

/*
 * Alot of TBD's in this file!
 */

#include "bkp.h"

struct bp_values bp_values[1];

int ICE = 0;
int eipsave;			/* TBD - not MPX safe */
int max_ACPUs = 0;
int cpu_family = 5;
int processor_index = 0;

ismpx()
{
	return (0);
}

broadcast()
{}

crllry_index()
{
	return(0);
}

scodb_enter()
{}

scodb_exit()
{}

check_tfe()
{
	return -1;
}
