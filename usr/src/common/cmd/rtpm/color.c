#ident	"@(#)rtpm:color.c	1.6"

/*
 * color.c:	colorize rtpm output
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
#include <sys/dl.h>
#include <sys/param.h>
#include "rtpm.h"

/*
 * color range compare operators
 */
#define NOP	0
#define EQ	1
#define LT	2
#define LE	LT|EQ
#define GT	4
#define GE	GT|EQ
#define NE	8

/*
 * the default rtpm initialization files
 */
#define SYS_COLOR_FILE	gettxt("RTPM:632", "/etc/.rtpmrc")
#define COLOR_FILE	gettxt("RTPM:633", ".rtpmrc")
char *filename;

/*
 * id numbers for color pairs
 */
#define BLACK_PAIR	1
#define REV_BLACK_PAIR	2
#define RED_PAIR	3
#define REV_RED_PAIR	4
#define GREEN_PAIR	5
#define REV_GREEN_PAIR	6
#define YELLOW_PAIR	7
#define REV_YELLOW_PAIR	8
#define BLUE_PAIR	9
#define REV_BLUE_PAIR	10
#define MAGENTA_PAIR	11
#define REV_MAGENTA_PAIR	12
#define CYAN_PAIR	13
#define REV_CYAN_PAIR	14
#define WHITE_PAIR	15
#define REV_WHITE_PAIR	16

/*
 * default colors of various things
 */
int background_color = COLOR_BLACK;
int header_color = COLOR_WHITE;
int header_pair = WHITE_PAIR;
int stdout_hdr_pair = REV_WHITE_PAIR;
int label_color = COLOR_WHITE;
int label_pair = WHITE_PAIR;
int stdout_lbl_pair = REV_WHITE_PAIR;
int message_color = COLOR_WHITE;
int message_pair = WHITE_PAIR;
int stdout_msg_pair = REV_WHITE_PAIR;
int default_color = COLOR_WHITE;
int default_pair = WHITE_PAIR;
int stdout_dft_pair = REV_WHITE_PAIR;
int plot_color = COLOR_WHITE;
int plot_pair = WHITE_PAIR;
int stdout_plt_pair = REV_WHITE_PAIR;
/*
 * flag indicates whether the terminal supports underline
 */
int does_underline = 0;
/*
 * flag indicates whether the terminal supports color
 */
int does_color = 0;
/*
 * locale of .rtpmrc file
 */
char *chg_locale = NULL;
/*
 * flag indicates whether we need to get parse tokens from catalog
 */
static int need_tokens = 1;
/*
 * parsing tokens 
 */
#define NUMBER	1
#define OPERATOR 2
#define COLOR 3
#define SEPARATOR 4
#define EOL 5
#define PLOT 6
#define INVERT 7
#define INUMBER 8
#define LEFT_PAREN 9
#define RIGHT_PAREN 10
#define COMMA 11
#define ASTERISK 12
#define TOTAL 13
#define UNKNOWN 14

struct color_range {	/* holds one expr of the form: a opa color opb b */
	struct color_range *nxt;	/* next color range for this met */
	int opa;			/* ==, !=, <, <=, >, >=, or NOP	 */
	float a;			/* left range limit		 */
	int color;			/* color identifier		 */
	int opb;			/* ==, !=, <, <=, >, >=, or NOP	 */
	float b;			/* right range limit		 */
};

/*
 * a color range for the bargraph
 */
struct color_range *bargraph_color = NULL;

/*
 * A struct to hold things against which input strings are compared.
 * The main purpose of this is to allow sorting of things by length
 * so we don't mistakingly take a substring as a complete match.
 */
struct inpt {
	int code;
	char *str;
	int len;
};

/*
 * operators
 */
struct inpt ops[] = {
	{ EQ, NULL, 0 },
	{ NE, NULL, 0 },
	{ LT, NULL, 0 },
	{ GT, NULL, 0 },
	{ LE, NULL, 0 },
	{ GE, NULL, 0 },
};
#define NOPS (sizeof( ops )/sizeof( struct inpt ))

/*
 * colors
 */
struct inpt colors[] = {
	{ COLOR_BLACK, NULL, 0 },
	{ COLOR_RED, NULL, 0 },
	{ COLOR_GREEN, NULL, 0 },
	{ COLOR_YELLOW, NULL, 0 },
	{ COLOR_BLUE, NULL, 0 },
	{ COLOR_MAGENTA, NULL, 0 },
	{ COLOR_CYAN, NULL, 0 },
	{ COLOR_WHITE, NULL, 0 },
};
#define NCOLORS (sizeof( colors )/sizeof( struct inpt ))
/*
 * titles and indices extracted from mettbl, used to identify metrics
 * by name (for color assignment).
 */
struct inpt *mets = NULL;
/*
 * the metric table
 */
extern struct metric mettbl[];		/* metric table */
/*
 * the number of metrics in mettbl
 */
extern int nmets;
/*
 * flag if we are in curses
 */
extern int in_curses;
/*
 * number of rows and cols on the screen
 */
extern int scr_rows, scr_cols;

/*
 * flag to only accept integer numbers as tokens
 */
int number_kludge = 0;

/*
 * functions that are internal to color.c
 */
void init_op_inpt( void );
void init_color_inpt( void );	
void init_met_inpt( void );
void sort_inpt( struct inpt *inpt, int n );
struct color_range *malloc_stuff( struct color_range **cpp, struct color_range *cp, double a, int opa, int color, int opb, double b );
int parse_range( char *p, struct color_range **cpp, struct metric *mp );
int atocol( char *p );	
int atoop( char *p );	
int get_decpt( void );
int get_tok( char **pp, char **tok_p );
void init_color_pairs( void );
int set_pair( int color );
int set_rev_pair( int color );
int color_test( double a, int opa, double m, int opb, double b );
int get_color( double met, struct color_range *r, int pflg );
void color_parse( FILE *fp );
void add_plot( struct metric *mp, int r1, int r2, int ndim, int dimx, int dimy );
int check_lang( char *p );
void change_locale( char *loc );

/*
 *	function: 	get_color
 *
 *	args:		metric, color range, and plotflag
 *
 *	ret val:	color
 *
 *	get color evaluates the metric according to color range
 *	and returns the color associated with the metric value.
 *	plotflag specifies whether to return the default plot color
 *	if no other matching condition is found.
 */
