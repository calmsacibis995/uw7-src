#ident	"@(#)nas:common/syms.c	1.12"
/*
* common/syms.c - common assembler symbol handling
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "common/as.h"
#include "common/eval.h"
#include "common/expr.h"
#include "common/objf.h"
#include "common/sect.h"
#include "common/syms.h"
#include "align.h"	/* default bss/common symbol alignment */

#include "intemu.h"

	/*
	* Bits for sym_flags.
	*/
#define SYM_BOUND	0x01	/* binding fixed */
#define SYM_SIZED	0x02	/* size fixed */
#define SYM_TYPED	0x04	/* type fixed */
#define SYM_OLDSET	0x08	/* bypassed .set symbol */
#define SYM_VALSIZE	0x10	/* size is now value */
#define SYM_VALTYPE	0x20	/* type is now value */
#define SYM_VALALIGN	0x40	/* alignment is now value */

static Section nosect;	/* only used to define symbols without sections */
#define NOSECT (&nosect)

	/*
	* The initial allocation of symbols is 500.  Assuming no
	* more than one extra allocation of symbols (total of 1000),
	* and since a reasonable hashing algorithm needs to have
	* about 1/3 of the array empty, 1000/2*3 -> 1500 and the
	* closest prime number is 1499.
	*/
static struct symtree	/* look up trees for symbols */
{
	Symbol		sym;	/* must be first member of structure */
	struct symtree	*next;	/* singly-linked list of symbols */
	struct symtree	*left;
	struct symtree	*right;
	struct symtree	**parent;	/* only needed for redefined .set's */
} *buckets[1499];
#ifdef DEBUG
   static size_t buckuse[sizeof(buckets) / sizeof(buckets[0])]; /* histogram */
#endif
#define HIGH5_BITS ((~(Ulong)0) & ~((~(Ulong)0) >> 5))	/* high 5 bit mask */
#define HIGH5_DIST (CHAR_BIT * sizeof(Ulong) - 5)	/* low order bits */

static struct symtree *symbols;	/* heads linked list of all symbols */

static const char MSGmlt[] = "%s for name multiply specified: %s";
static const char MSGdef[] = "name multiply defined: %s";

static Symbol *bss_symbol;	/* .bss symbol entry */
static Expr *expr_object;	/* ulongexpr((Ulong)SymTy_Object) */

static int
#ifdef __STDC__
defnsym(register Symbol *sp, Section *secp) /* symbol defined "now" in sect */
#else
defnsym(sp, secp)register Symbol *sp; Section *secp;
#endif
{
	if (sp->sym_defn != 0)
	{
		error(gettxt(":1015",MSGdef), (const char *)sp->sym_name);
		return 0;
	}
	sp->sym_defn = secp;
	sp->sym_line = curlineno;
	sp->sym_file = curfileno;
	sp->sym_refd = 1;
	return 1;
}

void
#ifdef __STDC__
initsyms(void)	/* set up predefined symbols */
#else
initsyms()
#endif
{
#ifndef GOT_NAME
#   define GOT_NAME "_GLOBAL_OFFSET_TABLE_"
#endif
	static const Uchar MSGdot[] = ".";
	static const Uchar MSGgot[] = GOT_NAME;
	static const Uchar MSGbss[] = ".bss";
	register Symbol *sp;

	/*
	* The symbol . is the name for the current address
	* in the current section.  It cannot be placed into
	* the symbol table and its value is instantiated
	* when an expression is fixed to a location [fixexpr()].
	*/
	sp = lookup(MSGdot, sizeof(MSGdot) - 1);
	(void)defnsym(sp, NOSECT);
	sp->sym_exty = ExpTy_Relocatable;
	sp->sym_kind = SymKind_Dot;
	sp->sym_bind = Bind_Temporary;
	sp->sym_flags |= (SYM_BOUND | SYM_SIZED | SYM_TYPED);
	/*
	* _GLOBAL_OFFSET_TABLE_, by default, is the address of
	* the ld-created table of addresses for runtime-bound
	* symbols.  If it survives as SymKind_GOT through to
	* relocations based off of it, special, PC-relative
	* relocation types should be generated instead.
	*/
	sp = lookup(MSGgot, sizeof(MSGgot) - 1);
	sp->sym_kind = SymKind_GOT;
	/*
	* Miscellaneous "cached" values.  This requires that
	* the number package has been initialized.  expr_object
	* is potentially shared by many symbol entries.  As its
	* parent pointer is never set, exprtype() and exprfrom()
	* should not (and will not) be called.
	*/
	bss_symbol = lookup(MSGbss, sizeof(MSGbss) - 1);
	expr_object = ulongexpr((Ulong)SymTy_Object);
}

static struct symtree *
#ifdef __STDC__
newsymtree(const Uchar *name, size_t len)	/* allocate new symtree */
#else
newsymtree(name, len)Uchar *name; size_t len;
#endif
{
	static const struct symtree empty = {0};
	static struct symtree *avail, *endavail;
	register struct symtree *stp;

	if ((stp = avail) == endavail)
	{
		register int cnt = 500;	/* not tuneable */

#ifdef DEBUG
		if (DEBUG('a') > 0)
		{
			static Ulong total;

			(void)fprintf(stderr,
				"Total Symbols=%lu @%lu ea.\n",
				total += cnt, (Ulong)sizeof(struct symtree));
		}
#endif
		avail = stp = (struct symtree *)alloc((void *)0,
			cnt * sizeof(struct symtree));
		endavail = stp + cnt;
	}
	avail = stp + 1;
	*stp = empty;
	stp->next = symbols;
	symbols = stp;
	stp->sym.sym_name = name;
	stp->sym.sym_nlen = len;
	stp->sym.sym_kind = SymKind_Regular;
	stp->sym.sym_exty = ExpTy_Unknown;
	stp->sym.sym_bind = Bind_Global;
	return stp;
}

