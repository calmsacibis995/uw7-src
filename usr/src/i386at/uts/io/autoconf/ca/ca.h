#ifndef _IO_AUTOCONF_CA_CA_H	/* wrapper symbol for kernel use */
#define _IO_AUTOCONF_CA_CA_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/autoconf/ca/ca.h	1.14.9.1"
#ident	"$Header$"

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * The data structure definitions in this file are a derivative 
 * of Plug and Play Driver Developer spec. from Intel.
 */

struct device_id {
	ulong_t		did_busid;	/* bus type */
	ulong_t		did_flags;	/* status flags */

	union {
		struct PCIdevice {
			union {
				struct PCIdevid1 {
					ushort_t physdevid;
					ushort_t vendorid;
				} PCIdevid1;
				ulong_t	pcibrdid;
			} PCIdevid;
		} PCIdevice;

		struct EISAdevice {
			ulong_t	eisabrdid;	/* EISA compressed board id */
		} EISAdevice;

		struct PNPISAdevice {
			ulong_t	serialno;	/* serial/instance number */
			ulong_t	logicalno;	/* logical dev id for PNPISA */
		} PNPISAdevice;

		struct MCAdevice {
			ushort_t mcabrdid;	/* MCA board id */
		} MCAdevice;
	} did;

#define	did_eisabrdid	did.EISAdevice.eisabrdid
#define	did_mcabrdid	did.MCAdevice.mcabrdid
#define	did_pcidevid	did.PCIdevice.PCIdevid.PCIdevid1.physdevid
#define	did_pcivendorid	did.PCIdevice.PCIdevid.PCIdevid1.vendorid
#define	did_pcibrdid	did.PCIdevice.PCIdevid.pcibrdid
};

/*
 * Status flags
 */
#define	DEVICE_INIT		0x01
#define	DEVICE_ENABLED		0x02
#define	DEVICE_LOCKED		0x04


struct bus_access {
	union {
		struct PCIaccess {
			union {
				struct PCIba0 {
					uchar_t	busnumber;
					uchar_t	devfuncnumber;
					ushort_t cgnum;
				} PCIba0;
				ulong_t	pciba;
			} PCIba;
		} PCIaccess;

		struct EISAaccess {
			union {
				struct EISAba0 {
					uchar_t	slotnumber;
					uchar_t	functionnumber;
					ushort_t reserved;
				} EISAba0;
				ulong_t	eisaba;
			} EISAba;
		} EISAaccess;

		struct PNPISAaccess {
			uchar_t	csn;
			uchar_t	logicaldevnumber;	
			ushort_t readdataport;
		} PNPISAaccess;

		struct MCAaccess {
			union {
				struct MCAba0 {
					uchar_t	slotnumber;
				} MCAba0;
				ulong_t	mcaba;
			} MCAba;
		} MCAaccess;
	} ba;

#define	ba_pci_busnumber	ba.PCIaccess.PCIba.PCIba0.busnumber
#define	ba_pci_devfuncnumber	ba.PCIaccess.PCIba.PCIba0.devfuncnumber
#define	ba_pci_cgnum		ba.PCIaccess.PCIba.PCIba0.cgnum
#define	ba_pciba		ba.PCIaccess.PCIba.pciba
#define ba_eisa_slotnumber	ba.EISAaccess.EISAba.EISAba0.slotnumber
#define	ba_eisa_functionnumber	ba.EISAaccess.EISAba.EISAba0.functionnumber
#define	ba_eisa_reserved	ba.EISAaccess.EISAba.EISAba0.reserved
#define	ba_eisaba		ba.EISAaccess.EISAba.eisaba
#define ba_mca_slotnumber	ba.MCAaccess.MCAba.MCAba0.slotnumber
#define	ba_mcaba		ba.MCAaccess.MCAba.mcaba
};

/*
 * The max values in here are based on the maximum resources within
 * a single EISA function block.
 */
