#ident	"@(#)rtpm:rtpm.c	1.9.1.1"


/*
 * rtpm.c:	real time performance monitor
 *
 *  gather system metrics, compute interval data and display.
 */

#define _KMEMUSER 1
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#include <sys/types.h>
#include <ctype.h>
#include <mas.h>
#include <metreg.h>
#include <curses.h>
#include <signal.h>
#undef _KMEMUSER	/* make setrlimit def visible */
#include <sys/resource.h>
#include "rtpm.h"

/*
 * a place to keep argv[0]
 */
char *progname;
/*

 * the current position in the cooked metric buffers
 */
int curcook = 0;
/*
 * maximum number of history points to keep
 */
int maxhist = 0;
/*
 * whether or not we have rolled over the end of the cooked metric buffers
 * eg. have we seen more than maxhist samples
 */
int rollover = 0;
/*
 * set by the signal handler to indicate we have to go get some metrics
 */
volatile int caught_alarm = 0;
/*
 * the number of rows and columns on the screen
 */
int scr_rows = 24;
int scr_cols = 80;
/*
 * sampling interval
 */
int  interval;
/*
 * some misc variables we need to know about
 */
int md = 0;		/* metric descriptor ret from mas_open	*/
/*
 * flag to indicate whether curses has been started
 */
int in_curses = 0;
/*
 * froot is first field on the screen, array index is for pushed screens
 */
extern struct field *froot[];
/*
 * currfield is current field selected on screen, array index is for
 * pushed screens
 */
extern struct field *currfield[];
/*
 * screendepth is current number of screens that are pushed, it is used
 * as the array index for froot and currfield
 */
extern int screendepth;
/*
 * flag indicates whether or not to redraw labels
 */
extern int need_header;
/*
 * list of plotted metrics
 */
extern struct plotdata *plotdata;
/*
 * functions in rtpm.c
 */
int open_mets( void );
void catch_sigint( int signum );
void catch_sigalarm( int signum );
static void my_init( void );

/*
 *	function: 	open_mets
 *
 *	args:		none
 *
 *	ret val:	non-negative int on success
 *			-1 on failure
 *
 *	Open_mets opens the metric registration file MAS_FILE
 *	which is defined in metreg.h.  The raw system metrics
 *	are memory mapped into the calling process' address space.
 *	Memory is allocated for the metrics declared in mettbl,
 *	see mtbl.c.
 */
int 
open_mets( void ) {
	if (!readata()) {	/* get data from psfile */
		getdev();
		getpasswd();
	}
/* 
 * 	this call to ether_stat sets nether, and must come before metalloc
 */
	ether_stat();
/* 
 * 	this call to netware_stat sets spxmaxconn and nlans, 
 *	and must come before metalloc
 */
	netware_stat();

	md = mas_open( MAS_FILE, MAS_MMAP_ACCESS );
	if( md < 0 ) {
		mas_perror();
		exit(1);
	}
	mas_snap(md);
	initproc();
	read_proc();
	ether_stat();
	net_stat();
	netware_stat();
	get_mem_and_swap();
	alloc_mets();
	return( md );
}

/*
 *	function: 	catch_sigint
 *
 *	args:		a signal number (unused)
 *
 *	ret val:	exits
 *
 *	catch_sigint calls quit, which undoes what curses has done to 
 *	the screen and then exits.
 */
/* ARGSUSED */
void catch_sigint( int signum ) {
	quit();
}

/*
 *	function: 	catch_sigalarm
 *
 *	args:		a signal number (unused)
 *
 *	ret val:	none
 *
 *	catch_sigalarm resets the alarm and sets caught_alarm, 
 *	a global flag that indicates an alarm has occurred.
 *	Using the interval timer supplied by setitimer() would 
 *	be a better choice than using alarm, but curses uses 
 *	it to determine whether a cursor arrow key has been
 *	received and doesn't bother to reset it afterwards.
 */
/* ARGSUSED */
void catch_sigalarm( int signum ) {
#ifdef SLOW_START
	static int firstime = 1;
#endif
	signal( SIGALRM, catch_sigalarm );
	caught_alarm = 1;

#ifdef SLOW_START
	if( firstime ) {
/*
 *		give curses 10 seconds to get the first screen drawn
 *		it should be a lot faster, but if we're running across
 *		a busy network, it may take a while.
 */
		alarm( max( 10, interval ) );
		firstime = 0;
		return;
	}
#endif
	alarm( interval );
}

/*
 *	function: 	my_init
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	init sets the signal handler for sigint, and the calls the
 *	curses start up routines.  Note that cbreak() is not called,
 *	rtpm expects the keyboard input to block until a sigalarm.
 *	Metric snapshots are taken every time a sigalarm hits.
 */
static void
my_init( void ) {

	signal( SIGINT, catch_sigint );
	initscr();
	in_curses = 1;
	init_rtpmcolor();
	set_default_color( 0 );
	color_clear();
	noecho();
	keypad( stdscr, TRUE );
}

/*
 *	function: 	main
 *
 *	args:		argc and argv, you already know these :-)
 *
 *	ret val:	none
 *
 *	parses args and calls a bunch of initialization functions.
 *	The overview of the main loop is:
 *	
 *	forever {
 *		take a snapshot of the metrics, cook and serve
 *		while keyboard input is not interrupted by sigalarm {
 *			do whatever the user requested
 *		}
 *		increment position in metric history buf for next snapshot
 * 	}
 */
