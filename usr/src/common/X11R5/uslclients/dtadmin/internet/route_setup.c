#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:internet/route_setup.c	1.30"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <wait.h>

#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/SeparatoG.h>
#include <Xm/Frame.h>
#include <Xm/ToggleBG.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/TextF.h>
#include <Xm/MessageB.h>

#include "DesktopP.h"

#include "lookup.h"
#include "lookupG.h"
#include "util.h"
#include "inet.h"
#include "inetMsg.h"

#include "route_util.h"

/* function prototypes */
/* external -- these should be in a header file somewhere */
extern Widget	crt_inet_addr(Widget, char *, int, inetAddr *);
extern Widget	crt_inputradio(Widget, char *, int, radioList *, Boolean);
extern Widget	crt_inputlabellabel(Widget, char *, int, char *);
extern Widget	crt_radio(Widget, radioList *);
extern Widget	crt_rc_radio(Widget, radioList *, Widget);
extern Widget	crt_inputlabel(Widget, char *, int);
extern char *	mygettxt(char *);
extern void	createMsg(Widget, msgType, char *, char *);
extern void	helpkeyCB(Widget, XtPointer, XtPointer);
extern Boolean	getAddrToDotStr(inetAddr, char *);
extern void	setAddrGui(inetAddr, unsigned char *);
extern int	populateAddr(Widget, Widget, inetAddr, char **, char *, char **);
extern void	clearAddr(inetAddr);
extern void	displayFooterMsgEH(Widget, MiniHelp_t *, XFocusChangeEvent *);
extern int	r_decor(Widget);
extern Boolean	removeWhitespace(char *);

/* static */
/* callback fcns */
static void	okCB(Widget, XtPointer, XtPointer);
static void	resetCB(Widget, XtPointer, XtPointer);
static void	cancelCB(Widget, XtPointer, XtPointer);
static void	getaddrCB(Widget, XtPointer, XtPointer);
static void	classA_CB(Widget, XtPointer, XtPointer);
static void	classB_CB(Widget,XtPointer,XtPointer);
static void	classC_CB(Widget, XtPointer, XtPointer);
static void	classOther_CB(Widget, XtPointer, XtPointer);
static void	verifyHexInputCB(Widget, XtPointer, XtPointer);
static void	destroyCB(Widget, XtPointer, XtPointer);
static void	createErrorMsg(Widget, msgType, char *, char *);
static void	msgClearCB(Widget, XtPointer, XtPointer);
static void	nameChangedCB(Widget, XtPointer, XtPointer);

/* other fcns */
static int	getInitialValues(void);
static void	createMaskField(Widget, char *);
static void	bailOut(void);

/* local definitions */

#define	BAD_NETMASK	0x01
#define	BAD_ADDR	0x02

#define CHANGED_NETMASK	0x01
#define CHANGED_ADDRESS	0x02
#define CHANGED_DAEMON	0x04

/* globals */

MiniHelp_t	routerHelp[] = {
	{ MiniHelp_route1,	NULL },
	{ MiniHelp_route2,	NULL },
	{ MiniHelp_route3,	NULL },
	{ MiniHelp_route4,	NULL }
};

HelpText	RSHelp = { NULL, NULL, "210" };

static ActionAreaItems	actions[] = {
	{TXT_OK, MNE_OK, okCB, NULL},
	{TXT_Reset, MNE_Reset, resetCB, NULL},
	{TXT_Cancel, MNE_Cancel, cancelCB, NULL},
	{TXT_Help, MNE_Help, helpkeyCB, &RSHelp }
};

static char *		net_form[]={
	TXT_subnetType,
	TXT_network_id,
	""
};

static char *		router_form[]={
	TXT_defRouter,
	TXT_inet_addr,
	""
};

static radioItem        subnetType[] = {
	{TXT_classA, classA_CB, NULL}, 
	{TXT_classB, classB_CB, NULL}, 
	{TXT_classC, classC_CB, NULL}, 
	{TXT_other, classOther_CB, NULL}
};

radioList       subnetRadio = {
	subnetType,
	NUM_SUBNET,
	XmHORIZONTAL,
	NULL
};

typedef struct _router {
	Widget			popup;
	Widget			netidForm;
	Widget			net_id;
	Widget			host_id;
	Widget			subnetType;
	Widget			maskForm;
	Widget			maskAddr;
	Widget			defaultRouter;
	Widget			getaddr;
	inetAddr		addr;
	Widget			update;
	Widget			status;
} router;

typedef struct _values {
	Class_t	class;
	String	netmask;	/* alloc'd */
	String	networkID;	/* alloc'd */
	String	hostID;		/* alloc'd */
	String	defaultRouter;	/* alloc'd */
	String	routerAddr;	/* alloc'd */
	ADDR	hexAddr;
	Boolean	updateButton;
	pid_t	routeProc;
} Values_t;

static void	freeValues(Values_t *);

router		rsWin;
Values_t	rsInitValues, rsNewValues;
char *		hostAddr = NULL;	/* alloc'd */
ConfigFile_t	configFile;
ConfigFile_t	interfaceFile;

