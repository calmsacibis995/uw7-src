#ifndef NOIDENT
#ident	"@(#)menu:MenuShellP.h	1.21"
#endif

/* 
 * MenuShellP.h
 */

#ifndef _MenuShellP_h
#define _MenuShellP_h

#include <X11/Xlib.h>
#include <X11/ShellP.h>		/* superclass' header */
#include <Xol/MenuShell.h>	/* public header */
#include <Xol/Olg.h>
  
typedef struct {
    char no_class_fields;		/* Makes compiler happy */
} PopupMenuShellClassPart;

typedef struct _PopupMenuShellClassRec
   {
   CoreClassPart                core_class;
   CompositeClassPart           composite_class;
   ShellClassPart               shell_class;
   WMShellClassPart             wm_shell_class;
   VendorShellClassPart         vendor_shell_class;
   TransientShellClassPart      transient_shell_class;
   PopupMenuShellClassPart      popup_menu_shell_class;
   } PopupMenuShellClassRec;

typedef struct
   {

   /* resource members */
   XFontStruct *  font;
   OlFontList *	  font_list;		/* new for 4m			*/
   Pixel          fontColor;
   Pixel          foreground;
   Boolean        pushpinDefault;
   Dimension      shadow_thickness;	/* XtNshadowThickness  */

   /* private members */
   GC            gc;
   XRectangle    pin;
   XRectangle    title;
   OlgAttrs *    attrs;
   Window	 fake_window;
   
   /* MooLIT extension */
   Boolean	option_menu;	/* private resource for now... */
   Cursor       cursor;
   } PopupMenuShellPart;

typedef struct _PopupMenuShellRec
   {
   CorePart             core;
   CompositePart        composite;
   ShellPart            shell;
   WMShellPart          wm;
   VendorShellPart      vendor;
   TransientShellPart   transient;
   PopupMenuShellPart   popup_menu_shell;
   } PopupMenuShellRec;

extern PopupMenuShellClassRec popupMenuShellClassRec;



/*
 * The following are private functions. No man pages will be provided.
 * This is your risk if you decide to use the functions below...
 *
 */
/*
 * Private types:
 */

typedef enum
   {
   DropDownAlignment,
   AbbrevDropDownAlignment,
   PullAsideAlignment, 
   PopupAlignment
   } OlMenuAlignment;

typedef enum
   {
   PENDING_STAYUP,
   NO_STAYUP,
   IN_STAYUP
   } OlStayupMode;

#define _OlIsInStayupMode(w)	(_OlGetStayupMode(w) == IN_STAYUP)

#define _OlIsNotInStayupMode(w)	(_OlGetStayupMode(w) == NO_STAYUP)

#define _OlIsPendingStayupMode(w) (_OlGetStayupMode(w) == PENDING_STAYUP)

#define _OlResetPreviewMode(w)	_OlSetPreviewModeValue(w,True)

#define _OlResetStayupMode(w)	_OlSetStayupModeValue(w,NO_STAYUP)

#define _OlSetPreviewMode(w)	_OlSetPreviewModeValue(w,False)

#define	_OlSetStayupMode(w)	_OlSetStayupModeValue(w,IN_STAYUP)



extern Boolean	_OlGetMenuDefault OL_ARGS((Widget, Widget *,
					   Cardinal *, Boolean));

extern OlStayupMode
		_OlGetStayupMode OL_ARGS((Widget));

extern Boolean	_OlInPreviewMode OL_ARGS((Widget));

extern void	_OlInitStayupMode OL_ARGS((Widget, Window, Position, Position));

extern Boolean	_OlIsEmptyMenuStack OL_ARGS((Widget));

extern Boolean	_OlIsInMenuStack OL_ARGS((Widget));

extern void	_OlMenuLock OL_ARGS((Widget, Widget, XEvent *));

extern Cardinal	_OlParentsInMenuStack OL_ARGS((Widget, WidgetList *));

extern Widget	_OlPopMenuStack  OL_ARGS((Widget, Widget *,
					  OlPopupMenuCallbackProc *));

