/*		copyright	"%c%" 	*/

#ident	"@(#)postprint.c	1.3"
#ident	"$Header$"

/*******************************************************************************
 *
 * FILENAME:    postprint.c
 *
 * DESCRIPTION: postprint - PostScript translator for ASCII files.
 *              A simple program that translates ASCII files into PostScript. 
 *              All it really does is expand tabs and backspaces, handle 
 *              character quoting, print text lines, and control when pages are
 *              started based on the requested number of lines per page.
 *
 * SCCS:	postprint.c 1.3  9/1/97 at 11:07:03
 *
 * CHANGE HISTORY:
 *
 * 01-09-97  Paul Cunningham        ul96-25718
 *           Change function options() in case 'e', so that it uses a call to
 *           function get_encode() to get the size of the memory required to 
 *           malloc for both option types.
 *
 *******************************************************************************
 */

/* postprint - PostScript translator for ASCII files.
 *
 * A simple program that translates ASCII files into PostScript. All it really
 * does is expand tabs and backspaces, handle character quoting, print text 
 * lines, and control when pages are started based on the requested number of 
 * lines per page.
 *
 * The PostScript prologue is copied from *prologue before any of the input 
 * files are translated. The program expects that the following procedures are 
 * defined in that file:
 *
 *	setup
 *
 *	  mark ... setup -
 *
 *	    Handles special initialization stuff that depends on how the program
 *	    was called. Expects to find a mark followed by key/value pairs on 
 *          the stack. The def operator is applied to each pair up to the mark,
 *          then the default state is set up.
 *
 *	pagesetup
 *
 *	  page pagesetup -
 *
 *	    Does whatever is needed to set things up for the next page. Expects
 *	    to find the current page number on the stack.
 *
 *	l
 *
 *	  string l -
 *
 *	    Prints string starting in the first column and then goes to the next
 *	    line.
 *
 *	L
 *
 *	  mark string column string column ... L mark
 *
 *	    Prints each string on the stack starting at the horizontal position
 *	    selected by column. Used when tabs and spaces can be sufficiently 
 *          well compressed to make the printer overhead worthwhile. Always 
 *          used when we have to back up.
 *
 *	done
 *
 *	  done
 *
 *	    Makes sure the last page is printed. Only needed when we're printing
 *	    more than one page on each sheet of paper.
 *
 * Almost everything has been changed in this version of postprint. The program
 * is more intelligent, especially about tabs, spaces, and backspacing, and as a
 * result output files usually print faster. Output files also now conform to
 * Adobe's file structuring conventions, which is undoubtedly something I should
 * have done in the first version of the program. If the number of lines per 
 * page is set to 0, which can be done using the -l option, pointsize will be 
 * used to guess a reasonable value. The estimate is based on the values of 
 * LINESPP, POINTSIZE, and pointsize, and assumes LINESPP lines would fit on a 
 * page if we printed in size POINTSIZE. Selecting a point size using the -s 
 * option and adding -l0 to the command line forces the guess to be made.
 *
 * Many default values, like the magnification and orientation, are defined in 
 * the prologue, which is where they belong. If they're changed (by options), an
 * appropriate definition is made after the prologue is added to the output 
 * file. The -P option passes arbitrary PostScript through to the output file. 
 * Among other things it can be used to set (or change) values that can't be 
 * accessed by other options.
 * 
 * Postprint is able to print 8-bits characters as well. Two new options
 * was added. Option -f ( font specification ) was enhanced to deal with
 * multiple fonts. Option -e was added for specifying encoding vector for
 * appropriate font. Option -E was added for specifying file name ( full path)
 * of file with encoding vector aliases. Default file is 
 * /usr/share/lib/hostfontdir/encodemap.
 *
 */


#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "comments.h"			/* PostScript file structuring comments */
#include "gen.h"			/* general purpose definitions */
#include "path.h"			/* for the prologue */
#include "ext.h"			/* external variable declarations */
#include "postprint.h"			/* a few special definitions */


char	*optnames = "a:c:e:f:l:m:n:o:p:r:s:t:x:y:A:C:E:J:L:P:R:DI";

char	*prologue = POSTPRINT;		/* default PostScript prologue */
char	*formfile = FORMFILE;		/* stuff for multiple pages per sheet */

int	formsperpage = 1;		/* page images on each piece of paper */
int	copies = 1;			/* and this many copies of each sheet */

int	linespp = LINESPP;		/* number of lines per page */
int	pointsize = POINTSIZE;		/* in this point size */
int	tabstops = TABSTOPS;		/* tabs set at these columns */
int	crmode = 0;			/* carriage return mode - 0, 1, or 2 */

int	col = 1;			/* next character goes in this column */
int	line = 1;			/* on this line */

int	stringcount = 0;		/* number of strings on the stack */
int	stringstart = 1;		/* column where current one starts */
int	font_flag = CS0;		/* indicates which font is active.
					 */
int	encode_flag = CS0;		/* indicates which encoding is active.
					 */

int	count = 0; 			/* number of encoding vectors aliases */

