#if !defined(lint) && !defined(DOS)
static char rcsid[] = "$Id$";
#endif
/*----------------------------------------------------------------------

            T H E    P I N E    M A I L   S Y S T E M

   Laurence Lundblade and Mike Seibel
   Networks and Distributed Computing
   Computing and Communications
   University of Washington
   Administration Builiding, AG-44
   Seattle, Washington, 98195, USA
   Internet: lgl@CAC.Washington.EDU
             mikes@CAC.Washington.EDU

   Please address all bugs and comments to "pine-bugs@cac.washington.edu"


   Pine and Pico are registered trademarks of the University of Washington.
   No commercial use of these trademarks may be made without prior written
   permission of the University of Washington.

   Pine, Pico, and Pilot software and its included text are Copyright
   1989-1996 by the University of Washington.

   The full text of our legal notices is contained in the file called
   CPYRIGHT, included with this distribution.


   Pine is in part based on The Elm Mail System:
    ***********************************************************************
    *  The Elm Mail System  -  Revision: 2.13                             *
    *                                                                     *
    * 			Copyright (c) 1986, 1987 Dave Taylor              *
    * 			Copyright (c) 1988, 1989 USENET Community Trust   *
    ***********************************************************************
 

  ----------------------------------------------------------------------*/

/*======================================================================
     help.c
     Functions to support the pine help screens
 ====*/


#include "headers.h"

/*
 * Internal prototypes
 */
void free_help_text PROTO((char **));


#ifdef HELPFILE
/*
 * get_help_text - return the help text associated with index
 *                 in an array of pointers to each line of text.
 */
char **
get_help_text(index, pages)
short index;
int   *pages;
{
    int  i, current_page, len;
    char buf[256], **htext, *tmp;
    void copy_fix_keys();
    struct hindx hindex_record;
    FILE *helpfile;
#if !defined(WIN32) && !defined(OS2)
    extern long coreleft();
#endif

    if(index < 0 || index >= LASTHELP)
	return(NULL);

    /* make sure index file is available, and read appropriate record */
    build_path(buf, ps_global->pine_dir, HELPINDEX);
    if(!(helpfile = fopen(buf, "rb"))){
	q_status_message1(SM_ORDER,3,5,
	    "No Help!  Index \"%s\" not found.", buf);
	return(NULL);
    }

    i = fseek(helpfile, index * sizeof(struct hindx), SEEK_SET) == 0
	 && fread((void *)&hindex_record,sizeof(struct hindx),1,helpfile) == 1;

    fclose(helpfile);
    if(!i){	/* problem in fseek or read */
        q_status_message(SM_ORDER, 3, 5,
		  "No Help!  Can't locate proper offset for help text file.");
	return(NULL);
    }

#if !defined(WIN32) && !defined(OS2)
    if(coreleft() < (long)(80 * hindex_record.lines)){
        q_status_message(SM_ORDER,3,5,
	    "No Help!  Not enough memory to display help.");
	return(NULL);
    }
#endif

    /* make sure help file is open */
    build_path(buf, ps_global->pine_dir, HELPFILE);
    if((helpfile = fopen(buf, "rb")) == NULL){
	q_status_message2(SM_ORDER,3,5,"No Help!  \"%s\" : %s", buf,
			  error_description(errno));
	return(NULL);
    }

    if(fseek(helpfile, hindex_record.offset, SEEK_SET) != 0
       || fgets(buf, 255, helpfile) == NULL
       || strncmp(hindex_record.key, buf, strlen(hindex_record.key))){
	/* problem in fseek, or don't see key */
        q_status_message(SM_ORDER, 3, 5,
		     "No Help!  Can't locate proper entry in help text file.");
	fclose(helpfile);
	return(NULL);
    }

    htext = (char **)fs_get(sizeof(char *) * (hindex_record.lines + 1));
    current_page = 0;
    for(i=0; i < hindex_record.lines; i++){
	if(fgets(buf, 255, helpfile) == NULL){
	    htext[i] = NULL;
	    free_help_text(htext);
	    fclose(helpfile);
            q_status_message(SM_ORDER,3,5,"No Help!  Entry not in help text.");
	    return(NULL);
	}

	if(*buf){
	    if((len = strlen(buf)) > 1
	       && (buf[len-2] == '\n' || buf[len-2] == '\r'))
	      buf[len-2] = '\0';
	    else if(buf[len-1] == '\n' || buf[len-1] == '\r')
	      buf[len-1] = '\0';
	}
	else
	  len = 0;

	htext[i] = NULL;
	if(tmp = srchstr(buf, "___")){
	    if(!strcmp(tmp+3, "----")){
		if(pages)
		  pages[current_page++] = i;
	    }
	    else {
		extern char datestamp[];

		/* allocate the line (allow for BIG date)... */
		htext[i] = (char *) fs_get(strlen(buf) + 128
					   + strlen(datestamp)
					   + strlen(pine_version));
		/* and copy the new text */
		strcpy(htext[i], buf);

		/* loop thru copy replacing strings */
		for(tmp = htext[i]; tmp = srchstr(tmp, "___"); tmp++)
		  if(!struncmp(tmp+3, "tdate", 5)){
		      char datebuf[128];
		      rfc822_date(datebuf);	 /* let c-client do the work */
		      rplstr(tmp, 8, datebuf);
		  }
		  else if(!struncmp(tmp+3, "cdate", 5))
		    rplstr(tmp, 8, datestamp);
		  else if(!struncmp(tmp+3, "version", 7))
		    rplstr(tmp, 10, pine_version);
	    }
	}

	if(!htext[i]){
	    htext[i] = (char *)fs_get(len + 1);
	    copy_fix_keys(htext[i], buf);
	}
    }

    htext[i] = NULL;
    if(pages != NULL)
      pages[current_page] = -1;

    fclose(helpfile);
    return(htext);
}
#endif	/* HELPFILE */


