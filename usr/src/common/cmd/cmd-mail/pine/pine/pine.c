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
   1989-1997 by the University of Washington.

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

#include "headers.h"


/*
 * Handy local definitions...
 */
#define	LEGAL_NOTICE \
   "Copyright 1989-1997.  PINE is a trademark of the University of Washington."

#define	STDIN_FD	0			/* Our normal input desc    */
#define	STDER_FD	2			/* Another desc tied to tty */ 
#define	PIPED_FD	5			/* Some innocuous desc	    */


/*
 * Globals referenced throughout pine...
 */
struct pine *ps_global;				/* THE global variable! */
char	    *pine_version = PINE_VERSION;	/* version string */


/*----------------------------------------------------------------------
  General use big buffer. It is used in the following places:
    compose_mail:    while parsing header of postponed message
    append_message2: while writing header into folder
    q_status_messageX: while doing printf formatting
    addr_book: Used to return expanded address in. (Can only use here 
               because mm_log doesn't q_status on PARSE errors !)
    pine.c: When address specified on command line
    init.c: When expanding variable values

 ----*/
char         tmp_20k_buf[20480];


/*
 * byte count used by our gets routine to keep track 
 */
unsigned long gets_bytes;


/*
 * Internal prototypes
 */
int   setup_mini_menu PROTO((int));
void  do_menu PROTO((int, Pos *));
void  main_redrawer PROTO(());
void  new_user_or_version PROTO((char []));
void  truncated_listvars_warning PROTO((int *));		/** temporary **/
void  phone_home_blurb PROTO((int));
void  do_setup_task PROTO(());
void  queue_init_errors PROTO((struct pine *));
void  upgrade_old_postponed PROTO(());
void  goodnight_gracey PROTO((struct pine *, int));
int   read_stdin_char PROTO(());
char *pine_gets PROTO((readfn_t, void *, unsigned long));
char *nntp_host PROTO((char *));


#ifdef	DOS
static char first_time_message[] = "\
                   Welcome to PC-Pine...\n\
\n\
a Program for Internet News and Email.  Pine offers the ability to:\n\
  -Access local and remote message folders using a simple user-interface\n\
  -Send documents, graphics, etc (via the MIME standard for attachments)\n\
\n\
COMMANDS IN PINE:  Available commands are always listed on the last\n\
  two lines of the screen.  If there are more than can be displayed, the\n\
  \"O\" command will cycle their display.  Except in function key mode,\n\
  commands can be executed even though they are not displayed.\n\
\n\
PINE CONFIGURATION:  If you haven't yet filled out your Pine configuration\n\
  file, Pine will ask you for this information as it's needed.  For further \n\
  customization, use the Setup/Config Screen (\"S\" then \"C\" in Main Menu).";

static char new_version_message[] = "\
             <<<This message will appear only once.>>>\n\
\n\
             Welcome to the latest version of Pine!\n\
\n\
Your Pine configuration file shows that you have not used this version of\n\
Pine before.  You'll see very few changes to Pine's standard behavior, but\n\
there are many new features that you may enable via the new Config\n\
screen under the Main Menu SETUP command.  See the Release Notes\n\
(\"R\" on the Main Menu) for a more detailed list of changes. \n";

#else
#ifdef OS2
static char first_time_message[] = "\
                   Welcome to PC-Pine for OS/2...\n\
\n\
a Program for Internet News and Email.  Pine offers the ability to:\n\
  -Access local and remote message folders using a simple user-interface\n\
  -Send documents, graphics, etc (via the MIME standard for attachments)\n\
\n\
COMMANDS IN PINE:  Available commands are always listed on the last\n\
  two lines of the screen.  If there are more than can be displayed, the\n\
  \"O\" command will cycle their display.  Except in function key mode,\n\
  commands can be executed even though they are not displayed.\n\
\n\
PINE CONFIGURATION:  If you haven't yet filled out your Pine configuration\n\
  file, Pine will ask you for this information as it's needed.  For further \n\
  customization, use the Setup/Config Screen (\"S\" then \"C\" in Main Menu).";

static char new_version_message[] = "\
             <<<This message will appear only once.>>>\n\
\n\
             Welcome to the latest version of Pine!\n\
\n\
Your Pine configuration file shows that you have not used this version of\n\
Pine before.  You'll see very few changes to Pine's standard behavior, but\n\
there are many new features that you may enable via the new Config\n\
screen under the Main Menu SETUP command.  See the Release Notes\n\
(\"R\" on the Main Menu) for a more detailed list of changes. \n";

#else
static char first_time_message[] = "\
                   Welcome to Pine...\n\
\n\
a Program for Internet News and Email.  Pine offers the ability to:\n\
  -Access local and remote message folders using a simple user-interface\n\
  -Send documents, graphics, etc (via the MIME standard for attachments)\n\
\n\
COMMANDS IN PINE:  Available commands are always listed on the last\n\
 two lines of the screen.  If there are more than can be displayed, the\n\
  \"O\" command will cycle their display.  Except in function key mode,\n\
  commands can be executed even though they are not displayed.\n\
\n\
PINE CONFIGURATION:  Pine has created a default configuration file for you.\n\
  To customize pine's behavior, use the Setup/Config (\"S\" then \"C\"\n\
  in Main Menu).  We also suggest seeing pine's main help (\"?\" in Main \
Menu).";


static char new_version_message[] = "\
             <<<This message will appear only once.>>>\n\
\n\
             Welcome to the latest version of Pine!\n\
\n\
Your Pine configuration file shows that you have not used this version of\n\
Pine before.  You'll see very few changes to Pine's standard behavior, but\n\
there are many new features that you may enable via the new Config\n\
screen under the Main Menu SETUP command.  See the Release Notes\n\
(\"R\" on the Main Menu) for a more detailed list of changes.";
#endif
#endif



static struct key main_keys[] =
       {{"?","Help",KS_SCREENHELP},
	{"O","OTHER CMDS",KS_NONE},
	{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},
	{"P","PrevCmd",KS_NONE},
	{"N","NextCmd",KS_NONE},
	{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},
	{"R","RelNotes",KS_NONE},
	{"K","KBLock",KS_NONE},
	{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},

	{"?","Help",KS_SCREENHELP},
	{"O","OTHER CMDS",KS_NONE},
	{"Q","Quit",KS_EXIT},
	{"C","Compose",KS_COMPOSER},
	{"L","ListFldrs",KS_FLDRLIST},
	{"G","GotoFldr",KS_GOTOFLDR},
	{"I","Index",KS_FLDRINDEX},
	{"J","Journal",KS_REVIEW},
	{"S","Setup",KS_NONE},
	{"A","AddrBook",KS_ADDRBOOK},
	{"B","Report Bug",KS_NONE},
	{NULL,NULL,KS_NONE}};
INST_KEY_MENU(main_keymenu, main_keys);
#define MAIN_HELP_KEY		0
#define MAIN_DEFAULT_KEY	3
#define MAIN_KBLOCK_KEY		9
#define MAIN_HELP_KEY2		12
#define MAIN_QUIT_KEY		14
#define MAIN_COMPOSE_KEY	15
#define MAIN_FOLDER_KEY		16
#define MAIN_INDEX_KEY		18
#define MAIN_SETUP_KEY		20
#define MAIN_ADDRESS_KEY	21

/*
 * length of longest label from keymenu, of labels corresponding to
 * commands in the middle of the screen.  9 is length of ListFldrs
 */
#define LONGEST_LABEL 9  /* length of longest label from keymenu */
#define LONGEST_NAME 1   /* length of longest name from keymenu */



/*----------------------------------------------------------------------
     main routine -- entry point

  Args: argv, argc -- The command line arguments


 Initialize pine, parse arguments and so on

 If there is a user address on the command line go into send mode and exit,
 otherwise loop executing the various screens in Pine.

 NOTE: The Windows 3.1 port def's this to "app_main"
  ----*/

