#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/error.c	1.5"
#endif

/*
 *************************************************************************
 *
 * Description:
 *		The error diagnostic functions are held in this file.
 *	There are two types of error procedures and two warning procedures.
 *
 *		OlError()		- is a simple fatal error handler
 *		OlWarning()		- is a simple non-fatal error handler
 *		OlVaDisplayErrorMsg()	- Variable argument list fatal
 *					  error handler
 *		OlVaDisplayWarningMsg() - Variable argument list non-fatal
 *					  error handler
 *
 *******************************file*header*******************************
 */

						/* #includes go here	*/

#include <stdio.h>
#include <string.h>
#if defined(__STDC__) || defined(c_plusplus) || defined(__cplusplus)
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <X11/IntrinsicP.h>

#include "error.h"

