#ident	"@(#)pcintf:util/getopt.c	1.2"
#ifdef NOSCCSID
#define SCCSID(arg)
#else
#define SCCSID(arg)	static char Sccsid[] = arg
#endif
SCCSID("@(#)getopt.c	6.1	LCC");	/* Modified: 10/15/90 15:47:53 */
/*	@LCCID(getopt.c, 6.1, 10/15/90, 15:47:53) */
/*
 *   GETOPT.C
 *
 *      This subroutine provides the functionality of the UNIX (TM) System
 *   getopt(3c) library routine for MS/PC-DOS and OS/2. Porting getopt() to
 *   Minix should be trivial. Functionality has been extended by the
 *   addition of 'getoptinit()', a function that can be called to
 *   reinitialize for another pass through the options list.
 *
 *   Author: Gerald L. Lewis
 *   Date:   February 18, 1988
 *
 *   I make this version of getopt() freely available to the public domain
 *   with no warranty whatsoever. The only requirement is that the authorship
 *   note and version date remain intact. 
 *
 */

#include <stdio.h>
#include <string.h>
#ifdef NLS
#include <lmf.h>
#endif /* NLS */

#ifdef MSDOS
#define OPTCHAR '/'
#else
#define OPTCHAR '-'
#endif 

char *optarg = NULL;      /* Points to start of argument when : found */
int   optind = 1;         /* Options index (skips over name operand) */
int   opterr = 0;         /* Used to override default error message */

static int off = 1;       /* Internal option list offset */


void 
getoptinit()     /* Resets getopt global variables */
{
  optarg = NULL;
  optind = 1;
  opterr = 0;
  off    = 1;
}
 

int 
getopt(argc, argv, optstring)
int argc;
char *argv[];
char *optstring;
{
  char *cp;
  int   ch;

  optarg = NULL;

  while ( optind < argc )
  {
     /* Only interested in options */
#ifdef MSDOS	/* allow '/'s */
     if ( argv[optind][0] != '-' && argv[optind][0] != '/')
#else /* MSDOS */
     if ( argv[optind][0] != '-' )
#endif /* MSDOS */
     {
        off = 1;
        return(EOF);
     }

#ifdef MSDOS
     /* "//" or "--" escapes getopt */     
     if ( argv[optind][1] == '/' || argv[optind][1] == '-' )
#else
     if ( argv[optind][1] == '-' )       /* "--" escapes getopt */
#endif
     {
        ++optind;
        off = 1;
        return(EOF);
     }

     if ( (ch = argv[optind][off++]) == '\0' )
     {
        optind++;
        off = 1;
        continue;
     }

     if ( (cp = strchr(optstring, ch)) == NULL)
     {
        if ( !opterr )
#ifdef NLS
	   fputs(lmf_format_string((char *) NULL, 0,
		lmf_get_message("GETOPT1",
		"Unknown option %1%2\n"),
		"%c%c", 
		OPTCHAR, ch),
		stderr);
#else /* NLS */
           fprintf(stderr, "Unknown option %c%c\n", OPTCHAR, ch);
#endif /* NLS */

        return('?');
     }
     else if ( *(cp + 1) != ':' )  {
        return(ch);
     }

     if ( argv[optind][off] != '\0' )
     {
        optarg = &argv[optind++][off];
        off = 1;
        return(ch);
     }

     ++optind;

#ifdef MSDOS	/* allow '/'s */
     if ( (optind >= argc) || (argv[optind][0] == '-' )
	|| ( argv[optind][0] == '/'))
#else /* MSDOS */
     if ( (optind >= argc) || (argv[optind][0] == '-' ))
#endif /* MSDOS */
     {
        if ( !opterr )
#ifdef NLS
	   fputs(lmf_format_string((char *) NULL, 0,
		lmf_get_message("GETOPT2",
		"Missing argument for option %1%2\n"),
		"%c%c",
		 OPTCHAR, ch),
		 stderr);
#else /* NLS */
           fprintf(stderr, "Missing argument for option %c%c\n", OPTCHAR, ch);
#endif /* NLS */
        return('?');
     }

     optarg = argv[optind++];
     off = 1;
     return(ch);
  }

  return(EOF);
}
