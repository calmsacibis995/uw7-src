/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:qued/fread.c	1.10.5.1"

#include <curses.h>
#include <string.h>		/* changed from "" to <> abs s19 */
#include "wish.h"
#include "token.h"
#include "winp.h"
#include "fmacs.h"
#include "vtdefs.h"
#include "vt.h"

#define STR_SIZE	256

int
freadline(row, buff, terminate)
int row;
char *buff;
int terminate;
{
	register int len=0, size = 0;
	chtype ch_string[STR_SIZE];

	fgo (row, 0);
	winchnstr((&VT_array[VT_curid])->win, ch_string, LASTCOL + 1);

	while ((len < (LASTCOL + 1)) && (ch_string[len]!=0))
		len++;
	len--;

	/* extract characters from the ch_string and copy them into buff */

	while (len >= 0 && ((ch_string[len] & A_CHARTEXT) == ' '))
		len--;

	if (len >= 0) {		/* if there is text on this line */
		size = ++len;
		len = 0;
		while (len < size)
			*buff++ = ch_string[len++] & A_CHARTEXT;
	}
	if (terminate)
		*buff = '\0';
	return(size);
}
