/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtnetlib:hosts.c	1.41.1.1"
#endif

#include <Xm/DialogS.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/PushBG.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#include <Xm/List.h>
#include <Xm/ArrowB.h>
#include <Xm/Separator.h>
#include <Xm/SeparatoG.h>
#include <Xm/SelectioB.h>
#include <Xm/Scale.h>
#include <Xm/TextF.h>
#include <X11/cursorfont.h>
#include "DesktopP.h"

#include <Dt/Desktop.h>

#include <Xm/AtomMgr.h>
#include <Xm/DragDrop.h>
#include <X11/Xatom.h>

#include <signal.h>
#include <sys/wait.h>
#include "lookup.h"
#include "lookupG.h"
#include "lookupMsg.h"
#include "util.h"

netInfo	*net;
Widget	dialog, slider;

extern Widget		crt_inputlabel(Widget, char *, int);
extern Widget		crt_inputlabellabel(Widget, char *, int, char *);
extern Widget	createActionArea(Widget, ActionAreaItems *, int, Widget, DmMnemonicInfo, int);
extern XmString		i18nString(char *);
extern void		freeString(XmString);
extern Boolean		isInDomainList(int, dnsList*);
extern int		makePosVisible(Widget, int);
extern void		createMsg(Widget, msgType, char *, char *);
extern void		createExitMsg(Widget, msgType, char *, char *);
extern DmGlyphPtr	DmGetPixmap(Screen *screen, char *name);
extern Boolean		getRemoteAccessPath(Widget, char *, char **);
extern Boolean		createRemoteAccessFile(Widget, char *);
extern int		XmListXYToPos(Widget, Position, Position);
extern void		ListProcessDrag(Widget, XEvent *, String *, Cardinal *);
extern int		findNameServers(char *, hostList *);
extern int		querynameServers(char *, hostEntry, hostList *, hostList *);
extern char *		mygettxt(char *);
extern void		racDisplayHelp(Widget, HelpText *);
extern Boolean		removeWhiteSpace(char *);

void okCB();
void cancelCB();
void helpCB();
void leftArrowCB();
void rightArrowCB();
void dnsListDefCB();
void etcListDefCB();
void dnsListBrowseCB();
void etcListBrowseCB();
void showCB();
void mappLists();
void updateArrowBut();
void setDnsList();
void busyCursor();
readHostsReturn readDnsHosts(char *);
void	itemInitProc(Widget, XtPointer, XtPointer);
void	dnsInitProc(Widget, XtPointer, XtPointer);
void	startDrag();
void	btn1Action();
Boolean	CheckForInterrupt();
void	TimeoutCursors();
static void	domainNameChangedCB(Widget, XtPointer, XtPointer);

typedef enum {
	IntrStart,
	IntrCheck,
	IntrEnd
} IntrVal;

typedef enum {
	OK,
	FAILED,
	INTERRUPTED,
} queryReturn;

static Boolean stopped;

HelpText	RACHelp = { NULL, NULL, "40" };

static ActionAreaItems	actions[] = {
	{ TXT_OK,	MNE_OK,		okCB, 		NULL },
	{ TXT_Cancel,	MNE_Cancel,	cancelCB, 	NULL },
	{ TXT_Help,	MNE_Help,	helpCB, 	&RACHelp },
};

static DmGlyphPtr	d_glyph=None, h_glyph=None;

/* Translations and actions. Pressing mouse button 2 on the
 * list item overrides the normal list action and call startDrag
 * to start a drag transaction. 
 */

static const char dragTranslations2[] =
        "<Btn2Down>:startDrag()";

#define CLICK_ACTION		0
#define MOVE_ACTION		1
static const char dragTranslations1[] = "\
~c ~s ~m ~a<Btn1Down>:btn1Action(ListBeginSelect,startDrag)\n\
c ~s ~m ~a<Btn1Down>:btn1Action(ListBeginToggle,startDrag)\n\
~m ~a<Btn1Down>:startDrag()";

static XtActionsRec dragActions[] =
	{{ "btn1Action", btn1Action },
         {"startDrag", startDrag} };

static Bool
LookForButton(Display *display, XEvent *event, XPointer arg)
{
#define DAMPING	5
#define ABS_DELTA(x1, x2) (x1 < x2 ? x2 - x1 : x1 - x2)

	Bool	ret_value = False;

	if (event->type == ButtonRelease) {
			ret_value = True;
	}
	else if (event->type == MotionNotify) {

		XEvent *press = (XEvent *)arg;

		if (ABS_DELTA(press->xbutton.x_root, 
			event->xmotion.x_root) > DAMPING ||
			ABS_DELTA(press->xbutton.y_root,
			event->xmotion.y_root) > DAMPING)
			ret_value = True;

	}
	return(ret_value);
}

static Boolean
TestInSelection(XmListWidget w, XEvent *event)
{
	int 	cur_item, *pos_list, pos_count, i;
	XEvent	new;
	Boolean ret_value = True;

	if ((cur_item = XmListXYToPos((Widget)w, 
		event->xbutton.x, event->xbutton.y)) < 0)

		return True;
/*
	if (XmListGetSelectedPos((Widget)w, &pos_list, &pos_count) == False) 
		return True;
	else {

		for (i=0; i < pos_count; i++) {
			if (cur_item == pos_list[i]) {
				-- Inside the list, so it is a drag --
				ret_value = False;
				break;
			}
		}

		free(pos_list);

		if (ret_value == True)
			return True;
	}
*/

	XPeekIfEvent(XtDisplay((Widget)w), &new, LookForButton, (XPointer)event);
	return(new.type == MotionNotify ? False: True);
}

/* This routine create the RemoteAccess file and convert the name to
 * compund string and return the values back to the drop site.
 */

static Boolean
DragConvertProc(Widget w, Atom *selection, Atom	*target, Atom *type, XtPointer *value, unsigned long *length, int *format)
{
	Arg	args[1];
	char	*path;
	XmString	cstring;
	char		tmpstr[MEDIUMBUFSIZE];
	char		*ctext, *passtext;

	/* Only handle COMPOOUND_TEXT and FILE_NAME targets */
	if (*target != net->common.COMPOUND_TEXT &&
	*target != OL_XA_FILE_NAME(XtDisplay(w))) {
		return(False);
	}
	
	XtSetArg(args[0], XmNclientData, &path);
	XtGetValues(w, args, 1);

	if (createRemoteAccessFile(net->common.toplevel, path) == False)
		return(False);

	if (*target == net->common.COMPOUND_TEXT) {
	/* get the client data, convert into compond string and 
	 * create the file.
	 */

	cstring = XmStringCreateLocalized(path);
	ctext = XmCvtXmStringToCT(cstring);
	passtext = (char *)XtMalloc(strlen(ctext)+1);
	memcpy(passtext, ctext, strlen(ctext)+1);

	/* format the value for transfer, convert the value from
	 * compund string to compund text for the transfer */

	*type = net->common.COMPOUND_TEXT;
	*value = (XtPointer)passtext;
	*length = strlen(passtext);
	} else { /* FILE_NAME target */
	passtext = (char *)XtMalloc(strlen(path)+1);
	memcpy(passtext, path, strlen(path)+1);
	*type = XA_STRING;
	}

	*value = (XtPointer)passtext;
	*length = strlen(passtext);
	*format = 8;

	return(TRUE);
}

/* This routine is performed by the initiator when a drag 
 * starts. It starts the drag processing, and establishes a drag
 * context. 
 */
static void
startDrag(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	int		n, item;
	Atom		targets[2];
	char 		*conv;	
	Widget		drag_icon;
	Arg		args[10];
	char		*remoteAccessPath;
	char		*sysName;
	int		fileIndex, i;
	dnsListNum	cur_wid_pos;
	dnsList		*cur_pos;


	if ((item = XmListXYToPos((Widget)w,
			event->xbutton.x, event->xbutton.y)) <= 0 )
	{
		return;
	}

	if (net->common.cur_view == etcHost) {

		/* If the 1st column is selected, call my own function, 
		 * otherwise, call the default list action.
		 */

		if ((item % ETCCOLS) == 1) {
		/* get file path and pass it to the convert proc */
			fileIndex = ((item+(ETCCOLS-1))/ETCCOLS)-2;
			sysName = strdup(net->etc.etcHosts->list[fileIndex].etcHost.name);
			if (getRemoteAccessPath(net->common.toplevel, sysName, &remoteAccessPath) == FALSE) {
				/* error message will be printed by the function. */
				return;
			}
		}
		else {
			XtCallActionProc(
				(Widget)w, "ListProcessDrag",
						event, params, *num_params);
			return;
		}
	}
	else {
		/* In dns view. */
		fileIndex = item - 1;
                /* find the right pane that current drag is on. This pane
                 * may NOT be the same as the selected pane thru single/
                 * double clicks.
                 */
                for (i=0; i<=2; i++) {
                        if (w == net->dns.dnsArray[i]) {
                                cur_wid_pos = i;
                                break;
                        }
                }
                /* move the cur_pos to the dragged position */
                cur_pos = net->dns.cur_pos;
		for (i=0; i <  cur_wid_pos; i++) {
                        cur_pos = cur_pos->next;
                }
		if (isInDomainList(item, cur_pos) == TRUE)
		{
			return;
		}
		else {
			fileIndex = fileIndex - cur_pos->domainList.count;
			sysName = strdup(cur_pos->systemList.list[fileIndex].name);
		}
		if (getRemoteAccessPath(net->common.toplevel, 
			sysName, &remoteAccessPath) == FALSE) {
				return;
		}
	}

	targets[0] = net->common.COMPOUND_TEXT;
	targets[1] = OL_XA_FILE_NAME(XtDisplay(w));

	/* should use the RemoteAccess icon */

	n = 0;
	XtSetArg(args[n], XmNexportTargets, targets); n++;
	XtSetArg(args[n], XmNnumExportTargets, 2); n++;
	XtSetArg(args[n], XmNconvertProc, DragConvertProc); n++;
	XtSetArg(args[n], XmNclientData, remoteAccessPath); n++;
	XtSetArg(args[n], XmNdragOperations, XmDROP_LINK); n++;
	(void)XmDragStart(w, event, args, n);
}

