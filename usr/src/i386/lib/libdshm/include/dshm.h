	/**********************************************************
	* Copyright (C) Data General Corporation, 1996 - 1997	  *
	* All Rights Reserved.					  *
	* Licensed Material-Property of Data General Corporation. *
	* This software is made available solely pursuant to the  *
	* terms of a DGC license agreement which governs its use. *
	**********************************************************/

#ifndef _DSHM_H
#define _DSHM_H

#ident	"@(#)libdshm:i386/lib/libdshm/include/dshm.h	1.3"
#ident	"$Header$"

#ifndef _SYS_DSHM_H
#include <sys/dshm.h>
#endif

#ifndef _SYS_IPC_H
#include <sys/ipc.h>
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
#define DSHM_NULL                ((void *) 0)

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
void          dshm_updatetlb(void);
int           dshm_map(dshm_handle_t, unsigned long, const void **);
unsigned long dshm_bufindex(dshm_handle_t, const void *);
int           dshm_unmap(dshm_handle_t, const void *);
int           dshm_detach(dshm_handle_t);
int           dshm_control(int, int, struct dshmid_ds *);
int           dshm_memloc(int, unsigned long, unsigned long, cgid_t *);

#else

size_t        dshm_alignment();
size_t        dshm_minmapsize();
int           dshm_get();
dshm_handle_t dshm_attach();
int           dshm_reattach();
void          dshm_updatetlb();
int           dshm_map();
unsigned long dshm_bufindex();
int           dshm_unmap();
int           dshm_detach();
int           dshm_control();
int           dshm_memloc();

#endif /* __STDC__ */
#endif /* _DSHM_H */
