#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/device.c	1.15"
#endif

/******************************file*header********************************

    Description:
	This file is the entry point for the routines
        to display the contents of the /etc/uucp/Devices file.
*/

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <X11/Shell.h>
#include <Xol/Form.h>
#include "uucp.h"
#include "error.h"


extern void callRegisterHelp(Widget, char * , char *);
extern char *	ApplicationName;
extern char *	Program;
extern Widget	InitButtons();
extern Widget	InitContainer();
extern void	InitFooter();
extern void	CreatePropertyWindow();
extern void	WindowManagerEventHandler();

static void initializeAllVisual();
char *device_path = "/etc/uucp/Devices";

void InitDevice();
/*
 * Global Variables
 */

Arg arg[50];

void
CreateDeviceFile(filename)
char *filename;
{
	df->select_op = (DmObjectPtr) NULL;
	df->cancelNotice = (Widget)0;
	df->filename = NULL;
    df->propPopup = NULL;
    df->quitNotice = NULL;
    df->cancelNotice = NULL;
	df->saveFilename = NULL;
	df->w_acu = NULL;
	df->openNotice = (Widget)NULL;

	if (filename != NULL) {
		df->filename = strdup(filename);
	}
	initializeAllVisual();
} /* CreateDeviceFile */

void
DevicePopupCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	static Boolean first_time = True;

	if (first_time) {
		XtPopup(sf->devicePopup, XtGrabNone);
		first_time = False;
	} else {
		ClearFooter(df->footer);
		XtMapWidget(df->toplevel);
	}
	XRaiseWindow(DISPLAY, XtWindow(sf->devicePopup));
	callRegisterHelp(df->toplevel, title_setup, help_device);
}

void
InitDevice()
{
	char text[1024];
	Pixmap	icon,
		iconmask;


	XtVaGetValues(sf->toplevel,
			XtNiconPixmap, &icon,
			XtNiconMask, &iconmask,
			0
		);
	sprintf (text, "%s: %s", ApplicationName, GGT(title_devices));
	XtSetArg (arg[0], XtNtitle, text);
	XtSetArg(arg[1], XtNheight, BNU_HEIGHT);
	XtSetArg(arg[2], XtNwidth, BNU_WIDTH);
	XtSetArg(arg[3], XtNiconPixmap, icon);
	XtSetArg(arg[4], XtNiconMask, iconmask);
	XtSetArg(arg[5], XtNiconName, ApplicationName);
	XtSetArg(arg[6], XtNwindowGroup, XtWindow(sf->toplevel));
	df->toplevel = XtAppCreateShell(
		Program,
		ApplicationName,
		topLevelShellWidgetClass,
		XtDisplay(sf->toplevel),
		arg, 7
	);

	XtVaSetValues (
		df->toplevel,
		XtNwmProtocolInterested,
		OL_WM_DELETE_WINDOW,
		0
	);
	OlAddCallback (
		df->toplevel,
		XtNwmProtocol, WindowManagerEventHandler,
		(XtPointer) 0
	);
	sf->devicePopup = df->toplevel;
	CreateDeviceFile(device_path);
	callRegisterHelp(df->toplevel, title_setup, help_device);
} /* InitDevice */

void
initializeAllVisual()
{
	Widget form;
        Widget topButtons;

	form = XtVaCreateManagedWidget(
		"form",
		formWidgetClass,
		df->toplevel,
		(String)0
	);
    topButtons = InitButtons (form);
    InitFooter (form);
    InitContainer (form, topButtons);
	CreatePropertyWindow(form);
} /* initializeAllVisual */
