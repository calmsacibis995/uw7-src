/*		copyright	"%c%" 	*/

#ident	"@(#)parse.c	1.2"
#ident  "$Header$"

#include "string.h"
#include "sys/types.h"
#include "stdlib.h"
#include "lp.h"
#include "printers.h"

#define	WHO_AM_I	I_AM_LPSTAT

#include "oam.h"
#include "lpstat.h"

typedef struct execute {
	char			*list;
	void			(*func)();
	int			inquire_type;
	struct execute		*forward;
} EXECUTE;

int			D		= 0;
int			v		= 0; /* ul90-35222 abs s19 */
static int		r		= 0;
unsigned int		verbosity	= 0;
unsigned int		lvlformat	= 0;
extern char		*optarg;

extern int		getopt(),
			optind,
			opterr,
			optopt;

#if	defined(__STDC__)
static void		usage ( void );
#else
static void		usage();
#endif

#if	defined(CAN_DO_MODULES)
#define OPT_LIST	"a:c:do:p:rstu:v:f:DS:zZlLRH"
#else
#define OPT_LIST	"a:c:do:p:rstu:v:f:DS:zZlLR"
#endif

#define	QUEUE(LIST, FUNC, TYPE) \
	{ \
		next->list = LIST; \
		next->func = FUNC; \
		next->inquire_type = TYPE; \
		next->forward = (EXECUTE *)Malloc(sizeof(EXECUTE)); \
		(next = next->forward)->forward = 0; \
	}
/*
 * Procedure:     parse
 *
 * Restrictions:
 *                getname: None
 *
 *  PARSE COMMAND LINE OPTIONS
*/

/*
 * This routine parses the command line, builds a linked list of
 * function calls desired, then executes them in the order they 
 * were received. This is necessary as we must apply -l to all 
 * options. So, we could either stash the calls away, or go
 * through parsing twice. I chose to build the linked list.
 */

