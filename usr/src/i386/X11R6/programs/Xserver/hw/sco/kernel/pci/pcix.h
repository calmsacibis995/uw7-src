#ifndef _SYS_PCI_H
#define _SYS_PCI_H
#ifdef __STDC__
#pragma comment(exestr, "@(#)pcix.h 11.1 ")
#else
#ident "@(#)pcix.h 11.1 "
#endif
/*
 *	Copyright (C) 1994 The Santa Cruz Operation, Inc.
 *		All Rights Reserved.
 *	The information in this file is provided for the exclusive use of
 *	the licensees of The Santa Cruz Operation, Inc.  Such users have the
 *	right to use, modify, and incorporate this code into other products
 *	for purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 */

/*
** PCI driver header file.
*/


#ifdef  _M_I386
#pragma pack(4)
#else
#pragma pack(2)
#endif


/*
** User space, and ioctl defines and structures
*/

#define PCI_DEVS_MECH1	32	/* Max devices on an access mech 1 bus */
#define PCI_DEVS_MECH2	16	/* Max devices on an access mech 2 bus */
#define PCI_MAX_FUNCS 8		/* Max functions per device */
#define PCI_CONFIG_LIMIT 256	/* Size of PCI config space */

/*
** PCI ioctl values
*/

#define PCIIOCT (('p' << 24) | ('c' << 16) | ('i' << 8))

#define PCI_BUS_PRESENT		(PCIIOCT | 0)
#define PCI_FIND_DEVICE		(PCIIOCT | 1)
#define PCI_FIND_CLASS		(PCIIOCT | 2)
#define PCI_SPECIAL_CYCLE	(PCIIOCT | 3)
#define PCI_READ_CONFIG		(PCIIOCT | 4)
#define PCI_WRITE_CONFIG 	(PCIIOCT | 5)
#define PCI_SEARCH_BUS		(PCIIOCT | 6)
#define PCI_TRANS_BASE		(PCIIOCT | 7)

/*
** End of PCI ioctl values
*/

/*
** Defines for some of the fields in the PCI config space predefined header
*/

#define PCI_VENDID 0		/* Vendor ID offset */
#define PCI_DEVID 2		/* Device ID offset */

/*
** PCI structure definitions
*/

struct pci_businfo{
	unsigned short numbuses;	/* Number of PCI buses */
	unsigned short mechanism;	/* HW access mechanism */
};

struct pci_devinfo{
	unsigned short slotnum;		/* Slot in PCI bus */
	unsigned short funcnum;		/* Function of device */
	unsigned short busnum;		/* Bus device is on */
};

struct pci_busdata{
	int bus_present;		/* PCI present flag */
	struct pci_businfo businfo;	/* Businfo structure */
};

struct pci_devicedata{
	unsigned short vend_id;		/* Vendor ID */
	unsigned short dev_id;		/* Device ID */
	unsigned short index;		/* Number of matches */
	int device_present;		/* Device found flag */
	struct pci_devinfo info;	/* Device info structure */
};

struct pci_classdata{
	unsigned short base_class;	/* Base class code */
	unsigned short sub_class;	/* Sub class code */
	unsigned short index;		/* Number of matches */
	int device_present;		/* Device found flag */
	struct pci_devinfo info;	/* Device info structure */
};

struct pci_specialdata{
	unsigned short busno;		/* Bus number for cycle */
	unsigned long busdata;		/* Special cycle data */
};

struct pci_configdata{
	struct pci_devinfo info;	/* Device info structure */
	unsigned short size;		/* Size of data to pass */
	unsigned short reg;		/* Register in config space */
	int status;			/* Completion status */
	unsigned long data;		/* Config space data */
};

struct pci_headerinfo{
	unsigned short vend_id;		/* Vendor ID */
	unsigned short dev_id;		/* Device ID */
	unsigned short base_class;	/* Base class code */
	unsigned short sub_class;	/* Sub class code */
	unsigned short index;		/* Number of matches */
	int device_present;		/* Device found flag */
	struct pci_devinfo info;	/* Device info structure */
};

struct pci_baseinfo{
	struct pci_devinfo info;	/* Device info structure */
	int status;			/* Completion status */
	unsigned long iobase;		/* Un-mapped IO base address */
	unsigned long mappedbase;	/* Re-mapped IO base address */
};

/*
** End of PCI structure definitions
*/

#ifdef _KERNEL
/*
** Kernel service routine defines
*/

#define PCI_CONFIG_ADDRESS 0xCF8	/* Mechanism 1 access */
#define PCI_CONFIG_DATA 0xCFC
#define PCI_CONFIG_ENABLE 0xCF8		/* Mechanism 2 access */
#define PCI_FORWARD 0xCFA
#define PCI_CONFIG_BASE 0xC000		/* Base of IO for mech 2 */
#define PCI_SPECIAL_ADDR 0xCF00		/* Mech 1 special cycle data addr */
#define PCI_CLASS 8			/* Byte offset of class code */
#define PCI_HEAD 0x0E			/* Header type byte offset */
#define PCI_INTLINE 0x3c		/* Interrupt Line offset */

#define PCI_IN 0			/* Move data from config space */
#define PCI_OUT 1			/* Move data to config space */

/*
** Kernel service routine prototypes
*/

#if defined(__STDC__) && !defined(_NO_PROTOTYPE)

int pci_buspresent(struct pci_businfo *);

int pci_finddevice(unsigned short,
			unsigned short,
			unsigned short,
			struct pci_devinfo *);

int pci_findclass(unsigned short,
			unsigned short,
			unsigned short,
			struct pci_devinfo *);

void pci_specialcycle(unsigned short, unsigned long);
int pci_readbyte(struct pci_devinfo *, unsigned short, unsigned char *);
int pci_readword(struct pci_devinfo *, unsigned short, unsigned short *);
int pci_readdword(struct pci_devinfo *, unsigned short, unsigned long *);
int pci_writebyte(struct pci_devinfo *, unsigned short, unsigned char);
int pci_writeword(struct pci_devinfo *, unsigned short, unsigned short);
int pci_writedword(struct pci_devinfo *, unsigned short, unsigned long);

int pci_search(unsigned short,
		unsigned short,
		unsigned short,
		unsigned short,
		unsigned short,
		struct pci_devinfo *);

int pci_transbase(unsigned long *, struct pci_devinfo *);

#endif /* defined(__STDC) && !defined(_NO_PROTOTYPE) */

#endif /* _KERNEL */

#pragma pack()
#endif	/* _SYS_PCI_H */
