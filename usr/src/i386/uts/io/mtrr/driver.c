#ident	"@(#)kern-i386:io/mtrr/driver.c	1.2.1.1"
#ident	"$Header$"

/*****************************************************************/
/*            Copyright (c) 1994 Intel Corporation               */
/*                                                               */
/* All rights reserved.  No part of this program or publication  */
/* may be reproduced, transmitted, transcribed, stored in a      */
/* retrieval system, or translated into any language or computer */
/* language, in any form or by any means, electronic, mechanical */
/* magnetic, optical, chemical, manual, or otherwise, without    */
/* the prior written permission of Intel Corporation.            */
/*                                                               */
/*****************************************************************/
/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

#ifndef DEBUG

#include <util/types.h>
#include <svc/cpu.h>
#include <proc/disp.h>
#include <util/cmn_err.h>
#include <svc/errno.h>

#endif

#include "mtrr.h"

#if defined(KDEBUG) || defined(DEBUG)

#include "debug.h"

#endif
/*----------------------------------------------------------------------*/	
/* Driver private defines */

#define SLAVE				0
#define MASTER				1

#define FALSE				0
#define TRUE				1

#define MTRR_TYPE_MASK		0xff
#define MTRR_VCNT			0xff
#define MTRR_FIXED			0x100
#define MTRR_USWC			0x400

#define MTRR_FIX_ENABLED	0x400
#define MTRR_VAR_VALID		0x800
#define MTRR_ENABLED		0x800

#define NUM_FIX_MTRR		11
#define VAR_MTRR_OFFSET		NUM_FIX_MTRR + 1
#define MTRRCAP			0xfe
#define MTRRVBASE		0x200
#define MTRRVMASK		0x201
#define MTRRDEFTYPE		0x2ff

#define FIX_MIN			0x0
#define FIX_MAX			0x100000
#define NUMFIX			8

#define BPW				32		/* bits per dword */
#define BPP				12		/* bits per 4K page */
#define MASKPAGE		0xfff
#define PAGELEN			0x1000
#define PAGEADDR		0xfffff000

#define NEAR_OVRFLW		0xffffffff

#define EXACT			1
#define INEXACT			0

typedef struct {
	unsigned long int length;
	unsigned long int start;
	unsigned long int msr;
	} mtrr_fix_t;


typedef struct mtrr_var_t {
	unsigned long int base; 
	unsigned long int length;
	int type,indx;
	struct mtrr_var_t *next;
	} mtrr_var_t;

typedef struct {
	int deftype;
	mtrr_var_t *nonuc,*uc,*free;
	} mtrr_status_t;

/*----------------------------------------------------------------------*/	
/* Function prototypes */

void set_fixtype(int, paddr_t, int);
int get_fixtype(int,paddr_t);
int get_fixshift(int, paddr_t);
int match_fixed(paddr_t,int);
int match_fixed_exact(int, paddr_t);
int match_var(paddr_t, int *);
int get4kmemtype(paddr_t, unsigned long int,int*);
void post_enable(int, unsigned long int);
unsigned long int pre_disable(int);
unsigned long int  pre_mtrr_change(int*);
void post_mtrr_change(unsigned long int);
int get_numvar_mtrrs(void);
void copy_mtrrs(ulonglong_t*);
void save_mtrrs(ulonglong_t*);
int set_default_type(mtrr_t*);
int disable_var_mtrr(mtrr_t*);
void enable_fixed_mtrrs(void);
void enable_mtrrs(void);
void disable_mtrrs(void);
void mtrr_lock(void);
void mtrr_unlock(void);
int mtrr_xcall_all(unsigned long int (*)(), void *);
int get_adralign(unsigned long int);
int get_align(unsigned long int);
int check_var_set(mtrr_t*, unsigned long int *);
int check_mtrr_type(mtrr_t *);

int MemTypeGet(mtrr_t*,int);
int MemTypeSet(mtrr_t*);
int GetVarMtrr(mtrr_t*);
int restore_init_mtrrs(mtrr_t*);

/*----------------------------------------------------------------------*/	
/* SVR4.2 MP Specific code                                              */
/*----------------------------------------------------------------------*/	

#ifndef UNIPROC
volatile unsigned long int mtrr_barrier_in,mtrr_barrier_out;
unsigned long int mtrr_num_resp;
lock_t mtrr_lck = {0};

void
mtrr_lock(void)
{
	LOCK_PLMIN(&mtrr_lck);
}

void
mtrr_unlock(void)
{
	UNLOCK_PLMIN(&mtrr_lck,PLBASE);
}

int
mtrr_xcall_all(unsigned long int (*func)(), void *data)
{
emask_t response;

	xcall_all(&response, B_TRUE, (void (*)())func, data);
}
#endif /* ! UNIPROC */
/*----------------------------------------------------------------------*/	

int mtrrdevflag = 0;

mtrr_fix_t mtrrfixed[NUM_FIX_MTRR] = {{0x10000,0x0,592},{0x4000,0x80000,600},
	{0x4000,0xa0000,601},{0x1000,0xc0000,616},{0x1000,0xc8000,617},
	{0x1000,0xd0000,618},{0x1000,0xd8000,619},{0x1000,0xe0000,620},
	{0x1000,0xe8000,621},{0x1000,0xf0000,622},{0x1000,0xf8000,623}};

int mtrr_instance = 0;
ulonglong_t *mtrrs = NULL;
ulonglong_t *init_mtrrs = NULL;
mtrr_var_t *var_mtrrs = NULL;
mtrr_var_t **var_ptrs = NULL;

mtrr_status_t ms;
int mtrr_changeable = 0;

/*
 * Mtrr_get_status
 *
 * Reads all MTRR's and puts status in ms.
 * ms.deftype = default type
 * ms.nonuc points to list of mtrr_var_t's (in order) of valid variable
 *          MTRR's that map non-UC memory 
 * ms.uc points to list of mtrr_var_t's (in order) of valid variable
 *          MTRR's that map UC memory 
 * ms.free points to list of mtrr_var_t's of invalid variable MTRR's 
 *
 */