Symbol *
#ifdef __STDC__
lookup(const Uchar *str, size_t len) /* look up (& enter) name as symbol */
#else
lookup(str, len)Uchar *str; size_t len;
#endif
{
	register struct symtree *stp, **prev;
	register const Uchar *s = str;
	register Ulong hval;
	register size_t n;

#ifdef DEBUG
	if (DEBUG('t') > 0)
		(void)fprintf(stderr, "lookup(%.*s)\n", len, str);
#endif
	/*
	* Since most names are fairly short (.L###), unroll the
	* initial portion of the hash calculation.  Since there
	* are at least 32 bits in a Ulong, even if there are 10
	* bits in a Uchar, 5 Uchars can be combined without
	* reaching into the HIGH5_BITS mask.  Moreover, the 6th
	* Uchar can be combined without overflowing the Ulong.
	*/
	/*CONSTANTCONDITION*/
	if (CHAR_BIT * sizeof(Ulong) < 16 + CHAR_BIT + 5 + 1)
		fatal(gettxt(":897","lookup():hash calculation unroll invalid"));
	switch (n = len)
	{
	case 0:
		fatal(gettxt(":898","lookup():zero length name"));
		/*NOTREACHED*/
	default:
		hval = s[0] << 20;
		hval += s[1] << 16;
		hval += s[2] << 12;
		hval += s[3] << 8;
		hval += s[4] << 4;
		hval += s[5];
		s += 5;
		n -= 5;
		for (;;)	/* loop for longer names */
		{
			register Ulong highbits;

			if ((highbits = hval & HIGH5_BITS) != 0)
			{
				hval ^= highbits >> HIGH5_DIST;
				hval &= ~HIGH5_BITS;
			}
			if (--n == 0)
				break;
			hval <<= 4;
			hval += *++s;
		}
		s = str;
		n = len;
		break;
	case 5:
		hval = s[0] << 16;
		hval += s[1] << 12;
		hval += s[2] << 8;
		hval += s[3] << 4;
		hval += s[4];
		break;
	case 4:
		hval = s[0] << 12;
		hval += s[1] << 8;
		hval += s[2] << 4;
		hval += s[3];
		break;
	case 3:
		hval = s[0] << 8;
		hval += s[1] << 4;
		hval += s[2];
		break;
	case 2:
		hval = s[0] << 4;
		hval += s[1];
		break;
	case 1:
		hval = s[0];
		break;
	}
	hval %= sizeof(buckets) / sizeof(buckets[0]);
	prev = &buckets[hval];
	while ((stp = *prev) != 0)
	{
		register int i = strncmp((const char *)s,
				(const char *)stp->sym.sym_name, n);

		if (i == 0 && n == stp->sym.sym_nlen)
			return &stp->sym;	/* found */
		if (i > 0)
			prev = &stp->right;
		else
			prev = &stp->left;
	}
	/*
	* Not found: add to bucket's tree.
	*/
#ifdef DEBUG
	if (++buckuse[hval] > 1 && DEBUG('h') > 0)
	{
		(void)fprintf(stderr, "sym-hash[%lu]: %lu entries\n",
			hval, (Ulong)buckuse[hval]);
	}
#endif
	*prev = stp = newsymtree(savestr(str, len), len);
	stp->parent = prev;
	return &stp->sym;
}

static void
#ifdef __STDC__
labsym(register Symbol *sp, Section *secp, int bind) /* define as label in sect */
#else
labsym(sp, secp, bind)register Symbol *sp; Section *secp; int bind;
#endif
{
	if (defnsym(sp, secp) == 0)
		return;
	if (!(sp->sym_flags & SYM_BOUND))
		sp->sym_bind = bind;
	if (sp->sym_kind != SymKind_Regular)	/* was common or GOT */
	{
		sp->sym_kind = SymKind_Regular;
		/*
		* Don't overwrite calculated size result, either.
		*/
		if (!(sp->sym_flags & (SYM_SIZED | SYM_VALSIZE)))
			sp->size.sym_expr = 0;
	}
	sp->sym_exty = ExpTy_Relocatable;
	sp->addr.sym_expr = dotexpr(secp);
	sp->addr.sym_expr->ex_cont = Cont_Label;
	sp->addr.sym_expr->parent.ex_sym = sp;
	if (sp->sym_uses != 0)
		exprtype(sp->sym_uses);	/* fix types for dependent exprs */
#ifdef P5_ERR_41
	secp->sec_last->code_impdep |= CODE_P5_BR_LABEL;
#endif
}

void
#ifdef __STDC__
label(const Uchar *str, size_t len)	/* define label */
#else
label(str, len)Uchar *str; size_t len;
#endif
{
	register Symbol *sp;

#ifdef DEBUG
	if (DEBUG('t') > 0)
		(void)fprintf(stderr, "label(%.*s)\n", len, str);
#endif
	sp = lookup(str, len);
	if (sp->sym_kind == SymKind_Common)
		warn(gettxt(":899","label was common symbol: %s"), (const char *)sp->sym_name);
	labsym(sp, cursect(), Bind_Local);
#ifdef DEBUG
	if (DEBUG('t') > 1)
		printsymbol(sp);
#endif
}

