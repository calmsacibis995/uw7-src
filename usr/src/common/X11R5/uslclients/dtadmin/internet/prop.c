#ifndef NOIDENT
#ident	"@(#)dtadmin:internet/prop.c	1.16.1.1"
#endif

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <Xm/DialogS.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/CascadeBG.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/PanedW.h>
#include <Xm/ScrolledW.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/TextF.h>
#include <Xm/Separator.h>
#include <Xm/SeparatoG.h>
#include <Xm/MessageB.h>

#include <Dt/Desktop.h>

#include "DesktopP.h"

#include "lookup.h"
#include "lookupG.h"
#include "util.h"
#include "inet.h"
#include "inetMsg.h"

extern Widget	createActionArea(Widget, ActionAreaItems *, int, Widget, DmMnemonicInfo, int);
extern int	GetMaxWidth(Widget, char **);
extern Widget	crt_inputlabel(Widget, char *, int);
extern Widget	crt_inet_addr(Widget, char *, int, inetAddr *);
extern int	populateAddr(Widget, Widget, inetAddr, char**, char*, char**);
extern void	clearAddr(inetAddr);
extern XmString	createString(char *);
extern void	freeString(XmString);
extern void	createMsg(Widget, msgType, char *, char *);
extern void	helpkeyCB(Widget, XtPointer, XtPointer);
extern Boolean	removeWhitespace(char *);
extern Boolean	getAddrToDotStr(inetAddr, char *);
extern char	*mygettxt(char *);
extern int	r_decor(Widget);

static char	*ans_form[]={
	TXT_system_name,
	TXT_inet_addr,
	TXT_comment,
	TXT_aliases,
	"",
};

static char *title;
static void okCB();
static void applyCB();
static void resetCB();
static void cancelCB();

HelpText	PropHelp = { NULL, NULL, "240" };

static ActionAreaItems actions[] = {
		{ TXT_OK,	MNE_OK,		okCB,		NULL },
		{ TXT_Apply,	MNE_Apply,	applyCB,	NULL },
		{ TXT_Reset,	MNE_Reset,	resetCB, 	NULL },	
		{ TXT_Cancel,	MNE_Cancel,	cancelCB, 	NULL },	
		{ TXT_Help,	MNE_Help,	helpkeyCB,	&PropHelp }
};

newPropInfo	ans;

/* non-gui data structure */
typedef struct	_localProp {
	char	*sysName_C;
	char	*sysName_P;
	char	*addr_C;
	char	*addr_P;
	char	*comment_C;
	char	*comment_P;
	char	*statusLine_P;
	char	*statusLine_C;
	ADDR	dotAddr;
	char	*haddr;
} localProp;

localProp lp;

int
writeEtcFile()
{
	static FILE	*fd;
	int		i;
	char		buf[MEDIUMBUFSIZE];
	etcLine		*linePtr;
	struct stat	statBuffer;
	
	while (!(fd = fopen(etcHostPath, "w")) && errno == EINTR)
		/* try again */;

	if (!fd) {
		
		createMsg(hi.net.common.toplevel, ERROR, mygettxt(ERR_cantOpenEtcFile), title);
		return Failure;
	}

	while (fstat(fileno(fd), &statBuffer) < 0 && errno == EINTR)
		/* try again */;

	linePtr = hi.net.etc.etcHosts[0].commentList;
	
	while (linePtr) {
		if (linePtr->line)
			fputs(linePtr->line, fd);
		linePtr = linePtr->next;
	}
	fclose(fd);
}

static void
getaddrCB()
{
	int	index;
	char	buf[MEDIUMBUFSIZE];
	char *	haddr = NULL;

	index =  ((hi.net.etc.etcHostIndex + (ETCCOLS -1))/ETCCOLS) - 2;
	if (lp.sysName_C)
		free(lp.sysName_C);
	XtVaGetValues(ans.sysName, XmNvalue, &lp.sysName_C, NULL);
	if (strlen(lp.sysName_C) == 0) {
		createMsg(hi.net.common.toplevel, INFO, mygettxt(INFO_systemNeed), title);
		return;
	}
	if (strcmp(lp.sysName_C, lp.sysName_P) != 0) {
		if (populateAddr(ans.popup, ans.getaddr, ans.addr,
			&(lp.addr_C), lp.sysName_C, &haddr) == True) {
			if (hi.net.etc.etcHosts->list[index].comment)
				XtVaSetValues(ans.comment, XmNvalue,
					hi.net.etc.etcHosts->list[index].comment, NULL);
		}
		else {
			sprintf(buf, mygettxt(ERR_cantGetAddr), lp.sysName_C);
			createMsg(hi.net.common.toplevel, ERROR, buf, title);
		}
	}
	else {
		createMsg(hi.net.common.toplevel, INFO, mygettxt(INFO_sameNameNoQuery2), title);
	}
	if (haddr) free(haddr);
}

