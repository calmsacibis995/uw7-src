#ident	"@(#)ws_cmap.c	1.21"
#ident	"$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * Modification history
 *
 *	L000	21Mar97		rodneyh@sco.com
 *	- Changes for ALT lock key support.
 *	  Added K_ALK to switches in ws_statekey and ws_shiftkey.
 *	  Added ALT_LOCK checking to ws_getstate.
 *
 */


#include <io/ascii.h>
#include <io/ansi/at_ansi.h>
#include <io/kd/kd.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <io/termio.h>
#include <io/termios.h>
#include <io/ws/chan.h>
#include <io/ws/vt.h>
#include <io/ws/ws.h>
#include <io/xque/xque.h>
#include <mem/kmem.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <io/ddi.h>


extern int	xlate_keymap(keymap_t *, keymap_t *, int);

pfxstate_t	kdpfxstr;

extern stridx_t	kdstrmap;
extern strmap_t	kdstrbuf;
extern keymap_t	kdkeymap;
extern extkeys_t ext_keys;
extern esctbl_t	kdesctbl;
extern esctbl2_t kdesctbl2;
extern srqtab_t	srqtab;
extern struct kb_shiftmkbrk kb_mkbrk[];
extern ushort kb_shifttab[];
extern struct attrmask kb_attrmask[];
extern int nattrmsks;

extern wstation_t Kdws;



/*
 * stridx_t *
 * ws_dflt_strmap_p(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Reset string table indices.
 *
 *	kdstrmap[] maps in the index to the kdstrbuf[].
 *	The string index table is not pre-initialized like kdkeymap.
 *	This routine initializes it.
 */
stridx_t *
ws_dflt_strmap_p(void)
{
	int	strix = 0;		/* index into string buffer */
	int	key;			/* function key number */
	static int init_flg = 0;


	ASSERT(getpl() == plstr || getpl() == plhi);

	if (init_flg) { 
		return((stridx_t *) kdstrmap);
	}

	init_flg++;

	/* make sure buffer ends with null */
	kdstrbuf[STRTABLN - 1] = '\0';

	for (key = 0; key < NSTRKEYS; key++) {
		/* point to start of string */
		kdstrmap[key] = (ushort) strix;	
		/* Search for start of next string */
		while (strix < STRTABLN - 1 && kdstrbuf[strix++])
			;
	}

	return ((stridx_t *) kdstrmap);
}


/*
 * int
 * ws_newscrmap(scrn_t *, int )
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *	- The scrn mutex lock is acquired and held across the function.
 */
int
ws_newscrmap(scrn_t *scrp, int km_flag)
{
	scrnmap_t *nscrp;
	pl_t	opl;


	opl = LOCK(scrp->scr_mutex, plstr);

	if (scrp->scr_map_p && 
			(scrp->scr_map_p != scrp->scr_defltp->scr_map_p)) {
		UNLOCK(scrp->scr_mutex, opl);
		return (1);
	}

	if (nscrp = kmem_alloc(sizeof(scrnmap_t), km_flag)) {
		scrp->scr_map_p = nscrp;
		UNLOCK(scrp->scr_mutex, opl);
		return (1);
	}

	UNLOCK(scrp->scr_mutex, opl);

	return (0);
}


/*
 * int
 * ws_newkeymap(charmap_t *, ushort, keymap_t *, int, pl_t)
 *
 * Calling/Exit State:
 *	- The charmap basic lock is acquired and held across the function. 
 *	  The interrupt level at which this lock is acquired is passed as
 *	  an argument to the function. This is necessary since the function
 *	  is called from kdinit() in which case the ipl must not change and 
 *	  from the CHAR module in which case the charmap lock is acquired
 *	  at plstr level. Thus to facilitate both the needs the caller of
 *	  this function sends the ipl as an extra argument.
 *
 * Description:
 *	If the current keymap in use is the default,
 *	ws_newkeymap() allocates a new keymap_t structure
 *	and copies the user keymap_t structure used for
 *	translating. Otherwise, it simply overlays the
 *	existing keymap with the user keymap.
 */
int
ws_newkeymap(charmap_t *cmp, ushort nkeys, keymap_t *nkmp, 
			int km_flag, pl_t pl)
{
	int		size;
	keymap_t	*okmp;
	charmap_t	*dcmp;
	pl_t		opl;


	opl = LOCK(cmp->cr_mutex, pl);

	okmp = cmp->cr_keymap_p;
	dcmp = cmp->cr_defltp;

	if (okmp == dcmp->cr_keymap_p) {
		okmp = (keymap_t *) kmem_alloc(sizeof(keymap_t), km_flag);
		if (okmp == (keymap_t *) NULL) {
			UNLOCK(cmp->cr_mutex, opl);
			return (0);
		}
	}

	size = nkeys * sizeof(okmp->key[0]) + sizeof(okmp->n_keys);

	ASSERT(getpl() == plstr || getpl() == plhi);

        if (cmp->cr_flags == SCO_FORMAT) {
		xlate_keymap(nkmp, okmp, USL_FORMAT);
#ifdef DEBUG
		cmn_err(CE_NOTE, "SCO keymap installed");
#endif
	} else
		bcopy(nkmp, okmp, size);

	cmp->cr_flags = USL_FORMAT;
	cmp->cr_keymap_p = okmp;

	UNLOCK(cmp->cr_mutex, opl);

	return (1);
}


