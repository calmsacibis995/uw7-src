#pragma ident	"@(#)m1.2libs:Xm/TravActI.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.1
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmTravActI_h
#define _XmTravActI_h

#include "XmI.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XmFOCUS_RESET	1<<0
#define XmFOCUS_IGNORE	1<<1

/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern unsigned short _XmGetFocusFlag() ;
extern void _XmSetFocusFlag() ;
extern Boolean _XmGetFocusResetFlag() ;
extern void _XmSetFocusResetFlag() ;
extern void _XmTrackShellFocus() ;
extern void _XmPrimitiveEnter() ;
extern void _XmPrimitiveLeave() ;
extern void _XmPrimitiveUnmap() ;
extern void _XmPrimitiveFocusInInternal() ;
extern void _XmPrimitiveFocusOut() ;
extern void _XmPrimitiveFocusIn() ;
extern void _XmEnterGadget() ;
extern void _XmLeaveGadget() ;
extern void _XmFocusInGadget() ;
extern void _XmFocusOutGadget() ;
extern void _XmManagerEnter() ;
extern void _XmManagerLeave() ;
extern void _XmManagerFocusInInternal() ;
extern void _XmManagerFocusIn() ;
extern void _XmManagerFocusOut() ;
extern void _XmManagerUnmap() ;

#else

extern unsigned short _XmGetFocusFlag(
			Widget w,
			unsigned int mask) ;
extern void _XmSetFocusFlag(
			Widget w,
			unsigned int mask,
#if NeedWidePrototypes
        		int value ) ;
#else
        		Boolean value ) ;
#endif /* NeedWidePrototypes */
extern Boolean _XmGetFocusResetFlag( 
                        Widget w) ;
extern void _XmSetFocusResetFlag( 
                        Widget w,
#if NeedWidePrototypes
                        int value) ;
#else
                        Boolean value) ;
#endif /* NeedWidePrototypes */
extern void _XmTrackShellFocus( 
                        Widget widget,
                        XtPointer client_data,
                        XEvent *event,
                        Boolean *dontSwallow) ;
extern void _XmPrimitiveEnter( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmPrimitiveLeave( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmPrimitiveUnmap( 
                        Widget pw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmPrimitiveFocusInInternal( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmPrimitiveFocusOut( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmPrimitiveFocusIn( 
                        Widget pw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmEnterGadget( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmLeaveGadget( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmFocusInGadget( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmFocusOutGadget( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmManagerEnter( 
                        Widget wid,
                        XEvent *event_in,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmManagerLeave( 
                        Widget wid,
                        XEvent *event_in,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmManagerFocusInInternal( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmManagerFocusIn( 
                        Widget mw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmManagerFocusOut( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmManagerUnmap( 
                        Widget mw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmTravActI_h */