static void 
okCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int		i;
	int		ret;
	Boolean		bret;
	ushort_t	whatChanged	= 0;
	ushort_t	errorFlags	= 0;
	char		tmpbuf[BUFSIZ];
	char *		broadcastAddr	= NULL;
	ulong_t		netmaskValue;
	pid_t		proc;
	off_t		size;
	FILE *		tmpfd;
	char *		tmpFileBuf;

	busyCursor(rsWin.popup, True);

	freeValues(&rsNewValues);

	/* get new netmask */
	for (i = 0; i < 4; i++) {
		Boolean	set;

		XtVaGetValues(subnetRadio.list[i].widget,
			XmNset, &set,
			NULL);
		if (set)
			break;
	}
	rsNewValues.class = i;
	/* get netmask */
	rsNewValues.netmask = XmTextFieldGetString(rsWin.maskAddr);
	removeWhitespace(rsNewValues.netmask);
	if (rsInitValues.class != rsNewValues.class) {
		whatChanged |= CHANGED_NETMASK;
		if (
			(rsNewValues.class == ClassOther) &&
			(strcmp(rsInitValues.netmask, rsNewValues.netmask))
		) {
			netmaskValue = strtoul(
				rsNewValues.netmask,
				(char **)NULL,
				16
			);
			ret = validateNetmask(netmaskValue);
			if (ret) {
				errorFlags |= BAD_NETMASK;
			}
		} else {
			netmaskValue = strtoul(
				rsNewValues.netmask,
				(char **)NULL,
				16
			);
		}
	}

	/* get new default router */
	rsNewValues.defaultRouter = XmTextFieldGetString(rsWin.defaultRouter);
	removeWhitespace(rsNewValues.defaultRouter);
	memset((void *)tmpbuf, 0, BUFSIZ * sizeof(char));
	bret = getAddrToDotStr(rsWin.addr, tmpbuf);
	rsNewValues.routerAddr = strdup(tmpbuf);
	if (
		(! bret) &&
		(strcmp(rsInitValues.routerAddr, rsNewValues.routerAddr))
	) {
		
		errorFlags |= BAD_ADDR;
	} else {
		if (strcmp(rsInitValues.routerAddr, rsNewValues.routerAddr)) {
			whatChanged |= CHANGED_ADDRESS;
		}
	}

	/* get new state of update button */
	XtVaGetValues(rsWin.update, XmNset, &rsNewValues.updateButton, NULL);
	if (rsInitValues.updateButton != rsNewValues.updateButton) {
		whatChanged |= CHANGED_DAEMON;
	}
	if (errorFlags) {
		/* pop up error msg */
		if (errorFlags == 1 || errorFlags == 2 ) {
			strcpy(tmpbuf, mygettxt(ERR_routeSingle));
		} else {
			strcpy(tmpbuf, mygettxt(ERR_routeMulti));
		}
		if (errorFlags & BAD_NETMASK) {
			strcat(tmpbuf, mygettxt(ERR_routeBadNetmask));
		}
		if (errorFlags & BAD_ADDR) {
			strcat(tmpbuf, mygettxt(ERR_routeBadAddr));
		}
		strcat(tmpbuf, mygettxt(ERR_routeMsg2));
		createErrorMsg(
			hi.net.common.toplevel,
			ERROR,
			tmpbuf,
			TXT_appName
		);
		busyCursor(rsWin.popup, False);

		return;
	}

	/* no errors, so proceed with procedure... */

	if (whatChanged == 0) {
		bailOut();
		busyCursor(rsWin.popup, False);
		XtUnmanageChild(rsWin.popup);

		return;
	}

	/* backup /etc/inet/config and /etc/confnet.d/inet/interface files */
	tmpFileBuf = copylist(CONFIGPATH, &size);
	if (tmpFileBuf) {
		tmpfd = fopen("/tmp/config.backup", "w");
		for (i = 0; i < size; i++) {
			if (tmpFileBuf[i])
				fputc(tmpFileBuf[i], tmpfd);
			else
				fputc('\n', tmpfd);
		}
		fclose(tmpfd);
		free(tmpFileBuf);
	}
	tmpFileBuf = copylist(INTERFACEPATH, &size);
	if (tmpFileBuf) {
		tmpfd = fopen("/tmp/interface.backup", "w");
		for (i = 0; i < size; i++) {
			if (tmpFileBuf[i])
				fputc(tmpFileBuf[i], tmpfd);
			else
				fputc('\n', tmpfd);
		}
		fclose(tmpfd);
		free(tmpFileBuf);
	}
	errorFlags = 0;
	if (whatChanged & CHANGED_NETMASK) {
		ret = getBroadcastFromAddr(
			hostAddr,
			netmaskValue,
			&broadcastAddr
		);
		if (ret) ++errorFlags;
		sprintf(tmpbuf, "0x%s", rsNewValues.netmask);
		ret = modifyNetmask(&interfaceFile, tmpbuf, broadcastAddr);
		if (ret) ++errorFlags;
		if (broadcastAddr) free(broadcastAddr);
		ret = writeConfigFile(&interfaceFile);
		if (ret) ++errorFlags;
	}
	if (whatChanged & CHANGED_ADDRESS) {
		ret = modifyDefaultRouter(&configFile, rsNewValues.routerAddr);
		if (ret) ++errorFlags;

	}
	if (whatChanged & CHANGED_DAEMON) {
		if (rsNewValues.updateButton == False) {
			killProcess(rsInitValues.routeProc);
			ret = modifyRoutedEntry(&configFile, 0);
			if (ret) ++errorFlags;
		} else {
			ret = system("/usr/sbin/in.routed -q");
			if ((ret == -1) || WEXITSTATUS(ret) != 0) {
			}
			ret = modifyRoutedEntry(&configFile, 1);
			if (ret) ++errorFlags;
		}
	}
	if (
		(whatChanged & CHANGED_DAEMON) ||
		(whatChanged & CHANGED_ADDRESS)
	) {
		ret = writeConfigFile(&configFile);
		if (ret) ++errorFlags;
	}

	if (errorFlags) {
		createMsg(
			hi.net.common.toplevel,
			ERROR,
			mygettxt(ERR_routeSaveErrors2),
			mygettxt(TXT_routeView)
		);
		/* restore system files */
		tmpFileBuf = copylist("/tmp/config.backup", &size);
		if (tmpFileBuf) {
			tmpfd = fopen(CONFIGPATH, "w");
			for (i = 0; i < size; i++) {
				if (tmpFileBuf[i])
					fputc(tmpFileBuf[i], tmpfd);
				else
					fputc('\n', tmpfd);
			}
			fclose(tmpfd);
			free(tmpFileBuf);
		}
		tmpFileBuf = copylist("/tmp/interface.backup", &size);
		if (tmpFileBuf) {
			tmpfd = fopen(INTERFACEPATH, "w");
			for (i = 0; i < size; i++) {
				if (tmpFileBuf[i])
					fputc(tmpFileBuf[i], tmpfd);
				else
					fputc('\n', tmpfd);
			}
			fclose(tmpfd);
			free(tmpFileBuf);
		}
	} else {
		if (whatChanged == 4) {
			goto bail;
		}
		createMsg(
			hi.net.common.toplevel,
			INFO,
			mygettxt(TXT_routeOkMsg),
			mygettxt(TXT_routeView)
		);
	}

