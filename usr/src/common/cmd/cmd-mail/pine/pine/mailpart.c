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
     mailpart.c
     The meat and pototoes of attachment processing here.

  ====*/

#include "headers.h"


/*
 * Information used to paint and maintain a line on the attachment
 * screen.
 */
typedef struct atdisp_line {
    char	     *dstring;			/* alloc'd var value string  */
    ATTACH_S	     *attp;			/* actual attachment pointer */
    struct atdisp_line *next, *prev;
} ATDISP_S;


/*
 * struct defining attachment screen's current state
 */
typedef struct att_screen {
    ATDISP_S  *current,
	      *top_line;
} ATT_SCREEN_S;
static ATT_SCREEN_S *att_screen;


#define	next_attline(p)	((p) ? (p)->next : NULL)
#define	prev_attline(p)	((p) ? (p)->prev : NULL)


static struct key att_index_keys[] = 
       {{"?","Help",KS_SCREENHELP},	{NULL,NULL,KS_NONE},
	{"E","Exit Index",KS_EXITMODE},	{"V","[View]",KS_VIEW},
	{"P","PrevAttch",KS_NONE},	{"N","NextAttch",KS_NONE},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{"A","AboutAttch",KS_NONE},	{"S","Save",KS_SAVE},
	{"|","Pipe",KS_NONE},		{"W","WhereIs",KS_WHEREIS}};
INST_KEY_MENU(att_index_keymenu, att_index_keys);
#define	ATT_PIPE_KEY	10

/* used to keep track of detached MIME segments total length */
static long	save_att_length;

/*
 * Internal Prototypes
 */
int	  display_attachment PROTO((long, ATTACH_S *));
void	  display_text_att PROTO((long, ATTACH_S *));
void	  display_msg_att PROTO((long, ATTACH_S *));
void	  display_abook_att PROTO((long, ATTACH_S *));
void	  display_attach_info PROTO((long, ATTACH_S *));
void	  run_viewer PROTO((char *, BODY *));
ATDISP_S *new_attline PROTO((ATDISP_S **));
void	  free_attline PROTO((ATDISP_S **));
ATDISP_S *first_attline PROTO((ATDISP_S *));
void	  attachment_screen_redrawer PROTO(());
int	  attachment_screen_updater PROTO((struct pine *, ATDISP_S *, \
					   ATT_SCREEN_S *));
int	  init_att_progress PROTO((char *, MAILSTREAM *, BODY *));
long	  save_att_piped PROTO((int));
int	  save_att_percent PROTO(());
int	  df_trigger_cmp PROTO((long, char *));
int	  df_trigger_cmp_text PROTO((char *, char *));
int	  df_trigger_cmp_lwsp PROTO((char *, char *));
int	  df_trigger_cmp_start PROTO((char *, char *));
int	  df_static_trigger PROTO((BODY *));
int	  df_valid_command PROTO((char **));
char	 *dfilter PROTO((char *, STORE_S *, gf_io_t, filter_t *));


#if defined(DOS) || defined(OS2)
#define	READ_MODE	"rb"
#define	WRITE_MODE	"wb"
#else
#define	READ_MODE	"r"
#define	WRITE_MODE	"w"
#endif



/*----------------------------------------------------------------------
   Provide attachments in browser for selected action

  Args: ps -- pointer to pine structure
	msgmap -- struct containing current message to display

  Result: 

  Handle painting and navigation of attachment index.  It would be nice
  to someday handle message/rfc822 segments in a neat way (i.e., enable
  forwarding, take address, etc.).
 ----*/