int
get_color( double met, struct color_range *r, int pflg ) {
	for( ; r ; r = r->nxt ) {
		if( color_test( (double)r->a, r->opa, (double)met, r->opb, (double)r->b ) ) {
			return( r->color );
		}
	}
	if( pflg )
		return( plot_color );
	return( default_color );
}

/*
 *	function: 	color_test
 *
 *	args:		left_limit, operator, metric, operator, right limit
 *
 *	ret val:	TRUE/FALSE
 *
 *	color_test returns:
 *
 *		left OPA metric && metric OPB right
 *
 *	where left, metric, and right are numerics and OPA/OPB
 *	are relational operators or NOP.
 */
color_test( double a, int opa, double m, int opb, double b ) {
	int e1, e2;	
	switch( opa ) {
	case NE:
		e1 = ( a != m );
		break;
	case EQ:
		e1 = ( a == m );
		break;
	case LT:
		e1 = ( a < m );
		break;
	case LE:
		e1 = ( a <= m );
		break;
	case GT:
		e1 = ( a > m );
		break;
	case GE:
		e1 = ( a >= m );
		break;
	case NOP:
		e1 = 1;
		break;
	default:
#ifdef DEBUG
		if( in_curses )
			endwin();
		fprintf(stderr, "DEBUG: unknown color operator\n");
		exit(1);
#endif
		break;
	}
	switch( opb ) {
	case NE:
		e2 = ( b != m );
		break;
	case EQ:
		e2 = ( b == m );
		break;
	case LT:
		e2 = ( m < b );
		break;
	case LE:
		e2 = ( m <= b );
		break;
	case GT:
		e2 = ( m > b );
		break;
	case GE:
		e2 = ( m >= b );
		break;
	case NOP:
		e2 = 1;
		break;
	default:
#ifdef DEBUG
		if( in_curses )
			endwin();
		fprintf(stderr, "DEBUG: unknown color operator\n");
		exit(1);
#endif
		break;
	}
	return( e1 && e2 );
}

/*
 *	function: 	init_rtpmcolor
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	init_rtpmcolor checks to see if color is supported.
 *	If color is supported, it parses the .rtpmrc file
 *	for color assignments and sets up color pairs for
 *	each of the colors against the specified background 
 *	color.
 */
void
init_rtpmcolor() {

	FILE *fp;
	char color_file[MAXPATHLEN + 1];
	char *home;

	does_underline = termattrs() & A_UNDERLINE;
	does_color = has_colors();

	home = getenv( gettxt("RTPM:634", "HOME") );
	if( !home || !*home )
		home = getenv( "HOME" );
	strcpy( color_file, home );
	strcat( color_file, "/" );
	strcat( color_file, COLOR_FILE );

	fp = fopen( color_file, "r" );
	filename = color_file;
	if( !fp ) {
		fp = fopen( SYS_COLOR_FILE, "r" );
		filename = SYS_COLOR_FILE;
	}
	if( !fp ) {
		init_color_pairs();
		return;
	}
	change_locale( "" );
/*
 *	parse the .rtpmrc file
 */
	color_parse( fp );
	free( mets );
	fclose( fp );
/*
 *	start up color and init a bunch of color pairs
 */
	start_color();
	init_color_pairs();
	if( chg_locale ) {
		free( chg_locale );
		setlocale( LC_ALL, "" );
		set_met_titles();
	}	
	return;
}

/*
 *	function: 	parse_color
 *
 *	args:		file pointer
 *
 *	ret val:	none
 *
 *	parse_color reads the .rtpmrc file and searchs for lines
 *	that begin with either a metric name followed by a colon,
 *	or one of: "background:", "headers:", "labels:", "plot:",
 *	"bargraph:", and "default:".
 *		
 *	If one of these is found, the remainder of the line is
 *	parsed as one or more color range expressions.  The syntax
 *	for this looks something like:
 *	
 *		background:	<color>
 *		headers:	<color>
 *		labels:		<color>
 *		plot:		<color>
 *		default:	<color>
 *		bargraph:	<RANGE>[;<RANGE>]*
 *		<metric_name>:	<RANGE>[;<RANGE>]*
 *
 *		<RANGE>::	[<num><OP>]<color>[<OP><num>[<OP><color>]]*
 *		<OP>::		< | <= | > | >= | == | !=
 *		<color>::	black | red | green | yellow | blue | cyan
 *				  magenta | white
 *		<num>::		<digit>[<digit>]*[.<digit>[<digit>]*]
 *		<digit>::	0|1|2|3|4|5|6|7|8|9
 */	
void
color_parse( FILE *fp ) {
	char buf[1024];
	char *title;
	struct metric *mp;
	char *p;
	int  i;
	int line = 0;
	int errcnt = 0;
	char *colon = gettxt("RTPM:635", ":");

	while( fgets( buf, 1024, fp ) ) {
		line++;
		p = buf;
		if( check_lang( p ) ) {
			colon = gettxt("RTPM:635", ":");
			continue;
		}
		while( *p && *p != '\n' && *p != *colon )
			p++;
		if( *p != *colon )
			continue;
		*p++ = '\0';
		title = buf;

		while( isspace( *p ) )
			p++;

		if( !strcmp( title, gettxt("RTPM:636", "background") ) ) {
			if( ( background_color = atocol( p ) ) == -1 )
				background_color = COLOR_BLACK;
		}

		if( !strcmp( title, gettxt("RTPM:637", "labels") ) ) {
			if( ( label_color = atocol( p ) ) == -1 )
				label_color = COLOR_WHITE;
		}

		if( !strcmp( title, gettxt("RTPM:638", "headers") ) ) {
			if( ( header_color = atocol( p ) ) == -1 )
				header_color = COLOR_WHITE;
		}

		if( !strcmp( title, gettxt("RTPM:639", "messages") ) ) {
			if( ( message_color = atocol( p ) ) == -1 )
				message_color = COLOR_WHITE;
		}

		if( !strcmp( title, gettxt("RTPM:640", "default") ) ) {
			if( ( default_color = atocol( p ) ) == -1 )
				default_color = COLOR_WHITE;
		}

		if( !strcmp( title, gettxt("RTPM:641", "plot") ) ) {
			if( ( plot_color = atocol( p ) ) == -1 )
				plot_color = COLOR_WHITE;
		}

		if( !strcmp( title, gettxt("RTPM:642", "bargraph") ) ) {
			if( parse_range( p, &bargraph_color, NULL ) ) {
				if( chg_locale )
					setlocale( LC_ALL, "" );
				fprintf(stderr,gettxt("RTPM:666", "ignoring error in .rtpmrc file:%s line:%d\n\r"),filename,line);
				if( chg_locale )
					setlocale( LC_ALL, chg_locale );
			}
		}

		for( i = 0 ; i < nmets ; i++ )
			if( !strcmp( mets[i].str, title ) )
				break;
		if( i == nmets )
			continue;

		mp = &mettbl[mets[i].code];
		if( parse_range( p, &mp->color, mp ) )
			if( errcnt++ < 10 ) {
				if( chg_locale )
					setlocale( LC_ALL, "" );
				fprintf(stderr,gettxt("RTPM:666", "ignoring error in .rtpmrc file:%s line:%d\n\r"),filename,line);
				if( chg_locale )
					setlocale( LC_ALL, chg_locale );
			}
	}
	if( errcnt )
		sleep( 3 );
}

