/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:xx/main.c	1.54.3.13"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <curses.h>
#include <term.h>
#include <pfmt.h>

#include "inc.types.h"		/* abs s14 */
#ifndef PRE_CI5_COMPILE		/* abs s14 */
#include <locale.h>
#endif				/* abs s14 */
#include "wish.h"
#include "token.h"
#include "actrec.h"
#include "terror.h"
#include "ctl.h"
#include "vtdefs.h"
/*  #include "status.h"  empty  include file abs 9/14/88 */
#include "moremacros.h"
#include "sizes.h"
#include "locale.h"


/*
 * External Globals
 */
int  Vflag = 0;		/* Set if running FACE User Interface */
pid_t Fmli_pid;		/* Process id of this FMLI.  EFT abs k16 */
/* the path of the FIFO for process synchronization */
char Semaphore[PATHSIZ] = "/tmp/FMLIsem.";	/* for asynch coprocs */
/* a flag that indicates interuption of the suspend process */
bool Suspend_interupt;
/* the name of the object to make current when suspend is terminated */
char *Suspend_window;
/* a pointer to the name of the file containing path aliases */
char *Aliasfile;	/* File of path aliases */
int ShellInterrupt = FALSE;
extern char	*Home;
extern char *tigetstr();	/* curses routine */
extern long strtol();		/* abs k16 */
extern pid_t  getpid();		/* EFT abs k18 */
extern void   mess_init();	/* abs k18 */

/*
 * Defines for SLK layout (4-4 or 3-2-3 layout can be specified in curses)
 */
#define FOURFOUR	"4-4"
#define LAYOUT_DESC	"slk_layout"

/*
 * Default FMLI Stream handler
 */
token physical_stream();
token virtual_stream();
token actrec_stream();
token global_stream();
token menu_stream();
token error_stream();

/* dmd TCB */
static token (*Defstream[])() = {
	physical_stream,
	virtual_stream,
	actrec_stream,
	global_stream,
	error_stream,
	NULL
};

extern char	*Progname;

/* Status line variables */
/* the interval in seconds for the mailcheck interupt */
long	Mail_check;
/* the name of the current user's mail file */
char	*Mail_file;
/* the current time for checking to see if MAILCHECK seconds has elapsed */
time_t	Cur_time;	/* EFT abs k16 */

#ifdef SIGTSTP
void on_suspend();
#endif

/*
 * Static globals
 */
static char Interpreter[] = "fmli";	
static char Viewmaster[] = "face";	
static char Vpid[12] = "VPID=";
static char Fcolor[17] = "HAS_COLORS=";
static char Display_H[16] = "DISPLAYH=";
static char Display_W[16] = "DISPLAYW=";

static void vm_initobjects();
static void vm_setup();
static void sig_catch();
/* static int sig_catch();   abs */



