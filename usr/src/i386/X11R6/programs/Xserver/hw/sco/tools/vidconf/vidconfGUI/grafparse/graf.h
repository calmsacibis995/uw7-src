/*
 *	@(#)graf.h	3.1	8/29/96	21:25:28
 *	@(#) graf.h 12.3 95/07/24 
 *
 *	Copyright (C) The Santa Cruz Operation, 1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

#define PROGNAME	"grafparse"

#ifdef GRAF_DEBUG

#define GRAFINFO 	"./grafinfo"
#define DEVICES		"./devices"
#define MONINFO		"./grafmon"

#else

#define GRAFINFO 	GRAFINFO_DIR
#define MONINFO		MONINFO_DIR
#define DEVICES		DEVICES_DIR

#endif

#define XGI 	 	".xgi"
#define MON 	 	".mon"
#define NEWLINE		"\n"

/* token identifiers */
#define VENDOR 		1
#define MODEL 		2
#define CLASS 		3
#define MODE 		4
#define IDENTIFIER      5
#define PROMPT          6
#define DATA            7
#define EQUAL		8
#define SEMICOLON	9
#define VIDSCRIPT      10
#define VIDSETUP       11

/* other constants */
#define MAXFILES 500		/* max number of XGI or MON files loadable */
#define MAXRESLINES 250		/* max number of resolutions in a xgi file */

#define SCRNLEN 80
#define BUFLEN	128		/* max size of char buffer */
#define STRLEN	64		/* size for strings */
#define PATHLEN  128		/* path name length */

#define FNULL (FILE *)0
#define CNULL 	'\0'
#define COLON 	':'
#define NEW_LN  '\n'
#define GOOD 	1
#define BAD	0
#define FATAL   -1
#define TTYDIR  "/dev"

#define OK 	   0		/* Worked okay */
#define NOTOK	   1		/* Something bad happened */
#define USAGE	   2		/* usage error */
#define NOGRAFINFO 3		/* Can't open /usr/lib/grafinfo dir */
#define NOMONINFO  4		/* Can't open /usr/lib/grafinfo/moninfo dir */

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

