#ident	"@(#)kern-i386:util/mod/mod_drv.c	1.20.21.1"
#ident	"$Header$"

#define	_DDI_C

#include	<io/conf.h>
#include	<io/stream.h>
#include	<io/strsubr.h>
#include	<io/udev/udev.h>
#include	<mem/kmem.h>
#include	<mem/seg_kvn.h>
#include	<proc/cred.h>
#include	<proc/user.h>
#include	<svc/errno.h>
#include	<svc/systm.h>
#include	<util/cmn_err.h>
#include	<util/debug.h>
#include	<util/mod/mod.h>
#include	<util/mod/mod_hier.h>
#include	<util/mod/mod_k.h>
#include	<util/mod/moddefs.h>
#include	<util/mod/moddrv.h>
#include	<util/param.h>
#include 	<util/sysmacros.h>
#include	<util/types.h>
#include	<proc/bind.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <fs/specfs/specfs.h>
#include <util/kdb/xdebug.h>

#ifndef NO_RDMA
#include	<mem/rdma.h>
#endif

#include	<io/ddi.h>

/*
 * Dynamically loadable device driver support.
 */

extern	int	nxio();

extern rwlock_t mod_bdevsw_lock, mod_cdevsw_lock;
extern	void	bswtbl_bind(int), cswtbl_bind(int);

extern void mod_dlkm_extract_name();

extern int default_bindcpu;


STATIC int mod_drvinstall(struct mod_type_data *, struct module *);
STATIC int mod_drvremove(struct mod_type_data *);
STATIC int mod_drvinfo(struct mod_type_data *, int *, int *, int *);
STATIC int mod_drvbind(struct mod_type_data *, int *);
STATIC int mod_dev_load(cfg_info_t *, cred_t *);
STATIC int drv_mod_load(char *, struct module **, cred_t *);
STATIC void drv_list_init(void);
STATIC drvlist_t *drv_lookup_l(const char *);
STATIC void drv_cfg_ready();
STATIC int drv_attach_common_l(drvlist_t *);
STATIC void cfg_list_init(void);
STATIC cfg_info_t *find_instance_l(cfg_info_t *, rm_key_t, const char *);
STATIC void delete_instance_l(cfg_info_t **, cfg_info_t *);
STATIC cfg_info_t *find_instance_by_name_l(cfg_info_t *, const char *);
STATIC cfg_info_t *add_instance_l(cfg_info_t **, rm_key_t, char *,
					cfg_info_t *, struct __conf_cb *);
STATIC cfg_info_t *find_cfg_pointer_l(ulong_t);
STATIC cfg_info_t *cfgp_on_modname_l(char *modname);
STATIC int (*drv_config(cfg_info_t *))(cfg_func_t, void *, rm_key_t);
STATIC int drv_reg_v7(const char *, major_t, uint_t);

struct	mod_operations	mod_drv_ops	= {
	mod_drvinstall,
	mod_drvremove,
	mod_drvinfo,
	mod_drvbind
};

STATIC cfg_info_t *cfg_list;
STATIC drvlist_t *drv_list;

event_t	mod_notify_spawnevent;
extern int mod_verify_in_progress;
STATIC sv_t	mod_daemon_q_sv;
STATIC boolean_t mod_daemon_idle;


LKINFO_DECL(drv_list_lkinfo,"drv_list_lp",0);
LKINFO_DECL(cfg_list_lkinfo,"cfg_list_lp",0);
STATIC lock_t *drv_list_lp;
STATIC lock_t *cfg_list_lp;
STATIC pl_t drv_pl;
STATIC pl_t cfg_pl;


#define PLDDI		PLSTR
#define DDI_HIER_BASE	(KERNEL_HIER_BASE + 21)

#define	LOCK_DRVLIST()		 (drv_pl = LOCK(drv_list_lp, PLDDI))
#define	UNLOCK_DRVLIST()	 UNLOCK(drv_list_lp,drv_pl)
#define	LOCK_CFGLIST()		(cfg_pl = LOCK(cfg_list_lp, PLDDI))
#define	UNLOCK_CFGLIST()	UNLOCK(cfg_list_lp, cfg_pl)
#define	UNLOCK_CFGLIST_OUT_OF_ORDER() UNLOCK(cfg_list_lp, drv_pl)

#define	MOD_WAIT_FLAG 	0x01

#ifdef DEBUG
#define DRVDBG_CONFIG	(1 << 0)
#define DRVDBG_ATTACH	(1 << 1)
#define DRVDBG_INIT	(1 << 2)
#define DRVDBG_NOTIFY	(1 << 3)
#define DRVDBG_MOD_NODE_DATA (1 << 4)
unsigned drvdebug;
#define DRVDBG(f, x)	((drvdebug & (f)) && ((cmn_err x), 1))
#else
#define DRVDBG(f, x)
#endif

/*
 * int mod_bdev_reg(void *arg)
 *	Register loadable module for block device drivers.
 *
 * Calling/Exit State:
 *	If successful, the routine returns 0, else the appropriate errno.
 */
int
mod_bdev_reg(void *arg)
{
	struct	bdevsw	*bdevp;
	struct	mod_mreg mr;

	major_t	major;
	char	*name;
	int	retval;
	int	nmajors;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	moddebug(cmn_err(CE_CONT, "!MOD: mod_bdev_reg()\n"));

	if (copyin(arg, &mr, sizeof(struct mod_mreg)))
		return (EFAULT);

	if (mod_static(mr.md_modname))
		return (EEXIST);

	if ((major = (major_t)(mr.md_typedata)) >= bdevswsz) {
		/*
		 *+ Bdevsw overflow.
		 */
		cmn_err(CE_NOTE, "!MOD: block major number (%d) overflow",
			major);
		return (ECONFIG);
	}

	name = kmem_alloc(strlen(mr.md_modname) + 1, KM_SLEEP);
        (void)RW_WRLOCK(&mod_bdevsw_lock, PLDLM);
        bdevp = &bdevsw[major];

	/*
	 * only allow a reg if the slot was blank before
	 */
	if (strcmp(bdevp->d_name, "nodev") != 0) {
		moddebug(cmn_err(CE_NOTE, "!MOD: Registration on non-empty slot failed on block maj=%d, %s, slot is currently '%s'\n", major, mr.md_mod_name, bdevp->d_name));
		RW_UNLOCK(&mod_bdevsw_lock, PLBASE);
		return (EBUSY);
	}

	bdevp->d_name = name;

	strcpy(bdevp->d_name, mr.md_modname);

	nmajors = max(shim_total_bmajors(mr.md_modname),
			shim_total_cmajors(mr.md_modname));

	RW_UNLOCK(&mod_bdevsw_lock, PLBASE);
	if ((retval = drv_reg_v7(mr.md_modname, (major-nmajors+1), nmajors))) {
		moddebug(cmn_err(CE_NOTE, "!MOD: Registration failed on block maj=%d, %s, errno = %d\n", major, mr.md_mod_name, retval));
		return(retval);
	}
	
	moddebug(cmn_err(CE_CONT, "!MOD: %d, %s\n", major, mr.md_mod_name));

	return (0);
}

/*
 * int mod_cdev_reg(void *arg)
 *	Register loadable module for character device drivers
 *
 * Calling/Exit State:
 *	If successful, the routine returns 0, else the appropriate errno.
 */
int
mod_cdev_reg(void *arg)
{
	struct	cdevsw	*cdevp;
	struct	mod_mreg mr;
	major_t	major;
	char	*name;
	int	retval;
	int	nmajors;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	moddebug(cmn_err(CE_CONT, "MOD: mod_cdev_reg()\n"));

	if (copyin(arg, &mr, sizeof(struct mod_mreg)))
		return (EFAULT);

	if (mod_static(mr.md_modname))
		return (EEXIST);

	if ((major = (major_t)(mr.md_typedata)) >= cdevswsz) {
		/*
		 *+ Cdevsw overflow.
		 */
		cmn_err(CE_NOTE, "!MOD: character major number (%d) overflow",
			 major);
		return (ECONFIG);
	}

	name = kmem_alloc(strlen(mr.md_modname) + 1, KM_SLEEP);
        (void)RW_WRLOCK(&mod_cdevsw_lock, PLDLM);
        cdevp = &cdevsw[major];

	cdevp->d_name = name;

	strcpy(cdevp->d_name, mr.md_modname);

	nmajors = max(shim_total_bmajors(mr.md_modname),
			shim_total_cmajors(mr.md_modname));

	RW_UNLOCK(&mod_cdevsw_lock, PLBASE);
	if ((retval = drv_reg_v7(mr.md_modname, (major-nmajors+1), nmajors))) {
		moddebug(cmn_err(CE_NOTE, "!MOD: Registration failed on char maj=%d, %s, errno = %d\n", major, mr.md_mod_name, retval));
		return(retval);
	}

	moddebug(cmn_err(CE_CONT, "!MOD: %d, %s\n", major, mr.md_mod_name));

	return (0);
}

/*
 * int mod_sdev_reg(void *arg)
 *	Register loadable module for STREAMS device driver
 *
 * Calling/Exit State:
 *	If successful, the routine returns 0, else the appropriate errno.
 */
int
mod_sdev_reg(void *arg)
{
	struct	cdevsw	*cdevp;
	struct	mod_mreg mr;
	major_t	major;
	char	*name;	
	int 	retval;
	int	nmajors;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	moddebug(cmn_err(CE_CONT, "!MOD: mod_sdev_adm()\n"));

	if (copyin(arg, &mr, sizeof(struct mod_mreg)))
		return (EFAULT);

	if (mod_static(mr.md_modname))
		return (EEXIST);

	if ((major = (major_t)(mr.md_typedata)) >= cdevswsz) {
		/*
		 *+ Cdevsw overflow.
		 */
		cmn_err(CE_NOTE, "!MOD: character major number (%d) overflow",
			major);
		return (ECONFIG);
	}

	name = kmem_alloc(strlen(mr.md_modname) + 1, KM_SLEEP);
        (void)RW_WRLOCK(&mod_cdevsw_lock, PLDLM);
        cdevp = &cdevsw[major];

	cdevp->d_name = name;

	strcpy(cdevp->d_name, mr.md_modname);

	nmajors = max(shim_total_bmajors(mr.md_modname),
			shim_total_cmajors(mr.md_modname));

	RW_UNLOCK(&mod_cdevsw_lock, PLBASE);
	if ((retval = drv_reg_v7(mr.md_modname, (major-nmajors+1), nmajors))) {
		moddebug(cmn_err(CE_NOTE, "!MOD: Registration failed on Streams maj=%d, %s, errno = %d\n", major, mr.md_mod_name, retval));
		return(retval);
	}
	cdevp->d_auxflag |= DAF_STR;

	moddebug(cmn_err(CE_CONT, "!MOD: %d, %s\n", major, mr.md_mod_name));

	return (0);
}

boolean_t
drv_is_str(major_t maj)
{
	return maj < cdevcnt
		&& ((cdevsw[maj].d_auxflag & DAF_STR) || cdevsw[maj].d_str);
}

/*
 * STATIC int mod_drvinstall(struct mod_type_data *drvdatap, 
 *		struct modctl *modctlp)
 *	Connect the driver to the appropriate switch table.
 *
 * Calling/Exit State:
 *	No locks held upon calling and exit
 */
