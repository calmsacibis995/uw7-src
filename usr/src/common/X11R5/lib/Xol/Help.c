#ifndef	NOIDENT
#ident	"@(#)olhelp:Help.c	1.82"
#endif

/*
 *************************************************************************
 *
 * Description:
 *		This file contains the source code for the Help Widget
 *
 ******************************file*header********************************
 */

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/ShellP.h>

#include <Xol/OpenLookP.h>
#include <Xol/HelpP.h>
#include <Xol/Mag.h>
#include <Xol/ScrolledWi.h>
#include <Xol/buffutil.h>
#include <Xol/textbuff.h>
#include <Xol/TextEdit.h>
#include <Xol/Flat.h>
#include <Xol/EventObj.h>

#define ClassName Help
#include <Xol/NameDefs.h>

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures 
 *
 **************************forward*declarations***************************
 */
						/* private procedures	*/

static Widget	CreateHelpTree();	/* Creates Help tree if needed	*/
static void	GetSubItemHelp();	/* gets help on a sub-item	*/
static int	HandleText();		/* Handles help message		*/
static void	LookupItemEntry();	/* looks up a hash table entry	*/
static void	LookupSubItemEntry();	/* looks up a sub-object entry	*/
static void	RegisterSubItem();	/* sub-objects and gadgets	*/

						/* class procedures	*/

static void	InitializeHook();	/* Creates sub components	*/
static void	Destroy();		/* Destroy this widget		*/
static void	WMMsgHandler();		/* wm protocol handler		*/
static void	DestroyShell();		/* popdown-help-when-shell-destroy */

						/* action procedures	*/

static void     _OlUnregisterHelp();    /* unregisters help		*/

						/* public procedures	*/

void		OlRegisterHelp OL_ARGS((
			OlDefine, XtPointer, String, OlDefine, XtPointer));
void		_OlPopdownHelpTree();	/* Pops help tree down		*/
void		_OlProcessHelpKey OL_ARGS((Widget, XEvent *));
static void	GetDesktopHelp OL_ARGS((Widget w, Window app_win, char *tag,
			int source_type, char *source, int x, int y));

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define HALF_DIM(id, d) (int)(id->core.d/(Dimension)2)

#define N_ATOM_HASH_ENT  127

#define HASH(Q) ((unsigned long)Q % N_ATOM_HASH_ENT)

typedef struct _HelpSubItem {
	XtPointer		id;		/* Gadget id or item_index*/
	String			tag;
	OlDefine      		source_type;
	XtPointer 		source;
	struct _HelpSubItem *	next;
} HelpSubItem, *HelpSubItemPtr;

typedef struct _HelpItem {
	OlDefine     		id_type;
	XtPointer		id;
	String			tag;
	OlDefine      		source_type;
	XtPointer 		source;
	HelpSubItem *		sub_items;
	struct _HelpItem *	next;
} HelpItem, *HelpItemPtr;

static HelpItemPtr	HashTbl[N_ATOM_HASH_ENT];
static HelpWidget	help_widget	= (HelpWidget) NULL;
static Widget		previous_shell	= (Widget) NULL;

#define HorizontalPoints(W,P) \
	OlScreenPointToPixel(OL_HORIZONTAL,(P),XtScreenOfObject(W))
#define VerticalPoints(W,P) \
	OlScreenPointToPixel(OL_VERTICAL,(P),XtScreenOfObject(W))

