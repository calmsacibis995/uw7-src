#ident	"@(#)MultiPList.C	1.3"
#include <iostream.h>		//  for cout()
#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/ScrollBar.h>
#include "MultiPList.h"
#include "PList.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

//
// Name: MultiPList (this is a new contructor which supports dbl-click)
//
// Purpose: Class constructor.  Create the Motif based MultiPList
//         component.  
//
// Description:   Create a multilist component from the following widgets.
//                     form widget field 
//                        |--- scroll bar field 
//                        |---numOfLists scrolling PLists
//
//       Setup a destroy callback and allocate space for the vector
//       of virtual lists.
//       Initialize the constants
//
// Returns: Nothing
//
// Side Effects: One initialized object
// 
MultiPList::MultiPList ( Widget parent, int numOfLists, int visibleItemCount,
			ItemSelCallback callback, XtPointer perObjClientData,
			int spacing, int rowcount, int numcol):
                             initialVirtualLists(32), 
                             EXTRA_SPACE(16), 
                             initialArraySize(256)
{
	int i;

	parentWidget = parent;
    clientCallback = callback;
    clientData = perObjClientData;
	currentLoadVList = 0;
	listStart = 0;
    amountOfLists = numOfLists;
    widgetsDestroyed = False;
    clientOpenCallback = NULL;
    clientUnSelectCallback = NULL;
    clientClearCallback = NULL;

	//
    // Create the listing panels
    // form widget field 
    //   |--- scroll bar field 
    //   |--- numOfLists forms containing PLists
	//
    
    form = XtVaCreateManagedWidget("form", 
					xmFormWidgetClass, 
					parent,
					XmNresizable, True,  
					XmNresizePolicy, XmRESIZE_NONE,
					XmNrightAttachment,XmATTACH_FORM,
					XmNleftAttachment,XmATTACH_FORM,
					XmNtopAttachment,XmATTACH_FORM,
					XmNbottomAttachment,XmATTACH_FORM,
					NULL);

    scrollbar = XtVaCreateManagedWidget("scrollbar",
					xmScrollBarWidgetClass,
					form,
					XmNorientation, XmHORIZONTAL,
					XmNleftAttachment,XmATTACH_FORM,
					XmNresizePolicy, XmRESIZE_NONE,  
					XmNresizable, False, 
					XmNrightAttachment,XmATTACH_FORM,
					XmNsliderSize,100,
					XmNmaximum,100,
					XmNminimum,0,
					XmNvalue,0,
					NULL );

    XtAddCallback ( scrollbar,
           XmNvalueChangedCallback,
           &MultiPList::ScrollBarCallback,
           (XtPointer) this );

	//
	// OK, now the list panels
	//
	int pos = 100/numOfLists;
    int position = 0;
	for ( i = 0; i < numOfLists; i++ )
	{
    	plistForm[i] = XtVaCreateWidget("form", 
					xmFormWidgetClass, 
					form,
					XmNresizable, True,  
					XmNresizePolicy, XmRESIZE_NONE,
					XmNtopAttachment,XmATTACH_WIDGET,
					XmNtopWidget, scrollbar,
					XmNbottomAttachment,XmATTACH_FORM,
					XmNleftAttachment,XmATTACH_POSITION,
					XmNleftPosition,position,
					XmNrightAttachment,XmATTACH_POSITION,
					XmNrightPosition,position + pos,
					NULL);
		plist[i] = new PList(plistForm[i],"MultiPList", spacing, rowcount, 
							 numcol, 16, NULL, visibleItemCount);
 		plist[i]->UW_RegisterSingleCallback( 
							&MultiPList::ListCallback, (XtPointer)this);
 		plist[i]->UW_RegisterDblCallback( 
							&MultiPList::ListDblCallback, (XtPointer)this);
		XtManageChild(plist[i]->UW_GetListWidget());
		XtManageChild(plistForm[i]);
		position += pos;
	}
	//
	// Allocate and initialize VList storage
	//
	numOfVirtualLists = initialVirtualLists;
	vlp = (vir *)XtMalloc(sizeof (vir) * numOfVirtualLists);
	for ( i = 0; i < numOfVirtualLists; i++)
	{
		vlp[i].selectedPos = INVALID;
 		vlp[i].listLoadedIn = INVALID;
 		vlp[i].topItem = 1;
 		vlp[i].arraySize = initialArraySize;
		vlp[i].count = 0;
		vlp[i].clientData = NULL;
		vlp[i].clientItemData = (XtPointer *)
								XtMalloc(sizeof (XtPointer) * vlp[i].arraySize);
		vlp[i].pixmapRelate = (int *)
								XtMalloc(sizeof (int) * vlp[i].arraySize);
		vlp[i].listItems = (XmString *)
								XtMalloc(sizeof (XmString) * vlp[i].arraySize);
	}
	//
	// Destroy callback in case Xt nukes us.
	//
	XtAddCallback (form, XmNdestroyCallback,
                &MultiPList::DestroyCallback,
				(XtPointer) this );
	XtManageChild(form);
}


