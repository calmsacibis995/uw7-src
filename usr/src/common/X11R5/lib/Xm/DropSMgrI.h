#pragma ident	"@(#)m1.2libs:Xm/DropSMgrI.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#ifndef _XmDropSMgrI_h
#define _XmDropSMgrI_h

#include <Xm/XmP.h>

#ifdef __cplusplus
extern "C" {
#endif

/* This is used for maintenance of the Pre-Register drop site tree */

#define XmDROP_SITE_LEFT_EDGE	(1<<0)
#define XmDROP_SITE_RIGHT_EDGE	(1<<1)
#define XmDROP_SITE_TOP_EDGE	(1<<2)
#define XmDROP_SITE_BOTTOM_EDGE	(1<<3)

#define XmDSM_DS_LEAF		(1<<0)
#define XmDSM_DS_INTERNAL	(1<<1)
#define XmDSM_DS_HAS_REGION	(1<<2)

#define XmDSM_T_CLOSE (1<<0)

/*
 * Notice that the top of the record is the samy layout as
 * an XRectangle.  This is important when it is passed to
 * IntersectWithAncestors by DetectAndInsertAllClippers.
 */
typedef struct _XmDSClipRect {
	Position	x;
	Position	y;
	Dimension	width;
	Dimension	height;
	unsigned char	detected;
} XmDSClipRect;

#define CHILDREN_INCREMENT 10

typedef struct _DSTableRec {
    unsigned int 	mask;		/* size of hash table - 1 */
    unsigned int 	rehash;		/* mask - 2 */
    unsigned int 	occupied;	/* number of occupied entries */
    unsigned int 	fakes;		/* number occupied by DSfake */
    XtPointer		*entries;	/* the entries */
} DSTableRec, *DSTable;


typedef struct _XmDSStatusRec {
	unsigned int	remote:1;
	unsigned int	leaf:1;
	unsigned int	shell:1;
	unsigned int	type:1;
	unsigned int	animation_style:3;
	unsigned int	internal:1;
	unsigned int	has_region:1;
	unsigned int	activity:1;
	unsigned int	registered:1;
} XmDSStatusRec, * XmDSStatus;

typedef struct _XmDSFullInfoRec {
	XmDSStatusRec	status;

	XtPointer		parent;
	unsigned short	import_targets_ID;
	unsigned char	operations;
	XmRegion		region;
    XtCallbackProc	drag_proc;
	XtCallbackProc	drop_proc;

    Widget 				widget;

    unsigned short	num_children;
    unsigned short	max_children;
    XtPointer		*children;

	/* Support for subresource magic; needed ONLY in Full structure */
	unsigned char	type;
	unsigned char	animation_style;
	unsigned char	activity;
	Atom			*import_targets;
	Cardinal		num_import_targets;
	XRectangle		*rectangles;
	Cardinal		num_rectangles;

	/* A complete laundry list of animation fields */
	Pixmap		animation_pixmap;
	Cardinal	animation_pixmap_depth;
	Pixmap		animation_mask;
	Pixel		background;
	Pixel		foreground;
	Pixel		highlight_color;
	Pixmap		highlight_pixmap;
	Dimension	highlight_thickness;
	Pixel		top_shadow_color;
	Pixmap		top_shadow_pixmap;
	Pixel		bottom_shadow_color;
	Pixmap		bottom_shadow_pixmap;
	Dimension	shadow_thickness;
	Dimension	border_width;

} XmDSFullInfoRec, * XmDSFullInfo;

/* One gazillion typedefs to allow for dataspace efficiency */

typedef struct _XmDSLocalLeafRec {
	XtPointer		parent;
	unsigned short	import_targets_ID;
	unsigned char	operations;
	XmRegion		region;
    XtCallbackProc	drag_proc;
	XtCallbackProc	drop_proc;
    Widget 			widget;
} XmDSLocalLeafRec, * XmDSLocalLeaf;

typedef struct _XmDSLocalNodeRec {
	XtPointer		parent;
	unsigned short	import_targets_ID;
	unsigned char	operations;
	XmRegion		region;
    unsigned short	num_children;
    unsigned short	max_children;
    XtPointer		*children;
    XtCallbackProc	drag_proc;
	XtCallbackProc	drop_proc;
    Widget 			widget;
} XmDSLocalNodeRec, * XmDSLocalNode;

typedef struct _XmDSLocalPixmapStyleRec {
	Pixmap		animation_pixmap;
	Cardinal	animation_pixmap_depth;
	Pixmap		animation_mask;
} XmDSLocalPixmapStyleRec, *XmDSLocalPixmapStyle;

typedef struct _XmDSRemoteLeafRec {
	XtPointer		parent;
	unsigned short	import_targets_ID;
	unsigned char	operations;
	XmRegion		region;
} XmDSRemoteLeafRec, * XmDSRemoteLeaf;

typedef struct _XmDSRemoteNodeRec {
	XtPointer		parent;
	unsigned short	import_targets_ID;
	unsigned char	operations;
	XmRegion		region;
    unsigned short	num_children;
    unsigned short	max_children;
    XtPointer		*children;
} XmDSRemoteNodeRec, * XmDSRemoteNode;


/* These style records are only used for remote trees */
typedef struct _XmDSRemoteNoneStyleRec {
	Dimension	border_width;
} XmDSRemoteNoneStyleRec, * XmDSRemoteNoneStyle;

typedef struct _XmDSRemoteHighlightStyleRec {
	Pixel		highlight_color;
	Pixmap		highlight_pixmap;
	Pixel		background; /* in case of highlight pixmaps */
	Dimension	highlight_thickness;
	Dimension	border_width;
} XmDSRemoteHighlightStyleRec, * XmDSRemoteHighlightStyle;

typedef struct _XmDSRemoteShadowStyleRec {
	Pixel		top_shadow_color;
	Pixmap		top_shadow_pixmap;
	Pixel		bottom_shadow_color;
	Pixmap		bottom_shadow_pixmap;
	Pixel		foreground; /* in case of shadow pixmaps */
	Dimension	shadow_thickness;
	Dimension	highlight_thickness;
	Dimension	border_width;
} XmDSRemoteShadowStyleRec, * XmDSRemoteShadowStyle;

typedef struct _XmDSRemotePixmapStyleRec {
	Pixmap		animation_pixmap;
	Cardinal	animation_pixmap_depth;
	Pixmap		animation_mask;
	Pixel		background;
	Pixel		foreground;
	Dimension	shadow_thickness;
	Dimension	highlight_thickness;
	Dimension	border_width;
} XmDSRemotePixmapStyleRec, * XmDSRemotePixmapStyle;


/* Now we permute the preceding types */

typedef struct _XmDSLocalNoneLeafRec {
	XmDSStatusRec			status;
	XmDSLocalLeafRec		info;
} XmDSLocalNoneLeafRec, * XmDSLocalNoneLeaf;

typedef struct _XmDSLocalNoneNodeRec {
	XmDSStatusRec			status;
	XmDSLocalNodeRec		info;
} XmDSLocalNoneNodeRec, * XmDSLocalNoneNode;

typedef struct _XmDSLocalHighlightLeafRec {
	XmDSStatusRec			status;
	XmDSLocalLeafRec		info;
} XmDSLocalHighlightLeafRec, * XmDSLocalHighlightLeaf;

typedef struct _XmDSLocalHighlightNodeRec {
	XmDSStatusRec			status;
	XmDSLocalNodeRec		info;
} XmDSLocalHighlightNodeRec, * XmDSLocalHighlightNode;

typedef struct _XmDSLocalShadowLeafRec {
	XmDSStatusRec		status;
	XmDSLocalLeafRec	info;
} XmDSLocalShadowLeafRec, * XmDSLocalShadowLeaf;

typedef struct _XmDSLocalShadowNodeRec {
	XmDSStatusRec		status;
	XmDSLocalNodeRec	info;
} XmDSLocalShadowNodeRec, * XmDSLocalShadowNode;

typedef struct _XmDSLocalPixmapLeafRec {
	XmDSStatusRec		status;
	XmDSLocalLeafRec	info;
	XmDSLocalPixmapStyleRec	animation_data;
} XmDSLocalPixmapLeafRec, * XmDSLocalPixmapLeaf;

typedef struct _XmDSLocalPixmapNodeRec {
	XmDSStatusRec		status;
	XmDSLocalNodeRec	info;
	XmDSLocalPixmapStyleRec	animation_data;
} XmDSLocalPixmapNodeRec, * XmDSLocalPixmapNode;


typedef struct _XmDSRemoteNoneLeafRec {
	XmDSStatusRec			status;
	XmDSRemoteLeafRec		info;
	XmDSRemoteNoneStyleRec	animation_data;
} XmDSRemoteNoneLeafRec, * XmDSRemoteNoneLeaf;

typedef struct _XmDSRemoteNoneNodeRec {
	XmDSStatusRec			status;
	XmDSRemoteNodeRec		info;
	XmDSRemoteNoneStyleRec	animation_data;
} XmDSRemoteNoneNodeRec, * XmDSRemoteNoneNode;

typedef struct _XmDSRemoteHighlightLeafRec {
	XmDSStatusRec			status;
	XmDSRemoteLeafRec		info;
	XmDSRemoteHighlightStyleRec	animation_data;
} XmDSRemoteHighlightLeafRec, * XmDSRemoteHighlightLeaf;

typedef struct _XmDSRemoteHighlightNodeRec {
	XmDSStatusRec			status;
	XmDSRemoteNodeRec		info;
	XmDSRemoteHighlightStyleRec	animation_data;
} XmDSRemoteHighlightNodeRec, * XmDSRemoteHighlightNode;

typedef struct _XmDSRemoteShadowLeafRec {
	XmDSStatusRec		status;
	XmDSRemoteLeafRec	info;
	XmDSRemoteShadowStyleRec	animation_data;
} XmDSRemoteShadowLeafRec, * XmDSRemoteShadowLeaf;

typedef struct _XmDSRemoteShadowNodeRec {
	XmDSStatusRec		status;
	XmDSRemoteNodeRec	info;
	XmDSRemoteShadowStyleRec	animation_data;
} XmDSRemoteShadowNodeRec, * XmDSRemoteShadowNode;

typedef struct _XmDSRemotePixmapLeafRec {
	XmDSStatusRec		status;
	XmDSRemoteLeafRec	info;
	XmDSRemotePixmapStyleRec	animation_data;
} XmDSRemotePixmapLeafRec, * XmDSRemotePixmapLeaf;

typedef struct _XmDSRemotePixmapNodeRec {
	XmDSStatusRec		status;
	XmDSRemoteNodeRec	info;
	XmDSRemotePixmapStyleRec	animation_data;
} XmDSRemotePixmapNodeRec, * XmDSRemotePixmapNode;

typedef union _XmDSInfoRec {
	XmDSStatusRec				status;

	XmDSLocalNoneLeafRec		local_none_leaf;
	XmDSLocalNoneNodeRec		local_none_node;
	XmDSLocalHighlightLeafRec	local_highlight_leaf;
	XmDSLocalHighlightNodeRec	local_highlight_node;
	XmDSLocalShadowLeafRec		local_shadow_leaf;
	XmDSLocalShadowNodeRec		local_shadow_node;
	XmDSLocalPixmapLeafRec		local_pixmap_leaf;
	XmDSLocalPixmapNodeRec		local_pixmap_node;

	XmDSRemoteNoneLeafRec		remote_none_leaf;
	XmDSRemoteNoneNodeRec		remote_none_node;
	XmDSRemoteHighlightLeafRec	remote_highlight_leaf;
	XmDSRemoteHighlightNodeRec	remote_highlight_node;
	XmDSRemoteShadowLeafRec		remote_shadow_leaf;
	XmDSRemoteShadowNodeRec		remote_shadow_node;
	XmDSRemotePixmapLeafRec		remote_pixmap_leaf;
	XmDSRemotePixmapNodeRec		remote_pixmap_node;
} XmDSInfoRec, * XmDSInfo;


/* A few macros to deal with the typedefs */

#define GetDSRemote(ds)			(((XmDSStatus)(ds))->remote)
#define GetDSLeaf(ds)			(((XmDSStatus)(ds))->leaf)
#define GetDSShell(ds)			(((XmDSStatus)(ds))->shell)
#define GetDSAnimationStyle(ds)	(((XmDSStatus)(ds))->animation_style)
#define GetDSType(ds) \
	((((XmDSStatus)(ds))->type) ? \
		XmDROP_SITE_COMPOSITE \
	: \
		XmDROP_SITE_SIMPLE)

#define GetDSRegistered(ds)		(((XmDSStatus)(ds))->registered)
#define GetDSInternal(ds)		(((XmDSStatus)(ds))->internal)
#define GetDSHasRegion(ds)		(((XmDSStatus)(ds))->has_region)
#define GetDSActivity(ds) \
	((((XmDSStatus)(ds))->activity) ? \
		XmDROP_SITE_ACTIVE \
	: \
		XmDROP_SITE_INACTIVE)

#define GetDSImportTargetsID(ds) \
	(((XmDSLocalNoneLeaf)(ds))->info.import_targets_ID)
#define GetDSOperations(ds) \
	(((XmDSLocalNoneLeaf)(ds))->info.operations)
#define GetDSRegion(ds) (((XmDSLocalNoneLeaf)(ds))->info.region)

#define GetDSParent(ds) \
	((GetDSShell(ds)) ? \
		NULL \
	: \
		(((XmDSLocalNoneLeaf)(ds))->info.parent))

#define GetDSUpdateLevel(ds) \
	((GetDSShell(ds)) ? \
		((int)(((XmDSLocalNoneNode)(ds))->info.parent)) \
	: \
		-1)

#define GetDSDragProc(ds) \
	((GetDSRemote(ds)) ? \
		NULL \
	: \
		((GetDSType(ds)) ? \
			(((XmDSLocalNoneNode)(ds))->info.drag_proc) \
		: \
			(((XmDSLocalNoneLeaf)(ds))->info.drag_proc)))

#define GetDSDropProc(ds) \
	((GetDSRemote(ds)) ? \
		NULL \
	: \
		((GetDSType(ds)) ? \
			(((XmDSLocalNoneNode)(ds))->info.drop_proc) \
		: \
			(((XmDSLocalNoneLeaf)(ds))->info.drop_proc)))

#define GetDSWidget(ds) \
	((GetDSRemote(ds)) ? \
		NULL \
	: \
		((GetDSType(ds)) ? \
			(((XmDSLocalNoneNode)(ds))->info.widget) \
		: \
			(((XmDSLocalNoneLeaf)(ds))->info.widget)))

#define GetDSNumChildren(ds) \
	((GetDSType(ds)) ? \
		(((XmDSLocalNoneNode)(ds))->info.num_children) \
	: \
		0)

#define GetDSMaxChildren(ds) \
	((GetDSType(ds)) ? \
		(((XmDSLocalNoneNode)(ds))->info.max_children) \
	: \
		0)

#define GetDSChildren(ds) \
	((GetDSType(ds)) ? \
		(((XmDSLocalNoneNode)(ds))->info.children) \
	: \
		NULL)

#define GetDSChild(ds, position) \
	((GetDSType(ds)) ? \
		(((XmDSLocalNoneNode)(ds))->info.children[position]) \
	: \
		NULL)

#define GetDSLocalAnimationPart(ds) \
	((GetDSType(ds)) ? \
		((GetDSAnimationStyle(ds) == XmDRAG_UNDER_PIXMAP) ? \
			&(((XmDSLocalPixmapNode)(ds))->animation_data) \
		: \
			NULL) \
	: \
		((GetDSAnimationStyle(ds) == XmDRAG_UNDER_PIXMAP) ? \
			&(((XmDSLocalPixmapLeaf)(ds))->animation_data) \
		: \
			NULL)) \

#define GetDSRemoteAnimationPart(ds) \
	((GetDSType(ds)) ? \
		&(((XmDSRemoteNoneNode)(ds))->animation_data) \
	: \
		&(((XmDSRemoteNoneLeaf)(ds))->animation_data))


#define SetDSRemote(ds, newRemote) \
	(((XmDSStatus)(ds))->remote) = (unsigned int)(newRemote)
#define SetDSLeaf(ds, newLeaf) \
	(((XmDSStatus)(ds))->leaf) = (unsigned int)(newLeaf)
#define SetDSShell(ds, newShell) \
	(((XmDSStatus)(ds))->shell) = (unsigned int)(newShell)
#define SetDSAnimationStyle(ds, newAnimationStyle) \
	(((XmDSStatus)(ds))->animation_style) = \
		(unsigned int)(newAnimationStyle)
#define SetDSType(ds, newType) \
	(((XmDSStatus)(ds))->type) = \
		( ((newType) == XmDROP_SITE_COMPOSITE) ? \
			1 \
		: \
			0)

#define SetDSRegistered(ds,newRegistered) \
	(((XmDSStatus)(ds))->registered) = \
		(unsigned int)(newRegistered)
#define SetDSInternal(ds, newInternal) \
	(((XmDSStatus)(ds))->internal) = (unsigned int)(newInternal)
#define SetDSHasRegion(ds, newHasRegion) \
	(((XmDSStatus)(ds))->has_region) = (unsigned int)(newHasRegion)
#define SetDSActivity(ds, newActivity) \
	(((XmDSStatus)(ds))->activity) = \
		( ((newActivity) == XmDROP_SITE_ACTIVE) ? \
			1 \
		: \
			0)

#define SetDSImportTargetsID(ds, newImportTargetsID) \
	(((XmDSLocalNoneLeaf)(ds))->info.import_targets_ID) = \
		(unsigned short)(newImportTargetsID)
#define SetDSOperations(ds, newOperations) \
	(((XmDSLocalNoneLeaf)(ds))->info.operations) = \
		(unsigned char)(newOperations)
#define SetDSRegion(ds, newRegion) \
	(((XmDSLocalNoneLeaf)(ds))->info.region) = \
		(XmRegion)(newRegion)
#define SetDSParent(ds, newParent) \
	((GetDSShell(ds)) ? \
		NULL \
	: \
		((((XmDSLocalNoneLeaf)(ds))->info.parent) = \
			(XtPointer)(newParent)))

#define SetDSUpdateLevel(ds, newUpdateLevel) \
	((GetDSShell(ds)) ? \
		((((XmDSLocalNoneNode)(ds))->info.parent) = \
			(XtPointer)(newUpdateLevel)) \
	: \
		0)

#define SetDSDragProc(ds, newDragProc) \
	((GetDSRemote(ds)) ? \
		NULL \
	: \
		((GetDSType(ds)) ? \
			((((XmDSLocalNoneNode)(ds))->info.drag_proc) = \
				(XtCallbackProc)(newDragProc)) \
		: \
			((((XmDSLocalNoneLeaf)(ds))->info.drag_proc) = \
				(XtCallbackProc)(newDragProc))))

#define SetDSDropProc(ds, newDropProc) \
	((GetDSRemote(ds)) ? \
		NULL \
	: \
		((GetDSType(ds)) ? \
			((((XmDSLocalNoneNode)(ds))->info.drop_proc) = \
				(XtCallbackProc)(newDropProc)) \
		: \
			((((XmDSLocalNoneLeaf)(ds))->info.drop_proc) = \
				(XtCallbackProc)(newDropProc))))

#define SetDSWidget(ds, newWidget) \
	((GetDSRemote(ds)) ? \
		NULL \
	: \
		((GetDSType(ds)) ? \
			(((((XmDSLocalNoneNode)(ds))->info.widget) = \
				(Widget)(newWidget))) \
		: \
			(((((XmDSLocalNoneLeaf)(ds))->info.widget) = \
				(Widget)(newWidget)))))

#define SetDSNumChildren(ds, newNumChildren) \
	((GetDSType(ds)) ? \
		((((XmDSLocalNoneNode)(ds))->info.num_children) = \
			(unsigned short)(newNumChildren)) \
	: \
		0)

#define SetDSMaxChildren(ds, newMaxChildren) \
	((GetDSType(ds)) ? \
		((((XmDSLocalNoneNode)(ds))->info.max_children) = \
			(unsigned short)(newMaxChildren)) \
	: \
		0)

#define SetDSChildren(ds, newChildren) \
	((GetDSType(ds)) ? \
		((((XmDSLocalNoneNode)(ds))->info.children) = \
			(XtPointer *)(newChildren)) \
	: \
		NULL)

/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmDSIAddChild() ;
extern void _XmDSIRemoveChild() ;
extern Cardinal _XmDSIGetChildPosition() ;
extern void _XmDSIReplaceChild() ;
extern void _XmDSISwapChildren() ;
extern void _XmDSIDestroy() ;
extern Dimension _XmDSIGetBorderWidth() ;

#else

extern void _XmDSIAddChild( 
                        XmDSInfo parentInfo,
                        XmDSInfo childInfo,
                        Cardinal childPosition) ;
extern void _XmDSIRemoveChild( 
                        XmDSInfo parentInfo,
                        XmDSInfo childInfo) ;
extern Cardinal _XmDSIGetChildPosition( 
                        XmDSInfo parentInfo,
                        XmDSInfo childInfo) ;
extern void _XmDSIReplaceChild( 
                        XmDSInfo oldChildInfo,
                        XmDSInfo newChildInfo) ;
extern void _XmDSISwapChildren( 
                        XmDSInfo parentInfo,
                        Cardinal position1,
                        Cardinal position2) ;
extern void _XmDSIDestroy( 
                        XmDSInfo info,
#if NeedWidePrototypes
                        int substructures) ;
#else
                        Boolean substructures) ;
#endif /* NeedWidePrototypes */
extern Dimension _XmDSIGetBorderWidth( 
                        XmDSInfo info) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

#define AddDSChild _XmDSIAddChild
#define RemoveDSChild _XmDSIRemoveChild
#define SwapDSChildren _XmDSISwapChildren
#define ReplaceDSChild _XmDSIReplaceChild
#define GetDSChildPosition _XmDSIGetChildPosition
#define DestroyDS _XmDSIDestroy
#define GetDSBorderWidth _XmDSIGetBorderWidth

externalref XtResource _XmDSResources[];
externalref Cardinal _XmNumDSResources;

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDropSMgrI_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
