#ifndef _SVC_PSM_H	/* wrapper symbol for kernel use */
#define _SVC_PSM_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:svc/psm.h	1.12.4.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This header file may be included by several different types of users:
 *
 *	1) PSMs conforming to PSM version 2 or greater.
 *		These must #define _PSM to the version number before
 *		including any system header files.
 *	2) Core kernel providers of PSM interfaces conforming to PSM
 *	   version 2 or greater (e.g. toolkits)
 *		These must #define _PSM to the version number before
 *		including any system header files.
 *	3) Core kernel users of PSM interfaces that are not themselves
 *	   PSM conforming.
 *		These must not #define _PSM.
 *
 * Through appropriate conditional compilation based on these symbols,
 * type definitions may be tailored to specific PSM versions, various
 * interfaces may be provided alternately as functions or macros, and
 * non-PSM symbols may be hidden from the namespace of conforming PSMs.
 *
 * Extern references to special symbol names are generated so the
 * definition of _PSM (or lack thereof) can be matched with the interface
 * definition referenced by the driver's Master file. This allows us,
 * for example, to make sure that a "$interface psm 2" PSM actually
 * defines _PSM to 2.
 */

#if defined(_PSM)
#if _PSM != 2
#error "Unsupported PSM version"
#endif
#endif

#ifndef __PSM_SYMREF

struct __PSM_dummy_st {
	int *__dummy1__;
	const struct __PSM_dummy_st *__dummy2__;
};
#define __PSM_SYMREF(symbol) \
	extern int symbol; \
	static const struct __PSM_dummy_st __dummy_PSM = \
		{ &symbol, &__dummy_PSM }

#if !defined(_PSM)
__PSM_SYMREF(No_PSM_version_defined);
#pragma weak No_PSM_version_defined
#else
#define __PSM_ver(x) _PSM_##x
#define _PSM_ver(x) __PSM_ver(x)
__PSM_SYMREF(_PSM_ver(_PSM));
#endif

#endif  /* __PSM_SYMREF */


/*
 * Basic types
 */
typedef int		ms_bool_t;		/* MS_TRUE / MS_FALSE boolean */
typedef int		ms_cpu_t;		/* cpu number or count */
typedef int		ms_cgnum_t;		/* CPU group number or count */
typedef long long	ms_cgid_t;		/* CPU group identifier */
typedef unsigned int	ms_islot_t;		/* interrupt slot number into interrupt cntlr */
typedef unsigned int	ms_ivec_t;		/* CPU interrupt vector number or count */
typedef unsigned int	ms_port_t;		/* IO space port number */
typedef unsigned int	ms_size_t;		/* size in bytes of software addressable memory */

typedef void		*ms_lockp_t;		/* pointer to a simple mutex lock */
typedef unsigned int	ms_lockstate_t;		/* opaque state returned when lock is acquired */

typedef unsigned long long	ms_memsize_t; 	/* size in bytes of phys mem */
typedef unsigned long long	ms_paddr_t; 	/* phys addr relative to CG */


/*
 * This is an 8-bit integral data type that holds a bitmask of event types.
 * Events are asynchronous indication of special conditions which can be
 * triggered by hardware events of sent between processors by software
 * requests.
 */
typedef unsigned char 	ms_event_t;


/*
 * The list of event types posted by the PSM.
 * These values are passed to os_post_events.
 */
#define	MS_EVENT_TICK_1		(0x01)	/* tick 1 event has occurred */
#define	MS_EVENT_TICK_2		(0x02)	/* tick 2 event has occurred */
#define	MS_EVENT_OS_1		(0x04)	/* OS-specific event type 1 */
#define	MS_EVENT_OS_2		(0x08)	/* OS-specific event type 2 */
#define	MS_EVENT_OS_3		(0x10)	/* OS-specific event type 3 */
#define	MS_EVENT_OS_4		(0x20)	/* OS-specific event type 4 */
#define	MS_EVENT_PSM_1		(0x40)	/* PSM-specific event type 1 */
#define	MS_EVENT_PSM_2		(0x80)	/* PSM-specific event type 2 */

/*
 * Aliases for kernel use only for OS events.
 */
#define MS_EVENT_SOFT		MS_EVENT_OS_1		/* soft interrupt */
#define MS_EVENT_XCALL		MS_EVENT_OS_2		/* xcall */


