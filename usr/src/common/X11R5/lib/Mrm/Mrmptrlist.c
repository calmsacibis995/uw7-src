#pragma ident	"@(#)m1.2libs:Mrm/Mrmptrlist.c	1.1"
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
 *	These routines manage a dynamic pointer list
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



Cardinal UrmPlistInit 
#ifndef _NO_PROTO
    (int		size,
    URMPointerListPtr	*list_id_return)
#else
(size, list_id_return)
    int			size ;
    URMPointerListPtr	*list_id_return ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine acquires and initializes a new pointer list.
 *
 *  FORMAL PARAMETERS:
 *
 *	size		number of pointer slots to allocate in list
 *	list_id_return	to return pointer to new list structure
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmFAILURE	allocation failure
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


/*
 * Allocate the list buffer and the slot vector buffer, and initialize
 */
*list_id_return = (URMPointerListPtr) XtMalloc (sizeof(URMPointerList)) ;
if ( *list_id_return == NULL )
    return Urm__UT_Error
        ("UrmPlistInit", "List allocation failed",
        NULL, NULL, MrmFAILURE) ;

(*list_id_return)->ptr_vec = (XtPointer *) XtMalloc (size*sizeof(XtPointer)) ;
if ( (*list_id_return)->ptr_vec == NULL )
    return Urm__UT_Error
        ("UrmPlistInit", "List vector allocation failed",
        NULL, NULL, MrmFAILURE) ;

(*list_id_return)->num_slots = size ;
(*list_id_return)->num_ptrs = 0 ;
return MrmSUCCESS ;

}



Cardinal UrmPlistResize 
#ifndef _NO_PROTO
    (URMPointerListPtr	list_id,
    int			size)
#else
(list_id, size)
    URMPointerListPtr	list_id ;
    int			size ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine reallocates the list vector in a pointer list in order
 *	to increase its size. The contents of the current list are copied
 *	into the new list. If the size parameter is smaller than the
 *	current buffer size, the request is ignored.
 *
 *  FORMAL PARAMETERS:
 *
 *	list_id		The pointer list to be resized
 *	size		The new number of pointer slots
 *
 *  IMPLICIT INPUTS:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmFAILURE	memory allocation failure
 *
 *  IMPLICIT OUTPUTS:
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
XtPointer			*newvec ;	/* new pointer slot vector */


/*
 * Allocate the new vector, and copy the current vector into it.
 */
newvec = (XtPointer *) XtMalloc (size*sizeof(XtPointer)) ;
if ( newvec == NULL )
    return Urm__UT_Error
        ("UrmPlistResize", "Vector re-allocation failed",
        NULL, NULL, MrmFAILURE) ;

if ( list_id->num_ptrs > 0 )
    UrmBCopy (list_id->ptr_vec, newvec, list_id->num_ptrs*sizeof(XtPointer)) ;

list_id->num_slots = size ;
list_id->ptr_vec = newvec ;
return MrmSUCCESS ;

}



Cardinal UrmPlistFree (list_id)
    URMPointerListPtr	list_id ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine frees the pointer vector and list structure.
 *
 *  FORMAL PARAMETERS:
 *
 *	list_id		The pointer list to be freed
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
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


XtFree ((char*)list_id->ptr_vec) ;
XtFree ((char*)list_id) ;
return MrmSUCCESS ;

}



Cardinal UrmPlistFreeContents (list_id)
    URMPointerListPtr	list_id ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine frees each of the items pointed to by the active
 *	pointers in the pointer list. The items must have been allocated
 *	with XtMalloc.
 *
 *  FORMAL PARAMETERS:
 *
 *	list_id		The pointer list
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operatoin succeeded
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
int			ndx ;		/* loop index */


for ( ndx=0 ; ndx<list_id->num_ptrs ; ndx++ )
    XtFree (list_id->ptr_vec[ndx]) ;
return MrmSUCCESS ;

}



Cardinal UrmPlistAppendPointer (list_id, ptr)
    URMPointerListPtr	list_id ;
    XtPointer		ptr ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine appends a pointer to the list. If no space remains,
 *	the list is resized to double its current size.
 *
 *  FORMAL PARAMETERS:
 *
 *	list_id		The pointer list
 *	ptr		The pointer to append
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmFAILURE	allocation failure
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
Cardinal		result ;	/* function results */


if ( list_id->num_ptrs == list_id->num_slots )
    {
    result = UrmPlistResize (list_id, 2*list_id->num_slots) ;
    if ( result != MrmSUCCESS ) return result ;
    }

list_id->ptr_vec[list_id->num_ptrs] = ptr ;
list_id->num_ptrs++ ;
return MrmSUCCESS ;

}



Cardinal UrmPlistAppendString (list_id, stg)
    URMPointerListPtr		list_id ;
    String			stg ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine allocates space for a string, copies, and appends the
 *	pointer to the string in the pointer list. All the strings in the
 *	list may be freed with UrmPlistFreeContents.
 *
 *  FORMAL PARAMETERS:
 *
 *	list_id		The pointer list
 *	stg		The string to alocate, copy, and append
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmFAILURE	allocation failure
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
String			newstg ;	/* allocated string */


newstg = XtMalloc (strlen(stg)+1) ;
if ( newstg == NULL )
    return Urm__UT_Error
        ("UrmPlistAppendString", "String allocation failed",
        NULL, NULL, MrmFAILURE) ;

strcpy (newstg, stg) ;
result = UrmPlistAppendPointer (list_id, newstg) ;
return result ;

}



MrmCount UrmPlistFindString (list_id, stg)
    URMPointerListPtr		list_id ;
    String			stg ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine searches a list (assumed to be a list of string
 *	pointers) for a case-sensitive match to a string. If found,
 *	its index in the list is found; else -1 is returned.
 *
 *  FORMAL PARAMETERS:
 *
 *	list_id		The pointer list
 *	stg		the (case-sensitive) string to be found
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	0-based index in list if found
 *	-1 if not found
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
MrmCount		ndx ;		/* search index */


for ( ndx=0 ; ndx<UrmPlistNum(list_id) ; ndx++ )
    if ( strcmp(stg,(String)UrmPlistPtrN(list_id,ndx)) == 0 )
        return ndx ;
return -1 ;

}