int	nl = YES;			/* indicates if function newline
					 * was used, i.e. if last printed line
					 * was empty.
					 */

int     cuse0 = NO;			/* indicates if user has specified
					 * f 0,name   */
int     cuse1 = NO;			/* indicates if user has specified
					 * f 1,name   */
int     euse0 = NO;			/* indicates if user has specified
					 * e 0,name   */
int     euse1 = NO;			/* indicates if user has specified
					 * e 1,name   */
int     euse = NO;			/* indicates if user has specified
					 * e name   */
					  
int	ndf = NO; 			/* indicates, if user has specified
					 * font instead of default font. */

Encodemap *encodemap;			/* for tranlating codeset names */
char	*encode[4];			/* names of encoding, user can specify*/
char	*encoding = "StandardEncoding";	/* default encoding  */
char	*fontname = "Courier";		/* use this PostScript font as default*/


char	*codealias =  "/usr/share/lib/hostfontdir/encodemap";

					/* Default aliases files */

char	*csname[4] ;			/* PostScript fonts, user can specify */

char	*dwlname[4] ;			/* original font names specified by
					   user */

int	created[4] ;			/* marks if new font font with
					   new encoding was created */

int	page = 0;			/* page we're working on */
int	printed = 0;			/* printed this many pages */

FILE	*fp_in = stdin;			/* read from this file */
FILE	*fp_out = stdout;		/* and write stuff here */
FILE	*fp_acct = NULL;		/* for accounting data */


/*****************************************************************************/


void main(agc, agv)


    int		agc;
    char	*agv[];


{


/*
 *
 * A simple program that translates ASCII files into PostScript. If there's more
 * than one input file, each begins on a new page.
 *
 */


    argc = agc;				/* other routines may want them */
    argv = agv;

    prog_name = argv[0];		/* really just for error messages */

    init_signals();			/* sets up interrupt handling */
    read_aliases();			/* read aliases from file */
    header();				/* PostScript header and prologue */
    options();				/* handle the command line options */
    setup();				/* for PostScript */
    arguments();			/* followed by each input file */
    done();				/* print the last page etc. */
    account();				/* job accounting data */

    exit(x_stat);			/* not much could be wrong */

}   /* End of main */


/*****************************************************************************/


void init_signals(void)


{


    void	interrupt();		/* signal handler */


/*
 *
 * Makes sure we handle interrupts.
 *
 */


    if ( signal(SIGINT, interrupt) == SIG_IGN )  {
	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGHUP, SIG_IGN);
    } else {
	(void)signal(SIGHUP, interrupt);
	(void)signal(SIGQUIT, interrupt);
    }   /* End else */

    (void)signal(SIGTERM, interrupt);

}   /* End of init_signals */


/*****************************************************************************/


void header(void)


{


    int		ch;			/* return value from getopt() */
    int		old_optind = optind;	/* for restoring optind - should be 1 */


/*
 *
 * Scans the option list looking for things, like the prologue file, that we need
 * right away but could be changed from the default. Doing things this way is an
 * attempt to conform to Adobe's latest file structuring conventions. In particular
 * they now say there should be nothing executed in the prologue, and they have
 * added two new comments that delimit global initialization calls. Once we know
 * where things really are we write out the job header, follow it by the prologue,
 * and then add the ENDPROLOG and BEGINSETUP comments.
 *
 */


    while ( (ch = getopt(argc, argv, optnames)) != EOF )
	if ( ch == 'L' )
	    prologue = optarg;
	else if ( ch == '?' )
	    error(FATAL, "", dummy, dummy, dummy);

    optind = old_optind;		/* get ready for option scanning */

    (void)fprintf(stdout, "%s", CONFORMING);
    (void)fprintf(stdout, "%s %s\n", VERSION, PROGRAMVERSION);
    (void)fprintf(stdout, "%s %s\n", DOCUMENTFONTS, ATEND);
    (void)fprintf(stdout, "%s %s\n", PAGES, ATEND);
    (void)fprintf(stdout, "%s", ENDCOMMENTS);

    if ( cat(prologue) == FALSE )
	error(FATAL, "can't read %s", (unsigned)prologue, dummy, dummy);

    (void)fprintf(stdout, "%s", ENDPROLOG);
    (void)fprintf(stdout, "%s", BEGINSETUP);
    (void)fprintf(stdout, "mark\n");

}   /* End of header */


/*****************************************************************************/


void options(void)