//
// Name: MultiPList
//
// Purpose: Class constructor.  Create the Motif based MultiPList
//         component.  
//
// Description:   Create a multilist component from the following widgets.
//                     form widget field 
//                        |--- scroll bar field 
//                        |---numOfLists scrolling PLists
//
//       Setup a destroy callback and allocate space for the vector
//       of virtual lists.
//       Initialize the constants
//
// Returns: Nothing
//
// Side Effects: One initialized object
// 
MultiPList::MultiPList ( Widget parent, int numOfLists,
						 ItemSelCallback callback, XtPointer perObjClientData,
						 int spacing, int rowcount, int numcol,
                         int visibleItemCount) :
                             initialVirtualLists(32), 
                             EXTRA_SPACE(16), 
                             initialArraySize(256)
{
	int i;

	parentWidget = parent;
    clientCallback = callback;
    clientData = perObjClientData;
	currentLoadVList = 0;
	listStart = 0;
    amountOfLists = numOfLists;
    widgetsDestroyed = False;
    clientOpenCallback = NULL;
    clientUnSelectCallback = NULL;
    clientClearCallback = NULL;

	//
    // Create the listing panels
    // form widget field 
    //   |--- scroll bar field 
    //   |--- numOfLists forms containing PLists
	//
    
    form = XtVaCreateManagedWidget("form", 
					xmFormWidgetClass, 
					parent,
					XmNresizable, True,  
					XmNresizePolicy, XmRESIZE_NONE,
					XmNrightAttachment,XmATTACH_FORM,
					XmNleftAttachment,XmATTACH_FORM,
					XmNtopAttachment,XmATTACH_FORM,
					XmNbottomAttachment,XmATTACH_FORM,
					NULL);

    scrollbar = XtVaCreateManagedWidget("scrollbar",
					xmScrollBarWidgetClass,
					form,
					XmNorientation, XmHORIZONTAL,
					XmNleftAttachment,XmATTACH_FORM,
					XmNresizePolicy, XmRESIZE_NONE,  
					XmNresizable, False, 
					XmNrightAttachment,XmATTACH_FORM,
					XmNsliderSize,100,
					XmNmaximum,100,
					XmNminimum,0,
					XmNvalue,0,
					NULL );

    XtAddCallback ( scrollbar,
           XmNvalueChangedCallback,
           &MultiPList::ScrollBarCallback,
           (XtPointer) this );

	//
	// OK, now the list panels
	//
	int pos = 100/numOfLists;
    int position = 0;
	for ( i = 0; i < numOfLists; i++ )
	{
    	plistForm[i] = XtVaCreateWidget("form", 
					xmFormWidgetClass, 
					form,
					XmNresizable, True,  
					XmNresizePolicy, XmRESIZE_NONE,
					XmNtopAttachment,XmATTACH_WIDGET,
					XmNtopWidget, scrollbar,
					XmNbottomAttachment,XmATTACH_FORM,
					XmNleftAttachment,XmATTACH_POSITION,
					XmNleftPosition,position,
					XmNrightAttachment,XmATTACH_POSITION,
					XmNrightPosition,position + pos,
					NULL);
		plist[i] = new PList(plistForm[i],"MultiPList", spacing, rowcount, 
							 numcol, 16, NULL, visibleItemCount);
 		plist[i]->UW_RegisterSingleCallback( 
							&MultiPList::ListCallback, (XtPointer)this);
		XtManageChild(plist[i]->UW_GetListWidget());
		XtManageChild(plistForm[i]);
		position += pos;
	}
	//
	// Allocate and initialize VList storage
	//
	numOfVirtualLists = initialVirtualLists;
	vlp = (vir *)XtMalloc(sizeof (vir) * numOfVirtualLists);
	for ( i = 0; i < numOfVirtualLists; i++)
	{
		vlp[i].selectedPos = INVALID;
 		vlp[i].listLoadedIn = INVALID;
 		vlp[i].topItem = 1;
 		vlp[i].arraySize = initialArraySize;
		vlp[i].count = 0;
		vlp[i].clientData = NULL;
		vlp[i].clientItemData = (XtPointer *)
								XtMalloc(sizeof (XtPointer) * vlp[i].arraySize);
		vlp[i].pixmapRelate = (int *)
								XtMalloc(sizeof (int) * vlp[i].arraySize);
		vlp[i].listItems = (XmString *)
								XtMalloc(sizeof (XmString) * vlp[i].arraySize);
	}
	//
	// Destroy callback in case Xt nukes us.
	//
	XtAddCallback (form, XmNdestroyCallback,
                &MultiPList::DestroyCallback,
				(XtPointer) this );
	XtManageChild(form);
}