static void
btn1Action(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	if (*num_params != 2 || !XmIsList(w))
		return;

	if (TestInSelection((XmListWidget)w, event))
		XtCallActionProc(
			w, params[CLICK_ACTION], event, params, *num_params);
	else
		XtCallActionProc(
			w, params[MOVE_ACTION], event, params, *num_params);
}

/* In dns view, insensitize the New, Delete and Domain Listing menu items.
 * Sensitize the Copy To Systems List and Systems List.
 * If the user has no Internet Setup privelge, insensitize the UUCP transfer,
 * DNS Access, NIS Access, Routing Setup and Copy to System List.
 */
void
dnsMenuBar()
{
	Boolean	isDomain;

	isDomain = isInDomainList(
		net->dns.dnsIndex[net->dns.cur_wid_pos],
		net->dns.cur_pos
	);
	/* New */
	XtVaSetValues(net->common.menu[1].subitems[0].handle,
		XmNsensitive, False, NULL);
	/* Delete */
	XtVaSetValues(net->common.menu[1].subitems[3].handle,
		XmNsensitive, False, NULL);
	/* Domain Listing */
	XtVaSetValues(net->common.menu[2].subitems[1].handle,
		XmNsensitive, False, NULL);
	/* Copy to System List */
	XtVaSetValues(net->common.menu[1].subitems[2].handle,
		XmNsensitive, isDomain ? False : True, NULL);
	/* System List */
	XtVaSetValues(net->common.menu[2].subitems[0].handle,
		XmNsensitive, True, NULL);
	/* Left */
/*
	XtVaSetValues(net->common.menu[2].subitems[2].handle,
		XmNsensitive, True, NULL);
*/
	/* Right */
/*
	XtVaSetValues(net->common.menu[2].subitems[3].handle,
		XmNsensitive, True, NULL);
*/
	if (net->common.isOwner == FALSE) {
		XtVaSetValues(net->common.menu[0].subitems[1].handle,
			XmNsensitive, False, NULL); /* UUCP */
		XtVaSetValues(net->common.menu[0].subitems[2].handle,
			XmNsensitive, False, NULL); /* DNS Access  */
		XtVaSetValues(net->common.menu[0].subitems[3].handle,
			XmNsensitive, False, NULL); /* NIS Access */
		XtVaSetValues(net->common.menu[0].subitems[4].handle,
			XmNsensitive, False, NULL); /* Routing Setup */
		XtVaSetValues(net->common.menu[1].subitems[2].handle,
			XmNsensitive, False, NULL); /* Copy to System List */
	}
	else {
		/* just want to make sure they are turn on */
		XtVaSetValues(net->common.menu[0].subitems[1].handle,
			XmNsensitive, isDomain ? False : True, NULL); /* UUCP */
		XtVaSetValues(net->common.menu[0].subitems[2].handle,
			XmNsensitive, True, NULL); /* DNS Access */
		XtVaSetValues(net->common.menu[0].subitems[3].handle,
			XmNsensitive, True, NULL); /* NIS Access */
		XtVaSetValues(net->common.menu[0].subitems[4].handle,
			XmNsensitive, True, NULL); /* Routing Setup */
	}
	if (isDomain) {
		XtSetSensitive(net->common.menu[1].subitems[1].handle, False);
		XtSetSensitive(net->common.menu[1].subitems[2].handle, False);
		XtSetSensitive(net->common.menu[1].subitems[5].handle, False);
	}
}

/* In etc view, insensitize the Copy To System List, Systems List.
 * Sensitize the New, Delete, Domain Listing.
 * If the user has no Internet Setup privelge, insensitize the UUCP transfer,
 * DNS Access, NIS Access, Routing Setup, New, and Delete.
 */
etcMenuBar()
{
	/* Copy to Folder */
	XtVaSetValues(net->common.menu[1].subitems[1].handle,
		XmNsensitive, True, NULL);
	/* Properties */
	XtVaSetValues(net->common.menu[1].subitems[5].handle,
		XmNsensitive, True, NULL);
	/* Copy to System List */
	XtVaSetValues(net->common.menu[1].subitems[2].handle,
		XmNsensitive, False, NULL);
	/* System List */
	XtVaSetValues(net->common.menu[2].subitems[0].handle,
		XmNsensitive, False, NULL);
	/* Left */
	XtVaSetValues(net->common.menu[2].subitems[2].handle,
		XmNsensitive, False, NULL);
	/* Right */
	XtVaSetValues(net->common.menu[2].subitems[3].handle,
		XmNsensitive, False, NULL);
	/* New */
	XtVaSetValues(net->common.menu[1].subitems[0].handle,
		XmNsensitive, True, NULL);
	/* Delete */
	XtVaSetValues(net->common.menu[1].subitems[3].handle,
		XmNsensitive, True, NULL);
	/* Domain Listing */
	if (net->common.isDnsConfigure != FALSE)
		XtVaSetValues(net->common.menu[2].subitems[1].handle,
			XmNsensitive, True, NULL);
	else
		XtVaSetValues(net->common.menu[2].subitems[1].handle,
			XmNsensitive, False, NULL);

	if (net->common.isOwner == FALSE) {
		XtVaSetValues(net->common.menu[0].subitems[1].handle,
			XmNsensitive, False, NULL); /* UUCP */
		XtVaSetValues(net->common.menu[0].subitems[2].handle,
			XmNsensitive, False, NULL); /* DNS Access */
		XtVaSetValues(net->common.menu[0].subitems[3].handle,
			XmNsensitive, False, NULL); /* NIS Access */
		XtVaSetValues(net->common.menu[0].subitems[4].handle,
			XmNsensitive, False, NULL); /* Routing Setup */
		XtVaSetValues(net->common.menu[1].subitems[0].handle,
			XmNsensitive, False, NULL); /* New */
		XtVaSetValues(net->common.menu[1].subitems[3].handle,
			XmNsensitive, False, NULL); /* Delete */
	}
	else {
		/* just want to make sure they are turn on */
		XtVaSetValues(net->common.menu[0].subitems[1].handle,
			XmNsensitive, True, NULL); /* UUCP */
		XtVaSetValues(net->common.menu[0].subitems[2].handle,
			XmNsensitive, True, NULL); /* DNS Access */
		XtVaSetValues(net->common.menu[0].subitems[3].handle,
			XmNsensitive, True, NULL); /* NIS Access */
		XtVaSetValues(net->common.menu[0].subitems[4].handle,
			XmNsensitive, True, NULL); /* Routing Setup */
		XtVaSetValues(net->common.menu[1].subitems[0].handle,
			XmNsensitive, True, NULL); /* New */
		XtVaSetValues(net->common.menu[1].subitems[3].handle,
			XmNsensitive, True, NULL); /* Delete */
	}
} 

addDnsItem(dnsInfo *dns, dnsList *new)
{
	dnsList *tmp;

	if(dns->cur_pos == NULL || dns->totalDnsItems == 0){
		dns->cur_pos = new;
		dns->totalDnsItems = 1;
	}else{
		for(tmp = dns->cur_pos; tmp->next != NULL; tmp = tmp->next);
		new->prev = tmp;
		tmp->next = new;
		dns->totalDnsItems++;
	}
}

printDnsList(dnsInfo *dns)
{
	dnsList *tmp;

	for(tmp = dns->cur_pos; tmp != NULL; tmp = tmp->next){
		printHostList(tmp->nameServerList);
		printHostList(tmp->domainList);
	}
}


void
unmapList(Widget w, Widget label)
{
	XmListDeleteAllItems(w);
	setSLabel(label, " ");
}

void
updateETC()
{
	if (net->etc.etcRC) {
		net->common.cur_view = etcHost;
		XtManageChild(net->etc.etcRC);
		/* BUG: should use the other function call */
		makePosVisible(net->etc.etcList, net->etc.etcHostIndex);
		XtPopup(net->common.toplevel, XtGrabNone);
	}
	else {
		/* BUG: need to exit thru the OK button on the popup */
		createMsg(net->common.toplevel, ERROR, mygettxt(ERR_noSetup),
				mygettxt(TXT_HostTitle));
	}
}

