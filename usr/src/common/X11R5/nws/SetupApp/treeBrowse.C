#ident	"@(#)treeBrowse.C	1.2"
/*  treeBrowse.C
//
//  This file contains the TreeBrowse object member functions.
*/



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream.h>

#include "MultiPList.h"
#include "setup.h"
#include "treeBrowse.h"
#include "controlArea.h"
#include <mail/setupList.h>
#include <mail/setupTypes.h>





/* //////////////////////////////////////////////////////////////////////////////
//
// 	Constructors/Destructors.
//
////////////////////////////////////////////////////////////////////////////// */

treeBrowse::treeBrowse (XtAppContext appContext, Widget parent, setupObject_t *listObject, XtPointer win, Boolean editButtons, int numColumns, int visibleItemCount)
{
	void		*root;
	callback_t 	*callback_p;
	Widget		browserTitle, buttonForm;
	Dimension	listWidth = (Dimension)0;
	treeList_t	*treeList;



	d_node = 0;
	d_topWidget = d_addButton = d_remButton = (Widget)0;
	d_openCallback = d_dblClickCallback = 0;
	d_selectCallback = 0;
	d_unselectCallback = 0;
	d_listNum = d_nodeIndex = 0;
	d_win = win;
	d_canClose = True;
	d_openedList = d_loadedList = (callback_t *)0;

	//  Create a form for the entire browser [w/ or w/o edit buttons].
	d_topWidget = XtVaCreateManagedWidget ("browserForm",
			xmFormWidgetClass,	parent,
			NULL);

	if (root = setupListTree (listObject))
	{
		XmFontList	fontList;

		treeList = nodeTreeList (root);
		callback_p = treeListOpen (treeList, NULL, NULL);
		
		//  The list widget uses the textFontList which may be different
		//  from the labelFontList or other.
		XtVaGetValues (d_topWidget, XmNtextFontList, &fontList, NULL);
		listWidth = calcWidestListItem (fontList, treeList);
	}

	/*  Instantiate a numColumns-wide browser, 6 items in each list.     */
	if (!(d_list = new MultiPList (d_topWidget, numColumns,
			visibleItemCount, &treeBrowse::selectCallback,
			(void*)this, 0, 0, 0)))
	{
#ifdef DEBUG
		cerr << "Constructor Failed" << endl;
#endif
		exit (0);	//	This should be done differently !!!
	}

	d_nodeList[0] = (void *)0;

	//  Put up the title for the browser list.
	browserTitle = XtVaCreateManagedWidget ("browserTitle",
			  	xmLabelWidgetClass,	d_topWidget,
			  	XmNlabelString,	   XmStringCreateLocalized
					       (setupObjectLabel (listObject)),
			  	NULL);

	if (editButtons)
		buttonForm = createEditButtons (d_topWidget, (SetupWin *)win);

	XtVaSetValues (browserTitle,
				XmNtopAttachment,	XmATTACH_FORM,
				XmNtopOffset,		10,
				XmNleftAttachment,	editButtons?
						XmATTACH_WIDGET : XmATTACH_FORM,
				XmNleftWidget,		buttonForm,
				XmNrightAttachment,	XmATTACH_FORM,
				NULL);
	if (editButtons)
	{
		XtVaSetValues (buttonForm,
				XmNtopAttachment,	XmATTACH_POSITION,
				XmNtopPosition,		55,
				XmNleftAttachment,	XmATTACH_FORM,
				XmNleftOffset,		4,
				XmNbottomAttachment,	XmATTACH_NONE,
				NULL);
	}

	XtVaSetValues (d_list->GetTopWidget (), 
				XmNtopAttachment,	XmATTACH_WIDGET,
				XmNtopWidget,		browserTitle,
				XmNbottomAttachment,	XmATTACH_FORM,
				XmNleftAttachment,	editButtons?
						XmATTACH_WIDGET : XmATTACH_FORM,
				XmNleftWidget,		buttonForm,
				XmNleftOffset,		editButtons? 7 : 0,
				XmNrightAttachment,	XmATTACH_FORM,
				XmNwidth,	      numColumns*(listWidth+18),
				NULL);

	d_list->SetOpenCallback (&treeBrowse::openCallback);
	d_list->SetClearCallback (&treeBrowse::clearCallback);
	d_list->SetUnSelectCallback (&treeBrowse::unselectCallback);

	if (root = setupListTree (listObject))
	{
		d_openedList = callback_p = treeListOpen (nodeTreeList (root),
				(void (*)())loadListCallback, (void*)this);
		
		callbackLevelSet (callback_p, 0);
	}

	d_list->DisplayLists ();

}	//  End  treeBrowse::treeBrowse ()



