#ifndef _IO_MPIO_MPIO_PROTO_H    /* wrapper symbol for kernel use */
#define _IO_MPIO_MPIO_PROTO_H    /* subject to change without notice */

#ident	"@(#)kern-pdi:io/layer/mpio/mpio_proto.h	1.1.8.2"

/*
 *    Module contains MPIO driver function prototypes.
 */

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include "mpio_core.h"
#include <io/conf.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <fs/buf.h>
#include <io/target/sdi/sdi_comm.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi_layer.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#endif /* _KERNEL_HEADERS */

/*
 * From mpio_port.c
 */
void    mpio_acct_create(os_acct_data_p_t);
void    mpio_acct_delete(os_acct_data_p_t );
void    mpio_acct_start(os_acct_data_p_t, os_io_op_record_p_t);
void    mpio_acct_stop(os_acct_data_p_t, os_io_op_record_p_t);
void    mpio_acct_update(os_acct_data_p_t, os_io_op_record_p_t);

/*
 * Functions from mpio_core.c
 */
void	    os_stub_init();
void	    mpio_start();
void        mpio_stop();
int         mpio_configure_path(void *, rm_key_t);
int         mpio_deconfigure_path(void *);
void        mpio_issue_blkio(vdev_p_t, os_io_op_record_p_t);
void        mpio_complete_blkio(os_io_op_record_p_t);
void        mpio_signal_start_blkio_failure(os_io_op_record_p_t bp);
status_t    mpio_deconfigure_devices();
boolean_t   mpio_is_a_recoverable_error(int);
vdev_p_t    mpio_dev_handle_map_table_get(int);
rm_key_t    mpio_report_new_device_to_kernel(vdev_p_t);

int	    mpio_ioctl_register_path ( int, int *);
int	    mpio_ioctl_deregister_path ( int, int *);
int	    mpio_ioctl_tuneable ( int, int *);

int	    mpio_ioctl_fail_path8(vdev_p_t, void *, int *);
int	    mpio_ioctl_repair_path8(vdev_p_t, void *, int *);
int	    mpio_ioctl_switch_group8(vdev_p_t, void *, int *);
int	    mpio_ioctl_get_paths8(vdev_p_t, void *, int *);
int	    mpio_ioctl_get_path_info8(vdev_p_t, void *, int *);
int	    mpio_ioctl_insert_error8(vdev_p_t, void *, int *);

/*
 * Vdev Class Functions.
 */

vdev_p_t    mpio_vdev_create();
boolean_t   mpio_vdev_delete(vdev_p_t);
boolean_t   mpio_vdev_put(sdi_device_t *, vdev_p_t);
boolean_t   mpio_vdev_remove_path(path_p_t , vdev_p_t );
path_p_t    mpio_vdev_get_path_by_rmkey(rm_key_t, vdev_p_t);
boolean_t   mpio_vdev_is_signature_consistent(sdi_signature_t *, vdev_p_t);
void	    mpio_vdev_recover_blkio_failure(path_p_t,
					    vdev_p_t,
					    os_io_op_record_p_t bp);
void	    mpio_vdev_switch_lpg_vector(int, int, vdev_p_t);
int	    mpio_vdev_recover_from_path_failure(path_p_t*,vdev_p_t);

/*
 * Locale Path Group (LPG) Class Functions
 */

lpg_p_t     mpio_lpg_create(int);
path_p_t    mpio_lpg_get_active_path(lpg_p_t);
status_t    mpio_lpg_activate_path(path_p_t, 
				     lpg_p_t, 
				     vdev_p_t,
				     path_activation_reason_t);
boolean_t   mpio_lpg_delete(lpg_p_t);
boolean_t   mpio_lpg_put(path_p_t, lpg_p_t);
boolean_t   mpio_lpg_remove_path(path_p_t, lpg_p_t );
boolean_t   mpio_lpg_delete_queue(mpio_qm_anchor_t *);
void	    mpio_lpg_remove(path_p_t);
void        mpio_lpg_fail_path(path_p_t, lpg_p_t, path_fail_reason_t);
void	    mpio_lpg_switch_state_after_trespass(lpg_p_t);
sig_compare_result_t
	    mpio_lpg_repair_path(path_p_t, lpg_p_t, sdi_signature_t *);

/*
 * Path Class (Path) Functions
 */
path_p_t    mpio_path_create(sdi_device_t *);
void	    mpio_path_delete(path_p_t);
sig_compare_result_t
	    mpio_path_verify(path_p_t, sdi_signature_t *);

path_p_t    mpio_get_best_path( buf_t *, vdev_p_t );

/*
 * Device Signature Class methods
 */
sig_compare_result_t mpio_signature_compare(const sdi_signature_t *,
				const sdi_signature_t *);
status_t mpio_signature_and_state_get(sdi_device_t*, const sdi_signature_t **);
sdi_signature_t *mpio_signature_copy(const sdi_signature_t *);

/*
 * Queue primitives in mpio_qm.c
 */
void	     mpio_qm_initialize (mpio_qm_anchor_t *, ulong_t);
void	     mpio_qm_move (mpio_qm_anchor_t *, mpio_qm_anchor_t *);
void *	     mpio_qm_remove_head (mpio_qm_anchor_t *);
void *	     mpio_qm_remove_tail (mpio_qm_anchor_t *);
void	     mpio_qm_add_head (mpio_qm_anchor_t *, void *);
void         mpio_qm_add_tail (mpio_qm_anchor_t *, void *);
void         mpio_qm_remove (mpio_qm_anchor_t *, void *);
mpio_qm_link_t *  mpio_qm_next_and_rotate( mpio_qm_anchor_t *);
boolean_t    mpio_qm_is_member (mpio_qm_anchor_t *, void *);

#if defined(__cplusplus)
    }

#endif

#endif /* _IO_MPIO_MPIO_PROTO_H */
