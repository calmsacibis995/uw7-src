#ident	"@(#)debugger:debug.d/common/main.C	1.35"
/*
 *	UNIX debugger
 */

#include "Interface.h"
#include "Machine.h"
#include "Msgtab.h"
#include "Parser.h"
#include "Proctypes.h"
#include "Procctl.h"
#include "global.h"
#include "Buffer.h"
#include "Vector.h"
#include "dbg_edit.h"
#include "Dbgvarsupp.h"
#include "utility.h"
#include "Path.h"
#include "NewHandle.h"
#include <sys/types.h>
#include <limits.h>
#include <stdio.h>
#include <termio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sgs.h>
#include <unistd.h>
#include <locale.h>
#include <errno.h>
#include <libgen.h>

#define OPTS		"DVrs:d:c:f:i:l:X:Y:?"  // getopt option string

#if FOLLOWER_PROC
const char		*follow_path;
#endif

#ifdef DEBUG_THREADS
#ifndef THREAD_LIBRARY_NAME
#define THREAD_LIBRARY_NAME	"libthread.so.1"
#endif
const char		*thread_library_name;
#endif

#ifdef __cplusplus
#define CEXTERN	extern "C"
#else
#define CEXTERN extern
#endif

#if DEBUG
#include "str.h"
#endif

const char		*msg_internal_error;
char			*prog_name;
static Buffer		bpool[BPOOL_SIZE];
static Vector		vpool[VPOOL_SIZE];
Buffer_pool		buf_pool(bpool);
Vector_pool		vec_pool(vpool);
long			pagesize;

extern void		debugtty();
static void		save_tty();
static void		new_handler();
extern void		do_defaults(int, const char *);
CEXTERN void		signal_setup();
CEXTERN void		signal_unset();

int	(*read_func)(int, void *, unsigned int) = debug_read;

#ifdef UNIXWARE_1_1
const char	*xui_name = "debug.ol.ui_CC";
#else
#ifdef MOTIF_UI
const char	*xui_name = "debug.motif.ui";
#else
const char	*xui_name = "debug.ol.ui";
#endif
#endif // !UNIXWARE_1_1

static void
show_options( char *s )
{
	printm(MSG_usage_cmdline_loc, s);
	printm(MSG_usage_grab, s);
	printm(MSG_usage_core, s);
#ifdef FOLLOWER_PROC
	printm(MSG_usage_common);
#else
	printm(MSG_usage_threads_common);
#endif
}

#define LIVE	1
#define EXEC	2

