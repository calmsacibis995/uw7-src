#ifndef _PROSRFS_H	/* wrapper symbol for kernel use */
#define _PROSRFS_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/profs/prosrfs.h	1.6.3.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <svc/clock.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/clock.h>

#else 	/* As a user include file */

#include <sys/time.h>

#endif /* _KERNEL_HEADERS */

/*
 * As of UnixWare 7, new fields have been added to the end
 * of the /system/processor/nnn profs file.  For compatibility
 * with existing binaries the structure and contents of the
 * old procfile_t are maintained.
 *
 * The pfs_chip field is an index into the pfs_chip_map array
 * that indicates what kind of processor the system is treating this as
 * (for example, a Pentium II might be treated as a Pentium Pro by the
 * kernel).  The new pfs_name field is a string that indicates the kernel's
 * best guess as to the actual processor type, and the pfs_model string may
 * contain additional processor version information.  Both of the new fields
 * are intended for informational purposes only; applications should avoid
 * basing decisions on these values, particularly the pfs_model string, as
 * they are subject to change from release to release.
 */

#define PFS_NAMELEN	257

typedef struct pfsproc {
	int	pfs_status;
	int	pfs_chip;
	int	pfs_clockspeed;	/* in MHz - currently meaningless */
	int	pfs_cachesize;	/* in Kbytes - currently meaningless */
	int	pfs_fpu;
	int	pfs_bdrivers;
	timestruc_t pfs_modtime;
	char	pfs_name[PFS_NAMELEN];
	char	pfs_model[PFS_NAMELEN];
} pfsproc_t;

typedef struct message {
	int 		m_cmd;		/* online or offline */
	processorid_t	m_argument;	/* processor id */
} ctlmessage_t;


/* chip types */
#define I_386	1
#define I_486	2
#define I_586	3
#define I_686	4

/* fpu types */
#define FPU_387		1	/* i387 present */
#define FPU_287		2
#define FPU_NONE	3	/* no fpu */


#ifndef _KERNEL

/*
 * Structure of a /system/processor/nnn profs file as it was
 * through UnixWare 2.X.  This is retained for compatibility
 * purposes; new code should use pfsproc_t instead.
 */
typedef struct procfile {
	int status;
	int chip;
	int clockspeed;
	int cachesize;
	int fpu;
	int bdrivers;
	timestruc_t modtime;
} procfile_t;

#ifdef PSRINFO_STRINGS

/*
 * Chip names indexed by pfs_chip,
 * should be used only by legacy applications.
 */

static char * 
pfs_chip_map[]= {
"????",
"i386",
"i486",
"Pentium",
"Pentium_Pro"
};

/*
 * Floating point support names indexed by pfs_fpu.
 */

static char *
pfs_fpu_map[]= {
"????",
"i387",
"i287",
"NO"
};

/*
 *	macros for printing processor and fpu type
 */

#define PFS_CHIP_TYPE(index) ((index > MAX_CHIP_TYPE )?"Bad-chip-type": \
pfs_chip_map[index])

#define PFS_FPU_TYPE(index) ((index > MAX_FPU_TYPE )?"Bad-fpu-type": \
pfs_fpu_map[index])

#endif	/* PSRINFO_STRINGS */

#define MAX_CHIP_TYPE 4		/* this must agree with pfs_chip_map */
#define MAX_FPU_TYPE 3		/* this must agree with pfs_fpu_type */

#define PI_TYPELEN 16		/* max length of the name of the processor */
#define PI_FPUTYPE 32

#define PISTATE_BOUND   0x00100000
#define PISTATE_ONLINE  0x00000001


typedef struct processor_info {
        int     pi_state;                       /* P_ONLINE or P_OFFLINE */
	int	pi_nfpu;			/* for 4.0 MP compatibility */
        int     pi_clock;                       /* in MHZ */
#define pi_cputype pi_processor_type		/* for 4.0 MP compatibility */
        char    pi_processor_type[PI_TYPELEN];  /* in ascii */
        char    pi_fputypes[PI_FPUTYPE];        /* ascii string */
} processor_info_t;

#ifdef  __STDC__
int processor_info(processorid_t, processor_info_t*);
#else
int processor_info();
#endif  /* __STDC__ */

/*
 *	useful strings and macros
 */
#define PFS_DIR		"/system/processor"		/* base directory */
#define PFS_CTL_FILE	"/system/processor/ctl"		/* control file */
#define PFSTYPE		"processorfs"
#define PFS_FORMAT	"%03d"		/* converts processor_id to file name */

#endif  /* KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _PROSRFS_H */
