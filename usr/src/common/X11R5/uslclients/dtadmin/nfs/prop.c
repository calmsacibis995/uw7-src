#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/prop.c	1.20"
#endif

/*
 * Module:	dtadmin:nfs   Graphical Administration of Network File Sharing
 * File:	prop.c        GUI for browse etcHost and DNS
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>


#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#ifdef EDIT_RES
#include <X11/Xmu/Editres.h>
#endif

#include <Xol/OpenLook.h>

#include <Xol/PopupWindo.h>
#include <Xol/FButtons.h>
#include <Xol/TextField.h>
#include <Xol/TextEdit.h>
#include <Xol/StaticText.h>
#include <Xol/Caption.h>
#include <Xol/FList.h>
#include <Xol/ScrolledWi.h>

#include <DtI.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/ListGizmo.h>
#include <Gizmo/InputGizmo.h>
#include <Gizmo/LabelGizmo.h>
#include <Gizmo/STextGizmo.h>
#include <Gizmo/SpaceGizmo.h>
#include <Gizmo/ChoiceGizm.h>
#include <Gizmo/ModalGizmo.h>

#include "client.h"
#include "common.h"
#include "prop.h"
#include "help.h"
#include "text.h"


#define HOST_FIELD_SIZE    20
#define MEDIUMBUFSIZE    1024

extern readHostsReturn	ReadHosts (HostList **);
extern readHostsReturn  readDnsHosts(char *);

static Bool TrapMapOrVis(Display *, XEvent *, XPointer);
static void ExecuteCB(Widget, XtPointer, OlFlatCallData *);
static void SelectCB(Widget, XtPointer, OlFlatCallData *);
static void UnselectCB(Widget, XtPointer, XtPointer);
static void hostMenuCB(Widget, XtPointer, XtPointer);
static Boolean decideScrLayout(errorList *);
static void getHosts(ListGizmo *, HostList *, readHostsReturn);
static void viewSysCB(Widget, XtPointer, XtPointer);
static void listSysCB(Widget, XtPointer, XtPointer);
static void leftArrowCB(Widget, XtPointer, XtPointer);
static void rightArrowCB(Widget, XtPointer, XtPointer);
static void errorCB(Widget, XtPointer, XtPointer);
static void popupMessage(char *);
static void popupErrMessage(errorList);
static void handleFocus(Widget, XtPointer , XFocusChangeEvent *);
static void busyCursor(Widget, int );
static void dnsQuery(int);
static void killCB();
static void mappLists(dnsList * , int , dnsListNum );
static void overflowCB(Widget, int, OlFListItemsLimitExceededCD *);
static void TimeoutCursors(Boolean);
static Boolean CheckForInterrupt(void);
static void stop(void);

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

static String lastHost=NULL;
static Boolean stopped;

/* for next page and previous page */
static char **next_fields, **prev_fields;
/* for swap out the slot occupied by next or previous page entries */
static int total_items[4];
static char **list_fields[4];
static char **p_fields[4];
static char **n_fields[4];
static int preferred;
static String SelectedHost = NULL;
static UserData *user;
Boolean isBusy;
Boolean etcExists;
dnsInfo dns;
static Pixmap hostPixmap = (Pixmap)0;
static Pixmap domainPixmap = (Pixmap)0;

static MenuItems  hostMenuItems[] =
   {
      {True, TXT_OK,  CHR_OK },
      {True, TXT_Cancel, CHR_Cancel },
      {True, TXT_Help,   CHR_Help },
      { 0 }
   };

static MenuGizmo  hostMenu = { &HostPopupMenuHelp, "_X_", "_X_", hostMenuItems, hostMenuCB };

static MenuItems listViewItems[] = 
   {
      {True, TXT_DomList,  CHR_DomList },
      {True, TXT_SysList, CHR_SysList },
      { 0 }
   };

static MenuGizmo listViewMenu = { &HostPopupMenuHelp, "_X_", "_X_", listViewItems,
                                    viewSysCB, NULL,EXC };

static ChoiceGizmo viewChoice = { &HostPopupMenuHelp, "_X_", TXT_View,  &listViewMenu};
static ListHead currentItem = {
	(ListItem *)0, 0, 0
};

static ListHead previousItem = {
	(ListItem *)0, 0, 0
};

static ListHead initialItem = {
	(ListItem *)0, 0, 0
};
static Setting HostListSetting =
{
	"hostList",
	(XtPointer)&initialItem,
	(XtPointer)&currentItem,
	(XtPointer)&previousItem
};

static ListHead domainCurrentItem1 = {
        (ListItem *)0, 0, 0
};

static ListHead domainPreviousItem1 = {
        (ListItem *)0, 0, 0
};

static ListHead domainInitialItem1 = {
        (ListItem *)0, 0, 0
};

static Setting DomainListSetting1 =
{
        "dnsList1",
        (XtPointer)&domainInitialItem1,
        (XtPointer)&domainCurrentItem1,
        (XtPointer)&domainPreviousItem1
};

static ListHead domainCurrentItem2 = {
        (ListItem *)0, 0, 0
};

static ListHead domainPreviousItem2 = {
        (ListItem *)0, 0, 0
};

static ListHead domainInitialItem2 = {
        (ListItem *)0, 0, 0
};

static Setting DomainListSetting2 =
{
        "dnsList2",
        (XtPointer)&domainInitialItem2,
        (XtPointer)&domainCurrentItem2,
        (XtPointer)&domainPreviousItem2
};

static ListHead domainCurrentItem3 = {
        (ListItem *)0, 0, 0
};

static ListHead domainPreviousItem3 = {
        (ListItem *)0, 0, 0
};

static ListHead domainInitialItem3 = {
        (ListItem *)0, 0, 0
};

static Setting DomainListSetting3 =
{
        "dnsList3",
        (XtPointer)&domainInitialItem3,
        (XtPointer)&domainCurrentItem3,
        (XtPointer)&domainPreviousItem3
};
static StaticTextGizmo etcHostsLabel= {
        &HostListHelp, "etcHostsLabel", TXT_etcHosts, NorthWestGravity
};

