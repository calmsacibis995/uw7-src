#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:internet/uucp.c	1.26"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netdb.h>		/* for struct hostent */
#include <sys/socket.h>		/* for AF_INET */
#include <netinet/in.h>		/* for struct in_addr */
#include <arpa/inet.h>		/* for inet_ntoa() */
#include <sys/byteorder.h>
#include <libgen.h>

#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/DialogS.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/SeparatoG.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/TextF.h>
#include <Xm/MessageB.h>
#include <X11/cursorfont.h>

#include "DesktopP.h"

#include "lookup.h"
#include "lookupG.h"
#include "util.h"
#include "inetMsg.h"
#include "inet.h"

#define ALLOC_SIZE		50
#define GOBBLEWHITE(p)		while (isspace(*p)) ++p
#define TRAVERSETOKEN(p)	while (*p != 0 && !isspace(*p)) ++p

typedef struct _info_msg {
	char *	msg;
	msgType	msgType;
	char *	okTitle;
	funcdef	okFcn;
	char *	cancelTitle;
	funcdef	cancelFcn;
	Boolean	isHelpAvailable;
} InfoMsg_t;

enum { FromList, FromUser };
	
extern void	createMsg(Widget, msgType, char *, char *);
extern Boolean	removeWhitespace(char *);
extern void	helpkeyCB(Widget, XtPointer, XtPointer);
extern char *	strndup(char *, int);
extern Boolean	getAddrToDotStr(inetAddr, char *);
extern char *	mygettxt(char *);
extern int	r_decor(Widget);

void	clearAddr(inetAddr);
void	setAddrGui(inetAddr, unsigned char *);
int	populateAddr(Widget, Widget, inetAddr, char **, char *, char **);
int	writeUucpFile(Boolean);
void	createUucpTransfer(Widget, XtPointer, XmAnyCallbackStruct *);
void	createInfoMsg(Widget, InfoMsg_t *);

static void	clearForm(void);
static void	clearFooter(Widget);
static void	setFooter(Widget, char *);
static void	nameChangedCB(Widget, XtPointer, XmAnyCallbackStruct *);
static void	clearStatus(void);
static void	setStatus(void);
static void	busyCursor(Widget, int);
static void	appendUucpList(void);
static void	deleteUucpList(int);
static void	getaddrCB(Widget, XtPointer, XtPointer);
static void	acceptCB(Widget, XtPointer, XtPointer);
static void	rejectCB(Widget, XtPointer, XtPointer);
static void	cancelCB(Widget, XtPointer, XtPointer);
static void	dot2hex(ADDR, char *) ;
static void	hex2dot(char *, char *);
static void	initUucp(void);
static void	setupGui(Widget, XtPointer, XmAnyCallbackStruct *);
static int	isInSystemsList(char *);
static void	freeSystemsEntry(systemsEntry);
static void	freeUucpList(systemsList *);
static int	readUucpFile(systemsList *);
static void	decideScreenLayout(void);
static void	cleanupUucp(void);
static void	breakupName(char *, int);
static void	yesCB(Widget, XtPointer, XtPointer);
static void	noCB(Widget, XtPointer, XtPointer);

static char *title;
static char *	pattern = "Any TcpCico10103 - \\x00020ace";
static char *	re = "TcpCico10103.*\\x00020[aA][cC][eE]";

HelpText	UUCPHelp = { NULL, NULL, "150" };

static ActionAreaItems actions[] = {
	{ TXT_accept,	MNE_accept,	acceptCB,	NULL },
	{ TXT_reject,	MNE_reject,	rejectCB,	NULL },
	{ TXT_Cancel,	MNE_Cancel,	cancelCB,	NULL },
	{ TXT_Help,	MNE_Help,	helpkeyCB,	&UUCPHelp }
};

static char    *uucp_form[]={
	TXT_cur_status,
	TXT_system_name,
	TXT_inet_addr,
        "",
};

typedef struct _uucp {
	/* gui */
	Widget			popup;
	Widget			curStatus;
	Widget			topForm;
	Widget			sysName;
	Widget			getaddr;
	inetAddr		addr;
	ActionAreaItems *	actions;
	Widget			footer;
	/* non-gui */
	char *			systemName_C;
	char *			systemName_N;
	char *			namePart_C;
	char *			namePart_N;
	char *			domainPart_C;
	char *			domainPart_N;
	char *			haddr;
} uucp;

uucp uucpWin;

int	duplicateEntry;

static void
clearFooter(Widget footer)
{
	setLabel(footer, " ");
}

static void
setFooter(Widget footer, char * msg)
{
	char	buf[MEDIUMBUFSIZE];

	sprintf(buf, mygettxt(msg), uucpWin.namePart_C);
	setLabel(footer, buf);
}