#define	MAX_MEM_REGS		9
#define MAX_IO_PORTS		20
#define	MAX_IRQS		7
#define	MAX_DMA_CHANNELS	7
#define MAX_TYPE		80
/*
 * PCI specific parameters - provided with PCI 2.1 bus support
 *
 */
struct pci_specific {
	long	slot;
	union{
		struct subsystem_brdid {
			ushort_t sdev_id; /* subsystem device id*/
			ushort_t sven_id; /* subsystem vendor id*/
		} sbrdid;
		ulong_t	subsysbrdid;
	} sid;
	union{
		struct subsystem_clsid {
			uchar_t class_id; /* class id*/
			uchar_t sclass_id; /* sub class id*/
			uchar_t pad[2];
		} classid;
		ulong_t	subsysclassid;
	} scid;
};
#define ci_pcislot	ci_pci.slot
#define ci_pcisvenid	ci_pci.sid.sbrdid.sven_id
#define ci_pcisdevid	ci_pci.sid.sbrdid.sdev_id
#define ci_pcisbrdid	ci_pci.sid.subsysbrdid
#define ci_pcisclassid	ci_pci.scid.classid.class_id
#define ci_pcissclassid	ci_pci.scid.classid.sclass_id
#define ci_pciclassid	ci_pci.scid.subsysclassid

/*
 * Logical Configuration Data Structure.
 *
 * Note: <device_id, bus_access> pair uniquely identify a device in the system.
 */
		
struct config_info {
	/*
	 * Device id. information.
	 */
	struct device_id ci_deviceid;			/* 0x00: */

#define	ci_busid		ci_deviceid.did_busid
#define	ci_eisabrdid		ci_deviceid.did_eisabrdid
#define	ci_mcabrdid		ci_deviceid.did_mcabrdid
#define	ci_pcivendorid		ci_deviceid.did_pcivendorid
#define	ci_pcidevid		ci_deviceid.did_pcidevid
#define	ci_pcibrdid		ci_deviceid.did_pcibrdid

	/*
	 * Device Access information.
	 */
	struct bus_access ci_busaccess;			/* 0x10: */

#define	ci_eisa_slotnumber	ci_busaccess.ba_eisa_slotnumber
#define ci_eisa_funcnumber	ci_busaccess.ba_eisa_functionnumber
#define	ci_eisa_reserved	ci_busaccess.ba_eisa_reserved
#define	ci_eisaba		ci_busaccess.ba_eisaba
#define	ci_mca_busnumber	ci_busaccess.ba_mca_busnumber
#define	ci_mca_slotnumber	ci_busaccess.ba_mca_slotnumber
#define	ci_mcaba		ci_busaccess.ba_mcaba
#define	ci_pci_busnumber	ci_busaccess.ba_pci_busnumber
#define	ci_pci_devfuncnumber	ci_busaccess.ba_pci_devfuncnumber
#define	ci_pci_cgnum		ci_busaccess.ba_pci_cgnum
#define	ci_pciba		ci_busaccess.ba_pciba

	/*
	 * Memory information.
	 */
	ushort_t	ci_nummemwindows;		/* 0x14: */
	ulong_t		ci_membase[MAX_MEM_REGS];	/* 0x18: */
	ulong_t		ci_memlength[MAX_MEM_REGS];	/* 0x3C: */
	ushort_t	ci_memattr[MAX_MEM_REGS];	/* 0x60: */

	/*
	 * IO ports information.
	 */
	ushort_t	ci_numioports;			/* 0x72: */
	ulong_t		ci_ioport_base[MAX_IO_PORTS];	/* 0x74: */
	ushort_t	ci_ioport_length[MAX_IO_PORTS];	/* 0xC4: */

	/*
	 * IRQ information.
	 */
	ushort_t	ci_numirqs;			/* 0xEC: */
	uchar_t		ci_irqline[MAX_IRQS];		/* 0xEE: */
	uchar_t		ci_irqattrib[MAX_IRQS];		/* 0xF5: */

