#ifndef _IO_MPIO_IOCTL_H
#define _IO_MPIO_IOCTL_H

#ident	"@(#)kern-pdi:io/layer/mpio/mpio_ioctl.h	1.1.4.1"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This module contains the MPIO UnixWare wrapper defines for ioctl 
 * related stuffs.
 */
#ifdef _KERNEL_HEADERS

#include "mpio_core.h"
#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>  /* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * ioctl arguments for MPIO ioctls
 */

#define MPIO_IOCTL_VERSION_1	1		/* ioctl version number */

#define MPIO_IOCTL	  (('V' << 24) | ('F' << 16) | ('C' << 8))

#define MPIO_IOCTL_FAIL_PATH	      (MPIO_IOCTL | 10)
#define MPIO_IOCTL_REPAIR_PATH	      (MPIO_IOCTL | 11)
#define MPIO_IOCTL_SWITCH_GROUP	      (MPIO_IOCTL | 12)
    
#define MPIO_IOCTL_GET_PATHS	      (MPIO_IOCTL | 20)
#define MPIO_IOCTL_GET_PATH_INFO      (MPIO_IOCTL | 21)

#define MPIO_IOCTL_TUNEABLE	      (MPIO_IOCTL | 99)
#define MPIO_IOCTL_INSERT_ERROR       (MPIO_IOCTL | 100)

/*
 * MPIO tunable ioctl. Use this ioctl to change fully-connected
 * routing policy.
 */
typedef struct {
    int		version;
    int		fc_policy;
}mpio_ioctl_tuneable_t, * mpio_ioctl_tuneable_p_t;

#define    FC_POLICY_INACTIVE_FIRST      (0)   /* use alternate port first    */
#define    FC_POLICY_FAR_FIRST	         (1)   /* use far port first	      */


/*
 *  MPIO structure used for failing and repairing paths.
 */
typedef struct {
    int		version;
    rm_key_t	path_rmkey;
} mpio_ioctl_path_t, * mpio_ioctl_path_p_t;
    
    
/*
 *  MPIO structure used for switching from one CPU group to another
 *  for a specified device.
 */
typedef struct {
    int	     version;
    rm_key_t	device;
    int	     source;
    int	     destination;
    
} mpio_ioctl_switch_group_t, * mpio_ioctl_switch_group_p_t;
   

/*
 *  MPIO structure used to get list of paths that the driver knows
 *  about.
 */
typedef struct {
    int		version;
    int		num_input;   /* the number of allocated slots    */
    rm_key_t	*input;      /* the array of slots	         */
    int		num_output;  /* the number of filled slots       */
    
} mpio_ioctl_get_paths_t, * mpio_ioctl_get_paths_p_t;
    
    
/*
 *  MPIO structure used to get all information about a path.
 */
typedef struct {
    int	        version;
    rm_key_t	path_rmkey;
    int	        state;
    int	        num_reads;    /* why do we care the different between */
    int	        num_writes;   /* write vs read ? we can't decode I/O  */
    int	        locale;
    
} mpio_ioctl_get_path_info_t, * mpio_ioctl_get_path_info_p_t;

    
/*
 * MPIO internal, debug assisted ioctl.
 */
typedef struct {
    int	        version;
    rm_key_t	path_rmkey;
    int         assert_error;                    /* toggle error insertion */
} mpio_ioctl_err_t, * mpio_ioctl_err_p_t;

#if defined(__cplusplus)
    }
#endif

#endif /* _IO_MPIO_IOCTL_H */
