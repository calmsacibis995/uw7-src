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
       screen.c
       Some functions for screen painting
          - Painting and formatting of the key menu at bottom of screen
          - Convert message status bits to string for display
          - Painting and formatting of the titlebar line on top of the screen
          - Updating of the titlebar line for changes in message number...
  ====*/


#include "headers.h"

/*
 * Internal prototypes
 */
void  savebits PROTO((bitmap_t, bitmap_t, int));
int   equalbits PROTO((bitmap_t, bitmap_t, int));
char *percentage PROTO((long, long, int));
void  output_keymenu PROTO((struct key_menu *, bitmap_t, int, int));
int   digit_count PROTO((long));
#ifdef	MOUSE
void  print_inverted_label PROTO((int, MENUITEM *));
#endif


/* Saved key menu drawing state */
static struct {
    struct key_menu *km;
    int              row,
		     column,
                     blanked;
    bitmap_t         bitmap;
} km_state;


/*
 * Longest label that can be displayed in keymenu
 */
#define	MAX_LABEL	40
#define MAX_KEYNAME	 3
static struct key last_time_buf[12];
static int keymenu_is_dirty = 1;

void
mark_keymenu_dirty()
{
    keymenu_is_dirty = 1;
}


/*
 * Write an already formatted key_menu to the screen
 *
 * Args: km     -- key_menu structure
 *       bm     -- bitmap, 0's mean don't draw this key
 *       row    -- the row on the screen to begin on, negative values
 *                 are counted from the bottom of the screen up
 *       column -- column on the screen to begin on
 *
 * The bits in the bitmap are used from least significant to most significant,
 * not left to right.  So, if you write out the bitmap in the normal way, for
 * example,
 * bm[0] = 0x5, bm[1] = 0x8, bm[2] = 0x21, bm[3] = bm[4] = bm[5] = 0
 *   0000 0101    0000 1000    0010 0001   ...
 * means that menu item 0 (first row, first column) is set, item 1 (2nd row,
 * first column) is not set, item 2 is set, items 3-10 are not set, item 11
 * (2nd row, 6th and last column) is set.  In the second menu (the second set
 * of 12 bits) items 0-3 are unset, 4 is set, 5-8 unset, 9 set, 10-11 unset.
 * That uses up bm[0] - bm[2].
 * Just to make sure, here it is drawn out for the first set of 12 items in
 * the first keymenu (0-11)
 *    bm[0] x x x x  x x x x   bm[1] x x x x  x x x x
 *          7 6 5 4  3 2 1 0                 1110 9 8
 */
