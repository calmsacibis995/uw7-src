#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:TimeGizmo.c	1.11"
#endif

/*
 * TimeGizmo.c
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/TextField.h>
#include <Xol/StepField.h>
#include <Xol/Caption.h>

#include "Gizmos.h"
#include "PopupGizmo.h"
#include "TimeGizmo.h"

static Widget    CreateTime();
static Gizmo     CopyTime();
static void      FreeTime();
static void      ManipulateTime();
static XtPointer QueryTime();
static void      ManageTime();
static void      Verify(Widget w, XtPointer client_data, XtPointer call_data);
static void      TimeStep(Widget w, XtPointer client_data, XtPointer call_data);

GizmoClassRec TimeGizmoClass[] = { 
   "TimeGizmo",
   CreateTime,     /* Create      */
   CopyTime,       /* Copy        */
   FreeTime,       /* Free        */
   NULL,           /* Map         */
   NULL,           /* Get         */
   NULL,           /* Get Menu    */
   NULL,           /* Build       */
   ManipulateTime, /* Manipulate  */
   QueryTime,      /* Query       */
   ManageTime,     /* Manage      */
};

/*
 * CopyTime
 *
 */

static Gizmo
CopyTime(Gizmo gizmo)
{
   TimeGizmo * old = (TimeGizmo *)gizmo;
   TimeGizmo * new = (TimeGizmo *)MALLOC(sizeof(TimeGizmo));

   new->help             = old->help;
   new->name             = old->name;
   new->caption          = old->caption;
   new->text             = old->text;
   new->settings         = (Setting *)MALLOC(sizeof(Setting));
   new->settings->initial_string = NULL;
   new->settings->initial_value  = NULL;
   new->settings->current_value  = NULL;
   new->settings->previous_value = NULL;
   new->settings->flag   = old->settings->flag;
   new->args             = old->args;
   new->num_args         = old->num_args;
   new->captionWidget    = NULL;
   new->textFieldWidget  = NULL;

   return (Gizmo)new;

} /* end of CopyTime */
/*
 * FreeTime
 *
 */

static void 
FreeTime(Gizmo gizmo)
{
   TimeGizmo * old = (TimeGizmo *)gizmo;

   if (old->settings->initial_string)
      FREE(old->settings->initial_string);
   if (old->settings->initial_value)
      FREE(old->settings->initial_value);
   if (old->settings->current_value)
      FREE(old->settings->current_value);
   if (old->settings->previous_value)
      FREE(old->settings->previous_value);
   FREE(old->settings);
   FREE(old);

} /* end of FreeTime */
/*
 * CreateTime
 *
 * Creates a popup caption.
 *
 */

static Widget
CreateTime(Widget promptArea, TimeGizmo * gizmo, ArgList args, int num_args)
{
   Arg      arg[100];
   Cardinal num_arg;
   char *   caption_name = gizmo->name ? gizmo->name : "caption";
   char *   text_name    = gizmo->name ? gizmo->name : "text_input";

   gizmo->settings->previous_value = STRDUP(gizmo->text);
   gizmo->settings->current_value = NULL;

   XtSetArg(arg[0], XtNlabel, GetGizmoText(gizmo->caption));
   gizmo->captionWidget = 
      XtCreateManagedWidget(caption_name, captionWidgetClass, promptArea, arg, 1);

   if (gizmo->help)
      GizmoRegisterHelp(OL_WIDGET_HELP, gizmo->captionWidget,
         gizmo->help->title, OL_DESKTOP_SOURCE, gizmo->help);
   
   XtSetArg(arg[0], XtNstring,       gizmo->text);
   XtSetArg(arg[1], XtNcharsVisible, 8);
   num_arg = AppendArgsToList(arg, 2, gizmo->args, gizmo->num_args);
      
   gizmo->textFieldWidget = 
      XtCreateManagedWidget(text_name, stepFieldWidgetClass, 
         gizmo->captionWidget, arg, num_arg);

   XtAddCallback(gizmo->textFieldWidget, XtNmodifyVerification, Verify, gizmo);
   XtAddCallback(gizmo->textFieldWidget, XtNstepped, TimeStep, gizmo);

   return (gizmo->captionWidget);

} /* end of CreateTime */
/*
 * GetCurrentTime
 *
 */

