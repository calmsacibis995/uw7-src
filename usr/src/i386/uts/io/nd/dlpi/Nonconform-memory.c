#ident "@(#)Nonconform-memory.c	11.3"
#ident "$Header$"

/*
 *	Nonconform-memory.c, memory allocation/deletion.
 *      adapted from usr/src/i386at/uts/io/odi/lsl/lslmemory.c
 *      The OLDWAY denotes areas that lslmemory.c supported but had to
 *      be removed for Gemini.  In particlar, the equivalent of odimem
 *      for MDI drivers doesn't exist at this time.  This define will prove 
 *      useful if/when it is implemented in the future.
 *      We can't include ddi.h as this will change our offset into the uio
 *      structure.
 */

#include <mem/immu.h>   /* NBPP */
#include <mem/kmem.h>   /* physreq_t */
#include <io/nd/sys/mdi.h>
#include <util/debug.h>
#include <util/cmn_err.h>

#if OLDWAY
extern	void	lsl_31_free(void *);
extern	void	*odimem_alloc_physreq(size_t, const physreq_t *, int);

extern lock_t		*lsl_odimem_lock;
extern int              lsl_odimem_max_cached;
#endif

extern void dlpibase_free(void *ptr, int memtype, int miscflags);

/* ripped from assorted lsl header files, inserted here */
typedef unsigned long           UINT32;
/*
 * memory management stuff.
 */
#define DLPIBASE_BUFSTART_MARK               0xAABBCCDD
#define DLPIBASE_BUFEND_MARK                 0xEEEEFFFF


/*
 * dlpibase_alloc(ulong_t nbytes, int memtype, int kmaflags, int miscflags)
 *
 * Description:
 *      Allocate memory.
 *
 * Calling/Exit status:
 *      No locking assumptions.
 *
 * can't be STATIC as called by other Nonconform.c files
 */
