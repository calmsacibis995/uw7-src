/*		copyright	"%c%" 	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/dtmessage.c	1.11"
#endif

/*
 *      dtmessage - utility for notify users with messages
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

#define FormalClientName   "dtmessage:1" FS "Message"

#define ClientName         "dtmessage"
#define ClientClass        "DTmessage"

#define TXT_NULL_MESSAGE	"dtmessage:2" FS "Operation Succeeded"
#define TXT_MSG_EXIT		"dtmessage:3" FS "Exit"
#define TXT_MSG_YES		"dtmessage:4" FS "YES"
#define TXT_MSG_NO		"dtmessage:5" FS "NO"
#define TXT_NOTICE		"dtmessage:6" FS "Are you sure you want to terminate all the tasks?"
#define TXT_TITLE		"dtmessage:7" FS "Message"
#define TXT_MN_EXIT		"dtmessage:8" FS "E"
#define TXT_MN_YES		"dtmessage:9" FS "Y"
#define TXT_MN_NO		"dtmessage:10" FS "N"
#define TXT_MN_OK		"dtmessage:11" FS "O"
#define TXT_MSG_OK		"dtmessage:12" FS "OK"

static void CreatePopup(Widget);
static void CreateExitPopup(Widget);
static void MessageCB(Widget, XtPointer, XtPointer);
static void ExitMainCB(Widget, XtPointer, XtPointer);

static void
ExitConfirmCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	ExitMainCB(wid, 0, 0);
        CreateExitPopup(wid);
}

static void
CancelCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
        XtPopdown ((Widget)_OlGetShellOfWidget (wid));
	ExitMainCB(wid, 0, 0);
}


typedef struct _applicationResources
   {
   Boolean      warnings;
   int          beepVolume;
   Pixel        DefaultForeground;
   Pixel        DefaultBackground;
   } ApplicationResources;

ApplicationResources MessageResources;

static XtResource resources[] =
   {
   { "warnings", "warnings", XtRBoolean, sizeof(Boolean),
     (Cardinal) &MessageResources.warnings, 
     XtRString, (XtPointer)"false" },
   { "beepVolume", "beepVolume", XtRInt, sizeof(int),
     (Cardinal) &MessageResources.beepVolume, 
     XtRString, "0" },
   { "TextFontColor", "TextFontColor", XtRPixel, sizeof(Pixel),
     (Cardinal) &MessageResources.DefaultForeground, XtRString, "black" },
   { "Background", "BackGround", XtRPixel, sizeof(Pixel),
     (Cardinal) &MessageResources.DefaultBackground, XtRString, "white" },
   };

typedef enum { MessageExit } MessageMenuItemIndex;

static MenuItems  MessageMenuItems[] =
   {
   { True, TXT_MSG_OK, TXT_MN_OK},
   { 0 }
   };

static MenuGizmo MessageMenu =
   { NULL, "_X_", "_X_", MessageMenuItems, MessageCB, NULL, CMD, OL_FIXEDROWS, 1, 0 };

static ModalGizmo MessagePopup = 
   { NULL, "_X_", TXT_TITLE, &MessageMenu, TXT_NULL_MESSAGE };

static MenuItems exitConfirmItems[] = {
    {True, TXT_MSG_YES,	TXT_MN_YES,  NULL,   NULL},
    {True, TXT_MSG_NO, TXT_MN_NO,  NULL,   CancelCB},
    {NULL, NULL,                (XtArgVal)NULL, NULL,   NULL}
};

static MenuGizmo exitMain = {
        NULL,                   /* help         */
        "",                     /* name         */
        "No Change",            /* title        */
        exitConfirmItems,       /* items        */
        0,                      /* function     */
        NULL,                   /* client_data  */
        CMD,                    /* buttonType   */
        OL_FIXEDROWS,           /* layoutType   */
        1,                      /* measure      */
        1                       /* default Item */
};

static ModalGizmo ExitMaindNotice = {
    NULL,
    "exitMain",
    "Exit: Warning",
    &exitMain,
    NULL
};

static void
ExitMainCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	exit(2);
}

static void
CreateExitPopup (Widget parent)
{
        static ModalGizmo *    gp;
	static Boolean  first_time = TRUE;

	if (first_time) {
		ExitMaindNotice.message = TXT_NOTICE;
		exitConfirmItems[0].function = ExitMainCB;
		gp = CopyGizmo (ModalGizmoClass, &ExitMaindNotice);
		CreateGizmo (parent, ModalGizmoClass, gp, 0, 0);
		XtAddCallback (
			GetModalGizmoShell(gp), XtNpopdownCallback,
			CancelCB, (XtPointer)0
		);
        }
        MapGizmo (ModalGizmoClass, gp);
}


/*
 * main
 *
 * This client posts a simple modal dialog window with a message
 * provided either by stdin or by the command line's arguments.
 * The routine is designed to be a generic mechanism for interfaces
 * in the UNIX SHELL level and is used by the Desktop UNIX System dtuuto
 * to handle the file transfer.
 * Ideally, this program should allow users to specify addtional buttons
 * to specify any other actions may desired.
 *
 */

main(argc, argv)
int    argc;
char * argv[];
{
   Widget root;

   root = InitializeGizmoClient(ClientName, ClientClass,
      FormalClientName,
      NULL, NULL,
      NULL, 0,
      &argc, argv,
      NULL,
      NULL, resources, XtNumber(resources), NULL, 0, NULL, NULL, NULL);

   if (argc == 2)
      MessagePopup.message = (char *)argv[1];
   else
      /* the hook here also allow the message taken from a file */
      /* for now, stdin is enough */
      MessagePopup.message = (char *)read_file ("-");

   if (MessagePopup.message == NULL)
      exit (0);

   CreatePopup(root);

   XtMainLoop();

} /* main() */

/*
 * CreatePopup
 *
 * This procedure creates the MessagePopup dialog window and maps it.
 *
 */

static void 
CreatePopup(Widget Shell)
{

   (void)CreateGizmo(Shell, ModalGizmoClass, &MessagePopup, NULL, 0);

   MapGizmo(ModalGizmoClass, &MessagePopup);

} /* end of CreatePopup */
/*
 * MessageCB
 *
 *
 * The callback procedure is called when any of the buttons in the menu bar
 * of the MessagePopup dialog are selected.  The callback switched on the
 * index of the flat button in the menu bar and either (in the future, it should
 * include other choices.):
 * Exit the task by simply exiting with exit code of ``2''.
 * .LE
 * 
 */

static void
MessageCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;

   switch(p-> item_index)
   {
      case MessageExit:
         ExitMainCB(w, 0, 0);
         break;
      default:
         (void)fprintf(stderr,"default in MessageCB taken!!!\n");
   }

} /* MessageCB */
