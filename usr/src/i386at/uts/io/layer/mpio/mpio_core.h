#ifndef _IO_MPIO_MPIO_CORE_H    /* wrapper symbol for kernel use */
#define _IO_MPIO_MPIO_CORE_H    /* subject to change without notice */

#ident	"@(#)kern-pdi:io/layer/mpio/mpio_core.h	1.1.5.3"

/*
 * This module contains the MPIO driver core defines. This module
 * content is OS independent.
 */

#if defined(__cplusplus)
extern "C" {
#endif

#include "mpio_qm.h"
#include "mpio_os.h" 

#define MPIO_SIGNATURE_SIZE MP_PORT_SIGNATURE_LEN /* number of bytes in a device sig. */
#define DEVICE_NAME_LENGTH_MAX	(64)
#define MAX_LOCALES_PER_SYSTEM	8

#define MPIO_PD_STAMP_DFLT	"            " /* Default value for stamp   */
                                               /* this is 12 spaces since   */
                                               /* pd_stamp is 12 bytes and  */
                                               /* gets initialized w spaces */
#define MPIO_PD_STAMP_DFLT_LEN	(12)

#ifdef _KERNEL

#endif

/*
 * The possible answers from a signature comparision function invocation.
 */
typedef enum {
    FSIG_NO_MATCHED, 
    FSIG_MATCHED, 
    FSIG_HALF_MATCHED
} sig_compare_result_t;

/*
 * Reason codes for trespasses. The trespass logic may shutdown
 * a path if the # of external caused trespasses is excessive.
 */
typedef enum { 
	INTERNAL_ACTIVATE_CAUSE, 
	EXTERN_ACTIVATE_CAUSE 
} path_activation_reason_t;

/*
 * Reason codes for path shutdown.
 */
typedef enum { 
	EXCESSIVE_TRESPASS, 
	MISC_CAUSE, 
	USER_DEMAND 
} path_fail_reason_t;

/*
 * The Path - The object associates with a physical I/O path.
 */
typedef struct {
    mpio_qm_link_t	        next_in_queue;	/* next path in state queue  */
    mpio_qm_link_t	        next_path;	/* next path for device      */
    path_state_t	state;
    path_fail_reason_t  failed_reason;
    sdi_device_t	*devicep;       /* the SDI device	     */
    int		        cpugroup;       /* my original cpugroup	     */
    int		        activate_count; /* detect excessive trespass */
    os_time_t	        clock_begin;	/* to measure trespass rate  */
    os_acct_data_t	acct;		/* for statistic	     */
    boolean_t		assert_error;	/* used for error insertion  */
} mpio_path_t, path_t, *path_p_t;

/*
 * The Locale Path Group (LPG) object. This object manages paths
 * of a device that are on the same locale.
 */
typedef struct {
    mpio_qm_anchor_t	    active_list;
    mpio_qm_anchor_t	    inactive_list;
    mpio_qm_anchor_t	    failed_list;
    int		    cpugroup;
} lpg_t, * lpg_p_t;

/*
 * LPG vector. This object manages locale switching system. A
 * locale may be switched to another locale due to a user demand
 * or path failures.
 */
typedef struct {
    lpg_p_t	     working_lpg;
    lpg_p_t	     home_lpg;
} lpg_vector_t, *    lpg_vector_p_t;

/*
 * Possible state of a MPIO device.
 */
typedef enum mpio_vstate {
    FVS_NORMAL,		     /* while operating	  	 */
    FVS_REPAIRING,	     /* while repairing paths    */
    FVS_FAILED		     /* after failure	 	 */
} mpio_vstate_t;

/*
 * Vdev Object. This is the MPIO device. This device is capable of
 * selecting optimal physical path to issue I/O as well as recover
 * from a path failure.
 */
typedef struct {
    mpio_qm_link_t		  links;	    /* Vdev chain	       */
    mpio_vstate_t	  state;
    sdi_signature_t	  *signature;	    /* device signature	       */
    int			  open_count;
    int			  number_of_paths;
    mpio_qm_anchor_t	          paths;	    /* all this vdev paths     */
    VSPIN_LOCK_T          lock;	            /* for this Vdev	       */
    sleep_t		  *vdev_sleeplock;  /* Used to sync path moves */
    lpg_vector_t          lpg_vector[MAX_LOCALES_PER_SYSTEM];
    int		          max_locales;	    /* # of locale present     */
    sdi_device_t          *devicep;  	    /* the exported device     */
    os_acct_data_t	  acct;	       	    /* for accounting/stat     */

} vdev_t, *vdev_p_t;

/*
 * The locking primitives per Vdev.
 */
#define    MPIO_VDEV_LOCK(vdevp)	VSPIN_LOCK((&vdevp->lock))
#define    MPIO_VDEV_UNLOCK(vdevp)	VSPIN_UNLOCK((&vdevp->lock))

#define	MPIO_VDEV_SLEEP_LOCK(vdevp) SLEEP_LOCK((vdevp)->vdev_sleeplock, pldisk)
#define	MPIO_VDEV_SLEEP_UNLOCK(vdevp) SLEEP_UNLOCK((vdevp)->vdev_sleeplock)

/*
 * The fully connected system-wide routing policy define.
 * When a muti-ported device path failure occurs, the MPIO can choose
 * to either deflect the traffic to another node (far) or perform a
 * trespass to deflect traffic to the alternate port locally. This
 * selection may impact system performance a great deal and therefore
 * should be documented in very detail elsewhere ie sysadm manual.
 */
typedef enum {
    FC_ROUTING_INACTIVE_FIRST, 
    FC_ROUTING_FAR_FIRST
} mpio_routing_policy_t;

/*
 *  The dip - driver info object that knows everything about the MPIO
 */
typedef struct {
    int		    	    num_of_vdevs;	/* this driver is managing   */
    int		    	    max_locales;	/* can be add or delete      */
    boolean_t	     	    ready;		/* sync the init process     */
    mpio_routing_policy_t   fc_policy;		/* far vs inactive policy    */
    int		    	    state_change_count; /* state change count	     */
    VSPIN_LOCK_T	    lock;		/* regulate access to driver */
    int		    	    ioreq_in;	        /* debug accounting	     */
    int		    	    ioreq_out;	        /* debug accounting	     */
    sdi_layer_t *           my_layer;
    sdi_driver_desc_t *     desc;

} mpio_driver_info_t, * mpio_driver_info_p_t;
/*
 *  num_of_devs:    The number of physical devices currently under the 
 *		    jurisdiction of this driver.
 *  
 *  max_locales:    The number of locales this system has. Currently we
 *		    cannot handle dynamic hot remove and insertion of
 *		    locale.
 *  
 *  ready:	    Set indicates that the driver initialization succeded
 *		    Clear indicates a failure, driver is not ready.
 *  
 *  lock:	    The lock that protects access to the MPIO.
 *  
 *  fc_policy:	    System-wide fully-connected policy. The policy can be
 *		    far first or inactive first.
 *  
 *  state_change_count: Count of state changes. The system can periodically
 *		    queries this count to determine if its user representation
 *		    of the system (GUI) should be updated or not.
 *  
 *  lock:	    Protect accesses to this object.
 *		  
 */


/*
 * Misc. macros
 */
#define MISC_ROUND_UP(value_to_round, round_boundary)(\
       ( (int)((value_to_round) + (round_boundary) - 1)) \
			   & ~(int)(round_boundary - 1)  )
