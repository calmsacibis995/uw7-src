#pragma ident	"@(#)m1.2libs:Mrm/MrmIheader.c	1.1"
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
*  (c) Copyright 1989, 1990, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */



/*
 *++
 *  FACILITY:
 *
 *      UIL Resource Manager (URM): IDB facility
 *
 *  ABSTRACT:
 *
 *	These routines transfer header information between a file and the
 *	in-memory file header struct. All IDB routines maintain the in-memory
 *	file header; the on-disk header is only updated when the header is
 *	initialized or the file is closed. These routines assume that all
 *	information required in the header is continuously up-to-date in
 *	the open file struct.
 *
 *	All header information must be updated at the time a record is
 *	modified, not written.
 *
 *--
 */


/*
 *
 *  INCLUDE FILES
 *
 */

#include <stdio.h>
#include <Mrm/MrmAppl.h>
#include <Mrm/Mrm.h>
#include <Mrm/IDB.h>


/*
 *
 *  TABLE OF CONTENTS
 *
 *	Idb__HDR_GetHeader		- Read a file header to memory
 *
 *	Idb__HDR_InitHeader		- Initialize a file header
 *
 *	Idb__HDR_PutHeader		- Write the header to disk
 *
 */


/*
 *
 *  DEFINE and MACRO DEFINITIONS
 *
 */

/*
 * Macros which validate index records in buffers
 */
#define	Idb__HDR_ValidRecord(buffer) \
	(_IdbBufferRecordType(buffer)==IDBrtHeader)



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
 * The following defines the database version written into all
 * header records.
 */

static XmConst	char		*idb__database_version = URMversion;



Cardinal Idb__HDR_GetHeader (file_id)
    IDBFile			file_id;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__HDR_GetHeader reads the file's header record into a buffer,
 *	copies the information into the in-memory header.
 *
 *	This routine is called as part of InitHeader to move the contents
 *	of the initialized header to memory. This usage expects the
 *	the record to be in the buffer pool, of course.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		Partially initialized in-memory file header struct; the
 *			file is open and the low-level file id is saved.
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmNOT_FOUND	header record not found
 *	MrmNOT_VALID	database version mismatch
 *	MrmBAD_RECORD	not the header record
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
IDBRecordBufferPtr	hdrbuf;		/* header record buffer */
IDBHeaderRecordPtr	recptr;		/* header record in buffer */
IDBHeaderHdrPtr		hdrptr;		/* record header */
int			ndx;		/* loop index */
char			errmsg[300];


/*
 * Get the header record
 */
result = Idb__BM_GetRecord (file_id, IDBHeaderRecordNumber, &hdrbuf);
if ( result != MrmSUCCESS ) return result;
if ( ! Idb__HDR_ValidRecord(hdrbuf) )
    return Urm__UT_Error
        ("Idb__HDR_GetHeader", "Unexpected record type",
        file_id, NULL, MrmBAD_RECORD);

/*
 * Read the header contents into memory, acquiring buffers as necessary
 */
recptr = (IDBHeaderRecordPtr) hdrbuf->IDB_record;
hdrptr = (IDBHeaderHdrPtr) &recptr->header_hdr;
file_id->index_root = hdrptr->index_root;
file_id->num_indexed = hdrptr->num_indexed;
file_id->num_RID = hdrptr->num_RID;
file_id->next_RID = hdrptr->next_RID;
file_id->last_record = hdrptr->last_record;
file_id->last_data_record = hdrptr->last_data_record;

for ( ndx=IDBrtMin ; ndx<=IDBrtMax ; ndx++ )
    file_id->rt_counts[ndx] = hdrptr->rt_counts[ndx];

for ( ndx=URMgMin ; ndx<=URMgMax ; ndx++ )
    file_id->group_counts[ndx] = hdrptr->group_counts[ndx];

strcpy (file_id->db_version, hdrptr->db_version);
strcpy (file_id->creator, hdrptr->creator);
strcpy (file_id->creator_version, hdrptr->creator_version);
strcpy (file_id->creation_date, hdrptr->creation_date);
strcpy (file_id->module, hdrptr->module);
strcpy (file_id->module_version, hdrptr->module_version);

/*
 * Header successfully read. Leave the buffer committed in case data
 * will be read from it.
 *
 * Compare version in header against compiled-in version, flagging
 * mismatch if found.
 */
