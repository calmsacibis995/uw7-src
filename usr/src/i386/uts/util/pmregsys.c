#ident  "@(#)kern-i386:util/pmregsys.c	1.1.3.2"
#ident  "$Header$"

/*
 * (C) Copyright 1991, 994, 1996, 1997 The Santa Cruz Operation, Inc.
 * All rights reserved.
 *
 * !!! SCO CONFIDENTIAL !!!
 *
 * This module contains confidential trade secrets of The Santa Cruz
 * Operation, Inc.  Unlicensed disclosure, use or distribution is
 * strictly prohibited.
 */

#include <util/types.h>
#include <util/param.h>
#include <util/debug.h>
#include <acc/priv/privilege.h>
#include <acc/priv/priv_hier.h>
#include <acc/priv/lpm/lpm.h>
#include <mem/kmem.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/ipl.h>
#include <svc/systm.h>
#include <svc/limitctl.h>
#include <util/pmregsys.h>
/*

 * Things that need doing/thinking about:
 *
 * 1. Are there any DDI issues, either with this module or drivers that
 *    have to call it?
 * 2. Implement REPORT/ADD/DEL
 * 3. Is there an equivalent of mp_restrict() for non MP safe drivers?
 * 4. LOCK_ALLOC has to be called by somebody for pmd_lock and callback_lock.
 */

/*
 * This module maintains two structures. An array of ID's that have been
 * supplied by the Policy Manager Deamon (PMD), indicating what kernel
 * features have been licensed, and a linked list of callbacks that have
 * been supplied by drivers who wish to be made aware of their licensing
 * state.
 *
 * Each product ID can have a counter associated with it. The PMD will
 * call this module to increment and decrement the counters. This allows
 * the PMD to accurately recover from a re-start, without losing track of
 * user counts. This only applies to user count fields, not to CPU counts.
 * (I.E the u-bit in the license data field).
 *
 * Flow of control
 * ---------------
 *
 * When the PMD first starts up, it will call this function with an
 * INITIALIZE command, passing a list of licenses that it knows about.
 * This sets up the first table of product ID's.
 *
 * By special dispensation, the PMD also registers information for the
 * uname() system call by calling pmd_set_uname(). This information can
 * then be queried by the rest of the kernel, but most notably by uname().
 *
 * As part of the INITIALIZE call, pmregsys() will examine the existing
 * tables, and will keep track of existing user counts. This call will then
 * replace the user count field in each element in the array where there
 * is a match, so that the PMD can determine the number of already used
 * licenses.
 *
 * During normal operation, the PMD will call pmregister() with various
 * PMD_INC_COUNT and PMD_DEC_COUNT calls to increment and decrement the
 * user counts for those products which have user count bits.
 */

#define PMD_HIER_BASE   200	/* For now */

/* An array used to hold kernel licenses. */
STATIC struct component_license *pmd_licenses;
STATIC int pmd_array_sz = 0;    /* Number of entries allocated.         */
STATIC int pminit = 0;          /* Has pmregister been called?          */
STATIC int pmd_valid = 0;       /* Has pmregister been called?          */
STATIC lock_t *pmd_lock;        /* lock structure for license array.    */
STATIC lock_t *pmd_callback_lock; /* Callback list lock.                */
STATIC callback *callback_list = NULL; /* Callback list. */
STATIC int pmd_policy = 1;
STATIC int pmd_regstate = 0;

extern char *os_hw_serial;
char pmd_serial_no[PMD_SERIAL_MAX];

STATIC LKINFO_DECL (pmd_lock_info, "util:pmd: pmd_lock", 0);
STATIC LKINFO_DECL (pmd_callback_lock_info, "util:pmd: pmd_callback_lock_info", 0);


/*
 * Function prototypes
 */
STATIC void	pmd_find_driver (int id, int *licensed, int *data);
STATIC void 	pmd_update_callback (int id, void (*f) (int, int, int), int
				 flags);
STATIC void 	pmd_call_callback (void (*f) (int, int, int), int product_id, 
			       int flags, int licensed, int data);
STATIC void 	pmd_check_single_callback (int id);
STATIC void 	pmd_check_all_callbacks (void);
STATIC struct 	component_license *pmd_grab_mem (int n);
STATIC void 	pmd_merge_license (struct component_license *license);
STATIC int 	pmd_validate_caller (char *buffer, int length,
				     unsigned int supplied_sum);