void *
dlpibase_alloc(ulong_t nbytes, int memtype, int kmaflags, int miscflags)
{
	void		*buffer;
	physreq_t	*preq;
	UINT32		*tmpptr;
	int		below16 = 0;
	int		pagealigned = 0;
	int		physcontig = 0;
	pl_t		opl;

	ASSERT((kmaflags == KM_SLEEP) || (kmaflags == KM_NOSLEEP));
	ASSERT((miscflags == DLPIBASE_KMEM) || (miscflags == DLPIBASE_ODIMEM));

	if (nbytes == 0) {
		cmn_err(CE_CONT,
		  "dlpibase_alloc: caller tried to allocate 0 bytes\n");

		return(NULL);
	}

	/*
	 * add 3 uints to the nbytes for our own use.
	 */
	nbytes = nbytes + (3 * sizeof(UINT32));

	switch (memtype) {
		case DLPIBASE_MEM_NONE:
			/*
			 * no special requirements.
			 */
			buffer = kmem_alloc((size_t) nbytes, kmaflags);

			break;

		case DLPIBASE_MEM_DMA_LOGICALTOPHYSICAL_BELOW16:

#if OLDWAY
			/*
			 * ODI spec 3.1 requirements.
			 */
			buffer = (void *)dlpibase_31_alloc(nbytes);
#endif
			/* this is unsupported */
			cmn_err(CE_PANIC,"dlpibase_alloc: DLPIBASE_MEM_DMA_LOGICALTOPHYSICAL_BELOW16 is unsupported");

			break;

		case DLPIBASE_MEM_NONE_BELOW16:
			/*
			 * only 24 bit DMA restriction.
			 */
			below16 = 1;
			pagealigned = 0;
			physcontig = 0;

			goto dlpibasephysreq;

		case DLPIBASE_MEM_NONE_PAGEALIGN_BELOW16:
			/*
			 * only 24 bit DMA restriction.
			 */
			below16 = 1;
			pagealigned = 1;
			physcontig = 0;

			goto dlpibasephysreq;

		case DLPIBASE_MEM_NONE_PAGEALIGN:
			/*
			 * page aligned with no other restrictions.
			 */
			below16 = 0;
			pagealigned = 1;
			physcontig = 0;

			goto dlpibasephysreq;

		case DLPIBASE_MEM_DMA_PAGEALIGN_BELOW16:
			/*
			 * page aligned contiguous with 24 bit DMA restriction.
			 */
			below16 = 1;
			pagealigned = 1;
			physcontig = 1;

			goto dlpibasephysreq;

		case DLPIBASE_MEM_DMA_PAGEALIGN:
			/*
			 * page aligned physically contiguous.
			 */
			below16 = 0;
			pagealigned = 1;
			physcontig = 1;

			goto dlpibasephysreq;

		case DLPIBASE_MEM_DMA_BELOW16:
			/*
			 * physically contiguous with 24 bit DMA restriction.
			 */
			below16 = 1;
			pagealigned = 0;
			physcontig = 1;

			goto dlpibasephysreq;

		case DLPIBASE_MEM_DMA:
			/*
			 * physically contiguous with no other restriction.
			 */
			below16 = 0;
			pagealigned = 0;
			physcontig = 1;
dlpibasephysreq:
			preq = physreq_alloc(kmaflags);
			if (preq == NULL) {
				cmn_err(CE_CONT,
				"!dlpibase_alloc: could not allocate physreq_t\n");

				buffer = NULL;
			} else {
				/*
			 	 * setup physreq.
			 	 *
			 	 * - alignment based on pagealign.
			 	 * - no restrictions on crossing physical
			 	 *   boundaries.
			 	 * - dma size based on below16.
			 	 * - no scather gather.
			 	 * - physically contiguous based on physcontig.
			 	 */
				if (pagealigned)
					preq->phys_align = NBPP;
				else
					preq->phys_align = 4;

				preq->phys_boundary = (paddr_t)0;

				if (below16)
					preq->phys_dmasize = 24;
				else
					preq->phys_dmasize = 32;

				preq->phys_max_scgth = (uchar_t)0;

				if (physcontig)
					preq->phys_flags |= PREQ_PHYSCONTIG;
				else
					preq->phys_flags = 0;

				if (!physreq_prep(preq, kmaflags)) {
					ASSERT(kmaflags == KM_NOSLEEP);

			cmn_err(CE_CONT,
		"dlpibase_alloc: could not prep physreq_t\n");
			physreq_free(preq);
	
					buffer = NULL;
			} else {
					if (miscflags == DLPIBASE_KMEM) {
					    /*
					     * need to get memory from KMA.
					     */
					    buffer = kmem_alloc_physreq
							((size_t) nbytes, preq,
							kmaflags);
					} else if (miscflags == DLPIBASE_ODIMEM) {
#if OLDWAY
					    /*
					     * need to get memory from
					     * the ODI memory allocator
					     * (odimem). this is a static
					     * driver which sets aside
					     * memory at boot time.
					     */
					     buffer = odimem_alloc_physreq
						    (nbytes, preq, kmaflags);
#endif
					     cmn_err(CE_PANIC,
			"dlpibase_alloc:miscflags DLPIBASE_ODIMEM unsupported");
					} else {
						cmn_err(CE_PANIC,
			"dlpibase_alloc: caller passed in bad miscflags %d\n",
							miscflags);
					}

					physreq_free(preq);
				}
			}
			break;

		default:
			cmn_err(CE_PANIC, 
				"dlpibase_alloc: illegal memtype 0x%x.\n",
				memtype);
	}

	if (buffer) {
		/*
		 * we got a valid buffer. put in our buffer start and
		 * end markers, and the size.
		 */
		tmpptr = (UINT32 *)buffer;
		*tmpptr = nbytes;
		tmpptr++;
		*tmpptr = DLPIBASE_BUFSTART_MARK;
		tmpptr = (UINT32 *)((UINT32)buffer + nbytes -
						sizeof(UINT32));
		*tmpptr = DLPIBASE_BUFEND_MARK;

		/*
		 * adjust buffer before returning.
		 */
		buffer = (void *)((UINT32)buffer + (2 * sizeof(UINT32)));
	} else {
		cmn_err(CE_CONT, "!dlpibase_alloc: req of %d failed\n", nbytes);
	}

	return(buffer);
}

