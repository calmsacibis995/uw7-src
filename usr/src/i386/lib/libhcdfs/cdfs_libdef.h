/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to USL under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)libcdfs:i386/lib/libhcdfs/cdfs_libdef.h	1.5"
#ident	"$Header$"

/*
 * Library defines for cdfs.
 */

#ifndef _LIBCDFS_CDFS_LIBDEF_H
#define _LIBCDFS_CDFS_LIBDEF_H

#include <sys/mnttab.h>
#include <sys/types.h>
#include <sys/fs/iso9660.h>

static uint_t		cdfs_DaysOfMonth[] =
						{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#define USHRT_MAX		65535		/* max value of an "unsigned short int" */
#define CDFS_HDT_TYPE	0x0100
#define CDFS_ADT_TYPE	0x1000

#define CDFS_BINARY_FIELD	0x1111
#define CDFS_STRING			0x1000

static void			(*oldhup) ();
static void			(*oldint) ();
static void			(*oldquit) ();
static void			(*oldsys) ();

/*
 * Prototypes for library functions that may not be externally referenced.
 */
#ifdef __STDC__

static void			cd_PrintString (const uchar_t *, const uint_t);
static void			cd_signal (int);
static void			cd_DoSignal ();
static void			cd_UndoSignal ();
static int			cd_GetDevName (const char *, char *);
static int			cd_FillPvd (struct iso9660_pvd *, union media_pvd *,
								enum cdfs_type);
static int			cd_FillXar (struct iso9660_xar *, union media_xar *,
								int, int, enum cdfs_type);
static int			cd_FillDRec (struct iso9660_drec *, union media_drec *,
								enum cdfs_type);
static int			cd_FillPTRec (struct iso9660_ptrec *, union media_ptrec *,
								enum cdfs_type);
static ushort_t		cd_ToDigit (uchar_t *, uint_t);
static ushort_t		cd_GetPerms (uchar_t, uchar_t);
static boolean_t	cd_FsMember (const char *);
static boolean_t	cd_IsMntPt (const char *, struct mnttab *);
static int			cd_PrintDate (char *, caddr_t, uint_t, enum cdfs_type);
static void			cd_FillField (void *, uint_t, void *, uint_t, uint_t);

#else

static void			cd_PrintString ();
static void			cd_signal ();
static void			cd_DoSignal ();
static void			cd_UndoSignal ();
static int			cd_GetDevName ();
static int			cd_FillPvd ();
static int			cd_FillXar ();
static int			cd_FillDRec ();
static int			cd_FillPTRec ();
static ushort_t		cd_ToDigit ();
static ushort_t		cd_GetPerms ();
static boolean_t	cd_FsMember();
static boolean_t	cd_IsMntPt ();
static int			cd_PrintDate ();
static void			cd_FillField ();

#endif	/* __STDC__ */

#endif	/* ! _LIBCDFS_CDFS_LIBDEF_H */