#ifdef DEBUG
static void
#ifdef __STDC__
printsymtree(const struct symtree *stp)	/* print names in bucket */
#else
printsymtree(stp)struct symtree *stp;
#endif
{
	while (stp != 0)
	{
		(void)fprintf(stderr, " %s",
			(const char *)stp->sym.sym_name);
		printsymtree(stp->left);
		stp = stp->right;
	}
}
#endif

void
#ifdef __STDC__
walksyms(void)	/* mark remaining unknowns as relocatable, etc. */
#else
walksyms()
#endif
{
	static const char MSGundf[] = "undefined name: %s";
	static const char MSGszty[] =
		"useless specification of size and/or type for name: %s";
	size_t nlocals = 0;		/* number of local symbols */
	size_t nothers = 0;		/* global and weak symbols */
	size_t nschars = 0;		/* number of strtab chars used */
	struct symtree *comms;		/* list of local/temp commons */
	struct symtree *sets;		/* list of .set symbols */
	register struct symtree *stp;
	register struct symtree **prev = &symbols;
	register struct symtree **cprev = &comms;
	register struct symtree **sprev = &sets;
	Section *secp;

#ifdef DEBUG
	if (DEBUG('h') > 1)	/* output histogram */
	{
		Ulong tot = 0;
		int i;

		for (i = 0; i < sizeof(buckets) / sizeof(buckets[0]); i++)
		{
			if (buckuse[i] == 0)
				continue;
			tot += buckuse[i];
			(void)fprintf(stderr, "syms-hash[%d] %lu:",
				i, (Ulong)buckuse[i]);
			printsymtree(buckets[i]);
			(void)putc('\n', stderr);
		}
		(void)fprintf(stderr, "total symbols: %lu\n", tot);
	}
#endif
	/*
	* 1. Mark just referenced symbols as undefined and update exprs.
	* 2. Change unbound ".*" symbols to temporary.
	* 3. Place some symbols on separate lists; ignore others.
	* 4. Make sure no "empty" sections are referenced.
	* 5. Give each local or global/weak symbol its index value.
	* 6. Relink symbol table to exclude unimportant symbols.
	*/
	stp = symbols;
	do
	{
		if (stp->sym.sym_defn == 0
			&& stp->sym.sym_kind != SymKind_Common)
		{
			if (stp->sym.sym_refd == 0)
				continue;	/* not needed */
			stp->sym.sym_exty = ExpTy_Relocatable;
			if (stp->sym.sym_uses != 0)
				exprtype(stp->sym.sym_uses);
		}
		if (stp->sym.sym_flags & SYM_OLDSET)
			continue;		/* ignore this */
		/*
		* Convert unbound .* names to temporary and warn about
		* unused size and/or types unless they are common names.
		*/
		if (!(stp->sym.sym_flags & SYM_BOUND)
			&& stp->sym.sym_name[0] == '.')
		{
			stp->sym.sym_bind = Bind_Temporary;
			if (stp->sym.sym_flags & (SYM_SIZED | SYM_TYPED))
			{
				if (stp->sym.sym_kind != SymKind_Common)
				{
					warn(gettxt(":1017",MSGszty),
						(char *)stp->sym.sym_name);
				}
			}
		}
		/*
		* Put the symbol on the appropriate list (if any).
		* All .set symbols are placed on their own list;
		* local/temporary common symbols are placed on yet
		* another; otherwise, nontemporary symbols go on
		* the regular list.
		*/
		switch (stp->sym.sym_kind)
		{
		case SymKind_Dot:
			continue;	/* never place in object file */
		default:
			if (stp->sym.sym_bind != Bind_Temporary)
			{
				*prev = stp;
				prev = &stp->next;
			}
			/*
			* If nothing exists in a section except for one
			* or more labels (including labels to be defined
			* later in .bss for local/temp common symbols),
			* no "code"s have been registered, and thus the
			* section is "empty".  To work around this problem,
			* a ".zero 0" directive is registered for each
			* such section.
			*/
			secp = stp->sym.sym_defn;
		emptycheck:;
			if (secp != 0 && secp->sec_code == secp->sec_last)
				sectzero(secp, (Ulong)0);
			break;
		case SymKind_Set:
			*sprev = stp;
			sprev = &stp->next;
			break;
		case SymKind_Common:
			if (stp->sym.sym_bind == Bind_Local
				|| stp->sym.sym_bind == Bind_Temporary)
			{
				*cprev = stp;
				cprev = &stp->next;
				/*
				* Check for empty .bss section.
				*/
				secp = bss_symbol->sym_sect;
				goto emptycheck;
			}
			else
			{
				*prev = stp;
				prev = &stp->next;
			}
			break;
		}
		/*
		* Count the symbol in the appropriate bucket.
		* Check here for undefined locals and temporaries.
		*/
		switch (stp->sym.sym_bind)
		{
		case Bind_Local:
			stp->sym.sym_index = ++nlocals;
			nschars += stp->sym.sym_nlen + 1;
			/*FALLTHROUGH*/
		case Bind_Temporary:
			if (stp->sym.sym_defn == 0
				&& stp->sym.sym_kind != SymKind_Common)
			{
				error(gettxt(":1016",MSGundf), (char *)stp->sym.sym_name);
			}
			break;
		case Bind_Global:
		case Bind_Weak:
			stp->sym.sym_index = ++nothers;
			nschars += stp->sym.sym_nlen + 1;
			break;
		}
	} while ((stp = stp->next) != 0);
	/*
	* Terminate list of "interesting" symbols.  Put local and
	* temporary common symbols (if any) at the head of the list.
	* After these come the .set symbols, and finally the rest.
	*/
	*prev = 0;
	*sprev = symbols;
	*cprev = sets;
	symbols = comms;
	objfmksyms(nlocals, nothers, nschars);	/* prime obj. file symbols */
}