void
clearAddr(inetAddr addr)
{
	XtVaSetValues(addr.addr[ADDR1], XmNvalue, "", NULL);
	XtVaSetValues(addr.addr[ADDR2], XmNvalue, "", NULL);
	XtVaSetValues(addr.addr[ADDR3], XmNvalue, "", NULL);
	XtVaSetValues(addr.addr[ADDR4], XmNvalue, "", NULL);
}

static void
nameChangedCB(Widget w, XtPointer client_data, XmAnyCallbackStruct * cbs)
{
	char *	name;

	name = XmTextFieldGetString(uucpWin.sysName);
	if (cbs->reason == XmCR_VALUE_CHANGED) {
		if (strlen(name) > 0) {
			if (hi.net.common.isDnsConfigure) {
				XtSetSensitive(uucpWin.getaddr, True);
			}
		} else {
			XtSetSensitive(uucpWin.getaddr, False);
			clearAddr(uucpWin.addr);
			clearStatus();
			clearFooter(uucpWin.footer);
		}
		XtFree(name);

		return;
	}
	/* else, must be XmCR_LOSING_FOCUS... */
	removeWhitespace(name);
	if (strlen(name) > 0) {
		if (strcmp(name, uucpWin.systemName_C) != 0) {
			breakupName(name, FromUser);
			if (hi.net.common.isDnsConfigure) {
				clearAddr(uucpWin.addr);
				clearStatus();
			} else {
				/* try to set status */
				clearAddr(uucpWin.addr);
				setStatus();
			}
		}
	} else {
		XtVaSetValues(uucpWin.getaddr, XmNsensitive, False, NULL);
		clearAddr(uucpWin.addr);
		clearStatus();
	}
	clearFooter(uucpWin.footer);
	XtFree(name);
}

static void
clearStatus(void)
{
	setLabel(uucpWin.curStatus, "");
}

static void
setStatus(void)
{
	char	buf[MEDIUMBUFSIZE];
	Widget	defwid, prewid;

	InfoMsg_t	im = {
		NULL,
		WARN,
		TXT_Yes,
		yesCB,
		TXT_No,
		noCB,
		False
	};
	char	addr[MEDIUMBUFSIZE];
		

	if ((duplicateEntry = isInSystemsList(uucpWin.namePart_C)) != -1) {
		if (hi.net.common.isDnsConfigure) {
			if (
				(strcmp(
					uucpWin.haddr,
					hi.inet.uucp->list[duplicateEntry].haddr
				))
			) {
				hex2dot(
					hi.inet.uucp->list[duplicateEntry].haddr,
					addr
				);
				sprintf(
					buf,
					mygettxt(WARN_uucpEntryExists),
					uucpWin.namePart_C,
					addr
				);
				im.msg = buf;
				createInfoMsg(hi.net.common.toplevel, &im);
			}
		} else {
			ADDR	addr;
			char	buf[MEDIUMBUFSIZE];

			hex2dot(
				hi.inet.uucp->list[duplicateEntry].haddr,
				buf
			);
			getDotStrToADDR(buf, addr);
			setAddrGui(uucpWin.addr, addr);
		}
		sprintf(buf, mygettxt(TXT_accepted2), uucpWin.namePart_C);
		defwid = actions[0].widget;
		prewid = actions[1].widget;
	} else {
		defwid = actions[1].widget;
		prewid = actions[0].widget;
		sprintf(buf, mygettxt(TXT_rejected2), uucpWin.namePart_C);
	}
	XtVaSetValues(prewid,
		XmNshowAsDefault, False,
		NULL);
	XtVaSetValues(uucpWin.topForm,
		XmNdefaultButton, defwid,
		NULL);
	XtVaSetValues(defwid,
		XmNshowAsDefault, True,
		NULL);
	setLabel(uucpWin.curStatus, buf);
}

static void
busyCursor(Widget w, int on)
{
	static Cursor cursor;
	XSetWindowAttributes attrs;
	Display *dpy = XtDisplay(w);
	Widget	shell = w;

	while(!XtIsShell(shell))
		shell = XtParent(shell);

	if (!cursor) /* make sure the timeout cursor is initialized */
		cursor = XCreateFontCursor(dpy, XC_watch);

	attrs.cursor = on? cursor : None;
	XChangeWindowAttributes(dpy, XtWindow(shell), CWCursor, &attrs);

	XFlush(dpy);
}


