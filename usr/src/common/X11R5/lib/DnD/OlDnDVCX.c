/*
 *      Copyright (C) 1986,1992  Sun Microsystems, Inc
 *                      All rights reserved.
 *              Notice of copyright on this source code
 *              product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *      Sun Microsystems, Inc., 2550 Garcia Avenue,
 *      Mountain View, California 94043.
 *
 */

#ifndef NOIDENT
#ident	"@(#)oldnd:OlDnDVCX.c	1.23"
#endif

#include <stdio.h>
#include <memory.h>
#include <search.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/ShellP.h>
#include <X11/Xatom.h>

#if !defined(USL)
#define USL
#endif

#ifdef GUI_DEP

#include <OpenLookP.h>
#include <Error.h>
#include <VendorI.h>
#include <OlDnDVCXP.h> /* class extension for new drag n drop stuff */

#define GetShellOfWidget(w)	_OlGetShellOfWidget(w)

#else	/* GUI_DEP	*/

#include <DnD/OlDnDVCXP.h> /* class extension for new drag n drop stuff */

#define GetShellOfWidget(w)	DnDGetShellOfWidget(w)

static OlDnDClassExtensionHdrPtr
			DnDGetClassExtension OL_ARGS((
				OlDnDClassExtensionHdrPtr, XrmQuark, long));

static Widget		DnDGetShellOfWidget OL_ARGS((Widget));

#endif	/* GUI_DEP	*/

/* borrowed from Vendor.c */

#define CLASSNAME(w)    (XtClass((Widget)(w))->core_class.class_name)
#define VCLASS(wc, r)   (((VendorShellWidgetClass)(wc))->vendor_shell_class.r)

#define NULL_EXTENSION          ((OlDnDVendorClassExtension)NULL)
#define NULL_PART_PTR           ((OlDnDVendorPartExtension*)NULL)
#define NULL_PART               ((OlDnDVendorPartExtension)NULL)

/*************************** forward decls for Drag and Drop **************/

/* public procedures */

/* class methods */

static void 	DnDVCXClassInitialize OL_ARGS((OlDnDVendorClassExtension));
static void	DnDVCXClassPartInitialize OL_ARGS(( WidgetClass ));

static Boolean  DnDVCXSetValues OL_ARGS((Widget, Widget, Widget,
					 ArgList, Cardinal *,
					 OlDnDVendorPartExtension,
					 OlDnDVendorPartExtension,
					 OlDnDVendorPartExtension
));

static void     DnDVCXGetValues OL_ARGS((Widget, ArgList, Cardinal *,
					 OlDnDVendorPartExtension
));

static void     DnDVCXInitialize OL_ARGS((Widget, Widget, ArgList, Cardinal *,
					  OlDnDVendorPartExtension,
					  OlDnDVendorPartExtension
));

static void     DnDVCXPostRealizeSetup OL_ARGS(( Widget,
                                                 OlDnDVendorClassExtension,
                                                 OlDnDVendorPartExtension 
));

static void     DnDVCXDestroy   OL_ARGS((Widget, OlDnDVendorPartExtension
));
static Boolean	DnDVCXTriggerMessageDispatcher OL_ARGS(( Widget,
						 OlDnDVendorPartExtension,
						 TriggerMessagePtr
));

static Boolean	DnDVCXPreviewMessageDispatcher OL_ARGS(( Widget,
						 OlDnDVendorPartExtension,
						 PreviewMessagePtr
));

static OlDnDDropSiteID DnDVCXRegisterDropSite OL_ARGS(( Widget,
						OlDnDVendorPartExtension,
						Widget,
						Window,
						OlDnDSitePreviewHints,
						OlDnDSiteRectPtr,
						unsigned int,
						OlDnDTriggerMessageNotifyProc,
						OlDnDPreviewMessageNotifyProc,
						Boolean,
						XtPointer
));

static Boolean	DnDVCXUpdateDropSiteGeometry OL_ARGS(( Widget,
						OlDnDVendorPartExtension,
						OlDnDDropSiteID,
						OlDnDSiteRectPtr,
						unsigned int
));

static void    DnDVCXDeleteDropSite OL_ARGS((Widget, OlDnDVendorPartExtension,
					     OlDnDDropSiteID
));

static Boolean DnDVCXQueryDropSiteInfo OL_ARGS(( Widget,
						 OlDnDVendorPartExtension,
						 OlDnDDropSiteID,
						 Widget *,
						 Window *,
						 OlDnDSitePreviewHints *,
						 OlDnDSiteRectPtr *,
						 unsigned int *,
						 Boolean *
));

static void	DnDVCXAssertDropSiteRegistry OL_ARGS(( Widget, 
					       OlDnDVendorPartExtension));

static void	DnDVCXDeleteDropSiteRegistry OL_ARGS(( Widget,
					       OlDnDVendorPartExtension
));

static Boolean	DnDVCXFetchDSDMInfo OL_ARGS(( Widget, OlDnDVendorPartExtension,
					      Time ));

static Boolean	DnDVCXDeliverTriggerMessage OL_ARGS (( Widget, 
					       OlDnDVendorPartExtension,
					       Widget, Window,
					       int, int, Atom,
					       OlDnDTriggerOperation, Time
));

static Boolean	DnDVCXDeliverPreviewMessage OL_ARGS (( Widget, 
					       OlDnDVendorPartExtension,
					       Widget, Window, int, int,
					       Time,
					       OlDnDPreviewAnimateCallbackProc,
					       XtPointer
));

static Boolean	DnDVCXInitializeDragState OL_ARGS (( Widget,
						     OlDnDVendorPartExtension
));

static void	DnDVCXClearDragState   	    OL_ARGS (( Widget,
						       OlDnDVendorPartExtension ));

static Atom	DnDVCXAllocTransientAtom    OL_ARGS (( Widget,
                                                       OlDnDVendorPartExtension,
						       Widget
));

static void	DnDVCXFreeTransientAtom    OL_ARGS (( Widget,
                                                      OlDnDVendorPartExtension,
						      Widget,
						      Atom
));

static DSSelectionAtomPtr DnDVCXAssocSelectionWithWidget OL_ARGS (( Widget,
					              OlDnDVendorPartExtension,
                                                      Widget,
                                                      Atom,
						      Time,
						      OwnerProcClosurePtr
));

static void	DnDVCXDissassocSelectionWithWidget OL_ARGS (( Widget,
					              OlDnDVendorPartExtension,
						      Widget,
                                                      Atom,
						      Time
));

static Atom     *DnDVCXGetCurrentSelectionsForWidget
			OL_ARGS (( Widget,
				   OlDnDVendorPartExtension,
				   Widget, Cardinal *
));

static Widget DnDVCXGetWidgetForSelection
				OL_ARGS (( Widget, OlDnDVendorPartExtension,
					   Atom, OwnerProcClosurePtr *
));

static Boolean DnDVCXChangeSitePreviewHints OL_ARGS (( Widget, 
					               OlDnDVendorPartExtension,
					               OlDnDDropSiteID,
					               OlDnDSitePreviewHints ));

static Boolean DnDVCXSetDropSiteOnInterest OL_ARGS (( Widget,
						    OlDnDVendorPartExtension,
						    OlDnDDropSiteID,
						    Boolean, Boolean ));

static void DnDVCXSetInterestInWidgetHier
			OL_ARGS (( Widget, OlDnDVendorPartExtension,
				   Widget, Boolean ));

/* event handlers */

static void     DnDVCXEventHandler();   /* drag and drop handler */

/* private functions */

static DSSelectionAtomPtr _OlDnDAssocSelectionWithWidget
					OL_ARGS(( Widget,
						  OlDnDVendorPartExtension,
						  Widget,
						  Atom,
						  Time,
						  OwnerProcClosurePtr
));

static void 		  _OlDnDDisassocSelectionWithWidget
					OL_ARGS(( Widget,
						  OlDnDVendorPartExtension,
						  Widget,
						  Atom,
						  Time
));

static Widget
_OlDnDGetWidgetForSelection OL_ARGS(( Widget,
				      OlDnDVendorPartExtension,
				      Atom, OwnerProcClosurePtr *
));

static Boolean
_OlDnDAtomIsTransient OL_ARGS (( Widget, OlDnDVendorPartExtension, Atom));

static void
_OlDnDResourceDependencies OL_ARGS (( XtResourceList *, Cardinal *,
				      XrmResourceList, Cardinal, Cardinal));

/*************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************/

#undef  OFFSET
#define OFFSET(field)   XtOffsetOf(OlDnDVendorPartExtensionRec, field)

static XtResource
dnd_ext_resources[] = {
    { XtNregistryUpdateTimestamp, XtCReadOnly, XtRInt, sizeof(Time),
        OFFSET(registry_update_timestamp), XtRImmediate, (XtPointer)NULL },

    { XtNnumberOfDropSites, XtCReadOnly, XtRInt, sizeof(int),
        OFFSET(number_of_sites), XtRImmediate, (XtPointer)NULL},

    { XtNrootX, XtCReadOnly, XtRPosition, sizeof(Position),
        OFFSET(root_x), XtRImmediate, (XtPointer)NULL},

    { XtNrootY, XtCReadOnly, XtRPosition, sizeof(Position),
        OFFSET(root_y), XtRImmediate, (XtPointer)NULL},

    { XtNautoAssertDropsiteRegistry, XtCAutoAssertDropsiteRegistry, XtRBoolean,
        sizeof(Boolean), OFFSET(auto_assert_dropsite_registry),
        XtRImmediate, (XtPointer)True},

    { XtNdirty, XtCReadOnly, XtRBoolean, sizeof(Boolean),
        OFFSET(dirty), XtRImmediate, (XtPointer)False},

    { XtNpendingDSDMInfo, XtCReadOnly, XtRBoolean, sizeof(Boolean),
        OFFSET(pending_dsdm_info), XtRImmediate, (XtPointer)False},

    { XtNdsdmPresent, XtCReadOnly, XtRBoolean, sizeof(Boolean),
        OFFSET(dsdm_present), XtRImmediate, (XtPointer)False},

    { XtNdoingDrag, XtCReadOnly, XtRBoolean, sizeof(Boolean),
        OFFSET(doing_drag), XtRImmediate, (XtPointer)False},

    { XtNdsdmLastLoaded, XtCReadOnly, XtRInt, sizeof(Time),
        OFFSET(dsdm_last_loaded), XtRImmediate, (XtPointer)NULL},

    { XtNpreviewForwardedSites, XtCPreviewForwardedSites, XtRBoolean,
        sizeof(Boolean), OFFSET(doing_drag), XtRImmediate, (XtPointer)False},

    { XtNdefaultDropSiteID, XtCDropSiteID, XtRPointer, 
	sizeof(OlDnDDropSiteID), OFFSET(default_drop_site), XtRImmediate,
	(XtPointer)NULL},
};
#undef  OFFSET

/*
 *************************************************************************
 *
 * Define Class Extension Records
 *
 *************************class*extension*records*************************
 */

/*static*/ /* we cant make it so since we initialize the extension chain
	    * at compile time in Vendor.c
	    */
OlDnDVendorClassExtensionRec dnd_vendor_extension_rec = {
	{
		NULL,					/* next_extension*/
		NULLQUARK,				/* record_type	*/
		OlDnDVendorClassExtensionVersion,	/* version	*/
		sizeof(OlDnDVendorClassExtensionRec)	/* record_size	*/
	},	/* End of OlClassExtensionHdr header */

	(Cardinal)sizeof(OlDnDVendorPartExtensionRec),	/* instance size */
	(OlDnDVendorPartExtension)NULL,			/* instance list */
	dnd_ext_resources,				/* resources 	 */
	XtNumber(dnd_ext_resources),			/* num_resources */

	DnDVCXClassInitialize,				/* class init    */
	DnDVCXClassPartInitialize,			/* class part    */
	(XtEnum)NULL,					/* inited 	 */
	DnDVCXSetValues,				/* set_values    */
	DnDVCXGetValues,				/* get_values	 */
	DnDVCXInitialize,				/* initialize	 */
	DnDVCXPostRealizeSetup,				/* post realize  */
	DnDVCXDestroy,					/* destroy 	 */

	DnDVCXTriggerMessageDispatcher,			/* TM dispatcher */
	DnDVCXPreviewMessageDispatcher,			/* PM dispatcher */

	DnDVCXRegisterDropSite,				/* register DS   */
	DnDVCXUpdateDropSiteGeometry,			/* update DS     */
	DnDVCXDeleteDropSite,				/* delete DS     */
	DnDVCXQueryDropSiteInfo,			/* query DS      */

	DnDVCXAssertDropSiteRegistry,			/* assert registry */
	DnDVCXDeleteDropSiteRegistry,			/* delete registry */

	DnDVCXFetchDSDMInfo,				/* fetch dsdm info */

	DnDVCXDeliverTriggerMessage,			/* deliver trigger */
	DnDVCXDeliverPreviewMessage,			/* deliver preview */

	DnDVCXInitializeDragState,			/* init drag state */
	DnDVCXClearDragState,				/* clear drag state */

	DnDVCXAllocTransientAtom,			/* alloc atom */
	DnDVCXFreeTransientAtom,			/* free atom  */

	DnDVCXAssocSelectionWithWidget,			/* assoc */
	DnDVCXDissassocSelectionWithWidget,		/* dissasoc */
	DnDVCXGetCurrentSelectionsForWidget,		/* list selections */
	DnDVCXGetWidgetForSelection,			/* find the widget */

	DnDVCXChangeSitePreviewHints,			/* change hints */

	DnDVCXSetDropSiteOnInterest,			/* set interest */
	DnDVCXSetInterestInWidgetHier,			/* set interest */
};

OlDnDVendorClassExtension  dnd_vendor_extension = &dnd_vendor_extension_rec;

XrmQuark OlXrmDnDVendorClassExtension = (XrmQuark)NULL;


Atom	_SUN_DRAGDROP_BEGIN;
Atom	_SUN_SELECTION_END;
Atom	_SUN_SELECTION_ERROR;

Atom	_SUN_AVAILABLE_TYPES;

Atom	_SUN_LOAD;
Atom	_SUN_DATA_LABEL;
Atom	_SUN_FILE_HOST_NAME;

Atom	_SUN_ENUMERATION_COUNT;
Atom	_SUN_ENUMERATION_ITEM;

Atom	_SUN_ALTERNATE_TRANSPORT_METHODS;
Atom	_SUN_LENGTH_TYPE;
Atom	_SUN_ATM_TOOL_TALK;
Atom	_SUN_ATM_FILE_NAME;

Atom	_SUN_DRAGDROP_INTEREST;
Atom	_SUN_DRAGDROP_PREVIEW;
Atom	_SUN_DRAGDROP_TRIGGER;
Atom	_SUN_DRAGDROP_DSDM;
Atom	_SUN_DRAGDROP_SITE_RECTS;
Atom	_SUN_DRAGDROP_ACK;
Atom	_SUN_DRAGDROP_DONE;

/************************************ public functions **********************/

/* internal toolkit functions follow */

/* note side effect */
#define	FetchPartExtensionIfNull(w, p)		\
	((p) != NULL_PART ? (p) : ((p) = _OlGetDnDVendorPartExtension((w))))

/* note side effect */
#define	FetchClassExtensionIfNull(w, c)		\
	((c) != NULL_EXTENSION ? (c) : 		\
	 ((c) = _OlGetDnDVendorClassExtension((w)->core.widget_class)))

/*******************************************
 *
 * _OlDnDInitialize
 *
 * called from InitializeOpenLook
 *
 * Initializes the appropriate atoms etc
 *
 *******************************************/

void
OlDnDInitialize OLARGLIST(( dpy ))
			OLGRA( Display,	*dpy)
{
#define	INTERN(dpy, atom)	XInternAtom(dpy, atom, False)

	_SUN_DRAGDROP_BEGIN	= INTERN(dpy, _SUN_DRAGDROP_BEGIN_NAME);
	_SUN_SELECTION_END	= INTERN(dpy, _SUN_SELECTION_END_NAME);
	_SUN_SELECTION_ERROR	= INTERN(dpy, _SUN_SELECTION_ERROR_NAME);

	_SUN_AVAILABLE_TYPES	= INTERN(dpy, _SUN_AVAILABLE_TYPES_NAME);

	_SUN_LOAD		= INTERN(dpy, _SUN_LOAD_NAME);
	_SUN_DATA_LABEL		= INTERN(dpy, _SUN_DATA_LABEL_NAME);
	_SUN_FILE_HOST_NAME	= INTERN(dpy, _SUN_FILE_HOST_NAME_NAME);

	_SUN_ENUMERATION_COUNT	= INTERN(dpy, _SUN_ENUMERATION_COUNT_NAME);
	_SUN_ENUMERATION_ITEM	= INTERN(dpy, _SUN_ENUMERATION_ITEM_NAME);

	_SUN_LENGTH_TYPE	= INTERN(dpy, _SUN_LENGTH_TYPE_NAME);
	_SUN_ATM_TOOL_TALK	= INTERN(dpy, _SUN_ATM_TOOL_TALK_NAME);
	_SUN_ATM_FILE_NAME	= INTERN(dpy, _SUN_ATM_FILE_NAME_NAME);

	_SUN_DRAGDROP_INTEREST	= INTERN(dpy, _SUN_DRAGDROP_INTEREST_NAME);
	_SUN_DRAGDROP_PREVIEW	= INTERN(dpy, _SUN_DRAGDROP_PREVIEW_NAME);
	_SUN_DRAGDROP_TRIGGER	= INTERN(dpy, _SUN_DRAGDROP_TRIGGER_NAME);
	_SUN_DRAGDROP_DSDM	= INTERN(dpy, _SUN_DRAGDROP_DSDM_NAME);

	_SUN_DRAGDROP_SITE_RECTS = INTERN(dpy, _SUN_DRAGDROP_SITE_RECTS_NAME);
	_SUN_DRAGDROP_ACK        = INTERN(dpy, _SUN_DRAGDROP_ACK_NAME);
	_SUN_DRAGDROP_DONE       = INTERN(dpy, _SUN_DRAGDROP_DONE_NAME);

	_SUN_ALTERNATE_TRANSPORT_METHODS =
		INTERN(dpy, _SUN_ALTERNATE_TRANSPORT_METHODS_NAME);
#undef	INTERN
}

/*******************************************
 *
 * _OlDnDDoExtensionClassInit
 *
 * called from VendorShell ClassInitialize
 *
 * calls the Class Initialize proc for ext
 *
 *******************************************/

void
_OlDnDDoExtensionClassInit OLARGLIST ((extension))
	OLGRA( OlDnDVendorClassExtension, extension )
{
	if (extension->class_inited == (XtEnum)NULL &&
	    extension->class_initialize != (OlDnDVCXClassInitializeProc)NULL) {
		(*extension->class_initialize)(extension);
		extension->class_inited = True;
	}
}

/*******************************************
 *
 * _OlDnDDoExtensionClassPartInit
 *
 * called from VendorShell ClassInitialize
 *
 * calls the Clas Part Initialize proc for ext
 *******************************************/

static void
_recurse_ext_part_init OLARGLIST (( wc , ancestor ))
			OLARG(WidgetClass, 	wc)
			OLGRA(WidgetClass,	ancestor)
{
	OlDnDVendorClassExtension	ext;

	if (ancestor != (WidgetClass)NULL &&
	    ancestor != vendorShellWidgetClass)
		_recurse_ext_part_init(wc, ancestor->core_class.superclass);

	ext = _OlGetDnDVendorClassExtension(ancestor);

	if (ext != (OlDnDVendorClassExtension)NULL &&
	    ext->class_part_initialize != (OlDnDVCXClassPartInitializeProc)NULL)
		(*ext->class_part_initialize)(wc);
	
}
void
_OlDnDDoExtensionClassPartInit OLARGLIST((wc))
	OLGRA(WidgetClass,	wc)
{
	_recurse_ext_part_init(wc, wc);
}

/*******************************************
 *
 * _OlDnDCallDnDVCXPostRealizeSetup
 *
 * called from VendorShell ClassInitialize
 *
 * should be called from vendor realize
 * after window is created ....
 *
 *******************************************/

void
_OlDnDCallVCXPostRealizeSetup OLARGLIST(( vendor ))
	OLGRA(Widget,	vendor)
{
	OlDnDVendorPartExtension	dnd_part = NULL_PART;
	OlDnDVendorClassExtension	dnd_class = NULL_EXTENSION;

	FetchPartExtensionIfNull(vendor, dnd_part);
	FetchClassExtensionIfNull(vendor, dnd_class);

	if ((dnd_class != NULL_EXTENSION && dnd_class->post_realize_setup != 
	    (OlDnDVCXPostRealizeSetupProc)NULL) && dnd_part != NULL_PART)
		(*dnd_class->post_realize_setup)(vendor, dnd_class, dnd_part);
}

/**************************************************
 *
 * _OlGetDnDVendorClassExtension
 *
 * fetch the drag and drop class extension record
 *
 **************************************************/

OlDnDVendorClassExtension
_OlGetDnDVendorClassExtension OLARGLIST((wc))
	OLGRA( WidgetClass,	wc)		/* Vendor subclass	*/
{
	OlDnDVendorClassExtension	ext = NULL_EXTENSION;

	if (!wc) {
#if 0
		OlWarning(
		 "_OlGetDnDVendorClassExtension: NULL WidgetClass pointer");
#else
		XtWarning(
		 "_OlGetDnDVendorClassExtension: NULL WidgetClass pointer");
#endif
	} else {
		ext = GET_DND_EXT(wc);
	}
	return(ext);
} /* END OF _OlGetDnDVendorClassExtension() */

/**************************************************
 *
 * _OlGetDnDVendorPartExtension
 *
 * fetch the drag and drop part extension record
 *
 **************************************************/

OlDnDVendorPartExtension
_OlGetDnDVendorPartExtension OLARGLIST((w))
	OLGRA( Widget,	w)			/* Vendor subclass	*/
{
	OlDnDVendorPartExtension	part = NULL_PART;

	if (w == (Widget)NULL) {
#if 0
		OlWarning("_OlGetDnDVendorPartExtension: NULL widget");
#else
		XtWarning("_OlGetDnDVendorPartExtension: NULL widget");
#endif
	} else if (XtIsVendorShell(w) == True) {
		OlDnDVendorClassExtension	extension =
							GET_DND_EXT(XtClass(w));
		OlDnDVendorPartExtension	*part_ptr = 
					        	&extension->instance_part_list;

		for (part = *part_ptr; part != NULL_PART && w != part->owner;)
		{
			part_ptr	= &part->next_part;
			part		= *part_ptr;
		}
	}
	return(part);
} /* END OF _OlDnDGetVendorPartExtension() */

/**************************************************
 *
 * CallDnDVCXExtensionMethods
 *
 * call chained the class part extension procs ..
 *
 **************************************************/

static Boolean
_CallExtensionMethods OLARGLIST((extension_method_to_call, wc, current,
				 request, new, args, num_args, cur_part,
				 req_part, new_part))
	OLARG( DnDVCXMethodType,		extension_method_to_call)
	OLARG( WidgetClass,			wc)
	OLARG( Widget,				current)
	OLARG( Widget,				request)
	OLARG( Widget,				new)
	OLARG( ArgList,				args)
	OLARG( Cardinal *,			num_args)
	OLARG( OlDnDVendorPartExtension,	cur_part)
	OLARG( OlDnDVendorPartExtension,	req_part)
	OLGRA( OlDnDVendorPartExtension,	new_part)
{
	Boolean				ret_val = False;
	OlDnDVendorClassExtension	ext = GET_DND_EXT(wc);

	if (wc != vendorShellWidgetClass)
	{
	    if (_CallExtensionMethods(extension_method_to_call,
				      wc->core_class.superclass,
				      current, request, new,
				      args, num_args,
				      cur_part, req_part, 
				      new_part) == TRUE)
	    {
		ret_val = TRUE;
	    }
	}

	if (ext != NULL_EXTENSION) {
	    switch (extension_method_to_call)
	    {
	    case CallDnDVCXInitialize :
		if (ext->initialize != (OlDnDVCXInitializeProc)NULL)
		{
		    (*ext->initialize)(request, new,
				args, num_args, req_part, new_part);
		}
		break;
	
	    case CallDnDVCXDestroy :
		if (ext->destroy != (OlDnDVCXDestroyProc)NULL)
		{
		    (*ext->destroy)(current, cur_part);
		}
		break;

	    case CallDnDVCXSetValues :
		if (ext->set_values != (OlDnDVCXSetValuesFunc)NULL)
		{
			if ((*ext->set_values)(current, request, new,
				args, num_args, cur_part, req_part, new_part)
			     == TRUE)
			{
				ret_val = TRUE;
			}
		}
		break;

	    case CallDnDVCXGetValues :
		if (ext->get_values != (OlDnDVCXGetValuesProc)NULL)
		{
		    (*ext->get_values)(current, args, num_args, cur_part);
		}
		break;

	    } /* end of switch */
	}

	return (ret_val);
}

