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
     mailview.c
     Implements the mailview screen
     Also includes scrolltool used to display help text
  ====*/


#include "headers.h"


/*----------------------------------------------------------------------
    Saved state for scrolling text 
 ----*/
typedef struct scroll_text {
    void *text;          /* Original text */
    char **text_lines;   /* Lines to display */
    FILE  *findex;	 /* file containing line offsets in another file */
    char  *fname;	 /* name of file containing line offsets */
    long top_text_line;  /* index into text array of line on top of screen */
    long num_lines;      /* Calculated number lines of text to display */
    int lines_allocated; /* size of array text_lines */
    int screen_length;	 /* screen length in lines of text displayed by
			  * scroll tool  (== PGSIZE). */
    int screen_width;    /* screen width of current formatting */
    int screen_start_line; /* First line on screen that we scroll text on */
    int screen_other_lines;/* Line ons screen not used for scroll text */
    short *line_lengths;   /* Lengths of lines for display, not \0 terminatd*/
    SourceType  source;	 /* How to interpret "text" field */
    TextType    style;	 /* scrolltool screen mode; message, news, etc. */
} SCROLL_S;


/*
 * Def's to help in sorting out multipart/alternative
 */
#define	SHOW_NONE	0
#define	SHOW_PARTS	1
#define	SHOW_ALL_EXT	2
#define	SHOW_ALL	3


/*
 * Definitions to help scrolltool
 */
#define SCROLL_LINES_ABOVE(X)	HEADER_ROWS(X)
#define SCROLL_LINES_BELOW(X)	FOOTER_ROWS(X)
#define	SCROLL_LINES(X)		max(((X)->ttyo->screen_rows               \
			 - SCROLL_LINES_ABOVE(X) - SCROLL_LINES_BELOW(X)), 0)
#define	get_scroll_text_lines()	(scroll_state(SS_CUR)->num_lines)


/*
 * Definitions for various scroll state manager's functions
 */
#define	SS_NEW	1
#define	SS_CUR	2
#define	SS_FREE	3


#define	PGSIZE(X)      (ps_global->ttyo->screen_rows - (X)->screen_other_lines)
#define	ISRFCEOL(S)    (*(S) == '\015' && *((S)+1) == '\012')

#define TYPICAL_BIG_MESSAGE_LINES 200 


/*
 * Internal prototypes
 */
int        is_an_env_hdr PROTO((char *, int));
void       format_env_hdr PROTO((MAILSTREAM *, long, ENVELOPE *, gf_io_t,
				 char *, char *));
int	   format_raw_header PROTO((MAILSTREAM *, long, gf_io_t, char *));
void	   format_addr_string PROTO((MAILSTREAM *, long, char *, ADDRESS *,
				     char *, gf_io_t));
void	   format_newsgroup_string PROTO((char *, char *, char *, gf_io_t));
int	   format_raw_hdr_string PROTO((char *, char *, gf_io_t, char *));
int	   format_env_puts PROTO((char *, gf_io_t));
int        delineate_this_header PROTO ((char *,char *,char *,char **,char **));
void	   set_scroll_text PROTO((SCROLL_S *, void *, long, SourceType,
				  TextType));
long	   scroll_scroll_text PROTO((long, int));
static int print_to_printer PROTO((void *, SourceType, char *));
int	   search_scroll_text PROTO((long, int, char *, Pos *));
void       describe_mime PROTO((BODY *, char *, int, int));
void       format_mime_size PROTO((char *, BODY *));
ATTACH_S  *next_attachment PROTO(());
void       zero_atmts PROTO((ATTACH_S *));
void	   zero_scroll_text PROTO((void));
void	   ScrollFile PROTO((long));
long	   make_file_index PROTO(());
char      *show_multipart PROTO((MESSAGECACHE *, int));
int	   mime_show PROTO((BODY *));
char      *part_desc PROTO((char *,BODY *, int));
SCROLL_S  *scroll_state PROTO((int));
int	   search_text PROTO((int, long, int, char *, Pos *));
void	   format_scroll_text PROTO((void));
void	   redraw_scroll_text PROTO((void));
void	   update_scroll_titlebar PROTO((char *, TextType, long, int));
#ifdef	_WINDOWS
int	   mswin_readscrollbuf PROTO((int));
int	   scroll_scroll_callback PROTO((int, long));
#endif



/*----------------------------------------------------------------------
     Format a buffer with the text of the current message for browser

    Args: ps - pine state structure
  
  Result: The scrolltool is called to display the message

  Loop here viewing mail until the folder changed or a command takes
us to another screen. Inside the loop the message text is fetched and
formatted into a buffer allocated for it.  These are passed to the
scrolltool(), that displays the message and executes commands. It
returns when it's time to display a different message, when we
change folders, when it's time for a different screen, or when
there are no more messages available.
  ---*/

void
mail_view_screen(ps)
     struct pine *ps;
{
    char            last_was_full_header = 0, coerce_seen_bit;
    long            last_message_viewed = -1L, raw_msgno;
    int             we_cancel = 0;
    MESSAGECACHE   *mc;
    ENVELOPE       *env;
    BODY           *body;
    STORE_S        *store;
    gf_io_t         pc;
    SourceType	    src = CharStar;

    dprint(1, (debugfile, "\n\n  -----  MAIL VIEW  -----\n"));

    /*----------------- Loop viewing messages ------------------*/
    do {
	/*
	 * Check total to make sure there's something to view.  Check it
	 * inside the loop to make sure everything wasn't expunged while
	 * we were viewing.  If so, make sure we don't just come back.
	 */
	if(mn_get_total(ps->msgmap) <= 0L || !ps->mail_stream){
	    q_status_message(SM_ORDER, 0, 3, "No messages to read!");
	    ps->next_screen = (ps->prev_screen != mail_view_screen)
				? ps->prev_screen : mail_index_screen;
	    break;
	}

	we_cancel = busy_alarm(1, NULL, NULL, 0);

	if(mn_get_cur(ps->msgmap) <= 0L)
	  mn_set_cur(ps->msgmap, 1L);

	raw_msgno = mn_m2raw(ps->msgmap, mn_get_cur(ps->msgmap));
	body      = NULL;
	if(!(env = mail_fetchstructure(ps->mail_stream, raw_msgno, &body))
	   || !(mc = mail_elt(ps->mail_stream, raw_msgno))){
	    q_status_message1(SM_ORDER, 3, 3, "Error getting message %s data",
			      comatose(mn_get_cur(ps->msgmap)));
	    dprint(1, (debugfile, "!!!! ERROR fetching %s of msg %ld\n",
		       env ? "elt" : "env",
		       mn_get_cur(ps_global->msgmap)));
	    ps->next_screen = (ps->prev_screen != mail_view_screen)
				? ps->prev_screen : mail_index_screen;
	    break;
	}

	if(!mc->seen){		/* count state change in check_point */
	    clear_index_cache_ent(mn_get_cur(ps->msgmap));
	    check_point_change();
	    ps_global->unseen_in_view = 1;
	}
	else
	  ps_global->unseen_in_view = 0;

#if	defined(DOS) && !defined(WIN32)
	/* 
	 * Handle big text for DOS here.
	 *
	 * judging from 1000+ message folders around here, it looks
	 * like 9X% of messages fall in the < 8k range, so it
	 * seems like this is as good a place to draw the line as any.
	 *
	 * We ALSO need to divert all news articles we read to secondary
	 * storage as its possible we're using c-client's NNTP driver
	 * which returns BOGUS sizes UNTIL the whole thing is fetched.
	 * Note: this is more a deficiency in NNTP than in c-client.
	 */
	src = (mc->rfc822_size > MAX_MSG_INCORE 
	       || strcmp(ps->mail_stream->dtb->name, "nntp") == 0)
		? FileStar : CharStar;
#endif
	store = so_get(src, NULL, EDIT_ACCESS);
	gf_set_so_writec(&pc, store);

	(void) format_message(raw_msgno, env, body,
			      (last_message_viewed != mn_get_cur(ps->msgmap)
			       || last_was_full_header == 1) ? FM_NEW_MESS : 0,
			      pc);

        last_message_viewed  = mn_get_cur(ps->msgmap);
        last_was_full_header = ps->full_header;

        ps->next_screen = SCREEN_FUN_NULL;
        scrolltool(so_text(store), "MESSAGE TEXT",
                   MessageText, src, (ATTACH_S *)NULL);

	ps_global->unseen_in_view = 0;
	so_give(&store);	/* free resources associated with store */
    }
    while(ps->next_screen == SCREEN_FUN_NULL);

    if(we_cancel)
      cancel_busy_alarm(-1);

    ps->prev_screen = mail_view_screen;
}



/*----------------------------------------------------------------------
    Add lines to the attachments structure
    
  Args: body   -- body of the part being described
        prefix -- The prefix for numbering the parts
        num    -- The number of this specific part
        should_show -- Flag indicating which of alternate parts should be shown

Result: The ps_global->attachments data structure is filled in. This
is called recursively to descend through all the parts of a message. 
The description strings filled in are malloced and should be freed.

  ----*/
void
describe_mime(body, prefix, num, should_show)
    BODY *body;
    char *prefix;
    int   num, should_show;
{
    PART      *part;
    char       numx[512], string[200], *description;
    int        n, named, use_viewer;
    ATTACH_S  *a;
    PARAMETER *param;

    if(!body)
      return;

    if(body->type == TYPEMULTIPART){
	int alt_to_show = 0;

        if(strucmp(body->subtype, "alternative") == 0){
	    int effort, best_effort = SHOW_NONE;

	    /*---- Figure out which alternative part to display ---*/
	    for(part=body->contents.part, n=1; part; part=part->next, n++)
	      if((effort = mime_show(&part->body)) >= best_effort
		 && effort != SHOW_ALL_EXT){
		  best_effort = effort;
		  alt_to_show = n;
	      }
	}

	for(part=body->contents.part, n=1; part; part=part->next, n++){
	    sprintf(numx, "%s%d.", prefix, n);
	    describe_mime(&(part->body),
			  (part->body.type == TYPEMULTIPART) ? numx : prefix,
			  n, (n == alt_to_show || !alt_to_show));
	}
    }
    else{
	format_mime_size((a = next_attachment())->size, body);

	description = (body->description)
		        ? body->description
			: (body->type == TYPEMESSAGE
			   && body->encoding <= ENCBINARY
			   && body->subtype
			   && strucmp(body->subtype, "rfc822") == 0
			   && body->contents.msg.env
			   && body->contents.msg.env->subject)
			   ? body->contents.msg.env->subject
			   : NULL;

        sprintf(string, "%s%s%.*s%s",
                type_desc(body->type, body->subtype, body->parameter, 0),
                description ? ", \"" : "",
                sizeof(string) - 20, 
                description ? description : "",
                description ? "\"": "");

        a->description = cpystr(string);
        a->body        = body;
	for(named = 0, param = body->parameter; param; param = param->next)
	  if(strucmp(param->attribute, "name") == 0){
	      /* 
	       * before we buy the name param, make sure it's not a
	       * name assigned by a particularly helpful UA.  cool.
	       */
	      named = strucmp(param->value, "Message Body");
	      break;
	  }

	/*
	 * Make sure we have the tools available to display the
	 * type/subtype, *AND* that we can decode it if needed.
	 * Of course, if it's text, we display it anyway in the
	 * mail_view_screen so put off testing mailcap until we're
	 * explicitly asked to display that segment 'cause it could
	 * be expensive to test...
	 */
	use_viewer = 0;
        a->can_display = (body->type == TYPETEXT
			  && !strucmp(body->subtype, "plain") && !named)
			   ? CD_DEFERRED
			     : (mime_can_display(body->type, body->subtype,
						 body->parameter, &use_viewer)
				&& body->encoding < ENCOTHER)
			      ? CD_GOFORIT
			      : CD_NOCANDO;

	a->use_external_viewer = use_viewer;
        a->shown = (((body->type == TYPETEXT
		      && ((!(*prefix && strcmp(prefix, "1.")) && num == 1)
			  || !(named || use_viewer)))
		    || (body->type == TYPEMESSAGE
			&& body->encoding <= ENCBINARY))
		    && a->can_display != CD_NOCANDO
		    && should_show);
	a->number = (char *)fs_get((strlen(prefix) + 16)* sizeof(char));
        sprintf(a->number, "%s%d",prefix, num);
        (a+1)->description = NULL;
        if(body->type == TYPEMESSAGE && body->encoding <= ENCBINARY
	   && body->subtype && strucmp(body->subtype, "rfc822") == 0){
	    body = body->contents.msg.body;
	    sprintf(numx, "%s%d.", prefix, num);
	    describe_mime(body, numx, 1, should_show);
        }
    }
}



/*----------------------------------------------------------------------
  Return a pointer to the next attachment struct
    
  Args: none

  ----*/
ATTACH_S *
next_attachment()
{
    ATTACH_S *a;
    int       n;

    for(a = ps_global->atmts; a->description; a++)
      ;

    if((n = a - ps_global->atmts) + 1 >= ps_global->atmts_allocated){
	ps_global->atmts_allocated *= 2;
	fs_resize((void **)&ps_global->atmts,
		  ps_global->atmts_allocated * sizeof(ATTACH_S));
	a = &ps_global->atmts[n];
    }

    return(a);
}



/*----------------------------------------------------------------------
   Zero out the attachments structure and free up storage
  ----*/
void
zero_atmts(atmts)
     ATTACH_S *atmts;
{
    ATTACH_S *a;

    for(a = atmts; a->description != NULL; a++){
	fs_give((void **)&(a->description));
	fs_give((void **)&(a->number));
    }

    atmts->description = NULL;
}


char *
body_type_names(t)
    int t;
{
    static char body_type[32];
    char   *p;

    strncpy(body_type,				/* copy the given type */
	    (t > -1 && t < TYPEMAX && body_types[t])
	      ? body_types[t] : "Other", 31);

    for(p = body_type + 1; *p; p++)		/* make it presentable */
      if(isupper((unsigned char)*p))
	*p = tolower((unsigned char)(*p));

    return(body_type);				/* present it */
}


/*----------------------------------------------------------------------
  Mapping table use to neatly display charset parameters
 ----*/

static struct set_names {
    char *rfcname,
	 *humanname;
} charset_names[] = {
    {"US-ASCII",		"Plain Text"},
    {"ISO-8859-1",		"Latin 1"},
    {"ISO-8859-2",		"Latin 2"},
    {"ISO-8859-3",		"Latin 3"},
    {"ISO-8859-4",		"Latin 4"},
    {"ISO-8859-5",		"Latin & Cyrillic"},
    {"ISO-8859-6",		"Latin & Arabic"},
    {"ISO-8859-7",		"Latin & Greek"},
    {"ISO-8859-8",		"Latin & Hebrew"},
    {"ISO-8859-9",		"Latin 5"},
    {"X-ISO-8859-10",		"Latin 6"},
    {"KOI8-R",			"Latin & Russian"},
    {"VISCII",			"Latin & Vietnamese"},
    {"ISO-2022-JP",		"Latin & Japanese"},
    {"ISO-2022-KR",		"Latin & Korean"},
    {"UNICODE-1-1",		"Unicode"},
    {"UNICODE-1-1-UTF-7",	"Mail-safe Unicode"},
    {"ISO-2022-JP-2",		"Multilingual"},
    {NULL,			NULL}
};


