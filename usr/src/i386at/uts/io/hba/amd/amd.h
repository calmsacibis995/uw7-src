#ident	"@(#)kern-pdi:io/hba/amd/amd.h	1.1.1.2"
#ident  "$Header$"

#ifndef _IO_HBA_AMD_H	/* wrapper symbol for kernel use */
#define _IO_HBA_AMD_H	/* subject to change without notice */

#if defined(__cplusplus)
extern "C" {
#endif

#define HBA_PREFIX	amd
#define DRVNAME		"amd - AMD SCSI HBA Driver"

#ifdef	_KERNEL_HEADERS

#include <io/target/scsi.h>		/* REQUIRED */
#include <util/types.h>			/* REQUIRED */
#if (PDI_VERSION > 1)
#include <io/target/sdi/sdi_hier.h>	/* REQUIRED */
#endif /* (PDI_VERSION > 1) */
#include <io/hba/hba.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/scsi.h>			/* REQUIRED */
#include <sys/types.h>			/* REQUIRED */
#if (PDI_VERSION > 1)
#include <sys/sdi_hier.h>		/* REQUIRED */
#endif /* (PDI_VERSION > 1) */
#include <sys/hba.h>			/* REQUIRED */

#endif

#define pgbnd(a)        (ptob(1) - ((ptob(1) - 1) & (int)(a)))

#define AMD_BLKSIZE	512

#define MAX_CMDSZ	12

#define MAX_DMASZ       32
#define NDMA		16

typedef struct {
        union {
            unsigned char Addr[4];
            ulong_t l;
        } Phy;
        union {
            unsigned char bytes[4];
            ulong_t l;
        } Len;
} SG_vect;

struct ScatterGather {
        uint_t SG_size;                 /* List size (in bytes)        */
        struct ScatterGather *d_next;   /* Points to next free list    */
        SG_vect  d_list[MAX_DMASZ];
};

typedef struct ScatterGather amd_dma_t;


/*
 * SCSI Request Block structure
 */
struct amd_srb {
	struct xsb     *sbp;		/* Target drv definition of SB	*/
	struct amd_srb *s_next;		/* Next block on LU queue	*/
	struct amd_srb *s_priv;		/* Private ptr for dynamic alloc*/
					/* routines DO NOT USE or MODIFY*/
	struct amd_srb *s_prev;		/* Previous block on LU queue	*/
	amd_dma_t      *s_dmap;		/* DMA scatter/gather list	*/
	paddr_t         s_addr;         /* Physical data pointer        */
};

typedef struct amd_srb amd_sblk_t;

/*
 * Logical Unit Queue structure
 */
struct amd_scsi_lu {
	struct amd_srb *q_first;	/* First job on LU queue	*/
	struct amd_srb *q_last;		/* Last job on LU queue	*/
	struct amd_srb *q_full;		/* jobs returned with QUEUE FULL */
	struct amd_scsi_lu *q_next;	/* Next queue to service	*/
	int		q_flag;		/* LU queue state flags		*/
	void	       *q_memory;	/* Memory for Portable Core	*/
	struct sense	q_sense;	/* Sense data			*/
	int		q_count;	/* Outstanding job counter	*/
	unsigned long	q_tag;		/* bits represent used tags	*/
	void	      (*q_func)();	/* Target driver event handler	*/
	long            q_param;        /* Target driver event param    */
	ushort		q_active;	/* Number of concurrent jobs	*/
	ushort		q_active_max;	/* Most concurrent scheduled jobs*/
        pl_t            q_opri;         /* Saved Priority Level         */
	unsigned int	q_addr;		/* Last read/write logical address */
	bcb_t		*q_bcbp;	/* Device breakup control block pntr */
	char		*q_sc_cmd;	/* SCSI cmd for pass-thru	*/
	
	struct amd_xsrb *q_xfirst;	/* first job in core*/
	struct amd_xsrb *q_xlast;	/* last job in core */

#ifndef PDI_SVR42
        lock_t          *q_lock;        /* Device Que Lock              */ 
#endif
};

#define	AMD_QBUSY		0x01
#define	AMD_QSUSP		0x04
#define	AMD_QSENSE		0x08		/* Sense data cache valid */
#define	AMD_QPTHRU		0x10
#define AMD_QTAGGED		0x20		/* tagged queueing supported */
#define AMD_QCORE		0x40		/* Core not ready */
#define AMD_QSERVE		0x80		/* Queue needs service	*/
#define AMD_QCHECK		0x100		/* Check Condition on queue */
#define AMD_QFULL		0x200		/* Received queue full status */

#define amd_qclass(x)	((x)->sbp->sb.sb_type)
#define	AMD_QNORM		SCB_TYPE

struct amd_xsrb {
	SCSI_REQUEST_BLOCK  x_srb;
	SRB_EXTENSION	    x_srb_extension;
	amd_sblk_t 	   *x_sp;		/* Associated SCSI Block */
	struct amd_xsrb    *x_next;
	struct amd_xsrb    *x_prev;
	struct sense	    x_sense;
};

/*
 * Host Adapter structure
 */
struct amd_scsi_ha {
	ushort_t	ha_state;
	uchar_t		ha_id;
	uchar_t		ha_flags;
	int		ha_vect;
	ulong_t		ha_base;
	struct amd_scsi_lu *ha_dev;
        int             ha_active_jobs;	/* Number Of Active Jobs	*/
	int		ha_max_jobs;	/* Max number of Active Jobs	*/

	int		ha_devconfig;	/* PCI Device Information from RM */

	/* ha_service is a list of queues that need servicing, i.e. need
	 * a job to be sent for it.  It is controlled by the srb lock for
	 * convenience.
	 */

	struct amd_scsi_lu *ha_service;


	/* PSSA structures */

	void		*ha_memory;

	struct amd_xsrb	*ha_xsrb;
	struct amd_xsrb	*ha_xsrb_freelist;
	struct amd_xsrb *ha_done;	/* List of completed jobs */

	PPORT_CONFIGURATION_INFORMATION PortConfigInfo;
	PHW_INITIALIZATION_DATA HwInitData;

	/* Portable Core functions */

	PHW_INITIALIZE               HwInitialize;
	PHW_INTERRUPT                HwInterrupt;
	PHW_DMA_STARTED              HwDmaStarted;
	PHW_FIND_ADAPTER             HwFindAdapter;
	PHW_RESET_BUS                HwResetBus;
	PHW_STARTIO 	             HwStartIo;

};

/* Valid values for ha_flags	  */

#define HA_INITIALIZE	01	/* Attempt to initialize controller */
#define HA_INIT_SUCCESS	02	/* Initialization was successful    */
#define HA_INIT_FAIL	04	/* Initialization failed	    */

#define	ScsiPortHwInitialize 	ha->HwInitialize
#define	ScsiPortHwInterrupt	ha->HwInterrupt
#define	ScsiPortHwDmaStarted	ha->HwDmaStarted
#define	ScsiPortHwFindAdapter	ha->HwFindAdapter
#define	ScsiPortHwResetBus	ha->HwResetBus
#define	ScsiPortHwStartIo	ha->HwStartIo

/*
**	Macros to help code, maintain, etc.
*/

#define SUBDEV(t,l)		(((t) << 3) | (l))
#define LU_Q(c,t,l)		amd_sc_ha[c].ha_dev[SUBDEV(t,l)]
#define HA_LU_Q(ha, t, l)	(ha)->ha_dev[SUBDEV(t,l)]

/*
 * Locking Macro Definitions
 *
 * LOCK/UNLOCK definitions are lock/unlock primitives for multi-processor
 * or spl/splx for uniprocessor.
 */

#define AMD_CORE_LOCK(p)       p = LOCK(amd_core_lock, pldisk)
#define AMD_SRB_LOCK(p)       p = LOCK(amd_srb_lock, pldisk)
#define AMD_DMALIST_LOCK(p)   p = LOCK(amd_dma_lock, pldisk)
#define AMD_SCSILU_LOCK(q)    (q)->q_opri = LOCK((q)->q_lock, pldisk)

#define AMD_CORE_UNLOCK(p)     UNLOCK(amd_core_lock, p)
#define AMD_SRB_UNLOCK(p)     UNLOCK(amd_srb_lock, p)
#define AMD_DMALIST_UNLOCK(p) UNLOCK(amd_dma_lock, p)
#define AMD_SCSILU_UNLOCK(q)  UNLOCK((q)->q_lock, (q)->q_opri)

#ifndef PDI_SVR42
/*
 * Locking Hierarchy Definition
 */
#define AMD_HIER	HBA_HIER_BASE	/* Locking hierarchy base for hba */
#endif /* !PDI_SVR42 */

#define ENABLE_SG     0
#define PCI1_SCAN     1
#define PCI2_SCAN     2
#define ENABLE_TQ     3
#define ENABLE_PARITY 4

#define TABLE_SIZE	50		/* SCO has 600 */

#define STATUS_MASK                    0x3E

#define GOOD                           0x00
#define CHECK_CONDITION                0x02
#define CONDITION_MET                  0x04
#define BUSY                           0x08
#define INTERMEDIATE                   0x10
#define INTERMEDIATE_COND_MET          0x14
#define RESERVATION_CONFLICT           0x18
#define COMMAND_TERMINATED             0x22
#define QUEUE_FULL                     0x28
#define NON_BUSY                       0
#define BUSY_SRB                       1
#define NUMBER_OF_LUNS_PER_BOARD       64
#define ADAPTER_FOUND                  (ULONG)1

/* EXTERNS AND PROTOTYPES */

extern struct amd_scsi_ha *amd_sc_ha;	
extern struct head      sm_poolhead;
extern int              amd_ctlr_id;
extern struct ver_no    amd_sdi_ver;
extern int       	amd_gtol[];
extern int       	amd_ltog[];
extern int		amd_cntls;

extern int amd_core_state;

/* amd_core_state values are defined below.  amd_core_state is synched
 * by the srb lock.  The srb lock is at a lower hierarchy (higher number
 * than the ha lock) and may therefore be locked in the Core.
 */

#define AMD_CORE_SENDING	01	/* Job is being sent		*/
#define AMD_CORE_BUSY		02	/* Job has been sent to core	*/

#if PDI_VERSION >= PDI_SVR42MP

	extern void *amd_kmem_zalloc_physreq (size_t size, int flags);
	#define KMEM_ZALLOC amd_kmem_zalloc_physreq
	#define KMEM_FREE kmem_free
	extern lock_t *amd_srb_lock;

#else

	#define KMEM_ZALLOC kmem_zalloc
	#define KMEM_FREE kmem_free

#endif


#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_AMD_H */
