#ident	"@(#)kern-i386at:io/async/async.cf/Space.c	1.2"
#ident	"$Header$"

#include <sys/aiosys.h>
#include <config.h>

u_int numaio = NUMAIO;			/* Total number of control blocks */
aio_t aio_list[NUMAIO];			/* Static list of control blocks */
u_int aio_listio_max = AIO_LISTIO_MAX;	/* Size of largest list I/O request */
u_int listio_size =
	sizeof(aiolistio_t) + (AIO_LISTIO_MAX - 1) * sizeof(aiojob_t);
