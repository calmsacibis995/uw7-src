#ident	"@(#)acomp:common/target.h	52.8"
/* target.h */

/* Definitions for numeric limits and other characteristics
** in the target machine.  The names here deliberately mimic
** those in limits.h.
*/

/* These values are suitable for a machine with:
**	32 bit ints/longs
**	64 bit long longs
**
** We assume that the 32-bit values (thus through [unsigned] long)
** can be handled natively.
*/

#define	T_SCHAR_MIN	-128
#define	T_SCHAR_MAX	127
#define	T_UCHAR_MAX	255
#define	T_SHRT_MIN	-32768
#define	T_SHRT_MAX	32767
#define	T_USHRT_MAX	65535

#define	T_INT_MIN	(-2147483647-1)
#define	T_INT_MAX	2147483647
#define	T_UINT_MAX	4294967295u

#ifndef T_LONG_MIN
#define	T_LONG_MIN	(-2147483647-1)
#endif
#ifndef T_LONG_MAX
#define	T_LONG_MAX	2147483647L
#endif
#ifndef T_ULONG_MAX
#define	T_ULONG_MAX	4294967295ul
#endif

#define T_LLONG_MIN	0x8000000000000000
#define T_LLONG_MAX	0x7fffffffffffffff
#define T_ULLONG_MAX	0xffffffffffffffff

#define	T_ptrdiff_t	TY_INT		/* type for pointer differences */
#define	T_ptrtype	TY_INT		/* integral type equivalent to ptr */
#define	T_size_t	TY_UINT		/* type for sizeof() */
#define	T_wchar_t	TY_LONG		/* type for wide characters */
#define	T_UWCHAR_MAX	T_ULONG_MAX	/* maximum unsigned wchar_t value */