void
output_keymenu(km, bm, row, column)
struct key_menu *km;
bitmap_t	 bm;
int		 row,
		 column;
{
    register struct key *k;
    struct key          *last_time;
    int                  i,
			 ufk,        /* using function keys */
			 real_row,
			 max_column, /* number of columns on screen */
			 off;        /* offset into keymap */
    int                  j;
#ifdef	MOUSE
    char		 keystr[MAX_KEYNAME + MAX_LABEL + 2];
    extern void          register_key();
#endif

    off    	  = km->which * 12;
    max_column    = ps_global->ttyo->screen_cols;

    if(ps_global->ttyo->screen_rows < 4 || max_column <= 0){
	keymenu_is_dirty = 1;
	return;
    }

    real_row = row > 0 ? row : ps_global->ttyo->screen_rows + row;

    if(keymenu_is_dirty){
	ClearLines(real_row, real_row+1);
	keymenu_is_dirty = 0;
	/* first time through, set up storage */
	if(!last_time_buf[0].name){
	    for(i = 0; i < 12; i++){
		last_time = &last_time_buf[i];
		last_time->name  = (char *)fs_get(MAX_KEYNAME + 1);
		last_time->label = (char *)fs_get(MAX_LABEL + 1);
	    }
	}

	for(i = 0; i < 12; i++)
	  last_time_buf[i].column = -1;
    }

    for(i = 0; i < 12; i++){
	int e;

	e = off + i;
        dprint(9, (debugfile, "%2d %-7.7s %-10.10s %d\n", i,
		   km == NULL ? "(no km)" 
		   : km->keys[e].name == NULL ? "(null)" 
		   : km->keys[e].name, 		   
		   km == NULL ? "(no km)" 
		   : km->keys[e].label == NULL ? "(null)" 
		   : km->keys[e].label, km ? km->keys[e].column : 0));
#ifdef	MOUSE
	register_key(i, NO_OP_COMMAND, "", NULL, 0, 0, 0);
#endif
    }

    ufk = F_ON(F_USE_FK, ps_global);
    dprint(9, (debugfile, "row: %d, real_row: %d, column: %d\n", row, 
               real_row, column));

    for(i = 0; i < 2; i++){
	int   c, el, empty, fkey, last_in_row;
	short next_col;
	char  temp[MAX_SCREEN_COLS+1];

	j = 6*i - 1;
	if(i == 1)
	  max_column--;  /* some terminals scroll if you write in the
			    lower right hand corner */

        for(c = 0, el = off+i, k = &km->keys[el];
	    k < &km->keys[off+12] && c < max_column;
	    k += 2, el += 2){

            if(k->column > max_column)
              break;

	    j++;
            if(ufk)
              fkey = 1 + k - &km->keys[off];

	    empty     = (!bitnset(el,bm) || !k->name);
	    last_time = &last_time_buf[j];
	    if(k+2 < &km->keys[off+12]){
	        last_in_row = 0;
		next_col    = last_time_buf[j+1].column;
	    }
	    else
	      last_in_row = 1;

	    if(!(k->column == last_time->column
		 && (last_in_row || (k+2)->column <= next_col)
		 && ((empty && !*last_time->label && !*last_time->name)
		     || (k->label && !strcmp(k->label,last_time->label)
			 && ((k->name && !strcmp(k->name,last_time->name))
			     || ufk))))){
		if(empty){
		    /* blank out key with spaces */
		    strcpy(temp, repeat_char(
				    ((last_in_row || (k+2)->column > max_column)
					? max_column : (k+2)->column)
					  - k->column, SPACE));
		    last_time->column  = k->column;
		    *last_time->name   = '\0';
		    *last_time->label  = '\0';
		    MoveCursor(real_row + i, column + k->column);
		    Write_to_screen(temp);
		    c = k->column + strlen(temp);
		}
		else{
		    /* short name of the key */
		    if(ufk)
		      sprintf(temp, "F%d", fkey);
		    else
		      strncpy(temp, k->name, MAX_KEYNAME);

		    temp[MAX_KEYNAME] = '\0';
		    last_time->column = k->column;
		    strcpy(last_time->name, temp);
		    /* make sure name not too long */
#ifdef	MOUSE
		    strcpy(keystr, temp);
#endif
		    MoveCursor(real_row + i, column + k->column);
		    if(!empty)
		      StartInverse();

		    Write_to_screen(temp);
		    c = k->column + strlen(temp);
		    if(!empty)
		      EndInverse();

		    /* now the space after the name and the label */
		    temp[0] = '\0';
		    if(c < max_column){
			strcpy(temp, " ");
			strncat(temp, k->label, MAX_LABEL);
			temp[max_column - c] = '\0';
			c += strlen(temp);
		    }
		    
#ifdef	MOUSE
		    strcat(keystr, temp);
#endif
		    /* fill out rest of this key with spaces */
		    if(c < max_column){
			if(last_in_row){
			    strcat(temp, repeat_char(max_column - c, SPACE));
			    c = max_column;
			}
			else{
			    if(c < (k+2)->column){
				strcat(temp,
				    repeat_char((k+2)->column - c, SPACE));
				c = (k+2)->column;
			    }
			}
		    }

		    if(k->label)
		      strcpy(last_time->label, k->label);
		    else
		      *last_time->label = '\0';

		    Write_to_screen(temp);
		}
	    }
#ifdef	MOUSE
	    else if(!empty)
	      /* fill in what register_key needs from cached data */
	      sprintf(keystr, "%s %s", last_time->name, last_time->label);

	    if(!empty)
	      register_key(j, (ufk) ? PF1 + fkey - 1
				    : (k->name[0] == '^')
					? ctrl(k->name[1])
					: (!strucmp(k->name, "ret"))
					    ? ctrl('M')
					    : (!strucmp(k->name, "tab"))
						? '\t'
						: (!strucmp(k->name, "spc"))
						    ? ' '
						    : k->name[0],
			   keystr, print_inverted_label,
			   real_row+i, k->column, strlen(keystr));
#endif

        }

	while(++j < 6*(i+1))
	  last_time_buf[j].column = -1;
    }

    fflush(stdout);
}


#ifdef	MOUSE
/*
 * print_inverted_label - highlight the label of the given menu item.
 * (callback from pico mouse routines)
 */
void
print_inverted_label(state, m)
    int state;
    MENUITEM *m;
{
    unsigned i, j;
    int      col_offset;
    char    *lp;

    /*
     * Leave the command name bold
     */
    col_offset = (state || !(lp=strchr(m->label, ' '))) ? 0 : (lp - m->label);
    MoveCursor((int)(m->tl.r), (int)(m->tl.c) + col_offset);
    if(state)
      StartInverse();
    else
      EndInverse();

    for(i = m->tl.r; i <= m->br.r; i++)
      for(j = m->tl.c + col_offset; j <= m->br.c; j++)
	if(i == m->lbl.r && j == m->lbl.c + col_offset && m->label){
	    lp = m->label + col_offset;		/* show label?? */
	    while(*lp && j++ < m->br.c)
	      Writechar((unsigned int)(*lp++), 0);

	    continue;
	}
	else
	  Writechar(' ', 0);

    if(state)
      EndInverse();			/* turn inverse back off */

}
#endif	/* MOUSE */


/*
 * Clear the key menu lines.
 */
void
blank_keymenu(row, column)
int row, column;
{
    if(FOOTER_ROWS(ps_global) > 1){
	km_state.blanked    = 1;
	km_state.row        = row;
	km_state.column     = column;
	MoveCursor(row, column);
	CleartoEOLN();
	MoveCursor(row+1, column);
	CleartoEOLN();
	fflush(stdout);
    }
}


static struct key cancel_keys[] = 
     {{NULL,NULL,KS_NONE},            {"^C","Cancel",KS_NONE},
      {NULL,NULL,KS_NONE},            {NULL,NULL,KS_NONE},
      {NULL,NULL,KS_NONE},            {NULL,NULL,KS_NONE},
      {NULL,NULL,KS_NONE},            {NULL,NULL,KS_NONE},
      {NULL,NULL,KS_NONE},            {NULL,NULL,KS_NONE},
      {NULL,NULL,KS_NONE},            {NULL,NULL,KS_NONE},
      {NULL,NULL,KS_NONE},            {NULL,NULL,KS_NONE}};
INST_KEY_MENU(cancel_keymenu, cancel_keys);

