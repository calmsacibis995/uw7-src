/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ifndef _FS_CDFS_ISO9660_H	/* wrapper symbol for kernel use */
#define _FS_CDFS_ISO9660_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/cdfs/iso9660.h	1.6"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * The following declarations define the various ISO-9660 data
 * structures as they would appear on the recorded media.  A
 * set of ISO-9660 data structures are also declared in 'sys/cdrom.h'
 * according to the XCDR specification.  The difference between the
 * two sets of declarations is that the ones in this file describe
 * the data structures as stored on the media, whereas, the XCDR
 * declarations contain only the "interesting" relavent data.  The
 * XCDR data structures cannot be used as a template for information
 * retrieved directly from the media.
 *
 * The XCDR declarations are named "iso9660_xxx" as defined by
 * the XCDR specification.   The declaration names chosen for
 * this file has the form "Pure9660_xxx" in order to convey
 * their congruence with the official ISO-9660 specification.
 *
 * A single set of data structures was initially contemplated,
 * however, it was determined that the added complexity and
 * the required "tricks" would only cause confusion.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#else

#include <sys/types.h> /* SVR4.2COMPAT */

#endif /* _KERNEL_HEADERS */




/**************************************************************************
 **************************************************************************
 ***																	***
 ***																	***
 ***				ISO-9660 Data Structure Definitions					***
 ***																	***
 ***																	***
 **************************************************************************
 **************************************************************************/

/*
 * Miscellaneous ISO-9660 definitions
 */
#define ISO_MIN_LSEC_SZ	2048			/* ISO-9660 Min Logical Sec Size*/



/*
 * ISO-9660 ASCII-style Date and Time (ADT)
 */
#pragma pack(1)
struct Pure9660_adt {
	uchar_t		adt_Year[4];		/* Year (0001-9999)	*/
	uchar_t		adt_Month[2];		/* Month of the year (01-12) */
	uchar_t		adt_Day[2];		/* Day of the month (01-31) */
	uchar_t		adt_Hour[2];		/* Hour of the day (00-23) */
	uchar_t		adt_Minute[2];		/* Minute of the hour (00-59) */
	uchar_t		adt_Second[2];		/* Second of the minute (00-59)	*/
	uchar_t		adt_Hundredths[2];	/* Hundredths of a second (00-99)*/
	char		adt_GmtOffset;		/* Offset from Greenwich Mean Time*/
};
#pragma pack()




/*
 * ISO-9660 Hex-based Date and Time (HDT)
 */
#pragma pack(1)
struct Pure9660_hdt {
	uchar_t		hdt_Year;		/* # of Years since 1900 (1-255)*/
	uchar_t		hdt_Month;		/* Month of year (1 to 12) */
	uchar_t		hdt_Day;		/* Day of month (1 to 31) */
	uchar_t		hdt_Hour;		/* Hour of day (0 to 23) */
	uchar_t		hdt_Minute;		/* Minute of hour (0 to 59) */
	uchar_t		hdt_Second;		/* Second of minute (0 to 59) */
	char		hdt_GmtOffset;		/* Offset from GMT (-48 to +52)	*/
};
#pragma pack()
		



/*
 * ISO_9660 Volume Descriptor (VD)
 */
#pragma pack(1)
struct Pure9660_vd {
	uchar_t		vd_Type;		/* Volume Descriptor Type */
	uchar_t		vd_StdID[5];		/* Standard ISO-9660 ID String*/
	uchar_t		vd_Ver;			/* Volume Descriptor Version #*/
	uchar_t		vd_data[2040];		/* Dependent upon Vol Descr Type*/ 
};
#pragma pack()

#define ISO_VD_LOC	16		/* Loc of ISO PVD (Log Sect #)	*/
#define ISO_VD_LEN	2048		/* Size of all Vol. Descr. Types*/
#define ISO_STD_ID	"CD001"		/* ISO-9660 Standared ID String */

/*
 * Define the various types of Volume Descriptors.
 */
#define	ISO_BOOT_TYPE		0x00		/* Boot-Record Volume Descriptor*/
#define	ISO_PVD_TYPE		0x01		/* Primary Volume Descriptor */
#define	ISO_SVD_TYPE		0x02		/* Supplementary Volume Descriptor*/
#define	ISO_PART_TYPE		0x03		/* Volume Partition Descriptor*/
#define	ISO_TERM_TYPE		0xFF		/* Volume Descriptor Terminiator*/




/*
 * ISO-9660 Boot-Record Volume Descriptor (BRVD)
 */
#pragma pack(1)
struct Pure9660_boot {
	uchar_t		boot_Type;		/* Volume Descriptor Type */
	uchar_t		boot_StdID[5];		/* Standard ISO-9660 ID String*/
	uchar_t		boot_Ver;		/* Volume Descriptor Version #*/
	uchar_t		boot_BootSysID[2];	/* System ID String	*/
	uchar_t		boot_BootID[32];	/* Boot Record ID String */
	uchar_t		boot_BootSysUse[1977];	/* System Use Area	*/
};
#pragma pack()




/*
 * ISO-9660 Volume Descriptor Set Terminator (VDST)
 */
#pragma pack(1)
struct Pure9660_term {
	uchar_t		term_Type;		/* Volume Descriptor Type */
	uchar_t		term_StdID[5];		/* Standard ISO-9660 ID String*/
	uchar_t		term_Ver;		/* Volume Descriptor Version #*/
	uchar_t		term_Rsvd[2040];	/* Reserved: Bytes 8-2048 */
};
#pragma pack()




/*
 * Volume Partition Descriptor (VPD)
 */
#pragma pack(1)
struct Pure9660_part {
	uchar_t		part_Type;		/* Volume Descriptor Type */
	uchar_t		part_StdID[5];		/* Standard ISO-9660 ID String*/
	uchar_t		part_Ver;		/* Volume Descriptor Version #*/
	uchar_t		part_Rsvd1;		/* Reserved: Byte 8	*/
	uchar_t		part_SysID[32];		/* System ID String	*/
	uchar_t		part_ID[32];		/* Partition ID String	*/
	ulong_t		part_Loc;		/* Loc of Partition (Log Blk #)	*/
	ulong_t		part_Loc_rev;		/* Loc of Partition (Log Blk #)	*/
	ulong_t		part_Size;		/* Size of Partition (Log Blks)	*/
	ulong_t		part_Size_rev;		/* Size of Partition (Log Blks)	*/
	uchar_t		part_SysUse[1960];	/* System Use Area: Bytes 89-2048*/
};
#pragma pack()




/*
 * ISO-9660 Primary Volume Descriptor (PVD)
 */