/*
 * int
 * ws_newsrqtab(charmap_t *, int, pl_t)
 *
 * Calling/Exit State:
 *	- The charmap basic lock is acquired and held across the function. 
 *	  The interrupt level at which this lock is acquired is passed as
 *	  an argument to the function. This is necessary since the function
 *	  is called from kdinit() in which case the ipl must not change and 
 *	  from the CHAR module in which case the charmap lock is acquired
 *	  at plstr level. Thus to facilitate both the needs the caller of
 *	  this function sends the ipl as an extra argument.
 */
int
ws_newsrqtab(charmap_t *cmp, int km_flag, pl_t pl)
{
	srqtab_t	*nsrqp, *osrqp;
	charmap_t	*dcmp;
	pl_t		opl;


	opl = LOCK(cmp->cr_mutex, pl);

	dcmp = cmp->cr_defltp;
	nsrqp = osrqp = cmp->cr_srqtabp;

	if (osrqp == dcmp->cr_srqtabp) {
		nsrqp = (srqtab_t *) kmem_alloc(sizeof(srqtab_t), km_flag);
		if (nsrqp == (srqtab_t *) NULL) {
			UNLOCK(cmp->cr_mutex, opl);
			return (0);
		}

		ASSERT(getpl() == plstr || getpl() == plhi);

		bcopy(osrqp, nsrqp, sizeof(srqtab_t));
		cmp->cr_srqtabp = nsrqp;
	}

	UNLOCK(cmp->cr_mutex, opl);

	return (1);
}


/*
 * int
 * ws_newstrbuf(charmap_t *, int, pl_t)
 *
 * Calling/Exit State:
 *	- The charmap basic lock is acquired and held across the function. 
 *	  The interrupt level at which this lock is acquired is passed as
 *	  an argument to the function. This is necessary since the function
 *	  is called from kdinit() in which case the ipl must not change and 
 *	  from the CHAR module in which case the charmap lock is acquired
 *	  at plstr level. Thus to facilitate both the needs the caller of
 *	  this function sends the ipl as an extra argument.
 */
int
ws_newstrbuf(charmap_t *cmp, int km_flag, pl_t pl)
{
	strmap_t	*osbp, *nsbp;
	stridx_t	*osip, *nsip;
	charmap_t	*dcmp;
	pl_t		opl;


	opl = LOCK(cmp->cr_mutex, pl);

	dcmp = cmp->cr_defltp;
	nsbp = osbp = cmp->cr_strbufp;
	nsip = osip = cmp->cr_strmap_p;

	if (osbp == dcmp->cr_strbufp) {
		nsbp = (strmap_t *) kmem_alloc(sizeof(strmap_t), km_flag);
		if (nsbp == (strmap_t *) NULL) {
			UNLOCK(cmp->cr_mutex, opl);
			return (0);
		}

		nsip = (stridx_t *) kmem_alloc(sizeof(stridx_t), km_flag);
		if (nsip == (stridx_t *) NULL) {
			UNLOCK(cmp->cr_mutex, opl);
			kmem_free(nsbp, sizeof(strmap_t));
			return (0);

		}

		ASSERT(getpl() == plstr || getpl() == plhi);

		bcopy(osbp, nsbp, sizeof(strmap_t));
		bcopy(osip, nsip, sizeof(stridx_t));
		cmp->cr_strbufp = nsbp;
		cmp->cr_strmap_p = nsip;
	}

	UNLOCK(cmp->cr_mutex, opl);

	return (1);
}


/*
 * void
 * ws_strreset(charmap_t *)
 * 
 * Calling/Exit State:
 *	- The charmap basic lock is acquired and held across the function.
 */
void
ws_strreset(charmap_t *cmp)
{
	int	strix;		/* Index into string buffer */
	ushort	key;		/* Function key number */
	ushort	*idxp;
	unchar	*bufp;
	pl_t	pl;


	pl = LOCK(cmp->cr_mutex, plstr);

	bufp = (unchar *) cmp->cr_strbufp;
	idxp = (ushort *) cmp->cr_strmap_p;

	ASSERT(getpl() == plstr || getpl() == plhi);

	bufp[STRTABLN - 1] = '\0';	/* Make sure buffer ends with null */
	strix = 0;			/* Start from beginning of buffer */

	for (key = 0; (int) key < NSTRKEYS; key++) {	
		idxp[key] = (ushort)strix;	/* Point to start of string */
		while (strix < STRTABLN - 1 && bufp[strix++])
			;			/* Find start of next string */
	}

	UNLOCK(cmp->cr_mutex, pl);

	return;
}


/*
 * int
 * ws_addstring(charmap_t *, ushort, unchar *, ushort);
 *
 * Calling/Exit State:
 *	- Return 1 on success, 0 on failure.
 *	- The charmap basic lock is acquired and held across the function.
 *
 * Description:
 *	Add string to function key table.
 */
