#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:ChoiceGizm.c	1.25"
#endif

/*
 * ChoiceGizmo.c
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <buffutil.h>

#include <Xol/OpenLook.h>
#include <Xol/FButtons.h>
#include <Xol/Caption.h>
#include <Xol/StaticText.h>
#include <Xol/AbbrevFMen.h>
#include <Xol/ControlAre.h>

#include "Gizmos.h"
#include "MenuGizmo.h"
#include "ChoiceGizm.h"

#define WIDTH		100
#define XtRChoiceGizmo	"ChoiceGizmo"

static Widget    CreateChoice();
static Widget    CreateAbbrevChoice();
static Gizmo     CopyChoice();
static void      FreeChoice();
static void      ManipulateAbbrevChoice();
static void      ManipulateChoice();
static XtPointer QueryChoice();
static void      ManageChoice();
static void      ChoiceUnconverter(ChoiceGizmo *);

static void      ChoiceConverter();
static int       ValueToCode();
static void      SetItems(ChoiceGizmo * gizmo, MenuItems * menu_items, int option);

GizmoClassRec ChoiceGizmoClass[] = 
   { 
      "ChoiceGizmo",
      CreateChoice,          /* Create     */
      CopyChoice,            /* Copy       */
      FreeChoice,            /* Free       */
      NULL,                  /* Map        */
      NULL,                  /* Get        */
      NULL,                  /* Get Menu   */
      NULL,                  /* Build      */
      ManipulateChoice,      /* Manipulate */
      QueryChoice,           /* Query      */
      ManageChoice,          /* Manage     */
   };

GizmoClassRec AbbrevChoiceGizmoClass[] = 
   { 
      "AbbrevChoiceGizmo",
      CreateAbbrevChoice,    /* Create     */
      CopyChoice,            /* Copy       */
      FreeChoice,            /* Free       */
      NULL,                  /* Map        */
      NULL,                  /* Get        */
      NULL,                  /* Get Menu   */
      NULL,                  /* Build      */
      ManipulateAbbrevChoice,/* Manipulate */
      QueryChoice,           /* Query      */
      ManageChoice,          /* Manage     */
   };

static char abbreviated = False;


/*
 * CopyChoice
 *
 */

static Gizmo
CopyChoice(Gizmo gizmo)
{
   ChoiceGizmo * old   = (ChoiceGizmo *)gizmo;
   ChoiceGizmo * new   = (ChoiceGizmo *)MALLOC(sizeof(ChoiceGizmo));

   new->help           = old->help;
   new->name           = old->name;
   new->caption        = old->caption;
   new->menu           = CopyGizmo(MenuGizmoClass, old->menu);
   new->settings       = (Setting *)CALLOC(1, sizeof(Setting));
   if (old->settings) {
   	new->settings->current_value  = old->settings->current_value;
   	new->settings->previous_value = old->settings->previous_value;
   	new->settings->initial_value  = old->settings->initial_value;
   	new->settings->flag           = old->settings->flag;
   }
   new->verify         = old->verify;
   new->args           = old->args;
   new->num_args       = old->num_args;
   new->captionWidget  = NULL;
   new->buttonsWidget  = NULL;
   new->previewWidget  = NULL;

   return ((Gizmo)new);

} /* end of CopyChoice */
/*
 * FreeChoice
 *
 */

static void
FreeChoice(Gizmo gizmo)
{
   ChoiceGizmo * old = (ChoiceGizmo *)gizmo;

   old->name;
   old->caption;
   FreeGizmo(MenuGizmoClass, old->menu);
   FREE(old->settings);
   FREE(old);

} /* end of FreeChoice */
/*
 * CreateAbbrevChoice
 *
 */

static Widget 
CreateAbbrevChoice(Widget promptArea, ChoiceGizmo * gizmo, ArgList args, int num_args)
{
   Widget w;

   abbreviated = True;
   w = CreateChoice(promptArea, gizmo, args, num_args);
   abbreviated = False;

   return w;

} /* end of CreateAbbrevChoice */
/*
 * CreateChoice
 *
 */

