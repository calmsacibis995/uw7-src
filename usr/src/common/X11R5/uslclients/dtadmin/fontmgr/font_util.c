#ident	"@(#)dtadmin:fontmgr/font_util.c	1.23"
/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       font_util.c
 */

#include <stdio.h>
#define XK_LATIN1
#include <X11/keysymdef.h>
#include <sys/stat.h>
#include <Intrinsic.h>

#include <Desktop.h>

#include <OpenLook.h>
#include <Xol/Modal.h>

#include <Gizmos.h>
#include <BaseWGizmo.h>
#include <MenuGizmo.h>
#include <ModalGizmo.h>
#include <Footer.h>
#include <fontmgr.h>

/*
 * external data
 */
extern Widget       app_shellW;		  /* application shell widget       */
extern Widget base_shell;
extern view_type *view;
extern BaseWindowGizmo base;

void callRegisterHelp(Widget, char *, char *);

/* work queue stuff */
#define BACKGROUND 10
typedef void (*XtProc)();
typedef struct WorkPiece WorkPieceRec, *WorkPiece;
struct WorkPiece {
    WorkPiece next;
    int priority;
    XtProc proc;
    XtPointer closure;
};
static WorkPiece workQueue = NULL;

/*
 * forward procedure declaration
 */
void HelpCB();
static void NoticeCB();
void PopdownError();
void PopdownPrompt();
void InformUser();
void PopupErrorMsg();
void PromptUser();
void BusyCursor();
void StandardCursor();
char *GetNextField();
void CapitalizeStr();
void UppercaseStr();
String LowercaseStr();
Boolean FindChar();
Boolean SkipSpace();
Boolean FindSpace();
void InitStringDB();
void InsertStringDB();
void DeleteStringsDB();
Boolean FileOK();
FILE *ForkWithPipe();
void ForkWithoutPipe();
Boolean DoWorkPiece();
void ScheduleWork();
void OpenProgress();
void UpdateProgress();
void CloseProgress();

static MenuItems error_menu_item[] = {
{ TRUE, TXT_OK,   ACCEL_TXTOK_OK  , 0, PopdownError},
{ TRUE, TXT_HELP_DDD,     ACCEL_PROMPT_HELP    , 0, HelpCB},
{ NULL }
};
static MenuItems prompt_menu_item[] = {
{ TRUE, TXT_CONTINUE, ACCEL_PROMPT_CONTINUE },
{ TRUE, TXT_CANCEL,   ACCEL_PROMPT_CANCEL  , 0, PopdownPrompt},
{ TRUE, TXT_HELP_DDD,     ACCEL_PROMPT_HELP    , 0, HelpCB},
{ NULL }
};
static MenuGizmo prompt_menu = {0, "cm", "cm", prompt_menu_item };
ModalGizmo prompt = {0, "dc", TXT_PROMPT_ADD, (Gizmo) &prompt_menu};
static MenuGizmo error_menu = {0, "cm", "cm", error_menu_item };
ModalGizmo error = {0, "dc", TXT_PROMPT_ADD, (Gizmo) &error_menu};

/*
 * if count is 0 then strcmp
 * else strncmp
 */
int
caseless_strcmp OLARGLIST(( first, second, count ))
	OLARG (OLconst char *,		first)
	OLARG (OLconst char *,		second)
	OLGRA (register int,	count)
{
	register unsigned char *ap	   = (unsigned char *)first;
	register unsigned char *bp	   = (unsigned char *)second;

	register Boolean	dont_count = (count == 0);


	for (; (dont_count || --count >= 0) && *ap && *bp; ap++, bp++) {
		register unsigned char	a  = *ap;
		register unsigned char	b  = *bp;

		if (a != b) {
			a = tolower(a);
			b = tolower(b);
			if (a != b)
				break;
		}
	}
	return ((int)*bp - (int)*ap);
}


static int
LookupHelpType(char * section)
{
   if (*section == '\0')
      return (DT_TOC_HELP); /* backward compatibility */
   else
   {
      if (strcmp(section, "TOC") == 0)
         return (DT_TOC_HELP);
      else
         if (strcmp(section, "HelpDesk") == 0)
            return (DT_OPEN_HELPDESK);
         else
            return (DT_SECTION_HELP);
   }

} /* end of LookupHelpType */