#define XA_OL_HELP_QUEUE(dpy)	XInternAtom(dpy, "_HELP_QUEUE", False)

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource
resources[] =
{
#define offset(FIELD) XtOffsetOf(HelpRec, FIELD)

    {
	XtNallowRootHelp, XtCAllowRootHelp, XtRBoolean, sizeof(Boolean),
	offset(help.allow_root_help), XtRImmediate, (XtPointer) False
    },
    {
	XtNorientation, XtCOrientation,
	XtROlDefine, sizeof(OlDefine), offset(rubber_tile.orientation),
	XtRString, (XtPointer)"horizontal"
    }

#undef	offset
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

HelpClassRec
helpClassRec = {
  {
	(WidgetClass) &rubberTileClassRec,	/* superclass		*/
	"Help",					/* class_name		*/
	sizeof(HelpRec),			/* widget_size		*/
	NULL,					/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	NULL,					/* initialize		*/
	InitializeHook,				/* initialize_hook	*/
	XtInheritRealize,			/* realize		*/
	NULL,					/* actions		*/
	NULL,					/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	TRUE,					/* compress_motion	*/
	TRUE,					/* compress_exposure	*/
	TRUE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	Destroy,				/* destroy		*/
	XtInheritResize,			/* resize		*/
	NULL,					/* expose		*/
	NULL,					/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	NULL,					/* get_values_hook	*/
	NULL,					/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_private	*/
	NULL,					/* tm_table		*/
	XtInheritQueryGeometry,			/* query_geometry	*/
	NULL,					/* display_accelerator	*/
	NULL					/* extension		*/
  },	/* End of CoreClass field initializations */
  {
	XtInheritGeometryManager,    		/* geometry_manager	*/
	XtInheritChangeManaged,			/* change_managed	*/
	XtInheritInsertChild,    		/* insert_child		*/
	XtInheritDeleteChild,    		/* delete_child		*/
	NULL    				/* extension         	*/
  },	/* End of CompositeClass field initializations */
  {
    	NULL,					/* resources		*/
    	0,					/* num_resources	*/
    	sizeof(HelpConstraintRec),		/* constraint_size	*/
    	NULL,					/* initialize		*/
    	NULL,					/* destroy		*/
    	NULL					/* set_values		*/
  },	/* End of ConstraintClass field initializations */
  {
    	NULL,					/* highlight_handler	*/
        True,					/* focus_on_select	*/
        NULL,					/* traversal_handler	*/
	NULL,					/* activate_widget	*/
	NULL,					/* event_procs		*/
	0,					/* num_event_procs	*/
	NULL,					/* register_focus	*/
	OlVersion,				/* version		*/
	NULL					/* extension		*/
  },	/* End of ManagerClass field initializations */
	/*
	 * Panes class:
	 */
	{
/* node_size         (I)*/                       XtInheritNodeSize,
/* node_initialize   (D)*/ (OlPanesNodeProc)     0,
/* node_destroy      (U)*/ (OlPanesNodeProc)     0,
/* state_size        (I)*/                       XtInheritPartitionStateSize,
/* partition_initial (I)*/                       XtInheritPartitionInitialize,
/* partition         (I)*/                       XtInheritPartition,
/* partition_accept  (I)*/                       XtInheritPartitionAccept,
/* partition_destroy (I)*/                       XtInheritPartitionDestroy,
/* steal_geometry    (I)*/                       XtInheritStealGeometry,
/* recover_geometry  (I)*/                       XtInheritRecoverGeometry,
/* pane_geometry     (I)*/                       XtInheritPaneGeometry,
/* configure_pane    (I)*/                       XtInheritConfigurePane,
/* accumulate_size   (I)*/                       XtInheritAccumulateSize,
/* extension            */ (XtPointer)           0
	},
  {
	NULL					/* field not used	*/
  },	/* End of RubberTileClass field initializations */
  {
	NULL,					/* field not used	*/
  },	/* End of HelpClass field initializations */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass
helpWidgetClass = (WidgetClass)&helpClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * CreateHelpTree - this routine creates the help widget tree if one does
 * not already exist.  The function returns the id of the shell 
 * containing the help widget.
 ****************************procedure*header*****************************
 */
static Widget
CreateHelpTree(w, tag, window)
	Widget		w;
	String		tag;
	Window		window;
{
	Display *	display	= XtDisplayOfObject(w);
	Widget		shell;
	char *  	title;
	char *  	name;
	char *		class;

	char 		format[BUFSIZ];  

	(void)	OlGetMessage(	display,
				format,
				BUFSIZ,
				OleNfileHelp,
				OleTmsg8,
				OleCOlToolkitMessage,
				OleMfileHelp_msg8,
				(XrmDatabase) NULL);

	shell = (Widget) (help_widget != (HelpWidget)NULL ?
			_OlGetShellOfWidget((Widget)help_widget) : NULL);

	XtGetApplicationNameAndClass(display, &name, &class);

	title = (char *) XtMalloc(_OlStrlen(name) + strlen(format)
					+ _OlStrlen(tag));

	(void) sprintf(title, format,
		(name != (char *)NULL ? name : ""),
		(tag != (char *)NULL ? tag : ""));

	if (shell != (Widget)NULL)
	{
		XtVaSetValues(
			shell,
			XtNtitle,		(XtArgVal)title,
			XtNwindowGroup,		(XtArgVal)window,
			(String)0
		);
	}
	else
	{
		shell = XtVaAppCreateShell(
			"helpShell",
			class,
			transientShellWidgetClass,
			display,
			XtNtitle,		(XtArgVal)title,
			XtNwindowGroup,		(XtArgVal)window,
			XtNpushpin,		(XtArgVal)OL_IN,
			XtNwinType,		(XtArgVal)OL_WT_HELP,
			XtNwindowHeader,	(XtArgVal)True,
			XtNmenuButton,		(XtArgVal)False,
			(String)0
		);
		OlAddCallback(shell, XtNwmProtocol, WMMsgHandler, NULL);
		XtVaCreateManagedWidget(
			"help",
			helpWidgetClass,
			shell,
			(String)0
		);
	}
	XtFree(title);
	return(shell);
} /* END OF CreateHelpTree() */

/*
 *************************************************************************
 * GetSubItemHelp - this routine gets help for a sub-item based on 
 * x and y coordinates.  Note: this routine does not check the value
 * of "p->sub_items" since the calling routine did so.
 ****************************procedure*header*****************************
 */
static void
GetSubItemHelp(w, p, s, x, y)
	Widget		w;		/* The containing widget	*/
	HelpItemPtr	p;
	HelpSubItemPtr *s;		/* returned subitem		*/
	int		x;
	int		y;
{
	XtPointer id;

	*s = (HelpSubItemPtr)NULL;

	if (XtIsSubclass(w, flatWidgetClass) == True)
	{
		id = (XtPointer)OlFlatGetItemIndex(w, (Position)x, (Position)y);

		if ((Cardinal)id != (Cardinal)OL_NO_ITEM)
		{
			HelpSubItemPtr * n;

			LookupSubItemEntry(p, id, s, &n);
		}
	}
	else
	{
		HelpSubItemPtr * n;

		id = (XtPointer) _OlWidgetToGadget(w, (Position)x,(Position)y);
		LookupSubItemEntry(p, id, s, &n);
	}
} /* END OF GetSubItemHelp() */

/*
 *************************************************************************
 * HandleText - This procedure populates the text widget
 ****************************procedure*header*****************************
 */
static int
HandleText(w, source_type, source)
	Widget w;
	int source_type;
	char *source;
{
	char *source_to_use;
	int return_state;

	if(source_type==(OlDefine)OL_DISK_SOURCE) {

		if(access(source,R_OK)==0) {
			source_to_use = source;
			return_state = 1;
		      }
		else {
			source_to_use = source;
			return_state = 0;
		      }
	}
	else {
		source_to_use = source;
		return_state = 1;
	      }

	if (return_state) {
	  XtVaSetValues (
		w,
	    	XtNsourceType,		(XtArgVal)source_type,
	    	XtNsource,		(XtArgVal)source_to_use,
	    	XtNeditType,		(XtArgVal)OL_TEXT_READ,
	    	XtNdisplayPosition,	(XtArgVal)0,
	    	XtNcursorPosition,	(XtArgVal)0,
	    	XtNselectStart,		(XtArgVal)0,
	    	XtNselectEnd,		(XtArgVal)0,
		(String)0
	  );
	}

	return return_state;
} /* END OF HandleText() */

/*
 *************************************************************************
 * LookupItemEntry - this procedure looks up a hash table entry and returns
 * it.  It also returns the address of the previous 'next' pointer.
 ****************************procedure*header*****************************
 */
static void
LookupItemEntry(id_type, id, p_return, q_return)
	OlDefine	id_type;
	XtPointer	id;
	HelpItemPtr *	p_return;
	HelpItemPtr **	q_return;
{
	HelpItemPtr	p;
	HelpItemPtr *	q;

	q = &HashTbl[HASH(id)];
	for (p = *q; p != NULL; p = p-> next)
	{
		if (id == p-> id && id_type == p-> id_type) 
		{
			break;
		}
		q = &p-> next;
	}

	*p_return = p;
	*q_return = q;
} /* END OF LookupItemEntry() */

/*
 *************************************************************************
 * LookupSubItemEntry - this procedure looks up a sub-object entry and
 * returns a pointer to it.  It also returns the address of the
 * previous 'next' pointer.
 ****************************procedure*header*****************************
 */
static void
LookupSubItemEntry(p, id, s_return, n_return)
	HelpItemPtr		p;
	XtPointer		id;
	HelpSubItemPtr *	s_return;
	HelpSubItemPtr **	n_return;
{
	HelpSubItemPtr		s;
	HelpSubItemPtr *	n;		/* next item pointer	*/

	for (n = &p->sub_items,s = *n; s != NULL; s = s->next)
	{
		if (id == s->id)
		{
			break;
		}
		n = &s->next;
	}

	*s_return = s;
	*n_return = n;
} /* END OF LookupSubItemEntry() */

/*
 *************************************************************************
 * RegisterSubItem - this routine registers sub-item help.  SubItems can
 * be sub-objects of a flat widget or they can be gadgets.
 ****************************procedure*header*****************************
 */
static void
RegisterSubItem(widget, id, tag, source_type, source)
	Widget		widget;		/* The containing widget id	*/
	XtPointer	id;
	String		tag;
	OlDefine	source_type;
	XtPointer	source;
{
	HelpItemPtr		p;
	HelpItemPtr *		q;
	HelpSubItemPtr		s;
	HelpSubItemPtr *	n;
						/* Get the widget node	*/

	LookupItemEntry(OL_WIDGET_HELP, (XtPointer)widget, &p, &q);

		/* If there's no widget node to hang the sub-object
		 * information, create a node.				*/

	if (p == NULL)
	{
		*q = p = XtNew(HelpItem);
		p->id_type	= (OlDefine)OL_WIDGET_HELP;
		p->id		= (XtPointer)widget;
		p->source_type	= (OlDefine)OL_IGNORE;
		p->source	= (XtPointer)NULL;
		p->tag		= (String)NULL;
		p->next		= (HelpItemPtr)NULL;
		p->sub_items	= (HelpSubItemPtr)NULL;
		XtAddCallback(widget, XtNdestroyCallback,
					   _OlUnregisterHelp, NULL);
	}

					/* Look up the sub-object	*/

	LookupSubItemEntry(p, id, &s, &n);

	if (s == NULL)
	{
		s		= XtNew(HelpSubItem);
		s->id		= id;
		s->next		= (HelpSubItemPtr)NULL;
		*n		= s;
	}
	s->source_type	= source_type;
	s->source	= source;
	s->tag		= tag;
} /* END OF RegisterSubItem() */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * InitializeHook - This creates the magnifying glass widget and the text
 * widget used by the help widget.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void 
InitializeHook(w, cargs, num_args)
	Widget		w;		/* The new help widget		*/
	Arg		cargs;		/* creation arg list		*/
	Cardinal *	num_args;	/* Number of creation args	*/
{
	HelpWidget	help = (HelpWidget) w;
	Widget		sw;
	char app_buf[BUFSIZ];
	char etc_buf[BUFSIZ];

	if (help_widget == (HelpWidget) NULL) {
		help_widget = help;
	}
	else {
		char * application = XrmQuarkToString(_OlApplicationName);
		OlVaDisplayWarningMsg(XtDisplay(w),
				      OleNfileHelp,
				      OleTmsg6,
				      OleCOlToolkitWarning,
				      OleMfileHelp_msg6,
				      application);
	}

	if (OlGetGui() == OL_OPENLOOK_GUI)
	  help->help.mag_widget = XtVaCreateManagedWidget(
		  "magnifier",
		  magWidgetClass,
		  w,
		  XtNborderWidth,	(XtArgVal)0,
		  XtNweight,		(XtArgVal)0,
		  (String)0
	  );
	else
	  help->help.mag_widget = (Widget) NULL;

	sw = XtVaCreateManagedWidget(
		"HelpSW",
		scrolledWindowWidgetClass,
		w,
		XtNborderWidth,		(XtArgVal)0,
		XtNweight,		(XtArgVal)1,
		(String)0
	);

	/*
	 * MORE: When the XtVaTypedArg feature is fixed, use it instead
	 * of doing the conversion ourselves?
	 */
	help->help.text_widget = XtVaCreateManagedWidget(
		"Text",
		textEditWidgetClass,
		sw,
		XtNeditType,		(XtArgVal)OL_TEXT_READ,
		XtNwrapMode,		(XtArgVal)OL_WRAP_WHITE_SPACE,
		XtNsource,		(XtArgVal)"",
		XtNwidth,		(XtArgVal)HorizontalPoints(w,390),
		XtNlinesVisible,	(XtArgVal)10,
		XtNleftMargin,		(XtArgVal)HorizontalPoints(w,20),
		XtNrightMargin,		(XtArgVal)HorizontalPoints(w,20),
		XtNtopMargin,		(XtArgVal)VerticalPoints(w,20),
		XtNbottomMargin,	(XtArgVal)VerticalPoints(w,20),
		(String)0
	);
} /* END OF InitializeHook() */

/*
 *************************************************************************
 * Destroy - this procedure destroys the help widget.  It also removes
 * event handlers from the Help's shell
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Destroy(w)
	Widget w;			/* The help Widget	*/
{
			/* Remove the global help pointer	*/

	help_widget = (HelpWidget)NULL;
	if (previous_shell) {
		XtRemoveCallback(previous_shell, XtNdestroyCallback,
				DestroyShell, NULL);
		previous_shell = NULL;
	}

} /* END OF Destroy() */

/* ARGSUSED */
static void
DestroyShell OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)	/* ignored	*/
OLGRA(XtPointer, call_data)
{
	if (previous_shell)
		_OlPopdownHelpTree(previous_shell);
}

/* ARGSUSED */
static void
WMMsgHandler OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)	/* ignored	*/
OLGRA(XtPointer, call_data)
{
	OlWMProtocolVerify *st = (OlWMProtocolVerify *)call_data;

	if (st->msgtype == OL_WM_DELETE_WINDOW) {
		XtPopdown(w);
		if (previous_shell) {
			XtRemoveCallback(previous_shell, XtNdestroyCallback,
				DestroyShell, NULL);
			previous_shell = NULL;
		}
	}
}

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 *************************************************************************
 * OlUnregisterHelp - This procedure deletes data base entries for dying
 * widgets.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
