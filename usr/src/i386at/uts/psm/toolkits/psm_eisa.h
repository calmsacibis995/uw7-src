#ident	"@(#)kern-i386at:psm/toolkits/psm_eisa.h	1.1"
#ident	"$Header$"

#ifndef _PSM_EISA_H			/* wrapper symbol for kernel use */
#define _PSM_EISA_H			/* subject to change without notice */

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_PSM)
#if _PSM != 2
#error "Unsupported PSM version"
#endif
#else
#error "Header file not valid for Core OS"
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


/* Mapping between eisa.h and psm_eisa.h, taken from types.h */
typedef	unsigned char	uchar_t;
typedef	unsigned short	ushort_t;
typedef	unsigned long	ulong_t;
typedef	unsigned int	uint_t;
typedef	ulong_t		paddr_t;
typedef	uint_t		size_t;

/* Taken from regset.h */
typedef struct regs {
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

	unsigned int eflags;

} regs;

/*	Copyright (c) 1988, 1989 Intel Corp.		*/
/*	  All Rights Reserved  	*/
/*
 *	INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *	This software is supplied under the terms of a license
 *	agreement or nondisclosure agreement with Intel Corpo-
 *	ration and may not be copied or disclosed except in
 *	accordance with the terms of that agreement.
 */


#define EISA_CFG0	0xc80	/* EISA configuration port 0 */
#define EISA_CFG1	0xc81	/* EISA configuration port 1 */
#define EISA_CFG2	0xc82	/* EISA configuration port 2 */
#define EISA_CFG3	0xc83	/* EISA configuration port 3 */

#define ELCR_PORT0	0x4d0	/* Edge/level trigger control register 0 */
#define ELCR_PORT1	0x4d1	/* Edge/level trigger control register 1 */

#define EDGE_TRIG	0	/* interrupt is edge triggered */
#define LEVEL_TRIG	1	/* interrupt is level triggered */

/*
 * EISA board identification ports. z is the slot number.
 */
#define EISA_ID0(z)	(z * 0x1000 + EISA_CFG0)
#define EISA_ID1(z)	(z * 0x1000 + EISA_CFG1)
#define EISA_ID2(z)	(z * 0x1000 + EISA_CFG2)
#define EISA_ID3(z)	(z * 0x1000 + EISA_CFG3)

#define EISA_READ_SLOT		0xd880          /* reg ax value */
#define EISA_READ_FUNC		0xd881          /* reg ax value */
#define EISA_CLEAR_NVM		0xd882          /* reg ax value */
#define EISA_WRITE_SLOT		0xd883          /* reg ax value */
#define EISA_BIOS_ADDRESS	0xff859
#define EISA_STRING_ADDRESS	0xfffd9
#define EISA_STRING		"EISA"
#define EISA_MAX_EXP_SLOTS	15
#define EISA_MAX_EMB_SLOTS	1
#define EISA_MAX_VIR_SLOTS	240
#define EISA_MAX_SLOTS		(EISA_MAX_EXP_SLOTS + EISA_MAX_EMB_SLOTS + \
				 EISA_MAX_VIR_SLOTS)
#define	EISA_BUFFER_SIZE	65536

/*
 * Set of data types to eisa_parse_devconfig.
 */
#define	EISA_SLOT_DATA		0x01		/* search all function blocks */
#define	EISA_FUNC_DATA		0x02		/* search one function block */

/*
 * Data search type that needs to be parsed.
 */
#define EISA_TYPE		0x01		/* search type string only */
#define EISA_SUBTYPE		0x02		/* search subtype string only */

#ifdef _KERNEL

struct regs;

/*
 * EISA bus initialization and verification routines.
 */
extern int	eisa_verify(void);

/*
 * Configuration Space (NVRAM) access routines.
 */
extern char	*eisa_uncompress(char *);
extern int	eisa_boardid(int, char *);
extern int	eisa_sysbrdid(uint_t *);
extern int	eisa_clear_nvm(unsigned char, unsigned char);
extern int	eisa_write_slot(int, char *);
extern int	eisa_read_slot(int, char *);
extern int	eisa_read_func(int, int, char *);
extern int	eisa_rom_call(struct regs *);
extern int	eisa_parse_devconfig(void *, void *, uint_t, uint_t);
extern int	eisa_read_nvm(int, uchar_t *, int *);

/*
 * Extended NMI support routines.
 *
 * Fail-Safe sanity timer support routines.
 */
extern void	eisa_sanity_init(void);
extern int	eisa_sanity_check(void);
extern void	eisa_sanity_halt(void);

extern int	eisa_nmi_bus_timeout(void);

/*
 * Edge/Level sensitivity interrupt control routines.
 */
extern void	eisa_set_elt(int, int);

#endif /* _KERNEL */