void
updateDNS()
{
	dnsList	*tmp;
	int	i;

	switch(net->dns.queryFrom) {
	case START:
		net->common.cur_view = DNS;
		net->dns.totalDnsItems = 0;
		net->dns.cur_wid_pos = DNSL1;
		net->dns.cur_pos = NULL;
		addDnsItem(&net->dns, net->dns.dnsHosts);
		net->dns.work_pos = net->dns.cur_pos;	
		mappLists(net->dns.cur_pos, 1, DNSL1);
		updateArrowBut();
		break;
	case SHOWDOMAIN:
		for (tmp = net->dns.cur_pos; tmp && tmp->prev != NULL; tmp = tmp->prev);
		pruneDnsList(&tmp, &net->dns.totalDnsItems);
		/* after prune the tree, set the cur_pos to NULL */
		net->dns.cur_pos = NULL;
		addDnsItem(&net->dns, net->dns.dnsHosts);
		net->dns.work_pos = net->dns.cur_pos;
		mappLists(net->dns.cur_pos, 1, DNSL1);
		unmapList(net->dns.dnsArray[DNSL2], net->dns.listLabel[DNSL2]);
		unmapList(net->dns.dnsArray[DNSL3], net->dns.listLabel[DNSL3]);
		updateArrowBut();
		break;
	case DOUBLECLICK:
		/* plume the tree starting from the one after cur_wid_pos */
		tmp = net->dns.cur_pos;
		for (i = 0; i < net->dns.cur_wid_pos; i++)
			tmp = tmp->next;
		pruneDnsList(&tmp->next, &net->dns.totalDnsItems);
		/* always add to the end of the link list */
		addDnsItem(&net->dns, net->dns.dnsHosts);	
		/* mapp the panes */
		switch(net->dns.cur_wid_pos) {
			case DNSL1:	
				net->dns.work_pos = tmp->next;
				mappLists(tmp->next, 1, DNSL2);
				unmapList(net->dns.dnsArray[DNSL3],
					net->dns.listLabel[DNSL3]);
				break;
			case DNSL2:
				net->dns.work_pos = tmp->next;
				mappLists(tmp->next, 1, DNSL3);
				break;
			case DNSL3:
				net->dns.cur_pos = net->dns.cur_pos->next;
				net->dns.work_pos = net->dns.cur_pos;
				mappLists(net->dns.cur_pos, 3, DNSL1);
				break;
			default:
				createMsg(net->common.toplevel, ERROR, ERR_unkPanePos, mygettxt(TXT_HostTitle));
				break;
		}
		updateArrowBut();
		break;
	default:
		createMsg(net->common.toplevel, ERROR, ERR_unkQuery, 
			mygettxt(TXT_HostTitle));
		break;
	}
	if (isInDomainList(net->dns.dnsIndex[net->dns.cur_wid_pos], net->dns.cur_pos)) {
		XtSetSensitive(net->common.menu[1].subitems[1].handle, False);
		XtSetSensitive(net->common.menu[1].subitems[2].handle, False);
		XtSetSensitive(net->common.menu[1].subitems[5].handle, False);
		XtSetSensitive(net->common.menu[0].subitems[1].handle, False);
	}
}

void 
mappLists(dnsList * pos, int num, dnsListNum wid_pos)
{
	int	i;
	char	*domain, *ptr, *strpbrk();

	if (pos == NULL)
		return;
	for (i=0; i < num; i++) {
		domain = strdup(pos->domain.name);
		ptr = strpbrk(domain, ".");
		if (ptr != NULL)
			*ptr = '\0';

		setSLabel(net->dns.listLabel[wid_pos], domain);
		setDnsList(pos, wid_pos++);
		pos = pos->next;
	}
}
void 
updateArrowBut()
{
		
	if (net->dns.cur_pos == NULL || (net->dns.cur_pos->prev == NULL 
		&& net->dns.totalDnsItems <= 3)) {
		XtVaSetValues(net->dns.leftArrow, XmNsensitive, FALSE, NULL);
		XtVaSetValues(net->dns.rightArrow, XmNsensitive, FALSE, NULL);
		XtVaSetValues(net->common.menu[2].subitems[2].handle,
			XmNsensitive, FALSE, NULL);
		XtVaSetValues(net->common.menu[2].subitems[3].handle,
			XmNsensitive, FALSE, NULL);
	}
	else {
		if (net->dns.cur_pos->prev == NULL) {
			XtVaSetValues(net->dns.leftArrow, 
				XmNsensitive, FALSE, NULL);
			XtVaSetValues(net->common.menu[2].subitems[2].handle,
				XmNsensitive, FALSE, NULL);
		}
		else {
			XtVaSetValues(net->dns.leftArrow, 
				XmNsensitive, TRUE, NULL);
			XtVaSetValues(net->common.menu[2].subitems[2].handle,
				XmNsensitive, TRUE, NULL);
		}

		if (net->dns.cur_pos->next && net->dns.cur_pos->next->next &&
			net->dns.cur_pos->next->next->next) {

			XtVaSetValues(net->dns.rightArrow, 
				XmNsensitive, TRUE, NULL);
			XtVaSetValues(net->common.menu[2].subitems[3].handle,
				XmNsensitive, TRUE, NULL);
		}
		else {
			XtVaSetValues(net->dns.rightArrow, 
				XmNsensitive, FALSE, NULL);
			XtVaSetValues(net->common.menu[2].subitems[3].handle,
				XmNsensitive, FALSE, NULL);
		}
	}
}

void
leftArrowCB()
{
	/* the left arrow must be sensitized when this function is called */
	net->dns.cur_pos = net->dns.cur_pos->prev;
	net->dns.work_pos = net->dns.cur_pos;
	mappLists(net->dns.cur_pos, 3, DNSL1);
	updateArrowBut();
}

void
rightArrowCB()
{
	/* the right arrow button must be sensitizied when this is called */
	net->dns.cur_pos = net->dns.cur_pos->next;
	net->dns.work_pos = net->dns.cur_pos;
	mappLists(net->dns.cur_pos, 3, DNSL1);
	updateArrowBut();
}

void 
okCB()
{
	String	select;
	char	*strchr();
	char	fullname[MEDIUMBUFSIZE];

	/* only for non-inet client: get the selection, update text widget 
	 * and then finally popdown
	 */

	if (net->common.cur_view == DNS) {
		if (strchr(net->dns.dnsSelection[net->dns.cur_wid_pos], '.')
			== NULL) {
			sprintf(fullname, "%s.%s", 
				net->dns.dnsSelection[net->dns.cur_wid_pos],
				net->dns.work_pos[0].domain.name);
		}
		else {
			sprintf(fullname, "%s", 
				net->dns.dnsSelection[net->dns.cur_wid_pos]);
		}

		XtVaSetValues(net->lookup.clientText, 
			XmNvalue, fullname, NULL);
	}
	else
		XtVaSetValues(net->lookup.clientText,
			XmNvalue, net->etc.etcSelection, NULL);
	XtPopdown(net->common.toplevel);
}

void
cancelCB()
{
	/* Only for non-inet client: popdown */
	XtPopdown(net->common.toplevel);
}

void
helpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	/* Only for non-inet client: help button */
	racDisplayHelp(w, (HelpText *)client_data);
}

void
showCB(Widget w, int client_data, XmAnyCallbackStruct *cbs)
{
	readHostsReturn	ret;
	String		domain;
	static char *	oldDomain = NULL;

	if (oldDomain) {
		free(oldDomain);
	}
	oldDomain = strdup(net->dns.dnsHosts->domain.name);
	net->dns.queryFrom = SHOWDOMAIN;
	XtVaGetValues(net->dns.domainText, XmNvalue, &domain, NULL);
	
	/* busyCursor(XtParent(net->dns.dnsRC), TRUE); */
	if ((ret = readDnsHosts(domain)) == Failure) {
		/* readDnsHosts will display error */;
		XtVaSetValues(
			net->dns.domainText,
			XmNvalue,	oldDomain,
			NULL
		);
	}
	free(domain);
}

Boolean
setDnsSelection(dnsListNum num, int list_pos)
{
	int	i;
	dnsList	*cur_pos;
	Boolean	isDomain;

	/* move the cur_pos to the selected widget list,
	 * update the dnsIndex and dnsSelection.
	 */

	net->dns.cur_wid_pos = num;
	cur_pos = net->dns.cur_pos;
	for (i=0; i < num; i++) {
		cur_pos = cur_pos->next;
	}
	net->dns.dnsIndex[num] = list_pos;
	if (isInDomainList(net->dns.dnsIndex[num], cur_pos) == TRUE) {
		net->dns.dnsSelection[num] = 
			strdup(cur_pos->domainList.list[net->dns.dnsIndex[num]-1].name);
		isDomain = TRUE;
	}
	else {
		/* it is a host, just update the dnsSelection */
		net->dns.dnsSelection[num] = 
			strdup(cur_pos->systemList.list[net->dns.dnsIndex[num]-cur_pos->domainList.count-1].name);
			
		isDomain = FALSE;
	}	
	return(isDomain);
}

/*	Single click on dns list
 *
 *                      inet                    non-inet
 *      --------------------------------------------------------------
 *              |                       |
 *      domain  | - update prop popup   | - update selection label
 *              | - update selection    | - update selection index
 *              |   index/string        |   and string
 *      --------------------------------------------------------------
 *              |                       |
 *      host    | - update prop popup   | - update selection label
 *              | - update selection    | - update selection index
 *              |   index/string        |   and string
 *      --------------------------------------------------------------
 */

void
dnsListBrowseCB(Widget w, dnsListNum wid_num, XmListCallbackStruct *cbs )
{
	int     i;
	dnsList *cur_pos;
	readHostsReturn ret;
	Boolean isDomain;

	/* update the dnsIndex and dnsSelection */
	isDomain = setDnsSelection(wid_num, cbs->item_position);
	
	if (net->common.isInet == TRUE) {
		XtSetSensitive(
			net->common.menu[1].subitems[1].handle,
			isDomain ? False : True
		);
		XtSetSensitive(
			net->common.menu[1].subitems[2].handle,
			isDomain ? False : True
		);
		XtSetSensitive(
			net->common.menu[1].subitems[5].handle,
			isDomain ? False : True
		);
		XtSetSensitive(
			net->common.menu[0].subitems[1].handle,
			isDomain ? False : True
		);
		/* inet: call special function to update prop popup  */
		if (net->common.isProp == TRUE) 
			net->dns.clickCB(w, NULL, NULL);
	}
	else {
		/* non-inet: update selection label */
		setLabel(net->lookup.selectLabel,
			net->dns.dnsSelection[wid_num]);
	}
}

/*	Double click on the dns list
 *
 *                      inet                    non-inet
 *      --------------------------------------------------------------
 *              |                       |
 *      domain  | - query and update    | - query and update 
 *              |   windows             |   windows
 *              | - high-light the item | - high-light the item
 *              |   on the new list     |   on the new list
 *              | - update selection    | - update selection label
 *              |   index/string        | - update selection index/str
 *      --------------------------------------------------------------
 *              |                       |
 *      host    | - update prop popup   | - update selection label
 *              | - update selection    | - update textfield
 *              |   index/string        | - update selection index/str
 *		|			| - popdown
 *      --------------------------------------------------------------
 */

