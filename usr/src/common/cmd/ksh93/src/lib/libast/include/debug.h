#ident	"@(#)ksh93:src/lib/libast/include/debug.h	1.1"
#pragma prototyped
/*
 * common ast debug definitions
 * include after the ast headers
 */

#ifndef _DEBUG_H
#define _DEBUG_H

#include <ast.h>
#include <error.h>

#if DEBUG || _TRACE_
#define debug(x)	x
#define libmessage(x)	do if (error_info.trace < 0) { liberror x; } while (0)
#define message(x)	do if (error_info.trace < 0) { error x; } while (0)
#else
#define debug(x)
#define libmessage(x)
#define message(x)
#endif

extern void		systrace(const char*);

#endif
