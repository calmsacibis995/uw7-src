#ifndef _IO_WS_8042_H	/* wrapper symbol for kernel use */
#define _IO_WS_8042_H	/* subject to change without notice */

#ident	"@(#)8042.h	1.13"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ksynch.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */


/*
 * defines for i8042_program() 
 */
#define	P8042_KBDENAB	1
#define	P8042_KBDDISAB	2
#define	P8042_AUXENAB	3
#define	P8042_AUXDISAB	4

/*
 * defines for i8042_send_cmd() 
 */
#define	P8042_TO_KBD	1
#define	P8042_TO_AUX	2


#ifdef _KERNEL

#define I8042PL		plstr

#define I8042_LOCK(opl) { \
		if (i8042_initialized) \
			(opl) = LOCK(i8042_mutex, I8042PL); \
}

#define I8042_UNLOCK(opl) { \
		if (i8042_initialized) \
			UNLOCK(i8042_mutex, (opl)); \
}

extern boolean_t i8042_initialized;
extern lock_t *i8042_mutex;

extern void	i8042_acquire(void);
extern void 	i8042_release(void);
extern int	i8042_send_cmd(uchar_t, uchar_t, uchar_t *, uchar_t);
extern void	i8042_program(int);
extern int	i8042_aux_port(void);
extern uchar_t	i8042_read(void);
extern int	i8042_write(uchar_t, uchar_t);
extern void	i8042_enable_interface(void);
extern void	i8042_disable_interface(void);
extern void	i8042_enable_aux_interface(void);
extern void	i8042_disable_aux_interface(void);
extern void	i8042_update_leds(ushort_t, ushort_t);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_WS_8042_H */