void
dnsListDefCB(Widget w, dnsListNum wid_num, XmListCallbackStruct *cbs )
{
	readHostsReturn ret;
	Boolean 	isDomain;
	char		fullname[MEDIUMBUFSIZE];
	char		*strchr();
	int		i;	
	dnsList		*cur_pos;

	/* update the dnsIndex and dnsSelection */
	isDomain = setDnsSelection(wid_num, cbs->item_position);

	if (isDomain == TRUE) {
		net->dns.queryFrom = DOUBLECLICK;
		/* busyCursor(XtParent(net->dns.dnsRC), TRUE); */
		if ((ret = readDnsHosts(net->dns.dnsSelection[wid_num])) 
			== Failure)
			return; 
		if (net->common.isInet == FALSE) {
			setLabel(net->lookup.selectLabel,
				net->dns.dnsSelection[wid_num]);
		}
	}
	else {
		/* host */
                cur_pos = net->dns.cur_pos;
                for (i=0; i<wid_num; i++) {
                        cur_pos = cur_pos->next;
                }
		if (net->common.isInet == FALSE) {
			/* non-inet client: update the selection */
			setLabel(net->lookup.selectLabel,
				net->dns.dnsSelection[wid_num]);
			if (strchr(net->dns.dnsSelection[wid_num],
				'.') == NULL) {
				sprintf(fullname, "%s.%s",
					net->dns.dnsSelection[wid_num],
					cur_pos->domain.name);
			}
			else 
				sprintf(fullname, "%s",
					net->dns.dnsSelection[wid_num]);

			XtVaSetValues(net->lookup.clientText, XmNvalue,
				fullname, NULL);
			XtPopdown(net->common.toplevel);
		}
		else {
			/* inet client: call func to popup/update the prop */
			net->dns.clickCB(w, NULL, NULL);
		}
	}
}

/*	Single click on etcHost list:
 *
 *	                inet                    non-inet
 *      --------------------------------------------------------------
 *	- update selection index/str	| - update selection index/str
 *	- update prop/new popup		| - update selection label 
 *      --------------------------------------------------------------
 */

void
etcListBrowseCB(Widget w, XtPointer client_data, XmListCallbackStruct*cbs)
{
	int	rowNum, index;
	/* update selection index and str */
	net->etc.etcHostIndex = cbs->item_position;
	rowNum = (net->etc.etcHostIndex + (ETCCOLS -1)) / ETCCOLS;
	index = rowNum - 2;
	net->etc.etcSelection = 
			strdup(net->etc.etcHosts->list[index].etcHost.name);
	if (net->common.isInet == FALSE) {
		/* non-inet: update the selection label */
		setLabel(net->lookup.selectLabel, net->etc.etcSelection);
	}
	else {
		/* inet client: call the specific function to update
		 * the prop popup.  For the new, just change the index.
		 */
		if (net->common.isProp == True)
			net->etc.clickCB(w, NULL, NULL);
	}
}

/*	Double click on etcHost list :
 *
 *	                inet                    non-inet
 *      --------------------------------------------------------------
 *	- update selection index/str	| - update selection index/str
 *	- update prop/new popup		| - update selection label 
 *					| - update text field
 *					| - popdown
 *      --------------------------------------------------------------
 */

void
etcListDefCB(Widget w, XtPointer client_data, XmListCallbackStruct*cbs)
{
	int	rowNum, index;
	/* update selection index and str */
	net->etc.etcHostIndex = cbs->item_position;
	rowNum = (net->etc.etcHostIndex + (ETCCOLS -1)) / ETCCOLS;
	index = rowNum - 2;
	net->etc.etcSelection = 
			strdup(net->etc.etcHosts->list[index].etcHost.name);
	if (net->common.isInet == FALSE) {
		/* non-inet: update the text widget and popdown */
		XtVaSetValues(net->lookup.clientText, XmNvalue,
			net->etc.etcSelection, NULL);
		XtPopdown(net->common.toplevel);
	}
	else {
		net->etc.clickCB(w, NULL, NULL);
	}	
}

void 
busyCursor(Widget w, int on)
{
	static Cursor cursor;
	XSetWindowAttributes attrs;
	Display *dpy = XtDisplay(w);
	Widget      shell = w;

	while(!XtIsShell(shell))
		shell = XtParent(shell);

	/* make sure the timeout cursor is initialized */
	if (!cursor) 
		cursor = XCreateFontCursor(dpy, XC_watch);

	attrs.cursor = on? cursor : None;
	XChangeWindowAttributes(dpy, XtWindow(shell), CWCursor, &attrs);

	XFlush(dpy);

}

void   
setDnsList(dnsList *dns, dnsListNum wid_pos)
{
	XmStringTable    str_list;
	int i, total;

		total = dns->domainList.count + dns->systemList.count;
		str_list = (XmStringTable)XtMalloc(total * sizeof (XmString *));
		for (i = 0; i < total; i++){
			str_list[i] = NULL;
		}
		XtVaSetValues(net->dns.dnsArray[wid_pos],
			XmNitems, str_list,
			XmNitemCount, total,
			NULL);
		if (str_list) XtFree((char *)str_list);
		XtManageChild(net->dns.dnsArray[wid_pos]);
		XmListSelectPos(net->dns.dnsArray[wid_pos], 1, True);
}

void
dnsInitProc(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmListItemInitCallbackStruct	*cd=(XmListItemInitCallbackStruct *)call_data;
	char	buff[MEDIUMBUFSIZE];
	int	rowNum, fileIndex, domainCount, i, wid_pos;
	dnsList	*cur_pos;
	
	if (cd->reason != XmCR_ASK_FOR_ITEM_DATA)
		return;
	if (d_glyph == None) 
		d_glyph = DmGetPixmap(XtScreen(net->common.toplevel), "domain16");
	if (h_glyph == None)
		h_glyph = DmGetPixmap(XtScreen(net->common.toplevel), "host16");
	fileIndex = cd->position - 1;
	/* find the current widget position */
	for (i = 0 ; i <= 2; i ++) {
		if (net->dns.dnsArray[i] == w) {
			wid_pos = i;
			break;
		}	
	}
	/* find the dnsList position in the link-list */
	cur_pos = net->dns.cur_pos;
	for (i = 0; i < wid_pos; i++) 
		cur_pos = cur_pos->next;		
		
	domainCount=cur_pos->domainList.count;
	if (fileIndex < cur_pos->domainList.count) {
		sprintf(buff, "%s", cur_pos->domainList.list[fileIndex].name);
		cd->pixmap = d_glyph->pix;
		cd->mask = d_glyph->mask;
		cd->depth = d_glyph->depth;
		cd->width = d_glyph->width;
		cd->height = d_glyph->height;
	}
	else {
		sprintf(buff, "%s", cur_pos->systemList.list[fileIndex-domainCount].name);
		cd->pixmap = h_glyph->pix;
		cd->mask = h_glyph->mask;
		cd->depth = h_glyph->depth;
		cd->width = h_glyph->width;
		cd->height = h_glyph->height;
	}
	cd->label = XmStringCreate(buff, XmFONTLIST_DEFAULT_TAG);
	cd->h_pad = 5;
	cd->v_pad = 0;
	cd->glyph_pos = XmGLYPH_ON_LEFT;
	cd->static_data = True;
}

void
itemInitProc(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmListItemInitCallbackStruct	*cd=(XmListItemInitCallbackStruct *)call_data;
	char			buff[MEDIUMBUFSIZE];
	int			rowNum, fileIndex;

	if (cd->reason != XmCR_ASK_FOR_ITEM_DATA)
		return;
	if (h_glyph == NULL)
		h_glyph = DmGetPixmap(XtScreen(net->common.toplevel), "host16");
	rowNum = (cd->position + (ETCCOLS -1))/ ETCCOLS;
	/* substract 2: one for the title row, one for the offset into array */
	fileIndex = rowNum - 2;
	sprintf(buff, "%s", net->etc.etcHosts->list[fileIndex].etcHost.name);	
	cd->label = XmStringCreate(buff, XmFONTLIST_DEFAULT_TAG);
	cd->pixmap = h_glyph->pix;
	cd->mask = h_glyph->mask;
	cd->depth = h_glyph->depth;
	cd->width = h_glyph->width;
	cd->height = h_glyph->height;

	cd->h_pad = 5;
	cd->v_pad = 0;
	cd->glyph_pos = XmGLYPH_ON_LEFT;
	cd->static_data = True;
}

void   
setEtcList(Widget w,etcHostList *list, readHostsReturn retval)
{
	XmStringTable    str_list;
	int i, j=0;
	static int first=True;
	char buf[MEDIUMBUFSIZE];
	/* BUG: what is the numbers 30 and 32 mean?? */
	char taddr[30];
	char tcomment[32];

	if (first || retval == NewList){
		str_list = (XmStringTable)XtMalloc((list->count+1) * sizeof (XmString *) * ETCCOLS);
		/* for the title: j=XmList element, i=etcHostList element */
		str_list[j] = i18nString(TXT_etcName); j++;
		str_list[j] = i18nString(TXT_etcAddr); j++;
		str_list[j] = i18nString(TXT_etcComment); j++;
		for (i = 0; i < list->count; i++){
			sprintf(taddr, "%d.%d.%d.%d", 
				list->list[i].etcHost.addr[0],
				list->list[i].etcHost.addr[1],
				list->list[i].etcHost.addr[2],
				list->list[i].etcHost.addr[3]);
			strncpy(tcomment, list->list[i].comment, 30);
			str_list[j] = NULL;
			j++;
			str_list[j] = XmStringCreateLocalized(taddr);
			j++;
			str_list[j] = XmStringCreateLocalized(tcomment);
			j++;
		}

		XtVaSetValues(w, 
			XmNitems, str_list,
			XmNitemCount, j,
			NULL);
		for (i = 0; i < j; i++) {
			if (str_list[i]) {
				XmStringFree(str_list[i]);
				str_list[i] = NULL;
			}
		}
		XtFree((char *)str_list);
		XtManageChild(w);
		XmListSelectPos(net->etc.etcList, 4, True);
		/* BUG?? set the index in the first time ?? should be
		 * done in XmListSelectPos()??	*/
		net->etc.etcHostIndex = 4;
		net->etc.etcSelection =
			strdup(net->etc.etcHosts->list[0].etcHost.name);
		first = False;
	}
}

