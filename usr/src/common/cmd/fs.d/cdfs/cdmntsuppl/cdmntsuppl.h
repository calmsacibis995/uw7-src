/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to USL under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)cdfs.cmds:common/cmd/fs.d/cdfs/cdmntsuppl/cdmntsuppl.h	1.4"
#ident	"$Header$"

/* Tabstops: 4 */

/*
 * Local include file for cdmntsuppl(1M) utility.
 */

#include <sys/cdrom.h>
#include <sys/types.h>


/*
 * Constants:
 */
#define				USER					0500	/* User perms mask		*/
#define				GROUP					0050	/* Group perms mask		*/
#define				OTHER					0005	/* Other perms mask		*/
#define				READ					0444	/* Read perms mask		*/
#define				EXECUTE					0111	/* Execute perms mask	*/
#define				ALL						0555	/* All allowed perms	*/


/*
 * Global variables:
 */
static uchar_t		*ProgName;						/* Program name			*/
static uchar_t		*MntPt;							/* Mount point			*/
static char			*Msg;							/* Error message pointer*/
static uchar_t		*OwnerString = NULL;			/* Owner (from cmd line)*/
static uid_t		Owner;							/* Parsed dflt owner	*/
static uchar_t		*GroupString = NULL;			/* Group (from cmd line)*/
static gid_t		Group;							/* Parsed dflt group	*/
static uchar_t		*FModeString = NULL;			/* File perm string	*/
static mode_t		FMode;							/* Default file perms	*/
static uchar_t		FModeOp;						/* Perms operator +|-|=	*/
static uchar_t		*DModeString = NULL;			/* Dir perm string	*/
static mode_t		DMode;							/* Default dir perms	*/
static uchar_t		DModeOp;						/* Perms operator +|-|=	*/
static uchar_t		*UMFile = NULL;					/* UID map file			*/
static uchar_t		*GMFile = NULL;					/* GID map file			*/
static boolean_t	DoConversion = B_FALSE;			/* Name conversion flag	*/
static uint_t		NameConv = 0;					/* Name conversion value*/
static boolean_t	DirInterpFlag = B_FALSE;		/* Dir srch perms set?	*/
static uint_t		DirSearch = 0;					/* Dir srch perms value	*/
static boolean_t	DoDefaults = B_FALSE;			/* Setting dflts flag	*/
static boolean_t	DoMapping = B_FALSE;			/* Setting name conv flg*/
static struct cd_idmap	UIDMap[CD_MAXUMAP];			/* UID mapping array	*/
static struct cd_idmap	GIDMap[CD_MAXGMAP];			/* GID mapping array	*/


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
static void			ValParams ();
static void			DispUsage ();
static void			ErrMsg (long, uint_t, uint_t, const char *, void *,
								void *, void *);
static void			SetDefaults ();
static void			SetMappings ();
static void			SetNameConv ();
static void			GetSettings ();
static boolean_t	IsDecimal (uchar_t *);
static boolean_t	IsOctal (uchar_t *);
static mode_t		GetPerms (uchar_t *, uchar_t *);
static mode_t		who (uchar_t *);
static int			what (uchar_t *);
static void			InterpretFailure (uint_t);

#else

static void			DoLocale ();
static void			ParseOpts ();
static void			ValParams ();
static void			DispUsage ();
static void			ErrMsg ();
static void			SetDefaults ();
static void			SetMappings ();
static void			SetNameConv ();
static void			GetSettings ();
static boolean_t	IsDecimal ();
static boolean_t	IsOctal ();
static mode_t		GetPerms ();
static mode_t		who ();
static int			what ();
static void			InterpretFailure ();

#endif		/* __STDC__ */
