/*		copyright	"%c%" 	*/


#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:ListGizmo.c	1.25"
#endif

/*
 * ListGizmo.c
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/Form.h>
#include <Xol/FList.h>
#include <Xol/ScrolledWi.h>

#include "Gizmos.h"
#include "PopupGizmo.h"
#include "ListGizmo.h"

static Gizmo     CopyListGizmo(Gizmo gizmo);
static void      FreeListGizmo (Gizmo gizmo);
static void      CopyList (ListHead * src, ListHead * dest);
static Widget    CreateListGizmo (Widget parent, ListGizmo * gizmo);
static void      UpdateList(ListGizmo * gizmo);
static void      ManipulateList(ListGizmo * gizmo, ManipulateOption option);
static XtPointer QueryList(ListGizmo * gizmo, int option, char * name);

GizmoClassRec ListGizmoClass[] =
{
   "ListGizmo",
   CreateListGizmo, /* Create      */
   CopyListGizmo,   /* Copy        */
   FreeListGizmo,   /* Free        */
   NULL,            /* Map         */
   NULL,            /* GetGiz      */
   NULL,            /* GetMenu     */
   NULL,            /* Build       */
   ManipulateList,  /* Manipulate  */
   QueryList,       /* Query       */
};

static String flatListFields[] =
{
   XtNset,
   XtNformatData,
   XtNclientData
};

/*
 * CopyListGizmo
 *
 */

static Gizmo
CopyListGizmo(Gizmo gizmo)
{
   ListGizmo * old = (ListGizmo *)gizmo;
   ListGizmo * new = (ListGizmo *)MALLOC(sizeof(ListGizmo));

   new->help                     = old->help;
   new->name                     = old->name;
   new->caption                  = old->caption;
   new->format                   = old->format;
   new->exclusive                = old->exclusive;
   new->height                   = old->height;
   new->settings                 = (Setting *)MALLOC(sizeof(Setting));
   new->settings->current_value  = (XtPointer)CALLOC(1, sizeof(ListHead));
   new->settings->previous_value = (XtPointer)CALLOC(1, sizeof(ListHead));
   new->settings->initial_value  = (XtPointer)CALLOC(1, sizeof(ListHead));
   if (old->settings) {
	if (old->settings->current_value) {
   		((ListHead *)new->settings->current_value)->list =
                        ((ListHead *)old->settings->current_value)->list;
   		((ListHead *)new->settings->current_value)->size =
                        ((ListHead *)old->settings->current_value)->size;
   		((ListHead *)new->settings->current_value)->numFields =
                        ((ListHead *)old->settings->current_value)->numFields;
	}
	if (old->settings->previous_value) {
   		((ListHead *)new->settings->previous_value)->list =
                        ((ListHead *)old->settings->previous_value)->list;
   		((ListHead *)new->settings->previous_value)->size =
                        ((ListHead *)old->settings->previous_value)->size;
   		((ListHead *)new->settings->previous_value)->numFields =
                        ((ListHead *)old->settings->previous_value)->numFields;
	}
	if (old->settings->initial_value) {
   		((ListHead *)new->settings->initial_value)->list =
                        ((ListHead *)old->settings->initial_value)->list;
   		((ListHead *)new->settings->initial_value)->size =
                        ((ListHead *)old->settings->initial_value)->size;
   		((ListHead *)new->settings->initial_value)->numFields =
                        ((ListHead *)old->settings->initial_value)->numFields;
	}

        new->settings->flag = old->settings->flag;
   }
   else
	new->settings->flag = 0;

   new->executeCB                = old->executeCB;
   new->selectCB                 = old->selectCB;
   new->unselectCB               = old->unselectCB;
   new->args                     = old->args;
   new->num_args                 = old->num_args;
   new->flatList                 = NULL;
   new->font                     = old->font;

   return (Gizmo)new;

} /* end of CopyListGizmo */
/*
 * FreeList
 *
 * Frees a flat list.
 *
 */

