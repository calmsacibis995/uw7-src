#ifndef	_IO_WS_MB_H	/* wrapper symbol for kernel use */
#define	_IO_WS_MB_H	/* subject to change without notice */

#ident	"@(#)mb.h	1.4"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>
#include <io/ws/ws.h>

#elif defined(_KERNEL)

#include <sys/types.h>
#include <sys/ws.h>

#endif /* _KERNEL_HEADERS */

/*
 * Multi-Byte Console support definitions.
 */

#define	FONTNAME_SIZE	32

struct fontioc {
	char	fontname[4][FONTNAME_SIZE];	/* fontname for each codeset */
};
typedef struct fontioc fontioc_t;

extern int fnt_init_flg; /* indicate fnt has been loaded and initialized */

#define	GS_CHAN_INITIALIZED(CHP)	(EUC_INFO(CHP))

#define	EUC_INFO(CHP)	((CHP)->ch_euc_info)

/*
 * These values are hardcoded for VGA 640x480 resolution.
 */
#define	FONTWIDTH	8
#define FONTHEIGHT	16
#define	CELLHEIGHT	16
#define	ORIGIN		40

#define	T_COLS		(WSCMODE(vp)->m_xpels/FONTWIDTH)
/* #define T_ROWS	(WSCMODE(vp)->m_ypels/FONTHEIGHT) */
#define	T_ROWS		25

#define	TXT2GR(TSP, POS)	((((POS)/(TSP)->t_cols)*CELLHEIGHT+ORIGIN)*(TSP)->t_cols+((POS)%(TSP)->t_cols))

struct gsfntops {
	uchar_t *(*fnt_getbitmap)(ws_channel_t *, wchar_t);
	void	(*fnt_unlockbitmap)(ws_channel_t *, wchar_t);
	int	(*fnt_setfont)(ws_channel_t *, caddr_t);
	int	(*fnt_getfont)(ws_channel_t *, caddr_t);
};
typedef struct gsfntops gsfntops_t;

extern gsfntops_t Gs;
extern int channel_ref_count;
extern int gs_init_flg;	/* indicate gsd has been loaded and initialized */

#endif /* _IO_WS_MB_H */
