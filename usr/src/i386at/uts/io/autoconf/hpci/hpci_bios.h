#ident	"@(#)kern-i386at:io/autoconf/hpci/hpci_bios.h	1.2.1.1"
#ident	"$Header$"

#ifndef _IO_AUTOCONF_CA_HPCI_BIOS_H
#define _IO_AUTOCONF_CA_HPCI_BIOS_H

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *
 * HPCI PCI BIOS interface header file
 * 
 * contains defines for PCI bus driver code, and externs usable by
 * HPCD drivers
*/
#define PCI_FUNCTION_ID			0xb1
#define PCI_FAILURE			-1
#define PCI_REV_2_0			0x0200
#define PCI_REV_2_1			0x0210
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
#define PCI_INVALID_VENDOR_ID       		0xFFFF

/* Header register values */
#define	PCI_HEADER_MULTIFUNC_MASK	0x80
#define PCI_ORDINARY_DEVICE_HEADER	0
#define PCI_TO_PCI_BRIDGE_HEADER	1

/* Class code register values */
#define PCI_BASE_CLASS_PCC		0x00
#define PCI_BASE_CLASS_MASS_STORAGE	0x01
#define PCI_BASE_CLASS_NETWORK		0x02
#define PCI_BASE_CLASS_DISPLAY		0x03
#define PCI_BASE_CLASS_MULTIMEDIA	0x04
#define PCI_BASE_CLASS_MEMORY		0x05
#define PCI_BASE_CLASS_BRIDGE		0x06
	/* PCI 2.1 class codes */
#define PCI_BASE_CLASS_COMMCTLR		0x07
#define PCI_BASE_CLASS_GENERICPERIPH	0x08
#define PCI_BASE_CLASS_INPUTDEVICE	0x09
#define PCI_BASE_CLASS_DOCKINGSTATION	0x0A
#define PCI_BASE_CLASS_PROCESSOR	0x0B
#define PCI_BASE_CLASS_SERIALBUS	0x0C

/* Subclass code register values */

/* to be done */

/* Generic Configuration Space register offsets */
#define PCI_VENDOR_ID_OFFSET		0x00
#define PCI_DEVICE_ID_OFFSET		0x02
#define PCI_COMMAND_OFFSET		0x04
#define PCI_BASE_CLASS_OFFSET		0x0B
#define PCI_SUB_CLASS_OFFSET		0x0A
#define PCI_PROG_INTF_OFFSET		0x09
#define PCI_CACHE_LINE_SIZE_OFFSET	0x0C
#define PCI_LATENCY_TIMER_OFFSET	0x0D
#define PCI_HEADER_TYPE_OFFSET		0x0E
#define PCI_BIST_OFFSET			0x0F
/* Ordinary ("Type 0") Device Configuration Space register offsets */
#define	PCI_BASE_REGISTER_OFFSET    	0x10
#define PCI_SVEN_ID_OFFSET	    	0x2C
#define PCI_SDEV_ID_OFFSET	    	0x2E
#define PCI_E_ROM_BASE_ADDR_OFFSET  	0x30
#define PCI_INTERPT_LINE_OFFSET     	0x3C
#define PCI_INTERPT_PIN_OFFSET      	0x3D
#define PCI_MIN_GNT_OFFSET	    	0x3E
#define PCI_MAX_LAT_OFFSET	    	0x3F

/* PCI-to-PCI Bridge ("Type 1") Configuration Space register offsets */
#define	PCI_PPB_SECONDARY_BUS_NUM_OFFSET	0x19
#define	PCI_PPB_SUBORDINATE_BUS_NUM_OFFSET	0x1A

/* Vendor ID register values */
#define PCI_INVALID_VENDOR_ID       0xFFFF

/* Header register values */
#define	PCI_HEADER_MULTIFUNC_MASK	0x80
#define PCI_ORDINARY_DEVICE_HEADER	0
#define PCI_TO_PCI_BRIDGE_HEADER	1


/* PCI ERROR CODES */

#define PCI_SUCCESS                     0x00
#define PCI_UNSUPPORTED_FUNCT           0x81
#define PCI_BAD_VENDOR_ID               0x83
#define PCI_DEVICE_NOT_FOUND            0x86
#define PCI_BAD_REGISTER_NUMBER         0x87

/* Number of bytes of config space */
#define MAX_PCI_REGISTERS		256
#define NUM_PCI_HEADER_REGISTERS	64
#pragma pack(1)

/* irq routing entry */
struct pci_irqrouting_entry {
        uchar_t         bus_number;
        uchar_t         device_number;
        uchar_t         link_a;
        ushort_t        irqmap_a;
        uchar_t         link_b;
        ushort_t        irqmap_b;
        uchar_t         link_c;
        ushort_t        irqmap_c;
        uchar_t         link_d;
        ushort_t        irqmap_d;
        uchar_t         slot;
        uchar_t         reserved;
};

/* BIOS function interfaces */
int pci_bios_present(rm_key_t hpci, uchar_t *maxbus, ushort_t *bus_rev, uchar_t * conf_cycle_type);
int pci_generate_special_cycle(rm_key_t hpci, uchar_t bus, uint_t data);
int pci_read_byte(rm_key_t hpci, uchar_t bus, uchar_t devfun, ushort_t offset, uchar_t *buf);
int pci_read_word(rm_key_t hpci, uchar_t bus, uchar_t devfun, ushort_t offset, ushort_t *buf);
int pci_read_dword(rm_key_t hpci, uchar_t bus, uchar_t devfun, ushort_t offset, uint_t *buf);
int pci_write_byte(rm_key_t hpci, uchar_t bus, uchar_t devfun, ushort_t offset, uchar_t buf);
int pci_write_word(rm_key_t hpci, uchar_t bus, uchar_t devfun, ushort_t offset, ushort_t buf);
int pci_write_dword(rm_key_t hpci, uchar_t bus, uchar_t devfun, ushort_t offset, uint_t buf);
int pci_get_irq_routing_options(rm_key_t hpci, ushort_t *sz, struct pci_irqrouting_entry *p, ushort_t * bitmap);
int pci_set_hw_interrupt(rm_key_t hpci, uchar_t bus, uchar_t devfun, uchar_t ipin, uchar_t irqnum);
int pci_find_pci_device(rm_key_t hpci, uchar_t *bus, uchar_t *devfun, ushort_t vendor_id, ushort_t device_id, ushort_t index);
int pci_find_pci_class_code(rm_key_t hpci, uchar_t *bus, uchar_t *devfun, uint_t classcode, ushort_t index);

#pragma pack()

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_AUTOCONF_CA_HPCI_BIOS_H */