static ListGizmo hostListG = {
	&HostListHelp, 
	"hostList", "list Label", (Setting *)&HostListSetting,
	"%p %s", True, 6,
	NULL,
	(XtArgVal) False,		/* Don't copy this list */
	ExecuteCB, SelectCB, UnselectCB,
};

static GizmoRec etcHostsGizmos[] =
{
   { StaticTextGizmoClass, &etcHostsLabel},
   { ListGizmoClass, &hostListG },
};

static LabelGizmo etcHostsScreen=
{ &HostListHelp, "_X_", NULL, etcHostsGizmos, XtNumber(etcHostsGizmos),OL_FIXEDCOLS};

static Setting inputSetting = {
        "", "",
};

static InputGizmo domainName =
{
    &HostListHelp,
    "domainName",
    TXT_domainName,
    NULL,
    &inputSetting,
    HOST_FIELD_SIZE,
    listSysCB,
};

static MenuItems listSysItems[] = {
	{ True, TXT_listSys, CHR_listSys}, {0},
};

static MenuGizmo listSysMenu = { &HostListHelp, "_X_", "_X_", listSysItems,
                                    listSysCB,NULL,NULL,OL_FIXEDROWS };

static ChoiceGizmo domainSysChoice = { &HostListHelp, "_X_", "",  &listSysMenu};

static GizmoRec domainSysGizmos[] =
{
   { InputGizmoClass, &domainName},
   { ChoiceGizmoClass, &domainSysChoice},
};

static LabelGizmo domainSysLabel=
{ &HostListHelp, "_X_", NULL, domainSysGizmos, XtNumber(domainSysGizmos), OL_FIXEDROWS };

/* definitions for the arrow buttons */

static SpaceGizmo space = {
	0, 80
};

static MenuItems button0Items[] = {
        {True, "<==",              (XtArgVal)0,    NULL,   leftArrowCB},
        {NULL}
};
static MenuItems button1Items[] = {
        {True, "==>",              (XtArgVal)0,    NULL,   rightArrowCB},
        {NULL}
};

static MenuGizmo menu0 = {
        &HostListHelp, "pixmapMenu0", NULL, button0Items,
        NULL, NULL
};
static MenuGizmo menu1 = {
        &HostListHelp, "pixmapMenu1", NULL, button1Items,
        NULL, NULL
};

static GizmoRec arrAr[] =
{
   { SpaceGizmoClass,	&space},
   { MenuBarGizmoClass, &menu0},
   { MenuBarGizmoClass, &menu1 },
};

static Arg labelArgs[] = {
        XtNhSpace,      200,
        XtNhPad,        100
};

static LabelGizmo arrMenu=
{ &HostListHelp, "_X_", NULL, arrAr, XtNumber(arrAr),OL_FIXEDROWS,1, labelArgs};

static StaticTextGizmo dnsLabel1 = {
        &HostListHelp, "dnsLabel1", " ", NorthWestGravity
};

static ListGizmo domainSysListG1 = {
	&HostListHelp, 
	"domainSysList1", "domain list Label", (Setting *)&DomainListSetting1,
	"%p %s", True, 6,
	NULL,
	(XtArgVal) False,		/* Don't copy this list */
	ExecuteCB, SelectCB, UnselectCB,
};
static GizmoRec dnsArr1[] =
{
   { StaticTextGizmoClass, &dnsLabel1},
   { ListGizmoClass, &domainSysListG1 },
};

static LabelGizmo dnsListG1=
{ &HostListHelp, "_X_", NULL, dnsArr1, XtNumber(dnsArr1),OL_FIXEDCOLS};

static StaticTextGizmo dnsLabel2 = {
        NULL, "dnsLabel2", " ", NorthWestGravity
};
static ListGizmo domainSysListG2 = {
	&HostListHelp, 
	"domainSysList2", "domain list Label", (Setting *)&DomainListSetting2,
	"%p %s", True, 6,
	NULL,
	(XtArgVal) False,		/* Don't copy this list */
	ExecuteCB, SelectCB, UnselectCB,
};

static GizmoRec dnsArr2[] =
{
   { StaticTextGizmoClass, &dnsLabel2},
   { ListGizmoClass, &domainSysListG2 },
};

static LabelGizmo dnsListG2=
{ &HostListHelp, "_X_", NULL, dnsArr2, XtNumber(dnsArr2),OL_FIXEDCOLS};

static StaticTextGizmo dnsLabel3 = {
        &HostListHelp, "dnsLabel3", " ", NorthWestGravity
};
static ListGizmo domainSysListG3 = {
	&HostListHelp, 
	"domainSysList3", "domain list Label", (Setting *)&DomainListSetting3,
	"%p %s", True, 6,
	NULL,
	(XtArgVal) False,		/* Don't copy this list */
	ExecuteCB, SelectCB, UnselectCB,
};
static GizmoRec dnsArr3[] =
{
   { StaticTextGizmoClass, &dnsLabel3},
   { ListGizmoClass, &domainSysListG3 },
};

static LabelGizmo dnsListG3=
{ &HostListHelp, "_X_", NULL, dnsArr3, XtNumber(dnsArr3),OL_FIXEDCOLS};

static GizmoRec domainLists[] =
{
   { LabelGizmoClass, &dnsListG1},
   { LabelGizmoClass, &dnsListG2},
   { LabelGizmoClass, &dnsListG3},
};


static LabelGizmo domainListsGizmo=
{ &HostListHelp, "_X_", NULL, domainLists, XtNumber(domainLists), OL_FIXEDROWS };

static GizmoRec domainHostsGizmos[] =
{
   { LabelGizmoClass, &domainSysLabel},
   { LabelGizmoClass, &arrMenu},
   { LabelGizmoClass, &domainListsGizmo}
};

static LabelGizmo domainHostsScreen=
{ &HostListHelp, "_X_", NULL, domainHostsGizmos, XtNumber(domainHostsGizmos),OL_FIXEDCOLS};

static StaticTextGizmo nameLabel= {
        &HostListHelp, "nameLabel", " ", NorthWestGravity
};

static GizmoRec labelArray[] = {
        {StaticTextGizmoClass,  &nameLabel}
};