if ( strcmp(hdrptr->db_version,idb__database_version) > 0 )
    {
    sprintf (errmsg, "File version %s is greater than Mrm version %s",
	     hdrptr->db_version, idb__database_version);
    return Urm__UT_Error
	("Idb__HDR_GetHeader", errmsg, file_id, NULL, MrmNOT_VALID);
    }

return MrmSUCCESS;

}



Cardinal Idb__HDR_InitHeader
        (file_id, creator, creator_version, module, module_version)
    IDBFile			file_id;
    String			creator;
    String			creator_version;
    String			module;
    String			module_version;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__HDR_InitHeader initializes a header record for a new file. The
 *	information in the routine arguments is combined with information
 *	derived by the routine (database version, dates and times) and
 *	written into the file header record. Then the buffer is
 *	decommitted.
 *
 *  FORMAL PARAMETERS:
 *
 *	creator		Creator id for the file
 *	creator_version	Creator version string
 *	module		Module id for the file
 *	module_version	Module version string
 *	file_id		To return new in-memory file header for the file.
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
IDBRecordBufferPtr	hdrbuf;		/* header record buffer */
IDBHeaderRecordPtr	recptr;		/* header record in buffer */
IDBHeaderHdrPtr		hdrptr;		/* record header */
int			ndx;		/* loop index */

/*
 * Acquire a buffer for the header, initialize it, and set pointers
 * into it.
 */
result = Idb__BM_InitRecord (file_id, IDBHeaderRecordNumber,
    IDBrtHeader, &hdrbuf);
if ( result != MrmSUCCESS ) return result;
recptr = (IDBHeaderRecordPtr) hdrbuf->IDB_record;
hdrptr = (IDBHeaderHdrPtr) &recptr->header_hdr;

/*
 * Initialize the record contents, first the ones which occur only in
 * the file header, then those in common with the in-memory file struct.
 */
strcpy ( hdrptr->db_version, "" );
strncat (hdrptr->db_version, idb__database_version, IDBhsVersion);
strcpy ( hdrptr->creator, "" );
strncat (hdrptr->creator, creator, IDBhsCreator);
strcpy ( hdrptr->creator_version, "" );
strncat (hdrptr->creator_version, creator_version, IDBhsVersion);
Urm__UT_Time (hdrptr->creation_date);
strcpy ( hdrptr->module, "" );
strncat (hdrptr->module, module, IDBhsModule);
strcpy ( hdrptr->module_version, "" );
strncat (hdrptr->module_version, module_version, IDBhsVersion);

hdrptr->index_root = 0;
hdrptr->num_indexed = 0;
hdrptr->num_RID = 0;
hdrptr->next_RID.internal_id.map_rec = hdrptr->header.record_num;
hdrptr->next_RID.internal_id.res_index = 0;
hdrptr->last_data_record = 0;
for ( ndx=IDBrtMin ; ndx<=IDBrtMax ; ndx++ )
    hdrptr->rt_counts[ndx] = file_id->rt_counts[ndx];
for ( ndx=URMgMin ; ndx<=URMgMax ; ndx++ )
    hdrptr->group_counts[ndx] = 0;

for ( ndx=0 ; ndx<IDBHeaderRIDMax ; ndx++ )
    recptr->RID_pointers[ndx].pointer = 0;
recptr->num_entry = 0;
recptr->last_entry = 0;
recptr->free_ptr = 0;
recptr->free_count = IDBHeaderFreeMax;

Idb__BM_MarkModified (hdrbuf);

/*
 * Set the file structure in memory
 */
file_id->index_root = hdrptr->index_root;
file_id->num_indexed = hdrptr->num_indexed;
file_id->num_RID = hdrptr->num_RID;
file_id->next_RID = hdrptr->next_RID;
file_id->last_data_record = hdrptr->last_data_record;
for ( ndx=URMgMin ; ndx<=URMgMax ; ndx++ )
    file_id->group_counts[ndx] = 0;

/*
 * Header successfully initialized
 */
return MrmSUCCESS;

}



Cardinal Idb__HDR_PutHeader (file_id)
    IDBFile			file_id;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__Hdr_PutHeader copies all dynamic information from the in-memory
 *	file header into the on-disk file header. This routine is called
 *	before the final DecommitAll on a file as it is being closed. It
 *	is required that no information be placed in the header as a result
 *	of the DecommitAll; thus all header modifications take place as
 *	records are modified, not written.
 *
 *	This routine updates the header, then decommits it.
 *
 *  FORMAL PARAMETERS:
 *
 *	IDBFile			file_id;
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
IDBRecordBufferPtr	hdrbuf;		/* header record buffer */


