#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:InputGizmo.c	1.18"
#endif

/*
 * InputGizmo.c
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/TextField.h>
#include <Xol/Caption.h>

#include "Gizmos.h"
#include "PopupGizmo.h"
#include "InputGizmo.h"

#define NULL_STRDUP(s) (s != NULL) ? STRDUP(s) : NULL

static Widget    CreateInput();
static Gizmo     CopyInput();
static void      FreeInput();
static void      ManipulateInput();
static XtPointer QueryInput();
static void      ManageInput();

GizmoClassRec InputGizmoClass[] = 
   { 
      "InputGizmo",
      CreateInput,     /* Create      */
      CopyInput,       /* Copy        */
      FreeInput,       /* Free        */
      NULL,            /* Map         */
      NULL,            /* Get         */
      NULL,            /* Get Menu    */
      NULL,            /* Build       */
      ManipulateInput, /* Manipulate  */
      QueryInput,      /* Query       */
      ManageInput,     /* Manage      */
   };


/*
 * CopyInput
 *
 */

static Gizmo
CopyInput(Gizmo gizmo)
{
   InputGizmo * old = (InputGizmo *)gizmo;
   InputGizmo * new = (InputGizmo *)MALLOC(sizeof(InputGizmo));

   new->help             = old->help;
   new->name             = old->name;
   new->caption          = old->caption;
   new->text             = old->text;
   new->settings         = (Setting *)CALLOC(1, sizeof(Setting));
   new->settings->flag   = old->settings ? old->settings->flag : 0;
   new->charsVisible     = old->charsVisible;
   new->verify           = old->verify;
   new->args             = old->args;
   new->num_args         = old->num_args;
   new->captionWidget    = NULL;
   new->textFieldWidget  = NULL;

   return (Gizmo)new;

} /* end of CopyInput */
/*
 * FreeInput
 *
 */

static void 
FreeInput(Gizmo gizmo)
{
   InputGizmo * old = (InputGizmo *)gizmo;

   if (old->settings) {
   	if (old->settings->initial_string)
	      FREE(old->settings->initial_string);
	if (old->settings->initial_value)
	      FREE(old->settings->initial_value);
	if (old->settings->current_value)
	      FREE(old->settings->current_value);
	if (old->settings->previous_value)
	      FREE(old->settings->previous_value);
	FREE(old->settings);
   }
   FREE(old);

} /* end of FreeInput */
/*
 * CreateInput
 *
 * Creates a popup caption.
 *
 * GetInputText() - returns the text from the captions text field.
 */

static Widget
CreateInput(Widget promptArea, InputGizmo * gizmo, ArgList args, int num_args)
{
   Arg      arg[100];
   Cardinal num_arg;
   char *   caption_name = gizmo->name ? gizmo->name : "caption";
   char *   text_name    = gizmo->name ? gizmo->name : "text_input";

   gizmo->settings->previous_value = gizmo->text ? STRDUP(gizmo->text) : NULL;
   gizmo->settings->current_value = NULL;

   if (gizmo->caption == NULL)
       gizmo->captionWidget = promptArea;
   else
   {
       XtSetArg(arg[0], XtNlabel, GetGizmoText(gizmo->caption));
       gizmo->captionWidget =
           XtCreateManagedWidget(caption_name, captionWidgetClass,
				 promptArea, arg, 1);
   }

   if (gizmo->help)
      GizmoRegisterHelp(OL_WIDGET_HELP, gizmo->captionWidget,
         gizmo->help->title, OL_DESKTOP_SOURCE, gizmo->help);

   XtSetArg(arg[0], XtNcharsVisible, gizmo->charsVisible);
   XtSetArg(arg[1], XtNstring, gizmo->text);
   num_arg = AppendArgsToList(arg, 2, gizmo->args, gizmo->num_args);
   num_arg = AppendArgsToList(arg, num_arg, args, num_args);
      
   gizmo->textFieldWidget = 
      XtCreateManagedWidget(text_name, textFieldWidgetClass, 
         gizmo->captionWidget, arg, num_arg);

   if (gizmo->verify)
      XtAddCallback(gizmo->textFieldWidget, XtNverification, 
         gizmo->verify, (XtPointer)0);

   return (gizmo->captionWidget);

} /* end of CreateInput */
/*
 * GetInputText
 *
 * The GetInputText function is used to retrieve the text
 * stored in a InputGizmo which is in the PopupGizmo shell.
 * The item index is used to determine which of the MiscGizmos
 * in the shell is to be retrieved.
 *
 * See also:
 *
 * CreatePopupGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <PopupGizmo.h>
 *#include <InputGizmo.h>
 * ...
 */