/*
 * dlpibase_zalloc(ulong_t nbytes, int memtype, int kmaflags, int miscflags)
 *
 * Description:
 *      Allocate memory and zero-fill it.
 *
 * Calling/Exit status:
 *      No locking assumptions.
 *
 * can't be STATIC as called by other Nonconform files
 */
void *
dlpibase_zalloc(ulong_t nbytes, int memtype, int kmaflags, int miscflags)
{
	void *mem;
	

	mem = dlpibase_alloc(nbytes, memtype, kmaflags, miscflags);
	if (mem)
		bzero(mem, nbytes);
	return mem;
}

/*
 * mdi_close_file(void *ptr)
 *
 * Description:
 *      Free memory from file we previously opened.
 *
 * Calling/Exit status:
 *      No locking assumptions.
 *
 * close the file we previously opened.  All we must do is free up
 * the buffer we previously allocated
 */
void
mdi_close_file(void *ptr)
{
	mdi_close_file_i(ptr, DLPIBASE_MEM_DMA_BELOW16, DLPIBASE_KMEM);
}

/*
 * internal version available to use if you know what you're doing. 
 */
void
mdi_close_file_i(void *ptr, int memtype, int miscflags)
{
	dlpibase_free(ptr, memtype, miscflags);
}

/*
 * dlpibase_free(void *ptr, int memtype, int miscflags)
 *
 * Description:
 *	Free memory allocated by dlpibase_alloc().
 *
 * Calling/Exit status:
 *	No locking assumptions.
 *
 * Can't be STATIC as called by other Nonconform.c files
 */
void
dlpibase_free(void *ptr, int memtype, int miscflags)
{
	UINT32	*tmpptr;
	UINT32	nbytes;

	ASSERT((miscflags == DLPIBASE_KMEM) || (miscflags == DLPIBASE_ODIMEM));

	/*
	 * check start marker.
	 */
	tmpptr = (UINT32 *)((UINT32)ptr - sizeof(UINT32));
	if ((*(UINT32 *)tmpptr) != DLPIBASE_BUFSTART_MARK) {
		cmn_err(CE_WARN,
		"dlpibase_free: start marker wrong. buffer corrupted.\n");
	}

	/*
	 * check buffer size.
	 */
	tmpptr = (UINT32 *)((UINT32)ptr - (2 * sizeof(UINT32)));
	nbytes = *tmpptr;
	if (nbytes <= 0) {
		cmn_err(CE_PANIC,
		"dlpibase_free: nbytes is negative, has been stepped on.\n");
	}

	/*
	 * check end marker.
	 */
	tmpptr = (UINT32 *)((UINT32)ptr + nbytes - (3 * sizeof(UINT32)));
	if ((*(UINT32 *)tmpptr) != DLPIBASE_BUFEND_MARK) {
		cmn_err(CE_WARN,
		"dlpibase_free: end marker wrong. buffer corrupted.\n");
	}

	/*
	 * free allocated buffer from start.
	 */
	tmpptr = (UINT32 *)((UINT32)ptr - (2 * sizeof(UINT32)));

	switch (memtype) {
		case DLPIBASE_MEM_DMA_LOGICALTOPHYSICAL_BELOW16:
#if OLDWAY
			lsl_31_free(tmpptr);
#endif
			cmn_err(CE_PANIC,"dlpibase_free: DLPIBASE_MEM_DMA_LOGICALTOPHYSICAL_BELOW16 not supported");
			/* NOTREACHED */

			break;

		case DLPIBASE_MEM_DMA:
		case DLPIBASE_MEM_DMA_BELOW16:
			if (miscflags == DLPIBASE_ODIMEM) 
			{
#if OLDWAY
			    odimem_free(tmpptr, nbytes);
#endif
			    cmn_err(CE_PANIC,"dlpibase_free: miscflags=DLPIBASE_ODIMEM not supported");
			} else  
			{
			    kmem_free(tmpptr, nbytes);
			}

			break;

		default:
			kmem_free(tmpptr, nbytes);

			break;
	}
}

