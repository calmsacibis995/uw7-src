/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:qued/editmulti.c	1.5.4.3"

#include	<ctype.h>
#include	<stdio.h>
#include        <curses.h>
#include	"token.h"
#include	"winp.h"
#include	"fmacs.h"
#include	"wish.h"

editmulti(tok)
token tok;
{
	register token rettok;
	register int row, col;
	int wrapcol;

	row = Cfld->currow;
	col = Cfld->curcol;
	rettok = TOK_NOP;
	switch(tok) {
	case TOK_UP:
		if (row == 0)
			rettok = TOK_UP;
		else 
			row--;
		break;
	case TOK_DOWN:
		if (row == LASTROW)
			rettok = TOK_DOWN;
		else
			row++;
		break;
	case TOK_RETURN:
	case TOK_ENTER:
		if (row == LASTROW)
			rettok = TOK_RETURN;
		else {
			row++;
			col = 0;
		}
		break;
	case TOK_BACKSPACE:
		/* reposition cursor for further backspaces or erases */
		if (row == 0)
			rettok = tok;
		else {
			fgo(--row, col = LASTCOL);
			fputchar(' ');
		}
		break;
	case TOK_WRAP:
		if (row == LASTROW)
			rettok = tok;
		else if (Flags & I_WRAP) {
			if ((wrapcol = do_wrap()) < 0) {
				wrapcol = 0;
				beep();
			}
			if (col == LASTCOL) {	/* if cursor on last col */ 
				col = wrapcol;
				row++;
			}
		}
		else if (col == LASTCOL) {
			row++;
			col = 0;
		}
		else
			beep();
		break;
	default:
		rettok = tok;
	}
	fgo(row, col);
	return(rettok);
}