#pragma pack(1)
struct Pure9660_pvd {
	uchar_t		pvd_Type;		/* Volume Descriptor Type */
	uchar_t		pvd_StdID[5];		/* Standard ISO-9660 ID String*/
	uchar_t		pvd_Ver;		/* Volume Descriptor Version #*/
	uchar_t		pvd_Rsvd1;		/* Reserved: Byte 8	*/
	uchar_t		pvd_SysID[32];		/* System ID String	*/
	uchar_t		pvd_VolID[32];		/* Volume ID String	*/
	uchar_t		pvd_Rsvd2[8];		/* Reserved: Bytes 73-80 */	
	ulong_t		pvd_VolSpcSz;		/* Volume Space Size (Blks) */
	ulong_t		pvd_VolSpcSz_rev;	/* Volume Space Size (Blks) */
	uchar_t		pvd_Rsvd3[32];		/* Reserved: Bytes 89-120 */
	ushort_t	pvd_VolSetSz;		/* Volume Set Size (Disc Count)	*/
	ushort_t	pvd_VolSetSz_rev;	/* Volume Set Size (Disc Count)	*/
	ushort_t	pvd_VolSeqNum;		/* Volume Sequence Number */
	ushort_t	pvd_VolSeqNum_rev;	/* Volume Sequence Number */
	ushort_t	pvd_LogBlkSz;		/* Logical Block Size (Bytes) */
	ushort_t	pvd_LogBlkSz_rev;	/* Logical Block Size (Bytes) */
	ulong_t		pvd_PathTabSz;		/* Path Table Size (Bytes) */
	ulong_t		pvd_PathTabSz_rev;	/* Path Table Size (Bytes) */
	ulong_t		pvd_PathTabLoc;		/* Type-L Path Table Loc (L-Blk #)*/
	ulong_t		pvd_OptPathTabLoc;	/* optional Type-L Path Table */
	ulong_t		pvd_PathTabLoc_rev;	/* Type-M Path Table Loc (L-Blk #)*/
	ulong_t		pvd_OptPathTabLoc_rev;	/* Optional Type-M Path Table */
	uchar_t		pvd_RootDir[34];	/* Directory Record for Root (/)*/
	uchar_t		pvd_VolSetID[128];	/* Volume Set ID String	*/
	uchar_t		pvd_PublsherID[128];	/* Publisher ID String	*/
	uchar_t		pvd_PreparerID[128];	/* Data Preparer ID String */
	uchar_t		pvd_ApplID[128];	/* Application ID String */
	uchar_t		pvd_CopyrightFile[37];	/* Copyright File Name	*/
	uchar_t		pvd_AbstractFile[37];	/* Abstract File Name	*/
	uchar_t		pvd_BiblioFile[37];	/* Bibliographic File Name */
	struct Pure9660_adt pvd_CreateDate;	/* Volume Creation Date/Time */
	struct Pure9660_adt pvd_ModDate;	/* Volume Modification Date/Time*/
	struct Pure9660_adt pvd_ExpireDate;	/* Volume Expiration Date/Time*/
	struct Pure9660_adt pvd_EffectDate;	/* Volume Effective Date/Time */
	uchar_t		pvd_FileVer;		/* File Structure Version # */
	uchar_t		pvd_Rsvd4;		/* Reserved: Byte 883 	*/
	uchar_t		pvd_ApplUse[512];	/* Application Use Area	*/
	uchar_t		pvd_Rsvd5[653];		/* Reserved: Bytes 1396-2048 */
};
#pragma pack()




/*
 * ISO-9660 Supplementary Volume Descriptor (SVD)
 */
#pragma pack(1)
struct Pure9660_svd {
	uchar_t		svd_Type;			/* Volume Descriptor Type		*/
	uchar_t		svd_StdID[5];			/* Standard ISO-9660 ID String	*/
	uchar_t		svd_Ver;			/* Volume Descriptor Version #	*/
	uchar_t		svd_Flags;			/* Volume Flags - See below		*/
	uchar_t		svd_SysID[32];			/* System ID String */
	uchar_t		svd_VolID[32];			/* Volume ID String */
	uchar_t		svd_Rsvd2[8];			/* Reserved: Bytes 73-80	 	*/	
	ulong_t		svd_VolSpcSz;			/* Volume Space Size (Blks)		*/
	ulong_t		svd_VolSpcSz_rev;		/* Volume Space Size (Blks)		*/
	uchar_t		svd_EscSeq[32];			/* Escape Sequence Definitions	*/
	ushort_t	svd_VolSetSz;			/* Volume Set Size (Disc Count)	*/
	ushort_t	svd_VolSetSz_rev;		/* Volume Set Size (Disc Count)	*/
	ushort_t	svd_VolSeqNum;			/* Volume Sequence Number		*/
	ushort_t	svd_VolSeqNum_rev;		/* Volume Sequence Number		*/
	ushort_t	svd_LogBlkSz;			/* Logical Block Size (Bytes)	*/
	ushort_t	svd_LogBlkSz_rev;		/* Logical Block Size (Bytes)	*/
	ulong_t		svd_PathTabSz;			/* Path Table Size (Bytes)		*/
	ulong_t		svd_PathTabSz_rev;		/* Path Table Size (Bytes)		*/
	ulong_t		svd_PathTabLoc;			/* Type-L Path Table Loc (L-Blk #)*/
	ulong_t		svd_OptPathTabLoc;		/* optional Type-L Path Table	*/
	ulong_t		svd_PathTabLoc_rev;		/* Type-M Path Table Loc (L-Blk #)*/
	ulong_t		svd_OptPathTabLoc_rev;		/* Optional Type-M Path Table	*/
	uchar_t		svd_RootDir[34];		/* Directory Record for Root (/)*/
	uchar_t		svd_VolSetID[128];		/* Volume Set ID String			*/
	uchar_t		svd_PublisherID[128];		/* Publisher ID String*/
	uchar_t		svd_PreparerID[128];		/* Data Preparer ID String		*/
	uchar_t		svd_ApplID[128];		/* Application ID String		*/
	uchar_t		svd_CopyrightFile[37];		/* Copyright File Name*/
	uchar_t		svd_AbstractFile[37];		/* Abstract File Name */
	uchar_t		svd_BiblioFile[37];		/* Bibliographic File Name		*/
	struct Pure9660_adt svd_CreateDate;		/* Volume Creation Date/Time	*/
	struct Pure9660_adt svd_ModDate;		/* Volume Modification Date/Time*/
	struct Pure9660_adt svd_ExpireDate;		/* Volume Expiration Date/Time	*/
	struct Pure9660_adt svd_EffectDate;		/* Volume Effective Date/Time	*/
	uchar_t		svd_FileVer;			/* File Structure Version #		*/
	uchar_t		svd_Rsvd4;			/* Reserved: Byte 883 			*/
	uchar_t		svd_ApplUse[512];		/* Application Use Area			*/
	uchar_t		svd_Rsvd5[653];			/* Reserved: Bytes 1396-2048	*/
};
#pragma pack()

/*
 * Bit definitions for the flag field of
 * the Supplementary Volume Descripter.
 */
#define	ISO_SVDFLG_2735		0x01		/* Esc Seqs conform to ISO-2735	*/
#define	ISO_SVDFLG_RSVD		0xFE		/* Reserved: Bits 1-7	*/




/*
 * ISO-9660 Directory Record.
 */
#pragma pack(1)
struct Pure9660_drec{
	uchar_t		drec_Size;		/* Size of this Dir Rec (Bytes)	*/
	uchar_t		drec_XarSize;		/* Size of XAR (Blocks)	*/
	ulong_t		drec_ExtentLoc;		/* Location of Extent (Block #)	*/
	ulong_t		drec_ExtentLoc_rev;	/* Location of Extent (Block #)	*/
	ulong_t		drec_DataSz;		/* Size of File Section data:Bytes*/
	ulong_t		drec_DataSz_rev;	/* Size of File Section data:Bytes*/
	struct Pure9660_hdt drec_RecordDate;	/* Recording Date/Time	*/
	uchar_t		drec_Flags;		/* File Flags		*/
	uchar_t		drec_UnitSz;		/* Size of File Unit (Blocks) */
	uchar_t		drec_Interleave;	/* Size of Interleave Gap (Blks)*/
	ushort_t	drec_VolSeqNum;		/* Volume Sequence Num (Disc #)	*/
	ushort_t	drec_VolSeqNum_rev;	/* Volume Sequence Num (Disc #)	*/
	uchar_t		drec_FileIDSz;		/* Size of File Name (ID String)*/
	uchar_t		drec_VarData;		/* Variable Len data: See Below */
	/*
	 * 'drec_VarData is a place holder and is used to locate the
	 * beginning of the three variable length fields of the Directory
	 * Record.  The locations are computed by adding the appropriate
	 * length values to the address of 'drec_VarData'.  The three
	 * variable length fields are:
	 * uchar_t	drec_FileID[]	-- File ID string
	 * uchar_t	drec_FileIDPad[] -- Optional pad: Even byte boundry
	 * uchar_t	drec_SysUse[]	-- System Use Area
	 */
};
#pragma pack()

#define ISO_DREC_FIXEDLEN	33		/* Len of Fixed-part of Dir Rec	*/
#define ISO_DREC_MAXLEN		255		/* Max len of a Directory Record*/

/*
 * Bit definitions for the flags field of the
 * Directory Record structure.
 */
