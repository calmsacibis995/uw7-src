/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#include <Xol/OlMinStr.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)scrollinglist:ListPane.c	1.160"
#endif

/*******************************file*header*******************************
 * Date:	Oct-88
 * File:	ListPane.c
 *
 * Description:
 *	ListPane.c - OPEN LOOK(TM) ListPane Widget
 */

#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/ListPaneP.h>
#include <Xol/ScrollingP.h>
#include <Xol/Scrollbar.h>
#include <Xol/TextFieldP.h>

#define ClassName ListPane
#include <Xol/NameDefs.h>

/**************************forward*declarations***************************
 *
 * Forward function definitions listed by category:
 *		1. Private functions
 *		2. Class   functions
 *		3. Action  functions
 *		4. Public  functions
 *
 */
						/* private procedures */
static OlListToken ApplAddItem OL_ARGS((
			Widget, OlListToken, OlListToken, OlListItem));
static void	ApplDeleteItem OL_ARGS((Widget, OlListToken));
static void	ApplEditClose OL_ARGS((Widget));
static void	ApplEditOpen OL_ARGS((Widget, Boolean, OlListToken));
static void	ApplTouchItem OL_ARGS((Widget, OlListToken));
static void	ApplUpdateView OL_ARGS((Widget, Boolean));
static void	ApplViewItem OL_ARGS((Widget, OlListToken));
static void	BuildClipboard OL_ARGS((ListPaneWidget));
static void	BuildDeleteList OL_ARGS((ListPaneWidget, OlListDelete *));
static void	ClearSelection OL_ARGS((ListPaneWidget));
static void	CreateGCs OL_ARGS((ListPaneWidget));
static void	CutOrCopy OL_ARGS((ListPaneWidget, Boolean));
static void	DeleteNode OL_ARGS((ListPaneWidget, int));
static void	DrawItem OL_ARGS((ListPaneWidget, int));
static void	FreeList OL_ARGS((ListPaneWidget));
static int	InsertNode OL_ARGS((ListPaneWidget, int));
static OlgTextLbl * Item2TextLbl OL_ARGS((ListPaneWidget, OlListItem *, GC,GC));
static void	MaintainWidth OL_ARGS((ListPaneWidget, OlListItem *));
static void	ScrollView OL_ARGS((ListPaneWidget));
static void	Search OL_ARGS((ListPaneWidget, char));
static void	SelectOrAdjustItem OL_ARGS((
			ListPaneWidget, int, OlVirtualName, Time));
static void	ToggleItemState OL_ARGS((ListPaneWidget, int));
static void	UpdateSBar OL_ARGS((ListPaneWidget));

						/* class procedures */
static Boolean	AcceptFocus OL_ARGS((Widget, Time *));
static Boolean	Activate OL_ARGS((Widget, OlVirtualName, XtPointer));
static void	Destroy();
static void	HighlightHandler OL_ARGS((Widget, OlDefine));
static void	Initialize();
static void	Redisplay();
static Widget	RegisterFocus OL_ARGS((Widget));
static Widget	TraversalHandler OL_ARGS((Widget, Widget, OlVirtualName, Time));
static Boolean	SetValues();
						/* action procedures */
static void	AutoScrollCB();
static void	ButtonDown();
static void	ButtonMotion();
static void	ButtonUp();
static Boolean	ConvertSelection();
static void	KeyDown();
static void	LoseSelection();
static void	SelectionDone();
static void	SBarMovedCB();
						/* public procedures */

/*****************************file*variables******************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 */

/* Since there is only one selection at any one time, it is satisfactory
   to have a single pointer for the contents defined here local to this
   file (a single "array" for all list widgets in the process space).
 */
static String	Clipboard;
static Cardinal	ClipboardCnt;		/* byte size */

/* Global List: a single structure can be shared between all lists in
   the process space.  Another reason for declaring TheList (below) so
   that it's visible in this file is for OlListItemPointer().  R1.0 of
   Open Look specified OlListItemPointer() as a macro taking a single
   (token) argument.  With dynamic storage, the list structure would
   have to be passed in as well (via the widget: (w, token)).  Having
   the list visible to the procedure gets around this problem.
 */

typedef struct {
    OlListItem	item;
    int		indx;
} Node;

typedef struct _List {
    Node *	nodes;			/* array of nodes */
    Cardinal	num_nodes;		/* number of elements */
    Cardinal	num_slots;		/* number of allocated slots */
    IntArray	free;			/* array of free elements */
} * List;

static struct _List TheList = { 0, 0, 0, _OL_ARRAY_INITIAL};

#define SLISTW(w)	( XtParent(w) )
#define NULL_OFFSET	-1
#define PANEW(w)	( (ListPaneWidget)(w) )
#define PANEPTR(w)	( &(PANEW(w)->list_pane) )
#define PRIMPTR(w)	( &(((PrimitiveWidget)(w))->primitive) )
#define SBAR(w)		( _OlListSBar(SLISTW(w)) )

#define THE_LIST		( &TheList )
#define HEADPTR(w)		( &(PANEPTR(w)->head) )
#define NODES			( TheList.nodes )
#define AboveView(w, node)	( IndexOf(node) < TopIndex(w) )
#define AfterBottom(w)		( NumItems(w) - TopIndex(w) - ViewHeight(w) )
#define BelowView(w, node)	( IndexOf(node) >= \
					(TopIndex(w) + ViewHeight(w)) ) 
#define EmptyList(w)		( NumItems(w) == 0 )
#define FocusItem(w)		( PANEPTR(w)->focus_item )
#define FontHeight(font)	( font->max_bounds.ascent + \
					font->max_bounds.descent )
#define FontWidth(font)		( font->max_bounds.rbearing - \
					font->max_bounds.lbearing )
#define HeadOffset(w)		( HEADPTR(w)->offset )
#define IndexOf(node)		( NodePtr(node)->indx )
#define InView(w, node)		( !AboveView(w, node) && \
					!BelowView(w, node) )
#define IsCurrent(node)		( ItemPtr(node)->attr & OL_B_LIST_ATTR_CURRENT )
#define IsFocused(node)		( ItemPtr(node)->attr & OL_B_LIST_ATTR_FOCUS )
#define IsSelected(node)	( ItemPtr(node)->attr & OL_B_LIST_ATTR_SELECTED)
#define IsTop(w, indx)		( TopIndex(w) == (indx) )
#define ItemPtr(node)		( &(NodePtr(node)->item) )
#define Next(w, node)		( OffsetNode(w, node, 1) )
#define NodePtr(node)		( &(NODES[node]) )
#define NumItems(w)		( _OlArraySize(&(HEADPTR(w)->offsets)) )
#define OffsetNode(w, node, offset) ( OffsetOf(w, IndexOf(node) + (offset)) )
#define OffsetOf(w, indx)	( _OlArrayElement(&(HEADPTR(w)->offsets),indx) )
#define PaneIndex(w, node)	( IndexOf(node) - TopIndex(w) )
#define Prev(w, node)		( OffsetNode(w, node, -1) )
#define SearchItem(w)		( PANEPTR(w)->search_item )
#define TextField(w)		( PANEPTR(w)->text_field )
#define Top(w)			( PANEPTR(w)->top_item )
#define TopIndex(w)		( IndexOf(Top(w)) )
#define ValidSListWidget(w)	( ((w) != NULL) && \
				    XtIsSubclass(w, scrollingListWidgetClass) )
#define ViewHeight(w)		( PANEPTR(w)->actualViewHeight )

/* Function aliases: */
#define LPFreeGCs(w)		{ XtReleaseGC(w, PANEPTR(w)->gc_normal); \
				  XtReleaseGC(w, PANEPTR(w)->gc_inverted); \
                                  OlgDestroyAttrs(PANEPTR(w)->attr_normal); \
				}
#define MakeFocused(node)	ItemPtr(node)->attr |= OL_B_LIST_ATTR_FOCUS
#define MakeUnfocused(node)	ItemPtr(node)->attr &= ~OL_B_LIST_ATTR_FOCUS
#define SetInputFocus(w, time)	OlSetInputFocus(w, RevertToNone, time)
#define LatestTime(w)		XtLastTimestampProcessed(XtDisplay(w))
#define CurrentNotify(w, node)	\
	    if (XtHasCallbacks(SLISTW(w), XtNuserMakeCurrent) == \
	      XtCallbackHasSome) \
		XtCallCallbacks(SLISTW(w), XtNuserMakeCurrent, (XtPointer)node);

#ifdef DEBUG
    Boolean SLdebug = False;
#   define DPRINT(x)  if (SLdebug == True) (void)fprintf x
#else
#   define DPRINT(x)
#endif

#define INITIAL_INDEX	-1
#define NO_MOTION	0
#define SCROLL_UP	1
#define SCROLL_DOWN	( -SCROLL_UP )

/* ITEM SPACING:
 *
 * starting from the top (left) of the pane:
 *	margin surrounding entire list of items		(ITEM_MARGIN)
 *	(beginning of item)
 *	beginning of item to current-item border	(ITEM_BORDER)
 *	current-item border				(ITEM_BORDER)
 *	current-item border to label			(ITEM_BORDER)
 *	(label)
 *	same for bottom (right)
 */
					/* item spacing (in points) */
#define ITEM_MARGIN		4
#define ITEM_BORDER		1

	/*
	 * padding: constants to define padding around the label.
	 *	padding + label width (height) = item width (height)
	 *
	 * in addition, since HORIZ_PAD includes the left & right margins:
	 *	PaneWidth = ItemWidth  (see macros below):
	 */
#define VERT_PADDING	(2*ITEM_BORDER + 2*ITEM_BORDER + 2*ITEM_BORDER)
#define HORIZ_PADDING	(VERT_PADDING + 2*ITEM_MARGIN)

			/* conversion macros: convert 'value' to pixels. */
#define ConvertVert(w, value) \
		(OlScreenPointToPixel(OL_VERTICAL, value, XtScreen(w)))
#define ConvertHoriz(w, value) \
		(OlScreenPointToPixel(OL_HORIZONTAL, value, XtScreen(w)))

/* Calculate width and height of item including padding */

#define ItemWidth(w, item_width) ( (item_width) + PANEPTR(w)->horiz_pad )
#define ItemHeight(w)	( PANEPTR(w)->max_height + PANEPTR(w)->vert_pad )

/* calculate an index into the pane based on 'y' value */
#define IndexFromY(w, y) ( (Dimension)((y) - PANEPTR(w)->vert_margin) / \
						(Dimension)ItemHeight(w) )

/* Resource shared with textField */
static MaskArg TFieldArgs[] = {
    { XtNfont, NULL, OL_SOURCE_PAIR },
    { XtNfontColor, NULL, OL_SOURCE_PAIR },
    { XtNforeground, NULL, OL_SOURCE_PAIR },
    };

#define BYTE_OFFSET	XtOffsetOf(ListPaneRec, list_pane.dyn_flags)
static _OlDynResource dyn_res[] = {
#ifdef COLORED_LIKE_TEXT
{ { XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel), 0,
	XtRString, XtDefaultBackground }, BYTE_OFFSET, OL_B_LISTPANE_BG, NULL },
#endif
{ { XtNfontColor, XtCTextFontColor, XtRPixel, sizeof(Pixel), 0, XtRString,
	XtDefaultForeground }, BYTE_OFFSET, OL_B_LISTPANE_FONTCOLOR, NULL },
};
#undef BYTE_OFFSET

/***********************widget*translations*actions***********************
 *
 * Translations and Actions
 *
 */
#ifdef XT_AUGMENT_WORKS_RIGHT

