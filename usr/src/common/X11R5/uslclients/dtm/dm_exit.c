#pragma ident	"@(#)dtm:dm_exit.c	1.6"

/*
 * dm_exit.c
 *
 */

#include <stdio.h>

#include <X11/Xlib.h>

#include <X11/Intrinsic.h>

#include <dm_exit.h>
#include <Dtm.h>

#ifdef DEBUG

#   define DPRINTF(x)   (void)fprintf x

#else /* DEBUG */

#   define DPRINTF(x)   

#endif /* DEBUG */

#define KillClient               0
#define UnmappedWindow           1
#define SaveYourself             2
#define DeleteWindow             4
#define SaveAndDelete            6
#define SessionWindow            8
#define WindowDied              16
#define TransientWindow         32
#define Leader                  64

#define XA_WM_COMMAND(d)        XInternAtom((d), "WM_COMMAND", False)

#define ErrorIgnore(dpy)                                                     \
   XSync(dpy, False);                                                        \
   Session.xerror = 0;                                                       \
   Session._XDefaultError = XSetErrorHandler(IgnoreErrors);

#define ErrorNotice(dpy)                                                     \
   XSync(dpy, False);                                                        \
   XSetErrorHandler(Session._XDefaultError);

SessionManagementInfo Session;

/* FLH REMOVE this when moving routine to olwsm */
void SendProtocolMessage(Display *dpy, Window w, Atom protocol, unsigned long time);


static int  IgnoreErrors     (Display * dpy, XErrorEvent * event);
static int  WindowOffScreen  (Display * dpy, Window win);
static int  WindowIsTransient(Display * dpy, Window win);
static int  GetProtocol      (Display * dpy, Window win, Window * win_return);
static void SendSaveYourself (Display * dpy, Window win);
static void SendDeleteWindow (Display * dpy, Window win);
static void HandleWMCommand  (Display * dpy, Window win);

/*
 * IgnoreErrors
 *
 * This function is used to track X protocol errors that happen during critical
 * sections of the exit handling code.  The variable \fISession.xerror\fP is 
 * used to feedback the fact that an error occurred in the critical section.
 *
 * Only BadWindows are considered non-fatal.  All other errors are passed to 
 * the default error handler.  The pointer to this function is stored in  
 * \fISession._XDefaultError\fP by the ErrorIgnore macro.
 *
 */

static int
IgnoreErrors(Display * dpy, XErrorEvent * event)
{

   Session.xerror = 1;

   switch(event->error_code)
   {
         case BadWindow:
            return (0);
            break;
         default:

#        ifdef COMPLETELY_BUG_FREE

            return (Session._XDefaultError(dpy, event));

#        else /* COMPLETELY_BUG_FREE */

            DPRINTF((stderr,"ignoring error %x\n", event->error_code));
            return (0);

#        endif /* COMPLETELY_BUG_FREE */
   }

} /* end of IgnoreErrors */
/*
 * WindowOffScreen
 *
 * This function determines if the coordinates stored in the window attributes
 * pointed to by \fIwin_attr\fP define a window that is on the default screen
 * of the Display \fIdpy\fP is at least partially visible.  The function can
 * determine either of the states: WindowDied (if the window becomes invalid
 * in the midst of this routine), UnmappedWindow (if the Window is mapped and
 * has at least 1 pixel within the limits of the screen), or KillClient (if
 * the window is mapped and has at least 1 pixel visible).
 *
 */

