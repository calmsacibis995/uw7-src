#ident	"@(#)cplusfe:common/usr_include.c	1.2"

#include "paths.h"

#ifndef DEFAULT_USR_INCLUDE
#define DEFAULT_USR_INCLUDE INCDIR
#endif

/*
 * We initialize this global to its corresponding preprocessor symbol
 * value here rather than in EDG proprietary code.
 */

char *default_usr_include = DEFAULT_USR_INCLUDE;
