#pragma ident	"@(#)m1.2libs:Mrm/Mrmwwrite.c	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif

/*
*  (c) Copyright 1989, 1990, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */

/*
 *++
 *  FACILITY:
 *
 *      UIL Resource Manager (URM):
 *
 *  ABSTRACT:
 *
 *	This module contains all the routines which write a widget
 *	from a resource context into an IDB file.
 *
 *--
 */


/*
 *
 *  INCLUDE FILES
 *
 */


#include <Mrm/MrmAppl.h>
#include <Mrm/Mrm.h>


/*
 *
 *  TABLE OF CONTENTS
 *
 *	UrmPutIndexedWidget		Write indexed widget to IDB file
 *
 *	UrmPutRIDWidget			Write RID widget to IDB file
 *
 */


/*
 *
 *  DEFINE and MACRO DEFINITIONS
 *
 */


/*
 *
 *  EXTERNAL VARIABLE DECLARATIONS
 *
 */


/*
 *
 *  GLOBAL VARIABLE DECLARATIONS
 *
 */


/*
 *
 *  OWN VARIABLE DECLARATIONS
 *
 */




Cardinal UrmPutIndexedWidget (file_id, index, context_id)
    IDBFile			file_id ;
    String			index ;
    URMResourceContextPtr	context_id ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmPutIndexedWidget puts a URMgWidget resource record in the
 *	database. Its contents are the widget record. The resource type is
 *	the value of the widget record's type field. The access is the
 *	value of the record's access field; the locking attribute is the
 *	value of the record's lock field.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		file into which to write record
 *	index		case-sensitive index for the widget
 *	context_id	resource context containing widget record
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmBAD_CONTEXT	invalid resource context
 *	MrmBAD_WIDGET_REC	invalid widget record in context
 *	Others:		See UrmIdbPutIndexedResource
 *
 *  SIDE EFFECTS:
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */
Cardinal		result ;	/* function results */
RGMWidgetRecordPtr	widgetrec ;	/* Widget record in context */


/*
 * Validate record, then set resource group and type, and enter in
 * IDB file.
 */
if ( ! UrmRCValid(context_id) )
    return Urm__UT_Error
        ("UrmPutIndexedWidget", "Invalid resource context",
        file_id, NULL, MrmBAD_CONTEXT) ;
widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer (context_id) ;
if ( ! UrmWRValid(widgetrec) )
    return Urm__UT_Error
        ("UrmPutIndexedWidget", "Invalid widget record",
        file_id, context_id, MrmBAD_WIDGET_REC) ;

UrmRCSetSize (context_id, widgetrec->size) ;
UrmRCSetGroup (context_id, URMgWidget) ;
UrmRCSetType (context_id, widgetrec->type) ;
UrmRCSetAccess (context_id, widgetrec->access) ;
UrmRCSetLock (context_id, widgetrec->lock) ;

result = UrmIdbPutIndexedResource (file_id, index, context_id) ;
return result ;

}



Cardinal UrmPutRIDWidget (file_id, resource_id, context_id)
    IDBFile			file_id ;
    MrmResource_id		resource_id ;
    URMResourceContextPtr	context_id ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmPutRIDWidget puts a widget accessed by a resource id into the
 *	given database file. The resource type is the value of the widget
 *	record's type field. As usual for resources accessed by resource id,
 *	the access is always private; this routine resets the record's access
 *	field if necessary to be consistent with private access. The value
 *	of the locking attribute is the value of the record's lock field.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		file into which to write record
 *	resource_id	resource id for the record
 *	context_id	resource context containing widget record
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmBAD_CONTEXT	invalid resource context
 *	MrmBAD_WIDGET_REC	invalid widget record in context
 *	Others:		See UrmIdbPutRIDResource
 *
 *  SIDE EFFECTS:
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */
Cardinal		result ;	/* function results */
RGMWidgetRecordPtr	widgetrec ;	/* Widget record in context */


/*
 * Validate record, then set resource group and type, and enter in
 * IDB file.
 */
if ( ! UrmRCValid(context_id) )
    return Urm__UT_Error
        ("UrmPutRIDWidget", "Invalid resource context",
        file_id, NULL, MrmBAD_CONTEXT) ;
widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer (context_id) ;
if ( ! UrmWRValid(widgetrec) )
    return Urm__UT_Error
        ("UrmPutRIDWidget", "Invalid widget record",
        file_id, context_id, MrmBAD_WIDGET_REC) ;

UrmRCSetSize (context_id, widgetrec->size) ;
UrmRCSetGroup (context_id, URMgWidget) ;
UrmRCSetType (context_id, widgetrec->type) ;
UrmRCSetAccess (context_id, URMaPrivate) ;
UrmRCSetLock (context_id, widgetrec->lock) ;

result = UrmIdbPutRIDResource (file_id, resource_id, context_id) ;
return result ;

}

