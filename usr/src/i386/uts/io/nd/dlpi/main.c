#ident "@(#)main.c	29.4"
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

STATIC int	register_media(int type, const per_media_info_t *mi);
STATIC void	dlpi_media_table_init(void);

/* ndcfg/netcfg version information. */
static char _ndversion[]="29.4";

LKINFO_DECL(dlpi_lkinfo, "DLPI::mdi mutex txmon lock", 0);
LKINFO_DECL(dlpi_mca_lkinfo, "DLPI::mdi multicasting r/w lock", 0);
LKINFO_DECL(mdicard_lkinfo, "DLPI::mdi mutex card lock", 0);
LKINFO_DECL(mdicard_sap_id_lkinfo, "DLPI::mdi mutex SAP id lock", 0);

/* global lock for serializing consumers through open routine.  */
lock_t *dlpi_open_lock;
LKINFO_DECL(dlpi_open_info, "DLPI::dlpi open lock", 0);

#ifdef DLPI_DEBUG
void
dlpi_card(per_card_info_t *cp)
{
	register int i;

	cmn_err(CE_CONT, "\nDLPI per_card_info (0x%x)\tlock_sap_id 0x%x\n",
		cp, cp->lock_sap_id);
	cmn_err(CE_CONT, "lock_card 0x%x\n", cp->lock_card);
	cmn_err(CE_CONT, "nopens %d\tmaxsaps %d\tmaxroutes %d\n",
		cp->nopens, cp->maxsaps, cp->maxroutes);
 	cmn_err(CE_CONT, "rxunbound 0x%x\tsap_table 0x%x\troute_table 0x%x\n",
		cp->rxunbound, cp->sap_table, cp->route_table);

	cmn_err(CE_CONT, "hw_mac_addr: ");
	for (i = 0; i < 6; i++)
		cmn_err(CE_CONT, "%b ", cp->hw_mac_addr[i]);
	cmn_err(CE_CONT, "local_mac_addr: ");
	for (i = 0; i < 6; i++)
		cmn_err(CE_CONT, "%b ", cp->local_mac_addr[i]);
	cmn_err(CE_CONT, "\tdown_queue 0x%x\n", cp->down_queue);

	cmn_err(CE_CONT, "mdi_driver_ready %b dlpidRegistered %b all_mca %b disab_ether %b\n",
		cp->mdi_driver_ready, cp->dlpidRegistered, cp->all_mca, cp->disab_ether);
	cmn_err(CE_CONT, "mac_driver_version 0x%x\tmac_media_type 0x%x\n",
		cp->mac_driver_version, cp->mac_media_type);
	cmn_err(CE_CONT, "mac_max_sdu 0x%x\tmac_ifspeed 0x%x\tmac_min_sdu 0x%x\n",
		cp->mac_max_sdu, cp->mac_min_sdu, cp->mac_ifspeed);

	cmn_err(CE_CONT, "media_specific 0x%x media_svcs_queue 0x%x media_private 0x%x\n",
		cp->media_specific, cp->media_svcs_queue, cp->media_private);

	cmn_err(CE_CONT, "control_sap 0x%x\thwfail_waiting 0x%x\n",
		cp->control_sap, cp->hwfail_waiting);
	cmn_err(CE_CONT, "mdi_primitive_handler() 0x%x\n", cp->mdi_primitive_handler);

	cmn_err(CE_CONT, "\tioctl_sap 0x%x\tdlpi_iocblk 0x%x\n", 
		cp->ioctl_sap, cp->dlpi_iocblk);

	cmn_err(CE_CONT, "\tioctl_handler() 0x%x\n", cp->ioctl_handler);
	cmn_err(CE_CONT, "\tmcahashtbl 0x%x\n", cp->mcahashtbl);

	cmn_err(CE_CONT, "\tsuspended=0x%x\t\tsuspenddrops=0x%x\n",
		cp->issuspended, cp->suspenddrops);

#if 0
	cmn_err(CE_CONT, "\told_tx.bcast %d\n", cp->old_tx.bcast);
	cmn_err(CE_CONT, "\told_tx.mcast %d\n", cp->old_tx.mcast);
	cmn_err(CE_CONT, "\told_tx.ucast_xid %d\n", cp->old_tx.ucast_xid);
	cmn_err(CE_CONT, "\told_tx.ucast_test %d\n", cp->old_tx.ucast_test );
	cmn_err(CE_CONT, "\told_tx.ucast %d\n", cp->old_tx.ucast);
	cmn_err(CE_CONT, "\told_tx.error %d\n", cp->old_tx.error);
	cmn_err(CE_CONT, "\told_tx.octets %d\n", cp->old_tx.octets);
	cmn_err(CE_CONT, "\told_rx.bcast %d\n", cp->old_rx.bcast);
	cmn_err(CE_CONT, "\told_rx.mcast %d\n", cp->old_rx.mcast);
	cmn_err(CE_CONT, "\told_rx.ucast_xid %d\n", cp->old_rx.ucast_xid);
	cmn_err(CE_CONT, "\told_rx.ucast_test %d\n", cp->old_rx.ucast_test);
	cmn_err(CE_CONT, "\told_rx.ucast %d\n", cp->old_rx.ucast);
	cmn_err(CE_CONT, "\told_rx.error %d\n", cp->old_rx.error);
	cmn_err(CE_CONT, "\told_rx.octets %d\n", cp->old_rx.octets);
	cmn_err(CE_CONT, "txmon 0x%x\tinit->is_mp %d\n",
		cp->txmon, cp->init->is_mp);
	cmn_err(CE_CONT, "init->txmon_consume_time %d\tinit->txmon_enabled %d\n",
		cp->init->txmon_consume_time, cp->init->txmon_enabled);
#endif
}

