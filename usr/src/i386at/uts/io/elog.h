#ifndef _IO_ELOG_H	/* wrapper symbol for kernel use */
#define _IO_ELOG_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/elog.h	1.7"
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
 * "True" major device numbers. These correspond
 * to standard positions in the configuration
 * table, but are used for error logging
 * purposes only.
 */

#define ELOG_CNTL	1
#define ELOG_SYS	2
#define ELOG_CAC	3
#define ELOG_PF		4

/*
 * IO statistics are kept for each physical unit of each
 * block device (within the driver). Primary purpose is
 * to establish a guesstimate of error rates during
 * error logging.
 */

struct iostat {
	long	io_ops;		/* number of read/writes */
	long	io_misc;	/* number of "other" operations */
	long	io_qcnt;	/* number of jobs assigned to drive */
	ushort_t io_unlog;	/* number of unlogged errors */
};

/*
 * structure for system accounting
 */
struct iotime {
	struct iostat ios;
	long	io_bcnt;	/* total blocks transferred */
	clock_t	io_resp;	/* total block response time */
	clock_t	io_act;		/* total drive active time (cumulative utilization) */
	int	io_pad;		/* round size to 2^n */
};
#define	io_cnt	ios.io_ops
#define io_qc	ios.io_qcnt

/*
 * Drive utilization times can be calculated by system software as follows:
 *
 * Average drive utilization = (io_cact/io_elapt)
 * Average drive utilization for last interval = (io_liact/io_intv)
 */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_ELOG_H */