#define	ISO_DREC_EXIST		0x01		/* Hide the file's existence	*/
#define ISO_DREC_DIR		0x02		/* Entry is a Directory	*/
#define ISO_DREC_ASSOC		0x04		/* Entry is an Associated File	*/
#define ISO_DREC_REC		0x08		/* File data has "Record" format*/
#define ISO_DREC_PROTECT	0x10		/* File has valid permission data*/
#define ISO_DREC_RSVD1		0x20		/* Reserved 		*/
#define ISO_DREC_RSVD2		0x40		/* Reserved		*/
#define ISO_DREC_MULTI		0x80		/* Additional Dir Recs follow */

/*
 * Define the various Record Formats for the data in a file.
 */
enum iso_recfmt {
	ISO_RECFMT_NONE		= 0,		/* No record format specified */
	ISO_RECFMT_FIXED	= 1,		/* Fixed length records	*/
	ISO_RECFMT_VAR1		= 2,		/* Variable length records: Type-1*/
	ISO_RECFMT_VAR2		= 3		/* Variable length records: Type-2*/
};


/*
 * Define the various Record Attributes for the Records of a file.
 * A record attribute defines the control characters that preceed
 * the actual record data.
 */
enum iso_recattr {
	ISO_RECATTR_CRLF	= 0,		/* Begins with a CR and LF chars*/
	ISO_RECATTR_1539	= 1,		/* 1st char conforms to ISO-1539*/
	ISO_RECATTR_NONE	= 2		/* No leading control characters*/
};

/*
 * Special File ID components: drec_FileID[]
 */
#define ISO_FILEID_SEPARATOR1	'.'		/* ISO-9660 File ID separator 1	*/
#define ISO_FILEID_SEPARATOR2	';'		/* ISO-9660 File ID separator 2	*/

#define ISO_DOT			0x00 		/* ISO-9660 Current Dir: 0x00 */
#define ISO_DOTDOT		0x01		/* ISO-9660 Parent Dir: 0x01,0x00*/

#define ISO_DOT_OFFSET		0 		/* Dir offset of Current Dir rec*/




/*
 * ISO-9660 Extended Attribute Record (XAR)
 */ 
#pragma pack(1)
struct Pure9660_xar {
	ushort_t	xar_User;		/* User (Owner) ID	*/
	ushort_t	xar_User_rev;		/* User (Owner) ID	*/
	ushort_t	xar_Group;		/* Group ID		*/
	ushort_t	xar_Group_rev;		/* Group ID		*/
	uchar_t		xar_Perms1;		/* Perm flags 8-15: See below */
	uchar_t		xar_Perms2;		/* Perm flags 0-7: See below */
	struct Pure9660_adt xar_CreateDate;	/* File Creation Date/Time */
	struct Pure9660_adt xar_ModDate;	/* File Modification Date/Time*/
	struct Pure9660_adt xar_ExpireDate;	/* File Expiration Date/Time */
	struct Pure9660_adt xar_EffectDate;	/* File Effective Date/Time  */
	uchar_t		xar_RecFmt;		/* Record Format	*/
	uchar_t		xar_RecAttr;		/* Record Attributes	*/
	ushort_t	xar_RecLen;		/* Record Length (Bytes) */
	ushort_t	xar_RecLen_rev;		/* Record Length (Bytes) */
	uchar_t		xar_SysID[32];		/* System Use Area ID String */
	uchar_t		xar_SysUse[64];		/* System Use Area	*/
	uchar_t		xar_Ver;		/* XAR Version		*/
	uchar_t		xar_EscSeqLen;		/* Len of Escape Sequences */
	uchar_t		xar_Rsvd[64];		/* Reserved: Bytes 183-246		*/
	ushort_t	xar_ApplUseLen;		/* Len of Application Use Area*/
	ushort_t	xar_ApplUseLen_rev;	/* Len of Application Use Area*/
	uchar_t		xar_VarData;		/* Appl Use Area/Escape Sequences*/
	/*
	 * 'xar_VarData is a place holder and is used to locate the
	 * beginning of the two variable length fields of the Directory
	 * Record.  The locations are computed by adding the appropriate
	 * length values to the address of 'xar_VarData'.  The two
	 * variable length fields are:
	 * uchar_t	xar_ApplUse[]	-- Application Use Area
	 * uchar_t	xar_EscSeq[]	-- Escape Sequences
	 */
};
#pragma pack()

#define	ISO_XAR_FIXEDLEN	250		/* Size of XAR fixed-len (Bytes)*/

/*
 * Bit definitions of the XAR Permission field.
 * Note: Unused bits of the bitfield are set to 'ONE'.
 */
#define ISO_XAR_RSYS		0x0001		/* "System Class" Read Perm Bit	*/
#define	ISO_XAR_XSYS		0x0004		/* "System Class" Execute Perm Bit*/
#define	ISO_XAR_RUSR		0x0010		/* User Read Permission Bit		*/
#define	ISO_XAR_XUSR		0x0040		/* User Execute Permission Bit	*/
#define	ISO_XAR_RGRP		0x0100		/* Group Read Permission Bit	*/
#define	ISO_XAR_XGRP		0x0400		/* Group Execute Permission Bit	*/
#define	ISO_XAR_ROTH		0x1000		/* Other Read Permission Bit	*/
#define	ISO_XAR_XOTH		0x4000		/* Other Execute Permission Bit */
#define ISO_XAR_NONE		0xFFFF		/* Value when no perms exist	*/




/*
 * Path Table Record (ptrec)
 * Note: Multi-byte numeric fields have the correct byte-order
 * provided that the structure was read from the Path Table
 * with the desired byte order.
 */
#pragma pack(1)
struct Pure9660_ptrec {
	uchar_t		pt_DirIDSz;		/* Size of Dir ID string (Bytes)*/
	uchar_t		pt_XarSz;		/* Size of XAR (Blocks)	*/
	ulong_t		pt_ExtendLoc;		/* Location of Extent (Block #)	*/
	ushort_t	pt_Parent;		/* PT Record # of Parent Dir */
	uchar_t		pt_DirID;		/* Directory ID string	*/
	/*
	 * Note: 'pt_DirID' is actually a variable length array characters
	 * followed by an optional NULL byte if 'pt_DirIDSz is an odd value.
	 */
};
#pragma pack()

#define ISO_PT_FIXEDLEN	8			/* Max len of Path Table Record */




/**************************************************************************
 **************************************************************************
 ***																	***
 ***																	***
 ***				High-Sierra Data Structure Definitions				***
 ***																	***
 ***																	***
 **************************************************************************
 **************************************************************************/

/*
 * Miscellaneous High-Sierra definitions
 */
#define HS_MIN_SEC_SZ	2048			/* High-Sierra Min Log Sec Size	*/



/*
 * High-Sierra ASCII-based Date and Time (ADT)
 */
#pragma pack(1)
struct HiSierra_adt {
	uchar_t		adt_Year[4];		/* Year (0001-9999)	*/
	uchar_t		adt_Month[2];		/* Month of the year (01-12)  */
	uchar_t		adt_Day[2];		/* Day of the month (01-31)  */
	uchar_t		adt_Hour[2];		/* Hour of the day (00-23) */
	uchar_t		adt_Minute[2];		/* Minute of the hour (00-59) */
	uchar_t		adt_Second[2];		/* Second of the minute (00-59)	*/
	uchar_t		adt_Hundredths[2];	/* Hundredths of a second (00-99)*/
};
#pragma pack()



/*
 * High-Sierra Hex-based Date and Time (HDT)
 */
#pragma pack(1)
struct HiSierra_hdt {
	uchar_t		hdt_Year;		/* # of Years since 1900 (1-255)*/
	uchar_t		hdt_Month;		/* Month of year (1 to 12) */
	uchar_t		hdt_Day;		/* Day of month (1 to 31) */
	uchar_t		hdt_Hour;		/* Hour of day (0 to 23) */
	uchar_t		hdt_Minute;		/* Minute of hour (0 to 59) */
	uchar_t		hdt_Second;		/* Second of minute (0 to 59) */
};
#pragma pack()
		



/*
 * High Sierra Volume Descriptor (VD)
 */
