#ident	"@(#)rtpm:input.c	1.5"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <assert.h>
#include <mas.h>
#include <metreg.h>
#include <sys/dl.h>
#include <curses.h>
#include <string.h>
#include <sys/lock.h>
#include "rtpm.h"
#include "mtbl.h"
#include "input.h"

/*
 * flag indicates whether or not to display messages at bottom of screen
 */
extern int msg_disp;
/*
 * flag indicates whether or not to redraw labels
 */
extern int need_header;
/* 
 * flag indicates whether or not rtpm is locked in core
 */
int plocked = 0;
/*
 * columns at which to put plock msg on bottom of screen
 */
extern int plock_col;
extern int time_col;
/*
 * state variable to indicate plotting format
 */
extern int ptype;
/*
 * flag for term supports underline mode or should use "_"
 */
extern int does_underline;
/*
 * screenfunc is an array ptrs to functs, screenfunc[screendepth]
 * is the address of the function that draws the current screen
 */
extern void (*screenfunc[])();
/*
 * the depth of the current screen
 */
extern int screendepth;
/*
 * the sleep/alarm interval
 */
extern int interval;
/*
 * flag indicates whether the alarm has gone off
 */
extern int caught_alarm;
/*
 * the current position in the metric buffers
 */
extern int curcook;
/*
 * flag indicates whether a roll over of the metric buffers has occurred.
 * eg. have we had more than maxhist samples
 */
extern int rollover;
/*
 * the number or rows and cols on the screen
 */
extern int scr_rows, scr_cols;
/*
 * the current lbolt time
 */
extern long currtime;
/*
 * time difference in seconds between the current and previous samples
 */
extern float tdiff;
/*
 * The metric table: where everything is kept and cooked
 */
extern struct metric mettbl[];

extern int barscroll;	/* num of spaces bargraph has been scrolled down */
extern int metscroll;	/* num of slots metrics have been scrolled right */
extern int canscroll;	/* are there enough metrics to need to scroll?	 */
extern int scrollcol;	/* kludge - left column from which to scroll	 */

extern struct field *froot[];
extern struct field *currfield[];
extern int screendepth;

extern struct plotdata *plotdata;
extern int maxplots;	/* total max num of plots fit on the screen	*/
extern int maxvplots;	/* max vertical number of plots			*/
extern int maxhplots;	/* max horizontal number of plots		*/
extern int disprows;	/* number of rows for plot display		*/
extern int metrows;	/* number of rows used by metrics		*/
extern int metcols;	/* number of columns used by metrics		*/
/*
 *	binding of input keystrokes to actions
 *	input keystrokes are defined in keystroke.h
 */
struct keyin keystrokes[] = {
/*
 *	downward cursor motions
 */
	{ KEY_DOWN,	move_down,	NULL },
/*
 *	upward cursor motions
 */
	{ KEY_UP,	move_up,	NULL },
/*
 *	left cursor motions
 */
	{ KEY_LEFT,	move_left,	NULL },
/*
 *	right cursor motions
 */
	{ KEY_RIGHT, 	move_right,	NULL },
/*
 *	ether plot or push the current screen
 */
	{ 0,		plot_or_push,	NULL },
/*
 *	pop the current screen, returning to the previous one
 */
	{ (int)'\033',	pop_back,	NULL },
/*
 *	toggle whether program is locked in memory via plock()
 */
	{ 0,		toggle_memlock,	NULL },
/*
 *	display the help screen
 */
	{ 0,		get_help,	NULL },
/*
 *	limit processes displayed on the lwp screen
 */
	{ 0,		set_user_procs,	NULL },
	{ 0,		set_sys_procs,	NULL },
	{ 0,		set_all_procs,	NULL },
/*
 *	cycle the plot type through:
 *		bars -> flower plot -> connect plot -> scatter -> bars ...
 */
	{ 0,		change_plot_type,	NULL },
/*
 *	scroll the bargraph up and down
 */
	{ 0,		scroll_up,	NULL },
	{ 0,		scroll_down,	NULL },
/*
 *	scroll per-resource metrics left and right
 */
	{ 0,		scroll_left,	NULL },
	{ 0,		scroll_right,	NULL },
/*
 *	change the sampling/display interval
 */
	{ 0,		increment_interval,	NULL },
	{ 0,		decrement_interval,	NULL },
/*
 *	clear the bargraph or the oldest plot on the screen
 */
	{ 0,		clear_a_plot,	NULL },
/*
 *	display the bargraph
 */
	{ 0,		toggle_bargraph,	NULL },
/*
 *	use "_" instead of termattr
 */
	{ 0,		toggle_underscore,	NULL },
/*
 *	redraw the screen
 */
	{ 0,		redraw_the_screen,	NULL },
/*
 *	quit the program
 */
	{ 0,		quit,	NULL },
};

/*
 *	function: 	init_keyseq
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	initialize keystrokes at runtime so they can be read from
 *	i18n catalog.
 */