/*
 * dlpibase_free_wrapper_none(void *ptr)
 *
 * Description:
 *	Free memory allocated by dlpibase_alloc() using DLPIBASE_MEM_NONE/
 *	DLPIBASE_MEM_NONE_BELOW16/DLPIBASE_MEM_NONE_PAGEALIGN/
 *	DLPIBASE_MEM_NONE_PAGEALIGN_BELOW16.
 *
 * Calling/Exit status:
 *	No locking assumptions.
 */
STATIC void
dlpibase_free_wrapper_none(void *ptr)
{
	dlpibase_free(ptr, DLPIBASE_MEM_NONE, DLPIBASE_KMEM);
}

/*
 * dlpibase_free_wrapper_31_dma(void *ptr)
 *
 * Description:
 *	Free memory allocated by dlpibase_alloc() using
 *	DLPIBASE_MEM_DMA_LOGICALTOPHYSICAL_BELOW16.
 *
 * Calling/Exit status:
 *	No locking assumptions.
 */
STATIC void
dlpibase_free_wrapper_31_dma(void *ptr)
{
#if OLDWAY
	dlpibase_free(ptr, DLPIBASE_MEM_DMA_LOGICALTOPHYSICAL_BELOW16, DLPIBASE_KMEM);
#endif
	cmn_err(CE_PANIC,"dlpibase_free_wrapper_31_dma: DLPIBASE_MEM_DMA_LOGICALTOPHYSICAL_BELOW16 is unsupported");
}

/*
 * dlpibase_free_wrapper_dma(void *ptr)
 *
 * Description:
 *	Free memory allocated by dlpibase_alloc() using DLPIBASE_MEM_DMA.
 *
 * Calling/Exit status:
 *	No locking assumptions.
 */
STATIC void
dlpibase_free_wrapper_dma(void *ptr)
{
	UINT32	*tmpptr;
	UINT32	nbytes;

	/*
	 * get buffer size.
	 */
	tmpptr = (UINT32 *)((UINT32)ptr - (2 * sizeof(UINT32)));
	nbytes = *tmpptr;
	if (nbytes <= 0) {
		cmn_err(CE_PANIC,
	"dlpibase_free_wrapper_dma: nbytes is negative,has been stepped on.\n");
	}

	if ((nbytes - (3 * sizeof(UINT32))) > PAGESIZE) {
		dlpibase_free(ptr, DLPIBASE_MEM_DMA, DLPIBASE_ODIMEM);
	} else {
		dlpibase_free(ptr, DLPIBASE_MEM_DMA, DLPIBASE_KMEM);
	}
}

/*
 * dlpibase_free_wrapper_dma_below16(void *ptr)
 *
 * Description:
 *    Free memory allocated by dlpibase_alloc() using DLPIBASE_MEM_DMA_BELOW16.
 *
 * Calling/Exit status:
 *	No locking assumptions.
 */
