#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:PopupGizmo.c	1.19"
#endif

/*
 * PopupGizmo.c
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h> 
#include <X11/StringDefs.h>

#include <buffutil.h>
#include <textbuff.h>

#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/Footer.h>

#include "Gizmos.h"
#include "MenuGizmo.h"
#include "PopupGizmo.h"

static Widget    CreatePopupGizmo();
static Gizmo     CopyPopupGizmo();
static void      FreePopupGizmo();
static void      MapPopupGizmo();
static Gizmo     GetPopupGizmo();
static Gizmo     GetTheMenuGizmo();
static void      ManipulatePopupGizmo();
static XtPointer QueryPopupGizmo();

GizmoClassRec PopupGizmoClass[] = { 
   "PopupGizmo",
   CreatePopupGizmo,
   CopyPopupGizmo,
   FreePopupGizmo, 
   MapPopupGizmo,
   GetPopupGizmo,
   GetTheMenuGizmo,
   NULL,
   ManipulatePopupGizmo,
   QueryPopupGizmo,
};

/*
 * CopyPopupGizmo
 *
 * The CopyPopupGizmo function is used to create a copy
 * of a given PopupGizmo gizmo.
 *
 * See also:
 *
 * FreePopupGizmo(3), CreatePopupGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <PopupGizmo.h>
 * ...
 */

static Gizmo
CopyPopupGizmo(Gizmo gizmo)
{
   PopupGizmo * new = (PopupGizmo*)MALLOC(sizeof(PopupGizmo));
   PopupGizmo * old = (PopupGizmo*)gizmo;

   new->help       = old->help;
   new->name       = old->name;
   new->title      = old->title;
   new->menu       = CopyGizmo(MenuGizmoClass, old->menu);
   CopyGizmoArray(&new->gizmos, &new->num_gizmos, old->gizmos, old->num_gizmos);
   new->args       = old->args;
   new->num_args   = old->num_args;
   new->message    = NULL;
   new->shell      = NULL;
   new->error_percent= old->error_percent;
   new->error        = old->error;
   new->status       = old->status;

   return ((Gizmo)new);

} /* end of CopyPopupGizmo */
/*
 * FreePopupGizmo
 *
 * The FreePopupGizmo procedure is used free the PopupGizmo gizmo.
 *
 * See also:
 *
 * CopyPopupGizmo(3), CreatePopupGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <PopupGizmo.h>
 * ...
 */

static void 
FreePopupGizmo(Gizmo gizmo)
{
   PopupGizmo * old = (PopupGizmo *)gizmo;

   FreeGizmo(MenuGizmoClass, old->menu);
   FreeGizmoArray(old->gizmos, old->num_gizmos);
   FREE(old);

} /* end of FreePopupGizmo */
/*
 * CreatePopupGizmo
 *
 * The CreatePopupGizmo function is used to create the Widget tree
 * defined by the PopupGizmo structure p.  parent is the
 * Widget parent of this new Widget tree.  args and num,
 * if non-NULL, are used as Args in the creation of the popup window
 * Widget which is returned by this function.
 *
 * Standard Appearance:
 *
 * The CreatePopupGizmo function creates a standard dialog
 * composed of a popupWindowShell widget containing widgets constructed
 * based on the MiscGizmos in the PopupGizmo structure in the
 * in the upperControlArea and a flatButtons Widget, constructed as 
 * a menu bar in the lowerControlArea.  A typical window appears as:~
 *
 * See also:
 *
 * CopyPopupGizmo(3), FreePopupGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <PopupGizmo.h>
 * ...
 */

static Widget
CreatePopupGizmo(Widget parent, PopupGizmo * gizmo, Arg * args, int num)
{
   Arg          arg[100];
   Widget       lowerArea;
   Widget       upperArea;
   Widget       footerPanel;
   int          error_percent;

   gizmo->shell = 
      XtCreatePopupShell(gizmo->name, popupWindowShellWidgetClass, parent, args, num);

   if (gizmo->help)
      GizmoRegisterHelp(OL_WIDGET_HELP, gizmo->shell,
         gizmo->help->title, OL_DESKTOP_SOURCE, gizmo->help);

   XtAddCallback(gizmo->shell, XtNverify, DisallowGizmoPopdown, NULL);

   XtSetArg(arg[0], XtNtitle, GetGizmoText(gizmo->title));
   XtSetValues(gizmo->shell, arg, 1);

   XtSetArg(arg[0], XtNupperControlArea, &upperArea);
   XtSetArg(arg[1], XtNlowerControlArea, &lowerArea);
   XtSetArg(arg[2], XtNfooterPanel, &footerPanel);
   XtGetValues(gizmo->shell, arg, 3);

   if (gizmo->num_gizmos > 0)
   {
      CreateGizmoArray(upperArea, gizmo->gizmos, gizmo->num_gizmos);
   }

   XtSetArg(arg[0], XtNlayoutType, OL_FIXEDROWS);
   XtSetArg(arg[1], XtNmeasure,    2);
   XtSetValues(lowerArea, arg, 2);

   if (gizmo->menu != NULL)
   {
      CreateGizmo(lowerArea, MenuBarGizmoClass, (Gizmo)gizmo->menu, NULL, 0);
   }

   error_percent = gizmo->error_percent == 0 ? 75 : gizmo->error_percent;

   XtSetArg(arg[0], XtNweight,      0);
   XtSetArg(arg[1], XtNleftFoot,    gizmo->error ? GetGizmoText(gizmo->error) : " ");
   XtSetArg(arg[2], XtNrightFoot,   gizmo->status ? GetGizmoText(gizmo->status) : " ");
   XtSetArg(arg[3], XtNleftWeight,  error_percent);
   XtSetArg(arg[4], XtNrightWeight, 100-error_percent);
   gizmo->message = XtCreateManagedWidget (
      "footer", footerWidgetClass, footerPanel, arg, 5
   );

   return (gizmo->shell);

} /* end of CreatePopupGizmo */
/*
 * BringDownPopup
 *
 * The BringDownPopup procedure is used to popdown a popup
 * shell Widget widget.  This convenience routine checks the
 * pupshpin state to determine if the popup should be popped down.
 *
 * Private:
 *   The toolkit currently doesn't provide a routine for us to
 *   do this, so we've got to do it ourselves...
 *   Can this be done more efficiently???
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <PopupGizmo.h>
 * ...
 */

