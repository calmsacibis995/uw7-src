#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/caption.c	1.1"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Caption.h>

Widget CreateCaption(w_parent, label)
Widget	w_parent;
String	label;
{
	Widget		w_caption;

	/*
	 * Create the Caption for the text field
	 */
	w_caption = 
		XtVaCreateManagedWidget(
			"fieldLabel",
			captionWidgetClass,
			w_parent,
			XtNlabel,		label,
			(String) 0
		);

	return(w_caption);
}
