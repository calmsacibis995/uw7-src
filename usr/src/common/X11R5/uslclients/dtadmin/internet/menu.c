#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:internet/menu.c	1.14"
#endif

#include <Xm/MainW.h>
#include <Xm/PanedW.h>
#include <Xm/RowColumn.h>
#include <Xm/DrawingA.h>
#include <Xm/CascadeBG.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ScrolledW.h>
#include <Xm/SeparatoG.h>
#include "lookup.h"
#include "lookupG.h"
#include "util.h"
#include "inet.h"
#include "inetMsg.h"

#include <Dt/Desktop.h>

extern void exit();
extern void systemsListCB();
extern void dnsListCB();
extern void createNameServerAccess();
extern void createNisAccess();
extern void createLocalAccess();
extern void createUucpTransfer();
extern void createConfigLocal();
extern void createRouterSetup();
extern void createAddNewSystem();
extern void createProperty();
extern void createCopyFolder();
extern void createSearch();
extern void firstCB();
extern void lastCB();
extern void deleteCB();
extern void leftArrowCB();
extern void rightArrowCB();
extern void copySystemList();
extern void DisplayHelp(Widget, HelpText *);
extern char *	mygettxt(char *);
extern void exitMainCB();

void tmp();
void helpCB(Widget, XtPointer, XtPointer);


MenuItem actions_menu[] = {
    { MENU_rua, &xmPushButtonGadgetClass, MNE_actionsRUA, NULL, NULL,
        (void(*)())createLocalAccess, (XtPointer)1, (MenuItem *)NULL,
	True, NULL },
    { MENU_uucp, &xmPushButtonGadgetClass, MNE_actionsUUCP, NULL, NULL,
        (void(*)())createUucpTransfer, (XtPointer)2, (MenuItem *)NULL,
	True, NULL },
    { MENU_dns, &xmPushButtonGadgetClass, MNE_actionsDNS, NULL, NULL,
        (void(*)())createNameServerAccess, (XtPointer)4, (MenuItem *)NULL,
	True, NULL },
    { MENU_nis, &xmPushButtonGadgetClass, MNE_actionsNIS, NULL, NULL,
        (void(*)())createNisAccess, (XtPointer)4, (MenuItem *)NULL,
	True, NULL },
    { MENU_route, &xmPushButtonGadgetClass, MNE_actionsROUTE, NULL, NULL,
        (void(*)())createRouterSetup, (XtPointer)3, (MenuItem *)NULL,
	True, NULL },
    { "Sep", &xmSeparatorGadgetClass, NULL, NULL, NULL, NULL, NULL, NULL },
    { MENU_exit, &xmPushButtonGadgetClass, MNE_actionsEXIT, NULL, NULL,
        (void(*)())exitMainCB, (XtPointer)4, (MenuItem *)NULL, True, NULL },
    NULL,
};

MenuItem system_menu[] = {
    { MENU_sysNew, &xmPushButtonGadgetClass, MNE_systemNEW, "Meta<Key>N", ACC_MetaN,
        (void(*)())createAddNewSystem, NULL, (MenuItem *)NULL,
	True, NULL },
    { MENU_sysCTF, &xmPushButtonGadgetClass, MNE_systemCTF, "Meta<Key>C", ACC_MetaC,
	 (void(*)())createCopyFolder, NULL, (MenuItem *)NULL,
	True, NULL },
    { MENU_sysCTSL, &xmPushButtonGadgetClass, MNE_systemCTSL, "Meta<Key>O", ACC_MetaO,
        (void(*)())copySystemList, NULL, (MenuItem *)NULL,
	True, NULL },
    { MENU_sysDel, &xmPushButtonGadgetClass, MNE_systemDEL, "Meta<Key>D", ACC_MetaD,
        (void(*)())deleteCB, NULL, (MenuItem *)NULL, True, NULL },
    { "Sep", &xmSeparatorGadgetClass, NULL, NULL, NULL, NULL, NULL, NULL },
    { MENU_sysProp, &xmPushButtonGadgetClass, MNE_systemPROP, "Meta<Key>P", ACC_MetaP,
        (void(*)())createProperty, NULL, (MenuItem *)NULL,
	True, NULL},
    NULL,
};

