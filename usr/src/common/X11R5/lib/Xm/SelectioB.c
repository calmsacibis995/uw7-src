#pragma ident	"@(#)m1.2libs:Xm/SelectioB.c	1.3"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*-------------------------------------------------------------------------
**
**	include files
**
**-------------------------------------------------------------------------
*/
#include <Xm/SelectioBP.h>
#include <Xm/GadgetP.h>
#include "XmI.h"
#include "RepTypeI.h"

#ifndef USE_TEXT_IN_DIALOGS
#include <Xm/TextF.h>
#else
#include <Xm/Text.h>
#endif
#include <Xm/RowColumnP.h>
#include <Xm/LabelG.h>
#include <Xm/DialogS.h>
#include <Xm/List.h>
#include <Xm/PushBG.h>
#include <Xm/SeparatoG.h>
#include <Xm/ArrowB.h>
#include "MessagesI.h"
#include <Xm/TransltnsP.h>

#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#ifdef I18N_MSG
#define WARN_DIALOG_TYPE_CHANGE catgets(Xm_catd,MS_SBoX,MSG_SBX_2,\
					_XmMsgSelectioB_0001)
#define WARN_CHILD_TYPE		catgets(Xm_catd,MS_SBoX,MSG_SBX_4,\
					_XmMsgSelectioB_0002)
#else
#define WARN_DIALOG_TYPE_CHANGE _XmMsgSelectioB_0001
#define WARN_CHILD_TYPE		_XmMsgSelectioB_0002
#endif


#define defaultTextAccelerators		_XmSelectioB_defaultTextAccelerators

#define IsButton(w) ( \
      XmIsPushButton(w)   || XmIsPushButtonGadget(w)   || \
      XmIsToggleButton(w) || XmIsToggleButtonGadget(w) || \
      XmIsArrowButton(w)  || XmIsArrowButtonGadget(w)  || \
      XmIsDrawnButton(w))
  
#define IsAutoButton(sb, w) ( \
      w == SB_OkButton(sb)     || \
      w == SB_ApplyButton(sb)  || \
      w == SB_CancelButton(sb) || \
      w == SB_HelpButton(sb))

#define SetupWorkArea(sb) \
    if (_XmGeoSetupKid (boxPtr, SB_WorkArea(sb)))     \
    {                                                 \
      layoutPtr->space_above = vspace;                \
      vspace = BB_MarginHeight(sb);                   \
      boxPtr += 2 ;                                 \
      ++layoutPtr ;                                 \
    }



/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ClassInitialize() ;
static void ClassPartInitialize() ;
static void Initialize() ;
static void InsertChild() ;
static void DeleteChild() ;
static void _XmDialogTypeDefault() ;
static XmImportOperator _XmSetSyntheticResForChild() ;
static void SelectionBoxCallback() ;
static void ListCallback() ;
static void UpdateString() ;
static Boolean SetValues() ;

#else

static void ClassInitialize( void ) ;
static void ClassPartInitialize( 
                        WidgetClass w_class) ;
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void InsertChild( 
                        Widget child) ;
static void DeleteChild( 
                        Widget child) ;
static void _XmDialogTypeDefault( 
                        Widget widget,
                        int offset,
                        XrmValue *value) ;
static XmImportOperator _XmSetSyntheticResForChild( 
                        Widget widget,
                        int offset,
                        XtArgVal *value) ;
static void SelectionBoxCallback( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
static void ListCallback( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
static void UpdateString( 
                        Widget w,
                        XmString string,
#if NeedWidePrototypes
                        int direction) ;
#else
                        XmStringDirection direction) ;
#endif /* NeedWidePrototypes */
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


static XtAccelerators defaultTextAcceleratorsParsed;

/*  Action list  */

static XtActionsRec actionsList[] =
{
    { "UpOrDown",               _XmSelectionBoxUpOrDown }, /* Motif 1.0 */
    { "SelectionBoxUpOrDown",   _XmSelectionBoxUpOrDown },
    { "SelectionBoxRestore",    _XmSelectionBoxRestore },
    };


/*  Resource definitions for SelectionBox
*/

static XmSyntheticResource syn_resources[] = 
{
	{	XmNselectionLabelString, 
		sizeof (XmString), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.selection_label_string), 
		_XmSelectionBoxGetSelectionLabelString,
		_XmSetSyntheticResForChild
	}, 

	{	XmNlistLabelString, 
		sizeof (XmString), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.list_label_string), 
		_XmSelectionBoxGetListLabelString,
		_XmSetSyntheticResForChild
	}, 

	{	XmNtextColumns, 
		sizeof(short), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.text_columns), 
		_XmSelectionBoxGetTextColumns,
		NULL
	}, 

	{	XmNtextString, 
		sizeof (XmString), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.text_string), 
		_XmSelectionBoxGetTextString,
		_XmSetSyntheticResForChild
	}, 

	{	XmNlistItems, 
		sizeof (XmStringTable), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.list_items), 
		_XmSelectionBoxGetListItems,
		_XmSetSyntheticResForChild
	},                                        

	{	XmNlistItemCount, 
		sizeof(int), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.list_item_count), 
		_XmSelectionBoxGetListItemCount,
		_XmSetSyntheticResForChild
	}, 
 
	{	XmNlistVisibleItemCount, 
		sizeof(int), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.list_visible_item_count), 
		_XmSelectionBoxGetListVisibleItemCount,
		_XmSetSyntheticResForChild
	}, 

	{	XmNokLabelString, 
		sizeof (XmString), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.ok_label_string), 
		_XmSelectionBoxGetOkLabelString,
		NULL
	}, 

	{	XmNapplyLabelString, 
		sizeof (XmString), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.apply_label_string), 
		_XmSelectionBoxGetApplyLabelString,
		NULL
	}, 

	{	XmNcancelLabelString, 
		sizeof (XmString), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.cancel_label_string), 
		_XmSelectionBoxGetCancelLabelString,
		NULL
	}, 

	{	XmNhelpLabelString, 
		sizeof (XmString), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.help_label_string), 
		_XmSelectionBoxGetHelpLabelString,
		NULL
	}, 
};



static XtResource resources[] = 
{
	{	XmNtextAccelerators, 
		XmCAccelerators, XmRAcceleratorTable, sizeof (XtAccelerators), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.text_accelerators), 
		XmRImmediate, NULL		
	}, 

	{	XmNselectionLabelString, 
		XmCSelectionLabelString, XmRXmString, sizeof (XmString), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.selection_label_string), 
		XmRString, NULL
	}, 

	{	XmNlistLabelString, 
		XmCListLabelString, XmRXmString, sizeof (XmString), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.list_label_string), 
		XmRString, NULL
	}, 

	{	XmNtextColumns, 
		XmCColumns, XmRShort, sizeof(short), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.text_columns), 
		XmRImmediate, (XtPointer) 20
	}, 

	{	XmNtextString, 
		XmCTextString, XmRXmString, sizeof (XmString), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.text_string), 
		XmRImmediate, (XtPointer) XmUNSPECIFIED
	}, 

	{	XmNlistItems, 
		XmCItems, XmRXmStringTable, sizeof (XmStringTable), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.list_items), 
		XmRImmediate, NULL
	},                                        

	{	XmNlistItemCount, 
		XmCItemCount, XmRInt, sizeof(int), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.list_item_count), 
		XmRImmediate, (XtPointer) XmUNSPECIFIED
	}, 
 
	{	XmNlistVisibleItemCount, 
		XmCVisibleItemCount, XmRInt, sizeof(int), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.list_visible_item_count), 
		XmRImmediate, (XtPointer) 8
	}, 

	{	XmNokLabelString, 
		XmCOkLabelString, XmRXmString, sizeof (XmString), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.ok_label_string), 
		XmRString, NULL
	}, 

	{	XmNapplyLabelString, 
		XmCApplyLabelString, XmRXmString, sizeof (XmString), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.apply_label_string), 
		XmRString, NULL
	}, 

	{	XmNcancelLabelString, 
		XmCCancelLabelString, XmRXmString, sizeof (XmString), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.cancel_label_string), 
		XmRString, NULL
	}, 

	{	XmNhelpLabelString, 
		XmCHelpLabelString, XmRXmString, sizeof (XmString), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.help_label_string), 
		XmRString, NULL
	}, 

	{	XmNnoMatchCallback, 
		XmCCallback, XmRCallback, sizeof (XtCallbackList), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.no_match_callback), 
		XmRImmediate, (XtPointer) NULL
	}, 

	{	XmNmustMatch, 
		XmCMustMatch, XmRBoolean, sizeof(Boolean), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.must_match), 
		XmRImmediate, (XtPointer) False
	}, 
                     
	{	XmNminimizeButtons, 
		XmCMinimizeButtons, XmRBoolean, sizeof(Boolean), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.minimize_buttons), 
		XmRImmediate, (XtPointer) False
	}, 
                     
	{	XmNokCallback, 
		XmCCallback, XmRCallback, sizeof (XtCallbackList), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.ok_callback), 
		XmRImmediate, (XtPointer) NULL
	}, 

	{	XmNapplyCallback, 
		XmCCallback, XmRCallback, sizeof (XtCallbackList), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.apply_callback), 
		XmRImmediate, (XtPointer) NULL
	}, 

	{	XmNcancelCallback, 
		XmCCallback, XmRCallback, sizeof (XtCallbackList), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.cancel_callback), 
		XmRImmediate, (XtPointer) NULL
	}, 

	{	XmNdialogType, 
		XmCDialogType, XmRSelectionType, sizeof (unsigned char), 
		XtOffsetOf( struct _XmSelectionBoxRec, selection_box.dialog_type), 
		XmRCallProc, (XtPointer) _XmDialogTypeDefault
        },
 
        {       XmNchildPlacement, 
                XmCChildPlacement, XmRChildPlacement, sizeof (unsigned char), 
                XtOffsetOf( struct _XmSelectionBoxRec, selection_box.child_placement), 
                XmRImmediate, (XtPointer) XmPLACE_ABOVE_SELECTION
	}, 
};



