#pragma ident	"@(#)m1.2libs:Xm/RepTypeI.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmRepTypeI_h
#define _XmRepTypeI_h

#include <Xm/RepType.h>
#include "XmI.h"

#ifdef __cplusplus
extern "C" {
#endif


#define XmREP_TYPE_STD_BIT	0x2000
#define XmREP_TYPE_RT_BIT	0x4000
#define XmREP_TYPE_MAP_BIT	0x8000

#define XmREP_TYPE_TAG_MASK	0xE000
#define XmREP_TYPE_OFFSET_MASK	0x1FFF

#define XmREP_TYPE_STD_MAP_TAG	(XmREP_TYPE_STD_BIT | XmREP_TYPE_MAP_BIT)
#define XmREP_TYPE_STD_TAG	XmREP_TYPE_STD_BIT
#define XmREP_TYPE_RT_MAP_TAG	(XmREP_TYPE_RT_BIT | XmREP_TYPE_MAP_BIT)
#define XmREP_TYPE_RT_TAG	XmREP_TYPE_RT_BIT

#define	XmREP_TYPE_STD_MAP( id) \
		((id & XmREP_TYPE_TAG_MASK) == XmREP_TYPE_STD_MAP_TAG)
#define	XmREP_TYPE_STD( id)	\
		((id & XmREP_TYPE_TAG_MASK) == XmREP_TYPE_STD_TAG)
#define XmREP_TYPE_RT_MAP( id)	\
		((id & XmREP_TYPE_TAG_MASK) == XmREP_TYPE_RT_MAP_TAG)
#define XmREP_TYPE_RT( id)	\
		((id & XmREP_TYPE_TAG_MASK) == XmREP_TYPE_RT_TAG)

#define XmREP_TYPE_MAPPED( id)	(id & XmREP_TYPE_MAP_BIT)
#define XmREP_TYPE_OFFSET( id)	(id & XmREP_TYPE_OFFSET_MASK)
#define XmREP_TYPE_TAG( id)	(id & XmREP_TYPE_TAG_MASK)


/* The XmRepTypeRec is also used as an XtConvertArgRec for the rep. type
 *   converter.  The first and second fields are crucial, the first being
 *   used by Xt to "copy" the following fields into the first argument
 *   of the argument list used by the converter.  The "id" field is extracted
 *   from this argument under the assumption that it is the first element
 *   of the structure that is pointed to by the addr field of the argument
 *   (see the ConvertRepType routine).
 */

typedef struct
{   
    XtAddressMode mode_for_converter ;	/* This must match XtConvertArgRec.*/
    XmRepTypeId id ;			/* Do not move this field. */
    unsigned int num_values:7 ; 
    unsigned int reverse_installed:1 ;
    unsigned char conversion_buffer ;
    String rep_type_name ;
    String *value_names ;
    }XmRepTypeRec, *XmRepType ;

typedef struct
{   
    XtAddressMode mode_for_converter ;	/* Must match XmRepTypeRec. */
    XmRepTypeId id ;			/* Must match XmRepTypeRec. */
    unsigned int num_values:7 ;		/* Must match XmRepTypeRec. */
    unsigned int reverse_installed:1 ;	/* Must match XmRepTypeRec. */
    unsigned char conversion_buffer ; 	/* Must match XmRepTypeRec. */
    String rep_type_name ;		/* Must match XmRepTypeRec. */
    String *value_names ;		/* Must match XmRepTypeRec. */
    unsigned char *map ;
    }XmRepTypeMappedRec ;

typedef struct
{
    XmRepType list ;	     /* All structures must have same initial fields.*/
    unsigned short num_records ;
    unsigned short rec_size ;
    XmRepTypeId tag ;
    }XmRepTypeListDataRec, *XmRepTypeListData ;


/* The following enumerations of representation type identification
 *   numbers have a one-to-one positional mapping to the corresponding
 *   representation type record in the static rep type lists.
 *   For Motif version 1.2, the two static lists are in alphabetical
 *   order, as is required by the current coding of the XmRepTypeGetId
 *   routine.
 * As long as these XmRID* values are accessed by applications only
 *   through the exported API, this alphabetical ordering can be
 *   maintained as new rep type resources are added in future
 *   versions of Motif.  However, if these values are exported
 *   directly (by distributing this header file to binary licencees),
 *   then binary compatibility will require that the current XmRID* 
 *   values remain unchanged.  Addition of new representation type
 *   resources in a post-Motif 1.2 environment would thus require a
 *   re-coding of the XmRepTypeGetId routine to remove its dependency
 *   on the pre-sorted list.
 */

enum {
	XmRID_ALIGNMENT = XmREP_TYPE_STD_TAG,	XmRID_ANIMATION_STYLE,
	XmRID_ARROW_DIRECTION,			XmRID_ATTACHMENT,
	XmRID_AUDIBLE_WARNING,			XmRID_BLEND_MODEL,
	XmRID_CHILD_HORIZONTAL_ALIGNMENT,	XmRID_CHILD_PLACEMENT,
	XmRID_CHILD_TYPE,			XmRID_CHILD_VERTICAL_ALIGNMENT,
	XmRID_COMMAND_WINDOW_LOCATION,		XmRID_DIALOG_TYPE,
	XmRID_DRAG_INITIATOR_PROTOCOL_STYLE,
	XmRID_DRAG_RECEIVER_PROTOCOL_STYLE,
	XmRID_DROP_SITE_ACTIVITY,		XmRID_DROP_SITE_TYPE,
	XmRID_EDIT_MODE,			XmRID_ICON_ATTACHMENT,
	XmRID_LIST_SIZE_POLICY,			XmRID_MULTI_CLICK,
	XmRID_NAVIGATION_TYPE,			XmRID_PROCESSING_DIRECTION,
	XmRID_RESIZE_POLICY,			XmRID_ROW_COLUMN_TYPE,
	XmRID_SCROLL_BAR_DISPLAY_POLICY,	XmRID_SCROLL_BAR_PLACEMENT,
	XmRID_SCROLLING_POLICY,			XmRID_SELECTION_POLICY,
	XmRID_SELECTION_TYPE,			XmRID_SEPARATOR_TYPE,
	XmRID_STRING_DIRECTION,			XmRID_TEAR_OFF_MODEL,
	XmRID_UNIT_TYPE,			XmRID_UNPOST_BEHAVIOR,
	XmRID_VERTICAL_ALIGNMENT,		XmRID_VISUAL_POLICY,

/* Add consecutive-valued rep type resources above this line.
* _______________________________________________________________
* 
* Add non-consecutive (mapped) rep type resources below this line.
*/
	XmRID_DEFAULT_BUTTON_TYPE = XmREP_TYPE_STD_MAP_TAG,
	XmRID_DIALOG_STYLE,
	XmRID_FILE_TYPE_MASK,			XmRID_INDICATOR_TYPE,
	XmRID_LABEL_TYPE,			XmRID_ORIENTATION,
	XmRID_PACKING,				XmRID_SHADOW_TYPE,
	XmRID_WHICH_BUTTON
	} ;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmRepTypeInstallConverters() ;

#else

extern void _XmRepTypeInstallConverters( void ) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/



#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmRepTypeI_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
