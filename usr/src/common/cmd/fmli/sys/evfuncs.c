/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/




#ifdef CACA
#define _DEBUG2	1
#endif
/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */

#ident	"@(#)fmli:sys/evfuncs.c	1.40.3.13"


#define SUBSTLEN 2              /*** length of conversion specification ***/


#include	<stdio.h>
#include	<fcntl.h>
#include	"inc.types.h"	/* abs s14 */
#include	<sys/stat.h>
#include	<errno.h>
#include	<signal.h>
#include	<termio.h>
#include        <libgen.h>
#include	"wish.h"
#include	"eval.h"
#include	"ctl.h"
#include	"moremacros.h"
#include 	"message.h"
#include  	"interrupt.h"
#include	"retcodes.h"	/* abs */
#include	"sizes.h"
#include        <unistd.h>      
#include	"if_def_inc.h"  /* if then else macros; ck p7 */
#include	<locale.h>
#include	<string.h>

int	in_an_if = 0;			/* keeps track of if depth */
/* dmd TCB */
char	status_of_if[MAX_IF_DEPTH];	/* status and pos. of each if */

extern void exit();		/* fmli's own exit routine */

/*
 * return value of lastly executed command within an "if" statement
 */ 
static int Lastret = SUCCESS;


#ifdef TEST
main(argc, argv)
char	*argv[];
{
	IOSTRUCT	*in, *out, *err;

	wish_init(argc, argv);
	in = io_open(EV_USE_FP, stdin);
	out = io_open(EV_USE_FP, stdout);
	err = io_open(EV_USE_FP, stderr);
	exit(evalargv(argc - 1, argv + 1, in, out, err));
}

mess_temp(s)
char	*s;
{
	fprintf(stderr, "%s\n", s);
}

mess_perm(s)
char	*s;
{
	fprintf(stderr, "%s\n", s);
}

#endif 

int	cmd_if();
int	cmd_elif();
int	cmd_fmlmax();

cmd_fi(argc, argv, instr, outstr, errstr)
int	argc;
char	*argv[];
IOSTRUCT	*instr;
IOSTRUCT	*outstr;
IOSTRUCT	*errstr;
{
    if (in_an_if <= 0)
    {							  /* abs s14 */
	putastr( gettxt(":220","Syntax error - \"fi\" with no pending \"if\""), errstr);
	in_an_if = 0;
	return FAIL;
    }
    if (((status_of_if[in_an_if] & IN_A_THEN) == 0) && 
	((status_of_if[in_an_if] & IN_AN_ELSE) == 0) && 
	((status_of_if[in_an_if] & IN_AN_ELIF_SKIP) == 0))
    { 
	putastr(gettxt(":221","Syntax error - \"fi\" with no pending \"then\""), errstr);
	in_an_if = 0;
	return FAIL;
    }

    in_an_if--;

    if (argc > 1)
    {
	putastr( gettxt(":222","Syntax error - missing semi-colon after \"fi\" statement."),
		errstr);					/* abs s14 */
	in_an_if = 0;
	return FAIL;
    }

    return(Lastret);
}

cmd_then(argc, argv, instr, outstr, errstr)
int	argc;
char	*argv[];
IOSTRUCT	*instr;
IOSTRUCT	*outstr;
IOSTRUCT	*errstr;
{
    if (in_an_if <= 0)
    {
	/* changed from mess_temp abs s14 */
	putastr( gettxt(":223","Syntax error - \"then\" with no pending \"if\""), errstr);
	in_an_if = 0;
	return FAIL;
    }

    if (status_of_if[in_an_if] & IN_A_THEN)
    {
	putastr( gettxt(":224","Syntax error - \"then\" within \"then\""), errstr);/* abs s14 */
	in_an_if = 0;
	return FAIL;
    }

    if (status_of_if[in_an_if] & IN_AN_ELSE)
    {
	putastr( gettxt(":225","Syntax error - \"then\" within \"else\""),errstr); /* abs s14 */
	in_an_if = 0;
	return FAIL;
    }

    if (((status_of_if[in_an_if] & IN_A_CONDITION) == 0) &&
	((status_of_if[in_an_if] & IN_AN_ELIF_SKIP) == 0))
    {
	putastr( gettxt(":226","Syntax error - \"then\" without a preceeding \"if\""), errstr);
	in_an_if = 0;
	return FAIL;
    }

    /*
     * If we are in an "elif" but are NOT evaluating its condition
     * statement(s) (i.e., a previous "if" condition already evauated
     * to true) ... then just return SUCCESS;
     */
    if (status_of_if[in_an_if] & IN_AN_ELIF_SKIP)
	return SUCCESS;

    status_of_if[in_an_if] &= ~(ANY_IF_STATE);
    status_of_if[in_an_if] &= ~(IN_A_CONDITION);
    status_of_if[in_an_if] |= IN_A_THEN;

    return SUCCESS;
}

