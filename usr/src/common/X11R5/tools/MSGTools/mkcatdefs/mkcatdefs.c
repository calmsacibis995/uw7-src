#pragma ident	"@(#)mmisc:tools/MSGTools/mkcatdefs/mkcatdefs.c	1.2"
static char sccsid[] = "@(#)68	1.19  com/cmd/msg/mkcatdefs.c, bos, bos320 5/29/91 10:23:42";
/*
 * COMPONENT_NAME: (CMDMSG) Message Catalogue Facilities
 *
 * FUNCTIONS: main, mkcatdefs, incl, chkcontin 
 *
 * ORIGINS: 27, 18
 *
 * IBM CONFIDENTIAL -- (IBM Confidential Restricted when
 * combined with the aggregated modules for this product)
 * OBJECT CODE ONLY SOURCE MATERIALS
 * (C) COPYRIGHT International Business Machines Corp. 1988, 1989, 1991
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */
/*
 * @OSF_COPYRIGHT@
 */
/*static char rcsid[] = "RCSfile: mkcatdefs.c,v Revision: 1.4  (OSF) Date: 90/10/07 16:45:30 ";*/

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <ctype.h>
#include <ctype.h>

#if ( defined(SVR4) || defined(sun) )
#include <wctype.h>
#endif

#ifdef aix
#include <sys/dir.h>
#endif

#ifdef hpux
#ifndef _XPG2
#define _XPG2
#endif
#endif

#include <limits.h>
#include <nl_types.h>
#include <string.h>
#include "msgfac_msg.h"
#define	 MSGSTR(N,S)	catgets(errcatd,MS_MKCATDEFS,N,S)

#define MAXLINELEN NL_TEXTMAX
#define KEY_START '$'
#define MAXIDLEN 64
#ifdef _D_NAME_MAX
#define MDIRSIZ _D_NAME_MAX
#else
#define MDIRSIZ 14
#endif

#ifdef aix
#ifndef iswblank
#define iswblank(wc)      is_wctype(wc, get_wctype("blank")) /* not defined by X/Open */
#endif
#endif

#ifdef hpux
#ifndef iswblank
#define iswblank(wc)      iswctype(wc, wctype("blank")) 
#endif 
#endif

#if ( defined(sun) || defined(SVR4) ) 
#ifndef iswblank
#define iswblank(wc) iswspace(wc)
#endif
#endif
/*
 * EXTERNAL PROCEDURES CALLED: descopen, descclose, descset, descgets,
 *                             descerrck, insert, nsearch
 */

char *descgets();
nl_catd errcatd;
static int errflg = 0;
static int setno = 1;
static int msgno = 1;
static int symbflg = 0;
static int inclfile = 1;
FILE *outfp;
FILE *msgfp;
static char inname [PATH_MAX];
static char outname [PATH_MAX];
static char catname [PATH_MAX];
char *mname;
#ifdef _NO_PROTO
static void mkcatdefs();
static void incl();
static chkcontin();
#else
static void mkcatdefs(char *);
static void incl(char *, char *, char *);
static int chkcontin(char *);
#endif /* _NO_PROTO */


/*
 * NAME: main
 *
 * FUNCTION: Make message catalog defines.
 *
 * EXECUTION ENVIRONMENT:
 *  	User mode.
 *
 * NOTES:  Invoked by:
 *         mkcatdefs <name> <msg_file>
 *
 *  	Results are 1) Creates header file <name>.h.
 *                  2) Displays message file to stdout. The message file is 
 *                     ready to be used as input to gencat.
 *
 *   	mkcatdefs takes a message definition file and produces
 *  	a header file containing #defines for the message catalog,
 * 	the message sets and the messages themselves.  It also
 *  	produces a new message file which has the symbolic message set and
 *  	message identifiers replaced by their numeric values (in the form
 *  	required by gencat).
 *
 * DATA STRUCTURES: Effects on global data structures -- none.
 *
 * RETURNS: 1 - error condition
 */

main (int argc, char *argv[]) 

