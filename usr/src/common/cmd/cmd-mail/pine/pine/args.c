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
      args.c
      Command line argument parsing functions 

  ====*/

#ifdef	OS2
#define	INCL_DOS
#define	INCL_NOPM
#define INCL_VIO
#include <os2.h>
#undef ADDRESS
#endif
#include "headers.h"

void args_help PROTO(());
void display_args_err PROTO((char *, char **, int));


/*
 * Name started as to invoke function key mode
 */
#define	PINE_FKEY_NAME	"pinef"

/*
 * Various error and informational strings..
 */
char args_err_missing_pinerc[] =	"missing argument for option \"-pinerc\" (use - for standard out)";
char args_err_missing_lu[] =		"missing argument for option \"-create_lu\"\nUsage: pine -create_lu <addrbook_file> <addrbook_sort_type>";
char args_err_missing_sort[] =		"missing argument for option \"-sort\"";
char args_err_missing_flag_arg[] =	"missing argument for flag \"%c\"";
char args_err_missing_flag_num[] =	"Non numeric argument for flag \"%c\"";
char args_err_unknown[] =		"unknown flag \"%c\"";
char args_err_I_error[] =		"-I argument \"%s\": %s";
char args_err_internal[] =		"%s";


char *args_pine_args[] = {
"Possible Starting Arguments for Pine program:",
"",
"\tArgument\t\tMeaning",
"\t <addrs>...\tGo directly into composer sending to given address",
"\t\t\tStandard input redirection is allowed with addresses.",
#ifdef	DEBUG
"\t -d n\t\tDebug - set debug level to 'n'",
#endif
"\t -f <folder>\tFolder - give folder name to open",
"\t -c <number>\tContext - which context to apply to -f arg",
"\t -F <file>\tFile - give file name to open and page thru",
"\t -h \t\tHelp - give this list of options",
"\t -k \t\tKeys - Force use of function keys",
"\t -z \t\tSuspend - allow use of ^Z suspension",
"\t -r \t\tRestricted - can only send mail to one self",
"\t -sort <sort> \tSort - Specify sort order of folder:",
"\t\t\t        subject, arrival, date, from, size, /reverse",
"\t -i\t\tIndex - Go directly to index, bypassing main menu",
"\t -I <keystroke_list>\tInitial keystrokes to be executed",
"\t -n <number>\tEntry in index to begin on",
"\t -o \t\tReadOnly - Open first folder read-only",
"\t -conf\t\tConfiguration - Print out fresh global configuration",
"\t -pinerc <file>\tConfiguration - Put fresh pinerc configuration in <file>",
"\t -p <pinerc>\tUse alternate .pinerc file",
#if	!defined(DOS) && !defined(OS2)
"\t -P <pine.conf>\tUse alternate pine.conf file",
#endif
#if defined(OS2)
"\t -w <rows>\tSet window size in rows on startup",
#endif
"\t -create_lu <abook_file> <ab_sort_type>   create .lu from script",
"\t -nr\t\tSpecial mode for UWIN",
"\t -a\t\tSpecial anonymous mode for UWIN",
"\t -l\t\tList - Expand List of folder collections by default",
"\t -<option>=<value>\tAssign <value> to the pinerc option <option>",
"\t\t\t        e.g. -signature-file=sig1",
"\t\t\t        (Note: feature-list is additive)",
NULL
};



/*
 *  Parse the command line args.
 *
 *  Args: pine_state -- The pine_state structure to put results in
 *        argc, argv -- The obvious
 *        addrs      -- Pointer to address list that we set for caller
 *
 * Result: command arguments parsed
 *       possible printing of help for command line
 *       various flags in pine_state set
 *       returns the string name of the first folder to open
 *       addrs is set
 */