void
init_keyseq() {
	/*	downward cursor motions					*/
	keystrokes[ KEYSEQ_DOWN_IDX ].str = KEYSEQ_DOWN;
	/* 	upward cursor motions					*/
	keystrokes[ KEYSEQ_UP_IDX ].str = KEYSEQ_UP;
	/*	left cursor motions					*/
	keystrokes[ KEYSEQ_LEFT_IDX ].str = KEYSEQ_LEFT;
	/*	right cursor motions					*/
	keystrokes[ KEYSEQ_RIGHT_IDX ].str = KEYSEQ_RIGHT;
	/*	either plot or push the current screen			*/
	keystrokes[ KEYSEQ_PUSH_IDX ].str = KEYSEQ_PUSH;
	/*	pop the current screen, returning to the previous one	*/
	keystrokes[ KEYSEQ_POP_IDX ].str = KEYSEQ_POP;
	/*	toggle whether program is locked in memory via plock()	*/
	keystrokes[ KEYSEQ_LOCK_IDX ].str = KEYSEQ_LOCK;
	/*	display the help screen					*/
	keystrokes[ KEYSEQ_HELP_IDX ].str = KEYSEQ_HELP;
	/*	limit processes displayed on the lwp screen		*/
	keystrokes[ KEYSEQ_USR_IDX ].str = KEYSEQ_USR;
	keystrokes[ KEYSEQ_SYS_IDX ].str = KEYSEQ_SYS;
	keystrokes[ KEYSEQ_ALL_IDX ].str = KEYSEQ_ALL;
	/*	change the plot display type				*/
	keystrokes[ KEYSEQ_PLOT_IDX ].str = KEYSEQ_PLOT;
	/*	scroll the bargraph up and down				*/
	keystrokes[ KEYSEQ_SCRLLUP_IDX ].str = KEYSEQ_SCRLLUP;
	keystrokes[ KEYSEQ_SCRLLDN_IDX ].str = KEYSEQ_SCRLLDN;
	/*	scroll per-resource metrics left and right		*/
	keystrokes[ KEYSEQ_SCRLLFT_IDX ].str = KEYSEQ_SCRLLFT;
	keystrokes[ KEYSEQ_SCRLLRT_IDX ].str = KEYSEQ_SCRLLRT;
	/*	change the sampling/display interval			*/
	keystrokes[ KEYSEQ_INCR_IDX ].str = KEYSEQ_INCR;
	keystrokes[ KEYSEQ_DECR_IDX ].str = KEYSEQ_DECR;
	/*	clear the bargraph or the oldest plot on the screen	*/
	keystrokes[ KEYSEQ_CLR_IDX ].str = KEYSEQ_CLR;
	/*	display the bargraph					*/
	keystrokes[ KEYSEQ_BAR_IDX ].str = KEYSEQ_BAR;
	/*	use underscore instead of terminal attr			*/
	keystrokes[ KEYSEQ_UNDERSCORE_IDX ].str = KEYSEQ_UNDERSCORE;
	/*	redraw the screen					*/
	keystrokes[ KEYSEQ_REDRAW_IDX ].str = KEYSEQ_REDRAW;
	/*	quit the program					*/
	keystrokes[ KEYSEQ_QUIT_IDX ].str = KEYSEQ_QUIT;
}
/*
 *	function: 	move_down
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	move the cursor down to the next field on the screen and 
 *	make it the current field, if necessary, scroll or wrap around.
 */
void
move_down() {
	msg_disp = 0;
	mv_field( KEY_DOWN );
}

/*
 *	function: 	move_up
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	move the cursor up to the previous field on the screen and 
 *	make it the current field, if necessary, scroll or wrap around.
 */
void
move_up() {
	msg_disp = 0;
	mv_field( KEY_UP );
}

/*
 *	function: 	move_left
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	move the cursor left to the previous field on the screen and 
 *	make it the current field, if necessary, scroll or wrap around.
 */
void
move_left() {
	msg_disp = 0;
	mv_field( KEY_LEFT );
}

/*
 *	function: 	move_right
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	move the cursor right to the next field on the screen and 
 *	make it the current field, if necessary, scroll or wrap around.
 */
void
move_right() {
	msg_disp = 0;
	mv_field( KEY_RIGHT );
}

/*
 *	function: 	toggle_memlock
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	locks and unlocks rtpm in memory with plock() call.
 *	also sets plocked flag to indicate whether rtpm is locked.
 */
