#pragma ident	"@(#)m1.2libs:Mrm/MrmIindex.c	1.1"
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
 *	Index management routines
 *
 *  ABSTRACT:
 *
 *	These routines manage the index of an IDB file, including entering
 *	data entries accessed by index. These routines are read or common
 *	(used by both read and writing (MrmIindexw.c)).
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
 *	Idb__INX_ReturnItem		- Return the data entry for an index
 *
 *	Idb__INX_FindIndex		- Search the index
 *
 *	Idb__INX_SearchIndex		- Search a record for an index
 *
 *	Idb__INX_GetBTreeRecord		- Read a record in the B-tree
 *
 *	Idb__INX_FindResources		- Search the index for resources 
 *					  matching the filter
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
#define	Idb__INX_ValidLeaf(buffer) \
	(_IdbBufferRecordType(buffer)==IDBrtIndexLeaf)
#define	Idb__INX_ValidNode(buffer) \
	(_IdbBufferRecordType(buffer)==IDBrtIndexNode)
#define	Idb__INX_ValidRecord(buffer) \
	(_IdbBufferRecordType(buffer)==IDBrtIndexLeaf ||  \
	 _IdbBufferRecordType(buffer)==IDBrtIndexNode)


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




Cardinal Idb__INX_ReturnItem (file_id, index, data_entry)
    IDBFile			file_id ;
    char			*index ;
    IDBDataHandle		*data_entry ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__INX_ReturnItem locates a data entry in the file, and returns
 *	the data entry pointer (without reading the data record).
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		Open IDB file in which to write entry
 *	index		The entry's case-sensitive index
 *	data_entry	To return data entry pointer for data
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
IDBRecordBufferPtr	bufptr ;	/* buffer containing entry */
MrmCount		entndx ;	/* entry index */
IDBIndexLeafRecordPtr	leafrec ;	/* index leaf record */
IDBIndexNodeRecordPtr	noderec ;	/* index node record */

/*
 * Attempt to find the index
 */
result = Idb__INX_FindIndex (file_id, index, &bufptr, &entndx) ;
switch ( result )
    {
    case MrmINDEX_GT:
    case MrmINDEX_LT:
        return MrmNOT_FOUND ;
    case MrmSUCCESS:
        break ;
    default:
        return result ;
    }

/*
 * Point into the buffer, and retrieve the data pointer
 */
switch ( _IdbBufferRecordType (bufptr) )
    {
    case IDBrtIndexLeaf:
        leafrec = (IDBIndexLeafRecordPtr) bufptr->IDB_record ;
        *data_entry = leafrec->index[entndx].data.pointer ;
        return MrmSUCCESS ;
    case IDBrtIndexNode:
        noderec = (IDBIndexNodeRecordPtr) bufptr->IDB_record ;
	*data_entry = noderec->index[entndx].data.pointer ;
        return MrmSUCCESS ;
    default:
        return Urm__UT_Error
            ("Idb__INX_ReturnItem", "Unexpected record type",
            file_id, NULL, MrmBAD_RECORD) ;
    }


}



Cardinal Idb__INX_FindIndex (file_id, index, buffer_return, index_return)
    IDBFile			file_id ;
    char			*index ;
    IDBRecordBufferPtr		*buffer_return ;
    MrmCount			*index_return ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__INX_FindIndex finds the index record containing an index entry,
 *	and returns the buffer containing that record. It is used both as the
 *	low-level routine for locating an index for retrieving a data entry,
 *	and for locating the record in which a new index should be inserted.
 *	Thus the interpretation of the return code is:
 *
 *	MrmSUCCESS	found the index, the index record is in the buffer
 *			and the index_return locates the entry
 *	MrmINDEX_GT	buffer contains the leaf index record which should
 *	MrmINDEX_LT	contain the index, and index_return locates the entry
 *			in the buffer at which search terminated. The result
 *			value indicates how the given index orders against
 *			the entry in index_return.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		Open IDB file in which to find index
 *	index		Case-sensitive index string
 *	buffer_return	To return pointer to buffer containing index record
 *	index_return	To return item's index in the records index vector
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmINDEX_GT	index not found, but orders greater-than entry at
 *			index_return
 *	MrmINDEX_LT	index not found, but orders less-than entry at
 *			index_return
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

