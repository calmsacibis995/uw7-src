#ident	"@(#)ksh93:src/lib/libast/misc/univdata.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * universe common data
 */

#include "univlib.h"

#ifndef UNIV_MAX

char		univ_env[] = "__UNIVERSE__";

#else

#ifndef NUMUNIV

#if !_lib_universe
#undef	U_GET
#endif

#ifdef	U_GET
char*		univ_name[] = { "ucb", "att" };
#else
char*		univ_name[] = { "att", "ucb" };
#endif

int		univ_max = sizeof(univ_name) / sizeof(univ_name[0]);

#endif

char		univ_cond[] = "$(UNIVERSE)";

int		univ_size = sizeof(univ_cond) - 1;

#endif
