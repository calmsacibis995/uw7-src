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
#ident	"@(#)oldnd:OlDnDVCXP.h	1.5"
#endif

#ifndef	_OlDnDVCXP_h_
#define	_OlDnDVCXP_h_

#include <X11/ShellP.h> 
#include <X11/VendorP.h>

#ifdef GUI_DEP

#include <Xol/OpenLookP.h>

#include <Xol/OlDnDVCXI.h>
#define GetClassExt	_OlGetClassExtension

#else	/* GUI_DEP	*/

#include <DnD/OlDnDVCXI.h>
#define GetClassExt	DnDGetClassExtension

	/*
	 * Xt assumes that the first 4 fields in an extension record
	 * are mandatory. These four fields are (see ShellP.h,
	 * ConstrainP.h etc. Xt header for example) defined as
	 * "OlDnDClassExtensionHdr" below:
	 *
	 */
typedef struct {
	XtPointer	next_extension;
	XrmQuark	record_type;
	long		version;
	Cardinal	record_size;
} OlDnDClassExtensionHdr, *OlDnDClassExtensionHdrPtr;

#endif	/* GUI_DEP	*/

#define GET_DND_EXT(wc) (OlDnDVendorClassExtension)GetClassExt(            \
			  (OlDnDClassExtensionHdrPtr)VCLASS(wc,extension), \
			  OlXrmDnDVendorClassExtension,                    \
			  OlDnDVendorClassExtensionVersion)

/*************************** forward decls for Drag and Drop **************/

typedef	struct _OlDnDVendorPartExtensionRec  *OlDnDVendorPartExtension;
typedef struct _OlDnDVendorClassExtensionRec *OlDnDVendorClassExtension;

/* public procedures */

extern OlDnDVendorClassExtension        _OlGetDnDVendorClassExtension
                                                OL_ARGS((WidgetClass));

extern OlDnDVendorPartExtension _OlGetDnDVendorPartExtension OL_ARGS((Widget));

extern void _OlDnDDoExtensionClassInit();
extern void _OlDnDDoExtensionClassPartInit OL_ARGS((WidgetClass));
extern void _OlDnDCallVCXPostRealizeSetup OL_ARGS((Widget));

typedef	enum	{ CallDnDVCXInitialize, 
		  CallDnDVCXDestroy,
		  CallDnDVCXSetValues,
		  CallDnDVCXGetValues } DnDVCXMethodType;

extern	Boolean	CallDnDVCXExtensionMethods OL_ARGS(( DnDVCXMethodType,
						     WidgetClass, Widget,
						     Widget, Widget,
						     ArgList, Cardinal *));


extern Widget
GetShellParentOfWindow OL_ARGS (( Display *, Window ));

/********************************************
 *
 * OLDnDVendorPartExtensionRec
 *
 ********************************************/

typedef	struct _OlDnDVendorPartExtensionRec {
	OlDnDVendorPartExtension	next_part;
	Widget				owner;
	OlDnDVendorClassExtension	class_extension;

	Time			registry_update_timestamp;
	Time			dsdm_last_loaded;
	unsigned int		number_of_sites;
	OlDnDDropSitePtr	drop_site_list;
	Position		root_x;
	Position		root_y;
	Boolean			auto_assert_dropsite_registry;
	Boolean			dirty;
	Boolean			doing_drag;
	Boolean			dsdm_present;
	Boolean			preview_forwarded_sites;
	OlDnDDropSitePtr	default_drop_site;

	InternalDSRPtr		dropsite_rects;		/* private */
	InternalDSRPtr		current_dsr;		/* private */

	OlDnDDropSitePtr	current_dsp;		/* private */

	InternalDSDMSRPtr	*dsdm_rects;		/* private */
	InternalDSDMSRPtr	current_dsdmsr;		/* private */
	InternalDSDMSRPtr	internal_dsdm_sr_list;	/* private */
	DSDMSiteRectPtr		dsdm_sr_list;		/* private */
	unsigned long		num_dsdmsrs;		/* private */
	Boolean			pending_dsdm_info;	/* private */

	TransientAtomListPtr	transient_atoms;	/* private */
	DSSelectionAtomPtr	selection_atoms;	/* private */
} OlDnDVendorPartExtensionRec /*, *OlDnDVendorPartExtension*/;

/********************************************
 *
 * OLDnDVendorClassExtensionRec
 *
 ********************************************/

