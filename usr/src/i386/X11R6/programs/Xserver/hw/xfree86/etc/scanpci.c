/* $XConsortium: scanpci.c /main/8 1996/01/30 15:20:27 kaleb $ */
/*
 *  name:             scanpci.c
 *
 *  purpose:          This program will scan for and print details of
 *                    devices on the PCI bus.

 *  author:           Robin Cutshaw (robin@xfree86.org)
 *
 *  supported O/S's:  SVR4, UnixWare, SCO, Solaris,
 *                    FreeBSD, NetBSD, 386BSD, BSDI BSD/386,
 *                    Linux, Mach/386,
 *                    DOS (WATCOM 9.5 compiler)
 *
 *  compiling:        [g]cc scanpci.c -o scanpci
 *                    for SVR4 (not Solaris), UnixWare use:
 *                        [g]cc -DSVR4 scanpci.c -o scanpci
 *                    for DOS, watcom 9.5:
 *                        wcc386p -zq -omaxet -7 -4s -s -w3 -d2 name.c
 *                        and link with PharLap or other dos extender for exe
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/etc/scanpci.c,v 3.7 1996/01/30 15:26:17 dawes Exp $ */

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

#if defined(__SVR4)
#if !defined(SVR4)
#define SVR4
#endif
#endif

#include <stdio.h>
#include <sys/types.h>
#if defined(SVR4)
#if defined(sun)
#define __EXTENSIONS__
#endif
#include <sys/proc.h>
#include <sys/tss.h>
#if defined(NCR)
#define __STDC
#include <sys/sysi86.h>
#undef __STDC
#else
#include <sys/sysi86.h>
#endif
#if defined(__SUNPRO_C) || defined(sun) || defined(__sun)
#include <sys/psw.h>
#else
#include <sys/seg.h>
#endif
#include <sys/v86.h>
#endif
#if defined(__FreeBSD__) || defined(__386BSD__)
#include <sys/file.h>
#include <machine/console.h>
#ifndef GCCUSESGAS
#define GCCUSESGAS
#endif
#endif
#if defined(__NetBSD__) 
#include <sys/param.h>
#include <sys/file.h>
#include <machine/sysarch.h>
#ifndef GCCUSEGAS
#define GCCUSEGAS
#endif
#endif
#if defined(__bsdi__)
#include <sys/file.h>
#include <sys/ioctl.h>
#include <i386/isa/pcconsioctl.h>
#ifndef GCCUSESGAS
#define GCCUSESGAS
#endif
#endif
#if defined(SCO)
#include <sys/console.h>
#include <sys/param.h> 
#include <sys/immu.h>
#include <sys/region.h>
#include <sys/proc.h>
#include <sys/tss.h> 
#include <sys/sysi86.h>
#include <sys/v86.h>
#endif
#if defined(Lynx_22)
#ifndef GCCUSESGAS
#define GCCUSESGAS
#endif
#endif


#if defined(__WATCOMC__)

#include <stdlib.h>
void outl(unsigned port, unsigned data);
#pragma aux outl =  "out    dx, eax" parm [dx] [eax];
void outb(unsigned port, unsigned data);
#pragma aux outb = "out    dx, al" parm [dx] [eax];
unsigned inl(unsigned port);
#pragma aux inl = "in     eax, dx" parm [dx];
unsigned inb(unsigned port);
#pragma aux inb = "xor    eax,eax" "in     al, dx" parm [dx];

#else /* __WATCOMC__ */

#if defined(__GNUC__)

#if defined(GCCUSESGAS)
#define OUTB_GCC "outb %0,%1"
#define OUTL_GCC "outl %0,%1"
#define INB_GCC  "inb %1,%0"
#define INL_GCC  "inl %1,%0"
#else
#define OUTB_GCC "out%B0 (%1)"
#define OUTL_GCC "out%L0 (%1)"
#define INB_GCC "in%B0 (%1)"
#define INL_GCC "in%L0 (%1)"
#endif /* GCCUSESGAS */

static void outb(unsigned short port, unsigned char val) {
     __asm__ __volatile__(OUTB_GCC : :"a" (val), "d" (port)); }
static void outl(unsigned short port, unsigned long val) {
     __asm__ __volatile__(OUTL_GCC : :"a" (val), "d" (port)); }
static unsigned char inb(unsigned short port) { unsigned char ret;
     __asm__ __volatile__(INB_GCC : "=a" (ret) : "d" (port)); return ret; }
static unsigned long inl(unsigned short port) { unsigned long ret;
     __asm__ __volatile__(INL_GCC : "=a" (ret) : "d" (port)); return ret; }

#else  /* __GNUC__ */