#pragma pack(1)
struct HiSierra_vd {
	ulong_t		vd_LogBlkNum;		/* Volume Des Logical Block # */
	ulong_t		vd_LogBlkNum_rev;	/* Volume Des Logical Block # */
	uchar_t		vd_Type;		/* Volume Descriptor Type */
	uchar_t		vd_StdID[5];		/* Standard ISO-9660 ID String*/
	uchar_t		vd_Ver;			/* Volume Descriptor Version #*/
	uchar_t		vd_data[2032];		/* Dependent upon Vol Descr Type*/ 
};
#pragma pack()

#define HS_VD_LOC	16			/* Loc of HS PVD (Log Sec #) */
#define HS_VD_LEN	2048			/* Size of all Vol Descrs */
#define HS_STD_ID	"CDROM"			/* High-Sierra Standared ID String*/

/*
 * Define the various types of Volume Descriptors.
 */
#define	HS_BOOT_TYPE	0x00			/* Boot-Record Volume Descriptor*/
#define	HS_PVD_TYPE	0x01			/* Primary Volume Descriptor */
#define	HS_SVD_TYPE	0x02			/* Supplementary Volume Descriptor*/
#define	HS_PART_TYPE	0x03			/* Volume Partition Descriptor*/
#define	HS_TERM_TYPE	0xFF			/* Volume Descriptor Terminiator*/




/*
 * High Sierra Boot-Record Volume Descriptor (BRVD)
 */
#pragma pack(1)
struct HiSierra_boot {
	ulong_t		boot_LogBlkNum;		/* Volume Des Logical Block # */
	ulong_t		boot_LogBlkNum_rev;	/* Volume Des Logical Block # */
	uchar_t		boot_Type;		/* Volume Descriptor Type */
	uchar_t		boot_StdID[5];		/* Standard ISO-9660 ID String*/
	uchar_t		boot_Ver;		/* Volume Descriptor Version #*/
	uchar_t		boot_BootSysID[32];	/* System ID String	*/
	uchar_t		boot_BootID[32];	/* Boot Record ID String */
	uchar_t		boot_BootSysUse[1969];	/* System Use Area	*/
};
#pragma pack()




/*
 * High Sierra Volume Descriptor Sequence Terminator (VDST)
 */
#pragma pack(1)
struct HiSierra_term {
	ulong_t		term_LogBlkNum;		/* Volume Des Logical Block # */
	ulong_t		term_LogBlkNum_rev;	/* Volume Des Logical Block # */
	uchar_t		term_Type;		/* Volume Descriptor Type */
	uchar_t		term_StdID[5];		/* Standard ISO-9660 ID String*/
	uchar_t		term_Ver;		/* Volume Descriptor Version #*/
	uchar_t		term_Rsvd[2032];	/* Reserved: Bytes 16-2048 */
};
#pragma pack()





/*
 * High Sierra Volume Partition Descriptor (VPD)
 */
#pragma pack(1)
struct HiSierra_part {
	ulong_t		part_LogBlkNum;		/* Vol Desc Logical Block # */
	ulong_t		part_LogBlkNum_rev;	/* Vol Desc Logical Block # */
	uchar_t		part_Type;		/* Volume Descriptor Type */
	uchar_t		part_StdID[5];		/* Standard ISO-9660 ID String*/
	uchar_t		part_Ver;		/* Volume Descriptor Version #*/
	uchar_t		part_Rsvd1;		/* Reserved: Byte 16	*/
	uchar_t		part_SysID[32];		/* System ID String	*/
	uchar_t		part_ID[32];		/* Partition ID String	*/
	ulong_t		part_Loc;		/* Loc of Partition (Log Blk #)	*/
	ulong_t		part_Loc_rev;		/* Loc of Partition (Log Blk #)	*/
	ulong_t		part_Size;		/* Size of Partition (Log Blks)	*/
	ulong_t		part_Size_rev;		/* Size of Partition (Log Blks)	*/
	uchar_t		part_SysUse[1952];	/* System Use Area	*/
};
#pragma pack()





/*
 *  High Sierra Primary Volume Descriptor (PVD)
 */
#pragma pack(1)
struct HiSierra_pvd {
	ulong_t		pvd_LogBlkNum;		/* Volume Des Logical Block # */
	ulong_t		pvd_LogBlkNum_rev;	/* Volume Des Logical Block # */
	uchar_t		pvd_Type;		/* Volume Descriptor Type */
	uchar_t		pvd_StdID[5];		/* Standard ISO-9660 ID String*/
	uchar_t		pvd_Ver;		/* Volume Descriptor Version #*/
	uchar_t		pvd_Rsvd1;		/* Reserved: Byte 16	*/
	uchar_t		pvd_SysID[32];		/* System ID String	*/
	uchar_t		pvd_VolID[32];		/* Volume ID String	*/
	uchar_t		pvd_Rsvd2[8];		/* Reserved: Bytes 81-88 */	
	ulong_t		pvd_VolSpcSz;		/* Volume Space Size (Blks) */
	ulong_t		pvd_VolSpcSz_rev;	/* Volume Space Size (Blks) */
	uchar_t		pvd_Rsvd3[32];		/* Reserved: Bytes 97-128 */
	ushort_t	pvd_VolSetSz;		/* Volume Set Size (Disc Count)	*/
	ushort_t	pvd_VolSetSz_rev;	/* Volume Set Size (Disc Count)	*/
	ushort_t	pvd_VolSeqNum;		/* Volume Sequence Number */
	ushort_t	pvd_VolSeqNum_rev;	/* Volume Sequence Number */
	ushort_t	pvd_LogBlkSz;		/* Logical Block Size (Bytes) */
	ushort_t	pvd_LogBlkSz_rev;	/* Logical Block Size (Bytes) */
	ulong_t		pvd_PathTabSz;		/* Path Table Size (Bytes) */
	ulong_t		pvd_PathTabSz_rev;	/* Path Table Size (Bytes) */
	ulong_t		pvd_PathTabLoc;		/* Type-L Path Table Loc (L-Blk #)*/
	ulong_t		pvd_Opt1PathTabLoc;	/* Optional Type-L Path Table */
	ulong_t		pvd_Opt2PathTabLoc;	/* Optional Type-L Path Table */
	ulong_t		pvd_Opt3PathTabLoc;	/* Optional Type-L Path Table */
	ulong_t		pvd_PathTabLoc_rev;	/* Type-M Path Table Loc (L-Blk #)*/
	ulong_t		pvd_Opt1PathTabLoc_rev;		/* Optional Type-M Path Table	*/
	ulong_t		pvd_Opt2PathTabLoc_rev;		/* Optional Type-M Path Table	*/
	ulong_t		pvd_Opt3PathTabLoc_rev;		/* Optional Type-M Path Table	*/
	uchar_t		pvd_RootDir[34];	/* Directory Record for Root (/)*/
	uchar_t		pvd_VolSetID[128];	/* Volume Set ID String	*/
	uchar_t		pvd_PublsherID[128];	/* Publisher ID String	*/
	uchar_t		pvd_PreparerID[128];	/* Data Preparer ID String */
	uchar_t		pvd_ApplID[128];	/* Application ID String */
	uchar_t		pvd_CopyrightFile[32];	/* Copyright File Name	*/
	uchar_t		pvd_AbstractFile[32];	/* Abstract File Name	*/
	struct HiSierra_adt pvd_CreateDate;	/* Volume Creation Date/Time */
	struct HiSierra_adt pvd_ModDate;	/* Volume Modification Date/Time*/
	struct HiSierra_adt pvd_ExpireDate;	/* Volume Expiration Date/Time*/
	struct HiSierra_adt pvd_EffectDate;	/* Volume Effective Date/Time */
	uchar_t		pvd_FileVer;		/* File Structure Version # */
	uchar_t		pvd_Rsvd4;		/* Reserved: Byte 856 	*/
	uchar_t		pvd_ApplUse[512];	/* Application Use Area	*/
	uchar_t		pvd_Rsvd5[680];		/* Reserved: Bytes 1396-2048 */
};
#pragma pack()





