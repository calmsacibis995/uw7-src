#ident	"@(#)ksh93:src/lib/libast/misc/proclib.h	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * process library definitions
 */

#ifndef _PROCLIB_H
#define _PROCLIB_H

#include <errno.h>
#include <proc.h>
#include <sig.h>

#define proc_default	_proc_info_	/* hide external symbol		*/

extern Proc_t		proc_default;	/* first proc			*/

#ifndef errno
extern int		errno;
#endif

#endif
