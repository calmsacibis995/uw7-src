/*		copyright	"%c%" 	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/* Copyright (c) 1981 Regents of the University of California */
#ident	"@(#)vi:port/ex.c	1.28.1.15"
#ident  "$Header$"
#include <pfmt.h>
#include "ex.h"
#include "ex_argv.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include <locale.h>
#include <fcntl.h>
#ifdef TRACE
char	tttrace[]	= { '/','d','e','v','/','t','t','y','x','x',0 };
#endif

#define EQ(a,b)	!strcmp((const char *)a, (const char *)b)

#define CMDCLASS        "UX:"   /* Command classification */

char	*strrchr();

/*
 * The code for ex is divided as follows:
 *
 * ex.c			Entry point and routines handling interrupt, hangup
 *			signals; initialization code.
 *
 * ex_addr.c		Address parsing routines for command mode decoding.
 *			Routines to set and check address ranges on commands.
 *
 * ex_cmds.c		Command mode command decoding.
 *
 * ex_cmds2.c		Subroutines for command decoding and processing of
 *			file names in the argument list.  Routines to print
 *			messages and reset state when errors occur.
 *
 * ex_cmdsub.c		Subroutines which implement command mode functions
 *			such as append, delete, join.
 *
 * ex_data.c		Initialization of options.
 *
 * ex_get.c		Command mode input routines.
 *
 * ex_io.c		General input/output processing: file i/o, unix
 *			escapes, filtering, source commands, preserving
 *			and recovering.
 *
 * ex_put.c		Terminal driving and optimizing routines for low-level
 *			output (cursor-positioning); output line formatting
 *			routines.
 *
 * ex_re.c		Global commands, substitute, regular expression
 *			compilation and execution.
 *
 * ex_set.c		The set command.
 *
 * ex_subr.c		Loads of miscellaneous subroutines.
 *
 * ex_temp.c		Editor buffer routines for main buffer and also
 *			for named buffers (Q registers if you will.)
 *
 * ex_tty.c		Terminal dependent initializations from termcap
 *			data base, grabbing of tty modes (at beginning
 *			and after escapes).
 *
 * ex_unix.c		Routines for the ! command and its variations.
 *
 * ex_v*.c		Visual/open mode routines... see ex_v.c for a
 *			guide to the overall organization.
 */

/*
 * This sets the Version of ex/vi for both the exstrings file and
 * the version command (":ver"). 
 */

char *Version = "SVR4.2";	/* variable used by ":ver" command */

/*
 * NOTE: when changing the Version number, it must be changed in the
 *  	 following files:
 *
 *			  port/READ_ME
 *			  port/ex.c
 *			  port/ex.news
 *
 */

/*
 * Main procedure.  Process arguments and then
 * transfer control to the main command processing loop
 * in the routine commands.  We are entered as either "ex", "edit", "vi"
 * or "view" and the distinction is made here. For edit we just diddle options;
 * for vi we actually force an early visual command.
 */
static unsigned char cryptkey[19]; /* contents of encryption key */
static char posix_var[] = "POSIX2";

