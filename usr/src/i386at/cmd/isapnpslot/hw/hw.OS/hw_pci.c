/*
 * File hw_pci.c
 * Information handler for pci
 *
 * @(#) hw_pci.c 65.1 97/06/02 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/pci.h>
#include <string.h>
#include <ctype.h>

#include "hw_pci.h"
#include "hw_util.h"
#include "pci_data.h"

typedef struct pci_busdata	pci_busdata_t;
typedef struct pci_headerinfo	pci_headerinfo_t;
typedef struct pci_configdata	pci_configdata_t;

const char	*const callme_pci[] =
{
    "pci",
    "pci_bus",
    "pci_buss",
    NULL
};

const char	short_help_pci[] = "Info on PCI bus devices";

static int	PciFd = -1;

/*
 * The config space is actually 256 bytes long.  Only the first
 * part is useful to us here since the remainder is chip specific.
 */

typedef struct pci_cfg_space_s
{
    u_short	VendorId;		/* [00] */
    u_short	DeviceId;		/* [02] */
    u_short	Command;		/* [04] */
    u_short	Status;			/* [06] */
    u_char	RevId;			/* [08] */
    u_char	ClassInterface;		/* [09] */
    u_char	SubClass;		/* [0a] */
    u_char	BaseClass;		/* [0b] */
    u_char	CacheLineSize;		/* [0c] */
    u_char	LatencyTimer;		/* [0d] */
    u_char	HeaderType;		/* [0e] */
    u_char	Bist;			/* [0f] */

    union
    {
	struct pci_cfg0_space_s
	{
	    u_long	BaseAddr[6];		/* [10] */
	    u_long	CardbusCISPtr;		/* [28] */
	    u_short	SubsysVendorID;		/* [2c] */
	    u_short	SubsystemID;		/* [2e] */

	    u_long	ROMbaseAddress;		/* [30] */
	    u_long	reserved1;		/* [34] */
	    u_long	reserved2;		/* [38] */
	    u_char	InterruptLine;		/* [3c] */
	    u_char	InterruptPin;		/* [3d] */
	    u_char	Min_Gnt;		/* [3e] */
	    u_char	Max_Lat;		/* [3f] */
	} Type0;

	struct pci_cfg1_space_s	/* PCI-to-PCI bridge */
	{
	    u_long	BaseAddr[2];		/* [10] */
	    u_char	PriBusNum;		/* [18] */
	    u_char	SecBusNum;		/* [19] */
	    u_char	SubBusNum;		/* [1a] */
	    u_char	SecLatencyTimer;	/* [1b] */
	    u_char	IoBase;			/* [1c] */
	    u_char	IoLimit;		/* [1d] */
	    u_short	SecStatus;		/* [1e] */
	    u_short	MemBase;		/* [20] */
	    u_short	MemLimit;		/* [22] */
	    u_short	PrefetchMemBase;	/* [24] */
	    u_short	PrefetchMemLimit;	/* [26] */
	    u_long	PrefetchBaseUpper;	/* [28] */
	    u_long	PrefetchLimitUpper;	/* [2c] */
	    u_short	IoBaseUpper;		/* [30] */
	    u_short	IoLimitUpper;		/* [32] */
	    u_long	reserved1;		/* [34] */
	    u_long	ROMbaseAddress;		/* [38] */
	    u_char	InterruptLine;		/* [3c] */
	    u_char	InterruptPin;		/* [3d] */
	    u_short	BridgeControl;		/* [3e] */
	} Type1;
    } Header;
} pci_cfg_space_t;

/*
 * Class information is stored in sparce arrays.
 */

typedef struct
{
    u_char	intf_id;
    const char	*descr;
} pci_interf_t;

#define END_OF_INTERFACE	{ 0x00, NULL }

typedef struct
{
    u_char		subclass_id;
    const pci_interf_t	*interface;
    const char		*(*func)(u_char intf);
} pci_subclass_t;

#define PCI_SUBCLASS(c,s,f)	{ 0x##s, pci_intf_##c##_##s, f }
#define END_OF_SUBCLASS		{ 0x00, NULL, NULL }

typedef struct
{
    u_char			class_id;
    const pci_subclass_t	*subclass;
    const char			*class_descr;
} pci_class_t;

#define END_OF_CLASS	{ 0x00, NULL, NULL }

/*
 * Base Class 0x00
 * Unclassified device
 */

