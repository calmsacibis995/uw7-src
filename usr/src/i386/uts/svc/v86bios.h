#ifndef _SVC_V86BIOS_H	/* wrapper symbol for kernel use */
#define _SVC_V86BIOS_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/v86bios.h	1.1.2.1"
#ident	"$Header$"

#ifdef _KERNEL

#define V86BIOS_PBASE		0x00000
#define V86BIOS_DS_SEG		0x00000
#define V86BIOS_ES_SEG		0x00000
#define V86BIOS_SS_SEG		0x00000
#define V86BIOS_PSTACK_BASE	MMU_PAGESIZE
#define V86BIOS_PDATA_BASE	(MMU_PAGESIZE/2)
#define V86BIOS_BUFFER_SIZE	(V86BIOS_PSTACK_BASE - V86BIOS_PDATA_BASE)

#define IOB_MAXPORT_MAP		0x2000 	/* 0 to 65535 */
#define V86MON_STACKSIZE	0x1000 	/* stack size of the v86 monitor */
#define KV86TSSSEL		0x148 	/* V86 TSS selector */
#define	V86INT			0x7f 	/* softint for returning from V86 mode */

/*
 * V86bios always uses the init plocal (l.) initialized by sysinit().
 * While running V86BIOS, the kernel does not set SPECF_VM86, which is used
 * by Merge processes.
 */
#define V86BIOS(efl) \
	(((efl) & PS_VM) && !(l.special_lwp & SPECF_VM86))

/* 
 * Memory needed to map for v86bios call for PC/AT
 */
#define V86BIOS_VIDEO		0xa0000
#define V86BIOS_SIZE		(0x10000 * 6) /* from 0xa0000 to 0xfffff */

/*
 * Privilege Level 0 stack frame after interrupt in V86 mode
 */
struct v86_pl0stack {
	unsigned int	v_eip;
	unsigned short	v_cs;
	unsigned short	v_padd0;
	unsigned int	v_eflags;
	unsigned int	v_esp;
	unsigned short	v_ss;
	unsigned short	v_padd1;
	unsigned short	v_es;
	unsigned short	v_padd2;
	unsigned short	v_ds;
	unsigned short	v_padd3;
	unsigned short	v_fs;
	unsigned short	v_padd4;
	unsigned short	v_gs;
	unsigned short	v_padd5;
};

typedef struct v86_pl0stack v86_pl0stack_t;

extern void 	t_v86(void);		/* defined in pstart.s */
extern void 	v86bios_leave(void);	/* defined in pstart.s */
extern int 	v86bios_chkopcode(unsigned int eip, volatile int type);

#endif /* _KERNEL */	
/*
 * Device name
 */
#define V86BIOS_DEVICE	"/dev/bios"
 
/*
 * BIOS INT code
 */
#define	PCI_BIOS_INT	0x1a
#define VIDEO_BIOS_INT	0x10
#define EISA_BIOS_INT	0x15

/* 
 * i8086 far pointer
 */
struct v86_farptr {
	unsigned short 	v86_off;
	unsigned short 	v86_cs;
};

typedef struct v86_farptr v86_farptr_t;

/*
 * commands for type
 */
#define	V86BIOS_INT16		0	/* for usual 16-bit registers */
#define	V86BIOS_INT32		1	/* for 32-bit registers */
#define	V86BIOS_CALL		2	/* for calling ROM routines */

#define V86BIOS_FUNCTYPE	0x0fff	/* mask for function type */
#define V86BIOS_COPYIN		0x1000	/* copyin from v86bios */
#define V86BIOS_COPYOUT		0x2000	/* copyout to v86bios */

/*
 * Structure for specifying the buffer(s) between v86bios and kernel.
 *
 * The user needs to set the V86BIOS_COPYIN/OUT flag in intrges.type:
 * Set V86BIOS_COPYOUT to provide data to v86bios for the call, if
 * necessary. V86BIOS_COPYIN is used to get data from v86bios as the
 * results. Therefore, a complex bios call needs to set both
 * V86BIOS_COPYIN and V86BIOS_COPYOUT. The address for the buffer(s) is
 * specified by the v86bios_buf_t sturcture. Although the buffer used by a
 * bios call is typically specified by a regsiter such as %esi, 
 * %edi, buf_v86addr needs to be set as such. In other words, both the
 * register and buf_v86addr have to be set consistently.
 *
 */
struct v86bios_buf {
	void		*v86addr; 	/* v86bios address space */
	void		*kaddr; 	/* kernel address space */
	size_t		size;		/* the length of the buffer */
};

typedef struct v86bios_buf v86bios_buf_t;

/*
 * Structure for calling a v86bios call
 */
struct intregs {
	unsigned int	type; 	 /* indicating how to call */
	int		retval;	 /* return value: carry bit */
	volatile unsigned int	done;	 /* internal use */
	int		error;	 /* flag indicating error */
	
	union {
		unsigned short	intval;
		v86_farptr_t 	farptr;
	} entry;

	union {
		unsigned int eax;
		struct {
			unsigned short ax;
		} word;
		struct {
			unsigned char al;
			unsigned char ah;
		} byte;
	} eax;

	union {
		unsigned int ebx;
		struct {
			unsigned short bx;
		} word;
		struct {
			unsigned char bl;
			unsigned char bh;
		} byte;
	} ebx;

	union {
		unsigned int ecx;
		struct {
			unsigned short cx;
		} word;
		struct {
			unsigned char cl;
			unsigned char ch;
		} byte;
	} ecx;

	union {
		unsigned int edx;
		struct {
			unsigned short dx;
		} word;
		struct {
			unsigned char dl;
			unsigned char dh;
		} byte;
	} edx;

	union {
		unsigned int edi;
		struct {
			unsigned short di;
		} word;
	} edi;

	union {
		unsigned int esi;
		struct {
			unsigned short si;
		} word;
	} esi;

	unsigned short	bp;
	unsigned short	es;
	unsigned short	ds;

	v86bios_buf_t	*buf_in;
	v86bios_buf_t	*buf_out;
	
#ifdef NOTYET
	unsigned short	ss;
	unsigned short	sp;
	unsigned int	oesp;
#endif /* NOTYET */
};

typedef struct intregs	intregs_t;

void v86bios(intregs_t *);

#endif /* _SVC_V86BIOS_H */




