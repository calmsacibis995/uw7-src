/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)olmisc:Packed.c	1.8"
#endif

/*
 *************************************************************************
 *
 * Date:	August 1988
 *
 * Description:
 *		This file contains the Packed Widget Code.
 *
 *******************************file*header*******************************
 */

						/* #includes go here	*/

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include <X11/Shell.h>
#include <Xol/OpenLookP.h>

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		Public  Procedures
 *
 **************************forward*declarations***************************
 */

					/* public procedures		*/

Widget  OlCreatePackedWidget();		/* Creates a single widget from
					 * a packed widget structure	*/
Widget  OlCreatePackedWidgetList();	/* Creates multiple widget
					 * hierarchies 			*/
/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define WORKAROUND 1

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * OlCreatePackedWidget - this routine creates a widget from an
 * OlPackedWidget structure.  Various checks are made to insure the
 * incoming structure has valid parameters.
 *	When a widget is created, its id is placed in the field "widget."
 * The routine returns the address of the new widget.
 *
 * During the creation process, the following should be understood.
 *
 *    * If the field "parent_ptr" is non-NULL, the parent's id will
 *	be taken from the given address.  Now, if the "descendant" key
 *	is not NULL, a GetValues call will be made to get the id
 *	of the descendant that is the actual destination of the widget
 *	to be added.
 *
 *    * If "parent_ptr" is NULL and the "class_ptr" does not evaluate to
 *	a ShellWidget subclass, an OlError is generated.
 *
 *    * If "class_ptr" evaluates to a ShellWidget subclass and "parent_ptr"
 *	is NULL, an application shell widget is created.
 *
 *    * If "class" evaluates to a ShellWidget subclass and "parent_ptr"
 *	is non-NULL, a popup shell widget is created as a child of the
 *	specified child.
 *
 ****************************procedure*header*****************************
 */
Widget
OlCreatePackedWidget(pw)
	register OlPackedWidget *pw;
{
	register WidgetClass wc;
	register Boolean     shell_widget = False;
	register Widget      parent       = (Widget) NULL;


	if (!pw->class_ptr || *pw->class_ptr == (WidgetClass) NULL)
		OlVaDisplayErrorMsg((Display *)NULL,
				    OleNfilePacked,
				    OleTmsg1,
				    OleCOlToolkitError,
				    OleMfilePacked_msg1);

				/* Is this to be a shell widget		*/

	for(wc = *pw->class_ptr; wc; wc = wc->core_class.superclass)
		if (wc == shellWidgetClass) {
			shell_widget = True;
			break;
		}

				/* Get the parent of the new widget	*/

	if ( pw->parent_ptr != (Widget *) NULL)
		parent = *pw->parent_ptr;

	if (parent != (Widget) NULL && pw->descendant != (String) NULL) {
		Arg	query_parent[1];
		Widget	real_parent = (Widget) NULL;
	
		XtSetArg(query_parent[0], pw->descendant, &real_parent);
		XtGetValues(parent, query_parent, 1);
		parent = real_parent;
	}

	if (shell_widget) {
		if (parent) 
			pw->widget = XtCreatePopupShell(pw->name,
						*pw->class_ptr,
						parent,
						pw->resources,
						pw->num_resources);
		else
				/* XtCreateApplicationShell uses
				 * the application class name passed to
				 * XtInitialize and uses NULL as application
				 * name. Now we replaced both with
				 * pw->name. If, we have complains
				 * from people, then we need to store
				 * application class name as a global.
				 *
				 * it only assumes display pointer is
				 * "toplevelDisplay".
				 */
			pw->widget = XtAppCreateShell(
						pw->name,
						pw->name,  /* class name */
						*pw->class_ptr,
						toplevelDisplay,
						pw->resources,
						pw->num_resources);
	}
	else {
	  if (parent)
	    pw->widget = XtCreateWidget(pw->name,
					*pw->class_ptr,
					parent,
					pw->resources,
					pw->num_resources);
	  else
	    OlVaDisplayErrorMsg((Display *)NULL,
				OleNbadParent,
				OleTnullParent,
				OleCOlToolkitError,
				OleMbadParent_nullParent,
				"Packed",
				"OlCreatePackedWidget",
				pw->name);
	}

	if (pw->managed)
		XtManageChild(pw->widget);

	return(pw->widget);
} /* END OF OlCreatePackedWidget() */

/*
 *************************************************************************
 * OlCreatePackedWidgetList - this routine creates many widgets using the
 * array of structures (i.e., of type OlPackedWidget).  The routine
 * makes sure that if the number of structures is more than zero, that
 * the array pointer is non-NULL.  The id of the first created widget
 * is returned.  If no widgets are created, NULL is returned.
 * The procedure OlCreatePackedWidget() is called to do the actual widget
 * creation.
 ****************************procedure*header*****************************
 */
Widget
OlCreatePackedWidgetList(pw_list, num_pw)
	OlPackedWidgetList pw_list;
	register Cardinal  num_pw;
{
	register int                 i;
	register OlPackedWidgetList  pwl = pw_list;
	register Widget              first_widget = (Widget) NULL;

	if (num_pw > 0 && !pwl)
		OlVaDisplayErrorMsg((Display *)NULL,
				    OleNfilePacked,
				    OleTmsg2,
				    OleCOlToolkitError,
				    OleMfilePacked_msg2);

	for (i = 0; i < num_pw; ++i, ++pwl)
		if (!first_widget)
			first_widget = OlCreatePackedWidget(pwl);
		else
			(void) OlCreatePackedWidget(pwl);

	return(first_widget);
} /* END OF OlCreatePackedWidgetList() */

