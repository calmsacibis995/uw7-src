#ident	"@(#)PList.C	1.2"
#include "PList.h"
#include <Xm/List.h>
#include <Xm/ScrolledW.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

//
// Pixmap read routine function proto "C"
//
extern "C" int XReadPixmapFile(Display *,Window ,Colormap ,char *,
	 			unsigned int *, unsigned int *, unsigned int, Pixmap *);

//
// Strings to indicate who is responsible for the pixmap/mask
// space
//
const char *const CLIENT_OWNS_PIXMAP = {"Client_Owns_Pixmap"};
const char *const CLIENT_OWNS_MASK = {"Client_Owns_Mask"};
const char *const CLIENT_OWNS_ALL = {"Client_Owns_All"};

//
// Name: PList
//
// Purpose: Constructs an instance of a PList object 
//
// Description:  Initialize/assign class memeber variables and create
//               the scrolled list widget.  All list widget callbacks
//               are vectored to a class member callback handler.
//
//        NOTE:  This object expects it's parent to be a Motif Form
//               widget.
//
// Returns: Nothing
//
// Side Effects: A PList object created.
// 
PList::PList (Widget parent, char *name, int spacing = 0, int rowcount = 0, 
				int numcol = 0, int pixmapGeom, XtTranslations parsed_xlations,
				int visibleItemCount)
            : initialArraySize( 256 ), EXTRA_SPACE( 16 )
{

	Arg al[32];
	int ac;
	XtCallbackRec	item_init_cb[2];

	//
	// Setup the private variables now.
	//
	pixmapWidth = pixmapGeom;
	pixmapHeight = pixmapGeom;
	parentWidget = parent;
	pixmapCount = 0;
	itemCount = 0;
	selectedItem = NULL;
	callbackSingle = NULL;
	callbackDbl = NULL;

	//
	// Allocate space to contain the list item pixmap relationships
	// Dynamic resized if needed.
	//
	pixmapRelateSize = initialArraySize;
	pixmapRelate = (int *)XtMalloc(sizeof (int) * pixmapRelateSize);

	//
	// Allocate space to contain our copy of the list items
	// Dynamic resized if needed.
	//
	listItemsSize = initialArraySize;
	listItems = (XmString *)XtMalloc(sizeof (XmString) * listItemsSize);
	listItemsCount = 0;
	

	item_init_cb[0].closure = this; 	// clientData 
	item_init_cb[1].closure = NULL;
	item_init_cb[0].callback = ItemInitProc;
	item_init_cb[1].callback = NULL;

	//
	// Create the ScrolledWindow and list Widget
	//
	ac = 0;
	if (parsed_xlations != NULL)
	{
		XtSetArg(al[ac], XmNtranslations,  parsed_xlations); ac++;
	}
	XtSetArg(al[ac], XmNlistColumnSpacing, spacing); ac++;
	XtSetArg(al[ac], XmNstaticRowCount, rowcount); ac++;
	XtSetArg(al[ac], XmNnumColumns, numcol); ac++;
	XtSetArg(al[ac], XmNselectionPolicy, XmSINGLE_SELECT); ac++;
	XtSetArg(al[ac], XmNlistSizePolicy, XmCONSTANT); ac++;
	XtSetArg(al[ac], XmNscrollingPolicy, XmAUTOMATIC); ac++;
	XtSetArg(al[ac], XmNscrollBarDisplayPolicy, XmAS_NEEDED); ac++;
	XtSetArg(al[ac], XmNvisibleItemCount, visibleItemCount); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNautomaticSelection, True); ac++;
	XtSetArg(al[ac], XmNitemInitCallback, item_init_cb); ac++;
	listWidget = XmCreateScrolledList(parent, "list", al, ac);
	scrollWindowWidget = XtParent(listWidget);

	XtVaGetValues(listWidget, XmNbackground, &pixmapBackground, NULL);

	XtVaSetValues(scrollWindowWidget,
				XmNworkWindow, listWidget,
				NULL);
	//
	// Callbacks for the list
	//
	XtAddCallback (listWidget, XmNsingleSelectionCallback,
			&PList::ListWindowCallback, (XtPointer) this);
	XtAddCallback (listWidget, XmNdefaultActionCallback,
			&PList::ListWindowCallback, (XtPointer) this);
	XtAddCallback (listWidget, XmNextendedSelectionCallback,
			&PList::ListWindowCallback, (XtPointer) this);
	XtAddCallback (listWidget, XmNmultipleSelectionCallback,
			&PList::ListWindowCallback, (XtPointer) this);
	XtAddCallback (listWidget, XmNbrowseSelectionCallback,
			&PList::ListWindowCallback, (XtPointer) this);

	XtManageChild(listWidget);
	XtManageChild(scrollWindowWidget);
}