/*
 * Initialize search at the root of the index, then continue searching
 * until either the index is found or search terminates at some leaf record.
 */
if ( file_id->index_root == NULL ) return MrmFAILURE ;
result = Idb__BM_GetRecord (file_id, file_id->index_root, buffer_return) ;
if ( result != MrmSUCCESS ) return result ;
if ( ! Idb__INX_ValidRecord(*buffer_return) )
    return Urm__UT_Error
        ("Idb__INX_FindIndex", "Unexpected record type",
        file_id, NULL, MrmBAD_RECORD) ;

do  {
    result =
        Idb__INX_SearchIndex (file_id, index, *buffer_return, index_return) ;
    if ( _IdbBufferRecordType(*buffer_return) == IDBrtIndexLeaf) return result ;
    switch ( result )
        {
        case MrmINDEX_GT:
        case MrmINDEX_LT:
            result = Idb__INX_GetBtreeRecord
                (file_id, buffer_return, *index_return, result) ;
	    if (result != MrmSUCCESS )
		{
		if (result == MrmNOT_FOUND)
		    result = MrmEOF;
		return result ;
		}
            break ;
        default:
            return result ;
        }
    } while ( TRUE ) ;

}



Cardinal Idb__INX_SearchIndex (file_id, index, buffer, index_return)
    IDBFile			file_id ;
    char			*index ;
    IDBRecordBufferPtr		buffer ;
    MrmCount			*index_return ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Idb__INX_SearchIndex searches a record for an index. The record
 *	may be either a leaf or a node record. If the index is found,
 *	index_return is its entry in the records index vector. If it is not
 *	found, then index_return locates the entry in the record at which
 *	search terminated.
 *
 *	Thus the interpretation of the return code is:
 *
 *	MrmSUCCESS	found the index, and the index_return locates the entry
 *	MrmINDEX_GT	index orders greater-than the entry at index_return
 *	MrmINDEX_LT	index orders less-than the entry at index_return
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		Open IDB file in which to find index
 *	index		Case-sensitive index string
 *	buffer		Buffer containing record to be searched
 *	index_return	To return item's index in the records index vector
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmINDEX_GT	index not found, but orders greater-than entry at
 *			index_return
 *	MrmINDEX_LT	index not found, but orders less-than entry at
 *			index_return
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
MrmType			buftyp ;	/* buffer type */
IDBIndexLeafRecordPtr	leafrec ;	/* index leaf record */
IDBIndexLeafHdrPtr	leafhdr ;	/* index leaf header */
IDBIndexNodeRecordPtr	noderec ;	/* index node record */
IDBIndexNodeHdrPtr	nodehdr ;	/* index node header */
IDBIndexLeafEntryPtr	leaf_ndxvec ;	/* index leaf entry vector */
IDBIndexNodeEntryPtr	node_ndxvec ;	/* index node entry vector */
MrmCount		ndxcnt ;	/* number of entries in vector */
char			*stgbase ;	/* base adddress for string offsets */
int			lowlim ;	/* binary search lower limit index */
int			uprlim ;	/* binary search upper limit index */
char			*ndxstg ;	/* pointer to current index string */
int			cmpres ;	/* strncmp result */


/*
 * Set up search pointers based on the record type
 */
buftyp = _IdbBufferRecordType (buffer) ;
switch ( buftyp )
    {
    case IDBrtIndexLeaf:
        leafrec = (IDBIndexLeafRecordPtr) buffer->IDB_record ;
        leafhdr = (IDBIndexLeafHdrPtr) &leafrec->leaf_header ;
        leaf_ndxvec = leafrec->index ;
        ndxcnt = leafhdr->index_count ;
	stgbase = (char *) leafrec->index ;
	break ;
    case IDBrtIndexNode:
        noderec = (IDBIndexNodeRecordPtr) buffer->IDB_record ;
        nodehdr = (IDBIndexNodeHdrPtr) &noderec->node_header ;
        node_ndxvec = noderec->index ;
        ndxcnt = nodehdr->index_count ;
	stgbase = (char *) noderec->index ;
	break ;
    default:
        return Urm__UT_Error
            ("Idb__INX_SearchIndex", "Unexpected record type",
            file_id, NULL, MrmBAD_RECORD) ;
    }

