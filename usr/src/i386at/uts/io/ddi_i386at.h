#ifndef _IO_DDI_I386AT_H	/* wrapper symbol for kernel use */
#define _IO_DDI_I386AT_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/ddi_i386at.h	1.5.4.3"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL

#if defined(_DDI) && _DDI - 0 >= 8

#error "Header file obsolete"

#endif /* _DDI >= 8 */

/* Parameter values for drv_gethardware() */
#define	PROC_INFO	0
#define	IOBUS_TYPE	1
#define	TOTAL_MEM	2
#define	DMA_SIZE	3
#define	BOOT_DEV	4
#define	HD_PARMS	5
#define EISA_BRDID	6

/* Structure argument for PROC_INFO */

struct cpuparms {
	ulong_t cpu_id;		/* CPU identification */
	ulong_t cpu_step;	/* Step (revision) number */
	ulong_t cpu_resrvd[2];	/* RESERVED for future expansion */
};

/* Legal values for cpu_id */

#define	CPU_UNK		0
#define	CPU_i386	1
#define	CPU_i486	2
#define	CPU_i586	3
#define	CPU_i686	4
#define	CPU_i786	5

/* Legal values for cpu_step */

#define STEP_UNK	0
#define STEP_B1		1

/* Return values for IOBUS_TYPE */

#define	BUS_ISA		0
#define	BUS_EISA	1
#define	BUS_MCA		2


/* Structure argument for BOOT_DEV */

struct bootdev {
	ulong_t	bdv_type;	/* Type of the boot device */
	ulong_t	bdv_unit;	/* Unit number of the boot device */
	ulong_t	bdv_resrvd[2];	/* RESERVED for future expansion */
};

/* Legal values for bdv_type */

#define BOOT_FLOPPY	1
#define BOOT_DISK	2
#define BOOT_CDROM	3
#define BOOT_NETWORK	4


/* Structure argument for HD_PARMS */

struct hdparms {
	ulong_t	hp_unit;	/* Hard disk unit number */
	ulong_t	hp_ncyls;	/* # of cylinders (0 == no disk) */
	ulong_t	hp_nheads;	/* # of heads */
	ulong_t	hp_nsects;	/* # of sectors per track */
	ushort_t hp_precomp;	/* write precomp cylinder */
	ushort_t hp_lz;		/* landing zone cylinder */
	ulong_t	hp_resrvd[2];	/* RESERVED for future expansion */
};

extern int drv_gethardware(ulong_t parameter, void *valuep);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_DDI_I386AT_H */
