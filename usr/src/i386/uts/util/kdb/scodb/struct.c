#ident	"@(#)kern-i386:util/kdb/scodb/struct.c	1.2"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/*
 * Modification History:
 *
 *	L000	scol!nadeem	26aug92
 *	- when doing a "struct" command which follows a pointer, terminate
 *	  if we end up back where we first started (ie: the end of a circular
 *	  list).
 *	L001	nadeem@sco.com	13dec94
 *	- added support for user level scodb (under #ifdef USER_LEVEL):
 *	- in the user binary, the db_stuntable and db_varitable variables
 *	  are not character arrays but pointers.
 *	- in stun(), change a "%x" to a "%X" so that hex numbers are output
 *	  with capital letters.
 */

#include	"sys/types.h"
#include	<syms.h>
#include	"sys/reg.h"
#include	"dbg.h"
#include	"histedit.h"
#include	"stunv.h"
#include	"sent.h"
#include	"bkp.h"
#include	"val.h"

extern char scodb_kstruct_info[];

NOTSTATIC int nstun;
NOTSTATIC struct cstun *stun = 0;

NOTSTATIC int nvari;
NOTSTATIC struct cvari *cvari = 0;

#define		PS_STOP		0
#define		PS_NOFOLLOW	1
#define		PS_CONT		2

/*
*	struct [stname] [-> field] [fields] address
*/
NOTSTATIC
c_struct(c, v)
	int c;
	char **v;
{
	int n, r, ret = DB_CONTINUE;
	int t_basic, t_derived, initial_seg, initial_value;	/* L000 */
	struct value va;
	struct cstun *cs = 0;
	struct cstun *findstun();
	char *followfield = 0;
	char **addrargl, *addrargs;
	extern int *REGP;

	va.va_seg = REGP[T_DS];
	if (!value(v[--c], &va, 1, 1)) {
		perr();
		return DB_ERROR;
	}
	addrargl = &v[c];
	addrargs = v[c];
	--c, ++v;
	/* check for a structure name */
	if (c && strcmp(*v, "->")) {
		if (cs = findstun(*v))
			--c, ++v;
	}
	if (c && !strcmp(*v, "->")) {
		--c, ++v;
		if (c == 0)
			return DB_USAGE;
		followfield = *v;
		--c, ++v;
	}
	t_basic = BTYPE(va.va_cvari.cv_type);
	t_derived = va.va_cvari.cv_type >> N_BTSHFT;
	if (!IS_STUN(t_basic) || (!IS_PTR(t_derived) && !IS_ARY(t_derived))) {
		if (cs == 0) {
			printf("Don't know structure/union type.\n");
			return DB_ERROR;
		}
	}
	else {
		extern char *e_stun_unk;

		if (va.va_cvari.cv_index < 0) {
			printf("%s\n", e_stun_unk);
			return DB_ERROR;
		}
		cs = &stun[va.va_cvari.cv_index];
	}

	initial_seg = va.va_seg;				/* L000 */
	initial_value = va.va_value;				/* L000 */

	/*
	*	terminate the argument vector after the
	*	field names we're supposed to print (if any)
	*
	*	the vector MUST be reconstructed afterwards
	*	or all hell will break loose
	*/
	*addrargl = 0;
	for (n = 0;;n++) {
		if (!validaddr(getlinear(va.va_seg, va.va_value))) {
			badaddr(va.va_seg, va.va_value);
			ret = DB_ERROR;
			break;
		}
		r = pstun(n, cs, va.va_seg, &va.va_value, followfield, v);
		putchar('\n');
		if (r == PS_STOP)
			break;
		if (followfield) {
			if (r == PS_NOFOLLOW) {
				printf("Couldn't find follow field \"%s\"!\n", followfield);
				ret = DB_ERROR;
				break;
			} else
			if (va.va_seg == initial_seg &&		/* L000 v */
			    va.va_value == initial_value) {
				printf("End of circular list");
				if (n > 1)
					printf(" (after %d entries).\n",n+1);
				else
					printf(".\n");
				break;
			} else					/* L000 ^ */
			if (va.va_value == 0) {
				printf("End of list reached");
				if (n > 1)			/* L000 v */
					printf(" (after %d entries).\n",n+1);
				else
					printf(".\n");		/* L000 ^ */
				break;
			}
		}
		if (!anykeydel("structures"))
			break;
	}
	/* rebuild arg vector */
	*addrargl = addrargs;
	return ret;
}

/*
*	the user may've initialized some of the elements to
*	stun names and addresses, so we try to resolve them
*/
NOTSTATIC
db_stv_init() {
	register int i;
	register int j;
	char *p;
	int n, sz, o;
	char *strt;
	extern int db_stuntablesize;
	extern int db_varitablesize;
	extern char *db_stuntable;
	extern char *db_varitable;

	/*
	 * See of unixsyms placed the information in the kernel
	 */

	if (scodb_kstruct_info) {
		struct scodb_usym_info *infop = (struct scodb_usym_info *)
							scodb_kstruct_info;

		if (infop->magic != USYM_MAGIC) {
			printf("Bad magic number from unixsyms info at %x\n",
				scodb_kstruct_info);
			return;
		}
		if (infop->stun_size) {
			db_stuntable = scodb_kstruct_info + infop->stun_offset;
			db_stuntablesize = infop->stun_size;
		}

		if (infop->vari_size) {
			db_varitable = scodb_kstruct_info + infop->vari_offset;
			db_varitablesize = infop->vari_size;
		}
	}

	/***************************************************
	*	initialize stuns first
	*/
stuns:	{
		if (db_stuntablesize < sizeof(int)) {
		nostun:	nstun = 0;
			goto vars;
		}

		nstun = *(int *)db_stuntable;

		if (db_stuntablesize < (sizeof(int) + nstun*sizeof(struct cstun)))
			goto nostun;

		stun = (struct cstun *)(db_stuntable + sizeof(int));

		for (i = 0;i < nstun;i++) {
			if (stun[i].cs_nameo > db_stuntablesize) {
				if ((nstun = i - 1) < 0)
					nstun = 0;
				break;
			}
			else if (stun[i].cs_nameo == db_stuntablesize) {
				nstun = i;
				break;
			}
			stun[i].cs_names = db_stuntable + stun[i].cs_nameo;
			stun[i].cs_cstel = (struct cstel *)(db_stuntable + stun[i].cs_offset);
			for (j = 0;j < stun[i].cs_nmel;j++)
				stun[i].cs_cstel[j].ce_names = db_stuntable + stun[i].cs_cstel[j].ce_nameo;
		}
		if (nstun != 0)	{
			/* make sure that last one fit correctly */
			if ((stun[nstun - 1].cs_cstel[stun[nstun - 1].cs_nmel - 1].ce_names + NAMEL) > (db_stuntable + db_stuntablesize)) {
				if (--nstun < 0)
					nstun = 0;
			}
		}
	}

vars:	{
		if (db_varitablesize < sizeof(int)) {
		novari:	nvari = 0;
			goto evar;
		}

		nvari = *(int *)db_varitable;

		if (db_varitablesize < (sizeof(int) + nvari*sizeof(struct cvari)))
			goto novari;

		cvari = (struct cvari *)(db_varitable + sizeof(int));

		for (i = 0;i < nvari;i++) {
			if (cvari[i].cv_nameo > db_varitablesize) {
				if ((nvari = i - 1) < 0)
					nvari = 0;
				break;
			}
			else if (cvari[i].cv_nameo == db_varitablesize) {
				nvari = i;
				break;
			}
			cvari[i].cv_names = db_varitable + cvari[i].cv_nameo;
		}
		if (nvari != 0)	{
			/* make sure that last one fit correctly */
			if ((cvari[nvari - 1].cv_names + NAMEL)	> (db_varitable + db_varitablesize)) {
				if (--nvari < 0)
					nvari = 0;
			}
		}
	}
evar:	;
}

NOTSTATIC
pstun(arn, cs, seg, off, followf, onlyv)
	int arn;
	struct cstun *cs;
	long seg, *off;
	char *followf, **onlyv;
{
	int i, n = 1, r, ff = 0;
	long nxoff;
	char **v, bf[9], *sy, *symname();
	struct cstel *ce;

	ce = cs->cs_cstel;
	r = cs->cs_size;
	nxoff = *off + r;
	r = PS_CONT;

	if (arn == 0) {
		pn(bf, cs->cs_size);
		printf("%s %s size = %s bytes\n",
			cs->cs_flags & SUF_STRUCT ? "struct" : "union",
			cs->cs_names,
			bf);
	}
	sy = symname(*off, 1);
	if (sy)
		printf("%s = ", sy);
	printf("%x:\n", *off);					/* L001 */
	for (i = 0;i < cs->cs_nmel;i++, ce++) {
		if (n && n % NLPP == 0 && !anykeydel("structure fields"))
			return PS_STOP;
		if (followf && !strcmp(ce->ce_names, followf)) {
			/*
			*	the field to follow.
			*	get the long at structure offset plus
			*	field offset.
			*	if getlong fails, we'll return `stop'.
			*/
			r = db_getlong(KDSSEL, *off + ce->ce_offset, &nxoff);
			if (r)
				r = PS_CONT;
			else
				r = PS_STOP;
			ff = 1;	/* found follow field */
		}
		if (*onlyv) {
			/* only print field if in `onlyv' */
			for (v = onlyv;*v;v++)
				if (!strcmp(ce->ce_names, *v)) {
					if (!pcstel(PI_STEL|PI_PSYM, ce, seg, *off))
						return PS_STOP;
					++n;
					break;
				}
		}
		else {
			if (!pcstel(PI_STEL|PI_PSYM, ce, seg, *off))
				return PS_STOP;
			++n;
		}
	}
	if (!ff && followf)	/* have a follow field, didn't find */
		r = PS_NOFOLLOW;
	else
		*off = nxoff;
	return r;
}

NOTSTATIC
struct cstun *
findstun(nm)
	char *nm;
{
	int n, r;

	r = ufind(2,
		  nm,
		  (char *)stun,
		  sizeof(stun[0]), 
		  nstun,
		  (int)&stun[0].cs_names - (int)&stun[0],
		  -1,
		  0,
		  &n);
	return (r == FOUND_BP) ? &stun[n] : NULL;
}
