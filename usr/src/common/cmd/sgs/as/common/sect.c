#ident	"@(#)nas:common/sect.c	1.18"
/*
* common/sect.c - common assembler section handling
*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#ifndef __STDC__
#  include <memory.h>
#endif
#include "common/as.h"
#include "common/eval.h"
#include "common/expr.h"
#include "common/objf.h"
#include "common/relo.h"
#include "common/sect.h"
#include "common/stmt.h"
#include "common/syms.h"
#include "align.h"

#include "intemu.h"

#define SECT_ASMGEN	0x01	/* section not available to users */
#define SECT_PREDEF	0x02	/* section predefined */
#define SECT_ATTR	0x04	/* attribute evaluated (as value, not expr) */
#define SECT_TYPE	0x08	/* type evaluated (as value, not expr) */
#define SECT_VARINST	0x10	/* contains variable-sized instruction(s) */

static struct	/* predefined section info */
{
	const char	*name;
	Uchar		flags;
	Ulong		type;
	Ulong		attr;
} const pds[] =	/* first is initial section, in desired order */
{
	{".text",	0,	SecTy_Progbits,	Attr_Alloc | Attr_Exec},
	{".init",	0,	SecTy_Progbits,	Attr_Alloc | Attr_Exec},
	{".fini",	0,	SecTy_Progbits,	Attr_Alloc | Attr_Exec},
	{".data",	0,	SecTy_Progbits,	Attr_Alloc | Attr_Write},
	{".data1",	0,	SecTy_Progbits,	Attr_Alloc | Attr_Write},
	{".rodata",	0,	SecTy_Progbits,	Attr_Alloc},
	{".rodata1",	0,	SecTy_Progbits,	Attr_Alloc},
	{".bss",	0,	SecTy_Nobits,	Attr_Alloc | Attr_Write},
	{".comment",	0,	SecTy_Progbits,	0},
	{".debug",	0,	SecTy_Progbits,	0},
	{".line",	0,	SecTy_Progbits,	0},
	{".note",	0,	SecTy_Note,	0},
	{".shstrtab",	0,	SecTy_Strtab,	0},
	{".dynstr",	0,	SecTy_Strtab,	Attr_Alloc},
	{".dynsym",	0,	SecTy_Dynsym,	Attr_Alloc},
	{".hash",	0,	SecTy_Hash,	Attr_Alloc},
	{".dynamic",	0,	SecTy_Dynamic,	Attr_Alloc | Attr_Write},
	{".got",	0,	SecTy_Progbits,	Attr_Alloc | Attr_Write},
	{".symtab",	SECT_ASMGEN,	SecTy_Symtab,	0},
	{".strtab",	SECT_ASMGEN,	SecTy_Strtab,	0},
	/*
	* ".interp" and ".plt" are not listed as their attributes are
	* not the same for all implementations.
	*/
};

static Section predefs[sizeof(pds) / sizeof(pds[0])];
static Code precodes[sizeof(pds) / sizeof(pds[0])];

static Section *sections = &predefs[0]; /* linked list of all sections */
static Section *lastsect = &predefs[sizeof(pds) / sizeof(pds[0]) - 1];

typedef struct stk Stack;
struct stk	/* stack of sections */
{
	Section	*secp;	/* "current" section at this level */
	Section	*prev;	/* "previous" section at this level */
	Stack	*next;	/* "pop" to this Stack */
};

static Stack initial = {&predefs[0], &predefs[0], 0};
static Stack *current = &initial;

static Symbol *reloc_name;	/* entry for ".<reloc>" */

static const char MSGbig[] = "section too big: %s";

void
#ifdef __STDC__
initsect(void)	/* install all predefined sections */
#else
initsect()
#endif
{
	register Symbol *sp;
	int i;

	for (i = 0; i < sizeof(pds) / sizeof(pds[0]); i++)
	{
		sp = lookup((const Uchar *)pds[i].name,
			(size_t)strlen(pds[i].name));
		sp->sym_sect = &predefs[i];
		predefs[i].sec_next = &predefs[i + 1];
		predefs[i].sec_sym = sp;
		predefs[i].sec_flags = pds[i].flags
			| (SECT_PREDEF | SECT_ATTR | SECT_TYPE);
		predefs[i].attr.sec_value = pds[i].attr;
		predefs[i].type.sec_value = pds[i].type;
		predefs[i].sec_code = &precodes[i];
		predefs[i].sec_last = &precodes[i];
		precodes[i].code_kind = CodeKind_Unset;
	}
	predefs[i - 1].sec_next = 0;
	/*
	* The name for this symbol includes < and > to
	* prevent any conflicts with user-generated names.
	*/
	reloc_name = lookup((const Uchar *)".<reloc>", (size_t)8);
}

static void
#ifdef __STDC__
attrstr_check(Section *secp)	/* validate section attribute string */
#else
attrstr_check(secp)Section *secp;
#endif
{
	Expr *ep = setlessexpr(secp->attr.sec_expr);
	const Uchar *p = ep->right.ex_str;
	size_t len;
	Ulong attr = 0;

	for (len = ep->left.ex_len; len != 0; len--)
	{
		switch (*p++)	/* multiple occurrances of letters okay */
		{
		default:
			break;
		case 'a':
			attr |= Attr_Alloc;
			continue;
		case 'w':
			attr |= Attr_Write;
			continue;
		case 'x':
			attr |= Attr_Exec;
			continue;
		}
		exprerror(secp->attr.sec_expr,
			gettxt(":875","invalid section attribute string: \"%s\""),
			prtstr(ep->right.ex_str, ep->left.ex_len));
		break;
	}
	secp->sec_flags |= SECT_ATTR;
	secp->attr.sec_value = attr;
}

static void
#ifdef __STDC__
typestr_check(Section *secp)	/* validate section type string */
#else
typestr_check(secp)Section *secp;
#endif
{
	Expr *ep = setlessexpr(secp->type.sec_expr);
	const char *p = (const char *)ep->right.ex_str;
	Ulong type = SecTy_Progbits;	/* default section type */

	switch (ep->left.ex_len)
	{
	default:
		p = 0;
		break;
	case 4:
		if (strncmp(p, "note", (size_t)4) == 0)
			type = SecTy_Note;
		else
			p = 0;
		break;
	case 6:
		if (strncmp(p, "strtab", (size_t)6) == 0)
			type = SecTy_Strtab;
		else if (strncmp(p, "symtab", (size_t)6) == 0)
			type = SecTy_Symtab;
		else if (strncmp(p, "nobits", (size_t)6) == 0)
			type = SecTy_Nobits;
		else
			p = 0;
		break;
	case 8:
		if (strncmp(p, "progbits", (size_t)8) == 0)
			break;
		else if (strncmp(p, "delayrel", (size_t)8) == 0)
			type = SecTy_Delay_Rel;
		else
			p = 0;
		break;
	}
	if (p == 0)	/* no match above */
	{
		exprerror(secp->type.sec_expr,
			gettxt(":876","invalid section type string: \"%s\""),
			prtstr(ep->right.ex_str, ep->left.ex_len));
	}
	secp->sec_flags |= SECT_TYPE;
	secp->type.sec_value = type;
}

