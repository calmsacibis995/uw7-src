#pragma ident	"@(#)dtm:olwsm/wsm.c	1.100"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <pwd.h>
#include <errno.h>
#include <libgen.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <X11/StringDefs.h>
#include <X11/ShellP.h>

#include <Xm/Xm.h>
#include <Xm/MessageB.h>

#include "WSMcomm.h"
#include "RootWindow.h"
 
#include "error.h"
#include <misc.h>
#include <node.h>
#include <xutil.h>  
#include <wsm.h>
 
extern String		CurrentLocale;
extern String		PrevLocale;

/*
 * Convenient macros:
 */

#define PUSH(p, data)		dring_push(p, alloc_DNODE((ADDR)data))
#define PULL(p, node)		free_DNODE(dring_delete(p, node))
#define MY_RESOURCE(x) \
	( \
		x >= DISPLAY->resource_base && \
		x <= (DISPLAY->resource_base | DISPLAY->resource_mask) \
	)
#define RC			"/.olinitrc"

#define WSMCantTouchThis	0		/* -- M.C. Hammer */
#define WSMDeleteWindow		1
#define WSMXKillClient		2
#define WSMSaveYourself		3

/*
 * Special types:
 */

typedef struct WinInfo {
	Window			win;
	short			fate;
}WinInfo, *WinInfoPtr;

/*
 * Global data:
 */

Widget			handleRoot;
Widget			workspaceMenu;
Widget			programsMenu;
Widget			InitShell;

/*
 * Local routines:
 */
static int		IgnoreErrors(Display *, XErrorEvent *);
Widget			CreateHandleRoot(void);
static void		RegisterHandleRoot(Widget);
static void		CheckRequestQueue(Widget, XtPointer, XEvent *);
static void		TerminateClient(Window, short);
static void		ServiceRequest(void);
static DNODE *		GetWindow(DNODE **, Window);
static void		WSMExec(Window, int, String);
static void		SetEnvironment(Widget	);
void			SetWSMBang(Display *,  Window, unsigned long);


static DNODE *		save_pending = NULL;
static String		SHELL;
static String		DEFAULT_SHELL	= "/bin/sh";

/**
 ** DmInitWSM()
 **/
int
DmInitWSM (int argc, String	argv[])
{
    SetEnvironment (InitShell);
    RegisterHandleRoot (handleRoot);
    InitProperty (DISPLAY);
    XSync (DISPLAY, False);
    ClearWSMQueue (DISPLAY);
    XSync (DISPLAY, False);
    return(0);				/* Always??!! */

}					/* DmInitWSM */

/**
 ** GetPath()
 **/
char * GetPath(char * name)
{
    char *		ptr;
    struct passwd *	pw;
    int			uid;

    if ((ptr = getenv("HOME")))
	return ((char *) CONCAT(ptr, name));

    if ((ptr = getenv("USER")))
	pw = getpwnam(ptr);
    else
    {
	uid = (int)getuid();
	pw = getpwuid(uid);
    }
    if (pw)
	return ((char *) CONCAT(pw->pw_dir, name));

    return ( (char *)NULL);
}					/* GetPath */

/**
 ** FooterMessage()
 **/
void FooterMessage(
	Widget			w,
	String			message,
	/* OlDefine		side, */
	Boolean			beep
)
{
    Widget top = NULL;
    Widget msg = NULL;
  
    if (beep && message)
    {
	XBell(DISPLAY, 100);		/* Ring keyboard bell at 100 percent vol */
	/* _OlBeepDisplay (w, 1); */
    }
    if (!message)
    {
	message = "";
    }

    /* Walk up the widget tree to the CategoryWidget--it owns the
     * footer space.
     */
    /* Actually, walk up to top most shell and then search for 
     * widget with name corresponding to standard name for footer
     * and then set label of footer to message.
     */
	
    top = (Widget) _XmFindTopMostShell(w);
    msg = XtNameToWidget(top, "*statusArea");
    if (msg)
    {
	XtVaSetValues(msg, 
		      XmNlabelString, XmStringCreateLocalized(message),
		      NULL);
	/* XtVaSetValues(msg, XmNvalue, message, NULL); */
    }
#ifdef DEBUG
    else
    {
	int found = False;
	Widget wTry = w;

	printf("FooterMessage:  Unable to find statusArea named widget\n");
	printf("                Trying for message box child\n");
	    
	while( (wTry != top) && (!found) && (wTry != NULL) )
	{
	    if(XtClass(wTry) == xmMessageBoxWidgetClass)
	    {
		found = True;
		XtVaSetValues(wTry, 
			      XmNmessageString, XmStringCreateLocalized(message),
			      NULL);
		printf("FooterMessage:  Found message box, setting messageString.\n");
	    }
	    else
	    {
		wTry = XtParent(wTry);
	    }
	}
    }
#endif
    return;
}					/* FooterMessage */