STATIC	int
mod_drvinstall(struct mod_type_data *drvdatap, struct module *modp)
{
	extern	void pageio_fix_bswtbl(uint_t);
	struct	mod_drv_data	*dta;
	uint_t	major, nm;
	char	*name;

	moddebug(cmn_err(CE_CONT, "!MOD: mod_drvinstall()\n"));
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	dta = (struct mod_drv_data *)drvdatap->mtd_pdata;

	if ((nm = dta->bmajors) != 0 && dta->bmajor_0 + nm > bdevswsz)
		return (ECONFIG);
	if ((nm = dta->cmajors) != 0 && dta->cmajor_0 + nm > cdevswsz)
		return (ECONFIG);

	/*
	 * Populate bdevsw[] for all supported block majors.
	 */
	if ((nm = dta->bmajors) != 0) {
		struct	bdevsw	*bdevp;

		major = dta->bmajor_0;
		moddebug(cmn_err(CE_CONT, "!MOD:    Block majors:\n"));

		(void)RW_WRLOCK(&mod_bdevsw_lock, PLDLM);
		while(nm--) {
			moddebug(cmn_err(CE_CONT, "!MOD: %d\n", major));

			bdevp = &bdevsw[major];
			name = bdevp->d_name;
			*bdevp = dta->drv_bdevsw;
			bdevp->d_name = name;
			bdevp->d_modp = modp;

			/*
			 * Set up the cpu binding routine if the driver
			 * is not multi threaded, or if the binding to a
			 * specific cpu is specified.
			 */
			if (bdevp->d_cpu == -2)
				bdevp->d_cpu = default_bindcpu;
			if (!(*bdevp->d_flag & D_MP) || (bdevp->d_cpu != -1))
				bswtbl_bind(major);

			/*
			 * Fix up the bdevsw entry to support non-D_NOBRKUP
			 * drivers (pageio_breakup()).
			 */
			pageio_fix_bswtbl(major);
#ifndef NO_RDMA
			if (rdma_mode != RDMA_DISABLED)
				rdma_fix_bswtbl(major);
#endif
			major++;
		}
		RW_UNLOCK(&mod_bdevsw_lock, PLBASE);
	}

	/*
	 * Populate cdevsw[] for all supported character majors.
	 */
	if ((nm = dta->cmajors) != 0)	{
		struct	cdevsw	*cdevp;

		major = dta->cmajor_0;
		moddebug(cmn_err(CE_CONT, "!MOD:    Character majors:\n"));

		(void)RW_WRLOCK(&mod_cdevsw_lock, PLDLM);
		while(nm--)	{
			moddebug(cmn_err(CE_CONT, "!MOD: %d\n", major));

			cdevp = &cdevsw[major];
			name = cdevp->d_name;
			*cdevp = dta->drv_cdevsw;
			cdevp->d_name = name;
			cdevp->d_modp = modp;

			/*
			 * set up the cpu binding routine if the driver
			 * is not multi threaded, or if the binding to a
			 * specific cpu is specified.
			 */
			if (cdevp->d_cpu == -2)
				cdevp->d_cpu = default_bindcpu;
			if (!(*cdevp->d_flag & D_MP) || (cdevp->d_cpu != -1))
				cswtbl_bind(major);
			major++;
		}
		RW_UNLOCK(&mod_cdevsw_lock, PLBASE);
	}

	/*
 	* set up the ddi <8 shim. The shim assumes that the cdev side
 	* is always populated and indeed the idtools will do that.
 	*/
	return(mod_drv_ddiinstall(drvdatap, modp));

}

/*
 * STATIC int mod_drvremove(struct mod_type_data *drvdatap)
 *	Disconnect the driver from appropriate device switch table.
 *
 * Calling/Exit State:
 *	No locks held upon calling and exit.
 */
STATIC	int
mod_drvremove(struct mod_type_data *drvdatap)
{
	struct	mod_drv_data	*dta;
	struct	module	**modpp;
	int	major, nm;

	moddebug(cmn_err(CE_CONT, "!MOD: mod_drvremove()\n"));
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	dta = (struct mod_drv_data *)drvdatap->mtd_pdata;

	/*
	 * Reset bdevsw[] for all supported block majors.
	 */
	if ((nm = dta->bmajors) != 0)	{
		struct	bdevsw	*bdevp;

		major = dta->bmajor_0;
		moddebug(cmn_err(CE_CONT, "!MOD:    Block majors:\n"));

		(void)RW_WRLOCK(&mod_bdevsw_lock, PLDLM);
		while(nm--)	{
			moddebug(cmn_err(CE_CONT, "!MOD: %d\n", major));

			/*
			 * If the next condition is true, it means
			 * that mod_drvinstall() failed at the same point, so
			 * there's no need to go on.
			 */
			if (*(modpp = &bdevsw[major].d_modp) ==
			   (struct module *)NULL) {
				RW_UNLOCK(&mod_bdevsw_lock, PLBASE);
				return (0);
			}
			*modpp = (struct module *)NULL;
			major++;
		}
		mod_drv_ddiremove(dta->drv_bdevsw.d_name);
		RW_UNLOCK(&mod_bdevsw_lock, PLBASE);
	}

	/*
	 * Reset cdevsw[] for all supported character majors.
	 */
	if ((nm = dta->cmajors) != 0)	{
		struct	cdevsw	*cdevp;

		major = dta->cmajor_0;
		moddebug(cmn_err(CE_CONT, "!MOD:    Character majors:\n"));

		(void)RW_WRLOCK(&mod_cdevsw_lock, PLDLM);
		while(nm--)	{
			moddebug(cmn_err(CE_CONT, "!MOD: %d\n", major));

			/*
			 * If either of the next 2 conditions is true, it means
			 * that mod_drvinstall() failed at the same point, so
			 * there's no need to go on.
			 */
			if ((major >= cdevswsz) ||
			    (*(modpp = &cdevsw[major].d_modp) ==
			     (struct module *)NULL)) {
				RW_UNLOCK(&mod_cdevsw_lock, PLBASE);
				return (0);
			}
			*modpp = (struct module *)NULL;
			major++;
		}
		mod_drv_ddiremove(dta->drv_cdevsw.d_name);
		RW_UNLOCK(&mod_cdevsw_lock, PLBASE);
	}

	return (0);
}

/*
 * STATIC int mod_drvinfo(struct mod_type_data *drvdatap, int *p0, 
 *		int *p1, int *type)
 *	Return the module type and the info of the first major
 *	number and the number of major numbers in both char
 *	and block device switch tables.
 *
 * Calling/Exit State:
 *	The keepcnt of the device driver is non-zero upon
 *	calling and exit of this routine.
 */
STATIC	int
mod_drvinfo(struct mod_type_data *drvdatap, int *p0, int *p1, int *type)
{
	struct	mod_drv_data	*dta;

	dta = (struct mod_drv_data *)drvdatap->mtd_pdata;

	/* Set the type to BDEV, since we only report it as device driver. */
	*type = MOD_TY_BDEV;

	p0[0] = dta->bmajor_0;
	p0[1] = dta->bmajors;

	p1[0] = dta->cmajor_0;
	p1[1] = dta->cmajors;

	return (0);
}

/*
 * STATIC int mod_drvbind(struct mod_type_data *drvdatap, int *cpup)
 *	Routine to handle cpu binding for non-MP modules.
 *	The cpu to be bind to is returned in cpup.
 *
 * Calling/Exit State:
 *	Returns 0 on success or appropriate errno if failed.
 */
STATIC int
mod_drvbind(struct mod_type_data *drvdatap, int *cpup)
{
	struct mod_drv_data	*dta;
	int drvcpu, devflag;

	dta = (struct mod_drv_data *)drvdatap->mtd_pdata;

	if (dta->bmajors != 0) {
		drvcpu = dta->drv_bdevsw.d_cpu;
		devflag = *dta->drv_bdevsw.d_flag;
	} else {
		drvcpu = dta->drv_cdevsw.d_cpu;
		devflag = *dta->drv_cdevsw.d_flag;
	}
	if (drvcpu == -2)
		drvcpu = default_bindcpu;
	if (drvcpu == -1 && !(devflag & D_MP))
		drvcpu = 0;

	if (*cpup != -1 && *cpup != drvcpu) {
		/*
		 *+ The driver is configured to bind with another cpu through
		 *+ other linkages, so cannot be loaded.
		 */
		cmn_err(CE_WARN,
	"MOD: The driver %s is illegally configured to bind with\n"
	"both cpu %d and %d; the driver cannot be loaded.",
	dta->bmajors != 0 ? dta->drv_bdevsw.d_name : dta->drv_cdevsw.d_name,
			*cpup, drvcpu);
		return (ECONFIG);
	} else
		*cpup = drvcpu;

	return (0);
}

/*
 * DDI 8 binding shims
 */
STATIC int ddi_bind_config(cfg_func_t, void *, rm_key_t);
STATIC int ddi_bind_open(void *, channel_t *, int , cred_t *, queue_t *);
STATIC int ddi_bind_close(void *, channel_t, int, cred_t *, queue_t *);
STATIC int ddi_bind_devinfo(void *, channel_t, di_parm_t, void *);
STATIC void ddi_bind_biostart(void *, channel_t, buf_t *);
STATIC int ddi_bind_ioctl(void *, channel_t, int, void *, int , cred_t *,
			int *);
STATIC int ddi_bind_drvctl(void *, channel_t, int, void *);
STATIC ppid_t ddi_bind_mmap(void *, channel_t, size_t, int);

/*
 * DDI 7 and earlier binding shims
 */
STATIC int ddi_bind_open_v7(void *, channel_t *, int , cred_t *, queue_t *);
STATIC int ddi_bind_close_v7(void *, channel_t, int, cred_t *, queue_t *);
STATIC int ddi_bind_devinfo_v7(void *, channel_t, di_parm_t, void *);
STATIC void ddi_bind_biostart_v7(void *, channel_t, buf_t *);
STATIC int ddi_bind_ioctl_v7(void *, channel_t, int, void *, int , cred_t *,
			int *);
STATIC int ddi_bind_drvctl_v7(void *, channel_t, int, void *);
STATIC ppid_t ddi_bind_mmap_v7(void *, channel_t, size_t, int);
STATIC int ddi_bind_poll_v7(dev_t, int, int, short *, struct pollhead **);

STATIC drvlist_t *drvlist_alloc(const drvinfo_t *);

STATIC const drvops_t bind_drv_ops = {
	ddi_bind_config,
	ddi_bind_open,
	ddi_bind_close,
	ddi_bind_devinfo,
	ddi_bind_biostart,
	ddi_bind_ioctl,
	ddi_bind_drvctl,
	ddi_bind_mmap
};


STATIC drvinfo_t bind_drv_info = {
	&bind_drv_ops,
	"bind_shim",
	0, NULL, 0 
};


STATIC const drvops_t bind_drv_ops_v7 = {
	ddi_bind_config,
	ddi_bind_open_v7,
	ddi_bind_close_v7,
	ddi_bind_devinfo_v7,
	ddi_bind_biostart_v7,
	ddi_bind_ioctl_v7,
	ddi_bind_drvctl,
	ddi_bind_mmap_v7
};


STATIC drvinfo_t bind_drv_info_v7 = {
	&bind_drv_ops_v7,
	"bind_shim_v7",
	0, NULL, 0 
};


/*
 * Initialize the drive attach list
 */
STATIC void
drv_list_init(void)
{
	drv_list_lp = LOCK_ALLOC((uchar_t)DDI_HIER_BASE + 1, PLDDI,
				&drv_list_lkinfo, KM_NOSLEEP);
	if (drv_list_lp == NULL)
		cmn_err(CE_PANIC, "drv_list_init: cannot allocate memory");
}

/*
 * Initialize the wait sv for adds to complete
 */
STATIC void
mod_event_init(void)
{
	EVENT_INIT(&mod_notify_spawnevent);
	SV_INIT(&mod_daemon_q_sv);
}



/*
 * Check to see if a driver has already been attached
 *
 * Returns the pointer to the list entryif the driver has been attached
 *         NULL if not attached
 */
drvlist_t *
drv_lookup(const char *name)
{
	drvlist_t *dlistp;

	/* ASSERT(KS_HOLD0LOCKS()); */
	/* ASSERT(getpl() == PLBASE); */
	LOCK_DRVLIST();
	dlistp = drv_lookup_l(name);
	UNLOCK_DRVLIST();
	return dlistp;
}

STATIC drvlist_t *
drv_lookup_l(const char *name)
{
	drvlist_t *dlistp;
	const drvinfo_t *drvip;

	ASSERT(name != NULL);
	for (dlistp = drv_list; dlistp; dlistp = dlistp->link)
		if((drvip = dlistp->info_p) != NULL
				&& strcmp(name, drvip->drv_name) == 0)
			break;
	return dlistp;
}

/*
 * Populate fmodsw drvops.  Called out of shim_static_init, when fmodsw
 * requires no locking.
 */
void
drv_populate_mod(void)
{
	int idx;
	drvlist_t *dlistp;
	const drvinfo_t *drvip;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	LOCK_DRVLIST();
	for (dlistp = drv_list; dlistp; dlistp = dlistp->link)
		if((drvip = dlistp->info_p) != NULL
				&& (drvip->drv_flags & D_MOD)) {
			for (idx = 0; idx < fmodcnt; idx++)
				if (strcmp(fmodsw[idx].f_name, drvip->drv_name)
						== 0) {
					if (!fmodsw[idx].f_flag)
						((const uint_t **)&fmodsw[idx].f_flag)[0] = &drvip->drv_flags;
					break;
				}
			if (idx == fmodcnt)
				cmn_err(CE_WARN, "!drv_populate_mod: static"
					" module %s not found in fmodsw.",
					drvip->drv_name);
		}
	UNLOCK_DRVLIST();
}

/*
 * Populate fmp from dlistp, because loading a DDI8 module does not itself
 * populate fmodsw.
 */
void
drv_listtomod(drvlist_t *dlistp, struct fmodsw *fmp)
{
	const drvinfo_t *drvip = dlistp->info_p;

	strcpy(fmp->f_name, drvip->drv_name);
	fmp->f_str = (struct streamtab *)drvip->drv_str;
	((const uint_t **)&fmp->f_flag)[0] = &drvip->drv_flags;
	fmp->f_modp = (struct module *)dlistp->modp;
}

