#pragma ident	"@(#)m1.2libs:Mrm/MrmIrid.c	1.1"
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
 *	Resource ID management routines.
 *
 *  ABSTRACT:
 *
 *	These routines acquire RIDs, allow entering a data block under an RID,
 *	and manage the RID and index map records.
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
 *	Idb__RID_EnterItem		- Enter a data entry under an RID
 *
 *	Idb__RID_ReturnItem		- Return the data entry for an RID
 *
 *	Idb__RID_NextRID		- Return next available RID
 *
 *	Idb__RID_AddRecord		- Set up a new resource ID record

 */


/*
 *
 *  DEFINE and MACRO DEFINITIONS
 *
 */

/*
 * Macros which validate index records in buffers
 */
#define	Idb__RID_ValidRecord(buffer) \
	(_IdbBufferRecordType(buffer)==IDBrtRIDMap)



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




Cardinal Idb__RID_EnterItem (file_id, resource_id, data_entry)
    IDBFile			file_id;
    IDBResource			resource_id;
    IDBDataHandle		data_entry;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__RID_EnterItem makes an entry in the resource record for the
 *	resourc id of the data entry pointer.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		Open IDB file in which to write entry
 *	resource_id	Resource ID under which to enter entry
 *	data_entry	Data entry pointer for data
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmFAILURE	some other failure
 *	MrmOUT_OF_RANGE	Record number out of range
 *	MrmBAD_RECORD	not an RID map record
 *	URMBadDataindex	Data index out of range
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
Cardinal		result;		/* function results */
IDBRecordBufferPtr	bufptr;		/* header record buffer */
IDBridMapRecordPtr	recptr;		/* header record in buffer */
IDBridMapHdrPtr		hdrptr;		/* record header */
IDBridDesc		resid;		/* CAST resource id */
IDBResourceIndex	resndx;		/* to check resource index */
IDBRecordNumber		recno;		/* Map record record number */


/*
 * Get the resource map record. Let the header handle it if the
 * RID is in the header record.
 */
resid.external_id = resource_id;
recno = resid.internal_id.map_rec;
if ( recno == IDBHeaderRecordNumber )
    return Idb__HDR_EnterItem (file_id, resource_id, data_entry);

resndx = resid.internal_id.res_index;
if ( recno > file_id->last_record )
    return Urm__UT_Error
        ("Idb__RID_EnterItem", "Record beyond EOF",
        file_id, NULL, MrmOUT_OF_RANGE);
result = Idb__BM_GetRecord (file_id, recno, &bufptr);
if ( result != MrmSUCCESS ) return result;
if ( ! Idb__RID_ValidRecord(bufptr) )
    return Urm__UT_Error
        ("Idb__RID_EnterItem", "Unexpected record type",
        file_id, NULL, MrmBAD_RECORD);

recptr = (IDBridMapRecordPtr) bufptr->IDB_record;
hdrptr = (IDBridMapHdrPtr) &recptr->map_header;

/*
 * Set the data item.
 */
if ( resndx >= IDBridPtrVecMax )
    return Urm__UT_Error
        ("Idb__RID_EnterItem", "Resource index out of range",
        file_id, NULL, MrmBAD_DATA_INDEX);
recptr->pointers[resndx].pointer = data_entry;
Idb__BM_MarkActivity (bufptr);

/*
 * successful entry
 */
Idb__BM_MarkModified (bufptr);
return MrmSUCCESS;

}



Cardinal Idb__RID_ReturnItem
#ifndef _NO_PROTO
    (IDBFile			file_id,
    IDBResource			resource_id,
    Boolean			signal_null,
    IDBDataHandle		*entry_return)
#else
        (file_id, resource_id, signal_null, entry_return)
    IDBFile			file_id;
    IDBResource			resource_id;
    Boolean			signal_null;
    IDBDataHandle		*entry_return;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb_RID_ReturnItem returns the data entry pointer for an item
 *	entered in the database under a resource id. Since this routine
 *	is also used to check if an entry exists before writing, it
 *	uses a flag to determine if a null entry should be signalled.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		Open IDB file in which to write entry
 *	resource_id	Entry's resource id
 *	signal_null	if TRUE, signal error on null data entry
 *	entry_return	To return data pointer for data
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmOUT_OF_RANGE	Record number out of range
 *	MrmBAD_RECORD	not an RID map record
 *	MrmNULL_DATA	no data for resource id
 *	MrmFAILURE	some other failure
 *	URMBadDataindex	Data index out of range
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
Cardinal		result;		/* function results */
IDBRecordBufferPtr	bufptr;		/* RID map record buffer */
IDBridMapRecordPtr	recptr;		/* RID map record in buffer */
IDBridMapHdrPtr		hdrptr;		/* record header */
IDBridDesc		resid;		/* CAST resource id */
IDBResourceIndex	resndx;		/* to check resource index */
IDBRecordNumber		recno;		/* Map record record number */


