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

  SCCS  : @(#)scosound.c	2.1	97/01/30
  Author: Shawn McMurdo (shawnm@sco.com)
  Date  : 28-Apr-94, rewritten 28-Aug-96
  File  : scosound.c

  Description:
	This file contains initialization routines.

  Modification History:
  S005,	06-Nov-96, shawnm
	more scosound rewrites
  S004,	28-Aug-96, shawnm
	rewritten, add OSS support, remove NAS support
  S003,	08-May-95, shawnm
	fixed some warnings
  S002,	02-Nov-94, shawnm
	fix comment, make error messages i18nable
  S001,	05-Jul-94, shawnm
	fix record dialog and graph display
  S000,	28-Apr-94, shawnm
	created
  -----------------------------------------------------------------------*/

#define SCOSOUND_C

/* 'is          (Include Files: System)                                  */
#include <stdio.h>
#if defined(sco)
#include <sys/itimer.h>
#endif
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/procset.h>
#include <sys/fcntl.h>

#include <sys/soundcard.h>

#include <X11/Xatom.h>
#include <Xm/Xm.h>
#include <Xm/Protocols.h>
#include <Xm/AtomMgr.h>
#include <Xm/Form.h>
#include <Xm/Scale.h>
#include <Xm/MessageB.h>
#include <Xm/FileSB.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/CascadeB.h>
#include <Xm/SeparatoG.h>
#include <Xm/List.h>
#include <Xm/Frame.h>
#include <Xm/TextF.h>

#include "scosound.h"
#include "converters.h"
#include "error.h"

#include "bitmaps/play_down.xbm"
#include "bitmaps/play_up.xbm"
#include "bitmaps/stop_down.xbm"
#include "bitmaps/stop_up.xbm"
#include "bitmaps/pause_down.xbm"
#include "bitmaps/pause_up.xbm"
#include "bitmaps/record_down.xbm"
#include "bitmaps/record_up.xbm"

void QuitCB(Widget, XtPointer, XtPointer);
void SaveYourselfCB(Widget, XtPointer, XtPointer);
void playCB(Widget, XtPointer, XtPointer);
void stopCB(Widget, XtPointer, XtPointer);
void pauseCB(Widget, XtPointer, XtPointer);
void recordCB(Widget, XtPointer, XtPointer);
void recordOptionsCB(Widget, XtPointer, XtPointer);
void recordOptionsOKCB(Widget, XtPointer, XtPointer);
void recordOptionsCancelCB(Widget, XtPointer, XtPointer);
void fileFormatCB(Widget, caddr_t, XmAnyCallbackStruct *);
void dataFormatCB(Widget, caddr_t, XmAnyCallbackStruct *);
void sampleRateCB(Widget, caddr_t, XmAnyCallbackStruct *);
void channelsCB(Widget, caddr_t, XmAnyCallbackStruct *);
void syserr();
static void childExited(int);
XtArgVal GetPixel(char *);
void CreateControlPanelPixmaps();
Widget BuildControlPanel(Widget);
void UpdateControlPanel();
Widget BuildInfoPanel(Widget);
void UpdateInfoPanel();
void SetShortFileName();

extern Widget BuildMenuBar(Widget);

int audioDevice_fd;
int soundFile_fd;
XtAppContext app;
Display *display;
int screen;
ScoSoundResources res;
ErrorResources errorRes;
Boolean paused = False;
Boolean playing = False;
Boolean recording = False;
pid_t child = 0;
char *newargv[64];
Widget top;
Widget fileName_v, fileFormat_v, dataFormat_v, sampleRate_v, channels_v;
Widget stop_b, play_b, pause_b, record_b;

Widget recordOptions_d = (Widget)NULL;
Pixmap stopUpPix, playUpPix, pauseUpPix, recordUpPix;
Pixmap stopDownPix, playDownPix, pauseDownPix, recordDownPix;

static Widget menuBar, infoPanel, controlPanel;
static Atom     xa_WM_SAVE_YOURSELF;
static Atom     xa_WM_DELETE_WINDOW;

#define Offset(field) XtOffset(ScoSoundResources *, field)
static XtResource appResources[] = {
	{"audioDevice", "AudioDevice", XtRString, sizeof(String),
		Offset(audio_device), XtRString,
		(caddr_t)DEFAULT_AUDIO_DEVICE},
	{"soundFileName", "SoundFileName", XtRString, sizeof(String),
		Offset(file_name), XtRString,
		(caddr_t)DEFAULT_FILE_NAME},
	{"nullFileName", "NullFileName", XtRString, sizeof(String),
		Offset(null_file_name), XtRString,
		(caddr_t)DEFAULT_NULL_FILE_NAME},
	{"soundFileFormat", "SoundFileFormat", XtRSoundFileFormat,
		sizeof(int), Offset(file_format), XtRImmediate,
		(caddr_t)DEFAULT_FILE_FORMAT},
	{"soundDataFormat", "SoundDataFormat", XtRSoundDataFormat,
		sizeof(int), Offset(data_format), XtRImmediate,
		(caddr_t)DEFAULT_DATA_FORMAT},
	{"sampleRate", "SampleRate", XmRInt, sizeof(int),
		Offset(sample_rate), XmRImmediate,
		(caddr_t)DEFAULT_SAMPLE_RATE},
	{"channels", "Channels", XmRInt, sizeof(int),
		Offset(channels), XmRImmediate,
		(caddr_t)DEFAULT_CHANNELS},
	{"autoPlay", "AutoPlay", XmRBoolean, sizeof(Boolean),
		Offset(auto_play), XmRImmediate,
		(caddr_t)DEFAULT_AUTO_PLAY},
};

#undef Offset

static XrmOptionDescRec options[] = {
        {"-audiodevice",	"*audioDevice",	XrmoptionSepArg, NULL},
        {"-device",		"*audioDevice",	XrmoptionSepArg, NULL},
        {"-soundfileformat",	"*soundFileFormat",	XrmoptionSepArg, NULL},
        {"-fileformat",		"*soundFileFormat",	XrmoptionSepArg, NULL},
        {"-file",		"*soundFileFormat",	XrmoptionSepArg, NULL},
        {"-sounddataformat",	"*soundDataFormat",	XrmoptionSepArg, NULL},
        {"-dataformat",		"*soundDataFormat",	XrmoptionSepArg, NULL},
        {"-data",		"*soundDataFormat",	XrmoptionSepArg, NULL},
        {"-samplerate",		"*sampleRate",	XrmoptionSepArg, NULL},
        {"-rate",		"*sampleRate",	XrmoptionSepArg, NULL},
        {"-channels",		"*channels",	XrmoptionSepArg, NULL},
        {"-mono",		"*channels",	XrmoptionNoArg, (char *)1},
        {"-stereo",		"*channels",	XrmoptionNoArg, (char *)2},
        {"-autoplay",		"*autoPlay",	XrmoptionNoArg, "True"},
};

main(argc, argv)
int argc;
char *argv[];
{
	Widget main_form;
	Arg args[16];
	int i, n;
	Atom protocols[2];
        struct sigaction act;

	XtSetLanguageProc(0, 0, 0);
	top = XtAppInitialize(&app, "ScoSound", options, XtNumber(options),
		 &argc, argv, NULL, NULL, 0);

	RegisterConverters(app);

        display = XtDisplay(top);
        screen = DefaultScreen(display);

	xa_WM_SAVE_YOURSELF = XmInternAtom(display, "WM_SAVE_YOURSELF", FALSE);
	xa_WM_DELETE_WINDOW = XmInternAtom(display, "WM_DELETE_WINDOW", FALSE);
	protocols[0] = xa_WM_SAVE_YOURSELF;
	protocols[1] = xa_WM_DELETE_WINDOW;
	XmAddWMProtocols(top, protocols, 2);
	XmAddWMProtocolCallback(top, xa_WM_SAVE_YOURSELF,
		SaveYourselfCB, NULL);
	XmAddWMProtocolCallback(top, xa_WM_DELETE_WINDOW,
		QuitCB, NULL);

	/* XXX call ScoHelpInit */
	/* XXX merge resources from server/preferences file/etc? */

	XtGetApplicationResources(top, &errorRes, errorResources,
		XtNumber(errorResources), NULL, 0);


        XtGetApplicationResources(top, &res, appResources,
		XtNumber(appResources), NULL, 0);

	/* get file name */
	if (argc > 1)
	{
		res.file_name = XtNewString(argv[1]);
	}
	SetShortFileName();

	n = 0;
	XtSetArg(args[n], XmNallowShellResize, True);	n++;
	XtSetValues(top, args, n);

	n = 0;
	main_form = XtCreateManagedWidget("main_form", xmFormWidgetClass,
		top, args, n);

	menuBar = BuildMenuBar(main_form);
	XtVaSetValues(menuBar,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	infoPanel = BuildInfoPanel(main_form);
	XtVaSetValues(infoPanel,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, menuBar,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	controlPanel = BuildControlPanel(main_form);
	XtVaSetValues(controlPanel,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtVaSetValues(infoPanel,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, controlPanel,
		NULL);

	UpdateInfoPanel();

	XtRealizeWidget(top);

        act.sa_handler = childExited;
        act.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT;
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGCHLD);
        if (sigaction(SIGCHLD, &act, (struct sigaction *)NULL) == -1)
        {
                syserr("Sigaction SIGCHLD failed");
        }

	if ((res.file_name != NULL) && res.auto_play)
	{
		playCB(NULL, NULL, NULL);
	}

	XtAppMainLoop(app);
}


void
syserr(msg)
char *msg;
{
	perror(msg);
	exit(-1);
}


static void
childExited(int sig)
{
	child = 0;
	playing = False;
	recording = False;
	paused = False;
	UpdateControlPanel();
}


static void
childStop()
{
	if (child > 1)
	{
#if defined(sco)
		kill(child, SIGTERM);
#else
		sigsend(P_PID, child, SIGTERM);
#endif
	}
}


static void
childPause()
{
	if (child > 1)
	{
#if defined(sco)
		kill(child, SIGSTOP);
#else
		sigsend(P_PID, child, SIGSTOP);
#endif
	}
}


static void
childContinue()
{
	if (child > 1)
	{
#if defined(sco)
		kill(child, SIGCONT);
#else
		sigsend(P_PID, child, SIGCONT);
#endif
	}
}


int
OpenSoundFile(char *soundFileName)
{
	if (soundFileName)
	{
		if ((soundFile_fd = open(soundFileName, O_RDWR, 0)) == -1)
		{
			/* XXX handle can't open error */
		}
		else
		{
			res.file_name = XtNewString(soundFileName);
			SetShortFileName();
		}
	}
	else
	{
		soundFile_fd = 0;
		/* no file to open */
	}

	/* read from file */
	res.file_format = 0;
	res.data_format = 0;
	res.sample_rate = 0;
	res.channels = 0;
}


void
QuitCB(w, client, call)
Widget w;
XtPointer client;
XtPointer call;
{
	exit(0);
}


void
playCB(w, client, call)
Widget w;
XtPointer client;
XtPointer call;
{
	if (playing || recording)
		return;

	/* XXX play file */
	child = fork();
	switch (child)
	{
	case -1:
		syserr("Fork failed.");
	case 0:
		newargv[0] = XtNewString("vplay");
		newargv[1] = XtNewString(res.file_name);
		newargv[2] = NULL;
		execvp("/usr/bin/vplay", newargv);
		syserr("Execvp failed.");
	}
	
	playing = True;
	UpdateControlPanel();
}


void
stopCB(w, client, call)
Widget w;
XtPointer client;
XtPointer call;
{
	if (w)
	{
		if (playing)
		{
			/* XXX stop */
			childStop();
		}
		else if (recording)
		{
			/* XXX stop */
			childStop();
		}
	}
	if (recording)
		UpdateInfoPanel();
	playing = False;
	recording = False;
	paused = False;
	UpdateControlPanel();
}


void
pauseCB(w, client, call)
Widget w;
XtPointer client;
XtPointer call;
{
	if (paused)
	{
		paused = False;
		if (playing)
		{
			/* XXX continue */
			childContinue();
		}
		else if (recording)
		{
			/* XXX continue */
			childContinue();
		}
	}
	else
	{
		if (playing || recording)
		{
			paused = True;
			if (playing)
			{
				/* XXX pause */
				childPause();
			}
			else if (recording)
			{
				/* XXX pause */
				childPause();
			}
		}
	}
	UpdateControlPanel();
}


void
recordCB(w, client, call)
Widget w;
XtPointer client;
XtPointer call;
{
	unsigned long	rate;

/* XXX check that we are open for recording */

	if (playing)
	{
		/* XXX stop */
		childStop();
		playing = False;
	}

	if (recording)
	{
		if (w)
			/* XXX stop */
			childStop();
		else
			recording = False;
		return;
	}

	/* XXX play file */
	child = fork();
	switch (child)
	{
	case -1:
		syserr("Fork failed.");
	case 0:
		newargv[0] = XtNewString("vrec");
		newargv[1] = XtNewString(res.file_name);
		newargv[2] = NULL;
		execvp("/usr/bin/vrec", newargv);
		syserr("Execvp failed.");
	}

	recording = True;
	UpdateControlPanel();
}


void
fileFormatCB(w, client, call)
Widget w;
caddr_t client;
XmAnyCallbackStruct *call;
{
}


void
dataFormatCB(w, client, call)
Widget w;
caddr_t client;
XmAnyCallbackStruct *call;
{
}


void
sampleRateCB(w, client, call)
Widget w;
caddr_t client;
XmAnyCallbackStruct *call;
{
}


void
channelsCB(w, client, call)
Widget w;
caddr_t client;
XmAnyCallbackStruct *call;
{
}


void
SaveYourselfCB(w, client_data, call_data) /* Callback for Save Yourself event */
Widget          w;
XtPointer       client_data;
XtPointer       call_data;
{
	XSetCommand(XtDisplay(top), XtWindow(top), NULL, 0);
}


XtArgVal GetPixel(colorstr)
     char *colorstr;
{
  XrmValue from, to;

  from.size = strlen(colorstr) +1;
  if (from.size < sizeof(String)) from.size = sizeof(String);
  from.addr = colorstr;
  to.addr = NULL;
  XtConvert(top, XmRString, &from, XmRPixel, &to);

  return ((XtArgVal) *((XtArgVal *) to.addr));
}


Widget
BuildInfoPanel(parent)
Widget parent;
{
	Widget fileName_l;
	Widget column1, column2, column3, column4;
	Widget fileFormat_l, dataFormat_l, sampleRate_l, channels_l;
	Arg args[8];
	int n;
	char numstr[8];

	n = 0;
	infoPanel = XtCreateManagedWidget("infoPanel", xmFormWidgetClass,
		parent, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);	n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);	n++;
	fileName_l = XtCreateManagedWidget(" File name:",
		 xmLabelWidgetClass, infoPanel, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);	n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget, fileName_l);	n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);	n++;
	fileName_v = XtCreateManagedWidget("fileName_v",
		 xmLabelWidgetClass, infoPanel, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget, fileName_l);	n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);	n++;
	XtSetArg(args[n], XmNorientation, XmVERTICAL);	n++;
	XtSetArg(args[n], XmNpacking, XmPACK_COLUMN);	n++;
	XtSetArg(args[n], XmNnumColumns, 1);	n++;
	column1 = XtCreateManagedWidget("column1", xmRowColumnWidgetClass,
		infoPanel, args, n);

	n = 0;
	fileFormat_l = XtCreateManagedWidget("File format:",
		 xmLabelWidgetClass, column1, args, n);

	n = 0;
	dataFormat_l = XtCreateManagedWidget("Data format:",
		 xmLabelWidgetClass, column1, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget, fileName_v);	n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget, column1);	n++;
	XtSetArg(args[n], XmNorientation, XmVERTICAL);	n++;
	XtSetArg(args[n], XmNpacking, XmPACK_COLUMN);	n++;
	XtSetArg(args[n], XmNnumColumns, 1);	n++;
	column2 = XtCreateManagedWidget("column2", xmRowColumnWidgetClass,
		infoPanel, args, n);

	n = 0;
	fileFormat_v = XtCreateManagedWidget("fileFormat_v", xmLabelWidgetClass,
		column2, args, n);

	n = 0;
	dataFormat_v = XtCreateManagedWidget("Data format:",
		 xmLabelWidgetClass, column2, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget, fileName_v);	n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget, column2);	n++;
	XtSetArg(args[n], XmNorientation, XmVERTICAL);	n++;
	XtSetArg(args[n], XmNpacking, XmPACK_COLUMN);	n++;
	XtSetArg(args[n], XmNnumColumns, 1);	n++;
	column3 = XtCreateManagedWidget("column3", xmRowColumnWidgetClass,
		infoPanel, args, n);

	n = 0;
	sampleRate_l = XtCreateManagedWidget("Sampling rate:",
		 xmLabelWidgetClass, column3, args, n);

	n = 0;
	channels_l = XtCreateManagedWidget("Channels:", xmLabelWidgetClass,
		column3, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget, fileName_v);	n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget, column3);	n++;
	XtSetArg(args[n], XmNorientation, XmVERTICAL);	n++;
	XtSetArg(args[n], XmNpacking, XmPACK_COLUMN);	n++;
	XtSetArg(args[n], XmNnumColumns, 1);	n++;
	column4 = XtCreateManagedWidget("column4", xmRowColumnWidgetClass,
		infoPanel, args, n);

	n = 0;
	sprintf(numstr, "%6d", res.sample_rate);
	XtSetArg(args[n], XmNlabelString, XmStringCreateSimple(numstr)); n++;
	sampleRate_v = XtCreateManagedWidget("sampleRate_v",
		 xmLabelWidgetClass, column4, args, n);

	n = 0;
	sprintf(numstr, "%2d", res.channels);
	XtSetArg(args[n], XmNlabelString, XmStringCreateSimple(numstr)); n++;
	channels_v = XtCreateManagedWidget("channels_v", xmLabelWidgetClass,
		column4, args, n);

	return(infoPanel);
}