/**
 ** WSMExit()
 **/
void
WSMExit (void)
{
    extern int DtmExitCode;
    static Boolean first_time = True;

    /* After (normal) exit below, this will be called again from AtExit
     * in dtm.c - ignore it.
     */
    if (!first_time)
	return;

    first_time = False;

    if (save_pending) 
    {
	debug((stderr, "WSMExit: exiting with client-saves pending\n"));

	/*	Should we do something intelligent here
	 *	like post a notice?
	 */
    }

    (void)signal (SIGHUP, SIG_IGN);
    (void)signal (SIGTERM, SIG_IGN);
    (void)signal (SIGINT, SIG_IGN);

    XCloseDisplay (DISPLAY);
    kill (0, SIGTERM);			/* kill all processes in our group */
    exit (DtmExitCode);
    /*NOTREACHED*/
}					/* WSMExit */

/**
 ** IgnoreErrors()
 **/
static int IgnoreErrors (Display * dpy, XErrorEvent * event)
{
	return (0);
} /* IgnoreErrors */

/**
 ** CreateHandleRoot()
 **/
Widget CreateHandleRoot (void)
{
	Widget	w;
	
	/* Problem to fix:  Motif does not assign */
	/* a widget to the root window, therefore */
	/* cannot get XtWindowToWidget where no   */
	/* widget exists.  As temporary fix, will */
	/* assign handleroot to be widget of Window */
	/* for InitShell (in a round-about way). */
	
	/* w = XtWindowToWidget(XtDisplay(InitShell), 
	      XtWindowOfObject(InitShell)); */
	/*w  = XtWindowToWidget(DISPLAY, ROOT); */
	
	/* Something new */
	w  = XtWindowToWidget(DISPLAY, ROOT);
	
	if (!w) 
	  {
	    w = XtVaCreateManagedWidget("root", rootWindowWidgetClass, InitShell,
	                                NULL);
	    
	    if(!w)
	      {
		Dm__VaPrintMsg(TXT_errorMsg_noWidget);
	      }
	      else {
		XtRealizeWidget(w);
	      }
	  }
	return (w);
} /* CreateHandleRoot */

/**
 ** RegisterHandleRoot()
 **/
static void RegisterHandleRoot (Widget	handle)
{
	/*
	 * Set up service on workspace manager queue.
	 */
	XtAddEventHandler (
		handle, PropertyChangeMask, False,
		(XtEventHandler)CheckRequestQueue, NULL
	);

	return;
} /* RegisterHandleRoot */

/**
 ** CheckRequestQueue()
 **/
static void CheckRequestQueue (
	Widget			w,
	XtPointer		client_data,
	XEvent *		event 
	)
{
	XPropertyEvent *	p = (XPropertyEvent *)event;

	/*
	 * Check for workspace manager request and service:
	 */
	if( (p->atom == XA_WSM_QUEUE(DISPLAY)) && 
	    (p->state == PropertyNewValue))
	    {
		ServiceRequest ();
	    }
	return;
} /* CheckRequestQueue */


/**
 ** TerminateClient()
 **/
static void TerminateClient (
	Window			client,
	short			action
	)
{
	debug ((
		stderr,
		"TerminateClient: action=%s clients=%x\n",
		(action == WSMDeleteWindow? "DeleteWindow":
		(action == WSMXKillClient ? "KillClient": "UNKNOWN")),
		client
	));

	switch (action)
	{

	case WSMDeleteWindow:
		debug ((stderr, "SendProtocolMessage\n"));
		SendProtocolMessage(
			DISPLAY, client,
			XA_WM_DELETE_WINDOW(DISPLAY), CurrentTime
		);
		break;

	case WSMXKillClient:
		/*
		 * Don't kill our windows!
		 */
		if (!MY_RESOURCE(client))
		  {
			debug ((stderr, "XKillClient\n"));
			XSync (DISPLAY, False);
			XKillClient (DISPLAY, client);
			XSync (DISPLAY, False);
		  } 
		else
		  {
			debug ((stderr, "No XKillClient, WSM client!\n"));
		  }
		break;

	default:
		debug((stderr, "TerminateClient: unknown action\n"));
		break;
	}

	return;
} /* TerminateClient */

