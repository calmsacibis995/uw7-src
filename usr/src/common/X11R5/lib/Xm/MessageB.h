#pragma ident	"@(#)m1.2libs:Xm/MessageB.h	1.2"
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
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmMessage_h
#define _XmMessage_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Class record constants */

externalref WidgetClass xmMessageBoxWidgetClass;

typedef struct _XmMessageBoxClassRec * XmMessageBoxWidgetClass;
typedef struct _XmMessageBoxRec      * XmMessageBoxWidget;

/* fast XtIsSubclass define */
#ifndef XmIsMessageBox
#define XmIsMessageBox(w) XtIsSubclass (w, xmMessageBoxWidgetClass)
#endif 


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmCreateMessageBox() ;
extern Widget XmCreateMessageDialog() ;
extern Widget XmCreateErrorDialog() ;
extern Widget XmCreateInformationDialog() ;
extern Widget XmCreateQuestionDialog() ;
extern Widget XmCreateWarningDialog() ;
extern Widget XmCreateWorkingDialog() ;
extern Widget XmCreateTemplateDialog() ;
extern Widget XmMessageBoxGetChild() ;

#else

extern Widget XmCreateMessageBox( 
                        Widget parent,
                        char *name,
                        ArgList al,
                        Cardinal ac) ;
extern Widget XmCreateMessageDialog( 
                        Widget parent,
                        char *name,
                        ArgList al,
                        Cardinal ac) ;
extern Widget XmCreateErrorDialog( 
                        Widget parent,
                        char *name,
                        ArgList al,
                        Cardinal ac) ;
extern Widget XmCreateInformationDialog( 
                        Widget parent,
                        char *name,
                        ArgList al,
                        Cardinal ac) ;
extern Widget XmCreateQuestionDialog( 
                        Widget parent,
                        char *name,
                        ArgList al,
                        Cardinal ac) ;
extern Widget XmCreateWarningDialog( 
                        Widget parent,
                        char *name,
                        ArgList al,
                        Cardinal ac) ;
extern Widget XmCreateWorkingDialog( 
                        Widget parent,
                        char *name,
                        ArgList al,
                        Cardinal ac) ;
extern Widget XmCreateTemplateDialog( 
                        Widget parent,
                        char *name,
                        ArgList al,
                        Cardinal ac) ;
extern Widget XmMessageBoxGetChild( 
                        Widget widget,
#if NeedWidePrototypes
                        unsigned int child) ;
#else
                        unsigned char child) ;
#endif /* NeedWidePrototypes */

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmMessage_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
