/*		copyright	"%c%" 	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/file.c	1.35"
#endif

#include <IntrinsicP.h>
#include <Xatom.h>
#include <CoreP.h>
#include <CompositeP.h>
#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Error.h>
#include <FButtons.h>
#include <PopupWindo.h>
#include <Caption.h>
#include <TextField.h>
#include <Notice.h>
#include <Gizmos.h>
#include "uucp.h"
#include "error.h"

char *disabled_device_path = "/etc/uucp/Devices.disabled";
char *incoming_device_path = "/etc/uucp/Devices.incoming";
extern char *		ApplicationName;
extern char *		system_path;
extern char *		device_path;

extern void		QuickDialCB();
extern Widget		AddMenu();
extern Boolean		IsSystemFile();
extern void		BringDownPopup();
extern void		DeleteSystemFile();
extern void		CreateSystemFile();
extern void		PutFlatItems();
extern void		DevicePopupCB();
extern void		GetFlatItems();
extern void		DeleteFlatItems();
extern void		GetContainerItems();
extern void		PutContainerItems();
extern void		DeleteContainerItems();
extern void		UnselectSelect();
extern void		InstallCB();

static void Quit();
static void QuitDevice();
static void Exit();

Arg arg[50];

static friend = 1;
static port = 2;

static Items fileItems[] = {
	{DevicePopupCB, NULL, (XA)TRUE},
	{Exit, NULL, (XA)TRUE},
};

static Menus fileButtonMenu = {
	"file",
	fileItems,
	XtNumber (fileItems),
	True,
	OL_FIXEDCOLS,
	OL_NONE,
	NULL
};

static Items dfileItems[] = {
	{QuickDialCB, NULL, (XA)TRUE},
	{Exit, NULL, (XA)TRUE},
};

static Menus dfileButtonMenu = {
	"file",
	dfileItems,
	XtNumber (dfileItems),
	True,
	OL_FIXEDCOLS,
	OL_NONE,
	NULL
};



Widget
AddFileMenu(wid)
Widget wid;
{
	Boolean sys = IsSystemFile(wid);
	if(sys) {
		/*fileItems[0].sensitive = sf->update;*/
		SET_LABEL (fileItems,0,devices);
		SET_LABEL (fileItems,1,exit);
		return AddMenu (wid, &fileButtonMenu, False);
	} else {
	if ((df->cp->num_objs>0 ) && (df->select_op != NULL))   {
		dfileItems[0].sensitive = True;
		} else {
		dfileItems[0].sensitive = False;
	}
		SET_LABEL (dfileItems,0,dial);
		SET_LABEL (dfileItems,1,exit);
		return AddMenu (wid, &dfileButtonMenu, False);
	}
} /* AddFileMenu */

void
SetDialSensitivity()
{

	int direction;
	char *port;
	char *enable;
	if ((df->cp->num_objs>0 ) && (df->select_op != NULL))   {
	/* need to check that Incoming Only is not set */
	/* if Incoming Only then we still grey the Dial button */
		port = ((DeviceData *)(df->select_op->objectdata))->portNumber;
		enable = ((DeviceData *)(df->select_op->objectdata))->portEnabled;
		if (( Get_ttymonPortDirection(port)) == INCOMING) {
			OlVaFlatSetValues(dfileButtonMenu.widget,
				0, XtNsensitive, (Boolean) False, 0);
		} else 
			/* if port is disabled grey the dial button */
		if ((enable) && ((strcmp(enable, "disabled")) == 0)) {
			OlVaFlatSetValues(dfileButtonMenu.widget,
				0, XtNsensitive, (Boolean) False, 0);
		} else {
		OlVaFlatSetValues(dfileButtonMenu.widget,
				0, XtNsensitive, (Boolean) True, 0);
		}

	} else {
		OlVaFlatSetValues(dfileButtonMenu.widget,
				0, XtNsensitive, (Boolean) False, 0);
		}
}


void
SaveSystemsFile()
{
	if (sf->filename[0] == '\0')
		PutFlatItems(system_path);
	else
		PutFlatItems (sf->filename);

}