main(argc, argv)
int argc;
char *argv[];
{
    register int	i, r, c;
    static	char mail_template[256] = "/usr/mail/";
    token	t;
    extern	char *optarg;
    extern	int optind;
    char	*initfile, *commfile, *aliasfile, *p, *pidstr;
    int	labfmt, errflg;
    vt_id	vid, copyright();
    char	*itoa(), *estrtok(), *path_to_full();
    char	*filename(), *getenv(), *strnsave();
    void	susp_res(), vinterupt(), screen_clean();
    time_t  time();		/* EFT abs k16 */
    extern int vt_redraw();
    extern  void sig_err_msg();	/* abs s13 */
    extern  void sig_exit();	/* abs s13 */
    extern  void restore_pfk();	/* abs s14 */
    extern  void exit();	/* abs 9/12/88 */

    struct	actrec *firstar, *wdw_to_ar();

    /* LES: optimize malloc(3X) */

    /* set up local options for character-set and date formats abs s12 */
#ifndef PRE_CI5_COMPILE		 			    /* abs s14 */
    setcat ("uxfmli");

    (void)setlabel("UX:fmli");

    
    setlocale (LC_ALL, "");

#endif							    /* abs s14 */
    /* initialize the slk structures to translate labels */
    /* init_I18n_slk() is defined in oh/slk.c */
    init_I18n_slk ();
    /* init_I18n_partab() is defined in oh/partabfunc.c */
    init_I18n_partab ();

    setlocale (LC_ALL, "");

    if ((p = getenv("VMFMLI")) && strcmp(p, "true") == 0)
    {
	Vflag++;
	putenv("VMFMLI=false");
    }
    errflg = 0;
    Progname = filename(argv[0]);
    Aliasfile = initfile = commfile = (char *) NULL;
    while ((c = getopt(argc, argv, "i:c:a:")) != EOF)
    {
	switch(c)
	{
	case 'i':
	    initfile = optarg;
	    break; 
	case 'c':
	    commfile = optarg;
	    break; 
	case 'a':
	    Aliasfile = strsave(optarg);
	    break;
	case '?':
	    errflg++;
	    break;
	}
    }

    /*
     * Check arguments
     */
    if (errflg || optind == argc) 		/* abs s18 */
    {
	pfmt (stderr, MM_ERROR, 
		":346:usage: \n\n%s [-i init-file] [-c command-file] [-a alias-file] frame-definition-file ...\n\n", argv[0]);

	pfmt (stderr, MM_INFO, 
		":347:The Form and Menu Language Interpreter (fmli) is a tool\nfor providing a user interface to your application.\n"); 	/* abs s18 */
	exit(-1);               /* this is fmli's exit not the C library call */
    }
/**
**  if (errflg)
**  {
**	fprintf(stderr, "usage: %s [-i initfile] [-c commandfile] [-a aliasfile] object ...\n", argv[0]);
**
**	exit(-1);		
**  }
**
** 	if (optind == argc) {
** 		fprintf(stderr, "Initial object must be specified\n");
** 		exit(-1); 
** 	}
** abs s18
**/
    /* 
     * check that all objects exist AND that they are readable
     */
    if (initfile && access(initfile, 4) < 0)
    {
	errflg++;
	pfmt (stderr, MM_ERROR, 
		":348:Can't open initialization file \"%s\"\n", initfile); 
    }
    if (commfile && access(commfile, 4) < 0)
    {
	errflg++;
	pfmt (stderr, MM_ERROR, 
		":349:Can't open commands file \"%s\"\n", commfile); 
    }
    if (Aliasfile && access(Aliasfile, 4) < 0)
    {
	errflg++;
	pfmt (stderr, MM_ERROR, 
		":350:Can't open alias file \"%s\"\n", Aliasfile); 
    }
    for (i = optind; i < argc; i++)
    {
	if (access(argv[i], 4) < 0)
	{
	    errflg++;
	    /* changed object to frame definition file abs s18 */
	    pfmt (stderr, MM_ERROR, ":351:Can't open frame definition file: \"%s\"\n",
		    argv[i]); 	
	}
    }
    if (errflg)
	exit(-1);		/* this is fmli's exit not the C library call */
		
    /*
     * handle signals
     *
     * changed signal()'s to sigset()'s..   abs
     */

    if (sigset(SIGINT, SIG_IGN) != SIG_IGN)
	sigset(SIGINT, sig_exit); 		/* abs s13 */
    if (sigset(SIGHUP, SIG_IGN) != SIG_IGN)
	sigset(SIGHUP, sig_exit); 		/* abs s13 */
    if (sigset(SIGTERM, SIG_IGN) != SIG_IGN)
	sigset(SIGTERM, sig_exit); 		/* abs s13 */
    (void) sigset(SIGALRM, sig_catch);

    /* for job control: want to reset tty before suspending and.. */
#ifdef SIGTSTP			/* .. cleanup after. */
    (void) signal(SIGTSTP, on_suspend); /* must be signal not sigset */
    (void) signal(SIGTTIN, on_suspend); /* must be signal not sigset */
    (void) signal(SIGTTOU, on_suspend); /* must be signal not sigset */
#endif

    /* catch the rest of the signals that cause termination
     * so we can reset the terminal before terminating.  abs s13
     */

    if (sigset(SIGQUIT, SIG_IGN) != SIG_IGN)
	sigset(SIGQUIT, sig_exit); 	
    sigset(SIGILL, sig_exit); 		
#ifdef SIGABRT
    if (sigset(SIGABRT, SIG_IGN) != SIG_IGN)
	sigset(SIGABRT, sig_exit); 	
#else
    if (sigset(SIGIOT, SIG_IGN) != SIG_IGN)
	sigset(SIGIOT, sig_exit);  	
#endif
    sigset(SIGEMT, sig_exit);
    sigset(SIGFPE, sig_exit);
    sigset(SIGBUS, sig_exit);
/*    sigset(SIGSEGV, sig_exit); */
    sigset(SIGSYS, sig_exit);	
    if (sigset(SIGPIPE, SIG_IGN) != SIG_IGN)
	sigset(SIGPIPE, sig_exit);
#ifdef SIGXCPU
    sigset(SIGXCPU, sig_exit); 
#endif
#ifdef SIGXFSZ
    sigset(SIGXFSZ, sig_exit); 	
#endif

    /*
     * initialize terminal/screen
     */
    labfmt = 0;
    if (initfile) {
	/*
	 * Set up SLK layout as 4-4 or 3-2-3 ... This needs
	 * to be determined BEFORE curses is initialized
	 * in vt_init.
	 */
	char *slk_layout, *get_desc_val();

	slk_layout = get_desc_val(initfile, LAYOUT_DESC); 
	if (strcmp(slk_layout, FOURFOUR) == 0)
	    labfmt = 1;

    }
    vt_init(labfmt);
    onexit(screen_clean);
    onexit(sig_err_msg);	/* abs s13 */
    onexit(restore_pfk);	/* abs s14 */

    /*
     * indicate in the environment if the terminal we just
     * initialized has color capability or not         --dmd
     * ..also indicate the size of the display area.   --abs
     */
    if ( has_colors() )
	strcat(Fcolor, "true");
    else
	strcat(Fcolor, "false");
    putenv(Fcolor);
    strcat(Display_W, itoa((long)COLS, 10)); 			/* abs k16 */
    strcat(Display_H, itoa((long)LINES - RESERVED_LINES, 10)); 	/* abs k16 */
    putenv(Display_W);
    putenv(Display_H);

    /*
     * Read defaults from initialization file
     */
    setup_slk_array();
    if (initfile)
	read_inits(initfile);

    /*
     * set up default banner and working indicator strings ...
     * and set color attributes if terminal supports color
     */
    /* Changed the order of next two calls.
     * First set colors and after it the baner line.    mek k17
     */
    set_def_colors();
    set_def_status();

    /*
     * Define screen function keys for terminals that do not have
     * them defined like att630.  mek k17
     * Must be After sigset(SIGALRM...).  abs k18
     */
    init_sfk(TRUE);

    /*
     * Put up an Introductory Object and the working indicator
     */
    vid = copyright();
    working(TRUE);
    if (vid)
	vt_close(vid);

    /* remove this ifdef and sigset when i386 sleep(2C) is fixed to reset the
     * SIGALRM signal handler.  abs k18
     */
    /* Looks like i386 sleep(2C) problem is fixed. Hence commenting the ifdef
       and SIGALARM signal handlet.  ck p7 */
/* #ifdef i386
    (void) sigset(SIGALRM, sig_catch);    copied from above. abs k18   
#endif */
    /*
     * Initialize command table...
     * Read commands from commands file
     */
    cmd_table_init();
    if (commfile)
	read_cmds(commfile);

    /*
     * Set up FACE globals
     */
    if (Vflag)
	vm_setup();
    else
	Home = NULL;

    /*
     * Set mailcheck (use mail_file variable as temp
     * variable for MAILCHECK)
     */
    Mail_check = 0L;
    if (Mail_file = getenv("MAILCHECK"))
	Mail_check = (long) atoi(Mail_file);
    if (Mail_check == 0L)
	Mail_check = 300L;
    else
	Mail_check = max(Mail_check, 120L);
    if ((Mail_file = getenv("MAIL")) == NULL)
    {
	char *ptr;

	if (ptr = getenv("LOGNAME")) {
		Mail_file = mail_template;
		strcat(Mail_file, ptr);
	}
    }
    Cur_time = time((time_t)0L); /* EFT abs k16 */

    /*
     * Initialize object architecture (SORTMODE)
     */
    oh_init();

    /*
     * set VPID env variable to pid for suspend/resume (SIGUSR1)
     * and asynchronous activity (SIGUSR2)
     */
    Fmli_pid = getpid();
    pidstr = itoa((long)Fmli_pid, 10);
    strcat(Vpid, pidstr);
    putenv(Vpid);
    strcat(Semaphore, pidstr);	/* for enhanced asynch coprocs */
    /* changed from signal()... abs */
    sigset(SIGUSR1, susp_res);	/* set sigs for suspend/resume */
    sigset(SIGUSR2, vinterupt);	/* set sigs for interactive windows */

    /*
     * Setup windows to be opened upon login
     */
    for (i = optind; i < argc; i++)
    {
	objop("OPEN", NULL, p = path_to_full(argv[i]), NULL);
	free(p);
	ar_ctl(ar_get_current(), CTSETLIFE, AR_INITIAL);
    }

    /*
     * make the first window current
     */

    if ((argc - optind) >= 1)
    {
	if (firstar = wdw_to_ar(1))
	{
	    /*
	     * clean-up messages left over by opening initial objects
	     */
	 
	    mess_init();		/* abs k18 */
	    ar_current(firstar, FALSE); /* abs k15 */
	}
	else
	{
	    mess_flush(FALSE);
	    vt_flush();
	    sleep(3);
	    exit(-1);		/* fmli's exit not the C library call */
	}
    }

    /*
     * Check wastebasket 
     */
    if (Vflag)
    {
	vm_initobjects();
	showmail(TRUE);
    }

    while ((t = stream(TOK_NOP, Defstream)) != TOK_LOGOUT)
	;
    exit(0);			/* fmli's exit not the C library call */
}