static void
appendUucpList(void)
{
	if (hi.inet.uucp->count % ALLOC_SIZE == 0) {
		hi.inet.uucp->list = (systemsEntry *)realloc((systemsEntry *)hi.inet.uucp->list, (hi.inet.uucp->count + ALLOC_SIZE) * sizeof(systemsEntry));

		if (hi.inet.uucp->list == NULL) {
			createMsg(hi.net.common.toplevel, ERROR, mygettxt(ERR_memory), title);
			return;
		}
	}
	hi.inet.uucp->list[hi.inet.uucp->count].name
		= strdup(uucpWin.namePart_C);
	hi.inet.uucp->list[hi.inet.uucp->count].haddr
		= strdup(uucpWin.haddr);
	hi.inet.uucp->list[hi.inet.uucp->count].cmt
		= strdup(pattern);
	hi.inet.uucp->list[hi.inet.uucp->count].preserve
		= NULL;
	hi.inet.uucp->count++;
}

static void
deleteUucpList(int index)
{
	if (hi.inet.uucp->count > 1) {
		if (hi.inet.uucp->count == index + 1) {
			freeSystemsEntry(hi.inet.uucp->list[index]);
			hi.inet.uucp->count--;

			return;
		}
		freeSystemsEntry(hi.inet.uucp->list[index]);
		(void)memmove(
			(void *)&(hi.inet.uucp->list[index]),
			(void *)&(hi.inet.uucp->list[index + 1]),
			sizeof(systemsEntry)*(hi.inet.uucp->count - index - 1)
		);
		hi.inet.uucp->list[hi.inet.uucp->count].name = NULL;
		hi.inet.uucp->list[hi.inet.uucp->count].haddr = NULL;
		hi.inet.uucp->list[hi.inet.uucp->count].cmt = NULL;
		hi.inet.uucp->list[hi.inet.uucp->count].preserve = NULL;
		hi.inet.uucp->count--;

		return;
	}
	if (hi.inet.uucp->count == 1) {
		freeSystemsEntry(hi.inet.uucp->list[index]);
		hi.inet.uucp->count--;

		return;
	}
}

void
setAddrGui(inetAddr addr, unsigned char * p)
{
	char	tmpaddr[MEDIUMBUFSIZE];

	sprintf(tmpaddr,"%d", p[ADDR1]);
	/*
	XtVaSetValues(addr.addr[ADDR1],
		XmNvalue, tmpaddr, NULL);
	*/
	XmTextFieldSetString(addr.addr[ADDR1], tmpaddr);
	sprintf(tmpaddr,"%d", p[ADDR2]);
	/*
	XtVaSetValues(addr.addr[ADDR2],
		XmNvalue, tmpaddr, NULL);
	*/
	XmTextFieldSetString(addr.addr[ADDR2], tmpaddr);
	sprintf(tmpaddr,"%d", p[ADDR3]);
	/*
	XtVaSetValues(addr.addr[ADDR3],
		XmNvalue, tmpaddr, NULL);
	*/
	XmTextFieldSetString(addr.addr[ADDR3], tmpaddr);
	sprintf(tmpaddr,"%d", p[ADDR4]);
	/*
	XtVaSetValues(addr.addr[ADDR4],
		XmNvalue, tmpaddr, NULL);
	*/
	XmTextFieldSetString(addr.addr[ADDR4], tmpaddr);
}

int
populateAddr(
	Widget		popup,
	Widget		getaddr,
	inetAddr	addr,
	char **		addr_C,
	char *		hostname,
	char **		haddrIn
) {
	struct hostent	*hostptr;
	struct in_addr	*ptr;
	u_long		theaddr;
	unsigned char	*p;
	Boolean		sensitive;
	char		haddr[MEDIUMBUFSIZE];
	
	/* populate the address */
	busyCursor(popup, True);
	if (getaddr != NULL) {
		if (XtIsSensitive(getaddr)) {
			XtSetSensitive(getaddr, False);
		}
	}
	if ((hostptr = gethostbyname(hostname)) != NULL) {
		while ((ptr = (struct in_addr *)*hostptr->h_addr_list++) != NULL) {
			if (*addr_C) free(*addr_C);
			*addr_C = strdup(inet_ntoa(*ptr));
			theaddr = inet_addr(inet_ntoa(*ptr));
			p = (unsigned char *)&theaddr;
			dot2hex(p, &haddr[0]);
			if (*haddrIn) free(*haddrIn);
			*haddrIn = strdup(haddr);
		}
		/* the dot notation is stored in unsigned char, 
	 	 * we need to convert them into a string for the XtSetValues.
		 */
		setAddrGui(addr, p);
		if (getaddr != NULL) {
			if (
				(!XtIsSensitive(getaddr)) &&
				(hi.net.common.isDnsConfigure)
			) {
				XtSetSensitive(getaddr, True);
			}
		}
		busyCursor(popup, False);
		return True;
	}
	else {
		if (getaddr != NULL) {
			if (
				(!XtIsSensitive(getaddr)) &&
				(hi.net.common.isDnsConfigure)
			) {
				XtSetSensitive(getaddr, True);
			}
		}
		busyCursor(popup, False);
		return False;
	}
}