//
// Name: ~PList
//
// Purpose: Destroys an instance of a PList object 
//
// Description:   Free all resources that this object has 
//                acquired.  Do not free pixmaps that were
//                not created by this object, client owned.
//
//
// Returns: Nothing
//
// Side Effects: All resources destroyed ( freed ).
// 
PList::~PList ()
{
	int i;
	
	for (i = 0; i < itemCount; i++)
	{
		XmStringFree(listItems[i]);
	}
	XtFree((char *)pixmapRelate);
	XtFree((char *)listItems);
	UW_DelAllPixmaps();
}

//
// Name: UW_RegisterDblCallback
//
// Purpose: Client can give a function to call on a double click
//          and a data argument for that function.
//
// Description:   
//
//
// Returns: Nothing
//
// Side Effects: 
// 
void   
PList::UW_RegisterDblCallback(clientCallback func, XtPointer userData)
{
	callbackDbl = func;
	callbackDblData = userData;
}

//
// Name: UW_RegisterSingleCallback
//
// Purpose: Client can give a function to call on a single click
//          and a data argument for that function.
//
// Description:   
//
//
// Returns: Nothing
//
// Side Effects: 
// 
void   
PList::UW_RegisterSingleCallback(clientCallback func, XtPointer userData)
{
	callbackSingle = func;
	callbackSingleData = userData;
}

//
// Name: ListWindowCallback
//
// Purpose: Gives a single entry point for all "C" scope callbacks.
//
// Description:   Cast clientData to be an object pointer of type
//                PList and then call the object's callback handler.
//
// Returns: Nothing
//
// Side Effects: 
// 
void 
PList::ListWindowCallback (Widget w,
	                  XtPointer clientData,
	                  XtPointer callData)
{
	PList *obj = (PList *)clientData;
	obj->List(w, clientData, callData);
}

//
// Name: List
//
// Purpose: A C++ member function to handle List widget callbacks.
//
// Description:  Save the item that was clicked upon and call the
//               clients function for the click type.
//
// Returns: Nothing
//
// Side Effects: 
// 
void 
PList::List (Widget w, XtPointer, XtPointer callData)
{
	XmListCallbackStruct *cb = (XmListCallbackStruct *)callData;

	//
	// Free the previous selectedItem string if any
	//
	if (selectedItem != NULL)
		XmStringFree(selectedItem);
	selectedItem = NULL;

	//
	// Only set if there is a selected item in the list
	//
	if (cb->selected_item_count)
		selectedItem = XmStringCopy((XmString)cb->item);

	if (cb->reason == XmCR_SINGLE_SELECT)
	{
		if (callbackSingle)
			callbackSingle(w, callbackSingleData, callData);
	}
	else if (cb->reason == XmCR_DEFAULT_ACTION)
	{
		if (callbackDbl)
			callbackDbl(w, callbackDblData, callData);
	}
}

//
// Name: UW_ListItemExists
//
// Purpose: Checks of a specified item is in the list.
//
// Description:   
//
// Returns: True is the specified item is present in the list.
//
// Side Effects: 
// 
Boolean 
PList::UW_ListItemExists (XmString str)
{
	return (XmListItemExists (listWidget, str));
}

//
// Name: UW_SetSelectedItem
//
// Purpose: Highlight the matching XmString in the list widget and
//          add the specified item to the current selected list.
//
// Description:   
//
// Returns: Nothing
//
// Side Effects: 
// 
void 
PList::UW_SetSelectedItem(XmString str)
{
	XmListSelectItem(listWidget, str, False);
}

//
// Name: UW_SetSelectedItem
//
// Purpose: Highlight the XmString at position in the list widget and
//          add the specified item to the current selected list.
//
// Description:   
//
//
// Returns: Nothing
//
// Side Effects: 
// 
void 
PList::UW_SetSelectedItem(int pos)
{
	XmListSelectPos(listWidget, pos, False);
}

//
// Name: UW_SetTopItem
//
// Purpose: Makes an existing item the first visible item in the list.
//
// Description:   
//
// Returns: Nothing
//
// Side Effects: 
// 
void 
PList::UW_SetTopItem(int pos)
{
	XmListSetPos(listWidget, pos);
}

//
// Name: UW_GetSelectedItem
//
// Purpose: To get the XmString that was last selected by the user.
//
// Description:   
//
// Returns: XmString of the last user selected item.
//
// Side Effects: 
// 
XmString
PList::UW_GetSelectedItem()
{
	return (selectedItem);
}