externaldef( xmselectionboxclassrec) XmSelectionBoxClassRec
                                                       xmSelectionBoxClassRec =
{
    {
    	/* superclass	      */	(WidgetClass) &xmBulletinBoardClassRec, 
    	/* class_name	      */	"XmSelectionBox", 
    	/* widget_size	      */	sizeof(XmSelectionBoxRec), 
    	/* class_initialize   */    	ClassInitialize, 
    	/* chained class init */	ClassPartInitialize, 
    	/* class_inited       */	FALSE, 
    	/* initialize	      */	Initialize, 
    	/* initialize hook    */	NULL, 
    	/* realize	      */	XtInheritRealize, 
    	/* actions	      */	actionsList, 
    	/* num_actions	      */	XtNumber(actionsList), 
    	/* resources	      */	resources, 
    	/* num_resources      */	XtNumber(resources), 
    	/* xrm_class	      */	NULLQUARK, 
    	/* compress_motion    */	TRUE, 
    	/* compress_exposure  */	XtExposeCompressMaximal,
    	/* compress enter/exit*/	TRUE, 
    	/* visible_interest   */	FALSE, 
    	/* destroy	      */	NULL, 
    	/* resize	      */	XtInheritResize,
    	/* expose	      */	XtInheritExpose, 
    	/* set_values	      */	SetValues, 
	/* set_values_hook    */	NULL,                    
	/* set_values_almost  */	XtInheritSetValuesAlmost,
	/* get_values_hook    */	NULL, 
    	/* accept_focus	      */	NULL, 
	/* version	      */	XtVersion, 
        /* callback_offsets   */        NULL, 
        /* tm_table           */        XtInheritTranslations, 
    	/* query_geometry     */	XtInheritGeometryManager,
    	/* display_accelerator*/	NULL, 
	/* extension	      */	NULL, 
    }, 

    {	/* composite class record */    

	/* childrens geo mgr proc   */  XtInheritGeometryManager,
	/* set changed proc	    */  XtInheritChangeManaged,
	/* insert_child		    */	InsertChild, 
	/* delete_child 	    */	DeleteChild, 
	/* extension		    */	NULL, 
    }, 

    {	/* constraint class record */

	/* no additional resources  */	NULL, 
	/* num additional resources */	0, 
	/* size of constraint rec   */	0, 
	/* constraint_initialize    */	NULL, 
	/* constraint_destroy	    */	NULL, 
	/* constraint_setvalue	    */	NULL, 
	/* extension		    */	NULL, 
    }, 

    {	/* manager class record */
      XmInheritTranslations, 		        /* default translations   */
      syn_resources, 				/* syn_resources      	  */
      XtNumber (syn_resources), 		/* num_syn_resources 	  */
      NULL, 					/* syn_cont_resources     */
      0, 					/* num_syn_cont_resources */
      XmInheritParentProcess,                   /* parent_process         */
      NULL, 					/* extension		  */
    }, 

    {	/* bulletin board class record */     
        TRUE,                                   /*always_install_accelerators*/
        _XmSelectionBoxGeoMatrixCreate,         /* geo_matrix_create */
        XmInheritFocusMovedProc,                /* focus_moved_proc */
	NULL, 					/* extension */
    }, 	

    {	/* selection box class record */
        ListCallback,                           /* list_callback */
	NULL, 					/* extension  */
    }, 
};

externaldef( xmselectionboxwidgetclass) WidgetClass xmSelectionBoxWidgetClass
                                      = (WidgetClass) &xmSelectionBoxClassRec ;


/****************************************************************/
static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{   
/****************/

    /* parse the default translation and accelerator tables
    */
    defaultTextAcceleratorsParsed =
                            XtParseAcceleratorTable( defaultTextAccelerators) ;
    return ;
    }
/****************************************************************/
static void 
#ifdef _NO_PROTO
ClassPartInitialize( w_class )
        WidgetClass w_class ;
#else
ClassPartInitialize(
        WidgetClass w_class )
#endif /* _NO_PROTO */
{   
/****************/
    XmSelectionBoxWidgetClass wc = (XmSelectionBoxWidgetClass) w_class;
    XmSelectionBoxWidgetClass super =
        (XmSelectionBoxWidgetClass) wc->core_class.superclass;

    if (wc->selection_box_class.list_callback == XmInheritCallbackProc)
        wc->selection_box_class.list_callback =
            super->selection_box_class.list_callback;

    _XmFastSubclassInit(w_class, XmSELECTION_BOX_BIT) ;

    return ;
}



/****************************************************************
 * Create a SelectionBox instance.
 ****************/
static void 
#ifdef _NO_PROTO
Initialize( rw, nw, args, num_args )
        Widget rw ;
        Widget nw ;
        ArgList args ;
        Cardinal *num_args ;
#else
Initialize(
        Widget rw,
        Widget nw,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmSelectionBoxWidget new_w = (XmSelectionBoxWidget) nw ;
/****************/

    new_w->selection_box.work_area = NULL;

    if ( new_w->selection_box.text_accelerators == NULL ) {
	new_w->selection_box.text_accelerators = 
	    defaultTextAcceleratorsParsed;
    }

    /*	Validate dialog type.
     */
    if(    !XmRepTypeValidValue( XmRID_SELECTION_TYPE,
				new_w->selection_box.dialog_type, 
				(Widget) new_w)    )
        {   
            new_w->selection_box.dialog_type = 
		XmIsDialogShell( XtParent( new_w))
		    ? XmDIALOG_SELECTION : XmDIALOG_WORK_AREA ;
	} 
    /*      Validate child placement.
     */
    if(    !XmRepTypeValidValue( XmRID_CHILD_PLACEMENT,
				new_w->selection_box.child_placement, 
				(Widget) new_w)    )
        {   
	    new_w->selection_box.child_placement = XmPLACE_ABOVE_SELECTION;
	}

    
    /*	Create child widgets.
	Here we have now to take care of XmUNSPECIFIED (CR 4856).
	*/
    new_w->selection_box.adding_sel_widgets = True;
    
    if ( (new_w->selection_box.dialog_type != XmDIALOG_PROMPT)
	&& (new_w->selection_box.dialog_type != XmDIALOG_COMMAND) )
	{   
            if (new_w->selection_box.list_label_string == 
		(XmString) XmUNSPECIFIED) {
		new_w->selection_box.list_label_string = NULL ;
		_XmSelectionBoxCreateListLabel( new_w) ;
		new_w->selection_box.list_label_string = 
		    (XmString) XmUNSPECIFIED ;
	    } else
		_XmSelectionBoxCreateListLabel( new_w) ;

	}
    else
        {   SB_ListLabel (new_w) = NULL;
	} 
    
    if (new_w->selection_box.list_label_string != 
	(XmString) XmUNSPECIFIED)
	new_w->selection_box.list_label_string = NULL ;
    
    
    if (new_w->selection_box.dialog_type != XmDIALOG_PROMPT)
	{   
            _XmSelectionBoxCreateList( new_w) ;
	}
    else
        {   SB_List (new_w) = NULL;
	} 
    new_w->selection_box.list_items = NULL ;
    new_w->selection_box.list_item_count = XmUNSPECIFIED ;
    
    
    if (new_w->selection_box.selection_label_string == 
	(XmString) XmUNSPECIFIED) {
	new_w->selection_box.selection_label_string = NULL ;
	_XmSelectionBoxCreateSelectionLabel(new_w);
	new_w->selection_box.selection_label_string = 
	    (XmString) XmUNSPECIFIED ;
    } else {
	_XmSelectionBoxCreateSelectionLabel(new_w);
	new_w->selection_box.selection_label_string = NULL ;
    }
    
    
    _XmSelectionBoxCreateText(new_w);
    
    /* Do not reset text_string to XmUNSPECIFIED until after calls 
     *   to CreateList and CreateText.
     */
    new_w->selection_box.text_string = (XmString) XmUNSPECIFIED ;
    
    if(    new_w->manager.initial_focus == NULL    )
        {   
            new_w->manager.initial_focus = SB_Text( new_w) ;
	} 
    
    if (new_w->selection_box.dialog_type != XmDIALOG_COMMAND)
	{
	    _XmSelectionBoxCreateSeparator (new_w);
	    _XmSelectionBoxCreateOkButton (new_w);
	    if (new_w->selection_box.apply_label_string ==
		(XmString) XmUNSPECIFIED) {
		new_w->selection_box.apply_label_string = NULL ;
		_XmSelectionBoxCreateApplyButton (new_w);
		new_w->selection_box.apply_label_string = 
		    (XmString) XmUNSPECIFIED ;
	    } else
		_XmSelectionBoxCreateApplyButton (new_w);
	    _XmSelectionBoxCreateCancelButton (new_w);
	    _XmSelectionBoxCreateHelpButton (new_w);
	    
	    BB_DefaultButton( new_w) = SB_OkButton( new_w) ;
	    _XmBulletinBoardSetDynDefaultButton( (Widget) new_w,
						BB_DefaultButton( new_w)) ;
	}
    else
	{
	    SB_Separator (new_w) = NULL;
	    SB_OkButton (new_w) = NULL;
	    SB_ApplyButton (new_w) = NULL;
	    SB_CancelButton (new_w) = NULL;
	    SB_HelpButton (new_w) = NULL;
	}
    new_w->selection_box.ok_label_string = NULL ;
    if (new_w->selection_box.apply_label_string !=
	(XmString) XmUNSPECIFIED) 
	new_w->selection_box.apply_label_string = NULL ;
    new_w->selection_box.cancel_label_string = NULL ;
    new_w->selection_box.help_label_string = NULL ;
    
    new_w->selection_box.adding_sel_widgets = False;
    

    XtManageChildren (new_w->composite.children, 
		      new_w->composite.num_children) ;

    if (new_w->selection_box.dialog_type == XmDIALOG_PROMPT ||
	new_w->selection_box.dialog_type == XmDIALOG_WORK_AREA)
	{
	    XtUnmanageChild (SB_ApplyButton (new_w));
	}

}

