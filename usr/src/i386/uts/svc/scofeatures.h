#ifndef _SVC_SCOFEATURES_H      /* wrapper symbol for kernel use */
#define _SVC_SCOFEATURES_H      /* subject to change without notice */

#ident	"@(#)kern-i386:svc/scofeatures.h	1.1"
#ident	"$Header$"

/* Enhanced Application Compatibility Support */

/* Used in system.c */
#define FEATURE_SIGCHLD		0	/* SIGCHLD feature vector index */
#define FEATURE_SIGCHLD_VAL	1	/* SIGCHLD feature vector value */

/* Used in ttyname and eventually catopen */
#define FEATURE_MMAP		1	/* Memory mapped I/O */
#define FEATURE_MMAP_VAL	1

/* Only necessary if _NICE_FAILURE is defined */
#define FEATURE_VNODE		2	/* struct namefd stuff */
#define FEATURE_VNODE_VAL	1

/* used in writev (to fall into kernels or use user level one ) */
#define FEATURE_WRITEV		3	/* Kernel writev support */
#define FEATURE_WRITEV_VAL	1

/* used in getlogin, tcgetsid */
#define FEATURE_SID		4	/* getsid support enabled */
#define FEATURE_SID_VAL		0

/* Necessary only for _NICE_FAILURE support */
#define FEATURE_CANPUT		5	/* support of I_CANPUT to ioctl */
#define FEATURE_CANPUT_VAL	1

/* At moment, only necessary for _NICE_FAILURE support. Note, extended signal
 * stuff NOT yet implemented, this may be necessary then */
#define FEATURE_PROCSET		6	/* support of procset, setprocset, etc */
#define FEATURE_PROCSET_VAL	1


/* Used in tcgetattr, tcgetpgrp, etc */
#define FEATURE_TCGETS		7	/* Support for TCGETS ioctl parameter */
					/* and for TIOCGSID as well */
#define FEATURE_TCGETS_VAL	0

/* Used for cf(get|set)(i|o)speed functions */
#define FEATURE_TERMIOS		8	/* CIBAUD Support, storing in and out */
#define FEATURE_TERMIO_VAL	2	/* speed in flags field of termios_p */

/* Used for fsync() */
#define	FEATURE_FSYNC		9	/* kernel supports fsync sys call */
#define	FEATURE_FSYNC_VAL	1

/* Used for gettimeofdate() */
#define	FEATURE_GTIMEOFDAY	10	/* kernel has a gettimeofday sys call */
#define	FEATURE_GTIMEOFDAY_VAL	1

/* Used for 32 bit inode APIs */
#define	FEATURE_INODE32		11	/* kernel has sys calls for 32 bit inode */
#define	FEATURE_INODE32_VAL	1

#define FEATURE_LIMIT		12	/* size of features_vector */

/* End Enhanced Application Compatibility Support */

#endif /* _SVC_SCOFEATURES_H */