cmd_else(argc, argv, instr, outstr, errstr)
int	argc;
char	*argv[];
IOSTRUCT	*instr;
IOSTRUCT	*outstr;
IOSTRUCT	*errstr;
{
    if (in_an_if <= 0)
    {
	/* changed from mess_temp to putastr abs s14 */
	putastr( gettxt(":227","Syntax error - \"else\" with no pending \"if\""), errstr);
	in_an_if = 0;
	return FAIL;
    }

    if (status_of_if[in_an_if] & IN_AN_ELSE)
    {
	putastr( gettxt(":228","Syntax error - \"else\" within \"else\""), errstr);/* abs s14 */
	in_an_if = 0;
	return FAIL;
    }

    if (((status_of_if[in_an_if] & IN_A_THEN) == 0) &&  
	((status_of_if[in_an_if] & IN_AN_ELIF_SKIP) == 0))
    {
	putastr( gettxt(":229","Syntax error - \"else\" with no pending \"then\""), errstr);
	in_an_if = 0;
	return FAIL;
    }

    status_of_if[in_an_if] &= ~(ANY_IF_STATE);
    status_of_if[in_an_if] |= IN_AN_ELSE;

    return SUCCESS;
}
shell(argc, argv, instr, outstr, errstr)
int	argc;
char	*argv[];
IOSTRUCT	*instr;
IOSTRUCT	*outstr;
IOSTRUCT	*errstr;
{
	register int	i;
	register int	len;
	register char	*crunch;
	char	*myargv[4];
	char	*strnsave();

	len = 0;
	for (i = 1; i < argc; i++)
		len += (int)strlen(argv[i]);
	crunch = strnsave(nil, len + argc - 1);
	for (i = 1; i < argc; i++) {
		strcat(crunch, " ");
		strcat(crunch, argv[i]);
	}
	myargv[0] = "sh";
	myargv[1] = "-c";
	myargv[2] = crunch;
	myargv[3] = NULL;
	i = execute(4, myargv, instr, outstr, errstr);
	free(crunch);
	return i;
}

