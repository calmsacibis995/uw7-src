#ifndef _IO_F_DDI_H	/* wrapper symbol for kernel use */
#define _IO_F_DDI_H	/* subject to change without notice */

#ident	"@(#)kern-i386:io/f_ddi.h	1.10.4.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * f_ddi.h -- i386 family-specific DDI definitions.
 * # include'ed by ddi.h .
 */

#ifdef _KERNEL

/*
 * Declarations for DDI functions defined either in ddi.c or elsewhere.
 * Some of these duplicate declarations from other header files; they are
 * included here since drivers aren't supposed to # include those other files.
 */

#define NMI_ATTACH		0x01
#define NMI_DETACH		0x02
#define WATCHDOG_ALIVE_ATTACH	0x04
#define WATCHDOG_ALIVE_DETACH	0x08
#define POWERDOWN_ATTACH	0x10
#define POWERDOWN_DETACH	0x20
#define	MCA_ATTACH		0x40
#define	MCA_DETACH		0x80

/*
 *  Argument structure for MCA handler functions
 */

struct mca_info {
	short flags;
	short bank;
	ulong_t mci_addr[2];
	ulong_t mci_misc[2];
};

#define		MCA_INFO_ADDRV		0x1	/* mca_info.mci_addr valid */
#define		MCA_INFO_MISCV		0x2	/* mca_info.mci_misc valid */

/*
 *  Handler function return values
 */

#define MCA_UNKNOWN	0x00
#define MCA_FATAL	0x01
#define MCA_BENIGN	0x02
#define MCA_BUS_TIMEOUT	0x04
#define MCA_REBOOT	0x10

/*
 * Return values from the driver NMI handler.
 */
#define NMI_UNKNOWN	MCA_UNKNOWN
#define NMI_FATAL	MCA_FATAL
#define NMI_BENIGN	MCA_BENIGN
#define NMI_BUS_TIMEOUT	MCA_BUS_TIMEOUT
#define NMI_REBOOT	MCA_REBOOT

extern uchar_t inb(int);
extern ushort_t inw(int);
extern ulong_t inl(int);
extern void outb(int, uchar_t);
extern void outw(int, ushort_t);
extern void outl(int, ulong_t);
extern void repinsb(int, uchar_t *, int);
extern void repinsw(int, ushort_t *, int);
extern void repinsd(int, ulong_t *, int);
extern void repoutsb(int, uchar_t *, int);
extern void repoutsw(int, ushort_t *, int);
extern void repoutsd(int, ulong_t *, int);
extern void drv_callback(int tag, int (*fcn)(), void *arg);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_F_DDI_H */
