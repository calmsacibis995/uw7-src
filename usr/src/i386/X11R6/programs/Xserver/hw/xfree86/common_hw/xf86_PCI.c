/* $XFree86: xc/programs/Xserver/hw/xfree86/common_hw/xf86_PCI.c,v 3.4 1996/01/08 08:55:35 dawes Exp $ */
/*
 * Copyright 1995 by Robin Cutshaw <robin@XFree86.Org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the above listed copyright holder(s)
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  The above listed
 * copyright holder(s) make(s) no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM(S) ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE 
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY 
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER 
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: xf86_PCI.c /main/5 1996/01/10 10:21:13 kaleb $ */

/*#define DEBUGPCI  1 */

#include <stdio.h>
#include "os.h"
#include "compiler.h"
#include "xf86_PCI.h"

extern void xf86ClearIOPortList(int);
extern void xf86AddIOPorts(int, int, unsigned *);
extern void xf86EnableIOPorts(int);
extern void xf86DisableIOPorts(int);

struct pci_config_reg *pci_devp[MAX_PCI_DEVICES];


void
xf86scanpci()
{
    unsigned long tmplong1, tmplong2, config_cmd;
    unsigned char tmp1, tmp2;
    unsigned int i, j, idx = 0;
    struct pci_config_reg pcr;
    unsigned PCI_CtrlIOPorts[] = { 0xCF8, 0xCFA, 0xCFC };
    int Num_PCI_CtrlIOPorts = 3;
    unsigned PCI_DevIOPorts[16];
    int Num_PCI_DevIOPorts = 16;
    unsigned PCI_DevIOAddrPorts[16*16];
    int Num_PCI_DevIOAddrPorts = 16*16;

    for (i=0; i<16; i++) {
        PCI_DevIOPorts[i] = 0xC000 + (i*0x0100);
        for (j=0; j<16; j++)
            PCI_DevIOAddrPorts[(i*16)+j] = PCI_DevIOPorts[i] + (j*4);
    }

    xf86ClearIOPortList(0);
    xf86AddIOPorts(0, Num_PCI_CtrlIOPorts, PCI_CtrlIOPorts);
    xf86AddIOPorts(0, Num_PCI_DevIOPorts, PCI_DevIOPorts);
    xf86AddIOPorts(0, Num_PCI_DevIOAddrPorts, PCI_DevIOAddrPorts);

    /* Enable I/O access */
    xf86EnableIOPorts(0);

    outb(0xCF8, 0x00);
    outb(0xCFA, 0x00);
    tmp1 = inb(0xCF8);
    tmp2 = inb(0xCFA);
    if ((tmp1 == 0x00) && (tmp2 == 0x00)) {
	pcr._configtype = 2;
#ifdef DEBUGPCI
        printf("PCI says configuration type 2\n");
#endif
    } else {
        tmplong1 = inl(0xCF8);
        outl(0xCF8, PCI_EN);
        tmplong2 = inl(0xCF8);
        outl(0xCF8, tmplong1);
        if (tmplong2 == PCI_EN) {
	    pcr._configtype = 1;
#ifdef DEBUGPCI
            printf("PCI says configuration type 1\n");
#endif
	} else {
	    pcr._configtype = 0;
#ifdef DEBUGPCI
            printf("No PCI !\n");
#endif
            xf86DisableIOPorts(0);
            xf86ClearIOPortList(0);
	    return;
	}
    }

    /* Try pci config 1 probe first */

#ifdef DEBUGPCI
    printf("\nPCI probing configuration type 1\n");
#endif

    pcr._pcibuses[0] = 0;
    pcr._pcinumbus = 1;
    pcr._pcibusidx = 0;

#ifndef DEBUGPCI
    if (pcr._configtype == 1)
#endif
    do {
        for (pcr._cardnum = 0x0; pcr._cardnum < 0x20; pcr._cardnum += 0x1) {
	    config_cmd = PCI_EN | (pcr._pcibuses[pcr._pcibusidx]<<16) |
                                  (pcr._cardnum<<11);

            outl(0xCF8, config_cmd);         /* ioreg 0 */
            pcr._device_vendor = inl(0xCFC);

            if (pcr._vendor == 0xFFFF)   /* nothing there */
                continue;

#ifdef DEBUGPCI
	    printf("\npci bus 0x%x cardnum 0x%02x, vendor 0x%04x device 0x%04x\n",
	        pcr._pcibuses[pcr._pcibusidx], pcr._cardnum, pcr._vendor,
                pcr._device);
#endif

            outl(0xCF8, config_cmd | 0x04); pcr._status_command  = inl(0xCFC);
            outl(0xCF8, config_cmd | 0x08); pcr._class_revision  = inl(0xCFC);
            outl(0xCF8, config_cmd | 0x0C); pcr._bist_header_latency_cache
								= inl(0xCFC);
            outl(0xCF8, config_cmd | 0x10); pcr._base0  = inl(0xCFC);
            outl(0xCF8, config_cmd | 0x14); pcr._base1  = inl(0xCFC);
            outl(0xCF8, config_cmd | 0x18); pcr._base2  = inl(0xCFC);
            outl(0xCF8, config_cmd | 0x1C); pcr._base3  = inl(0xCFC);
            outl(0xCF8, config_cmd | 0x20); pcr._base4  = inl(0xCFC);
            outl(0xCF8, config_cmd | 0x24); pcr._base5  = inl(0xCFC);
            outl(0xCF8, config_cmd | 0x30); pcr._baserom = inl(0xCFC);
            outl(0xCF8, config_cmd | 0x3C); pcr._max_min_ipin_iline
								= inl(0xCFC);
            outl(0xCF8, config_cmd | 0x40); pcr._user_config = inl(0xCFC);

            /* check for pci-pci bridges (currently we only know Digital) */
            if ((pcr._vendor == 0x1011) && (pcr._device == 0x0001))
                if (pcr._secondary_bus_number > 0)
                    pcr._pcibuses[pcr._pcinumbus++] = pcr._secondary_bus_number;

	    if (idx >= MAX_PCI_DEVICES)
	        continue;

	    if ((pci_devp[idx] = (struct pci_config_reg *)xalloc(sizeof(
		 struct pci_config_reg))) == (struct pci_config_reg *)NULL) {
                outl(0xCF8, 0x00);
                xf86DisableIOPorts(0);
                xf86ClearIOPortList(0);
		return;
	    }

	    memcpy(pci_devp[idx++], &pcr, sizeof(struct pci_config_reg));
        }
    } while (++pcr._pcibusidx < pcr._pcinumbus);

#ifndef DEBUGPCI
    if (pcr._configtype == 1) {
        outl(0xCF8, 0x00);
	return;
    }
#endif
    /* Now try pci config 2 probe (deprecated) */

    outb(0xCF8, 0xF1);
    outb(0xCFA, 0x00); /* bus 0 for now */

#ifdef DEBUGPCI
    printf("\nPCI probing configuration type 2\n");
#endif

    pcr._pcibuses[0] = 0;
    pcr._pcinumbus = 1;
    pcr._pcibusidx = 0;

    do {
        for (pcr._ioaddr = 0xC000; pcr._ioaddr < 0xD000; pcr._ioaddr += 0x0100){
	    outb(0xCFA, pcr._pcibuses[pcr._pcibusidx]); /* bus 0 for now */
            pcr._device_vendor = inl(pcr._ioaddr);
	    outb(0xCFA, 0x00); /* bus 0 for now */

            if (pcr._vendor == 0xFFFF)   /* nothing there */
                continue;
	    /* opti chipsets that use config type 1 look like this on type 2 */
            if ((pcr._vendor == 0xFF00) && (pcr._device == 0xFFFF))
                continue;

#ifdef DEBUGPCI
	    printf("\npci bus 0x%x slot at 0x%04x, vendor 0x%04x device 0x%04x\n",
	        pcr._pcibuses[pcr._pcibusidx], pcr._ioaddr, pcr._vendor,
                pcr._device);
#endif

	    outb(0xCFA, pcr._pcibuses[pcr._pcibusidx]); /* bus 0 for now */
            pcr._status_command = inl(pcr._ioaddr + 0x04);
            pcr._class_revision = inl(pcr._ioaddr + 0x08);
            pcr._bist_header_latency_cache = inl(pcr._ioaddr + 0x0C);
            pcr._base0 = inl(pcr._ioaddr + 0x10);
            pcr._base1 = inl(pcr._ioaddr + 0x14);
            pcr._base2 = inl(pcr._ioaddr + 0x18);
            pcr._base3 = inl(pcr._ioaddr + 0x1C);
            pcr._base4 = inl(pcr._ioaddr + 0x20);
            pcr._base5 = inl(pcr._ioaddr + 0x24);
            pcr._baserom = inl(pcr._ioaddr + 0x30);
            pcr._max_min_ipin_iline = inl(pcr._ioaddr + 0x3C);
            pcr._user_config = inl(pcr._ioaddr + 0x40);
	    outb(0xCFA, 0x00); /* bus 0 for now */

            /* check for pci-pci bridges (currently we only know Digital) */
            if ((pcr._vendor == 0x1011) && (pcr._device == 0x0001))
                if (pcr._secondary_bus_number > 0)
                    pcr._pcibuses[pcr._pcinumbus++] = pcr._secondary_bus_number;

	    if (idx >= MAX_PCI_DEVICES)
	        continue;

	    if ((pci_devp[idx] = (struct pci_config_reg *)xalloc(sizeof(
		 struct pci_config_reg))) == (struct pci_config_reg *)NULL) {
                outb(0xCF8, 0x00);
                outb(0xCFA, 0x00);
                xf86DisableIOPorts(0);
                xf86ClearIOPortList(0);
		return;
	    }

	    memcpy(pci_devp[idx++], &pcr, sizeof(struct pci_config_reg));
	}
    } while (++pcr._pcibusidx < pcr._pcinumbus);

    outb(0xCF8, 0x00);
    outb(0xCFA, 0x00);

    xf86DisableIOPorts(0);
    xf86ClearIOPortList(0);
}