/*
 *	function: 	init_op_inpt
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	init_op_inpt sets the strings, codes, and string lengths for
 *	parsing the relational operators.
 */
void
init_op_inpt() {
	int i;

	ops[0].code = EQ;
	ops[1].code = NE;
	ops[2].code = LT;
	ops[3].code = GT;
	ops[4].code = LE;
	ops[5].code = GE;
	ops[0].str = gettxt("RTPM:643", "==");
	ops[1].str = gettxt("RTPM:644", "!=");
	ops[2].str = gettxt("RTPM:645", "<");
	ops[3].str = gettxt("RTPM:646", ">");
	ops[4].str = gettxt("RTPM:647", "<=");
	ops[5].str = gettxt("RTPM:648", ">=");

	for( i = 0 ; i < NOPS ; i++ )
		ops[i].len = strlen( ops[i].str );
}

/*
 *	function: 	init_color_inpt
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	init_color_inpt sets the strings, codes, and string lengths for
 *	parsing the color input.
 */
void
init_color_inpt() {
	int  i;

	colors[0].code = COLOR_BLACK;
	colors[1].code = COLOR_RED;
	colors[2].code = COLOR_GREEN;
	colors[3].code = COLOR_YELLOW;
	colors[4].code = COLOR_BLUE;
	colors[5].code = COLOR_MAGENTA;
	colors[6].code = COLOR_CYAN;
	colors[7].code = COLOR_WHITE;
	colors[0].str = gettxt("RTPM:649", "black");
	colors[1].str = gettxt("RTPM:650", "red");
	colors[2].str = gettxt("RTPM:651", "green");
	colors[3].str = gettxt("RTPM:652", "yellow");
	colors[4].str = gettxt("RTPM:653", "blue");
	colors[5].str = gettxt("RTPM:654", "magenta");
	colors[6].str = gettxt("RTPM:655", "cyan");
	colors[7].str = gettxt("RTPM:656", "white");

	for( i = 0 ; i < NCOLORS ; i++ )
		colors[i].len = strlen( colors[i].str );
}	

/*
 *	function: 	init_met_inpt
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	init_met_inpt sets the strings, codes, and string lengths for
 *	parsing the metric names input.
 */
void
init_met_inpt() {
	int i;

	if( mets )
		free( mets );
	mets = malloc( sizeof( struct inpt ) * nmets );
	if( !mets ) {
		if( in_curses )
			endwin();
		if( chg_locale )
			setlocale( LC_ALL, "" );
		fprintf(stderr, gettxt("RTPM:7", "out of memory\n"));
		exit(1);
	}
	for( i = 0 ; i < nmets ; i++ ) {
		mets[i].code = i;
		mets[i].str = mettbl[i].title;
		mets[i].len = strlen(mets[i].str);
	}
}

/*
 *	function: 	sort_inpt
 *
 *	args:		pointer to an array of struct inpts containing 
 *			strings, string lengths and codes, and a count 
 *			of how many entries there are in the array.
 *
 *	ret val:	none
 *
 *	sort_inpt does a bubble sort on the array of struct inpts, putting
 *	things with the longest string lengths first.
 */
void
sort_inpt( struct inpt *inpt, int n ) {
	int i, j, tmp;
	char *tmpstr;

	do {
		j = 0;
		for( i = 0 ; i < (n-1) ; i++ ) {
			if( inpt[i].len < inpt[i+1].len ) {
				tmp = inpt[i+1].len;
				inpt[i+1].len = inpt[i].len;
				inpt[i].len = tmp;
				tmp = inpt[i+1].code;
				inpt[i+1].code = inpt[i].code;
				inpt[i].code = tmp;
				tmpstr = inpt[i+1].str;
				inpt[i+1].str = inpt[i].str;
				inpt[i].str = tmpstr;
				j = 1;
			}
		}
	} while( j );
}
/*
 *	function: 	malloc_stuff
 *
 *	args:		cpp, a pointer to the color_range pointer of a 
 *			metric in mettbl.  This is the head of the color
 *			list for this metric.  cp, a pointer to the tail 
 *			of the list.  a, opa, color, opb, b - elements of 
 *			a struct color_range.
 *
 *	ret val:	pointer to the newly allocated entry
 *
 *	malloc_stuff creates a new struct color_range and fills it with the
 *	values a, opa, color, opb, and b.  If cpp is NULL, the struct 
 *	becomes the head of a new list, otherwise is is added at the tail.
 */