/* the GenericTranslations must be augmented with BtnMotion */

static char extendedTranslations[] = "\
    #augment				\n\
    <BtnMotion>	:	OlAction()	\n";	/* see MotionHandler */

#else

/* everything but BtnMotion is taken from GenericTranslations (Action.c) */

static char extendedTranslations[] = "\
    <FocusIn>	:	OlAction()	\n\
    <FocusOut>	:	OlAction()	\n\
    <Key>	:	OlAction()	\n\
    <BtnDown>	:	OlAction()	\n\
    <BtnUp>	:	OlAction()	\n\
    <BtnMotion>	:	OlAction()	\n";	/* see MotionHandler */

#endif

static OlEventHandlerRec event_procs[] = {
    { ButtonPress,	ButtonDown },
    { ButtonRelease,	ButtonUp },
    { KeyPress,		KeyDown },
    { MotionNotify,	ButtonMotion },
};

/****************************widget*resources*****************************
 *
 * ListPane Resources
 */

#define OFFSET(field)	XtOffsetOf(ListPaneRec, field)
#define POFFSET(field)	OFFSET(list_pane.field)

static XtResource resources[] = {
					/* core resources: */
    { XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
	OFFSET(core.border_width), XtRImmediate, (XtPointer) 1
    },
#ifdef COLORED_LIKE_TEXT
    { XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel),
	OFFSET(core.background_pixel), XtRString, XtDefaultBackground
    },
#endif
    { XtNfontColor, XtCTextFontColor, XtRPixel, sizeof(Pixel),
	OFFSET(primitive.font_color), XtRString, XtDefaultForeground
    },
					/* list pane resources: */
    { XtNapplAddItem, XtCApplAddItem, XtRFunction, sizeof(OlListToken(*)()),
	POFFSET(applAddItem), XtRFunction, NULL
    },
    { XtNapplDeleteItem, XtCApplDeleteItem, XtRFunction, sizeof(void(*)()),
	POFFSET(applDeleteItem), XtRFunction, NULL
    },
    { XtNapplEditClose, XtCApplEditClose, XtRFunction, sizeof(void(*)()),
	POFFSET(applEditClose), XtRFunction, NULL
    },
    { XtNapplEditOpen, XtCApplEditOpen, XtRFunction, sizeof(void(*)()),
	POFFSET(applEditOpen), XtRFunction, NULL
    },
    { XtNapplTouchItem, XtCApplTouchItem, XtRFunction, sizeof(void(*)()),
	POFFSET(applTouchItem), XtRFunction, NULL
    },
    { XtNapplUpdateView, XtCApplUpdateView, XtRFunction, sizeof(void(*)()),
	POFFSET(applUpdateView), XtRFunction, NULL
    },
    { XtNapplViewItem, XtCApplViewItem, XtRFunction, sizeof(void(*)()),
	POFFSET(applViewItem), XtRFunction, NULL
    },
    { XtNrecomputeWidth, XtCRecomputeWidth, XtRBoolean, sizeof(Boolean),
	POFFSET(recompute_width), XtRImmediate, (XtPointer)True
    },
    { XtNselectable, XtCSelectable, XtRBoolean, sizeof(Boolean),
	POFFSET(selectable), XtRImmediate, (XtPointer)True
    },
    { XtNtextField, XtCTextField, XtRWidget, sizeof(Widget),
	POFFSET(text_field), XtRWidget, NULL
    },
    { XtNviewHeight, XtCViewHeight, XtRCardinal, sizeof(Cardinal),
	POFFSET(view_height), XtRImmediate, (XtPointer) 0
    },
};

#undef OFFSET
#undef POFFSET

/***************************widget*class*record***************************
 *
 * Full class record constant
 */

ListPaneClassRec listPaneClassRec = {
  {
/* core_class fields		*/
    /* superclass		*/	(WidgetClass) &primitiveClassRec,
    /* class_name		*/	"ListPane",
    /* widget_size		*/	sizeof(ListPaneRec),
    /* class_initialize		*/	NULL,
    /* class_part_init		*/	NULL,
    /* class_inited		*/	False,
    /* initialize		*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize			*/	XtInheritRealize,
    /* actions			*/	NULL,
    /* num_actions		*/	0,
    /* resources		*/	resources,
    /* num_resources		*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	False,
    /* compress_exposure	*/	True,
    /* compress_enterleave	*/	True,
    /* visible_interest		*/	False,
    /* destroy			*/	Destroy,
    /* resize			*/	XtInheritResize,
    /* expose			*/	Redisplay,
    /* set_values		*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus		*/	AcceptFocus,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table			*/	extendedTranslations,
    /* query geometry		*/	NULL
  },
  { /* primitive_class fields */
    /* focus_on_select		*/	True,
    /* highlight_handler	*/	HighlightHandler,
    /* traversal_handler	*/	TraversalHandler,
    /* register_focus		*/	RegisterFocus,
    /* activate			*/	Activate,
    /* event_procs		*/	event_procs,
    /* num_event_procs		*/	XtNumber(event_procs),
    /* version			*/	OlVersion,
    /* extension		*/	NULL,
    /* dyn_data			*/	{ dyn_res, XtNumber(dyn_res) },
    /* transparent_proc		*/	NULL,
  },
};

WidgetClass listPaneWidgetClass = (WidgetClass)&listPaneClassRec;

/***************************private*procedures****************************
 *
 * Private Functions
 *
 */

/******************************function*header****************************
 * ApplAddItem- called by appl to insert item into list
 */
static OlListToken
ApplAddItem OLARGLIST((slw, parent, refNode, item))
    OLARG(Widget,	slw)		/* scrolling list widget */
    OLARG(OlListToken,	parent)		/* for (future) mulit-level lists */
    OLARG(OlListToken,	refNode)	/* insert relative to this item */
    OLGRA(OlListItem,	item)		/* item to insert */
{
    ListPaneWidget	w;
    int			node;		/* index of new node */

    if (!ValidSListWidget(slw))
    {

	OlVaDisplayWarningMsg(	XtDisplay(slw),
				OleNfileListPane,
				OleTmsg1,
				OleCOlToolkitWarning,
				OleMfileListPane_msg1,
				"addItem");
	return (NULL);
    }

    if (item.label_type != OL_STRING)
    {
	if (item.label_type == OL_IMAGE)

		OlVaDisplayWarningMsg(	XtDisplay(slw),
					OleNfileListPane,
					OleTmsg3,
					OleCOlToolkitWarning,
					OleMfileListPane_msg3);
	else
		OlVaDisplayWarningMsg(	XtDisplay(slw),
					OleNfileListPane,
					OleTmsg4,
					OleCOlToolkitWarning,
					OleMfileListPane_msg4);
	return(NULL);
    }

    w = PANEW(_OlListPane(slw));	/* get list pane widget */

    node = InsertNode(w, (int)refNode);

    *ItemPtr(node) = item;		/* copy in item data */

    if (item.mnemonic != '\0')
	_OlAddMnemonic((Widget)w, (XtPointer)node, item.mnemonic);

    MaintainWidth(w, &item);

    /* If this is the 1st item in the list or inserting above view and
	view is unfilled, make it the top_item.
    */
    if ((Top(w) == NULL_OFFSET) ||
      (AboveView(w, node) && (NumItems(w) <= ViewHeight(w))))
	Top(w) = node;

    /* If we have focus but focus_item is NULL, make this the focus_item.
	This happens when entire list is deleted, then new item added.
    */
    if (PRIMPTR(w)->has_focus && (FocusItem(w) == NULL_OFFSET))
    {
	FocusItem(w) = node;
	MakeFocused(FocusItem(w));
    }

    /* # of items increased, recalibrate SBar */
    if (PANEPTR(w)->update_view)
    {
	UpdateSBar(w);

		/* scroll view if appropriate */
	if (InView(w, node) && XtIsRealized((Widget)w))
	{
	    ScrollView(w);
	}
    }

    return ( (OlListToken)node );
}

/*****************************function*header****************************
 * ApplDeleteItem- called by appl to delete item.
 */
static void
ApplDeleteItem OLARGLIST((slw, token))
    OLARG(Widget,	slw)		/* scrolling list widget */
    OLGRA(OlListToken,	token)
{
    ListPaneWidget	w;
    int			node;
    Boolean		in_view;

    if (!ValidSListWidget(slw))
    {
	OlVaDisplayWarningMsg(	XtDisplay(slw),
				OleNfileListPane,
				OleTmsg1,
				OleCOlToolkitWarning,
				OleMfileListPane_msg1,
				"deleteItem");
	return;
    }

    if (token == NULL)
    {
	OlVaDisplayWarningMsg(	XtDisplay(slw),
				OleNfileListPane,
				OleTmsg2,
				OleCOlToolkitWarning,
				OleMfileListPane_msg2,
				"deleteItem");
	return;
    }

    w = PANEW(_OlListPane(slw));	/* get list pane widget */

    /* safety check for deletion from NULL list */
    if (EmptyList(w))
	return;

    node = (int)token;			/* make token an offset into list */

    in_view = InView(w, node);

    /* if item being deleted is in view, consider scrolling in new item */
    if (in_view)
    {
	/* Deleting Top */
	if (node == Top(w))
	{
	    /* Move Top back.  If at head, move forward */
	    Top(w) = (Top(w) == HeadOffset(w)) ?
			(NumItems(w) == 1) ? NULL_OFFSET :	/* last item */
			Next(w, Top(w)) : Prev(w, Top(w));

	/* If node is in upper half of view, back up Top if possible.
	   If node is in lower half and there is an item below view,
	   it will 'naturally' scroll into view when the pane is redrawn.
	   However, if there is no item below the view, attempt to bring
	   in item from above.
	*/
	} else if ((Top(w) != HeadOffset(w)) &&
	  ((PaneIndex(w, node) < ViewHeight(w)/2) || (AfterBottom(w) < 1))) {
		Top(w) = Prev(w, Top(w));
	}
    }

    /* update count of selected items */
    if (IsSelected(node))
	PANEPTR(w)->items_selected--;

    /* if the focus_item is being deleted, move it to prev (or next) */
    if (node == FocusItem(w))
    {
	FocusItem(w) = (node == HeadOffset(w)) ?
			(NumItems(w) == 1) ? NULL_OFFSET :	/* last item */
			Next(w, FocusItem(w)) : Prev(w, FocusItem(w));
	
	if (PRIMPTR(w)->has_focus && (FocusItem(w) != NULL_OFFSET))
	    MakeFocused(FocusItem(w));
    }

    /* If last-searched item deleted, make it invalid */
    if (node == SearchItem(w))
	SearchItem(w) = NULL_OFFSET;

    /* If there is a mnemonic, remove from global list of mnemonics */
    if (ItemPtr(node)->mnemonic)
	_OlRemoveMnemonic((Widget)w, (XtPointer)node, False,
			  ItemPtr(node)->mnemonic);

    DeleteNode(w, node);		/* delete it from list */

    /* # of items decreased, recalibrate SBar */
    if (PANEPTR(w)->update_view)
    {
	UpdateSBar(w);

	/* redisplay pane if appropriate */
	if (in_view && XtIsRealized((Widget)w))
	{
	    Redisplay(w, (XEvent *)NULL, (Region)NULL);
	}
    }
}

/*****************************function*header****************************
 * ApplEditClose-
 */