STATIC int 	pmd_kern_info (struct sysreg *uap, unsigned long s);
STATIC int 	pmd_initialize (int cmd, struct prodreg *uap, unsigned long s);
STATIC int 	pmd_get_count (struct prodchk *uap, unsigned long s);
STATIC int 	pmd_set_count (struct prodmod *uap, int incdec,
			       unsigned long s);
STATIC int 	pmd_os_ival (struct pmd_ival *uap, int cmd, int *ival);
int 		pmregister (struct pmregargs *args, rval_t *rvp);
int 		pmd_driver_licensed (int product_id, int *data,
			 void (*f) (int, int, int), int flags);

int	license_init(struct component_license **comps);

/*
 * void
 * pmd_init(void)
 *  Initialize pmd_lock and pmd_callback_lock.
 *
 * Note:
 *  Called in sysinit().
 *
 */
void
pmd_init (void)
{
	struct prodreg kap;
	struct component_license *comps;
	int ncomp;
	
	pmd_callback_lock = LOCK_ALLOC (PMD_HIER_BASE, plbase,
					&pmd_callback_lock_info, KM_NOSLEEP);

	pmd_lock = LOCK_ALLOC (PMD_HIER_BASE + 1, plbase,
			       &pmd_lock_info, KM_NOSLEEP); 

	if (pmd_lock == NULL || pmd_callback_lock == NULL)
		cmn_err (CE_PANIC, "pmd_init: cannot allocate lock");

#ifndef MINI_KERNEL	
	/*
	 * Initialize the licensing data for kernel
	 */
	if ((ncomp = license_init(&comps))) {	/* if any components */
		kap.p = comps;
		kap.nelem = ncomp;

		ASSERT(ncomp > 0);
	
		pmd_initialize(PMD_KERN_INIT, &kap, 0); 
	}
#endif	
}

/*
 * void
 * pmd_find_driver()
 *
 * Loop through the array of drivers that we have licensing information
 * for, looking for a particular ID.
 *
 * If we find this driver in the array, pass back up the licensing information.
 *
 * If the driver is licensed data will hold the activation state of the driver.
 * Otherwise data holds a policy for the unlicensed driver.
 *
 * If we have no licensing information for this driver, we must assume here
 * that it is not licensed, and has a 0 policy.
 */
STATIC void
pmd_find_driver (int id, int *licensed, int *data)
{
	int i;
	pl_t s;

	s = LOCK (pmd_lock, plbase);

	/* Search for this product. */
	for (i = 0; i < pmd_valid; i++)
		if (id == pmd_licenses[i].product_id) {
			/*
			 * Found it. If it is licensed, return the activation
			 * state in data, else return the policy.
			 */
			*data = pmd_licenses[i].licensed ?
				pmd_licenses[i].activation_state :
				pmd_licenses[i].policy;
			*licensed = pmd_licenses[i].licensed;
			UNLOCK (pmd_lock, s);
			return;
		}
	UNLOCK (pmd_lock, s);

	/*
	 * Can't find any information about driver, therefore assume it
	 * is not licensed and has no policy.
	 */

	*licensed = 0;
	*data = 0;
	return;
}

/* pmd_update_callback()
 *
 * This function is passed a driver id and a pointer to the drivers callback
 * function.
 *
 * We move alone the sorted linked list of callbacks that were registered to
 * see if we are modifying an existing callback. If not, we allocate memory
 * for another element and slot it into the correct place in the list.
 */
