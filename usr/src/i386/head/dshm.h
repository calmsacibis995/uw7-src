            /* Data General Confidential and Proprietary */

#ifndef _DSHM_H
#define _DSHM_H

#ident	"@(#)sgs-head:i386/head/dshm.h	1.1"

#ifndef _SYS_DSHM_H
#include <sys/dshm.h>
#endif

#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif

/*
 * DSHM handle typedef and null value.
 */
typedef struct dshm_attach * dshm_handle_t;
#define DSHM_NULL_HANDLE         ((dshm_handle_t) -1)

/*
 * Null buffer pointer.  DSHM applications must initialize shared
 * buffer pointers to this value before using the buffer pointers
 * with dshm_map().
 */
#define DSHM_NULL                ((const void *) 0)

/*
 * Maximum DSHM buffer size in bytes.
 */
#define DSHM_MAX_BUFFER_SIZE     65536

/*
 * Flag for use in seventh argument to dshm_get().
 *
 * DSHM_FORCE_UNMAP requests DSHM to destroy all unused mappings so that
 * they cannot be reused.  This mode is useful for debugging application
 * problems where one suspects that stale (invalid) buffer pointers are
 * being used.  When DSHM_FORCE_UNMAP is set, dereferencing a stale
 * pointer will typically result in a signal.  This behavior contrasts
 * with DSHM's default (caching) behavior, where dereferencing a stale
 * pointer will typically work (accessing the intended buffer) or
 * silently access an unintended buffer.
 */
#define DSHM_FORCE_UNMAP        1

/*
 * DSHM function prototypes.
 */

#ifdef __STDC__

size_t        dshm_alignment(void);
size_t        dshm_minmapsize(size_t, unsigned long);
int           dshm_get(key_t, size_t, unsigned long, const void *,
                              size_t, int, int);
dshm_handle_t dshm_attach(int, int, unsigned long *);
int           dshm_reattach(dshm_handle_t);
int           dshm_map(dshm_handle_t, unsigned long, const void **);
int           dshm_unmap(dshm_handle_t, const void *);
int           dshm_detach(dshm_handle_t);
int           dshm_control(int, int, struct dshmid_ds *);

#else

size_t        dshm_alignment();
size_t        dshm_minmapsize();
int           dshm_get();
dshm_handle_t dshm_attach();
int           dshm_reattach();
int           dshm_map();
int           dshm_unmap();
int           dshm_detach();
int           dshm_control();

#endif /* __STDC__ */
#endif /* _DSH_H */