main(int argc, char *argv[], const char **envp)
{
	extern int 	optind;
	extern char 	*optarg;
	int		dfltfile;
	char		*defaults = 0;
	char		*corfil = 0;
	char		*interface = 0;
	char		*path = 0;
	int 		i, c, sz = 0;
	int		proc;
	int		rflag = 0;
	int		Vflag = 0;
	char		*cmd_line = 0;
	char		**xopts = 0;
	char		*loadfile = 0;
	char		*p;
	size_t		xsize;
	int		xnum;
	char 		*follow = 0;
	const char	*alias_path = 0;
	const char	*gui_path = 0;
	int		Dflag = 0;
#if FOLLOWER_PROC
	const char	*foll_path = 0;
#endif

	// use unbuffered output
	(void) setbuf(stdout, NULL);	
	(void) setbuf(stderr, NULL);


	// setup signal state and "new" failure disposition
	signal_setup();
	newhandler.install_user_handler(new_handler);

	pushoutfile(stdout);
	(void) setlocale(LC_ALL, "");
	prog_name = basename(argv[0]);
	Mtable.init();

	// error message used when debugger catches its own
	// synchronous signals
	msg_internal_error = Mtable.format(ERR_cannot_recover);

	ksh_init();
	init_cwd();
	pagesize = sysconf(_SC_PAGESIZE);

	// set up the option array to be passed to the ui
	// argc is a worst case for number of args - really
	// only pass those starting with -X
	//
	// BUT, the code assumes that there are at least two
	// elements in xopts!
	//
	if (argc < 2)
		xopts = new char * [2];
	else
		xopts = new char *[argc];
	xnum = 1;	// xopts[0] is filled in in set_interface

	// process command line arguments
	while ((c = getopt(argc, argv, OPTS)) != EOF) 
	{
		switch(c) 
		{
		case 'D':
			p = xopts[xnum++] = new(char[3]);
			p[0] = '-';
			p[1] = 'D';
			p[2] = '\0';
			Dflag = 1;
			break;
		case 'V':
			++Vflag;
			printm(MSG_version, prog_name,
				EDEBUG_PKG, EDEBUG_REL);
			break;
		case 'X':
			// args must be passed with "-"
			// but may be received with or without
			xsize = strlen(optarg) + 1;
			p = xopts[xnum++] = new(char[xsize]);
			strcpy(p, optarg);
			break;
		case 'Y':
		{
			int	set_alias = 0;
			int	set_gui = 0;
#if FOLLOWER_PROC
			int	set_follow = 0;
#endif

			while(*optarg && *optarg != ',')
			{
				switch(*optarg++)
				{
					case 'a':
						set_alias = 1;
						break;
#if FOLLOWER_PROC
					case 'f':
						set_follow = 1;
						break;
#endif
					case 'g':
						set_gui = 1;
						break;
					default:
					{
						goto bad_Y;
					}
				}
			}
			if (!*optarg++)
			{
				goto bad_Y;
			}
			while(*optarg && *optarg == ' ')
				optarg++;
			if (!*optarg)
			{
bad_Y:
				printe(ERR_bad_cmd_arg, E_FATAL, 
					 prog_name, "Y");
				exit(1);
			}
			if (set_alias)
				alias_path = optarg;
#if FOLLOWER_PROC
			if (set_follow)
				foll_path = optarg;
#endif
			if (set_gui)
				gui_path = optarg;
			break;
		}
		case 'c':
			if (corfil)
			{
				printe(ERR_multiple_opts, E_WARNING,
					"-c");
				break;
			}
			corfil = optarg;
			break;
		case 'd':
			if (defaults)
			{
				printe(ERR_multiple_opts, E_WARNING,
					"-d");
				break;
			}
			defaults = optarg;
			break;
		case 'f':
			if (follow)
			{
				printe(ERR_multiple_opts, E_WARNING,
					"-f");
				break;
			}
			follow = optarg;
			if ((strcmp(optarg, "none") != 0) &&
				(strcmp(optarg, "procs") != 0) &&
				(strcmp(optarg, "all") != 0))
			{
				printe(ERR_bad_cmd_arg, E_FATAL, 
					 prog_name, "f");
				exit(1);
			}
			break;
		case 'i':
			if (interface)
			{
				printe(ERR_multiple_opts, E_WARNING,
					"-i");
				break;
			}
			interface = optarg;
			break;
		case 'l':
			// -l is used for loadfiles for grab and
			// for the starting address for create
			if (loadfile)
			{
				printe(ERR_multiple_opts, E_WARNING,
					"-l");
				break;
			}
			loadfile = optarg;
			break;
		case 'r':
			rflag = 1;
			break;
		case 's':
			if (path)
			{
				printe(ERR_multiple_opts, E_WARNING,
					"-s");
				break;
			}
			path = optarg;
			break;
		case '?':
			show_options( prog_name );
			exit(1);
			break;
		default:
			exit(4);
		}
	}

	if ( Vflag && (argc == 2) ) 
		exit(0);

	if (Dflag)
		signal_unset();

	if (corfil)
	{
		if (loadfile)
			printe(ERR_opt_combo, E_FATAL, prog_name, "-l",
				"-c");
		else if (rflag)
			printe(ERR_opt_combo, E_FATAL, prog_name, "-r",
				"-c");
		else if (follow)
			printe(ERR_opt_combo, E_FATAL, prog_name, "-f",
				"-c");
	}
#if FOLLOWER_PROC
	// set up follower path
	if (foll_path)
	{
		// never deleted - used throughout debug session
		follow_path = new(char[strlen(foll_path)+sizeof("/follow")]);
		strcpy((char *)follow_path, foll_path);
		strcat((char *)follow_path, "/follow");
	}
	else
		follow_path = debug_follow_path;
#endif
#ifdef DEBUG_THREADS
	thread_library_name = THREAD_LIBRARY_NAME;
#endif

	// check inteface type
	xopts[xnum] = 0;
	set_interface(interface, xopts, gui_path);

	// save user tty settings and setup debugger's 
	if (get_ui_type() == ui_gui)
	{
		// allow read from terminal in background
		// process for gui - don't try to write to 
		// terminal, or we may hang
		sigignore(SIGTTOU);
		// in gui, we don't unblock interrupt around read calls
		prdelset(&debug_sset, SIGINT);
	}
	else
	{
		save_tty();
		debugtty();
	}

	Import_environment(envp);

	// defaults file  - either given on command line or
	// look for $HOME/.debug_rc
	if (defaults)
	{
		if ((dfltfile = debug_open(defaults, O_RDONLY)) == -1)
			printe(ERR_cant_open, E_WARNING, defaults,
				strerror(errno));
	}
	else
	{
		char	*home;

		home = getenv("HOME");
		if (home)
		{
			char	fname[PATH_MAX+1];

			(void)strcpy(fname, home);
			(void)strcat(fname, "/.debugrc");
			dfltfile = debug_open(fname, O_RDONLY);
		}
	}
	do_defaults(dfltfile, alias_path); // even if dfltfile <0
	if (dfltfile >= 0)
		close(dfltfile);

	//  set source path
	if (path) 
	{
		global_path = set_path( 0, path );
		pathage++;
	}

	// if command line request, create command to be handled
	//  by parse_and_execute()
	if (corfil)
	{
		if (argc - optind != 1)
		{
			show_options(prog_name);
			exit(4);
		}
		sz = sizeof("grab -c   \n");
		sz += strlen(corfil) + strlen(argv[optind]);
		cmd_line = new(char[sz]);
		(void)sprintf(cmd_line,"grab -c %s %s\n",
			corfil, argv[optind]);
	}
	else if (optind < argc)
	{
		/* command line or list of live procs */
		char	*cptr;
		char	follow_opt[sizeof("-f procs")];

		if (access(argv[optind], R_OK) == -1)
		{
			// not a file we can open - see if its a pid
			if (strtol(argv[optind], 0, 10) > 0)
				proc = LIVE;
			else
				// error case, but we'll let
				// the create command deal with it
				proc = EXEC;
		}
		else 
		{
			if (live_proc(argv[optind]))
				proc = LIVE;
			else
				proc = EXEC;
		}
		i = optind;
		sz = sizeof("create -r -f procs -l \n"); // worst case
		if (loadfile)
			sz += strlen(loadfile);
		while (argc > i)
		{
			int tsize;
			tsize = strlen(argv[i++]);
			if (!tsize)
				tsize = 2; // space for ""
			sz += tsize + 1;
		}
		cptr = cmd_line = new(char[sz]);

		if (follow)
			sprintf(follow_opt, "-f %s", follow);
		else
			follow_opt[0] = 0;

		if (proc == EXEC)
		{
			char	*redir;
			if (rflag || (get_ui_type() == ui_gui))
				redir = "-r";
			else
				redir = "";
			if (loadfile)
				// loadfile is start addr for create
				cptr += sprintf(cptr, 
					"create %s -l %s %s %s",
					redir, loadfile, follow_opt,
					argv[optind++]);
			else
				cptr += sprintf(cptr,
					"create %s %s %s",
					redir, follow_opt,
					argv[optind++]);
		}
		else 
		{
			if (rflag)
				printe(ERR_grab_dashr, E_FATAL,
					prog_name);
			if (loadfile)
				cptr += sprintf(cptr,
					"grab %s -l %s %s\n",
					follow_opt, loadfile,
					argv[optind++]);
			else
				cptr += sprintf(cptr, "grab %s %s",
					follow_opt, argv[optind++]);
		}
		while (argc > optind)
		{
			if (!*argv[optind])
			{
				strcpy(cptr, " \"\"");
				cptr += 3;
			}
			else
				cptr += sprintf(cptr, " %s",
					argv[optind]);
			optind++;
		}
		(void)strcpy(cptr, "\n");
	}

	if (cmd_line)
	{
		parse_and_execute(cmd_line);
		delete cmd_line;
	}

	/* main command loop */
	docommands();

	db_exit(0);
	/*NOTREACHED*/
}