static LabelGizmo selectedSystemLabel = {
        &HostListHelp, "selectedSystemLabel", TXT_nameLabel,
        labelArray, XtNumber (labelArray),
        OL_FIXEDROWS, 1,
        0, 0,
        True
};

static GizmoRec hostGizmos[] =
{
   { AbbrevChoiceGizmoClass, &viewChoice},
   { LabelGizmoClass, &etcHostsScreen},
   { LabelGizmoClass, &domainHostsScreen},
   { LabelGizmoClass, &selectedSystemLabel},
};

typedef enum _hostMenuItemsIndex 
   { applyHost,  cancelHost, hostHelp } 
   hostMenuItemIndex;


static PopupGizmo HostPopup =
   { &HostListHelp, "host", TXT_HostTitle, &hostMenu, hostGizmos,
	 XtNumber(hostGizmos) }; 

static MenuItems errorBut[] = {
        {True, BUT_ok,       MNEM_ok,     NULL, errorCB},
        {NULL}
};

static MenuGizmo errorMenu = {
        &HostListHelp, "errorMenu", NULL, errorBut,
        NULL, NULL, CMD, OL_FIXEDROWS, 1, 0
};

static ModalGizmo errorModal= {
        &HostListHelp,
        "no name",
        TXT_error1,
        &errorMenu
};
static MenuItems timeoutBut[] = {
        {True, But_Stop,       MNEM_Stop,     NULL, stop},
        {NULL}
};

static MenuGizmo timeoutMenu = {
        &HostListHelp, "timeoutMenu", NULL, timeoutBut,
        NULL, NULL, CMD, OL_FIXEDROWS, 1, 0
};

static ModalGizmo timeoutModal= {
        &HostListHelp,
        "no name",
        TXT_dnsQueryTitle,
        &timeoutMenu
};


static void
SetMenuPixmaps (menu, filename, func, i)
Widget  menu;
char *  filename;
void     (*func)();
int	i;
{
	static Boolean f=True;
        Screen *        screen = XtScreen(menu);
        DmGlyphRec *    gp;
        static char *   flatMenuFields[] = {
                XtNsensitive,  /* sensitive                      */
                XtNlabel,      /* label                          */
                XtNuserData,   /* mnemonic string                */
                XtNuserData,   /* nextTier | resource_value      */
                XtNselectProc, /* function                       */
                XtNclientData, /* client_data                    */
                XtNset,        /* set                            */
                XtNpopupMenu,  /* button                         */
                XtNmnemonic,   /* mnemonic                       */
                XtNbackgroundPixmap,
                XtNbusy
        };

        /* Change the item fields to add in a background pixmap */
        XtVaSetValues (
                menu,
                XtNitemFields,          flatMenuFields,
                XtNnumItemFields,       XtNumber(flatMenuFields),
                XtNhSpace,              0,
                (String)0
        );

        if ((gp = DmGetPixmap (screen, filename)) != NULL) {
                OlVaFlatSetValues (
                        menu, i,
                        XtNlabel, NULL,
                        XtNbackgroundPixmap, gp->pix,
                        (String)0
                );
        }
}

/*
 * This function is called from elsewhere when the lookup button on an
 * application is chosen. Returns the status of about successful listing.
 *
 */
extern Boolean 
HostCB(Widget w, UserData *client_data, XtPointer call_data)
{
	Arg             arg[10];
	errorList 	errnum = none;
	DmGlyphRec *    gp;
        Screen *        screen = XtScreen(w);

	/* get the user data passed from the application */
	user = client_data;    

	/* If first time create the lookup table popup */
	if (HostPopup.shell == NULL) {
		CreateGizmo(w, PopupGizmoClass, &HostPopup, NULL, 0);
        	SetMenuPixmaps (
                	(Widget)QueryGizmo (
				PopupGizmoClass, &HostPopup,
				GetGizmoWidget, "pixmapMenu0"
			),
			"lArrow16", leftArrowCB, 0
		);
		SetMenuPixmaps (
			(Widget)QueryGizmo (
				PopupGizmoClass, &HostPopup,
				GetGizmoWidget, "pixmapMenu1"
			),
			"rArrow16", rightArrowCB, 0
		);
		if (hostPixmap == (Pixmap)0) 
			if ((gp = DmGetPixmap (screen, "host16")) != NULL)
				hostPixmap = gp->pix;
		if (domainPixmap == (Pixmap)0) 
			if ((gp = DmGetPixmap (screen, "domain16")) != NULL)
				domainPixmap = gp->pix;

		/* update the help info */

		XtSetArg(arg[0], XtNnoneSet, True);
		XtSetArg(arg[1], XtNuserData, ETCL);
		XtSetValues(hostListG.flatList, arg, 2);
		XtSetArg(arg[1], XtNuserData, DNSL1);
		XtSetValues(domainSysListG1.flatList, arg, 2);
		XtSetArg(arg[1], XtNuserData, DNSL2);
		XtSetValues(domainSysListG2.flatList, arg, 2);
		XtSetArg(arg[1], XtNuserData, DNSL3);
		XtSetValues(domainSysListG3.flatList, arg, 2);

		XtAddCallback (
			(Widget) hostListG.flatList,
			XtNitemsLimitExceeded,
			(XtCallbackProc) overflowCB,
			(XtPointer) ETCL
		);

		XtAddCallback (
			(Widget) domainSysListG1.flatList,
			XtNitemsLimitExceeded,
			(XtCallbackProc) overflowCB,
			(XtPointer) DNSL1
		);

		XtAddCallback (
			(Widget) domainSysListG2.flatList,
			XtNitemsLimitExceeded,
			(XtCallbackProc) overflowCB,
			(XtPointer) DNSL2
		);

		XtAddCallback (
			(Widget) domainSysListG3.flatList,
			XtNitemsLimitExceeded,
			(XtCallbackProc) overflowCB,
			(XtPointer) DNSL3
		);

		OlVaFlatSetValues(hostMenu.child, applyHost,
					  XtNsensitive, False,	
					   NULL);
		XtVaSetValues(XtParent(domainSysListG1.flatList), XtNwidth, 40, NULL);
		XtVaSetValues(XtParent(domainSysListG2.flatList), XtNwidth, 40, NULL);
		XtVaSetValues(XtParent(domainSysListG3.flatList), XtNwidth, 40, NULL);
			/* create the error dialog  to be mapped on later */
		CreateGizmo ( HostPopup.shell, ModalGizmoClass, &errorModal, 0, 0);
	}else{
    		MapGizmo(PopupGizmoClass, &HostPopup);
		return;
	}

	busyCursor(XtParent(HostPopup.shell), True);
	isBusy = False;

	/* find if the any tables are being mapped */
	if(!decideScrLayout( &errnum)){
		user->hostSelected = False;
		return False;
	}
	user->hostSelected = True;
	/* If no tables, put up the error message */
	if(errnum)
		SetPopupMessage(&HostPopup, GetGizmoText(TXT_initMsg1));
	return True;
}