//
// Name: ~MultiPList
//
// Purpose: Class destructor
//
// Description: Check to make sure that Xt hasn't already
//              destroyed the widgets, if not destroy them.
//              Free all allocated space by calling destroy.
//
// Returns:
//
// Side Effects: All space freed and widgets destroyed
// 
//
// Being deleted, Clean up resources
//
MultiPList::~MultiPList()
{
	if ( widgetsDestroyed != True )
	{
		//
		// Beat Xt here, remove the callback and
		// start the Widget destory process
		//
		widgetsDestroyed = True;
		XtRemoveCallback(form,XmNdestroyCallback,
				&MultiPList::DestroyCallback, (XtPointer) this );
		XtDestroyWidget(form);
		Destroy();
	}
}

//
//
//
// Name:  DestroyCallback
//
// Purpose: Catch the Xt destruction of our widget structure.
//
// Description:  Just get the passed this pointer and call the
//               C++ member function to destory the correct
//               object. Widget has been destroyed out from 
//               under us.
//
// Returns: Nothing
//
// Side Effects:
// 
void 
MultiPList::DestroyCallback ( Widget ,
                      XtPointer clientData,
                      XtPointer )
{
    MultiPList * obj = (MultiPList *) clientData;
   	obj->Destroy ( );
}

//
// Name:  Destroy
//
// Purpose: Clean up space, and protext against Xt 2 phase widget
//          destruction.
//
// Description:  Widget has been destroyed either by us or Xt.
//               Set the widgetDestroyed flag so that we won't access
//               any widget function just in case destruction was not
//               by Xt. Free all allocated space.
//
// Returns: Nothing
//
// Side Effects: All space freed
// 
void 
MultiPList::Destroy ( )
{
	int i;

	widgetsDestroyed = True;
	//
	// Clear and free all resources from all
	//  virtual lists from first to last
	//
	for( i = 0; i < numOfVirtualLists; i++ )
	{
		for(int x = 0; x < vlp[i].count; x++)
		{
			XmStringFree(vlp[i].listItems[x]);
		}
		XtFree((char *)vlp[i].listItems);
		XtFree((char *)vlp[i].pixmapRelate);
		XtFree((char *)vlp[i].clientItemData);
	}
	XtFree((char *)vlp);
	//
	// Free the PLists
	//
	for (i = 0; i < amountOfLists; i++)
		delete plist[i];
}
//
// Name:  SetListClientData
//
// Purpose: Load the per list clientData
//
// Description: 
//
// Returns: Nothing
//
// Side Effects:
// 
void
MultiPList::SetListClientData(XtPointer clientData)
{
    vlp[currentLoadVList].clientData = clientData;
}

//
// Name:  SetOpenCallback
//
// Purpose: Save the client function to call when a VList item has been opened
//
// Description: Save the clients callback function pointer
//
// Returns: Nothing
//
// Side Effects:
// 
void
MultiPList::SetOpenCallback (ItemOpenCallback callback)
{
    clientOpenCallback = callback;
}

//
// Name:  SetClearCallback
//
// Purpose: Save the client function to call when a VList has been cleared 
//
// Description: Save the clients callback function pointer
//
// Returns: Nothing
//
// Side Effects:
// 
void
MultiPList::SetClearCallback (ClearCallback callback)
{
    clientClearCallback = callback;
}
//
// Name:  SetUnSelectCallback
//
// Purpose: Inform client that an item has been unselected 
//          in a particular list
//
// Description: Save the clients callback function pointer
//
// Returns: Nothing
//
// Side Effects:
// 
void
MultiPList::SetUnSelectCallback (UnSelectCallback callback)
{
    clientUnSelectCallback = callback;
}
//
// Name:  SetClearItemCallback
//
// Purpose: Inform client that a VList item has been cleared 
//
// Description: Save the clients callback function pointer
//
// Returns: Nothing
//
// Side Effects:
// 
void
MultiPList::SetClearItemCallback (ClearCallback callback)
{
    clientItemClearCallback = callback;
}