static int
WindowOffScreen(Display * dpy, Window win)
{
   static Display *  prev_dpy = NULL;
   static int        dpy_wid  = 0;
   static int        dpy_ht   = 0;

   int               protocol;
   XWindowAttributes win_attr;

   XGetWindowAttributes(dpy, win, &win_attr);

   if (Session.xerror)
      protocol = WindowDied;
   else
   {
      if (win_attr.map_state != IsViewable)
         protocol = UnmappedWindow;
      else
      {

         if (dpy != prev_dpy)
         {
            prev_dpy = dpy;
            dpy_wid  = WidthOfScreen(XDefaultScreenOfDisplay(dpy));
            dpy_ht   = HeightOfScreen(XDefaultScreenOfDisplay(dpy));
         }

         if (win_attr.x                   > dpy_wid     ||
             win_attr.y                   > dpy_ht      ||
             win_attr.x + win_attr.width  < 0           ||
             win_attr.y + win_attr.height < 0)
            protocol = UnmappedWindow;
         else
            protocol = KillClient;
      }
   }

   return (protocol);

} /* end of WindowOffScreen */
/*
 * WindowIsTransient
 *
 * This procedure determines if the given Window \fIwin\fP on the 
 * Display \fIdpy\fP is a transient window.  These windows will be
 * ignored, since they presumably are transient for some other
 * window (i.e., the group_leader for the given \fIwin\fP) which will
 * be (or has been) handled.
 *
 */

static int
WindowIsTransient(Display * dpy, Window win)
{
   Window            group_leader  = 0;

   int               is_transient;
   int               protocol = KillClient;

   is_transient = ((XGetTransientForHint(dpy, win, &group_leader)) != 0);

   if (Session.xerror)
      protocol = WindowDied;
   else
      if (is_transient && group_leader && group_leader != win)
         protocol = TransientWindow;

   return (protocol);

} /* end of WindowIsTransient */
/*
 * GetProtocol
 *
 * This \fIrecursive\fP function walks down the window branch searching for a
 * window that has requested participation in the WM_PROTOCOLS WM_DELETE_WINDOW
 * and/or WM_SAVE_YOURSELF.  When such a window is found the routine returns
 * the protocol(s) requested and stores the window requesting the protocol(s)
 * in the storage pointed to be the parameter \fIwin_return\fP.  The search
 * progresses downward for immediate children of the RootWindow.  If it reaches
 * the end of the branch without finding a window that has requested a protocol,
 * win_return is unchanged.  This logic accounts for the possibility of using a
 * decorating window manager.  By searching downward we'll be looking at the
 * window manager windows first then on to the real client windows.  We'll only
 * deal with windows that actually ask for the protocol, all others will be
 * ignored.  It is presumed that these windows will be killed when the session
 * manager exits (courtesy of the init program olinit or xinit).
 *
 */

static int
GetProtocol(Display * dpy, Window win, Window * win_return)
{
   Window *          p;
   Window *          children      = NULL;
   unsigned int      num_children  = 0;
   int               protocol;
   int               n;
   Window            root;
   Window            parent;
   Atom *            atoms         = NULL;

   if ((protocol = WindowOffScreen(dpy, win)) == KillClient)
   {
      if (XtWindowToWidget(dpy, win) != NULL)
         if (win != Session.SessionLeader && win != Session.SessionLeaderIcon)
         {
            protocol = SessionWindow;
            *win_return = win;
         }
         else
            protocol = Leader;
      else
         if ((protocol = WindowIsTransient(dpy, win)) == KillClient)
         {
            if (XGetWMProtocols(dpy, win, &atoms, &n) != 0)
            {
               if (Session.xerror)
                  protocol = WindowDied;

               if (atoms != NULL)
               {
                  if (!Session.xerror)
                     while (n--)
                     {
                        if (atoms[n] == XA_WM_DELETE_WINDOW(dpy))
                           protocol |= DeleteWindow;
                        else
                           if (atoms[n] == XA_WM_SAVE_YOURSELF(dpy))
                              protocol |= SaveYourself;
                     }
                  XFree((char *)atoms);
               }
            }
            if (protocol != KillClient)
               *win_return = win;
            else
            {
               XQueryTree(dpy, win, &root, &parent, &children, &num_children);
               if (Session.xerror)
                  protocol = WindowDied;
               else
                  if (children)
                  {
                     for (p = children; p < &children[num_children]; p++)
                        if ((protocol = GetProtocol(dpy, *p, win_return)))
                           break;
                     XFree((char *)children);
                  }
            }
         }
   }

   return (protocol);

} /* end of GetProtocol */
/*
 * QueryAndKillWindowTree
 *
 * This function returns the number of mapped toplevel windows of the Display 
 * \fIdpy\fP that are pending removal.  It performs this function by calling 
 * XQueryTree on the RootWindow to get the immediate descendants of the root, 
 * then calling GetProtocol for each of these children to traverse the branch.
 * GetProtocol returns the appropriate protocol to use in trying to close the 
 * window.  The session manager is interested in only windows whose protocol 
 * are: SaveYourself, SaveAndDelete, DeleteWindow, or SessionWindow.  These
 * windows are expecting a protocol message, all others are not.
 *
 * Note that windows that are looking for both SaveAndDelete are treated
 * the same as SaveYourself windows since this is the session manager.
 *
 */