Dimension
treeBrowse::calcWidestListItem (XmFontList fontList, setupObject_t *list)
{
	void		*nodePtr;
	Dimension	height, width, maxWidth = (Dimension)0;
	char		*itemName;
	XmString	xmStr;


	for (nodePtr=treeListGetFirst (list) ; nodePtr ;
						  nodePtr = nodeNext (nodePtr))
	{
		itemName = nodeName (nodePtr);
		xmStr = XmStringCreateLocalized (itemName);
		XmStringExtent (fontList, xmStr, &width, &height);

		if (width > maxWidth)
			maxWidth = width;

		XmStringFree (xmStr);
	}

	return (maxWidth);

}	//  End  treeBrowse::calcWidestListItem ()



treeBrowse::~treeBrowse ()
{
	//  Clear all lists
	d_listNum = -1;
	d_list->ClearLists (0);

	//  It could have happened that we had an "open in progress",
	//  meaning that the user "opened" a node, but the loadList()
	//  never occured.  If the user cancelled the window, then we
	//  couldn't have closed the partially opened list because it
	//  never finished opening.  We must close it here, to prevent
	//  loadList from being called once we're gone.
	if (d_openedList != d_loadedList)
	{
		treeListCallbackClose (d_openedList);
	}
	delete (d_list);

}	//  End  treeBrowse::~treeBrowse ()



////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void  treeBrowse::setDblClickCallback (DblClickCallback func,
//							XtPointer data)
//
//  DESCRIPTION:
//	Save what the user says is the double-click (open) callback and its
//  client data.  This is what gets called when an item in the browser
//  is to be "opened".
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

void
treeBrowse::setDblClickCallback (DblClickCallback func, XtPointer data)
{
	d_openCallback = func;
	d_openClientData = data;

}	//  End  treeBrowse::setDblClickCallback ()



////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void  treeBrowse::setSelectCallback (SelectCallback func,XtPointer data)
//
//  DESCRIPTION:
//	Save what the user says is the select callback and its client data.
//	This is what gets called when an item in the browser gets selected.
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

void
treeBrowse::setSelectCallback (SelectCallback func, XtPointer data)
{
	d_selectCallback = func;
	d_selectClientData = data;

}	//  End  treeBrowse::setSelectCallback ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void treeBrowse::setUnselectCallback (SelectCallback func,
//								XtPointer data)
//
//  DESCRIPTION:
//	Save what the user says is the unselect callback and its client data.
//	This is what gets called when an item in the browser gets unselected.
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

void
treeBrowse::setUnselectCallback (UnselectCallback func, XtPointer data)
{
	d_unselectCallback = func;
	d_unselectClientData = data;

}	//  End  treeBrowse::setUnselectCallback ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	const char *treeBrowse::getLeafName ()
//
//  DESCRIPTION:
//
//  RETURN:
//	Return the currently selected leaf node name.
//
///////////////////////////////////////////////////////////////////////////// */

const char* 
treeBrowse::getLeafName ()
{
	if (isLeaf ())
		return (nodeName (d_node));

	return (0);

}	//  End  treeBrowse::getLeafName ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void *treeBrowse::getSelectedNode ()
//
//  DESCRIPTION:
//
//  RETURN:
//	Return the currently selected node.
//
///////////////////////////////////////////////////////////////////////////// */

void* 
treeBrowse::getSelectedNode ()
{
	return (d_node);

}	//  End  treeBrowse::getSelectedNode ()



void 
treeBrowse::setSelectedNode (void *nodeValue)
{
	d_node = nodeValue;

}	//  End  treeBrowse::setSelectedNode ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	Widget	treeBrowse::getTopWidget ()
//
//  DESCRIPTION:
//
//  RETURN:
//	Return the highest level widget id.
//
///////////////////////////////////////////////////////////////////////////// */

Widget
treeBrowse::getTopWidget ()
{
	return (d_topWidget);

}	//  End  treeBrowse::getTopWidget ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	treeBrowse::clear (int listNum, XtPointer callback_p)
//
//  DESCRIPTION:
//	This is the MultiPList Clear Callback.
//
//  RETURN:
//
///////////////////////////////////////////////////////////////////////////// */

void
treeBrowse::clear (int listNum, callback_t *callback_p)
{
	if (listNum >= d_listNum && d_canClose)
		treeListCallbackClose (callback_p);

}	//  End  treeBrowse::clear ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	treeBrowse::clearCallback (XtPointer mlp, int listNum,
//							XtPointer callback_p)
//
//  DESCRIPTION:
//
//  RETURN:
//
///////////////////////////////////////////////////////////////////////////// */

