#ident	"@(#)ksh93:src/lib/libast/dir/rewinddir.c	1.1"
#pragma prototyped
/*
 * rewinddir
 *
 * rewind directory stream
 * provided for POSIX compatibility
 */

#include "dirlib.h"

#if _dir_ok && ( defined(rewinddir) || _lib_rewinddir )

NoN(rewinddir)

#else

#undef	rewinddir

void
rewinddir(DIR* dirp)
{
	seekdir(dirp, 0L);
}

#endif