static void
getaddrCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	char *	addr = NULL;
	char	buf[MEDIUMBUFSIZE];

	if (uucpWin.haddr) {
		free(uucpWin.haddr);
		uucpWin.haddr = NULL;
	}
	if (hi.net.common.isDnsConfigure) {
		sprintf(
			buf,
			"%s.%s",
			uucpWin.namePart_C,
			uucpWin.domainPart_C
		);
	} else {
		strcpy(buf, uucpWin.systemName_C);
	}
	if (
		(populateAddr(
			uucpWin.popup,
			uucpWin.getaddr,
			uucpWin.addr,
			&addr,
			buf,
			&uucpWin.haddr
		) == True)
	) {
		setStatus();
	} else {
		sprintf(buf, mygettxt(ERR_cantGetAddr), uucpWin.systemName_C);
		createMsg(hi.net.common.toplevel, ERROR, buf, title);
	}
	if (addr) free(addr);

	return;
}

static void 
acceptCB(Widget w, XtPointer client_data, XtPointer call_data) 
{
	char		*addr1, *addr2, *addr3, *addr4, c_addr[MEDIUMBUFSIZE];
	char		haddr[MEDIUMBUFSIZE], buf[MEDIUMBUFSIZE];
	u_long		theaddr;
	unsigned char *	p;
	int		index;

	if (
		(strlen(uucpWin.namePart_C) == 0) ||
		(getAddrToDotStr(uucpWin.addr, c_addr) == False)
	) {
		createMsg(hi.net.common.toplevel, ERROR, mygettxt(ERR_uucpError1), title);
		return;
	}
	/* store the hex value of the address */
	theaddr = inet_addr(c_addr);	
	p = (unsigned char *)&theaddr;
	dot2hex(p, &haddr[0]);
	uucpWin.haddr = strdup(haddr);
	if ((index = isInSystemsList(uucpWin.namePart_C)) == -1) {
		/* add to the internal list */
		appendUucpList();
		/* write back to the system file */
		writeUucpFile(False);
	} else {
		setFooter(uucpWin.footer, TXT_inUucpFile2);
		return;
	}
	cleanupUucp();
        XtUnmanageChild(uucpWin.popup);
}

static void
rejectCB(Widget w, XtPointer client_data, XtPointer call_data) 
{
	char		*addr1, *addr2, *addr3, *addr4, c_addr[MEDIUMBUFSIZE];
	char		haddr[MEDIUMBUFSIZE], buf[MEDIUMBUFSIZE];
	u_long		theaddr;
	unsigned char *	p;
	int		index;

	if (strlen(uucpWin.namePart_C) == 0) {
		createMsg(hi.net.common.toplevel, ERROR, mygettxt(INFO_systemNeed2), title);
		return;
	}
	if ((index = isInSystemsList(uucpWin.namePart_C)) == -1) {
		setFooter(uucpWin.footer, TXT_notInUucpFile2);
		return;
	}
	deleteUucpList(index);
	writeUucpFile(False);
	cleanupUucp();
        XtUnmanageChild(uucpWin.popup);
}

static void
cancelCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	cleanupUucp();
        XtUnmanageChild(uucpWin.popup);
}

static void
dot2hex(ADDR inetAddr, char * haddr) 
{
	sprintf(haddr, "%.2x%.2x%.2x%.2x", inetAddr[0], inetAddr[1], inetAddr[2], inetAddr[3]);
}

static void
hex2dot(char * haddr, char * addr)
{
	unsigned long	y;
	struct in_addr	x;
	y = strtoul(haddr, (char **)NULL, 16);
	x.s_addr = htonl(y);
	strcpy(addr, inet_ntoa(x));
}

static void
initUucp(void)
{
	if (hi.inet.uucp == NULL)
		hi.inet.uucp = (systemsList *)calloc(1, sizeof(systemsList));
}

