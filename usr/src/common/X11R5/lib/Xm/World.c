#pragma ident	"@(#)m1.2libs:Xm/World.c	1.2"
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
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1987, 1988, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY */

#include <Xm/WorldP.h>
#include <Xm/BaseClassP.h>
#include <Xm/DisplayP.h>


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO


#else


#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/***************************************************************************
 *
 * Class Record
 *
 ***************************************************************************/

externaldef(xmworldclassrec)
XmWorldClassRec xmWorldClassRec = {
    {	
	(WidgetClass) &xmDesktopClassRec,/* superclass		*/   
	"World",			/* class_name 		*/   
	sizeof(XmWorldRec), 		/* size 		*/   
	NULL,				/* Class Initializer 	*/   
	NULL,				/* class_part_init 	*/ 
	FALSE, 				/* Class init'ed ? 	*/   
	NULL,				/* initialize         	*/   
	NULL, 				/* initialize_notify    */ 
	NULL,	 			/* realize            	*/   
	NULL,	 			/* actions            	*/   
	0,				/* num_actions        	*/   
	NULL,				/* resources          	*/   
	0,				/* resource_count     	*/   
	NULLQUARK, 			/* xrm_class          	*/   
	FALSE, 				/* compress_motion    	*/   
	FALSE, 				/* compress_exposure  	*/   
	FALSE, 				/* compress_enterleave	*/   
	FALSE, 				/* visible_interest   	*/   
	NULL,				/* destroy            	*/   
	NULL,		 		/* resize             	*/   
	NULL, 				/* expose             	*/   
	NULL,		 		/* set_values         	*/   
	NULL, 				/* set_values_hook      */ 
	NULL,			 	/* set_values_almost    */ 
	NULL,				/* get_values_hook      */ 
	NULL, 				/* accept_focus       	*/   
	XtVersion, 			/* intrinsics version 	*/   
	NULL, 				/* callback offsets   	*/   
	NULL,				/* tm_table           	*/   
	NULL, 				/* query_geometry       */ 
	NULL, 				/* world_accelerator  */ 
	NULL, 				/* extension            */ 
    },	
    {					/* ext			*/
	NULL,				/* synthetic resources	*/
	0,				/* num syn resources	*/
	NULL,				/* extension		*/
    },
    {					/* desktop		*/
	(WidgetClass) &xmDisplayClassRec,/* child_class		*/
	XtInheritInsertChild,		/* insert_child		*/
	XtInheritDeleteChild,		/* delete_child		*/
	NULL,				/* extension		*/
    }
};

externaldef(xmworldclass) WidgetClass 
      xmWorldClass = (WidgetClass) (&xmWorldClassRec);


/************************************************************************
 *
 *  _XmGetWorldObject
 *
 ************************************************************************/
/* ARGSUSED */
XmWorldObject 
#ifdef _NO_PROTO
_XmGetWorldObject( shell, args, num_args )
        Widget shell ;
        ArgList args ;
        Cardinal *num_args ;
#else
_XmGetWorldObject(
        Widget shell,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmWorldObject	worldObject;
    static XContext	worldObjectContext = (XContext) NULL;
    XmWidgetExtData     ext;
    Display		*display;
    
    /*
    ** Note: in an ideal World we would be sure to delete this context when
    ** the display is closed, so that we don't get bad data if a second 
    ** display with the same id is opened.
    */
    if (worldObjectContext == (XContext) NULL)
      worldObjectContext = XUniqueContext();

    display = XtDisplayOfObject(shell);
    
    if (XFindContext(display,
		     (Window) NULL,
		     worldObjectContext,
		     (char **) &worldObject))
      {
	  WidgetClass		worldClass;
	  Widget		appShell = shell;

	  worldClass = _XmGetActualClass(display, xmWorldClass);
	  
	  while (XtParent(appShell)) 
	    appShell = XtParent(appShell);
	  
	  worldObject = (XmWorldObject)
	    XtCreateWidget("world",
			   worldClass,
			   appShell,
			   args,
			   num_args ? *num_args: 0);

          ext = _XmGetWidgetExtData(worldObject->ext.logicalParent,
                                    worldObject->ext.extensionType);
          _XmExtObjFree(ext->reqWidget);
          ext->reqWidget = NULL;
	  
	  XSaveContext(display,
		       (Window) NULL,
		       worldObjectContext,
		       (char *) worldObject);
      }
    return 
      worldObject;
}