void
draw_cancel_keymenu()
{
    bitmap_t   bitmap;

    setbitmap(bitmap);
    draw_keymenu(&cancel_keymenu, bitmap, ps_global->ttyo->screen_cols,
		 1-FOOTER_ROWS(ps_global), 0, FirstMenu, 0);
}


void
clearfooter(ps)
    struct pine *ps;
{
    ClearLines(ps->ttyo->screen_rows - 3, ps->ttyo->screen_rows - 1);
    mark_keymenu_dirty();
    mark_status_unknown();
}
        

/*
 * Calculate formatting for key menu at bottom of screen
 *
 * Args:  km    -- The key_menu structure to format
 *        bm    -- Bitmap indicating which menu items should be displayed.  If
 *		   an item is NULL, that also means it shouldn't be displayed.
 *		   Sometimes the bitmap will be turned on in that case and just
 *		   rely on the NULL entry.
 *        width -- the screen width to format it at
 *	  what  -- Used to indicate which of the possible sets of 12 keys
 *		   to show next
 *	  which -- Only used when what == AParticularTwelve, in which case
 *		   it indicates which particular twelve.
 *
 * If already formatted for this particular screen width, this set of twelve
 * keys, and this part of the bitmap set this way, then return.
 *
 * The formatting results in the column field in the key_menu being
 * filled in.  The column field is the column to start the label at, the
 * name of the key; after that is the label for the key.  The basic idea
 * is to line up the end of the names and beginning of the labels.  If
 * the name is too long and shifting it left would run into previous
 * label, then shift the whole menu right, or at least that entry of
 * things following are short enough to fit back into the regular
 * spacing.  This has to be calculated and not fixed so it can cope with
 * screen resize.
 *
 * This seems to be working correctly now.  I'd like to take some time to
 * try to figure out what the heck all the calculations are about.  It seems
 * too complicated at first glance.
 */
void
format_keymenu(km, bm, width, what, which)
struct key_menu *km;
bitmap_t	 bm;
int		 width;
OtherMenu	 what;
int		 which;	 /* only used if what == AParticularTwelve */
{
    short   spacing[6];
    int     i, first, prev_end_top, prev_end_bot, ufk, top_column, bot_column;
    int     off;  /* offset into keymenu */
    struct key *keytop, *keybot, *prev_keytop, *prev_keybot;

    if(what == FirstMenu)
      km->which = 0;
    else if(what == NextTwelve)
      km->which = (km->which + 1) % (unsigned int)km->how_many;
    else if(what == AParticularTwelve)
      km->which = which;
    /* else SameTwelve */

    /* it's already formatted. */
    if(width == km->width[km->which] && equalbits(bm, km->bitmap, km->which))
      return;

    /*
     * If we're in the initial command sequence we may be using function
     * keys instead of alphas, or vice versa, so we want to recalculate
     * the formatting next time through.
     */
    if((F_ON(F_USE_FK,ps_global) && ps_global->orig_use_fkeys) ||
        (F_OFF(F_USE_FK,ps_global) && !ps_global->orig_use_fkeys)){
	km->width[km->which] = width;
	savebits(km->bitmap, bm, km->which);
    }

    off = km->which * 12;		/* off is offset into keymenu */
    ufk = F_ON(F_USE_FK,ps_global);	/* ufk stands for using function keys */

    /* this spacing is column numbers for column key name ends in */
    /*       F1 Menu	   */
    /*        ^            */
    spacing[0] = 0;
    for(i = 1; i < 6; i++) 
      spacing[i] = spacing[i-1] + (width == 80 ? (i > 2 ? 14 : 13) : width/6);

    km->keys[off].column = km->keys[off+6].column = 0;
    if(ufk)
      first = 2;
    else
      first = max((bitnset(off,bm) && km->keys[off].name != NULL) ?
	        strlen(km->keys[off].name) : 0,
	        (bitnset(off+1,bm) && km->keys[off+1].name != NULL) ?
	        strlen(km->keys[off+1].name) : 0);

    prev_keytop = &km->keys[off];
    prev_keybot = &km->keys[off+1];
    for(i = 1; i < 6; i++){
	int k_top, k_bot;

	k_top  = off + i*2;
	k_bot  = k_top + 1;
        keytop = &km->keys[k_top];
        keybot = &km->keys[k_bot];

	prev_end_top = 
	     prev_keytop->column +
	     ((!bitnset(k_top-2,bm) || prev_keytop->name == NULL) ? 0 : 
	       (ufk ? (i-1 >= 5 ? 3 : 2) : strlen(prev_keytop->name))) +
	     1 + ((!bitnset(k_top-2,bm) || prev_keytop->label==NULL) ? 0 :
	     strlen(prev_keytop->label)) +
	     1 + ((!bitnset(k_top,bm) || keytop->name == NULL) ? 0 : 
	       (ufk ? (i >= 5 ? 3 : 2) : strlen(keytop->name)));
	top_column = max(prev_end_top, first + spacing[i]);
	dprint(9, (debugfile,
		   "prev_col: %d, prev_end:%d, top_column:%d spacing:%d\n",
		   prev_keytop->column, prev_end_top, top_column,
		   spacing[i]));
	prev_end_bot = 
	     prev_keybot->column +
	     ((!bitnset(k_bot-2,bm) || prev_keybot->name  == NULL) ? 0 :
	      (ufk ? (i-1 >= 4 ? 3 : 2) :strlen(prev_keybot->name))) +
	     1 + ((!bitnset(k_bot-2,bm) || prev_keybot->label==NULL) ? 0 :
	     strlen(prev_keybot->label)) +
	     1 + ((!bitnset(k_bot,bm) || keybot->name  == NULL) ? 0 :
	      (ufk ? (i >= 4 ? 3 : 2) :strlen(keybot->name)));
	bot_column = max(prev_end_bot, first + spacing[i]);

        keytop->column = max(bot_column, top_column) -
                          ((!bitnset(k_top,bm) || keytop->name == NULL) ? 0 : 
                            (ufk ? (i >= 5 ? 3 : 2) : strlen(keytop->name)));
        keybot->column = max(bot_column, top_column) -
                          ((!bitnset(k_bot,bm) || keybot->name  == NULL) ? 0 :
                            (ufk ? (i >= 4 ? 3 : 2) : strlen(keybot->name)));
        prev_keytop = keytop;
        prev_keybot = keybot;
    }
}


