/*----------------------------------------------------------------------
  $Id$

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
   1989-1997 by the University of Washington.

   The full text of our legal notices is contained in the file called
   CPYRIGHT, included with this distribution.


   USENET News reading additions in part by L Lundblade / NorthWestNet, 1993
   lgl@nwnet.net

   Pine is in part based on The Elm Mail System:
    ***********************************************************************
    *  The Elm Mail System  -  Revision: 2.13                             *
    *                                                                     *
    * 			Copyright (c) 1986, 1987 Dave Taylor              *
    * 			Copyright (c) 1988, 1989 USENET Community Trust   *
    ***********************************************************************
 

  ----------------------------------------------------------------------*/

/*======================================================================

     pine.h 

     Definitions here are fundamental to pine. 

    No changes should need to be made here to configure pine for one
  site or another.  That is, no changes for local preferences such as
  default directories and other parameters.  Changes might be needed here
  for porting pine to a new machine, but we hope not.

   Includes
     - Various convenience definitions and macros
     - macros for debug printfs
     - data structures used by Pine
     - declarations of all Pine functions

  ====*/


#ifndef _PINE_INCLUDED
#define _PINE_INCLUDED

#define PINE_VERSION		"3.96"
#define	PHONE_HOME_VERSION	"396"
#define	PHONE_HOME_HOST		"docserver.cac.washington.edu"
#define	UPDATE_FOLDER		"{pine.cac.washington.edu:144/anonymous}#news."


#define BREAK		'\0'  		/* default interrupt    */
#define BACKSPACE	'\b'     	/* backspace character  */
#define TAB		'\t'            /* tab character        */
#define RETURN		'\r'     	/* carriage return char */
#define LINE_FEED	'\n'     	/* line feed character  */
#define FORMFEED	'\f'     	/* form feed (^L) char  */
#define COMMA		','		/* comma character      */
#define SPACE		' '		/* space character      */
#define DOT		'.'		/* period/dot character */
#define BANG		'!'		/* exclaimation mark!   */
#define AT_SIGN		'@'		/* at-sign character    */
#define PERCENT		'%'		/* percent sign char.   */
#define COLON		':'		/* the colon ..		*/
#define BACKQUOTE	'`'		/* backquote character  */
#define TILDE_ESCAPE	'~'		/* escape character~    */
#define ESCAPE		'\033'		/* the escape		*/
#define	BELL		'\007'		/* the bell		*/
#define LPAREN		'('
#define RPAREN		')'
#define BSLASH		'\\'
#define QUOTE		'"'

#if defined(DOS) || defined(OS2)
#define	NEWLINE		"\r\n"		/* local EOL convention...  */
#else
#define	NEWLINE		"\n"		/* necessary for gf_* funcs */
#endif

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif


#define EXECUTE_ACCESS	01		/* These five are 	   */
#define WRITE_ACCESS	02		/*    for the calls	   */
#define READ_ACCESS	04		/*       to access()       */
#define ACCESS_EXISTS	00		/*           <etc>         */
#define EDIT_ACCESS	06		/*  (this is r+w access)   */

#define	FM_DO_PRINT	0x01		/* flag for format_message */
#define	FM_NEW_MESS	0x02		/* ditto		   */
#define	FM_DO_PGP	0x04		/* ditto		   */

#define	RB_NORM		0x00		/* flags modifying radio_buttons */
#define	RB_ONE_TRY	0x01		/* one shot answer, else default */
#define	RB_FLUSH_IN	0x02		/* discard pending input	 */
#define	RB_NO_NEWMAIL	0x04		/* Quell new mail check		 */

#define	GF_NOOP		0x01		/* flags used by generalized */
#define GF_EOD		0x02		/* filters                   */
#define GF_DATA		0x04		/* See filter.c for more     */
#define GF_ERROR	0x08		/* details                   */
#define GF_RESET	0x10


/*
 * Size of generic filter's input/output queue
 */
#define	GF_MAXBUF		256


#define KEY_HELP_LEN   (2 * MAX_SCREEN_COLS)
                          /*When saving escape sequences, etc length of string
                                for the usual key menu at the bottom. used with
                                begin_output_store(), end_output_store() */


#undef  min
#define min(a,b)	((a) < (b) ? (a) : (b))
#undef  max
#define max(a,b)	((a) > (b) ? (a) : (b))
#define ctrl(c)	        ((c) & 0x1f)	/* control character mapping */
#define plural(n)	((n) == 1 ? "" : "s")

/*
 * Macro to simplify clearing body portion of pine's display
 */
#define ClearBody()	ClearLines(1, ps_global->ttyo->screen_rows 	      \
						  - FOOTER_ROWS(ps_global) - 1)

/*
 * Macro to reveal horizontal scroll margin.  It can be no greater than
 * half the number of lines on the display...
 */
#define	HS_MAX_MARGIN(p)	(((p)->ttyo->screen_rows-FOOTER_ROWS(p)-3)/2)
#define	HS_MARGIN(p)		min((p)->scroll_margin, HS_MAX_MARGIN(p))


/*
 * Macros to support anything you'd ever want to do with a message
 * number...
 */
#define	mn_init(P, m)		msgno_init((P), (m))

#define	mn_get_cur(p)		(((p) && (p)->select) 			      \
				  ? (p)->select[(p)->sel_cur] : -1)

#define	mn_set_cur(p, m)	{					      \
				  if(p){				      \
				    (p)->select[(p)->sel_cur] = (m);	      \
				  }					      \
				}

#define	mn_inc_cur(s, p)	msgno_inc(s, p)

#define	mn_dec_cur(s, p)	msgno_dec(s, p)

#define	mn_add_cur(p, m)	{					      \
				  if(p){				      \
				      if((p)->sel_cnt+1L > (p)->sel_size){    \
					  (p)->sel_size += 10L;		      \
					  fs_resize((void **)&((p)->select),  \
						    (size_t)(p)->sel_size     \
						             * sizeof(long)); \
				      }					      \
				      (p)->select[((p)->sel_cnt)++] = (m);    \
				  }					      \
				}

#define	mn_total_cur(p)		((p) ? (p)->sel_cnt : 0L)

#define	mn_first_cur(p)		(((p) && (p)->sel_cnt > 0L)		      \
				  ? (p)->select[(p)->sel_cur = 0] : 0L)

#define	mn_next_cur(p)		(((p) && ((p)->sel_cur + 1) < (p)->sel_cnt)   \
				  ? (p)->select[++((p)->sel_cur)] : -1L)

#define	mn_is_cur(p, m)		msgno_in_select((p), (m))

#define	mn_reset_cur(p, m)	{					      \
				  if(p){				      \
				      (p)->sel_cur  = 0L;		      \
				      (p)->sel_cnt  = 1L;		      \
				      (p)->sel_size = 8L;		      \
				      fs_resize((void **)&((p)->select),      \
					(size_t)(p)->sel_size * sizeof(long));\
				      (p)->select[0] = (m);		      \
				  }					      \
			        }

#define	mn_m2raw(p, m)		(((p) && (p)->sort && (m) > 0 		      \
				  && (m) <= mn_get_total(p)) 		      \
				   ? (p)->sort[m] : 0L)

#define	mn_raw2m(p, m)		msgno_in_sort((p), (m))

#define	mn_get_total(p)		((p) ? (p)->max_msgno : 0L)

#define	mn_set_total(p, m)	{					      \
				  if(p)					      \
				    (p)->max_msgno = (m);		      \
			        }

#define	mn_add_raw(p, m)	msgno_add_raw((p), (m))

#define	mn_flush_raw(p, m)	msgno_flush_raw((p), (m))

#define	mn_get_sort(p)		((p) ? (p)->sort_order : SortArrival)

#define	mn_set_sort(p, t)	{					      \
				  if(p)					      \
				    (p)->sort_order = (t);		      \
				}

#define	mn_get_revsort(p)	((p) ? (p)->reverse_sort : 0)

#define	mn_set_revsort(p, t)	{					      \
				  if(p)					      \
				    (p)->reverse_sort = (t);		      \
				}

#define	mn_give(P)		{					      \
				  if(P){				      \
				      if((*(P))->sort)			      \
					fs_give((void **)&((*(P))->sort));    \
				      if((*(P))->select)		      \
					fs_give((void **)&((*(P))->select));  \
				      fs_give((void **)(P));		      \
				  }					      \
				}

/*
 * This searchs for lines beginning with From<space> so that we can QP-encode
 * them.  It also searches for lines consisting of only a dot.  Some mailers
 * will mangle these lines.  The reason it is ifdef'd is because most people
 * seem to prefer the >From style escape provided by a lot of mail software
 * to the QP-encoding.
 * Froms, dots, bmap, and dmap may be any integer type.  C is the next char.
 * bmap and dmap should be initialized to 1.
 * froms is incremented by 1 whenever a line beginning From_ is found.
 * dots is incremented by 1 whenever a line with only a dot is found.
 */
#define Find_Froms(froms,dots,bmap,dmap,c) { int x,y;			\
				switch (c) {				\
				  case '\n': case '\r':			\
				    x = 0x1;				\
				    y = 0x7;				\
				    bmap = 0;				\
				    break;				\
				  case 'F':				\
				    x = 0x3;				\
				    y = 0;				\
				    break;				\
				  case 'r':				\
				    x = 0x7;				\
				    y = 0;				\
				    break;				\
				  case 'o':				\
				    x = 0xf;				\
				    y = 0;				\
				    break;				\
				  case 'm':				\
				    x = 0x1f;				\
				    y = 0;				\
				    break;				\
				  case ' ':				\
				    x = 0x3f;				\
				    y = 0;				\
				    break;				\
				  case '.':				\
				    x = 0;				\
				    y = 0x3;				\
				    break;				\
				  default:				\
				    x = 0;				\
				    y = 0;				\
				    break;				\
				}					\
				bmap = ((x >> 1) == bmap) ? x : 0;	\
				froms += (bmap == 0x3f ? 1 : 0);	\
				if(y == 0x7 && dmap != 0x3){		\
				    y = 0x1;				\
				    dmap = 0;				\
				}					\
				dmap = ((y >> 1) == dmap) ? y : 0;	\
				dots += (dmap == 0x7 ? 1 : 0);		\
			     }

/*
 * Useful macro to test if current folder is a bboard type (meaning
 * netnews for now) collection...
 */
#define	IS_NEWS(S)	(ps_global->nr_mode				     \
			 || ((S) && (S)->mailbox && (S)->mailbox[0] == '*'))

#define	READONLY_FOLDER  (ps_global->mail_stream 			     \
			  && ((ps_global->mail_stream->rdonly		     \
			       && !IS_NEWS(ps_global->mail_stream))	     \
			      || ps_global->mail_stream->anonymous))

/*
 * Simple, handy macro to determine if folder name is remote 
 * (on an imap server)
 */
#define	IS_REMOTE(X)	(*(X) == '{' && *((X) + 1) && *((X) + 1) != '}' \
			 && strchr(((X) + 2), '}'))


/*
 * Macro used to fetch all flags.  Used when counting deleted messages
 * and finding next message with a particular flag set.  The idea is to
 * minimize the number of times we have to fetch all of the flags for all
 * messages in the folder.
 */
#define	FETCH_ALL_FLAGS(s) {long i;					      \
			    if(s == ps_global->mail_stream){		      \
				i = max(ps_global->last_msgno_flagged,1L);    \
				ps_global->last_msgno_flagged = s->nmsgs;     \
			    }						      \
			    else					      \
			      i = 1L;					      \
									      \
			    if(i < s->nmsgs){				      \
				char         sequence[16];		      \
									      \
				sprintf(sequence,"%ld:%ld", i, s->nmsgs);     \
				mail_fetchflags(s, sequence);		      \
			    }						      \
			   }


/*======================================================================
        Key code definitions
  ===*/
#define PF1           0x0100
#define PF2           0x0101
#define PF3           0x0102
#define PF4           0x0103
#define PF5           0x0104
#define PF6           0x0105
#define PF7           0x0106
#define PF8           0x0107
#define PF9           0x0108
#define PF10          0x0109
#define PF11          0x010A
#define PF12          0x010B
      
#define OPF1          0x0110
#define OPF2          0x0111
#define OPF3          0x0112
#define OPF4          0x0113
#define OPF5          0x0114
#define OPF6          0x0115
#define OPF7          0x0116
#define OPF8          0x0117
#define OPF9          0x0118
#define OPF10         0x0119
#define OPF11         0x011A
#define OPF12         0x011B

#define OOPF1          0x0120
#define OOPF2          0x0121
#define OOPF3          0x0122
#define OOPF4          0x0123
#define OOPF5          0x0124
#define OOPF6          0x0125
#define OOPF7          0x0126
#define OOPF8          0x0127
#define OOPF9          0x0128
#define OOPF10         0x0129
#define OOPF11         0x012A
#define OOPF12         0x012B

#define OOOPF1         0x0130
#define OOOPF2         0x0131
#define OOOPF3         0x0132
#define OOOPF4         0x0133
#define OOOPF5         0x0134
#define OOOPF6         0x0135
#define OOOPF7         0x0136
#define OOOPF8         0x0137
#define OOOPF9         0x0138
#define OOOPF10        0x0139
#define OOOPF11        0x013A
#define OOOPF12        0x013B

#define PF2OPF(x)      (x + 0x10)
#define PF2OOPF(x)     (x + 0x20)
#define PF2OOOPF(x)    (x + 0x30)


/*-- some control codes for arrow keys.. see read_char */
#define KEY_UP		0x0140
#define KEY_DOWN	0x0141
#define KEY_RIGHT	0x0142
#define KEY_LEFT	0x0143
#define KEY_JUNK	0x0144
#define KEY_RESIZE	0x0145  /* Fake key to cause resize */
#define	KEY_HOME	0x0146  /* Extras that aren't used outside DOS */
#define	KEY_END		0x0147
#define	KEY_PGUP	0x0148
#define	KEY_PGDN	0x0149
#define	KEY_DEL		0x014A
#define	KEY_MOUSE	0x014B  /* Fake key to indicate mouse event. */
#define KEY_SCRLTO	0x014C
#define KEY_SCRLUPL	0x014D
#define KEY_SCRLDNL	0x014E
#define KEY_SWALLOW_Z	0x014F
#define KEY_SWAL_UP	0x0150	/* mirror KEY_UP etc. order found above */
#define KEY_SWAL_DOWN	0x0151
#define KEY_SWAL_RIGHT	0x0152
#define KEY_SWAL_LEFT	0x0153
#define KEY_DOUBLE_ESC	0x0154
#define KEY_KERMIT	0x0155
#define KEY_XTERM_MOUSE	0x0156

#define NO_OP_COMMAND	'\0'    /* no-op for short timeouts   */
#define NO_OP_IDLE	0x0200  /* no-op for timeouts > 25 seconds  */
#define READY_TO_READ	0x0201
#define KEY_MENU_FLAG	0x1000
#define KEY_MASK	0x13FF

/*
 * Macro to help with new mail check timing...
 */
#define	NM_TIMING(X)  (((X)==NO_OP_IDLE) ? 0 : ((X)==NO_OP_COMMAND) ? 1 : 2)


/*
 * The array is initialized in init.c so the order of that initialization
 * must correspond to the order of the values here.  The order is
 * significant in that it determines the order that the variables
 * are written into the pinerc file.
 */
#define V_PERSONAL_NAME            0
#define V_USER_ID                  1
#define V_USER_DOMAIN              2
#define V_SMTP_SERVER              3
#define V_NNTP_SERVER              4
#define V_INBOX_PATH               5
#define V_INCOMING_FOLDERS         6
#define V_FOLDER_SPEC              7
#define V_NEWS_SPEC                8
#define	V_ARCHIVED_FOLDERS	   9
#define	V_PRUNED_FOLDERS	   10
#define V_DEFAULT_FCC              11
#define V_DEFAULT_SAVE_FOLDER      12
#define V_POSTPONED_FOLDER         13
#define V_MAIL_DIRECTORY           14
#define V_READ_MESSAGE_FOLDER      15
#define V_SIGNATURE_FILE           16
#define V_GLOB_ADDRBOOK            17
#define V_ADDRESSBOOK              18
#define V_FEATURE_LIST             19
#define V_INIT_CMD_LIST            20
#define V_COMP_HDRS                21
#define V_CUSTOM_HDRS              22
#define V_VIEW_HEADERS             23
#define V_SAVED_MSG_NAME_RULE      24
#define V_FCC_RULE                 25
#define V_SORT_KEY                 26
#define V_AB_SORT_RULE             27
#define	V_GOTO_DEFAULT_RULE	   28
#define V_CHAR_SET                 29
#define V_EDITOR                   30
#define V_SPELLER                  31
#define V_FILLCOL                  32
#define V_REPLY_STRING             33
#define V_EMPTY_HDR_MSG            34
#define V_IMAGE_VIEWER             35
#define V_USE_ONLY_DOMAIN_NAME     36
#define V_PRINTER                  37
#define V_PERSONAL_PRINT_COMMAND   38
#define V_PERSONAL_PRINT_CATEGORY  39
#define V_STANDARD_PRINTER         40
#define V_LAST_TIME_PRUNE_QUESTION 41
#define V_LAST_VERS_USED           42
#define V_BUGS_FULLNAME            43
#define V_BUGS_ADDRESS             44
#define V_BUGS_EXTRAS              45
#define V_SUGGEST_FULLNAME         46
#define V_SUGGEST_ADDRESS          47
#define V_LOCAL_FULLNAME           48
#define V_LOCAL_ADDRESS            49
#define V_FORCED_ABOOK_ENTRY       50
#define V_KBLOCK_PASSWD_COUNT      51
#define	V_SENDMAIL_PATH		   52
#define	V_OPER_DIR		   53
#define	V_DISPLAY_FILTERS	   54
#define	V_SEND_FILTER		   55
#define	V_ALT_ADDRS		   56
#define	V_ABOOK_FORMATS		   57
#define	V_INDEX_FORMAT		   58
#define	V_OVERLAP		   59
#define	V_MARGIN		   60
#define V_STATUS_MSG_DELAY	   61
#define	V_MAILCHECK		   62
#define	V_NEWSRC_PATH		   63
#define	V_NEWS_ACTIVE_PATH	   64
#define	V_NEWS_SPOOL_DIR	   65
#define	V_UPLOAD_CMD		   66
#define	V_UPLOAD_CMD_PREFIX	   67
#define	V_DOWNLOAD_CMD		   68
#define	V_DOWNLOAD_CMD_PREFIX	   69
#define	V_MAILCAP_PATH		   70
#define	V_MIMETYPE_PATH		   71
#define	V_TCPOPENTIMEO		   72
#define	V_RSHOPENTIMEO		   73
#define	V_NEW_VER_QUELL		   74

