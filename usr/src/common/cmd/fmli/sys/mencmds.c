/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/mencmds.c	1.29.3.9"

#include	<stdio.h>
#include	<unistd.h>
#include	<ctype.h>
#include 	<string.h>
#include	"inc.types.h"	/* abs s14 */
#include	"typetab.h"
#include	"wish.h"
#include	"terror.h"
#include	"procdefs.h"
#include	"eval.h"
#include	"ctl.h"
#include	"moremacros.h"
#include	"message.h"
#include	"sizes.h"


/*
 * Globals used in eval() ...
 */
extern int EV_retcode;
extern int EV_backquotes;
extern int Lasttok;
extern int Vflag;

extern char *gettxt();

int
cmd_setodi(argc, argv, instr, outstr, errstr)
int argc;
char *argv[];
IOSTRUCT *instr;
IOSTRUCT *outstr;
IOSTRUCT *errstr;
{
    char *path;
    char *key;
    char *value;
    struct ott_entry *ott;
    struct ott_entry *path_to_ott();
  
    switch (argc) {
    case 1:
    case 2:
	putastr ("setodi: ", errstr);	
	putastr (gettxt(":269","not enough arguments"), errstr);
	return(FAIL);
    case 3:
	path = argv[1];
	key = argv[2];
	value = io_string(instr);
	break;
    default:
	putastr ("setodi: ", errstr);	
	putastr (gettxt(":270","extra args ignored"), errstr);
    case 4:
	path = argv[1];
	key = argv[2];
	value = strsave(argv[3]);
	break;
    }

    if ((ott = path_to_ott(path)) != NULL) {
	ott_lock_dsk(path);
	odi_putkey(ott, key, value);
	ott_dirty();
	ott_synch(FALSE);
    }
    if (value)
	free(value);
    return(SUCCESS);
}

int
cmd_getodi(argc, argv, instr, outstr, errstr)
int argc;
char *argv[];
IOSTRUCT *instr;
IOSTRUCT *outstr;
IOSTRUCT *errstr;
{
	char *path;
	char *key;
	char *value;
	struct ott_entry *ott;
	struct ott_entry *path_to_ott();
	char *odi_getkey();

        char *i18n_label;
        char *i18n_msg;
        char *i18n_labeled_msg;

	switch (argc) {
	case 1:
	case 2:
		putastr ("putodi: ", errstr);
		putastr (gettxt(":271","not enough args"), errstr);
		return(FAIL);
	default:
		putastr ("putodi: ", errstr);
		putastr (gettxt(":270","extra args ignored"), errstr);
	case 3:
		path = argv[1];
		key = argv[2];
		break;
	}

	if ((ott = path_to_ott(path)) == NULL)
		return(FAIL);
	if (value = odi_getkey(ott, key)) {
		putastr(value, outstr);
		return(SUCCESS);
	} else
		return(FAIL);
}

int
cmd_run(argc, argv, instr, outstr, errstr)
int argc;
char *argv[];
IOSTRUCT *instr;
IOSTRUCT *outstr;
IOSTRUCT *errstr;
{
    register int i,j,changed;
    int	procflags = 0;
    char *title = NULL;
    bool silent = FALSE;

    changed = 1;
    while (changed == 1) {
       if (argc == 1)
	   break;
       changed = 0;
       if ((argv[1][0] == 0) || (strlen(argv[1]) == strspn(argv[1]," ")))
       {
	   for (j=1; j-1 < argc; j++)
	      argv[j] = argv[j+1];
	   argc--;
	   changed = 1;
       }
    } /* While loop */
    if (argc == 1)
	return (SUCCESS);
    for (i = 1; argv[i][0] == '-'; i++) {
	switch (argv[i][1]) {
	case 's':
	    silent = TRUE;
	    break;
	case 'n':
	    procflags = PR_NOPROMPT;
	    break;
	case 'e':
	    procflags = PR_ERRPROMPT;
	    break;
	case 't':
	    title = &argv[i][2];
	    break;
	default:
	    break;
	}
    }

    if (silent)
	return waitspawn(spawnv(argv[i], argv + i));
    else
	return proc_openv(procflags, title, NULL, argv+i);
}

