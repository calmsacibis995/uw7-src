#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:ModalGizmo.c	1.24"
#endif

/*
 * ModalGizmo.c
 *
 */

#include <stdio.h> 

#include <X11/Intrinsic.h> 
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/Modal.h>
#include <Xol/ControlAre.h>
#include <Xol/StaticText.h>

#include "Gizmos.h"
#include "MenuGizmo.h"
#include "ModalGizmo.h"

static Widget    CreateModalGizmo();
static Gizmo     CopyModalGizmo();
static Gizmo     GetTheMenuGizmo();
static void      FreeModalGizmo();
static void      MapModalGizmo();
static void      ManipulateModalGizmo();
static XtPointer QueryModalGizmo();

GizmoClassRec ModalGizmoClass[] = {
   "ModalGizmo",
   CreateModalGizmo,     /* Create        */
   CopyModalGizmo,       /* Copy          */
   FreeModalGizmo,       /* Free          */
   MapModalGizmo,        /* Map           */
   NULL,                 /* Get           */
   GetTheMenuGizmo,      /* Get Menu      */
   NULL,                  /* Build         */
   ManipulateModalGizmo, /* Manipulate    */
   QueryModalGizmo,      /* Query         */
};

/*
 * CopyModalGizmo
 *
 * The CopyModalGizmo function is used to create a copy
 * of a given ModalGizmo gizmo.
 *
 * See also:
 *
 * FreeModalGizmo(3), CreateModalGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <ModalGizmo.h>
 * ...
 */

static Gizmo
CopyModalGizmo(Gizmo gizmo)
{
   ModalGizmo * old = (ModalGizmo *)gizmo;
   ModalGizmo * new = (ModalGizmo *)MALLOC(sizeof(ModalGizmo));
   int i;

   new->help       = old->help;
   new->name       = old->name;
   new->title      = old->title;
   new->menu       = CopyGizmo(MenuGizmoClass, (Gizmo)(old->menu));
   new->message    = old->message;

   CopyGizmoArray(&new->gizmos, &new->num_gizmos, old->gizmos, old->num_gizmos);

   new->control    = NULL;
   new->stext      = NULL;
   new->shell      = NULL;

   return ((Gizmo)new);

} /* end of CopyModalGizmo */
/*
 * FreeModalGizmo
 *
 * The FreeModalGizmo procedure is used free the ModalGizmo gizmo.
 *
 * See also:
 *
 * CopyModalGizmo(3), CreateModalGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <ModalGizmo.h>
 * ...
 */

static void 
FreeModalGizmo(Gizmo gizmo)
{
   ModalGizmo * old = (ModalGizmo *)gizmo;

   FreeGizmoArray(old->gizmos, old->num_gizmos);
   FreeGizmo(MenuGizmoClass, (Gizmo)(old->menu));
   FREE(old);

} /* end of FreeModalGizmo */
/*
 * CreateModalGizmo
 *
 * The CreateModalGizmo function is used to create the Widget tree
 * defined by the ModalGizmo structure p.  parent is the
 * Widget parent of this new Widget tree.  args and num,
 * if non-NULL, are used as Args in the creation of the popup window
 * Widget which is returned by this function.
 *
 * Standard Appearance:
 *
 * The CreateModalGizmo function creates a standard modal dialog
 * (modalShell).  A staticText widget is created and its string
 * resource set.  The second child to the modalShell is a flatButtons
 * pased in which is constructed as a menu bar in the lowerArea.  A
 * typical window appears as:~
 *
 * See also:
 *
 * CopyModalGizmo(3), FreeModalGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <ModalGizmo.h>
 * ...
 */

