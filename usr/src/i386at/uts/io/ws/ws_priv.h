#ifndef	_IO_WS_WS_PRIV_H	/* wrapper symbol for kernel use */
#define	_IO_WS_WS_PRIV_H	/* subject to change without notice */

#ident	"@(#)ws_priv.h	1.3"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <proc/proc.h>		/* REQUIRED */
#include <proc/pid.h>		/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h>		/* REQUIRED */
#include <sys/proc.h>		/* REQUIRED */
#include <sys/pid.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)


/*
 * The size of ws_dispatch structure is 0x24 (36) bytes
 */
typedef struct ws_dispatch {

	pid_t	proc_id;
	proc_t	*procp;
	void	*ref;				/* Return from proc_ref */
	uint_t	flags;				/* Init option flags */

	int	(*on_proc)(void *);		/* LWP on proc handler */
	int	(*off_proc)(void *);		/* LWP off proc handler */
	int	(*release_access)(void *);	/* Release access handler */
	int	(*resume_access)(void *);	/* Resume access handler */

	void	*driver_data;			/* Pointer to opaque driver
						 * specific object
						 */
	int	driver_tag;			/* Used to identify drivers */

} ws_dispatch_t;

/*
 * The size of ws_bind_t structure is 0x0C bytes
 */
typedef struct ws_bind {

	proc_t	*procp;				/* Process to bind */
	void	*ref;				/* Proc ref tag */
	uint_t	count;				/* Number of maps for chan */

} ws_bind_t;

/*
 * Multiconsole support prototypes.
 */
extern int	ws_con_maybe_bind(proc_t *, boolean_t);

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_WS_WS_PRIV_H */

