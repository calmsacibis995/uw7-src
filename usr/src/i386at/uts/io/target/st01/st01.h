#ifndef _IO_TARGET_ST01_ST01_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_ST01_ST01_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/st01/st01.h	1.23.5.3"
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

#ifdef _KERNEL_HEADERS

#include <fs/buf.h>			/* REQUIRED */
#include <io/target/scsi.h>		/* REQUIRED */
#include <io/target/sdi/sdi.h>		/* REQUIRED */
#include <io/target/sdi/sdi_hier.h>	/* REQUIRED */
#include <util/types.h>			/* REQUIRED */

/* Work around for MET statistics deficiences: begin */
#include <io/metdisk.h>			/* REQUIRED */
/* Work around for MET statistics deficiences: end */

#elif defined(_KERNEL)

#include <sys/buf.h>		/* REQUIRED */
#include <sys/scsi.h>		/* REQUIRED */
#include <sys/sdi.h>		/* REQUIRED */
#include <sys/sdi_hier.h>	/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

/* Work around for MET statistics deficiences: begin */
#include <sys/metdisk.h>	/* REQUIRED */
/* Work around for MET statistics deficiences: end */

#endif /* _KERNEL_HEADERS */

/*
 * the minor device number is interpreted as follows:
 * 
 *     bits:
 *	  11  10  9   8   7   6   5   4   3   2   1   0
 * 	+-------------------------------+---+---+---+---+
 * 	|             unit              | 0 | u | r | n |
 * 	+-------------------------------+---+---+---+---+
 *
 *     codes:
 *	unit  - sequential device no. (0 - 255)
 *	r     - retension on open (0 or 1, 1 = True)
 *	n     - no rewind at close (0 or 1, 1 = True)
 *	u     - issue unload command at close (0 or 1, 1 = True)
 */
#define TP_MAX_TAPES	512		/* Maximum number of tape devices */
#define TP_MINORS_PER	16		/* Number of minors per unit */
#define TP_MAX_MINOR    (minor_t)TP_MAX_TAPES*TP_MINORS_PER -1 /* Max minor# */

/*
 * Given a valid minor for a tape
 * returns an index into st01_tp for the tape pointer
 */
#define TPINDEX(x)      (x / TP_MINORS_PER)

/*
 * Given a valid minor for a tape
 * returns a tape pointer.  May return NULL if dev_t is not valid.
 */
#define TPPTR(x)        (st01_tp[TPINDEX(x)])

/*
 * Given a index into st01_tp[]
 * returns the appropriate minor number
 */
#define TPINDEX2MINOR(x) (x * TP_MINORS_PER)

#define NOREWIND(x)		(geteminor(x) & 0x01)
#define RETENSION_ON_OPEN(x)	(geteminor(x) & 0x02)
#define	DOUNLOAD(x)		(geteminor(x) & 0x04)

#define	IMMEDIATE	0x01		/* Status returned immediately	*/
#define	VARIABLE	0x00		/* Variable block mode		*/
#define	FIXED		0x01		/* Fixed block mode		*/
#define	LOAD		0x01		/* Medium to be loaded		*/
#define	UNLOAD		0x00		/* Medium to be unloaded	*/
#define	RETENSION	0x03		/* Retension (and load) tape	*/
#define BLOCKS		0x00		/* Space blocks			*/
#define FILEMARKS	0x01		/* Space file marks		*/
#define SEQFLMRKS	0x02		/* Space sequential file marks	*/
#define EORD		0x03		/* Space to end-of-recorded-data */
#define SHORT		0x00		/* Short erase (not supported)	*/
#define	LONG		0x01		/* Long erase			*/
#define BUFFERED	0x01		/* Buffered mode		*/
#define UNBUFFERED	0x00		/* Unbuffered mode		*/
#define DEFAULT		0x00		/* Use default value		*/

#define ONE_SEC		1000		/* # of msec in one second	*/
#define ONE_MIN		60000		/* # of msec in one minute	*/
#define JTIME		30 * ONE_SEC	/* 30 sec for an I/O job	*/
#define MAX_RETRY	2		/* Max number of retries	*/
#define	ST01_MAXSIZE	60*1024		/* Max ST01 job size		*/

#ifdef _KERNEL

