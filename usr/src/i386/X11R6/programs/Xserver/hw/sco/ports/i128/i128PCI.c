/*
 * 	@(#) i128PCI.c 11.1 97/10/22
 *
 *      Copyright (C) The Santa Cruz Operation, 1991-1994.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation, and should be treated as Confidential.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 *
 * Modification History
 *
 * S000, 12-May-95, kylec@sco.com
 * 	created
 */


#include <fcntl.h>
#ifdef usl
#include "pcix.h"
#else
#include <sys/pci.h>
#endif
#include "i128PCI.h"

#ifdef usl
#define PCI_DEV "/dev/X/pci"
#else
#define PCI_DEV "/dev/pci"
#endif

typedef struct {
    int fd;
    struct pci_devinfo info;
} pciinfo;

void *
pci_open (int device_id,
          int vendor_id,
          int index)
{
    struct pci_devicedata device_data;
    pciinfo *pci = (pciinfo *)malloc(sizeof(pciinfo));

    if (pci == 0)
    {
        ErrorF("i128: out of memory\n");
        return 0;
    }
    
    if ((pci->fd = open(PCI_DEV, O_RDWR)) == -1)
    {
        ErrorF("i128: unable to open %s\n", PCI_DEV);
        free(pci);
        return 0;
    }
    
    device_data.vend_id = vendor_id;
    device_data.dev_id = device_id;
    device_data.index = index;
    
    if ((ioctl(pci->fd, PCI_FIND_DEVICE, &device_data) < 0) ||
        (device_data.device_present == 0))
    {
        free(pci);
        return 0;
    }    
    
    pci->info = device_data.info;
    return ((void*)pci);

}

int
pci_close (void *handle)
{

    pciinfo *pci = (pciinfo*)handle;

    if (!pci || pci->fd < 0)
        return PCI_ERROR;

    close (pci->fd);
    free(pci);

    return PCI_SUCCESS;
}

int
pci_write (void *handle,
           unsigned short offset,
           unsigned long data,
           int data_type)
{

    pciinfo *pci = (pciinfo*)handle;
    struct pci_configdata cfg_data;
     
    if (!pci)
        return PCI_ERROR;

    cfg_data.info = pci->info;

    switch (data_type)
    {
      case PCI_BYTE:
        cfg_data.size = sizeof(unsigned char);
        break;

      case PCI_WORD:
        cfg_data.size = sizeof(unsigned short);
        break;

      case PCI_DWORD:
      default:
        cfg_data.size = sizeof(unsigned long);
        break;
    }

    cfg_data.reg = offset;
    cfg_data.data = (unsigned long)data;
     
    if ((ioctl(pci->fd, PCI_WRITE_CONFIG, &cfg_data) < 0) ||
        (cfg_data.status == 0))
        return PCI_ERROR;

    return PCI_SUCCESS;

}


int
pci_read(void *handle,
         unsigned short offset,
         unsigned long *data,
         int data_type)
{

    pciinfo *pci = (pciinfo*)handle;
    struct pci_configdata cfg_data;

    *data = 0;

    if (!pci)
        return PCI_ERROR;

    cfg_data.info = pci->info;
    cfg_data.data = 0;

    switch (data_type)
    {
      case PCI_BYTE:
        cfg_data.size = sizeof(unsigned char);
        break;

      case PCI_WORD:
        cfg_data.size = sizeof(unsigned short);
        break;

      case PCI_DWORD:
      default:
        cfg_data.size = sizeof(unsigned long);
        break;
    }

    cfg_data.reg = offset;      /* in bytes */
     
    if ((ioctl(pci->fd, PCI_READ_CONFIG, &cfg_data) < 0) ||
        (cfg_data.status == 0))
    {
        return PCI_ERROR;
    }

    *data = (long)(cfg_data.data);
    return PCI_SUCCESS;

}


int pci_slot(void *handle)
{
    pciinfo *pci = (pciinfo*)handle;

    if (!pci)
        return PCI_ERROR;

    return pci->info.slotnum;
}