#ifdef	NEWBB
#define V_NNTP_NEW_GROUP_TIME      75
#define	V_LAST_REGULAR_VAR	   75
#else
#define	V_LAST_REGULAR_VAR	   74
#endif
#define V_ELM_STYLE_SAVE           (V_LAST_REGULAR_VAR + 1)  /* obsolete */
#define V_HEADER_IN_REPLY          (V_LAST_REGULAR_VAR + 2)  /* obsolete */
#define V_FEATURE_LEVEL            (V_LAST_REGULAR_VAR + 3)  /* obsolete */
#define V_OLD_STYLE_REPLY          (V_LAST_REGULAR_VAR + 4)  /* obsolete */
#define V_COMPOSE_MIME             (V_LAST_REGULAR_VAR + 5)  /* obsolete */
#define V_SHOW_ALL_CHARACTERS      (V_LAST_REGULAR_VAR + 6)  /* obsolete */
#define V_SAVE_BY_SENDER           (V_LAST_REGULAR_VAR + 7)  /* obsolete */
#if defined(DOS) || defined(OS2)
#define V_FOLDER_EXTENSION         (V_LAST_REGULAR_VAR + 8)
#define V_NORM_FORE_COLOR          (V_LAST_REGULAR_VAR + 9)
#define V_NORM_BACK_COLOR          (V_LAST_REGULAR_VAR + 10)
#define V_REV_FORE_COLOR           (V_LAST_REGULAR_VAR + 11)
#define V_REV_BACK_COLOR           (V_LAST_REGULAR_VAR + 12)
#ifdef	_WINDOWS
#define V_FONT_NAME		   (V_LAST_REGULAR_VAR + 13)
#define V_FONT_SIZE		   (V_LAST_REGULAR_VAR + 14)
#define V_FONT_STYLE		   (V_LAST_REGULAR_VAR + 15)
#define V_PRINT_FONT_NAME	   (V_LAST_REGULAR_VAR + 16)
#define V_PRINT_FONT_SIZE	   (V_LAST_REGULAR_VAR + 17)
#define V_PRINT_FONT_STYLE	   (V_LAST_REGULAR_VAR + 18)
#define V_WINDOW_POSITION	   (V_LAST_REGULAR_VAR + 19)
#define	V_LAST_VAR		   (V_LAST_REGULAR_VAR + 19)
#else   /* !_WINDOWS */
#define	V_LAST_VAR		   (V_LAST_REGULAR_VAR + 12)
#endif  /* _WINDOWS */
#else   /* !DOS */
#define	V_LAST_VAR		   (V_LAST_REGULAR_VAR + 7)
#endif  /* DOS */


