#pragma ident	"@(#)m1.2libs:Xm/DropSMgrI.c	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.1
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include <Xm/DropSMgrP.h>
#include "DropSMgrI.h"
#include <stdio.h>
#include "MessagesI.h"

#define MESSAGE1 _XmMsgDropSMgrI_0001
#define MESSAGE2 _XmMsgDropSMgrI_0002
#define MESSAGE3 _XmMsgDropSMgrI_0003

#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


 /********    Static Function Declarations    ********/
#ifdef _NO_PROTO


#else


#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

externaldef(xmdsresources)
XtResource _XmDSResources[] = {
	{   XmNdropSiteType, XmCDropSiteType, XmRDropSiteType,
		sizeof(unsigned char),
		XtOffsetOf( struct _XmDSFullInfoRec, type),
		XmRImmediate, (XtPointer) XmDROP_SITE_SIMPLE
	},
	{   XmNdropSiteActivity, XmCDropSiteActivity, XmRDropSiteActivity,
		sizeof(unsigned char),
		XtOffsetOf( struct _XmDSFullInfoRec, activity),
		XmRImmediate, (XtPointer) XmDROP_SITE_ACTIVE
	},
	{   XmNimportTargets, XmCImportTargets, XmRAtomList,
		sizeof(Atom *),
		XtOffsetOf( struct _XmDSFullInfoRec, import_targets),
		XmRImmediate, (XtPointer) NULL
	},
	{   XmNnumImportTargets, XmCNumImportTargets, XmRCardinal,
		sizeof(Cardinal),
		XtOffsetOf( struct _XmDSFullInfoRec, num_import_targets),
		XmRImmediate, (XtPointer) 0
	},
	{   XmNdropSiteOperations, XmCDropSiteOperations,
		XmRDropSiteOperations, sizeof(unsigned char),
		XtOffsetOf( struct _XmDSFullInfoRec, operations), XmRImmediate,
		(XtPointer) (XmDROP_MOVE | XmDROP_COPY),
	},
	{   XmNdropRectangles, XmCDropRectangles, XmRRectangleList,
		sizeof(XRectangle *),
		XtOffsetOf( struct _XmDSFullInfoRec, rectangles),
		XmRImmediate, (XtPointer) NULL
	},
	{   XmNnumDropRectangles, XmCNumDropRectangles, XmRCardinal,
		sizeof(Cardinal),
		XtOffsetOf( struct _XmDSFullInfoRec, num_rectangles),
		XmRImmediate, (XtPointer) 1
	},
	{   XmNdragProc, XmCDragProc, XmRProc,
		sizeof(XtPointer),
		XtOffsetOf( struct _XmDSFullInfoRec, drag_proc),
		XmRImmediate, (XtPointer) NULL
	},
	{   XmNdropProc, XmCDropProc, XmRProc,
		sizeof(XtPointer),
		XtOffsetOf( struct _XmDSFullInfoRec, drop_proc),
		XmRImmediate, (XtPointer) NULL
	},
	{   XmNanimationStyle, XmCAnimationStyle, XmRAnimationStyle,
		sizeof(unsigned char),
		XtOffsetOf( struct _XmDSFullInfoRec, animation_style),
		XmRImmediate, (XtPointer) XmDRAG_UNDER_HIGHLIGHT
	},
	{   XmNanimationPixmap, XmCAnimationPixmap, XmRAnimationPixmap,
		sizeof(Pixmap),
		XtOffsetOf( struct _XmDSFullInfoRec, animation_pixmap),
		XmRImmediate, (XtPointer) XmUNSPECIFIED_PIXMAP
	},
	{   XmNanimationMask, XmCAnimationMask, XmRAnimationMask,
		sizeof(Pixmap),
		XtOffsetOf( struct _XmDSFullInfoRec, animation_mask),
		XmRImmediate, (XtPointer) XmUNSPECIFIED_PIXMAP
	},
	{   XmNanimationPixmapDepth, XmCAnimationPixmapDepth, XmRCardinal,
		sizeof(int),
		XtOffsetOf( struct _XmDSFullInfoRec, animation_pixmap_depth),
		XmRImmediate, (XtPointer) 0
	},
};

externaldef(xmnumdsresources)
Cardinal _XmNumDSResources = XtNumber(_XmDSResources);

void 
#ifdef _NO_PROTO
_XmDSIAddChild( parentInfo, childInfo, childPosition )
        XmDSInfo parentInfo ;
        XmDSInfo childInfo ;
        Cardinal childPosition ;
#else
_XmDSIAddChild(
        XmDSInfo parentInfo,
        XmDSInfo childInfo,
        Cardinal childPosition )