/*
 * Draw the key menu at bottom of screen
 *
 * Args:  km     -- key_menu structure
 *        bitmap -- which fields are active
 *        width  -- the screen width to format it at
 *	  row    -- where to put it
 *	  column -- where to put it
 *        what   -- this is an enum telling us whether to display the
 *		    first menu (first set of 12 keys), or to display the same
 *		    one we displayed last time, or to display a particular
 *		    one (which), or to display the next one.
 *	  which  -- the keys to display if (what == AParticularTwelve)
 *
 * Fields are inactive if *either* the corresponding bitmap entry is 0 *or*
 * the actual entry in the key_menu is NULL.  Therefore, it is sometimes
 * useful to just turn on all the bits in a bitmap and let the NULLs take
 * care of it.  On the other hand, the bitmap gives a convenient method
 * for turning some keys on or off dynamically or due to options.
 * Both methods are used about equally.
 *
 * Also saves the state for a possible redraw later.
 *
 * Row should usually be a negative number.  If row is 0, the menu is not
 * drawn.
 */
void
draw_keymenu(km, bitmap, width, row, column, what, which)
struct key_menu *km;
bitmap_t	 bitmap;
int	         width;
int	         row,
                 column;
OtherMenu        what;
int	         which;
{
#ifdef _WINDOWS
    configure_menu_items (km, bitmap);
#endif
    if(row == 0)
      return;

    format_keymenu(km, bitmap, width, what, which);
    if(km_state.blanked)
      keymenu_is_dirty = 1;

    output_keymenu(km, bitmap, row, column);

    /*--- save state for a possible redraw ---*/
    km_state.km         = km;
    km_state.row        = row;
    km_state.column     = column;
    memcpy(km_state.bitmap, bitmap, BM_SIZE);
    km_state.blanked    = 0;
}


void
redraw_keymenu()
{
    if(km_state.blanked)
      blank_keymenu(km_state.row, km_state.column);
    else
      draw_keymenu(km_state.km, km_state.bitmap,
		   ps_global->ttyo->screen_cols,
		   km_state.row, km_state.column, SameTwelve, 0);
}
    

/*
 * some useful macros...
 */
#define	MS_DEL			(0x01)
#define	MS_NEW			(0x02)
#define	MS_ANS			(0x04)

#define	STATUS_BITS(X)	(!(X) ? 0					      \
			   : (X)->deleted ? MS_DEL			      \
			     : (X)->answered ? MS_ANS			      \
			       : (as.stream				      \
				  && (ps_global->unseen_in_view		      \
				      || (!(X)->seen			      \
					  && (!IS_NEWS(as.stream)	      \
					      || ((X)->recent		      \
						  && F_ON(F_FAKE_NEW_IN_NEWS, \
							  ps_global))))))     \
				      ? MS_NEW : 0)

#define	BAR_STATUS(X)	(((X) & MS_DEL) ? "DEL"   \
			 : ((X) & MS_ANS) ? "ANS"   \
		           : (as.stream		      \
			      && (!IS_NEWS(as.stream)   \
				  || F_ON(F_FAKE_NEW_IN_NEWS, ps_global)) \
			      && ((X) & MS_NEW)) ? "NEW" : "   ")

#define	BAR_NUMBER(X)



static struct titlebar_state {
    MAILSTREAM	*stream;
    MSGNO_S	*msgmap;
    char	*title,
		*folder_name,
		*context_name;
    long	 current_msg,
		 current_line,
		 total_lines;
    int		 msg_state,
		 cur_mess_col,
		 del_column, 
		 percent_column,
		 page_column,
		 screen_cols;
    enum	 {Normal, ReadOnly, Closed} stream_status;
    TitleBarType style;
} as, titlebar_stack;

    

/*----------------------------------------------------------------------
       Create little string for displaying message status

  Args: message_cache  -- pointer to MESSAGECACHE 

    Create a string with letters that indicate the status of the message.
  This is a function despite it's current simplicity so we can easily 
  add a few more flags
  ----------------------------------------------------------------------*/
char *
status_string(stream, mc)
     MAILSTREAM   *stream;
     MESSAGECACHE *mc;
{
     static char string[2] = {'\0', '\0'};

     if(!mc || ps_global->nr_mode) {
         string[0] = ' ';
         return(string);
     } 

     string[0] = (!stream || !IS_NEWS(stream)
		  || (mc->recent && F_ON(F_FAKE_NEW_IN_NEWS, ps_global)))
		   ? 'N' : ' ';

     if(mc->seen)
       string[0] = ' ';

     if(mc->answered)
       string[0] = 'A';

     if(mc->deleted)
       string[0] = 'D';

     return(string);
}