/*
 *  Definition of error state that is used thoughout this driver
 */
typedef enum status { OK, FAILURE } status_t;

/*
 * Initializing driver information block (dip)
 */
#define    MPIO_INIT_DRIVER_INFO(dip) 				\
    dip->ready = B_FALSE;					\
    dip->fc_policy = (mpio_inactive_first == 1) ?		\
	    FC_ROUTING_INACTIVE_FIRST : FC_ROUTING_FAR_FIRST;   \
    dip->num_of_vdevs = 0;					\
    dip->max_locales = MPIO_GET_LOCALE_COUNT();			\
    VSPIN_LOCK_ALLOC(0, &dip->lock, &mpio_info_lkinfo);	        \
    dip->state_change_count = 0;				\
    /* debug stuff */					        \
    dip->ioreq_in = dip->ioreq_out = 0;

/*
 * De-initialize driver information block.
 */
#define    MPIO_DEINIT_DRIVER_INFO(dip) \
    VSPIN_LOCK_DEALLOC(&dip->lock);

/*
 * Driver lock for admin activity that touchs the gut of the driver.
 */
#define    MPIO_DRIVER_LOCK()	    VSPIN_LOCK(&dip->lock);
#define MPIO_DRIVER_UNLOCK()	    VSPIN_UNLOCK(&dip->lock);


#ifdef MPIO_DEBUG
/*
 * Counting the number of requests entering and leaving the driver.
 */

#define    MPIO_DRIVER_INFO_CHECK_IN()  \
    VSPIN_LOCK(&dip->lock);		\
    dip->ioreq_in++;			\
    VSPIN_UNLOCK(&dip->lock);

#define    MPIO_DRIVER_INFO_CHECK_OUT() \
    VSPIN_LOCK(&dip->lock);		\
    dip->ioreq_out++;			\
    VSPIN_UNLOCK(&dip->lock);

#else

#define    MPIO_DRIVER_INFO_CHECK_IN()
#define    MPIO_DRIVER_INFO_CHECK_OUT()

#endif    /* MPIO_DEBUG */

/*
 * external defines
 */
extern mpio_driver_info_t *     dip;
extern mpio_qm_anchor_t		mpio_vdev_queue;

/*
 * The number of trespasses occur before we recalculate
 * the trespass rate.
 */
#define    MPIO_ACTIVATE_COUNT_LIMIT	    10

/*
 * The maximum rate of trespass per minute before we shutdown
 * the failing path to prevent ping-pong trespass effect.
 */
#define    MPIO_ACTIVATE_RATE_LIMIT	    5    /* per minute */

#if defined(__cplusplus)
    }
#endif

#endif /* _IO_MPIO_MPIO_CORE_H */