static Section *
#ifdef __STDC__
newsect(Symbol *sp)	/* allocate new section; must append to list */
#else
newsect(sp)Symbol *sp;
#endif
{
	register struct sectcode /* holds a Section and its initial Code */
	{
		Section	s;
		Code	c;
	} *scp;
	static const Section empty = {0};

#ifdef DEBUG
	if (DEBUG('a') > 0)
	{
		static Ulong total;

		(void)fprintf(stderr,
			"Total struct sectcodes=%lu @%lu ea.\n",
			total += 1, (Ulong)sizeof(struct sectcode));
	}
#endif
	scp = (struct sectcode *)alloc((void *)0, sizeof(struct sectcode));
	scp->s = empty;
	sp->sym_sect = &scp->s;
	scp->s.sec_sym = sp;
	lastsect->sec_next = &scp->s;
	lastsect = &scp->s;
	scp->s.sec_code = &scp->c;
	scp->s.sec_last = &scp->c;
	scp->c.code_addr = 0;
	scp->c.code_kind = CodeKind_Unset;
	return &scp->s;
}

void
#ifdef __STDC__
setsect(Symbol *sp, Expr *attr, Expr *type)	/* define/set section */
#else
setsect(sp, attr, type)Symbol *sp; Expr *attr, *type;
#endif
{
	register Section *secp;

#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "setsect(%s,attr=",
			(const char *)sp->sym_name);
		printexpr(attr);
		(void)fputs(",type=", stderr);
		printexpr(type);
		(void)fputs(")\n", stderr);
	}
#endif
	if ((secp = sp->sym_sect) == 0)
		secp = newsect(sp);
	else if (secp->sec_flags & SECT_ASMGEN)
	{
		error(gettxt(":877","assembler-generated section unavailable: %s"),
			(const char *)sp->sym_name);
		return;
	}
	else if (attr != 0)	/* also a definition */
	{
		if (secp->sec_flags & SECT_PREDEF)	/* allow override */
		{
			secp->sec_flags &=
				~(SECT_PREDEF | SECT_ATTR | SECT_TYPE);
		}
		else
		{
			warn(gettxt(":878","section already defined: %s"),
				(const char *)sp->sym_name);
			return;
		}
	}
	current->prev = current->secp;
	current->secp = secp;
	if (attr == 0)		/* nothing further to do unless definition */
		return;
	if (flags & ASFLAG_TRANSITION && type == 0) /* use obsolete rules */
	{
		secp->sec_flags |= (SECT_ATTR | SECT_TYPE);
		secp->attr.sec_value = 0;
		secp->type.sec_value = SecTy_Progbits;
		obssectattr(secp, attr);
	}
	else	/* check strings now, handle others later */
	{
		secp->attr.sec_expr = attr;
		if (attr->ex_type == ExpTy_String)
			attrstr_check(secp);
		else
			fixexpr(attr);
		if ((secp->type.sec_expr = type) != 0)
		{
			if (type->ex_type == ExpTy_String)
				typestr_check(secp);
			else
				fixexpr(type);
		}
	}
}

Section *
#ifdef __STDC__
relosect(Section *secp, int type) /* create matching relocation section */
#else
relosect(secp, type)Section *secp; int type;
#endif
{
	register Section *new;

#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "relosect(%s,type=%d)\n",
			(const char *)secp->sec_sym->sym_name, type);
	}
#endif
	if (secp->sec_relo != 0)
		fatal(gettxt(":879","relosect():already has relocation section"));
	new = newsect(reloc_name);
	new->sec_relo = secp;
	secp->sec_relo = new;
	new->sec_flags = SECT_ASMGEN | SECT_ATTR | SECT_TYPE;
	new->attr.sec_value = 0;
	new->type.sec_value = type;
	return new;
}

void
#ifdef __STDC__
prevsect(void)	/* toggle between current and previous sections */
#else
prevsect()
#endif
{
	Section *tmp;

	tmp = current->prev;
	current->prev = current->secp;
	current->secp = tmp;
}

Section *
#ifdef __STDC__
cursect(void)	/* return current section */
#else
cursect()
#endif
{
	return current->secp;
}

int
#ifdef __STDC__
stacksect(Section *secp)	/* push/pop current section */
#else
stacksect(secp)Section *secp;
#endif
{
	static Stack *avail;
	register Stack *tmp;

	if (secp == 0)	/* pop */
	{
		if (current->next == 0)
			return 0;
		else
		{
			tmp = current;
			current = current->next;
			tmp->next = avail;
			avail = tmp;
		}
	}
	else	/* push */
	{
		if ((tmp = avail) != 0)
			avail = tmp->next;
		else
		{
#ifdef DEBUG
			if (DEBUG('a') > 0)
			{
				static Ulong total;

				(void)fprintf(stderr,
					"Total Stacks=%lu @%lu ea.\n",
					total += 1, (Ulong)sizeof(Stack));
			}
#endif
			tmp = (Stack *)alloc((void *)0, sizeof(Stack));
		}
		tmp->secp = secp;
		tmp->prev = secp;
		tmp->next = current;
		current = tmp;
	}
	return 1;
}

#ifndef ALIGN_IS_POW2
static Ulong
#ifdef __STDC__
lcm(Ulong u0, Ulong v0)	/* return the least common multiple of u0,v0 */
#else
lcm(u0, v0)Ulong u0, v0;
#endif
{
	register Ulong t, u = u0, v = v0;
	register int neg, nbits;

	/*
	* Adapted from "Algorithm B" from Knuth Vol. 2, 4.5.2.,
	* using a flag instead of negative values.
	*/
	for (nbits = 0; ((u | v) & 0x1) == 0; nbits++)
	{
		u >>= 1;
		v >>= 1;
	}
	if ((neg = u & 0x1) != 0)
		t = v;
	else
		t = u;
	for (;;)
	{
		while ((t & 0x1) == 0)
			t >>= 1;
		if (neg)
			v = t;
		else
			u = t;
		if (u > v)
		{
			t = u - v;
			neg = 0;
		}
		else if (u == v)
			break;	/* only way out */
		else
		{
			t = v - u;
			neg = 1;
		}
	}
	/*
	* At this point, u << nbits is GCD(u0, v0).
	* LCM(u0, v0) is (u0 * v0) / GCD(u0, v0).
	*/
	return (u0 / (u << nbits)) * v0;
}
#endif	/*ALIGN_IS_POW2*/

