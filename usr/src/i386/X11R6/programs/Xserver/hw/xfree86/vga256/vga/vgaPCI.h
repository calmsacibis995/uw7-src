/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/vga/vgaPCI.h,v 3.1 1996/01/13 12:22:28 dawes Exp $ */
/*
 * PCI Probe
 *
 * Copyright 1995  The XFree86 Project, Inc.
 *
 * A lot of this comes from Robin Cutshaw's scanpci
 *
 */
/* $XConsortium: vgaPCI.h /main/2 1996/01/13 13:15:15 kaleb $ */

#ifndef _VGA_PCI_H
#define _VGA_PCI_H

#include "xf86_PCI.h"

#define PCI_VENDOR_NCR_1	0x1000
#define PCI_VENDOR_ATI		0x1002
#define PCI_VENDOR_AVANCE	0x1005
#define PCI_VENDOR_TSENG	0x100C
#define PCI_VENDOR_WEITEK	0x100E
#define PCI_VENDOR_DIGITAL	0x1011
#define PCI_VENDOR_CIRRUS	0x1013
#define PCI_VENDOR_NCR_2	0x101A
#define PCI_VENDOR_TRIDENT	0x1023
#define PCI_VENDOR_MATROX	0x102B
#define PCI_VENDOR_CHIPSTECH	0x102C
#define PCI_VENDOR_SIS		0x1039
#define PCI_VENDOR_NUMNINE	0x105D
#define PCI_VENDOR_UMC		0x1060
#define PCI_VENDOR_S3		0x5333
#define PCI_VENDOR_ARK		0xEDD8


/* ATI */
#define PCI_CHIP_MACH32		0x4158
#define PCI_CHIP_MACH64GX	0x4758
#define PCI_CHIP_MACH64CX	0x4358
#define PCI_CHIP_MACH64CT	0x4354
#define PCI_CHIP_MACH64ET	0x4554

/* Avance Logic */
#define PCI_CHIP_ALG2301	0x2301

/* Tseng */
#define PCI_CHIP_ET4000_W32P_A	0x3202
#define PCI_CHIP_ET4000_W32P_B	0x3205
#define PCI_CHIP_ET4000_W32P_C	0x3206
#define PCI_CHIP_ET4000_W32P_D	0x3207

/* Weitek */
#define PCI_CHIP_P9000		0x9001
#define PCI_CHIP_P9100		0x9100

/* Cirrus Logic */
#define PCI_CHIP_GD5430		0x00A0
#define PCI_CHIP_GD5434_4	0x00A4
#define PCI_CHIP_GD5434_8	0x00A8
#define PCI_CHIP_GD5436		0x00AC
#define PCI_CHIP_GD7542		0x1200

/* Trident */
#define PCI_CHIP_9420		0x9420
#define PCI_CHIP_9440		0x9440
#define PCI_CHIP_9660		0x9660
#define PCI_CHIP_9680		0x9680
#define PCI_CHIP_9682		0x9682

/* Chips & Tech */
#define PCI_CHIP_65545		0x00D8

/* SiS */
#define PCI_CHIP_SG86C201	0x0001

/* Number Nine */
#define PCI_CHIP_I128		0x2309

/* S3 */
#define PCI_CHIP_TRIO		0x8811
#define PCI_CHIP_868		0x8880
#define PCI_CHIP_928		0x88B0
#define PCI_CHIP_864_0		0x88C0
#define PCI_CHIP_864_1		0x88C1
#define PCI_CHIP_964_0		0x88D0
#define PCI_CHIP_964_1		0x88D1
#define PCI_CHIP_968		0x88F0

/* ARK Logic */
#define PCI_CHIP_1000PV		0xA091
#define PCI_CHIP_2000PV		0xA099

/* Increase this as required */
#define MAX_DEV_PER_VENDOR 16

typedef struct vgaPCIInformation {
    int Vendor;
    int ChipType;
    int ChipRev;
    unsigned long MemBase;
    unsigned long IOBase;
    struct pci_config_reg *PCIPtr;
} vgaPCIInformation;

extern vgaPCIInformation *vgaPCIInfo;

typedef struct pciVendorDeviceInfo {
    unsigned short VendorID;
    char *VendorName;
    struct pciDevice {
	unsigned short DeviceID;
	char *DeviceName;
    } Device[MAX_DEV_PER_VENDOR];
} pciVendorDeviceInfo;

