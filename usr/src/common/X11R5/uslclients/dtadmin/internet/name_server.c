#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:internet/name_server.c	1.34.1.1"
#endif


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <Xm/DialogS.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
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
#include <Xm/List.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>


#include <Dt/Desktop.h>

#include "DesktopP.h"

#include "lookup.h"
#include "lookupG.h"
#include "util.h"
#include "inetMsg.h"
#include "inet.h"

#include "nis_server.h"

#define UC(b)	(((int)b)&0xff)
#define NUMCOLS	2

extern Widget	createActionArea(Widget, ActionAreaItems *, int, Widget, DmMnemonicInfo, int);
extern int	GetMaxWidth(Widget, char **);
extern Widget	crt_inputlabel(Widget, char *, int);
extern int	crt_inet_addr(Widget, char *, int, inetAddr *);
extern void	setAddrGui(inetAddr, unsigned char *);
extern XmString	createString(char *);
extern XmString	i18nString(char *);
extern void	freeString(XmString);
extern void	clearAddr(inetAddr);
extern int	allocList(hostList *);
extern int	readResolvConf(resolvConf *);
extern int	freeResolvConf(resolvConf *);
extern void	busyCursor(Widget, int);
extern void	createMsg(Widget, msgType, char *, char *);
extern void	helpkeyCB(Widget, XtPointer, XtPointer);
extern void	remoteWhitespace(char *);
extern char *	mygettxt(char *);
extern void	displayFooterMsgEH(Widget, MiniHelp_t *, XFocusChangeEvent
*);
extern int	r_decor(Widget);

char	*nsa_form[]={
	TXT_LDN,
	TXT_name_server,
	TXT_inet_addr,
	" ",
	NULL
};

char *	nsa_buttons[] = {
	TXT_add,
	TXT_mod,
	TXT_delete,
	NULL
};

char *	nsa_button_mnemonics[] = {
	MNE_add,
	MNE_modify,
	MNE_delete,
	NULL
};

static char *title;
static void okCB();
static void resetCB();
static void cancelCB();
static void addCB();
static void delCB();
static void modCB();
static void singleClickCB();
static void domainNameChangedCB();
static void addrChangedCB();
static void infoOkCB();
static void infoCancelCB();
static void infoMsg(Widget, char *);
static void cleanup(char *);
static void verifyNameServer();
static void pre_dnsQuery();
static void dnsQuery();
static void writeResolvFile();
static void cleanupDns();

MiniHelp_t	nsHelp[] = {
	{ MiniHelp_dns1,	NULL },
	{ MiniHelp_dns2,	NULL },
	{ MiniHelp_dns3,	NULL }
};

HelpText	NSHelp = { NULL, NULL, "170" };

static ActionAreaItems actions[] = {
		{ TXT_OK,	MNE_OK,		okCB,		NULL },
		{ TXT_Reset,	MNE_Reset,	resetCB,	NULL },	
		{ TXT_Cancel,	MNE_Cancel,	cancelCB,	NULL },	
		{ TXT_Help,	MNE_Help,	helpkeyCB,	&NSHelp }
};

typedef enum {
	DOMAINNAME,
	DNSSERVER,
	NETADDR,
	ADDFIRST,
	VERIFYSERVER
} _msgType;	

typedef enum {
	DOMAINCHANGED,
	OKPRESSED,
	NSNONE
} nsqueryType;

typedef struct _nameServer {
	Widget		popup;
	Widget		domainName;
	Widget		nameServer;
	inetAddr	addr;
	ActionAreaItems	*actions;
	Widget		status;
	Widget		swin;
	Widget		list;
	Widget		button[3];
	/* non-gui data structure */
	resolvConf	*resolv_C;	/* current values on the screen */
	char		*domainName_C;
	char		*domainName_P;
	char		*server_C;
	char		*server_P;
	char		*addr_C;
	char		*addr_P;
	int		curSelectedPos;
	int		preSelectedPos;
	_msgType	msgType;
	nsqueryType	queryFrom;
	char		*addrStr; 	/* BAD: cannot pass this info thru the client_data */
	char		*nameStr;	
	/* query data */
	char		*outfile;
	int		pid;
	resolvConf	server; 	/* info from the nslookup query */
} nameServer;

nameServer	ns;

static void
infoOkCB(Widget w, XtPointer client_data, XmAnyCallbackStruct *cbs)
{
	XmString	string;
	char		*tmpname;
	String		name;
	char		fullname[MEDIUMBUFSIZE];

	switch(ns.msgType) {
	case ADDFIRST:
		/* add the new entry to the 1st slot - primary server */

		(void)memmove((void *)&(ns.resolv_C->serverList.list[1]),
			&(ns.resolv_C->serverList.list[0]),
			sizeof(hostEntry)*(ns.resolv_C->serverList.count));
		ns.resolv_C->serverList.list[0].name = strdup(ns.nameStr);
		getDotStrToADDR(ns.addrStr, ns.resolv_C->serverList.list[0].addr);
		ns.resolv_C->serverList.count++;

		string = createString(ns.addrStr);
		XmListAddItemUnselected(ns.list, string, 3);
		freeString(string);
		string = createString(ns.nameStr);
		XmListAddItemUnselected(ns.list, string, 4);
		freeString(string);
		clearGui();

		break;
	case DOMAINNAME:
		/* append .com to the end of domainName */
		XtVaGetValues(ns.domainName, XmNvalue, &name, NULL);
		strcpy(fullname, name);
		strcat(fullname, ".com");
		XtVaSetValues(ns.domainName, XmNvalue, fullname, NULL);
		/* repeat the query */
		busyCursor(ns.domainName, TRUE);
		signal(SIGCHLD, (void(*)())pre_dnsQuery);
		ns.queryFrom = DOMAINCHANGED;
		askForNameServers(name);
		break;
	default:
		break;
	}
}

