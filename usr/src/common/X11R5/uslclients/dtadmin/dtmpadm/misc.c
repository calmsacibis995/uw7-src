#ifndef NOIDENT
#ident	"@(#)misc.c	1.4"
#endif

/* build_option.c -- The final version of BuildMenu() is used to
 * build popup, option, pulldown -and- pullright menus.  Menus are
 * defined by declaring an array of MenuItem structures as usual.
 */
#include <Xm/DialogS.h>
#include <Xm/MainW.h>
#include <Xm/PanedW.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/DrawingA.h>
#include <Xm/CascadeBG.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/LabelG.h>
#include <Xm/TextF.h>
#include "ProcSetup.h"
#include "proc_msgs.h"


Boolean
_IsOwner(char *adm_name)
{
	char	buf[BUF_SIZE];
	sprintf(buf, "/sbin/tfadmin -t %s 2>/dev/null", adm_name);
	return(system(buf)==0);
}

char *
mygettxt(char * label)
{
	char	*p;
	char	c;
	
	if (label == NULL)
		return(FALSE);
	for (p = label; *p; p++)
		if (*p == '\001') {
			c = *p;
			*p++ = 0;
			label = (char *)gettxt(label, p);
			*--p = c;
			break;
		}
		
	return(label);
}

/* Build popup, option and pulldown menus, depending on the menu_type.
 * It may be XmMENU_PULLDOWN, XmMENU_OPTION or  XmMENU_POPUP.  Pulldowns
 * return the CascadeButton that pops up the menu.  Popups return the menu.
 * Option menus are created, but the RowColumn that acts as the option
 * "area" is returned unmanaged. (The user must manage it.)
 * Pulldown menus are built from cascade buttons, so this function
 * also builds pullright menus.  The function also adds the right
 * callback for PushButton or ToggleButton menu items.
 */
Widget
BuildMenu(parent, menu_type, menu_title, menu_mnemonic, items)
Widget parent;
int menu_type;
char *menu_title, *menu_mnemonic;
MenuItem *items;
{
    Widget 		menu, cascade, widget;
    int 		i;
    XmString 		str;
    unsigned char*	mne;
    KeySym	ks;

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
            NULL);
        XmStringFree(str);
	mne = (unsigned char *)mygettxt(menu_mnemonic);
	if (
		mne != NULL &&
		(((ks = XStringToKeysym((char *)mne)) != NoSymbol) ||
		((ks = (KeySym)(mne[0])))) &&
		strchr(menu_mnemonic, (char)(ks & 0xff)) != NULL
	) {
		XtVaSetValues(cascade, XmNmnemonic, ks, NULL);
	}
    } else if (menu_type == XmMENU_OPTION) {
        /* Option menus are a special case, but not hard to handle */
        Arg args[2];
        str = XmStringCreateSimple(mygettxt(menu_title));
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
                items[i].widget = BuildMenu(menu, XmMENU_PULLDOWN,
                    items[i].label, items[i].mnemonic, items[i].subitems);
        else {
	    XmString	xms = XmStringCreateLocalized(mygettxt(items[i].label));
            items[i].widget = XtVaCreateManagedWidget(items[i].label,
                *items[i].class, menu,
		XmNlabelString, xms,
                NULL);
	    XmStringFree(xms);
	}

        /* Whether the item is a real item or a cascade button with a
         * menu, it can still have a mnemonic.
         */
        if (items[i].mnemonic) {
		mne = (unsigned char *)mygettxt(items[i].mnemonic);
		if (
			mne != NULL &&
			(((ks = XStringToKeysym((char *)mne)) != NoSymbol) ||
			strchr(items[i].label, (char)(ks & 0xff)) != NULL)
		) {
		    XtVaSetValues(items[i].widget, XmNmnemonic, ks, NULL);
		}
	}

        /* any item can have an accelerator, except cascade menus. But,
         * we don't worry about that; we know better in our declarations.
         */
        if (items[i].accelerator) {
            str = XmStringCreateSimple(items[i].accel_text);
            XtVaSetValues(widget,
                XmNaccelerator, items[i].accelerator,
                XmNacceleratorText, str,
                NULL);
            XmStringFree(str);
        }

        if (items[i].callback)
            XtAddCallback(items[i].widget,
                (items[i].class == &xmToggleButtonWidgetClass ||
                 items[i].class == &xmToggleButtonGadgetClass)?
                    XmNvalueChangedCallback : /* ToggleButton class */
                    XmNactivateCallback,      /* PushButton class */
                items[i].callback, items[i].callback_data);
    }

    /* for popup menus, just return the menu; pulldown menus, return
     * the cascade button; option menus, return the thing returned
     * from XmCreateOptionMenu().  This isn't a menu, or a cascade button!
     */
    return menu_type == XmMENU_POPUP? menu : cascade;
}


Widget
CreateActionArea(parent, actions, num_actions)
Widget parent;
ActionAreaItem *actions;
int num_actions;
{
    Widget action_area, widget;
    int i;
    char	tmplabel[256];
    char	*mygettxt();

    action_area = XtVaCreateWidget("action_area", xmFormWidgetClass, parent,
        XmNfractionBase, TIGHTNESS*num_actions - 1,
        XmNleftOffset,   10,
        XmNrightOffset,  10,
        NULL);

    for (i = 0; i < num_actions; i++) {
	sprintf(tmplabel, mygettxt(actions[i].label));	
        widget = XtVaCreateManagedWidget(tmplabel,
            xmPushButtonWidgetClass, action_area,
            XmNleftAttachment,       i? XmATTACH_POSITION : XmATTACH_FORM,
            XmNleftPosition,         TIGHTNESS*i,
            XmNtopAttachment,        XmATTACH_FORM,
            XmNbottomAttachment,     XmATTACH_FORM,
            XmNrightAttachment,
                    i != num_actions-1? XmATTACH_POSITION : XmATTACH_FORM,
            XmNrightPosition,        TIGHTNESS*i + (TIGHTNESS-1),
            XmNshowAsDefault,        i == 0,
            XmNdefaultButtonShadowThickness, 1,
            NULL);
        if (actions[i].callback)
            XtAddCallback(widget, XmNactivateCallback,
                actions[i].callback, actions[i].data);
        if (i == 0) {
            /* Set the action_area's default button to the first widget
             * created (or, make the index a parameter to the function
             * or have it be part of the data structure). Also, set the
             * pane window constraint for max and min heights so this
             * particular pane in the PanedWindow is not resizable.
             */
            Dimension height, h;
            XtVaGetValues(action_area, XmNmarginHeight, &h, NULL);
            XtVaGetValues(widget, XmNheight, &height, NULL);
            height += 2 * h;
            XtVaSetValues(action_area,
                XmNdefaultButton, widget,
                XmNpaneMaximum,   height,
                XmNpaneMinimum,   height,
                NULL);
        }
    }

    XtManageChild(action_area);

    return action_area;
}