static Widget
CreateChoice (Widget promptArea, ChoiceGizmo * gizmo, ArgList args, int num_args)
{
   Arg           arg[SIZE_ARGS];
   Cardinal      num_arg;
   char *        name = gizmo->name ? gizmo->name : "buttons";
   MenuGizmo *   menu = (MenuGizmo *)gizmo->menu;
   char *        previous_value = (char *)gizmo->settings->previous_value;
   int           i;

   gizmo->caption = GetGizmoText(gizmo->caption);

   XtSetArg (arg[0], XtNlabel, gizmo->caption);
   num_arg = AppendArgsToList(arg, 1, gizmo->args, gizmo->num_args);
   gizmo->captionWidget = 
      XtCreateManagedWidget(name, captionWidgetClass, promptArea, arg, num_arg);

   if (gizmo->help)
      GizmoRegisterHelp(OL_WIDGET_HELP, gizmo->captionWidget, 
         gizmo->help->title, OL_DESKTOP_SOURCE, gizmo->help);

   /*
    * set XtNset to reflect "previous_value" of settings
    */

   switch (menu->buttonType)
   {
      case EXC:
      case ENS:
         for (i = 0; menu->items[i].label; i++)
         {
            menu->items[i].set = ((int)previous_value == i);
            if (abbreviated && !menu->function && !menu->items[i].function)
            {
               menu->items[i].function = SetPreview;
               menu->items[i].client_data = (XtPointer)gizmo;
            }
         }
#ifdef DEBUG
         fprintf(stderr,"exclusive = %d\n", previous_value);
#endif
         break;
      case CNS:
      case CHK:
      case NNS:
      case NON:
         gizmo->settings->current_value = previous_value ?
					  STRDUP(previous_value) : NULL;
         for (i = 0; menu->items[i].label; i++)
         {
            menu->items[i].set = previous_value ? (previous_value[i] == 'x'):0;
            if (abbreviated && !menu->function && !menu->items[i].function)
            {
               menu->items[i].function = SetPreview;
               menu->items[i].client_data = (XtPointer)gizmo;
            }
         }
#ifdef DEBUG
         fprintf(stderr,"non-exclusive = %s\n", previous_value);
#endif
         break;
      default:
         break;
   }

   if (abbreviated)
   {
      Widget ctlarea;
      Widget abbrev;
      Widget shell;
      MenuGizmo * menu;

      shell =
         CreateGizmo(gizmo->captionWidget, MenuGizmoClass, 
                     (Gizmo)gizmo->menu, args, num_args);

      menu = (MenuGizmo *)gizmo->menu;
      gizmo->buttonsWidget = menu->child;

      ctlarea = XtCreateManagedWidget("_X_", controlAreaWidgetClass, 
         gizmo->captionWidget, arg, 0);

      XtSetArg(arg[0], XtNpopupWidget, shell);
      abbrev  = XtCreateManagedWidget("_X_", abbreviatedButtonWidgetClass,
         ctlarea, arg, 1);

      XtSetArg(arg[0], XtNstring, 
         menu->items[(int)gizmo->settings->previous_value].label);
      XtSetArg(arg[1], XtNwidth,        WIDTH);
      gizmo->previewWidget = XtCreateManagedWidget("_X_", staticTextWidgetClass,
         ctlarea, arg, 1);
      XtSetArg(arg[0], XtNpreviewWidget, gizmo->previewWidget);
      XtSetValues(abbrev, arg, 1);
   }
   else
   {
      gizmo->buttonsWidget = 
         CreateGizmo(gizmo->captionWidget, MenuBarGizmoClass, 
                     (Gizmo)gizmo->menu, args, num_args);
   }

   return(gizmo->captionWidget);

} /* end of CreateChoice */
/*
 * GetChoiceButtons
 *
 */

extern Widget
GetChoiceButtons(ChoiceGizmo * gizmo)
{

   return (gizmo->buttonsWidget);

} /* end of GetChoiceButtons */

/*
 * ManipulateAbbrevChoice
 *
 */

static void
ManipulateAbbrevChoice(ChoiceGizmo * gizmo, ManipulateOption option)
{

   abbreviated = True;
   ManipulateChoice(gizmo, option);
   abbreviated = False;

} /* end of ManipulateAbbrevChoice */
/*
 * ManipulateChoice
 *
 */

