#ident	"@(#)kern-i386:fs/sfs/sfs.cf/Space.c	1.2.2.1"

#ifndef _FSKI
#define _FSKI	1
#endif

#include <config.h>

int sfs_ninode;      		 /* now autotuned */
int sfs_inode_lwm = SFSINODELWM; /* low-water mark */
int sfs_timelag = SFSTIMELAG;    /* Heuristic: how many ticks before 
				  * encouraging recycling of an inactive 
				  * inode */
int sfs_tflush = SFSFLUSH;	 /* flush time parameter is NAUTOUP */
int sfs_ndquot = NDQUOT;	 /* size of quota table */