void
toggle_memlock() {
	int i;
	int plock(int op);  /* can't find this in a header file */

	msg_disp = 0;
	if( !plocked ) {
		move( scr_rows - 1, plock_col-1 );
		for( i = plock_col-1; i < time_col; i++ )
			printw(" ");
		move( scr_rows - 1, plock_col );
		if( !plock( PROCLOCK ) ) {
			plocked = 1; 
			set_message_color( 0 );
			printw(gettxt("RTPM:10", "LOCKED"));
		} else {
			set_message_color( 0 );
			printw(gettxt("RTPM:11", "cannot lock"));
			beep();
			msg_disp |= LOCK_MSG_BIT;
		}
	} else {
		move( scr_rows - 1, plock_col-1 );
		for( i = plock_col-1; i < time_col; i++ )
			printw(" ");
                (void) plock( UNLOCK );
                plocked = 0;
                nice( 0 );
		print_uname();
	}
	set_default_color( 0 );
}

/*
 *	function: 	plot_or_push
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	either plot the current field or push to a subscreen
 *	associated with the current field, depending upon whether the 
 *	current field is numeric or a subscreen header.
 */
void
plot_or_push() {

	msg_disp = 0;
	switch( setplot( currfield[screendepth]->mp2, currfield[screendepth]->met2, currfield[screendepth]->r1, currfield[screendepth]->r2 ) ) {
	case 0:
		break;
	case 1:
		break;
	case -1:
		if( !push() )
			beep();
		break;
	}
}

/*
 *	function: 	pop_back
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	return to the previous screen, popping the current one
 */
void
pop_back() {
	msg_disp = 0;
	if( pop() ) {
		need_header |= METS;
	}
}

/*
 *	function: 	get_help
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	push the help screen 
 */
void
get_help() {
	msg_disp = 0;
	push_help();
	need_header |= METS;
}

/*
 *	function: 	set_user_procs
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	Set the lwp screen filtering to only display user procs
 *	Complain if the lwp screen is not currently being displayed.
 */
void
set_user_procs() {
	msg_disp = 0;
	if( screenfunc[screendepth] == print_lwp ) {
		if( set_ps_option_user() ) {
			need_header |= METS;
			scroll_ps( 0 );
		}
	} else beep();
}

/*
 *	function: 	set_sys_procs
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	Set the lwp screen filtering to only display system procs
 *	Complain if the lwp screen is not currently being displayed.
 */
void
set_sys_procs() {
	msg_disp = 0;
	if( screenfunc[screendepth] == print_lwp ) {
		if( set_ps_option_sys() ) {
			need_header |= METS;
			scroll_ps( 0 );
		}
	} else beep();
}

/*
 *	function: 	set_all_procs
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	Set the lwp screen filtering to display all procs.  This is
 *	the default state when rtpm is started.  Complain if the lwp 
 *	screen is not currently being displayed.
 */
void
set_all_procs() {
	msg_disp = 0;
	if( screenfunc[screendepth] == print_lwp ) {
		if( set_ps_option_all() ) {
			need_header |= METS;
			scroll_ps( 0 );
		}
	} else beep();
}

/*
 *	function: 	change_plot_type
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	Change the plotting format.  Format cycles through:
 *	FULLBARS (the default) -> FLOWERPLOT -> CONNECTBARS -> SCATTER 
 */
void
change_plot_type() {
	msg_disp = 0;
	if( find_plot() ) {
		switch( ptype ) {
		case FULLBARS:	ptype = FLOWERPLOT; break;
		case FLOWERPLOT:ptype = CONNECTBARS; break;
		case CONNECTBARS:ptype = SCATTER; break;
		case SCATTER:	ptype = FULLBARS; break;
		default: ptype = FULLBARS; break; /* shouldn't happen */
		}
		need_header |= PLOTS;
	} else beep();
}

/*
 *	function: 	scroll_up
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	Scroll the bargraph up (or down, depending on your perspective)
 *	Shows lower numbered cpus after completion.
 */
void
scroll_up() {
	msg_disp = 0;
	scroll_bar( -1 );
}

/*
 *	function: 	scroll_down
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	Scroll the bargraph down (or up, depending on your perspective)
 *	Shows higher numbered cpus after completion.
 */
void
scroll_down() {
	msg_disp = 0;
	scroll_bar( 1 );
}
/*
 *	function: 	scroll_left
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	Scroll metrics left (or right, depending on your perspective)
 *	Shows lower numbered disks/cpus/etherdevs after completion.
 */
void
scroll_left() {
	msg_disp = 0;
	if( screenfunc[screendepth] == print_lwp ) {
		if( scroll_ps( -1 ) )
			beep();
	} else if( scroll_met( -1, 1 ) )
		beep();
}

/*
 *	function: 	scroll_right
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	Scroll metrics right (or left, depending on your perspective)
 *	Shows higher numbered disks/cpus/etherdevs after completion.
 */
void
scroll_right() {
	msg_disp = 0;
	if( screenfunc[screendepth] == print_lwp ) {
		if( scroll_ps( 1 ) )
			beep();
	} else if( scroll_met( 1, 1 ) )
		beep();
}

/*
 *	function: 	increment_interval
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	Increment the sampling interval.  Resets curcook to
 *	the start of the history buffer and calls snap_mets.
 *	Plots are cleared and restarted with the new interval.
 *	Conceivably the plot data could be recalculated/compressed 
 *	from the existing history, but we don't...
 */
