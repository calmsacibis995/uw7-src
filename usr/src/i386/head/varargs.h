#ifndef _VARARGS_H
#define _VARARGS_H

#ident	"@(#)sgs-head:i386/head/varargs.h	1.4.4.3"

#ifdef m88k
#include <sys/varargs.h>
#else	/* not m88k */

#ifndef _VA_LIST
#ifdef i860 
#	define _VA_LIST	struct _va_list
	struct _va_list
	{
		unsigned	_ireg_used;
		unsigned	_freg_used;
		long		*_reg_base;
		long		*_mem_ptr;
	};
#else
#	define _VA_LIST	void *
#endif
#endif /*_VA_LIST*/

#ifndef __VA_LIST
#   define __VA_LIST
	typedef _VA_LIST	va_list;
#endif

#ifdef	i860

#ifndef __HIGHC__
extern void  __i860_builtin_va_start(va_list *);
extern void *__builtin_va_arg();
#define va_start(ap) __i860_builtin_va_start(&ap)
#define va_arg(ap,type) (*(type *)__builtin_va_arg(&ap, (type *)0))
#define va_alist __builtin_va_alist
#define va_dcl int __builtin_va_alist;
#else	/* __HIGHC__ version */
/* the _va_alist-4 in the code below is a kludge and
   va_alist better not be in memory */
#define va_start(ap)					\
	{extern char _ADDRESS_OF_MEMOFLO_AREA[];	\
	extern long _ADDRESS_OF_INT_END_AREA[];		\
	extern int  _PARMBYTES_USEDI##_va_alist[],	\
		    _PARMBYTES_USEDF##_va_alist[],	\
		    _PARMBYTES_USEDM##_va_alist[];	\
	ap.ireg_used = ((int)_PARMBYTES_USEDI##_va_alist-4)>>2,\
	ap.freg_used = (int)_PARMBYTES_USEDF##_va_alist>>2,\
	ap.mem_ptr = (void *)&_va_alist,		\
	ap.mem_ptr = _ADDRESS_OF_MEMOFLO_AREA +		\
		(int)_PARMBYTES_USEDM##_va_alist;	\
	ap.reg_base = (int)_ADDRESS_OF_INT_END_AREA-80;	\
	}
#define va_arg(ap,type) \
	(*(type*) _va_arg(&ap,sizeof(type),_INFO(type,0),_INFO(type,2)))
#define va_alist _va_alist, ...
#define va_dcl int _va_alist;
extern void *_va_arg(va_list *ap, unsigned len, unsigned align, unsigned class);
#endif  /* HIGHC */


#else	/* i860 */

#if sparc
#define va_alist __builtin_va_alist
#endif /* sparc */

#define va_dcl int va_alist;

#if sparc
#define va_start(list) (void) (list = (va_list) &va_alist)
#else
#define va_start(list) list = (void *) &va_alist
#endif

#if sparc
#if defined(__STDC__)
#define va_arg(list, mode) ((mode *)__builtin_va_arg_incr((mode *)list))[0]
#else
#define va_arg(list, mode) ((mode *)(list = (char *)list + sizeof(mode)))[-1]
#endif
#else /* not sparc */
#define va_arg(list, mode)  (((mode *)(list = (char *)list + sizeof(mode)))[-1])
#endif
#endif	/* i860 */

#define va_end(list)

#endif	/* m88k */

#endif	/* _VARARGS_H */
