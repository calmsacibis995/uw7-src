#pragma ident	"@(#)m1.2libs:Mrm/Mrmmodule.c	1.1"
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
 *	These routines get and put data entries from a record into a buffer.
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
 *	UrmIFMInitModule		- Init module in a context
 *
 *	UrmIFMSetTopmost		- Set a topmost widget in module
 *
 *	UrmIFMPutModule			- Write module to IDB file
 *
 *	UrmIFMHGetModule		- Get module from hierarchy
 *
 *	UrmIFMGetModule			- Get module from IDB file
 *
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



/*
 *
 *  MACROS
 *
 */

/*
 * Macro to validate a resource context and bind interface module pointer
 *
 * Formal parameters:
 *	context		variable bound to context pointer
 *	routine		quoted string giving routine name
 *	recptr		variable to be bound to module pointer
 *
 * WARNING: Don't use expressions where variables are expected!
 *
 */
#define	_validate_context(context,routine,recptr) {			\
    if ( ! UrmRCValid(context) )					\
        return Urm__UT_Error					\
            (routine, "Invalid resource context",			\
            NULL, context, MrmBAD_CONTEXT) ;				\
    recptr = (RGMModuleDescPtr) UrmRCBuffer (context) ; }



Cardinal UrmIFMInitModule 
#ifndef _NO_PROTO
    (URMResourceContextPtr	context_id,
    MrmCount			num_widget,
    MrmCode			access,
    MrmCode			lock)
#else
(context_id, num_widget, access, lock)
    URMResourceContextPtr	context_id ;
    MrmCount			num_widget ;
    MrmCode			access ;
    MrmCode			lock ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmIFMInitModule initializes a context for creating a new
 *	interface module. It stores the access and lock attributes, and
 *	sizes and initializes the topmost widget descriptors for the number
 *	of widgets specified.
 *
 *	This routine sets all the context parameters, including group,
 *	type, and size.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	URM resource context in which to build the
 *			interface module
 *	num_widget	number of topmost widgets to allocate in module
 *	access		The module's access attribute, URMaPublic or
 *			URMaPrivate
 *	lock		The module's locking attribute, TRUE to lock the
 *			module
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmBAD_CONTEXT	invalid resource context
 *	MrmTOO_MANY	number of indexes exceeds internal limit
 *	MrmFAILURE	unexplained failure
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
Cardinal		result ;	/* Function results */
RGMModuleDescPtr	ifmodptr ;	/* IF module in context */
MrmCount		descsiz ;	/* # bytes for descriptor */
int			ndx ;		/* loop index */


/*
 * validate context and bind pointer. Size check.
 */
_validate_context (context_id, "UrmIFMInitModule", ifmodptr) ;
if ( num_widget >= RGMListSizeMax )
    return Urm__UT_Error
        ("UrmIFMInitModule", "Too many widgets",
	 NULL, context_id, MrmTOO_MANY) ;

/*
 * compute descriptor size and guarantee space. Null all the widgets.
 */
descsiz = sizeof(RGMModuleDesc) + (num_widget-1)*sizeof(RGMTopmostDesc) ;
descsiz = _FULLWORD (descsiz) ;

result = UrmResizeResourceContext (context_id, descsiz) ;
if ( result != MrmSUCCESS ) return result ;

ifmodptr = (RGMModuleDescPtr) UrmRCBuffer(context_id) ;
ifmodptr->validation = URMInterfaceModuleValid ;
ifmodptr->count = num_widget ;
ifmodptr->annex1 = NULL ;
ifmodptr->annex2 = NULL ;

for ( ndx=0 ; ndx<num_widget ; ndx++ )
    strcpy (ifmodptr->topmost[ndx].index, "") ;

/*
 * Set context parameters
 */
UrmRCSetSize (context_id, descsiz) ;
UrmRCSetGroup (context_id, URMgResourceSet) ;
UrmRCSetType (context_id, URMrsInterfaceModule) ;
UrmRCSetAccess (context_id, access) ;
UrmRCSetLock (context_id, lock) ;

/*
 * Successfully created
 */
return MrmSUCCESS ;

}



Cardinal UrmIFMSetTopmost (context_id, topmost_ndx, index)
    URMResourceContextPtr	context_id ;
    Cardinal			topmost_ndx ;
    String			index ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine sets the topmost widget index for one of the topmost
 *	widgets specified by an interface module.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context holding a valid interface module
 *	topmost_ndx	0-based index of the widget in the module
 *	index		the widget's index (name)
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmBAD_IF_MODULE	invalid interface module
 *	MrmOUT_OF_BOUNDS	topmost_ndx out of bounds
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
RGMModuleDescPtr	ifmodptr ;	/* IF module in context */


/*
 * Validate context and interface module. Make sure the index is in bounds
 */
