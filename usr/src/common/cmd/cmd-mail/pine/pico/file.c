#if	!defined(lint) && !defined(DOS)
static char rcsid[] = "$Id$";
#endif
/*
 * Program:	High level file input and output routines
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
 * The routines in this file
 * handle the reading and writing of
 * disk files. All of details about the
 * reading and writing of the disk are
 * in "fileio.c".
 */
#include        <stdio.h>
#include	"osdep.h"
#include        "pico.h"
#include	"estruct.h"
#include        "edef.h"
#include        "efunc.h"


#ifdef	ANSI
    int ifile(char *);
    int insmsgchar(int);
#else
    int ifile();
    int insmsgchar();
#endif


/*
 * Read a file into the current
 * buffer. This is really easy; all you do it
 * find the name of the file, and call the standard
 * "read a file into the current buffer" code.
 * Bound to "C-X C-R".
 */
fileread(f, n)
int f, n;
{
        register int    s;
        char fname[NFILEN];

        if ((s=mlreply("Read file: ", fname, NFILEN, QNORML, NULL)) != TRUE)
                return(s);

	if(gmode&MDSCUR){
	    emlwrite("File reading disabled in secure mode",NULL);
	    return(0);
	}

	if (strlen(fname) == 0) {
	  emlwrite("No file name entered",NULL);
	  return(0);
	}

	if((gmode&MDTREE) && !in_oper_tree(fname)){
	  emlwrite("Can't read file from outside of %s", opertree);
	  return(0);
	}

        return(readin(fname, TRUE));
}




static char *inshelptext[] = {
  "Insert File Help Text",
  " ",
  "\tType in a file name to have it inserted into your editing",
  "\tbuffer between the line that the cursor is currently on",
  "\tand the line directly below it.  You may abort this by ",
  "~\ttyping the ~F~2 (~^~C) key after exiting help.",
  " ",
  "End of Insert File Help",
  " ",
  NULL
};

static char *writehelp[] = {
  "Write File Help Text",
  " ",
  "\tType in a file name to have it written out, thus saving",
  "\tyour buffer, to a file.  You can abort this by typing ",
  "~\tthe ~F~2 (~^~C) key after exiting help.",
  " ",
  "End of Write File Help",
  " ",
  " ",
  NULL
};


/*
 * Insert a file into the current
 * buffer. This is really easy; all you do it
 * find the name of the file, and call the standard
 * "insert a file into the current buffer" code.
 * Bound to "C-X C-I".
 */