struct color_range *
malloc_stuff( struct color_range **cpp, struct color_range *cp, double a, int opa, int color, int opb, double b ) {
	if( !(*cpp) ) {
		(*cpp)=malloc(sizeof(struct color_range));
		if( !(*cpp) ) {
			if( in_curses )
				endwin();
			if( chg_locale )
				setlocale( LC_ALL, "" );
			fprintf(stderr,gettxt("RTPM:7","out of memory\n"));
			exit(1);
		}
		cp = *cpp;
	} else {
		assert( cp );
		cp->nxt=malloc(sizeof(struct color_range));
		if( !cp->nxt ) {
			if( in_curses )
				endwin();
			if( chg_locale )
				setlocale( LC_ALL, "" );
			fprintf(stderr,gettxt("RTPM:7", "out of memory\n"));
			exit(1);
		}
		cp = cp->nxt;
	}
	cp->a = a;
	cp->opa = opa;
	cp->color = color;
	cp->opb = opb;
	cp->b = b;
	cp->nxt = NULL;
	return( cp );
}
/*
 *	function: 	parse_range
 *
 *	args:		char pointer to line being parsed and pointer to 
 *			a pointer to the head of a struct color_range list.
 *
 *	ret val:	0 line parsed OK, -1 error was found
 *
 *	parse range parses expressions of the following form:
 *
 *		<RANGE>[;<RANGE>]*
 *		<RANGE>::	[<num><OP>]<color>[<OP><num>[<OP><color>]]*
 *		<OP>::		< | <= | > | >= | == | !=
 *		<color>::	black | red | green | yellow | blue ...
 *		<num>::		<digit>[<digit>]*[<decpt><digit>[<digit>]*]
 *		<digit>::	0|1|2|3|4|5|6|7|8|9
 *		<decpt>::	. | , (depends on language)
 *
 *	This is done with a state machine:
 *
 *		state	meaning		accept_token:next_state
 *		-----	-------		-----------------------
 *		    0	start		number:1  
 *					color:3
 *		    1	have 1st num	op:2
 *		    2	have opa	color:3
 *		    3	have color	op:4	
 *					separator:0 (alloc color_range)
 *					EOL:return  (alloc color_range)
 *		    4	have opb	number:5
 *		    5	have 2nd num	op:2 (alloc, a=b, restart)
 *					separator:0 (alloc color_range)
 *					EOL:return  (alloc color_range)
 */
parse_range( char *p, struct color_range **cpp, struct metric *mp ) {
	float a;
	int opa;
	int color;
	int opb;
	float b;
	char *tok;
	int tok_type;
	int state;
	struct color_range *cp = NULL;
	int r1 = 0;
	int r2 = 0;
	uint32 ndim = 0;
	uint32 dimx = 1;
	uint32 dimy = 1;
	struct plotdata *pd, *tp;
	extern struct plotdata *plotdata;

	if( mp ) {
		ndim = mp->ndim;
		dimx = mp->resval[0]+1;
		dimy = mp->resval[1]+1;
	}

	a = b = -1.0;
	opa = opb = NOP;
	state = 0;
	number_kludge = 0;

	while( tok_type = get_tok( &p, &tok ) ) {
		switch( state ) {
		case 0: /* start */
			switch(tok_type) {
			case NUMBER: case INUMBER:
				a = atof( tok );
				state = 1;
				break;
			case COLOR:
				color = atocol( tok );
				state = 3;
				break;
			case EOL:
				return(0);
				/* NOTREACHED */
				break;
			case INVERT:
				if( !mp || mp->inverted )
					goto err;
				mp->inverted = 1;
				state = 6;
				break;
			case PLOT:
				if( !mp )
					goto err;
				state = 7;
				break;
			default:
				goto err;
				/* NOTREACHED */
				break;
			}
			break;
		case 1: /* have a */
			if( tok_type != OPERATOR ) {
				goto err;
			}
			state = 2;
			opa = atoop( tok );
			break;
		case 2:	/* have opa */
			if( tok_type != COLOR ) {
				goto err;
			}
			state = 3;
			color = atocol( tok );
			break;
		case 3:	/* have color */
			switch( tok_type ) {
		   	case OPERATOR:
				opb = atoop( tok );
				state = 4;
				break;
			case SEPARATOR:
				state = 0;
				cp = malloc_stuff( cpp, cp, (double)a, opa, color, opb, (double)b );
				break;
			case EOL:
				(void)malloc_stuff( cpp, cp, (double)a, opa, color, opb, (double)b );
				return(0);
				/* NOTREACHED */
				break;
			default:
				goto err;
				/* NOTREACHED */
				break;
			}
			break;
		case 4: /* have opb */
			if( tok_type != NUMBER && tok_type != INUMBER ) {
				goto err;
			}
			b = atof( tok );
			state = 5;
			break;
		case 5: /* have b */
			switch ( tok_type ) {
			case OPERATOR:
				cp = malloc_stuff( cpp, cp, (double)a, opa, color, opb, (double)b );
				opa = atoop( tok );
				a = b;
				opb = NOP;
				b = -1.0;
				state = 2;
				break;
			case SEPARATOR:
				cp = malloc_stuff( cpp, cp, (double)a, opa, color, opb, (double)b );
				a = b = -1.0;
				opa = opb = NOP;
				state = 0;
				break;
			case EOL:
				(void)malloc_stuff( cpp, cp, (double)a, opa, color, opb, (double)b );
				return(0);
				/* NOTREACHED */
				break;
			default:
				goto err;
				/* NOTREACHED */
				break;
			}
			break;
		case 6: /* have "invert" keyword or completed plot expr */
			switch( tok_type ) {
			case EOL:
				return( 0 );
				/* NOTREACHED */
				break;
			case SEPARATOR:
				state = 0;
				break;
			default:
				goto err;
				/* NOTREACHED */
				break;
			}
			break;
		case 7:	/* have "plot" key word */
			switch( tok_type ) {
			case SEPARATOR: case EOL:
				add_plot( mp, dimx-1, dimy-1, ndim, dimx, dimy );
				if( tok_type == EOL )
					return(0);
				state = 0;
				break;
			case LEFT_PAREN:
				if( ndim < 1 )
					goto err;
				number_kludge = 1;
				state = 8;
				break;
			default:
				goto err;
			}
			break;
		case 8:	/* have "plot(" */
			switch( tok_type ) {
			case INUMBER:
				r1 = atoi( tok );
				if( r1 >= dimx ){
					r1 = dimx-1;
				}
				state = 11;
				if( ndim == 2 )
					state = 9;
				break;
			case TOTAL:
				r1 = dimx-1;
				state = 11;
				if( ndim == 2 )
					state = 9;
				break;
			case ASTERISK:
				r1 = -1;
				state = 11;
				if( ndim == 2 )
					state = 9;
				break;
			default:
				goto err;
				/* NOTREACHED */
				break;
			}
			break;
		case 9:
			if( tok_type != COMMA )
				goto err;
			state = 10;
			break;
		case 10:
			switch( tok_type ) {
			case INUMBER:
				r2 = atoi( tok );
				if( r2 >= dimy ){
					r2 = dimy-1;
				}
				state = 11;
				break;
			case TOTAL:
				r2 = dimy-1;
				state = 11;
				break;
			case ASTERISK:
				r2 = -1;
				state = 11;
				break;
			default:
				goto err;
				/* NOTREACHED */
				break;
			}
			break;
		case 11:
			if( tok_type != RIGHT_PAREN )
				goto err;
			add_plot( mp, r1, r2, ndim, dimx, dimy );
			number_kludge = 0;
			state = 6;
			break;
		default:
#ifdef DEBUG
			if( in_curses )
				endwin();
			fprintf(stderr,"DEBUG unexpected state value encountered\n");
			exit(1);
#endif
			return(0);
			/* NOTREACHED */
			break;
		}
	}
/*
 *	An error was found, clear out whatever we've already allocated
 */
err:
	number_kludge = 0;
	if( mp ) {
		mp->inverted = 0;
		for(pd=(struct plotdata *)&plotdata; pd->nxt; pd=pd->nxt ){
			if( pd->nxt->mp == mp ) {
				tp = pd->nxt;
				pd->nxt = pd->nxt->nxt;
				free(tp);
				break;
			}
		}
	}
	while( *cpp ) {
		cp = *cpp;
		*cpp = (*cpp)->nxt;
		free( cp );
	}
	return( -1 );
}
/*
 *	function: 	atocol
 *
 *	args:		char string
 *
 *	ret val:	color associated with the char string or -1
 *
 *	atocol matches char strings against the color input strings
 *	and returns a color identifier.
 */
