#pragma ident	"@(#)m1.2libs:Mrm/Mrmtime.c	1.1"
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
 *	System-dependent routines dealing with time and dates.
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
#include <time.h>

/*
 *
 *  TABLE OF CONTENTS
 *
 *	Urm__UT_Time		- Get current time as a string
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




void Urm__UT_Time (time_stg)
    char			*time_stg ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine writes the current date/time string into a buffer
 *
 *  FORMAL PARAMETERS:
 *
 *	time_stg	location into which to copy time string. The
 *			length of this buffer should be at least
 *			URMhsDate1
 *
 *  IMPLICIT INPUTS:
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




#if __STDC__ 
time_t		timeval;
#else
long		timeval;
#endif /* __STDC__ */

time (&timeval);
strcpy (time_stg, ctime(&timeval));
}

