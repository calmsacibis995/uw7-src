#ifndef NOIDENT
#ident	"@(#)dtadmin:internet/search.c	1.11"
#endif

#include <stdio.h>
#include <sys/types.h>

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
extern int	makePosVisible(Widget, int);
extern void	createMsg(Widget, msgType, char *, char *);
extern XmString	createString(char *);
extern void	freeString(XmString);
extern Boolean	removeWhitespace(char *);
extern void	helpkeyCB(Widget, XtPointer, XtPointer);
extern char	*mygettxt(char *);
extern int	r_decor(Widget);

char	*search_form[]={
	TXT_system_name,
	"",
};

void searchCB();
void lastCB();
void firstCB();
static void cancelCB();

HelpText	SHelp = { NULL, NULL, "260" };

static char *title;
static ActionAreaItems actions[] = {
		{ TXT_Search,	MNE_Search, searchCB, NULL	},	
		{ TXT_Cancel,	MNE_Cancel, cancelCB, NULL	},	
		{ TXT_Help,	MNE_Help, helpkeyCB,	&SHelp	},
	};

typedef struct _search {
	Widget		popup;
	Widget		sysName;
	ActionAreaItems	*actions;
	Widget		status;
} search;

search	s;

void
firstCB() 
{
	Widget	list;
	int	index;

	if (hi.net.common.cur_view == etcHost)  {
		list = hi.net.etc.etcList;
		index = ETCCOLS+1;
	}
	else {
		list = hi.net.dns.dnsArray[hi.net.dns.cur_wid_pos];
		index = 1;
	}

	XmListSelectPos(list, index, True);
	XmListSetKbdItemPos(list, index);
}

void
lastCB()
{
	Widget	list;
	if (hi.net.common.cur_view == etcHost) 
		list = hi.net.etc.etcList;
	else 
		list = hi.net.dns.dnsArray[hi.net.dns.cur_wid_pos];

	XmListSelectPos(list, 0, True);
	XmListSetKbdItemPos(list, 0);
}

void
searchCB() 
{
	String		name;
	int		i;
	XmString	str;
	Widget		list;
	int		arrayIndex, listIndex;
	char		buf[MEDIUMBUFSIZE];
	XmStringTable	items;
	int		num_items;

	static viewType	lastView = 2;
	static int	lastIndex;

	if (lastView != hi.net.common.cur_view) {
		lastIndex = -1;
	}
	lastView = hi.net.common.cur_view;
	XtVaGetValues(s.sysName, XmNvalue, &name, NULL);
	removeWhitespace(name);
	if (hi.net.common.cur_view == etcHost) {
		/* for etcHost, we need to look at the actual list */
		list = hi.net.etc.etcList;
		arrayIndex = ((hi.net.etc.etcHostIndex + (ETCCOLS-1))/ETCCOLS)-2;
		if (lastIndex == -1) {
			i = arrayIndex;
		} else {
			i = lastIndex;
		}
		for ( ; i < hi.net.etc.etcHosts->count; i++) {
			if (strstr(hi.net.etc.etcHosts->list[i].etcHost.name, name)) {
				listIndex = ((i+2)*ETCCOLS)+1-ETCCOLS;
				lastIndex = i + 1;
				XmListSelectPos(list, listIndex, True);
				XmListSetKbdItemPos(list, listIndex);
				return;
			}	
		}
		for (i = 0; i < arrayIndex; i++) {
			if (strstr(hi.net.etc.etcHosts->list[i].etcHost.name, name)) {
				listIndex = ((i+2)*ETCCOLS)+1-ETCCOLS;
				lastIndex = i + 1;
				XmListSelectPos(list, listIndex, True);
				XmListSetKbdItemPos(list, listIndex);
				return;
			}	
		}
	} else {
		list = hi.net.dns.dnsArray[hi.net.dns.cur_wid_pos];
		arrayIndex = hi.net.dns.dnsIndex[hi.net.dns.cur_wid_pos];
		XtVaGetValues(
			list,
			XmNitemCount,	&num_items,
			XmNitems,	&items,
			NULL
		);
		if (lastIndex == -1) {
			i = arrayIndex - 1;
		} else {
			i = lastIndex;
		}
		for ( ; i < num_items; i++) {
			String	listItem;

			XmStringGetLtoR(
				items[i],
				XmFONTLIST_DEFAULT_TAG,
				&listItem
			);
			if (strstr(listItem, name)) {
				lastIndex = i + 1;
				XmListSelectPos(list, lastIndex, True);
				XmListSetKbdItemPos(list, lastIndex);

				XtFree(listItem);
				return;
			}
			XtFree(listItem);
		}
		for (i = 0; i < arrayIndex; i++) {
			String	listItem;

			XmStringGetLtoR(
				items[i],
				XmFONTLIST_DEFAULT_TAG,
				&listItem
			);
			if (strstr(listItem, name)) {
				lastIndex = i + 1;
				XmListSelectPos(list, lastIndex, True);
				XmListSetKbdItemPos(list, lastIndex);

				XtFree(listItem);
				return;
			}
			XtFree(listItem);
		}
	} /* part of dns list */

	/* not found */
	sprintf(buf, mygettxt(INFO_cantFindEntry), name);
	createMsg(hi.net.common.toplevel, INFO, buf, title);

	return;
}