result = Idb__HDR_UpdateHeader (file_id, &hdrbuf);
if ( result != MrmSUCCESS ) return result;

/*
 * Header successfully written
 */
return Idb__BM_Decommit (hdrbuf);

}



Cardinal Idb__HDR_UpdateHeader (file_id, buffer)
    IDBFile			file_id;
    IDBRecordBufferPtr		*buffer;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__Hdr_UpdateHeader copies all dynamic information from the
 *	in-memory file header into the on-disk file header.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		Open IDB file
 *	buffer		To return pointer to buffer containing header record
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmBAD_RECORD	not the header record
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
IDBHeaderRecordPtr	recptr;		/* header record in buffer */
IDBHeaderHdrPtr		hdrptr;		/* record header */
int			ndx;		/* loop index */


/*
 * Get the header record
 */
result = Idb__BM_GetRecord (file_id, IDBHeaderRecordNumber, buffer);
if ( result != MrmSUCCESS ) return result;
if ( ! Idb__HDR_ValidRecord(*buffer) )
    return Urm__UT_Error
        ("Idb__HDR_PutHeader", "Unexpected record type",
        file_id, NULL, MrmBAD_RECORD);

/*
 * Set up pointers into the record, then copy from the memory structure
 * into it.
 */
recptr = (IDBHeaderRecordPtr) (*buffer)->IDB_record;
hdrptr = (IDBHeaderHdrPtr) &recptr->header_hdr;
hdrptr->index_root = file_id->index_root;
hdrptr->num_indexed = file_id->num_indexed;
hdrptr->num_RID = file_id->num_RID;
hdrptr->next_RID = file_id->next_RID;
hdrptr->last_record = file_id->last_record;
hdrptr->last_data_record = file_id->last_data_record;

for ( ndx=IDBrtMin ; ndx<=IDBrtMax ; ndx++ )
    hdrptr->rt_counts[ndx] = file_id->rt_counts[ndx];

for ( ndx=URMgMin ; ndx<=URMgMax ; ndx++ )
    hdrptr->group_counts[ndx] = file_id->group_counts[ndx];

/*
 * Header successfully written
 */
Idb__BM_MarkModified (*buffer);
return MrmSUCCESS;

}


Cardinal Idb__HDR_EnterItem (file_id, resource_id, data_entry)
    IDBFile			file_id;
    IDBResource			resource_id;
    IDBDataHandle		data_entry;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__HDR_EnterItem is the analog of Idb__RID_EnterItem for entering
 *	a resource id in the header record.
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
 *	MrmBAD_RECORD	not the header record
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
IDBHeaderRecordPtr	recptr;		/* header record in buffer */
IDBHeaderHdrPtr		hdrptr;		/* record header */
IDBridDesc		resid;		/* CAST resource id */
IDBResourceIndex	resndx;		/* to check resource index */
IDBRecordNumber		recno;		/* Map record record number */


/*
 * Get the header record.
 */
resid.external_id = resource_id;
recno = resid.internal_id.map_rec;
resndx = resid.internal_id.res_index;
if ( recno != IDBHeaderRecordNumber )
    return Urm__UT_Error
        ("Idb__HDR_EnterItem", "Not the header record",
        file_id, NULL, MrmOUT_OF_RANGE);

result = Idb__BM_GetRecord (file_id, recno, &bufptr);
if ( result != MrmSUCCESS ) return result;
if ( ! Idb__HDR_ValidRecord(bufptr) )
    return Urm__UT_Error
        ("Idb__HDR_EnterItem", "Unexpected record type",
        file_id, NULL, MrmBAD_RECORD);

recptr = (IDBHeaderRecordPtr) bufptr->IDB_record;
hdrptr = (IDBHeaderHdrPtr) &recptr->header_hdr;

/*
 * Set the data item.
 */
if ( resndx >= IDBHeaderRIDMax )
    return Urm__UT_Error
        ("Idb__HDR_EnterItem", "Resource index out of range",
        file_id, NULL, MrmBAD_DATA_INDEX);
recptr->RID_pointers[resndx].pointer = data_entry;
Idb__BM_MarkActivity (bufptr);

/*
 * successful entry
 */