main(argc, argv)
    int   argc;
    char *argv[];
{
    char            *folder_to_open;
    int              rv;
    struct pine     *pine_state;
    char             int_mail[MAXPATH+1];
    gf_io_t	     stdin_getc = NULL;
#ifdef DYN
    char stdiobuf[64];
#endif

#ifdef LC_COLLATE
    /*
     * This may not have the desired effect, if strcmp and friends
     * don't know about it.
     */
    setlocale(LC_COLLATE, "");
#endif
#ifdef LC_CTYPE
    setlocale(LC_CTYPE, "");
#endif

    /*----------------------------------------------------------------------
          Set up buffering and some data structures
      ----------------------------------------------------------------------*/

    pine_state                 = (struct pine *)fs_get(sizeof (struct pine));
    memset((void *)pine_state, 0, sizeof(struct pine));
    ps_global                  = pine_state;
    ps_global->def_sort        = SortArrival;
    ps_global->sort_types[0]   = SortSubject;
    ps_global->sort_types[1]   = SortArrival;
    ps_global->sort_types[2]   = SortFrom;
    ps_global->sort_types[3]   = SortTo;
    ps_global->sort_types[4]   = SortCc;
    ps_global->sort_types[5]   = SortDate;
    ps_global->sort_types[6]   = SortSize;
    ps_global->sort_types[7]   = SortSubject2;
    ps_global->sort_types[8]   = EndofList;
    ps_global->atmts           = (ATTACH_S *) fs_get(sizeof(ATTACH_S));
    ps_global->atmts_allocated = 1;
    ps_global->atmts->description = NULL;
    ps_global->low_speed       = 1;
    ps_global->init_context    = -1;
    mn_init(&ps_global->msgmap, 0L);
    init_init_vars(ps_global);

#if !defined(DOS) && !defined(OS2)
    /*
     * Seed the random number generator with the date & pid.  Random 
     * numbers are used for new mail notification and bug report id's
     */
    srandom(getpid() + time(0));
#endif

#ifdef DYN
    /*-------------------------------------------------------------------
      There's a bug in DYNIX that causes the terminal driver to lose
      characters when large I/O writes are done on slow lines. Like
      a 1Kb write(2) on a 1200 baud line. Usually CR is output which
      causes a flush before the buffer is too full, some the pine composer
      doesn't output newlines a lot. Either stdio should be fixed to
      continue with more writes when the write request is partial, or
      fix the tty driver to always complete the write.
     */
    setbuffer(stdout, stdiobuf, 64);
#endif

    /* need home directory early */
    get_user_info(&ps_global->ui);
    if(getenv("HOME") != NULL)
      pine_state->home_dir = cpystr(getenv("HOME"));
    else
      pine_state->home_dir = cpystr(ps_global->ui.homedir);
 
#if defined(DOS) || defined(OS2)
    {
	char *p;

	/* normalize path delimiters */
	for(p = pine_state->home_dir; p = strchr(p, '/'); p++)
	  *p='\\';
    }
#endif

    /*----------------------------------------------------------------------
           Parse arguments and initialize debugging
      ----------------------------------------------------------------------*/
    folder_to_open = pine_args(pine_state, argc, argv, &argv);

#ifndef	_WINDOWS
    if(!isatty(0)){
	/*
	 * monkey with descriptors so our normal tty i/o routines don't
	 * choke...
	 */
	dup2(STDIN_FD, PIPED_FD);	/* redirected stdin to new desc */
	dup2(STDER_FD, STDIN_FD);	/* rebind stdin to the tty	*/
	stdin_getc = read_stdin_char;
    }
#endif

#ifdef DEBUG
    /* Since this is specific debugging we don't mind if the
       ifdef is the type of system. See conf/templets.h
     */
#ifdef HAVE_SMALLOC 
    if(debug > 8)
      malloc_debug(2);
#endif
#ifdef NXT
    if(debug > 8)
      malloc_debug(32); 
#endif
#ifdef	CSRIMALLOC
    mal_debug((debug <= DEFAULT_DEBUG) ? 1 : (debug < 9) ? 2 : 3);
#endif

    init_debug();

#ifdef	_WINDOWS
    mswin_setdebug(debug, debugfile);
#endif
#endif  /* DEBUG */

    /*------- Set up c-client drivers -------*/ 
#include "../c-client/linkage.c"

    /*------- Tune the drivers just installed -------*/ 
#if	defined(DOS) && !defined(WIN32)
    /*
     * install c-client callback to manage cache data outside
     * free memory
     */
    mail_parameters(NULL, SET_CACHE, (void *)dos_cache);

    /*
     * Sniff the environment for timezone offset.  We need to do this
     * here since Windows needs help figuring out UTC, and will adjust
     * what time() returns based on TZ.  THIS WILL SCREW US because
     * we use time() differences to manage status messages.  So, if 
     * rfc822_date, which calls localtime() and thus needs tzset(),
     * is called while a status message is displayed, it's possible
     * for time() to return a time *before* what we remember as the
     * time we put the status message on the display.  Sheesh.
     */
    tzset();
#else
    /*
     * Install our own gets routine so we can count the bytes read
     * during a fetch...
     */
    (void) mail_parameters(NULL, SET_GETS, (void *)pine_gets);
#endif
    /*
     * Give c-client a buffer to read "/user=" option names into.
     */
    (void) mail_parameters(NULL, SET_USERNAMEBUF, (void *) fs_get(256));

    init_vars(pine_state);

    /*
     * Set up a c-client read timeout and timeout handler.  In general,
     * it shouldn't happen, but a server crash or dead link can cause
     * pine to appear wedged if we don't set this up...
     */
    mail_parameters(NULL, SET_OPENTIMEOUT,
		    (void *)((pine_state->VAR_TCPOPENTIMEO
			      && (rv = atoi(pine_state->VAR_TCPOPENTIMEO)) > 4)
			       ? (long) rv : 30L));
    mail_parameters(NULL, SET_READTIMEOUT, (void *) 15L);
    mail_parameters(NULL, SET_TIMEOUT, (void *) pine_tcptimeout);
    if(pine_state->VAR_RSHOPENTIMEO
	&& ((rv = atoi(pine_state->VAR_RSHOPENTIMEO)) == 0 || rv > 4))
      mail_parameters(NULL, SET_RSHTIMEOUT, (void *) rv);

    if(init_username(pine_state) < 0)
      exit(-1);

    if(init_hostname(pine_state) < 0)
      exit(-1);

    if(!pine_state->nr_mode)
      write_pinerc(pine_state);

    /*
     * Verify mail dir if we're not in send only mode...
     */
    if(!pine_state->more_mode && *argv == NULL
	&& init_mail_dir(pine_state) < 0)
      exit(-1);

    init_signals();

    /*--- input side ---*/
    if(init_tty_driver(pine_state)){
#if !defined(DOS) && !defined(OS2)	/* always succeeds under DOS! */
        fprintf(stderr, "Can't access terminal or input is not a terminal. ");
        fprintf(stderr, "Redirection of\nstandard input is not allowed. For ");
        fprintf(stderr, "example \"pine < file\" doesn't work.\n%c", BELL);
        exit(-1);
#endif
    }
        

    /*--- output side ---*/
    rv = config_screen(&(pine_state->ttyo), &(pine_state->kbesc));
#if !defined(DOS) && !defined(OS2)	/* always succeeds under DOS! */
    if(rv){
        switch(rv){
          case -1:
	    printf("Terminal type (environment variable TERM) not set.\n");
            break;
          case -2:
	    printf("Terminal type \"%s\", is unknown.\n", getenv("TERM"));
            break;
          case -3:
            printf("Can't open termcap file; check TERMCAP");
	    printf(" variable and/or system manager.\n");
            break;
          case -4:
            printf("Your terminal, of type \"%s\",", getenv("TERM"));
	    printf(" is lacking functions needed to run pine.\n");
            break;
        }

        printf("\r");
        end_tty_driver(pine_state);
        exit(-1);
    }
#endif

    if(F_ON(F_BLANK_KEYMENU,ps_global))
      FOOTER_ROWS(ps_global) = 1;

    init_screen();
    init_keyboard(pine_state->orig_use_fkeys);
    strcpy(pine_state->inbox_name, INBOX_NAME);
    init_folders(pine_state);		/* digest folder spec's */

    pine_state->in_init_seq = 0;	/* so output (& ClearScreen) show up */
    pine_state->dont_use_init_cmds = 1;	/* don't use up initial_commands yet */
    ClearScreen();
    if(!pine_state->more_mode
       && (pine_state->first_time_user || pine_state->show_new_version)){
	pine_state->mangled_header = 1;
	show_main_screen(pine_state, 0, FirstMenu, NULL, 0, (Pos *)NULL);
	if(!pine_state->nr_mode){
	    if(pine_state->first_time_user)
	      new_user_or_version(first_time_message);
	    else
	      new_user_or_version(new_version_message);
	}

	ClearScreen();
    }
    
    /* put back in case we need to suppress output */
    pine_state->in_init_seq = pine_state->save_in_init_seq;

    /* queue any init errors so they get displayed in a screen below */
    queue_init_errors(ps_global);

    if(pine_state->more_mode){		/* "Page" the given file */
	int dice = 1, redir = 0;

	if(pine_state->in_init_seq){
	    pine_state->in_init_seq = pine_state->save_in_init_seq = 0;
	    clear_cursor_pos();
	    if(pine_state->free_initial_cmds)
	      fs_give((void **)&(pine_state->free_initial_cmds));

	    pine_state->initial_cmds = 0;
	}

	/*======= Requested that we simply page the given folder =======*/
	if(folder_to_open){		/* Open the requested folder... */
	    SourceType  src;
	    STORE_S    *store = NULL;
	    char       *decode_error = NULL;

	    if(stdin_getc){
		redir++;
		src = CharStar;
		if(isatty(0) && (store = so_get(src, NULL, EDIT_ACCESS))){
		    gf_io_t pc;

		    gf_set_so_writec(&pc, store);
		    gf_filter_init();
		    if(decode_error = gf_pipe(stdin_getc, pc)){
			dice = 0;
			q_status_message1(SM_ORDER, 3, 4,
					  "Problem reading stdin: %s",
					  decode_error);
		    }
		}
		else
		  dice = 0;
	    }
	    else{
		src = FileStar;
		strcpy(ps_global->cur_folder, folder_to_open);
		if((store = so_get(src, folder_to_open, READ_ACCESS)) == NULL)
		  dice = 0;
	    }

	    if(dice){
		scrolltool((void *)so_text(store), "FILE VIEW", SimpleText,
			   src, NULL);
		printf("\n\n");
		so_give(&store);
	    }
	}

	if(!dice){
	    q_status_message2(SM_ORDER, 3, 4, "Can't display \"%s\": %s",
		 (redir) ? "Standard Input" 
			 : folder_to_open ? folder_to_open : "NULL",
		 error_description(errno));
	}

	goodnight_gracey(pine_state, 0);
    }
    else if(*argv || stdin_getc){	/* send mail using given input */
        /*======= address on command line/send one message mode ============*/
        char *to, **t, *error = NULL, *addr;
        int   len, good_addr = 1;
	int   exit_val;
	BUILDER_ARG fcc;

	if(pine_state->in_init_seq){
	    pine_state->in_init_seq = pine_state->save_in_init_seq = 0;
	    clear_cursor_pos();
	    if(pine_state->free_initial_cmds)
	      fs_give((void **)&(pine_state->free_initial_cmds));

	    pine_state->initial_cmds = 0;
	}

        /*----- Format the To: line with commas for the composer ---*/
	if(argv){
	    for(t = argv, len = 0; *t != NULL; len += strlen(*t++) + 2)
	      ;/* do nothing */

	    to = fs_get(len + 5);
	    to[0] = '\0';
	    for(t = argv, len = 0; *t != NULL; t++){
		if(to[0] != '\0')
		  strcat(to, ", ");

		strcat(to, *t);
	    }

	    fcc.tptr = NULL;
	    fcc.next = NULL;
	    fcc.xtra = NULL;
	    good_addr = (build_address(to, &addr, &error, &fcc) >= 0);
	}

	if(good_addr)
	  compose_mail(addr, fcc.tptr, stdin_getc);

	if(addr)
	  fs_give((void **)&addr);

	if(fcc.tptr)
	  fs_give((void **)&fcc.tptr);

        fs_give((void **)&to);
	if(!good_addr){
	    char buf[500];

	    q_status_message1(SM_ORDER, 3, 4, "Bad address: %s", error);
	    exit_val = -1;
	}
	else
	  exit_val = 0;

	if(error)
	  fs_give((void **)&error);

	goodnight_gracey(pine_state, exit_val);
    }
    else{
        struct key_menu *km = &main_keymenu;

        /*========== Normal pine mail reading mode ==========*/
            
        pine_state->mail_stream    = NULL;
        pine_state->inbox_stream   = NULL;
        pine_state->mangled_screen = 1;
    
        if(!pine_state->start_in_index){
	    /* flash message about executing initial commands */
	    if(pine_state->in_init_seq){
	        pine_state->in_init_seq    = 0;
		clear_cursor_pos();
		pine_state->mangled_header = 1;
		pine_state->mangled_footer = 1;
		pine_state->mangled_screen = 0;
		/* show that this is Pine */
		show_main_screen(pine_state, 0, FirstMenu, km, 0, (Pos *)NULL);
		pine_state->mangled_screen = 1;
		pine_state->painted_footer_on_startup = 1;
		if(min(4, pine_state->ttyo->screen_rows - 4) > 1)
	          PutLine0(min(4, pine_state->ttyo->screen_rows - 4),
		    max(min(11, pine_state->ttyo->screen_cols -38), 0),
		    "Executing initial-keystroke-list......");

	        pine_state->in_init_seq = 1;
	    }
	    else{
                show_main_screen(pine_state, 0, FirstMenu, km, 0, (Pos *)NULL);
		pine_state->painted_body_on_startup   = 1;
		pine_state->painted_footer_on_startup = 1;
	    }
        }
	else{
	    /* cancel any initial commands, overridden by cmd line */
	    if(pine_state->in_init_seq){
		pine_state->in_init_seq      = 0;
		pine_state->save_in_init_seq = 0;
		clear_cursor_pos();
		if(pine_state->initial_cmds){
		    if(pine_state->free_initial_cmds)
		      fs_give((void **)&(pine_state->free_initial_cmds));

		    pine_state->initial_cmds = 0;
		}

		F_SET(F_USE_FK,pine_state, pine_state->orig_use_fkeys);
	    }

            do_index_border(pine_state->context_current,
			    pine_state->cur_folder, pine_state->mail_stream,
			    pine_state->msgmap, MsgIndex, NULL,
			    INDX_CLEAR|INDX_HEADER|INDX_FOOTER);
	    pine_state->painted_footer_on_startup = 1;
	    if(min(4, pine_state->ttyo->screen_rows - 4) > 1)
	      PutLine1(min(4, pine_state->ttyo->screen_rows - 4),
		max(min(11, pine_state->ttyo->screen_cols -40), 0),
		"Please wait, opening %s......",
		 pine_state->nr_mode ? "news messages" : "mail folder");
        }

        fflush(stdout);

	if(pine_state->in_init_seq){
	    pine_state->in_init_seq = 0;
	    clear_cursor_pos();
	}

        if(folder_to_open != NULL){
	    CONTEXT_S *cntxt;

	    if((rv = pine_state->init_context) < 0)
	      cntxt = pine_state->context_current;
	    else if(rv == 0)
	      cntxt = NULL;
	    else
	      for(cntxt = pine_state->context_list;
		  rv > 1 && cntxt->next;
		  rv--, cntxt = cntxt->next)
		;

            if(do_broach_folder(folder_to_open, cntxt) <= 0){
		q_status_message2(SM_ORDER, 3, 4,
		    "Unable to open %s \"%s\"",
		    pine_state->nr_mode ? "news messages" : "folder",
		    folder_to_open);

		goodnight_gracey(pine_state, -1);
            }
        }
	else{
#if defined(DOS) || defined(OS2)
            /*
	     * need to ask for the inbox name if no default under DOS
	     * since there is no "inbox"
	     */

	    if(!pine_state->VAR_INBOX_PATH || !pine_state->VAR_INBOX_PATH[0]
	       || strucmp(pine_state->VAR_INBOX_PATH, "inbox") == 0){
		HelpType help = NO_HELP;
		static   ESCKEY_S ekey[] = {{ctrl(T), 2, "^T", "To Fldrs"},
					  {-1, 0, NULL, NULL}};

		pine_state->mangled_footer = 1;
		int_mail[0] = '\0';
    		while(1){
        	    rv = optionally_enter(int_mail, -FOOTER_ROWS(pine_state),
				      0, MAXPATH, 1, 0,
				      "No inbox!  Folder to open as inbox : ",
				      ekey, help, 0);
        	    if(rv == 3){
			help = (help == NO_HELP) ? h_sticky_inbox : NO_HELP;
			continue;
		    }

        	    if(rv != 4)
		      break;
    		}

    		if(rv == 1){
		    q_status_message(SM_ORDER, 0, 2 ,"Folder open cancelled");
		    rv = 0;		/* reset rv */
		} 
		else if(rv == 2){
        	    if(!folder_lister(pine_state, OpenFolder, NULL,
			       &(pine_state->context_current),
			       int_mail, NULL,
			       pine_state->context_current, NULL))
		      *int_mail = '\0';	/* user cancelled! */

                    show_main_screen(pine_state,0,FirstMenu,km,0,(Pos *)NULL);
		}

		if(*int_mail){
		    removing_trailing_white_space(int_mail);
		    removing_leading_white_space(int_mail);
		    if((!pine_state->VAR_INBOX_PATH 
			|| strucmp(pine_state->VAR_INBOX_PATH, "inbox") == 0)
		     && want_to("Preserve folder as \"inbox-path\" in PINERC", 
				'y', 'n', NO_HELP, 0, 0) == 'y'){
			set_variable(V_INBOX_PATH, int_mail, 1);
		    }
		    else{
			if(pine_state->VAR_INBOX_PATH)
			  fs_give((void **)&pine_state->VAR_INBOX_PATH);

			pine_state->VAR_INBOX_PATH = cpystr(int_mail);
		    }

		    do_broach_folder(pine_state->inbox_name, 
				     pine_state->context_list);
    		}
		else
		  q_status_message(SM_ORDER, 0, 2 ,"No folder opened");

	    }
	    else

#endif
            do_broach_folder(pine_state->inbox_name, pine_state->context_list);
        }

        if(pine_state->mangled_footer)
	  pine_state->painted_footer_on_startup = 0;

        if(!pine_state->nr_mode
	   && pine_state->mail_stream
	   && expire_sent_mail())
	  pine_state->painted_footer_on_startup = 0;

	/*
	 * Initialize the defaults.  Initializing here means that
	 * if they're remote, the user isn't prompted for an imap login
	 * before the display's drawn, AND there's the chance that
	 * we can climb onto the already opened folder's stream...
	 */
	if(ps_global->first_time_user || ps_global->show_new_version)
	  init_save_defaults();	/* initialize default save folders */

	build_path(int_mail,
		   ps_global->VAR_OPER_DIR ? ps_global->VAR_OPER_DIR
					   : pine_state->home_dir,
		   INTERRUPTED_MAIL);
	if(!pine_state->nr_mode && folder_exists("[]", int_mail) > 0)
	  q_status_message(SM_ORDER | SM_DING, 4, 5, 
		       "Use compose command to continue interrupted message.");

#if defined(USE_QUOTAS)
	{
	    long q;
	    int  over;
	    q = disk_quota(pine_state->home_dir, &over);
	    if(q > 0 && over){
		q_status_message2(SM_ASYNC | SM_DING, 4, 5,
			      "WARNING! Over your disk quota by %s bytes (%s)",
			      comatose(q),byte_string(q));
	    }
	}
#endif

	pine_state->in_init_seq = pine_state->save_in_init_seq;
	pine_state->dont_use_init_cmds = 0;
	clear_cursor_pos();

	if(pine_state->give_fixed_warning)
	  q_status_message(SM_ASYNC, 0, 10,
"Note: some of your config options conflict with site policy and are ignored");

	if(timeout == 0 &&
	   ps_global->VAR_INBOX_PATH &&
	   ps_global->VAR_INBOX_PATH[0] == '{')
	  q_status_message(SM_ASYNC, 0, 10,
"Note: mail-check-interval=0 may cause IMAP server connection to time out");

        /*-------------------------------------------------------------------
                         Loop executing the commands
    
            This is done like this so that one command screen can cause
            another one to execute it with out going through the main menu. 
          ------------------------------------------------------------------*/
        pine_state->next_screen = pine_state->start_in_index ?
                                         mail_index_screen : 
                                         main_menu_screen;
        while(1){
            if(pine_state->next_screen == SCREEN_FUN_NULL) 
              pine_state->next_screen = main_menu_screen;

            (*(pine_state->next_screen))(pine_state);
        }
    }
}