/*----------------------------------------------------------------------
  Return a nicely formatted discription of the type of the part
 ----*/

char *
type_desc(type, subtype, params, full)
     int type, full;
     char *subtype;
     PARAMETER *params;
{
    static char  type_d[200];
    int		 i;
    char	*p;

    p = type_d;
    sstrcpy(&p, body_type_names(type));
    if(full && subtype){
	*p++ = '/';
	sstrcpy(&p, subtype);
    }

    switch(type) {
      case TYPETEXT:
        while(params && strucmp(params->attribute,"charset") != 0)
          params = params->next;

        if(params){
	    for(i = 0; charset_names[i].rfcname; i++)
	      if(!strucmp(params->value, charset_names[i].rfcname)){
		  if(!strucmp(params->value, ps_global->VAR_CHAR_SET)
		     || !strucmp(params->value, "us-ascii"))
		    i = -1;

		  break;
	      }

	    if(i >= 0){			/* charset to write */
		sstrcpy(&p, " (charset: ");
		sstrcpy(&p, charset_names[i].rfcname
			      ? charset_names[i].rfcname : "Unknown");
		if(full){
		    sstrcpy(&p, " \"");
		    sstrcpy(&p, charset_names[i].humanname
			    ? charset_names[i].humanname : params->value);
		    *p++ = '\"';
		}

		sstrcpy(&p, ")");
	    }
        }

        break;

      case TYPEAPPLICATION:
        if(full && subtype && strucmp(subtype, "octet-stream") == 0){
            while(params && strucmp(params->attribute,"name"))
              params = params->next;

	    if(params && params->value)
	      sprintf(p, " (Name: \"%.100s\")", params->value);
        }

        break;

      case TYPEMESSAGE:
	if(full && subtype && strucmp(subtype, "external-body") == 0){
            while(params && strucmp(params->attribute,"access-type"))
              params = params->next;

	    if(params && params->value)
	      sprintf(p, " (%s%.100s)", full ? "Access: " : "",
		      params->value);
	}

	break;

      default:
        break;
    }

    return(type_d);
}
     

void
format_mime_size(string, b)
     char *string;
     BODY *b;
{
    char tmp[10], *p = NULL;

    *string++ = ' ';
    switch(b->encoding){
      case ENCBASE64 :
	if(b->type == TYPETEXT)
	  *(string-1) = '~';

	strcpy(p = string, byte_string((3 * b->size.bytes) / 4));
	break;

      default :
      case ENCQUOTEDPRINTABLE :
	*(string-1) = '~';

      case ENC8BIT :
      case ENC7BIT :
	if(b->type == TYPETEXT)
	  sprintf(string, "%s lines", comatose(b->size.lines));
	else
	  strcpy(p = string, byte_string(b->size.bytes));

	break;
    }


    if(p){
	for(; *p && (isdigit((unsigned char)*p)
		     || ispunct((unsigned char)*p)); p++)
	  ;

	sprintf(tmp, " %-5.5s",p);
	strcpy(p, tmp);
    }
}

        

/*----------------------------------------------------------------------
   Determine if we can show all, some or none of the parts of a body

Args: body --- The message body to check

Returns: SHOW_ALL, SHOW_ALL_EXT, SHOW_PART or SHOW_NONE depending on
	 how much of the body can be shown and who can show it.
 ----*/     
int
mime_show(body)
     BODY *body;
{
    int   effort, best_effort, vwr;
    PART *p;

    if(!body)
      return(SHOW_NONE);

    switch(body->type) {
      default:
	return(mime_can_display(body->type,body->subtype,body->parameter,&vwr)
		 ? ((vwr) ? SHOW_ALL_EXT : SHOW_ALL) : SHOW_NONE);

      case TYPEMESSAGE:
	return(mime_show(body->contents.msg.body) == SHOW_ALL
		? SHOW_ALL: SHOW_PARTS);

      case TYPEMULTIPART:
	best_effort = SHOW_NONE;
        for(p = body->contents.part; p; p = p->next)
	  if((effort = mime_show(&p->body)) > best_effort)
	    best_effort = effort;

	return(best_effort);
    }
}
        


/*----------------------------------------------------------------------
   Format a message message for viewing

 Args: msgno -- The number of the message to view
       env   -- pointer to the message's envelope
       body  -- pointer to the message's body
       flgs  -- possible flags:
                FM_NEW_MESS -- flag indicating a different message being
			       formatted than was formatted last time 
			       function was called
		FM_DO_PRINT -- Indicates formatted text is bound for
			       something other than display by pine
			       (printing, export, etc)

Result: Returns true if no problems encountered, else false.

First the envelope is formatted; next a list of all attachments is
formatted if there is more than one. Then all the body parts are
formatted, fetching them as needed. This includes headers of included
message. Richtext is also formatted. An entry is made in the text for
parts that are not displayed or can't be displayed.  source indicates 
how and where the caller would like to have the text formatted.

 ----*/    
int
format_message(msgno, env, body, flgs, pc)
    long         msgno;
    ENVELOPE    *env;
    BODY        *body;
    int          flgs;
    gf_io_t      pc;
{
    char     *decode_error;
    HEADER_S  h;
    ATTACH_S *a;
    int       show_parts, error_found = 0, i;
    gf_io_t   gc;


    show_parts = !(flgs&FM_DO_PRINT);

    /*---- format and copy envelope ----*/
    if(ps_global->full_header)
      q_status_message(SM_INFO, 0, 3,
		       "Full header mode ON.  All header text being included");

    HD_INIT(&h, ps_global->VAR_VIEW_HEADERS, ps_global->view_all_except,
	    FE_DEFAULT);
    switch(format_header_text(ps_global->mail_stream, msgno, env, &h,pc,NULL)){
			      
      case -1 :			/* write error */
	goto write_error;

      case 1 :			/* fetch error */
	if(!(gf_puts("[ Error fetching header ]",  pc)
	     && !gf_puts(NEWLINE, pc)))
	  goto write_error;

	break;
    }

    if(body == NULL) {
        /*--- Server is not an IMAP2bis, It can't parse MIME
              so we just show the text here. Hopefully the 
              message isn't a MIME message 
          ---*/
	void *text2;
#if	defined(DOS) && !defined(WIN32)
	char *append_file_name;

	/* for now, always fetch to disk.  This could be tuned to
	 * check for message size, then decide to deal with it on disk...
	 */
	mail_parameters(ps_global->mail_stream, SET_GETS, (void *)dos_gets);
	if(!(append_file_name = temp_nam(NULL, "pv"))
	   || !(append_file = fopen(append_file_name, "w+b"))){
	    if(append_file_name)
	      fs_give((void **)&append_file_name);

	    q_status_message1(SM_ORDER,3,3,"Can't make temp file: %s",
			      error_description(errno));
	    return(0);
	}
#endif

        if(text2 = (void *)mail_fetchtext(ps_global->mail_stream, msgno)){
 	    if(!gf_puts(NEWLINE, pc))		/* write delimiter */
	      goto write_error;
#if	defined(DOS) && !defined(WIN32)
	    gf_set_readc(&gc, append_file, 0L, FileStar);
#else
	    gf_set_readc(&gc, text2, (unsigned long)strlen(text2), CharStar);
#endif
	    gf_filter_init();
	    gf_link_filter(gf_nvtnl_local);
	    if(decode_error = gf_pipe(gc, pc)){
                sprintf(tmp_20k_buf, "%s%s    [Formatting error: %s]%s",
			NEWLINE, NEWLINE, decode_error, NEWLINE);
		if(!gf_puts(tmp_20k_buf, pc))
		  goto write_error;
	    }
	}

#if	defined(DOS) && !defined(WIN32)
	fclose(append_file);			/* clean up tmp file */
	append_file = NULL;
	unlink(append_file_name);
	fs_give((void **)&append_file_name);
	mail_gc(ps_global->mail_stream, GC_TEXTS);
	mail_parameters(ps_global->mail_stream, SET_GETS, (void *)NULL);
#endif

	if(!text2){
	    if(!gf_puts(NEWLINE, pc)
	       || !gf_puts("    [ERROR fetching text of message]", pc)
	       || !gf_puts(NEWLINE, pc)
	       || !gf_puts(NEWLINE, pc))
	      goto write_error;
	}

	return(1);
    }

    if(flgs&FM_NEW_MESS) {
        zero_atmts(ps_global->atmts);
        describe_mime(body, "", 1, 1);
    }

    /*---- Calculate the approximate length of what we've got to show  ---*/

    /*=========== Format the header into the buffer =========*/
    /*----- First do the list of parts/attachments if needed ----*/
    if(show_parts && ps_global->atmts[1].description != NULL) {
	int max_num_l = 0, max_size_l = 0;

	if(!gf_puts("Parts/attachments:", pc) || !gf_puts(NEWLINE, pc))
	  goto write_error;

        /*----- Figure display lengths for nice display -----*/
        for(a = ps_global->atmts; a->description != NULL; a++) {
	    if(strlen(a->number) > max_num_l)
	      max_num_l = strlen(a->number);
	    if(strlen(a->size) > max_size_l)
	      max_size_l = strlen(a->size);
	}

        /*----- Format the list of attachments -----*/
        for(a = ps_global->atmts; a->description != NULL; a++) {
	    int i = ps_global->ttyo->screen_cols - max_num_l - max_size_l
		     - 14 - strlen(NEWLINE);
            sprintf(tmp_20k_buf, "   %-*.*s %s  %*.*s  %-*.*s%s",
                    max_num_l, max_num_l, a->number,
                    (a->shown ? "Shown" : (a->can_display != CD_NOCANDO)
					    ? "  OK " : "     "),
                    max_size_l, max_size_l, a->size, i, i, a->description,
		    NEWLINE);
	    if(!format_env_puts(tmp_20k_buf, pc))
	      goto write_error;
        }

	if(!gf_puts("----------------------------------------", pc)
	   || !gf_puts(NEWLINE, pc))
	  goto write_error;
    }

    if(!gf_puts(NEWLINE, pc))		/* write delimiter */
      goto write_error;

    show_parts = 0;

    /*======== Now loop through formatting all the parts =======*/
    for(a = ps_global->atmts; a->description != NULL; a++) {

        if(!a->shown) {
	    if(!gf_puts(part_desc(a->number, a->body,
				  (flgs&FM_DO_PRINT)
				    ? 3
				    : (a->can_display != CD_NOCANDO) ? 1 : 2),
			pc)
	       || ! gf_puts(NEWLINE, pc))
	      goto write_error;

            continue;
        } 

        switch(a->body->type){

          case TYPETEXT:
	    /*
	     * Don't write our delimiter if this text part is
	     * the first part of a message/rfc822 segment...
	     */
	    if(show_parts && a != ps_global->atmts 
	       && a[-1].body && a[-1].body->type != TYPEMESSAGE){
		sprintf(tmp_20k_buf, "%s  [ Part %s: \"%.55s\" ]%s%s",
			NEWLINE, a->number, 
			a->body->description ? a->body->description
					     : "Attached Text",
			NEWLINE, NEWLINE);
		if(!gf_puts(tmp_20k_buf, pc))
		  goto write_error;
	    }

	    error_found += decode_text(a, msgno, pc,
				       (flgs&FM_DO_PRINT) ? QStatus : InLine,
				       !(flgs&FM_DO_PRINT));
            break;

          case TYPEMESSAGE:
            sprintf(tmp_20k_buf, "%s  [ Part %s: \"%.55s\" ]%s%s",
		    NEWLINE, a->number, 
                    a->body->description ? a->body->description
					 : "Included Message",
		    NEWLINE, NEWLINE);
  	    if(!gf_puts(tmp_20k_buf, pc))
	      goto write_error;

	    if(a->body->subtype && strucmp(a->body->subtype, "rfc822") == 0){
		format_envelope(NULL, 0L, a->body->contents.msg.env, pc,
				FE_DEFAULT, NULL);
	    }
            else if(a->body->subtype 
		    && strucmp(a->body->subtype, "external-body") == 0) {
		if(!gf_puts("This part is not included and can be ", pc)
		   || !gf_puts("fetched as follows:", pc)
		   || !gf_puts(NEWLINE, pc)
		   || !gf_puts(display_parameters(a->body->parameter), pc))
		  goto write_error;
            }
	    else
	      error_found += decode_text(a, msgno, pc, 
					 (flgs&FM_DO_PRINT) ? QStatus : InLine,
					 !(flgs&FM_DO_PRINT));

	    if(!gf_puts(NEWLINE, pc))
	      goto write_error;

            break;

          default:
	    if(!gf_puts(part_desc(a->number,a->body,(flgs&FM_DO_PRINT) ? 3:1),
			pc))
	      goto write_error;
        }

	show_parts++;
    }

    return(!error_found);

  write_error:

    if(flgs & FM_DO_PRINT)
      q_status_message1(SM_ORDER, 3, 4, "Error writing message: %s", 
			error_description(errno));
    return(0);
}


/*
 *  This is a list of header fields that are represented canonically
 *  by the c-client's ENVELOPE structure.  The list is used by the
 *  two functions below to decide if a given field is included in this
 *  set.
 */
static struct envelope_s {
    char *name;
    long val;
} envelope_hdrs[] = {
    {"from",		FE_FROM},
    {"sender",		FE_SENDER},
    {"date",		FE_DATE},
    {"to",		FE_TO},
    {"cc",		FE_CC},
    {"bcc",		FE_BCC},
    {"newsgroups",	FE_NEWSGROUPS},
    {"subject",		FE_SUBJECT},
    {"message-id",	FE_MESSAGEID},
    {"reply-to",	FE_REPLYTO},
    {"in-reply-to",	FE_INREPLYTO},
    {NULL,		0}
};



/*
 * is_an_env_hdr - is this name a header in the envelope structure?
 *
 *             name - the header name to check
 * news_is_env_hack - Hack to compensate for c-client deficiency.  Newsgroups
 *                    are normally part of the envelope, unless it is over
 *                    imap, in which case c-client fails to fill it in for us.
 */
int
is_an_env_hdr(name, news_is_env_hack)
char *name;
int   news_is_env_hack;
{
    register int i;

    if(!news_is_env_hack && !strucmp(name, "newsgroups"))
      return(0);

    for(i = 0; envelope_hdrs[i].name; i++)
      if(!strucmp(name, envelope_hdrs[i].name))
	return(1);

    return(0);
}



/*
 * Format a single field from the envelope
 */
void
format_env_hdr(stream, msgno, env, pc, field, prefix)
    MAILSTREAM *stream;
    long	msgno;
    ENVELOPE   *env;
    gf_io_t	pc;
    char       *field;
    char       *prefix;
{
    register int i;

    for(i = 0; envelope_hdrs[i].name; i++)
      if(!strucmp(field, envelope_hdrs[i].name)){
	  format_envelope(stream, msgno, env, pc, envelope_hdrs[i].val,prefix);
	  return;
      }
}