char *
pine_args(pine_state, argc, argv, addrs)
     struct pine  *pine_state;
     int           argc;
     char        **argv;
     char       ***addrs;
{
    register int    ac;
    register char **av;
    int   c;
    char *str;
    char *folder              = NULL;
    char *cmd_list            = NULL;
    char *sort                = NULL;
    char *pinerc_file         = NULL;
    char *addrbook_file       = NULL;
    char *ab_sort_descrip     = NULL;
    char *lc		      = NULL;
    int   do_help             = 0;
    int   do_conf             = 0;
    int   anonymous           = 0;
    int   usage               = 0;
    int   do_use_fk           = 0;
    int   do_can_suspend      = 0;
    int   do_expanded_folders = 0;
    struct variable *vars     = pine_state->vars;

    
    ac = argc;
    av = argv;

    pine_state->pine_name = (lc = last_cmpnt(argv[0])) ? lc : (lc = argv[0]);
#ifdef	OS2
    /*
     * argv0 is not reliable under OS2, so we have to get
     * the executable path from the environment block
     */
    {
      PTIB ptib;
      PPIB ppib;
      char * p;
      DosGetInfoBlocks(&ptib, &ppib);
      p = ppib->pib_pchcmd - 1;
      while (*--p)
	;
      ++p;
      strcpy(tmp_20k_buf, p);
      if ((p = strrchr(tmp_20k_buf, '\\'))!=NULL)
	*++p = '\0';
      pine_state->pine_dir = cpystr(tmp_20k_buf);
    }
#endif
#ifdef	DOS
    sprintf(tmp_20k_buf, "%.*s", pine_state->pine_name - argv[0], argv[0]);
    pine_state->pine_dir = cpystr(tmp_20k_buf);
#endif

      /* while more arguments with leading - */
Loop: while(--ac > 0 && **++av == '-'){
	 /* while more chars in this argument */
	 while(*++*av){
	    /* check for pinerc options */
	    if(pinerc_cmdline_opt(*av)){
	       goto Loop;  /* done with this arg, go to next */
	    /* then other multi-char options */
	    }else if(strcmp(*av, "conf") == 0){
	       do_conf = 1;
	       goto Loop;  /* done with this arg, go to next */
	    }else if(strcmp(*av, "pinerc") == 0){
	       if(--ac){
		   pinerc_file = *++av;
	       }else{
		   display_args_err(args_err_missing_pinerc, NULL, 1);
		   ++usage;
	       }
	       goto Loop;
	    }else if(strcmp(*av, "create_lu") == 0){
	       if(ac > 2){
	          ac -= 2;
		  addrbook_file   = *++av;
		  ab_sort_descrip = *++av;
	       }else{
		   display_args_err(args_err_missing_lu, NULL, 1);
		   exit(-1);
	       }
	       goto Loop;
	    }else if(strcmp(*av, "nr") == 0){
	       pine_state->nr_mode = 1;
	       goto Loop;
	    }else if(strcmp(*av, "sort") == 0){
	       if(--ac){
		   sort = *++av;
		   COM_SORT_KEY = cpystr(sort);
	       }else{
		   display_args_err(args_err_missing_sort, NULL, 1);
		   ++usage;
	       }
	       goto Loop;

	    /* single char flags */
	    }else{
	       switch(c = **av) {
		 case 'h':
		   do_help = 1;
		   break;			/* break back to inner-while */
		 case 'k':
		   do_use_fk = 1;
		   break;
		 case 'a':
		   anonymous = 1;
		   break;
		 case 'z':
		   do_can_suspend = 1;
		   break;
		 case 'r':
		   pine_state->restricted = 1;
		   break;
		 case 'o':
		   pine_state->open_readonly_on_startup = 1;
		   break;
		 case 'i':
		   pine_state->start_in_index = 1;
		   break;
		 case 'l':
		   do_expanded_folders = 1;
		   break;
		 /* these take arguments */
		 case 'f' : case 'F': case 'p': case 'I':
		 case 'c' :			/* string args */
#if	!defined(DOS) && !defined(OS2)
		 case 'P':			/* also a string */
#endif
		 case 'n':			/* integer args */
#ifdef	OS2
		 case 'w':
#endif
#ifdef	DEBUG
		 case 'd':
#endif
		   if(*++*av){
		      str = *av;
		   }else if(--ac){
		      str = *++av;
		   }else{
		       sprintf(tmp_20k_buf, args_err_missing_flag_arg, c);
		       display_args_err(tmp_20k_buf, NULL, 1);
		       ++usage;
		       goto Loop;
		   }
		   switch(c){
		     case 'f':
		       folder = str;
		       break;
		     case 'F':
		       folder = str;
		       pine_state->more_mode = 1;
		       break;
		     case 'I':
		       cmd_list = str;
		       break;
		     case 'p':
		       if(str != NULL)
			  pine_state->pinerc = cpystr(str);
		       break;
#if	!defined(DOS) && !defined(OS2)
		     case 'P':
		       if(str != NULL)
			  pine_state->pine_conf = cpystr(str);
		       break;
#endif
		     case 'd':
		     case 'c':
		       if(!isdigit((unsigned char)str[0])){
			   sprintf(tmp_20k_buf, args_err_missing_flag_num, c);
			   display_args_err(tmp_20k_buf, NULL, 1);
			   ++usage;
			   break;
		       }

		       if(c == 'c')
			 pine_state->init_context = (short) atoi(str);
#ifdef	DEBUG
		       else
			 debug = atoi(str);
#endif

		       break;

		     case 'n':
		       if(!isdigit((unsigned char)str[0])){
			   sprintf(tmp_20k_buf, args_err_missing_flag_num, c);
			   display_args_err(tmp_20k_buf, NULL, 1);
			   ++usage;
			   break;
		       }

		       pine_state->start_entry = atoi(str);
		       if (pine_state->start_entry < 1)
			  pine_state->start_entry = 1;
		       break;
#ifdef	OS2
		     case 'w':
		       {
			   USHORT nrows = (USHORT)atoi(str);
			   if (nrows > 10 && nrows < 255) {
			       VIOMODEINFO mi;
			       mi.cb = sizeof(mi);
			       if (VioGetMode(&mi, 0)==0){
				   mi.row = nrows;
				   VioSetMode(&mi, 0);
			       }
			   }
		       }
#endif
		   }
		   goto Loop;

		 default:
		       sprintf(tmp_20k_buf, args_err_unknown, c);
		       display_args_err(tmp_20k_buf, NULL, 1);
		       ++usage;
		       break;
	       }
	    }
	 }
      }

    if(cmd_list){
	int    commas         = 0;
	char  *p              = cmd_list;
	char  *error          = NULL;

	while(*p++)
	    if(*p == ',')
		++commas;

	COM_INIT_CMD_LIST = parse_list(cmd_list, commas+1, &error);
	if(error){
	    sprintf(tmp_20k_buf, args_err_I_error, cmd_list, error);
	    display_args_err(tmp_20k_buf, NULL, 1);
	    exit(-1);
	}
    }

    if(lc && strncmp(lc, PINE_FKEY_NAME, sizeof(PINE_FKEY_NAME) - 1) == 0)
      do_use_fk = 1;

    if(do_use_fk || do_can_suspend || do_expanded_folders){
	char   list[500];
	int    commas         = 0;
	char  *p              = list;
	char  *error          = NULL;

        list[0] = '\0';

        if(do_use_fk){
	    if(list[0])
	        (void)strcat(list, ",");
	    (void)strcat(list, "use-function-keys");
	}
	if(do_can_suspend){
	    if(list[0])
	        (void)strcat(list, ",");
	    (void)strcat(list, "enable-suspend");
	}
	if(do_expanded_folders){
	    if(list[0])
	        (void)strcat(list, ",");
	    (void)strcat(list, "expanded-view-of-folders");
	}

	while(*p++)
	    if(*p == ',')
		++commas;
	pine_state->feat_list_back_compat = parse_list(list, commas+1, &error);
	if(error){
	    sprintf(tmp_20k_buf, args_err_internal, error);
	    display_args_err(tmp_20k_buf, NULL, 1);
	    exit(-1);
	}
    }

    if(anonymous){
	if(pine_state->nr_mode){
	    pine_state->anonymous = 1;
	}
	else{
	    display_args_err("Can't currently use -a without -nr", NULL, 1);
	    exit(-1);
	}
    }

    if(do_conf && pinerc_file){
      display_args_err("Can't have both -conf and -pinerc", NULL, 1);
      exit(-1);
    }
    if(do_conf && addrbook_file){
      display_args_err("Can't have both -conf and -create_lu", NULL, 1);
      exit(-1);
    }
    if(pinerc_file && addrbook_file){
      display_args_err("Can't have both -pinerc and -create_lu", NULL, 1);
      exit(-1);
    }

    if(do_help || usage)
      args_help(); 

    if(usage)
      exit(-1);

    if(do_conf)
      dump_global_conf();
    if(pinerc_file)
      dump_new_pinerc(pinerc_file);
    if(addrbook_file)
      just_update_lookup_file(addrbook_file, ab_sort_descrip);

    pine_state->show_folders_dir = 0;

    if(ac <= 0)
      *av = NULL;

    *addrs = av;

    return(folder);
}


/*----------------------------------------------------------------------
    print a few lines of help for command line arguments

  Args:  none

 Result: prints help messages
  ----------------------------------------------------------------------*/
void
args_help()
{
    /**  print out possible starting arguments... **/
    display_args_err(NULL, args_pine_args, 0);
    exit(1);
}


/*----------------------------------------------------------------------
   write argument error to the display...

  Args:  none

 Result: prints help messages
  ----------------------------------------------------------------------*/
void
display_args_err(s, a, err)
    char  *s;
    char **a;
    int    err;
{
    char  errstr[256], *errp;
    FILE *fp = err ? stderr : stdout;


    if(err && s)
      sprintf(errp = errstr, "Argument Error: %.200s", s);
    else
      errp = s;

#ifdef	_WINDOWS
    if(errp)
      mswin_messagebox(errp, err);

    if(a && *a){
	strcpy(tmp_20k_buf, *a++);
	while(a && *a){
	    strcat(tmp_20k_buf, "\n");
	    strcat(tmp_20k_buf, *a++);
	}

	mswin_messagebox(tmp_20k_buf, err);
    }
#else
    if(errp)
      fprintf(fp, "%s\n", errp);

    while(a && *a)
      fprintf(fp, "%s\n", *a++);
#endif
}