_OlUnregisterHelp(w, client_data, call_data)
	Widget		w;			/* widget to unregister	*/
	XtPointer	client_data;		/* The id_type    	*/
	XtPointer	call_data;		/* NULL          	*/
{
	HelpItemPtr	p;
	HelpItemPtr *	q;
	Boolean		is_gadget = _OlIsGadget(w);

	LookupItemEntry(OL_WIDGET_HELP, (XtPointer)
			(is_gadget == True ? XtParent(w) : w), &p, &q);
	
	if (p == (HelpItemPtr)NULL)
	{
		OlVaDisplayWarningMsg(	XtDisplayOfObject(w),
					OleNfileHelp,
					OleTmsg1,
					OleCOlToolkitWarning,
					OleMfileHelp_msg1);
	}
	else
	{
		HelpSubItemPtr		self;
		HelpSubItemPtr *	next;

		if (is_gadget == False)
		{
			self = p->sub_items;
			next = &self->next;

				/* First free the sub-object help	*/
	
			while (self != NULL)
			{
				next = &self->next;
				XtFree((XtPointer) self);
				self = *next;
			}
	
			*q = p->next;
			XtFree((XtPointer)p);
		}
		else
		{
			LookupSubItemEntry(p, (XtPointer)w, &self, &next);

			if (self != (HelpSubItemPtr) NULL)
			{
				*next = self->next;
				XtFree((XtPointer)self);
			}
			else
			{
				OlVaDisplayWarningMsg(	XtDisplayOfObject(w),
							OleNfileHelp,
							OleTmsg1,
							OleCOlToolkitWarning,
							OleMfileHelp_msg1);
			}
		}
	}
} /* END OF _OlUnregisterHelp */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * OlRegisterHelp - 
 ****************************procedure*header*****************************
 */