static int
#ifdef __STDC__
set_valid(Symbol *sp)	/* validate .set as appropriate to its binding */
#else
set_valid(sp)Symbol *sp;
#endif
{
	register Eval *vp;
	const char *str;

	switch (sp->sym_exty)
	{
	default:
		fatal(gettxt(":900","set_valid():unknown exty: %u"), (Uint)sp->sym_exty);
		/*NOTREACHED*/
	case ExpTy_Unknown:
		backerror((Ulong)sp->sym_file, sp->sym_line,
			gettxt(":901",".set name has indeterminable type: %s"),
			(const char *)sp->sym_name);
		return 0;
	case ExpTy_String:
		str = "string";
		break;
	case ExpTy_Register:
		str = "register";
		break;
	case ExpTy_Operand:
		str = "addressing mode";
		break;
	case ExpTy_Floating:
		str = "floating";
		break;
	case ExpTy_Integer:
		return sp->sym_bind != Bind_Temporary;
	case ExpTy_Relocatable:
		if (sp->sym_bind == Bind_Temporary)
			return 0;
		/*
		* Need to calculate the value for relocatable .set
		* names now (before its size and type are calculated)
		* because a relocatable .set name acquires its default
		* size and/or type from the symbol from which it
		* acquired its relocatable expression type.
		*/
		if ((sp->sym_flags & (SYM_SIZED | SYM_VALSIZE)) == SYM_VALSIZE)
		{
			fatal(gettxt(":902","set_valid():already have size: %s"),
				(const char *)sp->sym_name);
		}
		if ((sp->sym_flags & (SYM_TYPED | SYM_VALTYPE)) == SYM_VALTYPE)
		{
			fatal(gettxt(":903","set_valid():already have type: %s"),
				(const char *)sp->sym_name);
		}
		vp = evalexpr(sp->addr.sym_oper->oper_expr);
		/*
		* No reevaluation check needed: all addresses fixed.
		*/
		if (!(vp->ev_flags & EV_RELOC))
			fatal(gettxt(":904","set_valid():no sect after reloc expr eval"));
		if (vp->ev_flags & (EV_OFLOW | EV_TRUNC))
		{
			backerror((Ulong)sp->sym_file, sp->sym_line,
				gettxt(":905","offset for %s out of range: %s"),
				(const char *)sp->sym_name,
				num_tohex(vp->ev_int));
		}
		if ((sp->sym_defn = vp->ev_sec) == 0)
		{
			if (vp->ev_sym == 0)
				fatal(gettxt(":906","set_valid():no sym/sect for reloc"));
			backerror((Ulong)sp->sym_file, sp->sym_line,
				gettxt(":907","cannot base .set \"%s\" on undefined name: %s"),
				(const char *)sp->sym_name,
				(const char *)vp->ev_sym->sym_name);
		}
		if (vp->ev_sym != 0)
		{
			register Symbol *base = vp->ev_sym;

			if (!(sp->sym_flags & SYM_SIZED))
			{
				sp->size = base->size;
				if (base->sym_flags & SYM_VALSIZE)
					sp->sym_flags |= SYM_VALSIZE;
			}
			if (!(sp->sym_flags & SYM_TYPED))
			{
				sp->type = base->type;
				if (base->sym_flags & SYM_VALTYPE)
					sp->sym_flags |= SYM_VALTYPE;
			}
		}
		return 1;
	}
	if (sp->sym_bind == Bind_Temporary)
		return 0;
	backerror((Ulong)sp->sym_file, sp->sym_line,
		gettxt(":908","local/global/weak .set name cannot have %s value: %s"),
		str, (const char *)sp->sym_name);
	return 0;
}

static void
#ifdef __STDC__
size_valid(Symbol *sp)	/* validate and set size for symbol */
#else
size_valid(sp)Symbol *sp;
#endif
{
	Ulong size = 0;		/* default size */
	Eval *vp;
	Expr *ep;

	if ((ep = sp->size.sym_expr) != 0)
	{
		if (ep->ex_type != ExpTy_Integer)
		{
			fatal(gettxt(":909","size_valid():non-int symbol size: %s"),
				(const char *)sp->sym_name);
		}
		vp = evalexpr(ep);
		/*
		* No reevaluation check needed: all addresses fixed.
		*/
		if (vp->ev_flags & (EV_OFLOW | EV_TRUNC))
		{
			exprerror(ep, gettxt(":910","size for %s out of range: %s"),
				(const char *)sp->sym_name,
				num_tohex(vp->ev_int));
		}
		size = vp->ev_ulong;
	}
	sp->sym_flags |= SYM_VALSIZE;
	sp->size.sym_value = size;
}

