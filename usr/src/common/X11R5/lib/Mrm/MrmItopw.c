#pragma ident	"@(#)m1.2libs:Mrm/MrmItopw.c	1.1"
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
 *      UIL Resource Manager (URM): IDB facility
 *
 *  ABSTRACT:
 *
 *      This module contains the publicly accessible top-level 
 *	IDB write routines.
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
 *	UrmIdbOpenFileWrite		Open a file for write access
 *
 *	UrmIdbPutIndexedResource		Put indexed resource to file
 *
 *	UrmIdbPutRIDResource		Put resource id'ed resource in file
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



Cardinal UrmIdbOpenFileWrite
        (name, os_ext, creator, creator_version, module, module_version,
            file_id_return, fname_return)
    String			name ;
    MrmOsOpenParamPtr		os_ext ;
    String			creator ;
    String			creator_version ;
    String			module ;
    String			module_version ;
    IDBFile			*file_id_return ;
    char			*fname_return ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	IdbOpenFileWrite creates a new IDB database file, with null index
 *	structures. This call creates the file and initializes the file
 *	header.
 *
 *  FORMAL PARAMETERS:
 *
 *	name			A system-dependent string specifying the IDB
 *				file to be opened.
 *	os_ext			An operating-system specific structure which
 *				supports using specific file system features
 *	creator			Identifies the database creator (application)
 *	creator_version		Creator version
 *	module			Identifies the UIL module
 *	module_version		Module version
 *	file_id_return		returns the IDB file id
 *	fname_return		returns the name of the file actually opened
 *				(via strcpy). Should be at least 256 chars.
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
 *      1. Acquires memory for the file descriptor
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
 * An OS extension block must be present for Unix, and by default
 * specifies that existing files are over-written.
 */

MrmOsOpenParam		osext ;		/* to delete file on open */
osext.version = MrmOsOpenParamVersion ;
osext.nam_flg.clobber_flg = TRUE ;
if ( os_ext == NULL ) os_ext = &osext ;


/*
 * Attempt to open the file. For now, pay no attention to temporary naming
 * issues.
 */
result = Idb__FU_OpenFile
    (name, URMWriteAccess, os_ext, (IDBLowLevelFilePtr *)&fileid, fname_return) ;
if ( result != MrmCREATE_NEW ) return result ;

/*
 * File successfully opened. Acquire a file descriptor and initialize it.
 */
filedesc = (IDBFile) XtMalloc (sizeof(IDBOpenFile)) ;
filedesc->validation = IDBOpenFileValid ;
filedesc->access = URMWriteAccess ;
filedesc->lowlevel_id = fileid ;
filedesc->last_record = 0 ;
filedesc->last_data_record = 0 ;
filedesc->get_count = 0 ;
filedesc->put_count = 0 ;
for ( ndx=IDBrtMin ; ndx<=IDBrtMax ; ndx++ )
    filedesc->rt_counts[ndx] = 0 ;

/*
 * Make sure all variables are initialized that aren't initialized elsewhere
 */
filedesc->index_root = 0;
filedesc->timer = 0;
filedesc->class_ctable = (UidCompressionTablePtr)NULL;
filedesc->resource_ctable = (UidCompressionTablePtr)NULL;
filedesc->user1 = 0;
filedesc->user2 = 0;

for ( ndx=0 ; ndx<=IDBhsVersion ; ndx++ )
    {
    filedesc->db_version[ndx] = 0 ;
    filedesc->creator_version[ndx] = 0 ;
    filedesc->module_version[ndx] = 0 ;
    };
for ( ndx=0 ; ndx<=IDBhsCreator ; ndx++ )
    filedesc->creator[ndx] = 0 ;
for ( ndx=0 ; ndx<=IDBhsDate ; ndx++ )
    filedesc->creation_date[ndx] = 0 ;
for ( ndx=0 ; ndx<=IDBhsModule ; ndx++ )
    filedesc->module[ndx] = 0 ;

/*
 * Write a new file header for this file
 */
result = Idb__HDR_InitHeader (filedesc, creator, creator_version,
				module, module_version) ;
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



