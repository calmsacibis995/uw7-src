#ident	"@(#)ksh93:src/lib/libast/include/modex.h	1.1"
#pragma prototyped
/*
 * AT&T Bell Laboratories
 *
 * external mode_t representation support
 */

#ifndef _MODEX_H
#define _MODEX_H

#include <ast_fs.h>
#include <modecanon.h>

extern int		modei(int);
extern int		modex(int);

#if _S_IDPERM
#define modei(m)	((m)&X_IPERM)
#if _S_IDTYPE
#define modex(m)	(m)
#endif
#endif

#endif
