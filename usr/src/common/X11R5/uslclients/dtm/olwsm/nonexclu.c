#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/nonexclu.c	1.10"
#endif

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>

#include <misc.h>
#include <list.h>
#include "changebar.h"
#include "nonexclu.h"
#include <xtarg.h>

/*
 * Convenient macros:
 */

#define TouchItems(N, A)  \
  XtVaSetValues((N)->w[list_index((N)->items, (A))], XmNset, (A)->is_set, NULL);

/* #define TouchItems(N) XtVaSetValues((N)->w, XtNitemsTouched, (XtArgVal)True, (String)0) */

/*
 * Local data:
 */

static char		_shift [] = "Shift";
static char		_ctrl  [] = "Ctrl";
static char		_mod1  [] = "Alt";

/*
static String		fields[] = {
	XtNlabel,
	XtNuserData,
	XtNdefault,
	XtNset
};
*/

/* Widget CreateCaption ( String, String, Widget ); */

/*
 * Local routines:
 */

static void		NonexclusiveSelectCB(Widget, XtPointer, XtPointer);
static void		NonexclusiveUnselectCB(Widget, XtPointer, XmToggleButtonCallbackStruct * );

/**
 ** CreateNonexclusive()
 **/

void
CreateNonexclusive (
	Widget			parent,
	Nonexclusive *		nonexclusive,
	Boolean			track_changes
	)
{
	list_ITERATOR		I;

	NonexclusiveItem *	p;
	
	Widget label;
	Widget row;
	char string[10];
	int i = 0;
	
	if (nonexclusive->caption)
	  {
	    label = (Widget)CreateCaption(
		nonexclusive->name, nonexclusive->string, parent
	    );
	    parent = XtParent(label);
	    row = XtVaCreateManagedWidget("row", xmRowColumnWidgetClass, parent,
					  NULL);
	    AddToCaption(row, label);
	    parent = row;
	  }
	
	if(!nonexclusive->w)
	  {
	    nonexclusive->w = (Widget *) XtMalloc(nonexclusive->items->count * sizeof(Widget));
	  }
	else
	  {
	    nonexclusive->w = (Widget *) realloc((Widget *)nonexclusive->w, 
	    					(Cardinal) (nonexclusive->items->count * sizeof(Widget)));
	  }
	
	I = list_iterator(nonexclusive->items);
	while ((p = (NonexclusiveItem *)list_next(&I)))
	  {
		p->is_default = (nonexclusive->default_item == p);
		sprintf(string, "button%d", i);
		nonexclusive->w[i] = XtVaCreateManagedWidget(
	                          string,
		                  xmToggleButtonWidgetClass, 
		                  parent,
		                  XmNlabelString, XmStringCreateLocalized((String) p->name),
				  XmNset, p->is_set,  /* Default ? */
		                  NULL);
	        if((p->is_default) )
		  {
		    XtVaSetValues(parent, 
		                 XmNmenuHistory, nonexclusive->w[i],
				 NULL);
		  }
		
		XtAddCallback(nonexclusive->w[i], 
		              XmNvalueChangedCallback, NonexclusiveSelectCB, nonexclusive);
		i++;
          }
	
	/*
	nonexclusive->w = XtVaCreateManagedWidget(
		nonexclusive->name,
		flatButtonsWidgetClass,
		parent,
		XtNbuttonType,	  (XtArgVal)OL_RECT_BTN,
		XtNexclusives,	  (XtArgVal)False,
		XtNlayoutType,	  (XtArgVal)OL_FIXEDCOLS,
		XtNselectProc,    (XtArgVal)NonexclusiveSelectCB,
		XtNunselectProc,  (XtArgVal)NonexclusiveUnselectCB,
		XtNclientData,    (XtArgVal)nonexclusive,
		XtNitems,         (XtArgVal)nonexclusive->items->entry,
		XtNnumItems,      (XtArgVal)nonexclusive->items->count,
		XtNitemFields,    (XtArgVal)fields,
		XtNnumItemFields, (XtArgVal)XtNumber(fields),
		(String)0
	);
	*/

	nonexclusive->track_changes = track_changes;

	return;
} /* CreateNonexclusive */

/**
 ** UnsetAllNonexclusiveItems()
 **/

void
UnsetAllNonexclusiveItems ( 
        Nonexclusive * 		nonexclusive
	)
{
	list_ITERATOR		I;

	NonexclusiveItem *	ni;


	I = list_iterator(nonexclusive->items);
	while ((ni = (NonexclusiveItem *)list_next(&I)))
	  {
		ni->is_set = False;
	        TouchItems (nonexclusive, ni);
	  }
	

	return;
} /* UnsetAllNonexclusiveItems */