insfile(f, n)
int f, n;
{
    register int s;
    char	 fname[NFILEN], dir[NFILEN];
    int		 retval, bye = 0, msg = 0;
    char	 prompt[64];
    EXTRAKEYS    menu_ins[5];
    
    if (curbp->b_mode&MDVIEW) /* don't allow this command if */
      return(rdonly()); /* we are in read only mode */

    fname[0] = '\0';
    while(!bye){
	/* set up keymenu stuff */
	if(!msg){
	    int last_menu = 0;

	    menu_ins[last_menu].name  = "^T";
	    menu_ins[last_menu].key   = (CTRL|'T');
	    menu_ins[last_menu].label = "To Files";
	    KS_OSDATASET(&menu_ins[last_menu], KS_NONE);

	    if(Pmaster && Pmaster->msgntext){
		menu_ins[++last_menu].name  = "^W";
		menu_ins[last_menu].key     = (CTRL|'W');
		menu_ins[last_menu].label   = msg ? "InsertFile" : "InsertMsg";
		KS_OSDATASET(&menu_ins[last_menu], KS_NONE);
	    }

#if	!defined(DOS) && !defined(MAC)
	    if(Pmaster && Pmaster->upload){
		menu_ins[++last_menu].name = "^Y";
		menu_ins[last_menu].key    = (CTRL|'Y');
		menu_ins[last_menu].label  = "RcvUpload";
		KS_OSDATASET(&menu_ins[last_menu], KS_NONE);
	    }
#endif	/* !(DOS || MAC) */

	    if(gmode & MDCMPLT){
		menu_ins[++last_menu].name = msg ? "" : "TAB";
		menu_ins[last_menu].key    = (CTRL|'I');
		menu_ins[last_menu].label  = msg ? "" : "Complete";
		KS_OSDATASET(&menu_ins[last_menu], KS_NONE);
	    }

	    menu_ins[++last_menu].name = NULL;
	}

	sprintf(prompt, "%s to insert from %s %s: ",
		msg ? "Number of message" : "File",
		(msg || (gmode&MDCURDIR)) ? "current"
					  : (gmode&MDTREE) ? opertree
							   : "home",
		msg ? "folder" : "directory");
	s = mlreplyd(prompt, fname, NFILEN, QDEFLT, msg ? NULL : menu_ins);
	/* something to read and it was edited or the default accepted */
        if(fname[0] && (s == TRUE || s == FALSE)){
	    bye++;
	    if(msg){
		if((*Pmaster->msgntext)(atol(fname), insmsgchar))
		  emlwrite("Message %s included", fname);
	    }
	    else{
		bye++;
		if(gmode&MDSCUR){
		    emlwrite("Can't insert file in restricted mode",NULL);
		}
		else{
		    if(gmode&MDTREE)
		      compresspath(opertree, fname, NFILEN);

		    fixpath(fname, NFILEN);
		    if((gmode&MDTREE) && !in_oper_tree(fname))
		      emlwrite("Can't insert file from outside of %s",
			        opertree);
		    else
		      retval = ifile(fname);
		}
	    }
	}
	else{
	    switch(s){
	      case (CTRL|'I') :
		{
		    char *fn, *p;
		    int   l = NFILEN;;

		    dir[0] = '\0';
		    if(*fname && (p = strrchr(fname, C_FILESEP))){
			fn = p + 1;
			l -= fn - fname;
			if(p == fname)
			  strcpy(dir, S_FILESEP);
#ifdef	DOS
			else if(fname[0] == C_FILESEP
				 || (isalpha((unsigned char)fname[0])
				     && fname[1] == ':')){
#else
			else if (fname[0] == C_FILESEP || fname[0] == '~') {
#endif
			    strncpy(dir, fname, p - fname);
			    dir[p-fname] = '\0';
			}
			else
			  sprintf(dir, "%s%c%.*s", 
				  (gmode&MDCURDIR) ? "."
					  : (gmode&MDTREE) ? opertree
							   : gethomedir(NULL),
				  C_FILESEP, p - fname, fname);
		    }
		    else{
			fn = fname;
			strcpy(dir, gmode&MDCURDIR ? "."
					    : gmode&MDTREE ? opertree
							   : gethomedir(NULL));
		    }

		    if(!pico_fncomplete(dir, fn, l - 1))
		      (*term.t_beep)();
		}

		break;
	      case (CTRL|'W'):
		msg = !msg;			/* toggle what to insert */
		break;
	      case (CTRL|'T'):
		if(msg){
		    emlwrite("Can't select messages yet!", NULL);
		}
		else{
		    if(*fname && isdir(fname, NULL))
		      strcpy(dir, fname);
		    else
		      strcpy(dir, (gmode&MDCURDIR) ? "."
					  : (gmode&MDTREE) ? opertree
							   : gethomedir(NULL));

		    fname[0] = '\0';
		    if((s = FileBrowse(dir, fname, NULL, FB_READ)) == 1){
			if(gmode&MDSCUR){
			    emlwrite("Can't insert in restricted mode",
				     NULL);
			    sleep(2);
			}
			else{
			    strcat(dir, S_FILESEP);
			    strcat(dir, fname);
			    retval = ifile(dir);
			}
			bye++;
		    }
		    else
		      fname[0] = '\0';

		    refresh(FALSE, 1);
		    if(s != 1){
			update(); 		/* redraw on return */
			continue;
		    }
		}

		break;

#if	!defined(DOS) && !defined(MAC)
	      case (CTRL|'Y') :
		if(Pmaster && Pmaster->upload){
		    char tfname[NFILEN];

		    if(gmode&MDSCUR){
			emlwrite(
			      "\007Restricted mode disallows uploaded command",
			      NULL);
			return(0);
		    }

		    tfname[0] = '\0';
		    retval = (*Pmaster->upload)(tfname, NULL);

		    refresh(FALSE, 1);
		    update();

		    if(retval){
			retval = ifile(tfname);
			bye++;
		    }
		    else
		      sleep(3);			/* problem, show error! */

		    if(tfname[0])		/* clean up temp file */
		      unlink(tfname);
		}
		else
		  (*term.t_beep)();		/* what? */

		break;
#endif	/* !(DOS || MAC) */
	      case HELPCH:
		if(Pmaster){
		    (*Pmaster->helper)(Pmaster->ins_help,
				       "Help for Insert File", 1);
		}
		else
		  pico_help(inshelptext, "Help for Insert File", 1);
	      case (CTRL|'L'):
		refresh(FALSE, 1);
		update();
		continue;
	      default:
		ctrlg(FALSE, 0);
                retval = s;
		bye++;
	    }
        }
    }
    curwp->w_flag |= WFMODE|WFHARD;
    
    return(retval);
}


insmsgchar(c)
    int c;
{
    if(c == '\n'){
	char *p;

	lnewline();
	for(p = Pmaster->quote_str; p && *p; p++)
	  if(!linsert(1, *p))
	    return(0);
    }
    else if(c != '\r')			/* ignore CR (likely CR of CRLF) */
      return(linsert(1, c));

    return(1);
}



/*
 * Read file "fname" into the current
 * buffer, blowing away any text found there. Called
 * by both the read and find commands. Return the final
 * status of the read. Also called by the mainline,
 * to read in a file specified on the command line as
 * an argument.
 */
readin(fname, lockfl)
char    fname[];	/* name of file to read */
int	lockfl;		/* check for file locks? */
{
        register LINE   *lp1;
        register LINE   *lp2;
        register int    i;
        register WINDOW *wp;
        register BUFFER *bp;
        register int    s;
        register int    nbytes;
        register int    nline;
	register char	*sptr;		/* pointer into filename string */
	int		lflag;		/* any lines longer than allowed? */
        char            line[NLINE];
	CELL            ac;

        bp = curbp;                             /* Cheap.               */
	bp->b_linecnt = -1;			/* Must be recalculated */
	ac.a = 0;
        if ((s=bclear(bp)) != TRUE)             /* Might be old.        */
                return (s);
        bp->b_flag &= ~(BFTEMP|BFCHG);
	/* removed 'C' mode detection */
        strcpy(bp->b_fname, fname);
	if((gmode&MDTREE) && !in_oper_tree(fname)){
	    emlwrite("Can't read file from outside of %s", opertree);
	    s = FIOERR;
	    goto out;
	}

        if ((s=ffropen(fname)) != FIOSUC){      /* Hard file open.      */
	    if(s == FIOFNF)                     /* File not found.      */
	      emlwrite("New file", NULL);
	    else
	      fioperr(s, fname);

	    goto out;
	}

        emlwrite("Reading file", NULL);
        nline = 0;
	lflag = FALSE;
        while ((s=ffgetline(line, NLINE)) == FIOSUC || s == FIOLNG) {
		if (s == FIOLNG)
			lflag = TRUE;
                nbytes = strlen(line);
                if ((lp1=lalloc(nbytes)) == NULL) {
                        s = FIOERR;             /* Keep message on the  */
                        break;                  /* display.             */
                }
                lp2 = lback(curbp->b_linep);
                lp2->l_fp = lp1;
                lp1->l_fp = curbp->b_linep;
                lp1->l_bp = lp2;
                curbp->b_linep->l_bp = lp1;
                for (i=0; i<nbytes; ++i){
		    ac.c = line[i];
		    lputc(lp1, i, ac);
		}
                ++nline;
        }
        ffclose();                              /* Ignore errors.       */
        if (s == FIOEOF) {                      /* Don't zap message!   */
                sprintf(line,"Read %d line%s", nline, (nline > 1) ? "s" : "");
                emlwrite(line, NULL);
        }
	if (lflag){
		sprintf(line,"Read %d line%s, Long lines wrapped",
			nline, (nline > 1) ? "s" : "");
                emlwrite(line, NULL);
        }
out:
        for (wp=wheadp; wp!=NULL; wp=wp->w_wndp) {
                if (wp->w_bufp == curbp) {
                        wp->w_linep = lforw(curbp->b_linep);
                        wp->w_dotp  = lforw(curbp->b_linep);
                        wp->w_doto  = 0;
                        wp->w_imarkp = NULL;
                        wp->w_imarko = 0;

			if(Pmaster)
			  wp->w_flag |= WFHARD;
			else
			  wp->w_flag |= WFMODE|WFHARD;
                }
        }
        if (s == FIOERR || s == FIOFNF)		/* False if error.      */
                return(FALSE);
        return (TRUE);
}


/*
 * Ask for a file name, and write the
 * contents of the current buffer to that file.
 * Update the remembered file name and clear the
 * buffer changed flag. This handling of file names
 * is different from the earlier versions, and
 * is more compatable with Gosling EMACS than
 * with ITS EMACS. Bound to "C-X C-W".
 */
filewrite(f, n)
int f, n;
{
        register WINDOW *wp;
        register int    s;
        char            fname[NFILEN];
	char		shows[128], *bufp;
	long		l;		/* length returned from fexist() */
	EXTRAKEYS	menu_write[3];

	if(curbp->b_fname[0] != 0)
	  strcpy(fname, curbp->b_fname);
	else
	  fname[0] = '\0';

	menu_write[0].name  = "^T";
	menu_write[0].label = "To Files";
	menu_write[0].key   = (CTRL|'T');
	menu_write[1].name  = "TAB";
	menu_write[1].label = "Complete";
	menu_write[1].key   = (CTRL|'I');
	menu_write[2].name  = NULL;
	for(;!(gmode & MDTOOL);){
	    s = mlreplyd("File Name to write : ", fname, NFILEN,
			 QDEFLT|QFFILE, menu_write);

	    switch(s){
	      case FALSE:
		if(!fname[0]){			/* no file name to write to */
		    ctrlg(FALSE, 0);
		    return(s);
		}
	      case TRUE:
		if(gmode&MDTREE)
		  compresspath(opertree, fname, NFILEN);

		fixpath(fname, NFILEN);		/*  fixup ~ in file name  */
		if((gmode&MDTREE) && !in_oper_tree(fname)){
		    emlwrite("Can't write outside of %s", opertree);
		    sleep(2);
		    continue;
		}
		else
		  break;
	      case (CTRL|'I'):
		{
		    char *fn, *p, dir[NFILEN];
		    int   l = NFILEN;

		    dir[0] = '\0';
		    if(*fname && (p = strrchr(fname, C_FILESEP))){
			fn = p + 1;
			l -= fn - fname;
			if(p == fname)
			  strcpy(dir, S_FILESEP);
#ifdef	DOS
			else if(fname[0] == C_FILESEP
				 || (isalpha((unsigned char)fname[0])
				     && fname[1] == ':')){
#else
			else if (fname[0] == C_FILESEP || fname[0] == '~') {
#endif
			    strncpy(dir, fname, p - fname);
			    dir[p-fname] = '\0';
			}
			else
			  sprintf(dir, "%s%c%.*s", 
				  (gmode&MDCURDIR) ? "."
					  : gmode&MDTREE ? opertree
							 : gethomedir(NULL),
				  C_FILESEP, p - fname, fname);
		    }
		    else{
			fn = fname;
			strcpy(dir, gmode&MDCURDIR ? "."
					: gmode&MDTREE ? opertree
						       : gethomedir(NULL));
		    }

		    if(!pico_fncomplete(dir, fn, l - 1))
		      (*term.t_beep)();
		}

		continue;
	      case (CTRL|'T'):
		/* If we have a file name, break up into path and file name.*/
		*shows = 0;
		if(*fname) {
		    if (isdir (fname, NULL)) {
			/* fname is a directory. */
			strcpy (shows, fname);
			*fname = '\0';
		    }
		    else {
			/* Find right most seperator. */
			bufp = strrchr (fname, C_FILESEP);
			if (bufp != NULL) {
			    /* Copy directory part to 'shows', and file
			     * name part to front of 'fname'. */
			    *bufp = '\0';
			    strcpy (shows, fname);
			    memcpy (fname, bufp+1, strlen (bufp+1) + 1);
			}
		    }
		}

		/* If we did not end up with a valid directory, use home. */
		if (!*shows || !isdir (shows, NULL))
		  strcpy(shows, (gmode&MDTREE) ? opertree : gethomedir(NULL));

		s = FileBrowse(shows, fname, NULL, FB_SAVE);
		strcat(shows, S_FILESEP);
		strcat(shows, fname);
		strcpy(fname, shows);

		refresh(FALSE, 1);
		update();
		if(s == 1)
		  break;
		else
		  continue;
	      case HELPCH:
		pico_help(writehelp, "", 1);
	      case (CTRL|'L'):
		refresh(FALSE, 1);
		update();
		continue;
	      default:
		return(s);
		break;
	    }

	    if(strcmp(fname, curbp->b_fname) == 0)
		break;

	    if((s=fexist(fname, "w", &l)) == FIOSUC){ /* exists.  overwrite? */

		sprintf(shows, "File \"%s\" exists, OVERWRITE", fname);
		if((s=mlyesno(shows, FALSE)) == TRUE)
		  break;
	    }
	    else if(s == FIOFNF){
		break;				/* go write it */
	    }
	    else{				/* some error, can't write */
		fioperr(s, fname);
		return(ABORT);
	    }
	}
	emlwrite("Writing...", NULL);

        if ((s=writeout(fname)) != -1) {
	        if(!(gmode&MDTOOL)){
		    strcpy(curbp->b_fname, fname);
		    curbp->b_flag &= ~BFCHG;

		    wp = wheadp;                    /* Update mode lines.   */
		    while (wp != NULL) {
                        if (wp->w_bufp == curbp)
			    if((Pmaster && s == TRUE) || Pmaster == NULL)
                                wp->w_flag |= WFMODE;
                        wp = wp->w_wndp;
		    }
		}

		if(s > 1)
		  emlwrite("Wrote %d lines", (void *)s);
		else
		  emlwrite("Wrote 1 line", NULL);
        }
        return ((s == -1) ? FALSE : TRUE);
}



/*
 * Save the contents of the current
 * buffer in its associatd file. No nothing
 * if nothing has changed (this may be a bug, not a
 * feature). Error if there is no remembered file
 * name for the buffer. Bound to "C-X C-S". May
 * get called by "C-Z".
 */
filesave(f, n)
int f, n;
{
        register WINDOW *wp;
        register int    s;

	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
        if ((curbp->b_flag&BFCHG) == 0)         /* Return, no changes.  */
                return (TRUE);
        if (curbp->b_fname[0] == 0) {           /* Must have a name.    */
                emlwrite("No file name", NULL);
		sleep(2);
                return (FALSE);
        }

	emlwrite("Writing...", NULL);
        if ((s=writeout(curbp->b_fname)) != -1) {
                curbp->b_flag &= ~BFCHG;
                wp = wheadp;                    /* Update mode lines.   */
                while (wp != NULL) {
                        if (wp->w_bufp == curbp)
			  if(Pmaster == NULL)
                                wp->w_flag |= WFMODE;
                        wp = wp->w_wndp;
                }
		if(s > 1){
		    emlwrite("Wrote %d lines", (void *)s);
		}
		else
		  emlwrite("Wrote 1 line", NULL);
        }
        return (s);
}

/*
 * This function performs the details of file
 * writing. Uses the file management routines in the
 * "fileio.c" package. The number of lines written is
 * displayed. Sadly, it looks inside a LINE; provide
 * a macro for this. Most of the grief is error
 * checking of some sort.
 *
 * CHANGES: 1 Aug 91: returns number of lines written or -1 on error, MSS
 */
writeout(fn)
char    *fn;
{
        register int    s;
        register LINE   *lp;
        register int    nline;
	char     line[80];

        if (!ffelbowroom(fn) || (s=ffwopen(fn)) != FIOSUC)
                return (-1);			/* Open writes message. */

        lp = lforw(curbp->b_linep);             /* First line.          */
        nline = 0;                              /* Number of lines.     */
        while (lp != curbp->b_linep) {
                if ((s=ffputline(&lp->l_text[0], llength(lp))) != FIOSUC)
                        break;
                ++nline;
                lp = lforw(lp);
        }
        if (s == FIOSUC) {                      /* No write error.      */
                s = ffclose();
        } else                                  /* Ignore close error   */
                ffclose();                      /* if a write error.    */
        if (s != FIOSUC)                        /* Some sort of error.  */
                return (-1);
        return (nline);
}


/*
 * writetmp - write a temporary file for message text, mindful of 
 *	      access restrictions and included text.  If n is true, include
 *	      lines that indicated included message text, otw forget them
 */
char *writetmp(f, n)
int f, n;
{
        static   char	fn[NFILEN];
        register int    s;
        register LINE   *lp;
        register int    nline;

	tmpname(fn);
	
        if ((s=ffwopen(fn)) != FIOSUC)          /* Open writes message. */
                return(NULL);

#ifdef	MODE_READONLY
	chmod(fn, MODE_READONLY);		/* fix access rights */
#endif

        lp = lforw(curbp->b_linep);             /* First line.          */
        nline = 0;                              /* Number of lines.     */
        while (lp != curbp->b_linep) {
	    if(n || (!n && lp->l_text[0].c != '>'))
                if ((s=ffputline(&lp->l_text[0], llength(lp))) != FIOSUC)
                        break;
                ++nline;
                lp = lforw(lp);
        }
        if (s == FIOSUC) {                      /* No write error.      */
                s = ffclose();
        } else                                  /* Ignore close error   */
                ffclose();                      /* if a write error.    */
        if (s != FIOSUC){                       /* Some sort of error.  */
	        unlink(fn);
                return(NULL);
	}
        return(fn);
}


/*
 * Insert file "fname" into the current
 * buffer, Called by insert file command. Return the final
 * status of the read.
 */
ifile(fname)
char    fname[];
{
        register LINE   *lp0;
        register LINE   *lp1;
        register LINE   *lp2;
        register int    i;
        register BUFFER *bp;
        register int    s;
        register int    nbytes;
        register int    nline;
	int		lflag;		/* any lines longer than allowed? */
        char            line[NLINE];
        char     dbuf[128];
        register char    *dbufp;
	CELL            ac;

        bp = curbp;                             /* Cheap.               */
        bp->b_flag |= BFCHG;			/* we have changed	*/
	bp->b_flag &= ~BFTEMP;			/* and are not temporary*/
	bp->b_linecnt = -1;			/* must be recalculated */
	ac.a = 0;
        if ((s=ffropen(fname)) != FIOSUC){      /* Hard file open.      */
		fioperr(s,fname);
                return(FALSE);
	}

        emlwrite("Inserting %s.", fname);

	/* back up a line and save the mark here */
	curwp->w_dotp = lback(curwp->w_dotp);
	curwp->w_doto = 0;
	curwp->w_imarkp = curwp->w_dotp;
	curwp->w_imarko = 0;

        nline = 0;
	lflag = FALSE;
        while ((s=ffgetline(line, NLINE)) == FIOSUC || s == FIOLNG) {
		if (s == FIOLNG)
			lflag = TRUE;
                nbytes = strlen(line);
                if ((lp1=lalloc(nbytes)) == NULL) {
                        s = FIOERR;             /* Keep message on the  */
                        break;                  /* display.             */
                }
		lp0 = curwp->w_dotp;	/* line previous to insert */
		lp2 = lp0->l_fp;	/* line after insert */

		/* re-link new line between lp0 and lp2 */
		lp2->l_bp = lp1;
		lp0->l_fp = lp1;
		lp1->l_bp = lp0;
		lp1->l_fp = lp2;

		/* and advance and write out the current line */
		curwp->w_dotp = lp1;
                for (i=0; i<nbytes; ++i){
		    ac.c = line[i];
		    lputc(lp1, i, ac);
		}
                ++nline;
        }
        ffclose();                              /* Ignore errors.       */
	curwp->w_imarkp = lforw(curwp->w_imarkp);
        if (s == FIOEOF) {                      /* Don't zap message!   */
	        sprintf(dbuf,"Inserted %d line%s",nline,(nline>1) ? "s" : "");
		emlwrite(dbuf, NULL);
        }
	if (lflag) {
		sprintf(dbuf,"Inserted %d line%s, Long lines wrapped.",
			nline, (nline>1) ? "s" : "");
		emlwrite(dbuf, NULL);
        }
out:
	/* advance to the next line and mark the window for changes */
	curwp->w_flag |= WFHARD;

	/* copy window parameters back to the buffer structure */
	curbp->b_dotp = curwp->w_dotp;
	curbp->b_doto = curwp->w_doto;
	curbp->b_markp = curwp->w_imarkp;
	curbp->b_marko = curwp->w_imarko;

        if (s == FIOERR)                        /* False if error.      */
                return (FALSE);
        return (TRUE);
}



/*
 * pico_fncomplete - pico's function to complete the given file name
 */
int pico_fncomplete(dir, fn, len)
char *dir, *fn;
int   len;
{
    char *p, *dlist, tmp[NFILEN], dtmp[NFILEN];
    int   n, i, match = -1;
#ifdef	DOS
#define	FILECMP(x, y)	(toupper((unsigned char)(x))\
				== toupper((unsigned char)(y)))
#else
#define	FILECMP(x, y)	((x) == (y))
#endif

    strcpy(dtmp, dir);
    pfnexpand(dir = dtmp, NFILEN);
    if(*fn && (dlist = p = getfnames(dir, fn, &n, NULL))){
	memset(tmp, 0, sizeof(tmp));
	while(n--){			/* any names in it */
	    for(i = 0; fn[i] && FILECMP(p[i], fn[i]); i++)
	      ;

	    if(!fn[i]){			/* match or more? */
		if(tmp[0]){
		    for(; p[i] && FILECMP(p[i], tmp[i]); i++)
		      ;

		    match = !p[i] && !tmp[i];
		    tmp[i] = '\0';	/* longest common string */
		}
		else{
		    match = 1;		/* may be it!?! */
		    strcpy(tmp,  p);
		}
	    }

	    p += strlen(p) + 1;
	}

	free(dlist);
    }

    if(match >= 0){
	strncpy(fn, tmp, len);
	fn[len] = '\0';
	if(match == 1){
	    strcat(dir, S_FILESEP);
	    strcat(dir, fn);
	    if(isdir(dir, NULL))
	      strcat(fn, S_FILESEP);
	}
    }

    return(match == 1);

}


/*
 * in_oper_tree - returns true if file "f" does reside in opertree
 */
in_oper_tree(f)
char *f;
{
    return(!strncmp(opertree, f, strlen(opertree)));
}