/*--------
------*/
void
push_titlebar_state()
{
    titlebar_stack     = as;
    as.folder_name     = NULL;	/* erase knowledge of malloc'd data */
    as.context_name    = NULL;
}



/*--------
------*/
void
pop_titlebar_state()
{
    fs_give((void **)&(as.folder_name)); /* free malloc'd values */
    fs_give((void **)&(as.context_name));
    as = titlebar_stack;
}



/*--------
------*/
int
digit_count(n)
    long n;
{
    int i;

    return((n > 9)
	     ? (1 + (((i = digit_count(n / 10L)) == 3 || i == 7) ? i+1 : i))
	     : 1);
}


static int titlebar_is_dirty = 1;

void
mark_titlebar_dirty()
{
    titlebar_is_dirty = 1;
}

/*----------------------------------------------------------------------
      Sets up style and contents of current titlebar line

    Args: title -- The title that appears in the center of the line
          display_on_screen -- flag whether to display on screen or generate
                                string
          style  -- The format/style of the titlebar line
	  msgmap -- MSGNO_S * to selected message map
          current_pl -- The current page or line number
          total_pl   -- The total line or page count

  Set the contents of the acnhor line. It's called an acnhor line
because it's always present and titlebars the user. This accesses a
number of global variables, but doesn't change any. There are 4
different styles of the titlebar line. First three parts are put
together and then the three parts are put together adjusting the
spacing to make it look nice. Finally column numbers and lengths of
various fields are saved to make updating parts of it more efficient.

It's OK to call this withy a bogus current message - it is only used
to look up status of current message 
 
Formats only change the right section (part3).
  FolderName:      "<folder>"  xx Messages
  MessageNumber:   "<folder>" message x,xxx of x,xxx XXX
  TextPercent:     line xxx of xxx  xx%
  MsgTextPercent:  "<folder>" message x,xxx of x,xxx  xx% XXX
  FileTextPercent: "<filename>" line xxx of xxx  xx%

Several strings and column numbers are saved so later updates to the status 
line for changes in message number or percentage can be done efficiently.
This code is some what complex, and might benefit from some improvements.
 ----*/

char *
set_titlebar(title, stream, cntxt, folder, msgmap, display_on_screen, style,
	     current_pl, total_pl)
     char        *title;
     MAILSTREAM  *stream;
     CONTEXT_S   *cntxt;
     char        *folder;
     MSGNO_S	 *msgmap;
     TitleBarType style;
     int          display_on_screen;
     long	  current_pl, total_pl;
{
    char          *tb;
    MESSAGECACHE  *mc = NULL;
    int            start_col;

    dprint(9, (debugfile, "set_titlebar - style: %d  current message cnt:%ld",
	       style, mn_total_cur(msgmap)));
    dprint(9, (debugfile, "  current_pl: %ld  total_pl: %ld\n", 
	       current_pl, total_pl));

    as.current_msg   = max(0, mn_get_cur(msgmap));
    as.msgmap	     = msgmap;
    as.style	     = style;
    as.title	     = title;
    as.stream	     = stream;
    as.stream_status = (!as.stream || (as.stream == ps_global->mail_stream
				       && ps_global->dead_stream))
			 ? Closed : as.stream->rdonly ? ReadOnly : Normal;

    if(as.folder_name)
      fs_give((void **)&as.folder_name);

    as.folder_name = cpystr(pretty_fn(folder));

    if(as.context_name)
      fs_give((void **)&as.context_name);

    /*
     * Handle setting up the context if appropriate.
     */
    if(cntxt && context_isambig(folder) && ps_global->context_list->next
       && strucmp(as.folder_name, ps_global->inbox_name)){
	/*
	 * if there's more than one context and the current folder
	 * is in it (ambiguous name), set the context name...
	 */
	as.context_name = cpystr(cntxt->label[0]);
    }
    else
      as.context_name = cpystr("");

    if(mn_get_total(msgmap) < 1L)
      mn_set_cur(msgmap, 0L);		/* BUG Don't like this */

    if(as.stream && style != FolderName && mn_get_cur(msgmap) > 0L) {
        (void)mail_fetchstructure(as.stream,
				  mn_m2raw(msgmap, as.current_msg), NULL);
        mc  = mail_elt(as.stream, mn_m2raw(msgmap, as.current_msg));
    }
    
    switch(style) {
      case FolderName:
	break;

      case MessageNumber:
        as.total_lines = total_pl;
        as.current_line = current_pl;
        as.msg_state = STATUS_BITS(mc);
	break;

      case TextPercent:
      case MsgTextPercent:
      case FileTextPercent :
        as.total_lines = total_pl;
        as.current_line = current_pl;
        as.msg_state = STATUS_BITS(mc);
	break; 
    }

    tb = format_titlebar(&start_col);
    if(display_on_screen){
        StartInverse();
        PutLine0(0, start_col, tb+start_col);
        EndInverse();
        fflush(stdout);
    }

    return(tb);
}

void 
redraw_titlebar()
{
    int   start_col;
    char *tb;

    StartInverse();
    tb = format_titlebar(&start_col);
    PutLine0(0, start_col, tb+start_col);
    EndInverse();
    fflush(stdout);
}
     



