/*	copyright	"%c%"	*/

#ident	"@(#)fmt:fmtfile.c	1.4"

#include "misc.h"

#define TOOBIG	":129:Too many characters in a word.\n"

/*
* The initstruct macro is used like a function -- the ; after it will
* translate into an empty statement.
*/
#define initstruct(array, length, initlen) { \
    int i; \
    length = initlen; \
    for (i=0; i < length; i++) { \
	array[i].valid = 1; \
    } \
}

#define wordwrap()	(indent + width >= mwidth && width > 0)
#define wordlarger()	(indent + width > mwidth && width > 0)
#define wordequal()	(indent + width == mwidth)

/*
* fmtfile -- format an entire file.
*
* Inputs:	split	split mode (1 if split mode, 0 if not)
*		crown	like split except for crown mode
*		mwidth	the width that words are wrapped at
*
* Outputs:	None.
*/

void
fmtfile(int split, int crown, int mwidth) {

/*
 * inpara is zero when at the end of a paragraph, and counts the input
 * lines otherwise.
 * indent contains the width of the current indent.
 * lastwd points to the character after the most recent word in outline.
 * count points to the next position to store a character in outline.
 * width contains the current width of outline.
 * incount counts the number of characters read on the current input line.
 * inwidth contains the width of the current input line.
 * nnl contains the number of new lines after the last paragraph.
 * makecrown is true if crown is set and in second line of paragraph.
 */
    int inpara=0, indent=0;
    int lastwd=0, count=0, width=0, incount=0, inwidth=0, nnl=0;
    int tmpnnl;
    int makecrown=0, newpara=1;
    int newindent=0, cflag=0;

    wchar_t	wc;
    int i;

/*
 * outline contains the characters to be written to the output line.
 * indentstr contains the tab-optimized indent string.
 * charw stores the width of each character in the matching pos'n in outline.
 */
    static wchar_t outline[LINE_MAX];
    static char indentstr[LINE_MAX/8+7];
    static uchar charw[LINE_MAX];

/*
 * dontlist contains a list of all the words or strings that, if they
 * occur first in an input line, will cause that line to be treated
 * differently. See misc.h for a description of the various treatment
 * methods.
 */
    static struct dont dontlist[] = {
	0, L".", JOIN,
	0, L"From ", SPLIT|JOIN,
	0, L"From: ", SPLIT|JOIN,
	0, L"To: ", SPLIT|JOIN,
	0, L"Subject: ", SPLIT|JOIN,
	0, L"Cc: ", SPLIT|JOIN,
	0, L"cc: ", SPLIT|JOIN,
	0, L"Bcc: ", SPLIT|JOIN,
	0, L"bcc: ", SPLIT|JOIN,
	0, L"Reply-To: ", SPLIT|JOIN,
    };

    int dontleft, splitok=1, joinok=1, numdont;

/*
* dontleft is the number of possible matches left in the dontlist for
* the current input line.
* numdont is the total number of entries in the dontlist.
* splitok and joinok are flags to mark whether this line is to be treated
* differently (namely, not split, and/or not joined on either end).
*/

    numdont = (sizeof(dontlist) == 0)?0:sizeof(dontlist) /
	    sizeof(struct dont);

    while ((wc = getwchar()) != WEOF) {

	/* if at the beginning of an input line */
	if (incount == 0) {

	    /* count any extra new lines */
	    if (iswlnbr(wc)) {
		inpara = 0;
		newpara = 1;
		nnl = 1;
		while (iswlnbr(wc = getwchar())) {
		    nnl++;
		}
	    }

	    /* count any leading space */
	    if (iswblank(wc)) {
		do {
		    if (wc == TAB) {
			inwidth += 8 - (inwidth % 8);
		    } else {
			inwidth += scrwidth(wc);
		    }
		    incount++;
		} while (iswblank(wc = getwchar()));
		/*
		 * set dontleft depending on whether there was
		 * leading space or not.
		 */
		dontleft = 0;
	    } else {
		initstruct(dontlist, dontleft, numdont);
	    }

	    /*
	    * if crown mode isn't set and the indent changes,
	    * or if this is the last line of a paragraph,
	    * output the line and make a new indent string.
	    */
	    if ((!crown && inwidth != indent) || !inpara) {

		/*
		* if this line needs wrapping, and is more
		* than one word, and splitting is OK:
		* split it, otherwise don't.
		*/
		if (wordlarger() && lastwd != 0 && splitok) {
		    tmpnnl = nnl;
		    nnl = 0;
		    output(outline, charw, &count, &lastwd, &nnl);
		    nnl = tmpnnl;
		    if (newindent) {
			mkindent(indentstr, newindent);
			indent = newindent;
			newindent = 0;
		    }
		    fputs(indentstr, stdout);
		} else {
		    lastwd = count;
		    splitok = 1;
		}
		output(outline, charw, &count, &lastwd, &nnl);
		width = 0;
		joinok = 1;
		if (!crown || newpara || makecrown)	{
			mkindent(indentstr, inwidth);
			indent = inwidth;
		}
		fputs(indentstr, stdout);

	    /*
	    * if crown mode is set, and this is the second line
	    * in the paragraph, prepare a new indent string.
	    */
	    } else if (makecrown && inwidth != indent) {
		newindent = inwidth;
		cflag = 1;
	    }

	    /*
	    * if the output line requires wrapping, and it's OK
	    * to do so, split it.
	    */
	    if (wordwrap() && splitok) {
		if (wordequal() || lastwd == 0) {
		    lastwd = count;
		}
		output(outline, charw, &count, &lastwd, &nnl);
		if (newindent || cflag) {
		    mkindent(indentstr, newindent);
		    indent = newindent;
		    newindent = 0;
		    cflag = 0;
		}
		fputs(indentstr, stdout);
		width = addwidth(charw, count);

		/*
		* In case the next word exceeds the
		* mwidth by itself.
		*/
		if (wordwrap()) {
		    output(outline, charw, &count, &lastwd, &nnl);
		    fputs(indentstr, stdout);
		    width = 0;
		}
	    }

	    /*
	    * if outline still has some words in it, make
	    * sure that they have a space or two after them.
	    */
	    if (count != 0) {
		lastwd = count;

		/* end-sentence punctuation gets 2 spaces */
		if (iswendpunct(outline[count-1])) {
		    if (count >= LINE_MAX) {
			pfmt(stderr, MM_ERROR, TOOBIG);
			exit(1);
		    }
		    outline[count] = SPACE;
		    charw[count++] = 1;
		    width++;
		}

		/*
		 * put spaces between everything except two multibyte
		 * characters.
		 */
		if (!iswmb(outline[count-1]) || !iswmb(wc)) {
		    if (count > LINE_MAX) {
			pfmt(stderr, MM_ERROR, TOOBIG);
			exit(1);
		    }
		    outline[count] = SPACE;
		    charw[count++] = 1;
		    width++;
		}
	    }
	    inpara++;
	    if (newpara && crown && inpara == 1)	{
		makecrown = 1;
		newpara = 0;
	    } else
		makecrown = 0;

	    /*
	     * if we came across an EOF in the newline or blank
	     * searches above, end the loop.
	     */
	    if (wc == WEOF) {
		break;
	    }
	}

	/*
	* if there are still possible matches in the dontlist,
	* check each match, and apply the appropriate results.
	*/
	if (dontleft > 0) {
	    for (i = 0; i < numdont; i++) {
		if (dontlist[i].valid) {
		    if (wc == dontlist[i].str[incount]) {
			if (dontlist[i].str[incount+1] == '\0') {

			    /*
			    * if it's a JOIN, output the previous
			    * line, clear joinok, and force this to
			    * be the last line in the paragraph.
			    */
			    if (dontlist[i].type & JOIN) {
				output(outline, charw, &count, &lastwd, &nnl);
				width = addwidth(charw, count);
				joinok = inpara = 0;
			    }

			    /*
			    * if it's a SPLIT, clear splitok, and
			    * force this to be the last line in the
			    * paragraph.
			    */
			    if (dontlist[i].type & SPLIT) {
				splitok = inpara = 0;
			    }
			    dontleft = 0;
			}
		    } else {
			dontleft--;
			dontlist[i].valid = 0;
		    }
		}
	    }
	}

	/*
	 * if the character is not an ASCII space character, but is a
	 * printable character, put it in outline and set charw. If it
	 * is a control or ASCII space character, keep looking.
	 * Note: this means that non-ASCII spaces are treated as
	 * "hard spaces".
	 */
	if (wc != L' ' && iswprint(wc)) {

	    /*
	     * if the next character is a supplementary character in a
	     * multibyte locale, then a split is allowed before that
	     * character (but only if the start of the character is
	     * positioned before the max column width).
	     * This presumes that all such characters are used
	     * for languages like Japanese, Chinese, and Korean, which
	     * have no spaces separating words.
	     */
	    if (iswmb(wc)) {
		if (!wordwrap()) {
		    lastwd = count;
		}

		if (count > LINE_MAX) {
		    pfmt(stderr, MM_ERROR, TOOBIG);
		    exit(1);
		}
		charw[count] = (uchar)scrwidth(wc);
		width += charw[count];
		outline[count++] = wc;

		/*
		 * if the character is multibyte, then if it exceeds the width,
		 * split. A split point is placed after the mb char.
		 */
		if (wordwrap()) {
		    if (wordequal()) {
			lastwd = count;
		    }
		    output(outline, charw, &count, &lastwd, &nnl);
		    width = addwidth(charw, count);
		} else {
		    lastwd = count;
		}
	    } else {

		if (count > LINE_MAX) {
		    pfmt(stderr, MM_ERROR, TOOBIG);
		    exit(1);
		}
		charw[count] = 1;
		width++;
		outline[count++] = wc;
	    }

	/*
	* if the character is a blank, and it is not the
	* beginning of an output line.
	*/
	} else if (iswblank(wc)) {
	    if (width > 0) {
		if (wordwrap() && splitok) {
		    if (wordequal() || lastwd == 0) {
			lastwd = count;
		    }
		    output(outline, charw, &count, &lastwd, &nnl);
		    width = addwidth(charw, count);
		    if (newindent) {
			mkindent(indentstr, newindent);
			indent = newindent;
			newindent = 0;
		    }
		    if (wordwrap()) {
			fputs(indentstr, stdout);
			output(outline, charw, &count, &lastwd, &nnl);
			width = 0;
		    }
		    if (!crown || split || indent == inwidth || inpara != 1) {
			fputs(indentstr, stdout);
		    }

		/*
		* don't change lastwd from the first blank
		* after the word.
		*/
		} else if (!iswblank(outline[count-1])) {
		    lastwd = count;
		}

		/*
		* don't count blanks at the start of an output
		* line.
		*/
		if (width > 0) {
		    if (wc == TAB) {
			charw[count] = (uchar)(8 - (width % 8));
		    } else {
			charw[count] = (uchar)scrwidth(wc);
		    }
		    if (count >= LINE_MAX) {
			pfmt(stderr, MM_ERROR, TOOBIG);
			exit(1);
		    }
		    width += charw[count];
		    outline[count++] = wc;
		}
	    }

	/*
	* if the character is a line-breaking character,
	* restart the input counters, and if in split mode, force
	* this to be the last line of the input paragraph.
	*/
	} else if (iswlnbr(wc)) {
	    incount = -1;
	    inwidth = 0;
	    if (split) {
		inpara = 0;
	    }

	/*
	* if the character is a backspace, reduce the width by
	* the appropriate amount.
	*/
	} else if (wc == BACKSPC) {
	    if (count > 0) {
		width -= charw[--count];
	    }
	}

	incount++;
    }

    /*
    * if the last line needs wrapping, then wrap it, else just
    * print it out.
    */
    if (width > 0 &&
	    ( (!wordwrap() && !iswblank(outline[count-1])) || 
	    lastwd == 0 ) ) {
	lastwd = count;
    } else if (wordwrap() && lastwd != 0) {
	tmpnnl = nnl;
	nnl = 0;
	output(outline, charw, &count, &lastwd, &nnl);
	nnl = tmpnnl;
	if (newindent) {
	    mkindent(indentstr, newindent);
	    indent = newindent;
	    newindent = 0;
	}
	fputs(indentstr, stdout);
    }
    output(outline, charw, &count, &lastwd, &nnl);
}