extern int
QueryAndKillWindowTree(Display * dpy)
{
   Window            win                 = DefaultRootWindow(dpy);
   Window            root;
   Window            parent;
   Window *          children            = NULL;
   unsigned int      num_children        = 0;
   Window *          p;

   XQueryTree(dpy, win, &root, &parent, &children, &num_children);
   
   if (children)
   {
      for (p = children; p < &children[num_children]; p++)
      {
         ErrorIgnore(dpy);

         win = *p;

         switch(GetProtocol(dpy, *p, &win))
         {
            default: 
            case UnmappedWindow:
            case WindowDied:
            case TransientWindow:
            case KillClient:
            case Leader:
               DPRINTF((stderr, "Ignore: 0x%lx (0x%lx)\n", *p, win));
               break;
            case SessionWindow:
            case DeleteWindow:
               SendDeleteWindow(dpy, win);
               break;
            case SaveAndDelete:
            case SaveYourself:
               SendSaveYourself(dpy, win);
               break;
         }
         ErrorNotice(dpy);
      }

      XFree((char *)children);
   }

   DPRINTF((stderr,"children left = %d\n", Session.WindowKillCount));

   return (Session.WindowKillCount);

} /* end of QueryAndKillWindowTree */
/*
 * SendSaveYourself
 *
 * This procedure sends the WM_SAVE_YOURSELF protocol message to the Window 
 * \fIwin\fP on the Display \fIdpy\fP.  The routine adds PropertyChangeMask 
 * and StructureNotifyMask to the event mask for the window.  The 
 * PropertyChangeMask is used to track the WM_COMMAND property of the window.
 * When this property changes, the window can be killed.  To avoid problems if
 * the client window is destroyed prior to setting WM_COMMAND, 
 * StructureNotifyMask is selected.  If the window is destroyed a DestroyNotify
 * event will be sent and the internal data element \fISession.WindowKillList\fP
 * is updated accordingly.
 *
 * If the window is already in the \fISession.WindowKillList\fP then the window
 * is ignored (since it has already been sent the message).
 *
 */

static void
SendSaveYourself(Display * dpy, Window win)
{
   int i;

   for (i = 0; i < Session.WindowKillCount; i++)
      if (Session.WindowKillList[i] == win)
         return;

   DPRINTF((stderr, "SendSaveYourself: 0x%lx\n", win));

   XSelectInput(dpy, win, PropertyChangeMask | StructureNotifyMask);
   XSync(dpy, False);

   if (!Session.xerror)
   {
      SendProtocolMessage(dpy, win, XA_WM_SAVE_YOURSELF(dpy), CurrentTime);

      Session.WindowKillList = (Window *)
         XtRealloc((char *)Session.WindowKillList, 
                   (Session.WindowKillCount + 1) * sizeof(Window));

      Session.WindowKillList[Session.WindowKillCount++] = win;
   }

} /* end of SendSaveYourself */
/*
 * SendDeleteWindow
 *
 * This procedure is used to send the WM_DELETE_WINDOW protocol to the Window 
 * \fIwin\fP on the Display \fIdpy\fP.  So that the session manager will know
 * when these windows die, StructureNotifyMask is added to the event mask for
 * the window.  This allows the session manager to accurately maintain the 
 * \fISession.WindowKillList\fP.
 *
 * If the window is already in the \fISession.WindowKillList\fP then the window
 * is ignored (since it has already been sent the message).
 *
 */