/*
 * Popdown the error dialog
 */
static void
killCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	dns.query = NULL;
        kill((-1)*dns.pid, 9);
	XtPopdown(timeoutModal.shell);
}

void
updateArrowBut(dnsInfo *dns)
{
	int width;
        if (dns->totalDnsItems < 3) {
		OlVaFlatSetValues(menu0.child, 0,
		      XtNsensitive, False, NULL);
		OlVaFlatSetValues(menu1.child, 0,
		      XtNsensitive, False, NULL);
        }
        else {
                if (dns->cur_pos->prev == NULL) {
			OlVaFlatSetValues(menu0.child, 0,
		      		XtNsensitive, False, NULL);
                }
                else {
			OlVaFlatSetValues(menu0.child, 0,
		      		XtNsensitive, True, NULL);
                }
                if (dns->cur_pos->next && dns->cur_pos->next->next &&
                        dns->cur_pos->next->next->next) {
			OlVaFlatSetValues(menu1.child, 0,
		      		XtNsensitive, True, NULL);
                }
                else {
			OlVaFlatSetValues(menu1.child, 0,
		      		XtNsensitive, False, NULL);
                }
        }
}
static void
leftArrowCB(Widget w, XtPointer x, XtPointer y)
{
        /* the left arrow must be sensitized when this function is called */
        dns.cur_pos = dns.cur_pos->prev;
        mappLists(dns.cur_pos, 3, DNSL1);
        updateArrowBut(&dns);
}

static void
rightArrowCB(Widget w, XtPointer x, XtPointer y)
{
        /* the right arrow button must be sensitizied when this is called */
        dns.cur_pos = dns.cur_pos->next;
        mappLists(dns.cur_pos, 3, DNSL1);
        updateArrowBut(&dns);
}

void updateEtc(){
	if(etcExists){
		XtManageChild(etcHostsScreen.controlWidget);
		MapGizmo(PopupGizmoClass, &HostPopup);
	}else{
		/* popup error message */
	}
}

updateDns(dnsInfo *dns)
{
	dnsList *tmp;
        int     i;


	switch(dns->queryFrom){
		case START:
			dns->totalDnsItems = 0;
			dns->cur_wid_pos = DNSL1;
                	dns->cur_pos = NULL;
                	addDnsItem(dns, dns->dnsHosts);
			mappLists(dns->cur_pos, 1,DNSL1);
                	updateArrowBut(dns);
			XtManageChild(domainHostsScreen.controlWidget);
    			MapGizmo(PopupGizmoClass, &HostPopup);
			break;
		case SHOWDOMAIN:
			for (tmp = dns->cur_pos; tmp && tmp->prev != NULL; tmp = tmp
->prev);
			pruneDnsList(&tmp, &dns->totalDnsItems);
			dns->cur_pos = NULL;
                	addDnsItem(dns, dns->dnsHosts);
			mappLists(dns->cur_pos, 1,DNSL1);
			mappLists(NULL, 1, DNSL2);
			mappLists(NULL, 1, DNSL3);
                	updateArrowBut(dns);

			break;
		case DOUBLECLICK:
			/* plume the tree starting from the one after cur_wid_pos */
                	tmp = dns->cur_pos;
			for (i = 0; i < dns->cur_wid_pos; i++)
			tmp = tmp->next;
			pruneDnsList(&tmp->next, &dns->totalDnsItems);
			/* always add to the end of the link list */
                	addDnsItem(dns, dns->dnsHosts);
                	/* mapp the panes */
                	switch(dns->cur_wid_pos) {
                        case DNSL1:
                                mappLists(tmp->next, 1, DNSL2);
                                mappLists(NULL, 1, DNSL3);
                                break;
                        case DNSL2:
                                mappLists(tmp->next, 1, DNSL3);
                                break;
                        case DNSL3:
                                dns->cur_pos = dns->cur_pos->next;
                                mappLists(dns->cur_pos, 3, DNSL1);
                                break;
                        default:
				SetPopupMessage(&HostPopup, GetGizmoText(ERR_unkPanePos));
                                break;
			}
                	updateArrowBut(dns);
			break;
		default:
			SetPopupMessage(&HostPopup, GetGizmoText(ERR_unkQuery));
			break;
	}
}

static Boolean 
decideScrLayout(errorList *errnum) 
{
	readHostsReturn retval;
	char *defaultDomain;
	HostList	*hosts;

	if ((retval = ReadHosts(&hosts)) == Failure){
		/* if no entries in the /etc/hosts, unmanage host list */
		etcExists = False;
		XtUnmanageChild(etcHostsScreen.controlWidget);
		OlVaFlatSetValues(viewChoice.buttonsWidget, 1,
			XtNsensitive, False, NULL);
		OlVaFlatSetValues(viewChoice.buttonsWidget, 0,
			XtNset, True, NULL);
		*errnum = noEntries;
	}else{

		/* If there are entries map the list */
		etcExists = True;
		getHosts(&hostListG,hosts,retval);
	}

	initResolvConf(&dns.resolv);
	if((dns.dnsExists = readResolvConf(dns.resolv)) ){
		dns.queryFrom = START;
		readDnsHosts(dns.resolv->domain);

		XtVaSetValues(domainName.textFieldWidget,
				XtNstring, dns.resolv->domain,NULL);
		XtAddCallback(domainName.textFieldWidget,
                                XtNverification, listSysCB, NULL);
		return True;
	}else{
		XtUnmanageChild(domainHostsScreen.controlWidget);
		OlVaFlatSetValues(viewChoice.buttonsWidget, 0,
			XtNsensitive, False, NULL);
		OlVaFlatSetValues(viewChoice.buttonsWidget, 1,
			XtNset, True, NULL);
		XtVaSetValues (viewChoice.previewWidget,
        		XtNstring,      GetGizmoText(TXT_SysList), 0);
		updateEtc();
		busyCursor(XtParent(HostPopup.shell), False);

	}
}