static void
setupGui(Widget w, XtPointer client_data, XmAnyCallbackStruct *cbs)
{

	static	Widget sys_rc;
	Widget	topRC, label;
	Widget	labelForm;
	int	maxWidth, index;
	Widget	sep1, sep2;
	char	buf[MEDIUMBUFSIZE];	
	DmMnemonicInfoRec	mneInfo[MI_TOTAL];
	unsigned char *		mne;
	char			tmpstr[BUFSIZ];
	KeySym			mneks;
	XmString		mneString;
	int			mm;

	if(!uucpWin.popup){
		maxWidth = GetMaxWidth(XtParent(w), uucp_form);
		uucpWin.popup = XtVaCreatePopupShell("uucpTransfer",
                        xmDialogShellWidgetClass, XtParent(w),
                        NULL);
		title = mygettxt(TXT_uucpView);
                XtVaSetValues(uucpWin.popup,
                        XmNtitle, mygettxt(TXT_uucpView),
                        XmNallowShellResize, True,
			XmNdeleteResponse, XmUNMAP,
                        NULL);

		r_decor(uucpWin.popup);

		uucpWin.topForm = XtVaCreateWidget("nsa_topForm",
			xmFormWidgetClass, uucpWin.popup,
			XmNresizePolicy, XmRESIZE_ANY,
			NULL);

                topRC = XtVaCreateManagedWidget("nsa_rowColumn",
                        xmRowColumnWidgetClass, uucpWin.topForm,
                        XmNorientation, XmVERTICAL,
                        NULL);
		
		XtAddCallback(topRC, XmNhelpCallback, helpkeyCB, &UUCPHelp);

		labelForm = XtVaCreateManagedWidget(
			"form", xmFormWidgetClass, topRC,
			NULL
		);
		label = XtVaCreateManagedWidget(
			"label", xmLabelWidgetClass, labelForm,
			XmNalignment,		XmALIGNMENT_BEGINNING,
			XmNtopAttachment,	XmATTACH_FORM,
			XmNrightAttachment,	XmATTACH_FORM,
			XmNleftAttachment,	XmATTACH_FORM,
			XmNbottomAttachment,	XmATTACH_FORM,
                        NULL
		);
		setLabel(label, TXT_uucpLabel);
		sys_rc = XtVaCreateManagedWidget("sys_rc",
			xmRowColumnWidgetClass, topRC,
			XmNorientation, XmHORIZONTAL, NULL);
                uucpWin.sysName = (Widget) crt_inputlabel(sys_rc, TXT_system_name, maxWidth);
		XtAddCallback(uucpWin.sysName, XmNlosingFocusCallback,
			(void(*)())nameChangedCB, NULL);
		XtAddCallback(uucpWin.sysName, XmNvalueChangedCallback,
			(void(*)())nameChangedCB, NULL);
		strcpy(tmpstr, mygettxt(TXT_getAddr));
		uucpWin.getaddr = XtVaCreateManagedWidget(tmpstr,
			xmPushButtonWidgetClass, sys_rc, NULL);
		XmSTR_N_MNE(
			TXT_getAddr,
			MNE_getAddr,
			mneInfo[0],
			DM_B_MNE_ACTIVATE_CB | DM_B_MNE_GET_FOCUS
		);
		XmStringFree(mneString);
		XtVaSetValues(uucpWin.getaddr, XmNmnemonic, mneks, NULL);
		XtAddCallback(uucpWin.getaddr, XmNactivateCallback, 
			(void(*)())getaddrCB, NULL);
		mneInfo[0].w = uucpWin.getaddr;
		mneInfo[0].cb = (XtCallbackProc)getaddrCB;
		crt_inet_addr(topRC, TXT_inet_addr, maxWidth, &(uucpWin.addr));

		uucpWin.curStatus = (Widget) crt_inputlabellabel(topRC,
				mygettxt(TXT_cur_status), maxWidth, " ");
		
                sep1 = XtVaCreateManagedWidget("spe1",
                        xmSeparatorGadgetClass, topRC, NULL);
                createActionArea(
			topRC,
			actions,
                        XtNumber(actions),
			sys_rc,
			mneInfo,
			1
		);
                sep2 = XtVaCreateManagedWidget("spe2",
                        xmSeparatorGadgetClass, topRC, NULL);
		XtVaSetValues(uucpWin.topForm,
			XmNcancelButton, actions[2].widget,
			NULL);
			/* XmNdefaultButton, actions[1].widget, */
		initUucp();
		uucpWin.footer = XtVaCreateManagedWidget("uucpWin_footer",
			xmLabelWidgetClass, topRC, NULL);
		clearForm();
		decideScreenLayout();

		REG_MNE(uucpWin.popup, mneInfo, XtNumber(actions) + 1);
	}
	else {
		if (hi.net.common.cur_view == etcHost) {
			index=((hi.net.etc.etcHostIndex+(ETCCOLS-1))/ETCCOLS)-2;
			if (strcmp(uucpWin.systemName_C,
				hi.net.etc.etcHosts->list[index].etcHost.name) != 0) {
				clearForm();
				decideScreenLayout();
			}
		}
		else {
			if (strcmp(uucpWin.systemName_C,
				hi.net.dns.dnsSelection[hi.net.dns.cur_wid_pos]) != 0) {
				clearForm();
				decideScreenLayout();
			}
		}
	}
        XtManageChild(uucpWin.topForm);
        XtManageChild(uucpWin.popup);
}

int
isInSystemsList(char * hostName)
{
	int	i;

	for (i = 0; i< hi.inet.uucp->count; i++) {
		if (! strcmp(hi.inet.uucp->list[i].name, hostName))
			return i;
	}
	return -1;
}

