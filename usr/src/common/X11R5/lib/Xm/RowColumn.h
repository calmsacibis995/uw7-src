#pragma ident	"@(#)m1.2libs:Xm/RowColumn.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
#ifndef _XmRowColumn_h
#define _XmRowColumn_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

externalref WidgetClass xmRowColumnWidgetClass;

typedef struct _XmRowColumnClassRec * XmRowColumnWidgetClass;
typedef struct _XmRowColumnRec      * XmRowColumnWidget;

#ifndef XmIsRowColumn
#define XmIsRowColumn(w) XtIsSubclass((w),xmRowColumnWidgetClass)
#endif


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern void XmMenuPosition() ;
extern Widget XmCreateRowColumn() ;
extern Widget XmCreateWorkArea() ;
extern Widget XmCreateRadioBox() ;
extern Widget XmCreateOptionMenu() ;
extern Widget XmOptionLabelGadget() ;
extern Widget XmOptionButtonGadget() ;
extern Widget XmCreateMenuBar() ;
extern Widget XmCreatePopupMenu() ;
extern Widget XmCreatePulldownMenu() ;
extern void XmAddToPostFromList() ;
extern void XmRemoveFromPostFromList() ;
extern Widget XmGetPostedFromWidget() ;
extern Widget XmGetTearOffControl() ;

#else

extern void XmMenuPosition( 
                        Widget p,
                        XButtonPressedEvent *event) ;
extern Widget XmCreateRowColumn( 
                        Widget p,
                        char *name,
                        ArgList al,
                        Cardinal ac) ;
extern Widget XmCreateWorkArea( 
                        Widget p,
                        char *name,
                        ArgList al,
                        Cardinal ac) ;
extern Widget XmCreateRadioBox( 
                        Widget p,
                        char *name,
                        ArgList al,
                        Cardinal ac) ;
extern Widget XmCreateOptionMenu( 
                        Widget p,
                        char *name,
                        ArgList al,
                        Cardinal ac) ;
extern Widget XmOptionLabelGadget( 
                        Widget m) ;
extern Widget XmOptionButtonGadget( 
                        Widget m) ;
extern Widget XmCreateMenuBar( 
                        Widget p,
                        char *name,
                        ArgList al,
                        Cardinal ac) ;
extern Widget XmCreatePopupMenu( 
                        Widget p,
                        char *name,
                        ArgList al,
                        Cardinal ac) ;
extern Widget XmCreatePulldownMenu( 
                        Widget p,
                        char *name,
                        ArgList al,
                        Cardinal ac) ;
extern void XmAddToPostFromList( 
                        Widget menu_wid,
                        Widget widget) ;
extern void XmRemoveFromPostFromList( 
                        Widget menu_wid,
                        Widget widget) ;
extern Widget XmGetPostedFromWidget( 
                        Widget menu) ;
extern Widget XmGetTearOffControl(
			Widget menu) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmRowColumn_h  */
/* DON'T ADD STUFF AFTER THIS #endif */