extern void	_OlPopdownCascade OL_ARGS((Widget, Boolean));

extern void	_OlPopupMenu OL_ARGS((Widget, Widget,
				      OlPopupMenuCallbackProc,
				      XRectangle *, OlMenuAlignment, 
                                      Boolean, Window, Position, Position));

extern void	_OlPreviewMenuDefault OL_ARGS((Widget, Widget, Cardinal));

extern void	_OlPushMenuStack OL_ARGS((Widget, Widget,
					  OlPopupMenuCallbackProc));


extern void	_OlQueryResetStayupMode OL_ARGS((Widget, Window,
						 Position, Position));

extern Widget	_OlRootOfMenuStack OL_ARGS((Widget));

extern Widget	_OlRoot1OfMenuStack OL_ARGS((Widget));

extern Widget	_OlRootParentOfMenuStack OL_ARGS((Widget));

extern void	_OlSetPreviewModeValue OL_ARGS((Widget, Boolean));

extern void	_OlSetStayupModeValue OL_ARGS((Widget, OlStayupMode));

extern Widget	_OlTopOfMenuStack OL_ARGS((Widget));

extern Widget	_OlTopParentOfMenuStack OL_ARGS((Widget));

/* GUI specific functions...	*/
extern void _OloMSHandlePushpin OL_ARGS((
			PopupMenuShellWidget, PopupMenuShellPart *, OlDefine,
		char));
extern void _OlmMSHandlePushpin OL_ARGS((
			PopupMenuShellWidget, PopupMenuShellPart *, OlDefine,
		char));

extern void _OloMSHandleTitle OL_ARGS((
			PopupMenuShellWidget, PopupMenuShellPart *, char));
extern void _OlmMSHandleTitle OL_ARGS((
			PopupMenuShellWidget, PopupMenuShellPart *, char));
extern void	_OloMSLayoutPreferred OL_ARGS((
	Widget			w,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	Dimension *		width,
	Dimension *		height,
	Position *		xpos,
	Position *		ypos,
	Dimension *		child_space
));
extern void	_OlmMSLayoutPreferred OL_ARGS((
	Widget			w,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	Dimension *		width,
	Dimension *		height,
	Position *		xpos,
	Position *		ypos,
	Dimension *		child_space
));

extern void _OloMSCreateButton OL_ARGS((Widget));
extern void _OlmMSCreateButton OL_ARGS((Widget));

#if	!defined(OBJECT_C)
#define OBJECT_C(WC) ((ObjectClass)(WC))->object_class
#define OBJECT_P(W) ((Object)(W))->object
#define RECT_C(WC) ((RectObjClass)(WC))->rect_class
#define RECT_P(W) ((RectObj)(W))->rectangle
#define CORE_C(WC) ((WidgetClass)(WC))->core_class
#define CORE_P(W) ((Widget)(W))->core
#define SUPER_C(WC) CORE_C(WC).superclass
#define CLASS(WC) CORE_C(WC).class_name
#endif

#define COMPOSITE_C(WC) ((CompositeWidgetClass)(WC))->composite_class
#define CONSTRAINT_C(WC) ((ConstraintWidgetClass)(WC))->constraint_class
#define POPUPMENU_C(WC) ((PopupMenuShellWidgetClass)(WC))->popup_menu_shell_class

#define COMPOSITE_P(W) ((CompositeWidget)(W))->composite
#define POPUPMENU_P(W) ((PopupMenuShellWidget)(W))->popup_menu_shell

#define FOR_EACH_CHILD(W,CHILD,N) \
	for (N = 0; N < COMPOSITE_P(W).num_children			\
		 && (CHILD = COMPOSITE_P(W).children[N]); N++)

#define FOR_EACH_MANAGED_CHILD(W,CHILD,N) \
	FOR_EACH_CHILD (W, CHILD, N)					\
		if (XtIsManaged(CHILD))

#endif /* _MenuShellP_h */