/*
 *   (C) Copyright COMPAQ Computer Corporation 1985, 1989
 */

#pragma pack(1)

/*
 * Size of each function configuration block.
 */
#define EISA_MAX_SELECTIONS	26
#define EISA_MAX_TYPE		80
#define EISA_MAX_MEMORY		9
#define EISA_MAX_IRQ		7
#define EISA_MAX_DMA		4
#define EISA_MAX_PORT		20
#define EISA_MAX_INIT		60
#define EISA_MAX_DATA		203

/*
 * Returned information from an INT 15h call "Read Slot" (AH=D8h, AL=0). 
 */
typedef struct eisa_slotinfo {
	uchar_t		boardid[4];	/* 0x00:Compressed board ID */
	ushort_t	revision;	/* 0x04:Utility version */
	uchar_t		functions;	/* 0x06:Number of functions */

	struct {
		uchar_t	type	:1,	/* Type string present		*/
			memory	:1,	/* Memory configuration present */
			irq	:1,	/* IRQ configuration present	*/
			dma	:1,	/* DMA configuration present	*/
			port	:1,	/* Port configuration present	*/
			init	:1,	/* Port initialization present	*/
			data	:1,	/* Free form data		*/
			disable :1;	/* Function is disabled 	*/
	} fib;				/* 0x07: Function information byte */

	ushort_t	checksum;	/* 0x08:CFG checksum */

	struct {
		ushort_t cpid	:4,	/* Duplicate ID number */
			type	:2,	/* Slot type */
			readid	:1,	/* Readable ID */
			dups	:1,	/* Duplicates exist */
			disable :1,	/* Board disable is supported */
			IOcheck :1,	/* EISA I/O check supported */
				:5,	/* Reserved */
			partial :1;	/* Configuration incomplete */
	} dupid;			/* 0x0A:Duplicate ID information */

} eisa_slotinfo_t;

#define	EISA_SLOTINFO_SIZE	(sizeof(eisa_slotinfo_t))

#define	EISA_EXP_SLOT		0x00	/* expansion slot */
#define	EISA_EMB_SLOT		0x01	/* embedded slot */
#define	EISA_VIR_SLOT		0x02	/* virtual slot */

/*
 * Standard Function Configuration Data Block Structure 
 * (size = 320 (0x140) bytes).
 *
 * Returned information from an INT 15h call "Read Function" (AH=D8h, AL=01h). 
 */