void
increment_interval() {
	msg_disp = 0;
	chg_interval( 1 );
	snap_mets();
	alarm( interval );
	if( find_plot() ) {
		need_header |= PLOTS;
	}
}

/*
 *	function: 	decrement_interval
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	If the sampling interval is greater than 1, decrement it.
 *	Resets curcook to the start of the history buffer and calls 
 *	snap_mets.  Plots are cleared and restarted with the new interval.
 *	Conceivably the plot data could be interpolated/expanded 
 *	from the existing history...
 */
void
decrement_interval() {
	msg_disp = 0;
	if( interval > 1 ) {
		chg_interval( -1 );
		snap_mets();
		alarm( interval );

		if( find_plot() ) {
			need_header |= PLOTS;
		}
	} else beep();
}

/*
 *	function: 	chg_interval
 *
 *	args:		requested increment
 *
 *	ret val:	none
 *
 *	Called by increment_interval() and decrement_interval().
 *	Change the sampling interval and reset curcook and rollover.
 *	to restart the history buffer.  Call print_interval() to
 *	update the display on the bottom row of the screen with the 
 *	current requested and actual intervals.
 */
void
chg_interval( int incr ) {
	assert((interval + incr) > 0 );
	interval += incr;
	curcook = 0; rollover = 0;
	print_interval();
}

/*
 *	function: 	clear_a_plot
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	Clear the first thing on the plotdata list, which is either
 *	the bargraph or the oldest plot the user has requested.
 *	If the only thing on the plotdata list is the bargraph,
 *	clear_a_plot complains.
 */
void
clear_a_plot() {
	msg_disp = 0;
	if( clrplot() ) {
		need_header |= PLOTS;
	} 
	else beep();
}

/*
 *	function: 	clrbar
 *
 *	args:		none
 *
 *	ret val:	0 - no bar graph to clear
 *			1 - cleared the bar graph
 *
 *	Clear the bargraph.  If there's a stick plot underneath,
 *	let it return, otherwise, clear the plotdata entry.
 */
clrbar() {
	msg_disp = 0;
	if( !plotdata->barg_flg )
		return(0);
	if( plotdata->mp ) {
		plotdata->barg_flg = 0;
		plotdata->rows = -1;
		need_header |= PLOTS;
		return(1);
	}
	if( clrplot() ) {
		need_header |= PLOTS;
		return(1);
	}
	return(0);
}

/*
 *	function: 	toggle_bargraph
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	If it's not already there, put the bargraph up on the screen.
 *	Otherwise, remove it.
 */
void
toggle_bargraph() {
	if( setbar() || clrbar() ) {
		need_header |= PLOTS;
	} else beep();
}

/*
 *	function: 	redraw_the_screen
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	Redraw the screen, presumably after it's been hosed.
 */
void
redraw_the_screen() {
	redrawwin( stdscr );
	msg_disp = 0;
	print_time();
	highlight_currfield();
}

/*
 *	function: 	quit
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	clean up curses and exit.
 */
void
quit() {
	endwin();
	printf("\n");
	exit(0);
}

/*
 *	function: 	bad_input_keystroke
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	complain and put the help teaser at the bottom of the screen.
 */
void
bad_input_keystroke() {
	beep();
	print_help_msg();
}

/*
 *	function: 	do_input
 *
 *	args:		input character
 *
 *	ret val:	none
 *
 *	run through the list of valid keystrokes in the keyin struct
 *	and look for a function to call.  Keystrokes are kept as
 *	an int value, for things like KEY_RIGHT, and as a string
 *	for normal chars.
 */
void
do_input( int inch ) {
	int i;
	char *p;

	for ( i = 0; i < NKEYS; i++ ) {
		if( keystrokes[i].intval == inch ) {
			(*keystrokes[i].func)();
			return;
		}
		if( (p = &keystrokes[i].str[0]) != NULL ) {
			while( *p ) {
				if( inch == (int)((*p) & 0x00ff ) ) {
					(*keystrokes[i].func)();
					return;
				}
				p++;
			}
		}
	}
	bad_input_keystroke();
}

/*
 *	function: 	pop
 *
 *	args:		none
 *
 *	ret val:	1 - successful pop
 *			0 - already at top level screen
 *
 *	pop the current screen and return to the previous screen.
 */
pop() {
	int saverow = currfield[screendepth]->row;
	int savecol = currfield[screendepth]->col;
	struct field *fp;

	if( screendepth ) {
		clr_fields();	/* clear all fields */
		screendepth--;
		metscroll = 0;
		canscroll = 0;
		scrollcol = 0;
/*
 *		if the current field was up in the plot area
 *		restore it to that position, otherwise let it go back
 *		to where it was in the metrics area before the push
 */
		if( saverow <= disprows ) {
			for( fp = froot[screendepth]; fp; fp=fp->nxt )
				if( fp->row == saverow 
				  && fp->col == savecol ) {
					currfield[screendepth] = fp;
					break;
				}
		}
		return(1);
	}
	beep();
	return(0);	/* already at top level screen */
}


