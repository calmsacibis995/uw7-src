/*
 * @(#) pcislot.c 11.1 97/10/22
 *
 * Copyright (C) 1995 The Santa Cruz Operation, Inc.
 * Copyright (C) 1995 Compaq Computer Corp.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right
 * to use, modify, and incorporate this code into other products for purposes
 * authorized by the license agreement provided they include this notice
 * and the associated copyright notice with any such product.  The
 * information in this file is provided "AS IS" without warranty.
 *
 */

/*
 *	SCO MODIFICATION HISTORY
 *
 * S001, Wed Sep 18 19:13:23 GMT 1996, kylec
 * 	- use /dev/X/pci
 *	- include pcix.h
 *	- fix type problems
 * S000, 15 Dec 95, kylec
 * 	include device instance, sub-vendor id, and sub_device id in pcislot
 * 	output.
 */


#include <stdio.h>
#include <fcntl.h>

#include <pcix.h>

#ifdef	DEBUG
#define	debug_printf	printf
#else
#define	debug_printf
#endif

char *PCIDev	=	"/dev/X/pci";

#pragma	pack(1)
struct pci_config {
	unsigned short vend_id;
	unsigned short dev_id;
	unsigned short command;
	unsigned short status;
	unsigned rev_id:8;
	unsigned class:24;
	unsigned char cache_line_size;
	unsigned char latency;
	unsigned char header_type;
	unsigned char bist;
	unsigned long base_addr[6];
	unsigned long CIS_ptr;
	unsigned short sub_vend_id;
	unsigned short sub_dev_id;
	unsigned long ROM_addr;
	unsigned long reserved1;
	unsigned long reserved2;
	unsigned char intr_lat;
	unsigned char intr_pin;
	unsigned char min_gnt;
	unsigned char max_lat;
};
#pragma pack()

/*
 *	DevID is the Device ID, which is most likely an individual
 *		board, not necessarily the chip set.
 *	VenID is the Vendor ID, of the people who made the board,
 *		not necessarily the people who made the chipset
 *	RevID is the revision of the board.
 *	SubID is the Device ID of the actual chipset used on the board.
 *	SubVenID is the Vendor ID of the actual chipset used on the board.
 *
 *	if (RevID != 0xffff && RevID != board->RevID)
 *		goto next board.
 *	if (board->SubID == 0 || board->SubVenID == 0)
 *	{
 *		if (DevID != board->DevID)
 *			goto next board.
 *		if (VenID != board->VenID)
 *			goto next board.
 *	} else{
 *		if (DevID != board->SubID)
 *			goto next board.
 *		if (VenID != board->SubVenID)
 *			goto next board.
 *	}
 *	return found board.
 *
 *	If you call this multiple times, it will find the next board
 *	matching the DevID, VenID, and RevID.
 */

static int pci_fd = -1;

PCI_Find_Board( VenID, DevID, RevID, dev )
unsigned short DevID, VenID;
unsigned char RevID;
struct pci_devinfo **dev;
{
	static struct pci_headerinfo head;
	struct pci_busdata pci_busdata;
	struct pci_configdata cdata; 
	static int last_found = -1;
	static int last_vend = -1, last_dev = -1, last_rev = -1;
	int max_devs;
	int i, cnt;
	char config[sizeof(struct pci_config)];
	struct pci_config *pconf = (struct pci_config *)config;

	if (pci_fd == -1)
		pci_fd = open( PCIDev, O_RDONLY );
	if (pci_fd == -1)
	{
		perror( PCIDev );
		return -1;
	}

	if (ioctl( pci_fd, PCI_BUS_PRESENT, &pci_busdata ) == -1)
	{
		perror( "PCI_BUS_PRESENT" );
		return -1;
	}

	max_devs = pci_busdata.businfo.mechanism == 1 ? PCI_DEVS_MECH1 : 
		PCI_DEVS_MECH2;

	if (*dev == NULL || last_found >= max_devs)
	{
		debug_printf( "Reset due to maxdev\n" );
		last_found = -1;
	}

	if (last_vend != VenID || last_dev != DevID || last_rev != RevID)
	{
		debug_printf( "Reset stuff due to difference\n" );
		last_found = -1;
	}

	last_vend = VenID;
	last_dev = DevID;
	last_rev = RevID;