/*
 * Definitions for the various kinds of node descriptors.
 */

#define MSR_NONE	0
#define MSR_CPU         1
#define MSR_ICACHE      2
#define MSR_DCACHE      3
#define MSR_UCACHE      4
#define MSR_BUS         5
#define MSR_MEMORY	6
#define MSR_CG		7


#define MSR_BUS_NONSTD	0
#define MSR_BUS_PCI	1
#define MSR_BUS_EISA	2
#define MSR_BUS_MCA	3
#define MSR_BUS_ISA	4

/*
 * Macros to encode and extract the PCI-specific components of the msr_isource
 * element of an msr_routing_t.
 */

#define MSR_ENCODE_PCI_ISOURCE(bus, dev, pin) (((bus)<<7) | ((dev)<<2) | (pin))
#define MSR_ISOURCE_PCI_PIN(isource)    ((isource) & 0x3)
#define MSR_ISOURCE_PCI_DEV(isource)    (((isource) >> 2) & 0x1F)
#define MSR_ISOURCE_PCI_BUS(isource)    (((isource) >> 7) & 0xFF)


typedef struct msr_routing {
	unsigned long	msr_isource;
	ms_islot_t	msr_islot;
} msr_routing_t;

typedef struct {
        ms_cpu_t        msr_cpuid;
	unsigned long	msr_clockspeed;
} msr_cpu_t;

typedef struct {
	ms_memsize_t	msr_size;
	ms_size_t	msr_line_size;
	unsigned long	msr_level;
	unsigned long	msr_associativity;
	ms_bool_t	msr_dma_coherent;
	ms_bool_t	msr_write_through;
	ms_bool_t	msr_far_only;
	ms_bool_t	msr_read_only;
} msr_cache_t;

typedef struct {
	ms_paddr_t	msr_address;
	ms_memsize_t	msr_size;
        unsigned        msr_flags;
} msr_memory_t;
#define MSR_RESERVED (1 << 0)

typedef unsigned int	msr_bus_type_t;

typedef struct {
	msr_bus_type_t	msr_bus_type;
	unsigned long	msr_bus_number;
	msr_routing_t  *msr_intr_routing;
	unsigned int	msr_n_routing;
} msr_bus_t;
	
typedef struct {
	ms_cgid_t	msr_cgid;
} msr_cg_t;

typedef unsigned int	msr_type_t;

typedef struct {
	ms_cgnum_t	msr_cgnum;
	ms_bool_t	msr_private;
	ms_cpu_t	msr_private_cpu;
	msr_type_t	msr_type;
	union {
	  msr_cpu_t	msr_cpu;
	  msr_cache_t	msr_cache;
	  msr_memory_t	msr_memory;
	  msr_bus_t	msr_bus;
	  msr_cg_t	msr_cg;
	} msri;
} ms_resource_t;

typedef struct {
	unsigned int	mst_nresource;
	ms_resource_t  *mst_resources;
} ms_topology_t;

/*
 * Time related definitions
 */
typedef struct {
	unsigned long msrt_lo;
	unsigned long msrt_hi;
} ms_rawtime_t;

typedef struct {
	unsigned long mst_nsec;
	unsigned long mst_sec;
} ms_time_t;

typedef struct {
	unsigned long	dt_day;		/* day 0 = TBD ZZZ */
	unsigned long	dt_centisec;	/* 100ths of seconds into the day */
} ms_daytime_t;

typedef struct {
	unsigned short	xd_year;	/* full year (e.g. 1994) */
	unsigned char	xd_month;	/* 1 = Jan */
	unsigned char 	xd_day;		/* day in month, from 1 */
	unsigned char	xd_hour;	/* 0 - 23 */
	unsigned char	xd_min;		/* 0 - 59 */
	unsigned char	xd_sec;		/* 0 - 59 */
	unsigned char	xd_centisec;	/* 100ths of seconds into next second */
} ms_xdate_t;


/*
 * The following is used for interrupt management.
 */
typedef struct ms_intr_dist {
	ms_islot_t	msi_slot;	/* interrupt slot */
	unsigned int	msi_flags;	/* misc flags (see below) */
	unsigned int	msi_order;	/* masking order */
	ms_cpu_t	msi_cpu_dist;	/* CPU(s) interrupt can be sent to */
	void		*msi_mspec;	/* reserved for PSM use */
} ms_intr_dist_t;