/*----------------------------------------------------------------------
      Redraw or draw the top line, the title bar 

 The titlebar has Four fields:
     1) "Version" of fixed length and is always positioned two spaces 
        in from left display edge.
     2) "Location" which is fixed for each style of titlebar and
        is positioned two spaces from the right display edge
     3) "Title" which is of fixed length, and is centered if
        there's space
     4) "Folder" whose existance depends on style and which can
        have it's length adjusted (within limits) so it will
        equally share the space between 1) and 2) with the 
        "Title".  The rule for existance is that in the
        space between 1) and 2) there must be two spaces between
        3) and 4) AND at least 50% of 4) must be displayed.


 The rules for dislay are:
     a) Show at least some portion of 3)
     b) If no room for 1) and 3), 3)
     c) If no room for 1), 2) and 3), show 1) and 2)
     d) If no room for all and > 50% of 4), show 1), 2), and 3)
     e) show 1), 2) 3), and some portion of 4)

   Returns - Formatted title bar 
	   - Start_col will point to column to start printing in on return.
 ----*/
char *
format_titlebar(start_col)
    int *start_col;
{
    static  char titlebar_line[MAX_SCREEN_COLS+1];
    char    version[50], fold_tmp[MAXPATH],
           *loc_label, *ss_string;
    int     sc, tit_len, ver_len, loc_len, fold_len, num_len, ss_len, 
            is_context;

    if(start_col)
      *start_col = 0; /* default */

    /* blank the line */
    memset((void *)titlebar_line, ' ', MAX_SCREEN_COLS*sizeof(char));
    sc = min(ps_global->ttyo->screen_cols, MAX_SCREEN_COLS);
    titlebar_line[sc] = '\0';

    /* initialize */
    as.del_column     = -1;
    as.cur_mess_col   = -1;
    as.percent_column = -1;
    as.page_column    = -1;
    is_context        = strlen(as.context_name);
    sprintf(version, "PINE %s", pine_version); 
    ss_string         = as.stream_status == Closed ? "(CLOSED)" :
                        (as.stream_status == ReadOnly
			 && !IS_NEWS(as.stream))
                           ? "(READONLY)" : "";
    ss_len            = strlen(ss_string);

    tit_len = strlen(as.title);		/* fixed title field width   */
    ver_len = strlen(version);		/* fixed version field width */

    /* if only room for title we can get out early... */
    if(tit_len >= sc || (tit_len + ver_len + 6) > sc){
	int i = max(0, sc - tit_len)/2;
	strncpy(titlebar_line + i, as.title, min(sc, tit_len));
	titlebar_is_dirty = 0;
	return(titlebar_line);
    }

    /* 
     * set location field's length and value based on requested style 
     */
    loc_label = (is_context) ? "Msg" : "Message";
    loc_len   = strlen(loc_label);
    switch(as.style){
      case FolderName :			/* "x,xxx <loc_label>s" */
	loc_len += digit_count(mn_get_total(as.msgmap)) + 3;
	sprintf(tmp_20k_buf, "%s %s%s", comatose(mn_get_total(as.msgmap)),
		loc_label, plural(mn_get_total(as.msgmap)));
	break;
      case MessageNumber :	       	/* "<loc_label> xxx of xxx DEL"  */
	num_len		 = digit_count(mn_get_total(as.msgmap));
	loc_len		+= (2 * num_len) + 9;	/* add spaces and "DEL" */
	as.cur_mess_col  = sc - (2 * num_len) - 10;
	as.del_column    = as.cur_mess_col + num_len 
			    + digit_count(as.current_msg) + 5;
	sprintf(tmp_20k_buf, "%s %s of %s %s", loc_label,
		strcpy(tmp_20k_buf + 1000, comatose(as.current_msg)),
		strcpy(tmp_20k_buf + 1500, comatose(mn_get_total(as.msgmap))),
		BAR_STATUS(as.msg_state));
	break;
      case MsgTextPercent :		/* "<loc_label> xxx of xxx xxx% DEL" */
	num_len		   = digit_count(mn_get_total(as.msgmap));
	loc_len		  += (2 * num_len) + 13; /* add spaces, %, and "DEL" */
	as.cur_mess_col    = sc - 16 - (2 * num_len);
	as.percent_column  = as.cur_mess_col + num_len
			      + digit_count(as.current_msg) + 7;
	as.del_column      = as.percent_column + 4;
	sprintf(tmp_20k_buf, "%s %s of %s %s %s", loc_label, 
		strcpy(tmp_20k_buf + 1000, comatose(as.current_msg)),
		strcpy(tmp_20k_buf + 1500, comatose(mn_get_total(as.msgmap))),
		percentage(as.current_line, as.total_lines, 1),
		BAR_STATUS(as.msg_state));
	break;
      case TextPercent :
	/* NOTE: no fold_tmp setup below for TextPercent style */
      case FileTextPercent :
	as.page_column = sc - (14 + 2*(num_len = digit_count(as.total_lines)));
	loc_len        = 17 + 2*num_len;
	sprintf(tmp_20k_buf, "Line %*ld of %*ld %s    ",
		num_len, as.current_line, 
		num_len, as.total_lines,
		percentage(as.current_line, as.total_lines, 1));
	break;
    }

    /* at least the version will fit */
    strncpy(titlebar_line + 2, version, ver_len);
    if(!titlebar_is_dirty && start_col)
      *start_col = 2 + ver_len;

    titlebar_is_dirty = 0;

    /* if no room for location string, bail early? */
    if(ver_len + tit_len + loc_len + 10 > sc){
	strncpy((titlebar_line + sc) - (tit_len + 2), as.title, tit_len);
        as.del_column = as.cur_mess_col = as.percent_column
	  = as.page_column = -1;
	return(titlebar_line);		/* put title and leave */
    }

    /* figure folder_length and what's to be displayed */
    fold_tmp[0] = '\0';
    if(as.style == FileTextPercent || as.style == TextPercent){
	if(as.style == FileTextPercent && !ps_global->anonymous){
	    char *fmt    = "File: %s%s";
	    int   avail  = sc - (ver_len + tit_len + loc_len + 10);
	    fold_len     = strlen(as.folder_name);
	    if(fold_len + 6 < avail) 	/* all of folder fit? */
	      sprintf(fold_tmp, fmt, "", as.folder_name);
	    else if((fold_len/2) + 9 < avail)
	      sprintf(fold_tmp, fmt, "...",
		      as.folder_name + fold_len - (avail - 9));
	}
	/* else leave folder/file name blank */
    }
    else{
	int ct_len,
	    avail  = sc - (ver_len + tit_len + loc_len + 10);
	fold_len   = strlen(as.folder_name);
	
	if(is_context
	  && as.stream_status != Closed
	  && (ct_len = strlen(as.context_name))){
	    char *fmt;
	    int  extra;

	    fmt = "<%*.*s> %s%s"; extra = 3;

	    /*
	     * below are other formats we'd considered
	     *
	     * fmt = "%s - %s%s"; extra = 3;
	     * fmt = "%s[%s%s]"; extra = 2;
	     * fmt = "<%s>%s%s"; extra = 2;
	     * fmt = "%s: %s%s"; extra = 2;
	     */
	    if(ct_len + fold_len + ss_len + extra < avail)
	      sprintf(fold_tmp, fmt, ct_len, ct_len, as.context_name,
		      as.folder_name, ss_string);
	    else if((ct_len/2) + fold_len + ss_len + extra < avail)
	      sprintf(fold_tmp, fmt,
		      ct_len - (ct_len-(avail-(fold_len+ss_len+extra))),
		      ct_len - (ct_len-(avail-(fold_len+ss_len+extra))),
		      as.context_name,
		      as.folder_name, ss_string);
	    else if((ct_len/2) + (fold_len/2) + ss_len + extra < avail)
	      sprintf(fold_tmp, fmt, (ct_len/2), (ct_len/2), as.context_name,
		   as.folder_name+(fold_len-(avail-((ct_len/2)+ss_len+extra))),
		      ss_string);
	}
	else{
	    char *fmt = "Folder: %s%s";
	    if(fold_len + ss_len + 8 < avail) 	/* all of folder fit? */
	      sprintf(fold_tmp, fmt, as.folder_name, ss_string);
	    else if((fold_len/2) + ss_len + 8 < avail)
	      sprintf(fold_tmp, fmt, 
		      as.folder_name + fold_len - (avail - (8 + ss_len)),
		      ss_string);
	}
    }
    
    /* write title, location and, optionally, the folder name */
    fold_len = strlen(fold_tmp);
    strncpy(titlebar_line + ver_len + 5, as.title, tit_len);
    strncpy((titlebar_line + sc) - (loc_len + 2), tmp_20k_buf, 
	    strlen(tmp_20k_buf));
    if(fold_len)
      strncpy((titlebar_line + sc) - (loc_len + fold_len + 4), fold_tmp,
	      fold_len);

    return(titlebar_line);
}