static char *
GetCurrentTime(TimeGizmo * gizmo, int * hourp, int * minp)
{

   char *        time;
   char *        newtime;
   Arg           arg[5];

   XtSetArg(arg[0], XtNstring, &time);
   XtGetValues(gizmo->textFieldWidget, arg, 1);

   newtime = FormatTime(time, hourp, minp);
   FREE(time);

   return (newtime);

} /* end of GetCurrentTime */
/*
 * SetCurrentTime
 *
 */

static void
SetCurrentTime(TimeGizmo * gizmo, int hour, int min)
{

   char          buffer[50];
   Arg           arg[5];

   sprintf(buffer, "%02d:%02d", hour, min);
   XtSetArg(arg[0], XtNstring,         buffer);
   XtSetArg(arg[1], XtNcursorPosition, 5);
   XtSetArg(arg[2], XtNselectStart,    0);
   XtSetArg(arg[3], XtNselectEnd,      0);
   XtSetValues(gizmo->textFieldWidget, arg, 4);

} /* end of SetCurrentTime */
/*
 * Verify
 *
 */

static void
Verify(Widget w, XtPointer client_data, XtPointer call_data)
{
   TimeGizmo *            gizmo = (TimeGizmo *)client_data;
   OlTextModifyCallData * p     = (OlTextModifyCallData *)call_data;

   int           min            = 60;
   int           hour           = 0;
   int           found_colon    = False;
   int           i;

   char *        hourp;
   char *        minp;
   char *        time;
   char *        newtime;
   Arg           arg[5];

   XtSetArg(arg[0], XtNstring, &time);
   XtGetValues(gizmo->textFieldWidget, arg, 1);

   newtime = MALLOC(strlen(time) + p->text_length + 20);

   strcpy(newtime, time);
   strcpy(&newtime[p->select_start], p->text);
   strcat(newtime, &time[p->select_end]);

   /*
    * check for valid characters
    */

   p->ok = (strlen(newtime) == strspn(newtime, "0123456789:"));

   /*
    * check for multiple colons
    */

   for (i = 0; i < (int)strlen(newtime); i++)
      if (newtime[i] == ':')
         if (found_colon) {
            p->ok = False;
	    break;
	 }
         else
            found_colon = True;

   hourp = strtok(newtime, ":");
   minp  = strtok(NULL, ":");
   hour  = hourp ? atoi(hourp) : 0;
   min   = minp  ? atoi(minp)  : 0;

   /*
    * range check hours and minutes plus
    * number of digits (prevent too many leading zeroes)
    */

   p->ok = (p->ok) && (0 <= hour && hour <=23) && (0 <= min && min <= 59);
   if (minp != NULL) {
      p->ok = (p->ok) && strlen(minp) <= 2;
   }
   if (hourp != NULL) {
      p->ok = (p->ok) && strlen(hourp) <= 2;
   }

   if (!p->ok)
      XBell(XtDisplay(w), 0);
   
   FREE(newtime);
   FREE(time);

} /* end of Verify */
/*
 * TimeStep
 *
 */

static void
TimeStep(Widget w, XtPointer client_data, XtPointer call_data)
{
   TimeGizmo *          gizmo = (TimeGizmo *)client_data;
   OlTextFieldStepped * p     = (OlTextFieldStepped *)call_data;

   int                  hrs;
   int                  hour;
   int                  min;
   char *               time;

   time = GetCurrentTime(gizmo, &hour, &min);

   if (p->reason == OlSteppedIncrement)
   {
      hrs = p->count / 60;
      min += (p->count % 60);
      if (min >= 60)
      {
         hrs++;
         min -= 60;
      }
      hour += hrs;
      if (hour >= 24)
         hour -= 24;
   }
   else
   {
      hrs = p->count / 60;
      min -= (p->count % 60);
      if (min < 0)
      {
         hrs++;
         min += 60;
      }
      hour -= hrs;
      if (hour < 0)
         hour += 24;
   }

   SetCurrentTime(gizmo, hour, min);

} /* end of TimeStep */
/*
 * GetTime
 *
 * The GetTime function is used to retrieve the text
 * stored in a TimeGizmo which is in the PopupGizmo shell.
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
 *#include <TimeGizmo.h>
 * ...
 */

