#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:MenuGizmo.c	1.37"
#endif

/*
 * MenuGizmo.c
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h> 
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <Xol/OpenLook.h>
#include <Xol/PopupMenu.h>
#include <Xol/FButtons.h>

#include "Gizmos.h"
#include "MenuGizmo.h"

static char create_shell = 1;

static Widget    CreateMenu();
static Widget    CreateMenuBar();
static Gizmo     CopyMenu();
static void      FreeMenu();
static void      ManipulateMenu();
static XtPointer QueryMenu();

GizmoClassRec MenuGizmoClass[] = {
   "MenuGizmo",
   CreateMenu,     /* Create     */
   CopyMenu,       /* Copy       */
   FreeMenu,       /* Free       */
   NULL,           /* Map        */
   NULL,           /* Get        */
   NULL,           /* Get Menu   */
   NULL,           /* Build      */
   ManipulateMenu, /* Manipulate */
   QueryMenu,      /* Query      */
};

GizmoClassRec MenuBarGizmoClass[] = {
   "MenuBarGizmo",
   CreateMenuBar,  /* Create     */
   CopyMenu,       /* Copy       */
   FreeMenu,       /* Free       */
   NULL,           /* Map        */
   NULL,           /* Get        */
   NULL,           /* Get Menu   */
   NULL,           /* Build      */
   ManipulateMenu, /* Manipulate */
   QueryMenu       /* Query      */
};

/*
 * CopyMenu
 * 
 * The CopyMenu function copies a given MenuGizmo menu structure 
 * into a newly allocated structure.  This procedure is useful when a menu 
 * is to be shared across multiple base windows.
 *
 * See also:
 * 
 * FreeMenu(3), CreateMenu(3), CreateMenuBar(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <MenuGizmo.h>
 * ...
 */

static Gizmo
CopyMenu(Gizmo gizmo)
{
   MenuGizmo * old = (MenuGizmo *)gizmo;
   MenuGizmo * new = (MenuGizmo *)MALLOC(sizeof(MenuGizmo));

   register int i;

   new->help  = old->help;
   new->title = old->title;
   new->name  = old->name;

   if (old->items == NULL)
   {
       new->items = NULL;

   } else
   {
       /* Count size needed for copying the menu items */
       for (i = 0; old->items[i].label; i++)
          ;

       i++;
       new->items = (MenuItems *) CALLOC(i, sizeof(MenuItems));

       for (i = 0; old->items[i].label; i++)
       {
          new->items[i].sensitive   = old->items[i].sensitive;
          new->items[i].label       = old->items[i].label;
          new->items[i].mnemonic    = old->items[i].mnemonic;
           if (old->buttonType == CMD)
           {
               if (old->items[i].mod.nextTier != NULL)
                   new->items[i].mod.nextTier = CopyMenu(old->items[i].mod.nextTier);
               else
                   new->items[i].mod.nextTier   = NULL;
           }
           else
           {
               if (old->items[i].mod.resource_value != NULL)
                   new->items[i].mod.resource_value = old->items[i].mod.resource_value;
               else
                   new->items[i].mod.resource_value = NULL;
           }
           new->items[i].function      = old->items[i].function;
           new->items[i].client_data   = old->items[i].client_data;
           new->items[i].set           = old->items[i].set;
           new->items[i].button        = NULL;
       }
       new->items[i].label   = NULL;
   }

   new->function    = old->function;
   new->client_data = old->client_data;
   new->buttonType  = old->buttonType;
   new->layoutType  = old->layoutType;
   new->measure     = old->measure;
   new->default_item= old->default_item;
   new->parent      = NULL;
   new->child       = NULL;

   return((Gizmo)new);

} /* end of CopyMenu */
/*
 * FreeMenu
 *
 * The FreeMenu frees a given MenuGizmo menu structure.
 *
 * See also:
 * 
 * CopyMenu(3), CreateMenu(3), CreateMenuBar(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <MenuGizmo.h>
 * ...
 */

