#ident	"@(#)libthread:i386/lib/libthread/archdep/lwppriv.c	1.3"
#include <libthread.h>

lwppriv_block_t *_thr_free_lwppriv_block = (lwppriv_block_t *)0;
lwp_mutex_t _thr_lwpprivatelock;

/*
 * __lwp_desc_t *
 * _thr_lwp_allocprivate()
 *	This function is called by _thr_lwpcreate() to allocate lwp private 
 *	data area.
 * Parameter/Calling State:
 *	Caller's signal handlers must be disabled upon entry.
 *
 * Return Values/Exit State:
 *	Returns a pointer to the lwp private data area.
 */
__lwp_desc_t *
_thr_lwp_allocprivate()
{
	lwppriv_block_t *lp;
	int chunksize;
	int i;
	
	ASSERT(THR_ISSIGOFF(curthread));

	_lwp_mutex_lock(&_thr_lwpprivatelock);
	if ((lp = _thr_free_lwppriv_block) == (lwppriv_block_t *)0) {
		chunksize = (sizeof (lwppriv_block_t) + PAGESIZE - 1)
			    / PAGESIZE * PAGESIZE;
		if (_thr_alloc_chunk(0, chunksize, (caddr_t *)&lp) == 0) {
			_lwp_mutex_unlock(&_thr_lwpprivatelock);
			_thr_panic("_thr_lwp_allocprivate: _thr_alloc_chunk");
		}
		_thr_free_lwppriv_block = lp;
		for (i = 0; i < chunksize - (sizeof (lwppriv_block_t) * 2);
		     i += sizeof (lwppriv_block_t)) {
			ASSERT(lp->l_next == (lwppriv_block_t *)0);
			lp->l_next = lp + 1;
			lp++;
		}
		ASSERT(lp->l_next == (lwppriv_block_t *)0);
		lp = _thr_free_lwppriv_block;
	}
	_thr_free_lwppriv_block = lp->l_next;
	_lwp_mutex_unlock(&_thr_lwpprivatelock);
	return (&lp->l_lwpdesc);
}

/*
 * void 
 * _thr_lwp_freeprivate(__lwp_desc_t *ldp)
 *
 *      This function is called by _thr_lwpcreate() to free lwp private
 *      data area.
 * Parameter/Calling State:
 *      Caller's signal handlers must be disabled upon entry.
 *
 * Return Values/Exit State:
 *      Caller's signal handlers are still disabled on exit.
 */

void
_thr_lwp_freeprivate(__lwp_desc_t *ldp)
{
	lwppriv_block_t *lp = (lwppriv_block_t *)ldp;

	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(ldp != (__lwp_desc_t *) NULL);

	_lwp_mutex_lock(&_thr_lwpprivatelock);
	lp->l_next = _thr_free_lwppriv_block;
	_thr_free_lwppriv_block = lp;
	_lwp_mutex_unlock(&_thr_lwpprivatelock);
}