/*
 * High Sierra Supplementary Volume Descriptor (SVD)
 */
#pragma pack(1)
struct HiSierra_svd {
	ulong_t		svd_LogBlkNum;		/* Vol Desc Logical Block # */
	ulong_t		svd_LogBlkNum_rev;	/* Vol Desc Logical Block # */
	uchar_t		svd_Type;		/* Volume Descriptor Type */
	uchar_t		svd_StdID[5];		/* Standard ISO-9660 ID String*/
	uchar_t		svd_Ver;		/* Volume Descriptor Version #*/
	uchar_t		svd_Flags;		/* Volume Flags - See below */
	uchar_t		svd_SysID[32];		/* System ID String	*/
	uchar_t		svd_VolID[32];		/* Volume ID String	*/
	uchar_t		svd_Rsvd2[8];		/* Reserved: Bytes 73-80 */	
	ulong_t		svd_VolSpcSz;		/* Volume Space Size (Blks) */
	ulong_t		svd_VolSpcSz_rev;	/* Volume Space Size (Blks) */
	uchar_t		svd_EscSeq[32];		/* Escape Sequence Definitions	*/
	ushort_t	svd_VolSetSz;		/* Volume Set Size (Disc Count)	*/
	ushort_t	svd_VolSetSz_rev;	/* Volume Set Size (Disc Count)	*/
	ushort_t	svd_VolSeqNum;		/* Volume Sequence Number */
	ushort_t	svd_VolSeqNum_rev;	/* Volume Sequence Number */
	ushort_t	svd_LogBlkSz;		/* Logical Block Size (Bytes) */
	ushort_t	svd_LogBlkSz_rev;	/* Logical Block Size (Bytes) */
	ulong_t		svd_PathTabSz;		/* Path Table Size (Bytes) */
	ulong_t		svd_PathTabSz_rev;	/* Path Table Size (Bytes) */
	ulong_t		svd_PathTabLoc;		/* Type-L Path Table Loc (L-Blk #)*/
	ulong_t		svd_OptPathTabLoc1;	/* optional Type-L Path Table */
	ulong_t		svd_OptPathTabLoc2;	/* optional Type-L Path Table */
	ulong_t		svd_OptPathTabLoc3;	/* optional Type-L Path Table */
	ulong_t		svd_PathTabLoc_rev;	/* Type-M Path Table Loc (L-Blk #)*/
	ulong_t		svd_OptPathTabLoc1_rev;		/* Optional Type-M Path Table	*/
	ulong_t		svd_OptPathTabLoc2_rev;		/* Optional Type-M Path Table	*/
	ulong_t		svd_OptPathTabLoc3_rev;		/* Optional Type-M Path Table	*/
	uchar_t		svd_RootDir[34];	/* Directory Record for Root (/)*/
	uchar_t		svd_VolSetID[128];	/* Volume Set ID String	*/
	uchar_t		svd_PublisherID[128];	/* Publisher ID String	*/
	uchar_t		svd_PreparerID[128];	/* Data Preparer ID String */
	uchar_t		svd_ApplID[128];	/* Application ID String*/
	uchar_t		svd_CopyrightFile[32];	/* Copyright File Name	*/
	uchar_t		svd_AbstractFile[32];	/* Abstract File Name	*/
	struct HiSierra_adt svd_CreateDate;	/* Volume Creation Date/Time */
	struct HiSierra_adt svd_ModDate;	/* Volume Modification Date/Time*/
	struct HiSierra_adt svd_ExpireDate;	/* Volume Expiration Date/Time*/
	struct HiSierra_adt svd_EffectDate;	/* Volume Effective Date/Time */
	uchar_t		svd_FileVer;		/* File Structure Version # */
	uchar_t		svd_Rsvd4;		/* Reserved: Byte 856 	*/
	uchar_t		svd_ApplUse[512];	/* Application Use Area	*/
	uchar_t		svd_Rsvd5[680];		/* Reserved: Bytes 1369-2048 */
};
#pragma pack()

/*
 * Bit definitions for the flag field of
 * the Supplementary Volume Descripter.
 */
#define	HS_SVDFLG_2735		0x01		/* Esc Seqs conform to ISO-2735	*/
#define	HS_SVDFLG_RSVD		0xFE		/* Reserved: Bits 1-7	*/






/*
 * High Sierra Directory Record.
 */
#pragma pack(1)
struct HiSierra_drec	{
	uchar_t		drec_Size;		/* Size of this Dir Rec (Bytes)	*/
	uchar_t		drec_XarSize;		/* Size of XAR (Blocks)	*/
	ulong_t		drec_ExtentLoc;		/* Location of Extent (Block #)	*/
	ulong_t		drec_ExtentLoc_rev;	/* Location of Extent (Block #)	*/
	ulong_t		drec_DataSz;		/* Size of File Section data:Bytes*/
	ulong_t		drec_DataSz_rev;	/* Size of File Section data:Bytes*/
	struct HiSierra_hdt drec_RecordDate;	/* Recording Date/Time	*/
	uchar_t		drec_Flags;		/* File Flags		*/
	uchar_t		drec_unused1;		/* Reserved: Byte 26	*/
	uchar_t		drec_InterleaveSz;	/* Size of Interleave Gap (Blks)*/
	uchar_t		drec_InterleaveSkip;	/* Interleave Skip Factor */
	ushort_t	drec_VolSeqNum;		/* Volume Sequence Num (Disc #)	*/
	ushort_t	drec_VolSeqNum_rev;	/* Volume Sequence Num (Disc #)	*/
	uchar_t		drec_FileIDSz;		/* Size of File Name (ID String)*/
	uchar_t		drec_VarData;		/* Variable-length data */
	/*
	 * 'drec_VarData is a place holder and is used to locate the
	 * beginning of the three variable length fields of the Directory
	 * Record.  The locations are computed by adding the appropriate
	 * length values to the address of 'drec_VarData'.  The three
	 * variable length fields are:
	 * uchar_t	drec_FileID[]	-- File ID string
	 * uchar_t	drec_FileIDPad	-- Optional pad: Even byte boundry
	 * uchar_t	drec_SysUse[]	-- System Use Area				
	 */
};
#pragma pack()





/*
 * High Sierra Extended Attribute Record (XAR)
 */ 
#pragma pack(1)
struct HiSierra_xar {
	ushort_t	xar_User;		/* User (Owner) ID	*/
	ushort_t	xar_User_rev;		/* User (Owner) ID	*/
	ushort_t	xar_Group;		/* Group ID		*/
	ushort_t	xar_Group_rev;		/* Group ID		*/
	uchar_t		xar_Perms1;		/* Perm flags 8-15 (See below)*/
	uchar_t		xar_Perms2;		/* Perm flags 0-7 (See below) */
	struct HiSierra_adt xar_CreateDate;	/* File Creation Date/Time */
	struct HiSierra_adt xar_ModDate;	/* File Modification Date/Time*/
	struct HiSierra_adt xar_ExpireDate;	/* File Expiration Date/Time */
	struct HiSierra_adt xar_EffectDate;	/* File Effective Date/Time */
	uchar_t		xar_RecFmt;		/* Record Format	*/
	uchar_t		xar_RecAttr;		/* Record Attributes	*/
	ushort_t	xar_RecLen;		/* Record Length (Bytes) */
	ushort_t	xar_RecLen_rev;		/* Record Length (Bytes) */
	uchar_t		xar_SysID[32];		/* System Use Area ID String */
	uchar_t		xar_Rsvd1[64];		/* Reserved: Bytes 113-176 */
	uchar_t		xar_Ver;		/* XAR Version		*/
	uchar_t		xar_Rsvd2[65];		/* Reserved: Bytes 178-242 */
	ushort_t	xar_ParDir;		/* Parent dir path table rec #*/
	ushort_t	xar_ParDir_rev;		/* Parent dir path table rec #*/
	ushort_t	xar_ApplUseLen;		/* Len of Application Use Area*/
	ushort_t	xar_ApplUseLen_rev;	/* Len of Application Use Area*/
	uchar_t		xar_RecDir;		/* Directory Record	*/
	uchar_t		xar_ApplRes;		/* Reserved for Application Use	*/
	/*
	 * Note: 'xar_ApplRes' is actually a variable length array characters.
	 */
};
#pragma pack()