/****************************************************************
 * Selection widget supports ONE child.  This routine
 *   handles adding a child to selection widget
 ****************/
static void 
#ifdef _NO_PROTO
InsertChild( child )
        Widget child ;
#else
InsertChild(
        Widget child )
#endif /* _NO_PROTO */
{   
            XmSelectionBoxWidget sb ;
/****************/

    /* Use the dialog class insert proc to do all the dirty work
    */
    (*((XmBulletinBoardWidgetClass) xmBulletinBoardWidgetClass)
                                      ->composite_class.insert_child) (child) ;
    if(    !XtIsRectObj( child)    )
    {   return ;
        } 
    sb = (XmSelectionBoxWidget) XtParent( child) ;

    /* check if this child is to be the selection widget's work area widget
    */
    if(    !sb->selection_box.adding_sel_widgets
        && !XtIsShell( child)    )
    {   
        if(    !sb->selection_box.work_area    )
        {   sb->selection_box.work_area = child ;
            } 
        } 
    return ;
    }

/****************************************************************
 * Remove child from selection widget
 ****************/
static void 
#ifdef _NO_PROTO
DeleteChild( child )
        Widget child ;
#else
DeleteChild(
        Widget child )
#endif /* _NO_PROTO */
{   
            XmSelectionBoxWidget sel ;
/****************/

    if(    XtIsRectObj( child)    )
    {   
        sel = (XmSelectionBoxWidget) XtParent( child) ;
        /*	Clear widget fields (BulletinBoard does default and cancel).
        */
        if(    child == SB_ListLabel (sel)    )
        {   SB_ListLabel( sel) = NULL ;
            } 
        else
        {   if(    SB_List( sel)  &&  (child == XtParent( SB_List (sel)))    )
            {   SB_List( sel) = NULL ;
                } 
            else
            {   if(    child == SB_SelectionLabel (sel)    )
                {   SB_SelectionLabel( sel) = NULL ;
                    } 
                else
                {   if(    child == SB_Text (sel)    )
                    {   SB_Text( sel) = NULL ;
                        } 
                    else
                    {   if(    child == SB_WorkArea (sel)    )
                        {   SB_WorkArea( sel) = NULL ;
                            } 
                        else
                        {   if(    child == SB_Separator (sel)    )
                            {   SB_Separator( sel) = NULL ;
                                } 
                            else
                            {   if(    child == SB_OkButton (sel)    )
                                {   SB_OkButton( sel) = NULL ;
                                    } 
                                else
                                {   if(    child == SB_ApplyButton (sel)    )
                                    {   SB_ApplyButton( sel) = NULL ;
                                        } 
                                    else
                                    {   if(    child == SB_HelpButton (sel)   )
                                        {   SB_HelpButton( sel) = NULL ;
                                            } 
                                        }
                                    } 
                                } 
                            } 
                        } 
                    } 
                } 
            }
        } 
    (*((XmBulletinBoardWidgetClass) xmBulletinBoardWidgetClass)
                                      ->composite_class.delete_child)( child) ;
    return ;
    }

/****************************************************************
 * Set the default type (selection or workarea) based on parent.
 ****************/
static void 
#ifdef _NO_PROTO
_XmDialogTypeDefault( widget, offset, value )
        Widget widget ;
        int offset ;
        XrmValue *value ;
#else
_XmDialogTypeDefault(
        Widget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
    static unsigned char	type;
/****************/

    /*
     * Set the default type.  To do this, we check the dialog
     * box's parent.  If it is a DialogShell widget, then this
     * is a "pop-up" dialog box, and the default type is selection.
     * Else the default type is workarea.
     */
    type = XmDIALOG_WORK_AREA;
    if (XmIsDialogShell (XtParent (widget)))
	type = XmDIALOG_SELECTION;
    value->addr = (XPointer)(&type);
    return ;
}

static XmImportOperator
#ifdef _NO_PROTO
_XmSetSyntheticResForChild( widget, offset, value )
        Widget widget;
        int offset;
        XtArgVal * value;
#else /* _NO_PROTO */
_XmSetSyntheticResForChild(
        Widget widget,
        int offset,
        XtArgVal *value)
#endif /* _NO_PROTO */
{ 
    return XmSYNTHETIC_LOAD;
    }

/****************************************************************
 * Create the Label displayed above the List widget.
 ****************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxCreateListLabel( sel )
        XmSelectionBoxWidget sel ;
#else
_XmSelectionBoxCreateListLabel(
        XmSelectionBoxWidget sel )
#endif /* _NO_PROTO */
{
/****************/

#ifdef I18N_MSG
    Boolean created_list_label = False;

    if ((sel->selection_box.list_label_string == NULL) ||
	(sel->selection_box.list_label_string == XmUNSPECIFIED))
    {
  	if (sel->selection_box.dialog_type == XmDIALOG_FILE_SELECTION) {
	    sel->selection_box.list_label_string = XmStringCreateLocalized(
			   catgets(Xm_catd,MS_RESOURCES,MSG_Res_7,"Files"));
	}
	else {
	    sel->selection_box.list_label_string = XmStringCreateLocalized(
	                    catgets(Xm_catd,MS_RESOURCES,MSG_Res_9,"Items"));
	}
        created_list_label = True;
    }
#endif

    SB_ListLabel( sel) = _XmBB_CreateLabelG( (Widget) sel,
                               sel->selection_box.list_label_string, "Items") ;

#ifdef I18N_MSG
    if (created_list_label == True) {
	XmStringFree(sel->selection_box.list_label_string);
	sel->selection_box.list_label_string = NULL;
    }
#endif

    return ;
    }
/****************************************************************
 * Create the Label displayed above the Text widget.
 ****************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxCreateSelectionLabel( sel )
        XmSelectionBoxWidget sel ;
#else
_XmSelectionBoxCreateSelectionLabel(
        XmSelectionBoxWidget sel )
#endif /* _NO_PROTO */
{
/****************/


#ifdef I18N_MSG
    Boolean created_selection_label = False;

    if ((sel->selection_box.selection_label_string == NULL) ||
	(sel->selection_box.selection_label_string == XmUNSPECIFIED))
    {
	sel->selection_box.selection_label_string = XmStringCreateLocalized(
	    catgets(Xm_catd,MS_RESOURCES,MSG_Res_3,"Selection"));
	created_selection_label = True;
    }
#endif

    SB_SelectionLabel( sel) = _XmBB_CreateLabelG( (Widget) sel,
                      sel->selection_box.selection_label_string, "Selection") ;

#ifdef I18N_MSG
    if (created_selection_label == True) {
	XmStringFree(sel->selection_box.selection_label_string);
	sel->selection_box.selection_label_string = NULL;
    }
#endif

    return ;
    }