void
#ifdef __STDC__
recalc_sect_addrs(Section *secp, Code *cp, Ulong sz, char *func_name)
#else
recalc_sect_addrs(secp, cp, sz, func_name)
Section *secp; Code *cp; Ulong sz; char * func_name;
#endif
/*
** Recalculate the Code offsets (code_addr) for each Code in section
** "secp" starting at the Code pointed to by "cp".  "sz" contains the
** section offset (address) for the Code statement at "cp".
** The sec_size is updated upon completion of the walk through the Code
** statements.
**
** This routine is used by setvarsz() in sect.c and is usable from the
** machine dependent portion of the assembler (P5_check_conflict() in
** i386/stmt386.c).  
*/
{
#ifdef DEBUG
	if (DEBUG('v') > 1 || DEBUG('P') > 2)
	{
		(void)fprintf(stderr,
		   "recalc_sect_addrs() for %s:Address changes (start %#lx):\n",
			      func_name, sz);
	}
#endif
	for (;;cp = cp->code_next)	/* update each code_addr */
	{
#ifdef DEBUG
		if (DEBUG('v') > 1)
		{
			const char *str = "?HUH?";

			switch (cp->code_kind)
			{
			case CodeKind_Unset:
				str = "UNSET";
				break;
			case CodeKind_FixInst:
			case CodeKind_VarInst:
				str = (const char *)
					cp->info.code_inst->inst_name;
				break;
			case CodeKind_Expr:
				str = "EXPR";
				break;
			case CodeKind_Align:
				str = "ALIGN";
				break;
			case CodeKind_Backalign:
				str = "BACK";
				break;
			case CodeKind_Skipalign:
				str = "SKIP";
				break;
			case CodeKind_Pad:
				str = "PAD";
				break;
			case CodeKind_Zero:
				str = "ZERO";
				break;
			case CodeKind_String:
				str = "STRING";
				break;
			}
			(void)fprintf(stderr,
				"\tcp=%#lx: %s %lu becomes %lu\n",
				(Ulong)cp, str, cp->code_addr, sz);
		}
#endif
		if (cp->code_addr > sz)	/* check for overflow */
		{
			error(gettxt(":874",MSGbig),
				(const char *)secp->sec_sym->sym_name);
		}
		cp->code_addr = sz;
		switch (cp->code_kind)
		{
		default:
			fatal(gettxt(
		     ":0","recalc_sect_addrs() for %s: unknown code kind: %u"),
			      func_name, (Uint)cp->code_kind);
			/*NOTREACHED*/
		case CodeKind_RelSym:
		case CodeKind_RelSec:
			fatal(gettxt(
		    ":0","recalc_sect_addrs() for %s: bad code kind (reloc)"),
			      func_name);
			/*NOTREACHED*/
		case CodeKind_Unset:
			break;
		case CodeKind_Backalign:
			/*
			* Temporarily reduce the current
			* offset/size/address as if the
			* CodeKind_Skipalign were zero.
			*/
			sz -= cp->info.code_more->
				data.more_code->data.code_skip;
			/*FALLTHROUGH*/
		case CodeKind_Align:
		{
			const Ulong *p = cp->data.code_align;
			Uint i = cp->code_size;
			Ulong align = *p;
			Code *savcp;

			for (;; align = *++p)
			{
#ifdef ALIGN_IS_POW2
				align -= sz & (align - 1);
#else
				align -= sz % align;
#endif
				if (align <= *++p)
					break;	/* padding fits */
				if (--i == 0)
				{
					align = 0;
					break;
				}
			}
			sz += align;
			if (cp->code_kind == CodeKind_Align)
				continue;
			/*
			* Fix up all addresses between the padding
			* code and the directive.  The two loops
			* distinguish between reducing and growing
			* the intermediate addresses.
			*/
			savcp = cp;
			cp = cp->info.code_more->data.more_code;
			if (align > cp->data.code_skip)
			{
				Ulong diff = align - cp->data.code_skip;

				cp->data.code_skip = align;
				do
				{
					cp = cp->code_next;
					cp->code_addr += diff;
				} while (cp->code_kind
					!= CodeKind_Backalign);
			}
			else if (cp->data.code_skip > align)
			{
				Ulong diff = cp->data.code_skip - align;

				cp->data.code_skip = align;
				do
				{
					cp = cp->code_next;
					cp->code_addr -= diff;
				} while (cp->code_kind != CodeKind_Backalign);
			}
			else
			{
				cp = savcp;
			}
			continue;
		}
		case CodeKind_Skipalign:
			sz += cp->data.code_skip;
			continue;	/* really handled at Backalign */
		case CodeKind_Pad:
			sz = cp->info.code_more->data.more_code->code_addr
				+ cp->data.code_skip;
			continue;
		case CodeKind_Zero:
		case CodeKind_String:
			sz += cp->data.code_skip;
			continue;
		case CodeKind_FixInst:
		case CodeKind_VarInst:
		case CodeKind_Expr:
			sz += cp->code_size;
			continue;
		}
		break;	/* only here if is CodeKind_Unset */
	}
	/*
	* Finished.  Set resulting section size.
	*/
	if ((sz = secp->sec_last->code_addr) < secp->sec_size)
		error(gettxt(":874",MSGbig), (const char *)secp->sec_sym->sym_name);
	secp->sec_size = sz;
}

