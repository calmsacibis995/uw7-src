#ifndef	_IO_KD_KB_H	/* wrapper symbol for kernel use */
#define	_IO_KD_KB_H	/* subject to change without notice */

#ident	"@(#)kb.h	1.9"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

#define SEND2KBD(port, byte) { \
	while (inb(KB_STAT) & KB_INBF) \
		; \
	outb(port, byte); \
}

struct kbstate;
struct ws_channel_info;

extern int	kdkb_init(uchar_t *, struct kbstate *);
extern void	kdkb_cmd(uchar_t);
extern void	kdkb_tone(void);
extern void	kdkb_setled(struct kbstate *, uchar_t);
extern void	kdkb_update_kbstate(struct kbstate *, uchar_t);
extern int	kdkb_scrl_lock(void);
extern int	kdkb_locked(ushort_t, uchar_t);
extern void	kdkb_keyclick(void);
extern void	kdkb_force_enable(void);
extern void	kdkb_sound(int);
extern void	kdkb_mktone(struct ws_channel_info *, int);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_KD_KB_H */
