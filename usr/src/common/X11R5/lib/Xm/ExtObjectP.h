#pragma ident	"@(#)m1.2libs:Xm/ExtObjectP.h	1.3"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
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
#ifndef  _XmExtObjectP_h
#define _XmExtObjectP_h

#include <Xm/XmP.h>

#ifdef __cplusplus
extern "C" {
#endif

enum{	XmCACHE_EXTENSION = 1,			XmDESKTOP_EXTENSION,
	XmSHELL_EXTENSION,			XmPROTOCOL_EXTENSION,
	XmDEFAULT_EXTENSION
	} ;

#ifndef XmIsExtObject
#define XmIsExtObject(w) XtIsSubclass(w, xmExtObjectClass)
#endif /* XmIsExtObject */

/* Class record constants */

typedef struct _XmExtRec *XmExtObject;
typedef struct _XmExtClassRec *XmExtObjectClass;

externalref WidgetClass xmExtObjectClass;

#define XmNUM_ELEMENTS 4
#define XmNUM_BYTES 99

/* Class Extension definitions */

typedef struct _XmExtClassPart{
    XmSyntheticResource *syn_resources;   
    int                	num_syn_resources;   
#ifdef notdef
    XtResourceList	ext_resources;
    XtResourceList	compiled_ext_resources;
    Cardinal		num_ext_resources;
    Boolean		use_sub_resources;
#endif /* notdef */
    XtPointer		extension;
}XmExtClassPart, *XmExtClassPartPtr;

typedef struct _XmExtClassRec{
    ObjectClassPart		object_class;
    XmExtClassPart	 	ext_class;
}XmExtClassRec;

typedef struct {
    Widget		logicalParent;
    unsigned char	extensionType;
} XmExtPart, *XmExtPartPtr;

externalref XmExtClassRec 	xmExtClassRec;

typedef struct _XmExtRec{
    ObjectPart			object;
    XmExtPart			ext;
}XmExtRec;

typedef struct _XmExtCache {
   char    data[XmNUM_BYTES];
   Boolean inuse;
}XmExtCache;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern char * _XmExtObjAlloc() ;
extern void _XmExtObjFree() ;
extern void _XmBuildExtResources() ;

#else

extern char * _XmExtObjAlloc( 
                        int size) ;
extern void _XmExtObjFree( 
                        XtPointer element) ;
extern void _XmBuildExtResources( 
                        WidgetClass c) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif  /* _XmExtObjectP_h */