void
attachment_screen(ps, msgmap)
    struct pine *ps;
    MSGNO_S     *msgmap;
{
    int		  i, ch = 'x', orig_ch, done = 0, changes = 0, dline,
		  nl = 0, sl = 0, old_cols = -1, km_popped = 0;
    long	  msgno;
    char	 *p, *q;
    ATTACH_S	 *atmp;
    ATDISP_S	 *current = NULL, *ctmp = NULL;
    ATT_SCREEN_S  screen;

    if(mn_total_cur(msgmap) > 1L){
	q_status_message(SM_ORDER | SM_DING, 0, 3,
			 "Can only view one message's attachments at a time!");
	return;
    }
    else if(ps->atmts && !(ps->atmts + 1)->description)
      q_status_message1(SM_ASYNC, 0, 3,
	"Message %s has only one part (the message body), and no attachments.",
	long2string(mn_get_cur(msgmap)));

    /*
     * find the longest number and size strings
     */
    for(atmp = ps->atmts; atmp && atmp->description; atmp++){
	if((i = strlen(atmp->number)) > nl)
	  nl = i;

	if((i = strlen(atmp->size)) > sl)
	  sl = i;
    }

    /*
     * Then, allocate and initialize attachment line list...
     */
    for(atmp = ps->atmts; atmp && atmp->description; atmp++)
      new_attline(&current)->attp = atmp;

    screen.current     = screen.top_line = NULL;
    msgno	       = mn_m2raw(msgmap, mn_get_cur(msgmap));
    ps->mangled_screen = 1;			/* build display */
    ps->redrawer       = attachment_screen_redrawer;
    att_screen	       = &screen;
    current	       = first_attline(current);
    if(ctmp = next_attline(current))
      current = ctmp;

    while(!done){
	if(km_popped){
	    km_popped--;
	    if(km_popped == 0){
		clearfooter(ps);
		ps->mangled_body = 1;
	    }
	}

	if(ps->mangled_screen){
	    /*
	     * build/rebuild display lines
	     */
	    if(old_cols != ps->ttyo->screen_cols){
		old_cols = ps->ttyo->screen_cols;
		for(ctmp = first_attline(current);
		    ctmp && (atmp = ctmp->attp) && atmp->description;
		    ctmp = next_attline(ctmp)){
		    size_t len, dlen;

		    if(ctmp->dstring)
		      fs_give((void **)&ctmp->dstring);

		    len = max(80, ps->ttyo->screen_cols) * sizeof(char);
		    ctmp->dstring = (char *)fs_get(len + 1);
		    ctmp->dstring[len] = '\0';
		    memset(ctmp->dstring, ' ', len);
		    p = ctmp->dstring + 3;
		    for(i = 0; atmp->number[i]; i++)
		      *p++ = atmp->number[i];

		    /* add 3 spaces, plus pad number, and right justify size */
		    p += 3 + nl - i + (sl - strlen(atmp->size));

		    for(i = 0; atmp->size[i]; i++)
		      *p++ = atmp->size[i];

		    p += 3;

		    /* copy the type description */
		    q = type_desc(atmp->body->type, atmp->body->subtype,
				  atmp->body->parameter, 1);
		    if((dlen = strlen(q)) > (i = len - (p - ctmp->dstring)))
		      dlen = i;

		    strncpy(p, q, dlen);
		    p += dlen;

		    /* provided there's room, copy the user's description */
		    if(len - (p - ctmp->dstring) > 8
		       && ((q = atmp->body->description)
			   || (atmp->body->type == TYPEMESSAGE
			       && atmp->body->subtype
			       && strucmp(atmp->body->subtype, "rfc822") == 0
			       && atmp->body->contents.msg.env
			       && (q=atmp->body->contents.msg.env->subject)))){
			sstrcpy(&p, ", \"");
			if((dlen=strlen(q)) > (i=len - (p - ctmp->dstring)))
			  dlen = i;

			strncpy(p, q, dlen);
			*(p += dlen) = '\"';
		    }
		}
	    }

	    ps->mangled_header = 1;
	    ps->mangled_footer = 1;
	    ps->mangled_body   = 1;
	}

	/*----------- Check for new mail -----------*/
        if(new_mail(0, NM_TIMING(ch), 1) >= 0)
          ps->mangled_header = 1;

	if(ps->mangled_header){
	    set_titlebar("ATTACHMENT INDEX", ps->mail_stream,
			 ps->context_current, ps->cur_folder, ps->msgmap, 1,
			 MessageNumber, 0, 0);
	    ps->mangled_header = 0;
	}

	if(ps->mangled_screen){
	    ClearLine(1);
	    ps->mangled_screen = 0;
	}

	dline = attachment_screen_updater(ps, current, &screen);

	/*---- This displays new mail notification, or errors ---*/
	if(km_popped){
	    FOOTER_ROWS(ps) = 3;
	    mark_status_unknown();
	}

        display_message(ch);
	if(km_popped){
	    FOOTER_ROWS(ps) = 1;
	    mark_status_unknown();
	}

        /*------ Read the command from the keyboard ----*/
	if(ps->mangled_footer){
	    bitmap_t	 bitmap;

	    setbitmap(bitmap);
	    ps->mangled_footer = 0;
	    if(F_OFF(F_ENABLE_PIPE, ps))
	      clrbitn(ATT_PIPE_KEY, bitmap);	/* always clear for DOS */

	    if(km_popped){
		FOOTER_ROWS(ps) = 3;
		clearfooter(ps);
	    }

	    draw_keymenu(&att_index_keymenu, bitmap, ps->ttyo->screen_cols,
			 1-FOOTER_ROWS(ps), 0, FirstMenu,0);
	    if(km_popped){
		FOOTER_ROWS(ps) = 1;
		mark_keymenu_dirty();
	    }
	}

	if(F_ON(F_SHOW_CURSOR, ps))
	  MoveCursor(dline, 0);
	else
	  MoveCursor(max(0, ps->ttyo->screen_rows - FOOTER_ROWS(ps)), 0);

#ifdef	MOUSE
	mouse_in_content(KEY_MOUSE, -1, -1, 0, 0); /* prime the handler */
	register_mfunc(mouse_in_content, HEADER_ROWS(ps_global), 0,
		       ps_global->ttyo->screen_rows -(FOOTER_ROWS(ps_global)+1),
		       ps_global->ttyo->screen_cols);
#endif
	ch = orig_ch = read_command();
#ifdef	MOUSE
	clear_mfunc(mouse_in_content);
#endif

        if(ch <= 0xff && isupper((unsigned char)ch))
          ch = tolower((unsigned char)ch);

	if(km_popped)
	  switch(ch){
	    case NO_OP_IDLE:
	    case NO_OP_COMMAND: 
	    case KEY_RESIZE:
	    case ctrl('L'):
	      km_popped++;
	      break;
	    
	    default:
	      clearfooter(ps);
	      break;
	  }

	switch(ch){
	  case '?' :				/* help! */
	  case ctrl('G'):
	  case PF1 :
	    if(FOOTER_ROWS(ps) == 1 && km_popped == 0){
		km_popped = 2;
		ps->mangled_footer = 1;
		break;
	    }

	    helper(h_attachment_screen, "HELP FOR ATTACHMENT INDEX", 0);
	    ps->mangled_screen = 1;
	    break;

	  case 'e' :				/* exit attachment screen */
	  case PF3 :
	    done++;
	    break;

	  case 'n' :				/* next list element */
	  case '\t' :
	  case ctrl('F') :
	  case KEY_RIGHT :
	  case ctrl('N'):			/* down arrow */
	  case KEY_DOWN :
	  case PF6 :
	    ch = KEY_DOWN;
	    if(ctmp = next_attline(current))
	      current = ctmp;
	    else
	      q_status_message(SM_ORDER, 0, 1,
			       "Already on last attachment");

	    break;

	  case 'p' :				/* previous list element */
	  case ctrl('B') :
	  case KEY_LEFT :
	  case ctrl('P') :			/* up arrow */
	  case KEY_UP :
	  case PF5 :
	    ch = KEY_UP;
	    if(ctmp = prev_attline(current))
	      current = ctmp;
	    else
	      q_status_message(SM_ORDER, 0, 1,
			       "Already on first attachment");

	    break;

	  case '+' :				/* page forward */
	  case ' ' :
	  case ctrl('V') :
	  case PF8 :
	    ch = KEY_DOWN;
	    if(next_attline(current)){
		while(dline++ < ps->ttyo->screen_rows - FOOTER_ROWS(ps))
		  if(ctmp = next_attline(current))
		    current = ctmp;
		  else
		    break;
	    }
	    else
	      q_status_message(SM_ORDER, 0, 1,
			       "Already on last page of attachments");
	    

	    break;

	  case '-' :				/* page backward */
	  case ctrl('Y') :
	  case PF7 :
	    ch = KEY_UP;
	    if(prev_attline(current)){
		while(dline-- > HEADER_ROWS(ps))
		  if(ctmp = prev_attline(current))
		    current = ctmp;
		  else
		    break;

		while(++dline < ps->ttyo->screen_rows - FOOTER_ROWS(ps))
		  if(ctmp = prev_attline(current))
		    current = ctmp;
		  else
		    break;
	    }
	    else
	      q_status_message(SM_ORDER, 0, 1,
			       "Already on first page of attachments");

	    break;

#ifdef MOUSE	    
	  case KEY_MOUSE:
	    {
		MOUSEPRESS mp;

		mouse_get_last (NULL, &mp);
		mp.row -= HEADER_ROWS(ps);
		ctmp = screen.top_line;
		if (mp.doubleclick) {
		    display_attachment(msgno, current->attp);
		}
		else {
		    while (mp.row && ctmp != NULL) {
			--mp.row;
			ctmp = ctmp->next;
		    }
		    if (ctmp != NULL)
		      current = ctmp;
		}
	    }
	    break;
#endif

	  case 'w' :				/* whereis */
	  case ctrl('W') :
	  case PF12 :
	    /*--- get string  ---*/
	    {int   rc, found = 0;
	     char *result = NULL, buf[64];
	     static char last[64], tmp[64];
	     HelpType help;

	     ps->mangled_footer = 1;
	     buf[0] = '\0';
	     sprintf(tmp, "Word to find %s%.40s%s: ",
		     (last[0]) ? "[" : "",
		     (last[0]) ? last : "",
		     (last[0]) ? "]" : "");
	     help = NO_HELP;
	     while(1){
		 rc = optionally_enter(buf,-FOOTER_ROWS(ps),0,63,1,0,
					 tmp,NULL,help,0);
		 if(rc == 3)
		   help = help == NO_HELP ? h_attach_index_whereis : NO_HELP;
		 else if(rc == 0 || rc == 1 || !buf[0]){
		     if(rc == 0 && !buf[0] && last[0])
		       strcpy(buf, last);

		     break;
		 }
	     }

	     if(rc == 0 && buf[0]){
		 ch   = KEY_DOWN;
		 ctmp = current;
		 while(ctmp = next_attline(ctmp))
		   if(srchstr(ctmp->dstring, buf)){
		       found++;
		       break;
		   }

		 if(!found){
		     ctmp = first_attline(current);

		     while(ctmp != current)
		       if(srchstr(ctmp->dstring, buf)){
			   found++;
			   break;
		       }
		       else
			 ctmp = next_attline(ctmp);
		 }
	     }
	     else
	       result = "WhereIs cancelled";

	     if(found && ctmp){
		 strcpy(last, buf);
		 result  = "Word found";
		 current = ctmp;
	     }

	     q_status_message(SM_ORDER, 0, 3,
			      result ? result : "Word not found");
	    }

	    break;

	  case 'a' :
	  case PF9 :
	    display_attach_info(msgno, current->attp);
	    break;

	  case ctrl('L'):			/* redraw */
          case KEY_RESIZE:
	    ps->mangled_screen = 1;
	    break;

	  case 'v':				/* View command */
	  case ctrl('M'):
	  case PF4 :
	    display_attachment(msgno, current->attp);
	    break;

	  case 's':				/* Save command */
	  case PF10 :
	    save_attachment(-FOOTER_ROWS(ps), msgno, current->attp);
	    ps->mangled_footer = 1;
	    break;

	  case '|':				/* Pipe command */
	  case PF11 :
	    if(F_ON(F_ENABLE_PIPE, ps)){
		pipe_attachment(msgno, current->attp);
		ps->mangled_footer = 1;
		break;
	    }					/* fall thru to complain */

	  default:
	    bogus_command(orig_ch, F_ON(F_USE_FK, ps) ? "F1" : "?");

	  case NO_OP_IDLE:			/* simple timeout */
	  case NO_OP_COMMAND:
	    break;
	}
    }

    for(current = first_attline(current); current;){	/* clean up */
	ctmp = current->next;
	free_attline(&current);
	current = ctmp;
    }

    ps->mangled_screen = 1;
    return;
}



/*----------------------------------------------------------------------
  allocate and attach a fresh attachment line struct

  Args: current -- display line to attache new struct to

  Returns: newly alloc'd attachment display line
  ----*/
ATDISP_S *
new_attline(current)
    ATDISP_S **current;
{
    ATDISP_S *p;

    p = (ATDISP_S *)fs_get(sizeof(ATDISP_S));
    memset((void *)p, 0, sizeof(ATDISP_S));
    if(current){
	if(*current){
	    p->next	     = (*current)->next;
	    (*current)->next = p;
	    p->prev	     = *current;
	    if(p->next)
	      p->next->prev = p;
	}

	*current = p;
    }

    return(p);
}



/*----------------------------------------------------------------------
  Release system resources associated with attachment display line

  Args: p -- line to free

  Result: 
  ----*/
void
free_attline(p)
    ATDISP_S **p;
{
    if(p){
	if((*p)->dstring)
	  fs_give((void **)&(*p)->dstring);

	fs_give((void **)p);
    }
}



/*----------------------------------------------------------------------
  Manage display of the attachment screen menu body.

  Args: ps -- pine struct pointer
	current -- currently selected display line
	screen -- reference points for display painting

  Result: 
 */
int
attachment_screen_updater(ps, current, screen)
    struct pine  *ps;
    ATDISP_S	 *current;
    ATT_SCREEN_S *screen;
{
    int	      dline, return_line = HEADER_ROWS(ps);
    ATDISP_S *top_line, *ctmp;

    /* calculate top line of display */
    dline = 0;
    ctmp = top_line = first_attline(current);
    do
      if(((dline++)%(ps->ttyo->screen_rows-HEADER_ROWS(ps)-FOOTER_ROWS(ps)))==0)
	top_line = ctmp;
    while(ctmp != current && (ctmp = next_attline(ctmp)));

#ifdef _WINDOWS
    /* Don't know how to manage scroll bar for attachment screen yet. */
    scroll_setrange (0L);
#endif

    /* mangled body or new page, force redraw */
    if(ps->mangled_body || screen->top_line != top_line)
      screen->current = NULL;

    /* loop thru painting what's needed */
    for(dline = 0, ctmp = top_line;
	dline < ps->ttyo->screen_rows - FOOTER_ROWS(ps) - HEADER_ROWS(ps);
	dline++, ctmp = next_attline(ctmp)){

	/*
	 * only fall thru painting if something needs painting...
	 */
	if(!(!screen->current || ctmp == screen->current || ctmp == current))
	  continue;

	if(ctmp && ctmp->dstring){
	    char *p = tmp_20k_buf;
	    int   i, j, x = 0;
	    if(F_ON(F_FORCE_LOW_SPEED,ps) || ps->low_speed){
		x = 2;
		if(ctmp == current){
		    return_line = dline + HEADER_ROWS(ps);
		    PutLine0(dline + HEADER_ROWS(ps), 0, "->");
		}
		else
		  PutLine0(dline + HEADER_ROWS(ps), 0, "  ");

		/*
		 * Only paint lines if we have to...
		 */
		if(screen->current)
		  continue;
	    }
	    else if(ctmp == current){
		return_line = dline + HEADER_ROWS(ps);
		StartInverse();
	    }

	    /*
	     * Copy the value to a temp buffer expanding tabs, and
	     * making sure not to write beyond screen right...
	     */
	    for(i=0,j=x; ctmp->dstring[i] && j < ps->ttyo->screen_cols; i++){
		if(ctmp->dstring[i] == ctrl('I')){
		    do
		      *p++ = ' ';
		    while(j < ps_global->ttyo->screen_cols && ((++j)&0x07));
			  
		}
		else{
		    *p++ = ctmp->dstring[i];
		    j++;
		}
	    }

	    *p = '\0';
	    PutLine0(dline + HEADER_ROWS(ps), x, tmp_20k_buf + x);

	    if(ctmp == current
	       && !(F_ON(F_FORCE_LOW_SPEED,ps) || ps->low_speed))
	      EndInverse();
	}
	else
	  ClearLine(dline + HEADER_ROWS(ps));
    }

    ps->mangled_body = 0;
    screen->top_line = top_line;
    screen->current  = current;
    return(return_line);
}