int
ws_addstring(charmap_t *cmp, ushort keynum, unchar *str, ushort len)
{
	int	amount;		/* Amount to move */
	int	cnt;
	int	oldlen;		/* Length of old string */
	unchar	*oldstr;	/* Location of old string in table */
	unchar	*to;		/* Destination of move */
	unchar	*from;		/* Source of move */
	unchar	*bufend;	/* End of string buffer */
	unchar	*bufbase;	/* Beginning of buffer */
	unchar	*tmp;		/* Temporary pointer into old string */
	ushort	*idxp;
	pl_t	pl;
	

	if ((int) keynum >= NSTRKEYS)	/* Invalid key number? */
		return 0;		/* Ignore string setting */

	len++;				/* Adjust length to count end null */

	if (!ws_newstrbuf(cmp, KM_NOSLEEP, plstr))
		return 0;

	pl = LOCK(cmp->cr_mutex, plstr);

	idxp = (ushort *) cmp->cr_strmap_p;
	idxp += keynum;

	oldstr = (unchar *) cmp->cr_strbufp;
	oldstr += *idxp;
	/*
	 * Now oldstr points at beginning of old string for key. 
	 */
	bufbase = (unchar *) cmp->cr_strbufp;
	bufend = bufbase + STRTABLN - 1;

	tmp = oldstr;
	while (*tmp++ != '\0')		/* Find end of old string */
		;

	oldlen =  tmp - oldstr;		/* Compute length of string + null */

	/*
	 * If lengths are different, expand or contract table to fit.
	 */
	if (oldlen > (int) len) {	/* Move up for shorter string? */
		from = oldstr + oldlen;	/* Calculate source */
		to = oldstr + len;	/* Calculate destination */
		amount = STRTABLN - (oldstr - bufbase) - oldlen;
		ASSERT(getpl() == plstr || getpl() == plhi);
		for (cnt = amount; cnt > 0; cnt--)
			*to++ = *from++;
	} else if (oldlen < (int) len) {	/* Move down for longer string? */
		from = bufend - (len - oldlen);	/* Calculate source */
		to = bufend;			/* Calculate destination */
		if (from < (oldstr + len)) {	/* String won't fit? */
			UNLOCK(cmp->cr_mutex, pl);
			return 0;		/* Return without doing anything */
		}

		amount	= STRTABLN - (oldstr - bufbase) - len; /* Move length */
		ASSERT(getpl() == plstr || getpl() == plhi);
		while (--amount >= 0)		/* Copy whole length */
			*to-- = *from--;	/* Copy character at a time */
	}

	len--;				/* Remove previous addition for null */
	ASSERT(getpl() == plstr || getpl() == plhi);
	bcopy(str, oldstr, len);	/* Install new string over old */
	*(oldstr + len)  = '\0';	/* Terminate string with null */

	UNLOCK(cmp->cr_mutex, pl);

	ws_strreset(cmp);		/* Readjust string index table */

	return 1;
}


/*
 * int
 * ws_newpfxstr(charmap_t *, int, pl_t)
 *
 * Calling/Exit State:
 *	- Return 1 on success, 0 on failure.
 *	- The charmap basic lock is acquired and held across the function. 
 *	  The interrupt level at which this lock is acquired is passed as
 *	  an argument to the function. This is necessary since the function
 *	  is called from kdinit() in which case the ipl must not change and 
 *	  from the CHAR module in which case the charmap lock is acquired
 *	  at plstr level. Thus to facilitate both the needs the caller of
 *	  this function sends the ipl as an extra argument.
 */
int
ws_newpfxstr(charmap_t *cmp, int km_flag, pl_t pl)
{
	pfxstate_t	*npfxp, *opfxp;
	charmap_t	*dcmp;
	pl_t		opl;


	opl = LOCK(cmp->cr_mutex, pl);

	dcmp = cmp->cr_defltp;
	npfxp = opfxp = cmp->cr_pfxstrp;

	if (opfxp == dcmp->cr_pfxstrp) {
		npfxp = (pfxstate_t *) kmem_alloc(sizeof(pfxstate_t), km_flag);
		if (npfxp == (pfxstate_t *) NULL) {
			UNLOCK(cmp->cr_mutex, opl );
			return (0);
		}

		ASSERT(getpl() == plstr || getpl() == plhi);

		bcopy(opfxp, npfxp, sizeof(pfxstate_t));
		cmp->cr_pfxstrp = npfxp;
	}

	UNLOCK(cmp->cr_mutex, opl);

	return (1);
}


/*
 * void
 * ws_scrn_init(wstation_t *wsp, int km_flag)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 *
 */
/* ARGSUSED */
void
ws_scrn_init(wstation_t *wsp, int km_flag)
{
	scrn_t *scrp;


	ASSERT(getpl() == plstr || getpl() == plhi);

	scrp = &wsp->w_scrn;
	scrp->scr_defltp = scrp;
	scrp->scr_map_p = (scrnmap_t *) NULL;
}


/*
 * void
 * ws_cmap_init(wstation_t *, int)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 */
void
ws_cmap_init(wstation_t *wsp, int km_flag)
{
	charmap_t *cmp;


	ASSERT(getpl() == plstr || getpl() == plhi);

	cmp = &wsp->w_charmap;
	wsp->w_charmap.cr_defltp = cmp;
	cmp->cr_keymap_p = (keymap_t *) &kdkeymap;
	cmp->cr_extkeyp =  (extkeys_t *) ext_keys;
	cmp->cr_esctblp =  (esctbl_t *) kdesctbl;
        cmp->cr_esctbl2p =  (esctbl2_t *) kdesctbl2;
	cmp->cr_strbufp =  (strmap_t *) kdstrbuf;
	cmp->cr_srqtabp =  (srqtab_t *) srqtab;
	cmp->cr_strmap_p = ws_dflt_strmap_p();
	cmp->cr_pfxstrp = (pfxstate_t *) kdpfxstr;
        cmp->cr_flags   = USL_FORMAT;
        cmp->cr_kbmode  = KBM_XT;
	
	if (!ws_newkeymap(cmp, kdkeymap.n_keys, &kdkeymap, km_flag, getpl()))
		/*
		 *+ It is necessary to allocate space for keyboard map
		 *+ to allow for scancode to character set translation.
		 *+ ws_newkeymap() fails to allocate space during KD
		 *+ initialization.
		 */
		cmn_err(CE_PANIC, 
			"ws_cmap_init: Unable to allocate space for keyboard map");

	if (!ws_newsrqtab(cmp, km_flag, getpl()))
		/*
		 *+ It is necessary to allocate space for system request 
		 *+ table to recognize system request scancode sequence.
		 *+ ws_newsrqtab() fails to allocate space for such
		 *+ mapping during KD initialization.
		 */
		cmn_err(CE_PANIC, 
			"ws_cmap_init: Unable to allocate space for system request table");

	if (!ws_newstrbuf(cmp, km_flag, getpl()))
		/*
		 *+ It is necessary to allocate space for function keys
		 *+ mapping table. ws_newstrbuf() fails to allocate space
		 *+ for such function key mapping table during KD 
		 *+ initialization.
		 */
		cmn_err(CE_PANIC, 
			"ws_cmap_init: Unable to allocate space for function keys");

	if (!ws_newpfxstr(cmp, km_flag, getpl()))
		/*
		 *+ It is necessary to allocate space for prefix strings.
		 *+ ws_newpfxstr() fails to allocate space for such
		 *+ prefix string mapping during KD initialization.
		 */
		cmn_err(CE_PANIC,
			"ws_cmap_init: Unable to allocate space for prefix strings");
}


