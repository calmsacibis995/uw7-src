#pragma ident	"@(#)m1.2libs:Xm/VendorSP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.1
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1989, 1990  DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
/*
*  (c) Copyright 1988 MICROSOFT CORPORATION */
#ifndef  _XmVendorSP_h
#define _XmVendorSP_h

#ifndef _XmNO_BC_INCL
#define _XmNO_BC_INCL
#endif

#include <Xm/XmP.h>
#include <Xm/VendorS.h>
#include <X11/ShellP.h>
#include <Xm/DesktopP.h>
#include <Xm/ExtObjectP.h>

#ifdef __cplusplus
extern "C" {
#endif

externalref VendorShellClassRec  vendorShellClassRec;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern Cardinal _XmFilterResources() ;
extern void _XmAddGrab() ;
extern void _XmRemoveGrab() ;
extern XtGeometryResult _XmRootGeometryManager() ;
extern void _XmVendorExtRealize() ;
extern Display * _XmGetDefaultDisplay() ;

#else

extern Cardinal _XmFilterResources( 
                        XtResource *resources,
                        Cardinal numResources,
                        WidgetClass filterClass,
                        XtResource **filteredResourcesRtn) ;
extern void _XmAddGrab( 
                        Widget wid,
#if NeedWidePrototypes
                        int exclusive,
                        int spring_loaded) ;
#else
                        Boolean exclusive,
                        Boolean spring_loaded) ;
#endif /* NeedWidePrototypes */
extern void _XmRemoveGrab( 
                        Widget wid) ;
extern XtGeometryResult _XmRootGeometryManager( 
                        Widget w,
                        XtWidgetGeometry *request,
                        XtWidgetGeometry *reply) ;
extern void _XmVendorExtRealize( 
                        Widget w,
                        XtPointer closure,
                        XtPointer call_data) ;
extern Display * _XmGetDefaultDisplay( void ) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif  /* _XmVendorSP_h */