int
Mtrr_get_status()
{
ulonglong_t mtrrdeftype;
ulonglong_t var_base, var_mask;
int curr_base, curr_mask;
int numvars;
unsigned long int int_base, int_mask, temp_int_mask;
int base_align, mask_align;
int i,j;
mtrr_var_t *var_list;
mtrr_var_t *temp_var, *temp_val_var;
int ok;
unsigned long int cr4;
int err;
char reason1[80];
int reason2;
unsigned long int r2ob,r2ol,r2nb,r2nl;
int r2oi,r2ni;

	*reason1 = reason2 = 0;
	_rdmsr(MTRRDEFTYPE, &mtrrdeftype);
	ms.deftype = mtrrdeftype.l & MTRR_TYPE_MASK;

	numvars = get_numvar_mtrrs();

	for (i=0; i<numvars; i++)  {
		var_mtrrs[i].next = &(var_mtrrs[i+1]);
		var_ptrs[i] = NULL;
	}
	var_mtrrs[i].next = NULL;

	var_list = var_mtrrs;

	ok = TRUE;
	ms.nonuc = ms.uc = ms.free = NULL;
	for (i=0, curr_base=MTRRVBASE, curr_mask=MTRRVMASK; i<numvars && ok==TRUE;
			i++, curr_base += 2, curr_mask +=2)  {
		_rdmsr(curr_base, &var_base);
		_rdmsr(curr_mask, &var_mask);
		if (var_mask.l & MTRR_VAR_VALID)  {
			int_base = var_base.l & PAGEADDR; 
			int_mask = var_mask.l & PAGEADDR;
			base_align = get_adralign(int_base); 
			mask_align = get_adralign(int_mask); 
			if (mask_align > base_align)  {
				ok = FALSE;
				(void)strcpy(reason1,"mask alignment greater than base");
				break;
			}  else  {
				temp_int_mask = int_mask >> mask_align;
				for (j=mask_align; j<BPW; j++)  {
					if (!(temp_int_mask & 1))  {
						ok = FALSE;
						(void)strcpy(reason1,"hole in mask");
						break;
					}
					temp_int_mask >>= 1;
				}
			}
			if (var_list != NULL && ok==TRUE)  {
				temp_var = var_list;
				var_ptrs[i] = temp_var;
				var_list = var_list->next;
				temp_var->type =  var_base.l & MTRR_TYPE_MASK;
				temp_var->indx = i;
				temp_var->base = int_base;
				temp_var->length = ~int_mask+1;
				if (temp_var->type == UC) 
					if (ms.uc == NULL)  {
						ms.uc = temp_var;
						temp_var->next = NULL;
					}  else if (temp_var->base < ms.uc->base)  {
						temp_var->next = ms.uc;
						ms.uc = temp_var;
					}  else  {
						temp_val_var = ms.uc;
						while((temp_val_var->next != NULL) && (temp_val_var->next->base < temp_var->base))
							temp_val_var = temp_val_var->next;
						temp_var->next = temp_val_var->next;
						temp_val_var->next = temp_var;
					}
				else if (ms.nonuc == NULL)  {
					ms.nonuc = temp_var;
					temp_var->next = NULL;
				}  else if (temp_var->base < ms.nonuc->base)
					if (temp_var->base + temp_var->length > ms.nonuc->base)  {
						(void)strcpy(reason1,"overlapping non-UC ranges (3)");
						reason2++;
						r2ob = ms.nonuc->base;
						r2ol = ms.nonuc->length;
						r2oi = ms.nonuc->indx;
						r2nb = temp_var->base;
						r2nl = temp_var->length;
						r2ni = temp_var->indx;
						ok = FALSE;
						break;
					}  else  {
						temp_var->next = ms.nonuc;
						ms.nonuc=temp_var;
					}
				else  {
					for (temp_val_var=ms.nonuc;
						(temp_val_var->next != NULL) &&
						(temp_val_var->next->base < temp_var->base);
						temp_val_var = temp_val_var->next)
							;/* we know tvv->base < tv->base */
					if(temp_var->base < temp_val_var->base+temp_val_var->length)  {
						(void)strcpy(reason1,"overlapping non-UC ranges (1)");
						reason2++;
						r2ob = temp_val_var->base;
						r2ol = temp_val_var->length;
						r2oi = temp_val_var->indx;
						r2nb = temp_var->base;
						r2nl = temp_var->length;
						r2ni = temp_var->indx;
						ok = FALSE;
						break;
					}  else if (temp_val_var->next == NULL)  {
						temp_var->next = NULL;
						temp_val_var->next = temp_var;
					}  else if (temp_val_var->next->base <
						temp_var->base+temp_var->length)  {
						ok = FALSE;
						(void)strcpy(reason1,"overlapping non-UC ranges (2)");
						reason2++;
						r2ob = temp_val_var->next->base;
						r2ol = temp_val_var->next->length;
						r2oi = temp_val_var->next->indx;
						r2nb = temp_var->base;
						r2nl = temp_var->length;
						r2ni = temp_var->indx;
						break;
					}  else  {
						temp_var->next = temp_val_var->next;
						temp_val_var->next = temp_var;
					}
				}
			}  else  {
				if (ok==TRUE) {
					ok = FALSE;
					(void)strcpy(reason1,"ran out of varMTRR descriptors for used");
				}
				break;
			}
		}  else if (var_list != NULL)  {
			temp_var = var_list;
			var_list = var_list->next; 
			temp_var->next = ms.free;
			ms.free = temp_var; 
		}  else  {
			ok = FALSE;
			(void)strcpy(reason1,"ran out of varMTRR descriptors for used");
			break;
		}
	}
	if (!ok)  {
		cmn_err(CE_WARN,"MTRR problem:  %s",reason1);
		if (reason2)
			cmn_err(CE_CONT,"OB=0x%x, OL=0x%x, OI=%d; NB=0x%x, NL=0x%x, NI=%d.\n",
				r2ob,r2ol,r2oi,r2nb,r2nl,r2ni);
		cmn_err(CE_CONT,"Resetting variable MTRRs.\n");
		cr4 = pre_mtrr_change(&err);
		if (err != 0)  {
			cmn_err(CE_CONT,"Unable to reset variable MTRRs.\n");
			return(EINVAL);
		}
		for (i = 0, curr_mask = MTRRVMASK; i < numvars; i++, curr_mask += 2)  {
			_rdmsr(curr_mask, &var_mask);
			var_mask.l &= ~MTRR_VAR_VALID;
			_wrmsr(curr_mask, &var_mask);
			var_ptrs[i] = NULL;
		}
		post_mtrr_change(cr4);
		Mtrr_get_status();
	}
	cr4 = pre_mtrr_change(&err);
	enable_mtrrs();
	enable_fixed_mtrrs();
	post_mtrr_change(cr4);
	return(0);
}