void
#if	defined(__STDC__)
parse (
int  argc,
char **argv
)
#else
parse (argc, argv)
int  argc;
char **argv;
#endif
{
	int  optsw,need_mount=0,iss=0,ist=0,oopt=1,i;
	char *p,**list;
	EXECUTE	linked_list;
	level_t	curr_lvl = 0;
	register EXECUTE *next = &linked_list;

	next->forward = 0;
	/*
	   Check for both the -s and -t option to prevent displaying duplicate
	   information since several options are a complete subset of s and t.
	   Add "--" to the end of argc to flag end of the options.
	*/
	argv[argc] = (char *)Malloc(sizeof(char *));
	argv[argc++] = "--";
	opterr = 0;	/* disable printing of error message by getopt */
	while ((optsw = getopt(argc, (char * const *)argv, OPT_LIST)) != -1) {
		switch(optsw) {
		case 't':	
			ist = 1;
			break;
		case 's':	
			iss = 1;
		}
	}
	optind = 1;    /* reset the index so we can re-parse the options */
	for (;(optsw=getopt(argc,argv,OPT_LIST))!=-1;oopt=optind) {
		switch (optsw) {
		case 'a':
		case 'c':
		case 'o':
		case 'p':
		case 'u':
		case 'v':
		case 'f':
		case 'S':
			/*
			   Use oopt to prevent looping when command is called 
			   with "-o-l" options.
			*/
			if (*optarg == '-' && optind - oopt == 2) {
				optind--;
				optarg = NAME_ALL;
			}
			break;
		}
		switch(optsw) {
		case 'a':	/* acceptance status */
			if (!ist && !iss) QUEUE (optarg,do_accept,INQ_ACCEPT);
			break;
		case 'c':	/* class to printer mapping */
			if (!ist && !iss) QUEUE (optarg,do_class,0);
			break;
		case 'd':	/* default destination */
			if (!ist && !iss) QUEUE (0,def,0);
			break;
		case 'D':	/* Description of printers */
			D = 1;
			break;
		case 'f':	/* do forms */
			if (!ist && !iss) QUEUE (optarg, do_form, 0);
			need_mount = 1;
			break;

#if	defined(CAN_DO_MODULES)
		case 'H':	/* show modules pushed for printer */
			verbosity |= V_MODULES;
			break;
#endif
		case 'l':	/* verbose output */
			verbosity |= V_LONG;
			verbosity &= ~V_BITS;
			break;
		case 'z':	/* show mac level alias */
			lvlformat |= LVL_ALIAS;
			break;
		case 'Z':	/* show mac level */
			lvlformat |= LVL_FULL;
			break;
		case 'L':	/* special debugging output */
			verbosity |= V_BITS;
			verbosity &= ~V_LONG;
			break;
		case 'o':	/* output for destinations */
			if (!ist && !iss) QUEUE (optarg,do_request,0);
			break;
		case 'p':	/* printer status */
			if (!ist&&!iss) QUEUE (optarg,do_printer,INQ_PRINTER);
			break;
		case 'R':	/* show rank in queue */
			verbosity |= V_RANK;
			break;
		case 'r':	/* is scheduler running? */
			if (!ist && !iss) QUEUE (0, running, 0);
			r = 1;
			break;
		case 's':	/* configuration summary */
			if (!ist) {
				QUEUE (0,running,0);
				QUEUE (0,def,0);
				QUEUE (NAME_ALL,do_class,0);
				QUEUE (NAME_ALL,do_device,0);
   				QUEUE (NAME_ALL, do_form, 0);
				QUEUE (NAME_ALL,do_charset, 0);
			}
			r = 1;
			need_mount = 1;
			break;
		case 'S':	/* character set info */
			if (!ist && !iss) QUEUE (optarg, do_charset, 0);
			need_mount = 1;
			break;
		case 't':	/* print all info */
			QUEUE (0, running, 0);
			QUEUE (0, def, 0);
			QUEUE (NAME_ALL, do_class, 0);
			QUEUE (NAME_ALL, do_device, 0);
			QUEUE (NAME_ALL, do_form, 0);
			QUEUE (NAME_ALL, do_charset, 0);
			QUEUE (NAME_ALL, do_accept, INQ_ACCEPT);
			QUEUE (NAME_ALL, do_printer, INQ_PRINTER);
			QUEUE (NAME_ALL, do_request, 0);
			r = 1;
			need_mount = 1;
			break;
		case 'u':	/* output by user */
			QUEUE (optarg, do_user, INQ_USER);
			break;
		case 'v':	/* printers to devices mapping */
			v = 1;	/* ul90-35222 abs s19 */
			if (!ist && !iss) QUEUE (optarg,do_device,0);
			break;
		default:
			if (optopt == '?') {
				usage ();
				done (0);
			}
			(p = "-X")[1] = optopt;
			if (strchr(OPT_LIST, optopt))
				LP_ERRMSG1 (ERROR, E_LP_OPTARG, p);
			else
				LP_ERRMSG1 (ERROR, E_LP_OPTION, p);
			done(1);
			break;
		}
	}
	list = 0;
	if (optind <= argc) argc--;
	while (optind < argc) {
		if (addlist(&list,argv[optind++])==-1) {
			LP_ERRMSG (ERROR, E_LP_MALLOC);
			done(1);
		}
	}
	if ((lvlproc (MAC_GET, &curr_lvl) != 0) && errno == ENOPKG)
		lvlformat = 0; 		/*MAC is not installed*/
	if (list)
		QUEUE (sprintlist(list), do_request, 0);
	if (argc==1 || (verbosity & V_RANK) && argc==2 || lvlformat&&argc==2)
		QUEUE (getname(), do_user, INQ_USER);
	startup ();
	/* 
	   Linked list is completed, load up mount info if 
	   needed then do the requests
	*/
	if (need_mount) {
		inquire_type = INQ_STORE;
		do_printer (alllist);
	}
	for (next = &linked_list; next->forward; next = next->forward) {
		inquire_type = next->inquire_type;
		if (!next->list)
			(*next->func) ();
		else if (!*next->list)
			(*next->func) (alllist);
		else
			(*next->func) (getlist(next->list, LP_WS, LP_SEP));
	}
	return;
}

/**
 ** usage() - PRINT USAGE MESSAGE
 **/
static void
#if	defined(__STDC__)
usage (
	void
)
#else
usage ()
#endif
{
	LP_OUTMSG(INFO, E_STAT_USAGE);
	LP_OUTMSG(INFO|MM_NOSTD, E_STAT_USAGE1);
	LP_OUTMSG(INFO|MM_NOSTD, E_STAT_USAGE2);
#if	defined(CAN_DO_MODULES)
	LP_OUTMSG(INFO|MM_NOSTD, E_STAT_USAGE3);
#else
	LP_OUTMSG(INFO|MM_NOSTD, E_STAT_USAGE4);
#endif
	LP_OUTMSG(INFO|MM_NOSTD, E_STAT_USAGE5);
	LP_OUTMSG(INFO|MM_NOSTD, E_STAT_USAGE6);
	return;
}
