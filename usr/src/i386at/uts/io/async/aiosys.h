#ifndef _IO_AIOSYS_H
#define _IO_AIOSYS_H

#ident	"@(#)kern-i386at:io/async/aiosys.h	1.7.2.1"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef  _KERNEL_HEADERS

#include <io/async/aio_hier.h>
#include <util/types.h> 			/* REQUIRED */
#include <util/param.h> 			/* REQUIRED */
#include <proc/proc.h> 				/* REQUIRED */
#include <fs/buf.h> 				/* REQUIRED */
#include <fs/file.h> 				/* REQUIRED */
#include <mem/as.h>				/* REQUIRED */
#include <mem/seg.h>				/* REQUIRED */

#else  /*_KERNEL_HEADERS */

#include <sys/aio_hier.h>
#include <sys/types.h> 				/* REQUIRED */
#include <sys/param.h> 				/* REQUIRED */
#include <sys/proc.h> 				/* REQUIRED */
#include <sys/buf.h> 				/* REQUIRED */
#include <sys/file.h> 				/* REQUIRED */
#include <vm/as.h>				/* REQUIRED */
#include <vm/seg.h>				/* REQUIRED */

#endif  /* _KERNEL_HEADERS */


/* Definitions of the various ioctls */

#define 	AIOMEMLOCK      1
#define 	AIORW		2
#define 	AIOREAD         3
#define 	AIOWRITE        4
#define 	AIOPOLL         5
#define		AIOLISTIO	6
#define		AIOGETTUNE	7	/* Retr val of all kernel tuneables */

/* Max number of elements in a poll request. */
#define		NSTATUS		32

typedef struct aiostatus {
	void		*ast_cbp;	/* User provided cbp */
	int		ast_errno;	/* Error returned by the I/O */
	ssize_t		ast_count;	/* Size of I/O completed */
	int		ast_apflag;	/* Flag indicate the cbp type */
} aiostatus_t;

typedef struct aioresult {
	int		ar_total;
	uint_t		ar_timeout;
	aiostatus_t	ar_stat[NSTATUS];
} aioresult_t; 

typedef struct asyncmlock {
	vaddr_t	am_vaddr;		/* Addr of user provided buffer */
	size_t	am_size;		/* Size of the user buffers */
	caddr_t	am_polladdr;		/* Addr of user poll status */
} asyncmlock_t;

typedef struct aiojob {
	int		aj_fd;		/* File descriptor from user */
	vaddr_t		aj_buf;		/* User buffer for I/O */
	int		aj_cnt;		/* Number of bytes for I/O */
	off64_t		aj_offset;	/* Offset into the raw slice */
	uint_t		aj_flag;	/* Flag variable for general use*/
	uint_t		aj_cmd;		/* Read or write */
	void		*aj_cbp;	/* Ptr to the libctl structure */
	int		aj_errno;	/* Errno for list I/O */
	int		aj_apflag;	/* Flag vairable for aj_cbp type */
} aiojob_t;

typedef struct aiolistio {
	uint_t		al_mode;	/* LIOWAIT versus LIONOWAIT */
	uint_t		al_nent;	/* Number of entries in the list */
	uint_t		al_flag;	/* Async notifcation ? */
	aiojob_t	al_jobs[1];	/* List of jobs */
} aiolistio_t;

#define LIOWAIT		1
#define LIONOWAIT 	2

#define AIOLIST_HDRSIZE	(sizeof(aiolistio_t) - sizeof(aiojob_t))

typedef struct aio_tune {
	uint_t 	at_listio_max;	/* Max size of any LIST I/O */
	uint_t	at_max;		/* Max outstanding I/O per process */
	uint_t	at_num_ctlblks;	/* Total no of configured ctl blocks */
} aio_tune_t;

/*
 * These flags communicate setup and result information between the user and
 * kernel.  The notify flag is set by the user in the job aj_flag, and used
 * in the kernel in aio_kflags.  The kernel sets the other flags in aio_kflags,
 * and returns them in the job's aj_flag.
 */
#define	A_ASYNCNOTIFY	0x1	/* async notification needed on completion */
#define A_LISTNOTIFY	0x4	/* async notification on LIST I/O completion */
#define A_USERFLAGS	A_ASYNCNOTIFY

#ifdef _KERNEL

