#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:internet/add_new_system.c	1.22"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
extern int	populateAddr(Widget, Widget, inetAddr, char **, char *, char **);
extern void	clearAddr(inetAddr);
extern XmString	createString(char *);
extern void	freeString(XmString);
extern int	writeEtcFile();
extern int	parseAddr(ADDR, char *);
extern void	createMsg(Widget, msgType, char *, char *);
extern char *	mygettxt(char *);
extern Boolean	appendEtcFile(char *, char *, char *);
extern void	helpkeyCB(Widget, XtPointer, XtPointer);
extern Boolean	getAddrToDotStr(inetAddr, char *);
extern void	displayFooterMsgEH(Widget, MiniHelp_t *, XFocusChangeEvent *);
extern int	r_decor(Widget);

char	*ans_form[]={
	TXT_system_name,
	TXT_inet_addr,
	TXT_comment,
	TXT_aliases,
	"",
};

static void addCB();
static void resetCB();
static void cancelCB();
static void setupGui();
void deleteCB();
static char *title;
static void infoMsg(Widget, char *);
static void confirmMsg(Widget, char *);
static void copyOkCB();
static int	isInEtcList();

MiniHelp_t	ansHelp[] = {
	{ MiniHelp_new1,	NULL },
	{ MiniHelp_new2,	NULL },
	{ MiniHelp_new3,	NULL }
};

HelpText	ANSHelp		= { NULL, NULL, "110" };
HelpText	DELHelp		= { NULL, NULL, "130" };
HelpText	CTSLHelp	= { NULL, NULL, "40" };

static ActionAreaItems actions[] = {
		{ TXT_Add,	MNE_add, addCB,	NULL	},
		{ TXT_Reset,	MNE_Reset, resetCB, NULL	},	
		{ TXT_Cancel,	MNE_Cancel, cancelCB, NULL	},	
		{ TXT_Help,	MNE_Help, helpkeyCB, &ANSHelp }
	};

static newPropInfo	ans;

/*non-gui data structure */
typedef struct	_localNew {
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
} localNew;

localNew	ln;

localNew	ln;

/* This routine is used in Copy To Systems List. */
void
copySystemList()
{
	confirmMsg(hi.net.common.toplevel, mygettxt(INFO_copySystem));
}

static void
copyOkCB()
{
	char *			name;
	char *			addr;
	struct hostent *	hostptr;
	struct in_addr *	ptr;
	char			buf[MEDIUMBUFSIZE], fullname[MEDIUMBUFSIZE];
	dnsList *		cur_pos;
	char *			tmp;
	int			i;

	name = strdup(hi.net.dns.dnsSelection[hi.net.dns.cur_wid_pos]);
	cur_pos = hi.net.dns.cur_pos;
	for (i = 0; i < hi.net.dns.cur_wid_pos; i++)
		cur_pos = cur_pos->next;

	if (! strcmp(cur_pos->domain.name, hi.net.dns.resolv->domain)) {
		strcpy(fullname, name);
	} else {
		if (tmp = strchr(name, '.')) {
			strcpy(fullname, name);
		} else {
			sprintf(fullname, "%s.%s", name, cur_pos->domain.name);
		}
	}

	busyCursor(hi.net.dns.dnsRC, True);
	if ((hostptr = gethostbyname(fullname)) != NULL) {
		while((ptr = (struct in_addr *) *hostptr->h_addr_list ++)
			!= NULL) {
			addr = strdup(inet_ntoa(*ptr));
		}
	}
	else {
		sprintf(buf, mygettxt(ERR_cantGetAddr), fullname);
		createMsg(hi.net.common.toplevel, ERROR, buf, title);
		busyCursor(hi.net.dns.dnsRC, False);
		return;
	}
	appendEtcFile(fullname, addr, "#");
	busyCursor(hi.net.dns.dnsRC, False);
}

static void 
infoOkCB(Widget w, XtPointer client_data, XmAnyCallbackStruct *cbs)
{
	int	rowNum, fileIndex, startingPos;

	rowNum = ((hi.net.etc.etcHostIndex+(ETCCOLS-1))/ETCCOLS);
	fileIndex = rowNum-2;
	startingPos = (rowNum-1)*ETCCOLS+1;
	
	deleteEtcList(fileIndex);
	writeEtcFile();
	XmListDeleteItemsPos(hi.net.etc.etcList, ETCCOLS, startingPos);
}

