#ident	"@(#)cvtomf:cvtomf.c	1.1"

/*
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1989 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*
 *	Copyright (c) Altos Computer Systems, 1987
 */
/* Enhanced Application Compatibility Support */
/*	MODIFICATION HISTORY
 *
 *	S000	9/20/89		sco!kai
 *	- major re-write of enitre cvtomf tool.
 *	S001	3/13/90		sco!kai
 *	- unmarked: changes to fix a shared lib problem resulted in
 *	a "one-pass" converter again. $$TYPES and $$SYMBOLS stuff is
 *	now called from within omf() in the event that -g is specified. 
 *	PASS1 and PASS2 switch is still used in pieces of omf()
 */

/*	
 *	cvtomf: OMF to COFF conversion utility 
 *
 *	usage: 	cvtomf [-g] [-d] [-v] [-o file ] omf.o [omf.o . . .]
 *
 *	-g	translate symbolic debug data
 *	-d	debug output
 *	-v	verbose output (summary of COFF components generated)
 *	-o 	specify output file, else retain same file names
 *		this is only permitted with one omf.o file
 */

#include	"cvtomf.h"
#include	"varargs.h"

char	*argv0;		/* program name */
int	verbose;	/* command line options */
int 	do_debug;	/* translate ISLAND symbolic debug data */

#ifdef DEBUG
int	debug;
static  char *opts = "dgvo:";
#else
static  char *opts = "gvo:";
#endif


main(argc, argv)
int argc;
char **argv;
{
	int	c;
	char	*output = NULL;

	argv0 = argv[0];
	opterr = 0;

	/* process command line args */
	while ((c = getopt(argc, argv, opts)) != EOF) {
		switch (c) {

#ifdef DEBUG
		case ('d'):
			debug++;
			break;
#endif
		case ('v'):
			verbose++;
			break;

		case ('o'):
			if ( output != NULL )
				warning("Multiple -o options specified");
			output = optarg;
			break;

		case ('g'):
			do_debug = 1;
			break;

		case ('?'):
			fatal(USAGE);
			break;

		default:
			/* "Can't happen" */
			break;
		}
	}

	if ( ((argc - optind) > 1) && (output != NULL) )
		fatal("Cannot specify output file for more than one input");

	if ( optind >= argc )
		fatal("No input file specified");

	/* process each OMF file */
	do {

#ifdef DEBUG
		if(debug)
			fprintf(stderr,"processing objfile: %s\n",argv[optind]);
#endif
		/* if obfile is already COFF (or error), skip */
		if(!open_omf(argv[optind]))
			continue;

		omf(PASS1);

		/* Generate COFF file */
		coff((output == NULL) ? argv[optind] : output );

		close_omf();

	} while(++optind < argc);

	exit(0);
}


/*
 *	tmpfile: create a temporary file
 */
FILE *
tmpfile()
{
	char *name;
	FILE *file;

	name = tmpnam((char *) NULL);
	file = fopen(name, "w+");

	if (!file){
		perror("fopen");
		fatal("Cannot open temporary file");
	}

	unlink(name);

	return(file);
}


/*
 *	warning: print an error message (non-fatal)
 */
/*VARARGS0*/
void
warning(va_alist)
va_dcl
{
	va_list args;
	char *format;
	
	va_start(args);
	format = va_arg(args, char *);
	fprintf(stderr, "%s: warning -- ", argv0);
	vfprintf(stderr,format,args);
	putc('\n',stderr);
	va_end(args);
}


/*
 *	fatal: print an error message and exit
 */
/*VARARGS0*/
void
fatal(va_alist)
va_dcl
{
	va_list args;
	char *format;
	
	va_start(args);
	format = va_arg(args, char *);
	fprintf(stderr,"%s: fatal error -- ",argv0);
	vfprintf(stderr,format,args);
	putc('\n',stderr);
	va_end(args);
	exit(1);
	/* NOTREACHED */
}
/* End Enhanced Application Compatibility Support */