/*
 * Flags used in msi_flags.
 */
#define	MSI_ITYPE_CONTINUOUS	1	/* continuous assertion mode (level triggered)*/
#define	MSI_MASK_ON_INTR	2	/* mask interrupt slot */
#define	MSI_NONMASKABLE		4	/* cannot mask interrupt slot (set/clear by PSM
					   on ms_intr_attach) */
#define	MSI_EVENTS		8	/* events can be posted by delivery function*/
#define MSI_ORDERED_COMPLETES  16	/* complete ints in LIFO order (set/clear by PSM 
					   on ms_intr_attach) */


/*
 * Miscellaneous  defines
 */
#define	MS_TRUE			(1)
#define	MS_FALSE		(0)
#define	MS_CPU_ANY		(-1)
#define	MS_CPU_ALL_BUT_ME	(-2)

/*
 * The action argument of MSOP_SHUTDOWN can take one of the following values.
 * Only values which the PSM previously included as one of the bits set in
 * MSPARAM_SHUTDOWN_CAPS capabilities bitmask will be used. If the PSM set
 * the bit, it must be capable of performing the required action.
 */ 
#define	MS_SD_HALT		(0x01)
#define	MS_SD_POWEROFF		(0x02)
#define	MS_SD_AUTOBOOT		(0x04)
#define	MS_SD_BOOTPROMPT	(0x08)

/*
 * The following is used to register MSOP functions.
 */

typedef unsigned int	msop_t;

typedef struct {
	msop_t	msop_op;	/* MSOP request number */
	void	*msop_func;	/* PSM function to call for processing MSOP */
} msop_func_t;


/*
 * Machine-specific operations opcodes. These values are used for
 * registering MSOPS supported by the PSM.
 * They are also used for indexing into function pointer arrays.
 */

#define	MSOP_INIT_CPU		1
#define MSOP_TICK_2			2

#define	MSOP_INTR_ATTACH	3
#define	MSOP_INTR_DETACH	4
#define	MSOP_INTR_MASK		5
#define	MSOP_INTR_UNMASK	6
#define	MSOP_INTR_COMPLETE	7

#define	MSOP_INTR_TASKPRI	8

#define	MSOP_XPOST			9

#define	MSOP_TIME_GET		10
#define	MSOP_TIME_ADD		11
#define	MSOP_TIME_SUB		12
#define	MSOP_TIME_CVT		13
#define	MSOP_TIME_SPIN		14
#define	MSOP_RTODC			15
#define	MSOP_WTODC			16

#define	MSOP_IDLE_SELF		17
#define	MSOP_IDLE_EXIT		18
#define	MSOP_SHUTDOWN		19
#define	MSOP_OFFLINE_PREP	20
#define	MSOP_OFFLINE_SELF	21
#define	MSOP_START_CPU		22
#define	MSOP_SHOW_STATE		23

#define	MSOP_IO_READ_8		28
#define	MSOP_IO_READ_16		29
#define	MSOP_IO_READ_32		30
#define	MSOP_IO_WRITE_8		31
#define	MSOP_IO_WRITE_16	32
#define	MSOP_IO_WRITE_32	33

#define	MSOP_IO_REP_READ_8	34
#define	MSOP_IO_REP_READ_16	35
#define	MSOP_IO_REP_READ_32	36
#define	MSOP_IO_REP_WRITE_8	37
#define	MSOP_IO_REP_WRITE_16	38
#define	MSOP_IO_REP_WRITE_32	39

#define	MSOP_FARCOPY		40


#define	MSOP_NUMBER_OF_MSOPS	41	/* set to highest used + 1 */


void		ms_init_cpu(void);
void		ms_tick_2(ms_time_t);
	
ms_bool_t	ms_intr_attach(ms_intr_dist_t*);
void		ms_intr_detach(ms_intr_dist_t*);
void		ms_intr_mask(ms_intr_dist_t*);
void		ms_intr_unmask(ms_intr_dist_t*);
void		ms_intr_complete(ms_intr_dist_t*);

void		ms_intr_taskpri(unsigned int);

void		ms_xpost(ms_cpu_t, ms_event_t);

