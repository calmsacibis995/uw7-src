#ifndef _ACC_MAC_COVERT_H	/* wrapper symbol for kernel use */
#define _ACC_MAC_COVERT_H	/* subject to change without notice */

#ident	"@(#)covert.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef CC_PARTIAL
#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * This header file is for Covert Channel treatment.
 */

/*
 * Following are the covert channel events requiring auditing.
 * Note: Some of the following are no longer used by the kernel in this
 * release.  They remain defined so that auditrpt can process old
 * audit trails.
 */
#define CC_ALLOC_INODE	1	/* Allocation of (SFS) inodes */
#define CC_ALLOC_IPC	2	/* Allocation of IPC */
#define CC_CACHE_MACLVL	3	/* Cache MAC LIDs through lvldom */
#define CC_CACHE_MACSEC	4	/* Cache MAC LIDs through secadvise */
#define CC_CACHE_PAGE	5	/* Cache pages */
#define CC_RE_DB	6 	/* Resource Exhaustion data blocks */
#define CC_RE_FLOCK	7	/* Resource Exhaustion file locking */
#define CC_RE_INODE	8	/* Resource Exhaustion inodes */
#define CC_RE_LOG	9	/* Resource Exhaustion log driver */
#define CC_RE_NAMEFS	10	/* Resource Exhaustion namefs */
#define CC_RE_PIPE	11	/* Resource Exhaustion pipes */
#define CC_RE_PROC	12	/* Resource Exhaustion processes */
#define CC_RE_SAD	13	/* Resource Exhaustion sad driver */
#define CC_RE_SCSI	14	/* Resource Exhaustion SCSI */
#define CC_SPEC_DIROFF	15	/* i_diroff incore inode field */
#define CC_SPEC_DIRRM	16	/* non-empty directory removal */
#define CC_SPEC_SYNC	17	/* sync call */
#define CC_SPEC_UNLINK	18	/* unlink open file with 0 link count */
#define CC_SPEC_WAKEUP	19	/* wakeup from locks */
#define CC_RE_MSG   	20	/* Resource Exhaustion msgfp, msgmap */
#define CC_RE_SEM   	21	/* Resource Exhaustion semfup, semmap */
#define CC_RE_TIMERS	22	/* Resource Exhaustion hrtimers */
#define CC_SPEC_TIMERS	23	/* multiple BSD timer commands */
#define CC_CACHE_DNLC	24	/* Cache DNLC */

#define	CC_MAXEVENTS	25	/* maximum number of events */

/*
 * The default number of bits per event for each covert channel event.
 * If the value is calculated dynamically, we define it as 0 here.
 */
#define CCBITS_ALLOC_INODE	0	/* Allocation of (SFS) inodes */
#define CCBITS_CACHE_MACLVL	1	/* Cache MAC LIDs through lvldom */
#define CCBITS_CACHE_MACSEC	1	/* Cache MAC LIDs through secadvise */
#define CCBITS_CACHE_PAGE	1	/* Cache pages */
#define CCBITS_RE_DB		1	/* Resource Exhaustion data blocks */
#define CCBITS_RE_FLOCK		1	/* Resource Exhaustion file locking */
#define CCBITS_RE_INODE		1	/* Resource Exhaustion inodes */
#define CCBITS_RE_LOG		1	/* Resource Exhaustion log driver */
#define CCBITS_RE_NAMEFS	1	/* Resource Exhaustion namefs */
#define CCBITS_RE_PIPE		1	/* Resource Exhaustion pipes */
#define CCBITS_RE_PROC		1	/* Resource Exhaustion processes */
#define CCBITS_RE_SAD		1	/* Resource Exhaustion sad driver */
#define CCBITS_RE_SCSI		1	/* Resource Exhaustion SCSI */
#define CCBITS_SPEC_DIROFF	1	/* i_diroff incore inode field */
#define CCBITS_SPEC_DIRRM	1	/* non-empty directory removal */
#define CCBITS_SPEC_SYNC	1	/* sync call */
#define CCBITS_SPEC_UNLINK	1	/* unlink open file with 0 link count */
#define CCBITS_SPEC_WAKEUP	1	/* wakeup from locks */
#define CCBITS_CACHE_DNLC	1	/* Cache DNLC */
#define CCBITS_RE_MSG		1	/* Resource Exhaustion, message IPC */

/*
 * Following is the definition for the minimum number of free ids
 * to sample from, in the case that randomization is used to
 * treat a channel.
 */
#define	RANDMINFREE	1024

/*
 * Following are the definitions of the information to be retrieved
 * from cc_getinfo().
 */
#define	CC_PSEARCHMIN	1

/*
 * Following is the definition of covert_t, the structure that is a
 * field of struct user for keeping track of per-lwp covert channels
 * to be treated.
 */

typedef struct covert {
	unsigned int c_bitmap;	/* flags identifying which events occurred */
	unsigned char c_cnt[CC_MAXEVENTS];	/* event counters */
} covert_t;

/* incomplete structure definitions to avoid including header files */
struct cred;

#endif /* _KERNEL || _KMEMUSER*/

#ifdef _KERNEL
extern int random(unsigned long);
extern int cc_getinfo(int);
extern void cc_init(void);
extern void cc_limit_all(covert_t *, struct cred *);
#endif /* _KERNEL */

#endif /* CC_PARTIAL */

/*
 * Used in audit report command to map covert channel
 * event numbers to printable strings.
 */

struct cc_names {
	int	cc_number;
	char	*cc_name;
};

/*
 * Used in audit report command to initialize
 * an array of cc_names.  This must be kept
 * in sync with the #defines above.
 */

#define CC_NAMES			\
	0,	"unused",		\
	1,	"CC_ALLOC_INODE",	\
	2,	"CC_ALLOC_IPC",		\
	3,	"CC_CACHE_MACLVL",	\
	4,	"CC_CACHE_MACSEC",	\
	5,	"CC_CACHE_PAGE",	\
	6,	"CC_RE_DB",		\
	7,	"CC_RE_FLOCK",		\
	8,	"CC_RE_INODE",		\
	9,	"CC_RE_LOG",		\
	10,	"CC_RE_NAMEFS",		\
	11,	"CC_RE_PIPE",		\
	12,	"CC_RE_PROC",		\
	13,	"CC_RE_SAD",		\
	14,	"CC_RE_SCSI",		\
	15,	"CC_SPEC_DIROFF",	\
	16,	"CC_SPEC_DIRRM",	\
	17,	"CC_SPEC_SYNC",		\
	18,	"CC_SPEC_UNLINK",	\
	19,	"CC_SPEC_WAKEUP",	\
	20,	"CC_RE_MSG",		\
	21,	"CC_RE_SEM",		\
	22,	"CC_RE_TIMERS",		\
	23,	"CC_SPEC_TIMERS",	\
	24,	"CC_CACHE_DNLC",	\
	-1,	""

#if defined(__cplusplus)
	}
#endif

#endif /* _ACC_MAC_COVERT_H */