void 
treeBrowse::clearCallback (XtPointer mlp, int listNum, XtPointer callback_p)
{
	MultiPList	*list = (MultiPList*)mlp;
	treeBrowse	*thisPtr;

	thisPtr = (treeBrowse *)list->GetObjClientData ();
	thisPtr->clear (listNum, (callback_t *)callback_p);

}	//  End  treeBrowse::clearCallback ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	treeBrowse::open (char *item, int listNum, void *data)
//
//  DESCRIPTION:
//	This is the MultiPList Open Callback.  data, is the node that is
//	being "opened".
//
//  RETURN:
//
///////////////////////////////////////////////////////////////////////////// */

void 
treeBrowse::open (char *item, int listNum, void *data)
{
	void		*treePt;
	callback_t	*callback_p;
	int		tmpListNum;
	Window		window;


	if (data)
	{
		d_nodeIndex=listNum;
		d_nodeList[d_nodeIndex]=data;
	}

	d_node = data;
	window = XtWindow (d_list->GetTopWidget());
	setCursor (C_WAIT, window, C_FLUSH);

	if (nodeIsInternal (d_node))
	{
		d_listNum = listNum;
		tmpListNum = listNum + 1;
		d_list->ClearLists (tmpListNum);
		d_listNum = tmpListNum;
		treePt = nodeTreeList (d_node);

		//  The user is trying to open a second interior node
		//  before the first has been completed.  Close out
		//  the first before opening the second.
		if (d_openedList != d_loadedList)
		{
			treeListCallbackClose (d_openedList);
			d_openedList = d_loadedList;
		}

		//  Save return value for destruction time.
		d_openedList = callback_p = treeListOpen (treePt,
				(void (*)())loadListCallback, (void*)this);
		callbackLevelSet(callback_p, tmpListNum);
	}
	else	//  This is a leaf node, so open it.
	{
		nodeOpen (d_node);
	}


	if (d_openCallback)
	{
		d_openCallback (d_openClientData, d_node);
	}

	if (!nodeIsInternal (d_node))
		setCursor (C_POINTER, window, C_FLUSH);

}	//  End  treeBrowse::open ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	treeBrowse::openCallback (XtPointer mlp, XtPointer data)
//
//  DESCRIPTION:
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

void 
treeBrowse::openCallback (XtPointer mlp, char *item, int listNum, XtPointer data)
{
	MultiPList	*list = (MultiPList *)mlp;
	treeBrowse	*thisPtr;

	thisPtr = (treeBrowse *)list->GetObjClientData ();
	thisPtr->d_node = data;
	thisPtr->open (item, listNum, (void *)data);

}	//  End  treeBrowse::openCallback ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	treeBrowse::select (char *item, int listNum, void *data)
//
//  DESCRIPTION:
//	This is the MultiPList Select Callback.
//
//  RETURN:
//
///////////////////////////////////////////////////////////////////////////// */

void 
treeBrowse::select (char *item, int listNum, void *data)
{

	if (data)
	{
		d_nodeIndex=listNum;
		d_nodeList[d_nodeIndex]=data;
	}

	d_node = data;
	d_listNum = listNum + 1;
	d_list->ClearLists (listNum+1);
	nodeSelect (d_node);

	if (d_selectCallback)
	{
		d_selectCallback (d_selectClientData, d_node);
	}

}	//  End  treeBrowse::select ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	treeBrowse::selectCallback (XtPointer mlp, XtPointer data)
//
//  DESCRIPTION:
//
//  RETURN:
//	Nothing.
//
//////////////////////////////////////////////////////////////////////////// */

void 
treeBrowse::selectCallback (XtPointer mlp, char *item, int listNum, XtPointer data)
{
	MultiPList	*list = (MultiPList *)mlp;
	treeBrowse	*thisPtr;

	thisPtr = (treeBrowse *)list->GetObjClientData ();
	thisPtr->d_node = data;
	thisPtr->select (item, listNum, (void *)data);

}	//  End  treeBrowse::selectCallback ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	treeBrowse::unselect (int listNum)
//
//  DESCRIPTION:
//	This is the MultiPList unselect callback.
//
//  RETURN:
//	Nothing.
//
//////////////////////////////////////////////////////////////////////////// */

