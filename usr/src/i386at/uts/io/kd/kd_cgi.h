#ifndef _IO_KD_KD_CGI_H	/* wrapper symbol for kernel use */
#define _IO_KD_KD_CGI_H	/* subject to change without notice */

#ident	"@(#)kd_cgi.h	1.4"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Structure for listing valid adapter I/O addresses
 */
struct portrange {
	ushort_t first;		/* first port */
	ushort_t count;		/* number of valid right after 'first' */
};

#define	BLACK		0x0
#define	BLUE		0x1
#define	GREEN		0x2
#define	CYAN		0x3
#define	RED		0x4
#define	MAGENTA		0x5
#define	BROWN		0x6
#define	WHITE		0x7
#define	GRAY		0x8
#define	LT_BLUE		0x9
#define	LT_GREEN	0xA
#define	LT_CYAN		0xB
#define	LT_RED		0xC
#define	LT_MAGENTA	0xD
#define	YELLOW		0xE
#define	HI_WHITE	0xF

struct cgi_class {
	char   *name;
	char   *text;
	long	base;
	long	size;
	struct portrange *ports;
};

#ifdef _KERNEL

struct ws_channel_info;

extern int cgi_mapclass(struct ws_channel_info *, int, int *);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_KD_KD_CGI_H */
