#ident	"@(#)variables.h	1.2"
#ifndef VARIABLES_H
#define VARIABLES_H


#include	"setupAPIs.h"		//  for setupObject_t definition




//  Functions in variables.C
extern void	getStringValue (setupObject_t *curObj, char **buff);
extern void	getIntValue (setupObject_t *curObj, char **buff);


#endif	// VARIABLES_H