/*
 * free_help_text - free the strings and array of pointers pointed to 
 *                  by text
 */
void
free_help_text(text)
    char **text;
{
    int i = 0;

    while(text[i])
      fs_give((void **)&text[i++]);

    fs_give((void **)&text);
}



/*----------------------------------------------------------------------
     Make a copy of the help text for display, doing various substutions

  Args: a -- output string
        b -- input string

 Result: string copied into buffer, and any expressions of the form
 {xxx:yyy} are replaced by either xxx or yyy depending on whether
 we are using function keys or not.  _'s are removed except for the
 special case ^_, which is left as is.
  ----*/

void 
copy_fix_keys(a, b)
  register char *a, *b;
{
#if defined(DOS) || defined(OS2)
    register char *s = a;
#endif
    while(*b) {
	while(*b && *b != '{')
          if(*b == '^' && *(b+1) == '_') {
              *a++ = *b++;
              *a++ = *b++;
	  } else if(*b == '_' && *(b+1) == '_') {
	      while(*b == '_')
		b++;
	  }
	  else
	    *a++ = *b++;

	if(*b == '\0') {
	    break;
	}

	if(F_ON(F_USE_FK,ps_global)) {
	    if(*b)
	      b++;
	    while(*b && *b != ':')
	      *a++ = *b++;
	    while(*b && *b != '}')
	      b++;
	    if(*b)
	      b++;
	} else {
	    while(*b && *b != ':')
	      b++;
	    if(*b)
	      b++;
	    while(*b && *b != '}')
	      *a++ = *b++;
	    if(*b)
	      b++;
	}
    }
    *a = '\0';
#if defined(DOS) || defined(OS2)
    b = s;			/* eliminate back-slashes from "a" */
    while(*s){
	if(*s == '\\')
	  s++;

        *b++ = *s++;
    }

    *b = '\0';			/* tie off back-slashed-string */
#endif
}



/*----------------------------------------------------------------------
     Get the help text in the proper format and call scroller

    Args: text   -- The help text to display (from pine.help --> helptext.c)
          title  -- The title of the help text 
  
  Result: format text and call scroller

  The pages array contains the line number of the start of the pages in
the text. Page 1 is in the 0th element of the array.
The list is ended with a page line number of -1. Line number 0 is also
the first line in the text.
  -----*/
void
#ifdef	HELPFILE