/*
 * void 
 * ws_scrn_alloc(wstation_t *, ws_channel_t *)
 * 	Initialize per-channel state structure for screen map. 
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode. However, its
 *	  sufficient to hold the lock in shared mode.
 */
void
ws_scrn_alloc(wstation_t *wsp, ws_channel_t *chp)
{
	ASSERT(getpl() == plstr || getpl() == plhi);
	bcopy(&wsp->w_scrn, &chp->ch_scrn, sizeof(scrn_t));
}


/*
 * charmap_t *
 * ws_cmap_alloc(wstation_t *, int)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 *	- Return pointer to initialized charmap_t or NULL if kmem_alloc fails.
 */
charmap_t *
ws_cmap_alloc(wstation_t *wsp, int kmem_flag)
{
	charmap_t *cmp;


	cmp = (charmap_t *) kmem_alloc(sizeof(charmap_t), kmem_flag);
	if (cmp == (charmap_t *) NULL) 
		return ((charmap_t *) NULL);

	ASSERT(getpl() == plstr || getpl() == plhi);

	bcopy(&wsp->w_charmap, cmp, sizeof(charmap_t));
	return (cmp);
}


/*
 * void
 * ws_scrn_release(scrn_t *scrp)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 */
void
ws_scrn_release(scrn_t *scrp)
{
	scrn_t	*dcmp;
	pl_t	pl;


	if (scrp == (scrn_t *) NULL) 
		return;

	pl = LOCK(scrp->scr_mutex, plstr);

	dcmp = scrp->scr_defltp;

	if (dcmp == scrp || scrp->scr_map_p != dcmp->scr_map_p) {
		if (scrp->scr_map_p)
			kmem_free(scrp->scr_map_p, sizeof(scrnmap_t));
		scrp->scr_map_p = (scrnmap_t *) NULL;
	}

	UNLOCK(scrp->scr_mutex, pl);
}


/*
 * void
 * ws_cmap_release(charmap_t *cmp)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 *
 * Description:
 *	The translation table is only released if its different from
 *	the default translation table or it is the last close (kdclose).
 */
void
ws_cmap_release(charmap_t *cmp)
{
	charmap_t	*dcmp;
	int		same;
	pl_t		pl;


	if (cmp == (charmap_t *) NULL) 
		return;

	pl = LOCK(cmp->cr_mutex, plstr);

	dcmp = cmp->cr_defltp;
	same = (dcmp == cmp);
	if (same || cmp->cr_keymap_p != dcmp->cr_keymap_p)
		kmem_free(cmp->cr_keymap_p, sizeof(keymap_t));
	if (same || cmp->cr_extkeyp != dcmp->cr_extkeyp)
		kmem_free(cmp->cr_extkeyp, sizeof(extkeys_t));
	if (same || cmp->cr_esctblp !=  dcmp->cr_esctblp)
		kmem_free(cmp->cr_esctblp, sizeof(esctbl_t));
        if (same || cmp->cr_esctbl2p !=  dcmp->cr_esctbl2p)
                kmem_free(cmp->cr_esctbl2p, sizeof(esctbl2_t));
	if (same || cmp->cr_strbufp != dcmp->cr_strbufp)
		kmem_free(cmp->cr_strbufp, sizeof(strmap_t));
	if (same || cmp->cr_srqtabp != dcmp->cr_srqtabp)
		kmem_free(cmp->cr_srqtabp, sizeof(srqtab_t));
	if (same || cmp->cr_strmap_p != dcmp->cr_strmap_p)
		kmem_free(cmp->cr_strmap_p, sizeof(stridx_t));

	UNLOCK(cmp->cr_mutex, pl);
}


/*
 * void
 * ws_scrn_free(wstation_t *, ws_channel_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 */
/* ARGSUSED */
void
ws_scrn_free(wstation_t *wsp, ws_channel_t *chp)
{
	ws_scrn_release(&chp->ch_scrn);
	return;
}


/* 
 * void
 * ws_cmap_free(wstation_t *, charmap_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 *
 * Description:
 *	Release the allocated cmap structure. If any of the members
 *	are not the default, assume that they were allocated via
 *	kmem_alloc and should be released via kmem_free.
 */
void
ws_cmap_free(wstation_t *wsp, charmap_t *cmp)
{
	if (cmp == (charmap_t *) NULL) 
		return;

	ws_cmap_release(cmp);

	if (cmp != &wsp->w_charmap) {
		kmem_free(cmp, sizeof(charmap_t));
	}

	return;
}