Idb__BM_MarkModified (bufptr);
return MrmSUCCESS;

}


Cardinal Idb__HDR_ReturnItem

#ifndef _NO_PROTO
    (IDBFile	file_id,
    IDBResource	resource_id,
    Boolean	signal_null,
    IDBDataHandle	*entry_return)
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
 *	Idb__HDR_ReturnItem returns the data entry pointer for an item
 *	entered under a resource ID maintained in the header record. It
 *	is an analog of Idb__RID_ReturnItem.
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
 *	MrmBAD_RECORD	not the data record
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
IDBHeaderRecordPtr	recptr;		/* RID map record in buffer */
IDBHeaderHdrPtr		hdrptr;		/* record header */
IDBridDesc		resid;		/* CAST resource id */
IDBResourceIndex	resndx;		/* to check resource index */
IDBRecordNumber		recno;		/* Map record record number */


/*
 * Get the header record
 */
resid.external_id = resource_id;
recno = resid.internal_id.map_rec;
resndx = resid.internal_id.res_index;
if ( recno != IDBHeaderRecordNumber )
    return Urm__UT_Error
	("Idb__HDR_ReturnItem", "Record not header record",
	 file_id, NULL, MrmOUT_OF_RANGE);

result = Idb__BM_GetRecord (file_id, recno, &bufptr);
if ( result != MrmSUCCESS ) return result;
if ( ! Idb__HDR_ValidRecord(bufptr) )
    return Urm__UT_Error
	("Idb__HDR_ReturnItem", "Unexpected record type",
	 file_id, NULL, MrmBAD_RECORD);

recptr = (IDBHeaderRecordPtr) bufptr->IDB_record;
hdrptr = (IDBHeaderHdrPtr) &recptr->header_hdr;

/*
 * Retrieve the data item.
 */
if ( resndx >= IDBHeaderRIDMax )
    return Urm__UT_Error
        ("Idb__HDR_ReturnItem", "Resource index out of range",
        file_id, NULL, MrmBAD_DATA_INDEX);
if ( recptr->RID_pointers[resndx].pointer == 0)
    if ( signal_null )
        return Urm__UT_Error
            ("Idb__HDR_ReturnItem", "NULL data pointer",
            file_id, NULL, MrmNULL_DATA);
    else return MrmNULL_DATA;
*entry_return = recptr->RID_pointers[resndx].pointer;

/*
 * Successful retrieval
 */
Idb__BM_MarkActivity (bufptr);
return MrmSUCCESS;

}



Cardinal Idb__HDR_NextRID (file_id, res_id_return)
    IDBFile			file_id;
    IDBResource			*res_id_return;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__HDR_NextRID returns the next available resource ID in the
 *	header record, if there is one. It is an analog of Idb__RID_NextRID,
 *	and serves it.
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
 *	MrmBAD_RECORD	not the header record
 *	MrmFAILURE	some other failure. Use an RID record.
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
IDBResourceIndex	resndx;		/* to check resource index */

/*
 * use the next available ID, or fail.
 */
resndx = file_id->next_RID.internal_id.res_index;
if ( file_id->next_RID.internal_id.map_rec != IDBHeaderRecordNumber )
    return Urm__UT_Error
	("Idb__HDR_NextRID", "Not the header record",
	 file_id, NULL, MrmBAD_RECORD);
if ( resndx >= IDBHeaderRIDMax )
    return MrmFAILURE;

/*
 * Increment resource index, returning current value.
 */
*res_id_return = (IDBResource) file_id->next_RID.external_id;
file_id->next_RID.internal_id.res_index++;
return MrmSUCCESS;

}



Cardinal Idb__HDR_GetDataEntry (file_id, data_entry, context_id)
    IDBFile			file_id;
    IDBDataHandle		data_entry;
    URMResourceContextPtr	context_id;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__HDR_GetDataEntry is the analog to Idb__HDR_GetDataEntry for
 *	retrieving data entries from the header record.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		Open ID file
 *	data_entry	Data entry to be fetched
 *	context_id	To receive data block for data entry
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmBAD_RECORD	not the header record
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
Cardinal		result;		/* return status */
IDBDataPointer		entry_ptr;	/* to map onto data_entry parameter */
IDBRecordNumber		record_number;	/* Record to be read in */
IDBDataEntryHdrPtr	datahdr;	/* Header part of entry */
IDBSimpleDataPtr	sim_data;	/* Simple data entry */
IDBHeaderRecordPtr	recptr;		/* pointer data record */
IDBRecordBufferPtr	bufptr;		/* temp buffer for record */
char			*buff_ptr;	/* ptr into context buffer */

