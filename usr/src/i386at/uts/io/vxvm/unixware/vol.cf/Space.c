/* @(#)uts.vxvm:vxvm/unixware/Space.c	1.5.2.2 11/25/97 17:38:51 - %Q */
#ident	"@(#)uts.vxvm:vxvm/unixware/Space.c	1.5.2.2"

/*
 * Copyright(C)1996 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
 * UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
 * LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
 * IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
 * OR DISCLOSURE.
 * 
 * THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 * TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
 * OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
 * EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
 * 
 *               RESTRICTED RIGHTS LEGEND
 * USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
 * SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
 * (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
 * COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
 *               VERITAS SOFTWARE
 * 1600 PLYMOUTH STREET, MOUNTAIN VIEW, CA 94043
 */
#define UW_2_1

#ifndef _FSKI
#define	_FSKI	1
#endif

#include	"config.h"

#include	"sys/ksynch.h"
#include	"sys/conf.h"
#include	"sys/param.h"
#include	"sys/vtoc.h"

#include	"vxvm/voldefs.h"
#include	"vxvm/vollocks.h"
#include	"vxvm/vollocal.h"
#include	"vxvm/voldefault.h"
#include	"vxvm/volioctl.h"
#include	"vxvm/voldioctl.h"
#include	"vxvm/volstats.h"
#include	"vxvm/volkern.h"
#include	"vxvm/voliomem.h"
#include	"vxvm/volkio.h"
#include	"vxvm/volsio.h"

#ifdef SOLARIS
#ifdef SOLARIS_2_5
#include      <sys/promimpl.h>
#else /* !SOLARIS_2_5 */
#include      <sys/promcommon.h>
#endif /* SOLARIS_2_5 */
#endif /* SOLARIS */

/*
 *
 * parameters controlling root and swap volumes:
 *
 *      NO_SASSIGN
 *              Define stub values for vol_rootdev_is_volume and
 *              vol_swapdev_is_volume, so to avoid link problems when
 *              the volume manager kernel drivers are loadable.
 *              Normally, these variables are set from an sassign.d
 *              file, but sassign.d files are not used for loadable
 *              drivers.
 *
 *      VOL_ROOTDEV_IS_VOLUME
 *      VOL_SWAPDEV_IS_VOLUME
 *              Indicate the the root/swap file systems are volumes.
 *              This enables the automatic configuration of volumes
 *              with minor numbers 0 and 1, at boot time, as direct
 *              read-only pass-through volumes to the devices defined
 *              by vol_rootvol_dev, and vol_swapvol_dev.
 *
 * NOTE: These parameters are modified automatically by installation
 *       scripts.
 */
 
/*
 * If required some of the #defines used here can be converted to
 * a variable so that it can be set through the /etc/system file on
 * Solaris.
 */
#define VOL_NO_SASSIGN
#undef VOL_ROOTDEV_IS_VOLUME
#undef VOL_SWAPDEV_IS_VOLUME
 
#ifdef VOL_NO_SASSIGN
voldevno_t vol_rootvol_dev = VOLNODEV;	/* underlying root volume dev */
voldevno_t vol_swapvol_dev = VOLNODEV;	/* underlying swap volume dev */
#endif  /* VOL_NO_SASSIGN */
 
#ifdef SOLARIS
int	vol_root_dev_maj = 0;	/* can be initialized in /etc/system */
int	vol_root_dev_min = 0;	/* can be initialized in /etc/system */
int	vol_swap_dev_maj = 0;	/* can be initialized in /etc/system */
int	vol_swap_dev_min = 0;	/* can be initialized in /etc/system */
 
/* 
 * vol_usrpart_is_volume indicates that device mounted on /usr 
 * is a volume. vold knows about this through vol_is_usr field 
 * in volinfo. The usr volume is started from vold when vold
 * mode is set to boot. This is esential so that / can be 
 * fscked and mounted read-write. 
 */
int	vol_usrpart_is_volume = 0;
#endif /* SOLARIS */

#ifdef VOL_ROOTDEV_IS_VOLUME
int     vol_rootdev_is_volume = 1;
#else
int     vol_rootdev_is_volume = 0;
#endif /* VOL_ROOTDEV_IS_VOLUME */

#ifdef VOL_SWAPDEV_IS_VOLUME
int     vol_swapdev_is_volume = 1;
#else
int     vol_swapdev_is_volume = 0;
#endif /* VOL_SWAPDEV_IS_VOLUME */

