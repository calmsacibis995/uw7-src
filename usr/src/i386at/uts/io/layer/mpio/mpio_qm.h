#ident	"@(#)kern-pdi:io/layer/mpio/mpio_qm.h	1.1"

#ifndef _IO_LAYER_MPIO_MPIO_QM_H   /* wrapper symbol for kernel use */
#define _IO_LAYER_MPIO_MPIO_QM_H   /* subject to change without notice */

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * MPIO Queue Management. The queues defined here are double linked with 
 * a hacked rotor to emulate circular indicator.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>
#include <util/param.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>
#include <sys/param.h>

#endif

/*
 * Macros
 */

#define MPIO_QM_IS_QUEUE_EMPTY(anchor_ptr) ((anchor_ptr)->size == 0)
#define MPIO_QM_HEAD(anchor_ptr)     ((anchor_ptr)->links.next)
#define MPIO_QM_TAIL(anchor_ptr)     ((anchor_ptr)->links.prev)
#define MPIO_QM_ENTRY_PTR_TO_LINK_PTR(anchor_ptr, entry_ptr)( \
                 (mpio_qm_link_t *)((caddr_t *)(entry_ptr) +   \
                  (anchor_ptr)->link_offset) )
#define MPIO_QM_SIZE(anchor_ptr) (anchor_ptr)->size
#define MPIO_QM_NEXT(anchor_ptr, entry_ptr)                  \
                ( (MPIO_QM_ENTRY_PTR_TO_LINK_PTR((anchor_ptr), (entry_ptr))->next) )
#define MPIO_QM_PREV(anchor_ptr, entry_ptr)                  \
                ( (MPIO_QM_ENTRY_PTR_TO_LINK_PTR((anchor_ptr), (entry_ptr))->prev) )

/*
 *	The mpio_qm_link_type is a structure that must be included in each
 *	entry placed on a queue.
 */
typedef struct mpio_qm_link {
	void *		next;
	void *		prev;
} mpio_qm_link_t;

/*
 *	The mpio_qm_anchor_type is a structure that is allocated by the user
 *	of the Queue Manager.  This type represents a queue.
 */
typedef struct mpio_qm_anchor {
	mpio_qm_link_t		links;
	void *			rotor;
    ulong_t			link_offset;
    ulong_t			size;
} mpio_qm_anchor_t ;

#if defined(__cplusplus)
        }
#endif

#endif /* _IO_LAYER_MPIO_MPIO_QM_H */