extern char *
GetInputText(PopupGizmo * shell, int item)
{
   InputGizmo * gizmo = (InputGizmo *)shell->gizmos[item].gizmo;
   char *       text;
   Arg          arg[1];

   XtSetArg(arg[0], XtNstring, &text);
   XtGetValues(gizmo->textFieldWidget, arg, 1);

   return (text);          /* Must be freed */

} /* end of GetInputText */
/*
 * SetInputText
 *
 * The SetInputText procedure is used to replace the text
 * stored in a InputGizmo which is in the PopupGizmo shell.
 * The item index is used to determine which of the MiscGizmos
 * in the shell is to be replaced with text.  If selected
 * is True, then the text is selected.
 *
 * See also:
 *
 * CreatePopupGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <PopupGizmo.h>
 *#include <InputGizmo.h>
 * ...
 */

extern void
SetInputText(PopupGizmo * shell, int item, char * text, int selected)
{
   InputGizmo * gizmo = (InputGizmo *)shell->gizmos[item].gizmo;
   Arg          arg[10];
   Widget       textEditWidget;

#ifdef DEBUG
   (void)fprintf(stderr,"setting text to '%s'.\n", text);
#endif

   XtSetArg(arg[0], XtNstring,      text);
   XtSetValues(gizmo->textFieldWidget, arg, 1);
 
   if (selected)
   {
      XtSetArg(arg[0], XtNtextEditWidget, &textEditWidget);
      XtGetValues(gizmo->textFieldWidget, arg, 1);

      XtSetArg(arg[0], XtNselectStart, 0);
      XtSetArg(arg[1], XtNselectEnd,   strlen(text));
      XtSetValues(textEditWidget, arg, 2);

      OlSetInputFocus(textEditWidget, RevertToNone, CurrentTime);
   }

} /* end of SetInputText */
/*
 * ManipulateInput
 *
 */

static void
ManipulateInput(InputGizmo * gizmo, ManipulateOption option)
{
   Arg         arg[2];

   switch (option)
   {
   case GetGizmoValue:
      if (gizmo->settings->current_value != NULL)
         FREE(gizmo->settings->current_value);
      XtSetArg(arg[0], XtNstring, &gizmo->settings->current_value);
      XtGetValues(gizmo->textFieldWidget, arg, 1);
      break;
   case ApplyGizmoValue:
      if (gizmo->settings->previous_value != NULL)
         FREE(gizmo->settings->previous_value);
      gizmo->settings->previous_value = 
         NULL_STRDUP(gizmo->settings->current_value);
      break;
   case SetGizmoValue:
      if (gizmo->settings->previous_value != NULL)
         FREE(gizmo->settings->previous_value);
      gizmo->settings->previous_value = 
         NULL_STRDUP(gizmo->settings->current_value);
      break;
   case ResetGizmoValue:
      if (gizmo->settings->current_value != NULL)
         FREE(gizmo->settings->current_value);
      gizmo->settings->current_value = 
         NULL_STRDUP(gizmo->settings->previous_value);
      XtSetArg(arg[0], XtNstring, gizmo->settings->current_value);
      XtSetValues(gizmo->textFieldWidget, arg, 1);
      break;
   case ReinitializeGizmoValue:
      if (gizmo->settings->current_value != NULL)
         FREE(gizmo->settings->current_value);
      gizmo->settings->current_value = 
         NULL_STRDUP(gizmo->settings->initial_value);
      XtSetArg(arg[0], XtNstring, gizmo->settings->current_value);
      XtSetValues(gizmo->textFieldWidget, arg, 1);
      break;
   default:
      break;
   }

} /* end of ManipulateInput */

/*
 * QueryInput
 *
 */

static XtPointer
QueryInput(InputGizmo * gizmo, int option, char * name)
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

} /* end of QueryInput */
/*
 * ManageInput
 *
 */

static void
ManageInput(InputGizmo * gizmo, int option, char * name)
{
   Arg arg[5];

   if (name == NULL || strcmp(gizmo->name, name) == 0)
      switch(option)
      {
         case SensitizeGizmo:
            XtSetSensitive(gizmo->captionWidget, True);
            break;
         case DesensitizeGizmo:
            XtSetSensitive(gizmo->captionWidget, False);
            break;
         case UnhideGizmo:
            if (XtIsRealized(gizmo->captionWidget))
               XtMapWidget(gizmo->captionWidget);
            else
            {
               XtSetArg(arg[0], XtNmappedWhenManaged, True);
               XtSetValues(gizmo->captionWidget, arg, 1);
            }
            break;
         case HideGizmo:
            if (XtIsRealized(gizmo->captionWidget))
               XtUnmapWidget(gizmo->captionWidget);
            else
            {
               XtSetArg(arg[0], XtNmappedWhenManaged, False);
               XtSetValues(gizmo->captionWidget, arg, 1);
            }
         default:
            break;
      }

} /* end of ManageInput */
