#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:NumericGiz.c	1.20"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/TextField.h>
#include <Xol/IntegerFie.h>
#include <Xol/ControlAre.h>
#include <Xol/Caption.h>
#include <Xol/StaticText.h>

#include "Gizmos.h"
#include "NumericGiz.h"

#define MAX(x,y)    (((x) > (y)) ? (x) : (y))

static Widget    CreateNumericGizmo();
static void      FreeNumericGizmo();
static Gizmo     CopyNumericGizmo();
static void      ManipulateNumericGizmo();
static XtPointer QueryNumericGizmo();
static void      ManageNumericGizmo();

static void      SetTextFielditoa();

GizmoClassRec NumericGizmoClass[] = {
   "NumericGizmo",
   CreateNumericGizmo,       /* Create     */
   CopyNumericGizmo,         /* Copy       */
   FreeNumericGizmo,         /* Free       */
   NULL,                     /* Map        */
   NULL,                     /* Get        */
   NULL,                     /* Get Menu   */
   NULL,                     /* Build      */
   ManipulateNumericGizmo,   /* Manipulate */
   QueryNumericGizmo,        /* Query      */
   ManageNumericGizmo,       /* Manage     */
};

/*
 * CopyNumericGizmo
 *
 */

static Gizmo
CopyNumericGizmo(Gizmo gizmo)
{
   NumericGizmo * old = (NumericGizmo *) gizmo;
   NumericGizmo * new = (NumericGizmo *) MALLOC (sizeof(NumericGizmo));

   new->help            = old->help;
   new->name            = old->name;
   new->caption         = old->caption;
   new->min             = old->min;
   new->max             = old->max;
   new->settings        = (Setting *)MALLOC(sizeof(Setting));
   new->settings->current_value  = old->settings->current_value;
   new->settings->previous_value = old->settings->previous_value;
   new->settings->initial_value  = old->settings->initial_value;
   new->charsVisible    = old->charsVisible;
   new->label           = old->label;
   new->args            = old->args;
   new->num_args        = old->num_args;
   new->captionWidget   = (Widget)0;
   new->labelWidget     = (Widget)0;
   new->controlWidget   = (Widget)0;
   new->textFieldWidget = (Widget)0;
   new->scrollbarWidget = (Widget)0;

   return ((Gizmo)new);

} /* end of CopyNumericGizmo */
/*
 * FreeNumericGizmo
 *
 */

static void
FreeNumericGizmo (Gizmo gizmo)
{
   NumericGizmo * old = (NumericGizmo *)gizmo;

   FREE(old->settings);
   FREE(old);

} /* FreeNumericGizmo */
/*
 * CreateNumericGizmo
 *
 */

static Widget
CreateNumericGizmo (Widget parent, NumericGizmo * gizmo, ArgList args, int num_args)
{

   char     buf[256];
   Arg      arg[SIZE_ARGS];
   Cardinal num_arg;

   if (gizmo->caption)
   {
      XtSetArg(arg[0], XtNlabel, GetGizmoText(gizmo->caption));
      parent =
      gizmo->captionWidget = 
         XtCreateManagedWidget(gizmo->name, captionWidgetClass, parent, arg, 1);
   }
   else
      gizmo->captionWidget = NULL;

   XtSetArg(arg[0], XtNlayoutType, OL_FIXEDROWS);
   XtSetArg(arg[1], XtNmeasure,    1);
   XtSetArg(arg[2], XtNvPad,       0);
   XtSetArg(arg[3], XtNvSpace,     0);
   XtSetArg(arg[4], XtNshadowThickness, 0);
   gizmo->controlWidget = 
      XtCreateManagedWidget("_X_", controlAreaWidgetClass, parent, arg, 5);

   if (gizmo->help)
      GizmoRegisterHelp(OL_WIDGET_HELP, (gizmo->captionWidget == NULL) ?
         gizmo->controlWidget : gizmo->captionWidget,
         gizmo->help->title, OL_DESKTOP_SOURCE, gizmo->help);

   gizmo->settings->current_value = gizmo->settings->previous_value;
   sprintf (buf,"%d", gizmo->settings->current_value);
   XtSetArg(arg[0], XtNcharsVisible,     MAX(gizmo->charsVisible, 4));
   XtSetArg(arg[1], XtNstring,           buf);
   XtSetArg(arg[2], XtNvalueMin,         gizmo->min);
   XtSetArg(arg[3], XtNvalueMax,         gizmo->max);
   XtSetArg(arg[4], XtNvalueGranularity, 1);
   num_arg = AppendArgsToList(arg, 5, gizmo->args, gizmo->num_args);
   gizmo->textFieldWidget = 
      XtCreateManagedWidget("_X_", integerFieldWidgetClass, gizmo->controlWidget, arg, num_arg);

   if (gizmo->label)
   {
      XtSetArg(arg[0], XtNstring, GetGizmoText(gizmo->label));
      gizmo->labelWidget = 
         XtCreateManagedWidget("_X_", staticTextWidgetClass, 
                               gizmo->controlWidget, arg, 1);
   }

   return (gizmo->controlWidget);

} /* end of CreateNumericGizmo */
/*
 * ManipulateNumericGizmo
 *
 */