setDnsList(ListGizmo *listGizmo,dnsList *dns)
{
    Arg arg[1];
    int i, total, which;
    ListHead	*hp;
    char **tmp;

    /*  hp-> numfields will be zero if /etc/hosts was read to     */
	/*  validate a host before it was read to post the scrolling list. */
	hp = (ListHead *) (listGizmo->settings-> current_value);
	if (hp->list) {
		XtSetArg(arg[0],XtNuserData,&which);
		XtGetValues(listGizmo->flatList,arg,1);
		if (n_fields[which] != NULL) {
			if (n_fields[which][1] != NULL)
				free (n_fields[which][1]);
			free (n_fields[which]);
			n_fields[which] = NULL;
		}
		if (p_fields[which] != NULL) {
			if (p_fields[which][1] != NULL)
				free (p_fields[which][1]);
			free (p_fields[which]);
			p_fields[which] = NULL;
		}
		for (i=0; i<hp->size; i++) {
			tmp = (char **)hp->list[i].fields;
			if (tmp[1] != NULL) {
				free (tmp[1]);
			}
			free ((char *)(hp->list[i].fields));
		}
		free (hp->list);
	}
	hp->numFields = 2;
	if(dns) {
		hp->size = dns->domainList.count + dns->systemList.count;
		hp->list = (ListItem *)malloc(sizeof(ListItem) *hp->size);
		for (i = 0; i < dns->domainList.count; i++){
		    hp-> list[i].set = False;
		    hp-> list[i].clientData = NULL;
		    hp-> list[i].fields = (XtArgVal)malloc (
			sizeof (XtArgVal *) * hp->numFields);
		    allocList(&dns->domainList);
		    tmp = (char **) hp-> list[i].fields;
		    tmp[0] = (char *)domainPixmap;
		    tmp[1] = strdup((char *)dns->domainList.list[i].name);
		}
		total = i;
		for (i = 0; i < dns->systemList.count; i++){
		    hp-> list[total+i].set = False;
		    hp-> list[total+i].clientData = NULL;
		    hp-> list[total+i].fields = (XtArgVal)malloc (
			sizeof (XtArgVal *) * hp->numFields);
		    allocList(&dns->systemList);
		    tmp = (char **) hp-> list[total+i].fields;
		    tmp[0] = (char *)hostPixmap;
		    tmp[1] = strdup((char *)dns->systemList.list[i].name);
		}
	}else{
		hp->size = 0;
		hp->list = NULL;
	}
        XtVaSetValues (GetList(listGizmo),
                       XtNnumItems, hp->size,
                       XtNitems, hp->list,
                       (String)0);
}
void
mappLists(dnsList * pos, int num, dnsListNum wid_pos)
{
	LabelGizmo *lab;
	StaticTextGizmo *title;
	ListGizmo *list;
        int     i;

        for (i=0; i < num; i++) {
		lab = domainLists[wid_pos++].gizmo;
		title = lab->gizmos[0].gizmo;
		list = lab->gizmos[1].gizmo;
		if(pos == NULL){
			SetStaticTextGizmoText(title, " ");
			setDnsList(list, pos);
			return;
		}
		SetStaticTextGizmoText(title, pos->domain.name);
		if (lastHost) free(lastHost);
		lastHost = strdup(pos->domain.name);
		setDnsList(list, pos);
                pos = pos->next;
        }
}

/*
 * Puts the list of hosts/domains in the list gizmo to be dislplayed
 */
static void
getHosts(ListGizmo *listGizmo,HostList *hosts,readHostsReturn retval)
{
    int index;
    ListHead	*hp;
    char **tmp;

    /*  hp-> numfields will be zero if /etc/hosts was read to     */
    /*  validate a host before it was read to post the scrolling list. */
    hp = (ListHead *) (listGizmo->settings-> current_value);
    if (retval == NewList || hp-> numFields == 0) {
    	hp = (ListHead *) (listGizmo->settings-> current_value);
	if (hp->list) {
		for (index=0; index<hp->size; index++) {
			tmp = (char **)hp->list[index].fields;
			if (tmp[1] != NULL) {
				free (tmp[1]);
			}
			free ((char *)(hp->list[index].fields));
		}
		free (hp->list);
	}
	hp->numFields = 2;
	hp->size = hosts->cnt;
	hp->list = (ListItem *)MALLOC(sizeof(ListItem) *hp->size);
	for (index =  hosts->cnt - 1; index >= 0; index--)
	{
	    hp-> list[index].set = False;
	    hp-> list[index].clientData = NULL;
	    hp-> list[index].fields = (XtArgVal)MALLOC (
                sizeof (XtArgVal *) * hp->numFields);
	    tmp = (char **) hp-> list[index].fields;
	    tmp[0] = (char *)hostPixmap;
	    tmp[1] = strdup((char *)hosts->list[index].name);
	}
        XtVaSetValues ( GetList(listGizmo),
                       XtNnumItems, hp->size,
                       XtNitems, hp->list,
                       (String)0);
    }
}

/*
 * Popdown the lookup table, and put the entry selected in the text widget
 * passed by the user.
 */
static void
doApply()
{
    PopupGizmo *     popup      = &HostPopup;
    Widget          shell       = GetPopupGizmoShell(popup);
    Arg arg[1];

    BringDownPopup(shell);
    user->prevVal = strtok(strdup(SelectedHost),WHITESPACE);
    user->hostSelected = True;
    XtSetArg(arg[0],XtNstring,strtok(strdup(SelectedHost),WHITESPACE));
    XtSetValues(user->text,arg,1);
    free(SelectedHost);
    SelectedHost = NULL;
}