extern pciVendorDeviceInfo xf86PCIVendorInfo[];

extern vgaPCIInformation *vgaGetPCIInfo(
#if NeedFunctionPrototypes
    void
#endif
);
   
#ifdef INIT_PCI_VENDOR_INFO
pciVendorDeviceInfo xf86PCIVendorInfo[] = {
    {PCI_VENDOR_NCR_1,	"NCR",	{
				{0x0000,		NULL}}},
    {PCI_VENDOR_ATI,	"ATI",	{
				{PCI_CHIP_MACH32,	"Mach32"},
				{PCI_CHIP_MACH64GX,	"Mach64 GX"},
				{PCI_CHIP_MACH64CX,	"Mach64 CX"},
				{PCI_CHIP_MACH64CT,	"Mach64 CT"},
				{PCI_CHIP_MACH64ET,	"Mach64 ET"},
				{0x0000,		NULL}}},
    {PCI_VENDOR_AVANCE,	"Avance Logic",	{
				{PCI_CHIP_ALG2301,	"ALG2301"},
				{0x0000,		NULL}}},
    {PCI_VENDOR_TSENG,	"Tseng Labs", {
				{PCI_CHIP_ET4000_W32P_A, "ET4000W32P revA"},
				{PCI_CHIP_ET4000_W32P_B, "ET4000W32P revB"},
				{PCI_CHIP_ET4000_W32P_C, "ET4000W32P revC"},
				{PCI_CHIP_ET4000_W32P_D, "ET4000W32P revD"},
				{0x0000,		NULL}}},
    {PCI_VENDOR_WEITEK,	"Weitek", {
				{PCI_CHIP_P9000,	"P9000"},
				{PCI_CHIP_P9100,	"P9100"},
				{0x0000,		NULL}}},
    {PCI_VENDOR_DIGITAL, "Digital", {
				{0x0000,		NULL}}},
    {PCI_VENDOR_CIRRUS,	"Cirrus Logic", {
				{PCI_CHIP_GD5430,	"GD5430"},
				{PCI_CHIP_GD5434_4,	"GD5434"},
				{PCI_CHIP_GD5434_8,	"GD5434"},
				{PCI_CHIP_GD5436,	"GD5436"},
				{PCI_CHIP_GD7542,	"GD7542"},
				{0x0000,		NULL}}},
    {PCI_VENDOR_NCR_2,	"NCR",	{
				{0x0000,		NULL}}},
    {PCI_VENDOR_TRIDENT, "Trident", {
				{PCI_CHIP_9420,		"TGUI 9420"},
				{PCI_CHIP_9440,		"TGUI 9440"},
				{PCI_CHIP_9660,		"TGUI 9660"},
				{PCI_CHIP_9680,		"TGUI 9680"},
				{PCI_CHIP_9682,		"TGUI 9682"},
				{0x0000,		NULL}}},
    {PCI_VENDOR_MATROX,	"Matrox", {
				{0x0000,		NULL}}},
    {PCI_VENDOR_CHIPSTECH, "C&T", {
				{PCI_CHIP_65545,	"65545"},
				{0x0000,		NULL}}},
    {PCI_VENDOR_SIS,	"SiS",	{
				{PCI_CHIP_SG86C201,	"SG86C201"},
				{0x0000,		NULL}}},
    {PCI_VENDOR_NUMNINE, "Number Nine", {
				{PCI_CHIP_I128,		"Imagine 128"},
				{0x0000,		NULL}}},
    {PCI_VENDOR_UMC,	"UMC",	{
				{0x0000,		NULL}}},
    {PCI_VENDOR_S3,	"S3",	{
				{PCI_CHIP_TRIO,		"Trio32/64"},
				{PCI_CHIP_868,		"868"},
				{PCI_CHIP_928,		"928"},
				{PCI_CHIP_864_0,	"864"},
				{PCI_CHIP_864_1,	"864"},
				{PCI_CHIP_964_0,	"964"},
				{PCI_CHIP_964_1,	"964"},
				{PCI_CHIP_968,		"968"},
				{0x0000,		NULL}}},
    {PCI_VENDOR_ARK,	"ARK Logic", {
				{PCI_CHIP_1000PV,	"1000PV"},
				{PCI_CHIP_2000PV,	"2000PV"},
				{0x0000,		NULL}}},
    {0x0000,		NULL,	{
				{0x0000,		NULL}}},
};
#endif

#endif /* VGA_PCI_H */