//
// Name:  ClearLists
//
// Purpose: Clears all virtual lists from point "pos" to end of 
//          virtual lists. 
//
// Description: Deletes the visible lists contents if it is in
//              the delete inclution set. Frees all resources
//              used by the virtual lists in the deletion set.
//              Finally will adjust the visible lists if deletion
//              point is to the left of the first visible list, 
//              listStart.
//
// Returns: Nothing
//
// Side Effects: Freed lists.
// 
void 
MultiPList::ClearLists(int pos)
{
	register int i;
	register int x;
	register vir *vp;

	//
	// Leave if widgets have been destroyed by Xt
	//
	if ( widgetsDestroyed == True )
		return;
	//
	// Clear and free all resources from all VLists from end to pos
	//
	for (i = currentLoadVList; i >= pos; --i)
	{
		vp = &vlp[i];
		vp->selectedPos = INVALID;

		//
		// If this VList is currently visible then delete it from view
		//
		if ( vp->listLoadedIn != INVALID )
		{
			plist[vp->listLoadedIn]->UW_ListDeleteAllItems();
			vp->listLoadedIn = INVALID;
		}
		vp->topItem = 1;
		for (x = 0; x < vp->count; x++)
		{
			XmStringFree(vp->listItems[x]);
    		if (clientItemClearCallback)
	    		clientItemClearCallback(this, i, vp->clientItemData[x]);
		}
		vp->count = 0;

		//
		// Tell client this VList is being purged
		//
    	if (clientClearCallback)
	    	clientClearCallback(this, i, vp->clientData);
	}
	//
	// We have cleared from pos to the end inclusive, so set 
	// the currentLoadVList to the first empty VList.
	//
	currentLoadVList = pos;

	if (currentLoadVList <= listStart)
	{
		//
		// Pos was to the left of the viewport so we must 
		// bring currentLoadVList - 1 into view
		// Always try to keep at least one loaded list in view
		//
		listStart = currentLoadVList - 1;
		if ((listStart < 0) || (currentLoadVList <= amountOfLists))
			listStart = 0;
		DisplayAt(listStart,False);
	}
	PositionScrollBar();
}	//  End  ClearLists()

//
// Name:  DisplayLists
//
// Purpose: Fill the physical lists with the correct VList
//
// Description:  Update the physical lists from the virtual lists 
//               so that our view of the lists shows the last X
//               lists, where X = number of physical lists.
//
// Returns: Nothing
//
// Side Effects: None known
// 
void 
MultiPList::DisplayLists()
{
	register int i;

	//
	// Adjust the start display list number if
	//   there are full lists to the right of the view port
	//
	if (listStart + amountOfLists < currentLoadVList)
	{
		//
		// Invalidate listLoadedIn because listStart
		// is changing therefore different lists will 
		// be loaded into the PLists
		//
		for (i = 0; i < amountOfLists; i++)
		{
			vlp[listStart + i].listLoadedIn = INVALID;
		}
		listStart = currentLoadVList - amountOfLists;
	}
	//
	// Diplay the lists
	//
	for (i = 0; i < currentLoadVList-listStart; i++)
	{
		LoadList(i, &vlp[i + listStart]);
	}
	PositionScrollBar();
}

//
// Name: DisplayAt
//
// Purpose: Protected member function to respond to the horizontal
//              scrollbar movement and VList deletetions.
//            
// Description: Moves the viewport into the virtual lists. Fairly
//              simple, deletes all visable lists, uses arg value
//              to reposition and displays new lists. If the savePos
//              flag is TRUE then we need to query the physical lists
//              and save the top position and selected item position
//              so we can restore them when the list is brought back
//              into view.
//
// Returns: Nothing
//
// Side Effects: Viewport into lists updated.
// 
void 
MultiPList::DisplayAt(int value, int savePos)
{
	int top;
	register int i;

	if ( currentLoadVList == 0 )
		return;
	//
	// Save list info if necessary 
	//  ( only valid when scrolling horizontally )
	//
	if (savePos == True )
	{
		for (i = 0; i < amountOfLists; i++, listStart++)
		{
			//
			// Get the top item from each list and store for later
			// Invalidate all old lists positions
			//
			XtVaGetValues(plist[i]->UW_GetListWidget(),
	        		      	XmNtopItemPosition,&top,
	              			NULL);
        	vlp[listStart].topItem  = top;
        	vlp[listStart].listLoadedIn  = INVALID;
		}
	}
	//
	// Load the lists up starting with left most PList
	//
	listStart = value;
	for (i = 0; i < amountOfLists; i++)
	{
		LoadList(i, &vlp[listStart + i]);
	}
}