static void
#ifdef __STDC__
setvarsz(Section *secp)	/* choose sizes for varying-sized instructions */
#else
setvarsz(secp)Section *secp;
#endif
{
	register Code *cp;
	register Ulong sz;
	Code *topvar = secp->sec_code;
	Code *topchg;
#ifdef DEBUG
	Ulong init_sec_size;
	int passno = 0;		/* count of passes with changes */

	if (DEBUG('v') > 0)
	{
		(void)fprintf(stderr, "VarSize:section=%s\n",
			(const char *)secp->sec_sym->sym_name);
		init_sec_size = secp->sec_size;
	}
#endif
	/*
	* Walk through the contents of the section repeatedly until the
	* sizes of all CodeKind_VarInst's have stopped changing.
	*
	* This algorithm requires that encodings for instructions never
	* get smaller.  (Otherwise, it might never converge.)  It also
	* relies on the restriction imposed by the .backalign and .set
	* (of .) directives: that they can only be based on a label
	* before or at the current code.  (This means that CodeKind_Pad
	* and CodeKind_Backalign will never reduce the size of a section.)
	*
	* The following is a nonoptimal compromise algorithm that
	* requires far fewer iterations than the optimal algorithm.
	* In practice, it is at least one or two orders of magnitude
	* faster.
	*
	* The algorithm:
	*  1. Find the next varying-sized instruction.
	*  2. Determine the new sizes for all subsequent variable-sized
	*     instructions.  Note that since the addresses are not
	*     changed in this pass, some instructions may become bigger
	*     than may be necessary.
	*  3. Sweep though from just after the first variable-sized
	*     instruction to the end of the section updating addresses.
	*  4. Go back to step 1.
	* Once either step 1 or 2 finds no candidate, the process is
	* complete.
	*/
	for (;;)
	{
		/*
		* 1. Find first CodeKind_VarInst still in section.
		*/
		for (cp = topvar; cp != 0; cp = cp->code_next)
		{
			if (cp->code_kind == CodeKind_VarInst)
				break;
		}
		if ((topvar = cp) == 0)
			break;		/* no more sizes to choose */
		/*
		* 2. For each CodeKind_VarInst, check for a size change,
		*    remembering the first that changed size.
		*/
		topchg = 0;
		do		/* check each VarInst's code_size */
		{
			if (cp->code_kind != CodeKind_VarInst)
				continue;
			if ((sz = (*cp->info.code_inst->inst_gen)(secp, cp))
				== cp->code_size)
			{
				continue;
			}
			if (sz < cp->code_size)
				fatal(gettxt(":880","setvarsz():smaller varinst encoding"));
			if (topchg == 0)	/* first change */
			{
				topchg = cp;
#ifdef DEBUG
				if (DEBUG('v') > 0)
				{
					(void)fprintf(stderr,
						"VarSize:pass %d\n", ++passno);
				}
#endif
			}
#ifdef DEBUG
			if (DEBUG('v') > 1)
			{
				const Inst *ip = cp->info.code_inst;

				(void)fprintf(stderr,
					"\tcp=%#lx: %s %lu[%u] -> [%lu]\n",
					(Ulong)cp, (const char *)ip->inst_name,
					cp->code_addr, (Uint)cp->code_size, sz);
			}
#endif
			if ((cp->code_size = sz) != sz)
				fatal(gettxt(":881","setvarsz():varinst encoding too big"));
		} while ((cp = cp->code_next) != 0);
		/*
		* 3. Starting with the first CodeKind_VarInst that grew,
		*    update all the addresses.  Alignment and other padding
		*    are the tricky ones here.
		*/
		if ((cp = topchg) == 0)
			break;		/* no size changes */
		sz = cp->code_addr + cp->code_size;

		recalc_sect_addrs(secp, cp->code_next, sz, "setvarsz()");
	}  /* for */
#ifdef DEBUG
	if (DEBUG('v') > 0)
	{
		(void)fprintf(stderr,
			"VarSize:%s grows from %lu to %lu bytes\n",
			(const char *)secp->sec_sym->sym_name,
			init_sec_size, secp->sec_size);
	}
#endif
}

void
#ifdef __STDC__
walksect(void)	/* preliminary scan through all sections */
#else
walksect()
#endif
{
	register Section *secp;
	size_t nsects = 0;	/* number of sections with content */
	size_t nschars = 0;	/* amount of additional strtab space */

	/*
	* Give each section an index and handle variable-sized
	* instructions here.
	*/
	secp = sections;
	do
	{
		if (secp->sec_code == secp->sec_last)
			continue;
		secp->sec_index = ++nsects;
		nschars += secp->sec_sym->sym_nlen + 1;
		if (secp->sec_flags & SECT_VARINST)
			setvarsz(secp);	/* choose sizes for varinsts */
#ifdef P5_ERR_41
		if (secp->attr.sec_value & Attr_Exec) {
			while (pentium_bug(secp)) {
				setvarsz(secp);
			}  /* while */
		}  /* if */
#endif
	} while ((secp = secp->sec_next) != 0);
	objfmksect(nsects, nschars);	/* prime object file sections */
}

static void
#ifdef __STDC__
attr_check(Section *secp) /* validate section attribute value/string */
#else
attr_check(secp)Section *secp;
#endif
{
	Ulong attr = 0;		/* default attributes */
	Eval *vp;
	Expr *ep;

	if ((ep = secp->attr.sec_expr) != 0)
	{
		if (ep->ex_type == ExpTy_String)
		{
			attrstr_check(secp);
			return;
		}
		if (ep->ex_type != ExpTy_Integer)
		{
			fatal(gettxt(":884","attr_check():non-str/int section attr: %s"),
				(const char *)secp->sec_sym->sym_name);
		}
		vp = evalexpr(ep);
		/*
		* No need for a reevaluation check as all addresses have
		* been fixed by now.
		*/
		if (vp->ev_flags & (EV_OFLOW | EV_TRUNC))
		{
			exprerror(ep, gettxt(":885","section attribute out of range: %s"),
				num_tohex(vp->ev_int));
		}
		attr = vp->ev_ulong;
	}
	secp->sec_flags |= SECT_ATTR;
	secp->attr.sec_value = attr;
}

static void
#ifdef __STDC__
type_check(Section *secp) /* validate section type value/string */
#else
type_check(secp)Section *secp;
#endif
{
	Ulong type = SecTy_Progbits;	/* default section type */
	Eval *vp;
	Expr *ep;

	if ((ep = secp->type.sec_expr) != 0)
	{
		if (ep->ex_type == ExpTy_String)
		{
			typestr_check(secp);
			return;
		}
		if (ep->ex_type != ExpTy_Integer)
		{
			fatal(gettxt(":886","type_check():non-str/int section type: %s"),
				(const char *)secp->sec_sym->sym_name);
		}
		vp = evalexpr(ep);
		/*
		* No need for a reevaluation check as all addresses have
		* been fixed by now.
		*/
		if (vp->ev_flags & (EV_OFLOW | EV_TRUNC))
		{
			exprerror(ep, gettxt(":887","section type out of range: %s"),
				num_tohex(vp->ev_int));
		}
		type = vp->ev_ulong;
	}
	secp->sec_flags |= SECT_TYPE;
	secp->type.sec_value = type;
}