#if defined(__STDC__) && (__STDC__ == 1)
# if !defined(NCR)
#  define asm __asm
# endif
#endif

#if defined(__SUNPRO_C)
/*
 * This section is a gross hack in if you tell anyone that I wrote it,
 * I'll deny it.  :-)
 * The leave/ret instructions are the big hack to leave %eax alone on return.
 */
	unsigned char inb(int port) {
		asm("	movl 8(%esp),%edx");
		asm("	subl %eax,%eax");
		asm("	inb  (%dx)");
		asm("	leave");
		asm("	ret");
	}

	unsigned short inw(int port) {
		asm("	movl 8(%esp),%edx");
		asm("	subl %eax,%eax");
		asm("	inw  (%dx)");
		asm("	leave");
		asm("	ret");
	}

	unsigned long inl(int port) {
		asm("	movl 8(%esp),%edx");
		asm("	inl  (%dx)");
		asm("	leave");
		asm("	ret");
	}

	void outb(int port, unsigned char value) {
		asm("	movl 8(%esp),%edx");
		asm("	movl 12(%esp),%eax");
		asm("	outb (%dx)");
	}

	void outw(int port, unsigned short value) {
		asm("	movl 8(%esp),%edx");
		asm("	movl 12(%esp),%eax");
		asm("	outw (%dx)");
	}

	void outl(int port, unsigned long value) {
		asm("	movl 8(%esp),%edx");
		asm("	movl 12(%esp),%eax");
		asm("	outl (%dx)");
	}
#else

#if defined(SVR4)
# if !defined(__USLC__)
#  define __USLC__
# endif
#endif

#include <sys/inline.h>

#endif /* SUNPRO_C */

#endif /* __GNUC__ */
#endif /* __WATCOMC__ */


struct pci_config_reg {
    /* start of official PCI config space header */
    union {
        unsigned long device_vendor;
	struct {
	    unsigned short vendor;
	    unsigned short device;
	} dv;
    } dv_id;
#define _device_vendor dv_id.device_vendor
#define _vendor dv_id.dv.vendor
#define _device dv_id.dv.device
    union {
        unsigned long status_command;
	struct {
	    unsigned short command;
	    unsigned short status;
	} sc;
    } stat_cmd;
#define _status_command stat_cmd.status_command
#define _command stat_cmd.sc.command
#define _status  stat_cmd.sc.status
    union {
        unsigned long class_revision;
	struct {
	    unsigned char rev_id;
	    unsigned char prog_if;
	    unsigned char sub_class;
	    unsigned char base_class;
	} cr;
    } class_rev;
#define _class_revision class_rev.class_revision
#define _rev_id     class_rev.cr.rev_id
#define _prog_if    class_rev.cr.prog_if
#define _sub_class  class_rev.cr.sub_class
#define _base_class class_rev.cr.base_class
    union {
        unsigned long bist_header_latency_cache;
	struct {
	    unsigned char cache_line_size;
	    unsigned char latency_timer;
	    unsigned char header_type;
	    unsigned char bist;
	} bhlc;
    } bhlc;
#define _bist_header_latency_cache bhlc.bist_header_latency_cache
#define _cache_line_size bhlc.bhlc.cache_line_size
#define _latency_timer   bhlc.bhlc.latency_timer
#define _header_type     bhlc.bhlc.header_type
#define _bist            bhlc.bhlc.bist
    union {
	struct {
	    unsigned long dv_base0;
	    unsigned long dv_base1;
	    unsigned long dv_base2;
	    unsigned long dv_base3;
	    unsigned long dv_base4;
	    unsigned long dv_base5;
	} dv;
	struct {
	    unsigned long bg_rsrvd[2];
	    unsigned char primary_bus_number;
	    unsigned char secondary_bus_number;
	    unsigned char subordinate_bus_number;
	    unsigned char secondary_latency_timer;
	    unsigned char io_base;
	    unsigned char io_limit;
	    unsigned short secondary_status;
	    unsigned short mem_base;
	    unsigned short mem_limit;
	    unsigned short prefetch_mem_base;
	    unsigned short prefetch_mem_limit;
	} bg;
    } bc;
#define	_base0				bc.dv.dv_base0
#define	_base1				bc.dv.dv_base1
#define	_base2				bc.dv.dv_base2
#define	_base3				bc.dv.dv_base3
#define	_base4				bc.dv.dv_base4
#define	_base5				bc.dv.dv_base5
#define	_primary_bus_number		bc.bg.primary_bus_number
#define	_secondary_bus_number		bc.bg.secondary_bus_number
#define	_subordinate_bus_number		bc.bg.subordinate_bus_number
#define	_secondary_latency_timer	bc.bg.secondary_latency_timer
#define _io_base			bc.bg.io_base
#define _io_limit			bc.bg.io_limit
#define _secondary_status		bc.bg.secondary_status
#define _mem_base			bc.bg.mem_base
#define _mem_limit			bc.bg.mem_limit
#define _prefetch_mem_base		bc.bg.prefetch_mem_base
#define _prefetch_mem_limit		bc.bg.prefetch_mem_limit
    unsigned long rsvd1;
    unsigned long rsvd2;
    unsigned long _baserom;
    unsigned long rsvd3;
    unsigned long rsvd4;
    union {
        unsigned long max_min_ipin_iline;
	struct {
	    unsigned char int_line;
	    unsigned char int_pin;
	    unsigned char min_gnt;
	    unsigned char max_lat;
	} mmii;
    } mmii;
#define _max_min_ipin_iline mmii.max_min_ipin_iline
#define _int_line mmii.mmii.int_line
#define _int_pin  mmii.mmii.int_pin
#define _min_gnt  mmii.mmii.min_gnt
#define _max_lat  mmii.mmii.max_lat
    /* I don't know how accurate or standard this is (DHD) */
    union {
	unsigned long user_config;
	struct {
	    unsigned char user_config_0;
	    unsigned char user_config_1;
	    unsigned char user_config_2;
	    unsigned char user_config_3;
	} uc;
    } uc;
#define _user_config uc.user_config
#define _user_config_0 uc.uc.user_config_0
#define _user_config_1 uc.uc.user_config_1
#define _user_config_2 uc.uc.user_config_2
#define _user_config_3 uc.uc.user_config_3
    /* end of official PCI config space header */
    unsigned long _pcibusidx;
    unsigned long _pcinumbus;
    unsigned long _pcibuses[16];
    unsigned short _configtype;   /* config type found                   */
    unsigned short _ioaddr;       /* config type 1 - private I/O addr    */
    unsigned long _cardnum;       /* config type 2 - private card number */
};

