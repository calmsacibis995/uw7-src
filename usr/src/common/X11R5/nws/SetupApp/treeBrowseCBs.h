#ident	"@(#)treeBrowseCBs.h	1.2"
#ifndef TREE_BROWSECBS_H
#define	TREE_BROWSECBS_H

/*
//  This file contains the definitions for the callbacks associated
//  with the treeBrowse object.
*/

#include <mail/setupTypes.h> 


void	treeNodeSelectedCB (setupObject_t *curObj, void *d_node);
void	treeNodeUnSelectedCB (setupObject_t *curObj, void *d_node);
void	treeNodeOpenedCB (setupObject_t *curObj, void *d_node);

#endif	// TREE_BROWSECBS_H