extern void
FreeList(ListHead * flist)
{
   int        i;
   int        j;
   ListItem * fp;
   char **    tmp;

   if (flist == (ListHead *)0) {
      return;
   }
   fp = flist->list;
   for (i=0; i<flist->size; i++)
   {
      tmp = (char **)fp[i].fields;
      for (j=0; j<flist->numFields; j++)
      {
         if (tmp[j] != NULL)
         {
            FREE (tmp[j]);
         }
      }
      FREE ((char *)(fp[i].fields));
   }
   if (fp)
      FREE (fp);

} /* end of FreeList */
/*
 * FreeListGizmo
 *
 */

static void 
FreeListGizmo(Gizmo gizmo)
{
   ListGizmo * old = (ListGizmo *)gizmo;

   if (old->copy_settings == (XtArgVal)True && old->settings != (Setting *)0)
   {
      FreeList ((ListHead *)old->settings->initial_value);
      FreeList ((ListHead *)old->settings->previous_value);
      FreeList ((ListHead *)old->settings->current_value);
   }
   FREE(old->settings);
   if (old->font)
      FREE(old->font);
   FREE(old);

} /* end of FreeListGizmo */
/*
 * CopyList
 *
 * Copies a first list to second list.
 *
 */

static void
CopyList(ListHead * src, ListHead * dest)
{
   int     i;
   int     j;
   char ** stmp;
   char ** dtmp;

   if (src == NULL) {
	dest = NULL;
	return;
   }
   /* Alloc space for the destination list */
   dest->list = (ListItem *)MALLOC(sizeof(ListItem)*src->size);

   dest->size = src->size;
   dest->numFields = src->numFields;

   for (i=0; i<src->size; i++)
   {
      /* Alloc space for the fields in dest */
      dest->list[i].fields = 
         (XtArgVal)MALLOC(sizeof(XtArgVal ) * src->numFields);

      dest->list[i].set = (XtArgVal)src->list[i].set;
      dest->list[i].clientData = (XtArgVal)src->list[i].clientData;

      stmp = (char **)src->list[i].fields;
      dtmp = (char **)dest->list[i].fields;
      for (j=0; j<src->numFields; j++)
      {
         dtmp[j] = STRDUP (stmp[j]);
      }
   }

} /* end of CopyList */
/*
 * CreateListGizmo
 *
 * Creates a flat scrolling list.
 */

static Widget
CreateListGizmo(Widget parent, ListGizmo * gizmo)
{
   Arg           arg[100];
   Cardinal      num_arg;
   int           i;
   Widget        scrolledWindow;
   ListHead *    hp = (ListHead *)NULL;
   ListHead *    current;
   ListHead *    previous;
   ListHead *    initial;
   XrmValue      to;
   XrmValue      from;
   XFontStruct * large_font = NULL;

   if (gizmo->font != NULL)
   {
      from.addr = (caddr_t)gizmo->font;
      from.size = strlen((char *)from.addr);
      to.addr   = (caddr_t)&large_font;
      to.size   = sizeof(XFontStruct *);
      XtConvertAndStore(parent, XtRString, &from, XtRFontStruct, &to);
   }

   if (gizmo->settings) {
      initial = (ListHead *)gizmo->settings->initial_value;
      previous = (ListHead *)gizmo->settings->previous_value;
      current = (ListHead *)gizmo->settings->current_value;
   }
   else {
      initial = (ListHead *)NULL;
      previous = (ListHead *)NULL;
      current = (ListHead *)NULL;
   }
   hp = current;

   if (gizmo->copy_settings == (XtArgVal)True)
   {
      if (current != (ListHead *)0)
      {
         CopyList (current, previous);
         CopyList (current, initial);
      }
   }
   scrolledWindow = 
      XtCreateManagedWidget("_X_", scrolledWindowWidgetClass, parent, NULL, 0);

   if (gizmo->help)
      GizmoRegisterHelp(OL_WIDGET_HELP, scrolledWindow,
         gizmo->help->title, OL_DESKTOP_SOURCE, gizmo->help);

   i = 0;
   if (hp) {
      XtSetArg (arg[ i], XtNitems,         hp->list); i++;
      XtSetArg (arg[ i], XtNnumItems,      hp->size); i++;
   }
   else {
      XtSetArg (arg[ i], XtNitems,         0); i++;
      XtSetArg (arg[ i], XtNnumItems,      0); i++;
   }
   XtSetArg (arg[i], XtNformat,        gizmo->format); i++;
   XtSetArg (arg[i], XtNexclusives,    gizmo->exclusive); i++;
   XtSetArg (arg[i], XtNitemFields,    flatListFields); i++;
   XtSetArg (arg[i], XtNnumItemFields, XtNumber(flatListFields)); i++;
   XtSetArg (arg[i], XtNselectProc,    gizmo->selectCB); i++;
   XtSetArg (arg[i], XtNunselectProc,  gizmo->unselectCB); i++;
   XtSetArg (arg[i], XtNdblSelectProc, gizmo->executeCB); i++;
   XtSetArg (arg[i], XtNviewHeight,    gizmo->height); i++;
   if (large_font != (XFontStruct *)0) {
   	XtSetArg (arg[i], XtNfont,          large_font); i++;
   }
   num_arg = 
      AppendArgsToList(arg, i, gizmo->args, gizmo->num_args);
   gizmo->flatList = 
      XtCreateManagedWidget(gizmo->name, flatListWidgetClass, 
         scrolledWindow, arg, num_arg);

   return scrolledWindow;

} /* end of CreateListGizmo */
/*
 * GetListField
 *
 */