static void
#ifdef __STDC__
typestr_valid(Symbol *sp)	/* validate and set type from string */
#else
typestr_valid(sp)Symbol *sp;
#endif
{
	Expr *ep = setlessexpr(sp->type.sym_expr);
	const char *tystr = 0;
	Ulong type;

	switch (ep->left.ex_len)  /* These strings should NOT be 
					internationalized */
	{
	case 4:
		tystr = "none";
		type = SymTy_None;
		break;
	case 6:
		tystr = "object";
		type = SymTy_Object;
		break;
	case 8:
		tystr = "function";
		type = SymTy_Function;
		break;
	}
	if (tystr == 0 || strncmp((const char *)ep->right.ex_str,
		tystr, ep->left.ex_len) != 0)
	{
		exprerror(sp->type.sym_expr,
			gettxt(":911","invalid symbol type for %s: \"%s\""),
			(const char *)sp->sym_name,
			prtstr(ep->right.ex_str, ep->left.ex_len));
		type = SymTy_None;
	}
	sp->sym_flags |= SYM_VALTYPE;
	sp->type.sym_value = type;
}

static void
#ifdef __STDC__
type_valid(Symbol *sp)	/* validate symbol type value/string */
#else
type_valid(sp)Symbol *sp;
#endif
{
	Ulong type = SymTy_None;	/* default symbol type */
	Eval *vp;
	Expr *ep;

	if ((ep = sp->type.sym_expr) != 0)
	{
		if (ep->ex_type == ExpTy_String)
		{
			typestr_valid(sp);
			return;
		}
		if (ep->ex_type != ExpTy_Integer)
		{
			fatal(gettxt(":912","type_valid():non-str/int symbol type: %s"),
				(const char *)sp->sym_name);
		}
		vp = evalexpr(ep);
		/*
		* No reevaluation check needed: all addresses fixed.
		*/
		if (vp->ev_flags & (EV_OFLOW | EV_TRUNC))
		{
			if (ep == expr_object)
				fatal(gettxt(":913","type_valid():bad object type expr"));
			exprerror(ep, gettxt(":914","symbol type for %s out of range: %s"),
				(const char *)sp->sym_name,
				num_tohex(vp->ev_int));
		}
		type = vp->ev_ulong;
	}
	sp->sym_flags |= SYM_VALTYPE;
	sp->type.sym_value = type;
}

static void
#ifdef __STDC__
align_valid(Symbol *sp)	/* validate and set alignment for common */
#else
align_valid(sp)Symbol *sp;
#endif
{
	Eval *vp;
	Ulong align;

	/*
	* Since common symbols are given their alignments
	* at each .[l]comm directive, the file/line pair
	* in the symbol table is correct for error messages.
	* Also note that there must be a nonzero pointer here.
	*/
	if (sp->align.sym_expr->ex_type != ExpTy_Integer)
	{
		fatal(gettxt(":915","align_valid():non-int symbol alignment: %s"),
			(const char *)sp->sym_name);
	}
	vp = evalexpr(sp->align.sym_expr);
	/*
	* No reevaluation check needed; all addresses fixed.
	*/
	if (vp->ev_flags & (EV_OFLOW | EV_TRUNC))
	{
		backerror((Ulong)sp->sym_file, sp->sym_line,
			gettxt(":916","alignment for %s out of range: %s"),
			(const char *)sp->sym_name, num_tohex(vp->ev_int));
	}
	align = vp->ev_ulong;
	if (!validalign(align))
	{
		backerror((Ulong)sp->sym_file, sp->sym_line,
			gettxt(":917","invalid alignment value for %s: %lu"),
			(const char *)sp->sym_name, align);
		align = BSS_COMM_ALIGN;
	}
	sp->sym_flags |= SYM_VALALIGN;
	sp->align.sym_value = align;
}

void
#ifdef __STDC__
gensyms(void)	/* generate contents for object file symbol */
#else
gensyms()
#endif
{
	register struct symtree *stp;

	/*
	* The only names on the linked list are those names that
	* will appear in the symbol table or temporary common
	* names that need to be instantiated in .bss.  The list
	* has been ordered so that all local and temporary common
	* symbols come first.  This allows other size and type
	* calculations to be based on their final addresses.
	*
	* 1. Check for valid .set expression types.
	* 2. Finalize any sizes and types.
	* 3. Instantiate local or temporary common in .bss.
	* 4. Hand result off to object file symbol table.
	*/
	for (stp = symbols; stp != 0; stp = stp->next)
	{
		/*
		* Check that .set symbols have appropriate value types.
		* This eliminates any dependency loops.  Ignore if a
		* bad type found, or if the symbol is temporary.
		*/
		if (stp->sym.sym_kind == SymKind_Set)
		{
			if (set_valid(&stp->sym) == 0)
				continue;	/* ignore it otherwise */
		}
		/*
		* Finalize sizes and types.
		*/
		if (!(stp->sym.sym_flags & SYM_VALSIZE))
			size_valid(&stp->sym);
		if (!(stp->sym.sym_flags & SYM_VALTYPE))
			type_valid(&stp->sym);
		/*
		* Finalize alignments for common names.  Define local
		* and temporary common in .bss.  Notice that since
		* these common names are instantiated without regard
		* to dependencies on finalized addresses, if any of
		* these common symbols' sizes, types, or alignments
		* depend on its own or another local or temporary
		* common symbol's fixed address in .bss, there is no
		* guarantee that it will work.
		*
		* This seems like a reasonable restriction.
		*/
		if (stp->sym.sym_kind == SymKind_Common)
		{
			if (!(stp->sym.sym_flags & SYM_VALALIGN))
				align_valid(&stp->sym);
			if (stp->sym.sym_bind == Bind_Local
				|| stp->sym.sym_bind == Bind_Temporary)
			{
				bsssym(&stp->sym, (int)stp->sym.sym_bind,
					stp->sym.size.sym_value,
					stp->sym.align.sym_value);
				if (stp->sym.sym_bind == Bind_Temporary)
					continue; /* not in object file */
			}
		}
#ifdef DEBUG
		if (DEBUG('t') > 2)
			printsymbol(&stp->sym);
#endif
		objfsymbol(&stp->sym);
	}
}