//
// Name: LoadList
//
// Purpose: Load the physical lists.
//
// Description: If the physical list does not already contain 
//              this VList and the VList has items then load it.
//
// Returns: Nothing
//
// Side Effects: Updates the physical display.
// 
void 
MultiPList::LoadList(int listIndex,vir  *vp)
{
	Widget listWidget;

	if (vp->listLoadedIn != listIndex)
	{
		//
		// Physical list does not already contain this
		// VList, delete it's current contents
		//
		vp->listLoadedIn =  INVALID;
		listWidget = plist[listIndex]->UW_GetListWidget();
		XtSetMappedWhenManaged(listWidget, False);
		plist[listIndex]->UW_ListDeleteAllItems();
		if (vp->count)
		{
			plist[listIndex]->UW_ListAddItems(vp->listItems, vp->count, 0,
												vp->pixmapRelate);
			if (vp->selectedPos != INVALID);
				plist[listIndex]->UW_SetSelectedItem(vp->selectedPos);
			plist[listIndex]->UW_SetTopItem(vp->topItem);
			vp->listLoadedIn = listIndex;
		}
		XtSetMappedWhenManaged(listWidget, True);
	}
}

//
// Name:  SelectItem
//
// Purpose: Set the seleted item number in a virtual list to the
//          argumented value.
//
// Description: Changes the selectedPos value for the indexed
//              virtual list.  THIS WILL NOT change the current
//              physical display.
//
// Returns: Nothing
//
// Side Effects: selectedPos changed
// 
void 
MultiPList::SelectItem(int i, int position)
{
	assert(i >= 0);
	assert(i <= currentLoadVList);
	vlp[i].selectedPos = position + 1;  // List starts at 1
}
//
// Name:  SelectItem
//
// Purpose: Set the seleted item in a virtual list to the
//          match the XmString in the argumented XmString.
//
// Description: Changes the selectedPos value for the indexed
//              virtual list.  THIS WILL NOT change the current
//              physical display.
//
// Returns: Nothing
//
// Side Effects: selectedPos changed
// 
void 
MultiPList::SelectItem(int i, XmString xmstr)
{
	int x;

	assert(i >= 0);
	assert(i <= currentLoadVList);

	for (x = 0; x < vlp[i].count ; x++)
	{
		if (XmStringCompare(vlp[i].listItems[x], xmstr) == True)
		{
			vlp[i].selectedPos = x + 1;  // List starts at 1
			break;
		}
	}
}

//
// Name:  SelectTop
//
// Purpose: Set the top item number in a virtual list to the
//          argumented value.
//
// Description: Changes the top value for the indexed virutal list.
//              THIS WILL NOT change the current physical display.
//
// Returns: Nothing
//
// Side Effects: top changed
// 
void 
MultiPList::SelectTop(int i, int position)
{
	assert(i >= 0 );
	assert( i < currentLoadVList );
	vlp[i].topItem = position + 1;
}

//
// Name: GetTop
//
// Purpose: To retrieve the number of the topmost visible item
//          in a virtual list.
//            
// Description:
//
// Returns: topItem
//
// Side Effects:
// 
int 
MultiPList::GetTop(int i)
{
	assert(i >= 0);
	assert(i < currentLoadVList);
	return (vlp[i].topItem - 1);
}

//
// Name:  GetObjClientData
//
// Purpose: To retrieve the clientData specified in the constructor
//
// Description:  
//
// Returns: clientData
//
// Side Effects: None
// 
XtPointer 
MultiPList::GetObjClientData()
{
	return (clientData);
}
//
// Name:  GetListCount
//
// Purpose: To retrieve the number of items in a virtual list
//
// Description:  
//
// Returns: Number of items on the virtual list
//
// Side Effects: None
// 
int 
MultiPList::GetListCount(int i)
{
	assert(i >= 0);
	assert(i < currentLoadVList);
	return (vlp[i].count);
}

//
// Name: GetVisibleCount
//
// Purpose: To retrieve the number of visible items in the physical
//          list.  
//
// Description: Just gets the widgets XmNvisibleItemCount resource.
//
// Returns: Number of visible list items
//
// Side Effects: None
//
int 
MultiPList::GetVisibleCount()
{
	int i;
	XtVaGetValues(plist[0]->UW_GetListWidget(),
				XmNvisibleItemCount, &i,
				NULL);
	return (i);
}

