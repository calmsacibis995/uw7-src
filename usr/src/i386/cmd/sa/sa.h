/* copyright "%c%" */
#ident	"@(#)sa:i386/cmd/sa/sa.h	1.9.3.1"
#ident	"$Header$"

/* sa.h
 * 
 * Item codes (SAR_* constants) and item layouts (*_info structures), 
 * common to SADC and SAR.  
 *
 */

#include <sys/types.h>
#include <time.h>
#include <sys/param.h>
#include <sys/dl.h>
#include <sys/utsname.h>

#include <sys/metrics.h>

#include <sys/ksynch.h>
#include <sys/ddi.h>

#ifdef HEADERS_INSTALLED
# include <sys/cpumtr.h>
# include <sys/cgmtr.h>
#else /# !HEADERS_INSTALLED */
# include "../cpumtr/cpumtr.h" 
# include "../cgmtr/cgmtr.h"
#endif /* !HEADERS_INSTALLED */

#include <sys/procfs.h>


typedef unsigned long   flag32;
typedef char            flag;
typedef int             ID;



/* 
 * This is the header structure that starts each record.  
 *
 * reserved:   This field is reserved for future use.
 *  
 * item_code:   Identifies a group of related metrics (referred
 *              to as a "metric group."  Future releases may
 *              append new metrics to the end of an existing 
 *              group and add or remove groups as a whole, however
 *              individual metrics may not be deleted from a group.
 *              
 * item_size:   Size (in bytes) of each of the items written in the 
 *              data.  
 *
 * item_count:  Number of items written.
 *
 * Note that within an item, there may be a further subdivision
 * into sub-item.  It is up to the read routine for that item
 * to recognize this and read the item correctly.
 *
 * Total length of the data portion of a record is
 * item_size * item_count bytes.
 *
 */


typedef struct sar_header {
	short	reserved;     
	short	item_code;       
	short	item_size;
	short	item_count;
} sar_header;


/* item_codes */

#define SAR_INIT		0	/* first block in file */
#define SAR_KMEM_SIZES		1     	/* size of kmem pools (special) */
#define SAR_FS_NAMES      	2	/* file system type names (special) */
#define SAR_START_REC      	3	/* start sample record (special) */
#define SAR_CPU_P	        4	/* metp_cpu */
#define SAR_LOCSCHED_P	   	5	/* metp_sched */
#define SAR_GLOBSCHED	   	6	/* mets_sched */
#define SAR_BUFFER_P	      	7	/* metp_buf */
#define SAR_SYSCALL_P	   	8	/* metp_syscall */
#define SAR_PROCRESOURCE   	9	/* mets_proc_resrc */
#define SAR_FS_LOOKUP_P	  	10	/* metp_filelookup */
#define SAR_FS_TABLE	      	11	/* mets_files */
#define SAR_FS_ACCESS_P	   	12	/* metp_filesaccess */
#define SAR_FS_INODES	   	13	/* mets_ionodes */
#define SAR_TTY_P	        14	/* metp_tty */
#define SAR_IPC_P		15	/* metp_ipc */
#define SAR_VM_P		16	/* metp_vm */
#define SAR_KMEM_P		17	/* metp_kmem */
#define SAR_MEM			18	/* mets_mem */
#define SAR_DISK		19	/* met_disk_stats */
#define SAR_LWP_RESRC_P		20	/* metp_lpw_resrc */
#define SAR_HWMETRIC_CPU	21	/* hardware metrics from CPU */
#define SAR_HWMETRIC_LWP	22	/* per-LWP hardware metrics from CPU */
#define SAR_HWMETRIC_CG		23	/* hardware metrics from CG-interface */
#define SAR_HWMETRIC_INIT       24	/* number of CGs, type of CPU/CG */
#define SAR_PS                  25	/* 'ps' info */
#define SAR_CPUCG		26	/* CPU to CG mapping */

/* 
 * Structures used by read routines in sar and write routines in sadc.
 *
 */

struct sar_init_info {
	int			num_engines;
	int			fs_namesz;
	int			num_fs;
	int			num_kmem_sizes;
	time_t			boot_time;
	struct mets_native_units	mets_native_units;
	struct utsname		name;
};


/* sar_start_info is made a struct to facilitate addition of new
 * fields in the future.
 */

struct sar_start_info {
	time_t	timestamp;
};


/* buffer cache metrics - per processor */

struct buf_info {
	ID		id;
	struct metp_buf	data;
};


/* cpu metrics - per processor */

struct cpu_info {
	ID    		id;
	struct metp_cpu	data;
};


/* fileaccess metrics - per processors, per filesystem*/

struct facc_info {
	ID			id1;
	ID			id2;
	struct metp_fileaccess	data;
};


/* file lookup metrics - per processor */

struct flook_info {
	ID			id;
	struct metp_filelookup	data;
};


/* inter-process communication metrics - per processor */

struct ipc_info {
	ID		id;
	struct metp_ipc	data;
};


/* processor scheduling metrics - per processor */

struct lsched_info {
	ID			id;
	struct metp_sched	data;
};


/* system call metrics - per processor */

struct syscall_info {
	ID			id;
	struct metp_syscall	data;
};


/* tty metrics - per processors */

struct tty_info {
	ID		id;
	struct metp_tty	data;
};



/* vm metrics - per processor */

struct vm_info {
	ID		id;
	struct metp_vm	data;
};


/* lwp_resource metrics - per processor */

struct lwp_resrc_info {
	ID			id;
	struct metp_lwp_resrc	data;
};


/* inode metrics - per processor */

struct inodes_info {
	ID			id;
	struct mets_inodes	data;
};

struct hwmetric_init_info {
	int			num_cg;
	int			cpu_type;
	int			cg_type;
};


/* process info - per process */

struct ps_info {
  pid_t   pi_pid;           /* process id */
  char    pi_fname[PRFNSZ]; /* last component of exec()ed path name */
  char    pi_psargs[PRARGSZ];  /* initial characters of arg list */
};