//
// Name: UW_ListAddItem
//
// Purpose: A List function that adds an item to the list
//
// Description:   Adds an item to the list at a given position.
//                The item is compared with the current XmNselectedItems 
//                list.  If the new item matches an item on the selected
//                list, it appears selected. 
//
// Returns: Nothing
//
// Side Effects: New item in the list, possibly selected.
// 
void   
PList::UW_ListAddItem(XmString  xmstr, int pos, int pixmapIndex)
{
	int posInXmList;

	if (pos > itemCount + 1 || pos < 0)
	{
		// Error: Adding past end of list, change to append
		pos = APPEND;
	}
	posInXmList = pos;
	if (itemCount >= (pixmapRelateSize - 2))
	{
		GetMoreSpace(pixmapRelateSize);
	}
	//
	// Shift eveybody behind the insertion point down
	//
	if (pos == APPEND)
	{
		pos = itemCount;
	}
	else
	{
		pos--;
		memmove(&pixmapRelate[pos + 1], &pixmapRelate[pos],
						sizeof (int) * (itemCount - pos));
		memmove(&listItems[pos + 1], &listItems[pos],
						sizeof (XmString) * (itemCount - pos));
	}
	pixmapRelate[pos] = pixmapIndex;
	listItems[pos] = XmStringCopy(xmstr);
	itemCount += 1;
	if (pixmapIndex == INVALID || pixmapCount == 0)
		XmListAddItem(listWidget, xmstr, posInXmList);
	else
		XmListAddItem(listWidget, NULL, posInXmList);
}

//
// Name: UW_ListAddItems
//
// Purpose: A List function that adds an items to the list
//
// Description:   Adds the specified items to the list at the given
//                position. "count" items of the items array are
//                addes to the list. Items are compared with the 
//                current XmNselectedItems list, if an item matches
//                it will appear selected in the list.
//
// Returns: Nothing
//
// Side Effects: New items in the list, possibly selected.
// 
void   
PList::UW_ListAddItems(XmString *xmstr, int count, int pos, int *pixmapIndex)
{
	register int i;
	register int x;
	int posInXmList;

	if (pos > itemCount + 1 || pos < 0)
	{
		// Error: Adding past end of list, change to append
		pos = APPEND;
	}
	posInXmList = pos;

	if (itemCount + count >= (pixmapRelateSize - 2))
	{
		GetMoreSpace(count);
	}
	//
	// Shift eveybody behind the insertion point down
	//
	if (pos == APPEND)
	{
		pos = itemCount;
	}
	else
	{
		pos--;
		memmove(&pixmapRelate[pos + count], &pixmapRelate[pos],
						sizeof (int) * (itemCount - pos));
		memmove(&listItems[pos + count], &listItems[pos],
						sizeof (XmString) * (itemCount - pos));
	}
	//
	// Insert the new pixmaps and XmStrings
	//
	for (x = 0, i = pos; i <  pos + count; i++, x++)
	{
		if (pixmapIndex == NULL || pixmapCount == 0)
			pixmapRelate[i] = INVALID;
		else
			pixmapRelate[i] = pixmapIndex[x];
		listItems[i] = XmStringCopy(xmstr[x]);
	}
	itemCount += count;
	if (pixmapIndex == NULL || pixmapCount == 0)
		XmListAddItems(listWidget, xmstr, count, posInXmList);
	else
	{
		//
		// All list items must be null so that ItemInitProc can
		// specify the pixmap and XmString  combination
		//
		XmString *null_xmstr =(XmString *)XtCalloc(sizeof(XmString) * count, 1);
		XmListAddItems(listWidget, null_xmstr, count, posInXmList);
		XtFree((char *)null_xmstr);
	}
}
//
// Name: UW_ListAddItems
//
// Purpose: A List function that adds an items to the list
//
// Description:   Adds the specified items to the list at the given
//                position. "count" items of the items array are
//                addes to the list. Items are compared with the 
//                current XmNselectedItems list, if an item matches
//                it will appear selected in the list.
//
// Returns: Nothing
//
// Side Effects: New items in the list, possibly selected.
// 
void   
PList::UW_ListAddItems(XmString *xmstr, int count, int pos, int pixmapIndex)
{
	register int i;
	register int x;
	int posInXmList;

	if (pos > itemCount + 1 || pos < 0)
	{
		// Error: Adding past end of list, change to append
		pos = APPEND;
	}
	posInXmList = pos;

	if (itemCount + count >= (pixmapRelateSize - 2))
	{
		GetMoreSpace(count);
	}
	//
	// Shift eveybody behind the insertion point down
	//
	if (pos == APPEND)
	{
		pos = itemCount;
	}
	else
	{
		pos--;
		memmove(&pixmapRelate[pos + count], &pixmapRelate[pos],
						sizeof (int) * (itemCount - pos));
		memmove(&listItems[pos + count], &listItems[pos],
						sizeof (XmString) * (itemCount - pos));
	}
	//
	// Insert the new pixmaps and XmStrings
	//
	for (x = 0, i = pos; i <  pos + count; i++, x++)
	{
		if (pixmapIndex == INVALID || pixmapCount == 0)
			pixmapRelate[i] = INVALID;
		else
			pixmapRelate[i] = pixmapIndex;
		listItems[i] = XmStringCopy(xmstr[x]);
	}
	itemCount += count;
	if (pixmapIndex == INVALID || pixmapCount == 0)
		XmListAddItems(listWidget, xmstr, count, posInXmList);
	else
	{
		//
		// All list items must be null so that ItemInitProc can
		// specify the pixmap and XmString  combination
		//
		XmString *null_xmstr =(XmString *)XtCalloc(sizeof(XmString) * count, 1);
		XmListAddItems(listWidget, null_xmstr, count, posInXmList);
		XtFree((char *)null_xmstr);
	}
}

