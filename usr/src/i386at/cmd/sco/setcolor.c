#ident	"@(#)sco:setcolor.c	1.3.1.2"
/*
 *
 *	Copyright (C) The Santa Cruz Operation, 1985, 1986, 1987, 1988, 1989.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */


/* SETCOLOR - 11/21/83 pr
 *
 * For use with the ibm color adapter to make changes to:
 *	(fore)ground characters and (back)ground space
 *
 * M000 rr 3/17/85
 * 	-switched order in which green and lt_green usage is printed.
 * M001 buckm 7/22/85
 *	- modified usage message some.
 *	- changed write()'s to printf()'s.
 * M002 chapman 12/11/86
 *	- added border colors, beep, and cursor.
 *	- changed the hardware check.
 * M003 sco!davej	Apr 29, 1988
 *	- 2.3.0 kernal has new setcolor escape sequences.
 *	the old escape sequences are still supported, but we would like
 *	to phase them out because they conflict with DOS ANSI.SYS
 *	(and ANSI in general)
 * M004 sco!davej	Apr 29, 1988
 *	Gratuitous code tightening.
 */

char usage[] = "\n\
Usage  :  %9s -[nbrgcto] [argument] [argument]\n\
Options: -n		  Normal white foreground and black background\n\
	 color [color]	  Set foreground and optional background \n\
	 -b color	  Set background \n\
	 -r color color	  Set foreground & background reverse video colors\n\
	 -g color color	  Set foreground & background graphic box colors\n\
	 -o color	  Set border color\n\
	 -c first last	  Set cursor size\n\
	 -p pitch dur	  Set pitch and duration of bell\n\n\
Colors and their names used for [color] options:\n\n";

#include <stdio.h>
#include <ctype.h>
#include <signal.h>

#define GIO_ATTR  ('a'<<8)|0	/* Ioctl call for current attribute */
#define GIO_COLOR ('c'<<8)|0	/* Ioctl call to test for color card */

enum Map_Cmd
{
	SET_NORM,		/* set the normal video color */
	SET_REVERSE,		/* set the reverse video color */
	SET_GRAF,		/* set the 'graphics' color */
};

void set_color(enum Map_Cmd, char *, char *);

/* The array of color strings in order of their index numeric value */

char *color[]={	"black",
		"blue",
		"green",
		"cyan",
		"red",
		"magenta",
		"brown",
		"white",
		"gray",
		"lt_blue",
		"lt_green",
		"lt_cyan",
		"lt_red",
		"lt_magenta",
		"yellow",
		"hi_white"
		};

int initialized = 0;
int cur_attr;				/* Current char mode */
char *progname;

main(argc,argv)
int argc;
char *argv[];
{
	progname = argv[0];
	signal(SIGINT, SIG_IGN);

	if( argc == 1 )
	{	set_color(SET_NORM, "white", "black");
		fixback();
		printf(usage, progname);
		prcolors();

		/* Set mode back to current mode */
		set_color(SET_NORM, color[(cur_attr&0x0F)], 
				color[(cur_attr>>4)&0x0F]);
		fixback();
		exit(0);
	}
	switch( *argv[1] )
	{
		case '-':
			switch( *++argv[1] )
			{
			case 'n':	/* Normal */
				set_color(SET_NORM, "white", "black");
				fixback();
				break;
			case 'b':	/* Set background */
				set_color(SET_NORM,color[cur_attr&0x0F],
					argv[2]);
				fixback();
				break;
			case 'r':	/* Set reverse char color */
				set_color(SET_REVERSE, argv[2], argv[3]);
				break;
			case 'g':	/* Set graphic char mode */
				set_color(SET_GRAF, argv[2], argv[3]);
				break;
			case 'o':
				set_border(argv[2]);
				break;
			case 'c':
				set_cursor(argv[2], argv[3]);
				break;
			case 'p':
				set_bell(argv[2], argv[3]);
				break;
			default: 
				fatal("Unrecognized option\n");
			}
			break;
		default:
			if( isalpha(*argv[1]) )
			{
				if( argc == 2 )
				{	set_color(SET_NORM, argv[1],
					  color[(cur_attr>>4)&0x0F] );
					fixback();
				}
				else if( argc == 3 )
				{	
					set_color(SET_NORM, argv[1], argv[2]);
					fixback();
				}
				else 
				{
					fatal("Improper number of args\n");
				}
			}
			else	
			{
				fatal("Invalid string\n");
			}
	}

	reset();
	exit(0);
}

