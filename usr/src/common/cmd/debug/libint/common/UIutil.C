#ident	"@(#)debugger:libint/common/UIutil.C	1.1"

// This file contains utility functions that are used by both
// the debugger side and the ui side.

#include "Severity.h"
#include "UIutil.h"

#include <stdio.h>
#include <unistd.h>

static const char *istrings[E_FATAL+1];
static const char *strings[E_FATAL+1] =
{
	0,
	"Warning: ",
	"Error: ",
	"Fatal error: "
};

const char *
get_label(Severity sev)
{
	char catid[sizeof(CATALOG) + 3]; // 3 for ':' digit '\0'

	switch (sev)
	{
		// Get the translated string from the catalog
		// this relies on the severity strings being at the
		// beginning of the catalog
		case E_WARNING:
		case E_ERROR:
		case E_FATAL:
			if (!istrings[sev])
			{
				sprintf(catid, "%s:%d", CATALOG, sev);
				istrings[sev] = gettxt(catid, strings[sev]);
			}
			return istrings[sev];

		case E_NONE:
		default:
			return "";
	}
}