extern char *
GetListField(ListGizmo *gizmo, int item)
{
   ListHead * hp;

   if (gizmo                          != (ListGizmo *)0 &&
       gizmo->settings                != (Setting *)0 &&
       gizmo->settings->current_value != (ListHead *)0) {
      hp = (ListHead *)(gizmo->settings->current_value);
      return (char *)hp->list[item].fields;
   }
   return NULL;

} /* end of GetListField */
/*
 * GetList
 *
 */

extern Widget
GetList(ListGizmo *gizmo)
{

   return gizmo->flatList;

} /* end of GetList */
/*
 * UpdateList
 *
 */

static void
UpdateList(ListGizmo * gizmo)
{
   ListHead * hp;
   Arg        arg[10];

   if (gizmo                          != (ListGizmo *)0 &&
       gizmo->settings                != (Setting *)0 &&
       gizmo->settings->current_value != (ListHead *)0) {
      hp = (ListHead *)(gizmo->settings->current_value);
   }
   else {
      return;
   }

   XtSetArg(arg[0], XtNnumItems,   hp->size);
   XtSetArg(arg[1], XtNitems,      hp->list);
   XtSetArg(arg[2], XtNviewHeight, gizmo->height);
   XtSetValues(GetList(gizmo), arg, 3);

} /* end of UpdateList */
/*
 * ManipulateList
 *
 */

static void
ManipulateList(ListGizmo * gizmo, ManipulateOption option)
{
   ListHead * current  = (ListHead *)gizmo->settings->current_value;
   ListHead * previous = (ListHead *)gizmo->settings->previous_value;
   ListHead * initial  = (ListHead *)gizmo->settings->initial_value;

   switch (option)
   {
      case GetGizmoValue:
         break;
      case ApplyGizmoValue:
         /* Free the previous list and copy the
          * current list to previous.
          */
      case SetGizmoValue:
         FreeList (previous);
         CopyList (current, previous);
         UpdateList (gizmo);
         break;
      case ResetGizmoValue:
         /* Free the current list and copy the
          * previous list to current.
          */
         FreeList (current);
         CopyList (previous, current);
         UpdateList (gizmo);
         break;
      case ReinitializeGizmoValue:
         /* Free the current list and copy the
          * initial list to current.
          */
         FreeList (current);
         FreeList (previous);
         CopyList (initial, current);
         CopyList (initial, previous);
         UpdateList (gizmo);
         break;
      default:
         break;
   }

} /* end of ManipulateList */
/*
 * QueryList
 *
 */

