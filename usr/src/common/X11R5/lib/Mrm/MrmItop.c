#pragma ident	"@(#)m1.2libs:Mrm/MrmItop.c	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.2
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
 *      UIL Resource Manager (URM): IDB facility
 *
 *  ABSTRACT:
 *
 *      This module contains the publicly accessible top-level
 *	IDB read & common (to read and write operations) routines.
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
#include <Mrm/IDB.h>


/*
 *
 *  TABLE OF CONTENTS
 *
 *	UrmIdbOpenFileRead		Open a file for read access
 *
 *	UrmIdbCloseFile			Close an open file
 *
 *	UrmIdbGetIndexedResource		Get indexed resource from file
 *
 *	UrmIdbFindIndexedResource	Find indexed resource(s) in file
 *
 *	UrmIdbGetRIDResource		Get resource id'ed resource from file
 *
 *	UrmIdbGetResourceId		Get next resource id in file
 *
 *	Idb__FIL_Valid			Validate file id
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



Cardinal UrmIdbOpenFileRead 

#ifndef _NO_PROTO
    (String			name,
    MrmOsOpenParamPtr		os_ext,
    IDBFile			*file_id_return,
    char			*fname_return)
#else
(name, os_ext, file_id_return, fname_return)
    String			name ;
    MrmOsOpenParamPtr		os_ext ;
    IDBFile			*file_id_return ;
    char			*fname_return ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	IdbOpenFile opens the requested IDB database file, and returns a
 *	handle or id which is used in all subsequent IDB operations.
 *	IdbOpenFile initializes the file's index structures and sets up
 *	any buffers it needs. The database file is opened for read access only.
 *
 *  FORMAL PARAMETERS:
 *
 *	name		A system-dependent string specifying the IDB file
 *			to be opened.
 *	os_ext		An operating-system specific structure which
 *			supports using specific file system features
 *	file_id_return	returns the IDB file id used in all other IDB routines
 *	fname_return	returns the name of the file actually opened
 *			(via strcpy). Should be at least 256 chars.
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmNOT_FOUND	existing file not found
 *	MrmFAILURE	operation failed, no further reason
 *
 *  SIDE EFFECTS:
 *
 *      1. Acquires memory for the file descriptor.
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
unsigned		fileid ;	/* low-level id for file */
IDBFile			filedesc ;	/* new file descriptor */
int			ndx ;		/* loop index */


/*
 * Attempt to open the file.
 */
result = Idb__FU_OpenFile
    (name, URMReadAccess, os_ext, (IDBLowLevelFilePtr *)&fileid, 
     fname_return) ;
if ( result != MrmSUCCESS ) return result ;

/*
 * File successfully opened. Acquire a file descriptor and initialize it.
 */
filedesc = (IDBFile) XtMalloc (sizeof(IDBOpenFile)) ;
if ( filedesc == NULL )
    {
    UrmIdbCloseFile ((IDBFile)fileid, FALSE);
    return MrmFAILURE ;
    }

filedesc->validation = IDBOpenFileValid ;
filedesc->access = URMReadAccess ;
filedesc->lowlevel_id = fileid ;
filedesc->last_record = 0 ;
filedesc->last_data_record = 0 ;
filedesc->get_count = 0 ;
filedesc->put_count = 0 ;
filedesc->class_ctable = NULL;
filedesc->resource_ctable = NULL;
for ( ndx=IDBrtMin ; ndx<=IDBrtMax ; ndx++ )
    filedesc->rt_counts[ndx] = 0 ;

/*
 * Read the file header record info into the file descriptor
 */
result = Idb__HDR_GetHeader (filedesc) ;
if ( result != MrmSUCCESS )
    {
    UrmIdbCloseFile (filedesc, TRUE) ;
    return result ;
    }

/*
 * File successfully opened
 */
*file_id_return = filedesc ;
return MrmSUCCESS ;

}



Cardinal UrmIdbCloseFile 

#ifndef _NO_PROTO
    (IDBFile			file_id,
    Boolean			keep_new_file)
#else
(file_id, keep_new_file)
    IDBFile			file_id ;
    Boolean			keep_new_file ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	If the file was opened for read access, then UrmIdbCloseFile simply
 *	closes it. If it was opened for write acccess, then UrmIdbCloseFile
 *	uses keep_new_file to determine its behavior. If keep_new_file is
 *	true, then the file's index structures are updated, the file is
 *	closed, and it becomes the newest version of the database file.
 *	If keep_new_file is false, then the working copy of the file is closed
 *	and deleted, retaining the previous version of the file as the latest
 *	version. In all cases, UrmIdbCloseFile frees all memory structures
 *	associated with the file.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id			The IDB file id returned by IdbOpenFile
 *	keep_new_file		true if database is to be permanently updated;
 *				false to quit without changing the database
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmFAILURE	operation failed
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
Cardinal	result ;		/* function results */