/*
 *	function: 	mv_field
 *
 *	args:		direction
 *
 *	ret val:	none
 *
 *	Move the cursor in the requested direction to the next field on
 *	the screen.  If scrolling is in effect, may cause scrolling of
 *	the metrics, may also wrap around the screen.
 */
void
mv_field( int dir ) {
	int row, col, hdr_cnt;
	struct field *stp, *tp;


	assert(currfield[screendepth]);
	assert(froot[screendepth]);
/*
 *	Un-highlight the old currfield
 */
	lolight_currfield();
/*
 *	Fields are kept sorted by row and subsequently by column.  
 *	There are probably many better ways to track where the
 *	fields are located on the screen, which would simplify
 *	this function greatly, but would make inserting fields
 *	into the list more expensive.
 *
 *	If we're moving left or right from a subscreen header, 
 *	skip the intervening fields and go to next header. If 
 *	there's only one header on the screen, this would cycle 
 *	through every field on the screen and take us back to 
 *	where we started, so don't bother.
 */
	hdr_cnt = 0;
	if( currfield[screendepth]->func  		/* subscreen hdr*/
	  && currfield[screendepth]->func != nuke_plot
	  && ( dir == KEY_LEFT || dir == KEY_RIGHT )) { /* lft or right	*/
		hdr_cnt = -1;
		for( stp = froot[screendepth]; stp; stp = stp->nxt )
			if( stp->func && stp->func != nuke_plot )
				hdr_cnt++;
	}
keep_on_truckin:
	row = currfield[screendepth]->row;		/* current row	*/
	col = currfield[screendepth]->col;		/* current col	*/

	switch( dir ) {			/* dirs are defined in curses.h */

	case KEY_RIGHT:
/*
 *		If there is a next field on the list and it's not
 *		on the same row as the current field, try to scroll
 */
		if( currfield[screendepth]->nxt 
		  && currfield[screendepth]->nxt->row != row ) {
			scroll_met( 1, 0 );
		}
/*
 *		If there still is a next field on the list and it's not
 *		on the same row as the current field, scrolling didn't
 *		help, so scroll all of the way back in the other direction
 */
		if( currfield[screendepth]->nxt 
		  && currfield[screendepth]->nxt->row != row ) {
			scroll_met( -metscroll, 0 );
		}
/*
 *		Get the next field on the list
 */
		stp = currfield[screendepth]->nxt;
/*
 *		If the next field on the list is NULL, we are at
 *		the end of the list.  If canscroll is set, then
 *		we can try to scroll.
 */
		if( !stp && canscroll ) {
			scroll_met( 1, 0 );
		}
/*
 *		Get the next field on the list again, since it may 
 *		have changed by scrolling.
 */
		stp = currfield[screendepth]->nxt;
/*
 *		If the next field on the list still NULL, we are at
 *		the end of the list.  If metscroll is set, then
 *		we can have to scroll everything back before we 
 *		wrap back to the start of the list
 */
		if( !stp ) {
			if( metscroll )
				scroll_met( -metscroll, 0 );
			stp = froot[screendepth];
		}
		break;

	case KEY_DOWN:
/*
 *		look for the first entry we find with the same column
 *		and a row that is greater than the current row
 */
		stp = NULL;
		for( tp = currfield[screendepth] ; tp ; tp=tp->nxt )
			if( tp->row > row && tp->col == col ) {
				stp = tp;
				break;
			}
		if( stp )
			break;
/*
 *		didn't find an entry in the same column with a
 *		greater row.  Look for an entry which has a column
 *		greater than the current column.  There may be many
 *		entries with columns greater than the current column,
 *		but we want the first in the list with the minimum 
 *		column that is still greater than the current columns.
 *		This will be the entry that is closest entry to the 
 *		right of where we are and the topmost in that column.
 *		In other words, wrap down off the bottom to the upper
 *		right of where we are.
 */
		do {
			stp = NULL;
			for( tp = froot[screendepth]; tp ; tp = tp->nxt )
				if( tp->col > currfield[screendepth]->col){
					if( !stp )
						stp = tp;
					else if( tp->col < stp->col )
						stp = tp;
				}
/*
 *		If stp is still NULL, we weren't able to wrap off the 
 *		bottom, try to scroll and check again to see if we can 
 *		wrap.
 */
		} while( !stp && !scroll_met( 1, 0 ) );

		if( stp ) 
			break;
/*
 *		Couldn't wrap off the bottom, this means we are at then 
 *		end of the list.  Scroll everything back and look for the 
 *		first entry in the list with the smallest column.  This 
 *		may not be the first entry in the list, since entries are 
 *		sorted by row and then by column.
 */
		scroll_met( -metscroll, 0 );
		stp = froot[screendepth];
		for( tp = froot[screendepth]; tp ; tp = tp->nxt )
			if( tp->col < stp->col ) {
				stp = tp;
			}

		break;
	case KEY_LEFT:
/*
 *		Scrolling of metrics doesn't go all of the way to
 *		the left of the screen.  The left column is reserved
 *		for the total.  scrollcol is set to the left-hand 
 *		column where scrolling takes place, which is one column
 *		to the right of the column where the totals are displayed.
 *		If metscroll is greater than zero, then we've done 
 *		some scrolling to the right.  If the current position
 *		is scrollcol, scroll to the left by 1.
 */
		if( metscroll && col == scrollcol )
			scroll_met( -1, 0 );
/*
 *		Look for an entry that immediately precedes the current
 *		entry on the list.  If found, and it's on the same row,
 *		then this is the entry we want.  If it's not on the same
 *		row as the current entry, then we are wrapping around
 *		off the left of the screen up and to the right.  If this 
 *		is the case, we have to scroll everything as far to the 
 *		right as possible.   Having done the scrolling, we have
 *		to look for the preceding entry on the list again, since
 *		it probably changed.
 */
	
again:		stp = NULL;		
		for( tp = froot[screendepth]; tp->nxt; tp=tp->nxt )
			if( tp->nxt == currfield[screendepth] ) {
				stp = tp;
				break;			
			}
/*
 *		If the rows don't match, and we can scroll, and the
 *		column we're examining is on the right edge of the screen,
 *		scroll as far as possible.
 */
		if( tp->row != row && canscroll 
		  && (tp->col+18) >= scr_cols ) {
			while( !scroll_met( 1, 0 ) )
				;
			goto again;
		}
/*
 *		If stp is NULL, then there is no preceding entry on list.
 *		We've already scrolled everything to the right as far 
 *		possible, so move the the last entry in the list, which 
 *		is at the right hand end of the bottom-most row.
 */		
		if( !stp )
			stp = tp;
		break;

	case KEY_UP:
/*
 *		Look for the last entry that is in the same column as the 
 *		current entry and has a row less than the current entry.
 *		This is the entry immediately above the current entry.
 */
		stp = NULL;
		for( tp = froot[screendepth] ; tp ; tp=tp->nxt )
			if( tp->row < row && tp->col == col )
				stp = tp;
		if( stp )
			break;
/*
 *		Didn't find an entry immediately above the current entry.
 *		This means we will have to wrap (warp?) off the top of 
 *		the screen to the left and down.  If we are at scrollcol,
 *		and we have done some scrolling to the right, we need to
 *		scroll back to the left by 1.
 */
		if( metscroll && col == scrollcol )
			scroll_met( -1, 0 );
/*
 * 		Look for the first entry whose column is nearest to
 *		the left of the current column.
 */
		for( tp = froot[screendepth] ; tp ; tp=tp->nxt )
			if( tp->col < currfield[screendepth]->col )
				if(!stp) 
					stp = tp;
				else
					if( tp->col >= stp->col )
						stp = tp;
		if( stp )
			break;
/*
 *		Didn't find any entries to the left of the current entry.
 *		This means we are as at the top of the left-most column,
 *		and we are going to wrap to the bottom right of the screen.
 *		If we can scroll, scroll everything as far to the right 
 *		as possible.
 */

		while( !scroll_met( 1, 0 ) ) 
			;
/*
 *		Look for the last entry with the greatest column.
 *		This will be the bottom of the right-most column
 *		on the screen.  (Note stp == NULL here).
 */
		for( tp = froot[screendepth] ; tp ; tp=tp->nxt )
			if(!stp) 
				stp = tp;
			else
				if( tp->col >= stp->col )
					stp = tp;
		break;
	}
/*
 *	We've moved stp in the requested direction from currfield.
 *	If we started at a subscreen header, keep moving until 
 *	we get to another one.
 */
	if( hdr_cnt ) {
		if( !stp->func || stp->func == nuke_plot ) {
			/* didn't find a subscreen hdr */
			currfield[screendepth] = stp;
			goto keep_on_truckin;
		}
	}
/*
 *	Set currfield to stp, and highlight it
 */
	currfield[screendepth] = stp;
	highlight_currfield();
}