main(ac, av)
	register int ac;
	register unsigned char *av[];
{
	extern 	char 	*optarg;
	extern 	int	optind;
	unsigned char	*rcvname = 0;
	unsigned char	usage[80];
        char label[MAXLABEL+1];		/* Space for the catalogue label */
	register unsigned char *cp;
	register int c;
	unsigned char	*cmdnam;
	bool recov = 0;
	bool ivis = 0;
	bool itag = 0;
	bool fast = 0;
	extern int verbose;
	unsigned char *command = NULL;	/* -c and '+' option-argument */
#ifdef TRACE
	char tracef[40];
#endif
	(void)setlocale(LC_ALL, "");
	(void)setcat("uxed.abi");

	/*
	 * Immediately grab the tty modes so that we won't
	 * get messed up if an interrupt comes in quickly.
	 */
	gTTY(2);
	normf = tty;
	ppid = getpid();
	/* Note - this will core dump if you didn't -DSINGLE in CFLAGS */
	lines = 24;
	columns = 80;	/* until defined right by setupterm */
	/*
	 * Defend against d's, v's, w's, and a's in directories of
	 * path leading to our true name.
	 */
	if ((cmdnam = (unsigned char *)strrchr((const char *)av[0], '/')) != 0)
		cmdnam++;
	else
		cmdnam = av[0];

	if (getenv(posix_var) != NULL)
		value(vi_RECOMPAT) = 0;

        (void)strcpy(label, CMDCLASS);
        (void)strncat(label, (char*) cmdnam,
        	(MAXLABEL - sizeof(CMDCLASS) - 1));
        (void)setlabel(label);

	if (EQ(cmdnam, "vi")) 
		ivis = 1;
	else if (EQ(cmdnam, "view")) {
		ivis = 1;
		value(vi_READONLY) = 1;
	} else if (EQ(cmdnam, "vedit")) {
		ivis = 1;
		value(vi_NOVICE) = 1;
		value(vi_REPORT) = 1;
		value(vi_MAGIC) = 0;
		value(vi_SHOWMODE) = 1;
	} else if (EQ(cmdnam, "edit")) {
		value(vi_NOVICE) = 1;
		value(vi_REPORT) = 1;
		value(vi_MAGIC) = 0;
		value(vi_SHOWMODE) = 1;
	}

	draino();
	pstop();

	/*
	 * Initialize interrupt handling.
	 */
	oldhup = signal(SIGHUP, SIG_IGN);
	if (oldhup == SIG_DFL)
		signal(SIGHUP, onhup);
	oldquit = signal(SIGQUIT, SIG_IGN);
	ruptible = signal(SIGINT, SIG_IGN) == SIG_DFL;
	if (signal(SIGTERM, SIG_IGN) == SIG_DFL)
		signal(SIGTERM, onhup);
	if (signal(SIGEMT, SIG_IGN) == SIG_DFL)
		signal(SIGEMT, onemt);
	signal(SIGILL, oncore);
	signal(SIGTRAP, oncore);
	signal(SIGIOT, oncore);
	signal(SIGFPE, oncore);
	signal(SIGBUS, oncore);
	signal(SIGSEGV, oncore);
	signal(SIGPIPE, oncore);
	while(1) {
#ifdef TRACE
		while ((c = getopt(ac,(char **)av,"VS:Lc:Tvt:rlw:xRCs")) != EOF)
#else
		while ((c = getopt(ac,(char **)av,"VLc:vt:rlw:xRCs")) != EOF)
#endif
			switch(c) {
			case 's':
				hush = 1;
				value(vi_AUTOPRINT) = 0;
				fast++;
				break;
				
			case 'R':
				value(vi_READONLY) = 1;
				break;
#ifdef TRACE
			case 'T':
				strcpy((char *)tracef, "trace");
				goto trace;
			
			case 'S':
				strcpy((char *)tracef, tttrace);
				strcat((char *)tracef,(const char *)optarg);	
		trace:			
				trace = fopen((char *)tracef,"w");
#define	tracebuf	NULL
				if (trace == NULL)
					pfmt(stdout, MM_ERROR, 
						":1:Trace create error on \"%s\": %s\n",
						tracef, strerror(errno));
				else
					setbuf(trace, (char *)tracbuf);
				break;
#endif
			case 'c':
				if (command == NULL) {
					long am = sysconf(_SC_ARG_MAX);

					if (am <= 0)
						am = ARG_MAX;

					if ((command = malloc(am)) == NULL){
						(void)pfmt(stderr,MM_ERROR,
							":285:Malloc failed:%s\n",
							strerror(errno));
						exit(1);
					}
					strcpy(command, optarg);
				} else
					sprintf(command+strlen(command),"|%s",
						optarg);
			case 'l':
				value(vi_LISP) = 1;
				value(vi_SHOWMATCH) = 1;
				break;

			case 'r':
				if(av[optind] && (c = av[optind][0]) && c != '-') {
					rcvname = (unsigned char *)av[optind];
					optind++;
				}
		
			case 'L':
				recov++;
				break;

			case 'V':
				verbose = 1;
				break;

			case 't':
				itag = 1;
				strncpy(lasttag, optarg,TAGSIZE-1);
				lasttag[TAGSIZE-1]='0';
				break;
					
			case 'w':
				defwind = 0;
				if (optarg[0] == NULL)
					defwind = 3;
				else for (cp = (unsigned char *)optarg; isdigit(*cp); cp++)
					defwind = 10*defwind + *cp - '0';
				break;
					
			case 'C':
				crflag = 1;
				xflag = 1;
				break;

			case 'x':
				/* 
				 * encrypted mode
				 */
				xflag = 1;
				crflag = -1;
				break;
				
			case 'v':
				ivis = 1;
				break;

			default:
				pfmt(stderr, MM_ERROR, ":2:Incorrect usage\n");
				if (ivis) {
#ifdef TRACE
					pfmt(stderr, MM_ACTION,
						":273:Usage: %s [-t tag] [-r [file]] [-l] [-L] [-wn] [-R] [-T [-S suffix]]\n",
						cmdnam);
#else
					pfmt(stderr, MM_ACTION,
						":274:Usage: %s [-t tag] [-r [file]] [-l] [-L] [-wn] [-R]\n",
						cmdnam);
#endif
					pfmt(stderr, MM_NOSTD,
						":275:       [-x] [-C] [+cmd | -c cmd] file...\n");
				}
				else if (EQ(cmdnam, "edit")) {
#ifdef TRACE
					pfmt(stderr, MM_ACTION,
						":276:Usage: %s [-r [file]] [-x] [-C] [-T [-S suffix]]\n", cmdnam);
#else
					pfmt(stderr, MM_ACTION,
						":277:Usage: %s [-r [file] [-x] [-C]\n", cmdnam);
#endif
				}
				else {
#ifdef TRACE
					pfmt(stderr, MM_ACTION,
						":278:Usage: %s [-s] [-v] [-t tag] [-r [file]] [-L] [-R] [-T [-S suffix]]\n",
						cmdnam);
#else
					pfmt(stderr, MM_ACTION,
						":279:Usage: %s [-s] [-v] [-t tag] [-r [file]] [-L] [-R]\n",
						cmdnam);
#endif
					pfmt(stderr, MM_NOSTD,
						":275:       [-x] [-C] [+cmd | -c cmd] file...\n");
				}
				exit(1);
			}
		if(av[optind] && av[optind][0] == '+' && strcmp((const char *)av[optind-1],"--")) {
			if (command == NULL) {
				long am = sysconf(_SC_ARG_MAX);

				if (am <= 0)
					am = ARG_MAX;

				if ((command = malloc(am)) == NULL){
					(void)pfmt(stderr,MM_ERROR,
						":285:Malloc failed:%s\n",
						strerror(errno));
					exit(1);
				}
				strcpy(command, &av[optind][1]);
			} else
				sprintf(command+strlen(command),"|%s",
					&av[optind][1]);
			optind++;
			continue;
		} else if(av[optind] && av[optind][0] == '-'  && strcmp((const char *)av[optind-1], "--")) {
			hush = 1;
			value(vi_AUTOPRINT) = 0;
			fast++;
			optind++;
			continue;
		}
		break;
	}
	ac -= optind;
	av  = &av[optind];
					
#ifdef SIGTSTP
	if (!hush && signal(SIGTSTP, SIG_IGN) == SIG_DFL)
		signal(SIGTSTP, onsusp), dosusp++;
#endif

	if(xflag) {
		permflag = 1;
		if ((kflag = run_setkey(perm, (key = (unsigned char *)
			getpass(gettxt(EXmenterkeyid, EXmenterkey))))) == -1) {
			kflag = 0;
			xflag = 0;
			smerror(EXmnocryptid, EXmnocrypt);
			flush();
			if (ivis) {
				sleep(2);
			}
		}
		if(kflag == 0)
			crflag = 0;
		else {
			strcpy((char *)cryptkey, "CrYpTkEy=XXXXXXXXX");
			strcpy((char *)cryptkey + 9, (const char *)key);
			if(putenv((char *)cryptkey) != 0)
			smerror(EXmnocopykeyid, EXmnocopykey);
			flush();
			if (ivis) {
				sleep(2);
			}
		}

	}

	/*
	 * Initialize end of core pointers.
	 * Normally we avoid breaking back to fendcore after each
	 * file since this can be expensive (much core-core copying).
	 * If your system can scatter load processes you could do
	 * this as ed does, saving a little core, but it will probably
	 * not often make much difference.
	 */
	fendcore = (line *) sbrk(0);
	endcore = fendcore - 2;
	
	/*
	 * If we are doing a recover and no filename
	 * was given, then execute an exrecover command with
	 * the -r option to type out the list of saved file names.
	 * Otherwise set the remembered file name to the first argument
	 * file name so the "recover" initial command will find it.
	 */
	if (recov) {
		if (ac == 0 && (rcvname == NULL || *rcvname == NULL)) {
			ppid = 0;
			setrupt();
			execlp(EXRECOVER, "exrecover", "-r", (char *)0);
			filioerr(EXRECOVER);
			exit(++errcnt);
		}
		if (rcvname && *rcvname)
			CP(savedfile, rcvname);
		else
			CP(savedfile, *av++), ac--;
	}

	/*
	 * Initialize the argument list.
	 */
	argv0 = av;
	argc0 = ac;
	args0 = av[0];
	erewind();

	/*
	 * Initialize a temporary file (buffer) and
	 * set up terminal environment.  Read user startup commands.
	 */
	if (setexit() == 0) {
		setrupt();
		intty = isatty(0);
		value(vi_PROMPT) = intty;
		if (cp = (unsigned char *)getenv("SHELL"))
			CP(shell, cp);
		if (fast)
			setterm("dumb");
		else {
			gettmode();
			cp = (unsigned char *)getenv("TERM");
			if (cp == NULL || *cp == '\0')
				cp = (unsigned char *)"unknown";
			setterm(cp);
		}
	}
	if (setexit() == 0 && !fast) {
		struct stat home, dot;
		int check=0;

		if ((globp = (unsigned char *)getenv("EXINIT")) && *globp)
			commands(1,1);
		else {
			globp = 0;
			if ((cp = (unsigned char *)getenv("HOME")) != 0 && *cp){
				(void)strcat(strcpy((char *)genbuf, 
					     (const char *)cp), "/.exrc");
				if (stat(genbuf, &home) >= 0) {
					check = 1;
					source(genbuf, 1, &home);
				}
			}
		}
		/* 
		 * Allow local .exrc if the "exrc" option was set. 
		 */

		if (value(vi_EXRC)) {
			/*
			 * Don't source $HOME/.exrc twice.
			 */
			if (stat(".exrc", &dot) >=0 && 
			    (!check || home.st_ino != dot.st_ino ||
			     home.st_dev != dot.st_dev))
				source(".exrc", 1, &dot);
		}
	}
	init();	/* moved after prev 2 chunks to fix directory option */

	/*
	 * Initial processing.  Handle tag, recover, and file argument
	 * implied next commands.  If going in as 'vi', then don't do
	 * anything, just set initev so we will do it later (from within
	 * visual).
	 */
	if (setexit() == 0) {
		if (recov)
			globp = (unsigned char *)"recover";
		else if (itag)
			globp = ivis ? (unsigned char *)"tag" : (unsigned char *)"tag|p";
		else if (argc)
			globp = (unsigned char *)"next";

		if (command != NULL) {
			if (globp) {
				unsigned char *s;
				s=malloc(strlen(command)+strlen(globp)+2);
				if (s == NULL) {
					(void)pfmt(stderr,MM_ERROR,
					":285:Malloc failed:%s\n",
					strerror(errno));
					exit(1);
				}
				sprintf(s,"%s|%s", globp, command);
				free(command);
				command = s;
			}
			globp = command;
		}
		if (ivis)
			initev = globp;
		else if (globp) {
			inglobal = 1;
			commands(1, 1);
			inglobal = 0;
		}
	}

	/*
	 * Vi command... go into visual.
	 */
	if (ivis) {
		/*
		 * Don't have to be upward compatible 
		 * by starting editing at line $.
		 */
		if (dol > zero)
			dot = one;
		globp = (unsigned char *)"visual";
		if (setexit() == 0)
			commands(1, 1);
	}

	/*
	 * Clear out trash in state accumulated by startup,
	 * and then do the main command loop for a normal edit.
	 * If you quit out of a 'vi' command by doing Q or ^\,
	 * you also fall through to here.
	 */
	seenprompt = 1;
	ungetchar(0);
	globp = 0;
	initev = 0;
	if (command != NULL)
		free(command);
	setlastchar('\n');
	setexit();
	commands(0, 0);
	cleanup(1);
	exit(errcnt);
}

/*
 * Initialization, before editing a new file.
 * Main thing here is to get a new buffer (in fileinit),
 * rest is peripheral state resetting.
 */
init()
{
	register int i;
	void (*pstat)();
	fileinit();
	dot = zero = truedol = unddol = dol = fendcore;
	one = zero+1;
	undkind = UNDNONE;
	chng = 0;
	edited = 0;
	for (i = 0; i <= 'z'-'a'+1; i++)
		names[i] = 1;
	anymarks = 0;
        if(xflag) {
                xtflag = 1;
                /* ignore SIGINT before crypt process */
		pstat = signal(SIGINT, SIG_IGN);
		if(tpermflag)
			(void)crypt_close(tperm);
		tpermflag = 1;
		if (makekey(tperm) != 0) {
			xtflag = 0;
			smerror(":6", 
				"Warning--Cannot encrypt temporary buffer\n");
        	}
		signal(SIGINT, pstat);
	}
}

/*
 * Return last component of unix path name p.
 */
unsigned char *
tailpath(p)
register unsigned char *p;
{
	register unsigned char *r;

	for (r=p; *p; p++)
		if (*p == '/')
			r = p+1;
	return(r);
}