/*
 * Register a v7 driver, creating a half-baked config instance, to be fully
 * populated later via drv_attach_v7.
 */
STATIC int
drv_reg_v7(const char *name, major_t major, uint_t nmajors)
{
	cfg_info_t *newcfg, *cfgp;
	char *modname;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	newcfg = kmem_zalloc(sizeof(*newcfg), KM_SLEEP);
	modname = kmem_alloc(strlen(name) + 1, KM_SLEEP);
	strcpy(modname, name);
	LOCK_CFGLIST();
	/*
	 * Check to see if the driver has alread been registered
	 * If it has and the name is the same, just bump the number
	 * of majors, otherwise we are changing drivers
	 */
	if ((cfgp = find_instance_by_name_l(cfg_list, modname)) != NULL ||
		(cfgp = find_cfg_pointer_l(makedevice(major,0))) != NULL) {

		kmem_free(newcfg, sizeof(*newcfg));
		if (strcmp(modname, cfgp->modname) != 0) {
			/*
			 * If the driver is still loaded, then we
			 * can't register another one over it!
			 */
			if (cfgp->dlist_p) {
				UNLOCK_CFGLIST();
				kmem_free(modname, strlen(modname) + 1);
				return EBUSY;
			}
			/* 
			 * oldname != newname means a new
			 * driver is being registered.  Free the old
			 * name and copy in the new one
			 */
			kmem_free(cfgp->modname, strlen(cfgp->modname) + 1);
			cfgp->modname = modname;
			goto done;
		}
		else {
			kmem_free(modname, strlen(modname) + 1);
		}


		if (cfgp->valid_basedev == 0) {
			/*
			 * already 'registered', only allow change in
			 * nmajors, not the basic major
			 */
			if(cfgp->basedev != makedevice(major, 0)) {
				UNLOCK_CFGLIST();
				return EEXIST;
			}
		}
	} else
		cfgp = add_instance_l(&cfg_list, -1, modname, newcfg, NULL);
done:
	cfgp->basedev = makedevice(major, 0);
	cfgp->basedev_max = cfgp->basedev + (L_MAXMIN+1) * nmajors - 1;
	cfgp->state = INSTANCE_PENDING;
	cfgp->valid_basedev = 0;
	UNLOCK_CFGLIST();
	return 0;
}

int
drv_attach_v7(const drvinfo_t *info_p, struct cdevsw *cdevp, void *shimp,
		major_t major)
{
	cfg_info_t *cfgp, *newcfg, **cfgprev;
	int error;
	drvlist_t *dlistp;
	char *modname = NULL;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	dlistp = drvlist_alloc(info_p);
	dlistp->auxflag |= DAF_PRE8;
	if (mod_static(info_p->drv_name))
		dlistp->auxflag |= DAF_STATIC;
	if (shimp) {
		newcfg = kmem_zalloc(sizeof(*newcfg), KM_SLEEP);
		modname = kmem_alloc(strlen(info_p->drv_name) + 1, KM_SLEEP);
		strcpy(modname, info_p->drv_name);
	}
	LOCK_CFGLIST();
	LOCK_DRVLIST();
	if (shimp) {
		cfgp = find_instance_by_name_l(cfg_list, modname);
		if (cfgp == NULL) 
			cfgp = add_instance_l(&cfg_list, -1, modname, newcfg, NULL);
		else {
			kmem_free(newcfg, sizeof(*newcfg));
			kmem_free(modname, strlen(modname) + 1);
			newcfg = NULL;
		}

		cfgp->idata = cfgp->shimp = shimp;
		cfgp->basedev = makedevice(major, 0);
		cfgp->basedev_max = cfgp->basedev + info_p->drv_maxchan;
		cfgp->state = INSTANCE_PENDING;
		cfgp->valid_basedev = 0;
		dlistp->drv_basedev = cfgp->basedev;
	}
	error = drv_attach_common_l(dlistp);
	if (!error) {
		ASSERT(!shimp || cfgp->dlist_p);
		if (cdevp) {
			cfgp->dlist_p->auxflag |= cdevp->d_auxflag;
			cfgp->dlist_p->d_poll = cfgp->dlist_p->d_bind_poll = cdevp->d_poll;
		}
		UNLOCK_DRVLIST();
		UNLOCK_CFGLIST();
		if (!mod_verify_in_progress) EVENT_SIGNAL(&mod_notify_spawnevent, 0);
	} else {
		/*
		 * We have held the lock since the add_instance_l, so are
		 * guaranteed that the last list element is ours.
		 */
		if (newcfg) {
			for (cfgp = cfg_list, cfgprev = &cfg_list; cfgp;
					cfgprev = &cfgp->link,
					cfgp = cfgp->link)
				;
			ASSERT(*cfgprev == newcfg);
			*cfgprev = NULL;
			kmem_free(modname, strlen(modname) + 1);
			kmem_free(newcfg, sizeof(*newcfg));
		}
		UNLOCK_DRVLIST();
		UNLOCK_CFGLIST();
		kmem_free(dlistp, sizeof(*dlistp));
	}
	return error;
}

int
_drv_attach(const drvinfo_t *info_p, uint_t mpflag)
{
	int error;
	drvlist_t *dlistp;
	pl_t pl;
	int idx;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Make sure driver's choice of interface ("mp" or not) matches
	 * the D_MP flag setting.
	 */
	if ((info_p->drv_flags & D_MP) != mpflag) {
		/*
		 *+ This indicates a mismatch between the driver flag setting
		 *+ for D_MP, indicating a multi-threaded driver, and the
		 *+ selected ddi interface version, which must have an "mp"
		 *+ suffix if and only if D_MP is set.
		 */
		cmn_err(CE_WARN,
			mpflag ? "%s: non-D_MP driver must not use"
					" \"mp\" $interface"
			       : "%s: D_MP driver must use \"mp\" $interface",
			info_p->drv_name);
		return EINVAL;
	}

	dlistp = drvlist_alloc(info_p);
	if (mod_static(info_p->drv_name))
		dlistp->auxflag |= DAF_STATIC;

	/*
	 * populate fmodsw if this is a ddi8 streams module 
	 */
	if (info_p->drv_flags & D_MOD) {
		ASSERT(info_p->drv_str); 
		pl = RW_WRLOCK(&mod_fmodsw_lock, PLSTR);
		if ((idx = findmod_l((char *)info_p->drv_name)) < 0) {
			if ((idx = findmod_slot_l()) < 0) {
				RW_UNLOCK(&mod_fmodsw_lock, pl);
				moddebug(cmn_err(CE_CONT, 
					"!MOD: module %s: fmodsw full\n",
					name));
				return(ECONFIG);
			}
			drv_listtomod(dlistp, fmodsw + idx);
		}
	}
	LOCK_CFGLIST();
	LOCK_DRVLIST();
	error = drv_attach_common_l(dlistp);
	UNLOCK_DRVLIST();
	UNLOCK_CFGLIST();
	if (info_p->drv_flags & D_MOD) {
		if (error) {
			ASSERT(error > 0); 
			bzero(fmodsw + idx, sizeof(fmodsw[idx]));
		}
		RW_UNLOCK(&mod_fmodsw_lock, pl);
	}

	if (!error){
		if (!mod_verify_in_progress) EVENT_SIGNAL(&mod_notify_spawnevent, 0);
	}
	else
	{
		kmem_free(dlistp, sizeof(*dlistp));
		if (error < 0)
			error = 0;
	}
	return error;
}

int
drv_attach(const drvinfo_t *info_p)
{
	return _drv_attach(info_p, 0);
}

int
drv_attach_mp(const drvinfo_t *info_p)
{
	return _drv_attach(info_p, D_MP);
}

/*
 * This function is called from a device driver _load routine for DDI8 and
 * later drivers.  It registers a driver's entry points, driver name, and
 * then invokes any pending config entry point CFG_ADD calls
 *
 * Returns an errno
 */
STATIC int
drv_attach_common_l(drvlist_t *newdrv)
{
	drvlist_t *dlistp, **dlprev, *olddrv;
	cfg_info_t *cfgp;
	const drvinfo_t *drvip;

	drvip = newdrv->info_p;
	if ((olddrv = drv_lookup_l(drvip->drv_name)) != NULL) {
		if (olddrv->modp) {
			cmn_err(CE_NOTE,
				"!drv_attach_common_l: driver %s already"
				" present", drvip->drv_name);
			return EEXIST;
		}
	}

	if (!drvip->drv_str && (drvip->drv_flags & D_STR)) {
		cmn_err(CE_NOTE,"!drv_attach: Attempted to attach a non-streams"
							" driver (%s) as a streams driver",
							drvip->drv_name);
		return EINVAL;
	}

	/*
	 * Is it too late to replace olddrv?  Do this in two passes
	 * for atomicity.
	 */
	if (olddrv)
		for (cfgp = cfg_list; cfgp; cfgp = cfgp->link)
			if (cfgp->modname
				&& strcmp(cfgp->modname, drvip->drv_name) == 0
				&& cfgp->state != INSTANCE_PENDING
				&& cfgp->state != INSTANCE_READY_TO_ADD) {

				cmn_err(CE_NOTE, "!drv_attach_common_l: driver"
						" %s already present",
					drvip->drv_name);

				return EEXIST;
			}

	dlprev = &drv_list;
	dlistp = drv_list;
	while (dlistp) {
		if (olddrv && dlistp == olddrv) {
			*dlprev = olddrv->link;
			dlistp = olddrv->link;
			kmem_free(olddrv, sizeof(*olddrv));
			olddrv = NULL;
			/*
			 * Because we screen every addition, there cannot
			 * be multiple dups.
			 */
			ASSERT(drv_lookup_l(drvip->drv_name) == NULL);
		} else {
			dlprev = &dlistp->link;
			dlistp = dlistp->link;
		}
	}
	*dlprev = newdrv;

	for (cfgp = cfg_list; cfgp; cfgp = cfgp->link) {
		if (cfgp->modname
			&& strcmp(cfgp->modname, drvip->drv_name) == 0) {

			ASSERT(cfgp->state == INSTANCE_PENDING
				|| cfgp->state == INSTANCE_READY_TO_ADD
				|| cfgp->state == INSTANCE_ADD_ON_RELOAD
				|| cfgp->state == INSTANCE_INVALID);

			DRVDBG(DRVDBG_ATTACH,
				(CE_CONT,
				"drv_attach_common_l: Marking 0x%x %s as"
				" INSTANCE_READY_TO_ADD\n",
				cfgp, cfgp->modname));
			if (cfgp->state != INSTANCE_INVALID)
				cfgp->state = INSTANCE_READY_TO_ADD;
			mod_daemon_idle = B_FALSE;
			cfgp->dlist_p = newdrv;
			cfgp->info_p = cfgp->bind_info_p = newdrv->info_p;
		}
	}
	return 0;
}

/*
* shutdown drivers (called by uadmin and dump).
*
* suspend all current I/O
*/

void
drv_suspend_all(void)
{
	cfg_info_t *cfgp;
	for(cfgp=cfg_list; cfgp; cfgp=cfgp->link) {
		switch(cfgp->state) {
			case INSTANCE_ALREADY_ADDED:
				if (CFG_DRVOPS(cfgp)->d_config) {
					(void)(CFG_DRVOPS(cfgp)->d_config)
					(CFG_SUSPEND,cfgp->idata,cfgp->res_key);
				}
			default:
				continue;
		}
	}
}

/*
 * Detach a driver from the system
 */