#ifdef OlNeedFunctionPrototypes
void
OlRegisterHelp(
    OlDefine	id_type,
    XtPointer	id,
    String	tag,
    OlDefine	source_type,
    XtPointer	source)
#else
void
OlRegisterHelp(id_type, id, tag, source_type, source)
    OlDefine	id_type;
    XtPointer	id;
    String	tag;
    OlDefine	source_type;
    XtPointer	source;
#endif
{
    HelpItemPtr		p;
    HelpItemPtr *	q;
    OlFlatHelpId * 	complex_id;

    switch ((int)id_type)
    {
    case OL_FLAT_HELP:
	complex_id = (OlFlatHelpId *)id;

	if (XtIsSubclass(complex_id->widget, flatWidgetClass) == False)
	{

		OlVaDisplayWarningMsg((complex_id->widget) ?
				      XtDisplayOfObject(complex_id->widget) :
				      (Display *)NULL,
				      OleNfileHelp,
				      OleTmsg2,
				      OleCOlToolkitWarning,
				      OleMfileHelp_msg2);
		return;
	}
	else
	{
		RegisterSubItem(complex_id->widget, (XtPointer)
				complex_id->item_index, tag, source_type,
				source);
	}
	break;
    case OL_CLASS_HELP:					/* Fall Through	*/
    case OL_WINDOW_HELP:				/* Fall Through	*/
    case OL_WIDGET_HELP:
	if (id_type == (OlDefine)OL_WIDGET_HELP &&
	    _OlIsGadget((Widget)id) == True)
	{
		RegisterSubItem(XtParent((Widget)id), id,
				tag, source_type, source);
		break;
	}

							/* Else	.......	*/
	LookupItemEntry(id_type, id, &p, &q);

	if (p == NULL)
	{
		p		= XtNew(HelpItem);
		p->next		= NULL;
		p->sub_items	= NULL;
		*q		= p;

		if (id_type == (OlDefine)OL_WIDGET_HELP)
		{
			XtAddCallback((Widget) id, XtNdestroyCallback,
					   _OlUnregisterHelp, NULL);
		}
	}
	p->id_type	= id_type;
	p->id		= id;
	p->source_type	= source_type;
	p->source	= source;
	p->tag		= tag;
	break;
    default:

	OlVaDisplayWarningMsg(	(id) ? XtDisplayOfObject((Widget) id) :
			        (Display *)NULL,
				OleNfileHelp,
				OleTmsg3,
				OleCOlToolkitWarning,
				OleMfileHelp_msg3);
    }

} /* END OF OlRegisterHelp() */