static void
#ifdef __STDC__
gencode(Section *secp)	/* generate one section's content */
#else
gencode(secp)Section *secp;
#endif
{
	static const char MSGzby[]
		= "section contains only zero-valued bytes: %s";
	register Code *cp;
	register Uchar *base;
	register int exec = 0;
	register int nobits = 0;
	Eval *vp;
	Ulong fno, lno;

	/*
	* Walk through all data for the section.
	* If this item is an instruction, hand it off to the
	* appropriate encoding function.  If a section is
	* executable, pad with nop's instead of zeroes.
	*/
	if (secp->attr.sec_value & Attr_Exec)
		exec = 1;
	if (secp->type.sec_value == SecTy_Nobits)
		nobits = 1;
	else
	{
		base = (Uchar *)alloc((void *)0, secp->sec_size);
		secp->sec_data = base;
	}
	for (cp = secp->sec_code;; cp = cp->code_next)
	{
		switch (cp->code_kind)
		{
		default:
			fatal(gettxt(":888","gencode():unknown code kind: %u"),
				(Uint)cp->code_kind);
			/*NOTREACHED*/
		case CodeKind_RelSym:
		case CodeKind_RelSec:
			fatal(gettxt(":889","gencode():inappropriate code kind (reloc): %u"),
				(Uint)cp->code_kind);
			/*NOTREACHED*/
		case CodeKind_Unset:
			return;		/* ends list of contents */
		case CodeKind_FixInst:
		case CodeKind_VarInst:
			if (nobits)	/* assume not all zero bytes */
			{
				fno = cp->data.code_olst->olst_file;
				lno = cp->data.code_olst->olst_line;
				break;
			}
			if ((*cp->info.code_inst->inst_gen)(secp, cp)
				!= cp->code_size)
			{
				fatal(gettxt(":890","gencode():\"%s\" not expected size: %u"),
					(const char *)cp->info.code_inst
						->inst_name,
					(Uint)cp->code_size);
			}
			continue;
		case CodeKind_Expr:
			vp = evalexpr(cp->data.code_expr);
			if (vp->ev_flags & EV_RELOC)
			{
				if (nobits)
				{
					exprfrom(&fno, &lno, cp->data.code_expr);
					break;
				}
				relocexpr(vp, cp, secp);
				continue;
			}
			switch (cp->info.code_form)
			{
			case CodeForm_Float:
			case CodeForm_Double:
			case CodeForm_Extended:
				flt_todata(vp, cp, secp);
				break;
			default:
				int_todata(vp, cp, secp);
				break;
			}
			continue;
		case CodeKind_String:
			if (nobits)	/* assume not all zero...could check */
			{
				fno = cp->info.code_more->more_file;
				lno = cp->info.code_more->more_line;
				break;
			}
			(void)memcpy((void *)(base + cp->code_addr),
				(const void *)cp->info.code_more->data.more_str,
				(size_t)cp->data.code_skip);
			continue;
		case CodeKind_Backalign:
			continue;	/* CodeKind_Skipalign already done */
		case CodeKind_Pad:
			/*
			* It is possible, due to variable-sized instructions,
			* to have a .set of . move backwards.  However, this
			* is very unlikely.  The following check is placed
			* here [instead of in setvarsz()] because some
			* subsequent step might fix a temporary overlap
			* condition.
			*/
			if (cp->code_addr > cp->code_next->code_addr)
			{
				backerror((Ulong)cp->info.code_more->more_file,
					cp->info.code_more->more_line,
					gettxt(":891","backward .set of . causes overlap"));
				continue;
			}
			/*FALLTHROUGH*/
		case CodeKind_Align:
			/*
			* By now, the aligning has been finished.  The
			* number of bytes to pad is the difference between
			* the address of this code and the next.  Overwrite
			* code_skip with this difference.
			*/
			if ((cp->data.code_skip = cp->code_next->code_addr
				- cp->code_addr) == 0)
			{
				continue;
			}
			/*FALLTHROUGH*/
		case CodeKind_Skipalign:
			if (exec)
			{
				if (nobits)	/* "nop"s probably not 0 */
				{
					fno = cp->info.code_more->more_file;
					lno = cp->info.code_more->more_line;
					break;
				}
				gennops(secp, cp, cp->info.code_more->more_prev);
				continue;
			}
			/*FALLTHROUGH*/
		case CodeKind_Zero:
			if (!nobits)
			{
				(void)memset((void *)(base + cp->code_addr),
					0, (size_t)cp->data.code_skip);
			}
			continue;
		}
		/*
		* Only here when attempting to generate (possibly)
		* nonzero byte(s) for nobits section.
		*/
		backerror(fno, lno, gettxt(":789",MSGzby),
			(const char *)secp->sec_sym->sym_name);
	}
}

void
#ifdef __STDC__
gensect(void)	/* generate contents of all sections */
#else
gensect()
#endif
{
	register Section *secp;

	/*
	* For each user section:
	* 1. Finalize its section type and attributes.
	* 2. Update the minimum alignments.
	* 3. Allocate (if not nobits) and fill in data.
	* 4. Hand the result to the object file producer.
	*/
	secp = sections;
	do
	{
		if (secp->sec_index == 0)
			continue;
		if (!(secp->sec_flags & SECT_ATTR))
			attr_check(secp);
		if (!(secp->sec_flags & SECT_TYPE))
			type_check(secp);
#if EXEC_SECT_ALIGN > 1
		if (secp->attr.sec_value & Attr_Exec)
		{
#  ifdef ALIGN_IS_POW2
			if (secp->sec_align < EXEC_SECT_ALIGN)
				secp->sec_align = EXEC_SECT_ALIGN;
#  else
			if (secp->sec_align % EXEC_SECT_ALIGN != 0)
			{
				secp->sec_align = lcm(secp->sec_align,
					EXEC_SECT_ALIGN);
			}
#  endif
		}
#endif	/*EXEC_SECT_ALIGN*/
#if ALLO_SECT_ALIGN > 1
		if (secp->attr.sec_value & Attr_Alloc)
		{
#  ifdef ALIGN_IS_POW2
			if (secp->sec_align < ALLO_SECT_ALIGN)
				secp->sec_align = ALLO_SECT_ALIGN;
#  else
			if (secp->sec_align % ALLO_SECT_ALIGN != 0)
			{
				secp->sec_align = lcm(secp->sec_align,
					ALLO_SECT_ALIGN);
			}
#  endif
		}
#endif	/*ALLO_SECT_ALIGN*/
		gencode(secp);
		objfsection(secp);
	} while ((secp = secp->sec_next) != 0);
}

static void
#ifdef __STDC__
newcode(Section *secp)	/* allocate new code for section */
#else
newcode(secp)Section *secp;
#endif
{
	static Code *avail, *endavail;
	register Code *cp;

	/*
	* The "next" code for each section is always pointed to by
	* its current sec_last pointer.  This allows the value for
	* labels and . to be produced before the corresponding Code
	* has been filled.
	*/
#ifndef ALLOC_CODE_CHUNK
#   define ALLOC_CODE_CHUNK 2000
#endif
	if ((cp = avail) == endavail)
	{
#ifdef DEBUG
		if (DEBUG('a') > 0)
		{
			static Ulong total;

			(void)fprintf(stderr, "Total Codes=%lu @%lu ea.\n",
				total += ALLOC_CODE_CHUNK,
				(Ulong)sizeof(Code));
		}
#endif
		avail = cp = (Code *)alloc((void *)0,
			ALLOC_CODE_CHUNK * sizeof(Code));
		endavail = cp + ALLOC_CODE_CHUNK;
	}
	avail = cp + 1;
	cp->code_next = 0;
	cp->code_addr = secp->sec_size;
	cp->code_kind = CodeKind_Unset;
	cp->code_impdep = 0;
	secp->sec_prev = secp->sec_last;
	secp->sec_last->code_next = cp;
	secp->sec_last = cp;
}