//
// Name:  NextList
//
// Purpose: To set the next virtual list for loading
//
// Description: Initiallizes a virtual list vector element.  Also
//              tests for possible vector overflow and allocates
//              more space if necessary.
//
// Returns: Nothing
//
// Side Effects: vector of virtual lists might grow.  If there is no
//               more space available XtRealloc will TERMINATE.
// 
void 
MultiPList::NextList()
{
	int i;

	//
	// Go to the next VList
	//
	currentLoadVList++;

	//
	// Get some more VList space if we have grown to big
	//
	if (currentLoadVList >= numOfVirtualLists)
	{
		vlp = (vir *)XtRealloc((char *)vlp, sizeof(vir) *
									(numOfVirtualLists + initialVirtualLists));
		for (i = numOfVirtualLists; 
					i < numOfVirtualLists + initialVirtualLists; i++)
		{
 			vlp[i].listLoadedIn = INVALID;
			vlp[i].selectedPos = INVALID;
 			vlp[i].topItem = 1;
 			vlp[i].arraySize = initialArraySize;
			vlp[i].clientData = NULL;
			vlp[i].clientItemData = (XtPointer *)
								XtMalloc(sizeof (XtPointer) * vlp[i].arraySize);
			vlp[i].pixmapRelate = (int *)
								XtMalloc(sizeof (int) * vlp[i].arraySize);
			vlp[i].listItems = (XmString *)
								XtMalloc(sizeof (XmString) * vlp[i].arraySize);
			vlp[i].count = 0;
		}
		numOfVirtualLists += initialVirtualLists;
	}
}
 
//
// Name: ScrollBarCallback 
//
// Purpose: Convert from Xt C function to C++ member function
//
// Description: clientData is the obejct "this" pointer, which is
//              used to call the correct instance C++ member function.
//
// Returns: Nothing
//
// Side Effects: None
// 
void 
MultiPList::ScrollBarCallback ( Widget  ,
                      XtPointer clientData,
                      XtPointer callData )
{
	XmScrollBarCallbackStruct *cb = (XmScrollBarCallbackStruct *)callData;
    MultiPList * obj = (MultiPList *) clientData;
	obj->DisplayAt(cb->value,True);
}

//
// Name: ListDblCallback
//
// Purpose: To pass the selected information along to ItemOpened
//          C++ member function.
//            
// Description: Static member function, no this, to convert from
//              Xt space to C++.  Get the ascii string, and 
//              VList number to string came from and pass to C++
//              member function.
//
// Returns: Nothing
//
// Side Effects: Updates the selectedPos and topItem fields in the
//               vector of virtual lists for the selected list.
// 
void 
MultiPList::ListDblCallback ( Widget    w,
                      XtPointer clientData,
                      XtPointer callData )
{
    char *item;
	int status;
	int top;
	int list_num = 0;

    MultiPList * obj = (MultiPList *)clientData;
    XmListCallbackStruct * cd = (XmListCallbackStruct *)callData;

	//
	// Get the VList number
	//
	int i;
	for (i = 0; i < obj->amountOfLists; i++)
	{
		if (w == obj->plist[i]->UW_GetListWidget())
		{
			list_num = i+obj->listStart;
			break;
		}
	}

	//
	// Set to unselected the current VList selectedPOs incase this 
	// was a pick on an already selected item
	//
	obj->vlp[list_num].selectedPos = INVALID;

	//
    // Make sure an item was selected
	//
    if (cd->selected_item_count && cd->item)   
    {
    	// Extract the first character string matching the default
    	// character set from the compound string

    	status = XmStringGetLtoR(cd->item,
                  	XmSTRING_DEFAULT_CHARSET,
                  	&item);
    	// If a string was succesfully extracted, call
    	if (status)
		{
			//
			// Save away the top items position, and the postition
			// of the selected item
			//
			XtVaGetValues(obj->plist[i]->UW_GetListWidget(),
							XmNtopItemPosition,&top, NULL);
        	obj->vlp[list_num].topItem  = top;
			obj->vlp[list_num].selectedPos = cd->item_position;
			//
			// Go to C++ member function
			//
        	obj->ItemOpened(item, list_num );
		}
    }
	else
	{
   		if (obj->clientUnSelectCallback)
    		obj->clientUnSelectCallback(obj, list_num);
	}

}	//  End  ListDblCallback()