#define VAR_PERSONAL_NAME	     vars[V_PERSONAL_NAME].current_val.p
#define USR_PERSONAL_NAME	     vars[V_PERSONAL_NAME].user_val.p
#define GLO_PERSONAL_NAME	     vars[V_PERSONAL_NAME].global_val.p
#define FIX_PERSONAL_NAME	     vars[V_PERSONAL_NAME].fixed_val.p
#define COM_PERSONAL_NAME	     vars[V_PERSONAL_NAME].cmdline_val.p
#define VAR_USER_ID		     vars[V_USER_ID].current_val.p
#define USR_USER_ID		     vars[V_USER_ID].user_val.p
#define GLO_USER_ID		     vars[V_USER_ID].global_val.p
#define FIX_USER_ID		     vars[V_USER_ID].fixed_val.p
#define COM_USER_ID		     vars[V_USER_ID].cmdline_val.p
#define VAR_USER_DOMAIN		     vars[V_USER_DOMAIN].current_val.p
#define USR_USER_DOMAIN		     vars[V_USER_DOMAIN].user_val.p
#define GLO_USER_DOMAIN		     vars[V_USER_DOMAIN].global_val.p
#define FIX_USER_DOMAIN		     vars[V_USER_DOMAIN].fixed_val.p
#define COM_USER_DOMAIN		     vars[V_USER_DOMAIN].cmdline_val.p
#define VAR_SMTP_SERVER		     vars[V_SMTP_SERVER].current_val.l
#define USR_SMTP_SERVER		     vars[V_SMTP_SERVER].user_val.l
#define GLO_SMTP_SERVER		     vars[V_SMTP_SERVER].global_val.l
#define FIX_SMTP_SERVER		     vars[V_SMTP_SERVER].fixed_val.l
#define COM_SMTP_SERVER		     vars[V_SMTP_SERVER].cmdline_val.l
#define VAR_INBOX_PATH		     vars[V_INBOX_PATH].current_val.p
#define USR_INBOX_PATH		     vars[V_INBOX_PATH].user_val.p
#define GLO_INBOX_PATH		     vars[V_INBOX_PATH].global_val.p
#define FIX_INBOX_PATH		     vars[V_INBOX_PATH].fixed_val.p
#define COM_INBOX_PATH		     vars[V_INBOX_PATH].cmdline_val.p
#define VAR_INCOMING_FOLDERS	     vars[V_INCOMING_FOLDERS].current_val.l
#define USR_INCOMING_FOLDERS	     vars[V_INCOMING_FOLDERS].user_val.l
#define GLO_INCOMING_FOLDERS	     vars[V_INCOMING_FOLDERS].global_val.l
#define FIX_INCOMING_FOLDERS	     vars[V_INCOMING_FOLDERS].fixed_val.l
#define COM_INCOMING_FOLDERS	     vars[V_INCOMING_FOLDERS].cmdline_val.l
#define VAR_FOLDER_SPEC		     vars[V_FOLDER_SPEC].current_val.l
#define USR_FOLDER_SPEC		     vars[V_FOLDER_SPEC].user_val.l
#define GLO_FOLDER_SPEC		     vars[V_FOLDER_SPEC].global_val.l
#define FIX_FOLDER_SPEC		     vars[V_FOLDER_SPEC].fixed_val.l
#define COM_FOLDER_SPEC		     vars[V_FOLDER_SPEC].cmdline_val.l
#define VAR_NEWS_SPEC		     vars[V_NEWS_SPEC].current_val.l
#define USR_NEWS_SPEC		     vars[V_NEWS_SPEC].user_val.l
#define GLO_NEWS_SPEC		     vars[V_NEWS_SPEC].global_val.l
#define FIX_NEWS_SPEC		     vars[V_NEWS_SPEC].fixed_val.l
#define COM_NEWS_SPEC		     vars[V_NEWS_SPEC].cmdline_val.l
#define VAR_ARCHIVED_FOLDERS	     vars[V_ARCHIVED_FOLDERS].current_val.l
#define USR_ARCHIVED_FOLDERS	     vars[V_ARCHIVED_FOLDERS].user_val.l
#define GLO_ARCHIVED_FOLDERS	     vars[V_ARCHIVED_FOLDERS].global_val.l
#define FIX_ARCHIVED_FOLDERS	     vars[V_ARCHIVED_FOLDERS].fixed_val.l
#define COM_ARCHIVED_FOLDERS	     vars[V_ARCHIVED_FOLDERS].cmdline_val.l
#define VAR_PRUNED_FOLDERS	     vars[V_PRUNED_FOLDERS].current_val.l
#define USR_PRUNED_FOLDERS	     vars[V_PRUNED_FOLDERS].user_val.l
#define GLO_PRUNED_FOLDERS	     vars[V_PRUNED_FOLDERS].global_val.l
#define FIX_PRUNED_FOLDERS	     vars[V_PRUNED_FOLDERS].fixed_val.l
#define COM_PRUNED_FOLDERS	     vars[V_PRUNED_FOLDERS].cmdline_val.l
#define VAR_DEFAULT_FCC		     vars[V_DEFAULT_FCC].current_val.p
#define USR_DEFAULT_FCC		     vars[V_DEFAULT_FCC].user_val.p
#define GLO_DEFAULT_FCC		     vars[V_DEFAULT_FCC].global_val.p
#define FIX_DEFAULT_FCC		     vars[V_DEFAULT_FCC].fixed_val.p
#define COM_DEFAULT_FCC		     vars[V_DEFAULT_FCC].cmdline_val.p
#define VAR_DEFAULT_SAVE_FOLDER	     vars[V_DEFAULT_SAVE_FOLDER].current_val.p
#define USR_DEFAULT_SAVE_FOLDER	     vars[V_DEFAULT_SAVE_FOLDER].user_val.p
#define GLO_DEFAULT_SAVE_FOLDER	     vars[V_DEFAULT_SAVE_FOLDER].global_val.p
#define FIX_DEFAULT_SAVE_FOLDER	     vars[V_DEFAULT_SAVE_FOLDER].fixed_val.p
#define COM_DEFAULT_SAVE_FOLDER	     vars[V_DEFAULT_SAVE_FOLDER].cmdline_val.p
#define VAR_POSTPONED_FOLDER	     vars[V_POSTPONED_FOLDER].current_val.p
#define USR_POSTPONED_FOLDER	     vars[V_POSTPONED_FOLDER].user_val.p
#define GLO_POSTPONED_FOLDER	     vars[V_POSTPONED_FOLDER].global_val.p
#define FIX_POSTPONED_FOLDER	     vars[V_POSTPONED_FOLDER].fixed_val.p
#define COM_POSTPONED_FOLDER	     vars[V_POSTPONED_FOLDER].cmdline_val.p
#define VAR_MAIL_DIRECTORY	     vars[V_MAIL_DIRECTORY].current_val.p
#define USR_MAIL_DIRECTORY	     vars[V_MAIL_DIRECTORY].user_val.p
#define GLO_MAIL_DIRECTORY	     vars[V_MAIL_DIRECTORY].global_val.p
#define FIX_MAIL_DIRECTORY	     vars[V_MAIL_DIRECTORY].fixed_val.p
#define COM_MAIL_DIRECTORY	     vars[V_MAIL_DIRECTORY].cmdline_val.p
#define VAR_READ_MESSAGE_FOLDER	     vars[V_READ_MESSAGE_FOLDER].current_val.p
#define USR_READ_MESSAGE_FOLDER	     vars[V_READ_MESSAGE_FOLDER].user_val.p
#define GLO_READ_MESSAGE_FOLDER	     vars[V_READ_MESSAGE_FOLDER].global_val.p
#define FIX_READ_MESSAGE_FOLDER	     vars[V_READ_MESSAGE_FOLDER].fixed_val.p
#define COM_READ_MESSAGE_FOLDER	     vars[V_READ_MESSAGE_FOLDER].cmdline_val.p
#define VAR_SIGNATURE_FILE	     vars[V_SIGNATURE_FILE].current_val.p
#define USR_SIGNATURE_FILE	     vars[V_SIGNATURE_FILE].user_val.p
#define GLO_SIGNATURE_FILE	     vars[V_SIGNATURE_FILE].global_val.p
#define FIX_SIGNATURE_FILE	     vars[V_SIGNATURE_FILE].fixed_val.p
#define COM_SIGNATURE_FILE	     vars[V_SIGNATURE_FILE].cmdline_val.p
#define VAR_GLOB_ADDRBOOK	     vars[V_GLOB_ADDRBOOK].current_val.l
#define USR_GLOB_ADDRBOOK	     vars[V_GLOB_ADDRBOOK].user_val.l
#define GLO_GLOB_ADDRBOOK	     vars[V_GLOB_ADDRBOOK].global_val.l
#define FIX_GLOB_ADDRBOOK	     vars[V_GLOB_ADDRBOOK].fixed_val.l
#define COM_GLOB_ADDRBOOK	     vars[V_GLOB_ADDRBOOK].cmdline_val.l
#define VAR_ADDRESSBOOK		     vars[V_ADDRESSBOOK].current_val.l
#define USR_ADDRESSBOOK		     vars[V_ADDRESSBOOK].user_val.l
#define GLO_ADDRESSBOOK		     vars[V_ADDRESSBOOK].global_val.l
#define FIX_ADDRESSBOOK		     vars[V_ADDRESSBOOK].fixed_val.l
#define COM_ADDRESSBOOK		     vars[V_ADDRESSBOOK].cmdline_val.l
#define VAR_FEATURE_LIST	     vars[V_FEATURE_LIST].current_val.l
#define USR_FEATURE_LIST	     vars[V_FEATURE_LIST].user_val.l
#define GLO_FEATURE_LIST	     vars[V_FEATURE_LIST].global_val.l
#define FIX_FEATURE_LIST	     vars[V_FEATURE_LIST].fixed_val.l
#define COM_FEATURE_LIST	     vars[V_FEATURE_LIST].cmdline_val.l
#define VAR_INIT_CMD_LIST	     vars[V_INIT_CMD_LIST].current_val.l
#define USR_INIT_CMD_LIST	     vars[V_INIT_CMD_LIST].user_val.l
#define GLO_INIT_CMD_LIST	     vars[V_INIT_CMD_LIST].global_val.l
#define FIX_INIT_CMD_LIST	     vars[V_INIT_CMD_LIST].fixed_val.l
#define COM_INIT_CMD_LIST	     vars[V_INIT_CMD_LIST].cmdline_val.l
#define VAR_COMP_HDRS		     vars[V_COMP_HDRS].current_val.l
#define USR_COMP_HDRS		     vars[V_COMP_HDRS].user_val.l
#define GLO_COMP_HDRS		     vars[V_COMP_HDRS].global_val.l
#define FIX_COMP_HDRS		     vars[V_COMP_HDRS].fixed_val.l
#define COM_COMP_HDRS		     vars[V_COMP_HDRS].cmdline_val.l
#define VAR_CUSTOM_HDRS		     vars[V_CUSTOM_HDRS].current_val.l
#define USR_CUSTOM_HDRS		     vars[V_CUSTOM_HDRS].user_val.l
#define GLO_CUSTOM_HDRS		     vars[V_CUSTOM_HDRS].global_val.l
#define FIX_CUSTOM_HDRS		     vars[V_CUSTOM_HDRS].fixed_val.l
#define COM_CUSTOM_HDRS		     vars[V_CUSTOM_HDRS].cmdline_val.l
#define VAR_VIEW_HEADERS	     vars[V_VIEW_HEADERS].current_val.l
#define USR_VIEW_HEADERS	     vars[V_VIEW_HEADERS].user_val.l
#define GLO_VIEW_HEADERS	     vars[V_VIEW_HEADERS].global_val.l
#define FIX_VIEW_HEADERS	     vars[V_VIEW_HEADERS].fixed_val.l
#define COM_VIEW_HEADERS	     vars[V_VIEW_HEADERS].cmdline_val.l
#define VAR_SAVED_MSG_NAME_RULE	     vars[V_SAVED_MSG_NAME_RULE].current_val.p
#define USR_SAVED_MSG_NAME_RULE	     vars[V_SAVED_MSG_NAME_RULE].user_val.p
#define GLO_SAVED_MSG_NAME_RULE	     vars[V_SAVED_MSG_NAME_RULE].global_val.p
#define FIX_SAVED_MSG_NAME_RULE	     vars[V_SAVED_MSG_NAME_RULE].fixed_val.p
#define COM_SAVED_MSG_NAME_RULE	     vars[V_SAVED_MSG_NAME_RULE].cmdline_val.p
#define VAR_FCC_RULE		     vars[V_FCC_RULE].current_val.p
#define USR_FCC_RULE		     vars[V_FCC_RULE].user_val.p
#define GLO_FCC_RULE		     vars[V_FCC_RULE].global_val.p
#define FIX_FCC_RULE		     vars[V_FCC_RULE].fixed_val.p
#define COM_FCC_RULE		     vars[V_FCC_RULE].cmdline_val.p
#define VAR_SORT_KEY		     vars[V_SORT_KEY].current_val.p
#define USR_SORT_KEY		     vars[V_SORT_KEY].user_val.p
#define GLO_SORT_KEY		     vars[V_SORT_KEY].global_val.p
#define FIX_SORT_KEY		     vars[V_SORT_KEY].fixed_val.p
#define COM_SORT_KEY		     vars[V_SORT_KEY].cmdline_val.p
#define VAR_AB_SORT_RULE	     vars[V_AB_SORT_RULE].current_val.p
#define USR_AB_SORT_RULE	     vars[V_AB_SORT_RULE].user_val.p
#define GLO_AB_SORT_RULE	     vars[V_AB_SORT_RULE].global_val.p
#define FIX_AB_SORT_RULE	     vars[V_AB_SORT_RULE].fixed_val.p
#define COM_AB_SORT_RULE	     vars[V_AB_SORT_RULE].cmdline_val.p
#define VAR_CHAR_SET		     vars[V_CHAR_SET].current_val.p
#define USR_CHAR_SET		     vars[V_CHAR_SET].user_val.p
#define GLO_CHAR_SET		     vars[V_CHAR_SET].global_val.p
#define FIX_CHAR_SET		     vars[V_CHAR_SET].fixed_val.p
#define COM_CHAR_SET		     vars[V_CHAR_SET].cmdline_val.p
#define VAR_EDITOR		     vars[V_EDITOR].current_val.p
#define USR_EDITOR		     vars[V_EDITOR].user_val.p
#define GLO_EDITOR		     vars[V_EDITOR].global_val.p
#define FIX_EDITOR		     vars[V_EDITOR].fixed_val.p
#define COM_EDITOR		     vars[V_EDITOR].cmdline_val.p
#define VAR_SPELLER		     vars[V_SPELLER].current_val.p
#define USR_SPELLER		     vars[V_SPELLER].user_val.p
#define GLO_SPELLER		     vars[V_SPELLER].global_val.p
#define FIX_SPELLER		     vars[V_SPELLER].fixed_val.p
#define COM_SPELLER		     vars[V_SPELLER].cmdline_val.p
#define VAR_FILLCOL		     vars[V_FILLCOL].current_val.p
#define USR_FILLCOL		     vars[V_FILLCOL].user_val.p
#define GLO_FILLCOL		     vars[V_FILLCOL].global_val.p
#define FIX_FILLCOL		     vars[V_FILLCOL].fixed_val.p
#define COM_FILLCOL		     vars[V_FILLCOL].cmdline_val.p
#define VAR_REPLY_STRING	     vars[V_REPLY_STRING].current_val.p
#define USR_REPLY_STRING	     vars[V_REPLY_STRING].user_val.p
#define GLO_REPLY_STRING	     vars[V_REPLY_STRING].global_val.p
#define FIX_REPLY_STRING	     vars[V_REPLY_STRING].fixed_val.p
#define COM_REPLY_STRING	     vars[V_REPLY_STRING].cmdline_val.p
#define VAR_EMPTY_HDR_MSG	     vars[V_EMPTY_HDR_MSG].current_val.p
#define USR_EMPTY_HDR_MSG	     vars[V_EMPTY_HDR_MSG].user_val.p
#define GLO_EMPTY_HDR_MSG	     vars[V_EMPTY_HDR_MSG].global_val.p
#define FIX_EMPTY_HDR_MSG	     vars[V_EMPTY_HDR_MSG].fixed_val.p
#define COM_EMPTY_HDR_MSG	     vars[V_EMPTY_HDR_MSG].cmdline_val.p
#define VAR_IMAGE_VIEWER	     vars[V_IMAGE_VIEWER].current_val.p
#define USR_IMAGE_VIEWER	     vars[V_IMAGE_VIEWER].user_val.p
#define GLO_IMAGE_VIEWER	     vars[V_IMAGE_VIEWER].global_val.p
#define FIX_IMAGE_VIEWER	     vars[V_IMAGE_VIEWER].fixed_val.p
#define COM_IMAGE_VIEWER	     vars[V_IMAGE_VIEWER].cmdline_val.p
#define VAR_USE_ONLY_DOMAIN_NAME     vars[V_USE_ONLY_DOMAIN_NAME].current_val.p
#define USR_USE_ONLY_DOMAIN_NAME     vars[V_USE_ONLY_DOMAIN_NAME].user_val.p
#define GLO_USE_ONLY_DOMAIN_NAME     vars[V_USE_ONLY_DOMAIN_NAME].global_val.p
#define FIX_USE_ONLY_DOMAIN_NAME     vars[V_USE_ONLY_DOMAIN_NAME].fixed_val.p
#define COM_USE_ONLY_DOMAIN_NAME     vars[V_USE_ONLY_DOMAIN_NAME].cmdline_val.p
#define VAR_PRINTER		     vars[V_PRINTER].current_val.p
#define USR_PRINTER		     vars[V_PRINTER].user_val.p
#define GLO_PRINTER		     vars[V_PRINTER].global_val.p
#define FIX_PRINTER		     vars[V_PRINTER].fixed_val.p
#define COM_PRINTER		     vars[V_PRINTER].cmdline_val.p
#define VAR_PERSONAL_PRINT_COMMAND   vars[V_PERSONAL_PRINT_COMMAND].current_val.l
#define USR_PERSONAL_PRINT_COMMAND   vars[V_PERSONAL_PRINT_COMMAND].user_val.l
#define GLO_PERSONAL_PRINT_COMMAND   vars[V_PERSONAL_PRINT_COMMAND].global_val.l
#define FIX_PERSONAL_PRINT_COMMAND   vars[V_PERSONAL_PRINT_COMMAND].fixed_val.l
#define COM_PERSONAL_PRINT_COMMAND   vars[V_PERSONAL_PRINT_COMMAND].cmdline_val.l
#define VAR_PERSONAL_PRINT_CATEGORY  vars[V_PERSONAL_PRINT_CATEGORY].current_val.p
#define USR_PERSONAL_PRINT_CATEGORY  vars[V_PERSONAL_PRINT_CATEGORY].user_val.p
#define GLO_PERSONAL_PRINT_CATEGORY  vars[V_PERSONAL_PRINT_CATEGORY].global_val.p
#define FIX_PERSONAL_PRINT_CATEGORY  vars[V_PERSONAL_PRINT_CATEGORY].fixed_val.p
#define COM_PERSONAL_PRINT_CATEGORY  vars[V_PERSONAL_PRINT_CATEGORY].cmdline_val.p
#define VAR_STANDARD_PRINTER	     vars[V_STANDARD_PRINTER].current_val.l
#define USR_STANDARD_PRINTER	     vars[V_STANDARD_PRINTER].user_val.l
#define GLO_STANDARD_PRINTER	     vars[V_STANDARD_PRINTER].global_val.l
#define FIX_STANDARD_PRINTER	     vars[V_STANDARD_PRINTER].fixed_val.l
#define COM_STANDARD_PRINTER	     vars[V_STANDARD_PRINTER].cmdline_val.l
#define VAR_LAST_TIME_PRUNE_QUESTION vars[V_LAST_TIME_PRUNE_QUESTION].current_val.p
#define USR_LAST_TIME_PRUNE_QUESTION vars[V_LAST_TIME_PRUNE_QUESTION].user_val.p
#define GLO_LAST_TIME_PRUNE_QUESTION vars[V_LAST_TIME_PRUNE_QUESTION].global_val.p
#define FIX_LAST_TIME_PRUNE_QUESTION vars[V_LAST_TIME_PRUNE_QUESTION].fixed_val.p
#define COM_LAST_TIME_PRUNE_QUESTION vars[V_LAST_TIME_PRUNE_QUESTION].cmdline_val.p
#define VAR_LAST_VERS_USED	     vars[V_LAST_VERS_USED].current_val.p
#define USR_LAST_VERS_USED	     vars[V_LAST_VERS_USED].user_val.p
#define GLO_LAST_VERS_USED	     vars[V_LAST_VERS_USED].global_val.p
#define FIX_LAST_VERS_USED	     vars[V_LAST_VERS_USED].fixed_val.p
#define COM_LAST_VERS_USED	     vars[V_LAST_VERS_USED].cmdline_val.p
#define VAR_BUGS_FULLNAME	     vars[V_BUGS_FULLNAME].current_val.p
#define USR_BUGS_FULLNAME	     vars[V_BUGS_FULLNAME].user_val.p
#define GLO_BUGS_FULLNAME	     vars[V_BUGS_FULLNAME].global_val.p
#define FIX_BUGS_FULLNAME	     vars[V_BUGS_FULLNAME].fixed_val.p
#define COM_BUGS_FULLNAME	     vars[V_BUGS_FULLNAME].cmdline_val.p
#define VAR_BUGS_ADDRESS	     vars[V_BUGS_ADDRESS].current_val.p
#define USR_BUGS_ADDRESS	     vars[V_BUGS_ADDRESS].user_val.p
#define GLO_BUGS_ADDRESS	     vars[V_BUGS_ADDRESS].global_val.p
#define FIX_BUGS_ADDRESS	     vars[V_BUGS_ADDRESS].fixed_val.p
#define COM_BUGS_ADDRESS	     vars[V_BUGS_ADDRESS].cmdline_val.p
#define VAR_BUGS_EXTRAS		     vars[V_BUGS_EXTRAS].current_val.p
#define USR_BUGS_EXTRAS		     vars[V_BUGS_EXTRAS].user_val.p
#define GLO_BUGS_EXTRAS		     vars[V_BUGS_EXTRAS].global_val.p
#define FIX_BUGS_EXTRAS		     vars[V_BUGS_EXTRAS].fixed_val.p
#define COM_BUGS_EXTRAS		     vars[V_BUGS_EXTRAS].cmdline_val.p
#define VAR_SUGGEST_FULLNAME	     vars[V_SUGGEST_FULLNAME].current_val.p
#define USR_SUGGEST_FULLNAME	     vars[V_SUGGEST_FULLNAME].user_val.p
#define GLO_SUGGEST_FULLNAME	     vars[V_SUGGEST_FULLNAME].global_val.p
#define FIX_SUGGEST_FULLNAME	     vars[V_SUGGEST_FULLNAME].fixed_val.p
#define COM_SUGGEST_FULLNAME	     vars[V_SUGGEST_FULLNAME].cmdline_val.p
#define VAR_SUGGEST_ADDRESS	     vars[V_SUGGEST_ADDRESS].current_val.p
#define USR_SUGGEST_ADDRESS	     vars[V_SUGGEST_ADDRESS].user_val.p
#define GLO_SUGGEST_ADDRESS	     vars[V_SUGGEST_ADDRESS].global_val.p
#define FIX_SUGGEST_ADDRESS	     vars[V_SUGGEST_ADDRESS].fixed_val.p
#define COM_SUGGEST_ADDRESS	     vars[V_SUGGEST_ADDRESS].cmdline_val.p
#define VAR_LOCAL_FULLNAME	     vars[V_LOCAL_FULLNAME].current_val.p
#define USR_LOCAL_FULLNAME	     vars[V_LOCAL_FULLNAME].user_val.p
#define GLO_LOCAL_FULLNAME	     vars[V_LOCAL_FULLNAME].global_val.p
#define FIX_LOCAL_FULLNAME	     vars[V_LOCAL_FULLNAME].fixed_val.p
#define COM_LOCAL_FULLNAME	     vars[V_LOCAL_FULLNAME].cmdline_val.p
#define VAR_LOCAL_ADDRESS	     vars[V_LOCAL_ADDRESS].current_val.p
#define USR_LOCAL_ADDRESS	     vars[V_LOCAL_ADDRESS].user_val.p
#define GLO_LOCAL_ADDRESS	     vars[V_LOCAL_ADDRESS].global_val.p
#define FIX_LOCAL_ADDRESS	     vars[V_LOCAL_ADDRESS].fixed_val.p
#define COM_LOCAL_ADDRESS	     vars[V_LOCAL_ADDRESS].cmdline_val.p
#define VAR_FORCED_ABOOK_ENTRY	     vars[V_FORCED_ABOOK_ENTRY].current_val.l
#define USR_FORCED_ABOOK_ENTRY	     vars[V_FORCED_ABOOK_ENTRY].user_val.l
#define GLO_FORCED_ABOOK_ENTRY	     vars[V_FORCED_ABOOK_ENTRY].global_val.l
#define FIX_FORCED_ABOOK_ENTRY	     vars[V_FORCED_ABOOK_ENTRY].fixed_val.l
#define COM_FORCED_ABOOK_ENTRY	     vars[V_FORCED_ABOOK_ENTRY].cmdline_val.l
#define VAR_KBLOCK_PASSWD_COUNT	     vars[V_KBLOCK_PASSWD_COUNT].current_val.p
#define USR_KBLOCK_PASSWD_COUNT	     vars[V_KBLOCK_PASSWD_COUNT].user_val.p
#define GLO_KBLOCK_PASSWD_COUNT	     vars[V_KBLOCK_PASSWD_COUNT].global_val.p
#define FIX_KBLOCK_PASSWD_COUNT	     vars[V_KBLOCK_PASSWD_COUNT].fixed_val.p
#define COM_KBLOCK_PASSWD_COUNT	     vars[V_KBLOCK_PASSWD_COUNT].cmdline_val.p
#define VAR_STATUS_MSG_DELAY	     vars[V_STATUS_MSG_DELAY].current_val.p
#define USR_STATUS_MSG_DELAY	     vars[V_STATUS_MSG_DELAY].user_val.p
#define GLO_STATUS_MSG_DELAY	     vars[V_STATUS_MSG_DELAY].global_val.p
#define FIX_STATUS_MSG_DELAY	     vars[V_STATUS_MSG_DELAY].fixed_val.p
#define COM_STATUS_MSG_DELAY	     vars[V_STATUS_MSG_DELAY].cmdline_val.p
#define VAR_SENDMAIL_PATH	     vars[V_SENDMAIL_PATH].current_val.p
#define USR_SENDMAIL_PATH	     vars[V_SENDMAIL_PATH].user_val.p
#define GLO_SENDMAIL_PATH	     vars[V_SENDMAIL_PATH].global_val.p
#define FIX_SENDMAIL_PATH	     vars[V_SENDMAIL_PATH].fixed_val.p
#define COM_SENDMAIL_PATH	     vars[V_SENDMAIL_PATH].cmdline_val.p
#define VAR_OPER_DIR		     vars[V_OPER_DIR].current_val.p
#define USR_OPER_DIR		     vars[V_OPER_DIR].user_val.p
#define GLO_OPER_DIR		     vars[V_OPER_DIR].global_val.p
#define FIX_OPER_DIR		     vars[V_OPER_DIR].fixed_val.p
#define COM_OPER_DIR		     vars[V_OPER_DIR].cmdline_val.p
#define VAR_DISPLAY_FILTERS	     vars[V_DISPLAY_FILTERS].current_val.l
#define USR_DISPLAY_FILTERS	     vars[V_DISPLAY_FILTERS].user_val.l
#define GLO_DISPLAY_FILTERS	     vars[V_DISPLAY_FILTERS].global_val.l
#define FIX_DISPLAY_FILTERS	     vars[V_DISPLAY_FILTERS].fixed_val.l
#define COM_DISPLAY_FILTERS	     vars[V_DISPLAY_FILTERS].cmdline_val.l
#define VAR_SEND_FILTER		     vars[V_SEND_FILTER].current_val.l
#define USR_SEND_FILTER		     vars[V_SEND_FILTER].user_val.l
#define GLO_SEND_FILTER		     vars[V_SEND_FILTER].global_val.l
#define FIX_SEND_FILTER		     vars[V_SEND_FILTER].fixed_val.l
#define COM_SEND_FILTER		     vars[V_SEND_FILTER].cmdline_val.l
#define VAR_ALT_ADDRS		     vars[V_ALT_ADDRS].current_val.l
#define USR_ALT_ADDRS		     vars[V_ALT_ADDRS].user_val.l
#define GLO_ALT_ADDRS		     vars[V_ALT_ADDRS].global_val.l
#define FIX_ALT_ADDRS		     vars[V_ALT_ADDRS].fixed_val.l
#define COM_ALT_ADDRS		     vars[V_ALT_ADDRS].cmdline_val.l
#define VAR_ABOOK_FORMATS	     vars[V_ABOOK_FORMATS].current_val.l
#define USR_ABOOK_FORMATS	     vars[V_ABOOK_FORMATS].user_val.l
#define GLO_ABOOK_FORMATS	     vars[V_ABOOK_FORMATS].global_val.l
#define FIX_ABOOK_FORMATS	     vars[V_ABOOK_FORMATS].fixed_val.l
#define COM_ABOOK_FORMATS	     vars[V_ABOOK_FORMATS].cmdline_val.l
#define VAR_INDEX_FORMAT	     vars[V_INDEX_FORMAT].current_val.p
#define USR_INDEX_FORMAT	     vars[V_INDEX_FORMAT].user_val.p
#define GLO_INDEX_FORMAT	     vars[V_INDEX_FORMAT].global_val.p
#define FIX_INDEX_FORMAT	     vars[V_INDEX_FORMAT].fixed_val.p
#define COM_INDEX_FORMAT	     vars[V_INDEX_FORMAT].cmdline_val.p
#define VAR_OVERLAP		     vars[V_OVERLAP].current_val.p
#define USR_OVERLAP		     vars[V_OVERLAP].user_val.p
#define GLO_OVERLAP		     vars[V_OVERLAP].global_val.p
#define FIX_OVERLAP		     vars[V_OVERLAP].fixed_val.p
#define COM_OVERLAP		     vars[V_OVERLAP].cmdline_val.p
#define VAR_MARGIN		     vars[V_MARGIN].current_val.p
#define USR_MARGIN		     vars[V_MARGIN].user_val.p
#define GLO_MARGIN		     vars[V_MARGIN].global_val.p
#define FIX_MARGIN		     vars[V_MARGIN].fixed_val.p
#define COM_MARGIN		     vars[V_MARGIN].cmdline_val.p
#define VAR_MAILCHECK		     vars[V_MAILCHECK].current_val.p
#define USR_MAILCHECK		     vars[V_MAILCHECK].user_val.p
#define GLO_MAILCHECK		     vars[V_MAILCHECK].global_val.p
#define FIX_MAILCHECK		     vars[V_MAILCHECK].fixed_val.p
#define COM_MAILCHECK		     vars[V_MAILCHECK].cmdline_val.p
#define VAR_NEWSRC_PATH		     vars[V_NEWSRC_PATH].current_val.p
#define USR_NEWSRC_PATH		     vars[V_NEWSRC_PATH].user_val.p
#define GLO_NEWSRC_PATH		     vars[V_NEWSRC_PATH].global_val.p
#define FIX_NEWSRC_PATH		     vars[V_NEWSRC_PATH].fixed_val.p
#define COM_NEWSRC_PATH		     vars[V_NEWSRC_PATH].cmdline_val.p
#define VAR_NEWS_ACTIVE_PATH	     vars[V_NEWS_ACTIVE_PATH].current_val.p
#define USR_NEWS_ACTIVE_PATH	     vars[V_NEWS_ACTIVE_PATH].user_val.p
#define GLO_NEWS_ACTIVE_PATH	     vars[V_NEWS_ACTIVE_PATH].global_val.p
#define FIX_NEWS_ACTIVE_PATH	     vars[V_NEWS_ACTIVE_PATH].fixed_val.p
#define COM_NEWS_ACTIVE_PATH	     vars[V_NEWS_ACTIVE_PATH].cmdline_val.p
#define VAR_NEWS_SPOOL_DIR	     vars[V_NEWS_SPOOL_DIR].current_val.p
#define USR_NEWS_SPOOL_DIR	     vars[V_NEWS_SPOOL_DIR].user_val.p
#define GLO_NEWS_SPOOL_DIR	     vars[V_NEWS_SPOOL_DIR].global_val.p
#define FIX_NEWS_SPOOL_DIR	     vars[V_NEWS_SPOOL_DIR].fixed_val.p
#define COM_NEWS_SPOOL_DIR	     vars[V_NEWS_SPOOL_DIR].cmdline_val.p
  /* Elm style save is obsolete in Pine 3.81 (see saved msg name rule) */   