static void
nameChangedCB(Widget w, XtPointer client_data, XmAnyCallbackStruct *cb)
{
	String 	name;

	XtVaGetValues(ans.sysName, XmNvalue, &name, NULL);
	if (strlen(name) > 0) {
		XtVaSetValues(ans.getaddr, XmNsensitive, True, NULL);
		clearAddr(ans.addr);
		if (lp.sysName_C) free(lp.sysName_C);
		lp.sysName_C = strdup(name);
	}
	else {
		XtVaSetValues(ans.getaddr, XmNsensitive, False, NULL);
	}
	clearAddr(ans.addr);
	XtVaSetValues(ans.comment, XmNvalue, "", NULL);
}

static void
okCB() {
	applyCB();
	hi.net.common.isProp = False;
	XtUnmanageChild(ans.popup);
}

static void
applyCB() 
{
	char	*addr1, *addr2, *addr3, *addr4, c_addr[MEDIUMBUFSIZE];
	u_long	theaddr;
	unsigned char	*p;
	char	*name, *cmt;
	char	buf[MEDIUMBUFSIZE];
	int	rowNum, index, startingPos;
	etcHostList	*list;
	XmString	*string;
	int		pos[3];

	rowNum = ((hi.net.etc.etcHostIndex + (ETCCOLS -1))/ETCCOLS);
	index = rowNum-2;
	startingPos=(rowNum-1)*ETCCOLS+1;
	list = hi.net.etc.etcHosts;

	XtVaGetValues(ans.sysName, XmNvalue, &name, NULL);
	removeWhitespace(name);
	if (strlen(name) == 0) {
		createMsg(hi.net.common.toplevel, INFO, mygettxt(INFO_systemNeed), title);
		return;
	}
/*
	XtVaGetValues(ans.addr.addr[ADDR1], XmNvalue, &addr1, NULL);
	XtVaGetValues(ans.addr.addr[ADDR2], XmNvalue, &addr2, NULL);
	XtVaGetValues(ans.addr.addr[ADDR3], XmNvalue, &addr3, NULL);
	XtVaGetValues(ans.addr.addr[ADDR4], XmNvalue, &addr4, NULL);
	if ((strlen(addr1) == 0 || (strlen(addr1) == 1 && isspace(addr1[0]))) ||
		(strlen(addr2) == 0 || (strlen(addr2) == 1 && isspace(addr2[0]))) ||
		(strlen(addr3) == 0 || (strlen(addr3) == 1 && isspace(addr3[0]))) ||
		(strlen(addr4) == 0 || (strlen(addr4) == 1 && isspace(addr4[0])))) {
		createMsg(hi.net.common.toplevel, INFO, mygettxt(INFO_addrNeed),title);
		return;
	} 
	sprintf(c_addr, "%s.%s.%s.%s", addr1, addr2, addr3, addr4);
*/
	if (getAddrToDotStr(ans.addr, c_addr) == False) {
		createMsg(hi.net.common.toplevel, INFO, mygettxt(INFO_addrNeed2),title);
		return;
	}
	theaddr = inet_addr(c_addr);
	p = (unsigned char *)&theaddr;
	XtVaGetValues(ans.comment, XmNvalue, &cmt, NULL);
	/* update the etcHostList data structure */
	if (list->list[index].etcHost.name) 
		free(list->list[index].etcHost.name);
	list->list[index].etcHost.name = strdup(name);
	list->list[index].etcHost.addr[0] = p[0];
	list->list[index].etcHost.addr[1] = p[1];
	list->list[index].etcHost.addr[2] = p[2];
	list->list[index].etcHost.addr[3] = p[3];
	if (list->list[index].comment)
		free(list->list[index].comment);
	list->list[index].comment = strdup(cmt);

	/* update the line that will be put back to /etc/hosts */
	sprintf(buf, "%s\t%s\t%s\t# %s\n", 
		c_addr,
		list->list[index].etcHost.name,
		list->list[index].aliases? list->list[index].aliases[0] : 0, 
		list->list[index].comment);
	if (list->list[index].line->line)
		free(list->list[index].line->line);
	list->list[index].line->line = strdup(buf);
		
	/* update the _C and _P values*/
	if (lp.sysName_C) free(lp.sysName_C);
		lp.sysName_C = strdup(name);
	if (lp.sysName_P) free(lp.sysName_P);
		lp.sysName_P = strdup(lp.sysName_C);

	if (lp.addr_C) free(lp.addr_C);
		lp.addr_C = strdup(c_addr);

	if (lp.comment_C) free(lp.comment_C);
		lp.comment_C = strdup(cmt);
	if (lp.comment_P) free(lp.comment_P);
		lp.comment_P = strdup(lp.comment_C);

	/* update the etcList Gui */
	string = (XmStringTable)XtMalloc(ETCCOLS*sizeof(XmString *));
	string[0] = NULL;
	pos[0] = startingPos;
	sprintf(buf, "%s", c_addr);
	string[1] = XmStringCreate(buf, XmFONTLIST_DEFAULT_TAG);
	pos[1] = startingPos+1;
	sprintf(buf, "# %s", list->list[index].comment);
	string[2] = XmStringCreate(buf, XmFONTLIST_DEFAULT_TAG);
	pos[2] = startingPos+2;
	
	XmListReplacePositions(hi.net.etc.etcList, pos, string, ETCCOLS);

	/* write back to /etc/hosts file */
	writeEtcFile();
}

