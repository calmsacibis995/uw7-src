#if	!defined(lint) && !defined(DOS)
static char rcsid[] = "$Id$";
#endif
/*
 * Program:	Word at a time routines
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
 * The routines in this file implement commands that work word at a time.
 * There are all sorts of word mode commands. If I do any sentence and/or
 * paragraph mode commands, they are likely to be put in this file.
 */

#include        <stdio.h>
#include	"osdep.h"
#include        "pico.h"
#include        "estruct.h"
#include        <ctype.h>
#include	"edef.h"


/* Word wrap on n-spaces. Back-over whatever precedes the point on the current
 * line and stop on the first word-break or the beginning of the line. If we
 * reach the beginning of the line, jump back to the end of the word and start
 * a new line.  Otherwise, break the line at the word-break, eat it, and jump
 * back to the end of the word.
 * Returns TRUE on success, FALSE on errors.
 */
wrapword()
{
    register int cnt;			/* size of word wrapped to next line */
    register int bp;			/* index to wrap on */
    register int first = -1;
    register int i;

    if(curwp->w_doto <= 0)		/* no line to wrap? */
      return(FALSE);

    for(bp = cnt = i = 0; cnt < llength(curwp->w_dotp) && !bp; cnt++, i++){
	if(isspace((unsigned char) lgetc(curwp->w_dotp, cnt).c)){
	    first = 0;
	    if(lgetc(curwp->w_dotp, cnt).c == TAB)
	      while(i+1 & 0x07)
		i++;
	}
	else if(!first)
	  first = cnt;

	if(first > 0 && i >= fillcol)
	  bp = first;
    }

    if(!bp)
      return(FALSE);

    /* bp now points to the first character of the next line */
    cnt = curwp->w_doto - bp;
    curwp->w_doto = bp;

    if(!lnewline())			/* break the line */
      return(FALSE);

    /* clean up trailing whitespace from line above ... */
    if(backchar(FALSE, 1)){
	while(llength(curwp->w_dotp) > 0 && backchar(FALSE, 1)
	      && isspace((unsigned char) lgetc(curwp->w_dotp, curwp->w_doto).c)
	      && (cnt > 0 || cnt < -1)){
	    forwdel(FALSE, 1);
	    if(cnt < 0)
	      ++cnt;
	}

	gotoeol(FALSE, 1);
	forwchar(FALSE, 1);		/* goto first char of next line */
    }

    /*
     * if there's a line below, it doesn't start with whitespace 
     * and there's room for this line...
     */
    if(!(curbp->b_flag & BFWRAPOPEN)
       && lforw(curwp->w_dotp) != curbp->b_linep 
       && llength(lforw(curwp->w_dotp)) 
       && !isspace((unsigned char) lgetc(lforw(curwp->w_dotp), 0).c)
       && (llength(curwp->w_dotp) + llength(lforw(curwp->w_dotp)) < fillcol)){
	gotoeol(0, 1);			/* then pull text up from below */
	if(lgetc(curwp->w_dotp, curwp->w_doto - 1).c != ' ')
	  linsert(1, ' ');

	forwdel(0, 1);
	gotobol(0, 1);
    }

    curbp->b_flag &= ~BFWRAPOPEN;	/* don't open new line next wrap */
					/* restore dot (account for NL)  */
    if(cnt && !forwchar(0, cnt < 0 ? cnt-1 : cnt))
      return(FALSE);

    return(TRUE);
}


/*
 * Move the cursor backward by "n" words. All of the details of motion are
 * performed by the "backchar" and "forwchar" routines. Error if you try to
 * move beyond the buffers.
 */
backword(f, n)
{
        if (n < 0)
                return (forwword(f, -n));
        if (backchar(FALSE, 1) == FALSE)
                return (FALSE);
        while (n--) {
                while (inword() == FALSE) {
                        if (backchar(FALSE, 1) == FALSE)
                                return (FALSE);
                }
                while (inword() != FALSE) {
                        if (backchar(FALSE, 1) == FALSE)
                                return (FALSE);
                }
        }
        return (forwchar(FALSE, 1));
}