void
drv_detach(const drvinfo_t *info_p)
{
	cfg_info_t *cfgp;
	drvlist_t **prev, *drvp;

	LOCK_CFGLIST();
	/*
	 * Tear down any instances for this driver.
	 * 1st suspend all I/O
	 * then REMOVE the instance
	 * and mark the instance PENDING
	 */
	for(cfgp=cfg_list; cfgp; cfgp=cfgp->link) {
		if (cfgp->bind_info_p == info_p) {
			switch(cfgp->state) {
				case INSTANCE_READY_TO_MODIFY:
				case INSTANCE_READY_TO_RESUME:
				case INSTANCE_READY_TO_SUSPEND:
				case INSTANCE_ALREADY_ADDED:
				case INSTANCE_MODIFIED:
				case INSTANCE_SUSPENDED:
					cfgp->state = INSTANCE_PENDING;
					UNLOCK_CFGLIST();
					if (CFG_DRVOPS(cfgp)->d_config) {

						/* Run the SUSPEND */
						(void)(CFG_DRVOPS(cfgp)->d_config)
							(CFG_SUSPEND,cfgp->idata,cfgp->res_key);

						/* Run the REMOVE */
						(void)(CFG_DRVOPS(cfgp)->d_config)
							(CFG_REMOVE,cfgp->idata,cfgp->res_key);
					}
					LOCK_CFGLIST();
					break;

				case INSTANCE_READY_TO_RM:
					UNLOCK_CFGLIST();
					if (CFG_DRVOPS(cfgp)->d_config) {
						/* Run the REMOVE */
						(void)(CFG_DRVOPS(cfgp)->d_config)
							(CFG_REMOVE,cfgp->idata,cfgp->res_key);
					}
					LOCK_CFGLIST();
					delete_instance_l(&cfg_list,cfgp);
					if (cfgp->modname)
						kmem_free(cfgp->modname,strlen(cfgp->modname) + 1);
					kmem_free(cfgp,sizeof(cfg_info_t));
					break;

				case INSTANCE_READY_TO_ADD:
				case INSTANCE_ADD_ON_RELOAD:
					 cfgp->state = INSTANCE_PENDING;
					 break;


				case INSTANCE_INVALID:
					delete_instance_l(&cfg_list,cfgp);
					if (cfgp->modname)
						kmem_free(cfgp->modname,strlen(cfgp->modname) + 1);
					kmem_free(cfgp,sizeof(cfg_info_t));
					break;

				default: break;
			}
		}
	}
	LOCK_DRVLIST();
	for (prev = &drv_list, drvp = drv_list; drvp;
			prev = &drvp->link, drvp = drvp->link)
		if (drvp->info_p == info_p) {
			ASSERT(!drvp->nholds);
#ifdef DEBUG
			for (cfgp = cfg_list; cfgp; cfgp = cfgp->link)
				ASSERT(cfgp->dlist_p != drvp);
#endif
			*prev = drvp->link;
			kmem_free(drvp, sizeof(*drvp));
			break;
		}
	UNLOCK_DRVLIST();
	UNLOCK_CFGLIST();
}

/*
 * In non-error cases, guarantees that the driver denoted by cfgp is
 * callable; if the driver is loadable, the module is held.
 *
 * Returns an errno.
 */
int
drv_mod_hold(cfg_info_t *cfgp, cred_t *crp)
{
	int error;
	drvlist_t *dlistp;
	boolean_t held;
	boolean_t signalled;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	error = 0;
	held = B_FALSE;

	LOCK_CFGLIST();

	/*
	 * Must check at least once in any open; guard against
	 * sloppy drivers.
	 */
	if (cfgp->state == INSTANCE_INVALID) {
		UNLOCK_CFGLIST();
		return ENODEV;
	}

	/*
	 * Make sure that the driver is loaded.
	 */
	if (cfgp->dlist_p == NULL) {
		/*
		 * Driver was never loaded; this will never be true for
		 * statically loaded drivers.
		 */
		UNLOCK_CFGLIST();

		/*
		 * Returns with both lists locked and module held
		 * in non-error case.
		 */
		error = mod_dev_load(cfgp, crp);
		if (error)
			return error;
		held = B_TRUE;
	} else
		LOCK_DRVLIST();

	/*
	 * We hold both list locks.  The driver will be static or will have
	 * been loaded at some time in the past.  Note, however, that if
	 * the driver is loadable, but was not loaded by mod_dev_load,
	 * above, it may be in the process of being unloaded.  Also, note
	 * that a hold on the module holds cfgp->dlist_p as well, because
	 * it blocks calls to drv_detach.
	 */
	dlistp = cfgp->dlist_p;
	ASSERT(dlistp);
	if (!CFG_STATIC(cfgp)) {
		if (!held) {
			/*
			 * Get a hold on the module.  To prevent races on
			 * dlistp->modp while we drop locks, take a reference
			 * through dlistp->nholds.
			 */
			ASSERT(dlistp->nholds >= 0);
			dlistp->nholds++;
			/*
			 * Unlock for the benefit of MOD_IS_UNLOADING.
			 */
			UNLOCK_DRVLIST();
			UNLOCK_CFGLIST();
			if (!dlistp->modp || MOD_IS_UNLOADING(dlistp->modp)) {
				/*
				 * Returns with both lists locked and module
				 * held in non-error case.
				 */
				error = mod_dev_load(cfgp, crp);
				if (error) {
					LOCK_CFGLIST();
					LOCK_DRVLIST();
					goto done;
				}
			} else {
				MOD_HOLD_L(dlistp->modp, PLBASE);
				LOCK_CFGLIST();
				LOCK_DRVLIST();
			}
			held = B_TRUE;
			ASSERT(dlistp->nholds > 0);
			dlistp->nholds--;
		}

		/*
		 * Wait now for mod_notify_daemon to finish instantiating
		 * the driver.
		 */
		signalled = B_FALSE;
		while (!signalled
			&& (cfgp->state == INSTANCE_PENDING
				|| cfgp->state == INSTANCE_READY_TO_ADD)) {

			/*
			 * Module hold is effectively a dlistp hold.
			 */
			UNLOCK_CFGLIST_OUT_OF_ORDER();
			signalled = !SV_WAIT_SIG(&dlistp->sv, PRIMED,
						drv_list_lp);
			LOCK_CFGLIST();
			LOCK_DRVLIST();
		}
		if (signalled)
			error = EINTR;


	} /* end if (!CFG_STASTIC(cfgp)) */

	if (cfgp->state != INSTANCE_ALREADY_ADDED)
		error = ENODEV;	/* driver is gone */

done:
	UNLOCK_DRVLIST();
	UNLOCK_CFGLIST();
	if (error && held)
		MOD_RELE(dlistp->modp);
	return error;
}

void
drv_mod_rele(cfg_info_t *cfgp)
{

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(cfgp->dlist_p);

	if (CFG_STATIC(cfgp))
		return;
	ASSERT(cfgp->dlist_p->modp);
	MOD_RELE(cfgp->dlist_p->modp);
}

/*
 * int
 * mod_smod_load(char *name, drvlist_t **dlistpp, cred_t *crp)
 *
 * 	Load routine for STREAMS modules.
 *
 *	On success, *dlistpp is set; if a module is static, (*dlistpp)->modp
 *	is NULLed; otherwise it is set and the denoted module is held.
 *
 * Calling/Exit State:
 *	No locks held on calling and exit.
 */
int
mod_smod_load(char *name, drvlist_t **dlistpp, cred_t *crp)
{
	int error;
	drvlist_t *dlistp;
	struct module *modp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	LOCK_DRVLIST();
	dlistp = drv_lookup_l(name);
	if (dlistp) {
		if (DLISTP_STATIC(dlistp)) {
			UNLOCK_DRVLIST();
			if (!(dlistp->info_p->drv_flags & D_MOD)) {
				cmn_err(CE_WARN,
					"!mod_sdrv_mod_load: %s is not a streams"
					" module", name);
				return EINVAL;
			}
			ASSERT(dlistp->info_p->drv_str);
			ASSERT(!dlistp->modp);
			*dlistpp = dlistp;
			return 0;
		}

		/*
		 * Get a hold on the module.  Unlock for the benefit of
		 * MOD_IS_UNLOADING. To prevent races on dlistp->modp while
		 * we drop locks, take a reference through dlistp->nholds,
		 * blocking calls to drv_detach.
		 */
		ASSERT(dlistp->nholds >= 0);
		dlistp->nholds++;
		UNLOCK_DRVLIST();
		if (!dlistp->modp || MOD_IS_UNLOADING(dlistp->modp)) {
			error = drv_mod_load(name, &dlistp->modp, crp);
			if (error)
				return error;
		} else {
			MOD_HOLD_L(dlistp->modp, PLBASE);
			LOCK_DRVLIST();
		}

		ASSERT(dlistp->nholds > 0);
		dlistp->nholds--;
		UNLOCK_DRVLIST();
	} else {
		UNLOCK_DRVLIST();
		error = drv_mod_load(name, &modp, crp);
		if (error)
			return error;
		dlistp = drv_lookup_l(name);
		UNLOCK_DRVLIST();
		ASSERT(dlistp);	/* by virtue of drv_mod_load */
		dlistp->modp = modp;
	}

	if (!(dlistp->info_p->drv_flags & D_MOD)) {
		cmn_err(CE_WARN,
			"!mod_sdrv_mod_load: %s is not a streams module",
			name);
		return EINVAL;
	}
	ASSERT(dlistp->info_p->drv_str);
	*dlistpp = dlistp;
	return 0;
}		

/*
 * int
 * mod_dev_load(cfg_info_t *cfgp, cred_t *crp)
 *	DDI.8 autoload routine for all devices.
 *
 * Calling/Exit State:
 *	No locks held on calling.
 *	In the success case, updates cfgp->dlist_p->modp, and returns with
 *	the module held and with the cfg instance list and driver list
 *	locked.
 */
STATIC int
mod_dev_load(cfg_info_t *cfgp, cred_t *crp)
{
	int err;
	struct modctl *modctlp;
	struct module *modp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	moddebug(cmn_err(CE_CONT, "!MOD: mod_dev_load():%s\n", name));
	if (err = modld(cfgp->modname, crp, &modctlp, 0)) {
		moddebug(cmn_err(CE_CONT, "!MOD: load failed errno = %d\n",
				 err));
	} else {
		/*
		 * Note that in earlier autoload versions, loads of STREAMS
		 * devices returned without a hold, because qattach took a
		 * reference to every driver and module.  qattach no longer
		 * holds drivers, but we do so here, closing a race.
		 */
		modp = modctlp->mc_modp;
		MOD_HOLD_L(modp, PLBASE);
		ASSERT(cfgp->dlist_p);	/* by virtue of module load */
		LOCK_CFGLIST();
		LOCK_DRVLIST();
		cfgp->dlist_p->modp = modp;
	}
	return err;
}

/*
 * int
 * drv_mod_load(char *name, struct module **modpp, cred_t *crp)
 *	Load and hold the module denoted by name, leaving the module
 *	pointer in *modpp.
 *
 * Calling/Exit State:
 *	No locks held on calling.
 *	In the success case, returns with the driver list locked.  In failure
 *	cases, returns with no locks held.
 */
STATIC int
drv_mod_load(char *name, struct module **modpp, cred_t *crp)
{
	int error;
	struct modctl *modctlp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	moddebug(cmn_err(CE_CONT, "!MOD: drv_mod_load():%s\n", name));
	if (error = modld(name, crp, &modctlp, 0)) {
		moddebug(cmn_err(CE_CONT, "!MOD: load failed errno = %d\n",
				 error));
		return error;
	}
	MOD_HOLD_L(modctlp->mc_modp, PLBASE);
	*modpp = modctlp->mc_modp;
	LOCK_DRVLIST();
	return 0;
}

/*
 * If (1) there is no drvlist_t denoted by name, or (2) if there is, and it
 * is not held, return B_TRUE.  Otherwise (3) return B_FALSE.  In case (2),
 * NULLS any cfg_info dlist_p pointers to the drvlist_t.  Must be called
 * with module locked.  Takes and drops cfg_list and drv_list locks.
 */
boolean_t
drv_mod_unld(char *name)
{
	boolean_t res;
	drvlist_t *drvp;
	cfg_info_t *cfgp;

	LOCK_CFGLIST();
	LOCK_DRVLIST();
	drvp = drv_lookup_l((const char *)name);
	if (!drvp)
		res = B_TRUE;
	else if (!drvp->nholds) {
		res = B_TRUE;
		for (cfgp = cfg_list; cfgp; cfgp = cfgp->link)
			if (cfgp->dlist_p == drvp) {
				cfgp->dlist_p = NULL;
				/* cfgp->state = INSTANCE_PENDING; */
			}
	} else
		res = B_FALSE;
	UNLOCK_DRVLIST();
	UNLOCK_CFGLIST();
	return res;
}

void
ddi_drvinit(void)
{
	drv_list_init();
	cfg_list_init();
	mod_event_init();
}

/*
 * This function walks thru the list of statically bound DDI-8 Drivers
 * calling the load routine for each, noting whether or not sd01 was
 * initialized.  If it wasn't then return that info to the caller
 */
int
ddi_static_init()
{
	extern struct _DLKM static_DLKM[];
	struct _DLKM *ptr;
	int sd01_found = 0;
	char buf[MODMAXNAMELEN];

	ptr = static_DLKM;
	if (ptr == NULL) {
		cmn_err(CE_NOTE, "!ddi_static_init called, but no statically"
			" linked drivers!");
		return 1;
	}

	DRVDBG(DRVDBG_INIT, (CE_CONT,
		"ddi_static_init called, processing statically linked"				" drivers!\n"));
	while(ptr->_DLKM) {
		mod_dlkm_extract_name(ptr, buf);
		if (strcmp(buf,"sd01") == 0)
			sd01_found = 1;
		(ptr->_load)();		/* call the load routine */
		ptr++;
	}

	if (sd01_found)
		return 0;
	else
		return 1;
}

