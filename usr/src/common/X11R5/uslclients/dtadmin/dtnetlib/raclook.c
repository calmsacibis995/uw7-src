#ifndef NOIDENT
#pragma ident	"@(#)dtnetlib:raclook.c	1.1"
#endif


#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <Xm/RowColumn.h>
#include <Xm/MwmUtil.h>
#include "lookup.h"
#include "lookupG.h"
#include "util.h"
#include "inet.h"

extern int	createHostsWin(netInfo *);

static	Widget statusWid;

int
lookup(Widget pb, Widget txt)
{
	void	env_set_gbl();
	char	buf[128];
	static	Boolean first=TRUE;

/*
	myhelp.title = strdup("MpHelp");
	myhelp.file = strdup("dtadmin/DialMgr.hlp");
	myhelp.section = strdup("10");

	myudata.text = txt;
	myudata.help = &myhelp;
	myudata.hostSelected = False;
	myudata.prevVal = NULL;
	HostCB(pb, &myudata); 
*/
	if (first) {
		hi.net.common.toplevel = NULL;
		first = FALSE;
	}
	hi.net.common.isInet = FALSE;
	hi.net.common.isFirst = TRUE;
	hi.net.lookup.clientText = txt;
	hi.net.common.app = XtWidgetToApplicationContext(pb);

	createHostsWin(&(hi.net));

	return(0);
}
