#pragma ident	"@(#)m1.2libs:Mrm/Mrmvm.c	1.1"
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
 *      UIL Resource Manager (URM)
 *
 *  ABSTRACT:
 *
 *      This module handles memory management for URM.
 *
 *  AUTHORS:
 *
 *  MODIFICATION HISTORY:
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
 *	Urm__UT_AllocString	- allocates and copies a nul-terminated string
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


String Urm__UT_AllocString (stg)
    String		stg ;
/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This utility allocates memory for a string from the miscellaneous
 *	zone and copies the string into it.
 *
 *  FORMAL PARAMETERS:
 *
 *	stg		null-terminated string to copy
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	pointer to the new string; NULL if an allocation error
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
String			new_stg ;	/* the string copy */


if ( stg == NULL ) return NULL ;
new_stg = (String) XtMalloc (strlen(stg)+1) ;
if ( new_stg != NULL )
    strcpy (new_stg, stg) ;
return new_stg ;


}



