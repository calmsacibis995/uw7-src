#ifndef _STDIO_H
#define _STDIO_H
#ident	"@(#)sgs-head:i386/head/stdio.h	2.34.7.27"

#if defined(_LARGEFILE_SOURCE) || defined(_LARGEFILE64_SOURCE) \
	|| defined(_FILE_OFFSET_BITS) \
	|| __STDC__ - 0 == 0 && !defined(_XOPEN_SOURCE) \
		&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)
#include <sys/types.h>
typedef long		fpos32_t;
typedef long long	fpos64_t;
#endif

#ifndef _FILE_OFFSET_BITS
typedef long		fpos_t;
#elif _FILE_OFFSET_BITS - 0 == 32
typedef fpos32_t	fpos_t;
#elif _FILE_OFFSET_BITS - 0 == 64
typedef fpos64_t	fpos_t;
#else
#error "_FILE_OFFSET_BITS, if defined, must be 32 or 64"
#endif

#ifndef _SIZE_T
#   define _SIZE_T
	typedef unsigned int	size_t;
#endif

#ifndef NULL
#   define NULL	0
#endif

#ifndef EOF
#   define EOF	(-1)
#endif

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

#define TMP_MAX		17576	/* 26 * 26 * 26 */

#define BUFSIZ		1024	/* default buffer size */
#define FOPEN_MAX	60	/* at least this many FILEs available */
#define FILENAME_MAX	1024	/* max # of characters in a path name */

#define _IOFBF		0000	/* full buffered */
#define _IOLBF		0100	/* line buffered */
#define _IONBF		0004	/* not buffered */
#define _IOEOF		0020	/* EOF reached on read */
#define _IOERR		0040	/* I/O error from system */

#define _IOREAD		0001	/* currently reading */
#define _IOWRT		0002	/* currently writing */
#define _IORW		0200	/* opened for reading and writing */
#define _IOMYBUF	0010	/* stdio malloc()'d buffer */

#if __STDC__ - 0 == 0 || defined(_XOPEN_SOURCE) \
	|| defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE)
#   define L_ctermid	9
#   define L_cuserid	9
#endif

#if defined(_XOPEN_SOURCE) || (__STDC__ - 0 == 0 \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE))
#   define P_tmpdir	"/var/tmp/"
#endif

#define L_tmpnam	25	/* (sizeof(P_tmpdir) + 15) */

typedef struct _FILE_
{
	int		__cnt;		/* num. avail. characters in buffer */
	unsigned char	*__ptr;		/* next character from/to here */
	unsigned char	*__base;	/* the buffer (not really) */
	unsigned char	__flag;		/* the state of the stream */
	unsigned char	__file;		/* file descriptor (not necessarily) */
	unsigned char	__buf[2];	/* micro buffer as a fall-back */
} FILE;

#ifndef _VA_LIST
#   if #machine(i860)
	struct _va_list
	{
		unsigned	_ireg_used;
		unsigned	_freg_used;
		long		*_reg_base;
		long		*_mem_ptr;
	};
#	define _VA_LIST struct _va_list
#   else
#	define _VA_LIST void *
#   endif
#endif

#if defined(_XOPEN_SOURCE) && !defined(__VA_LIST)
#   define __VA_LIST
	typedef _VA_LIST	va_list;
#endif

#define stdin	(&__iob[0])
#define stdout	(&__iob[1])
#define stderr	(&__iob[2])