/*----------------------------------------------------------------------
  Redraw the attachment screen based on the global "att_screen" struct

  Args: none

  Result: 
  ----*/
void
attachment_screen_redrawer()
{
    bitmap_t	 bitmap;

    set_titlebar("ATTACHMENT INDEX", ps_global->mail_stream,
		 ps_global->context_current, ps_global->cur_folder,
		 ps_global->msgmap, 1, FolderName,0,0);

    ClearLine(1);

    ps_global->mangled_body = 1;
    (void)attachment_screen_updater(ps_global,att_screen->current,att_screen);

    setbitmap(bitmap);
    draw_keymenu(&att_index_keymenu, bitmap, ps_global->ttyo->screen_cols,
		 1-FOOTER_ROWS(ps_global), 0, FirstMenu,0);
}



/*----------------------------------------------------------------------
  Seek back from the given display line to the beginning of the list

  Args: p -- display linked list

  Result: 
  ----*/
ATDISP_S *
first_attline(p)
    ATDISP_S *p;
{
    while(p && p->prev)
      p = p->prev;

    return(p);
}


int
init_att_progress(msg, stream, body)
    char       *msg;
    MAILSTREAM *stream;
    BODY	*body;
{
    if(save_att_length = body->size.bytes){
	/* if there are display filters, factor in extra copy */
	if(body->type == TYPETEXT && ps_global->VAR_DISPLAY_FILTERS)
	  save_att_length += body->size.bytes;

	/* if remote folder and segment not cached, factor in IMAP fetch */
	if(stream && stream->mailbox && IS_REMOTE(stream->mailbox)
	   && !((body->type == TYPETEXT && body->contents.text)
		|| (body->type == TYPEMESSAGE && body->contents.msg.text)
		|| body->contents.binary))
	  save_att_length += body->size.bytes;

	gf_filter_init();			/* reset counters */
	pine_gets_bytes(1);
	save_att_piped(1);
	return(busy_alarm(1, msg, save_att_percent, 1));
    }

    return(0);
}


long
save_att_piped(reset)
    int reset;
{
    static long x;
    long	y;

    if(reset){
	x = y = 0L;
    }
    else if((y = gf_bytes_piped()) >= x){
	x = y;
	y = 0;
    }

    return(x + y);
}


int
save_att_percent()
{
    int i = (int) (((pine_gets_bytes(0) + save_att_piped(0)) * 100)
							   / save_att_length);
    return(min(i, 100));
}



/*----------------------------------------------------------------------
  Save the given attachment associated with the given message no

  Args: ps

  Result: 
  ----*/
void
save_attachment(qline, msgno, a)
     int       qline;
     long      msgno;
     ATTACH_S *a;
{
    char	filename[MAXPATH+1], full_filename[MAXPATH+1], *ill;
    HelpType	help;
    char       *l_string, prompt_buf[200];
    int         r, is_text, over = 0, we_cancel = 0;
    long        len;
    PARAMETER  *param;
    gf_io_t     pc;
    STORE_S    *store;
    char       *err;
    struct variable *vars = ps_global->vars;
    static ESCKEY_S att_save_opts[] = {
	{ctrl('T'), 10, "^T", "To Files"},
	{-1, 0, NULL, NULL},
	{-1, 0, NULL, NULL},
	{-1, 0, NULL, NULL}};

    is_text = a->body->type == TYPETEXT;

    /*-------  Figure out suggested file name ----*/
    filename[0] = full_filename[0] = '\0';
    for(param = a->body->parameter; param; param = param->next)
      if(param->value && strucmp(param->attribute, "name") == 0){
	  strcpy(filename, param->value);
	  break;
      }

    dprint(9, (debugfile, "save_attachment(name: %s)\n", filename));

    /*---------- Prompt the user for the file name -------------*/
    r = 0;
#if	!defined(DOS) && !defined(MAC) && !defined(OS2)
    if(ps_global->VAR_DOWNLOAD_CMD && ps_global->VAR_DOWNLOAD_CMD[0]){
	att_save_opts[++r].ch  = ctrl('V');
	att_save_opts[r].rval  = 12;
	att_save_opts[r].name  = "^V";
	att_save_opts[r].label = "Downld Msg";
    }
#endif	/* !(DOS || MAC) */

    if(F_ON(F_ENABLE_TAB_COMPLETE,ps_global)){
	att_save_opts[++r].ch  =  ctrl('I');
	att_save_opts[r].rval  = 11;
	att_save_opts[r].name  = "TAB";
	att_save_opts[r].label = "Complete";
    }

    att_save_opts[++r].ch = -1;

    help = NO_HELP;
    while(1) {
	sprintf(prompt_buf, "Copy attachment to file in %s directory: ",
		F_ON(F_USE_CURRENT_DIR, ps_global) ? "current"
		: VAR_OPER_DIR ? VAR_OPER_DIR : "home");
	r = optionally_enter(filename, qline, 0, MAXPATH, 1, 0, prompt_buf,
			     att_save_opts, help, 0);

        /*--- Help ----*/
        if(r == 3) {
            help = (help == NO_HELP) ? h_oe_export : NO_HELP;
            continue;
        }

	if(r == 10){				/* File Browser */
	    if(filename[0])
	      strcpy(full_filename, filename);
	    else if(F_ON(F_USE_CURRENT_DIR, ps_global))
	      (void) getcwd(full_filename, MAXPATH);
	    else if(VAR_OPER_DIR)
	      build_path(full_filename, VAR_OPER_DIR, filename);
	    else
              build_path(full_filename, ps_global->home_dir, filename);

	    r = file_lister("SAVE ATTACHMENT", full_filename, filename, TRUE,
			    FB_SAVE);

	    if(r == 1){
		build_path (tmp_20k_buf, full_filename, filename);
		strcpy (full_filename, tmp_20k_buf);
	    }
	    else
	      continue;
	}
	else if(r == 11){			/* File Name Completion */
	    char dir[MAXPATH], *fn;
	    int  l = MAXPATH;

	    dir[0] = '\0';
	    if(*filename && (fn = last_cmpnt(filename))){
		l -= fn - filename;
		if(is_absolute_path(filename)){
		    strncpy(dir, filename, fn - filename);
		    dir[fn - filename] = '\0';
		}
		else{
		    char *p = NULL;
		    sprintf(full_filename, "%.*s", fn - filename, filename);
		    build_path(dir, F_ON(F_USE_CURRENT_DIR, ps_global)
				       ? p = (char *) getcwd(NULL, MAXPATH)
				       : VAR_OPER_DIR ? VAR_OPER_DIR
						      : ps_global->home_dir,
			       full_filename);
		    if(p)
		      free(p);
		}
	    }
	    else{
		fn = filename;
		if(F_ON(F_USE_CURRENT_DIR, ps_global))
		  (void) getcwd(dir, MAXPATH);
		else if(VAR_OPER_DIR)
		  strcpy(dir, VAR_OPER_DIR);
		else
		  strcpy(dir, ps_global->home_dir);
	    }

	    if(!pico_fncomplete(dir, fn, l - 1))
	      Writechar(BELL, 0);

	    continue;
	}
#if	!defined(DOS) && !defined(MAC) && !defined(OS2)
	else if(r == 12){			/* Download */
	    char     cmd[MAXPATH], *fp, *tfp;
	    int	     next = 0;
	    PIPE_S  *syspipe;
	    gf_io_t  pc;

	    if(ps_global->restricted){
		q_status_message(SM_ORDER | SM_DING, 3, 3,
				 "Download disallowed in restricted mode");
		return;
	    }

	    err = NULL;
	    tfp = temp_nam(NULL, "pd");
	    dprint(1, (debugfile, "Download attachment called!\n"));
	    if(store = so_get(FileStar, tfp, WRITE_ACCESS)){
		sprintf(prompt_buf, "Saving to \"%.50s\"", full_filename);
		we_cancel = init_att_progress(prompt_buf,
					      ps_global->mail_stream,
					      a->body);

		gf_set_so_writec(&pc, store);
		if(err = detach(ps_global->mail_stream, msgno, a->body,
				a->number, &len, pc, NULL))
		  q_status_message2(SM_ORDER | SM_DING, 3, 5,
				    "%s: Error writing attachment to \"%s\"",
				    err, full_filename);

		/* cancel regardless, so it doesn't get in way of xfer */
		cancel_busy_alarm(0);

		so_give(&store);		/* close file */

		if(!err){
		    build_updown_cmd(cmd, ps_global->VAR_DOWNLOAD_CMD_PREFIX,
				     ps_global->VAR_DOWNLOAD_CMD, tfp);
		    if(syspipe = open_system_pipe(cmd, NULL, NULL,
						  PIPE_USER | PIPE_RESET))
		      (void) close_system_pipe(&syspipe);
		    else
		      q_status_message(SM_ORDER | SM_DING, 3, 3,
				       err = "Error running download command");
		}

		unlink(tfp);
	    }
	    else
	      q_status_message(SM_ORDER | SM_DING, 3, 3,
			       err = "Error building temp file for download");

	    fs_give((void **)&tfp);
	    if(!err)
	      q_status_message1(SM_ORDER, 0, 4, "Part %s downloaded",
				a->number);
				

	    return;
	}
#endif	/* !(DOS || MAC) */
        else if(r == 1 || filename[0] == '\0') {
            q_status_message(SM_ORDER, 0, 2, "Save attachment cancelled");
            return;
        }
        else if(r == 4)
          continue;


        /* check out and expand file name. give possible error messages */
	if(!full_filename[0])
	  strcpy(full_filename, filename);

        removing_trailing_white_space(full_filename);
        removing_leading_white_space(full_filename);
        if((ill = filter_filename(filename)) != NULL) {
/* BUG: we should beep when the key's pressed rather than bitch later */
            q_status_message1(SM_ORDER | SM_DING, 3, 3, "%s", ill);
            continue;
        }

#if	defined(DOS) || defined(OS2)
	if(is_absolute_path(full_filename)){
	    fixpath(full_filename, MAXPATH);
	}
#else
	if(full_filename[0] == '~') {
	    if(fnexpand(full_filename, sizeof(full_filename)) == NULL) {
		char *p = strindex(full_filename, '/');
		if(p != NULL)
		  *p = '\0';
		q_status_message1(SM_ORDER | SM_DING, 3, 3,
			      "Error expanding file name: \"%s\" unknown user",
			      full_filename);
		continue;
	    }
	}
#endif

	if(!is_absolute_path(full_filename)){
	    if(F_ON(F_USE_CURRENT_DIR, ps_global))
	      (void)strcpy(full_filename, filename);
	    else if(VAR_OPER_DIR)
	      build_path(full_filename, VAR_OPER_DIR, filename);
	    else
	      build_path(full_filename, ps_global->home_dir, filename);
	}

	break;		/* Must have got an OK file name */
    }

    if(ps_global->restricted) {
        q_status_message(SM_ORDER | SM_DING, 0, 4,
			 "Pine demo can't save attachments");
        return;
    }

    if(VAR_OPER_DIR && !in_dir(VAR_OPER_DIR, full_filename)){
	q_status_message1(SM_ORDER, 0, 2, "Can't save to file outside of %s",
			  VAR_OPER_DIR);
	return;
    }
     

    /*----------- Write the contents to the file -------------*/
    if(can_access(full_filename, ACCESS_EXISTS) == 0) {
	static ESCKEY_S access_opts[] = {
	    {'o', 'o', "O", "Overwrite"},
	    {'a', 'a', "A", "Append"},
	    {-1, 0, NULL, NULL}};

        sprintf(prompt_buf,
		"File \"%s%s\" already exists.  Overwrite or append it ? ",
		((r = strlen(full_filename)) > 20) ? "..." : "",
                full_filename + ((r > 20) ? r - 20 : 0));

	switch(radio_buttons(prompt_buf, -FOOTER_ROWS(ps_global),
			     access_opts, 'a', 'x', NO_HELP, RB_NORM)){
	  case 'o' :
	    over = 1;
	    if(unlink(full_filename) < 0){	/* BUG: breaks links */
		q_status_message2(SM_ORDER | SM_DING, 3, 5,
				  "Error deleting old %s: %s",
				  full_filename, error_description(errno));
		return;
	    }

	    break;

	  case 'a' :
	    over = -1;
	    break;

	  case 'x' :
	  default :
            q_status_message(SM_ORDER, 0, 2, "Save of attachment cancelled");
            return;
	}
    }

    if((store = so_get(FileStar, full_filename, WRITE_ACCESS)) == NULL){
	q_status_message2(SM_ORDER | SM_DING, 3, 5,
			  "Error opening destination %s: %s",
			  full_filename, error_description(errno));
	return;
    }

    sprintf(prompt_buf, "Saving to \"%.50s\"", full_filename);
    we_cancel = init_att_progress(prompt_buf, ps_global->mail_stream, a->body);

    gf_set_so_writec(&pc, store);
    err = detach(ps_global->mail_stream,msgno,a->body,a->number,&len,pc,NULL);

    if(we_cancel)
      cancel_busy_alarm(0);

    if(!err){
        l_string = cpystr(byte_string(len));
        q_status_message8(SM_ORDER, 0, 4,
			  "Part %s, %s%s %s to \"%s\"%s%s%s",
			  a->number, 
			  is_text
			    ? comatose(a->body->size.lines) : l_string,
			  is_text ? " lines" : "",
			  over==0 ? "written"
				  : over==1 ? "overwritten" : "appended",
			  full_filename,
			  (is_text || len == a->body->size.bytes)
			    ? "" : "(decoded from ",
                          (is_text || len == a->body->size.bytes)
			    ? "" : byte_string(a->body->size.bytes),
			  is_text || len == a->body->size.bytes
			    ? "" : ")");
        fs_give((void **)&l_string);
    }
    else
      q_status_message2(SM_ORDER | SM_DING, 3, 5,
			"%s: Error writing attachment to \"%s\"",
                        err, full_filename);
    so_give(&store);
}