/**
 ** ServiceEvent()
 **/
void ServiceEvent(XEvent *event)
{
  if (event->type == PropertyNotify)
    {
      XPropertyEvent *p = (XPropertyEvent *)event;
      DNODE *q;

      /*
       * Check for client notification of SAVE_YOURSELF done:
       */
      if (p->window != ROOT && p->atom == XA_WM_COMMAND)
	{
	  printf("Client notification of command: ServiceEvent\n");
	  
	  if (q = GetWindow( &save_pending, p->window ))
	    {
	      short action ;

	      action = ((WinInfoPtr)DNODE_data(q))->fate ;
	      FREE( DNODE_data(q) ) ;
	      PULL( &save_pending, q ) ;

	      /*
	       * Is client now due for termination?
	       */
	      if (action != WSMCantTouchThis)
	      {
		  int (*prev_handler)() = XSetErrorHandler( IgnoreErrors ) ;
		  XSync( DISPLAY, False ) ;
		  TerminateClient( p->window, action) ;
		  XSync( DISPLAY, False ) ;
		  XSetErrorHandler( prev_handler ) ;
	      }
	    }
	  else
	    {
	      debug((stderr, "ServiceEvent: unknown window\n"));
	    }
	  return;
	}
    }
  
  XtDispatchEvent(event);
  /* XtAppDispatchEvent( app_context, event ) ; */  /* XtApp not defined ? */ /* normal Xt dispatch */

  return;
} /* ServiceEvent */

/**
 ** ServiceRequest()
 **/
static void ServiceRequest (void)
{
	unsigned char		type;
	Window			client;
	int			serial;
	String			name;
	String			command;
	WSM_Request		request;
	short			action;
	DNODE *			p;
	WinInfoPtr		info;
	int			(*prev_handler)();


	trace("ServiceRequest: IN");

	XSync (DISPLAY, False);
	prev_handler = XSetErrorHandler (IgnoreErrors);

	/*
	 * Service all queued workspace manager requests:
	 */
	while (DequeueWSMRequest(DISPLAY, &client, &type, &request)
		== GOTREQUEST) 
	  {

		serial  = request.serial;
		name    = request.name;
		command = request.command;

		switch (type) 
		{

		case WSM_EXECUTE:
			debug((stderr, "%s: WSM_EXECUTE\n", name));
			WSMExec (client, serial, command);
			break;

		case WSM_TERMINATE:
			debug ((stderr, "%s: WSM_TERMINATE %s\n", name, command));
			if (command && MATCH(command, "WM_DELETE_WINDOW"))
			  {
			        action = WSMDeleteWindow;
			  }
			else
			  {
				action = WSMXKillClient;
			  }

			if ((p = GetWindow(&save_pending, client))) 
			  {
				/*
				 * Postpone death until client is
				 * through saving itself.
				 */
				((WinInfoPtr)DNODE_data(p))->fate = action;
			  } 
			else
			  {
			        TerminateClient (client, action);
			  }
			break;

		case WSM_SAVE_YOURSELF:
			debug((stderr, "%s: WSM_SAVE_YOURSELF\n", name));
			XSelectInput (DISPLAY, client, PropertyChangeMask);
			/*
			 * Old protocol...
			 */
			SetWSMBang (DISPLAY, client, NoEventMask);
			/*
			 * New protocol...
			 */
			SendProtocolMessage (
			    DISPLAY, client,
			    XA_WM_SAVE_YOURSELF(DISPLAY), CurrentTime
			);
			info = ELEMENT(WinInfo);
			info->win = client;
			info->fate = WSMCantTouchThis;
			PUSH (&save_pending, info);
			break;

		case WSM_EXIT:
			debug((stderr, "%s: WSM_EXIT\n", name));
			WSMExit ();
			/*NOTREACHED*/

		case WSM_MERGE_RESOURCES:
			debug((stderr, "%s: WSM_MERGE_RESOURCES\n", name));
			MergeResources (command);
			break;

		case WSM_DELETE_RESOURCES:
			debug((stderr, "%s: WSM_DELETE_RESOURCES\n", name));
			DeleteResources (command);
			break;

		default:
			debug((stderr, "%s: UNKNOWN REQUEST\n", name));
			break;
		}
	  }
	XSync (DISPLAY, False);
	XSetErrorHandler (prev_handler);
	trace("ServiceRequest: OUT");

	return;
} /* ServiceRequest */