static void
ManipulateNumericGizmo(NumericGizmo * gizmo, ManipulateOption option)
{
   Arg         arg[2];
   char *      value;

   switch (option)
   {
   case GetGizmoValue:
      XtSetArg(arg[0], XtNstring, &value);
      XtGetValues(gizmo->textFieldWidget, arg, 1);
      gizmo->settings->current_value = (XtPointer)atoi(value);
      FREE(value);
      break;
   case ApplyGizmoValue:
      gizmo->settings->previous_value = gizmo->settings->current_value;
      break;
   case SetGizmoValue:
      gizmo->settings->previous_value = gizmo->settings->current_value;
      break;
   case ResetGizmoValue:
      gizmo->settings->current_value = gizmo->settings->previous_value;
      SetTextFielditoa(gizmo->textFieldWidget, gizmo->settings->current_value);
      break;
   case ReinitializeGizmoValue:
      gizmo->settings->current_value = gizmo->settings->initial_value;
      SetTextFielditoa(gizmo->textFieldWidget, gizmo->settings->current_value);
      break;
   case RegisterGizmoConverter:
      break;
   default:
      break;
   }

} /* end of ManipulateNumericGizmo */
/*
 * QueryNumericGizmo
 *
 */

static XtPointer
QueryNumericGizmo(NumericGizmo * gizmo, int option, char * name)
{
   if (!name || strcmp(name, gizmo->name) == 0)
   {
      switch(option)
      {
         case GetGizmoSetting:
            return (XtPointer)(gizmo->settings);
            break;
         case GetGizmoWidget:
            return (XtPointer)(gizmo->textFieldWidget);
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
      return (XtPointer)(NULL);

} /* end of QueryNumericGizmo */
/*
 * ManageNumericGizmo
 *
 */

static void
ManageNumericGizmo(NumericGizmo * gizmo, int option, char * name)
{
   Arg arg[5];

   if (name == NULL || strcmp(gizmo->name, name) == 0 &&
       gizmo->captionWidget != (Widget)0)
      switch(option)
      {
         case SensitizeGizmo:
            XtSetSensitive(gizmo->captionWidget, True);
            break;
         case DesensitizeGizmo:
            XtSetSensitive(gizmo->captionWidget, False);
            break;
         case UnhideGizmo:
#ifdef MANAGE
            XtManageChild(gizmo->captionWidget);
#else
            if (XtIsRealized(gizmo->captionWidget))
               XtMapWidget(gizmo->captionWidget);
            else
            {
               XtSetArg(arg[0], XtNmappedWhenManaged, True);
               XtSetValues(gizmo->captionWidget, arg, 1);
            }
#endif
            break;
         case HideGizmo:
#ifdef MANAGE
            XtUnmanageChild(gizmo->captionWidget);
#else
            if (XtIsRealized(gizmo->captionWidget))
               XtUnmapWidget(gizmo->captionWidget);
            else
            {
               XtSetArg(arg[0], XtNmappedWhenManaged, False);
               XtSetValues(gizmo->captionWidget, arg, 1);
            }
#endif
            break;
         default:
            break;
      }

} /* end of ManageNumericGizmo */
/*
 * SetTextFielditoa
 *
 */

static void
SetTextFielditoa (Widget wid, int n)
{
   char buf[256];
   Arg  arg[1];

   sprintf (buf, "%d", n);
   XtSetArg(arg[0],  XtNstring, buf);
   XtSetValues(wid, arg, 1);

} /* end of SetTextFielditoa */
/*
 * GetNumericFieldValue
 *
 */

extern int
GetNumericFieldValue (NumericGizmo * gizmo)
{

   return (int)gizmo->settings->current_value;

} /* end of GetNumericFieldValue */
/*
 * SetNumericFieldValue
 *
 */

extern void
SetNumericFieldValue (NumericGizmo * gizmo, int val)
{

   SetTextFielditoa (gizmo->textFieldWidget, val);

} /* end of SetNumericFieldValue */
