#ident	"@(#)kern-i386:util/kdb/scodb/mod_ksym.c	1.1"
#ident  "$Header$"
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <mem/as.h>
#include <mem/faultcatch.h>
#include <mem/faultcode.h>
#include <mem/kmem.h>
#include <mem/seg.h>
#include <mem/seg_kvn.h>
#include <mem/seg_vn.h>
#include <mem/seg_kmem.h>
#include <mem/vmparam.h>
#include <proc/obj/elf.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/mod/ksym.h>
#include <util/mod/mod.h>
#include <util/mod/mod_hier.h>
#include <util/mod/mod_k.h>
#include <util/mod/mod_obj.h>
#include <util/sysmacros.h>
#include <util/types.h>

/* The module id for kernel module is 0 */
#define IS_KERN_MOD(mcp)	((mcp)->mc_id == 0)

#ifdef USE_SCOBD_ADDRCHECK
/*
 * boolean_t mod_obj_addrcheck(const struct modobj *mp, 
 *		ulong_t addr)
 *
 *	See if the given addr is in the text, data, or bss of the module
 *	given by mp. Called primarily from mod_obj_searchsym and 
 *	mod_obj_validaddr.
 *
 * Calling/Exit State: 
 *	addr conatins the address in question and mp contains a 
 *	pointer to the module to be checked. The routine returns B_TRUE
 *	if the addr is within the module and B_FALSE otherwise.
 */

STATIC
boolean_t
mod_obj_addrcheck(const struct modobj *mp, ulong_t addr)
{
	if (addr >= (ulong_t) mp->md_space && 
	     addr < (ulong_t) (mp->md_space + mp->md_space_size))
		return (B_TRUE);
	if (addr >= (ulong_t) mp->md_bss 
	     && addr < (ulong_t) (mp->md_bss + mp->md_bss_size))
		return (B_TRUE);
	return (B_FALSE);
}
#endif

/*
 * char *mod_obj_searchsym_sp(const struct modobj *mp, 
 *	ulong_t value, ulong_t *offset, Elf32_Sym *retsp)
 *
 *	Look for a symbol with the nearest value less than or equal to
 *	the value specified by value. The search is limited within the
 *	loadable module given by mp.
 *
 * Calling/Exit State:
 *	This routine is called with the symbol table of the module
 *	locked and the module also being held (or they may be effectively
 *	locked and held by kdb).
 *
 *	If the value is not within the address range of the module,
 *	then NULL is returned; otherwise the name of the symbol is
 *	returned and the offset to the specified value is copied to the
 *	space pointed to by offset.
 */

STATIC char *
mod_obj_searchsym_sp(const struct modobj *mp, ulong_t value, 
	ulong_t *offset, Elf32_Sym *retsp)
{
	Elf32_Sym *symend;
	Elf32_Sym *sym;
	Elf32_Sym *cursym;
	unsigned int curval;

	ASSERT(mp != NULL);

	if (!mod_obj_addrcheck(mp, value)) {
		return (NULL);				/* not in this module */
	}

	*offset = (ulong_t) -1;			/* assume not found */
	cursym  = NULL;

	/* LINTED pointer alignment */
	symend = (Elf32_Sym *)mp->md_strings;

	/*
	 * Scan the module's symbol table for a global symbol <= value
	 */
	/* LINTED pointer alignment */
	for (sym = (Elf32_Sym *)(mp->md_symspace); sym < symend;
		/* LINTED pointer alignment */
		sym = (Elf32_Sym *)((char *)sym + mp->md_symentsize)) {

		curval = sym->st_value;

		if (curval > value)
			continue;

		if (value - curval < *offset) {
			*offset = value - curval;
			cursym = sym;
		}
	}

	if (cursym != NULL) {
		curval = cursym->st_name;
		*retsp = *cursym;
	} else {
		return (NULL);
	}

	return (mp->md_strings + curval);
}

/*
 * int mod_obj_getsymp_byname(const char *name, boolean_t kernelonly, 
 *		int flags, Elf32_Sym *retsp)
 *
 *	Look for the symbol given by name in the static kernel and 
 *	all loaded modules (unless kernelonly is B_TRUE). Called by 
 *	getksym and mod_obj_getsymvalue.
 *
 * Calling/Exit State: 
 *	name contains a '\0' terminated string of the symbol in 
 *	question. kernelonly is B_TRUE if only the static kernel 
 *	symbol table should be searched for the symbol, B_FALSE 
 *	otherwise. 
 *
 *	The routine returns 0 and a copy of the symbol table entry
 *	in the space pointed to by retsp if it is successful in
 *	finding the symbol in the table(s), ENOMATCH otherwise.
 *
 *	The NOSLEEP flag only set when kdb calls this routine indirectly.
 *	So, no locks can be acquired, and no need for holding any module
 *	while the flag is NOSLEEP.
 */