static void
SendDeleteWindow(Display * dpy, Window win)
{
   int i;

   for (i = 0; i < Session.WindowKillCount; i++)
      if (Session.WindowKillList[i] == win)
         return;

   DPRINTF((stderr, "SendDeleteWindow 0x%lx\n", win));

   XSelectInput(dpy, win, StructureNotifyMask);
   XSync(dpy, False);

   if (!Session.xerror)
   {
      SendProtocolMessage(dpy, win, XA_WM_DELETE_WINDOW(dpy), CurrentTime);

      Session.WindowKillList = (Window *)
         XtRealloc((char *)Session.WindowKillList, 
                   (Session.WindowKillCount + 1) * sizeof(Window));

      Session.WindowKillList[Session.WindowKillCount++] = win;
   }

} /* end of SendDeleteWindow */
/*
 * HandleWMCommand
 *
 * This procedure retreives the WM_COMMAND property for the Window \fIwin\fP 
 * on Display \fIdpy\fP.
 *
 * Note: Nothing is done with the command now.  Something should be done in 
 * the future.  WM_COMMANDS that arrive while \fISession.ProcessTerminating\fP
 * is set should be stored in some file used to restart the session.  This file
 * should be re-created whenever \fISession.ProcessTerminating\fP is turned on.
 *
 */

static void
HandleWMCommand(Display * dpy, Window win)
{
   int    i;
   char * command ;

   ErrorIgnore(dpy);
#ifdef MOTIF_WM_PROPERTY
   command = GetCharProperty(dpy, win, XA_WM_COMMAND(dpy), &i);
#else
   command = NULL;
#endif
   ErrorNotice(dpy);

   if (!Session.xerror)
   {
      DPRINTF((stderr, "retrieved"));
      while (*command)
      {
         DPRINTF((stderr, " '%s'", command));
         command += (strlen(command) + 1);
      }
      DPRINTF((stderr, ".\n"));
   }

} /* end of HandleWMCommand */
/*
 * HandleWindowDeaths
 *
 * This procedure is called from within the custom main loop of the session
 * manager.  It checks to see if the current event is interesting, from a
 * session manager's point-of-view, and handles it accordingly.  The 
 * interesting events are DestroyNotify and PropertyNotify where the property
 * changed is WM_COMMAND.  The latter event is always handled (using the
 * \fIHandleWMCommand\fP procedure).  Both events are processed only if the 
 * flag \fISessionProcessTerminating\fP is set.  In this case the window is
 * processed if it appears in the \fISession.WindowKillList\fP by doing an 
 * XKillClient for WM_COMMAND responses and removing the window from the kill
 * list.
 *
 * Session termination is handled in phases.  Once on set of windows is
 * shutdown, this routine checks to see if any new ones arrived.  If so
 * processing continues, otherwise RealExit is called to effect an exit().
 *
 */