/*
 * void
 * ws_scrn_reset(wstation_t *, ws_channel_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 */
void
ws_scrn_reset(wstation_t *wsp, ws_channel_t *chp)
{
	ws_scrn_release(&chp->ch_scrn);
	ASSERT(getpl() == plstr);
	bcopy(&wsp->w_scrn, &chp->ch_scrn, sizeof(scrn_t));
}


/*
 * void
 * ws_cmap_reset(wstation_t *, charmap_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 */
/* ARGSUSED */
void
ws_cmap_reset(wstation_t *wsp, charmap_t *cmp)
{
	ws_cmap_release(cmp);
	ASSERT(getpl() == plstr);
	bcopy(cmp->cr_defltp, cmp, sizeof(charmap_t));
}


/*
 * void 
 * ws_kbtime(wstation_t *)
 *
 * Calling/Exit State:
 *	- No locks are held either on entry or exit.
 *
 * Description:
 *	Send the data upstream, if the data block read and
 *	writer pointer are unequal and the workstation read
 *	queue pointer is non null.
 */
void
ws_kbtime(wstation_t *wsp)
{
	mblk_t	*mp;
	pl_t	pl;


	pl = LOCK(wsp->w_mutex, plstr);

	wsp->w_timeid = 0;
	mp = wsp->w_mp;

	if (mp) {		/* something to send up */
		if (mp->b_wptr != mp->b_rptr) {
			if (wsp->w_qp == (queue_t *)NULL)
				freemsg(wsp->w_mp);
			else {
				UNLOCK(wsp->w_mutex, pl);
				putnext(wsp->w_qp, mp);
				pl = LOCK(wsp->w_mutex, plstr);
			}
			wsp->w_mp = allocb(4, BPRI_MED);
		}
	}

	UNLOCK(wsp->w_mutex, pl);
}


/*
 * int
 * ws_enque(queue_t *, mblk_t **, unchar, pl_t)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *	- Place the scancode in the message block, and return 1 if there
 *	  is still room left in the message block. Return 0 if the data
 *	  was sent up or dropped (used to indicate whether a timer should
 *	  be set to make sure the data gets sent up).
 *	- Arguments:
 *		<qp> is the active channel queue pointer.
 *		<scan> is the scancode that is to be sent upstream.
 *		<mpp> is a pointer to workstation message block pointer.
 *		<pl> is the incoming/callers priority level.
 *
 * Note: 
 *	Since this routine is also called from an interrupt handler;
 *	the priority level at which the lock is acquired must not
 *	lower/raise the current ipl. Ideally it is required to take
 *	the max of current ipl and plstr; but its not permissible 
 *	because the ipl may be mapped differently on different systems.
 *	Thus an extra argument indicating the priority level at which
 *	the lock is acquired is passed in to the function.
 */
int
ws_enque(queue_t *qp, mblk_t **mpp, unchar scan, pl_t pl)
{
	mblk_t *mp;


	if (!mpp || !qp)
		return (0);		/* bail out if invalid qp or mpp */

	(void) LOCK(Kdws.w_mutex, pl);

	/*
	 * Make sure message block is available to send the scancode
	 * upstream, otherwise allocate one.
	 */
	if (!(mp = *mpp)) {
		if ((*mpp = allocb(4, BPRI_HI)) ==  (mblk_t *)NULL) {
			UNLOCK(Kdws.w_mutex, pl);
			return (0);
		}
		mp = *mpp;
	}

	*mp->b_wptr++ = scan;

	if (mp->b_wptr == mp->b_datap->db_lim) {
		/*
		 * Allocate a new message block before sending the existing
		 * block upstream. This is to prevent a race condition between
		 * data being sent upstream and new incoming data.
		 */
		*mpp = allocb(4, BPRI_MED);
		UNLOCK(Kdws.w_mutex, pl);
		putnext(qp, mp);
		return (0);
	}

	UNLOCK(Kdws.w_mutex, pl);

	return (1);
}


/*
 * unchar
 * ws_procscan(charmap_t *, kbstate_t *, unchar)
 *	Translate raw scan code into stripped, usable form.
 *
 * Calling/Exit State:
 *	- charmap basic lock is held across the function. 
 */
unchar
ws_procscan(charmap_t *chmp, kbstate_t *kbp, unchar scan)
{
	int	indx;			/* index */
	unchar	stscan;			/* stripped scan code */
	unchar	oldprev;		/* old previous scan code */

	
        stscan = scan & ~KBD_BREAK;	/* strip break bit form scan code */
	oldprev = kbp->kb_prevscan;	/* get local copy of previous scan */
	kbp->kb_prevscan = scan;	/* current scan becomes previous */

	if (oldprev == 0xe1) {		/* previous scan code was 0xe1? */
		if (stscan == 0x1d)	/* second byte of 0xe1 sequence? */
			kbp->kb_prevscan = oldprev;	/* ignore and reset previous */
		else if (stscan == 0x45)/* third byte of 0xe1 sequence */
			return (0x77);	/* return scan code for break key */
	} else if (oldprev == 0xe0) {	/* previous scan code was 0xe0? */
		/* search table */
		for (indx = 0; indx < ESCTBLSIZ; indx++) {
			/* found code? */
			if ((*chmp->cr_esctblp)[indx][0] == stscan)
				/* translate */
				return ((*chmp->cr_esctblp)[indx][1]);
		}
	} else if (scan != 0xe0 && scan != 0xe1) {	/* not lead-in code? */
                for (indx = 0; indx < ESCTBL2SIZ; indx++) {
                        if ((*chmp->cr_esctbl2p)[indx][0] == stscan)
                                return((*chmp->cr_esctbl2p)[indx][1]);
                }
		/* return stripped scan code */
		return (stscan);
	}

	/* return scan code for no character */
	return (0);
}


