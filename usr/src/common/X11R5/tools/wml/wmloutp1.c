#pragma ident	"@(#)mmisc:tools/wml/wmloutp1.c	1.1"
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
 * This is the standard output module for creating the UIL compiler
 * .h files.
 */


#include "wml.h"


void wmlOutput ()

{

/*
 * Output the .h files
 */
wmlOutputHFiles ();
if ( wml_err_count > 0 ) return;

/*
 * Output the keyword (token) tables
 */
wmlOutputKeyWordFiles ();
if ( wml_err_count > 0 ) return;

/*
 * Output the .mm files
 */
wmlOutputMmFiles ();
if ( wml_err_count > 0 ) return;

return;

}