#ifdef SOLARIS
/*
 * for Solaris, only simple variables can be tuned, so define external
 * variables for all tunables in the volinfo structure.
 */

int vol_maxvol = MAX_VOL;
int vol_subdisk_num = SUBDISK_NUM;
int vol_maxioctl = VOL_MAXIOCTL;
int vol_maxspecialio = VOL_MAXSPECIALIO;
int vol_maxio = VOL_MAXIO;
int vol_maxkiocount = VOL_MAXKIOCOUNT;
int vol_default_iodelay = VOL_DEFAULT_IODELAY;
int voldrl_min_regionsz = VOLDRL_MIN_REGIONSZ;
int voldrl_max_drtregs = VOLDRL_MAX_DRTREGS;
int vol_maxparallelio = VOL_MAXPARALLELIO;
int vol_mvr_maxround = VOL_MVR_MAXROUND;
int vol_maxstablebufsize = VOL_MAXSTABLEBUFSIZE;
int voliot_iobuf_limit = VOLIOT_IOBUF_LIMIT;
int voliot_iobuf_max = VOLIOT_IOBUF_MAX;
int voliot_iobuf_default = VOLIOT_IOBUF_DEFAULT;
int voliot_errbuf_default = VOLIOT_ERRBUF_DEFAULT;
int voliot_max_open = VOLIOT_MAX_OPEN;
int vol_checkpt_default = VOL_CHECKPT_DEFAULT;
int volraid_rsrtransmax = VOLRAID_RSRTRANSMAX;
#endif /* SOLARIS */

/*
 * The volinfo structure
 */
struct volinfo volinfo = {
	VOL_KERNEL_VERSION,			/* version */
	VOL_PRIVMEM_MAX_ID,			/* max_volprivmem */
#ifdef SOLARIS
	/*
	 * On Solaris, these numbers are filled in at load time.
	 */
	0,
	0,
	0,
#else /* !SOLARIS */
#ifndef VOL_BMAJOR_0
#ifdef VOL_TESTING
        0,
        0,
        0,
#endif VOL_TESTING
#else /* not !VOL_BMAJOR_0) */
        VOL_BMAJOR_0,                           /* vol_bmajor */
        VOL_CMAJOR_0,                           /* vol_cmajor */
#ifdef PLEX_CMAJOR_0
        PLEX_CMAJOR_0,                          /* plex_cmajor */
#else
        NODEV,                                  /* plex_cmajor */
#endif /* PLEX_CMAJOR */
#endif /* !VOL_BMAJOR_0 */
#endif /* SOLARIS */
	MAX_VOL,				/* max # of volumes */
	MAX_PLEX,				/* max # of plexes */
	PLEX_NUM,				/* max plexes per volume */
	SUBDISK_NUM,				/* max sd's per plex */
	VOL_MAXIOCTL,				/* max size of ioctl data */
	VOL_MAXSPECIALIO,			/* max size of ioctl I/O ops */
	VOL_MAXIO,				/* max I/O size in sectors */
	VOL_MAXKIOCOUNT,			/* max # of concurrent I/Os */
	VOL_DEFAULT_IODELAY,			/* default I/O delay - utils */
	VOLDRL_MIN_REGIONSZ,			/* min DRL region size */
	VOLDRL_MAX_DRTREGS,			/* max # of DRL dirty regs */
	VOL_MAXPARALLELIO,			/* max # voldios allowed */
#ifdef SOLARIS
	0,					/* root is not volume */
	VOL_MVR_MAXROUND,			/* max round-robin reg. size */
	0,					/* prom version, filled in */
#else   /* !SOLARIS */
        0,                                      /* unused */
        VOL_MVR_MAXROUND,                       /* max round-robin reg. size */
        0,                                      /* unused */
#endif  /* SOLARIS */
	VOL_MAXSTABLEBUFSIZE,			/* max # of blocks for
						   stable buf copies */
	VOLIOT_IOBUF_LIMIT,			/* max total trace buf spc */
	VOLIOT_IOBUF_MAX,			/* max size of I/O trace buf */
	VOLIOT_IOBUF_DEFAULT,			/* default I/O trace buf sz */
	VOLIOT_ERRBUF_DEFAULT,			/* default err trace buf sz */
	VOLIOT_MAX_OPEN,			/* max # of trace channels */
	VOL_CHECKPT_DEFAULT,			/* default checkpoint size */
	VOLRAID_RSRTRANSMAX,			/* max # of transient RSRs */
};