//
// Name: UW_ListAddItemUnselected
//
// Purpose: A List function that adds an item to the list
//          so that it is unselected.
//
// Description:   Adds an item to the list at a given position.
//                The item does not appear selected, even if it
//                matches an item in the current XmNselectedItems
//                list.
//
// Returns: Nothing
//
// Side Effects: New item in the list
// 
void   
PList::UW_ListAddItemUnselected(XmString  xmstr, int pos, int pixmapIndex)
{
	int posInXmList;

	if (pos > itemCount + 1 || pos < 0)
	{
		// Error: Adding past end of list, change to append
		pos = APPEND;
	}
	posInXmList = pos;
	if (itemCount >= (pixmapRelateSize - 2))
	{
		GetMoreSpace(pixmapRelateSize);
	}
	//
	// Shift eveybody behind the insertion point down
	//
	if (pos == APPEND)
	{
		pos = itemCount;
	}
	else
	{
		pos--;
		memmove(&pixmapRelate[pos + 1], &pixmapRelate[pos],
						sizeof (int) * (itemCount - pos));
		memmove(&listItems[pos + 1], &listItems[pos],
						sizeof (XmString) * (itemCount - pos));
	}
	pixmapRelate[pos] = pixmapIndex;
	listItems[pos] = XmStringCopy(xmstr);
	itemCount += 1;
	if (pixmapIndex == INVALID)
		XmListAddItemUnselected(listWidget, xmstr, posInXmList);
	else
		XmListAddItemUnselected(listWidget, NULL, posInXmList);
}

//
// Name: UW_ListAppendItem
//
// Purpose: A List function that adds an item to the end of the list
//
// Description:   Adds an item to the end of the list. The item is 
//                compared with the current XmNselectedItems list.
//                If the new item matches an item on the selected
//                list, it appears selected. 
//
// Returns: Nothing
//
// Side Effects: New item at the end of the list, possibly selected.
// 
void
PList::UW_ListAppendItem(XmString xmstr, int pixmapIndex)
{
	UW_ListAddItem(xmstr, APPEND, pixmapIndex);
}

//
// Name: UW_ListReplaceItems
//
// Purpose: A List function that replaces each specified item of 
//          the list with a corresponding new item.
//          It is assumed the pixmapRelationship has already been 
//          established when the XmString was first put into the list.
//
// Description:   
//
// Returns: Nothing
//
// Side Effects:  Internal list of XmStrings is freed and refilled
//                with the lists current items list.
// 
void
PList::UW_ListReplaceItems(XmString *old_items, int item_count,
						XmString *new_items)
{
	int i;
	XmString *items;

	assert (item_count < (itemCount + 1));
 
	XmListReplaceItems(listWidget, old_items,item_count,new_items);
	XtVaGetValues(listWidget, XmNitems, &items,NULL);

	//
	// Update our copy of the list items
	//
	for (i = 0; i < itemCount; i++)
	{
		XmStringFree(listItems[i]);
		listItems[i] = XmStringCopy(items[i]);
	}
}

