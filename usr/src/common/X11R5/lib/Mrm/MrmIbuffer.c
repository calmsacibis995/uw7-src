#pragma ident	"@(#)m1.2libs:Mrm/MrmIbuffer.c	1.1"
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
 *      UIL Resource Manager (URM): IDB Facility
 *	Buffer management routines
 *
 *  ABSTRACT:
 *
 *	These routines manage the buffer pool for all IDB files, and manage
 *	reading and writing actual file records into these buffers on request.
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
 *	Idb__BM_InitBufferVector	- Allocates and initializes the buffer vector
 *
 *	Idb__BM_GetBuffer		- Acquire a free buffer
 *
 *	Idb__BM_MarkActivity		- Mark a buffer as having seen activity
 *
 *	Idb__BM_MarkModified		- Mark a buffer as modified for write
 *
 *	Idb__BM_GetRecord		- Get a record into some buffer
 *
 *	Idb__BM_InitRecord		- Initialize a new record into
 *					  some buffer
 *
 *	Idb__BM_InitDataRecord		- Initialize a new data record into
 *					  some buffer
 *
 *	Idb__BM_Decommit		- Through with buffer contents
 *
 *	Idb__BM_DecommitAll		- Decommit all buffers for some file
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
 * This module manages a pool of buffers for all open IDB files. The pool
 * is located and managed via the following globals.
 */

/*
 * The following defines the number of buffers in the buffer pool. Once
 * the pool is initialized, this number is invariant (currently). This
 * variable will be accessible via currently undefined means for applications
 * to tailor the buffer pool size at startup.
 */
externaldef(idb__buffer_pool_size)	int	idb__buffer_pool_size = 8 ;


/*
 * The following pointer locates a vector of pointers to buffers. This pointer
 * is initially NULL. The first request for a buffer detects that the buffer
 * has not been allocated, and does the following:
 *	o Allocate a vector of buffer pointers (IDBRecordBufferPtr) containing
 *	  the number of entries specified in idb__buffer_pool_size.
 *
 * Thereafter, this vector locates the buffer pool.
 */
static	IDBRecordBufferPtr	idb__buffer_pool_vec = NULL ;


/*
 * The following counter is used to maintain the activity count in all
 * buffers.
 */
static	long int		idb__buffer_activity_count = 1 ;


Cardinal Idb__BM_InitBufferVector ()

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__BM_InitBufferVector is responsible for initializaing
 *	the buffer pool. This routine allocates the vector of record
 *	buffers, but does not allocate the actual buffer for each
 *	record, which is done on demand.
 *
 *  FORMAL PARAMETERS:
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 * 	MrmSUCCESS	operation succeeded
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
Cardinal		ndx ;		/* loop variable */
IDBRecordBufferPtr	bufptr ;	/* entry into buffer pool vector */

idb__buffer_pool_vec = (IDBRecordBufferPtr) XtMalloc
    (idb__buffer_pool_size*sizeof(IDBRecordBuffer)) ;
if ( idb__buffer_pool_vec == NULL )
     return Urm__UT_Error
         ("Idb__BM_InitBufferVector", "Vector allocation failed",
         NULL, NULL, MrmFAILURE) ;

for ( ndx=0,bufptr=idb__buffer_pool_vec ;
        ndx<idb__buffer_pool_size ;
        ndx++,bufptr++ )
    {
    bufptr->validation = IDBRecordBufferValid;
    bufptr->activity = 0 ;
    bufptr->access = 0 ;
    bufptr->cur_file = NULL ;
    bufptr->modified = FALSE ;
    bufptr->IDB_record = NULL ;
    }

return MrmSUCCESS;

}			/* End of routine Idb__BM_InitBufferVector	*/



Cardinal Idb__BM_GetBuffer (file_id, buffer_return)
    IDBFile			file_id ;
    IDBRecordBufferPtr		*buffer_return ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__BM_GetBuffer acquires a buffer from the buffer pool. The
 *	buffer chosen is some buffer with the lowest activity count. If
 *	necessary, its contents are written to disk. Note that a buffer
 *	which has been decommitted (activity count = 0) may be used
 *	immediately - a search for a buffer with a lower activity count
 *	is not required.
 *
 *	This routine is responsible for initializaing the buffer pool if
 *	it is not initialized. It is also responsible for acquiring
 *	a record buffer if a buffer pool vector entry is needed but has none.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		Open IDB file
 *	buffer_return	Returns a free buffer, contents undefined
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 * 	MrmSUCCESS	operation succeeded
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
 * Local macro to complete allocation of a buffer to a file, and
 * mark its activity.
 */