Cardinal UrmIdbPutIndexedResource (file_id, index, context_id)
    IDBFile			file_id ;
    String			index ;
    URMResourceContextPtr	context_id ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	IdbPutIndexedResource creates a resource record holding the
 *	group, type, and resource data in the resource context, and saves it
 *	under the given index. The group and type must not be null, and the
 *	index must not be currently entered in the database index. The
 *	resource record is marked public or private depending on the value
 *	of the access parameter.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		The IDB file id returned by IdbOpenFile
 *	index		case-sensitive index for the new entry, must
 *			not match any existing entry
 *	context_id	URM resource context containing data block
 *			for entry
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmEXISTS	operation failed, index already exists
 *	MrmNUL_GROUP	operation failed, null group parameter
 *	MrmNUL_TYPE	operation failed, null type parameter
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
IDBRecordBufferPtr	bufptr ;	/* not used */
MrmCount		recno ;		/* not used */
IDBDataHandle		data_entry ;	/* return new data entry */
MrmCode			group ;		/* the entry's group code */


if ( (result=Idb__FIL_Valid(file_id)) != MrmSUCCESS ) return result ;

/*
 * Validity check on group and type, and make sure entry doesn't
 * currently exist.
 */
if ( UrmRCGroup(context_id) == URMgNul ) return MrmNUL_GROUP ;
if ( UrmRCType(context_id) == URMtNul) return MrmNUL_TYPE ;
result = Idb__INX_FindIndex (file_id, index, &bufptr, &recno) ;
if ( result == MrmSUCCESS) return MrmEXISTS ;

/*
 * Create the data entry for this item
 */
result = Idb__DB_PutDataEntry (file_id, context_id, &data_entry) ;
if ( result != MrmSUCCESS ) return result ;

/*
 * Enter the data entry under the index
 */
result = Idb__INX_EnterItem (file_id, index, data_entry) ;
if ( result != MrmSUCCESS ) return result ;
file_id->num_indexed++ ;

group = UrmRCGroup (context_id) ;
if ( group>=URMgMin && group<=URMgMax )
    file_id->group_counts[group]++ ;

/*
 * item successfully entered
 */
return MrmSUCCESS ;

}



Cardinal UrmIdbPutRIDResource (file_id, resource_id, context_id)
    IDBFile			file_id ;
    MrmResource_id		resource_id ;
    URMResourceContextPtr	context_id ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	IdbPutRIDResource creates a resource record holding the group, type,
 *	and resource data, and saves under the given resource id. The group
 *	and type must not be null, and the resource must currently have no
 *	resource record associated with it. Access is always URMaPrivate
 *	regardless of the data context access field.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		The IDB file id returned by IdbOpenFile
 *	resource_id	resource id for the new entry, should
 *			currently have no existing resource record
 *	context_id	URM resource context holding data block.
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmEXISTS	operation failed, index already exists
 *	MrmNUL_GROUP	operation failed, null group parameter
 *	MrmNUL_TYPE	operation failed, null type parameter
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
IDBDataHandle		data_entry ;	/* new data entry */
MrmCode			group ;		/* the entry's group code */


if ( (result=Idb__FIL_Valid(file_id)) != MrmSUCCESS ) return result ;

/*
 * Validity check on group and type, and make sure entry doesn't
 * currently exist.
 */
if ( UrmRCGroup(context_id) == URMgNul ) return MrmNUL_GROUP ;
if ( UrmRCType(context_id) == URMtNul) return MrmNUL_TYPE ;
result = Idb__RID_ReturnItem (file_id, resource_id, FALSE, &data_entry) ;
if ( result == MrmSUCCESS) return MrmEXISTS ;

/*
 * Create the data entry for this item
 */
result = Idb__DB_PutDataEntry (file_id, context_id, &data_entry) ;
if ( result != MrmSUCCESS ) return result ;

/*
 * Enter the data entry under the resource id
 */
result = Idb__RID_EnterItem (file_id, resource_id, data_entry) ;
if ( result != MrmSUCCESS ) return result ;
file_id->num_RID++ ;

group = UrmRCGroup (context_id) ;
if ( group>=URMgMin && group<=URMgMax )
    file_id->group_counts[group]++ ;

/*
 * item successfully entered
 */
return MrmSUCCESS ;

}