void
dlpi_sap(struct per_sap_info *sp)
{
	cmn_err(CE_CONT, "\nDLPI per_sap_info struct (0x%x)\n", sp);

	cmn_err(CE_CONT, "\tsap_type 0x%x sap 0x%x\n", sp->sap_type, sp->sap);
	cmn_err(CE_CONT, "\tsap_state 0x%x sap_protid 0x%x\n", sp->sap_state, sp->sap_protid);

	cmn_err(CE_CONT, "\tup_queue 0x%x\n", sp->up_queue);
	cmn_err(CE_CONT, "\tcard_info 0x%x\n", sp->card_info);

	cmn_err(CE_CONT, "\tsrmode 0x%x llcmode 0x%x dlpimode 0x%x\n", sp->srmode, sp->llcmode, sp->dlpimode);
	
	cmn_err(CE_CONT, "\tllcxidtest_flg 0x%x\n", sp->llcxidtest_flg);

	cmn_err(CE_CONT, "\tsap_tx.bcast %d\n", sp->sap_tx.bcast);
	cmn_err(CE_CONT, "\tsap_tx.mcast %d\n", sp->sap_tx.mcast);
	cmn_err(CE_CONT, "\tsap_tx.ucast_xid %d\n", sp->sap_tx.ucast_xid);
	cmn_err(CE_CONT, "\tsap_tx.ucast_test %d\n", sp->sap_tx.ucast_test );
	cmn_err(CE_CONT, "\tsap_tx.ucast %d\n", sp->sap_tx.ucast);
	cmn_err(CE_CONT, "\tsap_tx.error %d\n", sp->sap_tx.error);
	cmn_err(CE_CONT, "\tsap_tx.octets %d\n", sp->sap_tx.octets);
	cmn_err(CE_CONT, "\tsap_rx.bcast %d\n", sp->sap_rx.bcast);
	cmn_err(CE_CONT, "\tsap_rx.mcast %d\n", sp->sap_rx.mcast);
	cmn_err(CE_CONT, "\tsap_rx.ucast_xid %d\n", sp->sap_rx.ucast_xid);
	cmn_err(CE_CONT, "\tsap_rx.ucast_test %d\n", sp->sap_rx.ucast_test);
	cmn_err(CE_CONT, "\tsap_rx.ucast %d\n", sp->sap_rx.ucast);
	cmn_err(CE_CONT, "\tsap_rx.error %d\n", sp->sap_rx.error);
	cmn_err(CE_CONT, "\tsap_rx.octets %d\n", sp->sap_rx.octets);

	cmn_err(CE_CONT, "\tdlpi_prim 0x%x all_mca %b\n", sp->dlpi_prim, sp->all_mca);
}

void
dlpi_queue(const queue_t *qp)
{
	cmn_err(CE_CONT, "\nSTREAM queue struct (queue_t): size=0x%x(%d)\n",
		sizeof(queue_t), sizeof(queue_t));
	cmn_err(CE_CONT, "\tq_qinfo=0x%x, \tq_first=0x%x\n",
		qp->q_qinfo, qp->q_first);
	cmn_err(CE_CONT, "\tq_last=0x%x, \tq_next=0x%x\n",
		qp->q_last, qp->q_next);
	cmn_err(CE_CONT, "\tq_link=0x%x, \tq_ptr=0x%x\n",
		qp->q_link, qp->q_ptr);
	cmn_err(CE_CONT, "\tq_count=0x%x, \tq_flag=0x%x\n",
		qp->q_count, qp->q_flag);
	cmn_err(CE_CONT, "\tq_minpsz=0x%x, \tq_maxpsz=0x%x\n",
		qp->q_minpsz, qp->q_maxpsz);
	cmn_err(CE_CONT, "\tq_hiwat=0x%x, \tq_lowat=0x%x\n",
		qp->q_hiwat, qp->q_lowat);
	cmn_err(CE_CONT, "\tq_bandp=0x%x, \tq_nband=0x%x\n",
		qp->q_bandp, qp->q_nband);
	cmn_err(CE_CONT, "\tq_blocked=0x%x, \tq_svcflag=0x%x\n",
		qp->q_blocked, qp->q_svcflag);
	cmn_err(CE_CONT, "\tq_str=0x%x, \tq_blink=0x%x\n",
		qp->q_str, qp->q_blink);
	cmn_err(CE_CONT, "\tq_putp=0x%x, \tq_putcnt=0x%x\n",
		qp->q_putp, qp->q_putcnt);
	cmn_err(CE_CONT, "\tq_defcnt=0x%x\n",
		qp->q_defcnt);
}

/*
 * void
 * dlpi_dblk(const dblk_t *)
 *	Formatted print of datab (dblk_t) structure. Can be invoked from
 *	kernel debugger.
 *
 * Calling/Exit State:
 *	None.
 */
