/*		copyright	"%c%" 	*/


#ident	"@(#)lp.c	1.7"
#ident  "$Header$"

/*******************************************************************************
 *
 * FILENAME:    lp.c
 *
 * DESCRIPTION: The "lp" command: Print files on a line printer
 *
 * SCCS:	lp.c 1.7  9/3/97 at 11:31:51
 *
 * CHANGE HISTORY:
 *
 * 02-09-97  Paul Cunningham        ul95-05915
 *           Change function main() so that it appends "'" to the end of the
 *           last file name in the command line list, instead of doing a
 *           called appendlist( &opts, "'"). This gets rid of the trailing
 *           space at the end of the flist in the control file, so it look
 *           like; flist='/file1 /file2 /file3'
 *           Note: this does not remove it for print jobs received across the
 *           netware, that requires a change to lpNet, but that has been done
 *           under this MR.
 * 03-09-97  Paul Cunningham        ul95-17319
 *           Change function copyfile() so that it checks the result from 
 *           fwrite() to check that the copy competed okay (eg. the disk did
 *           not fill up), if it fails report an error (used E_LP_MNOMEM) and
 *           exit lp program.
 *
 *******************************************************************************
 */

/***************************************************************************
 * Command: lp
 * Inheritable Privileges: P_MACREAD,P_DACREAD
 *       Fixed Privileges: None
 *
 ***************************************************************************
 */

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <priv.h>
#include <locale.h>
#include "requests.h"
#include "lp.h"
#include "msgs.h"
#include "printers.h"

#define WHO_AM_I	I_AM_LP
#include "oam.h"
#include "lpd.h"

#define TRUE 1
#define FALSE 0
#define FSTYPES "/etc/dfs/fstypes"
#define LCL_HDR "locale="
#define MAX_FILE_SIZE	20

static char *dest = NULL;	/* destination class or printer */
static struct stat stbuf;	/* Global stat buffer */
static char *title = NULL;	/* User-supplied title for output */
static int specialh = 0;	/* -H flag indicates special handling */
#define HOLD 1
#define RESUME 2
#define IMMEDIATE 3
static char *formname = NULL;	/* form to use */
static char *char_set = NULL;	/* print wheel or character set to use */
static char *cont_type = NULL;	/* content type of input files */
static short priority = -1;	/* priority of print request */
static short copies = 0;	/* number of copies of output */
static char **opts = NULL;	/* options for interface program */
static char **yopts = NULL;	/* options for filter program */
static char *pages = NULL;	/* pages to be printed */
static short silent = FALSE;	/* don't run off at the mouth */
static short mail = FALSE;	/* TRUE => user wants mail, FALSE ==> no mail */
static short wrt = FALSE;	/* TRUE => user wants notification on tty via write
			   FALSE => don't write */
short raw = FALSE;	/* set option xx"stty=raw"xx and raw flag if true */
static short copy = FALSE;	/* TRUE => copy files, FALSE ==> don't */
static short remove_after = FALSE; /* TRUE => remove files, FALSE ==> don't */
static char *curdir;	/* working directory at time of request */
static char *pre_rqid = NULL;	/* previos request id (-i option) */
static char *reqid = NULL;	/* request id for this job */
static char reqfile[20];	/* name of request file */
static char *locale = NULL;	/* locale selected for this job */

static char **files = NULL;	/* list of file to be printed */
static char **orig_files = NULL;/* list of file to be removed */
static int nfiles = 0;		/* number of files on cmd line (excluding "-") */
static int stdinp = 0;		/* indicates how many times to print std input
			   -1 ==> standard input empty		*/
static char *stdinfile;
static char *rfilebase;

extern char *strcpy(), *strdup(), *strchr(), *que_job(),
    *sprintlist(), *getspooldir();
extern int appendlist(), errno;

#define OPTSTRING "q:H:f:d:L:T:S:o:y:P:i:cmwn:st:rR"

char *strcat(), *strncpy();
static	void  escape(char *, char **, char *);
static void ck_mount();
static int
chk_cont_type(str)
char *str;
{
    if (STREQU(str, NAME_ANY) || STREQU(str, NAME_TERMINFO)) {
	LP_ERRMSG2(ERROR, E_LP_BADOARG, 'T', str);
	exit(1);
    }
}