static void 
hostMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData * p          = (OlFlatCallData *)call_data;
    Widget	    shell	= GetPopupGizmoShell(&HostPopup);
    Arg arg[1];
    char *currHost;
    HelpInfo help;

    SetPopupMessage(&HostPopup, "");
    switch (p-> item_index)
    {
    case applyHost:
	doApply();
	return;
	break;

    case cancelHost:
	SetWMPushpinState(XtDisplay(shell), XtWindow(shell), WMPushpinIsOut);
	XtPopdown(shell);
        XtSetArg(arg[0],XtNstring,&currHost);
        XtGetValues(user->text,arg,1);
        user->prevVal = strdup(currHost);
	user->hostSelected = False;
	break;

    case hostHelp:
	help.app_title = strdup(GetGizmoText(TXT_HostTitle));
	help.title = user->help->title;
	help.filename = user->help->file;
	help.section  = user->help->section;
	PostGizmoHelp( HostPopup.shell, &help);
	break;
    default:
	break;
    }
}				/* end of hostMenuCB */


static void viewSysCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData      *pFlatData = (OlFlatCallData *) call_data;
	static Boolean view=True;


	if(pFlatData->item_index){
		XtUnmanageChild(domainHostsScreen.controlWidget);
		XtManageChild(etcHostsScreen.controlWidget);
		XtVaSetValues (viewChoice.previewWidget,
        		XtNstring,      GetGizmoText(TXT_SysList), 0);
	}else{
		XtUnmanageChild(etcHostsScreen.controlWidget);
		XtManageChild(domainHostsScreen.controlWidget);
		XtVaSetValues (viewChoice.previewWidget,
        		XtNstring,      GetGizmoText(TXT_DomList), 0);
	}
}

static void 
UnselectCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    SelectedHost = NULL;
    OlVaFlatSetValues(hostMenu.child, applyHost,
		      XtNsensitive, False, NULL);
    return;
}



/*
 * Error message to be identified by numbers as the error messages obtained
 * from nslookup are not internationalized.
 */
static void popupErrMessage(errorList errnum)
{
   switch(errnum){
	case none: return;
	case noEntries:
		popupMessage(GetGizmoText(TXT_noEntries));
		break; 
	case noDomain:
		popupMessage(GetGizmoText(TXT_noDomain));
		break; 
	case invalidHost:
		popupMessage(GetGizmoText(TXT_invalidHost));
		break; 
	default :
		popupMessage(GetGizmoText(ERR_unknown));
		break;
   }
}

/*
 * Popup the error dialog after setting the error message
 */
static void popupMessage(char *message)
{
	SetModalGizmoMessage (&errorModal, message);

        MapGizmo (ModalGizmoClass, &errorModal);
	if(errorModal.menu->child)
		XtMapWidget(errorModal.menu->child);
}

/*
 * Check for the name input in the text field, and get the domain list after
 * checking for a valid name. If a valid host name and a list of hosts are
 * obtained, dislplay that list, or else display the default list.
 */
static void listSysCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    	readHostsReturn retval;
	int errnum;
	HostList	domainHosts = { NULL, 0, 0 };

	OlVaFlatSetValues(hostMenu.child, applyHost,
		      XtNsensitive, False, NULL);
	dns.queryFrom = SHOWDOMAIN;
	ManipulateGizmo(InputGizmoClass, &domainName,
		GetGizmoValue);
	if((retval = readDnsHosts(domainName.settings->current_value)) == Failure){
		/* readDnsHosts will display it's own error message */
	}
}

/*
 * Popdown the error dialog
 */
static void errorCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	XtPopdown(errorModal.shell);
}

/*
 * Display the help messages in the footer 
 */
static void handleFocus(Widget w, XtPointer msg, XFocusChangeEvent *event)
{
	if(event->type == FocusIn)
		SetPopupMessage(&HostPopup, msg);
}


static void busyCursor(Widget w, int on)
{
    static Cursor cursor;
    XSetWindowAttributes attrs;
    Display *dpy = XtDisplay(w);
    Widget      shell = w;

    while(!XtIsShell(shell))
        shell = XtParent(shell);

    if (!cursor) /* make sure the timeout cursor is initialized */
        cursor = XCreateFontCursor(dpy, XC_watch);

    attrs.cursor = on? cursor : None;
    XChangeWindowAttributes(dpy, XtWindow(shell), CWCursor, &attrs);

    XFlush(dpy);

}

/* a call back func from toolkit when the flat list overflowed */
/* this function, which in turn calls handle_overflow to disp- */
/* lay the rest of the list					*/

static void
overflowCB(Widget wid, int which, OlFListItemsLimitExceededCD *cdp)
{
	static Boolean first = True;
	ListItem *fp;
	DmGlyphRec *    gp;
        Screen *        screen = XtScreen(wid);
	
	if (first) {
		next_fields = malloc(sizeof(XtArgVal *) * 2);
		prev_fields = malloc(sizeof(XtArgVal *) * 2);
		if ((gp = DmGetPixmap (screen, "dArrow16")) != NULL)
			next_fields[0] = (char *)gp->pix;
		next_fields[1] = GetGizmoText(TXT_nextPage);
		if ((gp = DmGetPixmap (screen, "uArrow16")) != NULL)
			prev_fields[0] = (char *)gp->pix;
		prev_fields[1] = GetGizmoText(TXT_prevPage);
		first = False;
	}
	fp = (ListItem *) (cdp->item_data)->items;
	preferred = cdp->preferred;
	total_items[which] = (cdp->item_data)->num_items;
	/* store the last item field before blasting */
	list_fields[which] = (char **) fp[cdp->preferred -1].fields;
	/* remember the list head index for the next page */
	fp[cdp->preferred -1].clientData = (XtArgVal)&fp[cdp->preferred -2];
	if (n_fields[which] == NULL) {
		n_fields[which] = malloc(sizeof(XtArgVal *) * 2);
		n_fields[which][0] = next_fields[0];
		n_fields[which][1] = strdup(next_fields[1]);
	}
	OlVaFlatSetValues (
		wid, cdp->preferred -1, XtNformatData, n_fields[which], 0
	);
        cdp->ok = TRUE;
}