int
cmd_unset(argc, argv, instr, outstr, errstr)
int argc;
char *argv[];
IOSTRUCT *instr;
IOSTRUCT *outstr;
IOSTRUCT *errstr;
{
    register int	i;
    char	*var = NULL;
    char	*file = NULL;
    char	path[PATHSIZ];
    char	buf[BUFSIZ];
    char	whichenv;
    extern char	*Home;
    char	*strchr();
    char	*io_string();

    whichenv = '\0';
    for (i = 1; argv[i]; i++) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
	    case 'l':
		whichenv = argv[i][1];
		break;
	    case 'f':
		file = argv[i] + 2;
		break;
	    }
	}
	else {
	    var = argv[i];
	    if (whichenv == 'l')
		delAltenv(var);
	    else {
		if (!file) {
		    strcpy(path, Home ? Home : "");
		    strcat(path, "/pref/.environ");
		    file = path;
		}
		chgenv(file, var, NULL);
	    }
	    file = NULL;
	    whichenv = '\0';
	}
    }
    return SUCCESS;
}

int
cmd_set(argc, argv, instr, outstr, errstr)
int argc;
char *argv[];
IOSTRUCT *instr;
IOSTRUCT *outstr;
IOSTRUCT *errstr;
{
    register int	i;
    char	*val = NULL;
    char	*var = NULL;
    char	*file = NULL;
    char	path[PATHSIZ];
    char	buf[BUFSIZ];
    char	whichenv;
    char	*envbuf;
    extern char	*Home;
    char	*strchr();
    char	*io_string();
    int	dofree, amt, maxamt;

    whichenv = '\0';
    for (i = 1; argv[i]; i++) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
	    case 'l':
	    case 'e':
		whichenv = argv[i][1];
		break;
	    case 'f':
		file = argv[i] + 2;
		break;
	    }
	}
	else {
	    dofree = 0;
	    if (!(var = strchr(argv[i], '='))) {
		maxamt = BUFSIZ - (int)strlen(argv[i]) - 2;
		val = io_string(instr);
		if ((amt = strlen(val)) > maxamt) { 
		    /*
		     * Value is greater than 1K so malloc 
		     * enough space to hold it. 
		     */
		    maxamt = amt + (int)strlen(argv[i]) + 2;
		    if ((envbuf = (char *) malloc(maxamt)) == NULL)
			fatal(NOMEM, nil); 
		    dofree++;
		}
		else {
		    /*
		     * ... otherwise, use static 1K buffer
		     */
		    envbuf = buf;
		}
		strcpy(envbuf, argv[i]);
		strcat(envbuf, "=");
		strncat(envbuf, val, maxamt); 
		var = envbuf;
		free(val);
		val = &var[(int)strlen(var) - 1];
		if (*val == '\n')
		    *val = '\0';
	    }
	    else
		var = argv[i];
	    if (whichenv == 'l')
		putAltenv(var);
	    else if (whichenv == 'e')
		putenv(strsave(var));
	    else {
		if (!file) {
		    strcpy(path, Home ? Home : "");
		    strcat(path, "/pref/.environ");
		    file = path;
		}
		val = strchr(var, '=');
		*val++ = '\0';
		chgenv(file, var, val);
	    }
	    if (dofree) {
		free(envbuf);
		envbuf = NULL;
	    }
	    file = NULL;
	    whichenv = '\0';
	}
    }
    return SUCCESS;
}

/* types of messages (temporary, permanent, frame permanent) */
#define MTEMP	0x1
#define MPERM	0x2
#define MFRAME	0x4