/**
 ** SetNonexclusiveItem()
 **/

void
SetNonexclusiveItem (
	Nonexclusive *		nonexclusive,
	NonexclusiveItem *		item
	)
{
	if (item) 
	  {
		item->is_set = True;
		TouchItems (nonexclusive, item);
	  }

	return;
} /* SetNonexclusiveItem */

/**
 ** SetSavedItems()
 **/

void
SetSavedItems (
	Nonexclusive *		nonexclusive
	)
{
	list_ITERATOR		I;

	NonexclusiveItem *	ni;


	nonexclusive->modifiers = 0;
	I = list_iterator(nonexclusive->items);
	while ((ni = (NonexclusiveItem *)list_next(&I)))
	  {
		if (ni->is_set) 
		  {
			if (MATCH((String)ni->addr, _shift))
			  {
				nonexclusive->modifiers |= ShiftMask;
			  }
			else if (MATCH((String)ni->addr, _ctrl))
			  {
				nonexclusive->modifiers |= ControlMask;
			  }
			else if (MATCH((String)ni->addr, _mod1))
			  {
				nonexclusive->modifiers |= Mod1Mask;
			  }
		  }
	  }

	return;
} /* SetSavedItems */

/**
 ** ReadSavedItems
 **/

void
ReadSavedItems (
	Nonexclusive *		nonexclusive
	)
{
	list_ITERATOR		I;

	NonexclusiveItem *	ni;


	/*
	 * For each modifier that is set,
	 * traverse items list and set that item
	 */
	if (nonexclusive->modifiers & ShiftMask) 
	  {
		I = list_iterator(nonexclusive->items);
		while ((ni = (NonexclusiveItem *)list_next(&I)))
		  {
			if (MATCH((String)ni->addr, _shift)) 
			  {
				ni->is_set = True;
				TouchItems (nonexclusive, ni);
				break;
			  }
		  }
	}
	if (nonexclusive->modifiers & ControlMask) 
	  {
		I = list_iterator(nonexclusive->items);
		while ((ni = (NonexclusiveItem *)list_next(&I)))
		  {
			if (MATCH((String)ni->addr, _ctrl)) 
			  {
				ni->is_set = True;
				TouchItems (nonexclusive, ni);
				break;
			  }
		  }
	  }
	if (nonexclusive->modifiers & Mod1Mask) 
	  {
		I = list_iterator(nonexclusive->items);
		while ((ni = (NonexclusiveItem *)list_next(&I)))
		  {
			if (MATCH((String)ni->addr, _mod1)) 
			  {
				ni->is_set = True;
				TouchItems (nonexclusive, ni);
				break;
			  }
		  }
	  }

	return;
} /* ReadSavedItems */

/**
 ** NonexclusiveSelectCB()
 **/

static void
NonexclusiveSelectCB (
	Widget		w,
	XtPointer	client_data,
	XtPointer	call_data
	)
{
	Nonexclusive *		nonexclusive = (Nonexclusive *)client_data;
	list_ITERATOR		I;
	NonexclusiveItem *	ni;
	int	set = 0;
	int i = 0;
	
	set = ((XmToggleButtonCallbackStruct *)call_data)->set;

	I = list_iterator(nonexclusive->items);
	while ((ni = (NonexclusiveItem *)list_next(&I)))
	  {
		if(w == nonexclusive->w[i])
		  {
		    if(set)
		      {
		        ni->is_set = True;
		      }
		    else
		      {
		        ni->is_set = False;
		      }
		  }
		i++;
	  }
	
	
	
	if (nonexclusive->track_changes)
	  {
		/* _OlSetChangeBarState (nonexclusive->w, OL_NORMAL, OL_PROPAGATE); */
	  }
	
	if (nonexclusive->f)
	  {
		(*nonexclusive->f) (nonexclusive);
	  }

	return;
} /* NonexclusiveSelectCB */

/**
 ** NonexclusiveUnselectCB()
 **/

static void
NonexclusiveUnselectCB (
	Widget					w,
	XtPointer				client_data,
	XmToggleButtonCallbackStruct *		call_data
	)
{
	Nonexclusive *		nonexclusive = (Nonexclusive *)client_data;


	if (nonexclusive->track_changes)
	  {
		/* _OlSetChangeBarState (nonexclusive->w, OL_NORMAL, OL_PROPAGATE); */
	  }
	if (nonexclusive->f)
	  {
		(*nonexclusive->f) (nonexclusive);
	  }

	return;
} /* NonexclusiveUnselectCB */