void 
treeBrowse::unselect (int listNum)
{

	if (listNum > 0)	//  A previous column has something selected.
	{
		d_nodeIndex = listNum - 1;
		d_node = d_nodeList[d_nodeIndex];
	}
	else	//  We're in the 1st column of the browser - nothing selected.
	{
		d_nodeIndex = 0;
		d_node = d_nodeList[0] = (void *)0;
	}

	//  The user is unselecting a node that is still in the process of
	//  being opened.  Close out the node and undo the watch cursor.
	if (d_openedList != d_loadedList)
	{
		treeListCallbackClose (d_openedList);
		d_openedList = d_loadedList;
		Window window = XtWindow (d_list->GetTopWidget ());
		if (window)
			setCursor (C_POINTER, window, C_FLUSH);
	}

	d_listNum = listNum + 1;
	d_list->ClearLists (listNum + 1);
	nodeDeselect (d_node);

	if (d_unselectCallback)
	{
		d_unselectCallback (d_unselectClientData);
	}

}	//  End  treeBrowse::unselect ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	treeBrowse::unselectCallback (XtPointer mlp, int listNum)
//
//  DESCRIPTION:
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

void 
treeBrowse::unselectCallback (XtPointer mlp, int listNum)
{
	MultiPList	*list = (MultiPList*)mlp;
	treeBrowse	*thisPtr;

	thisPtr = (treeBrowse *)list->GetObjClientData ();
	thisPtr->unselect (listNum);

}	//  End  treeBrowse::unselectCallback ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	treeBrowse::loadList (void *list, int state,
//							callback_t *callback_p)
//
//  DESCRIPTION:
//	treeListOpen callback to load the list of node names.
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

void
treeBrowse::loadList (void *list, int state, callback_t *callback_p)
{
	XmString	xmStr;
	void		*nodePt, *selectedNode = (void *)0;
	char		*name;
	char		*str;
	Window		window;

	switch (state)
	{
		case	2:	//  something changed in list
		{
			int	callbackListNum = callbackLevel (callback_p);

			if (callbackListNum != (d_listNum - 1))
			{
			    fprintf (stderr,
"Number mismatch callbackListNum = %d, d_listNum = %d.\n", callbackListNum, d_listNum);
			    break;
			}

			d_listNum = callbackListNum;

			//  If there was an item selected in this list, save it
			//  so that we can re-select it after the list gets
			//  cleared then repainted.
			if (d_nodeList[d_nodeIndex])
				selectedNode = d_nodeList[d_nodeIndex];

			//  Since we're updating the list, we don't want to
			//  call treeListCallbackClose(), because that would
			//  prevent the list from getting updated after it
			//  is cleared.  That's what this kludgy d_canClose
			//  flag is for.
			d_canClose = False;
			d_list->ClearLists (d_listNum);
			d_canClose = True;
		}

		case	1:	//  open successful
		case	0:	//  open not completely successful
		{
			d_list->SetListClientData ((XtPointer)callback_p);

			for (nodePt = treeListGetFirst (list); nodePt;
						     nodePt = nodeNext (nodePt))
			{
				if (!(name = nodeName (nodePt)))
				{
					continue;
				}

				if (nodeIsInternal (nodePt))
				{
					if (!(str = new char[strlen(name) + 4]))
					{
						continue;
					}

					sprintf (str, "%s >", name);
					xmStr = XmStringCreateLocalized (str);
					delete (str);
				}
				else
				{
					xmStr = XmStringCreateLocalized (name);
				}

				d_list->AddListItem (xmStr, 0, nodePt);

				//  If we are regenerating the list because
				//  of an update, and this is the node that
				//  was previously selected, make it so again.
				if (state == 2 && nodePt == selectedNode)
				{
					d_list->SelectItem (d_listNum, xmStr);
				}
			}

			d_list->NextList ();
			d_listNum++;
			d_list->DisplayLists ();
			setEditButtonSensitivities ();
			break;
		}
	}

	//  Save callback_p for checking at destruction time.
	d_loadedList = callback_p;

	if ((state == 1 || state == 0) && d_topWidget)
	{
		window = XtWindow (d_list->GetTopWidget());

		if (window)
		{
			setCursor (C_POINTER, window, C_FLUSH);
		}
	}

}	//  End  treeBrowse::loadList ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	treeBrowse::loadListCallback (void *list, treeBrowse *thisPtr,
//								int state,
//							callback_t *callback_p)
//  DESCRIPTION:
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

void
treeBrowse::loadListCallback (void *list, treeBrowse *thisPtr, int state,
							callback_t *callback_p)
{
	thisPtr->loadList (list, state, callback_p);

}	//  End  treeBrowse::loadListCallback ()