/*
 *	function: 	setplot
 *
 *	args:		metric struct, metric history buffer, instance id's
 *
 *	ret val:	 1:  succeeding in plotting current field
 *			 0:  current field already plotted
 *			-1:  nothing to plot
 *
 *	add the metric associated with the arguments to the plotdata
 *	list.  This may involve taking something off of the list to make 
 *	room.
 */
setplot( struct metric *mp, float *met, int r1, int r2 ) {
	int cnt;	/* count of plots and bargraphs */
	struct plotdata *pd;
	int foo, bar;
/*
 *	make sure we have a metric to plot, if not, return -1
 */
	if (!met) {
		return(-1);		/*  nothing to plot */
	}
/*
 *	Check to see if the metric associated with the current field
 *	is already plotted
 */
	for( pd = plotdata; pd; pd = pd->nxt )
		if( pd->met == met ) {
			return(0);	/* already plotted */
		}
/*
 *	We have a metric to plot, and it's not already plotted.
 *	Sum up how many things are already plotted and see if we
 *	need to make room for the new plot.
 */
 	cnt = 0;
	for( pd = plotdata; pd; pd = pd->nxt )
		cnt++;
/*
 *	if not set, set maxplots based on screen size
 */
	if( maxplots <= 1 )
		justify( &foo, &bar );

	if( cnt >= maxplots ) {
		/* no room on screen, make some */
		clrplot();
	}

/*
 *	run the links to the end of the plotdata list and add an entry
 *	at the tail.
 */
	for( pd = (struct plotdata *)&plotdata; pd->nxt; pd = pd->nxt )
		;
	if( !(pd->nxt = (struct plotdata *)malloc( sizeof( struct plotdata )))){
		endwin();
		fprintf(stderr,gettxt("RTPM:7", "out of memory\n"));
		exit(1);
	}
	pd = pd->nxt;
	pd->nxt = NULL;
	pd->rows = -1;
	pd->barg_flg = 0;
	pd->met = met;
	pd->mp = mp;
	pd->r1 = r1;
	pd->r2 = r2;
	need_header |= PLOTS;
	return(1);
}


