/*
 *	@(#) graferror.c 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989, 1990.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

#include "grafinfo.h"

#define UNKNOWN	"Unknown error number"

char *GrafErrorText[NUMERROR] = {
"All OK - no error",
"Memory Allocation error",
"Illegal mode string",
"Illegal format for field in grafdev",
"Illegal format for field in grafinfo.def",
"Can't open grafdev or grafinfo.def files",
"Unable to open GrafInfo file",
"Unable to get mode",
"Unable to parse file",				/* GEPARSE */
"Bad mode",
"Bad parse of section header",
"Bad register number",
"Can't open moninfo file",
"Bad syntax in monitor file",
"Illegal format of moninfo file",
"Can't open monitor file",
"No class in string list",
"Map class call failed"
};

char *grafError ()
{
	extern char *GetParseError ();
	if (graferrno == GEPARSE)
		return (GetParseError ());
	else if (graferrno < NUMERROR && graferrno >= 0)
		return (GrafErrorText[graferrno]);
	else
		return (UNKNOWN);

}