execute(argc, argv, instr, outstr, errstr)
int	argc;
char	*argv[];
IOSTRUCT	*instr;
IOSTRUCT	*outstr;
IOSTRUCT	*errstr;
{
    register char	*p;
    register FILE       *errfp;
    register pid_t	pid;	/* EFT abs k16 */
    int	pfd[2];
    char	*strchr();
    struct termio  tbuf2;
    bool  ioctl_pass = FALSE;
    char *InvalidFile;

    for ( ; argc > 0, *argv[0] == EOS; argv++, argc--) 	/* abs s19.4 */
	;						/* abs s19.4 */
    if (argc == 0)					/* abs s19.4 */
	return SUCCESS;					/* abs s19.4 */

    if (argc > 1 && strcmp(argv[0], "extern") == 0) 	/* abs s19.4 */
    {
	argv++;
	argc--;						/* abs s19.4 */
    }

    if (pipe(pfd))
	return FAIL;
    if (errstr == NULL || (errstr->flags & EV_USE_STRING))   /* abs s14 */
	if ((errfp = fopen(p = tmpnam(NULL), "w+")) != NULL)
/*	if ((errfd = open(p = tmpnam(NULL), O_EXCL | O_CREAT | O_RDWR, 0600)) >= 0)
abs */
	    unlink(p);
	else
	    return FAIL;
    else
	errfp = errstr->mu.fp;

    if (ioctl(fileno(stdin), TCGETA, &tbuf2) != -1) 
	ioctl_pass = TRUE ; /* Save the terminal settings */

    switch (pid = fork()) {
    case -1:
	return FAIL;
    case 0:			/* child */
	close(pfd[0]);
	close(1);
	dup(pfd[1]);
	close(pfd[1]);
	close(2);
	dup(fileno(errfp));
	fclose(errfp);

    {
	if (instr->flags & EV_USE_FP) {
	    close(0);
	    dup(fileno(instr->mu.fp));
	}
	else if (instr->mu.str.count > 0) {
	    register int	c;
	    register FILE	*infp;
	    FILE	*tempfile();

	    if ((infp = tempfile(NULL, "w+")) == NULL)
		exit(1);
	    while (c = getac(instr))
		putc(c, infp);
	    close(0);
	    dup(fileno(infp));
	    fclose(infp);
	}
	if (Cur_intr.interrupt)   	/* if (interrupts enabled) */
	    sigset(SIGINT, SIG_DFL);
	else			        /* hide the interrupt key */
	{
	    struct termio  tbuf;

	    if (ioctl(fileno(stdin), TCGETA, &tbuf) != -1) /* if successful.. */
	    {
		tbuf.c_cc[VINTR] = 0xff;
		ioctl(fileno(stdin), TCSETA, &tbuf);
	    }
	}
	execvp(argv[0], argv);
	error_exec(errno);
	/* This gives the filename instead of just "fmli: No such file
        or directory" */
        InvalidFile = (char *) malloc(sizeof(char) * BUFSIZ);
        sprintf(InvalidFile, "fmli: %s", basename(argv[0]));
        perror(InvalidFile);
        free(InvalidFile);
	exit(R_BAD_CHILD);	/* abs changed from exit(1).
				   This is fmli's exit not the C lib. call */
    }
	break;
    default:			/* parent (FMLI) */
    {
	register int	c;
	register int	retval;
	FILE	*fp;

	close(pfd[1]);
	if ((fp = fdopen(pfd[0], "r")) == NULL)
	    return FAIL;

	/* the errno == EINTR is added below to check for
	   system interrupts like MAILCHECK that were terminating
	   the read in progress -- added 7/89 by njp */
	while ((c = getc(fp)) != EOF || errno == EINTR)
	{
	    if (errno != EINTR)			/* abs s16 */
		putac(c, outstr);
	    errno = 0;
	}
	fclose(fp);
	retval = waitspawn(pid);

/* for unknown reasons  this never worked (returns c = 0) abs
            if ((c = read(errfd, buf, sizeof(buf) - 1)) > 0) {
*/

	/* if stderr wasn't redirected to a file, save it. abs s14 */
	if (errfp != NULL && errstr != NULL && (errstr->flags & EV_USE_STRING))
	{
	    char	buf[MESSIZ];

	    rewind(errfp);				/* abs s14 */
	    if ((c = fread(buf, sizeof(char), MESSIZ-1, errfp)) > 0)
	    {
		buf[c] = '\0';
		putastr(buf, errstr);
	    }
	}

	if (ioctl_pass == TRUE )
	     ioctl(fileno(stdin), TCSETA, &tbuf2); /* Reset the terminal settings */

	if (errfp != NULL)
	    fclose(errfp);
	return retval;
    }
	break;
    }
    /*
     * lint will complain about it, but there's actually no way to
     * reach here because of the exit(), so this is not a return
     * without an expression
     */
}