/* TimeoutCursors() turns on the "watch" cursor over the 
 * application to provide feedback for the user that she's
 * going to be waiting a while before she can interact with
 * the application again.
 */

static void
TimeoutCursors(Boolean on)
{
	static int	locked;
	XEvent event;
	char	buf[MEDIUMBUFSIZE];
	Display *dpy = XtDisplay(HostPopup.shell);
	Widget button;
	static Boolean first = True;
	char domain_name[256];

	/* "locked" keeps track if we've already called
	 * the function.  This allows recursion and is
	 * necessary for most situations.
	 */

	on ? locked++:locked--;
	if (locked > 1 || locked == 1 && on == 0)
		/* already locked and we're not unlocking */
		return;

	/* doesn't matter at this point; initialize */
	stopped = False;

	if (on) {
		/*
		 * For nfs, the busycursor is on the parent.
		 */
		busyCursor(XtParent(HostPopup.shell), True);
		/* We're timing out, use a ModalGizmo to display the
		 * fact that we're busy.  If the processor is interruptable, 
		 * allow a "Stop" button. Else, remove all actions so the 
		 * user can't stop the processing.
		 */
		if (!timeoutModal.shell) {  
		        CreateGizmo (HostPopup.shell, ModalGizmoClass, 
				     &timeoutModal, 0, 0);
		} 
#ifdef EDIT_RES
		XtAddEventHandler(
			timeoutModal.shell, (EventMask) 0, True,
			_XEditResCheckMessages, NULL
		);
#endif
		if (first) {
		        strcpy(domain_name,dns.resolv->domain);
			first = False;
		} else
		        strcpy(domain_name,dns.dnsHosts->domain.name);
		sprintf(buf, GetGizmoText(INFO_dnsQuery), domain_name);
		SetModalGizmoMessage (&timeoutModal, buf);
		MapGizmo (ModalGizmoClass, &timeoutModal);

	/* Check the event queue for MapNotify/VisibilityNotify event. 
	 * XIfEvent will block until the expected event show up in the queue.
	 */
		XIfEvent(dpy, &event, TrapMapOrVis, (XPointer)timeoutModal.shell);

	/* If the event is found, then update the display (XSync and dispatch
	 * the Expose event.
	 */
		OlUpdateDisplay(timeoutModal.stext);
	}
	else { /* !on */
		/* We're turning off the timeout-cursors.
		 * Get rid of all button and keyboard events that occurred
		 * during the timeout.  The user shouldn't have done 
		 * anything during this time, so flush out button and 
		 * keypress events.
		 */
		while(XCheckMaskEvent(dpy,
		      ButtonPressMask|ButtonReleaseMask|ButtonMotionMask
		      | PointerMotionMask | KeyPressMask, &event)) {
			/* do nothing */;
		}
		XtPopdown(timeoutModal.shell);
		busyCursor(XtParent(HostPopup.shell), False);
	}
}

/* User pressed the "stop" button in dialog;
 * set global "stopped" value.
 */

static void
stop()
{
	stopped = True;
}