extern void
BringDownPopup(Widget wid)
{
   long pushpin_state = WMPushpinIsOut;

   if (XtIsRealized(wid)) 
   {
      GetWMPushpinState(XtDisplay(wid), XtWindow(wid), &pushpin_state);
      switch (pushpin_state) 
      {
         case WMPushpinIsIn:
            break;
         case WMPushpinIsOut:
         default:
            XtPopdown(wid);
            break;
      }
   }

} /* end of BringDownPopup */
/*
 * GetTheMenuGizmo
 *
 */
static Gizmo 
GetTheMenuGizmo(PopupGizmo * gizmo)
{

   return (gizmo->menu);

} /* end of GetTheMenuGizmo */
/*
 * GetPopupGizmoShell
 *
 */

extern Widget
GetPopupGizmoShell(PopupGizmo * gizmo)
{

   return (gizmo->shell);

} /* end of GetPopupGizmoShell */
/*
 * MapPopupGizmo
 *
 */

static void 
MapPopupGizmo(PopupGizmo * gizmo)
{
   Widget    shell = gizmo->shell;

   XtPopup(shell, XtGrabNone);
   XRaiseWindow(XtDisplay(shell), XtWindow(shell));

} /* end of MapPopupGizmo */
/*
 * GetPopupGizmo
 *
 */

static Gizmo 
GetPopupGizmo(gizmo, item)
PopupGizmo * gizmo;
int          item;
{

   return (gizmo->gizmos[item].gizmo);

} /* end of GetPopupGizmo */
/*
 * ManipulatePopupGizmo
 *
 */

static void
ManipulatePopupGizmo(PopupGizmo * gizmo, ManipulateOption option)
{
   GizmoArray   gp = gizmo->gizmos;
   int i;

   for (i = 0; i < gizmo->num_gizmos; i++)
   {
      ManipulateGizmo(gp[i].gizmo_class, gp[i].gizmo, option);
   }

} /* end of ManipulatePopupGizmo */
/*
 * QueryPopupGizmo
 *
 */

static XtPointer
QueryPopupGizmo(PopupGizmo * gizmo, int option, char * name)
{
   XtPointer value = NULL;

   if (!name || strcmp(name, gizmo->name) == 0)
   {
      switch(option)
      {
         case GetGizmoSetting:
            return (XtPointer)(NULL);
            break;
         case GetGizmoWidget:
            return (XtPointer)(gizmo->shell);
            break;
         case GetGizmoGizmo:
            return (XtPointer)(gizmo);
            break;
         default:
            return (XtPointer)(NULL);
            break;
      }
   }
   else
   {
      if (gizmo->menu) {
         value = QueryGizmo(MenuGizmoClass, gizmo->menu, option, name);
      }
      if (value)
         return (value);
      else
      {
         return(QueryGizmoArray(gizmo->gizmos, gizmo->num_gizmos, option, name));
      }
   }

} /* end of QueryPopupGizmo */
/*
 * SetPopupMessage
 *
 */

extern void 
SetPopupMessage(PopupGizmo * gizmo, char * message)
{
   Arg arg[10];

   XtSetArg(arg[0], XtNleftFoot,
      ((message == NULL) || (*message == 0)) ? "" : GetGizmoText(message));
   XtSetValues(gizmo->message, arg, 1);

} /* end of SetPopupMessage */

/*
 * SetPopupStatus
 *
 * The SetPopupStatus procedure is set the message
 * appearing in the right footer message are of the gizmo PopupGizmo
 * to the message string.
 *
 */

extern void 
SetPopupStatus(PopupGizmo * gizmo, char * message)
{
   Arg arg[10];

   XtSetArg(arg[0], XtNrightFoot,
      ((message == NULL) || (*message == 0)) ? "" : GetGizmoText(message));
   XtSetValues(gizmo->message, arg, 1);

} /* end of SetPopupStatus */