#ifdef __cplusplus
extern "C" {
#endif

extern FILE	__iob[];

extern int	remove(const char *);
extern int	rename(const char *, const char *);
extern FILE	*tmpfile(void);
extern char	*tmpnam(char *);
extern int	fclose(FILE *);
extern int	fflush(FILE *);
extern FILE	*fopen(const char *, const char *);
extern FILE	*freopen(const char *, const char *, FILE *);
extern void	setbuf(FILE *, char *);
extern int	setvbuf(FILE *, char *, int, size_t);
		/*PRINTFLIKE2*/
extern int	fprintf(FILE *, const char *, ...);
		/*SCANFLIKE2*/
extern int	fscanf(FILE *, const char *, ...);
		/*PRINTFLIKE1*/
extern int	printf(const char *, ...);
		/*SCANFLIKE1*/
extern int	scanf(const char *, ...);
		/*PRINTFLIKE2*/
extern int	sprintf(char *, const char *, ...);
		/*SCANFLIKE2*/
extern int	sscanf(const char *, const char *, ...);
extern int	vfprintf(FILE *, const char *, _VA_LIST);
extern int	vprintf(const char *, _VA_LIST);
extern int	vsprintf(char *, const char *, _VA_LIST);
extern int	fgetc(FILE *);
extern char	*fgets(char *, int, FILE *);
extern int	fputc(int, FILE *);
extern int	fputs(const char *, FILE *);
extern int	getc(FILE *);
extern int	getchar(void);
extern char	*gets(char *);
extern int	putc(int, FILE *);
extern int	putchar(int);
extern int	puts(const char *);
extern int	ungetc(int, FILE *);
extern size_t	fread(void *, size_t, size_t, FILE *);
extern size_t	fwrite(const void *, size_t, size_t, FILE *);
extern int	fgetpos(FILE *, fpos_t *);
extern int	fseek(FILE *, long, int);
extern int	fsetpos(FILE *, const fpos_t *);
extern long	ftell(FILE *);
extern void	rewind(FILE *);
extern void	clearerr(FILE *);
extern int	feof(FILE *);
extern int	ferror(FILE *);
extern void	perror(const char *);

extern int	__filbuf(FILE *);
extern int	__flsbuf(int, FILE *);

#ifndef __cplusplus
	#pragma int_to_unsigned fread
	#pragma int_to_unsigned fwrite
#endif

#if !#lint(on)

#ifndef _REENTRANT
#   define getc(p)	(--((FILE *)(p))->__cnt < 0 ? __filbuf(p) \
				: (int)*((FILE *)(p))->__ptr++)
#   define putc(x, p)	(--((FILE *)(p))->__cnt < 0 ? __flsbuf(x, p) \
				: (int)(*((FILE *)(p))->__ptr++ = (x)))
#endif

#define getchar()	getc(stdin)
#define putchar(x)	putc((x), stdout)

#ifdef __cplusplus
#define feof(p)   ((p)->__flag & _IOEOF)
#define ferror(p) ((p)->__flag & _IOERR)
#else /* use sizeof and cast to make these act more like C functions */
#define feof(p)   ((void)sizeof(__filbuf(p)), ((FILE *)(p))->__flag & _IOEOF)
#define ferror(p) ((void)sizeof(__filbuf(p)), ((FILE *)(p))->__flag & _IOERR)
#endif /*__cplusplus*/

#endif /*#lint(on)*/

#if __STDC__ - 0 == 0 || defined(_XOPEN_SOURCE) \
	|| defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE)
extern char	*ctermid(char *);
extern FILE	*fdopen(int, const char *);
extern int	fileno(FILE *);
#endif

#if defined(_XOPEN_SOURCE) || (__STDC__ - 0 == 0 \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE))
extern FILE	*popen(const char *, const char *);
extern char	*cuserid(char *);
extern char	*tempnam(const char *, const char *);
extern char	*optarg;
extern int	optind, opterr, optopt;
extern int	getopt(int, char *const *, const char *);
extern int	getw(FILE *);
extern int	putw(int, FILE *);
extern int	pclose(FILE *);
#endif

#ifdef _REENTRANT

extern int	getc_unlocked(FILE *);
extern int	getchar_unlocked(void);
extern int	putc_unlocked(int, FILE *);
extern int	putchar_unlocked(int);

#if !#lint(on)
#   define getc_unlocked(p)	(--((FILE *)(p))->__cnt < 0 ? __filbuf(p) \
					: (int)*((FILE *)(p))->__ptr++)
#   define putc_unlocked(x, p)	(--((FILE *)(p))->__cnt < 0 ? __flsbuf(x, p) \
					: (int)(*((FILE *)(p))->__ptr++ = (x)))
#   define getchar_unlocked()	getc_unlocked(stdin)
#   define putchar_unlocked(x)	putc_unlocked((x), stdout)
#endif

extern void	flockfile(FILE *);
extern int	ftrylockfile(FILE *);
extern void	funlockfile(FILE *);

#endif /*_REENTRANT*/

#if defined(_LARGEFILE_SOURCE) || defined(_LARGEFILE64_SOURCE) \
	|| defined(_FILE_OFFSET_BITS) \
	|| __STDC__ - 0 == 0 && !defined(_XOPEN_SOURCE) \
		&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