void
dlpi_dblk(const dblk_t *dp)
{
	cmn_err(CE_CONT, "\n STREAM datab struct (dblk_t): size=0x%x(%d)\n",
		sizeof(dblk_t), sizeof(dblk_t));
	cmn_err(CE_CONT, "\tdb_frtnp=0x%x, \tdb_base=0x%x\n",
		dp->db_frtnp, dp->db_base);
	cmn_err(CE_CONT, "\tdb_lim=0x%x, \tdb_ref=0x%x\n",
		dp->db_lim, dp->db_ref);
	cmn_err(CE_CONT, "\tdb_type=0x%x, \tdb_muse=0x%x\n",
		dp->db_type, dp->db_muse);
	cmn_err(CE_CONT, "\tdb_size=0x%x, \tdb_addr=0x%x\n",
		dp->db_size, dp->db_addr);
	cmn_err(CE_CONT, "\tdb_odp=0x%x, \tdb_cpu=0x%x\n",
		dp->db_odp, dp->db_cpu);
}

/*
 * void
 * dlpi_mblk(const mblk_t *)
 *	Formatted print of msgb (mblk_t) structure. Can be invoked from
 *	kernel debugger.
 *
 * Calling/Exit State:
 *	None.
 */
void
dlpi_mblk(const mblk_t *mp)
{
	cmn_err(CE_CONT, "\n STREAM msgb struct (mblk_t): size=0x%x(%d)\n",
		sizeof(mblk_t), sizeof(mblk_t));
	cmn_err(CE_CONT, "\tb_next=0x%x, \tb_prev=0x%x\n",
		mp->b_next, mp->b_prev);
	cmn_err(CE_CONT, "\tb_cont=0x%x, \tb_rptr=0x%x\n",
		mp->b_cont, mp->b_rptr);
	cmn_err(CE_CONT, "\tb_wptr=0x%x, \tb_datap=0x%x\n",
		mp->b_wptr, mp->b_datap);
	cmn_err(CE_CONT, "\tb_band=0x%x, \tb_flag=0x%x\n",
		mp->b_band, mp->b_flag);
	dlpi_dblk(mp->b_datap);
}

/* 
 * void
 * dlpi_locks(void)
 *
 * print locks and status.  Can be invoked from kernel debugger
 *
 * Calling/Exit State:
 * None.
 */
void
dlpi_locks(per_card_info_t *cp)
{
	int loop;

	cmn_err(CE_CONT, "lock_t:   lock_sap_id=0x%x, \tlock_card=0x%x\n",
		cp->lock_sap_id, cp->lock_card);
	cmn_err(CE_CONT, "rwlock_t: lock_mca=0x%x\n",
		cp->lock_mca);
	cmn_err(CE_CONT, "lock_t:   txmon.trylock=0x%x\n",
		cp->txmon.trylock);
	if (cp->route_table) {
			cmn_err(CE_CONT, "lock_t:   route_table.lock_rt=0x%x\n",
				cp->route_table->lock_rt);
	}
}

void
dlpi_lock_t_print(lock_t *lp)
{
	cmn_err(CE_CONT, "sp_lock=0x%x\n",lp->sp_lock);
#if (defined DEBUG || defined SPINDEBUG)  /* what ksynch.h uses */
	cmn_err(CE_CONT, "sp_flags=0x%x\n",lp->sp_flags);
	cmn_err(CE_CONT, "sp_hier=0x%x\n",lp->sp_hier);
	cmn_err(CE_CONT, "sp_minipl=0x%x\n",lp->sp_minipl);
	cmn_err(CE_CONT, "sp_lkstatp=0x%x\n",lp->sp_lkstatp);
	if (lp->sp_lkstatp) {
		cmn_err(CE_CONT, "lks_infop=0x%x\n",lp->sp_lkstatp->lks_infop);
		cmn_err(CE_CONT, "lks_wrcnt=0x%x \t lks_rdcnt=0x%x\n",
			lp->sp_lkstatp->lks_wrcnt, lp->sp_lkstatp->lks_rdcnt);
		cmn_err(CE_CONT, "lks_solordcnt=0x%x \t lks_fail=0x%x\n",
			lp->sp_lkstatp->lks_solordcnt, lp->sp_lkstatp->lks_fail);
		/* I won't print the others */
	}
	cmn_err(CE_CONT, "sp_lkinfop=0x%x\n",lp->sp_lkinfop);
	if (lp->sp_lkinfop) {
		cmn_err(CE_CONT, "lk_name=%s\n",lp->sp_lkinfop->lk_name);
		cmn_err(CE_CONT, "lk_flags=0x%x\n",lp->sp_lkinfop->lk_flags);
	}
#endif /* (DEBUG || SPINDEBUG) */

}

#endif /* DLPI_DEBUG */

/*
 * Let the compiler do some sanity checks
 */

#if (MAC_CSMACD != DL_CSMACD) ||	/* Probably 0x00 */	\
    (MAC_TPR != DL_TPR) ||		/* Probably 0x02 */	\
    (MAC_FDDI != DL_FDDI)		/* Probably 0x08 */
	Oops we have a problem here
#endif

/* int dlpidevflag = D_MP; not needed any more; we're a MISC_WRAPPER */
#define DRVNAME "dlpi - DLPI module"
static int dlpi_load(), dlpi_unload();
MOD_MISC_WRAPPER(dlpi, dlpi_load, dlpi_unload, DRVNAME);

static int
dlpi_load()
{
	dlpi_open_lock=LOCK_ALLOC(DLPIOPENHIER, plstr, &dlpi_open_info, KM_NOSLEEP);
	if (dlpi_open_lock == (lock_t *)NULL) {
		cmn_err(CE_WARN,"dlpi_load: no memory for open lock");
		return(EAGAIN);
	}
	return(0);
}

