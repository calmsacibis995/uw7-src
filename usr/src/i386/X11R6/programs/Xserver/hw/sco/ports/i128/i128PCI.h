/*
 * 	@(#) i128PCI.h 11.1 97/10/22
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

#ifndef _PCICONFIG_H
#define _PCICONFIG_H

enum pci_status { PCI_ERROR = -1, PCI_SUCCESS = 0, PCI_DEVICE_NOT_FOUND };
enum pci_type { PCI_BYTE = 0, PCI_WORD, PCI_DWORD };

void *
pci_open (int device_id,
          int vendor_id,
          int index);

int
pci_close (void *handle);

int
pci_write (void *handle,
           unsigned short offset,
           unsigned long data,
           int data_type);

int
pci_read(void *handle,
         unsigned short offset,
         unsigned long *data,
         int data_type);

#endif