/*
 * Move the cursor forward by the specified number of words. All of the motion
 * is done by "forwchar". Error if you try and move beyond the buffer's end.
 */
forwword(f, n)
{
        if (n < 0)
                return (backword(f, -n));
        while (n--) {
#if	NFWORD
                while (inword() != FALSE) {
                        if (forwchar(FALSE, 1) == FALSE)
                                return (FALSE);
                }
#endif
                while (inword() == FALSE) {
                        if (forwchar(FALSE, 1) == FALSE)
                                return (FALSE);
                }
#if	NFWORD == 0
                while (inword() != FALSE) {
                        if (forwchar(FALSE, 1) == FALSE)
                                return (FALSE);
                }
#endif
        }
	return(TRUE);
}

#ifdef	MAYBELATER
/*
 * Move the cursor forward by the specified number of words. As you move,
 * convert any characters to upper case. Error if you try and move beyond the
 * end of the buffer. Bound to "M-U".
 */
upperword(f, n)
{
        register int    c;
	CELL            ac;

	ac.a = 0;
	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
        if (n < 0)
                return (FALSE);
        while (n--) {
                while (inword() == FALSE) {
                        if (forwchar(FALSE, 1) == FALSE)
                                return (FALSE);
                }
                while (inword() != FALSE) {
                        c = lgetc(curwp->w_dotp, curwp->w_doto).c;
                        if (c>='a' && c<='z') {
                                ac.c = (c -= 'a'-'A');
                                lputc(curwp->w_dotp, curwp->w_doto, ac);
                                lchange(WFHARD);
                        }
                        if (forwchar(FALSE, 1) == FALSE)
                                return (FALSE);
                }
        }
        return (TRUE);
}

/*
 * Move the cursor forward by the specified number of words. As you move
 * convert characters to lower case. Error if you try and move over the end of
 * the buffer. Bound to "M-L".
 */
lowerword(f, n)
{
        register int    c;
	CELL            ac;

	ac.a = 0;
	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
        if (n < 0)
                return (FALSE);
        while (n--) {
                while (inword() == FALSE) {
                        if (forwchar(FALSE, 1) == FALSE)
                                return (FALSE);
                }
                while (inword() != FALSE) {
                        c = lgetc(curwp->w_dotp, curwp->w_doto).c;
                        if (c>='A' && c<='Z') {
                                ac.c (c += 'a'-'A');
                                lputc(curwp->w_dotp, curwp->w_doto, ac);
                                lchange(WFHARD);
                        }
                        if (forwchar(FALSE, 1) == FALSE)
                                return (FALSE);
                }
        }
        return (TRUE);
}

/*
 * Move the cursor forward by the specified number of words. As you move
 * convert the first character of the word to upper case, and subsequent
 * characters to lower case. Error if you try and move past the end of the
 * buffer. Bound to "M-C".
 */
capword(f, n)
{
        register int    c;
	CELL	        ac;

	ac.a = 0;
	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
        if (n < 0)
                return (FALSE);
        while (n--) {
                while (inword() == FALSE) {
                        if (forwchar(FALSE, 1) == FALSE)
                                return (FALSE);
                }
                if (inword() != FALSE) {
                        c = lgetc(curwp->w_dotp, curwp->w_doto).c;
                        if (c>='a' && c<='z') {
			    ac.c = (c -= 'a'-'A');
			    lputc(curwp->w_dotp, curwp->w_doto, ac);
			    lchange(WFHARD);
                        }
                        if (forwchar(FALSE, 1) == FALSE)
                                return (FALSE);
                        while (inword() != FALSE) {
                                c = lgetc(curwp->w_dotp, curwp->w_doto).c;
                                if (c>='A' && c<='Z') {
				    ac.c = (c += 'a'-'A');
				    lputc(curwp->w_dotp, curwp->w_doto, ac);
				    lchange(WFHARD);
                                }
                                if (forwchar(FALSE, 1) == FALSE)
                                        return (FALSE);
                        }
                }
        }
        return (TRUE);
}

