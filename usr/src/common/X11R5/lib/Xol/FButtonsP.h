#ifndef	NOIDENT
#ident	"@(#)flat:FButtonsP.h	1.23"
#endif

#ifndef _FButtonsP_h
#define _FButtonsP_h

/*
 ************************************************************************	
 * Description:
 *	This is the flat menu button container's private header file.
 ************************************************************************	
 */

#include <Xol/FRowColumP.h>	/* superclasses' private header file */
#include <Xol/FButtons.h>	/* public header file */

#define FEPART(w)  ( &((FlatButtonsWidget)(w))->buttons )
#define FECPART(w) ( &((FlatButtonsWidgetClass)XtClass(w))->buttons_class )
#define FEIPART(i) ( &((FlatButtonsItem)(i))->buttons )

#define FPART(w)   ( &((FlatWidget)(w))->flat )
#define FCPART(w)  ( &((FlatWidgetClass)XtClass(w))->flat_class )
#define RCPART(w)  ( &((FlatRowColumnWidget)(w))->row_column )

#define PADDING    4	/* for OPENLOOK of checkbox */
#define MPADDING   10	/* for MOTIF of checkbox*/

	/* this marco is used by *ButtonHandler() (appeared in *[O|M].c)*/
	/* the assumption is that a static varable, last_item_index, is	*/
	/* defined in that routine...					*/
#define CALL_SELECTPROC(w,n,i)					\
	if (last_item_index != i	     &&				\
	    (n == OL_SELECT || n == OL_MENU) &&				\
	    i != OL_NO_ITEM) {						\
		Arg		arg[1];					\
		Widget		popup_menu = NULL;			\
		XtSetArg(arg[0], XtNpopupMenu, (XtArgVal)&popup_menu);	\
		OlFlatGetValues(w, i, arg, 1);				\
		if (popup_menu != (Widget)NULL) {			\
			last_item_index = i;				\
			_OlFBCallCallbacks(w, i, True);			\
		}							\
	}

/*
 ************************************************************************	
 * Define Widget Class Part and Class Rec
 ************************************************************************	
 */

				/* Define new fields for the class part	*/

typedef struct {
   Boolean	allow_class_default;	/* is default item allowed ?	*/
} FlatButtonsClassPart;

				/* Full class record declaration 	*/

typedef struct _FlatButtonsClassRec {
   CoreClassPart		core_class;
   PrimitiveClassPart		primitive_class;
   FlatClassPart		flat_class;
   FlatRowColumnClassPart	row_column_class;
   FlatButtonsClassPart		buttons_class;
} FlatButtonsClassRec;

				/* External class record declaration	*/

extern FlatButtonsClassRec		flatButtonsClassRec;

/*
 ************************************************************************	
 * Define Widget Instance Structure
 ************************************************************************	
 */

				/* Define Expanded sub-object instance	*/

typedef struct {
   XtPointer		client_data;	/* for callbacks		 */
   XtCallbackProc	select_proc;	/* select callback		 */
   XtCallbackProc	unselect_proc;	/* unselect callback		 */
   Widget 		popupMenu; 	/* pull down, pull aside, or NULL*/
   Boolean		set;		/* is this item set ?		 */
   Boolean		is_default;	/* is this item the default ?	 */
   Boolean		is_busy;	/* is this oblong item busy ?	 */
   Dimension		shadow_thickness; /* always OL_SHADOW_OUT type	 */
					  /* the code is in OlgOblongM.c */
} FlatButtonsItemPart;

			/* Item's Full Instance record declaration	*/

typedef struct {
   FlatItemPart			flat;
   FlatRowColumnItemPart	row_column;
   FlatButtonsItemPart		buttons;
} FlatButtonsItemRec, * FlatButtonsItem;

			/* Define new fields for the instance part	*/

typedef struct {
                                                /* Public Resources	*/

   Boolean             exclusive_settings; /* are settings exclusive?    */
   Boolean             none_set;           /* must one sub-object be set?*/
   OlDefine            button_type;        /* OL_RECT_BTN or
                                              OL_OBLONG_BTN or
                                              OL_CHECKBOX                */
   OlDefine            position;           /* OL_LEFT, OL_RIGHT          */

						/* Private Resources	*/

   Boolean             show_default;       /* should default be shown?   */
   Boolean             being_reset;        /* is current being set/unset?*/
   Boolean             inMenu;             /* is container in a menu?    */
   Boolean             menu_descendant;    /* is the shell a menu?       */
   Boolean             allow_instance_default;
                                           /* can instance have default? */
   Boolean             dim;                /* is the setting dimmed?     */
   Boolean             has_default;        /* some sub-object is default */
   Cardinal            current_item;       /* the currect item           */
   Cardinal            set_item;           /* previously selected item   */
   Cardinal            default_item;       /* the default item           */
   Cardinal            preview_item;       /* preview widget sub-object  */
   Widget              preview;            /* widget to preview in       */
   Widget              clone_shell;        /* clone shell widget         */
   Widget              clone;              /* clone widget               */
   XtCallbackList      post_select;        /* call after select/unselect */

   /* MooLIT extension... */
   Boolean	       fill_on_select;	   /* fill with select_color?    */
   Boolean	       menubar_behavior;   /* act like in menubar?       */
   Pixel	       select_color;	   /* color to fill with	 */
   Boolean	       focus_on_select;    /* XtNfocusOnSelect		 */

   Position	       drag_right_x;
   OlDefine	       traversal_type;	   /* private resource, YUK...	 */
} FlatButtonsPart;

				/* Full instance record declaration	*/

typedef struct _FlatButtonsRec {
   CorePart		core;
   PrimitivePart	primitive;
   FlatPart		flat;
   FlatRowColumnPart	row_column;
   FlatButtonsPart	buttons;

   FlatButtonsItemRec	default_item;	/* embedded full item record */
} FlatButtonsRec;

extern Cardinal	_OlFBFindMenuItem OL_ARGS((Widget, Widget));

extern void	_OlFBPostMenu OL_ARGS((Widget, Cardinal, OlFlatDrawInfo *, 
				       FlatButtonsItemPart *));

extern void	_OlFBCallCallbacks OL_ARGS((Widget, Cardinal, Boolean));

extern int	_OlFBIsSet OL_ARGS((Widget, Cardinal));

extern void	_OlFBSelectItem OL_ARGS((Widget));

extern void	_OlFBSetDefault OL_ARGS((Widget, Boolean,
					Position, Position, Cardinal));

extern void	_OlFBSetToCurrent OL_ARGS((Widget, Boolean,
					Position, Position, Cardinal, Boolean));

extern void	_OlFBUnsetCurrent OL_ARGS((Widget));

extern void	_OlFBResetParentSetItem OL_ARGS((Widget, Boolean));

extern void	_OloFBDrawItem OL_ARGS((Widget, FlatItem, OlFlatDrawInfo *));

extern void	_OloFBButtonHandler OL_ARGS((Widget, OlVirtualEvent));

extern Boolean	_OloFBItemLocCursorDims OL_ARGS((Widget,
						 FlatItem, OlFlatDrawInfo *));

extern void	_OlmFBDrawItem OL_ARGS((Widget, FlatItem, OlFlatDrawInfo *));

extern void	_OlmFBButtonHandler OL_ARGS((Widget, OlVirtualEvent));

extern Boolean	_OlmFBItemLocCursorDims OL_ARGS((Widget,
						 FlatItem, OlFlatDrawInfo *));

#endif /* _FButtonsP_h */
