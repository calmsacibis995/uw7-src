/*		copyright	"%c%" 	*/

#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:floppy/medwindow.c	1.2"
#endif

#include "media.h"
#include <Gizmo/InputGizmo.h>

#include <Dt/Desktop.h>
#include <Dt/DtDTMMsg.h>

static void
WindowManagerEventHandler(Widget wid, XtPointer client_data, XtPointer call_data)
{
	OlWMProtocolVerify *    p = (OlWMProtocolVerify *)call_data;

        switch (p->msgtype) {
        case OL_WM_DELETE_WINDOW:
        case OL_WM_SAVE_YOURSELF:
                exit(0);
	default:
		OlWMProtocolAction(wid, p, OL_DEFAULTACTION);
		break;
	}
}

 /*
 *	bastardized CreateBaseGizmo; replace with a "correct" initialization
 *	of the base window by passing in a gizmo array (if I can figure it out)
 */

Widget 
CreateMediaWindow(Widget parent, BaseWindowGizmo *gizmo, Arg *args, int num, Widget *w_action)
{
	Pixmap		PixmapOfFile();
	MenuGizmo	*menu	= (MenuGizmo *)gizmo->menu;
	Widget		menuparent;
	Widget		w_ctl;
	int		i;
	int		error_percent;
	Pixmap		icon_pixmap;
	int		width;
	int		height;
	char		*str;

	XtSetArg(arg[0], XtNborderWidth,       0);
	XtSetArg(arg[1], XtNtranslations,      XtParseTranslationTable(""));
	XtSetArg(arg[2], XtNgeometry,          "32x32");
	XtSetArg(arg[3], XtNmappedWhenManaged, False);
	if (gizmo->icon_pixmap && *(gizmo->icon_pixmap)) {
		gizmo->icon_shell = 
		XtCreateApplicationShell("_X_", vendorShellWidgetClass, arg, 4);

		XtRealizeWidget(gizmo->icon_shell);

		icon_pixmap = PixmapOfFile(gizmo->icon_shell,gizmo->icon_pixmap,
					gizmo->icon_name,&width,&height);

		XtSetArg(arg[0], XtNbackgroundPixmap,  icon_pixmap);
		XtSetArg(arg[1], XtNwidth,  width);
		XtSetArg(arg[2], XtNheight, height);
		XtSetArg(arg[3], XtNwmProtocolInterested,
				OL_WM_DELETE_WINDOW | OL_WM_SAVE_YOURSELF);
		XtSetValues(gizmo->icon_shell, arg, 4);
		OlAddCallback(gizmo->icon_shell, XtNwmProtocol,
				WindowManagerEventHandler, (XtPointer)NULL);
	}
	else
		gizmo->icon_shell = NULL;

	if (gizmo->shell == NULL)
		gizmo->shell = XtCreateApplicationShell(gizmo->name,
					topLevelShellWidgetClass, args, num);

	str = gizmo->title? GetGizmoText(gizmo->title): " ";
	XtSetArg(arg[0], XtNiconName, gizmo->icon_name? gizmo->icon_name: str);
	XtSetArg(arg[1], XtNtitle,    str);
	i = 2;
	if (gizmo->icon_shell) {
		XtSetArg(arg[i], XtNiconWindow, XtWindow(gizmo->icon_shell));
		i++;
	}
	XtSetValues(gizmo->shell, arg, i);

	XtSetArg(arg[0], XtNorientation,   OL_VERTICAL);
	gizmo->form = XtCreateManagedWidget("_X_", rubberTileWidgetClass,
			gizmo->shell, arg, 1);
	/*
	* adjust the gravity and padding
	* note: the padding should be specified in points
	*
	*/
	XtSetArg(arg[0], XtNgravity, NorthWestGravity);
	XtSetArg(arg[1], XtNvPad,  6);
	XtSetArg(arg[2], XtNhPad,  6);
	XtSetArg(arg[3], XtNmenubarBehavior,	TRUE);
	menuparent = CreateGizmo(gizmo->form, MenuBarGizmoClass, (Gizmo)menu,
								arg, 4);
	XtSetArg(arg[0], XtNweight, 		0);
	XtSetArg(arg[1], XtNmenubarBehavior,	TRUE);
	XtSetValues(menuparent, arg, 2);

/*	here's where I shove in my stuff (though evidently it "should" be
 *	done as an array of gizmos through the arguments to CreateGizmo)
 */

	XtSetArg(arg[0], XtNlayoutType,		OL_FIXEDCOLS);
	XtSetArg(arg[1], XtNalignCaptions,	TRUE);
	XtSetArg(arg[2], XtNvSpace,		2*y3mm);
	XtSetArg(arg[3], XtNvPad,		2*y3mm);
	XtSetArg(arg[4], XtNyRefWidget,		menu->child);
	XtSetArg(arg[5], XtNyAddHeight,		TRUE);
	XtSetArg(arg[6], XtNshadowThickness,	0);
	w_ctl = XtCreateManagedWidget("_X_", controlAreaWidgetClass,
						gizmo->form, arg, 7);
/*
 *	this widget will be returned, instead of the shell
 */
	if (gizmo->num_gizmos == 0) {
		XtSetArg(arg[0], XtNxResizable,    True);
		XtSetArg(arg[1], XtNyResizable,    True);
		XtSetArg(arg[2], XtNxAttachRight,  True);
		XtSetArg(arg[3], XtNyAttachBottom, True);
		XtSetArg(arg[4], XtNyAddHeight,    True);
		XtSetArg(arg[5], XtNyRefWidget,    w_ctl);
		XtSetArg(arg[6], XtNweight, 1);
		gizmo->scroller =
			XtCreateManagedWidget("_X_", scrolledWindowWidgetClass,
							gizmo->form, arg, 7);
	}
	else {
		CreateGizmoArray(gizmo->form, gizmo->gizmos, gizmo->num_gizmos);
	}

	if (w_action != NULL) {
		XtSetArg(arg[0], XtNtraversalOn,	TRUE);
		XtSetArg(arg[1], XtNitemFields,		MBtnFields);
		XtSetArg(arg[2], XtNnumItemFields,	NUM_MBtnFields);
		*w_action = XtCreateManagedWidget("action",
			flatButtonsWidgetClass, gizmo->form, arg, 3);
	}

	error_percent = gizmo->error_percent == 0 ? 75 : gizmo->error_percent;

	XtSetArg(arg[0], XtNweight,      0);
	XtSetArg(arg[1], XtNleftFoot,    gizmo->error?
						GetGizmoText(gizmo->error):
						" ");
	XtSetArg(arg[2], XtNrightFoot,   gizmo->status?
						GetGizmoText(gizmo->status):
						" ");
	XtSetArg(arg[3], XtNleftWeight,  error_percent);
	XtSetArg(arg[4], XtNrightWeight, 100-error_percent);
	gizmo->message = XtCreateManagedWidget ("footer", footerWidgetClass,
							gizmo->form, arg, 5);

	return (w_ctl);

}