/*
 * This routine will search a linked list of cfg_info_t structures looking
 * to find a matching resmgr key.  If it is found it returns a pointer to
 * the structure, otherwise it returns NULL
 */
STATIC cfg_info_t *
find_instance_l(cfg_info_t *cfgp, rm_key_t key, const char *modname)
{
	for ( ; cfgp; cfgp = cfgp->link)
		if (cfgp->res_key == key
			|| (cfgp->res_key == -1
				&& cfgp->modname
				&& strcmp(cfgp->modname, modname) == 0))
			break;
	return cfgp;
}


/*
 * This routine will search a linked list of cfg_info_t structures looking
 * to find a matching config ptr.  If it is found it deletes the item
 * from the list.
 */
void
delete_instance_l(cfg_info_t **listp, cfg_info_t *instance)
{
	cfg_info_t **lastp;

	while(*listp) {
		lastp = listp;
		if (*listp == instance) {
			*lastp = instance->link;
			break;
		}
		listp = &(*listp)->link;
	}
}

/*
 * This routine will search a linked list of cfg_info_t structures looking
 * to find a matching resmgr key.  If it is found it returns a pointer to
 * the structure, otherwise it returns NULL
 */
STATIC cfg_info_t *
find_instance_by_name_l(cfg_info_t *cfgp, const char *modname)
{
	for ( ; cfgp; cfgp = cfgp->link)
		if (cfgp->modname && (strcmp(cfgp->modname, modname) == 0))
			break;
	return cfgp;
}

/*
 * This routine will add an instance record at the end of a linked cfgp of
 * instance records
 */
STATIC cfg_info_t *
add_instance_l(cfg_info_t **listarg, rm_key_t key, char *modname,
		cfg_info_t *newcfg, struct __conf_cb *cbp)
{
	cfg_info_t *cfgp, **cfgprev;

	for (cfgprev = listarg, cfgp = *listarg; cfgp;
			cfgprev = &cfgp->link, cfgp = cfgp->link)
		;
	*cfgprev = newcfg;
	newcfg->modname = modname;
	newcfg->res_key = key;
	newcfg->link = NULL;
	newcfg->conf_cb = cbp;
	return newcfg;
}


void
ddi_notify_mod(cfg_func_t func, rm_key_t key, cfg_info_t *cur_cfgp,
	       const char *modname, struct __conf_cb *cbp)
{
	cfg_info_t *instance, *newcfg;
	drvlist_t *ptr;
	char *mname;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);


	switch (func) {
	case CFG_ADD:
		newcfg = kmem_zalloc(sizeof(*newcfg), KM_SLEEP);
		newcfg->valid_basedev = -1;
		mname = kmem_alloc(strlen(modname) + 1, KM_SLEEP);
		strcpy(mname, modname);

		LOCK_CFGLIST();
		instance = find_instance_l(cfg_list, key, mname);
		if (instance == NULL) {
			/*
			 * No instance found for this key
			 */
			instance = add_instance_l(&cfg_list, key, mname,
						newcfg,cbp);
			if ((ptr = drv_lookup(mname))) {
				if (instance->dlist_p == NULL)
					instance->dlist_p = ptr;
				instance->state = INSTANCE_READY_TO_ADD;
			} else
				instance->state = INSTANCE_PENDING;
			instance->action = NODE_CREATE;
		} else {
			if (instance->res_key == -1)
				instance->res_key = key;
			kmem_free(newcfg, sizeof(*newcfg));
			instance->modname = mname;
			if ((ptr = drv_lookup(mname))) {
				if (instance->dlist_p == NULL)
					instance->dlist_p = ptr;
				instance->state = INSTANCE_READY_TO_ADD;
			} else
				instance->state = INSTANCE_PENDING;
			instance->action = NODE_CREATE;
		}
		mod_daemon_idle = B_FALSE;
		UNLOCK_CFGLIST();
		break;

	case CFG_REMOVE:
		LOCK_CFGLIST();
		if (cur_cfgp == NULL)
			instance = find_instance_l(cfg_list, key, modname);
		else
			instance = cur_cfgp;

		if (instance == NULL) {
			UNLOCK_CFGLIST();
			break;	/* no such instance */
		}
		if (instance->state == INSTANCE_READY_TO_ADD
			  || instance->state == INSTANCE_PENDING
			  || instance->state == INSTANCE_ADD_ON_RELOAD) {

			delete_instance_l(&cfg_list,instance);
			if (instance->modname)
				kmem_free(instance->modname,strlen(instance->modname) + 1);
			kmem_free(instance,sizeof(cfg_info_t));
			UNLOCK_CFGLIST();
			break;

		} else {
			instance->state = INSTANCE_READY_TO_RM;
			instance->action = NODE_DELETE;
		}
		mod_daemon_idle = B_FALSE;
		instance->conf_cb = cbp;
		LOCK_DRVLIST();
		if (SV_BLKD(&instance->dlist_p->sv)) {
			UNLOCK_DRVLIST();
			SV_BROADCAST(&instance->dlist_p->sv, 0);
		} else
			UNLOCK_DRVLIST();
		UNLOCK_CFGLIST();
		break;

	case CFG_MODIFY:
		if (cur_cfgp) {
			if (cur_cfgp->state != INSTANCE_SUSPENDED)
				break;
			LOCK_CFGLIST();
			cur_cfgp->res_key = key;
			cur_cfgp->state = INSTANCE_READY_TO_MODIFY;
			cur_cfgp->conf_cb = cbp;
			mod_daemon_idle = B_FALSE;
			UNLOCK_CFGLIST();
		}
		LOCK_DRVLIST();
		if (SV_BLKD(&cur_cfgp->dlist_p->sv)) {
			UNLOCK_DRVLIST();
			SV_BROADCAST(&cur_cfgp->dlist_p->sv, 0);
		} else
			UNLOCK_DRVLIST();
		break;

	case CFG_SUSPEND:
		LOCK_CFGLIST();
		if (cur_cfgp == NULL) 
			instance = find_instance_l(cfg_list, key, modname);
		else
			instance = cur_cfgp;
		if (instance == NULL) {
			UNLOCK_CFGLIST();
			break;	/* no such instance */
		}
		if (instance->state == INSTANCE_ALREADY_ADDED) {
			instance->state = INSTANCE_READY_TO_SUSPEND;
			mod_daemon_idle = B_FALSE;
		} 
		instance->conf_cb = cbp;
		LOCK_DRVLIST();
		if (SV_BLKD(&instance->dlist_p->sv)) {
			UNLOCK_DRVLIST();
			SV_BROADCAST(&instance->dlist_p->sv, 0);
		} else
			UNLOCK_DRVLIST();
		UNLOCK_CFGLIST();

		break;

	case CFG_RESUME:
		LOCK_CFGLIST();
		if (cur_cfgp == NULL) 
			instance = find_instance_l(cfg_list, key, modname);
		else
			instance = cur_cfgp;
		if (instance == NULL) {
			UNLOCK_CFGLIST();
			break;	/* no such instance */
		}
		if (instance->state == INSTANCE_SUSPENDED
			  || instance->state == INSTANCE_MODIFIED) {
			instance->state = INSTANCE_READY_TO_RESUME;
			mod_daemon_idle = B_FALSE;
		} 
		instance->conf_cb = cbp;
		LOCK_DRVLIST();
		if (SV_BLKD(&instance->dlist_p->sv)) {
			UNLOCK_DRVLIST();
			SV_BROADCAST(&instance->dlist_p->sv, 0);
		} else
			UNLOCK_DRVLIST();
		UNLOCK_CFGLIST();

		break;

	default:
		cmn_err(CE_WARN, "!specfs: ddi_notify_mod: Unknown operation");
		break;
	}

	DRVDBG(DRVDBG_NOTIFY,
		(CE_NOTE, "notify_mod: sending event signal"));

	if (!mod_verify_in_progress) EVENT_SIGNAL(&mod_notify_spawnevent, 0);
	udev_signal();		/* Notify the udev driver that there are
				 * nodes to be processed */

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
}

/*
 * Initialize the instance list
 */
STATIC void
cfg_list_init(void)
{
	cfg_list_lp = LOCK_ALLOC((uchar_t)DDI_HIER_BASE, PLDDI,
				&cfg_list_lkinfo, KM_NOSLEEP);
	if (cfg_list_lp == NULL)
		cmn_err(CE_PANIC, "cfg_list_init: cannot allocate memory");

	specfs_basedev = (cdevcnt + 1) << L_BITSMINOR;
}

/*
 * This routine calls the config entry point for the specified driver
 * if there are any pending CFG_ADD calls.  It has to scan the list
 * twice to look for any CFG_REMOVE calls that may cancel out a CFG_ADD
 * call.  Each CONFIG call must be run on its own kernel thread in order
 * to prevent system deadlocks.
 */
