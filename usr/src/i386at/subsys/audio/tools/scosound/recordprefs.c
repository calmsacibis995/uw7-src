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

  SCCS  : @(#)recordprefs.c	5.1	97/06/25
  Author: Shawn McMurdo (shawnm@sco.com)
  Date  : 29-Apr-94
  File  : recordprefs.c

  Description:
	This file contains routines to handle Options -> Recording Preferences.

  Modification History:
  S000,	08-Nov-96, shawnm
	created
  -----------------------------------------------------------------------*/

#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <Xm/TextF.h>
#include <Xm/PushBG.h>

#include "scosound.h"
#include "error.h"

void RecordingPreferencesCB(Widget, XtPointer, XtPointer);

static void recordingPreferencesOKCB(Widget, XtPointer, XtPointer);
static void recordingPreferencesCancelCB(Widget, XtPointer, XtPointer);

extern Widget top;
extern ScoSoundResources res;
extern ErrorResources errorRes;

static Widget recordingPreferences_d = (Widget)NULL;
static Widget fileFormat_om, dataFormat_om, sampleRate_om, channels_om;


static void
createRecordingPreferencesDialog()
{
	Arg args[16];
	int i, n, numsroptions;
	Widget recordingPreferences_form;
	Widget column1;
	Widget fileFormat_oml, dataFormat_oml, sampleRate_oml, channels_oml;
	Widget fileFormat_pd, dataFormat_pd, sampleRate_pd, channels_pd;
	Widget curW, tmpW;
	char numstr[8];

	n = 0;
	recordingPreferences_d = (Widget)XmCreateMessageDialog(top,
		"Recording Preferences", args, n);
	XtAddCallback(recordingPreferences_d, XmNokCallback,
		recordingPreferencesOKCB, NULL);
	XtAddCallback(recordingPreferences_d, XmNcancelCallback,
		recordingPreferencesCancelCB, NULL);

	n = 0;
	recordingPreferences_form =
		XtCreateManagedWidget("recordingPreferences_form",
		xmFormWidgetClass, recordingPreferences_d, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);	n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);	n++;
	XtSetArg(args[n], XmNorientation, XmVERTICAL);	n++;
	XtSetArg(args[n], XmNpacking, XmPACK_COLUMN);	n++;
	XtSetArg(args[n], XmNnumColumns, 2);	n++;
	column1 = XtCreateManagedWidget("column1", xmRowColumnWidgetClass,
		recordingPreferences_form, args, n);

	n = 0;
	fileFormat_oml = XtCreateManagedWidget("File format",
		 xmLabelWidgetClass, column1, args, n);

	n = 0;
	dataFormat_oml = XtCreateManagedWidget("Data format",
		 xmLabelWidgetClass, column1, args, n);

	n = 0;
	sampleRate_oml = XtCreateManagedWidget("Sampling rate",
		 xmLabelWidgetClass, column1, args, n);

	n = 0;
	channels_oml = XtCreateManagedWidget("Channels",
		 xmLabelWidgetClass, column1, args, n);

	n = 0;
	fileFormat_pd = XmCreatePulldownMenu(recordingPreferences_form,
		 "fileFormat_pd", args, n);

	for (i = 0; i < NUM_FILE_FORMATS; i++)
	{
		n - 0;
		XtSetArg(args[n], XmNlabelString,
			XmStringCreateSimple(fileFormat[i].name)); n++;
		tmpW = XtCreateManagedWidget("fileformatpb",
			xmPushButtonGadgetClass, fileFormat_pd, args, n);
		if ((i == 0) ||
			(res.file_format == i))
		{
			curW = tmpW;
		}
	}

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);	n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);	n++;
	XtSetArg(args[n], XmNmenuHistory, curW);	n++;
	XtSetArg(args[n], XmNsubMenuId, fileFormat_pd);	n++;
	fileFormat_om = XmCreateOptionMenu(column1,
		 "fileFormat_om", args, n);
	XtManageChild(fileFormat_om);

	n = 0;
	dataFormat_pd = XmCreatePulldownMenu(recordingPreferences_form,
		 "dataFormat_pd", args, n);

	for (i = 0; i < NUM_DATA_FORMATS; i++)
	{
		n - 0;
		XtSetArg(args[n], XmNlabelString,
			XmStringCreateSimple(dataFormat[i].name)); n++;
		tmpW = XtCreateManagedWidget("dataFormat_b",
			xmPushButtonGadgetClass, dataFormat_pd, args, n);
		if (i == 0) /* XXX */
		{
			curW = tmpW;
		}
	}

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget, fileFormat_om);	n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);	n++;
	XtSetArg(args[n], XmNmenuHistory, curW);	n++;
	XtSetArg(args[n], XmNsubMenuId, dataFormat_pd);	n++;
	dataFormat_om = XmCreateOptionMenu(column1,
		 "dataFormat_om", args, n);
	XtManageChild(dataFormat_om);

	n = 0;
	sampleRate_pd = XmCreatePulldownMenu(recordingPreferences_form,
		 "sampleRate_pd", args, n);

	for (i = 0; i < NUM_SAMPLE_RATES; i++)
	{
		n - 0;
		XtSetArg(args[n], XmNlabelString,
			XmStringCreateSimple(sampleRate[i].name)); n++;
		tmpW = XtCreateManagedWidget("sampleRate_b",
			xmPushButtonGadgetClass, sampleRate_pd, args, n);
		if (i == 0) /* XXX */
		{
			curW = tmpW;
		}
	}

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget, dataFormat_om);	n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);	n++;
	XtSetArg(args[n], XmNmenuHistory, curW);	n++;
	XtSetArg(args[n], XmNsubMenuId, sampleRate_pd);	n++;
	sampleRate_om = XmCreateOptionMenu(column1,
		 "sampleRate_om", args, n);
	XtManageChild(sampleRate_om);

	n = 0;
	channels_pd = XmCreatePulldownMenu(recordingPreferences_form,
		 "channels_pd", args, n);

	n - 0;
	XtSetArg(args[n], XmNlabelString,
		XmStringCreateSimple("1")); n++;
	XtCreateManagedWidget("channels_pb", xmPushButtonGadgetClass,
		channels_pd, args, n);

	n - 0;
	XtSetArg(args[n], XmNlabelString,
		XmStringCreateSimple("2")); n++;
	XtCreateManagedWidget("channels_pb", xmPushButtonGadgetClass,
		channels_pd, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget, sampleRate_om);	n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);	n++;
	XtSetArg(args[n], XmNsubMenuId, channels_pd);	n++;
	channels_om = XmCreateOptionMenu(column1,
		 "channels_om", args, n);
	XtManageChild(channels_om);
}