/*
 *************************************************************************
 * _OlPopupHelpTree - this routine is in charge of popping up the help
 * widget tree
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
void
_OlPopupHelpTree OLARGLIST((w, client_data, xevent, cont_to_dispatch))
	OLARG( Widget,		w)	/* Id of arbitrary widget	*/
	OLARG( XtPointer,	client_data)/* Unused			*/
	OLARG( XEvent *,	xevent)
	OLGRA( Boolean *,	cont_to_dispatch)
{
	HelpItemPtr		p;
	HelpItemPtr *		q;
	HelpSubItemPtr		s = (HelpSubItemPtr)NULL;
	Widget			wid;
	Widget			shell = w;
	Window			win;
	Window 			window;
	int			win_x_return;
	int			win_y_return;
	int			root_x_return;
	int			root_y_return;
	Display *		dpy;
	OlDefine		source_type	= (OlDefine)OL_STRING_SOURCE;
	XtPointer		source;
	String			tag		= (String)NULL;
	void			(* func) ();
	Boolean			repeat;
	char			nf_buf[BUFSIZ];

 	(void) OlGetMessage(	XtDisplayOfObject(w),
				nf_buf,
				BUFSIZ,
				OleNfileHelp,
				OleTmsg5,
				OleCOlToolkitMessage,
				OleMfileHelp_msg5,
				(XrmDatabase)NULL);

	source = (XtPointer) nf_buf;
	
	if (xevent->xany.type != ClientMessage ||
	    xevent->xclient.message_type !=
			XA_OL_HELP_KEY(xevent->xany.display))
	{
		return;
	}

	dpy	= XtDisplayOfObject(w);
	window	= RootWindowOfScreen(XtScreenOfObject(w));

	GetHelpKeyMessage(dpy, xevent, &win, 
		&win_x_return, &win_y_return, &root_x_return, &root_y_return);

			/* Does this window belong to a widget ?	*/

	if ((wid = XtWindowToWidget(dpy, win)) != NULL)
	{
		Widget	self;

		shell = _OlGetShellOfWidget(wid);

		window	= XtWindow(shell);

		for (self = wid; self != NULL; self = XtParent(self))
		{
			LookupItemEntry(OL_WIDGET_HELP, (XtPointer)self,
						&p, &q);

			if (p != NULL)
			{
				if (p->sub_items != NULL)
				{
					GetSubItemHelp(wid, p, &s,
						win_x_return, win_y_return);

						/* If we've found subitem
						 * help or if this widget
						 * has its own help,
						 * break.		*/

					if (s != NULL ||
					    p->source_type !=
							(OlDefine)OL_IGNORE)
					{
						break;
					}
					else
					{
						p = (HelpItemPtr)NULL;
					}
				}
				else
				{
					break;
				}
			}
			else if (self == shell)
			{
				p = NULL;
				break;
			}
		}

		if (p == NULL)
		{
			LookupItemEntry(OL_CLASS_HELP, (XtPointer) XtClass(wid),
					&p, &q);
		}
	}
	else
	{
				/* Look for help on the raw window	*/

		LookupItemEntry(OL_WINDOW_HELP, (XtPointer) win, &p, &q);
	}

	if (s != NULL)
	{
		source_type = s-> source_type;
		source      = s-> source;
		tag         = s-> tag;
	}
	else if (p != NULL)
	{
		source_type = p-> source_type;
		source      = p-> source;
		tag         = p-> tag;
	}

			/*
			 * Register destroy callback so that the help window
			 * goes away with the shell. Otherwise, dangling help
			 * windows could create problems for olwm when trying
			 * to terminate a session.
			 */
	if (previous_shell) {
		XtRemoveCallback(previous_shell, XtNdestroyCallback,
			DestroyShell, NULL);
	}
	previous_shell = shell;
	XtAddCallback(previous_shell, XtNdestroyCallback, DestroyShell, NULL);

	/* Check if Desktop Metaphor's help manager is running.		  */
	/* If the help manager is not running, then it's business as      */
	/* usual; otherwise, re-direct help messages to the help manager. */ 

	if (XGetSelectionOwner(dpy, XA_OL_HELP_QUEUE(dpy)) == None) {

			/* Create the Help widget if it does not exist */

		shell = CreateHelpTree(w, tag, window);

			/* Inform the Magnifier that it should take a  */
			/* snapshot				       */

		if (help_widget->help.mag_widget != NULL)
		  XtVaSetValues(
			  help_widget->help.mag_widget,
			  XtNmouseX,	(XtArgVal)root_x_return,
			  XtNmouseY,	(XtArgVal)root_y_return,
			  (String)0
		  );
			
		do {
			repeat = False;
			switch ((int)source_type) {
			case OL_DISK_SOURCE: 
			  if (!HandleText( help_widget-> help.text_widget, 
					    source_type, source)) {
					source_type =
						(OlDefine)OL_STRING_SOURCE;
					source      = (XtPointer) nf_buf;
					HandleText(
						help_widget-> help.text_widget,
						   source_type, source);
				}
				break;
			case OL_STRING_SOURCE:
				HandleText( help_widget-> help.text_widget, 
					    source_type, source);
				break;
			case OL_INDIRECT_SOURCE:
			case OL_TRANSPARENT_SOURCE:
				func = (void (*)()) source;
				if (func == NULL) {
					source_type = (OlDefine)OL_STRING_SOURCE;
					source      = (XtPointer) nf_buf;
				} else {
					if (source_type ==
					    (OlDefine)OL_TRANSPARENT_SOURCE)
					{
						(*func) (p-> id_type, p-> id, 
						win_x_return, win_y_return);
						return;
					} else {
						(*func) (p-> id_type,p-> id, 
						 win_x_return, win_y_return,
						 &source_type, &source);
						repeat = True;
					}
				}
				break;
			case OL_DESKTOP_SOURCE:
				OlVaDisplayWarningMsg(XtDisplayOfObject(w),
						OleNfileHelp,
						OleTmsg4,
						OleCOlToolkitWarning,
						OleMfileHelp_msg9);
				break;
			default:

			OlVaDisplayWarningMsg(XtDisplayOfObject(w),
						OleNfileHelp,
						OleTmsg4,
						OleCOlToolkitWarning,
						OleMfileHelp_msg4);
			}
		} while (repeat == True);

			/* Now realize the help widget's shell so we can */
			/* set the the decoration hints on it.           */

		if (((ShellWidget) shell)->shell.popped_up == False)
		{
			if (XtIsRealized(shell) == False)
			{
				XSizeHints	hints;
				Window		win;

				shell->core.managed = False;
				XtRealizeWidget(shell);
				shell->core.managed = True;

				/*
				 * MORE: Improve this hack. For now we limit
				 * the size of the window by guessing from the
				 * spec. We can do better, by e.g. querying the
				 * children for the smallest preferred size.
				 *
				 * MORE: Put this in a Realize procedure.
				 *
				 * Look at page 483 of the spec for the min :
				 * width: min_width	= -b + c + d + 60 + d
				 * 	min_width	= -b + c + d + 60 + d
				 *
				 * Look at page 481-483 of the spec for the min
				 * height:
				 *	min_height	= a(483) + a(482)
				 *
				 * These values provide enough room for the
				 * magnifying glass (assuming the Mag widget is
				 * properly sized, which it currently isn't),
				 * plus a few characters of text within the
				 * prescribed margins.
				 */
				win = XtWindowOfObject(shell);
				if (!XGetNormalHints(dpy, win, &hints))
					hints.flags = 0;
				hints.flags      |= PMinSize;
				hints.min_width  = HorizontalPoints(shell,206);
				hints.min_height = VerticalPoints(shell,110);
				XSetNormalHints (dpy, win, &hints);
			}

			XtVaSetValues(
				shell,
				XtNpushpin,	(XtArgVal)OL_IN,
				(String)0
			);
			XtPopup(shell, XtGrabNone);
			/*
			 * If a modal cascade is active, we need to make
			 * the help window modal so that it can get
			 * events. We can't use XtGrabNonexclusive in
			 * the XtPopup, because Xt would automatically
			 * do an XtRemoveGrab when the help window is
			 * popped down. But that could happen after
			 * the modal cascade (or part of it) is popped
			 * down, and that would cause an Xt complaint.
			 * (When the modal cascade is popped down,
			 * XtRemoveGrab is called to dismantle the
			 * cascade--but that will remove the Help window,
			 * too, since XtRemoveGrab operates on all widgets
			 * up to and including the one being removed.
			 * The subsequent popdown of the help window would
			 * cause a redundant XtRemoveGrab, thus the
			 * complaint.)
			 */
			if (XtModalCascadeActive(shell))
				XtAddGrab (shell, False, False);
		}
		else
		{
			XtVaSetValues(
				shell,
				XtNtransientFor, (XtArgVal)previous_shell,
				(String)0
			);
			XRaiseWindow(XtDisplay(shell), XtWindow(shell));
		}
	} else {
		do {
			repeat = False;

			/* re-route help request to help manager */  

			switch((int)source_type) {

			case OL_DESKTOP_SOURCE:
			case OL_DISK_SOURCE:
			case OL_STRING_SOURCE:
				GetDesktopHelp(w, window, tag, source_type,
					source, root_x_return, root_y_return);
				break;

			case OL_INDIRECT_SOURCE:
			case OL_TRANSPARENT_SOURCE:
				func = (void (*)()) source;
				if (func == NULL) {
					source_type =
						(OlDefine)OL_STRING_SOURCE;
					source      = (XtPointer) nf_buf;
				} else {
					if (source_type ==
					    (OlDefine)OL_TRANSPARENT_SOURCE)
					{
						(*func) (p-> id_type, p-> id, 
						win_x_return, win_y_return);
						return;
					} else {
						(*func) (p-> id_type,p-> id, 
						 win_x_return, win_y_return,
						 &source_type, &source);
						repeat = True;
					}
				}
				break;

			default:
				GetDesktopHelp(w, window, tag, source_type,
					NULL, root_x_return, root_y_return);
			}

		} while (repeat == True);
	}

} /* END OF _OlPopupHelpTree() */