static void
ApplEditClose OLARGLIST((slw))
    OLGRA(Widget, slw)			/* scrolling list widget */
{
    ListPaneWidget w;

    if (!ValidSListWidget(slw))
    {
	OlVaDisplayWarningMsg(	XtDisplay(slw),
				OleNfileListPane,
				OleTmsg1,
				OleCOlToolkitWarning,
				OleMfileListPane_msg1,
				"editClose");
	return;
    }

    w = PANEW(_OlListPane(slw));		/* get list pane widget */

    if (!PANEPTR(w)->editing)			/* if we're not editing */
	return;					/*  return silently */

    PANEPTR(w)->editing = False;

    SetInputFocus((Widget)w, LatestTime(w));	/* set focus back to pane */

    XtUnmapWidget(TextField(w));		/* unmap text field */

    if (NumItems(w) > ViewHeight(w))
	XtSetSensitive(SBAR(w), True);		/* turn scrollbar back on */
}

/*****************************function*header****************************
 * ApplEditOpen-
 */
static void
ApplEditOpen OLARGLIST((slw, insert, reference))
    OLARG(Widget,	slw)		/* scrolling list widget */
    OLARG(Boolean,	insert)
    OLGRA(OlListToken,	reference)	/* edit this item */
{
    ListPaneWidget	w;
    int			node;
    Arg			args[10];
    Cardinal		cnt;
    Time		time;

    if (!ValidSListWidget(slw))
    {
	OlVaDisplayWarningMsg(	XtDisplay(slw),
				OleNfileListPane,
				OleTmsg1,
				OleCOlToolkitWarning,
				OleMfileListPane_msg1,
				"editOpen");
	return;
    }

    if (reference == NULL)
    {

	OlVaDisplayWarningMsg(	XtDisplay(slw),
				OleNfileListPane,
				OleTmsg5,
				OleCOlToolkitWarning,
				OleMfileListPane_msg5);
	return;
    }

    /* editing only makes sense if we're Realized */
    if (!XtIsRealized(slw))
    {
	OlVaDisplayWarningMsg(	XtDisplay(slw),
				OleNfileListPane,
				OleTmsg6,
				OleCOlToolkitWarning,
				OleMfileListPane_msg6);
	return;
    }

    if (insert) {

	OlVaDisplayWarningMsg(	XtDisplay(slw),
				OleNfileListPane,
				OleTmsg7,
				OleCOlToolkitWarning,
				OleMfileListPane_msg7);
	return;
    }

    ApplViewItem(slw, reference);	/* be sure item is in view */

    w = PANEW(_OlListPane(slw));	/* get list pane widget */

    XtSetSensitive(SBAR(w), False);	/* shut off scrollbar */

    PANEPTR(w)->editing = True;		/* we're now editing */
    node = (int)reference;		/* make reference an offset into list */
    XtRealizeWidget(TextField(w));	/* must be realized before resize */

    /* set geometry and string of text field */
    cnt = 0;
    XtSetArg(args[cnt], XtNx, PANEPTR(w)->horiz_margin); cnt++;
    XtSetArg(args[cnt], XtNy, PANEPTR(w)->vert_margin +
			PaneIndex(w, node) * ItemHeight(w)); cnt++; 
    XtSetArg(args[cnt], XtNwidth,
			w->core.width - 2*PANEPTR(w)->horiz_margin); cnt++;
    XtSetArg(args[cnt], XtNstring, ItemPtr(node)->label); cnt++;
    XtSetValues(TextField(w), args, cnt);

    /* map the text field and set focus to it */
    XtMapWidget(TextField(w));
    time = LatestTime(w);
    (void) XtCallAcceptFocus(TextField(w), &time);
}

/*****************************function*header****************************
 * ApplTouchItem-
 */
static void
ApplTouchItem OLARGLIST((slw, token))
    OLARG(Widget,	slw)		/* scrolling list widget */
    OLGRA(OlListToken,	token)		/* touch this item */
{
    ListPaneWidget	w;
    int			node;

    if (!ValidSListWidget(slw))
    {
	OlVaDisplayWarningMsg(	XtDisplay(slw),
				OleNfileListPane,
				OleTmsg1,
				OleCOlToolkitWarning,
				OleMfileListPane_msg1,
				"touchItem");
	return;
    }

    if (token == NULL)
    {
	OlVaDisplayWarningMsg(	XtDisplay(slw),
				OleNfileListPane,
				OleTmsg2,
				OleCOlToolkitWarning,
				OleMfileListPane_msg2,
				"touchItem");
	return;
    }

    w = PANEW(_OlListPane(slw));	/* get list pane widget */
    node = (int)token;			/* make token an offset into list */

    MaintainWidth(w, ItemPtr(node));	/* do bookkeeping on widest item */

    /* draw item if appropriate */
    if (InView(w, node) && XtIsRealized((Widget)w) && PANEPTR(w)->update_view)
	DrawItem(w, node);
}

/*****************************function*header****************************
 * ApplUpdateView-
 */
static void
ApplUpdateView OLARGLIST((slw, ok))
    OLARG(Widget,	slw)		/* scrolling list widget */
    OLGRA(Boolean,	ok)		/* ok to update view (or not) */
{
    ListPaneWidget w;

    if (!ValidSListWidget(slw))
    {
	OlVaDisplayWarningMsg(	XtDisplay(slw),
				OleNfileListPane,
				OleTmsg1,
				OleCOlToolkitWarning,
				OleMfileListPane_msg1,
				"updateView");
	return;
    }

    w = PANEW(_OlListPane(slw));	/* get list pane widget */

    PANEPTR(w)->update_view = ok;

    if (ok)
    {
	MaintainWidth(w, NULL);		/* pass in NULL so max_width is used */

	if (XtIsRealized((Widget)w))
	    Redisplay(w, (XEvent *)NULL, (Region)NULL);

	UpdateSBar(w);
    }
}

/*****************************function*header****************************
 * ApplViewItem-
 */
static void
ApplViewItem OLARGLIST((slw, token))
    OLARG(Widget,	slw)		/* scrolling list widget */
    OLGRA(OlListToken,	token)		/* view this item */
{
    ListPaneWidget	w;
    int			node;

    if (!ValidSListWidget(slw))
    {
	OlVaDisplayWarningMsg(	XtDisplay(slw),
				OleNfileListPane,
				OleTmsg1,
				OleCOlToolkitWarning,
				OleMfileListPane_msg1,
				"viewItem");
	return;
    }

    if (token == NULL)
    {
	OlVaDisplayWarningMsg(	XtDisplay(slw),
				OleNfileListPane,
				OleTmsg2,
				OleCOlToolkitWarning,
				OleMfileListPane_msg2,
				"viewItem");
	return;
    }

    w = PANEW(_OlListPane(slw));	/* get list pane widget */
    node = (int)token;			/* make token an offset into list */

    if (InView(w, node))		/* trivial: already in view */
	return;

    /* adjust top_item according to whether node is above or below view */
    Top(w) = (AboveView(w, node)) ?
		node :			/* move node to top of view */
					/* move node to bottom of view: */
		OffsetNode(w, Top(w),
			IndexOf(node) - TopIndex(w) - ViewHeight(w) + 1);
		
    ScrollView(w);		/* do scroll specific stuff */
    UpdateSBar(w);		/* item moved into view: adjust scrollbar! */
}

/*****************************function*header****************************
 * BuildClipboard-
 */
static void
BuildClipboard OLARGLIST((w))
    OLGRA(ListPaneWidget, w)
{
    int		node;
    Cardinal	char_cnt = 0;
    Cardinal	item_cnt;
    String	ptr;

    /* first, accumulate byte count from labels of selected items */
    node = HeadOffset(w);
    for (item_cnt = PANEPTR(w)->items_selected; item_cnt > 0; )
    {
	if (IsSelected(node))
	{
	    item_cnt--;
	    char_cnt += _OlStrlen(ItemPtr(node)->label) + 1; /* + \n */
	}
	node = Next(w, node);
    }

    /* allocate storage (if necessary) for char_cnt bytes */
    if (char_cnt > ClipboardCnt)
    {
	ClipboardCnt	= char_cnt;
	Clipboard	= XtRealloc(Clipboard, ClipboardCnt);
    }

    /* copy labels (and '\n') into Clipboard */

    ptr = Clipboard;			/* point to Clipboard */

    node = HeadOffset(w);
    for (item_cnt = PANEPTR(w)->items_selected; item_cnt > 0; )
    {
	if (IsSelected(node))
	{
	    item_cnt--;
	    (void) strcpy(ptr, ItemPtr(node)->label);
	    ptr += _OlStrlen(ItemPtr(node)->label);
	    *ptr++ = '\n';
	}
	node = Next(w, node);
    }

    *(ptr - 1) = '\0';		/* replace last \n with terminator */
}

/*****************************function*header****************************
 * BuildDeleteList-
 */
static void
BuildDeleteList OLARGLIST((w, deleteList))
    OLARG(ListPaneWidget,	w)
    OLGRA(OlListDelete *,	deleteList)
{
    Cardinal		cnt;
    int			node;
    OlListToken *	tokens;		/* array of token to be deleted */

    deleteList->num_tokens = PANEPTR(w)->items_selected;

    if (deleteList->num_tokens == 0)
    {
	deleteList->tokens = NULL;
	return;
    }

    deleteList->tokens = (OlListToken *)XtMalloc(
				deleteList->num_tokens * sizeof(OlListToken));
    tokens = deleteList->tokens;

    for (node = HeadOffset(w), cnt = deleteList->num_tokens;
      cnt > 0; node = Next(w, node))
	if (IsSelected(node))
	{
	    *tokens++ = (OlListToken)node;
	    cnt--;
	}
}

/*****************************function*header****************************
   SetPaneHeight- called at Initialize & SetValues to set actualViewHeight
	and Pane height based on settings in core.height &
	list_pane.view_height.  Scrollbar is resized to height of pane.

	If view_height > 0, make widget tall enough for that many items.
	Else, if height > 0, make pane height tall enough for an integral
	number of items.
	Else, make height & actualViewHeight size of (marginal) minimal
	scrollbar.

	SetPaneHeight depends on list_pane.max_height being set.
*/
static void
SetPaneHeight OLARGLIST((w))
    OLGRA(ListPaneWidget, w)
{
    if (PANEPTR(w)->view_height > 0)
    {
	ViewHeight(w) = PANEPTR(w)->view_height;

    } else {
	Widget model_widget = (w->core.height > 0) ? (Widget)w : SBAR(w);

	ViewHeight(w) =
	    (Dimension)(model_widget->core.height + ItemHeight(w) - 1) /
						(Dimension)ItemHeight(w);
    }

    /* Set pane height */
    w->core.height = ViewHeight(w) * ItemHeight(w) +
					2 * PANEPTR(w)->vert_margin;

    /* Set height of SBar to that of ListPane.

       Ideally, sizing the scrollbar would be done automatically by
       placing it in a control area with the pane.  The correct visual
       cannot be achieved this way, however.  The line separating the
       pane from the title would be continuous rather than broken.
    */
    XtResizeWidget(SBAR(w), SBAR(w)->core.width,
			w->core.height,		/* height of pane */
			SBAR(w)->core.border_width);
}

/*****************************function*header****************************
 * ClearSelection-
 */
static void
ClearSelection OLARGLIST((w))
    OLGRA(ListPaneWidget, w)
{
    int		node;
    Cardinal	cnt;

    for (node = HeadOffset(w), cnt = PANEPTR(w)->items_selected;
      cnt > 0; node = Next(w, node))
	if (IsSelected(node))
	{
	    cnt--;
	    ItemPtr(node)->attr &= ~OL_B_LIST_ATTR_SELECTED;	/* unselect */
	    if (InView(w, node) && PANEPTR(w)->update_view)
		DrawItem(w, node);
	}

    PANEPTR(w)->items_selected = 0;
}

