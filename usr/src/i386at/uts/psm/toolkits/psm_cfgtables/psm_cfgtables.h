#ifndef _PSM_PSM_CFGTABLE_H	/* wrapper symbol for kernel use */
#define _PSM_PSM_CFGTABLE_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:psm/toolkits/psm_cfgtables/psm_cfgtables.h	1.6.2.1"
#ident	"$Header$"

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


/*
 * Structures for holding information extracted from an MPS configuration
 * table.
 */

struct cpu_info {
	volatile unsigned int	idleflag;
	volatile unsigned int	eventflags;	
        unsigned int            cpu_node;
	volatile long		*lapic_addr; 
        unsigned int            lapic_pid;
        unsigned int            lapic_lid;
	unsigned int		lapic_svec;
	unsigned int		lapic_vec0;
	unsigned int		lapic_vec1;
	long			fill[7];	/* pad to cache line boundary */
};

struct bus_info {
	unsigned char	bus_id;
	unsigned char	bus_type;
};

struct ioapic_info {
        ms_lockp_t      	ioapic_lock;
	volatile long		*ioapic_addr;
        unsigned int            ioapic_node;
        unsigned char		ioapic_id;
};

struct nmi_info {
        unsigned char   bus_id;
        unsigned char   line;
        unsigned char   apic;
        unsigned char   local;
        unsigned char   flags;
};

struct intr_info {
	unsigned char	bus_id;
	unsigned char	bus_type;
	unsigned char	irq;
	unsigned char	ioapic_num;
	unsigned char	line;
	unsigned char 	flags;
};

struct mcp_info {
	unsigned short		node_id;
	unsigned long long	mc_table;
};

struct mp_info {
	unsigned int		num_cpus;
	unsigned int		num_buses;
	unsigned int		num_ioapics;
	unsigned int		num_intrs;
	unsigned int		num_pcis;
	unsigned int		num_nmis;
	struct cpu_info	       *cpus;
	struct bus_info        *buses;
	struct ioapic_info     *ioapics;
	struct intr_info       *intrs;
	struct nmi_info        *nmis;
	struct mcp_info	       	mc;
	unsigned char		imcrp;
	unsigned char		platform_name[22];
};

/*
 * Structures for holding information extracted from an MCS configuration
 * table.
 */

struct node_info {
	unsigned short		node_id;
	unsigned char		bsp_id;
	unsigned char		console;
	unsigned char		bsn;
	unsigned long long	local_mp;
	unsigned long long	bsn_mp;
};

struct bridge_info {
	unsigned short		node_id;
	unsigned char		bridge_id;
};

struct mem_info {
        unsigned short          node_id;
        unsigned char           resource_type;
        unsigned char           resource_id;
        unsigned short          memory_type;
        unsigned char           flags;
        unsigned long long      global_addr;
        unsigned long long      local_addr;
        unsigned long long 	size;
};
	
struct mc_info {
	unsigned int		num_nodes;
	unsigned int		num_mc_bridges;
	unsigned int		num_mem_spaces;
	struct node_info       *nodes;
	struct bridge_info     *mc_bridges;
	struct mem_info	       *mem_spaces;
};

/*
 * MPS configuration table's floating pointer structure.
 */

struct mpfp {
	unsigned char sig[4];	/* 0x00: signature "_MP_"	*/
	unsigned int mpaddr;	/* 0x04: physical address pointer to MP table */
	unsigned char len; 
	unsigned char rev;	
	unsigned char checksum;	
	unsigned char mp_feature_byte[5];
};				

/*
 * MPS and MCS configuration table headers and entries.
 */

struct mpchdr {
 	unsigned char sign[4];		/* 0x00: signature "MPAT" */
 	unsigned short tbl_len;		/* 0x04: length of table */
 	unsigned char spec_rev;		/* 0x06: MP+AT spec revision no. */
 	unsigned char checksum;		/* 0x07: */
 	unsigned char oem_id[8];	/* 0x08: */
 	unsigned char product_id[12];	/* 0x10: */
 	unsigned long oem_ptr;		/* 0x1C: pointer to optional oem table*/
 	unsigned short oem_len;		/* 0x20: length of above table */
 	unsigned short num_entry;	/* 0x22: number of 'entry's to follow */
 	unsigned long loc_apic_adr;	/* 0x24: local apic physical address */
	unsigned short ext_len;		/* 0x26: length of extended MP table */
 	unsigned char fill[2];		/* 0x28: */
};
 