/*
 * volkioq_start - queue of unstarted top-level I/Os
 * volsioq_start - queue of I/O stages to start
 * volsioq_idle_start - queue of I/O stages to start before iod goes to sleep
 * volsioq_done - queue of I/O stage completions to process
 *
 * These three queues define the queues of work to perform immediately
 * for the I/O daemons.
 */
struct volkioq volkioq_start;
struct volsioq volsioq_start;
struct volsioq volsioq_idle_start;
struct volsioq volsioq_done;

/*
 * volsioq_restart - queue of I/O stages to start later
 * volsioq_redone - queue of I/O stage completions to process later
 *
 * These two queues define work queues to be processed the next time
 * an I/O daemon wakes up.  Work that cannot be done immediately can
 * be retried the next time an I/O daemon wakes up, on the presumption
 * that letting the I/O daemons sleep for a short period of time may
 * clear whatever condition makes processing temporarily impossible.
 *
 * NOTE: There is no volkioq_restart queue.  This is because iogen
 *	 functions cannot be deferred if VOLMEM_SLEEP is passed, so
 *	 an iogen function called from an I/O daemon cannot cause the
 *	 kio to be deferred.
 */
struct volsioq volsioq_restart;
struct volsioq volsioq_redone;

/*
 * volkioq_commitwait_start - top-level I/Os waiting for transaction commit
 *
 * Top-level I/Os that are blocked due to a transaction lock are put on
 * this list.  When a transction commit completes, I/Os are moved from
 * the volkioq_twait_start list to the volkioq_start list for
 * processing by the I/O daemons.
 */
struct volkioq volkioq_commitwait_start;

#ifdef VOLSIO_DEBUG
/*
 * volsio_list - list of all SIOs on the system
 * volsio_count - count of SIOs on volsio_list
 * volsio_list_lock - lock for update access to volsio_list
 *
 * This list is managed only when the kernel is compiled with appropriate
 * debugging options.  The VOLSIO_REGISTER() macro adds to this list,
 * the VOLSIO_UNREGISTER() macro then deletes from the list.
 */

struct volsio *volsio_list;
size_t volsio_count;
volfastlock_t volsio_list_lock;
#endif	/* VOLSIO_DEBUG */

/*
 * volopenclose_sleep - sleep lock for open/close serialization
 *
 * volopenenter, volcloseenter, and transactions obtain this lock to
 * ensure open, close, and transaction serialization.
 */
volocsleep_t volopenclose_sleep;

/*
 * volop_rwsleep - shared/exclusive sleep lock for operations
 *
 * volopenter obtains this lock for shared access.  Transactions
 * obtain this lock for exclusive access.  This lock ensures that
 * transactions happen with no operations in progress.
 */
volrwsleep_t volop_rwsleep;

/*
 * volio_spin - spinlock to protect interactions with I/O daemons
 *
 * volkiostart obtains this lock before checking resources, getting
 * virtual devices, and before incrementing volkiocount.  This lock is
 * also obtained to manipulate the work lists for the I/O daemons.
 */
volspin_t volio_spin;

/*
 * voliod_sync - wait lock for I/O daemons
 *
 * I/O daemons wait on this synchronization lock.  Daemons should be
 * woken up when there is work to do.
 */
volsync_t voliod_sync;

/*
 * volmapin_spin - spinlock to protect mapin limiting code.
 */
volspin_t vol_mapin_spin;

/*
 * vol_mapin_reserve - Value of reservation space for bp_mapin currently
 * in use.
 */
size_t vol_mapin_reserve;

/*
 * volkioq_mapin - Q of kios waiting for bp_mapin space to be freed from the
 * reservation pool.
 */
struct volkioq volkioq_mapin;

/*
 * vol_bpsync_hash - Hash table of sync locks used to provide synchronization
 * point for blocked kios waiting for bp_mapin space.  Hash list saves from
 * allocating synclocks in the I/O path and saves from waking every sleeping
 * kio for every kiodone().
 */
volsync_t vol_bpsync_hash[VOL_BPSYNC_NHASH];