/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	Widget	createEditButtons (Widget parent, XtPointer win)
//
//  DESCRIPTION:
//	Create the "Add" and "Delete" edit buttons for the list (browser).
//
//  RETURN:
//	Return the widget id of the form containing the buttons.
//
///////////////////////////////////////////////////////////////////////////// */

Widget
treeBrowse::createEditButtons (Widget parent, XtPointer win)
{
	Widget		buttonForm;
	void		*node = setupListTree (((SetupWin *)win)->objList);
	Boolean		isAddable, isDeletable;


	if (d_node)
	{
		isAddable   = nodeIsAddable (d_node)?     True : False;
		isDeletable = nodeIsDeleteable (d_node)?  True : False;
	}

	buttonForm = XtVaCreateWidget ("action_area",
                                xmFormWidgetClass,              parent,
                                NULL);

	d_addButton = XtVaCreateManagedWidget (getStr (TXT_browserAdd),
				xmPushButtonWidgetClass,	buttonForm,
				XmNsensitive,			isAddable,
				XmNtopAttachment,		XmATTACH_FORM,
				XmNleftAttachment,		XmATTACH_FORM,
				XmNrightAttachment,		XmATTACH_FORM,
				XmNuserData,			this,
				NULL);

	XtAddCallback (d_addButton, XmNactivateCallback,
						(XtCallbackProc)addCB1, this);

	d_remButton = XtVaCreateManagedWidget (getStr (TXT_browserDelete),
				xmPushButtonWidgetClass,	buttonForm,
				XmNsensitive,			isDeletable,
				XmNtopAttachment,		XmATTACH_WIDGET,
				XmNtopWidget,			d_addButton,
				XmNleftAttachment,		XmATTACH_FORM,
				XmNrightAttachment,		XmATTACH_FORM,
				XmNbottomAttachment,		XmATTACH_FORM,
				XmNuserData,			this,
				NULL);

	XtAddCallback (d_remButton, XmNactivateCallback,
						(XtCallbackProc)remCB1, this);

	XtManageChild (buttonForm);
	return (buttonForm);

}	// End  treeBrowse::createEditButtons ()



void
treeBrowse::addCB1 (Widget w, treeBrowse *object,
					    XmToggleButtonCallbackStruct *cbs)
{
	object->addCB(w, object->d_win, cbs);
}



void
treeBrowse::addCB (Widget w, XtPointer win, XmToggleButtonCallbackStruct *cbs)
{
	treeBrowse	*thisObj;
	treeList_t	*treeList;

	log5 (C_FUNC, "addCB(w, win=", win, "cbs=", cbs, ")");

	XtVaGetValues (w, XmNuserData, &thisObj, NULL);

	if (!d_node)
		return;

	setCursor (C_WAIT, ((SetupWin *)d_win)->window, C_FLUSH);

	if (treeList = nodeTreeList (d_node))
	{
		treeListAdd (treeList);		//  has no return value
	}

	setEditButtonSensitivities ();
	setCursor (C_POINTER, ((SetupWin *)d_win)->window, C_FLUSH);

}	//  End  addCB ()


void
treeBrowse::remCB1 (Widget w, treeBrowse *object,
					    XmToggleButtonCallbackStruct *cbs)
{
	object->remCB (w, object->d_win, cbs);
}

void
treeBrowse::remCB (Widget w, XtPointer win, XmToggleButtonCallbackStruct *cbs)
{
	treeBrowse	*thisObj;
	node_t		*tmpNode;

	log5 (C_FUNC, "remCB(w, win=", win, "cbs=", cbs, ")");

	XtVaGetValues (w, XmNuserData, &thisObj, NULL);

	setCursor (C_WAIT, ((SetupWin *)d_win)->window, C_FLUSH);

	tmpNode = d_node;
	d_node = d_nodeList[d_nodeIndex-1];

	if (nodeDelete (tmpNode))
	{
		//  ERROR!!!!  Do what?
	}

	setEditButtonSensitivities ();
	setCursor (C_POINTER, ((SetupWin *)d_win)->window, C_FLUSH);

}	//  End  treeBrowse::remCB ()

void
treeBrowse::setEditButtonSensitivities ()
{

	if (d_addButton)
	{
		XtVaSetValues (d_addButton, XmNsensitive,
					nodeIsAddable (d_node)? True : False,
					NULL);
	}

	if (d_remButton)
	{
		XtVaSetValues (d_remButton, XmNsensitive,
					nodeIsDeleteable (d_node)? True : False,
					NULL);
	}

}	//  End  setEditButtonSensitivities ()