static int
dlpi_unload()
{
	ASSERT(LOCK_OWNED(dlpi_open_lock) == B_FALSE);
	LOCK_DEALLOC(dlpi_open_lock);
	return(0);
}

/* our budget atoi routine.  note different arguments and different
 * functionality than stdio atoi -- we're stricter, emulating strtoul(ddi8 
 * only).  The intended purpose is to take a custom parameter string in the 
 * resmgr and convert it to an integer.  It must exist as a legal decimal 
 * string.  If the custom parameter has __STRING__ and the user either
 * a) doesn't enter anything (leaving __STRING__ in the resmgr) OR
 * b) enters bogus text ("hello world") instead of a number 
 * we need to return a failure.
 * returns 0 for success, -1 for error.
 * would be nice to use kernel strtoul routine but that's for ddi8 only
 */
STATIC int
dlpi_atoi(const char *p, int *result)
{
	register int n;
	register int f;
	register int goodchars;

	n = 0;
	f = 0;
	goodchars = 0;

	for(;;p++) {
		switch(*p) {
			case ' ':
			case '\t':
				continue;  /* be gracious and allow leading whitespace */
			case '-':
				f++;
			case '+':
				p++;
		}
		break;
	}

	/* no locale isdigit stuff here */
	while (*p >= '0' && *p <= '9') {
		goodchars++;
		n = n*10 - (*p++ - '0');
	}
	if (goodchars) {
		*result = ( f ? n : -n );
		return(0);
	} else {
		return(-1);
	}
	/* NOTREACHED */
}


/* this routine exists for certain bcfg files (Token-Ring, FDDI) that may 
 * specify NETX scope custom parameters to alter certain characteristics
 * of the netX driver.  Since these are normally implemented as idtunes
 * they will not have any effect at ISL.  This routine looks for a magic
 * parameter in the resmgr to denote "ISL-time" functionality which allows
 * 3 main parameters to be changed from their Mtune defaults: 
 * - Source Routing on or off
 * - Use All Route Explorers (ARE)
 * - Maximum number of routes
 * This parameter is added by the ISL code 
 * (usr/src/work/sysinst/desktop/ui_modules/nics_config)
 * and will not exist in the resmgr post-isl once ndcfg installs the board 
 * (ndcfg cmdops.c function AddDLPIToLinkKit calls DelAllKeys which removes 
 * the entire net0 key)
 */
STATIC void
dlpi_isl(char *myname, int *srmodep, struct route_param *rparam, int *nroutesp)
{

	rm_key_t rmkey;
	cm_num_t tmpkey;
	cm_args_t cma;
	char tmpnumstr[11];
	int tmpnum,status;

	/* force it to be null terminated; could be > 10 chars in resmgr */
	tmpnumstr[10]='\0';

	/* we know there is exactly 1 instance of net0 in resmgr to get here */
	cma.cm_key = cm_getbrdkey(myname, 0);
	cma.cm_param = "DLPIDRIVERKEY";
	cma.cm_val = &tmpkey;
	cma.cm_vallen = sizeof(cm_num_t);
	cma.cm_n = 0;

	if (cm_getval(&cma)) {
		/* probably doesn't exist in the resmgr. 
		 * normal for non-isl net0. 
		 */
		return;
	}

	/* value at parameter indicates the numeric key where we can find
	 * parameter we care about.  someday dlpid will tell us this information
	 * Should be stored in resmgr at isl time with command:
	 * resmgr -k -1 -p DLPIDRIVERKEY,n -v 123
	 */
	rmkey=(rm_key_t) tmpkey;

	cma.cm_key = rmkey;
	cma.cm_param = "SRSRMODE";
	cma.cm_val = tmpnumstr;
	cma.cm_vallen = 9;
	cma.cm_n = 0;

	if (cm_getval(&cma) == 0) {
		status=dlpi_atoi(tmpnumstr,&tmpnum);
		/* 0=SR_NON  = none
		 * 1=SR_AUTO = automatically interpret the routing information 
		 *             field and set up routing entries and frame 
		 *             headers as needed
		 * 2=SR_STACK= The DLS user will provide the source 
		 *             routing information
		 */
		if ((status == 0) && (tmpnum >= 0) && (tmpnum <= 2)) {
			*srmodep = tmpnum;
		} else {
			cmn_err(CE_NOTE,"invalid NETxSRSRMODE parameter '%s'", 
				tmpnumstr);
		}
	}

	cma.cm_key = rmkey;
	cma.cm_param = "SRAREDISAB";
	cma.cm_val = tmpnumstr;
	cma.cm_vallen = 9;
	cma.cm_n = 0;

	if (cm_getval(&cma) == 0) {
		status=dlpi_atoi(tmpnumstr,&tmpnum);
		if ((status == 0) && (tmpnum >= 0) && (tmpnum <= 1)) {
			rparam->ARE_disa = status;
		} else {
			cmn_err(CE_NOTE,"invalid NETxSRAREDISAB parameter '%s'",
				tmpnumstr);
		}
	}

	cma.cm_key = rmkey;
	cma.cm_param = "SRMAXROUTE";
	cma.cm_val = tmpnumstr;
	cma.cm_vallen = 9;
	cma.cm_n = 0;

	if (cm_getval(&cma) == 0) {
		status=dlpi_atoi(tmpnumstr,&tmpnum);
		if (status == 0) {
			if (tmpnum < 2) tmpnum = 2;
			*nroutesp = tmpnum;
		} else {
			cmn_err(CE_NOTE,"invalid NETxSRMAXROUTE parameter '%s'",
				tmpnumstr);
		}
	}

	DLPI_PRINTF01(("DLPI dlpi_isl driverkey=%d srmode=%d aredisab=%d maxroute=%d\n", rmkey, *srmodep, rparam->ARE_disa, *nroutesp));

	cmn_err(CE_NOTE,
		"!dlpi_isl: driverkey=%d, srmode=%d, aredisab=%d, maxroute=%d",
		rmkey, *srmodep, rparam->ARE_disa, *nroutesp);

	return;
}