#endif /* _NO_PROTO */
{
	unsigned short i;
	unsigned short num_children;

	if ((parentInfo == NULL) || (childInfo == NULL))
		return;

	num_children = GetDSNumChildren(parentInfo);

	if (GetDSType(parentInfo) != XmDROP_SITE_COMPOSITE)
	{

#ifdef I18N_MSG
		_XmWarning(GetDSWidget(childInfo),
			   catgets(Xm_catd,MS_DropS,MSG_DRS_10,
			   MESSAGE1));
#else
		_XmWarning(GetDSWidget(childInfo), MESSAGE1 );
#endif

	}

	if (childPosition > num_children)
	{

#ifdef I18N_MSG
		_XmWarning(GetDSWidget(parentInfo),
			   catgets(Xm_catd,MS_DropS,MSG_DRS_11, MESSAGE2));
#else
		_XmWarning(GetDSWidget(parentInfo), MESSAGE2);
#endif

		childPosition = num_children;
	}

	if (num_children == GetDSMaxChildren(parentInfo))
	{
		SetDSMaxChildren(parentInfo, num_children + CHILDREN_INCREMENT);
		SetDSChildren(parentInfo, (XtPointer *) XtRealloc(
				(char *) GetDSChildren(parentInfo),
				sizeof(XmDSInfo) * GetDSMaxChildren(parentInfo)));
	}

	for (i = num_children; i > childPosition; i--)
		GetDSChildren(parentInfo)[i] = GetDSChildren(parentInfo)[i-1];
	
	GetDSChildren(parentInfo)[childPosition] = (XtPointer) childInfo;
	SetDSNumChildren(parentInfo, (num_children + 1));
	SetDSParent(childInfo, (XtPointer) parentInfo);

	SetDSLeaf(parentInfo, False);
}

void 
#ifdef _NO_PROTO
_XmDSIRemoveChild( parentInfo, childInfo )
        XmDSInfo parentInfo ;
        XmDSInfo childInfo ;
#else
_XmDSIRemoveChild(
        XmDSInfo parentInfo,
        XmDSInfo childInfo )
#endif /* _NO_PROTO */
{
	int i;
	unsigned short num_children;
	Cardinal position;

	if ((parentInfo == NULL) || (childInfo == NULL))
		return;

	num_children = GetDSNumChildren(parentInfo);

	/* Find the child to be Removed */
	position = _XmDSIGetChildPosition(parentInfo, childInfo);
	
	/*
	 * Take it out of the list by writing over its location and
	 * reducing the child count.
	 */
	for (i = position + 1; i < num_children; i++)
		GetDSChildren(parentInfo)[i - 1] = GetDSChildren(parentInfo)[i];
	
	SetDSNumChildren(parentInfo, --num_children);

	if (!num_children)
		SetDSLeaf(parentInfo, True);
}


Cardinal 
#ifdef _NO_PROTO
_XmDSIGetChildPosition( parentInfo, childInfo )
        XmDSInfo parentInfo ;
        XmDSInfo childInfo ;
#else
_XmDSIGetChildPosition(
        XmDSInfo parentInfo,
        XmDSInfo childInfo )
#endif /* _NO_PROTO */
{
	int i;
	unsigned short num_children;

	if ((parentInfo == NULL) || (childInfo == NULL))
		return(0);

	num_children = GetDSNumChildren(parentInfo);

	if (GetDSParent(childInfo) != (XtPointer) parentInfo)
	{
		char buf[256];

#ifdef I18N_MSG
		sprintf(buf, catgets(Xm_catd,MS_DropS,MSG_DRS_12,
			MESSAGE3),
			XrmQuarkToString(GetDSWidget(childInfo)->core.xrm_name),
			XrmQuarkToString(GetDSWidget(parentInfo)->core.xrm_name));
#else
		sprintf(buf, MESSAGE3,
			XrmQuarkToString(GetDSWidget(childInfo)->core.xrm_name),
			XrmQuarkToString(GetDSWidget(parentInfo)->core.xrm_name));
#endif

		_XmWarning(GetDSWidget(parentInfo), buf);
		return(num_children);
	}

	for (i = 0; i < num_children; i++)
		if (GetDSChildren(parentInfo)[i] == (XtPointer) childInfo)
			break;

	if (i == num_children)
	{
		char buf[256];

#ifdef I18N_MSG
		sprintf(buf, catgets(Xm_catd,MS_DropS,MSG_DRS_12,
			MESSAGE3),
			XrmQuarkToString(GetDSWidget(childInfo)->core.xrm_name),
			XrmQuarkToString(GetDSWidget(parentInfo)->core.xrm_name));
#else
		sprintf(buf, MESSAGE3,
			XrmQuarkToString(GetDSWidget(childInfo)->core.xrm_name),
			XrmQuarkToString(GetDSWidget(parentInfo)->core.xrm_name));
#endif

		_XmWarning(GetDSWidget(parentInfo), buf);
	}
	
	return(i);
}