void
SetShortFileName()
{
	if (res.file_name == NULL)
		res.short_file_name = res.null_file_name;
	else
	{
		res.short_file_name = strrchr(res.file_name, '/');
		if (res.short_file_name == NULL)
			res.short_file_name = res.file_name;
		else
			res.short_file_name++;
	}
}


void
UpdateInfoPanel()
{
	Arg args[8];
	int n;
	char numstr[8];
	XtVaSetValues(fileName_v,
		XmNlabelString, XmStringCreateSimple(res.short_file_name),
		NULL);

	n = 0;
	XtSetArg(args[n], XmNlabelString,
		XmStringCreateSimple(fileFormat[res.file_format].name)); n++;
	XtSetValues(fileFormat_v, args, n);

	n = 0;
	XtSetArg(args[n], XmNlabelString,
		XmStringCreateSimple(dataFormat[res.data_format].name)); n++;
	XtSetValues(dataFormat_v, args, n);

	n = 0;
	sprintf(numstr, "%6d", res.sample_rate);
	XtSetArg(args[n], XmNlabelString, XmStringCreateSimple(numstr)); n++;
	XtSetValues(sampleRate_v, args, n);

	n = 0;
	sprintf(numstr, "%2d", res.channels);
	XtSetArg(args[n], XmNlabelString, XmStringCreateSimple(numstr)); n++;
	XtSetValues(channels_v, args, n);
}


