#ident	"@(#)ksh93:src/lib/libast/include/sfdisc.h	1.1"
#pragma prototyped
/*
 * AT&T Bell Laboratories
 *
 * sfio discipline interface definitions
 */

#ifndef _SFDISC_H
#define _SFDISC_H

#include <ast.h>

/*
 * %(...) printf support
 */

typedef int (*Sf_key_lookup_t)(void*, const char*, const char*, int, char**, long*);
typedef char* (*Sf_key_convert_t)(void*, const char*, const char*, int, char*, long);

extern int		sfkeyprintf(Sfio_t*, void*, const char*, Sf_key_lookup_t, Sf_key_convert_t);

/*
 * slow io exception discipline
 */

extern int		sfslowio(Sfio_t*);

#endif