extern XtAppContext app;

/* TimeoutCursors() turns on the "watch" cursor over the 
 * application to provide feedback for the user that she's
 * going to be waiting a while before she can interact with
 * the application again.
 */

void
TimeoutCursors(on, interruptable)
int on, interruptable;
{
	static int	locked;
	static Cursor	cursor;
	Widget 		topShell; 
	XSetWindowAttributes attrs;
	Display *dpy;
	XEvent event;
	Arg	args[1];
	XmString	str;
	char	buf[MEDIUMBUFSIZE];

	extern void stop();
	/* "locked" keeps track if we've already called
	 * the function.  This allows recursion and is
	 * necessary for most situations.
	 */

	on ? locked++:locked--;
	if (locked > 1 || locked == 1 && on == 0)
		/* already locaked and we're not unlocking */
		return;

	/* doesn't matter at this point; initialize */
	stopped = False;

	if (net->common.isFirst) {
		/* For inet, toplevel shell is not mapped yet, so do nothing.
		 * For non-inet, first time busycursor is on the parent
		 */
		if (net->common.isInet == FALSE) {
			topShell = XtParent(net->lookup.clientText);
			dpy = XtDisplay(topShell);
			/* make sure the timeout cursor is initialize */
			if (!cursor)
				cursor = XCreateFontCursor(dpy, XC_watch);

			/* if "on" is true, then turn on watch cursor,
			 * otherwise, return the shell's cursor to normal.
			 */
			attrs.cursor = on ? cursor:None;

			/* change the main application shell's cursor to be
			 * the timeout cursor (or to reset it to normal).
			 */
			XChangeWindowAttributes(dpy, XtWindow(topShell),
				CWCursor, &attrs);

		}
		else {
			topShell = net->common.toplevel;
			dpy = XtDisplay(topShell);
		}	
	}
	else  {
		topShell = net->common.toplevel;
		dpy = XtDisplay(topShell);
		if (!cursor)
			cursor = XCreateFontCursor(dpy, XC_watch);
		attrs.cursor = on ? cursor:None;

		XChangeWindowAttributes(dpy, XtWindow(topShell),
			CWCursor, &attrs);

	}
	XFlush(dpy);

	if (on) {
		Widget w;

		/* We're timing out, use a PromptDialog to display the
		 * fact that we're busy.  If the processor is interruptable, 
		 * allow a "Stop" button. Else, remove all actions so the 
		 * user can't stop the processing.
		 */
		/*sprintf(buf, INFO_dnsRunning2, net->dns.dnsHosts->domain);
		str = XmStringCreateLtoR(mygettxt(buf), XmFONTLIST_DEFAULT_TAG);
		*/
		sprintf(buf, mygettxt(INFO_dnsRunning2), net->dns.dnsHosts->domain);
		str = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
		XtSetArg(args[0], XmNselectionLabelString, str);
		dialog = XmCreatePromptDialog(net->common.toplevel, mygettxt(TXT_inProgress), args, 1);	
		XmStringFree(str);
		str = XmStringCreateLocalized(mygettxt(TXT_inProgress));
		XtVaSetValues(dialog, XmNdialogTitle, str, NULL);
		XmStringFree(str);
		XtUnmanageChild(
			XmSelectionBoxGetChild(dialog, XmDIALOG_OK_BUTTON));
		XtUnmanageChild(
			XmSelectionBoxGetChild(dialog,XmDIALOG_TEXT));
		if (interruptable) {
			str = XmStringCreateLocalized(mygettxt(TXT_Stop));
			XtVaSetValues(dialog, XmNcancelLabelString, str, NULL);
			XmStringFree(str);
			XtAddCallback(dialog, XmNcancelCallback, stop, NULL);
		}
		else
			XtUnmanageChild(
				XmSelectionBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
		XtUnmanageChild(
			XmSelectionBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));	
		XtManageChild(dialog);
		XmUpdateDisplay(dialog);
	}
	else { /* !on */
		/* We're turning off the timeout-cursors.
		 * Get rid of all button and keyboard events that occurred
		 * during the timeout.  The user shouldn't have done 
		 * anything during this time, so flush out button and 
		 * keypress events.
		 */
		if (net->common.isFirst == TRUE && net->common.isInet == FALSE) {
			while(XCheckMaskEvent(dpy,
				ButtonPressMask|ButtonReleaseMask|ButtonMotionMask
				| PointerMotionMask | KeyPressMask, &event)) {
				/* do nothing */;
			}
		}
		XtDestroyWidget(dialog);
	}
}

/* User pressed the "stop" button in disalog;
 * set global "stopped" value.
 */

void
stop(dialog)
Widget dialog;
{
	stopped = True;
}

Boolean
CheckForInterrupt()
{
	/* extern Widget shell; */
	Display *dpy = XtDisplay(net->common.toplevel);
	Window win = XtWindow(dialog);
	XEvent event;

	/* Make sure all our requests get to the server */
	XFlush(dpy);
	/* Let motif process all pending exposure events for us */
	XmUpdateDisplay(net->common.toplevel);

	/* Check the event loop for events in the dialog (Stop?) */
	while(XCheckMaskEvent(dpy,
		ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
		PointerMotionMask|KeyPressMask|KeyReleaseMask, &event)) {
		/* got an interrupt event */
		if (event.xany.window == win) 
			XtDispatchEvent(&event);
		else
			/* uninteresting event - throw it away */;

	}
	/* If an "interesting" event took place
	 * (i.e., the user pressed the stop button),
	 * then the stop() callback would have been invoked,
	 * setting the "stopped" value to 1. return that
	 * value here.
	 */
	return stopped;
}

readHostsReturn 
readDnsHosts(char *domain)
{
	dnsList	*dnsHosts1;
	int	n, total = 20;
	char	*nsList[20];
	int	nsNum = 0, ns_error, ho_error;
	XmString	str;
	Arg	args[1];
	char	buf[MEDIUMBUFSIZE];
	queryReturn	query=FAILED;

	initDnsList(&dnsHosts1);
	net->dns.dnsHosts = dnsHosts1;
	net->dns.dnsHosts->domain.name = strdup(domain);

	/* In this routine, we set busy cursor on the main woindow,
	 * and popup a message box to tell user we are busy on getting
	 * info from tcpip thru the resolv lib.
	 * We allow user to stop the process ONLY after the completion of
	 * the query on each Name Server.  Note: During the query, user
	 * cannot interact with the gui at all since the GUI is locked
	 * by the query process.
	 */

	TimeoutCursors(True, True);
	CheckForInterrupt();

	XmUpdateDisplay(net->common.toplevel);
	net->dns.query = NAME_SERVERS;
	if ((ns_error = findNameServers(domain, &net->dns.dnsHosts->nameServerList)) == 0 ) { 
		/* successful query */
		net->dns.query = HOSTS;
		for (n=0; n < net->dns.dnsHosts->nameServerList.count; n++) {
			if (CheckForInterrupt()) {
				query = INTERRUPTED;	
				break;
			}
			if ((ho_error = queryNameServers(domain,
				net->dns.dnsHosts->nameServerList.list[n],
				&net->dns.dnsHosts->domainList,
				&net->dns.dnsHosts->systemList)) == 0) {
					query = OK;
					break;
			}
		}
	}
	else {
		/* failed to find the Name server */
		query = FAILED;
	}

	CheckForInterrupt();
	TimeoutCursors(False, False);

	switch (query) {
		case OK:
			/* everthing fine here, go update GUI */
			if (net->common.isInet == TRUE)
				dnsMenuBar();
			updateDNS();
			XtManageChild(net->dns.dnsRC);
			if (net->common.isInet == TRUE)
				XtPopup(net->common.toplevel, XtGrabNone);
			if (net->common.isFirst)
				net->common.isFirst = FALSE;
			break;
		case INTERRUPTED:
			/* query was interrupted by user pressing "stop" */
			createMsg(net->common.toplevel, ERROR, ERR_killBySignal,
				mygettxt(TXT_HostTitle));
			return Failure;
		case FAILED:
			/* FALL THRU */
		default:			
			/* Error had occurred: For the 1st time, we need to map
			 * the etcList if dns query failed. Also, sensitize the
			 * menu bar items appropriately.
			 * From the 2nd times on, we need to display error 
			 * messages.
			 */
			if (net->common.isFirst) {
				updateETC();
				if (net->common.isInet == TRUE)
					etcMenuBar();
				net->common.isFirst = FALSE;
			}
			else {
				if (net->dns.query == NAME_SERVERS) {
					switch (ns_error) {
					case 1:
						sprintf(buf, mygettxt(ERR_MEMORY1), domain);
						break;
					case 2:
						sprintf(buf, mygettxt(ERR_EXPAND), domain);
						break;
					case 3:
						sprintf(buf, mygettxt(ERR_NO_ADDR), domain);
						break;
					case 10:
						sprintf(buf, mygettxt(ERR_HOST_NOT_FOUND1), domain);
						break;
					case 11:
						sprintf(buf, mygettxt(ERR_NO_DATA), domain);
						break;
					case 12:
						sprintf(buf, mygettxt(ERR_TRY_AGAIN), domain);
						break;
					case 13:
					default:
						sprintf(buf, mygettxt(ERR_UNEXPECTED_ERR), domain);
						break;
					}
					createMsg(net->common.toplevel, ERROR, buf,
						mygettxt(TXT_HostTitle));
				}
				else {
					switch (ho_error) {
					case 1:
						sprintf(buf, mygettxt(ERR_MEMORY1), domain);
						break;
					case 4:
						sprintf(buf, mygettxt(ERR_MKQUERY), domain);
						break;
					case 5: 
						sprintf(buf, mygettxt(ERR_NO_NS_RUNNING), domain);
						break;
					case 6:
						sprintf(buf, mygettxt(ERR_NO_SOC_CONN1), domain);
						break;
					case 7:
						sprintf(buf, mygettxt(ERR_CANNOT_WRITE_SOCKET1), domain);
						break;
					case 8:
						sprintf(buf, mygettxt(ERR_CANNOT_READ_SOCKET1), domain);
						break;
					case 9:
						sprintf(buf, mygettxt(ERR_DN_SKIPNAME), domain);
						break;
					case 14:
						sprintf(buf, mygettxt(ERR_FORMAT), domain);
						break;
					case 15:
						sprintf(buf, mygettxt(ERR_SERVER), domain);
						break;
					case 16:
						sprintf(buf, mygettxt(ERR_NOT_EXISTED1), domain);
						break;
					case 17:
						sprintf(buf, mygettxt(ERR_NOT_IMP), domain);
						break;
					case 18:
						sprintf(buf, mygettxt(ERR_REFUSED1), domain);
						break;
					case 19:
						sprintf(buf, mygettxt(ERR_NO_ANSWER1), domain);
						break;
					case 20:
						sprintf(buf, mygettxt(ERR_NO_SOC_CRT1), domain);
						break;
					case 13:
					default:
						sprintf(buf, mygettxt(ERR_UNEXPECTED_ERR), domain);
						break;
					}
					createMsg(net->common.toplevel, ERROR, buf, mygettxt(TXT_HostTitle));
				}
			}
			return Failure;
	}
	return NewList;
}