static void
infoCancelCB(Widget w, XtPointer client_data, XmAnyCallbackStruct *cbs)
{
	XmString	string;

	switch(ns.msgType) {
	case ADDFIRST:
		/* append the new entry after the 1st one */

		(void)memmove((void *)&(ns.resolv_C->serverList.list[2]),
			&(ns.resolv_C->serverList.list[1]),
			sizeof(hostEntry)*(ns.resolv_C->serverList.count -1));
		ns.resolv_C->serverList.list[1].name = strdup(ns.nameStr);
		getDotStrToADDR(ns.addrStr, ns.resolv_C->serverList.list[1].addr);
		ns.resolv_C->serverList.count++;
		
		string = createString(ns.addrStr);
		XmListAddItemUnselected(ns.list, string, 5);
		freeString(string);
		string = createString(ns.nameStr);
		XmListAddItemUnselected(ns.list, string, 6);
		freeString(string);
		clearGui();

		break;
	case VERIFYSERVER:
		/* just popdown the message window and do nothing */
		break;
	default:
		break;
	}
}

Boolean
copyResolvConf(resolvConf *org, resolvConf **cpy)
{
	int	count, i;

	if (org->serverList.count < HOST_ALLOC_SIZE)
		count = HOST_ALLOC_SIZE;
	else {
		i = count/HOST_ALLOC_SIZE;
		count = (i+1)*HOST_ALLOC_SIZE;
	}

	*cpy = (resolvConf*) malloc(sizeof(resolvConf));
	(*cpy)->domain = strdup(org->domain);

	(*cpy)->serverList.list = (hostEntry *)malloc(sizeof(hostEntry)*count);
	if ((*cpy)->serverList.list == NULL)
		return False;
	else {
		memset((*cpy)->serverList.list, '\0', sizeof(hostEntry)*count);
		for (i=0; i< org->serverList.count; i++) {
			(*cpy)->serverList.list[i].name = strdup(org->serverList.list[i].name);
			(*cpy)->serverList.list[i].addr[ADDR1] = org->serverList.list[i].addr[ADDR1];
			(*cpy)->serverList.list[i].addr[ADDR2] = org->serverList.list[i].addr[ADDR2];
			(*cpy)->serverList.list[i].addr[ADDR3] = org->serverList.list[i].addr[ADDR3];
			(*cpy)->serverList.list[i].addr[ADDR4] = org->serverList.list[i].addr[ADDR4];
		}
		(*cpy)->serverList.count = org->serverList.count;
	}
	return True;
}

clearGui()
{
	/*
	XtVaSetValues(ns.nameServer, XmNvalue, "", NULL);
	*/
	XmTextSetString(ns.nameServer, "");
	clearAddr(ns.addr);
	XtVaSetValues(ns.button[ADD], XmNsensitive, False, NULL);
	XtVaSetValues(ns.button[MODIFY], XmNsensitive, False, NULL);
	XtVaSetValues(ns.button[DELETE], XmNsensitive, False, NULL);
}
Boolean
getAddrToDotStr(inetAddr addr, char * dotStr)
{
	char	*addrval[4] = { NULL, NULL, NULL, NULL};
	long	val, strtol();
	char	*sptr;
	int	i;
	Boolean	errorFlags=True;

	for (i = 0 ; i < 4; i++) {
		XtVaGetValues(addr.addr[i], XmNvalue, &addrval[i], NULL);
		removeWhitespace(addrval[i]);
		val = strtol(addrval[i], (char **)&sptr, 10);
		if (sptr == addrval[i] || *sptr != 0 || val < 0 || val > 255) {
			errorFlags = False;
			break;
		}
	}
	if (
		(! strcmp(addrval[0], "0")) &&
		(! strcmp(addrval[1], "0")) &&
		(! strcmp(addrval[2], "0")) &&
		(! strcmp(addrval[3], "0"))
	)
		return False;
	if (
		(! strcmp(addrval[0], "255")) &&
		(! strcmp(addrval[1], "255")) &&
		(! strcmp(addrval[2], "255")) &&
		(! strcmp(addrval[3], "255"))
	)
		return False;

	if (errorFlags == False) {
		return(False);
	}
	else {	
		sprintf(dotStr, "%s.%s.%s.%s", 
			addrval[0], addrval[1], addrval[2], addrval[3]);
		return(True);
	}
}

getDotStrToADDR(char * buf, ADDR addr)
{
	u_long		theaddr;
	unsigned char	*p;
	
	theaddr = inet_addr(buf);
	p = (unsigned char *)&theaddr;
	addr[ADDR1] = UC(p[0]);
	addr[ADDR2] = UC(p[1]);
	addr[ADDR3] = UC(p[2]);
	addr[ADDR4] = UC(p[3]);
}

static void
infoMsg(Widget parent, char *msg)
{
	static Widget	info_shell, info_dialog;
	char		buf[MEDIUMBUFSIZE];
	XmString	text;

	if (!info_dialog) {
		info_dialog = XmCreateInformationDialog(parent,
			"infoMsg", NULL, 0);
		XtVaSetValues(XtParent(info_dialog),
			XmNtitle, mygettxt(TXT_info), NULL);
		text = XmStringCreateLocalized(mygettxt(TXT_Yes));
		XtVaSetValues(info_dialog,
			XmNokLabelString, text, NULL);
		XmStringFree(text);
		text = XmStringCreateLocalized(mygettxt(TXT_No));
		XtVaSetValues(info_dialog,
			XmNcancelLabelString, text, NULL);
		XmStringFree(text);
		XtAddCallback(info_dialog, XmNokCallback,
			(void(*)())infoOkCB, NULL);
		XtAddCallback(info_dialog, XmNcancelCallback,
			(void(*)())infoCancelCB, NULL);
		XtAddCallback(info_dialog, XmNhelpCallback,
			(void(*)())helpkeyCB, &NSHelp);
	}
	strcpy(buf, msg);
	text = XmStringCreateLtoR(buf, "charset");
	XtVaSetValues(info_dialog,
		XmNmessageString, text,
		NULL);
	XmStringFree(text);
	XtManageChild(info_dialog);
}