/*
 * Local macros
 */
#define	_error_close()	\
	{ Idb__FU_CloseFile((IDBLowLevelFile*)file_id->lowlevel_id,TRUE) ; \
	  file_id->validation = NULL ; \
	  XtFree((char*)file_id) ; \
	  return MrmFAILURE ; \
	}

if ( (result=Idb__FIL_Valid(file_id)) != MrmSUCCESS ) return result ;

/*
 * We are serious about closing this file so deallocate the class and resource
 * tables before we do anything else.
 */

if (file_id->class_ctable != NULL)
    XtFree ((char*)file_id->class_ctable);
if (file_id->resource_ctable != NULL)
    XtFree ((char*)file_id->resource_ctable);

/*
 * If the file was opened for write, update its header
 */
if ( file_id->access == URMWriteAccess )
    {
    result = Idb__HDR_PutHeader ( file_id ) ;
    if ( result != MrmSUCCESS ) _error_close () ;
    }

/*
 * Decommit the file's buffers
 */
result = Idb__BM_DecommitAll (file_id) ;
if ( result != MrmSUCCESS ) _error_close () ;

/*
 * Close the file and deallocate the file descriptor
 */
result = Idb__FU_CloseFile ((IDBLowLevelFile*)file_id->lowlevel_id, FALSE) ;
file_id->validation = NULL ;
XtFree ((char*)file_id) ;

/*
 * File successfully closed
 */
return MrmSUCCESS ;

}



Cardinal UrmIdbGetIndexedResource
#ifndef _NO_PROTO
    (IDBFile			file_id,
    String			index,
    MrmGroup			group_filter,
    MrmType			type_filter,
    URMResourceContextPtr	context_id)
#else
        (file_id, index, group_filter, type_filter, context_id)
    IDBFile			file_id ;
    String			index ;
    MrmGroup			group_filter ;
    MrmType			type_filter ;
    URMResourceContextPtr	context_id ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	IdbGetIndexedResource attempts to retrieve a resource record with
 *	a matching index. The entry's group and type must match the filters
 *	when these are not null (RMCcNull and RMCtNull respectively). If a
 *	matching entry is found, its resource group, type, access, and data
 *	block are returned in the resource context. The resource context
 *	memory buffer will be reallocated as required to hold the data block.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		The IDB file id returned by IdbOpenFile
 *	index		case-sensitive index for the entry to match
 *	group_filter	if not null, entry found must match this group
 *	type_filter	if not null, entry found must match this type
 *	context_id	URM resource context to receieve data block
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmNOT_FOUND	entry not found
 *	MrmWRONG_GROUP	entry didn't match group filter
 *	MrmWRONG_TYPE	entry didn't match type filter
 *	MrmFAILURE	operation failed, no further reason
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
IDBDataHandle		data_entry ;	/* return new data entry */
MrmCode			group ;		/* data entry's group */
MrmCode			type ;		/* data entry's type */


if ( (result=Idb__FIL_Valid(file_id)) != MrmSUCCESS ) return result ;

/*
 * Attempt to find the item.
 */
result = Idb__INX_ReturnItem (file_id, index, &data_entry) ;
if ( result != MrmSUCCESS ) return result ;
result = Idb__DB_GetDataEntry (file_id, data_entry, context_id) ;
if ( result != MrmSUCCESS ) return result ;

/*
 * Check group and type against filters.
 */
group = UrmRCGroup (context_id) ;
type = UrmRCType (context_id) ;
if ( (group_filter!=URMgNul) && (group_filter!=group) )
    return MrmWRONG_GROUP ;
if ( (type_filter!=URMtNul) && (type_filter!=type) )
    return MrmWRONG_TYPE ;

/*
 * Item successfuly retrieved
 */
return MrmSUCCESS ;

}



Cardinal UrmIdbFindIndexedResource
#ifndef _NO_PROTO
    (IDBFile			file_id,
    MrmGroup			group_filter,
    MrmType			type_filter,
    URMPointerListPtr		index_list)
