#ifndef _IO_AUTOCONF_CA_PCI_H
#define _IO_AUTOCONF_CA_PCI_H

#ident	"@(#)kern-i386at:io/autoconf/ca/pci/pci.h	1.5.8.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include "proc/regset.h"	/* REQUIRED */
#include "util/ksynch.h"	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/regset.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */

#else

#include <sys/regset.h>
#include <sys/ksynch.h>

#endif /* _KERNEL_HEADERS */


#pragma pack(1)
/*
 *
 * PCI header file
 * 
 * contains defines for PCI bus driver code, and externs usable by
 * drivers (pci_read/write devconfig, find_pci_id)
*/
#define PCI_FUNCTION_ID		0xb1
#define PCI_FAILURE		-1
#define PCI_BIOS_PRESENT	0x1
#define PCI_FIND_PCI_DEVICE	0x2
#define PCI_FIND_CLASS_CODE		0x3
#define PCI_GENERATE_SPECIAL_CYCLE      0x6
#define PCI_REV_2_0		0x0200
#define PCI_REV_2_1		0x0210

#define PCI_READ_CONFIG_BYTE	0x8
#define PCI_READ_CONFIG_WORD	0x9
#define PCI_READ_CONFIG_DWORD	0xa
#define PCI_WRITE_CONFIG_BYTE	0xb
#define PCI_WRITE_CONFIG_WORD	0xc
#define PCI_WRITE_CONFIG_DWORD	0xd
#define PCI_GET_PCI_IRQ_ROUTING_OPTIONS 0xe
#define PCI_SET_PCI_HARDWARE_INTERRUPT	0xF
/* Generic Configuration Space register offsets */
#define PCI_VENDOR_ID_OFFSET		0x00
#define PCI_DEVICE_ID_OFFSET		0x02
#define PCI_HEADER_OFFSET		0x0E
#define PCI_COMMAND_OFFSET		0x04
#define PCI_BASE_CLASS_OFFSET		0x0B
#define PCI_SUB_CLASS_OFFSET		0x0A
#define PCI_PROG_INTF_OFFSET		0x09

/* Ordinary ("Type 0") Device Configuration Space register offsets */
#define	PCI_BASE_REGISTER_OFFSET    0x10
#define PCI_E_ROM_BASE_ADDR_OFFSET  0x30
#define PCI_INTERPT_LINE_OFFSET     0x3C
#define PCI_INTERPT_PIN_OFFSET      0x3D
#define PCI_SVEN_ID_OFFSET	    0x2C
#define PCI_SDEV_ID_OFFSET	    0x2E

/* PCI-to-PCI Bridge ("Type 1") Configuration Space register offsets */
#define	PCI_PPB_SECONDARY_BUS_NUM_OFFSET	0x19
#define	PCI_PPB_SUBORDINATE_BUS_NUM_OFFSET	0x1A

/* Vendor ID register values */
#define PCI_INVALID_VENDOR_ID       0xFFFF

/* Header register values */
#define	PCI_HEADER_MULTIFUNC_MASK	0x80
#define PCI_ORDINARY_DEVICE_HEADER	0
#define PCI_TO_PCI_BRIDGE_HEADER	1

/* Class code register values */
#define PCI_CLASS_TYPE_PCC		0x00
#define PCI_CLASS_TYPE_MASS_STORAGE	0x01
#define PCI_CLASS_TYPE_NETWORK		0x02
#define PCI_CLASS_TYPE_DISPLAY		0x03
#define PCI_CLASS_TYPE_MULTIMEDIA	0x04
#define PCI_CLASS_TYPE_MEMORY		0x05
#define PCI_CLASS_TYPE_BRIDGE		0x06

/* Subclass code register values */
#define PCI_SUB_CLASS_IDE		0x01

/* Base Registers in PCI Config space */
#define	MAX_BASE_REGISTERS		6
#define	MAX_PPB_BASE_REGISTERS		2


/* PCI ERROR CODES */

#define PCI_SUCCESS                     0x00
#define PCI_UNSUPPORTED_FUNCT           0x81
#define PCI_BAD_VENDOR_ID               0x83
#define PCI_DEVICE_NOT_FOUND            0x86
#define PCI_BAD_REGISTER_NUMBER         0x87
#define PCI_BUFFER_TOO_SMALL		0x89	

#define BIOS_32_START			0xE0000
#define BIOS_32_SIZE			(0xFFFF0 - 0xE0000)
#define MAX_PCI_DEVICES			32
#define MAX_PCI_FUNCTIONS		8
#define MAX_PCI_BUSES			256

/* miscellaneous defines, specific to
 * our PCI ca code
*/

/* a guess as to the size of the PCI bios*/
#define PCI_BIOS_SIZE			0x10000

/* string we search the BIOS32 directory for: */
#define BIOS_32_ID			(caddr_t) "_32_"

/* BIOS 32 directory string for the PCI BIOS */
#define PCI_BIOS_ID			(caddr_t) "$PCI"

