#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/exclusive.c	1.19"
#endif

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xm/Xm.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>

#include "misc.h"
#include "list.h"
#include "exclusive.h"

/*
 * Local data:
 */

/* Widget CreateCaption ( String, String, Widget ); */

/*
 * Local functions:
 */

static void		SelectCB(Widget, XtPointer, XtPointer);

/**
 ** CreateExclusive()
 **/

Widget
CreateExclusive (
	Widget			parent,
	Exclusive *		exclusive,
	Boolean			track_changes
	)
{
	list_ITERATOR		I;
	Widget			label;
	ExclusiveItem *		p;
	
	Widget row;
	char string[10];
	int i = 0;
	
	if (exclusive->caption)
	  {
	    label = (Widget)CreateCaption(
		exclusive->name, exclusive->string, parent
	    );
	    parent = XtParent(label);
	    row = XtVaCreateManagedWidget("row", xmRowColumnWidgetClass, parent,
	                                  XmNradioBehavior, True,
					  XmNorientation, XmHORIZONTAL,
					  XmNpacking,	XmPACK_TIGHT,
					  XmNmarginWidth, 20,
					  NULL);
	    AddToCaption(row, label);
	
	    exclusive->ChangeBarDB = (ChangeBar *) calloc(1, sizeof(ChangeBar));
	    CreateChangeBar(label, exclusive->ChangeBarDB);
	    
	    parent = row;
		
	  }
	else
	  {
	    XtVaSetValues(parent, 
	                  XmNradioBehavior, True,
			  XmNmarginWidth, 20,
			  XmNpacking,	XmPACK_TIGHT,
		          NULL);
	    
	    exclusive->ChangeBarDB = (ChangeBar *) calloc(1, sizeof(ChangeBar));
	    CreateChangeBar(parent, exclusive->ChangeBarDB);
	    label = NULL;
          }
	
	if(!exclusive->w)
	  {
	    exclusive->w = (Widget *) XtMalloc(exclusive->items->count * sizeof(Widget));
	  }
	else
	  {
	    exclusive->w = (Widget *) realloc((Widget *)exclusive->w, 
	    					(Cardinal) (exclusive->items->count * sizeof(Widget)));
	  }
	
	I = list_iterator(exclusive->items);
	while ((p = (ExclusiveItem *)list_next(&I))) 
	  {
		p->is_default = (exclusive->default_item == p);
		p->is_set     = (exclusive->current_item == p);
		sprintf(string, "button%d", i);
		exclusive->w[i] = XtVaCreateManagedWidget(
	                          string,
		                  xmToggleButtonWidgetClass, 
		                  parent,
		                  XmNlabelString, XmStringCreateLocalized((String) p->name),
				  XmNset, p->is_set,
		                  NULL);
		
		XtAddCallback(exclusive->w[i], XmNvalueChangedCallback, SelectCB, exclusive);
		i++;
	  }
	
	exclusive->track_changes = track_changes;

	return label;
} /* CreateExclusive */

/**
 ** SetExclusive()
 **/

void
SetExclusive (
	Exclusive *		exclusive,
	ExclusiveItem *		item,
	int			change_state
	)
{
	if (!item) 
	  {
		debug((stderr, "SetExclusive: name = NULL\n"));
		
		XtVaSetValues(exclusive->w[list_index(exclusive->items, exclusive->current_item)],
		              XmNset, FALSE,
			      NULL);

	  } 
	else if (exclusive->current_item != item) 
	  {
	  	list_ITERATOR I;
		ExclusiveItem * p;
		int i = 0;
		int pos = list_index(exclusive->items, item);
		
	    	I = list_iterator(exclusive->items);
	    	while ((p = (ExclusiveItem *)list_next(&I))) 
	    	  {
		  	if(i == pos)
			  {
			  	XtVaSetValues(exclusive->w[i],
		              		XmNset, True,
			      		NULL);
				p->is_set = True;
			  }
			else
			  {
			  	XtVaSetValues(exclusive->w[i],
		              		XmNset, False,
			      		NULL);
				p->is_set = False;
			  }
			
			i++;
	 	  }
		
		if (exclusive->track_changes)
		  {
			SetChangeBarState (exclusive->ChangeBarDB, 0, 
				change_state, True, exclusive->change);
	 	  }

	  } 
	else
	  {
		
		if (exclusive->track_changes && change_state == WSM_NONE)
		  {
			SetChangeBarState (exclusive->ChangeBarDB, 0,
				WSM_NONE, True,  exclusive->change);
	 	  }
          }
	
	exclusive->current_item = item;
	
	return;
} /* SetExclusive */

/**
 ** SelectCB()
 **/

static void
SelectCB (
	Widget		w,
	XtPointer	client_data,
	XtPointer	call_data
	)
{
	
	Exclusive *		exclusive = (Exclusive *)client_data;
	list_ITERATOR		I;
	ExclusiveItem * 	p;
	
	int index;
	int set = 0;
	int i = 0;
	
	set = ((XmToggleButtonCallbackStruct *)call_data)->set;
	
	if(set)
	  {
	    I = list_iterator(exclusive->items);
	    while ((p = (ExclusiveItem *)list_next(&I))) 
	      {
		    if(w == exclusive->w[i])  /* At appropriate spot in list */
		      {
		        exclusive->current_item = p;
			p->is_set = True;
			if (exclusive->track_changes)
		 	 {
				SetChangeBarState (exclusive->ChangeBarDB, 0,
					WSM_NORMAL, True, exclusive->change);
	 	 	 }
		      }
		    else
		      {
		        p->is_set = False;
		      }
		    i++;
	      }
	
	    if (exclusive->f)
	      {
		    (*exclusive->f) (exclusive);
	      }
	  }

	return;
} /* SelectCB */

int CheckExclusiveChangeBar(
	Exclusive *		exclusive)
{
	
	return(exclusive->ChangeBarDB->state);
}
