#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/menu.c	1.15"
#endif

/*
 * Module:	dtadmin:nfs  Graphical Administration of Network File Sharing
 * File:        menu.c
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <Xol/buffutil.h>
#include <Xol/textbuff.h>

#include <Xol/OpenLook.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/ModalGizmo.h>
#include <Gizmo/BaseWGizmo.h>

#include "text.h"

extern void nfsEditCB();
extern void nfsViewCB();
extern void nfsHelpCB();

/*
 *
 #    #  ######  #    #  #    #   ####
 ##  ##  #       ##   #  #    #  #
 # ## #  #####   # #  #  #    #   ####
 #    #  #       #  # #  #    #       #
 #    #  #       #   ##  #    #  #    #
 #    #  ######  #    #   ####    ####
 */
extern void RemotePropertyCB();
extern void LocalPropertyCB();
extern void nfsActionsCB();


MenuItems  ActionsMenuItems[] =
{
   { False, TXT_Mount,       CHR_Mount, NULL, NULL, 0, False },
   { False, TXT_UnMount,     CHR_UnMount, NULL, NULL, 0, False },
   { False, TXT_Advertise,   CHR_Advertise, NULL, NULL, 0, False },
   { False, TXT_UnAdvertise, CHR_UnAdvertise, NULL, NULL, 0, False },
   { True,  TXT_Status,      CHR_Status, NULL, NULL, 0, False },
   { True,  TXT_Exit,        CHR_Exit, NULL, NULL, 0, False },
   { NULL }
};


MenuItems  EditMenuItems[] =
{
   { True,  TXT_New,        CHR_New},
   { False, TXT_Delete,     CHR_Delete },
   { False, TXT_Properties, CHR_Properties },
   { NULL }
};

static MenuItems  ViewMenuItems[] = 
{
   { True, TXT_Local,	CHR_Local },
   { False, TXT_Remote,	CHR_Remote },
   { NULL }
};

static MenuItems  HelpMenuItems[] =
{
   { True, TXT_FileSharingW,  CHR_FileSharingW },
   { True, TXT_TOC,           CHR_TOC },
   { True, TXT_HelpDesk,      CHR_HelpDesk },
   { NULL }
};
extern MenuGizmo      ActionsMenu =
   { NULL, "Actions", NULL, ActionsMenuItems, nfsActionsCB };
extern MenuGizmo      EditMenu =
   { NULL, "Edit",   NULL,   EditMenuItems, nfsEditCB };

extern MenuGizmo      ViewMenu =
   { NULL, "View",   NULL,   ViewMenuItems, nfsViewCB };

static MenuGizmo      HelpMenu =
   { NULL, "Help",   NULL,   HelpMenuItems, nfsHelpCB };

static MenuItems  BarMenuItems[] =
{
   { True, TXT_Actions,  CHR_Actions,  (char *)&ActionsMenu, NULL, 0, False },
   { True, TXT_View,     CHR_View,     (char *)&ViewMenu, NULL, 0, False },
   { True, TXT_Resource, CHR_Resource, (char *)&EditMenu, NULL, 0, False },
   { True, TXT_HelpMenu, CHR_HelpMenu, (char *)&HelpMenu, NULL, 0, False },
   { NULL }
};

extern MenuGizmo      MenuBar =
   { NULL,      "MenuBar", "MenuBar", BarMenuItems, NULL, NULL, CMD,
	 OL_FIXEDROWS, 1, OL_NO_ITEM }; 