struct mcchdr {
        unsigned char sign[4];          /* 0x00: signature "SMMC" */
        unsigned int tbl_len;          	/* 0x04: length of table */
	unsigned char minor_rev;	/* 0x08: minor revision */
	unsigned char major_rev;	/* 0x09: major revision */
	unsigned short num_entries;	/* 0x0A: number of entries */
	unsigned short num_nodes;	/* 0x0C: number of nodes */
	unsigned short reserve;		/* 0x0E: */
	unsigned char oem_id[8];	/* 0x10: */
	unsigned char product_id[12];	/* 0x18: */
	unsigned int checksum;		/* 0x24: */
};

struct mpe_proc {
 	unsigned char entry_type;	/* 0x00: */
 	unsigned char apic_id;		/* 0x01: */
 	unsigned char apic_vers;	/* 0x02: */
 	unsigned char cpu_flags;	/* 0x03: */
 	unsigned int  cpu_signature;	/* 0x04: */
 	unsigned int features;		/* 0x08: */
 	unsigned int fill[2];		/* 0x0C: */
};
 
struct mpe_bus {
 	unsigned char entry_type;	/* 0x00: */
 	unsigned char bus_id;		/* 0x01: */
 	unsigned char name[6];		/* 0x02: */
};
 
struct mpe_ioapic {
 	unsigned char entry_type;	/* 0x00: */
 	unsigned char apic_id;		/* 0x01: */
 	unsigned char apic_vers;	/* 0x02: */
 	unsigned char ioapic_flags;	/* 0x03: */
 	unsigned long io_apic_adr;	/* 0x04: */
};
 
struct mpe_intr {
 	unsigned char entry_type;	/* 0x00: */
 	unsigned char intr_type;	/* 0x01: */
 	unsigned char intr_flags;	/* 0x02: */
 	unsigned char fill;		/* 0x03: */
 	unsigned char src_bus;		/* 0x04: */
 	unsigned char src_irq;		/* 0x05: */
 	unsigned char dest_apicid;	/* 0x06: */
 	unsigned char dest_line;	/* 0x07: */
};
 
struct mpe_mcp {
        unsigned char entry_type;       /* 0x00: */
        unsigned char entry_len;        /* 0x01: */
        unsigned short node_id;         /* 0x02: */
        unsigned int models;      	/* 0x04: */
        unsigned long long table_ptr; 	/* 0x08: */
};

struct mpe_node {
        unsigned char entry_type;       /* 0x00: */
        unsigned char entry_len;        /* 0x01: */
        unsigned short node_id;         /* 0x02: */
        unsigned char apic_id;      	/* 0x04: */
        unsigned char apic_ver;    	/* 0x05: */
	unsigned short flags;		/* 0x06  */
	unsigned long long table_ptr;	/* 0x08  */
};

struct mpe_bridge {
        unsigned char entry_type;       /* 0x00: */
        unsigned char entry_len;        /* 0x01: */
        unsigned short node_id;         /* 0x02: */
        unsigned char bridge_id;        /* 0x04: */
        unsigned char type[7];          /* 0x05: */
        unsigned int vendor_id;         /* 0x0C  */
	unsigned short fw_ver;		/* 0x10  */
	unsigned short cache_line;	/* 0x12  */
	unsigned short flags;		/* 0x14  */
	unsigned short int_features;	/* 0x16  */
	unsigned long long features;	/* 0x1A  */
};

struct mpe_addr {
        unsigned char entry_type;       /* 0x00  */
        unsigned char entry_len;        /* 0x01  */
        unsigned short node_id;         /* 0x02  */
        unsigned char resource_type;    /* 0x04  */
        unsigned char resource_id;      /* 0x05  */
        unsigned short mem_type;        /* 0x06  */
        unsigned short cache_attrs;     /* 0x08  */
        unsigned short res;             /* 0x0A  */
        unsigned int flags;             /* 0x0C  */
        unsigned long long global_addr; /* 0x10  */
        unsigned long long local_addr;  /* 0x18  */
        unsigned long long addr_size;   /* 0x28  */
};

/*
 * Types of entries (stored in bytes[0]).
 */ 
#define CFG_ET_PROC		0	/* processor */
#define CFG_ET_BUS		1	/* bus */
#define CFG_ET_IOAPIC		2	/* i/o apic */
#define CFG_ET_I_INTR		3	/* interrupt assignment -> i/o apic */
#define CFG_ET_L_INTR		4	/* interrupt assignment -> local apic */
#define CFG_ET_MC_PTR		192	/* pointer to MCS config table */
#define CFG_ET_NODE		193	/* MC Node identifier */
#define CFG_ET_MC_BRIDGE	194	/* MC Bridge controller */
#define CFG_ET_MC_ADDR		195	/* MC Address space */
#define CFG_ET_VENDOR_ID	196	/* Optional vendor information */
#define CFG_ET_NULL		256	/* Skip this entry */
 