/*
 * Kill forward by "n" words. Remember the location of dot. Move forward by
 * the right number of words. Put dot back where it was and issue the kill
 * command for the right number of characters. Bound to "M-D".
 */
delfword(f, n)
{
        register long   size;
        register LINE   *dotp;
        register int    doto;

	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
        if (n < 0)
                return (FALSE);
        dotp = curwp->w_dotp;
        doto = curwp->w_doto;
        size = 0L;
        while (n--) {
#if	NFWORD
		while (inword() != FALSE) {
			if (forwchar(FALSE,1) == FALSE)
				return(FALSE);
			++size;
		}
#endif
                while (inword() == FALSE) {
                        if (forwchar(FALSE, 1) == FALSE)
                                return (FALSE);
                        ++size;
                }
#if	NFWORD == 0
                while (inword() != FALSE) {
                        if (forwchar(FALSE, 1) == FALSE)
                                return (FALSE);
                        ++size;
                }
#endif
        }
        curwp->w_dotp = dotp;
        curwp->w_doto = doto;
        return (ldelete(size, TRUE));
}

/*
 * Kill backwards by "n" words. Move backwards by the desired number of words,
 * counting the characters. When dot is finally moved to its resting place,
 * fire off the kill command. Bound to "M-Rubout" and to "M-Backspace".
 */
delbword(f, n)
{
        register long   size;

	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
        if (n < 0)
                return (FALSE);
        if (backchar(FALSE, 1) == FALSE)
                return (FALSE);
        size = 0L;
        while (n--) {
                while (inword() == FALSE) {
                        if (backchar(FALSE, 1) == FALSE)
                                return (FALSE);
                        ++size;
                }
                while (inword() != FALSE) {
                        if (backchar(FALSE, 1) == FALSE)
                                return (FALSE);
                        ++size;
                }
        }
        if (forwchar(FALSE, 1) == FALSE)
                return (FALSE);
        return (ldelete(size, TRUE));
}
#endif	/* MAYBELATER */

/*
 * Return TRUE if the character at dot is a character that is considered to be
 * part of a word. The word character list is hard coded. Should be setable.
 */
inword()
{
    return(curwp->w_doto < llength(curwp->w_dotp)
	   && isalnum((unsigned char)lgetc(curwp->w_dotp, curwp->w_doto).c));
}


/*
 * Return TRUE if whatever starts the line matches the quote string
 */
quote_match(q, l)
    char *q;
    LINE *l;
{
    register int i;

    for(i = 0; i <= llength(l) && q[i]; i++)
      if(q[i] != lgetc(l, i).c)
	return(0);

    return(1);
}


fillpara(f, n)	/* Fill the current paragraph according to the current
		   fill column						*/

int f, n;	/* deFault flag and Numeric argument */