void
#ifdef __STDC__
bindsym(register Symbol *sp, int bind)	/* set binding for symbol */
#else
bindsym(sp, bind)register Symbol *sp; int bind;
#endif
{
	static const char MSGbind[] = "name %s bound as %s: %s";
	const char *bstr;

	if (sp->sym_flags & SYM_BOUND)
	{
		switch (sp->sym_bind)
		{
		default:
			fatal(gettxt(":918","bindsym():unknown binding: %d\n"), bind);
			/*NOTREACHED*/
		case Bind_Local:
			bstr = "local";
			break;
		case Bind_Global:
			bstr = "global";
			break;
		case Bind_Weak:
			bstr = "weak";
			break;
		case Bind_Temporary:
			bstr = "temporary"; /* shouldn't happen */
			break;
		}
		if (sp->sym_bind == bind)
		{
			warn(gettxt(":1018",MSGbind), gettxt(":919","already"), bstr,
				(const char *)sp->sym_name);
		}
		else
		{
			error(gettxt(":1018",MSGbind), gettxt(":920","previously"), bstr,
				(const char *)sp->sym_name);
		}
	}
	sp->sym_bind = bind;
	sp->sym_flags |= SYM_BOUND;
	sp->sym_refd = 1;	/* don't ignore otherwise unused symbol */
}

void
#ifdef __STDC__
sizesym(register Symbol *sp, Expr *ep) /* set size for symbol */
#else
sizesym(sp, ep, dflval)register Symbol *sp; Expr *ep;
#endif
{
	if (sp->sym_flags & SYM_SIZED)
	{
		error(gettxt(":1014",MSGmlt), gettxt(":768","size"), (const char *)sp->sym_name);
		return;
	}
	fixexpr(ep);
	sp->size.sym_expr = ep;
	sp->sym_flags |= SYM_SIZED;
}

void
#ifdef __STDC__
typesym(Symbol *sp, Expr *ep)	/* set type for symbol */
#else
typesym(sp, ep)Symbol *sp; Expr *ep;
#endif
{
	if (sp->sym_flags & SYM_TYPED)
	{
		error(gettxt(":1014",MSGmlt), gettxt(":921","type"), (const char *)sp->sym_name);
		return;
	}
	sp->type.sym_expr = ep;
	sp->sym_flags |= SYM_TYPED;
	if (ep->ex_type == ExpTy_String)
		typestr_valid(sp);
	else
		fixexpr(ep);
}

void
#ifdef __STDC__
bsssym(register Symbol *sp, int bind, Ulong sz, Ulong al) /* def in bss */
#else
bsssym(sp, bind, sz, al)register Symbol *sp; int bind; Ulong sz, al;
#endif
{
	register Section *bss = bss_symbol->sym_sect;

	if (sp->sym_defn != 0)	/* check before we align the section */
	{
		error(gettxt(":1015",MSGdef), (const char *)sp->sym_name);
		return;
	}
	if (al > 1)	/* optionally fix alignment */
	{
		static const Ulong aligns[17][2] = /* most likely alignments */
		{
			{BSS_COMM_ALIGN, BSS_COMM_ALIGN - 1},
			{1, 0},		{2, 1},		{3, 2},
			{4, 3},		{5, 4},		{6, 5},
			{7, 6},		{8, 7},		{9, 8},
			{10, 9},	{11, 10},	{12, 11},
			{13, 12},	{14, 13},	{15, 14},
			{16, 15},
		};
		Ulong *ulp;

		if (al == BSS_COMM_ALIGN)	/* special case */
			ulp = (Ulong *)aligns[0];
		else if (al < 17)		/* normal alignment values */
			ulp = (Ulong *)aligns[al];
		else				/* have to allocate */
		{
#ifdef DEBUG
			if (DEBUG('a') > 0)
			{
				static Ulong total;

				(void)fprintf(stderr,
					"Total bss aligns=%lu @%lu ea.\n",
					total += 1, 2 * sizeof(Ulong));
			}
#endif
			ulp = (Ulong *)alloc((void *)0, 2 * sizeof(Ulong));
			ulp[0] = al;
			ulp[1] = al - 1;
		}
		(void)sectoptalign(bss, ulp);
	}
	labsym(sp, bss, bind);
	sectzero(bss, sz);
	if (!(sp->sym_flags & SYM_TYPED))  /* type defaults to object */
	{
		/*
		* Since bsssym() can be called after a type has
		* been calculated, overwrite the appropriate member.
		*/
		if (sp->sym_flags & SYM_VALTYPE)
			sp->type.sym_value = SymTy_Object;
		else
			sp->type.sym_expr = expr_object; /* shared */
	}
	/*
	* Don't overwrite calculated size value, either.
	*/
	if (!(sp->sym_flags & (SYM_SIZED | SYM_VALSIZE)))
	{
		sp->sym_flags |= (SYM_SIZED | SYM_VALSIZE);
		sp->size.sym_value = sz;
	}
#ifdef DEBUG
	if (DEBUG('t') > 1)
		printsymbol(sp);
#endif
}