static void
resetCB() {
	char		tmpaddr[MEDIUMBUFSIZE];
	u_long		theaddr;
	unsigned char	*p;

	if (lp.sysName_C) free(lp.sysName_C);
		lp.sysName_C = strdup(lp.sysName_P);
	if (lp.addr_C) free(lp.addr_C);
		lp.addr_C = strdup(lp.addr_P);
	if (lp.comment_C) free(lp.comment_C);
		lp.comment_C = strdup(lp.comment_P);

	/* update the gui */
	XtVaSetValues(ans.sysName, XmNvalue, lp.sysName_C, NULL);

	theaddr = inet_addr(lp.addr_C);
	p = (unsigned char *)&theaddr;
	sprintf(tmpaddr, "%d", p[ADDR1]);
	XtVaSetValues(ans.addr.addr[ADDR1], XmNvalue, tmpaddr, NULL);
	sprintf(tmpaddr, "%d", p[ADDR2]);
	XtVaSetValues(ans.addr.addr[ADDR2], XmNvalue, tmpaddr, NULL);
	sprintf(tmpaddr, "%d", p[ADDR3]);
	XtVaSetValues(ans.addr.addr[ADDR3], XmNvalue, tmpaddr, NULL);
	sprintf(tmpaddr, "%d", p[ADDR4]);
	XtVaSetValues(ans.addr.addr[ADDR4], XmNvalue, tmpaddr, NULL);

	XtVaSetValues(ans.comment, XmNvalue, lp.comment_P, NULL);
}

static void
cancelCB() {
	hi.net.common.isProp = False;
	XtUnmanageChild(ans.popup);
}