{
    register int c;			/* current char durring scan	*/
    register int wordlen;		/* length of current word	*/
    register int clength;		/* position on line during fill	*/
    register int i;			/* index during word copy	*/
    register int newlength;		/* tentative new line length	*/
    register int eopflag;		/* Are we at the End-Of-Paragraph? */
    register int firstflag;		/* first word? (needs no space)	*/
    register LINE *eopline;		/* pointer to line just past EOP */
    register int dotflag;		/* was the last char a period?	*/
    char wbuf[NSTRING];			/* buffer for current word	*/
    int  first_word_of_paragraph;
    int  end_of_input_line;
    char *quote = NULL;
    int   quote_len;
    LINE *bqline, *eqline, *qline, *lp;

    if(curbp->b_mode&MDVIEW){		/* don't allow this command if	*/
	return(rdonly());		/* we are in read only mode	*/
    }
    else if (fillcol == 0) {		/* no fill column set */
	mlwrite("No fill column set");
	return(FALSE);
    }

    qline = curwp->w_dotp;		/* remember where we started */

    /* record the pointer to the line just past the EOP */
    if(gotoeop(FALSE, 1) == FALSE)
      return(FALSE);

    eopline = lforw(curwp->w_dotp);

    /* and back to the beginning of the paragraph */
    gotobop(FALSE, 1);

    /* if we're the composer, and we were given a quoting string,
     * and it starts the line we're on, do some special magic...
     */
    if(Pmaster && Pmaster->quote_str
       && (quote_len = i = strlen(Pmaster->quote_str))
       && i <= llength(curwp->w_dotp)){
	while(--i >= 0 && Pmaster->quote_str[i] == lgetc(curwp->w_dotp, i).c)
	  ;

	if(i < 0){				/* bingo! */
#ifdef	MAYBELATER
	    switch(mlyesno("Format quoted text", TRUE)){
	      case FALSE :
		gotoeop(FALSE, 1);
		forwchar(FALSE, 1);
	      case ABORT :
		emlwrite("Text not Justified", NULL);
		return(TRUE);

	      default :
		emlwrite("Justifying quoted text", NULL);
	    }
#endif

	    /*
	     * adjust fillcol and bracket the quoted text with special
	     * blank lines
	     */
	    quote = Pmaster->quote_str;
	    fillcol -= quote_len;

	    i = curwp->w_doto;
	    gotobol(FALSE, 1);
	    lnewline();				/* insert special blank line */
	    backchar(FALSE, 1);			/* back onto it */
	    bqline = curwp->w_dotp;		/* mark start of quoted text */
	    curwp->w_dotp = eopline;
	    gotoeol(FALSE, 1);
	    lnewline();
	    eqline = curwp->w_dotp;		/* mark end of quoted text */
	    curwp->w_dotp = lforw(bqline);	/* restore dot */
	    curwp->w_doto = i;
	}
    }

    /* let yank() know that it may be restoring a paragraph */
    thisflag |= CFFILL;
    
    if(Pmaster == NULL)
      sgarbk = TRUE;
    
    curwp->w_flag |= WFMODE;

    /* save text to be justified to the cut buffer */
    fdelete();
    for(lp = curwp->w_dotp;
	lp != curbp->b_linep && lp != eopline;
	lp = lforw(lp)){
	for(i=(lp == curwp->w_dotp) ? curwp->w_doto : 0; i < llength(lp); i++)
	  finsert(lgetc(lp, i).c);

	finsert('\n');
    }

    if(quote){
	/* remove quoting from whole shebang */
	curwp->w_dotp = lforw(bqline);
	curwp->w_doto = 0;
	do
	  if(quote_match(quote, curwp->w_dotp))
	    forwdel(FALSE, (quote_len > llength(curwp->w_dotp))
			     ? llength(curwp->w_dotp) : quote_len);
	while((curwp->w_dotp = lforw(curwp->w_dotp)) != lback(eqline));

	/* got to the line we started on */
	curwp->w_dotp = qline;
	curwp->w_doto = 0;
	gotoeop(FALSE, 1);
	eopline = lforw(curwp->w_dotp);	/* eop within quoted text */
	/* make sure new eop isn't beyond special token line */
	for(lp = curwp->w_dotp;
	    lp != curbp->b_linep && lp != lback(eqline);
	    lp = lforw(lp))
	  ;

	/* no quoted text to justify, just clean up and return */
	if(lp == curbp->b_linep){
	    fillcol += quote_len;	/* restore fillcol */
	    curwp->w_dotp = bqline;	/* blast leading special blank line */
	    gotobol(FALSE, 1);
	    forwdel(FALSE, 1);
	    do{				/* restore quoting */
		curwp->w_doto = 0;
		for(i = 0; i < quote_len; i++)
		  linsert(1, quote[i]);
	    }
	    while((curwp->w_dotp = lforw(curwp->w_dotp)) != lback(eqline));
	    curwp->w_dotp = lback(eqline);
	    gotoeol(FALSE, 1);
	    forwdel(FALSE, 1);		/* blast trailing special blank line */
	    gotobol(FALSE, 1);
	    fdelete();			/* blast pre-justify text */
	    return(TRUE);
	}

	curwp->w_dotp = lback(eopline);
	curwp->w_doto = 0;
	gotobop(FALSE, 1);
    }
    
    /* initialize various info */
    clength = curwp->w_doto;
    if (clength && curwp->w_dotp->l_text[0].c == TAB)
      clength = 8;
    wordlen = 0;
    dotflag = FALSE;
    
    /* scan through lines, filling words */
    firstflag = TRUE;
    eopflag = FALSE;
    first_word_of_paragraph = TRUE;
    
    while (!eopflag) {
	/* get the next character in the paragraph */
	if (curwp->w_doto == llength(curwp->w_dotp)) {
	    c = ' ';
	    end_of_input_line = TRUE;
	    if (lforw(curwp->w_dotp) == eopline)
	      eopflag = TRUE;
	}
	else{
	    c = lgetc(curwp->w_dotp, curwp->w_doto).c;
	    end_of_input_line = FALSE;
	}

	/* and then delete it */
	ldelete(1L, FALSE);

	/* if not a separator, just add it in */
	if (c != ' ' && c != TAB) {
	    /* 
	     * don't want to limit ourselves to only '.'
	     */
	    dotflag = (int)strchr(".?!:;\")", c); /* dot ? */
	    if (wordlen < NSTRING - 1)
	      wbuf[wordlen++] = c;
	} else if (wordlen) {
	    /* at a word break with a word waiting */
	    /* calculate tentitive new length with word added */
	    newlength = clength + 1 + wordlen;
	    if (newlength <= fillcol) {
		/* add word to current line */
		if (!firstflag) {
		    linsert(1, ' ');	/* the space */
		    ++clength;
		}
		firstflag = FALSE;
	    } else {
		/* start a new line */
		if(!first_word_of_paragraph)
		  lnewline();
		clength = 0;
	    }

	    first_word_of_paragraph = FALSE;

	    /* and add the word in in either case */
	    for (i=0; i<wordlen; i++) {
		linsert(1, wbuf[i]);
		++clength;
	    }

	    /*  Strategy:  Handle 3 cases:
	     *     1. if . at end of line put extra space after it
	     *     2. if . and only 1 space, leave only one space 
	     *     3. if . and more than 1 space, leave 2 spaces
	     *
	     *  So, we know the current c is a space. if then 
	     *  is no next c or the next c is a ' ' then we 
	     *  need to insert a space else don't do it.
	     */
	    if(dotflag && (end_of_input_line
			   || (' ' == lgetc(curwp->w_dotp, curwp->w_doto).c))){
		linsert(1, ' ');
		++clength;
	    }

	    wordlen = 0;
	}
    }
    
    /* and add a last newline for the end of our new paragraph */
    lnewline();
    
    if(quote){				/* replace quoting strings */
	fillcol += quote_len;
	/* remember the current dot. note: ldelete at eob special, handle it */
	eopline = (lforw(curwp->w_dotp) == curbp->b_linep)
		     ? curbp->b_linep : curwp->w_dotp;
	curwp->w_dotp = bqline;
	forwdel(FALSE, 1);		/* blast special blank line */
	do{
	    curwp->w_doto = 0;		/* fill quote strings back in */
	    for(i = 0; i < quote_len; i++)
	      linsert(1, quote[i]);
	}
	while((curwp->w_dotp = lforw(curwp->w_dotp)) != lback(eqline));

	curwp->w_dotp = lback(eqline);	/* blast ending special blank line */
	gotobol(FALSE, 1);
	forwdel(FALSE, 1);

	/*
	 * Restore cursor to eop w/in quote.  Note special handling 
	 * when end of quoted text and end of buffer are the same...
	 */
	curwp->w_dotp = eopline;
	curwp->w_doto = (quote_len > llength(curwp->w_dotp)) ? 0 : quote_len;
    }
}