atocol( char *p ) {
	int i;
	for( i = 0 ; i < NCOLORS ; i++ )
		if( !strncmp( colors[i].str, p, colors[i].len ) )
				return( colors[i].code );
	return( -1 );
}	
/*
 *	function: 	atoop
 *
 *	args:		char string
 *
 *	ret val:	operator id associated with the char string or -1
 *
 *	atocol matches char strings against the operator input strings
 *	and returns a relational operator identifier.
 */
atoop( char *p ) {
	int i;
	for( i = 0 ; i < NOPS ; i++ )
		if( !strncmp( ops[i].str, p, ops[i].len ) )
				return( ops[i].code );
	return( NOP );
}	
/*
 *	function: 	get_decpt
 *
 *	args:		none
 *
 *	ret val:	int representation of a decimal point for 
 *			this locale
 *
 */
get_decpt() {
	char foobuf[16];
	int decpt = '.';
	sprintf( foobuf, "%3.1f", 0.1 );
	if( foobuf[0] == '0' && foobuf[2] == '1' )
		decpt = foobuf[1];
	return(decpt);
}
/*
 *	function: 	get_tok
 *
 *	args:		pointer to char string pointer (input line)
 *			pointer to token string pointer (parsed token)
 *
 *	ret val:	token type
 */
int
get_tok( char **pp, char **tok_p ) {
	int i;
	char *p = *pp;
	static int decpt = 0;
	static char *separator = NULL;
	static char *plot = NULL;
	static char *invert = NULL;
	static char *left_paren = NULL;
	static char *right_paren = NULL;
	static char *comma = NULL;
	static char *asterisk = NULL;
	static char *total = NULL;

	if( need_tokens ) {
		need_tokens = 0;
		decpt = get_decpt();
		separator = gettxt("RTPM:657", ";");
		plot = gettxt("RTPM:660", "plot");
		invert = gettxt("RTPM:661", "invert");
		total = gettxt("RTPM:50", "total");
		left_paren = gettxt("RTPM:662", "(");
		right_paren = gettxt("RTPM:663", ")");
		comma = gettxt("RTPM:664", ",");
		asterisk = gettxt("RTPM:665", "*");
	}

	if( !p )
		return( EOL );

	while( isspace( *p ) ) 
		p++;
	
	/* look for one of: EOL, SEPARATOR, NUMBER, OPERATOR, COLOR, PLOT */

	if( !(*p) || *p == '\n' )
		return( EOL );

	*tok_p = p;

	/* look for number */

	if( isdigit( *p ) ) {
		while( isdigit( *p ) ) 
			p++; 
		if( *p != decpt || number_kludge ) {
			*pp = p;
			return( INUMBER );
		}
		p++;
		while( isdigit( *p ) )
			p++; 
		*pp = p;
		return( NUMBER );
	}

	/* look for operator */

	for( i = 0; i < NOPS ; i++ ) {
		if( !strncmp( p, ops[i].str, ops[i].len ) ) {
			*pp = p + ops[i].len;
			return( OPERATOR );
		}
	}

	/* look for color */

	for( i = 0; i < NCOLORS ; i++ ) {
		if( !strncmp( p, colors[i].str, colors[i].len ) ) {
			*pp = p + colors[i].len;
			return( COLOR );
		}
	}

	/* look for separator */

	if( !strncmp( p, separator, strlen( separator ) ) ) {
		*pp = p + strlen( separator );
		return( SEPARATOR );
	}

	/* look for plot keyword */
	if( !strncmp( p, plot, strlen( plot ) ) ) {
		*pp = p + strlen( plot );
		return( PLOT );
	}

	/* look for invert keyword */

	if( !strncmp( p, invert, strlen( invert ) ) ) {
		*pp = p + strlen( invert );
		return( INVERT );
	}

	/* look for total keyword */

	if( !strncmp( p, total, strlen( total ) ) ) {
		*pp = p + strlen( total );
		return( TOTAL );
	}

	/* look for left paren */

	if( !strncmp( p, left_paren, strlen( left_paren ) ) ) {
		*pp = p + strlen( left_paren );
		return( LEFT_PAREN );
	}

	/* look for right paren */

	if( !strncmp( p, right_paren, strlen( right_paren ) ) ) {
		*pp = p + strlen( right_paren );
		return( RIGHT_PAREN );
	}

	/* look for comma */

	if( !strncmp( p, comma, strlen( comma ) ) ) {
		*pp = p + strlen( comma );
		return( COMMA );
	}

	/* look for asterisk */

	if( !strncmp( p, asterisk, strlen( asterisk ) ) ) {
		*pp = p + strlen( asterisk );
		return( ASTERISK );
	}

	/* didn't recognize anything */

	return( UNKNOWN );
}
/*
 *	function: 	init_color_pairs
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	init_color_pairs sets up color pairs against the requested
 *	background color.  If the requested foreground color is the
 *	same as the background color, the foreground color is changed 
 *	to either white or black, depending on which one contrasts 
 *	better with the background color.
 */