/*----------------------------------------------------------------------*/
int
get_numvar_mtrrs(void)
{
ulonglong_t mtrrcap;

	_rdmsr(MTRRCAP, &mtrrcap);
	return(mtrrcap.l & MTRR_VCNT);
}


/*----------------------------------------------------------------------*/
void
enable_fixed_mtrrs(void)
{
ulonglong_t mtrrdef;
mtrr_t fixmtrr;
paddr_t addr;
int indx, fixtype;

	_rdmsr(MTRRDEFTYPE,&mtrrdef);
	if (!(mtrrdef.l & MTRR_FIX_ENABLED)) {
		mtrrdef.l |= MTRR_FIX_ENABLED;
		_wrmsr(MTRRDEFTYPE, &mtrrdef);
		fixtype = mtrrdef.l & MTRR_TYPE_MASK;
		addr = FIX_MIN;
		while (addr < FIX_MAX)  {
			indx = match_fixed(addr,EXACT);
			fixmtrr.addr = addr;
			fixmtrr.len = mtrrfixed[indx].length;
			fixmtrr.type = UC;
			if (MemTypeGet(&fixmtrr,B_FALSE) || fixmtrr.type < 0)
				set_fixtype(indx,addr,fixtype);
			else
				set_fixtype(indx,addr,fixmtrr.type);
			addr += mtrrfixed[indx].length;
			}
		}
}
/*----------------------------------------------------------------------*/
int
get_fixshift(int indx, paddr_t addr)
{
int shift;

shift=(((addr - mtrrfixed[indx].start)/mtrrfixed[indx].length) * 8);
return(shift);
}
/*----------------------------------------------------------------------*/
void
set_fixtype(int indx, paddr_t address, int type)
{
ulonglong_t fixed;
unsigned long int mask = MTRR_TYPE_MASK;
unsigned long int *ptr = &fixed.l;
int shift;

	_rdmsr(mtrrfixed[indx].msr, &fixed);
	shift = get_fixshift(indx, address);
	if (shift >= BPW) {
		ptr = &fixed.h;
		shift -= BPW;
		}
	mask <<= shift;
	*ptr &= ~mask;
	*ptr |= (type << shift);
	_wrmsr(mtrrfixed[indx].msr, &fixed);
}
/*----------------------------------------------------------------------*/
int
get_fixtype(int indx, paddr_t address)
{
ulonglong_t fixed;
unsigned long int mask = MTRR_TYPE_MASK;
unsigned long int *ptr = &fixed.l;
int shift;

	_rdmsr(mtrrfixed[indx].msr, &fixed);
	shift = get_fixshift(indx, address);
	if (shift >= BPW) {
		ptr = &fixed.h;
		shift -= BPW;
		}
	mask <<= shift;
	return((*ptr & mask) >> shift);
}

/*----------------------------------------------------------------------*/
int
match_fixed_exact(int i, paddr_t addr)
{
int j;

	for (j=0; j< NUMFIX; j++) {
		if ((mtrrfixed[i].start + (j * mtrrfixed[i].length)) == addr)
			break;
		}
	if (j < NUMFIX)
		return (j);
	else
		return (-1);
}
/*----------------------------------------------------------------------*/
/* This function returns the mtrr index from the mtrrfixed[] array for */
/* the requested address if the address matches a fixed range (640-1M) */

int
match_fixed(paddr_t addr,int how)
{
ulonglong_t mtrrcap;
int i = 0;

	_rdmsr(MTRRCAP, &mtrrcap);
	if (mtrrcap.l & MTRR_FIXED) {
		while (i < NUM_FIX_MTRR) {
			if (how == EXACT) {
				if (match_fixed_exact(i,addr) >= 0)
				break;
				}
			else {
				if ((mtrrfixed[i].start+(NUMFIX*mtrrfixed[i].length)) >
						addr)
					break;
				}
			i++;
			}
		if (i < NUM_FIX_MTRR)
			return(i);
		}
	return(INVALID);
}
/*----------------------------------------------------------------------*/
int
match_var(paddr_t address, int *ext)
{
ulonglong_t mtrrvar;
paddr_t mask, base;
int indx,var;

	var = get_numvar_mtrrs();
	for (indx=0; indx < var; indx++) {
		_rdmsr((indx * 2) + MTRRVMASK, &mtrrvar);
		if (!(mtrrvar.l & MTRR_VAR_VALID))
			continue;
		mask = mtrrvar.l & PAGEADDR;
		_rdmsr((indx * 2) + MTRRVBASE, &mtrrvar);
		base = mtrrvar.l & PAGEADDR;
		if ((mask & base) == (address & mask)) {
			*ext = indx;
			return(mtrrvar.l & MTRR_TYPE_MASK);
			}
		}
	return(INVALID);
}


