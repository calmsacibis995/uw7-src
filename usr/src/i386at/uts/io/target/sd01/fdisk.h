#ifndef _IO_TARGET_SD01_FDISK_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SD01_FDISK_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/sd01/fdisk.h	1.7.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#define BOOTSZ		446	/* size of boot code in master boot block */
#define FD_NUMPART	4	/* number of 'partitions' in fdisk table */
#define MBB_MAGIC	0xAA55	/* magic number for mboot.signature */
#define DEFAULT_INTLV	4	/* default interleave for testing tracks */
#define MINPSIZE	4	/* minimum number of cylinders in a partition */
#define TSTPAT		0xE5	/* test pattern for verifying disk */

/*
 * structure to hold the fdisk partition table
 */
struct ipart {
	unsigned char bootid;	/* bootable or not */
	unsigned char beghead;	/* beginning head, sector, cylinder */
	unsigned char begsect;	/* begcyl is a 10-bit number. High 2 bits */
	unsigned char begcyl;	/*     are in begsect. */
	unsigned char systid;	/* OS type */
	unsigned char endhead;	/* ending head, sector, cylinder */
	unsigned char endsect;	/* endcyl is a 10-bit number.  High 2 bits */
	unsigned char endcyl;	/*     are in endsect. */
	long    relsect;	/* first sector relative to start of disk */
	long    numsect;	/* number of sectors in partition */
};
/*
 * Values for bootid.
 */
#define NOTACTIVE	0
#define ACTIVE		128
/*
 * Values for systid.
 */
#define DOSOS12		1	/* DOS partition, 12-bit FAT */
#define PCIXOS		2	/* PC/IX partition */
#define DOSDATA		86	/* DOS data partition */
#define DOSOS16		4	/* DOS partition, 16-bit FAT */
#define EXTDOS		5	/* EXT-DOS partition */
#define DOSHUGE         6       /* Huge DOS partition (> 32MB) */
#define SYSCONFIG	18	/* System configuration partition */
#define OTHEROS		98	/* Raw partition for special (DB?) apps */
#define UNIXOS		99	/* UnixWare/SYSV UNIX partition */
#define UNUSED		100	/* unassigned partition */

#define PTYP_WHOLE_DISK		0xFE	/* partition for whole disk */
#define PTYP_MASK	0xFF	/* mask for part type in systid and p_type */

#define MAXDOS		32L	/* max size for DOS partition in MB */

/*
 * Structure to hold master boot block in physical sector 0 of the disk.
 * Note that partitions stuff can't be directly included in the structure
 * because of default alignment rules.
 */
struct mboot {
	char bootinst[BOOTSZ];		/* boot code */
	char parts[FD_NUMPART * sizeof(struct ipart)];
	unsigned short signature;	/* MBB_MAGIC */
};

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_TARGET_SD01_FDISK_H */
