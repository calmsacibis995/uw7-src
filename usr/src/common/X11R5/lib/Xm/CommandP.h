#pragma ident	"@(#)m1.2libs:Xm/CommandP.h	1.2"
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
#ifndef _XmCommandP_h
#define _XmCommandP_h

#include <Xm/SelectioBP.h>
#include <Xm/Command.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Constraint part record for Command widget */

typedef struct _XmCommandConstraintPart
{
   char unused;
} XmCommandConstraintPart, * XmCommandConstraint;

/*  New fields for the Command widget class record  */

typedef struct
{
    XtPointer           extension;      /* Pointer to extension record */
} XmCommandClassPart;


/* Full class record declaration */

typedef struct _XmCommandClassRec
{
   CoreClassPart            core_class;
   CompositeClassPart       composite_class;
   ConstraintClassPart      constraint_class;
   XmManagerClassPart       manager_class;
   XmBulletinBoardClassPart bulletin_board_class;
   XmSelectionBoxClassPart  selection_box_class;
   XmCommandClassPart       command_class;
} XmCommandClassRec;

externalref XmCommandClassRec xmCommandClassRec;

/* New fields for the Command widget record */

typedef struct
{
    XtCallbackList       callback;
    XtCallbackList       value_changed_callback;
    int                  history_max_items;
    Boolean              error;        /* error has been made visible in list */
} XmCommandPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _XmCommandRec
{
    CorePart	        core;
    CompositePart       composite;
    ConstraintPart      constraint;
    XmManagerPart       manager;
    XmBulletinBoardPart bulletin_board;
    XmSelectionBoxPart  selection_box;
    XmCommandPart       command;
} XmCommandRec;



/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmCommandReturn() ;
extern void _XmCommandUpOrDown() ;

#else

extern void _XmCommandReturn( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *numParams) ;
extern void _XmCommandUpOrDown( 
                        Widget wid,
                        XEvent *event,
                        String *argv,
                        Cardinal *argc) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmCommandP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