{
    register int i;
    register char *cp;
    int count;
    char *t;
    wchar_t wc;		/* filename process code */

    setlocale (LC_ALL,"");
    errcatd = catopen(MF_MSGFAC,0);

    /* usage: handle multiple files; -h option has to be at the end */
    if (argc < 3) {
	fprintf (stderr, MSGSTR(MKCATUSAGE,"mkcatdefs: Usage: %s catname msg_file [msg_file...] [-h]\n"), argv [0]);	
	exit (0);
    }

    /* check if  include file should be created; -h is the last argument */
    if (argv[argc-1][0] == '-' && argv[argc-1][1] == 'h') 
		inclfile = 0;

    /* open header output file */
    if (inclfile) {
	mname = argv [1];
	if ((strlen((t = strrchr(mname,'/')) ? t + 1 : mname) +
             sizeof("_msg.h") - 1) > MDIRSIZ) {
		fprintf (stderr, MSGSTR(MNAMTOOLONG, "mkcatdefs: catname too long\n"));
		exit (1);
	}
    	sprintf (outname, "%s_msg.h", mname);
	if (strrchr(mname,'/'))
	    mname = strrchr(mname,'/') + 1;
        sprintf (catname, "%s.cat", mname);
    	if ((outfp = fopen (outname, "w")) == NULL) {
		fprintf (stderr, MSGSTR(MKCATOPN, "mkcatdefs: Cannot open %s\n"), outname);								/*MSG*/
		exit (1);
	} else  {
    		/* convert name to upper case */
    		for (cp=mname; *cp; cp+=i) {
			i = mbtowc(&wc, cp, MB_CUR_MAX);
			if (i < 0) {
				fprintf (stderr, MSGSTR(IMBCHD, "mkcatdefs: catname contains invalid character\n"));
				exit (1);
			}
			if (iswlower(wc) != 0)
				wc = towupper(wc);
			else if (!iswupper(wc) && !iswdigit(wc))
				wc = '_';
			wctomb(cp, wc);
		}
                incl ("#ifndef _H_%s_MSG \n", mname, "");
                incl ("#define _H_%s_MSG \n", mname, "");
    		incl ("#include <limits.h>\n", "", "");
    		incl ("#include <nl_types.h>\n", "", "");
    		incl ("#define MF_%s \"%s\"\n\n", mname, catname);
	}
    } else sprintf (outname, "msg.h");


    /* open new msg output file */
    msgfp = stdout;

/* if message descriptor files were specified then process each one in turn */
         
    if (inclfile == 0 )
        count = argc - 1;
    else
        count = argc;
    for (i = 2; i < count; i++) {
    /* open input file */
    	sprintf (inname, "%s", argv[i]);
	if (strcmp(inname,"-") == 0) {
		strcpy(inname,"stdin");
		descset(stdin);       /* input from stdin if no source files */
		mkcatdefs(inname);
	} else	{
		if (descopen(inname) < 0) {
			fprintf (stderr, MSGSTR(MKCATOPN,"mkcatdefs: Cannot open %s\n"), inname);							/*MSG*/
			errflg = 1;
		} else  {
			mkcatdefs (inname);
			descclose();
		}
	}
    }
    incl ("#endif \n", "", "");

    if (inclfile) {
    	fflush (outfp);
    	if (ferror (outfp)) {
		fprintf (stderr, MSGSTR(WRITERRS,"mkcatdefs: There were write errors on file %s\n"), outname);						/*MSG*/
		errflg = 1;
	}
    	fclose (outfp);
    }

    if (errflg) {
	fprintf (stderr, MSGSTR(ERRFND,"mkcatdefs: Errors found: no %s created\n"), outname);								/*MSG*/
	if (inclfile)  unlink(outname);
    } else {
	   if (inclfile) {
		if (symbflg)
			fprintf (stderr, MSGSTR(HCREAT,"mkcatdefs: %s created\n"), outname);
	   	else {
			fprintf (stderr, MSGSTR(NOSYMB,"mkcatdefs: No symbolic identifiers; no %s created\n"), outname);				/*MSG*/
			unlink (outname);
		}
   	   } 
	   else 
                fprintf(stderr,MSGSTR(NOHDR,"mkcatdefs: no %s created\n"), outname);                                      				/*MSG*/
    }
    exit (errflg);
}

/*
 * NAME: mkcatdefs
 *
 * FUNCTION: Make message catalog definitions.
 *
 * EXECUTION ENVIRONMENT:
 *  	User mode.
 *
 * RETURNS: None
 */
static void
#ifdef _NO_PROTO
mkcatdefs(fname)
char *fname;
#else
mkcatdefs(char *fname)
#endif /* _NO_PROTO */
	/*---- fname: message descriptor file name ----*/