Boolean
CallDnDVCXExtensionMethods OLARGLIST((extension_method_to_call, wc,
				      current, request, new, args, num_args))
	OLARG( DnDVCXMethodType,	extension_method_to_call)
	OLARG( WidgetClass,		wc)
	OLARG( Widget,			current)
	OLARG( Widget,			request)
	OLARG( Widget,			new)
	OLARG( ArgList,			args)
	OLGRA( Cardinal *,		num_args)
{
	Boolean				ret_val = False;
	WidgetClass			original_wc = wc;
	OlDnDVendorClassExtension	ext = GET_DND_EXT(wc);

	OlDnDVendorPartExtension	cur_part = NULL_PART,
					new_part = NULL_PART,
					req_part = NULL_PART,
					part;
	OlDnDVendorPartExtensionRec	cur, req;
	unsigned int			size;

	while (ext == NULL_EXTENSION && wc != vendorShellWidgetClass) {
		wc = wc->core_class.superclass;
		ext = GET_DND_EXT(wc);
	}

	if (ext == NULL_EXTENSION)
		return (False);

	switch (extension_method_to_call) {
	    case CallDnDVCXInitialize :
			
			/*
			 * create a new extension instance record
			 */ 

			size = (int)ext->instance_part_size;
			part = (OlDnDVendorPartExtension) XtCalloc(1, size);

			/* chain it */

			part->owner = new;
			part->class_extension = ext;
			part->next_part = ext->instance_part_list;
			ext->instance_part_list = part;

			/* init the resources */

			XtGetSubresources(new, (XtPointer)part, (String)NULL,
					  (String)NULL, ext->resources,
					  ext->num_resources, args, *num_args);

			new_part = part;
			req = *part;
			req_part = &req;
			break;

	    case CallDnDVCXDestroy :
			cur_part = _OlGetDnDVendorPartExtension(current);
			request = new = (Widget)NULL;
			args = (ArgList)NULL;
			num_args = (Cardinal)0;
			break;

	    case CallDnDVCXSetValues :
			new_part = _OlGetDnDVendorPartExtension(new);
			if (new_part == NULL_PART)
				return (False);	/*oops*/

			cur = *new_part;
			cur_part = &cur;

			XtSetSubvalues((XtPointer)new_part, ext->resources,
				       ext->num_resources, args, *num_args);
			req = *new_part;
			req_part = &req;
			break;

	    case CallDnDVCXGetValues :
			cur_part = _OlGetDnDVendorPartExtension(current);
			if (cur_part == NULL_PART)
				return (False);

			XtGetSubvalues((XtPointer)cur_part, ext->resources,
				       ext->num_resources, args, *num_args);

			request = new = (Widget)NULL;
			break;
	}

	ret_val = _CallExtensionMethods(extension_method_to_call, original_wc,
				       current, request, new, args, num_args,
				       cur_part, req_part, new_part);

	if (extension_method_to_call == CallDnDVCXDestroy) {
		OlDnDVendorPartExtension	*part_ptr;

		/* free the extension instance part */

		for ((part_ptr = &ext->instance_part_list), (part = *part_ptr);
		     part != NULL_PART && current != part->owner;) {
			part_ptr = &part->next_part;
			part = *part_ptr;
		}
		
		if (part != NULL_PART) {
			*part_ptr = part->next_part;
			XtFree((char *)part);
		}
	}

	return (ret_val);
}

/******************************** USER CALLS ********************************/

/************************************************************
 *
 *	OlDnDGetWidgetOfDropSite
 *
 * return widget associated with drop site .. this is either
 * the widget associated with the drop site on registration
 * or the widget parent of the window registered.
 *
 ************************************************************/

/*FTNPROTOB*/
Widget
OlDnDGetWidgetOfDropSite OLARGLIST(( dropsiteid ))
	OLGRA(OlDnDDropSiteID,	dropsiteid)
/*FTNPROTOE*/
{
	return (OlDnDDropSitePtrOwner((OlDnDDropSitePtr)dropsiteid));
}

/************************************************************
 *
 *	OlDnDGetWindowOfDropSite
 *
 * return the window associated with the dropsite ... this
 * is either the window registered or the window of the
 * "owning" widget ...
 *
 * if the 'object' registered was a gadget then the window
 * ID is that of its windowed ancestor
 *
 ************************************************************/

/*FTNPROTOB*/
Window
OlDnDGetWindowOfDropSite OLARGLIST(( dropsiteid ))
	OLGRA(OlDnDDropSiteID,	dropsiteid)
/*FTNPROTOE*/
{
	return (OlDnDDropSitePtrWindow((OlDnDDropSitePtr)dropsiteid));
}

/************************************************************
 *
 *	OlDnDGetDropSitesOfWidget
 *
 * return a list of dropsites registered for this widget
 *
 * note: the client must free the store returned
 *
 ************************************************************/

/*FTNPROTOB*/
OlDnDDropSiteID *
OlDnDGetDropSitesOfWidget OLARGLIST(( widget, num_sites_return ))
	OLARG(Widget,		widget)
	OLGRA(Cardinal *,	num_sites_return)
/*FTNPROTOE*/
{
	OlDnDDropSitePtr		dsp;
	Widget				vendor = GetShellOfWidget(widget);
	OlDnDVendorPartExtension	dnd_part;
	unsigned int			num;
	OlDnDDropSiteID			*dsids, *p;

	*num_sites_return = (Cardinal)0;

	if ((dnd_part = _OlGetDnDVendorPartExtension(vendor)) ==
	    (OlDnDVendorPartExtension)NULL ||
	    dnd_part->drop_site_list == (OlDnDDropSitePtr)NULL) {
		return (OlDnDDropSiteID *)NULL;
	}

	for (num = 0, dsp = dnd_part->drop_site_list;
	     dsp != (OlDnDDropSitePtr)NULL;
	     dsp = OlDnDDropSitePtrNextSite(dsp))
		if (widget == OlDnDDropSitePtrOwner(dsp)) num++;

	if (!num) return (OlDnDDropSiteID *)NULL;

	p = dsids = (OlDnDDropSiteID *)XtCalloc(num, sizeof(OlDnDDropSiteID));

	for (dsp = dnd_part->drop_site_list;
	     dsp != (OlDnDDropSitePtr)NULL;
             dsp = OlDnDDropSitePtrNextSite(dsp))
		if (widget == OlDnDDropSitePtrOwner(dsp)) 
			*p++ = (OlDnDDropSiteID)dsp;

	*num_sites_return = num;

	return dsids;
}
	

/************************************************************
 *
 *	OlDnDGetDropSitesOfWindow
 *
 * return a list of dropsites registered for this window
 *
 * note: the client must free the store returned
 *
 ************************************************************/

/*FTNPROTOB*/
OlDnDDropSiteID *
OlDnDGetDropSitesOfWindow OLARGLIST(( dpy, window, num_sites_return ))
	OLARG(Display *,	dpy)
	OLARG(Window,		window)
	OLGRA(Cardinal *,	num_sites_return)
/*FTNPROTOE*/
{
	OlDnDDropSitePtr		dsp;
	Widget				vendor;
	OlDnDVendorPartExtension	dnd_part;
	unsigned int			num;
	OlDnDDropSiteID			*dsids, *p;

	*num_sites_return = (Cardinal)0;

	vendor = GetShellParentOfWindow(dpy, window);

	if ((dnd_part = _OlGetDnDVendorPartExtension(vendor)) ==
	    (OlDnDVendorPartExtension)NULL ||
	    dnd_part->drop_site_list == (OlDnDDropSitePtr)NULL) {
		return (OlDnDDropSiteID *)NULL;
	}

	for (num = 0, dsp = dnd_part->drop_site_list;
	     dsp != (OlDnDDropSitePtr)NULL;
	     dsp = OlDnDDropSitePtrNextSite(dsp))
		if (window == OlDnDDropSitePtrWindow(dsp)) num++;

	if (!num) return (OlDnDDropSiteID *)NULL;

	p = dsids = (OlDnDDropSiteID *)XtCalloc(num, sizeof(OlDnDDropSiteID));

	for (dsp = dnd_part->drop_site_list;
	     dsp != (OlDnDDropSitePtr)NULL;
             dsp = OlDnDDropSitePtrNextSite(dsp))
		if (window == OlDnDDropSitePtrWindow(dsp)) 
			*p++ = (OlDnDDropSiteID)dsp;

	return dsids;
}
	

/************************************************************
 *
 *	OlDnDOwnSelection
 *
 * this is the function used to assert ownership of a
 * selection suitable for drag and drop operations.
 * it is identical in function to the Intrinisics rountines
 * for handling selections except that instead of specifying
 * a widget the caller specifies a dropsite ID ...
 *
 * 
 ************************************************************/

/*
 * this handler is used to catch the death of the requestors window
 * during the drag and drop operation.
 *
 * if the property notify is for the requestor window, with our transient
 * atom specified and the state being delet then we can assume that
 * the window has been destoryed so we call the Protocol state callback
 * with a detail of OlDnDTransactionRequestorWindowDeath.
 *
 */

static void
_OlDnDRequestorWindowDiedHandler OLARGLIST((widget, client_data, xevent,
					     continue_to_dispatch ))
	OLARG(Widget,		widget)
	OLARG(XtPointer,	client_data)
	OLARG(XEvent *,		xevent)
	OLGRA(Boolean *,	continue_to_dispatch)
{
	OwnerProcClosurePtr		opc = (OwnerProcClosurePtr)client_data;
	XPropertyEvent			*pev = (XPropertyEvent*)xevent;
	Atom				selection;

	selection = DSSelectionAtomPtrSelectionAtom(
			OwnerProcClosurePtrAssoc(opc));

	if (pev->type == PropertyNotify && pev->window == 
	    DSSelectionAtomPtrRequestorWindow(OwnerProcClosurePtrAssoc(opc)) &&
	    OwnerProcClosurePtrTransient(opc) == pev->atom &&
	    pev->state == PropertyDelete) {
		if (OwnerProcClosurePtrStateProc(opc) !=
		    (OlDnDTransactionStateCallback)NULL)
			(*OwnerProcClosurePtrStateProc(opc))
				(widget, selection,
				 OlDnDTransactionRequestorWindowDeath,
				 pev->time,
				 OwnerProcClosurePtrClientData(opc));
		*continue_to_dispatch = False;
	}
}

static void
_LocalRequestorDied OLARGLIST((w, client_data, call_data))
		OLARG(Widget,		w)
		OLARG(XtPointer,	client_data)
		OLGRA(XtPointer,	call_data)
{
	XPropertyEvent		prop;
	OwnerProcClosurePtr	opc = (OwnerProcClosurePtr)client_data;
	Boolean			dummy;

	/*
 	 * the widget is dying .... forge an event and tell the owner
	 * that the requestor is shaking off its mortal coil!
	 */

	prop.type = PropertyNotify;
	prop.display = DSSelectionAtomPtrRequestorDisplay(
						OwnerProcClosurePtrAssoc(opc));
	prop.window = DSSelectionAtomPtrRequestorWindow(
						OwnerProcClosurePtrAssoc(opc)); 
	prop.atom = OwnerProcClosurePtrTransient(opc);
	prop.state = PropertyDelete;
	prop.time = XtLastTimestampProcessed(XtDisplay(w));

	
	_OlDnDRequestorWindowDiedHandler(w, client_data, (XEvent *)&prop, &dummy);
}

/*
 * tidy up the requestor window death handler 
 */

static void _RemoveRequestorWindowDeathHandler OLARGLIST(( closure ))
		OLGRA(OwnerProcClosurePtr,	closure)
{
	Widget			widget;
	DSSelectionAtomPtr	sap = OwnerProcClosurePtrAssoc(closure);
	Window			window;
	Display			*dpy;

	widget = DSSelectionAtomPtrOwner(sap);
	window = DSSelectionAtomPtrRequestorWindow(sap);
	dpy    = DSSelectionAtomPtrRequestorDisplay(sap);

	/* was this the death of a non-local window ?? */
	if (OwnerProcClosurePtrTransient(closure) != (Atom)NULL) {
		_XtUnregisterWindow(window, widget);
		XtRemoveEventHandler(widget, PropertyChangeMask, False,
				     _OlDnDRequestorWindowDiedHandler,
				     (XtPointer)closure);
		OlDnDFreeTransientAtom(widget,
					OwnerProcClosurePtrTransient(closure));
	} else if ((widget = XtWindowToWidget(dpy, window)) != (Widget)NULL)
			XtRemoveCallback(widget, XtNdestroyCallback,
				         _LocalRequestorDied, closure);

	OwnerProcClosurePtrCleanupProc(closure) = 
		(OlDnDTransactionCleanupProc)NULL;
}

/*
 * filter out Selection conversion requests that are part of the
 * drag and drop protocol, and not part of the data transfer.
 */

static Boolean
_OlDnDConvertSelectionFilter OLARGLIST(( vendor, dnd_part, opc,
				         widget, selection, target,
				         type, value, length, format,
				         dispatch_convert ))
		OLARG( Widget,			 vendor)
		OLARG( OlDnDVendorPartExtension, dnd_part)
		OLARG( OwnerProcClosurePtr,	 opc)
		OLARG( Widget,			 widget)
		OLARG( Atom *,			 selection)
		OLARG( Atom *,			 target)
		OLARG( Atom *,			 type)
		OLARG( XtPointer *,		 value)
		OLARG( unsigned long *, 	 length)
		OLARG( int *,			 format)
		OLGRA( Boolean *,	 	 dispatch_convert)
{
	XSelectionRequestEvent	*sev;
	OlDnDTransactionState	state;

	FetchPartExtensionIfNull(vendor, dnd_part);

	*dispatch_convert = False;

	sev = XtGetSelectionRequest(widget, *selection,
				    (XtRequestId)NULL);

	if (*target == _SUN_DRAGDROP_ACK) {
		Widget			w;

		*value = (XtPointer)NULL;
		*length = (unsigned long)0;
		*format = 32;
		*type = XA_ATOM;

		DSSelectionAtomPtrRequestorWindow(
			OwnerProcClosurePtrAssoc(opc)) = sev->requestor;
		DSSelectionAtomPtrRequestorDisplay(
			OwnerProcClosurePtrAssoc(opc)) = sev->display;

		if ((w = XtWindowToWidget(sev->display, sev->requestor)) !=
		     (Widget)NULL) {
			/* this is a window owned by this app ..... */

			OwnerProcClosurePtrTransient(opc) = (Atom)NULL;
			XtAddCallback(w, XtNdestroyCallback,
				      _LocalRequestorDied, (XtPointer)opc);
		} else {	/* non local window */
			Atom			transient;

			/* 
			 * assign a transient atom and register it as
			 * a property on the requestors window.
			 *
			 * then register an event handler to watch
			 * for the premature demise of the requestors
			 * window.
			 */

			transient = OlDnDAllocTransientAtom(widget);
			OwnerProcClosurePtrTransient(opc) = transient;

			XChangeProperty(sev->display, sev->requestor, transient,
					XA_ATOM, 32, PropModeReplace, 
					(unsigned char *)NULL, 0);

			_XtRegisterWindow(sev->requestor, widget);

			XtAddEventHandler(widget, PropertyChangeMask, False,
					  _OlDnDRequestorWindowDiedHandler,
					  (XtPointer)opc);
		}

		return (True);
	}

	if (*target == _SUN_DRAGDROP_BEGIN) {

		*value = (XtPointer)NULL;
		*length = (unsigned long)0;
		*format = 32;
		*type = XA_ATOM;

		if (OwnerProcClosurePtrStateProc(opc) !=
		    (OlDnDTransactionStateCallback)NULL)
			(*OwnerProcClosurePtrStateProc(opc))
				(widget,
				 (int)*selection,
				 OlDnDTransactionBegins,
				 sev->time,
				 OwnerProcClosurePtrClientData(opc));
		return (True);
	}

	if (((state = OlDnDTransactionEnds), *target == _SUN_SELECTION_END)  ||
	    ((state = OlDnDTransactionRequestorError),
	      *target == _SUN_SELECTION_ERROR)||
	    ((state = OlDnDTransactionDone), *target == _SUN_DRAGDROP_DONE)) {

		*value = (XtPointer)NULL;
		*length = (unsigned long)0;
		*format = 32;
		*type = XA_ATOM;

		/*
		 * call the state callback to indicate
		 * that the transaction failed.
		 */

		if (OwnerProcClosurePtrStateProc(opc) != 
		    (OlDnDTransactionStateCallback)NULL)
			(*OwnerProcClosurePtrStateProc(opc))
				(widget,
				 (int)*selection,
				 state,
				 sev->time,
				 OwnerProcClosurePtrClientData(opc));

#if !defined(att) && !defined(USL)
			/* This step is not necessary because:
			 *
			 **if OlDnDDisownSelection is invoked in
			 * state_proc (above) then "opc" is invalid
			 * (See OlDnDDisownSelection()).
			 *
			 **otherwise, just invoke
			 * OwnerProcClosurePtrCleanupProc won't
			 * (i.e., _RemoveRequestorWindowDeathHandler)
			 * help anyway because "cleanup" procedures
			 * are in-completed
			 * (See OlDnDDisownSelection() for this).
			 */
		if (OwnerProcClosurePtrCleanupProc(opc) !=
			(OlDnDTransactionCleanupProc)NULL) 
				(*OwnerProcClosurePtrCleanupProc(opc))(opc);
#endif

		return (True);
	}

	/*
	 * if we got to here then its part of the selection
	 * transfer and not part of the drag and drop protocol
	 * so call the users registered Convert Proc.
	 */

	*dispatch_convert = True;
	return (True);
}


/*
 * the Selection Converter ..... filters out protocol conversions
 * but calls the users ConvertSelection Proc whern appropriate
 */

static Boolean
_OlDnDConvertSelectionFunc OLARGLIST(( widget, selection, target,
				      type, value, length, format ))
		OLARG( Widget,		widget)
		OLARG( Atom *,		selection)
		OLARG( Atom *,		target)
		OLARG( Atom *,		type)
		OLARG( XtPointer *,	value)
		OLARG( unsigned long *, length)
		OLGRA( int *,		format)
{
	Widget				vendor = GetShellOfWidget(widget);
	Widget				sw;
	OlDnDVendorPartExtension	dnd_part;
	OwnerProcClosurePtr		opc;
	Boolean				ret_val, dispatch_convert = False;

	if ((dnd_part = _OlGetDnDVendorPartExtension(vendor)) ==
	    (OlDnDVendorPartExtension)NULL)
		return False; /* oops! */

	sw = _OlDnDGetWidgetForSelection(vendor, dnd_part, *selection, &opc);

	if (sw != widget)
		return (False);

	ret_val = _OlDnDConvertSelectionFilter(vendor, dnd_part, opc,
					     widget, selection, target,
					     type, value, length, format,
					     &dispatch_convert);

	if (dispatch_convert && OwnerProcClosurePtrConvertProc(opc) !=
	    (XtConvertSelectionProc)NULL)
		ret_val = (*OwnerProcClosurePtrConvertProc(opc))
				(widget, selection, target, type, value,
				 length, format);

	return (ret_val);
}

/*
 * tidy up its all over .....
 */

static void
_OlDnDSelectionDoneProc OLARGLIST(( widget, selection, target ))
		OLARG( Widget,	widget)
		OLARG( Atom *,	selection)
		OLGRA( Atom *,	target)
{
	Widget				vendor = GetShellOfWidget(widget);
	Widget				sw;
	OlDnDVendorPartExtension	dnd_part;
	OwnerProcClosurePtr		opc;

	if ((dnd_part = _OlGetDnDVendorPartExtension(vendor)) ==
	    (OlDnDVendorPartExtension)NULL)
		return; /* oops! */

	sw = _OlDnDGetWidgetForSelection(vendor, dnd_part, *selection, &opc);

	if (sw != widget)
		return;

	if (OwnerProcClosurePtrDoneProc(opc) != 
	    (XtSelectionDoneProc)NULL)
		(*OwnerProcClosurePtrDoneProc(opc))
			(widget, selection, target); 
}

/*
 * tidy up we lost the selection ..... 
 */

static void
_OlDnDLoseSelectionProc OLARGLIST(( widget, selection ))
	OLARG( Widget,	widget)
	OLGRA( Atom *,	selection)
{
	Widget				vendor = GetShellOfWidget(widget);
	Widget				sw;
	OlDnDVendorPartExtension	dnd_part;
	OwnerProcClosurePtr		opc;

	if ((dnd_part = _OlGetDnDVendorPartExtension(vendor)) ==
	    (OlDnDVendorPartExtension)NULL)
		return; /* oops! */

	sw = _OlDnDGetWidgetForSelection(vendor, dnd_part, *selection, &opc);

	if (sw != widget)
		return;

	if (OwnerProcClosurePtrLoseProc(opc) != 
	    (XtLoseSelectionProc)NULL)
		(*OwnerProcClosurePtrLoseProc(opc))
			(widget, selection); 

	if (OwnerProcClosurePtrCleanupProc(opc) !=
		(OlDnDTransactionCleanupProc)NULL)
			(*OwnerProcClosurePtrCleanupProc(opc))(opc);

	_OlDnDDisassocSelectionWithWidget(vendor, dnd_part, widget,
				          *selection, 
					  XtLastTimestampProcessed(
						XtDisplay(vendor)));

	XtFree((char *)opc);
}


/* OlDnDOwnSelection */

/*FTNPROTOB*/
Boolean
OlDnDOwnSelection OLARGLIST(( widget, selection, timestamp, convert_proc,
			      lose_selection_proc, done_proc, state_proc,
			      closure ))
	OLARG(Widget,				widget)
	OLARG(Atom,				selection)
	OLARG(Time,				timestamp)
	OLARG(XtConvertSelectionProc,		convert_proc)
	OLARG(XtLoseSelectionProc,		lose_selection_proc)
	OLARG(XtSelectionDoneProc,		done_proc)
	OLARG(OlDnDTransactionStateCallback, 	state_proc)
	OLGRA(XtPointer, 			closure)
/*FTNPROTOE*/
{
	Widget				vendor;
	OwnerProcClosurePtr		opc;
	OlDnDVendorPartExtension	dnd_part;

	vendor = GetShellOfWidget(widget);
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part == (OlDnDVendorPartExtension)NULL)
		return (False);

	/*
	 * create our own private closure of the users procs and then
	 * assert ownership of the selection, using our private 
	 * filter functions.
	 */
	  
	opc = (OwnerProcClosurePtr)XtCalloc(1, sizeof(OwnerProcClosure));

	OwnerProcClosurePtrTransient(opc) = (Atom)NULL;
	OwnerProcClosurePtrConvertProc(opc) = convert_proc;
	OwnerProcClosurePtrLoseProc(opc) = lose_selection_proc;
	OwnerProcClosurePtrDoneProc(opc) = done_proc;
	OwnerProcClosurePtrDoneIncrProc(opc) =
		(XtSelectionDoneIncrProc)NULL;
	OwnerProcClosurePtrConvertIncrProc(opc) =
		(XtConvertSelectionIncrProc)NULL;
	OwnerProcClosurePtrLoseIncrProc(opc) =
		(XtLoseSelectionIncrProc)NULL;
	 OwnerProcClosurePtrCancelIncrProc(opc) =
		(XtCancelConvertSelectionProc)NULL;
	OwnerProcClosurePtrStateProc(opc) = state_proc;
	OwnerProcClosurePtrClientData(opc) = closure;
	OwnerProcClosurePtrCleanupProc(opc) =
			_RemoveRequestorWindowDeathHandler;
	OwnerProcClosurePtrSelectionTransient(opc) = 
			_OlDnDAtomIsTransient(vendor, dnd_part, selection);
 
	if (!XtOwnSelection(widget,
			    selection, timestamp,
			    _OlDnDConvertSelectionFunc,
			    _OlDnDLoseSelectionProc,
			    _OlDnDSelectionDoneProc)) {

		XtFree((char *)opc);
		return (False);
	}

	OwnerProcClosurePtrAssoc(opc) = _OlDnDAssocSelectionWithWidget(
						vendor, dnd_part, widget,
						selection, timestamp, opc);

	return (True);
}

/************************************************************
 *
 *	OlDnDOwnSelectionIncremental
 *
 ************************************************************/


static Boolean
_OlDnDConvertSelectionIncrFunc OLARGLIST(( widget, selection, target,
				      type, value, length, format, max_length,
				      client_data, request_id ))
		OLARG( Widget,		widget)
		OLARG( Atom *,		selection)
		OLARG( Atom *,		target)
		OLARG( Atom *,		type)
		OLARG( XtPointer *,	value)
		OLARG( unsigned long *, length)
		OLARG( int *,		format)
		OLARG( unsigned long *,	max_length)
		OLARG( XtPointer,	client_data)
		OLGRA( XtRequestId *,	request_id)
{
	Widget				vendor = GetShellOfWidget(widget);
	Widget				sw;
	OlDnDVendorPartExtension	dnd_part;
	OwnerProcClosurePtr		opc;
	Boolean				ret_val, dispatch_convert;

	if ((dnd_part = _OlGetDnDVendorPartExtension(vendor)) ==
	    (OlDnDVendorPartExtension)NULL)
		return; /* oops! */

	sw = _OlDnDGetWidgetForSelection(vendor, dnd_part, *selection, &opc);

	if (sw != widget)
		return (False);

	ret_val = _OlDnDConvertSelectionFilter(vendor, dnd_part, opc,
					     widget, selection, target,
					     type, value, length, format,
					     &dispatch_convert);

	if (dispatch_convert && OwnerProcClosurePtrConvertIncrProc(opc) !=
	    (XtConvertSelectionIncrProc)NULL)
		return (*OwnerProcClosurePtrConvertIncrProc(opc))
				(widget, selection, target, type,
				 value, length, format, max_length,
				 client_data, request_id);
	else
		return (ret_val);
}


static void
_OlDnDSelectionDoneIncrProc OLARGLIST(( widget, selection, target,
					request_id, client_data ))
		OLARG( Widget,		widget)
		OLARG( Atom *,		selection)
		OLARG( Atom *,		target)
		OLARG( XtRequestId *,	request_id)
		OLGRA( XtPointer,	client_data)
{
	Widget				vendor = GetShellOfWidget(widget);
	Widget				sw;
	OlDnDVendorPartExtension	dnd_part;
	OwnerProcClosurePtr		opc;

	if ((dnd_part = _OlGetDnDVendorPartExtension(vendor)) ==
	    (OlDnDVendorPartExtension)NULL)
		return; /* oops! */

	sw = _OlDnDGetWidgetForSelection(vendor, dnd_part, *selection, &opc);

	if (sw != widget)
		return;

	if (OwnerProcClosurePtrDoneIncrProc(opc) != 
	    (XtSelectionDoneIncrProc)NULL)
		(*OwnerProcClosurePtrDoneIncrProc(opc))
			(widget, selection, target, request_id, client_data);
}

