#ident	"@(#)kern-i386:fs/xxfs/xx.cf/Space.c	1.1.2.1"
#ident	"$Header$"

#ifndef _FSKI
#define _FSKI	1
#endif

#include <config.h>

#define XXNINODE	100	/* Number of allowable XENIX inodes */
long int xx_ninode = XXNINODE;	/* Maximum number of inodes */
int	xx_tflush = XXFSFLUSH;	/* Flush time interval */