/*
 * read_stdin_char - simple function to return a character from
 *		     redirected stdin
 */
int
read_stdin_char(c)
    char *c;
{
    /* it'd probably be a good idea to fix this to pre-read blocks */
    return(read(PIPED_FD, c, 1) == 1);
#ifdef	notdef

		    gf_io_t pc;
		    char    bigbuf[1025];
		    int     i;

		    strcpy(ps_global->cur_folder, "Standard-Input");
		    gf_set_so_writec(&pc, store);
		    while((i = read(PIPED_FD, bigbuf, 1024)) > 0){
			bigbuf[i] = '\0';
			gf_puts(bigbuf, pc);
		    }

		    if(i < 0)		/* Bummer. */
		      dice = 0;
#endif
}


/* this default is from the array of structs below */
#define DEFAULT_MENU_ITEM 6		/* LIST FOLDERS */
#define MAX_DEFAULT_MENU_ITEM 12
#define UNUSED 0
static unsigned char current_default_menu_item = DEFAULT_MENU_ITEM;

/*
 * One of these for each line that gets printed in the middle of the
 * screen in the main menu.
 */
static struct menu_key {
    char         *key_and_name,
		 *news_addition;
    unsigned int f_key;           /* function key that invokes this action */
    unsigned int key;             /* alpha key that invokes this action */
    unsigned int keymenu_number;  /* index into keymenu array for this cmd */
} mkeys[] = {
    {" %s     HELP               -  Get help using Pine",
       NULL, PF1, '?', MAIN_HELP_KEY},
    {"", NULL, UNUSED, UNUSED, UNUSED},
    {" %s     COMPOSE MESSAGE    -  Compose and send%s a message",
       "/post", OPF4, 'C', MAIN_COMPOSE_KEY},
    {"", NULL, UNUSED, UNUSED, UNUSED},
    {" %s     FOLDER INDEX       -  View messages in current folder",
       NULL, OPF7, 'I', MAIN_INDEX_KEY},
    {"", NULL, UNUSED, UNUSED, UNUSED},
    {" %s     FOLDER LIST        -  Select a folder%s to view",
       " OR news group", OPF5, 'L', MAIN_FOLDER_KEY},
    {"", NULL, UNUSED, UNUSED, UNUSED},
    {" %s     ADDRESS BOOK       -  Update address book",
       NULL, OPF10, 'A', MAIN_ADDRESS_KEY},
    {"", NULL, UNUSED, UNUSED, UNUSED},
    {" %s     SETUP              -  Configure or update Pine",
       NULL, OPF9, 'S', MAIN_SETUP_KEY},
    {"", NULL, UNUSED, UNUSED, UNUSED},
    {" %s     QUIT               -  Exit the Pine program",
       NULL, OPF3, 'Q', MAIN_QUIT_KEY},
    {NULL, NULL, UNUSED, UNUSED, UNUSED}
};



/*----------------------------------------------------------------------
      display main menu and execute main menu commands

    Args: The usual pine structure

  Result: main menu commands are executed


              M A I N   M E N U    S C R E E N

   Paint the main menu on the screen, get the commands and either execute
the function or pass back the name of the function to execute for the menu
selection. Only simple functions that always return here can be executed
here.

This functions handling of new mail, redrawing, errors and such can 
serve as a template for the other screen that do much the same thing.

There is a loop that fetchs and executes commands until a command to leave
this screen is given. Then the name of the next screen to display is
stored in next_screen member of the structure and this function is exited
with a return.

First a check for new mail is performed. This might involve reading the new
mail into the inbox which might then cause the screen to be repainted.

Then the general screen painting is done. This is usually controlled
by a few flags and some other position variables. If they change they
tell this part of the code what to repaint. This will include cursor
motion and so on.
  ----*/
void
main_menu_screen(pine_state)
    struct pine *pine_state;
{
    int		    ch, orig_ch, setup_command,
		    just_a_navigate_cmd, km_popped;
    char            *new_folder;
    CONTEXT_S       *tc;
    struct key_menu *km;
    OtherMenu        what;
    Pos              curs_pos;
#if defined(DOS) || defined(OS2)
    extern void (*while_waiting)();
#endif