static void 
FreeMenu(Gizmo gizmo)
{
   MenuGizmo * old = (MenuGizmo *)gizmo;
   int i;

   for (i = 0; old->items[i].label; i++)
   {
      if (old->buttonType == CMD)
      {
         if (old->items[i].mod.nextTier != NULL)
            FreeMenu(old->items[i].mod.nextTier);
      }
   }

   FREE(old->items);
   FREE(old);

} /* end of FreeMenu */
/*
 * CreateMenu
 *
 * The CreateMenu function is used to create the Widget tree
 * defined by the MenuGizmo structure menu.  parent is the
 * Widget parent of this new Widget tree.  This function will traverse
 * the tree defined in menu, recursively calling itself to 
 * process the hierarchy.
 *
 * See also:
 * 
 * CopyMenu(3), FreeMenu(3), CreateMenuBar(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <MenuGizmo.h>
 * ...
 */

static Widget
CreateMenu (Widget widget, MenuGizmo * gizmo, ArgList args, int num_args)
{
   Arg      arg[100];
   Cardinal num_arg;
   static char * flatMenuFields[] = 
   {
      XtNsensitive,  /* sensitive                      */
      XtNlabel,      /* label                          */ 
      XtNuserData,   /* mnemonic string                */ 
      XtNuserData,   /* nextTier | resource_value      */
      XtNselectProc, /* function                       */
      XtNclientData, /* client_data                    */
      XtNset,        /* set                            */
      XtNpopupMenu,  /* button                         */
      XtNmnemonic,   /* mnemonic                       */ 
   };
   int          layout;
   int          measure;
   int          noneSet;
   int          exclusive;
   int          i;
   OlDefine     buttonType;

   if (gizmo->parent)
      return (gizmo->parent);

   if (create_shell) 
   {
      gizmo->parent = 
         XtCreatePopupShell(gizmo->name, popupMenuShellWidgetClass, widget, (ArgList) NULL, 0
      );
      if (gizmo->title)
      {
         XtSetArg(arg[0], XtNtitle, gizmo->title);
         XtSetValues(gizmo->parent, arg, 1);
      }
   }
   else 
   {
      gizmo->parent = widget;
      create_shell = 1;
   }

   for (i = 0; gizmo->items[i].label; i++) 
   {
      if (gizmo->items[i].mnemonic == NULL)
	  gizmo->items[i].real_mnemonic = 0;
      else
      {
	  char * mnemonic = GetGizmoText((char *)gizmo->items[i].mnemonic);
	  gizmo->items[i].real_mnemonic = (XtArgVal)mnemonic[0];
      }
      gizmo->items[i].label = GetGizmoText(gizmo->items[i].label);
      if ((gizmo->buttonType == CMD) && (gizmo->items[i].mod.nextTier != NULL))
      {
         gizmo->items[i].button =
            CreateMenu(gizmo->parent, gizmo->items[i].mod.nextTier, NULL, 0);
/*
 * FIX: change should be made to support args in the future
 */
      }
      else 
      {
         if (gizmo->items[i].function == NULL)
            gizmo->items[i].function = gizmo->function;
         gizmo->items[i].button = NULL;
      }

      if (gizmo->items[i].client_data == NULL)
         gizmo->items[i].client_data = gizmo->client_data;
   }
   layout      = gizmo->layoutType ? 
                      gizmo->layoutType : 
                      XtIsSubclass (gizmo->parent, popupMenuShellWidgetClass) ? 
                         OL_FIXEDCOLS : OL_FIXEDROWS;
   noneSet = False;
   exclusive = True;
   switch(gizmo->buttonType)
   {
      case CMD:
         noneSet = True;
         exclusive = False;
         buttonType = OL_OBLONG_BTN;
         break;
      case CNS:
         noneSet = True;
         /* fall through */
      case CHK:
         exclusive = False;
         buttonType = OL_CHECKBOX;
         break;
      case NNS:
         noneSet = True;
         /* fall through */
      case NON:
         exclusive = False;
         buttonType = OL_RECT_BTN;
         break;
      case ENS:
         noneSet = True;
         /* fall through */
      case EXC:
         exclusive = True;
         buttonType = OL_RECT_BTN;
         break;
   }

   measure     = gizmo->measure ? gizmo->measure : 1;

   XtSetArg (arg[0], XtNitemFields,    flatMenuFields);
   XtSetArg (arg[1], XtNnumItemFields, XtNumber(flatMenuFields));
   XtSetArg (arg[2], XtNitems,         gizmo->items);
   XtSetArg (arg[3], XtNnumItems,      i);
   XtSetArg (arg[4], XtNlayoutType,    layout);
   XtSetArg (arg[5], XtNmeasure,       measure);
   XtSetArg (arg[6], XtNbuttonType,    buttonType);
   XtSetArg (arg[7], XtNexclusives,    exclusive);
   XtSetArg (arg[8], XtNnoneSet,       noneSet);
   XtSetArg (arg[9], XtNdefault,       (gizmo->default_item != OL_NO_ITEM));
   XtSetArg (arg[10], XtNmenubarBehavior, False);

   num_args = AppendArgsToList(arg, 11, args, num_args);

   gizmo->child = XtCreateManagedWidget(gizmo->name, flatButtonsWidgetClass, 
      gizmo->parent, arg, num_args);

   if (gizmo->help)
      GizmoRegisterHelp(OL_WIDGET_HELP, gizmo->child,
         gizmo->help->title, OL_DESKTOP_SOURCE, gizmo->help);

   if (gizmo->default_item != OL_NO_ITEM)
   {
      OlVaFlatSetValues
         (gizmo->child, gizmo->default_item, XtNset, True, NULL);
   }
   
   return (gizmo->parent);

} /* end of CreateMenu */
/*
 * CreateMenuBar
 *
 */