{


    int		ch;			/* return value from getopt() */

/*
 *
 * Reads and processes the command line options. Added the -P option so arbitrary
 * PostScript code can be passed through. Expect it could be useful for changing
 * definitions in the prologue for which options have not been defined.
 *
 * Although any PostScript font can be used, things will only work well for
 * constant width fonts.
 * 
 * Added -e option ( for specifying encoding vectors). Also the -f options
 * has been enhanceded for using several fonts. 
 *  
 * Added -E options for specifying path for encoding vectors aliases file.
 *
 */


    while ( (ch = getopt(argc, argv, optnames)) != EOF )  {

	switch ( ch )  {

	    case 'a':			/* aspect ratio */
		    (void)fprintf(stdout, "/aspectratio %s def\n", optarg);
		    break;

	    case 'c':			/* copies */
		    copies = atoi(optarg);
		    (void)fprintf(stdout, "/#copies %s store\n", optarg);
		    break;

	    case 'e':
	    {
		    if ( (optarg[0]=='0' || optarg[0]=='1' ||
			  optarg[0]=='2' || optarg[0]=='3') &&
			 (optarg[1]==',') ) {

			if ((encode[optarg[0]-'0'] =
			         malloc
				   (
				   (char)(strlen(get_encode(optarg+2))+2)
				   ))== NULL)
			{
				error(FATAL, "Can't allocate memory",
				       dummy, dummy, dummy);
			}
			(void)strcpy(encode[optarg[0]-'0'], get_encode(optarg+2));
			if (optarg[0]=='0')
			   euse0 = YES;
			if (optarg[0]=='1')
			   euse1 = YES;
		    }
		    else
		    {
		      if (( encoding = 
			      malloc(strlen( get_encode( optarg))+2))==NULL) 
		      {
			error(FATAL, "Can't allocate memory",
				       dummy, dummy, dummy);
		      }
		      (void)strcpy(encoding, get_encode(optarg));
		      euse = YES;
		    }
		    break;
	    }

	    case 'f':			/* use this PostScript font */
		    if ( (optarg[0]=='0' || optarg[0]=='1' ||
			  optarg[0]=='2' || optarg[0]=='3') &&
			 (optarg[1]==',') ) {
		       if ((csname[optarg[0]-'0'] = malloc(strlen(optarg)+2))==NULL){
				error(FATAL, "Can't allocate memory",
				       dummy, dummy, dummy);
		       }
		       (void)strcpy(csname[optarg[0]-'0'], optarg+2);
		       if (optarg[0]=='0') {
			   cuse0 = YES;	
			   if ((dwlname[optarg[0]-'0'] = malloc(strlen(optarg)+2))==NULL){
				error(FATAL, "Can't allocate memory",
				       dummy, dummy, dummy);
		       	   }
		       (void)strcpy(dwlname[optarg[0]-'0'], csname[0]);
 		       }
			if (optarg[0]=='1') {
			   cuse1 = YES;
			   if ((dwlname[optarg[0]-'0'] = malloc(strlen(optarg)+2))==NULL){
				error(FATAL, "Can't allocate memory",
				       dummy, dummy, dummy);
		       	   }
		       (void)strcpy(dwlname[optarg[0]-'0'], csname[1]);
 		       }
		       if ((optarg[0]=='2') || (optarg[0]=='3')){
			   if ((dwlname[optarg[0]-'0'] = malloc(strlen(optarg)+2))==NULL){
				error(FATAL, "Can't allocate memory",
				       dummy, dummy, dummy);
		       	   }
		       (void)strcpy(dwlname[optarg[0]-'0'], csname[optarg[0]-'0']);
		       }
		    }
		    else {
   		      if ((fontname = malloc(strlen(optarg)+2))==NULL) {
			error(FATAL, "Can't allocate memory",
				       dummy, dummy, dummy);
		      }
		    (void)strcpy(fontname, optarg);
		    ndf = YES;
		    }
		    break;

	    case 'l':			/* lines per page */
		    linespp = atoi(optarg);
		    break;

	    case 'm':			/* magnification */
		    (void)fprintf(stdout, "/magnification %s def\n", optarg);
		    break;

	    case 'n':			/* forms per page */
		    formsperpage = atoi(optarg);
		    (void)fprintf(stdout, "%s %s\n", FORMSPERPAGE, optarg);
		    (void)fprintf(stdout, "/formsperpage %s def\n", optarg);
		    break;

	    case 'o':			/* output page list */
		    out_list(optarg);
		    break;

	    case 'p':			/* landscape or portrait mode */
		    if ( *optarg == 'l' )
			(void)fprintf(stdout, "/landscape true def\n");
		    else (void)fprintf(stdout, "/landscape false def\n");
		    break;

	    case 'r':			/* carriage return mode */
		    crmode = atoi(optarg);
		    break;

	    case 's':			/* point size */
		    pointsize = atoi(optarg);
		    (void)fprintf(stdout, "/pointsize %s def\n", optarg);
		    break;

	    case 't':			/* tabstops */
		    tabstops = atoi(optarg);
		    break;

	    case 'x':			/* shift things horizontally */
		    (void)fprintf(stdout, "/xoffset %s def\n", optarg);
		    break;

	    case 'y':			/* and vertically on the page */
		    (void)fprintf(stdout, "/yoffset %s def\n", optarg);
		    break;

	    case 'A':			/* force job accounting */
	    case 'J':
		    if ( (fp_acct = fopen(optarg, "a")) == NULL )
			error(FATAL, "can't open accounting file %s", (unsigned)optarg, dummy, dummy);
		    break;

	    case 'C':			/* copy file straight to output */
		    if ( cat(optarg) == FALSE )
			error(FATAL, "can't read %s", (unsigned)optarg, dummy, dummy);
		    break;
	    
	    case 'E':			/* Encoding vectors aliases file */
		    if ((codealias = malloc(strlen(optarg)+2))==NULL) {
			error(FATAL, "Can't allocate memory",
			      dummy, dummy, dummy);
		    }
		    (void)strcpy(codealias ,optarg);
		    break;
	
	    case 'L':			/* PostScript prologue file */
		    prologue = optarg;
		    break;

	    case 'P':			/* PostScript pass through */
		    (void)fprintf(stdout, "%s\n", optarg);
		    break;

	    case 'R':			/* special global or page level request */
		    saverequest(optarg);
		    break;

	    case 'D':			/* debug flag */
		    debug = ON;
		    break;

	    case 'I':			/* ignore FATAL errors */
		    ignore = ON;
		    break;

	    case '?':			/* don't understand the option */
		    error(FATAL, "", dummy, dummy, dummy);
		    break;

	    default:			/* don't know what to do for ch */
		    error(FATAL, "missing case for option %c\n", (unsigned) ch, dummy, dummy);
		    break;

	}   /* End switch */

    }   /* End while */

    argc -= optind;			/* get ready for non-option args */
    argv += optind;
    if ( csname[0] == NULL ) {
	if((csname[0] =malloc(strlen(fontname)+2))==NULL) {
	  error(FATAL, "Can't allocate memory",
		dummy, dummy, dummy);
	}
    (void)strcpy( csname[0], fontname);
	if((dwlname[0] =malloc(strlen(fontname)+2))==NULL) {
	  error(FATAL, "Can't allocate memory",
		dummy, dummy, dummy);
	}
    (void)strcpy( dwlname[0], fontname);
    }

    if ( csname[1] == NULL ) {
	if((csname[1] =malloc(strlen(fontname)+2))==NULL) {
	  error(FATAL, "Can't allocate memory",
		dummy, dummy, dummy);
	}
    (void)strcpy( csname[1], fontname);
	if((dwlname[1] =malloc(strlen(fontname)+2))==NULL) {
	  error(FATAL, "Can't allocate memory",
		dummy, dummy, dummy);
	}
    (void)strcpy( dwlname[1], fontname);
    }

    if ( encode[0] == NULL ) {
	if((encode[0] =malloc(strlen(encoding)+2))==NULL) {
	  error(FATAL, "Can't allocate memory",dummy,dummy,dummy);
	}
    (void)strcpy( encode[0], encoding);
    }
    if ( encode[1] == NULL ) {
	if((encode[1] =malloc(strlen(encoding)+2))==NULL) {
	  error(FATAL, "Can't allocate memory",dummy, dummy,dummy);
	}
    (void)strcpy( encode[1], encoding);
    }
  }   /* End of options */


