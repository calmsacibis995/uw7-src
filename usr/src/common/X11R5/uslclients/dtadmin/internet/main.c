#ifndef NOIDENT
#ident	"@(#)dtadmin:internet/main.c	1.38"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <errno.h>
#include <Xm/MainW.h>
#include <Xm/PanedW.h>
#include <Xm/RowColumn.h>
#include <Xm/DrawingA.h>
#include <Xm/CascadeBG.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Form.h>
#include <Xm/ScrolledW.h>
#include <Xm/MessageB.h>
#include "DesktopP.h"
#include "lookup.h"
#include "lookupG.h"
#include "util.h"
#include "inet.h"
#include "inetMsg.h"

#define	TYPE			"RAISE_MYSELF"
#define	APP_ID			"INTERNETMGR"
#define	XA_RAISE_MYSELF(d)	XInternAtom(d, TYPE, False)
#define XA_APP_ID(d)		XInternAtom(d, APP_ID, False)

extern	char	*mygettxt();
extern	void	*listDefaultCB();		
extern	void	createMsg(Widget, msgType, char *);
extern	void	createProperty();
extern	int	initUucp();
extern	int 	readUucpFile(systemsList *);
extern	int	wirteUucpFile();	
extern void	helpkeyCB(Widget, XtPointer, XtPointer);
extern void	ExmInitDnDIcons(Widget);

char *	ApplicationName;
char *	Program;
DmGlyphPtr	glyph;

HelpText	MainHelp = { NULL, NULL, "10" };

createMainWin()
{
	Widget		main_w;
	struct stat	statbuf;
	int		ret;

	/* use form instead of rc */
	/*main_w = XtVaCreateManagedWidget("mainrc",  */
	main_w = XtVaCreateWidget("mainrc", 
		xmFormWidgetClass, hi.net.common.toplevel,NULL);
	XtAddCallback(main_w, XmNhelpCallback, helpkeyCB, &MainHelp);
	hi.net.common.topRC = main_w;
	hi.net.common.isInet = TRUE;
	hi.net.common.isFirst = TRUE;
	createMenu(main_w);
	createHostsWin(&(hi.net));

	/* if /etc/uucp/Systems.tcp doesn't exist, add all hosts entries */
	while ((ret = stat(uucpPath, &statbuf)) < 0 && errno == EINTR)
		; /* keep trying */
	if (ret == -1 && errno == ENOENT && hi.net.common.isOwner) {
		/*
		initUucp();
		readUucpFile(hi.inet.uucp);
		*/
		writeUucpFile(True);
	}

	hi.net.etc.clickCB = createProperty;
	if (hi.net.common.isDnsConfigure == True) {
		hi.net.dns.clickCB = createProperty;
	}

	if (hi.net.common.mb != 0) 
		if (XtIsRealized(hi.net.common.mb))
			XtPopdown(XtParent(hi.net.common.mb));
}

main(argc, argv)
int argc;
char *argv[];
{
	Widget	menu; 
	Widget	option_menu, menubar;
	void 	input();
	XtWidgetGeometry geom;
	int	i;
	static Window	another_window;
	Display	*display;
	struct	utsname	sname;
	extern MenuItem *drawing_menus;
	Pixmap	icon, iconmask;
	Boolean	drag_icon;

	XtSetLanguageProc(NULL, NULL, NULL);

	hi.net.common.toplevel = XtVaAppInitialize(&hi.net.common.app, "Demos", NULL, 0,
		&argc, argv, NULL, NULL);

	ApplicationName = mygettxt(TXT_appName);
	hi.net.common.COMPOUND_TEXT = XmInternAtom(XtDisplay(hi.net.common.toplevel),
			"COMPOUND_TEXT", False);

	DtiInitialize(hi.net.common.toplevel);

	XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(hi.net.common.toplevel)),
		"enableDragIcon", &drag_icon, NULL);
	if (drag_icon) {
		ExmInitDnDIcons(hi.net.common.toplevel);
	}

	if (glyph == NULL)
		glyph = DmGetPixmap(XtScreen(hi.net.common.toplevel), "tcpadm48.icon");

	if (glyph) {
		icon = glyph->pix;
		iconmask = glyph->mask;
	}
	else
		icon = iconmask = (Pixmap) 0;

	XtVaSetValues(hi.net.common.toplevel,
		XmNallowShellResize, True,
		XmNtitle, ApplicationName,
		XtNiconPixmap, (XtArgVal) icon,
		XtNiconMask, (XtArgVal) iconmask,
		XtNiconName, (XtArgVal) mygettxt(TXT_iconName),
		NULL);

	hi.net.common.isOwner = _DtamIsOwner("inet"); /* we should use OWN_NETADM */
	XtVaSetValues(hi.net.common.toplevel, 
		XmNmappedWhenManaged, False, NULL);
	XtRealizeWidget(hi.net.common.toplevel);
	display = XtDisplay(hi.net.common.toplevel);
	another_window = DtSetAppId(display,
				XtWindow(hi.net.common.toplevel),
				APP_ID);
	if (another_window != None) {
		XMapWindow(display, another_window);
		XRaiseWindow(display, another_window);
		XFlush(display);
		exit(0);
	}
	XtVaSetValues(hi.net.common.toplevel, XmNmappedWhenManaged, True, NULL);
	uname(&sname);
	if (chkEtcHostEnt(sname.nodename) == FALSE) {
		 createConfigLocal(hi.net.common.toplevel);
	}
	else {
		createMainWin();
	}
	XtAppMainLoop(hi.net.common.app);
}

void
exitMainCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	exit(0);
}