static void
_OlDnDLoseSelectionIncrProc OLARGLIST(( widget, selection, client_data ))
	OLARG( Widget,		widget)
	OLARG( Atom *,		selection)
	OLGRA(XtPointer,	client_data)
{
	Widget				vendor = GetShellOfWidget(widget);
	Widget				sw;
	OlDnDVendorPartExtension	dnd_part;
	OwnerProcClosurePtr		opc;

	if ((dnd_part = _OlGetDnDVendorPartExtension(vendor)) ==
	    (OlDnDVendorPartExtension)NULL)
		return; /* oops! */

	sw = _OlDnDGetWidgetForSelection(vendor, dnd_part, *selection, &opc);

	if (sw != widget)
		return;

	if (OwnerProcClosurePtrLoseIncrProc(opc) != 
	    (XtLoseSelectionIncrProc)NULL)
		(*OwnerProcClosurePtrLoseIncrProc(opc))
			(widget, selection, client_data); 

	if (OwnerProcClosurePtrCleanupProc(opc) !=
		(OlDnDTransactionCleanupProc)NULL)
			(*OwnerProcClosurePtrCleanupProc(opc))(opc);

	_OlDnDDisassocSelectionWithWidget(vendor, dnd_part, widget,
					  *selection, 
					  XtLastTimestampProcessed(
						XtDisplay(vendor)));

	XtFree((char *)opc);
}

static void
_OlDnDCancelConvertSelectionProc OLARGLIST(( widget, selection, target,
					     requestid, client_data ))
	OLARG( Widget,		widget)
	OLARG( Atom *,		selection)
	OLARG( Atom *,		target)
	OLARG( XtRequestId *, 	requestid)
	OLGRA(XtPointer,	client_data)
{
	Widget				vendor = GetShellOfWidget(widget);
	Widget				sw;
	OlDnDVendorPartExtension	dnd_part;
	OwnerProcClosurePtr		opc;

	if ((dnd_part = _OlGetDnDVendorPartExtension(vendor)) ==
	    (OlDnDVendorPartExtension)NULL)
		return; /* oops! */

	sw = _OlDnDGetWidgetForSelection(vendor, dnd_part, *selection, &opc);

	if (sw != widget)
		return;

	if (OwnerProcClosurePtrCancelIncrProc(opc) != 
	    (XtCancelConvertSelectionProc)NULL)
		(*OwnerProcClosurePtrCancelIncrProc(opc))
			(widget, selection, target, requestid, client_data); 
}

/*FTNPROTOB*/
Boolean
OlDnDOwnSelectionIncremental OLARGLIST(( widget, selection, timestamp,
					 convert_incr_proc,
					 lose_incr_selection_proc,
					 incr_done_proc, incr_cancel_proc,
					 client_data, state_proc))
	OLARG(Widget,				widget)
	OLARG(Atom,				selection)
	OLARG(Time,				timestamp)
	OLARG(XtConvertSelectionIncrProc,	convert_incr_proc)
	OLARG(XtLoseSelectionIncrProc,		lose_incr_selection_proc)
	OLARG(XtSelectionDoneIncrProc,		incr_done_proc)
	OLARG(XtCancelConvertSelectionProc,	incr_cancel_proc)
	OLARG(XtPointer,			client_data)
	OLGRA(OlDnDTransactionStateCallback, 	state_proc)
/*FTNPROTOE*/
{
	Widget				vendor;
	OwnerProcClosurePtr		opc;
	OlDnDVendorPartExtension	dnd_part;

	vendor = GetShellOfWidget(widget);
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part == (OlDnDVendorPartExtension)NULL)
		return (False);

	opc = (OwnerProcClosurePtr)XtCalloc(1, sizeof(OwnerProcClosure));

	OwnerProcClosurePtrTransient(opc) = (Atom)NULL;
	OwnerProcClosurePtrConvertIncrProc(opc) = convert_incr_proc;
	OwnerProcClosurePtrLoseIncrProc(opc) = lose_incr_selection_proc;
	OwnerProcClosurePtrCancelIncrProc(opc) = incr_cancel_proc;
	OwnerProcClosurePtrDoneIncrProc(opc) = incr_done_proc;
	OwnerProcClosurePtrDoneProc(opc) = (XtSelectionDoneProc)NULL;
	OwnerProcClosurePtrConvertProc(opc) = (XtConvertSelectionProc)NULL;
	OwnerProcClosurePtrLoseProc(opc) = (XtLoseSelectionProc)NULL;
	OwnerProcClosurePtrStateProc(opc) = state_proc;
	OwnerProcClosurePtrClientData(opc) = client_data;
	OwnerProcClosurePtrCleanupProc(opc) =
			_RemoveRequestorWindowDeathHandler;
	OwnerProcClosurePtrSelectionTransient(opc) = 
			_OlDnDAtomIsTransient(vendor, dnd_part, selection);
 
	if (!XtOwnSelectionIncremental(widget,
			    selection, timestamp,
			    _OlDnDConvertSelectionIncrFunc,
			    _OlDnDLoseSelectionIncrProc,
			    _OlDnDSelectionDoneIncrProc,
			    _OlDnDCancelConvertSelectionProc,
			    client_data)) {
		XtFree((char *)opc);
		return (False);
	}

	OwnerProcClosurePtrAssoc(opc) = _OlDnDAssocSelectionWithWidget(
						vendor, dnd_part, widget,
						selection, timestamp, opc);

	return (True);
}

/************************************************************
 *
 * OlDnDDisownSelection
 *
 * disown the selection ....
 *
 ************************************************************/

/*FTNPROTOB*/
void
OlDnDDisownSelection OLARGLIST((widget, selection, time))
	OLARG(Widget,		widget)
	OLARG(Atom,		selection)
	OLGRA(Time,		time)
/*FTNPROTOE*/
{
	Widget				vendor, sw;
	OwnerProcClosurePtr		opc;
	OlDnDVendorPartExtension	dnd_part;

	vendor = GetShellOfWidget(widget);
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part == (OlDnDVendorPartExtension)NULL)
		return;

	sw = _OlDnDGetWidgetForSelection(vendor, dnd_part, selection, &opc);

	if (widget != sw)
		return;
	
	_OlDnDDisassocSelectionWithWidget(vendor, dnd_part, widget,
					  selection, time);

	XtDisownSelection(widget, selection, time);

	if (OwnerProcClosurePtrCleanupProc(opc) !=
		(OlDnDTransactionCleanupProc)NULL)
			(*OwnerProcClosurePtrCleanupProc(opc))(opc);

	XtFree((char *)opc);
}

/************************************************************
 *
 *	OlDnDBeginSelectionTransaction
 *
 ************************************************************/

static void
_DNDVCXHandleProtocolActionCB OLARGLIST((vendor, client_data, selection, type,
					value, length, format))
	OLARG(Widget,		vendor)
	OLARG(XtPointer,	client_data)
	OLARG(Atom *,		selection)
	OLARG(Atom *,		type)
	OLARG(XtPointer,	value)
	OLARG(unsigned long *,	length)
	OLGRA(int *,		format)
{
	OlDnDProtocolAction	action;
	OlDnDDropSitePtr	dsp;
	ReqProcClosurePtr	pc = (ReqProcClosurePtr)client_data;

	if (ReqProcClosurePtrAction(pc) == _SUN_DRAGDROP_BEGIN) {
		action = OlDnDSelectionTransactionBegins;
	} else if (ReqProcClosurePtrAction(pc) == _SUN_SELECTION_END)
		action = OlDnDSelectionTransactionEnds;
	else if (ReqProcClosurePtrAction(pc) == _SUN_DRAGDROP_DONE)
			action = OlDnDDragNDropTransactionDone;
	     else
			action = OlDnDSelectionTransactionError;

		
	if (ReqProcClosurePtrCallback(pc) != 
	    (OlDnDProtocolActionCallbackProc)NULL)
		(*ReqProcClosurePtrCallback(pc))
				(ReqProcClosurePtrWidget(pc),
				 *selection, action,
				 (*type != XT_CONVERT_FAIL),
				 ReqProcClosurePtrClosure(pc));
	XtFree((char *)pc);
}

static void
DnDVCXGetProtocolActionSelection OLARGLIST(( widget, selection, protocol,
					     timestamp, proc, closure ))
	OLARG(Widget,					widget)
	OLARG(Atom,					selection)
	OLARG(Atom,					protocol)
	OLARG(Time,					timestamp)
	OLARG(OlDnDProtocolActionCallbackProc,		proc)
	OLGRA(XtPointer,				closure)
{
	Widget		vendor;
	ReqProcClosurePtr	pc = (ReqProcClosurePtr)XtCalloc(1, sizeof(ReqProcClosure));

	ReqProcClosurePtrWidget(pc) = widget;
	ReqProcClosurePtrCallback(pc) = proc;
	ReqProcClosurePtrClosure(pc) = closure;
	ReqProcClosurePtrAction(pc) = protocol;

#if !defined(att) && !defined(USL)
	if (!XtIsVendorShell(widget))
		vendor = GetShellOfWidget(widget);
#else
	vendor = GetShellOfWidget(widget);
#endif

	XtGetSelectionValue(vendor, selection, protocol,
#if !defined(att) && !defined(USL)
			    _DNDVCXHandleProtocolActionCB, (XtPointer)pc);
#else
			    _DNDVCXHandleProtocolActionCB, (XtPointer)pc,
			    timestamp);
#endif
}

/*FTNPROTOB*/
void
OlDnDBeginSelectionTransaction OLARGLIST(( widget, selection, timestamp, 
					   proc, closure ))
	OLARG(Widget,					widget)
	OLARG(Atom,					selection)
	OLARG(Time,					timestamp)
	OLARG(OlDnDProtocolActionCallbackProc,		proc)
	OLGRA(XtPointer,				closure)
/*FTNPROTOE*/
{
	DnDVCXGetProtocolActionSelection(widget, selection,
					 _SUN_DRAGDROP_BEGIN, timestamp, 
					 proc, closure);
}

/************************************************************
 *
 *	OlDnDEndSelectionTransaction
 *
 ************************************************************/

/*FTNPROTOB*/
void
OlDnDEndSelectionTransaction OLARGLIST(( widget, selection, timestamp, 
					   proc, closure ))
	OLARG(Widget,					widget)
	OLARG(Atom,					selection)
	OLARG(Time,					timestamp)
	OLARG(OlDnDProtocolActionCallbackProc,		proc)
	OLGRA(XtPointer,				closure)
/*FTNPROTOE*/
{
	DnDVCXGetProtocolActionSelection(widget, selection,
					 _SUN_SELECTION_END, timestamp,
					 proc, closure);
}

/************************************************************
 *
 *	OlDnDDragNDropDone
 *
 ************************************************************/

/*FTNPROTOB*/
void
OlDnDDragNDropDone OLARGLIST(( widget, selection, timestamp, proc, closure ))
	OLARG(Widget,					widget)
	OLARG(Atom,					selection)
	OLARG(Time,					timestamp)
	OLARG(OlDnDProtocolActionCallbackProc,		proc)
	OLGRA(XtPointer,				closure)
/*FTNPROTOE*/
{
	DnDVCXGetProtocolActionSelection(widget, selection,
					 _SUN_DRAGDROP_DONE, timestamp,
					 proc, closure);
}

/************************************************************
 *
 *	OlDnDErrorDuringSelectionTransaction
 *
 ************************************************************/

/*FTNPROTOB*/
void
OlDnDErrorDuringSelectionTransaction OLARGLIST(( widget, selection, timestamp, 
					         proc, closure ))
	OLARG(Widget,					widget)
	OLARG(Atom,					selection)
	OLARG(Time,					timestamp)
	OLARG(OlDnDProtocolActionCallbackProc,		proc)
	OLGRA(XtPointer,				closure)
/*FTNPROTOE*/
{
	DnDVCXGetProtocolActionSelection(widget, selection,
					 _SUN_SELECTION_ERROR, timestamp, 
					 proc, closure);
}

/************************************************************
 *
 *	OlDnDAllocTransientAtom
 *
 ************************************************************/

/*FTNPROTOB*/
Atom
OlDnDAllocTransientAtom OLARGLIST(( widget ))
	OLGRA( Widget, widget)
/*FTNPROTOE*/
{
	Widget				vendor;
	OlDnDVendorPartExtension	dnd_part;

	vendor = GetShellOfWidget(widget);
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part->class_extension->alloc_transient_atom != 
	    (OlDnDVCXAllocTransientAtomFunc)NULL)
		return ((*dnd_part->class_extension->alloc_transient_atom)
			  (vendor, dnd_part, widget));
	else
		return (Atom)NULL;
}

/************************************************************
 *
 *	OlDnDFreeTransientAtom
 *
 ************************************************************/

/*FTNPROTOB*/
void
OlDnDFreeTransientAtom OLARGLIST(( widget, atom ))
	OLARG( Widget,  widget )
	OLGRA( Atom,	atom )
/*FTNPROTOE*/
{
	Widget				vendor;
	OlDnDVendorPartExtension	dnd_part;

	vendor = GetShellOfWidget(widget);
	dnd_part = _OlGetDnDVendorPartExtension(vendor);
	if (dnd_part->class_extension->free_transient_atom !=
	    (OlDnDVCXFreeTransientAtomProc)NULL)
		(*dnd_part->class_extension->free_transient_atom)
		  (vendor, dnd_part, widget, atom);
}

/************************************************************
 *
 *	OlDnDRegisterWidgetDropSite()
 *
 * 	Add a new ObjectDropSite
 *
 ************************************************************/

/*FTNPROTOB*/
OlDnDDropSiteID
OlDnDRegisterWidgetDropSite OLARGLIST(( widget, preview_hints, site_rects,
				        num_sites, tmnotify, pmnotify,
					on_interest, closure))
	OLARG( Widget,				widget )
	OLARG( OlDnDSitePreviewHints,		preview_hints )
	OLARG( OlDnDSiteRectPtr,		site_rects )
	OLARG( unsigned int,			num_sites)
	OLARG( OlDnDTMNotifyProc,		tmnotify )
	OLARG( OlDnDPMNotifyProc,		pmnotify )
	OLARG( Boolean,				on_interest )
	OLGRA( XtPointer,			closure )
/*FTNPROTOE*/
{
	Widget				vendor;
	Window				window;
	OlDnDVendorPartExtension	dnd_part;

	if (widget == (Widget)NULL) {
#if 0
		OlWarning("OlDnDRegisterWidgetDropSite: NULL widget");
#else
		XtWarning("OlDnDRegisterWidgetDropSite: NULL widget");
#endif
		return (OlDnDDropSiteID)NULL;
	}

	vendor = GetShellOfWidget(widget);
	window = XtWindowOfObject(widget);

	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->register_drop_site !=
	    (OlDnDVCXRegisterDSFunc)NULL) {
		return (*dnd_part->class_extension->register_drop_site)
				(vendor, dnd_part, widget, window,
				 preview_hints, site_rects, num_sites,
				 tmnotify, pmnotify, on_interest, closure);
	} else {
#if 0
		OlWarning(
		 "OlDnDRegisterWidgetDropSite: NULL class extension or proc");
#else
		XtWarning(
		 "OlDnDRegisterWidgetDropSite: NULL class extension or proc");
#endif
		return (OlDnDDropSiteID)NULL;
	}
}


/************************************************************
 *
 *	OlDnDRegisterWindowDropSite()
 *
 * 	Add a new WindowDropSite
 *
 ************************************************************/

/*FTNPROTOB*/
OlDnDDropSiteID
OlDnDRegisterWindowDropSite OLARGLIST(( dpy, window, preview_hints, site_rects,
				        num_sites, tmnotify, pmnotify,
					on_interest, closure))
	OLARG( Display *,			dpy )
	OLARG( Window,				window )
	OLARG( OlDnDSitePreviewHints,		preview_hints )
	OLARG( OlDnDSiteRectPtr,		site_rects )
	OLARG( unsigned int,			num_sites)
	OLARG(OlDnDTMNotifyProc,		tmnotify )
	OLARG(OlDnDPMNotifyProc,		pmnotify )
	OLARG( Boolean,				on_interest )
	OLGRA( XtPointer,			closure )
/*FTNPROTOE*/
{
	Widget				vendor, widget;
	OlDnDVendorPartExtension	dnd_part;

	if (dpy == (Display*)NULL || window == (Window)NULL) {
#if 0
		OlWarning(
			"OlDnDRegisterWindowDropSite: NULL Display or Window");
#else
		XtWarning(
			"OlDnDRegisterWindowDropSite: NULL Display or Window");
#endif
		return (OlDnDDropSiteID)NULL;
	}

	vendor = GetShellParentOfWindow(dpy, window);

	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->register_drop_site !=
	    (OlDnDVCXRegisterDSFunc)NULL) {
		return (*dnd_part->class_extension->register_drop_site)
				(vendor, dnd_part, widget, window,
				 preview_hints, site_rects, num_sites,
				 tmnotify, pmnotify, on_interest, closure);
	} else {
#if 0
		OlWarning(
		 "OlDnDRegisterWindowDropSite: NULL class extension or proc");
#else
		XtWarning(
		 "OlDnDRegisterWindowDropSite: NULL class extension or proc");
#endif
		return (OlDnDDropSiteID)NULL;
	}
}

/********************************************
 *
 * OlDnDDestroyDropSite
 *
 ********************************************/


/*FTNPROTOB*/
void
OlDnDDestroyDropSite OLARGLIST (( dropsiteid ))
	OLGRA(OlDnDDropSiteID,	dropsiteid)
/*FTNPROTOE*/
{
	OlDnDDropSitePtr		dsp  = (OlDnDDropSitePtr)dropsiteid;
	OlDnDVendorPartExtension	dnd_part;
	Widget				vendor;

	vendor = GetShellOfWidget(OlDnDDropSitePtrOwner(dsp));
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->delete_drop_site != 
	    (OlDnDVCXDeleteDSProc)NULL)
		(*dnd_part->class_extension->delete_drop_site)
			(vendor, dnd_part, dropsiteid);
}

/********************************************
 *
 * OlDnDUpdateDropSiteGeometry
 *
 ********************************************/

/*FTNPROTOB*/
Boolean
OlDnDUpdateDropSiteGeometry OLARGLIST(( dropsiteid, site_rects, num_rects ))
	OLARG(OlDnDDropSiteID,	dropsiteid)
	OLARG(OlDnDSiteRectPtr,	site_rects)
	OLGRA(unsigned int,	num_rects)
/*FTNPROTOE*/
{
	OlDnDDropSitePtr		dsp  = (OlDnDDropSitePtr)dropsiteid;
	OlDnDVendorPartExtension	dnd_part;
	Widget				vendor;

	vendor = GetShellOfWidget(OlDnDDropSitePtrOwner(dsp));
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->update_drop_site_geometry != 
	    (OlDnDVCXUpdateDSGeometryProc)NULL) {
		return (*dnd_part->class_extension->update_drop_site_geometry)
				(vendor, dnd_part, dropsiteid, site_rects, num_rects);
	} else return (False);

}

/********************************************
 *
 * OlDnDQueryDropSiteInfo
 *
 ********************************************/

/*FTNPROTOB*/
Boolean
OlDnDQueryDropSiteInfo OLARGLIST(( dropsiteid, widget, window, preview_hints,
				   site_rects, num_rects, on_interest ))
	OLARG( OlDnDDropSiteID, 	dropsiteid)
	OLARG( Widget *,		widget)
	OLARG( Window *,		window)
	OLARG( OlDnDSitePreviewHints *,	preview_hints)
	OLARG( OlDnDSiteRectPtr *,	site_rects)
	OLARG( unsigned int *, 		num_rects)
	OLGRA( Boolean *, 		on_interest )
/*FTNPROTOE*/
{
	OlDnDDropSitePtr		dsp = (OlDnDDropSitePtr)dropsiteid;
	OlDnDVendorPartExtension	dnd_part;
	Widget				vendor;

	vendor = GetShellOfWidget(OlDnDDropSitePtrOwner(dsp));
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->query_drop_site_info !=
	    (OlDnDVCXQueryDSInfoFunc)NULL) {
		return (*dnd_part->class_extension->query_drop_site_info)
				( vendor, dnd_part, dropsiteid, widget, window,
				  preview_hints, site_rects, num_rects,
				  on_interest );
			
	} else return (False);
}

/********************************************
 *
 * OlDnDDeliverTriggerMessage
 *
 ********************************************/

static OlDnDVendorPartExtension	horrible_hack = 
					(OlDnDVendorPartExtension)NULL;

#if defined(att) || defined(USL)
/*
 * OlDnDSendTriggerMessage - simulate a DnD trigger message and send it
 *		to "dst_win". The assumption is that
 *		"_SUN_DRAGDROP_INTEREST" should be attached to "dst_win".
 *		(-1, -1) will be used for (x, y) to indicate that this is
 *		a simulate trigger message since DnD protocol deal with
 *		"root" window coordinate.
 *		
 */
extern Boolean
OlDnDSendTriggerMessage OLARGLIST((w, root, dst_win, selection, op, timestamp))
	OLARG( Widget,			w)
	OLARG( Window,			root)
	OLARG( Window,			dst_win)
	OLARG( Atom,			selection)
	OLARG( OlDnDTriggerOperation,	op)
	OLGRA( Time,			timestamp)
{
	OlDnDVendorPartExtension	dnd_part;
	Widget				vendor;

	Atom				actual_type;
	int				actual_format;
	unsigned long			nitems;
	unsigned long			bytes_remaining;
	InterestPropertyPtr		interest = (InterestPropertyPtr)NULL;
	SiteDescriptionPtr		site;
	OlDnDDropSiteID			dropsiteid;
	Window				window_id;

	Boolean				ret = False;
	TriggerMessage			trigger_message;
	XClientMessageEvent		client_message;
	Widget				local_vendor = (Widget)NULL;
	OlDnDVendorPartExtension	part;
	OlDnDTriggerFlags		flags = 0;
	Display	*			dpy;

#if !defined(att) && !defined(USL)
	if (!XtIsVendorShell(widget))
		vendor = GetShellOfWidget(w);
#else
	vendor = GetShellOfWidget(w);
#endif

	if (horrible_hack != (OlDnDVendorPartExtension)NULL &&
	    horrible_hack->owner == vendor) {
		dnd_part = horrible_hack;
	} else {
		horrible_hack = dnd_part = _OlGetDnDVendorPartExtension(vendor);
	}

	dpy = XtDisplay(vendor);

	if (XGetWindowProperty(dpy, dst_win, _SUN_DRAGDROP_INTEREST, 0L, 1L,
			      False, _SUN_DRAGDROP_INTEREST, &actual_type,
			      &actual_format, &nitems, &bytes_remaining, 
			      (unsigned char **)&interest) != Success)
	{
		return(ret);
	}

/* mem leak in FindDropSiteInfoFromDestination	*/
	if (interest == (InterestPropertyPtr)NULL)
	{
		XtWarning("dst_win didn't register DnD Interest");
		return(ret);
	}

	XFree((char *)interest);

	if (bytes_remaining == 0)
	{
		XtWarning("dst_win contains incompleted DnD Interest info");
		return(ret);
	}

	if (XGetWindowProperty(dpy, dst_win, _SUN_DRAGDROP_INTEREST,0L,
			      1 + bytes_remaining / 4, False, actual_type,
			      &actual_type, &actual_format, &nitems,
			      &bytes_remaining,
			      (unsigned char **)&interest) != Success)
	{
		return(ret);
	}

	if (interest == (InterestPropertyPtr)NULL ||
	    InterestPropertyPtrVersionNumber(interest) != 0)
	{
		return(ret);	/* bad version...	*/
	}

		/* Gee, I can't handle this...		*/
	if (InterestPropertyPtrSiteCount(interest) != 1)
	{
		XtWarning("Site count of dst_win is not 1\n");
		return(ret);
	}

	site = InterestPropertyPtrSiteDescriptions(interest);
	dropsiteid = (OlDnDDropSiteID)SiteDescriptionPtrSiteID(site);
	window_id = SiteDescriptionPtrEventWindow(site);
	XFree((char *)interest);

	if (window_id != dst_win)
	{
		XtWarning("dst_win != window_id\n");
		return(ret);
	}

	/* O.K. now we can simulate this trigger...			*/
	/* the code is from DnDVCXDeliverTriggerMessage	and we can make	*/
	/* the following as a function...				*/

	/* we've got the drop site ..... lets do it! */

	
	TriggerMessageType(trigger_message) = _SUN_DRAGDROP_TRIGGER;

	TriggerMessageWindow(trigger_message) = window_id;

	TriggerMessageSiteID(trigger_message) = (unsigned long)dropsiteid;

	flags = OlDnDTriggerAck; /* well, we might as well! */

	switch (op) {
		case OlDnDTriggerCopyOp:
			flags |= OlDnDTriggerCopy;
			break;
#if defined(att) || defined(USL)
		case OlDnDTriggerLinkOp:
			flags |= OlDnDTriggerLink;
			break;
#endif
		default:
		case OlDnDTriggerMoveOp:
			flags |= OlDnDTriggerMove;
			break;
	}

	/* check if transient */

	if (_OlDnDAtomIsTransient(vendor, dnd_part, selection))
			flags |= OlDnDTriggerTransient;

	TriggerMessageFlags(trigger_message) = flags;

	TriggerMessageX(trigger_message) = -1;
	TriggerMessageY(trigger_message) = -1;

	TriggerMessageSelection(trigger_message) = selection;
	TriggerMessageTimestamp(trigger_message) = timestamp;

	/* try locally first */

	local_vendor = XtWindowToWidget(dpy, window_id);

	if (local_vendor != (Widget)NULL &&
	    XtIsSubclass(local_vendor, vendorShellWidgetClass) &&
	    (part = _OlGetDnDVendorPartExtension(local_vendor)) !=
					(OlDnDVendorPartExtension)NULL) {
		OlDnDVCXTMDispatcherFunc	tm_dispatcher;

		tm_dispatcher =
			part->class_extension->trigger_message_dispatcher;

		if (tm_dispatcher != (OlDnDVCXTMDispatcherFunc)NULL) {
			ret = (*tm_dispatcher)
				    (local_vendor, part, &trigger_message);
		}
	}

	if (!ret) {
		CopyTriggerMessageToClientMessage(&trigger_message,
						  &client_message);

		ret = XSendEvent(dpy, client_message.window, False,
				 NoEventMask, (XEvent *)&client_message);
	}
 
	return (ret);

} /* end of OlDnDSendTriggerMessage */
#endif

