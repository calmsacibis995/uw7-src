#if	!defined(DOS)
static char rcsid[] = "$Id$";
#endif
/*
 * Program:	ANSI terminal driver routines
 *
 *
 * Michael Seibel
 * Networks and Distributed Computing
 * Computing and Communications
 * University of Washington
 * Administration Builiding, AG-44
 * Seattle, Washington, 98195, USA
 * Internet: mikes@cac.washington.edu
 *
 * Please address all bugs and comments to "pine-bugs@cac.washington.edu"
 *
 * Pine and Pico are registered trademarks of the University of Washington.
 * No commercial use of these trademarks may be made without prior written
 * permission of the University of Washington.
 * 
 * Pine, Pico, and Pilot software and its included text are Copyright
 * 1989-1996 by the University of Washington.
 * 
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this distribution.
 *
 */
/*
 * The routines in this file provide support for ANSI style terminals
 * over a serial line. The serial I/O services are provided by routines in
 * "termio.c". It compiles into nothing if not an ANSI device.
 */

#define	termdef	1			/* don't define "term" external */

#include        <stdio.h>
#include	"pico.h"
#include	"estruct.h"
#include        "edef.h"
#include        "osdep.h"

#if     ANSI_DRIVER

#define	MARGIN	8			/* size of minimim margin and	*/
#define	SCRSIZ	64			/* scroll size for extended lines */
#define	MROW	2			/* rows in menu                 */
#define BEL     0x07                    /* BEL character.               */
#define ESC     0x1B                    /* ESC character.               */

extern  int     ttopen();               /* Forward references.          */
extern  int     ttgetc();
extern  int     ttputc();
extern  int     ttflush();
extern  int     ttclose();
extern  int     ansimove();
extern  int     ansieeol();
extern  int     ansieeop();
extern  int     ansibeep();
extern  int     ansiopen();
extern	int	ansirev();
/*
 * Standard terminal interface dispatch table. Most of the fields point into
 * "termio" code.
 */
#if defined(VAX) && !defined(__ALPHA)
globaldef
#endif
TERM    term    = {
        NROW-1,
        NCOL,
	MARGIN,
	SCRSIZ,
	MROW,
        ansiopen,
        ttclose,
        ttgetc,
        ttputc,
        ttflush,
        ansimove,
        ansieeol,
        ansieeop,
        ansibeep,
	ansirev
};

ansimove(row, col)
{
        ttputc(ESC);
        ttputc('[');
        ansiparm(row+1);
        ttputc(';');
        ansiparm(col+1);
        ttputc('H');
}

ansieeol()
{
        ttputc(ESC);
        ttputc('[');
        ttputc('K');
}

ansieeop()
{
        ttputc(ESC);
        ttputc('[');
        ttputc('J');
}

ansirev(state)		/* change reverse video state */

int state;	/* TRUE = reverse, FALSE = normal */

{
	static int PrevState = 0;

	if(state != PrevState) {
		PrevState = state ;
		ttputc(ESC);
		ttputc('[');
		ttputc(state ? '7': '0');
		ttputc('m');
	}
}

ansibeep()
{
        ttputc(BEL);
        ttflush();
}

ansiparm(n)
register int    n;
{
        register int    q;

        q = n/10;
        if (q != 0)
                ansiparm(q);
        ttputc((n%10) + '0');
}

#endif

ansiopen()
{
#if     V7
        register char *cp;
        char *getenv();

        if ((cp = getenv("TERM")) == NULL) {
                puts("Shell variable TERM not defined!");
                exit(1);
        }
        if (strcmp(cp, "vt100") != 0) {
                puts("Terminal type not 'vt100'!");
                exit(1);
        }
#endif
	revexist = TRUE;
        ttopen();
}