get_wdw(argc, argv, instr, outstr, errstr)
int	argc;
char	*argv[];
IOSTRUCT	*instr;
IOSTRUCT	*outstr;
IOSTRUCT	*errstr;
{
	int wdwnum;

	wdwnum = ar_ctl(ar_get_current(), CTGETWDW);
	putastr(itoa((long)wdwnum, 10), outstr); /* abs k16 */
	return SUCCESS;
}

getmod(argc, argv, instr, outstr, errstr)
int	argc;
char	*argv[];
IOSTRUCT	*instr;
IOSTRUCT	*outstr;
IOSTRUCT	*errstr;
{
    static mode_t modes;	/* EFT abs k16 */
    int i;
    struct stat	statbuf;
    long	strtol();
    char	*bsd_path_to_title();
 
    char *i18n_string;
    int   i18n_length;

    if (argc < 2)
	return FAIL;
    i = 1;
    if (argc > 2) {
	i++;
	if (strcmp(argv[1], "-u") == 0) {
	    register char	*p;
	    char	*getepenv();

	    if ((p = getepenv("UMASK")) == NULL || *p == '\0')
		modes = 0775;
	    else
		modes = ~(strtol(p, NULL, 8)) & 0777; 
	} else if (stat(argv[1], &statbuf) == -1) {

            /*** This is a new function to handle the construction of
                 semantic units that are a prerequisite for internationalised
                 code
                 dynamic concatenation of strings will cause problems
                 due to different syntax in different languages 
                 thus, complete strings with literals into msg catalogues
            ***/

            i18n_string=gettxt(":158","Could not access %s");
            i18n_length=(int)strlen(i18n_string) - SUBSTLEN;

            io_printf('p', errstr, i18n_string,
		       bsd_path_to_title(argv[1], MESS_COLS-i18n_length) );

	    return FAIL;
	} else
	    modes = statbuf.st_mode;
    }
    if (strtol(argv[i], NULL, 8) & modes)
	putastr( gettxt(":189","yes"), outstr);
    else
	putastr( gettxt(":230","no"), outstr);
    return SUCCESS;
}

setmod(argc, argv, instr, outstr, errstr)
int	argc;
char	*argv[];
IOSTRUCT	*instr;
IOSTRUCT	*outstr;
IOSTRUCT	*errstr;
{
    register int	i;
    register mode_t	mode;	/* EFT abs k16 */
    char *bsd_path_to_title();

    char *i18n_string;
    int   i18n_length;

    if (argc < 2)
	return FAIL;
    for (i = 2, mode = 0; argv[i]; i++)
    {
	int cmp_val1,cmp_val2;

	cmp_val1 = strCcmp(argv[i], "yes");
	/*** for face-compatibility: nationalized "yes"/"no"
	     are valid, too ***/
	cmp_val2 = strCcmp(argv[i], gettxt( ":189", "yes") );
	cmp_val1 = (cmp_val1 == 0 || cmp_val2 == 0) ? 0 : 1;
	mode = (mode << 1) | !cmp_val1;
    }

    if ((mode & 0600) != 0600)
	putastr( gettxt(":231","WARNING: You are denying some permissions to yourself!"),
		errstr);	/* abs s14 */

    if (strcmp(argv[1], "-u") == 0) {
	char buf[20];

	mode = ~mode & 0777;
	(void) umask(mode);
	sprintf(buf, "0%o", mode);
	return chgepenv("UMASK", buf);
    } else if (chmod(argv[1], mode) < 0) {
        i18n_string=gettxt(":232","Unable to change security on %s");
        i18n_length=(int)strlen(i18n_string) - SUBSTLEN;

        io_printf( 'p', errstr, i18n_string,
			  bsd_path_to_title(argv[1], MESS_COLS-i18n_length) );

	return(FAIL);
    } else
	return SUCCESS;
}

/* dmd TCB */
static int Long_line = -1;