static const pci_interf_t	pci_intf_00_00[] =
{
    { 0x00, "Unclassified non-VGA device" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_00_01[] =
{
    { 0x00, "VGA-compatible device" },
    END_OF_INTERFACE
};

static const pci_subclass_t	pci_subclass_00[] =
{
    PCI_SUBCLASS(00, 00, NULL),
    PCI_SUBCLASS(00, 01, NULL),
    END_OF_SUBCLASS
};

/*
 * Base Class 0x01
 * Mass storage controller
 */

static const pci_interf_t	pci_intf_01_00[] =
{
    { 0x00, "SCSI bus controller" },
    END_OF_INTERFACE
};

#define pci_intf_01_01	NULL
static const char *pci_intf_01_01_func(u_char intf);

static const pci_interf_t	pci_intf_01_02[] =
{
    { 0x00, "Floppy disk controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_01_03[] =
{
    { 0x00, "IPI bus controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_01_04[] =
{
    { 0x00, "RAID controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_01_80[] =
{
    { 0x00, "Other mass storage controller" },
    END_OF_INTERFACE
};

static const pci_subclass_t	pci_subclass_01[] =
{
    PCI_SUBCLASS(01, 00, NULL),
    PCI_SUBCLASS(01, 01, pci_intf_01_01_func),
    PCI_SUBCLASS(01, 02, NULL),
    PCI_SUBCLASS(01, 03, NULL),
    PCI_SUBCLASS(01, 04, NULL),
    PCI_SUBCLASS(01, 80, NULL),
    END_OF_SUBCLASS
};

/*
 * Base Class 0x02
 * Network controller
 */

static const pci_interf_t	pci_intf_02_00[] =
{
    { 0x00, "Ethernet controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_02_01[] =
{
    { 0x00, "Token Ring controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_02_02[] =
{
    { 0x00, "FDDI controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_02_03[] =
{
    { 0x00, "ATM controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_02_80[] =
{
    { 0x00, "Other network controller" },
    END_OF_INTERFACE
};

static const pci_subclass_t	pci_subclass_02[] =
{
    PCI_SUBCLASS(02, 00, NULL),
    PCI_SUBCLASS(02, 01, NULL),
    PCI_SUBCLASS(02, 02, NULL),
    PCI_SUBCLASS(02, 03, NULL),
    PCI_SUBCLASS(02, 80, NULL),
    END_OF_SUBCLASS
};

/*
 * Base Class 0x03
 * Display controller
 */

static const pci_interf_t	pci_intf_03_00[] =
{
    { 0x00, "VGA-compatible display controller" },
    { 0x01, "8514-compatible display controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_03_01[] =
{
    { 0x00, "XGA display controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_03_80[] =
{
    { 0x00, "Other display controller" },
    END_OF_INTERFACE
};

static const pci_subclass_t	pci_subclass_03[] =
{
    PCI_SUBCLASS(03, 00, NULL),
    PCI_SUBCLASS(03, 01, NULL),
    PCI_SUBCLASS(03, 80, NULL),
    END_OF_SUBCLASS
};

/*
 * Base Class 0x04
 * Multimedia device
 */

static const pci_interf_t	pci_intf_04_00[] =
{
    { 0x00, "Video multimedia device" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_04_01[] =
{
    { 0x00, "Audio multimedia device" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_04_80[] =
{
    { 0x00, "Other multimedia device" },
    END_OF_INTERFACE
};

static const pci_subclass_t	pci_subclass_04[] =
{
    PCI_SUBCLASS(04, 00, NULL),
    PCI_SUBCLASS(04, 01, NULL),
    PCI_SUBCLASS(04, 80, NULL),
    END_OF_SUBCLASS
};

/*
 * Base Class 0x05
 * Memory controller
 */

static const pci_interf_t	pci_intf_05_00[] =
{
    { 0x00, "RAM memory controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_05_01[] =
{
    { 0x00, "Flash memory controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_05_80[] =
{
    { 0x00, "Other memory controller" },
    END_OF_INTERFACE
};

static const pci_subclass_t	pci_subclass_05[] =
{
    PCI_SUBCLASS(05, 00, NULL),
    PCI_SUBCLASS(05, 01, NULL),
    PCI_SUBCLASS(05, 80, NULL),
    END_OF_SUBCLASS
};

/*
 * Base Class 0x06
 * Bridge device
 */

static const pci_interf_t	pci_intf_06_00[] =
{
    { 0x00, "Host bridge" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_06_01[] =
{
    { 0x00, "ISA bridge" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_06_02[] =
{
    { 0x00, "EISA bridge" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_06_03[] =
{
    { 0x00, "MCA bridge" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_06_04[] =
{
    { 0x00, "PCI-to-PCI bridge" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_06_05[] =
{
    { 0x00, "PCMCIA bridge" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_06_06[] =
{
    { 0x00, "NuBus bridge" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_06_07[] =
{
    { 0x00, "CardBus bridge" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_06_80[] =
{
    { 0x00, "Other bridge device" },
    END_OF_INTERFACE
};

static const pci_subclass_t	pci_subclass_06[] =
{
    PCI_SUBCLASS(06, 00, NULL),
    PCI_SUBCLASS(06, 01, NULL),
    PCI_SUBCLASS(06, 02, NULL),
    PCI_SUBCLASS(06, 03, NULL),
    PCI_SUBCLASS(06, 04, NULL),
    PCI_SUBCLASS(06, 05, NULL),
    PCI_SUBCLASS(06, 06, NULL),
    PCI_SUBCLASS(06, 07, NULL),
    PCI_SUBCLASS(06, 80, NULL),
    END_OF_SUBCLASS
};

/*
 * Base Class 0x07
 * Simple communications controller
 */

static const pci_interf_t	pci_intf_07_00[] =
{
    { 0x00, "Generic XT-compatible serial controller" },
    { 0x01, "16450-compatible serial controller" },
    { 0x02, "16550-compatible serial controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_07_01[] =
{
    { 0x00, "Parallel port" },
    { 0x01, "Bidirectional parallel port" },
    { 0x02, "ECP 1.x compliant parallel port" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_07_80[] =
{
    { 0x00, "Other communications device" },
    END_OF_INTERFACE
};

static const pci_subclass_t	pci_subclass_07[] =
{
    PCI_SUBCLASS(07, 00, NULL),
    PCI_SUBCLASS(07, 01, NULL),
    PCI_SUBCLASS(07, 80, NULL),
    END_OF_SUBCLASS
};

/*
 * Base Class 0x08
 * Base system peripherals
 */

static const pci_interf_t	pci_intf_08_00[] =
{
    { 0x00, "Generic 8259 PIC" },
    { 0x01, "ISA PIC" },
    { 0x02, "EISA PIC" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_08_01[] =
{
    { 0x00, "Generic 8237 DMA controller" },
    { 0x01, "ISA DMA controller" },
    { 0x02, "EISA DMA controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_08_02[] =
{
    { 0x00, "Generic 8254 system timer" },
    { 0x01, "ISA system timer" },
    { 0x02, "EISA system timer" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_08_03[] =
{
    { 0x00, "Generic RTC controller" },
    { 0x01, "ISA RTC controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_08_80[] =
{
    { 0x00, "Other system peripheral" },
    END_OF_INTERFACE
};

static const pci_subclass_t	pci_subclass_08[] =
{
    PCI_SUBCLASS(08, 00, NULL),
    PCI_SUBCLASS(08, 01, NULL),
    PCI_SUBCLASS(08, 02, NULL),
    PCI_SUBCLASS(08, 03, NULL),
    PCI_SUBCLASS(08, 80, NULL),
    END_OF_SUBCLASS
};

/*
 * Base Class 0x09
 * Input device
 */

static const pci_interf_t	pci_intf_09_00[] =
{
    { 0x00, "Keyboard controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_09_01[] =
{
    { 0x00, "Digitizer (pen)" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_09_02[] =
{
    { 0x00, "Mouse controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_09_80[] =
{
    { 0x00, "Other input controller" },
    END_OF_INTERFACE
};

static const pci_subclass_t	pci_subclass_09[] =
{
    PCI_SUBCLASS(09, 00, NULL),
    PCI_SUBCLASS(09, 01, NULL),
    PCI_SUBCLASS(09, 02, NULL),
    PCI_SUBCLASS(09, 80, NULL),
    END_OF_SUBCLASS
};

/*
 * Base Class 0x0a
 * Docking station
 */

static const pci_interf_t	pci_intf_0a_00[] =
{
    { 0x00, "Generic docking station" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_0a_80[] =
{
    { 0x00, "Other type of docking station" },
    END_OF_INTERFACE
};

static const pci_subclass_t	pci_subclass_0a[] =
{
    PCI_SUBCLASS(0a, 00, NULL),
    PCI_SUBCLASS(0a, 80, NULL),
    END_OF_SUBCLASS
};

/*
 * Base Class 0x0b
 * Processor device
 */

static const pci_interf_t	pci_intf_0b_00[] =
{
    { 0x00, "386 processor device" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_0b_01[] =
{
    { 0x00, "486 processor device" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_0b_02[] =
{
    { 0x00, "Pentium processor device" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_0b_10[] =
{
    { 0x00, "Alpha processor device" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_0b_20[] =
{
    { 0x00, "PowerPC processor device" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_0b_40[] =
{
    { 0x00, "Co-processor processor device" },
    END_OF_INTERFACE
};

static const pci_subclass_t	pci_subclass_0b[] =
{
    PCI_SUBCLASS(0b, 00, NULL),
    PCI_SUBCLASS(0b, 01, NULL),
    PCI_SUBCLASS(0b, 02, NULL),
    PCI_SUBCLASS(0b, 10, NULL),
    PCI_SUBCLASS(0b, 20, NULL),
    PCI_SUBCLASS(0b, 40, NULL),
    END_OF_SUBCLASS
};

/*
 * Base Class 0x0c
 * Serial bus controller
 */

static const pci_interf_t	pci_intf_0c_00[] =
{
    { 0x00, "FireWire (IEEE 1394) bus controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_0c_01[] =
{
    { 0x00, "ACCESS.bus bus controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_0c_02[] =
{
    { 0x00, "SSA bus controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_0c_03[] =
{
    { 0x00, "Universal Serial Bus (USB) bus controller" },
    END_OF_INTERFACE
};

static const pci_interf_t	pci_intf_0c_04[] =
{
    { 0x00, "Fiber Channel bus controller" },
    END_OF_INTERFACE
};

static const pci_subclass_t	pci_subclass_0c[] =
{
    PCI_SUBCLASS(0c, 00, NULL),
    PCI_SUBCLASS(0c, 01, NULL),
    PCI_SUBCLASS(0c, 02, NULL),
    PCI_SUBCLASS(0c, 03, NULL),
    PCI_SUBCLASS(0c, 04, NULL),
    END_OF_SUBCLASS
};

/*
 * The class data lookup for header encoding 0 and 1 classes
 * begins with this table.  The description field in this
 * structure is only displayed if there is no more specific
 * match in the subclass.
 */

static const pci_class_t	pci_class_0[] =
{
    { 0x00, pci_subclass_00, "Unclassified device" },

    { 0x01, pci_subclass_01, "Mass storage controller" },
    { 0x02, pci_subclass_02, "Network controller" },
    { 0x03, pci_subclass_03, "Display controller" },
    { 0x04, pci_subclass_04, "Multimedia device" },
    { 0x05, pci_subclass_05, "Memory controller" },
    { 0x06, pci_subclass_06, "Bridge device" },
    { 0x07, pci_subclass_07, "Simple communications controller" },
    { 0x08, pci_subclass_08, "Base system peripherals" },
    { 0x09, pci_subclass_09, "Input device" },
    { 0x0a, pci_subclass_0a, "Docking station" },
    { 0x0b, pci_subclass_0b, "Processor device" },
    { 0x0c, pci_subclass_0c, "Serial bus controller" },

    { 0xff, NULL,	     "Other device" },
    END_OF_CLASS
};

static void show_bios32(FILE *out);
static void show_base_addr(FILE *out, const u_long *BaseAddr, int cnt);
static void show_irq(FILE *out, u_char InterruptLine, u_char InterruptPin);
static void show_ROMbase(FILE *out, u_long ROMbaseAddress);
static const pci_busdata_t *get_bus_present(void);
static const pci_cfg_space_t *get_pci_config(pci_headerinfo_t *ph);
static int open_pci(void);
static const char *pci_vendor_name(u_short vendor_id,
					u_short device_id, int rev);
static const char *pci_device_name(u_short vendor_id,
					u_short device_id, int rev);
static const char *pci_class_descr(u_char header_type, u_char base_class,
				    u_char subclass, u_char class_interface);

#define ANY_PCI_MATCH	0xffff

int
have_pci(void)
{
    const pci_busdata_t	*pres;


    if (!(pres = get_bus_present()) || !pres->bus_present)
	return 0;

    return 1;
}

void
report_pci(FILE * out)
{
    const pci_busdata_t	*pres;
    int			maxindex;
    int			index;


    report_when(out, "PCI");

    if (!(pres = get_bus_present()) || !pres->bus_present)
    {
	fprintf(out, "    No PCI bus found\n");
	return;
    }

    load_pci_data();

    fprintf(out, "    Present:   %d", pres->bus_present);
    if (pres->bus_present != 1)
	fprintf(out, " <-- WARNING: normal value is 1");
    fprintf(out, "\n");

    fprintf(out, "    Mechanism: %d", pres->businfo.mechanism);
    switch (pres->businfo.mechanism)
    {
	case 1:
	case 2:
	    break;

	case 32:
	    fprintf(out, "    PCI BIOS32 ROM servives");
	    break;

	default:
	    fprintf(out, " <-- WARNING: normal values are 1, 2 and 32");
    }
    fprintf(out, "\n");

    fprintf(out, "    PCI buses: %d\n", pres->businfo.numbuses);

    /*
     * At this point, we know that there is at least one PCI
     * bus in this system.  Now, get the information about the
     * devices on the bus...
     */

    maxindex = pres->businfo.numbuses *
	    ((pres->businfo.mechanism == 1)
	     ? PCI_DEVS_MECH1
	     : PCI_DEVS_MECH2) *
	    PCI_MAX_FUNCS;

    if (verbose)
	fprintf(out, "    MaxIndex:  %d\n", maxindex);

    show_bios32(out);

    for (index = 0; index < maxindex; index++)
    {
	pci_headerinfo_t	ph;
	const pci_cfg_space_t	*cfg;
	int			n;
	const char		*tp;


	ph.vend_id =	ANY_PCI_MATCH;
	ph.dev_id =	ANY_PCI_MATCH;
	ph.base_class =	ANY_PCI_MATCH;
	ph.sub_class =	ANY_PCI_MATCH;
	ph.index =	index;

	if (ioctl(PciFd, PCI_SEARCH_BUS, &ph) == -1)
	{
	    error_print("PCI bus device search failed");
	    break;
	}

	if (!ph.device_present)
	    continue;

	/*
	 * Patch for everest pci driver problem where it
	 * keeps reporting the same device over and over
	 * again when it has actually run out of devices...
	 */

	if (ph.info.busnum >= pres->businfo.numbuses)
	{
	    debug_print("Invalid bus: %d\n", ph.info.busnum);
	    break;
	}

	fprintf(out, "\n    Index: %d\n", ph.index);
	fprintf(out, "\tDeviceNum:\t%d\n", ph.info.slotnum);
	fprintf(out, "\tFunction:\t%d\n", ph.info.funcnum);
	fprintf(out, "\tBus:\t\t%d\n", ph.info.busnum);

#ifdef NOT_YET_IMPLEMENTED
	ioctl for PCI 2.1 slot call ##
	fprintf(out, "\tSlot:\t\t%d\n", slot);
#endif

	fflush(out);	/* The get_pci_config() takes a long time */
	if (!(cfg = get_pci_config(&ph)))
	    continue;

	fprintf(out, "\tVendorId:\t");
	if (verbose)
	    fprintf(out, "0x%.4x      ", cfg->VendorId);
	tp = pci_vendor_name(cfg->VendorId, cfg->DeviceId, cfg->RevId);
	if (tp != NULL)
	    fprintf(out, "%s\n", tp);
	else if (!verbose)
	    fprintf(out, "0x%.4x\n", cfg->VendorId);
	else
	    fprintf(out, "\n");

	fprintf(out, "\tDeviceId:\t");
	if (verbose)
	    fprintf(out, "0x%.4x      ", cfg->DeviceId);
	tp = pci_device_name(cfg->VendorId, cfg->DeviceId, cfg->RevId);
	if (tp != NULL)
	    fprintf(out, "%s\n", tp);
	else if (!verbose)
	    fprintf(out, "0x%.4x\n", cfg->DeviceId);
	else
	    fprintf(out, "\n");

	fprintf(out, "\tRevId:\t\t0x%.2x\n", cfg->RevId);

	fprintf(out, "\tCommand:\t0x%.4x      Memory %s, I/O %s\n",
				cfg->Command,
				(cfg->Command & 0x0002) ? Enabled : Disabled,
				(cfg->Command & 0x0001) ? Enabled : Disabled);

	fprintf(out, "\tStatus:\t\t0x%.4x\n", cfg->Status);

	fprintf(out, "\tClassCode:\t0x%.2x%.2x%.2x",
						cfg->BaseClass,
						cfg->SubClass,
						cfg->ClassInterface);
	tp = pci_class_descr(cfg->HeaderType, cfg->BaseClass,
				    cfg->SubClass, cfg->ClassInterface);
	if (tp != NULL)
	    fprintf(out, "    %s", tp);
	fprintf(out, "\n");

	fprintf(out, "\tCacheLineSize:\t%d           %s\n",
						cfg->CacheLineSize,
						(cfg->CacheLineSize == 0)
						    ? "No limiting"
						    : "32 bit words");

	if (cfg->LatencyTimer || verbose)
	    fprintf(out, "\tLatencyTimer:\t0x%.2x\n", cfg->LatencyTimer);

	fprintf(out, "\tHeaderType:\t0x%.2x        ", cfg->HeaderType);
	fprintf(out, "%s-function, ",
	    (cfg->HeaderType & 0x80) ? "Multi" : "Single");
	switch (cfg->HeaderType & 0x7f)
	{
	    case 0x00:
		fprintf(out, "Standard encoding");
		break;

	    case 0x01:
		fprintf(out, "PCI-to-PCI bridge");
		break;

	    default:
		fprintf(out, "Reserved encoding");
	}
	fprintf(out, "\n");

	if (cfg->Bist || verbose)
	    fprintf(out, "\tSelfTest:\t0x%.2x        %s BIST capable\n",
				cfg->Bist, (cfg->Bist & 0x80) ? "Is" : "Not");

	if ((cfg->HeaderType & 0x7f) == 0x00)
	{
	    /*
	     * Header type zero - standard encoding
	     */

	    const struct pci_cfg0_space_s	*cfg0 = &cfg->Header.Type0;

	    show_base_addr(out, cfg0->BaseAddr,
				sizeof(cfg0->BaseAddr)/sizeof(*cfg0->BaseAddr));

	    if (cfg0->CardbusCISPtr || verbose)
		fprintf(out, "\tCardbusCISPtr:\t0x%.8x\n", cfg0->CardbusCISPtr);

	    if (cfg0->SubsysVendorID || verbose)
	    {
		fprintf(out, "\tSubsysVendorID:\t");
		if (verbose)
		    fprintf(out, "0x%.4x      ", cfg0->SubsysVendorID);
		tp = pci_vendor_name(cfg0->SubsysVendorID,cfg0->SubsystemID,-1);
		if (tp != NULL)
		    fprintf(out, "%s\n", tp);
		else if (!verbose)
		    fprintf(out, "0x%.4x\n", cfg0->SubsysVendorID);
		else
		    fprintf(out, "\n");
	    }

	    if (cfg0->SubsystemID || verbose)
	    {
		fprintf(out, "\tSubsystemID:\t");
		if (verbose)
		    fprintf(out, "0x%.4x      ", cfg0->SubsystemID);
		tp = pci_device_name(cfg0->SubsysVendorID,cfg0->SubsystemID,-1);
		if (tp != NULL)
		    fprintf(out, "%s\n", tp);
		else if (!verbose)
		    fprintf(out, "0x%.4x\n", cfg0->SubsystemID);
		else
		    fprintf(out, "\n");
	    }

	    show_ROMbase(out, cfg0->ROMbaseAddress);
	    show_irq(out, cfg0->InterruptLine, cfg0->InterruptPin);

	    if (cfg0->Min_Gnt || cfg0->Max_Lat || verbose)
	    {
		fprintf(out, "\tMin_Gnt:\t0x%.2x\n", cfg0->Min_Gnt);
		fprintf(out, "\tMax_Lat:\t0x%.2x\n", cfg0->Max_Lat);
	    }
	}
	else if ((cfg->HeaderType & 0x7f) == 0x01)
	{
	    /*
	     * Header type one - PCI-to-PCI bridge
	     */

	    const struct pci_cfg1_space_s	*cfg1 = &cfg->Header.Type1;

	    show_base_addr(out, cfg1->BaseAddr,
				sizeof(cfg1->BaseAddr)/sizeof(*cfg1->BaseAddr));

	    fprintf(out, "\tPriBusNum:\t%d\n", cfg1->PriBusNum);
	    fprintf(out, "\tSecBusNum:\t%d\n", cfg1->SecBusNum);
	    fprintf(out, "\tSubBusNum:\t%d\n", cfg1->SubBusNum);

	    if (cfg1->SecLatencyTimer || verbose)
		fprintf(out, "\tSecLatencyTime:\t0x%.2x\n",
							cfg1->SecLatencyTimer);

	    if (cfg1->IoBase || cfg1->IoLimit || verbose)
	    {
		fprintf(out, "\tIoBase:\t\t");
		if (verbose || ((cfg1->IoBase & 0x0f) > (u_int)0x01))
		    fprintf(out, "0x%.2x        ", cfg1->IoBase);

		switch (cfg1->IoBase & 0x0f)
		{
		    case 0x00:		/* 16 bit addressing */
			fprintf(out, "0x%.4x", 
					    ((cfg1->IoBase & 0xf0) << 8));
			break;

		    case 0x01:		/* 32 bit addressing */
			fprintf(out, "0x%.8lx", 
					(u_long)(cfg1->IoBaseUpper << 16) |
					       ((cfg1->IoBase & 0xf0) << 8));
			break;

		    default:
			fprintf(out, "Undefined addressing\n");
		}

		if ((cfg1->IoBase & 0x0f) <= (u_int)0x01)
		    switch (cfg1->IoLimit & 0x0f)
		    {
			case 0x00:		/* 16 bit addressing */
			    fprintf(out, "-0x%.4x\n", 
				(cfg1->IoLimit == 0)
				    ? 0xffff
				    : ((cfg1->IoLimit & 0xf0) << 8) | 0x0fff);
			    break;

			case 0x01:		/* 32 bit addressing */
			    fprintf(out, "-0x%.8lx\n", 
				((cfg1->IoLimit | cfg1->IoLimitUpper) == 0)
				    ? 0xffffffff
				    : (u_long)(cfg1->IoLimitUpper << 16) |
					   ((cfg1->IoLimit & 0xf0) << 8) |
					   0x0fff);
			    break;

			default:
			    fprintf(out, ", Undefined addressing\n");
		    }

		if (verbose || ((cfg1->IoBase & 0x0f) > (u_int)0x01))
		    fprintf(out, "\tIoBaseUpper:\t0x%.4x\n",
						    cfg1->IoBaseUpper);

		if (verbose || ((cfg1->IoLimit & 0x0f) > (u_int)0x01))
		    fprintf(out, "\tIoLimit:\t0x%.2x\n"
				 "\tIoLimitUpper:\t0x%.4x\n",
							cfg1->IoLimit,
							cfg1->IoLimitUpper);
	    }

	    if (verbose || cfg1->MemBase || cfg1->MemLimit)
	    {
		fprintf(out, "\tMemBase:\t");
		if (verbose)
		    fprintf(out, "0x%.4x      ", cfg1->MemBase);
		fprintf(out, "0x%.8x-0x%.8x\n",
			    (u_long)(cfg1->MemBase & 0xfff0) << 16,
			    (cfg1->MemLimit == 0)
				? 0xffffffff
				: ((u_long)(cfg1->MemLimit & 0xfff0) << 16));
		if (verbose)
		    fprintf(out, "\tMemLimit:\t0x%.4x\n", cfg1->MemLimit);

	    }

	    if (cfg1->PrefetchMemBase || cfg1->PrefetchMemLimit || verbose)
	    {
		fprintf(out, "\tPrefMemBase:\t");
		if (verbose || ((cfg1->PrefetchMemBase & 0x000f) > (u_int)0x01))
		    fprintf(out, "0x%.4x      ", cfg1->PrefetchMemBase);

		switch (cfg1->PrefetchMemBase & 0x0f)
		{
		    case 0x00:		/* 16 bit addressing */
			fprintf(out, "0x%.4x", cfg1->PrefetchMemBase & 0xfff0);
			break;

		    case 0x01:		/* 32 bit addressing */
			fprintf(out, "0x%.8lx", 
				    (u_long)(cfg1->PrefetchBaseUpper << 16) |
					   (cfg1->PrefetchMemBase & 0xfff0));
			break;

		    default:
			fprintf(out, "Undefined addressing\n");
		}

		if ((cfg1->PrefetchMemBase & 0x0f) <= (u_int)0x01)
		    switch (cfg1->PrefetchMemLimit & 0x0f)
		    {
			case 0x00:		/* 16 bit addressing */
			    fprintf(out, "-0x%.4x\n", 
				(cfg1->PrefetchMemLimit == 0)
				    ? 0xffff
				    : ((cfg1->PrefetchMemLimit & 0xfff0) |
								    0x000f));
			    break;

			case 0x01:		/* 32 bit addressing */
			    fprintf(out, "-0x%.8lx\n", 
				((cfg1->PrefetchMemLimit |
				  cfg1->PrefetchLimitUpper) == 0)
				    ? 0xffffffff
				    : (u_long)(cfg1->PrefetchLimitUpper << 16) |
				        (cfg1->PrefetchMemLimit & 0xfff0) |
				        0x000f);
			    break;

			default:
			    fprintf(out, ", Undefined addressing\n");
		    }

		if (verbose || ((cfg1->PrefetchMemBase & 0x0f) > (u_int)0x01))
		    fprintf(out, "\tPrefBaseUpper:\t0x%.4x\n",
						    cfg1->PrefetchBaseUpper);

		if (verbose || ((cfg1->PrefetchMemLimit & 0x0f) > (u_int)0x01))
		    fprintf(out, "\tPrefMemLimit:\t0x%.4x\n"
				 "\tPrefLimitUpper:\t0x%.4x\n",
						    cfg1->PrefetchMemLimit,
						    cfg1->PrefetchLimitUpper);
	    }

	    fprintf(out, "\tSecStatus:\t0x%.4x\n", cfg1->SecStatus);

	    show_ROMbase(out, cfg1->ROMbaseAddress);
	    show_irq(out, cfg1->InterruptLine, cfg1->InterruptPin);

	    fprintf(out, "\tBridgeControl:\t0x%.4x\n", cfg1->BridgeControl);
	}

#ifdef ERATTA_DEBUG
	/*
	 * There is a workarropund in the PCI driver for an
	 * 82454 PCI bridge bug.  The following code detects the
	 * 82454 and flags the state of the inbound posting. 
	 * This code is for driver debug only.
	 */

	if ((ph.info.busnum == 0) &&
	    (cfg->VendorId == 0x8086) &&	/* Intel */
	    (cfg->DeviceId == 0x84c4) &&	/* 82454 PCI bridge */
	    (cfg->RevId <= 4))			/* B0 stepping or below */
	{
	    pci_configdata_t	b_cfg;
	    u_char		PciReg;

	    b_cfg.info =	ph.info;
	    b_cfg.reg =		0x54;		/* PciReg */
	    b_cfg.size =	1;		/* u_char */
	    b_cfg.data =	0;

	    if ((ioctl(PciFd, PCI_READ_CONFIG, &b_cfg) != -1) &&
		(b_cfg.status == 1))
	    {
		PciReg = (u_char)b_cfg.data;
		fprintf(out,
			"\tPciReg:\t\t0x%.2x        %s, Posting %s <-* * * *\n",
					PciReg,
					(ph.info.slotnum == (0xc8 >> 3))
					    ? "c82454" : "nc82454",
					(PciReg & 0x01) ? "TRUE" : "FALSE");
	    }
	}
#endif
    }
}

typedef struct pci_bios32_s
{
    union
    {
	u_char	byte[4];
	u_long	dword;
    } sig;			/* [0x00] */
    int		(*entry)();	/* [0x04] */
    u_char	rev;		/* [0x08] */
    u_char	len;		/* [0x09] */
    u_char	chksum;		/* [0x0a] */
    u_char	reserved[5];	/* [0x0b] */
} pci_bios32_t;

static void
show_bios32(FILE *out)
{
    pci_bios32_t	bios32;
    u_long		paddr;
    static const u_long	scanStart = 0e0000;
    static const u_long	scanEnd = 0x0fffff;
    static const u_long	bios32Sig = 0x5f32335f;	/* "_32_" */
    static const size_t	bios32len = (sizeof(bios32) + 15) / 16;
    u_long		bios32_entry;


    if (!verbose)
	return;

    for (paddr = scanStart; paddr < scanEnd; paddr += 4)
    {
	register u_char	sum;
	register char	*tp;

	if (read_mem(paddr, &bios32, sizeof(bios32)) != sizeof(bios32))
	    continue;

	if ((bios32.sig.dword != bios32Sig) || (bios32.len < bios32len))
	    continue;

	sum = 0;
	for (tp = (char *)&bios32; tp < (char *)&bios32 + sizeof(bios32); ++tp)
	    sum += *tp;

	if (sum == 0)
	    break;	/* Found it */
    }

    if (paddr >= scanEnd)
	return;

    fprintf(out, "\n    BIOS32:\t0x%.6lx\n", paddr);
    fprintf(out, "\tsignature:\t\"%c%c%c%c\"\n",
					bios32.sig.byte[0],
					bios32.sig.byte[1],
					bios32.sig.byte[2],
					bios32.sig.byte[3]);
    fprintf(out, "\tSvc Dir entry:\t0x%.6lx\n",	bios32.entry);
    fprintf(out, "\trev:\t\t%d\n",		(u_int)bios32.rev);
    fprintf(out, "\tlen:\t\t%d 16-byte unit\n",	(u_int)bios32.len);
    fprintf(out, "\tchksum:\t\t0x%.2x\n",	(u_int)bios32.chksum);

    if (!read_kvar_d("bios32_entry", &bios32_entry) && bios32_entry)
	fprintf(out, "\n\tBIOS entry:\t0x%.8lx\n", bios32_entry);
}

static void
show_base_addr(FILE *out, const u_long *BaseAddr, int cnt)
{
    int			n;
    const char		*tp;

    for (n = 0; n < cnt; ++n)
	if (BaseAddr[n] || verbose)
	{
	    fprintf(out, "\tBaseAddr[%d]:\t", n);
	    if (verbose)
		fprintf(out, "0x%.8x", BaseAddr[n]);

	    if (BaseAddr[n])
	    {
		if (BaseAddr[n] & 0x01)
		{
		    if (verbose)
			fprintf(out, "  I/O 0x%.4x", BaseAddr[n] & ~0x03);
		    else
			fprintf(out, "I/O         0x%.4x", BaseAddr[n] & ~0x03);
		}
		else
		{
		    static const char	*mem_loc[] =
		    {
			"32 bit space",	/* 00 */
			"below 1 Meg",	/* 01 */
			"64 bit space",	/* 10 */
			NULL,		/* 11 */
		    };

		    if (verbose)
			fprintf(out, "  Memory 0x%.4x", BaseAddr[n] & ~0x0f);
		    else
			fprintf(out, "Memory      0x%.4x", BaseAddr[n] & ~0x0f);

		    tp = mem_loc[(BaseAddr[n] >> 1) & 0x03];
		    if (tp != NULL)
			fprintf(out, ", %s", tp);

		    if (BaseAddr[n] & 0x80)
			fprintf(out, ", prefetchable");
		}
	    }

	    fprintf(out, "\n");
	}
}

static void
show_irq(FILE *out, u_char InterruptLine, u_char InterruptPin)
{
    if ((InterruptPin != 0) || verbose)
    {
	fprintf(out, "\tInterruptLine:\t");

	if (verbose)
	    fprintf(out, "0x%.2x        ", InterruptLine);

	if ((InterruptLine == 0xff) ||
	    ((InterruptLine == 0x00) &&
	     (InterruptPin == 0x00)))
	    fprintf(out, "Unused\n");
	else
	    fprintf(out, "IRQ-%d\n", InterruptLine);

	fprintf(out, "\tInterruptPin:\t");

	if (verbose)
	    fprintf(out, "0x%.2x        ", InterruptPin);

	switch (InterruptPin)
	{
	    case 0x00:
		fprintf(out, "Unused");
		break;

	    case 0x01:
	    case 0x02:
	    case 0x03:
	    case 0x04:
		fprintf(out, "INT%c", 'A' - 1 + InterruptPin);
		break;

	    default:
		if (verbose)
		    fprintf(out, "0x%.2x        ", InterruptPin);
		fprintf(out, "INVALID VALUE");
	}
	fprintf(out, "\n");
    }
}

static void
show_ROMbase(FILE *out, u_long ROMbaseAddress)
{
    if (ROMbaseAddress || verbose)
    {
	fprintf(out, "\tROMbaseAddress:\t");
	if (verbose)
	    fprintf(out, "0x%.8x  ", ROMbaseAddress);
	fprintf(out, "Expan ROM   0x%.8x, ", ROMbaseAddress & ~0x7ff);
	if (ROMbaseAddress & 0x01)
	    fprintf(out, "%s\n", Enabled);
	else
	    fprintf(out, "%s\n", Disabled);
    }
}

static int
open_pci()
{
    return open("/dev/pci", O_RDONLY);
}

static const pci_busdata_t *
get_bus_present()
{
    static pci_busdata_t	pres;


    if (((PciFd == -1) && ((PciFd = open_pci()) == -1)) ||
	(ioctl(PciFd, PCI_BUS_PRESENT, &pres) == -1))
	    return NULL;

    /*
     * Patch for the everest ../boot/boot/bootsup.s bug that didn't
     * mask off the high order bytes...when fixed boots get out
     * there (this change is marked "L023" in bootsup.s), then
     * this can be removed.  But Beta releases did not have this fix.
     */

    pres.businfo.numbuses &= 0x00ff;	/* bug: mask off high byte */
    pres.businfo.mechanism &= 0x00ff;	/* bug: mask off high byte */

    return &pres;
}

static const pci_cfg_space_t *
get_pci_config(pci_headerinfo_t *ph)
{
    pci_configdata_t		cfg;
    static union
    {
	u_short		words[sizeof(pci_cfg_space_t)/sizeof(u_short)];
	u_long		dwords[sizeof(pci_cfg_space_t)/sizeof(u_long)];
	pci_cfg_space_t	data;
    } hdr;
    u_short	reg;


    if ((PciFd == -1) && ((PciFd = open_pci()) == -1))
    {
	error_print("Cannot open pci");
	return NULL;
    }

    /*
     * Read the vendor ID to determine if we have a device here.
     * Reading a dword here could result in faulty information.
     */

    cfg.info =	ph->info;
    cfg.reg =	0;		/* Vendor ID */
    cfg.size =	sizeof(u_short);
    cfg.data =	0;		/* Insure that value is valid */

    if ((ioctl(PciFd, PCI_READ_CONFIG, &cfg) == -1) ||
	(cfg.status != 1) ||			/* An error */
	((u_short)cfg.data == 0xffff))		/* Device not present */
	    return NULL;

    hdr.data.VendorId = (u_short)cfg.data;

    /*
     * Read the other half of the forst dword to get in sync with dwords.
     */

    cfg.info =	ph->info;
    cfg.reg =	2;		/* DeviceId */
    cfg.size =	sizeof(u_short);
    cfg.data =	0;		/* Insure that value is valid */

    if ((ioctl(PciFd, PCI_READ_CONFIG, &cfg) == -1) ||
	(cfg.status != 1))			/* An error */
    {
	error_print("Cannot read ConfigSpace");
	return NULL;
    }

    hdr.data.DeviceId = (u_short)cfg.data;

    /*
     * Read the remainder of the data as dwords, for speed.
     */

    for (reg = 1; reg < sizeof(hdr.dwords)/sizeof(*hdr.dwords); ++reg)
    {
	cfg.info =	ph->info;
	cfg.reg =	reg * sizeof(u_long);
	cfg.size =	sizeof(u_long);
	cfg.data =	0;		/* Insure that value is valid */

	if ((ioctl(PciFd, PCI_READ_CONFIG, &cfg) == -1) ||
	    (cfg.status != 1))			/* An error */
	{
	    error_print("Cannot read ConfigSpace");
	    return NULL;
	}

	hdr.dwords[reg] = cfg.data;
    }

    return &hdr.data;
}

static const char *
pci_vendor_name(u_short vendor_id, u_short device_id, int rev)
{
    pci_prod_t	*pp;
    pci_vend_t	*vp;


    if (!vendor_id || (vendor_id == 0xffff) || !pci_vendors)
	return NULL;

    /*
     * First pass, rev is significant.
     * However if rev == -1, we have no rev.  In fact if there
     * are bits used in rev that are nor available to a u_char,
     * we do not have a valid rev.
     */

    if (rev & ~0xff)
	for (vp = pci_vendors; vp; vp = vp->next)
	    if (vendor_id == vp->id)
	    {
		for (pp = vp->prod; pp; pp = pp->next)
		    if ((pp->id == device_id) && (pp->rev == (u_char)rev))
			return vp->name;

		/* Look for another vendor with this ID */
	    }

    /*
     * Second pass, rev is not significant.
     */

    for (vp = pci_vendors; vp; vp = vp->next)
	if (vendor_id == vp->id)
	{
	    for (pp = vp->prod; pp; pp = pp->next)
		if (pp->id == device_id)
		    return vp->name;

	    /* Look for another vendor with this ID */
	}

    /*
     * Third pass, device and rev are not significant as long as
     * there is only one vendor using this ID.
     */

    for (vp = pci_vendors; vp; vp = vp->next)
	if (vendor_id == vp->id)
	{
	    pci_vend_t	*nvp;

	    for (nvp = vp->next; nvp; nvp = nvp->next)
		if (vendor_id == nvp->id)
		    return NULL;	/* More than one vendor for this ID */

	    return vp->name;
	}

    return NULL;	/* Not known */
}

static const char *
pci_device_name(u_short vendor_id, u_short device_id, int rev)
{
    pci_prod_t	*pp;
    pci_vend_t	*vp;


    if (!vendor_id || (vendor_id == 0xffff) || !pci_vendors)
	return NULL;

    /*
     * First pass, rev is significant.
     * However if rev == -1, we have no rev.  In fact if there
     * are bits used in rev that are nor available to a u_char,
     * we do not have a valid rev.
     */

    if (rev & ~0xff)
	for (vp = pci_vendors; vp; vp = vp->next)
	    if (vendor_id == vp->id)
	    {
		for (pp = vp->prod; pp; pp = pp->next)
		    if ((pp->id == device_id) && (pp->rev == (u_char)rev))
			return pp->name;

		/* Look for another vendor with this ID */
	    }

    /*
     * Second pass, rev is not significant.
     */

    for (vp = pci_vendors; vp; vp = vp->next)
	if (vendor_id == vp->id)
	{
	    for (pp = vp->prod; pp; pp = pp->next)
		if (pp->id == device_id)
		    return pp->name;

	    /* Look for another vendor with this ID */
	}

    return NULL;	/* Not known */
}

/*
 * Class descriptions are sparce arrays of descriptions.
 * The most specific reply is returned.
 */

static const char *
pci_class_descr(u_char header_type, u_char base_class,
				u_char subclass, u_char class_interface)
{
    const pci_class_t	*cp;

    switch (header_type & 0x7f)
    {
	case 0x00:		/* Standard */
	case 0x01:		/* PCI-to-PCI bridge */
	    cp = pci_class_0;
	    break;

	default:		/* Reserved encodings */
	    return NULL;
    }

    /*
     * Look up the class
     */

    for ( ; cp->class_descr; ++cp)
	if (base_class == cp->class_id)
	{
	    const pci_subclass_t	*scp;

	    if ((scp = cp->subclass) != NULL)
		for ( ; scp->interface; ++scp)
		    if (scp->subclass_id == subclass)
		    {
			const pci_interf_t	*ifp;

			if (scp->func)
			    return scp->func(class_interface);

			if ((ifp = scp->interface) != NULL)
			{
			    for ( ; ifp->descr; ++ifp)
				if (ifp->intf_id == class_interface)
				    return ifp->descr;

			    break;
			}

			break;
		    }

	    return cp->class_descr;
	}

    return NULL;
}

static const char *
pci_intf_01_01_func(u_char intf)
{
    static char	func_buf[128];

    if (intf & 0x80)
	strcpy(func_buf, "Master");
    else
	strcpy(func_buf, "Slave");

    strcat(func_buf, " IDE Device");

#ifdef NOT_YET	/* ## */
    if (intf & 0x10)
	strcat(func_buf, "#");
    else
	strcat(func_buf, "#");

    if (intf & 0x10)
	strcat(func_buf, "#");
    else
	strcat(func_buf, "#");

#endif

    return func_buf;
}