_validate_context (context_id, "UrmIFMSetTopmost", ifmodptr) ;
if ( ifmodptr->validation != URMInterfaceModuleValid )
    return Urm__UT_Error
        ("UrmIFMSetTopmost", "Invalid interface module",
        NULL, context_id, MrmBAD_IF_MODULE) ;

if ( topmost_ndx >= ifmodptr->count )
    return Urm__UT_Error
        ("UrmIFMSetTopmost", "Topmost index out of bounds",
        NULL, context_id, MrmOUT_OF_BOUNDS) ;

/*
 * Copy in index, and return
 */
strncat (ifmodptr->topmost[topmost_ndx].index, index, URMMaxIndexLen) ;

return MrmSUCCESS ;

}



Cardinal UrmIFMPutModule (file_id, index, context_id)
    IDBFile			file_id ;
    String			index ;
    URMResourceContextPtr	context_id ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmIFMPutModule creates a resource set entry of type interface
 *	module in the given IDB file. The access and locking attriutes
 *	are taken from the context.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		IDB file into which to write the interface module
 *	index		index (name) under which to enter the module
 *	context_id	URM resource context containing the module
 *			representation. This must be a valid interface
 *			module.
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *	MrmSUCCESS	Operation succeeded
 *	MrmBAD_CONTEXT	invalid resource context
 *	MrmBAD_IF_MODULE	invalid interface module in context
 *	Others		see UrmIdbPutIndexedResource
 *
 *  FUNCTION VALUE:
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
Cardinal		result ;	/* Function results */
RGMModuleDescPtr	ifmodptr ;	/* IF module in context */


/*
 * Validate context and interface module.
 */
_validate_context (context_id, "UrmIFMPutModule", ifmodptr) ;
if ( ifmodptr->validation != URMInterfaceModuleValid )
    return Urm__UT_Error
        ("UrmIFMPutModule", "Invalid interface module",
        NULL, context_id, MrmBAD_IF_MODULE) ;

/*
 * Make sure the context is properly set, then put the module in the file
 */
UrmRCSetGroup (context_id, URMgResourceSet) ;
UrmRCSetType (context_id, URMrsInterfaceModule) ;

result = UrmIdbPutIndexedResource (file_id, index, context_id) ;
return result ;

}



Cardinal UrmIFMHGetModule (hierarchy_id, index, context_id, file_id_return)
    MrmHierarchy		hierarchy_id ;
    String			index ;
    URMResourceContextPtr	context_id ;
    IDBFile			*file_id_return ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmIFMHGetModule searches the database hierarchy for the module
 *	given its index. It returns the module descriptor in the resource
 *	context.
 *
 *  FORMAL PARAMETERS:
 *
 *	hierarchy_id	id of an open URM database hierarchy
 *	index		index (name) of the desired interface module
 *	context_id	resource context into which to read the module
 *	file_id_return	to return IDB file in which the module is found
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	module found and loaded in context
 *	MrmNOT_FOUND	module not found in hierarchy
 *	MrmBAD_HIERARCHY	hierarchy not valid
 *	Others		See UrmGetModule
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
 * Get the module
 */
result = UrmHGetIndexedResource
    (hierarchy_id, index, URMgResourceSet, URMrsInterfaceModule,
    context_id, file_id_return) ;

return result ;

}



Cardinal UrmIFMGetModule (file_id, index, context_id)
    IDBFile			file_id ;
    String			index ;
    URMResourceContextPtr	context_id ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmIFMGetModule retrieves an interface module from an IDB file, and
 *	reads it into the given resource context.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		open IDB file from which to retrieve module
 *	index		index (name) of the desired interface module
 *	context_id	resource context into which to read the module
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	DMBadContext	invalid resource context
 *	MrmBAD_IF_MODULE	invalid interface module found in file
 *	Others		see UrmIdbGetIndexedResource
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
RGMModuleDescPtr	ifmodptr ;	/* IF module in context */


/*
 * Validate context, then attempt the read
 */
if ( ! UrmRCValid(context_id) )
    return Urm__UT_Error
        ("UrmIFMGetModule", "Invalid resource context",
        file_id, context_id, MrmBAD_CONTEXT) ;

result = UrmIdbGetIndexedResource (file_id, index, URMgResourceSet,
    URMrsInterfaceModule, context_id) ;
if ( result != MrmSUCCESS ) return result ;

/*
 * validate the interface module
 */
ifmodptr = (RGMModuleDescPtr) UrmRCBuffer(context_id) ;
if ( ifmodptr->validation != URMInterfaceModuleValid )
    return Urm__UT_Error
        ("UrmIFMPutModule", "Invalid interface module",
        NULL, context_id, MrmBAD_IF_MODULE) ;

/*
 * Successfully retrieved
 */
return MrmSUCCESS ;

}