long_line(argc, argv, instr, outstr, errstr)
int	argc;
char	*argv[];
IOSTRUCT	*instr;
IOSTRUCT	*outstr;
IOSTRUCT	*errstr;
{
    register int maxcount, c, count;
    FILE *fp;

    if (argv[1]) {		/* if there is a file argument */
	if ((fp = fopen(argv[1], "r")) == NULL)
	    return(FAIL);
	for (maxcount = 0, count = 0; (c = getc(fp)) != EOF; count++) {
	    if (c=='\t') { while (++count % 8); count--; }
	    else if (c == '\n') {
		maxcount = max(maxcount, count);
		count = -1;
	    }
	}
	fclose(fp);
	Long_line = max(maxcount, count);
    }
    else if (Long_line < 0)
	return(FAIL);
    putastr(itoa((long)Long_line + 1, 10), outstr); /* abs k16 */
    return SUCCESS;
}

/***********************************************************************
*
* char *nls_path(f)
* char *f;
*
* This function converts the file name "f" into a language dependend
* file name for the readfile command. For this a language dependend
* directory name is inserted before the final component of the file
* name:
*     <path>/<file> -> <path>/<locale>/<file>
* or  <file>        -> ./<locale>/<file>
*
* <locale> will have the following value:
* "" if the locale category LC_MESSAGES is "C" or blank. Otherwise it has
* the value of the category as determined by setlocale()
*
* NULL is returned if there is no language dependend file.
*
* If a valid file name is returned, the returned string address is the
* address of a dynamically allocated string. The caller is responsible
* for freeing this memory.
************************************************************************
*/


static char *
nls_path (f)
char *f;	/* the original file name that is to be mapped into a
		** language dependend file name   		     */
{
    char *new;	/* will hold the address of the new lang. dep. file   */
    char *p;	/* scratch					      */
    char *insert;/* insertion point for lang. dep. file name part     */
    int count;
    static char *lc_messages;

    extern char *getenv(), *strrchr();

#ifdef DEBUG
    fprintf (stderr, "nls_path (%s)\n", f);
#endif
    if ( lc_messages == NULL )
      {
	lc_messages = setlocale (LC_MESSAGES,NULL);
        if  (strncmp (lc_messages, "C", 1) == 0 )
	    lc_messages = nil;
	else
	    lc_messages = strsave (lc_messages);
      }
    if ( lc_messages != nil )
	new = malloc (strlen(f) + strlen(lc_messages) + 8);
    else 
	new = NULL;
    
    if ( new == NULL )
	return (NULL);
    
    (void) strcpy (new, f);
    if ( (p = strrchr (new, '/')) == NULL )
      {
	count = 0;
	p = new;
	*p++ = '.';
      }
    else
	count = p - new;
    *p++ = '/';
    insert = p;

    if ( lc_messages != nil )
      {
	(void) strcpy (p, lc_messages);
	if ( count == 0 )
	    (void) strcat (p, "/");
	(void) strcat (p, f + count);
#ifdef DEBUG
	fprintf (stderr, "new full lang path with LC_MESSAGES:\"%s\"\n", new);
#endif
	if ( access(new, R_OK) >= 0 )
	    return (new);
      }

#ifdef DEBUG
    fprintf (stderr, "no language dependend file for %s\n", f);
#endif

    (void) free (new);
    return (NULL);
}

/*************************************************************************
* genfind(f):
* global accesible interface for nls_path(f) (see above)
* for usage with "genfind()" (see this file)
**************************************************************************/
char *genfind_nls_path(f)
char *f;
{
  return(nls_path(f));
}

read_file(argc, argv, instr, outstr, errstr)
int	argc;
char	*argv[];
IOSTRUCT	*instr;
IOSTRUCT	*outstr;
IOSTRUCT	*errstr;
{
    char	*p, *f;
    register int c, count, maxcount;
    FILE	*fp;
    char 	*n;
    char	*path_to_full();

    if (argc > 1)
	f = path_to_full(argv[1]);
    else {
	f = path_to_full(p = io_string(instr));
	free(p);
    }
    if ( (n = nls_path (f)) == NULL )
      {
	n = f;
	f = NULL;
      }
    if ( f != NULL )
        (void) free (f);
    if ((fp = fopen(n, "r")) == NULL)
      {
	(void) free (n);
	return(FAIL);
      }
    if ( n != NULL )
        (void) free(n);
    for (count = 0, maxcount = 0; (c = getc(fp)) != EOF; count++) {
	if (c=='\t') { while (++count % 8); count--; }
	else if (c == '\n') {
	    maxcount = max(maxcount, count);
	    count = -1;
	}
	putac(c, outstr);
    }
    Long_line = max(maxcount, count);
    fclose(fp);
    return SUCCESS;
}