#define VAR_ELM_STYLE_SAVE           vars[V_ELM_STYLE_SAVE].current_val.p
#define USR_ELM_STYLE_SAVE           vars[V_ELM_STYLE_SAVE].user_val.p
#define GLO_ELM_STYLE_SAVE           vars[V_ELM_STYLE_SAVE].global_val.p
#define FIX_ELM_STYLE_SAVE           vars[V_ELM_STYLE_SAVE].fixed_val.p
#define COM_ELM_STYLE_SAVE           vars[V_ELM_STYLE_SAVE].cmdline_val.p
  /* Header in reply is obsolete in Pine 3.83 (see feature list) */   
#define VAR_HEADER_IN_REPLY          vars[V_HEADER_IN_REPLY].current_val.p
#define USR_HEADER_IN_REPLY          vars[V_HEADER_IN_REPLY].user_val.p
#define GLO_HEADER_IN_REPLY          vars[V_HEADER_IN_REPLY].global_val.p
#define FIX_HEADER_IN_REPLY          vars[V_HEADER_IN_REPLY].fixed_val.p
#define COM_HEADER_IN_REPLY          vars[V_HEADER_IN_REPLY].cmdline_val.p
  /* Feature level is obsolete in Pine 3.83 (see feature list) */   
#define VAR_FEATURE_LEVEL            vars[V_FEATURE_LEVEL].current_val.p
#define USR_FEATURE_LEVEL            vars[V_FEATURE_LEVEL].user_val.p
#define GLO_FEATURE_LEVEL            vars[V_FEATURE_LEVEL].global_val.p
#define FIX_FEATURE_LEVEL            vars[V_FEATURE_LEVEL].fixed_val.p
#define COM_FEATURE_LEVEL            vars[V_FEATURE_LEVEL].cmdline_val.p
  /* Old style reply is obsolete in Pine 3.83 (see feature list) */   
#define VAR_OLD_STYLE_REPLY          vars[V_OLD_STYLE_REPLY].current_val.p
#define USR_OLD_STYLE_REPLY          vars[V_OLD_STYLE_REPLY].user_val.p
#define GLO_OLD_STYLE_REPLY          vars[V_OLD_STYLE_REPLY].global_val.p
#define FIX_OLD_STYLE_REPLY          vars[V_OLD_STYLE_REPLY].fixed_val.p
#define COM_OLD_STYLE_REPLY          vars[V_OLD_STYLE_REPLY].cmdline_val.p
  /* Compose MIME is obsolete in Pine 3.81 */
#define VAR_COMPOSE_MIME             vars[V_COMPOSE_MIME].current_val.p
#define USR_COMPOSE_MIME             vars[V_COMPOSE_MIME].user_val.p
#define GLO_COMPOSE_MIME             vars[V_COMPOSE_MIME].global_val.p
#define FIX_COMPOSE_MIME             vars[V_COMPOSE_MIME].fixed_val.p
#define COM_COMPOSE_MIME             vars[V_COMPOSE_MIME].cmdline_val.p
  /* Show all characters is obsolete in Pine 3.83 */   
#define VAR_SHOW_ALL_CHARACTERS      vars[V_SHOW_ALL_CHARACTERS].current_val.p
#define USR_SHOW_ALL_CHARACTERS      vars[V_SHOW_ALL_CHARACTERS].user_val.p
#define GLO_SHOW_ALL_CHARACTERS      vars[V_SHOW_ALL_CHARACTERS].global_val.p
#define FIX_SHOW_ALL_CHARACTERS      vars[V_SHOW_ALL_CHARACTERS].fixed_val.p
#define COM_SHOW_ALL_CHARACTERS      vars[V_SHOW_ALL_CHARACTERS].cmdline_val.p
  /* Save by sender is obsolete in Pine 3.83 (see saved msg name rule) */   
#define VAR_SAVE_BY_SENDER           vars[V_SAVE_BY_SENDER].current_val.p
#define USR_SAVE_BY_SENDER           vars[V_SAVE_BY_SENDER].user_val.p
#define GLO_SAVE_BY_SENDER           vars[V_SAVE_BY_SENDER].global_val.p
#define FIX_SAVE_BY_SENDER           vars[V_SAVE_BY_SENDER].fixed_val.p
#define COM_SAVE_BY_SENDER           vars[V_SAVE_BY_SENDER].cmdline_val.p
#define VAR_NNTP_SERVER              vars[V_NNTP_SERVER].current_val.l
#define USR_NNTP_SERVER              vars[V_NNTP_SERVER].user_val.l
#define GLO_NNTP_SERVER              vars[V_NNTP_SERVER].global_val.l
#define FIX_NNTP_SERVER              vars[V_NNTP_SERVER].fixed_val.l
#define COM_NNTP_SERVER              vars[V_NNTP_SERVER].cmdline_val.l
#define VAR_UPLOAD_CMD		     vars[V_UPLOAD_CMD].current_val.p
#define USR_UPLOAD_CMD		     vars[V_UPLOAD_CMD].user_val.p
#define GLO_UPLOAD_CMD		     vars[V_UPLOAD_CMD].global_val.p
#define FIX_UPLOAD_CMD		     vars[V_UPLOAD_CMD].fixed_val.p
#define COM_UPLOAD_CMD		     vars[V_UPLOAD_CMD].cmdline_val.p
#define VAR_UPLOAD_CMD_PREFIX	     vars[V_UPLOAD_CMD_PREFIX].current_val.p
#define USR_UPLOAD_CMD_PREFIX	     vars[V_UPLOAD_CMD_PREFIX].user_val.p
#define GLO_UPLOAD_CMD_PREFIX	     vars[V_UPLOAD_CMD_PREFIX].global_val.p
#define FIX_UPLOAD_CMD_PREFIX	     vars[V_UPLOAD_CMD_PREFIX].fixed_val.p
#define COM_UPLOAD_CMD_PREFIX	     vars[V_UPLOAD_CMD_PREFIX].cmdline_val.p
#define VAR_DOWNLOAD_CMD	     vars[V_DOWNLOAD_CMD].current_val.p
#define USR_DOWNLOAD_CMD	     vars[V_DOWNLOAD_CMD].user_val.p
#define GLO_DOWNLOAD_CMD	     vars[V_DOWNLOAD_CMD].global_val.p
#define FIX_DOWNLOAD_CMD	     vars[V_DOWNLOAD_CMD].fixed_val.p
#define COM_DOWNLOAD_CMD	     vars[V_DOWNLOAD_CMD].cmdline_val.p
#define VAR_DOWNLOAD_CMD_PREFIX	     vars[V_DOWNLOAD_CMD_PREFIX].current_val.p
#define USR_DOWNLOAD_CMD_PREFIX	     vars[V_DOWNLOAD_CMD_PREFIX].user_val.p
#define GLO_DOWNLOAD_CMD_PREFIX	     vars[V_DOWNLOAD_CMD_PREFIX].global_val.p
#define FIX_DOWNLOAD_CMD_PREFIX	     vars[V_DOWNLOAD_CMD_PREFIX].fixed_val.p
#define COM_DOWNLOAD_CMD_PREFIX	     vars[V_DOWNLOAD_CMD_PREFIX].cmdline_val.p
#define VAR_GOTO_DEFAULT_RULE	     vars[V_GOTO_DEFAULT_RULE].current_val.p
#define USR_GOTO_DEFAULT_RULE	     vars[V_GOTO_DEFAULT_RULE].user_val.p
#define GLO_GOTO_DEFAULT_RULE	     vars[V_GOTO_DEFAULT_RULE].global_val.p
#define FIX_GOTO_DEFAULT_RULE	     vars[V_GOTO_DEFAULT_RULE].fixed_val.p
#define COM_GOTO_DEFAULT_RULE	     vars[V_GOTO_DEFAULT_RULE].cmdline_val.p
#define VAR_MAILCAP_PATH	     vars[V_MAILCAP_PATH].current_val.p
#define USR_MAILCAP_PATH	     vars[V_MAILCAP_PATH].user_val.p
#define GLO_MAILCAP_PATH	     vars[V_MAILCAP_PATH].global_val.p
#define FIX_MAILCAP_PATH	     vars[V_MAILCAP_PATH].fixed_val.p
#define COM_MAILCAP_PATH	     vars[V_MAILCAP_PATH].cmdline_val.p
#define VAR_MIMETYPE_PATH	     vars[V_MIMETYPE_PATH].current_val.p
#define USR_MIMETYPE_PATH	     vars[V_MIMETYPE_PATH].user_val.p
#define GLO_MIMETYPE_PATH	     vars[V_MIMETYPE_PATH].global_val.p
#define FIX_MIMETYPE_PATH	     vars[V_MIMETYPE_PATH].fixed_val.p
#define COM_MIMETYPE_PATH	     vars[V_MIMETYPE_PATH].cmdline_val.p
#define VAR_TCPOPENTIMEO	     vars[V_TCPOPENTIMEO].current_val.p
#define USR_TCPOPENTIMEO	     vars[V_TCPOPENTIMEO].user_val.p
#define GLO_TCPOPENTIMEO	     vars[V_TCPOPENTIMEO].global_val.p
#define FIX_TCPOPENTIMEO	     vars[V_TCPOPENTIMEO].fixed_val.p
#define COM_TCPOPENTIMEO	     vars[V_TCPOPENTIMEO].cmdline_val.p
#define VAR_RSHOPENTIMEO	     vars[V_RSHOPENTIMEO].current_val.p
#define USR_RSHOPENTIMEO	     vars[V_RSHOPENTIMEO].user_val.p
#define GLO_RSHOPENTIMEO	     vars[V_RSHOPENTIMEO].global_val.p
#define FIX_RSHOPENTIMEO	     vars[V_RSHOPENTIMEO].fixed_val.p
#define COM_RSHOPENTIMEO	     vars[V_RSHOPENTIMEO].cmdline_val.p
#define VAR_NEW_VER_QUELL	     vars[V_NEW_VER_QUELL].current_val.p
#define USR_NEW_VER_QUELL	     vars[V_NEW_VER_QUELL].user_val.p
#define GLO_NEW_VER_QUELL	     vars[V_NEW_VER_QUELL].global_val.p
#define FIX_NEW_VER_QUELL	     vars[V_NEW_VER_QUELL].fixed_val.p
#define COM_NEW_VER_QUELL	     vars[V_NEW_VER_QUELL].cmdline_val.p

#ifdef	NEWBB
#define VAR_NNTP_NEW_GROUP_TIME      vars[V_NNTP_NEW_GROUP_TIME].current_val.p
#define USR_NNTP_NEW_GROUP_TIME      vars[V_NNTP_NEW_GROUP_TIME].user_val.p
#define GLO_NNTP_NEW_GROUP_TIME      vars[V_NNTP_NEW_GROUP_TIME].global_val.p
#define FIX_NNTP_NEW_GROUP_TIME      vars[V_NNTP_NEW_GROUP_TIME].fixed_val.p
#define COM_NNTP_NEW_GROUP_TIME      vars[V_NNTP_NEW_GROUP_TIME].cmdline_val.p
#endif

#if defined(DOS) || defined(OS2)
#define VAR_FOLDER_EXTENSION	     vars[V_FOLDER_EXTENSION].current_val.p
#define USR_FOLDER_EXTENSION	     vars[V_FOLDER_EXTENSION].user_val.p
#define GLO_FOLDER_EXTENSION	     vars[V_FOLDER_EXTENSION].global_val.p
#define FIX_FOLDER_EXTENSION	     vars[V_FOLDER_EXTENSION].fixed_val.p
#define COM_FOLDER_EXTENSION	     vars[V_FOLDER_EXTENSION].cmdline_val.p
#define VAR_NORM_FORE_COLOR	     vars[V_NORM_FORE_COLOR].current_val.p
#define USR_NORM_FORE_COLOR	     vars[V_NORM_FORE_COLOR].user_val.p
#define GLO_NORM_FORE_COLOR	     vars[V_NORM_FORE_COLOR].global_val.p
#define FIX_NORM_FORE_COLOR	     vars[V_NORM_FORE_COLOR].fixed_val.p
#define COM_NORM_FORE_COLOR	     vars[V_NORM_FORE_COLOR].cmdline_val.p
#define VAR_NORM_BACK_COLOR	     vars[V_NORM_BACK_COLOR].current_val.p
#define USR_NORM_BACK_COLOR	     vars[V_NORM_BACK_COLOR].user_val.p
#define GLO_NORM_BACK_COLOR	     vars[V_NORM_BACK_COLOR].global_val.p
#define FIX_NORM_BACK_COLOR	     vars[V_NORM_BACK_COLOR].fixed_val.p
#define COM_NORM_BACK_COLOR	     vars[V_NORM_BACK_COLOR].cmdline_val.p
#define VAR_REV_FORE_COLOR	     vars[V_REV_FORE_COLOR].current_val.p
#define USR_REV_FORE_COLOR	     vars[V_REV_FORE_COLOR].user_val.p
#define GLO_REV_FORE_COLOR	     vars[V_REV_FORE_COLOR].global_val.p
#define FIX_REV_FORE_COLOR	     vars[V_REV_FORE_COLOR].fixed_val.p
#define COM_REV_FORE_COLOR	     vars[V_REV_FORE_COLOR].cmdline_val.p
#define VAR_REV_BACK_COLOR	     vars[V_REV_BACK_COLOR].current_val.p
#define USR_REV_BACK_COLOR	     vars[V_REV_BACK_COLOR].user_val.p
#define GLO_REV_BACK_COLOR	     vars[V_REV_BACK_COLOR].global_val.p
#define FIX_REV_BACK_COLOR	     vars[V_REV_BACK_COLOR].fixed_val.p
#define COM_REV_BACK_COLOR	     vars[V_REV_BACK_COLOR].cmdline_val.p
#ifdef _WINDOWS
#define VAR_FONT_NAME		     vars[V_FONT_NAME].current_val.p
#define USR_FONT_NAME		     vars[V_FONT_NAME].user_val.p
#define GLO_FONT_NAME		     vars[V_FONT_NAME].global_val.p
#define FIX_FONT_NAME		     vars[V_FONT_NAME].fixed_val.p
#define COM_FONT_NAME		     vars[V_FONT_NAME].cmdline_val.p
#define VAR_FONT_SIZE		     vars[V_FONT_SIZE].current_val.p
#define USR_FONT_SIZE		     vars[V_FONT_SIZE].user_val.p
#define GLO_FONT_SIZE		     vars[V_FONT_SIZE].global_val.p
#define FIX_FONT_SIZE		     vars[V_FONT_SIZE].fixed_val.p
#define COM_FONT_SIZE		     vars[V_FONT_SIZE].cmdline_val.p
#define VAR_FONT_STYLE		     vars[V_FONT_STYLE].current_val.p
#define USR_FONT_STYLE		     vars[V_FONT_STYLE].user_val.p
#define GLO_FONT_STYLE		     vars[V_FONT_STYLE].global_val.p
#define FIX_FONT_STYLE		     vars[V_FONT_STYLE].fixed_val.p
#define COM_FONT_STYLE		     vars[V_FONT_STYLE].cmdline_val.p
#define VAR_PRINT_FONT_NAME	     vars[V_PRINT_FONT_NAME].current_val.p
#define USR_PRINT_FONT_NAME	     vars[V_PRINT_FONT_NAME].user_val.p
#define GLO_PRINT_FONT_NAME	     vars[V_PRINT_FONT_NAME].global_val.p
#define FIX_PRINT_FONT_NAME	     vars[V_PRINT_FONT_NAME].fixed_val.p
#define COM_PRINT_FONT_NAME	     vars[V_PRINT_FONT_NAME].cmdline_val.p
#define VAR_PRINT_FONT_SIZE	     vars[V_PRINT_FONT_SIZE].current_val.p
#define USR_PRINT_FONT_SIZE	     vars[V_PRINT_FONT_SIZE].user_val.p
#define GLO_PRINT_FONT_SIZE	     vars[V_PRINT_FONT_SIZE].global_val.p
#define FIX_PRINT_FONT_SIZE	     vars[V_PRINT_FONT_SIZE].fixed_val.p
#define COM_PRINT_FONT_SIZE	     vars[V_PRINT_FONT_SIZE].cmdline_val.p
#define VAR_PRINT_FONT_STYLE	     vars[V_PRINT_FONT_STYLE].current_val.p
#define USR_PRINT_FONT_STYLE	     vars[V_PRINT_FONT_STYLE].user_val.p
#define GLO_PRINT_FONT_STYLE	     vars[V_PRINT_FONT_STYLE].global_val.p
#define FIX_PRINT_FONT_STYLE	     vars[V_PRINT_FONT_STYLE].fixed_val.p
#define COM_PRINT_FONT_STYLE	     vars[V_PRINT_FONT_STYLE].cmdline_val.p
#define VAR_WINDOW_POSITION	     vars[V_WINDOW_POSITION].current_val.p
#define USR_WINDOW_POSITION	     vars[V_WINDOW_POSITION].user_val.p
#define GLO_WINDOW_POSITION	     vars[V_WINDOW_POSITION].global_val.p
#define FIX_WINDOW_POSITION	     vars[V_WINDOW_POSITION].fixed_val.p
#define COM_WINDOW_POSITION	     vars[V_WINDOW_POSITION].cmdline_val.p
#endif	/* _WINDOWS */
#endif	/* DOS */