    ps_global                 = pine_state;
    just_a_navigate_cmd       = 0;
    km_popped		      = 0;
    current_default_menu_item = DEFAULT_MENU_ITEM;
    what                      = FirstMenu;  /* which keymenu to display */
    ch                        = 'x'; /* For display_message 1st time through */
    pine_state->prev_screen   = main_menu_screen;
    curs_pos.row = pine_state->ttyo->screen_rows-FOOTER_ROWS(pine_state);
    curs_pos.col = 0;

    mailcap_free(); /* free resources we won't be using for a while */

    if(!pine_state->painted_body_on_startup 
       && !pine_state->painted_footer_on_startup){
	pine_state->mangled_screen = 1;
    }
    else{
	/* need cursor position if not drawing */
	char buf[MAX_SCREEN_COLS+1];

	/* This only works because default is the longest one */
	sprintf(buf, mkeys[current_default_menu_item].key_and_name,
	    F_ON(F_USE_FK,ps_global)
	      ? "" : pretty_command(mkeys[current_default_menu_item].key),
	    (ps_global->VAR_NEWS_SPEC &&
		    mkeys[current_default_menu_item].news_addition)
	      ? mkeys[current_default_menu_item].news_addition : "");
	curs_pos.col = max(((ps_global->ttyo->screen_cols-strlen(buf))/2)-1, 0);
	curs_pos.col += 6;
	if(F_OFF(F_USE_FK,ps_global))
	  curs_pos.col++;
	
	curs_pos.col = min(ps_global->ttyo->screen_cols-1, curs_pos.col);
	curs_pos.row = current_default_menu_item + 3;
    }

    km = &main_keymenu;

    dprint(1, (debugfile, "\n\n    ---- MAIN_MENU_SCREEN ----\n"));

    while(1){
	if(km_popped){
	    km_popped--;
	    if(km_popped == 0){
		clearfooter(pine_state);
		pine_state->mangled_body = 1;
	    }
	}

	/*
	 * fix up redrawer just in case some submenu caused it to get
	 * reassigned...
	 */
	pine_state->redrawer = main_redrawer;

	/*----------- Check for new mail -----------*/
        if(new_mail(0, NM_TIMING(ch), 1) >= 0)
          pine_state->mangled_header = 1;

        if(streams_died())
          pine_state->mangled_header = 1;

        show_main_screen(pine_state, just_a_navigate_cmd, what, km,
			 km_popped, &curs_pos);
        just_a_navigate_cmd = 0;
	what = SameTwelve;

	/*---- This displays new mail notification, or errors ---*/
	if(km_popped){
	    FOOTER_ROWS(pine_state) = 3;
	    mark_status_dirty();
	}

        display_message(ch);
	if(km_popped){
	    FOOTER_ROWS(pine_state) = 1;
	    mark_status_dirty();
	}

	if(F_OFF(F_SHOW_CURSOR, ps_global)){
	    curs_pos.row =pine_state->ttyo->screen_rows-FOOTER_ROWS(pine_state);
	    curs_pos.col =0;
	}

        MoveCursor(curs_pos.row, curs_pos.col);

        /*------ Read the command from the keyboard ----*/      
#ifdef	MOUSE
	mouse_in_content(KEY_MOUSE, -1, -1, 0, 0);
	register_mfunc(mouse_in_content, HEADER_ROWS(pine_state), 0,
		    pine_state->ttyo->screen_rows-(FOOTER_ROWS(pine_state)+1),
		       pine_state->ttyo->screen_cols);
#endif
#if defined(DOS) || defined(OS2)
	/*
	 * AND pre-build header lines.  This works just fine under
	 * DOS since we wait for characters in a loop. Something will
         * will have to change under UNIX if we want to do the same.
	 */
	while_waiting = build_header_cache;
#endif
        ch = read_command();
#ifdef	MOUSE
	clear_mfunc(mouse_in_content);
#endif
#if defined(DOS) || defined(OS2)
	while_waiting = NULL;
#endif
        orig_ch = ch;

        if(ch < 0x0100 && isupper((unsigned char)ch))
          ch = tolower((unsigned char)ch);
	else if(ch >= PF1 && ch <= PF12 && km->which == 1)
	  ch = PF2OPF(ch);

	/*----- Validate the command ----*/
	if(ch == ctrl('M') || ch == ctrl('J') || ch == PF4){
	  ch = F_ON(F_USE_FK,pine_state)
                                       ? mkeys[current_default_menu_item].f_key
                                       : mkeys[current_default_menu_item].key;
          if(ch <= 0xff && isupper((unsigned char)ch))
            ch = tolower((unsigned char)ch);
	}


        /*
	 * 'q' is always valid as a way to exit pine even in function key mode
         */
        if(ch != 'q')
          ch = validatekeys(ch);

	if(km_popped)
	  switch(ch){
	    case NO_OP_IDLE:
	    case NO_OP_COMMAND: 
	    case PF2:
	    case OPF2:
            case 'o' :
	    case KEY_RESIZE:
	    case ctrl('L'):
	      km_popped++;
	      break;
	    
	    default:
	      clearfooter(pine_state);
	      break;
	  }

	/*------ Execute the command ------*/
	switch (ch){
#if	defined(DOS) && !defined(_WINDOWS)
/* while we're testing DOS */
	  case 'h': 
	    {
#include <malloc.h>
		int    heapresult;
		int    totalused = 0;
		int    totalfree = 0;
		long   totalusedbytes = 0L;
		long   totalfreebytes = 0L;
		long   largestinuse = 0L;
		long   largestfree = 0L, freeaccum = 0L;

		_HEAPINFO hinfo;
		extern long coreleft();
		extern void dumpmetacache();

		hinfo._pentry = NULL;
		while((heapresult = _heapwalk(&hinfo)) == _HEAPOK){
		    if(hinfo._useflag == _USEDENTRY){
			totalused++;
			totalusedbytes += (long)hinfo._size; 
			if(largestinuse < (long)hinfo._size)
			  largestinuse = (long)hinfo._size;
		    }
		    else{
			totalfree++;
			totalfreebytes += (long)hinfo._size;
		    }

		    if(hinfo._useflag == _USEDENTRY){
			if(freeaccum > largestfree) /* remember largest run */
			  largestfree = freeaccum;
			
			freeaccum = 0L;
		    }
		    else
		      freeaccum += (long)hinfo._size;
		}

		sprintf(tmp_20k_buf,
		  "use: %d (%ld, %ld lrg), free: %d (%ld, %ld lrg), DOS: %ld", 
			totalused, totalusedbytes, largestinuse,
			totalfree, totalfreebytes, largestfree, coreleft());
		q_status_message(SM_ORDER, 5, 7, tmp_20k_buf);

		switch(heapresult/* = _heapchk()*/){
		  case _HEAPBADPTR:
		    q_status_message(SM_ORDER | SM_DING, 1, 2,
				     "ERROR - Bad ptr in heap");
		    break;
		  case _HEAPBADBEGIN:
		    q_status_message(SM_ORDER | SM_DING, 1, 2,
				     "ERROR - Bad start of heap");
		    break;
		  case _HEAPBADNODE:
		    q_status_message(SM_ORDER | SM_DING, 1, 2,
				     "ERROR - Bad node in heap");
		    break;
		  case _HEAPEMPTY:
		    q_status_message(SM_ORDER, 1, 2, "Heap OK - empty");
		    break;
		  case _HEAPEND:
		    q_status_message(SM_ORDER, 1, 2, "Heap checks out!");
		    break;
		  case _HEAPOK:
		    q_status_message(SM_ORDER, 1, 2, "Heap checks out!");
		    break;
		  default:
		    q_status_message1(SM_ORDER | SM_DING, 1, 2,
				      "BS from heapchk: %d",
				      (void *)heapresult);
		    break;
		}

		/*       dumpmetacache(ps_global->mail_stream);*/
		/* DEBUG: heapcheck() */
		/*       q_status_message1(SM_ORDER, 1, 3,
                         " * * There's %ld bytes of core left for Pine * * ", 
                         (void *)coreleft());*/
	    }
	    break;
#endif	/* DOS for testing */

	  /*---------- help ------*/
	  case PF1:
	  case OPF1:
	  case ctrl('G'):
	  case '?':
	  help_case :
	    if(FOOTER_ROWS(pine_state) == 1 && km_popped == 0){
		km_popped = 2;
		pine_state->mangled_footer = 1;
		break;
	    }

            helper(main_menu_tx, "HELP FOR MAIN MENU", 0);
	    pine_state->mangled_screen = 1;
	    break;
  
	  /*---------- display other key bindings ------*/
	  case PF2:
	  case OPF2:
          case 'o' :
            if(ch == 'o')
	      warn_other_cmds();
	    what = NextTwelve;
	    pine_state->mangled_footer = 1;
	    break;

	  /* case PF4: */
	  /* PF4 is handled above switch */

          /*---------- Previous item in menu ----------*/
	  case PF5:
	  case 'p':
	  case ctrl('P'):
	  case KEY_UP:
	    if(current_default_menu_item > 1) {
	      current_default_menu_item -= 2;  /* 2 to skip the blank lines */
	      pine_state->mangled_body = 1;
	      if(km->which == 0)
	        pine_state->mangled_footer = 1;
	      just_a_navigate_cmd++;
	    }else {
	      q_status_message(SM_ORDER, 0, 2, "Already at top of list");
	    }
	    break;

          /*---------- Next item in menu ----------*/
	  case PF6:
	  case 'n':
	  case ctrl('N'):
	  case KEY_DOWN:
	    if(current_default_menu_item < (unsigned)(MAX_DEFAULT_MENU_ITEM-1)){
	      current_default_menu_item += 2;
	      pine_state->mangled_body = 1;
	      if(km->which == 0)
	        pine_state->mangled_footer = 1;
	      just_a_navigate_cmd++;
	    }else {
	      q_status_message(SM_ORDER, 0, 2, "Already at bottom of list");
	    }
	    break;

          /*---------- Release Notes ----------*/
	  case PF9:
	  case 'r':
	    helper(h_news, "NEWS ABOUT PINE", 0);
	    pine_state->mangled_screen = 1;
	    break;

#ifndef NO_KEYBOARD_LOCK
          /*---------- Keyboard lock ----------*/
	  case PF10:
	  case 'k':
	    if(ps_global->restricted || F_ON(F_DISABLE_KBLOCK_CMD,ps_global))
              goto bleep;

            (void)lock_keyboard();
	    pine_state->mangled_screen = 1;
	    break;
#endif /* !NO_KEYBOARD_LOCK */

          /*---------- Quit pine ----------*/
	  case OPF3: 
	  case 'q':
	  quit_case :
	    pine_state->next_screen = quit_screen;
	    return;
  
          /*---------- Go to composer ----------*/
	  case OPF4:
	  case 'c':
	  compose_case :
	    pine_state->next_screen = compose_screen;
	    return;
  
          /*---------- Folders ----------*/
	  case OPF5: 
	  case 'l':
	  folder_case :
	    pine_state->next_screen = folder_screen;
	    return;

          /*---------- Old Folders Command ----------*/
	  case 'f':
	    q_status_message(SM_ORDER, 0, 2, "Use \"L\" to list Folders");
	    break;

          /*---------- Goto new folder ----------*/
	  case OPF6:
	  case 'g':
	    tc = ps_global->context_current;
            new_folder = broach_folder(-FOOTER_ROWS(pine_state), 1, &tc);
#if	defined(DOS) && !defined(_WINDOWS)
	    if(new_folder && *new_folder == '{' && coreleft() < 20000){
	      q_status_message(SM_ORDER | SM_DING, 3, 3,
			       "Not enough memory to open IMAP folder");
	      new_folder = NULL;
	    }
#endif
            if(new_folder)
	      visit_folder(ps_global, new_folder, tc);

	    return;

          /*---------- Go to index ----------*/
	  case OPF7:
	  case 'i':
	  index_case :
	    pine_state->next_screen = mail_index_screen;
	    return;

	  case OPF8:
	  case 'j':
	    review_messages("REVIEW RECENT MESSAGES");
	    pine_state->mangled_screen = 1;
	    break;

          /*---------- Setup mini menu ----------*/
	  case OPF9:
	  case 's':
	  setup_case :
	    setup_command = setup_mini_menu(-FOOTER_ROWS(pine_state));
	    pine_state->mangled_footer = 1;
	    do_setup_task(setup_command);
            if(ps_global->next_screen != main_menu_screen)
	      return;
	    break;

          /*---------- Go to address book ----------*/
	  case OPF10 :
	  case 'a':
	  addrbook_case :
	    pine_state->next_screen = addr_book_screen;
	    return;
  
          /*---------- report a bug ----------*/
	  case OPF11 :
	  case 'b':
	    gripe(ps_global);
	    break;

#ifdef	DEBUG
	  case '1':
	  case '2':
	  case '3':
	  case '4':
	  case '5':
	  case '6':
	  case '7':
	  case '8':
	  case '9':
	    if(!debug)		/* must have started with debugging */
	      goto bleep;

	    debug = ch - '0';
	    dprint(1, (debugfile, "*** Debug level set to %d ***\n", debug));
	    fflush(debugfile);
	    q_status_message1(SM_ORDER, 0, 1, "Debug level set to %s",
			      int2string(debug));
	    break;
#endif	/* DEBUG */

          case KEY_RESIZE:
	  case ctrl('L'):
	    ClearScreen();
	    pine_state->mangled_screen = 1;
	    break;
  
#ifdef	MOUSE
	  case KEY_MOUSE:
	    {   
		MOUSEPRESS mp;
		unsigned char ndmi;

		mouse_get_last (NULL, &mp);
		ndmi = mp.row - 3;
		if (mp.row >= 3 && !(ndmi & 0x01)
		    && ndmi <= (unsigned)MAX_DEFAULT_MENU_ITEM
		    && ndmi < pine_state->ttyo->screen_rows
				       - 4 - FOOTER_ROWS(ps_global)) {
		    if(mp.doubleclick){
			switch(ndmi){		/* fake main_screen request */
			  case 0 :
			    goto help_case;

			  case 2 :
			    goto compose_case;

			  case 4 :
			    goto index_case;

			  case 6 :
			    goto folder_case;

			  case 8 :
			    goto addrbook_case;

			  case 10 :
			    goto setup_case;

			  case 12 :
			    goto quit_case;

			  default:			/* no op */
			    break;
			}
		    }
		    else{
			current_default_menu_item = ndmi;
			pine_state->mangled_body = 1;
			if(km->which == 0)
			  pine_state->mangled_footer = 1;
			just_a_navigate_cmd++;
		    }
		}
	    }
	    break;
#endif

	  case NO_OP_COMMAND :
          case NO_OP_IDLE:
            break;	/* noop for timeout loop mail check */
  
	  default:
          bleep:
	    bogus_command(orig_ch, F_ON(F_USE_FK,pine_state) ? "F1" : "?");
	    break;
	 } /* the switch */
    } /* the BIG while loop! */
}



