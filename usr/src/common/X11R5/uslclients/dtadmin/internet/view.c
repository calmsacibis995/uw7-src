#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:internet/view.c	1.23"
#endif


#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/PushBG.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/List.h>
#include <X11/cursorfont.h>
#include <signal.h>
#include <sys/wait.h>
#include "lookup.h"
#include "lookupG.h"
#include "inetMsg.h"
#include "util.h"
#include "inet.h"

extern void setEtcList();
extern void etcMenuBar();
extern void dnsMenuBar();
extern void createExitMsg();
extern Widget	createDnsHosts(netInfo *);
extern void	createProperty(Widget, XtPointer, XtPointer);
extern int	resizeMainWindow(netInfo *);
extern char 	mygettxt(char *);
static char *title;

void systemsListCB(Widget w)
{
	int ret;

	/* parse the file */
	if((ret = readEtcHosts(hi.net.etc.etcHosts)) == Failure)
		return;
	/* update the scrolled list */
	setEtcList(hi.net.etc.etcList, hi.net.etc.etcHosts, ret);
	if (hi.net.common.isDnsConfigure == TRUE)
		XtUnmanageChild(hi.net.dns.dnsRC);
	title = mygettxt(TXT_etcHostView);
	/* check etcHosts exist , if both not exiist, error msg*/
	if (hi.net.etc.etcHosts != NULL)
		XtManageChild(hi.net.etc.etcRC);
	else {
		/* pop up error msg */;
		createExitMsg(hi.net.common.toplevel, ERROR, mygettxt(ERR_noSetup), title);
		return;
	}
        hi.net.common.cur_view = etcHost;
        XtVaSetValues(hi.net.common.toplevel,
                XmNtitle, mygettxt(TXT_etcHostView),
                NULL);
	etcMenuBar();
}

void dnsListCB(Widget w)
{
	Boolean dnsExists;
	readHostsReturn ret;

	/* This will be called even if DNS is not configured at the
	 * start up time. So check for the dnsHosts existence.
	 */

	if ((dnsExists = readResolvConf(hi.net.dns.resolv)) == TRUE) {
		hi.net.common.isDnsConfigure = TRUE;
		if (hi.net.dns.dnsRC == NULL) {
			hi.net.dns.dnsRC = createDnsHosts(&hi.net);
			if (hi.net.dns.dnsRC == NULL) {
				createMsg(
					hi.net.common.toplevel,
					WARN,
					mygettxt(ERR_noDnsSetup2),
					title
				);

				return;
			}
			XtVaSetValues(hi.net.dns.domainText,
				XmNvalue, hi.net.dns.resolv->domain, NULL);
			/* need to resize the 3 panes here */
			resizeMainWindow(&(hi.net));
		}
		else {
			/* 2nd or later time comes in here, just map the
			 * RC.
			 */
			XtManageChild(hi.net.dns.dnsRC);
		}
		hi.net.dns.clickCB = createProperty;
		/* check the etcRC exiist */
		if (hi.net.etc.etcHosts != NULL) {
			XtUnmanageChild(hi.net.etc.etcRC);
		}
		hi.net.common.cur_view = DNS;
		title = mygettxt(TXT_dnsView);
		XtVaSetValues(hi.net.common.toplevel,
			XmNtitle, mygettxt(TXT_dnsView),
			NULL);
		dnsMenuBar();
	}
	else {
		/* DNS is still not set up */
		createMsg(hi.net.common.toplevel, WARN, mygettxt(ERR_noDnsSetup2), title);
	}
}

void  listDefaultCB(Widget w, XtPointer client_data, XmListCallbackStruct *cbs)
{
        if (hi.net.common.cur_view == etcHost) {
                hi.net.etc.etcHostIndex = cbs->item_position;
		if (hi.net.etc.etcSelection) free(hi.net.etc.etcSelection);
		hi.net.etc.etcSelection =
			strdup(hi.net.etc.etcHosts->list[hi.net.etc.etcHostIndex -1].etcHost.name);
	}
        else {
                hi.net.dns.dnsIndex[0] = cbs->item_position;
	}
}
