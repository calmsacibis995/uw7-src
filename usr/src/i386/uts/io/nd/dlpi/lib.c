#ident "@(#)lib.c	28.1"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#ifdef  _KERNEL_HEADERS

#include <io/nd/dlpi/include.h>

#else

#include "include.h"

#endif

/*
 * mdi_htons(s), mdi_ntohs(s)
 *	Host to Network byte ordering, Network to host byte ordering
 *	swapping functions.
 *  returns s, with the byte order swapped.
 */
ushort
mdi_ntohs(ushort s)
{
	return ( (s>>8) | (s<<8) );
}

ushort
mdi_htons(ushort s)
{
	return ( (s>>8) | (s<<8) );
}

/*
 * Send a MAC_OK_ACK to the specified queue
 */
void
mdi_macokack(queue_t *q, long prim)
{
	mblk_t *okmp;
	mac_ok_ack_t *ok_ack;

	if (okmp = allocb(sizeof(mac_ok_ack_t), BPRI_HI)) {
		ok_ack = (mac_ok_ack_t *) okmp->b_rptr;
		okmp->b_wptr += sizeof(mac_ok_ack_t);
		okmp->b_datap->db_type = M_PCPROTO;
		ok_ack->mac_primitive = MAC_OK_ACK;
		ok_ack->mac_correct_primitive = prim;
		putnext(q, okmp);
	}
}

/*
 * Send a MAC_ERROR_ACK to the specified queue
 */
void
mdi_macerrorack(queue_t *q,long prim,int tl_err)
{
	mblk_t *errmp;
	mac_error_ack_t *error_ack;

	if (errmp = allocb(sizeof(mac_error_ack_t), BPRI_HI)) {
		error_ack = (mac_error_ack_t *) errmp->b_rptr;
		errmp->b_wptr += sizeof(mac_error_ack_t);
		errmp->b_datap->db_type = M_PCPROTO;
		error_ack->mac_primitive = MAC_ERROR_ACK;
		error_ack->mac_error_primitive = prim;
		error_ack->mac_errno = tl_err;
		putnext(q, errmp);
	}
}

#define CM_ISAVERIFYMODE "ISAVERIFYMODE"

/* support for ISA autodetect.  
 * we don't need to call cm_begin_trans/cm_end_trans until dlpi is made
 * DDI8.  calls to cm_addval will go to _Compat_cm_addval.
 * returns 0 if success else error number
 */