#define LARGEST_BITMAP 88
/* Feature list support (can have up to LARGEST_BITMAP features) */
/* Is feature "feature" turned on? */
#define F_ON(feature,ps)   (bitnset((feature),(ps)->feature_list))
#define F_OFF(feature,ps)  (!F_ON(feature,ps))
#define F_TURN_ON(feature,ps)   (setbitn((feature),(ps)->feature_list))
#define F_TURN_OFF(feature,ps)  (clrbitn((feature),(ps)->feature_list))
/* turn off or on depending on value */
#define F_SET(feature,ps,value) ((value) ? F_TURN_ON((feature),(ps))       \
					 : F_TURN_OFF((feature),(ps)))

/* list of feature numbers (which bit goes with which feature) */
#define F_OLD_GROWTH                0
#define F_ENABLE_FULL_HDR           1
#define F_ENABLE_PIPE               2
#define F_ENABLE_TAB_COMPLETE       3
#define F_QUIT_WO_CONFIRM           4
#define F_ENABLE_JUMP               5
#define F_ENABLE_ALT_ED             6
#define F_ENABLE_BOUNCE             7
#define F_ENABLE_AGG_OPS	    8
#define F_ENABLE_FLAG               9
#define F_CAN_SUSPEND              10
#define F_EXPANDED_FOLDERS         11
#define F_USE_FK                   12
#define F_INCLUDE_HEADER           13
#define F_SIG_AT_BOTTOM            14
#define F_DEL_SKIPS_DEL            15
#define F_AUTO_EXPUNGE             16
#define F_AUTO_READ_MSGS           17
#define F_READ_IN_NEWSRC_ORDER     18
#define F_SELECT_WO_CONFIRM        19
#define F_USE_CURRENT_DIR          20
#define F_SAVE_WONT_DELETE         21
#define F_SAVE_ADVANCES            22
#define F_FORCE_LOW_SPEED          23
#define F_ALT_ED_NOW		   24
#define	F_SHOW_DELAY_CUE	   25
#define	F_AUTO_OPEN_NEXT_UNREAD	   26
#define	F_SELECTED_SHOWN_BOLD	   27
#define	F_QUOTE_ALL_FROMS	   28
#define	F_AUTO_INCLUDE_IN_REPLY	   29
#define	F_DISABLE_CONFIG_SCREEN    30
#define	F_DISABLE_PASSWORD_CMD     31
#define	F_DISABLE_UPDATE_CMD       32
#define	F_DISABLE_KBLOCK_CMD       33
#define	F_DISABLE_SIGEDIT_CMD	   34
#define	F_DISABLE_DFLT_IN_BUG_RPT  35
#define	F_ATTACHMENTS_IN_REPLY	   36
#define	F_ENABLE_INCOMING	   37
#define	F_NO_NEWS_VALIDATION	   38
#define	F_EXPANDED_ADDRBOOKS	   39
#define	F_QUELL_LOCAL_LOOKUP	   40
#define	F_COMPOSE_TO_NEWSGRP	   41
#define	F_PRESERVE_START_STOP      42
#define	F_COMPOSE_REJECTS_UNQUAL   43
#define	F_FAKE_NEW_IN_NEWS	   44
#define	F_SUSPEND_SPAWNS	   45
#define	F_ENABLE_8BIT		   46
#define	F_COMPOSE_MAPS_DEL	   47
#define	F_ENABLE_8BIT_NNTP	   48
#define	F_ENABLE_MOUSE		   49
#define	F_SHOW_CURSOR		   50
#define	F_PASS_CONTROL_CHARS	   51
#define	F_VERT_FOLDER_LIST	   52
#define	F_AUTO_REPLY_TO		   53
#define	F_VERBOSE_POST		   54
#define	F_FCC_ON_BOUNCE		   55
#define	F_USE_SENDER_NOT_X	   56
#define	F_BLANK_KEYMENU		   57
#define	F_CUSTOM_PRINT		   58
#define	F_DEL_FROM_DOT		   59
#define	F_AUTO_ZOOM		   60
#define	F_AUTO_UNZOOM		   61
#define	F_PRINT_INDEX		   62
#define	F_ALLOW_TALK		   63
#define	F_AGG_PRINT_FF		   64
#define	F_ENABLE_DOT_FILES	   65
#define	F_ENABLE_DOT_FOLDERS	   66
#define	F_FIRST_SEND_FILTER_DFLT   67
#define	F_ALWAYS_LAST_FLDR_DFLT    68
#define	F_TAB_TO_NEW		   69
#define	F_QUELL_DEAD_LETTER	   70
#define	F_QUELL_BEEPS		   71
#define	F_QUELL_LOCK_FAILURE_MSGS  72
#define	F_ENABLE_SPACE_AS_TAB	   73
#define	F_ENABLE_TAB_DELETES	   74
#define	F_FLAG_SCREEN_DFLT	   75
#define	F_ENABLE_XTERM_NEWMAIL	   76
#define	F_EXPANDED_DISTLISTS	   77
#define	F_AGG_SEQ_COPY		   78
#define	F_DISABLE_ALARM		   79
#define	F_FROM_DELIM_IN_PRINT	   80
#define	F_BACKGROUND_POST	   81
#define	F_ALLOW_GOTO		   82
#define F_LAST_FEATURE		   82		 /* RESET WITH NEW FEATURES */

#if (F_LAST_FEATURE > (LARGEST_BITMAP - 1))
   Whoa!  Too many features!
#endif


/*======================================================================
       Macros for debug printfs 
   n is debugging level:
       1 logs only highest level events and errors
       2 logs events like file writes
       3
       4 logs each command
       5
       6 
       7 logs details of command execution (7 is highest to run any production)
         allows core dumps without cleaning up terminal modes
       8
       9 logs gross details of command execution
    
    This is sort of dumb.  The first argument in x is always debugfile, and
    we're sort of assuming that to be the case below.  However, we don't
    want to go back and change all of the dprint calls now.
  ====*/
#ifdef DEBUG
#define   dprint(n,x) {							\
	       if(debugfile && debug >= (n) && do_debug(debugfile))	\
		  fprintf x ;						\
	      }
#else
#define   dprint(n,x)
#endif


/*
 * Status message types
 */
#define	SM_DING		0x01			/* ring bell when displayed  */
#define	SM_ASYNC	0x02			/* display any time	     */
#define	SM_ORDER	0x04			/* ordered, flush on prompt  */
#define	SM_MODAL	0x08			/* display, wait for user    */
#define	SM_INFO		0x10			/* Handy, but discardable    */

/*
 * Which header fields should format_envelope output?
 */
#define	FE_FROM		0x0001
#define	FE_SENDER	0x0002
#define	FE_DATE		0x0004
#define	FE_TO		0x0008
#define	FE_CC		0x0010
#define	FE_BCC		0x0020
#define	FE_NEWSGROUPS	0x0040
#define	FE_SUBJECT	0x0080
#define	FE_MESSAGEID	0x0100
#define	FE_REPLYTO	0x0200
#define	FE_INREPLYTO	0x0400
#define	FE_DEFAULT	(FE_FROM | FE_DATE | FE_TO | FE_CC | FE_BCC \
			 | FE_NEWSGROUPS | FE_SUBJECT | FE_REPLYTO)

#define ALL_EXCEPT	"all-except"


/*
 * Flags to indicate how much index border to paint
 */
#define	INDX_CLEAR	0x01
#define	INDX_HEADER	0x02
#define	INDX_FOOTER	0x04


/*
 * Flags to indicate context (i.e., folder collection) use
 */
#define	CNTXT_PRIME	0x0001			/* primary collection	    */
#define	CNTXT_SECOND	0x0002			/* secondary collection     */
#define	CNTXT_NEWS	0x0004			/* news group collection    */
#define	CNTXT_PSEUDO	0x0008			/* fake folder entry exists */
#define	CNTXT_INCMNG	0x0010			/* inbox collection	    */
#define	CNTXT_SAVEDFLT	0x0020			/* default save collection  */
#define	CNTXT_PARTFIND	0x0040			/* partial find done        */
#define	CNTXT_NOFIND	0x0080			/* no find done in context  */
#define	CNTXT_FINDALL   0x0100                   /* Do a find_all on context */
#ifdef	NEWBB
#define	CNTXT_NEWBB     0x0200                   /* New bboards - NNTP only  */
#endif


/*
 * Flags indicating folder collection type
 */
#define	FTYPE_LOCAL	0x01			/* Local folders	  */
#define	FTYPE_REMOTE	0x02			/* Remote folders	  */
#define	FTYPE_SHARED	0x04			/* Shared folders	  */
#define FTYPE_BBOARD    0x08			/* Bulletin Board folders */
#define	FTYPE_OLDTECH	0x10			/* Not accessed via IMAP  */
#define	FTYPE_ANON	0x20			/* anonymous access       */


/*
 * Useful def's to specify interesting flags
 */
#define	F_SEEN		0x00000001
#define	F_UNSEEN	0x00000002
#define	F_DEL		0x00000004
#define	F_UNDEL		0x00000008
#define	F_FLAG		0x00000010
#define	F_UNFLAG	0x00000020
#define	F_ANS		0x00000040
#define	F_UNANS		0x00000080
#define	F_RECENT	0x00000100
#define	F_UNRECENT	0x00000200
#define	F_OR_SEEN	0x00000400
#define	F_OR_UNSEEN	0x00000800
#define	F_OR_DEL	0x00001000
#define	F_OR_UNDEL	0x00002000
#define	F_OR_FLAG	0x00004000
#define	F_OR_UNFLAG	0x00008000
#define	F_OR_ANS	0x00010000
#define	F_OR_UNANS	0x00020000
#define	F_OR_RECENT	0x00040000
#define	F_OR_UNRECENT	0x00080000


/*
 * Save message rules.  if these grow, widen pine
 * struct's save_msg_rule...
 */
#define	MSG_RULE_DEFLT			0x00
#define	MSG_RULE_SENDER			0x01
#define	MSG_RULE_FROM			0x02
#define	MSG_RULE_RECIP			0x03
#define	MSG_RULE_LAST			0x04
#define	MSG_RULE_NICK_SENDER		0x05
#define	MSG_RULE_NICK_FROM		0x06
#define	MSG_RULE_NICK_RECIP		0x07
#define	MSG_RULE_NICK_SENDER_DEF	0x08
#define	MSG_RULE_NICK_FROM_DEF		0x09
#define	MSG_RULE_NICK_RECIP_DEF		0x0A
#define	MSG_RULE_FCC_SENDER		0x0B
#define	MSG_RULE_FCC_FROM		0x0C
#define	MSG_RULE_FCC_RECIP		0x0D
#define	MSG_RULE_FCC_SENDER_DEF		0x0E
#define	MSG_RULE_FCC_FROM_DEF		0x0F
#define	MSG_RULE_FCC_RECIP_DEF		0x10

/*
 * Fcc rules.  if these grow, widen pine
 * struct's fcc_rule...
 */
#define	FCC_RULE_DEFLT		0x00
#define	FCC_RULE_RECIP		0x01
#define	FCC_RULE_LAST		0x02
#define	FCC_RULE_NICK		0x03
#define	FCC_RULE_NICK_RECIP	0x04
#define	FCC_RULE_CURRENT	0x05

/*
 * Addrbook sorting rules.  if these grow, widen pine
 * struct's ab_sort_rule...
 */
#define	AB_SORT_RULE_FULL_LISTS		0x00
#define	AB_SORT_RULE_FULL  		0x01
#define	AB_SORT_RULE_NICK_LISTS         0x02
#define	AB_SORT_RULE_NICK 		0x03
#define	AB_SORT_RULE_NONE		0x04

/*
 * Goto default rules.
 */
#define	GOTO_INBOX_RECENT_CLCTN		0x00
#define	GOTO_INBOX_FIRST_CLCTN		0x01
#define	GOTO_LAST_FLDR			0x02

/*
 * These are def's to help manage local, private flags pine uses
 * to maintain it's mapping table (see MSGNO_S def).  The local flags
 * are actually stored in spare bits in c-client's pre-message
 * MESSAGECACHE struct.  But they're private, you ask.  Since the flags
 * are tied to the actual message (independent of the mapping), storing
 * them in the c-client means we don't have to worry about them during
 * sorting and such.  See {set,get}_lflags for more on local flags.
 */
#define	MN_NONE		0x0000			/* No Pine specific flags  */
#define	MN_HIDE		0x0001			/* Pine specific hidden    */
#define	MN_EXLD		0x0002			/* Pine specific excluded  */
#define	MN_SLCT		0x0004			/* Pine specific selected  */


/*
 * Macros to aid hack to turn off talk permission.
 * Bit 020 is usually used to turn off talk permission, we turn off
 * 002 also for good measure, since some mesg commands seem to do that.
 */
#define	TALK_BIT		020		/* mode bits */
#define	GM_BIT			002
#define	TMD_CLEAR		0		/* functions */
#define	TMD_SET			1
#define	TMD_RESET		2
#define	allow_talk(p)		tty_chmod((p), TALK_BIT, TMD_SET)
#define	disallow_talk(p)	tty_chmod((p), TALK_BIT|GM_BIT, TMD_CLEAR)


/*
 * Macros to help set numeric pinerc variables.  Defined here are the 
 * allowed min and max values as well as the name of the var as it
 * should be displayed in error messages.
 */
#define	SVAR_OVERLAP(ps, n, e)	strtoval((ps)->VAR_OVERLAP,		  \
					 &(n), 0, 20, (e),		  \
					 "Viewer-overlap")
#define	SVAR_MARGIN(ps, n, e)	strtoval((ps)->VAR_MARGIN,		  \
					 &(n), 0, 20, (e),		  \
					 "Scroll-margin")
#define	SVAR_FILLCOL(ps, n, e)	strtoval((ps)->VAR_FILLCOL,		  \
					 &(n), 0, MAX_FILLCOL, (e),	  \
					 "Composer-wrap-column")
#define	SVAR_MSGDLAY(ps, n, e)	strtoval((ps)->VAR_STATUS_MSG_DELAY,	  \
					 &(n), 0, 30, (e),		  \
					"Status-message-delay")
#define	SVAR_MAILCHK(ps, n, e)	strtoval((ps)->VAR_MAILCHECK,		  \
					 &(n), 14, 1, (e),		  \
					"Mail-check-interval")

/*
 * [Re]Define signal functions as needed...
 */
#ifdef	POSIX_SIGNALS
/*
 * Redefine signal call to our wrapper of POSIX sigaction
 * NOTE: our_unblock gets defined in signals.c
 */
#define	signal(SIG,ACT)		posix_signal(SIG,ACT)
#define	pine_sigunblock(SIG)	posix_sigunblock(SIG)
#else
#ifdef	SYSV_SIGNALS
/*
 * Redefine signal calls to SYSV style call.
 */
#define	signal(SIG,ACT)		sigset(SIG,ACT)
#define	pine_sigunblock(SIG)	sigrelse(SIG)
#else
/*
 * Good ol' BSD signals.  No unblock required.
 */
#define	pine_sigunblock(SIG)
#endif /* SYSV_SIGNALS */
#endif /* POSIX_SIGNALS */


/*======================================================================
            Various structures that Pine uses
 ====*/

struct ttyo {
    int	screen_rows,
	screen_cols,
	header_rows,	/* number of rows for titlebar and whitespace */
	footer_rows;	/* number of rows for status and keymenu      */
};

/*
 * HEADER_ROWS is always 2.  1 for the titlebar and 1 for the
 * blank line after the titlebar.  We should probably make it go down
 * to 0 when the screen shrinks but instead we're just figuring out
 * if there is enough room by looking at screen_rows.
 * FOOTER_ROWS is either 3 or 1.  Normally it is 3, 2 for the keymenu plus 1
 * for the status line.  If the NoKeyMenu command has been given, then it is 1.
 */
#define HEADER_ROWS(X) ((X)->ttyo->header_rows)
#define FOOTER_ROWS(X) ((X)->ttyo->footer_rows)


/* (0,0) is upper left */
typedef struct screen_position {
    int row;
    int col;
} Pos;

typedef enum {SortDate, SortArrival, SortFrom, SortSubject, SortSubject2,
                SortTo, SortCc, SortSize, EndofList}   SortOrder;