/*
 *
 * Convention: where a class extension method expects a class extension
 *	       pointer, if this is set to NULL then the method must
 *	       do a lookup for the extension. This is primarily used for
 *	       performance improvements where the caller already has the
 *	       necessary info .... and therefore passes in the extension
 *	       pointer reducing the lookup cost.
 *
 */

typedef void (*OlDnDVCXClassInitializeProc)
				OL_ARGS((OlDnDVendorClassExtension));

typedef void (*OlDnDVCXClassPartInitializeProc) OL_ARGS(( WidgetClass ));

typedef	Boolean	(*OlDnDVCXSetValuesFunc) OL_ARGS((Widget, Widget, Widget,
						  ArgList, Cardinal *,
						  OlDnDVendorPartExtension,
						  OlDnDVendorPartExtension,
						  OlDnDVendorPartExtension));

typedef	void	(*OlDnDVCXGetValuesProc) OL_ARGS((Widget, ArgList,
						  Cardinal *,
						  OlDnDVendorPartExtension));

typedef	void	(*OlDnDVCXInitializeProc) OL_ARGS((Widget, Widget,
						   ArgList, Cardinal *,
						   OlDnDVendorPartExtension,
						   OlDnDVendorPartExtension));
typedef	void	(*OlDnDVCXPostRealizeSetupProc) 
			OL_ARGS((Widget, OlDnDVendorClassExtension,
				 OlDnDVendorPartExtension));

typedef	void	(*OlDnDVCXDestroyProc) OL_ARGS((Widget, 
						OlDnDVendorPartExtension));

typedef	Boolean	(*OlDnDVCXTriggerMessageDispatcherFunc) 
			OL_ARGS(( Widget, OlDnDVendorPartExtension,
				  TriggerMessagePtr ));

typedef	OlDnDVCXTriggerMessageDispatcherFunc OlDnDVCXTMDispatcherFunc;

typedef	Boolean (*OlDnDVCXPreviewMessageDispatcherFunc)
			OL_ARGS(( Widget, OlDnDVendorPartExtension,
				  PreviewMessagePtr));

typedef	OlDnDVCXPreviewMessageDispatcherFunc OlDnDVCXPMDispatcherFunc;

typedef	OlDnDDropSiteID (*OlDnDVCXRegisterDSFunc)
				OL_ARGS(( Widget, OlDnDVendorPartExtension,
					  Widget, Window,
					  OlDnDSitePreviewHints,
					  OlDnDSiteRectPtr,
					  unsigned int,
					  OlDnDTriggerMessageNotifyProc,
					  OlDnDPreviewMessageNotifyProc,
					  Boolean,
					  XtPointer ));

typedef Boolean	(*OlDnDVCXUpdateDSGeometryProc) OL_ARGS((Widget,
						  OlDnDVendorPartExtension,
					  	  OlDnDDropSiteID,
                                                  OlDnDSiteRectPtr,
                                                  unsigned int ));



typedef void	(*OlDnDVCXDeleteDSProc) OL_ARGS((Widget, 
						 OlDnDVendorPartExtension,
						 OlDnDDropSiteID ));

typedef Boolean (*OlDnDVCXQueryDSInfoFunc) OL_ARGS ((Widget,
						     OlDnDVendorPartExtension,
						     OlDnDDropSiteID,
                                                     Widget *,
                                                     Window *,
                                                     OlDnDSitePreviewHints *,
                                                     OlDnDSiteRectPtr *,
                                                     unsigned int *,
						     Boolean * ));


typedef	void	(*OlDnDVCXAssertRegistryProc) 
			OL_ARGS(( Widget, OlDnDVendorPartExtension ));
typedef	void	(*OlDnDVCXDeleteRegistryProc)
			OL_ARGS(( Widget, OlDnDVendorPartExtension ));

typedef	Boolean	(*OlDnDVCXFetchDSDMInfoFunc)
			OL_ARGS(( Widget, OlDnDVendorPartExtension, Time ));

typedef	Boolean	(*OlDnDVCXDeliverTriggerMessageFunc)
			OL_ARGS (( Widget, OlDnDVendorPartExtension,
                                   Widget, Window, int, int, Atom,
				   OlDnDTriggerOperation, Time));

typedef	OlDnDVCXDeliverTriggerMessageFunc	OlDnDVCXDeliverTMFunc;

typedef	Boolean (*OlDnDVCXDeliverPreviewMessageFunc)
			OL_ARGS (( Widget, OlDnDVendorPartExtension,
                                   Widget, Window, int, int, Time,
				   OlDnDPreviewAnimateCallbackProc, 
				   XtPointer ));