int
cmd_message(argc, argv, instr, outstr, errstr)
int argc;
char *argv[];
IOSTRUCT *instr;
IOSTRUCT *outstr;
IOSTRUCT *errstr;
{
    char	msg[MESSIZ];
    register char	*amessage;
    register int	i;
    register int messtype;
    bool output = FALSE;
    bool work = FALSE;
    bool bell = FALSE;
    bool dofree;
    int	num;
    char 	*ptr;
    char	*io_string();
    long	strtol();

    messtype = 0;
    amessage = NULL;
    msg[0] = msg[MESSIZ - 1] = '\0';
    for (i = 1; argv[i]; i++) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
	    case 'p':
		messtype = MPERM;
		break;
	    case 't':
		messtype = MTEMP;
		break;
	    case 'f':
		messtype = MFRAME;
		break;
	    case 'o':
		output = TRUE;
		break;
	    case 'w':
		work = TRUE;
		break;
	    case 'b':
		bell = TRUE;
/* dmd s15
 * the following block reads:
 *
 * if the rest of the arg that starts with -b contains any integer,
 *    we beep that integer number of times
 * else if there are no more args
 *    we beep once
 * else since there is at least one more arg 
 *    we try and convert it ( the arg after the -b arg )
 *    if no conversion was possible or if the entire string wasn't a number
 *       we beep once
 *    else
 *       we beep that arg number of times.
 *
 * the only possible error here is if the next arg is really the
 * message we want to display and it is an integer string
 */
		if ((ptr=strpbrk(argv[i],"0123456789")))
			num = atoi(ptr);
		else if ( i+1 == argc )
			num = 1;
		else {
			num=(int)strtol(argv[i+1],&ptr,10);
			if ( ptr == argv[i+1] || *ptr != '\0' )
				num = 1;
			else
				i++;
		}
		break;
	    default:
		break;
	    }
	} else {
	    amessage = msg;
	    strncat(msg, argv[i], MESSIZ - 1 - strlen(msg));
	    strncat(msg, " ", MESSIZ - 1 - strlen(msg));
	}
    }


    if (amessage == NULL) {
	amessage = io_string(instr);
	dofree = TRUE;
    } else
	dofree = FALSE;

    if (messtype & MPERM) {
	mess_perm(amessage);
	mess_flash(amessage);
    }
    else if (messtype & MFRAME) {
	mess_frame(amessage);
	mess_flash(amessage);
	ar_ctl(ar_get_current(), CTSETMSG, TRUE);
    }
    else {			/* temporary message assumed */
	mess_temp(amessage);
	mess_flash(amessage);
    }
    if (output)
	putastr(amessage, outstr);
    if (dofree)
	free(amessage);
    if (work) 
	working(TRUE);
    /* les */
    else
	doupdate();

    if (bell) {
	for(i = num; i > 0; i--)
	    beep();
    }
    /*
      flush_output();
      */
    return SUCCESS;
}

int
cmd_indicator(argc, argv, instr, outstr, errstr)
int argc;
char *argv[];
IOSTRUCT *instr;
IOSTRUCT *outstr;
IOSTRUCT *errstr;
{
    char	msg[MESSIZ];
    register char	*amessage;
    register int	i, mescol, meslength;
    bool output = FALSE;
    bool work = FALSE;
    bool bell = FALSE;
    bool dofree;
    int	num;
    char 	*ptr;
    char	*io_string();

    mescol = 0;
    meslength = MESSIZ;
    amessage = NULL;
    msg[0] = msg[MESSIZ - 1] = '\0';
    for (i = 1; argv[i]; i++) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
	    case 'c':
		mescol = atoi(argv[i + 1]);
		i++;
		break;
	    case 'l':
		meslength = atoi(argv[i + 1]);
		i++;
		break;
	    case 'o':
		output = TRUE;
		break;
	    case 'w':
		work = TRUE;
		break;
	    case 'b':
		bell = TRUE;
		if((ptr=strpbrk(argv[i],"0123456789")) || (ptr=strpbrk(argv[i + 1],"0123456789"))) {
		    num = atoi(ptr);
		    i++;
		}
		else
		    num=1;
		break;
	    default:
		break;
	    }
	} else {
	    amessage = msg;
	    strncat(msg, argv[i], meslength - 1 - strlen(msg));
	    strncat(msg, " ", meslength - 1 - strlen(msg));
	}
    }


    if (amessage == NULL) {
	amessage = io_string(instr);
	dofree = TRUE;
    } else
	dofree = FALSE;

    indicator(amessage, mescol);
	
    if (output)
	putastr(amessage, outstr);
    if (dofree)
	free(amessage);
    if (work) 
	working(TRUE);
    else
	doupdate();

    if (bell) {
	for(i = num; i > 0; i--)
	    beep();
    }
    return SUCCESS;
}

#define MAX_PATTERNS	32