/**
 ** GetWindow()
 **/
static DNODE * GetWindow (
	DNODE **		root,
	Window			window
	)
{
	DNODE *			p;
	dring_ITERATOR		I;


	I = dring_iterator(root);
	while (p = dring_next(&I)) 
	  {
		if (window == ((WinInfoPtr)DNODE_data(p))->win) 
		  {
			return (p);
		  }
	  }
	return (0);
} /* GetWindow */

/**
 ** WSMExec()
 **/
static void WSMExec (
	Window			client,
	int			serial,
	String			command
	)
{
	int			pid;
	static WSM_Reply	reply;


	reply.serial = serial;

	switch (pid = fork()) 
	{
	  case 0:
		signal (SIGCLD, SIG_DFL);
		(void)execl (SHELL, "sh", "-c", command, (String)0);
		(void)execl (DEFAULT_SHELL, "sh", "-c", command, (String)0);
		reply.detail = errno;
		SendWSMReply (DISPLAY, client, WSM_EXEC_FAILURE, &reply);
		exit (-1);
		/*NOTREACHED*/
	  case -1:
		reply.detail = errno;
		SendWSMReply (DISPLAY, client, WSM_FORK_FAILURE, &reply);
		break;
	  default:
		reply.detail = pid;
		SendWSMReply (DISPLAY, client, WSM_SUCCESS, &reply);
		break;
	}

	return;
} /* WSMExec */

/**
 ** ExecRC()
 **
 **	An attempt to minimize contention at startup: in the olinitrc file,
 **	DO NOT start the window manager in the background.  This way, the
 **	initialization code for the two applications is run sequentially.
 **/
void 
ExecRC (String path)
{
#define WAIT_FOR_RC  (unsigned) 240

    if ((path == NULL) || (access(path, 4) != 0))
    {
	Dm__VaPrintMsg(TXT_warningMsg_noFile, path);
	return;
    }

    if (fork() == 0)
    {
	/* Make sure that DEFAULT_SHELL - Bourne Shell is used here
	   to interpret .olinitrc */
	(void)execl (DEFAULT_SHELL, "sh", path, (String)0);
	_exit(0);

    } else 
    {
	/* In parent, wait for startup child shell.  An alarm is used so we
	 * don't wait forever if someone forgot to run program in background.
	 */
	(void)alarm(WAIT_FOR_RC);
	(void)wait(NULL);
	(void)alarm(0);			/* Child started so disable alarm */
    }
}					/* end of ExecRC */

/**
 ** SetEnvironment()
 **/
static void
SetEnvironment (Widget	w)
{
#define FORMAT	"DISPLAY=%s"

	static char		buf[128];

	String			p;
	String			sdisplay;

	Cardinal		len;


	if (!(SHELL = getenv("SHELL")) || !*SHELL)
		SHELL = DEFAULT_SHELL;

	sdisplay = XDisplayString(XtDisplayOfObject(w));

	len = sizeof(FORMAT) + strlen(sdisplay) + 1;
	if (len > XtNumber(buf))
	  {
		p = XtMalloc(len);
          }
	else
	  {
		p = buf;
	  }

	sprintf (p, FORMAT, sdisplay);
	putenv (p);

	/*
	 * Don't free "p", as the environment is still using it!
	 */

	return;
}

/**
 ** SetWSMBang()
 **/
void
SetWSMBang (
	Display *		display,
	Window			send_to,
	unsigned long		mask
	)
{
	XEvent			sev;

	sev.xclient.type = ClientMessage;
	sev.xclient.display = display;
	sev.xclient.window = send_to;
	sev.xclient.message_type = XA_BANG(display);
	sev.xclient.format = 8;
	XSendEvent (display, send_to, False, mask, &sev);

	return;
} /* SetWSMBang */
