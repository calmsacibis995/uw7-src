#pragma ident	"@(#)m1.2libs:Xm/TearOffP.h	1.2"
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
#ifndef _XmTearOffP_h
#define _XmTearOffP_h

#include <Xm/XmP.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _XmExcludedParentPaneRec
{
   short pane_list_size;
   Widget *pane;
   short num_panes;
} XmExcludedParentPaneRec;

externalref XmExcludedParentPaneRec _XmExcludedParentPane;

/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmTearOffBtnDownEventHandler() ;
extern void _XmTearOffBtnUpEventHandler() ;
extern void _XmDestroyTearOffShell() ;
extern void _XmDismissTearOff() ;
extern void _XmTearOffInitiate() ;
extern void _XmAddTearOffEventHandlers() ;
extern Boolean _XmIsTearOffShellDescendant() ;
extern void _XmLowerTearOffObscuringPoppingDownPanes() ;
extern void _XmRestoreExcludedTearOffToToplevelShell() ;
extern void _XmRestoreTearOffToToplevelShell() ;
extern void _XmRestoreTearOffToMenuShell() ;

#else

extern void _XmTearOffBtnDownEventHandler( 
                        Widget reportingWidget,
                        XtPointer data,
                        XEvent *event,
                        Boolean *cont) ;
extern void _XmTearOffBtnUpEventHandler( 
                        Widget reportingWidget,
                        XtPointer data,
                        XEvent *event,
                        Boolean *cont) ;
extern void _XmDestroyTearOffShell( 
                        Widget wid) ;
extern void _XmDismissTearOff( 
                        Widget shell,
                        XtPointer closure,
                        XtPointer call_data) ;
extern void _XmTearOffInitiate( 
                        Widget wid,
                        XEvent *event) ;
extern void _XmAddTearOffEventHandlers( 
                        Widget wid) ;
extern Boolean _XmIsTearOffShellDescendant( 
                        Widget wid) ;
extern void _XmLowerTearOffObscuringPoppingDownPanes(
			Widget ancestor,
			Widget tearOff ) ;
extern void _XmRestoreExcludedTearOffToToplevelShell( 
                        Widget wid,
                        XEvent *event) ;
extern void _XmRestoreTearOffToToplevelShell( 
                        Widget wid,
                        XEvent *event) ;
extern void _XmRestoreTearOffToMenuShell( 
                        Widget wid,
                        XEvent *event) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif  /* _XmTearOffP_h */
/* DON'T ADD STUFF AFTER THIS #endif */