static bool Suspend_allowed = TRUE;

bool
suspset(b)
bool b;
{
	bool old = Suspend_allowed;

	Suspend_allowed = b;
	return(old);
}

/* a flag that indicates that a SIGUSR2 interupt is pending */
long Interupt_pending = 0;

static void
vinterupt(sig)
int sig;
{
	(void) sigset(sig, vinterupt); /* changed from signal() abs */
	Interupt_pending++;
	return;
}
 
static void
susp_res(sig)
int sig;
{
	char *p;	/* temp for some operations */
	char buf[BUFSIZ];
	pid_t  respid;		/* EFT abs k16 */

	FILE *fp;

	char *getepenv();

	(void) sigset(sig, susp_res);  /* changed from signal  abs */

	sprintf(buf, "/tmp/suspend%ld", getpid());
	if ((fp = fopen(buf, "r")) == NULL) {		/* resume failed */
		_debug(stderr, "Unable to open resume file\n");
		return;
	}

	(void) unlink(buf);
	if (fgets(buf, BUFSIZ, fp) == NULL) {
		_debug(stderr, "could not read resume file");
	} else {
		respid = strtol(buf, (char **)NULL, 0);	/* EFT abs k16 */
		if (!Suspend_allowed) {
			fflush(stdout);
			fflush(stderr);
			pfmt (stdout, MM_INFO, 
				":352:\r\n\nYou are not allowed to suspend at this time.\r\n");
			pfmt (stdout, MM_INFO, 
				":353:You are in the process of logging out of FACE.\r\n");
			pfmt (stdout, MM_INFO,
				":354:Please take steps to end the application program you are\n\rcurrently using.\n\r");
				
			fflush(stdout);
			sleep(4);
			if (kill(respid, SIGUSR1) == FAIL)
				Suspend_interupt = TRUE;
			fclose(fp);
			return;
		}

		if (fgets(buf, BUFSIZ, fp) != NULL) {
			buf[(int)strlen(buf) - 1] = '\0';
			if (buf[0])
				Suspend_window = strsave(buf);
			else
				Suspend_window = NULL;
		} else
			Suspend_window = NULL;
		_debug(stderr, "Resume pid is %d\n", respid);
		ar_ctl(ar_get_current(), CTSETPID, respid);
	}
	fclose(fp);

	Suspend_interupt = TRUE;	/* let wait loop in proc_current know */
	return;
}
	