/*
 * Pops down help tree, if it is mapped.
 */
/* ARGSUSED */
void		
_OlPopdownHelpTree OLARGLIST((w))
	OLGRA(Widget, w)
{
	Widget shell;

	if ((help_widget) &&
	    (shell = _OlGetShellOfWidget((Widget)help_widget)) &&
	    (((ShellWidget) shell)->shell.popped_up == True))
		XtPopdown(shell);

	if (previous_shell) {
		XtRemoveCallback(previous_shell, XtNdestroyCallback,
				DestroyShell, NULL);
		previous_shell = NULL;
	}
} /* END OF _OlPopdownHelpTree() */


/*
 *************************************************************************
 * _OlProcessHelpKey - takes a keypress and pops up the help window
 * using the information from the keypress.
 *
 * Note: this routine's implementation for Pointer-based help depends on
 * the window manager's implementation for Pointer-based help since
 * the OPEN LOOK ICCCM extensions don't specify the contents of the
 * help ClientMessage -- yuck!!!
 ****************************procedure*header*****************************
 */
void
_OlProcessHelpKey OLARGLIST((w, xevent))
	OLARG( Widget,		w)
	OLGRA( XEvent *,	xevent)
{
	Arg			args[1];
	OlDefine		help_model = (OlDefine)OL_POINTER;
	XClientMessageEvent	xcm;
	int			root_x;
	int			root_y;
	int			x;
	int			y;
	Window			help_window;
	Boolean			ignore;

	if (w == (Widget)NULL ||
	    xevent == (XEvent *)NULL ||
	    xevent->type != KeyPress)
	{
		return;
	}

	XtSetArg(args[0], XtNhelpModel, &help_model);
	OlGetApplicationValues(w, args, 1);

	root_x		= xevent->xkey.x_root;
	root_y		= xevent->xkey.y_root;
	x		= root_x;
	y		= root_y;
	help_window	= xevent->xkey.root;

	if (help_model == (OlDefine)OL_POINTER)
	{
		Window		tmp_window;
		Window		child_window = xevent->xkey.root;


			/* find leaf-most window of RootWindow under
			 * the pointer.					*/

		do {
			tmp_window	= help_window;
			help_window	= child_window;
			if (!XTranslateCoordinates(xevent->xkey.display,
					tmp_window, help_window,
					x, y, &x, &y, &child_window))
			{
				return;
			}
		} while (child_window != (Window)None);

			/* If no widget is associated with this window,
			 * return since the pointer is probably over
			 * another client and we won't deal with
			 * them now.					*/

		if (XtWindowToWidget(xevent->xkey.display, help_window)
			== (Widget)NULL)
		{
			return;
		}

		if (help_window == xevent->xkey.root)
		{
			/* Force creation of the Help Tree so we can
			 * check to see if this application allows help
			 * for the RootWindow.				*/

			(void) CreateHelpTree(w, (XtPointer)NULL, help_window);

			if (help_widget != (HelpWidget)NULL &&
			    help_widget->help.allow_root_help == False)
			{
				return;
			}
			x = xevent->xkey.x_root;
			y = xevent->xkey.y_root;
		}
	}
	else	/* focus-based */
	{
		Window	ignore_child;
		Widget	fw;

		if ((fw = OlGetCurrentFocusWidget(w)) == (Widget)NULL &&
		    (fw = XtWindowToWidget(xevent->xkey.display,help_window))
			== (Widget)NULL)
		{
			fw = w;
		}

		if (_OlIsGadget(fw) == True)
		{
			help_window = XtWindowOfObject(fw);
			x = (int)(fw->core.x + HALF_DIM(fw, width));
			y = (int)(fw->core.y + HALF_DIM(fw, height));
		}
		else if (XtIsSubclass(w, flatWidgetClass) != True)
		{
			help_window = XtWindowOfObject(fw);
			x = HALF_DIM(fw, width);
			y = HALF_DIM(fw, height);
		}
		else		/* flat widget	*/
		{
			Cardinal	i = OlFlatGetFocusItem(fw);

			help_window = XtWindow(fw);

			if (i != (Cardinal)OL_NO_ITEM)
			{
				Position	xp;
				Position	yp;
				Dimension	width;
				Dimension	height;

				OlFlatGetItemGeometry(fw, i, &xp, &yp,
							&width, &height);

				x = (int)(xp + width/(Dimension)2);
				y = (int)(yp + height/(Dimension)2);
			}
			else
			{
				x = HALF_DIM(fw, width);
				y = HALF_DIM(fw, height);
			}
		}


		XTranslateCoordinates(xevent->xkey.display,
				XtWindowOfObject(fw),
				RootWindowOfScreen(XtScreenOfObject(fw)),
				x, y, &root_x, &root_y, &ignore_child);
	}

	/*
	 * Forge a client-message for help.  The fields are
	 * undocumented, window manager implementation dependent --
	 * YUCK!!!
	 */

	xcm.type		= ClientMessage;
	xcm.serial		= xevent->xkey.serial;
	xcm.send_event		= xevent->xkey.send_event;
	xcm.display		= xevent->xkey.display;
	xcm.window		= xevent->xkey.window;
	xcm.message_type	= XA_OL_HELP_KEY(xevent->xkey.display);
	xcm.format		= 32;
	xcm.data.l[0]		= help_window;
	xcm.data.l[1]		= x;
	xcm.data.l[2]		= y;
	xcm.data.l[3]		= root_x;
	xcm.data.l[4]		= root_y;

	_OlPopupHelpTree(w, NULL, (XEvent *)&xcm, &ignore);
} /* END OF _OlProcessHelpKey() */