/*
 * Look through header string headers, beginning with begin, for the next
 * occurrence of header field.  Set start to that.  Set end to point one
 * position past all of the continuation lines that go with field.
 * That is, if end is converted to a null
 * character then the string start will be the next occurence of header
 * field including all of its continuation lines.  Headers is assumed to
 * have CRLF's as end of lines.
 *
 * If field is NULL, then we just leave start pointing to begin and
 * make end the end of that header.
 *
 * Returns 1 if found, 0 if not.
 */
int
delineate_this_header(headers, field, begin, start, end)
    char  *headers, *field, *begin;
    char **start, **end;
{
    char tmpfield[MAILTMPLEN+2]; /* copy of field with colon appended */
    char *p;

    if(field == NULL){
	if(!begin || !*begin || isspace((unsigned char)*begin))
	  return 0;
	else
	  *start = begin;
    }
    else{
	strncpy(tmpfield, field, MAILTMPLEN);
	tmpfield[MAILTMPLEN] = '\0';
	strcat(tmpfield, ":");

	*start = srchstr(begin, tmpfield);
	if(!*start)
	  return 0;
    }

    for(p = *start; *p; p++){
	if(ISRFCEOL(p)
	   && (!isspace((unsigned char)*(p+2)) || *(p+2) == '\015')){
	    /*
	     * The final 015 in the test above is to test for the end
	     * of the headers.
	     */
	    *end = p+2;
	    break;
	}
    }

    if(!*p)
      *end = p;
    
    return 1;
}



/*----------------------------------------------------------------------
   Handle fetching and filtering a text message segment to be displayed
   by scrolltool or printed.

Args: att   -- segment to fetch
      msgno -- message number segment is a part of
      pc    -- function to write characters from segment with
      style -- Indicates special handling for error messages
      display_bound -- Indicates special necessary filtering

Returns: 1 if errors encountered, 0 if everything went A-OK

 ----*/     
int
decode_text(att, msgno, pc, style, display_bound)
    ATTACH_S       *att;
    long            msgno;
    gf_io_t         pc;
    DetachErrStyle  style;
    int		    display_bound;
{
    PARAMETER         *param;
    filter_t	       aux_filter[5];
    int		       filtcnt = 0, error_found = 0;
    char	      *err;

    if(F_OFF(F_PASS_CONTROL_CHARS, ps_global)){
	aux_filter[filtcnt++] = gf_escape_filter;
	aux_filter[filtcnt++] = gf_control_filter;
    }

    if(att->body->subtype){
	filter_t rt_filt = NULL;

	if(!strucmp(att->body->subtype, "richtext")){
	    gf_rich2plain_opt(!display_bound);	/* maybe strip everything! */
	    rt_filt = gf_rich2plain;
	}
	else if(!strucmp(att->body->subtype, "enriched")){
	    gf_enriched2plain_opt(!display_bound);
	    rt_filt = gf_enriched2plain;
	}

	if(rt_filt){
	    aux_filter[filtcnt++] = rt_filt;
	    if(!display_bound){
		gf_wrap_filter_opt(75);  /* width to use for file or printer */
		aux_filter[filtcnt++] = gf_wrap;
	    }
	}
	else if(!strucmp(att->body->subtype, "html")
		&& !ps_global->full_header){
		
/*BUG:	    sniff the params for "version=2.0" ala draft-ietf-html-spec-01 */
/*	    aux_filter[filtcnt++] = gf_html2plain;*/
	}
    }

    for(param = att->body->parameter; 
		param != NULL && strucmp(param->attribute,"charset") != 0;
		param = param->next)
	;

    /*
     * If there's a charset specified and it's not US-ASCII, and our
     * local charset's undefined or it's not the same as the specified
     * charset, put up a warning...
     */
    if(param && param->value && strucmp(param->value, "us-ascii")
       && (!ps_global->VAR_CHAR_SET
	   || strucmp(param->value, ps_global->VAR_CHAR_SET)))
      if(!gf_puts("    [The following text is in the \"", pc)
	 || !gf_puts(param->value, pc)
	 || !gf_puts("\" character set]", pc)
	 || !gf_puts(NEWLINE , pc)
	 || !gf_puts("    [Your display is set for the \"" , pc)
	 || !gf_puts(ps_global->VAR_CHAR_SET
		       ? ps_global->VAR_CHAR_SET : "US-ASCII" , pc)
	 || !gf_puts("\" character set]" , pc)
	 || !gf_puts(NEWLINE , pc)
	 || !gf_puts("    [Some characters may be displayed incorrectly]",pc)
	 || !gf_puts(NEWLINE, pc)
	 || !gf_puts(NEWLINE, pc))
	goto write_error;

    aux_filter[filtcnt] = NULL;
    err = detach(ps_global->mail_stream, msgno, att->body, att->number,
		 NULL, pc, aux_filter);
    if(err) {
	error_found++;
	if(style == QStatus) {
	    q_status_message1(SM_ORDER, 3, 4, "%s", err);
	} else if(style == InLine) {
	    sprintf(tmp_20k_buf, "%s   [Error: %s]  %c%s%c%s%s",
		    NEWLINE, err,
		    att->body->description ? '\"' : ' ',
		    att->body->description ? att->body->description : "",
		    att->body->description ? '\"' : ' ',
		    NEWLINE, NEWLINE);
	    if(!gf_puts(tmp_20k_buf, pc))
	      goto write_error;
	}
    }

    if(att->body->subtype
       && (!strucmp(att->body->subtype, "richtext")
	   || !strucmp(att->body->subtype, "enriched"))
       && !display_bound){
	if(!gf_puts(NEWLINE, pc) || !gf_puts(NEWLINE, pc))
	  goto write_error;
    }

    return(error_found);

  write_error:
    if(style == QStatus)
      q_status_message1(SM_ORDER, 3, 4, "Error writing message: %s", 
			error_description(errno));

    return(1);
}




 
/*------------------------------------------------------------------
   This list of known escape sequences is taken from RFC's 1486 and 1554
   and draft-apng-cc-encoding, and the X11R5 source with only a remote
   understanding of what this all means...

   NOTE: if the length of these should extend beyond 4 chars, fix
	 MAX_ESC_LEN in filter.c
  ----*/
static char *known_escapes[] = {
    "(B",  "(J",  "$@",  "$B",			/* RFC 1468 */
    "$A",  "$(C", "$(D", ".A",  ".F",		/* added by RFC 1554 */
    "$)C", "$)A", "$*E", "$*X",			/* those in apng-draft */
    "$+G", "$+H", "$+I", "$+J", "$+K",
    "$+L", "$+M",
    ")I",   "-A",  "-B",  "-C",  "-D",		/* codes form X11R5 source */
    "-F",   "-G",  "-H",   "-L",  "-M",
    "-$(A", "$(B", "$)B", "$)D",
    NULL};

int
match_escapes(esc_seq)
    char *esc_seq;
{
    char **p;
    int    n;

    for(p = known_escapes; *p && strncmp(esc_seq, *p, n = strlen(*p)); p++)
      ;

    return(*p ? n + 1 : 0);
}



/*----------------------------------------------------------------------
  Format header text suitable for display

  Args: stream -- mail stream for various header text fetches
	msgno -- sequence number in stream of message we're interested in
	env -- pointer to msg's envelope
	hdrs -- struct containing what's to get formatted
	pc -- function to write header text with
	prefix -- prefix to append to each output line

  Result: 0 if all's well, -1 if write error, 1 if fetch error

  NOTE: Blank-line delimiter is NOT written here.  Newlines are written
	in the local convention.

 ----*/
#define	FHT_OK		0
#define	FHT_WRTERR	-1
#define	FHT_FTCHERR	1
int
format_header_text(stream, msgno, env, hdrs, pc, prefix)
    MAILSTREAM *stream;
    long	msgno;
    ENVELOPE   *env;
    HEADER_S   *hdrs;
    gf_io_t	pc;
    char       *prefix;
{
    int   rv = FHT_OK;
    int  nfields, i;
    char *h = NULL, **fields = NULL, **v, *p, *q, *start,
	 *finish, *current;

    if(ps_global->full_header)
      return(format_raw_header(stream, msgno, pc, prefix));

    /*
     * First, calculate how big a fields array we need.
     */

    /* Custom header viewing list specified */
    if(hdrs->type == HD_LIST){
	/* view all these headers */
	if(!hdrs->except){
	    for(nfields = 0, v = hdrs->h.l; (q = *v) != NULL; v++)
	      if(!is_an_env_hdr(q, env && env->newsgroups))
		nfields++;
	}
	/* view all except these headers */
	else{
	    for(nfields = 0, v = hdrs->h.l; *v != NULL; v++)
	      nfields++;
	      
	    if(nfields > 1)
	      nfields--; /* subtract one for ALL_EXCEPT field */
	}
    }
    /* default view */
    else{
	nfields = 6;
	if(!(env && env->newsgroups))
	  nfields++;
    }

    /* allocate pointer space */
    if(nfields){
	fields = (char **)fs_get((size_t)(nfields+1) * sizeof(char *));
	memset(fields, 0, (size_t)(nfields+1) * sizeof(char *));
    }

    if(hdrs->type == HD_LIST){
	/* view all these headers */
	if(!hdrs->except){
	    /* put the non-envelope headers in fields */
	    if(nfields)
	      for(i = 0, v = hdrs->h.l; (q = *v) != NULL; v++)
		if(!is_an_env_hdr(q, env && env->newsgroups))
		  fields[i++] = q;
	}
	/* view all except these headers */
	else{
	    /* put the list of headers not to view in fields */
	    if(nfields)
	      for(i = 0, v = hdrs->h.l; (q = *v) != NULL; v++)
		if(strucmp(ALL_EXCEPT, q))
		  fields[i++] = q;
	}

	v = hdrs->h.l;
    }
    else{
	if(nfields){
	    i = 0;
	    if(!(env && env->newsgroups))
	      fields[i++] = "Newsgroups";

	    fields[i++] = "Resent-Date";
	    fields[i++] = "Resent-From";
	    fields[i++] = "Resent-To";
	    fields[i++] = "Resent-cc";
	    fields[i++] = "Resent-Subject";
	    fields[i++] = "Followup-To";
	}

	v = fields;
    }

    /* custom view with exception list */
    if(hdrs->type == HD_LIST && hdrs->except){
	/*
	 * Go through each header in h and print it.
	 * First we check to see if it is an envelope header so we
	 * can print our envelope version of it instead of the raw version.
	 */

	/* fetch all the other headers */
	if(nfields)
	  h = xmail_fetchheader_lines_not(stream, msgno, fields);

	for(current = h;
	    h && delineate_this_header(h,NULL,current,&start,&finish);
	    current = finish){
	    char tmp[MAILTMPLEN+1];
	    char *colon_loc;

	    colon_loc = strindex(start, ':');
	    if(colon_loc && colon_loc < finish){
		strncpy(tmp, start, min(colon_loc-start, MAILTMPLEN));
		tmp[min(colon_loc-start, MAILTMPLEN)] = '\0';
	    }
	    else
	      colon_loc = NULL;

	    if(colon_loc && is_an_env_hdr(tmp, env && env->newsgroups)){
		/* pretty format for env hdrs */
		format_env_hdr(stream, msgno, env, pc, tmp, prefix);
	    }
	    else{
		if(rv = format_raw_hdr_string(start, finish, pc, prefix))
		  goto write_error;
		else
		  start = finish;
	    }
	}
    }
    /* custom view or default */
    else{
	/* fetch the non-envelope headers */
	if(nfields)
	  h = xmail_fetchheader_lines(stream, msgno, fields);

	/* default envelope for default view */
	if(hdrs->type == HD_BFIELD)
	  format_envelope(stream, msgno, env, pc, hdrs->h.b, prefix);

	/* go through each header in list, v initialized above */
	for(; q = *v; v++){
	    if(is_an_env_hdr(q, env && env->newsgroups)){
		/* pretty format for env hdrs */
		format_env_hdr(stream, msgno, env, pc, q, prefix);
	    }
	    else{
		/*
		 * Go through h finding all occurences of this header
		 * and all continuation lines, and output.
		 */
		for(current = h;
		    h && delineate_this_header(h,q,current,&start,&finish);
		    current = finish){
		    if(rv = format_raw_hdr_string(start, finish, pc, prefix))
		      goto write_error;
		    else
		      start = finish;
		}
	    }
	}
    }

  write_error:

    if(h)
      fs_give((void **)&h);

    if(fields)
      fs_give((void **)&fields);

    return(rv);
}



/*----------------------------------------------------------------------
  Format RAW header text for display

  Args: stream --
	msgno --
	pc --
	prefix --

  Result: 0 if all's well, -1 if write error, 1 if fetch error

  NOTE: Blank-line delimiter is NOT written here.  Newlines are written
	in the local convention.

 ----*/
int
format_raw_header(stream, msgno, pc, prefix)
    MAILSTREAM *stream;
    long	msgno;
    gf_io_t	pc;
    char       *prefix;
{
    char *h = mail_fetchheader(stream, msgno);

    if(h){
	if(prefix && !gf_puts(prefix, pc))
	  return(FHT_WRTERR);

	while(*h){
	    if(ISRFCEOL(h)){
		h += 2;
		if(!gf_puts(NEWLINE, pc))
		  return(FHT_WRTERR);

		if(ISRFCEOL(h))		/* all done! */
		  return(FHT_OK);

		if(prefix && !gf_puts(prefix, pc))
		  return(FHT_WRTERR);
	    }
	    else if(F_OFF(F_PASS_CONTROL_CHARS, ps_global) && CAN_DISPLAY(*h)){
		if(!((*pc)('^') && (*pc)(*h++ + '@')))
		  return(FHT_WRTERR);
	    }
	    else if(!(*pc)(*h++))
	      return(FHT_WRTERR);
	}
    }
    else
      return(FHT_FTCHERR);

    return(FHT_OK);
}



/*----------------------------------------------------------------------
  Format c-client envelope data suitable for display

  Args: stream -- stream associated with this envelope
	msgno -- message number associated with this envelope
	e -- envelope
	pc -- place to write result
	which -- which header lines to write
	prefix -- string to write before each header line

  Result: 0 if all's well, -1 if write error, 1 if fetch error

  NOTE: Blank-line delimiter is NOT written here.  Newlines are written
	in the local convention.

 ----*/