/*
 * Path Table Record (ptrec)
 * Note: Multi-byte numeric fields have the correct byte-order
 * provided that the structure was read from the Path Table
 * with the desired byte order.
 */
#pragma pack(1)
struct HiSierra_ptrec {
	ulong_t		pt_ExtendLoc;		/* Location of Extent (Block #)	*/
	uchar_t		pt_XarSz;		/* Size of XAR (Blocks)	*/
	uchar_t		pt_DirIDSz;		/* Size of Dir ID string (Bytes)*/
	ushort_t	pt_Parent;		/* PT Record # of Parent Dir */
	uchar_t		pt_DirID;		/* Directory ID string	 */
	/*
	 * Note: 'pt_DirID' is actually a variable length array characters
	 * followed by an optional NULL byte if 'pt_DirIDSz is an odd value.
	 */
};
#pragma pack()





/**************************************************************************
 **************************************************************************
 ***																	***
 ***																	***
 ***				SUSP (System Use Sharing Protocal) 					***
 ***					Data Structure Definitions						***
 ***																	***
 ***																	***
 **************************************************************************
 **************************************************************************/

/*
 * SUSP Generic System Use Field (SUF).
 */
#pragma pack(1)
struct susp_suf {
	uchar_t		suf_Sig1;		/* SUF Signature char #1 */
	uchar_t		suf_Sig2;		/* SUF Signature char #2 */
	uchar_t		suf_Len;		/* TOTAL length of SUF (Bytes)*/
	uchar_t		suf_Ver;		/* SUF version #	*/
};
#pragma pack()

	

/*
 * SUSP Identification ('SP') SUF.
 */
#pragma pack(1)
struct susp_sp {
	uchar_t		sp_Sig1;		/* SUF Signature char #1: 'S' */
	uchar_t		sp_Sig2;		/* SUF Signature char #2: 'P' */
	uchar_t		sp_Len;			/* TOTAL len of SUF (Bytes)   */
	uchar_t		sp_Ver;			/* SP SUF version #	*/
	uchar_t		sp_ID1;			/* SP ID char #1	*/
	uchar_t		sp_ID2;			/* SP ID char #2	*/
	uchar_t		sp_Offset;		/* Offset of SUSP within SUA  */
};
#pragma pack()

#define	SUSP_SP_SIG1		'S'		/* Value of 'SP' Signature char #1*/
#define	SUSP_SP_SIG2		'P'		/* Value of 'SP' Signature char #2*/
#define	SUSP_SP_VER		1		/* Value of 'SP' Version field	*/
#define	SUSP_SP_ID1		0xBE		/* Value of 'SP' ID char #1 */
#define	SUSP_SP_ID2		0xEF		/* Value of 'SP' ID char #2 */



/*
 * SUSP Continuation Area ('CE') SUF.
 */
#pragma pack(1)
struct susp_ce {
	uchar_t		ce_Sig1;		/* SUF Signature char #1: 'C' */
	uchar_t		ce_Sig2;		/* SUF Signature char #2: 'E' */
	uchar_t		ce_Len;			/* TOTAL len of SUF (Bytes)   */
	uchar_t		ce_Ver;			/* CE SUF version #	*/
	ulong_t		ce_Loc;			/* Location of CE Area (L-Blk #)*/
	ulong_t		ce_Loc_rev;		/* Location of CE Area (L-Blk #)*/
	ulong_t		ce_Offset;		/* Offset of CE Area within block*/
	ulong_t		ce_Offset_rev;		/* Offset of CE Area within block*/
	ulong_t		ce_AreaSz;		/* Size of CE Area (Bytes) */
	ulong_t		ce_AreaSz_rev;		/* Size of CE Area (Bytes) */
};
#pragma pack()

#define	SUSP_CE_SIG1		'C'		/* Value of 'CE' Signature char #1*/
#define	SUSP_CE_SIG2		'E'		/* Value of 'CE' Signature char #2*/
#define	SUSP_CE_VER		1		/* Value of 'CE' Version field	*/

	

/*
 * SUSP Pad Area ('PD') SUF.
 */
#pragma pack(1)
struct susp_pd {
	uchar_t		pd_Sig1;		/* SUF Signature char #1: 'P' */
	uchar_t		pd_Sig2;		/* SUF Signature char #2: 'D' */
	uchar_t		pd_Len;			/* TOTAL len of SUF (Bytes)  */
	uchar_t		pd_Ver;			/* PD SUF version #	*/
	uchar_t		pd_Pad;			/* Pad area: Unused	*/
};
#pragma pack()

#define	SUSP_PD_SIG1		'P'		/* Value of 'PD' Signature char #1*/
#define	SUSP_PD_SIG2		'D'		/* Value of 'PD' Signature char #2*/
#define	SUSP_PD_VER		1		/* Value of 'PD' Version field	*/



/*
 * SUSP Termination ('ST') SUF.
 */
#pragma pack(1)
struct susp_st {
	uchar_t		st_Sig1;		/* SUF Signature char #1: 'S'	*/
	uchar_t		st_Sig2;		/* SUF Signature char #2: 'T'	*/
	uchar_t		st_Len;			/* TOTAL len of SUF (Bytes)		*/
	uchar_t		st_Ver;			/* ST SUF version #				*/
};
#pragma pack()

#define	SUSP_ST_SIG1		'S'		/* Value of 'ST' Signature char #1*/
#define	SUSP_ST_SIG2		'T'		/* Value of 'ST' Signature char #2*/
#define	SUSP_ST_VER		1		/* Value of 'ST' Version field	*/

	

/*
 * SUSP Extension Reference ('ER') SUF.
 */
#pragma pack(1)
struct susp_er {
	uchar_t		er_Sig1;		/* SUF Signature char #1: 'E' */
	uchar_t		er_Sig2;		/* SUF Signature char #2: 'R' */
	uchar_t		er_Len;			/* TOTAL len of SUF (Bytes) */
	uchar_t		er_Ver;			/* ER SUF version #	*/
	uchar_t		er_IdLen;		/* Len of ER ID string	*/
	uchar_t		er_DescrLen;		/* Len of ER Description string	*/
	uchar_t		er_SrcLen;		/* Len of ER Source string */
	uchar_t		er_ExtVer;		/* Version # of extension */
	uchar_t		er_VarData;		/* Variable length data	*/
	/*
	 * 'er_VarData is a place holder used to locate the beginning 
	 * of the variable length data portion of this structure.
	 * The beginning of each variable length field can be located
	 * by adding the appropriate length fields to the address of
	 * 'er_VarData'.  The variable length fields are as follows:
	 * - SUSP Extension Identifier String
	 * - SUSP Extension Descriptor String
	 * - SUSP Extension Source ID String
	 */
};
#pragma pack()

#define	SUSP_ER_SIG1		'E'		/* Value of 'ER' Signature char #1*/
#define	SUSP_ER_SIG2		'R'		/* Value of 'ER' Signature char #2*/
#define	SUSP_ER_VER		1		/* Value of 'ER' Version field	*/

	

	
/**************************************************************************
 **************************************************************************
 ***																	***
 ***																	***
 ***				 RRIP (Rock Ridge Interchange Protocal) 			***
 ***					Data Structure Definitions						***
 ***																	***
 ***																	***
 **************************************************************************
 **************************************************************************/

/*
 * RRIP values for the SUSP 'ER' SUF.
 */
#define RRIP_ER_VER		1		/* RRIP 'ER' Extension Ver #	*/

#define RRIP_ER_ID_LEN		10		/* RRIP 'ER' ID string			*/
#define RRIP_ER_ID_STRING	"RRIP_1991A"

#define RRIP_ER_DESCR_LEN	84		/* RRIP 'ER' Descriptor string	*/
#define RRIP_ER_DESCR_STRING \
"THE ROCK RIDGE INTERCHANGE PROTOCOL PROVIDES SUPPORT FOR POSIX \
FILE SYSTEM SEMANTICS."

