/*		copyright	"%c%" 	*/


#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/main.c	1.34"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <Shell.h>
#include <Gizmos.h>
#include <signal.h>
#include "uucp.h"
#include "../dtamlib/dtamlib.h"
#include "error.h"
#include <Xol/Error.h>

void 	callRegisterHelp(Widget, char *, char *);
extern void	WindowManagerEventHandler();
extern void	initialize();
extern void	_OlSetApplicationTitle();
extern char *	GetUser();
extern char *	GetNodeName();

char *		ApplicationName;
char *		Program;
char *		system_path = "/etc/uucp/Systems";
static Widget	TopLevel;
static OlDtHelpInfo help_info[]= {NULL, NULL, HELP_FILE, NULL, NULL};

Arg arg[50];

Boolean
IsSystemFile(w)
Widget w;
{
	Widget		shell;

	for (shell = w; 
	     shell != NULL && strcmp(XtName(shell), Program); 
	     shell = XtParent(shell)) {
	}

	if (sf->toplevel == shell || sf->propPopup == shell) {
		return(True);
	}

	return (False);
} /* IsSystemFile */


void
CreateSystemFile(filename)
char *filename;
{
	char            login[UNAMESIZE];
	char            nodename[UNAMESIZE];

	XtSetArg(arg[0], XtNwmProtocolInterested, (XtArgVal) OL_WM_DELETE_WINDOW);
	XtSetArg(arg[1], XtNtitle, (XtArgVal) ApplicationName);
	sf->toplevel = XtAppCreateShell(
		Program,
		ApplicationName,
		topLevelShellWidgetClass,
		XtDisplay(TopLevel),
		arg, 2
	);

	OlAddCallback (
		sf->toplevel,
		XtNwmProtocol, WindowManagerEventHandler,
		(XtPointer) 0
	);
	/* tfadmin */
	sf->readAllow = _DtamIsOwner(OWN_DIAL_USE);
	sf->update = _DtamIsOwner(OWN_DIALUP);

#ifdef priv
	fprintf(stderr,"OWN_DIAL_USE = %d, OWN_DIALUP = %d\n",
			sf->readAllow, sf->update);
#endif
	sf->flatItems = (FlatList *)0;
	sf->filename = NULL;
		sf->category = NULL;
        sf->propPopup = NULL;
        sf->devicePopup = NULL;
        sf->findPopup = NULL;
        sf->footer = NULL;
        sf->sprop_footer = NULL;
        sf->dprop_footer = NULL;
        sf->dfooter = NULL;
        sf->quitNotice = NULL;
	sf->cancelNotice = NULL;
	sf->numFlatItems = 0;
	sf->numAllocated = 0;
	if (GetUser(login) == (char *)NULL)
                NotifyUser(sf->toplevel, GGT(string_noLogin));
	sf->userName = strdup (login);
	sf->userHome = getenv("HOME");
	if (!sf->userHome)
		sf->userHome = "";
	if (GetNodeName(nodename) == (char *) NULL)
		NotifyUser(sf->toplevel, GGT(string_noLogin));
	sf->nodeName = strdup (nodename);

	if (filename != NULL) {
		sf->filename = strdup(filename);
	}
	
	sf->lp = (LinePtr)0;
	new = (FlatList *) NULL;

	initialize ();
	XtPopup(sf->toplevel, XtGrabNone);

} /* CreateSystemFile */

static void
Usage()
{
} /* Usage */

#define	TYPE			"RAISE_MYSELF"
#define	APP_ID			"DIALUPMGR"
#define XA_RAISE_MYSELF(d)	XInternAtom(d, TYPE, False)
#define XA_APP_ID(d)		XInternAtom(d, APP_ID, False)

/* Owner... */
static Boolean
CvtSelectionProc(
Widget		w,
Atom *		selection,
Atom *		target,
Atom *		type_rtn,
XtPointer *	val_rtn,
unsigned long *	length_rtn,
int *		format_rtn)
{
#ifdef debug
	(void)fprintf(stderr,"at %d in %s\n", __LINE__, __FILE__);
	(void)fprintf(stderr, "Enter CvtSelectionProc \n");
	(void)fprintf(stderr, "w = 0x%x, selection = 0x%x, target = 0x%x\n",
				w, *selection, *target);
#endif
	if (*target == XA_RAISE_MYSELF(XtDisplay(w)) &&
	    *selection == XA_APP_ID(XtDisplay(w)))
	{
		/* map and raise the systems base window 		*/
		/* do we need to raise the device base window? how?	*/
		XMapRaised(XtDisplay(TopLevel), XtWindow(sf->toplevel));
		*format_rtn = 8;
		*length_rtn = 0;
		*val_rtn = 0;
		*type_rtn = *target;
		return(True); /* False */
	}
} /* end of CvtSelectionProc */