/*
 * Check and see if the context is valid
 */
if (! UrmRCValid (context_id))
    return Urm__UT_Error
        ("Idb__HDR_GetDataEntry", "Invalid context",
        NULL, NULL, MrmBAD_CONTEXT);

/*
 * Make the data entry accessible as a data pointer.
 */
entry_ptr.pointer = data_entry;
record_number = entry_ptr.internal_id.rec_no;
if ( record_number != IDBHeaderRecordNumber )
    return Urm__UT_Error
        ("Idb__HDR_GetDataEntry", "Not the header record",
        NULL, NULL, MrmBAD_RECORD);

/*
 * Get the record that contains this data, get to the correct offset in
 * that record.
 */
result = Idb__BM_GetRecord (file_id, record_number, &bufptr);
if ( result != MrmSUCCESS ) return result;

/*
 * Point to the header in the data entry, set the context data. The context
 * is resized if necessary. Note that all context info except the
 * actual data can be set now regardless of the entry type.
 */
recptr = (IDBHeaderRecordPtr) bufptr->IDB_record;
datahdr =
    (IDBDataEntryHdrPtr) &recptr->data[entry_ptr.internal_id.item_offs];
if (datahdr->validation != IDBDataEntryValid)
    return Urm__UT_Error
        ("Idb__HDR_GetDataEntry", "Invalid data entry",
        NULL, context_id, MrmNOT_VALID);

if ( datahdr->entry_size > UrmRCSize(context_id) )
    {
    result = UrmResizeResourceContext (context_id, datahdr->entry_size);
    if ( result != MrmSUCCESS ) return result;
    }

UrmRCSetSize (context_id, datahdr->entry_size);
UrmRCSetGroup (context_id, datahdr->resource_group);
UrmRCSetType (context_id, datahdr->resource_type);
UrmRCSetAccess (context_id, datahdr->access);
UrmRCSetLock (context_id, datahdr->lock);

/*
 * Read the data into the context. It must always be a simple type in
 * the header record.
 */
buff_ptr = (char *) UrmRCBuffer(context_id);
switch ( datahdr->entry_type )
    {
    case IDBdrSimple:
        sim_data = (IDBSimpleDataPtr) datahdr;
	UrmBCopy (sim_data->data, buff_ptr, datahdr->entry_size);
        return MrmSUCCESS;

    case IDBdrOverflow:
	return Urm__UT_Error
	    ("Idb__HDR_GetDAtaEntry", "Illegal overflow record",
	     file_id, NULL, MrmFAILURE);

    default:
        return Urm__UT_Error
            ("Idb__HDR_GetDataEntry", "Unknown data entry type",
            NULL, context_id, MrmFAILURE);
    }

}



Cardinal Idb__HDR_PutDataEntry (file_id, context_id, data_entry)
    IDBFile			file_id;
    URMResourceContextPtr	context_id;
    IDBDataHandle		*data_entry;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb_DB_PutDataEntry stores the resource described in the resource
 *	context into the database, returning the resulting data entry pointer.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		Open IDB file
 *	context_id	contains data block to be stored
 *	one_entry	To return data entry for newly stored entry
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
Cardinal		result;		/* returned status */
IDBDataPointer		entry_ptr;	/* to map onto data_entry parameter */
IDBSimpleDataPtr	simpledata;	/* simple data entry ptr */
IDBHeaderRecordPtr	recptr;		/* pointer data record */
IDBRecordBufferPtr	bufptr;		/* current record buffer pointer */
MrmCount		entsiz;		/* Number of bytes for new entry */
MrmOffset		entoffs;	/* Entry offset in buffer */
char			*dataptr;	/* pointer to data in context */

/*
 * Consistency check
 */
if (! UrmRCValid (context_id))
    return Urm__UT_Error
        ("Idb__HDR_PutDataEntry", "Invalid context",
        NULL, NULL, MrmBAD_CONTEXT);

/*
 * Acquire the header record and pointers to it
 */
result = Idb__BM_GetRecord (file_id, IDBHeaderRecordNumber, &bufptr);
if ( result != MrmSUCCESS ) return result;
recptr = (IDBHeaderRecordPtr) bufptr->IDB_record;