int
mdi_AT_verify(rm_key_t rm_key, int *getset, 
		ulong_t *sioa, ulong_t *eioa, int *vector,
		ulong_t *scma, ulong_t *ecma, int *dmac)
{
	cm_args_t cma;
	cm_num_t mode;
	cm_num_t irq, dma;
	cm_range_t mem, io;

	if (getset == NULL) {
		cmn_err(CE_CONT,"mdi_AT_verify: getset is null!\n");
		return(EINVAL);
	}

	cma.cm_key = rm_key;
	cma.cm_n = 0;
	cma.cm_param = CM_ISAVERIFYMODE;
	cma.cm_val = &mode;
	cma.cm_vallen = sizeof(cm_num_t);

	if (cm_getval(&cma)) {

		/* ensure we're in starting state */
		if (*getset != MDI_ISAVERIFY_UNKNOWN) {
			cmn_err(CE_NOTE,"!bad initial mode for cm_AT_verify");
			return(EINVAL);
		}

		/* probably the dcu calling the verify routine */
		*getset = MDI_ISAVERIFY_TRADITIONAL;
		return(0);
	}
	if (mode == MDI_ISAVERIFY_GET) {
		if (*getset == MDI_ISAVERIFY_UNKNOWN) {
			*getset = MDI_ISAVERIFY_GET;
			return(0);
		} else 
		if (*getset == MDI_ISAVERIFY_GET_REPLY) {

			/* we use "_IRQ", "_DMA", and "_MEMADDR" instead of 
			 * CM_IRQ, CM_DMA, CM_MEMADDR because we must be in 
			 * sync with ndcfg
			 * and if we use CM_IRQ then that value would always 
			 * appear to
			 * be "claimed" by a board in the resmgr.
			 */

			if (vector != NULL) {
				irq=*vector;
				cma.cm_param = "_IRQ";
				cma.cm_val = (void *)&irq;
				cma.cm_vallen = sizeof(cm_num_t);
				if (cm_addval(&cma)) {
					cmn_err(CE_CONT,"mdi_AT_verify: could "
						"not add IRQ to system "
						"database.\n");
					return(EINVAL);
				}
			}

			if (dmac != NULL) {
				dma=*dmac;
				cma.cm_param = "_DMA";
				cma.cm_val = (void *)&dma;
				cma.cm_vallen = sizeof(cm_num_t);
				if (cm_addval(&cma)) {
					cmn_err(CE_CONT,"mdi_AT_verify: could "
						"not add DMA to system "
						"database.\n");
					return(EINVAL);
				}
			}

			if ((scma != NULL) && (ecma != NULL)) {
				mem.startaddr = (ulong_t) *scma;
				mem.endaddr = (ulong_t) *ecma;
				cma.cm_param = "_MEMADDR";
				cma.cm_val = (void *)&mem;
				cma.cm_vallen = sizeof(cm_range_t);
				if (cm_addval(&cma)) {
					cmn_err(CE_CONT,"mdi_AT_verify: could "
						"not add MEMADDR to system "
						"database.\n");
					return(EINVAL);
				}
			}

			if ((sioa != NULL) && (eioa != NULL)) {
				io.startaddr = (ulong_t) *sioa;
				io.endaddr = (ulong_t) *eioa;
				cma.cm_param = "_IOADDR";
				cma.cm_val = (void *)&io;
				cma.cm_vallen = sizeof(cm_range_t);
				if (cm_addval(&cma)) {
					cmn_err(CE_CONT,"mdi_AT_verify: could "
						"not add IOADDR to system "
						"database.\n");
					return(EINVAL);
				}
			}

			return(0);

		} else {
			cmn_err(CE_NOTE,"mdi_AT_verify: bad getset %d "
				"for MDI_ISAVERIFY_GET", *getset);
			return(EINVAL);
		}
	} else if (mode == MDI_ISAVERIFY_SET) {

		/* ensure we're in starting state */
		if (*getset != MDI_ISAVERIFY_UNKNOWN) {
			cmn_err(CE_NOTE,"!bad initial mode for cm_AT_verify");
			return(EINVAL);
		}

		/* program the firmware. fill in the supplied arguments so 
		 * that the
		 * caller can program the firmware.  If a given parameter was 
		 * not set in the resmgr by ndcfg then set the value to -1.
		 */

		/* we use "_IRQ", "_DMA", "_IOADDR", and "_MEMADDR" instead of 
		 * CM_IRQ, CM_DMA, CM_MEMADDR because we must be in sync with 
		 * ndcfg and if we use CM_IRQ then that value would always 
		 * appear to be "claimed" by a board in the resmgr.
		 */

		*getset = MDI_ISAVERIFY_SET;

		if (vector != NULL) {
			cma.cm_param = "_IRQ";
			cma.cm_val = (void *)&irq;
			cma.cm_vallen = sizeof(cm_num_t);
			if (cm_getval(&cma)) {
				/* never added to resmgr by ndcfg.  */
				*vector=-1;	/* we don't have a vector */
			} else {
				*vector=irq;
			}
		}

		if (dmac != NULL) {
			cma.cm_param = "_DMA";
			cma.cm_val = (void *)&dma;
			cma.cm_vallen = sizeof(cm_num_t);
			if (cm_getval(&cma)) {
				/* never added to resmgr by ndcfg. */
				*dmac=-1;
			} else {
				*dmac=dma;
			}
		}

		if (scma != NULL && ecma != NULL) {
			cma.cm_param = "_MEMADDR";
			cma.cm_val = (void *)&mem;
			cma.cm_vallen = sizeof(cm_range_t);
			if (cm_getval(&cma)) {
				/* never added to resmgr by ndcfg */
				*scma=(ulong_t) -1;
				*ecma=(ulong_t) -1;
			} else {
				*scma=(ulong_t) mem.startaddr;
				*ecma=(ulong_t) mem.endaddr;
			}
		}

		if (sioa != NULL && eioa != NULL) {
			cma.cm_param = "_IOADDR";
			cma.cm_val = (void *)&io;
			cma.cm_vallen = sizeof(cm_range_t);
			if (cm_getval(&cma)) {
				/* never added to resmgr by ndcfg */
				*sioa=(ulong_t) -1;
				*eioa=(ulong_t) -1;
			} else {
				*sioa=(ulong_t) io.startaddr;
				*eioa=(ulong_t) io.endaddr;
			}
		}

		return(0);

	} else {
		cmn_err(CE_NOTE,"mdi_AT_verify: unknown mode %d",mode);
		return(EINVAL);
	}
}

