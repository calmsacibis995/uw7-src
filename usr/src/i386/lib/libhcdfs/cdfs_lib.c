/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to USL under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)libcdfs:i386/lib/libhcdfs/cdfs_lib.c	1.15"
#ident	"$Header$"

static char cdfs_lib_copyright[] = "Copyright 1991, 1992 Intel Corp. 469267";

/*
 * CD-ROM file system library functions.
 *
 * A number of these functions are defined by the X/Open CD-ROM Support
 * Component (XCDR) Preliminary Specification (1991), a number are
 * defined by the Rock Ridge Interchange Protocol (RRIP) draft 1.09
 * (1991).  The rest are functions used by the standard utilities, or
 * private support functions.
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/cdrom.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mkdev.h>
#include <sys/mnttab.h>
#include <sys/param.h>
#include <sys/pathname.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/vnode.h>

#include <sys/fs/cdfs_fs.h>
#include <sys/fs/cdfs_inode.h>
#include <sys/fs/cdfs_ioctl.h>
#include <sys/fs/cdfs_susp.h>
#include <sys/fs/iso9660.h>

#include "cdfs_libdef.h"

/*
 * cd_FsMember - Find out if a given path is in a cdfs file system.
 */
static boolean_t
cd_FsMember (path)
const char	*path;		/* Path to check			*/
{
	struct statvfs	buf;	/* Results structure for statvfs(2)	*/
	int		RetVal;	/* Return value				*/

	if (path != NULL) {
		RetVal = statvfs (path, &buf);
		if (RetVal == 0) {
			if ((strcmp (buf.f_basetype, CDFS_ID)) == 0) {
				return (B_TRUE);
			} else {
				errno = EINVAL;
			}
		}
	} else {
		errno = EFAULT;
	}
	return (B_FALSE);
}


/*
 * cd_IsMntPt - Find out if a given path is a cdfs mount point.  If it
 * is not, try to find the mount point.  Note that this code will only
 * work on a local mount point.
 */
static boolean_t
cd_IsMntPt (path, mp)
const char	*path;			/* Path to check		*/
struct mnttab	*mp;			/* Mount table entry to return	*/
{
	int		RetVal;		/* Return Value			*/
	struct mnttab	tmp;		/* Working entry from mount tab.*/
	FILE		*fp;		/* File pointer			*/
	struct stat	StatBuf;	/* Stat results			*/
	struct statvfs	StatvfsBuf;	/* Statvfs results		*/
	dev_t		Device;		/* Device # we're looking for	*/

	if (path != NULL) {
		/*
		 * Check whether this is a mount point.
		 */
		fp = fopen (MNTTAB, "r");
		if (fp == NULL) {
			errno = EMFILE;
			return (B_FALSE);
		}

		tmp.mnt_mountp = (char *)path;
		tmp.mnt_special = NULL;
		tmp.mnt_fstype = NULL;
		tmp.mnt_mntopts = NULL;
		tmp.mnt_time = NULL;

		RetVal = getmntany (fp, mp, &tmp);
		if (RetVal == 0) {
			(void) fclose (fp);
			return (B_TRUE);
		}

		rewind (fp);
		tmp.mnt_mountp = NULL;

		RetVal = stat (path, &StatBuf);
		if (RetVal != 0) {
			(void) fclose (fp);
			return (B_FALSE);
		}
		Device = StatBuf.st_dev;

		/*
		 * Cycle through the mnttab, checking entries to see if they match
		 * the device we're currently mounted on.
		 */
		errno = EINVAL;
		while (getmntent (fp, mp) == 0) {
			RetVal = statvfs (mp->mnt_mountp, &StatvfsBuf);
			if (RetVal != 0) {
				break;
			}

			if ((strcmp (StatvfsBuf.f_basetype, CDFS_ID)) != 0) {
				continue;
			}

			RetVal = stat (mp->mnt_special, &StatBuf);
			if (RetVal != 0) {
				break;
			}

			if (((StatBuf.st_mode & S_IFBLK) == S_IFBLK) ||
						((StatBuf.st_mode & S_IFCHR) == S_IFCHR)) {
				if (StatBuf.st_rdev == Device) {
					/*
					 * In this case, we return B_FALSE, but we have found the
					 * mount point.
					 */
					(void) fclose (fp);
					return (B_FALSE);
				}
			}
		}

		/*
		 * This is not a mount point and we were unable to find out any
		 * more info about the mount point.
		 */
		mp->mnt_special = NULL;
		mp->mnt_mountp = NULL;
		(void) fclose (fp);
	} else {
		errno = EFAULT;
	}
	return (B_FALSE);
}


/*
 * cd_GetDevName - Get the device name that a file system is on.
 */
static int
cd_GetDevName (path, tmppath)
const char	*path;			/* Path to turn into a dev name	*/
char		*tmppath;		/* Path to fill			*/
{
	struct mnttab	mp;		/* One line from MNTTAB file	*/

	/*
	 * If this is a CD-ROM file system member, get its device name.
	 */
	if (!cd_FsMember (path)) {
		return (-1);
	}

	(void) cd_IsMntPt (path, &mp);
	if (mp.mnt_special == NULL) {
		return (-1);
	}
	(void) strcpy (tmppath, mp.mnt_special);

	return (0);
}

/*
 * Determine the Logical sector size of the current CDFS file-system.
 */
int
cdfs_GetSectSize(Dev, SectSize)
int		Dev;			/* File descr of the Device	*/
uint_t	*SectSize;			/* Return addr of sector size	*/
{
	char		*Buf;		/* Buf for 1st part of Vol Descr*/
	uint_t		BufSize;	/* Size of Vol Descr. buffer	*/
	uint_t		Offset;		/* Device offset of desired data*/
	union media_vd	*VolD;	 	/* Volume Descriptor template	*/
	int		RetVal;		/* Return value of called procs */

	/*
	 * Compute the number of bytes (from the beginning of a
	 * Volume Descr.) required in order guarantee that the
	 * entire Standard ID string is present regardless of the
	 * CDFS File-system type i.e. ISO-9660 or High-Sierra.
	 */
	BufSize = MAX(
		CDFS_STRUCTOFF(Pure9660_vd,vd_StdID[0]) + sizeof(VolD->Iso.vd_StdID),
		CDFS_STRUCTOFF(HiSierra_vd,vd_StdID[0]) + sizeof(VolD->Hs.vd_StdID)
	);
	
	/*
	 * Allocate a buffer to hold the required portion
	 * of the Volume Descriptor.
	 */
	BufSize = roundup(BufSize, DEV_BSIZE);
	Buf = malloc(BufSize);
	if (Buf == NULL) {
		return(1);
	}
	VolD = (union media_vd *)Buf;
		
	/*
	 * Using different Logical Sector Sizes, try to locate
	 * the first Volume Descriptor on the media.
	 */
	for (*SectSize=ISO_MIN_LSEC_SZ; *SectSize <= MAXBSIZE; (*SectSize)*=2) {
		/*
		 * Based on the current Logical Sector Size, read in the
		 * first part of the first Volume Descriptor on the media.
		 */
		Offset = (*SectSize) * ISO_VD_LOC;

		RetVal = lseek(Dev, Offset, SEEK_SET);
		if (RetVal != Offset) {
			RetVal = 1;
			break;
		}
			
		RetVal = read(Dev, Buf, BufSize);
		if (RetVal != BufSize) {
			perror("Read Error 1");
			RetVal = 1;
			break;
		}

		/*
		 * If the current Logical Sector Size is correct,
		 * a valid ISO-9660 or High-Sierra ID string should
		 * exist at the appropriate offset within the buffer.
		 */
		if ((strncmp((caddr_t)&VolD->Iso.vd_StdID[0], ISO_STD_ID,
				sizeof(VolD->Iso.vd_StdID)) == 0) &&
			(sizeof(VolD->Iso.vd_StdID) == sizeof(ISO_STD_ID)-1)) {
			RetVal = 0;
			break;

		} else
		if ((strncmp((caddr_t)&VolD->Hs.vd_StdID[0], HS_STD_ID,
				sizeof(VolD->Hs.vd_StdID)) == 0) &&
			(sizeof(VolD->Hs.vd_StdID) == sizeof(HS_STD_ID)-1)) {
			RetVal = 0;
			break;
		}

		/*
		 * The current Logical sector does not contain a known Volume
		 * Descriptor type, so try the next Logical Sector Size.
		 */
		RetVal = -1;
	}

	free(Buf);
	return(RetVal);
}




/*
 * Get the Primary Volume Descriptor from the current CDFS file-system.
 */ 
int
cdfs_ReadPvd(Dev, Buf, SectSize, PvdLoc, FsType)
int				Dev;						/* File Descr. of the file-sys	*/
char 			*Buf;						/* Caller's buffer to store PVD	*/
uint_t			SectSize;					/* Log. Sect size of file-sys	*/
uint_t			*PvdLoc;					/* Ret addr of PVD location		*/
enum cdfs_type	*FsType;					/* Return addr of CDFS type		*/
{
	uint			Sect;					/* Current sector number		*/
	uint			Offset;					/* Device offset of current sect*/
	union media_vd	*VolD;					/* Volume Descriptor buffer		*/
	int				RetVal;					/* Return value of called procs	*/

	VolD = (union media_vd *)Buf;

	/*
	 * Scan each Log. Sector in the Volume Descr. list for the PVD.
	 */
	for (Sect=ISO_VD_LOC; ;Sect++) {

		Offset = Sect * SectSize;

		RetVal = lseek(Dev, Offset, SEEK_SET);
		if (RetVal != Offset) {
			RetVal = 1;
			break;
		}

		RetVal = read(Dev, Buf, SectSize);
		if (RetVal != SectSize) {
			RetVal = 1;
			break;
		}

		/*
		 * See if this is an ISO-9660 Volume Descriptor by checking
		 * for the ISO_9660 Standard ID string.
		 */
		if ((strncmp((caddr_t)&VolD->Iso.vd_StdID[0], ISO_STD_ID,
				sizeof(VolD->Iso.vd_StdID)) == 0) &&
			(sizeof(VolD->Iso.vd_StdID) == sizeof(ISO_STD_ID)-1)) {
			/*
			 * If desired, return the CDFS file-system type.
			 */
			if (FsType != NULL) {
				*FsType = CDFS_ISO_9660;
			}

			/*
			 * Determine Vol. Descr. type and proceed accordingly.
			 * - If this is a PVD then we're done.
			 * - If this is a Terminator VD then we've failed.
			 * - Skip all other VD types.
			 */
			if (VolD->Iso.vd_Type == ISO_PVD_TYPE) {
				RetVal = 0;
				break;

			} else if (VolD->Iso.vd_Type == ISO_TERM_TYPE) {
				RetVal = -1;
				break;
			}
				
		/*
		 * Check for HIGH Sierra Volume Descriptor.
		 */
		} else 
		if ((strncmp((caddr_t)&VolD->Hs.vd_StdID[0], HS_STD_ID,
				sizeof(VolD->Hs.vd_StdID)) == 0) &&
			(sizeof(VolD->Hs.vd_StdID) == sizeof(HS_STD_ID)-1)) {
			/*
			 * If desired, return the CDFS file-system type.
			 */
			if (FsType != NULL) {
				*FsType = CDFS_HIGH_SIERRA;
			}

			/*
			 * Determine Vol. Descr. type and responde accordingly.
			 * - If this is a PVD then we're done.
			 * - If this is a Terminator VD then we've failed.
			 * - Skip all other VD types.
			 */
			if (VolD->Hs.vd_Type == HS_PVD_TYPE) {
				RetVal = 0;
				break;

			} else if (VolD->Hs.vd_Type == HS_TERM_TYPE) {
				RetVal = -1;
				break;
			}

		} else {
			/*
			 * Unrecognized Volume Descriptor.
			 * Return error.
			 */
			RetVal = -2;
			break;
		}
	}

	/*
	 * If desired, return the location of the PVD.
	 * Note: If the PVD was not found, the location
	 * of the last sector examined is returned.
	 */
	if (PvdLoc != NULL) {
		*PvdLoc = Sect;
	}

	return(RetVal);
}




/*
 * Display the contents of a PVD.
 */
void
cdfs_DispPvd(Buf, FsType)
	char				*Buf;				/* Buffer with PVD contents		*/
	enum cdfs_type		FsType;				/* CDFS file system type (9660/HS)*/
{
	struct Pure9660_pvd	*IsoPvd;			/* ISO-9660 PVD Template		*/
	struct HiSierra_pvd	*HsPvd;				/* High-Sierra PVD Template		*/
	uchar_t				*RootDir;			/* Pointer to root string		*/

