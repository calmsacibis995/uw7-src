/*-------------------------------------------------------------------------
  Copyright (c) 1994-1997      		The Santa Cruz Operation, Inc.
  -------------------------------------------------------------------------
  All rights reserved.  No part of this  program or publication may be
  reproduced, transmitted, transcribed, stored  in a retrieval system,
  or translated into any language or computer language, in any form or
  by any  means,  electronic, mechanical, magnetic, optical, chemical,
  biological, or otherwise, without the  prior written  permission of:

           The Santa Cruz Operation, Inc.  (408) 425-7222
           400 Encinal St, Santa Cruz, CA  95060 USA
  -------------------------------------------------------------------------

  SCCS  : @(#)menu.c	5.1 97/06/25
  Author: Shawn McMurdo (shawnm@sco.com)
  Date  : 29-Apr-94
  File  : menu.c

  Description:
	This file contains routines to build the menubar,
	and callbacks for the menubar widgets.

  Modification History:
  S005,	15-Nov-96, shawnm
	remove Options->DeviceSelection, add File->New
  S004,	01-Nov-96, shawnm
	rewrite for scosound
  S003,	09-Sep-96, shawnm
	rewrite for scosound
  S002,	02-Nov-94, shawnm
	fix Save and Save As
  S001,	04-Jul-94, shawnm
	change View to Zoom
  S000,	29-Apr-94, shawnm
	created
  -----------------------------------------------------------------------*/

/* 'is          (Include Files: System)                                  */
#include <stdio.h>
#include <pwd.h>
#include <sys/soundcard.h>

/* 'ix          (Include Files: X)                                       */
#include <Xm/Xm.h>
#include <Xm/CascadeB.h>
#include <Xm/RowColumn.h>

/* 'il          (Include Files: Local)                                   */
#include "scosound.h"

/* 'de          (Defines)                                                */

/* 'st          (Structures)                                             */

/* 'fe          (Function Prototypes: External)                          */
extern void SetShortFileName();
extern void UpdateInfoPanel();
extern void RecordingPreferencesCB(Widget, XtPointer, XtPointer);

/* 'ff          (Function Prototypes: Public)                           */
Widget BuildMenuBar(Widget);

/* 'ff          (Function Prototypes: Static)                           */
static void createFileNewDialog();
static void fileNewOKCB(Widget, XtPointer, XmFileSelectionBoxCallbackStruct*);
static void fileNewCancelCB(Widget, XtPointer, XmFileSelectionBoxCallbackStruct*);
static void createFileOpenDialog();
static void fileOpenOKCB(Widget, XtPointer, XmFileSelectionBoxCallbackStruct*);
static void fileOpenCancelCB(Widget, XtPointer, XmFileSelectionBoxCallbackStruct*);
static void createFileCopyDialog();
static void fileCopyOKCB(Widget, XtPointer, XmFileSelectionBoxCallbackStruct*);
static void fileCopyCancelCB(Widget, XtPointer, XmFileSelectionBoxCallbackStruct*);
static void fileNewCB(Widget, XtPointer, XtPointer);
static void fileOpenCB(Widget, XtPointer, XtPointer);
static void fileCopyCB(Widget, XtPointer, XtPointer);
static void fileExitCB(Widget, XtPointer, XtPointer);

/* 've          (Variables: External)                                    */
extern Widget top;
extern ScoSoundResources res;

/* 'vs          (Variables: Public)                                      */

/* 'vs          (Variables: Static)                                      */
static Widget fileNew_d = (Widget)NULL;
static Widget fileOpen_d = (Widget)NULL;
static Widget fileCopy_d = (Widget)NULL;

/* 'fp          (Functions: Public)                                      */

Widget
BuildMenuBar(parent)
Widget	parent;
{
	int	n;
	Arg	args[10];
	Widget	menubar;
	Widget	file_cb, file_m, new_b, open_b, copy_b, exit_b;
	Widget	options_cb, options_m, recordingPreferences_b;

	n = 0;
	menubar = XmCreateMenuBar(parent, "menubar", NULL, 0);

	file_cb = XmCreateCascadeButton(menubar, "File", NULL, 0);

	file_m = XmCreatePulldownMenu(menubar, "file_m", NULL, 0);
	n = 0;
	XtSetArg(args[n], XmNsubMenuId, file_m); n++;
	XtSetValues(file_cb, args, n);

	new_b = XmCreateCascadeButton(file_m, "New", NULL, 0);
        XtAddCallback(new_b, XmNactivateCallback, fileNewCB, NULL);

	open_b = XmCreateCascadeButton(file_m, "Open", NULL, 0);
        XtAddCallback(open_b, XmNactivateCallback, fileOpenCB, NULL);

	copy_b = XmCreateCascadeButton(file_m, "Copy", NULL, 0);
        XtAddCallback(copy_b, XmNactivateCallback, fileCopyCB, NULL);

	exit_b = XmCreateCascadeButton(file_m, "Exit", NULL, 0);
        XtAddCallback(exit_b, XmNactivateCallback, fileExitCB, NULL);

	options_cb = XmCreateCascadeButton(menubar, "Options", NULL, 0);

	options_m = XmCreatePulldownMenu(menubar, "options_m", NULL, 0);
	n = 0;
	XtSetArg(args[n], XmNsubMenuId, options_m); n++;
	XtSetValues(options_cb, args, n);

	recordingPreferences_b = XmCreateCascadeButton(options_m,
		 "Rec. Preferences", NULL, 0);
        XtAddCallback(recordingPreferences_b, XmNactivateCallback,
		 RecordingPreferencesCB, NULL);

	/* XXX ScoHelpMenuInit(menubar, NULL,
		ON_VERSION|ON_CONTEXT|ON_WINDOW|ON_KEYS|INDEX|ON_HELP);
	*/
	XtManageChild(new_b);
	XtManageChild(open_b);
	XtManageChild(copy_b);
	XtManageChild(exit_b);
	XtManageChild(file_cb);
	XtManageChild(recordingPreferences_b);
	XtManageChild(options_cb);
	XtManageChild(menubar);
	return(menubar);
}