/*
 * DLPI module open routine:
 *	Initialize the Per Card Information Structure on 1st open
 */
int
dlpiopen(queue_t *q, dev_t *dev, int flag, int sflag,
	per_card_info_t *cp,
	int nroutes, int srmode,
	struct route_param *rparam,
	per_sap_info_t *sp, int nsaps,
	int size_cp, int size_sp, dlpi_init_t * init,
	int size_init,
	ulong nmcahash, ulong disab_ether,
	cred_t *crp, char *myname)
{
	struct route_table *rp;
	pl_t s;
	int numboards;

	DLPI_PRINTF01(("DLPI dlpiopen( q=%x dev=%x flag=%x sflag=%x cp=%x nroutes=%x srmode=%x rparam=%x sp=%x nsaps=%x size_cp=%x size_sp=%x init=%x size_init=%x nmcahash=%x disab_ether=%x\n", q, dev, flag, sflag, cp, nroutes, srmode, rparam, sp, nsaps, size_cp, size_sp, init, size_init, nmcahash, disab_ether));

	/* set q_ptr early on to detect streams clone race conditions 
	 * in dlpiclose
	 */
	q->q_ptr = OTHERQ(q)->q_ptr = NULL;

	if ((sizeof(per_card_info_t) != size_cp) ||
		(sizeof(dlpi_init_t) != size_init) ||
		(sizeof(per_sap_info_t) != size_sp)) {
		cmn_err(CE_WARN,
			"dlpiopen - "
			"Structure size mismatch, space.c compiled incorrectly");
		return(ENODEV);
	}

	if (sflag != CLONEOPEN) {
		return(EINVAL);
	}

	/* As of BL15.1 you can't call cm_getnbrd with a spin lock held
	 * (contrary to man page for ddi7 drivers, big bug) as Gemini calls
	 * rm_begin_trans implicitly for you which calls rwsleep_rdlock which
	 * could sleep, fails assertion in rwsleep.c line 265. We call it here 
	 * outside of lock to avoid this bug for now.  We must also move dlpi_isl
	 * code here too
	 * See MR ul97-33053
	 */
	numboards = cm_getnbrd(myname);
	if (numboards != 1) {
		cmn_err(CE_WARN,"dlpiopen: must have exactly 1 '%s' entry in resmgr",
			myname);
		return(ENODEV);
	}

	/* Possibly update SRSRMODE(srmode), 
	 * SRAREDISAB(rparam->ARE_disa), 
	 * and SRMAXROUTE(nroutes) if we're at ISL time and these 
	 * custom parameters exist in resmgr and were modified by 
	 * user from a Token-Ring or FDDI .bcfg file.  Does not 
	 * affect Ethernet/ISDN since they won't have these customs
	 * in their bcfg file.
	 * Again, due to BL15.1 bug you can't call cm_getval with spin lock held
	 * so we move this outside of the LOCK.
	 * see MR ul97-33055
	 */
	if ((strcmp(myname,"net0") == 0) && !cp->nopens) {
		dlpi_isl(myname, &srmode, rparam, &nroutes);
	}

	/* don't use LOCK_CARDINFO as 
	 * - we haven't yet done the LOCK_ALLOC to obtain space for lock
	 * - leads to messy problems with lock_t in per_card_info structure 
	 *   fighting with bzero which will stomp over it.  
	 * - it penalizes existing consumers of /dev/netX which must endure 
	 *   waiting for lock when user app does an open/close repeatedly 
	 *   on /dev/netX
	 * the purpose for this lock is to ensures no races with 
	 * dlpi_allocsap and LOCK_ALLOC's (1st open) if multiple CPUs run 
	 * through this code.
	 */
	s = LOCK(dlpi_open_lock, plstr);

	/* note only 1 cpu will get here at a time, no race to worry about */
	if (!cp->nopens)
	{

		bzero(cp, sizeof (per_card_info_t));
		if ( !(cp->txmon.trylock = LOCK_ALLOC(DLPIHIER, plstr, &dlpi_lkinfo, KM_NOSLEEP)) ) {
			cmn_err(CE_WARN, "dlpiopen: Not enough memory for txmon lock");
			UNLOCK(dlpi_open_lock, s);
			return(EAGAIN);
		}
		if ( !(cp->lock_sap_id = LOCK_ALLOC(DLPIHIER, plstr, &mdicard_sap_id_lkinfo, KM_NOSLEEP)) ) {
			cmn_err(CE_WARN, "dlpiopen: Not enough memory for sap_id lock");
			LOCK_DEALLOC(cp->txmon.trylock);
			UNLOCK(dlpi_open_lock, s);
			return(EAGAIN);
		}
		if ( !(cp->lock_card = LOCK_ALLOC(TXMONHIER, plstr, &mdicard_lkinfo, KM_NOSLEEP)) ) {
			cmn_err(CE_WARN, "dlpiopen: Not enough memory for trylock");
			LOCK_DEALLOC(cp->txmon.trylock);
			LOCK_DEALLOC(cp->lock_sap_id);
			UNLOCK(dlpi_open_lock, s);
			return(EAGAIN);
		}
		if ( !(cp->lock_mca = RW_ALLOC(DLPIHIER, plstr, &dlpi_mca_lkinfo, KM_NOSLEEP)) ) {
			cmn_err(CE_WARN, "dlpiopen: No memory for mca lock");
			LOCK_DEALLOC(cp->txmon.trylock);
			LOCK_DEALLOC(cp->lock_sap_id);
			LOCK_DEALLOC(cp->lock_card);
			UNLOCK(dlpi_open_lock, s);
			return(EAGAIN);
		}
		if ( ! dlpi_alloc_sap_counters(&cp->old_tx)) {
			LOCK_DEALLOC(cp->txmon.trylock);
			LOCK_DEALLOC(cp->lock_sap_id);
			LOCK_DEALLOC(cp->lock_card);
			RW_DEALLOC(cp->lock_mca);
			UNLOCK(dlpi_open_lock, s);
			return(EAGAIN);
		}
		if ( ! dlpi_alloc_sap_counters(&cp->old_rx)) {
			LOCK_DEALLOC(cp->txmon.trylock);
			LOCK_DEALLOC(cp->lock_sap_id);
			LOCK_DEALLOC(cp->lock_card);
			RW_DEALLOC(cp->lock_mca);
			dlpi_dealloc_sap_counters(&cp->old_tx);
			UNLOCK(dlpi_open_lock, s);
			return(EAGAIN);
		}
		DLPI_PRINTF02(("DLPI cp LOCK_ALLOC: cp->txmon.trylock %x cp->lock_sap_id %x cp->lock_card %x\n", cp->txmon.trylock, cp->lock_sap_id, cp->lock_card));

		/* Initialize SAP Table */
		bzero(sp, nsaps * sizeof (per_sap_info_t));

		cp->init = init;
		cp->sap_table = sp;
		cp->maxsaps = nsaps;
		cp->nmcahash = nmcahash;
		cp->disab_ether = (disab_ether ? 1 : 0);
		if (! (cp->mcahashtbl = (card_mca_t **)kmem_zalloc(sizeof(card_mca_t *)*nmcahash, KM_NOSLEEP))) {
			cmn_err(CE_WARN, "dlpiopen: Not enough memory for mca hash table");
			LOCK_DEALLOC(cp->txmon.trylock);
			LOCK_DEALLOC(cp->lock_sap_id);
			LOCK_DEALLOC(cp->lock_card);
			RW_DEALLOC(cp->lock_mca);
			dlpi_dealloc_sap_counters(&cp->old_tx);
			dlpi_dealloc_sap_counters(&cp->old_rx);
			UNLOCK(dlpi_open_lock, s);
			return(EAGAIN);
		}
		if ( !(cp->rxunbound = ATOMIC_INT_ALLOC(KM_NOSLEEP)) ) {
			cmn_err(CE_WARN, "dlpiopen: Not enough memory for atomic rxunbound");
			LOCK_DEALLOC(cp->txmon.trylock);
			LOCK_DEALLOC(cp->lock_sap_id);
			LOCK_DEALLOC(cp->lock_card);
			RW_DEALLOC(cp->lock_mca);
			dlpi_dealloc_sap_counters(&cp->old_tx);
			dlpi_dealloc_sap_counters(&cp->old_rx);
			kmem_free((void *)cp->mcahashtbl, cp->nmcahash*sizeof(card_mca_t *));
			UNLOCK(dlpi_open_lock, s);
			return(EAGAIN);
		}

		if (nroutes < 2) {
			cmn_err(CE_NOTE,"dlpiopen: NETxSRMAXROUTE kernel parameter too low");
			nroutes = 2;
		}
		if ( !(rp = (struct route_table *)kmem_zalloc(sizeof(struct route_table), KM_NOSLEEP))) {
			cmn_err(CE_WARN, "dlpiopen: Not enough memory for route_table");
			LOCK_DEALLOC(cp->txmon.trylock);
			LOCK_DEALLOC(cp->lock_sap_id);
			LOCK_DEALLOC(cp->lock_card);
			RW_DEALLOC(cp->lock_mca);
			dlpi_dealloc_sap_counters(&cp->old_tx);
			dlpi_dealloc_sap_counters(&cp->old_rx);
			kmem_free((void *)cp->mcahashtbl, cp->nmcahash*sizeof(card_mca_t *));
			ATOMIC_INT_DEALLOC(cp->rxunbound);
			UNLOCK(dlpi_open_lock, s);
			return(EAGAIN);
		}
		if ( !(rp->routes = (struct route *)kmem_zalloc(sizeof(struct route) * nroutes, KM_NOSLEEP))) {
			cmn_err(CE_WARN, "dlpiopen: Not enough memory for %d routes",nroutes);
			cmn_err(CE_CONT, "dlpiopen: decrease NETxSRMAXROUTE kernel parameter");
			LOCK_DEALLOC(cp->txmon.trylock);
			LOCK_DEALLOC(cp->lock_sap_id);
			LOCK_DEALLOC(cp->lock_card);
			RW_DEALLOC(cp->lock_mca);
			dlpi_dealloc_sap_counters(&cp->old_tx);
			dlpi_dealloc_sap_counters(&cp->old_rx);
			kmem_free((void *)cp->mcahashtbl, cp->nmcahash*sizeof(card_mca_t *));
			ATOMIC_INT_DEALLOC(cp->rxunbound);
			kmem_free((void *)rp, sizeof(struct route_table));
			UNLOCK(dlpi_open_lock, s);
			return(EAGAIN);
		}
		ATOMIC_INT_INIT(cp->rxunbound, 0);
		ATOMIC_INT_INIT(&cp->suspenddrops, 0);

		while (nsaps)
		{
			sp->sap_state = SAP_FREE;
			++sp;
			--nsaps;
		}

		/* first open of netX should reset nrestarts back to 0.  if dlpid
		 * encounters a HWFAIL_IND then it will just close the mdi device
		 * not netX so this won't reset back to 0
		 */
		cp->txmon.nrestarts = 0;
		
		/* Initialize Pointers to Route table */
		cp->route_table = rp;
		cp->maxroutes = nroutes;
		cp->route_table->parms = rparam;

		/* Initialize main part of per_card_info struct */
		cp->mdi_primitive_handler = mdi_inactive_handler;
		cp->mdi_driver_ready = 0;

		dlpi_media_table_init();
	}

	/* note only 1 cpu at a time will get here, no race to worry about */
	if (!(sp = dlpi_allocsap(q, cp, srmode, crp)))
	{
		cmn_err(CE_WARN,"dlpiopen - Unable to allocate SAP structure");
		if (!cp->nopens) {
			LOCK_DEALLOC(cp->txmon.trylock);
			LOCK_DEALLOC(cp->lock_sap_id);
			LOCK_DEALLOC(cp->lock_card);
			RW_DEALLOC(cp->lock_mca);
			dlpi_dealloc_sap_counters(&cp->old_tx);
			dlpi_dealloc_sap_counters(&cp->old_rx);
			kmem_free((void *)cp->mcahashtbl, cp->nmcahash*sizeof(card_mca_t *));
			ATOMIC_INT_DEALLOC(cp->rxunbound);
			/* N.B.: note order of frees! */
			kmem_free((void *)rp->routes, sizeof(struct route) * nroutes);
			kmem_free((void *)rp, sizeof(struct route_table));
		}
		UNLOCK(dlpi_open_lock, s);
		return(ECHRNG);
	}

	/* now set q_ptr; if streams clone race occurs dlpiclose should
	 * free up memory
	 */
	q->q_ptr = OTHERQ(q)->q_ptr = (caddr_t)sp;

	qprocson(q);	/* enable put and svc routines */

	noenable(WR(q));
	DLPI_PRINTF02(("DLPI - SAP-Table-Entry=%x,up_q=%x q->q_ptr=%x\n", sp, sp->up_queue, q->q_ptr));

	DLPI_PRINTF01(("major=%x minor=%x\n", getmajor(*dev), (sp - cp->sap_table)));
	*dev = makedevice(getmajor(*dev), (sp - cp->sap_table));
	cp->nopens++;
	UNLOCK(dlpi_open_lock, s);
	return(0);
}

