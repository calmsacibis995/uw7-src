#ident	"@(#)cscope:common/help.c	1.5"
/*	cscope - interactive C symbol cross-reference
 *
 *	display help
 *
 */

#include "global.h"
#include <curses.h>	/* LINES needed by constants.h */
/*
	max num of lines of help screen -
	this number needs to be increased if more than n help items are needed
*/
#define MAXHELP	50	/* maximum number of help strings */

void
help()
{
	char	**ep, *s, **tp, *text[MAXHELP];	
	int	ln;

	tp = text;
	if (changing == NO) {
		if (mouse) {
			*tp++ = "Point with the mouse and click button 1 to move to the desired input field,\n";
			*tp++ = "type the pattern to search for, and then press the RETURN key.  For the first 4\n";
			*tp++ = "and last 2 input fields, the pattern can be a regcmp(3X) regular expression.\n";
			*tp++ = "If the search is successful, you can edit the file containing a displayed line\n";
			*tp++ = "by pointing with the mouse and clicking button 1.\n";
			*tp++ = "\nYou can either use the button 2 menu or these single-character commands:\n\n";
		}
		else {
			*tp++ = "Press the TAB key repeatedly to move to the desired input field, type the\n";
			*tp++ = "pattern to search for, and then press the RETURN key.  For the first 4 and\n";
			*tp++ = "last 2 input fields, the pattern can be a regcmp(3X) regular expression.\n";
			*tp++ = "If the search is successful, you can use these single-character commands:\n\n";
			*tp++ = "1-9\t\tEdit the file containing the displayed line.\n";
		}
		*tp++ = "space bar\tDisplay next set of matching lines.\n";
		*tp++ = "+\t\tDisplay next set of matching lines.\n";
		*tp++ = "^V\t\tDisplay next set of matching lines.\n";
		*tp++ = "-\t\tDisplay previous set of matching lines.\n";
		*tp++ = "^E\t\tEdit all lines.\n";
		*tp++ = ">\t\tWrite the list of lines being displayed to a file.\n";
		*tp++ = ">>\t\tAppend the list of lines being displayed to a file.\n";
		*tp++ = "<\t\tRead lines from a file.\n";
		*tp++ = "^\t\tFilter all lines through a shell command.\n";
		*tp++ = "|\t\tPipe all lines to a shell command.\n";
		if (!mouse) {
			*tp++ = "\nAt any time you can use these single-character commands:\n\n";
			*tp++ = "TAB\t\tMove to the next input field.\n";
			*tp++ = "RETURN\t\tMove to the next input field.\n";
			*tp++ = "^N\t\tMove to the next input field.\n";
			*tp++ = "^P\t\tMove to the previous input field.\n";
		}
		*tp++ = "^Y\t\tSearch with the last pattern typed.\n";
		*tp++ = "^B\t\tRecall previous input field and search pattern.\n";
		*tp++ = "^F\t\tRecall next input field and search pattern.\n";
		if(caseless)
			*tp++ = "^C\t\tToggle ignore/use letter case when searching (IGNORE).\n";
		else
			*tp++ = "^C\t\tToggle ignore/use letter case when searching (USE).\n";
		*tp++ = "^R\t\tRebuild the cross-reference.\n";
		*tp++ = "!\t\tStart an interactive shell (type ^D to return to cscope).\n";
		*tp++ = "^L\t\tRedraw the screen.\n";
		*tp++ = "?\t\tDisplay this list of commands.\n";
		*tp++ = "^D\t\tExit cscope.\n";
		*tp++ = "\nNote: If the first character of the pattern you want to search for matches\n";
		*tp++ = "a command, type a \\ character first.\n";
	}
	else {
		if (mouse) {
			*tp++ = "Point with the mouse and click button 1 to mark or unmark the line to be\n";
			*tp++ = "changed.  You can also use the button 2 menu or these single-character\n";
			*tp++ = "commands:\n\n";
		}
		else {
			*tp++ = "When changing text, you can use these single-character commands:\n\n";
			*tp++ = "1-9\t\tMark or unmark the line to be changed.\n";
		}
		*tp++ = "*\t\tMark or unmark all displayed lines to be changed.\n";
		*tp++ = "space bar\tDisplay next set of lines.\n";
		*tp++ = "+\t\tDisplay next set of lines.\n";
		*tp++ = "-\t\tDisplay previous set of lines.\n";
		*tp++ = "a\t\tMark or unmark all lines to be changed.\n";
		*tp++ = "^D\t\tChange the marked lines and exit.\n";
		*tp++ = "ESC\t\tExit without changing the marked lines.\n";
		*tp++ = "!\t\tStart an interactive shell (type ^D to return to cscope).\n";
		*tp++ = "^L\t\tRedraw the screen.\n";
		*tp++ = "?\t\tDisplay this list of commands.\n";
	}
	/* print help, a screen at a time */
	ep = tp;
	ln = 0;
	for (tp = text; tp < ep; ) {
		if (ln < LINES - 1) {
			for (s = *tp; *s != '\0'; ++s) {
				if (*s == '\n') {
					++ln;
				}
			}
			(void) addstr(*tp++);
		}
		else {
			(void) addstr("\n");
			askforchar();
			(void) clear();
			ln = 0;
		}
	}
	if (ln) {
		(void) addstr("\n");
		askforchar();
	}
}