MenuItem view_menu[] = {
    { MENU_viewSL, &xmPushButtonGadgetClass, MNE_viewSYS, NULL, NULL,
        (void(*)())systemsListCB, NULL, (MenuItem *)NULL, True, NULL },
    { MENU_viewDL,  &xmPushButtonGadgetClass, MNE_viewDOM, NULL, NULL,
        (void(*)())dnsListCB, NULL, (MenuItem *)NULL, True, NULL },
    { "Sep", &xmSeparatorGadgetClass, NULL, NULL, NULL, NULL, NULL, NULL },
    { MENU_viewStL, &xmPushButtonGadgetClass, MNE_viewLEFT, NULL, NULL,
	(void(*)())leftArrowCB, NULL, (MenuItem *)NULL, False, NULL },
    { MENU_viewStR, &xmPushButtonGadgetClass, MNE_viewRIGHT, NULL, NULL,
	(void(*)())rightArrowCB, NULL, (MenuItem *)NULL, False, NULL },
    NULL,
};

MenuItem find_menu[] = {
    { MENU_findSearch,  &xmPushButtonGadgetClass, MNE_findSEARCH, NULL, NULL,
        (void(*)())createSearch, NULL, (MenuItem *)NULL, True, NULL },
    { MENU_findFirst, &xmPushButtonGadgetClass, MNE_findFIRST, NULL, NULL,
        (void(*)())firstCB, NULL, (MenuItem *)NULL, True, NULL },
    { MENU_findLast,  &xmPushButtonGadgetClass, MNE_findLAST, NULL, NULL,
        (void(*)())lastCB, NULL, (MenuItem *)NULL, True, NULL },
    NULL,
};

HelpText AppHelp = { NULL, NULL, "10" };
HelpText TOCHelp = { NULL, NULL, 0 };

MenuItem help_menu[] = {
    { MENU_helpIS, &xmPushButtonGadgetClass, MNE_helpINT, NULL, NULL,
        (void(*)())helpCB, (XtPointer)&AppHelp, (MenuItem *)NULL, True, NULL },
    { MENU_helpTOC,  &xmPushButtonGadgetClass, MNE_helpTOC, NULL, NULL,
        (void(*)())helpCB, (XtPointer)&TOCHelp, (MenuItem *)NULL, True, NULL },
    { MENU_helpHD,  &xmPushButtonGadgetClass, MNE_helpHELP, NULL, NULL,
        (void(*)())helpCB, NULL, (MenuItem *)NULL, True, NULL },
    NULL,
};

MenuItem inet_menus[] = {
    { MENUBAR_actions, &xmCascadeButtonGadgetClass, MNE_actions, NULL, NULL,
        0, 0, actions_menu, True, NULL },
    { MENUBAR_system, &xmCascadeButtonGadgetClass, MNE_system, NULL, NULL,
        0, 0, system_menu, True, NULL },
    { MENUBAR_view, &xmCascadeButtonGadgetClass, MNE_view, NULL, NULL,
        0, 0, view_menu, True, NULL },
    { MENUBAR_find, &xmCascadeButtonGadgetClass, MNE_find, NULL, NULL,
        0, 0, find_menu, True, NULL },
    { MENUBAR_help, &xmCascadeButtonGadgetClass, MNE_help, NULL, NULL,
        0, 0, help_menu, True, NULL },
    NULL,
};

void
helpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
 	DisplayHelp(w, (HelpText *)client_data);
}

