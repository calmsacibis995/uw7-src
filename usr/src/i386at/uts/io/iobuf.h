#ifndef _IO_IOBUF_H	/* wrapper symbol for kernel use */
#define _IO_IOBUF_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/iobuf.h	1.8.1.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Each IDFC controller has an iobuf, which contains private state data
 * and 2 list heads: the b_forw/b_back list, which is doubly linked
 * and has all the buffers currently associated with that major
 * device; and the d_actf/d_actl list, which is private to the
 * controller but in fact is always used for the head and tail
 * of the I/O queue for the device.
 * Various routines in bio.c look at b_forw/b_back
 * (notice they are the same as in the buf structure)
 * but the rest is private to each device controller.
 */
#undef b_forw
#undef b_back
typedef struct iobuf
{
	int	b_flags;		/* see buf.h */
	struct	buf *b_forw;		/* first buffer for this dev */
	struct	buf *b_back;		/* last buffer for this dev */
	struct	buf *b_actf;		/* head of I/O queue (b_forw)*/
	struct 	buf *b_actl;		/* tail of I/O queue (b_back)*/
	o_dev_t	b_dev;			/* major+minor device name */
	char	b_active;		/* busy flag */
	char	b_errcnt;		/* error count (for recovery) */
	int	jrqsleep;		/* process sleep counter on jrq full */
	struct eblock	*io_erec;	/* error record */
	int	io_nreg;		/* number of registers to log on errors */
	paddr_t	io_addr;		/* local bus address */
	physadr	io_mba;			/* mba address */
	struct	iostat	*io_stp;	/* unit I/O statistics */
	clock_t	io_start;
	int	sgreq;			/* SYSGEN required flag */
	int	qcnt;			/* outstanding job request counter */
	int	io_s1;			/* space for drivers to leave things */
	int	io_s2;			/* space for drivers to leave things */
	dev_t	b_edev;			/* expanded device number */
} iobuf_t;

#define tabinit(dv,stat) {0,0,0,0,0,o_makedev(dv,0),0,00,0,0,0,0,0,stat,0,0,0,0,0}
#define NDEVREG	(sizeof(struct device)/sizeof(int))

#define	B_ONCE	01	/* flag for once only driver operations */
#define	B_TIME	04	/* for timeout use */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_IOBUF_H */