/****************************************************************
 * Create the List widget.
 ****************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxCreateList( sel )
        XmSelectionBoxWidget sel ;
#else
_XmSelectionBoxCreateList(
        XmSelectionBoxWidget sel )
#endif /* _NO_PROTO */
{   
            Arg		al[20] ;
    register int	ac = 0 ;
            int *       position ;
            int         pos_count ;
            XtCallbackProc callbackProc ;
/****************/

    if(    sel->selection_box.list_items    )
    {   
        XtSetArg( al[ac], XmNitems, sel->selection_box.list_items) ; ac++ ;
        }
    if(    sel->selection_box.list_item_count != XmUNSPECIFIED    )
    {   
        XtSetArg( al[ac], XmNitemCount, 
                                  sel->selection_box.list_item_count) ;  ac++ ;
        }
    XtSetArg( al[ac], XmNvisibleItemCount, 
		sel->selection_box.list_visible_item_count) ;  ac++ ;

    sel->selection_box.list_selected_item_position = 0 ;

    XtSetArg( al[ac], XmNstringDirection, SB_StringDirection (sel)) ;  ac++ ;
    XtSetArg( al[ac], XmNselectionPolicy, XmBROWSE_SELECT) ;  ac++ ;
    XtSetArg( al[ac], XmNlistSizePolicy, XmCONSTANT) ;  ac++ ;
    XtSetArg( al[ac], XmNscrollBarDisplayPolicy, XmAS_NEEDED) ;  ac++ ;
    XtSetArg( al[ac], XmNnavigationType, XmSTICKY_TAB_GROUP) ; ++ac ;

    SB_List( sel) = XmCreateScrolledList( (Widget) sel, "ItemsList", al, ac) ;

    if(    sel->selection_box.text_string != (XmString) XmUNSPECIFIED    )
    {   
        if(    sel->selection_box.text_string
            && XmListGetMatchPos( SB_List( sel), 
                    sel->selection_box.text_string, &position, &pos_count)    )
        {   if(    pos_count    )
            {   
                sel->selection_box.list_selected_item_position = position[0] ;
                XmListSelectPos( SB_List( sel), position[0], FALSE) ;
                } 
            XtFree( (char *) position) ;
            } 
        }
    callbackProc = ((XmSelectionBoxWidgetClass) sel->core.widget_class)
                                          ->selection_box_class.list_callback ;
    if(    callbackProc    )
    {   
        XtAddCallback( SB_List( sel), XmNsingleSelectionCallback,
                                               callbackProc, (XtPointer) sel) ;
        XtAddCallback( SB_List( sel), XmNbrowseSelectionCallback,
                                               callbackProc, (XtPointer) sel) ;
        XtAddCallback( SB_List( sel), XmNdefaultActionCallback,
                                               callbackProc, (XtPointer) sel) ;
        } 
    XtManageChild( SB_List( sel)) ;

    return ;
    }

/****************************************************************
 * Create the Text widget.
 ****************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxCreateText( sel )
        XmSelectionBoxWidget sel ;
#else
_XmSelectionBoxCreateText(
        XmSelectionBoxWidget sel )
#endif /* _NO_PROTO */
{
	Arg		al[10];
	register int	ac = 0;
	String		text_value ;
	XtAccelerators	temp_accelerators ;
/****************/

	XtSetArg (al[ac], XmNcolumns, sel->selection_box.text_columns);  ac++;
	XtSetArg (al[ac], XmNresizeWidth, False);  ac++;
	XtSetArg (al[ac], XmNeditMode, XmSINGLE_LINE_EDIT);  ac++;
        XtSetArg( al[ac], XmNnavigationType, XmSTICKY_TAB_GROUP) ; ++ac ;

#ifndef USE_TEXT_IN_DIALOGS
	SB_Text( sel) = XmCreateTextField( (Widget) sel, "Text", al, ac);
#else
	SB_Text( sel) = XmCreateText( (Widget) sel, "Text", al, ac);
#endif
	if(    (sel->selection_box.text_string != (XmString) XmUNSPECIFIED)    )
        {   
            text_value = _XmStringGetTextConcat(
                                              sel->selection_box.text_string) ;
#ifndef USE_TEXT_IN_DIALOGS
            XmTextFieldSetString( SB_Text (sel), text_value) ;
            if(    text_value    )
            {   XmTextFieldSetCursorPosition( SB_Text( sel),
			          XmTextFieldGetLastPosition( SB_Text( sel))) ;
                } 
#else
            XmTextSetString( SB_Text (sel), text_value) ;
            if(    text_value    )
            {   XmTextSetCursorPosition( SB_Text (sel),
			          XmTextGetLastPosition( SB_Text( sel))) ;
                } 
#endif
            XtFree( (char *) text_value) ;
            } 

	/*	Install text accelerators.
	*/
        temp_accelerators = sel->core.accelerators;
	sel->core.accelerators = sel->selection_box.text_accelerators;
	XtInstallAccelerators( SB_Text( sel), (Widget) sel) ;
	sel->core.accelerators = temp_accelerators;
        return ;
}

/****************************************************************
 * Create the Separator displayed above the buttons.
 ****************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxCreateSeparator( sel )
        XmSelectionBoxWidget sel ;
#else
_XmSelectionBoxCreateSeparator(
        XmSelectionBoxWidget sel )
#endif /* _NO_PROTO */
{
	Arg		al[10];
	register int	ac = 0;
/****************/

	XtSetArg (al[ac], XmNhighlightThickness, 0);  ac++;
	SB_Separator (sel) =
		XmCreateSeparatorGadget( (Widget) sel, "Separator", al, ac);
        return ;
}
/****************************************************************
 * Create the "OK" PushButton.
 ****************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxCreateOkButton( sel )
        XmSelectionBoxWidget sel ;
#else
_XmSelectionBoxCreateOkButton(
        XmSelectionBoxWidget sel )
#endif /* _NO_PROTO */
{
/****************/


#ifdef I18N_MSG
    Boolean created_ok_label = False;

    if ((sel->selection_box.ok_label_string == NULL) ||
	(sel->selection_box.ok_label_string == XmUNSPECIFIED))
    {
	sel->selection_box.ok_label_string = XmStringCreateLocalized(
	    catgets(Xm_catd,MS_RESOURCES,MSG_Res_1,"OK"));
	created_ok_label = True;
    }
#endif

    SB_OkButton( sel) = _XmBB_CreateButtonG( (Widget) sel, 
                                    sel->selection_box.ok_label_string, "OK") ;

#ifdef I18N_MSG
    if (created_ok_label == True) {
	XmStringFree(sel->selection_box.ok_label_string);
	sel->selection_box.ok_label_string = NULL;
    }
#endif

    XtAddCallback (SB_OkButton (sel), XmNactivateCallback, 
                        SelectionBoxCallback, (XtPointer) XmDIALOG_OK_BUTTON) ;
    return ;
    }

/****************************************************************
 * Create the "Apply" PushButton.
 ****************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxCreateApplyButton( sel )
        XmSelectionBoxWidget sel ;
#else
_XmSelectionBoxCreateApplyButton(
        XmSelectionBoxWidget sel )
#endif /* _NO_PROTO */
{
/****************/


#ifdef I18N_MSG
    Boolean created_apply_label = False;

    if ((sel->selection_box.apply_label_string == NULL) ||
	(sel->selection_box.apply_label_string == XmUNSPECIFIED))
    {
	if (sel->selection_box.dialog_type == XmDIALOG_FILE_SELECTION) {
	    sel->selection_box.apply_label_string = XmStringCreateLocalized(
	                    catgets(Xm_catd,MS_RESOURCES,MSG_Res_10,"Filter"));
	}
	else {
	    sel->selection_box.apply_label_string = XmStringCreateLocalized(
	                    catgets(Xm_catd,MS_RESOURCES,MSG_Res_4,"Apply"));
	}
	created_apply_label = True;
    }
#endif

    SB_ApplyButton( sel) = _XmBB_CreateButtonG( (Widget) sel, 
                              sel->selection_box.apply_label_string, "Apply") ;

#ifdef I18N_MSG
    if (created_apply_label == True) {
	XmStringFree(sel->selection_box.apply_label_string);
	sel->selection_box.apply_label_string = NULL;
    }
#endif

    /* Remove BulletinBoard Unmanage callback from apply and help buttons.
    */
    XtRemoveAllCallbacks( SB_ApplyButton( sel), XmNactivateCallback) ;
    XtAddCallback (SB_ApplyButton (sel), XmNactivateCallback, 
                     SelectionBoxCallback, (XtPointer) XmDIALOG_APPLY_BUTTON) ;
    return ;
    }
/****************************************************************
 * Create the "Cancel" PushButton.
 ****************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxCreateCancelButton( sel )
        XmSelectionBoxWidget sel ;
#else
_XmSelectionBoxCreateCancelButton(
        XmSelectionBoxWidget sel )
#endif /* _NO_PROTO */
{
/****************/


#ifdef I18N_MSG
    Boolean created_cancel_label = False;

    if ((sel->selection_box.cancel_label_string == NULL) ||
	(sel->selection_box.cancel_label_string == XmUNSPECIFIED))
    {
	sel->selection_box.cancel_label_string = XmStringCreateLocalized(
	    catgets(Xm_catd,MS_RESOURCES,MSG_Res_2,"Cancel"));
	created_cancel_label = True;
    }
#endif

    SB_CancelButton( sel) = _XmBB_CreateButtonG( (Widget) sel, 
                            sel->selection_box.cancel_label_string, "Cancel") ;

#ifdef I18N_MSG
    if (created_cancel_label == True) {
	XmStringFree(sel->selection_box.cancel_label_string);
	sel->selection_box.cancel_label_string = NULL;
    }
#endif

    XtAddCallback( SB_CancelButton( sel), XmNactivateCallback, 
                    SelectionBoxCallback, (XtPointer) XmDIALOG_CANCEL_BUTTON) ;
    return ;
    }
