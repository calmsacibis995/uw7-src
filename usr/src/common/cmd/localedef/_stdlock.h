#ifndef _STDLOCK_H
#define _STDLOCK_H
#ident	"@(#)localedef:_stdlock.h	1.1"
/*
* stdlock.h - locking internal to libc.
*/

#include <sys/types.h>	/* for id_t */

typedef struct
{
	int	lock[2];	/* [0] is nonzero if locked; [1] is count */
} StdLock;

#ifdef _REENTRANT

#define STDLOCK(p)	_stdlock(p)
#define STDUNLOCK(p)	_stdunlock(p)
#define STDTRYLOCK(p)	_stdtrylock(p)
#define STDTRYUNLOCK(p)	_stdtryunlock(p)

#ifdef __STDC__
	extern void (*_libc_block)(int *);
	extern void (*_libc_unblock)(int *);
	extern id_t (*_libc_self)(void);

	extern void _stdlock(StdLock *);
	extern void _stdunlock(StdLock *);
	extern int _stdtrylock(StdLock *);
	extern void _stdtryunlock(StdLock *);
#else
	extern void (*_libc_block)();
	extern void (*_libc_unblock)();
	extern id_t (*_libc_self)();

	extern void _stdlock();
	extern void _stdunlock();
	extern int _stdtrylock();
	extern void _stdtryunlock();
#endif

#else /*!_REENTRANT*/

#define STDLOCK(p)
#define STDUNLOCK(p)
#define STDTRYLOCK(p)	0	/* success */
#define STDTRYUNLOCK(p)

#endif /*_REENTRANT*/

#endif /*_STDLOCK_H*/