typedef struct eisa_funcinfo {

	uchar_t		boardid[4];	/* 0x0: Compressed board ID */

	/*
	 * Duplicate ID information byte format. (total bytes = 2, offset = 0x4)
	 */
	struct {
	    ushort_t	cpid	:4,	/* Duplicate ID number */
			type	:2,	/* Slot type */
			readid	:1,	/* Readable ID */
			dups	:1,	/* Duplicates exist */
			disable :1,	/* Board disable is supported */
			IOcheck :1,	/* EISA I/O check supported */
				:5,	/* Reserved */
			partial :1;	/* Configuration incomplete */
	} dupid;			/* 0x04: Duplicate ID information */

	uchar_t		ovl_minor;	/* 0x06: Minor revision of .OVL code */
	uchar_t		ovl_major;	/* 0x07: Major revision of .OVL code */
	uchar_t		selects[EISA_MAX_SELECTIONS]; /* 0x08: Selections */

	/*
	 * Function information byte. (total bytes = 1, offset = 0x22)
	 */
	struct {
	    uchar_t	type	:1,	/* Type string present		*/
			memory	:1,	/* Memory configuration present */
			irq	:1,	/* IRQ configuration present	*/
			dma	:1,	/* DMA configuration present	*/
			port	:1,	/* Port configuration present	*/
			init	:1,	/* Port initialization present	*/
			data	:1,	/* Free form data		*/
			disable :1;	/* Function is disabled 	*/
	} fib;				/* 0x22:Function information byte */

	uchar_t		type[EISA_MAX_TYPE]; /* 0x23: Function type/subtype */

	/*
	 * Function block configuration resource data definition.
	 */
	union {

	  /*
	   * Configuration resource data definition.
	   */
	  struct {

	    /*
	     * Memory Configuration Info. (total bytes = 63, offset = 0x73)
	     */
	    struct {
		struct {
		    uchar_t	write	:1,	/* Memory is read only */
				cache	:1,	/* Memory is cached */
					:1,	/* Reserved */
				type	:2,	/* Memory type */
				share	:1,	/* Shared Memory */
					:1,	/* Reserved */
				more	:1;	/* More entries follow */
		} config;
		struct {
		    uchar_t	width	:2,	/* Data path size */
				decode	:2;	/* Address decode */
		} datapath;
		uchar_t		start[3];	/* Start addr DIV 100h */
		ushort_t	size;		/* Mem size in 1K bytes */
	    } memory[EISA_MAX_MEMORY];		/* 0x73: Memory conf. */

	    /*
	     * IRQ Configuration Info. (total bytes = 14, offset = 0x82)
	     */
	    struct {
		ushort_t	line	:4,	/* IRQ line */
					:1,	/* Reserved */
				trigger :1,	/* Trigger (EGDE=0, LEVEL=1) */
				share	:1,	/* Sharable */
				more	:1,	/* More follow */
					:8;	/* Reserved */
	    } irq[EISA_MAX_IRQ];		/* 0xb2: IRQ conf. */

	    /*
	     * DMA Channel Description. (total bytes = 8, offset = 0xc0)
	     */
	    struct {
		ushort_t	channel :3,	/* DMA channel number */
					:3,	/* Reserved */
				share	:1,	/* Shareable */
				more	:1,	/* More entries follow */
					:2,	/* Reserved */
				width	:2,	/* Transfer size */
				timing	:2,	/* Transfer timing */
					:2;	/* Reserved */
	    } dma[EISA_MAX_DMA];		/* 0xc0: DMA conf. */

	    /*
	     * I/O port info. (total bytes = 60, offset = 0xc8)
	     */
	    struct {
		uchar_t		count	:5,	/* No.of sequential ports - 1 */
					:1,	/* Reserved */
				share	:1,	/* Shareable */
				more	:1;	/* More entries follow */
		ushort_t	address;	/* IO port address */
	    } port[EISA_MAX_PORT];		/* 0xc8: PORT conf. */

	    /*
	     * Initialization Data (total bytes = 60, offset = 0x104)
	     */
	    union {
	      struct {
		 uchar_t	type	:2,	/* Port type */
				mask	:1,	/* Apply mask */
					:4,	/* Reserved */
				more	:1;	/* More entries follow */
		 ushort_t	port;		/* Port address */
		 union {
		    struct {
			uchar_t	value;		/* Byte to write */
		    } bv;

		    struct {
			uchar_t	value;		/* Byte to write */
			uchar_t	mask;		/* Mask to apply */
		    } bvm;

		    struct {
			ushort_t value;		/* Word to write */
		    } wv;

		    struct {
			ushort_t value;		/* Word to write */
			ushort_t mask;		/* Mask to apply */
		    } wvm;

		    struct {
			ulong_t	value;		/* Dword to write */
		    } dv;

		    struct {
			ulong_t	value;		/* Dword to write */
			ulong_t	mask;		/* Mask to apply */
		    } dvm;
		  } vm;
	      } initform[5];			/* EISA_MAX_INIT / 11 */

	      uchar_t		initdata[EISA_MAX_INIT];

	    } init;				/* 0x104: INIT info. */

	  } rd;					/* Resource Data */

	  uchar_t		freeform[EISA_MAX_DATA + 1];
						/* 0x73: Free format */

	} fd;					/* Function Data */

} eisa_funcinfo_t;

/*
 * EISA Function Information aliases.
 */
#define	eisa_memory		fd.rd.memory
#define	eisa_irq		fd.rd.irq
#define	eisa_dma		fd.rd.dma
#define	eisa_port		fd.rd.port
#define	eisa_init		fd.rd.init
#define	eisa_freeform		fd.freeform

#define	EISA_FUNCINFO_SIZE	(sizeof(eisa_funcinfo_t))

#pragma pack()

#ifdef _KERNEL

typedef struct eisa_info {
	eisa_slotinfo_t	eslotinfo;	/* slot information */
	uint_t		efuncs;		/* number of functions */
	uint_t		estatus;	/* error code */
	paddr_t		eaddr;		/* base physical address of NVM data */
	size_t		esize;		/* size of the NVM data  */
} eisa_info_t;

#define	EISA_INFO_SIZE		(sizeof(eisa_info_t))

/*
 * The EISA function return values from the ROM BIOS calls.
 */
#define EISA_SUCCESS		0x00	/* No errors */
#define EISA_INVALID_SLOT	0x80	/* Invalid slot number */
#define EISA_INVALID_FUNC	0x81	/* Invalid function number */
#define EISA_CORRUPT_NVRAM	0x82	/* Nonvolatile memory corrupt */
#define EISA_EMPTY_SLOT		0x83	/* Slot is empty */
#define EISA_WRITE_ERROR	0x84	/* Failure to write to CMOS */
#define EISA_NVRAM_FULL		0x85	/* CMOS memory is full */
#define EISA_UNSUPPORTED	0x86	/* EISA CMOS not supported */
#define EISA_INVALID_SETUP	0x87	/* Invalid Setup information */
#define EISA_INVALID_VERSION	0x88	/* BIOS cannot support this version */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PSM_EISA_H */