//
// Name: UW_ListReplaceItemsPos
//
// Purpose: A List function that replaces the specified number of items
//          of the list with new items, starting at the specified 
//          position in the list.  It is assumed the pixmapRelationship
//          has already been established when the XmString was first
//          put into the list.
//
// Description:
//
// Returns: Nothing
//
// Side Effects:
// 
void
PList::UW_ListReplaceItemsPos(XmString *new_items, int item_count,
								int position)
{
	int pos;
	int i;

	assert (position >= 0);
	assert (position < (itemCount + 1));
 
	XmListReplaceItemsPos(listWidget, new_items,item_count,position);
	pos = position - 1;

	//
	// Update our copy of the list items
	//
	for (i = pos; i < pos + item_count; i++, new_items++)
	{
		XmStringFree(listItems[i]);
		listItems[i] = XmStringCopy(*new_items);
	}
}

//
// Name: UW_ListDeleteAllItems
//
// Purpose: To delete all items within the list widget
//
// Description:   All items are deleted from the list and all
//                associated object internal resources are freed
//
// Returns: Nothing
//
// Side Effects: 
// 
void   
PList::UW_ListDeleteAllItems()
{
	register int i;

	XmListDeleteAllItems(listWidget);
	for (i = 0; i < itemCount; i++)
	{
		pixmapRelate[i] = INVALID;
		XmStringFree(listItems[i]);
	}
	itemCount = 0;
}

//
// Name: UW_ListDeleteItem
//
// Purpose: Deletes a specified item from the list if it exists.
//
// Description:   If the item does not exist, no action is taken.
//
// Returns: Nothing
//
// Side Effects: 
// 
void   
PList::UW_ListDeleteItem(XmString item)
{
	int pos;
 
	pos = XmListItemPos(listWidget, item);
	if (pos)
	{
		XmListDeleteItem(listWidget, item);
		pos--;
		XmStringFree(listItems[pos]);
		memmove(&pixmapRelate[pos], &pixmapRelate[pos + 1],
							sizeof (int) * (itemCount - pos));
		memmove(&listItems[pos], &listItems[pos + 1],
							sizeof (XmString) * (itemCount - pos));
		pixmapRelate[itemCount] = INVALID;
		itemCount -= 1;
	}
}

//
// Name: UW_ListDeleteItems
//
// Purpose:  Deletes the specified items for the list.  A warning
//           message is generated by the list widget if any of the
//           items do not exist.
//
// Description:   Items in the objects internal list and their
//                associated resources are freed/deleted.
//
// Returns: Nothing
//
// Side Effects: 
// 
void   
PList::UW_ListDeleteItems(XmString *item, int item_count)
{
	int i, x;
	int *indexes;
	int pos;

	assert (item_count < (itemCount + 1));

	//
	// Get a list of the list positions to use
	// for deletions, possible un-sequential
	//
	indexes = (int *)malloc(sizeof (int) * item_count);
	for (i = x = 0; x < item_count; x++, i++)
	{
		pos = XmListItemPos(listWidget, item[x]);
		if (pos)
		{
			indexes[i] = pos - 1;
		}
	}
	//
	// Delete the list items, user will see the list
	// error if all XmStrings are not present in the
	// list
	//
	XmListDeleteItems(listWidget, item, item_count);

	//
	// We can only delete those that we found
	// so set item_count equal to the amount that
	// had positions
	//
	item_count = i;

	//
	// Delete the pixmaps indexes and XmStrings from our internal lists
	//
	for (x = 0; x < item_count; x++)
	{
		i = indexes[x];
		XmStringFree(listItems[i]);
		memmove(&pixmapRelate[i], &pixmapRelate[i + 1],
						sizeof (int) * (itemCount - i));
		memmove(&listItems[i], &listItems[i + 1],
						sizeof (XmString) * (itemCount - i));
	}
	itemCount -= item_count;
	pixmapRelate[itemCount + 1] = INVALID;
	free((char *)indexes);
}

//
// Name: UW_ListDeleteItemsPos
//
// Purpose: Deletes the specified number of items from the list
//          starting at the specified position.
//
// Description:   Internal list resources freed and deleted.
//
// Returns: Nothing
//
// Side Effects: 
// 
void   
PList::UW_ListDeleteItemsPos(int item_count, int position)
{
	int x;
	int pos;

	assert (position >= 0);
//	THIS DOESN'T WORK for multi column lists
//	assert ((position + item_count) < (itemCount + 1));

	//
	// Delete the list items
	//
	XmListDeleteItemsPos(listWidget, item_count, position);

	//
	// Delete the pixmaps indexes and XmStrings from our internal lists
	//
	pos = position - 1;
	for (x = 0; x < item_count; x++, pos++)
	{
		XmStringFree(listItems[pos]);
		memmove(&pixmapRelate[pos], &pixmapRelate[pos + 1],
						sizeof (int) * (itemCount - pos));
		memmove(&listItems[pos], &listItems[pos + 1],
						sizeof (XmString) * (itemCount - pos));
	}
	itemCount -= item_count;
	pixmapRelate[itemCount + 1] = INVALID;
}
//
// Name: UW_ListDeletePos
//
// Purpose: Deletes an item at the specified position.
//
// Description:   
//
//
// Returns: Nothing
//
// Side Effects: 
// 
void   
PList::UW_ListDeletePos(int pos)
{
	assert (pos >= 0);
	assert (pos < (itemCount + 1));

	XmListDeletePos(listWidget, pos);
	pos--;
	XmStringFree(listItems[pos]);
	memmove(&pixmapRelate[pos], &pixmapRelate[pos + 1],
					sizeof (int) * (itemCount - pos));
	memmove(&listItems[pos], &listItems[pos + 1],
					sizeof (XmString) * (itemCount - pos));
	pixmapRelate[itemCount] = INVALID;
	itemCount -= 1;
}