/*----------------------------------------------------------------------
    Update the titlebar line if the message number changed

   Args: None, uses state setup on previous call to set_titlebar.

This is a bit messy because the length of the number displayed might 
change which repositions everything after it, so we adjust all the saved 
columns and shorten tail, the string holding the rest of the line.
  ----*/

void
update_titlebar_message()
{
    int delta;

    if(as.cur_mess_col < 0)
      return;

    delta = digit_count(mn_get_cur(as.msgmap)) - digit_count(as.current_msg);

    StartInverse();
    if(delta) {
	as.current_msg = mn_get_cur(as.msgmap);

        if(as.style == MsgTextPercent){
            PutLine5(0, as.cur_mess_col, "%s of %s %s %s%s",
		     strcpy(tmp_20k_buf + 1000, comatose(as.current_msg)),
		     strcpy(tmp_20k_buf + 1500,
			    comatose(mn_get_total(as.msgmap))),
                     percentage(as.current_line, as.total_lines, 0),
                     BAR_STATUS(as.msg_state),
                     repeat_char(max(0, -delta), ' '));
	    as.del_column     += delta;
	    as.percent_column += delta;
        } else {
            PutLine4(0, as.cur_mess_col, "%s of %s %s%s",
		     strcpy(tmp_20k_buf + 1000, comatose(as.current_msg)),
		     strcpy(tmp_20k_buf + 1500,
			    comatose(mn_get_total(as.msgmap))),
		     BAR_STATUS(as.msg_state),
                     repeat_char(max(0, -delta),' '));
	    as.del_column += delta;
        }
    }
    else if(mn_get_cur(as.msgmap) != as.current_msg){
	as.current_msg = mn_get_cur(as.msgmap);
	PutLine0(0, as.cur_mess_col, comatose(as.current_msg));
    }

    EndInverse();
    fflush(stdout);
}



/*----------------------------------------------------------------------
    Update titlebar line's message status field ("DEL", "NEW", etc)

  Args:  None, operates on state set during most recent set_titlebar call

  ---*/