/*----------------------------------------------------------------------
  Unpack and display the given attachment associated with given message no.

  Args: msgno -- message no attachment is part of
	a -- attachment to display

  Returns: 0 on success, non-zero (and error message queued) otherwise
  ----*/        
int
display_attachment(msgno, a)
     long      msgno;
     ATTACH_S *a;
{
    char    *filename;
    STORE_S *store;
    gf_io_t  pc;
    char    *err;
    int      we_cancel = 0;
#if !defined(DOS) && !defined(OS2)
    char     prefix[8];
#endif

    if(a->can_display == CD_DEFERRED){
	int use_viewer;
	a->can_display = mime_can_display(a->body->type, a->body->subtype,
					  a->body->parameter, &use_viewer)
			  ? CD_GOFORIT : CD_NOCANDO;
	a->use_external_viewer = use_viewer;
    }

    /*------- Display the attachment -------*/
    if(a->can_display == CD_NOCANDO) {
        /*----- Can't display this type ------*/
	if(a->body->encoding < ENCOTHER)
	  q_status_message3(SM_ORDER | SM_DING, 3, 5,
		     "Don't know how to display %s%s%s attachments. Try Save.",
			    body_type_names(a->body->type),
			    a->body->subtype ? "/" : "",
			    a->body->subtype ? a->body->subtype :"");
	else
	  q_status_message1(SM_ORDER | SM_DING, 3, 5,
			    "Don't know how to unpack \"%s\" encoding",
			    body_encodings[(a->body->encoding <= ENCMAX)
					     ? a->body->encoding : ENCOTHER]);

        return(1);
    }
    else if(!a->use_external_viewer){
	if(a->body->type == TYPETEXT)
	  display_text_att(msgno, a);
	else if(a->body->type == TYPEMESSAGE)
	  display_msg_att(msgno, a);
	else if(a->body->type == TYPEAPPLICATION
		&& a->body->subtype
		&& !strucmp(a->body->subtype, "DIRECTORY"))
	  display_abook_att(msgno, a);

	ps_global->mangled_screen = 1;
	return(0);
    }

    /*------ Write the image to a temporary file ------*/
#if defined(DOS) || defined(OS2)
    /*
     *  Dos applications are particular about the file name extensions
     *  Try to look up the expected extension from the mime.types file.
     */
    {  char ext[32];
       char mtype[128];
       
     ext[0] = '\0';
     strcpy (mtype, body_type_names(a->body->type));
     if (a->body->subtype) {
	 strcat (mtype, "/");
	 strcat (mtype, a->body->subtype);
     }

     set_mime_extension_by_type (ext, mtype);
     filename = temp_nam_ext(NULL, "im", ext);
   }
#else
    sprintf(prefix, "img-%.3s", (a->body->subtype) ? a->body->subtype : "unk");
    filename = temp_nam(NULL, prefix);
#endif

    if((store = so_get(FileStar, filename, WRITE_ACCESS)) == NULL){
        q_status_message2(SM_ORDER | SM_DING, 3, 5,
                          "Error \"%s\", Can't write file %s",
                          error_description(errno), filename);
        return(1);
    }


    if(a->body->size.bytes){
	char msg_buf[128];

	sprintf(msg_buf, "Decoding %s%.50s%s%s",
		a->description ? "\"" : "",
		a->description ? a->description : "attachment number ",
		a->description ? "" : a->number,
		a->description ? "\"" : "");
	we_cancel = init_att_progress(msg_buf, ps_global->mail_stream,
				      a->body);
    }

    gf_set_so_writec(&pc, store);
    err = detach(ps_global->mail_stream,msgno,a->body,a->number,NULL,pc,NULL);

    if(we_cancel)
      cancel_busy_alarm(0);

    if(err){
	q_status_message2(SM_ORDER | SM_DING, 3, 5,
		     "%s: Error saving image to temp file %s", err, filename);
	return(1);
    }

    so_give(&store);

    /*----- Run the viewer process ----*/
    run_viewer(filename, a->body);
    fs_give((void **)&filename);
    return(0);
}


/*----------------------------------------------------------------------
   Fish the required command from mailcap and run it

  Args: image_file -- The name of the file to pass viewer
	body -- body struct containing type/subtype of part

A side effect may be that scrolltool is called as well if
exec_mailcap_cmd has any substantial output...
 ----*/
