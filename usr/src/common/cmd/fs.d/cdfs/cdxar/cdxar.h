/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to USL under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)cdfs.cmds:common/cmd/fs.d/cdfs/cdxar/cdxar.h	1.4"
#ident	"$Header$"

/* Tabstops: 4 */

/*
 * Local include file for cdxar(1M) utility.
 */

#include <sys/cdrom.h>
#include <sys/types.h>


/*
 * Global variables:
 */
static uchar_t		*ProgName;						/* Program name			*/
static char			*Msg;							/* Error message pointer*/
static char			*File;							/* File to get DREC from*/
static int			Section = -1;					/* File Section			*/
static boolean_t	Binary = B_FALSE;				/* Binary output?		*/
static char			*Buf;							/* XAR buffer			*/
static struct iso9660_pvd PVD;						/* PVD structure		*/
static struct iso9660_xar XAR;						/* XAR structure		*/
static struct iso9660_drec DREC;					/* DREC structure		*/
static ushort_t		XARBlks;						/* Size of XAR in blks	*/
static uint_t		XARBytes;						/* Size of XAR in bytes	*/


/*
 * Local definitions.
 */
#define ONLY_NOTIFY			0
#define EXIT_CODE_1			1
#define EXIT_CODE_2			2
#define EXIT_CODE_3			3
#define EXIT_CODE_4			4

#define SHOW_USAGE			12
#define NO_USAGE			13

#define UNKNOWN_EXIT_STATUS	0


/*
 * Function prototypes:
 */
#ifdef __STDC__

static void			DoLocale ();
static void			ParseOpts (int, char * []);
static void			DispUsage ();
static void			ErrMsg (long, uint_t, uint_t, const char *, void *,
								void *, void *);
static void			InterpretFailure (uint_t);
static void			GetXARSize ();
static void			GetXAR ();
static void			PrintXAR ();

#else

static void			DoLocale ();
static void			ParseOpts ();
static void			DispUsage ();
static void			ErrMsg ();
static void			InterpretFailure ();
static void			GetXARSize ();
static void			GetXAR ();
static void			PrintXAR ();

#endif		/* __STDC__ */