/* Requestor... */
static void
RaiseSelectionCB(
Widget		w,
XtPointer	client_data,
Atom *		selection,
Atom *		type,
XtPointer	value,
unsigned long * length,
int *		format)
{
#ifdef debug
	(void)fprintf(stderr,"at %d in %s\n", __LINE__, __FILE__);
	(void)fprintf(stderr,"Enter RaiseSelectionCB \n");
	(void)fprintf(stderr,"w = 0x%x, selection = 0x%x, type = 0x%x\n",
				w, *selection, *type);
#endif
	/* conversion fail message..., then do recovery...   */
	/* in the future, we may need to run the application */
	if (*type == XT_CONVERT_FAIL) {
		exit(0);
	} else {
		if (*type == XA_RAISE_MYSELF(XtDisplay(w)) &&
		    *selection == XA_APP_ID(XtDisplay(w))) {
			exit(0);
		}
		exit(0);
	}
} /* end of RaiseSelectionCB */
	
void
main(argc, argv)
int argc;
char *argv[];
{
	XtAppContext	app_con;
	Window		another_window;

	char	*p, *q;

#ifdef MEMUTIL
	InitializeMemutil();
#endif

	/* Get rid of embedded slashes */
	/* Since XtName uses basename  */
	q = p = argv[0];
	while ((p = strstr(p, "/")) != NULL)
		q = ++p;
	Program = q;
	

	OlToolkitInitialize(&argc, argv, NULL);

	TopLevel = XtAppInitialize(
			&app_con,		/* app_context_return	*/
			Program,		/* application_class	*/
			(XrmOptionDescList)NULL,/* options		*/
			(Cardinal)0,		/* num_options		*/
			&argc,			/* argc_in_out		*/
			argv,			/* argv_in_out		*/
			(String *)NULL,		/* fallback_resources	*/
			(ArgList)NULL,		/* args			*/
			(Cardinal)0		/* num_args		*/
	);
		
	ApplicationName = GGT (string_appName);
	if (argc > 2) Usage();

	XtVaSetValues (
		TopLevel,
		XtNtitle,			(XtArgVal) ApplicationName,
		XtNmappedWhenManaged,           (XtArgVal) False,
		XtNwidth,			(XtArgVal) 1,
		XtNheight,			(XtArgVal) 1,
		0
	);

	XtRealizeWidget(TopLevel);
	another_window = DtSetAppId (
				XtDisplay(TopLevel),
				XtWindow (TopLevel),
				APP_ID);
	if (another_window != None) {

#ifdef debug
		(void)fprintf(stderr,"at %d in %s\n", __LINE__, __FILE__);
		(void)fprintf(stderr,"Initiate a selection resuest \n");
		(void)fprintf(stderr,"w = 0x%x, selection = 0x%x, type = 0x%x\n",
				TopLevel,
				XA_APP_ID(XtDisplay(TopLevel)),
				XA_RAISE_MYSELF(XtDisplay(TopLevel)));
#endif
		XtGetSelectionValue(
			TopLevel,
			XA_APP_ID(XtDisplay(TopLevel)),
			XA_RAISE_MYSELF(XtDisplay(TopLevel)) ,
			RaiseSelectionCB, (XtPointer)NULL, 0
		);

		/* bug? if one wants to do recover, how can we break? */
		XtAppMainLoop(app_con);
	} else {
#ifdef debug
		(void)fprintf(stderr,"claim a selection owner \n");
		(void)fprintf(stderr,"w = 0x%x, selection = 0x%x\n",
				TopLevel,
				XA_APP_ID(XtDisplay(TopLevel)));
#endif
	XtOwnSelection(TopLevel, XA_APP_ID(XtDisplay(TopLevel)), CurrentTime,
			CvtSelectionProc, NULL, NULL);
	}

	DtInitialize (TopLevel);

	df = (DeviceFile *)XtMalloc (sizeof(DeviceFile));
	memset (df, 0, sizeof (DeviceFile));
	sf = (SystemFile *)XtMalloc (sizeof(SystemFile));
	memset (sf, 0, sizeof (SystemFile));


	/* ignore death of child */
	signal(SIGCLD, SIG_IGN) ;
		
	if (argc == 2)
		CreateSystemFile (argv[1]);
	else
		CreateSystemFile (system_path);

	callRegisterHelp(sf->toplevel, title_setup, help_setup);
	XtAppMainLoop(app_con);
} /* main */


void
callRegisterHelp(widget, title, section)
Widget widget;
char *title;
char *section;
{

  char * ptr;
  help_info->filename =  HELP_FILE;
  help_info->app_title    =  GetGizmoText(title);
  help_info->section = GetGizmoText(section);
	

  OlRegisterHelp(OL_WIDGET_HELP,widget, "DialMgr", OL_DESKTOP_SOURCE,
   (XtPointer)&help_info);
}