//
// Name: ListCallback
//
// Purpose: To pass the selected information along to ItemSelected
//          C++ member function.
//            
// Description: Static member function, no this, to convert from
//              Xt space to C++.  Get the ascii string, and 
//              VList number to string came from and pass to C++
//              member function.
//
// Returns: Nothing
//
// Side Effects: Updates the selectedPos and topItem fields in the
//               vector of virtual lists for the selected list.
// 
void 
MultiPList::ListCallback ( Widget    w,
                      XtPointer clientData,
                      XtPointer callData )
{
    char *item;
	int status;
	int top;
	int list_num = 0;

    MultiPList * obj = (MultiPList *)clientData;
    XmListCallbackStruct * cd = (XmListCallbackStruct *)callData;

	//
	// Get the VList number
	//
	int i;
	for (i = 0; i < obj->amountOfLists; i++)
	{
		if (w == obj->plist[i]->UW_GetListWidget())
		{
			list_num = i+obj->listStart;
			break;
		}
	}

	//
	// Set to unselected the current VList selectedPOs incase this 
	// was a pick on an already selected item
	//
	obj->vlp[list_num].selectedPos = INVALID;

	//
    // Make sure a item was selected
	//
    if (cd->selected_item_count && cd->item)   
    {
    	// Extract the first character string matching the default
    	// character set from the compound string

    	status = XmStringGetLtoR(cd->item,
                  	XmSTRING_DEFAULT_CHARSET,
                  	&item);
    	// If a string was succesfully extracted, call
    	if (status)
		{
			//
			// Save away the top items position, and the postition
			// of the selected item
			//
			XtVaGetValues(obj->plist[i]->UW_GetListWidget(),
							XmNtopItemPosition,&top, NULL);
        	obj->vlp[list_num].topItem  = top;
			obj->vlp[list_num].selectedPos = cd->item_position;
			//
			// Go to C++ member function
			//
        	obj->ItemSelected(item, list_num );
		}
    }
	else
	{
   		if (obj->clientUnSelectCallback)
    		obj->clientUnSelectCallback(obj, list_num);
	}
}	//  End  ListCallback()

//
// Name: ItemOpened 
//
// Purpose: To pass to the function pointed to by callback
//          the item that the user has "opened" (double-clicked)..
//
// Description: If the clientOpenCallback function pointer has been set
//              then call it with the data the register specified,
//              the item "opened" in character form and the virtual
//              list number the item came from.
//
// Returns: Nothing
//
// Side Effects: None
// 
void 
MultiPList::ItemOpened ( char *item,int list_num )
{
	int pos = vlp[list_num].selectedPos - 1;

    if (clientOpenCallback)
	    clientOpenCallback((XtPointer)this, item, list_num,
                       (XtPointer)vlp[list_num].clientItemData[pos]);
}


//
// Name: ItemSelected 
//
// Purpose: To pass to the function pointed to by callback
//          the item that the user has selected.
//
// Description: If the clientCallback function pointer has been set
//              then call it with the data the register specified,
//              the item selected in character form and the virtual
//              list number the item came from.
//
// Returns: Nothing
//
// Side Effects: None
// 
void 
MultiPList::ItemSelected ( char *item,int list_num )
{
	int pos = vlp[list_num].selectedPos - 1;

    if (clientCallback)
	    clientCallback((XtPointer)this, item, list_num,
                       (XtPointer)vlp[list_num].clientItemData[pos]);
}
//
// Name: GetListWidget 
//
// Purpose: To retrieve the list widget 
//
// Description: 
//
// Returns: List Widget 
//
// Side Effects: Out of bounds = assertion failure, core dump :-)
// 
Widget 
MultiPList::GetListWidget (int listNum)
{
	assert(listNum >= 0);
	assert(listNum < amountOfLists);
    return (plist[listNum]->UW_GetListWidget());
}