	/*
	 * DMA information.
	 */
	ushort_t	ci_numdmas;			/* 0xFC: */
	uchar_t		ci_dmachan[MAX_DMA_CHANNELS];	/* 0xFE: */
	ushort_t	ci_dmaattrib[MAX_DMA_CHANNELS];	/* 0x106: */

	/*
	 * Type information.
	 */
	char		ci_type[MAX_TYPE];		/* 0x114: */

	/*
	 * CPU Group -- always zero on non-NUMA systems.
	 */
	cgnum_t		ci_cgnum;			/* 0x168: */

	/*
	 * PCI specific info (valid for PCI 2.1 systems ONLY)
	 */
	struct		pci_specific ci_pci;		/* 0x16C: */
};


/*
 * Memory Attributes.
 */
struct config_memory_info {

	uchar_t	cmemi_decode	:1;	/* Bit 0
					 * 0 = memory upper limit for decoding 
					 * 1 = memory range length for decoding
					 */
	uchar_t	cmemi_datasize0	:2;	/* Bit 1-2
					 * 00 = 8-bit Memory
					 * 01 = 16-bit Memory
					 * 10 = 8 and 16-bit Memory
					 * 11 = Reserved
					 */
	uchar_t	cmemi_datasize1	:2;	/* Bit 3-4
					 * 00 = 32-bit Memory
					 * 01 = 16 and 32-bit Memory
					 * 10, 11 = Reserved
					 */
	uchar_t	cmemi_rdwr 	:1;	/* Bit 5 - 0=ROM, 1=RAM */
	uchar_t	cmemi_shared	:1;	/* Bit 6 - 0=!shared, 1=shared */
	uchar_t	cmemi_rsvrd	:1;	/* Bit 7 - Reserved */

	uchar_t	cmemi_attrib2;		/* Reserved */
};


/*
 * IRQ Attributes.
 */
struct config_irq_info {
	uchar_t	cirqi_trigger	:1;	/* Bit 0 - 0=Edge , 1=Level */
	uchar_t	cirqi_level	:1;	/* Bit 1 - 0=Low, 1=High  */
	uchar_t	cirqi_type	:1;	/* Bit 2 - 0=Non-shared, 1=Sharable */
	uchar_t	cirqi_rsvrd	:5;	/* Bit 3-7 - Reserved */
};


/*
 * DMA Attributes.
 */
struct config_dma_info {
	uchar_t	cdmai_transfersize :2;	/* DMA transfer preference
					 * Bit 0-1
					 * 00 = 8-bit transfer
					 * 01 = 8-bit and 16-bit transfer
					 * 10 = 16-bit transfer
					 * 11 = 32-bit transfer
					 */
	uchar_t	cdmai_busstatus	:1;	/* Logical Device bus master status
					 * Bit 2
					 * 0 = logical device is not a bus master
					 * 1 = logical device is a bus master
					 */
	uchar_t	cdmai_bytemode	:1;	/* DMA byte mode status
					 * Bit 3
					 * 0 = may not execute in count by byte mode
					 * 1 = may execute in count by byte mode
					 */
	uchar_t	cdmai_wordmode	:1;	/* DMA word mode status
					 * Bit 4
					 * 0 = may not execute in count by word mode
					 * 1 = may execute in count by word mode
					 */
	uchar_t	cdmai_timing1	:2;	/* DMA channel speed support
					 * Bit 5-6
					 * 00 = Isa Compatible timing
					 * 01 = Type "A"
					 * 10 = Type "B"
					 * 11 = Type "F"
					 */
	uchar_t	cdmai_rsvrd1	:1;	/* Bit 7 Reserved (set to 0) */
					/* Byte 1 */

	uchar_t	cdmai_type	:1;	/* Bit 0
					 * 0 = Non-Sharable 
					 * 1 = Sharable
					 */
	uchar_t	cdmai_timing2	:1;	/* Bit 1
					 * 0 = C type timing supported
					 * 1 = C type timing not supportedi
					 */
	uchar_t	cdmai_rsvrd2	:6;	/* Bit 2-7 Reserved */
};

#define	CA_SUCCESS	0
#define	CA_FAILURE	1

#ifdef _KERNEL