void
SaveDeviceFile(port, oldport, action)
char * port;
char * oldport;
int action;
{
	if (df->incoming_filename[0] == '\0')
		PutContainerItems (incoming_device_path, DEVICES_INCOMING, port, oldport, action);
	else
		PutContainerItems (df->incoming_filename, DEVICES_INCOMING, port, oldport, action);
	if (df->filename[0] == '\0')
		PutContainerItems (device_path, DEVICES_OUTGOING, port, oldport, action);
	else
		PutContainerItems (df->filename, DEVICES_OUTGOING, port, oldport, action);
	if (df->disabled_filename[0] == '\0')
		PutContainerItems (disabled_device_path, DEVICES_DISABLED, port, oldport, action);
	else
		PutContainerItems (df->disabled_filename, DEVICES_DISABLED, port, oldport, action);
	/* need to call UpdateContainItems to write out
		any Reset lines needed in /etc/uucp/Devices */

} /* SaveDeviceFile */

static void
Exit(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	Boolean sys = IsSystemFile(wid);
#ifdef TRACE
	fprintf(stderr,"Exit \n");
#endif
	if (sys)
		Quit ();
	else
		QuitDevice();
} /* Exit */

void
QuitDevice()
{
#ifdef TRACE
	fprintf(stderr,"QuitDevice\n");
#endif
		if(df->propPopup &&
		   XtIsRealized(df->propPopup) &&
		   GetWMState(XtDisplay(df->propPopup),
			      XtWindow(df->propPopup)) != WithdrawnState)
			XtPopdown(df->propPopup);

		if(df->QDPopup &&
		   XtIsRealized(df->QDPopup) &&
		   GetWMState(XtDisplay(df->QDPopup),
			      XtWindow(df->QDPopup)) != WithdrawnState)
			XtPopdown(df->QDPopup);

		if(GetWMState(XtDisplay(df->toplevel),
		   XtWindow(df->toplevel)) != IconicState)
			XtUnmapWidget (df->toplevel);
		else
			XWithdrawWindow(XtDisplay(df->toplevel),
					XtWindow(df->toplevel),
					XScreenNumberOfScreen(XtScreen(df->toplevel)));
	XSync (DISPLAY, False);
} /* QuitDevice */

static void
Quit()
{
#ifdef TRACE
	fprintf(stderr,"Quit\n");
#endif
	FreeInstallList();
		/* system file */
	XtDestroyWidget (sf->toplevel);
#ifdef TRACE
	fprintf(stderr,"Quit after Xtdestroy\n");
#endif

	XtFree((char *)sf->popupMenuItems);
	if (sf->filename != NULL) {
		XtFree ((char *)sf->filename);
	}
#ifdef TRACE
	fprintf(stderr,"Quit before DeleteFlatItems\n");
#endif
	DeleteFlatItems ();
#ifdef TRACE
	fprintf(stderr,"Quit after DeleteFlatItems\n");
#endif
	XtFree ((char *)sf->flatItems);
	XtFree ((char *)sf);
	if (df->toplevel) {
			/* device file */
		XtDestroyWidget (df->toplevel);
#ifdef TRACE
	fprintf(stderr,"Quit after destroy sf->toplevel\n");
#endif
		DeleteContainerItems ();
		XtFree((char *)df->popupMenuItems->label);
		XtFree((char *)df->popupMenuItems);
		if (df->filename != NULL) {
			XtFree ((char *)df->filename);
		}
	}
	XtFree ((char *)df);
	XSync (DISPLAY, False);
	exit (0);
} /* Quit */

void
WindowManagerEventHandler(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	Boolean sys = IsSystemFile(wid);
	OlWMProtocolVerify *	p = (OlWMProtocolVerify *)call_data;

	switch (p->msgtype) {
	case OL_WM_DELETE_WINDOW:
#ifdef debug
		fprintf (stdout, "Delete yourself\n");
#endif
		if (sys)
			Quit ();
		else
			QuitDevice ();
		break;

	case OL_WM_SAVE_YOURSELF:
		/*
		 *	Do nothing for now; just respond.
		 */
#ifdef debug
		fprintf (stdout, "Save yourself\n");
#endif
		if (sys)
			Quit ();
		else
			QuitDevice ();
		break;

	default:
#ifdef debug
		fprintf (stdout, "Default action\n");
#endif
		OlWMProtocolAction(wid, p, OL_DEFAULTACTION);
		break;
	}
} /* WindowManagerEventHandler */