bail:
	busyCursor(rsWin.popup, False);
	unlink("/tmp/interface.backup");
	unlink("/tmp/config.backup");
	bailOut();
	XtUnmanageChild(rsWin.popup);

	return;
}

static void
resetCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget		lastButton;
	XmString	net, host;
	int		ret;
	int		i;
	Boolean		set;

	for (i = 0; i < 4; i++) {
		XtVaGetValues(subnetRadio.list[i].widget,
			XmNset, &set,
			NULL);
		if (set)
			break;
	}
	XtVaSetValues(subnetRadio.list[i].widget, XmNset, False, NULL);
	if (i == ClassOther) {
		XtUnmanageChild(rsWin.maskForm);
		XtManageChild(rsWin.netidForm);
	}
	XtVaSetValues(
		subnetRadio.list[rsInitValues.class].widget,
		XmNset,	True,
		NULL
	);
	net = XmStringCreateLocalized(rsInitValues.networkID);
	XtVaSetValues(
		rsWin.net_id,
		XmNlabelString,	net,
		NULL
	);
	XmStringFree(net);
	host = XmStringCreateLocalized(rsInitValues.hostID);
	XtVaSetValues(
		rsWin.host_id,
		XmNlabelString,	host,
		NULL
	);
	XmStringFree(host);
	XtVaSetValues(
		rsWin.maskAddr,
		XmNvalue,	rsInitValues.netmask,
		NULL
	);
	XtVaSetValues(
		rsWin.defaultRouter,
		XmNvalue,	rsInitValues.defaultRouter,
		NULL
	);
	if (rsInitValues.routerAddr) {
		setAddrGui(rsWin.addr, rsInitValues.hexAddr);
	} else {
		clearAddr(rsWin.addr);
	}
	XtVaSetValues(
		rsWin.update,
		XmNset,	rsInitValues.updateButton,
		NULL
	);

	return;
}

static void
cancelCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	bailOut();
	XtUnmanageChild(rsWin.popup);
}

static void
destroyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	bailOut();
}