void
#ifdef __STDC__
commsym(register Symbol *sp, int bind, register Expr *sz, register Expr *al)
#else
commsym(sp, bind, sz, al)register Symbol *sp; int bind; register Expr *sz, *al;
#endif
{
	if (sp->sym_kind == SymKind_Dot)
	{
		error(gettxt(":922","cannot define . (dot) as common"));
		return;
	}
	if (sp->sym_kind == SymKind_Set)
	{
		error(gettxt(":923",".set name cannot also be common: %s"),
			(const char *)sp->sym_name);
		return;
	}
	sp->sym_file = curfileno;	/* each time */
	sp->sym_line = curlineno;
	if (!(sp->sym_flags & SYM_BOUND))
		sp->sym_bind = bind;
	if (!(sp->sym_flags & SYM_SIZED))
	{
		fixexpr(sz);
		if (sp->sym_kind == SymKind_Common)	/* not first time */
		{
			Operand *oper;

			oper = sz->parent.ex_oper;	/* use same context */
			sz = binaryexpr(ExpOp_Maximum,
				sp->size.sym_expr, sz);
			sz->ex_cont = Cont_Operand;
			sz->parent.ex_oper = oper;
		}
		sp->size.sym_expr = sz;
	}
	if (!(sp->sym_flags & SYM_TYPED))  /* type defaults to object */
		sp->type.sym_expr = expr_object;	/* shared */
	if (sp->sym_defn != 0)
	{
		warn(gettxt(":924","common symbol already defined: %s"),
			(const char *)sp->sym_name);
		return;
	}
	sp->sym_exty = ExpTy_Relocatable;
	fixexpr(al);
	if (sp->sym_kind != SymKind_Common)
		sp->sym_kind = SymKind_Common;
	else
	{
		Operand *oper;

		oper = al->parent.ex_oper;	/* use same context */
#ifdef ALIGN_IS_POW2
		al = binaryexpr(ExpOp_Maximum, sp->align.sym_expr, al);
#else
		al = binaryexpr(ExpOp_LCM, sp->align.sym_expr, al);
#endif
		al->ex_cont = Cont_Operand;
		al->parent.ex_oper = oper;
	}
	sp->align.sym_expr = al;
	if (sp->sym_uses != 0)
		exprtype(sp->sym_uses);	/* fix types for dependent exprs */
#ifdef DEBUG
	if (DEBUG('t') > 1)
		printsymbol(sp);
#endif
}

void
#ifdef __STDC__
opersym(register Symbol *sp, Operand *op) /* operand for .set symbol */
#else
opersym(sp, op)register Symbol *sp; Operand *op;
#endif
{
	static const char MSGbadarg[] =
		"must .set . to an evaluatable relocatable expr";
	static const char MSGbadval[] =
		"must .set . to further forward in the same section";

	if (sp->sym_kind == SymKind_Common)
	{
		error(gettxt(":925","common name cannot also be .set: %s"),
			(const char *)sp->sym_name);
		olstfree(op);
		return;
	}
	if (sp->sym_kind == SymKind_Dot)	/* .set .,.+intexpr */
	{
		register Eval *vp;
		register Expr *ep;
		register Section *secp;

		if ((ep = op->oper_expr) == 0 || op->oper_flags != 0
			|| ep->ex_type != ExpTy_Relocatable)
		{
			error(MSGbadarg);
			olstfree(op);
			return;
		}
		/*
		* Evaluate op->oper_expr and verify that it is
		* further forward in the current section.
		*/
		vp = evalexpr(ep);
		secp = cursect();
		if (!(vp->ev_flags & EV_RELOC))
			fatal(gettxt(":926","opersym():no sect after reloc expr eval"));
		if (vp->ev_sec != secp
			|| vp->ev_flags & (EV_OFLOW | EV_TRUNC)
			|| vp->ev_pic != 0 || vp->ev_mask != 0
			|| vp->ev_ulong < secp->sec_size)
		{
			error(MSGbadval);
			olstfree(op);
			return;
		}
		if (vp->ev_flags & EV_LDIFF)
		{
			delayeval(vp);
			cutoper(op);
		}
		else
			operfree(op);
		sectpad(secp, vp);
		return;
	}
	/*
	* .set names are special in that multiple definitions are valid.
	* When a .set name is redefined, replace current symbol entry
	* with a fresh one.  The old one is still referenced by the
	* expressions (including those of our new op) that made use
	* of it, if any.
	*/
	if (sp->sym_defn != 0)
	{
		register struct symtree *stp, *old;

		if (sp->sym_kind != SymKind_Set)
		{
			error(gettxt(":1015",MSGdef), (const char *)sp->sym_name);
			olstfree(op);
			return;
		}
		stp = newsymtree(sp->sym_name, sp->sym_nlen);
		old = (struct symtree *)sp;
		stp->left = old->left;
		stp->right = old->right;
		if ((stp->parent = old->parent) != 0)
			*stp->parent = stp;
		stp->sym = *sp;
		sp->sym_flags |= SYM_OLDSET;
		sp = &stp->sym;
		sp->sym_defn = 0;
	}
	(void)defnsym(sp, NOSECT);
	sp->sym_kind = SymKind_Set;
	sp->addr.sym_oper = op;
	cutolst(op->parent.oper_olst);
	op->oper_setsym = 1;
	op->parent.oper_sym = sp;
	if (!(sp->sym_flags & SYM_BOUND))
		sp->sym_bind = Bind_Temporary;
	if (op->oper_flags == 0)	/* simple expression operand */
	{
		register Expr *ep = op->oper_expr;

		fixexpr(ep);
		ep->ex_cont = Cont_Set;
		sp->sym_exty = ep->ex_type;
		sp->sym_mods = ep->ex_mods;
	}
	else
	{
		sp->sym_exty = setamode(op);
		sp->sym_mods = 0;
	}
	if (sp->sym_uses != 0)
		exprtype(sp->sym_uses);	/* fix types for dependent exprs */
#ifdef DEBUG
	if (DEBUG('t') > 1)
		printsymbol(sp);
#endif
}

