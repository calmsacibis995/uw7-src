/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)cdfs.cmds:common/cmd/fs.d/cdfs/mount/mount_local.h	1.4"
#ident	"$Header$"

/*
 * If STATIC is not already defined then define it.
 * If 'lint' is being run, then use the 'static' prefix to keep
 * 'lint' from complaining.  Otherwise, don't use anything.
 */
#ifndef STATIC

#ifdef lint
#define	STATIC static
#else
#define STATIC
#endif

#endif



/*
 * Define the debug support structure.
 */
#ifndef DEBUG

#define DB_CODE(x,y)

#else

#define	DB_CODE(x,y)	if (((x) & db_flags) != 0) y

#define DB_NONE			0x00000000
#define DB_BASE_PARSE	0x00000001
#define DB_XCDR_PARSE	0x00000002
#define DB_RRIP_PARSE	0x00000004
#define DB_PARSE		DB_BASE_PARSE | DB_XCDR_PARSE | DB_DB_RRIP_PARSE

#define DB_BASE_OPTS	0x00000010
#define DB_XCDR_OPTS	0x00000020
#define DB_RRIP_OPTS	0x00000040
#define DB_OPTS	 		DB_BASE_OPTS | DB_XCDR_OPTS | DB_DB_RRIP_OPTS

#define DB_MOUNT_CMD	0x00000100
#define DB_XCDR_CMD		0x00000200
#define DB_RRIP_CMD		0x00000400
#define DB_CMD			DB_BASE_CMD | DB_XCDR_CMD | DB_DB_RRIP_CMD

#define DB_MT_UPDATE	0x00001000

STATIC u_int	db_flags = DB_NONE;
#endif


#define		RET_OK		0
#define		RET_ERR		1
#define		RET_OTHER	-1


/* 
 * Define external references.
 */
#ifdef FIXED
extern int		errno;						/* System call error number		*/

extern int		optind;						/* getopts(): Option index		*/
extern char		*optarg;					/* getopts(): Option arugment	*/
extern int		opterr;						/* getopts(): Error message mode*/
extern int		optopt;						/* getopts(): Erroneous option	*/

extern int		getopt();					/* Get next command-line option */
extern int		getsubopt();				/* Get next sub-option			*/
extern int		getmntent();				/* Get a mount-table entry		*/
extern void		(*signal())();				/* Set signal processing mode	*/
extern int		strcmp();					/* Compare two strings			*/
extern u_int	strlen();					/* Get length of a string		*/
extern char		*strncpy();					/* Copy a string				*/
extern char		*strrchr();					/* Find a char within a string	*/
extern time_t	time();						/* Get the current system time	*/
#endif



/*
 * Local subroutine list.
 */
STATIC void	DispUsage();					/* Display the usage message	*/
STATIC void	DispMntErr(char *, char *);		/* Display a mount(2) error msg	*/ 
STATIC int 	ParseOpts(u_int, char *[]);		/* Parse the command-line options*/
STATIC int 	ParseXcdrOpt(char *);			/* Parse the XCDR options		*/
STATIC int 	ParseRripOpt(char *);			/* Parse the RRIP options		*/
STATIC int 	BldOpts(char *, u_int);			/* Build the final option list	*/
STATIC int 	BldXcdrOpts(char *, u_int);		/* Build final XCDR option list	*/
STATIC int 	BldRripOpts(char *, u_int);		/* Build final RRIP option list	*/
STATIC int 	BldXcdrCmd(char *, char *, u_int);	/* Build XCDR command string*/
STATIC int 	BldRripCmd(char *, char *, u_int);	/* Build RRIP command string*/