/****************************************************************
 * Create the "Help" PushButton.
 ****************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxCreateHelpButton( sel )
        XmSelectionBoxWidget sel ;
#else
_XmSelectionBoxCreateHelpButton(
        XmSelectionBoxWidget sel )
#endif /* _NO_PROTO */
{
/****************/


#ifdef I18N_MSG
    Boolean created_help_label = False;

    if ((sel->selection_box.help_label_string == NULL) ||
	(sel->selection_box.help_label_string == XmUNSPECIFIED))
    {
	sel->selection_box.help_label_string = XmStringCreateLocalized(
		catgets(Xm_catd,MS_RESOURCES,MSG_Res_5,"Help"));
	created_help_label = True;
    }
#endif

    SB_HelpButton( sel) = _XmBB_CreateButtonG( (Widget) sel, 
                                sel->selection_box.help_label_string, "Help") ;

#ifdef I18N_MSG
    if (created_help_label == True) {
	XmStringFree(sel->selection_box.help_label_string);
	sel->selection_box.help_label_string = NULL;
    }
#endif

    /* Remove BulletinBoard Unmanage callback from apply and help buttons.
    */
    XtRemoveAllCallbacks( SB_HelpButton( sel), XmNactivateCallback) ;
    XtAddCallback (SB_HelpButton (sel), XmNactivateCallback, 
                      SelectionBoxCallback, (XtPointer) XmDIALOG_HELP_BUTTON) ;
    return ;
    }
   
/****************************************************************/

XmGeoMatrix 
#ifdef _NO_PROTO
_XmSelectionBoxGeoMatrixCreate( wid, instigator, desired )
        Widget wid ;
        Widget instigator ;
        XtWidgetGeometry *desired ;
#else
_XmSelectionBoxGeoMatrixCreate(
        Widget wid,
        Widget instigator,
        XtWidgetGeometry *desired )
#endif /* _NO_PROTO */
{
    XmSelectionBoxWidget sb = (XmSelectionBoxWidget) wid ;
    XmGeoMatrix     geoSpec ;
    register XmGeoRowLayout  layoutPtr ;
    register XmKidGeometry   boxPtr ;
    XmKidGeometry   firstButtonBox ;
    XmListWidget    list ;
    Boolean         listLabelBox ;
    Boolean         selLabelBox ;
    Dimension       vspace = BB_MarginHeight(sb);
    int             i;

/*
 * Layout SelectionBox XmGeoMatrix.
 * Each row is terminated by leaving an empty XmKidGeometry and
 * moving to the next XmGeoRowLayout.
 */

    geoSpec = _XmGeoMatrixAlloc( XmSB_MAX_WIDGETS_VERT,
                              sb->composite.num_children, 0) ;
    geoSpec->composite = (Widget) sb ;
    geoSpec->instigator = (Widget) instigator ;
    if(    desired    )
    {   geoSpec->instig_request = *desired ;
        } 
    geoSpec->margin_w = BB_MarginWidth( sb) + sb->manager.shadow_thickness ;
    geoSpec->margin_h = BB_MarginHeight( sb) + sb->manager.shadow_thickness ;
    geoSpec->no_geo_request = _XmSelectionBoxNoGeoRequest ;

    layoutPtr = &(geoSpec->layouts->row) ;
    boxPtr = geoSpec->boxes ;

    /* menu bar */
 
    for (i = 0; i < sb->composite.num_children; i++)
    {	Widget w = sb->composite.children[i];

        if(    XmIsRowColumn(w)
            && ((XmRowColumnWidget)w)->row_column.type == XmMENU_BAR
            && w != SB_WorkArea(sb)
            && _XmGeoSetupKid( boxPtr, w)    )
	{   layoutPtr->fix_up = _XmMenuBarFix ;
            boxPtr += 2;
            ++layoutPtr;
            vspace = 0;		/* fixup space_above of next row. */
            break;
            }
        }

    /* work area, XmPLACE_TOP */

    if (sb->selection_box.child_placement == XmPLACE_TOP)
      SetupWorkArea(sb);

    /* list box label */

    listLabelBox = FALSE ;
    if(    _XmGeoSetupKid( boxPtr, SB_ListLabel( sb))    )
    {   
        listLabelBox = TRUE ;
        layoutPtr->space_above = vspace;
        vspace = BB_MarginHeight(sb);
        boxPtr += 2 ;
        ++layoutPtr ;
        } 

    /* list box */

    list = (XmListWidget) SB_List( sb) ;
    if(    list  &&  XtIsManaged( list)
        && _XmGeoSetupKid( boxPtr, XtParent( list))    )
    {   
        if(    !listLabelBox    )
        {   layoutPtr->space_above = vspace;
            vspace = BB_MarginHeight(sb);
            } 
        layoutPtr->stretch_height = TRUE ;
        layoutPtr->min_height = 70 ;
        boxPtr += 2 ;
        ++layoutPtr ;
        }

    /* work area, XmPLACE_ABOVE_SELECTION */

    if (sb->selection_box.child_placement == XmPLACE_ABOVE_SELECTION)
      SetupWorkArea(sb)

    /* selection label */

    selLabelBox = FALSE ;
    if(    _XmGeoSetupKid( boxPtr, SB_SelectionLabel( sb))    )
    {   selLabelBox = TRUE ;
        layoutPtr->space_above = vspace;
        vspace = BB_MarginHeight(sb);
        boxPtr += 2 ;
        ++layoutPtr ;
        }

    /* selection text */

    if(    _XmGeoSetupKid( boxPtr, SB_Text( sb))    )
    {   
        if(    !selLabelBox    )
        {   layoutPtr->space_above = vspace;
            vspace = BB_MarginHeight(sb);
            } 
        boxPtr += 2 ;
        ++layoutPtr ;
        }

    /* work area, XmPLACE_BELOW_SELECTION */

    if (sb->selection_box.child_placement == XmPLACE_BELOW_SELECTION)
      SetupWorkArea(sb)

    /* separator */

    if(    _XmGeoSetupKid( boxPtr, SB_Separator( sb))    )
    {   layoutPtr->fix_up = _XmSeparatorFix ;
        layoutPtr->space_above = vspace;
        vspace = BB_MarginHeight(sb);
        boxPtr += 2 ;
        ++layoutPtr ;
        } 

    /* button row */

    firstButtonBox = boxPtr ;
    if(    _XmGeoSetupKid( boxPtr, SB_OkButton( sb))    )
    {   ++boxPtr ;
        } 
    for (i = 0; i < sb->composite.num_children; i++)
    {
      Widget w = sb->composite.children[i];
      if (IsButton(w) && !IsAutoButton(sb,w) && w != SB_WorkArea(sb))
      {
          if (_XmGeoSetupKid( boxPtr, w))
          {   ++boxPtr ;
              } 
          }
      }
    if(    _XmGeoSetupKid( boxPtr, SB_ApplyButton( sb))    )
    {   ++boxPtr ;
        } 
    if(    _XmGeoSetupKid( boxPtr, SB_CancelButton( sb))    )
    {   ++boxPtr ;
        } 
    if(    _XmGeoSetupKid( boxPtr, SB_HelpButton( sb))    )
    {   ++boxPtr ;
        } 
    if(    boxPtr != firstButtonBox    )
    {   layoutPtr->fill_mode = XmGEO_CENTER ;
        layoutPtr->fit_mode = XmGEO_WRAP ;
        layoutPtr->space_above = vspace;
        vspace = BB_MarginHeight(sb);
        if(    !(sb->selection_box.minimize_buttons)    )
        {   layoutPtr->even_width = 1 ;
            } 
        layoutPtr->even_height = 1 ;
	++layoutPtr ;
        }

    /* the end. */

    layoutPtr->space_above = vspace;
    layoutPtr->end = TRUE ;
    return( geoSpec) ;
    }

/****************************************************************/
Boolean 
#ifdef _NO_PROTO
_XmSelectionBoxNoGeoRequest( geoSpec )
        XmGeoMatrix geoSpec ;
#else
_XmSelectionBoxNoGeoRequest(
        XmGeoMatrix geoSpec )
#endif /* _NO_PROTO */
{
/****************/

    if(    BB_InSetValues( geoSpec->composite)
        && (XtClass( geoSpec->composite) == xmSelectionBoxWidgetClass)    )
    {   
        return( TRUE) ;
        } 
    return( FALSE) ;
    }
   
/****************************************************************
 * Call the callbacks for a SelectionBox button.
 ****************/