/*
 * Search the index vector for the given index (binary search)
 */
Idb__BM_MarkActivity (buffer) ;
for ( lowlim=0,uprlim=ndxcnt-1 ; lowlim<=uprlim ; )
    {
    *index_return = (lowlim+uprlim) / 2 ;
    ndxstg = (buftyp==IDBrtIndexLeaf) ?
        (char *) stgbase + leaf_ndxvec[*index_return].index_stg :
        (char *) stgbase + node_ndxvec[*index_return].index_stg ;
    cmpres = strncmp (index, ndxstg, IDBMaxIndexLength) ;
    if ( cmpres == 0 ) return MrmSUCCESS ;
    if ( cmpres < 0 ) uprlim = *index_return - 1 ;
    if ( cmpres > 0 ) lowlim = *index_return + 1 ;
    }

/*
 * Not found, result determined by final ordering.
 */
return (cmpres>0) ? MrmINDEX_GT : MrmINDEX_LT ;

}



Cardinal Idb__INX_GetBtreeRecord
#ifndef _NO_PROTO
    ( IDBFile			file_id,
    IDBRecordBufferPtr		*buffer_return,
    MrmCount			entry_index,
    Cardinal			order)
#else
        (file_id, buffer_return, entry_index, order)
    IDBFile			file_id ;
    IDBRecordBufferPtr		*buffer_return ;
    MrmCount			entry_index ;
    Cardinal			order ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine reads in the next level index record in the B-tree
 *	associated with some entry in the current record (i.e. the one
 *	currently contained in the buffer). The buffer pointer is reset.
 *	The order variable indicates which record to read:
 *		MrmINDEX_GT - read the record ordering greater-than the entry
 *		MrmINDEX_LT - read the record ordering less-than the entry
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		Open IDB file from which to read record
 *	buffer_return	points to current buffer; reset to buffer read in
 *	entry_index	entry in current buffer to use as reference
 *	order		MrmINDEX_GT for GT ordered record, else MrmINDEX_LT
 *			for LT ordered record.
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmBAD_ORDER	Order variable has illegal value
 *	MrmBAD_RECORD	new record not an index record
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
IDBIndexNodeRecordPtr	recptr ;	/* node record in buffer */
IDBIndexNodeHdrPtr	hdrptr ;	/* record header */
IDBRecordNumber		recno ;		/* Record number to read in */

/*
 * Set buffer pointers
 */
recptr = (IDBIndexNodeRecordPtr) (*buffer_return)->IDB_record ;
hdrptr = (IDBIndexNodeHdrPtr) &recptr->node_header ;

/*
 * Retrieve the record number
 */
switch ( order )
    {
    case MrmINDEX_GT:
        recno = recptr->index[entry_index].GT_record ;
        break ;
    case MrmINDEX_LT:
        recno = recptr->index[entry_index].LT_record ;
        break ;
    default:
        return Urm__UT_Error
            ("Idb__INX_GetBTreeRecord", "Unexpected record type",
            file_id, NULL, MrmBAD_ORDER) ;
    }

/*
 * Retrieve and sanity check the record
 */
result = Idb__BM_GetRecord (file_id, recno, buffer_return) ;
if ( result != MrmSUCCESS ) return result ;
if ( ! Idb__INX_ValidRecord(*buffer_return) )
    return Urm__UT_Error
        ("Idb__INX_GetBTreeRecord", "Unexpected record type",
        file_id, NULL, MrmBAD_RECORD) ;

/*
 * Record successfully retrieved
 */
return MrmSUCCESS ;

}



Cardinal Idb__INX_FindResources
#ifndef _NO_PROTO
    (IDBFile			file_id,
    IDBRecordNumber		recno,
    MrmGroup			group_filter,
    MrmType			type_filter,
    URMPointerListPtr		index_list)