union mpcentry {
 	char bytes[20];		
 	struct mpe_proc p;
 	struct mpe_bus b;
 	struct mpe_ioapic a;	
 	struct mpe_intr i;
	struct mpe_mcp m;
	struct mpe_node n;
	struct mpe_bridge br;
	struct mpe_addr ad;
			
};

struct mpconfig {
 	struct mpchdr hdr;	/* 0x00: */
 	union mpcentry entry[1]; /* 0x2c: */
};

struct mcconfig {
        struct mcchdr hdr;      /* 0x00: */
        union mpcentry entry[1]; /* 0x2c: */
};


	/* Interrupt Mode Configuration Register bit */
#define CFG_IMCRP        0x80    /* IMCR present */
#define CFG_IMCRA        0x0     /* IMCR absent */
#define CFG_DEF_TYPE     6       /* default table to use */
#define CFG_DEF_IMCR     CFG_IMCRA/* default IMCRP to use */

	/* CPU flags held in a CPU entry */
#define CFG_CPU_ENABLE	0x1
#define CFG_CPU_BOOT	0x2

	/* IO APIC flags held in an IO APIC entry */
#define CFG_IOAPIC_ENABLE	0x1

	/* Definitions for various bus types found in an MP table */
#define CFG_BUS_NONE		0
#define CFG_BUS_ISA		1
#define CFG_BUS_EISA		2
#define CFG_BUS_MCA		3
#define CFG_BUS_PCI		4
#define CFG_BUS_OTHER		0xFF

	/* Definitions for various interrupt types found in an MP table */
#define CFG_INT_INT		0
#define CFG_INT_NMI		1
#define CFG_INT_SMI		2
#define CFG_INT_EXINT		3

        /* Interrupt flags held in an Interrupt entry */
#define CFG_INT_PODEF   0x0
#define CFG_INT_POHIGH  0x1
#define CFG_INT_POLOW   0x3
#define CFG_INT_POMASK  0x3

        /* Interrupt flags held in an intr_info entry */
#define CFG_INT_FL_POMASK       0x1
#define CFG_INT_FL_POLOW        0x0
#define CFG_INT_FL_POHIGH       0x1

        /* Definitions for memory resource types found in an MC table */
#define CFG_MEM_PROC_TYPE           0x00
#define CFG_MEM_BUS_TYPE            0x01
#define CFG_MEM_APIC_TYPE           0x02
#define CFG_MEM_NODE_TYPE           0xC1
#define CFG_MEM_BRIDGE_TYPE         0xC2

#define CFG_MEM_NODE_SHARED	    0x00
#define CFG_MEM_BUS_IO_PORTS        0x81
#define CFG_MEM_BRIDGE_CTLR         0x00
#define CFG_MEM_BRIDGE_ICR          0x01

        /* Definitions for masks for flags found in node types in an MC table */
#define CFG_NODE_ENABLED_MASK       0x0001
#define CFG_NODE_BSN_MASK           0x0002
#define CFG_NODE_CONSOLE_MASK       0x0004

	/* Masks for deriving the PCI interrupt sources */
#define CFG_INT_PCI_IRQ	0x03
#define CFG_INT_PCI_DEV	0x7C

	/* Definitions for the PCI interrupt signals */
#define CFG_INT_PCI_IRQ_A	1
#define CFG_INT_PCI_IRQ_B	2
#define CFG_INT_PCI_IRQ_C	3
#define CFG_INT_PCI_IRQ_D	4

#define CFG_APIC_SIZE	    4096    /* map one page of memory for each APIC */
#define CFG_EBDA_BASE       0x9fc00 /* base of EBDA default location 639k   */
#define CFG_EBDA_PTR        0x40e   /* pointer to base of EBDA segment      */
#define CFG_BMEM_PTR        0x413   /* pointer to installed base mem in kbyte */

ms_bool_t cfgtable_mpinfo(struct mp_info*);
ms_bool_t cfgtable_readmp(struct mpconfig*, struct mp_info*);
ms_bool_t cfgtable_readmc(struct mcconfig*, struct mc_info*);
void	  cfgtable_pci(struct mp_info*, msr_bus_t*, int);
ms_bool_t cfgtable_eisa(struct mp_info*);

#endif /* _PSM_PSM_CFGTABLE_H */
