#ifndef	NOIDENT
#ident	"@(#)basewindow:BaseWindoP.h	1.8"
#endif

#ifndef _BaseWindoP_h_
#define _BaseWindoP_h_

/*************************************************************************
 *
 * Description:
 *		Private ".h" file for the BaseWindow Widget
 *
 *****************************file*header********************************/

#include <X11/ShellP.h>
#include <Xol/BaseWindow.h>

		/* New fields for the BaseWindow widget class record	*/

typedef struct _BaseWindowClass {
    char no_class_fields;		/* Makes compiler happy */
} BaseWindowShellClassPart;

				/* Full class record declaration 	*/

typedef struct _BaseWindowClassRec {
    CoreClassPart		core_class;
    CompositeClassPart		composite_class;
    ShellClassPart		shell_class;
    WMShellClassPart		wm_shell_class;
    VendorShellClassPart	vendor_shell_class;
    TopLevelShellClassPart	top_level_shell_class;
    ApplicationShellClassPart	application_shell_class;
    BaseWindowShellClassPart	base_window_shell_class;
} BaseWindowShellClassRec;

extern BaseWindowShellClassRec baseWindowShellClassRec;

/***************************************
 *
 *  Instance (widget) structure 
 *
 **************************************/


			/* New fields for the BaseWindow widget record */
typedef struct {
    int		not_used;
} BaseWindowShellPart;

			/*
			 * Widget Instance declaration
			 */

typedef struct _BaseWindowShellRec {
    CorePart			core;
    CompositePart		composite;	
    ShellPart			shell;
    WMShellPart			wm;
    VendorShellPart		vendor;
    TopLevelShellPart		topLevel;
    ApplicationShellPart	application;
    BaseWindowShellPart		base_window;
} BaseWindowShellRec;

#endif /* _BaseWindoP_h_ */