Widget
BuildMenu(parent, menu_type, menu_title, menu_mnemonic, items)
Widget parent;
int menu_type;
char *menu_title, *menu_mnemonic;
MenuItem *items;
{
    Widget menu, cascade;
    int i;
    XmString str;
    unsigned char *	mne;
    KeySym		ks;

    if (menu_type == XmMENU_PULLDOWN || menu_type == XmMENU_OPTION)
        menu = XmCreatePulldownMenu(parent, "_pulldown", NULL, 0);
    else if (menu_type == XmMENU_POPUP)
        menu = XmCreatePopupMenu(parent, "_popup", NULL, 0);
    else {
        XtWarning("Invalid menu type passed to BuildMenu()");
        return NULL;
    }

    /* Pulldown menus require a cascade button to be made */
    if (menu_type == XmMENU_PULLDOWN) {
        str = XmStringCreateLocalized(mygettxt(menu_title));
        cascade = XtVaCreateManagedWidget(
	    menu_title,
            xmCascadeButtonGadgetClass, parent,
            XmNsubMenuId,   menu,
            XmNlabelString, str,
            NULL
	);
        XmStringFree(str);
	mne = (unsigned char *)mygettxt(menu_mnemonic);
	if (
	    mne != NULL &&
	    (((ks = XStringToKeysym((char *)mne)) != NoSymbol) ||
	    ((ks = (KeySym)(mne[0])))) &&
	    strchr(mygettxt(menu_title), (char)(ks & 0xff)) != NULL
	) {
	    XtVaSetValues(cascade, XmNmnemonic, ks, NULL);
	}
    } else if (menu_type == XmMENU_OPTION) {
        /* Option menus are a special case, but not hard to handle */
        Arg args[2];
        str = XmStringCreateLocalized(mygettxt(menu_title));
        XtSetArg(args[0], XmNsubMenuId, menu);
        XtSetArg(args[1], XmNlabelString, str);
        /* This really isn't a cascade, but this is the widget handle
         * we're going to return at the end of the function.
         */
        cascade = XmCreateOptionMenu(parent, menu_title, args, 2);
        XmStringFree(str);
    }

    /* Now add the menu items */
    for (i = 0; items[i].label != NULL; i++) {
        /* If subitems exist, create the pull-right menu by calling this
         * function recursively.  Since the function returns a cascade
         * button, the widget returned is used..
         */
        if (items[i].subitems)
            if (menu_type == XmMENU_OPTION) {
                XtWarning("You can't have submenus from option menu items.");
                continue;
            } else 
                items[i].handle = BuildMenu(menu, XmMENU_PULLDOWN,
                    items[i].label, items[i].mnemonic, items[i].subitems);
		
        else {
	    XmString	xms = XmStringCreateLocalized(mygettxt(items[i].label));
            items[i].handle = XtVaCreateManagedWidget(items[i].label,
                *items[i].class, menu,
		XmNlabelString, xms,
		NULL);
	    XmStringFree(xms);
	}

	if (items[i].sensitivity == False)
		XtVaSetValues(items[i].handle, XmNsensitive, False, NULL);

        /* Whether the item is a real item or a cascade button with a
         * menu, it can still have a mnemonic.
         */
        if (items[i].mnemonic) {
	    mne = (unsigned char *)mygettxt(items[i].mnemonic);
	    if (
		mne != NULL &&
		(((ks = XStringToKeysym((char *)mne)) != NoSymbol) ||
		((ks = (KeySym)(mne[0])))) &&
		strchr(items[i].label, (char)(ks & 0xff)) != NULL
	    ) {
            	XtVaSetValues(items[i].handle, XmNmnemonic, ks, NULL);
	    }
	}

        /* any item can have an accelerator, except cascade menus. But,
         * we don't worry about that; we know better in our declarations.
         */
        if (items[i].accelerator) {
            str = XmStringCreateLocalized(mygettxt(items[i].accel_text));
            XtVaSetValues(items[i].handle,
                XmNaccelerator, items[i].accelerator,
                XmNacceleratorText, str,
                NULL);
            XmStringFree(str);
        }
        if (items[i].callback)
            XtAddCallback(items[i].handle,
                (items[i].class == &xmToggleButtonWidgetClass ||
                 items[i].class == &xmToggleButtonGadgetClass)?
                    XmNvalueChangedCallback : 
                    XmNactivateCallback,     
                items[i].callback, items[i].callback_data);
    }

    /* for popup menus, just return the menu; pulldown menus, return
     * the cascade button; option menus, return the thing returned
     * from XmCreateOptionMenu().  This isn't a menu, or a cascade button!
     */
    return menu_type == XmMENU_POPUP? menu : cascade;
}

void
tmp(widget, color)
Widget widget;
char *color;
{
    printf(" Test function \n");
}

createMenu(parent)
Widget parent;
{
	Widget tmp;
	int i;

	hi.net.common.menu = &inet_menus[0];
	hi.net.common.menubar = XmCreateMenuBar(parent, "menubar", NULL, 0);
	for (i = 0; inet_menus[i].label; i++){
           tmp = BuildMenu(hi.net.common.menubar, XmMENU_PULLDOWN, inet_menus[i].label, 
			inet_menus[i].mnemonic, inet_menus[i].subitems);
        }
	XtVaSetValues(hi.net.common.menubar, XmNmenuHelpWidget, tmp, 
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		NULL);

	XtManageChild(hi.net.common.menubar);
}