STATIC void
pmd_update_callback (int id, void (*f) (int, int, int), int flags)
{
	callback *p1, *p2, *tmp;
	pl_t s;

	/* Get some memory now, in case this is a new element. */
	tmp = kmem_alloc (sizeof (callback), KM_SLEEP);

	s = LOCK (pmd_callback_lock, plbase);

	p1 = callback_list;
	p2 = p1;

	/* Move through the list, either looking for this entry, or
	 * a place to stick it. p2 follows 1 element behind p1.
	 */
	while ((p1 != (callback *) NULL) && (p1->product_id <= id)) {
		if (p1->product_id == id) {
			/* Found it, update the function and flags,
			 * unlock the list, free the memory, and exit.
			 */
			p1->func = f;
			p1->flags = flags;
			UNLOCK (pmd_callback_lock, s);
			kmem_free (tmp, sizeof (callback));
			return;
		}
		p2 = p1;
		p1 = p1->next;
	}

#if 0	
	/*
	 * This is a new driver being registered. If this is a NULL callback,
	 * then simply free the tmp memory and return. We don't want this in
	 * our list.
	 */
	if (f == (void (*)(int, int, int)) NULL) {
		UNLOCK (pmd_callback_lock, s);
		kmem_free (tmp, sizeof (callback));
		return;
	}
#endif	

	/*
	 * If we are here, then we didn't find it in our array, but p2
	 * is pointing to where the new entry should be slotted in.
	 * Fill in the new memory with our data.
	 */
	tmp->product_id = id;
	tmp->flags = flags;
	tmp->func = f;
	tmp->next = p1;

	/* Are we still at the start of the list? */
	if (p2 == p1) {
		callback_list = tmp;
		UNLOCK (pmd_callback_lock, s);
		return;
	}

	/* Slot the new element in the sorted list. */
	tmp->next = p1;
	p2->next = tmp;

	UNLOCK (pmd_callback_lock, s);
	return;
}

/*
 * Call a callback. If the driver is not marked as MP safe, then restrict 
 * the function to the base processor.
 */
STATIC void
pmd_call_callback (void (*f) (int, int, int), int product_id,
                   int flags, int licensed, int data)
{
	int mp_flag = 0;

	/*
	 * If the driver is not MP safe, restrict this function to the
	 * base processor.
	 *
	 * GEMINI: How do we do this?
	 */
#if 0
	if ((flags & MP_SAFE) == 0)
		mp_flag = mp_restrict ();
#endif

#ifndef MINI_KERNEL
	(f) (product_id, licensed, data);
#endif

#if 0
	if ((flags & MP_SAFE) == 0)
		mp_unrestrict (mp_flag);
#endif
	return;
}

/*
 * Scan our callback list for this specific ID. If we find it, and there
 * is a callback function to call, then get the licensing details for the
 * ID and pass these on to the driver's callback function. Note that it is
 * impossible for us to enter this function until pminit has been set
 * indicating that some licensing information is now to the kernel.
 */
STATIC void
pmd_check_single_callback (int id)
{
	int s;
	callback *p1;

	ASSERT (pminit);
	s = LOCK (pmd_callback_lock, plbase);
	p1 = callback_list;

	while ((p1 != (callback *) NULL) && (p1->product_id <= id)) {
		if (p1->product_id == id) {
			/* Found the product. If the driver has a callback,
			 * then find the licensing details and call the
			 * driver supplied function.
			 */
			if (p1->func != (void (*)(int, int, int)) NULL) {
				/* L001 Start */
				void (*f) (int, int, int);
				int flags, licensed, data;

				f = p1->func;
				flags = p1->flags;

				pmd_find_driver (id, &licensed, &data);

				UNLOCK (pmd_callback_lock, s);
				pmd_call_callback (f, id, flags,
						   licensed, data); 
				return;
			}
			else {
			
				/* Unlock the list and return. */
				UNLOCK (pmd_callback_lock, s);
				return;
			}
		}
		p1 = p1->next;
	}
	UNLOCK (pmd_callback_lock, s);
	return;
}


/*
 * After an INITIALIZE pmregister command, we have to get all the licensing
 * information for all the drivers who have registered a callback.
 * The lock on the callback list is dropped while the callback is executed.
 * This only works because the linked list never shrinks or moves, but only
 * grows.
 */
STATIC void
pmd_check_all_callbacks (void)
{
	pl_t s;
	callback *p1;

	ASSERT (pminit);
	s = LOCK (pmd_callback_lock, plbase);
	p1 = callback_list;

	while (p1 != (callback *) NULL) {
		/* If this driver has a callback, then call it. */
		if (p1->func != (void (*)(int, int, int)) NULL) {
			void (*f) (int, int, int);
			int id,	flags, licensed, data;

			id = p1->product_id;
			f = p1->func;
			flags = p1->flags;

			pmd_find_driver (id, &licensed, &data);

			UNLOCK (pmd_callback_lock, s);
			pmd_call_callback (f, id, flags, licensed, data);
			s = LOCK (pmd_callback_lock, plbase);
		}
		p1 = p1->next;
	}
	UNLOCK (pmd_callback_lock, s);
	return;
}