extern int	fseeko(FILE *, n_off_t, int);
extern n_off_t	ftello(FILE *);

extern int	fgetpos32(FILE *, fpos32_t *);
extern FILE	*fopen32(const char *, const char *);
extern FILE	*freopen32(const char *, const char *, FILE *);
extern int	fseeko32(FILE *, off32_t, int);
extern int	fsetpos32(FILE *, const fpos32_t *);
extern off32_t	ftello32(FILE *);
extern FILE	*tmpfile32(void);

extern int	fgetpos64(FILE *, fpos64_t *);
extern FILE	*fopen64(const char *, const char *);
extern FILE	*freopen64(const char *, const char *, FILE *);
extern int	fseeko64(FILE *, off64_t, int);
extern int	fsetpos64(FILE *, const fpos64_t *);
extern off64_t	ftello64(FILE *);
extern FILE	*tmpfile64(void);

#if _FILE_OFFSET_BITS - 0 == 32

#undef fgetpos
#define fgetpos	fgetpos32
#undef fopen
#define fopen	fopen32
#undef freopen
#define freopen	freopen32
#undef fseeko
#define fseeko	fseeko32
#undef fsetpos
#define fsetpos	fsetpos32
#undef ftello
#define ftello	ftello32
#undef tmpfile
#define tmpfile	tmpfile32

#elif _FILE_OFFSET_BITS - 0 == 64

#undef fgetpos
#define fgetpos	fgetpos64
#undef fopen
#define fopen	fopen64
#undef freopen
#define freopen	freopen64
#undef fseeko
#define fseeko	fseeko64
#undef fsetpos
#define fsetpos	fsetpos64
#undef ftello
#define ftello	ftello64
#undef tmpfile
#define tmpfile	tmpfile64

#endif /*_FILE_OFFSET_BITS*/

#endif /*defined(_LARGEFILE_SOURCE) || ...*/

#if __STDC__ - 0 == 0 && !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

#ifndef _WCHAR_T
#   define _WCHAR_T
	typedef long	wchar_t;
#endif

#ifndef _WINT_T
#   define _WINT_T
	typedef long	wint_t;
#endif

extern int	system(const char *);
extern int	fwide(FILE *, int);
extern wint_t	fgetwc(FILE *);
extern wchar_t	*fgetws(wchar_t *, int, FILE *);
extern wint_t	fputwc(wint_t, FILE *);
extern int	fputws(const wchar_t *, FILE *);
extern wint_t	getwc(FILE *);
extern wint_t	getwchar(void);
extern wint_t	putwc(wint_t, FILE *);
extern wint_t	putwchar(wint_t);
extern wint_t	ungetwc(wint_t, FILE *);
		/*WPRINTFLIKE2*/
extern int	fwprintf(FILE *, const wchar_t *, ...);
		/*WSCANFLIKE2*/
extern int	fwscanf(FILE *, const wchar_t *, ...);
		/*WPRINTFLIKE1*/
extern int	wprintf(const wchar_t *, ...);
		/*WSCANFLIKE1*/
extern int	wscanf(const wchar_t *, ...);
		/*WPRINTFLIKE3*/
extern int	swprintf(wchar_t *, size_t, const wchar_t *, ...);
		/*WSCANFLIKE2*/
extern int	swscanf(const wchar_t *, const wchar_t *, ...);
extern int	vfwprintf(FILE *, const wchar_t *, _VA_LIST);
extern int	vfwscanf(FILE *, const wchar_t *, _VA_LIST);
extern int	vwprintf(const wchar_t *, _VA_LIST);
extern int	vwscanf(const wchar_t *, _VA_LIST);
extern int	vswprintf(wchar_t *, size_t, const wchar_t *, _VA_LIST);
extern int	vswscanf(const wchar_t *, const wchar_t *, _VA_LIST);
extern void	funflush(FILE *);
		/*PRINTFLIKE3*/
extern int	snprintf(char *, size_t, const char *, ...);
extern int	vsnprintf(char *, size_t, const char *, _VA_LIST);
extern int	vfscanf(FILE *, const char *, _VA_LIST);
extern int	vscanf(const char *, _VA_LIST);
extern int	vsscanf(const char *, const char *, _VA_LIST);

#endif /*__STDC__ - 0 == 0 && ...*/

#ifdef __cplusplus
}
#endif

#endif /*_STDIO_H*/