extern void
HandleWindowDeaths(XEvent * event)
{

   if (Session.ProcessTerminating)
   {
      int       i;
      Display * dpy            = event->xany.display;
      Window    win            = event->xany.window;
      int       got_destroy    = (event->type == DestroyNotify) ||
                                 (event->type == UnmapNotify);
      int       got_wm_command = (event->type == PropertyNotify && 
                                  event->xproperty.atom == XA_WM_COMMAND(dpy));

      if (got_wm_command)
         HandleWMCommand(dpy, win);

      if (got_wm_command || got_destroy)
      {
         for (i = 0; i < Session.WindowKillCount; i++)
            if (win == Session.WindowKillList[i])
            {
               if (got_wm_command)
               {
                  ErrorIgnore(dpy);
                  XKillClient(dpy, win);
                  ErrorNotice(dpy);
               }
               if (--Session.WindowKillCount)
               {
                  Session.WindowKillList[i] = 
                     Session.WindowKillList[Session.WindowKillCount];
               }
               break;
            }
      }

      DPRINTF((stderr,"killcount = %d\n", Session.WindowKillCount));

      if (Session.WindowKillCount == 0)
         if (QueryAndKillWindowTree(dpy) == 0)
            RealExit(Session.ProcessTerminating);
   }

} /* end of HandleWindowDeaths */
/*
 * RealExit
 *
 * This routine is called when the session manager is supposed to exit as a
 * result of completing the graceful shutdown of all clients.  It's always
 * called with the value of the \fISession.ProcessTerminating\fP variable
 * which is carefully maintained as either 0 (indicating that the session
 * isn't terminating and therefore this routine would not be called), 1
 * (indicating that the session should terminate normally), or 43 (indicating
 * that the user also selected to shutdown the OS.
 *
 */

extern void
RealExit(int rc_plus_one)
{
    exit(rc_plus_one - 1);
}					/* end of RealExit */

#ifdef MAIN
/*
 * main
 *
 * This is a simple main containing a main loop that will kill off
 * the windows on the display.  This main is used to unit test the
 * code used to perform this operation.
 *
 */

main(int argc, char * argv[])
{

   Display * dpy = XOpenDisplay(NULL);
   XEvent    event;

   Session.ProcessTerminating = 1;
   Session.SessionLeader      = 0;

   if (QueryAndKillWindowTree(dpy))
      for(;;)
      {
         DPRINTF((stderr," kill count = %d\n", Session.WindowKillCount));
      
         XNextEvent(dpy, &event); 

         if (Session.ProcessTerminating)
         {
            HandleWindowDeaths(&event);
         }
      }

} /* end of main */
#else
#endif


/* FLH MORE: move SendProtocolMessage, GetWMState to olwsm */
#include <X11/Xmd.h>
#define XA_WM_PROTOCOLS(d)	XInternAtom((d), "WM_PROTOCOLS", False)
void
SendProtocolMessage(dpy, w, protocol, time)
Display *dpy;
Window w;
Atom protocol;
unsigned long time;
{
	XEvent	sev;

	sev.xclient.type = ClientMessage;
	sev.xclient.display = dpy;
	sev.xclient.window = w;
	sev.xclient.message_type = XA_WM_PROTOCOLS(dpy);
	sev.xclient.format = 32;
	sev.xclient.data.l[0] = (long) protocol;
	sev.xclient.data.l[1] = time;

	XSendEvent(dpy, w, False, NoEventMask, &sev);
}

typedef struct {
	int		state;
	Window		icon;
} WMState;
#define XA_WM_STATE(d)		XInternAtom((d), "WM_STATE", False)

#define NumPropWMStateElements 2
typedef struct {
	CARD32		state;
	Window		icon;
} xPropWMState;

GetWMState(dpy, w)
	Display *dpy;
	Window w;
{
	WMState		wms;
	Atom		atr,
			wm_state;
	int		afr;
	unsigned long	nir,
			bar;
	xPropWMState	*prop;

	wm_state = XA_WM_STATE(dpy);
	if (XGetWindowProperty(dpy, w, wm_state, 0L,
				NumPropWMStateElements, False,
				wm_state, &atr, &afr, &nir, &bar,
				(unsigned char **) (&prop)) != Success)
	{
		return NormalState;		/* Punt */
	}

        if (atr != wm_state || nir < NumPropWMStateElements || afr != 32)
	{
		if (prop != (xPropWMState *) 0)
			free ((char *)prop);

		wms.state =  NormalState;		/* Punt */
	}
	else
	{
		wms.state = prop->state;
		if (prop != (xPropWMState *) 0)
			free((char *)prop);
	}

	return wms.state;
}