/*----------------------------------------------------------------------
    Re-Draw the main menu

    Args: none.

  Result: main menu is re-displayed
  ----*/
void
main_redrawer()
{
    struct key_menu *km = &main_keymenu;

    ps_global->mangled_screen = 1;
    show_main_screen(ps_global, 0, FirstMenu, km, 0, (Pos *)NULL);
}


	
/*----------------------------------------------------------------------
         Draw the main menu

    Args: pine_state - the usual struct
	  quick_draw - tells do_menu() it can skip some drawing
	  what       - tells which section of keymenu to draw
	  km         - the keymenu
	  cursor_pos - returns a good position for the cursor to be located

  Result: main menu is displayed
  ----*/
void
show_main_screen(ps, quick_draw, what, km, km_popped, cursor_pos)
    struct pine     *ps;
    int		     quick_draw;
    OtherMenu	     what;
    struct key_menu *km;
    int		     km_popped;
    Pos             *cursor_pos;
{
    if(ps->painted_body_on_startup || ps->painted_footer_on_startup){
	ps->mangled_screen = 0;		/* only worry about it here */
	ps->mangled_header = 1;		/* we have to redo header */
	if(!ps->painted_body_on_startup)
	  ps->mangled_body = 1;		/* make sure to paint body*/

	if(!ps->painted_footer_on_startup)
	  ps->mangled_footer = 1;	/* make sure to paint footer*/

	ps->painted_body_on_startup   = 0;
        ps->painted_footer_on_startup = 0;
    }

    if(ps->mangled_screen){
	ps->mangled_header = 1;
	ps->mangled_body   = 1;
	ps->mangled_footer = 1;
	ps->mangled_screen = 0;
    }

#ifdef _WINDOWS
    /* Reset the scroll range.  Main screen never scrolls. */
    scroll_setrange (0L);
#endif

    /* paint the titlebar if needed */
    if(ps->mangled_header){
	set_titlebar("MAIN MENU", ps->mail_stream, ps->context_current,
		     ps->cur_folder, ps->msgmap, 1, FolderName, 0, 0);
	ps->mangled_header = 0;
    }

    /* paint the body if needed */
    if(ps->mangled_body){
	if(!quick_draw)
	  ClearBody();

	do_menu(quick_draw, cursor_pos);
	ps->mangled_body = 0;
    }

    /* paint the keymenu if needed */
    if(ps->mangled_footer){
	bitmap_t    bitmap;
	static char label[LONGEST_LABEL + 2 + 1], /* label + brackets + \0 */
		    name[max(LONGEST_NAME,3)+1];  /* longest name+1 (3=F12) */

	setbitmap(bitmap);

#ifndef NO_KEYBOARD_LOCK
	if(ps_global->restricted || F_ON(F_DISABLE_KBLOCK_CMD,ps_global))
#endif
	  clrbitn(MAIN_KBLOCK_KEY, bitmap);

	/* put brackets around the default action */
	label[0] = '[';  label[1] = '\0';
	strcat(label,
	      km->keys[mkeys[current_default_menu_item].keymenu_number].label);
	strcat(label, "]");
	km->keys[MAIN_DEFAULT_KEY].label = label;
	sprintf(name, "%s", F_ON(F_USE_FK,ps_global) ?
		pretty_command(mkeys[current_default_menu_item].f_key) :
		pretty_command(mkeys[current_default_menu_item].key));
	km->keys[MAIN_DEFAULT_KEY].name = name;
	if(km_popped){
	    FOOTER_ROWS(ps) = 3;
	    clearfooter(ps);
	}

	draw_keymenu(km, bitmap, ps_global->ttyo->screen_cols,
	    1-FOOTER_ROWS(ps_global), 0, what, 0);
	ps->mangled_footer = 0;
	if(km_popped){
	    FOOTER_ROWS(ps) = 1;
	    mark_keymenu_dirty();
	}
    }
}


/*----------------------------------------------------------------------
         Actually display the main menu

    Args: quick_draw - just a next or prev command was typed so we only have
		       to redraw the highlighting
          cursor_pos - a place to return a good value for cursor location

  Result: Main menu is displayed
  ---*/
void
do_menu(quick_draw, cursor_pos)
    int  quick_draw;
    Pos *cursor_pos;
{
    int  dline, indent, longest = 0, i;
    char buf[MAX_DEFAULT_MENU_ITEM+1][MAX_SCREEN_COLS+1], *p;
    static int last_inverse = -1;
    Pos pos;


    /*
     * Build all the menu lines...
     */
    for(dline = 0;
	mkeys[dline].key_and_name && dline < ps_global->ttyo->screen_rows - 7;
	dline++){
	memset((void *)buf[dline], ' ', MAX_SCREEN_COLS * sizeof(char));
        sprintf(buf[dline], mkeys[dline].key_and_name,
                F_ON(F_USE_FK,ps_global)
		  ? "" : pretty_command(mkeys[dline].key),
		(ps_global->VAR_NEWS_SPEC && mkeys[dline].news_addition)
		  ? mkeys[dline].news_addition : "");

	if(longest < (indent = strlen(buf[dline])))
	  longest = indent;

	buf[dline][indent] = ' ';	/* buf's really tied off below */
    }

    indent = max(((ps_global->ttyo->screen_cols - longest)/2) - 1, 0);

    /* leave room for keymenu, status line, and trademark message */
    for(dline = 3;
	mkeys[dline-3].key_and_name
	    && dline < ps_global->ttyo->screen_rows-(FOOTER_ROWS(ps_global)+1);
	dline++){
	if(quick_draw && !(dline-3 == last_inverse
			   || dline-3 == current_default_menu_item))
	  continue;

	if(dline-3 == current_default_menu_item)
	  StartInverse();

	buf[dline-3][min(ps_global->ttyo->screen_cols-indent,longest+1)]= '\0';
	pos.row = dline;
	pos.col = indent;
        PutLine0(pos.row, pos.col, buf[dline-3]);

	if(dline-3 == current_default_menu_item){
	    if(cursor_pos){
		cursor_pos->row = pos.row;
		cursor_pos->col = pos.col + 6;
		if(F_OFF(F_USE_FK,ps_global))
		  cursor_pos->col++;
	    }

	    EndInverse();
	}
    }

    last_inverse = current_default_menu_item;

    if(!quick_draw)	/* the devi.. uh, I mean, lawyer made me do it. */
      PutLine0(ps_global->ttyo->screen_rows - (FOOTER_ROWS(ps_global)+1),
	  3, LEGAL_NOTICE);

    fflush(stdout);
}


/*----------------------------------------------------------------------

Args: ql -- Line to prompt on
Returns: character selected

  ----*/
