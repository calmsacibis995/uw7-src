#ident "@(#)edit.c	1.2"
#ident "$Header$"

/*
 *  edit.c - common routines for vi and emacs one line editors in shell
 *
 *   David Korn				P.D. Sullivan
 *   AT&T Bell Laboratories		AT&T Bell Laboratories
 *   Room 3C-526B			Room 1B-286
 *   Murray Hill, N. J. 07974		Columbus, OH 43213
 *   Tel. x7975				Tel. x 2655
 *
 *   Coded April 1983.
 */

extern char sobuf[];

#include	"edit.h"
#include	"history.h"

#define sobuf   ed_errbuf
extern char ed_errbuf[];

#define lookahead	editb.e_index
#define previous	editb.e_lbuf
#define in_raw		editb.e_addnl

struct edit editb;

extern char *strcpy();

genchar edit_killbuf[MAXLINE];

/*
 *	ED_WINDOW()
 *
 *	return the window size
 */

static int
ed_window()
{
	return(80);
}

/*	E_FLUSH()
 *
 *	Flush the output buffer.
 *
 */

void
ed_flush()
{
	register int n = editb.e_outptr-editb.e_outbase;

	if(n <= 0)
		return;
	write(2, editb.e_outbase, (unsigned)n);
	editb.e_outptr = editb.e_outbase;
}

/*
 * send the bell character ^G to the terminal
 */

void
ed_ringbell()
{
	write(2,"\7",1);
}

/*
 * send a carriage return line feed to the terminal
 */

void
ed_crlf()
{
	ed_putchar('\n');
	ed_flush();
}
 
/*	E_SETUP( max_prompt_size )
 *
 *	This routine sets up the prompt string
 *	The following is an unadvertised feature.
 *	  Escape sequences in the prompt can be excluded from the calculated
 *	  prompt length.  This is accomplished as follows:
 *	  - if the prompt string starts with "%\r, or contains \r%\r", where %
 *	    represents any char, then % is taken to be the quote character.
 *	  - strings enclosed by this quote character, and the quote character,
 *	    are not counted as part of the prompt length.
 */

void
ed_setup()
{
	register char *pp;
	register char *last;
	char *ppmax;
	register struct history *fp = &db_history;

	int myquote = 0;
	int qlen = 1;
	char inquote = 0;
	last = editb.e_prbuff;		/* sobuf for KSHELL */

	editb.e_hismax = fp->cur_cmd;
	editb.e_hloff = 0;
	editb.e_hismin = fp->first_cmd - 1;

	if (editb.e_hismin < 0)
		editb.e_hismin = 0;

	editb.e_hline = editb.e_hismax;
	editb.e_wsize = ed_window()-2;
	editb.e_crlf = (*last ? YES : NO);
	pp = editb.e_prompt;
	ppmax = pp+PRSIZE-1;
	*pp++ = '\r';
	{
		register int c;
		while(c= *last++) switch(c)
		{
			case '\r':
				if(pp == (editb.e_prompt+2)) /* quote char */
					myquote = *(pp-1);
				/*FALLTHROUGH*/

			case '\n':
				/* start again */
				editb.e_crlf = YES;
				qlen = 1;
				inquote = 0;
				pp = editb.e_prompt+1;
				break;

			case '\t':
				/* expand tabs */
				while((pp-editb.e_prompt)%TABSIZE)
				{
					if(pp >= ppmax)
						break;
					*pp++ = ' ';
				}
				break;

			case BELL:
				/* cut out bells */
				break;

			default:
				if(c==myquote)
				{
					qlen += inquote;
					inquote ^= 1;
				}
				if(pp < ppmax)
				{
					qlen += inquote;
					*pp++ = c;
					if(!inquote && !isprint(c))
						editb.e_crlf = NO;
				}
		}
	}
	editb.e_plen = pp - editb.e_prompt - qlen;
	*pp = 0;
	if((editb.e_wsize -= editb.e_plen) < 7)
	{
		register int shift = 7-editb.e_wsize;
		editb.e_wsize = 7;
		pp = editb.e_prompt+1;
		strcpy(pp,pp+shift);
		editb.e_plen -= shift;
		last[-editb.e_plen-2] = '\r';
	}
	editb.e_outptr = sobuf;
	editb.e_outbase = editb.e_outptr;
	editb.e_outlast = editb.e_outptr + IOBSIZE-3;

	/* RRR */
	editb.e_kill = 21;
	editb.e_eof = 4;
	editb.e_intr = 127;
}

/*
 * routine to perform read from terminal for vi and emacs mode
 */


int 
ed_getchar()
{
	int i;
	int c;
	unsigned nchar = READAHEAD; /* number of characters to read at a time */
	char readin[LOOKAHEAD] ;

	if (lookahead) {
		c = previous[--lookahead];
		/*** map '\r' to '\n' ***/
		if(c == '\r' && !in_raw)
			c = '\n';
		return(c);
	}
	
	ed_flush() ;

	if(editb.e_cur>=editb.e_eol)
		nchar = 1;

retry:
	editb.e_inmacro = 0;
	i = debug_read(readin, nchar);
	while (i > 0) {
		c = readin[--i] & STRIP;
		previous[lookahead++] = c;
#ifndef CBREAK
		if( c == '\0' ) {
			/*** user break key ***/
			lookahead = 0;
		}
#endif	/* !CBREAK */
	}
	if (lookahead > 0)
		return(ed_getchar());
}

void
ed_ungetchar(int c)
{
	if (lookahead < LOOKAHEAD)
		previous[lookahead++] = c;
	return;
}

/*
 * put a character into the output buffer
 */

ed_putchar(int c)
{
	register char *dp = editb.e_outptr;
	if (c == '_') {
		*dp++ = ' ';
		*dp++ = '\b';
	}
	*dp++ = c;
	*dp = '\0';
	if(dp >= editb.e_outlast)
		ed_flush();
	else
		editb.e_outptr = dp;
}

/*
 * copy virtual to physical and return the index for cursor in physical buffer
 */

ed_virt_to_phys(genchar *virt, genchar *phys, int cur, int voff, int poff)
{
	genchar *sp = virt;
	genchar *dp = phys;
	int c;
	genchar *curp = sp + cur;
	genchar *dpmax = phys+MAXLINE;
	int r;

	sp += voff;
	dp += poff;
	for(r=poff;c= *sp;sp++) {
		if(curp == sp)
			r = dp - phys;
		if(!isprint(c)) {
			if(c=='\t') {
				c = dp-phys;
				if(is_option(EDITVI))
					c += editb.e_plen;
				c = TABSIZE - c%TABSIZE;
				while(--c>0)
					*dp++ = ' ';
				c = ' ';
			} else {
				*dp++ = '^';
				c ^= TO_PRINT;
			}
			/* in vi mode the cursor is at the last character */
			if(curp == sp && is_option(EDITVI))
				r = dp - phys;
		}
		*dp++ = c;
		if(dp>=dpmax)
			break;
	}
	*dp = 0;
	return(r);
}