/*****************************function*header****************************
 * CreateGCs-
 */
static void
CreateGCs OLARGLIST((w))
    OLGRA(ListPaneWidget, w)
{
    unsigned long	valueMask;
    XGCValues		values;
    Pixel		tmp;

    valueMask			= (GCForeground | GCBackground | GCFont);
    values.foreground		= PRIMPTR(w)->font_color;
    values.background		= w->core.background_pixel;
    values.font			= PRIMPTR(w)->font->fid;

    PANEPTR(w)->gc_normal	= XtGetGC((Widget)w, valueMask, &values);

    tmp				= values.foreground;
    values.foreground		= values.background;
    values.background		= tmp;

    PANEPTR(w)->gc_inverted	= XtGetGC((Widget)w, valueMask, &values);

    PANEPTR(w)->attr_normal	= OlgCreateAttrs(XtScreen(w),
					PRIMPTR(w)->foreground,
					(OlgBG *)&(w->core.background_pixel),
					False, OL_DEFAULT_POINT_SIZE);

    PANEPTR(w)->attr_focus	= OlgCreateAttrs(XtScreen(w),
					PRIMPTR(w)->foreground,
					(OlgBG *)&(PRIMPTR(w)->input_focus_color),
					False, OL_DEFAULT_POINT_SIZE);
}

/******************************function*header****************************
    CutOrCopy- copy selection contents to clipboard
*/
static void
CutOrCopy OLARGLIST((w, cut))
    OLARG(ListPaneWidget,	w)
    OLGRA(Boolean,		cut)
{
    /* Do we beep the display if we own the selection but it is NULL? */
    if (PANEPTR(w)->items_selected == 0)
    {
	_OlBeepDisplay((Widget)w, 1);
	return;			/* return silently */
    }

    /* Do we beep when CUT and no appl delete callback? */
    if (cut &&
      (XtHasCallbacks(SLISTW(w), XtNuserDeleteItems) != XtCallbackHasSome))
    {
	_OlBeepDisplay((Widget)w, 1);
	return;			/* return silently */
    }

    /* get the clipboard if we don't already own it */
    if (!PANEPTR(w)->own_clipboard)
    {
	PANEPTR(w)->own_clipboard =
	    XtOwnSelection((Widget)w, XA_CLIPBOARD(XtDisplay(w)), LatestTime(w),
		ConvertSelection, LoseSelection, SelectionDone);

	if (!PANEPTR(w)->own_clipboard) {
		OlVaDisplayWarningMsg(	XtDisplay(w),
					OleNfileListPane,
					OleTmsg8,
					OleCOlToolkitWarning,
					OleMfileListPane_msg8);
	    return;
	}
    }

    BuildClipboard(w);

    DPRINT((stderr, "%s selection= %s \n", (cut) ? "cut" : "copy", Clipboard));

    if (cut)			/* call any user delete callbacks */
    {
	OlListDelete items;

	BuildDeleteList(PANEW(w), &items);

	XtCallCallbacks(SLISTW(w), XtNuserDeleteItems, (XtPointer)&items);
	XtFree((char *)items.tokens);
    }

    ClearSelection(w);		/* OL says clear selection after CUT/COPY */
}

/******************************function*header****************************
    DeleteNode-

    Widget list specific first:
     1	decrement index of all nodes 'above' node to be deleted
     2	delete node from widget's list
     3	fix up HeadOffset if deleting head and/or last item

    List store specific:
     4	decrement number of total items
     5	put deleted node in free list or free everything if last item

     Note that inserting at the end of the free list is faster than at the
     beginning of the list.  Deleting is done at the beginning of the free
     list (during InsertNode) which was felt to be less frequent.  Optimizing
     DeleteNode was deemed appropriate since destroying a widget would
     result in many calls to DeleteNode.
*/
static void
DeleteNode OLARGLIST((w, node))
    OLARG(ListPaneWidget,	w)
    OLGRA(int,			node)	/* (offset of) node to delete */
{
    _OlHead	head = HEADPTR(w);
    List	list = THE_LIST;
    int		i;

    /* Do work on behalf of widget first */

    /* Decrement index of all nodes 'above' deleted node */
    for (i = IndexOf(node) + 1; i < NumItems(w); i++)
	IndexOf(OffsetOf(w, i))--;

    /* Delete node from widget's list of offsets (nodes). */
    _OlArrayDelete(&(head->offsets), IndexOf(node));

    if (EmptyList(w))			/* free offset array when last item */
    {
	_OlArrayFree(&(head->offsets));
	HeadOffset(w) = NULL_OFFSET;

    } else if (node == HeadOffset(w)) {	/* if deleting the head, */
	HeadOffset(w) = OffsetOf(w, 0);	/*  new head is offset of index 0 */
    }

    /* Now delete node from list store */

    list->num_nodes--;			/* one less node */

    if (list->num_nodes == 0)	/* If all nodes deleted, free storage. */
    {
	XtFree((char *)list->nodes);
	list->nodes	= NULL;
	list->num_slots	= 0;

	_OlArrayFree(&(list->free));	/* free "free" list */

    } else {
				/* save node on free list */
	_OlArrayAppend(&(list->free), node);
    }
}

/*****************************function*header****************************
    DrawItem-
	First, the background is painted.
	Then the current item border is drawn.
	Then the string is drawn.
*/
static void
DrawItem OLARGLIST((w, node))
    OLARG(ListPaneWidget,	w)
    OLGRA(int,			node)
{
    Position	x, y;
    Dimension	width, height;
    Dimension	hBorder;
    Dimension	vBorder;
    GC		fore_gc, back_gc;
    OlgAttrs *	attr;
    Boolean	releaseGCs = False;
    Boolean	releaseAttr = False;

    x		= PANEPTR(w)->horiz_margin;
    width	= w->core.width - 2*x;	/* ie., left & right margins */
    height	= ItemHeight(w);
    y		= PANEPTR(w)->vert_margin + PaneIndex(w, node) * height;
    hBorder	= OlgGetHorizontalStroke(PANEPTR(w)->attr_normal);
    vBorder	= OlgGetVerticalStroke(PANEPTR(w)->attr_normal);

    if (IsFocused(node))		/* drawing focus_item ? */
    {
	if (PRIMPTR(w)->font_color == PRIMPTR(w)->input_focus_color)
	{
	    /* when collision with colors, must reverse normal settings */
	    if (IsSelected(node))
	    {
		back_gc = PANEPTR(w)->gc_inverted;
		fore_gc = PANEPTR(w)->gc_normal;
		attr = PANEPTR(w)->attr_normal;
	    } else {
		back_gc = PANEPTR(w)->gc_normal;
		fore_gc = PANEPTR(w)->gc_inverted;
		releaseAttr = True;
		attr = OlgCreateAttrs(XtScreen(w),
				      w->core.background_pixel,
				      (OlgBG *)&(PRIMPTR(w)->foreground), False,
				      OL_DEFAULT_POINT_SIZE);
	    }

	} else {
	    unsigned long	valueMask = GCForeground|GCBackground|GCFont;
	    XGCValues		values;
	    Pixel		tmp;

	    releaseGCs = True;

	    if (IsSelected(node))
	    {
		values.foreground = PRIMPTR(w)->font_color;
		values.background = PRIMPTR(w)->input_focus_color;
	    
	    } else {
		values.foreground = PRIMPTR(w)->input_focus_color;
		values.background = PRIMPTR(w)->font_color;
	    }

	    values.font		= PRIMPTR(w)->font->fid;
	    back_gc		= XtGetGC((Widget)w, valueMask, &values);

	    tmp			= values.foreground;
	    values.foreground	= values.background;
	    values.background	= tmp;
	    fore_gc		= XtGetGC((Widget)w, valueMask, &values);

	    attr		= PANEPTR(w)->attr_focus;
	}

    } else {				/* not drawing focus_item */
	if (IsSelected(node))
	{
	    back_gc = PANEPTR(w)->gc_normal;
	    fore_gc = PANEPTR(w)->gc_inverted;

	} else {
	    back_gc = PANEPTR(w)->gc_inverted;
	    fore_gc = PANEPTR(w)->gc_normal;
	}
	attr = PANEPTR(w)->attr_normal;
    }

    /* Paint background with background color */

    XFillRectangle(XtDisplay(w), XtWindow(w), back_gc, x, y, width, height);

    x		+= hBorder;
    y		+= vBorder;
    width	-= 2 * hBorder;
    height	-= 2 * vBorder;

    if (IsCurrent(node))
    {
	if (OlgIs3d())
	{
	    XFillRectangle(XtDisplay(w), XtWindow(w),
				OlgGetBg2GC(attr), x, y, width, height);

	    OlgDrawBox(XtScreen(w), XtWindow(w),
				attr, x, y, width, height, True);

	} else {
	    XRectangle	rects [4];

	    rects [0].x = x;
	    rects [0].y = y;
	    rects [0].width = width;
	    rects [0].height = vBorder;

	    rects [1].x = x + width - hBorder;
	    rects [1].y = y + vBorder;
	    rects [1].width = hBorder;
	    rects [1].height = height - vBorder;

	    rects [2].x = x;
	    rects [2].y = y + height - vBorder;
	    rects [2].width = width - hBorder;
	    rects [2].height = vBorder;

	    rects [3].x = x;
	    rects [3].y = y + vBorder;
	    rects [3].width = hBorder;
	    rects [3].height = height - vBorder * 2;

	    XFillRectangles(XtDisplay(w), XtWindow(w), fore_gc, rects, 4);

	}
    }

    /* update bounding box for (re)drawing selected area inside of 3D item */

    x		+= hBorder * 2;
    y		+= vBorder * 2;
    width	-= hBorder * 4;
    height	-= vBorder * 4;

    if (OlgIs3d() && IsSelected(node))
	XFillRectangle(XtDisplay(w), XtWindow(w), back_gc, x, y, width, height);

    /* Now draw text */
    OlgDrawTextLabel(XtScreen(w), XtWindow(w), attr, x, y, width, height,
			Item2TextLbl(w, ItemPtr(node), fore_gc, back_gc));

    if (releaseGCs)		/* if temp GC's created, release them */
    {
	XtReleaseGC((Widget)w, back_gc);
	XtReleaseGC((Widget)w, fore_gc);
    }

    if (releaseAttr)
	OlgDestroyAttrs (attr);

    XFlush(XtDisplay(w));
}

/******************************function*header****************************
   FreeList- move any items to free list by deleting them.  Widget's offset
	array is freed when all items deleted.
*/
static void
FreeList OLARGLIST((w))
    OLGRA(ListPaneWidget, w)
{
    int i;

    for (i = NumItems(w) - 1; i >= 0; i--)
	DeleteNode(w, OffsetOf(w, i));
}