//
// Name: UW_AddPixmap
//
// Purpose: Creates a pixmap from the xpm file specified. Returns the 
//          index number and will copy the pixamp ID to the specified 
//          pixmap location, if not NULL.
//
// Description:   
//
// Returns: Pixmap reference number, -1 on error
//
// Side Effects: 
//
int    
PList::UW_AddPixmap(char *pixmapName, Pixmap *pixmap)
{
	unsigned int ph, pw;
	Window root;
	int screen;
	Display *dpy;

	assert (pixmapCount < maxPixmapFiles);

	dpy = XtDisplay(listWidget);
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);

	if (XReadPixmapFile (dpy , root,
		XDefaultColormap(dpy, screen), pixmapName, &pw, &ph,
		XDefaultDepth(dpy, screen), &pix[pixmapCount].pixmap))
	{
		return (-1);
	}
	pix[pixmapCount].name = strdup(pixmapName);
	GetPixmapMask(listWidget, &pix[pixmapCount]);
	if ( pixmap != NULL )
		*pixmap = pix[pixmapCount].pixmap;
	pixmapCount++;
	return (pixmapCount - 1);
}

//
// Name: UW_AddPixmap
//
// Purpose: Adds a pixmap ID to the objects internal list. 
//          This pixmap resource is marked as client owned.
//
// Description:   
//
// Returns: Pixmap reference number
//
// Side Effects: 
//
int    
PList::UW_AddPixmap(Pixmap pixmap, Pixmap mask)
{
	pix[pixmapCount].pixmap = pixmap;
	if (mask != 0 )
	{
		pix[pixmapCount].mask = mask;
		pix[pixmapCount].name = strdup(CLIENT_OWNS_ALL);
	}
	else
	{
		pix[pixmapCount].name = strdup(CLIENT_OWNS_PIXMAP);
		GetPixmapMask(listWidget, &pix[pixmapCount]);
	}
	pixmapCount++;
	return (pixmapCount - 1);
}

//
// Name: GetPixmapMask
//
// Purpose: Create a mask pixmap for the specified pixmap
//
// Description:   
//
// Returns: Nothing
//
// Side Effects: 
// 
void
PList::GetPixmapMask(Widget w, struct pixmapStruct *pix)
{
/* tony
	DmGlyphRec g;

	g.pix = pix->pixmap;
	g.width = pixmapWidth;
	g.height = pixmapHeight;
	g.depth = DefaultDepthOfScreen(XtScreen(w));
	Dm__CreateIconMask(XtScreen(w), &g);
	pix->mask = g.mask;
*/
}

//
// Name: UW_ChangePixmap
//
// Purpose: 
//
// Description: Change pixmap at pixmapIndex to the new named pixmap
//
//
// Returns: The index number
//
// Side Effects: All list items that were related to the
//               old pixmap at the pixmapIndex will be redone 
//               to be related to the new pixmap.
//               Out of bounds = assertion failure, core dump :-)
//
//
int    
PList::UW_ChangePixmap(int pixmapIndex, char *newPixmapName)
{
	Pixmap oldPixmap;
	Pixmap oldMask;
	int i;
	int index;
	char *oldName;

	assert (pixmapIndex >= 0);
	assert (pixmapIndex < pixmapCount);
		
	//
	// Add the new pixmap to the end of the list
	//
	index = UW_AddPixmap(newPixmapName);

	oldPixmap = pix[pixmapIndex].pixmap;
	oldMask = pix[pixmapIndex].mask;
	oldName = pix[pixmapIndex].name;

	//
	// Move the added pixmap information into the
	// correct index, set count back one
	//
	pix[pixmapIndex].name = strdup(newPixmapName);
	pix[pixmapIndex].pixmap = pix[index].pixmap;
	pix[pixmapIndex].mask = pix[index].mask;
	pixmapCount--;

	//
	// Need to change the relationship of all items that had
	// the old pixmap.
	//
	for (i = 0; i < itemCount; i++)
	{
		if ( pixmapRelate[i] == pixmapIndex )
			UW_ChangePixmapRelation(i + 1,pixmapIndex);
	}
	//
	// Free the old pixmaps AFTER we have established the new ones
	// but only if it was created by us
	//
	if (strcmp(oldName,CLIENT_OWNS_PIXMAP) == 0 )
	{
		XFreePixmap(XtDisplay(listWidget), oldMask);
	}
	else if (strcmp(oldName,CLIENT_OWNS_MASK) == 0 )
	{
		XFreePixmap(XtDisplay(listWidget), oldPixmap);
	}
	else if (strcmp(oldName,CLIENT_OWNS_ALL) != 0 )
	{
		XFreePixmap(XtDisplay(listWidget), oldMask);
		XFreePixmap(XtDisplay(listWidget), oldPixmap);
	}

	free((char *)oldName);
	return (index);
}