/*
 *************************************************************************
 * This routine sends a DT_OL_DISPLAY_HELP request to the Desktop Metaphor's
 * help manager.  It is called from _OlPopupHelpTree if the help manager
 * is running.
 ****************************procedure*header*****************************
 */

static void
GetDesktopHelp OLARGLIST((w, app_win, tag, source_type, source, x, y))
	OLARG( Widget,	w)
	OLARG( Window,	app_win)
	OLARG( char *,	tag)
	OLARG( int,	source_type)
	OLARG( char *,	source)
	OLARG( int,	x)
	OLGRA( int,	y)
{
	int     size;
	char    *buf;
	char    *name;
	char    *class;
	Display *dpy;
	
	dpy = XtDisplay(w);
	XtGetApplicationNameAndClass(dpy, &name, &class);
	
	if (source_type == OL_DISK_SOURCE) {

		/* create request string */

		if (tag) {

			size = strlen(name) + strlen(tag) + strlen(source) + 200;
			buf = (char *)malloc(size * sizeof(char));

			sprintf(buf,
"@OL_DISPLAY_HELP: SERIAL=-2 VERSION=0\
 CLIENT=%lu HELPTYPE=%d APPNAME=\"%s\"\
 TITLE=\"%s\" FILENAME=\"%s\" XPOS=%d YPOS=%d",
				app_win, source_type, name, tag, source, x, y); 

		} else {

			size = strlen(name) + strlen(source) + 200;
			buf = (char *)malloc(size * sizeof(char));

			sprintf(buf,
"@OL_DISPLAY_HELP: SERIAL=-2 VERSION=0\
 CLIENT=%lu HELPTYPE=%d APPNAME=\"%s\"\
 FILENAME=\"%s\" XPOS=%d YPOS=%d",
				app_win, source_type, name, source, x, y); 
		}

	} else if (source_type == OL_STRING_SOURCE) {
		if (tag) {

			size = strlen(name) + strlen(tag) + strlen(source) +200;
			buf = (char *)malloc(size * sizeof(char));

			sprintf(buf,
"@OL_DISPLAY_HELP: SERIAL=-2 VERSION=0\
 CLIENT=%lu HELPTYPE=%d APPNAME=\"%s\"\
 TITLE=\"%s\" STRING=\"%s\" XPOS=%d YPOS=%d",
			 	app_win, source_type, name, tag, source, x, y); 

		} else {

			size = strlen(name) + strlen(source) + 200;
			buf = (char *)malloc(size * sizeof(char));

			sprintf(buf,
"@OL_DISPLAY_HELP: SERIAL=-2 VERSION=0\
 CLIENT=%lu HELPTYPE=%d APPNAME=\"%s\"\
 STRING=\"%s\" XPOS=%d YPOS=%d",
			 	app_win, source_type, name, source, x, y); 
		}

	} else { /* source_type = OL_DESKTOP_SOURCE */

		/*
		 * The only required information is the help file name. If
		 * app_title is not specified, it is set to app_name.
		 */

		char *file;
		char *path;
		char *sect;
		char *app_title;

		OlDtHelpInfo *s = (OlDtHelpInfo *)source;

		/* file name must be supplied */
		if (s->filename == NULL) {
			OlVaDisplayWarningMsg(XtDisplayOfObject(w),
				OleNfileHelp,
				OleTmsg4,
				OleCOlToolkitWarning,
				OleMfileHelp_msg10);
			return;
		}

		file = s->filename;
		path = s->path;
		sect = s->section;
		if (s->app_title == NULL || strcmp(s->app_title, "") == 0)
			app_title = name;
		else
			app_title = s->app_title;

		/*
		 * should s->title always override tag or used only when
		 * tag is not specified?
		 */
		if (!tag && s->title != NULL && strcmp(s->title, ""))
			tag = s->title;

		/* Build request string. */
		if (tag) {
			if (path && file && sect) {

				size = strlen(name) + strlen(tag) + strlen(file)
						+ strlen(path) + strlen(sect) + 250;
				buf = (char *)malloc(size * sizeof(char));

				sprintf(buf,
"@OL_DISPLAY_HELP: SERIAL=-2 VERSION=0\
 CLIENT=%lu HELPTYPE=%d APPNAME=\"%s\"\
 APPTITLE=\"%s\" TITLE=\"%s\" FILENAME=\"%s\"\
 HELPDIR=\"%s\" SECTTAG=\"%s\" XPOS=%d YPOS=%d",
					app_win, source_type, name, app_title,
					tag, file, path, sect, x, y);

			} else if (path && file) {

				size = strlen(name) + strlen(tag) + strlen(file)
						+ strlen(path) + 250;
				buf = (char *)malloc(size * sizeof(char));

				sprintf(buf,
"@OL_DISPLAY_HELP: SERIAL=-2 VERSION=0\
 CLIENT=%lu HELPTYPE=%d APPNAME=\"%s\"\
 APPTITLE=\"%s\" TITLE=\"%s\" FILENAME=\"%s\"\
 HELPDIR=\"%s\" XPOS=%d YPOS=%d",
					app_win, source_type, name,
					app_title, tag, file, path, x, y);

			} else if (file && sect) {

				size = strlen(name) + strlen(tag) + strlen(file)
						+ strlen(sect) + 250;
				buf = (char *)malloc(size * sizeof(char));

				sprintf(buf,
"@OL_DISPLAY_HELP: SERIAL=-2 VERSION=0\
 CLIENT=%lu HELPTYPE=%d APPNAME=\"%s\"\
 APPTITLE=\"%s\" TITLE=\"%s\" FILENAME=\"%s\"\
 SECTTAG=\"%s\" XPOS=%d YPOS=%d",
					app_win, source_type, name,
					app_title, tag, file, sect, x, y);

			} else if (file) {

				size = strlen(name) + strlen(tag) + strlen(file) + 250;
				buf = (char *)malloc(size * sizeof(char));

				sprintf(buf,
"@OL_DISPLAY_HELP: SERIAL=-2 VERSION=0\
 CLIENT=%lu HELPTYPE=%d APPNAME=\"%s\"\
 APPTITLE=\"%s\" TITLE=\"%s\" FILENAME=\"%s\" XPOS=%d YPOS=%d",
					app_win, source_type, name,
					app_title, tag, file, x, y);
			} else {
				/* should never get here since file
				 * must be specified
				 */
			}

		} else { /* no title */
			if (path && file && sect) {

				size = strlen(name) + strlen(file) + strlen(path)
						+ strlen(sect) + 250;
				buf = (char *)malloc(size * sizeof(char));

				sprintf(buf,
"@OL_DISPLAY_HELP: SERIAL=-2 VERSION=0\
 CLIENT=%lu HELPTYPE=%d APPNAME=\"%s\"\
 APPTITLE=\"%s\" FILENAME=\"%s\" HELPDIR=\"%s\"\
 SECTTAG=\"%s\" XPOS=%d YPOS=%d",
					app_win, source_type, name,
					app_title, file, path, sect, x, y);

			} else if (path && file) {

				size = strlen(name) + strlen(file) +
						strlen(path) + 250;
				buf = (char *)malloc(size * sizeof(char));

				sprintf(buf,
"@OL_DISPLAY_HELP: SERIAL=-2 VERSION=0\
 CLIENT=%lu HELPTYPE=%d APPNAME=\"%s\"\
 APPTITLE=\"%s\" FILENAME=\"%s\" HELPDIR=\"%s\"\ XPOS=%d YPOS=%d",
					app_win, source_type, name,
					app_title, file, path, x, y);

			} else if (file && sect) {

				size = strlen(name) + strlen(file) +
						strlen(sect) + 250;
				buf = (char *)malloc(size * sizeof(char));

				sprintf(buf,
"@OL_DISPLAY_HELP: SERIAL=-2 VERSION=0\
 CLIENT=%lu HELPTYPE=%d APPNAME=\"%s\" APPTITLE=\"%s\"\
 FILENAME=\"%s\" SECTTAG=\"%s\" XPOS=%d YPOS=%d",
					app_win, source_type, name,
					app_title, file, sect, x, y);

			} else if (file) {

				size = strlen(name) + strlen(file) + 250;
				buf = (char *)malloc(size * sizeof(char));

				sprintf(buf,
"@OL_DISPLAY_HELP: SERIAL=-2 VERSION=0\
 CLIENT=%lu HELPTYPE=%d APPNAME=\"%s\"\
 APPTITLE=\"%s\" FILENAME=\"%s\" XPOS=%d YPOS=%d",
					app_win, source_type, name,
					app_title, file, x, y);
			} else {
				/* should never get here since
				 * file must be specified
				 */
			}
		}
	}

	XChangeProperty(
		dpy, app_win, XA_OL_HELP_QUEUE(dpy), XA_STRING, 8,
		PropModeAppend, (unsigned char *)buf, strlen(buf));

	XConvertSelection(
		dpy, XA_OL_HELP_QUEUE(dpy), XA_OL_HELP_QUEUE(dpy),
		XA_OL_HELP_QUEUE(dpy), app_win, CurrentTime);

	free(buf);

} /* END OF GetDesktopHelp() */