	switch (FsType) {
		case CDFS_ISO_9660: {
			IsoPvd = (struct Pure9660_pvd *) Buf;
			(void) printf("Primary Volume Descriptor:\n");
			(void) printf("Standard ISO-9660 Identifier: ");
			cd_PrintString (IsoPvd->pvd_StdID, sizeof (IsoPvd->pvd_StdID));
			(void) printf("Volume Descriptor Version: %d\n", IsoPvd->pvd_Ver);
			(void) printf("System Identifier: ");
			cd_PrintString (IsoPvd->pvd_SysID, sizeof (IsoPvd->pvd_SysID));
			(void) printf("Volume Identifier: ");
			cd_PrintString (IsoPvd->pvd_VolID, sizeof (IsoPvd->pvd_VolID));
			(void) printf("Volume Space Size: %ld Logical Blocks\n",
				IsoPvd->pvd_VolSpcSz);
			(void) printf("Volume Set Size: %d\n",
				IsoPvd->pvd_VolSetSz);
			(void) printf("Volume Sequence Number: %d\n",
				IsoPvd->pvd_VolSeqNum);
			(void) printf("Logical Block Size: %d Bytes\n",
				IsoPvd->pvd_LogBlkSz);
			(void) printf("Path Table Size: %ld Bytes\n",
				IsoPvd->pvd_PathTabSz);
			(void) printf("Path Table Location: Logical Block %ld\n",
				IsoPvd->pvd_PathTabLoc);
			(void) printf("Optional Path Table Location: Logical Block %ld\n",
				IsoPvd->pvd_OptPathTabLoc);

			RootDir = &IsoPvd->pvd_RootDir[0];
			(void) printf("Root Directory Record Size: %d Bytes\n",
				((struct Pure9660_drec *)RootDir)->drec_Size);
			(void) printf("Root Directory XAR Size: %d Byte(s)\n",
				((struct Pure9660_drec *)RootDir)->drec_XarSize);
			(void) printf("Location of Root Directory Extent: Logical Block %ld\n",
				((struct Pure9660_drec *)RootDir)->drec_ExtentLoc);
			(void) printf("Size of Root Directory Extent: %ld Bytes\n",
				((struct Pure9660_drec *)RootDir)->drec_DataSz);
			/*
			 * Calculate and print time value.
			 */
			(void) cd_PrintDate ("Root Directory Recording Date/Time", 
						(caddr_t) &((struct Pure9660_drec *)RootDir)->drec_RecordDate,
						CDFS_HDT_TYPE, FsType);

			(void) printf("Root Directory File Flags: 0x%.2X\n",
				((struct Pure9660_drec *)RootDir)->drec_Flags);
			(void) printf("Root Directory File Unit Size: %d Byte(s)\n",
				((struct Pure9660_drec *)RootDir)->drec_UnitSz);
			(void) printf("Root Directory Interleave Gap Size: %d Logical Blocks\n",
				((struct Pure9660_drec *)RootDir)->drec_Interleave);
			(void) printf("Root Directory File ID Size: %d Byte(s)\n",
				((struct Pure9660_drec *)RootDir)->drec_FileIDSz);
			/*
			 * Don't print out the root directory file ID, as it is
			 * unprintable.
			 */
			(void) printf("Volume Set ID: ");
			cd_PrintString (IsoPvd->pvd_VolSetID,
				sizeof (IsoPvd->pvd_VolSetID));
			(void) printf("Publisher ID: ");
			cd_PrintString (IsoPvd->pvd_PublsherID,
				sizeof (IsoPvd->pvd_PublsherID));
			(void) printf("Preparer ID: ");
			cd_PrintString (IsoPvd->pvd_PreparerID,
				sizeof (IsoPvd->pvd_PreparerID));
			(void) printf("Application ID: ");
			cd_PrintString (IsoPvd->pvd_ApplID, sizeof (IsoPvd->pvd_ApplID));
			(void) printf("Copyright File: ");
			cd_PrintString (IsoPvd->pvd_CopyrightFile,
				sizeof (IsoPvd->pvd_CopyrightFile));
			(void) printf("Abstract File: ");
			cd_PrintString (IsoPvd->pvd_AbstractFile,
				sizeof (IsoPvd->pvd_AbstractFile));
			(void) printf("Bibliographic File: ");
			cd_PrintString (IsoPvd->pvd_BiblioFile,
				sizeof (IsoPvd->pvd_BiblioFile));

			/*
			 * Calculate and print time value for various date/time fields.
			 */
			(void) cd_PrintDate ("Volume Creation Date/Time",
						(caddr_t) &IsoPvd->pvd_CreateDate, CDFS_ADT_TYPE,
						FsType);

			(void) cd_PrintDate ("Volume Modification Date/Time",
						(caddr_t) &IsoPvd->pvd_ModDate, CDFS_ADT_TYPE,
						FsType);

			(void) cd_PrintDate ("Volume Expiration Date/Time",
						(caddr_t) &IsoPvd->pvd_ExpireDate, CDFS_ADT_TYPE,
						FsType);

			(void) cd_PrintDate ("Volume Effective Date/Time",
						(caddr_t) &IsoPvd->pvd_EffectDate, CDFS_ADT_TYPE,
						FsType);

			(void) printf("File Structure Version Number: %d\n",
				IsoPvd->pvd_FileVer);

			break;
		}
		case CDFS_HIGH_SIERRA: {
			HsPvd = (struct HiSierra_pvd *) Buf;
			(void) printf("Primary Volume Descriptor:\n");
			(void) printf("Volume Descriptor Logical Block Number: %ld\n",
				HsPvd->pvd_LogBlkNum);
			(void) printf("Volume Descriptor Type: %d\n",
				HsPvd->pvd_Type);
			(void) printf("Standard High Sierra Identifier: ");
			cd_PrintString (&HsPvd->pvd_StdID[0], sizeof (HsPvd->pvd_StdID));
			(void) printf("Volume Descriptor Version: %d\n",
				HsPvd->pvd_Ver);
			(void) printf("System Identifier: ");
			cd_PrintString (HsPvd->pvd_SysID, sizeof (HsPvd->pvd_SysID));
			(void) printf("Volume Identifier: ");
			cd_PrintString (HsPvd->pvd_VolID, sizeof (HsPvd->pvd_VolID));
			(void) printf("Volume Space Size: %ld Logical Blocks\n",
				HsPvd->pvd_VolSpcSz);
			(void) printf("Volume Set Size: %d\n",
				HsPvd->pvd_VolSetSz);
			(void) printf("Volume Sequence Number: %d\n",
				HsPvd->pvd_VolSeqNum);
			(void) printf("Logical Block Size: %d Bytes\n",
				HsPvd->pvd_LogBlkSz);
			(void) printf("Path Table Size: %ld Bytes\n",
				HsPvd->pvd_PathTabSz);
			(void) printf("Path Table Location: Logical Block %ld\n",
				HsPvd->pvd_PathTabLoc);
			(void) printf("First Optional Path Table Location: Logical Block %ld\n",
				HsPvd->pvd_Opt1PathTabLoc);
			(void) printf("Second Optional Path Table Location: Logical Block %ld\n",
				HsPvd->pvd_Opt2PathTabLoc);
			(void) printf("Third Optional Path Table Location: Logical Block %ld\n",
				HsPvd->pvd_Opt3PathTabLoc);

			RootDir = &HsPvd->pvd_RootDir[0];
			(void) printf("Root Directory Record Size: %d Bytes\n",
				((struct HiSierra_drec *)RootDir)->drec_Size);
			(void) printf("Root Directory XAR Size: %d Byte(s)\n",
				((struct HiSierra_drec *)RootDir)->drec_XarSize);
			(void) printf("Location of Root Directory Extent: Logical Block %ld\n",
				((struct HiSierra_drec *)RootDir)->drec_ExtentLoc);
			(void) printf("Size of Root Directory Extent: %ld Bytes\n",
				((struct HiSierra_drec *)RootDir)->drec_DataSz);
			/*
			 * Calculate and print time value.
			 */
			(void) cd_PrintDate ("Root Directory Recording Date/Time", 
						(caddr_t) &((struct HiSierra_drec *)RootDir)->drec_RecordDate,
						CDFS_HDT_TYPE, FsType);

			(void) printf("Root Directory File Flags: 0x%.2X\n",
				((struct HiSierra_drec *)RootDir)->drec_Flags);
			(void) printf("Root Directory Interleave Gap Size: %d Logical Blocks\n",
				((struct HiSierra_drec *)RootDir)->drec_InterleaveSz);
			(void) printf("Root Directory Interleave Skip Factor: %d Logical Blocks\n",
				((struct HiSierra_drec *)RootDir)->drec_InterleaveSkip);
			(void) printf("Root Directory Volume Sequence Number: %d\n",
				((struct HiSierra_drec *)RootDir)->drec_VolSeqNum);
			(void) printf("Root Directory File ID Size: %d Byte(s)\n",
				((struct HiSierra_drec *)RootDir)->drec_FileIDSz);
			/*
			 * Don't print out the root directory file ID, as it is
			 * unprintable.
			 */
			(void) printf("Volume Set ID: ");
			cd_PrintString (HsPvd->pvd_VolSetID, sizeof (HsPvd->pvd_VolSetID));
			(void) printf("Publisher ID: ");
			cd_PrintString (HsPvd->pvd_PublsherID,
				sizeof (HsPvd->pvd_PublsherID));
			(void) printf("Preparer ID: ");
			cd_PrintString (HsPvd->pvd_PreparerID,
				sizeof (HsPvd->pvd_PreparerID));
			(void) printf("Application ID: ");
			cd_PrintString (HsPvd->pvd_ApplID, sizeof (HsPvd->pvd_ApplID));
			(void) printf("Copyright File: ");
			cd_PrintString (HsPvd->pvd_CopyrightFile,
				sizeof (HsPvd->pvd_CopyrightFile));
			(void) printf("Abstract File: ");
			cd_PrintString (HsPvd->pvd_AbstractFile,
				sizeof (HsPvd->pvd_AbstractFile));

			/*
			 * Calculate and print time value for various date/time fields.
			 */
			(void) cd_PrintDate ("Volume Creation Date/Time",
						(caddr_t) &HsPvd->pvd_CreateDate, CDFS_ADT_TYPE,
						FsType);

			(void) cd_PrintDate ("Volume Modification Date/Time",
						(caddr_t) &HsPvd->pvd_ModDate, CDFS_ADT_TYPE,
						FsType);

			(void) cd_PrintDate ("Volume Expiration Date/Time",
						(caddr_t) &HsPvd->pvd_ExpireDate, CDFS_ADT_TYPE,
						FsType);

			(void) cd_PrintDate ("Volume Effective Date/Time",
						(caddr_t) &HsPvd->pvd_EffectDate, CDFS_ADT_TYPE,
						FsType);

			(void) printf("File Structure Version Number: %d\n",
				HsPvd->pvd_FileVer);

			break;
		}
		default: {
			(void) fprintf(stderr, "%s (Unrecognized format)\n", CDFS_ID);
			break;
		}
	}
}





/*
 * cd_pvd(), cd_cpvd() - Read Primary Volume Descriptor from CD-ROM.
 *
 * (Note that the disc might not be mounted, so we may not use file system
 * code.)
 */

int
cd_pvd (path, pvd)
const char		*path;		/* CD-ROM device or file/dir	*/
struct iso9660_pvd	*pvd;		/* Ptr to PVD structure		*/
{
	int		RetVal;			/* Return value		*/
	char		*buf;			/* Local copy of the PVD*/
	int		fd;			/* File descriptor for device*/
	char		TmpPath[MAXPATHLEN];	/* Tmp path name	*/
	uint_t		SecSize;		/* Sector size of the device*/
	enum cdfs_type	FsType;			/* File system type	*/

	cd_DoSignal();