void		ms_time_get(ms_rawtime_t*);
void		ms_time_add(ms_rawtime_t*, ms_rawtime_t*);
void		ms_time_sub(ms_rawtime_t*, ms_rawtime_t*);
void		ms_time_cvt(ms_time_t*, ms_rawtime_t*);
void		ms_time_spin(unsigned int);
ms_bool_t	ms_rtodc(ms_daytime_t*);
ms_bool_t	ms_wtodc(ms_daytime_t*);

void		ms_idle_self(void);
void		ms_idle_exit(ms_cpu_t);
void		ms_shutdown(int);
void		ms_offline_prep(void);
void		ms_offline_self(void);
void		ms_start_cpu(ms_cpu_t, ms_paddr_t);
void		ms_show_state(void);

unsigned char	ms_io_read_8(ms_port_t);
unsigned short	ms_io_read_16(ms_port_t);
unsigned int	ms_io_read_32(ms_port_t);
void		ms_io_write_8(ms_port_t, unsigned char);
void		ms_io_write_16(ms_port_t, unsigned short);
void		ms_io_write_32(ms_port_t, unsigned int);

void		ms_io_rep_read_8(ms_port_t, unsigned char*, int);
void		ms_io_rep_read_16(ms_port_t, unsigned short*, int);
void		ms_io_rep_read_32(ms_port_t, unsigned int*, int);
void		ms_io_rep_write_8(ms_port_t, unsigned char*, int);
void		ms_io_rep_write_16(ms_port_t, unsigned short*, int);
void		ms_io_rep_write_32(ms_port_t, unsigned int*, int);

void		ms_farcopy(void*, void*, ms_size_t);


/*
 * The following variables are set during PSM initialization and are exported to the rest
 * of the kernel.
 */

extern ms_bool_t	os_soft_sysdump;	/* core kernel implements dumps */
extern ms_cpu_t		os_ncpus;		/* number of cpus physically present in the system */
extern ms_time_t	os_time_res;		/* resolution of free running clock */
extern ms_time_t	os_tick_1_res;		/* resolution of heartbeat clock 1 */
extern ms_time_t	os_tick_1_max;		/* maximum value of heartbeat clock 1 */
extern ms_time_t	os_tick_2_res;		/* resolution of heartbeat clock 2 */
extern ms_time_t	os_tick_2_max;		/* maximum value of heartbeat clock 2 */
extern ms_islot_t	os_islot_max;		/* maximum islot supported by PSM */
extern ms_cpu_t		os_cpu_max;		/* maximum cpu number supported by PSM  -
						   (NOT number of cpus) */
extern ms_cgnum_t	os_cgnum_max;		/* maximum CG number supported by PSM */
extern unsigned int	os_intr_order_max;	/* maximum interrupt order supported by PSM */
extern unsigned int	os_intr_taskpri_max;	/* maximum task priority supported by PSM */
extern ms_size_t	os_farcopy_min;		/* use ms_farcopy if block size exceeds this value */
extern unsigned int	os_shutdown_caps;	/* supported shutdown capabilities */
extern ms_topology_t	*os_topology_p;		/* pointer to topology structure */
extern char		*os_platform_name;	/* Name of platform. */
extern char		*os_hw_serial;		/* machine serial number */

/*
 * Machine-specific parameters for the (os_set_msparam) call.
 */ 

typedef unsigned int	msparam_t;

#define	MSPARAM_PLATFORM_NAME		1
#define	MSPARAM_SW_SYSDUMP		2
#define	MSPARAM_TIME_RES		3
#define	MSPARAM_TICK_1_RES		4
#define	MSPARAM_TICK_1_MAX		5
#define	MSPARAM_TICK_2_RES		6
#define	MSPARAM_TICK_2_MAX		7
#define	MSPARAM_ISLOT_MAX		8
#define	MSPARAM_INTR_ORDER_MAX		9
#define	MSPARAM_INTR_TASKPRI_MAX	10
#define	MSPARAM_SHUTDOWN_CAPS		11
#define MSPARAM_TOPOLOGY                12
#define MSPARAM_FARCOPY_MIN             15
#define MSPARAM_HW_SERIAL		16

/*
 * VARIABLES EXPORTED TO PSM
 */
 