static void
addCB(Widget w, XtPointer client_data, XmAnyCallbackStruct *cbs) 
{
	XmString	astr, nstr;
	char		buf[MEDIUMBUFSIZE];
	String		server;
	int		row_num, count;

	XtVaGetValues(ns.nameServer, XmNvalue, &server, NULL);	
	if (getAddrToDotStr(ns.addr, buf) == False) {
		createMsg(hi.net.common.toplevel, INFO, mygettxt(INFO_addrNeed2), title);
		return;
	}

	astr = createString(buf);		
	nstr = createString(server);

	if (ns.resolv_C == 0) {
		/* DNS is never configured, so update the list and internal
		 * data structure.
		 */
		ns.resolv_C = (resolvConf *)malloc(sizeof(resolvConf));
		ns.resolv_C->serverList.list = 
			(hostEntry *)malloc(sizeof(hostEntry)*HOST_ALLOC_SIZE);
		memset(ns.resolv_C->serverList.list, '\0', sizeof(hostEntry)*HOST_ALLOC_SIZE);
		ns.resolv_C->serverList.list[0].name = strdup(server);
		getDotStrToADDR(buf, ns.resolv_C->serverList.list[0].addr);
		ns.resolv_C->serverList.count=1;
		XmListAddItemUnselected(ns.list, astr, 0);
		XmListAddItemUnselected(ns.list, nstr, 0);
		/* also clear the server and addr fields; add to clearGui fcn? */
		clearAddr(ns.addr);
		XtVaSetValues(ns.nameServer, XmNvalue,	"", NULL);
		/* clear the gui and insensitize the buttons */
		clearGui();
		return;
	}

	/* malloc internal data structure if necessary */
	if (allocList(&(ns.resolv_C->serverList)) == FALSE) {
		createMsg(hi.net.common.toplevel, ERROR, mygettxt(ERR_memory), title);
		return;
	}

	/* store info in ns.nameStr and ns.addrStr so they can be used
	 * in the popup message.
	 */
	if (ns.addrStr) free(ns.addrStr);
	ns.addrStr = strdup(buf);
	if (ns.nameStr) free(ns.nameStr);
	ns.nameStr = strdup(server);

	/* 1 and 2 are for the titles in the list */
	if (ns.curSelectedPos == 3 || ns.curSelectedPos == 4) {
		/* popup confirmation box */
		ns.msgType = ADDFIRST;
		infoMsg(hi.net.common.toplevel, 
			(char *)mygettxt(TXT_insertInfo));
	}
	else {
		if (ns.curSelectedPos <= 0) {
			/* no selection was made, append at the end */
			count = ns.resolv_C->serverList.count;
			ns.resolv_C->serverList.list[count].name = strdup(server);
			getDotStrToADDR(buf, ns.resolv_C->serverList.list[count].addr);	
			ns.resolv_C->serverList.count++;
			
			XmListAddItemUnselected(ns.list, astr, 0);	
			XmListAddItemUnselected(ns.list, nstr, 0);	
			
		}
		else {
			/* insert the new entry at the selection position */
			row_num = (ns.curSelectedPos + (NUMCOLS-1))/NUMCOLS;
			(void)memmove((void *)&(ns.resolv_C->serverList.list[row_num-1]),
				&(ns.resolv_C->serverList.list[row_num - 2]),
				sizeof(hostEntry)*(ns.resolv_C->serverList.count - (row_num - 2)));

			ns.resolv_C->serverList.list[row_num-2].name 
				= strdup(server);
			getDotStrToADDR(buf, ns.resolv_C->serverList.list[row_num-2].addr);	
			ns.resolv_C->serverList.count++;

			if ((ns.curSelectedPos % NUMCOLS) == 1) {
				/* insert at this position */
				 XmListAddItemUnselected(ns.list, 
					astr, ns.curSelectedPos);	
				 XmListAddItemUnselected(ns.list, 
					nstr, ns.curSelectedPos+1);	
			}
			else {
				/* insert at this - 1 position */
				 XmListAddItemUnselected(ns.list, 
					astr, ns.curSelectedPos-1);	
				 XmListAddItemUnselected(ns.list, 
					nstr, ns.curSelectedPos);	
			}
		}
		/* clear the server and addr fields; add to clearGui fcn? */
		clearAddr(ns.addr);
		XtVaSetValues(ns.nameServer, XmNvalue,	"", NULL);
		/* clear the gui and insensitize the buttons */
		clearGui();
	}
	freeString(astr);
	freeString(nstr);
	free(server);
}

