#ifndef _IO_CRAM_CRAM_H	/* wrapper symbol for kernel use */
#define _IO_CRAM_CRAM_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/cram/cram.h	1.7"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Defines for accessing the PC AT CMOS ram.
 */
#define	CMOS_ADDR	0x70	/* I/O port address for CMOS ram address */
#define	CMOS_DATA	0x71	/* I/O port address for CMOS ram data */

#define	DSB		0x0e	/* Diagnostic status byte ram address */
#define	SSB		0x0f	/* Shutdown status byte ram address */
#define DDTB		0x10	/* Diskette drive type byte ram address */
#define	FDTB		0x12	/* Fixed disk type byte ram address */
#define	EB		0x14	/* Equipment byte ram address */
#define	BMLOW		0x15	/* Base mem size low byte ram address */
#define	BMHIGH		0x16	/* Base mem size high byte ram address */
#define	EMLOW		0x17	/* Expansion mem size low byte ram address */
#define	EMHIGH		0x18	/* Expansion mem size high byte ram address */
#define	DCEB		0x19	/* Drive C Extended byte ram address */
#define	DDEB		0x1a	/* Drive D Extended byte ram address */
#define	CKSUMLOW	0x2e	/* Checksum low byte ram address */
#define	CKSUMHIGH	0x2f	/* Checksum high byte ram address */
#define	EMLOW2		0x30	/* Expansion mem size low byte ram address */
#define	EMHIGH2		0x31	/* Expansion mem size high byte ram address */
#define	DCB		0x32	/* Date century byte ram address */
#define	IF		0x33	/* Information flag ram address */

/*
 * ioctls for accessing CMOS ram.
 */
#define CMOSIOC	('C' << 8)

#define	CMOSREAD	(CMOSIOC | 0x01)
#define	CMOSWRITE	(CMOSIOC | 0x02)

/*
 * defines for XENIX compatibility
 */

/*
 * Ports for interacting with chip at 
 */
#define CMOSADDR	0x70 	/* Use to select RAM address */
#define CMOSDATA	0x71	/* R/W data */

/*
 * Number of cmos bytes 
 */
#define CMOSSIZE	0x40	/* 64 addressable bytes in chip */

/*
 * Addresses of interest 
 */
#define CMOSDIAG	0x0e	/* Diagnostic Status */
#define CMOSFDT		0x10	/* Floppy Disk Type */
#define CMOSHDT		0x12	/* Hard Disk Type; bits 7-4 are 1st drive */
#define CMOSEQP		0x14	/* Diskette, Video, and CoProcessor info */
#define CMOSADF		0x2d	/* Additional flags - Compaq VDU info */

/*
 * Shifts of interest 
 */
#define VID_SHFT	4	/* Shift display type bits into 0-3 */

/*
 * masks of interest 
 */
#define CMPQVDU		0x04	/* Compaq VDU bit */
#define CMPQDMM		0x01	/* Compaq Dual Mode Monitor bit */


#ifdef _KERNEL

extern unsigned char	CMOSread(unsigned char);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_CRAM_CRAM_H */