/*
 * os_default_topology
 * Everything the kernel has discovered about system configuration
 * before initpsm is called.
 */
extern ms_topology_t *os_default_topology;
 
/*
 * os_this_cpu
 *	This variable is local to each cpu and contains the cpu
 *	number of the CPU.
 */
extern ms_cpu_t	os_this_cpu;


/*
 * os_this_cgnum
 *	This variable is local to each cpu group (or cpu) and
 *	contains the number of the cpu group to which the CPU belongs.
 */
extern ms_cgnum_t	os_this_cgnum;


/*
 * os_intr_dist_stray
 *	PSM interrupt delivery functions return this value if they
 *	cannot determine the cause of the interrupt and wish to have the
 *	interrupt treated as an error. The PSM will store the interrupt
 *	slot number in <msi_slot>.
 *	The base kernel will typically log this event.
 */
extern ms_intr_dist_t	*os_intr_dist_stray;


/*
 * os_intr_dist_nop
 *	PSM interrupt delivery functions may return this value if they
 *	determine no action is required in the base kernel to handle
 *	the interrupt.
 */
extern ms_intr_dist_t	*os_intr_dist_nop;


/*
 * os_page0
 *	Holds the virtual address that maps to physical page 0.
 */
extern void	*os_page0;


/*
 * os_tick_period
 *	Tick period for the TICK1 clock.
 */
extern ms_time_t	os_tick_period;



/*
 * PROCEDURES EXPORTED TO PSM
 */
 

/*
 * ms_boot_t
 * os_register_msops(msop_func_t *op)
 *	Called by psm_init routines to
 *	register a PSM function to be called for a specific MSOP.
 *
 * Calling/Exit State:
 *	Registers all valid MSOPs in the array.
 *	Returns 
 *		MS_TRUE  -  if all MSOP are valid. 
 *		MS_FALSE -  if an unsupported MSOP was found
 *	
 */
ms_bool_t
os_register_msops(msop_func_t*);



/*
 * void *
 * os_alloc(ms_size_t sz)
 *	Allocates at least <sz> bytes of memory (with no alignment
 *	guarantees or special properties).
 *
 * Calling/Exit State:
 *	Returns NULL on failure. This call will not wait for
 *	memory if it is not immediately available.
 *
 *	May be called from PSM load and unload code, or
 *	from the following MSOP entry points: 
 *		MSOP_xxx_INIT, MSOP_ONLINE, MSOP_INTR_ATTACH,
 *		MSOP_INTR_DETACH.
 *
 *	May not be called while holding a lock.
 */
void *os_alloc(ms_size_t);


/*
 * void *
 * os_alloc_pages(ms_size_t sz)
 *	Allocates page-aligned contiguous memory for enough
 *	whole pages to contain <sz> bytes.  
 *
 * Calling/Exit State:
 *	Returns NULL on failure. This call will not wait for
 *	memory if it is not immediately available.
 *
 *	May be called from PSM initialization and unload code, or
 *	from the following MSOP entry points:
 *	 	MSOP_xxx_INIT, MSOP_ONLINE, MSOP_INTR_ATTACH,
 *		MSOP_INTR_DETACH.
 */
void *os_alloc_pages(ms_size_t);


/*
 * void
 * os_free(void *addr, ms_size_t sz)
 *	Free memory (if possible) previously allocated by (*os_alloc) or
 *	(*os_alloc_pages).
 *
 * Calling/Exit State:
 *	None.
 */
void os_free(void*, ms_size_t);


/*
 * char *
 * os_get_option(const char*)
 *	Looks for a option parameter with the specified name and returns 
 *	its value as a string.
 *
 * Calling/Exit State:
 *	Returns NULL if the option is not found. 
 */
char *os_get_option(const char*);


/*
 * void
 * os_printf(const char*, ...)
 *	Print an informative message on the operator console.
 *	This is intended for debugging only.
 *
 * Calling/Exit State:
 *	None
 */
void os_printf(const char*, ...);


/*
 * void*
 * os_physmap(ms_paddr_t, ms_size_t)
 *	Allocates a virtual address for mapping a given range
 *	of physical addresses. On systems with caches, the mapping will
 *	bypass the cache.
 *
 * Calling/Exit State:
 *	Returns NULL on failure.
 */
