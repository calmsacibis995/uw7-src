#pragma ident	"@(#)m1.2libs:Xm/SelectioBP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmSelectionBoxP_h
#define _XmSelectionBoxP_h

#include <Xm/BulletinBP.h>
#include <Xm/SelectioB.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Defines for use in allocation geometry matrix. */

#define XmSB_MAX_WIDGETS_VERT	8

#define XmUNSPECIFIED		(~0)

/* Constraint part record for SelectionBox widget */
typedef struct _XmSelectionBoxConstraintPart
{
   char unused;
} XmSelectionBoxConstraintPart, * XmSelectionBoxConstraint;

/*  New fields for the SelectionBox widget class record  */

typedef struct
{
        XtCallbackProc  list_callback ;
	XtPointer		extension;      /* Pointer to extension record */
} XmSelectionBoxClassPart;


/* Full class record declaration */

typedef struct _XmSelectionBoxClassRec
{
	CoreClassPart			core_class;
	CompositeClassPart		composite_class;
	ConstraintClassPart		constraint_class;
	XmManagerClassPart		manager_class;
	XmBulletinBoardClassPart	bulletin_board_class;
	XmSelectionBoxClassPart		selection_box_class;
} XmSelectionBoxClassRec;

externalref XmSelectionBoxClassRec xmSelectionBoxClassRec;


/* New fields for the SelectionBox widget record */

typedef struct
{
	Widget		list_label;		/*  list Label  */
	XmString	list_label_string;

	Widget		list;			/*  List  */
	XmString	*list_items;
	int		list_item_count;
	int		list_visible_item_count;
	int		list_selected_item_position;

	Widget		selection_label;	/*  selection Label  */
	XmString	selection_label_string;

	Widget		text;			/*  Text  */
	XmString	text_string;
	short		text_columns;

	Widget		work_area;		/*  other widget  */

	Widget		separator;		/*  separator  */

	Widget		ok_button;		/*  enter button  */
	XmString	ok_label_string;

	Widget		apply_button;		/*  apply button  */
	XmString	apply_label_string;

	XmString	cancel_label_string;	/*  cancel button label  */

	Widget		help_button;		/*  help button  */
	XmString	help_label_string;

	XtCallbackList	ok_callback;		/*  callbacks  */
	XtCallbackList	apply_callback;
	XtCallbackList	cancel_callback;
	XtCallbackList	no_match_callback;

	XtAccelerators	text_accelerators;

	Boolean		must_match;		/*  flags  */
	Boolean		adding_sel_widgets;
	Boolean		minimize_buttons;

	unsigned char	dialog_type;		/*  prompt or selection  */
      unsigned char   child_placement;
} XmSelectionBoxPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _XmSelectionBoxRec
{
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	XmManagerPart		manager;
	XmBulletinBoardPart	bulletin_board;
	XmSelectionBoxPart	selection_box;
} XmSelectionBoxRec;


/*  access macros  */

#define SB_ListLabel(w) (((XmSelectionBoxWidget) (w))->selection_box.list_label)
#define SB_List(w) (((XmSelectionBoxWidget) (w))->selection_box.list)
#define SB_SelectionLabel(w) (((XmSelectionBoxWidget) (w))->selection_box.selection_label)
#define SB_Text(w) (((XmSelectionBoxWidget) (w))->selection_box.text)
#define SB_WorkArea(w) (((XmSelectionBoxWidget) (w))->selection_box.work_area)
#define SB_Separator(w) (((XmSelectionBoxWidget) (w))->selection_box.separator)
#define SB_OkButton(w) (((XmSelectionBoxWidget) (w))->selection_box.ok_button)
#define SB_ApplyButton(w) (((XmSelectionBoxWidget) (w))->selection_box.apply_button)
#define SB_CancelButton(w) (((XmSelectionBoxWidget) (w))->bulletin_board.cancel_button)
#define SB_HelpButton(w) (((XmSelectionBoxWidget) (w))->selection_box.help_button)
#define SB_DefaultButton(w) (((XmSelectionBoxWidget) (w))->bulletin_board.default_button)
#define SB_MarginHeight(w) (((XmSelectionBoxWidget) (w))->bulletin_board.margin_height)
#define SB_MarginWidth(w) (((XmSelectionBoxWidget) (w))->bulletin_board.margin_width)
#define SB_ButtonFontList(w) (((XmSelectionBoxWidget) (w))->bulletin_board.button_font_list)
#define SB_LabelFontList(w) (((XmSelectionBoxWidget) (w))->bulletin_board.label_font_list)
#define SB_TextFontList(w) (((XmSelectionBoxWidget) (w))->bulletin_board.text_font_list)
#define SB_StringDirection( w) \
                       (((XmSelectionBoxWidget)( w))->manager.string_direction)
