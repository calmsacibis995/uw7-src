#ifndef _JBLEN
#ident	"@(#)sgs-head:i386/head/setjmp.h	1.9.5.7"

#if #machine(i860)
#define _JBLEN		22
#elif #machine(m68k)
#define _JBLEN		40
#elif #machine(m88k)
#define _JBLEN		24
#elif #machine(sparc)
#define _JBLEN		12
#else
#define _JBLEN		10
#endif

typedef int	jmp_buf[_JBLEN];

#if #machine(i386)
#define _SIGJBLEN	128	/* (sizeof(ucontext_t) / sizeof (int)) */
#elif #machine(i860)
#define _SIGJBLEN	137
#elif #machine(sparc)
#define _SIGJBLEN	19
#else
#define _SIGJBLEN	64
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int	setjmp(jmp_buf);
extern void	longjmp(jmp_buf, int);

extern int	_setjmp(jmp_buf);
extern void	_longjmp(jmp_buf, int);

#if __STDC__ - 0 == 0 || defined(_XOPEN_SOURCE) \
	|| defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE)

typedef int	sigjmp_buf[_SIGJBLEN];

extern int	sigsetjmp(sigjmp_buf, int);
extern void	siglongjmp(sigjmp_buf, int);

#endif /*__STDC__ - 0 == 0 || ...*/

#if __STDC__ - 0 != 0
#define setjmp(env)	setjmp(env)
#endif

#ifdef __cplusplus
}
#endif

#endif /*_JBLEN*/