/*****************************************************************************/


char *get_encode(code)


    char	*code;			/* code the user asked for */


{


    int		index;			/* for looking through encodemap[] */


/*
 *
 * Called from options() to map a codeset name into a legal PostScript
 * encoding vector. 
 * If the lookup fails *code is returned to the caller. 
 *
 */


    for ( index = 0; index <= count; index++ )
	if ( strcmp(code, encodemap[index].name) == 0 ) 
	    return(encodemap[index].val);
 	

    return(code);

}   /* End of get_encode */

/*****************************************************************************/

void setup(void)


{


/*
 *
 * Handles things that must be done after the options are read but before the
 * input files are processed. linespp (lines per page) can be set using the -l
 * option. If it's not positive we calculate a reasonable value using the
 * requested point size - assuming LINESPP lines fit on a page in point size
 * POINTSIZE.
 *
 */


    writerequest(0, stdout);		/* global requests eg. manual feed */
    (void)fprintf(stdout, "setup\n");

    if ( formsperpage > 1 )  {
	if ( cat(formfile) == FALSE )
	    error(FATAL, "can't read %s", (unsigned)formfile, dummy,dummy);
	(void)fprintf(stdout, "%d setupforms\n", formsperpage);
    }	/* End if */

    (void)fprintf(stdout, "%s", ENDSETUP);

    if ( linespp <= 0 )
	linespp = LINESPP * POINTSIZE / pointsize;

}   /* End of setup */


/*****************************************************************************/


void arguments(void)


{


/*
 *
 * Makes sure all the non-option command line arguments are processed. If we get
 * here and there aren't any arguments left, or if '-' is one of the input files
 * we'll translate stdin.
 *
 */


    if ( argc < 1 )
	text();
    else {				/* at least one argument is left */
	while ( argc > 0 )  {
	    if ( strcmp(*argv, "-") == 0 )
		fp_in = stdin;
	    else if ( (fp_in = fopen(*argv, "r")) == NULL )
		error(FATAL, "can't open %s", (unsigned)*argv, dummy, dummy);
	    text();
	    if ( fp_in != stdin )
		(void)fclose(fp_in);
	    argc--;
	    argv++;
	}   /* End while */
    }   /* End else */

}   /* End of arguments */