static More *
#ifdef __STDC__
allomore(Section *secp)	/* allocate a new More adjunct structure */
#else
allomore(secp)Section *secp;
#endif
{
	static More *avail, *endavail;
	register More *mp;

#ifndef ALLOC_MORE_CHUNK
#   define ALLOC_MORE_CHUNK 100
#endif
	if ((mp = avail) == endavail)
	{
#ifdef DEBUG
		if (DEBUG('a') > 0)
		{
			static Ulong total;

			(void)fprintf(stderr, "Total Mores=%lu @%lu ea.\n",
				total += ALLOC_MORE_CHUNK,
				(Ulong)sizeof(More));
		}
#endif
		avail = mp = (More *)alloc((void *)0,
			ALLOC_MORE_CHUNK * sizeof(More));
		endavail = mp + ALLOC_MORE_CHUNK;
	}
	avail = mp + 1;
	mp->more_prev = secp->sec_prev;
	mp->more_file = curfileno;
	mp->more_line = curlineno;
	return mp;
}

void
#ifdef __STDC__
sectalign(register Section *secp, const Ulong *pair, Uint npair) /* align */
#else
sectalign(secp, pair, npair)register Section *secp; Ulong *pair; Uint npair;
#endif
{
	register Code *cp;
	register Ulong rem, align;

	if (secp == 0)
		secp = current->secp;
#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		Uint i;

		(void)fprintf(stderr, "sectalign(%s,align/max=",
			(const char *)secp->sec_sym->sym_name);
		for (i = 0; i < 2 * npair; i += 2)
		{
			(void)fprintf(stderr, "%lu/%lu,",
				pair[i], pair[i + 1]);
		}
		(void)fprintf(stderr, "npair=%u)\n", npair);
	}
#endif
	cp = secp->sec_last;
	secp->sec_lastpad = cp;
	cp->code_kind = CodeKind_Align;
	cp->data.code_align = pair;
	if ((cp->code_size = npair) != npair)
		fatal(gettxt(":892","sectalign():too many align/max pairs: %u"), npair);
	cp->info.code_more = allomore(secp);
	align = *pair;
#ifdef ALIGN_IS_POW2
	if (secp->sec_align < align)	/* only need to check the first */
		secp->sec_align = align;
#endif
	for (;; align = *++pair)
	{
		rem = align;
#ifdef ALIGN_IS_POW2
		rem -= secp->sec_size & (align - 1);
#else
		rem -= secp->sec_size % align;
		if (secp->sec_align % align != 0)	/* check each one */
			secp->sec_align = lcm(secp->sec_align, align);
#endif
		if (rem <= *++pair)
			break;	/* padding fits within max-fill */
		if (--npair == 0)
		{
			rem = 0;
			break;
		}
	}
	if ((secp->sec_size += rem) < rem)
		error(gettxt(":874",MSGbig), (const char *)secp->sec_sym->sym_name);
	newcode(secp);
}

void
#ifdef __STDC__
sectbackalign(register Section *secp,	/* align w/pad at previous label */
	const Symbol *sp,
	const Ulong *pair,
	Uint npair)
#else
sectbackalign(secp, sp, pair, npair)
	register Section *secp; Symbol *sp; Ulong *pair; Uint npair;
#endif
{
	register Code *cp;
	register Ulong rem, align;
	More *mp;
	Expr *ep;

	if (secp == 0)
		secp = current->secp;
#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		Uint i;

		(void)fprintf(stderr, "sectbackalign(%s,%s,align/max=",
			(const char *)secp->sec_sym->sym_name,
			(const char *)sp->sym_name);
		for (i = 0; i < 2 * npair; i += 2)
		{
			(void)fprintf(stderr, "%lu/%lu,",
				pair[i], pair[i + 1]);
		}
		(void)fprintf(stderr, "npair=%u)\n", npair);
	}
#endif
	if ((ep = sp->addr.sym_expr) == 0 || ep->ex_op != ExpOp_LeafCode
		|| (cp = ep->right.ex_code) == 0)
	{
		fatal(gettxt(":893","sectbackalign():bogus label: %s"),
			(const char *)sp->sym_name);
	}
	if (secp->sec_lastpad->code_addr > cp->code_addr
		|| secp->sec_lastpad == cp)
	{
		error(gettxt(":894","Padding is not permitted between .backalign and %s"),
			(const char *)sp->sym_name);
		return;
	}
	if (cp == secp->sec_last)	/* label points to .backalign! */
	{
		sectalign(secp, pair, npair);	/* regular .align code instead */
		return;
	}
	/*
	* Add another code to the end of the list.  Copy the current
	* code at the label into the new list end and insert it after
	* the labeled code.  Change the labeled code into a back-set
	* padding code.
	*/
	newcode(secp);
	*secp->sec_last = *cp;
	if (cp->code_kind == CodeKind_VarInst
		|| cp->code_kind == CodeKind_FixInst)	/* fix Oplist pointer */
	{
		cp->data.code_olst->olst_code = secp->sec_last;
		/* also fix any LeafCode ex_code pointer */
		movopexcode(cp->data.code_olst, secp->sec_last, cp);
	}
	else if (cp->code_kind == CodeKind_Expr)
	{
		/* fix any LeafCode ex_code pointer */
		movexcode(cp->data.code_expr, secp->sec_last, cp);
	}
#ifdef P5_ERR_41
	/* Reset any specific machine flags, but not any flags that indicate
	   that this statement is tagged with a label.
	*/
	cp->code_impdep &= ~(CODE_P5_0F_NOT_8X | CODE_PREFIX_PSEUDO_OP);
#endif
	cp->code_next = secp->sec_last;
	cp->code_kind = CodeKind_Skipalign;
	cp->info.code_more = mp = allomore(secp);	/* shared */
	mp->more_prev = 0;		/* sorry, can't find previous cheaply */
	mp->data.more_code = cp;
	cp = secp->sec_prev;	/* still CodeKind_Unset */
	secp->sec_last = cp;
	secp->sec_lastpad = cp;
	cp->code_kind = CodeKind_Backalign;
	cp->data.code_align = pair;
	if ((cp->code_size = npair) != npair)
		fatal(gettxt(":895","sectbackalign():too many align/max pairs: %u"), npair);
	cp->info.code_more = mp;
	align = *pair;
