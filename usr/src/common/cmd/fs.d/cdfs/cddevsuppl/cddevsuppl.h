/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to USL under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)cdfs.cmds:common/cmd/fs.d/cdfs/cddevsuppl/cddevsuppl.h	1.4"
#ident	"$Header$"

/* Tabstops: 4 */

/*
 * Local include file for cddevsuppl(1M) utility.
 */

#include <sys/cdrom.h>
#include <sys/types.h>
#include <sys/param.h>


/*
 * Structures:
 */
struct dev_map_entry {
	char			DevPath[MAXPATHLEN];			/* Device path to map	*/
	int				Major;							/* New major number		*/
	int				Minor;							/* New minor number		*/
};


/*
 * Global variables:
 */
static uchar_t		*ProgName;						/* Program name			*/
static char			*Msg;							/* Error message pointer*/
static uchar_t		MapFile[MAXPATHLEN] = "";		/* Mapping file			*/
static boolean_t	TmpCont = B_FALSE;				/* Hold val during parsing*/
static boolean_t	ContOnError = B_FALSE;			/* Flag continue on err	*/
static boolean_t	DoMap = B_FALSE;				/* Flag to do mappings	*/
static boolean_t	DoUnmap = B_FALSE;				/* Flag to undo mappings*/
static struct dev_map_entry Maps[CD_MAXDMAP];		/* Array of mappings	*/


/*
 * Local definitions.
 */
#define ONLY_NOTIFY			0
#define EXIT_CODE_1			1
#define EXIT_CODE_2			2
#define EXIT_CODE_3			3
#define EXIT_CODE_4			4
#define EXIT_CODE_5			5
#define EXIT_CODE_6			6

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
static void			DoMappings ();

#else

static void			DoLocale ();
static void			ParseOpts ();
static void			DispUsage ();
static void			ErrMsg ();
static void			InterpretFailure ();
static void			DoMappings ();

#endif		/* __STDC__ */
