#pragma ident	"@(#)m1.2libs:Xm/MenuShellP.h	1.2"
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
#ifndef _XmMenuShellP_h
#define _XmMenuShellP_h

#include <Xm/MenuShell.h>
#include <Xm/XmP.h>
#include <X11/ShellP.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The MenuShell instance record */

typedef struct 
{
    unsigned char	    focus_policy;
    XmFocusData		    focus_data;
    Boolean                 private_shell;
    XmFontList      	    default_font_list;
    XmFontList              button_font_list;
    XmFontList              label_font_list;
} XmMenuShellPart;


/* Full instance record declaration */

typedef  struct _XmMenuShellRec 
{
    CorePart		    core;
    CompositePart	    composite;
    ShellPart		    shell;
    OverrideShellPart	    override;
    XmMenuShellPart	    menu_shell;
} XmMenuShellRec;

typedef  struct _XmMenuShellWidgetRec /* OBSOLETE (for compatibility only).*/
{
    CorePart		    core;
    CompositePart	    composite;
    ShellPart		    shell;
    OverrideShellPart	    override;
    XmMenuShellPart	    menu_shell;
} XmMenuShellWidgetRec;



/* MenuShell class structure */

typedef struct 
{
	XtActionProc popdownOne;  /* fnct to unpost portion or all of menu */
	XtActionProc popdownEveryone; /* fnct to unpost portion of menu */
	XtActionProc popdownDone;     /* fnct to unpost all menus */
	XmMenuPopupProc popupSharedMenupane; /* fnct to post shared menus */
				    /* Really should be popupSharedMenuShell */
	XtPointer		extension;	 /* Pointer to extension record */
} XmMenuShellClassPart;


/* Full class record declaration */

typedef struct _XmMenuShellClassRec 
{
    CoreClassPart	    core_class;
    CompositeClassPart	    composite_class;
    ShellClassPart	    shell_class;
    OverrideShellClassPart  override_shell_class;
    XmMenuShellClassPart    menu_shell_class;
} XmMenuShellClassRec;


externalref XmMenuShellClassRec  xmMenuShellClassRec;

#define MS_FocusPolicy(m) \
       (((XmMenuShellWidget)m)->menu_shell.focus_policy)

/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmEnterRowColumn() ;
extern void _XmClearTraversal() ;
extern void _XmSetLastManagedMenuTime() ;

#else

extern void _XmEnterRowColumn( 
                        Widget widget,
                        XtPointer closure,
                        XEvent *event,
                        Boolean *cont) ;
extern void _XmClearTraversal( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmSetLastManagedMenuTime(
			Widget wid,
			Time newTime ) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmMenuShellP_h */
/* DON'T ADD STUFF AFTER THIS #endif */
