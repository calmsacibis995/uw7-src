#pragma comment(exestr, "@(#) error.c 12.1 95/05/09 SCOINC")

/************************************************************************* 
************************************************************************** 
**	Copyright (C) The Santa Cruz Operation, 1989-1993
**	This Module contains Proprietary Information of
**	The Santa Cruz Operation and should be treated as Confidential.
**
**
**  File Name:  	error.c        
**  ---------
**
**  Author:		Kyle Clark
**  ------
**
**  Creation Date:	21 November 1989
**  -------------
**
**  Overview:	
**  --------
**  implements error functions.
**
**  External Functions:
**  ------------------
** 
**  Data Structures:
**  ---------------
**
**  Bugs:
**  ----
**
*************************************************************************** 
***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "vidparse.h"


extern  nameStruct * currentStr;
static int warnings = NO;


/*************************************************************************
 *
 *  errexit()
 *
 *  Description:
 *  -----------
 *  bye!, too many errors
 *
 *  Arguments:
 *  ---------
 *  msg - message to print out.
 *
 *  Output:
 *  ------
 *
 *************************************************************************/

errexit(msg)

char *msg;

{
    fprintf(stderr, "FATAL ERROR: %s\n", msg);
    fflush(stderr);
    exit(1);
}





/*************************************************************************
 *
 *  warning()
 *
 *  Description:
 *  -----------
 *  Print warning message
 *
 *  Arguments:
 *  ---------
 *  msg - message to print
 *
 *  Output:
 *  ------
 *
 *************************************************************************/

warning(msg)

char *msg;

{
    fflush(stdout);

    if (currentStr->xgiFile != (char *)NULL)
        fprintf(stderr, "error in file %s:\n\t%s\n", currentStr->xgiFile, msg);
    else
        fprintf(stderr, "error: %s\n", msg);

    warnings = YES;

    fflush(stderr);
}






/*************************************************************************
 *
 *  errfile()
 *
 *  Description:
 *  -----------
 *  Stores name of current file being parsed
 *
 *  Arguments:
 *  ---------
 *
 *  Output:
 *  ------
 *
 *************************************************************************/

int
SetCurrentFile(name)

char *name;

{
    if (currentStr->xgiFile != (char *)NULL)
	free((char*)currentStr->xgiFile);

    if (name != (char *)NULL)
        currentStr->xgiFile = strdup(name);
    else
	currentStr->xgiFile = (char *)NULL;
}





/*************************************************************************
 *
 *  Warnings()
 *
 *  Description:
 *  -----------
 *  Returns YES if any warnings have been generated.
 *
 *  Arguments:
 *  ---------
 *
 *  Output:
 *  ------
 *
 *************************************************************************/

int
Warnings()

{
    return(warnings);
}