void
init_color_pairs() {
	int not_background_color;

	switch( background_color ) {
	case COLOR_BLACK: case COLOR_GREEN:
	case COLOR_BLUE:
		not_background_color = COLOR_WHITE;
		break;
	case COLOR_RED:	case COLOR_YELLOW: case COLOR_WHITE:
	case COLOR_MAGENTA: case COLOR_CYAN:
		not_background_color = COLOR_BLACK;
		break;
	default:
#ifdef DEBUG
		if( in_curses )
			endwin();

		fprintf(stderr, "DEBUG unknown background color found in init_color_pairs\n");
		exit(1);
		/* NOTREACHED */
#endif
		break;
	}

	if( header_color == background_color )
		header_color = not_background_color;

	if( label_color == background_color )
		label_color = not_background_color;

	if( message_color == background_color )
		message_color = not_background_color;

	if( default_color == background_color )
		default_color = not_background_color;

	if( plot_color == background_color )
		plot_color = not_background_color;

	header_pair = set_pair( header_color );	
	stdout_hdr_pair = set_rev_pair( header_color );	
	label_pair =  set_pair( label_color );	
	stdout_lbl_pair =  set_rev_pair( label_color );	
	message_pair =  set_pair( message_color );	
	stdout_msg_pair =  set_rev_pair( message_color );	
	default_pair =  set_pair( default_color );	
	stdout_dft_pair =  set_rev_pair( default_color );	
	plot_pair =  set_pair( plot_color );
	stdout_plt_pair =  set_rev_pair( plot_color );	

	if( background_color != COLOR_BLACK ) {
		init_pair( BLACK_PAIR, COLOR_BLACK, background_color );
		init_pair( REV_BLACK_PAIR, background_color, COLOR_BLACK );
	}
	else {
		init_pair( BLACK_PAIR, not_background_color, background_color );
		init_pair( REV_BLACK_PAIR, background_color, not_background_color );
	}
	if( background_color != COLOR_RED ) {
		init_pair( RED_PAIR, COLOR_RED, background_color );
		init_pair( REV_RED_PAIR, background_color, COLOR_RED );
	}
	else {
		init_pair( RED_PAIR, not_background_color, background_color );
		init_pair( REV_RED_PAIR, background_color, not_background_color );
	}
	if( background_color != COLOR_BLUE ) {
		init_pair( BLUE_PAIR, COLOR_BLUE, background_color );
		init_pair( REV_BLUE_PAIR, background_color, COLOR_BLUE );
	}
	else {
		init_pair( BLUE_PAIR, not_background_color, background_color );
		init_pair( REV_BLUE_PAIR, background_color, not_background_color );
	}
	if( background_color != COLOR_GREEN ) {
		init_pair( GREEN_PAIR, COLOR_GREEN, background_color );
		init_pair( REV_GREEN_PAIR, background_color, COLOR_GREEN );
	}
	else {
		init_pair( GREEN_PAIR, not_background_color, background_color );
		init_pair( REV_GREEN_PAIR, background_color, not_background_color );
	}
	if( background_color != COLOR_YELLOW ) {
		init_pair( YELLOW_PAIR, COLOR_YELLOW, background_color );
		init_pair( REV_YELLOW_PAIR, background_color, COLOR_YELLOW );
	}
	else {
		init_pair( YELLOW_PAIR, not_background_color, background_color );
		init_pair( REV_YELLOW_PAIR, background_color, not_background_color );
	}
	if( background_color != COLOR_CYAN ) {
		init_pair( CYAN_PAIR, COLOR_CYAN, background_color );
		init_pair( REV_CYAN_PAIR, background_color, COLOR_CYAN );
	}
	else {
		init_pair( CYAN_PAIR, not_background_color, background_color );
		init_pair( REV_CYAN_PAIR, background_color, not_background_color );
	}
	if( background_color != COLOR_MAGENTA ) {
		init_pair( MAGENTA_PAIR, COLOR_MAGENTA, background_color );
		init_pair( REV_MAGENTA_PAIR, background_color, COLOR_MAGENTA );
	}
	else {
		init_pair( MAGENTA_PAIR, not_background_color, background_color );
		init_pair( REV_MAGENTA_PAIR, background_color, not_background_color );
	}
	if( background_color != COLOR_WHITE ) {
		init_pair( WHITE_PAIR, COLOR_WHITE, background_color );
		init_pair( REV_WHITE_PAIR, background_color, COLOR_WHITE );
	}
	else {
		init_pair( WHITE_PAIR, not_background_color, background_color );
		init_pair( REV_WHITE_PAIR, background_color, not_background_color );
	}
}

/*
 *	function: 	set_metric_color
 *
 *	args:		metric value, color_range list, additional attrs -
 *			such as STANDOUT or UNDERLINE, plotflag - are we
 *			plotting the metric or displaying it numerically
 *
 *	ret val:	none
 *
 *	set_metric_color sets the screen color depending on the metric
 *	value and the color range list.  If the attr is A_STANDOUT, the
 *	foreground and background colors are reversed.
 */
