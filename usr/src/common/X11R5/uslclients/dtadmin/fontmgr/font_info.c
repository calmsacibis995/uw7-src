#ident	"@(#)dtadmin:fontmgr/font_info.c	1.9"
/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       font_info.c
 */

#include <stdio.h>
#include <Intrinsic.h>
#include <StringDefs.h>
#include <OpenLook.h>
#include <PopupWindo.h>
#include <Caption.h>
#include <FButtons.h>
#include <StaticText.h>
#include <Gizmos.h>
#include <MenuGizmo.h>
/*#include <ModalGizmo.h>*/
#include <PopupGizmo.h>
#include <RubberTile.h>
/*#include <InputGizmo.h>*/

#include <fontmgr.h>
#define Default75dpiFont "-adobe-courier-bold-r-normal--*-120-75-75-m-*-*-*"
#define Default100dpiFont "-adobe-courier-bold-r-normal-*-120-100-100-m-*-*-*"
#define DefaultOutlineFont "-adobe-courier-bold-r-normal--0-0-0-0-m-0-*-*"




/*
 * external data
 */

extern void	callRegisterHelp(Widget, char *, char *);
extern Widget app_shellW;
extern Widget base_shell;
static void CancelCB();
extern void HelpCB();

static HelpInfo help_info = { 0, 0, HELP_PATH, TXT_HELP_INFO_RESOLUTION};

static MenuItems info_menu_item[] = {
{ TRUE,TXT_OK,ACCEL_NOTICE_OK,0, CancelCB, 0 },
{ TRUE,TXT_HELP_DDD,  ACCEL_DELETE_HELP  ,0, HelpCB, (char *)&help_info },
{ NULL }
};
static MenuGizmo info_menu = {0, "im", "im", info_menu_item,
	0,0 };
static PopupGizmo Popup = {0, "dp", TXT_SHOW_DPI, (Gizmo)&info_menu,
	0, 0, 0, 0, 0, 0, 100 };
 
void CreateShowDPI(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{

	view_type *view = (view_type *) client_data;
        static Widget popup , upper;
	static XFontStruct *font75dpi=NULL, *font100dpi=NULL, 
	*font_outline=NULL;
    	char textmsg2[1024];
    	char textmsg[1024];
    	int n;
    	Arg largs[20];

	if (!font75dpi)
	font75dpi  = XLoadQueryFont(XtDisplay(app_shellW), Default75dpiFont);
	if (!font100dpi)
	font100dpi = XLoadQueryFont(XtDisplay(app_shellW), Default100dpiFont);
	if (!font_outline)
	font_outline = XLoadQueryFont(XtDisplay(app_shellW), DefaultOutlineFont);
    /* if popup doesn't exist, then create it */
    if (!popup) {
        popup = CreateGizmo(app_shellW, PopupGizmoClass,
                              &Popup, NULL, 0);

        XtVaGetValues( popup, XtNupperControlArea, &upper, 0);


	
      	n = 0;
	sprintf(textmsg, GetGizmoText(TXT_INFO_RESOLUTION),
			view->resx, view->resy, view->dimx, view->dimy);


        XtVaCreateManagedWidget("staticText", staticTextWidgetClass, upper,
		XtNstring, textmsg,
		XtNalignment, OL_LEFT,
                (String) 0);

	sprintf(textmsg, GetGizmoText(TXT_INFO_MESSAGE), view->resy);
        XtVaCreateManagedWidget("staticText", staticTextWidgetClass, upper,
		XtNstring, textmsg,
		XtNalignment, OL_LEFT,
                (String) 0);

	if (font75dpi != NULL)
        XtVaCreateManagedWidget("staticText", staticTextWidgetClass, upper,
		XtNstring, (GetGizmoText(TXT_INFO_75MSG)),
		XtNfont, font75dpi,
		XtNalignment, OL_CENTER,
                (String) 0);

	if (font100dpi != NULL)
        XtVaCreateManagedWidget("staticText", staticTextWidgetClass, upper,
		XtNstring, (GetGizmoText(TXT_INFO_100MSG)),
		XtNfont, font100dpi,
		XtNalignment, OL_CENTER,
                (String) 0);
	/* do below only is IBM Courier Bold outline exists on system */

	if (font_outline != NULL) {
		sprintf(textmsg, GetGizmoText(TXT_INFO_1), view->resy);
        	XtVaCreateManagedWidget("staticText", staticTextWidgetClass, upper,
			XtNstring, textmsg,
			XtNalignment, OL_LEFT,
                	(String) 0);
        	XtVaCreateManagedWidget("staticText", staticTextWidgetClass, upper,
			XtNstring, (GetGizmoText(TXT_INFO_RENDERED)),
			XtNfont, font_outline,
			XtNalignment, OL_CENTER,
                	(String) 0);
	
		}
	}

	callRegisterHelp(popup, ClientName, TXT_HELP_INFO_RESOLUTION);
	MapGizmo(PopupGizmoClass, &Popup);
}  /* end of StaticTextCB() */


static void
CancelCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{

    BringDownPopup((Widget) _OlGetShellOfWidget(w));
    callRegisterHelp(app_shellW, ClientName, TXT_HELP_MAIN_MENU);

} /* end of CancelCB */



