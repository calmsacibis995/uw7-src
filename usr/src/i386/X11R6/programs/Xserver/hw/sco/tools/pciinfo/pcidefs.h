/*
 *	@(#)pcidefs.h	11.1
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *	SCO MODIFICATION HISTORY
 *
 * S002, 08-Aug-94, rogerv
 *	Added configuration space offsets needed in order to use everest
 *	pci bus funtionality in tbird and everest.  Everest kernel pci
 *      bus stuff reads one item at a time from configuration space per
 *	ioctl call.
 * S001, 19-Feb-94, staceyc
 * 	fixed config structure layout
 * S000, 14-Feb-94, staceyc
 * 	created
 */

#ifndef PCIDEFS_H
#define PCIDEFS_H

#define TRUE 1
#define FALSE 0

#define PCI_FUNCTION_ID         0xB1
#define PCI_BIOS_PRESENT        0x01
#define FIND_PCI_DEVICE         0x02
#define FIND_PCI_CLASS_CODE     0x03
#define GENERATE_SPECIAL_CYCLE  0x06
#define READ_CONFIG_BYTE        0x08
#define READ_CONFIG_WORD        0x09
#define READ_CONFIG_DWORD       0x0A

#define PCI_VENDOR_DEVICE_ID_OFFSET	0x0
#define PCI_STATUS_COMMAND_OFFSET	0x04
#define PCI_REV_CLASS_OFFSET		0x08
#define PCI_CACHE_LINE_ETC_OFFSET 	0x0c
#define PCI_BASE_REGISTERS_OFFSET	0x10
#define PCI_EXP_ROM_BASE_ADDR_OFFSET	0x30
#define PCI_INTERRUPT_LINE_ETC_OFFSET	0x3c

#define PCI_SUCCESSFUL          0x00
#define PCI_FUNC_NOT_SUPPORTED  0x81
#define PCI_BAD_VENDOR_ID       0x83
#define PCI_DEVICE_NOT_FOUND    0x86
#define PCI_BAD_REGISTER_NUMBER 0x87

#define BASE_ADDRESS_DWORDS 6

typedef struct pci_config_header {
	unsigned short vendor_id;
	unsigned short device_id;
	unsigned short command;
	unsigned short status;
	unsigned long class_revision;
	unsigned char cache_line_size, latency_timer, header_type, bist;
	unsigned long base_address[BASE_ADDRESS_DWORDS];
	unsigned long reserved1;
	unsigned long reserved2;
	unsigned long exp_rom_base_addr;
	unsigned long reserved3;
	unsigned long reserved4;
	unsigned char interrupt_line, interrupt_pin, min_gnt, max_lat;
	} pci_config_header;

#endif