static void
delCB(Widget w, XtPointer client_data, XmAnyCallbackStruct *cbs)
{
	int	row_num, pos_list[2];

	row_num = (ns.curSelectedPos + (NUMCOLS -1))/NUMCOLS;
	/* delete the item from list */
	if ((ns.curSelectedPos % NUMCOLS) == 1) {
		pos_list[0] = ns.curSelectedPos;
		pos_list[1] = ns.curSelectedPos+1;
	}
	else {
		pos_list[0] = ns.curSelectedPos-1;
		pos_list[1] = ns.curSelectedPos;
	}
	XmListDeletePositions(ns.list, &pos_list[0], 2);
	/* clear out gui and insensitize the buttons */
	clearGui();
	/* delete the internal data structure */
	if (ns.resolv_C->serverList.list[row_num-2].name)
		free(ns.resolv_C->serverList.list[row_num-2].name);
	(void)memmove((void *)&(ns.resolv_C->serverList.list[row_num-2]),
		&(ns.resolv_C->serverList.list[row_num-1]),
		sizeof(hostEntry)*(ns.resolv_C->serverList.count - (row_num - 1)));	
	ns.resolv_C->serverList.count--;
}
static void
modCB()
{
	String		server;
	XmString	astr, nstr;
	char		buf[MEDIUMBUFSIZE];
	int		row_num;

	row_num = (ns.curSelectedPos + (NUMCOLS-1))/NUMCOLS;
	XtVaGetValues(ns.nameServer, XmNvalue, &server, NULL);
	if (getAddrToDotStr(ns.addr, buf) == False) {
		/* popup error message, address is required */;
		createMsg(hi.net.common.toplevel, INFO, mygettxt(INFO_addrNeed2), title);
		return;
	}
	/* update internal data structure */
	if (ns.resolv_C->serverList.list[row_num-2].name)
		free(ns.resolv_C->serverList.list[row_num-2].name);
	ns.resolv_C->serverList.list[row_num-2].name 
		= strdup(server);
	getDotStrToADDR(buf, ns.resolv_C->serverList.list[row_num-2].addr);
	astr = createString(buf);
	nstr = createString(server);
	if ((ns.curSelectedPos % NUMCOLS) == 1) {
		XmListReplaceItemsPos(ns.list, &astr, 1, ns.curSelectedPos);
		XmListReplaceItemsPos(ns.list, &nstr, 1, ns.curSelectedPos+1);
	}
	else {
		XmListReplaceItemsPos(ns.list, &astr, 1, ns.curSelectedPos-1);
		XmListReplaceItemsPos(ns.list, &nstr, 1, ns.curSelectedPos);
	}
	freeString(astr);
	freeString(nstr);
	clearGui();
}

static void
writeResolvFile(resolvConf * res)
{
	FILE    *fd;
	int	i,j;
	struct stat	statBuffer;
	char    buf[MEDIUMBUFSIZE], tmpaddr[MEDIUMBUFSIZE];
	Boolean	ret;

	if ((fd = fopen(ETCCONF_PATH, "w")) == NULL) {
		sprintf(buf, mygettxt(ERR_cantOpenFile), ETCCONF_PATH);
		createMsg(hi.net.common.toplevel, ERROR, buf ,title);
		return;
	}
	while (fstat(fileno(fd), &statBuffer) < 0 && errno == EINTR)
		/* try again */;

	sprintf(buf, "domain\t%s\n", res->domain);
	fputs(buf, fd);
	for (i=0; i< res->serverList.count; i++) {
		sprintf(tmpaddr, "%d.%d.%d.%d",
			res->serverList.list[i].addr[0],	
			res->serverList.list[i].addr[1],	
			res->serverList.list[i].addr[2],	
			res->serverList.list[i].addr[3]);	

		sprintf(buf, "nameserver\t%s\t# %s\n", tmpaddr,
			res->serverList.list[i].name);
			
		fputs(buf, fd);
	}
	fclose(fd);

	if (stat(NETCONFIG, &statBuffer) != 0) {
		/* printf out warning message */
		createMsg(hi.net.common.toplevel, WARN, mygettxt(WARN_noNetConfig), title);
	}
	
	/* update the _C and _P values */
	if (ns.domainName_P) free(ns.domainName_P);
		ns.domainName_P = strdup(ns.domainName_C);
	if ((ret = readResolvConf(hi.net.dns.resolv)) != TRUE) {
		sprintf(buf, mygettxt(ERR_cantOpenFile), ETCCONF_PATH);
		createMsg(hi.net.common.toplevel, ERROR, buf, title);
	}	
	/* finally sensitize the domain listing menu item */
	if (hi.net.common.cur_view == etcHost)
		XtVaSetValues(hi.net.common.menu[2].subitems[1].handle,
			XmNsensitive, True, NULL);
	XtUnmanageChild(ns.popup);
}

static void
verifyNameServer()
{
	int	i, j;
	char	buf[MEDIUMBUFSIZE], tmpaddr[MEDIUMBUFSIZE];
	Boolean isFound = TRUE;
	String	name;
	
	XtVaGetValues(ns.domainName, XmNvalue, &name, NULL);
	if (ns.resolv_C->domain)
		free(ns.resolv_C->domain);
	ns.resolv_C->domain = strdup(name);
	if (ns.domainName_C) free(ns.domainName_C);
	ns.domainName_C = strdup(name);

	/* Check if all the servers specified by the user actually existed */
	
	if (ns.server.serverList.count < 0) {
		sprintf(buf, ERR_noNameServer, ns.resolv_C->domain);	
		cleanup(buf);
		return;
	}

	/* ns.resolv_C stores info on the screen, ns.server stores info
	 * from the nslookup.
	 */
	for (i=0; i < ns.resolv_C->serverList.count; i++) {
		for (j=0; j< ns.server.serverList.count; j++) {
			/* check the address as the key, not the comment */
			if ((ns.resolv_C->serverList.list[i].addr[ADDR1]
				== ns.server.serverList.list[j].addr[ADDR1]) &&
				(ns.resolv_C->serverList.list[i].addr[ADDR2]
				== ns.server.serverList.list[j].addr[ADDR2]) &&	
				(ns.resolv_C->serverList.list[i].addr[ADDR3]
				== ns.server.serverList.list[j].addr[ADDR3]) &&	
				(ns.resolv_C->serverList.list[i].addr[ADDR4]
				== ns.server.serverList.list[j].addr[ADDR4])) {

					isFound = TRUE;
					break;
			}
			else {
				isFound = False;
			}	
		}
		if (isFound == False) {
			free(name);

	                /* write user's input to /etc/resolv.conf file */
			writeResolvFile(ns.resolv_C);
			return;
		}
	}

	/* After all servers are found in the ns.server, write back to the 
	 * file. We will use the ns.server list instead of ns.resolv_C list.
	 * Since if more servers are listed by the nslookup, we will put
	 * all of them in the /etc/resolv.conf file for the user.
	 */

	ns.server.domain = strdup(ns.resolv_C->domain);
	freeResolvConf(ns.resolv_C);
	if (copyResolvConf(&ns.server, &ns.resolv_C) == False) {
		sprintf(buf, mygettxt(ERR_cantOpenFile), ETCCONF_PATH);
		createMsg(hi.net.common.toplevel, ERROR, buf, title);
		return;
	}
	writeResolvFile(ns.resolv_C);
	free(name);
}

