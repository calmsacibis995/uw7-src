/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/MS-DOS.h	1.1.3.1"
#ident  "$Header$"

/* MS-DOS is a trademark of Microsoft Corporation */

/* Maximum sector size of any device to be supported.  */
#define	MAX_SECTOR_SIZE		1024

/*
 * Maximum track size. This is used in dosformat.c when
 * we need a buffer to hold the contents of a track. malloc(3[sc])
 * memory faults when trying to allocate these BIG chunks of
 * memory, so we make it static.
 */
#define	MAX_TRACK		20480

/* MS-DOS Constants */
#define	BYTES_PER_DIR		32

/*
 * The root directory follows the FAT. Its size is established when
 * the disk is formatted and can never change.  The size of hard disk
 * is usually larger than the floppy disk.  This size can be upto 512
 * for the hard disk.  We are defining 256 for now.
 */
#define ROOT_DIR_ENTRY	256

/*
 * extended boot signature 
 */
#define EXBOOTSIG	0x29

/* MS-DOS Directory fields and their displacements */
#define	FILENAME		0	/* 8 bytes */
#define	EXTENSION		8	/* Filename Extension 3 bytes */
#define	FILE_ATTRIBUTE		11	/* File's attribute */
#define	TIME			22	/* Modification Time */
#define	DATE			24	/* Modification Date */
#define	STARTING_CLUSTER	26	/* Starting Cluster,  LSB 1st */
#define	FILE_SIZE		28	/* 4 Bytes */

/* Boot sector fields under MS-DOS */

#define	SECTOR_SIZE		11	/* ushort */
#define	SECTORS_PER_CLUSTER	13	/* ubyte  */
#define	RESERVED_SECTORS	14	/* ushort */
#define	NUMBER_OF_FATS		16	/* ubyte  */
#define	ROOT_DIR_ENT		17	/* ushort */
#define	TOTAL_SECTORS		19	/* ushort */
#define	MEDIA_DESCRIPTOR	21	/* ubyte  */
#define	SECTORS_PER_FAT		22	/* ushort */
#define	SECTORS_PER_TRACK	24	/* ushort */
#define	NUMBER_OF_HEADS		26	/* ushort */
#define	HIDDEN_SECTORS		28	/* ushort/ulong */
#define	HUGE_SECTORS		32	/* ulong  */
#define	XTD_BOOT_SIG		37	/* ubyte */
#define	VOLUME_ID		38	/* ubyte[4] */
#define	VOLUME_LABEL		42	/* ubyte[11] */
#define	FILESYS_TYPE		53	/* ubyte[8] */

/* MS-DOS FIle attriubute definitions */
#define	READ_ONLY	0x01
#define	HIDDEN		0x02
#define	SYSTEM		0x04
#define	LABEL		0x08
#define	SUB_DIRECTORY	0x10
#define	ARCHIVE		0x20
/* Macros for getting/putting bytes/shorts/longs from/to memory */
#define	getubyte(p)	*((unsigned char *)(p))
#define	getushort(p)	*((unsigned short *)(p))
#define	getulong(p)	*((unsigned long *)(p))

#define	putubyte(p, b)	*((unsigned char *)(p)) = (unsigned char)(b)
#define	putushort(p, b)	*((unsigned short *)(p)) = (unsigned short)(b)
#define	putulong(p, b)	*((unsigned long *)(p)) = (unsigned long)(b)

/* Determine total cluster on volume */
#define	TOTAL_CLUSTERS		(2 + ((odev.total_sectors - (odev.root_base_sector + odev.sectors_in_root)) / odev.sectors_per_cluster))

/* device information */
struct	table_struct {
	int	handle;
	char	*our_fat;
	int	sectors_per_cluster;
	uint_t	bytes_per_sector;
	int	reserved_sectors;
	int	number_of_fats;
	long	root_dir_ent;
	long	total_sectors;
	int	fat16;
	int	media_descriptor;
	long	sectors_per_fat;
	long	root_base_sector;
	long	sectors_in_root;
};

extern	int	optind;
extern	char	*optarg;
extern	int	errno;