void
optCB(Widget menu_item, int item_no, XmAnyCallbackStruct *cbs) 
{
	int	ret;
	XtWidgetGeometry	size;

	if (item_no == 0) {
		net->common.cur_view = DNS;
		/* DNS list, when this function is called, DNS should be here */
		/* need to resize the action area here */
/* BUG: dwd - can you take a look at this?
		XtQueryGeometry(net->lookup.actions[1].widget, NULL, &size);
		resizeActionArea(XtParent(net->dns.domainText),
			net->lookup.actions, 3, size.width);	
*/
		XtManageChild(net->dns.dnsRC);
		/* check for etcHosts existence */
		if (net->etc.etcRC != NULL)
			XtUnmanageChild(net->etc.etcRC);	
	}
	else {
		/* System list */
		net->common.cur_view = etcHost;
		if ((ret = readEtcHosts(net->etc.etcHosts)) == Failure)
			return;
		setEtcList(net->etc.etcList, net->etc.etcHosts, ret);
		/* check for dnsRC existence */
		if (net->dns.dnsRC != NULL)
			XtUnmanageChild(net->dns.dnsRC);
		XtManageChild(net->etc.etcRC);
	}
}

Widget 
createEtcHosts(netInfo *net)
{
	Widget label, hbar, sw, form1;
	char   heading[MEDIUMBUFSIZE];
	readHostsReturn ret;
	Arg	args[10];
	int	i;
	XtCallbackRec	item_init_cb[2];

	initEtcHostList(&(net->etc.etcHosts));
	if((ret = readEtcHosts(net->etc.etcHosts)) == Failure) {
		if (net->etc.etcHosts)
			freeEtcList(net->etc.etcHosts);
		return(NULL);
	}

	if (net->etc.etcRC == NULL) {
		if (net->common.isInet == TRUE) {
			net->etc.etcRC = XtVaCreateWidget("etcHosts", 
					xmFormWidgetClass, net->common.topRC, 
					NULL);
			XtVaSetValues(net->etc.etcRC,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, net->common.menubar,
				XmNleftAttachment, XmATTACH_FORM,
				XmNrightAttachment, XmATTACH_FORM,
				XmNbottomAttachment, XmATTACH_FORM,
				NULL);
		}
		else {
			net->etc.etcRC = XtVaCreateWidget("etcHosts", 
					xmRowColumnWidgetClass, 
					net->common.topRC, 
					NULL);
		}

		label = XtVaCreateManagedWidget("label", 
					xmLabelWidgetClass, net->etc.etcRC, 	
					XmNalignment, XmALIGNMENT_CENTER,
					NULL);

		if (net->common.isInet == TRUE)
			XtVaSetValues(label,
				XmNtopAttachment, XmATTACH_FORM,
				XmNleftAttachment, XmATTACH_FORM,
				NULL);

		setLabel(label, TXT_etcLabel2);

		item_init_cb[0].callback = itemInitProc;
		item_init_cb[0].closure = NULL;
		item_init_cb[1].callback = NULL;
		item_init_cb[1].closure = NULL;	
		i=0;
		XtSetArg(args[i], XmNvisibleItemCount, VISIBLE_ITEMS);i++;
		XtSetArg(args[i], XmNnumColumns, ETCCOLS);i++;
		XtSetArg(args[i], XmNstaticRowCount, 1); i++;
		XtSetArg(args[i], XmNlistColumnSpacing, 5); i++;
		XtSetArg(args[i], XmNlistSizePolicy, XmCONSTANT); i++;
		XtSetArg(args[i], XmNitemInitCallback, item_init_cb); i++;
		XtSetArg(args[i], XmNscrollBarDisplayPolicy, XmAS_NEEDED); i++;

		sw = XtVaCreateManagedWidget("sw",
			xmScrolledWindowWidgetClass, net->etc.etcRC,
			XmNscrollingPolicy, XmAPPLICATION_DEFINED,
			XmNvisualPolicy, XmVARIABLE,
			XmNscrollBarDisplayPolicy, XmSTATIC,
			XmNshadowThickness, 0,
			NULL);

		/* if (net->common.isInet == TRUE) */
		XtVaSetValues(sw,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);	

		net->etc.etcList = XtCreateWidget("etcList",
			xmListWidgetClass, sw, args,i);

		XtOverrideTranslations(net->etc.etcList,
			net->common.parsed_xlations);

		setEtcList(net->etc.etcList, net->etc.etcHosts, ret);
		/* non-inet: update the other widget thru the callback */
		/* double click */
		XtAddCallback(net->etc.etcList, XmNdefaultActionCallback,
			(void(*)())etcListDefCB, NULL);
		/* single click */
		XtAddCallback(net->etc.etcList, XmNbrowseSelectionCallback,
			(void(*)())etcListBrowseCB, NULL);
		XtManageChild(net->etc.etcList);
		if (net->common.etcStatus == NULL) {
			if (net->common.isInet == TRUE) {
				net->common.etcStatus = 
					XtVaCreateManagedWidget("status",
					xmLabelWidgetClass, net->etc.etcRC, 
					NULL);
				XtVaSetValues(net->common.etcStatus,
					XmNleftAttachment, XmATTACH_FORM,
					XmNbottomAttachment, XmATTACH_FORM,
					XmNrightAttachment, XmATTACH_FORM,
					NULL);
				XtVaSetValues(sw,
					XmNbottomAttachment, XmATTACH_WIDGET,
					XmNbottomWidget, net->common.etcStatus,
					NULL);
				setLabel(net->common.etcStatus, "");
			}
		}
	}
        return(net->etc.etcRC);
}

void
resizeDnsWin(Widget w, XtPointer msg, XConfigureEvent *event)
{
	Widget	Panesform;
	Dimension	total_width, sub_width;
		
	if (event->type == ConfigureNotify) {
		Panesform = XtParent(XtParent(net->dns.dnsArray[DNSL1]));	
		XtVaGetValues(Panesform, XmNwidth, &total_width, NULL);
		sub_width = total_width/3;
		XtVaSetValues(XtParent(net->dns.dnsArray[DNSL1]),
			XmNwidth, sub_width, NULL);
		XtVaSetValues(XtParent(net->dns.dnsArray[DNSL2]),
			XmNwidth, sub_width, NULL);
		XtVaSetValues(XtParent(net->dns.dnsArray[DNSL3]),
			XmNwidth, sub_width, NULL);
	}	
}

void
resizeEtcWin(Widget w, XtPointer msg, XConfigureEvent *event)
{
	Widget	Panesform;
	Dimension	total_width;
		
	if (event->type == ConfigureNotify) {
		Panesform = XtParent(XtParent(net->etc.etcList));	
		XtVaGetValues(Panesform, XmNwidth, &total_width, NULL);
		XtVaSetValues(XtParent(net->etc.etcList),
			XmNwidth, total_width, NULL);
	}	
}