#define _IDB_Getbuffer_return() 					\
    {									\
    (*buffer_return)->cur_file = file_id;				\
    (*buffer_return)->access = file_id->access;				\
    Idb__BM_MarkActivity (*buffer_return);				\
    return MrmSUCCESS;							\
    }

/*
 *  Local variables
 */
Cardinal		result ;	/* function results */
int			ndx ;		/* loop index */
long int		lowest ;	/* lowest activity count found */
IDBRecordBufferPtr	curbuf ;	/* current buffer being examined */

/*
 * If the buffer pool is uninitialized, allocate it and
 * return the first buffer in the pool. Else search the buffer pool for the
 * buffer with the lowest activity. Decommited/0 activity buffers can
 * be returned immediately.
 */
if (idb__buffer_pool_vec == NULL)
    {
    result = Idb__BM_InitBufferVector () ;
    if ( result != MrmSUCCESS ) return result ;
    *buffer_return = idb__buffer_pool_vec ;
    }
else
    {
    lowest = idb__buffer_activity_count;
    for ( ndx=0,curbuf=idb__buffer_pool_vec ;
        ndx<idb__buffer_pool_size ;
        ndx++,curbuf++ )
        {
        if ( curbuf->activity == 0 )
            {
            *buffer_return = curbuf ;
            break ;
            }
        if ( curbuf->activity < lowest )
            {
            *buffer_return = curbuf ;
            lowest = curbuf->activity ;
            }
        }
    }

/*
 * Allocate a record buffer if required, and return immediately if
 * the buffer is decommitted or not yet used.
 */
if ( (*buffer_return)->IDB_record == NULL )
    {
    (*buffer_return)->IDB_record =
        (IDBDummyRecord *) XtMalloc(sizeof(IDBDummyRecord)) ;
    if ( (*buffer_return)->IDB_record == NULL )
        return Urm__UT_Error ("Idb__BM_GetBuffer",
            "Buffer allocation failed", NULL, NULL, MrmFAILURE) ;
    _IDB_Getbuffer_return ();
    }
if ( (*buffer_return)->activity == 0 ) _IDB_Getbuffer_return ();

/*
 * We have set the buffer pointer. See if it needs to be updated on
 * disk, and do so if required.
 */
if ( ((*buffer_return)->access==URMWriteAccess) && ((*buffer_return)->modified) )
    {
    result = Idb__BM_Decommit (*buffer_return) ;
    if ( result != MrmSUCCESS ) return result ;
    }

/*
 * Allocate the buffer to the file, and return.
 */
_IDB_Getbuffer_return ();

}


Cardinal Idb__BM_MarkActivity (buffer)
    IDBRecordBufferPtr		buffer ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__BM_MarkActivity asserts that the buffer has seen some activity,
 *	and updates its activity count. This is done by storing the current
 *	idb__buffer_activity_count++ in the buffer.
 *
 *  FORMAL PARAMETERS:
 *
 *	buffer		Record buffer to mark as modified
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

if ( ! Idb__BM_Valid(buffer) )
    return Urm__UT_Error
        ("Idb__BM_MarkActivity", "Invalid buffer",
        NULL, NULL, MrmNOT_VALID) ;
buffer->activity = idb__buffer_activity_count++ ;
return MrmSUCCESS ;

}



Cardinal Idb__BM_MarkModified (buffer)
    IDBRecordBufferPtr		buffer ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__BM_MarkModified asserts that the buffer has been modified,
 *	and sets its modify flag. Its activity count is updated.
 *
 *  FORMAL PARAMETERS:
 *
 *	buffer		Record buffer to mark as modified
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

if ( ! Idb__BM_Valid(buffer) )
    return Urm__UT_Error
        ("Idb__BM_MarkModified", "Invalid buffer",
        NULL, NULL, MrmNOT_VALID) ;
buffer->activity = idb__buffer_activity_count++ ;
buffer->modified = TRUE ;
return MrmSUCCESS ;

}



Cardinal Idb__BM_GetRecord 
#ifndef _NO_PROTO
    (IDBFile                     file_id,
    IDBRecordNumber             record,
    IDBRecordBufferPtr          *buffer_return)