/*
 * Usage: regex [-v string] [-e] pattern [template] [pattern [template] ...]
 *
 * The first form takes a string and matches it against patterns until
 * there is a match.  Patterns are regular expression as described in
 * regcmp(3X).  When a match is found, the corresponding template is
 * returned in the output stream.  Templates may be arbitrary strings,
 * but if the string $m0, $m1, ..., $m9 appear in the string, then they
 * will be expanded to the corresponding pattern that matched within
 * (...)$0 through (...)$9 parts of the pattern.
 * For example:
 *		regex "0123456789" '.{2}(.{3})$0' 'hi $m0'
 * would return the string: 'hi 234'.
 * The default template is '$m0$m1$m2$m3$m4$m5$m6$m7$m8$m9'.
 *
 * The -v option causes string to be parsed instead of stdin
 * 
 * The -e option causes the resulting template to be evaluated
 */


cmd_regex(argc, argv, instr, outstr, errstr)
int argc;
char *argv[];
IOSTRUCT	*instr;
IOSTRUCT	*outstr;
IOSTRUCT	*errstr;
{
	IOSTRUCT *myoutstr, *tmpoutstr;
	register char	*p;
	char	*ptr, *value;
	char	buf[BUFSIZ];
	char	ret[10][BUFSIZ];
	char	*pattern[MAX_PATTERNS];
	char	*template[MAX_PATTERNS];
	static char	deftemplate[] = "$m0$m1$m2$m3$m4$m5$m6$m7$m8$m9";
	register int	i;
	register int	j;
	register int	numpatterns, execflag;
	register bool	matches;

	extern char	*regex();
	extern char	*regcmp();
	char	*getastr();
	int 	savecode, savetok, savequotes;

  
	value = NULL;
	execflag = FALSE;
	for (i = 1; argv[i][0] == '-'; i++) {
		switch (argv[i][1]) {
		case 'e':		/* evaluate the matched template */
			execflag = TRUE;
			break;
		case 'v':		/* from a variable */
			if (argv[1][2])
				value = &argv[1][2];
			else 
				value = argv[++i];
			break;
		default:
			break;
		}
	}
	if (execflag) {			/* generate an intermediate IO chanel */
		tmpoutstr = io_open(EV_USE_STRING, NULL);
		myoutstr = tmpoutstr;
	}
	else
		myoutstr = outstr;	/* use the requested IO channel */

	numpatterns = 0;
	for ( ; i < argc; i += 2) {
		/* my_regcmp() instead of regcmp() */
		if ((pattern[numpatterns] = regcmp(argv[i], NULL)) == NULL)
                        putastr(gettxt(":272","warning: bad regex pattern"), errstr);
		if (argv[i + 1])
			template[numpatterns++] = argv[i + 1];
		else
			template[numpatterns++] = deftemplate;
	}
	matches = FALSE;
	for ( ; ; ) {
		if (value)
			ptr = value;
		else if (getastr(buf, BUFSIZ, instr)) {
			ptr = buf;
			if (buf[i = (int)strlen(buf) - 1] == '\n')
				buf[i] = '\0';
		}
		else
			break;
		for (i = 0; i < numpatterns; i++) {
			if (pattern[i] == NULL)
				continue;
			for (j = 0; j < 10; j++)
				ret[j][0] = '\0';
			/* my_regex() instead of regex() */
			if (regex(pattern[i], ptr, ret[0], ret[1], ret[2],
					ret[3], ret[4], ret[5], ret[6],
					ret[7], ret[8], ret[9])) {
				matches = TRUE;
				for (p = template[i]; *p; p++) {
					if (*p == '$' && p[1] == 'm' && isdigit(p[2])) {
						putastr(ret[p[2] - '0'], myoutstr);
						p += 2;
					}
					else
						putac(*p, myoutstr);
				}
				if (strcmp(template[i], deftemplate) != 0)
					putac('\n', myoutstr);
				break;
			}
		}
		if (value)
			break;
	}
	if (execflag == TRUE) {		/* evaluate the matched template */
		/*
	 	 * Save/restore globals used in eval() before/after
		 * making a recursive call to eval() ... this SHOULD 
		 * change in the future and should not be necessary
		 */
		savecode = EV_retcode;
		savequotes = EV_backquotes;
		savetok = Lasttok;

		io_seek(myoutstr, 0);
		while (eval(myoutstr, outstr, EV_TOKEN | EV_USE_STRING))
			putac(' ', outstr);
		io_close(myoutstr);

		EV_retcode = savecode;
		EV_backquotes = savequotes;
		Lasttok = savetok;
	}
	for (i = 0; i < numpatterns; i++)
		if (pattern[i])
			/* regfree() instead of free() */
			free(pattern[i]);
	return matches ? SUCCESS : FAIL;
}