/*FTNPROTOB*/
Boolean
OlDnDDeliverTriggerMessage OLARGLIST(( widget, root, rootx, rooty, selection,
				       operation, timestamp ))
	OLARG( Widget,			widget)
	OLARG( Window,			root)
	OLARG( Position,		rootx)
	OLARG( Position,		rooty)
	OLARG( Atom,			selection)
	OLARG( OlDnDTriggerOperation,	operation)
	OLGRA(Time,			timestamp)
/*FTNPROTOE*/
{
	OlDnDVendorPartExtension	dnd_part;
	Widget				vendor;

#if !defined(att) && !defined(USL)
	if (!XtIsVendorShell(widget))
		vendor = GetShellOfWidget(widget);
#else
	vendor = GetShellOfWidget(widget);
#endif

	if (horrible_hack != (OlDnDVendorPartExtension)NULL &&
	    horrible_hack->owner == vendor) {
		dnd_part = horrible_hack;
	} else {
		horrible_hack = dnd_part = _OlGetDnDVendorPartExtension(vendor);
	}

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->deliver_trigger_message !=
	    (OlDnDVCXDeliverTMFunc)NULL)
		return (*dnd_part->class_extension->deliver_trigger_message)
			(vendor, dnd_part, widget, root, rootx,
			 rooty, selection, operation, timestamp);
	else return (False);
}

/********************************************
 *
 * OlDnDDeliverPreviewMessage
 *
 ********************************************/

/*FTNPROTOB*/
Boolean
OlDnDDeliverPreviewMessage OLARGLIST(( widget, root, rootx, rooty, timestamp ))
	OLARG( Widget,			widget)
	OLARG( Window,			root)
	OLARG( Position,		rootx)
	OLARG( Position,		rooty)
	OLGRA(Time,			timestamp)
/*FTNPROTOE*/
{
	OlDnDVendorPartExtension	dnd_part;
	Widget				vendor;

#if !defined(att) && !defined(USL)
	if (!XtIsVendorShell(widget))
		vendor = GetShellOfWidget(widget);
#else
	vendor = GetShellOfWidget(widget);
#endif

	if (horrible_hack != (OlDnDVendorPartExtension)NULL &&
	    horrible_hack->owner == vendor) {
		dnd_part = horrible_hack;
	} else {
		horrible_hack = dnd_part = _OlGetDnDVendorPartExtension(vendor);
	}

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->deliver_preview_message !=
	    (OlDnDVCXDeliverPMFunc)NULL)
		return (*dnd_part->class_extension->deliver_preview_message)
			(vendor, dnd_part, widget, root, rootx,
			 rooty, timestamp, 
			(OlDnDPreviewAnimateCallbackProc)NULL, (XtPointer)NULL);
	else return (False);
}

/********************************************
 *
 * OlDnDPreviewAndAnimate
 *
 ********************************************/

/*FTNPROTOB*/
Boolean
OlDnDPreviewAndAnimate OLARGLIST(( widget, root, rootx, rooty, timestamp,
				   animate_proc, closure ))
	OLARG( Widget,				widget)
	OLARG( Window,				root)
	OLARG( Position,			rootx)
	OLARG( Position,			rooty)
	OLARG( Time,				timestamp)
	OLARG( OlDnDPreviewAnimateCbP,		animate_proc)
	OLGRA( XtPointer,			closure)
/*FTNPROTOE*/
{
	OlDnDVendorPartExtension	dnd_part;
	Widget				vendor;

#if !defined(att) && !defined(USL)
	if (!XtIsVendorShell(widget))
		vendor = GetShellOfWidget(widget);
#else
	vendor = GetShellOfWidget(widget);
#endif

	if (horrible_hack != (OlDnDVendorPartExtension)NULL &&
	    horrible_hack->owner == vendor) {
		dnd_part = horrible_hack;
	} else {
		horrible_hack = dnd_part = _OlGetDnDVendorPartExtension(vendor);
	}

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->deliver_preview_message !=
	    (OlDnDVCXDeliverPMFunc)NULL)
		return (*dnd_part->class_extension->deliver_preview_message)
			(vendor, dnd_part, widget, root, rootx,
			 rooty, timestamp, animate_proc, closure );
	else return (False);
}

/********************************************
 *
 * OlDnDInitializeDragState
 *
 ********************************************/

/*FTNPROTOB*/
Boolean
OlDnDInitializeDragState OLARGLIST(( widget ))
	OLGRA(Widget,			widget)
/*FTNPROTOE*/
{
	Widget				vendor;
	OlDnDVendorPartExtension	dnd_part;

	vendor = GetShellOfWidget(widget);

	horrible_hack = dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->initialize_drag_state !=
	    (OlDnDVCXInitializeDragStateFunc)NULL) {
		return(*dnd_part->class_extension->initialize_drag_state)
			(vendor, dnd_part);
	}

	return (False);
}

/********************************************
 *
 * OlDnDClearDragState
 *
 ********************************************/

/*FTNPROTOB*/
void
OlDnDClearDragState OLARGLIST(( widget ))
	OLGRA(Widget,	widget)
/*FTNPROTOE*/
{
	OlDnDVendorPartExtension	dnd_part;

	if (!XtIsVendorShell(widget))
		widget = GetShellOfWidget(widget);

	dnd_part = _OlGetDnDVendorPartExtension(widget);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->clear_drag_state !=
	    (OlDnDVCXClearDragStateProc)NULL) 
		(*dnd_part->class_extension->clear_drag_state)
					    (widget, dnd_part);
}

/********************************************
 *
 * OlDnDGetCurrentSelectionsForWidget
 *
 ********************************************/

/*FTNPROTOB*/
Boolean     
OlDnDGetCurrentSelectionsForWidget OLARGLIST(( widget, 
					       atoms_return,
					       num_sites_return))
			OLARG( Widget,		 widget)
			OLARG( Atom **,		 atoms_return)
			OLGRA( Cardinal *,	 num_sites_return)
/*FTNPROTOE*/
{
	Widget			 vendor;
	OlDnDVendorPartExtension dnd_part;

	vendor = GetShellOfWidget(widget);
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->get_w_current_selections !=
	    (OlDnDVCXGetCurrentSelectionsFunc)NULL)
		*atoms_return = (*dnd_part->class_extension->get_w_current_selections)
					(vendor, dnd_part, widget, num_sites_return);
	else {
		*num_sites_return = (Cardinal)NULL;
		*atoms_return = (Atom *)NULL;
	}
	return (*atoms_return != (Atom *)NULL);
}

/********************************************
 *
 * OlDnDChangeDropSitePreviewHints
 *
 ********************************************/

/*FTNPROTOB*/
Boolean     
OlDnDChangeDropSitePreviewHints OLARGLIST(( dropsiteid, hints ))
			OLARG( OlDnDDropSiteID,		dropsiteid)
			OLGRA( OlDnDSitePreviewHints,	hints)
/*FTNPROTOE*/
{
	OlDnDDropSitePtr	 dsp = (OlDnDDropSitePtr)dropsiteid;
	Widget			 vendor;
	OlDnDVendorPartExtension dnd_part;

	vendor = GetShellOfWidget(OlDnDDropSitePtrOwner(dsp));
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->change_site_hints !=
	    (OlDnDVCXChangeSitePreviewHintsFunc)NULL) {
		return (*dnd_part->class_extension->change_site_hints)
					(vendor, dnd_part, dropsiteid, hints);
	} else
		return (False);
}

/********************************************
 *
 * OlDnDSetDropSiteInterest
 *
 ********************************************/

/*FTNPROTOB*/
Boolean     
OlDnDSetDropSiteInterest OLARGLIST(( dropsiteid, on_interest ))
				OLARG( OlDnDDropSiteID,	dropsiteid)
				OLGRA( Boolean,		on_interest)
/*FTNPROTOE*/
{

	OlDnDDropSitePtr	 dsp = (OlDnDDropSitePtr)dropsiteid;
	Widget			 vendor;
	OlDnDVendorPartExtension dnd_part;

	vendor = GetShellOfWidget(OlDnDDropSitePtrOwner(dsp));
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->set_ds_on_interest !=
	    (OlDnDVCXSetDSOnInterestFunc)NULL) {
		return (*dnd_part->class_extension->set_ds_on_interest)
				(vendor, dnd_part, dropsiteid,
				 on_interest, True);
	} else
		return (False);
}

/********************************************
 *
 * OlDnDSetInterestInWidgetHier
 *
 ********************************************/

/*FTNPROTOB*/
void
OlDnDSetInterestInWidgetHier OLARGLIST(( widget, on_interest ))
				OLARG( Widget,	widget)
				OLGRA( Boolean,	on_interest)
/*FTNPROTOE*/
{

	Widget			 vendor;
	OlDnDVendorPartExtension dnd_part;

	vendor = GetShellOfWidget(widget);
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->set_interest_in_widget_hier !=
	    (OlDnDVCXSetInterestWidgetHierFunc)NULL) {
		(*dnd_part->class_extension->set_interest_in_widget_hier)
				(vendor, dnd_part, widget, on_interest);
	}
}

/************************** private functions *****************************/

/********************************************
 *
 *	GetShellParentOfWindow
 *
 ********************************************/

static Widget
GetShellParentOfWindow OLARGLIST ((dpy,  window ))
	OLARG(Display *,	dpy)
	OLGRA(Window,		window)
{
	Widget				widget;
	Window				root, parent, w, *children;
	unsigned int			nchildren;

	for (w = window;
	     w != (Window)NULL &&
	     (widget = XtWindowToWidget(dpy, w)) != (Widget)NULL;
	     w = parent) {
		if (!XQueryTree(dpy, w, &root, &parent, &children, &nchildren)) {
			return (Widget)NULL;
		}
		XtFree((char *)children);
	} 

	if (widget == (Widget)NULL) {
		return widget;
	}

	return(GetShellOfWidget(widget));
}

/********************************************
 *
 * InsertSiteRectsInDropSiteList
 *
 * insert the list of drop sites into the
 * dispatcher list
 *
 ********************************************/

#define	EQSITE(s1, s2)						\
	((InternalDSRPtrTopLevelRectX(s1) ==			\
	  InternalDSRPtrTopLevelRectX(s2))		&&	\
	 (InternalDSRPtrTopLevelRectY(s1) ==			\
	  InternalDSRPtrTopLevelRectY(s2))		&&	\
	 (InternalDSRPtrTopLevelRectWidth(s1) ==		\
	  InternalDSRPtrTopLevelRectWidth(s2))	&&		\
	 (InternalDSRPtrTopLevelRectHeight(s1) ==		\
	  InternalDSRPtrTopLevelRectHeight(s2)))

#define	GESITE(s1, s2)						\
	((InternalDSRPtrTopLevelRectX(s1) >=			\
	  InternalDSRPtrTopLevelRectX(s2))		&&	\
	 (InternalDSRPtrTopLevelRectY(s1) >=			\
	  InternalDSRPtrTopLevelRectY(s2))		&&	\
	 (InternalDSRPtrTopLevelRectWidth(s1) >=		\
	  InternalDSRPtrTopLevelRectWidth(s2))	&&		\
	 (InternalDSRPtrTopLevelRectHeight(s1) >=		\
	  InternalDSRPtrTopLevelRectHeight(s2)))


static	void
InsertSiteRectsInDropSiteList OLARGLIST((dnd_part, sites, num_sites))
		OLARG( OlDnDVendorPartExtension,	dnd_part)
		OLARG( InternalDSRPtr,			sites)
		OLGRA( unsigned int,			num_sites)
{
	for (; num_sites--; sites++) {
		InternalDSRPtr	*rect = &dnd_part->dropsite_rects, next;

		for (; *rect; rect = &InternalDSRPtrNext(*rect))
			if (GESITE(sites, *rect)) break;

		if (*rect && EQSITE(sites, *rect)) continue;
		
		next = *rect;
		*rect = sites;
		InternalDSRPtrNext(sites) = next;

		if (next) {
			InternalDSRPtrPrev(sites) = InternalDSRPtrPrev(next);
			InternalDSRPtrPrev(next)  = sites;
		} else 
			InternalDSRPtrPrev(sites) = (InternalDSRPtr)NULL;
	}
}

#undef	EQSITE
#undef	GESITE

/********************************************
 *
 * LookupXYInDropSiteList
 *
 * find an InternalDSR which contains
 * the (x,y) pair.
 *
 ********************************************/

static InternalDSRPtr
LookupXYInDropSiteList OLARGLIST((dnd_part, x, y))
	OLARG( OlDnDVendorPartExtension,	dnd_part)
	OLARG( int,				x)
	OLGRA( int,				y)
{
	InternalDSRPtr	*site = &dnd_part->dropsite_rects;

	if (dnd_part->current_dsr != (InternalDSRPtr)NULL &&
	    XYInInternalDSR(dnd_part->current_dsr, x, y))
			return (dnd_part->current_dsr);
		
	while (*site) {
		if (XYInInternalDSR(*site, x, y))
			break;

		site = &InternalDSRPtrNext(*site);
	}
	return (*site);
}

/********************************************
 *
 * DeleteSiteRectsInDropSiteLIst
 *
 * need we say more ?
 *
 ********************************************/

static	void
DeleteSiteRectsInDropSiteList OLARGLIST((dnd_part, sites, num_sites))
		OLARG( OlDnDVendorPartExtension,	dnd_part)
		OLARG( InternalDSRPtr,			sites)
		OLGRA( unsigned int,			num_sites)
{
	for (; num_sites--; sites++) {
		InternalDSRPtr	*rect = &dnd_part->dropsite_rects, next;

		for (; *rect && *rect != sites;
		     rect = &InternalDSRPtrNext(*rect))
		;

		if (*rect) {
			next = InternalDSRPtrNext(sites);
			*rect = next;
			if (next)
				InternalDSRPtrPrev(next) =
					InternalDSRPtrPrev(sites);

			InternalDSRPtrNext(sites) = 
				InternalDSRPtrNext(sites) =
					(InternalDSRPtr)NULL;

			/* invalidate dsr cache */

			if (dnd_part->current_dsr == sites)
				dnd_part->current_dsr = (InternalDSRPtr)NULL;

		}
	}
}

/********************************************
 *
 * InsertSiteRectsInDropSiteList
 *
 * insert the list of drop sites into the
 * DSDM list
 *
 ********************************************/

#define	EQSITE(s1, s2)					\
	((InternalDSDMSRPtrRectX(s1) ==			\
	  InternalDSDMSRPtrRectX(s2))		 &&	\
	 (InternalDSDMSRPtrRectY(s1) ==			\
	  InternalDSDMSRPtrRectY(s2))		 &&	\
	 (InternalDSDMSRPtrRectWidth(s1) ==		\
	  InternalDSDMSRPtrRectWidth(s2))	 &&	\
	 (InternalDSDMSRPtrRectHeight(s1) ==		\
	  InternalDSDMSRPtrRectHeight(s2)))

#define	GESITE(s1, s2)					\
	((InternalDSDMSRPtrRectX(s1) >=			\
	  InternalDSDMSRPtrRectX(s2))		&&	\
	 (InternalDSDMSRPtrRectY(s1) >=			\
	  InternalDSDMSRPtrRectY(s2))		&&	\
	 (InternalDSDMSRPtrRectWidth(s1) >=		\
	  InternalDSDMSRPtrRectWidth(s2))	&&	\
	 (InternalDSDMSRPtrRectHeight(s1) >=		\
	  InternalDSDMSRPtrRectHeight(s2)))


static	void
InsertDSDMSRsIntoDSDMSRList OLARGLIST((dnd_part, dsdmsrs, num_dsdmsrs))
		OLARG( OlDnDVendorPartExtension,	dnd_part)
		OLARG( InternalDSDMSRPtr,		dsdmsrs)
		OLGRA( unsigned int,			num_dsdmsrs)
{
	unsigned int		noofscreens, screen;

	noofscreens = ScreenCount(XtDisplay(dnd_part->owner));

	if (dnd_part->dsdm_rects == (InternalDSDMSRPtr *)NULL) {
		dnd_part->dsdm_rects = 
				(InternalDSDMSRPtr *)XtCalloc(noofscreens,
						sizeof(InternalDSDMSRPtr));
	}

	for (; num_dsdmsrs--; dsdmsrs++) {
		InternalDSDMSRPtr	*rect = dnd_part->dsdm_rects, next;

		screen = InternalDSDMSRPtrRectScreenNumber(dsdmsrs);
		if (screen >= 0 && screen < noofscreens) {
			rect += screen;
		} else continue;

		for (; *rect; rect = &InternalDSDMSRPtrNext(*rect))
			if (GESITE(dsdmsrs, *rect)) break;

		if (*rect && EQSITE(dsdmsrs, *rect)) continue;
		
		next = *rect;
		*rect = dsdmsrs;
		InternalDSDMSRPtrNext(dsdmsrs) = next;

		if (next) {
			InternalDSDMSRPtrPrev(dsdmsrs) =
					InternalDSDMSRPtrPrev(next);
			InternalDSDMSRPtrPrev(next)  = dsdmsrs;
		} else 
			InternalDSDMSRPtrPrev(dsdmsrs) =
					(InternalDSDMSRPtr)NULL;
	}
}

#undef	EQSITE
#undef	GESITE

/********************************************
 *
 * LookupXYInDSDMSRList
 *
 * find an InternalDSDMSR which contains
 * the (x,y) pair.
 *
 ********************************************/

static InternalDSDMSRPtr
LookupXYInDSDMSRList OLARGLIST((dnd_part, screen, x, y))
	OLARG( OlDnDVendorPartExtension,	dnd_part)
	OLARG(Screen,				*screen)
	OLARG( int,				x)
	OLGRA( int,				y)
{
	InternalDSDMSRPtr	*site = dnd_part->dsdm_rects;
	unsigned int		noofscreens, scrn;

	noofscreens = ScreenCount(XtDisplay(dnd_part->owner));
	scrn = XScreenNumberOfScreen(screen);

	if (dnd_part->current_dsdmsr != (InternalDSDMSRPtr)NULL &&
	    XYInInternalDSDMSR(dnd_part->current_dsdmsr, scrn, x, y))
		return (dnd_part->current_dsdmsr);

	if (dnd_part->dsdm_rects == (InternalDSDMSRPtr *)NULL)
                return (InternalDSDMSRPtr)NULL;

	if (scrn >= 0 && scrn < noofscreens) {
		site += scrn;
	} else  return (InternalDSDMSRPtr)NULL;

	if (*site == (InternalDSDMSRPtr)NULL)
                return (InternalDSDMSRPtr)NULL;

	while (*site) {
		if (XYInInternalDSDMSR(*site, scrn, x, y))
			break;

		site = &InternalDSDMSRPtrNext(*site);
	}
	return (*site);
}

/********************************************
 *
 * DeleteDSDMSRList
 *
 * need we say more ?
 *
 ********************************************/

static	void
DeleteDSDMSRList OLARGLIST((dnd_part))
		OLGRA( OlDnDVendorPartExtension,	dnd_part)
{
	InternalDSDMSRPtr	*site = dnd_part->dsdm_rects;
	unsigned int		i, noofscreens;
   
	if (dnd_part->dsdm_rects == (InternalDSDMSRPtr *)NULL) return;
   
	noofscreens = ScreenCount(XtDisplay(dnd_part->owner));

	for (; site < dnd_part->dsdm_rects + noofscreens; site++) {
		if (*site != (InternalDSDMSRPtr)NULL) {
			InternalDSDMSRPtr	ptr = *site, next;
		  
			while (ptr) {
				next = InternalDSDMSRPtrNext(ptr);
				InternalDSDMSRPtrPrev(ptr) =
					InternalDSDMSRPtrNext(ptr) =
						(InternalDSDMSRPtr)NULL;
				ptr = next;
			  }
		}
	}

	XtFree((char *)dnd_part->dsdm_rects);
	dnd_part->dsdm_rects = (InternalDSDMSRPtr *)NULL;
	dnd_part->current_dsdmsr = (InternalDSDMSRPtr)NULL;
}

/************************************************************
 *
 *	DnDFetchRootCoords()
 *
 *	update the extension instances root_x and root_y 
 * 	resources.
 *
 ************************************************************/

static void
DnDFetchRootCoords OLARGLIST (( vendor, dnd_part ))
	OLARG( Widget,				vendor )
	OLGRA( OlDnDVendorPartExtension,	dnd_part )
{
	int	rx, ry;
	Screen	*screen = vendor->core.screen;
	Window	child;

	FetchPartExtensionIfNull(vendor, dnd_part);

	if (XTranslateCoordinates(DisplayOfScreen(screen), XtWindow(vendor), 
				   RootWindowOfScreen(screen), 0, 0,
				   &rx, &ry, &child)) {
		dnd_part->root_x = (Position)rx;
		dnd_part->root_y = (Position)ry;
	}
}

/************************************************************
 *
 *	MapLocalCoordsToTopLevel()
 *
 *	Map the site rect coords in the local coordinate
 *	space to theat of the vendor vendor
 *
 *	NOTE: widgets should be realised before calling this
 *	      function ....
 *
 ************************************************************/

static void
MapLocalCoordsToTopLevel OLARGLIST(( vendor, dnd_part, widget, window,
				     site_rect, return_rect ))
		OLARG( Widget,				vendor)
		OLARG( OlDnDVendorPartExtension,	dnd_part)
		OLARG( Widget,				widget)
		OLARG( Window,				window)
		OLARG( OlDnDSiteRectPtr,		site_rect)
		OLGRA( OlDnDSiteRectPtr,		return_rect)
{
	int	x, y;

	FetchPartExtensionIfNull(vendor, dnd_part);

	x = (int)SiteRectPtrX(site_rect);
	y = (int)SiteRectPtrY(site_rect);

	if (window != XtWindowOfObject(widget)) {	/* client window */
		Window		dummy;

		XTranslateCoordinates(XtDisplay(vendor), window, 
				      XtWindow(vendor), x, y, &x, &y, 
				      &dummy);
	} else { /* widget or gadget */
		register RectObj rect = (RectObj)widget;

#define		Adjust(g)	\
		(g += (rect->rectangle.g + rect->rectangle.border_width))

		for (rect = (RectObj)widget;
		     rect != (RectObj)vendor;
		     rect = (RectObj)XtParent((Widget)rect)) {
			Adjust(x);
			Adjust(y);
		}
	}

	SiteRectPtrX(return_rect)      = (Position)x;
	SiteRectPtrY(return_rect)      = (Position)y;
	SiteRectPtrWidth(return_rect)  = SiteRectPtrWidth(site_rect);
	SiteRectPtrHeight(return_rect) = SiteRectPtrHeight(site_rect);
}

 /***********************************************************************
 *
 * DnDVCXEventHandler
 *
 * This is the event handler which dispatches incoming events on Vendor 
 * Shell widgets for the purposes of Drag and Drop.
 *
 *************************************************************************/

static void
DnDVCXEventHandler OLARGLIST((widget, client_data, xevent,
			      continue_to_dispatch))
	OLARG( Widget, 		widget )
	OLARG( XtPointer,	client_data )
	OLARG( XEvent,		*xevent )
	OLGRA( Boolean,		*continue_to_dispatch )
{
	Widget				vendor;
	OlDnDVendorPartExtension	dnd_part;
	OlDnDVendorClassExtension	dnd_class;

	if ((vendor = GetShellOfWidget(widget)) == widget) {    
		dnd_part = (OlDnDVendorPartExtension)client_data;
	} else {
		dnd_part = _OlGetDnDVendorPartExtension(vendor);
	}

	dnd_class = _OlGetDnDVendorClassExtension(vendor->core.widget_class);
	
	if (dnd_class == (OlDnDVendorClassExtension)NULL || 
	    dnd_part  == (OlDnDVendorPartExtension)NULL) {
			return; /* its all gone horribly wrong! */
	}

	if (xevent->xany.type == ClientMessage) {
		if (xevent->xclient.message_type == _SUN_DRAGDROP_PREVIEW) {
			PreviewMessage	preview;

			CopyPreviewMessageFromClientMessage(&preview,
					 		    &(xevent->xclient));

			if (dnd_class->preview_message_dispatcher !=
			    (OlDnDVCXPMDispatcherFunc)NULL) {
				(*dnd_class->preview_message_dispatcher)
					(vendor, dnd_part, &preview);
			}

			return;
		} /* preview message */

		if (xevent->xclient.message_type == _SUN_DRAGDROP_TRIGGER) {
			TriggerMessage	trigger;

			CopyTriggerMessageFromClientMessage(&trigger,
							    &(xevent->xclient));

			if (dnd_class->trigger_message_dispatcher !=
			    (OlDnDVCXTMDispatcherFunc)NULL) {
				(*dnd_class->trigger_message_dispatcher)
					(vendor, dnd_part, &trigger);
			}

			return;
		} /* trigger message */

		return;
	} /* ClientMessage */

	if (xevent->xany.type == PropertyNotify && 
	    (xevent->xproperty.atom == _SUN_DRAGDROP_INTEREST &&
	     xevent->xproperty.state == PropertyNewValue)) {
		dnd_part->registry_update_timestamp = xevent->xproperty.time;

		if (dnd_part->dirty && dnd_part->auto_assert_dropsite_registry &&
		    dnd_class->assert_drop_site_registry != (OlDnDVCXAssertRegistryProc)NULL) {
			(*dnd_class->assert_drop_site_registry)(vendor, dnd_part);
		} /* its changed since we last updated it!!! phew ... */

		return;
	} /* PropertyNotify */

#if	0
	if (xevent->xany.type == FocusIn) {
		if (dnd_part->class_extension->fetch_dsdm_info !=
		    (OlDnDVCXFetchDSDMInfoFunc)NULL)
			(*dnd_part->class_extension->fetch_dsdm_info)
					(vendor, dnd_part, 
					 XtLastTimestampProcessed(
						 XtDisplay(vendor)));
	}
#endif
}