/******************************function*header****************************
    InsertNode-
     1	get node (either from free list of at end of 'store')
     2	inc number of nodes in 'store'
     3	inc indices in widget's portion of list above inserted node
     4	insert node (offset) into widget's list of offsets
     5	return node

	Node is inserted before refOffset where refOffset == 0 means append
	to list (as specified from OL R1.0).  Since '0' is reserved for
	this purpose, node returned must be 'overstated' by 1; that is,
	the very 1st offset returned is '1' rather than '0'.  As a result,
	Node '0' is unused space.
*/
static int
InsertNode OLARGLIST((w, refOffset))
    OLARG(ListPaneWidget,	w)
    OLGRA(int,			refOffset)	/* reference node offset */
{
    _OlHead	head = HEADPTR(w);
    List	list = THE_LIST;
    int		node;
    int		refIndex;

    /* inc num_nodes 1st so '0' is never used (see func header) */
    list->num_nodes++;

    /* Get node offset for new item */
    if (_OlArraySize(&(list->free)) > 0)
    {
	/*
	 * Take node off the end of the array to avoid having to do any
	 * real shifts when deleting the node.
	 */
	int	z = _OlArraySize(&(list->free)) - 1;
	node = _OlArrayElement(&(list->free), z);
	_OlArrayDelete(&(list->free), z);

    } else {
	/* increase size of Node store if necessary */
	if (list->num_nodes >= list->num_slots)
	{
	    list->num_slots += (list->num_slots / 2) + 2;
	    list->nodes = (Node *)XtRealloc((char *)list->nodes,
					list->num_slots * sizeof(Node));
	}
	node = list->num_nodes;
    }

    /* If this is the 1st node or inserting before Head, make it the head */
    if ((HeadOffset(w) == NULL_OFFSET) || (HeadOffset(w) == refOffset))
	HeadOffset(w) = node;

    /* Increment indices in widget's portion of the list. */

    if (refOffset == 0)			/* inserting at end-of-list */
    {
	refIndex = NumItems(w);		/* 'beyond' end-of-list */

    } else {
	int i;

	refIndex = IndexOf(refOffset);

	/* Increment index of all nodes from reference node and up */
	for (i = refIndex; i < NumItems(w); i++)
	    IndexOf(OffsetOf(w, i))++;
    }

    IndexOf(node) = refIndex;		/* index of new node gets refIndex */

    /* Insert new node into widget's list of offsets (nodes). */
    _OlArrayInsert(&(head->offsets), refIndex, node);

    return(node);
}

/******************************function*header****************************
    Item2TextLbl- populate a OlgTextLbl struct with OlListItem data
*/
static OlgTextLbl *
Item2TextLbl OLARGLIST((w, item, normalGC, inverseGC))
    OLARG(ListPaneWidget,	w)
    OLARG(OlListItem *,		item)
    OLARG(GC,			normalGC)
    OLGRA(GC,			inverseGC)
{
    static OlgTextLbl text_lbl;

    text_lbl.label		= item->label;
    text_lbl.normalGC		= normalGC;
    text_lbl.inverseGC		= inverseGC;
    text_lbl.font		= PRIMPTR(w)->font;
    text_lbl.accelerator	= NULL;
    text_lbl.mnemonic		= item->mnemonic;
    text_lbl.justification	= TL_LEFT_JUSTIFY;
    text_lbl.flags		= 0;
    text_lbl.font_list		= w->primitive.font_list;

    return (&text_lbl);
}

/******************************function*header****************************
    MaintainWidth- the width of the widest item must be kept.  If this
		is larger than the current width of the widget, make
    geometry request.  If item == NULL, use established width in max_width.
    This is the case for updateView: when updateView goes from False to True,
    we must consider updating the geometry.
*/
static void
MaintainWidth OLARGLIST((w, item))
    OLARG(ListPaneWidget,	w)
    OLGRA(OlListItem *,		item)
{
    Dimension width;
    Dimension height;		/* don't care */

    if (item == NULL)
    {
	width	= PANEPTR(w)->max_width;

    } else {
	OlgSizeTextLabel(XtScreen(w), PANEPTR(w)->attr_normal, 
	    Item2TextLbl(w, item,
		PANEPTR(w)->gc_normal, PANEPTR(w)->gc_inverted),
	    &width, &height);
					/* keep width of widest item: */
	_OlAssignMax(PANEPTR(w)->max_width, width);
    }

    /* consider adjusting widget width */
    if (PANEPTR(w)->recompute_width && PANEPTR(w)->update_view)
    {
	width = ItemWidth(w, width);		/* include padding */
	if (width > w->core.width)		/* grow, never shrink */
	{
	    XtWidgetGeometry request;

	    request.request_mode = CWWidth;
	    request.width	 = width;
	    (void)XtMakeGeometryRequest((Widget)w, &request, NULL);
	}
    }
}

/*****************************function*header****************************
   ScollView- called when view scrolls.  Maintain focus_item in view.
*/
static void
ScrollView OLARGLIST((w))
    OLGRA(ListPaneWidget, w)
{
    if ((FocusItem(w) != NULL_OFFSET) && !InView(w, FocusItem(w)))
    {
	/* turn off current focus_item, adjust it & make it focus'ed */

	if (PRIMPTR(w)->has_focus)
	    MakeUnfocused(FocusItem(w));
	
	/* set it to Top or Bottom */
	FocusItem(w) = AboveView(w, FocusItem(w)) ? Top(w) :
				OffsetOf(w, TopIndex(w) + ViewHeight(w) - 1);

	if (PRIMPTR(w)->has_focus)
	    MakeFocused(FocusItem(w));
    }
    Redisplay(w, (XEvent *)NULL, (Region)NULL);
}

/*****************************function*header****************************
   Search- 
*/
static void
Search OLARGLIST((w, ch))
    OLARG(ListPaneWidget,	w)
    OLGRA(char,			ch)
{
    int node;
    int cnt;

    DPRINT((stderr, "Searching for '%c'\n", ch));

    /* Interpret empty list as failed search */
    if (EmptyList(w))
    {
	_OlBeepDisplay((Widget)w, 1);
	return;
    }

    if ((SearchItem(w) == NULL_OFFSET) ||
      (IndexOf(SearchItem(w)) + 1 == NumItems(w)))
    {
	node = HeadOffset(w);

    } else {
	node = Next(w, SearchItem(w));
    }

    /* Loop thru all items.  This means that the current search_item (if
	any) will be visited again (as it should be.  It's as though
	selected item has been reselected).
    */
    for (cnt = NumItems(w); cnt > 0; cnt--)
    {
	if ((ItemPtr(node)->label != NULL) &&
	    (((char *)(ItemPtr(node)->label))[0] == ch))
	{
	    SearchItem(w) = node;	/* update search_item */
	    CurrentNotify(w, node);	/* call any callbacks */
	    return;
	}

	node = (IndexOf(node) + 1 == NumItems(w)) ?
					HeadOffset(w) : Next(w, node);
    }

    /* Falling thru means search failed */
    _OlBeepDisplay((Widget)w, 1);
}

/*****************************function*header****************************
   SelectOrAdjustItem- called when item is SELECTed or ADJUSTed (from
	mouse of keyboard)

     If editing, finish up by calling verification callback.  Then:

     1	consider selection ownership
     2	consider highlighting
     3	consider input focus
     4	call any callbacks

     +	starting place is always important
     +	selection ownership is only meaningful if we're selectable
     +	input focus is always set on SELECT (regardless of selectability)
*/
static void
SelectOrAdjustItem OLARGLIST((w, node, type, time))
    OLARG(ListPaneWidget,	w)
    OLARG(int,			node)
    OLARG(OlVirtualName,	type)
    OLGRA(Time,			time)
{
#define IsSELECT(type) ( (type) == OL_SELECT )
    DPRINT((stderr, "selected node= %s \n", ItemPtr(node)->label));
	
    if (PANEPTR(w)->editing && (IsSELECT(type) || PANEPTR(w)->selectable))
    {
	OlTextFieldVerify verify;
	Arg arg;

	XtSetArg(arg, XtNstring, &(verify.string));
	XtGetValues(TextField(w), &arg, 1);		/* get string */

	verify.ok = True;				/* although not used */

	XtCallCallbacks(TextField(w), XtNverification, &verify);
	XtFree(verify.string);				/* free string */
    }

    /* Consider selection ownership & highlighting:
	1st, only meaningful if selectable.
	Then, if this is the only highlighted item, don't reselect it.
	This prevents highlighting an already highlighted item.
    */
    if (PANEPTR(w)->selectable &&
      !(IsSELECT(type) && IsSelected(node) &&
	(PANEPTR(w)->items_selected == 1)))
    {
	/* consider selection-ownership */
	if (PANEPTR(w)->items_selected > 0)	/* ie. we own the selection */
	{
	    if (IsSELECT(type))			/* clears any/all others */
		ClearSelection(w);

	} else if (!PANEPTR(w)->own_selection) {
	    PANEPTR(w)->own_selection =
		XtOwnSelection((Widget)w, XA_PRIMARY, time,
		    ConvertSelection, LoseSelection, SelectionDone);
	}

	/* Item Highlighting:
	   test for ownership again in case attempt above fails
	*/
	if (PANEPTR(w)->own_selection)
	    ToggleItemState(w, node);
    }

    if (IsSELECT(type))				/* call any callbacks */
	CurrentNotify(w, node);

#undef IsSELECT
}

/*****************************function*header****************************
 * ToggleItemState- [un]highlights item & counts selected items.
 *		item state is toggled & select count updated accordingly
 *
 * 'node' is viewable when called from the Button event handler (since
 * they react to user interaction (ie. the items are in view).  But the
 * AutoScroll callback function desires to Toggle the state 1st, then make
 * the item viewable (then Redisplay) to reduce flashes.
 */
static void
ToggleItemState OLARGLIST((w, node))
    OLARG(ListPaneWidget,	w)
    OLGRA(int,			node)
{
    if (IsSelected(node))
    {
	ItemPtr(node)->attr &= ~OL_B_LIST_ATTR_SELECTED;
	PANEPTR(w)->items_selected--;

    } else {
	ItemPtr(node)->attr |= OL_B_LIST_ATTR_SELECTED;
	PANEPTR(w)->items_selected++;
    }

    if (InView(w, node))
	DrawItem(w, node);
}

/*****************************function*header****************************
   UpdateSBar- update scrollbar values.
    
    If view in unfilled, set scrollbar to insensitive.
    sliderMax = num of items (sliderMin is always default 0).
    sliderValue is "array" index of top item.
    proportionLength is smaller of view & number of item.
*/
static void
UpdateSBar OLARGLIST((w))
    OLGRA(ListPaneWidget, w)
{
    Arg		args[4];
    Cardinal	cnt = 0;

    if (NumItems(w) > ViewHeight(w))
    {
	XtSetArg(args[cnt], XtNsensitive, True); cnt++;
	XtSetArg(args[cnt], XtNproportionLength, ViewHeight(w)); cnt++;
	XtSetArg(args[cnt], XtNsliderMax, NumItems(w)); cnt++;
	XtSetArg(args[cnt], XtNsliderValue, TopIndex(w)); cnt++;

    } else {				/* Unfilled view: */
	XtSetArg(args[cnt], XtNsensitive, False); cnt++;
	if (EmptyList(w))
	{
	    XtSetArg(args[cnt], XtNproportionLength, 1); cnt++;
	    XtSetArg(args[cnt], XtNsliderMax, 1); cnt++;
	    XtSetArg(args[cnt], XtNsliderValue, 0); cnt++;
	} else {
	    XtSetArg(args[cnt], XtNproportionLength, NumItems(w)); cnt++;
	    XtSetArg(args[cnt], XtNsliderMax, NumItems(w)); cnt++;
	    XtSetArg(args[cnt], XtNsliderValue, TopIndex(w)); cnt++;
	}
    }

    XtSetValues(SBAR(w), args, cnt);
}

/*************************************************************************
 *
 * Class Procedures
 *
 */