/* static int    changed to void abs 9/12/88  */
static void
sig_catch(n)
int	n;
{
/*	signal(n, sig_catch);
abs */
        sigset(n, sig_catch);
}


void
pull()		/* force various library routines to get pulled in */
{
	menu_stream();
	actrec_stream();
	error_stream();
	virtual_stream();
	indicator();
}

/* the current working directory before it was last changed */
char 		Opwd[PATHSIZ+5];
extern char	*Filecabinet;
extern char	*Wastebasket;
extern char	*Filesys;
extern char	*Oasys;

static void
vm_setup()
{
	static	char filecabinet[] = "";
	static	char wastebasket[] = "/WASTEBASKET";
	char *p, *getenv(), *getepenv();

	/*
	 * get/set globals
	 */
	if ((Home = getenv("HOME")) == NULL)
		fatal(MUNGED, "HOME!");
	if ((Filesys = getenv("VMSYS")) == NULL)
		fatal(MUNGED, "VMSYS");
	if ((Oasys = getenv("OASYS")) == NULL)
		fatal(MUNGED, "OASYS");
	Filecabinet = strnsave(Home, (int)strlen(Home) + sizeof(filecabinet) - 1);
	strcat(Filecabinet, filecabinet);
	Wastebasket = strnsave(Home, (int)strlen(Home) + sizeof(wastebasket) - 1);
	strcat(Wastebasket, wastebasket);

	sprintf(Opwd, "OPWD=%s", Filecabinet);
	putenv(Opwd);
	if (p = getepenv("UMASK"))		/* set file masking */
		umask((mode_t) strtol(p, NULL, 8)); /* EFT abs k16 */
}