int
cmd_getlist(argc, argv, instr, outstr, errstr)
int argc;
char *argv[];
IOSTRUCT *instr, *outstr, *errstr;
{
    register int i, numitems, pending;
    struct actrec *cur_ar;
    register char *delimiter;
    char *itemptr;

    if (argc == 1)
	delimiter = "\n";
    else
	delimiter = argv[1];
	
    cur_ar = (struct actrec *) ar_get_current();
    if ((numitems = ar_ctl(cur_ar, CTGETSIZ)) == FAIL) {
	putac('\0', outstr);
	return(FAIL);
    }
    for (i = 0, pending = 0; i < numitems; i++) {
	ar_ctl(cur_ar, CTGETLIST, i, &itemptr); 
	if (itemptr != NULL) {
	    if (pending++)
		putastr(delimiter, outstr);
	    putastr(itemptr, outstr);
	}
    }
    putac('\0', outstr);
    return(SUCCESS);
}
/* defined messages below based on terrmess.c  abs s14 */
#define MISSING_STR "Some necessary information is missing: arg to pathconv" 
#define MISSING_STRID ":273"
#define MUNGED_STR  "Data has been corrupted bad: -arg to pathconv"
#define MUNGED_STRID ":274"

int
cmd_pathconv(argc, argv, instr, outstr, errstr)
int argc;
char *argv[];
IOSTRUCT *instr, *outstr, *errstr;
{
    register char	*p;
    register char	*pre;
    register char	*input;
    register int	i;
    register int	n;
    register int	width; 
    bool	fullpath;
    bool	show_max;
    bool	freeit;
    char	*io_string();
    char	*path_to_full();
    char	*path_to_title();

    char *i18n_msg;

    fullpath = TRUE;
    freeit = FALSE;
    show_max = FALSE;
    pre = input = NULL;
    width = MAX_TITLE;

    for (i = 1; i < argc; i++) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
	    case 'v':
		if (argv[i][2])
		    input = argv[i] + 2;
		else if ((input = argv[++i]) == NULL) {
                    i18n_msg=gettxt(MISSING_STRID,MISSING_STR);
		    putastr(i18n_msg, errstr); 		/* abs s14 */
		    i--;
		}
		break;
	    case 'l':
		show_max = TRUE;
		break;
	    case 'f':
		fullpath = TRUE;
		break;
	    case 't':
		fullpath = FALSE;
		break;
	    case 'n':
		if (argv[i][2])
		    width = atoi(argv[i] + 2);
		else if ((width = atoi(argv[++i])) <= 0) {
		    putastr(MISSING_STR, errstr); 		/* abs s14 */
		    i--;
		    width = MAX_TITLE;
		}
		break;
	    default:
                i18n_msg=gettxt(MUNGED_STRID,MUNGED_STR);
		putastr(i18n_msg, errstr); 			/* abs s14 */
		break;
	    }
	}
	else
	    pre = argv[i];
    }
    if (!input) {

	/* io_string was returning the input with tabs and newlines
	   and therefore input to path_to_full was not expanding the
	   alias.  The following two lines will truncate the white space
	   and newlines from input. -- added 7/89 by njp. */

	n = strcspn( input = io_string(instr), " \t\n");
	input[n]= '\0';
	freeit = TRUE;
    }
    if (input && *input && *input != ' ')
	if (fullpath) {
	    putastr(p = path_to_full(input), outstr);
	    free(p);
	}
	else {
             if ( Vflag && (width == 0))
                  width = 42;
	       if ( show_max )
	           putastr(bsd_path_to_title(input, width), outstr);
	       else
	           putastr(path_to_title(input, pre, width), outstr);
	}
    if (freeit)
	free(input);
    return SUCCESS;
}



cmd_setcolor(argc, argv, instr, outstr, errstr)
int argc;
char *argv[];
IOSTRUCT *instr;
IOSTRUCT *outstr;
IOSTRUCT *errstr;
{
	if (argc != 5)
		return(FAIL);
	if (setcolor(argv[1], atoi(argv[2]), atoi(argv[3]), atoi(argv[4]))) {
		putastr(argv[1], outstr);
		return(SUCCESS);
	}
	else {
		putastr("", outstr);
		return(FAIL);
	}
}
