#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:MenuGizmo.h	1.17"
#endif

/*
 * MenuGizmo.h
 *
 */

#ifndef _MenuGizmo_h
#define _MenuGizmo_h

/*
 * Menu
 *
 * The MenuGizmo is used to construct, optionally tiered, menu
 * interface elements.  The Menu Gizmo is a comprized of two
 * data structures: a List of MenuItems used to describe the
 * per-item information for the MenuButtons (and is used as the XtNitems
 * resource of the FlatButtons Widget in the PopupShell) and the Menu
 * structure that describes attributes of the popup shell widget and
 * for the FlatButtons container.
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <MenuGizmo.h>
 * ... 
 */

typedef enum { CMD, CNS, CHK, NNS, NON, ENS, EXC } buttonTypes;

typedef struct MenuItems 
{
   XtArgVal            sensitive;      /* sensitivity state                  */
   char *              label;          /* the label appearing in the button  */
   char *              mnemonic;       /* the button's mnemonic *string*     */
   union 
   {
      char *           resource_value; /* the resource_value for Xdefaults   */
      struct _MenuGizmo * nextTier;    /* pointer to a cascading Menu        */
   } mod;
   void                (*function)();  /* selectProc for the button          */
   char *              client_data;    /* clientData passed from selectProc  */
   XtArgVal            set;            /* the current state                  */
   Widget              button;         /* the popupMenu (if any - returned)  */
   XtArgVal            real_mnemonic;  /* the mnemonic for the button        */
} MenuItems;

typedef struct _MenuGizmo
{
   HelpInfo *          help;          /* help information                   */
   char *              name;          /* the name of the shell widget       */
   char *              title;         /* the title for the menu implies pin */
   MenuItems *         items;         /* pointer to the MenuItems           */
   void                (*function)(); /* the default selectProc             */
   char *              client_data;   /* the default clientData             */
   int                 buttonType;    /* the type of button (rect, check, ) */
   int                 layoutType;    /* OL_FIXEDROWS or OL_FIXEDCOLS       */
   int                 measure;       /* number of rows or cols             */
   Cardinal            default_item;  /* the default menu item index        */
   Widget              parent;        /* (return) menu's parent             */
   Widget              child;         /* (return) FlatButtons Widget        */
} MenuGizmo;

extern GizmoClassRec MenuGizmoClass[];
extern GizmoClassRec MenuBarGizmoClass[];

extern Widget       GetMenu(MenuGizmo * menu);
extern MenuGizmo *  GetSubMenuGizmo(MenuGizmo * menu, int item);

/*
 * The following function is defined in Gizmo.c.  It's been 
 * put here because Gizmo.h can't include MenuGizmo.h.
 *
 */

extern MenuGizmo *  GetMenuGizmo(GizmoClass gizmoClass, void * shell);

#endif /* _MenuGizmo_h */