/*
 * void
 * ws_rstmkbrk(queue_t *, mblk_t **, ushort, ushort, pl_t)
 *	Reset the make/break state.
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Note: 
 *	Since this routine is also called from an interrupt 
 *	handler; the priority level at which the lock is 
 *	acquired must not lower/raise the current ipl.
 *	Ideally it is required to take the max of current ipl 
 *	and plstr; but its not permissible because the ipl
 *	may be mapped differently on different systems, Thus it
 *	was decided to pass an extra argument to the function
 *	which would decide the priority level at which the
 *	lock should be acquired.
 */
void
ws_rstmkbrk(queue_t *qp, mblk_t **mpp, ushort state, ushort mask, pl_t pl)
{
	int	cnt, make;
	unchar	prfx = 0xe0;


	for (cnt = 0; cnt < 6; cnt++) {		/* non-toggles */
		if (!(mask & kb_mkbrk[cnt].mb_mask))
			continue;

		if (kb_mkbrk[cnt].mb_prfx)
			(void) ws_enque(qp, mpp, prfx, pl);

		make = (state & kb_mkbrk[cnt].mb_mask) ? 1 : 0;
		if (make) 
			(void) ws_enque(qp, mpp, kb_mkbrk[cnt].mb_make, pl);
		else
			(void) ws_enque(qp, mpp, kb_mkbrk[cnt].mb_break, pl);
	}
}

/*
 * void
 * ws_stnontog(queue_t *, mblk_t **, ushort, ushort, pl_t)
 *	Set the non toggle make/break state.
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Note: 
 *	Since this routine is also called from an interrupt 
 *	handler; the priority level at which the lock is 
 *	acquired must not lower/raise the current ipl.
 *	Ideally it is required to take the max of current ipl 
 *	and plstr; but its not permissible because the ipl
 *	may be mapped differently on different systems, Thus it
 *	was decided to pass an extra argument to the function
 *	which would decide the priority level at which the
 *	lock should be acquired.
 */
void
ws_stnontog(queue_t *qp, mblk_t **mpp, ushort ostate, ushort nstate, pl_t pl)
{
	int	cnt;
	unchar	prfx = 0xe0;
	ushort oshift, nshift;

	for (cnt = 0; cnt < 6 ; cnt++) {
		oshift = ostate & kb_mkbrk[cnt].mb_mask & NONTOGGLES;
		nshift = nstate & kb_mkbrk[cnt].mb_mask & NONTOGGLES;

		if (!(oshift ^ nshift))
			continue;

		if (kb_mkbrk[cnt].mb_prfx)
			ws_enque(qp, mpp, prfx, pl);

		if (nshift)
			ws_enque(qp, mpp, kb_mkbrk[cnt].mb_make, pl);
		else
			ws_enque(qp, mpp, kb_mkbrk[cnt].mb_break, pl);
	}
}

/*
 * ushort
 * ws_getstate(keymap_t *, kbstate_t *, unchar)
 *	get the current keyboard state.
 *
 * Calling/Exit State:
 *	- charmap basic lock is held. This lock is held
 *	  to protect the keymap table.
 */
ushort
ws_getstate(keymap_t *kmp, kbstate_t *kbp, unchar scan)
{
	ushort_t state = 0;


	if (kbp->kb_state & SHIFTSET)
		state |= SHIFTED;
	if (kbp->kb_state & CTRLSET)
		state |= CTRLED;
	if (kbp->kb_state & ALTSET)
		state |= ALTED;
	if ((kmp->key[scan].flgs & KMF_CLOCK && kbp->kb_state & CAPS_LOCK) ||
	    (kmp->key[scan].flgs & KMF_NLOCK && kbp->kb_state & NUM_LOCK))
		state ^= SHIFTED;	/* Locked - invert shift state */

	/* L000 begin
	 *
	 * The ALT_LOCK key is also affected by the KMF_CLOCK token, as for the
	 * other lock keys we invert the current ALT state.
	 *
	 */
	if(kmp->key[scan].flgs & KMF_CLOCK && kbp->kb_state & ALT_LOCK)
		state ^= ALTED;				/* L000 end */

	return (state);
}


/*
 * void
 * ws_xferkbstat(kbstate_t *, kbstate_t *)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Transfer the NONTOGGLE state.
 */
void
ws_xferkbstat(kbstate_t *okbp, kbstate_t *nkbp)
{
	ushort_t state;


	state = okbp->kb_state & NONTOGGLES;
	state |= nkbp->kb_state & (TOGGLESET | LEDMASK);
	nkbp->kb_state = state;
}


/*
 * ushort
 * ws_transchar(keymap_t *kmp, kbstate_t *kbp, unchar scan)
 *
 * Calling/Exit State:
 *	- charmap basic lock is held across the function.
 *
 * Note: 
 *	An equivalent WS_TRANSCHAR macro also exist in ws/ws.h. This function
 *	is here for compatibility reasons.
 */
ushort
ws_transchar(keymap_t *kmp, kbstate_t *kbp, unchar scan)
{
	return ((ushort)kmp->key[scan].map[ws_getstate(kmp, kbp, scan)]);
}


/*
 * int
 * ws_statekey(ushort, charmap_t *, kbstate_t *, unchar)
 *
 * Calling/Exit State:
 *	- charmap basic lock is held across the function.
 */