#ifdef DEBUG
void
pmd_dump_licenses (void)
{
	int i;
	pl_t s;

	s = LOCK (pmd_lock, plbase);
	for (i = 0; i < pmd_valid; i++) {
		printf ("Product %d ", pmd_licenses[i].product_id);
		printf ("%s licensed. ", pmd_licenses[i].licensed ?
			"is" : "is not");
		if (pmd_licenses[i].licensed)
			printf ("Activation state = %d\n",
				pmd_licenses[i].activation_state);
		else
			printf ("Policy = %d\n",
				pmd_licenses[i].policy);
	}
	UNLOCK (pmd_lock, s);
}

#endif

/* Create a license array capable of holding n elements. */
STATIC struct component_license *
pmd_grab_mem (int n)
{
	int size;
	struct component_license *p;

	size = n * sizeof (struct component_license);
	p = (struct component_license *) kmem_alloc (size, KM_SLEEP);

	return (p);
}

STATIC void
pmd_merge_license (struct component_license *license)
{
	int i, amount_to_free;
	int needed = 0;
	pl_t s;
	struct component_license *tmp, *p = (struct component_license *) NULL;

#ifdef DEBUG
	cmn_err (CE_NOTE, "merge of ID %d", license->product_id);
#endif
	s = LOCK (pmd_lock, plbase);
	for (i = 0; i < pmd_valid; i++) {
		if (pmd_licenses[i].product_id == license->product_id) {
			pmd_licenses[i] = *license;
			UNLOCK (pmd_lock, s);
			return;
		}
	}

	if (pmd_valid < pmd_array_sz) {
		pmd_licenses[pmd_valid++] = *license;
		UNLOCK (pmd_lock, s);
		return;
	}

	ASSERT (pmd_valid == pmd_array_sz);
	/* There is no room in the current array. We must create a new one
	 * with some (16) extra elements.
	 */
	while (needed < (pmd_array_sz + 16)) {
		needed = pmd_array_sz + 16;
		if (p)
			kmem_free (p, needed *
				   sizeof (struct component_license));
		p = pmd_grab_mem (needed);
	}

	/*
	 * Here we are certain to have enough extra memory for 16 more
	 * element in our array.
	 */

	/* Copy in the old members. */
	bcopy (pmd_licenses, p, pmd_array_sz *
	       sizeof (struct component_license));

	amount_to_free = pmd_array_sz * sizeof (struct component_license);
	tmp = pmd_licenses;

	/* Correct the array pointer and size. */
	pmd_licenses = p;
	pmd_array_sz += 16;

	/* If we have anything to free, free it here. */
	if (amount_to_free)
		kmem_free (tmp, amount_to_free);

	/* Rescan the array to check that somebody else has not put
	 * our entry in it while we may have been sleeping.
	 */
	for (i = 0; i < pmd_valid; i++)
		if (pmd_licenses[i].product_id == license->product_id) {
			pmd_licenses[i] = *license;
			UNLOCK (pmd_lock, s);
			return;
		}

	/* Not found still, so add it here. */
	pmd_licenses[pmd_valid++] = *license;
	UNLOCK (pmd_lock, s);
	return;
}

STATIC int
pmd_validate_caller (char *buffer, int length, unsigned int supplied_sum)
{
	register unsigned long sum = 0xfe4ba237, nsum;
	register char *c = buffer;

	while (length--) {
		nsum = (sum << 31) + (sum >> 1) + *c;
		sum = nsum;
		c++;
	}

	if (sum != supplied_sum) {
		return 0;
	}
	return (1);
}