/* PCI BIOS ID string */
#define PCI_ID				(caddr_t) "PCI "
#define PCI_ID_REG_VAL			((unsigned int) ('P' | \
				('C' << 8) | ('I' << 16) | (' ' << 24)))

/* Number of bytes of config space */
#define MAX_PCI_REGISTERS		256
#define NUM_PCI_HEADER_REGISTERS	64

/* writing the following to devices command register disables it */
#define PCI_COMMAND_DISABLE		(unsigned short) 0xFFFC
/* expansion rom sizes and header offsets */
#define PCI_EXP_ROM_START_ADDR		0xC0000
#define PCI_EXP_ROM_SIZE		0x30000
#define PCI_EXP_ROM_HDR_CHUNK		0x200
#define PCI_EXP_ROM_HDR_SIG		0xAA55
#define PCI_EXP_ROM_DATA_SIG		"PCIR"
#define PCI_EXP_ROM_PCC_CODE		0
#ifdef _KERNEL_HEADERS
#include <util/types.h>
#elif defined(_KERNEL)
#include <sys/types.h>
#endif
struct pci_rom_header {
	ushort_t	sig;			/* 0x55AA */
	uchar_t		run_length;		/* run-time length */
	uint_t		init_fun;		/* pointer to init fun */
	uchar_t		reserved[17];		/* unique to app */
	ushort_t	offset; 		/* offset to pci_rom_data */
};
struct pci_rom_data {
	uchar_t		signature[4];		/* "PCIR" */
	ushort_t	v_id;			/* vendor id*/
	ushort_t	d_id;			/* device id*/
	ushort_t	vpd_offset;		/* vital product data offset*/
	ushort_t	len;			/* length of this struct */
	uchar_t		rev_code;		/* revision code... 0 */
	uchar_t		base_class[3];		/* base class */
	ushort_t	sub_class;		/* sub class */
	ushort_t	imagelength;		/* run time image length */
	ushort_t	rev_level;		/* rom revision level */
	uchar_t		code_type;		/* type of rom */
	uchar_t		cont_ind;		/* last rom image? */
	uchar_t		pad[2];			/* pad to 16 bytes */
};

struct pci_rom_signature_data {
	ushort_t	vendor_id;
	ushort_t	device_id;
	uint_t		addr;
	uint_t		length;
	uint_t		used;
};
struct pci_irq_routing_entry {
	uchar_t		bus_number;
	uchar_t		device_number;
	uchar_t		link_a;
	ushort_t	irqmap_a;
	uchar_t		link_b;
	ushort_t	irqmap_b;
	uchar_t		link_c;
	ushort_t	irqmap_c;
	uchar_t		link_d;
	ushort_t	irqmap_d;
	uchar_t		slot;
	uchar_t		reserved;
};
struct pci_routebuffer {
	ushort_t	sz;
	void		*addr;
	ushort_t	selector;
};

/*
 * A description of a PCI bus bridge's position in a PCI hierarchy.  If
 * the bus is a host bridge, <parent_bus> will be identical with the
 * bus's own number, and the remaining fields will have no meaning.
 * But if the bus is a PCI-to-PCI Bus Bridge, then the fields will
 * reflect the PPB's location on its parent bus.
 */
struct pci_bus_num_info {
	uchar_t		parent_bus;
	uchar_t		subordinate_bus;
	uchar_t		dev_on_parent;
	uchar_t		func_on_parent;
};

/*
 * A type to describe PCI bus hierarchy info for each CG in system.
 */
struct pci_bus_data {
        ushort_t			pci_bus_rev;
        uchar_t				pci_maxbus;
        uchar_t				pci_conf_cycle_type;
        struct pci_irq_routing_entry	*pci_irq_buffer;
	unsigned			nre;
	struct msr_routing		*rip;
        int				pci_irq_size;
	ushort_t			pci_irq_bitmap;
	struct pci_bus_num_info		bus_table[MAX_PCI_BUSES];
	int				num_rom_signatures;
	struct pci_rom_signature_data	*rom_signature_buf;
	char 				*bios_entry;
	lock_t				*bios_call_lockp;
	lkinfo_t			lockinfo;

	/*
	 * Information for the PCI BIOS call
	 */
	regs			*bios_call_regs; 	/* registers */
	int			bios_call_rval;		/* return value */
	int			bios_call_type;		/* call type */
	void			*buf_out; 		/* buf for copyout */
	void			*buf_in; 		/* buf for copyin */
};

/* visible functions */
extern int find_pci_id(cgnum_t, struct pci_bus_data *);
extern int pci_generate__special_cycle(cgnum_t, uchar_t, uint_t);
extern int generate_pci_special_cycle(uchar_t, uint_t);
extern void *get_pci_bus_data(void);
#if defined(_KERNEL)
extern int _pci_bios_call(void *, char *);
extern int pci_read_devconfig(cgnum_t, uchar_t, uchar_t, uchar_t *, int, int);
extern int pci_write_devconfig(cgnum_t, uchar_t, uchar_t, uchar_t *, int, int);
#endif
#pragma pack()

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_AUTOCONF_CA_PCI_H */
