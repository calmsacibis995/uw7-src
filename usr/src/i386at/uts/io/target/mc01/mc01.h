#ident	"@(#)kern-pdi:io/target/mc01/mc01.h	1.3.1.3"
#ident	"$Header$"

#ifndef _IO_TARGET_MC01_MC01_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_MC01_MC01_H	/* subject to change without notice */

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/target/scsi.h>	/* REQUIRED */
#include <io/target/sdi/sdi.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/scsi.h>		/* REQUIRED */
#include <sys/sdi.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

struct mc_exchange {
	unsigned	ex_source;
	unsigned	ex_dest;
};

struct mc_ex_medium {
	unsigned short	transport;
	unsigned short	source;
	unsigned short	first_dest;
	unsigned short	second_dest;
};

struct mc_mv_medium {
	unsigned short	transport;
	unsigned short	source;
	unsigned short	dest;
};

struct mc_position {
	unsigned short	transport;
	unsigned short	dest;
};

struct mc_rd_status {
	unsigned char	res:3;
	unsigned char	voltag:1;
	unsigned char	type:4;
	unsigned short	start_elem;
	unsigned short	num_elem;
	unsigned int	len:24;
};

struct mc_status_hdr {
	unsigned short	first_elem;
	unsigned short	num_elem;
	unsigned int	res:8;
	unsigned int	byte_count:24;
};

struct mc_esd {
	unsigned int	esd_first_elem:16;
	unsigned int	esd_num_elem:16;
	unsigned int	esd_res1:8;
	unsigned int	esd_byte_count:24;
};

struct mc_esp {
	unsigned char	esp_type;
	unsigned char	esp_res1;
	unsigned short	esp_edl;
	unsigned int	esp_res2:8;
	unsigned int	esp_byte_count:24;
};

struct sed {
	unsigned char	sed_res1;
	unsigned char	sed_address;
	unsigned char	sed_full:1;
	unsigned char	sed_res2:7;
};

#define	MC_BASE			('[' << 8)
#define	MC_EXCHANGE		(MC_BASE | 0x01)	/** Exchange Medium		**/
#define	MC_INIT_STATUS		(MC_BASE | 0x02)	/** Initialize Element Status	**/
#define	MC_MOVE_MEDIUM		(MC_BASE | 0x03)	/** Move medium			**/
#define MC_POSITION		(MC_BASE | 0x04)	/** Position To Element		**/
#define MC_RD_STATUS		(MC_BASE | 0x05)	/** Read Element Status		**/
#define	MC_ELEMENT_COUNT	(MC_BASE | 0x06)	/* Number of Elements	**/
#define	MC_STATUS		(MC_BASE | 0x07)	/* Number of Elements	**/
#define MC_MAP			(MC_BASE | 0x08)	/* Status Map		**/
#define	MC_PREVMR		(MC_BASE | 0x09)	/* Prevent Media Removal */
#define MC_ALLOWMR		(MC_BASE | 0x10)	/* Allow Media Removal */
#define	MC_LAST_LOADED		(MC_BASE | 0x11)

#define UNIT(x)		(geteminor(x) & 0xFF)
#define MC_MINORS_PER	1

#define MC_JTIME	10000
#define MC_LATER	20000

#define MC_MAXRETRY  2

/* Values of mc_state */
#define	MC_INIT		0x01
#define	MC_WOPEN	0x02
#define	MC_SUSP		0x04
#define	MC_SEND		0x08

/*
 * Job structure
 */
struct mc_job {
	struct mc_job	*j_next;	/* Next job on queue		 */
	struct mc_job	*j_prev;	/* Previous job on queue	 */
	struct mc_job	*j_priv;	/* private pointer for dynamic  */
					 /* alloc routines DON'T USE IT  */
	struct mc_job	*j_cont;	/* Next job block of request  */
	struct sb	*j_sb;		/* SCSI block for this job	 */
	struct buf	*j_bp;		/* Pointer to buffer header	 */
	struct changer	*j_cdp;		/* Device to be accessed	 */
	unsigned	j_errcnt;	/* Error count (for recovery) */ 
	daddr_t		j_addr;		/* Physical block address	 */
	union sc {			/* SCSI command block	 */
		struct scs  ss;		/*	group 0 (6 bytes)	*/
		struct scm  sm;		/*	group 1 (10 bytes)	*/
		struct scv  sv;		/*	group 6 (10 bytes)	*/
		struct sce  se;		/*	group 5 (12 bytes)	*/
	} j_cmd;
};

/*
 * changer information structure
 */
struct changer {
	struct mc_job   *mc_last;	    /* Tail of job queue	 */
	struct mc_job   *mc_next;	    /* Next job to send to HA	 */
	struct scsi_ad	mc_addr;	    /* SCSI address		 */
	unsigned long	mc_sendid;	    /* Timeout id for send	 */
	unsigned  	mc_state;	    /* Operational state	 */ 
	unsigned  	mc_count;	    /* Number of jobs on Q	 */ 
	unsigned	mc_fltcnt;	    /* Retry cnt for fault jobs	 */
	struct mc_job   *mc_fltjob;	    /* Job associated with fault */
	struct sb       *mc_fltreq;	    /* SB for request sense	 */
	struct sb       *mc_fltres;	    /* SB for resume job	 */
	struct scs	mc_fltcmd;	    /* Request Sense command	 */
	struct sense	*mc_sense;	    /* Request Sense data	 */
	struct dev_spec *mc_spec;
	char		mc_iotype;	    /* io device capability	 */
	int		mc_last_loaded;	
	int		mc_door_closed;     /* Attempted to access media */
					    /* in the drive, but the     */
					    /* was closed.               */
#ifdef _KERNEL_HEADERS
	pl_t		mc_pl;
	sv_t		*mc_sv;
	lock_t		*mc_lock;
#else
	struct lock	*mc_lock;
	pl_t		mc_pl;
	struct sv	*mc_sv;
#endif
};

extern struct dev_spec *mc01_dev_spec[];/* pointers to helper structs	*/
extern struct dev_cfg MC01_dev_cfg[];	/* configurable devices struct	*/
extern int MC01_dev_cfg_size;		/* number of dev_cfg entries	*/

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_MC01_MC01_H */
