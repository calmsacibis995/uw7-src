#ifndef _FS_BUF_F_H	/* wrapper symbol for kernel use */
#define _FS_BUF_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/buf_f.h	1.12.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Family-specific buf struct layout.
 */

#if defined _KERNEL || defined _KMEMUSER

/*
 * The buf structure is defined here, in terms of #define's for individual
 * fields given in fs/buf.h.  Additional fields, mostly padding, are inserted
 * for binary compatibility.
 */

struct buf {
	_B_FLAGS
	_B_FORW
	_B_BACK
	_B_AVFORW
	_B_AVBACK
	ushort_t _b_pad1; /* was _b_scgth_count */
	_B_BLKOFF
	_B_BCOUNT
	_B_ADDR
	_B_BLKNO
	char _b_pad2;	/* was _b_oerror; removed as we drop DDI1 compat. */
	_B_NUMPAGES
	_B_ADDRTYPE
	_B_ORIG_TYPE
	_B_RESID
	_B_PRIV2
	_B_START
	_B_PROC
	_B_PAGES
	_B_RELTIME
	_B_BUFSIZE
	_B_IODONE
	_B_MISC
	_B_CHILDCNT
	_B_ORIG_ADDR
	_B_ERROR
	_B_EDEV
	_B_PRIV
	_B_WRITESTRAT
	_B_IOWAIT
	_B_AVAIL
	_B_SCGTH
	_B_CBLKNO
	_B_CHANNELP
	_B_BCB_ID
};

/*
 * Old names for some fields for compatibility.
 */
#if !defined(_DDI) || _DDI < 8
#define b_private	b_priv.un_ptr
#define b_sector	b_priv2.un_daddr
#endif /* _DDI */

/*
 * Structure used for the head of buffer hash chains.
 * We only need the first three fields of a buffer for these,
 * so this abbreviated definition saves some space.
 * To make indexing into an array of hbuf structs more efficient,
 * we pad the structure out to a power of 2.
 *
 * This structure, and the fields b_flags, b_forw, and b_back must
 * be present for all architectures, and the fields must have the same
 * offsets as corresponding buf structure fields.  The structure is
 * defined at the family level since the padding and (potentially) the
 * field offsets are not generic.
 */
struct hbuf {
	uint_t	hb_flags;
	buf_t	*hb_forw;
	buf_t	*hb_back;
	int	hb_pad;			/* round size to 2^n */
};

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

/*
 * Hooks for physio pre- and post-processing, to allow for systems
 * which need to set up special mappings for B_PHYS buffers before
 * handing them to drivers.
 *
 * PHYSIO_START() is used whenever a strategy routine is about to be called
 * for a B_PHYS buffer (i.e. a buffer with user virtual b_un.b_addr).  It
 * should return the strategy routine which should be called, usually the
 * actual driver strategy routine passed in the 'strat' argument.
 *
 * PHYSIO_DONE() is used after the B_PHYS I/O has completed,
 * and we've waited for it with biowait(); this can be used to unmap
 * special mappings.
 *
 * For i386, special processing is dynamic; normally there is no special
 * processing; if needed, the caller provides a physio_start function
 * and assigns it to u.u_physio_start, setting u.u_physio_start back to
 * NULL when done.
 */

#define PHYSIO_START(bp, strat)		\
	(u.u_physio_start ? \
		(*u.u_physio_start)(bp, strat) : \
		(strat))

#define PHYSIO_DONE(bp, addr, count)	/**/

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_BUF_F_H */
