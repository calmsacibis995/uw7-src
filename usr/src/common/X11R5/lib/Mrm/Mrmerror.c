#pragma ident	"@(#)m1.2libs:Mrm/Mrmerror.c	1.1"
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
 *	All error signalling and handling routines are found here.
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
externaldef(urm__err_out)		MrmCode	urm__err_out = URMErrOutStdout ;
externaldef(urm__latest_error_code)	MrmCode	urm__latest_error_code = 0 ;
externaldef(urm__latest_error_msg)	String	urm__latest_error_msg = NULL ;

/*
 *
 *  OWN VARIABLE DECLARATIONS
 *
 */
#ifdef _NO_PROTO
String Urm__UT_UrmCodeString ();
#endif

static	String	urm_codes_codstg[] = {
    "MrmFAILURE"
    ,"MrmSUCCESS"
    ,"MrmNOT_FOUND"
    ,"MrmCREATE_NEW"
    ,"MrmEXISTS"
    ,"URMIndex retry"
    ,"MrmNUL_GROUP"
    ,"MrmINDEX_GT"
    ,"MrmNUL_TYPE"
    ,"MrmINDEX_LT"
    ,"MrmWRONG_GROUP"
    ,"MrmPARTIAL_SUCCESS"
    ,"MrmWRONG_TYPE"
    ,"URM unused code 13"
    ,"MrmOUT_OF_RANGE"
    ,"URM unused code 15"
    ,"MrmBAD_RECORD"
    ,"URM unused code 17"
    ,"MrmNULL_DATA"
    ,"URM unused code 19"
    ,"MrmBAD_DATA_INDEX"
    ,"URM unused code 21"
    ,"MrmBAD_ORDER"
    ,"URM unused code 23"
    ,"MrmBAD_CONTEXT"
    ,"URM unused code 25"
    ,"MrmNOT_VALID"
    ,"URM unused code 27"
    ,"MrmBAD_BTREE"
    ,"URM unused code 29"
    ,"MrmBAD_WIDGET_REC"
    ,"URM unused code 31"
    ,"MrmBAD_CLASS_TYPE"
    ,"URM unused code 33"
    ,"MrmNO_CLASS_NAME"
    ,"URM unused code 35"
    ,"MrmTOO_MANY"
    ,"URM unused code 37"
    ,"MrmBAD_IF_MODULE"
    ,"URM unused code 39"
    ,"MrmNULL_DESC"
    ,"URM unused code 41"
    ,"MrmOUT_OF_BOUNDS"
    ,"URM unused code 43"
    ,"MrmBAD_COMPRESS"
    ,"URM unused code 45"
    ,"MrmBAD_ARG_TYPE"
    ,"URM unused code 47"
    ,"MrmNOT_IMP"
    ,"URM unused code 49"
    ,"MrmNULL_INDEX"
    ,"URM unused code 51"
    ,"MrmBAD_KEY_TYPE"
    ,"URM unused code 53"
    ,"MrmBAD_CALLBACK"
    ,"URM unused code 55"
    ,"MrmNULL_ROUTINE"
    ,"URM unused code 57"
    ,"MrmVEC_TOO_BIG"
    ,"URM unused code 59"
    ,"MrmBAD_HIERARCHY"
    ,"URM unused code 61"
    ,"MrmBAD_CLASS_CODE"
    } ;

static String urm_codes_invalidcode = "Invalid URM code" ;




Cardinal Urm__UT_Error (module, error, file_id, context_id, status)
    char			*module ;
    char			*error ;
    IDBFile			file_id ;
    URMResourceContextPtr	context_id ;
    Cardinal			status ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine is an error signalling routine for use within URM.
 *	It currently just reports the error on the terminal.
 *
 *  FORMAL PARAMETERS:
 *
 *	module		Identifies the module (routine) detecting the error
 *	error		Brief description of the error
 *	file_id		if not NULL, the IDB file implicated in the error
 *	context_id	if not NULL, the URM resource implicated in the error
 *	status		the return code associated with the error.
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

char		msg[300] ;	/* error message */

/*
 * construct error message
 */

/* Old form
sprintf (msg, "%s detected error %s - %s", module, error,
    Urm__UT_UrmCodeString(status)) ;
 */
sprintf (msg, "%s: %s - %s", module, error, Urm__UT_UrmCodeString(status)) ;

/*
 * Print or save the message depending on the reporting style
 */
urm__latest_error_code = status ;

switch ( urm__err_out )
    {
    case URMErrOutMemory:
        if ( urm__latest_error_msg != NULL )
            XtFree (urm__latest_error_msg) ;
        urm__latest_error_msg = (String) XtMalloc (strlen(msg)+1) ;
        strcpy (urm__latest_error_msg, msg) ;
        return status ;
    case URMErrOutStdout:
    default:
        XtWarning (msg) ;
        return status ;
    }

}



Cardinal Urm__UT_SetErrorReport 

#ifndef _NO_PROTO
	(MrmCode report_type)
#else
	(report_type)
    MrmCode			report_type ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine sets the URM error report type to a standard state
 *
 *  FORMAL PARAMETERS:
 *
 *	report_type	URMErrOutMemory	- save message in memory, don't print
 *			URMErrOutStdout	- report to stdout
 *
 *  IMPLICIT INPUTS:
 *
 *	urm__err_out
 *
 *  IMPLICIT OUTPUTS:
 *
 *	urm__err_out
 *
 *  FUNCTION VALUE:
 *
 *      MrmSUCCESS	operation succeeded
 *	MrmFAILURE	illegal state requested
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
switch ( report_type )
    {
    case URMErrOutMemory:
    case URMErrOutStdout:
        urm__err_out = report_type ;
        return MrmSUCCESS ;
    default:
        return MrmFAILURE ;
    }

}



MrmCode Urm__UT_LatestErrorCode ()

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	 Returns the current error code
 *
 *  FORMAL PARAMETERS:
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

return urm__latest_error_code ;

}



String Urm__UT_LatestErrorMessage ()

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	 Returns the current error message
 *
 *  FORMAL PARAMETERS:
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

return urm__latest_error_msg ;

}



String Urm__UT_UrmCodeString 

#ifndef _NO_PROTO
	(MrmCode cod)
#else
	(cod)
    MrmCode		cod ;
#endif
/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine returns a static string naming a URM return code.
 *
 *  FORMAL PARAMETERS:
 *
 *	cod		A URM return code
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
if ( cod>=MrmFAILURE && cod<=MrmBAD_CLASS_CODE )
    return urm_codes_codstg[cod] ;
return urm_codes_invalidcode ;

}