int
setup_mini_menu(ql)
     int ql;
{
    char        prompt[80];
    char        letters[20];
    char        *printer  = "Printer";
    char        *passwd   = "Newpassword";
    char        *config   = "Config";
    char        *update   = "Update";
    char	*sigedit  = "Signature";
    HelpType     help     = h_mini_setup;
    int          deefault = 'p';
    int          s, ekey_num;
    ESCKEY_S     setup_names[6];

    ekey_num = 0;

    /* There isn't anything that is allowed */
    if(ps_global->vars[V_PRINTER].is_fixed &&
       F_ON(F_DISABLE_PASSWORD_CMD,ps_global) &&
       F_ON(F_DISABLE_CONFIG_SCREEN,ps_global) &&
       F_ON(F_DISABLE_UPDATE_CMD,ps_global) && 
       F_ON(F_DISABLE_SIGEDIT_CMD,ps_global)){
	  q_status_message(SM_ORDER | SM_DING, 3, 4,
		     "All setup tasks turned off or prohibited by Sys. Mgmt.");
	  return('e');
    }

    if(!ps_global->vars[V_PRINTER].is_fixed){ /* printer can be changed */
	setup_names[ekey_num].ch      = 'p';
	setup_names[ekey_num].rval    = 'p';
	setup_names[ekey_num].name    = "P";
	setup_names[ekey_num++].label = printer;
    }

    if(F_OFF(F_DISABLE_PASSWORD_CMD,ps_global)){ /* password is allowed */
	setup_names[ekey_num].ch      = 'n';
	setup_names[ekey_num].rval    = 'n';
	setup_names[ekey_num].name    = "N";
	setup_names[ekey_num++].label = passwd;
    }

    if(F_OFF(F_DISABLE_CONFIG_SCREEN,ps_global)){ /* config allowed */
	setup_names[ekey_num].ch      = 'c';
	setup_names[ekey_num].rval    = 'c';
	setup_names[ekey_num].name    = "C";
	setup_names[ekey_num++].label = config;
    }

    if(F_OFF(F_DISABLE_UPDATE_CMD,ps_global)){ /* update is allowed */
	setup_names[ekey_num].ch      = 'u';
	setup_names[ekey_num].rval    = 'u';
	setup_names[ekey_num].name    = "U";
	setup_names[ekey_num++].label = update;
    }

    if(F_OFF(F_DISABLE_SIGEDIT_CMD,ps_global)){ /* .sig editing is allowed */
	setup_names[ekey_num].ch      = 's';
	setup_names[ekey_num].rval    = 's';
	setup_names[ekey_num].name    = "S";
	setup_names[ekey_num++].label = sigedit;
    }

    setup_names[ekey_num].ch    = -1;

    if(F_ON(F_BLANK_KEYMENU,ps_global)){
	char *p;

	p = letters;
	*p = '\0';
	for(ekey_num = 0; setup_names[ekey_num].ch != -1; ekey_num++){
	    *p++ = setup_names[ekey_num].ch;
	    if(setup_names[ekey_num + 1].ch != -1)
	      *p++ = ',';
	}

	*p = '\0';
    }

    sprintf(prompt,
	    "Choose a setup task from %s : ",
	    F_ON(F_BLANK_KEYMENU,ps_global) ? letters : "the menu below");

    s = radio_buttons(prompt, ql, setup_names, deefault, 'x', help, RB_NORM);
    /* ^C */
    if(s == 'x') {
	q_status_message(SM_ORDER,0,3,"Setup command cancelled");
	s = 'e';
    }

    return(s);
}


/*----------------------------------------------------------------------

Args: command -- command char to perform

  ----*/
void
do_setup_task(command)
    int command;
{
    switch(command) {
        /*----- UPDATE -----*/
      case 'u':
	{
	    char update_folder[256];

	    q_status_message(SM_ORDER, 3, 5, "Connecting to update server");
	    sprintf(update_folder, "%supdates.pine%s", UPDATE_FOLDER,
		    PHONE_HOME_VERSION);
	    visit_folder(ps_global, update_folder, NULL);
	    ps_global->mangled_screen = 1;
	}

	break;

        /*----- EDIT SIGNATURE -----*/
      case 's':
	signature_edit(ps_global->VAR_SIGNATURE_FILE);
	ps_global->mangled_screen = 1;
	break;

        /*----- CONFIGURE OPTIONS -----*/
      case 'c':
	option_screen(ps_global);
	ps_global->mangled_screen = 1;
	break;

        /*----- EXIT -----*/
      case 'e':
        break;

        /*----- NEW PASSWORD -----*/
      case 'n':
#ifdef	PASSWD_PROG
        if(ps_global->restricted){
	    q_status_message(SM_ORDER, 3, 5,
	    "Password change unavailable in restricted demo version of Pine.");
        }else {
	    change_passwd();
	    ClearScreen();
	    ps_global->mangled_screen = 1;
	}
#else
        q_status_message(SM_ORDER, 0, 5,
		 "Password changing not configured for this version of Pine.");
	display_message('x');
#endif	/* DOS */
        break;

        /*----- CHOOSE PRINTER ------*/
      case 'p':
#ifdef	DOS
	q_status_message(SM_ORDER, 3, 5,
	    "Printer configuration not available in the DOS version of Pine.");
#else
        select_printer(ps_global); 
	ps_global->mangled_screen = 1;
#endif
        break;
    }
}


/*
 * Make sure any errors during initialization get queued for display
 */
void
queue_init_errors(ps)
    struct pine *ps;
{
    int i;

    if(ps->init_errs){
	for(i = 0; ps->init_errs[i]; i++){
	    q_status_message(SM_ORDER | SM_DING, 3, 5, ps->init_errs[i]);
	    fs_give((void **)&ps->init_errs[i]);
	}

	fs_give((void **)&ps->init_errs);
    }
}


/*
 * Display a new user or new version message.
 */
void
new_user_or_version(message)
    char message[];
{
    char buf[256];
    int dline, i, j;

    j = 0;
    for(dline = 2;
	dline < ps_global->ttyo->screen_rows - (FOOTER_ROWS(ps_global)+1);
	dline++){
	for(i = 0; i < 256 && message[j] && message[j] != '\n' ; i++)
	  buf[i] = message[j++];

	buf[i] = '\0';
	if(message[j])
	  j++;
	else if(!i)
	  break;

        PutLine0(dline, 2, buf);
    }

    /*
     * Temporary message for old bug.
     * 3.89 and earlier would delete continuation lines of variables
     * it didn't know about, so new *list* variables added would be
     * truncated to length 1 if a user went back to a previous version.
     * There are many new list vars in 3.90 so it could be a problem now.
     * Check to see if the new list variables are set and may have been
     * truncated, and issue a warning.  This works because 3.89 and earlier
     * rewrite the version number in pinerc even if it goes backwards.
     */
    if(ps_global->pre390)
      truncated_listvars_warning(&dline);

    PutLine0(ps_global->ttyo->screen_rows - (FOOTER_ROWS(ps_global)+1), 13,
		 "PINE is a trademark of the University of Washington.");

    /*
     * You may think this is weird.  We're trying to offer sending
     * the "phone home" message iff there's no evidence we've ever
     * run pine *or* this is the first time we've run a version
     * a version of pine >= 3.90.  The check for the existence of a var
     * new in 3.90, is to compensate for pre 3.90 pine's rewriting their
     * lower version number.
     */
    if(ps_global->first_time_user
       || (ps_global->pre390
	   && !var_in_pinerc(ps_global->vars[V_NNTP_SERVER].name))
       || !var_in_pinerc(ps_global->vars[V_MAILCHECK].name)){
	phone_home_blurb(dline);
	if(want_to("Request document", 'y', 0, NO_HELP, 0, 1) == 'y')
	  phone_home();
	else
	  q_status_message(SM_ORDER,0,3,"No request sent");

    }
    else{
	StartInverse();
	PutLine0(ps_global->ttyo->screen_rows - FOOTER_ROWS(ps_global), 0,
		 "Type any character to continue : ");
	EndInverse();

	/* ignore the character typed, 120 second timeout */
	(void)read_char(120);
    }

    /*
     * Check to see if we have an old style postponed mail folder and
     * that there isn't a new style postponed folder.  If true,
     * fix up the old one, and move it into postition for pine >= 3.90...
     */
    if(!ps_global->first_time_user && ps_global->pre390)
      upgrade_old_postponed();
}


void
upgrade_old_postponed()
{
    int	       i;
    char       file_path[MAXPATH], *status, buf[6];
    STORE_S   *in_so, *out_so;
    CONTEXT_S *save_cntxt = default_save_context(ps_global->context_list);
    STRING     msgtxt;
    gf_io_t    pc, gc;

    /*
     * NOTE: woe to he who redefines things in os.h such that
     * the new and old pastponed folder names are the same.
     * If so, you're on your own...
     */
    build_path(file_path, ps_global->folders_dir, POSTPONED_MAIL);
    if(in_so = so_get(FileStar, file_path, READ_ACCESS)){
	for(i = 0; i < 6 && so_readc((unsigned char *)&buf[i], in_so); i++)
	  ;

	buf[i] = '\0';
	if(strncmp(buf, "From ", 5)){
	    dprint(1, (debugfile,
		       "POSTPONED conversion %s --> <%s>%s\n",
		       file_path,save_cntxt->context,
		       ps_global->VAR_POSTPONED_FOLDER));
	    so_seek(in_so, 0L, 0);
	    if((out_so = so_get(CharStar, NULL, WRITE_ACCESS))
	       && (folder_exists(save_cntxt->context,
				 ps_global->VAR_POSTPONED_FOLDER) > 0
		   || context_create(save_cntxt->context, NULL,
				     ps_global->VAR_POSTPONED_FOLDER))){
		gf_set_so_readc(&gc, in_so);
		gf_set_so_writec(&pc, out_so);
		gf_filter_init();
		gf_link_filter(gf_local_nvtnl);
		if(!(status = gf_pipe(gc, pc))){
		    so_seek(out_so, 0L, 0);	/* just in case */
		    INIT(&msgtxt, mail_string, so_text(out_so),
			 strlen((char *)so_text(out_so)));

		    if(context_append(save_cntxt->context, NULL, 
				      ps_global->VAR_POSTPONED_FOLDER,
				      &msgtxt)){
			so_give(&in_so);
			unlink(file_path);
		    }
		    else{
			q_status_message(SM_ORDER | SM_DING, 3, 5,
					"Problem upgrading postponed message");
			dprint(1, (debugfile,
				   "Conversion failed: Can't APPEND\n"));
		    }
		}
		else{
		    q_status_message(SM_ORDER | SM_DING, 3, 5,
				     "Problem upgrading postponed message");
		    dprint(1,(debugfile,"Conversion failed: %s\n",status));
		}
	    }
	    else{
		q_status_message(SM_ORDER | SM_DING, 3, 5,
				 "Problem upgrading postponed message");
		dprint(1, (debugfile,
			   "Conversion failed: Can't create %s\n",
			   (out_so) ? "new postponed folder"
				    : "temp storage object"));
	    }

	    if(out_so)
	      so_give(&out_so);
	}

	if(in_so)
	  so_give(&in_so);
    }
}


