#ident	"@(#)kern-i386:fs/s5fs/s5.cf/Space.c	1.3.2.1"

#ifndef _FSKI
#define _FSKI	1
#endif

#include <config.h>

int ninode;			/* now autotuned */
int s5_inode_lwm = S5INODELWM;	/* low-water mark */
int s5_tflush = S5FSFLUSH;	/* the frequency of flush daemon */