/*
 *	function: 	setbar
 *
 *	args:		none
 *
 *	ret val:	0:  bargraph already plotted
 *			1:  added bargraph to plot list
 *
 *	add the bargraph to the head of the plotdata list.
 *	This may involve taking something off of the list to make room.
 */
setbar() {
	int cnt;
	struct plotdata *pd;

	if( plotdata->barg_flg )
		return(0);	/* bargraph is already displayed */
/*
 *	sum up the number of things that are plotted, see if we have 
 *	enough room on the screen.
 */
	cnt = 0;
	for( pd = plotdata; pd; pd = pd->nxt )
		cnt++;
#ifdef NOT_STICKY
	if( cnt >= maxplots ) {
		/* no room on screen, make some */
		clrplot();
	}

#else
	if( cnt >= maxplots ) {
		/* no room on screen, make some */
		plotdata->barg_flg = 1;
		return(1);
	}
#endif

/*
 *	Add the bargraph entry to the front of the plotdata list.
 */
	if( !(pd = (struct plotdata *)malloc( sizeof( struct plotdata )))){
		endwin();
		fprintf(stderr,gettxt("RTPM:7", "out of memory\n"));
		exit(1);
	}
	pd->nxt = plotdata;
	plotdata = pd;
	pd->zrow = 0;
	pd->zcol = 0;
	pd->rows = -1;
	pd->barg_flg = 1;
	pd->met = NULL;
	pd->mp = NULL;
	pd->r1 = 0;
	pd->r2 = 0;
	need_header |= PLOTS;
	return(1);
}


/*
 *	function: 	push
 *
 *	args:		none
 *
 *	ret val:	0:  couldn't push, curr field not a subscreen hdr
 *			1:  succeeded
 *
 *	display a subscreen by pushing it on the top of the display
 *	stack.
 */
push() {
	struct field *f, *tp;

	if (!currfield[screendepth]->func) {
		/* not a subscreen header */
		return(0);
	}
	if (currfield[screendepth]->func == nuke_plot) {
		nuke_plot();
		return(1);
	}
/*
 *	copy the fields for the plot labels
 */
	tp = (struct field *)&froot[screendepth+1];
	for( f = froot[screendepth] ; f ; f=f->nxt ) {
		if( f->row > disprows )
			break;
		if(!(tp->nxt = (struct field *)malloc( sizeof( struct field ) ))) {
			endwin();
			fprintf(stderr,gettxt("RTPM:7", "out of memory\n"));
			exit(1);
		}
		tp = tp->nxt;
		tp->row = f->row;
		tp->col = f->col;
		tp->len = f->len;
		tp->fmt = f->fmt;
		tp->met = f->met;
		tp->mp = f->mp;
		tp->met2 = f->met2;
		tp->mp2 = f->mp2;
		tp->r1 = f->r1;
		tp->r2 = f->r2;
		tp->func = f->func;
	}
	tp->nxt = NULL;
	screenfunc[screendepth+1] = currfield[screendepth]->func;
	screendepth++;
	need_header |= METS;
	return(1);
}


/*
 *	function: 	push_help
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *
 *	if the current screen is not the help screen, push the help screen.
 *	on the top of the display stack.
 */
void
push_help() {
	if( screenfunc[screendepth] != print_help ) {
		screenfunc[screendepth+1] = print_help;
		screendepth++;
		need_header |= METS;
	}
}