void
mod_notify_daemon(void)
{
#define	VTOC_BASEDEV_VALUE	0x77FC0000
	int i, ret;
	cfg_info_t *cfgp;
	int (*cfgfunc) ();
	cm_args_t cma;
	drvlist_t *dlistp;
	const drvinfo_t *drvip;
	boolean_t mod_work_left;
	extern char VTOC_modname[];
	static ulong_t vtoc_basedev = VTOC_BASEDEV_VALUE;

	u.u_lwpp->l_name = "mod_notifyd";

	LOCK_CFGLIST();
	for (;;) {

		DRVDBG(DRVDBG_NOTIFY,
			(CE_NOTE, "mod_notify_deamon... processing..."));

		mod_work_left = B_FALSE;

		for (cfgp = cfg_list; cfgp; cfgp = cfgp->link) {
			if (cfgp->state == INSTANCE_READY_TO_ADD) {
				dlistp = cfgp->dlist_p;
				drvip = dlistp->info_p;
				cfgp->bind_info_p = drvip;
				cfgp->info_p = drvip;


				cfgfunc = CFG_DRVOPS(cfgp)->d_config;

				mod_work_left = B_TRUE;
				UNLOCK_CFGLIST();

				/* 
				 * Get the CPU Binding information from the
				 * database
				 */
				cma.cm_key = cfgp->res_key;
				cma.cm_n = 0;
				cma.cm_val = &cfgp->bind_cpu;
				cma.cm_vallen = sizeof(cfgp->bind_cpu);
				cma.cm_param = CM_BINDCPU;
				cm_begin_trans(cfgp->res_key, RM_READ);
				ret = cm_getval(&cma);
				cm_end_trans(cfgp->res_key);
				if (ret == ENOENT)
					cfgp->bind_cpu = -1;

				LOCK_CFGLIST();
				if (!drvip->drv_str) {
					if (!(drvip->drv_flags & D_MP))
						cfgp->bind_cpu = 0;
					/* 
					 * if CPU binding is specified make sure it
					 * is only on CPU 0
					 */
					if (cfgp->bind_cpu != -1) {
						if (cfgp->bind_cpu != 0) {
							cmn_err(CE_WARN, "mod_notify_daemon: Attempt to bind to a cpu other than CPU 0.\nCFG_ADD not performed for module %s", drvip->drv_name);
							continue;
						}
						if (cfgp->bind_cpu < 0 || cfgp->bind_cpu >= Nengine)
							cfgp->bind_cpu = 0;
						/* Interpose the binding calls */
						cfgp->idata = cfgp;
						if (CFG_PRE_DDI8(cfgp)) {
							cfgp->info_p = &bind_drv_info_v7;
							cfgp->dlist_p->d_poll = ddi_bind_poll_v7;
						}
						else {
							cfgp->info_p = &bind_drv_info; 
							cfgfunc = CFG_DRVOPS(cfgp)->d_config;
						}
					}
				}
				UNLOCK_CFGLIST();

				DRVDBG(DRVDBG_NOTIFY,
					(CE_NOTE, "mod:%s cfgentry:%x",
					drvip->drv_name, cfgfunc));


				if (cfgfunc)
					ret = (*cfgfunc) (CFG_ADD, &cfgp->idata, cfgp->res_key);
				else
					ret = 0;


				DRVDBG(DRVDBG_NOTIFY,
					(CE_NOTE, "mod: returned from CONFIG"
					" call"));
				LOCK_CFGLIST();

				cfgp->ret_code = ret;

				/*
				 * If the CFG_ADD has failed, we don't 
				 * really know why, so mark this instance
				 * to be retried if the driver gets unloade
				 * and then reloaded.
				 */
				if (ret == 0)
					cfgp->state = INSTANCE_ALREADY_ADDED;
				else
					cfgp->state = INSTANCE_ADD_ON_RELOAD;


				LOCK_DRVLIST();
				if (SV_BLKD(&dlistp->sv)) {
					UNLOCK_DRVLIST();
					SV_BROADCAST(&dlistp->sv, 0);
				} else
					UNLOCK_DRVLIST();

				/* CONF Daemon callbacks */
				if (cfgp->conf_cb) 
					if (cfgp->conf_cb->cb_func) {
						void (*func)(void *);
						func = cfgp->conf_cb->cb_func;
						cfgp->conf_cb->cb_cfgp = cfgp;
						cfgp->conf_cb->cb_type = CFG_ADD;
						cfgp->conf_cb->cb_ret = ret;
						UNLOCK_CFGLIST();
						(*func)(cfgp->conf_cb);
						LOCK_CFGLIST();
					}

				if (cfgp->valid_basedev != 0) {
					/*
					 * MAJOR FREAKING HACK/KLUDGE for VTOC 
					 * because the PDI/SDI mkdev user level
					 * code is so messed up.  VTOC can only
					 * have one "major" number eventho it
					 * shouldn't care!!  This code must go
					 * away once sdimkdev is fixed!!!!!
					 * This code also assumes that VTOC is
					 * statically bound
					 */
					if (strcmp(cfgp->modname,VTOC_modname) == 0) {
						cfgp->basedev = vtoc_basedev;
						cfgp->basedev_max = vtoc_basedev +
							drvip->drv_maxchan;
						if (cfgp->basedev_max >= drvip->drv_maxchan 
								&& cfgp->basedev_max <= 0x7FFFFFFF) {
							vtoc_basedev = cfgp->basedev_max + 1;
							cfgp->valid_basedev = 0;
						}
					}
					else {
						if (specfs_basedev < VTOC_BASEDEV_VALUE &&
							(specfs_basedev + drvip->drv_maxchan) <
								VTOC_BASEDEV_VALUE) {
							cfgp->basedev = specfs_basedev;
							cfgp->basedev_max = specfs_basedev
								+ drvip->drv_maxchan;
							specfs_basedev = cfgp->basedev_max + 1;
							cfgp->valid_basedev = 0;
						}
					}
				}


			} else if (cfgp->state == INSTANCE_READY_TO_RM) {
				drvip = cfgp->dlist_p->info_p;
				cfgfunc = CFG_DRVOPS(cfgp)->d_config;

				UNLOCK_CFGLIST();
				DRVDBG(DRVDBG_NOTIFY,
					(CE_NOTE, "mod:%s cfgentry:%x",
					drvip->drv_name, cfgfunc));

				if (cfgfunc)
					ret = (*cfgfunc) (CFG_REMOVE, cfgp->idata, cfgp->res_key);
				else
					ret = 0;
				DRVDBG(DRVDBG_NOTIFY,
					(CE_NOTE, "mod: returned from CONFIG"
					" call"));

				LOCK_CFGLIST();
				cfgp->ret_code = ret;
				cfgp->state = INSTANCE_INVALID;

				/* CONF Daemon callbacks */
				if (cfgp->conf_cb)
					if (cfgp->conf_cb->cb_func) {
						void (*func)(void *);
						func = cfgp->conf_cb->cb_func;
						cfgp->conf_cb->cb_cfgp = cfgp;
						cfgp->conf_cb->cb_type = CFG_REMOVE;
						cfgp->conf_cb->cb_ret = ret;
						UNLOCK_CFGLIST();
						(*func)(cfgp->conf_cb);
						LOCK_CFGLIST();
					}

				cfgp->state = INSTANCE_INVALID;

			}
			else if (cfgp->state == INSTANCE_READY_TO_SUSPEND) {
				drvip = cfgp->dlist_p->info_p;
				cfgfunc = CFG_DRVOPS(cfgp)->d_config;

				UNLOCK_CFGLIST();
				DRVDBG(DRVDBG_NOTIFY,
					(CE_NOTE, "mod:%s cfgentry:%x",
					drvip->drv_name, cfgfunc));

				if (cfgfunc)
					ret = (*cfgfunc) (CFG_SUSPEND, cfgp->idata, cfgp->res_key);
				else
					ret = 0;


				DRVDBG(DRVDBG_NOTIFY,
					(CE_NOTE, "mod: returned from CONFIG"
					" call"));

				LOCK_CFGLIST();
				cfgp->ret_code = ret;
				if (ret == 0)
					cfgp->state = INSTANCE_SUSPENDED;

				/* CONF Daemon callbacks */
				if (cfgp->conf_cb)
					if (cfgp->conf_cb->cb_func) {
						void (*func)(void *);
						func = cfgp->conf_cb->cb_func;
						cfgp->conf_cb->cb_cfgp = cfgp;
						cfgp->conf_cb->cb_type = CFG_SUSPEND;
						cfgp->conf_cb->cb_ret = ret;
						UNLOCK_CFGLIST();
						(*func)(cfgp->conf_cb);
						LOCK_CFGLIST();
					}
			}
			else if (cfgp->state == INSTANCE_READY_TO_RESUME) {
				drvip = cfgp->dlist_p->info_p;
				cfgfunc = CFG_DRVOPS(cfgp)->d_config;

				UNLOCK_CFGLIST();
				DRVDBG(DRVDBG_NOTIFY,
					(CE_NOTE, "mod:%s cfgentry:%x",
					drvip->drv_name, cfgfunc));

				if (cfgfunc)
					ret = (*cfgfunc) (CFG_RESUME, cfgp->idata, cfgp->res_key);
				else
					ret = 0;

				DRVDBG(DRVDBG_NOTIFY,
					(CE_NOTE, "mod: returned from CONFIG"
					" call"));

				LOCK_CFGLIST();
				cfgp->ret_code = ret;
				if (ret == 0)
					cfgp->state = INSTANCE_ALREADY_ADDED;
				/* CONF Daemon callbacks */
				if (cfgp->conf_cb)
					if (cfgp->conf_cb->cb_func) {
						void (*func)(void *);
						func = cfgp->conf_cb->cb_func;
						cfgp->conf_cb->cb_cfgp = cfgp;
						cfgp->conf_cb->cb_type = CFG_RESUME;
						cfgp->conf_cb->cb_ret = ret;
						UNLOCK_CFGLIST();
						(*func)(cfgp->conf_cb);
						LOCK_CFGLIST();
					}

			}
			else if (cfgp->state == INSTANCE_READY_TO_MODIFY) {
				drvip = cfgp->dlist_p->info_p;
				cfgfunc = CFG_DRVOPS(cfgp)->d_config;

				UNLOCK_CFGLIST();
				DRVDBG(DRVDBG_NOTIFY,
					(CE_NOTE, "mod:%s cfgentry:%x",
					drvip->drv_name, cfgfunc));

				if (cfgfunc)
					ret = (*cfgfunc) (CFG_MODIFY, cfgp->idata, cfgp->res_key);
				else
					ret = 0;

				DRVDBG(DRVDBG_NOTIFY,
					(CE_NOTE, "mod: returned from CONFIG"
					" call"));

				LOCK_CFGLIST();
				cfgp->ret_code = ret;
				if (ret == 0)
					cfgp->state = INSTANCE_MODIFIED;
				/* CONF Daemon callbacks */
				if (cfgp->conf_cb)
					if (cfgp->conf_cb->cb_func) {
						void (*func)(void *);
						func = cfgp->conf_cb->cb_func;
						cfgp->conf_cb->cb_cfgp = cfgp;
						cfgp->conf_cb->cb_type = CFG_MODIFY;
						cfgp->conf_cb->cb_ret = ret;
						UNLOCK_CFGLIST();
						(*func)(cfgp->conf_cb);
						LOCK_CFGLIST();
					}

			}
		}

		if (!mod_work_left) {
			mod_daemon_idle = B_TRUE;
			UNLOCK_CFGLIST();
			SV_BROADCAST(&mod_daemon_q_sv,0);
			DRVDBG(DRVDBG_NOTIFY,
				(CE_NOTE, "mod_notify_deamon... waiting..."));
			EVENT_WAIT(&mod_notify_spawnevent, PRIMED);
			LOCK_CFGLIST();
			mod_daemon_idle = B_FALSE;
		}
	}

#undef	VTOC_BASEDEV_VALUE

}

/*
 * This routine will search a linked list of cfg_info_t structures looking
 * to find a matching range of basedevs .  If it is found it returns a
 * pointer to the structure, otherwise it returns NULL
 */
cfg_info_t *
find_cfg_pointer(ulong_t dev)
{
	cfg_info_t *cfgp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	LOCK_CFGLIST();
	cfgp = find_cfg_pointer_l(dev);
	UNLOCK_CFGLIST();
	return cfgp;
}

STATIC cfg_info_t *
find_cfg_pointer_l(ulong_t dev)
{
	cfg_info_t *cfgp;

	for (cfgp = cfg_list; cfgp; cfgp = cfgp->link)
		if (dev >= cfgp->basedev && dev <= cfgp->basedev_max) {

if (getmajor(dev) < getmajor(cfgp->basedev))
	cmn_err(CE_PANIC, "find_cfg_pointer_l: line %d: getmajor(dev) %d < getmajor(cfgp 0x%x ->basedev) %d", __LINE__, getmajor(dev), cfgp, getmajor(cfgp->basedev));

if (getmajor(dev) > getmajor(cfgp->basedev_max))
	cmn_err(CE_PANIC, "find_cfg_pointer_l: line %d: getmajor(dev) %d > getmajor(cfgp 0x%x ->basedev_max) %d", __LINE__, getmajor(dev), cfgp, getmajor(cfgp->basedev_max));

			break;
		}
	return cfgp;
}

boolean_t
mod_node_data_available(void)
{
	cfg_info_t *cfgp;
	int res = B_FALSE;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	DRVDBG(DRVDBG_MOD_NODE_DATA,
		(CE_CONT, "mod_node_data_available: called\n"));
	LOCK_CFGLIST();
	for (cfgp = cfg_list; cfgp; cfgp = cfgp->link)
		if (cfgp->res_key != -1 && (cfgp->action & NODE_ACTION)
			&& (cfgp->state != INSTANCE_INVALID)) {
			res = B_TRUE;
			DRVDBG(DRVDBG_MOD_NODE_DATA,
				(CE_CONT,
				"mod_node_data_avail: returned AVAILABLE\n"));
			break;
		}
	UNLOCK_CFGLIST();
	return res;
}

/*
 * Serialized by cfg_list lock; used by mod_get_node_data to avoid
 * cfg_list searches.
 */
STATIC cfg_info_t *node_ptr;

boolean_t
mod_get_node_data(mod_node_info_t *ptr)
{
	cfg_info_t *cfgp;
	mod_node_info_t pass_out;
	static char *ACTION_STRINGS[] = {
		"NO-ACTION",
		"NODE-CREATE",
		"NODE-DELETE",
	};

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	LOCK_CFGLIST();

	DRVDBG(DRVDBG_MOD_NODE_DATA,
		(CE_NOTE, "mod_get_node_data: looking for work starting at %x",
		cfgp));
	for (cfgp = cfg_list ; cfgp; cfgp = cfgp->link) {
		if (cfgp->res_key != -1 && (cfgp->action & NODE_ACTION)) {
			if (cfgp->state != INSTANCE_INVALID) {
				ptr->key = cfgp->res_key;
				ptr->sbasedev = specfs_basedev;
				ptr->action.command = cfgp->action;
				cfgp->action &= ~NODE_ACTION;
					DRVDBG(DRVDBG_MOD_NODE_DATA,
					(CE_NOTE,"mod_get_node_data: reskey = %d, next"
					" is %x, basedev is %x, driver is %s",
					cfgp->res_key, cfgp,ptr->sbasedev,
						cfgp->modname));
				UNLOCK_CFGLIST();
				return B_TRUE;
			}
		}
	}
	UNLOCK_CFGLIST();
	DRVDBG(DRVDBG_MOD_NODE_DATA,
		(CE_NOTE,"mod_get_node_data: NO work found\n"));
	return B_FALSE;
}