//
// Name: UW_ChangePixmapRelation
//
// Purpose: Change the pixmap relationship for list item at itemPosition
//          to the specified new pixmap reference.
//
// Description:   
//
// Returns: Nothing
//
// Side Effects: Out of bounds = assertion failure, core dump :-)
//
//
void    
PList::UW_ChangePixmapRelation(int itemPosition, int pixmapIndex)
{
	assert (pixmapIndex >= 0);
	assert (pixmapIndex < pixmapCount);
	assert ((itemPosition - 1) >= 0);
	assert ((itemPosition - 1) < itemCount);

	pixmapRelate[itemPosition - 1] = pixmapIndex;
	//
	// Need to delete the old XmString from the List and then 
	// add the string back as a NULL. ItemInitProc will get 
	// called because of the NULL string.  ItemInitProc will use 
	// the new pixmapIndex and vahlah the relationship is changed.
	//
	XmListDeletePos(listWidget, itemPosition);
	XmListAddItemUnselected(listWidget, NULL, itemPosition);
}

//
// Name: UW_ChangePixmapRelation
//
// Purpose: Change the pixmap relationship for list item at itemPosition
//          to the specified pixmap name.
//
// Description:   
//
// Returns: Nothing
//
// Side Effects: 
//
void    
PList::UW_ChangePixmapRelation(int itemPosition, char *pixmapName)
{
	int i;

	for (i = 0; i < pixmapCount; i++)
	{
		if (strcmp(pixmapName, pix[i].name) == 0)
		{
			UW_ChangePixmapRelation(itemPosition, i + 1);
		}
	}
}

//
// Name: UW_DelAllPixmaps
//
// Purpose:  Deletes all object owned pixmaps.
//
// Description:   
//
// Returns: Nothing
//
// Side Effects: All pixmaps are now invalid, list will be deleted and
//               then redrawn without pixmaps if reloadFlag is TRUE.
//
//
void   
PList::UW_DelAllPixmaps(int reloadFlag)
{
	int i;

	for (i = 0; i < pixmapCount; i++)
	{
		//
		// Delete the pixmap from the server
		// if we created it
		//
		if (strcmp(pix[i].name,CLIENT_OWNS_PIXMAP) == 0 )
		{
			XFreePixmap(XtDisplay(listWidget), pix[i].mask);
		}
		else if (strcmp(pix[i].name,CLIENT_OWNS_MASK) == 0 )
		{
			XFreePixmap(XtDisplay(listWidget), pix[i].pixmap);
		}
		else if (strcmp(pix[i].name,CLIENT_OWNS_ALL) != 0 )
		{
			XFreePixmap(XtDisplay(listWidget), pix[i].pixmap);
			XFreePixmap(XtDisplay(listWidget), pix[i].mask);
		}
		pix[i].pixmap = 0;
		pix[i].mask = 0;
		free((char *)pix[i].name);
		pix[i].name = NULL;
	}
	pixmapCount = 0;
	if ( reloadFlag == True )
	{
		XmListDeleteAllItems(listWidget);
		for ( i = 0; i < itemCount; i++)
		{
			XmListAddItem(listWidget, listItems[i],APPEND);
		}
	}
}
//
// Name: UW_GetPixmapAtIndex
//
// Purpose: Get the pixmap ID for a given pixmap index
//
// Description:   Check the bounds of the pixmapIndex and
//                return the pixmap ID.
//
// Returns: The pixmap at the index number
//
// Side Effects: Out of bounds = assertion failure, core dump :-)
// 
Pixmap     
PList::UW_GetPixmapAtIndex(int pixmapIndex)
{
	assert (pixmapIndex >= 0);
	assert (pixmapIndex < pixmapCount);

	return (pix[pixmapIndex].pixmap);
}