typedef struct aio_queue {
	struct aio_queue	*aq_fowd;
	struct aio_queue	*aq_back;
} aio_queue_t;

/*
 * Each I/O request submitted to the underlying device by the aio driver is
 * described by an aio structure.  List I/O uses multiple aio structures,
 * strung on a list.  Every element of the list has a pointer to the head in
 * aio_lhead.  The head element also has a pointer to the tail of the list in
 * aio_ltail.
 */
typedef struct aio {
	aio_queue_t	aio_queue;	/* Thread on free or listio list. */
	sv_t		aio_sv;		/* Sync on I/O completion, protected
					 * by ap_lock in aio_proc. */
	file_t		*aio_filep;	/* fp corresponding to user fd */
	int		aio_nbytes;	/* Bytes actually read or written */
	int		aio_errno;	/* Error number values */
	uint_t		aio_kflags;	/* Flag variable for local use */
	int		aio_apflag;	/* Flag variable for cbp type */
	void 		*aio_cbp;	/* Ptr to ctlblk provided by user */
	struct aio *	aio_lhead;	/* Ptr to head of sub list */
	struct aio *	aio_ltail;	/* Ptr to tail of sub list */
	uint_t		aio_lcount;	/* No of elements in the sub list */
	struct buf	aio_buf;	/* Buffer header for disk request */
	void		(*aio_strat)();	/* Driver strategy routine */
	struct seg *	aio_saved_segp;	/* for AIOs which need aio_prep() */
} aio_t;

/* Obtain the head of the sub list */
#define LIST_HEAD(aiop)		((aiop)->aio_lhead)

/* Identify whether a given aio aiop is part of a sub list */
#define ISPART_OF_SUBLIST(aiop) ((aiop)->aio_lhead != NULL)

/* Is given aio block head of a sub list? */
#define NOTHEAD_OF_SUBLIST(aiop) ((aiop)->aio_lhead != (aiop))

/* Convert a pointer to aio_queue to a pointer to the enclosing aio structure. */
#define QPTOAIOP(aioqp) \
	((aio_t *)((char *)(aioqp) - offsetof(aio_t, aio_queue)))

/* Queue operations require applicaiton of QPTOAIOP to get the aio structure. */
#define INIT_QUEUE(headp)	((headp)->aq_fowd = (headp)->aq_back = (headp))
#define ISNULL_QUEUE(headp)	((headp) == (headp)->aq_fowd)
#define FIRST_ELEM(headp)	((headp)->aq_fowd)
#define LAST_ELEM(headp)	((headp)->aq_back)
#define NEXT_ELEM(elem)		((elem)->aq_fowd)
#define END_QUEUE(headp)	(headp)
#define ADD_QUEUE(headp, elem)  ((void)( \
	(elem)->aq_fowd = (headp)->aq_fowd, \
	(elem)->aq_back = (headp)->aq_fowd->aq_back, \
	(headp)->aq_fowd->aq_back = (elem), \
	(headp)->aq_fowd = (elem) \
))
#define DEL_QUEUE(elem) ((void)( \
	(elem)->aq_fowd->aq_back = (elem)->aq_back, \
	(elem)->aq_back->aq_fowd =  (elem)->aq_fowd, \
	(elem)->aq_fowd = NULL, \
	(elem)->aq_back = NULL \
))

/*
 * Tuneable objects.
 */
extern uint_t numaio;		/* Number of aio control blocks. */
extern uint_t aio_listio_max;	/* Max number of jobs in a listio. */
extern uint_t listio_size;	/* Max size of a list i/o request. */
extern aio_t aio_list[];	/* [numaio], the storage for control blocks. */

/* Hooks for address space operations. */
extern void	aio_intersect(struct as *as, vaddr_t base, size_t size);
extern void	aio_as_free(struct as *as);

/* Exported entry points. */
extern int	aio_init(void);
extern int	aio_open(dev_t *dev_p, int flags, int otype,
			 struct cred *cred_p);
extern int	aio_close(dev_t dev, int flag, int otyp, struct cred *cred_p);
extern int	aio_ioctl(dev_t dev, int cmd, int arg, int mode,
			  struct cred *cred_p, int *rval_p);

#endif /* _KERNEL */

#if defined(__cplusplus)
        }
#endif

#endif /* _IO_AIOSYS_H */
