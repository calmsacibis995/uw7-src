/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

#ifndef	_IO_FNT_FNT_H	/* wrapper symbol for kernel use */
#define	_IO_FNT_FNT_H	/* subject to change without notice */

#ident	"@(#)fnt.h	1.3"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/ws/ws.h>
#include <io/ws/mb.h>
#include <util/types.h>

#elif defined(_KERNEL)

#include <sys/ws/ws.h>
#include <sys/mb.h>
#include <sys/types.h>

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL)

extern int fnt_init_flg; /* indicate fnt has been loaded and initialized */
extern unsigned char	*altcharsetdata[];
extern unsigned char	*codeset0data[];
extern unsigned char	*codeset1data[];
extern unsigned char	*codeset2data[];
extern unsigned char	*codeset3data[];
extern int		codeset0width;
extern int		codeset1width;
extern int		codeset2width;
extern int		codeset3width;

extern unsigned char	*fnt_getbitmap(ws_channel_t *, wchar_t);
extern void		fnt_unlockbitmap(ws_channel_t *, wchar_t);
extern int		fnt_setfont(ws_channel_t *, caddr_t);
extern int		fnt_getfont(ws_channel_t *, caddr_t);

#define MAX_CODESET 4
/*
 * Pointers to font data and thier lenghts
 */
typedef struct fnt_codeset {

	size_t	length;
	void	*addr;

} fnt_codeset_t;

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_FNT_FNT_H */