void
run_viewer(image_file, body)
     char *image_file;
     BODY *body;
{
    char *cmd            = NULL;
    int   needs_terminal = 0, we_cancel = 0;

    we_cancel = busy_alarm(1, "Displaying attachment", NULL, 1);

    if(cmd = mailcap_build_command(body, image_file, &needs_terminal)){
	if(we_cancel)
	  cancel_busy_alarm(-1);

	exec_mailcap_cmd(cmd, image_file, needs_terminal);
	fs_give((void **)&cmd);
    }
    else{
	if(we_cancel)
	  cancel_busy_alarm(-1);

	q_status_message1(SM_ORDER, 3, 4, "Cannot display %s attachment",
			  type_desc(body->type, body->subtype,
				    body->parameter, 1));
    }
}



/*----------------------------------------------------------------------
  Detach and provide for browsing a text body part

  Args: msgno -- raw message number to get part from
	 a -- attachment struct for the desired part

  Result: 
 ----*/
void
display_text_att(msgno, a)
    long      msgno;
    ATTACH_S *a;
{
    STORE_S        *store;
    gf_io_t         pc;
    SourceType	    src = CharStar;

    clear_index_cache_ent(msgno);

    /* BUG, should check this return code */
    (void)mail_fetchstructure(ps_global->mail_stream, msgno, NULL);

    /* initialize a storage object */
#if	defined(DOS) && !defined(WIN32)
    if(a->body->size.bytes > MAX_MSG_INCORE
       || strcmp(ps_global->mail_stream->dtb->name, "nntp") == 0)
      src = FileStar;
#endif

    if(store = so_get(src, NULL, EDIT_ACCESS)){
	gf_set_so_writec(&pc, store);
	(void) decode_text(a, msgno, pc, QStatus, 1);
	scrolltool(so_text(store), "ATTACHED TEXT", AttachText, src, a);
	so_give(&store);	/* free resources associated with store */
    }
    else
      q_status_message(SM_ORDER | SM_DING, 3, 3,
		       "Error allocating space for attachment.");
}


/*----------------------------------------------------------------------
  Detach and provide for browsing a body part of type "MESSAGE"

  Args: msgno -- message number to get partrom
	 a -- attachment struct for the desired part

  Result: 
 ----*/
void
display_msg_att(msgno, a)
    long      msgno;
    ATTACH_S *a;
{
    STORE_S        *store;
    gf_io_t         pc;
    SourceType	    src = CharStar;

    clear_index_cache_ent(msgno);

    /* BUG, should check this return code */
    (void)mail_fetchstructure(ps_global->mail_stream, msgno, NULL);

    /* initialize a storage object */
#if	defined(DOS) && !defined(WIN32)
    if(a->body->size.bytes > MAX_MSG_INCORE
       || strcmp(ps_global->mail_stream->dtb->name, "nntp") == 0)
      src = FileStar;
#endif

    if(!(store = so_get(src, NULL, EDIT_ACCESS))){
	q_status_message(SM_ORDER | SM_DING, 3, 3,
			 "Error allocating space for message.");
	return;
    }

    gf_set_so_writec(&pc, store);

    if(a->body->subtype && strucmp(a->body->subtype, "rfc822") == 0){
	format_envelope(NULL, 0L, a->body->contents.msg.env, pc,
			FE_DEFAULT, NULL);
	gf_puts(NEWLINE, pc);
	if((a+1)->description && (a+1)->body && (a+1)->body->type == TYPETEXT){
	    (void) decode_text(a+1, msgno, pc, QStatus, 1);
	}
	else{
	    gf_puts("[Can't display ", pc);
	    if((a+1)->description && (a+1)->body)
	      gf_puts("first non-", pc);
	    else
	      gf_puts("missing ", pc);

	    gf_puts("text segment]", pc);
	}
    }
    else if(a->body->subtype 
	    && strucmp(a->body->subtype, "external-body") == 0) {
	gf_puts("This part is not included and can be fetched as follows:",pc);
	gf_puts(NEWLINE, pc);
	gf_puts(NEWLINE, pc);
	gf_puts(display_parameters(a->body->parameter), pc);
    }
    else
      (void) decode_text(a, msgno, pc, QStatus, 1);

    scrolltool(so_text(store), "ATTACHED MESSAGE", AttachText, src, a);

    so_give(&store);	/* free resources associated with store */
}


void
display_abook_att(msgno, a)
    long      msgno;
    ATTACH_S *a;
{
    STORE_S   *store;
    char     **lines, **ll, *defaulttype = NULL;
    PARAMETER *parm;

    lines = detach_abook_att(ps_global->mail_stream, msgno, a->body, a->number);
    if(!lines){
	q_status_message(SM_ORDER | SM_DING, 3, 3,
			 "Error accessing attachment."); 
	return;
    }

    if(!(store = so_get(CharStar, NULL, EDIT_ACCESS))){
	if(lines)
	  free_list(&lines);

	q_status_message(SM_ORDER | SM_DING, 3, 3,
			 "Error allocating space for attachment."); 
	return;
    }

    for(parm = a->body->parameter; parm; parm = parm->next)
      if(!strucmp("DEFAULTTYPE", parm->attribute))
	break;
    
    if(parm)
      defaulttype = parm->value;

    for(ll = lines; ll && *ll && **ll; ll++){
	char *p;

	if(ll == lines)
	  so_puts(store, "  ");
	else
	  so_puts(store, "\n  ");

	p = (char *)rfc1522_decode((unsigned char *)tmp_20k_buf, *ll, NULL);
	if(p && *p == ':' && defaulttype){
	    unsigned char buf[1000];

	    so_puts(store, (char *)rfc1522_decode(buf, defaulttype, NULL));
	}

	so_puts(store, p);
    }

    so_puts(store, "\n\n(This is an address book entry which has been forwarded to you.\nYou may add it to your address book by exiting this screen and the\nattachment index screen, then using the \"TakeAddr\" command.  You will have\na chance to edit it before committing it to your address book.)\n");

    scrolltool(so_text(store), "ADDRESS BOOK ATTACHMENT", ViewAbookAtt,
	       CharStar, NULL);
    so_give(&store);

    if(lines)
      free_list(&lines);
}



/*----------------------------------------------------------------------
  Display attachment information

  Args: msgno -- message number to get partrom
	 a -- attachment struct for the desired part

  Result: a screen containing information about attachment:
	  Type		:
	  Subtype	:
	  Parameters	:
	    Comment	:
            FileName	:
	  Encoded size	:
	  Viewer	:
 ----*/
void
display_attach_info(msgno, a)
    long      msgno;
    ATTACH_S *a;
{
    int	       i;
    STORE_S   *store;
    PARAMETER *parms;

    if(a->can_display == CD_DEFERRED){
	int use_viewer;
	a->can_display = mime_can_display(a->body->type, a->body->subtype,
					  a->body->parameter, &use_viewer)
			  ? CD_GOFORIT : CD_NOCANDO;
	a->use_external_viewer = use_viewer;
    }

    if(!(store = so_get(CharStar, NULL, EDIT_ACCESS))){
	q_status_message(SM_ORDER | SM_DING, 3, 3,
			 "Error allocating space for message.");
	return;
    }

    so_puts(store, "\nDetails about Attachment #");
    so_puts(store, a->number);
    so_puts(store, " :\n\n\n");
    so_puts(store, "\t\tType\t\t: ");
    so_puts(store, body_type_names(a->body->type));
    so_puts(store, "\n");
    so_puts(store, "\t\tSubtype\t\t: ");
    so_puts(store, a->body->subtype ? a->body->subtype : "Unknown");
    so_puts(store, "\n");
    so_puts(store, "\t\tEncoding\t: ");
    so_puts(store, a->body->encoding < ENCMAX
			 ? body_encodings[a->body->encoding]
			 : "Unknown");
    so_puts(store, "\n");
    if(a->body->parameter){
	so_puts(store, "\t\tParameters\t: ");
	for(i=0, parms=a->body->parameter; parms; parms = parms->next, i++){
	    so_puts(store, parms->attribute);
	    so_puts(store, " = ");
	    so_puts(store, parms->value);
	    so_puts(store, "\n");
	    if(parms->next)
	      so_puts(store, "\t\t\t\t  ");
	}
    }

    if(a->body->description){
	so_puts(store, "\t\tDescription\t: \"");
	so_puts(store, a->body->description);
	so_puts(store, "\"\n");
	/* BUG: Wrap description text if necessary */
    }

    so_puts(store, "\t\tApprox. Size\t: ");
    so_puts(store, comatose((a->body->encoding == ENCBASE64)
			      ? ((a->body->size.bytes * 3)/4)
			      : a->body->size.bytes));
    so_puts(store, " bytes\n");
    so_puts(store, "\t\tDisplay Method\t: ");
    if(a->can_display == CD_NOCANDO) {
	so_puts(store, "Can't, ");
	so_puts(store, (a->body->encoding < ENCOTHER)
			 ? "Unknown Attachment Format"
			 : "Unknown Encoding");
    }
    else if(!a->use_external_viewer){
	so_puts(store, "Pine's Internal Pager");
    }
    else{
	int   nt;
	char *cmd;

	if(cmd = mailcap_build_command(a->body, "<datafile>", &nt)){
	    so_puts(store, "\"");
	    so_puts(store, cmd);
	    so_puts(store, "\"");
	    fs_give((void **)&cmd);
	}
    }

    so_puts(store, "\n");

    scrolltool(so_text(store), "ABOUT ATTACHMENT", AttachText, CharStar, NULL);
    so_give(&store);	/* free resources associated with store */
    ps_global->mangled_screen = 1;
}



/*----------------------------------------------------------------------

  ----*/        