/*----------------------------------------------------------------------*/
int
get4kmemtype(paddr_t addr, unsigned long int mtrrdef, int *ext)
{
int indx,type;

	if ((indx=match_fixed(addr,INEXACT)) >= 0) {
		*ext = MTRR_MAPPED_FIXED;
		return(get_fixtype(indx, addr));
	}
	if ((type=match_var(addr,ext)) >= 0)
			return(type);

	*ext = MTRR_MAPPED_DEFAULT;
	return(ms.deftype);
}
/*----------------------------------------------------------------------*/	
void
save_mtrrs(ulonglong_t *save_array)
{
int i,var;

	for (i=0; i<NUM_FIX_MTRR; i++)
		_rdmsr(mtrrfixed[i].msr, save_array++);

	_rdmsr(MTRRDEFTYPE, save_array++);

	var = get_numvar_mtrrs();
	for (i=0; i< (var * 2); i+=2) {
		_rdmsr(i + MTRRVBASE, save_array++);
		_rdmsr(i + MTRRVMASK, save_array++);
	}
}
/*----------------------------------------------------------------------*/	
void
copy_mtrrs(ulonglong_t *save_array)
{
int i,var;

	for (i=0; i<NUM_FIX_MTRR; i++)
		_wrmsr(mtrrfixed[i].msr, save_array++);

	_wrmsr(MTRRDEFTYPE, save_array++);

	var = get_numvar_mtrrs();
	for (i=0; i< (var * 2); i+=2) {
		_wrmsr(i + MTRRVBASE, save_array++);
		_wrmsr(i + MTRRVMASK, save_array++);
	}
}
/*----------------------------------------------------------------------*/	
asm
enable_interrupts(void)
{
#if defined(KDEBUG) || !defined(DEBUG)
	sti
#endif
}
/*----------------------------------------------------------------------*/	
asm
disable_interrupts(void)
{
#if defined(KDEBUG) || !defined(DEBUG)
	cli
#endif
}

/*----------------------------------------------------------------------*/	
asm
put_cr4(unsigned long int oldcr4)
{
%mem oldcr4;

#if defined(KDEBUG) || !defined(DEBUG)

	movl oldcr4,%eax
	movl %eax,%cr4
#endif
}
/*----------------------------------------------------------------------*/	
asm
get_cr4(void)
{
#if defined(KDEBUG) || !defined(DEBUG)
	movl %cr4,%eax
#endif
}
/*----------------------------------------------------------------------*/	
asm void
enable_caches(void)
{
#if defined(KDEBUG) || !defined(DEBUG)
	.set CR0_CD_MASK, 0xbfffffff

	movl %cr0,%eax
	andl $CR0_CD_MASK,%eax
	movl %eax,%cr0
#endif
}
/*----------------------------------------------------------------------*/	
asm void
disable_caches(void)
{
#if defined(KDEBUG) || !defined(DEBUG)
	.set CR0_NW_MASK, 0xdfffffff
	.set CR0_CD, 0x40000000
	.set CR4_MASK, 0xffffff7f

#ifdef P6A0
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	xorl %eax,%eax
	call _wbinvd		/* software workaround for cache flush - P6A0 only */
	cpuid			/* serializes */
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
#endif /* P6A0 */

	movl %cr0,%eax
	andl $CR0_NW_MASK,%eax
	orl $CR0_CD,%eax
	movl %eax,%cr0

#ifndef P6A0
	wbinvd
#endif /* !P6A0 */

	movl %cr4,%eax
	andl $CR4_MASK,%eax
	movl %eax,%cr4

	movl %cr3,%eax
	movl %eax,%cr3
#endif
}

/*----------------------------------------------------------------------*/	
void
enable_mtrrs(void)
{
ulonglong_t mtrrdeftype;

	_rdmsr(MTRRDEFTYPE, &mtrrdeftype);
	mtrrdeftype.l |= MTRR_ENABLED;
	_wrmsr(MTRRDEFTYPE, &mtrrdeftype);
}
/*----------------------------------------------------------------------*/	
void
disable_mtrrs(void)
{
ulonglong_t mtrrdeftype;

	_rdmsr(MTRRDEFTYPE, &mtrrdeftype);
	mtrrdeftype.l &= ~MTRR_ENABLED;
	_wrmsr(MTRRDEFTYPE, &mtrrdeftype);
}
/*----------------------------------------------------------------------*/	
asm void
barrier(volatile unsigned long int *addr, unsigned long int cnt)
{
%mem addr,cnt; lab wait;

	pushl cnt
	movl addr,%eax
	popl %ecx
	lock
	inc (%eax)
wait:
	cmpl %ecx,(%eax)
	jne wait
}
/*----------------------------------------------------------------------*/	
void
post_enable(int type, unsigned long int oldcr4)
{
#ifndef UNIPROC

	if (type == MASTER) {
		save_mtrrs(mtrrs);
		barrier(&mtrr_barrier_in, mtrr_num_resp);
		}
	else
		copy_mtrrs(mtrrs);

#endif /* ! UNIPROC */

	disable_caches();
	enable_mtrrs();

#ifndef UNIPROC

	barrier(&mtrr_barrier_out, mtrr_num_resp);

#endif /* ! UNIPROC */

	enable_caches();
	put_cr4(oldcr4);
	enable_interrupts();
}

/*----------------------------------------------------------------------*/	
/* This routine disables interrupts, saves the old value of cr4, and    */
/* then disables on-chip caches, writes them back and invalidates them, */
/* clears the PGE bit in cr4, and then flushes the tlb.                 */
/* It returns the value of cr4 prior to when the function was called.   */

