#ident	"@(#)treeBrowseCBs.C	1.2"
/*  treeBrowseCBs.C
//
//  This file the callbacks associated with events we need
//  to capture for treeBrowse object.
*/


#include	<iostream.h>		//  for cout()
#include	<Xm/Text.h>
#include	"controlArea.h"
#include	"setupWin.h"		//  for VarEntry
#include	"treeBrowseCBs.h"	//  for tree browser callbacks



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	treeNodeSelectedCB (setupObject_t *curObj, void *d_node)
//
//  DESCRIPTION:
//	This is the callback that gets executed when a tree (browser) node
//	gets selected.
//
//  RETURN:
//	Nothing.
//
////////////////////////////////////////////////////////////////////////////// */

void
treeNodeSelectedCB (setupObject_t *curObj, void *node)
{
	VarEntry	*cData = (VarEntry *)setupObjectClientData (curObj);
	char		*description;


	log4 (C_FUNC, "treeNodeSelectedCB(obj=", curObj, ", node=",
							nodeName (node));
	if (nodeSelected(node))
	{
	    cData->win->treeBrowse->setSelectedNode (node);

	    if (cData && cData->win && cData->win->descArea)
	    {
		//  Should have separate win struct for popup browser!
	    	if (description = nodeDescription (node))
			XmTextSetString (cData->win->descArea, description);
	    }
	}
	else		//  Node is *not* selected.
	{
	    cData->win->treeBrowse->setSelectedNode ((void *)0);
	}

	cData->win->treeBrowse->setEditButtonSensitivities ();

}	//  End  treeNodeSelectedCB ()



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	treeNodeUnSelectedCB (setupObject_t *curObj, void *d_node)
//
//  DESCRIPTION:
//	This is the callback that gets executed when a tree (browser) node
//	gets unselected.
//
//  RETURN:
//	Nothing.
//
////////////////////////////////////////////////////////////////////////////// */

void
treeNodeUnSelectedCB (setupObject_t *curObj, void *node)
{
	VarEntry	*cData = (VarEntry *)setupObjectClientData (curObj);

	cData->win->treeBrowse->setEditButtonSensitivities ();

}	//  End  treeNodeUnSelectedCB ()



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	treeNodeOpenedCB (setupObject_t *curObj, void *d_node)
//
//  DESCRIPTION:
//	This is the callback that gets executed when a tree (browser) node
//	gets "opened" (double-clicked)..
//
//  RETURN:
//	Nothing.
//
////////////////////////////////////////////////////////////////////////////// */

void
treeNodeOpenedCB (setupObject_t *curObj, void *node)
{
	VarEntry	*cData = (VarEntry *)setupObjectClientData (curObj);
	char		*description;


	log4 (C_FUNC, "treeNodeOpenedCB(obj=", curObj, ", node=",
							nodeName(node));
	if (cData && cData->win && cData->win->descArea)
	{
	    if (description = nodeDescription (node))
		XmTextSetString (cData->win->descArea, description);
	}

	cData->win->treeBrowse->setEditButtonSensitivities ();

}	//  End  treeNodeOpenedCB ()