/*
 * volkiocount - count of top-level I/Os being processed
 *
 * This is incremented each time a top-level I/O is processed, and is
 * decremented when each top-level I/O completes.  This is used
 * to determine whether I/o daemons can be stopped.  I/O daemons will
 * go away (due to reduction in the voliod_want_count variable) only
 * if volkiocount is zero and the volkioq_start and commitwait_start
 * queues are both empty.  The two volkioq_* lists are used to hold
 * KIO's that are not yet being processed.  These two lists are not
 * counted in volkiocount.
 *
 * volkiocount is also used to limit the number of KIO's that are
 * being processed concurrently.  volkiocount is prevented from ever
 * exceeding a threshold value stored in the volinfo structure.  A
 * new KIO that would exceed the threshold is queued to the
 * volkioq_start list.
 */
int volkiocount = 0;

/*
 * volkio_overrun - set if too many KIOs are started
 *
 * If more than vol_maxkiocount KIOs are given to the system, then
 * the additional KIOs are just queued onto the volkioq_start queue,
 * and volkio_overrun is set.  When enough KIOs are done'd to cause
 * volkiocount to drop below vol_maxkiocount, volkio_overrun is
 * cleared and an I/O daemon is woken up to process the queued KIOs.
 */
int volkio_overrun = 0;

/*
 * voliod_running - number of currently running I/O daemons
 * voliod_need - indicate that an I/O daemon is needed
 *
 * These variables are used to avoid unnecessary wakeups on the I/O
 * daemons.  voliod_running counts the number of I/O daemons
 * that are currently running (that are not sleeping waiting for more
 * work).  If voliod_running is non-zero, no wakeup is necessary.
 *
 * voliod_need is set to 1 whenever a wakeup is issued against the 
 * I/O daemons, and is cleared whenever an I/O daemon wakes up to look
 * for more work.  Thus, if voliod_need is 1, then a wakeup has
 * already been issued, and further wakeups are not yet necessary.
 */
int voliod_running;
int voliod_need;

/*
 * voliod_count - count of I/O daemons
 * voliod_want_count - count of desired I/O daemons
 *
 * These variables control the number of running I/O daemons.
 * voliod_count is the number of I/O daemon processes that
 * currently exist (not counting I/O daemons started to handle one
 * particular I/O).  If this number exceeds voliod_want_count,
 * then I/O daemons will start exiting until the number reaches
 * voliod_want_count.  I/O daemons will exit only if there are no
 * outstanding I/Os in the system.
 */
int voliod_count;
int voliod_want_count;

/*
 * vol_ktrans_klocks - list of lock buffers for the current transaction
 * volkstate	     - current transaction state
 * vol_config_loaded - set to 1 after first successful transaction
 * vol_commit_in_progress - set to 1 for duration of a transaction commit
 */
struct volktrans_klocks	*vol_ktrans_klocks = NULL;
struct volktranstate	volkstate;
int			vol_config_loaded = 0;
int			vol_commit_in_progress = 0;

/*
 * Global trace event number
 */
voleventno_t		voliot_eventno;

/*
 * Other global vars
 */
pid_t			voldpid = 0;	/* pid of vold process, or 0 */
void			*voldprocp;	/* proc pointer for vold process */
volfastlock_t		voldpid_lock;	/* lock for voldpid */

struct voldevlist	*voldevlist;	/* list of volume devices */
struct voldevlist	*volnewdevlist;	/* pending list of volume devices */
struct voldevice	*vololddevices;	/* devices to be removed */
struct voldg		*voldglist = NULL; /* list of disk groups */
volfastlock_t		voldglist_lock;	/* disk group list lock */
struct voldg		*vol_transdg;	/* dg for current transaction */
struct voldg		vol_nulldg;	/* the null disk group */

struct volspec		volspec[MAXVSPEC];

/*
 * volioctl_ktok_info - kernel-to-kernel volioctl_info structure
 *
 * VxVM kernel functions can point to this structure as the volioctl_info
 * structure to use when calling other ioctl functions within the kernel,
 * where the argument in the ioctl points to kernel space, not user space.
 */
struct volioctl_info volioctl_ktok_info = VOLIOCTL_KTOK_INFO;

/*
 * Global objects for RAID
 */
struct volsioq	volraidrsrq;
volfastlock_t	volraidrsr_spin;
int		volraidrsr_transcnt;	/* transient RSR recovery count */

/*
 * Device interface flag
 */
#ifdef UW_2_1
int                     voldevflag = D_NEW | D_MP | D_NOBRKUP;
#else
int			voldevflag = D_NEW | D_DMA | D_NOBRKUP;
#endif /* UW_2_1 */

/*
 * Disk device information. Solaris systems fill this in at
 * driver load time.
 */
#if defined(NO_SVR4_SDI) || defined(SOLARIS)