static Boolean
CheckForInterrupt()
{
	/* extern Widget shell; */
        Widget toplevel = HostPopup.shell;
	Display *dpy = XtDisplay(toplevel);
	Window win = XtWindow(timeoutModal.shell);
	XEvent event;

	/* Make sure all our requests get to the server */
	XFlush(dpy);
	/* Let server process all pending exposure events for us */
	while(XCheckMaskEvent(dpy, ExposureMask, &event))
		XtDispatchEvent(&event);

	/* Check the event loop for events in the dialog (Stop?) */
	while(XCheckMaskEvent(dpy,
		ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
		PointerMotionMask|KeyPressMask|KeyReleaseMask, &event)) {
		/* got an interrupt event */
		if (event.xany.window == win) 
			XtDispatchEvent(&event);
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
	String	str;
	Arg	args[1];
	char	buf[MEDIUMBUFSIZE];
	queryReturn	query=FAILED;
        Widget toplevel = HostPopup.shell;

	initDnsList(&dnsHosts1);
	dns.dnsHosts = dnsHosts1;
	dns.dnsHosts->domain.name = strdup(domain);

	/* In this routine, we set busy cursor on the main woindow,
	 * and popup a message box to tell user we are busy on getting
	 * info from tcpip thru the resolv lib.
	 * We allow user to stop the process ONLY after the completion of
	 * the query on each Name Server.  Note: During the query, user
	 * cannot interact with the gui at all since the GUI is locked
	 * by the query process.
	 */

	TimeoutCursors(True);
	CheckForInterrupt();

	XSync(XtDisplay(toplevel), False);
	dns.query = NAME_SERVERS;
	if ((ns_error = findNameServers(domain, 
				&dns.dnsHosts->nameServerList)) == 0 ) { 
		/* successful query */
		dns.query = HOSTS;
		for (n=0; n < dns.dnsHosts->nameServerList.count; n++) {
			if (CheckForInterrupt()) {
				query = INTERRUPTED;	
				break;
			}
			if ((ho_error = queryNameServers(domain,
				dns.dnsHosts->nameServerList.list[n],
				&dns.dnsHosts->domainList,
				&dns.dnsHosts->systemList)) == 0) {
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

	switch (query) {
		case OK:
			/* everthing fine here, go update GUI */
			updateDns(&dns);
			if (etcExists == True)
			       XtUnmanageChild(etcHostsScreen.controlWidget);
			XtPopup(toplevel, XtGrabNone);
	                TimeoutCursors(False);
			break;
		case INTERRUPTED:
			/* query was interrupted by user pressing "stop" */
	                TimeoutCursors(False);
/*			createMsg(toplevel, ERROR, ERR_killBySignal); */
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
			
	                TimeoutCursors(False);
			popupErrMessage(invalidHost);
			return Failure;
	}
	return NewList;
}

/*
 * Called when user double clicks or selects ok button to select the entry
 */
static void 
ExecuteCB(Widget wid, XtPointer client_data, OlFlatCallData* call_data)
{
	Arg	arg[5];
	char	buf[256];
	int       item_index = ((OlFlatCallData *)call_data)->item_index;
	ListItem	*fp = call_data->items;
	char **        fields;
	StaticTextGizmo 	*nameLabel;
	int     i;
	dnsList *cur_pos;
	readHostsReturn ret;

	fields = (char **)fp[item_index].fields;
	if(wid != hostListG.flatList){
		XtSetArg(arg[0],XtNuserData,&dns.cur_wid_pos);
		XtGetValues(wid,arg,1);
		cur_pos = dns.cur_pos;
		for (i=0; i < dns.cur_wid_pos; i++) {
			cur_pos = cur_pos->next;
		}
		dns.dnsIndex[dns.cur_wid_pos] = item_index;
        	if (isInDomainList(dns.dnsIndex[dns.cur_wid_pos], cur_pos) == TRUE) {
               		dns.queryFrom = DOUBLECLICK;
			SelectedHost = dns.dnsSelection[dns.cur_wid_pos] = strdup(
				cur_pos->domainList.list[dns.dnsIndex[dns.cur_wid_pos]].name);

       		        if ((ret = readDnsHosts(dns.dnsSelection[dns.cur_wid_pos])) == Failure)
                        	return;
		} else {
			if (strchr(fields[1], '.')) {

				SelectedHost = strdup(fields[1]);
			} else  
				if (lastHost == NULL) {
					/* format address */
				sprintf(buf,"%s.%s",fields[1],dns.cur_pos->domain.name);
				SelectedHost = strdup(buf);
			} else {
				/* we previusly selected something so
				use the same lasthost name */
				sprintf(buf,"%s.%s",fields[1],lastHost);
				SelectedHost = strdup(buf);
			}
			doApply();
		}
	} else {
		SelectedHost = strdup(fields[1]);
		doApply();
	}
	nameLabel = (StaticTextGizmo *) QueryGizmo (
                PopupGizmoClass, &HostPopup, GetGizmoGizmo, "nameLabel"
	);
	SetStaticTextGizmoText(nameLabel,strtok(strdup(SelectedHost),WHITESPACE));
	OlVaFlatSetValues(hostMenu.child, applyHost,
		      XtNsensitive, True, NULL);
}

/*
 * When an item is selected, update the selected host name at teh bottom
 * of the popup.
 */
static void 
SelectCB(Widget wid, XtPointer client_data, OlFlatCallData* call_data)
{
	Arg     arg[5];
	char	buf[256];
	int	item_index = call_data->item_index;
	int	num_items = call_data->num_items;
	ListItem	*fp = call_data->items;
	char ** 	fields;
	StaticTextGizmo	*nameLabel;
	int     which;
	ListItem *np = client_data;

	if(wid == hostListG.flatList){
		which = ETCL;
	} else {
		XtSetArg(arg[0],XtNuserData,&dns.cur_wid_pos);
		XtGetValues(wid,arg,1);
		which = dns.cur_wid_pos;
	}
		
	/* np (client_data) carries the address of the new list */
	if (np != NULL) {
		if (item_index == 0) { /* it is called by prev page items */
			fp[item_index].fields = (XtArgVal)list_fields[which];
			fp[item_index].clientData = NULL;
			XtSetArg(arg[0], XtNitems, np);
			XtSetArg(arg[1],XtNnumItems,num_items+preferred-2);
			XtSetArg(arg[2],XtNitemsTouched, TRUE);
			XtSetValues(wid, arg, 3);
		} else { /* it is called by next page items */
		/* restore the item occupied by next page item */
			fp[item_index].fields = (XtArgVal)list_fields[which];
			fp[item_index].clientData = NULL;
			/* store the one b4 last item field before blasting */
			list_fields[which] =
				(char **) fp[item_index -1].fields;
			/* remember the list head index for the prev page */
			fp[item_index -1].clientData = (XtArgVal)fp;
			if (p_fields[which] == NULL) {
				p_fields[which] =
					malloc(sizeof(XtArgVal *) * 2);
				p_fields[which][0] = prev_fields[0];
				p_fields[which][1] = strdup(prev_fields[1]);
			}
			fp[item_index -1].fields = (XtArgVal)p_fields[which];
			XtSetArg(arg[0], XtNitems, np);
			/* one for the indexing */
			XtSetArg(arg[1],
				XtNnumItems,
				total_items[which]-item_index+1
			);
			XtSetArg(arg[2],XtNitemsTouched, TRUE);
			XtSetValues(wid, arg, 3);
		}
		return;
	}
	fields = (char **)fp[item_index].fields;
	if (which != ETCL) {
		if (strchr(fields[1], '.')) {
			SelectedHost = strdup(fields[1]);
		 }else 
			if (lastHost == NULL) {
				sprintf(buf,"%s.%s",fields[1],dns.cur_pos->domain.name);
				SelectedHost = strdup(buf);
			} else {
				sprintf(buf,"%s.%s",fields[1],lastHost);
				SelectedHost = strdup(buf);
		}
	} else {
		SelectedHost = strdup(fields[1]);
	}
	nameLabel = (StaticTextGizmo *) QueryGizmo (
		PopupGizmoClass, &HostPopup, GetGizmoGizmo, "nameLabel"
	);
	SetStaticTextGizmoText(nameLabel,strtok(strdup(SelectedHost),WHITESPACE));
	OlVaFlatSetValues(hostMenu.child, applyHost,
		      XtNsensitive, True, NULL);
}

/*
 * Simulate return to main loop - dispatch awating events.  Called
 * when a callback is going to perform a time-expensive task.
 */
static Bool
TrapMapOrVis(Display * dpy, XEvent * xe, XPointer client_data)
{
	if (xe->type == MapNotify || xe->type == VisibilityNotify) {
		Window		window = XtWindow((Widget)client_data);

		XSetInputFocus(
			xe->xany.display, window, RevertToNone, CurrentTime);
		return True;
	} else {
		return False;
	}
}