extern char *
GetTime(PopupGizmo * shell, int item)
{
   TimeGizmo * gizmo = (TimeGizmo *)shell->gizmos[item].gizmo;
   char *       text;
   Arg          arg[1];

   XtSetArg(arg[0], XtNstring, &text);
   XtGetValues(gizmo->textFieldWidget, arg, 1);

   return (text);          /* Must be freed */

} /* end of GetTime */
/*
 * SetTime
 *
 * The SetTime procedure is used to replace the text
 * stored in a TimeGizmo which is in the PopupGizmo shell.
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
 *#include <TimeGizmo.h>
 * ...
 */

extern void
SetTime(PopupGizmo * shell, int item, char * text, int selected)
{
   TimeGizmo * gizmo = (TimeGizmo *)shell->gizmos[item].gizmo;
   Arg         arg[10];

#ifdef DEBUG
   (void)fprintf(stderr,"setting text to '%s'.\n", text);
#endif

   XtSetArg(arg[0], XtNstring,      text);
   XtSetValues(gizmo->textFieldWidget, arg, 1);
 
   if (selected)
   {
      XtSetArg(arg[0], XtNselectStart, 0);
      XtSetArg(arg[1], XtNselectEnd,   strlen(text));
      XtSetValues(gizmo->textFieldWidget, arg, 2);
   }

} /* end of SetTime */
/*
 * FormatTime
 *
 */

extern char *
FormatTime(char * time, int * hourp, int * minp)
{
   static char buffer[10];

   char *        p;
   int           hour = 0;
   int           min  = 0;
   int           flag = 0;

   for (p = time; *p; p++)
   {
      switch(*p)
      {
         case ':':
            flag++;
            break;
         case 'a':
            break;
         case 'p':
            hour += 12;
            break;
         case 'm':
            break;
         case '0':
         case '1':
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         case '7':
         case '8':
         case '9':
            if (flag == 0)
               hour = hour * 10 + (*p - '0');
            else
               if (flag == 1)
                  min = min * 10 + (*p - '0');
               else
                  ; /* ignore seconds... */
            break;
         default:
            break;
      }
   }

   hour += (min / 60);
   min  %= 60;

   hour %= 24;

   FREE(time);

   if (hourp)
      *hourp = hour;
   if (minp)
      *minp  = min;

   sprintf(buffer, "%02d:%02d", hour, min);

   return (buffer);

} /* end of FormatTime */
/*
 * ManipulateTime
 *
 */

static void
ManipulateTime(TimeGizmo * gizmo, ManipulateOption option)
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
         STRDUP(gizmo->settings->current_value);
      break;
   case SetGizmoValue:
      if (gizmo->settings->previous_value != NULL)
         FREE(gizmo->settings->previous_value);
      gizmo->settings->previous_value = 
         STRDUP(gizmo->settings->current_value);
      break;
   case ResetGizmoValue:
      if (gizmo->settings->current_value != NULL)
         FREE(gizmo->settings->current_value);
      gizmo->settings->current_value = 
         STRDUP(gizmo->settings->previous_value);
      XtSetArg(arg[0], XtNstring, gizmo->settings->current_value);
      XtSetValues(gizmo->textFieldWidget, arg, 1);
      break;
   case ReinitializeGizmoValue:
      if (gizmo->settings->current_value != NULL)
         FREE(gizmo->settings->current_value);
      gizmo->settings->current_value = 
         STRDUP(gizmo->settings->initial_value);
      XtSetArg(arg[0], XtNstring, gizmo->settings->current_value);
      XtSetValues(gizmo->textFieldWidget, arg, 1);
      break;
   default:
      break;
   }

} /* end of ManipulateTime */
/*
 * QueryTime
 *
 */

static XtPointer
QueryTime(TimeGizmo * gizmo, int option, char * name)
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

} /* end of QueryTime */
/*
 * ManageTime
 *
 */

static void
ManageTime(TimeGizmo * gizmo, int option, char * name)
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

} /* end of ManageTime */
