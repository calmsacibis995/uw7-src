#ident	"@(#)ksh93:src/lib/libast/include/magic.h	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * magic interface definitions
 */

#ifndef _MAGIC_H
#define _MAGIC_H

#include <ls.h>

#ifndef MAGIC_FILE
#define MAGIC_FILE	"lib/file/magic"
#endif

#define MAGIC_PHYSICAL	(1<<0)		/* don't follow symlinks	*/
#define MAGIC_STAT	(1<<1)		/* magictype stat already done	*/
#define MAGIC_VERBOSE	(1<<2)		/* verbose magic file errors	*/

#define MAGIC_USER	(1<<8)		/* first user flag bit		*/

typedef struct
{
	unsigned long	flags;		/* MAGIC_* flags		*/

#ifdef	_MAGIC_PRIVATE_
	_MAGIC_PRIVATE_
#endif

} Magic_t;

extern Magic_t*		magicopen(unsigned long);
extern int		magicload(Magic_t*, const char*, unsigned long);
extern int		magiclist(Magic_t*, Sfio_t*);
extern char*		magictype(Magic_t*, const char*, struct stat*);
extern void		magicclose(Magic_t*);

#endif