static void 
#ifdef _NO_PROTO
SelectionBoxCallback( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
SelectionBoxCallback(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	unsigned char		which_button = (unsigned) client_data;
	XmSelectionBoxWidget	sel = (XmSelectionBoxWidget) XtParent (w);
	XmAnyCallbackStruct	*callback = (XmAnyCallbackStruct *) call_data;
	XmSelectionBoxCallbackStruct	temp;
	Boolean			match = True;
	String			text_value;
/****************/

#ifndef USE_TEXT_IN_DIALOGS
	text_value = XmTextFieldGetString (SB_Text (sel));
#else
	text_value = XmTextGetString (SB_Text (sel));
#endif
	temp.event = callback->event;
	temp.value = XmStringLtoRCreate (text_value, XmFONTLIST_DEFAULT_TAG);
	temp.length = XmStringLength (temp.value);
	XtFree (text_value);
	
	switch (which_button)
	{
		case XmDIALOG_OK_BUTTON:
                        if (sel->selection_box.list != NULL)
			   if (sel->selection_box.must_match)
			   {
				match = XmListItemExists (SB_List (sel), temp.value);
			   }
			if (!match)
			{
				temp.reason = XmCR_NO_MATCH;
				XtCallCallbackList ((Widget) sel, sel->
					selection_box.no_match_callback, &temp);
			}
			else
			{
				temp.reason = XmCR_OK;
				XtCallCallbackList ((Widget) sel, sel->
					selection_box.ok_callback, &temp);
			}
			break;

		case XmDIALOG_APPLY_BUTTON:
			temp.reason = XmCR_APPLY;
			XtCallCallbackList ((Widget) sel, sel->selection_box.apply_callback, &temp);
			break;

		case XmDIALOG_CANCEL_BUTTON:
			temp.reason = XmCR_CANCEL;
			XtCallCallbackList ((Widget) sel, sel->selection_box.cancel_callback, &temp);
			break;

		case XmDIALOG_HELP_BUTTON:
			/* Invoke the help system. */
                        _XmManagerHelp((Widget)sel, callback->event, NULL, NULL) ;
			break;
	}
        XmStringFree( temp.value) ;
        return ;
}
   
/****************************************************************
 * Process callback from the List in a SelectionBox.
 ****************/
static void 
#ifdef _NO_PROTO
ListCallback( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
ListCallback(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	XmListCallbackStruct	*callback = (XmListCallbackStruct *) call_data;
	XmSelectionBoxWidget	sel = (XmSelectionBoxWidget) client_data ;
	XmGadgetClass		gadget_class;
        XmGadget                dbutton = (XmGadget)
                                                BB_DynamicDefaultButton( sel) ;
	char			*text_value;
/****************/

	/*	Update the text widget to relect the latest list selection.
	*	If list default action (double click), activate default button.
	*/
	sel->selection_box.list_selected_item_position = 
                                                       callback->item_position;
	if ((text_value = _XmStringGetTextConcat( callback->item)) != NULL)
	{   
#ifndef USE_TEXT_IN_DIALOGS
            XmTextFieldSetString (SB_Text (sel), text_value);
            XmTextFieldSetCursorPosition (SB_Text (sel),
			          XmTextFieldGetLastPosition( SB_Text( sel))) ;
#else
            XmTextSetString (SB_Text (sel), text_value);
            XmTextSetCursorPosition (SB_Text (sel),
			          XmTextGetLastPosition( SB_Text( sel))) ;
#endif
            XtFree( text_value) ;
            } 
        /* Catch only double-click default action here.
        *  Key press events are handled through the ParentProcess routine.
        */
	if(    (callback->reason == XmCR_DEFAULT_ACTION)
            && (callback->event->type != KeyPress)
            && dbutton  &&  XtIsManaged( dbutton)
            && XtIsSensitive( dbutton)  &&  XmIsGadget( dbutton)    )
	 {
            gadget_class = (XmGadgetClass) dbutton->object.widget_class ;
            if (gadget_class->gadget_class.arm_and_activate)
		{   
		/* pass the event so that the button can pass it on to its
		** callbacks, even though the event isn't within the button
		*/
		(*(gadget_class->gadget_class.arm_and_activate))
			  ((Widget) dbutton, callback->event, NULL, NULL) ;
		} 
	 }
        return ;
        }


/****************************************************************
 * Set the label string of a label or button
 ****************/
static void 
#ifdef _NO_PROTO
UpdateString( w, string, direction )
        Widget w ;
        XmString string ;
        XmStringDirection direction ;
#else
UpdateString(
        Widget w,
        XmString string,
#if NeedWidePrototypes
        int direction )
#else
        XmStringDirection direction )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
	Arg		al[3];
    	register int	ac = 0;
/****************/

	if (w)
	{
		XtSetArg (al[ac], XmNstringDirection, direction);  ac++;
		XtSetArg (al[ac], XmNlabelString, string);  ac++;
		XtSetValues (w, al, ac);
	}
        return ;
}

/****************************************************************
 * Update widget when values change.
 ****************/
static Boolean 
#ifdef _NO_PROTO
SetValues( cw, rw, nw, args, num_args )
        Widget cw ;
        Widget rw ;
        Widget nw ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValues(
        Widget cw,
        Widget rw,
        Widget nw,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
            XmSelectionBoxWidget current = (XmSelectionBoxWidget) cw ;
            XmSelectionBoxWidget request = (XmSelectionBoxWidget) rw ;
            XmSelectionBoxWidget new_w = (XmSelectionBoxWidget) nw ;
	Arg		al[10];
	register int	ac;

	String		text_value ;
/****************/

	BB_InSetValues (new_w) = True;


      /*      Validate child placement.
      */
      if(    (new_w->selection_box.child_placement
                      != current->selection_box.child_placement)
              && !XmRepTypeValidValue( XmRID_CHILD_PLACEMENT,
                      new_w->selection_box.child_placement, (Widget) new_w)    )
      {
          new_w->selection_box.child_placement =
                      current->selection_box.child_placement;
          }

	/*	Update label strings.
	*/
	if(    new_w->selection_box.selection_label_string
                          != current->selection_box.selection_label_string    )
	{   UpdateString( SB_SelectionLabel (new_w), 
                                     new_w->selection_box.selection_label_string,
                                                    SB_StringDirection (new_w)) ;
            new_w->selection_box.selection_label_string = NULL ;
            }

	if(    new_w->selection_box.list_label_string
                               != current->selection_box.list_label_string    )
	{   UpdateString( SB_ListLabel (new_w), 
                                          new_w->selection_box.list_label_string,
                                                    SB_StringDirection (new_w)) ;
            new_w->selection_box.list_label_string = NULL ;
            }

	if(    new_w->selection_box.ok_label_string
                                 != current->selection_box.ok_label_string    )
	{   UpdateString( SB_OkButton (new_w), 
                                            new_w->selection_box.ok_label_string,
                                                    SB_StringDirection (new_w)) ;
            new_w->selection_box.ok_label_string = NULL ;
            }

	if(    new_w->selection_box.apply_label_string 
                              != current->selection_box.apply_label_string    )
	{   UpdateString( SB_ApplyButton (new_w), 
                                         new_w->selection_box.apply_label_string,
                                                    SB_StringDirection (new_w)) ;
            new_w->selection_box.apply_label_string = NULL ;
            }

	if(    new_w->selection_box.cancel_label_string
                             != current->selection_box.cancel_label_string    )
	{   UpdateString( SB_CancelButton (new_w), 
                                        new_w->selection_box.cancel_label_string,
                                                    SB_StringDirection (new_w)) ;
            new_w->selection_box.cancel_label_string = NULL ;
            }

	if(    new_w->selection_box.help_label_string
                               != current->selection_box.help_label_string    )
	{   UpdateString( SB_HelpButton (new_w), 
                                          new_w->selection_box.help_label_string,
                                                    SB_StringDirection (new_w)) ;
            new_w->selection_box.help_label_string = NULL ;
            }

	/*	Update List widget.
	*/
	ac = 0;
	if(    new_w->selection_box.list_items    )
	{   
            XtSetArg( al[ac], XmNitems, 
                                       new_w->selection_box.list_items) ;  ac++ ;
            new_w->selection_box.list_items = NULL ;
            }
	if(    new_w->selection_box.list_item_count != XmUNSPECIFIED    )
	{   XtSetArg( al[ac], XmNitemCount, 
                                  new_w->selection_box.list_item_count) ;  ac++ ;
            new_w->selection_box.list_item_count = XmUNSPECIFIED ;
            }
	if (new_w->selection_box.list_visible_item_count !=
		current->selection_box.list_visible_item_count)
	{
		XtSetArg (al[ac], XmNvisibleItemCount, 
			new_w->selection_box.list_visible_item_count);  ac++;
	}
	if (ac)
	{
		if (SB_List (new_w))
			XtSetValues (SB_List (new_w), al, ac);
	}


	/*	Update Text widget.
	*/
        text_value = NULL ;
	ac = 0;
	if(    new_w->selection_box.text_string
                                     != current->selection_box.text_string    )
	{   
            text_value = _XmStringGetTextConcat(
                                              new_w->selection_box.text_string) ;
            XtSetArg( al[ac], XmNvalue, text_value) ;  ac++ ;
            new_w->selection_box.text_string = (XmString) XmUNSPECIFIED ;
            }
	if (new_w->selection_box.text_columns !=
		current->selection_box.text_columns)
	{
		XtSetArg (al[ac], XmNcolumns, 
			new_w->selection_box.text_columns);  ac++;
	}
	if (ac)
	{
		if (SB_Text (new_w))
			XtSetValues (SB_Text (new_w), al, ac);
	}
	if (text_value)
	{
#ifndef USE_TEXT_IN_DIALOGS
		if (SB_Text (new_w))
			XmTextFieldSetCursorPosition (SB_Text (new_w),
			          XmTextFieldGetLastPosition( SB_Text( new_w))) ;
#else
		if (SB_Text (new_w))
			XmTextSetCursorPosition (SB_Text (new_w),
			          XmTextGetLastPosition( SB_Text( new_w))) ;
#endif
		XtFree (text_value);
	}

	/*	Validate dialog type.
	*/
	if (request->selection_box.dialog_type !=
			 current->selection_box.dialog_type)
	{
		_XmWarning( (Widget) new_w, WARN_DIALOG_TYPE_CHANGE);
		new_w->selection_box.dialog_type = 
			current->selection_box.dialog_type;
	}
	BB_InSetValues (new_w) = False;

	/*	If this is the instantiated class then do layout.
	*/
	if(    XtClass( new_w) == xmSelectionBoxWidgetClass    )
	{
            _XmBulletinBoardSizeUpdate( (Widget) new_w) ;
	    }
	return (Boolean) (FALSE);
        }

