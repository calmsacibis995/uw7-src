#ident	"@(#)kern-i386:io/prf/prf.cf/Space.c	1.1"
#ident	"$Header$"

#include <config.h>
/*
 * PRFMAX limits the amount of memory that may be allocated 
 * for the kernel profiler.  Setting it to 0 makes it unlimited.
 */
int maxprf = PRFMAX;