struct vol_diskdriver_info	vol_ddi_info;
major_t			vol_ddi_cmajors[SD01_CMAJORS+1];
major_t			vol_ddi_bmajors[SD01_CMAJORS+1];

#else /* !NO_SVR4_SDI && !SOLARIS */
 
static major_t vol_ddi_cmajors[] = {
        7679,0
};
 
static major_t vol_ddi_bmajors[] = {
        7679,0
};

/*
 * NOTE: Post-BL11 Number of minors per major will be reduced from 512 to 256
 */
struct vol_diskdriver_info vol_ddi_info = {
        256,
        V_NUMPAR,
        1,
        vol_ddi_cmajors,
        vol_ddi_bmajors,
};
 
#endif /* NO_SVR4_SDI */

/*
 * Maximum number of DRL dirty regions in sequential mode
 */
ulong_t		voldrl_max_seq_dirty = VOLDRL_MAX_SEQUENTIAL_DIRTY;

#if defined(VXVM_DEBUG)
int	vol_mapin_hit = 0;
int	vol_mapin_miss = 0;
int	vol_mapin_wakeup = 0;
#endif	/* VXVM_DEBUG */

/*
 * vol_debug_level - determine which debugging messages should be printed
 */
#if defined(VXVM_DEBUG)
int	vol_debug_level = 1;
#else	/* not VXVM_DEBUG */
int	vol_debug_level = 0;
#endif	/* VXVM_DEBUG */

/*
 * vol_call_debugger() - function to call to trap to a debugger
 */
#if defined(VXVM_DEBUG)
char *vol_debugger_msg;
VOID *vol_debugger_arg;

void
vol_call_debugger(arg, msg)
	VOID *arg;
	char *msg;
{
	vol_debugger_msg = msg;
	vol_debugger_arg = arg;

#ifdef SVR4
        call_demon(msg);
#else   /* not SVR4 */
#ifdef SOLARIS
	debug_enter(msg);
#else   /* not SOLARIS */
#ifdef UW_2_1
        if (msg) {
                cmn_err(CE_CONT,"%s\n", msg);
        }
        call_demon();
#else /* !UW_2_1 */
        VOL_WARN_MSG2("BREAKPOINT: %s, arg=%x", msg, (unsigned)arg);
#endif /* UW_2_1 */
#endif  /* SOLARIS */
#endif  /* SVR4 */
}
#endif	/* VXVM_DEBUG */

#ifdef SOLARIS
/*
 * Store miscellaneous tunables from the /etc/system-settable
 * simple variables into the volinfo structure. Plus 3 or 4 variables
 * in volinfo which can't be staticly configured.
 */
int 
volinfo_init()
{
	/*
	 * The block and character device numbers must be
	 * filled in at load time on Solaris systems.
	 */
	volinfo.volcmajor = ddi_name_to_major(VOL_DRIVER);
	volinfo.volbmajor = ddi_name_to_major(VOL_DRIVER);
	volinfo.vol_is_root = vol_rootdev_is_volume;
	volinfo.prom_version  = obp_romvec_version;

	/*
	 * Store miscellaneous tunables from the /etc/system-settable
	 * simple variables into the volinfo structure.
	 */
	volinfo.maxvol = vol_maxvol;
	volinfo.sdnum = vol_subdisk_num;
	volinfo.max_ioctl = vol_maxioctl;
	volinfo.max_specio = vol_maxspecialio;
	volinfo.max_io = vol_maxio;
	volinfo.vol_maxkiocount = vol_maxkiocount;
	volinfo.dflt_iodelay = vol_default_iodelay;
	volinfo.voldrl_min_regionsz = voldrl_min_regionsz;
	volinfo.voldrl_max_drtregs = voldrl_max_drtregs;
	volinfo.max_parallelio = vol_maxparallelio;
	volinfo.mvrmaxround = vol_mvr_maxround;
	volinfo.vol_maxstablebufsize = vol_maxstablebufsize;
	volinfo.voliot_iobuf_limit = voliot_iobuf_limit;
	volinfo.voliot_iobuf_max = voliot_iobuf_max;
	volinfo.voliot_iobuf_default = voliot_iobuf_default;
	volinfo.voliot_errbuf_default = voliot_errbuf_default;
	volinfo.voliot_max_open = voliot_max_open;
	volinfo.vol_checkpt_default = vol_checkpt_default;
	volinfo.volraid_rsrtransmax = volraid_rsrtransmax;
}
#endif /* SOLARIS */