/******************************************************************************
 *
 * FindDropSiteInfoFromDestination
 *
 ******************************************************************************/

static	Boolean
FindDropSiteInfoFromDestination OLARGLIST((screen, root_window, root_x,
					   root_y, site_found))
	OLARG(Screen *,			screen)
	OLARG(Window,			root_window)
	OLARG(int,			root_x)
	OLARG(int,			root_y)
	OLGRA(SiteDescriptionPtr,	*site_found)
{
	Window			w, child,
				toplevel = (Window)NULL;
	Window			*children;
	unsigned int		nchildren;
	Display			*dpy = DisplayOfScreen(screen);
	int			x, y;
	int			top_x, top_y;
	unsigned int		i;

	Atom			actual_type;
	int			actual_format;
	unsigned long		nitems;
	unsigned long		bytes_remaining;
	InterestPropertyPtr	interest = (InterestPropertyPtr)NULL;
	SiteDescriptionPtr	site = (SiteDescriptionPtr)NULL;
	unsigned int		sizeof_site = 0;

	/*
	 * welcome to an expensive function ... so whats a few round trips
	 * between clients and servers?
	 */

	children = (Window *)XtMalloc(sizeof(Window));
	children[0] = root_window;
	nchildren = 1;

	for (;;) {
		Atom		*props;
		unsigned int	num_props, propsfound = 0;

		for (i = 0; i < nchildren; i++) {
			w = children[i];
			if (!XTranslateCoordinates(dpy, root_window,
						   w, root_x, root_y,
						   &x, &y, &child))
			continue;

			if (child != None) break;
		}
		XtFree((char *)children);

		if (i == nchildren) break; /* bale out */

		props = XListProperties(dpy, w, (int *)&num_props);

		for (i = 0; i < num_props; i++) {
			if (props[i] == _SUN_DRAGDROP_INTEREST) {
				if (propsfound++) 
					break; 
				else continue;
			}

#define WM_STATE	XInternAtom(dpy, "WM_STATE", False)

			if (props[i] == WM_STATE) {
				if (propsfound++) 
					break; 
				else continue;
			}
		}

		if (props) XtFree((char *)props);

		if (propsfound == 2) {
			toplevel = w;
			break;
		}

		if (!XQueryTree(dpy, child, &root_window, &w,
			        &children, &nchildren))
			break; /* bale out */
	}

	if (toplevel == (Window)NULL) 
		toplevel = root_window;

	if (XGetWindowProperty(dpy, toplevel, _SUN_DRAGDROP_INTEREST, 0L, 1L,
			      False, _SUN_DRAGDROP_INTEREST, &actual_type,
			      &actual_format, &nitems, &bytes_remaining, 
			      (unsigned char **)&interest))
		goto failed;

#if defined(att) || defined(USL)
		/* memory leak fix...	*/
	if (interest != NULL)
	    XFree((char *)interest);
#endif

	if (XGetWindowProperty(dpy, toplevel, _SUN_DRAGDROP_INTEREST,0L,
			      1 + bytes_remaining / 4, False, actual_type,
			      &actual_type, &actual_format, &nitems,
			      &bytes_remaining,
			      (unsigned char **)&interest))
		goto failed;

	if (interest == (InterestPropertyPtr)NULL || 
	    InterestPropertyPtrVersionNumber(interest) != 0)
		goto failed; /* bad version */

	if (!XTranslateCoordinates(dpy, toplevel, root_window,
				   0, 0, &top_x, &top_y, &w)) {
	}

	x = root_x - top_x; /* adjust co-ordinates */
	y = root_y - top_y;

	for (i = 0, site = InterestPropertyPtrSiteDescriptions(interest);
	     i < InterestPropertyPtrSiteCount(interest); i++) {
		unsigned int		p, offset;
		AreaListPtr		area = &SiteDescriptionPtrAreas(site);

		if (SiteDescriptionPtrAreaIsRectList(site)) {
			unsigned int	nrects = NumOfRectsInAreaListPtr(area);
			RectListPtr	rect = &AreaListPtrRectList(area);

			offset = SizeOfSiteDescriptionForNRects(nrects);

			for (p = 0; p < nrects; p++) {
			    if ((x >= SiteRectX(RectListPtrRects(rect)[p]) &&
			         x <= (SiteRectX(RectListPtrRects(rect)[p]) +
				      SiteRectWidth(RectListPtrRects(rect)[p]))) &&
			       (y >= SiteRectY(RectListPtrRects(rect)[p]) &&
			        y <= (SiteRectY(RectListPtrRects(rect)[p]) +
				      SiteRectHeight(RectListPtrRects(rect)[p])))) {
					sizeof_site = offset;
					goto found;
			     }
			}
		} else if (SiteDescriptionPtrAreaIsWindowList(site)) {
			XWindowAttributes	wa;
			unsigned int		nwindows =
						NumOfWindowsInAreaListPtr(area);
			WindowListPtr		windows;	

			for (p = 0; p < nwindows; p++ ) {
				int	wx, wy;

				if (XGetWindowAttributes(dpy, 
					WindowListPtrWindows(windows)[p], &wa)
				    != Success)
					continue;

				if (!XTranslateCoordinates(dpy, 
					WindowListPtrWindows(windows)[p],
					root_window, 0, 0, &wx, &wy, &w))
					continue;

				if ((x >= wx && x <= (wx + wa.width)) &&
				    (y >= wy && y <= (wy + wa.height))) {
					sizeof_site = offset;
					goto found;
				}
			}

			offset = SizeOfSiteDescriptionForNWindows(nwindows);
		} else {
			site = (SiteDescriptionPtr)NULL;
			break; 
		}

		site = (SiteDescriptionPtr)((unsigned char *)site + offset);
	}

found:	
	if (interest != (InterestPropertyPtr)NULL &&
	    i < InterestPropertyPtrSiteCount(interest)) {
		*site_found = (SiteDescriptionPtr)XtCalloc(1, sizeof_site);
		memcpy((char *)*site_found, (char *)site, sizeof_site);
	} else 
		failed: site = *site_found = (SiteDescriptionPtr)NULL;

	if (interest != (InterestPropertyPtr)NULL)
		XtFree((char *)interest);

	return (site != (SiteDescriptionPtr)NULL);
}


/********************************************
 *
 * _OlDnDAssocSelectionWithWidget
 *
 ********************************************/

static DSSelectionAtomPtr
_OlDnDAssocSelectionWithWidget OLARGLIST(( vendor, dnd_part, widget, 
					     selection, timestamp, closure ))
		OLARG( Widget,				vendor)
		OLARG( OlDnDVendorPartExtension,	dnd_part)
		OLARG( Widget,				widget)
		OLARG( Atom,				selection)
		OLARG( Time,				timestamp)
		OLGRA( OwnerProcClosurePtr,		closure)
{
	DSSelectionAtomPtr	sel;

	for (;;) {
		if (dnd_part->class_extension->associate_selection_and_w !=
		    (OlDnDVCXAssocSelectionFunc)NULL)
			sel = (*dnd_part->class_extension->associate_selection_and_w)
					(vendor, dnd_part, widget, selection, timestamp, closure);

		if (sel != (DSSelectionAtomPtr)NULL &&
		    DSSelectionAtomPtrOwner(sel) != widget)
			OlDnDDisownSelection(DSSelectionAtomPtrOwner(sel),
					     selection, timestamp);
		else
			return (sel);
	}
}
	
/********************************************
 *
 * _OlDnDDisassocSelectionWithWidget
 *
 ********************************************/

static void
_OlDnDDisassocSelectionWithWidget OLARGLIST(( vendor, dnd_part, widget, 
					      selection, timestamp))
		OLARG( Widget,				vendor)
		OLARG( OlDnDVendorPartExtension,	dnd_part)
		OLARG( Widget,				widget)
		OLARG( Atom,				selection)
		OLGRA( Time,				timestamp)
{
	if (dnd_part->class_extension->disassociate_selection_and_w !=
		    (OlDnDVCXDisassocSelectionProc)NULL)
		(*dnd_part->class_extension->disassociate_selection_and_w)
			(vendor, dnd_part, widget, selection, timestamp);

}

/********************************************
 *
 * _OlDnDGetWidgetForSelection
 *
 ********************************************/

static Widget
_OlDnDGetWidgetForSelection OLARGLIST(( vendor, dnd_part, selection, closure ))
		OLARG( Widget,			 vendor)
		OLARG( OlDnDVendorPartExtension, dnd_part)
		OLARG( Atom,			 selection)
		OLGRA( OwnerProcClosurePtr *,	closure)
{
	if (dnd_part->class_extension->get_w_for_selection !=
	    (OlDnDVCXGetSelectionFunc)NULL)
		return (*dnd_part->class_extension->get_w_for_selection)
				(vendor, dnd_part, selection, closure);
	else {
		*closure = (OwnerProcClosurePtr)NULL;
		return (Widget)NULL;
	}
}

/********************************************
 *
 * _OlDnDAtomIsTransient
 *
 ********************************************/

static Boolean
_OlDnDAtomIsTransient OLARGLIST(( vendor, dnd_part, atom ))
		OLARG( Widget,			 vendor)
		OLARG( OlDnDVendorPartExtension, dnd_part)
		OLGRA( Atom,			 atom)
{
	TransientAtomListPtr	transients;
	unsigned int		i;

	if ((transients = dnd_part->transient_atoms) !=
	    (TransientAtomListPtr)NULL) {
		for (i = 0; i < TransientAtomListPtrAlloc(transients); i++)
			if (atom == TransientAtomListPtrAtom(transients, i)) {
				return (True);
			}
	}
	return (False);
}

/********************************************
 *
 * _OlDnDResourceDependencies
 *
 * this function bears some resemblance
 * to _XtDependencies() except that it
 * doesnt create the indirection table
 * since XtGetSubresources does this for 
 * you ...... blah ....
 *
 ********************************************/

static void
_OlDnDResourceDependencies OLARGLIST(( class_res, class_num_res,
				       super_res, super_num_res, 
				       super_ext_size ))
		OLARG(XtResourceList *,		class_res)
		OLARG(Cardinal *,		class_num_res)
		OLARG(XrmResourceList,		super_res)
		OLARG(Cardinal,			super_num_res)
		OLGRA(Cardinal,			super_ext_size)
{
	register Cardinal	i,j;
	Cardinal		new_next;
	Cardinal		new_num_res;
	XrmResourceList		new_res, class_xrm_res;

	if (*class_res != (XtResourceList)NULL &&
	    (int)(*class_res)->resource_offset > 0) {
#if defined(XtSpecificationRelease) && XtSpecificationRelease >= 5
		_XtCompileResourceList(*class_res, *class_num_res);
#else
		XrmCompileResourceList(*class_res, *class_num_res);
#endif
	}
	class_xrm_res = (XrmResourceList)*class_res;

	new_num_res = super_num_res + *class_num_res;
	if (new_num_res > 0) {
		new_res = (XrmResourceList)XtCalloc(new_num_res, 
					            sizeof(XrmResource));
	} else
		new_res = (XrmResourceList)NULL;

	if (super_res != (XrmResourceList)NULL) {
		memcpy((char *)new_res, (char *)super_res,
		       super_num_res * sizeof(XrmResource));
	}

	if (*class_res == (XtResourceList)NULL) {
		*class_res = (XtResourceList)new_res;
		*class_num_res = new_num_res;
		return;
	}

	new_next = super_num_res;
	for (i = 0; i < *class_num_res; i++) {
		if (-class_xrm_res[i].xrm_offset-1 < super_ext_size) {
			for (j = 0; j < super_num_res; j++) {
				if (class_xrm_res[i].xrm_offset ==
				    new_res[j].xrm_offset) {
					class_xrm_res[i].xrm_size =
						new_res[j].xrm_size;

					new_res[j] = class_xrm_res[i];
					new_num_res--;
					goto NextResource;
				}
			} /* for j */
		} /* if */
		new_res[new_next++] = class_xrm_res[i];
NextResource:;
	} /* for i */

	*class_res = (XtResourceList)new_res;
	*class_num_res = new_num_res;
}

/********************************* Class Methods *****************************/

/*******************************************
 *
 * DnDVCXClassInitialize
 *
 *******************************************/

static void
DnDVCXClassInitialize OLARGLIST((extension))
	OLGRA(OlDnDVendorClassExtension,	extension)
{
	OlXrmDnDVendorClassExtension =
                        XrmStringToQuark(OlDnDVendorClassExtensionName);

        extension->header.record_type = OlXrmDnDVendorClassExtension;
}

/*******************************************
 *
 * DnDVCXClassPartInitialize
 *
 *******************************************/

static void
DnDVCXClassPartInitialize OLARGLIST((wc))
	OLGRA(WidgetClass,	wc)
{
        Cardinal                	size;
        OlDnDVendorClassExtension       super_ext;
        OlDnDVendorClassExtension       ext;
	Cardinal			super_num_res;
	Cardinal			super_ext_size;
	XrmResourceList			super_res;
	Boolean				super_null;
	Boolean				copied_super;

#if defined(att) || defined(USL)
		/* don't go furthur if this is the case...	*/
	if (wc == vendorShellWidgetClass)
		super_ext = (OlDnDVendorClassExtension)NULL;
	else
		super_ext = GET_DND_EXT(wc->core_class.superclass);
#else
	super_ext = GET_DND_EXT(wc->core_class.superclass);
#endif
	ext =       GET_DND_EXT(wc);

	if ((super_null = (super_ext == (OlDnDVendorClassExtension)NULL)) &&
	    ext == (OlDnDVendorClassExtension)NULL) {
		return;	/*oops*/
	}

	if ((copied_super = (ext == (OlDnDVendorClassExtension)NULL))) {
                size = sizeof(OlDnDVendorClassExtensionRec);
                ext = (OlDnDVendorClassExtension)XtMalloc(size);

                (void)memcpy((char *)ext, (OLconst char *)super_ext, 
			     (int)size);

                ext->header.next_extension = VCLASS(wc, extension);
                VCLASS(wc, extension) = (XtPointer)ext;

		/* if this is a copy zero out class functions to 
	         * stop multiple calls .... due to chaining.
		 */

		ext->class_initialize = (OlDnDVCXClassInitializeProc)NULL;
		ext->class_part_initialize = 
					(OlDnDVCXClassPartInitializeProc)NULL;
		ext->get_values = (OlDnDVCXGetValuesProc)NULL;
		ext->set_values = (OlDnDVCXSetValuesFunc)NULL;
		ext->destroy = (OlDnDVCXDestroyProc)NULL;

		ext->resources = (XtResourceList)NULL;
		ext->num_resources = (Cardinal)NULL;
	}

	ext->instance_part_list = (OlDnDVendorPartExtension)NULL;

	if (super_null) {
		super_ext_size = super_num_res = (Cardinal)0;
		super_res = (XrmResourceList)NULL;
	} else {
		super_ext_size = super_ext->instance_part_size;
		super_num_res = super_ext->num_resources;
		super_res = (XrmResourceList)super_ext->resources;
	}

	_OlDnDResourceDependencies(&ext->resources, &ext->num_resources,
				   super_res, super_num_res, super_ext_size);

	if (super_null || copied_super)
		return;

#ifndef	__STDC__
#define	INHERITPROC(ext, super, field, PROCNAME)	\
	if (ext->field == XtInherit/**/PROCNAME)	\
		ext->field = super->field
#else
#define	INHERITPROC(ext, super, field, PROCNAME)	\
	if (ext->field == XtInherit##PROCNAME)		\
		ext->field = super->field
#endif

	/* note that no spaces must appear in the actual param PROCNAME
	   when invoking this macro otherwise the correct concatentation
	   will not take place
         */

	INHERITPROC(ext, super_ext, 
		    post_realize_setup,OlDnDVCXPostRealizeSetupProc);
	INHERITPROC(ext, super_ext,
		    register_drop_site,OlDnDVCXRegisterDSFunc);
	INHERITPROC(ext, super_ext,
		    update_drop_site_geometry,OlDnDVCXUpdateDSGeometryProc);
	INHERITPROC(ext, super_ext, delete_drop_site,OlDnDVCXDeleteDSProc);
	INHERITPROC(ext, super_ext,
		    query_drop_site_info,OlDnDVCXQueryDSInfoFunc);
	INHERITPROC(ext, super_ext,
		    assert_drop_site_registry,OlDnDVCXAssertRegistryProc);
	INHERITPROC(ext, super_ext,
		    delete_drop_site_registry,OlDnDVCXDeleteRegistryProc);
	INHERITPROC(ext, super_ext,
		    fetch_dsdm_info,OlDnDVCXFetchDSDMInfoFunc);
	INHERITPROC(ext, super_ext,
		    trigger_message_dispatcher,OlDnDVCXTMDispatcherFunc);
	INHERITPROC(ext, super_ext,
		    preview_message_dispatcher,OlDnDVCXPMDispatcherFunc);
	INHERITPROC(ext, super_ext,
		    deliver_trigger_message,OlDnDVCXDeliverTMFunc);
	INHERITPROC(ext, super_ext,
		    deliver_preview_message,OlDnDVCXDeliverPMFunc);
	INHERITPROC(ext, super_ext,
		    initialize_drag_state,OlDnDVCXInitializeDragStateFunc);
	INHERITPROC(ext, super_ext,
		    clear_drag_state,OlDnDVCXClearDragStateProc);
	INHERITPROC(ext, super_ext,
		    alloc_transient_atom,OlDnDVCXAllocTransientAtomFunc);
	INHERITPROC(ext, super_ext,
		    free_transient_atom,OlDnDVCXFreeTransientAtomProc);
	INHERITPROC(ext, super_ext,
		    associate_selection_and_w,OlDnDVCXAssocSelectionFunc);
	INHERITPROC(ext, super_ext, disassociate_selection_and_w
		    ,OlDnDVCXDisassocSelectionProc);
	INHERITPROC(ext, super_ext, get_w_current_selections
		    ,OlDnDVCXGetCurrentSelectionsFunc);
	INHERITPROC(ext, super_ext,
		    get_w_for_selection,OlDnDVCXGetSelectionFunc);
	INHERITPROC(ext, super_ext,
		    change_site_hints,OlDnDVCXChangeSitePreviewHintsFunc);
	INHERITPROC(ext, super_ext,
		    set_ds_on_interest,OlDnDVCXSetDSOnInterestFunc);
	INHERITPROC(ext, super_ext,
		    set_interest_in_widget_hier,OlDnDVCXSetInterestWidgetHierFunc);
#undef	INHERITPROC
}

/******************************************************************************
 *
 * DnDVCXSetValues
 *
 ******************************************************************************/

static	Boolean
DnDVCXSetValues OLARGLIST(( current, request, new, args, num_args, cur_part,
			    req_part, new_part))
	OLARG( Widget,			current)
	OLARG( Widget,			request)
	OLARG( Widget,			new)
	OLARG( ArgList,			args)
	OLARG( Cardinal *,		num_args)
	OLARG(OlDnDVendorPartExtension,	cur_part)
	OLARG(OlDnDVendorPartExtension,	req_part)
	OLGRA(OlDnDVendorPartExtension,	new_part)
{

	/* enforce read-only resources */

	new_part->root_x = cur_part->root_x;
	new_part->root_y = cur_part->root_y;
	new_part->dirty  = cur_part->dirty;

	new_part->dsdm_present = cur_part->dsdm_present;
	new_part->doing_drag   = cur_part->doing_drag;

	new_part->number_of_sites           = cur_part->number_of_sites;
	new_part->registry_update_timestamp =
					cur_part->registry_update_timestamp;
	new_part->dsdm_last_loaded = cur_part->dsdm_last_loaded;

	if (new_part->default_drop_site != cur_part->default_drop_site) {
		OlDnDDropSitePtr			odsp, ndsp, p;
		OlDnDVCXChangeSitePreviewHintsFunc	cspftn;
		Boolean					save;

		cspftn = cur_part->class_extension->change_site_hints;

		if (cspftn == (OlDnDVCXChangeSitePreviewHintsFunc)NULL) {
			new_part->default_drop_site =
				cur_part->default_drop_site;
			/* cant effect the change */
			goto foo;
		}

		odsp = ndsp = (OlDnDDropSitePtr)NULL;

		for (p = new_part->drop_site_list;
		     p != (OlDnDDropSitePtr)NULL;
		     p = OlDnDDropSitePtrNextSite(p)) {
			if (p == cur_part->default_drop_site)
				odsp = p;
			if (p == new_part->default_drop_site)
				ndsp = p;

		}

		save = new_part->auto_assert_dropsite_registry;
		new_part->auto_assert_dropsite_registry = False;

		if (odsp != (OlDnDDropSitePtr)NULL &&
		    ndsp == new_part->default_drop_site) {
			OlDnDSitePreviewHints 	hints;

			hints = OlDnDDropSitePtrPreviewHints(odsp) &
				~OlDnDSitePreviewDefaultSite;
			(*cspftn)(new, new_part, (OlDnDDropSiteID)odsp,
				  hints);
		}

		if (ndsp != (OlDnDDropSitePtr)NULL) {
			OlDnDSitePreviewHints 	hints;

			hints = OlDnDDropSitePtrPreviewHints(ndsp) |
				OlDnDSitePreviewDefaultSite;
			(*cspftn)(new, new_part, (OlDnDDropSiteID)ndsp,
				  hints);
		}

		new_part->auto_assert_dropsite_registry = save;

		foo:;
	}

	if (new_part->auto_assert_dropsite_registry &&
	    new_part->dirty)
		(*cur_part->class_extension->assert_drop_site_registry)
			(new, new_part);
		

	return (False);
}

/******************************************************************************
 *
 * DnDVCXGetValues
 *
 ******************************************************************************/

static	void
DnDVCXGetValues OLARGLIST(( current, args, num_args, cur_part))
	OLARG( Widget,				current)
	OLARG( ArgList, 			args)
	OLARG( Cardinal *,			num_args)
	OLGRA( OlDnDVendorPartExtension,	cur_part)
{
	Boolean		fetch = False;
	Position	*x = (Position*)NULL, *y = (Position *)NULL;
	unsigned int	i;

	for (i = 0; i < *num_args; i++) {
		if (!strcmp(args[i].name,XtNrootX)) {
			x = (Position *)args[i].value;
			fetch = True;
			continue;
		}
		if (!strcmp(args[i].name,XtNrootY)) {
			y = (Position *)args[i].value;
			fetch = True;
			continue;
		}

		if (!strcmp(args[i].name, XtNdsdmPresent)) {
			Boolean *ret = (Boolean *)args[i].value;

			if (cur_part->class_extension->fetch_dsdm_info !=
			    (OlDnDVCXFetchDSDMInfoFunc)NULL)
				(*cur_part->class_extension->fetch_dsdm_info)
					(current, cur_part, 
					 XtLastTimestampProcessed(
						XtDisplay(current)));

			*ret = cur_part->dsdm_present;
		}
	}

	if (fetch) {
		DnDFetchRootCoords(current, cur_part);
		if (x) *x = cur_part->root_x;
		if (y) *y = cur_part->root_y;
	}
}

/******************************************************************************
 *
 * DnDVCXInitialize
 *
 ******************************************************************************/

static	void
DnDVCXInitialize OLARGLIST(( request, new, args, num_args, req_part, new_part))
	OLARG( Widget,			request)
	OLARG( Widget,			new)
	OLARG( ArgList,			args)
	OLARG( Cardinal *,		num_args)
	OLARG(OlDnDVendorPartExtension,	req_part)
	OLGRA(OlDnDVendorPartExtension,	new_part)
{
	new_part->drop_site_list = (OlDnDDropSitePtr)NULL;

	new_part->dsdm_rects = (InternalDSDMSRPtr*)NULL;
	new_part->dropsite_rects = (InternalDSRPtr)NULL;

	new_part->current_dsr = (InternalDSRPtr)NULL;
	new_part->current_dsp = (OlDnDDropSitePtr)NULL;
	new_part->current_dsdmsr = (InternalDSDMSRPtr)NULL;

	new_part->dsdm_sr_list = (DSDMSiteRectPtr)NULL;
	new_part->internal_dsdm_sr_list = (InternalDSDMSRPtr)NULL;
	new_part->num_dsdmsrs = 0;

	new_part->transient_atoms = (TransientAtomListPtr)NULL;
	new_part->selection_atoms = (DSSelectionAtomPtr)NULL;
}

/******************************************************************************
 *
 * DnDVCXPostRealizeSetup
 *
 ******************************************************************************/

static void
DnDVCXPostRealizeSetup OLARGLIST(( vendor, dnd_class, dnd_part ))
			OLARG( Widget,				vendor)
			OLARG( OlDnDVendorClassExtension,	dnd_class)
			OLGRA( OlDnDVendorPartExtension,	dnd_part) 
{
	FetchPartExtensionIfNull(vendor, dnd_part);
	FetchClassExtensionIfNull(vendor, dnd_class);

	if (dnd_part == (OlDnDVendorPartExtension)NULL || 
	    dnd_class == (OlDnDVendorClassExtension)NULL) /* null pointer neurosis */
		return;

	XtAddEventHandler(vendor, (PropertyChangeMask | FocusChangeMask),
			  True, DnDVCXEventHandler,
			  (XtPointer) dnd_part);

	if (dnd_part->dirty && dnd_part->auto_assert_dropsite_registry &&
	    dnd_class->assert_drop_site_registry != (OlDnDVCXAssertRegistryProc)NULL) {
		(*dnd_class->assert_drop_site_registry)(vendor, dnd_part);
	}
}

/******************************************************************************
 *
 * DnDVCXDestroy
 *
 ******************************************************************************/

static void
DnDVCXDestroy OLARGLIST(( vendor, dnd_part ))
		OLARG(Widget,			vendor)
		OLGRA(OlDnDVendorPartExtension,	dnd_part)
{
	OlDnDDropSitePtr dsp; 

	FetchPartExtensionIfNull(vendor, dnd_part);

	XtRemoveEventHandler(vendor, (PropertyChangeMask | FocusChangeMask),
			     True, DnDVCXEventHandler, (XtPointer)dnd_part);

	dsp = dnd_part->drop_site_list;
	dnd_part->drop_site_list = (OlDnDDropSitePtr)NULL;
	dnd_part->number_of_sites = 0;

	for (; dsp != (OlDnDDropSitePtr)NULL;
	       dsp = OlDnDDropSitePtrNextSite(dsp)) {
		DeleteSiteRectsInDropSiteList(dnd_part,
					OlDnDDropSitePtrTopLevelRects(dsp),
					OlDnDDropSitePtrNumRects(dsp));
		XtFree((char *)OlDnDDropSitePtrTopLevelRects(dsp));
		XtFree((char *)dsp);
	}

	XtFree((char *)dnd_part->internal_dsdm_sr_list);
	XtFree((char *)dnd_part->dsdm_sr_list);
	if (dnd_part->transient_atoms)
		XtFree((char *)dnd_part->transient_atoms);

	DeleteDSDMSRList(dnd_part);
}

/****************************************************************************
 *
 * DnDVCXTriggerMessageDispatcher
 *
 ****************************************************************************/



static void
_DnDVCXTriggerACKCB OLARGLIST((vendor, client_data, selection,
			       type, value, length, format ))
	OLARG(Widget,			vendor)
	OLARG(XtPointer,		client_data)
	OLARG(Atom *,			selection)
	OLARG(Atom *,			type)
	OLARG(XtPointer,		value)
	OLARG(unsigned long *,		length)
	OLGRA(int *,			format)
{
	OlDnDDropSitePtr		dsp = (OlDnDDropSitePtr)client_data;
	OlDnDVendorPartExtension	dnd_part;

	dnd_part = _OlGetDnDVendorPartExtension(vendor);
	if ((OlDnDDropSitePtrGotAck(dsp) = (*type == XT_CONVERT_FAIL))) {
		OlDnDDropSitePtrState(dsp) = DropSiteError;
	} else  OlDnDDropSitePtrState(dsp) = DropSiteTx;
}

static Boolean
DnDVCXTriggerMessageDispatcher OLARGLIST(( vendor, dnd_part, trigger_message ))
		OLARG( Widget,				vendor)
		OLARG( OlDnDVendorPartExtension,	dnd_part)
		OLGRA( TriggerMessagePtr,		trigger_message)
{
	Position		topx, topy;
	InternalDSRPtr		idsr;
	OlDnDDropSitePtr	dsp;
	Boolean			ret_val;
	OlDnDTriggerOperation	operation;
	unsigned int		tryagain = 2; /* twice, 2nd time update root
					       * (x,y) first ...
					       */

	if (!vendor->core.visible) return (False);

	FetchPartExtensionIfNull(vendor, dnd_part);

#if	!defined(USE_GEOMETRY_LOOKUP_ONLY)
	dsp = (OlDnDDropSitePtr)TriggerMessagePtrSiteID(trigger_message);
	{
		OlDnDDropSitePtr	p;


		for (p = dnd_part->drop_site_list;
		     p != (OlDnDDropSitePtr)NULL && p != dsp;
		     p = OlDnDDropSitePtrNextSite(p))
		;

#if	defined(USE_SITE_ID_LOOKUP_ONLY)
		if (p == (OlDnDDropSitePtr)NULL)
			return (False);
#else
		if (p == dsp && dsp != (OlDnDDropSitePtr)NULL)
			goto	gotsite;
#endif
	}
#endif
#if	defined(USE_GEOMETRY_LOOKUP_ONLY) || !defined(USE_SITE_ID_LOOKUP_ONLY)
	while (tryagain--) {
		topx = TriggerMessagePtrX(trigger_message) - dnd_part->root_x;
		topy = TriggerMessagePtrY(trigger_message) - dnd_part->root_y;

		if ((idsr = LookupXYInDropSiteList(dnd_part, topx, topy)) != 
		    (InternalDSRPtr)NULL) {
			break;
		}
		
		if (tryagain) DnDFetchRootCoords(vendor, dnd_part);
		/* try again with new root (x,y) */
	}

	if (idsr == (InternalDSRPtr)NULL) {
		if (dnd_part->default_drop_site == (OlDnDDropSitePtr)NULL ||
		    !dnd_part->dsdm_present)
			return (False);
		else {	/* handle forwarding to default sites */
			InternalDSDMSRPtr	idsdmsr = 
						       (InternalDSDMSRPtr)NULL;

			idsdmsr = LookupXYInDSDMSRList(dnd_part,
				       XtScreen(vendor),
				       TriggerMessagePtrX(trigger_message),
				       TriggerMessagePtrY(trigger_message));


			if (idsdmsr == (InternalDSDMSRPtr)NULL ||
			    !InternalDSDMSRPtrRectIsDefaultSite(idsdmsr))
				return (False);

			/* we have a match - its a default site .... */

			if (XtWindow(vendor) ==
			    InternalDSDMSRPtrRectWindowID(idsdmsr))
				dsp = dnd_part->default_drop_site;
			else
				return (False); /* how did we get here */
		}
	} else
		dsp = (OlDnDDropSitePtr)InternalDSRPtrDropSite(idsr);
#endif

#ifndef	USE_GEOMETRY_LOOKUP_ONLY
gotsite:;
#endif

	dnd_part->current_dsr = (InternalDSRPtr)NULL; /* zero cache */
	dnd_part->current_dsp = (OlDnDDropSitePtr)NULL;


	if (OlDnDDropSitePtrTriggerNotify(dsp) ==
	    (OlDnDTriggerMessageNotifyProc)NULL ||
	    !OlDnDDropSitePtrIsSensitive(dsp)) {
		return (False);
	}

	if (TriggerMessagePtrFlags(trigger_message) & OlDnDTriggerAck) {
		XtGetSelectionValue(vendor,
				 TriggerMessagePtrSelection(trigger_message),
				 _SUN_DRAGDROP_ACK, _DnDVCXTriggerACKCB,
				 (XtPointer)dsp,
				 TriggerMessagePtrTimestamp(trigger_message));
		OlDnDDropSitePtrState(dsp) = DropSiteAck;
	} else OlDnDDropSitePtrState(dsp) = DropSiteTrigger;

	OlDnDDropSitePtrIncomingTransient(dsp) =
		(TriggerMessagePtrFlags(trigger_message) & 
#if defined(att) || defined(USL)
		 OlDnDTriggerTransient) ? True : False;
#else
		 OlDnDTriggerTransient);
#endif

	/* call the notifier */

	OlDnDDropSitePtrTimestamp(dsp) =
		TriggerMessagePtrTimestamp(trigger_message);

	operation = (TriggerMessagePtrFlags(trigger_message) &
		     OlDnDTriggerMove) ? OlDnDTriggerMoveOp
#if defined(att) || defined(USL)
			: (TriggerMessagePtrFlags(trigger_message) &
			  OlDnDTriggerLink) ? OlDnDTriggerLinkOp
#endif
				       : OlDnDTriggerCopyOp;

	ret_val = (*OlDnDDropSitePtrTriggerNotify(dsp))
			(OlDnDDropSitePtrOwner(dsp),
			 OlDnDDropSitePtrWindow(dsp),
			 TriggerMessagePtrX(trigger_message),
			 TriggerMessagePtrY(trigger_message),
			 TriggerMessagePtrSelection(trigger_message),
			 TriggerMessagePtrTimestamp(trigger_message),
			 (OlDnDDropSiteID)dsp,
			 operation,
			 OlDnDDropSitePtrIncomingTransient(dsp),
/* SAMC, always pass False,->forwarded*/ False,
			 OlDnDDropSitePtrClosure(dsp));

	return (ret_val);
}