static void 
infoCancelCB(Widget w, XtPointer client_data, XmAnyCallbackStruct *cbs)
{
	XtUnmanageChild(w);
}


/* infoMsg() - Inform user with messagge */
static void
infoMsg(Widget parent, char *msg)
{
	static Widget	info_shell, info_dialog;
	Widget		ok, cancel, help;
	char		buf[MEDIUMBUFSIZE];
	XmString	text;
	DmMnemonicInfoRec	mneInfo[MI_TOTAL];
	unsigned char *		mne;
	KeySym			mneks;
	XmString		mneString;
	int			mm;

	if (!info_dialog) {
		info_dialog = XmCreateInformationDialog(parent,
			"infoMsg", NULL, 0);
		XtVaSetValues(XtParent(info_dialog),
			XmNtitle, mygettxt(TXT_info), NULL);
		ok = XmMessageBoxGetChild(
			info_dialog,
			XmDIALOG_OK_BUTTON
		);
		XmSTR_N_MNE(
			TXT_OK,
			MNE_OK,
			mneInfo[0],
			DM_B_MNE_ACTIVATE_CB | DM_B_MNE_GET_FOCUS
		);
		XmStringFree(mneString);
		XtVaSetValues(ok, XmNmnemonic, mneks, NULL);
		cancel = XmMessageBoxGetChild(
			info_dialog,
			XmDIALOG_CANCEL_BUTTON
		);
		XmSTR_N_MNE(
			TXT_Cancel,
			MNE_Cancel,
			mneInfo[1],
			DM_B_MNE_DEFAULT_ACTION | DM_B_MNE_GET_FOCUS
		);
		XmStringFree(mneString);
		XtVaSetValues(cancel, XmNmnemonic, mneks, NULL);
		help = XmMessageBoxGetChild(
			info_dialog,
			XmDIALOG_HELP_BUTTON
		);
		XmSTR_N_MNE(
			TXT_Help,
			MNE_Help,
			mneInfo[2],
			DM_B_MNE_ACTIVATE_CB | DM_B_MNE_GET_FOCUS
		);
		XmStringFree(mneString);
		XtVaSetValues(help, XmNmnemonic, mneks, NULL);
		XtAddCallback(info_dialog, XmNokCallback,
			(void(*)())infoOkCB, NULL);
		XtAddCallback(info_dialog, XmNcancelCallback,
			(void(*)())infoCancelCB, NULL);
		XtAddCallback(info_dialog, XmNhelpCallback,
			(void(*)())helpkeyCB, &DELHelp);
		mneInfo[0].w = ok;
		mneInfo[0].cb = (XtCallbackProc)infoOkCB;
		mneInfo[1].w = cancel;
		mneInfo[2].w = help;
		mneInfo[2].cb = (XtCallbackProc)helpkeyCB;
		mneInfo[2].cd = (XtPointer)&DELHelp;

		REG_MNE(info_dialog, mneInfo, 3);
	}
	strcpy(buf, mygettxt(msg));
	text = XmStringCreateLtoR(buf, "charset");
	XtVaSetValues(info_dialog,
		XmNmessageString, text,
		NULL);
	XmStringFree(text);
	XtManageChild(info_dialog);
}