static void
decideScreenLayout(Widget add_form)
{
	char		buf[MEDIUMBUFSIZE], fullname[MEDIUMBUFSIZE];
	XmString	str;
	int		index;
	char *		haddr = NULL;
	dnsList *	cur_pos;
	int		i;

	if (hi.net.common.cur_view == etcHost) {

		index = ((hi.net.etc.etcHostIndex + (ETCCOLS -1))/ETCCOLS) - 2;
		/* mapping/unmapping control for read only and read/write 
		 * screens 
		 */
		XtUnmanageChild(XtParent(ans.sysLabel));
		XtManageChild(XtParent(ans.sysName));
		XtUnmanageChild(XtParent(ans.naLabel));
		XtManageChild(add_form);
		XtManageChild(XtParent(ans.comment));
		XtVaSetValues(actions[0].widget, XmNsensitive, True, NULL);
		XtVaSetValues(actions[1].widget, XmNsensitive, True, NULL);
		XtVaSetValues(actions[2].widget, XmNsensitive, True, NULL);
		/* resizeActionArea(ans.sysName, actions, XtNumber(actions), NULL);  */
		lp.sysName_C = strdup(hi.net.etc.etcHosts->list[index].etcHost.name);
		lp.sysName_P = strdup(lp.sysName_C);
		XtVaSetValues(ans.sysName, XmNvalue, lp.sysName_C, NULL);
		if (hi.net.etc.etcHosts->list[index].comment) 
			XtVaSetValues(ans.comment, XmNvalue, 
				hi.net.etc.etcHosts->list[index].comment, NULL);
		lp.comment_C = strdup(hi.net.etc.etcHosts->list[index].comment);
		lp.comment_P = strdup(lp.comment_C);
	}
	else {
		XtUnmanageChild(XtParent(ans.sysName));
		XtUnmanageChild(add_form);
		XtUnmanageChild(XtParent(ans.comment));
		XtManageChild(XtParent(ans.sysLabel));
		XtManageChild(XtParent(ans.naLabel));
		XtVaSetValues(actions[0].widget, XmNsensitive, False, NULL);
		XtVaSetValues(actions[1].widget, XmNsensitive, False, NULL);
		XtVaSetValues(actions[2].widget, XmNsensitive, False, NULL);
		/* resizeActionArea(ans.naLabel, actions, XtNumber(actions), NULL);  */

		if (strchr(hi.net.dns.dnsSelection[hi.net.dns.cur_wid_pos], '.') 
			== NULL) {
			cur_pos = hi.net.dns.cur_pos;
			for (i = 0; i < hi.net.dns.cur_wid_pos; i++) {
				cur_pos = cur_pos->next;
			}
			sprintf(fullname, "%s.%s", 
				hi.net.dns.dnsSelection[hi.net.dns.cur_wid_pos],
				cur_pos->domain.name);
		}
		else {
			sprintf(fullname, "%s",
				hi.net.dns.dnsSelection[hi.net.dns.cur_wid_pos]);
		}

		lp.sysName_C = strdup(fullname);
		lp.sysName_P = strdup(lp.sysName_C);
		/* no i18n?? */
		str = createString(lp.sysName_C);
		XtVaSetValues(ans.sysLabel, XmNlabelString, str, NULL);	
		freeString(str);
		
	}

	/* populate the address */
	if (populateAddr(ans.popup, ans.getaddr, ans.addr, &lp.addr_C, lp.sysName_C, &haddr)) {
		if (lp.addr_P) free(lp.addr_P);
		lp.addr_P = strdup(lp.addr_C);
		if (hi.net.common.cur_view != etcHost) {
			str = createString(lp.addr_C);
			XtVaSetValues(ans.naLabel, XmNlabelString, str, NULL);
			freeString(str);
		}
	}
	else {
		/* error message for not finding the address */;
		if (hi.net.common.cur_view == etcHost) {
			clearAddr(ans.addr);
		}
		else {
			str = createString("");
			XtVaSetValues(ans.naLabel, XmNlabelString, str, NULL);
			freeString(str);
		}
			
		sprintf(buf, mygettxt(ERR_cantGetAddr), lp.sysName_C);
		createMsg(hi.net.common.toplevel, ERROR, buf, title);
	}	

	/* if DNS is not configured or in dns view, grey out Get 
	 * Address Button 
	 */
	if ((hi.net.common.isDnsConfigure == False) ||
		(hi.net.common.cur_view != etcHost)) {
		XtVaSetValues(ans.getaddr, XmNsensitive, False, NULL);
	}
	else {
		XtVaSetValues(ans.getaddr, XmNsensitive, True, NULL);	
	}
	if (haddr) free(haddr);
}

