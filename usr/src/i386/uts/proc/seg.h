#ifndef _PROC_SEG_H	/* wrapper symbol for kernel use */
#define _PROC_SEG_H	/* subject to change without notice */

#ident	"@(#)kern-i386:proc/seg.h	1.18.5.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */


#if defined _KERNEL || defined _KMEMUSER

/*
 * Intel386(tm) hardware segment descriptor format.
 */

struct segment_desc {
ushort_t	sd_limit_low,		/* limit bits 15-0 */
		sd_base_low;		/* base bits 15-0 */
uchar_t		sd_base_mid,		/* base bits 23-16 */
		sd_type_etc,		/* various type stuff (P,DPL,type) */
		sd_limit_high,		/* limit bits 19-16 (plus G,B) */
		sd_base_high;		/* base bits 31-24 */
};

#define	SD_LIMIT_LOW(l)		 (((uint_t)(l) - 1) & 0x0000FFFF)
#define	SD_LIMIT_HIGH(l)	((((uint_t)(l) - 1) & 0x000F0000) >> 16)

#define	SD_BASE_LOW(b)		 ((uint_t)(b) & 0x0000FFFF)
#define	SD_BASE_MID(b)		(((uint_t)(b) & 0x00FF0000) >> 16)
#define	SD_BASE_HIGH(b)		(((uint_t)(b) & 0xFF000000) >> 24)

#define	BUILD_MEM_DESC(desc,base,limit,acc1,acc2)	{ \
		(desc)->sd_base_low = SD_BASE_LOW(base); \
		(desc)->sd_base_mid = SD_BASE_MID(base); \
		(desc)->sd_base_high = SD_BASE_HIGH(base); \
		(desc)->sd_limit_low = SD_LIMIT_LOW(limit); \
		(desc)->sd_limit_high = SD_LIMIT_HIGH(limit) | ((acc2) << 4); \
		(desc)->sd_type_etc = (acc1); \
	}

#define	BUILD_SYS_DESC(desc,base,limit,acc1,acc2)	{ \
		(desc)->sd_base_low = SD_BASE_LOW(base); \
		(desc)->sd_base_mid = SD_BASE_MID(base); \
		(desc)->sd_base_high = SD_BASE_HIGH(base); \
		(desc)->sd_limit_low = SD_LIMIT_LOW(limit); \
		(desc)->sd_limit_high = SD_LIMIT_HIGH(limit) | ((acc2) << 4); \
		(desc)->sd_type_etc = (acc1); \
	}

#define SD_GET_LIMIT(sd)	\
		((sd)->sd_limit_low + (((sd)->sd_limit_high & 0xF) << 16))

#define SD_GET_BASE(sd)		\
		((sd)->sd_base_low + ((sd)->sd_base_mid << 16) + \
		 ((sd)->sd_base_high << 24))

#define SD_GET_ACC1(sd)		\
		((sd)->sd_type_etc)

#define SD_GET_ACC2(sd)		\
		((sd)->sd_limit_high >> 4)

#define setdscrbase(desc, base) { \
		(desc)->sd_base_low = SD_BASE_LOW(base); \
		(desc)->sd_base_mid = SD_BASE_MID(base); \
		(desc)->sd_base_high = SD_BASE_HIGH(base); \
	}

#define setdscrlim(desc, limit) { \
		(desc)->sd_limit_low = SD_LIMIT_LOW(limit); \
		(desc)->sd_limit_high = SD_LIMIT_HIGH(limit) | \
					 ((desc)->sd_limit_high & 0xF0); \
	}

#define setdscracc1(desc, acc1) { \
		(desc)->sd_type_etc = (acc1); \
	}

#define setdscracc2(desc, acc2) { \
		(desc)->sd_limit_high = ((desc)->sd_limit_high & 0x0F) | \
					 ((acc2) << 4); \
	}

#define	SD_MAX_SEG	(0x100000)	/* in HW pages */

#endif /* _KERNEL || _KMEMUSER */

/* Access rights and type codes for segments */ 