static void
freeSystemsEntry(systemsEntry entry)
{
	if (entry.name) {
		free(entry.name);
		entry.name = NULL;
	}
	if (entry.haddr) {
		free(entry.haddr);
		entry.haddr = NULL;
	}
	if (entry.cmt) {
		free(entry.cmt);
		entry.cmt = NULL;
	}
	if (entry.preserve) {
		free(entry.preserve);
		entry.preserve = NULL;
	}
}

static void
freeUucpList(systemsList * ptr)
{
	int	i;
	
	if (ptr == NULL || ptr->list == NULL)
		return;
	for (i = 0; i < ptr->count; i++) {
		freeSystemsEntry(ptr->list[i]);
	}
	free(ptr->list);
	ptr->list = NULL;
	ptr->count = 0;
}

int
writeUucpFile(Boolean firstTime)
{
	static FILE	*fd;
	int		i;
	char		buf[MEDIUMBUFSIZE];
	char		haddr[MEDIUMBUFSIZE];
	char *		p1;
	char *		p2;

	if ((fd = fopen(uucpPath, "w")) == NULL) {
		sprintf(buf, mygettxt(ERR_cantOpenFile), uucpPath);
		createMsg(hi.net.common.toplevel, ERROR, buf, title);
		return;
	}

	if (firstTime) {
		for (i = 0; i < hi.net.etc.etcHosts->count; i++) {

			if (! strcmp(hi.net.etc.etcHosts->list[i].etcHost.name, "")) {
				continue;
			}
			/* strip off domain information */
			p1 = strdup(hi.net.etc.etcHosts->list[i].etcHost.name);
			p2 = strchr(p1, '.');
			if (p2)
				*p2 = '\0';
			dot2hex(
				hi.net.etc.etcHosts->list[i].etcHost.addr,
				&haddr[0]
			);
			sprintf(buf, "%s\t\t%s%s\n",
				p1,
				pattern,
				haddr
			);
			fputs(buf, fd);
			free(p1);
		}
		fclose(fd);
		return;
	}
	for (i = 0; i < hi.inet.uucp->count; i++) {
		if (hi.inet.uucp->list[i].preserve) {
			fputs(hi.inet.uucp->list[i].preserve, fd);
		} else {
			p1 = strdup(hi.inet.uucp->list[i].name);
			p2 = strchr(p1, '.');
			if (p2)
				*p2 = '\0';
			sprintf(buf, "%s\t\t%s%s\n",
				p1,
				hi.inet.uucp->list[i].cmt,
				hi.inet.uucp->list[i].haddr);
			fputs(buf, fd);
			free(p1);
		}
	}
	fclose(fd);
}

int
readUucpFile(systemsList *list)
{
	static FILE	*fd;
	struct stat	statBuffer;
	char		buf[MEDIUMBUFSIZE];
	char		*host, *p;
	char		*cmt, *haddr;	
	int		i=0, j=0, len;
	Boolean		found=False;
	static time_t	lastRead = 0;
	char *		regc;
	int		ret;

	while ((ret = stat(uucpPath, &statBuffer)) < 0 && errno == EINTR)
		/* try again */;
	if (ret == -1 && errno == ENOENT) {
		return False;
	}
	while (!(fd = fopen(uucpPath, "r")) && errno == EINTR)
		/* try again */;
	if (!fd) {
		sprintf(buf, mygettxt(ERR_cantOpenFile), uucpPath);
		createMsg(hi.net.common.toplevel, ERROR, buf, title);
		return False;
	}
	if (statBuffer.st_mtime > lastRead) {
		lastRead = statBuffer.st_mtime;
		freeUucpList(hi.inet.uucp);
	}
	else {
		fclose(fd);
		return True;
	}
	regc = regcmp(re, (char *)0);
	while (fgets (buf, MEDIUMBUFSIZE, fd)) {
		char *	p2;
		char *	p3;

		p = buf;
		GOBBLEWHITE(p);
		if (*p == '#') {
			continue;
		} else if ((!(*p)) || (*p == '\n')) {
			continue;
		}
		p2 = p;
		TRAVERSETOKEN(p2);
		host = strndup(p, p2-p);
		GOBBLEWHITE(p2);
		if (! (p3 = regex(regc, p2))) {
			free(host);
			if (hi.inet.uucp->count % ALLOC_SIZE == 0) {
				hi.inet.uucp->list = (systemsEntry *)realloc((systemsEntry *)hi.inet.uucp->list, (hi.inet.uucp->count + ALLOC_SIZE) * sizeof(systemsEntry));
				if (hi.inet.uucp->list == NULL) {
					createMsg(hi.net.common.toplevel,
						ERROR, mygettxt(ERR_memory),
						title);
					return Failure;
				}
			}	
			hi.inet.uucp->list[hi.inet.uucp->count].name = NULL;
			hi.inet.uucp->list[hi.inet.uucp->count].haddr = NULL;
			hi.inet.uucp->list[hi.inet.uucp->count].cmt = NULL;
			hi.inet.uucp->list[hi.inet.uucp->count].preserve = strdup(buf);
			hi.inet.uucp->count++;	
			continue;
		}
		haddr = strndup(p3, 8);
		*p3 = '\0';
		cmt = strdup(p2);

		/* store info into the data structure */
		if (hi.inet.uucp->count % ALLOC_SIZE == 0) {
			hi.inet.uucp->list = (systemsEntry *)realloc((systemsEntry *)hi.inet.uucp->list, (hi.inet.uucp->count + ALLOC_SIZE) * sizeof(systemsEntry));

			if (hi.inet.uucp->list == NULL) {
				createMsg(hi.net.common.toplevel,
					ERROR, mygettxt(ERR_memory),
					title);
				return Failure;
			}
		}	
		hi.inet.uucp->list[hi.inet.uucp->count].name = strdup(host);
		free(host);
		hi.inet.uucp->list[hi.inet.uucp->count].haddr = strdup(haddr);
		free(haddr);
		hi.inet.uucp->list[hi.inet.uucp->count].cmt = strdup(cmt);
		free(cmt);
		hi.inet.uucp->list[hi.inet.uucp->count].preserve = NULL;
		hi.inet.uucp->count++;	
	}
	free(regc);
	fclose(fd);
}