unsigned long int
pre_disable(int type)
{
unsigned long int cr4;

	disable_interrupts();
	cr4 = get_cr4();
	disable_caches();
	disable_mtrrs();

#ifndef UNIPROC
	if (type != MASTER) {
		barrier(&mtrr_barrier_in, mtrr_num_resp);
		post_enable(type,cr4);
		}
#endif /* ! UNIPROC */
	return(cr4);
}
/*----------------------------------------------------------------------*/	
void
post_mtrr_change(unsigned long int oldcr4)
{
	post_enable(MASTER, oldcr4);
}
/*----------------------------------------------------------------------*/	
unsigned long int
pre_mtrr_change(int *rval)
{
#ifndef UNIPROC

emask_t responders;
int type = SLAVE;
engine_t *eng;
int engnum;

	/* Initialize the responders mask with all active engines. */
	mtrr_num_resp = 0;
	EMASK_CLRALL(&responders);
	for (eng = &engine[engnum = Nengine]; engnum-- != 0;) {
		if (!((--eng)->e_flags & E_NOWAY)) {
			EMASK_SET1(&responders, engnum);
			mtrr_num_resp++;
		}
	}
	mtrr_barrier_in = mtrr_barrier_out = 0;
	if (mtrr_num_resp != Nengine) {
		*rval = ENGINES_OFFLINE;
		return(0);
	}

	if (EMASK_TESTALL(&responders))
		mtrr_xcall_all(pre_disable, &type);

#endif /* ! UNIPROC */

	*rval = 0;
	return(pre_disable(MASTER));
}
/*----------------------------------------------------------------------*/	
int
get_adralign(unsigned long int val)
{
int mask = 1;
int i;

	for (i=0; i<BPW; i++) {
		if (val & mask)
			break;
		mask <<= 1;
		}
	return(i);
}

/*----------------------------------------------------------------------*/	
int
get_align(unsigned long int val)
{
int mask = 0x1000;
int i;

	for (i=BPP; i<BPW; i++) {
		if (val == mask)
			return(i);
		mask <<= 1;
		}
	return(-1);
}
/*----------------------------------------------------------------------*/	
int
check_var_set(mtrr_t* mtrr, unsigned long int *mask)
{
int i,adr_align,len_align;

	*mask = 0xfffff000;
	if (mtrr->len == 0x1000) /* SPECIAL CASE: only 4K alignment */
		return(0);

	/* Check base address alignment */
	adr_align = get_adralign(mtrr->addr);

	/* Check for invalid length */
	if ((len_align = get_align(mtrr->len)) < 0)
		return(-1);

	if (adr_align >= len_align) {
		for (i=0; i<(len_align - BPP); i++)
			*mask <<= 1;
		return(0);
		}
	else
		return(-1);
}
/*----------------------------------------------------------------------*/	
int
check_mtrr_type(mtrr_t *mtrr)
{
ulonglong_t mtrrcap;
int err = EINVAL;

	switch(mtrr->type) {
		case UC:
		case WT:
		case WP:
		case WB:
			err = 0;
			break;

		case USWC:
			_rdmsr(MTRRCAP, &mtrrcap);
			if (!(mtrrcap.l & MTRR_USWC))
				mtrr->type = USWC_NOT_SUPPORTED;
			else
				err = 0;
			break;
		default:
			mtrr->type = INVALID_MEM_TYPE;
		}
	return(err);
}

/*----------------------------------------------------------------------*/	
int
disable_var_mtrr(mtrr_t *mtrr)
{
unsigned long int cr4;
int var,rval;
ulonglong_t mtrrvar;
mtrr_var_t *temp_var, *ttemp_var;

	var = get_numvar_mtrrs();
	if ((mtrr->misc < 0)||(mtrr->misc >= var)) {
		mtrr->type = INVALID_VAR_MTRR;
		return(EINVAL);
	}
	_rdmsr((mtrr->misc * 2) + MTRRVMASK, &mtrrvar);
	if (mtrrvar.l & MTRR_VAR_VALID) {
		mtrrvar.l &= ~MTRR_VAR_VALID;
		cr4 = pre_mtrr_change(&rval);
		if (rval != 0) {
			mtrr->type = rval;
			return(EINVAL);
			}
		_wrmsr((mtrr->misc * 2) + MTRRVMASK, &mtrrvar);
		post_mtrr_change(cr4);
		if (ms.uc != NULL)
			if (ms.uc->indx == mtrr->misc)  {
				temp_var = ms.uc;
				ms.uc = temp_var->next;
				temp_var->next = ms.free;
				ms.free = temp_var;
				return(0);
			}  else
				for (temp_var=ms.uc; temp_var->next != NULL; temp_var=temp_var->next)
					if (temp_var->next->indx == mtrr->misc)  {
						ttemp_var = temp_var->next;
						temp_var->next = ttemp_var->next;
						ttemp_var->next = ms.free;
						ms.free = ttemp_var;
						return(0);
					}
		if (ms.nonuc != NULL)
			if (ms.nonuc->indx == mtrr->misc)  {
				temp_var = ms.nonuc;
				ms.nonuc = temp_var->next;
				temp_var->next = ms.free;
				ms.free = temp_var;
				return(0);
			}  else
				for (temp_var=ms.nonuc; temp_var->next != NULL; temp_var=temp_var->next)
					if (temp_var->next->indx == mtrr->misc)  {
						ttemp_var = temp_var->next;
						temp_var->next = ttemp_var->next;
						ttemp_var->next = ms.free;
						ms.free = ttemp_var;
						return(0);
					}
		cmn_err(CE_WARN,"Problem with MTRR data structure.");
	}
	return(0);
}
/*----------------------------------------------------------------------*/	
int
set_default_type(mtrr_t *mtrr)
{
ulonglong_t mtrrdeftype;
unsigned long int cr4;
int rval;

	_rdmsr(MTRRDEFTYPE, &mtrrdeftype);

	if ((mtrrdeftype.l & MTRR_TYPE_MASK) != mtrr->type) {
		mtrrdeftype.l &= ~MTRR_TYPE_MASK;
		mtrrdeftype.l |= mtrr->type;
		cr4 = pre_mtrr_change(&rval);
		if (rval != 0) {
			mtrr->type = rval;
			return(EINVAL);
			}
		_wrmsr(MTRRDEFTYPE, &mtrrdeftype);
		post_mtrr_change(cr4); 
		ms.deftype = mtrr->type;
		}
	return(0);
}

/*----------------------------------------------------------------------*/	
/* This function returns the memory type for the given physical address */
/* base of length size. */