/****************************function*header*******************************
    AcceptFocus- if editing, have TextField take focus.  Else, have
	superclass do the work.
*/
static Boolean
AcceptFocus OLARGLIST((w, time))
    OLARG( Widget,	w)
    OLGRA( Time *,	time)
{
    XtAcceptFocusProc accept_focus;

    /* If editing, have TextField take focus */
    if (PANEPTR(w)->editing)
	return ( XtCallAcceptFocus(TextField(w), time) );

    accept_focus =
	listPaneWidgetClass->core_class.superclass->core_class.accept_focus;

    return ( (accept_focus != NULL) && (*accept_focus)(w, time) );
} /* end of AcceptFocus */

/******************************function*header****************************
 * Destroy-
 */
static void
Destroy(w)
    Widget w;
{
    LPFreeGCs(w);
    FreeList(PANEW(w));

    /* Free clipboard store if we own it.
	(Xt disowns XA_CLIPBOARD  & XA_PRIMARY on our behalf)
    */
    if (PANEPTR(w)->own_clipboard)
    {
	XtFree(Clipboard);
	Clipboard = NULL;
	ClipboardCnt = 0;
    }

    /* shut off auto-scroll timer if it's set */
    if (PANEPTR(w)->timer_id)
	XtRemoveTimeOut(PANEPTR(w)->timer_id);
}

/******************************function*header****************************
 * HighlightHandler-
 */
static void
HighlightHandler OLARGLIST((w, type))
    OLARG(Widget,	w)
    OLGRA(OlDefine,	type)
{
    switch (type)
    {
    case OL_IN :

	if (EmptyList(w))
	    break;
	
	/* if no focus node, make it the top_item */
	if (FocusItem(w) == NULL_OFFSET)
	    FocusItem(w) = Top(w);

	/* highlight the focus_item and draw it */
	MakeFocused(FocusItem(w));
	DrawItem(PANEW(w), FocusItem(w));

	break;

    case OL_OUT :
	if ((FocusItem(w) != NULL_OFFSET) && IsFocused(FocusItem(w)))
	{
	    /* turn off highlighting and redraw item */
	    MakeUnfocused(FocusItem(w));
	    DrawItem(PANEW(w), FocusItem(w));
	}
	break;
    }
}

/******************************function*header****************************
 * Initialize-
 */
static void
Initialize(request, new, args, num_args)
    Widget	request;
    Widget	new;
    ArgList	args;
    Cardinal *	num_args;
{
    MaskArg	mArgs[10];
    ArgList	mergedArgs = NULL;
    Cardinal	mergedCnt;
    Cardinal	cnt;

    /* Can't easily specify default values for function pointers
	so initialize these here.
    */
    PANEPTR(new)->applAddItem		= ApplAddItem;
    PANEPTR(new)->applDeleteItem	= ApplDeleteItem;
    PANEPTR(new)->applEditClose		= ApplEditClose;
    PANEPTR(new)->applEditOpen		= ApplEditOpen;
    PANEPTR(new)->applTouchItem		= ApplTouchItem;
    PANEPTR(new)->applUpdateView	= ApplUpdateView;
    PANEPTR(new)->applViewItem		= ApplViewItem;

    /* CLIPBOARD / SELECTION / INPUT FOCUS / SEARCHING  RELATED */

    PANEPTR(new)->own_clipboard		= False;
							/* selection */
    PANEPTR(new)->own_selection		= False;
    PANEPTR(new)->items_selected	= 0;
    PANEPTR(new)->start_index		= INITIAL_INDEX;
    PANEPTR(new)->prev_index		= INITIAL_INDEX;
    PANEPTR(new)->initial_motion	= NO_MOTION;
							/* auto-scroll */
    PANEPTR(new)->scroll		= 0;
    PANEPTR(new)->repeat_rate		= 0;
    PANEPTR(new)->timer_id		= NULL;
							/* input focus */
    FocusItem(new)			= NULL_OFFSET;

							/* searching */
    SearchItem(new)			= NULL_OFFSET;

    /* COMPUTE ITEM SPACING IN PIXELS

	Offsets for drawing an item are fixed and specified in device-
	independent units.  They are converted to (device-dependent)
	pixel dimensions here.  Conversion is only needed once so is
	done here.
    */
    /* margin: space between pane and item */
    PANEPTR(new)->vert_margin	= ConvertVert(new, ITEM_MARGIN);
    PANEPTR(new)->horiz_margin	= ConvertHoriz(new, ITEM_MARGIN);

    /* padding: total vertical/horizontal padding surrounding label
     * pad + label width (height) = item width (height)
     */
    PANEPTR(new)->vert_pad	= ConvertVert(new, VERT_PADDING);
    PANEPTR(new)->horiz_pad	= ConvertHoriz(new, HORIZ_PADDING);

    /* WIDEST / TALLEST ITEM BOOKKEEPING */

    PANEPTR(new)->max_width	= FontWidth(PRIMPTR(new)->font); /* avg char */
    PANEPTR(new)->max_height	= FontHeight(PRIMPTR(new)->font);

    /* DRAWING RELATED */

    CreateGCs(PANEW(new));

    /* Can't use a window border for 3-D.  Remove it and adjust the sizes
     * to account for the wider, chiseled line.
     */
    if (OlgIs3d())
    {
	PANEPTR(new)->vert_margin +=
			OlgGetVerticalStroke(PANEPTR(new)->attr_normal);
	PANEPTR(new)->vert_pad +=
			2 * OlgGetVerticalStroke(PANEPTR(new)->attr_normal);
	PANEPTR(new)->horiz_margin +=
			OlgGetHorizontalStroke(PANEPTR(new)->attr_normal);
	PANEPTR(new)->horiz_pad +=
			2 * OlgGetHorizontalStroke(PANEPTR(new)->attr_normal);
	new->core.border_width = 0;
    }

    /* LIST RELATED */

    HEADPTR(new)->offset	= NULL_OFFSET;
    _OlArrayInit(&(HEADPTR(new)->offsets));	/* init array of offsets */
  
    /* VIEW RELATED */

    Top(new)			= NULL_OFFSET;
    PANEPTR(new)->editing	= False;
    PANEPTR(new)->update_view	= True;

    /* PANE WIDTH
     *
     * make initial width equal to the widest character in the font
     */
    if (new->core.width == 0)
	new->core.width = ItemWidth(new, PANEPTR(new)->max_width);

    /* PANE HEIGHT (and SBar height) */
    SetPaneHeight(PANEW(new));

    /* calibrate SBAR for empty list (after ViewHeight & NumItems set) */
    UpdateSBar(PANEW(new));

    /* Add callback to SBar */
    XtAddCallback(SBAR(new), XtNsliderMoved, SBarMovedCB, (XtPointer)new);

    /* create editable text field */
    cnt = 0;
    _OlSetMaskArg(mArgs[cnt], XtNfont, 0, OL_SOURCE_PAIR); cnt++;
    _OlSetMaskArg(mArgs[cnt], XtNfontColor, 0, OL_SOURCE_PAIR); cnt++;
    _OlSetMaskArg(mArgs[cnt], XtNforeground, 0, OL_SOURCE_PAIR); cnt++;
    _OlSetMaskArg(mArgs[cnt], XtNmaximumSize, 0, OL_SOURCE_PAIR); cnt++;
    _OlSetMaskArg(mArgs[cnt], XtNstring, 0, OL_SOURCE_PAIR); cnt++;
    _OlSetMaskArg(mArgs[cnt], XtNverification, 0, OL_SOURCE_PAIR); cnt++;
    _OlComposeArgList(args, *num_args, mArgs, cnt, &mergedArgs, &mergedCnt);
    TextField(new) = XtCreateWidget("textfield", textFieldWidgetClass,
				    SLISTW(new), mergedArgs, mergedCnt);
    XtFree((char *)mergedArgs);

    _OlDeleteDescendant(TextField(new)); /* delete it from traversal list */
}

/******************************function*header****************************
 * Redisplay-
 */
static void
Redisplay(w, event, region)
    Widget	w;
    XEvent *	event;
    Region	region;
{
    int	i;
    int	node;

    OlgDrawChiseledBox(XtScreen(w), XtWindow(w),
		PANEPTR(w)->attr_normal, 0, 0, w->core.width, w->core.height);

    /* check for unfilled view and empty list.  When last item is
	deleted, it must be cleared from the pane here
    */
    if (NumItems(w) < ViewHeight(w))
    {
	XClearArea(XtDisplay(w), XtWindow(w),
	    PANEPTR(w)->horiz_margin,				   /* x */
	    PANEPTR(w)->vert_margin + NumItems(w) * ItemHeight(w), /* y */
	    w->core.width - 2 * PANEPTR(w)->horiz_margin,	/* width */
	    ItemHeight(w) * (ViewHeight(w) - NumItems(w)),	/* height */
	    False);						/* expose's */

	if (EmptyList(w))
	    return;
    }

    node = Top(w);			/* start at Top of pane */
    for (i = _OlMin(ViewHeight(w), NumItems(w) - TopIndex(w)); i > 0; i--)
    {
	DrawItem(PANEW(w), node);
	node = Next(w, node);
    }
}

/******************************function*header****************************
 * RegisterFocus-
 */
static Widget
RegisterFocus OLARGLIST((w))
    OLGRA(Widget, w)
{
    return(SLISTW(w));
}

/******************************function*header****************************
 * SetValues-
 */
/* ARGSUSED */
static Boolean
SetValues(current, request, new, args, num_args)
    Widget current, request, new;
    ArgList	args;
    Cardinal *	num_args;
{
    Boolean	redisplay = False;

    if ((PANEPTR(new)->view_height != PANEPTR(current)->view_height) ||
      (new->core.height != current->core.height))
    {
	SetPaneHeight(PANEW(new));

	/* if height indeed changed, redisplay */
	if (new->core.height != current->core.height)
	    redisplay = True;
    }

    if ((new->core.background_pixel != current->core.background_pixel) ||
      (PRIMPTR(new)->font != PRIMPTR(current)->font) ||
      (PRIMPTR(new)->font_color != PRIMPTR(current)->font_color) ||
      (PRIMPTR(new)->foreground != PRIMPTR(current)->foreground) ||
      (PRIMPTR(new)->input_focus_color != PRIMPTR(current)->input_focus_color))
    {
	/* pass on changes to TextField */
	ArgList		mergedArgs;
	Cardinal	mergedCnt;

	_OlComposeArgList(args, *num_args, TFieldArgs, XtNumber(TFieldArgs),
		      &mergedArgs, &mergedCnt);

	if (mergedCnt != 0) {
	    XtSetValues(TextField(new), mergedArgs, mergedCnt);
	    XtFree((char *)mergedArgs);
	}

	/* re-create ListPane GC's */
	LPFreeGCs(new);
	CreateGCs(PANEW(new));

	/* If font changed, recompute max_height/width and set Pane height */
	if (PRIMPTR(new)->font != PRIMPTR(current)->font)
	{
	    PANEPTR(new)->max_height = FontHeight(PRIMPTR(new)->font);

	    /* currently can't do max_width */

	    SetPaneHeight(PANEW(new));
	}

	redisplay = True;
    }

    /* update_view must always be considered */
    return(redisplay && PANEPTR(new)->update_view);
}

/******************************function*header****************************
 * TraversalHandler-
 */
static Widget
TraversalHandler OLARGLIST((mgr, w, direction, time))
    OLARG(Widget,	mgr)		/* traversal manager (me) */
    OLARG(Widget,	w)		/* starting widget (me) */
    OLARG(OlVirtualName,direction)
    OLGRA(Time,		time)
{
    if (EmptyList(w))
	return(NULL);

    switch(direction)
    {
    case OL_MOVEUP :
    case OL_MOVEDOWN :
    case OL_MULTIUP :
    case OL_MULTIDOWN :
	Activate(w,
	    (direction == OL_MULTIUP) ? OL_PAGEUP :
	    (direction == OL_MULTIDOWN) ? OL_PAGEDOWN : direction,
	    NULL);
	break;

    default :
	break;
    }
    return(w);
}