/*
 * This is *the* struct that keeps track of the pine message number to
 * raw c-client sequence number mappings.  The mapping is necessary
 * because pine may re-sort or even hide (exclude) c-client numbers
 * from the displayed list of messages.  See mailindx.c:msgno_* and
 * the mn_* macros above for how this things gets used.  See
 * mailcmd.c:pseudo_selected for an explanation of the funny business
 * going on with the "hilited" field...
 */
typedef struct msg_nos {
    long      *select,				/* selected message array  */
	       sel_cur,				/* current interesting msg */
	       sel_cnt,				/* its size		   */
	       sel_size,			/* its size		   */
              *sort,				/* sorted array of msgno's */
               sort_size,			/* its size		   */
	       max_msgno,			/* total messages	   */
	       hilited;				/* holder for "current" msg*/
    SortOrder  sort_order;			/* list's current sort     */
    unsigned   reverse_sort:1;			/* whether that's reversed */
    long       flagged_hid,			/* hidden count		   */
	       flagged_exld,			/* excluded count	   */
	       flagged_tmp;			/* tmp flagged count	   */
} MSGNO_S;

/*
 * Flags for the pipe command routines...
 */
#define	PIPE_WRITE	0x0001			/* set up pipe for reading */
#define	PIPE_READ	0x0002			/* set up pipe for reading */
#define	PIPE_NOSHELL	0x0004			/* don't exec in shell     */
#define	PIPE_USER	0x0008			/* user mode		   */
#define	PIPE_STDERR	0x0010			/* stderr to child output  */
#define	PIPE_PROT	0x0020			/* protected mode	   */
#define	PIPE_RESET	0x0040			/* reset terminal mode     */
#define	PIPE_DESC	0x0080			/* no stdio desc wrapping  */
#define	PIPE_SILENT	0x0100			/* no screen clear, etc	   */

/*
 * stucture required for the pipe commands...
 */
typedef struct pipe_s {
    int      pid,				/* child's process id       */
	     mode;				/* mode flags used to open  */
    SigType  (*hsig)(),				/* previously installed...  */
	     (*isig)(),				/* handlers		    */
	     (*qsig)();
    union {
	FILE *f;
	int   d;
    }	     in;				/* input data handle	    */
    union {
	FILE *f;
	int   d;
    }	     out;				/* output data handle	    */
    char   **argv,				/* any necessary args	    */
	    *args,
	    *tmp;				/* pointer to stuff	    */
#ifdef	_WINDOWS
    char    *command;				/* command to execute */
#endif
} PIPE_S;


/*------------------------------
    Stucture to keep track of the various folder collections being
    dealt with.
  ----*/
typedef struct context {
    char           *context;			/* context string	   */
    char           *label[4];			/* description lines	   */
    char           *nickname;			/* user provided nickname  */
    char            last_folder[MAXFOLDER+1];	/* last folder used        */
    void           *folders;			/* folder data             */
    unsigned short  type;			/* type of collection      */
    unsigned short  use;			/* use flags for context   */
    unsigned short  num;			/* context number in list  */
    int		    d_line;			/* display line for labels */
    struct context *next;			/* next context struct	   */
} CONTEXT_S;


/*------------------------------
   Used for displaying as well as
   keeping track of folders. 
   Currently about 25 bytes.
  ----*/
typedef struct folder {
    char     prefix[8];				/* news prefix?		   */
    unsigned char   name_len;			/* name length		   */
    unsigned short msg_count;			/* Up to 65,000 messages   */
    unsigned short unread_count;
    unsigned short d_col;
    unsigned short d_line;
    char     *nickname;				/* folder's short name     */
    char     name[1];				/* folder's name           */
} FOLDER_S;


/*------------------------------
   Used for index display
   formatting.
  ----*/
typedef enum {iStatus, iFStatus, iMessNo, iDate, iFromTo, iFrom, iTo, iSender,
		iSize, iDescripSize, iSubject, iNothing} IndexColType;
typedef enum {AllAuto, Fixed, Percent, WeCalculate, Special} WidthType;
typedef enum {Left, Right} ColAdj;

typedef struct col_description {
    IndexColType ctype;
    WidthType    wtype;
    int		 req_width;
    int		 width;
    char	*string;
    int		 actual_length;
    ColAdj	 adjustment;
} INDEX_COL_S;


typedef long MsgNo;

struct variable {
    char *name;
    unsigned  is_obsolete:1;	/* variable read in, not written unless set */
    unsigned  is_used:1;	/* Some variables are disabled              */
    unsigned  been_written:1;
    unsigned  is_user:1;
    unsigned  is_global:1;
    unsigned  is_list:1;	/* flag indicating variable is a list       */
    unsigned  is_fixed:1;	/* sys mgr has fixed this variable          */
    char     *descrip;		/* description                              */
    union {
	char *p;		/* pointer to single string value           */
	char **l;		/* pointer to list of string values         */
    } current_val;
    union {
	char *p;		/* pointer to single string value           */
	char **l;		/* pointer to list of string values         */
    } user_val;			/* from pinerc                              */
    union {
	char *p;		/* pointer to single string value           */
	char **l;		/* pointer to list of string values         */
    } global_val;		/* from default or pine.conf                */
    union {
	char *p;		/* pointer to single string value           */
	char **l;		/* pointer to list of string values         */
    } fixed_val;		/* fixed value assigned in pine.conf.fixed  */
    union {
	char *p;		/* pointer to single string value           */
	char **l;		/* pointer to list of string values         */
    } cmdline_val;		/* user typed as cmdline arg                */
};



/*
 * Generic name/value pair structure
 */
typedef struct nameval {
    char *name;			/* the name that goes in the config file */
    int   value;		/* the internal bit number */
} NAMEVAL_S;			/* available features in init.c:feat_list */


typedef struct attachment {
    char           *description;
    BODY           *body;
    unsigned int    can_display:2;
    unsigned int    use_external_viewer:1;
    unsigned int    shown:1;
    char	   *number;
    char            size[25];
} ATTACH_S;

/*
 * Valid flags for can_display field in attachment struct...
 */
#define	CD_NOCANDO	0	/* Can't display this type     */
#define	CD_GOFORIT	1	/* Can display this type       */
#define	CD_DEFERRED	2	/* haven't queried mailcap yet */


typedef struct header_line {
    unsigned long id;				/* header line's uid */
    char     line[1];				/* header line name  */
} HLINE_S;




/*------
   A key menu has two ways to turn on and off individual items in the menu.
   If there is a null entry in the key_menu structure for that key, then
   it is off.  Also, if the passed bitmap has a zero in the position for
   that key, then it is off.  This means you can usually set all of the
   bitmaps and only turn them off if you want to kill a key that is normally
   there otherwise.
   Each key_menu is an array of keys with a multiple of 12 number of keys.
  ------*/
#define BM_SIZE (LARGEST_BITMAP / 8)
#define BM_MENUS (4)  /* could be as large as (LARGEST_BITMAP / 12) */
#define _BITCHAR(bit) ((bit) / 8)
#define _BITBIT(bit) (1 << ((bit) % 8))
typedef unsigned char bitmap_t[BM_SIZE];
/* is bit set? */
#define bitnset(bit,map) (((map)[_BITCHAR(bit)] & _BITBIT(bit)) ? 1 : 0)
/* set bit */
#define setbitn(bit,map) ((map)[_BITCHAR(bit)] |= _BITBIT(bit))
/* clear bit */
#define clrbitn(bit,map) ((map)[_BITCHAR(bit)] &= ~_BITBIT(bit))
/* clear entire bitmap */
#define clrbitmap(map)   memset((void *)(map), 0, (size_t)BM_SIZE)
/* set entire bitmap */
#define setbitmap(map)   memset((void *)(map), 0xff, (size_t)BM_SIZE)
/*------
  Argument to draw_keymenu().  These are to identify which of the possibly
  multiple sets of twelve keys should be shown in the keymenu.  That is,
  a keymenu may have 24 or 36 keys, so that there are 2 or 3 different
  screens of key menus for that keymenu.  FirstMenu means to use the
  first twelve, NextTwelve uses the one after the previous one, SameTwelve
  uses the same one, AParticularTwelve uses the value in the "which"
  argument to draw_keymenu().
  ------*/
typedef enum {FirstMenu, NextTwelve, SameTwelve, AParticularTwelve} OtherMenu;


/*
 * Placeholders for keymenu tags used in some ports  (well, only in the 
 * windows port for now) to turn on commands in the Menu Bar.
 */
#ifndef	KS_OSDATAVAR
#define KS_OSDATAVAR
#define	KS_OSDATAGET(X)
#define	KS_OSDATASET(X, Y)

#define KS_NONE
#define KS_VIEW
#define KS_EXPUNGE
#define KS_ZOOM
#define KS_SORT
#define KS_HDRMODE
#define KS_MAINMENU
#define KS_FLDRLIST
#define KS_FLDRINDEX
#define KS_COMPOSER
#define KS_PREVPAGE
#define KS_PREVMSG
#define KS_NEXTMSG
#define KS_ADDRBOOK
#define KS_WHEREIS
#define KS_PRINT
#define KS_REPLY
#define KS_FORWARD
#define KS_BOUNCE
#define KS_DELETE
#define KS_UNDELETE
#define KS_FLAG
#define KS_SAVE
#define KS_EXPORT
#define KS_TAKEADDR
#define KS_SELECT
#define	KS_SELECTCUR
#define KS_APPLY
#define KS_POSTPONE
#define KS_SEND
#define KS_CANCEL
#define KS_ATTACH
#define KS_TOADDRBOOK
#define KS_READFILE
#define KS_JUSTIFY
#define KS_ALTEDITOR
#define KS_GENERALHELP
#define KS_SCREENHELP
#define KS_EXIT
#define KS_NEXTPAGE
#define KS_SAVEFILE
#define KS_CURPOSITION
#define KS_GOTOFLDR
#define KS_JUMPTOMSG
#define KS_RICHHDR
#define KS_EXITMODE
#define KS_REVIEW
#define KS_UNDO
#define KS_KEYMENU
#define KS_OPTIONS
#endif

/*
 * In the next iteration we want to move column out of this structure since
 * it is the only dynamic data here.  That way we should be able to
 * consolidate some of the static data that initializes the keymenus.
 */
struct key {
    char  *name;			/* the short name */
    char  *label;			/* the descriptive label */
    KS_OSDATAVAR			/* slot for port-specific data */
    short  column;			/* menu col after formatting */
};

struct key_menu {
    unsigned char how_many;		/* how many separate sets of 12      */
    unsigned char which;		/* which of the sets are we using    */
    short         width[BM_MENUS]; 	/* this ought to be of size how_many */
    struct key	 *keys;			/* array of how_many*12 size structs */
    bitmap_t      bitmap;
};

/*
 * Macro to simplify instantiation of key_menu structs from key structs
 */
#define	INST_KEY_MENU(X, Y)	static struct key_menu X = \
						{sizeof(Y)/(sizeof(Y[0])*12), \
						0, 0, 0, 0, 0, Y}

/*
 * used to store system derived user info
 */
typedef struct user_info {
    char *homedir;
    char *login;
    char *fullname;
} USER_S;

typedef int (*percent_done_t)();    /* returns %done for progress status msg */

/* used to fake alarm syscall on systems without it */
#ifndef	ALARM_BLIP
#define ALARM_BLIP()
#endif


/*
 * Printing control structure
 */
typedef struct print_ctrl {
#ifndef	DOS
    PIPE_S	*pipe;		/* control struct for pipe to write text */
    FILE	*fp;		/* file pointer to write printed text into */
    char	*result;	/* file containing print command's output */
#endif
#ifdef	OS2
    int		ispipe;
#endif
    int		err;		/* bit indicating something went awry */
} PRINT_S;


/*
 * Child posting control structure
 */
typedef struct post_s {
    int		pid;		/* proc id of child doing posting */
    int		status;		/* child's exit status */
    char       *fcc;		/* fcc we may have copied */
} POST_S;




/*----------------------------------------------------------------------
   This structure sort of takes the place of global variables or perhaps
is the global variable.  (It can be accessed globally as ps_global.  One
advantage to this is that as soon as you see a reference to the structure
you know it is a global variable. 
   In general it is treated as global by the lower level and utility
routines, but it is not treated so by the main screen driving routines.
Each of them receives it as an argument and then sets ps_global to the
argument they received.  This is sort of with the thought that things
might be coupled more loosely one day and that Pine might run where there
is more than one window and more than one instance.  But we haven't kept
up with this convention very well.
 ----*/
  
struct pine {
    void       (*next_screen)();	/* See loop at end of main() for how */
    void       (*prev_screen)();	/* these are used...		     */
    void       (*redrawer)();		/* NULL means stay in current screen */

    CONTEXT_S   *context_list;		/* list of user defined contexts */
    CONTEXT_S   *context_current;	/* default open context          */
    CONTEXT_S   *context_last;		/* most recently open context    */

    char         inbox_name[MAXFOLDER+1];
    MAILSTREAM  *inbox_stream;		/* these used when current folder */
    long         inbox_new_mail_count;	/* is *not* inbox...		  */
    long         inbox_expunge_count;
    int          inbox_changed;
    MSGNO_S	*inbox_msgmap;		/* pointer to inbox mapping struct  */
    
    MAILSTREAM  *mail_stream;		/* c-client's current folder stream */

    MSGNO_S	 *msgmap;		/* message num mapping into stream  */

    long         last_msgno_flagged;
    long         new_mail_count;
    long         expunge_count;
    int          mail_box_changed;
    int          sort_is_deferred;
    int          unsorted_newmail;
    char         cur_folder[MAXPATH+1];
    ATTACH_S    *atmts;
    int          atmts_allocated;
    INDEX_COL_S *index_disp_format;

    char        *folders_dir;
    unsigned     mangled_footer:1; 	/* footer needs repainting */
    unsigned     mangled_header:1;	/* header needs repainting */
    unsigned     mangled_body:1;	/* body of screen needs repainting */
    unsigned     mangled_screen:1;	/* whole screen needs repainting */
    unsigned     in_init_seq:1;		/* executing initial cmd list */
    unsigned     save_in_init_seq:1;
    unsigned     dont_use_init_cmds:1;	/* use keyboard input when true */
    unsigned     give_fixed_warning:1;	/* warn user about "fixed" vars */
    unsigned     fix_fixed_warning:1;	/* offer to fix it              */
    unsigned     unseen_in_view:1;
    unsigned     show_folders_dir:1;	/* show folders dir path when showing
                                                   folder names */
    unsigned     io_error_on_stream:1;	/* last write on mail_stream failed */
    unsigned     def_sort_rev:1;	/* true if reverse sort is default  */ 
    unsigned     restricted:1;
    unsigned	 show_dot_names:1;
    unsigned     nr_mode:1;
    unsigned     anonymous:1;           /* for now implys nr_mode */
    unsigned	 save_msg_rule:4;
    unsigned	 fcc_rule:3;
    unsigned	 ab_sort_rule:3;
    unsigned	 goto_default_rule:2;
    unsigned     full_header:1;         /* display full headers */
    unsigned     orig_use_fkeys:1;

    unsigned     try_to_create:1;     /* flag to try mail_create during save */
    unsigned     low_speed:1;         /* Special screen painting 4 low speed */
    unsigned     dead_inbox:1;
    unsigned     dead_stream:1;
    unsigned     noticed_dead_inbox:1;
    unsigned     noticed_dead_stream:1;
    unsigned     mm_log_error:1;
    unsigned     compose_mime:1;
    unsigned     show_new_version:1;
    unsigned     pre390:1;		/** temporary!!!! **/
    unsigned     first_time_user:1;
    unsigned	 outstanding_pinerc_changes:1;
    unsigned	 intr_pending:1;	/* received SIGINT and haven't acted */
    unsigned	 expunge_in_progress:1;	/* don't want to re-enter c-client   */
    unsigned	 readonly_pinerc:1;
    unsigned	 view_all_except:1;
    bitmap_t     feature_list;       /* a bitmap of all the features */
    char       **feat_list_back_compat;

    /*--- Command line flags, modify only on start up ---*/
    unsigned     start_in_index:1;
    unsigned     start_entry;
    unsigned     more_mode:1;

    unsigned     noshow_error:1;
    unsigned     noshow_warn:1;
    unsigned	 shown_newmail:1;
    unsigned	 noshow_timeout:1;

    unsigned     painted_body_on_startup:1;
    unsigned     painted_footer_on_startup:1;
    unsigned     open_readonly_on_startup:1;

#if defined(DOS) || defined(OS2)
    unsigned     blank_user_id:1;
    unsigned     blank_personal_name:1;
    unsigned     blank_user_domain:1;
#endif

    unsigned 	 viewer_overlap:8;
    unsigned	 scroll_margin:8;

    short	 init_context;

    int         *initial_cmds;         /* cmds to execute on startup */
    int         *free_initial_cmds;    /* used to free when done */

    char         c_client_error[300];  /* when nowhow_error is set and PARSE */

    struct ttyo *ttyo;
    KBESC_T     *kbesc;

    USER_S	 ui;		/* system derived user info */

    POST_S      *post;

    char	*home_dir,
                *hostname,	/* Fully qualified hostname */
                *localdomain,	/* The (DNS) domain this host resides in */
                *userdomain,	/* The per user domain from .pinerc or */
                *maildomain,	/* Domain name for most uses */
#if defined(DOS) || defined(OS2)
                *pine_dir,	/* argv[0] as provided by DOS */
#endif
                *pine_conf,	/* Location of global pine.conf */
                *pinerc,	/* Location of user's pinerc */
		*pine_name;	/* name we were invoked under */