/******************************************************************************
 *
 * DnDVCXPreviewMessageDispatcher
 *
 ******************************************************************************/

static Boolean
DnDVCXPreviewMessageDispatcher OLARGLIST(( vendor, dnd_part, preview_message ))
		OLARG( Widget,				vendor)
		OLARG( OlDnDVendorPartExtension,	dnd_part)
		OLGRA( PreviewMessagePtr,		preview_message)
{
	Position		topx, topy;
	InternalDSRPtr		idsr = (InternalDSRPtr)NULL, prev_cache;
	InternalDSDMSRPtr	idsdmsr = (InternalDSDMSRPtr)NULL;
	OlDnDDropSitePtr	dsp, prev_dsp;
	unsigned int		tryagain = 2; /* twice, 2nd time update root
					       * (x,y) first ...
					       */

	FetchPartExtensionIfNull(vendor, dnd_part);

	if (((prev_cache = dnd_part->current_dsr) != (InternalDSRPtr)NULL ||
	    ((prev_dsp = dnd_part->current_dsp) != (OlDnDDropSitePtr)NULL)) &&
	    PreviewMessagePtrEventcode(preview_message) == LeaveNotify) {
		
		if (prev_cache)
			dsp = (OlDnDDropSitePtr)
				InternalDSRPtrDropSite(prev_cache);
		else
			dsp = prev_dsp;

		if (OlDnDDropSitePtrWantsPreviewEnterLeave(dsp))
			(*OlDnDDropSitePtrPreviewNotify(dsp))
					(OlDnDDropSitePtrOwner(dsp),
					 OlDnDDropSitePtrWindow(dsp),
					 PreviewMessagePtrX(preview_message),
					 PreviewMessagePtrY(preview_message),
					 PreviewMessagePtrEventcode(
							preview_message),
					 PreviewMessagePtrTimestamp(
							preview_message),
					 (OlDnDDropSiteID)dsp,
/* SAMC, always pass False,->forwarded*/ False,
					 OlDnDDropSitePtrClosure(dsp));

		dnd_part->current_dsr = (InternalDSRPtr)NULL;
		dnd_part->current_dsp = (OlDnDDropSitePtr)NULL;

		return (True);
	}

#if	!defined(USE_GEOMETRY_LOOKUP_ONLY)
	dsp = (OlDnDDropSitePtr)PreviewMessagePtrSiteID(preview_message);
	{
		OlDnDDropSitePtr	p;

		for (p = dnd_part->drop_site_list;
		     p != (OlDnDDropSitePtr)NULL && p != dsp;
		     p = OlDnDDropSitePtrNextSite(p))
		;

#if	defined(USE_SITE_ID_LOOKUP_ONLY)
		if (p == (OlDnDDropSitePtr)NULL)
			return (False);
#else
		if (p == dsp && dsp != (OlDnDDropSitePtr)NULL)
			goto	gotsite;
#endif
	}
#endif
#if	defined(USE_GEOMETRY_LOOKUP_ONLY) || !defined(USE_SITE_ID_LOOKUP_ONLY)
	while (tryagain--) { 
		topx = PreviewMessagePtrX(preview_message) - dnd_part->root_x;
		topy = PreviewMessagePtrY(preview_message) - dnd_part->root_y;

		if ((idsr = LookupXYInDropSiteList(dnd_part, topx, topy)) != 
		    (InternalDSRPtr)NULL) {
			break;
		}
		
		if (tryagain) DnDFetchRootCoords(vendor, dnd_part);
		/* try again with new root (x,y) */
	}

	if (idsr == (InternalDSRPtr)NULL) {
		if (dnd_part->default_drop_site == (OlDnDDropSitePtr)NULL ||
		    !dnd_part->dsdm_present)
			return (False);
		else {	/* handle forwarding to default sites */

			idsdmsr = LookupXYInDSDMSRList(dnd_part,
				       XtScreen(vendor),
				       PreviewMessagePtrX(preview_message),
				       PreviewMessagePtrY(preview_message));


			if (idsdmsr == (InternalDSDMSRPtr)NULL ||
			    !InternalDSDMSRPtrRectIsDefaultSite(idsdmsr))
				return (False);

			/* we have a match - its a default site .... */

			if (XtWindow(vendor) ==
			    InternalDSDMSRPtrRectWindowID(idsdmsr))
				dsp = dnd_part->default_drop_site;
			else
				return (False); /* how did we get here ? */
		}
	} else
		dsp = (OlDnDDropSitePtr)InternalDSRPtrDropSite(idsr);
#endif

#ifndef	USE_GEOMETRY_LOOKUP_ONLY
gotsite:;
#endif

	if (OlDnDDropSitePtrPreviewNotify(dsp) ==
	    (OlDnDPreviewMessageNotifyProc)NULL ||
	    !OlDnDDropSitePtrIsSensitive(dsp)) {
		return (False);
	}

	if (!dnd_part->preview_forwarded_sites &&
	    idsr == (InternalDSRPtr)NULL) {
		InternalDSRPtr	rects = OlDnDDropSitePtrTopLevelRects(dsp);
		unsigned int	num   = OlDnDDropSitePtrNumRects(dsp);

		dnd_part->current_dsp = dsp;
		dnd_part->current_dsr = idsr; /* update cache */

		tryagain = 2;

		while (tryagain--) {
			register InternalDSRPtr  r;

			topx = PreviewMessagePtrX(preview_message) -
				dnd_part->root_x;
			topy = PreviewMessagePtrY(preview_message) -
				dnd_part->root_y;

			for (r = rects; r < rects + num; r++) {
				if (XYInInternalDSR(r, topx, topy))
					goto preview;
			}

			if (tryagain) DnDFetchRootCoords(vendor, dnd_part);
		} 

		return (False);
	}

preview:;
	if (PreviewMessagePtrEventcode(preview_message) == EnterNotify) {
		dnd_part->current_dsr = idsr; /* update cache */
		dnd_part->current_dsp = dsp;
	}

	if (PreviewMessagePtrEventcode(preview_message) == LeaveNotify) 
		OlDnDDropSitePtrState(dsp) = DropSiteState0;

	OlDnDDropSitePtrState(dsp) = DropSitePreviewing;
	OlDnDDropSitePtrTimestamp(dsp) =
		PreviewMessagePtrTimestamp(preview_message);

	/* call the notifier */

	if ((OlDnDDropSitePtrWantsPreviewMotion(dsp) &&
	     PreviewMessagePtrEventcode(preview_message) == MotionNotify) ||
	    (OlDnDDropSitePtrWantsPreviewEnterLeave(dsp) &&
	     (PreviewMessagePtrEventcode(preview_message) == EnterNotify)))
		(*OlDnDDropSitePtrPreviewNotify(dsp))
				(OlDnDDropSitePtrOwner(dsp),
				 OlDnDDropSitePtrWindow(dsp),
				 PreviewMessagePtrX(preview_message),
				 PreviewMessagePtrY(preview_message),
				 PreviewMessagePtrEventcode(preview_message),
				 PreviewMessagePtrTimestamp(preview_message),
				 (OlDnDDropSiteID)dsp,
/* SAMC, always pass False,->forwarded*/ False,
				 OlDnDDropSitePtrClosure(dsp));

	return (True);
}

/******************************************************************************
 *
 * DnDVCXRegisterDropSite
 *
 * register a drop site.
 *
 ******************************************************************************/

static void _calldsdestroy(w, dsp)
		Widget			w;
		OlDnDDropSitePtr	dsp;
{
	Widget			 vendor  = GetShellOfWidget(w);
	OlDnDVendorPartExtension dnd_part= _OlGetDnDVendorPartExtension(vendor);


	if (dnd_part == (OlDnDVendorPartExtension)NULL)
		return;

	if (dnd_part->class_extension->delete_drop_site !=
	    (OlDnDVCXDeleteDSProc)NULL)
		(*dnd_part->class_extension->delete_drop_site)
			(vendor, dnd_part, (OlDnDDropSiteID)dsp);
}

static void
unregisterondestroycb(w, clientd, calld)
	Widget		w;
	XtPointer	clientd,
			calld;
{
	OlDnDDropSitePtr 	 dsp = (OlDnDDropSitePtr)(clientd);

	_calldsdestroy(w, dsp);
}

static void
_windowevh(w, clientd, event, continue_to_dispatch)
	Widget		w;
	XtPointer	clientd;
	XEvent		*event;
	Boolean		*continue_to_dispatch;
{
	OlDnDDropSitePtr 	 dsp = (OlDnDDropSitePtr)(clientd);

	if (OlDnDDropSitePtrWindow(dsp) != event->xany.window)
		return;

	switch (event->xany.type) {
		case	DestroyNotify:
			_XtUnregisterWindow(event->xany.window, w);
			_calldsdestroy(w, dsp);
	}
}

static OlDnDDropSiteID
DnDVCXRegisterDropSite OLARGLIST(( vendor, dnd_part, widget, window, 
				   preview_hints, site_rects, num_sites,
				   tmnotify, pmnotify, on_interest, closure ))
	OLARG(Widget,				vendor )
	OLARG(OlDnDVendorPartExtension,		dnd_part )
	OLARG( Widget,				widget )
	OLARG( Window,				window )
	OLARG( OlDnDSitePreviewHints,		preview_hints )
	OLARG( OlDnDSiteRectPtr,		site_rects )
	OLARG( unsigned int,			num_sites)
	OLARG( OlDnDTriggerMessageNotifyProc,	tmnotify )
	OLARG( OlDnDPreviewMessageNotifyProc,	pmnotify )
	OLARG( Boolean,				on_interest )
	OLGRA( XtPointer,			closure )
{
	OlDnDDropSitePtr		dsp;
	register InternalDSRPtr		isr;
	register OlDnDSiteRectPtr	sr = site_rects;

	FetchPartExtensionIfNull(vendor, dnd_part);
	
	if (!XtIsRealized(widget)) {
#if 0
		OlWarning("DnDVCXRegisterDropSite: Widget not Realized");
#else
		XtWarning("DnDVCXRegisterDropSite: Widget not Realized");
#endif
		return ((OlDnDDropSiteID)NULL);
	}

	dsp = (OlDnDDropSitePtr)XtCalloc(1, sizeof(OlDnDDropSite));

	isr = (InternalDSRPtr) XtCalloc(num_sites, sizeof(InternalDSR));

	OlDnDDropSitePtrTopLevelRects(dsp) = isr;

	for (; sr < site_rects + num_sites; isr++, sr++) {
		InternalDSRPtrNext(isr) = (InternalDSRPtr)NULL;
		InternalDSRPtrPrev(isr) = (InternalDSRPtr)NULL;
		InternalDSRPtrDropSite(isr) = (OlDnDDropSiteID)dsp;
		InternalDSRPtrLocalX(isr) = SiteRectPtrX(sr);
		InternalDSRPtrLocalY(isr) = SiteRectPtrY(sr);

		MapLocalCoordsToTopLevel(vendor, dnd_part, widget, window,
				 sr, &InternalDSRPtrTopLevelRect(isr));
	}

	InsertSiteRectsInDropSiteList(dnd_part,
				      OlDnDDropSitePtrTopLevelRects(dsp),
				      num_sites); 

	preview_hints &= (OlDnDSitePreviewNone	      | /* redundant */
			  OlDnDSitePreviewEnterLeave  |
			  OlDnDSitePreviewMotion      |
			  OlDnDSitePreviewBoth	      | /* redundant */
			  OlDnDSitePreviewDefaultSite |
			  OlDnDSitePreviewInsensitive);


	if (preview_hints & OlDnDSitePreviewDefaultSite) {
		dnd_part->default_drop_site = dsp;
	}

	OlDnDDropSitePtrOwner(dsp) 	       = widget;
	OlDnDDropSitePtrWindow(dsp)	       = window;
	OlDnDDropSitePtrEventWindow(dsp)       = XtWindow(vendor);
	OlDnDDropSitePtrPreviewHints(dsp)      = preview_hints;
	OlDnDDropSitePtrNumRects(dsp)	       = num_sites;
	OlDnDDropSitePtrTimestamp(dsp)         = XtLastTimestampProcessed(
							XtDisplay(vendor));
	OlDnDDropSitePtrState(dsp)             = DropSiteState0;
	OlDnDDropSitePtrTriggerNotify(dsp)     = tmnotify;
	OlDnDDropSitePtrPreviewNotify(dsp)     = pmnotify;
	OlDnDDropSitePtrClosure(dsp) 	       = closure;
	OlDnDDropSitePtrOnInterest(dsp)        = on_interest;
	OlDnDDropSitePtrGotAck(dsp)            = False;
	OlDnDDropSitePtrIncomingTransient(dsp) = False;

	dnd_part->number_of_sites++;
	OlDnDDropSitePtrNextSite(dsp) = dnd_part->drop_site_list;
	dnd_part->drop_site_list = dsp;

	if (widget != vendor) {
		XtAddCallback(widget, XtNdestroyCallback, unregisterondestroycb,
			      (XtPointer)dsp);
	} else if (window != XtWindowOfObject(widget)) {
		_XtRegisterWindow(window, widget);
		XtInsertEventHandler(widget, DestroyNotify, True, _windowevh,
				     dsp, XtListHead);
	}

	if (on_interest) {
		dnd_part->dirty = True;

		if (dnd_part->auto_assert_dropsite_registry &&
		    dnd_part->class_extension->assert_drop_site_registry !=
		    (OlDnDVCXAssertRegistryProc)NULL) {
			(*dnd_part->class_extension->assert_drop_site_registry)
				(vendor, dnd_part);

#if !defined(att) && !defined(USL)
			if (dnd_part->class_extension->fetch_dsdm_info !=
			    (OlDnDVCXFetchDSDMInfoFunc)NULL) {
				(*dnd_part->class_extension->fetch_dsdm_info)
						(vendor, dnd_part, 
						 XtLastTimestampProcessed(
						 XtDisplay(vendor)));
			}
#endif
		}
	}
	return ((OlDnDDropSiteID)dsp);
}

/******************************************************************************
 *
 * DnDVCXUpdateDropSiteGeometry
 *
 * update the geometry of a drop site.
 *
 ******************************************************************************/

static void
InvalidateCurrentDSR OLARGLIST(( dnd_part, isr, num_isrs ))
	OLARG( OlDnDVendorPartExtension,	dnd_part)
	OLARG( InternalDSRPtr,			isr)
	OLGRA( unsigned int,			num_isrs)
{
	register InternalDSRPtr	srp = isr;

	if (dnd_part->current_dsr != (InternalDSRPtr)NULL) {
		for (; srp != isr + num_isrs; srp = InternalDSRPtrNext(srp))
			if (srp == dnd_part->current_dsr) {
				dnd_part->current_dsr = (InternalDSRPtr)NULL;
				break;
			}
	}
}

static Boolean
DnDVCXUpdateDropSiteGeometry OLARGLIST((vendor, dnd_part, drop_site,
					site_rects, num_rects ))
	OLARG( Widget,				vendor)
	OLARG( OlDnDVendorPartExtension,	dnd_part)
	OLARG( OlDnDDropSiteID,			drop_site)
	OLARG( OlDnDSiteRectPtr,		site_rects)
	OLGRA( unsigned int,			num_rects)
{
	OlDnDDropSitePtr	dsp;
	Boolean			ret_val = False;

	FetchPartExtensionIfNull(vendor, dnd_part);

        for (dsp = dnd_part->drop_site_list;
             dsp != (OlDnDDropSitePtr)NULL &&
             dsp != (OlDnDDropSitePtr)drop_site;
                dsp = OlDnDDropSitePtrNextSite(dsp))
	;

	if (dsp != (OlDnDDropSitePtr)NULL) {
		InternalDSRPtr		isr;
		OlDnDSiteRectPtr	sr;

		if (!XtIsRealized(OlDnDDropSitePtrOwner(dsp))) {
#if 0
			OlWarning(
			 "DnDVCXUpdateDropSiteGeometry: Widget not Realized");
#else
			XtWarning(
			 "DnDVCXUpdateDropSiteGeometry: Widget not Realized");
#endif
			return (ret_val);
		}

		InvalidateCurrentDSR(dnd_part, 
				     OlDnDDropSitePtrTopLevelRects(dsp),
				     OlDnDDropSitePtrNumRects(dsp)); 

		DeleteSiteRectsInDropSiteList(dnd_part,
				      OlDnDDropSitePtrTopLevelRects(dsp),
				      OlDnDDropSitePtrNumRects(dsp)); 

		XtFree((char *)OlDnDDropSitePtrTopLevelRects(dsp));

		OlDnDDropSitePtrNumRects(dsp) = num_rects;

		isr = (InternalDSRPtr) XtCalloc(num_rects, sizeof(InternalDSR));
		OlDnDDropSitePtrTopLevelRects(dsp) = isr;

		for (sr = site_rects; sr < site_rects+num_rects; isr++, sr++) {
			InternalDSRPtrNext(isr) = (InternalDSRPtr)NULL;
			InternalDSRPtrPrev(isr) = (InternalDSRPtr)NULL;
			InternalDSRPtrDropSite(isr) = (OlDnDDropSiteID)dsp;
			InternalDSRPtrLocalX(isr) = SiteRectPtrX(sr);
			InternalDSRPtrLocalY(isr) = SiteRectPtrY(sr);

			MapLocalCoordsToTopLevel(vendor, dnd_part,
					OlDnDDropSitePtrOwner(dsp),
					OlDnDDropSitePtrWindow(dsp), sr,
					&InternalDSRPtrTopLevelRect(isr));
		}

		InsertSiteRectsInDropSiteList(dnd_part,
				      OlDnDDropSitePtrTopLevelRects(dsp),
				      OlDnDDropSitePtrNumRects(dsp)); 

		if (!OlDnDDropSitePtrOnInterest(dsp))
			return (True);

		dnd_part->dirty = True;

		if (dnd_part->auto_assert_dropsite_registry &&
		    dnd_part->class_extension->assert_drop_site_registry !=
		    (OlDnDVCXAssertRegistryProc)NULL) {
			(*dnd_part->class_extension->assert_drop_site_registry)
				(vendor, dnd_part);

#if !defined(att) && !defined(USL)
			if (dnd_part->class_extension->fetch_dsdm_info !=
			    (OlDnDVCXFetchDSDMInfoFunc)NULL) {
				(*dnd_part->class_extension->fetch_dsdm_info)
						(vendor, dnd_part, 
						 XtLastTimestampProcessed(
							XtDisplay(vendor)));
			}
#endif
		}
		ret_val = True;
	}

	return (ret_val);
}

