/*		copyright	"%c%" 	*/

#ident	"@(#)sttyparse.c	1.6"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <termio.h>
#include <sys/termiox.h>
#include "stty.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>

char 	*STTY = "stty: ";

#ifndef SNI
#include <errno.h>
#include <pfmt.h>
#endif /* SNI */

static char	*s_arg;			/* s_arg: ptr to mode to be set */
#ifdef SNI
int	match;
static 	int 	gct(), encode();
int	eq();
#else
static int	match;
static int gct(), eq(), encode();
#endif

/* set terminal modes for supplied options */
/*
 * Procedure:     sttyparse
 *
 * Restrictions:
                 ioctl(2): None
                 pfmt: None
                 strerror: None
 */
char *
sttyparse(argc, argv, term, cb, termiox, winsize)
int	argc;
char	*argv[];
int	term; /* type of tty device */ 

struct	termios	*cb;
struct	termiox	*termiox;
struct	winsize	*winsize;
{
	register i;

	while(--argc > 0) {

		s_arg = *++argv;
		match = 0;

		if (eq("mono"))
		    {
			if (ioctl(0, SWAPMONO, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("color"))
		    {
			if (ioctl(0, SWAPCGA, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("pro"))
		    {
			if (ioctl(0, SWAPPGA, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("enhanced"))
		    {
			if (ioctl(0, SWAPEGA, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("B40x25"))
		    {
			if (ioctl(0, SW_B40x25, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("C40x25"))
		    {
			if (ioctl(0, SW_C40x25, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("B80x25"))
		    {
			if (ioctl(0, SW_B80x25, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("C80x25"))
		    {
			if (ioctl(0, SW_C80x25, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("BG320"))
		    {
			if (ioctl(0, SW_BG320, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("CG320"))
		    {
			if (ioctl(0, SW_CG320, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("BG640"))
		    {
			if (ioctl(0, SW_BG640, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("EGAMONO80x25"))
		    {
			if (ioctl(0, SW_EGAMONO80x25, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("CG320_D"))
		    {
			if (ioctl(0, SW_CG320_D, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("CG640_E"))
		    {
			if (ioctl(0, SW_CG640_E, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("EGAMONOAPA"))
		    {
			if (ioctl(0, SW_EGAMONOAPA, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("CG640x350"))
		    {
			if (ioctl(0, SW_CG640x350, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("ENHMONOAPA2"))
		    {
			if (ioctl(0, SW_ENH_MONOAPA2, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("ENH_CG640"))
		    {
			if (ioctl(0, SW_ENH_CG640, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("ENH_B40x25"))
		    {
			if (ioctl(0, SW_ENHB40x25, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("ENH_C40x25"))
		    {
			if (ioctl(0, SW_ENHC40x25, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("ENH_B80x25"))
		    {
			if (ioctl(0, SW_ENHB80x25, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("ENH_B80x43"))
		    {
			if (ioctl(0, SW_ENHB80x43, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("ENH_C80x43"))
		    {
			if (ioctl(0, SW_ENHC80x43, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("ENH_C80x25"))
		    {
			if (ioctl(0, SW_ENHC80x25, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("VGA_C80x25"))
		    {
			if (ioctl(0, SW_VGAC80x25, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("VGA_C40x25"))
		    {
			if (ioctl(0, SW_VGAC40x25, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("VGAMONO80x25"))
		    {
			if (ioctl(0, SW_VGAMONO80x25, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("MCAMODE"))
		    {
			if (ioctl(0, SW_MCAMODE, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("KB_ENABLE"))
		    {
			if (ioctl(0, TIOCKBON, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
		    }
		else if (eq("KB_DISABLE"))
		    {
			if (ioctl(0, TIOCKBOF, 0) == -1)
			{
				perror(STTY);
				exit(2);
			}
			continue;
	    	    }
		else if (eq("erase") && --argc)
			cb->c_cc[VERASE] = gct(*++argv, term);
		else if (eq("intr") && --argc)
			cb->c_cc[VINTR] = gct(*++argv, term);
		else if (eq("quit") && --argc)
			cb->c_cc[VQUIT] = gct(*++argv, term);
		else if (eq("eof") && --argc)
			cb->c_cc[VEOF] = gct(*++argv, term);
		else if (eq("min") && --argc) {
			if(isdigit((unsigned char)argv[1][0]))
				cb->c_cc[VMIN] = atoi(*++argv);
			else
				cb->c_cc[VMIN] = gct(*++argv);
		}
		else if (eq("eol") && --argc)
			cb->c_cc[VEOL] = gct(*++argv, term);
		else if (eq("eol2") && --argc)
			cb->c_cc[VEOL2] = gct(*++argv, term);
		else if (eq("time") && --argc) {
			if(isdigit((unsigned char)argv[1][0]))
				cb->c_cc[VTIME] = atoi(*++argv);
			else
				cb->c_cc[VTIME] = gct(*++argv);
		}
		else if (eq("kill") && --argc)
			cb->c_cc[VKILL] = gct(*++argv, term);
		else if (eq("swtch") && --argc)
			cb->c_cc[VSWTCH] = gct(*++argv, term);
		else if (eq("start") && --argc)
			cb->c_cc[VSTART] = gct(*++argv, term);
		else if (eq("stop") && --argc)
			cb->c_cc[VSTOP] = gct(*++argv, term);
		else if (eq("susp") && --argc)
			cb->c_cc[VSUSP] = gct(*++argv, term);
		else if (eq("dsusp") && --argc)
			cb->c_cc[VDSUSP] = gct(*++argv, term);
		else if (eq("rprnt") && --argc)
			cb->c_cc[VREPRINT] = gct(*++argv, term);
		else if (eq("flush") && --argc)
			cb->c_cc[VDISCARD] = gct(*++argv, term);
		else if (eq("werase") && --argc)
			cb->c_cc[VWERASE] = gct(*++argv, term);
		else if (eq("lnext") && --argc)
			cb->c_cc[VLNEXT] = gct(*++argv, term);
		else if (eq("ek")) {
			cb->c_cc[VERASE] = CERASE;
			cb->c_cc[VKILL] = CKILL;
		}
		else if ((eq("line") || eq("ctab")) && --argc) {
			/* "line" and "ctab" are obsolete; ignore them */
			++argv;
			continue;
		}
		else if (eq("raw")) {
			cb->c_cc[VMIN] = 1;
			cb->c_cc[VTIME] = 1;
		}
		else if (eq("-raw") || eq("cooked")) {
			cb->c_cc[VEOF] = CEOF;
			cb->c_cc[VEOL] = CNUL;
		}
		else if(eq("sane")) {
			cb->c_cc[VINTR] = CINTR;
			cb->c_cc[VQUIT] = CQUIT;
			cb->c_cc[VERASE] = CERASE;
			cb->c_cc[VKILL] = CKILL;
			cb->c_cc[VEOF] = CEOF;
			cb->c_cc[VEOL] = CNUL;
			/* VSWTCH purposefully not set */
			cb->c_cc[VSTART] = CSTART;
			cb->c_cc[VSTOP] = CSTOP;
			cb->c_cc[VSUSP] = CSUSP;
			cb->c_cc[VDSUSP] = CDSUSP;
			cb->c_cc[VREPRINT] = CREPRINT;
			cb->c_cc[VDISCARD] = CDISCARD;
			cb->c_cc[VWERASE] = CWERASE;
			cb->c_cc[VLNEXT] = CLNEXT;
		}
		else if(eq("ospeed") && --argc) { 
			s_arg = *++argv;
			if (!isdigit(s_arg[0]))
				return s_arg;
			tcsetspeed(TCS_OUT, cb, atoi(s_arg));
			continue;
		}
		else if(eq("ispeed") && --argc) { 
			s_arg = *++argv;
			if (!isdigit(s_arg[0]))
				return s_arg;
			tcsetspeed(TCS_IN, cb, atoi(s_arg));
			continue;
		}
		if (isdigit(s_arg[0])) {
			for (i = 1; s_arg[i]; i++) {
				if (!isdigit(s_arg[i]) && (s_arg[i] != '.'))
					break;
			}
			if (s_arg[i] == '\0') {
				tcsetspeed(TCS_ALL, cb, atoi(s_arg));
				continue;
			}
		}
		
		for(i=0; imodes[i].string; i++)
			if(eq(imodes[i].string)) {
				cb->c_iflag &= ~imodes[i].reset;
				cb->c_iflag |= imodes[i].set;
			}
		for(i=0; omodes[i].string; i++)
			if(eq(omodes[i].string)) {
				cb->c_oflag &= ~omodes[i].reset;
				cb->c_oflag |= omodes[i].set;
			}
		for(i=0; cmodes[i].string; i++)
			if(eq(cmodes[i].string)) {
				cb->c_cflag &= ~cmodes[i].reset;
				cb->c_cflag |= cmodes[i].set;
			}
		for(i=0; lmodes[i].string; i++)
			if(eq(lmodes[i].string)) {
				cb->c_lflag &= ~lmodes[i].reset;
				cb->c_lflag |= lmodes[i].set;
			}
		if (term & FLOW) {
			for(i=0; hmodes[i].string; i++)
				if(eq(hmodes[i].string)) {
					termiox->x_hflag &= ~hmodes[i].reset;
					termiox->x_hflag |= hmodes[i].set;
				}
			for(i=0; clkmodes[i].string; i++)
				if(eq(clkmodes[i].string)) {
					termiox->x_cflag &= ~clkmodes[i].reset;
					termiox->x_cflag |= clkmodes[i].set;
				}
			
		}

		if(eq("rows") && --argc)
			winsize->ws_row = atoi(*++argv);
		else if(eq("columns") && --argc)
			winsize->ws_col = atoi(*++argv);
		else if(eq("xpixels") && --argc)
			winsize->ws_xpixel = atoi(*++argv);
		else if(eq("ypixels") && --argc)
			winsize->ws_ypixel = atoi(*++argv);

		if(!match)
			if(!encode(cb)) {
				return(s_arg); /* parsing failed */
			}
	}
	return((char *)0);
}

#ifndef SNI
static int
#endif
eq(string)
char *string;
{
register i;

if(!s_arg)
	return(0);
i = 0;
loop:
if(s_arg[i] != string[i])
	return(0);
if(s_arg[i++] != '\0')
	goto loop;
match++;
return(1);
}

/* get pseudo control characters from terminal */
/* and convert to internal representation      */
static int
gct(cp, term)
register char *cp;
int term;
{
register c;

if(strcmp(cp, "undef") == 0) {
		/* map "undef" to undefined */
	return (_POSIX_VDISABLE);
}

c = *cp++;
if (c == '^') {
	c = *cp;
	if (c == '?')
		c = CINTR;		/* map '^?' to DEL */
	else if (c == '-')
		c = _POSIX_VDISABLE;		/* map '^-' to undefined */
	else
		c &= 037;
}
return(c);
}

/*
 * Procedure:     get_ttymode
 *
 * Restrictions:
                 ioctl(2): none
*/

/* get modes of tty device and fill in applicable structures */
int
get_ttymode(fd, termios, termiox, winsize)
int fd;
struct termios *termios;
struct termiox *termiox;
struct winsize *winsize;
{
	int term = 0;

	if (tcgetattr(fd, termios) == -1)
		return -1;

	if (termiox != NULL && ioctl(fd, TCGETX, termiox) == 0)
		term |= FLOW;

	if (winsize != NULL && ioctl(fd, TIOCGWINSZ, winsize) == 0)
		term |= WINDOW;

	return term;
}


/*
 * Procedure:     set_ttymode
 *
 * Restrictions:
                 ioctl(2): none
*/
/* set tty modes */
int
set_ttymode(fd, term, termios, termiox, winsize, owinsize)
int fd, term;
struct termios *termios;
struct termiox *termiox;
struct winsize *winsize, *owinsize;
{
	if (tcsetattr(fd, TCSADRAIN, termios) == -1)
		return -1;
		
	if (termiox != NULL && (term & FLOW) &&
	    ioctl(fd, TCSETXW, termiox) == -1) {
		return -1;
	}
	if (winsize != NULL &&
	    (owinsize->ws_col != winsize->ws_col ||
	     owinsize->ws_row != winsize->ws_row ||
	     owinsize->ws_xpixel != winsize->ws_xpixel ||
	     owinsize->ws_ypixel != winsize->ws_ypixel) &&
	    ioctl(0, TIOCSWINSZ, winsize) != 0) {
		return -1;
	}

	return 0;
}


/*
 * Procedure:     encode
 *
 * Restrictions:
                 sscanf: none
*/
static int
encode(cb)
struct	termios	*cb;
{
unsigned long grab[24];
int last, i;

for (i = 0; i < 24; i++)
	grab[i] = 0;
i = sscanf(s_arg, 
"%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx",
&grab[0],&grab[1],&grab[2],&grab[3],&grab[4],&grab[5],&grab[6],
&grab[7],&grab[8],&grab[9],&grab[10],&grab[11],
&grab[12], &grab[13], &grab[14], &grab[15], 
&grab[16], &grab[17], &grab[18], &grab[19]);

if(i < 12) 
	return(0);
cb->c_iflag = grab[0];
cb->c_oflag = grab[1];
cb->c_cflag = grab[2];
cb->c_lflag = grab[3];

if(i == 20)
	last = NCCS - 1;
else
	last = NCC;
for(i=0; i<last; i++)
	cb->c_cc[i] = (unsigned char) grab[i+4];
return(1);
}