#else
    (file_id, record, buffer_return)
    IDBFile			file_id ;
    IDBRecordNumber		record ;
    IDBRecordBufferPtr		*buffer_return ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__BM_GetRecord reads a record from an open IDB file into a buffer.
 *	If the record is already available in some buffer, then that buffer is
 *	returned without re-reading the record from disk. Otherwise, a buffer is
 *	acquired, the record read into it, and the buffer is returned. The
 *	buffer's activity count is updated.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		Open IDB file
 *	record		The desired record number
 *	buffer_return	Returns buffer containing record
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmNOT_FOUND	record doesn't exist
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
Cardinal		result ;	/* function results */
int			ndx ;		/* loop index */
IDBRecordBufferPtr	curbuf ;	/* current buffer being examined */


/*
 * If buffer pool is unallocated, get a buffer (which WILL allocate it),
 * and read the record into that. Else see if the record is already in
 * memory, and return it if so. If the record is not found, get a buffer
 * to read it into. We exit this if statement with a buffer ready for
 * the read operation.
 */
if ( idb__buffer_pool_vec == NULL )
    {
    result = Idb__BM_GetBuffer (file_id, buffer_return) ;
    if ( result != MrmSUCCESS ) return result ;
    }
else
    {
    for ( ndx=0,curbuf=idb__buffer_pool_vec ;
            ndx<idb__buffer_pool_size ;
            ndx++,curbuf++ )
        {
        if ( (curbuf->cur_file==file_id) &&
             (curbuf->IDB_record->header.record_num==record) )
            {
            *buffer_return = curbuf ;
            Idb__BM_MarkActivity (*buffer_return) ;
            return MrmSUCCESS ;
            }
        }
    result = Idb__BM_GetBuffer (file_id, buffer_return) ;
    if ( result != MrmSUCCESS ) return result ;
    }

/*
 * Read the record into the buffer.
 */
result = Idb__FU_GetBlock
    (( IDBLowLevelFile *)file_id->lowlevel_id, record, 
     (char*)(*buffer_return)->IDB_record) ;
if ( result != MrmSUCCESS )
     return Urm__UT_Error
         ("Idb__BM_GetRecord", "Get block failed",
         file_id, NULL, result) ;
file_id->get_count++ ;

/*
 * Validate the record
 */
if ( (*buffer_return)->IDB_record->header.validation != IDBRecordHeaderValid )
     return Urm__UT_Error
         ("Idb__BM_GetRecord", "Invalid record header",
         file_id, NULL, MrmNOT_VALID) ;

/*
 * Record successfully read
 */
Idb__BM_MarkActivity (*buffer_return) ;
return MrmSUCCESS ;

}



Cardinal Idb__BM_InitRecord
#ifndef _NO_PROTO
    (IDBFile                     file_id,
    IDBRecordNumber             record,
    MrmType                     type,
    IDBRecordBufferPtr          *buffer_return)
#else
 (file_id, record, type, buffer_return)
    IDBFile			file_id ;
    IDBRecordNumber		record ;
    MrmType			type ;
    IDBRecordBufferPtr		*buffer_return ;
#endif
/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__BM_InitRecord initializes a new record for the file. A buffer
 *	is acquired, and the record number and record type set. The record
 *	number may be specified; if the record number given is <= 0, then the
 *	next available record is taken from the file header. The next
 *	available record number is updated to the new record number if it is
 *	greater than the current value. The record is marked for write access,
 *	and as modified. It is not written to disk. The buffer's activity
 *	count is updated.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		Open IDB file
 *	record		The desired record number. This will be a new record in
 *			the file
 *	type		The record type, from IDBrt...
 *	buffer_return	Returns buffer initialized for record, with type set.
 *			Access is URMWriteAccess, modified.
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


if ((Idb__BM_GetBuffer (file_id, buffer_return)) != MrmSUCCESS)
    return MrmFAILURE;

/*
 * Set the record number if required. Since the last_record parameter
 * records the last record currently in use, it must be incremented
 * before use to give the new last record.
 */
if (record <= 0)
    record = ++file_id->last_record;

if (record > file_id->last_record)
	file_id->last_record = record;

(*buffer_return)->IDB_record->header.validation = IDBRecordHeaderValid ;
(*buffer_return)->IDB_record->header.record_num = record ;
(*buffer_return)->IDB_record->header.record_type = type ;

(*buffer_return)->access	= file_id->access ;
(*buffer_return)->cur_file	= file_id ;
(*buffer_return)->modified	= TRUE ;

