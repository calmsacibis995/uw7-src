#ident	"@(#)rtpm:output.c	1.7.2.2"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <assert.h>
#include <mas.h>
#include <metreg.h>
#include <curses.h>
#include <sys/dl.h>
#include <sys/utsname.h>
#include <time.h>
#include <sys/times.h>
#include "rtpm.h"
#include "mtbl.h"
/*
 * the current lbolt time
 */
extern long currtime;
/*
 *  a struct tms to pass to times()
 */
extern struct tms tbuf;
/*
 * time difference in seconds between the current and previous samples
 */
extern float tdiff;
/*
 * requested sampling interval
 */
extern int interval;
/*
 * maximum number of history points to keep
 */
extern int maxhist;
/*
 * the metric table
 */
extern struct metric mettbl[];
/*
 * flag for term supports underline mode or should use "_"
 */
extern int does_underline;
/*
 * color ranges for bargraph
 */
extern struct color_range *bargraph_color;

struct field *froot[ MAXSCREENDEPTH ] = { NULL, NULL, NULL, NULL, NULL };
struct field *currfield[ MAXSCREENDEPTH ] = { NULL, NULL, NULL, NULL, NULL };
/*
 * screenfunc is an array ptrs to functs, screenfunc[screendepth]
 * is the address of the function that draws the current screen
 */
void (*screenfunc[MAXSCREENDEPTH])() = { print_sum, NULL, NULL, NULL, NULL };

int msg_disp = 0;	/* put help message at bottom of screen?	*/
int screendepth = 0;	/* how many subscreens are pushed		*/
int need_header = ALL;	/* print header / redraw screen flag		*/
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
 * previous timestamp
 */
extern int oldtime;
/*
 * flag for whether eisa bus utilizatiopn metric is present
 */
extern int eisa_bus_util_flg;
/* 
 * flag indicates whether or not rtpm is locked in core
 */
extern int plocked;
/*
 * progname is argv[0]
 */
extern char *progname;
/*
 * columns at which to put things on bottom of screen, and the max len 
 * of the plock message.
 */
int plock_col;
int time_col;
static int interval_col;
static int plock_len = 0;
/*
 * linked list of things that are plotted (includes the bargraph)
 */
struct plotdata *plotdata = NULL;
/*
 * misc plot variables
 */
int ptype = FULLBARS;	/* default plot type				*/
int maxplots = 1;	/* total max num of plots fit on the screen	*/
int maxvplots = 1;	/* max vertical number of plots			*/
int maxhplots = 1;	/* max horizontal number of plots		*/
int disprows = 7;	/* number of rows for plot display		*/
int metrows;		/* number of rows used by metrics		*/
int metcols;		/* number of columns used by metrics		*/
int barscroll = 0;	/* amount bargraph has been scrolled		*/
int metscroll = 0;	/* amount metrics have been scrolled		*/
int canscroll = 0;	/* are there too many metrics to fit on screen?	*/
int scrollcol = 0;	/* kludge keeps total on screen while scrolling */
/*
 * spx limit and sap limits
 */
extern int spx_mused_conn;
extern int nlans;

#define print_perfs_hdr( row, col ) print_hdr((row), (col), NFSTYP, nfstyp)
#define print_percpu_hdr( row, col ) print_hdr((row), (col), NCPU, ncpu)
#define print_perdisk_hdr( row, col ) print_hdr((row), (col), NDISK, ndisk)
#define print_pereth_hdr( row, col ) print_hdr((row),(col), NETHER, nether)
#define print_perspx_hdr( row, col ) print_hdr((row), (col), SPX_max_connections, maxspxconn)
#define print_perlan_hdr( row, col ) print_hdr((row), (col), SAP_Lans, nlans)

/*
 *	function: 	format_met
 *
 *	args:		fmt - formatting string
 *			met - metric to format
 *
 *	ret val:	ptr to static char buffer to display
 *
 *	This function makes sure the metric doesn't overflow the field 
 *	width.
 */
char *
format_met( char *fmt, double met ) {
	static char buf[32];
	char myfmt[16];
	int flen, slen, ldiff, overhead_len;


	if( fmt[0] != '%' || fmt[1] > '9' || fmt[1] < '0' || fmt[2] != '.' 
	  || fmt[3] > '9' || fmt[3] < '0' || fmt[4] != 'f' 
	  || fmt[5] != '\0' ) {
#ifdef DEBUG
		endwin();
		fprintf(stderr,"Format error detected in format_met()\n");
		fprintf(stderr,"Bad format string is:'%s'\n",fmt);
		exit(1);
#endif
		sprintf( buf, fmt, met );
		return( buf );
	}

	/* does it fit in the alloted space ? */
	flen = fmt[1] - '0';
	sprintf( buf, fmt, met );
	slen = strlen( buf );
	if( slen <= flen )
		return( buf );

	/* how much over is it?  will removing the decimal places do it? */
	ldiff = slen - flen;
	strcpy( myfmt, fmt );
	if( fmt[3] > '0' && (fmt[3] - '0') >= (ldiff-1) ) {
		myfmt[3] -= ldiff;
		sprintf( buf, myfmt, met );
		return( buf );
	}

	/* nope, got to use exponential notation */
	overhead_len = 6;
	if( met < 0.0 )
		overhead_len = 7;
	myfmt[4] = 'e';
	myfmt[3] = flen - overhead_len + '0';
	if( flen < overhead_len )
		myfmt[3] = '0';
	sprintf( buf, myfmt, met );
	return( buf );
}

/*
 *	function: 	mk_field
 *
 *	args:		metp - pointer to metric in mettbl to display
 *			metp2 - ptr to alternate metric in mettbl to plot
 *			r1, r2 - values of resources for this instance
 *			row,col,len,fmt - screen position, length & format
 *			met, met2 - addrs of instances for metp and metp2.
 *			func - func call if this a subscreen hdr (or NULL)
 *
 *	ret val:	ptr to the field
 *
 *	Add a field to the current screen.  Fields are kept sorted by row
 *	and then by columns within a row.  This make insertion relatively
 *	easy, but makes scrolling and wrapping around cumbersome.  The
 *	scrolling and wrapping operations are really the ones that should
 *	have been optimized, perhaps with double links for rows and cols.
 *	Maybe next time.
 */
struct field *
mk_field( struct metric *metp, struct metric *metp2, int r1, int r2, int row, int col, int len, char *fmt, float *met, float *met2, void (*func)() ) {
	struct field *fp, *tp;

/*
 *	search for the position within the list where this row should be
 *	inserted
 */
	for( fp = (struct field *)&froot[screendepth] ; fp->nxt ; fp=fp->nxt )
		if( fp->nxt->row >= row )
			break;

/*
 *	search for the position within the row where this col should be
 *	inserted
 */
	for( ; fp->nxt && fp->nxt->row == row ; fp=fp->nxt )
		if( fp->nxt->col >= col )
			break;
/*
 *	If there's nothing at this row and col, or if the existing field
 *	differs from the requested one, allocate a new field
 *	and initialize its elements.
 *
 *	Two fields can land at the same row,col is when the plots are 
 *	being re-positioned.  In this case, the old field will be cleared
 *	and moved shortly.
 */
	if( !fp->nxt || fp->nxt->row != row || fp->nxt->col != col 
	  || fp->nxt->fmt != fmt ) {
		if(!(tp = (struct field *)malloc( sizeof( struct field ) ))) {
			endwin();
			fprintf(stderr,gettxt("RTPM:7", "out of memory\n"));
			exit(1);
		}
		tp->row = row;
		tp->col = col;
		tp->len = len;
		tp->fmt = fmt;
		tp->met = met;
		tp->mp = metp;
		tp->met2 = met2;
		tp->mp2 = metp2;
		tp->r1 = r1;
		tp->r2 = r2;
		tp->func = func;

		tp->nxt = fp->nxt;
		fp->nxt = tp;
	}
	fp = fp->nxt;
	move( row, col );
/*
 *	If this is the current field, print it highlighted,
 *	otherwise, just print it.
 */
	if( fp == currfield[screendepth] ) {
		if( met )  {
			set_metric_color( (double) met[curcook], fp->mp->color, A_STANDOUT, 0 );
			printw( format_met(fmt, (double)met[curcook]) );
		} else if( func ) {
			set_header_color( A_STANDOUT );
			printw( fmt );
		} else {
			set_default_color( A_STANDOUT );
			printw( fmt );
		}
		attroff( A_STANDOUT );
	} else {
		if( met )  {
			set_metric_color( (double) met[curcook], fp->mp->color, 0, 0 );
			printw( format_met( fmt, (double)met[curcook]) );
		} else if( func ) {
			set_header_color( 0 );
			printw( fmt );
		} else {
			set_default_color( 0 );
			printw( fmt );
		}
	}
	set_default_color( 0 );
	return( fp );
}

/*
 *	function: 	clr_fields
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	clear all of the fields associated with the current screen display
 *	and reset currfield to NULL.
 */
void
clr_fields() {
	struct field *tp;

	while( (tp = froot[screendepth]) != NULL ) {
		froot[screendepth] = froot[screendepth]->nxt;
		free( tp );
	}
	currfield[screendepth] = NULL;
}

/*
 *	function: 	clr_metfields
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	clear all of the metric fields associated with the current 
 *	screen display.
 */
void
clr_metfields() {
	struct field *tp, *xp;

	for( tp = (struct field *)&froot[screendepth]; tp->nxt;  ) {
		if( !tp->nxt->func ) {
			xp = tp->nxt;
			tp->nxt = tp->nxt->nxt;
			free( xp );
		} else
			tp = tp->nxt;
	}
	currfield[screendepth] = NULL;
}


/*
 *	function: 	clrplot
 *
 *	args:		none
 *
 *	ret val:	1 - cleared a plot
 *			0 - nothing to clear
 *
 *	clear the oldest plot on the screen, unless the only thing
 *	on the screen is the bargraph.  This could be modified to
 *	allow the bargraph to be cleared, but there's also a hook
 *	in print_mets that would have to be disabled.  It's kind of a 
 *	preference choice as to whether you want to have to ask to
 *	have it displayed, or if you want to have to ask to have it
 *	cleared.  Right now it's set to the latter.
 */
clrplot() {
	struct plotdata *p;
	struct field *fp, *tp;
	int depth;


#ifdef DEFAULT_BAR
/*
 *	if the only thing plotted is the bargraph, don't do
 *	anything.
 */
	assert( p );	/* p may be NULL when DEFAULT_BAR is not defined */
	if( p->barg_flg  && !p->nxt )
		return(1);
#endif

	if( !plotdata ) 
		return(0);
/*
 *	clear the oldest thing on the screen and
 *	all the fields (at all screendepths)
 *	associated with this plot title.
 */
	p = plotdata;
	for( depth = 0; depth <= screendepth; depth++ ) {
		for( fp = (struct field*)&froot[depth]; fp->nxt;  ) {
			if( fp->nxt->fmt == p->title ) {
				tp = fp->nxt;
				fp->nxt = fp->nxt->nxt;
				if( currfield[ depth ] == tp )
					currfield[depth] = tp->nxt;
				assert( currfield[depth] );
				free( tp );
			} else {
				fp = fp->nxt;
			}
		}
	}
	plotdata = plotdata->nxt;
	free(p);
	return(1);
}


/*
 *	function: 	nuke_plot
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	clear the plot on the screen associated with currfield
 */
void
nuke_plot() {
	int depth;
	struct plotdata *p, *tp;
	struct field *f, *tf;

	for( p = (struct plotdata *)&plotdata; p->nxt; p = p->nxt )
		if( p->nxt->title == currfield[screendepth]->fmt )
			break;
	tp = p->nxt;
	p->nxt = p->nxt->nxt;
	free(tp);

	for( depth = 0; depth <= screendepth; depth++ ) {
		for( f=(struct field *)&froot[depth] ; f->nxt; f=f->nxt) {
			if( f->nxt->fmt == currfield[screendepth]->fmt ) {
				tf = f->nxt;
				f->nxt = tf->nxt;
/*
 *				Reset currfield to something reasonable. 
 */
				if( currfield[depth] == tf )
					currfield[depth] = f->nxt;
				assert( currfield[depth] );
				free( tf );
			}
		}
	}
	need_header |= PLOTS;
}


/*
 *	function: 	print_hdr
 *
 *	args:		row and column
 *			resource, which is one of: NCPU, NETHER, NFSTYP, 
 *			  NDISK, SAP_Lans (nlans), or
 *			  SPX_max_used_connections(spx_mused_conn)
 *
 *			resource value
 *
 *	ret val:	none
 *
 *	if need_header is set:
 *		print the resource labels.  If necessary, print the scroll 
 *		indicators at the end of the line.
 */
void
print_hdr( int row, int col, resource_t resource, int resval ) {
	int i,j;
	char *totstr;
	char buf[256];

	if( need_header & METS ) {
		set_label_color( 0 );
		totstr = gettxt("RTPM:50", "total");

		move( row, col + 19 - strlen( totstr ) );
		printw("%s",totstr);
/*
 *		If metscroll is set, we've already done some scrolling to
 *		the right, so we put "<-" at the right edge of the screen.
 *		This may be overwritten below if there are more mets that
 *		are invisible off the right of the screen.
 */

		if( metscroll ) {
			set_message_color( 0 );
			move( row, scr_cols-2 );
			printw(gettxt("RTPM:612", "<-"));
			scrollcol = col+20;
		}
/*
 *		Starting with metscroll, count the number of metrics
 *		we have to display horizontally.  If the number rolls off 
 *		the right-hand edge of the screen, set canscroll and 
 *		adjust the indicator on the right-hand edge.
 */
		for(j=0,i=metscroll; i < resval; i++,j++) {
			if((col + 20 + j*9 + 9) >= scr_cols ) {
				canscroll = 1;
				set_message_color( 0 );
				if( !metscroll ) {
					move( row, scr_cols-2 );
					printw(gettxt("RTPM:613", "->"));
				} else {
					move( row, scr_cols-2 );
					printw(gettxt("RTPM:614", "<>"));
				}
				break;
			}
			set_label_color( 0 );
			switch( resource ) {
			case NCPU:
				sprintf( buf, gettxt("RTPM:51", "cpu %02d"),i);
				break;
			case NDISK:
				strncpy( buf, dsname[i].met_p, 9);
				break;
			case NFSTYP:
				strncpy( buf, fsname[i].met_p, 9);
				break;
			case NETHER:
				strncpy( buf, ethname[i].met_p, 9);
				break;
			case SAP_Lans:
				sprintf(buf, gettxt( "RTPM:978","   lan_%02d"), i );
				break;
			case SPX_max_connections:
				sprintf(buf, gettxt( "RTPM:979"," spx_%02d"), i );
				break;
			default:
#ifdef DEBUG
				endwin();
				fprintf(stderr,"DEBUG unknown resource in print_hdr\n");
				exit(1);
#endif
				break;
			}
			move( row, col + 20 + j*9 );
			printw("%8s",buf);
		}
	}
	set_default_color( 0 );
}


/*
 *	function: 	rprint_metric
 *
 *	args:		pointer to metric in mettbl to display numeric data
 *			pointer to an alternate metric to plot
 *			row and col at which to do the display
 *			number of digits to right of decimal pt to display
 *
 *	ret val:	none
 *
 *	print an individual metric, which may be composed of multiple 
 */
void
rprint_metric( struct metric *metp, struct metric *metp2, int row, int col, int digits ) {
	int i, j, k;
	int dimx = metp->resval[0]+1;
	int dimy = metp->resval[1]+1;
	char buf[13];
	static char *fmts[] = {	"%8.0f", "%8.1f", "%8.2f", "%8.3f" };
#define NFMTS (sizeof(fmts)/sizeof(char *))
	static char *zeros[NFMTS] = { "        ","        ",
	  "        ","        " };
	static int zflg = 1;

	assert( digits >=0 && digits <= NFMTS );

	if( zflg ) {
		for( i = 0 ; i < NFMTS; i++ )
			sprintf( zeros[i], fmts[i], 0.0 );
		zflg = 0;
	}

	if( need_header & METS ) {
		int i,j;
		char my_title[24];

		set_label_color( 0 );
		move( row, col );
		strncpy( my_title, metp[0].title, 23 );
		for( i = 0, j = 0 ; i < 10; i++, j++ )
			if( my_title[j] == '%' )
				j++;
		my_title[j] = '\0';
		printw(my_title);
		set_default_color( 0 );
	}
	switch( metp->ndim ) {
	case 0: /* A single global met.  Just call mk_field. */

		mk_field( metp, metp2, 0, 0, row, col+11, 8, fmts[digits], metp->metval[0].cooked, metp2->metval[0].cooked, NULL );
		break;

	case 1: /* A 1-dimensional met, such as usr time (kept per-cpu) */
/*
 *		If metscroll is set, we've already done some scrolling to
 *		the right, so we put "<-" at the right edge of the screen.
 *		This may be overwritten below if there are more mets that
 *		are invisible off the right of the screen.
 */
		if( metscroll ) {
			set_message_color( 0 );
			move( row, scr_cols-2 );
			printw(gettxt("RTPM:612", "<-"));
			scrollcol = col+20;
		}

/*
 *		Starting with metscroll, count the number of metrics
 *		we have to display.  If the number of metrics rolls off 
 *		the right-hand edge of the screen, set canscroll and 
 *		adjust the indicator on the right-hand edge.
 */
		for( k=0,i=metscroll; i < dimx-1 ; i++, k++ ) {
			if((col + 20 + k*9 + 9) >= scr_cols ) {
				canscroll = 1;
				set_message_color( 0 );
				if( !metscroll ) {
					move( row, scr_cols-2 );
					printw(gettxt("RTPM:613", "->"));
				} else {
					move( row, scr_cols-2 );
					printw(gettxt("RTPM:614", "<>"));
				}
				break;
			}
/*
 *			call mk_field for each instance we are displaying
 */
			mk_field( metp, metp2, i, 0, row, col + 20 + k*9, 8, fmts[digits], metp->metval[i].cooked, metp2->metval[i].cooked, NULL );
		}
/*
 *		call mk_field for the "total"
 */
		if( metp->action == NONE )
			mk_field( NULL, NULL, 0, 0, row, col + 11, 8, "       -", NULL, NULL, NULL );
		else if( dimx <= 1 )
			mk_field( NULL, NULL, 0, 0, row, col + 11, 8, zeros[digits], NULL, NULL, NULL );
		else
			mk_field( metp, metp2, dimx-1, 0, row, col + 11, 8, fmts[digits], metp->metval[dimx-1].cooked, metp2->metval[dimx-1].cooked, NULL );
		break;

	case 2:	/* display a 2-dim met, first resource must be per-cpu */

/*
 *		Two dimensional metrics look like this:
 *
 *		   iget/s        total   cpu 00   cpu 01   cpu 02  ...
 *		           s5     0.00     0.00     0.00     0.00  ...
 *		      sfs/ufs     0.00     0.00     0.00     0.00  ...
 *		         vxfs     0.00     0.00     0.00     0.00  ...
 *		        other     0.00     0.00     0.00     0.00  ...
 *		        total     0.00     0.00     0.00     0.00  ...
 *
 *		For the the above, the metric title is "iget/s", the 
 *		first resource is cpu, and the second resource is fstyp.
 */

/*
 *		Check to see if we need to write the labels
 */
		if( need_header & METS ) {
			set_label_color( 0 );
			assert( metp->reslist[0] == NCPU );
			switch( metp->reslist[1] ) {
			case NFSTYP:
				buf[12] = '\0';
				for(i=0;i<nfstyp;i++){
					move(row+i+1,col+3);
					strncpy( buf, fsname[i].met_p,12);
					printw("%7s",buf);
				}
				move(row+i+1,col+3);
				printw("%7s",gettxt("RTPM:50", "total"));
				break;
			case KMPOOLS:
				for(i=0;i<nkmpool;i++){
					move(row+i+1,col+3);
					printw("%4d",
					  kmasize[i].met.sngl);
				}
				move(row+i+1,col+3);
				printw(gettxt("RTPM:50", "total"));
				break;
				
			}
		}
/*
 *		increment the row, because the per-cpu labels took the 
 *		first row.
 */
		row++;
/*
 *		If metscroll is set, we've already done some scrolling to
 *		the right, so we put "<-" at the right edge of the screen.
 *		This may be overwritten below if there are more mets that
 *		are invisible off the right of the screen.
 */
		if( metscroll ) {
			set_message_color( 0 );
			for( j=0 ; j < dimy ; j++ ) {
				move( row+j, scr_cols-2 );
				printw(gettxt("RTPM:612", "<-"));
			}
			scrollcol = col+20;
		}

/*
 *		Starting with metscroll, count the number of metrics
 *		we have to display horizontally.  If the number rolls off 
 *		the right-hand edge of the screen, set canscroll and 
 *		adjust the indicator on the right-hand edge.
 */
		for( k=0,i=metscroll ; i < dimx-1 ; i++,k++ ) {
			if((col + 20 + k*9 + 9) >= scr_cols ) {
				canscroll = 1;
				set_message_color( 0 );
				if( !metscroll ) {
					for( j=0 ; j < dimy ; j++ ) {
						move( row+j, scr_cols-2 );
						printw(gettxt("RTPM:613", "->"));
					}
				} else {
					for( j=0 ; j < dimy ; j++ ) {
						move( row+j, scr_cols-2 );
						printw(gettxt("RTPM:614", "<>"));
					}
				}
				break;
			}
/*
 *			display a column of metrics
 */
			for( j=0 ; j < dimy-1 ; j++ ) {
				mk_field( metp, metp2, i, j, row+j, col + 20 + k*9, 8, fmts[digits], metp->metval[i*dimy+j].cooked, metp2->metval[i*dimy+j].cooked, NULL );
			}
/*
 *			display the total at the bottom of the column
 */
			mk_field( metp, metp2, i, j, row+j, col + 20 + k*9, 8, fmts[digits], metp->metval[i*dimy+j].cooked, metp2->metval[i*dimy+j].cooked, NULL );
		}
/*
 *		Do the first column, which are the totals across the rows
 */
		for( j=0 ; j < dimy ; j++ ) {
			mk_field( metp, metp2, dimx-1, j, row+j, col + 11, 8, fmts[digits], metp->metval[(dimx-1)*dimy+j].cooked, metp2->metval[(dimx-1)*dimy+j].cooked, NULL );
		}
		
		break;
	}
	set_default_color( 0 );
}


/*
 *	function: 	justify
 *
 *	args:		pointers to a row offset and a columns offset
 *
 *	ret val:	none
 *
 *	justify centers the metrics portion of the display in the
 *	center of the lower 14 rows of the screen.  Some thought
 *	was given to allowing the plot area to float into the
 *	metrics portion, if not all 14 rows were needed.  This 
 *	was decided against, because as you change to different
 *	subscreens, the plot scale keeps changing to try to use
 *	the available space, which is annoying to the user.
 */
void
justify( int *roff, int *coff ) {

/* 
 *	Set the size of the metric area needed in rows and columns
 *	This is done at run time because some of the displays are based
 *	on resource values, like ncpu.
 */
	if( screenfunc[screendepth] == print_sum ) { 
		metrows = 14;
		metcols = 80;
	} else if( screenfunc[screendepth] == print_help ) {
		metrows = 14;
		metcols = 80;
	} else if( screenfunc[screendepth] == print_pgin ) {
		metrows = 11;
		metcols = 10 + 9*(ncpu+1);
	} else if( screenfunc[screendepth] == print_pgout ) {
		metrows = 11;
		metcols = 10 + 9*(ncpu+1);
	} else if( screenfunc[screendepth] == print_fscalls ) {
		metrows = 9;
		metcols = 10 + 9*(ncpu+1);
	} else if( screenfunc[screendepth] == print_bufcache ) {
		metrows = 9;
		metcols = 10 + 9*(ncpu+1);
	} else if( screenfunc[screendepth] == print_fstbls ) {
		metrows = 14;
		metcols = 10 + 9*(nfstyp+1);
	} else if( screenfunc[screendepth] == print_igets_dirblks ){
		metrows = 13;
		metcols = 10 + 9*(ncpu+1);
	} else if( screenfunc[screendepth] == print_inoreclaim ) {
		metrows = 13;
		metcols = 10 + 9*(ncpu+1);
	} else if( screenfunc[screendepth] == print_ethtraffic ) {
		metrows = 8;
		metcols = 10 + 9*(nether+1);
	} else if( screenfunc[screendepth] == print_ethinerr ) {
		metrows = 10;
		metcols = 10 + 9*(nether+1);
	} else if( screenfunc[screendepth] == print_ethouterr ) {
		metrows = 6;
		metcols = 10 + 9*(nether+1);
	} else if( screenfunc[screendepth] == print_icmp ) {
		metrows = 14;
		metcols = 72;
	} else if( screenfunc[screendepth] == print_tcp ) {
		metrows = 14;
		metcols = 80;
	} else if( screenfunc[screendepth] == print_ip ) {
		metrows = 11;
		metcols = 64;
	} else if( screenfunc[screendepth] == print_cputime ) {
		metrows = 7;
		metcols = 10 + 9*(ncpu+1);
	} else if( screenfunc[screendepth] == print_syscalls ) {
		metrows = 11;
		metcols = 10 + 9*(ncpu+1);
	} else if( screenfunc[screendepth] == print_disk ) {
		metrows = 8;
		metcols = 10 + 9*(ndisk+1);
	} else if( screenfunc[screendepth] == print_queue ) {
		metrows = 9;
		metcols = 10 + 9*(ncpu+1);
	} else if( screenfunc[screendepth] == print_tty ) {
		metrows = 14;
		metcols = max( 10 + 9*(ncpu+1), 65 );/* includes streams */
	} else if( screenfunc[screendepth] == print_eth ) {
		metrows = 10;
		metcols = 80;
	} else if( screenfunc[screendepth] == print_mem ) {
		metrows = 14;
		metcols = 80;
	} else if( screenfunc[screendepth] == print_paging ) {
		metrows = 11;
		metcols = 80;
	} else if( screenfunc[screendepth] == print_fs ) {
		metrows = 13;
		metcols = 80;
	} else if( screenfunc[screendepth] == print_lwp ) {
		metrows = scr_rows;	/* variable size display */
		metcols = scr_cols;
	} else if( screenfunc[screendepth] == print_net ) {
		metrows = 12;
		metcols = 80;
	} else if( screenfunc[screendepth] == print_netware ) {
		metrows = 14;
		metcols = 80;
	} else if( screenfunc[screendepth] == print_rip ) {
		metrows = 13;
		metcols = 75;
	} else if( screenfunc[screendepth] == print_spx ) {
		metrows = 11;
		metcols = 76;
	} else if( screenfunc[screendepth] == print_spx_snd ) {
		metrows = 10;
		metcols = 10 + 9*(maxspxconn+1);
	} else if( screenfunc[screendepth] == print_spx_rcv ) {
		metrows = 11;
		metcols = 10 + 9*(maxspxconn+1);
	} else if( screenfunc[screendepth] == print_spx_msc ) {
		metrows = 13;
		metcols = 10 + 9*(maxspxconn+1);
	} else if( screenfunc[screendepth] == print_sap ) {
		metrows = 14;
		metcols = 78;
	} else if( screenfunc[screendepth] == print_sap_lan ) {
		metrows = 9;
		metcols = 10 + 9*(nlans+1);
	} else if( screenfunc[screendepth] == print_ipx_lan ) {
		metrows = 14;
		metcols = 78;
	} else if( screenfunc[screendepth] == print_ipx_sock ) {
		metrows = 14;
		metcols = 78;
	} else if( screenfunc[screendepth] == print_ipx ) {
		metrows = 14;
		metcols = 78;
	} else {
#ifdef DEBUG
		endwin();
		fprintf(stderr,"DEBUG unlisted function in justify\n");
		exit(1);
#endif
		metrows = 14;
		metcols = 80;
	}
/*
 *	We could float the plot size based on the amount of metric
 *	data to display on the bottom of the screen, but then
 *	we could get into a problem if we call up a lot of plots
 *	when there is room and then try to switch back to a screen
 *	without as much space.  Instead, assume the bottom 14 lines
 *	of the screen are used by the metrics, and if we are using 
 *	less, let_print mets center the metrics vertically within the
 *	14 rows.
 */
	disprows = scr_rows - 24 + 7;	/* num of rows for plot display */
	maxvplots = disprows / 7;	/* max vertical plots		*/
	maxhplots = scr_cols/40;	/* max horizontal plots		*/
	maxplots = maxvplots * maxhplots;	/* max total plots	*/
	if( metrows <= 14 )		/* center mets vertically	*/
		*roff = (14-metrows)/2;
	else
		*roff = 0;
	if( metcols <= scr_cols )	/* center mets horizontally	*/
		*coff = (scr_cols - metcols)/2;
	else
		*coff = 0;
}

/*
 *	function: 	print_mets
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	Print_mets does pretty much what you'd expect.  It's
 *	called every time a screen or plot changes.
 */
