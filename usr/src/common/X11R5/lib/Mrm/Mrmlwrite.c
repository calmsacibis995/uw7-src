#pragma ident	"@(#)m1.2libs:Mrm/Mrmlwrite.c	1.1"
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
 *	This module contains all the routines which write a Literal
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
 *	UrmPutIndexedLiteral		Write indexed literal to IDB file
 *
 *	UrmPutRIDLiteral		Write RID literal to IDB file
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




Cardinal UrmPutIndexedLiteral (file_id, index, context_id)
    IDBFile			file_id ;
    String			index ;
    URMResourceContextPtr	context_id ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmPutIndexedLiteral puts a URMgLiteral resource record in the
 *	database. Its content is the literal. The resource type, access,
 *	and locking attributes are assumed to be already set.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		file into which to write record
 *	index		case-sensitive index for the literal
 *	context_id	resource context containing literal
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmBAD_CONTEXT	invalid resource context
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
Cardinal	result;	/* return status */

/*
 * Validate record, then set resource group and enter in the
 * IDB file.
 */
if ( ! UrmRCValid(context_id) )
    return Urm__UT_Error
        ("UrmPutIndexedLiteral", "Invalid resource context",
        file_id, context_id, MrmBAD_CONTEXT) ;

UrmRCSetGroup (context_id, URMgLiteral) ;
/*
 * The size, type, access, and lock fields should have already be
 * set.
 *
 *UrmRCSetSize (context_id, ) ;
 *UrmRCSetType (context_id, ) ;
 *UrmRCSetAccess (context_id, ) ;
 *UrmRCSetLock (context_id, ) ;
 */
result = UrmIdbPutIndexedResource (file_id, index, context_id) ;
return result ;

}



Cardinal UrmPutRIDLiteral (file_id, resource_id, context_id)
    IDBFile			file_id ;
    MrmResource_id		resource_id ;
    URMResourceContextPtr	context_id ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmPutRIDLiteral puts a literal accessed by a resource id into the
 *	database. Its content is the literal. The resource type, access,
 *	and locking attributes are assumed to be already set.
 *
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		file into which to write record
 *	resource_id	resource id for the record
 *	context_id	resource context containing literal 
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmBAD_CONTEXT	invalid resource context
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


/*
 * Validate record, then set resource group and enter in the
 * IDB file.
 */
if ( ! UrmRCValid(context_id) )
    return Urm__UT_Error
        ("UrmPutRIDLiteral", "Invalid resource context",
        file_id, context_id, MrmBAD_CONTEXT) ;

UrmRCSetGroup (context_id, URMgLiteral) ;
UrmRCSetAccess (context_id, URMaPrivate) ;

/*
 * The size, type, and lock fields are assumed to be set.
 *
 *UrmRCSetSize (context_id, ) ;
 *UrmRCSetType (context_id, ) ;
 *UrmRCSetLock (context_id, ) ;
 */

result = UrmIdbPutRIDResource (file_id, resource_id, context_id) ;
return result ;

}