#define RRIP_ER_SRC_LEN		135		/* RRIP 'ER' Source string		*/
#define RRIP_ER_SRC_STRING \
"PLEASE CONTACT DISC PUBLISHER FOR SPECIFICATION SOURCE. SEE PUBLISHER \
IDENTIFIER IN PRIMARY VOLUME DESCRIPTOR FOR CONTACT INFORMATION."



/*
 * RRIP POSIX File Attributes ('PX') SUF.
 */
#pragma pack(1)
struct rrip_px {
	uchar_t		px_Sig1;		/* SUF Signature char #1: 'P' */
	uchar_t		px_Sig2;		/* SUF Signature char #2: 'X' */
	uchar_t		px_Len;			/* TOTAL len of SUF (Bytes) */
	uchar_t		px_Ver;			/* PX SUF version #	*/
	ulong_t		px_Mode;		/* POSIX st_mode: File mode flags*/
	ulong_t		px_Mode_rev;		/* POSIX st_mode: File mode flags*/
	ulong_t		px_LinkCnt;		/* POSIX st_nlink: # of links */
	ulong_t		px_LinkCnt_rev;		/* POSIX st_nlink: # of links */
	ulong_t		px_UserID;		/* POSIX st_uid: User ID # */
	ulong_t		px_UserID_rev;		/* POSIX st_uid: User ID # */
	ulong_t		px_GroupID;		/* POSIX st_gid: Group ID # */
	ulong_t		px_GroupID_rev;		/* POSIX st_gid: Group ID # */
};
#pragma pack()

#define	RRIP_PX_SIG1		'P'		/* Value of 'PX' Signature char #1*/
#define	RRIP_PX_SIG2		'X'		/* Value of 'PX' Signature char #2*/
#define	RRIP_PX_VER		1		/* Value of 'PX' Version field	*/

/*
 * Bit definitions for the RRIP 'PX' mode flags.
 */
#define	RRIP_PX_UREAD		0000400		/* User-read permission bit */
#define	RRIP_PX_UWRITE		0000200		/* User-write permission bit */
#define	RRIP_PX_UEXEC		0000100		/* User-execute permission bit*/
#define	RRIP_PX_GREAD		0000040		/* Group-read permission bit */
#define	RRIP_PX_GWRITE		0000020		/* Group-write permission bit */
#define	RRIP_PX_GEXEC		0000010		/* Group-execute permission bit	*/
#define	RRIP_PX_OREAD		0000004		/* Other-read permission bit */
#define	RRIP_PX_OWRITE		0000002		/* Other-write permission bit */
#define	RRIP_PX_OEXEC		0000001		/* Other-execute permission bit	*/

#define RRIP_PX_SETUID		0004000		/* Set User-ID on execute bit */
#define RRIP_PX_SETGID		0002000 	/* Shared: Set Group-ID on execute bit	*/
#define RRIP_PX_FILELOCK	0002000 	/* Shared: File-locking enforced bit	*/
#define RRIP_PX_STICKY		0001000		/* Save swapped text after use bit*/

#define RRIP_PX_FILETYPE	0170000		/* File-type Mask	*/
#define RRIP_PX_SOCKET		0140000		/* Type = Socket	*/
#define RRIP_PX_SYM		0120000		/* Type = Symbolic Link	*/
#define RRIP_PX_REG		0100000		/* Type = Regular	*/
#define RRIP_PX_BLOCK		0060000		/* Type = Block Special	*/
#define RRIP_PX_DIR		0040000		/* Type = Directory	*/
#define RRIP_PX_CHAR		0020000		/* Type = Character Special */
#define RRIP_PX_PIPE		0010000		/* Type = Pipe or FIFO 	*/



/*
 * RRIP POSIX Device Number ('PN') SUF.
 */
#pragma pack(1)
struct rrip_pn {
	uchar_t		pn_Sig1;		/* SUF Signature char #1: 'P' */
	uchar_t		pn_Sig2;		/* SUF Signature char #2: 'N' */
	uchar_t		pn_Len;			/* TOTAL len of SUF (Bytes)  */
	uchar_t		pn_Ver;			/* PN SUF version #	*/
	ulong_t		pn_DevHigh;		/* Device #: High-order 32-bits */
	ulong_t		pn_DevHigh_rev;		/* Device #: High-order 32-bits */
	ulong_t		pn_DevLow;		/* Device #: Low-order 32-bits */
	ulong_t		pn_DevLow_rev;		/* Device #: Low-order 32-bits */
};
#pragma pack()

#define	RRIP_PN_SIG1		'P'		/* Value of 'PN' Signature char #1*/
#define	RRIP_PN_SIG2		'N'		/* Value of 'PN' Signature char #2*/
#define	RRIP_PN_VER		1		/* Value of 'PN' Version field	*/



/*
 * RRIP Symbolic Link ('SL') SUF.
 */
#pragma pack(1)
struct rrip_sl {
	uchar_t			sl_Sig1;	/* SUF Signature char #1: 'S' */
	uchar_t			sl_Sig2;	/* SUF Signature char #2: 'L' */
	uchar_t			sl_Len;		/* TOTAL len of SUF (Bytes)  */
	uchar_t			sl_Ver;		/* SL SUF version #	*/
	uchar_t			sl_Flags;	/* SL Flags - See below	*/
	uchar_t			sl_CompRec;	/* Start of Component Rec list*/
	/*
	 * 'sl_CompRec' is a place holder used to locate the beginning 
	 * of the SL Component Record data, which contains one or more
	 * SL Component Records.  Each Component Record is variable
	 * length and is defined by the 'rrip_slcr' data structure.
	 */
};
#pragma pack()

#define	RRIP_SL_SIG1		'S'		/* Value of 'SL' Signature char #1*/
#define	RRIP_SL_SIG2		'L'		/* Value of 'SL' Signature char #2*/
#define	RRIP_SL_VER		1		/* Value of 'SL' Version field	*/

/*
 * Special processing flags used to build the RRIP 'SL
 * Symbolic Link name.
 */
#define RRIP_SL_CONTINUE	0x01		/* Name continues with next SL	*/
#define RRIP_SL_RSRVD		0xF7		/* Reserved		*/

/*
 * RRIP Symbolic Link Component Record.
 */
#pragma pack(1)
struct rrip_slcr {
	uchar_t		slcr_Flags;		/* Flags - See below	*/
	uchar_t		slcr_Len;		/* Len of Component data */
	uchar_t		slcr_Comp;		/* Component data	*/
	/*
	 * 'slcr_Comp' is a place holder used to locate the beginning 
	 * of the SL Component data.  The Component data a portion of
	 * the Symbolic Link pathname.
	 */
};
#pragma pack()

#define RRIP_SLCR_SIZE(slcr)	\
	((sizeof(struct rrip_slcr) - sizeof(slcr->slcr_Comp)) + slcr->slcr_Len)


/*
 * Special processing flags for building the RRIP 'SL'
 * Symbolic Link component.
 */
#define RRIP_SLCR_CONTINUE	0x01		/* Component continues w/ next SL*/
#define RRIP_SLCR_CURRENT	0x02		/* Use current dir token ('.')*/
#define RRIP_SLCR_PARENT	0x04		/* Use parent dir token ('..')*/
#define RRIP_SLCR_ROOT		0x08		/* Use root dir token ('/')   */
#define RRIP_SLCR_VOLROOT	0x10		/* Use file-system's mount-point*/
#define RRIP_SLCR_HOST		0x20		/* Use host-system name	*/
#define RRIP_SLCR_RSVD1		0x40		/* Reserved 		*/
#define RRIP_SLCR_RSVD2		0x80		/* Reserved		*/



/*
 * RRIP Alternate Name ('NM') SUF.
 */