#ifdef ALIGN_IS_POW2
	if (secp->sec_align < align)	/* only need to check the first */
		secp->sec_align = align;
#endif
	for (;; align = *++pair)
	{
		rem = align;
#ifdef ALIGN_IS_POW2
		rem -= secp->sec_size & (align - 1);
#else
		rem -= secp->sec_size % align;
		if (secp->sec_align % align != 0)	/* check each one */
			secp->sec_align = lcm(secp->sec_align, align);
#endif
		if (rem <= *++pair)
			break;	/* padding fits within max-fill */
		if (--npair == 0)
		{
			rem = 0;
			break;
		}
	}
	cp = mp->data.more_code;
	if ((cp->data.code_skip = rem) != 0)	/* fix subsequent addresses */
	{
		do	/* all intermediate codes currently have fixed sizes */
		{
			cp = cp->code_next;
			cp->code_addr += rem;
		} while (cp->code_kind != CodeKind_Backalign);
		if ((secp->sec_size += rem) < rem)
			error(gettxt(":874",MSGbig), (const char *)secp->sec_sym->sym_name);
	}
	newcode(secp);
}

int
#ifdef __STDC__
sectoptalign(register Section *secp, const Ulong *al) /* test alignment and fix */
#else
sectoptalign(secp, al)register Section *secp; Ulong *al;
#endif
{
	register Code *cp;
	register Ulong align;
	int currently_aligned = 0;	/* default to "no" */

	if (secp == 0)
		secp = current->secp;
#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "sectoptalign(%s,align/max=%lu/%lu)\n",
			(const char *)secp->sec_sym->sym_name, al[0], al[1]);
	}
#endif
	cp = secp->sec_last;	/* probably won't be used */
	secp->sec_lastpad = cp;
	align = al[0];
#ifdef ALIGN_IS_POW2
	if (secp->sec_align < align)
		secp->sec_align = align;
	if ((secp->sec_size & (align - 1)) == 0)
	{
		if (!(secp->sec_flags & SECT_VARINST))
			return 1;	/* no need for code */
		currently_aligned = 1;
	}
#else
	if (secp->sec_align % align != 0)
		secp->sec_align = lcm(secp->sec_align, align);
	if (secp->sec_size % align == 0)
	{
		if (!(secp->sec_flags & SECT_VARINST))
			return 1;	/* no need for code */
		currently_aligned = 1;
	}
#endif
	cp->code_kind = CodeKind_Align;
	cp->data.code_align = al;
	cp->code_size = 1;
	cp->info.code_more = allomore(secp);
#ifdef ALIGN_IS_POW2
	align -= secp->sec_size & (al[0] - 1);
#else
	align -= secp->sec_size % al[0];
#endif
	if (align <= al[1] && (secp->sec_size += align) < align)
		error(gettxt(":874",MSGbig), (const char *)secp->sec_sym->sym_name);
	newcode(secp);
	return currently_aligned;
}

void
#ifdef __STDC__
sectzero(register Section *secp, Ulong size)	/* add zero-valued bytes */
#else
sectzero(secp, size)register Section *secp; Ulong size;
#endif
{
	register Code *cp;

	if (secp == 0)
		secp = current->secp;
#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "sectzero(%s,size=%lu)\n",
			(const char *)secp->sec_sym->sym_name, size);
	}
#endif
	cp = secp->sec_last;
	cp->code_kind = CodeKind_Zero;
	cp->data.code_skip = size;
	if ((secp->sec_size += size) < size)
		error(gettxt(":874",MSGbig), (const char *)secp->sec_sym->sym_name);
	newcode(secp);
}

void
#ifdef __STDC__
sectpad(register Section *secp, register Eval *addr) /* add padding bytes */
#else
sectpad(secp, addr)register Section *secp; register Eval *addr;
#endif
{
	register Code *cp;
	More *mp;

	if (secp == 0)
		secp = current->secp;
#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr,
			"sectpad(%s,addr->ev_dot=%#lx/_ulong=%lu)\n",
			(const char *)secp->sec_sym->sym_name,
			(Ulong)addr->ev_dot, addr->ev_ulong);
	}
#endif
	cp = secp->sec_last;
	secp->sec_lastpad = cp;
	cp->code_kind = CodeKind_Pad;
	secp->sec_size = addr->ev_ulong;	/* new value for . */
	cp->info.code_more = mp = allomore(secp);
	if ((mp->data.more_code = addr->ev_dot) == 0)
		fatal(gettxt(":896","sectpad():null ev_dot"));
	cp->data.code_skip = addr->ev_ulong - addr->ev_dot->code_addr;
	newcode(secp);
}

#ifdef P5_ERR_41
void
#ifdef __STDC__
sectinsertpad(register Section *secp, Code *after_cp, Ulong bytes)
#else
sectinsertpad(secp, after_cp, bytes)register Section *secp; Code * after_cp;
Ulong bytes;
#endif
/* Insert "bytes" bytes of padding after the code statement pointed to
   by "after_cp".
*/
{
	register Code *work_cp;
	Code * prev_cp;

#ifdef DEBUG
	if (DEBUG('P') > 0)
	{
		(void)fprintf(stderr,
			"sectinsertpad(%s, at loc=%#lx, bytes=%lu)\n",
			(const char *)secp->sec_sym->sym_name,
			      (after_cp ? after_cp->code_next->code_addr : 0)
			      , bytes);
	}
#endif
	if (   after_cp
	    && after_cp->code_kind == CodeKind_Pad ) {
		/* Simply increase the padding count of the existing 
		   CodeKind_Pad instruction.  CC effecting code 
		   code generation can remain as it already is. */
#ifdef DEBUG
		if (DEBUG('P') > 1) {
			(void) fprintf(stderr,
				       "sectinsertpad: combining with previous"
				       " Pad of %d bytes\n",
				       after_cp->data.code_skip);
		}  /* if */
#endif
		after_cp->data.code_skip += bytes;
	} else {
		/* Insert a Pad instruction after the designated instruction
		   or at the beginning of the section if NULL. */
		prev_cp = secp->sec_prev;	/* save for continuity */
		/* Add another code to the end of the list.  Use this to
		   construct the CodeKind_Pad to be inserted following
		   after_cp. */
		newcode(secp);
		work_cp = secp->sec_last;
		if (after_cp) {
			work_cp->code_next = after_cp->code_next;
			after_cp->code_next = work_cp;
		} else {
			work_cp->code_next = secp->sec_code;
			secp->sec_code = work_cp;
		}  /* if */

		work_cp->code_addr = work_cp->code_next->code_addr;
		work_cp->code_kind = CodeKind_Pad;
		work_cp->info.code_more = allomore(secp);
		work_cp->info.code_more->data.more_code = work_cp;
		work_cp->data.code_skip = bytes;
		work_cp->code_impdep |= CODE_NO_CC_PAD;

		secp->sec_last = secp->sec_prev;
		secp->sec_prev = prev_cp;		/* saved earlier */
		secp->sec_last->code_next = 0;
	}  /* if */
}
#endif