main( int argc, char **argv ) {
	char *rowp, *colp;
	int inch;
	struct rlimit rlim;
	extern char *optarg;
	extern int optind;
	int c;
	int hflg = 0;
	int errflg = 0;
	char *bspace;
/*
 *	increase limits on brk and mmap.
 */
	rlim.rlim_cur = RLIM_INFINITY;
	rlim.rlim_max = RLIM_INFINITY;
	setrlimit( RLIMIT_VMEM, &rlim );
	setrlimit( RLIMIT_DATA, &rlim );
/*
 *	grab some brk space while it is available,
 *	later on, we may bump into an mmap
 */
	bspace = malloc( 512*1024 ); 
	if(bspace) 
		free(bspace);

	setlocale( LC_ALL, "" );

	interval = DEFAULT_ALARM;

	progname = argv[0];

	while(( c = getopt( argc, argv, "h:" ) ) != EOF ) {
		switch( c ) {
		case 'h':
			if( hflg++ )
				errflg++;
			maxhist = atoi( optarg );
#ifdef PEAK_HOLD
			if( maxhist < HOLDTIME ) {
				maxhist = HOLDTIME;
				fprintf(stderr,gettxt("RTPM:668", "%s ignoring histsz arg, must be > %d\n"),progname, HOLDTIME );
			}

#else 
			if( maxhist < 1 ) {
				maxhist = 1;
				fprintf(stderr,gettxt("RTPM:668", "%s ignoring histsz arg, must be > %d\n"),progname, 0 );
			}
#endif
			break;
		default:
			errflg++;
			break;
		}
	}
	if( optind < argc-1 )
		errflg++;

	if( errflg || ( optind != argc && !isdigit(*argv[optind]))) {
		fprintf(stderr,gettxt("RTPM:667", "usage: %s [-h history_buffer_size] [interval]\n"),progname);
		exit(1);
	}
	if( optind != argc ) {
		interval = atoi( argv[optind] );
		if( interval <= 0 ) {
			fprintf(stderr,gettxt("RTPM:2", "%s interval must be > 0\n"),progname);
			exit(1);
		}
	}
/*
 *	Try to get LINES and COLUMNS
 */
	rowp = getenv(gettxt("RTPM:8", "LINES"));
	if( !rowp )
		rowp = getenv("LINES");
	colp = getenv(gettxt("RTPM:9", "COLUMNS"));
	if( !colp )
		colp = getenv("COLUMNS");	
	if( rowp && isdigit(*rowp) )
		scr_rows = max( 24, atoi( rowp ));
	if( colp && isdigit(*colp) )
		scr_cols = max( 80, atoi( colp ));

	if( !maxhist )
		maxhist = scr_cols;		

/*
 *	set nmets, the number of metrics in mettbl
 */
	set_nmets();
/*
 * 	open the metric registration table and allocate space for the mets
 */
	open_mets();

/*
 *	set the titles of the metrics, this is done at runtime for i18n
 */
	set_met_titles();
/*
 *	take the first snapshot of the metrics
 */
	set_oldtime();
	snap_mets();

/* 	call calc_interval_data to "cook" everything.  Since there's only 
 *	one sample, this call computes garbage for the cooked values, but 
 * 	we need to call it anyway to set the initial values used for 
 *	computing the interval data.		
 */
	calc_interval_data();	
/*
 *	get the input keystrokes from the i18n catalog
 */
	init_keyseq();
/*
 *	start up curses		
 */
	my_init();
/*
 *	if no plots were requested in .rtpmrc, turn on the bargraph
 */
	if( !plotdata )
		setbar();
/*
 * 	take a short nap, since we can't display until we have two 
 *	samples.  Using a short interval for first sample allows us 
 *	to get the screen up with a valid display as quickly as possible.
 */
	sleep(1);
/*
 *	put on our catcher's mitt and ...
 */
	signal( SIGALRM, catch_sigalarm );
/*
 *	here comes the first pitch!
 */
	alarm( interval );
 	for( ; ; ) {
		caught_alarm = 0;	/* reset alarm flag		*/
		need_header |= PLOT_UPDATE; /* note that we need to plot*/
		snap_mets();		/* take snapshot of the metrics	*/
		calc_interval_data();	/* cook everything		*/
		print_mets();		/* display them			*/
/*
 *		check to make sure there is at least one addressable
 *		field on the screen.  If there is no current field,
 *		make the first one on the list the current one and
 *		highlight it
 */
		assert(froot[screendepth]);
		if( !currfield[screendepth] ) {
			currfield[screendepth] = froot[screendepth];
		}
		highlight_currfield();
/*
 *		For lack of a better place, move the cursor to the 
 *		bottom right hand corner, this is because some cursors 
 *		will interfere with the reverse video of the highlighted
 *		field, doing a reverse of a reverse, which looks funny.
 *		Then refresh the screen.
 */
		move( scr_rows-1, scr_cols-1 );
		refresh();
/*
 *		While we don't have an alarm, read the keyboard 
 *		and do whatever the user wants.  Potentially, there
 *		is a race between the clock and the user's keystrokes,
 *		but in practice, lost keystrokes are very infrequent.
 */
		while( !caught_alarm ) {
/*
 *			wait for alarm or input char
 */
			if( ( inch = wgetch( stdscr ) ) != ERR ) {
				do_input( inch );
				if( need_header ) {
					print_mets();
				}
				move( scr_rows-1, scr_cols-1 );
				refresh();
			}
		}
/*
 *		The alarm has gone off, increment the position within
 *		the metric history buffer at which the next sample
 *		will be put.
 */
		if( ++curcook == maxhist ) {
			curcook = 0;
			rollover = 1;
		}
	}
}