void
pipe_attachment(msgno, a)
     long      msgno;
     ATTACH_S *a;
{
    char    *err, *resultfilename = NULL, prompt[80];
    int      rc, flags, capture = 1, raw = 0, we_cancel = 0;
    PIPE_S  *syspipe;
    HelpType help;
    char     pipe_command[MAXPATH+1];
    static ESCKEY_S pipe_opt[] = {
	{0, 0, "", ""},
	{ctrl('W'), 10, "^W", NULL},
	{ctrl('Y'), 11, "^Y", NULL},
	{-1, 0, NULL, NULL}
    };
    
    if(ps_global->restricted){
	q_status_message(SM_ORDER | SM_DING, 0, 4,
			 "Pine demo can't pipe attachments");
	return;
    }

    help = NO_HELP;
    pipe_command[0] = '\0';
    while(1){
	sprintf(prompt, "Pipe %sattachment %s to %s: ", raw ? "RAW " : "",
		a->number, capture ? "" : "(Free Output) ");
	pipe_opt[1].label = raw ? "DecodedData" : "Raw Data";
	pipe_opt[2].label = capture ? "Free Output" : "Capture Output";
	rc = optionally_enter(pipe_command, -FOOTER_ROWS(ps_global), 0,
			      MAXPATH, 1, 0, prompt, pipe_opt, help, 0);
	if(rc == -1){
	    q_status_message(SM_ORDER | SM_DING, 3, 4,
			     "Internal problem encountered");
	    break;
	}
	else if(rc == 10){
	    raw = !raw;			/* flip raw text */
	}
	else if(rc == 11){
	    capture = !capture;		/* flip capture output */
	}
	else if(rc == 0){
	    if(pipe_command[0] == '\0'){
		q_status_message(SM_ORDER, 0, 2, "Pipe command cancelled");
		break;
	    }

	    flags = PIPE_USER | PIPE_WRITE | PIPE_STDERR;
	    if(!capture){
#ifndef	_WINDOWS
		ClearScreen();
		fflush(stdout);
		clear_cursor_pos();
		ps_global->mangled_screen = 1;
#endif
		flags |= PIPE_RESET;
	    }

	    if(syspipe = open_system_pipe(pipe_command,
				   (flags&PIPE_RESET) ? NULL : &resultfilename,
				   NULL, flags)){
		gf_io_t  pc;		/* wire up a generic putchar */
		gf_set_writec(&pc, syspipe->out.f, 0L, FileStar);

		/*------ Write the image to a temporary file ------*/
		if(raw){		/* pipe raw text */
		    char      *contents;
		    unsigned long len;
		    gf_io_t    gc;
		    SourceType src = CharStar;
#if	defined(DOS) && !defined(WIN32)
		    char *tmpfile_name = NULL;
#endif

		    err = NULL;
#if	defined(DOS) && !defined(WIN32)
		    if(a->body->size.bytes > MAX_MSG_INCORE
		       || !strcmp(ps_global->mail_stream->dtb->name,"nntp")){
			src = FileStar;
			if(!(tmpfile_name = temp_nam(NULL, "dt"))
			   || !(append_file = fopen(tmpfile_name, "w+b"))){
			    if(tmpfile_name)
			      fs_give((void **)&tmpfile_name);

			    err = "Can't create temp file for pipe";
			}
			else
			  mail_parameters(ps_global->mail_stream, SET_GETS,
					  (void *)dos_gets);
		    }
		    else
		      mail_parameters(ps_global->mail_stream, SET_GETS,
				      (void *)NULL);
#endif	/* DOS */

		    if(capture)
		      we_cancel = busy_alarm(1, NULL, NULL, 0);
		    else
		      suspend_busy_alarm();

		    if(!err
		       && !(contents = mail_fetchbody(ps_global->mail_stream,
						      msgno, a->number, &len)))
		      err = "Can't access body part";

		    if(!err){
			gf_set_readc(&gc,
#if	defined(DOS) && !defined(WIN32)
				     (src == FileStar) ? (void *)append_file :
#endif
				     contents, len, src);
			gf_filter_init();
			gf_link_filter(gf_nvtnl_local);
			err = gf_pipe(gc, pc);
		    }

#if	defined(DOS) && !defined(WIN32)
		    /*
		     * free up file pointer, and delete tmpfile
		     */
		    if(src == FileStar){
			fclose(append_file);
			append_file = NULL;
			unlink(tmpfile_name);
			fs_give((void **)&tmpfile_name);
			mail_parameters(ps_global->mail_stream, SET_GETS,
					(void *)NULL);
			mail_gc(ps_global->mail_stream, GC_TEXTS);
		    }
#endif
		    if(capture){
			if(we_cancel)
			  cancel_busy_alarm(0);
		    }
		    else
		      resume_busy_alarm();
		}
		else{
		    /* BUG: there's got to be a better way */
		    if(!capture)
		      ps_global->print = (PRINT_S *) 1;

		    err = detach(ps_global->mail_stream, msgno, a->body,
				 a->number, (long *)NULL, pc, NULL);
		    ps_global->print = (PRINT_S *) NULL;
		}

		if(err)
		  q_status_message1(SM_ORDER | SM_DING, 3, 4,
				    "Error detaching for pipe: %s", err);

		(void) close_system_pipe(&syspipe);
		if(err)
		  unlink(resultfilename);
		else
		  display_output_file(resultfilename,"PIPE ATTACHMENT", NULL);

		fs_give((void **)&resultfilename);
	    }
	    else
	      q_status_message(SM_ORDER | SM_DING, 3, 4,
			       "Error opening pipe");

	    break;
	}
	else if(rc == 1){
	    q_status_message(SM_ORDER, 0, 2,"Pipe cancelled");
	    break;
	}
	else if(rc = 3)
	  help = (help == NO_HELP) ? h_pipe_attach : NO_HELP;
    }
}


/*
 * We need to defined simple functions here for the piping and 
 * temporary storage object below.  We can use the filter.c functions
 * because they're already in use for the "putchar" function passed to
 * detach.
 */
static STORE_S *detach_so = NULL;

/*
 * The display filter is locally global because it's set in df_trigger_cmp
 * which sniffs at lines of the unencoded segment...
 */
static char    *display_filter;
static struct   triggers {
    int		   (*cmp) PROTO((char *, char *));
    char	    *text;
    char	    *cmd;
    struct triggers *next;
} *df_trigger_list;
struct triggers *build_trigger_list PROTO(());
void		 blast_trigger_list PROTO((struct triggers **));



int
detach_writec(c)
    int c;
{
    return(so_writec(c, detach_so));
}


/*----------------------------------------------------------------------
  detach the given body part using the given encoding

  Args: a bunch

  Returns: NULL on success, error message otherwise
  ----*/
