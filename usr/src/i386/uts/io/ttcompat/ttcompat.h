#ifndef _IO_TTCOMPAT_TTCOMPAT_H	/* wrapper symbol for kernel use */
#define _IO_TTCOMPAT_TTCOMPAT_H	/* subject to change without notice */

#ident	"@(#)ttcompat.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * BSD/XENIX/V7 ttcompat module header file
 */

#ifdef _KERNEL_HEADERS

#include <io/stream.h>		/* REQUIRED */
#include <io/termios.h>		/* REQUIRED */
#include <io/ttold.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

/* Enhanced Application Compatibility Support */
#include <svc/sco.h>		/* REQUIRED */
/* End Enhanced Application Compatibility Support */

#endif /* _KERNEL_HEADERS */


#ifdef _KERNEL

/*
 * Old-style terminal state.
 */
typedef struct {
	int	t_flags;		/* flags */
	char	t_ispeed;		/* input speed code */
	char	t_ospeed;		/* output speed code */
	speed_t	t_real_ispeed;		/* actual input speed */
	speed_t	t_real_ospeed;		/* actual output speed */
	char	t_erase;		/* erase last character */
	char	t_kill;			/* erase entire line */
	char	t_intrc;		/* interrupt */
	char	t_quitc;		/* quit */
	char	t_startc;		/* start output */
	char	t_stopc;		/* stop output */
	char	t_eofc;			/* end-of-file */
	char	t_brkc;			/* input delimiter (like nl) */
	char	t_suspc;		/* stop process signal */
	char	t_dsuspc;		/* delayed stop process signal */
	char	t_rprntc;		/* reprint line */
	char	t_flushc;		/* flush output (toggles) */
	char	t_werasc;		/* word erase */
	char	t_lnextc;		/* literal next character */
	int	t_xflags;		/* XXX extended flags */
	tcflag_t t_lflag;		/* saved lflag(s) */
	tcflag_t t_iflag;		/* saved iflag(s) */
} compat_state_t;

/*
 * Per-tty structure.
 */
typedef struct {
	lock_t		*t_lock;	/* MP lock */
	compat_state_t	t_curstate;	/* current emulated state */

	/* Enhanced Application Compatibility Support */
	struct sco_termios t_new_sco;	/* new sco_termios for SCO_X[SG]ETA */
	/* End Enhanced Application Compatibility Support */

	struct sgttyb	t_new_sgttyb;	/* new sgttyb from TIOCSET[PN] */
	struct tchars	t_new_tchars;	/* new tchars from TIOCSETC */
	struct ltchars	t_new_ltchars;	/* new ltchars from TIOCSLTC */
	int		t_new_lflags;	/* new lflags from TIOCLSET/LBIS/LBIC */
	int		t_state;	/* state bits */
	int		t_iocid;	/* ID of "ioctl" we handle specially */
	int		t_ioccmd;	/* ioctl code for that "ioctl" */
} ttcompat_state_t;

/*
 * values for t_state bits
 */
#define TS_IOCWAIT 0x01	/* waiting for an M_IOCACK/M_IOCNAK from downstream */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TTCOMPAT_TTCOMPAT_H */