cmd_echo(argc, argv, instr, outstr, errstr)
int	argc;
char	*argv[];
IOSTRUCT	*instr;
IOSTRUCT	*outstr;
IOSTRUCT	*errstr;
{
	register char	*p;
	register int	i;
	char	*strrchr();

	for (i = 1; i < argc; i++) {
		if (i > 1)
			putac(' ', outstr);
		if (i == argc - 1 && (p = strrchr(argv[i], '\\')) && strcmp(p, "\\c'") == 0) {
			*p = '\0';
			putastr(argv[i], outstr);
			return SUCCESS;
		}
		putastr(argv[i], outstr);
	}
	putac('\n', outstr);
	return SUCCESS;
}

#ifndef TEST
extern int	cocreate();
extern int	cosend();
extern int	codestroy();
extern int	cocheck();
extern int	coreceive();
extern int	genfind();
extern int	cmd_pathconv();
#endif 
extern int	cmd_set();
extern int	cmd_run();
extern int	cmd_regex();
extern int	cmd_getlist();
extern int	cmd_setcolor();
extern int	cmd_reinit();
extern int	cmd_message();
extern int	cmd_indicator();
extern int	cmd_unset();
extern int	cmd_getodi();
extern int	cmd_setodi();
extern int	cmd_cut();
extern int	cmd_grep();
extern int	cmd_test();		/* ehr3 */
extern int	cmd_expr();

#define NUM_FUNCS	(sizeof(func) / sizeof(*func))

static struct {
	char	*name;
	int	(*function)();
} func[] = {
	{ "extern",	execute },
	{ "shell",	shell },
	{ "regex",	cmd_regex },
	{ "echo",	cmd_echo },
	{ "fmlcut",	cmd_cut },
	{ "fmlgrep",	cmd_grep },
	{ "fmlexpr",	cmd_expr },
	{ "set",	cmd_set },
	{ "unset",	cmd_unset },
	{ "getmod",	getmod },
	{ "getodi",	cmd_getodi },
	{ "getfrm",	get_wdw },
	{ "getwdw",	get_wdw },	/* alias to getfrm */
	{ "setmod",	setmod },
	{ "setodi",	cmd_setodi },
	{ "readfile",	read_file },
	{ "longline",	long_line },
	{ "message",	cmd_message },
	{ "indicator", 	cmd_indicator },
	{ "run",	cmd_run },
	{ "getitems",	cmd_getlist },
	{ "genfind",	genfind },
	{ "pathconv",	cmd_pathconv },
	{ "setcolor",	cmd_setcolor},
	{ "reinit",	cmd_reinit},
	{ "test",	cmd_test},	/* ehr3 */
	{ "[",		cmd_test},	/* ehr3 */
	{ "if",		cmd_if},	/* ehr3 */
	{ "then",	cmd_then},	/* ehr3 */
	{ "else",	cmd_else},	/* ehr3 */
	{ "elif",	cmd_elif},	/* ehr3 */
	{ "fi",		cmd_fi},	/* ehr3 */
        { "fmlmax",	cmd_fmlmax},
/*
 * not yet ...
 *
	{ "true",	cmd_true},
	{ "false",	cmd_false},
 */
#ifndef TEST
	{ "cocreate",	cocreate },
	{ "cosend",	cosend },
	{ "codestroy",	codestroy },
	{ "cocheck",	cocheck },
	{ "coreceive",	coreceive }
#endif 
};