int
MemTypeGet(mtrr_t* mtrr, int fixok)
{
int firsttype,nexttype,ext;
paddr_t addr;
mtrr_var_t *temp_uc, *temp_nonuc;
int hold_misc;
unsigned long int target_beg, target_end, old_beg, orig_base; 
unsigned long int old_uc_end, old_nonuc_end;

	if ((mtrr->len == 0) || ((mtrr->addr + (mtrr->len - 1)) < mtrr->addr)) {
		mtrr->type = INVALID;
		return(EINVAL);
	}

	if (!fixok || mtrr->addr >= FIX_MAX)  {
		orig_base = mtrr->addr;
		target_beg = mtrr->addr;
		target_end = mtrr->addr + (mtrr->len - 1);
		for (temp_uc=ms.uc, old_uc_end=0; (temp_uc != NULL) &&
				(target_beg > temp_uc->base + (temp_uc->length - 1));
			old_uc_end = temp_uc->base + (temp_uc->length - 1),
			temp_uc = temp_uc->next)
			; 
		for (temp_nonuc=ms.nonuc, old_nonuc_end=0; (temp_nonuc != NULL) &&
				(target_beg > temp_nonuc->base + (temp_nonuc->length - 1));
			old_nonuc_end = temp_nonuc->base + (temp_nonuc->length - 1),
			temp_nonuc = temp_nonuc->next)
			;
		if (temp_uc == NULL && temp_nonuc == NULL)  {
			mtrr->type = ms.deftype;
			mtrr->misc = MTRR_MAPPED_DEFAULT;
			mtrr->addr = (MAX(old_uc_end,old_nonuc_end) ?
					1+MAX(old_uc_end,old_nonuc_end) :
					0);
			if (mtrr->addr != 0)
				mtrr->len = (NEAR_OVRFLW-mtrr->addr)+1;
			else
				mtrr->len = NEAR_OVRFLW;
		}  else  {
			if ((temp_nonuc == NULL || target_beg < temp_nonuc->base) &&
				(temp_uc == NULL || target_beg < temp_uc->base))  {
				firsttype = ms.deftype;
				hold_misc = MTRR_MAPPED_DEFAULT;
				mtrr->addr = 1+MAX(old_uc_end,old_nonuc_end);
				if (temp_nonuc == NULL)
					target_beg = temp_uc->base;
				else if (temp_uc == NULL)
					target_beg = temp_nonuc->base;
				else
					target_beg = MIN(temp_uc->base,temp_nonuc->base);
			}  else if (temp_uc != NULL && target_beg >= temp_uc->base)  {
				firsttype = UC;
				hold_misc = temp_uc->indx;
				mtrr->addr = temp_uc->base;
				target_beg = temp_uc->base + temp_uc->length;
				temp_uc = temp_uc->next;
				while (temp_nonuc != NULL &&
						temp_nonuc->base+(temp_nonuc->length-1) <= target_beg-1)
					temp_nonuc = temp_nonuc->next;
			}  else  {
				firsttype = temp_nonuc->type;
				hold_misc = temp_nonuc->indx;
				mtrr->addr = temp_nonuc->base;
				if (temp_uc == NULL ||
						temp_uc->base > temp_nonuc->base+(temp_nonuc->length-1))
					target_beg = temp_nonuc->base + temp_nonuc->length;
				else
					target_beg = temp_uc->base;
				temp_nonuc = temp_nonuc->next;
			}
			while (target_beg && (temp_uc != NULL || temp_nonuc != NULL))  {
				old_beg = target_beg;
				if ((temp_nonuc == NULL || target_beg < temp_nonuc->base) &&
					(temp_uc == NULL || target_beg < temp_uc->base))  {
					nexttype = ms.deftype;
					if (temp_nonuc == NULL)
						target_beg = temp_uc->base;
					else if (temp_uc == NULL)
						target_beg = temp_nonuc->base;
					else
						target_beg = MIN(temp_uc->base,temp_nonuc->base);
				}  else if (temp_uc != NULL && target_beg >= temp_uc->base)  {
					nexttype = UC;
					target_beg = temp_uc->base + temp_uc->length;
					temp_uc = temp_uc->next;
					while (temp_nonuc != NULL &&
						temp_nonuc->base+(temp_nonuc->length-1) < target_beg)
						temp_nonuc = temp_nonuc->next;
				}  else  {
					nexttype = temp_nonuc->type;
					if (temp_uc == NULL ||
						temp_uc->base > temp_nonuc->base + (temp_nonuc->length-1))
						target_beg = temp_nonuc->base + temp_nonuc->length;
					else
						target_beg = temp_uc->base;
					temp_nonuc = temp_nonuc->next;
				}
				if (firsttype != nexttype)
					if (old_beg > target_end)  {
						mtrr->len = old_beg - mtrr->addr;
						mtrr->type = firsttype;
						mtrr->misc = hold_misc;
						if (mtrr->addr < FIX_MAX && orig_base >= FIX_MAX)  {
							mtrr->len -= (FIX_MAX - mtrr->addr);
							mtrr->addr = FIX_MAX;
						}
						return(0);
					}  else  {
						mtrr->type = MIXED_TYPES;
						return(EINVAL);	
					}
				hold_misc = MTRR_MAPPED_MULTIPLE;
			}
			nexttype = ms.deftype;
			if (firsttype != nexttype)
				if ((target_beg > target_end) || target_beg == 0)  {
					mtrr->len = target_beg - mtrr->addr;
					mtrr->type = firsttype;
					mtrr->misc = hold_misc;
					if (mtrr->addr < FIX_MAX && orig_base >= FIX_MAX)  {
						mtrr->len -= (FIX_MAX - mtrr->addr);
						mtrr->addr = FIX_MAX;
					}
					return(0);
				}  else  {
					mtrr->type = MIXED_TYPES;
					return(EINVAL);
				}
			hold_misc = MTRR_MAPPED_MULTIPLE;
			if (mtrr->addr != 0)
				mtrr->len = (NEAR_OVRFLW-mtrr->addr)+1;
			else
				mtrr->len = NEAR_OVRFLW;
			mtrr->type = firsttype;
			mtrr->misc = hold_misc;
		}
	}  else  {
		if (mtrr->addr & MASKPAGE)	/* align address to 4K boundary */
			mtrr->addr &= PAGEADDR; 

		if (mtrr->len & MASKPAGE)	/* align length to 4K size */
			mtrr->len = ((mtrr->len - 1) & PAGEADDR) + PAGELEN;

		if ((mtrr->len > 0)&&((mtrr->addr + (mtrr->len - 1)) < mtrr->addr)) {
			mtrr->type = INVALID;
			return(EINVAL);
			}
		firsttype = get4kmemtype(mtrr->addr, ms.deftype, &ext);
		if (mtrr->len > 0) {
			for (addr=(mtrr->addr+NBPP);
					addr<(mtrr->addr+mtrr->len);addr+=NBPP){
				nexttype = get4kmemtype(addr, ms.deftype, &ext);
				if (nexttype != firsttype) {
					mtrr->type = MIXED_TYPES;
					return(EINVAL);
					}
				}
			}
		mtrr->type = firsttype;
		mtrr->misc = ext;
	}
	return(0);
}