/*
 * See if the entry will fit. Simple failure if not, use a regular data
 * record.
 */
entsiz = IDBSimpleDataHdrSize + UrmRCSize(context_id);
entsiz = _FULLWORD (entsiz);
if ( entsiz > recptr->free_count )
    return MrmFAILURE;

/*
 * Acquire a simple data entry, and put this entry into it.
 */
entoffs = recptr->free_ptr;
simpledata = (IDBSimpleDataPtr) &recptr->data[entoffs];
simpledata->header.validation		= IDBDataEntryValid;
simpledata->header.entry_type		= IDBdrSimple;
simpledata->header.resource_group	= UrmRCGroup(context_id);
simpledata->header.resource_type	= UrmRCType(context_id);
simpledata->header.access		= UrmRCAccess(context_id);
simpledata->header.entry_size		= UrmRCSize(context_id);
simpledata->header.lock			= UrmRCLock(context_id);

dataptr = (char *) UrmRCBuffer(context_id);
UrmBCopy (dataptr, simpledata->data, UrmRCSize(context_id));

/*
 * Set the return value to the data pointer for this entry
 */	
entry_ptr.internal_id.rec_no = _IdbBufferRecordNumber (bufptr);
entry_ptr.internal_id.item_offs = recptr->free_ptr;
*data_entry = entry_ptr.pointer;

/*
 * Update the entry chain, mark the buffer, and return.
 */
simpledata->header.prev_entry = recptr->last_entry;
recptr->num_entry++;
recptr->last_entry = entoffs;
recptr->free_ptr += entsiz;
recptr->free_count -= entsiz;

Idb__BM_MarkModified (bufptr);
return MrmSUCCESS;

}



Boolean Idb__HDR_MatchFilter 

#ifndef _NO_PROTO
    (IDBFile	file_id,
    IDBDataHandle		data_entry,
    MrmCode			group_filter,
    MrmCode			type_filter)
#else
(file_id, data_entry, group_filter, type_filter)
    IDBFile			file_id;
    IDBDataHandle		data_entry;
    MrmCode			group_filter;
    MrmCode			type_filter;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine checks if a data entry matchs a set of filters.
 *	It reads the record containing the header for the data entry,
 *	then does the filter match. If both filters are NUL, then
 *	the test becomes merely one of confirming the data entry
 *	can be read.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		Open ID file
 *	data_entry	Data entry to be matched
 *	group_filter	if not null, entry found must match this group
 *	type_filter	if not null, entry found must match this type
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	TRUE		Match is good
 *	FALSE		match not good.
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
Cardinal		result;		/* return status */
IDBRecordNumber		record_number;	/* Record to be read in */
IDBRecordBufferPtr	bufptr;		/* buffer for data record */
IDBHeaderRecordPtr	recptr;		/* pointer data record */
IDBDataEntryHdrPtr	datahdr;	/* Header part of entry */
IDBDataPointer		entry_ptr;	/* to map onto data_entry parameter */


/*
 * Get the header record and get to the correct offset in
 * that record. Immediately decommit the buffer, since it won't be
 * compromised by this read.
 */
entry_ptr.pointer = data_entry;
record_number = entry_ptr.internal_id.rec_no;
if ( record_number != IDBHeaderRecordNumber )
    {
    Urm__UT_Error
	("Idb__HDR_MatchFilter", "not the header record",
	 file_id, NULL, MrmBAD_RECORD);
    return FALSE;
    }

result = Idb__BM_GetRecord (file_id, record_number, &bufptr);
if ( result != MrmSUCCESS ) return FALSE;
Idb__BM_Decommit (bufptr);

/*
 * Point to the header in the entry, and check the filters.
 */
recptr = (IDBHeaderRecordPtr) bufptr->IDB_record;
datahdr =
    (IDBDataEntryHdrPtr) &recptr->data[entry_ptr.internal_id.item_offs];
if (datahdr->validation != IDBDataEntryValid)
    {
    Urm__UT_Error
        ("Idb__HDR_GetDataEntry", "Invalid data entry",
        NULL, NULL, MrmNOT_VALID);
    return FALSE;
    }

if ( group_filter!=URMgNul && group_filter!=datahdr->resource_group )
    return FALSE;
if ( type_filter!=URMtNul && type_filter!=datahdr->resource_type )
    return FALSE;

return TRUE;

}