char *
detach(stream, msg_no, body, part_no, len, pc, aux_filters)
     MAILSTREAM *stream;		/* c-client stream to use         */
     long        msg_no;		/* message number to deal with	  */
     BODY       *body;			/* body pointer 		  */
     char       *part_no;		/* part number of message 	  */
     long       *len;			/* returns bytes read in this arg */
     gf_io_t	 pc;			/* where to put it		  */
     filter_t   *aux_filters;		/* null terminated array of filts */
{
    unsigned long  length, rv;
    int            we_cancel = 0;
    char          *status, *contents, *fcontents = NULL;
    gf_io_t        gc;
    SourceType     src = CharStar;
    static char    err_string[100];
#ifdef	DOS
#ifndef	WIN32
    char	  *tmpfile_name = NULL;
#endif
    extern unsigned char  *xlate_to_codepage;
#endif

    err_string[0] = '\0';

#if	defined(DOS) && !defined(WIN32)
    if(body->size.bytes > MAX_MSG_INCORE || !strcmp(stream->dtb->name,"nntp")){
	src = FileStar;
	if(!(tmpfile_name = temp_nam(NULL, "dt"))
	   || !(append_file = fopen(tmpfile_name, "w+b"))){
	    if(tmpfile_name)
	      fs_give((void **)&tmpfile_name);

	    sprintf(err_string, "Can't detach!  Temp file %s",
		    error_description(errno));
	    return(err_string);
	}

	mail_parameters(stream, SET_GETS, (void *)dos_gets);
    }
    else
      mail_parameters(stream, SET_GETS, (void *)NULL);
#endif	/* DOS */

    if(!ps_global->print)
      we_cancel = busy_alarm(1, NULL, NULL, 0);

    gf_filter_init();			/* prepare for filtering! */

    /*
     * go grab the requested body part
     */
    contents = mail_fetchbody(stream, msg_no, part_no, &length);
    if(contents == NULL) {
	sprintf(err_string, "Unable to access body part %s", part_no);
	rv = 0L;
	goto fini;
    }

    rv = (length) ? length : 1L;

    switch(body->encoding) {		/* handle decoding */
      case ENC7BIT:
      case ENC8BIT:
      case ENCBINARY:
        break;

      case ENCBASE64:
	gf_link_filter(gf_b64_binary);
        break;

      case ENCQUOTEDPRINTABLE:
	gf_link_filter(gf_qp_8bit);
        break;

      case ENCOTHER:
      default:
	dprint(1, (debugfile, "detach: unknown CTE: \"%s\" (%d)\n",
		   (body->encoding <= ENCMAX) ? body_encodings[body->encoding]
					      : "BEYOND-KNOWN-TYPES",
		   body->encoding));
	break;
    }

    /*
     * If we're detaching a text segment and there are user-defined
     * filters and there are text triggers to look for, install filter
     * to let us look at each line...
     */
    display_filter = NULL;
    if(body->type == TYPETEXT && ps_global->VAR_DISPLAY_FILTERS){
	/* check for "static" triggers (i.e., none or CHARSET) */
	if(!df_static_trigger(body)
	   && (df_trigger_list = build_trigger_list())){
	    /* else look for matching text trigger */
	    gf_line_test_opt(df_trigger_cmp);
	    gf_link_filter(gf_line_test);
	}
    }
    else
      /* add aux filters if we're not going to MIME decode into a temporary
       * storage object, otherwise we pass the aux_filters on to gf_filter
       * below so it can pass what comes out of the external filter command
       * thru the rest of the filters...
       */
      while(aux_filters && *aux_filters)	/* apply provided filters */
	gf_link_filter(*aux_filters++);

    /*
     * Following canonical model, after decoding convert newlines from
     * crlf to local convention.
     */
    if(body->type == TYPETEXT || (body->type == TYPEMESSAGE
				  && !strucmp(body->subtype, "rfc822"))){
	gf_link_filter(gf_nvtnl_local);
#ifdef	DOS
	/*
	 * When detaching a text part AND it's US-ASCII OR it
	 * matches what the user's defined as our charset,
	 * translate it...
	 */
	if(mime_can_display(body->type, body->subtype, body->parameter,
								(int *)NULL)
	   && xlate_to_codepage){
	    gf_translate_opt(xlate_to_codepage, 256);
	    gf_link_filter(gf_translate);
	}
#endif
    }

#ifdef	LATER
    dprint(9, (debugfile, "Attachment lengths: %ld (decoded) %lud (encoded)\n",
               *decoded_len, length));
#endif

    gf_set_readc(&gc,
#if	defined(DOS) && !defined(WIN32)
		 /*
		  * If we fetched this to a file, make sure we set
		  * up the storage object's getchar function accordingly...
		  */
		 (src == FileStar) ? (void *)append_file :
#endif
		 contents, length, src);

    /*
     * If we're detaching a text segment and a user-defined filter may
     * need to be invoked later (see below), decode the segment into
     * a temporary storage object...
     */
    if(body->type == TYPETEXT && ps_global->VAR_DISPLAY_FILTERS
       && !(detach_so = so_get(src, NULL, EDIT_ACCESS)))
      strcpy(err_string,
	   "Formatting error: no space to make copy, no display filters used");

    if(status = gf_pipe(gc, detach_so ? detach_writec : pc)) {
	sprintf(err_string, "Formatting error: %s", status);
        rv = 0L;
    }

    /*
     * If we wrote to a temporary area, there MAY be a user-defined
     * filter to invoke.  Filter it it (or not if no trigger match)
     * *AND* send the output thru any auxiliary filters, destroy the
     * temporary object and be done with it...
     */
    if(detach_so){
	if(!err_string[0] && display_filter && *display_filter){
	    filter_t *p, *aux = NULL;

	    if(aux_filters && *aux_filters){
		/* insert NL conversion filters around remaining aux_filters
		 * so they're not tripped up by local NL convention
		 */
		for(p = aux_filters; *p; p++)	/* count aux_filters */
		  ;

		p = aux = (filter_t *)fs_get(((p - aux_filters) + 3)
					      * sizeof(filter_t));
		*p++ = gf_local_nvtnl;
		for(; *aux_filters; p++, aux_filters++)
		  *p = *aux_filters;

		*p++ = gf_nvtnl_local;
		*p   = NULL;
	    }

	    if(status = dfilter(display_filter, detach_so, pc, aux)){
		sprintf(err_string, "Formatting error: %s", status);
		rv = 0L;
	    }

	    if(aux)
	      fs_give((void **)&aux);
	}
	else{					/*  just copy it, then */
	    gf_set_so_readc(&gc, detach_so);
	    so_seek(detach_so, 0L, 0);
	    gf_filter_init();
	    if(aux_filters && *aux_filters){
		/* if other filters are involved, correct for
		 * newlines on either side of the pipe...
		 */
		gf_link_filter(gf_local_nvtnl);
		while(*aux_filters)
		  gf_link_filter(*aux_filters++);

		gf_link_filter(gf_nvtnl_local);
	    }

	    if(status = gf_pipe(gc, pc)){	/* Second pass, sheesh */
		sprintf(err_string, "Formatting error: %s", status);
		rv = 0L;
	    }
	}

	so_give(&detach_so);			/* blast temp copy */
    }

  fini :

#if	defined(DOS) && !defined(WIN32)
    /*
     * free up file pointer, and delete tmpfile opened for
     * dos_gets.  Again, we can't use DOS' tmpfile() as it writes root
     * sheesh.
     */
    if(src == FileStar){
	fclose(append_file);
	append_file = NULL;
	unlink(tmpfile_name);
	fs_give((void **)&tmpfile_name);
	mail_parameters(stream, SET_GETS, (void *)NULL);
	mail_gc(stream, GC_TEXTS);
    }
#endif

    if(!ps_global->print && we_cancel)
      cancel_busy_alarm(0);

    if (len)
      *len = rv;

    if(df_trigger_list)
      blast_trigger_list(&df_trigger_list);

    return((err_string[0] == '\0') ? NULL : err_string);
}


/*
 * df_static_trigger - look thru the display filter list and set the
 *		       filter to any triggers that don't require scanning
 *		       the message segment.
 */
int
df_static_trigger(body)
    BODY *body;
{
    char **l;
    int    success = 0;
    char  *test = NULL, *cmd = NULL;

    for(l = ps_global->VAR_DISPLAY_FILTERS ; l && *l && !success; l++){
	get_pair(*l, &test, &cmd, 1);

	if(test){
	    if(!*test){				/* null test means always! */
		success = 1;
	    }
	    else if(struncmp(test, "_CHARSET(", 9) == 0){
		PARAMETER *params = body->parameter;
		char *p = strrindex(test, ')');
		if(p){
		    *p = '\0';
		    while(params && strucmp(params->attribute, "charset"))
		      params = params->next;

		    success = !strucmp(test + 9,
				       params ? params->value : "us-ascii");
		}
		else
		  dprint(1, (debugfile,
			     "filter trigger: malformed test: %s\n", test));
	    }

	    fs_give((void **)&test);		/* clean up test */
	}
	/* else empty list value */

	/*
	 * make sure cmd exists!  NOTE: success overloaded
	 */
	if(!(success && df_valid_command(&cmd))){
	    success = 0;
	    if(cmd)
	      fs_give((void **)&cmd);
	    else
	      dprint(5, (debugfile, "filter trigger: EMPTY command!\n"));
	}
    }

    return((display_filter = cmd) != NULL);
}


/*
 * build_trigger_list - return possible triggers in a list of triggers 
 *			structs
 */
struct triggers *
build_trigger_list()
{
    struct triggers  *tp = NULL, **trailp;
    char	    **l, *test, *str, *ep, *cmd = NULL;
    int		      i;

    trailp = &tp;
    for(l = ps_global->VAR_DISPLAY_FILTERS ; l && *l; l++){
	get_pair(*l, &test, &cmd, 1);
	if(test && df_valid_command(&cmd)){
	    *trailp	  = (struct triggers *)fs_get(sizeof(struct triggers));
	    (*trailp)->cmp = df_trigger_cmp_text;
	    str		   = test;
	    if(*test == '_' && (i = strlen(test)) > 10
	       && *(ep = &test[i-1]) == '_' && *--ep == ')'){
		if(struncmp(test, "_CHARSET(", 9) == 0){
		    fs_give((void **)&test);
		    fs_give((void **)&cmd);
		    fs_give((void **)trailp);
		    continue;
		}

		if(strncmp(test+1, "LEADING(", 8) == 0){
		    (*trailp)->cmp = df_trigger_cmp_lwsp;
		    *ep		   = '\0';
		    str		   = cpystr(test+9);
		    fs_give((void **)&test);
		}
		else if(strncmp(test+1, "BEGINNING(", 10) == 0){
		    (*trailp)->cmp = df_trigger_cmp_start;
		    *ep		   = '\0';
		    str		   = cpystr(test+11);
		    fs_give((void **)&test);
		}
	    }

	    (*trailp)->text = str;
	    (*trailp)->cmd = cmd;
	    *(trailp = &(*trailp)->next) = NULL;
	}
	else{
	    fs_give((void **)&test);
	    fs_give((void **)&cmd);
	}
    }

    return(tp);
}


/*
 * blast_trigger_list - zot any list of triggers we've been using 
 */
void
blast_trigger_list(tlist)
    struct triggers **tlist;
{
    if((*tlist)->next)
      blast_trigger_list(&(*tlist)->next);

    fs_give((void **)&(*tlist)->text);
    fs_give((void **)&(*tlist)->cmd);
    fs_give((void **)tlist);
}


/*
 * df_trigger_cmp - compare the line passed us with the list of defined
 *		    display filter triggers
 */
int
df_trigger_cmp(n, s)
    long  n;
    char *s;
{
    register struct triggers *tp;
    int			      result;

    if(!display_filter)				/* already found? */
      for(tp = df_trigger_list; tp; tp = tp->next)
	if(tp->cmp){
	    if((result = (*tp->cmp)(s, tp->text)) < 0)
	      tp->cmp = NULL;
	    else if(result > 0)
	      return((display_filter = tp->cmd) != NULL);
	}

    return(0);
}


/*
 * df_trigger_cmp_text - return 1 if s1 is in s2
 */
int
df_trigger_cmp_text(s1, s2)
    char *s1, *s2;
{
    return(strstr(s1, s2) != NULL);
}


/*
 * df_trigger_cmp_lwsp - compare the line passed us with the list of defined
 *		         display filter triggers. returns:
 *
 *		    0  if we don't know yet
 *		    1  if we match
 *		   -1  if we clearly don't match
 */