/*************************************************************************
 *
 * Action Procedures
 *
 */

/*****************************function*header****************************
   Activate- call to activate widget "remotely"
*/
static Boolean
Activate OLARGLIST((w, name, data))
    OLARG(Widget,	w)
    OLARG(OlVirtualName,name)
    OLGRA(XtPointer,	data)
{
    Boolean	consumed;
    int		delta;

    if (!XtIsSensitive(w) || EmptyList(w))
	return (True);

    switch(name)
    {
    case OL_CUT :
    case OL_COPY :
	consumed = True;

	CutOrCopy(PANEW(w), (name == OL_CUT));
	break;

    case OL_PASTE :
	consumed = True;

	_OlBeepDisplay(w, 1);		/* can't paste into list */
	break;

    case OL_ADJUST :
    case OL_ADJUSTKEY :
    case OL_SELECT :
    case OL_SELECTKEY :
	consumed = True;

	/* FocusItem is not NULL since we have focus
	   Fire FocusItem or item passed as 'data' (mnemonic)
	*/
	SelectOrAdjustItem(PANEW(w),
	    (data == NULL) ? FocusItem(w) : (int)data,
	    (name == OL_SELECTKEY) ? OL_SELECT :
	    (name == OL_ADJUSTKEY) ? OL_ADJUST : name,
	    LatestTime(w));
	break;


    /* these involve scrolling the view (and potentially the focus_item): */
    case OL_SCROLLUP :
    case OL_SCROLLTOP :
    case OL_PAGEUP :
	consumed = True;

	if (Top(w) == HeadOffset(w))		/* can't back up any farther */
	    break;

	if (name == OL_SCROLLUP)
	{
	    Top(w) = Prev(w, Top(w));		/* update top_item */

	} else if (name == OL_SCROLLTOP) {
	    Top(w) = HeadOffset(w);		/* update top_item */

	} else {
	    delta = _OlMin(_OlMax(ViewHeight(w) - 1, 1), TopIndex(w));
	    Top(w) = OffsetNode(w, Top(w), -delta); /* update top_item */
	}

	ScrollView(PANEW(w));			/* update view */
	UpdateSBar(PANEW(w));			/* update scrollbar */
	break;


    case OL_SCROLLDOWN :
    case OL_SCROLLBOTTOM :
    case OL_PAGEDOWN :
	consumed = True;
	delta = AfterBottom(w);

	if (delta <= 0)			/* can't scroll forward any farther */
	    break;

	if (name == OL_SCROLLDOWN)
	{
	    Top(w) = Next(w, Top(w));			/* update top_item */
	
	} else if (name == OL_SCROLLBOTTOM) {
	    Top(w) = OffsetOf(w, NumItems(w) - ViewHeight(w)); /* update top */

	} else {
	    int		d;

	    d = _OlMin(_OlMax(ViewHeight(w) - 1, 1), delta);
	    Top(w) = OffsetNode(w, Top(w), d);
	}

	ScrollView(PANEW(w));			/* update view */
	UpdateSBar(PANEW(w));			/* update scrollbar */
	break;


    /* these involve moving the focus_item (and potentially the view): */
    case OL_MOVEUP :
    case OL_MOVEDOWN :
    case OL_PANESTART :
    case OL_PANEEND :
	consumed = True;

	/* disregard extremes */

	if (((name == OL_MOVEUP) && (FocusItem(w) == HeadOffset(w))) ||
	  ((name == OL_MOVEDOWN) &&
				(IndexOf(FocusItem(w)) + 1 == NumItems(w))) ||
	  ((name == OL_PANESTART) && (FocusItem(w) == Top(w))))
	{
	    break;

	} else if (name == OL_PANEEND) {
	    delta = _OlMin(TopIndex(w) + ViewHeight(w) - 1, NumItems(w) - 1);

	    if (FocusItem(w) == OffsetOf(w, delta))
		break;				/* can't go any farther down */
	}

	/* From here on it's all the same:
	   Unfocus the current focus_item and redraw it.
	   Establish new focus_item and redraw it or view it
	*/
	MakeUnfocused(FocusItem(w));
	DrawItem(PANEW(w), FocusItem(w));

	FocusItem(w) = (name == OL_MOVEUP) ? Prev(w, FocusItem(w)) :
			(name == OL_MOVEDOWN) ? Next(w, FocusItem(w)) :
			(name == OL_PANESTART) ? Top(w) :
			OffsetOf(w, delta);	/* OL_PANEEND */
	MakeFocused(FocusItem(w));

	if (InView(w, FocusItem(w)))
	{
	    DrawItem(PANEW(w), FocusItem(w));

	} else {
	    ApplViewItem(SLISTW(w), (OlListToken)FocusItem(w));
	}
	break;
	
    default :
	consumed = False;
	break;
    }

    return (consumed);
}

/*****************************function*header****************************
 * AutoScrollCB-
 */
static void
AutoScrollCB(closure, id)
    XtPointer		closure;
    XtIntervalId *	id;
{
    ListPaneWidget	w = PANEW((Widget)closure);

    if (!PANEPTR(w)->scroll)		/* no need to scroll */
    {
	PANEPTR(w)->timer_id = NULL;	/* indicates scrolling has stopped */
	return;
    }

    if (PANEPTR(w)->timer_id == NULL)	/* 1st time thru? */
    {
	Arg arg;
	int initial_delay;

	XtSetArg(arg, XtNinitialDelay, &initial_delay);
	XtGetValues(SBAR(w), &arg, 1);			/* set initial delay */

	PANEPTR(w)->timer_id = OlAddTimeOut(
				(Widget)w, initial_delay,
				AutoScrollCB, (XtPointer)w);

    } else {
	if (PANEPTR(w)->scroll == SCROLL_UP)
	{
	    if (Top(w) != HeadOffset(w))
	    {
		ToggleItemState(PANEW(w), Prev(w, Top(w)));	/* SELECT it */
		Activate((Widget)w, OL_SCROLLUP, NULL);		/* scroll up */
		PANEPTR(w)->prev_index--;
	    }

	} else if (AfterBottom(w) > 0) {
	    ToggleItemState(PANEW(w), OffsetOf(w, TopIndex(w) + ViewHeight(w)));
	    Activate((Widget)w, OL_SCROLLDOWN, NULL);	/* scroll down */
	    PANEPTR(w)->prev_index++;
	}

	if (PANEPTR(w)->repeat_rate == 0)
	{
	    Arg arg;
	    XtSetArg(arg, XtNrepeatRate, &(PANEPTR(w)->repeat_rate));
	    XtGetValues(SBAR(w), &arg, 1);
	}
	PANEPTR(w)->timer_id = OlAddTimeOut(
				(Widget)w, PANEPTR(w)->repeat_rate,
				AutoScrollCB, (XtPointer)w);
    }
}

/*****************************function*header****************************
   ButtonDown- handle buttonPress event.
	For SELECT or ADJUST see if event occurred over valid
	item and dispatch to SelectOrAdjustItem.
*/
static void
ButtonDown OLARGLIST((widget, ve))
    OLARG(Widget,		widget)
    OLGRA(OlVirtualEvent,	ve)
{
    ListPaneWidget	w = PANEW(widget);
    XButtonEvent *	buttonEvent;
    int			indx;

    if (!XtIsSensitive((Widget)w))
	return;

    switch(ve->virtual_name)
    {

    case OL_SELECT :
    case OL_ADJUST :
	ve->consumed = True;

	/* ignore presses when empty list */
	if (EmptyList(w))
	    break;

	buttonEvent = (XButtonEvent *) ve->xevent;

	/* ignore press in top or bottom margins */
	if ((buttonEvent->y < (Position)PANEPTR(w)->vert_margin) ||
	  (buttonEvent->y >=
			(Position)(w->core.height - PANEPTR(w)->vert_margin)))
	{
	    break;
	}

	/* compute index based on y offset in pane and index of top_item */
	indx = IndexFromY(w, buttonEvent->y) + TopIndex(w);

	/* Call SelectOrAdjustItem only if index in pane is valid; that
	   is, list may not extend down as far as where button was pressed
	   (ie. list does not fill pane and pointer is in whitespace below
	   last item).
	*/
	if (indx < NumItems(w))
	{
	    DPRINT((stderr, "index = %d \n", indx));

	    /* For SELECT, take focus if don't already have it. */
	    if ((ve->virtual_name == OL_SELECT) && !PRIMPTR(w)->has_focus)
		SetInputFocus((Widget)w, buttonEvent->time);

	    /* track values in case of motion */
	    PANEPTR(w)->start_index = indx;	/* must track starting pos */
	    PANEPTR(w)->prev_index = indx;	/* any motion started here */
	    PANEPTR(w)->initial_motion = NO_MOTION;	/* no motion yet */

	    SelectOrAdjustItem(PANEW(w), OffsetOf(w, indx),
				ve->virtual_name, buttonEvent->time);
	}

	break;

    default :
	break;
    }
}

/*****************************function*header****************************
   ButtonMotion- for wipe-thru selections & auto-scrolling

   dismiss OL_SELECT or OL_ADJUST outright if:
     *  not selectable OR
     *  motion w/o down-press (enter with button down) OR
     *  empty list
 */