typedef	OlDnDVCXDeliverPreviewMessageFunc	OlDnDVCXDeliverPMFunc;

typedef	Boolean (*OlDnDVCXInitializeDragStateFunc)
				OL_ARGS ((Widget,
					  OlDnDVendorPartExtension ));

typedef	void (*OlDnDVCXClearDragStateProc) OL_ARGS ((Widget,
						     OlDnDVendorPartExtension ));

typedef Atom (*OlDnDVCXAllocTransientAtomFunc)
				OL_ARGS (( Widget,
					   OlDnDVendorPartExtension,
					   Widget ));

typedef void (*OlDnDVCXFreeTransientAtomProc)
				OL_ARGS (( Widget,
					   OlDnDVendorPartExtension,
					   Widget, Atom ));

typedef DSSelectionAtomPtr (*OlDnDVCXAssocSelectionFunc) OL_ARGS(( Widget,
						       OlDnDVendorPartExtension,
						       Widget,
						       Atom,
						       Time,
						       OwnerProcClosurePtr));

typedef void (*OlDnDVCXDisassocSelectionProc) OL_ARGS(( Widget,
						       OlDnDVendorPartExtension,
						       Widget,
						       Atom, Time ));

typedef Atom *(*OlDnDVCXGetCurrentSelectionsFunc)
					OL_ARGS (( Widget,
					           OlDnDVendorPartExtension,
                                                   Widget,
                                                   Cardinal * ));

typedef Widget (*OlDnDVCXGetSelectionFunc)
					OL_ARGS (( Widget,
					           OlDnDVendorPartExtension,
                                                   Atom,
						   OwnerProcClosurePtr *));

typedef	Boolean (*OlDnDVCXChangeSitePreviewHintsFunc)
					OL_ARGS (( Widget,
						   OlDnDVendorPartExtension,
						   OlDnDDropSiteID,
						   OlDnDSitePreviewHints ));

typedef	Boolean	(*OlDnDVCXSetDSOnInterestFunc)
					OL_ARGS (( Widget,
						   OlDnDVendorPartExtension,
						   OlDnDDropSiteID,
						   Boolean, Boolean ));

typedef	void	(*OlDnDVCXSetInterestWidgetHierFunc)
					OL_ARGS (( Widget,
						   OlDnDVendorPartExtension,
						   Widget, Boolean ));

/* inheritance tokens */

#define XtInheritOlDnDVCXPostRealizeSetupProc	\
			((OlDnDVCXPostRealizeSetupProc) _XtInherit)
#define XtInheritOlDnDVCXRegisterDSFunc		\
			((OlDnDVCXRegisterDSFunc) _XtInherit)
#define XtInheritOlDnDVCXUpdateDSGeometryProc	\
			((OlDnDVCXUpdateDSGeometryProc) _XtInherit)
#define XtInheritOlDnDVCXDeleteDSProc		\
			((OlDnDVCXDeleteDSProc) _XtInherit)
#define XtInheritOlDnDVCXQueryDSInfoFunc	\
			((OlDnDVCXQueryDSInfoFunc) _XtInherit)
#define XtInheritOlDnDVCXAssertRegistryProc	\
			((OlDnDVCXAssertRegistryProc) _XtInherit)
#define XtInheritOlDnDVCXDeleteRegistryProc	\
			((OlDnDVCXDeleteRegistryProc) _XtInherit)
#define XtInheritOlDnDVCXFetchDSDMInfoFunc	\
			((OlDnDVCXFetchDSDMInfoFunc) _XtInherit)
#define XtInheritOlDnDVCXTMDispatcherFunc	\
			((OlDnDVCXTMDispatcherFunc) _XtInherit)
#define XtInheritOlDnDVCXPMDispatcherFunc	\
			((OlDnDVCXPMDispatcherFunc) _XtInherit)
#define XtInheritOlDnDVCXDeliverTMFunc		\
			((OlDnDVCXDeliverTMFunc) _XtInherit)
#define XtInheritOlDnDVCXDeliverPMFunc			\
			((OlDnDVCXDeliverPMFunc) _XtInherit)
#define XtInheritOlDnDVCXInitializeDragStateFunc	\
			((OlDnDVCXInitializeDragStateFunc) _XtInherit)
#define XtInheritOlDnDVCXClearDragStateProc		\
			((OlDnDVCXClearDragStateProc) _XtInherit)