static void
ManipulateChoice(ChoiceGizmo * gizmo, ManipulateOption option)
{
   MenuItems * p;
   MenuGizmo * menu = (MenuGizmo *)(gizmo->menu);

   XtConvertArgRec * cargs;
   char *            string;
   int               i;

   switch (option)
   {
   case GetGizmoValue:
      switch(menu->buttonType)
      {
         case NON:
         case CNS:
         case CHK:
         case NNS:
            string = (char *)gizmo->settings->current_value;
            for (i = 0, p = menu->items; p->label; i++, p++)
               string[i] = (p->set) ? 'x' : '_';
            break;
         case ENS:
         case EXC:
            for (p = menu->items; p->label; p++)
               if (p->set)
                  break;
            if (p-> label)
               gizmo->settings->current_value = (XtPointer)(p - menu->items);
            else
               gizmo->settings->current_value = (XtPointer)OL_NO_ITEM;
            break;
      }
      break;
   case ApplyGizmoValue:
   case SetGizmoValue:
      switch(menu->buttonType)
      {
         case NON:
         case CNS:
         case CHK:
         case NNS:
            if (gizmo->settings->previous_value)
               FREE(gizmo->settings->previous_value);
            gizmo->settings->previous_value = 
               STRDUP(gizmo->settings->current_value);
            break;
         case ENS:
         case EXC:
            gizmo->settings->previous_value = gizmo->settings->current_value;
            break;
      }
      if (option == SetGizmoValue)
         ChoiceUnconverter(gizmo);
      break;
   case ResetGizmoValue:
   case ReinitializeGizmoValue:
      SetItems(gizmo, menu->items, option);
      break;
   case RegisterGizmoConverter:
      cargs = (XtConvertArgRec *)MALLOC(sizeof(XtConvertArgRec));

      cargs->address_mode = XtImmediate;
      cargs->address_id   = (XtPointer)gizmo;
      cargs->size         = sizeof(Gizmo);
#ifdef DEBUG
      fprintf(stderr,"adding converter for '%s' (%x)\n", gizmo->name, gizmo);
#endif
      XtAddConverter(XtRString, gizmo->name, ChoiceConverter, cargs, 1);
      break;
   default:
      break;
   }

} /* end of ManipulateChoice */
/*
 * QueryChoice
 *
 */

