#ident	"@(#)treeBrowse.h	1.2"
#ifndef TREE_BROWSE_H
#define	TREE_BROWSE_H

/*
//  This file contains the definition of the TreeBrowse object.
*/


#include <X11/Intrinsic.h>

#include	<Xm/Xm.h>		//  ... always needed for Motif
#include	<Xm/Form.h>
#include	<Xm/Label.h>
#include	<Xm/PushB.h>
#include	<Xm/ToggleB.h>

#include <mail/hosts.h> 
#include <mail/tree.h> 
#include <mail/setupTypes.h> 

#include	"setup_txt.h"		//  the message catalog database

typedef void	(*OpenCallback) (XtPointer clientData, void *node);
typedef void	(*SelectCallback) (XtPointer clientData, void *node);
typedef void	(*DblClickCallback) (XtPointer clientData, void *node);
typedef void	(*UnselectCallback) (XtPointer clientData);


class MultiPList;


class treeBrowse
{
    public:		// Constructors and Destructors

	treeBrowse (XtAppContext appContext, Widget parent,
			setupObject_t *listObject, XtPointer win,
			Boolean editButtons = False, int numColumns = 3,
			int visibleItemCount = 6);
	~treeBrowse ();

    private:		// Private Data

	int		 d_listNum;
	MultiPList*	 d_list;
	void		*d_node;		//  Currently selected node
	Widget		 d_topWidget;		//  Highest level Motif widget
	void		*d_nodeList[256];	//  List of selected nodes
	int		 d_nodeIndex;		//  Index into d_nodeList
	Widget		 d_addButton;		//  Widget id of add button
	Widget		 d_remButton;		//  Widget id of delete button
	SelectCallback	 d_dblClickCallback;
	OpenCallback	 d_openCallback;
	SelectCallback	 d_selectCallback;
	UnselectCallback d_unselectCallback;
	XtPointer	 d_openClientData;
	XtPointer	 d_selectClientData;
	XtPointer	 d_unselectClientData;
	XtPointer	 d_win;
	callback_t	*d_openedList;
	callback_t	*d_loadedList;
	Boolean		 d_canClose;

    private:		// Private Methods

	Widget	createEditButtons (Widget parent, XtPointer win);
	static void	addCB1 (Widget w, treeBrowse *,
					XmToggleButtonCallbackStruct *cbs);
	void	addCB (Widget w, XtPointer win,
					XmToggleButtonCallbackStruct *cbs);
	static void	remCB1 (Widget w, treeBrowse *,
					XmToggleButtonCallbackStruct *cbs);
	void	remCB (Widget w, XtPointer win,
					XmToggleButtonCallbackStruct *cbs);
	void	open (char *item, int listNum, void *data);
	void	select (char *item, int listNum, void *data);
	void	clear (int listNum, XtPointer callback_p);
	static void	clearCallback (XtPointer mlp, int listNum, XtPointer callback_p);
	static void	dblClickCallback (XtPointer mlp, char *item,
					int listNum, XtPointer data);
	static void	openCallback (XtPointer mlp, char *item, int listNum,
					XtPointer data);
	static void	selectCallback (XtPointer mlp, char *item, int listNum,
					XtPointer data);
	static void	unselectCallback (XtPointer mlp, int listNum);
	static Dimension calcWidestListItem (XmFontList fontList, setupObject_t *listObj);
	void		loadList (void* list, int state, callback_t *callback_p);
	static void	loadListCallback (void *list, treeBrowse *ptr, int state, callback_t *callback_p);

    public:		// Public Interface Methods

	void		unselect (int listNum);
	inline int	isLeaf ();
	const char	*getPathName ();
	const char	*getLeafName ();
	Widget		getTopWidget ();	//  Retrieve the top widget id
	void		setEditButtonSensitivities ();
	void		*getSelectedNode ();	//  Retrieve the selected node
	void		setSelectedNode (void *node);	//  Set selected node
	void		setDblClickCallback (DblClickCallback callback,
							XtPointer data = 0);
	void		setSelectCallback (SelectCallback callback,
							XtPointer data = 0);
	void		setUnselectCallback (UnselectCallback callback,
							XtPointer data = 0);
	int		checkAddr (char* systemName);
};

/*----------------------------------------------------------------------------
 *	Is the currently selected node a leaf.
 */
inline int
treeBrowse::isLeaf ()
{
	return (!nodeIsInternal (d_node));
}

#endif	// TREE_BROWSE_H