extern void identify_card(struct pci_config_reg *);
extern void print_i128(struct pci_config_reg *);
extern void print_mach64(struct pci_config_reg *);
extern void print_pcibridge(struct pci_config_reg *);
extern void enable_os_io();
extern void disable_os_io();

#define MAX_DEV_PER_VENDOR 16
#define MAX_PCI_DEVICES    64
#define NF ((void (*)())NULL)

struct pci_vendor_device {
    unsigned short vendor_id;
    char *vendorname;
    struct pci_device {
        unsigned short device_id;
        char *devicename;
	void (*print_func)(struct pci_config_reg *);
    } device[MAX_DEV_PER_VENDOR];
} pvd[] = {
        { 0x1000, "NCR", {
                            { 0x0001, "53C810", NF },
                            { 0x0002, "53C820", NF },
                            { 0x0003, "53C825", NF },
                            { 0x0004, "53C815", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1002, "ATI", {
                            { 0x4158, "Mach32", NF },
                            { 0x4758, "Mach64 GX", print_mach64 },
                            { 0x4358, "Mach64 CX", print_mach64 },
                            { 0x4354, "Mach64 CT", print_mach64 },
                            { 0x4554, "Mach64 ET", print_mach64 },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1004, "VLSI", {
                            { 0x0005, "82C592-FC1", NF },
                            { 0x0006, "82C593-FC1", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1005, "Avance Logic", {
                            { 0x2301, "ALG2301", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x100B, "NS", {
                            { 0xD001, "87410", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x100C, "Tseng Labs", {
                            { 0x3202, "ET4000w32p rev A", NF },
                            { 0x3205, "ET4000w32p rev B", NF },
                            { 0x3206, "ET4000w32p rev C", NF },
                            { 0x3207, "ET4000w32p rev D", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x100E, "Weitek", {
                            { 0x9001, "P9000", NF },
                            { 0x9100, "P9100", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1011, "Digital", {
                            { 0x0001, "DC21050 PCI-PCI Bridge",print_pcibridge},
                            { 0x0002, "DC21040 10Mb/s Ethernet", NF },
                            { 0x0009, "DC21140 10/100 Mb/s Ethernet", NF },
                            { 0x0014, "DC21041 10Mb/s Ethernet Plus", NF },
                            { 0x000F, "DEFPA (FDDI PCI)", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1013, "Cirrus Logic", {
                            { 0x00A0, "GD 5430", NF },
                            { 0x00A4, "GD 5434-4", NF },
                            { 0x00A8, "GD 5434-8", NF },
                            { 0x00AC, "GD 5436", NF },
                            { 0x1100, "CL 6729", NF },
                            { 0x1200, "CL 7542", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x101A, "NCR", {
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1022, "AMD", {
                            { 0x2000, "79C970 Lance", NF },
                            { 0x2020, "53C974 SCSI", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1023, "Trident", {
                            { 0x9420, "TGUI 9420", NF },
                            { 0x9440, "TGUI 9440", NF },
                            { 0x9660, "TGUI 9660", NF },
                            { 0x9680, "TGUI 9680", NF },
                            { 0x9682, "TGUI 9682", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1025, "ALI", {
                            { 0x1435, "M1435", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x102B, "Matrox", {
                            { 0x0518, "MGA-2 Atlas PX2085", NF },
                            { 0x0D10, "MGA Impression", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x102C, "CT", {
                            { 0x00D8, "65545", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1036, "FD", {
                            { 0x0000, "TMC-18C30 (36C70)", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1039, "SIS", {
                            { 0x0001, "86C201", NF },
                            { 0x0008, "85C503", NF },
                            { 0x0406, "85C501", NF },
                            { 0x0496, "85C496", NF },
                            { 0x0601, "85C601", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x103c, "HP", {
                            { 0x1030, "J2585A", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1042, "SMC", {
                            { 0x1000, "FDC 37C665", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1044, "DPT", {
                            { 0xA400, "SmartCache/Raid", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1045, "Opti", {
                            { 0xC557, "82C557", NF },
                            { 0xC558, "82C558", NF },
                            { 0xC621, "82C621", NF },
                            { 0xC822, "82C822", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x104B, "BusLogic", {
                            { 0x0140, "946C 01", NF },
                            { 0x1040, "946C 10", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x105A, "Promise", {
                            { 0x5300, "DC5030", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x105D, "Number Nine", {
                            { 0x2309, "Imagine-128", print_i128 },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1060, "UMC", {
                            { 0x0101, "UM8673F", NF },
                            { 0x8881, "UM8881F", NF },
                            { 0x8886, "UM8886F", NF },
                            { 0x888A, "UM8886A", NF },
                            { 0x8891, "UM8891A", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1061, "X", {
                            { 0x0001, "ITT AGX016", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1077, "QLogic", {
                            { 0x1020, "ISP1020", NF },
                            { 0x1022, "ISP1022", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x107D, "Leadtek", {
                            { 0x0000, "S3 805", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1080, "Contaq", {
                            { 0x0600, "82C599", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1095, "CMD", {
                            { 0x0640, "640A", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1098, "Vision", {
                            { 0x0001, "QD 8500", NF },
                            { 0x0002, "QD 8580", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x10AA, "ACC", {
                            { 0x0000, (char *)NULL, NF } } },
        { 0x10AD, "Winbond", {
                            { 0x0001, "W83769F", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x10B7, "3COM", {
                            { 0x5900, "3C590 10bT", NF },
                            { 0x5950, "3C595 100bTX", NF },
                            { 0x5951, "3C595 100bT4", NF },
                            { 0x5952, "3C595 10b-MII", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x10B9, "ALI", {
                            { 0x1445, "M1445", NF },
                            { 0x1449, "M1449", NF },
                            { 0x1451, "M1451", NF },
                            { 0x5215, "M4803", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x10E0, "IMS", {
                            { 0x8849, "8849", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1106, "VIA", {
                            { 0x0505, "VT 82C505", NF },
                            { 0x0561, "VT 82C505", NF },
                            { 0x0576, "VT 82C576 3V", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1119, "Vortex", {
                            { 0x0001, "GDT 6000b", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x111A, "EF", {
                            { 0x0000, "155P-MF1", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1159, "Mutech", {
                            { 0x0001, "MV1000", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1193, "Zeinet", {
                            { 0x0001, "1221", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1C1C, "Symphony", {
                            { 0x0001, "82C101", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x1DE1, "Tekram", {
                            { 0xDC29, "DC290", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x5333, "S3", {
                            { 0x8811, "Trio32/64", NF },
                            { 0x8880, "868", NF },
                            { 0x88B0, "928", NF },
                            { 0x88C0, "864-0", NF },
                            { 0x88C1, "864-1", NF },
                            { 0x88D0, "964-0", NF },
                            { 0x88D1, "964-1", NF },
                            { 0x88F0, "968", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x8086, "Intel", {
                            { 0x0482, "82375EB pci-eisa bridge", NF },
                            { 0x0483, "82424ZX cache dram controller", NF },
                            { 0x0484, "82378IB pci-isa bridge", NF },
                            { 0x0486, "82430ZX Aries", NF },
                            { 0x04A3, "82434LX pci cache mem controller", NF },
                            { 0x1223, "SAA7116", NF },
                            { 0x122D, "82437 Triton", NF },
                            { 0x122E, "82471 Triton", NF },
                            { 0x1230, "82438", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x9004, "Adaptec", {
                            { 0x5078, "7850", NF },
                            { 0x7078, "294x", NF },
                            { 0x7178, "2940", NF },
                            { 0x7278, "7872", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x907F, "Atronics", {
                            { 0x2015, "IDE-2015PL", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0xEDD8, "ARK Logic", {
                            { 0xA091, "1000PV", NF },
                            { 0xA099, "2000PV", NF },
                            { 0x0000, (char *)NULL, NF } } },
        { 0x0000, (char *)NULL, {
                            { 0x0000, (char *)NULL, NF } } }
};

#define PCI_EN 0x80000000

main(int argc, unsigned char *argv[])
{
    unsigned long tmplong1, tmplong2, config_cmd;
    unsigned char tmp1, tmp2;
    unsigned int idx;
    struct pci_config_reg pcr;

    if (argc != 1) {
	printf("Usage: %s\n");
	exit(1);
    }
#if !defined(MSDOS)
    if (getuid()) {
	printf("This program must be run as root\n");
	exit(1);
    }
#endif

    enable_os_io();

    pcr._configtype = 0;

    outb(0xCF8, 0x00);
    outb(0xCFA, 0x00);
    tmp1 = inb(0xCF8);
    tmp2 = inb(0xCFA);
    if ((tmp1 == 0x00) && (tmp2 == 0x00)) {
	pcr._configtype = 2;
        printf("PCI says configuration type 2\n");
    } else {
        tmplong1 = inl(0xCF8);
        outl(0xCF8, PCI_EN);
        tmplong2 = inl(0xCF8);
        outl(0xCF8, tmplong1);
        if (tmplong2 == PCI_EN) {
	    pcr._configtype = 1;
            printf("PCI says configuration type 1\n");
	} else {
            printf("No PCI !\n");
	    disable_os_io();
	    exit(1);
	}
    }

    /* Try pci config 1 probe first */

    printf("\nPCI probing configuration type 1\n");

    pcr._ioaddr = 0xFFFF;

    pcr._pcibuses[0] = 0;
    pcr._pcinumbus = 1;
    pcr._pcibusidx = 0;
    idx = 0;

    do {
        for (pcr._cardnum = 0x0; pcr._cardnum < 0x20; pcr._cardnum += 0x1) {
	    config_cmd = PCI_EN | (pcr._pcibuses[pcr._pcibusidx]<<16) |
                                  (pcr._cardnum<<11);

            outl(0xCF8, config_cmd);         /* ioreg 0 */
            pcr._device_vendor = inl(0xCFC);

            if ((pcr._vendor == 0xFFFF) || (pcr._device == 0xFFFF))
                continue;   /* nothing there */

	    printf("\npci bus 0x%x cardnum 0x%02x, vendor 0x%04x device 0x%04x\n",
	        pcr._pcibuses[pcr._pcibusidx], pcr._cardnum, pcr._vendor,
                pcr._device);

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

	    if (idx++ >= MAX_PCI_DEVICES)
	        continue;

	    identify_card(&pcr);
        }
    } while (++pcr._pcibusidx < pcr._pcinumbus);

    /* Now try pci config 2 probe (deprecated) */

    outb(0xCF8, 0xF1);
    outb(0xCFA, 0x00); /* bus 0 for now */

    printf("\nPCI probing configuration type 2\n");

    pcr._pcibuses[0] = 0;
    pcr._pcinumbus = 1;
    pcr._pcibusidx = 0;
    idx = 0;

    do {
        for (pcr._ioaddr = 0xC000; pcr._ioaddr < 0xD000; pcr._ioaddr += 0x0100){
	    outb(0xCFA, pcr._pcibuses[pcr._pcibusidx]); /* bus 0 for now */
            pcr._device_vendor = inl(pcr._ioaddr);
	    outb(0xCFA, 0x00); /* bus 0 for now */

            if ((pcr._vendor == 0xFFFF) || (pcr._device == 0xFFFF))
                continue;
            if ((pcr._vendor == 0xF0F0) || (pcr._device == 0xF0F0))
                continue;  /* catch ASUS P55TP4XE motherboards */

	    printf("\npci bus 0x%x slot at 0x%04x, vendor 0x%04x device 0x%04x\n",
	        pcr._pcibuses[pcr._pcibusidx], pcr._ioaddr, pcr._vendor,
                pcr._device);

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

	    if (idx++ >= MAX_PCI_DEVICES)
	        continue;

	    identify_card(&pcr);
	}
    } while (++pcr._pcibusidx < pcr._pcinumbus);

    outb(0xCF8, 0x00);

    disable_os_io();
}


void
identify_card(struct pci_config_reg *pcr)
{

	int i = 0, j, foundit = 0;

	while (pvd[i].vendorname != (char *)NULL) {
	    if (pvd[i].vendor_id == pcr->_vendor) {
		j = 0;
		printf(" %s ", pvd[i].vendorname);
		while (pvd[i].device[j].devicename != (char *)NULL) {
		    if (pvd[i].device[j].device_id == pcr->_device) {
	                printf("%s", pvd[i].device[j].devicename);
			foundit = 1;
			break;
		    }
		    j++;
		}
	    }
	    if (foundit)
		break;
	    i++;
	}

	if (!foundit)
		printf(" Device unknown\n");
	else {
	    printf("\n");
	    if (pvd[i].device[j].print_func != (void (*)())NULL) {
                pvd[i].device[j].print_func(pcr);
		return;
	    }
	}

        if (pcr->_status_command)
            printf("  STATUS    0x%04x  COMMAND 0x%04x\n",
                pcr->_status, pcr->_command);
        if (pcr->_class_revision)
            printf("  CLASS     0x%02x 0x%02x 0x%02x  REVISION 0x%02x\n",
                pcr->_base_class, pcr->_sub_class, pcr->_prog_if,
		pcr->_rev_id);
        if (pcr->_bist_header_latency_cache)
            printf("  BIST      0x%02x  HEADER 0x%02x  LATENCY 0x%02x  CACHE 0x%02x\n",
                pcr->_bist, pcr->_header_type, pcr->_latency_timer,
		pcr->_cache_line_size);
        if (pcr->_base0)
            printf("  BASE0     0x%08x  addr 0x%08x  %s\n",
                pcr->_base0, pcr->_base0 & (pcr->_base0 & 0x1 ?
		0xFFFFFFFC : 0xFFFFFFF0), pcr->_base0 & 0x1 ? "I/O" : "MEM");
        if (pcr->_base1)
            printf("  BASE1     0x%08x  addr 0x%08x  %s\n",
                pcr->_base1, pcr->_base1 & (pcr->_base1 & 0x1 ?
		0xFFFFFFFC : 0xFFFFFFF0), pcr->_base1 & 0x1 ? "I/O" : "MEM");
        if (pcr->_base2)
            printf("  BASE2     0x%08x  addr 0x%08x  %s\n",
                pcr->_base2, pcr->_base2 & (pcr->_base2 & 0x1 ?
		0xFFFFFFFC : 0xFFFFFFF0), pcr->_base2 & 0x1 ? "I/O" : "MEM");
        if (pcr->_base3)
            printf("  BASE3     0x%08x  addr 0x%08x  %s\n",
                pcr->_base3, pcr->_base3 & (pcr->_base3 & 0x1 ?
		0xFFFFFFFC : 0xFFFFFFF0), pcr->_base3 & 0x1 ? "I/O" : "MEM");
        if (pcr->_base4)
            printf("  BASE4     0x%08x  addr 0x%08x  %s\n",
                pcr->_base4, pcr->_base4 & (pcr->_base4 & 0x1 ?
		0xFFFFFFFC : 0xFFFFFFF0), pcr->_base4 & 0x1 ? "I/O" : "MEM");
        if (pcr->_base5)
            printf("  BASE5     0x%08x  addr 0x%08x  %s\n",
                pcr->_base5, pcr->_base5 & (pcr->_base5 & 0x1 ?
		0xFFFFFFFC : 0xFFFFFFF0), pcr->_base5 & 0x1 ? "I/O" : "MEM");
        if (pcr->_baserom)
            printf("  BASEROM   0x%08x  addr 0x%08x  %sdecode-enabled\n",
                pcr->_baserom, pcr->_baserom & 0xFFFF8000,
                pcr->_baserom & 0x1 ? "" : "not-");
        if (pcr->_max_min_ipin_iline)
            printf("  MAX_LAT   0x%02x  MIN_GNT 0x%02x  INT_PIN 0x%02x  INT_LINE 0x%02x\n",
                pcr->_max_lat, pcr->_min_gnt, pcr->_int_pin, pcr->_int_line);
        if (pcr->_user_config)
            printf("  BYTE_0    0x%02x  BYTE_1  0x%02x  BYTE_2  0x%02x  BYTE_3  0x%02x\n",
                pcr->_user_config_0, pcr->_user_config_1, pcr->_user_config_2, pcr->_user_config_3);
}


void
print_mach64(struct pci_config_reg *pcr)
{
    unsigned long sparse_io = 0;

    if (pcr->_status_command)
        printf("  STATUS    0x%04x  COMMAND 0x%04x\n",
            pcr->_status, pcr->_command);
    if (pcr->_class_revision)
        printf("  CLASS     0x%02x 0x%02x 0x%02x  REVISION 0x%02x\n",
            pcr->_base_class, pcr->_sub_class, pcr->_prog_if, pcr->_rev_id);
    if (pcr->_bist_header_latency_cache)
        printf("  BIST      0x%02x  HEADER 0x%02x  LATENCY 0x%02x  CACHE 0x%02x\n",
            pcr->_bist, pcr->_header_type, pcr->_latency_timer,
            pcr->_cache_line_size);
    if (pcr->_base0)
        printf("  APBASE    0x%08x  addr 0x%08x\n",
            pcr->_base0, pcr->_base0 & (pcr->_base0 & 0x1 ?
		0xFFFFFFFC : 0xFFFFFFF0));
    if (pcr->_base1)
        printf("  BLOCKIO   0x%08x  addr 0x%08x\n",
            pcr->_base1, pcr->_base1 & (pcr->_base1 & 0x1 ?
		0xFFFFFFFC : 0xFFFFFFF0));
    if (pcr->_baserom)
        printf("  BASEROM   0x%08x  addr 0x%08x  %sdecode-enabled\n",
            pcr->_baserom, pcr->_baserom & 0xFFFF8000,
            pcr->_baserom & 0x1 ? "" : "not-");
    if (pcr->_max_min_ipin_iline)
        printf("  MAX_LAT   0x%02x  MIN_GNT 0x%02x  INT_PIN 0x%02x  INT_LINE 0x%02x\n",
            pcr->_max_lat, pcr->_min_gnt, pcr->_int_pin, pcr->_int_line);
    switch (pcr->_user_config_0 & 0x03) {
    case 0:
	sparse_io = 0x2ec;
	break;
    case 1:
	sparse_io = 0x1cc;
	break;
    case 2:
	sparse_io = 0x1c8;
	break;
    }
    printf("  SPARSEIO  0x%03x    %s    %s\n",
	    sparse_io, pcr->_user_config_0 & 0x04 ? "Block IO enabled" :
	    "Sparse IO enabled",
	    pcr->_user_config_0 & 0x08 ? "Disable 0x46E8" : "Enable 0x46E8");
}

void
print_i128(struct pci_config_reg *pcr)
{
    if (pcr->_status_command)
        printf("  STATUS    0x%04x  COMMAND 0x%04x\n",
            pcr->_status, pcr->_command);
    if (pcr->_class_revision)
        printf("  CLASS     0x%02x 0x%02x 0x%02x  REVISION 0x%02x\n",
            pcr->_base_class, pcr->_sub_class, pcr->_prog_if, pcr->_rev_id);
    if (pcr->_bist_header_latency_cache)
        printf("  BIST      0x%02x  HEADER 0x%02x  LATENCY 0x%02x  CACHE 0x%02x\n",
            pcr->_bist, pcr->_header_type, pcr->_latency_timer,
            pcr->_cache_line_size);
    printf("  MW0_AD    0x%08x  addr 0x%08x  %spre-fetchable\n",
        pcr->_base0, pcr->_base0 & 0xFFC00000,
        pcr->_base0 & 0x8 ? "" : "not-");
    printf("  MW1_AD    0x%08x  addr 0x%08x  %spre-fetchable\n",
        pcr->_base1, pcr->_base1 & 0xFFC00000,
        pcr->_base1 & 0x8 ? "" : "not-");
    printf("  XYW_AD(A) 0x%08x  addr 0x%08x\n",
        pcr->_base2, pcr->_base2 & 0xFFC00000);
    printf("  XYW_AD(B) 0x%08x  addr 0x%08x\n",
        pcr->_base3, pcr->_base3 & 0xFFC00000);
    printf("  RBASE_G   0x%08x  addr 0x%08x\n",
        pcr->_base4, pcr->_base4 & 0xFFFF0000);
    printf("  IO        0x%08x  addr 0x%08x\n",
        pcr->_base5, pcr->_base5 & 0xFFFFFF00);
    printf("  RBASE_E   0x%08x  addr 0x%08x  %sdecode-enabled\n",
        pcr->_baserom, pcr->_baserom & 0xFFFF8000,
        pcr->_baserom & 0x1 ? "" : "not-");
    if (pcr->_max_min_ipin_iline)
        printf("  MAX_LAT   0x%02x  MIN_GNT 0x%02x  INT_PIN 0x%02x  INT_LINE 0x%02x\n",
            pcr->_max_lat, pcr->_min_gnt, pcr->_int_pin, pcr->_int_line);
}

void
print_pcibridge(struct pci_config_reg *pcr)
{
    if (pcr->_status_command)
        printf("  STATUS    0x%04x  COMMAND 0x%04x\n",
            pcr->_status, pcr->_command);
    if (pcr->_class_revision)
        printf("  CLASS     0x%02x 0x%02x 0x%02x  REVISION 0x%02x\n",
            pcr->_base_class, pcr->_sub_class, pcr->_prog_if, pcr->_rev_id);
    if (pcr->_bist_header_latency_cache)
        printf("  BIST      0x%02x  HEADER 0x%02x  LATENCY 0x%02x  CACHE 0x%02x\n",
            pcr->_bist, pcr->_header_type, pcr->_latency_timer,
            pcr->_cache_line_size);
    printf("  PRIBUS 0x%02x SECBUS 0x%02x SUBBUS 0x%02x SECLT 0x%02x\n",
           pcr->_primary_bus_number, pcr->_secondary_bus_number,
	   pcr->_subordinate_bus_number, pcr->_secondary_latency_timer);
    printf("  IOBASE: 0x%02x00 IOLIM 0x%02x00 SECSTATUS 0x%04x\n",
	pcr->_io_base, pcr->_io_limit, pcr->_secondary_status);
    printf("  NOPREFETCH MEMBASE: 0x%08x MEMLIM 0x%08x\n",
	pcr->_mem_base, pcr->_mem_limit);
    printf("  PREFETCH MEMBASE: 0x%08x MEMLIM 0x%08x\n",
	pcr->_prefetch_mem_base, pcr->_prefetch_mem_limit);
    printf("  RBASE_E   0x%08x  addr 0x%08x  %sdecode-enabled\n",
        pcr->_baserom, pcr->_baserom & 0xFFFF8000,
        pcr->_baserom & 0x1 ? "" : "not-");
    if (pcr->_max_min_ipin_iline)
        printf("  MAX_LAT   0x%02x  MIN_GNT 0x%02x  INT_PIN 0x%02x  INT_LINE 0x%02x\n",
            pcr->_max_lat, pcr->_min_gnt, pcr->_int_pin, pcr->_int_line);
}

static int io_fd;

void
enable_os_io()
{
#if defined(SVR4) || defined(SCO)
#if defined(SI86IOPL)
    sysi86(SI86IOPL, 3);
#else
    sysi86(SI86V86, V86SC_IOPL, PS_IOPL);
#endif
#endif
#if defined(linux)
    iopl(3);
#endif
#if defined(__FreeBSD__)  || defined(__386BSD__) || defined(__bsdi__)
    if ((io_fd = open("/dev/console", O_RDWR, 0)) < 0) {
        perror("/dev/console");
        exit(1);
    }
#if defined(__FreeBSD__)  || defined(__386BSD__)
    if (ioctl(io_fd, KDENABIO, 0) < 0) {
        perror("ioctl(KDENABIO)");
        exit(1);
    }
#endif
#if defined(__NetBSD__)
#if !defined(NetBSD1_1)
    if ((io_fd = open("/dev/io", O_RDWR, 0)) < 0) {
	perror("/dev/io");
	exit(1);
    }
#else
    if (i386_iopl(1) < 0) {
	perror("i386_iopl");
	exit(1);
    }
#endif /* NetBSD1_1 */
#endif /* __NerBSD__ */
#if defined(__bsdi__)
    if (ioctl(io_fd, PCCONENABIOPL, 0) < 0) {
        perror("ioctl(PCCONENABIOPL)");
        exit(1);
    }
#endif
#endif
#if defined(MACH386)
    if ((io_fd = open("/dev/iopl", O_RDWR, 0)) < 0) {
        perror("/dev/iopl");
        exit(1);
    }
#endif
}


void
disable_os_io()
{
#if defined(SVR4) || defined(SCO)
#if defined(SI86IOPL)
    sysi86(SI86IOPL, 0);
#else
    sysi86(SI86V86, V86SC_IOPL, 0);
#endif
#endif
#if defined(linux)
    iopl(0);
#endif
#if defined(__FreeBSD__)  || defined(__386BSD__)
    if (ioctl(io_fd, KDDISABIO, 0) < 0) {
        perror("ioctl(KDDISABIO)");
	close(io_fd);
        exit(1);
    }
    close(io_fd);
#endif
#if defined(__NetBSD__)
#if !defined(NetBSD1_1)
    close(io_fd);
#else
    if (i386_iopl(0) < 0) {
	perror("i386_iopl");
	exit(1);
    }
#endif /* NetBSD1_1 */
#endif /* __NetBSD__ */
#if defined(__bsdi__)
    if (ioctl(io_fd, PCCONDISABIOPL, 0) < 0) {
        perror("ioctl(PCCONDISABIOPL)");
	close(io_fd);
        exit(1);
    }
    close(io_fd);
#endif
#if defined(MACH386)
    close(io_fd);
#endif
}