static Widget
CreateModalGizmo (Widget parent, ModalGizmo * gizmo, Arg * args, int num)
{
   Arg           arg[10];
   ArgList merged_args;
   Cardinal      num_arg;

   /* Create the Modal shell.  Modal shells take two children:
    * the first is positioned at the top of the widget, and the
    * second is centered at the bottom of the widget.
    */
   XtSetArg(arg[0], XtNresizeCorners, False);
   merged_args = XtMergeArgLists(arg, 1, args, num);

   gizmo->shell = 
      XtCreatePopupShell(gizmo->name, modalShellWidgetClass, parent,
              merged_args, num+1);
   if (gizmo->title)
   {
      XtSetArg(arg[0], XtNtitle, GetGizmoText(gizmo->title));
      XtSetValues(gizmo->shell, arg, 1);
   }

   if (gizmo->num_gizmos == 0)
   {
      /* If there are no other gizmos, then create a static text widget
       * as the first child.  
       */

      XtSetArg(arg[0], XtNstring,
         (gizmo->message == NULL) ? NULL : GetGizmoText(gizmo->message));
      XtSetArg(arg[1], XtNgravity,   CenterGravity);
      XtSetArg(arg[2], XtNalignment, OL_CENTER);
      gizmo->stext = 
         XtCreateManagedWidget("_X_", staticTextWidgetClass, gizmo->shell, arg, 3);
   }
   else
   {
      /* At least one gizmo has been specified for the upper area
       * of the Modal.  Create a control area to contain these gizmos
       * so that the Modal still only manages two children.
       */
      gizmo->stext = NULL;
      XtSetArg(arg[0], XtNalignCaptions, True);
      XtSetArg(arg[1], XtNcenter,        True);
      XtSetArg(arg[2], XtNsameSize,      OL_NONE);
      XtSetArg(arg[3], XtNlayoutType,    OL_FIXEDCOLS);
      XtSetArg(arg[4], XtNmeasure,       1);
      gizmo->control =
         XtCreateManagedWidget("_X_", controlAreaWidgetClass, gizmo->shell, arg, 5);
      CreateGizmoArray(gizmo->control, gizmo->gizmos, gizmo->num_gizmos);
   }

   /* The buttons at the bottom of the Modal are contained in the menu
    * field.  Create these as the second child of the Modal shell.
    */
   if (gizmo->menu != NULL)
      CreateGizmo(gizmo->shell, MenuBarGizmoClass, (Gizmo)(gizmo->menu), NULL, 0);

   if (gizmo->help)
      GizmoRegisterHelp(OL_WIDGET_HELP, gizmo->shell,
         gizmo->help->title, OL_DESKTOP_SOURCE, gizmo->help);

   return (gizmo->shell);

} /* end of CreateModalGizmo */
/*
 * GetTheMenuGizmo
 *
 */
static Gizmo
GetTheMenuGizmo (ModalGizmo * gizmo)
{

   return (gizmo->menu);

} /* end of GetTheMenuGizmo */
/*
 * MapModalGizmo
 *
 */

static void
MapModalGizmo(ModalGizmo * gizmo)
{
   Widget    shell = gizmo->shell;

   XtPopup(shell, XtGrabExclusive);
   XRaiseWindow(XtDisplay(shell), XtWindow(shell));

} /* end of MapModalGizmo */
/*
 * ManipulateModalGizmo
 *
 */

static void
ManipulateModalGizmo(Gizmo gizmo, ManipulateOption option)
{
} /* end of ManipulateModalGizmo */
/*
 * QueryModalGizmo
 *
 */

static XtPointer
QueryModalGizmo(ModalGizmo * gizmo, int option, char * name)
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

} /* end of QueryModalGizmo */
/*
 * GetModalGizmoShell
 *
 */

extern Widget
GetModalGizmoShell(ModalGizmo * gizmo)
{

   return (gizmo->shell);

} /* end of GetModalGizmoShell */
/*
 * SetModalGizmoMessage
 *
 * this facilitates setting the message string
 * in the StaticText widget which was created for the "Notice" case.
 *
 */

extern void
SetModalGizmoMessage(ModalGizmo * gizmo, char * message)
{
   Arg arg[1];

   if (gizmo->stext != NULL)
   {
      XtSetArg(arg[0], XtNstring, message);
      XtSetValues(gizmo->stext, arg, 1);
   }

} /* end of SetModalGizmoMessage */
