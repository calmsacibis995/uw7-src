#pragma ident	"@(#)m1.2libs:Xm/RCUtilsP.h	1.2"
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
#ifndef _XmRCUtilsP_h
#define _XmRCUtilsP_h

#include <Xm/RowColumnP.h>

#ifdef __cplusplus
extern "C" {
#endif

/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern XmRCKidGeometry _XmRCGetKidGeo() ;
extern void _XmRCSetKidGeo() ;

#else

extern XmRCKidGeometry _XmRCGetKidGeo( 
                        Widget wid,
                        Widget instigator,
                        XtWidgetGeometry *request,
                        int uniform_border,
#if NeedWidePrototypes
                        int border,
#else
                        Dimension border,
#endif /* NeedWidePrototypes */
                        int uniform_width_margins,
                        int uniform_height_margins,
                        Widget help,
			Widget toc,
                        int geo_type) ;
extern void _XmRCSetKidGeo( 
                        XmRCKidGeometry kg,
                        Widget instigator) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif  /* _XmRCUtilsP_h */
/* DON'T ADD STUFF AFTER THIS #endif */