/*
 * Get the resource map record. Let the header handle it if the RID is
 * in the header.
 */
resid.external_id = resource_id;
recno = resid.internal_id.map_rec;
if ( recno == IDBHeaderRecordNumber )
    return Idb__HDR_ReturnItem
	(file_id, resource_id, signal_null, entry_return);
resndx = resid.internal_id.res_index;
if ( recno > file_id->last_record )
return Urm__UT_Error
    ("Idb__RID_ReturnItem", "Record beyond EOF",
    file_id, NULL, MrmOUT_OF_RANGE);
result = Idb__BM_GetRecord (file_id, recno, &bufptr);
if ( result != MrmSUCCESS ) return result;
if ( ! Idb__RID_ValidRecord(bufptr) )
return Urm__UT_Error
    ("Idb__RID_ReturnItem", "Unexpected record type",
    file_id, NULL, MrmBAD_RECORD);

recptr = (IDBridMapRecordPtr) bufptr->IDB_record;
hdrptr = (IDBridMapHdrPtr) &recptr->map_header;

/*
 * Retrieve the data item.
 */
if ( resndx >= IDBridPtrVecMax )
    return Urm__UT_Error
        ("Idb__RID_ReturnItem", "Resource index out of range",
        file_id, NULL, MrmBAD_DATA_INDEX);
if ( recptr->pointers[resndx].pointer == NULL )
    if ( signal_null )
        return Urm__UT_Error
            ("Idb__RID_ReturnItem", "NULL data pointer",
            file_id, NULL, MrmNULL_DATA);
    else return MrmNULL_DATA;
*entry_return = recptr->pointers[resndx].pointer;

/*
 * Successful retrieval
 */
Idb__BM_MarkActivity (bufptr);
return MrmSUCCESS;

}



Cardinal Idb__RID_NextRID (file_id, res_id_return)
    IDBFile			file_id;
    IDBResource			*res_id_return;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__RID_NextRID returns the next available resource ID in the
 *	file. It is the internal version of UrmIdbGetResourceId. If a new
 *	resource record is required, this routine:
 *		- Acquires a new resource record, and initializes it
 *		  to all null pointers.
 *		- Adds a new entry to map index vector in the file header
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		Open IDB file in which to write entry
 *	res_id_return	To return new resource id
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
Cardinal		result;		/* function results */
IDBResourceIndex	resndx;		/* to check resource index */

/*
 * use the next available ID, or acquire a new record. Handle both header
 * record and RID record maps.
 */
resndx = file_id->next_RID.internal_id.res_index;
if ( file_id->next_RID.internal_id.map_rec == IDBHeaderRecordNumber )
    {
    if ( resndx >= IDBHeaderRIDMax )
	{
	result = Idb__RID_AddRecord (file_id);
	if ( result != MrmSUCCESS ) return result;
	resndx = file_id->next_RID.internal_id.res_index;
	}
    }

/*
 * RID comes from a RID record
 */
if ( resndx >= IDBridPtrVecMax )
    {
    result = Idb__RID_AddRecord (file_id);
    if ( result != MrmSUCCESS ) return result;
    resndx = file_id->next_RID.internal_id.res_index;
    }

/*
 * Increment resource index, returning current value.
 */
*res_id_return = (IDBResource) file_id->next_RID.external_id;
file_id->next_RID.internal_id.res_index++;
return MrmSUCCESS;

}



Cardinal Idb__RID_AddRecord (file_id)
    IDBFile		file_id;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__RID_AddRecord initializes a new resource id record and enters
 *	in in the index map in the header.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		Open IDB file
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
Cardinal		result;		/* function results */
IDBRecordBufferPtr	bufptr;		/* RID map record buffer */
IDBridMapRecordPtr	recptr;		/* RID map record in buffer */
int			ndx;		/* loop index */

/*
 * Acquire a record.
 */
result = Idb__BM_InitRecord (file_id, 0, IDBrtRIDMap, &bufptr);
if ( result != MrmSUCCESS ) return result;
recptr = (IDBridMapRecordPtr) bufptr->IDB_record;

/*
 * Initialize the record contents
 */
for (ndx=0 ; ndx<IDBridPtrVecMax ; ndx++)
    recptr->pointers[ndx].pointer = NULL;
Idb__BM_MarkModified (bufptr);

/*
 * Update the next RID in the file header to point to the first entry in
 * the new record.
 */
file_id->next_RID.internal_id.map_rec = recptr->map_header.header.record_num;
file_id->next_RID.internal_id.res_index = 0;

/*
 * Record successfully created.
 */
Idb__BM_MarkModified (bufptr);
return MrmSUCCESS;

}