void 
HelpCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    HelpInfo *help = (HelpInfo *) client_data;
    static String help_app_title;
    DtDisplayHelpRequest req;

    if (help_app_title == NULL)
	help_app_title = GetGizmoText(TXT_WINDOW_TITLE);

    help->app_title = help_app_title;
    help->title = TXT_WINDOW_TITLE;
    help->section = GetGizmoText( help->section);

    /*
     * we can't use PostGizmoHelp because the Gizmos requires its internal
     * AppName to be some string for the help system to work correctly.
     * Unfortunally the fontmgr needs that AppName to be
     * NULL so our "Apply Font" will work
     */
    memset( &req, 0, sizeof(req));
    req.rqtype         = DT_DISPLAY_HELP;
    req.serial         = 0;
    req.version        = 1;
    req.client         = XtWindow(w);
    req.nodename       = NULL;
    req.source_type    = LookupHelpType(help->section);
    req.app_name       = "fontmgr";
    req.app_title      = GetGizmoText(help->app_title);
    req.title          = GetGizmoText(help->title);
    req.help_dir       = NULL;
    req.file_name      = help->filename;
    req.sect_tag       = help->section;
    (void)DtEnqueueRequest(XtScreen(w), _HELP_QUEUE(XtDisplay(w)),
      _HELP_QUEUE(XtDisplay(w)), XtWindow(w), (DtRequest *)&req);

} /* end of HelpCB */



void
PopdownError()

{
    if (error.shell)
	XtPopdown( error.shell);
    callRegisterHelp(app_shellW, ClientName, TXT_HELP_MAIN_MENU);

}

void
PopdownPrompt()
{
    if (prompt.shell)
	XtPopdown( prompt.shell);

    callRegisterHelp(app_shellW, ClientName, TXT_HELP_MAIN_MENU);
} /* end of NoticeCB */


void
InformUser( text)
    char *text;
{
    XtVaSetValues(view->footer_text, XtNleftFoot, GetGizmoText(text), 0);

} /* end of InformUser */



void
PopupErrorMsg( text, ok_CB, ok_data, title, help_index, section)
    char *text;
    XtPointer ok_CB, ok_data;
    char *title;
    XtPointer help_index;
    char *section;
{
#define OK_INDEX     0
#define HELP_INDEX   1

    if (!error.shell) {
	error.title = GetGizmoText(title);
	CreateGizmo(app_shellW, ModalGizmoClass, &error, NULL, 0);
    }

    
    OlVaFlatSetValues( error_menu.child, OK_INDEX,
			  XtNselectProc, ok_CB, XtNclientData, ok_data, 0);
    OlVaFlatSetValues( error_menu.child, HELP_INDEX,
		 XtNclientData, help_index, 0);
    
    SetModalGizmoMessage(&error, GetGizmoText(text));
    
    callRegisterHelp(error.shell, ClientName, section);
    MapGizmo(ModalGizmoClass, &error);
}

void
PromptUser( text, ok_CB, ok_data, cancel_CB, cancel_data, help_index,title, section)
    char *text;
    XtPointer ok_CB, ok_data, cancel_CB, cancel_data, help_index;
    char *title;
    char *section;
{
#define OK_INDEX     0
#define CANCEL_INDEX 1
#define HELP_INDX   2

    if (!prompt.shell) {
	prompt.title = GetGizmoText(title);
	CreateGizmo(app_shellW, ModalGizmoClass, &prompt, NULL, 0);
    }

    
	XtVaSetValues(prompt.shell, XtNtitle, GetGizmoText(title), 0);
    OlVaFlatSetValues( prompt_menu.child, OK_INDEX,
			  XtNselectProc, ok_CB, XtNclientData, ok_data, 0);
    OlVaFlatSetValues( prompt_menu.child, CANCEL_INDEX,
		 XtNselectProc, cancel_CB, XtNclientData, cancel_data, 0);
    OlVaFlatSetValues( prompt_menu.child, HELP_INDX,
		 XtNclientData, help_index, 0);
    
    SetModalGizmoMessage(&prompt, GetGizmoText(text));
    
    callRegisterHelp(prompt.shell, title, section);
    MapGizmo(ModalGizmoClass, &prompt);
	    
} /* end of PromptUser */


