#ifndef	_IO_ANSI_ANSI_H	/* wrapper symbol for kernel use */
#define	_IO_ANSI_ANSI_H	/* subject to change without notice */

#ident	"@(#)ansi.h	1.6"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/stream.h>		/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/stream.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * definitions for Integrated Workstation Environment ANSI x3.64 
 * terminal control language parser 
 */
#define ANSI_MAXPARAMS	5	/* maximum number of ANSI paramters */
#define ANSI_MAXTAB	40	/* maximum number of tab stops */
#define ANSI_MAXFKEY	30	/* max length of function key with <ESC>Q */


#define	ANSIPSZ		64	/* max packet size sent by ANSI */

/*
 * Font values for ansistate
 */
#define	ANSI_FONT0	0	/* Primary font (default) */
#define	ANSI_FONT1	1	/* First alternate font */
#define	ANSI_FONT2	2	/* Second alternate font */

#define	ANSI_BLKOUT	0x8000	/* Scroll lock, for M_START, M_STOP */

/*
 * state for ansi x3.64 emulator 
 */
struct ansi_state {		
	ushort_t	a_flags;	/* flags for this x3.64 terminal */
	uchar_t		a_font;		/* font type */
	uchar_t		a_state;	/* state in output esc seq processing */
	uchar_t		a_gotparam;	/* does output esc seq have a param */
	ushort_t	a_curparam;	/* current param # of output esc seq */
	ushort_t	a_paramval;	/* value of current param */
	short	a_params[ANSI_MAXPARAMS];  /* parameters of output esc seq */
	char	a_fkey[ANSI_MAXFKEY];	/* work space for function key */
	mblk_t	*a_wmsg;	/* ptr to data message being assembled */
	queue_t	*a_wqp;		/* ptr to write queue for associated stream */
	queue_t	*a_rqp;		/* ptr to read queue for associated stream */
};


typedef struct ansi_state ansistat_t;

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_ANSI_ANSI_H */