void
print_mets() {
	int i, j, k, l, n, pcol, pltcnt;
	static int metrow_offset, metcol_offset;
	struct plotdata *p;
	int maxdigits = 0;

	if( need_header & METS ) {
/*
 *		Get the row and col offsets to center the metrics.
 *		Justify also sets the maximum number of plots based 
 *		on screen size.  The information justify calculates
 *		should probably be computed just once at startup and 
 *		saved, instead of on every call to print_mets, but 
 *		it's not very expensive to do it this way.
 */
		justify( &metrow_offset, &metcol_offset );
	}

	if( need_header & ( PLOTS | PLOT_UPDATE ) ) {
/*
 *		Find the maximum number of digits in use by any plot 
 *		on the screen.  This may have changed since the last 
 *		time we looked if something had to re-scale to accommodate 
 *		a large value.
 */
		for( pltcnt = 0, p = plotdata; p; p = p->nxt, pltcnt++ ) {
			if( !p->barg_flg && p->rows >= 0 ) {
				maxdigits = max( maxdigits, set_format( 
				  (double)p->yscale, (double)p->yincr, NULL ));
			}
		}
/*
 *		Run through the list of everything on the screen and make 
 *		sure the number of digits is set to the max found above. 
 *		This will push all of the x scales out the same distance 
 *		from the y-axis, so that samples on stacked plots will 
 *		align vertically.  Otherwise they'd be skewed to the left 
 *		and right, making them harder to read.
 */
		for( p = plotdata; p; p = p->nxt ) {
			if( p->digits != maxdigits ) {
				need_header |= PLOTS;
				p->digits = maxdigits;
			}
		}
	}
#ifdef DEFAULT_BAR
/*
 *	This is where the default bar graph is generated.
 */
	if( !plotdata ) {
		pltcnt = 1;
		setbar();
		need_header |= PLOTS;
	}
#endif
/*
 *	If the plot area has changed, clear it out
 */
	if( need_header & PLOTS ) {
		for( i = 0; i < disprows; i++ )
			color_clrtoeol( i, 0 );
	}

/*
 *	If the mets area has changed, clear it out
 */
	if( need_header & METS ) {
		for( i = disprows+1; i < scr_rows-2; i++ )
			color_clrtoeol( i, 0 );
	}
	if( need_header & ( PLOTS | PLOT_UPDATE ) ) {
/*
 *		Figure out where all of the plots are going to go,
 *		and display them.  If there are no sided by sided plots,
 *		the plots will do a little auto-justification vertically.
 *		They are given a maximum amount of vertical space, but if 
 *		plot decides it doesn't need it all for a reasonable 
 *		y-scale, it will give some back.  Getting multiple columns
 *		of plots to do this in a reasonable fashion so that the 
 *		x-axes line up on the same row is difficult, rather than 
 *		attack that here, we go to a fixed vertical spacing for 
 *		all of the plots as soon as we hit multi-column plotting.
 */
		p = plotdata;	
		j = 0;		/* number of vertical plot slots used */
		k = pltcnt;	/* number of plots left to do */
		i=0;		/* current row */
		while( p ) {
/*
 *			figure out how many columns we need on 
 *			this row of plots
 */
			for( pcol = 1; k > pcol*(maxvplots-j); pcol++ ) {
				assert( pcol <= maxhplots );
				;
			}
			if( pcol > 1 ) {
/*
 *				multi-col mode, use fixed vert spacing.
 *
 *				for l in number of plots on this row, 
 *				  do plot
 */
				for( l = 0; l < pcol; l++ ) {
	 				n = plot( i, l*scr_cols/pcol + l, 
					  disprows/maxvplots-3, 
					  (scr_cols/pcol)-l-1,p);
					p = p->nxt;
					k--;			
				}
				i += disprows/maxvplots;
			} else  {
/*
 *				single col mode, use variable vert spacing
 */
				n = plot( i, 0, 
				  (disprows-i)/k - 2, scr_cols-1, p);
				p = p->nxt;
				k--;			
				i = n+1;
			}
			j++;
		}
	}
/*
 *	Call function associated with current screen.
 */
	(*screenfunc[screendepth])(disprows + 1 + metrow_offset, metcol_offset );
/*
 *	figure out where things go on the last line
 */
	if( !plock_len ) 
		horz_justify();
/*
 *	print the system name at the bottom of the screen
 */
 	(void) print_uname();
/*
 *	print the plock message at the bottom of the screen
 */
 	(void) print_plock();
/*
 *	print the current time at the bottom of the screen
 */
 	(void) print_time();
/*
 *	print either the help teaser or the interval at 
 *	the bottom of the screen
 */
	if( msg_disp & HELP_MSG_BIT )
		print_help_msg();
	else
		print_interval();
/*
 *	make sure we have a current field and that it's highlighted
 *	pick the first field that is displayed below disprows on the 
 *	screen.  (first field whose row > disprows)
 */
	if( !currfield[screendepth] ) 
		for( currfield[screendepth] = froot[screendepth];
			currfield[screendepth]->row <= disprows ;
			currfield[screendepth]=currfield[screendepth]->nxt);
	highlight_currfield();
/*
 *	clear need_header flag
 */
	need_header = 0;
}


/*
 *	function: 	highlight_currfield
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	display the current field in reverse video
 */
void
highlight_currfield() {
	move( currfield[screendepth]->row, currfield[screendepth]->col );
	if( currfield[screendepth]->met ) {
		set_metric_color( (double) currfield[screendepth]->met[curcook], currfield[screendepth]->mp->color, A_STANDOUT, 0 );
		printw( format_met( currfield[screendepth]->fmt, (double)currfield[screendepth]->met[curcook]) );
	} else if( currfield[screendepth]->func ) {
		set_header_color( A_STANDOUT );
		printw( currfield[screendepth]->fmt );
	} else {
		set_default_color( A_STANDOUT );
		printw( currfield[screendepth]->fmt );
	}
	attroff( A_STANDOUT );
	set_default_color( 0 );
}


/*
 *	function: 	lolight_currfield
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	display the current field in normal (not reverse) video,
 *	probably because we are moving to a new current field.
 */
void
lolight_currfield() {
	move( currfield[screendepth]->row, currfield[screendepth]->col );
	attroff( A_STANDOUT );
	if( currfield[screendepth]->met ) {
		set_metric_color( (double) currfield[screendepth]->met[curcook], currfield[screendepth]->mp->color, 0, 0 );
		printw( format_met( currfield[screendepth]->fmt, (double) currfield[screendepth]->met[curcook]) );
	} else if( currfield[screendepth]->func ) {
		set_header_color( 0 );
		printw( currfield[screendepth]->fmt );
	} else {
		set_default_color( 0 );
		printw( currfield[screendepth]->fmt );
	}
	set_default_color( 0 );
}


/*
 *	function: 	plot
 *
 *	args:		zrow, zcol: upper left hand corner of plot area
 *			plotrows, plotcols: max rows and cols available
 *				for this plot
 *			pd: pointer to plotdata structure
 *
 *	ret val:	first row after completed plot
 *
 *	plot plots a metric associated with plotdata struct pd at 
 *	zrow,zcol within plotrows and plotcols amount of screen space.
 */
int 
plot(int zrow, int zcol, int plotrows, int plotcols, struct plotdata *pd) {
	int i;
	int depth;
	struct field *fp, *tp;

	if( !pd )
		return(zrow);		/* nothing to plot */
	if( pd->barg_flg ) {		/* do a bargraph   */
		return( bar( zrow, zcol, plotrows, plotcols, pd ) );
	}
	if( (need_header & PLOTS) || pd->met[curcook] > pd->maxx) {
		char str[8];
		int flag[MAXSCREENDEPTH];
		int len;
/*
 *		Either need_header is set, and we have to redraw 
 *		everything, or the current value of metric is greater than
 *		the previous max, and we have to re-scale the y-axis.
 *		In any case, we have to do the labels, scale the y-axis,
 *		and display whatever history data is available.
 */
		if( !(need_header & PLOTS) ) { /* re-sizing */
			set_default_color( 0 );
			for( i = 0; i <= pd->rows + pd->offset + 1; i++ ) {
				
/*
 *				clear this plot.
 *
 *				using clrtoeol relies on the order in 
 *				which things are put on the screen:
 *					1  2  3
 *					4  5  6
 *					7  8  9 ...
 */
				color_clrtoeol( zrow+i, zcol );
			}
/*
 *			and clear every plot that comes after it.
 */

			for( ; zrow+i < disprows; i++ ) {
				color_clrtoeol( zrow+i, 0 );
			}
/*
 *			inform everyone else they have to re-draw
 */
			need_header |= PLOTS;
		}
/*
 *		clear all the fields (at all screendepths)
 *		associated with this plot title.
 */
		for( depth = 0; depth <= screendepth; depth++ ) {
			flag[depth] = 0;
			for( fp = (struct field*)&froot[depth];
			  fp->nxt;  ) {
				if( fp->nxt->fmt == pd->title ) {
					tp = fp->nxt;
					fp->nxt = fp->nxt->nxt;
					if( currfield[ depth ] == tp )
						flag[depth] = 1;
					free( tp );
				} else {
					fp = fp->nxt;
				}
			}
		}
 /*
  *		construct the title
  */
		make_plot_title( pd );

		len = strlen( pd->title );
/*
 *		put plot title on all pushed screens, restore 
 *		any currfields we destroyed.
 */
		depth = screendepth;
		for( screendepth=0; screendepth <= depth; screendepth++ ) {
			fp = mk_field( NULL, NULL, 0, 0, zrow, zcol, len, 
			  pd->title, NULL, NULL, nuke_plot );
			if( flag[screendepth] )
				currfield[screendepth]=fp;
		}
		screendepth = depth;

/*
 *		lastpos is used for calculating how to place the 
 *		connecting bars in a CONNECT_PLOT.  It tracks the last 
 *		position that was plotted.  However, we want to 
 *		distinguish between a value where the metric was 0,
 *		meaning no point was plotted, and the value where a
 *		metric was small and a point was plotted on the x-axis.
 *		To do this, lastpos is set to -1 when there was no point
 *		plotted.  Hence, -2 is used as a starting value.
 */
		pd->lastpos = -2;
/*
 *		set the number of columns to either the limit of available
 *		screen space or the limit of how much history data we have
 *		available for display.
 */
		pd->cols = min( plotcols, maxhist );
/*
 *		run through the history data looking for the maximum value.
 */
		for( pd->maxx = 0.0, i = 0 ; i < maxhist; i++ )
			pd->maxx = max( pd->met[i], pd->maxx );
/*
 *		pick a scale for the y-axis
 */
		pd->yscale = (float)setscale( (double)pd->maxx, &pd->rows, plotrows );
/*
 *		if were are doing multi-column plots, fix the size of
 *		the vertical plot scale, otherwise set it to 0 to let 
 *		it float.
 */
		if( plotcols < min( scr_cols-1, maxhist ) ) /* multi column */
			pd->offset = max( plotrows - pd->rows, 0 );
		else 
			pd->offset = 0;
/*
 *		set the y increment and get a format string for the
 *		numeric labels on the y-axis.  Then print the labels
 *		and save the number of digits so that we can line up
 *		multiple plots vertically.  (eg. with two plots, one 
 *		above the other, we want to make both the samples for
 *		the same intreval line up in the same column, regardless
 *		of what the scales on y-axis are for the two plots).
 */
		pd->yincr = pd->yscale/pd->rows;
		pd->digits = max( pd->digits, set_format( (double)pd->yscale, (double)pd->yincr, str ));
		set_label_color( 0 );
		for( i = 0 ; i <= pd->rows ; i++ ) {
			move( zrow + i + 1 + pd->offset, zcol );
			if( !pd->mp->inverted )
				printw(str,(pd->rows-i)*pd->yscale/pd->rows);
			else
				printw(str,i*pd->yscale/pd->rows);
		}
		pd->curx = pd->digits;
/*
 *		pick a point in the history buffer at which to start
 */
		i = curcook - pd->cols + pd->digits + 1;
		if( i < 0 ) {
			if( rollover ) 
				i += maxhist;
			else 
				i = 0;
		}
/*
 *		run through the history buffer, plotting points
 */
		/* LINTED statement has null effect */
		for( ; i != curcook ; ( (++i < maxhist) ? 0 : (i=0) ) ) {
			pd->curx++;
			plot_point( zrow + pd->offset, zcol, pd->rows, pd->curx,
			  (double)pd->met[i], (double)pd->yincr, &pd->lastpos, pd->mp, pd->mp->inverted );
		}
/*
 *		we are done re-drawing the existing data
 */
	}
/*
 *	Now it's time to post the current value to the right hand end 
 *	of the plot.
 */
	if( pd->curx == pd->cols - 1 ) {
/*
 *		we are at the end of the plot, scroll to left.
 *		remove the first column to the right of the y-axis.
 */
		for( i = 0 ; i <= pd->rows; i++ ) {
			move(zrow + i + 1 + pd->offset, zcol + pd->digits + 1 );
			delch();
		}
/*
 *		If we're doing multi-column plots, we have to push
 *		things to the right of us back once columns, since we 
 *		just deleted a column.
 */
		set_plot_color( 0 );
		for( i = 0 ; i <= pd->rows; i++ ) {
			move(zrow + i + 1 + pd->offset, zcol + pd->curx );
			insch(' ');
		}
	} else {
/*
 *		we are not at the edge of the plot, just increment the 
 *		x-position
 */
		pd->curx++;
	}
/*
 *	plot a point at the right-hand end of the plot.
 */
	plot_point( zrow + pd->offset, zcol, pd->rows, pd->curx, (double)pd->met[curcook],
	  (double)pd->yincr, &pd->lastpos, pd->mp, pd->mp->inverted );
	set_default_color( 0 );
/*
 *	return the number of rows consumed by this plot.
 */
	return(zrow + pd->rows + pd->offset + 2);
}


/*
 *	function: 	make_plot_title
 *
 *	args:		pointer to struct plotdata
 *
 *	ret val:	none
 *
 *	make_plot_title prints the name of the metric at the top of
 *	a plot and then calls plot_restitle to print labels associated 
 *	with the resource values for this instance of the metric.
 */
void
make_plot_title( struct plotdata *pd ) {
	struct metric *mp = pd->mp;
	int r1 = pd->r1;
	int r2 = pd->r2;
	char *title = pd->title;
	char *p;

	strcpy( title, mp->title );
	p = title + strlen( title );

	if( mp->reslist[1] != KMPOOLS || mp->reslist[0] != NCPU ) {
		plot_restitle( mp->reslist[0], r1, p );
		p = title + strlen( title );
	}
	plot_restitle( mp->reslist[1], r2, p );
}


/*
 *	function: 	plot_restitle
 *
 *	args:		resource id, resource value, and char pointer
 *			to where string should be returned,
 *
 *	ret val:	none
 *
 *	plot_restitle generates labels associated with resource values.
 *	It is used to display titles of metrics at the top of plots.
 */
void
plot_restitle( int res_id, int res_val, char *buf ) {
	switch( res_id ) {
	default: break;
	case NCPU:
		*buf++ = ' ';
		if( res_val >= ncpu )
			sprintf(buf, gettxt("RTPM:50", "total"));
		else
			sprintf(buf, gettxt("RTPM:53", "cpu:%d"),res_val);
		break;
	case NETHER:
		*buf++ = ' ';
		if( res_val >= nether )
			sprintf(buf, gettxt("RTPM:50", "total"));
		else
			sprintf(buf, gettxt("RTPM:54", "dev:%s"), ethname[res_val].met_p );
		break;
	case SAP_Lans:
		*buf++ = ' ';
		if( res_val >= nlans )
			sprintf(buf, gettxt("RTPM:50", "total"));
		else
			sprintf(buf, gettxt( "RTPM:980","lan:%02d"),res_val );
		break;
	case SPX_max_connections:
		*buf++ = ' ';
		if( res_val >= maxspxconn )
			sprintf(buf, gettxt("RTPM:50", "total"));
		else
			sprintf(buf, gettxt( "RTPM:981","spx:%02d"),res_val );
		break;
	case NFSTYP:
		*buf++ = ' ';
		if( res_val >= nfstyp )
			sprintf(buf, gettxt("RTPM:50", "total"));
		else {
			char lbuf[13];
			lbuf[12] = '\0';
			strncpy( lbuf, fsname[res_val].met_p,12);
			sprintf(buf, gettxt("RTPM:55", "filesys:%s"),lbuf);
		}
		break;
	case NDISK:
		*buf++ = ' ';
		if( res_val >= ndisk )
			sprintf(buf, gettxt("RTPM:50", "total"));
		else {
			char lbuf[13];
			lbuf[12] = '\0';
			strncpy( lbuf, dsname[res_val].met_p,12);
			sprintf(buf, gettxt("RTPM:56", "disk:%s"),lbuf);
		}
		break;
	case KMPOOLS:
		*buf++ = ' ';
		if( res_val >= nkmpool )
			sprintf(buf, gettxt("RTPM:50", "total"));
		else if( res_val == nkmpool-1 )
			sprintf(buf, gettxt("RTPM:57", "pool:ovsz"));
		else
			sprintf(buf, gettxt("RTPM:58", "pool:%d"), kmasize[res_val].met.sngl);
		break;
	}
}

/*
 *	function: 	make_bar_title
 *
 *	args:		pointer to struct plotdata
 *			column offset of title
 *
 *	ret val:	none
 *
 *	make_bar_title prepares a string of the form "%s= %u-" to
 *	use a title field for the bargraph.
 */
void
make_bar_title( struct plotdata *pd, int col ) {
	char *title = pd->title;
	char *q;
	int len;
	char *mytitle;

	mytitle = gettxt("RTPM:59", "%%s= %%u-");

	q = title;

	for( len = 0 ; len < col ; len++ )
		*q++ = ' ';				

	strcpy(q, mytitle);
}


/*
 *	function: 	setscale
 *
 *	args:		maxx: max metric value
 *			rows: pointer to number of required for the scale
 *			  that set scale selects
 *			plotrows: the maximum number of rows available
 *
 *	ret val:	total y-scale selected
 *
 *	setscale attempts to pick a reasonable and "round" scale for the
 *	y-axis of a plot.  It sets the number of rows required for the
 *	scale in *rows, and returns the total y-axis scale.
 */
double
setscale( double maxx, int *rows, int plotrows ){

/*
 *	factor is a list of "reasonable" and "round" scales.  This
 *	is to avoid having y-axis increments that read like irrational
 *	numbers.
 */
	static float factor[] = {
		 2.0,  3.0,  4.0,  5.0,  6.0,  8.0, 10.0, 12.0, 15.0, 
		20.0, 25.0, 30.0, 40.0, 50.0, 60.0, 75.0, 80.0, 100.0 };
#define NFACT	( sizeof( factor ) / sizeof ( float ) )

	float scale = 1.0;	/* initial exponent */
	int i, j, lrows, ifact;

/*	if the metric is very small (it's probably 0), set scale to 1.0 */
	if( maxx < 0.000001 )
		maxx = 1.0;
/*
 *	if metric <= 1.0, bump it up into range of factor above
 *	and decrease exponent.
 */
	while( maxx <= 1.0 ) {
		maxx *= 10.0;
		scale /= 10.0;
	}

/*
 *	if metric > 100.0, bring it down into range of factor above
 *	and increase exponent.
 */
	while( maxx > 100.0 ) {
		scale *= 10.0;
		maxx /= 10.0;
	}

/*
 *	pick a reasonable range for the y-axis
 */
	for( i = 0 ; i < NFACT ; i++ )
		if( maxx <= factor[i] )
			break;

	assert( i < NFACT );

/*
 *	see if we can expand the number of rows for this scale and still
 *	stay within plotrows.
 */
	ifact = (int)factor[i];
	for( lrows = ifact; (lrows + ifact) <= plotrows; lrows += ifact )
		;
/*
 *	see if we need to compress the number of rows to stay within
 *	plotrows.
 */
	for( j = 1; (lrows/j) > plotrows; j++ )
		;
/*
 *	set the number of rows and return the selected scale
 */
	*rows = lrows/j;

	return( scale * factor[i] );	
}

/*
 *	function: 	set_format
 *
 *	args:		the y-scale and y-increment
 *			a string in which to return the format string
 *
 *	ret val:	the number of characters need to print 
 *			  a y-axis value.
 *
 *	set_format constructs a string of the form "%<d>.<d>f" for printing
 *	values on the y-axis of a plot, where <d> is a digit.  It returns
 *	the number of characters printed by the format string.
 */
int 
set_format( double yscale, double yincr, char *str ) {
	int i, bigdigits, lildigits;

	assert( yincr > 0.000001 );
	assert( yscale > 0.000001 );

/*
 *	figure out how many digits we need for yscales greater than 1.
 */
	bigdigits = 0;
	for( i = (int)yscale; i ; i /= 10 )
		bigdigits++;
/*
 *	figure out how many digits we need for yscales and yincrs
 *	less than 1:
 */
	lildigits = 0;
	while( yscale < 1.0 || yincr < 1.0 ) {
		yscale *= 10.0;
		yincr *= 10.0;
		lildigits++;
		bigdigits++;
	}
/*
 *	if we set lildigits, add one to bigdigits for the decimal point
 */
	if( lildigits ) bigdigits++;
	if( str )
		sprintf(str,"%%%d.%df ",bigdigits,lildigits);
	return( bigdigits );
}


/*
 *	function: 	plot_point
 *
 *	args:		zrow, zcol: row and col of origin
 *			rows: number of rows for plotting 
 *				(zrow+rows) is row at which x-axis resides
 *			curx: current x position (this is a column)
 *			*met: pointer to the value to plot
 *			yincr: y increment
 *			*lastpos: pointer to last position plotted,
 *			  will be overwritten with new position
 *			mp: pointer to metric struct for getting color
 *
 *	ret val:	none
 *
 *	plot_point plots a single metric value (*met) at zrow+rows and 
 *	zcol+curx according to the parameters yincr and (if CONNECT_PLOT)
 *	*lastpos.
 */
void
plot_point( int zrow, int zcol, int rows, int curx, 
  double met, double yincr, int *lastpos, struct metric *mp, int inverted){
	int pos;
	int j;
	double xval;
	int nopoint;

	set_plot_color( 0 );
/* 
 *	figure out where the point lands
 */
	pos = (met+yincr*0.5)/yincr;
	xval = 0.25 * yincr;
	if( inverted ) {
		pos = rows - pos;
		xval = ((double)rows - 0.25) * yincr;
	}
	nopoint = (!inverted && met < xval ) || (inverted && met > xval);
	
	switch( ptype ) {
	case SCATTER:		/* simple plot with '*' at the points */
/*
 *		draw the x-axis
 */
		move(zrow + rows + 1, zcol + curx );	/* x-axis */

		if ( pos == 0 && nopoint ) {
			set_metric_color( (double) met, mp->color, A_UNDERLINE, 1 );
			printw(gettxt("RTPM:615", "*"));
		} else {
			set_metric_color( (double) xval, mp->color, A_UNDERLINE, 1 );
			if( does_underline )
				printw(" ");
			else
				printw(gettxt("RTPM:659", "_"));
		}
		attroff( A_UNDERLINE );
/*
 *		place the point
 */
		if( pos > 0 ) {
			move( zrow+rows-pos+1, zcol+curx );
			set_metric_color( (double) met, mp->color, 0, 1 );
			printw(gettxt("RTPM:615", "*"));
		}
		break;
	case FULLBARS:	/* vertical bargraph from x-axis to point */
/*
 *		draw the x-axis
 */
		move(zrow + rows + 1, zcol + curx );	/* x-axis */

		if( nopoint ) {
			set_metric_color( (double) xval, mp->color, A_UNDERLINE, 1 );
			if( does_underline )
				printw(" ");
			else
				printw(gettxt("RTPM:659", "_"));
		}
		else {
			set_metric_color( xval, mp->color, A_UNDERLINE, 1 );
			printw(gettxt("RTPM:616", "|"));
		}
		attroff( A_UNDERLINE );
/*
 *		draw vertical bars from the x-axis to the point
 */
		for(j=zrow+rows; j > (zrow+rows-pos+1); j-- ) {
			move( j, zcol+curx );
			if( !inverted )
				set_metric_color( (double) yincr*(zrow+rows+1-j), mp->color, 0, 1 );
			else
				set_metric_color( (double) yincr*(zrow+j), mp->color, 0, 1 );
			printw(gettxt("RTPM:616", "|"));
		}
		if( j == (zrow+rows-pos+1 )) {
			move( j, zcol+curx );
			set_metric_color( (double) met, mp->color, 0, 1 );
			printw(gettxt("RTPM:616", "|"));
		}
		break;
	case FLOWERPLOT:	/* vertical bargraph with '*' at top */
/*
 *		draw the x-axis
 */
		move(zrow + rows + 1, zcol + curx );	/* x-axis */

		if( nopoint ) {
			set_metric_color( (double) xval, mp->color, A_UNDERLINE, 1 );
			if( does_underline )
				printw(" ");
			else
				printw(gettxt("RTPM:659", "_"));
		}
		else if( (!inverted && met < yincr * 0.5) || (inverted  && met > ((double)rows - 0.5)*yincr ) ) {
			set_metric_color( (double) met, mp->color, A_UNDERLINE, 1 );
			printw(gettxt("RTPM:615", "*"));
		} else {
			set_metric_color( (double) xval, mp->color, A_UNDERLINE, 1 );
			printw(gettxt("RTPM:616", "|"));
		}
		attroff( A_UNDERLINE );
/*
 *		draw the bargraph
 */
		for(j=zrow+rows; j > (zrow+rows-pos+1); j-- ) {
			move( j, zcol+curx );
			if( !inverted )
				set_metric_color( (double) yincr*(zrow+rows+1-j), mp->color, 0, 1 );
			else
				set_metric_color( (double) yincr*(zrow+j), mp->color, 0, 1 );
			printw(gettxt("RTPM:616", "|"));
		}
/*
 *		place the point
 */
		if( pos > 0 ) {
			move( zrow+rows-pos+1, zcol+curx );
			set_metric_color( (double) met, mp->color, 0, 1 );
			printw(gettxt("RTPM:615", "*"));
		}
		break;
	case CONNECTBARS:
/*
 *		Connect points more than 2 vertical spaces apart with '|'
 *		*lastpos is used for calculating how to place the 
 *		connecting bars.  It contains the last vertical position 
 *		that was plotted.  At the origin, however, we want to 
 *		distinguish between a value where the metric was 0,
 *		meaning no point was plotted, and the value where a
 *		metric was small so the point was plotted on the x-axis.
 *		To do this, lastpos is set to -1 when there was no point
 *		plotted and -2 is used as a starting value.
 *
 *
 *		draw the x-axis
 */	

		if( pos > 0 ) {
			if( *lastpos == -1 ) { /* going up from  0 */
				move(zrow + rows + 1, zcol + curx );
				if( !inverted )
					set_metric_color( (double) yincr, mp->color, A_UNDERLINE, 1 );
				else
					set_metric_color( xval, mp->color, A_UNDERLINE, 1 );

				printw(gettxt("RTPM:616", "|"));
			} else {
				move(zrow + rows + 1, zcol + curx );
				set_metric_color( (double) xval, mp->color, A_UNDERLINE, 1 );
				if( does_underline )
					printw(" ");
				else
					printw(gettxt("RTPM:659", "_"));
			}
		} else { /* pos == 0 */
			if ( nopoint ) {
				if( *lastpos > 0 ) { /* going down to 0 */
					move(zrow+rows+1, zcol+curx-1 );
					if( !inverted ) 
						set_metric_color( (double) yincr, mp->color, A_UNDERLINE, 1 );
					else
						set_metric_color( (double) (rows-1)*yincr, mp->color, A_UNDERLINE, 1 );
					printw(gettxt("RTPM:616", "|"));
					move(zrow+rows+1, zcol+curx );
					set_metric_color( (double) xval, mp->color, A_UNDERLINE, 1 );
					if( does_underline )
						printw(" ");
					else
						printw(gettxt("RTPM:659", "_"));
				} else {
					move(zrow+rows+1, zcol+curx );
					set_metric_color( (double) xval, mp->color, A_UNDERLINE, 1 );
					if( does_underline )
						printw(" ");
					else
						printw(gettxt("RTPM:659", "_"));
				}
			} else {
				move(zrow+rows+1, zcol+curx );
				if( !inverted )
					set_metric_color( (double) yincr, mp->color, A_UNDERLINE, 1 );
				else
					set_metric_color( (double)(rows-1)*yincr, mp->color, A_UNDERLINE, 1 );
					
				printw(gettxt("RTPM:615", "*"));
			}
		}
		attroff( A_UNDERLINE );
/* 
 *		place the point
 */
		if( pos > 0 ) {
			move( zrow+rows-pos+1, zcol+curx );
			set_metric_color( (double) met, mp->color, 0, 1 );
			printw(gettxt("RTPM:615", "*"));
		}
/*
 *		if *lastpos == 2, we are just starting a plot, and the
 *		connecting bars were handled above when we did the x-axis
 */
		if ( *lastpos == -2 ) {	/* left edge of screen 		*/
			*lastpos = pos;
			if( nopoint )
				*lastpos = -1;
			break;
		}
/*
 *		if last plotted value was 0, reset *lastpos to 0 to decode.
 */
		if( *lastpos == -1 )
			*lastpos = 0;
/*
 *		if the difference between *lastpos and the current position
 *		is 2 or more, we have to fill in the gap with |'s.
 */
		if( *lastpos > (pos+1) ) {
/*
 *			going down
 */
			for( j=zrow+rows-pos; j>zrow+rows- *lastpos+1;j--){
				move(j,zcol+curx-1);
				if( !inverted )
					set_metric_color( (double) yincr*(zrow+rows+1-j), mp->color, 0, 1 );
				else
					set_metric_color( (double) yincr*(zrow+j), mp->color, 0, 1 );
				printw(gettxt("RTPM:616", "|"));
			}
		} else if( pos > ((*lastpos)+1) ) {
/*
 *			going up
 */
			for( j=zrow+rows-*lastpos;j>zrow+rows-pos+1; j-- ){
				move(j,zcol+curx);
				if( !inverted )
					set_metric_color( (double) yincr*(zrow+rows+1-j), mp->color, 0, 1 );
				else
					set_metric_color( (double) yincr*(zrow+j), mp->color, 0, 1 );
				printw(gettxt("RTPM:616", "|"));
			}
		}
/*
 *		save pos in *lastpos
 */
		*lastpos = pos;
		if( nopoint ) 
			*lastpos = -1;
	}
	set_default_color( 0 );
}