static void
decideScreenLayout(void)
{
	char		*host, tmpaddr[MEDIUMBUFSIZE];
	struct hostent	*hostptr;
	struct in_addr	*ptr;
	u_long		theaddr;
	unsigned char	*p;
	int		index;
	char *		addr = NULL;
	char *		tmp;
	char *		name;
	char		buf[MEDIUMBUFSIZE];

	/* read in the Systems.tcp file into the internal data structure */
	readUucpFile(hi.inet.uucp);
	/* Get the current selection */
	if (hi.net.common.cur_view == etcHost) { 
		index=((hi.net.etc.etcHostIndex+(ETCCOLS-1))/ETCCOLS)-2;
		name = strdup(hi.net.etc.etcHosts->list[index].etcHost.name);
	} else {
		name = strdup(hi.net.dns.dnsSelection[hi.net.dns.cur_wid_pos]);
	}
	breakupName(name, FromList);
	free(name);
	/* if DNS is not configured, grey out Get Address button */
	if (hi.net.common.isDnsConfigure == False) {
		XtVaSetValues(uucpWin.getaddr, XmNsensitive, False, NULL);
		strcpy(buf, uucpWin.systemName_C);
	} else {
		XtVaSetValues(uucpWin.getaddr, XmNsensitive, True, NULL);
		sprintf(buf, "%s.%s", uucpWin.namePart_C, uucpWin.domainPart_C);
	}
	/* populate the address */
	if (uucpWin.haddr) free(uucpWin.haddr);
	if (
		(populateAddr(
			uucpWin.popup,
			uucpWin.getaddr,
			uucpWin.addr,
			&addr,
			buf,
			&uucpWin.haddr
		) == True)
	) {
		/* check the current status in uucp file */
		setStatus();
	}
	clearFooter(uucpWin.footer);
	if (addr) free(addr);

	return;
}

void
createUucpTransfer(Widget w, XtPointer client_data, XmAnyCallbackStruct * cbs)
{
	setupGui(w, client_data, cbs);
}

static void
clearForm(void)
{
	clearFooter(uucpWin.footer);
	clearStatus();
	clearAddr(uucpWin.addr);
	XmTextSetString(uucpWin.sysName, "");
}

static void
cleanupUucp(void)
{
	clearFooter(uucpWin.footer);
	clearStatus();
	clearAddr(uucpWin.addr);
	XmTextSetString(uucpWin.sysName, "");
	if (uucpWin.systemName_C) free(uucpWin.systemName_C);
	if (uucpWin.systemName_N) free(uucpWin.systemName_N);
	if (uucpWin.namePart_C) free(uucpWin.namePart_C);
	if (uucpWin.namePart_N) free(uucpWin.namePart_N);
	if (uucpWin.domainPart_C) free(uucpWin.domainPart_C);
	if (uucpWin.domainPart_N) free(uucpWin.domainPart_N);
	if (uucpWin.haddr) free(uucpWin.haddr);
	uucpWin.systemName_C = NULL;
	uucpWin.systemName_N = NULL;
	uucpWin.namePart_C = NULL;
	uucpWin.namePart_N = NULL;
	uucpWin.domainPart_C = NULL;
	uucpWin.domainPart_N = NULL;
	uucpWin.haddr = NULL;
}