{
    char msgname [PATH_MAX];
    char line [MAXLINELEN];
    register char *cp;
    register char *cpt;
    register int m;
    register int n;
    int contin = 0;
    int len;		/* # bytes in a character */
    wchar_t wc;		/* process code */


    /* put out header for include file */
    incl ("\n\n/* The following was generated from %s. */\n\n",fname, "");

    /* process the message file */
    while (descgets (line, MAXLINELEN) ) {
	/* find first nonblank character */
	for (cp=line; *cp; cp+=len) {
		len = mbtowc(&wc, cp, MB_CUR_MAX);
		if (len < 0) {
			fprintf (stderr, MSGSTR(IMBCTX, "mkcatdefs: sourcefile contains invalid character:\n\t%s"),line);
			errflg = 1;
			return;
		}
		if (iswblank(wc) == 0)
			break;
	}
	    if (*cp == KEY_START) {
		cp++;
		for (cpt = cp; *cp; cp += len) {
			len = mbtowc(&wc, cp, MB_CUR_MAX);
			if (len < 0) {
				fprintf (stderr, MSGSTR(IMBCTX, "mkcatdefs: sourcefile contains invalid character:\n\t%s"),line);
				errflg = 1;
				return;
			}
			if (iswspace(wc) == 0)
				break;
		}
		if (cp != cpt) {
		    sscanf (cp, "%s", msgname);
		    if ((m = nsearch(msgname)) > 0) {
			fprintf (msgfp, "$ %d", m);
			cp += strlen(msgname);
			fprintf (msgfp, "%s", cp);
		    } else
		    	fputs (line, msgfp);
		    continue; /* line is a comment */
		}
		if ((strncmp (cp, "set", 3) == 0) && ((len = mbtowc(&wc, cp+3, MB_CUR_MAX)) > 0) && (iswspace(wc) != 0)) {
    		    char setname [MAXIDLEN];

		    sscanf (cp+3+len, "%s", setname);
		    incl ("\n/* definitions for set %s */\n", setname, "");
		    if (isdigit(setname[0])) {
			    cpt = setname;
			    do  {
				if (!isdigit(*cpt)) {
				   fprintf(stderr,MSGSTR(ZEROINV, "mkcatdefs: %s is an invalid identifier\n"), setname);
					errflg = 1;
					return;
				}
			    }   while (*++cpt);
			n = atoi (setname);
			if (n >= setno)
			    	setno = n;
		        else {
				if (n = 0)
				   fprintf(stderr,MSGSTR(ZEROINV, "mkcatdefs: %s is an invalid identifier\n"), setname);	
				else
				   fprintf(stderr,MSGSTR(INVLDSET, "mkcatdefs: set # %d already assigned or sets not in ascending sequence\n"), n);
				errflg = 1;
				return;
			}
		    } else  {
			    cpt = setname;
			    do  {
				len = mbtowc(&wc, cpt, MB_CUR_MAX);
				if (len <= 0) {
					fprintf (stderr, MSGSTR(IMBCTX, "mkcatdefs: sourcefile contains invalid character:\n\t%s"),line);
					errflg = 1;
					return;
				}
				if ((iswalnum(wc) == 0) && (wc != '_')) {
					fprintf(stderr,MSGSTR(ZEROINV, "mkcatdefs: %s is an invalid identifier\n"), setname);
					errflg = 1;
					return;
				}
			    }   while (*(cpt += len));
			incl ("#define %s %d\n\n", setname, (char *)setno);
		        symbflg = 1;
		    }
#ifdef aix
		    fprintf (msgfp,"$delset");
		    fprintf (msgfp," %d\n", setno);
#endif
		    fprintf (msgfp,"%.4s", line);
		    fprintf (msgfp," %d\n", setno++);
		    msgno = 1;
		    continue;
		} else {
		     /* !!!other command */
		}
	    } else
		if (contin) {
		    if (!chkcontin(line))
			contin = 0;
		} else if (setno > 1) { /* set must have been seen first */
    		    char msgname [MAXIDLEN];

		    msgname [0] = '\0';
		    if (sscanf (cp, "%s", msgname) && msgname[0]) {
			len = mbtowc(&wc, cp, MB_CUR_MAX);
			if (len < 0) {
				fprintf (stderr, MSGSTR(IMBCTX, "mkcatdefs: sourcefile contains invalid character:\n\t%s"),line);
				errflg = 1;
				return;
			}
			if (iswalpha(wc) != 0) {
			    cpt = msgname;
			    do  {
				len = mbtowc(&wc, cpt, MB_CUR_MAX);
				if (len < 0) {
					fprintf (stderr, MSGSTR(IMBCTX, "mkcatdefs: sourcefile contains invalid character:\n\t%s"),line);
					errflg = 1;
					return;
				}
				if ((iswalnum(wc) == 0) && (wc != '_')) {
					fprintf(stderr,MSGSTR(ZEROINV, "mkcatdefs: %s is an invalid identifier\n"), msgname);	
					errflg = 1;
					return;
				}
			    }   while (*(cpt += len));
			    cp += strlen(msgname);
			    fprintf (msgfp,"%d%s", msgno,cp);
			    incl ("#define %s %d\n", msgname, (char *)msgno);
			    symbflg = 1;
			    if (chkcontin(line))
				contin = 1;
			    if(insert(msgname,msgno++) < 0) {
				fprintf(stderr,MSGSTR(MULTOPN, "mkcatdefs: name %s used more than once\n"), msgname); 
				errflg = 1;
				return;
			    }
			    continue;
			} else if (isdigit (msgname[0])){
			    cpt = msgname;
			    do  {
				if (!isdigit(*cpt)) {
					fprintf(stderr,MSGSTR(INVTAG, "mkcatdefs: invalid syntax in %s\n"), line);
					errflg = 1;
					return;
				}
			    }   while (*++cpt);
			    n = atoi (msgname);
			    if ((n >= msgno) || (n == 0 && msgno == 1))
				msgno = n + 1;
			    else {
				 errflg = 1;
				 if (n == 0)
				    fprintf(stderr,MSGSTR(ZEROINV, "mkcatdefs: %s is an invalid identifier\n"), msgno);
				 else if (n == msgno) 
					fprintf(stderr,MSGSTR(MULTNR, "mkcatdefs: message id %s already assigned to identifier\n"), msgname);		/*MSG*/
				      else
					fprintf(stderr,MSGSTR(NOTASC, "mkcatdefs: source messages not in ascending sequence\n"));			/*MSG*/
				return;
			    }
			}
		    }
		    if (chkcontin(line))
			contin = 1;
		}
	fputs (line, msgfp);
    }

    /* make sure the operations read/write operations were successful */
    if (descerrck() == -1) {
	fprintf (stderr, MSGSTR(READERRS, "mkcatdefs: There were read errors on file %s\n"), inname);							/*MSG*/
	errflg = 1;
    }
    return;
}

