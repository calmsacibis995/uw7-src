#ifndef _SYS_PACCESS_H
#define _SYS_PACCESS_H

#ident	"@(#)debugger:inc/i386/sys/paccess.h	1.1"

#if defined(__cplusplus)
extern "C" {
#endif
/* paccess() command codes */

#define P_RDUSER	128
#define P_WDUSER	129
#define P_RIUSER	130
#define P_WIUSER	131
#define P_RUREGS	132
#define P_WUREGS	133
#define P_RUFREGS	134
#define P_WUFREGS	135
#define P_RULDT		136
#define P_RUOFFS	137
#define P_RUDREGS	138
#define P_WUDREGS	139
#define P_RDSIZE	140

#define P_MINCMD	128
#define P_MAXCMD	140

#define P_VERSION	1	/* paccess version */
#define C_VERSION	2	/* core file version */

typedef long uoff;

/* HACK !! */
/* GEMINI compat: hardcode values from user.h - 
 * should have been in uoffsets and coreoffsets
 */
#define PSARGSZ		80
#define PSARG_OFF	0x1078

/* GEMINI compat: import values from region.h */
#define PF_WRITE	0x1
#define PF_READ		0x2
#define PF_EXEC		0x4

#define MAX_NO_SIGS	31

/* UNIX 3.2 uarea offsets  */

struct uoffsets {
	uoff u_info;		/* version */
	uoff u_uaddr;		/* kernel virtual address of uarea  */
	uoff u_ar0;		/* user register save area pointer */
	uoff u_fps;		/* floating point save area  */
	uoff u_fpemul;		/* separate emulator save area  */
	uoff u_fpvalid;		/* if floating point save is valid */
	uoff u_weitek;		/* per proc weitek flag */
	uoff u_weitek_reg;	/* weitek save area  */
	uoff u_debugreg;	/* debug register save area  */
	uoff u_ldt;		/* offset of ldt  */
	uoff u_ldtlimit;	/* size of ldt  */
	uoff u_tss;		/* 3.2 adb  */
	uoff u_sztss;		/* 3.2 adb  */
	uoff u_sigreturn;	/* user signal return */
	uoff u_signal;
	uoff u_sdata;		/* u_exdata.ux_datorg */
	uoff u_dsize;		/* data size */
	uoff u_ssize;		/* stack size */
	uoff u_tsize;		/* text size  */
	uoff u_sub;		/* stack upper  bound */
	uoff uvstack;		/* virtual address of stack top */
	uoff uvtext;		/* virtual address of text */
};


#define MAXIPCDATA 132

/*
 * See core(FP) for a description of a core file's layout.
 */

#define CORES_MAGIC		((1 << 2) | 1)	/* magic number */
#define CORES_UAREA		((2 << 2) | 1)	/* U-area */
#define CORES_PREGION		((3 << 2) | 0)	/* process data/stack/etc. */
#define CORES_PROC		((4 << 2) | 1)	/* struct proc */
#define CORES_ITIMER		((5 << 2) | 1)	/* interval timers */
#define CORES_SCOUTSNAME	((6 << 2) | 1)	/* struct scoutsname */
#define CORES_OFFSETS		((7 << 2) | 1)	/* struct coreoffsets */

#define COREMAGIC_INVALID	(0x66696566)	/* possibly bad core file */
#define COREMAGIC_NUMBER	(0x2e696564)	/* buried magic number */

/*
 * For C_VERSION > 1, chain of information blocks about
 * this core dump.
 */
struct coresecthead {
	uchar_t	cs_stype;	/* Type of section (CORES_xxx) */
	uchar_t	cs_hsize;	/* Size (bytes) of section header */
	ushort	cs_osize;	/* Size (bytes) of optional info */
	union {
	    struct {
		short	csxp_rflg; /* Associated pregion's p_flags */
		short	csxp_rtyp; /* Associated pregion's p_type */
	    } csx_preg;
	    ulong_t	csx_magic; /* COREMAGIC_xxx for OFFSETS, MAGIC */
	} cs_x;
	ulong_t	cs_vaddr;	/* Virtual address of contents */
	ulong_t	cs_vsize;	/* Size (bytes) of contents */
	long	cs_sseek;	/* Offset of this section's contents */
	long	cs_hseek;	/* Offset of next section's header */
};

/*
 * Offset information structure at end of core file;
 * for C_VERSION > 1, preceded by the offset of the
 * first coresecthead (CORES_OFFSETS).
 */
struct coreoffsets {
	uoff u_info;		/* interface version (C_VERSION) */
	uoff u_user;		/* offset of upage in core file */
	uoff u_ldt;		/* offset of ldt in core file  */
	uoff u_data;		/* offset of data in core file */
	uoff u_stack;		/* offset of stack in core file */
	uoff u_usize;		/* size of uarea */
	uoff u_uaddr;		/* kernel virtual address of uarea  */
	uoff u_ar0;		/* user register save area pointer */
	uoff u_fps;		/* floating point save area  */
	uoff u_fper;		/* 2.3 field */
	uoff u_fpemul;		/* separate emulator save area  */
	uoff u_fpvalid;		/* valid save fpstate */
	uoff u_weitek;		/* flag indicating weitek usage */
	uoff u_weitek_reg;	/* weitek save area */
	uoff u_ssize;		/* stack size */
	uoff u_dsize;		/* data size  */
	uoff u_tsize;		/* text size  */
	uoff u_comm;		/* name of executable */
	uoff u_ldtlimit;	/* size of ldt   */
	uoff u_sub;		/* stack upper  bound */
	uoff u_sdata;		/* 3.2 u_exdata.ux_datorg  */
	uoff u_mag;		/* u_exdata.ux_mag  */
	uoff u_sigreturn;
	uoff u_signal;
	uoff uvstack;		/* virtual address of stack top */
	uoff uvtext;		/* virtual address of text */
	uoff size;		/* size of this structure;
				   WARNING: this must remain as the last
				   field; all additions must go above */
};

/*
 *	Template for accessing i386 debug registers via P_RUGREGS and
 *	P_WUDREGS
 */

struct debugregs {
	char *d_dr0;	/* breakpoint 0, virtual address */
	char *d_dr1;	/* breakpoint 1, virtual address */
	char *d_dr2;	/* breakpoint 2, virtual address */
	char *d_dr3;	/* breakpoint 3, virtual address */
	int d_res1;		/* reserved; undefined */
	int d_res2;
	unsigned int d_status;  /* status after breakpoint trap */
	unsigned int d_l0:2;	/* local enable */
	unsigned int d_l1:2;
	unsigned int d_l2:2;
	unsigned int d_l3:2;
	unsigned int d_le:2;	/* local "slowdown" */
	unsigned int res3:6;
	unsigned int d_rw0:2;	/* breakpoint type */
	unsigned int d_len0:2;	/* data size; 0 for text */
	unsigned int d_rw1:2;
	unsigned int d_len1:2;
	unsigned int d_rw2:2;
	unsigned int d_len2:2;
	unsigned int d_rw3:2;
	unsigned int d_len3:2;
};


/* Kernel data structure template shared by ptrace() and paccess() */

struct ipcbuf {
	int	ip_lock;
	int	ip_req;
	int	*ip_addr;
	int	ip_data;
};

#if defined(__cplusplus)
}
#endif

#endif	/* _SYS_PACCESS_H */