/*
 *	function: 	bar
 *
 *	args:		row, col: row and col of origin
 *			plotrows, plotcols: max number of rows and cols
 *			 	available for plotting the bargraph
 *			*pd: pointer to plotdata structure
 *
 *	ret val:	last row used + 1
 *
 *	bar draws a cpu consumption bargraph at row,col within a
 *	maximum of plotrows rows and plotcols columns.
 */
int
bar( int row, int col, int plotrows, int plotcols, struct plotdata *pd ) {
	int i, j, k;
	int labelen;
	char buf[64];
	char *cfmt, *tfmt;
	int cols_left;
	int incr;
/*
 *	figure out how much room we have, this would be constant except
 *	for i18n.
 */
	cfmt = gettxt("RTPM:52", "cpu %-2d");
	tfmt = gettxt("RTPM:50", "total");
	sprintf(buf,cfmt,99);
	labelen = max(strlen(buf),strlen(tfmt)) + 3;
	cols_left = plotcols - labelen;
	incr = cols_left/10;
/*	
 *	Center the bargraph on a full screen, otherwise left justify it.
 */
	if( plotcols == (scr_cols-1) ) {
		i =  (( scr_cols - (9 + (incr*10) ) )/2);
		col += i;
		pd->plotcols -= i;
	}

	if( need_header & PLOTS ) {
		pd->plotrows = plotrows;
		pd->plotcols = plotcols;
		pd->zrow = row;
		pd->zcol = col;
		pd->offset = 0;
/*
 *		If we're doing multi-columns plots, shift the bargraph 
 *		down so that the bottom of it lines up with the other
 *		plots on the same row.
 */
		if( plotcols <= scr_cols/2 && plotrows > ncpu )
			pd->offset = plotrows - ncpu;

		if( ncpu == 1 )
			pd->offset++;
/*
 *		Starting with barscroll, run through the list of
 *		cpus and generate labels for them until we've
 *		run out of either cpus or plotrows.
 */
		set_label_color( 0 );
		if( ncpu == 1 ) 
			j = 0;
		else {
			for( j=0, i = barscroll ; i < ncpu ; i++, j++ ) {
				if( j > pd->plotrows ) {
					break;
				}
				move( row+j+pd->offset, col );
				for( k = 0; k < labelen ; k++ )
					printw(" ");
				move( row+j+pd->offset, col+1 );
				printw(cfmt,i);
			}
		}
/*
 *		If we have room, do the label for the total.
 */
		if( j <= pd->plotrows ) {
			move( row+j+pd->offset, col );
			for( k = 0; k < labelen ; k++ )
				printw(" ");
			move( row+j+pd->offset, col+1 );
			printw(tfmt);
		}
/*
 *		If barscroll is set, display the scroll up indicator.
 */
		if( barscroll ) {
			move( row+pd->offset,col );
			set_message_color( 0 );
			printw(gettxt("RTPM:620", "^"));
			move( row+1+pd->offset, col );
			printw(gettxt("RTPM:616", "|"));
		}
/*
 *		If we have more cpus than rows, display the scroll 
 *		down indicator.
 */
		if( j > pd->plotrows ) {
			set_message_color( 0 );
			move( row+pd->plotrows-1+pd->offset, col );
			printw(gettxt("RTPM:616", "|"));
			move( row+pd->plotrows+pd->offset, col );
			printw(gettxt("RTPM:621", "v"));
		}
	}
	else{
/*
 *		We're not doing the header, so we've already figured out 
 *		where on the screen to place the bargraph.  Retrieve that 
 *		information from the plotdata struct.
 */
		row = pd->zrow;
		col = pd->zcol;
	}

/*
 *	Starting with barscroll, run through the list of
 *	cpus and generate bargraphs for them until we've
 *	run out of either cpus or plotrows.
 */
	if( ncpu == 1 )
		j = 0;
	else {
		for( j=0, i = barscroll ; i < ncpu ; i++, j++ ) {
			if( j > pd->plotrows )
				break;
			barline( mettbl[SYS_TIME_IDX].metval[i].cooked,
			  mettbl[USR_TIME_IDX].metval[i].cooked,
			  (100.0/((double)incr*10.0)), pd->offset+row++, col+8 );
		}
	}
/*
 *	If we have room, do the barline for the total.
 */
	if( j <= pd->plotrows ) {
		barline( mettbl[SYS_TIME_IDX].metval[ncpu].cooked,
		  mettbl[USR_TIME_IDX].metval[ncpu].cooked,

		  (100.0/((double)incr*10.0)), pd->offset+row++, col+8 );
	}
/*
 *	If we need the header information, do the bottom line of the
 *	bargraph, which displays the key and the scale.
 */
	if( need_header & PLOTS ) {
		int flag[MAXSCREENDEPTH];
		int len;
		int depth;
		struct field *fp, *tp;
/*
 *		clear all the fields (at all screendepths)
 *		associated with the bargraph title.
 */
		for( depth = 0; depth <= screendepth; depth++ ) {
			flag[depth] = 0;
			for( fp = (struct field*)&froot[depth]; fp->nxt; ){
				if( fp->nxt->fmt == pd->title ) {
					tp = fp->nxt;
					fp->nxt = fp->nxt->nxt;
					if( currfield[ depth ] == tp ) {
						flag[depth] = 1;
					}
					free( tp );
				} else {
					fp = fp->nxt;
				}
			}
		}
 /*
  *		construct the title
  */
		make_bar_title( pd, col );

		len = strlen( pd->title );
/*
 *		put plot title on all pushed screens, restore 
 *		any currfields we destroyed.
 */
		depth = screendepth;
		for( screendepth=0; screendepth <= depth; screendepth++ ) {
			fp = mk_field( NULL, NULL, 0, 0, row+pd->offset, 0, len, 
			  pd->title, NULL, NULL, nuke_plot );
			if( flag[screendepth] ) {
				currfield[screendepth]=fp;
			}
		}
		set_label_color( 0 );
		screendepth = depth;
	      	move( row+pd->offset, col+len );
		printw(" ");
	      	move( row+pd->offset, col+8 );
		set_label_color( A_UNDERLINE );
		for( i = 10 ; i < 100 ; i += 10 ) {
			printw(gettxt("RTPM:616", "|"));
			if( incr > 2 ) {
				for( j = 0; j < (incr - 3); j++ )
					printw(" ");
				printw("%2d",i);
			} else {
				for( j = 0; j < (incr - 1); j++ )
					printw(" ");
			}
		}
		printw(gettxt("RTPM:616", "|"));
		if( incr > 3 ) {
			for( j = 0; j < (incr - 4); j++ )
				printw(" ");
			printw(gettxt("RTPM:617", "%3d|"),100);
		} else {
			for( j=0; j < (incr-1); j++ )
				printw(" ");
			printw(gettxt("RTPM:616", "|"));
		}
		attroff( A_UNDERLINE );
		pd->rows = row - pd->zrow + 1;
	}
	return(row+pd->offset+1);
}


/*
 *	function: 	barline
 *
 *	args:		sys, usr: system and user time percentages
 *			xincr: amount of time / column
 *			row,col: starting position
 *
 *	ret val:	row + 1
 *
 *	barline draws line of a bargraph at row and col.
 */
void
barline( float *sys, float *usr, double xincr, int row, int col )
{
	int i;
#ifdef PEAK_HOLD
	int j;
	float peak;
	int ppos;
#endif
	int spos = (sys[curcook]+xincr*0.5)/xincr;		/* sys 	*/
	int pos = (sys[curcook]+usr[curcook]+xincr*0.5)/xincr;	/* tot 	*/
	int end = (100.0+xincr*0.5)/xincr;			/* end	*/

#ifdef PARANOID
	if( spos > end ) spos = end;
	if( pos > end ) pos = end;
#endif
	move( row, col );
	for( i = 0 ; i < spos; i++ ) {
		set_metric_color( (double)i*xincr, bargraph_color, 0, 1 );
		printw(gettxt("RTPM:618", "="));
	}
	for( ; i < pos-1 ; i++ ) {
		set_metric_color( (double)i*xincr, bargraph_color, 0, 1 );
		printw(gettxt("RTPM:619", "-"));
	}
	i++;
	set_metric_color( (double)usr[curcook]+sys[curcook], bargraph_color, 0, 1 );
	printw(gettxt("RTPM:619", "-"));
	set_default_color( 0 );
	for( ; i <= end ; i++ )	/* did not use clrtoeol here because 	*/
		printw(" "); 	/* we may be doing a multi-col plot	*/

#ifdef PEAK_HOLD
/*
 *	get the peak value within the last HOLDTIME samples
 */
	peak = 0.0;
	j = curcook;
	for( i = 0 ; i < HOLDTIME ; i++ ) {
		if( j < 0 ) {
			if( rollover )
				j += maxhist;
			else
				break;
		}
		if( peak < (usr[j]+sys[j]) ) peak = usr[j]+sys[j];
		j--;
	}
/*
 *	calculate the peak position and show it in reverse video
 */
	ppos = (peak+xincr*0.5)/xincr;
	if (ppos) {
		if( ppos > end ) 
			ppos = end;
		move( row, col+ppos );
		set_metric_color( (double) peak, bargraph_color, A_STANDOUT, 1 );
		if( ppos == pos )
			if( pos == spos )
				printw(gettxt("RTPM:618", "="));
			else
				printw(gettxt("RTPM:619", "-"));
		else
			printw(" ");
		attroff( A_STANDOUT );
	}
#endif
	set_default_color( 0 );
}

/*
 *	function: 	print_uname
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	print as much of the uname as possible at the bottom of the screen
 */
void
print_uname() {
	static int been_here = 0;
	static char name[128];
	struct utsname nm;
	size_t len;
	int i;

	if( !been_here ) {
		uname( &nm );
		strncpy( name, &nm.sysname[0], (size_t)(plock_col+plock_len) );
		if( ( (len=strlen( name )) + strlen( &nm.nodename[0] ) ) < (size_t)(plock_col+plock_len - 1)) {
			name[len++] = ' ';
			strcpy( &name[len], &nm.nodename[0] );
		} else goto unmdone;
		if( ( (len=strlen( name ))+ strlen( &nm.release[0] ) ) < (size_t)(plock_col+plock_len - 1)) {
			name[len++] = ' ';
			strcpy( &name[len], &nm.release[0] );
		} else goto unmdone;
		if( ( (len=strlen( name )) + strlen( &nm.machine[0] ) ) < (size_t)(plock_col+plock_len - 1)) {
			name[len++] = ' ';
			strcpy( &name[len], &nm.machine[0] );
		}
unmdone:	been_here = 1;
	}
	move( scr_rows-1, plock_col );
	for( i = plock_col; i < time_col; i++ )
		printw(" ");
	move( scr_rows-1, 0 ); 
	set_default_color( 0 );
	printw("%s", name );
}

/*
 *	function: 	print_plock
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	If rtpm is locked in memory via plock, print "LOCKED" at 
 *	the bottom of the screen.
 */
void
print_plock() {
	int i;

	if( !plocked && !(msg_disp & LOCK_MSG_BIT ))
		return;
	move( scr_rows - 1, plock_col-1 );
	for( i = plock_col-1; i < time_col; i++ )
		printw(" ");
	move( scr_rows - 1, plock_col );
	set_message_color( 0 );
	if( plocked ) {
		printw(gettxt("RTPM:10", "LOCKED"));
	} else if ( msg_disp & LOCK_MSG_BIT ) {
		printw(gettxt("RTPM:11", "cannot lock"));
	} 
	set_default_color( 0 );
}

/*
 *	function: 	print_time
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	print the time at the bottom of the screen
 */
void
print_time() {
	time_t t;
	static int toff = 0;
	char buf[64];
	int endcol, len;

	if( !toff || oldtime > currtime ) /* first time or rollover */
		toff = time(NULL) - times(&tbuf)/hz;

	t = currtime/hz + toff;
	strftime(buf, (size_t)64, NULL, localtime( &t ));
	len = strlen( buf );
	time_col = (scr_cols-len)/2;
	endcol = time_col+len;

	if(endcol >= (interval_col)){
		strftime(buf, (size_t)64, "%x %X", localtime( &t ));
		len = strlen( buf );
		time_col = (scr_cols-len)/2;
	}
	move( scr_rows-1, time_col );
	set_default_color( 0 );
	printw( buf );
}
/*
 *	function: 	horz_justify
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	figure out where things belong on the status line at the bottom of
 *	the screen.
 */
horz_justify() {
	char buf[64];
	int intv_len;

	plock_len = max( strlen(gettxt("RTPM:10", "LOCKED")),
	  strlen(gettxt("RTPM:11", "cannot lock")));
	sprintf( buf, gettxt("RTPM:12", "interval:%5d (%0.2f)"),
	  10000, 10000.00);
	intv_len = max( strlen(buf), 
	  strlen(gettxt("RTPM:13", "enter <?> for help")));
	intv_len = max( intv_len, plock_len );
	interval_col = scr_cols - intv_len - 1;
	plock_col = intv_len - plock_len;
}

/*
 *	function: 	print_help_msg
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	print_help_msg prints the help message teaser at the bottom row of
 *	the screen.
 */
void
print_help_msg() {
	set_default_color( 0 );
	color_clrtoeol( scr_rows-1, interval_col );
	set_message_color( A_STANDOUT );
	printw(gettxt("RTPM:13", "enter <?> for help"));
	set_default_color( 0 );
	attroff( A_STANDOUT );
	msg_disp |= HELP_MSG_BIT;
}
	

/*
 *	function: 	print_interval
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	print_interval() updates the display on the bottom row of 
 *	the screen with the current requested and actual intervals.
 */
void
print_interval() {
	set_default_color( 0 );
	color_clrtoeol( scr_rows-1, interval_col );
	printw(gettxt("RTPM:12", "interval:%5d (%0.2f)"), interval, tdiff);
}


/*
 *	function: 	print_sum
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the top level summary screen.
 */
