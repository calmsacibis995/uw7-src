#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/property.c	1.13"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <Shell.h>
#include <StaticText.h>
#include "error.h"
#include "uucp.h"

extern void callRegisterHelp(Widget, char *, char *);
extern char *ApplicationName;
void
PropPopupCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{

	Arg args[1];
    char buf[BUFSIZ];
#ifdef TRACE
	fprintf(stderr,"PropPopupCB\n");
#endif
	if (!new) {
    	sprintf(buf, "%s: %s", ApplicationName, GGT(title_properties));
		XtVaSetValues(sf->propPopup, XtNtitle, buf, NULL);
    	SetPropertyAddLabel(0, label_ok, mnemonic_ok);
	} else  {
    	sprintf(buf, "%s: %s", ApplicationName, GGT(title_addSys));
    	XtVaSetValues(sf->propPopup, XtNtitle, buf, NULL);
    	SetPropertyAddLabel(0, label_add, mnemonic_add);
	}
	callRegisterHelp(wid, title_properties, help_property);
	if (!new && sf->numFlatItems == 0) { /* nothing to operate */
		return;
	}
	/* select the current entry */
	if (!new && sf->numFlatItems)
		ResetFields(sf->flatItems[sf->currentItem].pField);
	else
		ResetFields(new->pField);
	
	ClearLeftFooter(sf->footer);

		/* always popup the first category page */
	SetValue(sf->propPopup, XtNfocusWidget, (Widget)sf->w_name);
	XtPopup(sf->propPopup, XtGrabNone);
	callRegisterHelp(sf->propPopup, title_property, help_property);
	XRaiseWindow(DISPLAY, XtWindow(sf->propPopup));
} /* PropPopupCB */