void
BusyCursor( w)
    Widget w;
{
    if (!XtIsRealized(view->form))
	return;

    /* if there is no widget then operate on the base view */
    if (w == NULL) {
	w = view->form;
	XDefineCursor(XtDisplay(w),
		  XtWindow(w),
		  GetOlBusyCursor(XtScreen(w)));
	w = view->sample_text;
	XDefineCursor(XtDisplay(w),
		  XtWindow(w),
		  GetOlBusyCursor(XtScreen(w)));
	w = view->ps_text;
	XDefineCursor(XtDisplay(w),
		  XtWindow(w),
		  GetOlBusyCursor(XtScreen(w)));
    }
    else
	XDefineCursor(XtDisplay(w),
		  XtWindow(w),
		  GetOlBusyCursor(XtScreen(w)));

} /* end of BusyCursor */


void
StandardCursor( w)
    Widget w;
{
    if (!XtIsRealized(view->form))
	return;

    /* if there is no widget then operate on the base view */
    if (w == NULL) {
	w = view->form;
	XDefineCursor(XtDisplay(w),
		  XtWindow(w),
		  GetOlStandardCursor(XtScreen(w)));
	w = view->sample_text;
	XDefineCursor(XtDisplay(w),
		  XtWindow(w),
		  GetOlStandardCursor(XtScreen(w)));
	w = view->ps_text;
	XDefineCursor(XtDisplay(w),
		  XtWindow(w),
		  GetOlStandardCursor(XtScreen(w)));
    }
    else
	XDefineCursor(XtDisplay(w),
		  XtWindow(w),
		  GetOlStandardCursor(XtScreen(w)));

} /* end of StandardCursor */


char *
GetNextField( delimiter, p, field, len)
    char delimiter;
    char *p;
    char **field;
    int *len;
{
    char *fieldP = *field;
    
    if (*p)
	fieldP = ++p;
    if (*p == delimiter || *p == '\0') {
	fieldP = "";
	*len = 0;
    } else {
	while (*p && *++p != delimiter);
	*len = p - fieldP;
    }

    *field = fieldP;
    return p;

} /* end of GetNextField */


/*
 * capitalize the first character of every space seperated word in a string
 */
void
CapitalizeStr(str)
     char *str;
{
  int i;
  Boolean cap_next_char = False;

  for (i=0; str[i]; i++) {
    if (i==0)
      /* uppercase first character in string */
      str[i] = toupper(str[i]);
    else if (str[i]==' ')
      cap_next_char = True;
    else if (cap_next_char) {
      str[i] = toupper(str[i]);
      cap_next_char = False;
    }
  }
} /* end of CapitalizeStr */


void
UppercaseStr( str)
    String str;
{
    int i;
  
    for(i=0; str[i]; i++)
	str[i] = toupper(str[i]);

} /* end of UppercaseStr */


String
LowercaseStr( str)
    String str;
{
    int i;
  
    for(i=0; str[i]; i++)
	str[i] = tolower(str[i]);
    return str;

} /* end of LowercaseStr */


/* locate position of char, returns TRUE if not end of line */
Boolean
FindChar(line, character)
    char **line;
    char character;
{
    char *ptr = *line;

    /* while not end of string do */
    for (; *ptr; ptr++)
	/* if match then break */
	if (*ptr == character)
	    break;
    
    *line = ptr;
    if (*ptr)
	return TRUE;
    else
	return FALSE;
} /* end of FindChar */


    /* skip white spaces, returns TRUE if not end of line */
Boolean
SkipSpace( line)
    char **line;
{
    char *ptr = *line;

    /* while not end of string do */
    for (; *ptr; ptr++)
	/* if not white space char then break */
	if (!isspace(*ptr))
	    break;
    
    *line = ptr;
    if (*ptr)
	return TRUE;
    else
	return FALSE;
} /* end of SkipSpace */