/*
 * mdi_printcfg()
 *
 * configuration information printf for MDI drivers
 *
 * NOTE:  changed output slightly to emulate OpenServer 5.0.4c 
 *        (prf.c SID 65.1 97/06/02) which gives more space for comment field.
 */

/* PRINTFLIKE6 */
void
mdi_printcfg(const char *name, unsigned base, unsigned offset, 
		int vec, int dma, const char *fmt, ...)
{
	static int firsttime = 1;
	static char xdigit[16] = "0123456789ABCDEF";
	unsigned end = base + offset;
	int i, *ip;
	int argument[30];
	VA_LIST argp;

	VA_START(argp, fmt);

	if (firsttime) {
		cmn_err(CE_CONT,"!device    address\tvec dma  comment\n");
		cmn_err(CE_CONT,"!-------------------------------------------------------------------------------\n");   /* 79 -'s */
		firsttime = 0;
	}
	cmn_err(CE_CONT,"!%c", '%');
	/* driver name was 8 in OpenServer mdevice(F), now 14 in Master(4), but
	 * we need the screen real estate, so keep at 8 
	 */
	for (i=8;i && *name;i--) cmn_err(CE_CONT,"!%c", *name++);
	while (i-- > -1) cmn_err(CE_CONT,"! ");
	if (base)
		cmn_err(CE_CONT,"!0x%c%c%c%c-0x%c%c%c%c\t",
			xdigit[(base>>12) & 0xf],
			xdigit[(base>>8) & 0xf],
			xdigit[(base>>4) & 0xf],
			xdigit[base & 0xf],
			xdigit[(end>>12) & 0xf],
			xdigit[(end>>8) & 0xf],
			xdigit[(end>>4) & 0xf],
			xdigit[end & 0xf]);
	else
		cmn_err(CE_CONT,"!-\t\t");
	if (vec >= 0)
		cmn_err(CE_CONT,"!%3d ", vec);
	else
		cmn_err(CE_CONT,"!  - ");
	if (dma >= 0)
		cmn_err(CE_CONT,"!%3d  ", dma);
	else
		cmn_err(CE_CONT,"!  -  ");
	if (fmt) {
		char *qfmt;

		if ((qfmt = (char *)kmem_zalloc(strlen(fmt)+2, KM_NOSLEEP)) == NULL) {
			qfmt = (char *)fmt;
		}
	qfmt[0] = '!';
	(void)strcpy(&qfmt[1], fmt);
		/* this obtuse method of getting the varargs arguments is to 
		 * a) keep this function in lib.c instead of putting it
		 *    in Nonconform.c and calling xcmn_err() in cmn_err.c 
		 *    directly (passing in the VA_LIST from above)
		 * b) fix compiler optimization (volatile didn't help)
		 *    problems with VA_ARG macro.  putting multiple VA_ARGS
		 *    in call to cmn_err below doesn't work, as compiler
		 *    doesn't understand the side effects properly.
		 * The bad part of this scheme is that we shove additional 
		 * arguments on kernel stack _and_ also require another
		 * local array ("argument") on stack too.
		 */
		for (i=0; i<30; i++) {
			argument[i]=VA_ARG(argp, int);
		}

		ip=&argument[0];
		cmn_err(CE_CONT,qfmt,
			*(ip+ 0),
			*(ip+ 1),
			*(ip+ 2),
			*(ip+ 3),
			*(ip+ 4),
			*(ip+ 5),
			*(ip+ 6),
			*(ip+ 7),
			*(ip+ 8),
			*(ip+ 9),
			*(ip+10),
			*(ip+11),
			*(ip+12),
			*(ip+13),
			*(ip+14),
			*(ip+15),
			*(ip+16),
			*(ip+17),
			*(ip+18),
			*(ip+19),
			*(ip+20),
			*(ip+21),
			*(ip+22),
			*(ip+23),
			*(ip+24),
			*(ip+25),
			*(ip+26),
			*(ip+27),
			*(ip+28),
			*(ip+29));

		if (qfmt != fmt) {
			kmem_free(qfmt, strlen(fmt)+2);
		}
	}
	cmn_err(CE_CONT,"!\n");
}