void
createRouterSetup(Widget w, XtPointer client_data, XtPointer call_data)
{
        int		maxWidth1, maxWidth2;
        Widget		sep1, sep2;
	Widget		labelForm;
	XmString	string;
	static Widget	topForm;
	Widget		topRC;
	Widget		networkLabel;
	Widget		systemLabel;
	Widget		form, form1;
	Widget		frame1, frame2;
	Widget		lowerForm, route_rc, label, netRC, label1; 
	Widget		routeRC;
	struct utsname	sname;
	int		ret;
	int		i;
	char		buf[MEDIUMBUFSIZE];
	DmMnemonicInfoRec	mneInfo[MI_TOTAL];
	unsigned char *		mne;
	char			tmpstr[BUFSIZ];
	KeySym			mneks;
	XmString		mneString;
	int			mm;


	busyCursor(hi.net.common.toplevel, True);
	uname(&sname);
	if (hi.net.common.isDnsConfigure) {
		sprintf(buf, "%s.%s", sname.nodename, hi.net.dns.resolv->domain);
	} else {
		strcpy(buf, sname.nodename);
	}
	getAddrFromName(buf, &hostAddr);
	if (ret = getInitialValues()) {
		createErrorMsg(
			hi.net.common.toplevel,
			ERROR,
			mygettxt(ERR_routeInit),
			TXT_appName
		);
		bailOut();
		busyCursor(hi.net.common.toplevel, False);

		return;
	}
        if(!rsWin.popup) {
                maxWidth1 = GetMaxWidth(XtParent(w), net_form);
                maxWidth2 = GetMaxWidth(XtParent(w), router_form);
                rsWin.popup = XtVaCreatePopupShell("routerSetup",
                        xmDialogShellWidgetClass, XtParent(w),
                        NULL);
                XtVaSetValues(rsWin.popup,
                        XmNtitle, mygettxt(TXT_appName),
                        XmNallowShellResize, True,
                        XmNdeleteResponse, XmUNMAP,
                        NULL);
		XtAddCallback(
			rsWin.popup,
			XmNdestroyCallback,
			destroyCB,
			NULL
		);

		r_decor(rsWin.popup);

		topForm = XtVaCreateWidget("topForm",
			xmFormWidgetClass, rsWin.popup,
			NULL);

                topRC = XtVaCreateManagedWidget("rowColumn",
                        xmRowColumnWidgetClass, topForm,
                        XmNorientation, XmVERTICAL,
                        NULL);

		XtAddCallback(topRC, XmNhelpCallback, helpkeyCB, &RSHelp);

		frame1 =  XtVaCreateManagedWidget("frame1",
				xmFrameWidgetClass, topRC,
        			NULL);

		form   = XtVaCreateManagedWidget("form",
                        xmFormWidgetClass, frame1,
                        NULL);


                label = XtVaCreateManagedWidget("label",
                        xmLabelWidgetClass, form,
			XmNtopAttachment, XmATTACH_FORM,
                        NULL);
		setLabel(label, TXT_subnetMask);
		netRC = XtVaCreateManagedWidget("netRC",
			xmRowColumnWidgetClass, form,
			XmNorientation, XmVERTICAL,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);

		rsWin.subnetType = crt_inputradio(netRC, TXT_subnetType, maxWidth1, &subnetRadio, True);
		XtVaSetValues(subnetRadio.list[rsInitValues.class].widget,
			XmNset,	True, NULL);

		rsWin.netidForm = XtVaCreateWidget("netidForm",
			xmFormWidgetClass,	netRC,
			NULL);
		string = XmStringCreateLocalized((String)mygettxt(TXT_network_id2));
		networkLabel = XtVaCreateManagedWidget("label",
			xmLabelWidgetClass, rsWin.netidForm,
			XmNalignment,		XmALIGNMENT_END,
			XmNwidth,		maxWidth1,
			XmNlabelString,		string,
			XmNleftAttachment,	XmATTACH_FORM,
			XmNtopAttachment,	XmATTACH_FORM,
			XmNbottomAttachment,	XmATTACH_FORM,
			NULL);
		XmStringFree(string);
		string = XmStringCreateLocalized(rsInitValues.networkID);
		rsWin.net_id = XtVaCreateManagedWidget("label",
			xmLabelWidgetClass, rsWin.netidForm,
			XmNlabelString,		string,
			XmNleftAttachment,	XmATTACH_WIDGET,
			XmNleftWidget,		networkLabel,
			XmNtopAttachment,	XmATTACH_FORM,
			XmNbottomAttachment,	XmATTACH_FORM,
			NULL);
		XmStringFree(string);
		string = XmStringCreateLocalized((String)mygettxt(TXT_host_id2));
		systemLabel = XtVaCreateManagedWidget("label",
			xmLabelWidgetClass, rsWin.netidForm,
			XmNwidth,		maxWidth1,
			XmNlabelString,		string,
			XmNleftAttachment,	XmATTACH_WIDGET,
			XmNleftWidget,		rsWin.net_id,
			XmNtopAttachment,	XmATTACH_FORM,
			XmNbottomAttachment,	XmATTACH_FORM,
			NULL);
		XmStringFree(string);
		string = XmStringCreateLocalized(rsInitValues.hostID);
		rsWin.host_id = XtVaCreateManagedWidget("label",
			xmLabelWidgetClass, rsWin.netidForm,
			XmNlabelString,		string,
			XmNleftAttachment,	XmATTACH_WIDGET,
			XmNleftWidget,		systemLabel,
			XmNtopAttachment,	XmATTACH_FORM,
			XmNbottomAttachment,	XmATTACH_FORM,
			NULL);
		XmStringFree(string);

		createMaskField(netRC, TXT_routeMask);

		if (rsInitValues.class == ClassOther) {
			XtManageChild(rsWin.maskForm);
		} else {
			XtManageChild(rsWin.netidForm);
		}

		frame2 =  XtVaCreateManagedWidget("frame2",
				xmFrameWidgetClass, topRC,
        			NULL);
                lowerForm = XtVaCreateManagedWidget("rowColumn",
                        xmRowColumnWidgetClass, frame2,
                        NULL);
                label = XtVaCreateManagedWidget("label",
                        xmLabelWidgetClass, lowerForm,
			XmNtopAttachment,	XmATTACH_FORM,
                        NULL);
		setLabel(label, TXT_networkRoute);
		routeRC = XtVaCreateManagedWidget("routeRC",
			xmRowColumnWidgetClass, lowerForm, NULL);
		route_rc = XtVaCreateManagedWidget("route_rc",
			xmRowColumnWidgetClass, routeRC,
			XmNorientation, XmHORIZONTAL,
			NULL);
		rsWin.defaultRouter = (Widget)crt_inputlabel(route_rc, TXT_defRouter , maxWidth2);
		XtAddCallback(
			rsWin.defaultRouter,
			XmNvalueChangedCallback,
			nameChangedCB,
			NULL
		);
		if (rsInitValues.defaultRouter) {
			XtVaSetValues(rsWin.defaultRouter,
				XmNvalue,	rsInitValues.defaultRouter,
				NULL);
		}
		strcpy(tmpstr, mygettxt(TXT_getAddr));
		rsWin.getaddr = XtVaCreateManagedWidget(tmpstr,
			xmPushButtonWidgetClass, route_rc,
			NULL);

		XmSTR_N_MNE(
			TXT_getAddr,
			MNE_getAddr,
			mneInfo[0],
			DM_B_MNE_ACTIVATE_CB | DM_B_MNE_GET_FOCUS
		);
		XmStringFree(mneString);

		XtVaSetValues(rsWin.getaddr, XmNmnemonic, mneks, NULL);

		mneInfo[0].w = rsWin.getaddr;
		mneInfo[0].cb = (XtCallbackProc)getaddrCB;

		if (hi.net.common.isDnsConfigure) {
			XtSetSensitive(rsWin.getaddr, True);
			XtAddCallback(rsWin.getaddr, XmNactivateCallback, 
				getaddrCB, NULL);
		} else {
			XtSetSensitive(rsWin.getaddr, False);
		}

                crt_inet_addr(routeRC, TXT_inet_addr, maxWidth2, &(rsWin.addr));
		if (rsInitValues.routerAddr) {
			char	a[4], b[4], c[4], d[4];

			breakupAddr(rsInitValues.routerAddr, a, b, c, d);
			/*
			XtVaSetValues(rsWin.addr.addr[0], XmNvalue, a, NULL);
			XtVaSetValues(rsWin.addr.addr[1], XmNvalue, b, NULL);
			XtVaSetValues(rsWin.addr.addr[2], XmNvalue, c, NULL);
			XtVaSetValues(rsWin.addr.addr[3], XmNvalue, d, NULL);
			*/
			XmTextFieldSetString(rsWin.addr.addr[0], a);
			XmTextFieldSetString(rsWin.addr.addr[1], b);
			XmTextFieldSetString(rsWin.addr.addr[2], c);
			XmTextFieldSetString(rsWin.addr.addr[3], d);
		}
                rsWin.update = XtVaCreateManagedWidget("label",
                        xmToggleButtonWidgetClass, routeRC,
                        NULL);
		setLabel(rsWin.update, TXT_updateRoute);

		if (rsInitValues.updateButton) {
			XtVaSetValues(
				rsWin.update,
				XmNset,	True,
				NULL
			);
		}

                createActionArea(
			topRC,
			actions,
                        XtNumber(actions),
			label,
			mneInfo,
			1
		);

                sep2 = XtVaCreateManagedWidget("spe2",
                        xmSeparatorGadgetClass, topRC, NULL);
                rsWin.status = XtVaCreateManagedWidget("label",
                        xmLabelGadgetClass, topRC,
                        NULL);
		setLabel(rsWin.status, "");

		XtVaSetValues(topForm,
			XmNdefaultButton, actions[0].widget,
			XmNcancelButton, actions[2].widget,
			NULL);

		/* add mini-help */

		routerHelp[0].widget = rsWin.status;
		routerHelp[1].widget = rsWin.status;
		routerHelp[2].widget = rsWin.status;
		routerHelp[3].widget = rsWin.status;

		/* subnet type radio buttons */
		for (i = 0; i < 4; i++) {
			XtAddEventHandler(
				subnetRadio.list[i].widget,
				FocusChangeMask,
				False,
				(void(*)())displayFooterMsgEH,
				(XtPointer)&routerHelp[0]
			);
		}
		/* default router */
		XtAddEventHandler(
			rsWin.defaultRouter,
			FocusChangeMask,
			False,
			(void(*)())displayFooterMsgEH,
			(XtPointer)&routerHelp[1]
		);
		/* inet addr */
		for (i = 0; i < 4; i++) {
			XtAddEventHandler(
				rsWin.addr.addr[i],
				FocusChangeMask,
				False,
				(void(*)())displayFooterMsgEH,
				(XtPointer)&routerHelp[2]
			);
		}
		/* update button */
		XtAddEventHandler(
			rsWin.update,
			FocusChangeMask,
			False,
			(void(*)())displayFooterMsgEH,
			(XtPointer)&routerHelp[3]
		);

		REG_MNE(rsWin.popup, mneInfo, XtNumber(actions) + 1);
	} else {
		for (i = 0; i < 4; i++) {
			XtVaSetValues(
				subnetRadio.list[i].widget,
				XmNset,	False,
				NULL
			);
		}
		XtVaSetValues(
			subnetRadio.list[rsInitValues.class].widget,
			XmNset,	True,
			NULL
		);
		string = XmStringCreateLocalized(rsInitValues.networkID);
		XtVaSetValues(
			rsWin.net_id,
			XmNlabelString,	string,
			NULL
		);
		XmStringFree(string);
		string = XmStringCreateLocalized(rsInitValues.hostID);
		XtVaSetValues(
			rsWin.host_id,
			XmNlabelString,	string,
			NULL
		);
		XmStringFree(string);
		if (rsInitValues.class == ClassOther) {
			XtManageChild(rsWin.maskForm);
			XtUnmanageChild(rsWin.netidForm);
		} else {
			XtManageChild(rsWin.netidForm);
			XtUnmanageChild(rsWin.maskForm);
		}
		if (rsInitValues.defaultRouter) {
			XtVaSetValues(
				rsWin.defaultRouter,
				XmNvalue,	rsInitValues.defaultRouter,
				NULL
			);
		}
		if (hi.net.common.isDnsConfigure) {
			XtSetSensitive(rsWin.getaddr, True);
		}
		clearAddr(rsWin.addr);
		if (rsInitValues.routerAddr) {
			char	a[4], b[4], c[4], d[4];

			breakupAddr(rsInitValues.routerAddr, a, b, c, d);
			/*
			XtVaSetValues(rsWin.addr.addr[0], XmNvalue, a, NULL);
			XtVaSetValues(rsWin.addr.addr[1], XmNvalue, b, NULL);
			XtVaSetValues(rsWin.addr.addr[2], XmNvalue, c, NULL);
			XtVaSetValues(rsWin.addr.addr[3], XmNvalue, d, NULL);
			*/
			XmTextFieldSetString(rsWin.addr.addr[0], a);
			XmTextFieldSetString(rsWin.addr.addr[1], b);
			XmTextFieldSetString(rsWin.addr.addr[2], c);
			XmTextFieldSetString(rsWin.addr.addr[3], d);
		}
		if (rsInitValues.updateButton) {
			XtVaSetValues(
				rsWin.update,
				XmNset,	True,
				NULL
			);
		}
	}
        XtManageChild(topForm);
        XtManageChild(rsWin.popup);
	busyCursor(hi.net.common.toplevel, False);
}