void
xf86writepci(cardnum, reg, mask, value)
    int cardnum;
    int reg;
    unsigned long mask;
    unsigned long value;
{
    unsigned char tmp1, tmp2;
    unsigned long tmplong1, tmplong2, tmp, config_cmd;
    unsigned int i, j;
    unsigned PCI_CtrlIOPorts[] = { 0xCF8, 0xCFA, 0xCFC };
    int configtype;
    int Num_PCI_CtrlIOPorts = 3;
    unsigned PCI_DevIOPorts[16];
    int Num_PCI_DevIOPorts = 16;
    unsigned PCI_DevIOAddrPorts[16*16];
    int Num_PCI_DevIOAddrPorts = 16*16;

    for (i=0; i<16; i++) {
        PCI_DevIOPorts[i] = 0xC000 + (i*0x0100);
        for (j=0; j<16; j++)
            PCI_DevIOAddrPorts[(i*16)+j] = PCI_DevIOPorts[i] + (j*4);
    }

    xf86ClearIOPortList(0);
    xf86AddIOPorts(0, Num_PCI_CtrlIOPorts, PCI_CtrlIOPorts);
    xf86AddIOPorts(0, Num_PCI_DevIOPorts, PCI_DevIOPorts);
    xf86AddIOPorts(0, Num_PCI_DevIOAddrPorts, PCI_DevIOAddrPorts);

    /* Enable I/O access */
    xf86EnableIOPorts(0);

    outb(0xCF8, 0x00);
    outb(0xCFA, 0x00);
    tmp1 = inb(0xCF8);
    tmp2 = inb(0xCFA);
    if ((tmp1 == 0x00) && (tmp2 == 0x00)) {
	configtype = 2;
#ifdef DEBUGPCI
        printf("PCI says configuration type 2\n");
#endif
    } else {
        tmplong1 = inl(0xCF8);
        outl(0xCF8, PCI_EN);
        tmplong2 = inl(0xCF8);
        outl(0xCF8, tmplong1);
        if (tmplong2 == PCI_EN) {
	    configtype = 1;
#ifdef DEBUGPCI
            printf("PCI says configuration type 1\n");
#endif
	} else {
	    configtype = 0;
#ifdef DEBUGPCI
            printf("No PCI !\n");
#endif
            xf86DisableIOPorts(0);
            xf86ClearIOPortList(0);
	    return;
	}
    }

    /* Try pci config 1 probe first */

#ifdef DEBUGPCI
    printf("\nPCI probing configuration type 1\n");
#endif

#ifndef DEBUGPCI
    if (configtype == 1)
#endif
    {
	config_cmd = PCI_EN | (0<<16) | (cardnum<<11);

        outl(0xCF8, config_cmd | reg);
	tmp = inl(0xCFC) & ~mask;
	outl(0xCFC, tmp | (value & mask));
    }
#ifndef DEBUGPCI
    if (configtype == 1) {
        outl(0xCF8, 0x00);
	return;
    }
#endif
    /* Now try pci config 2 probe (deprecated) */

    outb(0xCF8, 0xF1);
    outb(0xCFA, 0x00); /* bus 0 for now */

#ifdef DEBUGPCI
    printf("\nPCI probing configuration type 2\n");
#endif

    {
	int ioaddr = 0xC000 + (cardnum * 0x100);

	outb(0xCFA, 0x00); /* bus 0 for now */
        tmp = inl(ioaddr + reg) & ~mask;
	outb(0xCFA, 0x00); /* bus 0 for now */
        outl(ioaddr + reg, tmp | (value & mask));
	outb(0xCFA, 0x00); /* bus 0 for now */

    }

    outb(0xCF8, 0x00);
    outb(0xCFA, 0x00);

    xf86DisableIOPorts(0);
    xf86ClearIOPortList(0);
    return;
}