void *os_physmap(ms_paddr_t, ms_size_t);


/*
 * void
 * os_physmap_free(void*, ms_size_t)
 *	Releases a mapping allocated by a previous call to os_physmap.
 *
 * Calling/Exit State:
 *	None
 */
void os_physmap_free(void*, ms_size_t);


/*
 * void
 * os_claim_vectors(ms_ivec_t basevec, ms_ivec_t nvec,
 *			ms_intr_dist_t *(*func)(ms_ivec_t vec))
 *	Claim a range of CPU interrupt vectors to be handled by the PSM.
 *	These would be the interrupt vectors numbers used by the interrupt
 *	controller circuitry.
 *
 * Calling/Exit State:
 *	None.
 *	May be called multiple times and for already-claimed vectors
 *	to change the mapping assigned to the vectors.
 *	Only affects the cpu on which the call is made.
 */
void
os_claim_vectors(ms_ivec_t, ms_ivec_t, ms_intr_dist_t *(*)(ms_ivec_t));


/*
 * void
 * os_unclaim_vectors(ms_ivec_t basevec, ms_ivec_t nvec)
 *	Release a range of CPU interrupt vectors privously claimed
 *	with os_claim_vectors.
 *	Affects ONLY the cpu on which it is called.
 *
 * Calling/Exit State:
 *	None.
 */
void
os_unclaim_vectors(ms_ivec_t, ms_ivec_t);


/*
 * void
 * os_post_events(ms_event_t events)
 *	Notify the OS of one or more special events which have occurred
 *	since the last time os_post_events was called. The indicated
 *	bitmask of events will be logically ORed into a bitmask of events
 *	the OS has yet to process for the current CPU.
 *
 * Calling/Exit State:
 *	Called with IE flag off and it remains off throughout the call.
 *
 *	The events must not include MS_EVENT_PSM_xxx.
 *
 * 	It is only called from the PSM interrupt delivery functions. This
 *	allows the OS to limit the places it checks for posted events.
 *	Only delivery functions that return os_intr_dist_nop or an
 *	ms_intr_dist_t with MSI_EVENTS flag set.
 */
void
os_post_events(ms_event_t);
	


/*
 * void
 * os_set_msparam(msparam_t msparam, void *valuep)
 *	Used to set values of OS parameters. Must be called during
 *	the PSM load phase to set any OS parameters that differ
 *	from default values. Values cannot be subsequently changed.
 *
 * Calling/Exit State:
 *	None.
 */
void
os_set_msparam(msparam_t, void*);


/*
 * unsigned int
 * os_count_himem_resources (void)
 * 	Return the number of the ms_resource_t entries describing ranges
 *	of the high memery (memory above 4GB).
 *
 * Calling/Exit State:
 * 	Called from pfxinitpsm()
 */
unsigned int
os_count_himem_resources(void);


/*
 * void
 * os_fill_himem_resources(ms_resource_t *)
 *	Fill all fields of ms_resource_t entries as appropriate for MSR_MEMORY.
 *	The caller (PSM) must allocate sufficient memory to be filled in
 *	advance. 
 * 
 * Calling/Exit State:
 * 	Called from pfxinitpsm()
 */
void
os_fill_himem_resources(ms_resource_t *);

/*
 * ms_lockp_t
 * os_mutex_alloc(void)
 *	Allocates and initializes a simple mutex.
 *
 * Calling/Exit State:
 *	None.
 */
ms_lockp_t
os_mutex_alloc(void);



/*
 * void
 * os_mutex_free(void)
 *	Fress a simple mutex previously allocated by os_mutex_alloc.
 *
 * Calling/Exit State:
 *	None.
 */
void
os_mutex_free(ms_lockp_t);



/*
 * ms_lockstate_t
 * os_mutex_lock(ms_lockp_t)
 *	Allocates and initializes a simple mutex.
 *
 * Calling/Exit State:
 *	None.
 */
ms_lockstate_t
os_mutex_lock(ms_lockp_t);



/*
 * ms_lockp_t
 * os_mutex_unlock(ms_lockp_t, void)
 *	Allocates and initializes a simple mutex.
 *
 * Calling/Exit State:
 *	None.
 */
void
os_mutex_unlock(ms_lockp_t, ms_lockstate_t);

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_PSM_H */