Widget 
createDnsHosts(netInfo *net)
{
	Widget label, sep1, sep2, sw1, sw2, sw3;
	Widget labelRC,listRC, labelForm, listForm;
	Widget arrowRC, arrowForm, arrowrc;
	Widget leftRC, middleRC, rightRC;
	readHostsReturn ret;
	Boolean dnsExists;
	Arg	args[20];
	int	i;
	XtCallbackRec	dns_init_cb[2];

	DmMnemonicInfoRec	mneInfo[MI_TOTAL];
	char			tmpstr[BUFSIZ];
	unsigned char *		mne;
	KeySym			mneks;
	XmString		mneString;
	int			mm;

	if (net->common.isInet == TRUE) {
		/* use form instead of rc */
		net->dns.dnsRC = XtVaCreateWidget("dns", 
			xmFormWidgetClass, net->common.topRC, NULL);
		XtVaSetValues(net->dns.dnsRC,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, net->common.menubar,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL); 
	} 
	else { 
		net->dns.dnsRC = XtVaCreateWidget("dns",
			xmRowColumnWidgetClass, net->common.topRC, NULL);
	}
		
	labelRC = XtVaCreateManagedWidget("labelRC", 
			xmRowColumnWidgetClass, net->dns.dnsRC,
			 XmNorientation, XmHORIZONTAL, NULL);

	if (net->common.isInet == TRUE)
		XtVaSetValues(labelRC,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);

	net->dns.domainText = crt_inputlabel(labelRC, TXT_domain, 0);
	XtAddCallback(
		net->dns.domainText,
		XmNvalueChangedCallback,
		domainNameChangedCB,
		NULL
	);
	sprintf(tmpstr, mygettxt(TXT_domainBut));
	net->dns.showDomainBut = XtVaCreateManagedWidget(tmpstr,
				 xmPushButtonWidgetClass, labelRC, NULL);
		
	/*
	setLabel(net->dns.showDomainBut, TXT_domainBut);
	*/

	XmSTR_N_MNE(
		TXT_domainBut,
		MNE_update,
		mneInfo[0],
		DM_B_MNE_ACTIVATE_CB | DM_B_MNE_GET_FOCUS
	);

	XmStringFree(mneString);
	XtVaSetValues(net->dns.showDomainBut, XmNmnemonic, mneks, NULL);

	XtAddCallback(net->dns.showDomainBut, XmNactivateCallback,
		(void(*)())showCB, NULL);

	mneInfo[0].w = net->dns.showDomainBut;
	mneInfo[0].cb = (XtCallbackProc)showCB;

	REG_MNE(net->common.toplevel, mneInfo, 1);

	sep1 = XtVaCreateManagedWidget("spe1",
		xmSeparatorWidgetClass, net->dns.dnsRC, NULL);
	
	if (net->common.isInet == TRUE)
		XtVaSetValues(sep1, XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, labelRC,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);

	arrowForm = XtVaCreateManagedWidget("arrowForm",
		xmFormWidgetClass, net->dns.dnsRC, 
		NULL);

	if (net->common.isInet == TRUE)
		XtVaSetValues(arrowForm, XmNfractionBase, 5, 
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, sep1,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL); 
	else
		XtVaSetValues(arrowForm, XmNfractionBase, 5, NULL);

	net->dns.leftArrow = XtVaCreateManagedWidget("leftArrow",
		xmArrowButtonWidgetClass, arrowForm, 
		XmNarrowDirection, XmARROW_LEFT,
		NULL);
	XtVaSetValues(net->dns.leftArrow, 
		XmNleftAttachment, XmATTACH_POSITION, 
		XmNleftPosition, 2, 
		NULL);
	XtAddCallback(net->dns.leftArrow, 
		XmNactivateCallback, (void(*)()) leftArrowCB, NULL);
	net->dns.rightArrow = XtVaCreateManagedWidget("rightArrow",
		xmArrowButtonWidgetClass, arrowForm, 
		XmNarrowDirection, XmARROW_RIGHT,
		NULL);
	XtVaSetValues(net->dns.rightArrow, 
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, net->dns.leftArrow,
		NULL);	
	XtAddCallback(net->dns.rightArrow, 
		XmNactivateCallback, (void(*)()) rightArrowCB, NULL);

	sep2 = XtVaCreateManagedWidget("spe2",
		xmSeparatorWidgetClass, net->dns.dnsRC, NULL);

	if (net->common.isInet == TRUE)
		XtVaSetValues(sep2, XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, arrowForm,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);

	listRC = XtVaCreateManagedWidget("listRC", 
			xmFormWidgetClass, net->dns.dnsRC, NULL);

	if (net->common.isInet == TRUE)
		XtVaSetValues(listRC, XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, sep2,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);

	labelForm = XtVaCreateManagedWidget("labelForm",
		xmFormWidgetClass, listRC, 
		XmNfractionBase, 3, NULL);

	XtVaSetValues(labelForm, XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	net->dns.listLabel[DNSL1] = XtVaCreateManagedWidget("label", 
				xmLabelWidgetClass, labelForm, 	
				NULL);
	setSLabel(net->dns.listLabel[DNSL1], " "); 
	XtVaSetValues(net->dns.listLabel[DNSL1],
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 0, NULL);
	dns_init_cb[0].closure = dns_init_cb[1].closure = NULL;
	dns_init_cb[0].callback = dnsInitProc;
	dns_init_cb[1].callback = NULL;
	i=0;
	XtSetArg(args[i], XmNvisibleItemCount, VISIBLE_ITEMS); i++;
	XtSetArg(args[i], XmNnumColumns, 1); i++;	
	XtSetArg(args[i], XmNstaticRowCount, 0); i++;	
	XtSetArg(args[i], XmNlistColumnSpacing, 5); i++;	
	XtSetArg(args[i], XmNitemInitCallback, dns_init_cb); i++;	
	XtSetArg(args[i], XmNlistSizePolicy, XmCONSTANT); i++; 
	XtSetArg(args[i], XmNscrollBarDisplayPolicy, XmAS_NEEDED); i++;

	sw1 = XtVaCreateManagedWidget("sw1",
		xmScrolledWindowWidgetClass, listRC,
		XmNscrollingPolicy, XmAPPLICATION_DEFINED,
		XmNvisualPolicy, XmVARIABLE,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNshadowThickness, 0,
		NULL);
	/* net->dns.dnsArray[DNSL1] = XmCreateScrolledList(listRC,"domlist",args,i); */
	if (net->common.isInet == TRUE)
		XtVaSetValues(sw1,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, labelForm,
			XmNleftAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);
	else
		XtVaSetValues(sw1,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, labelForm,
			XmNleftAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);

	net->dns.dnsArray[DNSL1] = XtCreateWidget("domlist1",
		xmListWidgetClass, sw1, 
		args, i);
		
	XtOverrideTranslations(net->dns.dnsArray[DNSL1], 
		net->common.parsed_xlations);

	XtAddCallback(net->dns.dnsArray[DNSL1], 
		XmNdefaultActionCallback, (void(*)())dnsListDefCB, DNSL1);
	XtAddCallback(net->dns.dnsArray[DNSL1], 
		XmNbrowseSelectionCallback, (void(*)())dnsListBrowseCB, DNSL1);
    	XtManageChild(net->dns.dnsArray[DNSL1]); 

	/* middleRC = XtVaCreateManagedWidget("middleRC", 
			xmRowColumnWidgetClass, listRC, NULL); */
	sw3 = XtVaCreateManagedWidget("sw3",
		xmScrolledWindowWidgetClass, listRC,
		XmNscrollingPolicy, XmAPPLICATION_DEFINED,
		XmNvisualPolicy, XmVARIABLE,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNshadowThickness, 0,
		NULL);
	if (net->common.isInet == TRUE)
		XtVaSetValues(sw3,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, labelForm,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);
	else
		XtVaSetValues(sw3,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, labelForm,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);
	sw2 = XtVaCreateManagedWidget("sw2",
		xmScrolledWindowWidgetClass, listRC,
		XmNscrollingPolicy, XmAPPLICATION_DEFINED,
		XmNvisualPolicy, XmVARIABLE,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNshadowThickness, 0,
		NULL);

	XtVaSetValues(sw2,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, labelForm,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, sw1,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, sw3,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	net->dns.listLabel[DNSL2] = XtVaCreateManagedWidget("label", 
			xmLabelWidgetClass, labelForm, NULL);
	setSLabel(net->dns.listLabel[DNSL2], " "); 
	XtVaSetValues(net->dns.listLabel[DNSL2],
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 1,
		NULL);

	/* create the 3rd scrolled list first before 2nd one */
	net->dns.dnsArray[DNSL3] = XtCreateWidget("domlist3",
		xmListWidgetClass, sw3, args,i);
	net->dns.dnsArray[DNSL2] = XtCreateWidget("domlist2", 
		xmListWidgetClass, sw2, args,i);

	XtOverrideTranslations(net->dns.dnsArray[DNSL2],
		net->common.parsed_xlations);

	XtAddCallback(net->dns.dnsArray[DNSL2], 
		XmNdefaultActionCallback, (void(*)())dnsListDefCB, (XtPointer)DNSL2);
	XtAddCallback(net->dns.dnsArray[DNSL2], 
		XmNbrowseSelectionCallback, (void(*)())dnsListBrowseCB, (XtPointer)DNSL2);

	net->dns.listLabel[DNSL3] = XtVaCreateManagedWidget("label", 
		xmLabelWidgetClass, labelForm, NULL);
	setSLabel(net->dns.listLabel[DNSL3], " "); 
	XtVaSetValues(net->dns.listLabel[DNSL3],
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 2,
		NULL);
	XtOverrideTranslations(net->dns.dnsArray[DNSL3],
		net->common.parsed_xlations);

	XtAddCallback(net->dns.dnsArray[DNSL3], XmNdefaultActionCallback,
		(void(*)())dnsListDefCB, (XtPointer)DNSL3);
    	XtAddCallback(net->dns.dnsArray[DNSL3], XmNbrowseSelectionCallback,
		(void(*)())dnsListBrowseCB, (XtPointer)DNSL3);
    	XtManageChild(net->dns.dnsArray[DNSL2]); 
    	XtManageChild(net->dns.dnsArray[DNSL3]);

	/* create status line for inet */
	if (net->common.isInet == TRUE) {
		net->common.dnsStatus = XtVaCreateManagedWidget("status",
			xmLabelWidgetClass, net->dns.dnsRC, NULL);
		XtVaSetValues(net->common.dnsStatus,
			XmNleftAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);
		XtVaSetValues(listRC,
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget, net->common.dnsStatus,
			NULL);
		setLabel(net->common.dnsStatus, "");
	}

	/* calling readDnsHosts from the initialization time */
	net->dns.queryFrom = START;
	if((ret = readDnsHosts(net->dns.resolv->domain)) == Failure) {
		return(NULL);
	}
	return(net->dns.dnsRC);
}

resizeMainWindow(netInfo *net)
{
	Widget		panesForm, etcForm;
	Dimension	total_width, sub_width, base_width, base_height, width;
	int		max_w, max_h;

	max_w = DisplayWidth(XtDisplay(net->common.toplevel),
		DefaultScreen(XtDisplay(net->common.toplevel)));
	if (max_w <= 800)
		max_w = max_w * 5/8;
	else
		max_w = max_w * 1/2;
	
	max_h = DisplayHeight(XtDisplay(net->common.toplevel),
		DefaultScreen(XtDisplay(net->common.toplevel)));

	if (max_h <= 600)
		max_h = max_h * 5/8;
	else
		max_h = max_h * 1/3;

	/* set the min width and height of the toplevel */
	XtVaSetValues(net->common.toplevel,
		XmNminWidth, max_w/2,
		XmNminHeight, max_h/2,
		NULL);

	if (net->dns.dnsRC && (net->common.isDnsConfigure == TRUE)) {

		sub_width = max_w/3;
		XtVaSetValues(XtParent(net->dns.dnsArray[DNSL1]),
			XmNwidth, sub_width, NULL);
		XtVaSetValues(XtParent(net->dns.dnsArray[DNSL2]),
			XmNwidth, sub_width, NULL);
		XtVaSetValues(XtParent(net->dns.dnsArray[DNSL3]),
			XmNwidth, sub_width, NULL);

		/* size the etc view so it has proper size on switching view*/
		XtVaSetValues(XtParent(net->etc.etcList), 
			XmNwidth, max_w, NULL);
	
		if (net->common.isInet == TRUE) {
			/* add event handler to do the resizing */
			XtAddEventHandler(net->dns.dnsRC, StructureNotifyMask,
				False, (void(*)())resizeDnsWin, NULL);
			XtAddEventHandler(net->etc.etcRC, StructureNotifyMask,
				False, (void(*)())resizeEtcWin, NULL);
		}
	}
	else {
		/* only Etc hosts */

		if (net->common.isInet == TRUE) {
			/* add event handler to do the resizing */
			XtVaSetValues(XtParent(net->etc.etcList), 
				XmNwidth, max_w, NULL);
			XtAddEventHandler(net->etc.etcRC, StructureNotifyMask,
				False, (void(*)())resizeEtcWin, NULL);
		}
	}

	XtVaSetValues(net->common.toplevel, XmNwidth, max_w, 
		XmNheight, max_h, NULL);

}

createHostsWin(netInfo *client_net)
{

	Boolean		ret, btn1_transfer;
	XmString	viewstr, domainstr, systemstr;
	Widget		sep1, sep2, sep3, panesForm, etcForm, actionForm, f;
	char		trans[MEDIUMBUFSIZE];
	Dimension	total_width, sub_width, base_width, base_height, width;
	unsigned char 	*Vmne, *Dmne, *Smne;
	KeySym		Vks, Dks, Sks;
	DmMnemonicInfoRec	mneInfo[MI_TOTAL];
	int			i;
	int			mm;

	/* net is a global variable in this file */

	net = client_net;

	if (net == 0)
		return;

	if (net->common.isInet == FALSE) {
		if (net->common.toplevel != NULL) {
			/* the 2nd time or later comes to this function */
			XtPopup(net->common.toplevel, XtGrabNone);	
			return;
		}

		/* busyCursor(XtParent(net->lookup.clientText), TRUE); */
		
		/* create the popup shell for non-inet */
		if (net->common.toplevel == NULL) {
			net->common.toplevel = XtVaCreatePopupShell("dialog",
				xmDialogShellWidgetClass,
				XtParent(net->lookup.clientText),
				XmNtitle,  mygettxt(TXT_HostTitle),
				XmNdeleteResponse, XmUNMAP,
				XmNallowShellResize, True,
				NULL);
			r_decor(net->common.toplevel);
			f = XtVaCreateWidget("f",
				xmFormWidgetClass, net->common.toplevel,
				NULL);
			net->common.topRC = XtVaCreateWidget("topRC",
				xmRowColumnWidgetClass, f,
				NULL);
			XtVaSetValues(net->common.topRC, 
				XmNtopAttachment, XmATTACH_FORM,
				XmNleftAttachment, XmATTACH_FORM,
				XmNrightAttachment, XmATTACH_FORM,
				XmNbottomAttachment, XmATTACH_FORM,
				NULL);
			/* create the simple option menu */
			viewstr = i18nString(TXT_view);
			domainstr = i18nString(TXT_domainList);
			systemstr = i18nString(TXT_systemList);
			Vmne = (unsigned char *)mygettxt(NM_view);
			Vks = XStringToKeysym((char *)Vmne);
			Dmne = (unsigned char *)mygettxt(NM_domainList);
			Dks = XStringToKeysym((char *)Dmne);
			Smne = (unsigned char *)mygettxt(NM_systemList);
			Sks = XStringToKeysym((char *)Smne);
			net->lookup.viewBtn = XmVaCreateSimpleOptionMenu(
				net->common.topRC, "optmenu",
				viewstr, Vks, 0, (void(*)())optCB,
				XmVaPUSHBUTTON, domainstr, Dks, NULL, NULL,
				XmVaPUSHBUTTON, systemstr, Sks, NULL, NULL,
				NULL);
			
			XmStringFree(viewstr);
			XmStringFree(domainstr);
			XmStringFree(systemstr);
			XtManageChild(net->lookup.viewBtn); 

			sep1 = XtVaCreateManagedWidget("sep1",
			      xmSeparatorWidgetClass, net->common.topRC, NULL);
		}
	}

	/* See if the enableBtn1Transfer is turned on or not */
	XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(net->common.toplevel)),
		"enableBtn1Transfer", &btn1_transfer, NULL);
	if (!btn1_transfer)
		sprintf(trans, "%s", dragTranslations2);
	else {
		sprintf(trans, "%s\n%s", dragTranslations2, dragTranslations1);
	}

	if (net->common.parsed_xlations == NULL) {
		net->common.parsed_xlations = 
			XtParseTranslationTable(trans);
		XtAppAddActions(net->common.app, dragActions,
			XtNumber(dragActions));
	}
		
	/* Create etcRC first so we can popup etcList if DNS fails */
	net->etc.etcRC = createEtcHosts(net);

	if (net->dns.resolv == NULL) 
		initResolvConf(&net->dns.resolv);
	if((ret = readResolvConf(net->dns.resolv)) == TRUE ){
		net->common.isDnsConfigure = TRUE;
		if (net->dns.dnsRC == NULL) {
			net->dns.dnsRC = createDnsHosts(net);
			if (net->dns.dnsRC == NULL) {
				/* the query fails, need to map ETC view
				net->common.isDnsConfigure = FALSE; */
			}
			else {
				XtVaSetValues(net->dns.domainText, 
				      	XmNvalue, net->dns.resolv->domain, NULL);
				
				/* In dns view in inet, insensitize New, 
				 * Delete and Domian Listing menu items.
				 */
				if (net->common.isInet == TRUE)
					dnsMenuBar();
			}
		}
		else {
			/* 2nd or later time comes in here, just map the
			 * RC, turns off the busy cursor and popup
			 */
			XtManageChild(net->dns.dnsRC);
			/* busyCursor(XtParent(net->dns.dnsRC), FALSE); */
			XtPopup(net->common.toplevel, XtGrabNone);
		}
	} 
	else {
		net->common.isDnsConfigure = FALSE;
		/* check if etcHosts exists , if not, popup error msg*/
		if (net->etc.etcHosts != NULL)  {

			XtManageChild(net->etc.etcRC);

			/* In etc view, insensitize the menu item and view 
			   button */
			if (net->common.isInet == TRUE) {
				etcMenuBar();
				XtPopup(net->common.toplevel, XtGrabNone); 
			}
		}
		else {
			/* need to exit the inet thru the OK button */
			createExitMsg(net->common.toplevel, ERROR, mygettxt(ERR_noSetup), NULL);
		}
	}
	
	if (net->common.isInet == FALSE && net->lookup.selectLabel == NULL) { 
		/* non-inet: create the selection label and  action area */
		/* action area for non-inet */
		net->lookup.selectLabel = crt_inputlabellabel(net->common.topRC,
			TXT_selection, 0, NULL);
		net->lookup.actions = actions;
		sep2 = XtVaCreateManagedWidget("sep2",
			xmSeparatorWidgetClass, net->common.topRC, NULL);
		if (net->common.isDnsConfigure == TRUE) {
			actionForm = createActionArea(
				net->common.topRC, 
				actions,
				XtNumber(actions),
				net->dns.dnsRC,
				mneInfo,
				0
			);	
		} else {
			actionForm = createActionArea(
				net->common.topRC, 
				actions,
				XtNumber(actions),
				net->etc.etcRC,
				mneInfo,
				0
			);	
		}
		XtVaSetValues(f,
			XmNdefaultButton, actions[0].widget,
			XmNcancelButton, actions[1].widget,
			NULL);
		XtVaSetValues(actions[0].widget,
			XmNshowAsDefault, True,
			XmNdefaultButtonShadowThickness, 1,
			NULL);
		XtManageChild(actionForm);
		sep3 = XtVaCreateManagedWidget("sep3",
			xmSeparatorWidgetClass, net->common.topRC, NULL);
		if (net->common.status == NULL) {
			net->common.status = XtVaCreateManagedWidget("status",
				xmLabelWidgetClass, net->common.topRC, NULL);
			setLabel(net->common.status, ""); 
		}
	}
	
	/* resize the 3 panes before popup */
	resizeMainWindow(net);

	if (net->common.isInet == TRUE)
		XtManageChild(net->common.topRC);
	else {
		XtManageChild(net->common.topRC);
		XtManageChild(f);

		REG_MNE(net->common.toplevel, mneInfo, 3);
	}

	XtPopup(net->common.toplevel, XtGrabNone);
}

static void
domainNameChangedCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	char *  name;

	name = XmTextFieldGetString(net->dns.domainText);
	removeWhitespace(name);
	if (strlen(name) > 0) {
		XtSetSensitive(net->dns.showDomainBut, True);
	} else {
		XtSetSensitive(net->dns.showDomainBut, False);
	}
	XtFree(name);

	return;
}