void
update_titlebar_status()
{
    MESSAGECACHE *mc;
    
    if(!as.stream || as.current_msg < 0L || as.del_column < 0)
      return;

    mc = mail_elt(as.stream, mn_m2raw(as.msgmap, as.current_msg));

    if(mc->deleted){			/* deleted takes precedence */
	if(as.msg_state & MS_DEL)
	  return;
    }
    else if(mc->answered){		/* then answered */
	if(as.msg_state & MS_ANS)
	  return;
    }
    else if(!mc->seen && as.stream
	    && (!IS_NEWS(as.stream)
		|| (mc->recent && F_ON(F_FAKE_NEW_IN_NEWS, ps_global)))){
	if(as.msg_state & MS_NEW)	/* then seen */
	  return;
    }
    else if(IS_NEWS(as.stream) && F_ON(F_FAKE_NEW_IN_NEWS, ps_global)){
    }
    else if(as.msg_state == 0)		/* nothing to change... */
      return;

    as.msg_state = STATUS_BITS(mc);
    StartInverse();
    PutLine0(0, as.del_column, BAR_STATUS(as.msg_state));
    EndInverse();
    fflush(stdout);
}



/*---------------------------------------------------------------------- 
    Update the percentage shown in the titlebar line

  Args: new_line_number -- line number to calculate new percentage
   
  ----*/

void
update_titlebar_percent(line)
    long line;
{
    if(as.percent_column < 0)
      return;

    StartInverse();
    PutLine0(0, as.percent_column,
	     percentage(as.current_line = line, as.total_lines, 0));
    EndInverse();
    fflush(stdout);
}



/*---------------------------------------------------------------------- 
    Update the percentage AND line number shown in the titlebar line

  Args: new_line_number -- line number to calculate new percentage
   
  ----*/

void
update_titlebar_lpercent(new_line_number)
    long new_line_number;
{
    if(as.page_column < 0 || new_line_number == as.current_line)
      return;

    as.current_line = new_line_number;

    sprintf(tmp_20k_buf, "%*ld of %*ld %s    ",
	    digit_count(as.total_lines), as.current_line, 
	    digit_count(as.total_lines), as.total_lines,
	    percentage(as.current_line, as.total_lines, 0));
    StartInverse();
    PutLine0(0, as.page_column, tmp_20k_buf);
    EndInverse();
    fflush(stdout);
}



/*----------------------------------------------------------------------
    Return static buf containing portion of lines displayed

  Args:  part -- how much so far
	 total -- how many total

  ---*/
char *
percentage(part, total, suppress_top)
    long part, total;
    int  suppress_top;
{
    static char percent[4];

    if(total == 0L || (total <= ps_global->ttyo->screen_rows
				 - HEADER_ROWS(ps_global)
				 - FOOTER_ROWS(ps_global)))
      strcpy(percent, "ALL");
    else if(!suppress_top && part <= ps_global->ttyo->screen_rows
				      - HEADER_ROWS(ps_global)
				      - FOOTER_ROWS(ps_global))
      strcpy(percent, "TOP");
    else if(part >= total)
      strcpy(percent, "END");
    else
      sprintf(percent, "%2ld%%", (100L * part)/total);

    return(percent);
}




/*
 * end_titlebar - free resources associated with titlebar state struct
 */
void
end_titlebar()
{
    if(as.folder_name)
      fs_give((void **)&as.folder_name);

    if(as.context_name)
      fs_give((void **)&as.context_name);
}


/*
 * end_keymenu - free resources associated with keymenu display cache
 */
void
end_keymenu()
{
    int i;

    for(i = 0; i < 12; i++){
	if(last_time_buf[i].name)
	  fs_give((void **)&last_time_buf[i].name);

	if(last_time_buf[i].label)
	  fs_give((void **)&last_time_buf[i].label);
    }
}


/*
 * Save the bits from the which'th set of twelve bits in from into to.
 * An assumption is that bitmap_t's are 48 bits longs.  (There must be a
 * clever macro way to do this.)
 */
void
savebits(to, from, which)
bitmap_t to,
	 from;
int	 which;
{
    unsigned char c1, c2;
    int x;

    switch(which){
      case 0:
      case 2:
	x       =  which == 0 ? 0 : 3;
	to[x]   =  from[x];
	c1      =  from[x+1]  &  0x0f;
	c2      =  to[x+1]    &  0xf0;
	to[x+1] =  c1         |  c2;
	break;

      case 1:
      case 3:
	x       =  which == 1 ? 1 : 4;
	c1      =  to[x]    &  0x0f;
	c2      =  from[x]  &  0xf0;
	to[x]   =  c1       |  c2;
	to[x+1] =  from[x+1];
	break;

      default:
	panic("Can't happen in savebits()\n");
	break;
    }
}


/*
 * Returns 1 if the which'th set of twelve bits in bm1 is the same as the
 * which'th set of twelve bits in bm2, else 0.
 */
int
equalbits(bm1, bm2, which)
    bitmap_t bm1, bm2;
    int	     which;
{
    int x;

    switch(which){
      case 0:
      case 2:
	x = which == 0 ? 0 : 3;
	return ((bm1[x] == bm2[x]) && ((bm1[x+1] & 0x0f) == (bm2[x+1] & 0x0f)));

      case 1:
      case 3:
	x = which == 1 ? 1 : 4;
	return (((bm1[x] & 0xf0) == (bm2[x] & 0xf0)) && (bm1[x+1] == bm2[x+1]));

      default:
	panic("Can't happen in equalbits()\n");
    }
}
