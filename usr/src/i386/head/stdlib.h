#ifndef _STDLIB_H
#define _STDLIB_H
#ident	"@(#)sgs-head:i386/head/stdlib.h	1.39.1.6"

typedef	struct
{
	int	quot;
	int	rem;
} div_t;

typedef struct
{
	long	quot;
	long	rem;
} ldiv_t;

#ifndef _SIZE_T
#   define _SIZE_T
	typedef unsigned int	size_t;
#endif

#if __STDC__ - 0 == 0 && !defined(_SSIZE_T)
#   define _SSIZE_T
	typedef int	ssize_t;
#endif

#ifndef _WCHAR_T
#   define _WCHAR_T
	typedef long	wchar_t;
#endif

#ifndef NULL
#   define NULL	0
#endif

#define EXIT_FAILURE	1
#define EXIT_SUCCESS	0
#define RAND_MAX	32767

#if defined(_XOPEN_SOURCE) || __STDC__ - 0 == 0 \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

#ifndef WUNTRACED
#   define WUNTRACED	  0004
#   define WNOHANG	  0100
#   define WIFEXITED(x)	  (((int)((x)&0377))==0)
#   define WIFSIGNALED(x) (((int)((x)&0377))>0&&((int)(((x)>>8)&0377))==0)
#   define WIFSTOPPED(x)  (((int)((x)&0377))==0177&&((int)(((x)>>8)&0377))!=0)
#   define WEXITSTATUS(x) ((int)(((x)>>8)&0377))
#   define WTERMSIG(x)	  (((int)((x)&0377))&0177)
#   define WSTOPSIG(x)	  ((int)(((x)>>8)&0377))
#endif /*WUNTRACED*/

#endif /*defined(_XOPEN_SOURCE) || ...*/

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char	__ctype[];

#define MB_CUR_MAX	((int)__ctype[520])

extern double	atof(const char *);
extern int	atoi(const char *);
extern long	atol(const char *);
extern double	strtod(const char *, char **);
	/*LINTED*/
extern float	strtof(const char *, char **);
extern long	strtol(const char *, char **, int);
long double	strtold(const char *, char **);
long long	strtoll(const char *, char **, int);
unsigned long	strtoul(const char *, char **, int);
unsigned long long strtoull(const char *, char **, int);

extern int	rand(void);
extern void	srand(unsigned int);

extern void	*calloc(size_t, size_t);
extern void	free(void *);
extern void	*malloc(size_t);
extern void	*realloc(void *, size_t);

extern void	abort(void);
extern int	atexit(void (*)(void));
extern void	exit(int);
extern char	*getenv(const char *);
extern int	system(const char *);

extern void	*bsearch(const void *, const void *, size_t, size_t,
			int (*)(const void *, const void *));
extern void	qsort(void *, size_t, size_t,
			int (*)(const void *, const void *));

extern int	abs(int);
extern div_t	div(int, int);
extern long	labs(long);
extern ldiv_t	ldiv(long, long);

extern int	mbtowc(wchar_t *, const char *, size_t);
extern int	mblen(const char *, size_t);
extern int	wctomb(char *, wchar_t);

extern size_t	mbstowcs(wchar_t *, const char *, size_t);
extern size_t	wcstombs(char *, const wchar_t *, size_t);

#if defined(_XOPEN_SOURCE) || __STDC__ == 0 \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

extern double	drand48(void);
extern double	erand48(unsigned short *);
extern long	jrand48(unsigned short *);
extern void	lcong48(unsigned short *);
extern long	lrand48(void);
extern long	mrand48(void);
extern long	nrand48(unsigned short *);
extern int	putenv(char *);
unsigned short	*seed48(unsigned short *);
extern void	setkey(const char *);
extern void	srand48(long);

#endif

#if __STDC__ - 0 == 0 && !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

#ifndef _MALLINFO
#define _MALLINFO
struct mallinfo {
	size_t	arena;		/* total space in arena */
	size_t	ordblks;	/* number of ordinary blocks */
	size_t	smblks;		/* number of small blocks */
	size_t	hblks;		/* number of holding blocks */
	size_t	hblkhd;		/* space in holding block headers */
	size_t	usmblks;	/* space in small blocks in use */
	size_t	fsmblks;	/* space in free small blocks */
	size_t	uordblks;	/* space in ordinary blocks in use */
	size_t	fordblks;	/* space in free ordinary blocks */
	size_t	keepcost;	/* cost of enabling keep option */
};
#endif

