#ifndef NOIDENT
#ident	"@(#)dtadmin:dialup/message.c	1.7"
#endif

/*
 *      message - utility for notify users with messages
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>

#include <X11/Shell.h>
#include <FButtons.h>

#include <Gizmos.h>
#include <MenuGizmo.h>
#include <ModalGizmo.h>
#include "error.h"

#define TXT_MSG_CONTINUE	"message:1" FS "Continue"
#define TXT_TITLE		"message:2" FS "Message"
#define TXT_NULL_MESSAGE	"message:3" FS "Operation Succeeded"

static Widget popup;
static Widget device_popup;
static Widget CreatePopup(Widget);
static void DeviceMessageCB(Widget, XtPointer, XtPointer);
static void MessageCB(Widget, XtPointer, XtPointer);

typedef enum { MessageContinue, MessageHelp } MessageMenuItemIndex;

static MenuItems  MessageMenuItems[] =
   {
   { True, label_ok,	mnemonic_ok },
   { 0 }
   };

static MenuItems  DeviceMessageMenuItems[] =
   {
   { True, label_ok,	mnemonic_ok },
   { 0 }
   };

static MenuGizmo MessageMenu =
   { NULL, "_X_", "_X_", MessageMenuItems, MessageCB, NULL, CMD, OL_FIXEDROWS, 1, 0 };

static ModalGizmo MessagePopup = 
   { NULL, "_X_", TXT_TITLE, &MessageMenu, TXT_NULL_MESSAGE };


static MenuGizmo DeviceMessageMenu =
   { NULL, "_X_", "_X_", DeviceMessageMenuItems, DeviceMessageCB, NULL, CMD, OL_FIXEDROWS, 1, 0 };

static ModalGizmo DeviceMessagePopup = 
   { NULL, "devicemodal", TXT_TITLE, &DeviceMessageMenu, TXT_NULL_MESSAGE };

/*
 * NotifyUser
 *
 * This routine posts a simple modal dialog window with a message
 * provided arguments.
 * Ideally, this program should allow users to specify addtional buttons
 * to specify any other actions may desired.
 *
 */

extern void
DeviceNotifyUser(root, msg)
Widget root;
char  msg[];
{
   static Boolean	first_time = True;
   if (first_time) {
       first_time = False;
   	device_popup = CreateGizmo(root, ModalGizmoClass, 
			&DeviceMessagePopup, NULL, 0);
   }
   if (msg != NULL)
      SetModalGizmoMessage(&DeviceMessagePopup, msg);

   MapGizmo(ModalGizmoClass, &DeviceMessagePopup);
} /* NotifyUser */

extern void
NotifyUser(root, msg)
Widget root;
char  msg[];
{
   static Boolean	first_time = True;
   if (first_time) {
       first_time = False;
   	popup = CreateGizmo(root, ModalGizmoClass, &MessagePopup, NULL, 0);
   }
   if (msg != NULL)
      SetModalGizmoMessage(&MessagePopup, msg);

   MapGizmo(ModalGizmoClass, &MessagePopup);
} /* NotifyUser */


/*
 * MessageCB
 *
 *
 * The callback procedure is called when any of the buttons in the menu bar
 * of the MessagePopup dialog are selected.  The callback switched on the
 * index of the flat button in the menu bar and either (in the future, it sould
 * allow exitsting as well):
 *
 * Continue the task by simply returning.
 * 
 */

static void
MessageCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;

   switch(p-> item_index)
   {
      case MessageContinue:
	XtPopdown(popup);
	return;
	break;
      default:
	(void)fprintf(stderr,"default in MessageCB taken!!!\n");
   }

} /* MessageCB */

static void
DeviceMessageCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;

   switch(p-> item_index)
   {
      case MessageContinue:
	XtPopdown(device_popup);
	return;
	break;
      default:
	break;
   }

} /* DeviceMessageCB */