/*----------------------------------------------------------------------*/	
int
MemTypeSet(mtrr_t *mtrr)
{
ulonglong_t mtrrvar;
unsigned long int tlen,varmask,cr4;
paddr_t addr;
int i,indx,type,var,err;
mtrr_var_t *temp_var, *temp_val_var;

	if ((mtrr->len == 0)||(mtrr->addr & MASKPAGE)||(mtrr->len & MASKPAGE)) {
		mtrr->type = INVALID;
		return(EINVAL);
	}

	/* Range wraps around to address zero */
	if ((mtrr->addr + (mtrr->len - 1)) < mtrr->addr) {
		mtrr->type = INVALID;
		return(EINVAL);
	}

	/* Check for invalid memory type */
	if ((err=check_mtrr_type(mtrr)))
		return(err);

	/* Try to map range using fixed range registers */
	if ((mtrr->addr >= FIX_MIN) && ((mtrr->addr + mtrr->len) <= FIX_MAX)) {
		/* Address MUST map exactly to fixed range registers */
		/* Size MUST map exactly to fixed range registers */
		addr = mtrr->addr; tlen = 0;
		while (addr < (mtrr->addr + mtrr->len)) {
			if ((indx=match_fixed(addr,EXACT)) >= 0) {
				addr += mtrrfixed[indx].length;
				tlen += mtrrfixed[indx].length;
				}
			else {
				/* Start address does not match fixed range register */
				tlen = 0;
				break;
				}
			}
		/* Proper length and size for fixed range registers ? */
		if (tlen == mtrr->len) {
			/* Second pass: do the whole range, batch mode */
			mtrr->misc = MTRR_MAPPED_FIXED;
			cr4 = pre_mtrr_change(&err);
			if (err != 0) {
				mtrr->type = err;
				return(EINVAL);
				} 
			enable_fixed_mtrrs();
			addr = mtrr->addr;
			while (addr < (mtrr->addr + mtrr->len)) {
				indx = match_fixed(addr,EXACT);
				set_fixtype(indx,addr,mtrr->type);
				addr += mtrrfixed[indx].length;
				}
			post_mtrr_change(cr4);
			return(0);
			}
		}
	/* Use variable range MTRR */
	if ((var=get_numvar_mtrrs()) == 0) {
		mtrr->type = VAR_NOT_SUPPORTED;
		return(EINVAL);
		}
	/* Already mapped using variable MTRR ? */
	if (mtrr->type != UC)  {
		for (temp_var = ms.nonuc;
			temp_var != NULL && mtrr->addr >= temp_var->base + temp_var->length;
			temp_var = temp_var->next)
				;
		if (temp_var != NULL && mtrr->addr + mtrr->len > temp_var->base)  {
			mtrr->type = RANGE_OVERLAP;
			mtrr->misc = temp_var->indx;
			return(EINVAL);
		}
	}
	if (check_var_set(mtrr,&varmask) != 0) {
		mtrr->type = INVALID_VAR_REQ;
		return(EINVAL);
		}
	for (i=0; i< var; i++) {
		_rdmsr((i * 2) + MTRRVMASK, &mtrrvar);
		if (!(mtrrvar.l & MTRR_VAR_VALID))
			break;
		}	
	if (i >= var) {
		mtrr->type = VAR_NOT_AVAILABLE;
		return(EINVAL);
		}
	mtrr->misc = i;
	cr4 = pre_mtrr_change(&err);
	if (err != 0) {
		mtrr->type = err;
		return(EINVAL);
		} 
	mtrrvar.h = 0;
	mtrrvar.l = (mtrr->addr & PAGEADDR) | mtrr->type;
	_wrmsr((i * 2) + MTRRVBASE, &mtrrvar);
	mtrrvar.l = varmask | MTRR_VAR_VALID;
	_wrmsr((i * 2) + MTRRVMASK, &mtrrvar);
	post_mtrr_change(cr4);
	if (ms.free != NULL)  {
		temp_var = ms.free;
		ms.free = ms.free->next;
		temp_var->type =  mtrr->type;
		temp_var->indx = i;
		temp_var->base = mtrr->addr;
		temp_var->length = mtrr->len;
		if (temp_var->type == UC) 
			if (ms.uc == NULL)  {
				ms.uc = temp_var;
				temp_var->next = NULL;
			}  else  {
				temp_val_var = ms.uc;
				while((temp_val_var->next != NULL) && (temp_val_var->next->base < mtrr->addr))
					temp_val_var = temp_val_var->next;
				temp_var->next = temp_val_var->next;
				temp_val_var->next = temp_var;
			}
		else if (ms.nonuc == NULL)  {
			ms.nonuc = temp_var;
			temp_var->next = NULL;
		}  else  {
			temp_val_var = ms.nonuc;
			while((temp_val_var->next != NULL) &&
				(temp_val_var->next->base < mtrr->addr))
				temp_val_var = temp_val_var->next;
			temp_var->next = temp_val_var->next;
			temp_val_var->next = temp_var;
		}
	}  else
		cmn_err(CE_WARN,"Internal data structure problem in MTRR driver.");
	return(0);
}