//
// Name: UW_GetPixmapIndex
//
// Purpose: Get the pixmap ID for a specified pixmap name
//
// Description:   
//
// Returns: The index number of the named pixmap, -1 on failure
//
// Side Effects: 
// 
int    
PList::UW_GetPixmapIndex(char *pixmapName)
{
	int i;

	for (i = 0; i < pixmapCount; i++)
	{
		if (strcmp(pixmapName, pix[i].name) == 0)
		{
			return (i);
		}
	}
	return (-1);
}

//
// Name: UW_GetPixmapIndex
//
// Purpose: Get the pixmap ID for a specfied pixmap ID
//
// Description:   
//
// Returns: The index number of the pixmap ID, -1 on failure
//
// Side Effects: 
// 
int    
PList::UW_GetPixmapIndex(Pixmap pixmap)
{
	int i;

	for (i = 0; i < pixmapCount; i++)
	{
		if (pixmap == pix[i].pixmap)
		{
			return (i);
		}
	}
	return (-1);
}

//
// Name: UW_GetPixmapName
//
// Purpose: Get the pixmap name for the specified pixmap reference
//
// Description:   
//
// Returns: The name of pixmap at pixmapIndex
//
// Side Effects: Out of bounds = assertion failure, core dump :-)
//
int    
PList::UW_GetPixmapName(int pixmapIndex, char *pixmapName)
{
	assert (pixmapIndex >= 0);
	assert (pixmapIndex < pixmapCount);
	
	if (pixmapName != NULL)
		pixmapName = pix[pixmapIndex].name;
	return (pixmapIndex);
}
//
// Name: UW_GetPixmapBackground
//
// Purpose: Get the pixmap background color.
//
// Description:   
//
// Returns: Nothing
//
// Side Effects: 
// 
unsigned long 
PList::UW_GetPixmapBackground()
{
	return (pixmapBackground);
}

//
// Name: GetMoreSpace
//
// Purpose: A function to increase the amount of local list item storage
//          space.
//
// Description:   Adds the argumented amount to the current list storage
//                size and reallocate storage based on the new size.
//                Invalidates the new list items.
//
// Returns: Nothing
//
// Side Effects: XtRealloc failure is space all used up
// 
void
PList::GetMoreSpace(int incBy)
{
	int i;

	pixmapRelateSize = pixmapRelateSize + incBy + EXTRA_SPACE;
	listItemsSize = pixmapRelateSize;
	pixmapRelate = (int *)XtRealloc((char *)pixmapRelate, 
					sizeof (int) * pixmapRelateSize);

	listItems = (XmString *)XtRealloc((char *)listItems, 
					sizeof (XmString) * listItemsSize);
	for (i = itemCount; i < pixmapRelateSize; i++)
	{
		pixmapRelate[i] = INVALID;
		listItems[i] = 0;
	}
}

//
// Name: ItemInitProc
//
// Purpose:  List item callback for list items that have a pixmap
//           relationship.
//
// Description:   
//           This routine is called whenever an XmNItem is NULL
//           ie: items 0 and 2 are set to a XmString, item 1 is NULL
//               during the list display, display of item 1 will cause
//               a vector to here so we can specify the item contents
//               (only happens on first realization)
//
// Returns: Nothing
//
// Side Effects: 
//
void
PList::ItemInitProc(Widget w, XtPointer clientData, XtPointer callData)
{
	register XmListItemInitCallbackStruct *cd = 
						(XmListItemInitCallbackStruct *)callData;
	register PList *obj = (PList *)clientData;
	register int index;

	// We are only expecting this reason 
	if (cd->reason != XmCR_ASK_FOR_ITEM_DATA)
	    return;

	index = cd->position - 1;
	cd->label = obj->listItems[index];
//	if (obj->pixmapCount != 0)
	{
//		if ( obj->pixmapRelate[cd->position] != INVALID )
		{
			cd->pixmap = obj->pix[obj->pixmapRelate[index]].pixmap;
			cd->mask =  obj->pix[obj->pixmapRelate[index]].mask;
			cd->width = obj->pixmapWidth;
			cd->height = obj->pixmapHeight;
			cd->depth =  DefaultDepthOfScreen(XtScreen(w));
			cd->h_pad = 5;          /* pixels */
		}
	}
	cd->v_pad = 0;          
//	cd->glyph_on_left = True;
	cd->static_data = True;    // I do all pixmap deletes
} 