void
format_envelope(s, n, e, pc, which, prefix)
    MAILSTREAM *s;
    long	n;
    ENVELOPE   *e;
    gf_io_t     pc;
    long	which;
    char       *prefix;
{
    if(!e)
      return;

    if((which & FE_DATE) && e->date) {
	if(prefix)
	  gf_puts(prefix, pc);

	gf_puts("Date: ", pc);
	format_env_puts(e->date, pc);
	gf_puts(NEWLINE, pc);
    }

    if((which & FE_FROM) && e->from)
      format_addr_string(s, n, "From: ", e->from, prefix, pc);

    if((which & FE_REPLYTO) && e->reply_to
       && (!e->from || !address_is_same(e->reply_to, e->from)))
      format_addr_string(s, n, "Reply-To: ", e->reply_to, prefix, pc);

    if((which & FE_TO) && e->to)
      format_addr_string(s, n, "To: ", e->to, prefix, pc);

    if((which & FE_CC) && e->cc)
      format_addr_string(s, n, "Cc: ", e->cc, prefix, pc);

    if((which & FE_BCC) && e->bcc)
      format_addr_string(s, n, "Bcc: ", e->bcc, prefix, pc);

    if((which & FE_NEWSGROUPS) && e->newsgroups && !ps_global->nr_mode)
      format_newsgroup_string("Newsgroups: ", e->newsgroups, prefix, pc);

    if((which & FE_SUBJECT) && e->subject && e->subject[0]){
	if(prefix)
	  gf_puts(prefix, pc);

	gf_puts("Subject: ", pc);
	format_env_puts((char *) rfc1522_decode((unsigned char *) tmp_20k_buf,
						e->subject, NULL), pc);
	gf_puts(NEWLINE, pc);
    }

    if((which & FE_SENDER) && e->sender
       && (!e->from || !address_is_same(e->sender, e->from)))
      format_addr_string(s, n, "Sender: ", e->sender, prefix, pc);

    if((which & FE_MESSAGEID) && e->message_id){
	if(prefix)
	  gf_puts(prefix, pc);

	gf_puts("Message-ID: ", pc);
	format_env_puts((char *) rfc1522_decode((unsigned char *) tmp_20k_buf,
						e->message_id, NULL), pc);
	gf_puts(NEWLINE, pc);
    }

    if((which & FE_INREPLYTO) && e->in_reply_to){
	if(prefix)
	  gf_puts(prefix, pc);

	gf_puts("In-Reply-To: ", pc);
	format_env_puts((char *) rfc1522_decode((unsigned char *) tmp_20k_buf,
						e->in_reply_to, NULL), pc);
	gf_puts(NEWLINE, pc);
    }
}



/*----------------------------------------------------------------------
     Format an address field, wrapping lines nicely at commas

  Args: field_name  -- The name of the field we're formatting ("TO: ", ...)
        addr        -- ADDRESS structure to format
        line_prefix -- A prefix string for each line such as "> "

 Result: A formatted, malloced string is returned.

The resulting lines formatted are 80 columns wide.
  ----------------------------------------------------------------------*/
void
format_addr_string(stream, msgno, field_name, addr, line_prefix, pc)
    MAILSTREAM *stream;
    long	msgno;
    ADDRESS    *addr;
    char       *line_prefix, *field_name;
    gf_io_t	pc;
{
    char     buf[MAILTMPLEN], *ptmp, *mtmp;
    int	     trailing = 0, was_start_of_group = 0, llen, alen, plen = 0;
    ADDRESS *atmp;

    if(!addr)
      return;

    if(line_prefix)
      gf_puts(line_prefix, pc);

    /*
     * quickly run down address list to make sure none are patently bogus.
     * If so, just blat raw field out.
     */
    for(atmp = addr; stream && atmp; atmp = atmp->next)
      if(atmp->host && atmp->host[0] == '.'){
	  char *field, *fields[2];

	  fields[1] = NULL;
	  fields[0] = cpystr(field_name);
	  if(ptmp = strchr(fields[0], ':'))
	    *ptmp = '\0';

	  if(field = xmail_fetchheader_lines(stream, msgno, fields)){
	      char *h, *t;

	      for(t = h = field; *h ; t++)
		if(*t == '\015' && *(t+1) == '\012'){
		    *t = '\0';			/* tie off line */
		    format_env_puts(h, pc);
		    if(*(h = (++t) + 1)){	/* set new h and skip CRLF */
			gf_puts(NEWLINE, pc);	/* more to write */
			if(line_prefix)
			  gf_puts(line_prefix, pc);
		    }
		    else
		      break;
		}
		else if(!*t){			/* shouldn't happen much */
		    if(h != t)
		      format_env_puts(h, pc);

		    break;
		}

	      fs_give((void **)&field);
	  }

	  fs_give((void **)&fields[0]);
	  gf_puts(NEWLINE, pc);
	  q_status_message1(SM_ORDER, 0, 3, "Error in \"%s\" field address",
			    field_name);
	  return;
      }

    gf_puts(field_name, pc);

    if(line_prefix)
      plen = strlen(line_prefix);

    llen = plen + strlen(field_name);
    while(addr){
	atmp	       = addr->next;		/* remember what's next */
	addr->next     = NULL;
	if(!addr->host && addr->mailbox){
	    mtmp = addr->mailbox;
	    addr->mailbox = cpystr((char *)rfc1522_decode(
			   (unsigned char *)tmp_20k_buf, addr->mailbox, NULL));
	}

	ptmp	       = addr->personal;	/* RFC 1522 personal name? */
	addr->personal = (char *) rfc1522_decode((unsigned char *)tmp_20k_buf,
						 addr->personal, NULL);
	buf[0]	       = '\0';
	rfc822_write_address(buf, addr);	/* write address into buf */
	alen	       = strlen(buf);
	addr->personal = ptmp;			/* restore old personal ptr */
	if(!addr->host && addr->mailbox){
	    fs_give((void **)&addr->mailbox);
	    addr->mailbox = mtmp;
	}

	if(!trailing){				/* 1st pass, just address */
	    llen += alen;
	    trailing++;
	}
	else{					/* else comma, unless */
	    if(!((was_start_of_group && addr->host)  /* 1st addr in group, */
	       || (!addr->host && !addr->mailbox))){ /* or end of group */
		gf_puts(",", pc);
		llen++;
	    }

	    if(alen + llen + 1 > 76){
		gf_puts(NEWLINE, pc);
		if(line_prefix)
		  gf_puts(line_prefix, pc);

		gf_puts("    ", pc);
		llen = alen + plen + 5;
	    }
	    else{
		gf_puts(" ", pc);
		llen += alen + 1;
	    }
	}

	if(alen && llen > 76){		/* handle long addresses */
	    register char *q, *p = &buf[alen-1];

	    while(p > buf){
		if(isspace((unsigned char)*p)
		   && (llen - (alen - (int)(p - buf))) < 76){
		    for(q = buf; q < p; q++)
		      if(F_OFF(F_PASS_CONTROL_CHARS, ps_global)
			 && CAN_DISPLAY(*q)){
			  (*pc)('^');
			  (*pc)(*q + '@');
		      }
		      else
			(*pc)(*q);

		    gf_puts(NEWLINE, pc);
		    gf_puts("    ", pc);
		    format_env_puts(p, pc);
		    break;
		}
		else
		  p--;
	    }

	    if(p == buf)		/* no reasonable break point */
	      format_env_puts(buf, pc);
	}
	else
	  format_env_puts(buf, pc);

	if(!addr->host && addr->mailbox)
	  was_start_of_group = 1;
	else
	  was_start_of_group = 0;

	addr->next = atmp;
	addr       = atmp;
    }

    gf_puts(NEWLINE, pc);
}



/*----------------------------------------------------------------------
  Format an address field, wrapping lines nicely at commas

  Args: field_name  -- The name of the field we're formatting ("TO:", Cc:...)
        newsgrps    -- ADDRESS structure to format
        line_prefix -- A prefix string for each line such as "> "

  Result: A formatted, malloced string is returned.

The resuling lines formatted are 80 columns wide.
  ----------------------------------------------------------------------*/
void
format_newsgroup_string(field_name, newsgrps, line_prefix, pc)
    char    *newsgrps;
    char    *line_prefix, *field_name;
    gf_io_t  pc;
{
    char     buf[MAILTMPLEN];
    int	     trailing = 0, llen, alen, plen = 0;
    char    *next_ng;
    
    if(!newsgrps || !*newsgrps)
      return;
    
    if(line_prefix)
      gf_puts(line_prefix, pc);

    gf_puts(field_name, pc);

    if(line_prefix)
      plen = strlen(line_prefix);

    llen = plen + strlen(field_name);
    while(*newsgrps){
        for(next_ng = newsgrps; *next_ng && *next_ng != ','; next_ng++);
        strncpy(buf, newsgrps, next_ng - newsgrps);
        buf[next_ng - newsgrps] = '\0';
        newsgrps = next_ng;
        if(*newsgrps)
          newsgrps++;
	alen = strlen(buf);
	if(!trailing){			/* first time thru, just address */
	    llen += alen;
	    trailing++;
	}
	else{				/* else preceding comma */
	    gf_puts(",", pc);
	    llen++;

	    if(alen + llen + 1 > 76){
		gf_puts(NEWLINE, pc);
		if(line_prefix)
		  gf_puts(line_prefix, pc);

		gf_puts("    ", pc);
		llen = alen + plen + 5;
	    }
	    else{
		gf_puts(" ", pc);
		llen += alen + 1;
	    }
	}

	if(alen && llen > 76){		/* handle long addresses */
	    register char *q, *p = &buf[alen-1];

	    while(p > buf){
		if(isspace((unsigned char)*p)
		   && (llen - (alen - (int)(p - buf))) < 76){
		    for(q = buf; q < p; q++)
		      (*pc)(*q);	/* write character */

		    gf_puts(NEWLINE, pc);
		    gf_puts("    ", pc);
		    gf_puts(p, pc);
		    break;
		}
		else
		  p--;
	    }

	    if(p == buf)		/* no reasonable break point */
	      gf_puts(buf, pc);
	}
	else
	  gf_puts(buf, pc);
    }

    gf_puts(NEWLINE, pc);
}



/*----------------------------------------------------------------------
  Format a text field that's part of some raw (non-envelope) message header

  Args: start --
        finish --
	pc -- 
	prefix --

  Result: Semi-digested text (RFC 1522 decoded, anyway) written with "pc"

  ----------------------------------------------------------------------*/
int
format_raw_hdr_string(start, finish, pc, prefix)
    char    *start;
    char    *finish;
    gf_io_t  pc;
    char    *prefix;
{
    register char *current;
    char     ch;

    ch = *finish;
    *finish = '\0';
    current = (char *) rfc1522_decode((unsigned char *)tmp_20k_buf,start,NULL);
    if(islower((unsigned char)(*start)))
      *start = toupper((unsigned char)(*start));

    if(prefix && !gf_puts(prefix, pc))
      return(FHT_WRTERR);

    /* output from start to finish */
    while(*current)
      if(ISRFCEOL(current)){
	  current += 2;

	  if(!gf_puts(NEWLINE, pc)
	     || (*current && prefix && !gf_puts(prefix, pc)))
	    return(FHT_WRTERR);
      }
      else if(F_OFF(F_PASS_CONTROL_CHARS, ps_global) && CAN_DISPLAY(*current)){
	  if(!((*pc)('^') && (*pc)(*current++ + '@')))
	    return(FHT_WRTERR);
      }
      else if(!(*pc)(*current++))
	return(FHT_WRTERR);

    *finish = ch;
    return(FHT_OK);
}




/*----------------------------------------------------------------------
  Format a text field that's part of some raw (non-envelope) message header

  Args: s --
	pc -- 

  Result: Output

  ----------------------------------------------------------------------*/
int
format_env_puts(s, pc)
    char    *s;
    gf_io_t  pc;
{
    if(F_ON(F_PASS_CONTROL_CHARS, ps_global))
      return(gf_puts(s, pc));

    for(; *s; s++)
      if(CAN_DISPLAY(*s)){
	  if(!((*pc)('^') && (*pc)(*s + '@')))
	    return(0);
      }
      else if(!(*pc)(*s))
	return(0);

    return(1);
}




/*----------------------------------------------------------------------
    Format a strings describing one unshown part of a Mime message

Args: number -- A string with the part number i.e. "3.2.1"
      body   -- The body part
      type   -- 1 - Not shown, but can be
                2 - Not shown, cannot be shown
                3 - Can't print


Result: pointer to formatted string in static buffer

Note that size of the strings are carefully calculated never to overflow 
the static buffer:
    number  < 20,  description limited to 100, type_desc < 200,
    size    < 20,  second line < 100           other stuff < 60
 ----*/
char *
part_desc(number, body, type)
     BODY *body;
     int type;
     char *number;
{
    char *t;

    sprintf(tmp_20k_buf, "%s  [Part %s, %s%.100s%s%s  %s%s]%s",
	    NEWLINE,
            number,
            body->description == NULL ? "" : "\"",
            body->description == NULL ? "" : body->description,
            body->description == NULL ? "" : "\"  ",
            type_desc(body->type, body->subtype, body->parameter, 1),
            body->type == TYPETEXT ? comatose(body->size.lines) :
                                     byte_string(body->size.bytes),
            body->type == TYPETEXT ? " lines" : "",
	    NEWLINE);

    t = &tmp_20k_buf[strlen(tmp_20k_buf)];

    switch(type) {
      case 1:
        sstrcpy(&t,
	    "  [Not Shown. Use the \"V\" command to view or save this part]");
	sstrcpy(&t, NEWLINE);
        break;

      case 2:
	sstrcpy(&t, "  [Cannot ");
	if(body->type != TYPEAUDIO && body->type != TYPEVIDEO)
	  sstrcpy(&t, "dis");

	sstrcpy(&t, 
		"play this part. Press \"V\" then \"S\" to save in a file]");
	sstrcpy(&t, NEWLINE);
        break;

      case 3:
        sstrcpy(&t, "  [Unable to print this part]");
	sstrcpy(&t, NEWLINE);
        break;
    }

    return(tmp_20k_buf);
}


/*
 * This could be better.  For example, look at the first two.  The
 * thing that keeps us from using the same array is that the column number
 * is stored with each entry and it could be different for each.
 */