/*
 * Job structure
 */
struct job {
	struct job     *j_next;	   	/* Points to next job on list	*/
	int	       (*j_func)();	/* Function to process job	*/
	struct job     *j_priv;	   	/* private pointer for dynamic  */
					/* alloc routines DON'T USE IT  */
	struct sb      *j_sb;		/* SCSI block for this job	*/
	buf_t	       *j_bp;		/* Pointer to buffer header	*/
	struct tape    *j_tp;		/* Device to be accessed	*/
	time_t 		j_time;		/* Time limit for job		*/
	union sc {
		struct scs  ss;		/* Group 0,6 command - 6 bytes	*/
		struct scm  sm;		/* Group 1,7 command - 10 bytes */
	} j_cmd;
};

/*
 * Device information structure
 */

struct tape {
	int		t_marked;	/* Boolean for disclaiming tape */
	struct scsi_ad	t_addr;		/* SCSI address			*/
	unsigned  	t_state;	/* Operational state flags	*/ 
	unsigned	t_lastop;	/* Last command completed	*/ 
	unsigned  	t_bsize;	/* Block size			*/ 
	unsigned	t_fltcnt;	/* Retry count (for recovery)	*/ 
	struct job     *t_fltjob;	/* Job associated with fault	*/
	struct sb      *t_fltreq;	/* SCSI block for Request Sense */
	struct sb      *t_fltres;	/* SCSI block for resume job	*/
	struct sb      *t_fltrst;	/* SCSI block for reset job	*/
	struct scs	t_fltcmd;	/* Request Sense command	*/
	struct sense   *t_sense;	/* Request Sense data		*/
	struct mode    *t_mode;		/* Mode Sense/Select data	*/
	struct blklen  *t_blklen;	/* Tape block length limit	*/
	struct ident   *t_ident;	/* Inquiry ident data		*/
	struct dev_spec *t_spec;
	char 		t_iotype;	/* Drive capability (DMA/PIO)	*/
	bcb_t		*t_bcbp;	/* Breakup control block	*/
	struct job     *t_head;		/* Head of job queue		*/
	struct job     *t_tail;		/* Tail of job queue		*/
	bcb_t		*t_abcbp;	/* Page-aligned internal bcb	*//*XX*/
	void		*t_abuff;	/* Aligned buffer (for t_abcbp) *//*XX*/
	struct mode_dcc_page *t_dcc;	/* Data Compression Mode page	*//*XX*/
	uio_t		*t_uiop;	/* UIO for aligned buffer	*//*XX*/
	iovec_t		t_iovec;	/* iovec for t_uiop		*//*XX*/
	struct sb	*t_fltread;	/* SCSI block for Read recovery *//*XX*/

/* Work around for MET statistics deficiences: begin */
	met_disk_stats_t *t_stat;	/* Tape stats */
/* Work around for MET statistics deficiences: end */
};

/* Values of t_state */
#define	T_OPENED	0x01		/* Tape is open			*/
#define	T_WRITTEN	0x02		/* Tape has been written 	*/
#define	T_SUSPEND	0x04		/* Tape LU Q suspended by HA	*/
#define	T_FILEMARK	0x08		/* File mark encountered	*/
#define	T_TAPEEND	0x10		/* End of media 		*/
#define	T_PARMS		0x20		/* Tape parms set and valid     */
#define	T_READ		0x40		/* Tape has been read	 	*/
#define	T_RESERVED	0x80		/* Tape has been reserved 	*/
#define	T_OPENING	0x100		/* Tape is being opened		*/
#define T_SEND		0x200		/* Timeout has been issued	*/
#define T_RESUMEQ	0x400		/* Queue has been resumed	*/
#define	T_ABORTED	0x800		/* User aborted execution       */
#define T_COMPRESSION	0x1000		/* Compression capable device	*/
#define T_COMPRESSED 	0x2000		/* Compression has been altered	*/
#define	T_SHORT_READ	0x4000		/* Short read occurred		*/

extern struct dev_spec *st01_dev_spec[];/* pointers to helper structs	*/
extern struct dev_cfg ST01_dev_cfg[];	/* configurable devices struct	*/
extern int ST01_dev_cfg_size;		/* number of dev_cfg entries	*/

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_ST01_ST01_H */
