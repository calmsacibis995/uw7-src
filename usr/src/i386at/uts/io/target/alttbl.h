#ifndef _IO_TARGET_ALTTBL_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_ALTTBL_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/alttbl.h	1.4"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * ALTTBL.H
 *
 * This file defines the bad block table for the hard disk driver.
 *	The same table structure is used for the bad track table.
 */

#define MAX_ALTENTS     253	/* Maximum # of slots for alts	*/
				/* allowed for in the table.	*/

#define ALT_SANITY      0xdeadbeef      /* magic # to validate alt table */
#define ALT_VERSION	0x02		/* version of table 		 */

struct alt_table {
	ushort_t alt_used;	/* # of alternates already assigned	*/
	ushort_t alt_reserved;	/* # of alternates reserved on disk	*/
	daddr_t alt_base;	/* 1st sector (abs) of the alt area	*/
	daddr_t alt_bad[MAX_ALTENTS];	/* list of bad sectors/tracks	*/
};

struct alt_info {	/* table length should be multiple of 512	*/
	long    alt_sanity;	/* to validate correctness		*/
	ushort_t alt_version;	/* to corroborate vintage		*/
	ushort_t alt_pad;	/* padding for alignment		*/
	struct alt_table alt_trk;	/* bad track table	*/
	struct alt_table alt_sec;	/* bad sector table	*/
};

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_ALTTBL_H */