static XtPointer
QueryChoice(ChoiceGizmo * gizmo, int option, char * name)
{
   if (!name || strcmp(name, gizmo->name) == 0)
   {
      switch(option)
      {
         case GetGizmoSetting:
            return (XtPointer)(gizmo->settings);
            break;
         case GetGizmoWidget:
            return (XtPointer)(gizmo->buttonsWidget);
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

} /* end of QueryChoice */
/*
 * ChoiceUnconverter
 *
 */

static void
ChoiceUnconverter(ChoiceGizmo * gizmo)
{
   MenuGizmo * menu  = (MenuGizmo *)gizmo->menu;
   Buffer *    value = NULL;
   int         i;
   int         len;

   for (i = 0; menu->items[i].label; i++)
      if (menu->items[i].set)
      {
         len = strlen(menu->items[i].mod.resource_value);
         if (value == NULL)
         {
            value = AllocateBuffer(sizeof(char), len + 1);
            (void)strcpy((char *)value->p, menu->items[i].mod.resource_value);
            value->used += (len + 1);
         }
         else
         {
            GrowBuffer(value, len + 3);
            (void)strcat((char *)value->p, "|");
            (void)strcat((char *)value->p, menu->items[i].mod.resource_value);
            value->used += (len + 3);
         }
      }
  
   AppendToResourceBuffer((*GetGizmoAppName)(), gizmo->name, value ? value->p : NULL);

} /* end of ChoiceUnconverter */
/*
 * ChoiceConverter
 *
 */

static void
ChoiceConverter(args, num_args, from, to)
XrmValue * args;
Cardinal num_args;
XrmValue * from;
XrmValue * to;
{
   static int       mode;
   static char *    mask = NULL;
   char *           p = from->addr;
   char *           c;
   int              i;

   if (args == NULL)
      XtStringConversionWarning(from->addr, XtRChoiceGizmo);
   else
   {
      ChoiceGizmo * gizmo = (ChoiceGizmo *)(*((ChoiceGizmo **)(args[0].addr)));
      MenuGizmo *   menu  = (MenuGizmo *)(gizmo->menu);

#ifdef DEBUG
      fprintf(stderr," converting '%s' for choice '%s' (%x)\n", p, gizmo->name, gizmo);
#endif
      switch (menu->buttonType)
      {
         case EXC:
         case ENS:
            mode = ValueToCode(p, menu->items, OL_NO_ITEM);
#ifdef DEBUG
            fprintf(stderr,"exclusive = %d\n", mode);
#endif
            to->addr = (XtPointer)&mode;
            to->size = sizeof(XtPointer);
            break;
         case CNS:
         case CHK:
         case NNS:
         case NON:
            for (i = 0; menu->items[i].label; i++)    ;
            mask = REALLOC(mask, i);
            memset(mask, '_', i - 1);

            for (c = strtok(p, "|"); c != NULL; c = strtok(NULL, "|"))
            {
#ifdef DEBUG
               fprintf(stderr,"c = '%s' \n", c);
#endif
               mode = ValueToCode(c, menu->items, i);
               if (mode < i)
                  mask[mode] = 'x';
            }
#ifdef DEBUG
            fprintf(stderr,"nonexclusive = '%s'\n", mask);
#endif
            to->addr = (XtPointer)&mask;
            to->size = sizeof(XtPointer);
            break;
         default:
            break;
      }
   }

} /* end of ChoiceConverter */
/*
 * ValueToCode
 *
 */

static int
ValueToCode(char * value, MenuItems * items, int nomatch_code)
{
   register i;

   /*
    * can add check for substring of label (up to size of value provided)
    */

   for (i = 0; items[i].label; i++)
      if (items[i].mod.resource_value && 
          (strcmp(value, items[i].mod.resource_value) == 0))
         return (i);

   return (nomatch_code);

} /* end of ValueToCode */
/*
 * SetItems
 *
 */

static void
SetItems(ChoiceGizmo * gizmo, MenuItems * menu_items, int option)
{
   Arg         arg_on[2];
   Arg         arg_off[2];
   XtPointer * old;
   XtPointer * new;
   MenuGizmo * menu   = gizmo->menu;
   Widget      widget = menu->child;
   MenuGizmo * p;
   int         i;
   int         flag;

   char *      old_string;
   char *      new_string;

   XtSetArg(arg_on[0], XtNset, True);
   XtSetArg(arg_off[0], XtNset, False);

   if (option == ResetGizmoValue)
   {
      old = &gizmo->settings->current_value;
      new = &gizmo->settings->previous_value;
   }
   else
   {
      old = &gizmo->settings->current_value;
      new = &gizmo->settings->initial_value;
   }

   switch(menu->buttonType)
   {
      case ENS:
         if ((Cardinal)*new == OL_NO_ITEM)
	 {
            if ((Cardinal)*old != OL_NO_ITEM)
               OlFlatSetValues(widget, (Cardinal)*old, arg_off, 1);
	 }
         else
            OlFlatSetValues(widget, (Cardinal)*new, arg_on, 1);
         *old = *new;
         break;
      case EXC:
         if (abbreviated)
         {
            MenuGizmo * menu = (MenuGizmo *)gizmo->menu;
            Arg         arg[5];

            XtSetArg(arg[0], XtNstring, menu->items[(int)*new].label);
            XtSetArg(arg[1], XtNwidth,   WIDTH);
            XtSetValues(gizmo->previewWidget, arg, 2);
         }
         OlFlatSetValues(widget, (Cardinal)*new, arg_on, 1);
         *old = *new;
         break;
      case NON:
      case CNS:
      case CHK:
      case NNS:
            flag       = False;

            old_string = (char *)*old;
            new_string = (char *)*new;

            for (i = 0; old_string[i]; i++)
            {
               if (old_string[i] != new_string[i])
               {
                  flag = True;
               }
               OlFlatSetValues(widget, i, 
                  'x' == new_string[i] ? arg_on : arg_off, 1);
            }
            if (flag)
            {
               FREE(old_string);
               *old = (XtPointer)STRDUP(new_string);
            }
         break;
   }
      
} /* end of SetItems */
/*
 * ManageChoice
 *
 */

static void
ManageChoice(ChoiceGizmo * gizmo, int option, char * name)
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
            break;
         default:
            break;
      }

} /* end of ManageChoice */
/*
 * SetPreview
 *
 */

extern void
SetPreview(Widget widget, XtPointer client_data, XtPointer call_data)
{
   ChoiceGizmo *    gizmo = (ChoiceGizmo *)client_data;
   MenuGizmo *      menu  = (MenuGizmo *)gizmo->menu;
   OlFlatCallData * p     = (OlFlatCallData *)call_data;

   Arg         arg[5];

   XtSetArg(arg[0], XtNstring, menu->items[p->item_index].label);
   XtSetArg(arg[1], XtNwidth,  WIDTH);
   XtSetValues(gizmo->previewWidget, arg, 1);

} /* end of SetPreview */
