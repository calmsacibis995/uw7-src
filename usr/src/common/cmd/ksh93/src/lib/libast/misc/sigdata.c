#ident	"@(#)ksh93:src/lib/libast/misc/sigdata.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * signal name and text data
 */

#include <ast.h>
#include <sig.h>

#include "FEATURE/signal"

#if _DLL_INDIRECT_DATA && !_DLL
static Sig_info_t	sig_info_data =
#else
Sig_info_t		sig_info =
#endif

{ (char**)sig_name, (char**)sig_text, SIG_MAX };

#if _DLL_INDIRECT_DATA && !_DLL
Sig_info_t		sig_info = &sig_info_data;
#endif