#pragma pack(1)
struct rrip_nm {
	uchar_t		nm_Sig1;		/* SUF Signature char #1: 'N' */
	uchar_t		nm_Sig2;		/* SUF Signature char #2: 'M' */
	uchar_t		nm_Len;			/* TOTAL len of SUF (Bytes) */
	uchar_t		nm_Ver;			/* NM SUF version #	*/
	uchar_t		nm_Flags;		/* NM Flags - See below	*/
	uchar_t		nm_Name;		/* Beginning of Alternate Name*/
	/*
	 * 'nm_Name' is a place holder used to locate the beginning 
	 * of the NM Alternate name.  The length of the Alternate Name
	 * can be derived from the length of the NM SUF.
	 */
};
#pragma pack()

#define	RRIP_NM_SIG1		'N'		/* Value of 'NM' Signature char #1*/
#define	RRIP_NM_SIG2		'M'		/* Value of 'NM' Signature char #2*/
#define	RRIP_NM_VER		1		/* Value of 'NM' Version field	*/

/*
 * Special processing flags for building the RRIP 'NM'
 * Alternate name.
 */
#define RRIP_NM_CONTINUE	0x01		/* Name continues with next NM	*/
#define RRIP_NM_CURRENT		0x02		/* Use current dir token eg '.' */
#define RRIP_NM_PARENT		0x04		/* Use parent dir token eg '..'	*/
#define RRIP_NM_RSVD1		0x08		/* Reserved		*/
#define RRIP_NM_RSVD2		0x10		/* Reserved		*/
#define RRIP_NM_HOST		0x20		/* Use host-system name	*/
#define RRIP_NM_RSVD3		0x40		/* Reserved 		*/
#define RRIP_NM_RSVD4		0x80		/* Reserved		*/



/*
 * RRIP Child Link ('CL') SUF.
 */
#pragma pack(1)
struct rrip_cl {
	uchar_t		cl_Sig1;		/* SUF Signature char #1: 'C' */
	uchar_t		cl_Sig2;		/* SUF Signature char #2: 'L' */
	uchar_t		cl_Len;			/* TOTAL len of SUF (Bytes) */
	uchar_t		cl_Ver;			/* CL SUF version #	*/
	ulong_t		cl_Loc;			/* Loc of Child Dir (Log Blk #)	*/
	ulong_t		cl_Loc_rev;		/* Loc of Child Dir (Log Blk #)	*/
};
#pragma pack()

#define	RRIP_CL_SIG1		'C'		/* Value of 'CL' Signature char #1*/
#define	RRIP_CL_SIG2		'L'		/* Value of 'CL' Signature char #2*/
#define	RRIP_CL_VER		1		/* Value of 'CL' Version field	*/



/*
 * RRIP Parent Link ('PL') SUF.
 */
#pragma pack(1)
struct rrip_pl {
	uchar_t		pl_Sig1;		/* SUF Signature char #1: 'P' */
	uchar_t		pl_Sig2;		/* SUF Signature char #2: 'L' */
	uchar_t		pl_Len;			/* TOTAL len of SUF (Bytes) */
	uchar_t		pl_Ver;			/* PL SUF version #	*/
	ulong_t		pl_Loc;			/* Loc of Parent Dir (Log Blk #)*/
	ulong_t		pl_Loc_rev;		/* Loc of Parent Dir (Log Blk #)*/
};
#pragma pack()

#define	RRIP_PL_SIG1		'P'		/* Value of 'PL' Signature char #1*/
#define	RRIP_PL_SIG2		'L'		/* Value of 'PL' Signature char #2*/
#define	RRIP_PL_VER		1		/* Value of 'PL' Version field	*/



/*
 * RRIP Relocated Directory ('RE') SUF.
 */
#pragma pack(1)
struct rrip_re {
	uchar_t		re_Sig1;		/* SUF Signature char #1: 'R' */
	uchar_t		re_Sig2;		/* SUF Signature char #2: 'E' */
	uchar_t		re_Len;			/* TOTAL len of SUF (Bytes) */
	uchar_t		re_Ver;			/* RE SUF version #	*/
};
#pragma pack()

#define	RRIP_RE_SIG1		'R'		/* Value of 'RE' Signature char #1*/
#define	RRIP_RE_SIG2		'E'		/* Value of 'RE' Signature char #2*/
#define	RRIP_RE_VER		1		/* Value of 'RE' Version field	*/



/*
 * RRIP Time Stamp ('TF') SUF.
 */
#pragma pack(1)
struct rrip_tf {
	uchar_t		tf_Sig1;		/* SUF Signature char #1: 'T' */
	uchar_t		tf_Sig2;		/* SUF Signature char #2: 'F' */
	uchar_t		tf_Len;			/* TOTAL len of SUF (Bytes) */
	uchar_t		tf_Ver;			/* TF SUF version #	*/
	uchar_t		tf_Flags;		/* TF Flags - See below	*/
	uchar_t		tf_TimeStamps;		/* One or more time stamp structs*/
	/*
	 * 'tf_TimeStamps' is a place holder used to locate the beginning 
	 * of the first time stamp structure.  Depending on the value of
	 * flags field, the time stamp structures are either a 'Pure9660_adt'
	 * type or a 'Pure9660_hdt'.  The flags field also indicates how
	 * many time-stamp structures exist.
	 */
};
#pragma pack()

#define	RRIP_TF_SIG1		'T'		/* Value of 'TF' Signature char #1*/
#define	RRIP_TF_SIG2		'F'		/* Value of 'TF' Signature char #2*/
#define	RRIP_TF_VER		1		/* Value of 'TF' Version field	*/

/*
 * Bit defenitions for the RRIP 'TF' flags field.
 * These bit indicate whether or not a particular
 * time-stamp has been recorded and if so, the
 * time-structure that was used e.g.  * ISO-9660
 * ASCII-based or ISO-9660 Hex-based time stamp structures.
 */
#define RRIP_TF_CREATE		0x01		/* Create time existence bit */
#define RRIP_TF_MODIFY		0x02		/* Modification time existence bit*/
#define RRIP_TF_ACCESS		0x04		/* Access time existence bit */
#define RRIP_TF_ATTR		0x08		/* Attr Change time existence bit*/
#define RRIP_TF_BACKUP		0x10		/* Last backup time existence bit*/
#define RRIP_TF_EXPIRE		0x20		/* Expiration time existence bit*/
#define RRIP_TF_EFFECT		0x40		/* Effective time existence bit	*/
#define RRIP_TF_LONG		0x80		/* Long (ASCII-based) time structs*/




/*
 * RRIP Existence List ('RR') SUF.
 */
#pragma pack(1)
struct rrip_rr {
	uchar_t		rr_Sig1;		/* SUF Signature char #1: 'R' */
	uchar_t		rr_Sig2;		/* SUF Signature char #2: 'R' */
	uchar_t		rr_Len;			/* TOTAL len of SUF (Bytes) */
	uchar_t		rr_Ver;			/* RR SUF version #	*/
	uchar_t		rr_Flags;		/* RR Flags - See below	*/
};
#pragma pack()

#define	RRIP_RR_SIG1		'R'		/* Value of 'RR' Signature char #1*/
#define	RRIP_RR_SIG2		'R'		/* Value of 'RR' Signature char #2*/
#define	RRIP_RR_VER		1		/* Value of 'RR' Version field	*/

/*
 * Bit definitions of the RRIP 'RR' flags field.
 */
#define RRIP_RR_PX		0x01		/* RRIP 'PX' SUF existence bit	*/
#define RRIP_RR_PN		0x02		/* RRIP 'PN' SUF existence bit	*/
#define RRIP_RR_SL		0x04		/* RRIP 'SL' SUF existence bit	*/
#define RRIP_RR_NM		0x08		/* RRIP 'NM' SUF existence bit	*/
#define RRIP_RR_CL		0x10		/* RRIP 'CL' SUF existence bit	*/
#define RRIP_RR_PL		0x20		/* RRIP 'PL' SUF existence bit	*/
#define RRIP_RR_RE		0x40		/* RRIP 'RE' SUF existence bit	*/
#define RRIP_RR_TF		0x80		/* RRIP 'TF' SUF existence bit	*/


#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_CDFS_ISO9660_H */
