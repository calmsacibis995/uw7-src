/*	copyright	"%c%"	*/

#ident	"@(#)fmt:misc.c	1.1.1.1"

#include "misc.h"

/*
* output -- output a line, and move the remaining characters to the
*	     beginning of the buffer.
*
* Inputs:	outline		the buffer containing the output string
*		charw		the buffer containing the widths of the
* 				characters in the output string.
*		count		the total number of characters in outline.
*		lastwd		the number of characters to output.
*		nnl		the number of newlines to output (minus 1).
*
* Outputs:	outline		the remaining characters are moved to
*				the beginning of the buffer.
*		charw		same modification as outline.
*		count		contains the new total of characters.
*		lastwd		is the same as count.
*		nnl		set to zero.
*/

void
output(wchar_t outline[], uchar charw[], int *count, int *lastwd, int *nnl) {

    int i, j;

    /*
    * if there are extra newlines to output, the whole string
    * should always be printed.
    */
    if (*nnl) {
	*lastwd = *count;
    }

    /* ignore null outputs */
    if (*lastwd == 0) {
	if (*nnl == 0) {
		return;
	} else {
		(*nnl)--;
	}
    }

    for (i=0; i < *lastwd; i++) {
	putwchar(outline[i]);
    }
    for (i=0; i <= *nnl; i++) {
	putchar('\n');
    }

    /* don't copy leading blanks to the start of outline */
    for (j=*lastwd; j < *count && iswblank(outline[j]); j++);

    *lastwd = j;

    for (i=0; j < *count; j++, i++) {
	outline[i] = outline[j];
	charw[i] = charw[j];
    }
    *count = *count - *lastwd;

    *lastwd = *count;
    *nnl = 0;
}

/*
* mkindent -- construct an optimal indent string given a width (ie.
*		 create a string of tabs and spaces with the minimal number of
*		 characters for a given width)
*
* Inputs:	inwidth		the required width, in screen columns
*				(presumably starting from zero).
*
* Outputs:	indentstr	the resulting byte string.
*/

void
mkindent(char indentstr[], int inwidth) {

    int i, j=0, num;

    /* Number of tabs needed */
    num = inwidth / 8;
    for (i=0; i < num; i++) {
	indentstr[j++] = '\t';
    }

    /* Number of spaces needed */
    num = inwidth % 8;
    for (i=0; i < num; i++) {
	indentstr[j++] = ' ';
    }

    indentstr[j] = '\0';
}

/*
* addwidth -- total the width of the string up to a given character.
*
* Inputs:	charw		the array containing the width of each
*				character.
*		count		the number of characters to count across.
*
* Outputs:	<result>	the width in screen columns of the string.
*/

int
addwidth(uchar charw[], int count) {

    int i, tot=0;

    for (i=0; i < count; i++) {
	tot += charw[i];
    }
    return tot;
}
