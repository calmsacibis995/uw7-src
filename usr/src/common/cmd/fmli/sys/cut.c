/*		copyright	"%c%" 	*/

#ident	"@(#)fmli:sys/cut.c	1.9.3.4"

#include <stdio.h>	/* make: cc cut.c */
#include <string.h>	
#include <ctype.h>
#include "wish.h"
#include "ctl.h"
#include "eval.h"
#include "moremacros.h"
#include "message.h"
#include "sizes.h"
#include <unistd.h>

/* cut : cut and paste columns of a table (projection of a relation) */
/* Release 1.5; handles single backspaces as produced by nroff    */

# define NFIELDS 1024	/* max no of fields or resulting line length */
# define BACKSPACE '\b'

int strcmp(), atoi();
void exit();
char *getastr();	/* rjk */

char i18n_label[] = "Usage: ";
char i18n_labelid[] = ":211";

static char usage[] = "fmlcut [-s] [-d<char>] {-c<list> | -f<list>} file ...";
static char usageid[] = ":212";
static char cflist[] = "bad list for c/f option";
static char cflistid[] = ":213";

cmd_cut(argc, argv, instr, outstr, errstr)
int	argc;
char	*argv[];
IOSTRUCT	*instr;
IOSTRUCT	*outstr;
IOSTRUCT	*errstr;
{
    extern int 	opterr, optopt;	/* abs s14 */
    extern int 	optind;
    extern char	*optarg;
    register int	c;
    register char	*p1, *rbuf;
    register char	*p, *list;
    register int	i;
    int	del = '\t';
    int	num, j, count, poscnt, r, s;
    int	endflag, supflag, cflag, fflag, backflag, filenr;
    int	sel[NFIELDS];
    char  	buf[BUFSIZ];	
    char	*p2, outbuf[NFIELDS];
    FILE	*inptr;
    int	fromfile;		/* rjk */
    
    char i18n_msg[BUFSIZ]; 

    supflag = cflag = fflag = r = num = s = 0;
    for (i = 0; i < NFIELDS; i++)
	sel[i] = 0;

    opterr = 0;			/* abs s14 */
    optind = 1;
    optarg = NULL;
    while((c = getopt(argc, argv, "c:d:f:s")) != EOF)
	switch(c) {
	case 'c':
	    if (fflag)
		return( diag(errstr, "", gettxt(cflistid,cflist) ) );
	    cflag++;
	    list = optarg;
	    break;
	case 'd':
	    if ((int)strlen(optarg) > 1)
		diag(errstr, "", gettxt(":214","no delimiter") );
	    else
		del = (int)*optarg;
	    break;
	case 'f':
	    if (cflag)
		return(diag(errstr, gettxt(i18n_labelid,i18n_label), gettxt(usageid,usage) ));
	    fflag++;
	    list = optarg;
	    break;
	case 's':
	    supflag++;
	    break;
	case '?':
	    return(diag(errstr, gettxt(i18n_labelid,i18n_label), gettxt(usageid,usage) ));
	}

    argv = &argv[optind];
    argc -= optind;

    if (!(cflag || fflag))
	return(diag(errstr, "", gettxt(cflistid,cflist) ) );

    do {
	p = list;
	switch(*p) {
	case '-':
	    if (r)
		return(diag(errstr, "", gettxt(cflistid,cflist) ) );
	    r = 1;
	    if (num == 0)
		s = 1;
	    else {
		s = num;
		num = 0;
	    }
	    break;
	case '\0' :
	case ','  :
	    if (num >= NFIELDS)
		return(diag(errstr, "", gettxt(cflistid,cflist) ) );
	    if (r) {
		if (num == 0)
		    num = NFIELDS - 1;
		if (num < s)
		    return(diag(errstr, "", gettxt(cflistid,cflist) ) );
		for (j = s; j <= num; j++)
		    sel[j] = 1;
	    } else
		sel[num] = (num > 0 ? 1 : 0);
	    s = num = r = 0;
	    if (*p == '\0')
		continue;
	    break;
	default:
	    if (!isdigit(*p))
		return(diag(errstr, "", gettxt(cflistid,cflist) ) );
	    num = atoi(p);
	    while (isdigit(*list))
		list++;
	    continue;
	}
	list++;
    }while (*p != '\0');
    for (j=0; j < NFIELDS && !sel[j]; j++);
    if (j >= NFIELDS)
	return(diag(errstr, "", gettxt(":215","no fields") ) );

    filenr = 0;
    do {			/* for all input files */
	if ( argc == 0 || strcmp(argv[filenr],"-") == 0 )
	    fromfile = 0;
	else {
	    /* rjk */
	    if ((inptr = fopen(argv[filenr], "r")) == NULL) {
		char errbuf[PATHSIZ + 50];
                strcpy(i18n_msg, "fmlcut: ");
                strcat(i18n_msg, gettxt(":216","WARNING: cannot open %s\n") );
		sprintf(errbuf, i18n_msg, argv[filenr]);
		putastr(errbuf, errstr);		/* abs s15 */
		continue;
	    }
	    else
		fromfile++;
	}
	endflag = 0;
	do {			/* for all lines of a file */
	    count = poscnt = backflag = 0;
	    p1 = &outbuf[0] - 1 ;
	    p2 = p1;
	    rbuf = buf;
	    /* rjk ... from a file or form Instr) */
	    if (fromfile ? ((fgets(buf, BUFSIZ, inptr)) == NULL) :
		(getastr(buf, BUFSIZ, instr) == NULL || buf[0] == '\0')) {
		endflag = 1;
		continue;
	    }
	    do {		/* for all char of the line */
		if (rbuf >= &buf[NFIELDS])
		    return(diag(errstr, "", gettxt(":217","line too long") ) );
		if (*rbuf != '\n')
		    *++p1 = *rbuf;
		if (cflag && (*rbuf == BACKSPACE))
		    backflag++;
		else if (!backflag)
		    poscnt += 1;
		else
		    backflag--;
		if ( backflag > 1 )
		    return(diag(errstr, "", gettxt(":218","cannot handle multiple adjacent backspaces\n") ) );
		if (*rbuf == '\n' && count > 0  || *rbuf == del || cflag) {
		    count += 1;
		    if (fflag)
			poscnt = count;
		    if (sel[poscnt])
			p2 = p1;
		    else
			p1 = p2;
		}
	    } while (*rbuf++ != '\n');
	    if ( !endflag && (count > 0 || !supflag)) {
		if (*p1 == del && !sel[count])
		    *p1 = '\0'; /*suppress trailing delimiter*/
		else
		    *++p1 = '\0';
		putastr(outbuf, outstr); /* rjk */
		putac('\n', outstr); /* rjk */
	    }
	} while (!endflag);
	if (fromfile)
	    fclose(inptr);
    } while (++filenr < argc);
    return(SUCCESS);		/* rjk */
}


diag(errstr, i18n_label, s)
IOSTRUCT *errstr;			/* abs s14 */
char     *i18n_label;
char	 *s;
{
    putastr("fmlcut: ", errstr); 	/* abs s14 */
    putastr(i18n_label, errstr);	/* abs s14 */
    putastr(s,errstr); 
	/*********** UNIX cut's approach: 
	fprintf(stderr, "fmlcut: ERROR: %s\n", s);
	exit(2);
	************/
        /***********			   abs s14
	mess_temp(s);
	mess_lock();
	***********/

    return(FAIL);
}