static void
cancelCB() {
	XtUnmanageChild(s.popup);
}

void
createSearch(w, client_data, cbs)
Widget			w;
int			client_data;
XmAnyCallbackStruct	*cbs;
{
	static Widget	s_rc, topForm; 
	Widget		sys_rc, sep1, sep2;
	int		maxWidth;
	XmString	string;
	DmMnemonicInfoRec	mneInfo[MI_TOTAL];
	int			mm;


	if (!s.popup) {
		s.popup = XtVaCreatePopupShell("search",
			xmDialogShellWidgetClass, XtParent(w),
			NULL);
		title = mygettxt(TXT_internetSearch);
		XtVaSetValues(s.popup,
			/*XmNtitle, "Internet Setup: Search",*/
			XmNtitle, title,
			XmNallowShellResize, True,
			XmNdeleteResponse, XmUNMAP,
			NULL);

		r_decor(s.popup);
		
		topForm = XtVaCreateWidget("topForm",
			xmFormWidgetClass, s.popup,
			NULL);

		s_rc = XtVaCreateManagedWidget("ans_rowColumn",
			xmRowColumnWidgetClass, topForm,
			XmNorientation, XmVERTICAL,
			NULL);
		
		XtAddCallback(s_rc, XmNhelpCallback, helpkeyCB, &SHelp);

		sys_rc = XtVaCreateManagedWidget("sys_rc",
			xmRowColumnWidgetClass, s_rc,
			XmNorientation, XmHORIZONTAL, NULL);

		s.sysName = crt_inputlabel(sys_rc, TXT_system_name, 0);

		sep1 = XtVaCreateManagedWidget("spe1",
			xmSeparatorWidgetClass, s_rc, NULL);
		createActionArea(
			s_rc,
			actions, 
			XtNumber(actions),
			s.sysName,
			mneInfo,
			0
		);
		sep2 = XtVaCreateManagedWidget("spe2",
			xmSeparatorWidgetClass, s_rc, NULL);
		s.status = XtVaCreateManagedWidget("s_status",
			xmLabelWidgetClass, s_rc,
			NULL);

		setLabel(s.status, "");
		XtVaSetValues(topForm, 
			XmNdefaultButton, actions[0].widget,
			XmNcancelButton, actions[1].widget,
			NULL);
		REG_MNE(s.popup, mneInfo, XtNumber(actions));
	}	
	XtManageChild(topForm);
	XtManageChild(s.popup);
}