#define SB_AddingSelWidgets( w) \
                 (((XmSelectionBoxWidget) w)->selection_box.adding_sel_widgets)
#define SB_TextAccelerators( w) \
                  (((XmSelectionBoxWidget) w)->selection_box.text_accelerators)
#define SB_ListItemCount( w) \
                    (((XmSelectionBoxWidget) w)->selection_box.list_item_count)
#define SB_ListSelectedItemPosition( w) (((XmSelectionBoxWidget) w) \
                                   ->selection_box.list_selected_item_position)
#define SB_ListVisibleItemCount( w) (((XmSelectionBoxWidget) w) \
                                       ->selection_box.list_visible_item_count)
#define SB_TextColumns( w) \
                       (((XmSelectionBoxWidget) w)->selection_box.text_columns)
#define SB_MinimizeButtons( w) \
                   (((XmSelectionBoxWidget) w)->selection_box.minimize_buttons)
#define SB_MustMatch( w) (((XmSelectionBoxWidget) w)->selection_box.must_match)


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmSelectionBoxCreateListLabel() ;
extern void _XmSelectionBoxCreateSelectionLabel() ;
extern void _XmSelectionBoxCreateList() ;
extern void _XmSelectionBoxCreateText() ;
extern void _XmSelectionBoxCreateSeparator() ;
extern void _XmSelectionBoxCreateOkButton() ;
extern void _XmSelectionBoxCreateApplyButton() ;
extern void _XmSelectionBoxCreateCancelButton() ;
extern void _XmSelectionBoxCreateHelpButton() ;
extern XmGeoMatrix _XmSelectionBoxGeoMatrixCreate() ;
extern Boolean _XmSelectionBoxNoGeoRequest() ;
extern void _XmSelectionBoxGetSelectionLabelString() ;
extern void _XmSelectionBoxGetListLabelString() ;
extern void _XmSelectionBoxGetTextColumns() ;
extern void _XmSelectionBoxGetTextString() ;
extern void _XmSelectionBoxGetListItems() ;
extern void _XmSelectionBoxGetListItemCount() ;
extern void _XmSelectionBoxGetListVisibleItemCount() ;
extern void _XmSelectionBoxGetOkLabelString() ;
extern void _XmSelectionBoxGetApplyLabelString() ;
extern void _XmSelectionBoxGetCancelLabelString() ;
extern void _XmSelectionBoxGetHelpLabelString() ;
extern void _XmSelectionBoxUpOrDown() ;
extern void _XmSelectionBoxRestore() ;

#else

extern void _XmSelectionBoxCreateListLabel( 
                        XmSelectionBoxWidget sel) ;
extern void _XmSelectionBoxCreateSelectionLabel( 
                        XmSelectionBoxWidget sel) ;
extern void _XmSelectionBoxCreateList( 
                        XmSelectionBoxWidget sel) ;
extern void _XmSelectionBoxCreateText( 
                        XmSelectionBoxWidget sel) ;
extern void _XmSelectionBoxCreateSeparator( 
                        XmSelectionBoxWidget sel) ;
extern void _XmSelectionBoxCreateOkButton( 
                        XmSelectionBoxWidget sel) ;
extern void _XmSelectionBoxCreateApplyButton( 
                        XmSelectionBoxWidget sel) ;
extern void _XmSelectionBoxCreateCancelButton( 
                        XmSelectionBoxWidget sel) ;
extern void _XmSelectionBoxCreateHelpButton( 
                        XmSelectionBoxWidget sel) ;
extern XmGeoMatrix _XmSelectionBoxGeoMatrixCreate( 
                        Widget wid,
                        Widget instigator,
                        XtWidgetGeometry *desired) ;
extern Boolean _XmSelectionBoxNoGeoRequest( 
                        XmGeoMatrix geoSpec) ;
extern void _XmSelectionBoxGetSelectionLabelString( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
extern void _XmSelectionBoxGetListLabelString( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
extern void _XmSelectionBoxGetTextColumns( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
extern void _XmSelectionBoxGetTextString( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
extern void _XmSelectionBoxGetListItems( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
extern void _XmSelectionBoxGetListItemCount( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
extern void _XmSelectionBoxGetListVisibleItemCount( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
extern void _XmSelectionBoxGetOkLabelString( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
extern void _XmSelectionBoxGetApplyLabelString( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
extern void _XmSelectionBoxGetCancelLabelString( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
extern void _XmSelectionBoxGetHelpLabelString( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
extern void _XmSelectionBoxUpOrDown( 
                        Widget wid,
                        XEvent *event,
                        String *argv,
                        Cardinal *argc) ;
extern void _XmSelectionBoxRestore( 
                        Widget wid,
                        XEvent *event,
                        String *argv,
                        Cardinal *argc) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmSelectionBoxP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