/* confirmMsg() - confirm user with messagge */
static void
confirmMsg(Widget parent, char *msg)
{
	static Widget	confirm_shell, confirm_dialog;
	Widget		ok, cancel, help;
	char		buf[MEDIUMBUFSIZE];
	XmString	text;
	DmMnemonicInfoRec	mneInfo[MI_TOTAL];
	unsigned char *		mne;
	KeySym			mneks;
	XmString		mneString;
	int			mm;

	if (!confirm_dialog) {
		confirm_dialog = XmCreateInformationDialog(parent,
			"infoMsg", NULL, 0);
		XtVaSetValues(XtParent(confirm_dialog),
			XmNtitle, mygettxt(TXT_info), NULL);
		ok = XmMessageBoxGetChild(
			confirm_dialog,
			XmDIALOG_OK_BUTTON
		);
		XmSTR_N_MNE(
			TXT_OK,
			MNE_OK,
			mneInfo[0],
			DM_B_MNE_ACTIVATE_CB | DM_B_MNE_GET_FOCUS
		);
		XmStringFree(mneString);
		XtVaSetValues(ok, XmNmnemonic, mneks, NULL);
			
		cancel = XmMessageBoxGetChild(
			confirm_dialog,
			XmDIALOG_CANCEL_BUTTON
		);
		XmSTR_N_MNE(
			TXT_Cancel,
			MNE_Cancel,
			mneInfo[1],
			DM_B_MNE_DEFAULT_ACTION | DM_B_MNE_GET_FOCUS
		);
		XmStringFree(mneString);
		XtVaSetValues(cancel, XmNmnemonic, mneks, NULL);
			
		help = XmMessageBoxGetChild(
			confirm_dialog,
			XmDIALOG_HELP_BUTTON
		);
		XmSTR_N_MNE(
			TXT_Help,
			MNE_Help,
			mneInfo[2],
			DM_B_MNE_ACTIVATE_CB | DM_B_MNE_GET_FOCUS
		);
		XmStringFree(mneString);
		XtVaSetValues(help, XmNmnemonic, mneks, NULL);
		XtAddCallback(confirm_dialog, XmNokCallback,
			(void(*)())copyOkCB, NULL);
		XtAddCallback(confirm_dialog, XmNcancelCallback,
			(void(*)())infoCancelCB, (XtPointer)confirm_dialog);
		XtAddCallback(confirm_dialog, XmNhelpCallback,
			(void(*)())helpkeyCB, &CTSLHelp);
		mneInfo[0].w = ok;
		mneInfo[0].cb = (XtCallbackProc)copyOkCB;
		mneInfo[1].w = cancel;
		mneInfo[2].w = help;
		mneInfo[2].cb = (XtCallbackProc)helpkeyCB;
		mneInfo[2].cd = (XtPointer)&CTSLHelp;

		REG_MNE(confirm_dialog, mneInfo, 3);
	}
	/* strcpy(buf, mygettxt(msg)); */
	text = XmStringCreateLtoR(msg, "charset");
	XtVaSetValues(confirm_dialog,
		XmNmessageString, text,
		NULL);
	XmStringFree(text);
	XtManageChild(confirm_dialog);
}

int
deleteCmtList(int index)
{
	if (hi.net.etc.etcHosts->list[index].line->prev != NULL) {
		hi.net.etc.etcHosts->list[index].line->prev->next =
			hi.net.etc.etcHosts->list[index].line->next;
		if (hi.net.etc.etcHosts->list[index].line->next != NULL) {
			hi.net.etc.etcHosts->list[index].line->next->prev =
				hi.net.etc.etcHosts->list[index].line->prev;
		}
		else {
			/* delete the last one - just free it */;
		}
	}
	else {
		if (hi.net.etc.etcHosts->list[index].line->next != NULL) {
			/* delete the 1st one */
			hi.net.etc.etcHosts->list[index].line->next->prev = NULL;
			/* set the comentList points to the next one */
			hi.net.etc.etcHosts->commentList = 
				hi.net.etc.etcHosts->list[index].line->next;
		}
		else {
			/* delete the only one  - just free it*/;	
			hi.net.etc.etcHosts->commentList = NULL;	
		}
	}

	free(hi.net.etc.etcHosts->list[index].line);
}

int
deleteEtcList(int index)
{
	deleteCmtList(index);
	(void)memmove((void *)&(hi.net.etc.etcHosts->list[index]),
		&(hi.net.etc.etcHosts->list[index+1]),
		sizeof(etcHostEntry)*(hi.net.etc.etcHosts->count - index));
	hi.net.etc.etcHosts->count--;
	/* should free the last entry */
}

void
deleteCB()
{
	/* popup a warning message before delete */
	infoMsg(hi.net.common.toplevel, TXT_deleteInfo); 
}

