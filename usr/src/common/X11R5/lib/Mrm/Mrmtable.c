#pragma ident	"@(#)m1.2libs:Mrm/Mrmtable.c	1.1"
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
 *      UIL Manager (URM)
 *
 *  ABSTRACT:
 *
 *      This module contains the keyword tables and functions used by to
 *	support the compressing and uncompressing of strings in URM.
 *
 *--
 */


/*
 *
 *  INCLUDE FILES
 *
 */


#include <X11/Intrinsic.h>
#include <Mrm/MrmAppl.h>
#include <Mrm/Mrm.h>

/*
 *
 *  TABLE of CONTENTS
 *
 *	Urm__FixupCompressionTable	make table memory resident
 *
 *	Urm__FindClassDescriptor	find descriptor in file/memory
 *
 *	Urm__UncompressCode		uncompress a code
 *
 *	Urm__IsSubtreeResource		predicate for subtree resources
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



Cardinal Urm__FixupCompressionTable 
#ifndef _NO_PROTO
    (UidCompressionTablePtr	ctable,
    Boolean			qfindcldesc)
#else
(ctable, qfindcldesc)
    UidCompressionTablePtr	ctable;
    Boolean			qfindcldesc;
#endif

/*
 *++
 *
 *  FUNCTIONAL DESCRIPTION:
 *
 *	This routine fixes up a file-based compression table. It resolves
 *	offsets into memory pointers. If requested, it then looks up
 *	each resulting string and attempts to replace it with a
 *	function pointer (from the function hash table).
 *
 *  FORMAL PARAMETERS:
 *
 *	ctable		the compression table to fix up
 *	qfindcldesc	if TRUE, attempt to look up the strings as
 *			indexes of class descriptors
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *      none
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	if all fixup operations succeed
 *	MrmFAILURE	if any operation fails (usually function lookup)    
 *
 * SIDE EFFECTS:
 *
 *	none
 *
 *--
 */

{

/*
 *  Local variables
 */
int			fixndx ;	/* table fixup loop index */
Cardinal		result = MrmSUCCESS;
WCIClassDescPtr		cldesc;		/* for class descriptor */
Cardinal		clres;		/* lookup result */

/*
 * Fix up the table offsets to be pointers
 */
for ( fixndx=UilMrmMinValidCode ; fixndx<ctable->num_entries ; fixndx++ )
    ctable->entry[fixndx].cstring = 
	((char *)ctable+ctable->entry[fixndx].stoffset);
/*
 * Look up each string as a function if requested
 */
if ( qfindcldesc )
    for ( fixndx=UilMrmMinValidCode ; fixndx<ctable->num_entries ; fixndx++ )
	{
	clres = Urm__WCI_LookupClassDescriptor
	    (ctable->entry[fixndx].cstring, &cldesc);
	if ( clres == MrmSUCCESS )
	    ctable->entry[fixndx].cldesc = cldesc;
	else
	    {
	    ctable->entry[fixndx].cldesc = NULL;
	    result = MrmFAILURE;
	    }
	}

return result;

}    



Cardinal Urm__FindClassDescriptor 
#ifndef _NO_PROTO
    (IDBFile			cfile,
    MrmCode			code,
    char			*name,
    WCIClassDescPtr		*class_return)
#else
(cfile, code, name, class_return)
    IDBFile			cfile;
    MrmCode			code ;
    char			*name;
    WCIClassDescPtr		*class_return ;
#endif

/*
 *++
 *
 *  FUNCTIONAL DESCRIPTION:
 *
 *	This routine finds a class descriptor corresponding to a class
 *	compression code or name. It looks up the class in the file's
 *	compression table if possible. Otherwise, it uses the older built-in
 *	tables.
 *
 *  FORMAL PARAMETERS:
 *
 *	cfile		IDB file in which to find compression table
 *	code		compression code to be uncompressed
 *	name		the class name; the convenience function name.
 *	class_return	to return the class descriptor
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *      none
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	if class descriptor found
 *	MrmFAILURE	otherwise
 *
 * SIDE EFFECTS:
 *
 *	none
 *
 *--
 */

{

/*
 *  Local variables
 */

/*
 * Use the built-in tables if the file has none. Else a simple lookup.
 */
if ( code == UilMrmUnknownCode )
    return Urm__WCI_LookupClassDescriptor (name, class_return);
if ( cfile->class_ctable == NULL )
    return Urm__UT_Error
	("Urm__FindClassDescriptor",
	 "UID file is obsolete - has no compression table",
	 NULL, NULL, MrmFAILURE);
if ( code < UilMrmMinValidCode )
    return MrmFAILURE;
if ( code >= cfile->class_ctable->num_entries )
    return MrmFAILURE;
*class_return = cfile->class_ctable->entry[code].cldesc;
if ( *class_return == NULL )
    return MrmFAILURE;
return MrmSUCCESS;

}    



Cardinal Urm__UncompressCode 
#ifndef _NO_PROTO
    (IDBFile			cfile,
    MrmCode			code,
    String			*stg_return)
#else
(cfile, code, stg_return)
    IDBFile			cfile;
    MrmCode			code ;
    String			*stg_return ;
#endif

/*
 *++
 *
 *  FUNCTIONAL DESCRIPTION:
 *
 *	This routine returns the string corresponding to a compression code.
 *	It looks up the code in the file's compression table if it has
 *	one, else falls back on the old built-in tables. The code is looked
 *	up in the resource tables.
 *
 *  FORMAL PARAMETERS:
 *
 *	cfile		IDB file in which to find compression table
 *	code		compression code to be uncompressed
 *	stg_return	to return result of uncompression
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *      none
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	if uncompression successful
 *	MrmFAILURE	otherwise
 *
 * SIDE EFFECTS:
 *
 *	none
 *
 *--
 */

{

/*
 *  Local variables
 */


/*
 * A simple lookup in the file's compression tables.
 */
if ( cfile->resource_ctable == NULL )
    return Urm__UT_Error
	("Urm__UncompressCode",
	 "UID file is obsolete - has no compression table",
	 NULL, NULL, MrmFAILURE);
if ( code < UilMrmMinValidCode )
    return MrmFAILURE;
if ( code >= cfile->resource_ctable->num_entries )
    return MrmFAILURE;
*stg_return = cfile->resource_ctable->entry[code].cstring;
return MrmSUCCESS;

}    



Boolean Urm__IsSubtreeResource 
#ifndef _NO_PROTO
    (IDBFile			cfile,
    MrmCode			code)
#else
(cfile, code)
    IDBFile			cfile;
    MrmCode			code ;
#endif

/*
 *++
 *
 *  FUNCTIONAL DESCRIPTION:
 *
 *	This routine checks to see if a resource is marked as rooting a
 *	widget subtree, that is, is a resource which requires that a
 *	widget subtree be instantiated as its value.
 *
 *	Initial version uses crude check. To be replaced by table lookup.
 *
 *  FORMAL PARAMETERS:
 *
 *	cfile		file containing table information
 *	code		code to be checked
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *      none
 *
 *  FUNCTION VALUE:
 *
 *	TRUE		if subtree value is required
 *	FALSE		otherwise
 *
 * SIDE EFFECTS:
 *
 *	none
 *
 *--
 */

{

/*
 * Do a string comparison for for the subtree resources.
 */
if ( code < UilMrmMinValidCode )
    return FALSE;
if ( code >= cfile->resource_ctable->num_entries )
    return FALSE;

if ( strcmp(cfile->resource_ctable->entry[code].cstring,XmNsubMenuId) == 0 )
    return TRUE;
return FALSE;

}    


