#ident	"@(#)odimem.h	9.1"

#ifdef _KERNEL_HEADERS

#include <util/types.h>         /* REQUIRED */
#include <io/stream.h>          /* REQUIRED */

#else

#include <sys/types.h>          /* REQUIRED */
#include <sys/stream.h>         /* REQUIRED */

#endif /* _KERNEL_HEADERS */

typedef struct odimem {

	struct odimem 	*odi_next;
	caddr_t		 odi_buf;
	mblk_t		*odi_mblk;
	frtn_t		 odi_frtn;
	u_int		 size;

} odimem_t;

typedef struct membuf5k {

	uchar_t	 odi5kbuf[5120];
	odimem_t *backp_5k;

} membuf5k_t;

typedef struct membuf17k {

	uchar_t	 odi17kbuf[17408];
	odimem_t *backp_17k;

} membuf17k_t;