	for (i = (1+last_found); i < max_devs; i++)
	{
		head.vend_id = 0x0ffff;
		head.dev_id = 0x0ffff;
		head.base_class = 0x0ffff;
		head.sub_class = 0x0ffff;
		head.index = i;

		if (ioctl( pci_fd, PCI_SEARCH_BUS, &head ) == -1)
		{
			perror( "PCI_SEARCH_BUS" );
			return -1;
		}
		if (!head.device_present)
			continue;

		for (cnt = 0; cnt < sizeof(config); cnt += sizeof(long))
		{
			cdata.info = head.info;
			cdata.size = sizeof(long);
			cdata.reg = cnt;
			if (ioctl( pci_fd, PCI_READ_CONFIG, &cdata) == -1)
			{
				perror( "PCI_READ_CONFIG" );
				return -1;
			}
			*((long *)&config[cnt]) = cdata.data;
		}
		last_found = i;

		if (RevID != 0xff && RevID != pconf->rev_id)
		{
			debug_printf( "Revision fail (%x vs %x)\n",
				RevID, pconf->rev_id );
			continue;
		}
		if (pconf->sub_dev_id == 0 || pconf->sub_vend_id == 0)
		{
			if (DevID != 0xffff && pconf->dev_id != DevID)
			{
				debug_printf( "DevID fail (%x vs %x)\n",
					DevID,  pconf->dev_id  );
				continue;
			}
			if (VenID != 0xffff && pconf->vend_id != VenID)
			{
				debug_printf( "VenID Fail (%x vs %x)\n",
					VenID, pconf->vend_id );
				continue;
			}
		} else {
			if (DevID != 0xffff && pconf->sub_dev_id != DevID)
			{
				debug_printf( "SubDevID Fail (%x vs %x)\n",
					DevID, pconf->sub_dev_id  );
				continue;
			}
			if (VenID != 0xffff && pconf->sub_vend_id != VenID)
			{
				debug_printf( "SubVenID Fail (%x vs %x)\n",
					VenID, pconf->sub_vend_id );
				continue;
			}
		}

		*dev = &head.info;
		return 0;
	}
	return -1;
}

PCI_Read_Config( dev, offset, size, data )
struct pci_devinfo *dev;
int offset;
unsigned int size;
unsigned long *data;
{
	struct pci_configdata cdata; 
	if (offset >= 256 || size > sizeof(long))
		return -1;

	cdata.info = *dev;
	cdata.size = size;
	cdata.reg = offset;
        cdata.data = 0;
	if (ioctl( pci_fd, PCI_READ_CONFIG, &cdata) == -1)
	{
		perror( "PCI_READ_CONFIG" );
		return -1;
	}

	*data = cdata.data;
	return 1;
}

PCI_Write_Config( dev, offset, size, data )
struct pci_devinfo *dev;
int offset;
unsigned int size;
unsigned long *data;
{
	struct pci_configdata cdata; 
	if (offset >= 256 || size > sizeof(long))
		return -1;

	cdata.info = *dev;
	cdata.size = size;
	cdata.reg = offset;
	cdata.data = *data;
	if (ioctl( pci_fd, PCI_WRITE_CONFIG, &cdata) == -1)
	{
		perror( "PCI_WRITE_CONFIG" );
		return -1;
	}
	return 1;
}


PCI_Device_Instance( dev, vendor, device, revision, instance ) /* S000 */
struct pci_devinfo *dev;
unsigned short vendor, device;
unsigned char revision;
unsigned char *instance;
{
    struct pci_devicedata device_data;
    int index;

    if (pci_fd == -1)
        return pci_fd;

    device_data.vend_id = vendor;
    device_data.dev_id = device;

    for (index=0; index<0xFF; index++)
    {
        device_data.index = index;
    
        if ((ioctl(pci_fd, PCI_FIND_DEVICE, &device_data) < 0) ||
            (device_data.device_present == 0))
        {
            index = -1;
            break;
        }    
        if (dev->slotnum == device_data.info.slotnum)
            break;
    }
    *instance = (unsigned char)index;
    return index;
}


main()
{
	struct pci_devinfo *ptr = NULL;
	unsigned long vendor = 0, device = 0;
	unsigned long revision;
        unsigned long dev_instance;       /* S000 */
        unsigned long sub_v, sub_d; /* S000 */

	while ( PCI_Find_Board( -1, -1, -1, &ptr ) != -1)
	{
		printf( "%d %d %d ", ptr->funcnum, ptr->slotnum, ptr->busnum );
		PCI_Read_Config( ptr, 0, sizeof(short), &device );
		PCI_Read_Config( ptr, 2, sizeof(short), &vendor );
		PCI_Read_Config( ptr, 8, sizeof(char), &revision );
                PCI_Read_Config( ptr, 0x2C, sizeof(short), &sub_d ); /* S000 */
                PCI_Read_Config( ptr, 0x2E, sizeof(short), &sub_v ); /* S000 */
                PCI_Device_Instance( ptr, device, vendor, revision, &dev_instance ); 

		printf( "%.4x%.4x%.2x%.2x%.4x%.4x\n",
                       (unsigned short)device,
                       (unsigned short)vendor,
                       (unsigned char)revision,
                       (unsigned char)dev_instance,
                       (unsigned short)sub_d,
                       (unsigned short)sub_v );

	}
	exit(0);
}