void 
set_metric_color( double met, struct color_range *color, int attr, int plotflg ) {
	int pair;

	attrset( 0 );
	if( !does_underline )
		attr &= ~A_UNDERLINE;

	if( !does_color ) {
		attron( attr );
		return;
	}

	switch( get_color( (double)met, color, plotflg ) ) {
	case COLOR_BLACK:
		if( attr == A_STANDOUT )
			pair = REV_BLACK_PAIR;
		else
			pair = BLACK_PAIR;
		break;
	case COLOR_RED:
		if( attr == A_STANDOUT )
			pair = REV_RED_PAIR;
		else
			pair = RED_PAIR;
		break;
	case COLOR_GREEN:
		if( attr == A_STANDOUT )
			pair = REV_GREEN_PAIR;
		else
			pair = GREEN_PAIR;
		break;
	case COLOR_YELLOW:
		if( attr == A_STANDOUT )
			pair = REV_YELLOW_PAIR;
		else
			pair = YELLOW_PAIR;
		break;
	case COLOR_BLUE:
		if( attr == A_STANDOUT )
			pair = REV_BLUE_PAIR;
		else
			pair = BLUE_PAIR;
		break;
	case COLOR_MAGENTA:
		if( attr == A_STANDOUT )
			pair = REV_MAGENTA_PAIR;
		else
			pair = MAGENTA_PAIR;
		break;
	case COLOR_CYAN:
		if( attr == A_STANDOUT )
			pair = REV_CYAN_PAIR;
		else
			pair = CYAN_PAIR;
		break;
	case COLOR_WHITE:
		if( attr == A_STANDOUT )
			pair = REV_WHITE_PAIR;
		else
			pair = WHITE_PAIR;
		break;
	default:
#ifdef DEBUG
		if( in_curses )
			endwin();
		fprintf(stderr, "DEBUG unknown color returned from get_color\n");
		exit(1);
#endif
		pair = default_pair;
		if( attr == A_STANDOUT )
			pair = stdout_dft_pair;

		break;
	}
	if( attr == A_STANDOUT ) {
		attron( COLOR_PAIR( pair ) );
	}
	else {
		attron( attr | COLOR_PAIR( pair ) );
	}

}

/*
 *	function: 	set_message_color
 *
 *	args:		attributes, such as STANDOUT or UNDERLINE
 *
 *	ret val:	none
 *
 *	set_message_color sets the screen color to the message color.
 *	If the attr is A_STANDOUT, the foreground and background colors 
 *	are reversed.
 */
void
set_message_color( int attr ) {

	attrset( 0 );

	if( !does_underline )
		attr &= ~A_UNDERLINE;

	if( !does_color ) {
		attron( attr );
		return;
	}
	if( attr == A_STANDOUT )
		attron( COLOR_PAIR( stdout_msg_pair ) );
	else 
		attron( attr | COLOR_PAIR( message_pair ) );
}


/*
 *	function: 	set_default_color
 *
 *	args:		attributes, such as STANDOUT or UNDERLINE
 *
 *	ret val:	none
 *
 *	set_default_color sets the screen color to the default color.
 *	If the attr is A_STANDOUT, the foreground and background colors 
 *	are reversed.
 */
void
set_default_color( int attr ) {

	attrset( 0 );

	if( !does_underline )
		attr &= ~A_UNDERLINE;

	if( !does_color ) {
		attron( attr );
		return;
	}
	if( attr == A_STANDOUT )
		attron( COLOR_PAIR( stdout_dft_pair ) );
	else
		attron( attr | COLOR_PAIR( default_pair ) );
}


/*
 *	function: 	set_plot_color
 *
 *	args:		attributes, such as STANDOUT or UNDERLINE
 *
 *	ret val:	none
 *
 *	set_plot_color sets the screen color to the default plot color.
 *	If the attr is A_STANDOUT, the foreground and background colors 
 *	are reversed.
 */
void
set_plot_color( attr ) {

	attrset( 0 );
	if( !does_underline )
		attr &= ~A_UNDERLINE;

	if( !does_color ) {
		attron( attr );
		return;
	}
	if( attr == A_STANDOUT )
		attron( COLOR_PAIR( stdout_plt_pair ) );
	else
		attron( attr | COLOR_PAIR( plot_pair ) );
}


/*
 *	function: 	set_label_color
 *
 *	args:		attributes, such as STANDOUT or UNDERLINE
 *
 *	ret val:	none
 *
 *	set_plot_color sets the screen color to the label color.
 *	If the attr is A_STANDOUT, the foreground and background colors 
 *	are reversed.
 */
void
set_label_color( int attr ) {

	attrset( 0 );
	if( !does_underline )
		attr &= ~A_UNDERLINE;

	if( !does_color ) {
		attron( attr );
		return;
	}
	if( attr == A_STANDOUT )
		attron( COLOR_PAIR( stdout_lbl_pair ) );
	else 
		attron( attr | COLOR_PAIR( label_pair ) );
}


/*
 *	function: 	set_header_color
 *
 *	args:		attributes, such as STANDOUT or UNDERLINE
 *
 *	ret val:	none
 *
 *	set_header_color sets the screen color to the subscreen hdr color.
 *	If the attr is A_STANDOUT, the foreground and background colors 
 *	are reversed.
 */
void
set_header_color( int attr ) {

	attrset( 0 );
	if( !does_underline )
		attr &= ~A_UNDERLINE;

	if( !does_color ) {
		attron( attr );
		return;
	}
	if( attr == A_STANDOUT )
		attron( COLOR_PAIR( stdout_hdr_pair ) );
	else 
		attron( attr | COLOR_PAIR( header_pair ) );
}


/*
 *	function: 	color_clear
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	color_clear clears the screen and then fills it with the background
 *	color, since clear() leaves the background black.
 */
void
color_clear() {
	int i, j;

	set_default_color( 0 );
	clear();
	if( !does_color ) 
		return;
	for( i = 0 ; i < scr_rows ; i++ ) {
		move( i, 0 );
		for( j = 0 ; j < scr_cols ; j++ )
			printw(" ");
	}
}


/*
 *	function: 	color_clrtoeol
 *
 *	args:		row and col
 *
 *	ret val:	none
 *
 *	color_clrtoeol clears from the current cursor position (row,col)
 *	to the end of the line (row,scr_cols-1).  It then fills the row
 *	with the background color, (since clrtoeol() leaves it black) 
 *	and moves the cursor back to the starting position.
 */
void
color_clrtoeol( int row, int col ) {
	int i;

	set_default_color( 0 );
	move( row, col );
	clrtoeol();
	if( !does_color )
		return;
	for( i = col ; i < scr_cols ; i++ )
		printw(" ");
	move( row, col );
}


/*
 *	function: 	color_clrtobot
 *
 *	args:		row and col
 *
 *	ret val:	none
 *
 *	color_clrtobot clears from the current cursor position (row,col)
 *	to the bottom of the screen (scr_row-1,scr_cols-1).  It then fills
 *	the cleared area with the background color, (since clrtobot() 
 *	leaves the screen black), and moves the cursor back to the 
 *	starting position.
 */
void
color_clrtobot( int row, int col ) {
	int i, j;

	set_default_color( 0 );
	move( row, col );
	clrtobot();
	if( !does_color )
		return;
	for( i = row ; i < scr_rows ; i++ ) {
		move( i, 0 );
		for( j = 0 ; j < scr_cols ; j++ )
			printw(" ");
	}
	move( row, col );
}