#else
        (file_id, recno, group_filter, type_filter, index_list)
    IDBFile			file_id ;
    IDBRecordNumber		recno ;
    MrmGroup			group_filter ;
    MrmType			type_filter ;
    URMPointerListPtr		index_list ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This is the internal routine which searches the database for
 *	indexed resources matching a filter. It starts at the current node,
 *	then recurses down the BTree inspecting every entry. Each entry
 *	which matches the filter is appended to the index list.
 *
 *  FORMAL PARAMETERS:
 *
 *	file_id		The IDB file id returned by XmIdbOpenFile
 *	recno		The record to be searched. If a node entry,
 *			then each pointed-to record is also searched.
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
IDBRecordBufferPtr	bufptr ;	/* buffer containing entry */
int			entndx ;	/* entry loop index */
IDBIndexLeafRecordPtr	leafrec ;	/* index leaf record */
IDBIndexLeafHdrPtr	leafhdr ;	/* index leaf header */
IDBIndexNodeRecordPtr	noderec ;	/* index node record */
IDBIndexNodeHdrPtr	nodehdr ;	/* index node header */
IDBIndexLeafEntryPtr	leaf_ndxvec ;	/* index leaf entry vector */
IDBIndexNodeEntryPtr	node_ndxvec ;	/* index node entry vector */
MrmCount		ndxcnt ;	/* number of entries in vector */
char			*stgbase ;	/* base adddress for string offsets */



/*
 * Read the record in, then bind pointers and process the record.
 */
result = Idb__BM_GetRecord (file_id, recno, &bufptr) ;
if ( result != MrmSUCCESS ) return result ;

switch ( _IdbBufferRecordType (bufptr) )
    {

/*
 * Simply apply the filter to all entries in the leaf record
 */
    case IDBrtIndexLeaf:
        leafrec = (IDBIndexLeafRecordPtr) bufptr->IDB_record ;
        leafhdr = (IDBIndexLeafHdrPtr) &leafrec->leaf_header ;
        leaf_ndxvec = leafrec->index ;
        ndxcnt = leafhdr->index_count ;
	stgbase = (char *) leafrec->index ;

        for ( entndx=0 ; entndx<ndxcnt ; entndx++ )
            {
            if ( Idb__DB_MatchFilter
                    (file_id, leaf_ndxvec[entndx].data.pointer,
                    group_filter, type_filter) )
                UrmPlistAppendString (index_list,
                    stgbase+leaf_ndxvec[entndx].index_stg) ;
            Idb__BM_MarkActivity (bufptr) ;
            }
        return MrmSUCCESS ;

/*
 * Process the first LT record, then process each index followed by
 * its GT record. This will produce a correctly ordered list. The
 * record is read again, and all pointers bound, after each FindResources
 * call in order to guarantee that buffer turning has not purged the
 * current record from memory
 */
    case IDBrtIndexNode:
        noderec = (IDBIndexNodeRecordPtr) bufptr->IDB_record ;
        nodehdr = (IDBIndexNodeHdrPtr) &noderec->node_header ;
        node_ndxvec = noderec->index ;
        ndxcnt = nodehdr->index_count ;
	stgbase = (char *) noderec->index ;
        result = Idb__INX_FindResources
            (file_id, node_ndxvec[0].LT_record,
            group_filter, type_filter, index_list) ;
        if ( result != MrmSUCCESS ) return result ;

        for ( entndx=0 ; entndx<ndxcnt ; entndx++ )
            {
            Idb__BM_GetRecord (file_id, recno, &bufptr) ;
            noderec = (IDBIndexNodeRecordPtr) bufptr->IDB_record ;
            nodehdr = (IDBIndexNodeHdrPtr) &noderec->node_header ;
            node_ndxvec = noderec->index ;
            stgbase = (char *) noderec->index ;
            if ( Idb__DB_MatchFilter
                    (file_id, node_ndxvec[entndx].data.pointer,
                    group_filter, type_filter) )
                UrmPlistAppendString (index_list,
                    stgbase+node_ndxvec[entndx].index_stg) ;
            result = Idb__INX_FindResources
                (file_id, node_ndxvec[entndx].GT_record,
                group_filter, type_filter, index_list) ;
            if ( result != MrmSUCCESS ) return result ;
            }
        return MrmSUCCESS ;

    default:
        return Urm__UT_Error
            ("Idb__INX_FindResources", "Unexpected record type",
            file_id, NULL, MrmBAD_RECORD) ;
    }

}