void
phone_home_blurb(dline)
    int dline;
{
    static char *blurb[] = {
      "SPECIAL OFFER:  Would you like to receive (via email) a brief document",
      "  entitled \"Getting the most out of Pine\" ?"
    };

    if(dline+3 < ps_global->ttyo->screen_rows - FOOTER_ROWS(ps_global))
      dline++;

    if(dline+2 < ps_global->ttyo->screen_rows - FOOTER_ROWS(ps_global)){
	PutLine0(dline++, 2, blurb[0]);
	PutLine0(dline++, 2, blurb[1]);
    }
}


void
truncated_listvars_warning(dline)
    int *dline;
{
    char **p;
    int    i;
#define N_LISTS 9
    char **new_lists[N_LISTS];

    i = 0;
    new_lists[i++] = ps_global->USR_COMP_HDRS;
    new_lists[i++] = ps_global->USR_CUSTOM_HDRS;
    new_lists[i++] = ps_global->USR_NNTP_SERVER;
    new_lists[i++] = ps_global->USR_ADDRESSBOOK;
    new_lists[i++] = ps_global->USR_GLOB_ADDRBOOK;
    new_lists[i++] = ps_global->USR_FORCED_ABOOK_ENTRY;
    new_lists[i++] = ps_global->USR_DISPLAY_FILTERS;
    new_lists[i++] = ps_global->USR_ALT_ADDRS;
    new_lists[i++] = ps_global->USR_ABOOK_FORMATS;

    for(i=0; i < N_LISTS; i++){
        p = new_lists[i];
        if(p && *p && **p)
	  break;
    }

#define LINES_OF_EXPLANATION 2
    if(i < N_LISTS){
      if((*dline)+LINES_OF_EXPLANATION+1
         < ps_global->ttyo->screen_rows - FOOTER_ROWS(ps_global))
	  (*dline)++;
      if((*dline)+LINES_OF_EXPLANATION
         < ps_global->ttyo->screen_rows - FOOTER_ROWS(ps_global)){
	PutLine0((*dline)++, 2,
    "[You ran an old version of Pine which may have truncated some of your]");
	PutLine0((*dline)++, 2,
    "[new pinerc list variables (because of a bug in the old version).    ]");
      }
    }
}


/*----------------------------------------------------------------------
          Quit pine if the user wants to 

    Args: The usual pine structure

  Result: User is asked if she wants to quit, if yes then execute quit.

       Q U I T    S C R E E N

Not really a full screen. Just count up deletions and ask if we really
want to quit.
  ----*/
void
quit_screen(pine_state)
    struct pine *pine_state;
{
    dprint(1, (debugfile, "\n\n    ---- QUIT SCREEN ----\n"));    

    if(!pine_state->nr_mode && F_OFF(F_QUIT_WO_CONFIRM,pine_state)
       && want_to("Really quit pine", 'y', 0, NO_HELP, 0, 0) != 'y') {
        pine_state->next_screen = pine_state->prev_screen;
        return;
    }

    goodnight_gracey(pine_state, 0);
}



/*----------------------------------------------------------------------
    The nuts and bolts of actually cleaning up and exitting pine

    Args: ps -- the usual pine structure, 
	  exit_val -- what to tell our parent

  Result: This never returns

  ----*/
void
goodnight_gracey(pine_state, exit_val)
    struct pine *pine_state;
    int		 exit_val;
{
    int   i, cur_is_inbox;
    char *final_msg = NULL, *p;
    char  msg[MAX_SCREEN_COLS+1];
    char *pf = "Pine finished";

    cur_is_inbox = (pine_state->inbox_stream == pine_state->mail_stream);

    /* clean up open streams */
    if(pine_state->mail_stream)
      expunge_and_close(pine_state->mail_stream, pine_state->context_current,
			pine_state->cur_folder,
			(!pine_state->inbox_stream || cur_is_inbox)
			  ? &final_msg : NULL);
    if(pine_state->msgmap)
      mn_give(&pine_state->msgmap);

    pine_state->redrawer = (void (*)())NULL;

    if(pine_state->inbox_stream && !cur_is_inbox){
	pine_state->mail_stream = pine_state->inbox_stream;
	pine_state->msgmap      = pine_state->inbox_msgmap;
        expunge_and_close(pine_state->inbox_stream, NULL,
			  pine_state->inbox_name, &final_msg);
	mn_give(&pine_state->msgmap);
    }

    if(pine_state->outstanding_pinerc_changes)
      write_pinerc(pine_state);

    if(final_msg){
	strcpy(msg, pf);
	strcat(msg, " -- ");
	strncat(msg, final_msg, MAX_SCREEN_COLS - strlen(msg));
	fs_give((void **)&final_msg);
    }
    else
      strcpy(msg, pf);

    end_screen(msg);
    end_titlebar();
    end_keymenu();

    end_keyboard(F_ON(F_USE_FK,pine_state));
    end_tty_driver(pine_state);
#if !defined(DOS) && !defined(OS2)
    kbdestroy(pine_state->kbesc);
#endif
    end_signals(0);
    if(filter_data_file(0))
      unlink(filter_data_file(0));

    imap_flush_passwd_cache();
    clear_index_cache();
    completely_done_with_adrbks();
    free_newsgrp_cache();
    mailcap_free();
    free_folders();

    if(p = (char *) mail_parameters(NULL, GET_USERNAMEBUF, NULL)){
	fs_give((void **)&p);
	(void) mail_parameters(NULL, SET_USERNAMEBUF, NULL);
    }

    if(pine_state->hostname != NULL)
      fs_give((void **)&pine_state->hostname);
    if(pine_state->localdomain != NULL)
      fs_give((void **)&pine_state->localdomain);
    if(pine_state->ttyo != NULL)
      fs_give((void **)&pine_state->ttyo);
    if(pine_state->home_dir != NULL)
      fs_give((void **)&pine_state->home_dir);
    if(pine_state->folders_dir != NULL)
      fs_give((void **)&pine_state->folders_dir);
    if(pine_state->ui.homedir)
      fs_give((void **)&pine_state->ui.homedir);
    if(pine_state->ui.login)
      fs_give((void **)&pine_state->ui.login);
    if(pine_state->ui.fullname)
      fs_give((void **)&pine_state->ui.fullname);
    if(pine_state->index_disp_format)
      fs_give((void **)&pine_state->index_disp_format);
    if(pine_state->pinerc)
      fs_give((void **)&pine_state->pinerc);
#if defined(DOS) || defined(OS2)
    if(pine_state->pine_dir)
      fs_give((void **)&pine_state->pine_dir);
#endif

    if(ps_global->atmts){
	for(i = 0; ps_global->atmts[i].description; i++){
	    fs_give((void **)&ps_global->atmts[i].description);
	    fs_give((void **)&ps_global->atmts[i].number);
	}

	fs_give((void **)&ps_global->atmts);
    }

    free_vars(pine_state);
    free_pinerc_lines();

    fs_give((void **)&pine_state);

#ifdef DEBUG
    if(debugfile)
      fclose(debugfile);
#endif    

    exit(exit_val);
}


/*
 * Useful flag checking macro for 
 */
#define FLAG_MATCH(F, M)   (((((F)&F_SEEN)   ? (M)->seen		     \
				: ((F)&F_UNSEEN)     ? !(M)->seen : 1)	     \
			  && (((F)&F_DEL)    ? (M)->deleted		     \
				: ((F)&F_UNDEL)      ? !(M)->deleted : 1)    \
			  && (((F)&F_ANS)    ? (M)->answered		     \
				: ((F)&F_UNANS)	     ? !(M)->answered : 1)   \
			  && (((F)&F_FLAG) ? (M)->flagged		     \
				: ((F)&F_UNFLAG)   ? !(M)->flagged : 1)	     \
			  && (((F)&F_RECENT) ? (M)->recent		     \
				: ((F)&F_UNRECENT)   ? !(M)->recent : 1))    \
			  || ((((F)&F_OR_SEEN) ? (M)->seen		     \
				: ((F)&F_OR_UNSEEN)   ? !(M)->seen : 0)      \
			  || (((F)&F_OR_DEL)   ? (M)->deleted		     \
				: ((F)&F_OR_UNDEL)    ? !(M)->deleted : 0)   \
			  || (((F)&F_OR_ANS)   ? (M)->answered		     \
				: ((F)&F_OR_ANS)      ? !(M)->answered : 0)  \
			  || (((F)&F_OR_FLAG)? (M)->flagged		     \
				: ((F)&F_OR_UNFLAG) ? !(M)->flagged : 0)     \
			  || (((F)&F_OR_RECENT)? (M)->recent		     \
				: ((F)&F_OR_UNRECENT) ? !(M)->recent : 0)))



/*----------------------------------------------------------------------
     Find the first message with the specified flags set

  Args: flags -- Flags in messagecache to match on
        stream -- The stream/folder to look at message status

 Result: Message number of first message with specified flags set
  ----------------------------------------------------------------------*/
MsgNo
first_sorted_flagged(flags, stream)
    unsigned long  flags;
    MAILSTREAM    *stream;
{
    MsgNo        i;
    MESSAGECACHE *mc;

    FETCH_ALL_FLAGS(stream);

    for(i = 1 ; i < mn_get_total(ps_global->msgmap); i++) {
	mc = mail_elt(stream, mn_m2raw(ps_global->msgmap, i));
	if(mc && FLAG_MATCH(flags, mc))
          break;
    }

    dprint(4, (debugfile, "First unseen returning %ld\n", (long)i));
    return(i);
}



/*----------------------------------------------------------------------
     Find the next message with specified flags set

  Args: flags -- Flags in messagecache to match on
        stream -- The stream/folder to look at message status
        start  -- Place to start looking
        found_is_new -- Set if the message found is actually new.  The only
                        case where this might not be set when the message
                        being returned is the last message in folder.

 Result: Message number of next message with specified flags set
  ----------------------------------------------------------------------*/
MsgNo
next_sorted_flagged(flags, stream, start, found_is_new)
    unsigned long  flags;
    MAILSTREAM    *stream;
    long           start;
    int           *found_is_new;
{
    MsgNo        i;
    MESSAGECACHE *mc;

    if(found_is_new)
      *found_is_new = 0;

    FETCH_ALL_FLAGS(stream);

    for(i = start ; i <= mn_get_total(ps_global->msgmap); i++) {
	mc = mail_elt(stream, mn_m2raw(ps_global->msgmap, i));
        if(mc && FLAG_MATCH(flags, mc)
	   && !get_lflag(stream, ps_global->msgmap, i, MN_HIDE)){
            if(found_is_new)
              *found_is_new = 1;

            break;
        }
    }

    return(min(i, mn_get_total(ps_global->msgmap)));
}