static void
createMaskField(Widget parent, char * labelstr)
{
	Widget	label;

	rsWin.maskForm = XtVaCreateWidget("form", xmFormWidgetClass, parent,
		NULL);
	
	rsWin.maskAddr = XtVaCreateManagedWidget("textf",
		xmTextFieldWidgetClass, rsWin.maskForm,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNcolumns,	(short)8,
		XmNmaxLength,	8,
		XmNvalue,	rsInitValues.netmask,
		NULL);
	label = XtVaCreateManagedWidget("label", xmLabelWidgetClass,
		rsWin.maskForm,
		NULL);
	setLabel(label, labelstr);
	XtVaSetValues(label,
		XmNalignment, XmALIGNMENT_END,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, rsWin.maskAddr,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(rsWin.maskAddr, XmNmodifyVerifyCallback,
		verifyHexInputCB, NULL);
}

static void
verifyHexInputCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmTextVerifyCallbackStruct *	cbs = (XmTextVerifyCallbackStruct *)call_data;

	XmTextBlock			textBlock = cbs->text;
	int				i;

	for (i = 0; i < textBlock->length; i++) {
		if (
			(textBlock->ptr[i] < '0') || (
				(textBlock->ptr[i] > '9') &&
				(textBlock->ptr[i] < 'A')
			) || (
				(textBlock->ptr[i] > 'F') &&
				(textBlock->ptr[i] < 'a')
			) ||
			    (textBlock->ptr[i] > 'f')
		) {
			cbs->doit = False;
		} else {
			textBlock->ptr[i] = toupper(textBlock->ptr[i]);
		}
	}
}