/******************************************************************************
 *
 * DnDVCXDeleteDropSite
 *
 * delete a drop site.
 *
 ******************************************************************************/

static void
DnDVCXDeleteDropSite OLARGLIST(( vendor, dnd_part, drop_site ))
	OLARG( Widget,				vendor)
	OLARG( OlDnDVendorPartExtension,	dnd_part)
	OLGRA( OlDnDDropSiteID,			drop_site)
{
	OlDnDDropSitePtr	dsp, *dspp;
	unsigned int		i;

	FetchPartExtensionIfNull(vendor, dnd_part);

	for (dspp = &dnd_part->drop_site_list;
	     *dspp != (OlDnDDropSitePtr)NULL &&
	     *dspp != (OlDnDDropSitePtr)drop_site;
		dspp = &OlDnDDropSitePtrNextSite(*dspp))
	;

	if (*dspp == (OlDnDDropSitePtr)NULL)
		return;	/* nae such luck */

	dsp = *dspp;	/* remove it from bondage */
	*dspp = OlDnDDropSitePtrNextSite(dsp);
	OlDnDDropSitePtrNextSite(dsp) = (OlDnDDropSitePtr)NULL;

	dnd_part->dirty = True;

	InvalidateCurrentDSR(dnd_part, 
			     OlDnDDropSitePtrTopLevelRects(dsp),
			     OlDnDDropSitePtrNumRects(dsp)); 

	if (dnd_part->current_dsp == dsp)
		dnd_part->current_dsp = (OlDnDDropSitePtr)NULL;

	DeleteSiteRectsInDropSiteList(dnd_part, 
			      OlDnDDropSitePtrTopLevelRects(dsp),
			      OlDnDDropSitePtrNumRects(dsp)); 

	XtRemoveCallback(OlDnDDropSitePtrOwner(dsp), XtNdestroyCallback, 
			 unregisterondestroycb, (XtPointer)dsp);


	if (dnd_part->class_extension->disassociate_selection_and_w !=
	    (OlDnDVCXDisassocSelectionProc)NULL)
		(*dnd_part->class_extension->disassociate_selection_and_w)
			(vendor, dnd_part, OlDnDDropSitePtrOwner(dsp),
			 FreeAllSelectionAtoms, 
			 XtLastTimestampProcessed(XtDisplay(vendor)));

        if (dnd_part->class_extension->free_transient_atom !=
            (OlDnDVCXFreeTransientAtomProc)NULL)
                (*dnd_part->class_extension->free_transient_atom)
                        (vendor, dnd_part, OlDnDDropSitePtrOwner(dsp),
			 FreeAllTransientAtoms);

	XtFree((char *)OlDnDDropSitePtrTopLevelRects(dsp));
	XtFree((char *)dsp);

	if (dsp == dnd_part->default_drop_site) {
		dnd_part->default_drop_site = (OlDnDDropSitePtr)NULL;
	}

	if (--dnd_part->number_of_sites == 0) {
		if (dnd_part->class_extension->delete_drop_site_registry !=
		    (OlDnDVCXDeleteRegistryProc)NULL)
			(*dnd_part->class_extension->delete_drop_site_registry)
				(vendor, dnd_part);
		dnd_part->number_of_sites = 0;
	} else if (dnd_part->auto_assert_dropsite_registry &&
	    	   dnd_part->class_extension->assert_drop_site_registry !=
	    	   (OlDnDVCXAssertRegistryProc)NULL) {
			(*dnd_part->class_extension->assert_drop_site_registry)
							     (vendor, dnd_part);
	}
}

/******************************************************************************
 *
 * DnDVCXQueryDropSiteInfo
 *
 * get the info for a registered drop site.
 *
 ******************************************************************************/

static	Boolean
DnDVCXQueryDropSiteInfo OLARGLIST(( vendor, dnd_part, drop_site, widget,
				    window, preview_hints, site_rects,
				    num_rects,  on_interest))
	OLARG( Widget, 				vendor)
	OLARG( OlDnDVendorPartExtension,	dnd_part)
	OLARG( OlDnDDropSiteID,			drop_site)
	OLARG( Widget *,			widget)
	OLARG( Window *,			window)
	OLARG( OlDnDSitePreviewHints *,		preview_hints)
	OLARG( OlDnDSiteRectPtr *,		site_rects)
	OLARG( unsigned int *,			num_rects)
	OLGRA( Boolean *,			on_interest)
{
	OlDnDDropSitePtr	dsp;

	FetchPartExtensionIfNull(vendor, dnd_part);

        for (dsp = dnd_part->drop_site_list;
             dsp != (OlDnDDropSitePtr)NULL &&
             dsp != (OlDnDDropSitePtr)drop_site;
                dsp = OlDnDDropSitePtrNextSite(dsp))
	;

	if (dsp != (OlDnDDropSitePtr)NULL) {
		if (widget != (Widget *)NULL)
			*widget = OlDnDDropSitePtrOwner(dsp);

		if (window != (Window *)NULL) 
			*window = OlDnDDropSitePtrWindow(dsp);

		if (preview_hints != (OlDnDSitePreviewHints *)NULL)
			*preview_hints = OlDnDDropSitePtrPreviewHints(dsp);

		if (num_rects != (unsigned int *)NULL)
			*num_rects = OlDnDDropSitePtrNumRects(dsp);

#if defined(att) || defined(USL)
		if (on_interest != (Boolean *)NULL)
			*on_interest = OlDnDDropSitePtrOnInterest(dsp);
#else
		if (on_interest != (Boolean *)NULL)
			*num_rects = OlDnDDropSitePtrOnInterest(dsp);
#endif

		if (num_rects != (unsigned int *)NULL &&
		    site_rects != (OlDnDSiteRectPtr *)NULL) {
			unsigned int 		num = *num_rects;
			OlDnDSiteRectPtr	site;
			InternalDSRPtr		isr;

			*site_rects = site = (OlDnDSiteRectPtr)
				XtCalloc(num, sizeof(OlDnDSiteRect));

			for (isr = OlDnDDropSitePtrTopLevelRects(dsp);
			     num--;
			     site++, isr++)
				*site = InternalDSRPtrTopLevelRect(isr);

		} else {
				if (num_rects)
					*num_rects = 0;
				if (site_rects)
					*site_rects = (OlDnDSiteRectPtr)NULL;

#if defined(att) || defined(USL)
					/* return False, only when one
					 * of them is not NULL. */
				if (num_rects || site_rects)
					return (False);
#else
				return (False);
#endif
		}
		return (True);
	}
	return (False);
}

/******************************************************************************
 *
 * DnDVCXAssertDropSiteRegistry
 *
 * assert the registry of our drop sites ....
 *
 * include format of property later .....
 *
 ******************************************************************************/

static void
DnDVCXAssertDropSiteRegistry OLARGLIST((vendor, dnd_part))
	OLARG( Widget,				vendor)
	OLGRA( OlDnDVendorPartExtension,	dnd_part)
{
	FetchPartExtensionIfNull(vendor, dnd_part);

	if (dnd_part->dirty && dnd_part->auto_assert_dropsite_registry) {
		InterestPropertyPtr	interest;
		SiteDescriptionPtr	sites;
		OlDnDDropSitePtr	dsp;
		unsigned int		sizeofinterestprop =
						SizeOfInterestPropertyHead;

		if (!XtIsRealized(vendor))
			return;

		for (dsp = dnd_part->drop_site_list;
		     dsp != (OlDnDDropSitePtr)NULL;
		     dsp = OlDnDDropSitePtrNextSite(dsp)) {
			if (OlDnDDropSitePtrOnInterest(dsp)) {
				sizeofinterestprop +=
#if defined(att) || defined(USL)
				(SizeOfSiteDescriptionForNRects(
					OlDnDDropSitePtrNumRects(dsp)) -
				 SizeOfInterestPropertyHead);
#else
				SizeOfSiteDescriptionForNRects(
					OlDnDDropSitePtrNumRects(dsp));
#endif
			}
		}
	
#if defined(att) || defined(USL)
		interest = (InterestPropertyPtr)XtCalloc(
				1, sizeofinterestprop * sizeof(long));
#else
		interest = (InterestPropertyPtr)XtCalloc(1, sizeofinterestprop);
#endif
		InterestPropertyPtrVersionNumber(interest) = 0;
		InterestPropertyPtrSiteCount(interest) =
						dnd_part->number_of_sites;
		
		for ((dsp = dnd_part->drop_site_list), 
		     (sites = InterestPropertyPtrSiteDescriptions(interest));
		     dsp != (OlDnDDropSitePtr)NULL;
		     dsp = OlDnDDropSitePtrNextSite(dsp)) { /* for */
			InternalDSRPtr	 isr;
			SiteRectPtr 	 rects;
			unsigned int	 nr = OlDnDDropSitePtrNumRects(dsp);

			if (!OlDnDDropSitePtrOnInterest(dsp))
				continue;
			
			SiteDescriptionPtrEventWindow(sites) = 
				OlDnDDropSitePtrEventWindow(dsp);
			SiteDescriptionPtrSiteID(sites) = (unsigned int)dsp;
			SiteDescriptionPtrPreviewHints(sites) =
				OlDnDDropSitePtrPreviewHints(dsp);
			SiteDescriptionPtrAreasType(sites) = IsRectList;

			RectListRectCount(SiteDescriptionPtrAreaRectList(sites)) = nr;

			for ((isr = OlDnDDropSitePtrTopLevelRects(dsp)),
			     (rects = RectListRects(
				        SiteDescriptionPtrAreaRectList(sites)));
			     nr--;
			     isr++, rects++) {
				SiteRectPtrX(rects) =
					InternalDSRPtrTopLevelRectX(isr);
				SiteRectPtrY(rects) =
					InternalDSRPtrTopLevelRectY(isr);
				SiteRectPtrWidth(rects) =
					InternalDSRPtrTopLevelRectWidth(isr);
				SiteRectPtrHeight(rects) =
					InternalDSRPtrTopLevelRectHeight(isr);
			}

			sites = (SiteDescriptionPtr)((char *)sites +
					SizeOfSiteDescriptionForNRects(
					OlDnDDropSitePtrNumRects(dsp)));
		}

		XChangeProperty(XtDisplay(vendor), XtWindow(vendor), 
				_SUN_DRAGDROP_INTEREST, _SUN_DRAGDROP_INTEREST,
				32, PropModeReplace, (unsigned char *)interest,
				sizeofinterestprop);

		XtFree((char *)interest);
		dnd_part->dirty = False;
	}
}

/******************************************************************************
 *
 * DnDVCXDeleteDropSiteRegistry
 *
 * blow dat suckah away!
 *
 ******************************************************************************/

static void
DnDVCXDeleteDropSiteRegistry OLARGLIST(( vendor, dnd_part ))
	OLARG( Widget,				vendor)
	OLGRA( OlDnDVendorPartExtension,        dnd_part)
{
	FetchPartExtensionIfNull(vendor, dnd_part);
	
	dnd_part->dirty = False;
	XDeleteProperty(XtDisplay(vendor), XtWindow(vendor), 
			_SUN_DRAGDROP_INTEREST);
}

/****************************************************************************
 *
 * DnDVCXFetchDSDMInfo
 *
 * fetch the _SUN_DRAGDROP_SITE_RECTS target from the _SUN_DRAGDROP_DSDM 
 * selection.
 *
 ****************************************************************************/

static void
_DnDVCXDSDMSiteRectSelectionCB OLARGLIST((vendor, client_data, selection,
					  type, value, length, format ))
	OLARG(Widget,			vendor)
	OLARG(XtPointer,		client_data)
	OLARG(Atom *,			selection)
	OLARG(Atom *,			type)
	OLARG(XtPointer,		value)
	OLARG(unsigned long *,		length)
	OLGRA(int *,			format)
{
	unsigned int			i;
	OlDnDVendorPartExtension	dnd_part;

	dnd_part = (OlDnDVendorPartExtension)client_data;

	if (!dnd_part->pending_dsdm_info) {
		return;
	}

	/* did we get the info ???? */

	if ((*selection != _SUN_DRAGDROP_DSDM &&
	    *type != XA_INTEGER) ||
	    *length == 0 || value == (XtPointer)NULL) {
		dnd_part->pending_dsdm_info = False;
		dnd_part->dsdm_present = False;
		return;	/* oops */
	}

	if (dnd_part->dsdm_rects != (InternalDSDMSRPtr *)NULL) {
		DeleteDSDMSRList(dnd_part);
		XtFree((char *)dnd_part->internal_dsdm_sr_list);
		XtFree((char *)dnd_part->dsdm_sr_list);
	}

	/* yes .. put it in the cache */

	dnd_part->dsdm_last_loaded =
		XtLastTimestampProcessed(XtDisplay(vendor));

	dnd_part->dsdm_sr_list = (DSDMSiteRectPtr)value;
	dnd_part->num_dsdmsrs = *length / (sizeof (DSDMSiteRect) / sizeof(int));
	dnd_part->internal_dsdm_sr_list =
			(InternalDSDMSRPtr) XtCalloc(dnd_part->num_dsdmsrs,
						     sizeof(InternalDSDMSR));

	for (i = 0; i < dnd_part->num_dsdmsrs; i++) {
		InternalDSDMSRRect(dnd_part->internal_dsdm_sr_list[i]) =
			dnd_part->dsdm_sr_list + i;
	}

	InsertDSDMSRsIntoDSDMSRList(dnd_part, dnd_part->internal_dsdm_sr_list,
				    dnd_part->num_dsdmsrs);

	dnd_part->pending_dsdm_info = False;
	dnd_part->dsdm_present = True;
}

/*
 * request the list of drop sites from the dsdm ....
 */

static Boolean
DnDVCXFetchDSDMInfo OLARGLIST((vendor, dnd_part, timestamp))
        OLARG( Widget,                          vendor)
        OLARG( OlDnDVendorPartExtension,        dnd_part)
	OLGRA( Time,				timestamp)
{
	XtAppContext			app;

#if defined(att) || defined(USL)
	app = XtWidgetToApplicationContext(vendor);
#else
	app = XtDisplayToApplicationContext(XtDisplay(vendor));
#endif

	if (!XtIsRealized(vendor) || dnd_part->pending_dsdm_info)
		return (False);

        FetchPartExtensionIfNull(vendor, dnd_part);

	if (timestamp == CurrentTime &&
	    (timestamp = XtLastTimestampProcessed(XtDisplay(vendor))) == CurrentTime)
		return (False);

	dnd_part->pending_dsdm_info = True;

	XtGetSelectionValue(vendor, _SUN_DRAGDROP_DSDM, 
			    _SUN_DRAGDROP_SITE_RECTS, 
			    _DnDVCXDSDMSiteRectSelectionCB,
			    (XtPointer)dnd_part, timestamp);

	while (dnd_part->pending_dsdm_info) {
		XtAppProcessEvent(app, XtIMAll);
	}

	/*
	 * we have to wait a while .....
	 */

	return (dnd_part->dsdm_sr_list != (DSDMSiteRectPtr)NULL);
}

/******************************************************************************
 *
 * DnDVCXDeliverTriggerMessage
 *
 * deliver a trigger message ....... if the dsdm is present lookup the
 * internal cache of site rects .... otherwise hunt the window heirarchy
 * for another shell with the drop site info .... if the drop site is 
 * local then call the dispatcher otherwise send the event.
 *
 ******************************************************************************/

static Boolean
DnDVCXDeliverTriggerMessage OLARGLIST((vendor, dnd_part, widget, root,
				       rootx, rooty, selection,
				       operation,timestamp))
	OLARG( Widget,				vendor)
	OLARG( OlDnDVendorPartExtension,        dnd_part)
	OLARG( Widget,				widget)
	OLARG( Window,				root)
	OLARG( int,				rootx)
	OLARG( int,				rooty)
	OLARG( Atom,				selection)
	OLARG( OlDnDTriggerOperation,		operation)
	OLGRA( Time,				timestamp)
{
	TriggerMessage			trigger_message;
	XClientMessageEvent		client_message;
	InternalDSDMSRPtr		idsdmsr = (InternalDSDMSRPtr)NULL;
	unsigned int			scrn, noofscreens;
	Boolean				ret = False;
	Window				window_id = (Window)NULL;
	OlDnDDropSiteID			dropsiteid = (OlDnDDropSiteID)NULL;
	OlDnDTriggerFlags		flags = 0;
	SiteDescriptionPtr		site = (SiteDescriptionPtr)NULL;
	Widget				local_vendor = (Widget)NULL;
	OlDnDVendorPartExtension	part;
	Display				*dpy = XtDisplay(widget);


        FetchPartExtensionIfNull(vendor, dnd_part);

	noofscreens = ScreenCount(dpy);

	for (scrn = 0;
	     scrn < noofscreens && root != RootWindow(dpy, scrn);
	     scrn++)
	;

	if (dnd_part->dsdm_present) {
		idsdmsr = LookupXYInDSDMSRList(dnd_part,
					       ScreenOfDisplay(dpy, scrn),
				       	       rootx, rooty);

		if (idsdmsr != (InternalDSDMSRPtr)NULL) {
			window_id = InternalDSDMSRPtrRectWindowID(idsdmsr);
			dropsiteid = (OlDnDDropSiteID)
					InternalDSDMSRPtrRectSiteID(idsdmsr);	
		}
	}

	if (!dnd_part->dsdm_present || idsdmsr == (InternalDSDMSRPtr)NULL)
	     if (FindDropSiteInfoFromDestination(ScreenOfDisplay(dpy, scrn),
				                 root, rootx, rooty, &site)) {
		window_id = SiteDescriptionPtrEventWindow(site);
		dropsiteid = (OlDnDDropSiteID)SiteDescriptionPtrSiteID(site);
		XtFree((char *)site);
	     }
					     
	if (idsdmsr == (InternalDSDMSRPtr)NULL &&
	    site == (SiteDescriptionPtr)NULL) {
		return (False);
	}

	if (dnd_part->current_dsdmsr != (InternalDSDMSRPtr)NULL)
		InternalDSDMSRPtrInSite(dnd_part->current_dsdmsr) = False;

	/* we've got the drop site ..... lets do it! */

	
	TriggerMessageType(trigger_message) = _SUN_DRAGDROP_TRIGGER;

	TriggerMessageWindow(trigger_message) = window_id;

	TriggerMessageSiteID(trigger_message) = (unsigned long)dropsiteid;

	flags = OlDnDTriggerAck; /* well, we might as well! */

	switch (operation) {
		case OlDnDTriggerCopyOp:
			flags |= OlDnDTriggerCopy;
			break;
#if defined(att) || defined(USL)
		case OlDnDTriggerLinkOp:
			flags |= OlDnDTriggerLink;
			break;
#endif
		default:
		case OlDnDTriggerMoveOp:
			flags |= OlDnDTriggerMove;
			break;
	}

	/* check if transient */

	if (_OlDnDAtomIsTransient(vendor, dnd_part, selection))
			flags |= OlDnDTriggerTransient;

	TriggerMessageFlags(trigger_message) = flags;

	TriggerMessageX(trigger_message) = rootx;
	TriggerMessageY(trigger_message) = rooty;

	TriggerMessageSelection(trigger_message) = selection;
	TriggerMessageTimestamp(trigger_message) = timestamp;

	/* try locally first */

	local_vendor = XtWindowToWidget(XtDisplay(vendor), window_id);

	if (local_vendor != (Widget)NULL &&
	    XtIsSubclass(local_vendor, vendorShellWidgetClass) &&
	    (part = _OlGetDnDVendorPartExtension(local_vendor)) !=
					(OlDnDVendorPartExtension)NULL) {
		OlDnDVCXTMDispatcherFunc	tm_dispatcher;

		tm_dispatcher =
			part->class_extension->trigger_message_dispatcher;

		if (tm_dispatcher != (OlDnDVCXTMDispatcherFunc)NULL) {
			ret = (*tm_dispatcher)
				    (local_vendor, part, &trigger_message);
#if defined(att) || defined(USL)
				/* return it right away in this case...	*/
			return(ret);
#endif
		}
	}

	if (!ret) {
		CopyTriggerMessageToClientMessage(&trigger_message,
						  &client_message);

		ret = XSendEvent(dpy, client_message.window, False,
				 NoEventMask, (XEvent *)&client_message);
	}
 
	return (ret);
}


/******************************************************************************
 *
 * DnDVCXDeliverPreviewMessage
 *
 * Send a preview message .... first find the drop site its intended for,
 * deliver any LeaveNotifies that are required (if we have left a drop
 * site we were previously in ..... if the drop site is local call the
 * dispatcher directly .... otherwise send the event ......
 *
 ******************************************************************************/

