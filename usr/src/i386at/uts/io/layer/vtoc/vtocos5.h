#ident	"@(#)kern-i386at:io/layer/vtoc/vtocos5.h	1.2"
#ident	"$Header$"

#ifndef _IO_TARGET_SD01_SD01OS5_H
#define _IO_TARGET_SD01_SD01OS5_H

/*
 * Open Server disk structure constants.
 */
#define MAXPARTS   	16              /* max partitions possible */
#define PAMAGIC 	0x1234		/* magic number for partition table */
#define TWO2PARLOC      (2 * 21L)	/* Disk location of divvy */
#define DIVVYTBL   	2		/* Size of divvy table   */
#define TWO2BADLOC	(2 * 22L)	/* Disk location of bad block table */
#define TWO2BADBLKS	(2 * 4)		/* Size of bad block table */
#define SCSIBAMAGIC 	0xBAD1		/* Magic number SCSI bad block table */
#define ALTS_VER_OS5	23
#define MAXALTS		1023		/* Max alternate tracks possible */

struct partable {
        unsigned short p_magic;         /* magic number validity indicator */
        struct parts {
                long p_off;             /* offset of partition */
                long p_siz;             /* size of partition */
        } p[MAXPARTS];
};

/*
 * Table format for SCSI bad block list
 * Stored in sorted order of b_blk, terminated by a -1 in b_blk
 */
struct SCSIbadtab{
	unsigned short b_magic;
	unsigned short b_maxbad;
	unsigned short res1;			/* Pad to ease alignment */
	unsigned short res2;			/* in following buffers */
	struct badblocks{
		unsigned int b_blk;		/* Bad block */
		unsigned int b_balt;		/* Aliased block */
	} b[MAXALTS];
};

#endif
