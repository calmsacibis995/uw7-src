#ifndef	_IO_GVID_VDC_H	/* wrapper symbol for kernel use */
#define	_IO_GVID_VDC_H	/* subject to change without notice */

#ident	"@(#)vdc.h	1.9"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/kd/kd.h>	/* REQUIRED */
#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/kd.h>	/* REQUIRED */
#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

struct vdc_info {
	int	v_type;
	uchar_t	v_switch,
		v_mode2sel;
	struct kd_vdctype v_info;
};


#define VTYPE(x)	(Vdc.v_type & (x))	/* Type of VDC card (if any) */
#define VSWITCH(x)	(Vdc.v_switch & (x))	/* VDC switch settings */


#define V400		1
#define V750		2
#define V600		4
#define CAS2		8 	/* indicates a Cascade 2 w/ on-board VGA */
#ifdef	EVC
#define VEVC		0x10	/* Olivetti EVC-1 is installed */
#endif	/* EVC */
#ifdef EVGA
#define VEVGA		0x20	/* Extended VGA type board */
#endif	/* EVGA */

#define V750_IDADDR	0xc0009
#define V600_IDADDR	0xc0035
#define CAS2_IDADDR	0xe0010

#define ATTCGAMODE	0x01
#define ATTDISPLAY	0x02


#ifdef _KERNEL

struct ws_channel_info;
struct termstate;
struct vidstate;

/*
 * vdc prototype declarations
 */

extern int	vdc_disptype(struct ws_channel_info *, int);
extern void	vdc_lktime(int);
extern void	vdc_scrambler(int);
extern void	vdc_check(uchar_t);
extern void	vdc_info(struct vidstate *);
extern uchar_t	vdc_rdmon(uchar_t);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_GVID_VDC_H */