    SortOrder    def_sort,	/* Default sort type */
		 sort_types[20];

    int          last_expire_year, last_expire_month;

    int		 printer_category;

    int		 status_msg_delay;

    int		 composer_fillcol;

    char	 last_error[500],
	       **init_errs;

    PRINT_S	*print;

    struct variable *vars;
};


/*------------------------------
  Structure to pass optionally_enter to tell it what keystrokes
  are special
  ----*/

typedef struct esckey {
    int  ch;
    int  rval;
    char *name;
    char *label;
} ESCKEY_S;

/*
 * Optionally_enter flags.
 */
#define OE_DISALLOW_CANCEL	0x01
#define OE_ENABLE_QUOTING	0x02


struct date {
    int	 sec, minute, hour, day, month, 
	 year, hours_off_gmt, min_off_gmt, wkday;
};


/*------------------------------
  Structures and enum used to expand the envelope structure to
  support user defined headers.
  ----*/

typedef enum {FreeText, Address, Fcc, Attachment, Subject} FieldType;

typedef struct pine_field {
    char     *name;			/* field's literal name		    */
    FieldType type;			/* field's type			    */
    unsigned  canedit:1;		/* allow editing of this field	    */
    unsigned  writehdr:1;		/* write rfc822 header for field    */
    unsigned  localcopy:1;		/* copy to fcc or postponed	    */
    unsigned  rcptto:1;			/* send to this if type Address	    */
    ADDRESS **addr;			/* used if type is Address	    */
    char     *scratch;			/* scratch pointer for Address type */
    char    **text;			/* field's free text form	    */
    char     *textbuf;			/* need to free this when done	    */
    struct headerentry *he;		/* he that goes with this field, a  */
					/*   pointer into headerentry array */
    struct pine_field *next;		/* next pine field		    */
} PINEFIELD;


typedef struct {
    ENVELOPE   *env;		/* standard c-client envelope		*/
    PINEFIELD  *local;		/* this is all of the headers		*/
    PINEFIELD  *custom;		/* ptr to start of custom headers	*/
    PINEFIELD **sending_order;	/* array giving writing order of hdrs	*/
} METAENV;


typedef enum {OpenFolder, SaveMessage, FolderMaint, GetFcc,
		Subscribe, PostNews} FolderFun;
typedef enum {MsgIndex, MultiMsgIndex, ZoomIndex} IndexType;
typedef enum {FolderName, MessageNumber, MsgTextPercent,
		TextPercent, FileTextPercent} TitleBarType;
typedef enum {HelpText, MainHelpText, ComposerHelpText, NewsText, MessageText,
		AttachText, WhoText, NetNewsText, SimpleText,
		ReviewMsgsText, ViewAbookText, ViewAbookAtt} TextType;
typedef enum {CharStarStar, CharStar, FileStar,
		TmpFileStar, PicoText} SourceType;
typedef enum {GoodTime, BadTime, VeryBadTime, DoItNow} CheckPointTime;
typedef enum {InLine, QStatus} DetachErrStyle;

/*
 * typedefs of generalized filters used by gf_pipe
 */
typedef int  (*gf_io_t)();	/* type of get and put char function     */
typedef void (*filter_t)();	/* type of filters for piping            */
typedef struct filter_s {	/* type to hold data for filter function */
    filter_t f;			/* next function in the pipe             */
    struct filter_s *next;	/* next filter to call                   */
    long     n;			/* number of chars seen                  */
    short    f1;		/* flags                                 */
    short    f2;		/* second place for flags                */
    unsigned char t;		/* temporary char                        */
    char     *line;		/* place for temporary storage           */
    char     *linep;		/* pointer into storage space            */
    void     *data;		/* misc data pointer			 */
    unsigned char queue[1 + GF_MAXBUF];
    short	  queuein, queueout;
} FILTER_S;

/*
 * typedef used by storage object routines
 */

typedef struct store_object {
    unsigned char *dp;		/* current position in data		 */
    unsigned char *eod;		/* end of current data			 */
    void	  *txt;		/* container's data			 */
    unsigned char *eot;		/* end of space alloc'd for data	 */
    int  (*writec) PROTO((int, struct store_object *));
    int  (*readc) PROTO((unsigned char *, struct store_object *));
    int  (*puts) PROTO((struct store_object *, char *));
    fpos_t	   used;	/* amount of object in use		 */
    char          *name;	/* optional object name			 */
    SourceType     src;		/* what we're copying into		 */
    short          flags;	/* flags relating to object use		 */
} STORE_S;

#define	so_writec(c, so)	((*(so)->writec)((c), (so)))
#define	so_readc(c, so)		((*(so)->readc)((c), (so)))
#define	so_puts(so, s)		((*(so)->puts)((so), (s)))


/*
 * Attribute table where information on embedded formatting and such
 * is really stored.
 */

typedef	enum {Link, LinkTarget} EmbedActions;

typedef struct atable_s {	/* a stands for either "anchor" or "action" */
    short	 handle;	/* handle for this action */
    EmbedActions action;	/* type of action indicated */
    short	 len;		/* number of characters in label */
    unsigned     wasuline:1;	/* previously underlined (not standard bold) */
    char	*name;		/* pointer to name of action */
    PARAMETER	*parms;		/* pointer to  necessary data */
    struct atable_s *next;
} ATABLE_S;


#define TAG_EMBED	'\377'	/* Announces embedded data in text string */
#define	TAG_INVON	'\001'	/* Supported character attributes	  */
#define	TAG_INVOFF	'\002'
#define	TAG_BOLDON	'\003'
#define	TAG_BOLDOFF	'\004'
#define	TAG_ULINEON	'\005'
#define	TAG_ULINEOFF	'\006'
#define	TAG_MIN_HANDLE	'\007'	/* indicate's a handle to an action	  */
#define	TAG_MAX_HANDLE	'\377'


/*
 * Structures to control flag maintenance screen
 */
struct flag_table {
    char     *name;		/* flag's name string */
    HelpType  help;		/* help text */
    long      flag;		/* flag tag (i.e., F_DEL above) */
    unsigned  set:2;		/* its state (set, unset, unknown) */
    unsigned  ukn:1;		/* allow unknown state */
};

struct flag_screen {
    char	      **explanation;
    struct flag_table  *flag_table;
};


/*
 * Some defs to help keep flag setting straight...
 */
#define	CMD_FLAG_CLEAR	FALSE
#define	CMD_FLAG_SET	TRUE
#define	CMD_FLAG_UNKN	2

 
/*
 * Structure and macros to help control format_header_text
 */
typedef struct header_s {
    unsigned type:4;
    unsigned except:1;
    union {
	char **l;		/* list of char *'s */
	long   b;		/* bit field of header fields (FE_* above) */
    } h;
} HEADER_S;

#define	HD_LIST		1
#define	HD_BFIELD	2
#define	HD_INIT(H, L, E, B)	if((L) && (L)[0]){			\
				    (H)->type = HD_LIST;		\
				    (H)->except = (E);			\
				    (H)->h.l = (L);			\
				}					\
				else{					\
				    (H)->type = HD_BFIELD;		\
				    (H)->h.b = (B);			\
				}


/*
 * Macro to help determine when we need to filter out naughty chars
 * from message index or headers...
 */
#define	CAN_DISPLAY(c)	(iscntrl((c) & 0x7f)				\
			 && !(isspace((unsigned char) (c))		\
			      || (c) == '\016'				\
			      || (c) == '\017'))



/*======================================================================
    Declarations of all the Pine functions.
 ====*/
  
/*-- addrbook.c --*/
void	    addrbook_reset PROTO((void));
char	   *addr_book_bounce PROTO((void));
char	   *addr_book_compose PROTO((char **));
char	   *addr_book_compose_lcc PROTO((char **));
void	    addr_book_screen PROTO((struct pine *));
int	    address_is_us PROTO((ADDRESS *, struct pine *));
int	    address_is_same PROTO((ADDRESS *, ADDRESS *));
char	   *addr_list_string PROTO((ADDRESS *,
				  char *(*f) PROTO((ADDRESS *, char *)), int));
char	   *addr_string PROTO((ADDRESS *, char *));
char	  **detach_abook_att PROTO ((MAILSTREAM *, long, BODY *, char *));
char	   *simple_addr_string PROTO((ADDRESS *, char *));
int	    build_address PROTO((char *, char **, char **, BUILDER_ARG *));
int	    build_addr_lcc PROTO((char *, char **, char **, BUILDER_ARG *));
void	    cmd_take_addr PROTO((struct pine *, MSGNO_S *, int));
void	    completely_done_with_adrbks PROTO((void));
ADDRESS    *copyaddr PROTO((ADDRESS *));
void	    just_update_lookup_file PROTO((char *, char *));
char	   *get_fcc PROTO((char *));
char	   *get_fcc_based_on_to PROTO((ADDRESS *));
char	   *get_fcc_from_addr PROTO((ADDRESS *, char *));
char	   *get_nickname_from_addr PROTO((ADDRESS *, char *));
void	    set_last_fcc PROTO((char *));

/*-- adrbklib.c --*/
int	    percent_abook_saved PROTO((void));

/*-- args.c --*/
char	    *pine_args PROTO((struct pine *, int, char **, char ***));

/*--- filter.c ---*/
STORE_S	   *so_get PROTO((SourceType, char *, int));
void	    so_give PROTO((STORE_S **));
int	    so_seek PROTO((STORE_S *, long, int));
int	    so_truncate PROTO((STORE_S *, long));
int	    so_release PROTO((STORE_S *));
void	   *so_text PROTO((STORE_S *));
void	    gf_filter_init PROTO((void));
char	   *gf_pipe PROTO((gf_io_t, gf_io_t));
long	    gf_bytes_piped PROTO(());
char	   *gf_filter PROTO((char *, char *, STORE_S *, gf_io_t, filter_t *));
void	    gf_set_so_readc PROTO((gf_io_t *, STORE_S *));
void	    gf_set_so_writec PROTO((gf_io_t *, STORE_S *));
void	    gf_set_readc PROTO((gf_io_t *, void *, unsigned long, SourceType));
void	    gf_set_writec PROTO((gf_io_t *, void *, unsigned long, \
				 SourceType));
int	    gf_puts PROTO((char *, gf_io_t));
void	    gf_set_terminal PROTO((gf_io_t));
void	    gf_binary_b64 PROTO((FILTER_S *, int));
void	    gf_b64_binary PROTO((FILTER_S *, int));
void	    gf_qp_8bit PROTO((FILTER_S *, int));
void	    gf_8bit_qp PROTO((FILTER_S *, int));
void	    gf_rich2plain PROTO((FILTER_S *, int));
void	    gf_rich2plain_opt PROTO((int));
void	    gf_enriched2plain PROTO((FILTER_S *, int));
void	    gf_enriched2plain_opt PROTO((int));
void	    gf_html2plain PROTO((FILTER_S *, int));
void	    gf_escape_filter PROTO((FILTER_S *, int));
void	    gf_control_filter PROTO((FILTER_S *, int));
void	    gf_wrap PROTO((FILTER_S *, int));
void	    gf_wrap_filter_opt PROTO((int));
void	    gf_busy PROTO((FILTER_S *, int));
void	    gf_nvtnl_local PROTO((FILTER_S *, int));
void	    gf_local_nvtnl PROTO((FILTER_S *, int));
void	    gf_line_test PROTO((FILTER_S *, int));
void	    gf_line_test_opt PROTO((int (*) PROTO((long, char *))));
void	    gf_prefix PROTO((FILTER_S *, int));
void	    gf_prefix_opt PROTO((char *));
#if defined(DOS) || defined(OS2)
void	    gf_translate PROTO((FILTER_S *, int));
void	    gf_translate_opt PROTO((unsigned char *, unsigned));
#endif

/*--- folder.c ---*/
void	    folder_screen PROTO((struct pine *));
char	   *folders_for_fcc PROTO((char **));
int	    folder_lister PROTO((struct pine *, FolderFun, CONTEXT_S *, \
			 CONTEXT_S **, char *, char ***, CONTEXT_S *, void *));
#ifdef	NEWBB
void	    mark_folder_as_new PROTO((char *));
#endif
char	   *pretty_fn PROTO((char *));
int	    folder_exists PROTO((char *, char *));
int	    folder_create PROTO((char *, CONTEXT_S *));
int	    folder_complete PROTO((CONTEXT_S *, char *));
void	    init_folders PROTO((struct pine *));
CONTEXT_S  *new_context PROTO((char *));
void	    free_folders PROTO(());
void	    free_context PROTO((CONTEXT_S **));
void	   *find_folders PROTO((char *, char *));
void	    find_folders_in_context PROTO((MAILSTREAM **, CONTEXT_S *,char *));
CONTEXT_S  *default_save_context PROTO((CONTEXT_S *));
FOLDER_S   *folder_entry PROTO((int, void *));
FOLDER_S   *new_folder PROTO((char *));
int	    folder_insert PROTO((int, FOLDER_S *, void *));
int	    folder_index PROTO((char *, void *));
char	   *folder_is_nick PROTO((char *, void *));
char	   *next_folder PROTO((MAILSTREAM **, char *, char *,CONTEXT_S *,int));
void	    init_inbox_mapping PROTO((char *, CONTEXT_S *));
char	   *context_string PROTO((char *));
int	    news_build PROTO((char *, char **, char **, BUILDER_ARG *));
char	   *news_group_selector PROTO((char **));
void	    free_newsgrp_cache PROTO(());

/*-- help.c --*/
#if defined(DOS) || defined(HELPFILE)
void	    helper PROTO((short, char *title, int)); 
char	  **get_help_text PROTO((short, int *lines)); 
#else
void	    helper PROTO((char *text[], char *title, int)); 
#endif
void	    print_all_help PROTO((void));
void	    review_messages PROTO((char *));
void	    add_review_message PROTO((char *));
void	    end_status_review PROTO((void));

/*-- imap.c --*/
char	   *cached_user_name PROTO((char *));
void	    imap_flush_passwd_cache PROTO(());
long	    pine_tcptimeout PROTO((long));

/*-- init.c --*/
void	    init_vars PROTO((struct pine *));
void	    free_vars PROTO((struct pine *));
char	   *expand_variables PROTO((char *, char *));
void	    set_current_val PROTO((struct variable *, int, int));
void	    set_feature_list_current_val PROTO((struct variable *));
int	    init_username PROTO((struct pine *));
int	    init_hostname PROTO((struct pine *));  
int	    write_pinerc PROTO((struct pine *));
int	    var_in_pinerc PROTO((char *));
void	    free_pinerc_lines PROTO((void));
void	    dump_global_conf PROTO((void));
void	    dump_new_pinerc PROTO((char *));
int	    set_variable PROTO((int, char *, int));
int	    set_variable_list PROTO((int, char **));
int	    init_mail_dir PROTO((struct pine *));
void	    init_save_defaults PROTO(());
int	    expire_sent_mail PROTO((void));
char	  **parse_list PROTO((char *, int, char **));
NAMEVAL_S  *feature_list PROTO((int));
NAMEVAL_S  *save_msg_rules PROTO((int));
NAMEVAL_S  *fcc_rules PROTO((int));
NAMEVAL_S  *ab_sort_rules PROTO((int));
NAMEVAL_S  *goto_rules PROTO((int));
void	    set_old_growth_bits PROTO((struct pine *, int));
int	    test_old_growth_bits PROTO((struct pine *, int));
void	    dump_config PROTO((struct pine *, gf_io_t));
void	    dump_pine_struct PROTO((struct pine *, gf_io_t));

/*---- mailcap.c ----*/
char	   *mailcap_build_command PROTO((BODY *, char *, int *));
int	    mailcap_can_display PROTO((int, char *, PARAMETER *));
void	    mailcap_free PROTO((void));
int	    set_mime_type_by_extension PROTO((BODY *, char *));
int	    set_mime_extension_by_type PROTO((char *, char *));

/*---- mailcmd.c ----*/
int	    process_cmd PROTO((struct pine *, MSGNO_S *, int, int,int,int *));
void	    bogus_command PROTO((int, char *));
char	   *broach_folder PROTO((int, int, CONTEXT_S **));
int	    do_broach_folder PROTO((char *, CONTEXT_S *));
void	    visit_folder PROTO((struct pine *, char *, CONTEXT_S *));
void	    expunge_and_close PROTO((MAILSTREAM *, CONTEXT_S *, char *,
				     char **));
char	   *get_uname PROTO((char *, char *, int));
char	   *build_updown_cmd PROTO((char *, char *, char *, char*));
int	    file_lister PROTO((char *, char *, char *, int, int));
int	    pseudo_selected PROTO((MSGNO_S *));
void	    restore_selected PROTO((MSGNO_S *));

/*--- mailindx.c ---*/
void	    mail_index_screen PROTO((struct pine *));
int	    index_lister PROTO((struct pine *, CONTEXT_S *, char *, \
				MAILSTREAM *, MSGNO_S *));
void	    clear_index_cache PROTO((void));
void	    clear_index_cache_ent PROTO((long));
int	    build_index_cache PROTO((long));
#ifdef	DOS
void	    flush_index_cache PROTO((void));
#endif
long	    line_hash PROTO((char *));
HLINE_S	   *build_header_line PROTO((struct pine *, MAILSTREAM *,
				     MSGNO_S *, long));
void	    init_index_format PROTO((char *, INDEX_COL_S **));
void	    build_header_cache PROTO((void));
void	    redraw_index_body PROTO((void));
void	    do_index_border PROTO((CONTEXT_S *, char *, MAILSTREAM *, \
				   MSGNO_S *, IndexType, int *, int));