void
CreateControlPanelPixmaps()
{
	stopUpPix = XCreatePixmapFromBitmapData(display,
		DefaultRootWindow(display), stop_up_bits,
		stop_up_width, stop_up_height,
		GetPixel("scoForeground"), GetPixel("scoBackground"),
		DefaultDepth(display,DefaultScreen(display)));

	stopDownPix = XCreatePixmapFromBitmapData(display,
		DefaultRootWindow(display), stop_down_bits,
		stop_down_width, stop_down_height,
		GetPixel("scoForeground"), GetPixel("scoBackground"),
		DefaultDepth(display,DefaultScreen(display)));

	playUpPix = XCreatePixmapFromBitmapData(display,
		DefaultRootWindow(display), play_up_bits,
		play_up_width, play_up_height,
		GetPixel("scoForeground"), GetPixel("scoBackground"),
		DefaultDepth(display,DefaultScreen(display)));

	playDownPix = XCreatePixmapFromBitmapData(display,
		DefaultRootWindow(display), play_down_bits,
		play_down_width, play_down_height,
		GetPixel("scoForeground"), GetPixel("scoBackground"),
		DefaultDepth(display,DefaultScreen(display)));

	pauseUpPix = XCreatePixmapFromBitmapData(display,
		DefaultRootWindow(display), pause_up_bits,
		pause_up_width, pause_up_height,
		GetPixel("scoForeground"), GetPixel("scoBackground"),
		DefaultDepth(display,DefaultScreen(display)));

	pauseDownPix = XCreatePixmapFromBitmapData(display,
		DefaultRootWindow(display), pause_down_bits,
		pause_down_width, pause_down_height,
		GetPixel("scoForeground"), GetPixel("scoBackground"),
		DefaultDepth(display,DefaultScreen(display)));

	recordUpPix = XCreatePixmapFromBitmapData(display,
		DefaultRootWindow(display), record_up_bits,
		record_up_width, record_up_height,
		GetPixel("scoForeground"), GetPixel("scoBackground"),
		DefaultDepth(display,DefaultScreen(display)));

	recordDownPix = XCreatePixmapFromBitmapData(display,
		DefaultRootWindow(display), record_down_bits,
		record_down_width, record_down_height,
		GetPixel("scoForeground"), GetPixel("scoBackground"),
		DefaultDepth(display,DefaultScreen(display)));

}