/* ARGSUSED */
int
ws_statekey(ushort ch, charmap_t *cmp, kbstate_t *kbp, unchar kbrk)
{
	ushort	shift;
	ushort	togls = 0;


	switch (ch) {
	case K_SLK: 
		togls = kbp->kb_togls;
		/* FALLTHROUGH */

	case K_ALT:
	case K_LAL:
	case K_RAL:
	case K_LSH:
	case K_RSH:
	case K_CTL:
	case K_CLK:
	case K_NLK:
	case K_LCT:
	case K_RCT:
	case K_KLK:
	case K_ALK:					/* L000 */
		shift = kb_shifttab[ch];
		break;

	case K_AGR:
		shift = kb_shifttab[K_ALT] | kb_shifttab[K_CTL];
		break;

	default:
		return (0);
	}

	if (kbrk) {
		if (shift & NONTOGGLES)
			kbp->kb_state &= ~shift;	/* state off */
		else
			kbp->kb_togls &= ~shift;	/* state off */
	} else {
		if (shift & NONTOGGLES)
			kbp->kb_state |= shift;		/* state on */
		else if (!(kbp->kb_togls & shift)) {
			kbp->kb_state ^= shift;		/* invert state */
			kbp->kb_togls |= shift;
		}
	}

	if ((ch == K_SLK) && !kbrk && (togls != kbp->kb_togls))
		return (0);

	return (1);
}


/*
 * int
 * ws_specialkey(keymap_t *, kbstate_t *, unchar)
 *
 * Calling/Exit State:
 *	- charmap basic lock is held. This lock is held
 *	  to protect the keymap table.
 *
 * Note: 
 *	An equivalent WS_SPECIALKEY macro also exist in ws/ws.h. This function
 *	is here for compatibility reasons.
 */
int
ws_specialkey(keymap_t *kmp, kbstate_t *kbp, unchar scan)
{
	return (IS_SPECKEY(kmp, scan, ws_getstate(kmp, kbp, scan)));
}


/*
 * ushort
 * ws_shiftkey(ushort, unchar, keymap_t *, kbstate_t *, unchar)
 *
 * Calling/Exit State:
 *	- charmap basic lock is held across the function to 
 *	  protect the keymap table. 
 */
ushort
ws_shiftkey(ushort ch, unchar scan, keymap_t *kmp, kbstate_t *kbp, unchar kbrk)
{
	ushort	shift;

	if (WS_SPECIALKEY(kmp, kbp, scan)) {
		switch (ch) {
		case K_LAL:
		case K_RAL:
		case K_LCT:
		case K_RCT:
		case K_SLK: 
		case K_ALT:
		case K_LSH:
		case K_RSH:
		case K_CTL:
		case K_CLK:
		case K_NLK:
		case K_KLK:
		case K_ALK:				/* L000 */
			shift = kb_shifttab[ch];
			if (kbrk) {
				if (shift & NONTOGGLES)
					kbp->kb_state &= ~shift;
				else
					kbp->kb_togls &= ~shift;
			} else {
				if (shift & NONTOGGLES)
					kbp->kb_state |= shift;
				else if (!(kbp->kb_togls & shift)) {
					kbp->kb_state ^= shift;
					kbp->kb_togls |= shift;
				}
			}

			return (shift);

		default:
			break;
		}
	}

	return (0);
}


/*
 * int 
 * ws_speckey(ushort)
 *
 * Calling/Exit State:
 *	None.
 */
int
ws_speckey(ushort ch)
{
	if (ch >= K_VTF && ch <= K_VTL)
		return (HOTKEY);

	switch (ch) {
	case K_NEXT:
	case K_PREV:
	case K_FRCNEXT:
	case K_FRCPREV:
		return (HOTKEY);
	default:
		return (NO_CHAR);
	}
}


/*
 * unchar
 * ws_ext(unchar *, ushort, ushort)
 *
 * Calling/Exit State:
 *	None.
 *
 * Note: 
 *	An equivalent WS_EXT macro also exist in ws/ws.h. This function
 *	is here for compatibility reasons.
 */
unchar
ws_ext(unchar *x, ushort y, ushort z)
{
	return *(x + y * NUMEXTSTATES + z);
}


/*
 * ushort
 * ws_extkey(unchar, charmap_t *, kbstate_t *, unchar)
 *
 * Calling/Exit State:
 *	- charmap basic lock is held across the function.
 */
