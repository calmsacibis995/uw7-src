#ifndef _IO_AUTOCONF_CA_EISA_NVM_H	/* wrapper symbol for kernel use */
#define _IO_AUTOCONF_CA_EISA_NVM_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/autoconf/ca/eisa/nvm.h	1.3"
#ident	"$Header$"

/*
 *   (C) Copyright COMPAQ Computer Corporation 1985, 1989
 */

#if defined(__cplusplus)
extern "C" {
#endif

#pragma pack(1)

/*
 * Size of each function configuration block.
 */
#define NVM_MAX_SELECTIONS	26
#define NVM_MAX_TYPE		80
#define NVM_MAX_MEMORY		9
#define NVM_MAX_IRQ		7
#define NVM_MAX_DMA		4
#define NVM_MAX_PORT		20
#define NVM_MAX_INIT		60
#define NVM_MAX_DATA		203
#define	NVM_MAX_SLOTS		16


/*
 * Duplicate ID information byte format. (total bytes = 2, offset = 0x4)
 */
typedef struct nvm_dupid {
	unsigned short	dup_id	:4,	/* Duplicate ID number */
			type	:2,	/* Slot type */
			readid	:1,	/* Readable ID */
			dups	:1,	/* Duplicates exist */
			disable :1,	/* Board disable is supported */
			IOcheck :1,	/* EISA I/O check supported */
				:5,	/* Reserved */
			partial :1;	/* Configuration incomplete */
} NVM_DUPID;

typedef struct nvm_dupid eisa_nvm_dupid_t;

#define	EISA_NVM_EXP_SLOT	0x00	/* expansion slot */
#define	EISA_NVM_EMB_SLOT	0x01	/* embedded slot */
#define	EISA_NVM_VIR_SLOT	0x02	/* virtual slot */


/*
 * Function information byte. (total bytes = 1, offset = 0x22)
 */
typedef struct nvm_fib {
	unsigned char	type	:1,	/* Type string present		*/
			memory	:1,	/* Memory configuration present */
			irq	:1,	/* IRQ configuration present	*/
			dma	:1,	/* DMA configuration present	*/
			port	:1,	/* Port configuration present	*/
			init	:1,	/* Port initialization present	*/
			data	:1,	/* Free form data		*/
			disable :1;	/* Function is disabled 	*/
} NVM_FIB;

typedef struct nvm_fib eisa_nvm_fib_t;


/*
 * Returned information from an INT 15h call "Read Slot" (AH=D8h, AL=0). 
 */
typedef struct nvm_slotinfo {
	unsigned char	boardid[4];	/* 0x00:Compressed board ID */
	unsigned short	revision;	/* 0x04:Utility version */
	unsigned char	functions;	/* 0x06:Number of functions */
	NVM_FIB 	fib;		/* 0x07:Function information byte*/
	unsigned short	checksum;	/* 0x08:CFG checksum */
	NVM_DUPID	dupid;		/* 0x0A:Duplicate ID information */
} NVM_SLOTINFO;

typedef struct nvm_slotinfo eisa_nvm_slotinfo_t;

#define	EISA_NVM_SLOTINFO_SIZE	(sizeof(eisa_nvm_slotinfo_t))


/*
 * Memory Configuration Info. (total bytes = 63, offset = 0x73)
 */
typedef struct nvm_memory {
	struct	{
		unsigned char	write	:1,	/* 0x0: Memory is read only */
				cache	:1,	/* Memory is cached */
					:1,	/* Reserved */
				type	:2,	/* Memory type */
				share	:1,	/* Shared Memory */
					:1,	/* Reserved */
				more	:1;	/* More entries follow */
	} config;

	struct	{
		unsigned char	width	:2,	/* 0x1: Data path size */
				decode	:2;	/* Address decode */
	} datapath;

	unsigned char	start[3];	/* 0x2: Start address DIV 100h */
	unsigned short	size;		/* 0x5: Memory size in 1K bytes */

} NVM_MEMORY;

typedef struct nvm_memory eisa_nvm_memory_t;


/*
 * IRQ Configuration Info. (total bytes = 14, offset = 0x82)
 */
typedef struct nvm_irq {
	unsigned short	line	:4,	/* IRQ line */
				:1,	/* Reserved */
			trigger :1,	/* Trigger (EGDE=0, LEVEL=1) */
			share	:1,	/* Sharable */
			more	:1,	/* More follow */
				:8;	/* Reserved */
} NVM_IRQ;

typedef struct nvm_irq eisa_nvm_irq_t;


/*
 * DMA Channel Description. (total bytes = 8, offset = 0xc0)
 */
typedef struct nvm_dma {
	unsigned short	channel :3,	/* DMA channel number */
				:3,	/* Reserved */
			share	:1,	/* Shareable */
			more	:1,	/* More entries follow */
				:2,	/* Reserved */
			width	:2,	/* Transfer size */
			timing	:2,	/* Transfer timing */
				:2;	/* Reserved */
} NVM_DMA;

typedef struct nvm_data eisa_nvm_data_t;


/*
 * I/O port info. (total bytes = 60, offset = 0xc8)
 */
typedef struct nvm_port {
	unsigned char	count	:5,	/* Number of sequential ports - 1 */
				:1,	/* Reserved */
			share	:1,	/* Shareable */
			more	:1;	/* More entries follow */
	unsigned short	address;	/* IO port address */
} NVM_PORT;

typedef struct nvm_port eisa_nvm_port_t;


/*
 * Initialization Data (total bytes = 60, offset = 0x104)
 */
typedef struct nvm_init {
	unsigned char	type	:2,	/* Port type */
			mask	:1,	/* Apply mask */
				:4,	/* Reserved */
			more	:1;	/* More entries follow */

	unsigned short	port;		/* Port address */

	union {
		struct	{
			unsigned char	value;	/* Byte to write */
		} byte_v;

		struct	{
			unsigned char	value,	/* Byte to write */
					mask;	/* Mask to apply */
		} byte_vm;

		struct	{
			unsigned short	value;	/* Word to write */
		} word_v;

		struct	{
			unsigned short	value,	/* Word to write */
					mask;	/* Mask to apply */
		} word_vm;

		struct	{
			unsigned long	value;	/* Dword to write */
		} dword_v;

		struct	{
			unsigned long	value,	/* Dword to write */
					mask;	/* mask to apply */
		} dword_vm;
	} u;
} NVM_INIT;

typedef struct nvm_init eisa_nvm_init_t;


/*
 * Standard Function Configuration Data Block Structure (size = 320 bytes).
 *
 * Returned information from an INT 15h call "Read Function" (AH=D8h, AL=01h). 
 */
typedef struct nvm_funcinfo {
	unsigned char	boardid[4];	/* 0x0: Compressed board ID */
	NVM_DUPID	dupid;		/* 0x4: Duplicate ID information */
	unsigned char	ovl_minor,	/* 0x6: Minor revision of .OVL code */
			ovl_major,	/* 0x7: Major revision of .OVL code */
			selects[NVM_MAX_SELECTIONS]; /* 0x8: Current selections */
	NVM_FIB 	fib;		/* 0x22: Combined function info. byte */
	unsigned char	type[NVM_MAX_TYPE]; /* 0x23: Function type/subtype */

	union {
	   struct {
		NVM_MEMORY	memory[NVM_MAX_MEMORY]; /* 0x73: Memory conf. */
		NVM_IRQ 	irq[NVM_MAX_IRQ];	/* 0xb2: IRQ conf. */
		NVM_DMA 	dma[NVM_MAX_DMA];	/* 0xc0: DMA conf. */
		NVM_PORT	port[NVM_MAX_PORT];	/* 0xc8: PORT conf. */
		unsigned char	init[NVM_MAX_INIT];	/* 0x104: INIT info. */
	   } r;

	   unsigned char	freeform[NVM_MAX_DATA + 1]; /* 0x73: */
	} u;

} NVM_FUNCINFO;

typedef struct nvm_funcinfo eisa_nvm_funcinfo_t;

/*
 * EISA NVRAM Function Information aliases.
 */
#define	enfi_memory		u.r.memory
#define	enfi_irq		u.r.irq
#define	enfi_dma		u.r.dma
#define	enfi_port		u.r.port
#define	enfi_init		u.r.init
#define	enfi_freeform		u.freeform

#define	EISA_NVM_FUNCINFO_SIZE	(sizeof(eisa_nvm_funcinfo_t))

#define	EISA_BUF_SIZE		0x5000	/* buffer reserved for BIOS info */

#define NVM_READ_SLOT		0xD800	/* Read slot information */
#define NVM_READ_FUNCTION	0xD801	/* Read function information */
#define NVM_CLEAR_CMOS		0xD802	/* Clear CMOS memory */
#define NVM_WRITE_SLOT		0xD803	/* Write slot information */
#define NVM_READ_SLOTID 	0xD804	/* Read board ID in specified slot */

#define NVM_XREAD_SLOT		0xD810	/* Read slot info from current source */
#define NVM_XREAD_FUNCTION	0xD811	/* Read func info from current source */

#define NVM_CURRENT		0	/* Use current SCI or CMOS */
#define NVM_CMOS		1	/* Use CMOS */
#define NVM_SCI 		2	/* Use SCI file */
#define NVM_BOTH		3	/* Both CMOS and SCI (write/clear) */
#define NVM_NONE		-1	/* Not established yet */

/*
 * The EISA function return values from the ROM BIOS calls.
 */
#define NVM_SUCCESSFUL		0x00	/* No errors */
#define NVM_INVALID_SLOT	0x80	/* Invalid slot number */
#define NVM_INVALID_FUNCTION	0x81	/* Invalid function number */
#define NVM_INVALID_CMOS	0x82	/* Nonvolatile memory corrupt */
#define NVM_EMPTY_SLOT		0x83	/* Slot is empty */
#define NVM_WRITE_ERROR 	0x84	/* Failure to write to CMOS */
#define NVM_MEMORY_FULL 	0x85	/* CMOS memory is full */
#define NVM_NOT_SUPPORTED	0x86	/* EISA CMOS not supported */
#define NVM_INVALID_SETUP	0x87	/* Invalid Setup information */
#define NVM_INVALID_VERSION	0x88	/* BIOS cannot support this version */

#define NVM_OUT_OF_MEMORY	-1	/* Out of system memory */
#define NVM_INVALID_BOARD	-2	/* Invalid board data */

#define NVM_MEMORY_SYS		0
#define NVM_MEMORY_EXP		1
#define NVM_MEMORY_VIR		2
#define NVM_MEMORY_OTH		3

#define NVM_MEMORY_BYTE 	0
#define NVM_MEMORY_WORD 	1
#define NVM_MEMORY_DWORD	2

#define NVM_DMA_BYTE		0
#define NVM_DMA_WORD		1
#define NVM_DMA_DWORD		2

#define NVM_DMA_DEFAULT 	0
#define NVM_DMA_TYPEA		1
#define NVM_DMA_TYPEB		2
#define NVM_DMA_TYPEC		3

#define NVM_MEMORY_20BITS	0
#define NVM_MEMORY_24BITS	1
#define NVM_MEMORY_32BITS	2

#define NVM_IOPORT_BYTE 	0
#define NVM_IOPORT_WORD 	1
#define NVM_IOPORT_DWORD	2

#define NVM_IRQ_EDGE		0
#define NVM_IRQ_LEVEL		1

#pragma pack()

#endif _IO_AUTOCONF_CA_EISA_NVM_H