static void
recordingPreferencesOKCB(w, client, call)
Widget w;
XtPointer client;
XtPointer call;
{
	char *tmpStr;
	XmString tmpXmStr;
	Widget tmpW;

	XtVaGetValues(fileFormat_om,
		XmNmenuHistory, &tmpW,
		NULL);
	XtVaGetValues(tmpW,
		XmNlabelString, &tmpXmStr,
		NULL);
	XmStringGetLtoR(tmpXmStr, XmFONTLIST_DEFAULT_TAG, &tmpStr);
	res.file_format = 0; /* XXX SoundStringToFileFormat(tmpStr) */
	XtFree((char *)tmpXmStr);
	XtFree(tmpStr);

	XtVaGetValues(dataFormat_om,
		XmNmenuHistory, &tmpW,
		NULL);
	XtVaGetValues(tmpW,
		XmNlabelString, &tmpXmStr,
		NULL);
	XmStringGetLtoR(tmpXmStr, XmFONTLIST_DEFAULT_TAG, &tmpStr);
	res.data_format = 0; /* XXX */
	XtFree((char *)tmpXmStr);
	XtFree(tmpStr);

	strcpy(tmpStr, "XXX sampleratecb");
	res.sample_rate = atoi(tmpStr);
	XtFree(tmpStr);
}


static void
recordingPreferencesCancelCB(w, client, call)
Widget w;
XtPointer client;
XtPointer call;
{
}


void
RecordingPreferencesCB(w, client, call)
Widget w;
XtPointer client;
XtPointer call;
{
	if (recordingPreferences_d == NULL)
		createRecordingPreferencesDialog();
	XtManageChild(recordingPreferences_d);
}

