#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:STextGizmo.c	1.14"
#endif

/*
 * STextGizmo.c
 *
 */

#include <stdio.h> 

#include <X11/Intrinsic.h> 
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/StaticText.h>

#include "Gizmos.h"
#include "MenuGizmo.h"
#include "STextGizmo.h"

static Widget    CreateStaticTextGizmo();
static Gizmo     CopyStaticTextGizmo();
static void      FreeStaticTextGizmo();
static void      ManipulateStaticTextGizmo();
static XtPointer QueryStaticTextGizmo();

GizmoClassRec StaticTextGizmoClass[] = {
   "StaticTextGizmo",
   CreateStaticTextGizmo,
   CopyStaticTextGizmo,
   FreeStaticTextGizmo, 
   NULL,
   NULL,
   NULL,
   NULL,
   ManipulateStaticTextGizmo,
   QueryStaticTextGizmo,
};

/*
 * CopyStaticTextGizmo
 *
 * The CopyStaticTextGizmo function is used to create a copy
 * of a given StaticTextGizmo ms.
 *
 * See also:
 *
 * FreeStaticTextGizmo(3), CreateStaticTextGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <STextGizmo.h>
 * ...
 */

static Gizmo 
CopyStaticTextGizmo(gizmo)
StaticTextGizmo * gizmo;
{
   StaticTextGizmo * new = (StaticTextGizmo*)MALLOC(sizeof(StaticTextGizmo));

   new-> help    = gizmo-> help;
   new-> name    = gizmo->name;
   new-> text    = gizmo->text;
   new-> gravity = gizmo-> gravity;
   new-> font    = gizmo->font;
   new-> widget  = NULL;

   return ((Gizmo)new);

} /* end of CopyStaticTextGizmo */
/*
 * FreeStaticTextGizmo
 *
 * The FreeStaticTextGizmo procedure is used free the StaticTextGizmo ms.
 *
 * See also:
 *
 * CopyStaticTextGizmo(3), CreateStaticTextGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <STextGizmo.h>
 * ...
 */

static void 
FreeStaticTextGizmo(gizmo)
StaticTextGizmo * gizmo;
{

   FREE(gizmo);

} /* end of FreeStaticTextGizmo */
/*
 * CreateStaticTextGizmo
 *
 * The CreateStaticTextGizmo function is used to create the Widget tree
 * defined by the StaticTextGizmo structure p.  parent is the
 * Widget parent of this new Widget tree.  args and num,
 * if non-NULL, are used as Args in the creation of the popup window
 * Widget which is returned by this function.
 *
 * Standard Appearance:
 *
 * The CreateStaticTextGizmo function creates a standard modal dialog
 * (modalShell).  A staticText widget is created and its string
 * resource set.  The second child to the modalShell is a flatButtons
 * pased in which is constructed as a menu bar in the lowerArea.  A
 * typical window appears as:~
 *
 * See also:
 *
 * CopyStaticTextGizmo(3), FreeStaticTextGizmo(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <STextGizmo.h>
 * ...
 */

static Widget
CreateStaticTextGizmo (parent, gizmo, args, num)
Widget parent;
StaticTextGizmo * gizmo;
ArgList args;
int num;
{
   Arg           arg[100];
   Cardinal      num_arg;
   XFontStruct * bigger_font;
   int           n;

   n = 0;
   if (gizmo->font != NULL)
   {
      XrmValue from;
      XrmValue to;
      from.addr = (caddr_t)gizmo->font;
      from.size = strlen(gizmo->font);
      to.addr   = (caddr_t)&bigger_font;
      to.size   = sizeof(XFontStruct *);
      XtConvertAndStore(parent, XtRString, &from, XtRFontStruct, &to);
      XtSetArg(arg[n], XtNfont,    bigger_font); n++;
   }

   XtSetArg(arg[n], XtNstring,  GetGizmoText(gizmo->text)); n++;
   XtSetArg(arg[n], XtNgravity, gizmo->gravity); n++;
   num_arg = AppendArgsToList(arg, n, args, num);
   gizmo->widget = 
      XtCreateManagedWidget(gizmo->name, staticTextWidgetClass, parent, arg, num_arg);

   if (gizmo->help)
      GizmoRegisterHelp(OL_WIDGET_HELP, gizmo->widget,
         gizmo->help->title, OL_DESKTOP_SOURCE, gizmo->help);

   return (gizmo->widget);

} /* end of CreateStaticTextGizmo */
/*
 * SetStaticTextGizmoMessage
 *
 */

extern void
SetStaticTextGizmoText(gizmo, text)
StaticTextGizmo * gizmo;
char * text;
{
   Arg arg[2];

   XtSetArg(arg[0], XtNstring, text);
   XtSetValues(gizmo->widget, arg, 1);

} /* end of SetStaticTextGizmoMessage */
/*
 * GetStaticTextGizmo
 *
 */

extern Widget
GetStaticTextGizmo(gizmo)
StaticTextGizmo * gizmo;
{

   return(gizmo->widget);

} /* end of GetStaticTextGizmo */
/*
 * ManipulateStaticTextGizmo
 *
 */

static void
ManipulateStaticTextGizmo(gizmo, option)
Gizmo           gizmo;
ManipulateOption option;
{
} /* end of ManipulateStaticTextGizmo */
/*
 * QueryStaticTextGizmo
 *
 */

static XtPointer
QueryStaticTextGizmo(StaticTextGizmo * gizmo, int option, char * name)
{
   if (!name || strcmp(name, gizmo->name) == 0)
   {
      switch(option)
      {
         case GetGizmoSetting:
            return (XtPointer)(NULL);
            break;
         case GetGizmoWidget:
            return (XtPointer)(gizmo->widget);
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

} /* end of QueryStaticTextGizmo */