struct config_info_list {
        struct config_info *ci_ptr;
        struct config_info_list *ci_next;
};

#define	CONFIG_INFO_SIZE	(sizeof(struct config_info))
#define	CONFIG_INFO_LIST_SIZE	(sizeof(struct config_info_list))

extern struct config_info_list *config_info_list_head;
extern struct config_info_list *config_info_list_tail;
extern uint_t ca_config_order;

/*
 * MACRO
 * CONFIG_INFO_KMEM_ZALLOC(config_info *cip)
 *      Allocate memory and prepend/append it to the <config_info_list> list
 *	based on the order set in the tuneable.
 *
 * Calling/Exit State:
 *      None.
 */
#define CONFIG_INFO_KMEM_ZALLOC(cip) { \
	struct config_info_list *cilp; \
	int order = ca_config_order; \
	if (((cip) = (struct config_info *) kmem_zalloc( \
	    CONFIG_INFO_SIZE, KM_NOSLEEP)) == NULL) { \
		cmn_err(CE_WARN, "ca_init: Not enough memory"); \
		return NULL; \
	} \
	if ((cilp = kmem_zalloc(CONFIG_INFO_LIST_SIZE, KM_NOSLEEP)) == NULL) { \
		cmn_err(CE_WARN, "ca_init: Not enough memory"); \
		return NULL; \
	} \
	if (order) { 	/* HIGH_TO_LOW */ \
		cilp->ci_ptr = (cip); \
		cilp->ci_next = config_info_list_head; \
		config_info_list_head = cilp; \
	} else { 	/* LOW_TO_HIGH */ \
		cilp->ci_ptr = (cip); \
		if (config_info_list_tail) \
			config_info_list_tail->ci_next = cilp; \
		else \
			config_info_list_head = cilp; \
		config_info_list_tail = cilp; \
	} \
}

/*
 * MACRO
 * CONFIG_INFO_KMEM_FREE(config_info *cip)
 *      Delete <cip> from the <config_info_list> list and free memory.
 *
 * Calling/Exit State:
 *      None.
 */
#define CONFIG_INFO_KMEM_FREE(cip) { \
	struct config_info_list *cilp; \
	struct config_info_list **cilpp; \
	cilpp = &config_info_list_head; \
	for (; cilp = *cilpp; cilpp = &cilp->ci_next) { \
		if (cilp->ci_ptr == cip) { \
			*cilpp = cilp->ci_next; \
			kmem_free(cip, CONFIG_INFO_SIZE); \
			kmem_free(cilp, CONFIG_INFO_LIST_SIZE); \
			break; \
		} \
	} \
}

extern int	ca_init(void);
extern int	ca_read_devconfig(ulong_t, ulong_t, void *, size_t, size_t);
extern int	ca_write_devconfig(ulong_t, ulong_t, void *, size_t, size_t);
extern size_t	ca_devconfig_size(ulong_t, ulong_t);

#endif /* _KERNEL */

#define CA_IOCTL		('C' << 16 | 'A' << 8)
#define CA_EISA_READ_NVM	(CA_IOCTL | 1)
#define CA_MCA_READ_POS		(CA_IOCTL | 2)
#define	CA_PCI_READ_CFG		(CA_IOCTL | 3)

struct cadata {
	char	*ca_buffer;		/* configuration data buffer */
	size_t	ca_size;		/* configuration data size */
	ulong_t	ca_busaccess;		/* bus access info like slot no. */
};

/*
 * Macros to manipulate "Extended I/O Addresses", which incorporate
 * a 16-bit cgnum along with a 16-bit I/O port address.
 */
#define CA_MAKE_EXT_IO_ADDR(cgnum, port) (((cgnum) << 16) + (port))
#define CA_GET_EXT_IO_ADDR_CG(addr) (((addr) & 0xFFFF0000) >> 16)
#define CA_GET_EXT_IO_ADDR_PORT(addr) ((addr) & 0x0000FFFF)

#endif /* _IO_AUTOCONF_CA_CA_H */