#else
        (file_id, group_filter, type_filter, index_list)
    IDBFile			file_id ;
    MrmGroup			group_filter ;
    MrmType			type_filter ;
    URMPointerListPtr		index_list ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	IdbFindIndexedResource locates and lists all resources in the
 *	database which match the given filters. The entries in the
 *	index list will be in alphabetical order. The index strings must
 *	be deallocated by the caller using UrmFreePlistContents. This
 *	routine returns MrmSUCCESS even if no entries (0 entries) are found.
 *	Failure returns are used only for internal and other errors.
 *
 *	The number of entries in index_list is not reset to 0 when
 *	the search begins; its state is as the caller gives it.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		The IDB file id returned by IdbOpenFile
 *	group_filter	if not null, entries found must match this group
 *	type_filter	if not null, entries found must match this type
 *	index_list	A pointer list in which to return index
 *			strings for matches. The required strings
 *			are automatically allocated.
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmFAILURE	operation failed, no further reason
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
 * Validate file id, then initiate search with the root node. Return
 * immediately if there are no indexed resources.
 */
if ( (result=Idb__FIL_Valid(file_id)) != MrmSUCCESS ) return result ;

if ( file_id->num_indexed <= 0 ) return MrmSUCCESS ;
return Idb__INX_FindResources (file_id, file_id->index_root,
    group_filter, type_filter, index_list) ;

}



Cardinal UrmIdbGetRIDResource
#ifndef _NO_PROTO
    (IDBFile			file_id,
    MrmResource_id		resource_id,
    MrmGroup			group_filter,
    MrmType			type_filter,
    URMResourceContextPtr	context_id)
#else
        (file_id, resource_id, group_filter, type_filter,context_id)
    IDBFile			file_id ;
    MrmResource_id		resource_id ;
    MrmGroup			group_filter ;
    MrmType			type_filter ;
    URMResourceContextPtr	context_id ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	IdbGetRIDResource attempts to retrieve a resource record with a
 *	matching resource id. If there is a resource record associated with
 *	the resource id, it is returned - the resource group, type, and data
 *	block are set in the resource context. The memory buffer in the
 *	resource context is reallocated automatically as require to hold the
 *	data block.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		The IDB file id returned by IdbOpenFile
 *	resource_id	resource id matching entry
 *	group_filter	if not null, entries found must match this group
 *	type_filter    	if not null, entries found must match this type
 *	context_id	to return data block
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmNOT_FOUND	entry not found
 *	MrmWRONG_GROUP	entry didn't match group filter
 *	MrmWRONG_TYPE	entry didn't match type filter
 *	MrmFAILURE	operation failed, no further reason
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
Cardinal	result ;		/* function results */
IDBDataHandle		data_entry ;	/* return new data entry */
MrmCode			group ;		/* data entry's group */
MrmCode			type ;		/* data entry's type */


if ( (result=Idb__FIL_Valid(file_id)) != MrmSUCCESS ) return result ;

/*
 * Attempt to find the item.
 */
result = Idb__RID_ReturnItem (file_id, resource_id, TRUE, &data_entry) ;
if ( result != MrmSUCCESS ) return result ;
result = Idb__DB_GetDataEntry (file_id, data_entry, context_id) ;
if ( result != MrmSUCCESS ) return result ;

/*
 * Check group and type against filters.
 */
group = UrmRCGroup (context_id) ;
type = UrmRCType (context_id) ;
if ( (group_filter!=URMgNul) && (group_filter!=group) )
    return MrmWRONG_GROUP ;
if ( (type_filter!=URMtNul) && (type_filter!=type) )
    return MrmWRONG_TYPE ;

/*
 * Item successfuly retrieved
 */
return MrmSUCCESS ;

}


Cardinal UrmIdbGetResourceId 
#ifndef _NO_PROTO
    (IDBFile			file_id,
    MrmResource_id		*resource_id_return)
#else
(file_id, resource_id_return)
    IDBFile			file_id ;
    MrmResource_id		*resource_id_return ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	IdbGetResourceId returns the next available resource id in the file
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id			The IDB file id returned by IdbOpenFile
 *	resource_id_return	To return new resource id
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmFAILURE	operation failed, no further reason given
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
Cardinal	result ;		/* function results */

if ( (result=Idb__FIL_Valid((IDBFile)file_id)) != MrmSUCCESS ) return result ;
result = Idb__RID_NextRID ((IDBFile)file_id, (IDBResource *)resource_id_return) ;
return result ;

}



Cardinal Idb__FIL_Valid (file_id)
    IDBFile		file_id ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine performs a validity check on a (supposed) IDB file
 *	pointer. It should probably signal errors, but currently just
 *	returns a code.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		Supposed pointer to file descriptor to validate
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmFAILURE	some other failure
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

if ( file_id == NULL ) return MrmFAILURE ;
if ( file_id->validation != IDBOpenFileValid ) return MrmFAILURE ;
return MrmSUCCESS ;

}