evalargv(argc, argv, instr, outstr, errstr)
int	argc;
char	*argv[];
IOSTRUCT	*instr;
IOSTRUCT	*outstr;
IOSTRUCT	*errstr;
{
    register int	n, ret;
    int	n2;

    /*	test moved to calling routine, SUCCESS is wrong here. abs
     *	if (argc < 1)
     *		return SUCCESS;
     */

    for (n = 0; n < NUM_FUNCS; n++)
	if (strcmp(argv[0], func[n].name) == 0)
	    break;

    if (n >= NUM_FUNCS)
	n = 0;

    if (in_an_if) {
	switch(argv[0][0]) {
	case 'i':
	    if (!strcmp(argv[0], "if")) {
		ret = cmd_if(argc, argv, instr, outstr, errstr); /* abs s15 */
		return ret;
	    }
	    break;

	case 't':
	    if (!strcmp(argv[0], "then")) {
		ret = cmd_then(argc, argv, instr, outstr, errstr); /* abs s15 */
		return ret;
	    }
	    break;

	case 'e':
	    if (!strcmp(argv[0], "else")) {
		ret = cmd_else(argc, argv, instr, outstr, errstr); /* abs s15 */
		return ret;
	    }

	    if (!strcmp(argv[0], "elif")) {
		ret = cmd_elif(argc, argv, instr, outstr, errstr); /* abs s15 */
		return ret;
	    }
	    break;

	case 'f':
	    if (!strcmp(argv[0], "fi")) {
		ret = cmd_fi(argc, argv, instr, outstr, errstr); /* abs s15 */
		return ret;
	    }
	    break;
	}

	/*
	  AFTER checking for if-then-else stuff 
	  we need to determine if we are in 
	  executable code or not. We do this by 
	  checking each prior level of nesting. 
	  If any of them fails, then we know 
	  we should not execute this command.
	  */
	for (n2 = 1; n2 <= in_an_if; n2++) {
	    if (status_of_if[n2] & IF_IS_TRUE) {
		/*
		 * The condition is TRUE ...
		 * skip the command if:
		 *
		 * we are in an "else"
		 *
		 * we are in an "elif" but a previous
		 * "if" or "elif" evaluated to true
		 */
		if ((status_of_if[n2] & IN_AN_ELSE) ||
		    (status_of_if[n2] & IN_AN_ELIF_SKIP)) 
		    return SUCCESS;
	    } else {
		/*
		 * The condition is FALSE ...
		 * skip the command if we are in
		 * a "then"
		 */
		if (status_of_if[n2] & IN_A_THEN)
		    return SUCCESS;
	    }
	}

	if (status_of_if[in_an_if] & IN_A_CONDITION) {
	    int	cmd_rc;

	    cmd_rc = (*func[n].function)(argc, argv, instr, outstr, errstr);
	    if (cmd_rc == SUCCESS)
		status_of_if[in_an_if] |= IF_IS_TRUE;
	    else 
		status_of_if[in_an_if] &= ~IF_IS_TRUE;
	    return(cmd_rc);
	} else {
	    /*
	     * Keep track of the return value from the
	     * lastly executed built-in/executable ...
	     * This value will determine the SUCCESS/FAILURE
	     * of the if/then/else statement (see cmd_fi).
	     */
	    Lastret = (*func[n].function)(argc, argv, instr, outstr, errstr);
	    return(Lastret);
	}
    }
    else return (*func[n].function)(argc, argv, instr, outstr, errstr);
}

cmd_if(argc, argv, in, out, err)
int	argc;
char	*argv[];
IOSTRUCT	*in;
IOSTRUCT	*out;
IOSTRUCT	*err;
{
    int	n;
    int	n2;

    in_an_if++;
    status_of_if[in_an_if] = IN_A_CONDITION;

    if (in_an_if == MAX_IF_DEPTH)
    {
	putastr( gettxt(":233","Internal error - \"if\" stack overflow"), err);	/* abs s14 */
	in_an_if = 0;
	return FAIL;
    }

    return SUCCESS;
}

