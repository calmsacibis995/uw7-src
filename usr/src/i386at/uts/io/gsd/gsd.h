/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

#ifndef	_IO_GSD_GSD_H	/* wrapper symbol for kernel use */
#define	_IO_GSD_GSD_H	/* subject to change without notice */

#ident	"@(#)gsd.h	1.4"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/ldterm/eucioctl.h>
#include <io/ws/mb.h>

#elif defined(_KERNEL)

#include <sys/eucioctl.h>
#include <sys/mb.h>

#endif

#ifdef _KERNEL

extern void gcl_bs(kdcnops_t *, ws_channel_t *, termstate_t *);
extern void gcl_bht(kdcnops_t *, ws_channel_t *, termstate_t *);
extern void gcl_ht(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp);
extern void gcl_cursor(kdcnops_t *, ws_channel_t *);
extern void gcl_escr(kdcnops_t *, ws_channel_t *, termstate_t *);
extern void gcl_eline(kdcnops_t *, ws_channel_t *, termstate_t *);
extern void gcl_norm(struct kdcnops *, ws_channel_t *, termstate_t *, ushort);
extern void gcl_handler(wstation_t *, mblk_t *, termstate_t *, ws_channel_t *);
extern void gcl_ichar(kdcnops_t *, ws_channel_t *, termstate_t *);
extern void gcl_dchar(kdcnops_t *, ws_channel_t *, termstate_t *);
extern void gcl_iline(kdcnops_t *, ws_channel_t *, termstate_t *);
extern void gcl_dline(kdcnops_t *, ws_channel_t *, termstate_t *);
extern void gcl_scrlup(struct kdcnops *, ws_channel_t *, termstate_t *);
extern void gcl_scrldn(struct kdcnops *, ws_channel_t *, termstate_t *);
extern void gcl_sfont(wstation_t *, ws_channel_t *, termstate_t *);
extern void gdv_scrxfer(ws_channel_t *, int);
extern int  gdv_mvword(ws_channel_t *, ushort, ushort, int, char);
extern void gdv_stchar(ws_channel_t *, ushort, wchar_t, uchar_t, int, int);
extern int  gv_cursortype(wstation_t *, ws_channel_t *, termstate_t *);
extern int  gdclrscr(ws_channel_t *, ushort, int);
extern int  gs_alloc(struct kdcnops *, ws_channel_t *, termstate_t *);
extern void gs_free(struct kdcnops *, ws_channel_t *, termstate_t *);
extern void gs_chinit(wstation_t *, ws_channel_t *);
extern int  gs_setcursor(ws_channel_t *, termstate_t *);
extern int  gs_unsetcursor(ws_channel_t *, termstate_t *);
extern int  gs_seteuc(ws_channel_t *, struct eucioc *);
extern void gs_writebitmap(ws_channel_t *, int, unsigned char *, int, uchar_t);
extern int  gs_ansi_cntl(struct kdcnops *, ws_channel_t *, termstate_t *, ushort);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_GSD_GSD_H */
