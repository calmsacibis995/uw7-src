#ident	"@(#)kern-i386:util/mod/mod_str.c	1.12.4.1"
#ident	"$Header$"

#define _DDI_C

#include <io/conf.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <mem/seg_kvn.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <util/mod/mod.h>
#include <util/mod/mod_hier.h>
#include <util/mod/mod_k.h>
#include <util/mod/moddefs.h>

#include <io/ddi.h>

/*
 * Dynamically loadable STREAMS module support.
 */

STATIC int mod_strinstall(struct mod_type_data *, struct module *);
STATIC int mod_strremove(struct mod_type_data *);
STATIC int mod_strinfo(struct mod_type_data *, int *, int *, int *);
STATIC int mod_strbind(struct mod_type_data *, int *);


struct	mod_operations	mod_str_ops	= {
	mod_strinstall,
	mod_strremove,
	mod_strinfo,
	mod_strbind
};

/*
 * int mod_str_reg(void *arg)
 *	Register loadable module for STREAMS drivers
 *
 * Calling/Exit State:
 *	If successful, the routine returns 0, else the appropriate errno.
 *
 * NOTE: obsolete
 */
int
mod_str_reg(void *arg)
{
	return 0;
}

/*
 * STATIC int mod_strinstall(struct mod_type_data *strdatap, 
 *	struct module *modp)
 *
 *	Connect the STREAMS module to the fmodsw table.
 *
 * Calling/Exit State:
 *	The routine will fail if the STREAMS module is not registered.
 *	No locks held upon calling and exit.
 */
STATIC int
mod_strinstall(struct mod_type_data *strdatap, struct module *modp)
{
	struct	fmodsw	*dta;
	struct	fmodsw	*fmodp;
	int	idx;
	pl_t	pl;
	int	error;
	const drvops_t *drvops;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	moddebug(cmn_err(CE_CONT, "!MOD: mod_strinstall()\n"));

	dta = (struct fmodsw *)strdatap->mtd_pdata;
	dta->f_modp = modp;

	/*
	 * install the shim driver.
	 */
	error = mod_str_ddiinstall(strdatap, modp, &drvops);
	if (error) {
		moddebug(cmn_err(CE_CONT, 
			"!MOD: module %s: mod_str_ddiinstall error\n",
			name, error));
		return(error);
	}

	/*
	 * Populate the fmodsw[] entry with the module's information.
	 */
	pl = RW_WRLOCK(&mod_fmodsw_lock, PLDLM);

	ASSERT(findmod_l(dta->f_name) < 0);
	if((idx = findmod_slot_l()) < 0) {
		RW_UNLOCK(&mod_fmodsw_lock, pl);
		moddebug(cmn_err(CE_CONT, 
			"!MOD: module %s: fmodsw full\n", dta->f_name));
		return(ECONFIG);
	}

	moddebug(cmn_err(CE_CONT, "!MOD: f_name = %s, idx = %d\n",
		dta->f_name, idx));

	fmodp = &fmodsw[idx];
	*fmodp = *dta;			/* struct copy */

	RW_UNLOCK(&mod_fmodsw_lock, pl);

	return(0);
}

/*
 * STATIC int mod_strremove(struct mod_type_data *strdatap)
 *	Disconnect the STREAMS module from the fmodsw table.
 *
 * Calling/Exit State:
 *	No locks held upon calling and exit.
 */
STATIC int
mod_strremove(struct mod_type_data *strdatap)
{
	struct	fmodsw	*dta;
	pl_t pl;
	int idx;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	moddebug(cmn_err(CE_CONT, "!MOD: mod_strremove()\n"));

	/*
	 * Saved in mod_strinstall().
	 */
	dta = (struct fmodsw *)strdatap->mtd_pdata;

	moddebug(cmn_err(CE_CONT, 
		"!MOD: f_name = %s, disconnected\n", dta->f_name));

	/*
	 * Reset for next load.
	 */
	pl = RW_WRLOCK(&mod_fmodsw_lock, PLDLM);
	idx = findmod_l(dta->f_name);
	if (idx >= 0)
		bzero(fmodsw + idx, sizeof(fmodsw[0]));
	mod_str_ddiremove(dta->f_name);
	RW_UNLOCK(&mod_fmodsw_lock, pl);

	return(0);
}

/*
 * STATIC int mod_strinfo(struct mod_type_data *strdatap, 
 *			  int *p0, int *p1, int *type)
 *	Return the module type and the index of the module
 *	in fmodsw.
 *
 * Calling/Exit State:
 *	The keepcnt of the STREAMS module is non-zero upon
 *	calling and exit of this routine.
 */
STATIC int
mod_strinfo(struct mod_type_data *strdatap, int *p0, int *p1, int *type)
{
	int idx;

	idx = findmod(((struct fmodsw *)strdatap->mtd_pdata)->f_name);
	if (idx < 0)
		return(EINVAL);
	*type = MOD_TY_STR;
	*p0 = idx;
	*p1 = -1;

	return(0);
}

/*
 * STATIC int mod_strbind(struct mod_type_data *strdatap, int *cpup)
 *	Routine to handle cpu binding for non-MP modules.
 *	The cpu to be bound to is returned in cpup.
 *
 * Calling/Exit State:
 *	Returns 0 on success or appropriate errno if failed.
 */
STATIC int
mod_strbind(struct mod_type_data *strdatap, int *cpup)
{
	struct fmodsw	*dta;

	dta = (struct fmodsw *)strdatap->mtd_pdata;

	/* do nothing if the module is multi threaded */
	if (*dta->f_flag & D_MP)
		return (0);

	if (*cpup > 0) {
		/*
		 *+ STREAMS modules, if not multithreaded, must be bound
		 *+ to cpu 0. If the module is configured to bind with another
		 *+ cpu through other linkages, the load of the module will
		 *+ be failed.
		 */
		cmn_err(CE_WARN,
	"The STREAMS module %s is illegally configured to bind with cpu %d,\n\
the module cannot be loaded.", dta->f_name, *cpup);
		return (ECONFIG);
	}

	/*
	 * Single threaded STREAMS module always bind to cpu 0.
	 */
	*cpup = 0;
	return (0);
}