/*****************************************************************************/


void done(void)


{


/*
 *
 * Finished with all the input files, so mark the end of the pages with a TRAILER
 * comment, make sure the last page prints, and add things like the PAGES comment
 * that can only be determined after all the input files have been read.
 *
 */


    (void)fprintf(stdout, "%s", TRAILER);
    (void)fprintf(stdout, "done\n");
    (void)fprintf(stdout, "%s ", DOCUMENTFONTS);
    if ((csname[0] != NULL) && cuse0)
	(void)fprintf(stdout, "%s ", dwlname[0]);
    if ((csname[1] != NULL) && cuse1)
	(void)fprintf(stdout, "%s ", dwlname[1]); 
    if (csname[2] != NULL)
	(void)fprintf(stdout, "%s ", dwlname[2]);
    if (csname[3] != NULL)
	(void)fprintf(stdout, "%s ", dwlname[3]);
    if ( (cuse0 != cuse1) || (!cuse0 && !cuse1))
	(void)fprintf(stdout, "%s ", fontname); 
    (void)fprintf(stdout, "\n%s %d\n", PAGES, printed);
}   /* End of done */


/*****************************************************************************/


void account(void)


{


/*
 *
 * Writes an accounting record to *fp_acct provided it's not NULL. Accounting is
 * requested using the -A or -J options.
 *
 */


    if ( fp_acct != NULL )
	(void)fprintf(fp_acct, " print %d\n copies %d\n", printed, copies);

}   /* End of account */


/*****************************************************************************/


void text(void)


{


    int		ch;			/* next input character */

/*
 *
 * Translates *fp_in into PostScript. All we do here is handle newlines, tabs,
 * backspaces, and quoting of special characters. All other unprintable characters
 * are totally ignored. The redirect(-1) call forces the initial output to go to
 * /dev/null. It's done to force the stuff that formfeed() does at the end of
 * each page to /dev/null rather than the real output file.
 *
 */


    redirect(-1);			/* get ready for the first page */
    formfeed();				/* force PAGE comment etc. */

    ch=getc(fp_in);
     
	switch ( ch )  {

	    case '\n':
		    newline();
		    break;

	    case '\t':
	    case '\b':
	    case ' ':
	 	    switch_font(CS0); 	
		    spaces(ch);
		    break;

	    case '\014':
		    formfeed();
		    break;

	    case '\r':
		    if ( crmode == 1 ) {
			switch_font(CS0);	
			spaces(ch);
		    }
		    else if ( crmode == 2 )
			newline();
		    break;


/*
 *
 * Fall through to the default case.
 *
 */

	    default:
		   if ( ch & 0x80 ) {   /* 8-bits characters */
		      if ( csname[2] != NULL || csname[3] != NULL ) {
			  if ( ch == SS2 ) {
				ch = getc(fp_in);
				switch_font(CS2);
				sput(ch);
 		          }
			  else
          	     	  if ( ch == SS3 ) {
				ch = getc(fp_in);
				switch_font(CS3);
				sput(ch);
			  }
	 	      }
		      else { /* did user specify font or encoding for 8-bits? */
		      if (cuse1 ||cuse0 || euse0 || euse1 ) 
			  switch_font(CS1);
		      else
		      if ( euse )
			  switch_font(CS0);
		      sput(ch);
		      }
	 	   }
		   else {
	              if (isascii (ch) && isprint (ch) ) {
			        switch_font(CS0);
		      if((ch=='(')||(ch==')')||(ch=='\\')) {
		   	   startline();
		      	(void)putc('\\', fp_out);
		      }
			      	oput(ch);	
		      }
		   }


	}   /* End switch */


    while ( ( ch = getc(fp_in)) != EOF )

	switch ( ch )  {

	    case '\n':
		    newline();
		    break;

	    case '\t':
	    case '\b':
	    case ' ':
		    switch_font(CS0);
		    spaces(ch);
		    break;

	    case '\014':
		    formfeed();
		    break;

	    case '\r':
		    if ( crmode == 1 ) {
			switch_font(CS0);
			spaces(ch);
		    }
		    else if ( crmode == 2 )
			newline();
		    break;


/*
 *
 * Fall through to the default case.
 *
 */

	    default:
		   if ( ch & 0x80 ) {   /* 8-bits characters */
		      if ( csname[2] != NULL || csname[3] != NULL ) {
			  if ( ch == SS2 ) {
				ch = getc(fp_in);
				switch_font(CS2);
				sput(ch);
 		          }
			  else
          	     	  if ( ch == SS3 ) {
				ch = getc(fp_in);
				switch_font(CS3);
				sput(ch);
			  }
	 	      }
		      else { /* did user specify font or encoding for 8-bits? */
		      if (cuse1 ||cuse0 || euse0 || euse1 ) 
			  switch_font(CS1);
		      else
		      if ( euse )
			  switch_font(CS0);
		      sput(ch);
		      }
	 	   }
		   else {
	              if (isascii (ch) && isprint (ch) ) {
			switch_font(CS0);
	    		if((ch=='(') || (ch==')') || (ch=='\\')) {
		        startline();
		        (void)putc('\\', fp_out);
			}
			      	oput(ch);	
		      }
		   }


	}   /* End switch */

    formfeed();				/* next file starts on a new page? */

}   /* End of text */