	if ((path == NULL) || (pvd == NULL)) {
		errno = EFAULT;
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * If path is a mount point, turn this into a device name.  Otherwise,
	 * just use the path we were given.  This will allow us to have a
	 * CD-ROM image in a file, and still invoke this code.
	 */
	RetVal = cd_GetDevName (path, TmpPath);
	if (RetVal < 0) {
		(void) strcpy (TmpPath, path);
	}

	/*
	 * Get the PVD.
	 */
	fd = open (TmpPath, O_RDONLY);
	if (fd < 0) {
		cd_UndoSignal();
		return (-1);
	}

	RetVal = cdfs_GetSectSize (fd, &SecSize);
	if (RetVal != 0) {
		(void) close (fd);
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * Make sure we have enough space for the results.
	 */
	if (CD_PVDLEN < SecSize) {
		errno = EINVAL;
		(void) close (fd);
		cd_UndoSignal();
		return (-1);
	}

	buf = malloc (SecSize);
	if (buf == NULL) {
		(void) close (fd);
		cd_UndoSignal();
		return (-1);
	}

	RetVal = cdfs_ReadPvd (fd, buf, SecSize, (uint_t *) NULL, &FsType);
	(void) close (fd);
	if (RetVal < 0) {
		free (buf);
		cd_UndoSignal();
		return (-1);
	}

	RetVal = cd_FillPvd (pvd, (union media_pvd *) buf, FsType);
	free (buf);
	cd_UndoSignal();
	if (RetVal < 0) {
		return (-1);
	}

	return (0);
}


int
cd_cpvd (path, pvd)
const	char		*path;		/* CD-ROM device or file/dir	*/
char			*pvd;		/* Ptr to PVD buffer		*/
{
	int		RetVal;		/* Return value		*/
	int		fd;		/* File descriptor for device*/
	char		TmpPath[MAXPATHLEN];	/* Tmp path name	*/
	uint_t		SecSize;		/* Sector size of the device*/
	enum cdfs_type	FsType;			/* File system type	*/

	cd_DoSignal();

	if ((path == NULL) || (pvd == NULL)) {
		errno = EFAULT;
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * If path is a mount point, turn this into a device name.  Otherwise,
	 * just use the path we were given.  This will allow us to have a
	 * CD-ROM image in a file, and still invoke this code.
	 */
	RetVal = cd_GetDevName (path, TmpPath);
	if (RetVal < 0) {
		(void) strcpy (TmpPath, path);
	}

	/*
	 * Get the PVD.
	 */
	fd = open (TmpPath, O_RDONLY);
	if (fd < 0) {
		cd_UndoSignal();
		return (-1);
	}

	RetVal = cdfs_GetSectSize (fd, &SecSize);
	if (RetVal != 0) {
		(void) close (fd);
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * Make sure we have enough space for the results.
	 */
	if (CD_PVDLEN < SecSize) {
		errno = EINVAL;
		(void) close (fd);
		cd_UndoSignal();
		return (-1);
	}

	RetVal = cdfs_ReadPvd (fd, pvd, SecSize, (uint_t *) NULL, &FsType);
	(void) close (fd);
	cd_UndoSignal();
	if (RetVal < 0) {
		return (-1);
	}
		
	return (0);
}





/*
 * cd_xar(), cd_cxar() - Read Extended Attribute Record for file/directory
 * from CD-ROM.
 * 
 * Note that cd_xar() is not implemented in terms of the cd_cxar()
 * routine, due to the mismatch between the XAR as it appears on the
 * media and the iso9660_xar structure.
 */

int
cd_xar (path, fsec, xar, applen, esclen)
const	char		*path;		/* Ptr to file/directory name	*/
const	int		fsec;		/* File section			*/
struct iso9660_xar	*xar;		/* Ptr to XAR structure		*/
const	int		applen;		/* Len of buf for App Use field	*/
const	int		esclen;		/* Len of buf for Esc Seq field	*/
{
	int			RetVal;		/* Return value		*/
	int			fd;		/* File descriptor	*/
	struct cdfs_IocArgs	GenArgs;	/* Generic argument structure*/
	struct cdfs_XarArgs	Args;		/* Argument structure for ioctl	*/
	uchar_t			MntPt[MAXPATHLEN];	/* Mount point of CD-ROM		*/
	struct mnttab		MntEnt;		/* Mount table entry	*/
	uchar_t			*buf;		/* local XAR copy	*/
	enum cdfs_type		FsType;		/* Type of file system	*/

	cd_DoSignal();

	if (!cd_FsMember (path)) {
		cd_UndoSignal();
		return (-1);
	}
	if ((fsec == 0) || (fsec < -1)) {
		errno = ENXIO;
		cd_UndoSignal();
		return (-1);
	}
	if ((applen < 0) || (esclen < 0)) {
		errno = EINVAL;
		cd_UndoSignal();
		return (-1);
	}
	if (xar == NULL) {
		errno = EFAULT;
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * Allocate space for working copy of XAR.
	 */
	buf = (uchar_t *) malloc ((size_t) (CD_XARFIXL + applen + esclen));
	if (buf == NULL) {
		cd_UndoSignal();
		return (-1);
	}

	Args.Xar = (union media_xar *) buf;
	Args.fsec = fsec;
	Args.applen = applen;
	Args.esclen = esclen;
	Args.xarlen = -1;

	/*
	 * Find the mount point of the CD-ROM to operate on.
	 */
	if (!cd_IsMntPt (path, &MntEnt)) {
		if (MntEnt.mnt_mountp == NULL) {
			free (buf);
			return (-1);
		}
		(void) strcpy ((char *) MntPt, MntEnt.mnt_mountp);
	} else {
		(void) strcpy ((char *) MntPt, path);
	}

	GenArgs.PathName = (uchar_t *) path;
	GenArgs.ArgPtr = &Args;

	/*
	 * Get the XAR.
	 */
	fd = open ((char *) MntPt, O_RDONLY);
	if (fd < 0) {
		free (buf);
		cd_UndoSignal();
		return (-1);
	}

	RetVal = ioctl (fd, CDFS_GETXAR, &GenArgs);
	(void) close (fd);
	if (RetVal < 0) {
		free (buf);
		cd_UndoSignal();
		return (-1);
	}

	FsType = cd_type ((char *) MntPt);
	if (FsType < 0) {
		free (buf);
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * Fill the user's structure from the raw buffer.
	 */
	RetVal = cd_FillXar (xar, (union media_xar *) buf, applen, esclen, FsType);
	free (buf);
	cd_UndoSignal();
	if (RetVal < 0) {
		return (-1);
	}

	/*
	 * Return number of bytes copied for variable part of XAR.
	 */
	return (applen + esclen);
}


int
cd_cxar (path, fsec, xar, xarlen)
const	char		*path;		/* Ptr to file/directory name	*/
const	int		fsec;		/* File section			*/
char			*xar;		/* Ptr to XAR buffer		*/
const	int		xarlen;		/* Length of XAR		*/
{
	int					RetVal;				/* Return value					*/
	int					fd;					/* File descriptor				*/
	struct cdfs_IocArgs	GenArgs;			/* Generic argument structure	*/
	struct cdfs_XarArgs	Args;				/* Argument structure for ioctl	*/
	uchar_t				MntPt[MAXPATHLEN];	/* Mount point of CD-ROM		*/
	struct mnttab		MntEnt;				/* Mount table entry			*/

	cd_DoSignal();

	if (!cd_FsMember (path)) {
		cd_UndoSignal();
		return (-1);
	}
	if ((fsec == 0) || (fsec < -1)) {
		errno = ENXIO;
		cd_UndoSignal();
		return (-1);
	}
	if (xarlen < 0) {
		errno = EINVAL;
		cd_UndoSignal();
		return (-1);
	}
	if (xar == NULL) {
		errno = EFAULT;
		cd_UndoSignal();
		return (-1);
	}

	Args.Xar = (union media_xar *) xar;
	Args.fsec = fsec;
	Args.applen = -1;
	Args.esclen = -1;
	Args.xarlen = xarlen;

	/*
	 * Find the mount point of the CD-ROM to operate on.
	 */
	if (!cd_IsMntPt (path, &MntEnt)) {
		if (MntEnt.mnt_mountp == NULL) {
			return (-1);
		}
		(void) strcpy ((char *) MntPt, MntEnt.mnt_mountp);
	} else {
		(void) strcpy ((char *) MntPt, path);
	}

	GenArgs.PathName = (uchar_t *) path;
	GenArgs.ArgPtr = &Args;

	/*
	 * Get the XAR.
	 */
	fd = open ((char *) MntPt, O_RDONLY);
	if (fd < 0) {
		cd_UndoSignal();
		return (-1);
	}

	RetVal = ioctl (fd, CDFS_GETXAR, &GenArgs);
	(void) close (fd);
	cd_UndoSignal();
	if (RetVal < 0) {
		return (-1);
	}

	/*
	 * Return the number of bytes copied for the whole XAR.
	 */
	return (xarlen);
}





/*
 * cd_drec(), cd_cdrec() - Read Directory Record from CD-ROM.
 */

int
cd_drec (path, fsec, drec)
const	char		*path;		/* Ptr to file/directory name	*/
const	int		fsec;		/* File section			*/
struct iso9660_drec	*drec;		/* Ptr to DREC structure	*/
{
	int					RetVal;				/* Return value					*/
	uchar_t				buf[CD_MAXDRECL];	/* Working buffer				*/
	enum cdfs_type		FsType;				/* Type of file system			*/

	cd_DoSignal();

	if (!cd_FsMember (path)) {
		cd_UndoSignal();
		return (-1);
	}
	if ((fsec == 0) || (fsec < -1)) {
		errno = ENXIO;
		cd_UndoSignal();
		return (-1);
	}
	if (drec == NULL) {
		errno = EFAULT;
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * Get the DREC.
	 */
	RetVal = cd_cdrec (path, fsec, (char *) buf);
	if (RetVal < 0) {
		cd_UndoSignal();
		return (-1);
	}

	FsType = cd_type (path);
	if (FsType < 0) {
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * Fill the user's copy of the DREC.
	 */
	RetVal = cd_FillDRec (drec, (union media_drec *) buf, FsType);
	cd_UndoSignal();
	if (RetVal < 0) {
		return (-1);
	}

	return (0);
}


int
cd_cdrec (path, fsec, drec)
const	char		*path;		/* Ptr to file/directory name	*/
const	int		fsec;		/* File section			*/
char			*drec;		/* Ptr to DREC buffer		*/
{
	int			RetVal;		/* Return value		*/
	int			fd;		/* File descriptor	*/
	struct cdfs_IocArgs	GenArgs;	/* Generic argument structure*/
	struct cdfs_DRecArgs	Args;		/* Argument structure for ioctl	*/
	uchar_t			MntPt[MAXPATHLEN];	/* Mount point of CD-ROM		*/
	struct mnttab		MntEnt;		/* Mount table entry	*/

	cd_DoSignal();

	if (!cd_FsMember (path)) {
		cd_UndoSignal();
		return (-1);
	}
	if ((fsec == 0) || (fsec < -1)) {
		errno = ENXIO;
		cd_UndoSignal();
		return (-1);
	}
	if (drec == NULL) {
		errno = EFAULT;
		cd_UndoSignal();
		return (-1);
	}

	Args.DRec = (union media_drec *) drec;
	Args.fsec = fsec;

	/*
	 * Find the mount point of the CD-ROM to operate on.
	 */
	if (!cd_IsMntPt (path, &MntEnt)) {
		if (MntEnt.mnt_mountp == NULL) {
			return (-1);
		}
		(void) strcpy ((char *) MntPt, MntEnt.mnt_mountp);
	} else {
		(void) strcpy ((char *) MntPt, path);
	}

	GenArgs.PathName = (uchar_t *) path;
	GenArgs.ArgPtr = &Args;

	/*
	 * Get the DREC.
	 */
	fd = open ((char *) MntPt, O_RDONLY);
	if (fd < 0) {
		cd_UndoSignal();
		return (-1);
	}

	RetVal = ioctl (fd, CDFS_GETDREC, &GenArgs);
	(void) close (fd);
	cd_UndoSignal();
	if (RetVal < 0) {
		return (-1);
	}

	return (0);
}





/*
 * cd_ptrec(), cd_cptrec() - Read Path Table Record from CD-ROM Path Table.
 */

int
cd_ptrec (path, ptrec)
const	char		*path;			/* Ptr to directory name */
struct iso9660_ptrec	*ptrec;			/* Ptr to PTREC structure */
{
	int					RetVal;				/* Return value					*/
	uchar_t				buf[CD_MAXPTRECL];	/* Temporary buffer				*/
	struct stat			StatStruct;			/* Results of stat() call		*/
	enum cdfs_type		FsType;				/* Type of file system			*/

	cd_DoSignal();

	if (!cd_FsMember (path)) {
		cd_UndoSignal();
		return (-1);
	}
	if (ptrec == NULL) {
		errno = EFAULT;
		cd_UndoSignal();
		return (-1);
	}

	RetVal = stat (path, &StatStruct);
	if (RetVal != 0) {
		cd_UndoSignal();
		return (-1);
	}

	if ((StatStruct.st_mode & S_IFDIR) != S_IFDIR) {
		errno = ENOTDIR;
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * Get the PTREC.
	 */
	RetVal = cd_cptrec (path, (char *) &buf);
	if (RetVal < 0) {
		cd_UndoSignal();
		return (-1);
	}

	FsType = cd_type (path);
	if (FsType < 0) {
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * Fill the user's copy of the PTREC.
	 */
	RetVal = cd_FillPTRec (ptrec, (union media_ptrec *) buf, FsType);
	cd_UndoSignal();
	if (RetVal < 0) {
		return (-1);
	}

	return (0);
}


int
cd_cptrec (path, ptrec)
const	char		*path;		/* Ptr to directory name	*/
char			*ptrec;		/* Ptr to PTREC buffer		*/
{
	int					RetVal;				/* Return value					*/
	int					fd;					/* File descriptor				*/
	struct cdfs_IocArgs	GenArgs;			/* Generic argument structure	*/
	uchar_t				MntPt[MAXPATHLEN];	/* Mount point of CD-ROM		*/
	struct mnttab		MntEnt;				/* Mount table entry			*/
	uchar_t				pt_buf[CD_MAXPTRECL + 2];	/* PTREC buffer			*/

	cd_DoSignal();

	if (!cd_FsMember (path)) {
		cd_UndoSignal();
		return (-1);
	}
	if (ptrec == NULL) {
		errno = EFAULT;
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * Find the mount point of the CD-ROM to operate on.
	 */
	if (!cd_IsMntPt (path, &MntEnt)) {
		if (MntEnt.mnt_mountp == NULL) {
			return (-1);
		}
		(void) strcpy ((char *) MntPt, MntEnt.mnt_mountp);
	} else {
		(void) strcpy ((char *) MntPt, path);
	}

	GenArgs.PathName = (uchar_t *) path;
	GenArgs.ArgPtr = &pt_buf;

	/*
	 * Get the PTREC.
	 */
	fd = open ((char *) MntPt, O_RDONLY);
	if (fd < 0) {
		cd_UndoSignal();
		return (-1);
	}

	RetVal = ioctl (fd, CDFS_GETPTREC, &GenArgs);
	(void) close (fd);
	if (RetVal < 0) {
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * Fill the user's copy of the PTREC.
	 */
	(void) memmove (ptrec, (char *) pt_buf, CD_MAXPTRECL);

	cd_UndoSignal();
	return (0);
}





/*
 * cd_type() - Get identification of CD-ROM.
 *
 * Note that this may be run on an unmounted CD-ROM.
 */

int
cd_type (path)
const char		*path;		/* CD-ROM dev or file/directory	*/
{
	int		RetVal;		/* Return value			*/
	int		fd;		/* File descriptor		*/
	FILE		*fp;		/* File pointer			*/
	struct stat	StatStruct;	/* Results of stat() call	*/
	struct mnttab	mp;			/* One line from MNTTAB file*/
	struct mnttab	mp_tmp;			/* Value to look for in MNTTAB*/
	struct cdfs_IocArgs	GenArgs;	/* Generic argument structure*/
	enum cdfs_type	Type;			/* Type of CD-ROM	*/
	uchar_t		TmpPath[MAXPATHLEN];	/* Path to open/issue ioctl*/
	uchar_t				*buf;	/* Hold the PVD if we have to*/

	cd_DoSignal();


	if (path == NULL) {
		errno = EFAULT;
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * If the path is in a cdfs file system, use the ioctl code to
	 * find the type.  Otherwise, a device node can get us to the
	 * mount point (if the device is mounted), or we can just read
	 * directly from the device.
	 *
	 * Set TmpPath if we need to operate on a different path than this one.
	 */
	if (cd_FsMember (path)) {
		if (!cd_IsMntPt (path, &mp)) {
			if (mp.mnt_mountp == NULL) {
				TmpPath[0] = '\0';
			} else {
				(void) strcpy ((char *) TmpPath, mp.mnt_mountp);
			}
		} else {
			(void) strcpy ((char *) TmpPath, path);
		}
	} else {
		/*
		 * Path is not in a cdfs file system.  Other valid args types are the
		 * block or character special device of a CD-ROM.
		 */
		RetVal = stat (path, &StatStruct);
		if (RetVal != 0) {
			cd_UndoSignal();
			return (-1);
		}
	
		if (((StatStruct.st_mode & S_IFBLK) == S_IFBLK) ||
					((StatStruct.st_mode & S_IFCHR) == S_IFCHR)) {
			/*
			 * Determine if this device is currently mounted.
			 */
			fp = fopen (MNTTAB, "r");
			if (fp == NULL) {
				cd_UndoSignal();
				return (-1);
			}
			mp_tmp.mnt_special = (char *)path;
			RetVal = getmntany (fp, &mp, &mp_tmp);
			if (RetVal == 0) {
				(void) strcpy ((char *) TmpPath, mp.mnt_mountp);
			} else {
				TmpPath[0] = '\0';
			}
			(void) fclose (fp);
		} else {
			errno = EINVAL;
			cd_UndoSignal();
			return (-1);
		}
	}

	/*
	 * Get the CD type in one of two ways, depending on whether the
	 * file system is mounted or not.  The ioctl should be much faster.
	 */
	if (TmpPath[0] == '\0') {
		buf = malloc (CD_PVDLEN);
		if (buf == NULL) {
			cd_UndoSignal();
			return (-1);
		}

		RetVal = cd_cpvd (path, (char *) buf);
		if (RetVal != 0) {
			free (buf);
			cd_UndoSignal();
			return (-1);
		}
		
		/*
		 * Examine the PVD buffer to determine the type.
		 */
		if (strncmp ((char *) &(((struct Pure9660_pvd *) buf)->pvd_StdID),
									ISO_STD_ID, strlen (ISO_STD_ID)) == 0) {
			Type = CD_ISO9660;
		} else {
			if (strncmp ((char *) &(((struct HiSierra_pvd *) buf)->pvd_StdID),
									HS_STD_ID, strlen (HS_STD_ID)) == 0) {
				Type = CDFS_HIGH_SIERRA;
			} else {
				Type = CDFS_UNDEF_FS_TYPE;
			}
		}
		free (buf);
	} else {
		fd = open ((char *) TmpPath, O_RDONLY);
		if (fd < 0) {
			cd_UndoSignal();
			return (-1);
		}

		GenArgs.PathName = TmpPath;
		GenArgs.ArgPtr = &Type;
	
		RetVal = ioctl (fd, CDFS_GETTYPE, &GenArgs);
		(void) close (fd);
		if (RetVal < 0) {
			cd_UndoSignal();
			return (-1);
		}
	}

	cd_UndoSignal();
	return (Type);
}





/*
 * cd_defs() - Set and get default values for User/Group ID and
 * file/directory permissions.
 *
 * For each of these defaults, that value will be used for any
 * file/directory on the mounted CD-ROM if that file/directory does
 * not have one.
 */

int
cd_defs (path, cmd, defs)
const	char		*path;		/* CD-ROM mount point		*/
int			cmd;		/* Operation to perform		*/
struct cd_defs		*defs;		/* Definitions to set		*/
{
	int					RetVal;				/* Return value					*/
	int					fd;					/* File descriptor				*/
	struct cdfs_IocArgs	GenArgs;			/* Generic argument structure	*/
	struct cdfs_DefArgs	Args;				/* Argument to ioctl			*/
	struct mnttab		MntEnt;				/* Mount table entry			*/

	cd_DoSignal();

	/*
	 * Reject if this is not a cdfs mount-point.
	 */
	if ((!cd_FsMember (path)) || (!cd_IsMntPt (path, &MntEnt))) {
		cd_UndoSignal();
		return (-1);
	}

	if (defs == NULL) {
		errno = EFAULT;
		cd_UndoSignal();
		return (-1);
	}
	if ((cmd != CD_GETDEFS) && (cmd != CD_SETDEFS)) {
		errno = EINVAL;
		cd_UndoSignal();
		return (-1);
	}
	if ((cmd == CD_SETDEFS) &&
				((defs->def_uid < 0) || ((defs->def_uid > MAXUID) ||
				(defs->def_gid < 0) || (defs->def_gid > MAXUID) ||
				((defs->def_fperm & ~MODEMASK) != 0) ||
				((defs->def_dperm & ~MODEMASK) != 0) ||
				((defs->dirsperm != CD_DIRRX) &&
							(defs->dirsperm != CD_DIRXAR))))) {
		errno = EINVAL;
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * Set defaults - make sure to not try turning write permissions on.
	 */
	Args.DefCmd = cmd;
	if (cmd == CD_SETDEFS) {
		Args.defs.def_uid = defs->def_uid;
		Args.defs.def_gid = defs->def_gid;
		Args.defs.def_fperm = defs->def_fperm &
					~(IWRITE_USER | IWRITE_GROUP | IWRITE_OTHER);
		Args.defs.def_dperm = defs->def_dperm &
					~(IWRITE_USER | IWRITE_GROUP | IWRITE_OTHER);
		Args.defs.dirsperm = defs->dirsperm;
	}

	GenArgs.PathName = (uchar_t *) path;
	GenArgs.ArgPtr = &Args;

	/*
	 * Get/set the defaults.
	 */
	fd = open (path, O_RDONLY);
	if (fd < 0) {
		cd_UndoSignal();
		return (-1);
	}

	RetVal = ioctl (fd, CDFS_DODEFS, &GenArgs);
	(void) close (fd);
	if (RetVal < 0) {
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * If we're getting defaults, fill the user's structure.
	 */
	if (cmd == CD_GETDEFS) {
		defs->def_uid = Args.defs.def_uid;
		defs->def_gid = Args.defs.def_gid;
		defs->def_fperm = Args.defs.def_fperm;
		defs->def_dperm = Args.defs.def_dperm;
		defs->dirsperm = Args.defs.dirsperm;
	}

	cd_UndoSignal();
	return (0);
}





/*
 * cd_idmap() - Set and get mappings of User/Group IDs.
 * 
 * These mappings will apply to all occurrences of the mapped IDs on
 * the CD-ROM.
 */

int
cd_idmap (path, cmd, idmap, nmaps)
const	char		*path;		/* CD-ROM mount point		*/
int			cmd;		/* Operation to perform		*/
struct cd_idmap		idmap[];	/* ID mapping structure		*/
int			*nmaps;		/* Number of mappings to do/find*/
{
	int					RetVal;				/* Return value					*/
	int					fd;					/* File descriptor				*/
	uint_t				count;				/* Loop counter					*/
	uint_t				limit;				/* Local copy of nmaps			*/
	struct cdfs_IocArgs	GenArgs;			/* Generic argument structure	*/
	struct cdfs_IDMapArgs Args;				/* Arguments for ioctl			*/
	struct mnttab		MntEnt;				/* Mount table entry			*/

	cd_DoSignal();

	/*
	 * Reject if this is not a cdfs mount-point.
	 */
	if ((!cd_FsMember (path)) || (!cd_IsMntPt (path, &MntEnt))) {
		cd_UndoSignal();
		return (-1);
	}
	if ((idmap == NULL) || (nmaps == NULL)) {
		cd_UndoSignal();
		return (-1);
	}
	if ((*nmaps < 0) || ((cmd != CD_SETUMAP) && (cmd != CD_GETUMAP) &&
				(cmd != CD_SETGMAP) && (cmd != CD_GETGMAP))) {
		errno = EINVAL;
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * Initialize count to 0.
	 */
	Args.count = 0;
	Args.IDMapCmd = cmd;

	GenArgs.PathName = (uchar_t *) path;
	GenArgs.ArgPtr = &Args;

	fd = open (path, O_RDONLY);
	if (fd < 0) {
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * If we're clearing all values (*nmaps == 0), traverse all mappings
	 * (up to the maximum allowed), otherwise do the requested number
	 * (*nmaps).
	 */
	if ((cmd == CD_SETUMAP) || (cmd == CD_GETUMAP)) {
		limit = ((*nmaps == 0) ? CD_MAXUMAP : *nmaps);
		limit = ((*nmaps > CD_MAXUMAP) ? CD_MAXUMAP : limit);
	}
	if ((cmd == CD_SETGMAP) || (cmd == CD_GETGMAP)) {
		limit = ((*nmaps == 0) ? CD_MAXGMAP : *nmaps);
		limit = ((*nmaps > CD_MAXGMAP) ? CD_MAXGMAP : limit);
	}

	/*
	 * Loop through all the requested mappings, either setting or getting
	 * values.
	 */
	for (count = 0; count < limit; count++) {
		if ((cmd == CD_GETUMAP) || (cmd == CD_GETGMAP)) {
			Args.count = count + 1;
		} else {
			Args.idmap.from_id = idmap[count].from_id;
			if (Args.idmap.from_id > (ushort_t) MAXUID) {
				(void) close (fd);
				*nmaps = count;
				errno = EINVAL;
				cd_UndoSignal();
				return (-1);
			}
			if (*nmaps == 0) {
				if (cmd == CD_SETUMAP) {
					Args.idmap.to_uid = CDFS_UNUSED_MAP_ENTRY;
				} else {
					Args.idmap.to_gid = CDFS_UNUSED_MAP_ENTRY;
				}
			} else {
				if (cmd == CD_SETUMAP) {
					Args.idmap.to_uid = idmap[count].to_uid;
					if (Args.idmap.to_uid > MAXUID) {
						(void) close (fd);
						*nmaps = count;
						errno = EINVAL;
						cd_UndoSignal();
						return (-1);
					}
				} else {
					Args.idmap.to_gid = idmap[count].to_gid;
					if (Args.idmap.to_gid > MAXUID) {
						(void) close (fd);
						*nmaps = count;
						errno = EINVAL;
						cd_UndoSignal();
						return (-1);
					}
				}
			}
		}
	
		RetVal = ioctl (fd, CDFS_DOIDMAP, &GenArgs);

		/*
		 * Note that ENOMATCH means that we ran off the end of the valid
		 * mappings without finding what we were looking for.
		 */
		if (RetVal < 0) {
			if (errno == ENOMATCH) {
				break;
			} else {
				(void) close (fd);
				cd_UndoSignal();
				return (-1);
			}
		}
	
		/*
		 * If we're getting mappings, copy results into the user's buffer.
		 */
		if ((cmd == CD_GETUMAP) || (cmd == CD_GETGMAP)) {
			idmap[count].from_id = Args.idmap.from_id;
			if (Args.idmap.from_id > (ushort_t) MAXUID) {
				*nmaps = count;
				errno = EINVAL;
				(void) close (fd);
				cd_UndoSignal();
				return (-1);
			}
			if (cmd == CD_GETUMAP) {
				idmap[count].to_uid = Args.idmap.to_uid;
				if (Args.idmap.to_uid > MAXUID) {
					*nmaps = count;
					errno = EINVAL;
					(void) close (fd);
					cd_UndoSignal();
					return (-1);
				}
			} else {
				idmap[count].to_gid = Args.idmap.to_gid;
				if (Args.idmap.to_gid > MAXUID) {
					*nmaps = count;
					errno = EINVAL;
					(void) close (fd);
					cd_UndoSignal();
					return (-1);
				}
			}
		}
	}
	(void) close (fd);

	/*
	 * Pass back the number of mappings that are being returned.
	 */
	if ((cmd == CD_GETUMAP) || (cmd == CD_GETGMAP)) {
		*nmaps = count;
	}
	cd_UndoSignal();
	return (0);
}





/*
 * cd_nmconv() - Set and get CD-ROM file name conversion.
 */

int
cd_nmconv (path, cmd, flag)
const	char		*path;		/* CD-ROM mount point		*/
int			cmd;		/* Operation to perform		*/
int			*flag;		/* Kind of mapping to perform	*/
{
	int					RetVal;				/* Return value					*/
	int					fd;					/* File descriptor				*/
	struct cdfs_IocArgs	GenArgs;			/* Generic argument structure	*/
	struct cdfs_NMConvArgs Args;			/* Arguments for ioctl			*/
	struct mnttab		MntEnt;				/* Mount table entry			*/

	cd_DoSignal();

	/*
	 * Reject if this is not a cdfs mount-point.
	 */
	if ((!cd_FsMember (path)) || (!cd_IsMntPt (path, &MntEnt))) {
		cd_UndoSignal();
		return (-1);
	}
	if (flag == NULL) {
		errno = EFAULT;
		cd_UndoSignal();
		return (-1);
	}
	if ((cmd != CD_SETNMCONV) && (cmd != CD_GETNMCONV)) {
		errno = EINVAL;
		cd_UndoSignal();
		return (-1);
	}
	if ((cmd == CD_SETNMCONV) &&
				((*flag & (CD_NOCONV | CD_LOWER | CD_NOVERSION)) == 0)) {
		errno = EINVAL;
		cd_UndoSignal();
		return (-1);
	}

	Args.ConvCmd = cmd;
	if (cmd == CD_SETNMCONV) {
		Args.conv_flags = *flag;
	}

	GenArgs.PathName = (uchar_t *) path;
	GenArgs.ArgPtr = &Args;

	/*
	 * Get or set the name conversion.
	 */
	fd = open (path, O_RDONLY);
	if (fd < 0) {
		cd_UndoSignal();
		return (-1);
	}

	RetVal = ioctl (fd, CDFS_DONMCONV, &GenArgs);
	(void) close (fd);
	cd_UndoSignal();
	if (RetVal < 0) {
		return (-1);
	}

	if (cmd == CD_GETNMCONV) {
		*flag = Args.conv_flags;
	}

	return (0);
}





/*
 * cd_suf() - Read System Use Field from a specified System Use Area.
 */

int
cd_suf (path, fsec, signature, index, buf, buflen)
const char		*path;		/* Ptr to file/directory	*/
int			fsec;		/* File section			*/
char			*signature;	/* SUF signature		*/
int			index;		/* Occurrence of signature to use*/
char			*buf;		/* Buffer that holds SUF	*/
int			buflen;		/* Length of buf	*/
{
	int					RetVal;				/* Return value					*/
	int					fd;					/* File descriptor				*/
	struct cdfs_IocArgs	GenArgs;			/* Generic argument structure	*/
	struct cdfs_SUFArgs	Args;				/* Arguments to ioctl			*/
	uchar_t				MntPt[MAXPATHLEN];	/* Mount point of CD-ROM		*/
	struct mnttab		MntEnt;				/* Mount table entry			*/

	cd_DoSignal();

	if (!cd_FsMember (path)) {
		cd_UndoSignal();
		return (-1);
	}
	if ((fsec == 0) || (fsec < -1)) {
		errno = ENXIO;
		cd_UndoSignal();
		return (-1);
	}
	if ((index <= 0) || (buflen < 0)) {
		errno = EINVAL;
		cd_UndoSignal();
		return (-1);
	}
	if (buf == NULL) {
		errno = EFAULT;
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * Initialize arguments.
	 */
	Args.SUF = (struct susp_suf *) buf;
	Args.length = buflen;
	Args.fsec = fsec;
	if (signature == NULL) {
		Args.signature = NULL;
	} else {
		Args.signature = signature;
	}
	Args.index = index;

	/*
	 * Find the mount point of the CD-ROM to operate on.
	 */
	if (!cd_IsMntPt (path, &MntEnt)) {
		if (MntEnt.mnt_mountp == NULL) {
			cd_UndoSignal();
			return (-1);
		}
		(void) strcpy ((char *) MntPt, MntEnt.mnt_mountp);
	} else {
		(void) strcpy ((char *) MntPt, path);
	}

	GenArgs.PathName = (uchar_t *) path;
	GenArgs.ArgPtr = &Args;

	/*
	 * Get the SUF.
	 */
	fd = open ((char *) MntPt, O_RDONLY);
	if (fd < 0) {
		cd_UndoSignal();
		return (-1);
	}

	RetVal = ioctl (fd, CDFS_GETSUF, &GenArgs);
	(void) close (fd);
	cd_UndoSignal();
	if (RetVal < 0) {
		if (errno == ENOMATCH) {
			return (0);
		} else {
			return (-1);
		}
	}

	return ((int) ((struct susp_suf *) buf)->suf_Len);
}





/*
 * cd_setdevmap() - Set mappings of major/minor numbers.
 */

int
cd_setdevmap (path, cmd, new_major, new_minor)
const char		*path;		/* Ptr to device node path	*/
int			cmd;		/* Operation to perform		*/
int			*new_major;	/* New device major number	*/
int			*new_minor;	/* New device minor number	*/
{
	int					RetVal;				/* Return value					*/
	int					fd;					/* File descriptor				*/
	struct cdfs_IocArgs	GenArgs;			/* Generic argument structure	*/
	struct cdfs_DevMapArgs Args;			/* Arguments to ioctl			*/
	uchar_t				MntPt[MAXPATHLEN];	/* Mount point of CD-ROM		*/
	struct mnttab		MntEnt;				/* Mount table entry			*/
	struct stat			StatStruct;			/* Results of stat() call		*/

	cd_DoSignal();

	if (!cd_FsMember (path)) {
		cd_UndoSignal();
		return (-1);
	}
	if ((new_major == NULL) || (new_minor == NULL)) {
		errno = EFAULT;
		cd_UndoSignal();
		return (-1);
	}
	if (((cmd != CD_SETDMAP) && (cmd != CD_UNSETDMAP)) ||
					(*new_major > MAXMAJ) || (*new_minor > MAXMIN)) {
		errno = EINVAL;
		cd_UndoSignal();
		return (-1);
	}

	RetVal = stat (path, &StatStruct);
	if (RetVal != 0) {
		cd_UndoSignal();
		return (-1);
	}

	if (((StatStruct.st_mode & S_IFBLK) != S_IFBLK) &&
				((StatStruct.st_mode & S_IFCHR) != S_IFCHR)) {
		errno = EINVAL;
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * Initialize arguments.
	 */
	Args.DevCmd = cmd;
	if (cmd == CD_SETDMAP) {
		Args.new_major = *new_major;
		Args.new_minor = *new_minor;
	}

	/*
	 * Find the mount point of the CD-ROM to operate on, rather than opening
	 * a device node.  Path SHOULD never be the same as "mount point", but
	 * let's make sure.
	 */
	if (!cd_IsMntPt (path, &MntEnt)) {
		if (MntEnt.mnt_mountp == NULL) {
			return (-1);
		}
		(void) strcpy ((char *) MntPt, MntEnt.mnt_mountp);
	} else {
		errno = EINVAL;
		cd_UndoSignal();
		return (-1);
	}

	GenArgs.PathName = (uchar_t *) path;
	GenArgs.ArgPtr = &Args;

	/*
	 * Set the device mapping.
	 */
	fd = open ((char *) MntPt, O_RDONLY);
	if (fd < 0) {
		cd_UndoSignal();
		return (-1);
	}

	RetVal = ioctl (fd, CDFS_SETDEVMAP, &GenArgs);
	(void) close (fd);
	if (RetVal < 0) {
		if (errno == ENOMATCH) {
			RetVal = 0;
		} else {
			RetVal = -1;
		}
	} else {
		RetVal = 1;
	}

	if ((cmd == CD_SETDMAP) || (cmd == CD_UNSETDMAP)){
		*new_major = Args.new_major;
		*new_minor = Args.new_minor;
	}

	cd_UndoSignal();
	return (RetVal);
}





/*
 * cd_getdevmap() - Get mappings of major/minor numbers.
 */

int
cd_getdevmap (path, pathlen, index, new_major, new_minor)
char		*path;		/* Ptr to device node path	*/
int			pathlen;	/* Length of path (if index set)*/
int			index;		/* Occurrence of mapping to use	*/
int			*new_major;	/* New device major number	*/
int			*new_minor;	/* New device minor number	*/
{
	int			RetVal;		/* Return value		*/
	int			fd;		/* File descriptor	*/
	struct cdfs_IocArgs	GenArgs;	/* Generic argument structure*/
	struct cdfs_DevMapArgs Args;		/* Argument structure for ioctl	*/
	uchar_t		MntPt[MAXPATHLEN];	/* Mount point of CD-ROM*/
	uchar_t		TmpPath[MAXPATHLEN];	/* Working pathname	*/
	struct mnttab		MntEnt;		/* Mount table entry	*/

	cd_DoSignal();

	if (!cd_FsMember (path)) {
		cd_UndoSignal();
		return (-1);
	}
	if ((new_major == NULL) || (new_minor == NULL)) {
		errno = EFAULT;
		cd_UndoSignal();
		return (-1);
	}
	if ((index < 0) || ((index > 0) && (pathlen < 0))) {
		errno = EINVAL;
		cd_UndoSignal();
		return (-1);
	}

	/*
	 * Set the ioctl arguments.
	 */
	Args.index = index;
	(void) strcpy ((char *) TmpPath, path);

	/*
	 * Find the mount point of the CD-ROM to operate on, rather than opening
	 * a device node.  If we get a mount point as an argument, that means
	 * we don't know the device path yet.
	 */
	if (!cd_IsMntPt (path, &MntEnt)) {
		if (MntEnt.mnt_mountp == NULL) {
			return (-1);
		}
		(void) strcpy ((char *) MntPt, MntEnt.mnt_mountp);
	} else {
		(void) strcpy ((char *) MntPt, path);
	}

	GenArgs.PathName = TmpPath;
	GenArgs.ArgPtr = &Args;

	/*
	 * Get the device mapping.
	 */
	fd = open ((char *) MntPt, O_RDONLY);
	if (fd < 0) {
		cd_UndoSignal();
		return (-1);
	}

	RetVal = ioctl (fd, CDFS_GETDEVMAP, &GenArgs);
	(void) close (fd);
	if (RetVal < 0) {
		/*
		 * Handle the case where there are no mappings.
		 */
		cd_UndoSignal();
		if (errno == ENOMATCH) {
			return (0);
		} else {
			return (-1);
		}
	}

	*new_major = Args.new_major;
	*new_minor = Args.new_minor;

	/*
	 * Pass back the device file name, truncated, if necessary.
	 */
	(void) strncpy (path, (char *) TmpPath, pathlen);
	RetVal = strlen ((char *) TmpPath);

	cd_UndoSignal();
	return (RetVal);
}





/*
 * Print a string that's not (necessarily) null-terminated, with a
 * newline after it.
 */
static void
cd_PrintString (string, size)
const uchar_t	*string;	/* Pointer to characters to print	*/
const uint_t	size;		/* Number of characters to print	*/
{
	register int	i, end;				/* Loop indices						*/

	/*
	 * Make sure this is a defined string.  A string of all spaces is
	 * classified as "not defined".
	 */
	for (end = size - 1; end >= 0; end--) {
		if (string[end] != ' ') {
			break;
		}
	}
	if ((end < 0) || (string[end] == '\0')) {
		(void) printf ("Not defined\n");
		return;
	}

	for (i = 0; i <= end; i++) {
		if (string[i] == '\0') {
			break;
		}
		(void) putchar (string[i]);
	}
	(void) putchar ('\n');

	return;
}





/*
 * Generic routine for handling caught signals.
 */
/* ARGSUSED */
static void
cd_signal (signame)
int		signame;	/* Name of signal			*/
{
	exit (EINTR);
}


/*
 * Set up interrupts.  We need to do this to catch signals while we're
 * inside the library, so we set up all signals to be handled by the
 * above routine.
 *
 * Note that we only reassign the signal handlers if we haven't done
 * them already.
 *
 */
static void
cd_DoSignal ()
{
	if (oldhup == NULL) {
		oldhup = signal (SIGHUP, cd_signal);
	}
	if (oldint == NULL) {
		oldint = signal (SIGINT, cd_signal);
	}
	if (oldquit == NULL) {
		oldquit = signal (SIGQUIT, cd_signal);
	}
	if (oldsys == NULL) {
		oldsys = signal (SIGSYS, cd_signal);
	}

	return;
}


/*
 * Restore original interrupts.
 */
static void
cd_UndoSignal ()
{
	(void) signal (SIGHUP, oldhup);
	(void) signal (SIGINT, oldint);
	(void) signal (SIGQUIT, oldquit);
	(void) signal (SIGSYS, oldsys);

	/*
	 * Clear the function pointers, so we know we don't have any
	 * signal handlers saved.
	 */
	oldhup = oldint = oldquit = oldsys = NULL;

	return;
}









/*
 * cd_FillPvd - Fill the right fields of the XCDR PVD structure from the
 * media version.
 */
static int
cd_FillPvd (xcdr_pvd, from_pvd, type)
struct iso9660_pvd	*xcdr_pvd;	/* XCDR PVD structure to fill	*/
union media_pvd		*from_pvd;	/* Media copy of PVD		*/
enum cdfs_type		type;		/* Type of file system		*/
{

	if ((xcdr_pvd == NULL) || (from_pvd == NULL)) {
		errno = EFAULT;
		return (-1);
	}

	switch (type) {
		case CDFS_ISO_9660: {
			xcdr_pvd->voldestype = from_pvd->Iso.pvd_Type;
			xcdr_pvd->voldesvers = from_pvd->Iso.pvd_Ver;
			xcdr_pvd->volspcsize = from_pvd->Iso.pvd_VolSpcSz;
			xcdr_pvd->volsetsize = from_pvd->Iso.pvd_VolSetSz;
			xcdr_pvd->volseqno = from_pvd->Iso.pvd_VolSeqNum;
			xcdr_pvd->lblksize = from_pvd->Iso.pvd_LogBlkSz;
			xcdr_pvd->ptsize = from_pvd->Iso.pvd_PathTabSz;
			xcdr_pvd->locpt_l = from_pvd->Iso.pvd_PathTabLoc;
			xcdr_pvd->locptopt_l = from_pvd->Iso.pvd_OptPathTabLoc;
			xcdr_pvd->locpt_m = from_pvd->Iso.pvd_PathTabLoc_rev;
			xcdr_pvd->locptopt_m = from_pvd->Iso.pvd_OptPathTabLoc_rev;
			xcdr_pvd->filestrver = from_pvd->Iso.pvd_FileVer;
			xcdr_pvd->res1 = from_pvd->Iso.pvd_Rsvd4;

			xcdr_pvd->cre_time =
						cd_iso_adtToDate (&from_pvd->Iso.pvd_CreateDate);
			xcdr_pvd->mod_time =
						cd_iso_adtToDate (&from_pvd->Iso.pvd_ModDate);
			xcdr_pvd->exp_time =
						cd_iso_adtToDate (&from_pvd->Iso.pvd_ExpireDate);
			xcdr_pvd->eff_time =
						cd_iso_adtToDate (&from_pvd->Iso.pvd_EffectDate);

			cd_FillField (&xcdr_pvd->std_id, sizeof (xcdr_pvd->std_id),
						from_pvd->Iso.pvd_StdID,
						sizeof (from_pvd->Iso.pvd_StdID), CDFS_STRING);
			cd_FillField (&xcdr_pvd->sys_id, sizeof (xcdr_pvd->sys_id),
						from_pvd->Iso.pvd_SysID,
						sizeof (from_pvd->Iso.pvd_SysID), CDFS_STRING);
			cd_FillField (&xcdr_pvd->vol_id, sizeof (xcdr_pvd->vol_id),
						from_pvd->Iso.pvd_VolID,
						sizeof (from_pvd->Iso.pvd_VolID), CDFS_STRING);
			cd_FillField (&xcdr_pvd->volset_id, sizeof (xcdr_pvd->volset_id),
						from_pvd->Iso.pvd_VolSetID,
						sizeof (from_pvd->Iso.pvd_VolSetID), CDFS_STRING);
			cd_FillField (&xcdr_pvd->pub_id, sizeof (xcdr_pvd->pub_id),
						from_pvd->Iso.pvd_PublsherID,
						sizeof (from_pvd->Iso.pvd_PublsherID), CDFS_STRING);
			cd_FillField (&xcdr_pvd->dtpre_id, sizeof (xcdr_pvd->dtpre_id),
						from_pvd->Iso.pvd_PreparerID,
						sizeof (from_pvd->Iso.pvd_PreparerID), CDFS_STRING);
			cd_FillField (&xcdr_pvd->app_id, sizeof (xcdr_pvd->app_id),
						from_pvd->Iso.pvd_ApplID,
						sizeof (from_pvd->Iso.pvd_ApplID), CDFS_STRING);
			cd_FillField (&xcdr_pvd->cpfile_id, sizeof (xcdr_pvd->cpfile_id),
						from_pvd->Iso.pvd_CopyrightFile,
						sizeof (from_pvd->Iso.pvd_CopyrightFile), CDFS_STRING);
			cd_FillField (&xcdr_pvd->abfile_id, sizeof (xcdr_pvd->abfile_id),
						from_pvd->Iso.pvd_AbstractFile,
						sizeof (from_pvd->Iso.pvd_AbstractFile), CDFS_STRING);
			cd_FillField (&xcdr_pvd->bgfile_id, sizeof (xcdr_pvd->bgfile_id),
						from_pvd->Iso.pvd_BiblioFile,
						sizeof (from_pvd->Iso.pvd_BiblioFile), CDFS_STRING);
			cd_FillField (&xcdr_pvd->rootdir, sizeof (xcdr_pvd->rootdir),
						from_pvd->Iso.pvd_RootDir,
						sizeof (from_pvd->Iso.pvd_RootDir), CDFS_BINARY_FIELD);
			cd_FillField (&xcdr_pvd->appuse, sizeof (xcdr_pvd->appuse),
						from_pvd->Iso.pvd_ApplUse,
						sizeof (from_pvd->Iso.pvd_ApplUse), CDFS_BINARY_FIELD);
			cd_FillField (&xcdr_pvd->res2, sizeof (xcdr_pvd->res2),
						from_pvd->Iso.pvd_Rsvd5,
						sizeof (from_pvd->Iso.pvd_Rsvd5), CDFS_BINARY_FIELD);
			break;
		}
		case CDFS_HIGH_SIERRA: {
			xcdr_pvd->voldestype = from_pvd->Hs.pvd_Type;
			xcdr_pvd->voldesvers = from_pvd->Hs.pvd_Ver;
			xcdr_pvd->volspcsize = from_pvd->Hs.pvd_VolSpcSz;
			xcdr_pvd->volsetsize = from_pvd->Hs.pvd_VolSetSz;
			xcdr_pvd->volseqno = from_pvd->Hs.pvd_VolSeqNum;
			xcdr_pvd->lblksize = from_pvd->Hs.pvd_LogBlkSz;
			xcdr_pvd->ptsize = from_pvd->Hs.pvd_PathTabSz;
			xcdr_pvd->locpt_l = from_pvd->Hs.pvd_PathTabLoc;
			xcdr_pvd->locptopt_l = from_pvd->Hs.pvd_Opt1PathTabLoc;
			xcdr_pvd->locpt_m = from_pvd->Hs.pvd_PathTabLoc_rev;
			xcdr_pvd->locptopt_m = from_pvd->Hs.pvd_Opt1PathTabLoc_rev;
			xcdr_pvd->filestrver = from_pvd->Hs.pvd_FileVer;
			xcdr_pvd->res1 = from_pvd->Hs.pvd_Rsvd4;

			xcdr_pvd->cre_time =
						cd_hs_adtToDate (&from_pvd->Hs.pvd_CreateDate);
			xcdr_pvd->mod_time =
						cd_hs_adtToDate (&from_pvd->Hs.pvd_ModDate);
			xcdr_pvd->exp_time =
						cd_hs_adtToDate (&from_pvd->Hs.pvd_ExpireDate);
			xcdr_pvd->eff_time =
						cd_hs_adtToDate (&from_pvd->Hs.pvd_EffectDate);

			cd_FillField (&xcdr_pvd->std_id, sizeof (xcdr_pvd->std_id),
						from_pvd->Hs.pvd_StdID,
						sizeof (from_pvd->Hs.pvd_StdID), CDFS_STRING);
			cd_FillField (&xcdr_pvd->sys_id, sizeof (xcdr_pvd->sys_id),
						from_pvd->Hs.pvd_SysID,
						sizeof (from_pvd->Hs.pvd_SysID), CDFS_STRING);
			cd_FillField (&xcdr_pvd->vol_id, sizeof (xcdr_pvd->vol_id),
						from_pvd->Hs.pvd_VolID,
						sizeof (from_pvd->Hs.pvd_VolID), CDFS_STRING);
			cd_FillField (&xcdr_pvd->volset_id, sizeof (xcdr_pvd->volset_id),
						from_pvd->Hs.pvd_VolSetID,
						sizeof (from_pvd->Hs.pvd_VolSetID), CDFS_STRING);
			cd_FillField (&xcdr_pvd->pub_id, sizeof (xcdr_pvd->pub_id),
						from_pvd->Hs.pvd_PublsherID,
						sizeof (from_pvd->Hs.pvd_PublsherID), CDFS_STRING);
			cd_FillField (&xcdr_pvd->dtpre_id, sizeof (xcdr_pvd->dtpre_id),
						from_pvd->Hs.pvd_PreparerID,
						sizeof (from_pvd->Hs.pvd_PreparerID), CDFS_STRING);
			cd_FillField (&xcdr_pvd->app_id, sizeof (xcdr_pvd->app_id),
						from_pvd->Hs.pvd_ApplID,
						sizeof (from_pvd->Hs.pvd_ApplID), CDFS_STRING);
			cd_FillField (&xcdr_pvd->cpfile_id, sizeof (xcdr_pvd->cpfile_id),
						from_pvd->Hs.pvd_CopyrightFile,
						sizeof (from_pvd->Hs.pvd_CopyrightFile), CDFS_STRING);
			cd_FillField (&xcdr_pvd->abfile_id, sizeof (xcdr_pvd->abfile_id),
						from_pvd->Hs.pvd_AbstractFile,
						sizeof (from_pvd->Hs.pvd_AbstractFile), CDFS_STRING);
			cd_FillField (&xcdr_pvd->rootdir, sizeof (xcdr_pvd->rootdir),
						from_pvd->Hs.pvd_RootDir,
						sizeof (from_pvd->Hs.pvd_RootDir), CDFS_BINARY_FIELD);
			cd_FillField (&xcdr_pvd->appuse, sizeof (xcdr_pvd->appuse),
						from_pvd->Hs.pvd_ApplUse,
						sizeof (from_pvd->Hs.pvd_ApplUse), CDFS_BINARY_FIELD);
			cd_FillField (&xcdr_pvd->res2, sizeof (xcdr_pvd->res2),
						from_pvd->Hs.pvd_Rsvd5,
						sizeof (from_pvd->Hs.pvd_Rsvd5), CDFS_BINARY_FIELD);

			/*
			 * High Sierra doesn't have this field.
			 */
			(void) memset ((char *) &xcdr_pvd->bgfile_id, '\0',
						sizeof (xcdr_pvd->bgfile_id));

			break;
		}
		default: {
			(void) fprintf(stderr, "%s (Unrecognized format)\n", CDFS_ID);
			break;
		}
	}

	return (0);
}





/*
 * cd_FillXar - Fill the right fields of the XCDR XAR structure from the
 * media version.
 */
static int
cd_FillXar (xcdr_xar, from_xar, applen, esclen, type)
struct iso9660_xar	*xcdr_xar;	/* XAR structure to fill	*/
union media_xar		*from_xar;	/* Media copy of XAR		*/
int			applen;		/* Length of App Use field	*/
int			esclen;		/* Length of Esc Seq field	*/
enum cdfs_type		type;		/* Type of file system		*/
{
	if ((xcdr_xar == NULL) || (from_xar == NULL)) {
		errno = EFAULT;
		return (-1);
	}

	switch (type) {
		case CDFS_ISO_9660: {
			xcdr_xar->own_id = from_xar->Iso.xar_User;
			xcdr_xar->grp_id = from_xar->Iso.xar_Group;
			xcdr_xar->rec_form = from_xar->Iso.xar_RecFmt;
			xcdr_xar->rec_attr = from_xar->Iso.xar_RecAttr;
			xcdr_xar->rec_len = from_xar->Iso.xar_RecLen;
			xcdr_xar->xar_vers = from_xar->Iso.xar_Ver;
			xcdr_xar->esc_len = from_xar->Iso.xar_EscSeqLen;
			xcdr_xar->appuse_len = from_xar->Iso.xar_ApplUseLen;

			xcdr_xar->permissions = cd_GetPerms (from_xar->Iso.xar_Perms1,
						from_xar->Iso.xar_Perms2);

			xcdr_xar->cre_time =
						cd_iso_adtToDate (&from_xar->Iso.xar_CreateDate);
			xcdr_xar->mod_time =
						cd_iso_adtToDate (&from_xar->Iso.xar_ModDate);
			xcdr_xar->exp_time =
						cd_iso_adtToDate (&from_xar->Iso.xar_ExpireDate);
			xcdr_xar->eff_time =
						cd_iso_adtToDate (&from_xar->Iso.xar_EffectDate);


			cd_FillField (&xcdr_xar->sys_id, sizeof (xcdr_xar->sys_id),
						from_xar->Iso.xar_SysID,
						sizeof (from_xar->Iso.xar_SysID), CDFS_BINARY_FIELD);
			cd_FillField (&xcdr_xar->sys_use, sizeof (xcdr_xar->sys_use),
						from_xar->Iso.xar_SysUse,
						sizeof (from_xar->Iso.xar_SysUse), CDFS_BINARY_FIELD);
			cd_FillField (&xcdr_xar->resv, sizeof (xcdr_xar->resv),
						from_xar->Iso.xar_Rsvd,
						sizeof (from_xar->Iso.xar_Rsvd), CDFS_BINARY_FIELD);


			if (applen > 0) {
				cd_FillField (&xcdr_xar->app_use, sizeof (xcdr_xar->app_use),
							&from_xar->Iso.xar_VarData, applen,
							CDFS_BINARY_FIELD);
			}
			if (esclen > 0) {
				cd_FillField (&xcdr_xar->esc_seq, sizeof (xcdr_xar->esc_seq),
							&from_xar->Iso.xar_VarData + applen, esclen,
							CDFS_BINARY_FIELD);
			}
			break;
		}
		case CDFS_HIGH_SIERRA: {
			xcdr_xar->own_id = from_xar->Hs.xar_User;
			xcdr_xar->grp_id = from_xar->Hs.xar_Group;
			xcdr_xar->rec_form = from_xar->Hs.xar_RecFmt;
			xcdr_xar->rec_attr = from_xar->Hs.xar_RecAttr;
			xcdr_xar->rec_len = from_xar->Hs.xar_RecLen;
			xcdr_xar->xar_vers = from_xar->Hs.xar_Ver;
			/*
			 * High Sierra doesn't have this field.
			 */
			xcdr_xar->esc_len = 0;

			xcdr_xar->appuse_len = from_xar->Hs.xar_ApplUseLen;

			xcdr_xar->permissions = cd_GetPerms (from_xar->Hs.xar_Perms1,
						from_xar->Hs.xar_Perms2);

			xcdr_xar->cre_time =
						cd_hs_adtToDate (&from_xar->Hs.xar_CreateDate);
			xcdr_xar->mod_time =
						cd_hs_adtToDate (&from_xar->Hs.xar_ModDate);
			xcdr_xar->exp_time =
						cd_hs_adtToDate (&from_xar->Hs.xar_ExpireDate);
			xcdr_xar->eff_time =
						cd_hs_adtToDate (&from_xar->Hs.xar_EffectDate);

			cd_FillField (&xcdr_xar->sys_id, sizeof (xcdr_xar->sys_id),
						from_xar->Hs.xar_SysID,
						sizeof (from_xar->Hs.xar_SysID), CDFS_BINARY_FIELD);
			cd_FillField (&xcdr_xar->sys_use, sizeof (xcdr_xar->sys_use),
						from_xar->Hs.xar_Rsvd1,
						sizeof (from_xar->Hs.xar_Rsvd1), CDFS_BINARY_FIELD);
			cd_FillField (&xcdr_xar->resv, sizeof (xcdr_xar->resv),
						from_xar->Hs.xar_Rsvd2,
						sizeof (from_xar->Hs.xar_Rsvd2), CDFS_BINARY_FIELD);


			if (applen > 0) {
				cd_FillField (&xcdr_xar->app_use, sizeof (xcdr_xar->app_use),
							&from_xar->Hs.xar_ApplRes, applen,
							CDFS_BINARY_FIELD);
			}

			
			/*
			 * High Sierra doesn't have this field.
			 */
			*(xcdr_xar->esc_seq) = 0;

			/*
			 * High Sierra DOES have a directory record field, but there's
			 * no place in the iso9660_xar struct for it.
			 */

			break;
		}
		default: {
			(void) fprintf(stderr, "%s (Unrecognized format)\n", CDFS_ID);
			break;
		}
	}

	return (0);
}




/*
 * cd_FillDRec - Fill an XCDR DREC structure from the media version of the
 * same thing.
 */
static int
cd_FillDRec (xcdr_drec, from_drec, type)
struct iso9660_drec	*xcdr_drec;		/* XCDR DREC to fill	*/
union media_drec	*from_drec;		/* Media version of DREC*/
enum cdfs_type		type;			/* Type of file system	*/
{
	uint_t					count;			/* Loop counter					*/
	uchar_t					*TmpPtr;		/* Temporary pointer			*/

	if ((xcdr_drec == NULL) || (from_drec == NULL)) {
		errno = EFAULT;
		return (-1);
	}

	switch (type) {
		case CDFS_ISO_9660: {
			xcdr_drec->drec_len = from_drec->Iso.drec_Size;
			xcdr_drec->xar_len = from_drec->Iso.drec_XarSize;
			xcdr_drec->locext = from_drec->Iso.drec_ExtentLoc;
			xcdr_drec->data_len = from_drec->Iso.drec_DataSz;
			xcdr_drec->file_flags = from_drec->Iso.drec_Flags;
			xcdr_drec->file_usize = from_drec->Iso.drec_UnitSz;
			xcdr_drec->ileav_gsize = from_drec->Iso.drec_Interleave;
			xcdr_drec->volseqno = from_drec->Iso.drec_VolSeqNum;
			xcdr_drec->fileid_len = from_drec->Iso.drec_FileIDSz;

			/*
			 * Note that this differs from the algorithm given in the XCDR.
			 * It says to use 34 as the fixed-length portion of the DREC - it's
			 * really 33, as defined in ISO_DREC_FIXEDLEN.
			 */
			xcdr_drec->sysuse_len = xcdr_drec->drec_len -
						xcdr_drec->fileid_len - ISO_DREC_FIXEDLEN -
						(((xcdr_drec->fileid_len % 2) == 0) ? 1 : 0);

			xcdr_drec->rec_time =
						cd_iso_hdtToDate (&from_drec->Iso.drec_RecordDate);

			/*
			 * Special-case the root directory fileid, otherwise, strip
			 * any leading nulls.
			 */
			if ((xcdr_drec->fileid_len == 1) &&
						(from_drec->Iso.drec_VarData == '\0')) {
				xcdr_drec->file_id[0] = '\0';
			} else {
				TmpPtr = (uchar_t *) &from_drec->Iso.drec_VarData;
				for (count = 0; count < xcdr_drec->fileid_len; count++) {
					if (*TmpPtr == '\0') {
						TmpPtr++;
					} else {
						break;
					}
				}
				xcdr_drec->fileid_len -= count;
				cd_FillField (&xcdr_drec->file_id, sizeof (xcdr_drec->file_id),
						TmpPtr, xcdr_drec->fileid_len, CDFS_STRING);
			}

			cd_FillField (&xcdr_drec->sys_use, sizeof (xcdr_drec->sys_use),
						(&from_drec->Iso.drec_VarData) + xcdr_drec->fileid_len,
						xcdr_drec->sysuse_len, CDFS_BINARY_FIELD);
			break;
		}
		case CDFS_HIGH_SIERRA: {
			xcdr_drec->drec_len = from_drec->Hs.drec_Size;
			xcdr_drec->xar_len = from_drec->Hs.drec_XarSize;
			xcdr_drec->locext = from_drec->Hs.drec_ExtentLoc;
			xcdr_drec->data_len = from_drec->Hs.drec_DataSz;
			xcdr_drec->file_flags = from_drec->Hs.drec_Flags;

			/*
			 * High Sierra doesn't specify the contents of this field.
			 */
			xcdr_drec->file_usize = from_drec->Hs.drec_unused1;
			xcdr_drec->ileav_gsize = from_drec->Hs.drec_InterleaveSz;

			/*
			 * High Sierra DOES have an Interleave Skip field, but there's
			 * no place in the iso9660_xar struct for it.
			 */

			xcdr_drec->volseqno = from_drec->Hs.drec_VolSeqNum;
			xcdr_drec->fileid_len = from_drec->Hs.drec_FileIDSz;

			/*
			 * Note that this differs from the algorithm given in the XCDR.
			 * It says to use 34 as the fixed-length portion of the DREC - it's
			 * really 33, as defined in ISO_DREC_FIXEDLEN.
			 */
			xcdr_drec->sysuse_len = xcdr_drec->drec_len -
						xcdr_drec->fileid_len - ISO_DREC_FIXEDLEN -
						(((xcdr_drec->fileid_len % 2) == 0) ? 1 : 0);

			xcdr_drec->rec_time =
						cd_hs_hdtToDate (&from_drec->Hs.drec_RecordDate);

			/*
			 * Special-case the root directory fileid, otherwise, strip
			 * any leading nulls.
			 */
			if ((xcdr_drec->fileid_len == 1) &&
						(from_drec->Hs.drec_VarData == '\0')) {
				xcdr_drec->file_id[0] = '\0';
			} else {
				TmpPtr = (uchar_t *) &from_drec->Hs.drec_VarData;
				for (count = 0; count < xcdr_drec->fileid_len; count++) {
					if (*TmpPtr == '\0') {
						TmpPtr++;
					} else {
						break;
					}
				}
				xcdr_drec->fileid_len -= count;
				cd_FillField (&xcdr_drec->file_id, sizeof (xcdr_drec->file_id),
							TmpPtr, xcdr_drec->fileid_len, CDFS_STRING);
			}

			cd_FillField (&xcdr_drec->sys_use, sizeof (xcdr_drec->sys_use),
						(&from_drec->Hs.drec_VarData) + xcdr_drec->fileid_len,
						xcdr_drec->sysuse_len, CDFS_BINARY_FIELD);
			break;
		}
		default: {
			(void) fprintf(stderr, "%s (Unrecognized format)\n", CDFS_ID);
			break;
		}
	}

	return (0);
}





/*
 * cd_FillPTRec - Fill an XCDR PTREC structure from the media version of the
 * same thing.
 */
static int
cd_FillPTRec (xcdr_ptrec, from_ptrec, type)
struct iso9660_ptrec	*xcdr_ptrec;	/* XCDR PTREC to fill		*/
union media_ptrec	*from_ptrec;	/* Media version of PTREC	*/
enum cdfs_type		type;		/* Type of file system		*/
{
	if ((xcdr_ptrec == NULL) || (from_ptrec == NULL)) {
		errno = EFAULT;
		return (-1);
	}

	switch (type) {
		case CDFS_ISO_9660: {
			xcdr_ptrec->dirid_len = from_ptrec->Iso.pt_DirIDSz;
			xcdr_ptrec->xar_len = from_ptrec->Iso.pt_XarSz;
			xcdr_ptrec->loc_ext = from_ptrec->Iso.pt_ExtendLoc;
			xcdr_ptrec->pdirno = from_ptrec->Iso.pt_Parent;

			cd_FillField (&xcdr_ptrec->dir_id, CD_MAXPTRECL,
						&from_ptrec->Iso.pt_DirID, xcdr_ptrec->dirid_len,
						CDFS_STRING);
			break;
		}
		case CDFS_HIGH_SIERRA: {
			xcdr_ptrec->dirid_len = from_ptrec->Hs.pt_DirIDSz;
			xcdr_ptrec->xar_len = from_ptrec->Hs.pt_XarSz;
			xcdr_ptrec->loc_ext = from_ptrec->Hs.pt_ExtendLoc;
			xcdr_ptrec->pdirno = from_ptrec->Hs.pt_Parent;

			cd_FillField (&xcdr_ptrec->dir_id, CD_MAXPTRECL,
						&from_ptrec->Hs.pt_DirID, xcdr_ptrec->dirid_len,
						CDFS_STRING);
			break;
		}
		default: {
			(void) fprintf(stderr, "%s (Unrecognized format)\n", CDFS_ID);
			break;
		}
	}

	return (0);
}





/*
 * cd_hs_adtToDate - Turn a High Sierra ASCII disc date field into a time_t.
 */
time_t
cd_hs_adtToDate (FromDate)
struct HiSierra_adt	*FromDate;	/* Date structure to convert	*/
{
	ushort_t		year;				/* Year								*/
	ushort_t		month;				/* Month							*/
	ushort_t		day;				/* Day								*/
	ushort_t		hour;				/* Hour								*/
	ushort_t		minute;				/* Minute							*/
	ushort_t		second;				/* Second							*/

	year = cd_ToDigit (FromDate->adt_Year, sizeof (FromDate->adt_Year));
	month = cd_ToDigit (FromDate->adt_Month, sizeof (FromDate->adt_Month));
	day = cd_ToDigit (FromDate->adt_Day, sizeof (FromDate->adt_Day));
	hour = cd_ToDigit (FromDate->adt_Hour, sizeof (FromDate->adt_Hour));
	minute = cd_ToDigit (FromDate->adt_Minute, sizeof (FromDate->adt_Minute));
	second = cd_ToDigit (FromDate->adt_Second, sizeof (FromDate->adt_Second));
	/*
	 * Note that adt_Hundredths is ignored, since it's outside the scope of a
	 * time_t variable, which has a resolution of seconds.
	 *
	 * Also notice that we approximate offset from GMT by using the local
	 * time.  HS does not specify this value.
	 */

	tzset ();
	return (cd_ConvertDate (year, month, day, hour, minute, second, 
				(short) (daylight != 0 ? altzone : timezone) / (-1 * 60 * 15)));
}





/*
 * cd_iso_adtToDate - Turn an ISO9660 ASCII disc date field into a time_t.
 */
time_t
cd_iso_adtToDate (FromDate)
	struct Pure9660_adt	*FromDate;		/* Date structure to convert		*/
{
	ushort_t		year;				/* Year								*/
	ushort_t		month;				/* Month							*/
	ushort_t		day;				/* Day								*/
	ushort_t		hour;				/* Hour								*/
	ushort_t		minute;				/* Minute							*/
	ushort_t		second;				/* Second							*/

	year = cd_ToDigit (FromDate->adt_Year, sizeof (FromDate->adt_Year));
	month = cd_ToDigit (FromDate->adt_Month, sizeof (FromDate->adt_Month));
	day = cd_ToDigit (FromDate->adt_Day, sizeof (FromDate->adt_Day));
	hour = cd_ToDigit (FromDate->adt_Hour, sizeof (FromDate->adt_Hour));
	minute = cd_ToDigit (FromDate->adt_Minute, sizeof (FromDate->adt_Minute));
	second = cd_ToDigit (FromDate->adt_Second, sizeof (FromDate->adt_Second));
	/*
	 * Note that adt_Hundredths is ignored, since it's outside the scope of a
	 * time_t variable, which has a resolution of seconds.
	 */

	return (cd_ConvertDate (year, month, day, hour, minute, second,
			(short) FromDate->adt_GmtOffset));
}





/*
 * cd_ToDigit - Convert ASCII strings (that aren't null-terminated) to
 * digits.
 */
static ushort_t
cd_ToDigit (string, len)
uchar_t		*string;		/* String to convert		*/
uint_t		len;			/* Number of characters to cvt.	*/
{
	uchar_t		tmp[10];	/* Temporary string		*/

	/*
	 * Null-terminate string, so that library function will behave well.
	 */
	(void) strncpy ((char *) tmp, (char *) string, len);
	tmp[len] = '\0';

	return (atoi((char *) tmp));
}





/*
 * cd_hs_hdtToDate - Turn a High Sierra hex disc date field into a time_t.
 *
 * Also notice that we approximate offset from GMT by using the local
 * time.  HS does not specify this value.
 */
time_t
cd_hs_hdtToDate (FromDate)
	struct HiSierra_hdt	*FromDate;			/* Date structure to convert	*/
{
	tzset ();
	return (cd_ConvertDate ((ushort_t) FromDate->hdt_Year + 1900,
							(ushort_t) FromDate->hdt_Month,
							(ushort_t) FromDate->hdt_Day,
							(ushort_t) FromDate->hdt_Hour,
							(ushort_t) FromDate->hdt_Minute,
							(ushort_t) FromDate->hdt_Second,
							(short) (daylight != 0 ? altzone :
							timezone) / (-1 * 60 * 15)));
}





/*
 * cd_iso_hdtToDate - Turn a hex an ISO disc date field into a time_t.
 */
time_t
cd_iso_hdtToDate (FromDate)
	struct Pure9660_hdt	*FromDate;			/* Date structure to convert	*/
{
	return (cd_ConvertDate ((ushort_t) FromDate->hdt_Year + 1900,
							(ushort_t) FromDate->hdt_Month,
							(ushort_t) FromDate->hdt_Day,
							(ushort_t) FromDate->hdt_Hour,
							(ushort_t) FromDate->hdt_Minute,
							(ushort_t) FromDate->hdt_Second,
							(short) FromDate->hdt_GmtOffset));
}




/*
 * cd_ConvertDate - Generic date conversion routine.
 */
time_t
#ifdef __STDC__
cd_ConvertDate (ushort_t year, ushort_t month, ushort_t day, ushort_t hour,
							ushort_t min, ushort_t sec, short gmt_off)
#else
cd_ConvertDate (year, month, day, hour, min, sec, gmt_off)
ushort_t		year;		/* Year				*/
ushort_t		month;		/* Month			*/
ushort_t		day;		/* Day				*/
ushort_t		hour;		/* Hour				*/
ushort_t		min;		/* Minute			*/
ushort_t		sec;		/* Second			*/
short			gmt_off;	/* +/- # 15 min intervals from GMT/UTC*/
#endif
{
	time_t			date;				/* Temporary holding variable		*/
	uint_t			count;				/* Loop counter						*/

	if ((year < (ushort_t) (1970 - CDFS_MAX_YEARS)) ||
				(year > (ushort_t) (1970 + CDFS_MAX_YEARS)) ||
				(month == 0) || (month > 12)) {
		return ((time_t) 0);
	}
	
	date = ((((time_t) year) - 1970) * 365);
	date += (time_t) (cd_NumLeapYears(year - 1) - cd_NumLeapYears(1970));
	if ((cd_NumLeapYears (year - 1) != cd_NumLeapYears (year)) &&
				((time_t) month >= 3)) {
		date++;
	}
	for (count = 1; count < month; count++) {
		date += cdfs_DaysOfMonth[count - 1];
	}
	date += (time_t) day - 1;
	date = date * 24 + (time_t) hour;
	date = date * 60 + (time_t) min;
	date = date * 60 + (time_t) sec;
	date -= (time_t) gmt_off * 15 * 60;

	return (date);
}





/*
 * cd_NumLeapYears - Calculate how many leap years have happened up to
 *						a given year.
 *
 * Every fourth year is a leap year, with the exception of century years,
 * which are only leap years if they are evenly divisible by 400.
 */
uint_t
cd_NumLeapYears (year)
	uint_t		year;					/* Year number to check				*/
{
	return (year/4 - year/100 + year/400);
}





/*
 * cd_GetPerms - Get a permissions field, given the media version of one.
 *
 * This assumes the arguments given to it are the two bytes of the media
 * XAR permissions field.
 */
static ushort_t
#ifdef __STDC__
cd_GetPerms (uchar_t perm1, uchar_t perm2)
#else
cd_GetPerms (perm1, perm2)
uchar_t		perm1;			/* First chunk of ISO perms field*/
uchar_t		perm2;			/* Second chunk of ISO perms field*/
#endif
{
	ushort_t	result;				/* Result accumulator					*/

	/*
	 * The definitions of the bitfield values are for the whole field as it
	 * exists on the media.  We really have that field broken down into two
	 * subfields, so we have to shift the constants to do valid comparisons
	 * with them.
	 */
	result = 0;
	result |= (((ushort_t) perm1 << 8) & (CD_XOTH | CD_ROTH | CD_XGRP |
				CD_RGRP));
	result |= (((ushort_t) perm2) & (CD_XUSR | CD_RUSR | CD_XSYS | CD_RSYS));

	return (result);
}













static int
cd_PrintDate (string, date, date_type, fs_type)
	char				*string;			/* Label string to print		*/
	caddr_t				date;				/* Date structure pointer		*/
	uint_t				date_type;			/* HDT or ADT?					*/
	enum cdfs_type		fs_type;			/* CDFS file system type (9660/HS)*/
{
	int					RetVal;				/* Return value of called procs */
	time_t				TmpTime;			/* Temporary time value			*/
	struct tm			*TimeStruct;		/* Working time structure		*/
	uchar_t				DateString[80];		/* String to hold date info		*/

	switch (fs_type) {
		case CDFS_ISO_9660: {
			switch (date_type) {
				case CDFS_HDT_TYPE: {
					TmpTime = cd_iso_hdtToDate ((struct Pure9660_hdt *) date);
					break;
				}
				case CDFS_ADT_TYPE: {
					TmpTime = cd_iso_adtToDate ((struct Pure9660_adt *) date);
					break;
				}
				default: {
					(void) fprintf (stderr, "PrintDate: bad date type\n");
					return (-1);
					/* NOTREACHED */
					break;
				}
			}
			break;
		}
		case CDFS_HIGH_SIERRA: {
			switch (date_type) {
				case CDFS_HDT_TYPE: {
					TmpTime = cd_hs_hdtToDate ((struct HiSierra_hdt *) date);
					break;
				}
				case CDFS_ADT_TYPE: {
					TmpTime = cd_hs_adtToDate ((struct HiSierra_adt *) date);
					break;
				}
				default: {
					(void) fprintf (stderr, "PrintDate: bad date type\n");
					return (-1);
					/* NOTREACHED */
					break;
				}
			}
			break;
		}
		default: {
			(void) fprintf(stderr, "%s (Unrecognized format)\n", CDFS_ID);
			return (-1);
			/* NOTREACHED */
			break;
		}
	}

	if (TmpTime != (time_t) 0) {
		TimeStruct = localtime (&TmpTime);
		RetVal = strftime ((char *) DateString, sizeof (DateString), "%N",
					TimeStruct);
		if (RetVal == 0) {
			return (-1);
		}

		(void) printf("%s: %s\n", string, DateString);
	} else {
		(void) printf("%s: Not defined\n", string);
	}

	return (0);
}





static void
cd_FillField (dest, destsz, source, sourcesz, binary)
void		*dest;		/* Pointer to copy to			*/
uint_t		destsz;		/* Size of destination			*/
void		*source;	/* Pointer to copy to			*/
uint_t		sourcesz;	/* Size of source			*/
uint_t		binary;		/* Treat as bits or as a string	*/
{
	uint_t				Size;				/* Working size					*/

	/*
	 * Figure out how many bytes to copy.
	 *
	 * If the source is smaller, first zero out the destination.
	 */
	if (destsz < sourcesz) {
		Size = destsz;
	} else {
		Size = sourcesz;
		if (binary == CDFS_BINARY_FIELD) {
			(void) memset ((char *) dest, '\0', destsz);
		}
	}

	(void) memmove ((char *) dest, (char *) source, Size);

	if (binary == CDFS_STRING) {
		Size = strlen ((char *) dest);
		if (Size < destsz) {
			((char *) dest)[Size] = '\0';
		}
	}

	return;
}