/*
 * DLPI module close routine:
 */
int
dlpiclose(queue_t *q, int flag, cred_t *crp)
{
	per_sap_info_t		*sp;
	per_card_info_t		*cp;
	pl_t			s;

	sp = (per_sap_info_t *)(q->q_ptr);

	/* check the obvious: null pointer denoting incomplete initialization */
	if (sp == NULL) {
		/* bogus pointer -- don't proceed further and free memory */
		cmn_err(CE_NOTE,"dlpiclose: streams clone race avoided(1)");
		return(0);
	}

	/* further check to determine if q_ptr points to valid per_sap_info_t
	 * sap_state shouldn't be SAP_FREE at this point ***if we made it
	 * all the way through dlpiopen to dlpi_allocsap***
	 */
	if ((sp->sap_state != SAP_ALLOC) && (sp->sap_state != SAP_BOUND)) {
		/* bogus pointer -- don't proceed further and free memory */
		cmn_err(CE_NOTE,"dlpiclose: streams clone race avoided(2)");
		return(0);
	}

	cp = sp->card_info;

	DLPI_PRINTF01(("DLPI: Device closed, nopens=%d Registered=%x control_sap=%x sp=%x\n", cp->nopens, cp->dlpidRegistered, cp->control_sap, sp));

	qprocsoff(q);	/* disable put and svc routines */

	if (cp->control_sap == sp)
	{
		cp->control_sap = (per_sap_info_t *)0;
		cp->dlpidRegistered = 0;
	}

	if (cp->media_specific && cp->media_specific->media_close)
		(*cp->media_specific->media_close)(q, sp);

	ADD_STAT(cp->old_tx.bcast, sp->sap_tx.bcast);
	ADD_STAT(cp->old_tx.mcast, sp->sap_tx.mcast);
	ADD_STAT(cp->old_tx.ucast_xid, sp->sap_tx.ucast_xid);
	ADD_STAT(cp->old_tx.ucast_test, sp->sap_tx.ucast_test);
	ADD_STAT(cp->old_tx.ucast, sp->sap_tx.ucast);
	ADD_STAT(cp->old_tx.error, sp->sap_tx.error);
	ADD_STAT(cp->old_tx.octets, sp->sap_tx.octets);
	ADD_STAT(cp->old_rx.bcast, sp->sap_rx.bcast);
	ADD_STAT(cp->old_rx.mcast, sp->sap_rx.mcast);
	ADD_STAT(cp->old_rx.ucast_xid, sp->sap_rx.ucast_xid);
	ADD_STAT(cp->old_rx.ucast_test, sp->sap_rx.ucast_test);
	ADD_STAT(cp->old_rx.ucast, sp->sap_rx.ucast);
	ADD_STAT(cp->old_rx.error, sp->sap_rx.error);
	ADD_STAT(cp->old_rx.octets, sp->sap_rx.octets);
	dlpi_clear_sap_counters(&sp->sap_tx);
	dlpi_clear_sap_counters(&sp->sap_rx);
	dlpi_clear_sap_multicasts(cp, sp);
	sp->sap_state = SAP_FREE;

	/* free any filters */
	if (sp->filter[DL_FILTER_INCOMING] != NULL) {
		freemsg(sp->filter[DL_FILTER_INCOMING]);
	}
	if (sp->filter[DL_FILTER_OUTGOING] != NULL) {
		freemsg(sp->filter[DL_FILTER_OUTGOING]);
	}

	q->q_ptr = OTHERQ(q)->q_ptr = NULL;

	dlpi_dealloc_sap_counters(&sp->sap_tx);
	dlpi_dealloc_sap_counters(&sp->sap_rx);

	if (--cp->nopens <= 0) {
		s = LOCK(dlpi_open_lock, plstr);
		if (cp->media_specific && (cp->media_specific->media_flags
		    & MEDIA_SOURCE_ROUTING)) {
			dlpiSR_uninit(cp);
		}
		LOCK_DEALLOC(cp->txmon.trylock);
		LOCK_DEALLOC(cp->lock_sap_id);
		LOCK_DEALLOC(cp->lock_card);
		RW_DEALLOC(cp->lock_mca);
		dlpi_dealloc_sap_counters(&cp->old_tx);
		dlpi_dealloc_sap_counters(&cp->old_rx);
		ATOMIC_INT_DEALLOC(cp->rxunbound);
		kmem_free((void *)cp->mcahashtbl, 
			cp->nmcahash*sizeof(card_mca_t *));
		if (cp->route_table) {
			/* N.B.: note the order of these frees! */
			kmem_free((void *)cp->route_table->routes, 
					cp->maxroutes*sizeof(struct route));
			kmem_free((void *)cp->route_table, 
					sizeof(struct route_table));
		}
		UNLOCK(dlpi_open_lock, s);
	}

	if (cp->media_svcs_queue == q)
	{
		if (cp->media_specific && cp->media_specific->media_svc_close)
		(*cp->media_specific->media_svc_close)(cp);

		cp->media_svcs_queue = NULL;
	}

	return(0);
}