static XtPointer
QueryList(ListGizmo * gizmo, int option, char * name)
{
   if (!name || strcmp(name, gizmo->name) == 0)
   {
      switch(option)
      {
         case GetGizmoSetting:
            return (XtPointer)(gizmo->settings);
            break;
         case GetGizmoWidget:
            return (XtPointer)(gizmo->flatList);
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

} /* end of QueryList */
/*
 * InsertListItem
 *
 * Insert a new item into the flat list pointed to by hp.
 * fields get added to this new item if numfields is nonzero,
 * otherwise a null fields gets added.
 */

extern void
InsertListItem(ListGizmo * gizmo, int item, char ** fields, int numfields)
{
   int        i;
   ListHead * hp;
   char **    tmp;

   if (gizmo                          != (ListGizmo *)0 &&
       gizmo->settings                != (Setting *)0 &&
       gizmo->settings->current_value != (ListHead *)0) {
      hp = (ListHead *)(gizmo->settings->current_value);
   }
   else {
      return;
   }

   /* Move up a line before the current item for a blank item */

   i = hp->size++;
   if (i == 0)
   {
      hp->list = (ListItem *)MALLOC(sizeof (ListItem) * 1);
   }
   else
   {
      hp->list = (ListItem *)REALLOC(hp->list, sizeof (ListItem) * hp->size);
   }

   /* Move all items down by one location */

   if (i != 0)
   {
      for (; i>item; i-- )
      {
        hp->list[i].set    = hp->list[i-1].set;
        hp->list[i].fields = hp->list[i-1].fields;
      }
   }
   hp->list[i].set = True;
   hp->list[i].fields = 
      (XtArgVal)MALLOC(sizeof(XtArgVal ) * (numfields == 0 ? 1 : numfields));
   tmp = (char **)hp->list[i].fields;
   if (numfields == 0)
   {
      tmp[0] = NULL;
   }
   else
   {
      hp->list[i].fields = (XtArgVal)fields;
   }

   /* Tell the flat the items have changed */

   UpdateList(gizmo);

} /* end of InsertListItem */
/*
 * DeleteListItem
 *
 * Delete the item from the given flat list.
 * Also free space alloced for fields.
 */

extern void
DeleteListItem(ListGizmo * gizmo, int item)
{
   int        i;
   ListHead * hp;
   char **    tmp;

   if (gizmo                          != (ListGizmo *)0 &&
       gizmo->settings                != (Setting *)0 &&
       gizmo->settings->current_value != (ListHead *)0) {
      hp = (ListHead *)(gizmo->settings->current_value);
   }
   else {
      return;
   }

   if (hp->size == 0)
   {
      return;
   }

   /* First, free the fields within the list. */

   tmp = (char **)hp->list[item].fields;
   if (hp->numFields == 0)
   {
      if (tmp[0] != NULL)
      {
         FREE (tmp[0]);
      }
   }
   else {
      for (i=0; i<hp->numFields; i++) {
         if (tmp[i] != NULL) {
            FREE (tmp[i]);
         }
      }
   }
   FREE (hp->list[item].fields);

   /* Move all of the items up to cover the deleted item. */

   for (i=item; i<hp->size-1; i++) {
      hp->list[i].set = hp->list[i+1].set;
      hp->list[i].fields = hp->list[i+1].fields;
      hp->list[i].clientData = hp->list[i+1].clientData;
   }

   /* Reallocate the space for the list, free it completely */
   /* if there are no more items. */

   if (--hp->size == 0) {
      FREE (hp->list);
      hp->list = (ListItem *)NULL;
   }
   else {
      hp->list = (ListItem *) REALLOC (
         hp->list,
         sizeof (ListItem) * hp->size
      );
   }

   /* Tell the widget about the new items */

   UpdateList (gizmo);

} /* DeleteListItem */
