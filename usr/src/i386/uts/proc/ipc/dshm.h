#ifndef _PROC_IPC_DSHM_H	/* wrapper symbol for kernel use */
#define _PROC_IPC_DSHM_H	/* subject to change without notice */

#ident	"@(#)kern-i386:proc/ipc/dshm.h	1.4.1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
**	IPC Dynamic Shared Memory Facility.
*/

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/param.h>  	/* SVR4COMPAT */
#include <proc/ipc/ipc.h>	/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/param.h>  	/* SVR4COMPAT */
#include <sys/ipc.h>		/* REQUIRED */

#else /* user */


#if defined(_XOPEN_SOURCE)
#include <sys/types.h>
#else
#include <sys/param.h>		/* SVR4COMPAT */
#endif /* defined(_XOPEN_SOURCE)*/

#include <sys/ipc.h>

#endif /* _KERNEL_HEADERS */

/*
 * Implementation constants.
 */

#define DSHM_MIN_ALIGN	0x400000
/*
 * Padding constants used to reserve space for future use.
 */

#define	DSHM_PAD	2

/*
 * Permission Definitions.
 */

#define	DSHM_R	IPC_R	/* read permission */
#define	DSHM_W	IPC_W	/* write permission */

/*
 * DSHM Attach Flags for library use only.
 */

#define _DSHM_PARTIAL   	020000  /* partial attach (else full) */

/*
 * DSHM Control Memory Placement Commands.
 */

#define DSHM_SETPLACE           100
#define _DSHM_MAP_SETPLACE      101
#define	DSHM_CGMEMLOC		102

/*
 * DSHM Control Memory Placement Policy Selectors.
 */

#define DSHM_PLC_DEFAULT    1   /* place by default policy */
#define DSHM_BALANCED       2   /* load balance placement among cpu groups */
#define DSHM_CPUGROUP       3   /* place on specified cpu group */

/*
 * DSHM Attach Count Type Definition.
 */

typedef ulong_t dshmatt_t;

/*
 * Structure Definitions.
 */
/*
 * There is a dynamic shared mem id data structure
 * (kdshmid_ds and dshmid_ds) for each segment in the system.
 */
struct dshmid_ds {
  struct ipc_perm dshm_perm;	/* operation permission struct */
  size_t	  dshm_mapsize;	/* size of segumap mapping */
  const void *	  dshm_mapaddr;	/* User virtual; segment attached at */
  size_t	  dshm_bufsize; /* application's buffer size  */
  ulong_t	  dshm_tbufcnt; /* total buffer count ( map + library admin )*/
  ulong_t	  dshm_abufcnt; /* application's buffer count */
  pid_t		  dshm_lpid;	/* pid of last dshmop */
  pid_t		  dshm_cpid;	/* pid of creator */
  dshmatt_t	  dshm_nattch; 	/* used only for dshminfo */
  time_t	  dshm_atime;	/* last dshmat time */
  long		  dshm_pad1;	/* reserved for possible time_t expansion */
  time_t  	  dshm_dtime;	/* last dshmdt time */
  long		  dshm_pad2;	/* reserved for possible time_t expansion */
  time_t	  dshm_ctime;	/* last change time */
  long		  dshm_pad3;	/* reserved for possible time_t expansion */
  union {
        struct {
                ulong_t         deb_index;
                ulong_t         deb_count;
        } de_buf;
        struct {
                const void *    dem_addr;
                size_t          dem_len;
        } de_map;
  } dshm_extent;
  int		  dshm_placepolicy;
  cgid_t          dshm_cg;
  ulong_t	  dshm_granularity;
  long		  dshm_pad[DSHM_PAD];/* reserved for future expansion */
};

#if defined(_KERNEL) || defined(_KMEMUSER)

#define dtotalbufs kdshm_ds.dshm_tbufcnt
#define dappbufs kdshm_ds.dshm_abufcnt
#define dbuffersz kdshm_ds.dshm_bufsize
#define dalignment kdshm_ds.dshm_align
#define dmapaddr kdshm_ds.dshm_mapaddr
#define dmapsize kdshm_ds.dshm_mapsize


struct kdshmid_ds {
  struct dshmid_ds kdshm_ds;	/* user visible dynamic shared memory header */
  lock_t	   kdshm_lck;  	/* mutex lock for this object.  
				 * This lock potects all fields
				 * in dshmid_ds,
				 * kdshm_flag, and dshm_perm contents.
				 */
  uint_t	   kdshm_refcnt;/* reference count of this header */
  char		   kdshm_flag;	/* set DSHM_BUSY to hold off all
				 * accesses to this object.
				 * set _DSHM_PARTIAL when library
				 * setting affinity for library only buffers
				 */
  sv_t		   kdshm_sv;	/* synchronization variable for this
				 * object.
				 */
  memsize_t	   kdshm_objsz;	/* actual amount of store */
				/* private date for use by VM.. */
  void		  *kdshm_hatp;	/* HAT private data; L2s */
  void		  *kdshm_vpage;	/* segment private; per-vpage data */
  void		  *kdshm_pfn;   /* segment private; pfns for object */
};

#define	DSHM_BUSY	0x1	/* the DSHM segment is currently
                                 * been manipulated. This will block out
				 * dshmconv and consequently all dshmat,
				 * dshmctl, aclipc and lvlipc calls.
				 * The other user dshmdt will do explicit
				 * serialization with the above routines.
				 */
#define DSHM_AF_BUSY	0x2	/* Defining Afinity 	*/
#define DSHM_AF_DONE	0x4	/* Affinity done i.e. all pfns allocated */


struct	dshminfo {
	int	dshmmin,	/* min shared memory segment size */
		dshmmni;	/* # of shared memory identifiers */
};

#endif	/* _KERNEL || _KMEMUSER */

#ifdef	_KERNEL

/*
 * Macros to lock and unlock a kshmid_ds cell.
 * idp is a pointer to the kshmid_ds cell to be locked/unlocked.
 */
#define DSHMID_LOCK(kdshmp)	LOCK_PLMIN(&(kdshmp)->kdshm_lck)
#define DSHMID_UNLOCK(kdshmp, pl) UNLOCK_PLMIN(&(kdshmp)->kdshm_lck, (pl))

extern int	dshmconv(int , struct kdshmid_ds **);
extern void	dshminit(void);
struct segacct;
extern int	kdshmdt(struct segacct *, boolean_t);
#else

#ifdef __STDC__

size_t	_dshm_kalignment(void);
int _dshmget(key_t, size_t, unsigned long, unsigned long, const void *, 
	     size_t, int);
void * _dshmat(int, int);
int _dshm_remap(int, const void *, unsigned long);
int _invlpg(const void *, size_t);
int _dshmdt(const void *);
int _dshmctl(int, int, struct dshmid_ds *);
#else

size_t	_dshm_kalignment();
int _dshmget();
void * _dshmat();
int _dshm_remap();
int _invlpg();
int _dshmdt();
int _dshmctl();

#endif /* __STDC__ */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_IPC_DSHM_H */