static void
setupGui(Widget w, int client_data, XmAnyCallbackStruct *cbs)
{
	static Widget	ans_rc, sys_rc, sep1, sep2, topForm;
	int		maxWidth, len, index;
	int		j;
	XmString	string;
	static Widget 	add_form, sysLabel, naLabel;
	DmMnemonicInfoRec	mneInfo[MI_TOTAL];
	unsigned char *		mne;
	char			tmpstr[BUFSIZ];
	KeySym			mneks;
	XmString		mneString;
	int			mm;

	if (!ans.popup) {
		maxWidth = GetMaxWidth(XtParent(w), ans_form);
		ans.popup = XtVaCreatePopupShell("add_new_system",
			xmDialogShellWidgetClass, XtParent(w),
			NULL);
		title = mygettxt(TXT_propView);
		XtVaSetValues(ans.popup,
			XmNtitle, title,
			XmNallowShellResize, True,
			XmNdeleteResponse, XmUNMAP,
			NULL);

		r_decor(ans.popup);

		topForm = XtVaCreateWidget("topForm",
			xmFormWidgetClass, ans.popup,
			NULL);
		
		ans_rc = XtVaCreateManagedWidget("ans_rowColumn",
			xmRowColumnWidgetClass, topForm,
			XmNorientation, XmVERTICAL,
			NULL);

		XtAddCallback(ans_rc, XmNhelpCallback, helpkeyCB, &PropHelp);

		sys_rc = XtVaCreateManagedWidget("sys_rc",
			xmRowColumnWidgetClass, ans_rc,
			XmNorientation, XmHORIZONTAL, NULL);

		/*XtAddCallback(ans.sysName, XmNlosingFocusCallback, */
		ans.sysName = crt_inputlabel(sys_rc, TXT_system_name, maxWidth);
		XtAddCallback(ans.sysName, XmNactivateCallback,
			(void(*)())nameChangedCB, NULL);
		ans.sysLabel = (Widget) crt_inputlabellabel(sys_rc, TXT_system_name, maxWidth,TXT_tmpSys);
		strcpy(tmpstr, mygettxt(TXT_getAddr));
		ans.getaddr = XtVaCreateManagedWidget(tmpstr,
			xmPushButtonWidgetClass, sys_rc, NULL);
		XmSTR_N_MNE(
			TXT_getAddr,
			MNE_getAddr,
			mneInfo[0],
			DM_B_MNE_ACTIVATE_CB | DM_B_MNE_GET_FOCUS
		);
		XmStringFree(mneString);
		XtVaSetValues(ans.getaddr, XmNmnemonic, mneks, NULL);
		XtAddCallback(ans.getaddr, XmNactivateCallback,
			(void(*)())getaddrCB, NULL);
		mneInfo[0].w = ans.getaddr;
		mneInfo[0].cb = (XtCallbackProc)getaddrCB;

		ans.naLabel = (Widget) crt_inputlabellabel(ans_rc, TXT_inet_addr, maxWidth,NULL);
		setLabel(ans.naLabel, "");
	
		add_form = crt_inet_addr(ans_rc, TXT_inet_addr, maxWidth, &(ans.addr));
		ans.comment = crt_inputlabel(ans_rc, TXT_comment, maxWidth);

		sep1 = XtVaCreateManagedWidget("spe1",
			xmSeparatorWidgetClass, ans_rc, NULL);
		createActionArea(
			ans_rc,
			actions, 
			XtNumber(actions),
			ans.sysName,
			mneInfo,
			1
		);
		sep2 = XtVaCreateManagedWidget("spe2",
			xmSeparatorWidgetClass, ans_rc, NULL);
		ans.status = XtVaCreateManagedWidget("ans_status",
			xmLabelWidgetClass, ans_rc,
			NULL);

		setLabel(ans.status, "");

		XtVaSetValues(topForm,
			XmNdefaultButton, actions[0].widget,
			XmNcancelButton, actions[3].widget,
			NULL);

		REG_MNE(ans.popup, mneInfo, XtNumber(actions) + 1);
		
		decideScreenLayout(add_form);
		
	}	
	else {
		if (hi.net.common.cur_view == etcHost) {
			index = ((hi.net.etc.etcHostIndex + (ETCCOLS-1))/ETCCOLS)+2;
			if (strcmp(lp.sysName_C, hi.net.etc.etcHosts->list[index].etcHost.name) != 0) {
				decideScreenLayout(add_form);
			}
		}
		else {
			if (strcmp(lp.sysName_C, hi.net.dns.dnsSelection[hi.net.dns.cur_wid_pos]) != 0) {
				decideScreenLayout(add_form);
			}
		}
	}

	XtManageChild(topForm);
	XtManageChild(ans.popup);
}

void
createProperty(Widget w, int client_data, XmAnyCallbackStruct *cbs)
{
	setupGui(w, client_data, cbs);
	hi.net.common.isProp = True;
}