static void
updateGui(resolvConf * res)
{
	char		tmpaddr[MEDIUMBUFSIZE];
	XmString	*string;
	int		i, j;

	/* clean out the GUI first */
	clearGui();

	/* populate the primary name server and its address and put
 	 * the rest in the list.
	 */
	if (res->serverList.count > 0) {
		if (ns.server_C) free(ns.server_C);
		if (ns.server_P) free(ns.server_P);
		ns.server_C = strdup(res->serverList.list[0].name);
		ns.server_P = strdup(ns.server_C);
		XtVaSetValues(ns.nameServer, XmNvalue, ns.server_C, NULL);
		sprintf(tmpaddr, "%d.%d.%d.%d",
			res->serverList.list[0].addr[0],
			res->serverList.list[0].addr[1],
			res->serverList.list[0].addr[2],
			res->serverList.list[0].addr[3]);
		if (ns.addr_C) free(ns.addr_C);
		if (ns.addr_P) free(ns.addr_P);
		ns.addr_C = strdup(tmpaddr);
		ns.addr_P = strdup(ns.addr_C);
		setAddrGui(ns.addr, (unsigned char *)&res->serverList.list[0].addr[0]);
	}

	/* need to copy a current version of the data structure */
	freeResolvConf(ns.resolv_C);

	/* no need to go any further... nothing's been done! */
	if (res->serverList.count == 0 && ns.resolv_C == 0) {
		return;
	}
	if (copyResolvConf(res, &ns.resolv_C) == False) {
		/* BUG: use ERR_cantSaveResolv */
		createMsg(hi.net.common.toplevel, ERROR, mygettxt(ERR_memory), title);
	}
	
	XmListDeleteAllItems(ns.list);
	/* we need to multiply the size by 2 because of 2 columns list */
	string = (XmString *)XtMalloc(sizeof(XmString *) *
			(res->serverList.count +1) *2);	
	string[0] = i18nString(TXT_address);
	string[1] = i18nString(TXT_name);

	for (i=0, j=2; i<res->serverList.count; i++) {
		sprintf(tmpaddr, "%d.%d.%d.%d",
			res->serverList.list[i].addr[0],
			res->serverList.list[i].addr[1],
			res->serverList.list[i].addr[2],
			res->serverList.list[i].addr[3]);
		string[j++] = createString(tmpaddr);
		if (res->serverList.list[i].name) {
			string[j++] = createString(
				res->serverList.list[i].name);
		} else {
			string[j++] = XmStringCreateLocalized("");
		}
	}
	XtVaSetValues(ns.list,
		XmNitems, string,
		XmNitemCount, j,
		NULL);
	XtManageChild(ns.list);
	for (i=0; i<j; i++) {
		freeString(string[i]);
	}
	XtFree(string);
	ns.curSelectedPos = ns.preSelectedPos = -1;
}

static void
cleanup(char *msg)
{
	if (ns.outfile) {
		unlink(ns.outfile);
		free(ns.outfile);
		ns.outfile = NULL;
	}
	busyCursor(ns.domainName, FALSE);
	createMsg(hi.net.common.toplevel, ERROR, msg, title);
}

static
askForNameServers(char *domain)
{
	char *file;
	int   stat;
	char  cmd[256];
	char *execargs[5] = {"sh", "-c"};

	ns.outfile = (char *)getTmpFile();

	/* This awk part is different from the one in hosts.c.  This one
	 * will get the address also. Note: there may be case that 
	 * nameserver has 2 or more addresses. We show all the addresses 
	 * here.
	 */
	sprintf(cmd,"%s -type=ns %s | awk '/internet address/ {print $1, $5}'|sort -u > %s 2>/dev/null", "/usr/sbin/nslookup", domain, ns.outfile);

	execargs[2] = strdup(cmd);
	switch (ns.pid = fork()) {
        case 0:  /* child */
        setpgrp();
            execvp(execargs[0], execargs);
	    /* BUG: should check the command earlier?? */	
            perror(execargs[0]); 
            _exit(255);
        case -1:
		createMsg(hi.net.common.toplevel, ERROR, 
			mygettxt(ERR_cantFork), title);
    }
}


getServers(resolvConf server, char *file)
{
	FILE	*serverFile;
	char	serverName[MEDIUMBUFSIZE];
	char	addr[MEDIUMBUFSIZE];

	while(!(serverFile = fopen (file, "r")) && errno == EINTR)
		/* try again */;

	if (!serverFile)
	{
		/* caller will popup error message */
		return Failure;
	}
	/* clean up the serverList data structure */
	freeResolvConf(&ns.server);
	while (fscanf(serverFile,"%s%s",serverName, addr)!=EOF) {
		/* allocate the space for nameserver list if necessary */
		allocList(&(ns.server.serverList));
		/* store the comment as the nameserver  */
		ns.server.serverList.list[ns.server.serverList.count].name 
			= strdup(serverName);
		/* store the server address */
		parseAddr(ns.server.serverList.list[ns.server.serverList.count].addr, addr);
		ns.server.serverList.count++;
	}
	fclose(serverFile);
}