STATIC int
pmd_kern_info (struct sysreg *uap, unsigned long s)
{
	long array_size = (long)sizeof(struct sysreg);
	int i;

	/* Validate the caller. */
	if (!pmd_validate_caller ((char *) uap, array_size, s))
		return EINVAL;

	/* Set user and CPU limits */
	if (uap->users >= 1)
		limit(L_SETUSERLIMIT, &uap->users);
	if (uap->cpus >= 1)
		limit(L_SETPROCLIMIT, &uap->cpus);
	
	for (i = 0; i < PMD_SERIAL_MAX; i++) {
		pmd_serial_no[i] = '\0';
		if (uap->serial[i] != '\0')
			pmd_serial_no[i] = uap->serial[i];
	}
	if (pmd_serial_no[0] == '\0') {
		pmd_serial_no[0] = 'N';
		pmd_serial_no[1] = 'U';
		pmd_serial_no[2] = 'L';
		pmd_serial_no[3] =
			pmd_serial_no[4] = pmd_serial_no[5] =
			pmd_serial_no[6] = pmd_serial_no[7] =
			pmd_serial_no[8] = '0';
		pmd_serial_no[9] = '\0';
	}

	/*
	 * If not set by the PSM, then set pmd_serial_no.
	 */
	if (strcmp(os_hw_serial, "no-serial") == 0)
		os_hw_serial = pmd_serial_no;

	return 0;
}

/*
 * NOTE:
 * 	The PMD_INITIALIZE can be used more than once because PMD can be
 * 	killed and thus it is stateless.
 */
STATIC int
pmd_initialize (int cmd, struct prodreg *uap, unsigned long s)
{
	struct component_license *local, *tmp;
	int amount_to_free,  array_size;
	pl_t pl;

	array_size = uap->nelem * sizeof (struct component_license);
	local = pmd_grab_mem (uap->nelem);

	if (cmd == PMD_KERN_INIT) {
		bcopy (uap->p, local, array_size);
	} else {
		if (copyin (uap->p, local, array_size) == -1) {
			kmem_free (local, array_size);
			return EFAULT;
		}

		/* Validate the caller. */
		if (!pmd_validate_caller ((char *) local, array_size, s)) {
			return EINVAL;
		}	
	}	
	
	switch (cmd) {
	case PMD_KERN_INIT:	
	case PMD_INITIALIZE:
		amount_to_free = pmd_array_sz *
			sizeof (struct component_license);
		pl = LOCK (pmd_lock, plbase);
		tmp = pmd_licenses;
		pmd_licenses = local;
		pmd_valid = uap->nelem;
		pmd_array_sz = uap->nelem;
		UNLOCK (pmd_lock, pl);

		/*
		 * If called before, free the previous ones
		 */
		if (amount_to_free)
			kmem_free (tmp, amount_to_free);
		pminit = 1;
		pmd_check_all_callbacks ();
		break;
	case PMD_ADD_LICENSE:
		tmp = local;
		while (uap->nelem--) {
			pmd_merge_license (tmp);
			pmd_check_single_callback (tmp->product_id);
			tmp++;
		}
		kmem_free (local, array_size);
		break;

	default:
		return EINVAL;
	}
	return 0;
}

/* NOT IMPLEMENTED YET */
STATIC int
pmd_get_count (struct prodchk *uap, unsigned long s)
{
	return 0;
}

/* NOT IMPLEMENTED YET */
STATIC int
pmd_set_count (struct prodmod *uap, int incdec, unsigned long s)
{
	return 0;
}

int
pmregister (struct pmregargs *args, rval_t *rvp)
{
	int scr;
	struct pmreg *uap = args->regs;

	if (uap->cmd != PMD_GET_OS_POLICY &&
	    uap->cmd != PMD_GET_OS_REGSTATE) 
		if (pm_denied (CRED (), P_SYSOPS))
			return EACCES;

	/* Check we understand the version number
	 * (Post OpenServer versions start at 100)
	 */

	if (uap->version < 100)
		return EINVAL;

	/*
	 * Even if we have not yet initialized, we can still allow the caller
	 * to set the OS serial number, user count and CPU/CG count.
	 */
	if (uap->cmd == PMD_KERN_INFO)
		return (pmd_kern_info (&uap->pcmd.sr, uap->s));

	/* Force INITIALIZE to be called first. */
	if (!pminit && uap->cmd != PMD_INITIALIZE)
		return EINVAL;

	/*
	 * For future expansion, call individual functions for each type of
	 * request. Let the request handle the data the way it needs to.
	 */

	switch (uap->cmd) {
	case PMD_INITIALIZE:
	case PMD_ADD_LICENSE:
		scr = pmd_initialize (uap->cmd, &uap->pcmd.pr, uap->s);
		break;

	case PMD_GET_COUNT:
		scr = pmd_get_count (&uap->pcmd.pc, uap->s);
		break;

	case PMD_INC_COUNT:
		scr = pmd_set_count (&uap->pcmd.pm, 1, uap->s);
		break;

	case PMD_DEC_COUNT:
		scr = pmd_set_count (&uap->pcmd.pm, -1, uap->s);
		break;

	case PMD_SET_OS_POLICY:
		scr = pmd_os_ival (&uap->pcmd.iv, PMD_SET_IVAL, &pmd_policy);
		break;

	case PMD_GET_OS_POLICY:
		scr = pmd_os_ival (&uap->pcmd.iv, PMD_GET_IVAL, &pmd_policy);
		break;

	case PMD_SET_OS_REGSTATE:
		scr = pmd_os_ival (&uap->pcmd.iv, PMD_SET_IVAL, &pmd_regstate);
		break;

	case PMD_GET_OS_REGSTATE:
		scr = pmd_os_ival (&uap->pcmd.iv, PMD_GET_IVAL, &pmd_regstate);
		break;

	default:
		scr = EINVAL;
	}

	return scr;
}