/*****************************************************************************/


void formfeed(void)


{


/*
 *
 * Called whenever we've finished with the last page and want to get ready for the
 * next one. Also used at the beginning and end of each input file, so we have to
 * be careful about what's done. The first time through (up to the redirect() call)
 * output goes to /dev/null.
 *
 * Adobe now recommends that the showpage operator occur after the page level
 * restore so it can be easily redefined to have side-effects in the printer's VM.
 * Although it seems reasonable I haven't implemented it, because it makes other
 * things, like selectively setting manual feed or choosing an alternate paper
 * tray, clumsy - at least on a per page basis. 
 *
 */


    if ( fp_out == stdout )		/* count the last page */
	printed++;

    endline();				/* print the last line */

    (void)fprintf(fp_out, "cleartomark\n");
    (void)fprintf(fp_out, "showpage\n");
    (void)fprintf(fp_out, "restore\n");
    (void)fprintf(fp_out, "%s %d %d\n", ENDPAGE, page, printed);

    if ( ungetc(getc(fp_in), fp_in) == EOF )
	redirect(-1);
    else redirect(++page);

    (void)fprintf(fp_out, "%s %d %d\n", PAGE, page, printed+1);
    (void)fprintf(fp_out, "save\n");
    (void)fprintf(fp_out, "mark\n");
    writerequest(printed+1, fp_out);
    (void)fprintf(fp_out, "%d pagesetup\n", printed+1);

    if ( page>1 ) {
    (void)fprintf(fp_out, "/%s findfont\n", dwlname[0]);
    (void)fprintf(fp_out, "dup length dict begin\n");
    (void)fprintf(fp_out, "{1 index /FID ne {def} {pop pop} ifelse} forall\n");
    (void)fprintf(fp_out, "/Encoding %s def\n", encode[0]);
    (void)fprintf(fp_out, "currentdict\n");
    (void)fprintf(fp_out, "end\n");
    (void)fprintf(fp_out, "/%s exch definefont pop\n",csname[0]);

    if ( cuse1 ) {
    (void)fprintf(fp_out, "/%s findfont\n", dwlname[1]);
    (void)fprintf(fp_out, "dup length dict begin\n");
    (void)fprintf(fp_out, "{1 index /FID ne {def} {pop pop} ifelse} forall\n");
    (void)fprintf(fp_out, "/Encoding %s def\n", encode[1]);
    (void)fprintf(fp_out, "currentdict\n");
    (void)fprintf(fp_out, "end\n");
    (void)fprintf(fp_out, "/%s exch definefont pop\n",csname[1]);
    }

    if (encode[2]!=NULL) {
    (void)fprintf(fp_out, "/%s findfont\n", dwlname[2]);
    (void)fprintf(fp_out, "dup length dict begin\n");
    (void)fprintf(fp_out, "{1 index /FID ne {def} {pop pop} ifelse} forall\n");
    (void)fprintf(fp_out, "/Encoding %s def\n", encode[2]);
    (void)fprintf(fp_out, "currentdict\n");
    (void)fprintf(fp_out, "end\n");
    (void)fprintf(fp_out, "/%s exch definefont pop\n",csname[2]);
    }
     if (encode[3]!=NULL) {
    (void)fprintf(fp_out, "/%s findfont\n", dwlname[3]);
    (void)fprintf(fp_out, "dup length dict begin\n");
    (void)fprintf(fp_out, "{1 index /FID ne {def} {pop pop} ifelse} forall\n");
    (void)fprintf(fp_out, "/Encoding %s def\n", encode[3]);
    (void)fprintf(fp_out, "currentdict\n");
    (void)fprintf(fp_out, "end\n");
    (void)fprintf(fp_out, "/%s exch definefont pop\n",csname[3]);
    }
    (void)fprintf(fp_out, "/%s SwitchFont\n", csname[font_flag]);
    }
    
    line = 1;

}   /* End of formfeed */


/*****************************************************************************/


void newline(void)


{


/*
 *
 * Called when we've read a newline character. The call to startline() ensures
 * that at least an empty string is on the stack.
 *
 */


    startline();
    endline();				/* print the current line */
    nl = YES;

    if ( ++line > linespp )		/* done with this page */
	formfeed();

}   /* End of newline */


/*****************************************************************************/


void spaces(ch)


    int		ch;			/* next input character */