/*
 *	function: 	set_pair
 *
 *	args:		color id
 *
 *	ret val:	pair id
 *
 *	set_pair converts color id numbers to color pair ids.
 */
set_pair( int color ) {
	switch( color ) {
	case COLOR_BLACK:
		return( BLACK_PAIR );
		/* NOTREACHED */
		break;
	case COLOR_RED:
		return( RED_PAIR );
		/* NOTREACHED */
		break;
	case COLOR_BLUE:
		return( BLUE_PAIR );
		/* NOTREACHED */
		break;
	case COLOR_YELLOW:
		return( YELLOW_PAIR );
		/* NOTREACHED */
		break;
	case COLOR_GREEN:
		return( GREEN_PAIR );
		/* NOTREACHED */
		break;
	case COLOR_CYAN:
		return( CYAN_PAIR );
		/* NOTREACHED */
		break;
	case COLOR_MAGENTA:
		return( MAGENTA_PAIR );
		/* NOTREACHED */
		break;
	case COLOR_WHITE:
		return( WHITE_PAIR );
		/* NOTREACHED */
		break;
	}
}


/*
 *	function: 	set_rev_pair
 *
 *	args:		color id
 *
 *	ret val:	pair id
 *
 *	set_pair converts color id numbers to reverse color pair ids.
 */
set_rev_pair( int color ) {
	switch( color ) {
	case COLOR_BLACK:
		return( REV_BLACK_PAIR );
		/* NOTREACHED */
		break;
	case COLOR_RED:
		return( REV_RED_PAIR );
		/* NOTREACHED */
		break;
	case COLOR_BLUE:
		return( REV_BLUE_PAIR );
		/* NOTREACHED */
		break;
	case COLOR_YELLOW:
		return( REV_YELLOW_PAIR );
		/* NOTREACHED */
		break;
	case COLOR_GREEN:
		return( REV_GREEN_PAIR );
		/* NOTREACHED */
		break;
	case COLOR_CYAN:
		return( REV_CYAN_PAIR );
		/* NOTREACHED */
		break;
	case COLOR_MAGENTA:
		return( REV_MAGENTA_PAIR );
		/* NOTREACHED */
		break;
	case COLOR_WHITE:
		return( REV_WHITE_PAIR );
		/* NOTREACHED */
		break;
	}
}
void
add_plot( struct metric *mp, int r1, int r2, int ndim, int dimx, int dimy ) {
	switch( ndim ) {
	case 0:
		(void) setplot( mp, mp->metval->cooked, r1, r2 );
		break;
	case 1:
		if( r1 >= 0 ) {
			(void) setplot( mp, mp->metval[r1].cooked, r1, 0 );
			break;
		}
		for( r1 = 0 ; r1 < dimx; r1++ )
			(void) setplot( mp, mp->metval[r1].cooked, r1, 0 );
		break;
	case 2:
		if( r1 >= 0 ) {
			if( r2 >= 0 ) { /* r1 >= 0 && r2  >= 0 */
				(void) setplot( mp, mp->metval[subscript(r1,r2)].cooked, r1, r2 );
			} else { /* r1 >= 0 && r2 == '*' */
				for( r2 = 0; r2 < dimy; r2++ )			
					(void) setplot( mp, mp->metval[subscript(r1,r2)].cooked, r1, r2 );
			}
		} else {
			if( r2 >= 0 ) { /* r1 == '*' && r2 >= 0 */
				for( r1 = 0; r1 < dimx; r1++ )			
					(void) setplot( mp, mp->metval[subscript(r1,r2)].cooked, r1, r2 );
			} else { /* r1 == '*' and r2 == '*' */
				for( r1 = 0; r1 < dimx; r1++ )
				for( r2 = 0; r2 < dimy; r2++ )		
					(void) setplot( mp, mp->metval[subscript(r1,r2)].cooked, r1, r2 );
			}
		}
		break;
	default:
#ifdef DEBUG
	if( in_curses )
		endwin();
	fprintf(stderr,"DEBUG bad value for ndim found in add_plot\n");
	exit(1);
	/* NOTREACHED */
#endif
		break;
	}
}
/*
 *	function: 	change_locale
 *
 *	args:		locale string
 *
 *	ret val:	none
 *
 *	change_locale changes locale based on arg and resets parse tokens
 */
void
change_locale( char *loc ) {
	assert( loc );
	if( chg_locale )
		free( chg_locale );
	if( *loc ) {
		chg_locale = malloc( strlen( loc ) + 1 );
		if( !chg_locale ) {
			setlocale( LC_ALL, "" );
			fprintf(stderr,gettxt("RTPM:7","out of memory\n"));
			exit(1);
		}
		strcpy( chg_locale, loc );
	}
	setlocale( LC_ALL, loc );
/*
 *	initialize and sort list of operator strings
 */
	init_op_inpt();
	sort_inpt( ops, NOPS );
/*
 *	initialize and sort list of color name strings
 */
	init_color_inpt();
	sort_inpt( colors, NCOLORS );
/*
 *	initialize and sort list of metric name strings
 */
	set_met_titles();
	init_met_inpt();
	sort_inpt( mets, nmets );
	need_tokens = 1;
}
/*
 *	function: 	check_lang
 *
 *	args:		line from .rtpmrc file
 *
 *	ret val:	0: no action taken
 *			1: locale was changed
 *
 *	check_lang checks the line for things like LANG=C and calls
 *	change_locale if such a string is found.
 */
int
check_lang( char *p ) {
	char *q;
	if( !p )
		return( 0 );
	while( isspace(*p) ) 
		p++;
	if( !strncmp( p, "LANG", 4 ) )
		p += 4;
	else if( !strncmp( p, "LC_MESSAGES", 11 ) )
		p += 11;
	else if ( !strncmp( p, "LC_ALL", 6 ) )
		p += 6;
	while( isspace(*p) ) 
		p++;
	if( *p != '=' )
		return(0);
	p++;
	while( isspace(*p) ) 
		p++;
	q = p;
	while( *q && ( isalnum( *q ) || *q == '_' ) )
		q++;
	*q = '\0';
	change_locale( p );
	return( 1 );
}
