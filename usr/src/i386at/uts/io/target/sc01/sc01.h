#ifndef _IO_TARGET_SC01_SC01_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SC01_SC01_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/sc01/sc01.h	1.12.2.1"
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

#include <io/target/scsi.h>	/* REQUIRED */
#include <io/target/sdi/sdi.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/scsi.h>		/* REQUIRED */
#include <sys/sdi.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */


/*
 * the SCSI CD-ROM minor device number is interpreted as follows:
 *
 *     bits:
 *	 7         0
 * 	+-----------+
 * 	|   unit    |
 * 	+-----------+
 *
 *     codes:
 *     unit - unit no. (0 - 256)
 */

#define UNIT(x)		(geteminor(x) & 0xFF)
#define CD_MINORS_PER		1	/* number of minors per unit */

#define CDNOTCS		-1		/* No TC's configured in system	*/

#define BLKSIZE  512			/* default block size */
#define BLKMASK  (BLKSIZE-1)

#define JTIME	10000			/* ten sec for a job */
#define LATER   20000

#define MAXPEND   2
#define MAXRETRY  2

#define INQ_SZ		98		/* inquiry data size		*/
#define	RDSTATUS_SZ	10		/* playback status &		*/
					/*  subcode Q address data size	*/

/* Values of cd_state */
#define	CD_INIT		0x01		/* Disk has been initialized  */
#define	CD_WOPEN	0x02		/* Waiting for 1st open	 */
#define	CD_DIRECTION	0x04		/* Elevator direction flag	 */
#define	CD_SUSP		0x08		/* Disk Q suspended by HA	 */
#define	CD_SEND		0x10		/* Send requested timeout	 */
#define	CD_PARMS	0x20		/* Disk parms set and valid	 */

/*
 * CD-ROM specific group 6 commands
 */
#define	SV_AUDIOSEARCH	0xC0		/* Audio track search	*/
#define	SV_PLAYAUDIO	0xC1		/* Play audio		*/
#define	SV_STILL	0xC2		/* still		*/
#define	SV_TRAYOPEN	0xC4		/* Tray open		*/
#define	SV_TRAYCLOSE	0xC5		/* Tray close		*/
#define	SV_RDSTATUS	0xC6		/* Read subcode Q data &*/
					/*  playing status	*/

/*
 * Read Capacity data
 */
struct capacity {
	unsigned long    cd_addr;	   /* Logical block address	*/
	unsigned long    cd_len;	   /* Block length	 	*/
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
	struct buf       *j_bp;		   /* Pointer to buffer header	 */
	struct cdrom     *j_cdp;	   /* Device to be accessed	 */
	unsigned	  j_errcnt;	   /* Error count (for recovery) */ 
	daddr_t		  j_addr;	   /* Physical block address	 */
	union sc {			   /* SCSI command block	 */
		struct scs  ss;		   /*	group 0 (6 bytes)	*/
		struct scm  sm;		   /*	group 1 (10 bytes)	*/
		struct scv  sv;		   /*	group 6 (10 bytes)	*/
	} j_cmd;
};

/*
 * cdrom information structure
 */
struct cdrom {
	struct job       *cd_first;	    /* Head of job queue	 */
	struct job       *cd_last;	    /* Tail of job queue	 */
	struct job       *cd_next;	    /* Next job to send to HA	 */
	struct job       *cd_batch;	    /* Elevator batch pointer	 */
	struct scsi_ad	  cd_addr;	    /* SCSI address		 */
	unsigned long	  cd_sendid;	    /* Timeout id for send	 */
	unsigned  	  cd_state;	    /* Operational state	 */ 
	unsigned  	  cd_count;	    /* Number of jobs on Q	 */ 
	unsigned 	  cd_npend;	    /* Number of jobs sent 	 */ 
	unsigned	  cd_fltcnt;	    /* Retry cnt for fault jobs	 */
	struct job       *cd_fltjob;	    /* Job associated with fault */
	struct sb        *cd_fltreq;	    /* SB for request sense	 */
	struct sb        *cd_fltres;	    /* SB for resume job	 */
	struct scs	  cd_fltcmd;	    /* Request Sense command	 */
	struct sense	 *cd_sense;	    /* Request Sense data	 */
	struct capacity	 *cd_capacity;	    /* Read Capacity data	 */
	struct dev_spec  *cd_spec;
	uint_t		  cd_blkshft;	    /* Block<->Log Sect shift cnt*/ 
	bcb_t		 *cd_bcbp;	    /* Breakup control block     */
	char		  cd_iotype;	    /* io device capability	 */
	lock_t		 *cd_lock;
	pl_t		 cd_pl;
	sv_t		 *cd_sv;
};

extern struct dev_spec *sc01_dev_spec[];/* pointers to helper structs	*/
extern struct dev_cfg SC01_dev_cfg[];	/* configurable devices struct	*/
extern int SC01_dev_cfg_size;		/* number of dev_cfg entries	*/

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_SC01_SC01_H */