/****************************************************************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxGetSelectionLabelString( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmSelectionBoxGetSelectionLabelString(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{   
            XmSelectionBoxWidget sel = (XmSelectionBoxWidget) wid ;
            XmString        data ;
            Arg             al[1] ;
/****************/

    if(    SB_SelectionLabel (sel)    )
    {   
        XtSetArg( al[0], XmNlabelString, &data) ;
        XtGetValues( SB_SelectionLabel( sel), al, 1) ;
        *value = (XtArgVal) data ;
        }
    else
    {   *value = (XtArgVal) NULL ;
        } 
    return ;
    }
/****************************************************************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxGetListLabelString( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmSelectionBoxGetListLabelString(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
            XmSelectionBoxWidget sel = (XmSelectionBoxWidget) wid ;
            XmString        data ;
            Arg             al[1] ;
/****************/

    if(    SB_ListLabel( sel)    )
    {   
        XtSetArg( al[0], XmNlabelString, &data) ;
        XtGetValues( SB_ListLabel( sel), al, 1) ;
        *value = (XtArgVal) data ;
        }
    else
    {   *value = (XtArgVal) NULL ;
        } 
    return ;
    }
/****************************************************************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxGetTextColumns( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmSelectionBoxGetTextColumns(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
            XmSelectionBoxWidget sel = (XmSelectionBoxWidget) wid ;
            short           data ;
            Arg             al[1] ;
/****************/

    if(    SB_Text( sel)    )
    {   
        XtSetArg( al[0], XmNcolumns, &data) ;
        XtGetValues( SB_Text( sel), al, 1) ;
        *value = (XtArgVal) data ;
        }
    else
    {   *value = (XtArgVal) 0 ;
        } 
    return ;
    }
/****************************************************************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxGetTextString( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmSelectionBoxGetTextString(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{   
            XmSelectionBoxWidget sel = (XmSelectionBoxWidget) wid ;
            String          data = NULL ;
            XmString        text_string ;
            Arg             al[1] ;
/****************/

    if(    SB_Text( sel)    )
    {   
        XtSetArg( al[0], XmNvalue, &data) ;
        XtGetValues( SB_Text( sel), al, 1) ;
        text_string = XmStringLtoRCreate( data, XmFONTLIST_DEFAULT_TAG) ;
        *value = (XtArgVal) text_string ;
        }
    else
    {   *value = (XtArgVal) NULL ;
    	}
    return;
    }
/****************************************************************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxGetListItems( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmSelectionBoxGetListItems(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{   
            XmSelectionBoxWidget sel = (XmSelectionBoxWidget) wid ;
            Arg             al[1] ;
            XmString        data ;
/****************/

    if(    SB_List( sel)    )
    {   
        XtSetArg( al[0], XmNitems, &data) ;
        XtGetValues( SB_List( sel), al, 1) ;
        *value = (XtArgVal) data ;
        }
    else
    {   *value = (XtArgVal) NULL ;
        } 
    return ;
    }
/****************************************************************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxGetListItemCount( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmSelectionBoxGetListItemCount(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{   
            XmSelectionBoxWidget sel = (XmSelectionBoxWidget) wid ;
            int             data ;
            Arg             al[1] ;
/****************/

    if(    SB_List( sel)    )
    {   
        XtSetArg( al[0], XmNitemCount, &data) ;
        XtGetValues( SB_List( sel), al, 1) ;
        *value = (XtArgVal) data ;
        }
    else
    {   *value = (XtArgVal) 0 ;
        } 
    return ;
    }
/****************************************************************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxGetListVisibleItemCount( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmSelectionBoxGetListVisibleItemCount(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
            XmSelectionBoxWidget sel = (XmSelectionBoxWidget) wid ;
            int             data ;
            Arg             al[1] ;
/****************/

    if(    SB_List( sel)    )
    {   
        XtSetArg( al[0], XmNvisibleItemCount, &data) ;
        XtGetValues( SB_List( sel), al, 1) ;
        *value = (XtArgVal) data ;
        }
    else
    {   *value = (XtArgVal) 0 ;
        } 
    return ;
    }
/****************************************************************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxGetOkLabelString( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmSelectionBoxGetOkLabelString(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
            XmSelectionBoxWidget sel = (XmSelectionBoxWidget) wid ;
            XmString        data ;
            Arg             al[1] ;
/****************/

    if(    SB_OkButton( sel)    )
    {   
        XtSetArg( al[0], XmNlabelString, &data) ;
        XtGetValues( SB_OkButton( sel), al, 1) ;
        *value = (XtArgVal) data ;
        }
    else
    {   *value = (XtArgVal) NULL ;
	}
    return;
    }
/****************************************************************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxGetApplyLabelString( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmSelectionBoxGetApplyLabelString(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
            XmSelectionBoxWidget sel = (XmSelectionBoxWidget) wid ;
            XmString        data ;
            Arg             al[1] ;
/****************/

    if(    SB_ApplyButton( sel)    )
    {   
        XtSetArg( al[0], XmNlabelString, &data) ;
        XtGetValues( SB_ApplyButton( sel), al, 1) ;
        *value = (XtArgVal) data ;
        }
    else
    {   *value = (XtArgVal) NULL ;
        } 
    return ;
    }
/****************************************************************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxGetCancelLabelString( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmSelectionBoxGetCancelLabelString(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
            XmSelectionBoxWidget sel = (XmSelectionBoxWidget) wid ;
            XmString        data ;
            Arg             al[1] ;
/****************/

    if(    SB_CancelButton( sel)    )
    {   
        XtSetArg( al[0], XmNlabelString, &data) ;
        XtGetValues( SB_CancelButton( sel), al, 1) ;
        *value = (XtArgVal) data ;
        }
    else
    {   *value = (XtArgVal) NULL ;
        } 
    return ;
    }
/****************************************************************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxGetHelpLabelString( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmSelectionBoxGetHelpLabelString(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
            XmSelectionBoxWidget sel = (XmSelectionBoxWidget) wid ;
            XmString        data ;
            Arg             al[1] ;
/****************/

    if(    SB_HelpButton( sel)    )
    {   
        XtSetArg( al[0], XmNlabelString, &data) ;
        XtGetValues( SB_HelpButton( sel), al, 1) ;
        *value = (XtArgVal) data ;
        }
    else
    {   *value = (XtArgVal) NULL ;
        } 
    return ;
    }

/****************************************************************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxUpOrDown( wid, event, argv, argc )
        Widget wid ;
        XEvent *event ;
        String *argv ;
        Cardinal *argc ;
#else
_XmSelectionBoxUpOrDown(
        Widget wid,
        XEvent *event,
        String *argv,
        Cardinal *argc )
#endif /* _NO_PROTO */
{   
            XmSelectionBoxWidget sel = (XmSelectionBoxWidget) wid ;
            int	            visible ;
            int	            top ;
            int	            key_pressed ;
            Widget	    list ;
            int	*           position ;
            int	            count ;
            Arg             av[5] ;
            Cardinal        ac ;
/****************/

    if(    !(list = sel->selection_box.list)    )
    {   return ;
        } 
    ac = 0 ;
    XtSetArg( av[ac], XmNitemCount, &count) ; ++ac ;
    XtSetArg( av[ac], XmNtopItemPosition, &top) ; ++ac ;
    XtSetArg( av[ac], XmNvisibleItemCount, &visible) ; ++ac ;
    XtGetValues( list, av, ac) ;

    if(    !count    )
    {   return ;
        } 
    key_pressed = atoi( *argv) ;
    position = &(sel->selection_box.list_selected_item_position) ;

    if(    *position == 0    )
    {   /*  No selection, so select first item or last if key_pressed == end.
        */
        if(    key_pressed == 3    )
        {   *position = count ;
            XmListSelectPos( list, *position, True) ;
            } 
        else
        {   XmListSelectPos( list, ++*position, True) ;
            } 
        } 
    else
    {   if(    !key_pressed && (*position > 1)    )
        {   /*  up  */
            XmListDeselectPos( list, *position) ;
            XmListSelectPos( list, --*position, True) ;
            }
        else
        {   if(    (key_pressed == 1) && (*position < count)    )
            {   /*  down  */
                XmListDeselectPos( list, *position) ;
                XmListSelectPos( list, ++*position, True) ;
                } 
            else
            {   if(    key_pressed == 2    )
                {   /*  home  */
                    XmListDeselectPos( list, *position) ;
                    *position = 1 ;
                    XmListSelectPos( list, *position, True) ;
                    } 
                else
                {   if(    key_pressed == 3    )
                    {   /*  end  */
                        XmListDeselectPos( list, *position) ;
                        *position = count ;
                        XmListSelectPos( list, *position, True) ;
                        } 
                    } 
                } 
            }
        } 
    if(    top > *position    )
    {   XmListSetPos( list, *position) ;
        } 
    else
    {   if(    (top + visible) <= *position    )
        {   XmListSetBottomPos( list, *position) ;
            } 
        } 
    return ;
    }