int
restore_init_mtrrs(mtrr_t *mtrr)
{
int err,cr4;

	cr4 = pre_mtrr_change(&err);
	if (err != 0) {
		mtrr->type = err;
		return(EINVAL);
	} 
	copy_mtrrs(init_mtrrs);
	post_mtrr_change(cr4);
	return(0);
}

/*----------------------------------------------------------------------*/	
int
GetVarMtrr(mtrr_t *mtrr)
{
int numvars, varnum;
ulonglong_t mtrrvar;
int rval = 0;

	numvars = get_numvar_mtrrs();
	varnum = mtrr->misc;

	if ((varnum < 0) || (varnum >= numvars)) {
		mtrr->addr = 0;
		mtrr->len = 0;
		mtrr->type = INVALID_VAR_MTRR;
		rval = EINVAL;
	} else if (var_ptrs[varnum] == NULL)  {
		mtrr->addr = 0;
		mtrr->len = 0;
		mtrr->type = VAR_MTRR_NOT_SET;
	}  else  {
		mtrr->addr = var_ptrs[varnum]->base;
		mtrr->len = var_ptrs[varnum]->length;
		mtrr->type = var_ptrs[varnum]->type;
	}

	return(rval);
}

/*----------------------------------------------------------------------*/	
/* I/O Control interface */

int
mtrrioctl(dev_t dev,int cmd,caddr_t arg,int mode,cred_t cred_t,int rval_p)
{
int type,err = 0;
ulonglong_t mtrrcap,mtrrdeftype;
mtrr_t param;

	if (arg == NULL)
		return(EFAULT);

	if (!(l.cpu_features[0] & CPUFEAT_MTRR)) {
		param.type = MTRR_NOT_SUPPORTED;
		return(EINVAL);
	}

	if ((err = Mtrr_get_status()) == 0)  {

		if (copyin((caddr_t)arg, (caddr_t)&param, sizeof(mtrr_t)))
			return(EFAULT);

		switch(cmd) {
			case IOCTL_MEM_TYPE_GET:
				err = MemTypeGet(&param,B_TRUE);
				break;

			case IOCTL_MEM_TYPE_SET:
				err = MemTypeSet(&param);
				break;

			case IOCTL_VAR_MTRR_DISABLE:
				err = disable_var_mtrr(&param);
				break;

			case IOCTL_GET_DEFAULT:
				_rdmsr(MTRRDEFTYPE, &mtrrdeftype);
				param.type = mtrrdeftype.l & MTRR_TYPE_MASK;
				break;

			case IOCTL_SET_DEFAULT:
				if ((err=check_mtrr_type(&param)))
					break;
				err = set_default_type(&param);
				break;

#ifdef XXX
			case IOCTL_RESTORE_INIT:
				err = restore_init_mtrrs(&param);
				break;
#endif

			case IOCTL_GET_VAR_MTRR:
				err = GetVarMtrr(&param);
				break;

			default:
				err = EINVAL;
				break;
		}
		if (copyout((caddr_t)&param, (caddr_t)arg, sizeof(mtrr_t)))
			return(EFAULT);
	}
	return(err);
}

/*----------------------------------------------------------------------*/	
mtrrinit()
{
int var;

#ifdef DEBUG
#ifdef P6A0
        cmn_err(CE_NOTE,"P6A0 Memory Type Range Register Driver (no WBINVD)");
#elif defined P6A1
        cmn_err(CE_NOTE,"P6A1 Memory Type Range Register Driver (revised)");
#else
        cmn_err(CE_NOTE,"Memory Type Range Register Driver");
        cmn_err(CE_NOTE,"Copyright (c) Intel Corporation 1995");
#endif
#endif /* DEBUG */	

#ifdef KDEBUG
	l.cpu_features[0] |= CPUFEAT_MTRR;
#endif /* KDEBUG */

	if (l.cpu_features[0] & CPUFEAT_MTRR) {
		var = get_numvar_mtrrs();
		mtrrs=(ulonglong_t*)kmem_alloc(sizeof(ulonglong_t)*(VAR_MTRR_OFFSET+var),KM_NOSLEEP);
		var_mtrrs=(mtrr_var_t*)kmem_alloc(sizeof(mtrr_var_t)*var,KM_NOSLEEP);
		var_ptrs=(mtrr_var_t**)kmem_alloc(sizeof(mtrr_var_t*)*var,KM_NOSLEEP);
		init_mtrrs=(ulonglong_t*)kmem_alloc(sizeof(ulonglong_t)*(VAR_MTRR_OFFSET+var),KM_NOSLEEP);
		if (mtrrs == NULL || var_mtrrs == NULL || var_ptrs == NULL || init_mtrrs == NULL) {
			cmn_err(CE_WARN,"Unable to allocate memory for MTRRs.");
			return(-1);
		}
		save_mtrrs(init_mtrrs);
	}
#ifdef DEBUG
	/*
	 * This driver is configured by default on even non P6 machines.
	 * So this message is misleading.
	 */
	else
		cmn_err(CE_WARN,"Processor does not support MTRRs.");
#endif	
	return(0);
}
/*----------------------------------------------------------------------*/	
/* ARGSUSED */
int
mtrropen(devp, mode, type, cr)
dev_t *devp;
int mode;
int type;
struct cred *cr;
{
int err = 0;

	if (mtrrs == NULL)
		return(-1);

#ifndef UNIPROC

	mtrr_lock();

#endif /* ! UNIPROC */

	if (mtrr_instance == 0)
		mtrr_instance++;
	else
		err = EBUSY;

#ifndef UNIPROC

	mtrr_unlock();

#endif /* ! UNIPROC */

	return(err);
}

/*----------------------------------------------------------------------*/	
/* ARGSUSED */
int
mtrrclose(dev, flag, cr)
dev_t dev;
int flag;
struct cred *cr;
{
#ifndef UNIPROC

	mtrr_lock();

#endif /* ! UNIPROC */

	if (mtrr_instance > 0)
		mtrr_instance--;

#ifndef UNIPROC

	mtrr_unlock();

#endif /* ! UNIPROC */

	return(0);
}