void
#ifdef __STDC__
sectexpr(register Section *secp, Expr *ep, Ulong size, int form) /* add expr */
#else
sectexpr(secp, ep, size, form)
	register Section *secp; Expr *ep; Ulong size; int form;
#endif
{
	register Code *cp;

	if (secp == 0)
		secp = current->secp;
#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "sectexpr(%s,ep=",
			(const char *)secp->sec_sym->sym_name);
		printexpr(ep);
		(void)fprintf(stderr, ",size=%lu)\n", size);
	}
#endif
	cp = secp->sec_last;
	cp->code_kind = CodeKind_Expr;
	cp->data.code_expr = ep;
	cp->info.code_form = form;
	if ((secp->sec_size += size) < size)
		error(gettxt(":874",MSGbig), (const char *)secp->sec_sym->sym_name);
	cp->code_size = size;	/* assumed to fit */
	newcode(secp);
}

void
#ifdef __STDC__
sectstr(register Section *secp, Expr *ep)	/* add string value */
#else
sectstr(secp, ep)register Section *secp; Expr *ep;
#endif
{
	register Code *cp;
	More *mp;

	if (secp == 0)
		secp = current->secp;
#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "sectstr(%s,ep=",
			(const char *)secp->sec_sym->sym_name);
		printexpr(ep);
		(void)fputs(")\n", stderr);
	}
#endif
	cp = secp->sec_last;
	cp->code_kind = CodeKind_String;
	cp->data.code_skip = ep->left.ex_len;
	cp->info.code_more = mp = allomore(secp);
	mp->data.more_str = ep->right.ex_str;
	if ((secp->sec_size += cp->data.code_skip) < cp->data.code_skip)
		error(gettxt(":874",MSGbig), (const char *)secp->sec_sym->sym_name);
	newcode(secp);
}

static Code *
#ifdef __STDC__
sectinst(register Section *secp, register const Inst *ip, Oplist *olp)
#else
sectinst(secp, ip, olp)register Section *secp; register Inst *ip; Oplist *olp;
#endif
{
	register Code *cp;

	cp = secp->sec_last;
	cp->info.code_inst = ip;
	if (olp == 0)		/* check for operand-less instruction */
		olp = oplist((Oplist *)0, (Operand *)0);
	cp->data.code_olst = olp;
	if ((secp->sec_size += ip->inst_minsz) < ip->inst_minsz)
		error(gettxt(":874",MSGbig), (const char *)secp->sec_sym->sym_name);
	cp->code_size = ip->inst_minsz;
	olp->olst_code = cp;
	newcode(secp);
#ifdef P5_ERR_41
	chk_P5_0F_issues(cp);	/* look for 0f !8x instructions and calls */
#endif
	return cp;
}

void
#ifdef __STDC__
sectfinst(Section *secp, const Inst *ip, Oplist *olp)	/* add fixed-inst */
#else
sectfinst(secp, ip, olp)Section *secp; Inst *ip; Oplist *olp;
#endif
{
	Code *cp;

	if (secp == 0)
		secp = current->secp;
#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "sectfinst(%s,%s,olp=",
			(const char *)secp->sec_sym->sym_name,
			(const char *)ip->inst_name);
		printoplist(olp);
		(void)fputs(")\n", stderr);
	}
#endif
	cp = sectinst(secp, ip, olp);
	cp->code_kind = CodeKind_FixInst;
}

void
#ifdef __STDC__
sectvinst(Section *secp, const Inst *ip, Oplist *olp)	/* add var-inst */
#else
sectvinst(secp, ip, olp)Section *secp; Inst *ip; Oplist *olp;
#endif
{
	Code *cp;

	if (secp == 0)
		secp = current->secp;
#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "sectvinst(%s,%s,olp=",
			(const char *)secp->sec_sym->sym_name,
			(const char *)ip->inst_name);
		printoplist(olp);
		(void)fputs(")\n", stderr);
	}
#endif
	cp = sectinst(secp, ip, olp);
	cp->code_kind = CodeKind_VarInst;
	secp->sec_flags |= SECT_VARINST;
}

	/*
	* sectrelsec() and sectrelsym() differ from the rest of the
	* Code-filling functions in a number of ways.  These two are
	* only called to fill in the contents of relocation sections.
	* These contents are then only rescanned by common/objf.c
	* instead of here.  Moreover, some of the structure members
	* are used in differing ways:
	*
	*	info.code_{sec,sym}	base for the entry
	*	code_size		the relocation type
	*	code_addr		relocation to be applied here
	*	data.{setadd,addend}	optional addend for relocation
	*
	*	sec_size		number of relocation entries
	*/
Code *
#ifdef __STDC__
sectrelsec(register Section *rsp, int rty, Ulong addr, Section *base)
#else
sectrelsec(rsp, rty, addr, base)
	register Section *rsp; int rty; Ulong addr; Section *base;
#endif
{
	register Code *cp;

#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "sectrelsec(%s,rty=%d,addr=%#lx,%s)\n",
			(const char *)rsp->sec_sym->sym_name, rty, addr,
			(const char *)base->sec_sym->sym_name);
	}
#endif
	cp = rsp->sec_last;
	cp->code_kind = CodeKind_RelSec;
	cp->info.code_sec = base;
	cp->code_size = rty;	/* sorry about using code_size for this */
	rsp->sec_size++;	/* number of entries, not the size */
	newcode(rsp);
	cp->code_addr = addr;	/* overwrite newcode()'s code_addr value */
	return cp;
}

Code *
#ifdef __STDC__
sectrelsym(register Section *rsp, int rty, Ulong addr, Symbol *base)
#else
sectrelsym(rsp, rty, addr, base)
	register Section *rsp; int rty; Ulong addr; Symbol *base;
#endif
{
	register Code *cp;

#ifdef DEBUG
	if (DEBUG('c') > 0)
	{
		(void)fprintf(stderr, "sectrelsym(%s,rty=%d,addr=%#lx,%s)\n",
			(const char *)rsp->sec_sym->sym_name, rty, addr,
			(const char *)base->sym_name);
	}
#endif
	cp = rsp->sec_last;
	cp->code_kind = CodeKind_RelSym;
	cp->info.code_sym = base;
	cp->code_size = rty;	/* sorry about using code_size for this */
	rsp->sec_size++;	/* number of entries, not the size */
	newcode(rsp);
	cp->code_addr = addr;	/* overwrite newcode()'s code_addr value */
	return cp;
}
