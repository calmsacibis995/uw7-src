#ident	"@(#)kern-pdi:io/target/sdi/sdi_qm.h	1.1"

#ifndef _IO_TARGET_SDI_SDI_QM_H   /* wrapper symbol for kernel use */
#define _IO_TARGET_SDI_SDI_QM_H   /* subject to change without notice */

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Queue Management. The queues defined here are double linked with 
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

#define QM_IS_QUEUE_EMPTY(anchor_ptr) ((anchor_ptr)->size == 0)
#define QM_HEAD(anchor_ptr)     ((anchor_ptr)->links.next)
#define QM_TAIL(anchor_ptr)     ((anchor_ptr)->links.prev)
#define QM_ENTRY_PTR_TO_LINK_PTR(anchor_ptr, entry_ptr)( \
                 (qm_link_t *)((caddr_t *)(entry_ptr) +   \
                  (anchor_ptr)->link_offset) )
#define QM_SIZE(anchor_ptr) (anchor_ptr)->size
#define QM_NEXT(anchor_ptr, entry_ptr)                  \
                ( (QM_ENTRY_PTR_TO_LINK_PTR((anchor_ptr), (entry_ptr))->next) )
#define QM_PREV(anchor_ptr, entry_ptr)                  \
                ( (QM_ENTRY_PTR_TO_LINK_PTR((anchor_ptr), (entry_ptr))->prev) )

/*
 *	The qm_link_type is a structure that must be included in each
 *	entry placed on a queue.
 */
typedef struct qm_link {
	void *		next;
	void *		prev;
} qm_link_t;

/*
 *	The qm_anchor_type is a structure that is allocated by the user
 *	of the Queue Manager.  This type represents a queue.
 */
typedef struct qm_anchor {
	qm_link_t		links;
	void *			rotor;
    ulong_t			link_offset;
    ulong_t			size;
} qm_anchor_t ;

#if defined(__cplusplus)
        }
#endif

#endif /* _IO_TARGET_SDI_SDI_QM_H */