ushort
ws_extkey(unchar ch, charmap_t *cmp, kbstate_t *kbp, unchar scan)
{
	unsigned	state = 0;
	unsigned	estate = 0;
	extkeys_t	*ext_keys;
	strmap_t	*strbufp;
	stridx_t	*strmap_p;
	keymap_t	*kmp;
	ushort		idx;


	ext_keys = cmp->cr_extkeyp;
	strbufp = cmp->cr_strbufp;
	strmap_p = cmp->cr_strmap_p;
	kmp = cmp->cr_keymap_p;

	state = ws_getstate(kmp, kbp, scan);

	/* Get index */
	estate = (state & ALTED) ? 3 : ((state & CTRLED) ? 2 : state);

	/*
	 * If the entry in ext_keys[][] is K_NOP and we don't have a
	 * ctl-ScrollLock, continue processing.	 The special test for
	 * ctl-ScrollLock is necessary because its value in
	 * ext_keys[][] is 0.  0 also happens to be what K_NOP is
	 * defined to be.  Unfortunately, ctl-ScrollLock in extended
	 * mode must return 0 as the extra byte.
	 */

	if ((WS_EXT((unchar *)ext_keys, scan, estate) == K_NOP) &&
	    !((state & CTRLED) && (scan == SCROLLLOCK))) {
		/* Not extended? */
		return (ch);	
	}

	/*
	 * If this key is a string key, but not a number pad key, and
	 * the string key is not null, let kdspecialkey() output the
	 * string.
	 */

	if (IS_FUNKEY(ch) && WS_SPECIALKEY(kmp, kbp, scan) && !IS_NPKEY(scan)) {
		idx = (*strmap_p)[ch - K_FUNF];
		if ( (*strbufp)[idx] != '\0') {
			return (ch);	/* Not extended character */
		}
	}

	/*
	 * If this is a number pad key and the alt key is depressed,
	 * then the user is building a code (modulo 256).
	 */
	if (IS_NPKEY(scan) && (state & ALTED))	{	/* Alt number pad? */
		if (kbp->kb_altseq == -1)
			/* No partial character yet? */
			kbp->kb_altseq = WS_EXT((unchar *)ext_keys, 
				scan, estate) & 0xf; /* Start */
		else	
			/* Partial character present */
			kbp->kb_altseq = ((kbp->kb_altseq * 10) +
			       (WS_EXT((unchar *) ext_keys, scan, estate) & 0xf)) & 0xff;	
		/* Return no character yet */
		return (NO_CHAR);
	}

	if (state & ALTED)		/* Alt key present without number? */
		kbp->kb_altseq = -1;	/* Make sure no partial characters */

	return (GEN_ZERO | WS_EXT((unchar *)ext_keys, scan, estate));	/* Add zero */
}


/*
 * ushort
 * ws_esckey(ushort, unchar, charmap_t *, kbstate_t *, unchar)
 *
 * Calling/Exit State:
 *	- charmap basic lock is held across the function.
 */
ushort
ws_esckey(ushort ch, unchar scan, charmap_t *cmp, kbstate_t *kbp, unchar kbrk)
{
	ushort		newch = ch;
	keymap_t	*kmp = cmp->cr_keymap_p;
	unsigned	state;


	if (IS_FUNKEY(ch))
		return (!kbrk ? (GEN_FUNC | ch) : NO_CHAR);

	if (!kbrk) {
		switch (ch) {
		case K_BTAB:
			newch = GEN_ESCLSB | 'Z';
			break;
	
		case K_ESN:
			state = ws_getstate(kmp, kbp, scan);
			newch = GEN_ESCN | kmp->key[scan].map[state ^ ALTED];
			break;

		case K_ESO:
			state = ws_getstate(kmp, kbp, scan);
			newch = GEN_ESCO | kmp->key[scan].map[state ^ ALTED];
			break;

		case K_ESL:
			state = ws_getstate(kmp, kbp, scan);
			newch = GEN_ESCLSB | kmp->key[scan].map[state ^ ALTED];
			break;

		default:
			break;
		}
	} else
		switch (ch) {
		case K_LAL:
		case K_RAL:
		case K_ALT:
			if (kbp->kb_altseq != -1) {
				newch = kbp->kb_altseq;
				kbp->kb_altseq = -1;
			}
			break;

		default:
			break;
		}

	return (newch);
}


/*
 * ushort
 * ws_scanchar(charmap_t *, kbstate_t *, unchar, uint)
 *
 * Calling/Exit State:
 *	- charmap basic lock is held across the function.
 */
ushort
ws_scanchar(charmap_t *cmp, kbstate_t *kbp, unchar rawscan, uint israw)
{
	unsigned char	scan;	/* "cooked" scan code */
	keymap_t	*kmp;
	unchar		kbrk;
	ushort		ch;


	kbrk = rawscan & KBD_BREAK;	/* extract make/break from scan */
	kmp = cmp->cr_keymap_p;
#ifdef AT_KEYBOARD
	if (cmp->cr_kbmode == KBM_AT)
		scan = char_xlate_at2xt(rawscan);
	else
#endif
		scan = rawscan;

	scan = (unsigned char) ws_procscan(cmp, kbp, rawscan);

	if (!scan || (int) scan > kmp->n_keys) {
		if (!israw)
			return (NO_CHAR);
		else
			return (rawscan);
	}

	ch = WS_TRANSCHAR(kmp, kbp, scan);

	if (!israw && kbp->kb_extkey && kbrk == 0) { /* Possible ext key?*/
		char nch;

		nch = (ushort) ws_extkey(ch, cmp, kbp, scan);
		if (nch != ch) 
			return (nch);		/* kbrk is 0 and ! raw */
	} else
		kbp->kb_altseq = -1;

	if (WS_SPECIALKEY(kmp, kbp, scan)) {
		ch = (ushort) ws_esckey(ch, scan, cmp, kbp, kbrk);
		if (ws_statekey(ch, cmp, kbp, kbrk) && !israw)
			return (NO_CHAR);
	}
	
	if (israw)
		return (ushort) (rawscan);

	return (kbrk ? NO_CHAR : ch);
}


/*
 * unchar
 * ws_getled(kbstate_t *kbp)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- channel basic lock is also held 
 *	  to protect the keyboard state.
 */
unchar
ws_getled(kbstate_t *kbp)
{
	unchar	tmp_led = 0;

	if (kbp->kb_state & CAPS_LOCK)
		tmp_led |= LED_CAP;
	if (kbp->kb_state & NUM_LOCK)
		tmp_led |= LED_NUM;
	if (kbp->kb_state & SCROLL_LOCK)
		tmp_led |= LED_SCR;
        if (kbp->kb_state & KANA_LOCK)
                tmp_led |= LED_KANA;
	return (tmp_led);
}