helper(text, title, from_composer)
     short  text;
     char   *title;
     int    from_composer;
{
    char **new_text;

    if((new_text = get_help_text(text, NULL)) == NULL)
      return;

    if(F_ON(F_BLANK_KEYMENU,ps_global)){
	FOOTER_ROWS(ps_global) = 3;
	clearfooter(ps_global);
	ps_global->mangled_screen = 1;
    }

#else

helper(text, title, from_composer)
     char  *text[], *title;
     int    from_composer;
{
    register char **t, **t2;
    char          **new_text, *tmp;
    int             pages[100], current_page, in_include;
    char            *line, line_buf[256];
    FILE           *file;
    int		    alloced_lines = 0;
    int		    need_more_lines = 0;


    dprint(1, (debugfile, "\n\n    ---- HELPER ----\n"));

    if(F_ON(F_BLANK_KEYMENU,ps_global)){
	FOOTER_ROWS(ps_global) = 3;
	clearfooter(ps_global);
	ps_global->mangled_screen = 1;
    }

    if(from_composer){
 	fix_windsize(ps_global);
	init_sigwinch();
    }

    /*----------------------------------------------------------------------
            First copy the help text and do substitutions
      ----------------------------------------------------------------------*/

    for(t = text ; *t != NULL; t++);
    alloced_lines = t - text;
    new_text = (char **)fs_get((alloced_lines + 1) * sizeof(char *));


    current_page = 0;
    in_include   = 0;
    file         = NULL;
    for(t = text, t2 = new_text; *t != NULL;) {
        int inc_line_cnt;
        int lines_to_use = ps_global->ttyo->screen_rows
			   - HEADER_ROWS(ps_global) - FOOTER_ROWS(ps_global);
        if(in_include) {
            /*--- Inside an ___include ... ___end_include section ---*/
            if(file != NULL) {

                /*--- Read next line out of include file ---*/

		/* Add a page break every so often.  This is stupid in that
		 * it adds a page break even if there are no more lines.
		 */
                if(inc_line_cnt++ % lines_to_use == 0) {
		    /* this is a big long string, don't delete part of it */
                    line = "                                                                         ___----";
                }else {
                    line = fgets(line_buf, sizeof(line_buf), file);
		}
                if(line == NULL) {
                    fclose(file);
                    in_include = 0;
                    continue;
                }
                if(line[strlen(line)-1] == '\n')
                    line[strlen(line)-1] = '\0';
            } else {
                /*--- File wasn't there, copy the default text ---*/
                if(struncmp(*t, "___end_include", 14) == 0){
                    in_include = 0;
                    t++;
                    continue;
                } else {
                    line = *t;
                }
            }
        } else {
            if(struncmp(*t, "___include", 10) == 0) {
                /*-- Found start of an ___include ... ___end_include block --*/
                char *p, *q, **pp;
		int cnt;
                in_include = 1;
                inc_line_cnt = (current_page > 0)
		                  ? t2 - new_text - pages[current_page-1]
				  : t2 - new_text;
                for(p = (*t)+10; *p && isspace((unsigned char)*p); p++);
                strcpy(line_buf, p);
                for(q = line_buf; *q && !isspace((unsigned char)*q); q++);
                *q = '\0';
                dprint(9, (debugfile, "About to open \"%s\"\n", line_buf));
                file = fopen(line_buf, "r");
                if(file != NULL) {
		  /*
		   * Calculate the size of the included file,
		   * might have to resize new_text.
		   */
		   for(cnt=0; fgets(line_buf, sizeof(line_buf), file) != NULL;
								   cnt++);
                   /* add lines so we can insert page breaks if necessary */
                   cnt = cnt + (inc_line_cnt-1+cnt)/(lines_to_use - 1);
		   /* rewind */
		   (void)fseek(file, 0L, 0);
		    /*
		     * Skip t forward to the end_include so that we know
		     * how many lines we're skipping.
		     */
		   pp = t;
                   while(*t && struncmp(*t, "___end_include", 14) != 0)
                     t++;
		   /* we need cnt more lines but freed up t-pp+1 lines */
		   need_more_lines += cnt - (t - pp + 1);
		   if(need_more_lines > 0) {
		     int offset = t2 - new_text;
		     alloced_lines += need_more_lines;
		     need_more_lines = 0;
		     fs_resize((void **)&new_text,
			       (alloced_lines + 1) * sizeof(char *));
		     t2 = &new_text[offset];
		   }
		}else {
                  dprint(1, (debugfile,"Helptext Failed open \"%s\": \"%s\"\n",
                             line_buf, error_description(errno)));
		}
                t++;
                continue;
            }  else {
                /*-- The normal case, just another line of help text ---*/
                line = *t;
            }
        }

        /*-- line now contains the text to use, where ever it came from --*/
        dprint(9, (debugfile, "line-->%s<-\n", line));

	*t2 = NULL;
	if(tmp = srchstr(line, "___")){
	    if(!strcmp(tmp+3, "----")){
		pages[current_page++] = t2 - new_text;
	    }
	    else{
		extern char datestamp[];

		/* allocate the line (allow for BIG date)... */
		*t2 = (char *) fs_get(strlen(line) + 128
				      + strlen(datestamp)
				      + strlen(pine_version));
		/* and copy the new text */
		strcpy(*t2, line);

		/* loop thru copy replacing strings */
		for(tmp = *t2; tmp = srchstr(tmp, "___"); tmp++)
		  if(!struncmp(tmp+3, "tdate", 5)){
		      char datebuf[128];
		      rfc822_date(datebuf);	 /* let c-client do the work */
		      rplstr(tmp, 8, datebuf);
		  }
		  else if(!struncmp(tmp+3, "cdate", 5))
		    rplstr(tmp, 8, datestamp);
		  else if(!struncmp(tmp+3, "version", 7))
		    rplstr(tmp, 10, pine_version);
	    }
	}

	if(!*t2){
	    *t2 = (char *)fs_get(strlen(line) + 1);
	    copy_fix_keys(*t2, line);
	}

        t2++;
	if(!in_include || (in_include && file == NULL))
            t++;
    }
    *t2 = *t;
    pages[current_page] = -1;

    dprint(7, (debugfile, "helper PAGE COUNT %d\n", current_page));
    { int i;
        for(i = 0 ; pages[i] != -1; i++) 
         dprint(7, (debugfile, "helper PAGE %d line %d [%s]\n",i+1, pages[i],
                    new_text[pages[i]]));
    }
#endif	/* HELPFILE */

#ifdef _WINDOWS
    /* Under windows, throw the help text into a window. */
    if (mswin_displaytext (title, NULL, 0, new_text, 0, 0) >= 0) 
      return;
    /* Buf if the window fails, fall through and display in main window. */
#endif

    /* This is mostly here to get the curses variables for line and column
        in sync with where the cursor is on the screen. This gets warped
	when the composer is called because it does it's own stuff */
    ClearScreen();
    scrolltool((void *)new_text, title,
               (text == main_menu_tx)
	         ? MainHelpText
		 : from_composer
		   ? ComposerHelpText
		   : HelpText,
	       CharStarStar, (ATTACH_S *)NULL);

    if(F_ON(F_BLANK_KEYMENU,ps_global))
      FOOTER_ROWS(ps_global) = 1;

    ClearScreen();

    free_help_text(new_text);
}


void
print_all_help()
{
#ifdef	HELPFILE
    short t;
    char **l, **h, buf[500];
#else
    char ***t, **l, buf[500];
#endif
    int     line_count;

    if(open_printer("all 50+ pages of help text ") == 0) {
#ifdef	HELPFILE
        for(t = 0; t < LASTHELP; t++) {
	    if((h = get_help_text(t, NULL)) == NULL){
		return;
    	    }
            for(l = h; *l != NULL; l++) {
#else
        for(t = h_texts; *t != NULL; t++) {
            line_count = 0;
            for(l = *t; *l != NULL; l++) {
#endif
                copy_fix_keys(buf, *l);
                print_text(buf);
                print_char('\n');
                line_count++;
            }

            if(line_count <= 10){
		print_text(NEWLINE);
		print_text(NEWLINE);
	    }
	    else{
		print_char(ctrl('L'));
		print_text(NEWLINE);
	    }
        }
        close_printer();
    }
}


#if defined(DOS) && !defined(_WINDOWS)
#define NSTATUS 25  /* how many status messages to save for review */
#else
#define NSTATUS 100
#endif

static char *stat_msgs[NSTATUS];
static int   latest;

/*----------------------------------------------------------------------
     Review last N status messages

    Args: title  -- The title of the screen
  -----*/
void
review_messages(title)
    char  *title;
{
    int             how_many = 0;
    char          **tmp_text;
    register char **e;
    register int    i, j;

    e = stat_msgs;
    tmp_text = (char **)fs_get((NSTATUS + 1) * sizeof(char *));

    /* allocate strings */
    for(j = 0, i = latest+1; i < NSTATUS; i++)
      if(e[i] && *e[i])
	tmp_text[j++] = cpystr(e[i]);

    for(i = 0; i <= latest; i++)
      if(e[i] && *e[i])
	tmp_text[j++] = cpystr(e[i]);
    
    /* scrolltool expects NULL at end */
    tmp_text[j] = NULL;
    how_many = j;

    scrolltool((void *)tmp_text, title, ReviewMsgsText,
	CharStarStar, (ATTACH_S *)NULL);

    /* free everything */
    for(j = 0; j < how_many; j++)
      fs_give((void **)&tmp_text[j]);

    fs_give((void **)&tmp_text);
}


/*----------------------------------------------------------------------
     Add a message to the circular status message review buffer

    Args: message  -- The message to add
  -----*/
void
add_review_message(message)
    char *message;
{
    if(!(message && *message))
      return;

    latest = (latest + 1) % NSTATUS;
    if(stat_msgs[latest] && strlen(stat_msgs[latest]) >= strlen(message))
      strcpy(stat_msgs[latest], message);  /* already enough space */
    else{
	if(stat_msgs[latest])
	  fs_give((void **)&stat_msgs[latest]);

	stat_msgs[latest] = cpystr(message);
    }
}



/*----------------------------------------------------------------------
    Free resources associated with the status message review list

    Args: 
  -----*/
void
end_status_review()
{
    for(latest = NSTATUS - 1; latest >= 0; latest--)
      if(stat_msgs[latest])
	fs_give((void **)&stat_msgs[latest]);
}