static void 
dnsQuery(int pid)
{
	int i;
	int status;
	char	buf[MEDIUMBUFSIZE];

	if(ns.pid == pid) { 
		if (getServers(ns.server, ns.outfile) == Failure) {
			sprintf(buf, mygettxt(ERR_cantOpenFile), ns.outfile);
			cleanup(buf);
			return;
		}
		else {
			if (ns.server.serverList.count <= 0) {
				sprintf(buf, mygettxt(ERR_noNameServer), ns.resolv_C->domain);
				cleanup(buf);
				return;
			}
		}
		switch (ns.queryFrom) {
			case DOMAINCHANGED:
				updateGui(&(ns.server));
				break;
			case OKPRESSED:
				verifyNameServer();
				break;
			default:
				break;
		}
		ns.queryFrom = NSNONE;
		busyCursor(ns.domainName, FALSE);
	}
}
				

static void
pre_dnsQuery()
{
	int	pid;
	int	status, exitCode;
	char	buf[MEDIUMBUFSIZE];

	if ((pid = wait(&status)) == -1) {
		/* an error of some kind (fork probably failed); ignore it- cantWait*/
		XtAppAddTimeOut(hi.net.common.app, 0, cleanup, mygettxt(ERR_cantWait));
	}
	else {
		if (WIFEXITED(status) == 0) {
			/* teminate abnormally */
			if (WIFSIGNALED(status) != 0) {
				XtAppAddTimeOut(hi.net.common.app, 0, cleanup, mygettxt(ERR_killBySignal));
			}
			else {
				XtAppAddTimeOut(hi.net.common.app, 0, cleanup, mygettxt(ERR_unkown));
			}
		}
		else {
			/* terminate normally */
			exitCode = WEXITSTATUS(status);
			switch (exitCode) {
			case 0:	 /* query is successful */
				XtAppAddTimeOut(hi.net.common.app, 0, dnsQuery, pid);
				break;
			case 1: /* fall thru */
			default: /* query is failed */
				sprintf(buf, mygettxt(ERR_noNameServer), ns.resolv_C->domain);
				XtAppAddTimeOut(hi.net.common.app, 0, cleanup, buf);
				break;
			}
		}
	}
}

static void
domainNameChangedCB(Widget w, XtPointer client_data, XmAnyCallbackStruct *cbs)
{
	String	name;
	char	*lname, *lptr;	

	/* if the DNS is not configureed, don't try to query */
	if (hi.net.common.isDnsConfigure != True)
		return;

	XtVaGetValues(ns.domainName, XmNvalue, &name, NULL);
	lname = strdup(name);
	lptr = &lname[0];
	while (*lptr != 0) {
		*lptr = tolower(*lptr);
		lptr++;
	}
	if (strstr(lname, ".com") == NULL &&
		strstr(lname, ".edu") == NULL &&
		strstr(lname, ".gov") == NULL) {
		ns.msgType = DOMAINNAME;
		createMsg(
			hi.net.common.toplevel,
			INFO,
			(char *)mygettxt(TXT_invalidDomainName),
			TXT_nsaView2
		);
	}
	else {
		/* Use the domain to get all the name servers and populate 
		 * them on the screen, assuming the domain exists.
		 */

		busyCursor(ns.domainName, TRUE);
		signal(SIGCHLD, (void(*)())pre_dnsQuery);
		ns.queryFrom = DOMAINCHANGED;
		askForNameServers(name);
	}
	free(lname);
}

static void
addrChangedCB(Widget w, XtPointer client_data, XmAnyCallbackStruct *cbs)
{
	XtVaSetValues(ns.button[ADD], XmNsensitive, True, NULL);
}

int
parseEntry(char ** server, char **addr)
{
	char	tmpstr[MEDIUMBUFSIZE];
	int	row_num;

	row_num = (ns.curSelectedPos + (NUMCOLS - 1))/NUMCOLS;
	/* the actual data structure index is row_num - 2
	 * one for the title row, one for the difference of starting number.
	 */
	*server	 = strdup(ns.resolv_C->serverList.list[row_num-2].name);
	sprintf(tmpstr, "%d.%d.%d.%d",
		ns.resolv_C->serverList.list[row_num-2].addr[0],	
		ns.resolv_C->serverList.list[row_num-2].addr[1],	
		ns.resolv_C->serverList.list[row_num-2].addr[2],	
		ns.resolv_C->serverList.list[row_num-2].addr[3]);	
	*addr = strdup(tmpstr);
}

int
setAddrFromDotStr(inetAddr addr, char * dotString)
{
	char	tmpaddr[MEDIUMBUFSIZE];
	char	*token, *tmpstr;

	tmpstr = strdup(dotString);
	token = strtok(tmpstr, ".");
	XtVaSetValues(addr.addr[ADDR1], XmNvalue, token, NULL);
	token = strtok(NULL, ".");
	XtVaSetValues(addr.addr[ADDR2], XmNvalue, token, NULL);
	token = strtok(NULL, ".");
	XtVaSetValues(addr.addr[ADDR3], XmNvalue, token, NULL);
	token = strtok(NULL, " ");
	XtVaSetValues(addr.addr[ADDR4], XmNvalue, token, NULL);
}

static void
singleClickCB(Widget list, XtPointer client_data, XmListCallbackStruct *cbs)
{
	char		*string;
	static Boolean	first=True;
	int		rowNum, index;

	ns.curSelectedPos = cbs->item_position;
	parseEntry(&ns.server_C, &ns.addr_C);
	if (first) {
		ns.preSelectedPos = ns.curSelectedPos;
		ns.server_P = strdup(ns.server_C);
		ns.addr_P = strdup(ns.addr_C);
		first = False;
	}
	/* populate the selection to the gui */
	XtVaSetValues(ns.nameServer, XmNvalue, ns.server_C, NULL);
	setAddrFromDotStr(ns.addr, ns.addr_C);
	XtVaSetValues(ns.button[ADD], XmNsensitive, True, NULL);	
	XtVaSetValues(ns.button[MODIFY], XmNsensitive, True, NULL);	
	XtVaSetValues(ns.button[DELETE], XmNsensitive, True, NULL);	
}