static int		ttysaved;
static struct termio	ttybuf;

void
restore_tty()
{
	if (ttysaved)
		ioctl(0, TCSETAW, &ttybuf);
}

static void
save_tty()		/* save the terminal settings */
{
	if (ioctl(0, TCGETA, &ttybuf) == 0)
		ttysaved = 1;
}

void 
debugtty()
{
	static struct termio	debugttym;
	static int		set;

	if (!set)
	{
		set = 1;
		debugttym = ttybuf;
	}
	ioctl(0, TCSETA, &debugttym);
}

/* clean up and exit */
void
db_exit(int ecode)
{
#if DEBUG
	// statistics on str use
	if (debugflag & DBG_STR)
		printf("%s\n", str(0));
#endif
	if (original_dir)
	{
		// reset original working directory
		chdir(original_dir);
	}
	destroy_all();
	stop_interface();
	restore_tty();
	exit(ecode);
}

/* exit if malloc fails */
/* can't use message facilities, since they use malloc */
static void
new_handler()
{
	char	*syserr = strerror(errno);
	char	*errmess = "Fatal error: memory allocation: ";
	write(2, errmess, strlen(errmess));
	if (syserr)
		write(2, syserr, strlen(syserr));
	write(2, "\n", 1);
	db_exit(1);
}