/* pmd_driver_licensed() - Routine that is to be used by drivers to determine
 * if they are licensed to run.
 * RETURNS:
 *  0  - This driver is not licensed. data will hold any policy that
 *       the driver should use. Policy is not required to be used, but
 *       any driver wishing to do so define their policy and communicate
 *       this to the group maintaining the PMD.
 *  >0 - This driver is licensed. data will hold an activation state for
 *       the driver. Each driver will decide how it wishes to make use of
 *       this field, and communicate this to the group maintaining the
 *       PMD. (MPX will use the activation state to determine the number
 *       of CPU's that can be used.)
 *  <0 - The INITIALIZE command to pmregister has not been used yet. We
 *       cannot determine if this driver is licensed, but will record it's
 *       callback function (if any) and call it when the system licensing
 *       state changes.
 *
 * NOTE:
 *	The function works differently depending on the flag:
 * 	PMD_DRV_REGISTER: register the callback handler.
 *	PMD_DRV_DEREGISTER: deregister the callback handler.
 *	PMD_DRV_QUERY: query only - no registration required. The kernel
 *		must use this flag, if the driver cannot support a callback
 *		mechanism.
 *	MP_SAFE is equivalent to PMD_DRV_REGISTER.
 */
int
pmd_driver_licensed (int product_id, int *data,
		     void (*f) (int, int, int), int flags)
{
	int licensed;

	if (flags == MP_SAFE)
		flags = PMD_DRV_REGISTER;
	
	switch (flags & ~MP_SAFE) { /* clear MP_SAFE flag */
	case PMD_DRV_REGISTER:
		/* Register the callback function for this driver. */
		pmd_update_callback (product_id, f, flags);
		break;
	case PMD_DRV_DEREGISTER:
		/* Deregister the callback function for this driver. */
		pmd_update_callback (product_id, NULL, flags);
		break;
	case PMD_DRV_QUERY:
		break;
	default:		/* not supported case */
		return (-1);
	}

	/* If pmregister() has not been successfully called, then
	 * return -1 to caller.
	 */
	if (!pminit) {
		*data = 0;
		return (-1);
	}

	pmd_find_driver (product_id, &licensed, data);
	return (licensed);
}

int
pmd_os_ival(struct pmd_ival *pl, int cmd, int *ival)
{
	if (cmd == PMD_SET_IVAL) {
		if (copyin ((caddr_t) pl->ival, (caddr_t) ival,
		   sizeof (int)))
			return EINVAL;
		return 0;
	}

	else if (cmd == PMD_GET_IVAL) {
		if (copyout ((caddr_t) ival, (caddr_t) pl->ival,
			sizeof (int)))
			return EINVAL;
		return 0;
	} else
			return EINVAL;
}

#ifdef DEBUG
#define DEBUG_ID	23451

void
pmd_debug_hanlder(int product_id, int licensed, int data)
{
	cmn_err(CE_CONT, "pmd_debug_hanlder: %d, %d, %d\n",
		product_id, licensed, data);
}

void
pmd_debug(void)
{
	int d, ret;

	ret = pmd_driver_licensed(DEBUG_ID, &d, NULL, PMD_DRV_QUERY);

	printf("pmd_debug: product %d: lisensed = %d, state = %d\n",
	       DEBUG_ID, ret, d);
	
}
#endif