static void
breakupName(char * name, int origin)
{
	char *		p;
	char		buf[MEDIUMBUFSIZE];
	dnsList *	cur_pos;

	if (uucpWin.domainPart_C) free(uucpWin.domainPart_C);
	if (uucpWin.namePart_C) free(uucpWin.namePart_C);
	if (uucpWin.systemName_C) free(uucpWin.systemName_C);
	uucpWin.systemName_C = NULL;
	uucpWin.namePart_C = NULL;
	uucpWin.domainPart_C = NULL;
	uucpWin.systemName_C = strdup(name);
	if (hi.net.common.isDnsConfigure) {
		p = strchr(name, '.');
		if (p) {
			*p = '\0';
			uucpWin.namePart_C = strdup(name);
			uucpWin.domainPart_C = strdup(++p);
			XtVaSetValues(
				uucpWin.sysName,
				XmNvalue,	uucpWin.systemName_C,
				NULL
			);
		} else {
			if (origin == FromUser) {
				uucpWin.namePart_C = strdup(name);
				uucpWin.domainPart_C = strdup(hi.net.dns.resolv->domain);
			} else { /* FromList */
				int	i;

				cur_pos = hi.net.dns.cur_pos;
				for (i = 0; i < hi.net.dns.cur_wid_pos; i++)
					cur_pos = cur_pos->next;
				uucpWin.namePart_C = strdup(name);
				uucpWin.domainPart_C = strdup(cur_pos->domain.name);
				XtVaSetValues(
					uucpWin.sysName,
					XmNvalue,	uucpWin.systemName_C,
					NULL
				);
			}
		}
	} else {
		uucpWin.domainPart_C = NULL;
		uucpWin.namePart_C = strdup(name);
		if (origin == FromList) {
			XtVaSetValues(
				uucpWin.sysName,
				XmNvalue,	uucpWin.systemName_C,
				NULL
			);
		}
	}
}

void
createInfoMsg(Widget w, InfoMsg_t * im)
{
	static Widget	msgDialog;
	char		buf[MEDIUMBUFSIZE];
	XmString	text;
	Arg		args[10];
	int		i;
	XmString	str;

	if (!msgDialog) {
		msgDialog = XmCreateMessageDialog(w, "infoMsg", NULL, 0);
		XtVaSetValues(
			XtParent(msgDialog),
			XmNtitle,
			mygettxt(TXT_appName),
			NULL
		);
		str = XmStringCreateLocalized((String)mygettxt(im->okTitle));
		XtVaSetValues(
			msgDialog,
			XmNokLabelString,	str,
			NULL
		);
		XmStringFree(str);
		str = XmStringCreateLocalized((String)mygettxt(im->cancelTitle));
		XtVaSetValues(
			msgDialog,
			XmNcancelLabelString,	str,
			NULL
		);
		XmStringFree(str);
		if (im->okFcn) {
			XtAddCallback(
				msgDialog,
				XmNokCallback,
				im->okFcn,
				NULL
			);
		} else {
			XtSetSensitive(
				XmMessageBoxGetChild(
					msgDialog,
					XmDIALOG_OK_BUTTON
				),
				False
			);
		}
		if (im->cancelFcn) {
			XtAddCallback(
				msgDialog,
				XmNcancelCallback,
				im->cancelFcn,
				NULL
			);
		} else {
			XtSetSensitive(
				XmMessageBoxGetChild(
					msgDialog,
					XmDIALOG_CANCEL_BUTTON
				),
				False
			);
		}
		if (im->isHelpAvailable) {
			XtSetSensitive(
				XmMessageBoxGetChild(
					msgDialog,
					XmDIALOG_HELP_BUTTON
				),
				True
			);
		} else {
			XtSetSensitive(
				XmMessageBoxGetChild(
					msgDialog,
					XmDIALOG_HELP_BUTTON
				),
				False
			);
		}
	}
	switch (im->msgType) {
		case INFO:
			XtVaSetValues(
				msgDialog,
				XmNdialogType,	XmDIALOG_INFORMATION,
				NULL
			);
			break;
		case ERROR:
			XtVaSetValues(
				msgDialog,
				XmNdialogType,	XmDIALOG_ERROR,
				NULL
			);
			break;
		case WARN:
			XtVaSetValues(
				msgDialog,
				XmNdialogType,	XmDIALOG_WARNING,
				NULL
			);
			break;
	}
	/*strcpy(buf, mygettxt(im->msg)); */
	strcpy(buf, im->msg);
	text = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
	XtVaSetValues(msgDialog,
		XmNmessageString,	text,
		NULL
	);
	XmStringFree(text);
	XtManageChild(msgDialog);
}

static void
yesCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	deleteUucpList(duplicateEntry);
	setStatus();
	writeUucpFile(False);
	readUucpFile(hi.inet.uucp);
}

static void
noCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	if (hi.net.common.isDnsConfigure) {
		clearForm();
	}
}