{


    int		endcol;			/* ending column */
    int		i;			/* final distance - in spaces */


/*
 *
 * Counts consecutive spaces, tabs, and backspaces and figures out where the next
 * string should start. Once that's been done we try to choose an efficient way
 * to output the required number of spaces. The choice is between using procedure
 * l with a single string on the stack and L with several string and column pairs.
 * We usually break even, in terms of the size of the output file, if we need four
 * consecutive spaces. More means using L decreases the size of the file. For now
 * if there are less than 6 consecutive spaces we just add them to the current
 * string, otherwise we end that string, follow it by its starting position, and
 * begin a new one that starts at endcol. Backspacing is always handled this way.
 *
 */


    startline();			/* so col makes sense */
    endcol = col;

    do {
	if ( ch == ' ' )
	    endcol++;
	else if ( ch == '\t' )
	    endcol += tabstops - ((endcol - 1) % tabstops);
	else if ( ch == '\b' )
	    endcol--;
	else if ( ch == '\r' )
	    endcol = 1;
	else break;
    } while ( ch = getc(fp_in) );	/* if ch is 0 we'd quit anyway */

    (void)ungetc(ch, fp_in);			/* wasn't a space, tab, or backspace */

    if ( endcol < 1 )			/* can't move past left edge */
	endcol = 1;

    if ( (i = endcol - col) >= 0 && i < 6 )
	for ( ; i > 0; i-- ) {
		(void)putc(' ',fp_out);
		col++;
	} 
    else {
	(void)fprintf(fp_out, ")%d(", stringstart-1);
	stringcount++;
	col = stringstart = endcol;
    }	/* End else */

}   /* End of spaces */


/*****************************************************************************/


void startline(void)


{


/*
 *
 * Called whenever we want to be certain we're ready to start pushing characters
 * into an open string on the stack. If stringcount is positive we've already
 * started, so there's nothing to do. The first string starts in column 1.
 *
 */


    if ( stringcount < 1 )  {
	(void)putc('(', fp_out);
	stringstart = col = 1;
	stringcount = 1;
	nl = NO ;
    }	/* End if */

}   /* End of startline */



/*****************************************************************************/

void endline(void)


{


/*
 *
 * Generates a call to the PostScript procedure that processes all the text on
 * the stack - provided stringcount is positive. If one string is on the stack
 * the fast procedure (ie. l) is used to print the line, otherwise the slower
 * one that processes string and column pairs is used.
 *
 */


    if ( stringcount == 1 ) 
	(void)fprintf(fp_out, ")l\n");
    else if ( stringcount > 1 )
	(void)fprintf(fp_out, ")%d L\n", stringstart-1);

    stringcount = 0;
    
    nl = YES;

}   /* End of endline */


/*****************************************************************************/

void oput(ch)


    int		ch;			/* next output character */


{


/*
 *
 * Responsible for adding all printing characters from the input file to the
 * open string on top of the stack. The only other characters that end up in
 * that string are the quotes required for special characters. Some simple
 * changes here and in spaces could make line wrapping possible. Doing a good
 * job would probably force lots of printer dependent stuff into the program,
 * so I haven't bothered with it. Could also change the prologue, or perhaps
 * write a different one, that uses kshow instead of show to display strings.
 *
 */

    startline();
    (void)putc(ch, fp_out);
    col++;

}   /* End of oput */


/*****************************************************************************/

void sput(ch)


    int		ch;			/* next output character */


{


/*
 *
 * If user didn't specify font for 8-bits characters we will use
 * default font and will write octal ASCII value to PoscScript output.
 * Anyway, we must print 8-bits characters using this way for 
 * compatibility with older PostScript interpreters.
 *
 */

    startline();
    (void)fprintf(fp_out, "\\%o", ch);
    col++;

}   /* End of oput */


/*****************************************************************************/

void redirect(pg)


    int		pg;			/* next page we're printing */


{


    static FILE	*fp_null = NULL;	/* if output is turned off */


/*
 *
 * If we're not supposed to print page pg, fp_out will be directed to /dev/null,
 * otherwise output goes to stdout.
 *
 */


    if ( pg >= 0 && in_olist(pg) == ON )
	fp_out = stdout;
    else if ( (fp_out = fp_null) == NULL )
	fp_out = fp_null = fopen("/dev/null", "w");

}   /* End of redirect */


/*****************************************************************************/

void switch_font( n )


   int	      n;		/* flag for font we will switch to */
/*
 * This function switches to font which is used
 * for actual character. It immediately ends output, checking
 * if there is any character in current line. Then changes font and
 * opens output. Space and string handling is not modified, because
 * user can change font anywhere in sentence or string, so these should
 * be kept without any change.
 * 
 */