static void
okCB() 
{
	FILE	*fd;
	String	name;
	struct  stat statBuffer;
	char    buf[MEDIUMBUFSIZE];

	XtVaGetValues(ns.domainName, XmNvalue, &name, NULL);
	removeWhitespace(name);
	if (!strlen(name) || ns.resolv_C == 0) {
		createMsg(
			hi.net.common.toplevel,
			ERROR,
			mygettxt(ERR_dnsIncomplete),
			title
		);
		if (name)
			free(name);
		return;
	}
	if (ns.resolv_C->serverList.count == 0) {
		XtUnmanageChild(ns.popup);
		unlink(ETCCONF_PATH);
		systemsListCB();
		XtVaSetValues(
			hi.net.common.menu[2].subitems[1].handle,
			XmNsensitive,	False,
			NULL
		);
		hi.net.common.isDnsConfigure = False;
		createMsg(
			hi.net.common.toplevel,
			INFO,
			mygettxt(ERR_disablingDNS),
			title
		);
		clearGui();
		XmListDeleteAllItems(ns.list);
		/*
		cleanupDns();
		*/
		return;
	}
	ns.resolv_C->domain = strdup(name);
	hi.net.common.isDnsConfigure = True;
	writeResolvFile(ns.resolv_C);
}

static void
resetCB() 
{
	int		i;
	XmString	astr, nstr;
	char		buf[MEDIUMBUFSIZE], tmpaddr[MEDIUMBUFSIZE];

	if (ns.domainName_C) free(ns.domainName_C);
	if (ns.domainName_P)
		ns.domainName_C = strdup(ns.domainName_P);
	XtVaSetValues(ns.domainName, XmNvalue, ns.domainName_C, NULL);

	updateGui(hi.net.dns.resolv);
}

static void
cancelCB() 
{
	XtUnmanageChild(ns.popup);
}

static void
decideScreenLayout()
{
	char		tmpaddr[MEDIUMBUFSIZE];
	int		i, j;
	char		entry[MEDIUMBUFSIZE];
	XmString *	string;
	Boolean		ret;
	static Boolean	first = True;


	/* check the timer on the resolvConf file, re-read the file
	 * if necessarty.
	 */
	
	if ((ret = readResolvConf(hi.net.dns.resolv)) == FALSE) {
		/* the DNS is not setup at all. We need to set up
		 * the GUI here.
		 */
		string = (XmString *)XtMalloc(sizeof(XmString *) * 2);
		string[0] = i18nString(TXT_address);
		string[1] = i18nString(TXT_name);
		XtVaSetValues(ns.list, XmNitems, string,
			XmNitemCount, 2, NULL);
		XmStringFree(string[0]);
		XmStringFree(string[0]);
		XtFree(string);
		return;
	}
	if (hi.net.common.isDnsConfigure == True) {
		/* populate the information from the resolv data structure */
		ns.domainName_C = strdup(hi.net.dns.resolv->domain);
		ns.domainName_P = strdup(ns.domainName_C);
		XtVaSetValues(ns.domainName, XmNvalue, ns.domainName_C, NULL);
		updateGui(hi.net.dns.resolv);
	}
	else {
		/* leave the popup blank. */;
	}
}