boolean_t
mod_update_node_data(mod_node_info_t *ptr)
{
	cfg_info_t *cfgp;
	mod_node_info_t pass_out;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	DRVDBG(DRVDBG_MOD_NODE_DATA,(CE_NOTE, 
		"mod_update_node_data: status = %d",ptr->action.status));

	if (ptr->action.status == NODE_ACTION_ERROR)
		return B_FALSE;

	LOCK_CFGLIST();
	for (cfgp = cfg_list; cfgp; cfgp = cfgp->link)
		if (ptr->key == cfgp->res_key) {
			if (ptr->action.status == NODE_ACTION_SUCCESS) {
				cfgp->res_key = ptr->key;
				cfgp->basedev = ptr->sbasedev;
				cfgp->basedev_max = ptr->ebasedev;
				cfgp->action = NODE_NO_ACTION;
				if ((ptr->ebasedev + 1) > specfs_basedev) {
					/*
					 * update global basedev
					 */
					specfs_basedev = ptr->ebasedev + 1;
				}
				cfgp->valid_basedev = 0;
				break;
			}
			else if (ptr->action.status == NODE_ACTION_REQUEUE) {
				cfgp->action |= NODE_ACTION;
				break;
			}
			else if (ptr->action.status == NODE_ACTION_IGNORED) {
				break;
			}
			else {
				cmn_err(CE_WARN,"!mod_update_node_data: Invalid node "
					"update request for mod %s", cfgp->modname);
				break;
			}
		}
	UNLOCK_CFGLIST();
	return cfgp != NULL;
}

#ifdef	DEBUG

void
print_1_drv_list(drvlist_t *dlistp)
{
	debug_printf("modname = %s",
			dlistp->info_p ? dlistp->info_p->drv_name : "NULL");
	debug_printf(" link = %x",dlistp->link);
	debug_printf(" info_p = %x\n",dlistp->info_p);
}

void
print_drv_list(void)
{
	drvlist_t *dlistp;

	for (dlistp = drv_list; dlistp; dlistp = dlistp->link)
		print_1_drv_list(dlistp);
}

void
print_cfg_info(cfg_info_t *cfgp)
{
	if (cfgp->modname) 
		debug_printf("modname = %s ", cfgp->modname);
	else
		debug_printf("modname = NULL ");
	debug_printf("res_key = %x ", cfgp->res_key);
	debug_printf("dlist_p = %x ", cfgp->dlist_p);
	debug_printf("idata = %x ", cfgp->idata);
	debug_printf("instance_num = %d ", cfgp->instance_num);
	debug_printf("basedev = %x ", cfgp->basedev);
	debug_printf("basedev = %x ", cfgp->basedev_max);
	debug_printf("state = %d ", cfgp->state);
	debug_printf("ret_code = %d ", cfgp->ret_code);
	debug_printf("link = %x\n", cfgp->link);
}

void
print_cfg_list(void)
{
	cfg_info_t *cfgp;

	for (cfgp = cfg_list; cfgp; cfgp = cfgp->link)
		print_cfg_info(cfgp);
}

#endif

/*
 * This routine will search a linked list of cfg_info_t structures looking
 * to find a matching res manager key.  If it is found it returns a TRUE
 * otherwise return FALSE.  Passes out the  matching basedev value.
 * Assumes ONLY on instance.  The only consumers of these routines should
 * be SDI
 */
boolean_t
find_basedev_on_rmkey(rm_key_t key, ulong_t *basedev)
{
	cfg_info_t *cfgp;
	boolean_t res = B_FALSE;

	LOCK_CFGLIST();
	for (cfgp = cfg_list; cfgp; cfgp = cfgp->link)
		if (cfgp->res_key == key) {
			*basedev = cfgp->basedev;
			res = B_TRUE;
			break;
		}
	UNLOCK_CFGLIST();
	return res;
}

/*
 * This routine will search a linked list of cfg_info_t structures looking
 * to find a matching dev_t.  If it is found it returns a the rm_key for the
 * record otherwise if returns RM_NULL_KEY
 * The only consumer of this routine should DDI providers or base kernel
 */
rm_key_t
find_rmkey_on_dev(ulong_t dev)
{
	rm_key_t res = RM_NULL_KEY;
	cfg_info_t *cfgp;

	LOCK_CFGLIST();
	for (cfgp = cfg_list; cfgp; cfgp = cfgp->link)
		if (dev >= cfgp->basedev && dev <= cfgp->basedev_max) {
			res = cfgp->res_key;
			break;
		}
	UNLOCK_CFGLIST();
	return res;
}

/*
 * This routine will search a linked list of cfg_info_t structures looking
 * to find a matching modname.  If it is found it returns a TRUE otherwise
 * return FALSE.  Passes out the  matching basedev value.  Assumes ONLY on
 * instance.  The only consumers of these routines should be SDI
 */
boolean_t
find_basedev_on_modname(char *modname, ulong_t *basedev)
{
	cfg_info_t *cfgp;

	LOCK_CFGLIST();
	cfgp = cfgp_on_modname_l(modname);
	if (cfgp)
		*basedev = cfgp->basedev;
	UNLOCK_CFGLIST();
	return cfgp != NULL;
}

STATIC cfg_info_t *
cfgp_on_modname_l(char *modname)
{
	cfg_info_t *cfgp;

	for (cfgp = cfg_list; cfgp; cfgp = cfgp->link)
		if (cfgp->modname && strcmp(modname, cfgp->modname) == 0)
			break;
	return cfgp; 
}

/*
 * This routine will search a linked list of cfg_info_t structures looking
 * to find a matching range of basedevs.  If it is found it returns a
 * pointer to the structure, otherwise it returns NULL
 */
cfg_info_t *
find_cfgp_key(rm_key_t key)
{
	cfg_info_t *cfgp;

	LOCK_CFGLIST();
	for (cfgp = cfg_list; cfgp; cfgp = cfgp->link)
		if (cfgp->res_key == key)
				break;
	UNLOCK_CFGLIST();
	return cfgp;
}

STATIC drvlist_t *
drvlist_alloc(const drvinfo_t *drvip)
{
	drvlist_t *dlistp;

	dlistp = kmem_zalloc(sizeof(*dlistp), KM_SLEEP);
	dlistp->info_p = drvip;
	SV_INIT(&dlistp->sv);
	return dlistp;
}

/*
 * The following routines are used for doing driver binding
 */
STATIC int
ddi_bind_config(cfg_func_t func, void *idata, rm_key_t key)
{
	cfg_info_t *cp;
	int ret, (*config)();
	engine_t *oldengp;
	label_t saveq;
	void *savep;

	if (func == CFG_ADD) {
		cp  = (cfg_info_t *)*(void **)idata;
	}
	else {
		cp = (cfg_info_t *)idata;
	}

	config = CFG_ORIG_DRVOPS(cp)->d_config;
	if (config) {
		savep = u.u_cfgp;
		u.u_cfgp = (void *)cp;
		oldengp = kbind(&engine[cp->bind_cpu]);
		DISABLE_PRMPT();
		u.u_lwpp->l_notrt++;

		saveq = u.u_qsav;
		if (setjmp(&u.u_qsav)) {
			u.u_qsav = saveq;
			ASSERT(u.u_lwpp->l_notrt != 0);
			u.u_lwpp->l_notrt--;
			ENABLE_PRMPT();
			kunbind(oldengp);
			return EINTR;
		}

		if (func == CFG_ADD)
			ret = (*config)(func,&cp->bind_idata,key);
		else
			ret = (*config)(func,cp->bind_idata,key);

		u.u_qsav = saveq;
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
		ENABLE_PRMPT();
		kunbind(oldengp);
	}
	else
		ret = 0;

	u.u_cfgp = savep;
	return ret;
}

STATIC int
ddi_bind_open(void *idata, channel_t *channelp, int oflags, cred_t *crp, queue_t *q)
{
	cfg_info_t *cp;
	int ret, (*open)();
	engine_t *oldengp;
        label_t saveq;
	void *savep;

	cp = (cfg_info_t *)idata; /* Get the pointer to the cfg info struct */
	open = CFG_ORIG_DRVOPS(cp)->d_open;

        oldengp = kbind(&engine[cp->bind_cpu]);
        DISABLE_PRMPT();
        u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
        if (setjmp(&u.u_qsav)) {
                u.u_qsav = saveq;
                ASSERT(u.u_lwpp->l_notrt != 0);
                u.u_lwpp->l_notrt--;
                ENABLE_PRMPT();
                kunbind(oldengp);
                return EINTR;
        }


	savep = u.u_cfgp;
	u.u_cfgp = (void *)cp;
	ret = (*open)(cp->bind_idata,channelp,oflags,crp,q);

	u.u_qsav = saveq;
        ASSERT(u.u_lwpp->l_notrt != 0);
        u.u_lwpp->l_notrt--;
        ENABLE_PRMPT();
        kunbind(oldengp);
	u.u_cfgp = savep;

	return ret;
}

STATIC int
ddi_bind_close(void *idata, channel_t channel, int oflags, cred_t *crp, queue_t *q)
{
	cfg_info_t *cp;
	int ret, (*close)();
	engine_t *oldengp;
        label_t saveq;
	void *savep;

	cp = (cfg_info_t *)idata; /* Get the pointer to the cfg info struct */
	close = CFG_ORIG_DRVOPS(cp)->d_close;

        oldengp = kbind(&engine[cp->bind_cpu]);
        DISABLE_PRMPT();
        u.u_lwpp->l_notrt++;

        saveq = u.u_qsav;
        if (setjmp(&u.u_qsav)) {
                u.u_qsav = saveq;
                ASSERT(u.u_lwpp->l_notrt != 0);
                u.u_lwpp->l_notrt--;
                ENABLE_PRMPT();
                kunbind(oldengp);
                return EINTR;
        }


	savep = u.u_cfgp;
	u.u_cfgp = (void *)cp;	

	ret = (*close)(cp->bind_idata,channel,oflags,crp,q);

        u.u_qsav = saveq;
        ASSERT(u.u_lwpp->l_notrt != 0);
        u.u_lwpp->l_notrt--;
        ENABLE_PRMPT();
        kunbind(oldengp);
	u.u_cfgp = savep;

	return ret;
}

STATIC int
ddi_bind_devinfo(void *idata, channel_t channel, di_parm_t parm, void *valp)
{
	cfg_info_t *cp;
	int ret, (*devinfo)();
	engine_t *oldengp;
        label_t saveq;
	void *savep;

	cp = (cfg_info_t *)idata; /* Get the pointer to the cfg info struct */
	devinfo = CFG_ORIG_DRVOPS(cp)->d_devinfo;

        oldengp = kbind(&engine[cp->bind_cpu]);
        DISABLE_PRMPT();
        u.u_lwpp->l_notrt++;

        saveq = u.u_qsav;
        if (setjmp(&u.u_qsav)) {
                u.u_qsav = saveq;
                ASSERT(u.u_lwpp->l_notrt != 0);
                u.u_lwpp->l_notrt--;
                ENABLE_PRMPT();
                kunbind(oldengp);
                return EINTR;
        }

	savep = u.u_cfgp;
	u.u_cfgp = (void *)cp;	
	ret = (*devinfo)(cp->bind_idata,channel,parm,valp);

	u.u_qsav = saveq;
        ASSERT(u.u_lwpp->l_notrt != 0);
        u.u_lwpp->l_notrt--;
        ENABLE_PRMPT();
        kunbind(oldengp);
	u.u_cfgp = savep;

	return ret;
}

STATIC void
ddi_bind_biostart(void *idata, channel_t channel, buf_t *bp)
{
	cfg_info_t *cp;
	void (*biostart)();
	engine_t *oldengp;
        label_t saveq;
	void *savep;

	cp = (cfg_info_t *)idata; /* Get the pointer to the cfg info struct */
	biostart = CFG_ORIG_DRVOPS(cp)->d_biostart;

        oldengp = kbind(&engine[cp->bind_cpu]);
        DISABLE_PRMPT();
        u.u_lwpp->l_notrt++;

        saveq = u.u_qsav;
        if (setjmp(&u.u_qsav)) {
                u.u_qsav = saveq;
                ASSERT(u.u_lwpp->l_notrt != 0);
                u.u_lwpp->l_notrt--;
                ENABLE_PRMPT();
                kunbind(oldengp);
        }


	savep = u.u_cfgp;
	u.u_cfgp = (void *)cp;

	(*biostart)(cp->bind_idata,channel,bp);

	u.u_qsav = saveq;
        ASSERT(u.u_lwpp->l_notrt != 0);
        u.u_lwpp->l_notrt--;
        ENABLE_PRMPT();
        kunbind(oldengp);
	u.u_cfgp = savep;
}

STATIC int
ddi_bind_ioctl(void *idata, channel_t channel, int cmd, void *arg, int oflags, cred_t *crp, int *rvalp)
{
	cfg_info_t *cp;
	int ret, (*ioctl)();
	engine_t *oldengp;
        label_t saveq;
	void *savep;

	cp = (cfg_info_t *)idata; /* Get the pointer to the cfg info struct */
	ioctl = CFG_ORIG_DRVOPS(cp)->d_ioctl;

        oldengp = kbind(&engine[cp->bind_cpu]);
        DISABLE_PRMPT();
        u.u_lwpp->l_notrt++;

        saveq = u.u_qsav;
        if (setjmp(&u.u_qsav)) {
                u.u_qsav = saveq;
                ASSERT(u.u_lwpp->l_notrt != 0);
                u.u_lwpp->l_notrt--;
                ENABLE_PRMPT();
                kunbind(oldengp);
                return EINTR;
        }


	savep = u.u_cfgp;
	u.u_cfgp = (void *) cp;

	ret = (*ioctl)(cp->bind_idata,channel,cmd,arg,oflags,crp,rvalp);

	u.u_qsav = saveq;
        ASSERT(u.u_lwpp->l_notrt != 0);
        u.u_lwpp->l_notrt--;
        ENABLE_PRMPT();
        kunbind(oldengp);
	u.u_cfgp = savep;

	return ret;
}