char	   *sort_name PROTO((SortOrder));
void	    sort_current_folder PROTO((int, SortOrder, int));
int	    percent_sorted PROTO((void));
void	    msgno_init PROTO((MSGNO_S **, long));
void	    msgno_add_raw PROTO((MSGNO_S *, long));
void	    msgno_flush_raw PROTO((MSGNO_S *, long));
int	    msgno_in_select PROTO((MSGNO_S *, long));
long	    msgno_in_sort PROTO((MSGNO_S *, long));
void	    msgno_inc PROTO((MAILSTREAM *, MSGNO_S *));
void	    msgno_dec PROTO((MAILSTREAM *, MSGNO_S *));
void	    msgno_include PROTO((MAILSTREAM *, MSGNO_S *));
void	    msgno_exclude PROTO((MAILSTREAM *, MSGNO_S *));

/*---- mailpart.c ----*/
void	    attachment_screen PROTO((struct pine *, MSGNO_S *));
char	   *detach PROTO((MAILSTREAM *, long, BODY *, char *, long *, \
			  gf_io_t, filter_t *));
void	    save_attachment PROTO((int, long, ATTACH_S *));
void	    pipe_attachment PROTO((long, ATTACH_S *));
char	   *expand_filter_tokens PROTO((char *, ENVELOPE *, char **, char **,
					int *));
char	   *filter_session_key PROTO(());
char	   *filter_data_file PROTO((int));

/*--- mailview.c ---*/
void	    mail_view_screen PROTO((struct pine *));
void	    scrolltool PROTO((void *, char *, TextType, SourceType, \
			      ATTACH_S *));
char	   *body_type_names PROTO((int));
char	   *type_desc PROTO((int, char *, PARAMETER *, int));
int	    format_message PROTO((long, ENVELOPE *, BODY *, int, gf_io_t));
int	    match_escapes PROTO((char *));
int	    decode_text PROTO((ATTACH_S *, long, gf_io_t, DetachErrStyle,int));
char	   *display_parameters PROTO((PARAMETER *));
void	    display_output_file PROTO((char *, char *, char *));
char	   *xmail_fetchheader_lines PROTO((MAILSTREAM *, long, char **));
char	   *xmail_fetchheader_lines_not PROTO((MAILSTREAM *, long, char **));
int	    format_header_text PROTO((MAILSTREAM *, long, ENVELOPE *,
				      HEADER_S *, gf_io_t, char *));
void	    format_envelope PROTO((MAILSTREAM *, long, ENVELOPE *, gf_io_t,
				   long, char *));

/*--newmail.c --*/
long	    new_mail PROTO((int, int, int));
int	    check_point PROTO((CheckPointTime));
void	    check_point_change PROTO(());
void	    reset_check_point PROTO((void));
void	    zero_new_mail_count PROTO((void));

/*-- os.c --*/
int	    can_access PROTO((char *, int));
int	    can_access_in_path PROTO((char *, char *, int));
long	    name_file_size PROTO((char *));
long	    fp_file_size PROTO((FILE *));
time_t	    name_file_mtime PROTO((char *));
time_t	    fp_file_mtime PROTO((FILE *));
void	    file_attrib_copy PROTO((char *, char *));
int	    is_writable_dir PROTO((char *));
int	    create_mail_dir PROTO((char *));
int	    rename_file PROTO((char *, char *));
void	    build_path PROTO((char *, char *, char *));
int	    is_absolute_path PROTO((char *));
char	   *last_cmpnt PROTO((char *));
int	    expand_foldername PROTO((char *));
char	   *fnexpand PROTO((char *, int));
char	   *filter_filename PROTO((char *));
int	    cntxt_allowed PROTO((char *));
long	    disk_quota PROTO((char *, int *));
char	   *read_file PROTO((char *));
FILE	   *create_tmpfile PROTO((void));
char	   *temp_nam PROTO((char *, char *));
void	    coredump PROTO((void));
void	    getdomainnames PROTO((char *, int, char *, int));
int	    have_job_control PROTO((void));
int 	    stop_process PROTO((void));
char	   *error_description PROTO((int));
void	    get_user_info PROTO((struct user_info *));
char	   *local_name_lookup PROTO((char *));
int	    change_passwd PROTO((void));
int	    mime_can_display PROTO((int, char *, PARAMETER *, int *));
char	   *canonical_name PROTO((char *));
PIPE_S	   *open_system_pipe PROTO((char *, char **, char **, int));
int	    close_system_pipe PROTO((PIPE_S **));
SigType	  (*posix_signal PROTO((int, SigType (*)())))();
int	    posix_sigunblock PROTO((int));
char	   *smtp_command PROTO((char *));
int	    mta_handoff PROTO((METAENV *, BODY *, char *));
char	   *post_handoff PROTO((METAENV *, BODY *, char *));
void	    exec_mailcap_cmd PROTO((char *, char *, int));
int	    exec_mailcap_test_cmd PROTO((char *));
#ifdef DEBUG
void	    init_debug PROTO((void));
void	    save_debug_on_crash PROTO((FILE *));
int	    do_debug PROTO((FILE *));
#endif
#if defined(DOS) || defined(OS2)
char	   *temp_nam_ext PROTO((char *, char *, char *));
#endif
#ifdef	DOS
void	   *dos_cache PROTO((MAILSTREAM *, long, long));
char	   *dos_gets PROTO((readfn_t, void *, unsigned long));
#endif
#ifdef	_WINDOWS
void	    scroll_setrange PROTO((long max));
void	    scroll_setpos PROTO((long pos));
long	    scroll_getpos PROTO((void));
long	    scroll_getscrollto PROTO((void));
#endif
#ifdef	MOUSE
unsigned long mouse_in_main PROTO((int, int, int));
#endif
int	    open_printer PROTO((char *));
void	    close_printer PROTO((void));
int	    print_char PROTO((int));
void	    print_text PROTO((char *));
void	    print_text1 PROTO((char *, char *));
void	    print_text2 PROTO((char *, char *, char *));
void	    print_text3 PROTO((char *, char *, char *, char *));

/*--- other.c ---*/
int	    lock_keyboard PROTO((void));
void	    signature_edit PROTO((char *));
void	    redraw_kl_body PROTO(());
void	    redraw_klocked_body PROTO(());
void	    gripe PROTO((struct pine *));
void	    option_screen PROTO((struct pine *));
void	    flag_maintenance_screen PROTO((struct pine *,
					   struct flag_screen *));
void	    parse_printer PROTO ((char *, char **, char **, char **,
				  char **, char **, char **));
#ifndef DOS
void	    select_printer PROTO((struct pine *));
#endif

/*-- pine.c --*/
void	    main_menu_screen PROTO((struct pine *));
void	    show_main_screen PROTO((struct pine *, int, OtherMenu, \
				    struct key_menu *, int, Pos *));
void	    news_screen PROTO((struct pine *));
void	    quit_screen PROTO((struct pine *));
long	    count_flagged PROTO((MAILSTREAM *, char *));
void	    panic PROTO((char *));
void	    panic1 PROTO((char *, char *));
MAILSTREAM *same_stream PROTO((char *, MAILSTREAM *));
MsgNo	    first_sorted_flagged PROTO((unsigned long, MAILSTREAM *));
MsgNo	    next_sorted_flagged PROTO((unsigned long, MAILSTREAM *, \
				       long, int *));
long	    any_lflagged PROTO((MSGNO_S *, int));
void	    dec_lflagged PROTO((MSGNO_S *, int, long));
int	    get_lflag PROTO((MAILSTREAM *, MSGNO_S *, long, int));
int	    set_lflag PROTO((MAILSTREAM *, MSGNO_S *, long, int, int));
void	    warn_other_cmds PROTO(());
unsigned long pine_gets_bytes PROTO((int));
     
/*-- reply.c --*/
void	    reply PROTO((struct pine *));
void	    reply_delimiter PROTO((ENVELOPE *, gf_io_t));
char	   *reply_quote_str PROTO((ENVELOPE *, int));
void	    forward PROTO((struct pine *));
void	    bounce PROTO((struct pine *));
void	    forward_text PROTO((struct pine *, void *, SourceType));
char	   *generate_message_id PROTO((struct pine *));
ADDRESS    *first_addr PROTO((ADDRESS *));
char	   *get_signature PROTO(());
char	   *signature_path PROTO((char *, char *, size_t));
ENVELOPE   *copy_envelope PROTO((ENVELOPE *));
BODY	   *copy_body PROTO((BODY *, BODY *));
PARAMETER  *copy_parameters PROTO((PARAMETER *));
int	    get_body_part_text PROTO((MAILSTREAM *, BODY *, long, char *, \
				      gf_io_t, char *, char *));
char	   *partno PROTO((BODY *, BODY *));

/*-- screen.c --*/
void	    format_keymenu PROTO((struct key_menu *, bitmap_t, int, \
				  OtherMenu, int));
void	    draw_keymenu PROTO((struct key_menu *, bitmap_t, int, int, \
				int, OtherMenu, int));
void	    blank_keymenu PROTO((int, int));
void	    draw_cancel_keymenu PROTO((void));
void	    redraw_keymenu PROTO((void));
void	    mark_keymenu_dirty PROTO((void));
void	    mark_titlebar_dirty PROTO((void));
char	   *status_string PROTO((MAILSTREAM *, MESSAGECACHE *));
char	   *format_titlebar PROTO((int *));
char	   *set_titlebar PROTO((char *, MAILSTREAM *, CONTEXT_S *, char *, \
				MSGNO_S *, int, TitleBarType, long, long));
void	    push_titlebar_state PROTO(());
void	    pop_titlebar_state PROTO(());
void	    redraw_titlebar PROTO((void));
void	    update_titlebar_message PROTO((void));
void	    update_titlebar_status PROTO((void));
void	    update_titlebar_percent PROTO((long));
void	    update_titlebar_lpercent PROTO((long));
void	    clearfooter PROTO((struct pine *));
void	    end_titlebar PROTO((void));
void	    end_keymenu PROTO((void));

/*-- send.c --*/
void	    compose_screen PROTO((struct pine *)); 
void	    compose_mail PROTO((char *, char *, gf_io_t));
void	    pine_send PROTO((ENVELOPE *, BODY **, char *, char *, \
			     long *, char *, char *, char *, void *, int));
int	    pine_simple_send PROTO((ENVELOPE *, BODY **, char *, char *,
				    char **, int));
char	   *pine_send_status PROTO((int, char *, char *, int *));
void	    phone_home PROTO(());
void	    pine_free_body PROTO((BODY **));
void	    simple_header_parse PROTO((char *, char **, char **));
int	    valid_subject PROTO((char *, char **, char **, BUILDER_ARG *));
long	    flags_for_pico PROTO((struct pine *));
long	    new_mail_for_pico PROTO((int, int));
int	    display_message_for_pico PROTO((int));
int	    upload_msg_to_pico PROTO((char *, long *));
char	   *checkpoint_dir_for_pico PROTO((char *, int));
void	    set_mime_type_by_grope PROTO((BODY *));
void	    resize_for_pico PROTO(());

/*-- signals.c --*/
int	    busy_alarm PROTO((unsigned, char *, percent_done_t, int));
void	    suspend_busy_alarm PROTO((void));
void	    resume_busy_alarm PROTO((void));
void	    cancel_busy_alarm PROTO((int)); 
void	    fake_alarm_blip PROTO((void));
void	    init_signals PROTO((void));
void	    init_sighup PROTO((void));
void	    end_sighup PROTO((void));
void	    init_sigwinch PROTO((void));
int	    do_suspend PROTO((struct pine *));
void	    fix_windsize PROTO((struct pine *));
void	    end_signals PROTO((int));
SigType	    hup_signal PROTO(());
SigType	    term_signal PROTO(());
SigType	    child_signal PROTO(());
void	    intr_allow PROTO((void));
void	    intr_disallow PROTO((void));
void	    intr_handling_on PROTO((void));
void	    intr_handling_off PROTO((void));

/*-- status.c --*/
int	    display_message PROTO((int));
void	    d_q_status_message PROTO((void));
void	    q_status_message PROTO(( int, int, int, char *));
void	    q_status_message1 PROTO((int, int, int, char *, void *));
void	    q_status_message2 PROTO((int, int, int, char *, void *, void *));
void	    q_status_message3 PROTO((int, int, int, char *, void *, void *, \
				     void *));
void	    q_status_message4 PROTO((int, int, int, char *, void *, void *, \
				     void *, void *));
void	    q_status_message7 PROTO((int, int, int, char *, void *, void *, \
				     void *, void *, void *, void *, void *));
int	    messages_queued PROTO((long *));
char	   *last_message_queued PROTO((void));
int	    status_message_remaining PROTO((void));
int	    status_message_write PROTO((char *, int));
void	    flush_status_messages PROTO((int));
void	    flush_ordered_messages PROTO((void));
void	    mark_status_dirty PROTO((void));
void	    mark_status_unknown PROTO((void));
int	    want_to PROTO((char *, int, int, HelpType, int, int));
int	    one_try_want_to PROTO((char *, int, int, HelpType, int, int));
int	    radio_buttons PROTO((char *, int, ESCKEY_S *, int, int, HelpType,
				 int));

/*-- strings.c --*/
char	   *rplstr PROTO((char *, int, char *));
void	    sqzspaces PROTO((char *));
void	    sqznewlines PROTO((char *));
void	    removing_trailing_white_space PROTO((char *));
void	    removing_leading_white_space PROTO((char *));
char	   *removing_quotes PROTO((char *));
char	   *strclean PROTO((char *));
int	    strucmp PROTO((char *, char *));
int	    struncmp PROTO((char *, char *, int));
int	    in_dir PROTO((char *, char *));
char	   *srchstr PROTO((char *, char *));
char	   *strindex PROTO((char *, int));
char	   *strrindex PROTO((char *, int));
void	    sstrcpy PROTO((char **, char *));
char	   *istrncpy PROTO((char *, char *, int));
char	   *month_abbrev PROTO((int));
char	   *week_abbrev PROTO((int));
int	    month_num PROTO((char *));
void	    parse_date PROTO((char *, struct date *));
int	    compare_dates PROTO((MESSAGECACHE *, MESSAGECACHE *));
void	    convert_to_gmt PROTO((MESSAGECACHE *));
char	   *pretty_command PROTO((int));
char	   *repeat_char PROTO((int, int));
char	   *comatose PROTO((long));
char	   *byte_string PROTO((long));
char	   *enth_string PROTO((int));
char	   *long2string PROTO((long));
char	   *int2string PROTO((int));
char	   *strtoval PROTO((char *, int *, int, int, char *, char *));
void	    get_pair PROTO((char *, char **, char **, int));
int	    read_hex PROTO((char *));
char	   *string_to_cstring PROTO((char *));
char	   *cstring_to_hexstring PROTO((char *));
char	   *bitstrip PROTO((char *));
unsigned char *rfc1522_decode PROTO((unsigned char *, char *, char **));
char	   *rfc1522_encode PROTO((char *, unsigned char *, char *));

/*-- ttyin.c--*/
int	    read_char PROTO((int));
int	    read_command PROTO(());
int	    optionally_enter PROTO((char *, int, int, int, int, int, char *, \
				    ESCKEY_S *, HelpType, unsigned));
int	    init_tty_driver PROTO((struct pine *));
void	    tty_chmod PROTO((struct pine *, int, int));
void	    setup_dflt_esc_seq PROTO((KBESC_T **));
void	    end_tty_driver PROTO((struct pine *));
int	    Raw PROTO((int));
void	    xonxoff_proc PROTO((int));
void	    crlf_proc PROTO((int));
void	    intr_proc PROTO((int));
void	    flush_input PROTO(());
void	    end_keyboard PROTO((int));
void	    init_keyboard PROTO((int));
int	    validatekeys PROTO((int));
int	    key_recorder PROTO((int, int));

/*-- ttyout.c --*/
int	    get_windsize PROTO((struct ttyo *));
int	    BeginScroll PROTO((int, int));
void	    EndScroll PROTO((void));
int	    ScrollRegion PROTO(( int));
int	    Writechar PROTO((unsigned int, int));
void	    Write_to_screen PROTO((char *));
void	    PutLine0 PROTO((int, int, char *));
void	    PutLine0n8b PROTO((int, int, char *, int));
void	    PutLine1 PROTO((int, int, char *, void *));
void	    PutLine2 PROTO((int, int, char *, void *, void *));
void	    PutLine3 PROTO((int, int, char *, void *, void *, void *));
void	    PutLine4 PROTO((int, int, char *, void *, void *, void *, void *));
void	    PutLine5 PROTO((int, int, char *, void *, void *, void *, void *, \
			    void *));
void	    CleartoEOLN PROTO((void));
int	    CleartoEOS PROTO((void));
void	    ClearScreen PROTO((void));
void	    ClearLine PROTO((int));
void	    ClearLines PROTO((int, int));
void	    MoveCursor PROTO((int, int));
void	    NewLine PROTO((void));
int	    StartInverse PROTO((void));
void	    EndInverse PROTO((void));
int	    InverseState PROTO((void));
int	    StartUnderline PROTO((void));
void	    EndUnderline PROTO((void));
int	    StartBold PROTO((void));
void	    EndBold PROTO((void));
int	    config_screen PROTO((struct ttyo **, KBESC_T **));
void	    init_screen PROTO((void));
void	    end_screen PROTO((char *));
void	    outchar PROTO((int));
void	    icon_text PROTO((char *));
void	    clear_cursor_pos PROTO((void));
int	    InsertChar PROTO((int));
int	    DeleteChar PROTO((int));

#define SCREEN_FUN_NULL ((void (*) PROTO((void *)))NULL)

#endif /* _PINE_INCLUDED */
