#ifndef _IO_GVID_VID_H	/* wrapper symbol for kernel use */
#define	_IO_GVID_VID_H	/* subject to changes without notice */

#ident	"@(#)vid.h	1.7"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define ATYPE(x, y)	(x.w_vstate.v_cmos == y)
#define DTYPE(x, y)	(x.w_vstate.v_type == y)
#define CMODE(x, y)	(x.w_vstate.v_cvmode == y)	/* Current disp. mode */
#define DMODE(x, y)	(x.w_vstate.v_dvmode == y)

#define KD_RDVMEM	0
#define KD_WRVMEM	1

struct modeinfo {
	ushort	m_cols;		/* 0x00: Number of character columns */
	ushort	m_rows;		/* 0x02: Number of character rows */
	ushort	m_xpels;	/* 0x04: Number of pels on x axis */
	ushort	m_ypels;	/* 0x06: Number of pels on y axis */
	unchar	m_color;	/* 0x08: Non-zero value indicates color mode */
	paddr_t	m_base;		/* 0x0C: Physical address of screen memory */
	ulong	m_size;		/* 0x10: Size of screen memory */
	unchar	m_font;		/* 0x14: Default font (0 == grahpics mode) */
	unchar	m_params;	/* 0x15: Parameter location: BIOS or static table */
	unchar	m_offset;	/* 0x16: offset with respect to m_params */
	unchar	m_ramdac;	/* 0x17: RAMDAC table offset */
	caddr_t	m_vaddr;	/* 0x18: virtual address of screen memory */
				/* 0x1C: */
};

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_GVID_VID_H */