static void
classA_CB(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonCallbackStruct *	cbs = (XmToggleButtonCallbackStruct *)call_data;
	char *		x;
	char *		y;
	XmString	xs, ys;

	if (cbs->set) {
		rsNewValues.class = ClassA;
		mungeAddr(hostAddr, &x, &y, ClassA);
		xs = XmStringCreateLocalized(x);
		XtVaSetValues(rsWin.net_id, XmNlabelString, xs, NULL);
		XmStringFree(xs);
		free(x);
		ys = XmStringCreateLocalized(y);
		XtVaSetValues(rsWin.host_id, XmNlabelString, ys, NULL);
		XmStringFree(ys);
		free(y);

		/* update netmask textfield */
		x = strdup(CLASS_A_NETMASK);
		y = x;
		y += 2; /* strip off leading '0x' */
		XtVaSetValues(
			rsWin.maskAddr,
			XmNvalue,	y,
			NULL
		);
		free(x);

		if (XtIsManaged(rsWin.maskForm)) {
			XtUnmanageChild(rsWin.maskForm);
			XtManageChild(rsWin.netidForm);
		}
	}
}

static void
classB_CB(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonCallbackStruct *	cbs = (XmToggleButtonCallbackStruct *)call_data;
	char *		x;
	char *		y;
	XmString	xs, ys;

	if (cbs->set) {
		rsNewValues.class = ClassB;
		mungeAddr(hostAddr, &x, &y, ClassB);
		xs = XmStringCreateLocalized(x);
		XtVaSetValues(rsWin.net_id, XmNlabelString, xs, NULL);
		XmStringFree(xs);
		free(x);
		ys = XmStringCreateLocalized(y);
		XtVaSetValues(rsWin.host_id, XmNlabelString, ys, NULL);
		XmStringFree(ys);
		free(y);

		/* update netmask textfield */
		x = strdup(CLASS_B_NETMASK);
		y = x;
		y += 2; /* strip off leading '0x' */
		XtVaSetValues(
			rsWin.maskAddr,
			XmNvalue,	y,
			NULL
		);
		free(x);

		if (XtIsManaged(rsWin.maskForm)) {
			XtUnmanageChild(rsWin.maskForm);
			XtManageChild(rsWin.netidForm);
		}
	}
}