static Widget
CreateMenuBar (Widget widget, MenuGizmo * gizmo, ArgList args, int num_args)
{
   create_shell = 0;
   CreateMenu(widget, gizmo, args, num_args);
   create_shell = 1;

   return gizmo->child;

} /* end of CreateMenuBar */
/*
 * GetSubMenuGizmo
 *
 * Returns menu gizmo for the next tier menu gizmo for
 * the ith menu button.
 *
 */

MenuGizmo *
GetSubMenuGizmo (MenuGizmo * menu, int item)
{

   return (menu->items[item].mod.nextTier);

} /* end of GetSubMenuGizmo */
/*
 * GetMenu
 *
 * Returns menu widget for the menu gizmo
 *
 */

Widget
GetMenu (MenuGizmo * gizmo)
{
   return gizmo->child;

} /* end of GetMenu */
/*
 * ManipulateMenu
 *
 */

static void
ManipulateMenu(Gizmo gizmo, ManipulateOption option)
{
} /* end of ManipulateMenu */
/*
 * QueryMenu
 *
 */

static XtPointer
QueryMenu(MenuGizmo * gizmo, int option, char * name)
{
   if (!name || strcmp(name, gizmo->name) == 0)
   {
      switch(option)
      {
         case GetGizmoSetting:
            return (XtPointer)(NULL);
            break;
         case GetGizmoWidget:
            return (XtPointer)(gizmo->child);
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
      int       i;
      XtPointer value = NULL;

      for (i = 0; value == NULL && gizmo->items[i].label; i++) 
      {
         if ((gizmo->buttonType == CMD) && 
             (gizmo->items[i].mod.nextTier != NULL) )
            value = 
               QueryGizmo(MenuGizmoClass, gizmo->items[i].mod.nextTier, option, name);
      }
      return (value);
   }

} /* end of QueryMenu */