main(argc, argv)
int argc;
char *argv[];
{
    int letter;
    char *p, **templist, **stemp;
    char *file, *cptr, *newstr;
    REQUEST *reqp, *makereq();
    int fileargs = 0;
    int insize = 0;
    int exitval = 0;
    extern char *optarg;
    extern int optind, opterr, optopt;

    /*
    **  Turn off all privs.  We don't want to do anything w/
    **  priv we can do without.
    */
/*
    (void)  procprivl (CLRPRV, MACREAD_W, DACREAD_W, (priv_t)0);
*/

    /* Read default copy mode from /etc/default/lp | with lpadmin -O */
    if (STREQU(getcpdefault(), "copy-files: copy"))
	copy = TRUE;	/* TRUE => copy files, FALSE ==> don't */
    if (STREQU(getcpdefault(), "copy-files: nocopy"))
	copy = FALSE;	/* TRUE => copy files, FALSE ==> don't */

    opterr = 0; /* disable printing of errors by getopt */
    while ((letter = getopt(argc, argv, OPTSTRING)) != -1)
	switch(letter) {
	case 'R':	/* remove file after submitting */
	    remove_after = TRUE;
	    /* FALLTHROUGH */
	case 'c':	/* copy files */
	    copy = TRUE;
	    break;
	case 'd':	/* destination */
	    if (dest) LP_ERRMSG1(WARNING, E_LP_2MANY, 'd');
	    dest = optarg;
	    if (!isprinter(dest) && !isclass(dest) && !STREQU(dest, NAME_ANY)) {
		LP_ERRMSG1(ERROR, E_LP_DSTUNK, dest);
		exit (1);
	    }
	    break;
	case 'f':
	    if (formname) LP_ERRMSG1(WARNING, E_LP_2MANY, 'f');
	    formname = optarg;
	    break;
	case 'H':
	    if (specialh) LP_ERRMSG1(WARNING, E_LP_2MANY, 'H');
	    if (STREQU(optarg, "hold")) specialh = HOLD;
	    else if (STREQU(optarg, "resume")) specialh = RESUME;
	    else if (STREQU(optarg, "immediate")) specialh = IMMEDIATE;
	    else {
		LP_ERRMSG2(ERROR, E_LP_BADOARG, 'H', optarg);
		exit(1);
	    }
	    break;
	case 'i':
	    if (pre_rqid) LP_ERRMSG1(WARNING, E_LP_2MANY, 'i');
	    pre_rqid = optarg;
	    break;
	case 'm':	/* mail */
	    if (mail) LP_ERRMSG1(WARNING, E_LP_2MANY, 'm');
	    mail = TRUE;
	    break;
	case 'n':	/* # of copies */
	    if (copies) LP_ERRMSG1(WARNING, E_LP_2MANY, 'n');
	    if (
		*optarg == 0
	     || (copies=(int)strtol(optarg, &p, 10)) <= 0
	     || copies > MOST_FILES
	     || *p
	    ) {
		LP_ERRMSG2(ERROR, E_LP_BADOARG, 'n', optarg);
		exit(1);
	    }
	    break;
	case 'o':	/* option for interface program */
	    stemp = templist = getlist(optarg, " \t", "");  /* MR bl88-13915 */
	    if (!stemp)
		break;			/* MR bl88-14720 */
	    while (*templist)
		appendlist(&opts, *templist++);
	    freelist(stemp);
	    break;
	case 'y':
	    stemp = templist = getlist(optarg, " \t", ",");
	    if (!stemp)
		break;			/* MR bl88-14720 */
	    while (*templist)
		appendlist(&yopts, *templist++);
	    freelist(stemp);
	    break;
	case 'P':
	    if (pages) LP_ERRMSG1(WARNING, E_LP_2MANY, 'P');
	    pages = optarg;
	    break;
	case 'q':
	    if (priority != -1) LP_ERRMSG1(WARNING, E_LP_2MANY, 'q');
	    priority = (int)strtol(optarg, &p, 10);
	    if (*p || priority<0 || priority>39) {
		LP_ERRMSG1(ERROR, E_LP_BADPRI, optarg);
	 	exit(1);
	    }
	    break;
	case 'r':
	    if (raw) LP_ERRMSG1(WARNING, E_LP_2MANY, 'r');
	    raw = TRUE;
	    break;
	case 's':	/* silent */
	    if (silent) LP_ERRMSG1(WARNING, E_LP_2MANY, 's');
	    silent = 1;
	    break;
	case 'S':
	    if (char_set) LP_ERRMSG1(WARNING, E_LP_2MANY, 'S');
	    char_set = optarg;
	    break;
	case 't':	/* title */
	    if (title) LP_ERRMSG1(WARNING, E_LP_2MANY, 't');
	    title = optarg;
	    break;
	case 'T':
	    if (cont_type) LP_ERRMSG1(WARNING, E_LP_2MANY, 'T');
	    chk_cont_type(optarg);
	    cont_type = optarg;
	    break;
	case 'w':	/* write */
	    if (wrt) LP_ERRMSG1(WARNING, E_LP_2MANY, 'w');
	    wrt = TRUE;
	    break;
	case 'L':
	    if (locale) LP_ERRMSG1(WARNING, E_LP_2MANY, 'L');
	    locale = optarg;
	    break;
	default:
	    if (optopt == '?') {
/*
 * The usage: message of the lp command is a very long message that does
 * not fit into the message buffer of 512 bytes.  The message was broken
 * into 4 messages USAGE, USAGE1, USAGE2 and USAGE3. The last three of
 * them are displayed with MM_NOSTD option to exclude the label and make
 * all 4 messages look like one.
*/
                LP_OUTMSG(INFO, E_LP_USAGE);
                LP_OUTMSG(MM_NOSTD, E_LP_USAGE1);
                LP_OUTMSG(MM_NOSTD, E_LP_USAGE2);
                LP_OUTMSG(MM_NOSTD, E_LP_USAGE3);
                LP_OUTMSG(MM_NOSTD, E_LP_USAGE4);
		exit(0);
	    }
	    (p = "-X")[1] = optopt;
	    if (strchr(OPTSTRING, optopt))
		LP_ERRMSG1(ERROR, E_LP_OPTARG, p);
	    else
		LP_ERRMSG1(ERROR, E_LP_OPTION, p);
	    exit(1);
	}

        /* -H resume is illegal without -i <req-id>  abs s20.1 */
        if (specialh == RESUME && pre_rqid == NULL)
	{
	    LP_ERRMSG(ERROR, E_LP_BADHARG);
	    exit(1);
	}

	if (mail && wrt) LP_ERRMSG(WARNING, E_LPP_COMBMW);

	/* if copy != TRUE, then check if the lp resources are
           remotely mounted  */
	if (!copy)
	   ck_mount();
	/* check locale and establish default if necessary */
	if ((specialh != RESUME) || (yopts || opts)) {
	    if (locale) {
	        cptr = (char *) malloc(strlen(locale) + strlen(LCL_HDR) + 2);
	        sprintf(cptr, "%s%s", LCL_HDR, locale);
	        /* locale value passed in interface options list */
	        appendlist( &opts, cptr);
	        /* locale value passed in filter modes list */
	        if (!STREQU(locale, C_LOCALE) && !STREQU(locale, POSIX_LOCALE)
		    && !raw)
		    appendlist( &yopts, cptr);
	        free (cptr);
	    }
	    else
	    {
	        if ((locale = setlocale (LC_CTYPE,"")) == NULL)
		    locale = Strdup(C_LOCALE);
	        cptr = (char *) malloc(strlen(locale) + strlen(LCL_HDR) + 2);
	        sprintf(cptr, "%s%s", LCL_HDR, locale);
	        if (!STREQU(locale, C_LOCALE) && !STREQU(locale, POSIX_LOCALE)
		    && !raw)
		    appendlist( &yopts, cptr);
	        appendlist( &opts, cptr);
	    }
	}
	while (optind < argc)
	{
		fileargs++;
		file = argv[optind++];
		if(strcmp (file, "-") == 0)
		{
			stdinp++;
			appendlist (&files, file);
			continue;
		}
		(void)  procprivl (CLRPRV, DACREAD_W, MACREAD_W, (priv_t)0);

		if (Access(file, 4/*read*/) || Stat(file, &stbuf))
		{
			(void)  procprivl (SETPRV, DACREAD_W, MACREAD_W,
				(priv_t)0);
			if (Access(file, 4/*read*/) || Stat(file, &stbuf))
			{
				LP_ERRMSG2 (WARNING, E_LP_BADFILE, file,
					errno);
				exitval++;
				continue;
			}
			copy = TRUE;
		}
		(void)  procprivl (SETPRV, DACREAD_W, MACREAD_W, (priv_t)0);
		if ((stbuf.st_mode & S_IFMT) == S_IFDIR)
		{
			LP_ERRMSG1 (WARNING, E_LP_ISDIR, file);
			exitval++;
			continue;
		}
		if (stbuf.st_size == 0)
		{
			LP_ERRMSG1 (WARNING, E_LP_EMPTY, file);
			exitval++;
			continue;
		}
		if (strpbrk(file, FLIST_ESCHARS))
			escape(file, &newstr, FLIST_ESCHARS);
		else
			newstr =  file;

		if (nfiles == 0) { /* first time only */
			cptr = (char *) malloc(sizeof(FLIST) + 
				      strlen(newstr) +
				      MAX_FILE_SIZE + 3);

			sprintf(cptr, "%s'%s", FLIST, newstr);
		} else {
			cptr = (char *) malloc(strlen(newstr) + MAX_FILE_SIZE + 2);
			strcpy(cptr, newstr);
		}

		if (( optind >= argc) && (stdinp == 0))
		{
		  /* last file in the list so append trailing ' now
		   */
		  sprintf(strchr(cptr, NULL), ":%d'", stbuf.st_size); 
		}
		else
		{
		  sprintf(strchr(cptr, NULL), ":%d", stbuf.st_size); 
		}
		appendlist(&opts, cptr);
		free(cptr);

		nfiles++;
		appendlist (&files, file);
		if (newstr != file)
			free(newstr);
		continue;
	} /* while */

	if (fileargs == 0)
	{
		if (!pre_rqid)
			stdinp = 1;
	}
	else
	if (pre_rqid)
	{
		LP_ERRMSG (ERROR, E_LPP_ILLARG);
		exit(1);
	}
	else
	if (nfiles == 0 && stdinp == 0)
	{
		LP_ERRMSG (ERROR, E_LP_NOFILES);
		exit(1);
	}

    /* args parsed, now let's do it */

    startup();	/* open message queue
		   and catch interupts so it gets closed too */

    if (!(reqp = makereq())) {	/* establish defaults & sanity check args */
	LP_ERRMSG1(ERROR, E_LPP_FGETREQ, pre_rqid);
	err_exit();
    }

    /* allocate files for request, standard input and files if copy */
    if (pre_rqid) {
	if (putrequest(reqfile, reqp) == -1) {	/* write request file */
puterr:
	    switch(errno) {
	    default:
		LP_ERRMSG(ERROR, E_LPP_FPUTREQ);
		err_exit();
	    }
	}
	end_change(pre_rqid, reqp);
	reqid = pre_rqid;
    } else {
	allocfiles();
	if(stdinp > 0) {
	    insize = savestd();	/* save standard input */
	    cptr = (char *) malloc(sizeof(FLIST) + MAX_FILE_SIZE + 3);
	    if (fileargs > 1)
		sprintf(cptr, " :%d'", insize);
	    else
		sprintf(cptr, "%s':%d'", FLIST, insize);
	    appendlist(&opts, cptr);
	    reqp->options = sprintlist(opts);
	    free(cptr);
	    
	}
	reqp->file_list = files;
	if (putrequest(reqfile, reqp) == -1) goto puterr;
	reqid = que_job(reqp);
    }

    if (remove_after && orig_files) {
	char **filep;
	for (filep = orig_files; *filep; filep++) {
		/* Try to remove files.
		 * We could try determing if we have privalges to
		 * remove before trying an unlink(), but instead we
		 * let the unlink() determine if we can remove.
		 */
		if(STREQU(*filep, "-"))
			continue;
		(void)  procprivl (CLRPRV, DACWRITE_W, MACWRITE_W, (priv_t)0);
		if (Unlink(*filep)) {
			/* try with privilege */
			(void)  procprivl (SETPRV, DACWRITE_W, DACREAD_W, (priv_t)0);
			if (Unlink(*filep))
				LP_ERRMSG1(ERROR, E_LP_DSTUNK, *filep);
		}
	}
    }

    signal(SIGHUP, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    clean_up();
    ack_job();		/* issue request id message */

    return(exitval);
}
/* startup -- initialization routine */

startup()
{
#if	defined(__STDC__)
    void catch();
#else
    int catch();
#endif
int	try = 0;
    char *getcwd();

    for (;;)
	if (mopen() == 0) break;
	else {
	    if (errno == ENOSPC && try++ < 5) {
		sleep(3);
		continue;
	    }
	    LP_ERRMSG(ERROR, E_LP_MOPEN);
	    exit(1);
	}

    if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
	signal(SIGHUP, catch);
    if(signal(SIGINT, SIG_IGN) != SIG_IGN)
	signal(SIGINT, catch);
    if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
	signal(SIGQUIT, catch);
    if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
	signal(SIGTERM, catch);

    umask(0000);
    curdir = getcwd(NULL, 512);
}

/* catch -- catch signals */

#if	defined(__STDC__)
void
#endif
catch()
{
    signal(SIGHUP, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    err_exit(1);
}

/* clean_up -- called by catch() after interrupts
   or by err_exit() after errors */

static int
clean_up()
{
    (void)mclose ();
}

err_exit()
{
    clean_up();
    exit(1);
}

/*
 * copyfile(stream, name) -- copy stream to file "name"
 */

static int
copyfile(stream, name)
FILE *stream;
char *name;
{
    FILE *ostream;
    int i;
    size_t w_count = 0;
    char buf[BUFSIZ];

    if((ostream = fopen(name, "w")) == NULL) {
	LP_ERRMSG2(ERROR, E_LP_BADFILE, name, errno);
	return;
    }

    Chmod(name, 0600);
    while((i = fread(buf, sizeof(char), BUFSIZ, stream)) > 0) 
    {
	w_count = fwrite(buf, sizeof(char), i, ostream);
	if ( w_count != (size_t)i)
	{
	  /* failed to copy file to disk, disk could be full
	   */
	  LP_ERRMSG(ERROR, E_LP_MNOMEM);
	  err_exit();
	}
	if(feof(stream)) break;
    }

    fclose(ostream);
}
/* makereq -- sanity check args, establish defaults */

static REQUEST *
makereq()
{
    static REQUEST rq;
    REQUEST *oldrqp;
    char *getenv(), *preqfile;
    char **optp, *opt, buf[16], *pdest = dest, *start_ch();
    char *p;
    long errflg;

    if (!dest && !pre_rqid) {
	if (((dest = getenv("LPDEST")) || (dest = getenv("PRINTER"))) && *dest) {
	    if (!isprinter(dest) && !isclass(dest) && !STREQU(dest, NAME_ANY)) {
		LP_ERRMSG1(ERROR, E_LP_DSTUNK, dest);
		exit (1);
	    }
	}
	else {
	    if (!(dest = getdefault()) && !cont_type)
	    {
		LP_ERRMSG(ERROR, E_LPP_NODEST);
		err_exit();
	    }
	}
    }
    if (!dest) dest = "any";

    if (!pre_rqid && !cont_type)
	cont_type = getenv("LPTYPE");
    if (!pre_rqid && !cont_type)
	cont_type = NAME_SIMPLE;

    if (formname && opts)
	for (optp = opts; *optp; optp++)
	    if (STRNEQU("lpi=", *optp, 4)
	     || STRNEQU("cpi=", *optp, 4)
	     || STRNEQU("length=", *optp, 7)
	     || STRNEQU("width=", *optp, 6)) {
		LP_ERRMSG(ERROR, E_LP_OPTCOMB);
		err_exit();
	    }

    if (raw && (yopts || pages)) {
	LP_ERRMSG(ERROR, E_LP_OPTCOMB);
	err_exit();
    }

    /* now to build the request */
    if (pre_rqid) {
	preqfile = start_ch(pre_rqid);
	strcpy(reqfile, preqfile);
	if (!(oldrqp = getrequest(preqfile))) return (NULL);
	rq.copies = (copies) ? copies : oldrqp->copies;
	rq.destination = (pdest) ? dest : oldrqp->destination;
	rq.file_list = oldrqp->file_list;
	rq.form = (formname) ? formname : oldrqp->form;
	rq.actions = (specialh) ? ((specialh == HOLD) ? ACT_HOLD :
	    ((specialh == RESUME) ? ACT_RESUME : ACT_IMMEDIATE)) :
	    oldrqp->actions;
	if (wrt) rq.actions |= ACT_WRITE;
	if (mail) rq.actions |= ACT_MAIL;
	if (raw) {
	    rq.actions |= ACT_RAW;
	    /*appendlist(&opts, "stty=raw");*/
	}
	rq.options = (opts) ? sprintlist(opts) : oldrqp->options;
	rq.priority = (priority == -1) ? oldrqp->priority : priority;
	rq.pages = (pages) ? pages : oldrqp->pages;
	rq.charset = (char_set) ? char_set : oldrqp->charset;
	rq.modes = (yopts) ? sprintlist(yopts) : oldrqp->modes;
	rq.title = (title) ? title : oldrqp->title;
	rq.input_type = (cont_type) ? cont_type : oldrqp->input_type;
	rq.user = oldrqp->user;
	rq.outcome = 0;
	return(&rq);
    }
    rq.copies = (copies) ? copies : 1;
    rq.destination = dest;
    rq.form = formname;
    rq.actions = (specialh) ? ((specialh == HOLD) ? ACT_HOLD : 
	((specialh == RESUME) ? ACT_RESUME : ACT_IMMEDIATE)) : 0;
    if (wrt) rq.actions |= ACT_WRITE;
    if (mail) rq.actions |= ACT_MAIL;
    if (raw) {
	rq.actions |= ACT_RAW;
	/*appendlist(&opts, "stty=raw");*/
    }
    rq.alert = NULL;
    rq.options = sprintlist(opts);
    rq.priority = priority;
    rq.pages = pages;
    rq.charset = char_set;
    rq.modes = sprintlist(yopts);
    rq.title = title;
    rq.input_type = cont_type;
    rq.file_list = 0;
    rq.user = getname();
    return(&rq);
}

/* files -- process command line file arguments */

static int
allocfiles()
{
    char **reqfiles = 0, **filep, *p, *getfiles(), *prefix;
    FILE *f;
    int numfiles, filenum = 1;

    numfiles = 1 + ((stdinp > 0) ? 1 : 0) + ((copy) ? nfiles : 0);

    if ((prefix = getfiles(numfiles)) == NULL)
    {
	numfiles += nfiles;
	prefix = getfiles(numfiles);
	copy = 1;
    }
    
    strcpy(reqfile, prefix);
    strcat(reqfile, "-0000");
    rfilebase = makepath(Lp_Temp, reqfile, NULL);
    if (stdinp > 0) {
	stdinfile = strdup(rfilebase);
	p = strchr(stdinfile, 0) - 4;
	*p++ = '1';
	*p = 0;
	filenum++;
    }
    p = strchr(reqfile, 0) - 4; *p++ = '0'; *p = 0;
    p = strchr(rfilebase, 0) - 4;

    if (!files) appendlist(&files, "-");

    for (filep = files; *filep; filep++) {
	if(STREQU(*filep, "-")) {
	    if(stdinp > 0)
		appendlist(&reqfiles, stdinfile);
	} else {
	    if (copy)
	    {
		(void)	procprivl (CLRPRV, DACREAD_W, MACREAD_W, (priv_t)0);
		f = fopen(*filep, "r");

		if (!f)
		{
		    /*
		    **  Try opening it w/ privilege.
		    */
		    (void)  procprivl (SETPRV, DACREAD_W, MACREAD_W,
				(priv_t)0);
		    f = fopen(*filep, "r");
		    if (!f)
		    {
		    	LP_ERRMSG2(WARNING, E_LP_BADFILE, *filep, errno);
		    }
		}
		(void)	procprivl (SETPRV, DACREAD_W, MACREAD_W, (priv_t)0);
		sprintf (p, "%d", filenum++);
		copyfile (f, rfilebase);
		appendlist (&reqfiles, rfilebase);
		fclose (f);
	    }
	    else
	    {
		if (**filep == '/' || (curdir && *curdir))
		    appendlist(&reqfiles,
			(**filep == '/') ? *filep
				: makepath(curdir, *filep, (char *)0));
		else {
		    LP_ERRMSG (ERROR, E_LPP_CURDIR);
		    err_exit ();
		}
	    }
	}
    }
    if (remove_after) {
	/* remember original file list */
	orig_files = files;
    } else
    	freelist(files);
    files = reqfiles;
    return(1);
}

/* start_ch -- start change request */
static char *
start_ch(rqid)
char *rqid;
{
    int size, type;
    short status;
    char message[100],
	 reply[100],
	 *rqfile;

    size = putmessage(message, S_START_CHANGE_REQUEST, rqid);
    assert(size != -1);
    if (msend(message)) {
	LP_ERRMSG(ERROR, E_LP_MSEND);
	err_exit();
    }
    if ((type = mrecv(reply, 100)) == -1) {
	LP_ERRMSG(ERROR, E_LP_MRECV);
	err_exit();
    }
    if (type != R_START_CHANGE_REQUEST
	   || getmessage(reply, type, &status, &rqfile) == -1) {
	LP_ERRMSG1(ERROR, E_LP_BADREPLY, type);
	err_exit();
    }

    switch (status) {
    case MOK:
	return(strdup(rqfile));
    case MNOPERM:
	LP_ERRMSG(ERROR, E_LP_NOTADM);
	break;
    case MUNKNOWN:
	LP_ERRMSG1(ERROR, E_LP_UNKREQID, rqid);
	break;
    case MBUSY:
	LP_ERRMSG1(ERROR, E_LP_BUSY, rqid);
	break;
    case M2LATE:
	LP_ERRMSG1(ERROR, E_LP_2LATE, rqid);
	break;
    case MGONEREMOTE:
	LP_ERRMSG1(ERROR, E_LP_GONEREMOTE, reqid);
	break;
    default:
	LP_ERRMSG1(ERROR, E_LP_BADSTATUS, status);
    }
    err_exit();
}

static int
end_change(rqid, rqp)
char *rqid;
REQUEST *rqp;
{
    int size, type;
    long chkbits;
    short status;
    char message[100],
	 reply[100],
	 *chkp,
	 *rqfile;

    size = putmessage(message, S_END_CHANGE_REQUEST, rqid);
    assert(size != -1);
    if (msend(message)) {
	LP_ERRMSG(ERROR, E_LP_MSEND);
	err_exit();
    }
    if ((type = mrecv(reply, 100)) == -1) {
	LP_ERRMSG(ERROR, E_LP_MRECV);
	err_exit();
    }
    if (type != R_END_CHANGE_REQUEST
	   || getmessage(reply, type, &status, &chkbits) == -1) {
	LP_ERRMSG1(ERROR, E_LP_BADREPLY, type);
	err_exit();
    }

    switch (status) {
    case MOK:
	return(1);
    case MNOPERM:
	LP_ERRMSG(ERROR, E_LP_NOTADM);
	break;
    case MNOSTART:
	LP_ERRMSG(ERROR, E_LPP_NOSTART);
	break;
    case MNODEST:
	LP_ERRMSG1(ERROR, E_LP_DSTUNK, rqp->destination);
	break;
    case MDENYDEST:
	if (chkbits) {
	    int    error_ind;
	    error_ind = 0;
            chkp = message;
                /* PCK_TYPE indicates a Terminfo error, and should */
                /* be handled as a ``get help'' problem.           */
  
		/* Added logic to handle the above problem by      */
		/* isolating the first case with a separate message*/
            if (chkbits & PCK_TYPE) {
		chkp += sprintf(chkp, "");
		LP_ERRMSG(ERROR, E_LP_NOTERMINFO);
	    }
            if (chkbits & PCK_CHARSET) {
		chkp += sprintf(chkp, "-S character-set, ");
		error_ind = 1;
	    }
            if (chkbits & PCK_CPI) {
		chkp += sprintf(chkp, "-o cpi=, ");
		error_ind = 1;
	    }
            if (chkbits & PCK_LPI) {
		chkp += sprintf(chkp, "-o lpi=, ");
		error_ind = 1;
	    }
            if (chkbits & PCK_WIDTH) {
		chkp += sprintf(chkp, "-o width=, ");
		error_ind = 1;
	    }
            if (chkbits & PCK_LENGTH) {
		chkp += sprintf(chkp, "-o length=, ");
		error_ind = 1;
	    }
            if (chkbits & PCK_BANNER) {
		chkp += sprintf(chkp, "-o nobanner, ");
		error_ind = 1;
	    }
            if (chkbits & PCK_LOCALE) {
		chkp += sprintf(chkp, "-L locale (or default locale), ");
		error_ind = 1;
	    }
            chkp[-2] = 0;
	    if(error_ind == 1)
            	LP_ERRMSG1(ERROR, E_LP_PTRCHK, message);

	}
	else LP_ERRMSG1(ERROR, E_LP_DENYDEST, rqp->destination);
	break;
    case MNOMEDIA:
	LP_ERRMSG(ERROR, E_LPP_NOMEDIA);
	break;
    case MDENYMEDIA:
	if (chkbits & PCK_CHARSET) LP_ERRMSG(ERROR, E_LPP_FORMCHARSET);
	else LP_ERRMSG1(ERROR, E_LPP_DENYMEDIA, rqp->form);
	break;
    case MNOMOUNT:
	LP_ERRMSG(ERROR, E_LPP_NOMOUNT);
	break;
    case MNOFILTER:
	LP_ERRMSG(ERROR, E_LP_NOFILTER);
	break;
    case MERRDEST:
	LP_ERRMSG1(ERROR, E_LP_REQDENY, rqp->destination);
	break;
    case MNOOPEN:
	LP_ERRMSG(ERROR, E_LPP_NOOPEN);
	break;
    default:
	LP_ERRMSG1(ERROR, E_LP_BADSTATUS, status);
    }
    err_exit();
}
/* getfile -- allocate the requested number of temp files */

static char *
getfiles(number)
int number;
{
    int size, type;
    short status;
    char message[100],
	 reply[100],
	 *pfix;

    size = putmessage(message, S_ALLOC_FILES, number);
    assert(size != -1);
    if (msend(message)) {
	LP_ERRMSG(ERROR, E_LP_MSEND);
	err_exit();
    }
    if ((type = mrecv(reply, 100)) == -1) {
	LP_ERRMSG(ERROR, E_LP_MRECV);
	err_exit();
    }
    if (type != R_ALLOC_FILES
	   || getmessage(reply, type, &status, &pfix) == -1) {
	LP_ERRMSG1(ERROR, E_LP_BADREPLY, type);
	err_exit();
    }

    switch (status) {
    case MOK:
	return(strdup(pfix));
    case MOKREMOTE:
	clean_up();
	startup();
	return(NULL);
    case MNOMEM:
	LP_ERRMSG(ERROR, E_LP_NOSPACE);
	break;
    default:
	LP_ERRMSG1(ERROR, E_LP_BADSTATUS, status);
    }
    err_exit();
}

static char *
que_job(rqp)
REQUEST *rqp;
{
    int size, type;
    long chkbits;
    short status;
    char message[100],
	 reply[100],
	 *chkp,
	 *junk,
	 *req_id;

    size = putmessage(message, S_PRINT_REQUEST, reqfile);
    assert(size != -1);
    if (msend(message)) {
	LP_ERRMSG(ERROR, E_LP_MSEND);
	err_exit();
    }
    if ((type = mrecv(reply, 100)) == -1) {
	LP_ERRMSG(ERROR, E_LP_MRECV);
	err_exit();
    }
    if (type != R_PRINT_REQUEST
	   || getmessage(reply, type, &status, &req_id, &chkbits, &junk) == -1) {
	LP_ERRMSG1(ERROR, E_LP_BADREPLY, type);
	err_exit();
    }

    switch (status) {
    case MOK:
	return(strdup(req_id));
    case MNOPERM:
	LP_ERRMSG(ERROR, E_LP_NOTADM);
	break;
    case MNODEST:
	LP_ERRMSG1(ERROR, E_LP_DSTUNK, rqp->destination);
	break;
    case MDENYDEST:
	if (chkbits) {
	    int    error_ind;
	    error_ind = 0;
            chkp = message;
                /* PCK_TYPE indicates a Terminfo error, and should */
                /* be handled as a ``get help'' problem.           */
  
		/* Added logic to handle the above problem by      */
		/* isolating the first case with a separate message*/
            if (chkbits & PCK_TYPE) {
		chkp += sprintf(chkp, "");
		LP_ERRMSG(ERROR, E_LP_NOTERMINFO);
	    }
            if (chkbits & PCK_CHARSET) {
		chkp += sprintf(chkp, "-S character-set, ");
		error_ind = 1;
	    }
            if (chkbits & PCK_CPI) {
		chkp += sprintf(chkp, "-o cpi=, ");
		error_ind = 1;
	    }
            if (chkbits & PCK_LPI) {
		chkp += sprintf(chkp, "-o lpi=, ");
		error_ind = 1;
	    }
            if (chkbits & PCK_WIDTH) {
		chkp += sprintf(chkp, "-o width=, ");
		error_ind = 1;
	    }
            if (chkbits & PCK_LENGTH) {
		chkp += sprintf(chkp, "-o length=, ");
		error_ind = 1;
	    }
            if (chkbits & PCK_BANNER) {
		chkp += sprintf(chkp, "-o nobanner, ");
		error_ind = 1;
	    }
            if (chkbits & PCK_LOCALE) {
		chkp += sprintf(chkp, "-L locale (or default locale), ");
		error_ind = 1;
	    }
            chkp[-2] = 0;
	    if(error_ind == 1)
            	LP_ERRMSG1(ERROR, E_LP_PTRCHK, message);

	}
	else LP_ERRMSG1(ERROR, E_LP_DENYDEST, rqp->destination);
	break;
    case MNOMEDIA:
	LP_ERRMSG(ERROR, E_LPP_NOMEDIA);
	break;
    case MDENYMEDIA:
	if (chkbits & PCK_CHARSET) LP_ERRMSG(ERROR, E_LPP_FORMCHARSET);
	else LP_ERRMSG1(ERROR, E_LPP_DENYMEDIA, rqp->form);
	break;
    case MNOMOUNT:
	LP_ERRMSG(ERROR, E_LPP_NOMOUNT);
	break;
    case MNOFILTER:
	LP_ERRMSG(ERROR, E_LP_NOFILTER);
	break;
    case MERRDEST:
	LP_ERRMSG1(ERROR, E_LP_REQDENY, rqp->destination);
	break;
    case MNOOPEN:
	LP_ERRMSG(ERROR, E_LPP_NOOPEN);
	break;
    case MUNKNOWN:
	LP_ERRMSG(ERROR, E_LPP_ODDFILE);
	break;
    default:
	LP_ERRMSG1(ERROR, E_LP_BADSTATUS, status);
    }
    err_exit();
}
/* ack_job -- issue request id message */

static int
ack_job()
{
    if(silent || pre_rqid) return;
    if(nfiles > 1) {
        if (stdinp > 0)
           LP_OUTMSG2(MM_NOSTD, E_LP_REQID6, reqid, nfiles);
        else
           LP_OUTMSG2(MM_NOSTD, E_LP_REQID5, reqid, nfiles);
    }
    else {
        if (nfiles > 0) {
           if (stdinp > 0)
              LP_OUTMSG1(MM_NOSTD, E_LP_REQID4, reqid);
           else
              LP_OUTMSG1(MM_NOSTD, E_LP_REQID3, reqid);
           }
           else {
              if (stdinp > 0)
                 LP_OUTMSG1(MM_NOSTD, E_LP_REQID2, reqid);
              else
                 LP_OUTMSG1(MM_NOSTD, E_LP_REQID1, reqid);
           }
    }
}
/* savestd -- save standard input */

static int
savestd()
{
    copyfile(stdin, stdinfile);
    Stat(stdinfile, &stbuf);
    if(stbuf.st_size == 0) {
	if(nfiles == 0) {
	    LP_ERRMSG(ERROR, E_LP_NOFILES);
	    err_exit();
	} else	{/* inhibit printing of std input */
	    LP_ERRMSG1(WARNING, E_LP_EMPTY, "(standard input)");
	    stdinp = -1;
	}
    }
	return (stbuf.st_size);
}


/* ck_mount() - check if ETCDIR and SPOOLDIR are mounted resources - 
   if so then return TRUE, so that the -c option will be
   turned on by default.  
*/

static void
ck_mount() 
{

	FILE *fp;
	struct stat ebuf;
	struct stat vbuf;
	int eflag = 0;
	int vflag = 0;
	int elen, vlen;
	char buffer[BUFSIZ];
	char *bp = buffer;

	if ((fp = fopen(FSTYPES,"r")) == NULL)
		return;
	else 
	{
		Stat(ETCDIR,&ebuf);
		Stat(SPOOLDIR,&vbuf);
		elen = strlen(ebuf.st_fstype);
		vlen = strlen(vbuf.st_fstype);
		while (fgets(bp,BUFSIZ,fp) != NULL)
		{
		     
	       	     if ((strncmp(bp,ebuf.st_fstype,elen) == 0) && (*(bp + elen) == '	'))
				eflag++;
			
	              if ((strncmp(bp,vbuf.st_fstype,vlen) == 0) && (*(bp + vlen) == '	'))
				vflag++;
		      if (vflag && eflag){
			    copy = TRUE;
			    break;
		      }
		}
	}
	return;
}

static void
#if defined(__STDC__)
escape(char *old, char **new, char *esc)
#else
escape(old, new, esc)
char *old, **new, *esc;
#endif
{
	char	*p;

	p = (char *) malloc(2*strlen(old) + 1);	/* maximum sized string */
	canonize(p, old, esc);
	*new = p;
}
