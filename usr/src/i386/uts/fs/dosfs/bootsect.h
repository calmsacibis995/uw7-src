#ifndef _FS_DOSFS_BOOTSECT_H       /* wrapper symbol for kernel use */
#define _FS_DOSFS_BOOTSECT_H       /* subject to change without notice */

#ident	"@(#)kern-i386:fs/dosfs/bootsect.h	1.1"
#ident  "$Header$"

#if defined(__cplusplus)
extern "C" {
#endif


/*
 *  Format of a boot sector.  This is the first sector
 *  on a DOS floppy disk or the fist sector of a partition
 *  on a hard disk.  But, it is not the first sector of
 *  a partitioned hard disk.
 */
struct bootsector33 {
	char bsJump[3];		/* jump instruction E9xxxx or EBxx90	*/
	char bsOemName[8];	/* OEM name and version			*/
	char bsBPB[19];		/* BIOS parameter block			*/
	char bsDriveNumber;	/* drive number (0x80)			*/
	char bsBootCode[474];	/* pad so structure is 512 bytes long	*/
	unsigned short bsBootSectSig;
#define	BOOTSIG	0xaa55
};

struct bootsector50 {
	char bsJump[3];		/* jump instruction E9xxxx or EBxx90	*/
	char bsOemName[8];	/* OEM name and version			*/
	char bsBPB[25];		/* BIOS parameter block			*/
	char bsDriveNumber;	/* drive number (0x80)			*/
	char bsReserved1;	/* reserved				*/
	char bsBootSignature;	/* extended boot signature (0x29)	*/
#define	EXBOOTSIG	0x29
	char bsVolumeID[4];	/* volume ID number			*/
	char bsVolumeLabel[11];	/* volume label				*/
	char bsFileSysType[8];	/* file system type (FAT12 or FAT16)	*/
	char bsBootCode[448];	/* pad so structure is 512 bytes long	*/
	unsigned short bsBootSectSig;
#define	BOOTSIG	0xaa55
};

union bootsector {
	struct bootsector33 bs33;
	struct bootsector50 bs50;
};

/*
 *  Shorthand for fields in the bpb.
 */
#define	bsBytesPerSec	bsBPB.bpbBytesPerSec
#define	bsSectPerClust	bsBPB.bpbSectPerClust
#define	bsResSectors	bsBPB.bpbResSectors
#define	bsFATS		bsBPB.bpbFATS
#define	bsRootDirEnts	bsBPB.bpbRootDirEnts
#define	bsSectors	bsBPB.bpbSectors
#define	bsMedia		bsBPB.bpbMedia
#define	bsFATsecs	bsBPB.bpbFATsecs
#define	bsSectPerTrack	bsBPB.bpbSectPerTrack
#define	bsHeads		bsBPB.bpbHeads
#define	bsHiddenSecs	bsBPB.bpbHiddenSecs
#define	bsHugeSectors	bsBPB.bpbHugeSectors

#if defined(__cplusplus)
        }
#endif

#endif /* _FS_DOSFS_BOOTSECT_H */