/* find first  white space, returns TRUE if not end of line */
Boolean
FindSpace( line)
    char **line;
{
    char *ptr = *line;

    /* while not end of string do */
    for (; *ptr; ptr++)
	/* if white space char then break */
	if (isspace(*ptr))
	    break;
    
    *line = ptr;
    if (*ptr)
	return TRUE;
    else
	return FALSE;
} /* end of FindSpace */


void
InitStringDB(info)
     string_array_type *info;
{
  info->alloc_strs = STR_ALLOC;
  info->strs = (char **) XtMalloc( sizeof(char *) * info->alloc_strs);
  info->n_strs = 0;
} /* end of InitStringDB */


void
InsertStringDB(info, str)
     string_array_type *info;
     char *str;
{
    /* if inserting into unallocated DB */
    if (info->alloc_strs <= 0) {
	info->alloc_strs = STR_ALLOC;
	info->strs = (char **) XtMalloc( sizeof(char *) * info->alloc_strs);
	info->n_strs = 0;
    }
	/* allocate more space if needed */
    else if (info->n_strs >= info->alloc_strs) {	
	info->alloc_strs += STR_ALLOC;
	info->strs = (char **) XtRealloc( (String) info->strs,
			      sizeof(char *) * info->alloc_strs);
    }
#ifdef DEBUG
fprintf(stderr, "cnt=%d inserting new string %s\n",info->n_strs, str);
#endif
    info->strs[info->n_strs] = XtNewString(str);
    (info->n_strs)++;
} /* end of InsertStringDB */


void
DeleteStringsDB(info)
     string_array_type *info;
{
    int i;

    if (info->n_strs < 0) {
	return;
	}
    for(i=0; i<info->n_strs; i++) {
	XtFree(info->strs[i]);
    }
    info->n_strs = 0;
} /* end of DeleteStringsDB */


Boolean
FileOK(file)
     FILE *file;
{
  struct stat statb;
  ushort not_normal_file;

  if (file == 0)
    return False;
  if (fstat (fileno(file), &statb) == -1) {
    fclose(file);
    return False;
  }

  not_normal_file = statb.st_mode & 070000;  /* octal mask */
  /* if file is a directory or special */
  if (not_normal_file)
      return FALSE;
  else
      return True;
} /* end of FileOK */


FILE *
ForkWithPipe(pid, cmd, arg0, arg1, arg2, arg3, arg4)
int *pid;
char *cmd, *arg0, *arg1, *arg2, *arg3, *arg4;
{
	int fildes[2];
	int fd;
	char errbuf[512];
	FILE *fp;

	if (pipe(fildes) != 0) {
		sprintf(errbuf, GetGizmoText(FORMAT_UNABLE_OPEN_PIPE), cmd);
		perror(errbuf);
		exit(0);
	}
	switch ((*pid) = fork()) {
	case -1:
		{

			perror(errbuf);
			return(NULL);
		}
	case 0:		/* We are the child */
		if (close(1) == -1) {
			strcpy(errbuf, GetGizmoText(FORMAT_UNABLE_CLOSE_STDOUT));
			perror(errbuf);
			exit(1);
		}
		if (dup(fildes[1]) == -1) {
			strcpy(errbuf,GetGizmoText(COULD_NOT_DUP));
			perror(errbuf);
			exit(1);
		}
		/* Close stderr on forked command */
		(void)close(2);
		if (execl(cmd, cmd, arg0, arg1, arg2, arg3, arg4, NULL) == -1) {
			char errbuf[512];
			sprintf(errbuf, GetGizmoText(FORMAT_UNABLE_EXEC), cmd);
			perror(errbuf);
			exit(0);
		}
		break;
	default:	/* We are the parent */
		close(fildes[1]);
		fd = fildes[0];

		if ((fp = fdopen(fd, "r")) == NULL) {
			sprintf(errbuf, GetGizmoText(FORMAT_UNABLE_FDOPEN), cmd);
			perror(errbuf);
		}
		return(fp);
	}
} /* end of ForkWithPipe */