STATIC void
dlpibase_free_wrapper_dma_below16(void *ptr)
{
	UINT32	*tmpptr;
	UINT32	nbytes;

	/*
	 * get buffer size.
	 */
	tmpptr = (UINT32 *)((UINT32)ptr - (2 * sizeof(UINT32)));
	nbytes = *tmpptr;
	if (nbytes <= 0) {
		cmn_err(CE_PANIC,
	"dlpibase_free_wrapper_dma: nbytes is negative,has been stepped on.\n");
	}

	if ((nbytes - (3 * sizeof(UINT32))) > PAGESIZE) {
		dlpibase_free(ptr, DLPIBASE_MEM_DMA_BELOW16, DLPIBASE_ODIMEM);
	} else {
		dlpibase_free(ptr, DLPIBASE_MEM_DMA_BELOW16, DLPIBASE_KMEM);
	}
}

/*
 * dlpibase_free_wrapper_dma_pagealign(void *ptr)
 *
 * Description:
 *   Free memory allocated by dlpibase_alloc() using DLPIBASE_MEM_DMA_PAGEALIGN.
 *
 * Calling/Exit status:
 *	No locking assumptions.
 */
STATIC void
dlpibase_free_wrapper_dma_pagealign(void *ptr)
{
	dlpibase_free(ptr, DLPIBASE_MEM_DMA_PAGEALIGN, DLPIBASE_KMEM);
}

/*
 * dlpibase_free_wrapper_dma_pagealign_below16(void *ptr)
 *
 * Description:
 *	Free memory allocated by dlpibase_alloc() using
 *	DLPIBASE_MEM_DMA_PAGEALIGN_BELOW16.
 *
 * Calling/Exit status:
 *	No locking assumptions.
 */
STATIC void
dlpibase_free_wrapper_dma_pagealign_below16(void *ptr)
{
	dlpibase_free(ptr, DLPIBASE_MEM_DMA_PAGEALIGN_BELOW16, DLPIBASE_KMEM);
}

/*
 * mdi_set_frtn()
 *
 * Description:
 *	Attach the correct wrapper to frtn_t struct.
 *
 * Calling/Exit status:
 *	No locking assumptions.
 *
 * Purpose:  This function would be called if you have a kmem_alloc'ed buffer
 * and you want to turn it into an esballoc'ed message so that the
 * streams daemon (function strdaemon()) can later free it up.  the
 * streams daemon calls the free_func passed in the frtn argument.
 * May be useful for MDI drivers someday.
 */
void
mdi_set_frtn(frtn_t *frtn, void *arg, int memtype)
{
	/*
	 * first set the arg.
	 */
	frtn->free_arg = arg;
	
	/*
	 * now the free wrapper.
	 */
	switch (memtype) {
		case DLPIBASE_MEM_NONE:
		case DLPIBASE_MEM_NONE_BELOW16:
		case DLPIBASE_MEM_NONE_PAGEALIGN:
		case DLPIBASE_MEM_NONE_PAGEALIGN_BELOW16:
			frtn->free_func = dlpibase_free_wrapper_none;

			break;

		case DLPIBASE_MEM_DMA_LOGICALTOPHYSICAL_BELOW16:
#if OLDWAY
			frtn->free_func = dlpibase_free_wrapper_31_dma;
#endif
			cmn_err(CE_PANIC,"dlpibase_set_frtn: DLPIBASE_MEM_DMA_LOGICALTOPHYSICAL_BELOW16 is unsupported");
			/* NOTREACHED */

			break;
		case DLPIBASE_MEM_DMA:
			frtn->free_func = dlpibase_free_wrapper_dma;

			break;

		case DLPIBASE_MEM_DMA_BELOW16:
			frtn->free_func = dlpibase_free_wrapper_dma_below16;

			break;
		case DLPIBASE_MEM_DMA_PAGEALIGN:
			frtn->free_func = dlpibase_free_wrapper_dma_pagealign;

			break;

		case DLPIBASE_MEM_DMA_PAGEALIGN_BELOW16:
			frtn->free_func =
				dlpibase_free_wrapper_dma_pagealign_below16;

			break;

		default:
			cmn_err(CE_PANIC, 
				"dlpibase_set_frtn: illegal memtype 0x%x.\n",
				memtype);

			break;
	}
}