static void
getaddrCB()
{
	char	buf[MEDIUMBUFSIZE];
	char *	haddr = NULL;

	if (ln.sysName_C)
		free(ln.sysName_C);
	XtVaGetValues(ans.sysName, XmNvalue, &ln.sysName_C, NULL);
	if (strlen(ln.sysName_C) == 0) {
		createMsg(hi.net.common.toplevel, INFO, mygettxt(INFO_systemNeed), title);
		return;
	}
	if (strcmp(ln.sysName_C, ln.sysName_P) != 0) {
		if (populateAddr(ans.popup, ans.getaddr, ans.addr,
			&(ln.addr_C), ln.sysName_C, &haddr) == False) {
			sprintf(buf, mygettxt(ERR_cantGetAddr), ln.sysName_C);
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
	String	name;
	XtVaGetValues(ans.sysName, XmNvalue, &name, NULL);
	if (strlen(name) > 0) {
		XtVaSetValues(ans.getaddr, XmNsensitive, True, NULL);
		clearAddr(ans.addr);
		if (ln.sysName_C) free(ln.sysName_C);
		ln.sysName_C = strdup(name);
	}
	else {
		XtVaSetValues(ans.getaddr, XmNsensitive, False, NULL);
	}
	clearAddr(ans.addr);
	XtVaSetValues(ans.comment, XmNvalue, "", NULL);
}

insertCmtList(int index, char *buf)
{
	etcLine *new;

	new = (etcLine *)calloc(1, sizeof(etcLine));
	new->line = strdup(buf);

	if (hi.net.etc.etcHosts->list[index].line->prev != NULL) {
		hi.net.etc.etcHosts->list[index].line->prev->next = new;
		new->prev = hi.net.etc.etcHosts->list[index].line->prev;
		hi.net.etc.etcHosts->list[index].line->prev = new;
		new->next = hi.net.etc.etcHosts->list[index].line;	
	}
	else {
		/* insert at the 1st position and move the comentList pointer
		 * points to it
		 */
		new->prev = NULL;
		new->next = hi.net.etc.etcHosts->list[index].line;
		hi.net.etc.etcHosts->list[index].line->prev = new;
		hi.net.etc.etcHosts->commentList = new;
	}
}

int
insertEtcList(char * name, char * addr, char *cmt) 
{ 
	char	buf[MEDIUMBUFSIZE];
	int	arrayIndex;

	arrayIndex = ((hi.net.etc.etcHostIndex+(ETCCOLS-1))/ETCCOLS)-2;
	sprintf(buf, "%-14s  %-15s  #%-25s\n",
		addr, name, cmt);
	insertCmtList(arrayIndex, buf);
	if (hi.net.etc.etcHosts->count % HOST_ALLOC_SIZE == 0) {
		hi.net.etc.etcHosts->list = (etcHostEntry *)
			realloc((etcHostEntry *)hi.net.etc.etcHosts->list,
			(hi.net.etc.etcHosts->count + HOST_ALLOC_SIZE) * 
			sizeof(etcHostEntry));
			if (hi.net.etc.etcHosts->list == NULL) {
				createMsg(hi.net.common.toplevel, ERROR, 
					mygettxt(ERR_memory), title);
				return Failure;
			}
	}
	(void)memmove((void *)&(hi.net.etc.etcHosts->list[arrayIndex+1]), 
		&(hi.net.etc.etcHosts->list[arrayIndex]),
		sizeof(etcHostEntry)*(hi.net.etc.etcHosts->count - arrayIndex));
	hi.net.etc.etcHosts->list[arrayIndex].etcHost.name =
		strdup(name);
	parseAddr(hi.net.etc.etcHosts->list[arrayIndex].etcHost.addr, addr);
	hi.net.etc.etcHosts->list[arrayIndex].comment =
		strdup(cmt);
	hi.net.etc.etcHosts->list[arrayIndex].aliases = NULL;
	hi.net.etc.etcHosts->list[arrayIndex].line =  
		hi.net.etc.etcHosts->list[arrayIndex+1].line[0].prev;
	hi.net.etc.etcHosts->count++;
}

static void
addCB() 
{
	char	*addr1, *addr2, *addr3, *addr4, c_addr[MEDIUMBUFSIZE];
	u_long	theaddr;
	unsigned char	*p;
	char	*name, *cmt;
	char	buf[MEDIUMBUFSIZE];
	int	i, rowNum, index, startingPos, pos[3];
	etcHostList	*list;
	XmString	*string;

	rowNum = ((hi.net.etc.etcHostIndex+(ETCCOLS-1))/ETCCOLS);
	index = rowNum-2;
	startingPos = (rowNum-1)*ETCCOLS+1;

	XtVaGetValues(ans.sysName, XmNvalue, &name, NULL);
	if (strlen(name) == 0) {
		createMsg(hi.net.common.toplevel, INFO, 
			mygettxt(INFO_systemNeed), title);	
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
		createMsg(hi.net.common.toplevel, INFO, 
			mygettxt(INFO_addrNeed), title);	
		return;
	}
	sprintf(c_addr, "%s.%s.%s.%s", addr1, addr2, addr3, addr4);
*/
	if (getAddrToDotStr(ans.addr, c_addr) == False) {
		createMsg(hi.net.common.toplevel, INFO, 
			mygettxt(INFO_addrNeed2), title);	
		return;
	}
	theaddr = inet_addr(c_addr);
	p = (unsigned char *)&theaddr;
	XtVaGetValues(ans.comment, XmNvalue, &cmt, NULL);
	if (isInEtcList(hi.net.etc.etcHosts, name, c_addr) != -1) {
		createMsg(hi.net.common.toplevel, INFO, 
			mygettxt(INFO_inEtcFile), title);
		return;
	}
	/* add one more entry in etcHostList data structure */
	insertEtcList(name, c_addr, cmt);
	/* update the etcList Gui  */
	string = (XmString *)XtMalloc(sizeof(XmString) * ETCCOLS);	
	string[0] = NULL;
	sprintf(buf, "%s", c_addr);
	string[1] = createString(buf);
	sprintf(buf, "%s", cmt);
	string[2] = createString(buf);
	XmListAddItems(hi.net.etc.etcList, string, ETCCOLS, startingPos);
	for (i=0; i< ETCCOLS; i++) {
		if (string[i])
			freeString(string[i]);
	}
	writeEtcFile();

	/* update the _C and _P  values */
	if (ln.sysName_C) free(ln.sysName_C);
	ln.sysName_C = strdup(name);
	if (ln.sysName_P) free(ln.sysName_P);
	ln.sysName_P = strdup(ln.sysName_C);

	if (ln.addr_C) free(ln.addr_C);
	ln.addr_C = strdup(c_addr);
	if (ln.addr_P) free(ln.addr_P);
	ln.addr_P = strdup(ln.addr_C);
	
	if (ln.comment_C) free(ln.comment_C);
	ln.comment_C = strdup(cmt);
	if (ln.comment_P) free(ln.comment_P);
	ln.comment_P = strdup(ln.comment_C);

	XtVaSetValues(ans.sysName, XmNvalue, "", NULL);
	clearAddr(ans.addr);
	XtVaSetValues(ans.comment, XmNvalue, "", NULL);
	
	XtUnmanageChild(ans.popup);
}
static void
resetCB() {
	char	tmpaddr[MEDIUMBUFSIZE];
	u_long	theaddr;
	unsigned char	*p;
	
	if (ln.sysName_C) free(ln.sysName_C);
	ln.sysName_C = strdup(ln.sysName_P);	
	if (ln.addr_C) free(ln.addr_C);
	ln.addr_C = strdup(ln.addr_P);
	if (ln.comment_C) free(ln.comment_C);
	ln.comment_C = strdup(ln.comment_P);

	/* update the gui */
	XtVaSetValues(ans.sysName, XmNvalue, ln.sysName_C, NULL);

	theaddr = inet_addr(ln.addr_C);
	p = (unsigned char *)&theaddr;
	sprintf(tmpaddr, "%d", p[ADDR1]);
	XtVaSetValues(ans.addr.addr[ADDR1], XmNvalue, tmpaddr, NULL);
	sprintf(tmpaddr, "%d", p[ADDR2]);
	XtVaSetValues(ans.addr.addr[ADDR2], XmNvalue, tmpaddr, NULL);
	sprintf(tmpaddr, "%d", p[ADDR3]);
	XtVaSetValues(ans.addr.addr[ADDR3], XmNvalue, tmpaddr, NULL);
	sprintf(tmpaddr, "%d", p[ADDR4]);
	XtVaSetValues(ans.addr.addr[ADDR4], XmNvalue, tmpaddr, NULL);

	XtVaSetValues(ans.comment, XmNvalue, ln.comment_C, NULL);
}

static void
cancelCB() {
	XtUnmanageChild(ans.popup);
}

static void
decideScreenLayout(Widget add_form)
{
	if (hi.net.common.cur_view == etcHost) {
		XtManageChild(add_form);
		XtManageChild(XtParent(ans.comment));
		ln.sysName_C = strdup("");	
		ln.sysName_P = strdup(ln.sysName_C);
		ln.comment_C = strdup("");
		ln.comment_P = strdup(ln.comment_C);
		ln.addr_C = strdup("");
		ln.addr_P = strdup(ln.addr_C);
		if (hi.net.common.isDnsConfigure == False) 
			XtVaSetValues(ans.getaddr, XmNsensitive, False, NULL);
		else	
			XtVaSetValues(ans.getaddr, XmNsensitive, True, NULL);
	}
}

static void
setupGui(w, client_data, cbs)
Widget			w;
int			client_data;
XmAnyCallbackStruct	*cbs;
{
	static Widget	ans_rc, add_form, topForm; 
	static Widget	sys_rc, sep1, sep2;
	int		maxWidth, len, arrayIndex;
	XmString	string;
	int		i;
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
		XtVaSetValues(ans.popup,
			XmNtitle, mygettxt(TXT_newView),
			XmNallowShellResize, True,
			XmNdeleteResponse, XmUNMAP,
			NULL);
		
		title = mygettxt(TXT_newView);
		r_decor(ans.popup);

		topForm = XtVaCreateWidget("ans_topForm",
			xmFormWidgetClass, ans.popup,
			NULL);
		
		ans_rc = XtVaCreateManagedWidget("ans_rowColumn",
			xmRowColumnWidgetClass, topForm,
			XmNorientation, XmVERTICAL,
			NULL);

		XtAddCallback(ans_rc, XmNhelpCallback, helpkeyCB, &ANSHelp);

		sys_rc = XtVaCreateManagedWidget("sys_rc",
			xmRowColumnWidgetClass, ans_rc,
			XmNorientation, XmHORIZONTAL, NULL);

		ans.sysName = crt_inputlabel(sys_rc, TXT_system_name, maxWidth);
		XtAddCallback(ans.sysName, XmNactivateCallback,
			(void(*)())nameChangedCB, NULL);
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
		mneInfo[0].w = ans.getaddr;
		mneInfo[0].cb = (XtCallbackProc)getaddrCB;
		XtAddCallback(ans.getaddr, XmNactivateCallback,
			(void(*)())getaddrCB, NULL);
	
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
			XmNcancelButton, actions[2].widget,
			NULL);

		/* add mini-help */
		for (i = 0; i < 3; i++) {
			ansHelp[i].widget = ans.status;
		}
		XtAddEventHandler(
			ans.sysName,
			FocusChangeMask,
			False,
			(void(*)())displayFooterMsgEH,
			(XtPointer)&ansHelp[0]
		);
		for (i = 0; i < 4; i++) {
			XtAddEventHandler(
				ans.addr.addr[i],
				FocusChangeMask,
				False,
				(void(*)())displayFooterMsgEH,
				(XtPointer)&ansHelp[1]
			);
		}
		XtAddEventHandler(
			ans.comment,
			FocusChangeMask,
			False,
			(void(*)())displayFooterMsgEH,
			(XtPointer)&ansHelp[2]
		);

		REG_MNE(ans.popup, mneInfo, XtNumber(actions) + 1);

		decideScreenLayout(add_form);	
	}	
	else {
		if (hi.net.common.cur_view == etcHost) {
			arrayIndex = ((hi.net.etc.etcHostIndex+(ETCCOLS-1))/ETCCOLS)-2;
			if (strcmp(ln.sysName_C, hi.net.etc.etcHosts->list[arrayIndex].etcHost.name) != 0) {
				decideScreenLayout(add_form);
			}
		}
		else {
			/* error, dns cannot has new */;
		}
	}
	XtManageChild(topForm);
	XtManageChild(ans.popup);
}

void
createAddNewSystem(w, client_data, cbs)
Widget			w;
int			client_data;
XmAnyCallbackStruct	*cbs;
{
	setupGui(w, client_data, cbs);
}

static int
isInEtcList(etcHostList	* list, char * name, char * c_addr)
{
	int	i;
	char	addr[MEDIUMBUFSIZE];
	
	for (i=0; i < list->count; i++) {
		if (strcmp(list->list[i].etcHost.name,
			name) == 0) {
			sprintf(addr, "%d.%d.%d.%d", 
				list->list[i].etcHost.addr[ADDR1],
				list->list[i].etcHost.addr[ADDR2],
				list->list[i].etcHost.addr[ADDR3],
				list->list[i].etcHost.addr[ADDR4]);
			if (strcmp(addr, c_addr) == 0) 
				return(i);
		}
	}
	return(-1);
}