void 
#ifdef _NO_PROTO
_XmDSIReplaceChild(oldChildInfo, newChildInfo )
        XmDSInfo oldChildInfo ;
        XmDSInfo newChildInfo ;
#else
_XmDSIReplaceChild(
        XmDSInfo oldChildInfo,
        XmDSInfo newChildInfo )
#endif
{
	int i;
	unsigned short num_children;
	XmDSInfo parentInfo;

	if ((oldChildInfo == NULL) ||
		(newChildInfo == NULL))
		return;
	
	if ((parentInfo = (XmDSInfo) GetDSParent(oldChildInfo)) == NULL)
		return;

	num_children = GetDSNumChildren(parentInfo);

	for (i=0; i < num_children; i++)
	{
		if (GetDSChildren(parentInfo)[i] == (XtPointer) oldChildInfo)
			GetDSChildren(parentInfo)[i] = (XtPointer) newChildInfo;
	}

	SetDSParent(oldChildInfo, NULL);

	if ((GetDSParent(newChildInfo)) &&
		(GetDSParent(newChildInfo) != (XtPointer) parentInfo))
		_XmDSIRemoveChild(parentInfo, newChildInfo);
	else
		SetDSParent(newChildInfo, parentInfo);
}


void 
#ifdef _NO_PROTO
_XmDSISwapChildren( parentInfo, position1, position2 )
        XmDSInfo parentInfo ;
		Cardinal position1 ;
		Cardinal position2 ;
#else
_XmDSISwapChildren(
        XmDSInfo parentInfo,
		Cardinal position1,
		Cardinal position2 )
#endif
{
	XmDSInfo tmp_info;
	unsigned short num_children;

	if (parentInfo == NULL)
		return;

	num_children = GetDSNumChildren(parentInfo);

	if ((position1 > num_children) || (position2 > num_children))
		return;

	tmp_info = (XmDSInfo) GetDSChildren(parentInfo)[position1];

	GetDSChildren(parentInfo)[position1] =
		GetDSChildren(parentInfo)[position2];
	GetDSChildren(parentInfo)[position2] = (XtPointer) tmp_info;
}

void 
#ifdef _NO_PROTO
_XmDSIDestroy( info, substructures )
        XmDSInfo info ;
		Boolean substructures ;
#else
_XmDSIDestroy(
        XmDSInfo info,
#if NeedWidePrototypes
                        int substructures )
#else
                        Boolean substructures )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
	if (info != NULL)
	{

		if ((GetDSType(info) == XmDROP_SITE_COMPOSITE) &&
			(GetDSChildren(info) != NULL) &&
			(substructures))
			XtFree( (char *) GetDSChildren(info));

		if (GetDSRegion(info) && (substructures))
			_XmRegionDestroy(GetDSRegion(info));

		XtFree( (char *) info);
	}
}

Dimension
#ifdef _NO_PROTO
_XmDSIGetBorderWidth(info)
        XmDSInfo info ;
#else
_XmDSIGetBorderWidth(
        XmDSInfo info)
#endif /* _NO_PROTO */
{
	if (info == NULL)
		return(0);

	if (GetDSRemote(info))
	{
		switch (GetDSAnimationStyle(info))
		{
			case XmDRAG_UNDER_NONE:
			{
				XmDSRemoteNoneStyleRec *sr =
					(XmDSRemoteNoneStyleRec *)
						GetDSRemoteAnimationPart(info);
				
				return(sr->border_width);
			}
			case XmDRAG_UNDER_HIGHLIGHT:
			{
				XmDSRemoteHighlightStyleRec *sr =
					(XmDSRemoteHighlightStyleRec *)
						GetDSRemoteAnimationPart(info);
				
				return(sr->border_width);
			}
			case XmDRAG_UNDER_SHADOW_IN:
			case XmDRAG_UNDER_SHADOW_OUT:
			{
				XmDSRemoteShadowStyleRec *sr =
					(XmDSRemoteShadowStyleRec *)
						GetDSRemoteAnimationPart(info);
				
				return(sr->border_width);
			}
			case XmDRAG_UNDER_PIXMAP:
			{
				XmDSRemotePixmapStyleRec *sr =
					(XmDSRemotePixmapStyleRec *)
						GetDSRemoteAnimationPart(info);
				
				return(sr->border_width);
			}
			default:
				/* Shouldn't be here */
				return 0;
				;
			break;
		}
	}
	else
	{
		Widget w = GetDSWidget(info);

		return(XtBorderWidth(w));
	}
}