static Boolean
DnDVCXDeliverPreviewMessage OLARGLIST((vendor, dnd_part, widget, root, 
				       rootx, rooty, timestamp,
				       animate_proc, closure))
	OLARG( Widget,				vendor)
	OLARG( OlDnDVendorPartExtension,        dnd_part)
	OLARG( Widget,				widget)	
	OLARG( Window,				root)
	OLARG( int,				rootx)
	OLARG( int,				rooty)
	OLARG( Time,				timestamp)
	OLARG( OlDnDPreviewAnimateCallbackProc,	animate_proc)
	OLGRA( XtPointer,			closure)
{
	PreviewMessage			preview_message;
	XClientMessageEvent		client_message;
	InternalDSDMSRPtr		idsdmsr, prev_cache;
	unsigned int			scrn, noofscreens;
	Screen				*screen;
	Boolean				ret_val = True;
	Boolean				deliver_preview;
#if defined(att) || defined(USL)
		/* make sure this value is not Enter/Leave/MotionNotify	*/
	unsigned long			eventcode = 0;
#else
	unsigned long			eventcode;
#endif
	Widget				local_vendor = (Widget)NULL;
	OlDnDVendorPartExtension	part;
	Display				*dpy = XtDisplay(widget);
	OlDnDSitePreviewHints		hints;

        FetchPartExtensionIfNull(vendor, dnd_part);

	noofscreens = ScreenCount(dpy);

	for (scrn = 0;
	     scrn < noofscreens && root != RootWindow(dpy, scrn);
	     scrn++)
	;

	screen = ScreenOfDisplay(dpy, scrn);
	prev_cache = dnd_part->current_dsdmsr;

	idsdmsr = LookupXYInDSDMSRList(dnd_part, screen, rootx, rooty);

	/* deliver any leave messages first! */

	PreviewMessageType(preview_message) = _SUN_DRAGDROP_PREVIEW;

	if (prev_cache != (InternalDSDMSRPtr)NULL) { /* we were in a site */
		Boolean	ret = False;

		/* if we are not still in it ... deliver a Leave event if
		 * appropriate
		 */

		if (!XYInInternalDSDMSR(prev_cache, scrn, rootx, rooty) &&
		    (idsdmsr == (InternalDSDMSRPtr)NULL ||
		     InternalDSDMSRPtrRectSiteID(idsdmsr) !=
		     InternalDSDMSRPtrRectSiteID(prev_cache))) {
			InternalDSDMSRPtrInSite(prev_cache) = False;
			deliver_preview =
				InternalDSDMSRPtrRectWantsPreviewEnterLeave(
							    prev_cache);
				eventcode = LeaveNotify;
		} else { /* we are still inside it !!!! */
			deliver_preview = False;

			if (idsdmsr != (InternalDSDMSRPtr)NULL &&
			    InternalDSDMSRPtrRectSiteID(idsdmsr) ==
			    InternalDSDMSRPtrRectSiteID(prev_cache))
				InternalDSDMSRPtrInSite(idsdmsr) = True;
		}
#if defined(att) || defined(USL)
		if (eventcode == LeaveNotify)
		{
			if (animate_proc !=
			    (OlDnDPreviewAnimateCallbackProc)NULL)
			{
				hints = InternalDSDMSRPtrRectPreviewHints(
						prev_cache);
				(*animate_proc)(widget, eventcode, timestamp,
/* SAMC, always pass False, ->sensitivity*/	False,
						closure);
			}
		}
#endif

		if (deliver_preview) {
			hints = InternalDSDMSRPtrRectPreviewHints(prev_cache);

			PreviewMessageWindow(preview_message) = 
			     InternalDSDMSRPtrRectWindowID(prev_cache);	

			PreviewMessageSiteID(preview_message) =
			     InternalDSDMSRPtrRectSiteID(prev_cache);

			PreviewMessageEventcode(preview_message) = eventcode;

			PreviewMessageX(preview_message) = rootx;
			PreviewMessageY(preview_message) = rooty;

			PreviewMessageTimestamp(preview_message) = timestamp;

#define	PCE_PMD(part)	(part)->class_extension->preview_message_dispatcher

			/* try locally */

			local_vendor = XtWindowToWidget(XtDisplay(vendor),
					PreviewMessageWindow(preview_message));

			if (local_vendor != (Widget)NULL &&
			    XtIsSubclass(local_vendor, vendorShellWidgetClass)
			    && (part = _OlGetDnDVendorPartExtension(local_vendor))
			             != (OlDnDVendorPartExtension)NULL) {
				OlDnDVCXPMDispatcherFunc	pm_dispatcher;

				pm_dispatcher = PCE_PMD(part);
				if (pm_dispatcher != (OlDnDVCXPMDispatcherFunc)NULL)
					ret = (*pm_dispatcher) 
							(local_vendor,
							 part,
							 &preview_message);
			}

			if (!ret) { /* remote site */
				CopyPreviewMessageToClientMessage(
					&preview_message, &client_message);

				ret = XSendEvent(dpy, client_message.window,
						 False, NoEventMask,
						 (XEvent *)&client_message);
			} 

#if !defined(att) && !defined(USL)
			if (ret && animate_proc !=
			    (OlDnDPreviewAnimateCallbackProc)NULL) {
				(*animate_proc)(widget, eventcode, timestamp,
/* SAMC, always pass False, ->sensitivity*/	False,
						closure);
			}
#endif
		} else ret = True;

		ret_val = ret;
	}

	dnd_part->current_dsdmsr = idsdmsr; /* update the cache */

	if (idsdmsr == (InternalDSDMSRPtr)NULL) {
		return (False); /* oops no sites */
	}

	if (InternalDSDMSRPtrInSite(idsdmsr)) {
		eventcode =  MotionNotify;
		deliver_preview = 
			InternalDSDMSRPtrRectWantsPreviewMotion(idsdmsr);
	} else {
		eventcode = EnterNotify;
		InternalDSDMSRPtrInSite(idsdmsr) = True;
		deliver_preview =
			InternalDSDMSRPtrRectWantsPreviewEnterLeave(idsdmsr);
	}

#if defined(att) || defined(USL)
	if (eventcode == EnterNotify)
	{
		hints = InternalDSDMSRPtrRectPreviewHints(idsdmsr);
		if (animate_proc !=
		    (OlDnDPreviewAnimateCallbackProc)NULL) {
			(*animate_proc)(widget, eventcode, timestamp,
/* SAMC, always pass False, ->sensitivity*/	False,
					closure);
		}
	}
#endif

	if (deliver_preview) {
		Boolean	ret = False;

		hints = InternalDSDMSRPtrRectPreviewHints(idsdmsr);

		PreviewMessageWindow(preview_message) = 
			InternalDSDMSRPtrRectWindowID(idsdmsr);	

		PreviewMessageSiteID(preview_message) =
			InternalDSDMSRPtrRectSiteID(idsdmsr);

		PreviewMessageEventcode(preview_message) = eventcode;

		PreviewMessageX(preview_message) = rootx;
		PreviewMessageY(preview_message) = rooty;

		PreviewMessageTimestamp(preview_message) = timestamp;

		/* try locally */

		local_vendor = XtWindowToWidget(XtDisplay(vendor),
					PreviewMessageWindow(preview_message));

		if (local_vendor != (Widget)NULL &&
		    XtIsSubclass(local_vendor, vendorShellWidgetClass) &&
		    (part = _OlGetDnDVendorPartExtension(local_vendor)) !=
					(OlDnDVendorPartExtension)NULL) {
			OlDnDVCXPMDispatcherFunc	pm_dispatcher;

			pm_dispatcher = PCE_PMD(part);

			if (pm_dispatcher != (OlDnDVCXPMDispatcherFunc)NULL) {
			    ret = (*pm_dispatcher) (local_vendor,
						    part,
						    &preview_message);
			}
		}

		if (!ret) {
			CopyPreviewMessageToClientMessage(&preview_message,
							  &client_message);

			ret = XSendEvent(dpy, client_message.window,
					      False, NoEventMask,
					      (XEvent *)&client_message);
		}
		ret_val &= ret;

#if !defined(att) && !defined(USL)
		if (ret_val && animate_proc !=
		    (OlDnDPreviewAnimateCallbackProc)NULL) {
			(*animate_proc)(widget, eventcode, timestamp,
/* SAMC, always pass False, ->sensitivity*/	False,
					closure);
		}
#endif
	}

	return (ret_val);
}


/******************************************************************************
 *
 * DnDVCXInitializeDragState
 *
 * enter drag state.
 *
 ******************************************************************************/

static Boolean
DnDVCXInitializeDragState OLARGLIST (( vendor, dnd_part ))
	OLARG( Widget,				vendor)
	OLGRA( OlDnDVendorPartExtension,        dnd_part)
{
	FetchPartExtensionIfNull(vendor, dnd_part);

	if (dnd_part->doing_drag)
		return (True);

	dnd_part->doing_drag = True;

	/* just in case */
	if (dnd_part->current_dsdmsr != (InternalDSDMSRPtr)NULL)
		InternalDSDMSRPtrInSite(dnd_part->current_dsdmsr) = False;

	dnd_part->current_dsdmsr = (InternalDSDMSRPtr)NULL;

	if (dnd_part->class_extension->fetch_dsdm_info !=
	    (OlDnDVCXFetchDSDMInfoFunc)NULL) {
		return (*dnd_part->class_extension->fetch_dsdm_info)
				(vendor, dnd_part, 
				 XtLastTimestampProcessed(XtDisplay(vendor)));
	}

	return (False);
}

/******************************************************************************
 *
 * DnDVCXClearDragState
 *
 * clear down the drag state.
 *
 ******************************************************************************/

static void
DnDVCXClearDragState OLARGLIST (( vendor, dnd_part ))
	OLARG(Widget,				vendor)
	OLGRA( OlDnDVendorPartExtension,        dnd_part)
{
	FetchPartExtensionIfNull(vendor, dnd_part);

	dnd_part->doing_drag = False;

	if (dnd_part->current_dsdmsr != (InternalDSDMSRPtr)NULL)
		InternalDSDMSRPtrInSite(dnd_part->current_dsdmsr) = False;

	dnd_part->current_dsdmsr = (InternalDSDMSRPtr)NULL;
}

/******************************************************************************
 *
 * DnDVCXAllocTransientAtom
 *
 * allocate a "transient" atom for use as a temporary selection atom ????
 *
 ******************************************************************************/

#define	INCRLIST	5

static Atom
DnDVCXAllocTransientAtom OLARGLIST ((vendor, dnd_part, widget ))
	OLARG( Widget,				vendor)
	OLARG( OlDnDVendorPartExtension,	dnd_part)
	OLGRA( Widget,				widget)
{
	TransientAtomListPtr	transients;
	unsigned int		i;
	Atom			atom;

	FetchPartExtensionIfNull(vendor, dnd_part);

	transients = dnd_part->transient_atoms;
	if (transients  == (TransientAtomListPtr)NULL ||
	     NeedsLargerTransientAtomList(transients)) {
		unsigned int	size, prev = 0, used = 0;

		if (transients) {
			prev = TransientAtomListPtrAlloc(transients);
			used = TransientAtomListPtrUsed(transients);
		}

		size = SizeOfTransientAtomListForNAtoms(prev + INCRLIST);
					
		dnd_part->transient_atoms = transients =
			(TransientAtomListPtr)XtRealloc((char*)transients,size);

		TransientAtomListPtrAlloc(transients) = prev + INCRLIST;
		TransientAtomListPtrUsed(transients) = used;

		for (i = prev; i < TransientAtomListPtrAlloc(transients);
		     i++) {	
			TransientAtomListPtrAtom(transients, i) = (Atom)NULL;
			TransientAtomListPtrOwner(transients, i) =
				(Widget)NULL;
		}
	}

	for (i = 0;
	     i < TransientAtomListPtrAlloc(transients) &&
	     TransientAtomListPtrOwner(transients, i) != (Widget)NULL; i++)
	;
	if ((atom = TransientAtomListPtrAtom(transients, i)) == (Atom)NULL) {
		char			str[32];

		sprintf(str,"_SUN_TA_%08x_%03d", XtWindow(vendor), i);
		TransientAtomListPtrAtom(transients, i) = atom =
		XInternAtom(XtDisplay(vendor), str, False);

	}

	TransientAtomListPtrOwner(transients, i) = widget;
	TransientAtomListPtrUsed(transients)++;

	return (atom);
}

/******************************************************************************
 *
 * DnDVCXFreeTransientAtom
 *
 * free a transient atom for re-use.
 *
 ******************************************************************************/

static void
DnDVCXFreeTransientAtom OLARGLIST ((vendor, dnd_part, widget, atom ))
	OLARG( Widget,				vendor)
	OLARG( OlDnDVendorPartExtension,	dnd_part)
	OLARG( Widget,				widget)
	OLGRA( Atom,				atom)
{
	TransientAtomListPtr	transients;
	unsigned int		i;

	if ((transients = dnd_part->transient_atoms) !=
	    (TransientAtomListPtr)NULL) {
		for (i = 0; i < TransientAtomListPtrAlloc(transients); i++)
			if (TransientAtomListPtrOwner(transients, i) == widget
			    && (atom == FreeAllTransientAtoms ||
			    atom == TransientAtomListPtrAtom(transients, i))) {
				TransientAtomListPtrOwner(transients, i) =
					(Widget)NULL;
				TransientAtomListPtrUsed(transients)--;
			}
	}
}

/******************************************************************************
 *
 * DnDVCXAssocSelectionWithWidget
 *
 * bind a selection atom to a widget.
 *
 ******************************************************************************/

static DSSelectionAtomPtr
DnDVCXAssocSelectionWithWidget OLARGLIST (( vendor, dnd_part, 
					    widget, selection, 
					    timestamp, closure ))
		OLARG( Widget,				vendor)
		OLARG( OlDnDVendorPartExtension,	dnd_part)
		OLARG( Widget,				widget)
		OLARG( Atom,				selection)
		OLARG( Time,				timestamp)
		OLGRA( OwnerProcClosurePtr,		closure)
{
	DSSelectionAtomPtr	part_list, p;

	FetchPartExtensionIfNull(vendor, dnd_part);

	part_list = dnd_part->selection_atoms;

	for (p = part_list; p != (DSSelectionAtomPtr)NULL &&
	     selection != DSSelectionAtomPtrSelectionAtom(p);
	     p = DSSelectionAtomPtrNext(p))
	;

	if (p != (DSSelectionAtomPtr)NULL) { /* someone else still has it */
		return p;
	}

	p = (DSSelectionAtomPtr)XtCalloc(1, sizeof(DSSelectionAtom));

	DSSelectionAtomPtrSelectionAtom(p)   = selection;
	DSSelectionAtomPtrOwner(p)   	     = widget;
	DSSelectionAtomPtrTimestamp(p) 	     = timestamp;
	DSSelectionAtomPtrClosure(p)         = closure;
	DSSelectionAtomPtrRequestorWindow(p) = (Window)NULL;
	DSSelectionAtomPtrRequestorDisplay(p) = (Display *)NULL;

	DSSelectionAtomPtrNext(p) = part_list;
	dnd_part->selection_atoms = p;

	return p;
}

/******************************************************************************
 *
 * DnDVCXDissassocSelectionWithWidget
 *
 * free a selection from its associated widget.
 *
 ******************************************************************************/


static void	
DnDVCXDissassocSelectionWithWidget OLARGLIST (( vendor, dnd_part,
						widget, selection, timestamp))
		OLARG( Widget,				vendor)
		OLARG( OlDnDVendorPartExtension,	dnd_part)
		OLARG( Widget,				widget)
		OLARG( Atom,				selection)
		OLGRA( Time,				timestamp)
{
	DSSelectionAtomPtr	*p, q;

	FetchPartExtensionIfNull(vendor, dnd_part);

	for (;;) {

		for (p = &dnd_part->selection_atoms;
		     *p != (DSSelectionAtomPtr)NULL;
		     p = &DSSelectionAtomPtrNext(*p)) {
		     if (DSSelectionAtomPtrOwner(*p) == widget &&
		         (selection == FreeAllSelectionAtoms ||
		          selection == DSSelectionAtomPtrSelectionAtom(*p)))
				break;
		}

		if (*p == (DSSelectionAtomPtr)NULL)
			return; /* its not there */

		q = *p;
		*p = DSSelectionAtomPtrNext(*p); /* unlink from ds */

		if (DSSelectionAtomPtrClosure(q) != 
		    (OwnerProcClosurePtr)NULL && 
		    OwnerProcClosurePtrCleanupProc(
				DSSelectionAtomPtrClosure(q))
		    != (OlDnDTransactionCleanupProc)NULL)
			(*OwnerProcClosurePtrCleanupProc(
				DSSelectionAtomPtrClosure(q)))
					(DSSelectionAtomPtrClosure(q));
	
		XtFree((char *)q);

		if (selection != FreeAllSelectionAtoms)
			return;	/* all done */
	}
}

/******************************************************************************
 *
 * DnDVCXGetCurrentSelectionsForWidget
 *
 * find the list of selections active for the widget.
 *
 ******************************************************************************/

static Atom     
*DnDVCXGetCurrentSelectionsForWidget OLARGLIST((vendor, dnd_part,
						  widget, num_sites_return))
			OLARG( Widget,			 vendor)
			OLARG( OlDnDVendorPartExtension, dnd_part)
			OLARG( Widget,			 widget)
			OLGRA( Cardinal *,		 num_sites_return)
{
	Atom			*atoms, *a;
	DSSelectionAtomPtr	p;

	*num_sites_return = 0;

	FetchPartExtensionIfNull(vendor, dnd_part);

	for (p = dnd_part->selection_atoms;
	     p != (DSSelectionAtomPtr)NULL; 
	     p = DSSelectionAtomPtrNext(p))
		if (DSSelectionAtomPtrOwner(p) == widget)
			*num_sites_return++;

	if (!*num_sites_return)
		return (Atom *)NULL;

	a = atoms = (Atom *)XtCalloc(*num_sites_return, sizeof(Atom));

	for (p = dnd_part->selection_atoms;
	     p != (DSSelectionAtomPtr)NULL; 
	     p = DSSelectionAtomPtrNext(p))
		if (DSSelectionAtomPtrOwner(p) == widget)
			*a++ = DSSelectionAtomPtrSelectionAtom(p);

	return atoms;
}

/******************************************************************************
 *
 * DnDVCXGetDropSiteForSelection
 *
 * find the widget associated with a particular selection atom.
 *
 ******************************************************************************/

static Widget
DnDVCXGetWidgetForSelection OLARGLIST(( vendor, dnd_part, selection, closure))
		OLARG( Widget,				vendor)
		OLARG( OlDnDVendorPartExtension,	dnd_part)
		OLARG( Atom,				selection)
		OLGRA( OwnerProcClosurePtr *,		closure)
{
	DSSelectionAtomPtr	p;
	Widget			w;

	FetchPartExtensionIfNull(vendor, dnd_part);

	for (p = dnd_part->selection_atoms;
	     p != (DSSelectionAtomPtr)NULL &&
	     selection != DSSelectionAtomPtrSelectionAtom(p);
	     p = DSSelectionAtomPtrNext(p))
	;

	if (p != (DSSelectionAtomPtr)NULL) {
		w = DSSelectionAtomPtrOwner(p);
		*closure = DSSelectionAtomPtrClosure(p);
	} else {
		w = (Widget)NULL;
		*closure = (OwnerProcClosurePtr)NULL;
	}
	
	return (w);
}

/******************************************************************************
 *
 * DnDVCXChangeSitePreviewHints
 *
 * update the site peview hints ....
 *
 ******************************************************************************/

static Boolean
DnDVCXChangeSitePreviewHints OLARGLIST(( vendor, dnd_part,
					dropsiteid, new_hints))
		OLARG( Widget,				vendor)
		OLARG( OlDnDVendorPartExtension,	dnd_part)
		OLARG( OlDnDDropSiteID,			dropsiteid)
		OLGRA( OlDnDSitePreviewHints,		new_hints)
{
	OlDnDDropSitePtr	dsp;

	FetchPartExtensionIfNull(vendor, dnd_part);

        for (dsp = dnd_part->drop_site_list;
             dsp != (OlDnDDropSitePtr)NULL &&
             dsp != (OlDnDDropSitePtr)dropsiteid;
                dsp = OlDnDDropSitePtrNextSite(dsp))
        ;

	if (dsp == (OlDnDDropSitePtr)NULL) {
		return (False);
	}

	new_hints &= (OlDnDSitePreviewNone	 | /* redundant */
		      OlDnDSitePreviewEnterLeave |
		      OlDnDSitePreviewMotion     |
		      OlDnDSitePreviewBoth       | /* redundant */
		      OlDnDSitePreviewDefaultSite|
		      OlDnDSitePreviewInsensitive);

	if (OlDnDDropSitePtrPreviewHints(dsp) == new_hints) {
		return (True);
	}
	
	if ((OlDnDDropSitePtrPreviewHints(dsp) & OlDnDSitePreviewDefaultSite)
	    && !(new_hints & OlDnDSitePreviewDefaultSite)) {
		dnd_part->default_drop_site = (OlDnDDropSitePtr)NULL;
	}

	OlDnDDropSitePtrPreviewHints(dsp) = new_hints;

	if ((OlDnDDropSitePtrPreviewHints(dsp) & OlDnDSitePreviewDefaultSite)
	    && dnd_part->default_drop_site != dsp) {
		dnd_part->default_drop_site = dsp;
	}

	dnd_part->dirty = True;

	if (dnd_part->auto_assert_dropsite_registry &&
	    dnd_part->class_extension->assert_drop_site_registry !=
	    (OlDnDVCXAssertRegistryProc)NULL) {
		(*dnd_part->class_extension->assert_drop_site_registry)
			(vendor, dnd_part);
	}
	
	return (True);
}

/******************************************************************************
 *
 * DnDVCXSetDropSiteOnInterest
 *
 ******************************************************************************/

static Boolean
DnDVCXSetDropSiteOnInterest OLARGLIST(( vendor, dnd_part,
					dropsiteid, on_interest, validate))
		OLARG( Widget,				vendor)
		OLARG( OlDnDVendorPartExtension,	dnd_part)
		OLARG( OlDnDDropSiteID,			dropsiteid)
		OLARG( Boolean,				on_interest)
		OLGRA( Boolean,				validate)
{
	OlDnDDropSitePtr	dsp;
	Boolean			save;

	FetchPartExtensionIfNull(vendor, dnd_part);

	if (validate)
		for (dsp = dnd_part->drop_site_list;
		     dsp != (OlDnDDropSitePtr)NULL &&
		     dsp != (OlDnDDropSitePtr)dropsiteid;
			dsp = OlDnDDropSitePtrNextSite(dsp))
		;

	if (dsp == (OlDnDDropSitePtr)NULL) {
		return (False);
	}

	save = OlDnDDropSitePtrOnInterest(dsp);
	OlDnDDropSitePtrOnInterest(dsp) = on_interest;

	if ((!save && on_interest) || (save && !on_interest)) {
		dnd_part->dirty = True;

		if (dnd_part->auto_assert_dropsite_registry &&
		    dnd_part->class_extension->assert_drop_site_registry !=
		    (OlDnDVCXAssertRegistryProc)NULL) {
			(*dnd_part->class_extension->assert_drop_site_registry)
				(vendor, dnd_part);

			if (dnd_part->class_extension->fetch_dsdm_info !=
			    (OlDnDVCXFetchDSDMInfoFunc)NULL) {
				(*dnd_part->class_extension->fetch_dsdm_info)
					(vendor, dnd_part, 
					 XtLastTimestampProcessed(
						XtDisplay(vendor)));
			}
		}
	}
	return (True);
}

/******************************************************************************
 *
 * DnDVCXSetInterestInWidgetHier
 *
 ******************************************************************************/

static OlDnDDropSitePtr	*_site_list = (OlDnDDropSitePtr *)NULL;

static OlDnDVendorPartExtension	_dnd_part = (OlDnDVendorPartExtension)NULL;
static Boolean			_interest;
static Widget			_vendor;


static int
#if defined(att) || defined(USL)
_comparWidgets OLARGLIST((i, j))
	OLARG( OLconst void *,	i)
	OLGRA( OLconst void *,	j)
#else
_comparWidgets(i, j)
	int *i, *j;
#endif
{
	OlDnDDropSitePtr	ip = (OlDnDDropSitePtr)i,
				jp = (OlDnDDropSitePtr)j;

	return	((int)OlDnDDropSitePtrOwner(jp) -
		 (int)OlDnDDropSitePtrOwner(ip));
}

static void
_recurshier(w)
	Widget	w;
{
	register OlDnDDropSitePtr	dsp;
	register CompositeWidget	comp = (CompositeWidget)w;

	dsp = (OlDnDDropSitePtr)bsearch((char *)&w, (char *)_site_list,
					_dnd_part->number_of_sites, 
					sizeof(Widget), _comparWidgets);

	if (dsp != (OlDnDDropSitePtr)NULL && 
	    _dnd_part->class_extension->set_ds_on_interest !=
	    (OlDnDVCXSetDSOnInterestFunc)NULL) {
		(*_dnd_part->class_extension->set_ds_on_interest)
				(_vendor, _dnd_part, (OlDnDDropSiteID)dsp,
				 _interest && w->core.mapped_when_managed,
				 False);
	}

	if (XtIsComposite(w) && comp->composite.num_children) {
		register Cardinal 	n;
		register WidgetList	child = comp->composite.children;

		for (n = 0; n < comp->composite.num_children; n++) {
			_recurshier(child[n]);
		}
	}
}

static void
DnDVCXSetInterestInWidgetHier OLARGLIST(( vendor, dnd_part, widget, on_interest))
		OLARG( Widget,				vendor)
		OLARG( OlDnDVendorPartExtension,	dnd_part)
		OLARG( Widget,				widget)
		OLGRA( Boolean,				on_interest)
{
	OlDnDDropSitePtr	dsp, *p;
	Boolean			save;

	FetchPartExtensionIfNull(vendor, dnd_part);
	
	if (dnd_part->number_of_sites == 0) return;

	p = _site_list = (OlDnDDropSitePtr *)XtCalloc(dnd_part->number_of_sites,
						      sizeof(OlDnDDropSitePtr));

	for (dsp = dnd_part->drop_site_list;
	     dsp != (OlDnDDropSitePtr)NULL;
	     (dsp = OlDnDDropSitePtrNextSite(dsp)), p++)
		*p = dsp;
	     
	qsort((char *)_site_list, dnd_part->number_of_sites, 
	      sizeof(OlDnDDropSitePtr), _comparWidgets);

	save = dnd_part->auto_assert_dropsite_registry;
	dnd_part->auto_assert_dropsite_registry = False;

	_interest = on_interest;
	_vendor = vendor;
	_dnd_part = dnd_part;

	_recurshier(widget);

	dnd_part->auto_assert_dropsite_registry = save;

	if (dnd_part->auto_assert_dropsite_registry &&
	    dnd_part->class_extension->assert_drop_site_registry !=
	    (OlDnDVCXAssertRegistryProc)NULL) {
		(*dnd_part->class_extension->assert_drop_site_registry)
			(vendor, dnd_part);

		if (dnd_part->class_extension->fetch_dsdm_info !=
		    (OlDnDVCXFetchDSDMInfoFunc)NULL) {
			(*dnd_part->class_extension->fetch_dsdm_info)
					(vendor, dnd_part, 
					 XtLastTimestampProcessed(
						XtDisplay(vendor)));
		}
	}
	
	XtFree((char *)_site_list);
	_site_list = (OlDnDDropSitePtr *)NULL;
}

#ifndef GUI_DEP
/*
 * DnDGetClassExtension -
 *
 *	This routine gets a class extension from a class's extension list.
 *
 */
static OlDnDClassExtensionHdrPtr
DnDGetClassExtension OLARGLIST((extension, record_type, version))
        OLARG( OlDnDClassExtensionHdrPtr,extension)   /* first extension   */
        OLARG( XrmQuark,		record_type) /* type to look for  */
        OLGRA( long,			version)     /* if non-zero, look */
						     /* for it		  */
{
	while (extension != (OlDnDClassExtensionHdrPtr)NULL &&
		!(extension->record_type == record_type &&
		(!version || version == extension->version)))
	{
	     extension = (OlDnDClassExtensionHdrPtr)extension->next_extension;
	}
        return (extension);
} /* end of DnDGetClassExtension */

/*
 * DnDGetShellOfWidget -
 *
 * This routine starts at the given widget looking
 * to see if the widget is a shell widget.  If it is not, it searches up
 * the widget tree until it finds a shell or until a NULL widget is
 * encountered.  The procedure returns either the located shell widget or
 * NULL.
 *
 */
static Widget
DnDGetShellOfWidget OLARGLIST((w))
        OLGRA( register Widget,	w)	/* Widget to begin search at    */
{
	while(w != (Widget) NULL && !XtIsShell(w))
		w = w->core.parent;

	return(w);
} /* end of DnDGetShellOfWidget */
#endif

extern void
OlDnDWidgetConfiguredInHier OLARGLIST((widget))
	OLGRA( Widget,	widget)
{
	XtWarning(
	  "OlDnDWidgetConfiguredInHier is not implemented in this software");
}