/*
 *	function: 	scroll_met
 *
 *	args:		increment - amount to scroll
 *			hilight_flag - whether or not to highlight 
 *			  currfield when done
 *
 *	ret val:	 0: successful completion 
 *			    ( possibly because incr == 0 )
 *			-1: can't scroll
 *
 *	Try to scroll metrics in the requested direction.
 *	The actual determination whether we can scroll is
 *	done in the metric printing functions, since it's 
 *	based on screen size.  Scroll_met changes the 
 *	requested scroll value (metscroll).
 */
scroll_met( int incr, int hilight_flag ) {
	float *met = currfield[screendepth]->met;
	int row = currfield[screendepth]->row;
	int col = currfield[screendepth]->col;
	struct field *fp;

	if( !incr )
		return(0);	/* nothing to do */

	if( (incr > 0) && !canscroll ) {
/*
 *		requested a scroll to the right, but the metrics 
 *		are not off the right hand edge of the screen.
 *		(possibly because we've already scrolled all the way).
 */
		return(1);
	}

	if( (incr < 0) && (metscroll <= 0) ) {
/*
 *		requested a scroll to the left, but the metrics 
 *		are already scrolled all of the way to the left.
 */
		return(1);
	}
/*
 *	The increment is permissible, give it a shot.  Clear the scroll
 *	column and flag and increment metscroll.  Then clear the metrics
 *	from the screen and clear all of the fields on the screen.  Then
 *	call print_mets to reconstruct everything with the new scroll
 *	value.
 */
	canscroll = 0;
	scrollcol = 0;
	metscroll += incr;
	need_header |= METS;
	move( disprows + 1, 0 );
	color_clrtobot( disprows+1, 0 );
	lolight_currfield();
	clr_metfields();	/* clear metric fields */
	print_mets();	/* this is needed here to restore the fields */
/*
 *	Now we have to restore currfield to same metric as before.
 *	This may not be possible because we may have scrolled the
 *	current field off the screen.  First we check to see if
 *	we can find a field on the screen with the same metric address
 *	as the one we had.  If so, that's the one we want.  If not,
 *	then we search for a field that has the same row and column
 *	as the one we had, figuring the one we had got scrolled off.
 */
	lolight_currfield();
	for( fp = froot[screendepth] ; fp ; fp=fp->nxt ) {
/*
 *		is this the same field we had before
 */
		if( fp->met == met ) {
			currfield[screendepth] = fp;
			if( hilight_flag )
				highlight_currfield();
			break;
		}
	}
	if( !fp || fp->met != met ) {
/*
 *		currfield is not onscreen anymore, leave currfield
 *		at the same coordinates at it was before
 */
		for( fp = froot[screendepth] ; fp ; fp=fp->nxt ) {
			if( fp->row == row && fp->col == col ) {
				currfield[screendepth] = fp;
				if( hilight_flag )
					highlight_currfield();
				break;
			}
		}

	}
	return(0);
}


/*
 *	function: 	scroll_bar
 *
 *	args:		increment
 *
 *	ret val:	none
 *
 *	Try to scroll the bargraph in the requested direction.
 *	If unsuccessful, beep and return.
 *	The actual determination whether we can scroll is
 *	done in the bargraph printing function, since it's 
 *	based on screen size.  Scroll_bar changes the 
 *	requested scroll value (barscroll).
 */
void
scroll_bar( int incr ) {
	struct plotdata *pd;

/*
 *	Search the plot list for the bargraph
 */
	for( pd = plotdata; pd; pd = pd->nxt )
		if( pd->barg_flg )
			break;

	if( !pd ) {	/* no bargraph plotted */
		beep();
		return;
	}

	if( (incr > 0) && (ncpu <= (pd->plotrows + barscroll) ) ) {
/*
 *		requested a scroll down, but the bargraphs
 *		are not off the bottom of the plot.
 *		(possibly because we've already scrolled all the way).
 */
		beep();
		return;
	}
	if( (incr < 0) && (barscroll <= 0) ) {
/*
 *		requested a scroll up, but the bargraphs
 *		are not off the top of the plot.
 */
		beep();
		return;
	}
/*
 *	change barscroll and call bar to re-plot
 */
	barscroll += incr;
	need_header |= PLOTS;
	bar( pd->zrow, pd->zcol, pd->plotrows, pd->plotcols, pd ); 
	need_header &= ~PLOTS;
}


/*
 *	function: 	find_plot
 *
 *	args:		none
 *
 *	ret val:	1: plot found
 *			0: no plot found
 *
 *	Look to see if there is a plot (not a bargraph) 
 *	in the plotdata list.
 */
find_plot() {
	struct plotdata *pd;
/*
 *	Search the plot list for a plot
 */
	for( pd = plotdata; pd; pd = pd->nxt )
		if( !pd->barg_flg )
			return(1);
	return(0);
}

/*
 *	function: 	toggle_underscore
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	change underscore handling to get around buggy terminfo entries
 */
void
toggle_underscore() {
	msg_disp = 0;
	does_underline = !does_underline;
	if( plotdata ) {
		need_header |= PLOTS;
	}
}