/*
 * NAME: incl
 *
 * FUNCTION: Output strings to file.
 *
 * EXECUTION ENVIRONMENT:
 *  	User mode.
 *
 * RETURNS: None
 */
static void
#ifdef _NO_PROTO
incl(a, b, c)
char *a;
char *b;
char *c;
#else
incl(char *a, char *b, char *c) 
#endif /* _NO_PROTO */

	/*
	  a - pointer to "printf" format
	  b - pointer to optional "printf" arg
	  c - pointer to optional "printf" arg
	*/

{
	if (inclfile) fprintf (outfp, a, b, c);
}


/*
 * NAME: chkcontin
 *
 * FUNCTION: Check for continuation line.
 *
 * EXECUTION ENVIRONMENT:
 *  	User mode.
 *
 * RETURNS: 0 - not a continuation line.
 *          1 - continuation line.
 */
static int
#ifdef _NO_PROTO
chkcontin(line)
char *line;
#else
chkcontin(char *line) 
#endif /* _NO_PROTO */
{
	int	len;		/* # bytes in character */
	wchar_t	wc;		/* process code of current character in line */
	wchar_t	wcprev;		/* process code of previous character in line */

	for (wc=0; *line; line+=len) {
		wcprev = wc;
		len = mbtowc(&wc, line, MB_CUR_MAX);
		if (len < 0) {
			fprintf (stderr, MSGSTR(IMBCTX, "mkcatdefs: sourcefile contains invalid character:\n\t%s"),line);
			errflg = 1;
			return (0);
		}
	}
	if (wcprev == '\\' && wc == '\n')
		return (1);
	return (0);
}