/*----------------------------------------------------------------------
  get the requested LOCAL flag bits for the given pine message number

   Accepts: msgs - pointer to message manipulation struct
            n - message number to get
	    f - bitmap of interesting flags
   Returns: non-zero if flag set, 0 if not set or no elt (error?)

   NOTE: this can be used to test system flags
  ----*/
int
get_lflag(stream, msgs, n, f)
     MAILSTREAM *stream;
     MSGNO_S    *msgs;
     long        n;
     int         f;
{
    MESSAGECACHE *mc;

    if(n < 1L || (msgs && n > mn_get_total(msgs)))
      return(0);

    FETCH_ALL_FLAGS(stream);

    mc = mail_elt(stream, msgs ? mn_m2raw(msgs, n) : n);
    return((!mc) ? 0 : (!f) ? !(mc->spare || mc->spare2 || mc->spare3)
			    : (((f & MN_HIDE) ? mc->spare : 0)
			       || ((f & MN_EXLD) ? mc->spare2 : 0)
			       || ((f & MN_SLCT) ? mc->spare3 : 0)));
}



/*----------------------------------------------------------------------
  set the requested LOCAL flag bits for the given pine message number

   Accepts: msgs - pointer to message manipulation struct
            n - message number to set
	    f - bitmap of interesting flags
	    v - value (on or off) flag should get
   Returns: our index number of first

   NOTE: this isn't to be used for setting IMAP system flags
  ----*/
int
set_lflag(stream, msgs, n, f, v)
     MAILSTREAM *stream;
     MSGNO_S    *msgs;
     long        n;
     int         f, v;
{
    MESSAGECACHE *mc;

    if(n < 1L || n > mn_get_total(msgs))
      return(0L);

    FETCH_ALL_FLAGS(stream);

    if(mc = mail_elt(stream, mn_m2raw(msgs, n))){
	if((f & MN_HIDE) && mc->spare != v){
	    mc->spare = v;
	    msgs->flagged_hid += (v) ? 1L : -1L;
	}

	if((f & MN_EXLD) && mc->spare2 != v){
	    mc->spare2 = v;
	    msgs->flagged_exld += (v) ? 1L : -1L;
	}

	if((f & MN_SLCT) && mc->spare3 != v){
	    mc->spare3 = v;
	    msgs->flagged_tmp += (v) ? 1L : -1L;
	}
    }

    return(1);
}



/*----------------------------------------------------------------------
  return whether the given flag is set somewhere in the folder

   Accepts: msgs - pointer to message manipulation struct
	    f - flag bitmap to act on
   Returns: number of messages with the given flag set.
	    NOTE: the sum, if multiple flags tested, is bogus
  ----*/
long
any_lflagged(msgs, f)
     MSGNO_S    *msgs;
     int         f;
{
    if(!msgs)
      return(0L);

    if(f == MN_NONE)
      return(!(msgs->flagged_hid || msgs->flagged_exld || msgs->flagged_tmp));
    else
      return(((f & MN_HIDE)   ? msgs->flagged_hid  : 0L)
	     + ((f & MN_EXLD) ? msgs->flagged_exld : 0L)
	     + ((f & MN_SLCT) ? msgs->flagged_tmp  : 0L));
}



/*----------------------------------------------------------------------
  Decrement the count of the given flag type messages

   Accepts: msgs - pointer to message manipulation struct
	    f - flag bitmap to act on
	    n - number of flags
   Returns: with the total count adjusted accordingly.
	    NOTE: this function is kind of bogus, and is mostly 
	    necessary because flags are stored in elt's which we can't
	    get at during an mm_expunged callback.
  ----*/
void
dec_lflagged(msgs, f, n)
     MSGNO_S    *msgs;
     int         f;
     long	 n;
{
    if(!msgs)
      return;

    if(f & MN_HIDE)
      msgs->flagged_hid = max(0L, msgs->flagged_hid - n);
    else if(f & MN_EXLD)
      msgs->flagged_exld = max(0L, msgs->flagged_exld - n);
    else if(f & MN_SLCT)
      msgs->flagged_tmp = max(0L, msgs->flagged_tmp - n);
}



/*----------------------------------------------------------------------
    Count messages on stream with specified system flag attributes

  Args: stream -- The stream/folder to look at message status
        flags -- flags on folder/stream to examine

 Result: count of messages with those attributes set
  ----------------------------------------------------------------------*/
long
count_flagged(stream, flags)
     MAILSTREAM *stream;
     char       *flags;
{
    char   *flagp;
    int     old_prefetch;
    extern  MAILSTREAM *mm_search_stream;
    extern  long	mm_search_count;

    mm_search_stream = stream;
    mm_search_count  = 0L;

    old_prefetch = (int) mail_parameters(stream, GET_PREFETCH, NULL);
    (void) mail_parameters(stream, SET_PREFETCH, NULL);
    mail_search(stream, flagp = cpystr(flags));
    mail_parameters(stream, SET_PREFETCH, (void *) old_prefetch);
    fs_give((void **)&flagp);

    return(mm_search_count);
}


/*----------------------------------------------------------------------
  See if stream can be used for a mailbox name (courtesy of Mark Crispin)

   Accepts: mailbox name
            candidate stream
   Returns: stream if it can be used, else NIL

  This is called to weed out unnecessary use of c-client streams.
  ----*/
MAILSTREAM *
same_stream(name, stream)
    char *name;
    MAILSTREAM *stream;
{
    NETMBX	 mb_n;
    char	*s, *t, *u, *host_s;

    if (stream && (s = stream->mailbox) &&
				/* must be a network stream */
	((*s == '{') || (*s == '*' && s[1] == '{')) &&
				/* must be a network name */
	((*(t = name) == '{') || (*name == '*' && (t = name)[1] == '{')) &&
				/* name must be valid for that stream */
	mail_valid_net_parse (t, &mb_n) &&
	((!strncmp (mb_n.service, "imap", 4) && 
				/* get host name from stream, require imap */
	  stream->dtb && stream->dtb->name &&
	  !strncmp(stream->dtb->name, "imap", 4) &&
	  (host_s = imap_host(stream))) ||
	 (!strncmp (mb_n.service, "nntp", 4) && 
				/* get host name from stream, require imap */
	  stream->dtb && stream->dtb->name &&
	  !strncmp(stream->dtb->name, "nntp", 4) &&
	  (host_s = nntp_host(stream->mailbox)))) &&
				/* require same host */
	!strucmp (canonical_name(mb_n.host), host_s) &&
				/* if named port, require ports match */
	(!mb_n.port || mb_n.port == imap_port(stream)) &&
				/* require anonymous modes match */
				/* debug and bboard flags safely ignored */
	mb_n.anoflag == stream->anonymous &&
				/* if named user, require it match stream's */
	((stream->dtb->name[0] == 'i')
	  ? (((u = (char *) mail_parameters(NULL,GET_USERNAMEBUF,NULL)) &&
	      *u && !strcmp(u, imap_user(stream))) ||
				/* OR if no named user, require default */
	     (!(u && *u) && !strcmp(ps_global->VAR_USER_ID,imap_user(stream))))
	  : 1))
      return stream;

    return NIL;			/* one of the tests failed */
}


/* BUG: shouldn't c-client export this? */
char *
nntp_host(mbx)
    char *mbx;
{
    NETMBX	 mb_n;
    static char host[256];

    if(mbx && mail_valid_net_parse (mbx, &mb_n)){
	strcpy(host, canonical_name(mb_n.host));
	return(host);
    }

    return(NULL);
}


/*----------------------------------------------------------------------
   Give hint about Other command being optional.  Some people get the idea
   that it is required to use the commands on the 2nd and 3rd keymenus.
   
   Args: none

 Result: message may be printed to status line
  ----*/
void
warn_other_cmds()
{
    static int other_cmds = 0;

    other_cmds++;
    if(((ps_global->first_time_user || ps_global->show_new_version) &&
	      other_cmds % 3 == 0 && other_cmds < 10) || other_cmds % 20 == 0)
        q_status_message(SM_ASYNC, 0, 9,
		    "Remember: the \"O\" command is always optional");
}


/*----------------------------------------------------------------------
    Panic pine - call on detected programmatic errors to exit pine

   Args: message -- message to record in debug file and to be printed for user

 Result: The various tty modes are restored
         If debugging is active a core dump will be generated
         Exits Pine

  This is also called from imap routines and fs_get and fs_resize.
  ----*/
void
panic(message)
    char *message;
{
    end_screen(NULL);
    end_keyboard(ps_global != NULL ? F_ON(F_USE_FK,ps_global) : 0);
    end_tty_driver(ps_global);
    end_signals(1);
    if(filter_data_file(0))
      unlink(filter_data_file(0));

    dprint(1, (debugfile, "Pine Panic: %s\n", message));
#ifndef _WINDOWS
    fprintf(stderr, "\n\nProblem detected: \"%s\".\nPine Exiting.\n",
            message);
#else
    /* fprintf won't do a thing in windows.  Instead, put up a message
     * box. */
    sprintf (tmp_20k_buf, "Sorry, Problem detected.  %s.  Very Sorry, pine will now crash, please report this.", message);
    mswin_messagebox (tmp_20k_buf, 1);
#endif

#ifdef DEBUG
    if(debugfile)
      save_debug_on_crash(debugfile);
    coredump();   /*--- If we're debugging get a core dump --*/
#endif

    exit(-1);
    fatal("ffo"); /* BUG -- hack to get fatal out of library in right order*/
}



/*----------------------------------------------------------------------
    Panic pine - call on detected programmatic errors to exit pine, with arg

  Input: message --  printf styule string for panic message (see above)
         arg     --  argument for printf string

 Result: The various tty modes are restored
         If debugging is active a core dump will be generated
         Exits Pine
  ----*/
void
panic1(message, arg)
    char *message;
    char *arg;
{
    char buf[1001];
    if(strlen(message) > 1000) {
        panic("Pine paniced. (Reason for panic is too long to tell)");
    } else {
        sprintf(buf, message, arg);
        panic(buf);
    }
}



/*----------------------------------------------------------------------
  Function used by c-client to save read bytes resulting from a fetch

  Input: f -- function
	 stream -- stream object to read from
	 size -- number of bytes we're expected to read

 Result: Alloc'd string containing fetched bytes
  ----*/
char *
pine_gets(f, stream, size)
    readfn_t	   f;
    void	  *stream;
    unsigned long  size;
{
    char	  *s, *p;
    unsigned long  i = 0;

    s = p = (char *) fs_get ((size_t) size + 1);
    *s = s[size] = '\0';		/* init in case getbuffer fails */
    do{
	(*f)(stream, i = min((unsigned long)MAILTMPLEN, size), p);
	p += i;
	gets_bytes += i;
    }
    while(size -= i);

    return(s);
}



/*----------------------------------------------------------------------
  Function to fish the current byte count from a c-client fetch.

  Input: reset -- flag telling us to reset the count

 Result: Returns the number of bytes read by the c-client so far
  ----*/
unsigned long
pine_gets_bytes(reset)
    int reset;
{
    if(reset)
      gets_bytes = 0L;

    return(gets_bytes);
}