static void
createFileNewDialog()
{
	Arg args[4];
	int n;
	Widget w;

	n = 0;
	XtSetArg(args[n], XmNautoUnmanage, FALSE); n++;
	fileNew_d = (Widget)XmCreateFileSelectionDialog(top, "fileNew_d",
		args, n);
	XtAddCallback(fileNew_d, XmNokCallback, (XtCallbackProc)fileNewOKCB,
		NULL);
	XtAddCallback(fileNew_d, XmNcancelCallback,
		(XtCallbackProc)fileNewCancelCB, NULL);
}


static void
fileNewOKCB(w, client, call)
Widget w;
XtPointer client;
XmFileSelectionBoxCallbackStruct *call;
{
	char *filename;
	Arg args[2];
	int n;

	if (XmStringGetLtoR(call->value, XmSTRING_DEFAULT_CHARSET, &filename))
	{
		if (res.file_name != NULL)
			XtFree(res.file_name);
		res.file_name = XtNewString(filename);
		SetShortFileName();
		UpdateInfoPanel();
	}
	XtFree(filename);
	XtUnmanageChild(w);
}


static void
fileNewCancelCB(w, client, call)
Widget w;
XtPointer client;
XmFileSelectionBoxCallbackStruct *call;
{
	XtUnmanageChild(w);
}


static void
fileNewCB(w, client, call)
Widget w;
XtPointer client;
XtPointer call;
{
	if (fileNew_d == (Widget)NULL)
		createFileNewDialog();
	XtManageChild(fileNew_d);
}


static void
createFileOpenDialog()
{
	Arg args[4];
	int n;

	n = 0;
	XtSetArg(args[n], XmNautoUnmanage, FALSE); n++;
	fileOpen_d = (Widget)XmCreateFileSelectionDialog(top, "fileOpen_d",
		 args, n);
	XtAddCallback(fileOpen_d, XmNokCallback, (XtCallbackProc)fileOpenOKCB,
		NULL);
	XtAddCallback(fileOpen_d, XmNcancelCallback,
		(XtCallbackProc)fileOpenCancelCB, NULL);
}


static void
fileOpenOKCB(w, client, call)
Widget w;
XtPointer client;
XmFileSelectionBoxCallbackStruct *call;
{
	char *filename;
	Arg args[2];
	int n;

	if (XmStringGetLtoR(call->value, XmSTRING_DEFAULT_CHARSET, &filename))
	{
		if (res.file_name != NULL)
			XtFree(res.file_name);
		res.file_name = XtNewString(filename);
		SetShortFileName();
		UpdateInfoPanel();
	}
	XtFree(filename);
	XtUnmanageChild(w);
}


static void
fileOpenCancelCB(w, client, call)
Widget w;
XtPointer client;
XmFileSelectionBoxCallbackStruct *call;
{
	XtUnmanageChild(w);
}


static void
fileOpenCB(w, client, call)
Widget w;
XtPointer client;
XtPointer call;
{
	if (fileOpen_d == (Widget)NULL)
		createFileOpenDialog();
	XtManageChild(fileOpen_d);
}


static void
createFileCopyDialog()
{
	Arg args[4];
	int n;

	n = 0;
	XtSetArg(args[n], XmNautoUnmanage, FALSE); n++;
	fileCopy_d = (Widget)XmCreateFileSelectionDialog(top, "fileCopy_d",
		 args, n);
	XtAddCallback(fileCopy_d, XmNokCallback, (XtCallbackProc)fileCopyOKCB,
		NULL);
	XtAddCallback(fileCopy_d, XmNcancelCallback,
		(XtCallbackProc)fileCopyCancelCB, NULL);
}


static void
fileCopyOKCB(w, client, call)
Widget w;
XtPointer client;
XmFileSelectionBoxCallbackStruct *call;
{
	char *filename;
	Arg args[2];
	int n;

	if (XmStringGetLtoR(call->value, XmSTRING_DEFAULT_CHARSET, &filename))
	{
		if (res.file_name != NULL)
		{
			/* XXX copy old to new */
			XtFree(res.file_name);
		}
		res.file_name = XtNewString(filename);
		SetShortFileName();
		UpdateInfoPanel();
	}
	XtFree(filename);
	XtUnmanageChild(w);
}


static void
fileCopyCancelCB(w, client, call)
Widget w;
XtPointer client;
XmFileSelectionBoxCallbackStruct *call;
{
	XtUnmanageChild(w);
}


static void
fileCopyCB(w, client, call)
Widget w;
XtPointer client;
XtPointer call;
{
	if (fileCopy_d == (Widget)NULL)
		createFileCopyDialog();
	XtManageChild(fileCopy_d);
}


static void
fileExitCB(w, client, call)
Widget w;
XtPointer client;
XtPointer call;
{
	/* XXX ScoHelpClose(); */
	exit(1);
}