static void
classC_CB(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonCallbackStruct *	cbs = (XmToggleButtonCallbackStruct *)call_data;
	char *		x;
	char *		y;
	XmString	xs, ys;

	if (cbs->set) {
		rsNewValues.class = ClassC;
		mungeAddr(hostAddr, &x, &y, ClassC);
		xs = XmStringCreateLocalized(x);
		XtVaSetValues(rsWin.net_id, XmNlabelString, xs, NULL);
		XmStringFree(xs);
		free(x);
		ys = XmStringCreateLocalized(y);
		XtVaSetValues(rsWin.host_id, XmNlabelString, ys, NULL);
		XmStringFree(ys);
		free(y);

		/* update netmask textfield */
		x = strdup(CLASS_C_NETMASK);
		y = x;
		y += 2; /* strip off leading '0x' */
		XtVaSetValues(
			rsWin.maskAddr,
			XmNvalue,	y,
			NULL
		);
		free(x);

		if (XtIsManaged(rsWin.maskForm)) {
			XtUnmanageChild(rsWin.maskForm);
			XtManageChild(rsWin.netidForm);
		}
	}
}

static void
classOther_CB(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonCallbackStruct *	cbs = (XmToggleButtonCallbackStruct *)call_data;
	if (cbs->set) {
		XtUnmanageChild(rsWin.netidForm);
		XtManageChild(rsWin.maskForm);
	}
}

/*
 *	return:	0 if success
 *		1 if failure
 */
static int
getInitialValues()
{
	ConfigFile_t *	cfile = &configFile;
	ConfigFile_t *	ifile = &interfaceFile;
	Class_t		class;
	char *		netmask = NULL;
	int		ret;
	AddrKind_t	kindOfRouter;
	char *		router;
	char *		ptr;
	char		buf[MEDIUMBUFSIZE];

	memset(cfile, 0, sizeof(ConfigFile_t));
	memset(ifile, 0, sizeof(ConfigFile_t));
	cfile->cf_kind = ConfigFile;
	cfile->cf_minFields = CF_FIELDS;
	ifile->cf_kind = InterfaceFile;
	ifile->cf_minFields = IF_FIELDS;

	if ((ret = readConfigFile(cfile)) == 1) {
		cleanupConfigFile(cfile);
		return 1;
	}
	if ((ret = readConfigFile(ifile)) == 1) {
		cleanupConfigFile(cfile);
		cleanupConfigFile(ifile);
		return 1;
	}
	if ((ret = getNetmask(ifile, hostAddr, &netmask,
			&(rsInitValues.class))) == 1) {
		return 1;
	}
	ptr = netmask;
	ptr += 2;	/* get rid of leading '0x' */
	rsInitValues.netmask = strdup(ptr);
	free(netmask);

	mungeAddr(hostAddr, &(rsInitValues.networkID), &(rsInitValues.hostID),
		rsInitValues.class);
	
	ret = getDefaultRouter(cfile, &router, &kindOfRouter);
	switch (kindOfRouter) {
		case IsName: {
			rsInitValues.defaultRouter = strdup(router);
			ptr = strchr(rsInitValues.defaultRouter, '.');
			if (ptr)
				*ptr = '\0';
			sprintf(buf, "%s.%s", rsInitValues.defaultRouter,
				hi.net.dns.resolv->domain);
			ret = getAddrFromName(buf,
				&(rsInitValues.routerAddr));
			if (ret) {
				if (rsInitValues.routerAddr) {
					free(rsInitValues.routerAddr);
					rsInitValues.routerAddr = NULL;
				}
			}
			getDotStrToADDR(
				rsInitValues.routerAddr,
				rsInitValues.hexAddr
			);
			break;
		}
		case IsIPAddr: {
			rsInitValues.routerAddr = strdup(router);
			getDotStrToADDR(
				rsInitValues.routerAddr,
				rsInitValues.hexAddr
			);
			getNameFromAddr(rsInitValues.routerAddr,
				&(rsInitValues.defaultRouter));
			ptr = strchr(rsInitValues.defaultRouter, '.');
			if (ptr)
				*ptr = '\0';
			break;
		}
		default: {
			break;
		}
	}
	(void)findProcess(INROUTEDPROC, &rsInitValues.routeProc);
	if (rsInitValues.routeProc == -1) {
		rsInitValues.updateButton = False;
	} else {
		rsInitValues.updateButton = True;
	}

	free(router);

	return 0;
}