/******************************************************************************
 * Media Registration.
 *	This function is the DLPI module's internal registration routine.
 *	it will override any entries in the media dependent table.
 *	See mdi_register_media() BELOW for the externally visible media
 *	registration function.
 ******************************************************************************/

per_media_info_t	mdi_media_info[MAX_MEDIA_TYPE];

STATIC int
register_media(int type, const per_media_info_t *mi)
{
	DLPI_PRINTF04(("DLPI: Media registered, media=%d\n", type));

	if ((type < 0) || (type >= MAX_MEDIA_TYPE))
		return ERANGE;

	if (!mi)
		return ENOENT;

	if (!mi->make_hdr || !mi->rx_parse)
		return EINVAL;

	mdi_media_info[type] = *mi;		/* Make private copy */
	return(0);
}

/*
 * DLPI Module's init routine.  Registers all media the DLPI module
 * knows about.  Registration will fail if another media support
 * driver has registered to support the built-in media types.
 */

STATIC void
dlpi_media_table_init()
{
	static unchar	builtin_registered = 0;

	if (!builtin_registered) {
		++builtin_registered;

		/*
		 * Register mac_media_info table for the built-in types.
		 */

		mdi_ether_register();
		mdi_token_register();
		mdi_fddi_register();
		mdi_isdn_register();
	}
}

/*
 * This function should be used by 3rd party MDI driver developers to register
 * new media-dependent code with the DLPI module.  This should not be
 * necessary unless the 3rd party is developing a driver for a new media.
 *
 * This routine should be called from the init routine of media support
 * modules except it will be called from dlpi_media_table_init() at first
 * open to provide default support for built-in types.
 *
 * Return values:
 *	EDOM	Invalid media_magic
 *	EEXIST	Media type already supported
 *	ERANGE	Invalid media type
 *	ENOENT	mi was NULL
 *	EINVAL	Missing required routine
 *	0	Successful
 */

int
mdi_register_media(ulong media_magic, int type, const per_media_info_t *mi)
{
	static unchar	table_initted = 0;

	if (!table_initted)
	{
		bzero(mdi_media_info, sizeof(mdi_media_info));
		table_initted = 1;
	}

	if (media_magic != MEDIA_MAGIC)
		return EDOM;

	if (HAVE_MEDIA_SUPPORT(type))
		return EEXIST;

	return register_media(type, mi);
}