Idb__BM_MarkActivity (*buffer_return) ;

/*
 * Update the record type counter in the file header
 */
file_id->rt_counts[type]++ ;

/*
 * Successfully initialized
 */
return MrmSUCCESS ;

}


Cardinal Idb__BM_InitDataRecord (file_id, buffer_return)
    IDBFile			file_id ;
    IDBRecordBufferPtr		*buffer_return ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__BM_InitDataRecord initializes a new data record for the file. A buffer
 *	is acquired, and the record number and record type set. The record
 *	number may be specified; if the record number given is <= 0, then the
 *	next available record is taken from the file header. The next
 *	available record number is updated to the new record number if it is
 *	greater than the current value. The record is marked for write access,
 *	and as modified. It is not written to disk. The buffer's activity
 *	count is updated.  This routine is nearly identical to InitRecord.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		Open IDB file
 *	buffer_return	Returns buffer initialized for record, with type set.
 *			Access is URMWriteAccess, modified.
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
Cardinal		result ;	/* function results */
IDBDataRecordPtr	data_rec;	/* data record specific */


/*
 * Get a new record
 */
result = Idb__BM_InitRecord (file_id, 0, IDBrtData, buffer_return) ;
if ( result != MrmSUCCESS ) return result ;

/*
 * Set the last data record pointer in the file
 */
file_id->last_data_record = _IdbBufferRecordNumber (*buffer_return) ;

/*
 * Initialize the record contents
 */
data_rec = (IDBDataRecord *) (*buffer_return)->IDB_record;

data_rec->data_header.free_ptr		= 0 ;
data_rec->data_header.free_count	= IDBDataFreeMax ;
data_rec->data_header.num_entry		= 0 ;
data_rec->data_header.last_entry	= 0 ;
Idb__BM_MarkActivity (*buffer_return) ;

return MrmSUCCESS ;

}



Cardinal Idb__BM_Decommit (buffer)
    IDBRecordBufferPtr		buffer ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__BM_Decommit asserts that the buffer contents are no longer
 *	required to be in memory. If the buffer is marked for write access
 *	and as modified, it is written to disk. Then the modified flag is
 *	reset and the activity count set to 0 (least value).
 *
 *	Note that GetRecord may legally re-acquire a decommitted buffer.
 *
 *  FORMAL PARAMETERS:
 *
 *	buffer		Record buffer to decommit
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
Cardinal		result ;	/* function results */


if ( ! Idb__BM_Valid(buffer) )
    return Urm__UT_Error
        ("Idb__BM_Decommit", "Invalid buffer",
        NULL, NULL, MrmNOT_VALID) ;

if ((buffer->access == URMWriteAccess) && (buffer->modified == TRUE))
    {
    result = Idb__FU_PutBlock (( IDBLowLevelFile *)buffer->cur_file->lowlevel_id,
		  	          buffer->IDB_record->header.record_num,
			          (char*)buffer->IDB_record) ;
    if ( result != MrmSUCCESS )
        return Urm__UT_Error
            ("Idb__BM_Decommit", "Put block failed",
            NULL, NULL, MrmNOT_VALID) ;
    buffer->cur_file->put_count++ ;

    buffer->activity = 0;
    buffer->modified = FALSE;
    }

return MrmSUCCESS;

}



Cardinal Idb__BM_DecommitAll (file_id)
    IDBFile		file_id ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__BM_DecommitAll decommits all the buffers currently allocated
 *	for a file. It returns an error immediately if there is any
 *	problem decommitting any buffer.
 *
 *	Since this routine removes all buffers for a file in prepration
 *	for closing it, it also wipes out the file pointer in each buffer.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		IDB file for which to write all modified buffers
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
Cardinal		result ;	/* function result */
Cardinal		ndx ;		/* loop variable		*/
IDBRecordBufferPtr	curbuf ;	/* ptr to element in buff pool	*/

if ( idb__buffer_pool_vec == NULL )
    return MrmFAILURE;

for ( ndx=0,curbuf=idb__buffer_pool_vec ;
        ndx<idb__buffer_pool_size ;
        ndx++,curbuf++ )
    {
    if (curbuf->cur_file == file_id)
        {
        result = Idb__BM_Decommit (curbuf) ;
        if ( result != MrmSUCCESS ) return result ;
        curbuf->cur_file = NULL ;
        }
    }

return MrmSUCCESS ;

}