static void
bailOut()
{
	if (hostAddr) {
		free(hostAddr);
	}
	cleanupConfigFile(&configFile);
	cleanupConfigFile(&interfaceFile);
	freeValues(&rsInitValues);
	freeValues(&rsNewValues);

	return;
}

static void
getaddrCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	char	buf[MEDIUMBUFSIZE];
	char *	routerName = NULL;
	char *	routerAddr = NULL;
	char *	haddr = NULL;
	char *	ptr;

	XtVaGetValues(
		rsWin.defaultRouter,
		XmNvalue,
		&routerName,
		NULL
	);
	removeWhitespace(routerName);
	ptr = strchr(routerName, '.');
	if (ptr)
		*ptr = '\0';
	sprintf(buf, "%s.%s", routerName, hi.net.dns.resolv->domain);
	if (
		(populateAddr(
			rsWin.popup,
			rsWin.getaddr,
			rsWin.addr,
			&routerAddr,
			buf,
			&haddr
		)) == False
	) {
		sprintf(buf, mygettxt(ERR_cantGetAddr), routerName);
		createErrorMsg(
			hi.net.common.toplevel,
			INFO,
			buf,
			TXT_appName
		);
	}
	if (haddr) free(haddr);
	if (routerName) free(routerName);
	if (routerAddr) free(routerAddr);
}

static void
createErrorMsg(Widget parent, msgType type, char * msg, char * title)
{
	static Widget		msgDialog;
	XmString	text;

	if (!msgDialog) {
		msgDialog = XmCreateMessageDialog(parent,
			"messagedialog", NULL, 0);
		XtVaSetValues(
			XtParent(msgDialog),
			XmNtitle,
			mygettxt(title),
			NULL
		);
		XtAddCallback(msgDialog, XmNokCallback, msgClearCB, NULL);
		XtUnmanageChild(
			XmMessageBoxGetChild(
				msgDialog,
				XmDIALOG_CANCEL_BUTTON
			)
		);
		XtUnmanageChild(
			XmMessageBoxGetChild(
				msgDialog,
				XmDIALOG_HELP_BUTTON
			)
		);
	}
	switch (type) {
		case INFO:
			XtVaSetValues(
				msgDialog,
				XmNdialogType,
				XmDIALOG_INFORMATION,
				NULL
			);
			break;
		case ERROR:
			XtVaSetValues(msgDialog,
				XmNdialogType,
				XmDIALOG_ERROR,
				NULL
			);
			break;
		case WARN:
			XtVaSetValues(msgDialog,
				XmNdialogType,
				XmDIALOG_WARNING,
				NULL
			);
			break;
	}
	text = XmStringCreateLtoR(msg, XmFONTLIST_DEFAULT_TAG);
	XtVaSetValues(
		msgDialog,
		XmNmessageString,	text,
		NULL
	);
	XmStringFree(text);
	XtManageChild(msgDialog);
}

static void
msgClearCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	XtUnmanageChild(w);
}

static void
nameChangedCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	char *	name;

	name = XmTextFieldGetString(rsWin.defaultRouter);
	removeWhitespace(name);
	if (strlen(name) > 0) {
		if (hi.net.common.isDnsConfigure) {
			XtSetSensitive(rsWin.getaddr, True);
		}
	} else {
		XtSetSensitive(rsWin.getaddr, False);
		clearAddr(rsWin.addr);
	}
	XtFree(name);

	return;
}

void
freeValues(Values_t * v)
{
	if (v->netmask) free(v->netmask);
	if (v->networkID) free(v->networkID);
	if (v->hostID) free(v->hostID);
	if (v->defaultRouter) free(v->defaultRouter);
	if (v->routerAddr) free(v->routerAddr);
	memset(v, 0, sizeof(Values_t));
}
