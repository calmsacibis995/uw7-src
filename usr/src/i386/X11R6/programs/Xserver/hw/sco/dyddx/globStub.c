/*
 *	@(#) globStub.c 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1994.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *
 *	S000	Fri Sep 16 16:23:42 PDT 1994	mikep@sco.com
 *	- Add a few symbols to make static linking smoother.
 */
#include "dyddx.h"

DYNA_EXTERN(loadDynamicExtensions);

symbolDef globalSymbols[] = { 
{"dyddxload", dyddxload},
{"dyddxunload", dyddxunload},
{"loadDynamicExtensions", loadDynamicExtensions},
{0, 0}
};

int globalSymbolCount = 0;