{
   char	     *temp; 

    if ((page==1) && (!created[n])) {
         font_flag = n;
	 encode_flag=n;
         if (nl==NO) 
		 (void)fprintf(fp_out, ")w\n");
         (void)fprintf(fp_out, "/%s findfont\n",csname[n]);
         (void)fprintf(fp_out, "dup length dict begin\n");
         (void)fprintf(fp_out, "{1 index /FID ne {def} {pop pop} ifelse} forall\n");
         (void)fprintf(fp_out, "/Encoding %s def\n", encode[n]);
         (void)fprintf(fp_out, "currentdict\n");
         (void)fprintf(fp_out, "end\n");
         if ((temp = malloc(strlen(csname[n])+strlen(encode[n])+5))==NULL)
	 {
		error(FATAL, "Can't allocate memory", dummy, dummy, 					dummy);
	 }
	 (void)sprintf(temp, "%s%s", csname[n], encode[n]);
	 if ((csname[n]=malloc(strlen(temp)+2))==NULL) {
		error(FATAL, "Can't allocate memory", dummy, dummy, 					dummy);
	 }
	 (void)strcpy(csname[n], temp);
         (void)fprintf(fp_out, "/%s exch definefont pop\n",csname[n]);
	 (void)fprintf(fp_out,"/%s SwitchFont\n",csname[n]);
	 created[n] = YES;
         if (nl==NO) 
		 (void)fprintf(fp_out, "(");
      }

    if ((font_flag != n ) || (encode_flag != n)) {
	font_flag = n;
 	encode_flag=n;
 	if ( ! created[n] ) {
		if (nl==NO)
	                (void)fprintf(fp_out, ")w\n");
                (void)fprintf(fp_out, "/%s findfont\n",csname[n]);
                (void)fprintf(fp_out, "dup length dict begin\n");
                (void)fprintf(fp_out, "{1 index /FID ne {def} {pop pop} ifelse} forall\n");
                (void)fprintf(fp_out, "/Encoding %s def\n", encode[n]);
                (void)fprintf(fp_out, "currentdict\n");
                (void)fprintf(fp_out, "end\n");
		if ((temp = malloc(strlen(csname[n])+strlen(encode[n])+5))==NULL)
 		{
			error(FATAL, "Can't allocate memory", dummy, dummy, dummy);

		}
		(void)sprintf(temp, "%s%s", csname[n], encode[n]);
		if ((csname[n]=malloc(strlen(temp)+2))==NULL) {
			error(FATAL, "Can't allocate memory", dummy, dummy,
                                dummy);
		}
		(void)strcpy(csname[n], temp);
                (void)fprintf(fp_out, "/%s exch definefont pop\n",csname[n]);
		(void)fprintf(fp_out,"/%s SwitchFont\n",csname[n]);
		if (nl==NO)
	                (void)fprintf(fp_out, "(");
		created[n] = YES;
	}
	else {
		if (nl==NO)
	                (void)fprintf(fp_out, ")w\n");
		(void)fprintf(fp_out,"/%s SwitchFont \n",csname[n]);
		if (nl==NO)
	                (void)fprintf(fp_out, "(");
		}
     }
}

/*****************************************************************************/



void read_aliases(void)


/* This function reads aliases from separate file.
   File contains encoding vectors aliases.
 */

{

FILE	*inf;
int	index, ch, old_optind; 
int	amount=0;	/* for memory allocating */
char    buf[MAX_LINE];	/* temporary buffer */
char	s1[MAX_STR];	/* alias name */
char	s2[MAX_STR];	/* real name */


  old_optind = optind;
  while ( (ch = getopt(argc, argv, optnames)) != EOF) 
	if (ch == 'E')
	   codealias  = optarg;
  optind = old_optind;
 

  if ((inf=fopen(codealias,"r")) == NULL) 
	error(FATAL, "Can't open encoding aliases file.\n", dummy,dummy
	      ,dummy);
  else {
	while ( fgets(buf, MAX_LINE, inf) != NULL) {
	   index = strlen(buf);
	   if ( buf[index-1] == '\n') {
		buf[--index] = 0;
		if (buf[0] == '#') continue;
		if (count >= amount) {
		   if (amount==0) {
		      if((encodemap=(Encodemap*)malloc(sizeof(Encodemap)\
			*MAX_LINE))==NULL) {
			error(FATAL, "Can't allocate memory",dummy,dummy
			     ,dummy);
		      }
		      amount=MAX_LINE;
		   }
		   else {
		      amount*=2;
		      encodemap = (Encodemap*)realloc(encodemap, \
				  sizeof(Encodemap)*MAX_LINE);
		   }
		}
		if (sscanf(buf, "%s %s",s1,s2) != 2) {
		   index = 0;
		   while ( buf[index++] == ' ');
		   if (buf[index])
		   error(NON_FATAL," Check the format of encode alias file, error in format.\n",dummy, dummy, dummy);
		   continue;
		}

		if((encodemap[count].name = malloc(strlen(s1)+2))==NULL) {
		   error(FATAL, "Can't allocate memory", dummy, dummy,
			dummy);
		}
		(void)strcpy(encodemap[count].name, s1);

		if((encodemap[count].val = malloc(strlen(s2)+2))==NULL) {
		  error(FATAL, "Can't allocate memory", dummy,dummy,
		       dummy);
		}
		(void)strcpy(encodemap[count].val, s2);	
	   
	   }
	   else {
		error(FATAL, " Wrong encoding vectors aliases file format.\n", dummy, dummy, dummy);
	   }
	   count++;
	}	   
	(void)fclose(inf);

  }
}	