void
set_color(command, forecolor, backcolor)
enum Map_Cmd command;
char *forecolor,*backcolor;
{
	int fc, bc;
	char cmd_ch;

	fc = validate(forecolor);
	bc = validate(backcolor);
	
	if(fc == bc)
	{
		fatal("Foreground and Background cannot be the same\n");
	}

	init();
	switch(command)		/* vvv M003*/
	{
		case SET_NORM:
			cmd_ch= 'F';
			break;

		case SET_REVERSE:
			cmd_ch= 'H';
			break;

		case SET_GRAF:
			cmd_ch= 'J';
			break;
	}

/* this takes advantage of the fact that the command letters for setting the
   background colors are one more that the foreground color.
*/
	printf("\033[=%d%c", fc, cmd_ch);
	printf("\033[=%d%c", bc, cmd_ch+1);
/* change mode back to normal to force normal attributes to be set */
	 printf("\033[0m");
				/* ^^^ M003 */
}

/*  Set the border color */

set_border(color)
char *color;
{
	int bc;

	bc = validate(color);
	init();
	printf("\033[=%dA", bc);
}

/* set the bell period and duration */

set_bell(period, dur)
char *period, *dur;
{
	int p, d;

	p = num_validate(period);
	d = num_validate(dur);

	init();
	printf("\033[=%d;%dB", p, d);
}

/* set the cursor size */

set_cursor(first, last)
char *first, *last;
{
	int f, l;

	f = num_validate(first);
	l = num_validate(last);

	init();
	printf("\033[=%d;%dC", f, l);
}

/* Compare string to possible colors strings */
/* Return index if ok, or error message and exit if not */

validate(colorname)  
char *colorname;
{
	register int i;

	for(i=0; i<16; ++i)
	{
		if(strcmp(colorname, color[i]) == 0)
			return(i);
	}
	fatal("Invalid color name\n");
}

/* Validate and convert a numaric string argument into an int */
/* error and exit() if any non-digit characters are found */

int
num_validate(number)
char *number;
{
	int i;

	i = atoi(number);

	while('\0' != *number)
	{	if(*number<'0' || *number>'9')
			fatal("Invalid numeric argument\n");
		++number;
	}
	return(i);
}
	

clrblink()	/* Clear blink bit to allow 16 background colors */
{
	printf("\033[=0E");
}
reset()	/* Turn on, then off reverse to set mode */
{
	printf("\033[7m");
	printf("\033[m");
}
fixback()	/* Clear from current cursor position to end of screen */
		/* This will set the rest of the screen to the new back */
{
	printf("\033[J");
}

color_patch(color)
char *color;
{
	set_color(SET_NORM, "black", color);
	printf("      ");
	set_color(SET_NORM, color, "black");
	printf(" %-11s", color);
}

prcolors()		/* Display possible colors and names */
{

		/* Row 1 */

	color_patch("blue");		/* vvv M004*/
	color_patch("magenta");
	color_patch("brown");

	set_color( SET_NORM,"white", "black");
	printf("[    ] black\n\n");

		/* Row 2 */

	color_patch("lt_blue");
	color_patch("lt_magenta");
	color_patch("yellow");
	color_patch("gray");
	printf("\n\n");

		/* Row 3 */

	color_patch("cyan");
	color_patch("white");
	color_patch("green");
	color_patch("red");
	printf("\n\n");

		/* Row 4 */

	color_patch("lt_cyan");
	color_patch("hi_white");
	color_patch("lt_green");
	color_patch("lt_red");
	printf("\n\n");			/* ^^^ M004*/
}


fatal(str, a, b, c, d, e)
char *str;
{
	fprintf(stderr, "%s: ", progname);
	fprintf(stderr, str, a, b, c, d, e);
	exit(1);
}

init()
{
	if (initialized) return;
	initialized = 1;
	printf("\033[0m");		/* start with norm attr's */

	/* Get current char mode */
	cur_attr = ioctl(0,GIO_ATTR,0);
	if(-1 == cur_attr)
	{	cur_attr = 0x0007;	/* white on black */
	}

	/* Allow all 16 background colors */
	clrblink();
}

