#ifndef _IO_TARGET_SW01_SW01_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SW01_SW01_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/sw01/sw01.h	1.10.1.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*	Copyright (c) 1988, 1989  Intel Corporation	*/
/*	All Rights Reserved	*/

/*      INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied under the terms of a license agreement   */
/*	or nondisclosure agreement with Intel Corporation and may not be   */
/*	copied or disclosed except in accordance with the terms of that    */
/*	agreement.							   */

/*	Copyright (c) 1989 TOSHIBA CORPORATION		*/
/*		All Rights Reserved			*/

/*	Copyright (c) 1989 SORD COMPUTER CORPORATION	*/
/*		All Rights Reserved			*/

#ifdef _KERNEL_HEADERS

#include <fs/buf.h>		/* REQUIRED */
#include <io/target/scsi.h>	/* REQUIRED */
#include <io/target/sdi/sdi.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/buf.h>		/* REQUIRED */
#include <sys/scsi.h>		/* REQUIRED */
#include <sys/sdi.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * the SCSI WORM minor device number is interpreted as follows:
 *
 *     bits:
 *	 7    4 3  0
 * 	+------+----+
 * 	|      |unit|
 * 	+------+----+
 *
 *     codes:
 *	unit  - unit no. (0 - 7)
 */

#define UNIT(x)		(geteminor(x) & 0xFF)
#define WM_MINORS_PER		1	/* number of minors per unit */

#define WMNOTCS		-1		/* No TC's configured in system	*/

#define BLKSIZE  512	/* default block size */
#define BLKSHFT  9
#define BLKMASK  (BLKSIZE-1)

#define JTIME	10000	/* ten sec for a job */
#define LATER   20000

#define MAXPEND   2
#define MAXRETRY  2

#define INQ_SZ	126	/* inquiry data size */
#define CB_SZ	1024	/* control block data size */

/* Values of dk_state */
#define	WM_INIT		0x01		   /* Disk has been initialized  */
#define	WM_WOPEN	0x02		   /* Waiting for 1st open	 */
#define	WM_DIR		0x08		   /* Elevator direction flag	 */
#define	WM_SUSP		0x10		   /* Disk Q suspended by HA	 */
#define	WM_SEND		0x20		   /* Send requested timeout	 */
#define	WM_PARMS	0x40		   /* Disk parms set and valid	 */

#define	GROUP0		0		/* group 0 command 		*/
#define	GROUP1		1		/* group 1 command 		*/
#define	GROUP6		6		/* group 6 command 		*/
#define	GROUP7		7		/* group 7 command 		*/

/*
 * Read Capacity data
 */
struct capacity {
	unsigned long    wm_addr;	   /* Logical block address	*/
	unsigned long    wm_len;	   /* Block length	 	*/
};

#define RDCAP_SZ        8
#define RDCAP_AD(x)     ((char *)(x))

/*
 * Job structure
 */
struct job {
	struct job       *j_next;	   /* Next job on queue		 */
	struct job       *j_prev;	   /* Previous job on queue	 */
	struct job	 *j_priv;	   /* private pointer for dynamic  */
					   /* alloc routines DON'T USE IT  */
	struct job       *j_cont;	   /* Next job block of request  */
	struct sb        *j_sb;		   /* SCSI block for this job	 */
	buf_t	         *j_bp;		   /* Pointer to buffer header	 */
	struct worm      *j_wmp;	   /* Device to be accessed	 */
	unsigned	  j_errcnt;	   /* Error count (for recovery) */ 
	daddr_t		  j_addr;	   /* Physical block address	 */
	union sc {			   /* SCSI command block	 */
		struct scs  ss;		/*	group 0,6 (6 bytes)	*/
		struct scm  sm;		/*	group 1,7 (10 bytes)	*/
	} j_cmd;
};

/*
 * worm information structure
 */
struct worm {
	struct job       *wm_first;	    /* Head of job queue	 */
	struct job       *wm_last;	    /* Tail of job queue	 */
	struct job       *wm_next;	    /* Next job to send to HA	 */
	struct job       *wm_batch;	    /* Elevator batch pointer	 */
	struct scsi_ad	  wm_addr;	    /* SCSI address		 */
	unsigned long	  wm_sendid;	    /* Timeout id for send	 */
	unsigned  	  wm_state;	    /* Operational state	 */ 
	unsigned  	  wm_count;	    /* Number of jobs on Q	 */ 
	unsigned 	  wm_npend;	    /* Number of jobs sent 	 */ 
	unsigned	  wm_fltcnt;	    /* Retry cnt for fault jobs	 */
	struct job       *wm_fltjob;	    /* Job associated with fault */
	struct sb        *wm_fltreq;	    /* SB for request sense	 */
	struct sb        *wm_fltres;	    /* SB for resume job	 */
	struct scs	  wm_fltcmd;	    /* Request Sense command	 */
	struct sense	 *wm_sense;	    /* Request Sense data	 */
	struct capacity	 *wm_capacity;	    /* Read Capacity data	 */
	struct dev_spec  *wm_spec;
	uint_t		  wm_blkshft;	    /* BlK <-> Log Sect shift cnt*/
	bcb_t		 *wm_bcbp;	    /* Breakup control block	 */
	char		  wm_iotype;	    /* Device I/O capability	 */
	lock_t           *wm_lock;
	pl_t             wm_pl;
	sv_t             *wm_sv;
};

extern struct dev_spec *sw01_dev_spec[];/* pointers to helper structs	*/
extern struct dev_cfg SW01_dev_cfg[];	/* configurable devices struct	*/
extern int SW01_dev_cfg_size;		/* number of dev_cfg entries	*/

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_SW01_SW01_H */