/****************************************************************/
void 
#ifdef _NO_PROTO
_XmSelectionBoxRestore( wid, event, argv, argc )
        Widget wid ;
        XEvent *event ;
        String *argv ;
        Cardinal *argc ;
#else
_XmSelectionBoxRestore(
        Widget wid,
        XEvent *event,
        String *argv,
        Cardinal *argc )
#endif /* _NO_PROTO */
{
            XmSelectionBoxWidget sel = (XmSelectionBoxWidget) wid ;
            Widget          list ;
            int	            count ;
            XmString *      items ;
            Arg             al[5] ;
            int             ac ;
            String          textItem ;
/****************/

    list = SB_List( sel) ;

    if(    list
        && SB_Text( sel)    )
    {   
        ac = 0 ;
        XtSetArg( al[ac], XmNselectedItems, &items) ; ++ac ;
        XtSetArg( al[ac], XmNselectedItemCount, &count) ; ++ac ;
        XtGetValues( list, al, ac) ;
        if(    count    )
        {   
            textItem = _XmStringGetTextConcat( *items) ;
#ifndef USE_TEXT_IN_DIALOGS
            XmTextFieldSetString( SB_Text( sel), textItem) ;
            XmTextFieldSetCursorPosition( SB_Text( sel),
			          XmTextFieldGetLastPosition( SB_Text( sel))) ;
#else
            XmTextSetString( SB_Text( sel), textItem) ;
            XmTextSetCursorPosition( SB_Text( sel),
			          XmTextGetLastPosition( SB_Text( sel))) ;
#endif
            XtFree( textItem) ;
            } 
        else
        {   
#ifndef USE_TEXT_IN_DIALOGS
            XmTextFieldSetString( SB_Text( sel), NULL) ;
#else
            XmTextSetString( SB_Text( sel), NULL) ;
#endif
            } 
        } 
    return ;
    }

/****************************************************************
 * This function returns the widget id of a SelectionBox child widget.
 ****************/
Widget 
#ifdef _NO_PROTO
XmSelectionBoxGetChild( sb, which )
        Widget sb ;
        unsigned char which ;
#else
XmSelectionBoxGetChild(
        Widget sb,
#if NeedWidePrototypes
        unsigned int which )
#else
        unsigned char which )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
/****************/
	Widget	child = NULL;

	switch (which)
	{
		case XmDIALOG_LIST:
			child = SB_List (sb);
			break;

		case XmDIALOG_LIST_LABEL:
			child = SB_ListLabel (sb);
			break;

		case XmDIALOG_SELECTION_LABEL:
			child = SB_SelectionLabel (sb);
			break;

		case XmDIALOG_WORK_AREA:
			child = SB_WorkArea (sb);
			break;

		case XmDIALOG_TEXT:
			child = SB_Text (sb);
			break;

		case XmDIALOG_SEPARATOR:
			child = SB_Separator (sb);
			break;

		case XmDIALOG_OK_BUTTON:
			child = SB_OkButton (sb);
			break;

		case XmDIALOG_APPLY_BUTTON:
			child = SB_ApplyButton (sb);
			break;

		case XmDIALOG_CANCEL_BUTTON:
			child = SB_CancelButton (sb);
			break;

		case XmDIALOG_HELP_BUTTON:
			child = SB_HelpButton (sb);
			break;

		case XmDIALOG_DEFAULT_BUTTON:
			child = SB_DefaultButton (sb);
			break;

		default:
			_XmWarning( (Widget) sb, WARN_CHILD_TYPE);
			break;
	}
	return (child);
}

/****************************************************************
 * This function creates and returns a SelectionBox widget.
 ****************/
Widget 
#ifdef _NO_PROTO
XmCreateSelectionBox( p, name, args, n )
        Widget p ;
        String name ;
        ArgList args ;
        Cardinal n ;
#else
XmCreateSelectionBox(
        Widget p,
        String name,
        ArgList args,
        Cardinal n )
#endif /* _NO_PROTO */
{
/****************/

    return (XtCreateWidget (name, xmSelectionBoxWidgetClass, p, args, n));
    }
/****************************************************************
 * This convenience function creates a DialogShell and a SelectionBox
 *   child of the shell; returns the SelectionBox widget.
 ****************/
Widget 
#ifdef _NO_PROTO
XmCreateSelectionDialog( ds_p, name, sb_args, sb_n )
        Widget ds_p ;
        String name ;
        ArgList sb_args ;
        Cardinal sb_n ;
#else
XmCreateSelectionDialog(
        Widget ds_p,
        String name,
        ArgList sb_args,
        Cardinal sb_n )
#endif /* _NO_PROTO */
{
	Widget		ds;		/*  DialogShell		*/
	ArgList		ds_args;	/*  arglist for shell	*/
	ArgList		_sb_args;	/*  arglist for sb	*/
	Widget		sb;		/*  new sb widget	*/
	char           *ds_name;
/****************/

	/*  create DialogShell parent
	*/
	ds_name = XtMalloc((strlen(name)+XmDIALOG_SUFFIX_SIZE+1)*sizeof(char));
	strcpy(ds_name,name);
	strcat(ds_name,XmDIALOG_SUFFIX);

        ds_args = (ArgList) XtMalloc( sizeof( Arg) * (sb_n + 1));
        memcpy( ds_args, sb_args, (sizeof( Arg) * sb_n));
        XtSetArg (ds_args[sb_n], XmNallowShellResize, True);
        ds = XmCreateDialogShell (ds_p, ds_name, ds_args, sb_n + 1);
  
        XtFree((char *) ds_args);
	XtFree(ds_name);

	/*  allocate arglist, copy args, add dialog type arg
	*/
	_sb_args = (ArgList) XtMalloc (sizeof (Arg) * (sb_n + 1));

	memcpy( _sb_args, sb_args, sizeof (Arg) * sb_n);
	XtSetArg (_sb_args[sb_n], XmNdialogType, XmDIALOG_SELECTION);  sb_n++;

	/*  create SelectionBox, free args, return 
	*/
	sb = XtCreateWidget (name, xmSelectionBoxWidgetClass, 
		ds, _sb_args, sb_n);
	XtAddCallback (sb, XmNdestroyCallback, _XmDestroyParentCallback, NULL);

	XtFree( (char *) _sb_args);
	return (sb);
}

/****************************************************************
 * This convenience function creates a DialogShell and a SelectionBox
 *   child of the shell; returns the SelectionBox widget.
 ****************/
Widget 
#ifdef _NO_PROTO
XmCreatePromptDialog( ds_p, name, sb_args, sb_n )
        Widget ds_p ;
        String name ;
        ArgList sb_args ;
        Cardinal sb_n ;
#else
XmCreatePromptDialog(
        Widget ds_p,
        String name,
        ArgList sb_args,
        Cardinal sb_n )
#endif /* _NO_PROTO */
{
	Widget		ds;		/*  DialogShell		*/
	ArgList		ds_args;	/*  arglist for shell	*/
	ArgList		_sb_args;	/*  arglist for sb	*/
	Widget		sb;		/*  new sb widget	*/
	char           *ds_name;
/****************/

	/*  create DialogShell parent
	*/
	ds_name = XtMalloc((strlen(name)+XmDIALOG_SUFFIX_SIZE+1)*sizeof(char));
	strcpy(ds_name,name);
	strcat(ds_name,XmDIALOG_SUFFIX);

        ds_args = (ArgList) XtMalloc( sizeof( Arg) * (sb_n + 1));
        memcpy( ds_args, sb_args, (sizeof( Arg) * sb_n));
        XtSetArg (ds_args[sb_n], XmNallowShellResize, True);
        ds = XmCreateDialogShell (ds_p, ds_name, ds_args, sb_n + 1);
  
        XtFree((char *) ds_args);
	XtFree(ds_name);

	/*  allocate arglist, copy args, add dialog type arg
	*/
	_sb_args = (ArgList) XtMalloc (sizeof (Arg) * (sb_n + 1));

	memcpy( _sb_args, sb_args, sizeof (Arg) * sb_n);
	XtSetArg (_sb_args[sb_n], XmNdialogType, XmDIALOG_PROMPT);  sb_n++;


	/*  create SelectionBox, free args, return 
	*/
	sb = XtCreateWidget (name, xmSelectionBoxWidgetClass, 
		ds, _sb_args, sb_n);
	XtAddCallback (sb, XmNdestroyCallback, _XmDestroyParentCallback, NULL);

	XtFree( (char *) _sb_args);
	return (sb);
}
