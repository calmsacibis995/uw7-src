#ident	"@(#)libc-i386:gen/lconstants.c	1.3"

#ifdef __STDC__
	#pragma weak lzero = _lzero
	#pragma weak lone = _lone
	#pragma weak lten = _lten
#endif
#include	"synonyms.h"
#include	<sys/types.h>
#include	<sys/dl.h>

dl_t	lzero	= { 0, 0};
dl_t	lone	= { 1, 0};
dl_t	lten	= {10, 0};
