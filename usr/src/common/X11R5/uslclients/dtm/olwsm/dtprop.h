#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/dtprop.h	1.25"
#endif

#ifndef	_dtprop_h_
#define	_dtprop_h_

/******************************file*header********************************

    Description:
	This file contains the resource name definitions and their
	default values for Desktop properties.

	These are shared between dtm.c (where the resources are gotten
	from initially) and the property sheet.
*/
/* Define resource names:
	Use #define's since resource spec in property sheets is
	"*resourceName".  This can be achieved by making use of
	pre-processor string concatenation:
		"*" XtNresourceName.
*/
#define XtNfolderCols		"folderCols"
#define XtNfolderRows		"folderRows"
#define XtNgridWidth		"gridWidth"
#define XtNgridHeight		"gridHeight"
#define XtNlaunchFromCWD	"launchFromCWD"
#define XtNopenInPlace		"openInPlace"
#define XtNshowFullPaths	"showFullPaths"
#define XtNsyncRemoteFolders	"syncRemoteFolders"
#define XtNsyncInterval		"syncInterval"
#define XtNtreeDepth		"treeDepth"
	
/* Define default values */
#define DEFAULT_FOLDER_COLS		5
#define DEFAULT_FOLDER_ROWS		2
#define DEFAULT_GRID_HEIGHT		65
#define DEFAULT_GRID_WIDTH		100
#define DEFAULT_LAUNCH_FROM_CWD		False
#define DEFAULT_OPEN_IN_PLACE		True
#define DEFAULT_SHOW_PATHS		False
#define DEFAULT_SYNC_INTERVAL		2000	/* in millisecs */
#define DEFAULT_TREE_DEPTH		2

#endif	/* _dtprop_h_ */