#define UDATA_ACC1	0xF2 	/* present dpl=3 writable */
#define URODATA_ACC1	0xF0 	/* present dpl=3 read-only */
#define KDATA_ACC1	0x92	/* present dpl=0 writable */
#define DATA_ACC2	0xC	/* 4Kbyte gran. 4Gb limit avl=0 */
#define DATA_ACC2_S	0x4	/* 1 byte gran., 32bit operands, avl=0 */
#define UTEXT_ACC1	0xFA 	/* present dpl=3 readable */
#define KTEXT_ACC1	0x9A 	/* present dpl=0 readable */
#define TEXT_ACC2	0xC	/* 4Kbyte gran., 32 bit operands avl=0 */
#define TEXT_ACC2_S	0x4	/* 1 byte gran., 32 bit operands avl=0 */
#define LDT_UACC1	0xE2	/* present dpl=3 type=ldt */
#define LDT_KACC1	0x82	/* present dpl=0 type=ldt */
#define LDT_ACC2	0x0	/* G=0 avl=0*/
#define TSS3_KACC1	0x89 	/* present dpl=0 type=available 386 TSS */
#define TSS3_KBACC1     0x8B    /* present dpl=0 type=busy 386 TSS      */
#define	TSS2_KACC1	0x81 	/* present dpl=0 type=available 286 TSS */
#define TSS3_UACC1	0xE9 	/* present dpl=3 type=available 386 TSS */
#define TGATE_UACC1     0xE5    /* present dpl=3 type=task gate         */
#define	TSS2_UACC1	0xE1 	/* present dpl=3 type=available 286 TSS */
#define TSS_ACC2	0x0	/* G=0 avl=0 */

/* Bit fields in ACC1 of segment descriptor */
#define SEG_READABLE	0x02U	/* readable (for code segments) */
#define SEG_WRITEABLE	0x02U	/* writeable (for data segments) */
#define SEG_CONFORM	0x04U	/* conforming (for code segments) */
#define SEG_CODE	0x08U	/* code segment */
#define SEG_DPL		0x60U	/* descriptor privilege level */
#define SEG_PRESENT	0x80U	/* present */

#define DPL_SHIFT	5U

/* Bit fields in ACC2 of segment descriptor */
#define SEG_SIZE32	0x4	/* 32-bit code segment */
#define	GRANBIT		0x8	/* Limit Granularity = 4k, not byte */
#define	BIGSTACK	0x4	/* If implicit SS, use ESP instead of SP */

/* Masks to get at and definitions of the requested privilege levels (RPL). */

#define	KERNEL_RPL	0x0	/* Kernel RPL for selector */
#define	USER_RPL	0x3	/* User RPL for selector */
#define	RPL_MASK	0x3	/* mask for RPL value */

#ifdef V86MODE
#define USERMODE(cs, efl) \
	((((efl) & PS_VM) && (l.special_lwp & SPECF_VM86)) || \
	 (((cs) & RPL_MASK) != KERNEL_RPL))
#endif				

#if defined _KERNEL || defined _KMEMUSER

#define	seltoi(sel)	((ushort_t)(sel) >> 3)
#define SEL_LDT		4	/* mask to determine if sel is GDT or LDT */

/*
 * Call/Interrupt/Trap Gate descriptor format.
 */

struct gate_desc {
ushort_t	gd_offset_low,		/* offset bits 15-0 */
		gd_selector;		/* selector */
uchar_t		gd_arg_count,		/* argument count */
		gd_type_etc;		/* various type stuff (P,DPL,type) */
ushort_t	gd_offset_high;		/* offset bits 31-16 */
};

#define	GD_OFFSET_LOW(o)	 ((uint_t)(o) & 0x0000FFFF)
#define	GD_OFFSET_HIGH(o)	(((uint_t)(o) & 0xFFFF0000) >> 16)

#define	BUILD_GATE_DESC(desc,sel,offset,type,priv,wc)	{ \
		(desc)->gd_selector = sel; \
		(desc)->gd_offset_low = GD_OFFSET_LOW(offset); \
		(desc)->gd_offset_high = GD_OFFSET_HIGH(offset); \
		(desc)->gd_arg_count = wc; \
		(desc)->gd_type_etc = (priv) | (type); \
	}

#endif /* _KERNEL || _KMEMUSER */
 
/* Access rights field for gates */

#define GATE_UACC	0xE0		/* present and dpl = 3 */
#define GATE_KACC	0x80		/* present and dpl = 0 */
#define GATE_386CALL	0xC		/* 386 call gate */
#define GATE_386INT	0xE		/* 386 int gate */
#define GATE_386TRP	0xF		/* 386 trap gate */
#define GATE_TSS	0x5		/* Task gate */

#if defined _KERNEL || defined _KMEMUSER

/*
 * GDT and IDT registers are loaded from a desctab structure.
 */

struct desctab {
	ushort_t	dt_limit;	/* table limit (max offset in bytes) */
	ushort_t	dt_baselow;	/* low 16-bits of table base */
	ushort_t	dt_basehigh;	/* high 16-bits of table base */
};

#define	DT_BASE_LOW(b)		 ((uint_t)(b) & 0x0000FFFF)
#define	DT_BASE_HIGH(b)		(((uint_t)(b) & 0xFFFF0000) >> 16)