STATIC int
ddi_bind_drvctl(void *idata, channel_t channel, int arg, void *rval)
{
	cfg_info_t *cp;
	int ret, (*drvctl)();
	engine_t *oldengp;
        label_t saveq;
	void *savep;

	cp = (cfg_info_t *)idata; /* Get the pointer to the cfg info struct */
	drvctl = CFG_ORIG_DRVOPS(cp)->d_drvctl;

        oldengp = kbind(&engine[cp->bind_cpu]);
        DISABLE_PRMPT();
        u.u_lwpp->l_notrt++;

        saveq = u.u_qsav;
        if (setjmp(&u.u_qsav)) {
                u.u_qsav = saveq;
                ASSERT(u.u_lwpp->l_notrt != 0);
                u.u_lwpp->l_notrt--;
                ENABLE_PRMPT();
                kunbind(oldengp);
                return EINTR;
        }

	savep = u.u_cfgp;
	u.u_cfgp = (void *)cp;	
	ret = (*drvctl)(cp->bind_idata,channel,arg,rval);

	u.u_qsav = saveq;
        ASSERT(u.u_lwpp->l_notrt != 0);
        u.u_lwpp->l_notrt--;
        ENABLE_PRMPT();
        kunbind(oldengp);
	u.u_cfgp = savep;

	return ret;
}

STATIC ppid_t
ddi_bind_mmap(void *idata, channel_t channel, size_t offset, int prot)
{
	cfg_info_t *cp;
	ppid_t (*mmap)();
	ppid_t ret;
	engine_t *oldengp;
        label_t saveq;
	void *savep;

	cp = (cfg_info_t *)idata; /* Get the pointer to the cfg info struct */
	mmap = CFG_ORIG_DRVOPS(cp)->d_mmap;

        oldengp = kbind(&engine[cp->bind_cpu]);
        DISABLE_PRMPT();
        u.u_lwpp->l_notrt++;

        saveq = u.u_qsav;
        if (setjmp(&u.u_qsav)) {
                u.u_qsav = saveq;
                ASSERT(u.u_lwpp->l_notrt != 0);
                u.u_lwpp->l_notrt--;
                ENABLE_PRMPT();
                kunbind(oldengp);
                return EINTR;
        }

	savep = u.u_cfgp;
	u.u_cfgp = (void *)cp;	
	ret = (*mmap)(cp->bind_idata,channel,offset,prot);

	u.u_qsav = saveq;
        ASSERT(u.u_lwpp->l_notrt != 0);
        u.u_lwpp->l_notrt--;
        ENABLE_PRMPT();
        kunbind(oldengp);
	u.u_cfgp = savep;

	return ret;
}

STATIC int
ddi_bind_open_v7(void *idata, channel_t *channelp, int oflags, cred_t *crp, queue_t *q)
{
	cfg_info_t *cp = get_preddi8_cfg(idata);
	int ret, (*open)();
	engine_t *oldengp;
	label_t saveq;
	void *savep;

	open = CFG_ORIG_DRVOPS(cp)->d_open;

        oldengp = kbind(&engine[cp->bind_cpu]);
        DISABLE_PRMPT();
        u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
        if (setjmp(&u.u_qsav)) {
                u.u_qsav = saveq;
                ASSERT(u.u_lwpp->l_notrt != 0);
                u.u_lwpp->l_notrt--;
                ENABLE_PRMPT();
                kunbind(oldengp);
                return EINTR;
        }


	savep = u.u_cfgp;
	u.u_cfgp = (void *)cp;
	ret = (*open)(idata,channelp,oflags,crp,q);

	u.u_qsav = saveq;
        ASSERT(u.u_lwpp->l_notrt != 0);
        u.u_lwpp->l_notrt--;
        ENABLE_PRMPT();
        kunbind(oldengp);
	u.u_cfgp = savep;

	return ret;
}

STATIC int
ddi_bind_poll_v7(dev_t dev, int events, int anyyet, short *reventsp,
			struct pollhead **phpp)
{
	cfg_info_t *cp = find_cfg_pointer(dev);
	engine_t *oldengp;
	label_t saveq;
	void *savep;
	int ret;


        oldengp = kbind(&engine[cp->bind_cpu]);
        DISABLE_PRMPT();
        u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
        if (setjmp(&u.u_qsav)) {
                u.u_qsav = saveq;
                ASSERT(u.u_lwpp->l_notrt != 0);
                u.u_lwpp->l_notrt--;
                ENABLE_PRMPT();
                kunbind(oldengp);
                return EINTR;
        }


	savep = u.u_cfgp;
	u.u_cfgp = (void *)cp;
	ret = (*cp->dlist_p->d_bind_poll)
		(dev, events, anyyet, reventsp, phpp);

	u.u_qsav = saveq;
        ASSERT(u.u_lwpp->l_notrt != 0);
        u.u_lwpp->l_notrt--;
        ENABLE_PRMPT();
        kunbind(oldengp);
	u.u_cfgp = savep;

	return ret;
}

STATIC int
ddi_bind_close_v7(void *idata, channel_t channel, int oflags, cred_t *crp, queue_t *q)
{
	cfg_info_t *cp = get_preddi8_cfg(idata);
	int ret, (*close)();
	engine_t *oldengp;
        label_t saveq;
	void *savep;

	close = CFG_ORIG_DRVOPS(cp)->d_close;

        oldengp = kbind(&engine[cp->bind_cpu]);
        DISABLE_PRMPT();
        u.u_lwpp->l_notrt++;

        saveq = u.u_qsav;
        if (setjmp(&u.u_qsav)) {
                u.u_qsav = saveq;
                ASSERT(u.u_lwpp->l_notrt != 0);
                u.u_lwpp->l_notrt--;
                ENABLE_PRMPT();
                kunbind(oldengp);
                return EINTR;
        }


	savep = u.u_cfgp;
	u.u_cfgp = (void *)cp;	

	ret = (*close)(idata,channel,oflags,crp,q);

        u.u_qsav = saveq;
        ASSERT(u.u_lwpp->l_notrt != 0);
        u.u_lwpp->l_notrt--;
        ENABLE_PRMPT();
        kunbind(oldengp);

	u.u_cfgp = savep;
	return ret;
}

STATIC int
ddi_bind_devinfo_v7(void *idata, channel_t channel, di_parm_t parm, void *valp)
{
	cfg_info_t *cp = get_preddi8_cfg(idata);
	int ret, (*devinfo)();
	engine_t *oldengp;
        label_t saveq;
	void *savep;

	devinfo = CFG_ORIG_DRVOPS(cp)->d_devinfo;

        oldengp = kbind(&engine[cp->bind_cpu]);
        DISABLE_PRMPT();
        u.u_lwpp->l_notrt++;

        saveq = u.u_qsav;
        if (setjmp(&u.u_qsav)) {
                u.u_qsav = saveq;
                ASSERT(u.u_lwpp->l_notrt != 0);
                u.u_lwpp->l_notrt--;
                ENABLE_PRMPT();
                kunbind(oldengp);
                return EINTR;
        }

	savep = u.u_cfgp;
	u.u_cfgp = (void *)cp;	
	ret = (*devinfo)(idata,channel,parm,valp);

	u.u_qsav = saveq;
        ASSERT(u.u_lwpp->l_notrt != 0);
        u.u_lwpp->l_notrt--;
        ENABLE_PRMPT();
        kunbind(oldengp);
	u.u_cfgp = savep;

	return ret;
}

STATIC void
ddi_bind_biostart_v7(void *idata, channel_t channel, buf_t *bp)
{
	cfg_info_t *cp = get_preddi8_cfg(idata);
	void (*biostart)();
	engine_t *oldengp;
        label_t saveq;
	void *savep;

	biostart = CFG_ORIG_DRVOPS(cp)->d_biostart;

        oldengp = kbind(&engine[cp->bind_cpu]);
        DISABLE_PRMPT();
        u.u_lwpp->l_notrt++;

        saveq = u.u_qsav;
        if (setjmp(&u.u_qsav)) {
                u.u_qsav = saveq;
                ASSERT(u.u_lwpp->l_notrt != 0);
                u.u_lwpp->l_notrt--;
                ENABLE_PRMPT();
                kunbind(oldengp);
        }


	savep = u.u_cfgp;
	u.u_cfgp = (void *)cp;

	(*biostart)(idata,channel,bp);

	u.u_qsav = saveq;
        ASSERT(u.u_lwpp->l_notrt != 0);
        u.u_lwpp->l_notrt--;
        ENABLE_PRMPT();
        kunbind(oldengp);
	u.u_cfgp = savep;
}

STATIC int
ddi_bind_ioctl_v7(void *idata, channel_t channel, int cmd, void *arg, int oflags, cred_t *crp, int *rvalp)
{
	cfg_info_t *cp = get_preddi8_cfg(idata);
	int ret, (*ioctl)();
	engine_t *oldengp;
        label_t saveq;
	void *savep;

	ioctl = CFG_ORIG_DRVOPS(cp)->d_ioctl;

        oldengp = kbind(&engine[cp->bind_cpu]);
        DISABLE_PRMPT();
        u.u_lwpp->l_notrt++;

        saveq = u.u_qsav;
        if (setjmp(&u.u_qsav)) {
                u.u_qsav = saveq;
                ASSERT(u.u_lwpp->l_notrt != 0);
                u.u_lwpp->l_notrt--;
                ENABLE_PRMPT();
                kunbind(oldengp);
                return EINTR;
        }


	savep = u.u_cfgp;
	u.u_cfgp = (void *) cp;

	ret = (*ioctl)(idata,channel,cmd,arg,oflags,crp,rvalp);

	u.u_qsav = saveq;
        ASSERT(u.u_lwpp->l_notrt != 0);
        u.u_lwpp->l_notrt--;
        ENABLE_PRMPT();
        kunbind(oldengp);
	u.u_cfgp = savep;

	return ret;
}

STATIC ppid_t
ddi_bind_mmap_v7(void *idata, channel_t channel, size_t offset, int prot)
{
	cfg_info_t *cp = (cfg_info_t *)idata;
	ppid_t (*mmap)();
	ppid_t ret;
	engine_t *oldengp;
        label_t saveq;
	void *savep;

	mmap = CFG_ORIG_DRVOPS(cp)->d_mmap;

        oldengp = kbind(&engine[cp->bind_cpu]);
        DISABLE_PRMPT();
        u.u_lwpp->l_notrt++;

        saveq = u.u_qsav;
        if (setjmp(&u.u_qsav)) {
                u.u_qsav = saveq;
                ASSERT(u.u_lwpp->l_notrt != 0);
                u.u_lwpp->l_notrt--;
                ENABLE_PRMPT();
                kunbind(oldengp);
                return EINTR;
        }

	savep = u.u_cfgp;
	u.u_cfgp = (void *)cp;	
	ret = (*mmap)(cp->shimp,channel,offset,prot);

	u.u_qsav = saveq;
        ASSERT(u.u_lwpp->l_notrt != 0);
        u.u_lwpp->l_notrt--;
        ENABLE_PRMPT();
        kunbind(oldengp);
	u.u_cfgp = savep;

	return ret;
}


/*
 * Return the driver's set of CONFIG capabilities
 *
 * Returns 0 if successfull and ENODEV if the driver isn't
 * loaded
 */
int
ddi_getdrv_cap(char *name, uint_t *drvcap)
{
	drvlist_t *dlistp;

	dlistp = drv_lookup(name);

	if (dlistp == NULL)
		return ENODEV;

	*drvcap = dlistp->info_p->drv_flags;
	return 0;

	
}


/*
 * Wait for all the PENDING CFG_ADD calls (state == READY_TO_ADD)
 * to complete.
 */
boolean_t
mod_wait_for_adds_to_complete()
{
	boolean_t inter;

	LOCK_CFGLIST();
	while(!mod_daemon_idle) {
		inter = SV_WAIT_SIG(&mod_daemon_q_sv,PRIMED, cfg_list_lp);
		if (!inter)
			return inter;
		LOCK_CFGLIST();
	}
	UNLOCK_CFGLIST();
	return B_TRUE;
}