//
// Name: AddListItem 
//
// Purpose: To add an item into the last virtual list. This will not
//          cause a visual change to the physical lists.
//
// Description:
//
// Returns: 
//
// Side Effects: None
// 
void 
MultiPList::AddListItem(XmString xmstr, int pixmapIndex, 
                        XtPointer itemClientData)
{
	register int i;
	register vir *vp = &vlp[currentLoadVList];  // virtual list pointer

	if (vp->count >= vp->arraySize)
	{
		vp->arraySize += EXTRA_SPACE;
		vp->clientItemData = (XtPointer *)XtRealloc((char *)
					vp->clientItemData, sizeof (XtPointer) * vp->arraySize);
		vp->pixmapRelate = (int *)XtRealloc((char *)
					vp->pixmapRelate, sizeof (int) * vp->arraySize);
		vp->listItems = (XmString *)XtRealloc((char *)
					vp->listItems, sizeof (XmString) * vp->arraySize);

		for (i = vp->count; i < vp->arraySize; i++)
		{
 			vp->listItems[i] = NULL;
 			vp->pixmapRelate[i] = 0;
 			vp->clientItemData[i] = 0;
		}
	}
	vp->listItems[vp->count] = xmstr;
	vp->pixmapRelate[vp->count] = pixmapIndex;
	vp->clientItemData[vp->count] = itemClientData;
	vp->count++;
}
//
// Name: AddListItems
//
// Purpose: To add items into the last virtual list. This will not
//          cause a visual change to the physical lists.
//
// Description:
//
// Returns: 
//
// Side Effects: None
// 
void 
MultiPList::AddListItems(XmString *xmstr, int *pixmapIndex, 
                         XtPointer * itemClientData, int count)
{
	int i,x;

	if (vlp[currentLoadVList].count + count >= vlp[currentLoadVList].arraySize)
	{
		vlp[currentLoadVList].clientItemData = (XtPointer *)XtRealloc((char *)
					vlp[currentLoadVList].clientItemData, 
					sizeof (XtPointer) * vlp[currentLoadVList].arraySize * 2);
		vlp[currentLoadVList].pixmapRelate = (int *)XtRealloc((char *)
					vlp[currentLoadVList].pixmapRelate, 
					sizeof (int) * vlp[currentLoadVList].arraySize * 2);
		vlp[currentLoadVList].listItems = (XmString *)XtRealloc((char *)
					vlp[currentLoadVList].listItems, 
					sizeof (XmString) *  vlp[currentLoadVList].arraySize * 2);
		vlp[currentLoadVList].arraySize = vlp[currentLoadVList].arraySize * 2;
		for (i = vlp[currentLoadVList].count; i < vlp[currentLoadVList].arraySize; i++)
		{
 			vlp[currentLoadVList].listItems[i] = NULL;
 			vlp[currentLoadVList].pixmapRelate[i] = 0;
 			vlp[currentLoadVList].clientItemData[i] = 0;
		}
	}
	for (x = vlp[currentLoadVList].count, i = 0; i < count; i++,x++ )
	{
		vlp[currentLoadVList].listItems[x] = *xmstr++;
		vlp[currentLoadVList].pixmapRelate[x] = *pixmapIndex++;
		vlp[currentLoadVList].clientItemData[x] = *itemClientData++;
	}
	vlp[currentLoadVList].count += count;
}

//
// Name: PositionScrollBar
//
// Purpose:
//
// Description:
//
// Returns: Pixmap index number
//
// Side Effects:
//
void
MultiPList::PositionScrollBar()
{
	if ( currentLoadVList < amountOfLists )
	{
		//
		// Full size scrollbar
		// 
		XtVaSetValues(scrollbar,
				  XmNsliderSize, amountOfLists,
				  XmNvalue, 0,
				  XmNincrement, 1,
				  XmNpageIncrement, amountOfLists,
				  XmNmaximum, amountOfLists,
				  NULL);
	}
	else if ( (listStart+amountOfLists) >= currentLoadVList )
	{
		//
		//  We have lists off to the left at this point and
		//  we don't have all physical lists full.
		//
		XtVaSetValues(scrollbar,
			  XmNsliderSize, amountOfLists,
			  XmNvalue, listStart,
			  XmNincrement, 1,
			  XmNpageIncrement, amountOfLists,
			  XmNmaximum, listStart + amountOfLists,
			  XmNminimum, 0,
			  NULL);
	}
	else
	{
		int value;
		value = listStart;
		if ( currentLoadVList - amountOfLists < listStart )
			value = currentLoadVList - amountOfLists;
		if ( value < 0 )
			value = 0;
		XtVaSetValues(scrollbar,
			  XmNsliderSize, amountOfLists,
			  XmNvalue, value,
			  XmNincrement, 1,
			  XmNpageIncrement, amountOfLists,
			  XmNmaximum, currentLoadVList,
			  XmNminimum, 0,
			  NULL);
	}
}

//
// Name: AddPixmap
//
// Purpose: To add pixmaps into the physical PLists.
//
// Description:
//
// Returns: Pixmap index number
//
// Side Effects: Pixmap load failure is considered fatal.
//
int 
MultiPList::AddPixmap(char  *pixmapName)
{
	Pixmap pixmap;
	int i;
	int index;

	index = plist[0]->UW_AddPixmap(pixmapName,&pixmap);
	assert(index != INVALID);

	for (i = 1; i < amountOfLists; i++)
	{
		plist[i]->UW_AddPixmap(pixmap);
	}
	return(index);
}