#define	BUILD_TABLE_DESC(desc, tab, nent) { \
		(desc)->dt_limit = (nent) * sizeof(struct segment_desc) - 1; \
		(desc)->dt_baselow = DT_BASE_LOW(tab); \
		(desc)->dt_basehigh = DT_BASE_HIGH(tab); \
	}

#define DT_GET_LIMIT(dt)	\
		((uint_t)(dt)->dt_limit)

#define DT_GET_BASE(dt)		\
		((dt)->dt_baselow + ((uint_t)(dt)->dt_basehigh << 16))

/*
 * Kernel segment selectors
 */
/* Selectors 0x08 - 0xF8 reserved for MERGE386 */
#define KCSSEL		0x100	/* kernel code segment selector */
#define KDSSEL		0x108	/* kernel data segment selector */
#define KTSSSEL		0x110	/* global (per-engine) TSS selector */
#define KLDTSEL		0x118	/* selector for descriptor to access LDT */
#define KPRIVTSSSEL	0x120	/* private (per-process) TSS selector */
#define FPESEL		0x12B	/* FP emulator code segment */
#define DFTSSSEL	0x130	/* double-fault TSS selector */
#define KPRIVLDTSEL	0x138	/* private (per-process) LDT selector */
/* Selector 0x2C8 reserved for i286emul */

/* the range of selectors [MIN...MAX) reserved for kernel use */
#define KERNSEG_MIN	0x100
#define KERNSEG_MAX	0x140

#define GDTSZ		90	/* number of entries */
#define IDTSZ		256	/* number of entries */

#endif /* _KERNEL || _KMEMUSER */

/*
 * User segment selectors
 */
#define	USER_SCALL	0x07	/* call gate for system calls */
#define	USER_SIGCALL	0x0F	/* call gate for sigreturn */
#define	USER_CS		0x17	/* user's code segment */
#define	USER_DS		0x1F	/* user's data segment */
#define USER_FPSTK	0x27	/* alias 32 bit stack selector for emulator */
#define USER_FP		0x2F	/* selector for fp emulator to u area */
#define CSALIAS_SEL	0x37	/* CS alias selector for USER_DS (for XENIX */
				/* 	execseg()/unexecseg() system calls). */
#define UVWINSEL	0x3F	/* window into selected kernel data at uvwin */

#if defined _KERNEL || defined _KMEMUSER

/*
 * Processes which do not require a private LDT (normal processes) share
 * a global standard LDT at "global_ldt".  This LDT contains entries to map
 * the user code and data segments and gates for a system call and the
 * signal handler return call.
 *
 * Processes which do require a private LDT (e.g. 286 binaries) allocate
 * per-LWP LDTs.  Private LDTs grow as necessary.
 *
 * The first STDLDTSZ LDT entries are considered "standard."  The user may
 * change them, but the kernel will reset them to standard values if it
 * delivers a signal to the LWP, so that the signal handler may be invoked
 * correctly.
 */
#define LDTSZ		9	/* size of global default LDT */
#define STDLDTSZ	4	/* # "standard" LDT entries */

/*
 * Types of descriptor tables.  For set_private_dt().
 */
#define DT_GDT	0
#define DT_LDT	1
#define NDESCTAB	2

struct desctab_info {
	struct segment_desc *di_table; /* pointer to GDT or LDT */
	size_t di_size;		       /* size, in bytes, of di_table */
	boolean_t di_installed;	       /* externally-installed table */
	struct desctab_info *di_link;  /* link to saved desctab_info */
};

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

/*
 * Segment descriptor values for idt[NOEXTFLT] with floating-point
 * enabled, and disabled, respectively.
 */
extern struct gate_desc fpuon_noextflt, fpuoff_noextflt;

/*
 * Global standard LDT.  Normal processes use this.
 */
extern struct segment_desc *myglobal_ldt;

/*
 * Descriptor table info for global standard descriptor tables.
 * Normal processes use these.
 */
extern struct desctab_info myglobal_dt_info[NDESCTAB];

/*
 * Descriptor for standard IDT.
 * Normal processes use these.
 */
extern struct desctab mystd_idt_desc;

/*
 * Pointer to current IDT per-engine.
 */
extern struct gate_desc *cur_idtp;

/*
 * Function declarations.
 */
extern int set_dt_entry(ushort_t sel, const struct segment_desc *sdp);
extern int set_private_dt(uint_t dt, struct segment_desc *table, size_t size);
extern void set_private_idt(struct gate_desc *idtp);
extern void restore_std_idt(void);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_SEG_H */