/*
 * mdi_hw_suspended:
 * This function is only used for DDI8 drivers.  It tells the higher level 
 * stacks that we have been suspended and that they shouldn't bother sending
 * further frames as they will be silently dropped.
 * Sends a MAC_HWSUSPEND_IND message upstream to either
 * - the dlpi module
 * - the stream head (raw MDI consumers like test suites or tcpdump)
 * - another module (like tap)
 *
 * While not used today, the rmkey argument may be useful in the future for
 * telling programs like ndcfg that the driver called this routine.
 * 
 * Arguments:  read queue from mdi driver to dlpi and resmgr key.
 *
 * Return value:
 * 0 for success else error number
 */
int
mdi_hw_suspended(queue_t *q, rm_key_t rmkey)
{
	mblk_t *mp;
	struct mac_hwsuspend_ind *si;

	DLPI_PRINTF1000(("MDI: sending MAC_HWSUSPEND_IND upstream\n"));
	if ((mp = allocb(sizeof(struct mac_hwsuspend_ind), BPRI_HI)) == NULL) {
		return(ENOSR);
	} 
	si=(struct mac_hwsuspend_ind *)(mp->b_rptr);
	mp->b_datap->db_type = M_PCPROTO;
	mp->b_wptr = mp->b_rptr + sizeof(struct mac_hwsuspend_ind);

	si->mac_primitive = MAC_HWSUSPEND_IND;
	putnext(q, mp);   /* ignore flow control */
	return(0);
}


/*
 * mdi_hw_resumed:
 * This function is only used for DDI8 drivers.  It tells the higher level 
 * stacks that we have been resumed and that it is now ok to send frames
 * again down to the MDI driver.
 * Sends a MAC_HWRESUME_IND message upstream to either
 * - the dlpi module
 * - the stream head (raw MDI consumers like test suites or tcpdump)
 * - another module (like tap)
 *
 * While not used today, the rmkey argument may be useful in the future for
 * telling programs like ndcfg that the driver called this routine.
 * 
 * Arguments:  read queue from mdi driver to dlpi and resmgr key.
 *
 * Return value:
 * 0 for success else error number
 * 
 * NOTE:  will typically be called whether or not a consumer is actually 
 *        using the MDI device at this time.
 */
int
mdi_hw_resumed(queue_t *q, rm_key_t rmkey)
{
	mblk_t *mp;
	struct mac_hwresume_ind *ri;

	DLPI_PRINTF1000(("MDI: sending MAC_HWRESUME_IND upstream\n"));
	if ((mp = allocb(sizeof(struct mac_hwresume_ind), BPRI_HI)) == NULL) {
		return(ENOSR);
	} 
	ri=(struct mac_hwresume_ind *)(mp->b_rptr);
	mp->b_datap->db_type = M_PCPROTO;
	mp->b_wptr = mp->b_rptr + sizeof(struct mac_hwresume_ind);

	ri->mac_primitive = MAC_HWRESUME_IND;
	putnext(q, mp);   /* ignore flow control */
	return(0);
}


#ifdef ODT_TBIRD
/*
 * put a message of the next queue of the given queue
 */
int
putnext(queue_t *q, mblk_t *mp)
{
	(*q->q_next->q_qinfo->qi_putp)(q->q_next, mp);
}

/*
 * Definition of Rbsize (which was added to Everest kernel)
 */
uint	Rbsize[] = { 4, 16, 64, 128, 256, 512, 1024, 2048, 4096 };
#endif