int
df_trigger_cmp_lwsp(s1, s2)
    char *s1, *s2;
{
    while(*s1 && isspace((unsigned char)*s1))
      s1++;

    return((*s1) ? (!strncmp(s1, s2, strlen(s2)) ? 1 : -1) : 0);
}


/*
 * df_trigger_cmp_start - return 1 if first strlen(s2) chars start s1
 */
int
df_trigger_cmp_start(s1, s2)
    char *s1, *s2;
{
    return(!strncmp(s1, s2, strlen(s2)));
}


/*
 * df_valid_command - make sure argv[0] of command really exists.
 *		      "cmd" is required to be an alloc'd string since
 *		      it will get realloc'd if the command's path is
 *		      expanded.
 */
int
df_valid_command(cmd)
    char **cmd;
{
    int  i;
    char cpath[MAXPATH+1], *p;

    /*
     * copy cmd to build expanded path if necessary.
     */
    for(i = 0; cpath[i] = (*cmd)[i]; i++)
      if(isspace((unsigned char)(*cmd)[i])){
	  cpath[i] = '\0';		/* tie off command's path*/
	  break;
      }

#if	defined(DOS) || defined(OS2)
    if(is_absolute_path(cpath)){
	fixpath(cpath, MAXPATH);
	p = (char *) fs_get(strlen(cpath) + strlen(&(*cmd)[i]) + 1);
	strcpy(p, cpath);		/* copy new path */
	strcat(p, &(*cmd)[i]);		/* and old args */
	fs_give((void **) cmd);		/* free it */
	*cmd = p;			/* and assign new buf */
    }
#else
    if(cpath[0] == '~'){
	if(fnexpand(cpath, MAXPATH)){
	    p = (char *) fs_get(strlen(cpath) + strlen(&(*cmd)[i]) + 1);
	    strcpy(p, cpath);		/* copy new path */
	    strcat(p, &(*cmd)[i]);	/* and old args */
	    fs_give((void **) cmd);	/* free it */
	    *cmd = p;			/* and assign new buf */
	}
	else
	  return(FALSE);
    }
#endif

    return(is_absolute_path(cpath) && can_access(cpath, EXECUTE_ACCESS) == 0);
}



/*
 * dfilter - pipe the data from the given storage object thru the
 *	     global display filter and into whatever the putchar's
 *	     function points to.
 */
char *
dfilter(rawcmd, input_so, output_pc, aux_filters)
    char    *rawcmd;
    STORE_S *input_so;
    gf_io_t  output_pc;
    filter_t   *aux_filters;
{
    char *status = NULL, *cmd, *resultf = NULL, *tmpfile = NULL;
    int   key = 0;

    if(cmd = expand_filter_tokens(rawcmd, NULL, &tmpfile, &resultf, &key)){
	suspend_busy_alarm();
#ifndef	_WINDOWS
	ps_global->mangled_screen = 1;
	ClearScreen();
	fflush(stdout);
#endif

	/*
	 * If it was requested that the interaction take place via
	 * a tmpfile, fill it with text from our input_so, and let
	 * system_pipe handle the rest...
	 */
	if(tmpfile){
	    PIPE_S	  *filter_pipe;
	    FILE	  *fp;
	    gf_io_t	   gc, pc;

	    /* write the tmp file */
	    so_seek(input_so, 0L, 0);
	    if(fp = fopen(tmpfile, WRITE_MODE)){
		/* copy input to tmp file */
		gf_set_so_readc(&gc, input_so);
		gf_set_writec(&pc, fp, 0L, FileStar);
		gf_filter_init();
		if(!(status = gf_pipe(gc, pc))){
		    fclose(fp);		/* close descriptor */
		    if(filter_pipe = open_system_pipe(cmd, NULL, NULL,
						      PIPE_USER | PIPE_RESET)){
			(void) close_system_pipe(&filter_pipe);

			/* pull result out of tmp file */
			if(fp = fopen(tmpfile, READ_MODE)){
			    gf_set_readc(&gc, fp, 0L, FileStar);
			    gf_filter_init();
			    while(aux_filters && *aux_filters)
			      gf_link_filter(*aux_filters++);

			    status = gf_pipe(gc, output_pc);
			    fclose(fp);
			}
			else
			  status = "Can't read result of display filter";
		    }
		    else
		      status = "Can't open pipe for display filter";
		}

		unlink(tmpfile);
	    }
	    else
	      status = "Can't open display filter tmp file";
	}
	else if(status = gf_filter(cmd, key ? filter_session_key() : NULL,
				   input_so, output_pc, aux_filters)){
	    int ch;

	    fprintf(stdout,"\r\n%s  Hit return to continue.", status);
	    fflush(stdout);
	    while((ch = read_char(300)) != ctrl('M') && ch != NO_OP_IDLE)
	      putchar(BELL);
	}

	if(resultf){
	    if(name_file_size(resultf) > 0L)
	      display_output_file(resultf, "Filter", NULL);

	    fs_give((void **)&resultf);
	}

	resume_busy_alarm();
#ifndef	_WINDOWS
	ClearScreen();
#endif
	fs_give((void **)&cmd);
    }

    return(status);
}


/*
 * expand_filter_tokens - return an alloc'd string with any special tokens
 *			  in the given filter expanded, NULL otherwise.
 */
char *
expand_filter_tokens(filter, env, tmpf, resultf, key)
    char      *filter;
    ENVELOPE  *env;
    char     **tmpf, **resultf;
    int       *key;
{
    char *cmd = NULL, *rlp = NULL, *tfp = NULL, *rfp = NULL,
	 *dfp = NULL, *skp = NULL;

    /*
     * Here's where we scan the filter replacing:
     * _TMPFILE_		temp file name holding data to filter
     * _RECIPIENTS_		space delimited list of recipients
     * _DATAFILE_		file for filter invocations to keep state
     */
    rlp = strstr(filter, "_RECIPIENTS_");
    tfp = strstr(filter, "_TMPFILE_");
    rfp = strstr(filter, "_RESULTFILE_");
    dfp = strstr(filter, "_DATAFILE_");
    skp = strstr(filter, "_PREPENDKEY_");
    if(rlp || tfp || dfp || rfp || skp){
	char *tfn = NULL, *dfn = NULL, *rfn = NULL,  *rl = NULL;

	if(tfp){
	    tfn = temp_nam(NULL, "sf");		/* send filter file */
	    if(tmpf)
	      *tmpf = tfn;
	}

	if(dfp)
	  dfn = filter_data_file(1);		/* filter data file */

	if(rfp){
	    rfn = temp_nam(NULL, "rf");		/* result to show the user */
	    if(resultf)
	      *resultf = rfn;
	}

	if(rlp && env){
	    char *to_l = addr_list_string(env->to,
					  simple_addr_string, 0),
		 *cc_l = addr_list_string(env->cc,
					  simple_addr_string, 0),
		 *bcc_l = addr_list_string(env->bcc,
					   simple_addr_string, 0);

	    rl = fs_get(strlen(to_l) + strlen(cc_l) + strlen(bcc_l) + 3);
	    sprintf(rl, "%s %s %s", to_l, cc_l, bcc_l);
	    fs_give((void **)&to_l);
	    fs_give((void **)&cc_l);
	    fs_give((void **)&bcc_l);
	    for(to_l = rl; *to_l; to_l++)	/* to_l overloaded! */
	      if(*to_l == ',')			/* space delim'd list */
		*to_l = ' ';
	}

	cmd = (char *)fs_get(strlen(filter) + 1
			     + ((tfn) ? strlen(tfn) : 0)
			     + ((rl) ? strlen(rl) : 0)
			     + ((rfn) ? strlen(rfn) : 0)
			     + ((dfn) ? strlen(dfn) : 0));
	strcpy(cmd, filter);
	if(rlp){
	    rplstr(cmd + (rlp - filter), 12, rl ? rl : "");
	    if(rl)
	      fs_give((void **)&rl);
	}

	if(skp = strstr(cmd, "_PREPENDKEY_")){
	    rplstr(skp, 12, "");
	    if(key)
	      *key = 1;
	}

	if(rfp = strstr(cmd, "_RESULTFILE_")){
	    if(!rfn){
		rplstr(rfp, 12, "");		/* couldn't create it! */
		dprint(1, (debugfile, "FAILED creat of _RESULTFILE_\n"));
	    }
	    else
	      rplstr(rfp, 12, rfn);
	}

	if(dfp = strstr(cmd, "_DATAFILE_")){
	    if(!dfn){
		rplstr(dfp, 10, "");		/* couldn't create it! */
		dprint(1, (debugfile, "FAILED creat of _DATAFILE_\n"));
	    }
	    else
	      rplstr(dfp, 10, dfn);
	}

	if(tfp = strstr(cmd, "_TMPFILE_"))
	  rplstr(tfp, 9, tfn);
    }
    else
      cmd = cpystr(filter);

    return(cmd);
}




/*
 * filter_data_file - function to return filename of scratch file for
 *		      display and sending filters.  This file is created
 *		      the first time it's needed, and persists until pine
 *		      exits.
 */
char *
filter_data_file(create_it)
    int create_it;
{
    static char *fn = NULL;

    if(!fn && create_it){
	if(fn = temp_nam(NULL, "df"))
	  if(creat(fn, S_IREAD|S_IWRITE) < 0)	/* owner read/write */
	    fs_give((void **)&fn);
    }
    
    return(fn);
}


/*
 * filter_session_key - function to return randomly generated number
 *			representing a key for this session.  The idea is
 *			the display/sending filter could use it to muddle
 *			up any pass phrase or such stored in the
 *			"_DATAFILE_".
 */
char *
filter_session_key()
{
    static char *key = NULL;

    if(!key){
	sprintf(tmp_20k_buf, "%ld", random());
	key = cpystr(tmp_20k_buf);
    }
    
    return(key);
}