Widget
BuildControlPanel(parent)
Widget parent;
{
	Widget controlPanelFrame, controlPanel, sep;
	Arg args[8];
	int n;

	CreateControlPanelPixmaps();

	n = 0;
	controlPanelFrame = XtCreateManagedWidget("controlPanelFrame",
		 xmFrameWidgetClass, parent, args, n);

	n = 0;
	XtSetArg(args[n], XmNorientation, XmHORIZONTAL);	n++;
	controlPanel = XtCreateManagedWidget("controlPanel",
		 xmRowColumnWidgetClass, controlPanelFrame, args, n);

	n = 0;
	XtSetArg(args[n], XmNlabelType, XmPIXMAP);	n++;
	XtSetArg(args[n], XmNlabelPixmap, stopDownPix);	n++;
	stop_b = XtCreateManagedWidget("stop_b", xmPushButtonWidgetClass,
		 controlPanel, args, n);
	XtAddCallback(stop_b, XmNactivateCallback, stopCB, NULL);
	
	n = 0;
	XtSetArg(args[n], XmNlabelType, XmPIXMAP);	n++;
	XtSetArg(args[n], XmNlabelPixmap, pauseUpPix);	n++;
	pause_b = XtCreateManagedWidget("pause_b", xmPushButtonWidgetClass,
		controlPanel, args, n);
	XtAddCallback(pause_b, XmNactivateCallback, pauseCB, NULL);
	
	n = 0;
	XtSetArg(args[n], XmNlabelType, XmPIXMAP);	n++;
	XtSetArg(args[n], XmNlabelPixmap, playUpPix);	n++;
	play_b = XtCreateManagedWidget("play_b", xmPushButtonWidgetClass,
		 controlPanel, args, n);
	XtAddCallback(play_b, XmNactivateCallback, playCB, NULL);

	n = 0;
	XtSetArg(args[n], XmNorientation, XmVERTICAL);	n++;
	sep = XtCreateManagedWidget("sep", xmSeparatorGadgetClass,
		controlPanel, args, n);

	n = 0;
	XtSetArg(args[n], XmNlabelType, XmPIXMAP);	n++;
	XtSetArg(args[n], XmNlabelPixmap, recordUpPix);	n++;
	record_b = XtCreateManagedWidget("record_b", xmPushButtonWidgetClass,
		controlPanel, args, n);
	XtAddCallback(record_b, XmNactivateCallback, recordCB, NULL);

	return(controlPanelFrame);
}


void
UpdateControlPanel()
{
	if (playing)
		XtVaSetValues(play_b,
			XmNlabelPixmap, playDownPix,
			NULL);
	else
		XtVaSetValues(play_b,
			XmNlabelPixmap, playUpPix,
			NULL);

	if (paused)
		XtVaSetValues(pause_b,
			XmNlabelPixmap, pauseDownPix,
			NULL);
	else
		XtVaSetValues(pause_b,
			XmNlabelPixmap, pauseUpPix,
			NULL);

	if (recording)
		XtVaSetValues(record_b,
			XmNlabelPixmap, recordDownPix,
			NULL);
	else
		XtVaSetValues(record_b,
			XmNlabelPixmap, recordUpPix,
			NULL);

	if (!recording && !playing && !paused)
		XtVaSetValues(stop_b,
			XmNlabelPixmap, stopDownPix,
			NULL);
	else
		XtVaSetValues(stop_b,
			XmNlabelPixmap, stopUpPix,
			NULL);
}