void
ForkWithoutPipe(pid, cmd, arg0, arg1, arg2, arg3, arg4)
int *pid;
char *cmd, *arg0, *arg1, *arg2, *arg3, *arg4;
{
	switch ((*pid) = fork()) {
	case -1:
		{
			char errbuf[512];

			sprintf(errbuf, GetGizmoText(FORMAT_UNABLE_FORK), cmd);
			perror(errbuf);
			return;
		}
	case 0:		/* We are the child */
		if (execl(cmd, cmd, arg0, arg1, arg2, arg3, arg4, NULL) == -1) {
			char errbuf[512];
			sprintf(errbuf, GetGizmoText(FORMAT_UNABLE_EXEC), cmd);
			perror(errbuf);
			exit(0);
		}
		break;
	default:	/* We are the parent */
		break;
	}
} /* end of ForkWithoutPipe */


/* ARGSUSED */
static Boolean DoWorkPiece(closure)
    XtPointer closure;		/* unused */
{
    WorkPiece piece = workQueue;

    if (piece) {
	(*piece->proc)(piece->closure);
	workQueue = piece->next;
	XtFree((XtPointer)piece);
	if (workQueue != NULL)
	    return False;
    }
    return True;
} /* end of DoWorkPiece */



/*
 * ScheduleWork( XtProc proc, XtPointer closure, int priority )
 *
 * Adds a WorkPiece to the workQueue in FIFO order by priority.
 * Lower numbered priority work is completed before higher numbered
 * priorities.
 *
 * If the workQueue was previously empty, then makes sure that
 * Xt knows we have (background) work to do.
 */
void
ScheduleWork( proc, closure, priority )
    XtProc proc;
    XtPointer closure;
    int priority;
{
    WorkPiece piece = XtNew(WorkPieceRec);

    piece->priority = priority;
    piece->proc = proc;
    piece->closure = closure;
    if (workQueue == NULL) {
	piece->next = NULL;
	workQueue = piece;
	XtAddWorkProc(DoWorkPiece, NULL);
    } else {
	if (workQueue->priority > priority) {
	    piece->next = workQueue;
	    workQueue = piece;
	}
	else {
	    WorkPiece n;
	    for (n = workQueue; n->next && n->next->priority <= priority;)
		n = n->next;
	    piece->next = n->next;
	    n->next = piece;
	}
    }
} /* end of ScheduleWork */


CountDashes (name, namelen)
    char    *name;
    int	    namelen;
{
    int ndashes = 0;

    while (namelen--)
	if (*name++ == '\055')	/* avoid non ascii systems */
            ++ndashes;
    return ndashes;
}


CopyISOLatin1Lowered (dst, src, len)
    char    *dst, *src;
    int	    len;
{
    register unsigned char *dest, *source;

    for (dest = (unsigned char *)dst, source = (unsigned char *)src;
	 *source && len > 0;
	 source++, dest++, len--)
    {
     	if ((*source >= XK_A) && (*source <= XK_Z))
            *dest = *source + (XK_a - XK_A);
	else if ((*source >= XK_Agrave) && (*source <= XK_Odiaeresis))
            *dest = *source + (XK_agrave - XK_Agrave);
	else if ((*source >= XK_Ooblique) && (*source <= XK_Thorn))
            *dest = *source + (XK_oslash - XK_Ooblique);
	else
            *dest = *source;
    }
    *dest = '\0';
}

void
callRegisterHelp(widget, title, section)
Widget widget;
char *title;
char *section;
{

  static OlDtHelpInfo help_info[]= {NULL, NULL, HELP_PATH, NULL, NULL};

  char * ptr;

	/* this routine is for registering MoOLIT help that
		comes up with the F1 key */
	/* NOTE: the last help registered is in effect when the F1
		key is hit, so each popup and popdown should register
		and re-register the correct help */

  help_info->filename =  HELP_PATH;
  help_info->app_title    =  GetGizmoText(TXT_WINDOW_TITLE);
  help_info->title    =  GetGizmoText(title);
  help_info->section = GetGizmoText(section);

#ifdef DEBUG
fprintf(stderr,"callRegisterHelp filename=%s\n",help_info->filename);
fprintf(stderr,"callRegisterHelp title=%s\n",help_info->title);
fprintf(stderr,"callRegisterHelp section=%s\n",help_info->section);
#endif

  OlRegisterHelp(OL_WIDGET_HELP,widget, ClientName, OL_DESKTOP_SOURCE,
   (XtPointer)&help_info);
}