static struct key help_keys[] =
       {{"M","Main Menu",KS_MAINMENU},	{NULL,NULL,KS_NONE},
	{"E","Exit Help",KS_EXITMODE},	{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{"Y","prYnt",KS_PRINT},		{"Z","Print All",KS_NONE},
	{"B","Report Bug",KS_NONE},	{"W","WhereIs",KS_WHEREIS}};
INST_KEY_MENU(help_keymenu, help_keys);
#define	HLP_MAIN_KEY	0
#define	HLP_ALL_KEY	9
#define	HLP_BUG_KEY	10

static struct key review_keys[] =
       {{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{"E","Exit",KS_EXITMODE},	{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{"Y","prYnt",KS_PRINT},		{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},		{"W","WhereIs",KS_WHEREIS}};
INST_KEY_MENU(review_keymenu, review_keys);

static struct key view_keys[] = 
       {{"?","Help",KS_SCREENHELP},	{"O","OTHER CMDS",KS_NONE},
	{"M","Main Menu",KS_MAINMENU},	{"V","ViewAttch",KS_VIEW},
	{"P","PrevMsg",KS_PREVMSG},	{"N","NextMsg",KS_NEXTMSG},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{"D","Delete",KS_DELETE},	{"U","Undelete",KS_UNDELETE},
	{"R","Reply",KS_REPLY},		{"F","Forward",KS_FORWARD},

	{"?","Help",KS_SCREENHELP},	{"O","OTHER CMDS",KS_NONE},
	{"Q","Quit",KS_EXIT},		{"C","Compose",KS_COMPOSER},
	{"L","ListFldrs",KS_FLDRLIST},	{"G","GotoFldr",KS_GOTOFLDR},
	{"I","Index",KS_FLDRINDEX},	{"W","WhereIs",KS_WHEREIS},
	{"Y","prYnt",KS_PRINT},		{"T","TakeAddr",KS_TAKEADDR},
	{"S","Save",KS_SAVE},		{"E","Export",KS_EXPORT},

	{"?","Help",KS_SCREENHELP},	{"O","OTHER CMDS",KS_NONE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{"J","Jump",KS_JUMPTOMSG},	{"TAB","NextNew",KS_NONE},
	{"H","HdrMode",KS_HDRMODE},	{"B","Bounce",KS_BOUNCE},
	{"*","Flag",KS_FLAG},		{"|","Pipe",KS_NONE}};
INST_KEY_MENU(view_keymenu, view_keys);
#define VIEW_FULL_HEADERS_KEY 32
#define BOUNCE_KEY 33
#define FLAG_KEY 34
#define VIEW_PIPE_KEY 35

static struct key nr_anon_view_keys[] = 
       {{"?","Help",KS_SCREENHELP},	{"W","WhereIs",KS_WHEREIS},
	{"Q", "Quit",KS_EXIT},		{NULL,NULL,KS_NONE},
	{"P","PrevMsg",KS_PREVMSG},	{"N","NextMsg",KS_NEXTMSG},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{"F","Fwd Email",KS_FORWARD},	{"J","Jump",KS_JUMPTOMSG},
	{"I", "Index",KS_FLDRINDEX},	{NULL, NULL,KS_NONE}};
INST_KEY_MENU(nr_anon_view_keymenu, nr_anon_view_keys);

static struct key nr_view_keys[] = 
       {{"?","Help",KS_SCREENHELP},	{"O","OTHER CMDS",KS_NONE},
	{"Q","Quit",KS_EXIT},		{NULL,NULL,KS_NONE},
	{"P","PrevMsg",KS_PREVMSG},	{"N","NextMsg",KS_NEXTMSG},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{"F","Fwd Email",KS_FORWARD},	{"J","Jump",KS_JUMPTOMSG},
	{"Y","prYnt",KS_PRINT},		{"S","Save",KS_SAVE},

	{"?","Help",KS_SCREENHELP},	{"O","OTHER CMDS",KS_NONE},
	{"E","Export",KS_EXPORT},	{"C","Compose",KS_COMPOSER},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{"I","Index",KS_FLDRINDEX},	{"W","WhereIs",KS_WHEREIS},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE}};
INST_KEY_MENU(nr_view_keymenu, nr_view_keys);

static struct key text_att_view_keys[] =
       {{"?","Help",KS_SCREENHELP},	{NULL,NULL,KS_NONE},
	{"E","Exit Viewer",KS_EXITMODE},	{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{"Y","prYnt",KS_PRINT},		{"S","Save",KS_SAVE},
	{"|","Pipe",KS_NONE},		{"W", "WhereIs",KS_WHEREIS}};
INST_KEY_MENU(text_att_view_keymenu, text_att_view_keys);
#define ATT_SAVE_KEY 9
#define ATT_PIPE_KEY 10


static struct key simple_view_keys[] =
       {{"?","Help",KS_SCREENHELP},	{NULL,NULL,KS_NONE},
	{"Q","Quit Viewer",KS_NONE},	{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{"F","Fwd Email",KS_FORWARD},	{"S","Save",KS_SAVE},
	{NULL,NULL,KS_NONE},		{"W","WhereIs",KS_WHEREIS}};
INST_KEY_MENU(simple_view_keymenu, simple_view_keys);
#define SAVE_KEY 9

#define	STYLE_NAME(s)	(((s) == HelpText || (s) == ComposerHelpText) ? "help"\
			  : ((s) == MessageText) ? "message"\
			  : ((s) == ViewAbookText) ? "expanded entry"\
			  : ((s) == ViewAbookAtt) ? "attachment" : "text")


/*----------------------------------------------------------------------
   routine for displaying help and message text on the screen.

  Args: text          buffer to display
        title         string with title of text being displayed
        style         whether we are display a message, help text ...
	source	      what's text: char **, char * or FILE * ???
 

   This displays in three different kinds of text. One is an array of
lines passed in in text_array. The other is a simple long string of
characters passed in in text. The simple string of characters may have
another string, the header which is prepended to the text with some
special processing. The two header.... args specify how format and
filter the header.

  The style determines what some of the error messages will be, and
what commands are available as different things are appropriate for
help text than for message text etc.

 ---*/

void
scrolltool(text, title, style, source, att)
    void       *text;
    char       *title;
    TextType    style;		/* message, news, etc. */
    SourceType  source;		/* char **, char * or FILE * */
    ATTACH_S   *att;		/* used only with style AttachText */
{
    register long    cur_top_line,  num_display_lines;
    int              result, done, ch, found_on, found_on_col,
		     orig_ch, first_view, force, scroll_lines, km_size,
		     cursor_row, cursor_col, km_popped;
    struct key_menu *km;
    bitmap_t         bitmap;
    OtherMenu        what;
    Pos		     whereis_pos;

    num_display_lines	      = SCROLL_LINES(ps_global);
    km_popped		      = 0;
    ps_global->mangled_screen = 1;

    what	    = FirstMenu;		/* which key menu to display */
    cur_top_line    = 0;
    done	    = 0;
    found_on	    = -1;
    found_on_col    = -1;
    first_view	    = 1;
    force	    = 0;
    ch		    = 'x';			/* for first time through */
    whereis_pos.row = 0;
    whereis_pos.col = 0;

    set_scroll_text(scroll_state(SS_NEW), text, cur_top_line, source, style);
    format_scroll_text();

    setbitmap(bitmap);
    if(style == AttachText) {
      km = &text_att_view_keymenu;
      if(!att){
	  clrbitn(ATT_SAVE_KEY, bitmap);
	  clrbitn(ATT_PIPE_KEY, bitmap);
      }
      else if(F_OFF(F_ENABLE_PIPE, ps_global))
	clrbitn(ATT_PIPE_KEY, bitmap);
    }
    else if(style == SimpleText) {
	km = &simple_view_keymenu;
	if(ps_global->anonymous)
	  clrbitn(SAVE_KEY, bitmap);
    }
    else if(ps_global->anonymous) {
	km = &nr_anon_view_keymenu;
    }
    else if(ps_global->nr_mode) {
	km = &nr_view_keymenu;
    }
    else if(style == MessageText) {
	km = &view_keymenu;
#ifndef DOS
	if(F_OFF(F_ENABLE_PIPE,ps_global))
#endif
	  clrbitn(VIEW_PIPE_KEY, bitmap);	/* always clear for DOS */
	if(F_OFF(F_ENABLE_BOUNCE,ps_global))
	  clrbitn(BOUNCE_KEY, bitmap);
	if(F_OFF(F_ENABLE_FLAG,ps_global))
	  clrbitn(FLAG_KEY, bitmap);
	if(F_OFF(F_ENABLE_FULL_HDR,ps_global))
	  clrbitn(VIEW_FULL_HEADERS_KEY, bitmap);
    }
    else if(style == ReviewMsgsText || style == ViewAbookText
            || style == ViewAbookAtt) {
	km = &review_keymenu;
	if(style == ReviewMsgsText){
	    cur_top_line=max(0, get_scroll_text_lines()-(num_display_lines-2));
	    if(F_ON(F_SHOW_CURSOR, ps_global)){
		whereis_pos.row = get_scroll_text_lines() - cur_top_line;
		found_on	    = get_scroll_text_lines() - 1;
	    }
	}
    }
    else {					/* must be paging help */
	km = &help_keymenu;
	if(style == ComposerHelpText) {		/* composer gets minimum */
	    clrbitn(HLP_MAIN_KEY, bitmap);
	    clrbitn(HLP_BUG_KEY, bitmap);
	}

	if(style != MainHelpText)		/* only main can "print all" */
	  clrbitn(HLP_ALL_KEY, bitmap);
    }

    cancel_busy_alarm(-1);

    while(!done) {
	if(km_popped){
	    km_popped--;
	    if(km_popped == 0){
		clearfooter(ps_global);
		ps_global->mangled_body = 1;
	    }
	}

	if(ps_global->mangled_screen) {
	    ps_global->mangled_header = 1;
	    ps_global->mangled_footer = 1;
            ps_global->mangled_body   = 1;
	}

        if(streams_died())
          ps_global->mangled_header = 1;

        dprint(9, (debugfile, "@@@@ current:%ld\n",
		   mn_get_cur(ps_global->msgmap)));


        /*==================== All Screen painting ====================*/
        /*-------------- The title bar ---------------*/
	update_scroll_titlebar(title, style, cur_top_line,
			       ps_global->mangled_header);

	if(ps_global->mangled_screen){
	    /* this is the only line not cleared by header, body or footer
	     * repaint calls....
	     */
	    ClearLine(1);
            ps_global->mangled_screen = 0;
	}

        /*---- Scroll or update the body of the text on the screen -------*/
        cur_top_line		= scroll_scroll_text(cur_top_line,
						     ps_global->mangled_body);
	ps_global->redrawer	= redraw_scroll_text;
        ps_global->mangled_body = 0;

        /*------------- The key menu footer --------------------*/
	if(ps_global->mangled_footer) {
	    if(km_popped){
		FOOTER_ROWS(ps_global) = 3;
		clearfooter(ps_global);
	    }

            draw_keymenu(km, bitmap, ps_global->ttyo->screen_cols,
		1-FOOTER_ROWS(ps_global),0,what,0);
	    what = SameTwelve;
	    ps_global->mangled_footer = 0;
	    if(km_popped){
		FOOTER_ROWS(ps_global) = 1;
		mark_keymenu_dirty();
	    }
	}

	/*============ Check for New Mail and CheckPoint ============*/
        if(new_mail(force, first_view ? 0 : NM_TIMING(ch), 1) >= 0)
	  update_scroll_titlebar(title, style, cur_top_line, 1);

	if(first_view && num_display_lines >= get_scroll_text_lines())
	  q_status_message1(SM_INFO, 0, 1, "ALL of %s", STYLE_NAME(style));

	force      = 0;		/* may not need to next time around */
	first_view = 0;		/* check_point a priority any more? */

	/*==================== Output the status message ==============*/
	if(km_popped){
	    FOOTER_ROWS(ps_global) = 3;
	    mark_status_unknown();
	}

        display_message(ch);
	if(km_popped){
	    FOOTER_ROWS(ps_global) = 1;
	    mark_status_unknown();
	}

	if(F_ON(F_SHOW_CURSOR, ps_global)){
	    if(whereis_pos.row > 0){
		cursor_row  = SCROLL_LINES_ABOVE(ps_global)
				+ whereis_pos.row - 1;
		cursor_col  = whereis_pos.col;
	    }
	    else{
		cursor_col = 0;
		/* first new line of text */
	        cursor_row  = SCROLL_LINES_ABOVE(ps_global) +
		    ((cur_top_line == 0) ? 0 : ps_global->viewer_overlap);
	    }
	}
	else{
	    cursor_col = 0;
	    cursor_row = ps_global->ttyo->screen_rows
			    - SCROLL_LINES_BELOW(ps_global);
	}

	MoveCursor(cursor_row, cursor_col);

	/*================ Get command and validate =====================*/
#ifdef	_WINDOWS
	mswin_allowcopy(mswin_readscrollbuf);
	mswin_setscrollcallback(scroll_scroll_callback);
#endif
        ch = read_command();
        orig_ch = ch;
#ifdef	_WINDOWS
	mswin_allowcopy(NULL);
	mswin_setscrollcallback(NULL);
	cur_top_line = scroll_state(SS_CUR)->top_text_line;
#endif

        if(ch < 0x0100 && isupper((unsigned char)ch))
	  ch = tolower((unsigned char)ch);
        else if(ch >= PF1 && ch <= PF12 && km->which > 0 && km->which < 3)
	  ch = (km->which == 1) ? PF2OPF(ch) : PF2OOPF(ch);

	ch = validatekeys(ch);

	if(km_popped)
	  switch(ch){
	    case NO_OP_IDLE:
	    case NO_OP_COMMAND: 
	    case PF2:
	    case OPF2:
            case OOPF2:
	    case 'o' :
	    case KEY_RESIZE:
	    case ctrl('L'):
	      km_popped++;
	      break;
	    
	    default:
	      clearfooter(ps_global);
	      break;
	  }


	/*============= Execute command =======================*/
        switch(ch){

            /* ------ Help -------*/
          case PF1:
          case OPF1:
          case OOPF1:
          case OOOPF1:
          case '?':
          case ctrl('G'):
	    if(FOOTER_ROWS(ps_global) == 1 && km_popped == 0){
		km_popped = 2;
		ps_global->mangled_footer = 1;
		break;
	    }

	    whereis_pos.row = 0;
            if(ps_global->nr_mode || style == SimpleText
	       || style == ReviewMsgsText || style == ViewAbookText
	       || style == ViewAbookAtt) {
                q_status_message(SM_ORDER, 0, 5,
		    "No help text currently available");
                break;
            }
            if(ch == PF1) {
	        if(style == HelpText || style == MainHelpText)
                  goto df;
		if(style == ComposerHelpText)
                  goto unknown;
	    }
            if(style == HelpText || style == MainHelpText
	       || style == ComposerHelpText) {
                q_status_message(SM_ORDER, 0, 5, "Already in Help");
		break;
	    }

	    km_size = FOOTER_ROWS(ps_global);
            if(style == AttachText)
	      helper(h_mail_text_att_view, "HELP FOR ATTACHED TEXT VIEW", 0);
	    else
	      helper(h_mail_view, "HELP FOR MESSAGE TEXT VIEW", 0);

	    if(ps_global->next_screen != main_menu_screen
	       && km_size == FOOTER_ROWS(ps_global)) {
		/* Have to reset because helper uses scroll_text */
		num_display_lines	  = SCROLL_LINES(ps_global);
		ps_global->mangled_screen = 1;
	    }
	    else
	      done = 1;

            break; 


            /*---------- Roll keymenu ------*/
          case PF2:
          case OPF2:
          case OOPF2:
	  case 'o':
	    if(ps_global->anonymous && ch == PF2)
	      goto whereis;
	    if(km->how_many == 1)
	      goto unknown;
            if (ch == 'o')
	      warn_other_cmds();
	    what = NextTwelve;
	    ps_global->mangled_footer = 1;
	    break;
            

            /* -------- Scroll back one page -----------*/
          case PF7:
          case '-':   
          case ctrl('Y'): 
          case KEY_PGUP:
	  pageup:
	    whereis_pos.row = 0;
	    if(cur_top_line) {
		scroll_lines = min(max(num_display_lines -
			ps_global->viewer_overlap, 1), num_display_lines);
		cur_top_line -= scroll_lines;
		if(cur_top_line <= 0){
		    cur_top_line = 0;
		    q_status_message1(SM_INFO, 0, 1, "START of %s",
			STYLE_NAME(style));
		}
	    }
	    else
	      q_status_message1(SM_ORDER, 0, 1, "Already at start of %s",
				STYLE_NAME(style));
            break;


            /*---- Scroll down one page -------*/
          case PF8:
          case '+':     
          case ctrl('V'): 
          case KEY_PGDN:
          case ' ':
	  pagedown:
            if(cur_top_line + num_display_lines < get_scroll_text_lines()){
		whereis_pos.row = 0;
		scroll_lines = min(max(num_display_lines -
			ps_global->viewer_overlap, 1), num_display_lines);
		cur_top_line += scroll_lines;

		if(cur_top_line + num_display_lines >= get_scroll_text_lines())
		  q_status_message1(SM_INFO, 0, 1, "END of %s",
			STYLE_NAME(style));
            }
	    else{
		if(style == MessageText
		   && F_ON(F_ENABLE_SPACE_AS_TAB, ps_global)){
		    ch = TAB;

		    if(F_ON(F_ENABLE_TAB_DELETES, ps_global)){
			long save_msgno;

			/* Let the TAB advance cur msgno for us */
			save_msgno = mn_get_cur(ps_global->msgmap);
			cmd_delete(ps_global, ps_global->msgmap, 0);
			mn_set_cur(ps_global->msgmap, save_msgno);
		    }

		    goto df;
		}
		else
		  q_status_message1(SM_ORDER, 0, 1, "Already at end of %s",
				    STYLE_NAME(style));
	    }

            break;


            /*------ Scroll down one line -----*/
	  case KEY_DOWN:
	  case ctrl('N'):
            if(cur_top_line + num_display_lines < get_scroll_text_lines()){
		whereis_pos.row = 0;
	        cur_top_line++;
		if(cur_top_line + num_display_lines >= get_scroll_text_lines())
		  q_status_message1(SM_INFO, 0, 1, "END of %s",
				  STYLE_NAME(style));
	    }
	    else
	      q_status_message1(SM_ORDER, 0, 1, "Already at end of %s",
				  STYLE_NAME(style));

	    break;


            /* ------ Scroll back up one line -------*/
          case KEY_UP:
	  case ctrl('P'):
	    whereis_pos.row = 0;
	    if(cur_top_line){
	        cur_top_line--;
		if(cur_top_line == 0)
		  q_status_message1(SM_INFO, 0, 1, "START of %s",
				    STYLE_NAME(style));
	    }
	    else
	      q_status_message1(SM_ORDER, 0, 1, "Already at start of %s",
				STYLE_NAME(style));

	    break;
	    

            /*---------- Search text (where is) ----------*/
          case PF12:
          case 'w':
          case ctrl('W'):
	    /* PF12 is not whereis in this case */
	    if(style == MessageText && ch == PF12)
	      goto df;

          whereis:
            ps_global->mangled_footer = 1;
	    {long start_row;
	     int  start_col;

	     if(F_ON(F_SHOW_CURSOR,ps_global)){
		 if(found_on < 0
		    || found_on >= get_scroll_text_lines()
		    || found_on < cur_top_line
		    || found_on >= cur_top_line + num_display_lines){
		     start_row = cur_top_line;
		     start_col = 0;
		 }
		 else{
		     if(found_on_col < 0){
			 start_row = found_on + 1;
			 start_col = 0;
		     }
		     else{
			 start_row = found_on;
			 start_col = found_on_col+1;
		     }
		 }
	     }
	     else{
		 start_row = (found_on < 0
			      || found_on >= get_scroll_text_lines()
			      || found_on < cur_top_line
			      || found_on >= cur_top_line + num_display_lines)
				? cur_top_line : found_on + 1,
		 start_col = 0;
	     }

             found_on = search_text(-FOOTER_ROWS(ps_global), start_row,
				     start_col, tmp_20k_buf, &whereis_pos);
	    }

            if(found_on >= 0) {
		result = found_on < cur_top_line;
		if(F_ON(F_FORCE_LOW_SPEED,ps_global) ||
		   ps_global->low_speed ||
		   F_ON(F_SHOW_CURSOR,ps_global)){
		    if((found_on >= cur_top_line + num_display_lines ||
		       found_on < cur_top_line) &&
		       num_display_lines > ps_global->viewer_overlap){
			cur_top_line = found_on - ((found_on > 0) ? 1 : 0);
			if(get_scroll_text_lines()-cur_top_line < 5)
			  cur_top_line = max(0,
			      get_scroll_text_lines()-min(5,num_display_lines));
		    }
		    /* else leave cur_top_line alone */
		}
		else{
		    cur_top_line = found_on - ((found_on > 0) ? 1 : 0);
		    if(get_scroll_text_lines()-cur_top_line < 5)
		      cur_top_line = max(0,
			  get_scroll_text_lines()-min(5,num_display_lines));
		}

		whereis_pos.row = whereis_pos.row - cur_top_line + 1;
		found_on_col = whereis_pos.col;
		if(tmp_20k_buf[0])
		  q_status_message(SM_ORDER, 0, 3, tmp_20k_buf);
		else
		  q_status_message2(SM_ORDER, 0, 3,
				    "%sFound on line %s on screen",
				    result ? "Search wrapped to start. " : "",
				    int2string(whereis_pos.row));
            }
	    else if(found_on == -1)
	      q_status_message(SM_INFO, 0, 2, "Search cancelled");
            else
	      q_status_message(SM_ORDER | SM_DING, 0, 3, "Word not found");

            break; 


            /*-------------- refresh -------------*/
          case KEY_RESIZE:
          case ctrl('L'):
            num_display_lines	      = SCROLL_LINES(ps_global);
	    mark_status_dirty();
	    mark_keymenu_dirty();
	    mark_titlebar_dirty();
            ps_global->mangled_screen = 1;
	    force                     = 1;
            break;


            /*------- no op timeout to check for new mail ------*/
          case NO_OP_IDLE:
          case NO_OP_COMMAND:
            break;

	  case ctrl('M'):
	  case ctrl('J'):
	    if(style != MessageText){
		q_status_message(SM_ORDER | SM_DING, 0, 2,
				 "No default command in this screen");
		break;
	    }			/* else fall thru and handle default */


	    /*------- Other commands of error ------*/
          default:
	  df:
	    whereis_pos.row = 0;
	    if(style == SimpleText){
		/*
		 * Yet another pitiful hack.
		 */
		if(ch == 'f' || ch == PF9){ 	/* Forward */
		    forward_text(ps_global, text, source);
		}
		else if((ch == PF10 || ch == 's') && !ps_global->anonymous){
		    char filename[MAXFOLDER+1];
		    int  rv;

		    filename[0] = '\0';
		    ps_global->mangled_footer = 1;
		    while(1) {
			rv = optionally_enter(filename, -FOOTER_ROWS(ps_global),
					      0, MAXPATH, 1, 0,
					      "File to save text in : ",
					      NULL, NO_HELP, 0);
			if(rv == 4)
			  redraw_scroll_text();
			else if(rv != 3)
			  break;
		    }

		    removing_trailing_white_space(filename);
		    removing_leading_white_space(filename);
		    if(rv == 0 && filename[0]){
			gf_io_t  pc, gc;
			STORE_S *store=so_get(FileStar,filename,WRITE_ACCESS);

			if(store){
			    char *pipe_err;

			    gf_set_so_writec(&pc, store);
			    gf_set_readc(&gc, text, (source == CharStar)
							? strlen((char *)text)
							: 0L,
					 source);

			    gf_filter_init();
			    if(pipe_err = gf_pipe(gc, pc)){
				q_status_message2(SM_ORDER | SM_DING, 3, 3,
						  "Problem saving to %s: %s",
						  filename, pipe_err);
			    }
			    else
			      q_status_message1(SM_ORDER,0,2,"Text saved to %s",
					       filename);

			    so_give(&store);
			}
			else
			  q_status_message2(SM_ORDER | SM_DING,3,3,
					    "Can't save to %s: %s",
					    filename,error_description(errno));

		    }
		    else
		      q_status_message(SM_ORDER | SM_DING, 0, 2,
				       "Save Cancelled");
		}
		else if(ch == PF3 || ch == 'q'){
		    done = 1;
		}
		else{
		    q_status_message(SM_ORDER | SM_DING, 0, 2,
				     "Unknown Command");
		}
	    }
	    else if(style == AttachText && (ch == 's' || ch == PF10) && att) {
	        /*
	         * This section is an expedient hack to get this working.
	         * We'd probably like to pass a "process_cmd()" in as an
	         * argument or something like that.  Also, save_attachment()
	         * opens and reads the attachment again even though we've
	         * already opened and read it before in this case.
	         */
		save_attachment(-FOOTER_ROWS(ps_global),
				mn_m2raw(ps_global->msgmap, 
					     mn_get_cur(ps_global->msgmap)),
				att);
                ps_global->mangled_footer = 1;

	    }
	    else if(style == AttachText && (ch == '|' || ch == PF11) && att
		    && F_ON(F_ENABLE_PIPE, ps_global)) {
	        /*
	         * This section is an expedient hack to get this working.
	         * We'd probably like to pass a "process_cmd()" in as an
	         * argument or something like that.  Also, pipe_attachment()
	         * opens and reads the attachment again even though we've
	         * already opened and read it before in this case.
	         */
		pipe_attachment(mn_m2raw(ps_global->msgmap, 
					 mn_get_cur(ps_global->msgmap)),
				att);
                ps_global->mangled_footer = 1;

	    }
	    else if(style == MessageText){
	        result = process_cmd(ps_global, ps_global->msgmap, ch, 0,
				     orig_ch, &force);
	        dprint(7, (debugfile, "PROCESS_CMD return: %d\n", result));

                if(ps_global->next_screen != SCREEN_FUN_NULL || result == 1){
		    done = 1;
		}
		else if(!scroll_state(SS_CUR)){
		    num_display_lines	      = SCROLL_LINES(ps_global);
		    ps_global->mangled_screen = 1;
		}
            }
	    else {
		if(!ps_global->nr_mode
		   && ((ch == 'm' || ch == PF1)
		       && (style == HelpText || style == MainHelpText))) {
    	            /*---------- Main menu -----------*/
                    ps_global->next_screen = main_menu_screen;
                    done = 1;
		}
		else if(!ps_global->nr_mode
			&& ((ch == 'b' || ch == PF11)
			    && (style == HelpText || style == MainHelpText))) {
    	            /*---------- report a bug  -----------*/
		    gripe(ps_global);
		    num_display_lines = SCROLL_LINES(ps_global);
		}
		else if((ch == 'z' || ch == PF10) && style == MainHelpText){
		    print_all_help();
    	        }else if(!ps_global->nr_mode && (ch == PF3 || ch == 'e')) {
    	            /*----------- Done -----------*/
                    done = 1;
                }else if((ch == 'y' && !ps_global->anonymous) ||
		 (ch == PF9 && !ps_global->nr_mode) ||
		 (ch == PF11 && ps_global->nr_mode)) {
    	            /*----------- Print ------------*/
		    char message[12];
		    if(style == AttachText){
			if(att)
			  strcpy(message, "attachment ");
			else
			  strcpy(message, "text ");
		    }
		    else if(style == ViewAbookText)
		      strcpy(message, "expanded entry ");
		    else if(style == ViewAbookAtt)
		      strcpy(message, "attachment ");
		    else
		      strcpy(message, "help text ");

		    print_to_printer(text, source, message);
                }
		else {		/*----------- Unknown command -------*/
unknown:
		    bogus_command(orig_ch,
				  (style == MainHelpText || style == HelpText)
				    ? NULL
				    : F_ON(F_USE_FK,ps_global) ? "F1" : "?");
    	        }
            }
            break;

        } /* End of switch() */

    } /* End of while() -- loop executing commands */

    ps_global->redrawer	= NULL;	/* next statement makes this invalid! */
    zero_scroll_text();		/* very important to zero out on return!!! */
    scroll_state(SS_FREE);
#ifdef	_WINDOWS
    scroll_setrange(0);
#endif
}



/*----------------------------------------------------------------------
      Print text on paper

    Args:  text -- The text to print out
	   source -- What type of source text is
	   message -- Message for open_printer()
    Handling of error conditions is very poor.

  ----*/
static int
print_to_printer(text, source, message)
     void		*text;		/* the data to be printed */
     SourceType		 source;	/* char **, char * or FILE * */
     char		*message;
{
    register char **t;

    if(open_printer(message) != 0)
      return(-1);

    if(source == CharStar && text != (char *)NULL) {
        print_text((char *)text);

    } else if(source == CharStarStar && text != (char **)NULL) {
        for(t = text; *t != NULL; t++) {
            print_text(*t);
	    print_text(NEWLINE);
        }

    } else if(source == FileStar && text != (FILE *)NULL) {
	size_t n;
	int i;
	fseek((FILE *)text, 0L, 0);
	n = 20480 - 1;
	while(i=fread((void *)tmp_20k_buf, sizeof(char), n, (FILE *)text)) {
	    tmp_20k_buf[i] = '\0';
	    print_text(tmp_20k_buf);
	}
    }

    close_printer();
    return(0);
}


/*----------------------------------------------------------------------
   Search text being viewed (help or message)

      Args: q_line      -- The screen line to prompt for search string on
            start_line  -- Line number in text to begin search on
            start_col   -- Column to begin search at in first line of text
            cursor_pos  -- position of cursor is returned to caller here
			   (Actually, this isn't really the position of the
			    cursor because we don't know where we are on the
			    screen.  So row is set to the line number and col
			    is set to the right column.)

    Result: returns line number string was found on
            -1 for cancel
            -2 if not found
 ---*/
int
search_text(q_line, start_line, start_col, report, cursor_pos)
    int   q_line;
    long  start_line;
    int   start_col;
    char *report;
    Pos  *cursor_pos;
{
    char        prompt[MAX_SEARCH+50], nsearch_string[MAX_SEARCH+1];
    HelpType	help;
    int         rc;
    static char search_string[MAX_SEARCH+1] = { '\0' };
    static ESCKEY_S word_search_key[] = { { 0, 0, "", "" },
					 {ctrl('Y'), 10, "^Y", "First Line"},
					 {ctrl('V'), 11, "^V", "Last Line"},
					 {-1, 0, NULL, NULL}
					};

    report[0] = '\0';
    sprintf(prompt, "Word to search for [%s] : ", search_string);
    help = NO_HELP;
    nsearch_string[0] = '\0';

    while(1) {
        rc = optionally_enter(nsearch_string, q_line, 0, MAX_SEARCH, 1, 0,
                              prompt, word_search_key, help, 0);
        if(rc == 3) {
            help = help == NO_HELP ? h_oe_searchview : NO_HELP;
            continue;
        }
	else if(rc == 10){
	    strcpy(report, "Searched to First Line.");
	    cursor_pos->row = 0;
	    cursor_pos->col = 0;
	    return(0);
	}
	else if(rc == 11){
	    strcpy(report, "Searched to Last Line."); 
	    cursor_pos->row = max(get_scroll_text_lines() - 1, 0);
	    cursor_pos->col = 0;
	    return(cursor_pos->row);
	}

        if(rc != 4)
          break;
    }

    if(rc == 1 || (search_string[0] == '\0' && nsearch_string[0] == '\0'))
      return(-1);

    if(nsearch_string[0] != '\0')
      strcpy(search_string, nsearch_string);

    rc = search_scroll_text(start_line, start_col, search_string, cursor_pos);
    return(rc);
}



/*----------------------------------------------------------------------
  Update the scroll tool's titlebar

    Args:  title -- title to put on titlebar
	   style -- type of titlebar to draw
	   redraw -- flag to force updating

  ----*/
void
update_scroll_titlebar(title, style, cur_top_line, redraw)
    char     *title;
    TextType  style;
    long      cur_top_line;
    int	      redraw;
{
    SCROLL_S *st = scroll_state(SS_CUR);
    int  num_display_lines = SCROLL_LINES(ps_global);
    long new_line = (cur_top_line + num_display_lines > st->num_lines)
		     ? st->num_lines
		     : cur_top_line + num_display_lines;

    if(redraw){
	set_titlebar(title, ps_global->mail_stream,
		     ps_global->context_current, ps_global->cur_folder,
		     ps_global->msgmap, 1,
		     (style == HelpText || style == MainHelpText
		      || style == ComposerHelpText || style == ViewAbookText
		      || style == ViewAbookAtt)
		       ? TextPercent
		       : (style == SimpleText) 
			   ? FileTextPercent 
			   : MsgTextPercent,
		     new_line, st->num_lines);

	ps_global->mangled_header = 0;
    }
    else if(style == SimpleText || style == HelpText
	    || style == MainHelpText || style == ComposerHelpText
	    || style == ViewAbookText || style == ViewAbookAtt)
      update_titlebar_lpercent(new_line);
    else
      update_titlebar_percent(new_line);
}



/*----------------------------------------------------------------------
  manager of global (to this module, anyway) scroll state structures


  ----*/
SCROLL_S *
scroll_state(func)
    int func;
{
    struct scrollstack {
	SCROLL_S s;
	struct scrollstack *prev;
    } *s;
    static struct scrollstack *stack = NULL;

    switch(func){
      case SS_CUR:			/* no op */
	break;
      case SS_NEW:
	s = (struct scrollstack *)fs_get(sizeof(struct scrollstack));
	memset((void *)s, 0, sizeof(struct scrollstack));
	s->prev = stack;
	stack  = s;
	break;
      case SS_FREE:
	if(stack){
	    s = stack->prev;
	    fs_give((void **)&stack);
	    stack = s;
	}
	break;
      default:				/* BUG: should complain */
	break;
    }

    return(stack ? &stack->s : NULL);
}



/*----------------------------------------------------------------------
      Save all the data for scrolling text and paint the screen


  ----*/
void
set_scroll_text(st, text, current_line, source, style)
    SCROLL_S	*st;
    void	*text;
    long	 current_line;
    SourceType	 source;
    TextType	 style;
{
    /* save all the stuff for possible asynchronous redraws */
    st->text               = text;
    st->top_text_line      = current_line;
    st->screen_start_line  = SCROLL_LINES_ABOVE(ps_global);
    st->screen_other_lines = SCROLL_LINES_ABOVE(ps_global)
				+ SCROLL_LINES_BELOW(ps_global);
    st->source             = source;
    st->style		   = style;
    st->screen_width	   = -1;	/* Force text formatting calculation */
}



/*----------------------------------------------------------------------
     Redraw the text on the screen, possibly reformatting if necessary

   Args None

 ----*/
void
redraw_scroll_text()
{
    int	      i, len, offset;
    SCROLL_S *st = scroll_state(SS_CUR);

    format_scroll_text();

    offset = (st->source == FileStar) ? 0 : st->top_text_line;

#ifdef _WINDOWS
    mswin_beginupdate();
#endif
    /*---- Actually display the text on the screen ------*/
    for(i = 0; i < PGSIZE(st); i++){
	ClearLine(i + st->screen_start_line);
        if((offset + i) < st->num_lines) {
            len = min(st->line_lengths[offset + i], st->screen_width);
            PutLine0n8b(i + st->screen_start_line, 0,
                      st->text_lines[offset + i], len);
        }
    }

#ifdef _WINDOWS
    mswin_endupdate();
#endif
    fflush(stdout);
}




/*----------------------------------------------------------------------
  Free memory used as scrolling buffers for text on disk.  Also mark
  text_lines as available
  ----*/
void
zero_scroll_text()
{
    SCROLL_S	 *st = scroll_state(SS_CUR);
    register int  i;

    for(i = 0; i < st->lines_allocated; i++)
      if(st->source == FileStar && st->text_lines[i])
	fs_give((void **)&st->text_lines[i]);
      else
	st->text_lines[i] = NULL;

    if(st->source == FileStar && st->findex != NULL){
	fclose(st->findex);
	st->findex = NULL;
	if(st->fname){
	    unlink(st->fname);
	    fs_give((void **)&st->fname);
	}
    }

    if(st->text_lines)
      fs_give((void **)&st->text_lines);

    if(st->line_lengths)
      fs_give((void **)&st->line_lengths);
}



/*----------------------------------------------------------------------

Always format at least 20 chars wide. Wrapping lines would be crazy for
screen widths of 1-20 characters 
  ----*/
void
format_scroll_text()
{
    int             i, line_len;
    char           *p, **pp;
#ifdef	X_NEW
    char           *max_line;
#endif
    SCROLL_S	    *st = scroll_state(SS_CUR);
    register short  *ll;
    register char  **tl, **tl_end, *last_space;

    if(!st || (st->screen_width == (i = ps_global->ttyo->screen_cols)
	       && st->screen_length == PGSIZE(st)))
        return;

    st->screen_width = max(20, i);
    st->screen_length = PGSIZE(st);

    if(st->lines_allocated == 0) {
        st->lines_allocated = TYPICAL_BIG_MESSAGE_LINES;
        st->text_lines = (char **)fs_get(st->lines_allocated *sizeof(char *));
	memset(st->text_lines, 0, st->lines_allocated * sizeof(char *));
        st->line_lengths = (short *)fs_get(st->lines_allocated *sizeof(short));
    }

    tl     = st->text_lines;
    ll     = st->line_lengths;
    tl_end = &st->text_lines[st->lines_allocated];

    if(st->source == CharStarStar) {
        /*---- original text is already list of lines -----*/
        /*   The text could be wrapped nicely for narrow screens; for now
             it will get truncated as it is displayed */
        for(pp = (char **)st->text; *pp != NULL;) {
            *tl++ = *pp++;
            *ll++ = st->screen_width;
            if(tl >= tl_end) {
		i = tl - st->text_lines;
                st->lines_allocated *= 2;
                fs_resize((void **)&st->text_lines,
                          st->lines_allocated * sizeof(char *));
                fs_resize((void **)&st->line_lengths,
                          st->lines_allocated*sizeof(short));
                tl     = &st->text_lines[i];
                ll     = &st->line_lengths[i];
                tl_end = &st->text_lines[st->lines_allocated];
            }
        }

	st->num_lines = tl - st->text_lines;
    } else if (st->source == CharStar) {
        /*------ Format the plain text ------*/
        for(p = (char *)st->text; *p; ) {
            *tl = p;
            line_len = 0;            
            last_space = NULL;

            while(*p && !(*p == RETURN || *p == LINE_FEED)){
		if(*p == ESCAPE && (i = match_escapes(p+1))) {
		    while(i--)			/* Don't count escape in len */
		      p++;
                } else if(*p == TAB) {
		    while(line_len < st->screen_width
			  && ((++line_len)&0x07) != 0) /* add tab's spaces */
		      ;

                    p++;
                    last_space = p;
                } else if(*p == TAG_EMBED){	/* this char and next are */
                    p += 2;			/* embedded data (not shown) */
                } else if(*p == ' ') {
                    last_space = ++p;
                    line_len++;
                } else {
                    p++;
                    line_len++;
                }

                if(line_len >= st->screen_width){
		    if(last_space)
		      p = last_space;

		    break;
                }
            }
        
            *ll = p - *tl;
            ll++; tl++;
            if(tl >= tl_end) {
		i = tl - st->text_lines;
                st->lines_allocated *= 2;
                fs_resize((void **)&st->text_lines,
                          st->lines_allocated * sizeof(char *));
                fs_resize((void **)&st->line_lengths,
                         st->lines_allocated*sizeof(short));
                tl     = &st->text_lines[i];
                ll     = &st->line_lengths[i];
                tl_end = &st->text_lines[st->lines_allocated];
            }      
            if(*p == '\r' && *(p+1) == '\n') 
              p += 2;
            else if(*p == '\n' || *p == '\r')
              p++;
        }

	st->num_lines = tl - st->text_lines;
    }
    else {
	/*------ Display text is in a file --------*/

	/*
	 * This is pretty much only useful under DOS where we can't fit
	 * all of big messages in core at once.  This scheme makes
	 * some simplifying assumptions:
	 *  1. Lines are on disk just the way we'll display them.  That
	 *     is, line breaks and such are left to the function that
	 *     writes the disk file to catch and fix.
	 *  2. We get away with this mainly because the DOS display isn't
	 *     going to be resized out from under us.
	 *
	 * The idea is to use the already alloc'd array of char * as a 
	 * buffer for sections of what's on disk.  We'll set up the first
	 * few lines here, and read new ones in as needed in 
	 * scroll_scroll_text().
	 *  
	 * but first, make sure there are enough buffer lines allocated
	 * to serve as a place to hold lines from the file.
	 *
	 *   Actually, this is also used under windows so the display will
	 *   be resized out from under us.  So I changed the following
	 *   to always
	 *	1.  free old text_lines, which may have been allocated
	 *	    for a narrow screen.
	 *	2.  insure we have enough text_lines
	 *	3.  reallocate all text_lines that are needed.
	 *   (tom unger  10/26/94)
	 */

	/* free old text lines, which may be too short. */
	for(i = 0; i < st->lines_allocated; i++)
	  if(st->text_lines[i]) 		/* clear alloc'd lines */
	    fs_give((void **)&st->text_lines[i]);

        /* Insure we have enough text lines. */
	if(st->lines_allocated < (2 * PGSIZE(st)) + 1){
	    st->lines_allocated = (2 * PGSIZE(st)) + 1; /* resize */

	    fs_resize((void **)&st->text_lines,
		      st->lines_allocated * sizeof(char *));
	    memset(st->text_lines, 0, st->lines_allocated * sizeof(char *));
	    fs_resize((void **)&st->line_lengths,
		      st->lines_allocated*sizeof(short));
	}

	/* reallocate all text lines that are needed. */
	for(i = 0; i <= PGSIZE(st); i++)
	  if(st->text_lines[i] == NULL)
	    st->text_lines[i] = (char *)fs_get(st->screen_width*sizeof(char));

	tl = &st->text_lines[i];

	st->num_lines = make_file_index();

	ScrollFile(st->top_text_line);		/* then load them up */

    }

#ifdef	_WINDOWS
    scroll_setrange (st->num_lines);
#endif

    *tl = NULL;
}




/*
 * ScrollFile - scroll text into the st struct file making sure 'line'
 *              of the file is the one first in the text_lines buffer.
 *
 *   NOTE: talk about massive potential for tuning...
 *         Goes without saying this is still under constuction
 */
void
ScrollFile(line)
    long line;
{
    SCROLL_S	 *st = scroll_state(SS_CUR);
    register int  i;
             long x;

    if(line <= 0){		/* reset and load first couple of pages */
	fseek((FILE *) st->text, 0L, 0);
    }
    else{
	/*** do stuff to get the file pointer into the right place ***/
	/*
	 * BOGUS: this is painfully crude right now, but I just want to get
	 * it going. 
	 *
	 * possibly in the near furture, an array of indexes into the 
	 * file that are the offset for the beginning of each line will
	 * speed things up.  Of course, this
	 * will have limits, so maybe a disk file that is an array
	 * of indexes is the answer.
	 */
	fseek(st->findex, (size_t)(line) * sizeof(long), 0);
	if(fread(&x, sizeof(long), (size_t)1, st->findex) != 1){
	    return;
	}

	fseek((FILE *) st->text, x, 0);
    }

    for(i = 0; i < PGSIZE(st); i++){
	if(!st->text_lines || !st->text_lines[i]
	   || fgets(st->text_lines[i],st->screen_width,
		    (FILE *)st->text) == NULL)
	  break;

	st->line_lengths[i] = strlen(st->text_lines[i]);
    }

    for(; i < PGSIZE(st); i++)
      if(st->text_lines && st->text_lines[i]){/* blank out any unused lines */
	  *st->text_lines[i]  = '\0';
	  st->line_lengths[i] = 0;
      }
}


/*
 * make_file_index - do a single pass over the file containing the text
 *                   to display, recording line lengths and offsets.
 *    NOTE: This is never really to be used on a real OS with virtual
 *          memory.  This is the whole reason st->findex exists.  Don't
 *          want to waste precious memory on a stupid array that could 
 *          be very large.
 */
long
make_file_index()
{
    SCROLL_S	  *st = scroll_state(SS_CUR);
    register long  l = 0;
    long	   i = 0;

    if(!st->findex){
	if(!st->fname)
	  st->fname = temp_nam(NULL, "pi");

	if((st->findex = fopen(st->fname,"w+b")) == NULL)
	  return(0);
    }
    else
      fseek(st->findex, 0L, 0);

    fseek((FILE *)st->text, 0L, 0);

    fwrite((void *)&i, sizeof(long), (size_t)1, st->findex);
    while(fgets(tmp_20k_buf, st->screen_width, (FILE *)st->text) != NULL){
	i = ftell((FILE *)st->text);
	fwrite((void *)&i, sizeof(long), (size_t)1, st->findex);
	l++;
    }

    fseek((FILE *)st->text, 0L, 0);

    return(l);
}



/*----------------------------------------------------------------------
     Scroll the text on the screen

   Args:  new_top_line -- The line to be displayed on top of the screen
          redraw -- Flag to force a redraw even in nothing changed 

   Returns: resulting top line
   Note: the returned line number may be less than new_top_line if
	 reformatting caused the total line count to change.

 ----*/
long
scroll_scroll_text(new_top_line, redraw)
    long new_top_line;
    int	 redraw;
{
    SCROLL_S *st = scroll_state(SS_CUR);
    int	      num_display_lines, len, l;

    num_display_lines = PGSIZE(st);

    if(st->top_text_line == new_top_line && !redraw)
      return(new_top_line);

    format_scroll_text();

    if(st->top_text_line >= st->num_lines)	/* don't pop line count */
      new_top_line = st->top_text_line = max(st->num_lines - 1, 0);

    if(st->source == FileStar)
      ScrollFile(new_top_line);		/* set up new st->text_lines */

#ifdef	_WINDOWS
    scroll_setrange (st->num_lines);
    scroll_setpos (new_top_line);
#endif

    /* --- 
       Check out the scrolling situation. If we want to scroll, but BeginScroll
       says we can't then repaint,  + 10 is so we repaint most of the time.
      ----*/
    if(redraw ||
       (st->top_text_line - new_top_line + 10 >= num_display_lines ||
        new_top_line - st->top_text_line + 10 >= num_display_lines) ||
	BeginScroll(st->screen_start_line,
                    st->screen_start_line + num_display_lines - 1) != 0) {
        /* Too much text to scroll, or can't scroll -- just repaint */
        st->top_text_line = new_top_line;
        redraw_scroll_text();
    }
    else{
	if(new_top_line > st->top_text_line){
	    /*------ scroll down ------*/
	    while(new_top_line > st->top_text_line) {
		ScrollRegion(1);
		if(st->source == FileStar)
		  l = num_display_lines - (new_top_line - st->top_text_line);
		else
		  l = st->top_text_line + num_display_lines;
		if(l < st->num_lines) {
		    len = min(st->line_lengths[l], st->screen_width);
		    PutLine0n8b(st->screen_start_line + num_display_lines - 1,
				0, st->text_lines[l], len);
		}
		st->top_text_line++;
	    }
	}
	else{
	    /*------ scroll up -----*/
	    while(new_top_line < st->top_text_line) {
		ScrollRegion(-1);
		st->top_text_line--;
		if(st->source == FileStar)
		  l = st->top_text_line - new_top_line;
		else
		  l = st->top_text_line;
		len = min(st->line_lengths[l], st->screen_width);
		PutLine0n8b(st->screen_start_line, 0, st->text_lines[l], len);
	    }
	}

	EndScroll();
	fflush(stdout);
    }

    return(new_top_line);
}



/*----------------------------------------------------------------------
      Search the set scrolling text

   Args:   start_line -- line to start searching on
	   start_col  -- column to start searching at in first line
           word       -- string to search for
           cursor_pos -- position of cursor is returned to caller here
			 (Actually, this isn't really the position of the
			  cursor because we don't know where we are on the
			  screen.  So row is set to the line number and col
			  is set to the right column.)

   Returns: the line the word was found on or -2 if it wasn't found

 ----*/
int
search_scroll_text(start_line, start_col, word, cursor_pos)
    long  start_line;
    int   start_col;
    char *word;
    Pos  *cursor_pos;
{
    SCROLL_S *st = scroll_state(SS_CUR);
    char      tmp[MAX_SCREEN_COLS+1], *wh, *p;
    long      l, offset, dlines;

    dlines = PGSIZE(st);
    offset = (st->source == FileStar) ? st->top_text_line : 0;

    if(start_line < st->num_lines){
	/* search first line starting at position start_col in */
	strncpy(tmp, st->text_lines[start_line - offset],
		min(st->line_lengths[start_line - offset], MAX_SCREEN_COLS));
	tmp[min(st->line_lengths[start_line - offset],
		MAX_SCREEN_COLS) + 1] = '\0';
	p = tmp + start_col;
	if((wh=srchstr(p, word)) != NULL){
	    cursor_pos->row = start_line;
	    cursor_pos->col = wh - tmp;
	    return(start_line);
	}

	if(st->source == FileStar)
	  offset++;

	for(l = start_line+1; l < st->num_lines; l++) {
	    if(st->source == FileStar && l > offset + dlines)
	      ScrollFile(offset += dlines);

	    strncpy(tmp, st->text_lines[l-offset], 
		    min(st->line_lengths[l-offset], MAX_SCREEN_COLS));
	    tmp[min(st->line_lengths[l-offset], MAX_SCREEN_COLS) + 1] = '\0';
	    if((wh=srchstr(tmp, word)) != NULL)
	      break;
	}

	if(l < st->num_lines) {
	    cursor_pos->row = l;
	    cursor_pos->col = wh - tmp;
	    return(l);
	}
    }
    else
      start_line = st->num_lines;

    if(st->source == FileStar)		/* wrap offset */
      ScrollFile(offset = 0);

    for(l = 0; l < start_line; l++) {
	if(st->source == FileStar && l > offset + dlines)
	  ScrollFile(offset += dlines);

	strncpy(tmp, st->text_lines[l-offset], 
		min(st->line_lengths[l-offset], MAX_SCREEN_COLS));
	tmp[min(st->line_lengths[l-offset], MAX_SCREEN_COLS) + 1] = '\0';
        if((wh=srchstr(tmp, word)) != NULL)
          break;
    }

    if(l == start_line)
      return(-2);
    else{
	cursor_pos->row = l;
	cursor_pos->col = wh - tmp;
        return(l);
    }
}
     

char *    
display_parameters(parameter_list)
     PARAMETER *parameter_list;
{
    int        longest_attribute;
    PARAMETER *p;
    char      *d;

    if(parameter_list == NULL) {
        tmp_20k_buf[0] = '\0';

    } else {
        longest_attribute = 0;
    
        for(p = parameter_list; p != NULL; p = p->next)
          longest_attribute = max(longest_attribute, (p->attribute == NULL ?
                                                      0 :
                                                      strlen(p->attribute)));
    
        longest_attribute = min(longest_attribute, 11);
    
        d = tmp_20k_buf;
        for(p = parameter_list; p != NULL; p = p->next) {
            sprintf(d, "%-*s: %s\n", longest_attribute,
                    p->attribute != NULL ? p->attribute : "",
                    p->value     != NULL ? p->value     : "");
            d += strlen(d);
        }
    }
    return(tmp_20k_buf);
}



/*----------------------------------------------------------------------
    Display the contents of the given file (likely output from some command)

  Args: filename -- name of file containing output
	title -- title to be used for screen displaying output
	alt_msg -- if no output, Q this message instead of the default
  Returns: none
 ----*/
void
display_output_file(filename, title, alt_msg)
    char *filename, *title, *alt_msg;
{
    int   msg_q = 0, i = 0;
    char  buf[512], *msg_p[4];
    FILE *f;
#define	MAX_SINGLE_MSG_LEN	60

    if(f = fopen(filename, "r")){
	buf[0]   = '\0';
	msg_p[0] = buf;
	while(fgets(msg_p[msg_q], sizeof(buf) - (msg_p[msg_q] - buf), f) 
	      && msg_q < 3 && (i = strlen(msg_p[msg_q])) < MAX_SINGLE_MSG_LEN){
	    msg_p[msg_q+1] = msg_p[msg_q]+strlen(msg_p[msg_q]);
	    if (*(msg_p[++msg_q] - 1) == '\n')
	      *(msg_p[msg_q] - 1) = '\0';
	}

	if(msg_q < 3 && i < MAX_SINGLE_MSG_LEN){
	    if(*msg_p[0])
	      for(i = 0; i < msg_q; i++)
		q_status_message2(SM_ORDER, 3, 4,
		    "%s Result: %s", title, msg_p[i]);
	    else
	      q_status_message2(SM_ORDER, 0, 4, "%s%s", title,
		alt_msg ? alt_msg : " command completed");
	}
	else{
	    scrolltool(f, title, AttachText, FileStar, NULL);
	    ps_global->mangled_screen = 1;
	}

	fclose(f);
	unlink(filename);
    }
    else
      dprint(2, (debugfile, "Error reopening %s to get results: %s\n",
		 filename, error_description(errno)));
}



/*----------------------------------------------------------------------
      Fetch the requested header fields from the msgno specified

   Args: stream -- mail stream of open folder
         msgno -- number of message to get header lines from
         fields -- array of pointers to desired fields

   Returns: allocated string containing matched header lines,
	    NULL on error.
 ----*/
char *
xmail_fetchheader_lines(stream, msgno, fields)
     MAILSTREAM  *stream;
     long         msgno;
     char       **fields;
{
    int   i, old_prefetch;
    char *p, *m, *h = NULL, *match = NULL, tmp[MAILTMPLEN];
    extern int find_field();

    old_prefetch = (int) mail_parameters(stream, GET_PREFETCH, NULL);
    (void) mail_parameters(stream, SET_PREFETCH, NULL);
    h		 = mail_fetchheader(stream, msgno);
    mail_parameters(stream, SET_PREFETCH, (void *) old_prefetch);

    if(!h)
      return(NULL);				/* fetchheader return error? */

    while(find_field(&h, tmp)){
	for(i = 0; fields[i] && strucmp(tmp, fields[i]); i++)
	  ;

	if(p = fields[i]){			/* interesting field */
	    /*
	     * Hold off allocating space for matching fields until
	     * we at least find one to copy...
	     */
	    if(!match)
	      match = m = fs_get(strlen(h) + strlen(p) + 1);

	    while(*p)				/* copy field name */
	      *m++ = *p++;

	    while(*h && (*m++ = *h++))		/* header includes colon */
	      if(*(m-1) == '\n' && (*h == '\r' || !isspace((unsigned char)*h)))
		break;

	    *m = '\0';				/* tie off match string */
	}
	else{					/* no match, pass this field */
	    while(*h && !(*h++ == '\n'
	          && (*h == '\r' || !isspace((unsigned char)*h))))
	      ;
	}
    }

    return(match ? match : cpystr(""));
}


/*----------------------------------------------------------------------
   Fetch everything except the requested header fields from given msgno

   Args: stream -- mail stream of open folder
         msgno -- number of message to get header lines from
         fields -- array of pointers to UNdesired fields

   Returns: allocated string containing NON-matching header lines,
	    NULL on error.
 ----*/
char *
xmail_fetchheader_lines_not(stream, msgno, fields)
     MAILSTREAM  *stream;
     long         msgno;
     char       **fields;
{
    int   i, old_prefetch;
    char *p, *m, *h = NULL, *match = NULL, tmp[MAILTMPLEN];
    extern int find_field();

    old_prefetch = (int) mail_parameters(stream, GET_PREFETCH, NULL);
    (void) mail_parameters(stream, SET_PREFETCH, NULL);
    h		 = mail_fetchheader(stream, msgno);
    mail_parameters(stream, SET_PREFETCH, (void *) old_prefetch);

    if(!h)
      return(NULL);				/* fetchheader return error? */

    while(find_field(&h, tmp)){
	for(i = 0; fields[i] && strucmp(tmp, fields[i]); i++)
	  ;

	if(!fields[i]){				/* interesting field */
	    /*
	     * Hold off allocating space for matching fields until
	     * we at least find one to copy...
	     */
	    if(!match)
	      match = m = fs_get(strlen(h) + strlen(tmp) + 1);

	    sstrcpy(&m, tmp);			/* copy field name */
	    while(*h && (*m++ = *h++))		/* header includes colon */
	      if(*(m-1) == '\n' && (*h == '\r' || !isspace((unsigned char)*h)))
		break;

	    *m = '\0';				/* tie off match string */
	}
	else{					/* no match, pass this field */
	    while(*h && !(*h++ == '\n'
	          && (*h == '\r' || !isspace((unsigned char)*h))))
	      ;
	}
    }

    return(match ? match : cpystr(""));
}


int
find_field(h, tmp)
     char **h;
     char *tmp;
{
    if(!h || !*h || !**h || isspace((unsigned char)**h))
      return(0);

    while(**h && **h != ':' && !isspace((unsigned char)**h))
      *tmp++ = *(*h)++;

    *tmp = '\0';
    return(1);
}


#ifdef	_WINDOWS
/*----------------------------------------------------------------------
    Return characters in scroll tool buffer serially

   Args: n -- index of char to return

   Returns: returns the character at index 'n', or -1 on error or
	    end of buffer.

 ----*/
int
mswin_readscrollbuf(n)
    int n;
{
    SCROLL_S *st = scroll_state(SS_CUR);
    int       c;
    static char **orig = NULL, **l, *p;
    static int    lastn;

    if(!st)
      return(-1);

    /*
     * All of these are mind-numbingly slow at the moment...
     */
    switch(st->source){
      case CharStar :
	return((n >= strlen((char *)st->text)) ? -1 : ((char *)st->text)[n]);

      case CharStarStar :
	/* BUG? is this test rigorous enough? */
	if(orig != (char **)st->text || n < lastn){
	    lastn = n;
	    if(orig = l = (char **)st->text)	/* reset l and p */
	      p = *l;
	}
	else{				/* use cached l and p */
	    c = n;			/* and adjust n */
	    n -= lastn;
	    lastn = c;
	}

	while(l){			/* look for 'n' on each line  */
	    for(; n && *p; n--, p++)
	      ;

	    if(n--)			/* 'n' found ? */
	      p = *++l;
	    else
	      break;
	}

	return((l && *l) ? *p ? *p : '\n' : -1);

      case FileStar :
	return((fseek((FILE *)st->text, (long) n, 0) < 0
		|| (c = fgetc((FILE *)st->text)) == EOF) ? -1 : c);

      default:
	return(-1);
    }
}



/*----------------------------------------------------------------------
     MSWin scroll callback.  Called during scroll message processing.
	     


  Args: cmd - what type of scroll operation.
	scroll_pos - paramter for operation.  
			used as position for SCROLL_TO operation.

  Returns: TRUE - did the scroll operation.
	   FALSE - was not able to do the scroll operation.
 ----*/
int
scroll_scroll_callback (cmd, scroll_pos)
int	cmd;
long	scroll_pos;
{
    SCROLL_S   *st = scroll_state(SS_CUR);
    int		paint = FALSE;
    int		num_display_lines;
    int		scroll_lines;
    int		num_text_lines;
    char	*message, *msgSTART, *msgEND;
    long	maxscroll;
    
	
    msgSTART = "START of message text";
    msgEND = "END of message text";
    message = NULL;
    maxscroll = st->num_lines;
    switch (cmd) {
    case MSWIN_KEY_SCROLLUPLINE:
	if(st->top_text_line > 0) {
	    st->top_text_line--;
	    paint = TRUE;
	    if (st->top_text_line <= 0)
	      message = msgSTART;
        }
	break;

    case MSWIN_KEY_SCROLLDOWNLINE:
        if(st->top_text_line < maxscroll) {
	    st->top_text_line++;
	    paint = TRUE;
	    if (st->top_text_line >= maxscroll) 
	      message = msgEND;
        }
	break;
		
    case MSWIN_KEY_SCROLLUPPAGE:
	if(st->top_text_line > 0) {
	    num_display_lines = SCROLL_LINES(ps_global);
	    scroll_lines = min(max(num_display_lines -
			ps_global->viewer_overlap, 1), num_display_lines);
	    if (st->top_text_line > scroll_lines)
		st->top_text_line -= scroll_lines;
	    else {
		st->top_text_line = 0;
		message = msgSTART;
	    }
	    paint = TRUE;
        }
	break;
	    
    case MSWIN_KEY_SCROLLDOWNPAGE:
	num_display_lines = SCROLL_LINES(ps_global);
	if(st->top_text_line  < maxscroll) {
	    scroll_lines = min(max(num_display_lines -
			ps_global->viewer_overlap, 1), num_display_lines);
	    st->top_text_line += scroll_lines;
	    if (st->top_text_line >= maxscroll) {
		st->top_text_line = maxscroll;
		message = msgEND;
	    }
	    paint = TRUE;
	}
	break;
	    
    case MSWIN_KEY_SCROLLTO:
	if (st->top_text_line != scroll_pos) {
	    st->top_text_line = scroll_pos;
	    if (st->top_text_line == 0)
		message = msgSTART;
	    else if(st->top_text_line >= maxscroll) 
		message = msgEND;
	    paint = TRUE;
        }
	break;
    }
    
    if (paint) {
	mswin_beginupdate();
	update_scroll_titlebar(NULL, st->style, st->top_text_line, 0);
	(void) scroll_scroll_text(st->top_text_line, 1);
	if (message != NULL)
	  q_status_message(SM_INFO, 0, 1, message);

	/* Display is always called so that the "START(END) of message" 
	 * message gets removed when no longer at the start(end). */
	display_message (KEY_PGDN);
	mswin_endupdate();
    }

    return (TRUE);
}
#endif