extern long	a64l(const char *);
extern int	dup2(int, int);
extern char	*ecvt(double, int, int *, int *);
extern char	*ecvtl(long double, int, int *, int *);
extern char	*fcvt(double, int, int *, int *);
extern char	*fcvtl(long double, int, int *, int *);
extern char	*getcwd(char *, size_t);
extern char	*getlogin(void);
extern int	getopt(int, char *const *, const char *);
extern int	getsubopt(char **, char *const *, char **);
extern char	*optarg;
extern int	optind, opterr, optopt;
extern char	*getpass(const char *);
extern int	getpw(int, char *);
extern char	*gcvt(double, int, char *);
extern char	*gcvtl(long double, int, char *);
extern int	grantpt(int);
extern char	*initstate(unsigned, char * , int);
extern int	isatty(int);
extern void	l3tol(long *, const char *, int);
extern char	*l64a(long);
extern char	*l64a_r(long, char *, size_t);
extern void	ltol3(char *, const long *, int);
struct mallinfo	mallinfo(void);
extern void	*memalign(size_t, size_t);
extern int	mkstemp(char *);
extern char	*mktemp(char *);
extern char	*ptsname(int);
extern int	rand_r(unsigned int *);
extern long	random(void);
extern char	*realpath(const char *, char *);
extern char	*setstate(char *);
extern void	srandom(unsigned);
extern void	swab(const void *, void *, ssize_t);
extern char	*ttyname(int);
extern int	ttyslot(void);
extern int	unlockpt(int);
extern void	*valloc(size_t);
extern double	wcstod(const wchar_t *, wchar_t **);
	/*LINTED*/
extern float	wcstof(const wchar_t *, wchar_t **);
extern long	wcstol(const wchar_t *, wchar_t **, int);
long double	wcstold(const wchar_t *, wchar_t **);
long long	wcstoll(const wchar_t *, wchar_t **, int);
unsigned long	wcstoul(const wchar_t *, wchar_t **, int);
unsigned long long wcstoull(const wchar_t *, wchar_t **, int);
extern int	_xgetlogin_r(char *, size_t);
extern int	_xttyname_r(int, char *, size_t);

#ifdef _SIMPLE_R

extern char	*getlogin_r(char *, size_t);
extern char	*ttyname_r(int, char *, size_t);

#elif defined(_EFTSAFE)

#undef getlogin_r
#define getlogin_r(p,s)		_xgetlogin_r(p,s)
#undef ttyame_r
#define ttyname_r(f,p,s)	_xttyname_r(f,p,s)

#else

#ifndef _GETLOGIN_R
#define _GETLOGIN_R
static int
getlogin_r(char *__1, size_t __2)
{
	return _xgetlogin_r(__1, __2);
}
#endif /*_GETLOGIN_R*/

#ifndef _TTYNAME_R
#define _TTYNAME_R
static int
ttyname_r(int __1, char *__2, size_t __3)
{
	return _xttyname_r(__1, __2, __3);
}
#endif /*_TTYNAME_R*/

#endif /*_SIMPLE_R*/

#elif defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1

extern long	a64l(const char *);
extern char	*ecvt(double, int, int *, int *);
extern char	*fcvt(double, int, int *, int *);
extern char	*gcvt(double, int, char *);
extern int	getsubopt(char **, char *const *, char **);
extern int	grantpt(int);
extern char	*initstate(unsigned, char * , int);
extern char	*l64a(long);
extern int	mkstemp(char *);
extern char	*mktemp(char *);
extern char	*ptsname(int);
extern long	random(void);
extern char	*realpath(const char *, char *);
extern char	*setstate(char *);
extern void	srandom(unsigned);
extern int	ttyslot(void);
extern int	unlockpt(int);
extern void	*valloc(size_t);

#endif /*__STDC__ - 0 == 0 && ...*/

#ifdef __cplusplus
}
#endif

#define mblen(s, n)	mbtowc((wchar_t *)0, s, n)

#endif /*_STDLIB_H*/