void
print_sum(int row, int col ) {
	struct metric *metp, *metp2;
	int index;

	if( need_header & METS ) {
		mk_field( NULL, NULL, 0, 0, row, col+1,  12, gettxt("RTPM:60", "     CPU:   "), NULL, NULL, print_cputime );
		mk_field( NULL, NULL, 0, 0, row, col+14, 12, gettxt("RTPM:61", "   CALLS/s: "), NULL, NULL, print_syscalls );
		mk_field( NULL, NULL, 0, 0, row, col+27, 12, gettxt("RTPM:62", "    IO/s:   "), NULL, NULL, print_disk );
		mk_field( NULL, NULL, 0, 0, row, col+40, 12, gettxt("RTPM:63", "    QUEUE:  "), NULL, NULL, print_queue );
		mk_field( NULL, NULL, 0, 0, row, col+53, 12, gettxt("RTPM:64", "    TTY/s:  "), NULL, NULL, print_tty );
		mk_field( NULL, NULL, 0, 0, row, col+66, 12, gettxt("RTPM:65", "    ETHER:  "), NULL, NULL, print_eth );

		set_label_color( 0 );
		move(row+1,col+8);
		printw(gettxt("RTPM:66", "%%u+s         calls        reads        runq         rcvs         xpkt/s"));
		move(row+2,col+8);
		printw(gettxt("RTPM:67", "%%w+i         forks        rdblk        %%run         xmit         rpkt/s"));
		move(row+3,col+8);
		printw(gettxt("RTPM:68", "%%usr         execs        writs        prunq        mdms         xoct/s"));
		move(row+4,col+8);
		printw(gettxt("RTPM:69", "%%sys         reads        wrblk        %%prun        canch        roct/s"));
		move(row+5,col+8);
		printw(gettxt("RTPM:70", "%%wio         writs        qlen         swpq         rawch        xerrs"));
		move(row+6,col+8);
		printw(gettxt("RTPM:71", "%%idl         Krwch        %%busy        %%swp         outch        rerrs"));

		mk_field( NULL, NULL, 0, 0, row+8, col+1,  12, gettxt("RTPM:72", "   MEMORY:  "), NULL, NULL, print_mem );
		mk_field( NULL, NULL, 0, 0, row+8, col+14, 12, gettxt("RTPM:73", "  PAGING/s: "), NULL, NULL, print_paging );
		mk_field( NULL, NULL, 0, 0, row+8, col+27, 12, gettxt("RTPM:74", " FILESYS/s: "), NULL, NULL, print_fs );
		mk_field( NULL, NULL, 0, 0, row+8, col+40, 12, gettxt("RTPM:75", "    LWPS:   "), NULL, NULL, print_lwp );
		mk_field( NULL, NULL, 0, 0, row+8, col+53, 12, gettxt("RTPM:983", "   NETWARE: "), NULL, NULL, print_netware );
		mk_field( NULL, NULL, 0, 0, row+8, col+66, 12, gettxt("RTPM:982", "   TCP/IP:  "), NULL, NULL, print_net );

		set_label_color( 0 );
		move(row+9,col+8);
		printw(gettxt("RTPM:996", "kma          pgins        igets        lwps         spx/s        tcp/s"));
		move(row+10,col+8);
		printw(gettxt("RTPM:997", "frmem        pgots        lkups        run          ipx/s        udp/s"));
		move(row+11,col+8);
		printw(gettxt("RTPM:998", "frswp        atchs        dirbk        sleep        sap/s        icmp/s"));
		move(row+12,col+8);
		printw(gettxt("RTPM:999", "%%mem         pflts        %%dnlc        zomb         rip/s        ip/s"));
		move(row+13,col+8);
		printw(gettxt("RTPM:1000", "%%swp         vflts        inode        procs        errs         errs"));
	}
	metp = &mettbl[TOT_CPU_IDX];
	mk_field( metp, metp, ncpu, 0, row+1, col+1, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ TOT_IDL_IDX ];
	mk_field( metp, metp, ncpu, 0, row+2, col+1, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ USR_TIME_IDX ];
	mk_field( metp, metp, ncpu, 0, row+3, col+1, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ SYS_TIME_IDX ];
	mk_field( metp, metp, ncpu, 0, row+4, col+1, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ WIO_TIME_IDX ];
	mk_field( metp, metp, ncpu, 0, row+5, col+1, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ IDL_TIME_IDX ];
	mk_field( metp, metp, ncpu, 0, row+6, col+1, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );

	metp = &mettbl[SYSCALL_IDX];
	mk_field( metp, metp, ncpu, 0, row+1, col+14, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[FORK_IDX ];
	mk_field( metp, metp, ncpu, 0, row+2, col+14, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[EXEC_IDX ];
	mk_field( metp, metp, ncpu, 0, row+3, col+14, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[READ_IDX ];
	mk_field( metp, metp, ncpu, 0, row+4, col+14, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[WRITE_IDX ];
	mk_field( metp, metp, ncpu, 0, row+5, col+14, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[TOT_KRWCH_IDX ];
	mk_field( metp, metp, ncpu, 0, row+6, col+14, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );

	metp = &mettbl[DS_READ_IDX];
	mk_field( metp, metp, ndisk, 0, row+1, col+27, 6, "%6.0f", metp->metval[ndisk].cooked, metp->metval[ndisk].cooked, NULL );
	metp = &mettbl[DS_READBLK_IDX ];
	mk_field( metp, metp, ndisk, 0, row+2, col+27, 6, "%6.0f", metp->metval[ndisk].cooked, metp->metval[ndisk].cooked, NULL );
	metp = &mettbl[DS_WRITE_IDX ];
	mk_field( metp, metp, ndisk, 0, row+3, col+27, 6, "%6.0f", metp->metval[ndisk].cooked, metp->metval[ndisk].cooked, NULL );
	metp = &mettbl[DS_WRITEBLK_IDX ];
	mk_field( metp, metp, ndisk, 0, row+4, col+27, 6, "%6.0f", metp->metval[ndisk].cooked, metp->metval[ndisk].cooked, NULL );
	metp = &mettbl[DS_QLEN_IDX ];
	mk_field( metp, metp, ndisk, 0, row+5, col+27, 6, "%6.0f", metp->metval[ndisk].cooked, metp->metval[ndisk].cooked, NULL );
	metp = &mettbl[DS_ACTIVE_IDX ];
	mk_field( metp, metp, ndisk, 0, row+6, col+27, 6, "%6.0f", metp->metval[ndisk].cooked, metp->metval[ndisk].cooked, NULL );


	metp = &mettbl[RUNQ_IDX];
	mk_field( metp, metp, 0, 0, row+1, col+40, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[RUNOCC_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col+40, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[MPS_RUNQUE_IDX];
	mk_field( metp, metp, ncpu, 0, row+3, col+40, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[MPS_RUNOCC_IDX ];
	mk_field( metp, metp, ncpu, 0, row+4, col+40, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[SWPQ_IDX ];
	mk_field( metp, metp, 0, 0, row+5, col+40, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[SWPOCC_IDX ];
	mk_field( metp, metp, 0, 0, row+6, col+40, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[MPT_RCVINT_IDX];
	mk_field( metp, metp, ncpu, 0, row+1, col+53, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[MPT_XMTINT_IDX ];
	mk_field( metp, metp, ncpu, 0, row+2, col+53, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[MPT_MDMINT_IDX ];
	mk_field( metp, metp, ncpu, 0, row+3, col+53, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[MPT_CANCH_IDX ];
	mk_field( metp, metp, ncpu, 0, row+4, col+53, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[MPT_RAWCH_IDX ];
	mk_field( metp, metp, ncpu, 0, row+5, col+53, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[MPT_OUTCH_IDX];
	mk_field( metp, metp, ncpu, 0, row+6, col+53, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );

	metp = &mettbl[ETH_OutUcastPkts_IDX];
	mk_field( metp, metp, nether, 0, row+1, col+66, 6, "%6.0f", metp->metval[nether].cooked, metp->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_InUcastPkts_IDX];
	mk_field( metp, metp, nether, 0, row+2, col+66, 6, "%6.0f", metp->metval[nether].cooked, metp->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_OutOctets_IDX];
	mk_field( metp, metp, nether, 0, row+3, col+66, 6, "%6.0f", metp->metval[nether].cooked, metp->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_InOctets_IDX];
	mk_field( metp, metp, nether, 0, row+4, col+66, 6, "%6.0f", metp->metval[nether].cooked, metp->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_OutErrors_IDX];
	metp2 = &mettbl[ETHR_OutErrors_IDX];
	mk_field( metp, metp2, nether, 0, row+5, col+66, 6, "%6.0f", metp->metval[nether].cooked, metp2->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_InErrors_IDX];
	metp2 = &mettbl[ETHR_InErrors_IDX];
	mk_field( metp, metp2, nether, 0, row+6, col+66, 6, "%6.0f", metp->metval[nether].cooked, metp2->metval[nether].cooked, NULL );

	metp = &mettbl[TOT_KMA_PAGES_IDX];
	mk_field( metp, metp, 0, 0, row+9, col+1, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[FREEMEM_IDX ];
	mk_field( metp, metp, 0, 0, row+10, col+1, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[FREESWAP_IDX ];
	mk_field( metp, metp, 0, 0, row+11, col+1, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[MEM_PERCENT_IDX ];
	mk_field( metp, metp, 0, 0, row+12, col+1, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[MEM_SWAP_PERCENT_IDX ];
	mk_field( metp, metp, 0, 0, row+13, col+1, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[PGIN_IDX];
	mk_field( metp, metp, ncpu, 0, row+9, col+14, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[PGOUT_IDX ];
	mk_field( metp, metp, ncpu, 0, row+10, col+14, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ATCH_IDX ];
	mk_field( metp, metp, ncpu, 0, row+11, col+14, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[PFAULT_IDX ];
	mk_field( metp, metp, ncpu, 0, row+12, col+14, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[VFAULT_IDX ];
	mk_field( metp, metp, ncpu, 0, row+13, col+14, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );

	index = (ncpu+1)*(nfstyp+1)-1;	/* total of all cpus and fstypes */
 	metp = &mettbl[IGET_IDX];
	mk_field( metp, metp, ncpu, nfstyp, row+9, col+27, 6, "%6.0f", metp->metval[index].cooked, metp->metval[index].cooked, NULL );
	metp = &mettbl[LOOKUP_IDX ];
	mk_field( metp, metp, ncpu, 0, row+10, col+27, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[DIRBLK_IDX ];
	mk_field( metp, metp, ncpu, nfstyp, row+11, col+27, 6, "%6.0f", metp->metval[index].cooked, metp->metval[index].cooked, NULL );
	metp = &mettbl[DNLC_PERCENT_IDX ];
	mk_field( metp, metp, ncpu, 0, row+12, col+27, 6, "%6.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[INUSEINODE_IDX];
	mk_field( metp, metp, ncpu, 0, row+13, col+27, 6, "%6.0f", metp->metval[nfstyp].cooked, metp->metval[nfstyp].cooked, NULL );


 	metp = &mettbl[LWP_TOTAL_IDX];
	mk_field( metp, metp, 0, 0, row+9, col+40, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[LWP_RUN_IDX ];
	mk_field( metp, metp, 0, 0, row+10, col+40, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[LWP_SLEEP_IDX ];
	mk_field( metp, metp, 0, 0, row+11, col+40, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[LWP_ZOMB_IDX ];
	mk_field( metp, metp, 0, 0, row+12, col+40, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[PROCUSE_IDX ];
	mk_field( metp, metp, 0, 0, row+13, col+40, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SPXR_total_IDX ];
	mk_field( metp, metp, 0, 0, row+9, col+53, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ IPXR_total_IDX ];
	mk_field( metp, metp, 0, 0, row+10, col+53, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ SAPR_total_IDX ];
	mk_field( metp, metp, 0, 0, row+11, col+53, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_total_IDX ];
	mk_field( metp, metp, 0, 0, row+12, col+53, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ NETWARE_errs_IDX ];
	metp2 = &mettbl[ NETWARER_errs_IDX ];
	mk_field( metp, metp2, 0, 0, row+13, col+53, 6, "%6.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

 	metp = &mettbl[TCPR_sum_IDX];
	mk_field( metp, metp, 0, 0, row+9, col+66, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[UDPR_sum_IDX ];
	mk_field( metp, metp, 0, 0, row+10, col+66, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ICMPR_sum_IDX ];
	mk_field( metp, metp, 0, 0, row+11, col+66, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[IPR_sum_IDX ];
	mk_field( metp, metp, 0, 0, row+12, col+66, 6, "%6.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[NETERR_sum_IDX ];
	metp2 = &mettbl[NETERRR_sum_IDX ];
	mk_field( metp, metp2, 0, 0, row+13, col+66, 6, "%6.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
}



/*
 *	function: 	print_cputime
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the cpu usage subscreen.
 */
void
print_cputime( int row, int col ) {

	print_percpu_hdr( row++, col );

	rprint_metric(  &mettbl[TOT_CPU_IDX], &mettbl[TOT_CPU_IDX], row++, col, 2 );
	rprint_metric(  &mettbl[TOT_IDL_IDX], &mettbl[TOT_IDL_IDX], row++, col, 2 );
	rprint_metric(  &mettbl[USR_TIME_IDX], &mettbl[USR_TIME_IDX], row++, col, 2 );
	rprint_metric(  &mettbl[SYS_TIME_IDX], &mettbl[SYS_TIME_IDX], row++, col, 2 );
	rprint_metric(  &mettbl[WIO_TIME_IDX], &mettbl[WIO_TIME_IDX], row++, col, 2 );
	rprint_metric(  &mettbl[IDL_TIME_IDX], &mettbl[IDL_TIME_IDX], row++, col, 2 );
	if( eisa_bus_util_flg ) {
		row++;
		rprint_metric(  &mettbl[EISA_BUS_UTIL_PERCENT_IDX],
		 &mettbl[EISA_BUS_UTIL_PERCENT_IDX], row, col, 2 );
	}
}

/*
 *	function: 	print_mem
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the kma/memory usage subscreen
 */
void
print_mem( int row, int col ) {
	struct metric *metp = &mettbl[KMEM_MEM_IDX];	/* total pool	*/
	struct metric *alloc = &mettbl[KMEM_BALLOC_IDX];/* allocated	*/
	struct metric *req = &mettbl[KMEM_RALLOC_IDX];	/* requested	*/
	struct metric *fail = &mettbl[KMEM_FAIL_IDX];	/* failed reqs	*/
	struct metric *failr = &mettbl[KMEMR_FAIL_IDX]; /* fail req/s	*/

	int i, j;
	int dimx = metp->resval[0]+1;
	int dimy = metp->resval[1]+1;

	if( need_header & METS ) {
		assert( metp->ndim == 2 );
		assert( metp->reslist[0] == NCPU );
		assert( metp->reslist[1] == KMPOOLS );
		assert( alloc->ndim == 2 );
		assert( alloc->reslist[0] == NCPU );
		assert( alloc->reslist[1] == KMPOOLS );
		assert( req->ndim == 2 );
		assert( req->reslist[0] == NCPU );
		assert( req->reslist[1] == KMPOOLS );

		set_label_color( 0 );
		move( row, col+1 );
		printw(gettxt("RTPM:83", "         frmem          frswpm           frswpdsk          swpmem          mem"));
		move( row+1, col+1 );
		printw(gettxt("RTPM:84", "         %%mem           %%swpmem          %%swpdsk           swpdsk          kma"));
		move( row+3, col+1 );
		printw(gettxt("RTPM:85", "kmasz      mem    alloc      req  fail  kmasz      mem    alloc      req  fail"));

		for(j=4,i=0;i<nkmpool;i++){
			move(row+j,col+2);
			if( kmasize[i].met.sngl ) {
				printw("%4d",kmasize[i].met.sngl);
				j++;
			}
/*
 *			move to second column of kma report
 */
			if( kmasize[i].met.sngl == 8192 ) {
				j = 4;
				col += 40;
			}
		}
		col -= 40;
		move(row+12,col+42);
		printw(gettxt("RTPM:86", "ovsz"));
		move(row+13,col+41);
		printw(gettxt("RTPM:50", "total"));
	}


	metp = &mettbl[FREEMEM_IDX];
	mk_field( metp, metp, 0, 1, row,col+1, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL);
	metp = &mettbl[MEM_PERCENT_IDX];
	mk_field( metp, metp, 0, 1, row+1,col+1, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL);
	metp = &mettbl[FREESWAP_IDX];
	mk_field( metp, metp, 0, 1, row,col+16, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL);
	metp = &mettbl[MEM_SWAP_PERCENT_IDX];
	mk_field( metp, metp, 0, 1, row+1,col+16, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL);
	metp = &mettbl[DSK_SWAPPGFREE_IDX];
	mk_field( metp, metp, 0, 1, row,col+33, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL);
	metp = &mettbl[DSK_SWAP_PERCENT_IDX];
	mk_field( metp, metp, 0, 1, row+1,col+33, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL);
	metp = &mettbl[MEM_SWAPPG_IDX];
	mk_field( metp, metp, 0, 1, row,col+51, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL);
	metp = &mettbl[DSK_SWAPPG_IDX];
	mk_field( metp, metp, 0, 1, row+1,col+51, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL);
	metp = &mettbl[TOTALMEM_IDX];
	mk_field( metp, metp, 0, 1, row,col+67, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL);
	metp = &mettbl[TOT_KMA_PAGES_IDX];
	mk_field( metp, metp, 0, 1, row+1,col+67, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL);

/*
 *	print the kma report
 */
	row += 4;
	metp = &mettbl[KMEM_MEM_IDX];
	for(i=j=0;i<nkmpool;i++){
/*
 *		only report on pool sizes that are actually set
 */
		if( kmasize[i].met.sngl || (i == nkmpool-1) ) {
/*
 *			put the ovsz stats at second to last row of the
 *			second column
 */
			if( i == nkmpool-1)
				j = 8;
			mk_field( metp, metp, ncpu, i, row+j,col+7, 8, "%8.0f", metp->metval[subscript(dimx-1,i)].cooked, metp->metval[subscript(dimx-1,i)].cooked, NULL );
			mk_field( alloc, alloc, ncpu, i, row+j,col+16, 8, "%8.0f", alloc->metval[subscript(dimx-1,i)].cooked, alloc->metval[subscript(dimx-1,i)].cooked, NULL );
			mk_field( req, req, ncpu, i, row+j,col+25, 8, "%8.0f", req->metval[subscript(dimx-1,i)].cooked, req->metval[subscript(dimx-1,i)].cooked, NULL );
			mk_field( fail, failr, ncpu, i, row+j,col+34, 5, "%5.0f", fail->metval[subscript(dimx-1,i)].cooked, failr->metval[subscript(dimx-1,i)].cooked, NULL );
			j++;
		}
/*
 *		move to the second column of the kma report
 */
		if( kmasize[i].met.sngl == 8192 ) {
			col += 40; j=0;
		}
	}
/*
 *	print the totals at the bottom of the second column
 */
	mk_field( metp, metp, ncpu, i, row+j,col+7, 8, "%8.0f", metp->metval[subscript(dimx-1,i)].cooked, metp->metval[subscript(dimx-1,i)].cooked, NULL );
	mk_field( alloc, alloc, ncpu, i, row+j,col+16, 8, "%8.0f", alloc->metval[subscript(dimx-1,i)].cooked, alloc->metval[subscript(dimx-1,i)].cooked, NULL );
	mk_field( req, req, ncpu, i, row+j,col+25, 8, "%8.0f", req->metval[subscript(dimx-1,i)].cooked, req->metval[subscript(dimx-1,i)].cooked, NULL );
	mk_field( fail, failr, ncpu, i, row+j,col+34, 5, "%5.0f", fail->metval[subscript(dimx-1,i)].cooked, failr->metval[subscript(dimx-1,i)].cooked, NULL );
}


/*
 *	function: 	print_lwp
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the proc/lwp usage subscreen
 */
void
print_lwp( int row, int col ) {
	struct metric *metp, *metp2;
	int ocol = col;
/*
 *	center the top part of this report
 */
	col += (scr_cols - 80)/2;
	if( need_header & METS ) {
		set_label_color( 0 );
		move(row,col+9);
		printw(gettxt("RTPM:87", "lwps                runnable lwps         zombie lwps         procs"));
		move(row+1,col+9);
		printw(gettxt("RTPM:88", "lwps on cpu         sleeping lwps         idle lwps           procmax"));
		move(row+2,col+9);
		printw(gettxt("RTPM:89", "lwpfail             stopped lwps          other lwps          procfail"));
	}
	metp = &mettbl[LWP_TOTAL_IDX];
	mk_field( metp, metp, 0, 0, row, col+1, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[LWP_ONPROC_IDX];
	mk_field( metp, metp, 0, 0, row+1, col+1, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[MPR_LWP_FAIL_IDX];
	metp2 = &mettbl[MPR_LWP_FAILR_IDX];
	mk_field( metp, metp2, ncpu, 0, row+2, col+1, 7, "%7.0f", metp->metval[ncpu].cooked, metp2->metval[ncpu].cooked, NULL );
	metp = &mettbl[LWP_RUN_IDX];
	mk_field( metp, metp, 0, 0, row, col+21, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[LWP_SLEEP_IDX];
	mk_field( metp, metp, 0, 0, row+1, col+21, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[LWP_STOP_IDX];
	mk_field( metp, metp, 0, 0, row+2, col+21, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[LWP_ZOMB_IDX];
	mk_field( metp, metp, 0, 0, row, col+43, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[LWP_IDLE_IDX];
	mk_field( metp, metp, 0, 0, row+1, col+43, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[LWP_OTHER_IDX];
	mk_field( metp, metp, 0, 0, row+2, col+43, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[PROCUSE_IDX];
	mk_field( metp, metp, 0, 0, row, col+63, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[PROCMAX_IDX];
	mk_field( metp, metp, 0, 0, row+1, col+63, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[PROCFAIL_IDX];
	metp2 = &mettbl[PROCFAILR_IDX];
	mk_field( metp, metp2, 0, 0, row+2, col+63, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	row += 4;
/* 
 *	display the ps info
 */
	print_proc( row, ocol );
}


/*
 *	function: 	print_pgin
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the page in subscreen
 */
void
print_pgin( int row, int col ) {

	print_percpu_hdr( row++, col );

	rprint_metric(  &mettbl[PGIN_IDX], &mettbl[PGIN_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[PGPGIN_IDX], &mettbl[PGPGIN_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[PREATCH_IDX], &mettbl[PREATCH_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[ATCH_IDX], &mettbl[ATCH_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[ATCHFREE_IDX], &mettbl[ATCHFREE_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[ATCHFREE_PGOUT_IDX], &mettbl[ATCHFREE_PGOUT_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[ATCHMISS_IDX], &mettbl[ATCHMISS_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[PFAULT_IDX], &mettbl[PFAULT_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[VFAULT_IDX], &mettbl[VFAULT_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[SFTLCK_IDX], &mettbl[SFTLCK_IDX], row++, col, 1 );
}


/*
 *	function: 	print_pgout
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the page out subscreen
 */
void
print_pgout( int row, int col ) {

	print_percpu_hdr( row++, col );

	rprint_metric(  &mettbl[PGOUT_IDX], &mettbl[PGOUT_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[PGPGOUT_IDX], &mettbl[PGPGOUT_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[VIRSCAN_IDX], &mettbl[VIRSCAN_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[VIRFREE_IDX], &mettbl[VIRFREE_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[PHYSFREE_IDX], &mettbl[PHYSFREE_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[SWPOUT_IDX], &mettbl[SWPOUT_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[PPGSWPOUT_IDX], &mettbl[PPGSWPOUT_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[VPGSWPOUT_IDX], &mettbl[VPGSWPOUT_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[SWPIN_IDX], &mettbl[SWPIN_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[PGSWPIN_IDX], &mettbl[PGSWPIN_IDX], row++, col, 1 );
}


/*
 *	function: 	print_paging
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the paging summary subscreen
 */
void
print_paging( int row, int col ) {
	struct metric *metp;	

	if( need_header & METS ) {
		mk_field( NULL, NULL, 0, 0, row, col+2,  20, gettxt("RTPM:90", "      PAGE IN:      "), NULL, NULL, print_pgin );
		mk_field( NULL, NULL, 0, 0, row, col+40,  20, gettxt("RTPM:91", " PAGE OUT/SWAPPING: "), NULL, NULL, print_pgout );

		set_label_color( 0 );
		move(row+1,col+11);
		printw(gettxt("RTPM:92", "page ins/s                            page outs/s"));
		move(row+2,col+11);
		printw(gettxt("RTPM:93", "pages paged in/s                      pages paged out/s"));
		move(row+3,col+11);
		printw(gettxt("RTPM:94", "preattaches/s                         virtual pages scanned/s"));
		move(row+4,col+11);
		printw(gettxt("RTPM:95", "attaches/s                            virtual pages freed/s"));
		move(row+5,col+11);
		printw(gettxt("RTPM:96", "attaches from freelist/s              physical pages freed/s"));
		move(row+6,col+11);
		printw(gettxt("RTPM:97", "attach from freelist pgout/s          swap outs/s"));
		move(row+7,col+11);
		printw(gettxt("RTPM:98", "attach misses/s                       physical pages swapped out/s"));
		move(row+8,col+11);
		printw(gettxt("RTPM:99", "protection faults/s                   virtual pages swapped out/s"));
		move(row+9,col+11);
		printw(gettxt("RTPM:100", "validity faults/s                     swap ins/s"));
		move(row+10,col+11);
		printw(gettxt("RTPM:101", "software locks/s                      pages swapped in/s"));

	}
	metp = &mettbl[PGIN_IDX];
	mk_field( metp, metp, ncpu, 0, row+1, col+2, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[PGPGIN_IDX];
	mk_field( metp, metp, ncpu, 0, row+2, col+2, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[PREATCH_IDX];
	mk_field( metp, metp, ncpu, 0, row+3, col+2, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ATCH_IDX];
	mk_field( metp, metp, ncpu, 0, row+4, col+2, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ATCHFREE_IDX];
	mk_field( metp, metp, ncpu, 0, row+5, col+2, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ATCHFREE_PGOUT_IDX];
	mk_field( metp, metp, ncpu, 0, row+6, col+2, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ATCHMISS_IDX];
	mk_field( metp, metp, ncpu, 0, row+7, col+2, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[PFAULT_IDX];
	mk_field( metp, metp, ncpu, 0, row+8, col+2, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[VFAULT_IDX];
	mk_field( metp, metp, ncpu, 0, row+9, col+2, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[SFTLCK_IDX];
	mk_field( metp, metp, ncpu, 0, row+10, col+2, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );

	metp = &mettbl[PGOUT_IDX];
	mk_field( metp, metp, ncpu, 0, row+1, col+40, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[PGPGOUT_IDX];
	mk_field( metp, metp, ncpu, 0, row+2, col+40, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[VIRSCAN_IDX];
	mk_field( metp, metp, ncpu, 0, row+3, col+40, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[VIRFREE_IDX];
	mk_field( metp, metp, ncpu, 0, row+4, col+40, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[PHYSFREE_IDX];
	mk_field( metp, metp, ncpu, 0, row+5, col+40, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[SWPOUT_IDX];
	mk_field( metp, metp, ncpu, 0, row+6, col+40, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[PPGSWPOUT_IDX];
	mk_field( metp, metp, ncpu, 0, row+7, col+40, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[VPGSWPOUT_IDX];
	mk_field( metp, metp, ncpu, 0, row+8, col+40, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[SWPIN_IDX];
	mk_field( metp, metp, ncpu, 0, row+9, col+40, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[PGSWPIN_IDX];
	mk_field( metp, metp, ncpu, 0, row+10, col+40, 8, "%8.1f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
}


/*
 *	function: 	print_syscalls
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the system calls subscreen
 */
void
print_syscalls( int row, int col ) {

	print_percpu_hdr( row++, col );

	rprint_metric( &mettbl[SYSCALL_IDX],&mettbl[SYSCALL_IDX], row++, col, 0 );
	rprint_metric( &mettbl[FORK_IDX],&mettbl[FORK_IDX], row++, col, 0 );
	rprint_metric( &mettbl[LWPCREATE_IDX],&mettbl[LWPCREATE_IDX], row++, col, 0 );
	rprint_metric( &mettbl[EXEC_IDX],&mettbl[EXEC_IDX], row++, col, 0 );
	rprint_metric( &mettbl[READ_IDX],&mettbl[READ_IDX], row++, col, 0 );
	rprint_metric( &mettbl[WRITE_IDX],&mettbl[WRITE_IDX], row++, col, 0 );
	rprint_metric( &mettbl[READCH_IDX],&mettbl[READCH_IDX], row++, col, 0 );
	rprint_metric( &mettbl[WRITECH_IDX],&mettbl[WRITECH_IDX], row++, col, 0 );
	rprint_metric( &mettbl[MPI_SEMA_IDX],&mettbl[MPI_SEMA_IDX], row++, col, 0 );
	rprint_metric( &mettbl[MPI_MSG_IDX],&mettbl[MPI_MSG_IDX], row++, col, 0 );
}


/*
 *	function: 	print_queue
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the run queue, per processor run queue,
 *	and swap queue subscreen
 */
void
print_queue( int row, int col ) {

	print_percpu_hdr( row++, col );

	rprint_metric(  &mettbl[MPS_PSWITCH_IDX], &mettbl[MPS_PSWITCH_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[MPS_RUNQUE_IDX], &mettbl[MPS_RUNQUE_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[MPS_RUNOCC_IDX], &mettbl[MPS_RUNOCC_IDX], row++, col, 1 );
	row++;
	rprint_metric(  &mettbl[RUNQ_IDX], &mettbl[RUNQ_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[RUNOCC_IDX], &mettbl[RUNOCC_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[SWPQ_IDX], &mettbl[SWPQ_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[SWPOCC_IDX], &mettbl[SWPOCC_IDX], row++, col, 1 );
}


/*
 *	function: 	print_tty
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the tty subscreen
 */
void
print_tty( int row, int col ) {
	struct metric *metp, *metp2;

	print_percpu_hdr( row++, col );

	rprint_metric(  &mettbl[MPT_RCVINT_IDX], &mettbl[MPT_RCVINT_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[MPT_XMTINT_IDX ], &mettbl[MPT_XMTINT_IDX ], row++, col, 1 );
	rprint_metric(  &mettbl[MPT_MDMINT_IDX ], &mettbl[MPT_MDMINT_IDX ], row++, col, 1 );
	rprint_metric(  &mettbl[MPT_CANCH_IDX ], &mettbl[MPT_CANCH_IDX ], row++, col, 1 );
	rprint_metric(  &mettbl[MPT_RAWCH_IDX ], &mettbl[MPT_RAWCH_IDX ], row++, col, 1 );
	rprint_metric(  &mettbl[MPT_OUTCH_IDX], &mettbl[MPT_OUTCH_IDX], row++, col, 1 );
	row++;
/*
 *	Can't use rprint here because streams stats are kept per cpu
 *	to avoid having to lock them.  The per-cpu numbers are meaningless,
 *	only the totals matter.
 */
	if( need_header & METS ) {
		set_label_color( 0 );
		move( row, col + 20 );
		printw(gettxt("RTPM:984", "streams           streams/s"));
		move( row+1, col + 20 );
		printw(gettxt("RTPM:985", "queues            queues/s"));
		move( row+2, col + 20 );
		printw(gettxt("RTPM:986", "mdbblks           mdbblks/s"));
		move( row+3, col + 20 );
		printw(gettxt("RTPM:987", "msgblks           msgblks/s"));
		move( row+4, col + 20 );
		printw(gettxt("RTPM:988", "links             links/s"));
		move( row+5, col + 20 );
		printw(gettxt("RTPM:989", "events            events/s          eventfails"));
	}
	metp = &mettbl[ STR_STREAM_INUSE_IDX ];
	mk_field( metp, metp, ncpu, 0, row, col+11, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ STR_STREAM_TOTAL_IDX ];
	mk_field( metp, metp, ncpu, 0, row, col+29, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ STR_QUEUE_INUSE_IDX ];
	mk_field( metp, metp, ncpu, 0, row + 1, col+11, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ STR_QUEUE_TOTAL_IDX ];
	mk_field( metp, metp, ncpu, 0, row + 1, col+29, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ STR_MDBBLK_INUSE_IDX ];
	mk_field( metp, metp, ncpu, 0, row + 2, col+11, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ STR_MDBBLK_TOTAL_IDX ];
	mk_field( metp, metp, ncpu, 0, row + 2, col+29, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ STR_MSGBLK_INUSE_IDX ];
	mk_field( metp, metp, ncpu, 0, row + 3, col+11, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ STR_MSGBLK_TOTAL_IDX ];
	mk_field( metp, metp, ncpu, 0, row + 3, col+29, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ STR_LINK_INUSE_IDX ];
	mk_field( metp, metp, ncpu, 0, row + 4, col+11, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ STR_LINK_TOTAL_IDX ];
	mk_field( metp, metp, ncpu, 0, row + 4, col+29, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ STR_EVENT_INUSE_IDX ];
	mk_field( metp, metp, ncpu, 0, row + 5, col+11, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ STR_EVENT_TOTAL_IDX ];
	mk_field( metp, metp, ncpu, 0, row + 5, col+29, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ STR_EVENT_FAIL_IDX ];
	metp2 = &mettbl[ STR_EVENT_FAILR_IDX ];
	mk_field( metp, metp2, ncpu, 0, row + 5, col+47, 8, "%8.0f", metp->metval[ncpu].cooked, metp2->metval[ncpu].cooked, NULL );
}


/*
 *	function: 	print_disk
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the disk statistics subscreen
 */
void
print_disk( int row, int col ) {
	print_perdisk_hdr( row++, col );

	rprint_metric( &mettbl[DS_READ_IDX],&mettbl[DS_READ_IDX], row++, col, 1 );
	rprint_metric( &mettbl[DS_READBLK_IDX],&mettbl[DS_READBLK_IDX], row++, col, 1 );
	rprint_metric( &mettbl[DS_WRITE_IDX],&mettbl[DS_WRITE_IDX], row++, col, 1 );
	rprint_metric( &mettbl[DS_WRITEBLK_IDX],&mettbl[DS_WRITEBLK_IDX], row++, col, 1 );
	rprint_metric( &mettbl[DS_QLEN_IDX],&mettbl[DS_QLEN_IDX], row++, col, 1 );
	rprint_metric( &mettbl[DS_RESP_IDX],&mettbl[DS_RESP_IDX], row++, col, 1 );
	rprint_metric( &mettbl[DS_ACTIVE_IDX],&mettbl[DS_ACTIVE_IDX], row++, col, 1 );
}


/*
 *	function: 	print_fscalls
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the file system calls subscreen
 */
void
print_fscalls( int row, int col ) {

	print_percpu_hdr( row++, col );

	rprint_metric( &mettbl[READ_IDX],&mettbl[READ_IDX], row++, col, 0);
	rprint_metric( &mettbl[WRITE_IDX],&mettbl[WRITE_IDX], row++, col, 0 );
	rprint_metric( &mettbl[READCH_IDX],&mettbl[READCH_IDX], row++, col, 0);
	rprint_metric( &mettbl[WRITECH_IDX],&mettbl[WRITECH_IDX], row++, col, 0 );
	rprint_metric( &mettbl[LOOKUP_IDX],&mettbl[LOOKUP_IDX], row++, col, 0 );
	rprint_metric( &mettbl[DNLCHITS_IDX],&mettbl[DNLCHITS_IDX], row++, col, 0 );
	rprint_metric( &mettbl[DNLCMISS_IDX],&mettbl[DNLCMISS_IDX], row++, col, 0 );
	rprint_metric( &mettbl[DNLC_PERCENT_IDX],&mettbl[DNLC_PERCENT_IDX], row++, col, 1 );
}


/*
 *	function: 	print_igets_dirblks
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the igets and dirblks subscreen
 */
void
print_igets_dirblks( int row, int col ) {

	print_percpu_hdr( row, col );

	rprint_metric(  &mettbl[IGET_IDX], &mettbl[IGET_IDX], row++, col, 1 );
	row += nfstyp+2;
	rprint_metric(  &mettbl[DIRBLK_IDX], &mettbl[DIRBLK_IDX], row++, col, 1 );
	row += nfstyp+1;
}


/*
 *	function: 	print_inoreclaim
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the inode reclaims subscreen
 */
void
print_inoreclaim( int row, int col ) {

	print_percpu_hdr( row, col );

	rprint_metric(  &mettbl[IPAGE_IDX], &mettbl[IPAGE_IDX], row++, col, 1 );
	row += nfstyp+2;
	rprint_metric(  &mettbl[INOPAGE_IDX], &mettbl[INOPAGE_IDX], row++, col, 1 );
	row += nfstyp+1;
}


/*
 *	function: 	print_bufcache
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the buffer cache subscreen
 */
void
print_bufcache( int row, int col ) {

	print_percpu_hdr( row++, col );

	rprint_metric(  &mettbl[MPB_LREAD_IDX], &mettbl[MPB_LREAD_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[MPB_LWRITE_IDX ], &mettbl[MPB_LWRITE_IDX ], row++, col, 1 );
	rprint_metric(  &mettbl[MPB_BREAD_IDX ], &mettbl[MPB_BREAD_IDX ], row++, col, 1 );
	rprint_metric(  &mettbl[MPB_BWRITE_IDX ], &mettbl[MPB_BWRITE_IDX ], row++, col, 1 );
	rprint_metric(  &mettbl[MPB_PHREAD_IDX ], &mettbl[MPB_PHREAD_IDX ], row++, col, 1 );
	rprint_metric(  &mettbl[MPB_PHWRITE_IDX ], &mettbl[MPB_PHWRITE_IDX ], row++, col, 1 );
	rprint_metric(  &mettbl[RCACHE_PERCENT_IDX ], &mettbl[RCACHE_PERCENT_IDX ], row++, col, 1 );
	rprint_metric(  &mettbl[WCACHE_PERCENT_IDX ], &mettbl[WCACHE_PERCENT_IDX ], row++, col, 1 );
}


/*
 *	function: 	print_fstbls
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the file system tables subscreen
 */
void
print_fstbls( int row, int col ) {

	print_perfs_hdr( row++, col );

	rprint_metric(  &mettbl[MAXINODE_IDX], &mettbl[MAXINODE_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[CURRINODE_IDX ], &mettbl[CURRINODE_IDX ], row++, col, 0 );
	rprint_metric(  &mettbl[INUSEINODE_IDX ], &mettbl[INUSEINODE_IDX ], row++, col, 0 );
	rprint_metric(  &mettbl[FAILINODE_IDX ], &mettbl[FAILINODER_IDX ], row++, col, 0 );
	row++;
	rprint_metric(  &mettbl[FILETBLINUSE_IDX ], &mettbl[FILETBLINUSE_IDX ], row++, col, 0 );
	rprint_metric(  &mettbl[FILETBLFAIL_IDX], &mettbl[FILETBLFAILR_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[FLCKTBLMAX_IDX], &mettbl[FLCKTBLMAX_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[FLCKTBLTOTAL_IDX], &mettbl[FLCKTBLTOTAL_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[FLCKTBLINUSE_IDX], &mettbl[FLCKTBLINUSE_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[FLCKTBLFAIL_IDX], &mettbl[FLCKTBLFAILR_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[FSWIO_IDX], &mettbl[FSWIO_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[PHYSWIO_IDX], &mettbl[PHYSWIO_IDX], row++, col, 0 );
}


/*
 *	function: 	print_fs
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the file system summary subscreen
 */
void
print_fs(int row, int col ) {
	struct metric *metp, *metp2;	
	int index;

	if( need_header & METS ) {
		mk_field( NULL, NULL, 0, 0, row, col+2,  20,gettxt("RTPM:109", "   FILE SYS CALLS:  "), NULL, NULL, print_fscalls );
		mk_field( NULL, NULL, 0, 0, row, col+30, 20,gettxt("RTPM:110", "    BUFFER CACHE:   "), NULL, NULL, print_bufcache );
		mk_field( NULL, NULL, 0, 0, row, col+56, 20, gettxt("RTPM:111", "     MISC/TABLES:   "), NULL, NULL, print_fstbls );
		set_label_color( 0 );
		move(row+1,col+11);
		printw(gettxt("RTPM:112", "reads/s                     lreads/s                  max inodes"));
		move(row+2,col+11);
		printw(gettxt("RTPM:113", "writes/s                    lwrites/s                 curr inodes"));
		move(row+3,col+11);
		printw(gettxt("RTPM:114", "readchars/s                 breads/s                  inuse inodes"));
		move(row+4,col+11);
		printw(gettxt("RTPM:115", "writechars/s                bwrites/s                 fail inodes"));
		move(row+5,col+11);
		printw(gettxt("RTPM:116", "lookups/s                   phreads/s                 file tbl inuse"));
		move(row+6,col+11);
		printw(gettxt("RTPM:117", "dir hits/s                  phwrites/s                file tbl fail"));
		move(row+7,col+11);
		printw(gettxt("RTPM:118", "dir misses/s                %%rcache                   flck tbl max"));
		move(row+8,col+11);
		printw(gettxt("RTPM:119", "%%dnlc hits                  %%wcache                   flck tbl/s"));
		move(row+9,col+11);
		printw(gettxt("RTPM:120", "                                                      flck tbl inuse"));
		move(row+10,col+11);
		printw(gettxt("RTPM:121", "                                                      flck tbl fail"));
		move(row+11,col+11);
		printw(gettxt("RTPM:122", "igets/s                     recl with pgs/s           fs wio jobs"));
		move(row+12,col+11);
		printw(gettxt("RTPM:123", "dirblks/s                   recl w/o pgs/s            phys wio jobs"));
/*
 *		These have to come after the above lines, or the
 *		display of the headers is cleared by one of the 
 *		lines that prints a label
 */
		mk_field( NULL, NULL, 0, 0, row+10, col+2, 20, gettxt("RTPM:124", "   IGETS/DIRBLKS:   "),  NULL,  NULL, print_igets_dirblks );
		mk_field( NULL, NULL, 0, 0, row+10, col+30, 20,gettxt("RTPM:125", "   INODE RECLAIMS:  "),  NULL,  NULL, print_inoreclaim );

	}
	metp = &mettbl[READ_IDX];
	mk_field( metp, metp, ncpu, 0, row+1, col+2, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ WRITE_IDX ];
	mk_field( metp, metp, ncpu, 0, row+2, col+2, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ READCH_IDX ];
	mk_field( metp, metp, ncpu, 0, row+3, col+2, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ WRITECH_IDX ];
	mk_field( metp, metp, ncpu, 0, row+4, col+2, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ LOOKUP_IDX ];
	mk_field( metp, metp, ncpu, 0, row+5, col+2, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ DNLCHITS_IDX ];
	mk_field( metp, metp, ncpu, 0, row+6, col+2, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ DNLCMISS_IDX ];
	mk_field( metp, metp, ncpu, 0, row+7, col+2, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[ DNLC_PERCENT_IDX ];
	mk_field( metp, metp, ncpu, 0, row+8, col+2, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );

	metp = &mettbl[MPB_LREAD_IDX];
	mk_field( metp, metp, ncpu, 0, row+1, col+30, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[MPB_LWRITE_IDX ];
	mk_field( metp, metp, ncpu, 0, row+2, col+30, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[MPB_BREAD_IDX ];
	mk_field( metp, metp, ncpu, 0, row+3, col+30, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[MPB_BWRITE_IDX ];
	mk_field( metp, metp, ncpu, 0, row+4, col+30, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[MPB_PHREAD_IDX ];
	mk_field( metp, metp, ncpu, 0, row+5, col+30, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[MPB_PHWRITE_IDX ];
	mk_field( metp, metp, ncpu, 0, row+6, col+30, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[RCACHE_PERCENT_IDX ];
	mk_field( metp, metp, ncpu, 0, row+7, col+30, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[WCACHE_PERCENT_IDX ];
	mk_field( metp, metp, ncpu, 0, row+8, col+30, 8, "%8.0f", metp->metval[ncpu].cooked, metp->metval[ncpu].cooked, NULL );
	metp = &mettbl[MAXINODE_IDX];
	mk_field( metp, metp, nfstyp, 0, row+1, col+56, 8, "%8.0f", metp->metval[nfstyp].cooked, metp->metval[nfstyp].cooked, NULL );
	metp = &mettbl[CURRINODE_IDX ];
	mk_field( metp, metp, nfstyp, 0, row+2, col+56, 8, "%8.0f", metp->metval[nfstyp].cooked, metp->metval[nfstyp].cooked, NULL );
	metp = &mettbl[INUSEINODE_IDX ];
	mk_field( metp, metp, nfstyp, 0, row+3, col+56, 8, "%8.0f", metp->metval[nfstyp].cooked, metp->metval[nfstyp].cooked, NULL );
	metp = &mettbl[FAILINODE_IDX ];
	metp2 = &mettbl[FAILINODER_IDX ];
	mk_field( metp, metp2, nfstyp, 0, row+4, col+56, 8, "%8.0f", metp->metval[nfstyp].cooked, metp2->metval[nfstyp].cooked, NULL );
	metp = &mettbl[FILETBLINUSE_IDX ];
	mk_field( metp, metp, 0, 0, row+5, col+56, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[FILETBLFAIL_IDX];
	metp2 = &mettbl[FILETBLFAILR_IDX];
	mk_field( metp, metp2, 0, 0, row+6, col+56, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[FLCKTBLMAX_IDX];
	mk_field( metp, metp, 0, 0, row+7, col+56, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[FLCKTBLTOTAL_IDX];
	mk_field( metp, metp, 0, 0, row+8, col+56, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[FLCKTBLINUSE_IDX];
	mk_field( metp, metp, 0, 0, row+9, col+56, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[FLCKTBLFAIL_IDX];
	metp2 = &mettbl[FLCKTBLFAILR_IDX];
	mk_field( metp, metp2, 0, 0, row+10, col+56, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[FSWIO_IDX];
	mk_field( metp, metp, 0, 0, row+11, col+56, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[PHYSWIO_IDX];
	mk_field( metp, metp, 0, 0, row+12, col+56, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	index = (ncpu+1)*(nfstyp+1)-1;	/* total of all cpus and fstypes */
 	metp = &mettbl[IGET_IDX];
	mk_field( metp, metp, ncpu, nfstyp, row+11, col+2, 8, "%8.0f", metp->metval[index].cooked, metp->metval[index].cooked, NULL );
	metp = &mettbl[DIRBLK_IDX ];
	mk_field( metp, metp, ncpu, 0, row+12, col+2, 8, "%8.0f", metp->metval[index].cooked, metp->metval[index].cooked, NULL );

	metp = &mettbl[IPAGE_IDX];
	mk_field( metp, metp, ncpu, nfstyp, row+11, col+30, 8, "%8.0f", metp->metval[index].cooked, metp->metval[index].cooked, NULL );
	metp = &mettbl[INOPAGE_IDX];
	mk_field( metp, metp, ncpu, nfstyp, row+12, col+30, 8, "%8.0f", metp->metval[index].cooked, metp->metval[index].cooked, NULL );
}


/*
 *	function: 	print_ethtraffic
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the ethernet rate subscreen
 */
void
print_ethtraffic( int row, int col ) {	

	print_pereth_hdr( row++, col );

	rprint_metric(  &mettbl[ETH_InUcastPkts_IDX], &mettbl[ETH_InUcastPkts_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[ETH_OutUcastPkts_IDX], &mettbl[ETH_OutUcastPkts_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[ETH_InNUcastPkts_IDX], &mettbl[ETH_InNUcastPkts_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[ETH_OutNUcastPkts_IDX], &mettbl[ETH_OutNUcastPkts_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[ETH_InOctets_IDX], &mettbl[ETH_InOctets_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[ETH_OutOctets_IDX], &mettbl[ETH_OutOctets_IDX], row++, col, 1 );
	rprint_metric(  &mettbl[ETH_OutQlen_IDX], &mettbl[ETH_OutQlen_IDX], row++, col, 1 );
}


/*
 *	function: 	print_ethinerr
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the ethernet input errors subscreen
 */
void
print_ethinerr( int row, int col ) {

	print_pereth_hdr( row++, col );

	rprint_metric(  &mettbl[ETH_InErrors_IDX], &mettbl[ETHR_InErrors_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[ETH_etherAlignErrors_IDX], &mettbl[ETHR_etherAlignErrors_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[ETH_etherCRCerrors_IDX], &mettbl[ETHR_etherCRCerrors_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[ETH_etherOverrunErrors_IDX], &mettbl[ETHR_etherOverrunErrors_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[ETH_etherUnderrunErrors_IDX], &mettbl[ETHR_etherUnderrunErrors_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[ETH_etherMissedPkts_IDX], &mettbl[ETHR_etherMissedPkts_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[ETH_InDiscards_IDX], &mettbl[ETHR_InDiscards_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[ETH_etherReadqFull_IDX], &mettbl[ETHR_etherReadqFull_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[ETH_etherRcvResources_IDX], &mettbl[ETHR_etherRcvResources_IDX], row++, col, 0 );
}


/*
 *	function: 	print_ethouterr
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the ethernet output errors subscreen
 */
void
print_ethouterr( int row, int col ) {

	print_pereth_hdr( row++, col );

	rprint_metric(  &mettbl[ETH_etherCollisions_IDX], &mettbl[ETHR_etherCollisions_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[ETH_OutDiscards_IDX], &mettbl[ETHR_OutDiscards_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[ETH_OutErrors_IDX], &mettbl[ETHR_OutErrors_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[ETH_etherAbortErrors_IDX], &mettbl[ETHR_etherAbortErrors_IDX], row++, col, 0 );
	rprint_metric(  &mettbl[ETH_etherCarrierLost_IDX], &mettbl[ETHR_etherCarrierLost_IDX], row++, col, 0 );
}


/*
 *	function: 	print_eth
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the ethernet summary subscreen
 */
void
print_eth(int row, int col ) {
	struct metric *metp, *metp2;	

	if( need_header & METS ) {
		mk_field( NULL, NULL, 0, 0, row, col+2,  20, gettxt("RTPM:126", "     ETHERNET:      "), NULL, NULL, print_ethtraffic );
		mk_field( NULL, NULL, 0, 0, row, col+26, 22, gettxt("RTPM:127", "   INPUT ERRORS:      "), NULL, NULL, print_ethinerr );
		mk_field( NULL, NULL, 0, 0, row, col+53, 25, gettxt("RTPM:128", "  OUTPUT ERRORS:         "), NULL, NULL, print_ethouterr );
		set_label_color( 0 );
		move( row+1, col+11 );
		printw(gettxt("RTPM:129", "recv pkts/s             receive errs:              collisions"));
		move( row+2, col+11 );
		printw(gettxt("RTPM:130", "xmit pkts/s               frame align              out pkts dropped"));
		move( row+3, col+11 );
		printw(gettxt("RTPM:131", "rcv bcast/s               crc errors               xmit errors"));
		move( row+4, col+11 );
		printw(gettxt("RTPM:132", "xmt bcast/s               overruns                 interface aborts"));
		move( row+5, col+11 );
		printw(gettxt("RTPM:133", "rcv octet/s               underruns                carrier lost"));
		move( row+6, col+11 );
		printw(gettxt("RTPM:134", "xmt octet/s               missed pkts"));
		move( row+7, col+11 );
		printw(gettxt("RTPM:135", "xmit qlen               good pkts discard:"));
		move( row+8, col+11 );
		printw(gettxt("RTPM:136", "                          readq full"));
		move( row+9, col+11 );
		printw(gettxt("RTPM:137", "                          rcv resrc"));
	}
	metp = &mettbl[ETH_InUcastPkts_IDX];
	mk_field( metp, metp, nether, 0, row+1, col+2, 8, "%8.0f", metp->metval[nether].cooked, metp->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_OutUcastPkts_IDX];
	mk_field( metp, metp, nether, 0, row+2, col+2, 8, "%8.0f", metp->metval[nether].cooked, metp->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_InNUcastPkts_IDX];
	mk_field( metp, metp, nether, 0, row+3, col+2, 8, "%8.0f", metp->metval[nether].cooked, metp->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_OutNUcastPkts_IDX];
	mk_field( metp, metp, nether, 0, row+4, col+2, 8, "%8.0f", metp->metval[nether].cooked, metp->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_InOctets_IDX];
	mk_field( metp, metp, nether, 0, row+5, col+2, 8, "%8.0f", metp->metval[nether].cooked, metp->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_OutOctets_IDX];
	mk_field( metp, metp, nether, 0, row+6, col+2, 8, "%8.0f", metp->metval[nether].cooked, metp->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_OutQlen_IDX];
	mk_field( metp, metp, nether, 0, row+7, col+2, 8, "%8.0f", metp->metval[nether].cooked, metp->metval[nether].cooked, NULL );

	metp = &mettbl[ETH_InErrors_IDX];
	metp2 = &mettbl[ETHR_InErrors_IDX];
	mk_field( metp, metp2, nether, 0, row+1, col+26, 8, "%8.0f", metp->metval[nether].cooked, metp2->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_etherAlignErrors_IDX];
	metp2 = &mettbl[ETHR_etherAlignErrors_IDX];
	mk_field( metp, metp2, nether, 0, row+2, col+26, 8, "%8.0f", metp->metval[nether].cooked, metp2->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_etherCRCerrors_IDX];
	metp2 = &mettbl[ETHR_etherCRCerrors_IDX];
	mk_field( metp, metp2, nether, 0, row+3, col+26, 8, "%8.0f", metp->metval[nether].cooked, metp2->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_etherOverrunErrors_IDX];
	metp2 = &mettbl[ETHR_etherOverrunErrors_IDX];
	mk_field( metp, metp2, nether, 0, row+4, col+26, 8, "%8.0f", metp->metval[nether].cooked, metp2->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_etherUnderrunErrors_IDX];
	metp2 = &mettbl[ETHR_etherUnderrunErrors_IDX];
	mk_field( metp, metp2, nether, 0, row+5, col+26, 8, "%8.0f", metp->metval[nether].cooked, metp2->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_etherMissedPkts_IDX];
	metp2 = &mettbl[ETHR_etherMissedPkts_IDX];
	mk_field( metp, metp2, nether, 0, row+6, col+26, 8, "%8.0f", metp->metval[nether].cooked, metp2->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_InDiscards_IDX];
	metp2 = &mettbl[ETHR_InDiscards_IDX];
	mk_field( metp, metp2, nether, 0, row+7, col+26, 8, "%8.0f", metp->metval[nether].cooked, metp2->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_etherReadqFull_IDX];
	metp2 = &mettbl[ETHR_etherReadqFull_IDX];
	mk_field( metp, metp2, nether, 0, row+8, col+26, 8, "%8.0f", metp->metval[nether].cooked, metp2->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_etherRcvResources_IDX];
	metp2 = &mettbl[ETHR_etherRcvResources_IDX];
	mk_field( metp, metp2, nether, 0, row+9, col+26, 8, "%8.0f", metp->metval[nether].cooked, metp2->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_etherCollisions_IDX];
	metp2 = &mettbl[ETHR_etherCollisions_IDX];
	mk_field( metp, metp2, nether, 0, row+1, col+53, 8, "%8.0f", metp->metval[nether].cooked, metp2->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_OutDiscards_IDX];
	metp2 = &mettbl[ETHR_OutDiscards_IDX];
	mk_field( metp, metp2, nether, 0, row+2, col+53, 8, "%8.0f", metp->metval[nether].cooked, metp2->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_OutErrors_IDX];
	metp2 = &mettbl[ETHR_OutErrors_IDX];
	mk_field( metp, metp2, nether, 0, row+3, col+53, 8, "%8.0f", metp->metval[nether].cooked, metp2->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_etherAbortErrors_IDX];
	metp2 = &mettbl[ETHR_etherAbortErrors_IDX];
	mk_field( metp, metp2, nether, 0, row+4, col+53, 8, "%8.0f", metp->metval[nether].cooked, metp2->metval[nether].cooked, NULL );
	metp = &mettbl[ETH_etherCarrierLost_IDX];
	metp2 = &mettbl[ETHR_etherCarrierLost_IDX];
	mk_field( metp, metp2, nether, 0, row+5, col+53, 8, "%8.0f", metp->metval[nether].cooked, metp2->metval[nether].cooked, NULL );
}


/*
 *	function: 	print_ip
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the ip networking statistics subscreen
 */
void
print_ip(int row, int col ) {
	struct metric *metp;	
	struct metric *metp2;

	if( need_header & METS ) {

		set_label_color( 0 );
		move(row+1,col+9);

		printw(gettxt("RTPM:138", "packets received/s              out requests/s"));
		move(row+2,col+9);

		printw(gettxt("RTPM:139", "bad header crc                  packets delivered/s"));
		move(row+3,col+9);

		printw(gettxt("RTPM:140", "packet too short                no routes"));
		move(row+4,col+9);

		printw(gettxt("RTPM:141", "data size too small             redirects sent"));
		move(row+5,col+9);

		printw(gettxt("RTPM:142", "bad header length               packets forwarded"));
		move(row+6,col+9);

		printw(gettxt("RTPM:143", "bad data length                 packets not forwardable"));
		move(row+7,col+9);

		printw(gettxt("RTPM:144", "protocol unknown                fragmented packets"));
		move(row+8,col+9);

		printw(gettxt("RTPM:145", "fragments received              fragments created"));
		move(row+9,col+9);

		printw(gettxt("RTPM:146", "dropped fragments               fragment fails"));
		move(row+10,col+9);

		printw(gettxt("RTPM:147", "timed out fragments             input system errors"));
		move(row+11,col+9);

		printw(gettxt("RTPM:148", "packets reassembled             output system errors"));
	}
	metp = &mettbl[ IPR_total_IDX ] ;
	mk_field( metp, metp, 0, 0, row+1, col, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ IP_badsum_IDX ] ;
	metp2 = &mettbl[ IPR_badsum_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+2, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_tooshort_IDX ] ;
	metp2 = &mettbl[ IPR_tooshort_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+3, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_toosmall_IDX ] ;
	metp2 = &mettbl[ IPR_toosmall_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+4, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_badhlen_IDX ] ;
	metp2 = &mettbl[ IPR_badhlen_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+5, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_badlen_IDX ] ;
	metp2 = &mettbl[ IPR_badlen_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+6, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_unknownproto_IDX ] ;
	metp2 = &mettbl[ IPR_unknownproto_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+7, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_fragments_IDX ] ;
	metp2 = &mettbl[ IPR_fragments_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+8, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_fragdropped_IDX ] ;
	metp2 = &mettbl[ IPR_fragdropped_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+9, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_fragtimeout_IDX ] ;
	metp2 = &mettbl[ IPR_fragtimeout_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+10, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_reasms_IDX ] ;
	metp2 = &mettbl[ IPR_reasms_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+11, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ IPR_outrequests_IDX ] ;
	mk_field( metp, metp, 0, 0, row+1, col+32, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ IPR_indelivers_IDX ] ;
	mk_field( metp, metp, 0, 0, row+2, col+32, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ IP_noroutes_IDX ] ;
	metp2 = &mettbl[ IPR_noroutes_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+3, col+32, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_redirectsent_IDX ] ;
	metp2 = &mettbl[ IPR_redirectsent_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+4, col+32, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_forward_IDX ] ;
	metp2 = &mettbl[ IPR_forward_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+5, col+32, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_cantforward_IDX ] ;
	metp2 = &mettbl[ IPR_cantforward_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+6, col+32, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_pfrags_IDX ] ;
	metp2 = &mettbl[ IPR_pfrags_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+7, col+32, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_frags_IDX ] ;
	metp2 = &mettbl[ IPR_frags_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+8, col+32, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_fragfails_IDX ] ;
	metp2 = &mettbl[ IPR_fragfails_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+9, col+32, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_inerrors_IDX ] ;
	metp2 = &mettbl[ IPR_inerrors_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+10, col+32, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_outerrors_IDX ] ;
	metp2 = &mettbl[ IPR_outerrors_IDX ] ;
	mk_field( metp, metp2, 0, 0, row+11, col+32, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
}


/*
 *	function: 	print_tcp
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the tcp networking statistics subscreen
 */
void
print_tcp(int row, int col ) {
	struct metric *metp, *metp2;	

	if( need_header & METS ) {
		set_label_color( 0 );
		move(row,col+9);
		printw(gettxt("RTPM:149", "snd pkt/s           rcv pkt/s           aftwinpkt           estabrsets"));
		move(row+1,col+9);
		printw(gettxt("RTPM:150", "datpkt/s            pkts ak/s           aftwinbyt           rttupdat/s"));
		move(row+2,col+9);
		printw(gettxt("RTPM:151", "databyt/s           byts ak/s           winprobes           segstimd/s"));
		move(row+3,col+9);
		printw(gettxt("RTPM:152", "pkt rexmt           dup acks            winupdats           rexmtimeot"));
		move(row+4,col+9);
		printw(gettxt("RTPM:153", "byt rexmt           aktoomuch           badcksums           timeodrops"));
		move(row+5,col+9);
		printw(gettxt("RTPM:154", "ack onlys           inseqpk/s           badhdroff           persisttio"));
		move(row+6,col+9);
		printw(gettxt("RTPM:155", "delay aks           inseqby/s           pktooshrt           keeptimeot"));
		move(row+7,col+9);
		printw(gettxt("RTPM:156", "URG pkts            dup pkts            connreqs            keepprobes"));
		move(row+8,col+9);
		printw(gettxt("RTPM:157", "probes              dup bytes           conacepts           keepdrops"));
		move(row+9,col+9);
		printw(gettxt("RTPM:158", "win updts           prtdupkt            connects            lingered"));
		move(row+10,col+9);
		printw(gettxt("RTPM:159", "cntl pkts           prtdupyt            connclsed           lngrtimeot"));
		move(row+11,col+9);
		printw(gettxt("RTPM:160", "resets              outordpkt           dropped             lingrcancl"));
		move(row+12,col+9);
		printw(gettxt("RTPM:161", "inpterrs            outordbyt           conndrops           lingrabort"));
		move(row+13,col+9);
		printw(gettxt("RTPM:162", "                    aftclspkt           atmptfail"));
	}
	metp = &mettbl[ TCPR_sndtotal_IDX ];
	mk_field( metp, metp, 0, 0, row+0, col+0, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ TCPR_sndpack_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col+0, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ TCPR_sndbyte_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col+0, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ TCP_sndrexmitpack_IDX ];
	metp2 = &mettbl[ TCPR_sndrexmitpack_IDX ];
	mk_field( metp, metp2, 0, 0, row+3, col+0, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_sndrexmitbyte_IDX ];
	metp2 = &mettbl[ TCPR_sndrexmitbyte_IDX ];
	mk_field( metp, metp2, 0, 0, row+4, col+0, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_sndacks_IDX ];
	metp2 = &mettbl[ TCPR_sndacks_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col+0, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_delack_IDX ];
	metp2 = &mettbl[ TCPR_delack_IDX ];
	mk_field( metp, metp2, 0, 0, row+6, col+0, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_sndurg_IDX ];
	metp2 = &mettbl[ TCPR_sndurg_IDX ];
	mk_field( metp, metp2, 0, 0, row+7, col+0, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_sndprobe_IDX ];
	metp2 = &mettbl[ TCPR_sndprobe_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col+0, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_sndwinup_IDX ];
	metp2 = &mettbl[ TCPR_sndwinup_IDX ];
	mk_field( metp, metp2, 0, 0, row+9, col+0, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_sndctrl_IDX ];
	metp2 = &mettbl[ TCPR_sndctrl_IDX ];
	mk_field( metp, metp2, 0, 0, row+10, col+0, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_sndrsts_IDX ];
	metp2 = &mettbl[ TCPR_sndrsts_IDX ];
	mk_field( metp, metp2, 0, 0, row+11, col+0, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_inerrors_IDX ];
	metp2 = &mettbl[ TCPR_inerrors_IDX ];
	mk_field( metp, metp2, 0, 0, row+12, col+0, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ TCPR_rcvtotal_IDX ];
	mk_field( metp, metp, 0, 0, row+0, col+20, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ TCPR_rcvackpack_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col+20, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ TCPR_rcvackbyte_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col+20, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ TCP_rcvdupack_IDX ];
	metp2 = &mettbl[ TCPR_rcvdupack_IDX ];
	mk_field( metp, metp2, 0, 0, row+3, col+20, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_rcvacktoomuch_IDX ];
	metp2 = &mettbl[ TCPR_rcvacktoomuch_IDX ];
	mk_field( metp, metp2, 0, 0, row+4, col+20, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCPR_rcvpack_IDX ];
	mk_field( metp, metp, 0, 0, row+5, col+20, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ TCPR_rcvbyte_IDX ];
	mk_field( metp, metp, 0, 0, row+6, col+20, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ TCP_rcvduppack_IDX ];
	metp2 = &mettbl[ TCPR_rcvduppack_IDX ];
	mk_field( metp, metp2, 0, 0, row+7, col+20, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_rcvdupbyte_IDX ];
	metp2 = &mettbl[ TCPR_rcvdupbyte_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col+20, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_rcvpartduppack_IDX ];
	metp2 = &mettbl[ TCPR_rcvpartduppack_IDX ];
	mk_field( metp, metp2, 0, 0, row+9, col+20, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_rcvpartdupbyte_IDX ];
	metp2 = &mettbl[ TCPR_rcvpartdupbyte_IDX ];
	mk_field( metp, metp2, 0, 0, row+10, col+20, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_rcvoopack_IDX ];
	metp2 = &mettbl[ TCPR_rcvoopack_IDX ];
	mk_field( metp, metp2, 0, 0, row+11, col+20, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_rcvoobyte_IDX ];
	metp2 = &mettbl[ TCPR_rcvoobyte_IDX ];
	mk_field( metp, metp2, 0, 0, row+12, col+20, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_rcvafterclose_IDX ];
	metp2 = &mettbl[ TCPR_rcvafterclose_IDX ];
	mk_field( metp, metp2, 0, 0, row+13, col+20, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ TCP_rcvpackafterwin_IDX ];
	metp2 = &mettbl[ TCPR_rcvpackafterwin_IDX ];
	mk_field( metp, metp2, 0, 0, row+0, col+40, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_rcvbyteafterwin_IDX ];
	metp2 = &mettbl[ TCPR_rcvbyteafterwin_IDX ];
	mk_field( metp, metp2, 0, 0, row+1, col+40, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_rcvwinprobe_IDX ];
	metp2 = &mettbl[ TCPR_rcvwinprobe_IDX ];
	mk_field( metp, metp2, 0, 0, row+2, col+40, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_rcvwinupd_IDX ];
	metp2 = &mettbl[ TCPR_rcvwinupd_IDX ];
	mk_field( metp, metp2, 0, 0, row+3, col+40, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_rcvbadsum_IDX ];
	metp2 = &mettbl[ TCPR_rcvbadsum_IDX ];
	mk_field( metp, metp2, 0, 0, row+4, col+40, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_rcvbadoff_IDX ];
	metp2 = &mettbl[ TCPR_rcvbadoff_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col+40, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_rcvshort_IDX ];
	metp2 = &mettbl[ TCPR_rcvshort_IDX ];
	mk_field( metp, metp2, 0, 0, row+6, col+40, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_connattempt_IDX ];
	metp2 = &mettbl[ TCPR_connattempt_IDX ];
	mk_field( metp, metp2, 0, 0, row+7, col+40, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_accepts_IDX ];
	metp2 = &mettbl[ TCPR_accepts_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col+40, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_connects_IDX ];
	metp2 = &mettbl[ TCPR_connects_IDX ];
	mk_field( metp, metp2, 0, 0, row+9, col+40, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_closed_IDX ];
	metp2 = &mettbl[ TCPR_closed_IDX ];
	mk_field( metp, metp2, 0, 0, row+10, col+40, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_drops_IDX ];
	metp2 = &mettbl[ TCPR_drops_IDX ];
	mk_field( metp, metp2, 0, 0, row+11, col+40, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_conndrops_IDX ];
	metp2 = &mettbl[ TCPR_conndrops_IDX ];
	mk_field( metp, metp2, 0, 0, row+12, col+40, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_attemptfails_IDX ];
	metp2 = &mettbl[ TCPR_attemptfails_IDX ];
	mk_field( metp, metp2, 0, 0, row+13, col+40, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ TCP_estabresets_IDX ];
	metp2 = &mettbl[ TCPR_estabresets_IDX ];
	mk_field( metp, metp2, 0, 0, row+0, col+60, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCPR_rttupdated_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col+60, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ TCPR_segstimed_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col+60, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ TCP_rexmttimeo_IDX ];
	metp2 = &mettbl[ TCPR_rexmttimeo_IDX ];
	mk_field( metp, metp2, 0, 0, row+3, col+60, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_timeoutdrop_IDX ];
	metp2 = &mettbl[ TCPR_timeoutdrop_IDX ];
	mk_field( metp, metp2, 0, 0, row+4, col+60, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_persisttimeo_IDX ];
	metp2 = &mettbl[ TCPR_persisttimeo_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col+60, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_keeptimeo_IDX ];
	metp2 = &mettbl[ TCPR_keeptimeo_IDX ];
	mk_field( metp, metp2, 0, 0, row+6, col+60, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_keepprobe_IDX ];
	metp2 = &mettbl[ TCPR_keepprobe_IDX ];
	mk_field( metp, metp2, 0, 0, row+7, col+60, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_keepdrops_IDX ];
	metp2 = &mettbl[ TCPR_keepdrops_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col+60, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_linger_IDX ];
	metp2 = &mettbl[ TCPR_linger_IDX ];
	mk_field( metp, metp2, 0, 0, row+9, col+60, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_lingerexp_IDX ];
	metp2 = &mettbl[ TCPR_lingerexp_IDX ];
	mk_field( metp, metp2, 0, 0, row+10, col+60, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_lingercan_IDX ];
	metp2 = &mettbl[ TCPR_lingercan_IDX ];
	mk_field( metp, metp2, 0, 0, row+11, col+60, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_lingerabort_IDX ];
	metp2 = &mettbl[ TCPR_lingerabort_IDX ];
	mk_field( metp, metp2, 0, 0, row+12, col+60, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
}


/*
 *	function: 	print_icmp
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the icmp networking statistics subscreen
 */
void
print_icmp( int row, int col ) {
	struct metric *metp;	
	struct metric *metp2;

	if( need_header & METS ) {
		set_label_color( 0 );
		move(row,col+9);
		printw(gettxt("RTPM:163", "                                                 in         out"));
		move(row+1,col+9);
		printw(gettxt("RTPM:164", "msgs sent/s             echo reply"));
		move(row+2,col+9);
		printw(gettxt("RTPM:165", "msgs rcvd/s             dest unreachable"));
		move(row+3,col+9);
		printw(gettxt("RTPM:166", "msg resps/s             source quench"));
		move(row+4,col+9);
		printw(gettxt("RTPM:167", "icmp errors             routing redirects"));
		move(row+5,col+9);
		printw(gettxt("RTPM:168", "old icmp msg            echo"));
		move(row+6,col+9);
		printw(gettxt("RTPM:169", "bad code fld            time exceeded"));
		move(row+7,col+9);
		printw(gettxt("RTPM:170", "msg too shrt            parameter problems"));
		move(row+8,col+9);
		printw(gettxt("RTPM:171", "bad chksums             time stamp"));
		move(row+9,col+9);
		printw(gettxt("RTPM:172", "bad length              time stamp reply"));
		move(row+10,col+9);
		printw(gettxt("RTPM:173", "outpt syserr            info request"));
		move(row+11,col+9);
		printw(gettxt("RTPM:174", "                        info reply"));
		move(row+12,col+9);
		printw(gettxt("RTPM:175", "                        addr mask req"));
		move(row+13,col+9);
		printw(gettxt("RTPM:176", "                        addr mask reply"));
	}
	metp = &mettbl[ ICMPR_outtotal_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ ICMPR_intotal_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ ICMPR_reflect_IDX ];
	mk_field( metp, metp, 0, 0, row+3, col, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ ICMP_error_IDX ];
	metp2 = &mettbl[ ICMPR_error_IDX ];
	mk_field( metp, metp2, 0, 0, row+4, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_oldicmp_IDX ];
	metp2 = &mettbl[ ICMPR_oldicmp_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_badcode_IDX ];
	metp2 = &mettbl[ ICMPR_badcode_IDX ];
	mk_field( metp, metp2, 0, 0, row+6, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_tooshort_IDX ];
	metp2 = &mettbl[ ICMPR_tooshort_IDX ];
	mk_field( metp, metp2, 0, 0, row+7, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_checksum_IDX ];
	metp2 = &mettbl[ ICMPR_checksum_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_badlen_IDX ];
	metp2 = &mettbl[ ICMPR_badlen_IDX ];
	mk_field( metp, metp2, 0, 0, row+9, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_outerrors_IDX ];
	metp2 = &mettbl[ ICMPR_outerrors_IDX ];
	mk_field( metp, metp2, 0, 0, row+10, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ ICMP_inhist0_IDX ];
	metp2 = &mettbl[ ICMPR_inhist0_IDX ];
	mk_field( metp, metp2, 0, 0, row+1, col+52, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_inhist3_IDX ];
	metp2 = &mettbl[ ICMPR_inhist3_IDX ];
	mk_field( metp, metp2, 0, 0, row+2, col+52, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_inhist4_IDX ];
	metp2 = &mettbl[ ICMPR_inhist4_IDX ];
	mk_field( metp, metp2, 0, 0, row+3, col+52, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_inhist5_IDX ];
	metp2 = &mettbl[ ICMPR_inhist5_IDX ];
	mk_field( metp, metp2, 0, 0, row+4, col+52, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_inhist8_IDX ];
	metp2 = &mettbl[ ICMPR_inhist8_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col+52, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_inhist11_IDX ];
	metp2 = &mettbl[ ICMPR_inhist11_IDX ];
	mk_field( metp, metp2, 0, 0, row+6, col+52, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_inhist12_IDX ];
	metp2 = &mettbl[ ICMPR_inhist12_IDX ];
	mk_field( metp, metp2, 0, 0, row+7, col+52, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_inhist13_IDX ];
	metp2 = &mettbl[ ICMPR_inhist13_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col+52, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_inhist14_IDX ];
	metp2 = &mettbl[ ICMPR_inhist14_IDX ];
	mk_field( metp, metp2, 0, 0, row+9, col+52, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_inhist15_IDX ];
	metp2 = &mettbl[ ICMPR_inhist15_IDX ];
	mk_field( metp, metp2, 0, 0, row+10, col+52, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_inhist16_IDX ];
	metp2 = &mettbl[ ICMPR_inhist16_IDX ];
	mk_field( metp, metp2, 0, 0, row+11, col+52, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_inhist17_IDX ];
	metp2 = &mettbl[ ICMPR_inhist17_IDX ];
	mk_field( metp, metp2, 0, 0, row+12, col+52, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_inhist18_IDX ];
	metp2 = &mettbl[ ICMPR_inhist18_IDX ];
	mk_field( metp, metp2, 0, 0, row+13, col+52, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ ICMP_outhist0_IDX ];
	metp2 = &mettbl[ ICMPR_outhist0_IDX ];
	mk_field( metp, metp2, 0, 0, row+1, col+64, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_outhist3_IDX ];
	metp2 = &mettbl[ ICMPR_outhist3_IDX ];
	mk_field( metp, metp2, 0, 0, row+2, col+64, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_outhist4_IDX ];
	metp2 = &mettbl[ ICMPR_outhist4_IDX ];
	mk_field( metp, metp2, 0, 0, row+3, col+64, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_outhist5_IDX ];
	metp2 = &mettbl[ ICMPR_outhist5_IDX ];
	mk_field( metp, metp2, 0, 0, row+4, col+64, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_outhist8_IDX ];
	metp2 = &mettbl[ ICMPR_outhist8_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col+64, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_outhist11_IDX ];
	metp2 = &mettbl[ ICMPR_outhist11_IDX ];
	mk_field( metp, metp2, 0, 0, row+6, col+64, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_outhist12_IDX ];
	metp2 = &mettbl[ ICMPR_outhist12_IDX ];
	mk_field( metp, metp2, 0, 0, row+7, col+64, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_outhist13_IDX ];
	metp2 = &mettbl[ ICMPR_outhist13_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col+64, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_outhist14_IDX ];
	metp2 = &mettbl[ ICMPR_outhist14_IDX ];
	mk_field( metp, metp2, 0, 0, row+9, col+64, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_outhist15_IDX ];
	metp2 = &mettbl[ ICMPR_outhist15_IDX ];
	mk_field( metp, metp2, 0, 0, row+10, col+64, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_outhist16_IDX ];
	metp2 = &mettbl[ ICMPR_outhist16_IDX ];
	mk_field( metp, metp2, 0, 0, row+11, col+64, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_outhist17_IDX ];
	metp2 = &mettbl[ ICMPR_outhist17_IDX ];
	mk_field( metp, metp2, 0, 0, row+12, col+64, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_outhist18_IDX ];
	metp2 = &mettbl[ ICMPR_outhist18_IDX ];
	mk_field( metp, metp2, 0, 0, row+13, col+64, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
}


/*
 *	function: 	print_net
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the tcp/ip networking summary subscreen
 */
void
print_net(int row, int col ) {
	struct metric *metp, *metp2;	

	if( need_header & METS ) {
		set_label_color( 0 );
		move( row, col+8 );
		printw(gettxt("RTPM:177", "                                                            udp:"));
		move( row+1, col+8 );
		printw(gettxt("RTPM:178", "msgs sent/s          snd pkt/s         rcv pkts/s           send pkts/s"));
		move( row+2, col+8 );
		printw(gettxt("RTPM:179", "msgs rcvd/s          datapkt/s         rcv frag/s           pkt delvr/s"));
		move( row+3, col+8 );
		printw(gettxt("RTPM:180", "msg resps/s          databyt/s         pkt reasmbls         hdr drops"));
		move( row+4, col+8 );
		printw(gettxt("RTPM:181", "icmp errors          pkt rexmt         pkt forwards         bad datalen"));
		move( row+5, col+8 );
		printw(gettxt("RTPM:182", "old icmp msg         byt rexmt         redirect snt         bad hdr crc"));
		move( row+6, col+8 );
		printw(gettxt("RTPM:183", "bad code fld         connects          pkt delvr/s          full socket"));
		move( row+7, col+8 );
		printw(gettxt("RTPM:184", "msg too shrt         rcvpkt/s          out req/s            no ports"));
		move( row+8, col+8 );
		printw(gettxt("RTPM:185", "bad chksums          pkts ak/s         fragged pkts         inpt syserr"));
		move( row+9, col+8 );
		printw(gettxt("RTPM:186", "bad length           byts ak/s         fragments"));
		move( row+10, col+8 );
		printw(gettxt("RTPM:187", "outpt syserr         inpterrs          input syserr"));
		move( row+11, col+8 );
		printw(gettxt("RTPM:188", "                                       outpt syserr"));
/*
 *		These have to come after the above labels, otherwise
 *		the label for "udp:" clears them off of the screen
 */
		mk_field( NULL, NULL, 0, 0, row, col,  20, gettxt("RTPM:189", "        ICMP:       "), NULL, NULL, print_icmp );
		mk_field( NULL, NULL, 0, 0, row, col+21, 17, gettxt("RTPM:190", "       TCP:      "), NULL, NULL, print_tcp );
		mk_field( NULL, NULL, 0, 0, row, col+39, 20, gettxt("RTPM:191", "         IP:        "), NULL, NULL, print_ip );
	}
	metp = &mettbl[ ICMPR_outtotal_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ ICMPR_intotal_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ ICMPR_reflect_IDX ];
	mk_field( metp, metp, 0, 0, row+3, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ ICMP_error_IDX ];
	metp2 = &mettbl[ ICMPR_error_IDX ];
	mk_field( metp, metp2, 0, 0, row+4, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_oldicmp_IDX ];
	metp2 = &mettbl[ ICMPR_oldicmp_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_badcode_IDX ];
	metp2 = &mettbl[ ICMPR_badcode_IDX ];
	mk_field( metp, metp2, 0, 0, row+6, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_tooshort_IDX ];
	metp2 = &mettbl[ ICMPR_tooshort_IDX ];
	mk_field( metp, metp2, 0, 0, row+7, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_checksum_IDX ];
	metp2 = &mettbl[ ICMPR_checksum_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_badlen_IDX ];
	metp2 = &mettbl[ ICMPR_badlen_IDX ];
	mk_field( metp, metp2, 0, 0, row+9, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ ICMP_outerrors_IDX ];
	metp2 = &mettbl[ ICMPR_outerrors_IDX ];
	mk_field( metp, metp2, 0, 0, row+10, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ UDPR_outtotal_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ UDPR_indelivers_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ UDP_hdrops_IDX ];
	metp2 = &mettbl[ UDPR_hdrops_IDX ];
	mk_field( metp, metp2, 0, 0, row+3, col+60, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ UDP_badlen_IDX ];
	metp2 = &mettbl[ UDPR_badlen_IDX ];
	mk_field( metp, metp2, 0, 0, row+4, col+60, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ UDP_badsum_IDX ];
	metp2 = &mettbl[ UDPR_badsum_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col+60, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ UDP_fullsock_IDX ];
	metp2 = &mettbl[ UDPR_fullsock_IDX ];
	mk_field( metp, metp2, 0, 0, row+6, col+60, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ UDP_noports_IDX ];
	metp2 = &mettbl[ UDPR_noports_IDX ];
	mk_field( metp, metp2, 0, 0, row+7, col+60, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ UDP_inerrors_IDX ];
	metp2 = &mettbl[ UDPR_inerrors_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col+60, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ TCPR_sndtotal_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col+21, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ TCPR_sndpack_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col+21, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ TCPR_sndbyte_IDX ];
	mk_field( metp, metp, 0, 0, row+3, col+21, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ TCP_sndrexmitpack_IDX ];
	metp2 = &mettbl[ TCPR_sndrexmitpack_IDX ];
	mk_field( metp, metp2, 0, 0, row+4, col+21, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_sndrexmitbyte_IDX ];
	metp2 = &mettbl[ TCPR_sndrexmitbyte_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col+21, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCP_connects_IDX ];
	metp2 = &mettbl[ TCPR_connects_IDX ];
	mk_field( metp, metp2, 0, 0, row+6, col+21, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ TCPR_rcvtotal_IDX ];
	mk_field( metp, metp, 0, 0, row+7, col+21, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ TCPR_rcvackpack_IDX ];
	mk_field( metp, metp, 0, 0, row+8, col+21, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ TCPR_rcvackbyte_IDX ];
	mk_field( metp, metp, 0, 0, row+9, col+21, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ TCP_inerrors_IDX ];
	metp2 = &mettbl[ TCPR_inerrors_IDX ];
	mk_field( metp, metp2, 0, 0, row+10, col+21, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ IPR_total_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col+39, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ IPR_fragments_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col+39, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ IP_reasms_IDX ];
	metp2 = &mettbl[ IPR_reasms_IDX ];
	mk_field( metp, metp2, 0, 0, row+3, col+39, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_forward_IDX ];
	metp2 = &mettbl[ IPR_forward_IDX ];
	mk_field( metp, metp2, 0, 0, row+4, col+39, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_redirectsent_IDX ];
	metp2 = &mettbl[ IPR_redirectsent_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col+39, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IPR_indelivers_IDX ];
	mk_field( metp, metp, 0, 0, row+6, col+39, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ IPR_outrequests_IDX ];
	mk_field( metp, metp, 0, 0, row+7, col+39, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ IP_pfrags_IDX ];
	metp2 = &mettbl[ IPR_pfrags_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col+39, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_frags_IDX ];
	metp2 = &mettbl[ IPR_frags_IDX ];
	mk_field( metp, metp2, 0, 0, row+9, col+39, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_inerrors_IDX ];
	metp2 = &mettbl[ IPR_inerrors_IDX ];
	mk_field( metp, metp2, 0, 0, row+10, col+39, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ IP_outerrors_IDX ];
	metp2 = &mettbl[ IPR_outerrors_IDX ];
	mk_field( metp, metp2, 0, 0, row+11, col+39, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

}

/*
 *	function: 	print_ipx_socket
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the ipx socket metrics
 */
void
print_ipx_sock( int row, int col ) {
	struct metric *metp, *metp2;	

	if( need_header & METS ) {
		set_label_color( 0 );
		move( row, col+8 );
		printw(gettxt("RTPM:1099", "tot rcv datapkt/s          TLI out lan rtr/s          invalid chksums"));
		move( row+1, col + 8 );
		printw(gettxt("RTPM:1100", "pkt rcv by ISM/s           sockets bound              busy sockets"));
		move( row+2, col + 8 );
		printw(gettxt("RTPM:1101", "out to lan routr/s         non-TLI bind req/s         socket not bound"));
		move( row+3, col + 8 );
		printw(gettxt("RTPM:1102", "IPX routed/s               TLI bindsock req/s         IPX routed TLI/s"));
		move( row+4, col + 8 );
		printw(gettxt("RTPM:1103", "out nonTLI datpk/s         TLI opt mgmt req/s         TLI hdr allocfail"));
		move( row+5, col + 8 );
		printw(gettxt("RTPM:1104", "out badlen < hdrsz         TLI unknown reqs           snt to TLI sckt/s"));
		move( row+6, col + 8 );
		printw(gettxt("RTPM:1105", "out TLI datapkt/s          invalid src socket         total ioctl/s"));
		move( row+7, col + 8 );
		printw(gettxt("RTPM:1106", "  socket not bound         padding alloc/s              setwater/s"));
		move( row+8, col + 8 );
		printw(gettxt("RTPM:1107", "  bad addr size            pad blk allocfails           bindsocket/s"));
		move( row+9, col + 8 );
		printw(gettxt("RTPM:1108", "  bad data size            even pad/s                   unbindsocket/s"));
		move( row+10, col + 8 );
		printw(gettxt("RTPM:1109", "  bad option size          chksum gen/s                 stats/s"));
		move( row+11, col + 8 );
		printw(gettxt("RTPM:1110", "  hdr alloc fails          chksum fails                 unknown"));
		move( row+12, col + 8 );
		printw(gettxt("RTPM:1111", "datapkt to sockt/s         data sz trim pkt/s"));
		move( row+13, col + 8 );
		printw(gettxt("RTPM:1112", "                             len < ipx hdr sz"));
	}

	/* data pkt/s */
	/* Total IPX data packets received from the stream head */
	metp  = &mettbl[ IPXR_datapackets_IDX ];
	mk_field( metp, metp, 0, 0, row, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* snt mux/s */
	/* Sent to the socket multiplexor */
	metp  = &mettbl[ IPXSOCKR_IpxInData_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* out to switch/s */
	/* Sent to IPX switch */
	metp  = &mettbl[ IPXSOCKR_IpxOutToSwitch_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* IPX routed/s */
	/* Valid socket found */
	metp  = &mettbl[ IPXSOCKR_IpxRouted_IDX ];
	mk_field( metp, metp, 0, 0, row+3, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* non-TLI/s */
	/* Non TLI IPX data packets */
	metp  = &mettbl[ IPXSOCKR_IpxOutData_IDX ];
	mk_field( metp, metp, 0, 0, row+4, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* bad sz */
	/* Length less than IPX header size, dropped */
	metp  = &mettbl[ IPXSOCK_IpxOutBadSize_IDX ];
	metp2 = &mettbl[ IPXSOCKR_IpxOutBadSize_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* TLI out data pkt/s */
	/* TLI IPX data packets */
	metp  = &mettbl[ IPXSOCKR_IpxTLIOutData_IDX ];
	mk_field( metp, metp, 0, 0, row+6, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* out bad state pkts */
	/* Bad TLI state, packet dropped */
	metp  = &mettbl[ IPXSOCK_IpxTLIOutBadState_IDX ];
	metp2 = &mettbl[ IPXSOCKR_IpxTLIOutBadState_IDX ];
	mk_field( metp, metp2, 0, 0, row+7, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* bad addr sz */
	/* Bad IPX address size, packet dropped */
	metp  = &mettbl[ IPXSOCK_IpxTLIOutBadAddr_IDX ];
	metp2 = &mettbl[ IPXSOCKR_IpxTLIOutBadAddr_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* bad data sz */
	/* Bad IPX data size, packet dropped */
	metp  = &mettbl[ IPXSOCK_IpxTLIOutBadSize_IDX ];
	metp2 = &mettbl[ IPXSOCKR_IpxTLIOutBadSize_IDX ];
	mk_field( metp, metp2, 0, 0, row+9, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* bad opt sz */
	/* Bad TLI option size, packet dropped */
	metp  = &mettbl[ IPXSOCK_IpxTLIOutBadOpt_IDX ];
	metp2 = &mettbl[ IPXSOCKR_IpxTLIOutBadOpt_IDX ];
	mk_field( metp, metp2, 0, 0, row+10, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* hdr alloc fails */
	/* Allocation of IPX header failed, packet dropped */
	metp  = &mettbl[ IPXSOCK_IpxTLIOutHdrAlloc_IDX ];
	metp2 = &mettbl[ IPXSOCKR_IpxTLIOutHdrAlloc_IDX ];
	mk_field( metp, metp2, 0, 0, row+11, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* data pkt rcv by sockt mux/s */
	/* Total data packets received by the socket multiplexor */
	metp  = &mettbl[ IPXSOCKR_IpxDataToSocket_IDX ];
	mk_field( metp, metp, 0, 0, row+12, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* TLI out to switch/s */
	/* Sent to IPX switch */
	metp  = &mettbl[ IPXSOCKR_IpxTLIOutToSwitch_IDX ];
	mk_field( metp, metp, 0, 0, row, col+27, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* bound sockts */
	/* Sockets Bound */
	metp  = &mettbl[ IPXSOCK_IpxBoundSockets_IDX ];
	metp2 = &mettbl[ IPXSOCKR_IpxBoundSockets_IDX ];
	mk_field( metp, metp2, 0, 0, row+1, col+27, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* non-TLI bind sock req/s */
	/* Non TLI Bind Socket Requests */
	metp  = &mettbl[ IPXSOCKR_IpxBind_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col+27, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* TLI bind sock req/s */
	/* TLI Bind Socket Requests */
	metp  = &mettbl[ IPXSOCKR_IpxTLIBind_IDX ];
	mk_field( metp, metp, 0, 0, row+3, col+27, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* TLI opt mgmt req/s */
	/* TLI Option Management Requests */
	metp  = &mettbl[ IPXSOCKR_IpxTLIOptMgt_IDX ];
	mk_field( metp, metp, 0, 0, row+4, col+27, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* TLI unk reqs */
	/* TLI Unknown Requests */
	metp  = &mettbl[ IPXSOCK_IpxTLIUnknown_IDX ];
	metp2 = &mettbl[ IPXSOCKR_IpxTLIUnknown_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col+27, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* Invalid sockt */
	/* BIND_SOCKET user sent packet with zero socket, packet dropped */
	metp  = &mettbl[ IPXSOCK_IpxSwitchInvalSocket_IDX ];
	metp2 = &mettbl[ IPXSOCKR_IpxSwitchInvalSocket_IDX ];
	mk_field( metp, metp2, 0, 0, row+6, col+27, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* pad alloc/s */
	/* padded, buffer alloc required */
	metp  = &mettbl[ IPXSOCKR_IpxSwitchEvenAlloc_IDX ];
	mk_field( metp, metp, 0, 0, row+7, col+27, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* padding blk alloc fails */
	/* dropped, could not allocate block for padding */
	metp  = &mettbl[ IPXSOCK_IpxSwitchAllocFail_IDX ];
	metp2 = &mettbl[ IPXSOCKR_IpxSwitchAllocFail_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col+27, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* even pad/s */
	/* padded to an even number of bytes */
	metp  = &mettbl[ IPXSOCKR_IpxSwitchEven_IDX ];
	mk_field( metp, metp, 0, 0, row+9, col+27, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* chksum gen/s */
	/* checksum generated */
	metp  = &mettbl[ IPXSOCKR_IpxSwitchSum_IDX ];
	mk_field( metp, metp, 0, 0, row+10, col+27, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* chksum fails */
	/* Failure to generate checksum, packet dropped */
	metp  = &mettbl[ IPXSOCK_IpxSwitchSumFail_IDX ];
	metp2 = &mettbl[ IPXSOCKR_IpxSwitchSumFail_IDX ];
	mk_field( metp, metp2, 0, 0, row+11, col+27, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* trim pkt/s */
	/* Data size trimmed to match IPX data size */
	metp  = &mettbl[ IPXSOCKR_IpxTrimPacket_IDX ];
	mk_field( metp, metp, 0, 0, row+12, col+27, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* in bad sz */
	/* pkt len < ipx hdr size, dropped */
	metp  = &mettbl[ IPXSOCK_IpxInBadSize_IDX ];
	metp2 = &mettbl[ IPXSOCKR_IpxInBadSize_IDX ];
	mk_field( metp, metp2, 0, 0, row+13, col+27, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* invalid chksum */
	/* Ipx checksum invalid, drop packet */
	metp  = &mettbl[ IPXSOCK_IpxSumFail_IDX ];
	metp2 = &mettbl[ IPXSOCKR_IpxSumFail_IDX ];
	mk_field( metp, metp2, 0, 0, row, col+54, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* busy sockts */
	/* Packets dropped because upper stream full */
	metp  = &mettbl[ IPXSOCK_IpxBusySocket_IDX ];
	metp2 = &mettbl[ IPXSOCKR_IpxBusySocket_IDX ];
	mk_field( metp, metp2, 0, 0, row+1, col+54, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* sockt not bound */
	/* Packets dropped, socket not bound */
	metp  = &mettbl[ IPXSOCK_IpxSocketNotBound_IDX ];
	metp2 = &mettbl[ IPXSOCKR_IpxSocketNotBound_IDX ];
	mk_field( metp, metp2, 0, 0, row+2, col+54, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* rte to TLI/s */
	/* Destined for TLI socket */
	metp  = &mettbl[ IPXSOCKR_IpxRoutedTLI_IDX ];
	mk_field( metp, metp, 0, 0, row+3, col+54, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* tli hdr alloc fails */
	/* Alloc of TLI header failed, packet dropped */
	metp  = &mettbl[ IPXSOCK_IpxRoutedTLIAlloc_IDX ];
	metp2 = &mettbl[ IPXSOCKR_IpxRoutedTLIAlloc_IDX ];
	mk_field( metp, metp2, 0, 0, row+4, col+54, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* snt to TLI sockt/s */
	/* Sent to TLI socket ( IpxRoutedTLI - IpxRoutedTLIAlloc ) */
	metp  = &mettbl[ IPXR_sent_to_tli_IDX ];
	mk_field( metp, metp, 0, 0, row+5, col+54, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* ioctl/s */
	/* Total Ioctls processed */
	metp  = &mettbl[ IPXR_total_ioctls_IDX ];
	mk_field( metp, metp, 0, 0, row+6, col+54, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* setwater/s */
	/* IOCTL requests SET_WATER */
	metp  = &mettbl[ IPXSOCKR_IpxIoctlSetWater_IDX ];
	mk_field( metp, metp, 0, 0, row+7, col+54, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* bindsock/s */
	/* IOCTL requests SET_SOCKET or BIND_SOCKET */
	metp  = &mettbl[ IPXSOCKR_IpxIoctlBindSocket_IDX ];
	mk_field( metp, metp, 0, 0, row+8, col+54, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* ubndsock/s */
	/* IOCTL requests UNBIND_SOCKET */
	metp  = &mettbl[ IPXSOCKR_IpxIoctlUnbindSocket_IDX ];
	mk_field( metp, metp, 0, 0, row+9, col+54, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* stats/s */
	/* IOCTL requests STATS */
	metp  = &mettbl[ IPXSOCKR_IpxIoctlStats_IDX ];
	mk_field( metp, metp, 0, 0, row+10, col+54, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* unknown */
	/* IOCTL requests Unknown, sent to lan router */
	metp  = &mettbl[ IPXSOCK_IpxIoctlUnknown_IDX ];
	metp2 = &mettbl[ IPXSOCKR_IpxIoctlUnknown_IDX ];
	mk_field( metp, metp2, 0, 0, row+11, col+54, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
}

/*
 *	function: 	print_ipx_lan
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the ipx lan metrics
 */
void
print_ipx_lan( int row, int col ) {
	struct metric *metp, *metp2;	

	if( need_header & METS ) {
		set_label_color( 0 );
		move( row, col+8 );
		printw(gettxt("RTPM:1085", "in datpk/s         in bcastpk/s         in diagpk/s         ioctlpk/s"));
		move( row+1, col+8 );
		printw(gettxt("RTPM:1086", " routed/s           to ISM/s             rte to ISM          setlan/s"));
		move( row+2, col+8 );
		printw(gettxt("RTPM:1087", " coalesced          to NIC/s             addr NIC            getlan/s"));
		move( row+3, col+8 );
		printw(gettxt("RTPM:1088", " bad len             diagnostic           rte ISM            setsap/s"));
		move( row+4, col+8 );
		printw(gettxt("RTPM:1134", " fwd rtr/s            to lans             ipx rsp/s          setinfo/s"));
		move( row+5, col+8 );
		printw(gettxt("RTPM:1090", " rte ISM/s            rte ipx           out totpk/s          getinfo/s"));
		move( row+6, col+8 );
		printw(gettxt("RTPM:1091", " dropped              ipx resp           tot ISM/s           getnode/s"));
		move( row+7, col+8 );
		printw(gettxt("RTPM:1131", " too small           routed/s            prop pkt/s          getaddr/s"));
		move( row+8, col+8 );
		printw(gettxt("RTPM:1093", " not DPLI           DLPI echo/s          rte ISM/s           getstat/s"));
		move( row+9, col+8 );
		printw(gettxt("RTPM:1094", "in sappk/s         in rippk/s            rte lan/s           links/s"));
		move( row+10, col+8 );
		printw(gettxt("RTPM:1095", " rte ISM/s          rte ipx/s            qued lan/s          unlink/s"));
		move( row+11, col+8 );
		printw(gettxt("RTPM:1133", " rtesapd/s          dropped/s            same sockt          unknown"));
		move( row+12, col+8 );
		printw(gettxt("RTPM:1097", " bad pkts          in netbio/s           fillin dst"));
		move( row+13, col+8 );
		printw(gettxt("RTPM:1132", " no ISM/s           not routed           bad lan"));
	}

	/* FIRST COLUMN OF METRICS */

	/* Total IPX data packets received from the lan(s) */
	metp  = &mettbl[ IPXLANR_InTotal_IDX ];
	mk_field( metp, metp, 0, 0, row, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  Packets, routed to node on connected net */
	metp  = &mettbl[ IPXLANR_InRoute_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Up stream data IPX packets coalesced */
	metp  = &mettbl[ IPXLAN_InCoalesced_IDX ];
	metp2 = &mettbl[ IPXLANR_InCoalesced_IDX ];
	mk_field( metp, metp2, 0, 0, row+2, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* Packets, smaller than IPX header size, dropped */
	metp  = &mettbl[ IPXLAN_InBadLength_IDX ];
	metp2 = &mettbl[ IPXLANR_InBadLength_IDX ];
	mk_field( metp, metp2, 0, 0, row+3, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/*  Packets, destination not my net, forwarded to next router */
	metp  = &mettbl[ IPXLANR_InForward_IDX ];
	mk_field( metp, metp, 0, 0, row+4, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  Packets, routed to internal net */
	metp  = &mettbl[ IPXLANR_InInternalNet_IDX ];
	mk_field( metp, metp, 0, 0, row+5, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  Packets, addressed to NIC, not diagnostic, dropped */
	metp  = &mettbl[ IPXLAN_InNICDropped_IDX ];
	metp2 = &mettbl[ IPXLANR_InNICDropped_IDX ];
	mk_field( metp, metp2, 0, 0, row+6, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* Up stream packets DLPI header too small, dropped */
	metp  = &mettbl[ IPXLAN_InProtoSize_IDX ];
	metp2 = &mettbl[ IPXLANR_InProtoSize_IDX ];
	mk_field( metp, metp2, 0, 0, row+7, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* Up stream packets, not DLPI data type, dropped */
	metp  = &mettbl[ IPXLAN_InBadDLPItype_IDX ];
	metp2 = &mettbl[ IPXLANR_InBadDLPItype_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/*  IPX/SAP packets */
	metp  = &mettbl[ IPXLANR_InSap_IDX ];
	mk_field( metp, metp, 0, 0, row+9, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  IPX/SAP packets, routed to ipx */
	metp  = &mettbl[ IPXLANR_InSapIpx_IDX ];
	mk_field( metp, metp, 0, 0, row+10, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  IPX/SAP packets, not under ipx, routed to sapd */
	metp  = &mettbl[ IPXLANR_InSapNoIpxToSapd_IDX ];
	mk_field( metp, metp, 0, 0, row+11, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  IPX/SAP packets invalid, dropped */
	metp  = &mettbl[ IPXLAN_InSapBad_IDX ];
	metp2 = &mettbl[ IPXLANR_InSapBad_IDX ];
	mk_field( metp, metp2, 0, 0, row+12, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/*  IPX/SAP packets, not under ipx, no sapd, dropped */
	/* metp  = &mettbl[ IPXLAN_InSapNoIpxDrop_IDX ]; */
	metp2 = &mettbl[ IPXLANR_InSapNoIpxDrop_IDX ];
	mk_field( metp2, metp2, 0, 0, row+13, col, 7, "%7.0f", metp2->metval->cooked, metp2->metval->cooked, NULL );

	/* SECOND COLUMN */

	/*  Broadcast packets */
	metp  = &mettbl[ IPXLANR_InBroadcast_IDX ];
	mk_field( metp, metp, 0, 0, row, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  Broadcast packets addressed to internal net */
	metp  = &mettbl[ IPXLANR_InBroadcastInternal_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  Broadcast packets addressed to NIC */
	metp  = &mettbl[ IPXLANR_InBroadcastNIC_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  IPX/DIAGNOSTIC broadcast packets addressed to NIC */
	metp  = &mettbl[ IPXLAN_InBroadcastDiag_IDX ];
	metp2 = &mettbl[ IPXLANR_InBroadcastDiag_IDX ];
	mk_field( metp, metp2, 0, 0, row+3, col+19, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/*  IPX/DIAGNOSTIC packets, forwarded to lan(s) */
	metp  = &mettbl[ IPXLAN_InBroadcastDiagFwd_IDX ];
	metp2 = &mettbl[ IPXLANR_InBroadcastDiagFwd_IDX ];
	mk_field( metp, metp2, 0, 0, row+4, col+19, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/*  IPX/DIAGNOSTIC packets, routed to ipx */
	metp  = &mettbl[ IPXLAN_InBroadcastDiagRoute_IDX ];
	metp2 = &mettbl[ IPXLANR_InBroadcastDiagRoute_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col+19, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/*  IPX/DIAGNOSTIC packets, no ipx, lan router responded */
	metp  = &mettbl[ IPXLAN_InBroadcastDiagResp_IDX ];
	metp2 = &mettbl[ IPXLANR_InBroadcastDiagResp_IDX ];
	mk_field( metp, metp2, 0, 0, row+6, col+19, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/*  Broadcast Packets addressed to NIC, routed */
	metp  = &mettbl[ IPXLAN_InBroadcastRoute_IDX ];
	metp2 = &mettbl[ IPXLANR_InBroadcastRoute_IDX ];
	mk_field( metp, metp2, 0, 0, row+7, col+19, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* Broadcast packets echoed back by DLPI driver, dropped */
	metp  = &mettbl[ IPXLAN_InDriverEcho_IDX ];
	metp2 = &mettbl[ IPXLANR_InDriverEcho_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col+19, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* IPX/RIP packets */
	metp  = &mettbl[ IPXLANR_InRip_IDX ];
	mk_field( metp, metp, 0, 0, row+9, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* IPX/RIP processed by router and routed to IPX */
	metp  = &mettbl[ IPXLANR_InRipRouted_IDX ];
	mk_field( metp, metp, 0, 0, row+10, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	/* IPX/RIP processed by router and dropped */
	/* metp  = &mettbl[ IPXLAN_InRipDropped_IDX ];	*/
	metp2 = &mettbl[ IPXLANR_InRipDropped_IDX ];
	mk_field( metp2, metp2, 0, 0, row+11, col+19, 7, "%7.0f", metp2->metval->cooked, metp2->metval->cooked, NULL );

	/* Up stream IPX/Propagation packets propagated */
	metp  = &mettbl[ IPXLANR_InPropagation_IDX ];
	mk_field( metp, metp, 0, 0, row+12, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Up stream IPX/Propagation packets not propagated */
	metp  = &mettbl[ IPXLAN_InNoPropagate_IDX ];
	metp2 = &mettbl[ IPXLANR_InNoPropagate_IDX ];
	mk_field( metp, metp2, 0, 0, row+13, col+19, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* THIRD COLUMN */

	/*  IPX/DIAGNOSTIC packets addressed to my net */
	metp  = &mettbl[ IPXLANR_InDiag_IDX ];
	mk_field( metp, metp, 0, 0, row, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  IPX/DIAGNOSTIC packets addressed internal net, routed to ipx */
	metp  = &mettbl[ IPXLAN_InDiagInternal_IDX ];
	metp2  = &mettbl[ IPXLANR_InDiagInternal_IDX ];
	mk_field( metp, metp2, 0, 0, row+1, col+40, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/*  IPX/DIAGNOSTIC packets addressed to NIC */
	metp  = &mettbl[ IPXLAN_InDiagNIC_IDX ];
	metp2 = &mettbl[ IPXLANR_InDiagNIC_IDX ];
	mk_field( metp, metp2, 0, 0, row+2, col+40, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/*  IPX/DIAGNOSTIC packets routed to ipx */
	metp  = &mettbl[ IPXLAN_InDiagIpx_IDX ];
	metp2 = &mettbl[ IPXLANR_InDiagIpx_IDX ];
	mk_field( metp, metp2, 0, 0, row+3, col+40, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/*  IPX/DIAGNOSTIC packets no ipx, lan router responded */
	/*	metp  = &mettbl[ IPXLAN_InDiagNoIpx_IDX ];	*/
	metp2 = &mettbl[ IPXLANR_InDiagNoIpx_IDX ];
	mk_field( metp2, metp2, 0, 0, row+4, col+40, 7, "%7.0f", metp2->metval->cooked, metp2->metval->cooked, NULL );

	/* Total IPX data packets sent to a lan or internal net */
	metp  = &mettbl[ IPXLANR_OutTotal_IDX ];
	mk_field( metp, metp, 0, 0, row+5, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Total IPX data packets from upstream */
	metp  = &mettbl[ IPXLANR_OutTotalStream_IDX ];
	mk_field( metp, metp, 0, 0, row+6, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Down stream IPX/Propagation packets propagated */
	metp  = &mettbl[ IPXLANR_OutPropagation_IDX ];
	mk_field( metp, metp, 0, 0, row+7, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Packets, routed to internal net */
	metp  = &mettbl[ IPXLANR_OutInternal_IDX ];
	mk_field( metp, metp, 0, 0, row+8, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Packets, sent to lan */
	metp  = &mettbl[ IPXLANR_OutSent_IDX ];
	mk_field( metp, metp, 0, 0, row+9, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Packets, queued to lan */
	metp  = &mettbl[ IPXLANR_OutQueued_IDX ];
	mk_field( metp, metp, 0, 0, row+10, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Packets, returned to socket router, dest/src socket same, dropped */
	metp = &mettbl[ IPXLAN_OutSameSocket_IDX ];
	metp2  = &mettbl[ IPXLANR_OutSameSocket_IDX ];
	mk_field( metp, metp2, 0, 0, row+11, col+40, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* Packets, destination net/node filled with internal net/node */
	metp  = &mettbl[ IPXLANR_OutFillInDest_IDX ];
	mk_field( metp, metp, 0, 0, row+12, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Packets, router error, bad lan, dropped */
	metp  = &mettbl[ IPXLAN_OutBadLan_IDX ];
	metp2  = &mettbl[ IPXLANR_OutBadLan_IDX ];
	mk_field( metp, metp2, 0, 0, row+13, col+40, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* FOURTH COLUMN */

	/*  IOCTL packets total */
	metp = &mettbl[ IPXLANR_Ioctl_IDX ];
	mk_field( metp, metp, 0, 0, row, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Set Configured Lans */
	metp  = &mettbl[ IPXLANR_IoctlSetLans_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Get Configured Lans */
	metp  = &mettbl[ IPXLANR_IoctlGetLans_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Set SAP Queue */
	metp  = &mettbl[ IPXLANR_IoctlSetSapQ_IDX ];
	mk_field( metp, metp, 0, 0, row+3, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Set Lan Info */
	metp  = &mettbl[ IPXLANR_IoctlSetLanInfo_IDX ];
	mk_field( metp, metp, 0, 0, row+4, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Get Lan Info */
	metp  = &mettbl[ IPXLANR_IoctlGetLanInfo_IDX ];
	mk_field( metp, metp, 0, 0, row+5, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	/* Get Node Addr */
	metp  = &mettbl[ IPXLANR_IoctlGetNodeAddr_IDX ];
	mk_field( metp, metp, 0, 0, row+6, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	/* Get Net Addr */
	metp  = &mettbl[ IPXLANR_IoctlGetNetAddr_IDX ];
	mk_field( metp, metp, 0, 0, row+7, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	/* Get Statistics */
	metp  = &mettbl[ IPXLANR_IoctlGetStats_IDX ];
	mk_field( metp, metp, 0, 0, row+8, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	/* Link */
	metp  = &mettbl[ IPXLANR_IoctlLink_IDX ];
	mk_field( metp, metp, 0, 0, row+9, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	/* Unlink */
	metp  = &mettbl[ IPXLANR_IoctlUnlink_IDX ];
	mk_field( metp, metp, 0, 0, row+10, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	/* Unknown type */
	metp  = &mettbl[ IPXLAN_IoctlUnknown_IDX ];
	metp2 = &mettbl[ IPXLANR_IoctlUnknown_IDX ];
	mk_field( metp, metp2, 0, 0, row+11, col+60, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
}
/*
 *	function: 	print_ipx
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print summary ipx metrics
 */
void
print_ipx( int row, int col ) {
	struct metric *metp, *metp2;	

	if( need_header & METS ) {
		mk_field( NULL, NULL, 0, 0, row, col, 47, gettxt("RTPM:1125","                IPX LAN ROUTER                 "), NULL, NULL, print_ipx_lan );
		mk_field( NULL, NULL, 0, 0, row, col+49, 30, gettxt("RTPM:1126", "    IPX SOCKET MUX            "), NULL, NULL, print_ipx_sock );
		set_label_color( 0 );
		move( row+1, col+8 );
		printw(gettxt("RTPM:1113", "in data pkt/s          in RIP pkt/s              tot rcved data pkt/s"));
		move( row+2, col+8 );
		printw(gettxt("RTPM:1114", "  routed/s               rte ISM/s               pkts rcved by ISM/s"));
		move( row+3, col+8 );
		printw(gettxt("RTPM:1115", "  coalesced            in netbios pkt/s          out to lan router/s"));
		move( row+4, col+8 );
		printw(gettxt("RTPM:1116", "  fwd router/s         in diagpk/s               IPX routed/s"));
		move( row+5, col+8 );
		printw(gettxt("RTPM:1117", "  route ISM/s            route to ISM            out non-TLI data pkt/s"));
		move( row+6, col+8 );
		printw(gettxt("RTPM:1118", "in sap pkt/s           out tot pkt/s             out TLI data pkt/s"));
		move( row+7, col+8 );
		printw(gettxt("RTPM:1119", "  route ipx/s            tot ISM/s               TLI out to lan routr/s"));
		move( row+8, col+8 );
		printw(gettxt("RTPM:1120", "  route sapd/s           netbios pkt/s           data pkts to socket/s"));
		move( row+9, col+8 );
		printw(gettxt("RTPM:1121", "in bcast pkt/s           route ISM/s             IPX routed TLI/s"));
		move( row+10, col+8 );
		printw(gettxt("RTPM:1122", "  to ISM/s               sent to lan/s           sent to TLI socket/s"));
		move( row+11, col+8 );
		printw(gettxt("RTPM:1123", "  to NIC/s               queued to lan/s"));
		move( row+12, col+8 );
		printw(gettxt("RTPM:1124", "  DLPI echo/s          ioctl pkt/s"));
	}
	row++;

	/* LAN METRICS  */
	/* FIRST COLUMN OF METRICS */

	/* Total IPX data packets received from the lan(s) */
	metp  = &mettbl[ IPXLANR_InTotal_IDX ];
	mk_field( metp, metp, 0, 0, row, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  Packets, routed to node on connected net */
	metp  = &mettbl[ IPXLANR_InRoute_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Up stream data IPX packets coalesced */
	metp  = &mettbl[ IPXLAN_InCoalesced_IDX ];
	metp2 = &mettbl[ IPXLANR_InCoalesced_IDX ];
	mk_field( metp, metp2, 0, 0, row+2, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/*  Packets, destination not my net, forwarded to next router */
	metp  = &mettbl[ IPXLANR_InForward_IDX ];
	mk_field( metp, metp, 0, 0, row+3, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  Packets, routed to internal net */
	metp  = &mettbl[ IPXLANR_InInternalNet_IDX ];
	mk_field( metp, metp, 0, 0, row+4, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  IPX/SAP packets */
	metp  = &mettbl[ IPXLANR_InSap_IDX ];
	mk_field( metp, metp, 0, 0, row+5, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  IPX/SAP packets, routed to ipx */
	metp  = &mettbl[ IPXLANR_InSapIpx_IDX ];
	mk_field( metp, metp, 0, 0, row+6, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  IPX/SAP packets, not under ipx, routed to sapd */
	metp  = &mettbl[ IPXLANR_InSapNoIpxToSapd_IDX ];
	mk_field( metp, metp, 0, 0, row+7, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  Broadcast packets */
	metp  = &mettbl[ IPXLANR_InBroadcast_IDX ];
	mk_field( metp, metp, 0, 0, row+8, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  Broadcast packets addressed to internal net */
	metp  = &mettbl[ IPXLANR_InBroadcastInternal_IDX ];
	mk_field( metp, metp, 0, 0, row+9, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  Broadcast packets addressed to NIC */
	metp  = &mettbl[ IPXLANR_InBroadcastNIC_IDX ];
	mk_field( metp, metp, 0, 0, row+10, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Broadcast packets echoed back by DLPI driver, dropped */
	metp  = &mettbl[ IPXLAN_InDriverEcho_IDX ];
	metp2 = &mettbl[ IPXLANR_InDriverEcho_IDX ];
	mk_field( metp, metp2, 0, 0, row+11, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* SECOND COLUMN */

	/* IPX/RIP packets */
	metp  = &mettbl[ IPXLANR_InRip_IDX ];
	mk_field( metp, metp, 0, 0, row, col+23, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* IPX/RIP processed by router and routed to IPX */
	metp  = &mettbl[ IPXLANR_InRipRouted_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col+23, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Up stream IPX/Propagation packets propagated */
	metp  = &mettbl[ IPXLANR_InPropagation_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col+23, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  IPX/DIAGNOSTIC packets addressed to my net */
	metp  = &mettbl[ IPXLANR_InDiag_IDX ];
	mk_field( metp, metp, 0, 0, row+3, col+23, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  IPX/DIAGNOSTIC packets addressed internal net, routed to ipx */
	metp  = &mettbl[ IPXLAN_InDiagInternal_IDX ];
	metp2  = &mettbl[ IPXLANR_InDiagInternal_IDX ];
	mk_field( metp, metp2, 0, 0, row+4, col+23, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	/* Total IPX data packets sent to a lan or internal net */
	metp  = &mettbl[ IPXLANR_OutTotal_IDX ];
	mk_field( metp, metp, 0, 0, row+5, col+23, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Total IPX data packets from upstream */
	metp  = &mettbl[ IPXLANR_OutTotalStream_IDX ];
	mk_field( metp, metp, 0, 0, row+6, col+23, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Down stream IPX/Propagation packets propagated */
	metp  = &mettbl[ IPXLANR_OutPropagation_IDX ];
	mk_field( metp, metp, 0, 0, row+7, col+23, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Packets, routed to internal net */
	metp  = &mettbl[ IPXLANR_OutInternal_IDX ];
	mk_field( metp, metp, 0, 0, row+8, col+23, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Packets, sent to lan */
	metp  = &mettbl[ IPXLANR_OutSent_IDX ];
	mk_field( metp, metp, 0, 0, row+9, col+23, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* Packets, queued to lan */
	metp  = &mettbl[ IPXLANR_OutQueued_IDX ];
	mk_field( metp, metp, 0, 0, row+10, col+23, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/*  IOCTL packets total */
	metp = &mettbl[ IPXLANR_Ioctl_IDX ];
	mk_field( metp, metp, 0, 0, row+11, col+23, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* IPX SOCKET MUX Metrics */

	/* data pkt/s */
	/* Total IPX data packets received from the stream head */
	metp  = &mettbl[ IPXR_datapackets_IDX ];
	mk_field( metp, metp, 0, 0, row, col+49, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* snt mux/s */
	/* Sent to the socket multiplexor */
	metp  = &mettbl[ IPXSOCKR_IpxInData_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col+49, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* out to switch/s */
	/* Sent to IPX switch */
	metp  = &mettbl[ IPXSOCKR_IpxOutToSwitch_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col+49, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* IPX routed/s */
	/* Valid socket found */
	metp  = &mettbl[ IPXSOCKR_IpxRouted_IDX ];
	mk_field( metp, metp, 0, 0, row+3, col+49, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* non-TLI/s */
	/* Non TLI IPX data packets */
	metp  = &mettbl[ IPXSOCKR_IpxOutData_IDX ];
	mk_field( metp, metp, 0, 0, row+4, col+49, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* TLI out data pkt/s */
	/* TLI IPX data packets */
	metp  = &mettbl[ IPXSOCKR_IpxTLIOutData_IDX ];
	mk_field( metp, metp, 0, 0, row+5, col+49, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* TLI out to switch/s */
	/* Sent to IPX switch */
	metp  = &mettbl[ IPXSOCKR_IpxTLIOutToSwitch_IDX ];
	mk_field( metp, metp, 0, 0, row+6, col+49, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* data pkt rcv by sockt mux/s */
	/* Total data packets received by the socket multiplexor */
	metp  = &mettbl[ IPXSOCKR_IpxDataToSocket_IDX ];
	mk_field( metp, metp, 0, 0, row+7, col+49, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* rte to TLI/s */
	/* Destined for TLI socket */
	metp  = &mettbl[ IPXSOCKR_IpxRoutedTLI_IDX ];
	mk_field( metp, metp, 0, 0, row+8, col+49, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	/* snt to TLI sockt/s */
	/* Sent to TLI socket ( IpxRoutedTLI - IpxRoutedTLIAlloc ) */
	metp  = &mettbl[ IPXR_sent_to_tli_IDX ];
	mk_field( metp, metp, 0, 0, row+9, col+49, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
}


/*
 *	function: 	print_sap_lan
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the per lan sap metrics
 */
void
print_sap_lan( int row, int col ) {
	int old_need_header = need_header;

	print_perlan_hdr( row++, col );
	if( need_header & METS ) {
		set_label_color( 0 );
		move( row, col );
		printw(gettxt("RTPM:1077", "lan index"));
		move( row+1, col );
		printw(gettxt("RTPM:1078", "updt intvl"));
		move( row+2, col );
		printw(gettxt("RTPM:1079", "age factor"));
		move( row+3, col );
		printw(gettxt("RTPM:1080", "pktgap(ms)"));
		move( row+4, col );
		printw(gettxt("RTPM:1081", "pkt sz"));
		move( row+5, col );
		printw(gettxt("RTPM:1082", "pkt snt/s"));
		move( row+6, col );
		printw(gettxt("RTPM:1083", "pkt rcv/s"));
		move( row+7, col );
		printw(gettxt("RTPM:1084", "  bad pkts"));
		need_header = 0;
	}

 	/* lan index */
 	rprint_metric( &mettbl[SAPLAN_LanNumber_IDX],&mettbl[SAPLAN_LanNumber_IDX], row++, col, 0 );
	/* periodic update interval */
 	rprint_metric( &mettbl[SAPLAN_UpdateInterval_IDX],&mettbl[SAPLAN_UpdateInterval_IDX], row++, col, 0 );
 	/* intervals b4 timeout */
 	rprint_metric( &mettbl[SAPLAN_AgeFactor_IDX],&mettbl[SAPLAN_AgeFactor_IDX],row++, col, 0 );
 	/* ms between pkts */
 	rprint_metric( &mettbl[SAPLAN_PacketGap_IDX],&mettbl[SAPLAN_PacketGap_IDX],row++, col, 0 );
 	/* pkt size */
 	rprint_metric( &mettbl[SAPLAN_PacketSize_IDX],&mettbl[SAPLAN_PacketSize_IDX],row++, col, 0 );
 	/* pkt/s sent */
 	rprint_metric( &mettbl[SAPLANR_PacketsSent_IDX],&mettbl[SAPLANR_PacketsSent_IDX],row++, col, 0 );
	/* pkt/s rcv */
 	rprint_metric( &mettbl[SAPLANR_PacketsReceived_IDX],&mettbl[SAPLANR_PacketsReceived_IDX],row++, col, 0 );
	/* bad pkts rcv */
 	rprint_metric( &mettbl[SAPLAN_BadPktsReceived_IDX],&mettbl[SAPLANR_BadPktsReceived_IDX],row++, col, 0 );
	need_header = old_need_header;
}

/*
 *	function: 	print_sap
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the sap metrics
 */
void
print_sap( int row, int col ) {
	struct metric *metp, *metp2;	

	if( need_header & METS ) {
		set_label_color( 0 );
		move( row, col+9 );
		printw(gettxt("RTPM:1062", "                                             receive:        sent:"));
		move( row+1, col+9 );
		printw(gettxt("RTPM:1063", "lans known             SAP pkts:                  rcv/s          snt/s"));
		move( row+2, col+9 );
		printw(gettxt("RTPM:1064", "servers known            GSQ:                     rcv/s          snt/s"));
		move( row+3, col+9 );
		printw(gettxt("RTPM:1065", "pkts snt/s               GSR:                     rcv/s          snt/s"));
		move( row+4, col+9 );
		printw(gettxt("RTPM:1066", "pkts rcv/s               NSQ:                     rcv/s "));
		move( row+5, col+9 );
		printw(gettxt("RTPM:1067", "  RIP net down           NSR:                                    snt/s"));
		move( row+6, col+9 );
		printw(gettxt("RTPM:1068", "  RIP server down        outside of lan:          rcv            snt"));
		move( row+7, col+9 );
		printw(gettxt("RTPM:1069", "  RIP bad pkts           lcl advertise:           req/s          ack/s"));
		move( row+8, col+9 );
		printw(gettxt("RTPM:1070", "  bad pkts                                                       nacks"));
		move( row+9, col+9 );
		printw(gettxt("RTPM:1071", "  bad size               lcl chg notify:          req/s          ack/s"));
		move( row+10, col+9 );
		printw(gettxt("RTPM:1072", "  invalid src                                                    nacks"));
		move( row+11, col+9 );
		printw(gettxt("RTPM:1073", "  drop, SAPD echoed                               procs          req/s"));
		move( row+12, col+9 );
		printw(gettxt("RTPM:1074", "server alloc fail        get shm id:              req/s          ak/s "));
		move( row+13, col+9 );
		printw(gettxt("RTPM:1075", "source alloc fail"));

		mk_field(NULL, NULL, 0, 0, row, col, 28, gettxt("RTPM:1076", "    LAN DATA:               "),NULL, NULL, print_sap_lan );
	}		

	metp = &mettbl[ SAP_Lans_IDX ];		/* lans known */
	mk_field( metp, metp, 0, 0, row+1, col, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAP_total_servers_IDX ];/* servers known */
	mk_field( metp, metp, 0, 0, row+2, col, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAPLANR_PacketsSent_IDX ]; /* snt/s */
	mk_field( metp, metp, nlans, 0, row+3, col, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAPLANR_PacketsReceived_IDX ]; /* rcv/s */
	mk_field( metp, metp, nlans, 0, row+4, col, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAP_TotalInRipSaps_IDX ];	/* net down */
	metp2 = &mettbl[ SAPR_TotalInRipSaps_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SAP_RipServerDown_IDX ]; /* server down */
	metp2 = &mettbl[ SAPR_RipServerDown_IDX ];
	mk_field( metp, metp2, 0, 0, row+6, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SAP_BadRipSaps_IDX ];	/* bad rip pkts */
	metp2 = &mettbl[ SAPR_BadRipSaps_IDX ];
	mk_field( metp, metp2, 0, 0, row+7, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SAPLAN_BadPktsReceived_IDX ]; /* bad pkts rcv */
	metp2 = &mettbl[ SAPLANR_BadPktsReceived_IDX ];
	mk_field( metp, metp2, nlans, 0, row+8, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SAP_BadSizeInSaps_IDX ];/* bad size pkt rcv */
	metp2 = &mettbl[ SAPR_BadSizeInSaps_IDX ];
	mk_field( metp, metp2, 0, 0, row+9, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SAP_BadSapSource_IDX ];/* bad src pkt rcv */
	metp2 = &mettbl[ SAPR_BadSapSource_IDX ];
	mk_field( metp, metp2, 0, 0, row+10, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SAP_EchoMyOutput_IDX ];	/* SAPD drop */
	metp2 = &mettbl[ SAPR_EchoMyOutput_IDX ];
	mk_field( metp, metp2, 0, 0, row+11, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SAP_SrvAllocFailed_IDX ];	/* server alloc fail */
	metp2 = &mettbl[ SAPR_SrvAllocFailed_IDX ];
	mk_field( metp, metp2, 0, 0, row+12, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ SAP_MallocFailed_IDX ];	/* src alloc fail */
	metp2 = &mettbl[ SAPR_MallocFailed_IDX ];
	mk_field( metp, metp2, 0, 0, row+13, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SAPR_TotalInSaps_IDX ];	/* tot rcv */
	mk_field( metp, metp, 0, 0, row+1, col+50, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAPR_GSQReceived_IDX ]; /* gen srv queries */
	mk_field( metp, metp, 0, 0, row+2, col+50, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAPR_GSRReceived_IDX ]; /* gen srv replies */
	mk_field( metp, metp, 0, 0, row+3, col+50, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAPR_NSQReceived_IDX ]; /* near srv queries */
	mk_field( metp, metp, 0, 0, row+4, col+50, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAP_NotNeighbor_IDX ];  /* not on our lan */
	metp2 = &mettbl[ SAPR_NotNeighbor_IDX ];
	mk_field( metp, metp2, 0, 0, row+6, col+50, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SAPR_SASReceived_IDX ]; /* lcl req to adv server */
	mk_field( metp, metp, 0, 0, row+7, col+50, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAPR_SNCReceived_IDX ]; /* lcl notify chg req  */
	mk_field( metp, metp, 0, 0, row+9, col+50, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAPR_ProcessesToNotify_IDX ]; /* procs req chg notify*/
	mk_field( metp, metp, 0, 0, row+11, col+50, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAPR_GSIReceived_IDX ]; /* lcl get shmid req */
	mk_field( metp, metp, 0, 0, row+12, col+50, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );


	metp = &mettbl[ SAPR_TotalOutSaps_IDX ];/* tot snt */
	mk_field( metp, metp, 0, 0, row+1, col+65, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAPR_GSQSent_IDX ];    /* gen srv queries */
	mk_field( metp, metp, 0, 0, row+2, col+65, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAPR_GSRSent_IDX ];/* gen srv replies */
	mk_field( metp, metp, 0, 0, row+3, col+65, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAPR_NSRSent_IDX ]; /* near srv replies */
	mk_field( metp, metp, 0, 0, row+5, col+65, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAP_BadDestOutSaps_IDX ]; /* not on our lan  */
	metp2 = &mettbl[ SAPR_BadDestOutSaps_IDX ];
	mk_field( metp, metp2, 0, 0, row+6, col+65, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SAPR_SASAckSent_IDX ];	/* lcl adv req ack */
	mk_field( metp, metp, 0, 0, row+7, col+65, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );
;
	metp = &mettbl[ SAP_SASNackSent_IDX ]; /* lcl adv nak */
	metp2 = &mettbl[ SAPR_SASNackSent_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col+65, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SAPR_SNCAckSent_IDX ]; /* chg notify ack */
	mk_field( metp, metp, 0, 0, row+9, col+65, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAP_SNCNackSent_IDX ]; /* chg notify nak */
	metp2 = &mettbl[ SAPR_SNCNackSent_IDX ];
	mk_field( metp, metp2, 0, 0, row+10, col+65, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SAPR_NotificationsSent_IDX ]; /* notify's sent */
	mk_field( metp, metp, 0, 0, row+11, col+65, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAPR_GSIAckSent_IDX ];	/* get shmid ack */
	mk_field( metp, metp, 0, 0, row+12, col+65, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

}

/*
 *	function: 	print_spx_snd
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the per line spx send metrics
 */
void
print_spx_snd( int row, int col ) {
	int old_need_header = need_header;

	print_perspx_hdr( row++, col );

	if( need_header & METS ) {
		set_label_color( 0 );
		move( row, col );
		printw(gettxt("RTPM:1044", "msg snt/s"));
		move( row+1, col );
		printw(gettxt("RTPM:1045", " unk msgs"));
		move( row+2, col );
		printw(gettxt("RTPM:1046", " bad msgs"));
		move( row+3, col );
		printw(gettxt("RTPM:1047", "pkt snt/s"));
		move( row+4, col );
		printw(gettxt("RTPM:1048", " timeouts"));
		move( row+5, col );
		printw(gettxt("RTPM:1049", " pkt naks"));
		move( row+6, col );
		printw(gettxt("RTPM:1050", "aks snt/s"));
		move( row+7, col );
		printw(gettxt("RTPM:1051", "naks snt"));
		move( row+8, col );
		printw(gettxt("RTPM:1052", "watchdg/s"));
		need_header = 0;
	}
 	rprint_metric(  &mettbl[SPXCONR_con_send_mesg_count_IDX],&mettbl[SPXCONR_con_send_mesg_count_IDX],row++, col, 2 );
   	rprint_metric(  &mettbl[SPXCON_con_unknown_mesg_count_IDX],&mettbl[SPXCONR_con_unknown_mesg_count_IDX],row++, col, 2 );
 	rprint_metric(  &mettbl[SPXCON_con_send_bad_mesg_IDX],&mettbl[SPXCONR_con_send_bad_mesg_IDX],row++, col, 2 );
 	rprint_metric(  &mettbl[SPXCONR_con_send_packet_count_IDX],&mettbl[SPXCONR_con_send_packet_count_IDX],row++, col, 2 );
 	rprint_metric(  &mettbl[SPXCON_con_send_packet_timeout_IDX],&mettbl[SPXCONR_con_send_packet_timeout_IDX],row++, col, 2 );
 	rprint_metric(  &mettbl[SPXCON_con_send_packet_nak_IDX],&mettbl[SPXCONR_con_send_packet_nak_IDX],row++, col, 2 );
 	rprint_metric(  &mettbl[SPXCONR_con_send_ack_IDX],&mettbl[SPXCONR_con_send_ack_IDX],row++, col, 2 );
 	rprint_metric(  &mettbl[SPXCON_con_send_nak_IDX],&mettbl[SPXCONR_con_send_nak_IDX],row++, col, 2 );
 	rprint_metric(  &mettbl[SPXCONR_con_send_watchdog_IDX],&mettbl[SPXCONR_con_send_watchdog_IDX],row++, col, 2 );
	need_header = old_need_header;
}
/*
 *	function: 	print_spx_rcv
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the per line spx receive metrics
 */
void
print_spx_rcv( int row, int col ) {
	int old_need_header = need_header;

	print_perspx_hdr( row++, col );

	if( need_header & METS ) {
		set_label_color( 0 );
		move( row, col );
		printw(gettxt("RTPM:1053", "pkt rcv/s"));
		move( row+1, col );
		printw(gettxt("RTPM:1054", " bad pkts"));
		move( row+2, col );
		printw(gettxt("RTPM:1055", " bad data"));
		move( row+3, col );
		printw(gettxt("RTPM:1056", " dup pkts"));
		move( row+4, col );
		printw(gettxt("RTPM:1057", " out seq"));
		move( row+5, col );
		printw(gettxt("RTPM:1058", " sentup/s"));
		move( row+6, col );
		printw(gettxt("RTPM:1059", " queued/s"));
		move( row+7, col );
		printw(gettxt("RTPM:1060", "rcv aks/s"));
		move( row+8, col );
		printw(gettxt("RTPM:1061", "rcvnaks/s"));
		move( row+9, col );
		printw(gettxt("RTPM:1052", "watchdg/s"));
		need_header = 0;
	}
 	rprint_metric( &mettbl[SPXCONR_con_rcv_packet_count_IDX],&mettbl[SPXCONR_con_rcv_packet_count_IDX],row++, col, 2 );
 	rprint_metric( &mettbl[SPXCON_con_rcv_bad_packet_IDX],&mettbl[SPXCONR_con_rcv_bad_packet_IDX],row++, col, 2 );
 	rprint_metric( &mettbl[SPXCON_con_rcv_bad_data_packet_IDX],&mettbl[SPXCONR_con_rcv_bad_data_packet_IDX],row++, col, 2 );
 	rprint_metric( &mettbl[SPXCON_con_rcv_dup_packet_IDX],&mettbl[SPXCONR_con_rcv_dup_packet_IDX],row++, col, 2 );
 	rprint_metric( &mettbl[SPXCON_con_rcv_packet_outseq_IDX],&mettbl[SPXCONR_con_rcv_packet_outseq_IDX],row++, col, 2 );
 	rprint_metric( &mettbl[SPXCONR_con_rcv_packet_sentup_IDX],&mettbl[SPXCONR_con_rcv_packet_sentup_IDX],row++, col, 2 );
 	rprint_metric( &mettbl[SPXCONR_con_rcv_packet_qued_IDX],&mettbl[SPXCONR_con_rcv_packet_qued_IDX],row++, col, 2 );
 	rprint_metric( &mettbl[SPXCONR_con_rcv_ack_IDX],&mettbl[SPXCONR_con_rcv_ack_IDX],row++, col, 2 );
 	rprint_metric( &mettbl[SPXCON_con_rcv_nak_IDX],&mettbl[SPXCONR_con_rcv_nak_IDX],row++, col, 2 );
 	rprint_metric( &mettbl[SPXCONR_con_rcv_watchdog_IDX],&mettbl[SPXCONR_con_rcv_watchdog_IDX],row++, col, 2 );
	need_header = old_need_header;
}

/*
 *	function: 	print_spx_msc
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the per line spx miscellaneous metrics
 */
void
print_spx_msc( int row, int col ) {
	int old_need_header = need_header;

	print_perspx_hdr( row++, col );
	if( need_header & METS ) {
		set_label_color( 0 );
		move( row, col );
		printw(gettxt("RTPM:1062", "connect id"));
		move( row+1, col );
		printw(gettxt("RTPM:1063", "state"));
		move( row+2, col );
		printw(gettxt("RTPM:1064", "snd size"));
		move( row+3, col );
		printw(gettxt("RTPM:1065", "rcv size"));
		move( row+4, col );
		printw(gettxt("RTPM:1066", "retries"));
		move( row+5, col );
		printw(gettxt("RTPM:1067", " time(ms)"));
		move( row+6, col );
		printw(gettxt("RTPM:1068", "type"));
		move( row+7, col );
		printw(gettxt("RTPM:1069", "chksum"));
		move( row+8, col );
		printw(gettxt("RTPM:1070", "window sz"));
		move( row+9, col );
		printw(gettxt("RTPM:1071", "remot wsz"));
		move( row+10, col );
		printw(gettxt("RTPM:1072", "win choke"));
		move( row+11, col );
		printw(gettxt("RTPM:1073", "rtt"));
		need_header = 0;
	}
 	rprint_metric( 	&mettbl[SPXCON_connection_id_IDX],&mettbl[SPXCON_connection_id_IDX],row++, col, 0 );
 	rprint_metric( 	&mettbl[SPXCON_con_state_IDX],&mettbl[SPXCON_con_state_IDX],row++, col, 0 );
 	rprint_metric( 	&mettbl[SPXCON_con_send_packet_size_IDX],&mettbl[SPXCON_con_send_packet_size_IDX],row++, col, 0 );
 	rprint_metric( 	&mettbl[SPXCON_con_rcv_packet_size_IDX],&mettbl[SPXCON_con_rcv_packet_size_IDX],row++, col, 0 );
 	rprint_metric( 	&mettbl[SPXCON_con_retry_count_IDX],&mettbl[SPXCON_con_retry_count_IDX],row++, col, 0 );
 	rprint_metric( 	&mettbl[SPXCON_con_retry_time_IDX],&mettbl[SPXCON_con_retry_time_IDX],row++, col, 0 );
 	rprint_metric( 	&mettbl[SPXCON_con_type_IDX],&mettbl[SPXCON_con_type_IDX],row++, col, 0 );
 	rprint_metric( 	&mettbl[SPXCON_con_ipxChecksum_IDX],&mettbl[SPXCON_con_ipxChecksum_IDX],row++, col, 0 );
 	rprint_metric( 	&mettbl[SPXCON_con_window_size_IDX],&mettbl[SPXCON_con_window_size_IDX],row++, col, 0 );
 	rprint_metric( 	&mettbl[SPXCON_con_remote_window_size_IDX],&mettbl[SPXCON_con_remote_window_size_IDX],row++, col, 0 );
 	rprint_metric( 	&mettbl[SPXCON_con_window_choke_IDX],&mettbl[SPXCON_con_window_choke_IDX],row++, col, 0 );
 	rprint_metric( 	&mettbl[SPXCON_con_round_trip_time_IDX],&mettbl[SPXCON_con_round_trip_time_IDX],row++, col, 0 );

	need_header = old_need_header;
}

/*
 *	function: 	print_spx
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the spx metrics
 */
void
print_spx( int row, int col ) {
	struct metric *metp, *metp2;	

	if( need_header & METS ) {
		mk_field( NULL, NULL, 0, 0, row, col,  22, gettxt("RTPM:1041", "     SPX SEND:        "), NULL, NULL, print_spx_snd );
		mk_field( NULL, NULL, 0, 0, row, col+27, 24, gettxt("RTPM:1042", "     SPX RECEIVE:       "), NULL, NULL, print_spx_rcv );
		mk_field( NULL, NULL, 0, 0, row, col+55,  22, gettxt("RTPM:1043", "     SPX MISC:        "), NULL, NULL, print_spx_msc );
		set_label_color( 0 );
		move( row+1, col+9 );
		printw(gettxt("RTPM:1031", "msg/s                      pkt/s                       curr connects"));
		move( row+2, col+9 );
		printw(gettxt("RTPM:1032", "unk msgs                   bad pkts                    max inuse"));
		move( row+3, col+9 );
		printw(gettxt("RTPM:1033", "bad msgs                   bad data                    max connects"));
		move( row+4, col+9 );
		printw(gettxt("RTPM:1034", "pkts/s                     dup pkts                    open fails"));
		move( row+5, col+9 );
		printw(gettxt("RTPM:1035", "pkts timed out             sentup/s                    alloc fails"));
		move( row+6, col+9 );
		printw(gettxt("RTPM:1036", "pkts nak'd                 connect req/s               ioctl/s"));
		move( row+7, col+9 );
		printw(gettxt("RTPM:1037", "connect reqs/s               aborts"));
		move( row+8, col+9 );
		printw(gettxt("RTPM:1038", "connect fails                abort retries"));
		move( row+9, col+9 );
		printw(gettxt("RTPM:1039", "listen req/s               no listener"));
		move( row+10, col+9 );
		printw(gettxt("RTPM:1040", "listen fails"));

	}
	metp = &mettbl[ SPXR_send_mesg_count_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SPX_unknown_mesg_count_IDX ];
	metp2 = &mettbl[ SPXR_unknown_mesg_count_IDX ];
	mk_field( metp, metp2, 0, 0, row+2, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPX_send_bad_mesg_IDX ];
	metp2 = &mettbl[ SPXR_send_bad_mesg_IDX ];
	mk_field( metp, metp2, 0, 0, row+3, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPXR_send_packet_count_IDX ];
	mk_field( metp, metp, 0, 0, row+4, col, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SPX_send_packet_timeout_IDX ];
	metp2 = &mettbl[ SPXR_send_packet_timeout_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPX_send_packet_nak_IDX ];
	metp2 = &mettbl[ SPXR_send_packet_nak_IDX ];
	mk_field( metp, metp2, 0, 0, row+6, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPXR_connect_req_count_IDX ];
	mk_field( metp, metp, 0, 0, row+7, col, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SPX_connect_req_fails_IDX ];
	metp2 = &mettbl[ SPXR_connect_req_fails_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPXR_listen_req_IDX ];
	mk_field( metp, metp, 0, 0, row+9, col, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SPX_listen_req_fails_IDX ];
	metp2 = &mettbl[ SPXR_listen_req_fails_IDX ];
	mk_field( metp, metp2, 0, 0, row+10, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPXR_rcv_packet_count_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col+27, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SPX_rcv_bad_packet_IDX ];
	metp2 = &mettbl[ SPXR_rcv_bad_packet_IDX ];
	mk_field( metp, metp2, 0, 0, row+2, col+27, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPX_rcv_bad_data_packet_IDX ];
	metp2 = &mettbl[ SPXR_rcv_bad_data_packet_IDX ];
	mk_field( metp, metp2, 0, 0, row+3, col+27, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPX_rcv_dup_packet_IDX ];
	metp2 = &mettbl[ SPXR_rcv_dup_packet_IDX ];
	mk_field( metp, metp2, 0, 0, row+4, col+27, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPXR_rcv_packet_sentup_IDX ];
	mk_field( metp, metp, 0, 0, row+5, col+27, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SPXR_rcv_conn_req_IDX ];
	mk_field( metp, metp, 0, 0, row+6, col+27, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SPX_abort_connection_IDX ];
	metp2 = &mettbl[ SPXR_abort_connection_IDX ];
	mk_field( metp, metp2, 0, 0, row+7, col+27, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPX_max_retries_abort_IDX ];
	metp2 = &mettbl[ SPXR_max_retries_abort_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col+27, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPX_no_listeners_IDX ];
	metp2 = &mettbl[ SPXR_no_listeners_IDX ];
	mk_field( metp, metp2, 0, 0, row+9, col+27, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPX_current_connections_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col+55, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SPX_max_used_connections_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col+55, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SPX_max_connections_IDX ];
	mk_field( metp, metp, 0, 0, row+3, col+55, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SPX_open_failures_IDX ];
	metp2 = &mettbl[ SPXR_open_failures_IDX ];
	mk_field( metp, metp2, 0, 0, row+4, col+55, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPX_alloc_failures_IDX ];
	metp2 = &mettbl[ SPXR_alloc_failures_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col+55, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPXR_ioctls_IDX ];
	mk_field( metp, metp, 0, 0, row+6, col+55, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );



}

/*
 *	function: 	print_rip
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the rip metrics
 */
void
print_rip( int row, int col ) {
	struct metric *metp, *metp2;	

	if( need_header & METS ) {
		set_label_color( 0 );
		move( row, col );
		printw(gettxt("RTPM:1018", "      send:                    receive:                  ioctl:"));
		move( row+1, col+9 );
		printw(gettxt("RTPM:1019", "total pkt/s               total pkt/s              total ioctl/s"));
		move( row+2, col+9 );
		printw(gettxt("RTPM:1020", "request/s                 request/s                init/s"));
		move( row+3, col+9 );
		printw(gettxt("RTPM:1021", "response/s                bad router len           get hash sz/s"));
		move( row+4, col+9 );
		printw(gettxt("RTPM:1022", "alloc fails               response/s               get hash stats/s"));
		move( row+5, col+9 );
		printw(gettxt("RTPM:1023", "bad dest                  no lan key               dump hash tbl/s"));
		move( row+6, col+9 );
		printw(gettxt("RTPM:1024", "lan0 dropped              coalesced/s              get router tbl/s"));
		move( row+7, col+9 );
		printw(gettxt("RTPM:1025", "lan0 routed/s             coalesce fails           get net info/s"));
		move( row+8, col+9 );
		printw(gettxt("RTPM:1026", "                          unk requests             check sap src/s"));
		move( row+9, col+9 );
		printw(gettxt("RTPM:1027", "                                                   reset router/s"));
		move( row+10, col+9 );
		printw(gettxt("RTPM:1028", "                                                   down router/s"));
		move( row+11, col+9 );
		printw(gettxt("RTPM:1029", "                                                   stats/s"));
		move( row+12, col+9 );
		printw(gettxt("RTPM:1030", "                                                   unk"));
	}
	metp = &mettbl[ RIPR_total_router_packets_sent_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_SentRequestPackets_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_SentResponsePackets_IDX ];
	mk_field( metp, metp, 0, 0, row+3, col, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIP_SentAllocFailed_IDX ];
	metp2 = &mettbl[ RIPR_SentAllocFailed_IDX ];
	mk_field( metp, metp2, 0, 0, row+4, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ RIP_SentBadDestination_IDX ];
	metp2 = &mettbl[ RIPR_SentBadDestination_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ RIP_SentLan0Dropped_IDX ];
	metp2 = &mettbl[ RIPR_SentLan0Dropped_IDX ];
	mk_field( metp, metp2, 0, 0, row+6, col, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ RIPR_SentLan0Routed_IDX ];
	mk_field( metp, metp, 0, 0, row+7, col, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_ReceivedPackets_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col+26, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_ReceivedRequestPackets_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col+26, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIP_ReceivedBadLength_IDX ];
	metp2 = &mettbl[ RIPR_ReceivedBadLength_IDX ];
	mk_field( metp, metp2, 0, 0, row+3, col+26, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ RIPR_ReceivedResponsePackets_IDX ];
	mk_field( metp, metp, 0, 0, row+4, col+26, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIP_ReceivedNoLanKey_IDX ];
	metp2 = &mettbl[ RIPR_ReceivedNoLanKey_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col+26, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ RIPR_ReceivedCoalesced_IDX ];
	mk_field( metp, metp, 0, 0, row+6, col+26, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIP_ReceivedNoCoalesce_IDX ];
	metp2 = &mettbl[ RIPR_ReceivedNoCoalesce_IDX ];
	mk_field( metp, metp2, 0, 0, row+7, col+26, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
	metp = &mettbl[ RIP_ReceivedUnknownRequest_IDX ];
	metp2 = &mettbl[ RIPR_ReceivedUnknownRequest_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col+26, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ RIPR_ioctls_processed_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col+51, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_RipxIoctlInitialize_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col+51, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_RipxIoctlGetHashSize_IDX ];
	mk_field( metp, metp, 0, 0, row+3, col+51, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_RipxIoctlGetHashStats_IDX ];
	mk_field( metp, metp, 0, 0, row+4, col+51, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_RipxIoctlDumpHashTable_IDX ];
	mk_field( metp, metp, 0, 0, row+5, col+51, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_RipxIoctlGetRouterTable_IDX ];
	mk_field( metp, metp, 0, 0, row+6, col+51, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_RipxIoctlGetNetInfo_IDX ];
	mk_field( metp, metp, 0, 0, row+7, col+51, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_RipxIoctlCheckSapSource_IDX ];
	mk_field( metp, metp, 0, 0, row+8, col+51, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_RipxIoctlResetRouter_IDX ];
	mk_field( metp, metp, 0, 0, row+9, col+51, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_RipxIoctlDownRouter_IDX ];
	mk_field( metp, metp, 0, 0, row+10, col+51, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_RipxIoctlStats_IDX ];
	mk_field( metp, metp, 0, 0, row+11, col+51, 8, "%8.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIP_RipxIoctlUnknown_IDX ];
	metp2 = &mettbl[ RIPR_RipxIoctlUnknown_IDX ];
	mk_field( metp, metp2, 0, 0, row+12, col+51, 8, "%8.0f", metp->metval->cooked, metp2->metval->cooked, NULL );
}

/*
 *	function: 	print_netware
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the netware networking summary subscreen
 */
void
print_netware(int row, int col ) {
	struct metric *metp, *metp2;	

	if( need_header & METS ) {
		mk_field( NULL, NULL, 0, 0, row, col, 18, gettxt("RTPM:1001", "      SPX:        "), NULL, NULL, print_spx );
		mk_field( NULL, NULL, 0, 0, row, col+19, 20, gettxt("RTPM:1002", "      IPX:         "), NULL, NULL, print_ipx );
		mk_field( NULL, NULL, 0, 0, row, col+40, 19, gettxt("RTPM:1003", "      SAP:         "), NULL, NULL, print_sap );
		mk_field( NULL, NULL, 0, 0, row, col+60, 20, gettxt("RTPM:1004", "      RIP:          "), NULL, NULL, print_rip );
		set_label_color( 0 );
		move( row+1, col+8 );
		printw(gettxt("RTPM:1005","rcv pkt/s          rcv pkt/s            rcv pkt/s           rcv pkt/s"));
		move( row+2, col+8 );
		printw(gettxt("RTPM:1006","  bad pkts           drvrecho/s           GSQ pkt/s           rout req/s"));
		move( row+3, col+8 );
		printw(gettxt("RTPM:1007","  bad data           brodcast/s           GSR pkt/s           rout rsp/s"));
		move( row+4, col+8 );
		printw(gettxt("RTPM:1008","  dup pkts           rip pkt/s            NSQ pkt/s           coalesced"));
		move( row+5, col+8 );
		printw(gettxt("RTPM:1009","  sentup/s           sap pkt/s            lcl adv/s           bad len"));
		move( row+6, col+8 );
		printw(gettxt("RTPM:1010","snt pkt/s            diag pkt/s           lcl chg/s           no lan"));
		move( row+7, col+8 );
		printw(gettxt("RTPM:1011","  timeouts           propagat/s           lcl shm/s         snt pkt/s"));
		move( row+8, col+8 );
		printw(gettxt("RTPM:1012","  rcv_naks         snt pkt/s              outlan/s            req pkt/s"));
		move( row+9, col+8 );
		printw(gettxt("RTPM:1013","snt msg/s            to lans/s            RIP pkt/s           resp pkt/s"));
		move( row+10, col+8 )
;
		printw(gettxt("RTPM:1014","  unk msgs           queued/s           snt pkt/s             lan0rout/s"));
		move( row+11, col+8 );
		printw(gettxt("RTPM:1015","  bad msgs           tot ISM/s            GSQ pkt/s           lan0drop/s"));
		move( row+12, col+8 );
		printw(gettxt("RTPM:1016","conn req/s           netbios/s            GSR pkt/s           bad dest"));
		move( row+13, col+8 );
		printw(gettxt("RTPM:1017","  aborts           ioctl/s                NSR pkt/s         ioctl/s"));
	}

	metp = &mettbl[ SPXR_rcv_packet_count_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SPX_rcv_bad_packet_IDX ];
	metp2 = &mettbl[ SPXR_rcv_bad_packet_IDX ];
	mk_field( metp, metp2, 0, 0, row+2, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPX_rcv_bad_data_packet_IDX ];
	metp2 = &mettbl[ SPXR_rcv_bad_data_packet_IDX ];
	mk_field( metp, metp2, 0, 0, row+3, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPX_rcv_dup_packet_IDX ];
	metp2 = &mettbl[ SPXR_rcv_dup_packet_IDX ];
	mk_field( metp, metp2, 0, 0, row+4, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SPXR_rcv_packet_sentup_IDX ];
	mk_field( metp, metp, 0, 0, row+5, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SPXR_send_packet_count_IDX ];
	mk_field( metp, metp, 0, 0, row+6, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SPX_send_packet_timeout_IDX ];
	metp2 = &mettbl[ SPXR_send_packet_timeout_IDX ];
	mk_field( metp, metp2, 0, 0, row+7, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPX_send_packet_nak_IDX ];
	metp2= &mettbl[ SPXR_send_packet_nak_IDX ];
	mk_field( metp, metp2, 0, 0, row+8, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPXR_send_mesg_count_IDX ];
	mk_field( metp, metp, 0, 0, row+9, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SPX_unknown_mesg_count_IDX ];
	metp2 = &mettbl[ SPXR_unknown_mesg_count_IDX ];
	mk_field( metp, metp2, 0, 0, row+10, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPX_send_bad_mesg_IDX ];
	metp2 = &mettbl[ SPXR_send_bad_mesg_IDX ];
	mk_field( metp, metp2, 0, 0, row+11, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ SPXR_rcv_conn_req_IDX ];
	mk_field( metp, metp, 0, 0, row+12, col, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SPX_abort_connection_IDX ];
	metp2= &mettbl[ SPXR_abort_connection_IDX ];
	mk_field( metp, metp2, 0, 0, row+13, col, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ IPXLANR_InTotal_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ IPXLANR_InDriverEcho_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ IPXLANR_InBroadcast_IDX ];
	mk_field( metp, metp, 0, 0, row+3, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ IPXLANR_InRip_IDX ];
	mk_field( metp, metp, 0, 0, row+4, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ IPXLANR_InSap_IDX ];
	mk_field( metp, metp, 0, 0, row+5, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ IPXLANR_InDiag_IDX ];
	mk_field( metp, metp, 0, 0, row+6, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ IPXLANR_InPropagation_IDX ];
	mk_field( metp, metp, 0, 0, row+7, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ IPXLANR_OutTotal_IDX ];
	mk_field( metp, metp, 0, 0, row+8, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ IPXLANR_OutSent_IDX ];
	mk_field( metp, metp, 0, 0, row+9, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ IPXLANR_OutQueued_IDX ];
	mk_field( metp, metp, 0, 0, row+10, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ IPXLANR_OutTotalStream_IDX ];
	mk_field( metp, metp, 0, 0, row+11, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ IPXLANR_OutPropagation_IDX ];
	mk_field( metp, metp, 0, 0, row+12, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ IPXLANR_Ioctl_IDX ];
	mk_field( metp, metp, 0, 0, row+13, col+19, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAPR_TotalInSaps_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ SAPR_GSQReceived_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ SAPR_GSRReceived_IDX ];
	mk_field( metp, metp, 0, 0, row+3, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ SAPR_NSQReceived_IDX ];
	mk_field( metp, metp, 0, 0, row+4, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ SAPR_SASReceived_IDX ];
	mk_field( metp, metp, 0, 0, row+5, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ SAPR_SNCReceived_IDX ];
	mk_field( metp, metp, 0, 0, row+6, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ SAPR_GSIReceived_IDX ];
	mk_field( metp, metp, 0, 0, row+7, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ SAPR_NotNeighbor_IDX ];
	mk_field( metp, metp, 0, 0, row+8, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ SAPR_TotalInRipSaps_IDX ];
	mk_field( metp, metp, 0, 0, row+9, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ SAPR_TotalOutSaps_IDX ];
	mk_field( metp, metp, 0, 0, row+10, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ SAPR_GSQSent_IDX ];
	mk_field( metp, metp, 0, 0, row+11, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ SAPR_GSRSent_IDX ];
	mk_field( metp, metp, 0, 0, row+12, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );
	metp = &mettbl[ SAPR_NSRSent_IDX ];
	mk_field( metp, metp, 0, 0, row+13, col+40, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_ReceivedPackets_IDX ];
	mk_field( metp, metp, 0, 0, row+1, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );


	metp = &mettbl[ RIPR_ReceivedRequestPackets_IDX ];
	mk_field( metp, metp, 0, 0, row+2, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_ReceivedResponsePackets_IDX ];
	mk_field( metp, metp, 0, 0, row+3, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_ReceivedCoalesced_IDX ];
	mk_field( metp, metp, 0, 0, row+4, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIP_ReceivedBadLength_IDX ];
	metp2 = &mettbl[ RIPR_ReceivedBadLength_IDX ];
	mk_field( metp, metp2, 0, 0, row+5, col+60, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ RIP_ReceivedNoLanKey_IDX ];
	metp2 = &mettbl[ RIPR_ReceivedNoLanKey_IDX ];
	mk_field( metp, metp2, 0, 0, row+6, col+60, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ RIPR_total_router_packets_sent_IDX ];
	mk_field( metp, metp, 0, 0, row+7, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_SentRequestPackets_IDX ];
	mk_field( metp, metp, 0, 0, row+8, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_SentResponsePackets_IDX ];
	mk_field( metp, metp, 0, 0, row+9, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIPR_SentLan0Routed_IDX ];
	mk_field( metp, metp, 0, 0, row+10, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );

	metp = &mettbl[ RIP_SentLan0Dropped_IDX ];
	metp2 = &mettbl[ RIPR_SentLan0Dropped_IDX ];
	mk_field( metp, metp2, 0, 0, row+11, col+60, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ RIP_SentBadDestination_IDX ];
	metp2 = &mettbl[ RIPR_SentBadDestination_IDX ];
	mk_field( metp, metp2, 0, 0, row+12, col+60, 7, "%7.0f", metp->metval->cooked, metp2->metval->cooked, NULL );

	metp = &mettbl[ RIPR_ioctls_processed_IDX ];
	mk_field( metp, metp, 0, 0, row+13, col+60, 7, "%7.0f", metp->metval->cooked, metp->metval->cooked, NULL );


}

/*
 *	function: 	print_help
 *
 *	args:		row and column
 *
 *	ret val:	none
 *
 *	print the help subscreen
 */
void
print_help( row, col ) {
	set_default_color( 0 );
	col += 5;
	move( row++, col );
	printw(gettxt("RTPM:14", "Cursor motions: <cursor keys> or h,j,k,l, or ^P,^N,^B,^F."));
	move( row++, col );
	printw(gettxt("RTPM:15", "Plotting:       <space> plots the current field, c clears displays."));
	move( row++, col );
	printw(gettxt("RTPM:16", "Bargraph:       b toggles display of the %%cpu bar graph."));
	move( row++, col );
	printw(gettxt("RTPM:17", "                On small screens, ^ and v scroll the bar graph."));
	move( row++, col );
	printw(gettxt("RTPM:18", "Sub Screens:    <space> on header line displays detailed metrics."));
	move( row++, col );
	printw(gettxt("RTPM:19", "                < and > scroll metrics and ps data on small screens."));
	move( row++, col );
	printw(gettxt("RTPM:20", "Return Screen:  <esc> returns to the previous screen."));
	move( row++, col );
	printw(gettxt("RTPM:21", "Plock:          x toggles whether %s is locked in memory."),progname);
	move( row++, col );
	printw(gettxt("RTPM:22", "Interval:       + and - change the sampling interval."));
	move( row++, col );
	printw(gettxt("RTPM:23", "Options:        p changes the plotting format. u,s,a display"));
	move( row++, col );
	printw(gettxt("RTPM:24", "                user, system, and all procs on the LWP screen."));
	move( row++, col );
	printw(gettxt("RTPM:25", "Refresh:        ^L refreshes the screen."));
	move( row++, col );
	printw(gettxt("RTPM:26", "Exit:           q or ^D exits %s."),progname);

	mk_field( NULL, NULL, 0, 0, scr_rows-3, col, 17, gettxt("RTPM:27", "<esc> to continue"), NULL, NULL, NULL );
}
#ifdef DEBUG
dump_plotdata( char *str ) {
	struct plotdata *pd;
	int pcnt = 0;

	fprintf(stderr,"plotdata dump: %s\n",str);
	for( pd = plotdata; pd ; pd=pd->nxt, pcnt++ ) {
		fprintf(stderr,"plotdata entry:%d\n",pcnt);
		fprintf(stderr,"mp->title:%s\n",pd->mp?pd->mp->title:"null");
		fprintf(stderr,"pd->title:%s\n",pd->title);
		fprintf(stderr,"zrow:%d zcol:%d\n",pd->zrow,pd->zcol);
		fprintf(stderr,"rows:%d cols:%d\n",pd->rows,pd->cols);
		fprintf(stderr,"plotrows:%d plotcols:%d\n",pd->plotrows,pd->plotcols);
		fprintf(stderr,"barg_flg:%d digits:%d\n",pd->barg_flg,pd->digits);
		fprintf(stderr,"yincr:%f max:%f\n",pd->yincr,pd->maxx);
		fprintf(stderr,"yscale:%f offset:%d\n",pd->yscale,pd->offset);
		fprintf(stderr,"r1:%d r2:%d\n",pd->r1, pd->r2);
	}
}
#endif