cmd_elif(argc, argv, in, out, err)
int	argc;
char	*argv[];
IOSTRUCT	*in;
IOSTRUCT	*out;
IOSTRUCT	*err;
{
    int	n;
    int	n2;

    if (in_an_if <= 0)
    {
	/* changed from mess_temp to putastr abs s14 */
	putastr( gettxt(":234","Syntax error - \"elif\" with no pending \"if\""), err);
	in_an_if = 0;
	return FAIL;
    }

    if (status_of_if[in_an_if] & IN_AN_ELSE)
    {
	putastr( gettxt(":235","Syntax error - \"elif\" after an \"else\""), err); /* abs s14 */
	in_an_if = 0;
	return FAIL;
    }

    if (((status_of_if[in_an_if] & IN_A_THEN) == 0) &&  
	((status_of_if[in_an_if] & IN_AN_ELIF_SKIP) == 0))
    {
	putastr( gettxt(":236","Syntax error - \"elif\" with no pending \"then\""), err);
	in_an_if = 0;
	return FAIL;
    }


    /*
     * if a previous "if/elif" condition is TRUE 
     * then don't evaluate the "elif" condition.
     */
	
    if (status_of_if[in_an_if] & IF_IS_TRUE) {
	status_of_if[in_an_if] &= ~(ANY_IF_STATE);
	status_of_if[in_an_if] |= IN_AN_ELIF_SKIP;
    }
    else 
	status_of_if[in_an_if] = IN_A_CONDITION; 

    return SUCCESS;
}


cmd_fmlmax(argc,argv,in,out,err)
int argc;
char *argv[];
IOSTRUCT	*in;
IOSTRUCT	*out;
IOSTRUCT	*err;
{
  int i,maxlen=0;
  int ncol = 1;
  int add = 1;

  if (argc < 2) 
     putastr( gettxt(":356","fmlmax: wrong number of arguments"),err);

  i=1;

  if (strcmp(argv[1],"-c")==0)
    {
      ncol = atoi(argv[2],10);
      i=3;
    }
  else if (strcmp(argv[1],"-l") == 0)
    {
      ncol = 0;
      add = 0;
      i = 2;
    }

  for (; i < argc; i++)
  {
    char *q,*p = argv[i];

    do
    {
      q=strchr(p,'\n');
      if (!q) q=strrchr(p,0);
      if ((int)(q - p) > maxlen) maxlen = (int)(q - p);
      p=q+1;
    }
    while (*q);
  }
  maxlen = ncol + maxlen + add;
  putastr(itoa(maxlen,10),out);
  return(SUCCESS);
}


/*
 * not yet ...
 *
cmd_true(argc, argv, in, out, err)
int	argc;
char	*argv[];
IOSTRUCT	*in;
IOSTRUCT	*out;
IOSTRUCT	*err;
{
	return(SUCCESS);
}

cmd_false(argc, argv, in, out, err)
int	argc;
char	*argv[];
IOSTRUCT	*in;
IOSTRUCT	*out;
IOSTRUCT	*err;
{
	return(FAIL);
}
*/

int
Evaluate_if()
{
	int n2;

		/*
			AFTER checking for if-then-else stuff
			we need to determine if we are in
			executable code or not. We do this by
			checking each prior level of nesting.
			If any of them fails, then we know
			we should not execute this command.
		*/
		for (n2 = 1; n2 <= in_an_if; n2++) {
			if (status_of_if[n2] & IF_IS_TRUE) {
				/*
				 * The condition is TRUE ...
			         * skip the command if:
				 *
				 * we are in an "else"
				 *
				 * we are in an "elif" but a previous
				 * "if" or "elif" evaluated to true
				 */
				 if ((status_of_if[n2] & IN_AN_ELSE) ||
				     (status_of_if[n2] & IN_AN_ELIF_SKIP))
					return FALSE;
			 } else {
				/*
				 * The condition is FALSE ...
				 * skip the command if we are in
				 * a "then"
				 */
				if (status_of_if[n2] & IN_A_THEN)
					return FALSE;
			  }
		}
		return TRUE;
}