#define MAX_WCUST (9)
static char	Loginwins[] = "LOGINWIN ";	/* leave space for a digit */

static void
vm_initobjects()
{
    char	*p;
    register int i;
    char *path_to_full(), *getepenv();
    struct actrec *prev, *firstobj;

    prev = NULL;
    firstobj = ar_get_current();
    for (i = 1; i <= MAX_WCUST; i++) {
	Loginwins[sizeof(Loginwins) - 2] = '0' + i;

	if ((p = getepenv(Loginwins)) != NULL && *p) {
	    p = path_to_full(p);
	    if (prev)		/* aids ordering */
		ar_current(prev, FALSE); /* abs k15 */
	    objop("OPEN", NULL, p, NULL);
	    if (firstobj != (prev = ar_get_current()))
		ar_ctl(prev, CTSETLIFE, AR_PERMANENT);
	    free(p);
	    ar_current(firstobj, FALSE); /* aids ordering *//*abs k15*/
	}
    }

    /* clean out WASTEBASKET, if needed */
    if (p = getepenv("WASTEPROMPT")) {
	objop("OPEN", "MENU", strCcmp(p, "yes") ?
	      "$VMSYS/OBJECTS/Menu.remove" :
	      "$VMSYS/OBJECTS/Menu.waste", NULL);
	free(p);
    }
    else
	objop("OPEN", "MENU", "$VMSYS/OBJECTS/Menu.remove", NULL);
}


/* Signal handler for SIGTSTP, the user job control suspend signal.
 * Signal handler for SIGTTIN and SIGTTOU, the terminal input and
 * terminal output signals.
 * Clear the screen and then do the default action for the signal.
 * On return, redraw the screen.
 */
#ifdef SIGTSTP
void 
on_suspend(signum)
int signum;
{
    endwin();		  /* reset tty to normal */
    kill(getpid(), signum);

    /* start back here when resumed after job control suspend */

    signal(signum, on_suspend); /* reset the signal */

    (void)putchar('\0');  /* output something innocent so we will.. */
                          /* ..suspend on o/p if running in background*/
    if (ShellInterrupt == FALSE)
    	vt_redraw();	  /* refresh the screen */
    set_mouse_info();	  /* Set the mouse information */
}
#endif
