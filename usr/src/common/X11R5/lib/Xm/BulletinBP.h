#pragma ident	"@(#)m1.2libs:Xm/BulletinBP.h	1.2"
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
#ifndef _XmBulletinBoardP_h
#define _XmBulletinBoardP_h

#include <Xm/BulletinB.h>
#include <Xm/ManagerP.h>

#ifdef __cplusplus
extern "C" {
#endif


/****************************************************************************
 * this suffix is added to dialog shells created by Xm convenience routines *
 * so that, for example, a call to create a form dialog named f generates a *
 * dialog shell named f_popup in addition to a form named f                 *
 ****************************************************************************/

#define XmDIALOG_SUFFIX		"_popup"
#define XmDIALOG_SUFFIX_SIZE	6

/* Constraint part record for bulletin board */

typedef struct _XmBulletinBoardConstraintPart
{
   char unused;
} XmBulletinBoardConstraintPart, * XmBulletinBoardConstraint;


/*  New fields for the BulletinBoard widget class record  */

typedef struct
{
	Boolean			always_install_accelerators;
	XmGeoCreateProc         geo_matrix_create ;
	XmFocusMovedProc	focus_moved_proc ;
	XtPointer			extension;
} XmBulletinBoardClassPart;


/* Full class record declaration */

typedef struct _XmBulletinBoardClassRec
{
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	XmManagerClassPart	manager_class;
	XmBulletinBoardClassPart	bulletin_board_class;
} XmBulletinBoardClassRec;

externalref XmBulletinBoardClassRec xmBulletinBoardClassRec;


/* New fields for the BulletinBoard widget record */

typedef struct
{
	Dimension	margin_width;		/*  margins		*/
	Dimension	margin_height;

	Widget		default_button;		/*  widgets		*/
	Widget		dynamic_default_button;	/*  widgets		*/
	Widget		cancel_button;
	Widget		dynamic_cancel_button;

	XtCallbackList	focus_callback;		/*  callback lists	*/
#ifdef BB_HAS_LOSING_FOCUS_CB
	XtCallbackList	losing_focus_callback;	/*  callback lists	*/
#endif
	XtCallbackList	map_callback;
	XtCallbackList	unmap_callback;

	XtTranslations	text_translations;

	XmFontList	button_font_list;	/*  font lists		*/
	XmFontList	label_font_list;
	XmFontList	text_font_list;

	Boolean		allow_overlap;		/*  policies		*/
	Boolean		default_position;
	Boolean		auto_unmanage;
	unsigned char	resize_policy;

	Dimension		old_width;		/*  shadow resources	*/
	Dimension		old_height;
	Dimension		old_shadow_thickness;
	unsigned char	shadow_type;

	Boolean		in_set_values;		/*  internal flag	*/
	Boolean		initial_focus ;

	Boolean		no_resize;		/*  dialog resources	*/
	unsigned char	dialog_style;
	XmString	dialog_title;
	Widget		shell;
	Widget		_UNUSED;

	XmGeoMatrix     geo_cache ;	    /* Cache for geometry management.*/
} XmBulletinBoardPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _XmBulletinBoardRec
{
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	XmManagerPart		manager;
	XmBulletinBoardPart	bulletin_board;
} XmBulletinBoardRec;


#define BB_CancelButton(w) \
                  (((XmBulletinBoardWidget) w)->bulletin_board.cancel_button)
#define BB_DynamicCancelButton(w) \
          (((XmBulletinBoardWidget) w)->bulletin_board.dynamic_cancel_button)
#define BB_DefaultButton(w) \
                 (((XmBulletinBoardWidget) w)->bulletin_board.default_button)
#define BB_DynamicDefaultButton(w) \
         (((XmBulletinBoardWidget) w)->bulletin_board.dynamic_default_button)
#define BB_MarginHeight(w) \
                  (((XmBulletinBoardWidget) w)->bulletin_board.margin_height)
#define BB_MarginWidth(w) \
                   (((XmBulletinBoardWidget) w)->bulletin_board.margin_width)
#define BB_ButtonFontList(w) \
               (((XmBulletinBoardWidget) w)->bulletin_board.button_font_list)
#define BB_LabelFontList(w) \
                (((XmBulletinBoardWidget) w)->bulletin_board.label_font_list)
#define BB_TextFontList(w) \
                 (((XmBulletinBoardWidget) w)->bulletin_board.text_font_list)
#define BB_StringDirection(w) \
                      (((XmBulletinBoardWidget) w)->manager.string_direction)
#define BB_ResizePolicy(w) \
	    	  (((XmBulletinBoardWidget) w)->bulletin_board.resize_policy)
#define BB_InSetValues(w) \
		  (((XmBulletinBoardWidget) w)->bulletin_board.in_set_values)
#define BB_InitialFocus(w) \
		  (((XmBulletinBoardWidget) w)->bulletin_board.initial_focus)


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget _XmBB_CreateButtonG() ;
extern Widget _XmBB_CreateLabelG() ;
extern void _XmBulletinBoardSizeUpdate() ;
extern void _XmBulletinBoardFocusMoved() ;
extern void _XmBulletinBoardReturn() ;
extern void _XmBulletinBoardCancel() ;
extern void _XmBulletinBoardMap() ;
extern void _XmBulletinBoardSetDefaultShadow() ;
extern void _XmBulletinBoardSetDynDefaultButton() ;
extern void _XmBBUpdateDynDefaultButton() ;

#else

extern Widget _XmBB_CreateButtonG( 
                        Widget bb,
                        XmString l_string,
                        char *name) ;
extern Widget _XmBB_CreateLabelG( 
                        Widget bb,
                        XmString l_string,
                        char *name) ;
extern void _XmBulletinBoardSizeUpdate( 
                        Widget wid) ;
extern void _XmBulletinBoardFocusMoved( 
                        Widget wid,
                        XtPointer client_data,
                        XtPointer data) ;
extern void _XmBulletinBoardReturn( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *numParams) ;
extern void _XmBulletinBoardCancel( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *numParams) ;
extern void _XmBulletinBoardMap( 
                         Widget wid,
                         XEvent *event,
                         String *params,
                         Cardinal *numParams) ;
extern void _XmBulletinBoardSetDefaultShadow( 
                        Widget button) ;
extern void _XmBulletinBoardSetDynDefaultButton( 
                        Widget wid,
                        Widget newDefaultButton) ;
extern void _XmBBUpdateDynDefaultButton( 
                         Widget bb) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmBulletinBoardP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