Uchar *
#ifdef __STDC__
savestr(const Uchar *str, register size_t len)	/* stash string + '\0' */
#else
savestr(str, len)Uchar *str; register size_t len;
#endif
{
	static Uchar *strbuf;	/* available space */
	static size_t buflen;	/* length of available space */
	Uchar *retval;

#ifndef ALLOC_STRS_CHUNK
#   define ALLOC_STRS_CHUNK (2 * BUFSIZ)
#endif
	if (len >= buflen)	/* need more space */
	{
#ifdef DEBUG
		static Ulong total, wasted;

		wasted += buflen;
		if ((buflen = ALLOC_STRS_CHUNK) <= len)
			buflen = (len / BUFSIZ + 1) * BUFSIZ;
		total += buflen;
		if (DEBUG('a') > 0)
		{
			(void)fprintf(stderr,
				"Total string space=%lu, wasted=%lu\n",
				total, wasted);
		}
#else
		if ((buflen = ALLOC_STRS_CHUNK) <= len)
			buflen = (len / BUFSIZ + 1) * BUFSIZ;
#endif	/*DEBUG*/
		strbuf = (Uchar *)alloc((void *)0, buflen);
	}
	retval = (Uchar *)memcpy((void *)strbuf, (void *)str, len);
	strbuf[len++] = '\0';
	strbuf += len;
	buflen -= len;
	return retval;
}

#ifdef DEBUG

void
#ifdef __STDC__
printsymbol(const Symbol *sp)	/* output contents of Symbol entry */
#else
printsymbol(sp)Symbol *sp;
#endif
{
	if (sp == 0)
	{
		(void)fputs("(Symbol*)0\n", stderr);
		return;
	}
	(void)fputs((const char *)sp->sym_name, stderr);
	if (sp->sym_kind == SymKind_Set)
	{
		(void)fputs(":oper=", stderr);
		printoperand(sp->addr.sym_oper);
	}
	else
	{
		(void)fputs(":addr=", stderr);
		printexpr(sp->addr.sym_expr);
	}
	(void)fprintf(stderr, ",sect=%#lx,refd=%d",
		(Ulong)sp->sym_sect, sp->sym_refd);
	if (sp->sym_defn != 0 && sp->sym_defn != NOSECT)
	{
		(void)fprintf(stderr, ",defn=<%s>",
			(const char *)sp->sym_defn->sec_sym->sym_name);
	}
	if (sp->sym_flags & SYM_VALSIZE)
		(void)fprintf(stderr, ",size=%lu", sp->size.sym_value);
	else
	{
		(void)fputs(",size=", stderr);
		printexpr(sp->size.sym_expr);
	}
	if (sp->sym_kind == SymKind_Common)
	{
		if (sp->sym_flags & SYM_VALALIGN)
		{
			(void)fprintf(stderr, ",align=%lu",
				sp->align.sym_value);
		}
		else
		{
			(void)fputs(",align=", stderr);
			printexpr(sp->align.sym_expr);
		}
	}
	(void)fprintf(stderr, ",mods=%#x,flags=%#x,uses=",
		sp->sym_mods, sp->sym_flags);
	printexpr(sp->sym_uses);
	if (sp->sym_flags & SYM_VALTYPE)
	{
		switch (sp->type.sym_value)
		{
		default:
			(void)fprintf(stderr, ",type=%lu?",
				sp->type.sym_value);
			break;
		case SymTy_None:
			(void)fputs(",nty", stderr);
			break;
		case SymTy_Object:
			(void)fputs(",obj", stderr);
			break;
		case SymTy_Function:
			(void)fputs(",fcn", stderr);
			break;
		}
	}
	else
	{
		(void)fputs(",type=", stderr);
		printexpr(sp->type.sym_expr);
	}
	switch (sp->sym_exty)
	{
	default:
		(void)fprintf(stderr, ",exty=%u?", sp->sym_exty);
		break;
	case ExpTy_String:
		(void)fputs(",str", stderr);
		break;
	case ExpTy_Register:
		(void)fputs(",reg", stderr);
		break;
	case ExpTy_Operand:
		(void)fputs(".opr", stderr);
		break;
	case ExpTy_Integer:
		(void)fputs(",int", stderr);
		break;
	case ExpTy_Floating:
		(void)fputs(",flt", stderr);
		break;
	case ExpTy_Relocatable:
		(void)fputs(",rel", stderr);
		break;
	case ExpTy_Unknown:
		(void)fputs(",unk", stderr);
		break;
	}
	switch (sp->sym_bind)
	{
	default:
		(void)fprintf(stderr, ",bind=%u?\n", sp->sym_bind);
		break;
	case Bind_Global:
		(void)fputs(",gbl\n", stderr);
		break;
	case Bind_Local:
		(void)fputs(",lcl\n", stderr);
		break;
	case Bind_Weak:
		(void)fputs(",wek\n", stderr);
		break;
	case Bind_Temporary:
		(void)fputs(",tmp\n", stderr);
		break;
	}
}

#endif /*DEBUG*/