int
mod_obj_getsymp_byname(const char *name, boolean_t kernelonly, 
	int flags, Elf32_Sym *retsp)
{
	struct modctl *mcp;
	
	mcp = mod_obj_getsym_mcp(name, kernelonly, flags, retsp);

	if (mcp) {
		if (!IS_KERN_MOD(mcp)) {
			if (!(flags & NOSLEEP))
				MOD_RELE(mcp->mc_modp);
		}
		return (0);
	}
	return (ENOMATCH);
}

/*
 * char *mod_obj_getsymname(ulong_t value, ulong_t *offset, int flags, 
 *			char *retspace)
 *
 *	Look for a symbol with the nearest value less than or equal to
 *	the value specified by value within the static kernel and all
 *	the loadable modules.
 *
 * Calling/Exit State:
 *	If symbol tables are pageable and the flags is NOSLEEP,
 *	or the value is not invalid, then NULL is returned;
 *	otherwise the name of the symbol is returned and the offset to
 *	the specified value is copied to the space pointed to by offset.
 *
 *	If retspace is not NULL, the name of the symbol is copied
 *	to the space. Since kdb cannot allocate the space, the routine
 *	just return a pointer into the string table. In the normal
 *	case, the retspace should not be NULL, since the string table
 *	may be paged out.
 *
 *	Only kdb call this routine with NOSLEEP flag set.
 *	So, no locks can be acquired, and no need for holding any module
 *	while the flag is NOSLEEP.
 */
char *
mod_obj_getsymp_byvalue(ulong_t value, ulong_t *offset, int flags, char *retspace, Elf32_Sym *retsp)
{
	char *name;
	struct modctl *mcp;
	struct module *modp;
	int error;

	ASSERT((flags & NOSLEEP) || retspace != NULL);

	if (modhead.mc_modp == NULL || (mod_symtab_lckcnt == 0 &&
					flags & NOSLEEP)) {
		return (NULL);
	}

	if (!(flags & NOSLEEP)) {
		ASSERT(KS_HOLD0LOCKS());
		ASSERT(getpl() == PLBASE);

		(void)RWSLEEP_RDLOCK(&symtab_lock, PRIMED);
		(void)RW_RDLOCK(&modlist_lock, PLDLM);
	}

	error = 0;
	for (mcp = &modhead; mcp != NULL; mcp = mcp->mc_next) {
		if (mcp->mc_modp == NULL)
			continue;
		modp = mcp->mc_modp;

		if (!IS_KERN_MOD(mcp)) {
			if (!(flags & NOSLEEP)) {
				(void)LOCK(&modp->mod_lock, PLDLM);
				if (!(modp->mod_flags & MOD_SYMTABOK)) {
					UNLOCK(&modp->mod_lock, PLDLM);
					continue;
				}
				RW_UNLOCK(&modlist_lock, PLDLM);
				MOD_HOLD_L(modp, PLBASE);
			} else {
				if (!(modp->mod_flags & MOD_SYMTABOK))
					continue;
			}

		} else {
			if (!(flags & NOSLEEP))
				RW_UNLOCK(&modlist_lock, PLBASE);
		}
		
		if (mod_symtab_lckcnt == 0) {
			faultcode_t fc;

			fc = segkvn_lock(modp->mod_obj.md_symspace_mapcookie,
				    SEGKVN_FUTURE_LOCK);
			if (fc) {
				/*
				 *+ Failed to lock in symbol table for the
				 *+ module. No symbolic information of the
				 *+ module will be available.
				 */
				cmn_err(CE_NOTE,
		"MOD: Failed to lock in symbol table. The error code is %d.\n",
					FC_ERRNO(fc));
				name = NULL;
				break;
			}

			CATCH_FAULTS(CATCH_SEGKVN_FAULT) {
				if ((name = mod_obj_searchsym_sp(&modp->mod_obj,
						value, offset, retsp)) != NULL) {

					if (retspace != NULL) {
						strncpy(retspace, name, MAXSYMNMLEN);
						name = retspace;
					}
				}
			}
			error = END_CATCH();

			segkvn_unlock(modp->mod_obj.md_symspace_mapcookie,
				      SEGKVN_FUTURE_LOCK);
		} else {

			if ((name = mod_obj_searchsym_sp(&modp->mod_obj, value, 
						offset, retsp)) != NULL) {

				if (retspace != NULL) {
					strncpy(retspace, name, MAXSYMNMLEN);
					name = retspace;
				}
			}
		}

		if (!(flags & NOSLEEP)) {
			if (!IS_KERN_MOD(mcp))
				MOD_RELE(modp);
			(void)RW_RDLOCK(&modlist_lock, PLDLM);
		}

		if (error) {
			name = NULL;
			break;
		}

		/* if name is not NULL, the symbol is found */
		if (name)
			break;
	}

	if (!(flags & NOSLEEP)) {
		RW_UNLOCK(&modlist_lock, PLBASE);
		RWSLEEP_UNLOCK(&symtab_lock);
	}
	return (name);
}

scodb_unlock_symtab()
{
	mod_obj_symlock(SYMUNLOCK);
}