#define XtInheritOlDnDVCXAllocTransientAtomFunc		\
			((OlDnDVCXAllocTransientAtomFunc) _XtInherit)
#define XtInheritOlDnDVCXFreeTransientAtomProc		\
			((OlDnDVCXFreeTransientAtomProc) _XtInherit)
#define XtInheritOlDnDVCXAssocSelectionFunc		\
			((OlDnDVCXAssocSelectionFunc) _XtInherit)
#define XtInheritOlDnDVCXDisassocSelectionProc		\
			((OlDnDVCXDisassocSelectionProc) _XtInherit)
#define XtInheritOlDnDVCXGetCurrentSelectionsFunc		\
			((OlDnDVCXGetCurrentSelectionsFunc) _XtInherit)
#define XtInheritOlDnDVCXGetSelectionFunc			\
			((OlDnDVCXGetSelectionFunc) _XtInherit)
#define XtInheritOlDnDVCXChangeSitePreviewHintsFunc		\
			((OlDnDVCXChangeSitePreviewHintsFunc) _XtInherit)
#define XtInheritOlDnDVCXSetDSOnInterestFunc			\
			((OlDnDVCXSetDSOnInterestFunc) _XtInherit)
#define XtInheritOlDnDVCXSetInterestWidgetHierFunc		\
			((OlDnDVCXSetInterestWidgetHierFunc) _XtInherit)


/*
 * the class extension itself.
 */

typedef	struct _OlDnDVendorClassExtensionRec {
#ifdef GUI_DEP
	OlClassExtensionRec		header;
#else
	OlDnDClassExtensionHdr		header;
#endif

	Cardinal			instance_part_size;
	OlDnDVendorPartExtension	instance_part_list;
	XtResourceList			resources;
	Cardinal			num_resources;

	OlDnDVCXClassInitializeProc	class_initialize;
	OlDnDVCXClassPartInitializeProc	class_part_initialize;
	XtEnum				class_inited;

	OlDnDVCXSetValuesFunc		set_values;
	OlDnDVCXGetValuesProc		get_values;
	OlDnDVCXInitializeProc		initialize;
	OlDnDVCXPostRealizeSetupProc	post_realize_setup;
	OlDnDVCXDestroyProc		destroy;

	OlDnDVCXTMDispatcherFunc	trigger_message_dispatcher;
	OlDnDVCXPMDispatcherFunc	preview_message_dispatcher;

	OlDnDVCXRegisterDSFunc		register_drop_site;
	OlDnDVCXUpdateDSGeometryProc	update_drop_site_geometry;
	OlDnDVCXDeleteDSProc		delete_drop_site;
	OlDnDVCXQueryDSInfoFunc		query_drop_site_info;

	OlDnDVCXAssertRegistryProc	assert_drop_site_registry;
	OlDnDVCXDeleteRegistryProc	delete_drop_site_registry;

	OlDnDVCXFetchDSDMInfoFunc	fetch_dsdm_info;

	OlDnDVCXDeliverTMFunc		deliver_trigger_message;
	OlDnDVCXDeliverPMFunc		deliver_preview_message;

	OlDnDVCXInitializeDragStateFunc	initialize_drag_state;
	OlDnDVCXClearDragStateProc	clear_drag_state;

	OlDnDVCXAllocTransientAtomFunc	alloc_transient_atom;
	OlDnDVCXFreeTransientAtomProc	free_transient_atom;

	OlDnDVCXAssocSelectionFunc		associate_selection_and_w;
	OlDnDVCXDisassocSelectionProc 		disassociate_selection_and_w;
	OlDnDVCXGetCurrentSelectionsFunc	get_w_current_selections;
	OlDnDVCXGetSelectionFunc		get_w_for_selection;

	OlDnDVCXChangeSitePreviewHintsFunc	change_site_hints;
	OlDnDVCXSetDSOnInterestFunc		set_ds_on_interest;
	OlDnDVCXSetInterestWidgetHierFunc	set_interest_in_widget_hier;
} OlDnDVendorClassExtensionRec /*, *OlDnDVendorClassExtension*/;

extern OlDnDVendorClassExtensionRec dnd_vendor_extension_rec;
extern OlDnDVendorClassExtension    dnd_vendor_extension;

extern	XrmQuark	OlXrmDnDVendorClassExtension;

#define	OlDnDVendorClassExtensionVersion	1L
#define	OlDnDVendorClassExtensionName	"OlDnDVendorClassExtension"

#endif	/* _OlDnDVCXP_h_ */