static void
ButtonMotion OLARGLIST((widget, ve))
    OLARG(Widget,		widget)
    OLGRA(OlVirtualEvent,	ve)
{
#define Up		( delta < 0 )
#define Down		( delta > 0 )
#define WentUp(w)	( PANEPTR(w)->initial_motion < 0 )
#define WentDown(w)	( PANEPTR(w)->initial_motion > 0 )
#define ScrollUp(w)	( PANEPTR(w)->scroll == SCROLL_UP )
#define ScrollDown(w)	( PANEPTR(w)->scroll == SCROLL_DOWN )

    XPointerMovedEvent * motionEvent;
    ListPaneWidget	w = PANEW(widget);
    int			indx;
    int			delta;

    if (!XtIsSensitive((Widget)w))
	return;

    switch(ve->virtual_name)
    {
    case OL_SELECT :
    case OL_ADJUST :
	ve->consumed = True;

	if (EmptyList(w) || !PANEPTR(w)->selectable ||
	  (PANEPTR(w)->start_index == INITIAL_INDEX))
	    break;

	motionEvent = (XPointerMovedEvent *) ve->xevent;

	/* If pointer is above or below pane, consider auto-scrolling.
	   If scrolling (in same direction) is in progress, return.
	   Otherwise, compute indx of item under the pointer and continue
	   (selection will be wiped-thru and auto-scroll kicked off).
	*/
	if (motionEvent->y < (Position)PANEPTR(w)->vert_margin)
	{
	    if ((PANEPTR(w)->timer_id != NULL) &&
	      (PANEPTR(w)->scroll == SCROLL_UP))
	    {
		break;

	    } else {
		indx = TopIndex(w);
		PANEPTR(w)->scroll = (indx > 0) ?
		    SCROLL_UP :			/* scroll in items from top */
		    0;				/* no scrolling */
	    }

	} else if (motionEvent->y >=
	  (Position)(w->core.height - PANEPTR(w)->vert_margin)) {
	    if ((PANEPTR(w)->timer_id != NULL) &&
	      (PANEPTR(w)->scroll == SCROLL_DOWN))
	    {
		break;

	    } else {
		indx = TopIndex(w) + ViewHeight(w) - 1;

		/* If list does not fill view, pointer may be in whitespace
		   below end of list.  If so, adjust index to last item
		*/
		if (indx >= NumItems(w))
		{
		    indx = NumItems(w) - 1;
		    PANEPTR(w)->scroll = 0;	/* no scrolling */
		
		} else {
		    PANEPTR(w)->scroll = SCROLL_DOWN;
		}
	    }
	} else {				/* y is in pane */
	    PANEPTR(w)->scroll = 0;		/* no scrolling */

	    /* compute index based on y offset in pane and index of top_item */
	    indx = IndexFromY(w, motionEvent->y) + TopIndex(w);

	    /* If list does not fill view, pointer may be in whitespace
		below end of list.  If so, adjust index to last item
	    */
		if (indx >= NumItems(w))
		    indx = NumItems(w) - 1;
	}

	/* compute # of items between previous and current indices */
	delta = indx - PANEPTR(w)->prev_index;

	/* disregard "jitter": motion within item */
	if (delta != 0)
	{
	    int i, cnt;

	    DPRINT((stderr, "index = %d \n", indx));

	    /* set initial motion if not already set */
	    if (PANEPTR(w)->initial_motion == 0)
		PANEPTR(w)->initial_motion = delta;

	    /* Now toggle ([un]highlight) items.  The prev_index
		must be adjusted (not included) if:
		    we're above start_index & moving up -OR-
		    we're below start_index & moving down
	    */
	    if ((indx < PANEPTR(w)->start_index) && Up)
		i = PANEPTR(w)->prev_index - 1;

	    else if ((indx > PANEPTR(w)->start_index) && Down)
		i = PANEPTR(w)->prev_index + 1;
	
	    else
		i = PANEPTR(w)->prev_index;

	    for (cnt = (delta < 0) ? -delta : delta; cnt > 0; cnt--)
	    {
		/* if direction changed and crossing start_index, toggle it */
		if (((i == PANEPTR(w)->start_index + 1) && WentUp(w) && Down) ||
		    ((i == PANEPTR(w)->start_index - 1) && WentDown(w) && Up))
		{
		    ToggleItemState(w, OffsetOf(w, PANEPTR(w)->start_index));
		}

		ToggleItemState(w, OffsetOf(w, i));

		i = Up ? i - 1 : i + 1;
	    }

	    if ((indx == PANEPTR(w)->start_index) &&
		((WentUp(w) && Up) || (WentDown(w) && Down)))
	    {
		ToggleItemState(w, OffsetOf(w, PANEPTR(w)->start_index));
	
	    }
	    
	    PANEPTR(w)->prev_index = indx;	/* update previous index */
	}

	/* finally, Toggle Top or Bottom item if the start_index has
	   scrolled out of view, we've changed direction and
	    +	we're at the Top and scrolling up -OR-
	    +	at the bottom and scrolling down
	*/
	if (!InView(w, OffsetOf(w, PANEPTR(w)->start_index)) &&

	  /* we're at the Top and about to ScrollUp */
	  (((indx == TopIndex(w)) && WentDown(w) && ScrollUp(w)) ||

	  /* we're at the bottom and about to scroll down */
	  ((indx == _OlMin(TopIndex(w) + ViewHeight(w) - 1,
			NumItems(w) - 1)) && WentUp(w) && ScrollDown(w))))
	{
	    ToggleItemState(w, OffsetOf(w, indx));
	}

	/* if we need to scroll and we aren't already, then do so */
	if ((PANEPTR(w)->scroll != 0) && (PANEPTR(w)->timer_id == NULL))
	    AutoScrollCB((Widget)w, (XtIntervalId *)NULL);

	break;
    
    default :
	break;
    }

#undef Up
#undef Down
#undef WentUp
#undef WentDown
#undef ScrollUp
#undef ScrollDown
}

/*****************************function*header****************************
 * ButtonUp-
 */
static void
ButtonUp OLARGLIST((widget, ve))
    OLARG(Widget,		widget)
    OLGRA(OlVirtualEvent,	ve)
{
    ListPaneWidget	w = PANEW(widget);

    if (!XtIsSensitive((Widget)w) || EmptyList(w) || !PANEPTR(w)->selectable)
	return;

    switch(ve->virtual_name)
    {
    case OL_SELECT :
    case OL_ADJUST :
	PANEPTR(w)->start_index		= INITIAL_INDEX;
	PANEPTR(w)->initial_motion	= NO_MOTION;

	/* if auto-scrolling is going on, stop it */
	PANEPTR(w)->scroll		= 0;

	/* initial state: for 1st time thru auto-scroll */
	PANEPTR(w)->timer_id		= NULL;

	break;

    default :
	break;
    }
}

/*****************************function*header****************************
 * ConvertSelection-
 */
static Boolean
ConvertSelection(w, selection, target, type_return, value_return,
		 length_return, format_return)
    Widget		w;
    Atom *		selection;
    Atom *		target;
    Atom *		type_return;
    XtPointer *		value_return;
    unsigned long *	length_return;
    int *		format_return;
{
    Boolean		status;

    if (*selection == XA_PRIMARY)
    {
	Display *	dpy = XtDisplay(w);
	Atom		stuff;

	if ((*target == XA_OL_COPY(dpy)) ||
	    (*target  == (stuff = XA_OL_CUT(dpy)))) {

	    CutOrCopy(PANEW(w), (*target == stuff));

	    *format_return	= 0;
	    *length_return	= NULL;
	    *value_return	= NULL;
	    *type_return	= *target;
	    status		= False;


	} else if (*target == XA_STRING) {
	    *format_return	= 8;
	    *length_return	= ClipboardCnt;
	    status		= (ClipboardCnt > 0);
	    *value_return	= (status) ? Clipboard : NULL;
	    *type_return	= XA_STRING;

	} else {
	    status		= False;

		OlVaDisplayWarningMsg(	XtDisplay(w),
					OleNfileListPane,
					OleTmsg9,
					OleCOlToolkitWarning,
					OleMfileListPane_msg9);
	}

    } else if (*selection == XA_CLIPBOARD(XtDisplay(w))) {
	if (*target == XA_STRING) {
	    *format_return	= 8;
	    *length_return	= ClipboardCnt;
	    status		= (ClipboardCnt > 0);
	    *value_return	= (status) ? Clipboard : NULL;
	    *type_return	= XA_STRING;

	} else {
	    status		= False;

		OlVaDisplayWarningMsg(	XtDisplay(w),
					OleNfileListPane,
					OleTmsg10,
					OleCOlToolkitWarning,
					OleMfileListPane_msg10);
	}

    } else {
	status			= False;

	OlVaDisplayWarningMsg(	XtDisplay(w),
				OleNfileListPane,
				OleTmsg11,
				OleCOlToolkitWarning,
				OleMfileListPane_msg11);
    }
    return(status);
}

/*****************************function*header****************************
   KeyDown- perform key-search.

   Only interested in non-Open Look keys (UNKNOWN_KEY_INPUT) that are
   unmodified and are not mnemonics or accelerators.  Cannot assume anything
   about where char is printable so don't use isprint.
*/
static void
KeyDown OLARGLIST((w, ve))
    OLARG(Widget,		w)
    OLGRA(OlVirtualEvent,	ve)
{
    if (!XtIsSensitive(w))
	return;

    switch(ve->virtual_name)
    {
    case OL_UNKNOWN_KEY_INPUT :
	/* interested in un-modified keys (Modifier press results in
	   length == 0).  Check for mnemonics & accelerators, too.
	*/
	if (ve->length != 0 && !(((XKeyEvent *)(ve->xevent))->state &
	  (Mod1Mask | Mod2Mask | Mod3Mask | Mod4Mask | Mod5Mask)) &&
	  (_OlFetchMnemonicOwner(w, NULL, ve) == NULL) &&
	  (_OlFetchAcceleratorOwner(w, NULL, ve) == NULL))
	{
	    ve->consumed = True;

	    Search(PANEW(w), ve->buffer[0]);
	}

	break;

    default:
	break;
    }
}

/****************************procedure*header*****************************
 *  LoseSelection - 
 */
static void
LoseSelection(w, selection)
    Widget	w;
    Atom *	selection;
{
    if (*selection == XA_CLIPBOARD(XtDisplay(w)))
    {
	PANEPTR(w)->own_clipboard = False;

    } else if (*selection == XA_PRIMARY) {
	ClearSelection(PANEW(w));
	PANEPTR(w)->own_selection = False;
    }
}

/*****************************function*header****************************
    SelectionDone- this is registered for Clipboard ownership.  When a
		SelectionDoneProc is registered, the selection owner
	owns the memory allocated for the selection.  Otherwise, new memory
	must be allocated which the Intrinsics frees on owners behalf.
	This is undesirable so SelectionDone is registered for the sole
	purpose of being able to own the clipboard memory (Clipboard).
*/
static void
SelectionDone(w, selection, target)
    Widget	w;
    Atom *	selection;
    Atom *	target;
{
}

/*****************************function*header****************************
 * SBarMovedCB-
 */
static void
SBarMovedCB(sbw, closure, callData)
    Widget	sbw;			/* scrollbar widget */
    XtPointer	closure;
    XtPointer	callData;
{
    OlScrollbarVerify * verifyData = (OlScrollbarVerify *)callData;
    ListPaneWidget	w;		/* pane widget */
    int			new_index;

    if (verifyData->ok == False)
	return;

    w = (ListPaneWidget)closure;

    new_index = verifyData->new_location;

    DPRINT((stderr, "SBarMoved- old= %d, new= %d \n", TopIndex(w), new_index));

    if ((new_index == TopIndex(w)) ||			/* no change */
      (new_index < 0) || (new_index >= NumItems(w)))	/* bad values */
	return;

    Top(w) = OffsetOf(w, new_index);	/* update top_item */
    ScrollView(PANEW(w));		/* update view */
}

/*************************************************************************
 *
 * Public Procedures
 *
 */

OlListItem *
OlListItemPointer OLARGLIST((token))
    OLGRA(OlListToken, token)
{
    int indx = (int)token;

    if ((indx < 1) || (indx >= TheList.num_slots))
    {
	OlVaDisplayWarningMsg(	(Display *) NULL,
				OleNfileListPane,
				OleTmsg12,
				OleCOlToolkitWarning,
				OleMfileListPane_msg12);
	return (NULL);
    }

    return ( ItemPtr(indx) );
}

#ifdef DEBUG
#include <stdio.h>

void
_OlPrintList(slw)
    Widget slw;
{
    Cardinal	i;
    Widget	w;

    if (slw == NULL)
    {
	OlVaDisplayWarningMsg(	(Display *)NULL,
				OleNfileListPane,
				OleTmsg13,
				OleCOlToolkitWarning,
				OleMfileListPane_msg13);
	return;
    }

    w = _OlListPane(slw);

    if (EmptyList(w))
    {
	OlVaDisplayWarningMsg(	XtDisplay(slw),
				OleNfileListPane,
				OleTmsg14,
				OleCOlToolkitWarning,
				OleMfileListPane_msg14);
	return;
    }

    fprintf(stderr, "index\toffset\tlabel\n");

    for (i = 0; i < NumItems(w); i++)
	fprintf(stderr, "%d\t%d,\t%s\n", i, OffsetOf(w, i),
					ItemPtr(OffsetOf(w, i))->label);
    fprintf(stderr, "NumItems= %d, TopIndex= %d\n", NumItems(w), TopIndex(w));
}
#endif /* DEBUG */