static void
setupGui(Widget w, int client_data, XmAnyCallbackStruct * cbs)
{
	static Widget	topForm;
	Widget		nsa_rc; 
	Widget		sep0, sep1, sep2, rc1, rc2, rc3, but_rc, form, form1;
	Widget		form2, form3, servers, nis_servers;
	int		len, i, j;
	Dimension	maxWidth;
	XmString	string;
	Arg		args[10];
	DmMnemonicInfoRec	mneInfo[MI_TOTAL];
	unsigned char *		mne;
	char			tmpstr[BUFSIZ];
	KeySym			mneks;
	XmString		mneString;
	int			mm;

	if (!ns.popup) {
		ns.popup = XtVaCreatePopupShell("name_server_popup",
			xmDialogShellWidgetClass, XtParent(w),
			NULL);
		title = mygettxt(TXT_nsaView2);
		XtVaSetValues(ns.popup,
			XmNtitle, title,
			XmNallowShellResize, True,
			XmNdeleteResponse, XmUNMAP,
			NULL);

		r_decor(ns.popup);

		topForm = XtVaCreateWidget("ns_topForm",
			xmFormWidgetClass, ns.popup,
			NULL);
		
		nsa_rc = XtVaCreateManagedWidget("nsa_rowColumn",
			xmRowColumnWidgetClass, topForm,
			XmNorientation, XmVERTICAL,
			NULL);

		XtAddCallback(nsa_rc, XmNhelpCallback, helpkeyCB, &NSHelp);

		maxWidth = getMaxWidth(
			nsa_form,
			xmLabelWidgetClass,
			XmNlabelString,
			nsa_rc
		);

		ns.domainName = crt_inputlabel(nsa_rc, TXT_LDN, maxWidth);
		(void)XtVaCreateManagedWidget(
			"separator",
                        xmSeparatorWidgetClass,
			nsa_rc,
			NULL
		);
		XtAddCallback(ns.domainName, XmNactivateCallback,
			(void(*)())domainNameChangedCB, NULL);
		/*XtAddCallback(ns.domainName, XmNlosingFocusCallback,
			(void(*)())domainNameChangedCB, NULL); */
		ns.nameServer = crt_inputlabel(nsa_rc, TXT_name_server, maxWidth);
		crt_inet_addr(nsa_rc, TXT_inet_addr, maxWidth, &(ns.addr));
		for (i = 0; i < 4; i++) {
			XtAddCallback(ns.addr.addr[i], XmNvalueChangedCallback,
				(void(*)())addrChangedCB, NULL);
		}
		(void)crt_inputlabellabel(nsa_rc, "", maxWidth, TXT_servers2);
		form = XtVaCreateManagedWidget("form",
			xmFormWidgetClass, nsa_rc, NULL);
		/*DWD*/
		for (i = 0; i < (XtNumber(nsa_buttons) - 1); i++) {
			XtWidgetGeometry	g;
			Dimension		d;
			XmString		xms;

			g.request_mode = CWWidth;
			d = getMaxWidth(
				nsa_buttons,
                                xmPushButtonWidgetClass,
                                XmNlabelString,
                                nsa_rc
                        );

			strcpy(tmpstr, mygettxt(nsa_buttons[i]));
                        ns.button[i] = XtVaCreateManagedWidget(
                                tmpstr, xmPushButtonWidgetClass, form,
                                XmNwidth,               d,
                                XmNleftAttachment,      XmATTACH_FORM,
                                XmNleftOffset,          maxWidth - d,
                                XmNsensitive,           False,
                                NULL
                        );
			XmSTR_N_MNE(
				nsa_buttons[i],
				nsa_button_mnemonics[i],
				mneInfo[i],
				DM_B_MNE_ACTIVATE_CB | DM_B_MNE_GET_FOCUS
			);
			XmStringFree(mneString);

			XtVaSetValues(ns.button[i], XmNmnemonic, mneks, NULL);

                        if (i == 0) {
                                XtVaSetValues(
                                        ns.button[i],
                                        XmNtopAttachment,       XmATTACH_FORM,
                                       NULL
                                );
                        } else {
                                XtVaSetValues(
                                        ns.button[i],
                                        XmNtopAttachment, XmATTACH_WIDGET,
                                        XmNtopWidget, ns.button[i-1],
                                        NULL
                                );
                        }
                        if (i == 2) {
                                XtVaSetValues(
                                        ns.button[i],
                                        XmNbottomAttachment, XmATTACH_WIDGET,
                                        NULL
                                );
                        }
                }

		XtAddCallback(ns.button[ADD], XmNactivateCallback, (void(*)())addCB, NULL);
		mneInfo[ADD].w = ns.button[ADD];
		mneInfo[ADD].cb = (XtCallbackProc)addCB;
		XtAddCallback(ns.button[MODIFY], XmNactivateCallback, (void(*)())modCB, NULL);
		mneInfo[MODIFY].w = ns.button[MODIFY];
		mneInfo[MODIFY].cb = (XtCallbackProc)modCB;
		XtAddCallback(ns.button[DELETE], XmNactivateCallback, (void(*)())delCB, NULL);
		mneInfo[DELETE].w = ns.button[DELETE];
		mneInfo[DELETE].cb = (XtCallbackProc)delCB;

		j=0;
		XtSetArg(args[j], XmNtopAttachment, XmATTACH_FORM); j++;
		XtSetArg(args[j], XmNbottomAttachment, XmATTACH_FORM); j++;
		XtSetArg(args[j], XmNleftAttachment, XmATTACH_FORM); j++;
		XtSetArg(args[j], XmNrightAttachment, XmATTACH_FORM); j++;
		XtSetArg(args[j], XmNleftOffset, (maxWidth + 2)); j++;
		XtSetArg(args[j], XmNvisibleItemCount, 4); j++;
		XtSetArg(args[j], XmNlistColumnSpacing, 5); j++;
		XtSetArg(args[j], XmNnumColumns, NUMCOLS); j++;
		XtSetArg(args[j], XmNstaticRowCount, 1); j++;
		ns.list = XmCreateScrolledList(form, "list", args, j);	

		XtAddCallback(ns.list, XmNbrowseSelectionCallback,
			(void(*)())singleClickCB, NULL);
		XtManageChild(ns.list);
		sep1 = XtVaCreateManagedWidget("spe1",
                        xmSeparatorWidgetClass, nsa_rc, NULL);
                createActionArea(
			nsa_rc,
			actions,
                        XtNumber(actions),
			ns.domainName,
			mneInfo,
			3
		);
                sep2 = XtVaCreateManagedWidget("spe2",
                        xmSeparatorWidgetClass, nsa_rc, NULL);
                ns.status = XtVaCreateManagedWidget("nsa_status",
                        xmLabelWidgetClass, nsa_rc,
                        NULL);

		setLabel(ns.status, "");

		XtVaSetValues(topForm,
			XmNdefaultButton, actions[0].widget,
			XmNcancelButton, actions[2].widget,
			NULL);

		/* add mini-help */
		for (i = 0; i < 3; i++) {
			nsHelp[i].widget = ns.status;
		}
		XtAddEventHandler(
			ns.domainName,
			FocusChangeMask,
			False,
			(void(*)())displayFooterMsgEH,
			(XtPointer)&nsHelp[0]
		);
		XtAddEventHandler(
			ns.nameServer,
			FocusChangeMask,
			False,
			(void(*)())displayFooterMsgEH,
			(XtPointer)&nsHelp[1]
		);
		for (i = 0; i < 4; i++) {
			XtAddEventHandler(
				ns.addr.addr[i],
				FocusChangeMask,
				False,
				(void(*)())displayFooterMsgEH,
				(XtPointer)&nsHelp[2]
			);
		}
		
		REG_MNE(ns.popup, mneInfo, XtNumber(actions) + 3);
	}	

	XtManageChild(topForm);
	decideScreenLayout();
	XtManageChild(ns.popup);
}

void
createNameServerAccess(Widget w, int client_data, XmAnyCallbackStruct * cbs)
{
	setupGui(w, client_data, cbs);
}

static void
cleanupDns()
{
	freeResolvConf(ns.resolv_C);
	freeResolvConf(&ns.server);
	freeResolvConf(hi.net.dns.resolv);
	if (ns.domainName_C) free(ns.domainName_C);
	if (ns.domainName_P) free(ns.domainName_P);
	if (ns.server_C) free(ns.server_C);
	if (ns.server_P) free(ns.server_P);
	if (ns.addr_C) free(ns.addr_C);
	if (ns.addr_P) free(ns.addr_P);
	if (ns.addrStr) free(ns.addrStr);
	if (ns.nameStr) free(ns.nameStr);
}
