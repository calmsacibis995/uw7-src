#ident	"@(#)fur:i386/cmd/fur/opdecode.c	1.7"
#ident	"$Header:"

#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <setjmp.h>
#include <macros.h>
#include "fur.h"
#include "op.h"

static int Travstack[1001];
static int Depth;
static int (*Check_each_statement)();
static int (*Check_each_block)();
static int (*Call_at_end)();
static int (*Proc_block)(int block);
static int Srcs[NUMOPS], Dests[NUMOPS];
static int MaxESP;

int Dont_subst;
int Dont_esr;

void
give_up(char *str)
{
	if (VERBOSE1)
		printf("Giving up: %s\n", str);
	longjmp(Giveup, 1);
}

void
opdebug()
{
	FLUSH_OUT();
}

grow_watch(int var)
{
	int prev_swatch;

	if (Swatchinfo <= var) {
		prev_swatch = Swatchinfo;
		Swatchinfo = var + 2;
		Watchinfo = REALLOC(Watchinfo, Swatchinfo * sizeof(struct varinfo));
		memset(Watchinfo + prev_swatch, '\0', (Swatchinfo - prev_swatch) * sizeof(struct varinfo));
	}
	Nwatchinfo = var + 1;
	return(0);
}

void
clear_watch()
{
	memset(Watchinfo, '\0', Swatchinfo * sizeof(struct varinfo));
}

void
out(const char *fmt, ...)
{
	int len;

	va_list ap;
	va_start(ap, fmt);
	if ((len = strlen(obuf)) > 768) {
		FLUSH_OUT();
		vsprintf(obuf, fmt, ap);
	}
	else
		vsprintf(obuf + len, fmt, ap);
}

char *
pr(int var)
{
	static char buf[100];

	sprintf(buf, "Stack[%d]", var - Before_stack);
	return(buf);
}

mark_read(int var)
{
	if (Traversal_ctl & MARK_READ_ONLY_AFTER_WRITTEN)
		if (!WROTE_VAR(var))
			return;
	Varinfo[var].flags |= READ_FLAG;
}

void
do_read(int var, int flags)
{
	int i;
	int subst = 0;

	if (IS_PSEUDO_REG(var))
		return;
	if ((Traversal_ctl & SEE_ARGS) && IS_WATCHED(var) && (var < Last_arg) && (FUNCNO(Curblock) == Enclosing_func)) {
		if (VERBOSE2)
			printf("Decreasing Last from %d to %d\n", Last_arg - Before_stack, var - Before_stack);
		Last_arg = var;
	}
	if (flags & ANDBACK) {
		for (i = var; !(Varinfo[i].flags & (BARRIER|VALUE_UNKNOWN_TO_CALLEE)) && (i >= NBASE); i--)
			MARK_READ(i);
		return;
	}
	else if (flags & INDEXED) {
		/* Notice the difference:  going back the barrier protects
		* itself; going forward it protects the next entry
		*/
		if (i >= NBASE) {
			for (i = var; !(Varinfo[i].flags & (BARRIER|VALUE_UNKNOWN_TO_CALLEE)) && (i >= NBASE); i--)
				MARK_READ(i);
			for (i = var + 1; (i != Varinfo[ESP].alias) && (i >= NBASE); i++) {
				MARK_READ(i);
				if (Varinfo[i].flags & (BARRIER|VALUE_UNKNOWN_TO_CALLEE))
					break;
			}
		}
		return;
	}
	else if (flags & TWOPOS) {
		MARK_READ(var);
		MARK_READ(var - 1);
		return;
	}

	if (!(Traversal_ctl & DO_SUBST)) {
		if (!mark_read_subst(var))
			MARK_READ(var);
		return;
	}
	if (Only_esr || !(Traversal_ctl & DO_SUBST) || (flags & CANT_SUBST) || (FUNCNO(Curblock) != Enclosing_func)) {
		MARK_READ(var);
		return;
	}
	if (FLAGS(Curblock) & BEENHERE) {
		if (!match_subst(var))
			MARK_READ(var);
		return;
	}
/*	if ((var >= NBASE) && (IS_CONSTANT(var) || IS_REL_CONSTANT(var))) {*/
	if (!IS_ANDBACK(var) && !IS_INDEXED(var) && (IS_CONSTANT(var) || IS_REL_CONSTANT(var))) {
		add_subst_constant(var);
		if ((Traversal_ctl & SHOW_SUBST) && VERBOSE2)
			printf("\tSaved reference to %s at 0x%x because it is constant\n", PR(var), Addr);
		subst = 1;
	}
	if (var < NBASE) {
		if (!subst)
			MARK_READ(var);
		return;
	}
	for (i = NEXTCOPY(var); i != var; i = NEXTCOPY(i)) {
		if (i < NREGS) {
			add_subst_var(var, i);
			if ((Traversal_ctl & SHOW_SUBST) && VERBOSE2)
				printf("\tSaved reference to %s at 0x%x because it is a copy of %s\n", PR(var), Addr, PR(i));
			if (!subst)
				MARK_READ(i);
			subst = 1;
		}
	}
	for (i = NEXTCOPY(var); i != var; i = NEXTCOPY(i)) {
		if ((i >= NBASE) && !IS_WATCHED_FOR_READ(i)) {
			char buf[20];

			strcpy(buf, PR(var));
			add_subst_var(var, i);
			if ((Traversal_ctl & SHOW_SUBST) && VERBOSE2)
				printf("\tSaved reference to %s at 0x%x because it is a copy of %s\n", buf, Addr, PR(i));
			if (!subst)
				MARK_READ(i);
			subst = 1;
		}
	}
	if (!subst)
		MARK_READ(var);
	return;
}

void
do_write(int var, int flags)
{
	int i;

	if ((var == PSEUDO_REG_MEM) || (var == PSEUDO_REG_SPECIAL))
		return;
	if ((Traversal_ctl & SEE_ARGS) && IS_WATCHED(var) && (var < Last_arg) && (FUNCNO(Curblock) == Enclosing_func)) {
		if (VERBOSE2)
			printf("Decreasing Last from %d to %d\n", Last_arg - Before_stack, var - Before_stack);
		Last_arg = var;
	}
	if (flags & ANDBACK) {
		for (i = var; !(Varinfo[i].flags & (BARRIER|VALUE_UNKNOWN_TO_CALLEE)) && (i != NBASE); i--)
			MARK_WRITE(i);
		return;
	}
	else if (flags & INDEXED) {
		/* Notice the difference:  going back the barrier protects
		* itself; going forward it protects the next entry
		*/
		for (i = var; !(Varinfo[i].flags & (BARRIER|VALUE_UNKNOWN_TO_CALLEE)) && (i != NBASE); i--)
			MARK_WRITE(i);
		for (i = var + 1; (i != Varinfo[ESP].alias); i++) {
			MARK_WRITE(i);
			if (Varinfo[i].flags & (BARRIER|VALUE_UNKNOWN_TO_CALLEE))
				break;
		}
		return;
	}
	else if (flags & TWOPOS) {
		MARK_WRITE(var);
		MARK_WRITE(var - 1);
		return;
	}

	MARK_WRITE(var);
	return;
}

void
newvar(int i)
{
	Varinfo[i].type = UNKNOWN_TYPE;
	Varinfo[i].alias = NO_SUCH_ADDR;
	Varinfo[i].u.rel = NULL;
	Varinfo[i].u.val = 0;
	Varinfo[i].nextcopy = i;
	Varinfo[i].prevcopy = i;
	Varinfo[i].flags = 0;
}

int
growstack(int growth)
{
	int esp;
	int newesp;

	if (growth < 0) {
		shrinkstack(-growth);
		return(0);
	}
	if (Varinfo)
		esp = Varinfo[ESP].alias;
	else
		esp = -1;

	if (esp + growth >= Svarinfo) {
		int save_size = Svarinfo;

		Svarinfo += 100 + growth;
		Varinfo = REALLOC(Varinfo, Svarinfo * sizeof(struct varinfo));
		memset(Varinfo + save_size, '\0', (Svarinfo - save_size) * sizeof(struct varinfo));
	}
	for (newesp = esp + 1; newesp <= esp + growth; newesp++)
		newvar(newesp);
	Varinfo[ESP].alias = newesp - 1;
	return(0);
}

int
shrinkstack(int shrinkage)
{
	if (shrinkage < 0) {
		growstack(-shrinkage);
		return(0);
	}
	while (shrinkage) {
		ANYWHERE(Varinfo[ESP].alias);
		Varinfo[ESP].alias--;
		shrinkage--;
		if (Varinfo[ESP].alias < NBASE) {
			STACK_RESET();
			return(0);
		}
	}
	return(0);
}

void
not_a_copy(int index)
{
	Varinfo[Varinfo[index].nextcopy].prevcopy = Varinfo[index].prevcopy;
	Varinfo[Varinfo[index].prevcopy].nextcopy = Varinfo[index].nextcopy;
	Varinfo[index].prevcopy = index;
	Varinfo[index].nextcopy = index;
}

void
not_stack(int index)
{
	not_a_copy(index);
	Varinfo[index].type = NOT_STACK_TYPE;
}

void
pushearlys(int growth)
{
	int i = Varinfo[ESP].alias;

	growstack(growth);
	for ( ; i < Varinfo[ESP].alias; i++)
		NOT_STACK(i);
}

void
makealias(int to, int from, int flags)
{
	if (to == ESP) {
		if (Varinfo[ESP].alias == -1)
			Varinfo[ESP].alias = from;
		else
			growstack(from - Varinfo[ESP].alias);
		Varinfo[ESP].type = ALIAS_TYPE;
		return;
	}
	Varinfo[to].flags &= ~VALUE_UNKNOWN_TO_CALLEE;
	if ((from == PSEUDO_REG_MEM) || (from == PSEUDO_REG_EARLY_STACK))
		anywhere(to);
	else {
		not_a_copy(to);
		Varinfo[to].type = ALIAS_TYPE;
		Varinfo[to].alias = from;
		if (flags & ANDBACK)
			MAKE_ANDBACK(to);
	}
}

void
makerelconstant(int to, Elf32_Rel *rel, int flags)
{
	Varinfo[to].flags &= ~(VALUE_UNKNOWN_TO_CALLEE|INDEXED_FLAG|ANDBACK_FLAG);
	Varinfo[to].flags |= flags & (INDEXED_FLAG|ANDBACK_FLAG);
	Varinfo[to].type = REL_CONSTANT_TYPE;
	Varinfo[to].u.rel = rel;
	not_a_copy(to);
	Varinfo[to].alias = NO_SUCH_ADDR;
	MAKE_NOT_ANDBACK(to);
}

void
makeconstant(int to, Elf32_Addr val, int flags)
{
	Varinfo[to].flags &= ~(VALUE_UNKNOWN_TO_CALLEE|INDEXED_FLAG|ANDBACK_FLAG);
	Varinfo[to].flags |= flags & (INDEXED_FLAG|ANDBACK_FLAG);
	Varinfo[to].type = CONSTANT_TYPE;
	not_a_copy(to);
	Varinfo[to].alias = NO_SUCH_ADDR;
	Varinfo[to].u.val = val;
	MAKE_NOT_ANDBACK(to);
}

void
makecopy(int to, int from)
{
	if (IS_PSEUDO_REG(to))
		return;
	if ((to == ESP) && IS_ALIAS(from)) {
		if (Varinfo[ESP].alias == -1)
			Varinfo[ESP].alias = Varinfo[from].alias;
		else
			growstack(Varinfo[from].alias - Varinfo[ESP].alias);
		Varinfo[ESP].type = ALIAS_TYPE;
		return;
	}
	if ((from == PSEUDO_REG_MEM) || (from == PSEUDO_REG_EARLY_STACK))
		ANYWHERE(to);
	else if (IS_CONSTANT(from))
		makeconstant(to, CONSTANT(from), Varinfo[from].flags);
	else if (IS_REL_CONSTANT(from))
		makerelconstant(to, Varinfo[from].u.rel, Varinfo[from].flags);
	else {
		Varinfo[to].flags = Varinfo[from].flags & (VALUE_UNKNOWN_TO_CALLEE|ANDBACK_FLAG|INDEXED_FLAG);
		Varinfo[to].type = Varinfo[from].type;
		not_a_copy(to);
		Varinfo[to].prevcopy = from;
		Varinfo[to].nextcopy = Varinfo[from].nextcopy;
		Varinfo[Varinfo[from].nextcopy].prevcopy = to;
		Varinfo[from].nextcopy = to;
		Varinfo[to].alias = Varinfo[from].alias;
		if (IS_ANDBACK(from))
			MAKE_ANDBACK(to);
		else if (IS_INDEXED(from))
			MAKE_INDEXED(to);
		else
			MAKE_NOT_ANDBACK_OR_INDEXED(to);
		if ((to == ESP) && (Varinfo[ESP].alias == NO_SUCH_ADDR))
			give_up("ESP undefined");
	}
}

void
anywhere(int index)
{
	not_a_copy(index);
	Varinfo[index].type = ALIAS_TYPE;
	Varinfo[index].alias = Varinfo[ESP].alias;
	MAKE_ANDBACK(index);
}

Elf32_Rel *
checkrel(Elf32_Addr start, Elf32_Addr end)
{
	static Elf32_Rel *rel;
	Elf32_Rel *high = ENDREL(Curblock), *low = REL(Curblock);

	if (high == low)
		return(NULL);
	start += Addr;
	end += Addr;
	if (start < REL(Curblock)->r_offset)
		return(NULL);
	else if (start == REL(Curblock)->r_offset)
		return(REL(Curblock));
	for (rel = REL(Curblock); rel < ENDREL(Curblock); rel++)
		if (rel->r_offset > end)
			break;
	rel--;
	if ((rel >= REL(Curblock)) && (rel->r_offset >= start) && (rel->r_offset < end))
		return(rel);
	else
		return(NULL);
}

Elf32_Addr
getval(int src, int *psrcflags)
{
	if ((src != NO_SUCH_REG) && IS_CONSTANT(src)) {
		if (!psrcflags)
			return(CONSTANT(src));
		switch(PARTIAL(*psrcflags)) {
		case FIRST8:
			return(CONSTANT(src) & 0xff);
		case SECOND8:
			return(CONSTANT(src) & 0xff00);
		case SIXTEEN:
			return(CONSTANT(src) & 0xffff);
		case 0:
			return(CONSTANT(src));
		}
	}
	return(NO_SUCH_ADDR);
}

int
arg_proc(int other, struct opsinfo *pops, int *pflags)
{
	int ret;
	int index;
	Elf32_Rel *rel = NULL;

	if ((pops->u.disp.loc != NO_SUCH_ADDR) && (rel = checkrel(pops->u.disp.loc, pops->u.disp.loc + 1))) {
		pops->flags |= OPTINFO_REL;
		pops->rel = rel;
	}
	if (pops->overreg)
		return(DANGEROUS_REF);
	switch (pops->fmt) {
	case OPTINFO_SIB:
		READREG(pops->reg);
		if (pops->index != NO_REG)
			READREG(pops->index);
		if (IS_ANDBACK(pops->reg))
			*pflags |= ANDBACK;
		else if (IS_INDEXED(pops->reg))
			*pflags |= INDEXED;
		if (IS_INDEFINITE(pops->reg)) {
			ret = Varinfo[ESP].alias;
			*pflags |= ANDBACK|POSSIBLY_MEMORY;
			break;
		}
		/* %esp is always an alias to the stack */
		if (!IS_ALIAS(pops->reg)) {
			ret = PSEUDO_REG_MEM;
			*pflags |= POSSIBLY_MEMORY;
			break;
		}
		if (ALIAS(pops->reg) == PSEUDO_REG_EARLY_STACK) {
			ANYWHERE(PSEUDO_REG_EARLY_STACK);
			ret = PSEUDO_REG_EARLY_STACK;
			break;
		}
		if (pops->flags & OPTINFO_REL) {
			ret = PSEUDO_REG_MEM;
			*pflags |= POSSIBLY_MEMORY;
			break;
		}
		if (IS_ALIAS(pops->index)) {
			index = 0;
			*pflags |= INDEXED;
		}
		else if (pops->index == NO_REG)
			index = 0;
		else if (IS_INDEXED(pops->index))
			*pflags |= INDEXED;
		else {
			index = getval(pops->index, NULL) << pops->scale;
			if (index % sizeof(Elf32_Addr))
				*pflags |= TWOPOS;
		}
		ret = ALIAS(pops->reg) - index / (long) sizeof(Elf32_Addr);
		if (pops->u.disp.value % sizeof(Elf32_Addr))
			*pflags |= TWOPOS;
		if (other & INCR)
			*pflags |= ANDBACK;
		ret -= pops->u.disp.value / (long) sizeof(Elf32_Addr);
		if (*pflags & TWOPOS)
			ret++;
		if (ret < NBASE) {
			ANYWHERE(PSEUDO_REG_EARLY_STACK);
			ret = PSEUDO_REG_EARLY_STACK;
		}
		break;
	case OPTINFO_SIB_NOREG:
		if (pops->index != NO_REG)
			DO_READ(pops->index, 0);
		else if (IS_INDEXED(pops->index))
			*pflags |= INDEXED;
		if (pops->flags & OPTINFO_REL) {
			ret = PSEUDO_REG_MEM;
			*pflags |= POSSIBLY_MEMORY;
			break;
		}
		/* What??? Well, there is no justifiable case for an s-i-b with
		** no base register and no displacement.  This would be the
		** equivalent of a shift.  But, this separates the logic in
		** case there is ever a reason to examine this case.
		*/
		ret = PSEUDO_REG_MEM;
		*pflags |= POSSIBLY_MEMORY;
		break;
	case OPTINFO_REGONLY:
		if (pops->reg == PSEUDO_REG_NO_REG)
			error("Internal error at 0x%x, exiting\n", Addr);
		if (pops->flags & (OPTINFO_SEGREG|OPTINFO_SPECIAL)) {
			ret = PSEUDO_REG_SPECIAL;
			break;
		}
		if ((pops->flags & OPTINFO_EIGHTBIT) && (pops->reg >= 4))
			ret = pops->reg - 4;
		else
			ret = pops->reg;
		break;
	case OPTINFO_DISPONLY:
		ret = PSEUDO_REG_MEM;
		*pflags |= POSSIBLY_MEMORY;
		break;
	case OPTINFO_DISPREG:
		pops->index = NO_REG;
		if (pops->flags & OPTINFO_ADDR_SIXTEEN) {
			ret = DANGEROUS_REF;
			break;
		}
		READREG(pops->reg);
		if (pops->flags & OPTINFO_REL) {
			ret = PSEUDO_REG_MEM;
			*pflags |= POSSIBLY_MEMORY;
			break;
		}
		if (IS_ANDBACK(pops->reg))
			*pflags |= ANDBACK;
		else if (IS_INDEXED(pops->reg))
			*pflags |= INDEXED;
		if (IS_INDEFINITE(pops->reg)) {
			ret = Varinfo[ESP].alias;
			*pflags |= ANDBACK|POSSIBLY_MEMORY;
			break;
		}
		if (!IS_ALIAS(pops->reg)) {
			ret = PSEUDO_REG_MEM;
			*pflags |= POSSIBLY_MEMORY;
			break;
		}
		if (ALIAS(pops->reg) == PSEUDO_REG_EARLY_STACK) {
			ANYWHERE(PSEUDO_REG_EARLY_STACK);
			ret = PSEUDO_REG_EARLY_STACK;
			break;
		}
		if (pops->u.disp.value % sizeof(Elf32_Addr))
			*pflags |= TWOPOS;
		if (other & INCR) {
			*pflags |= ANDBACK;
			makealias(pops->reg, ALIAS(pops->reg), ANDBACK);
		}
		ret = ALIAS(pops->reg) - pops->u.disp.value / (long) sizeof(Elf32_Addr);
		if (ret < NBASE) {
			ANYWHERE(PSEUDO_REG_EARLY_STACK);
			ret = PSEUDO_REG_EARLY_STACK;
		}
		else if (*pflags & TWOPOS)
			ret++;
		break;
	case OPTINFO_IMMEDIATE:
		ret = PSEUDO_REG_CONSTANT;
		if (rel = checkrel(pops->u.imm.loc, pops->u.imm.loc + 1)) {
			pops->rel = rel;
			makerelconstant(PSEUDO_REG_CONSTANT, rel, 0);
		}
		else
			makeconstant(PSEUDO_REG_CONSTANT, pops->u.imm.value, 0);
		break;
	default:
		ret = DANGEROUS_REF;
		printf("Unrecognized format\n");
	}
	if (ret > Varinfo[ESP].alias) {
		ret = Varinfo[ESP].alias;
		*pflags &= ~ANDBACK;
		*pflags |= INDEXED;
	}
	if (pops->flags & OPTINFO_EIGHTBIT) {
		*pflags &= ~TWOPOS;
		if (pops->reg >= 4)
			*pflags |= SECOND8;
		else
			*pflags |= FIRST8;
	}
	else if (pops->flags & OPTINFO_SIXTEEN) {
		*pflags &= ~TWOPOS;
		*pflags |= SIXTEEN;
	}
	return(ret);
}

int
save_optinfo(struct instable *dp, int reg, int regflags)
{
	if (dp->adr_mode == PREFIX) {
		Prefix[Nprefix++] = dp;
		return(0);
	}
	if (dp->adr_mode == OVERRIDE)
		give_up("Encountered override");
	if ((dp->optinfo.opgroup == 330) && (opsinfo[0].fmt == OPTINFO_REGONLY) && (opsinfo[0].reg == reg))
		dp = subst_xor_mov();
	SET_DP_SUB_I(Curblock, Curstatement, dp);
	if (reg == NO_REG) {
		opsinfo[REGOP].fmt = 0;
		opsinfo[REGOP].reg = NO_REG;
	}
	else {
		opsinfo[REGOP].reg = reg;
		opsinfo[REGOP].fmt = OPTINFO_REGONLY;
		opsinfo[REGOP].flags = regflags;
	}
	return(0);
}

static void
do_opinit()
{
	Optinfo = save_optinfo;
	obuf[0] = '\0';
	Opinit = 1;
	do_groups();
	growstack(NBASE + 100);
	STACK_RESET();
}

void
do_expr_subst()
{
	int i, j;
	int start;
	int delta_disp;
	int var;

	for (i = 0; i < NUMOPS; i++) {
		var = REFVAR(i);
		if (do_subst(i))
			var = REFVAR(i);
		if ((var < NBASE) && !(var == PSEUDO_REG_EARLY_STACK))
			continue;
		if (IS_INDEFINITE(opsinfo[i].reg) || ((opsinfo[i].index != NO_REG) && (IS_INDEFINITE(opsinfo[i].index))))
			continue;
		if (IS_ALIAS(opsinfo[i].reg))
			start = ALIAS(opsinfo[i].reg);
		else if ((opsinfo[i].index != NO_REG) && IS_ALIAS(opsinfo[i].index))
			start = ALIAS(opsinfo[i].index);
		else
			continue;
		delta_disp = 0;
		if (var > start) {
			for (j = start + 1; j < var; j++)
				if (IS_LOCATION_UNNEEDED(j))
					delta_disp += 4;
		}
		else {
			for (j = start; j > var; j--)
				if (IS_LOCATION_UNNEEDED(j))
					delta_disp -= 4;
		}
		opsinfo[i].u.disp.value += delta_disp;
		if (delta_disp)
			Opchanged = 1;
	}
}

void
set_partial_constant(int flags, int dest_var, int src_var)
{
	switch (PARTIAL(flags)) {
	case FIRST8:
		AND_CONSTANT(dest_var, 0xffffff00);
		OR_CONSTANT(dest_var, CONSTANT(src_var) & 0xff);
		break;
	case SECOND8:
		AND_CONSTANT(dest_var, 0xffff00ff);
		OR_CONSTANT(dest_var, (CONSTANT(src_var) & 0xff) << 8);
		break;
	case SIXTEEN:
		AND_CONSTANT(dest_var, 0xffff0000);
		OR_CONSTANT(dest_var, CONSTANT(src_var) & 0xffff);
		break;
	case 0:
		error("Internal error, exiting\n");
	}
}

void
analyze_copy_alias(struct instable *dp)
{
	int op = dp->optinfo.other & OPT_OPS;
	int dest;
	int i;
	int dest_opno, src_opno;
	int dest_flags, src_flags;
	int dest_var, src_var;
	int done = 0;

	src_opno = Srcs[0];
	src_var = REFVAR(Srcs[0]);
	src_flags = REFFLAGS(src_opno);
	for (dest = 0; (dest < NUMOPS) && (Dests[dest] != NO_SUCH_REG); dest++) {
		dest_opno = Dests[dest];
		dest_var = REFVAR(dest_opno);
		dest_flags = REFFLAGS(dest_opno);
		if (dest_flags & INDEXED) {
			done = 1;
			for (i = dest_var; !(Varinfo[i].flags & (BARRIER|VALUE_UNKNOWN_TO_CALLEE)) && (i >= NBASE); i--)
				ANYWHERE(i);
			for (i = dest_var + 1; (i != Varinfo[ESP].alias) && (i >= NBASE); i++) {
				ANYWHERE(i);
				if (Varinfo[i].flags & (BARRIER|VALUE_UNKNOWN_TO_CALLEE))
					break;
			}
			continue;
		}
		if (dest_flags & ANDBACK) {
			done = 1;
			for (i = dest_var; !(Varinfo[i].flags & (BARRIER|VALUE_UNKNOWN_TO_CALLEE)) && (i >= NBASE); i--)
				ANYWHERE(i);
			for (i = dest_var + 1; (i < Varinfo[ESP].alias) && (i >= NBASE); i++) {
				ANYWHERE(i);
				if (Varinfo[i].flags & (BARRIER|VALUE_UNKNOWN_TO_CALLEE))
					break;
			}
			continue;
		}
		if ((Srcs[1] != NO_SUCH_REG) || (Dests[1] != NO_SUCH_REG)) {
			ANYWHERE(dest_var);
			continue;
		}
		if (src_flags & ANDBACK) {
			done = 1;
			if (op == OPT_LEA)
				makealias(dest_var, src_var, src_flags);
			else
				ANYWHERE(dest_var);
			continue;
		}
		if (src_flags & INDEXED) {
			done = 1;
			ANYWHERE(dest_var);
			continue;
		}
	}

	if (done)
		return;
	if ((Srcs[1] != NO_SUCH_REG) || (Dests[1] != NO_SUCH_REG))
		return;
	if (Dests[0] != NO_SUCH_REG) {
		dest_var = REFVAR(Dests[0]);
		dest_flags = REFFLAGS(dest_opno);
		dest_opno = Dests[0];
		if (dest_var > Varinfo[ESP].alias)
			return;
	}
	if (Srcs[0] != NO_SUCH_REG) {
		if (src_var > Varinfo[ESP].alias) {
			ANYWHERE(dest_var);
			return;
		}
	}
	switch(op) {
	case OPT_PUSH:
		MARK_READ(ESP);
		MARK_WRITE(ESP);
/*		if (IS_PARTIAL(src_flags))*/
/*			give_up("Encountered partial push");*/
		GROWSTACK(1);
		dest_var = Varinfo[ESP].alias;
		MARK_WRITE(dest_var);
		makecopy(dest_var, src_var);
		break;
	case OPT_POP:
		MARK_READ(ESP);
		MARK_WRITE(ESP);
		if (IS_PARTIAL(dest_flags))
			give_up("Encountered partial pop");
		src_var = Varinfo[ESP].alias;
		DO_READ(src_var, 0);
		makecopy(dest_var, src_var);
		SHRINKSTACK(1);
		break;
	case OPT_CALL:
		break;
	case OPT_RET:
		break;
	case OPT_LEA:
		makealias(dest_var, src_var, src_flags);
		break;
	case OPT_MOV:
		if (IS_PARTIAL(dest_flags)) {
			not_a_copy(dest_var);
			if ((IS_CONSTANT(src_var)) && IS_CONSTANT(dest_var))
				set_partial_constant(dest_flags, dest_var, src_var);
			else
				ANYWHERE(dest_var);
		}
		else
			makecopy(dest_var, src_var);
		break;
	case OPT_SUB:
		/* FALLTHROUGH */
	case OPT_ADD:
		not_a_copy(dest_var);
		if (!IS_CONSTANT(src_var)) {
			ANYWHERE(dest_var);
			break;
		}
		switch (TYPE(dest_var)) {
		case ALIAS_TYPE:
			if (IS_CONSTANT(src_var)) {
				if (dest_var != ESP)
					MAKE_INDEXED(dest_var);
				else {
					if (op == OPT_ADD)
						ADD_TO_ALIAS(dest_var, CONSTANT(src_var));
					else
						SUBTRACT_FROM_ALIAS(dest_var, CONSTANT(src_var));
				}
			}
			else
				ANYWHERE(dest_var);
			break;
		case CONSTANT_TYPE:
/*			MAKE_INDEXED(dest_var);*/
/*#ifdef MOD_CONSTANT*/
			if (op == OPT_ADD)
				ADD_TO_CONSTANT(dest_var, CONSTANT(src_var));
			else
				SUBTRACT_FROM_CONSTANT(dest_var, CONSTANT(src_var));
/* #endif  /* MOD_CONSTANT */
		}
		break;
	case OPT_AND:
		not_a_copy(dest_var);
		switch (TYPE(dest_var)) {
		case ALIAS_TYPE:
			if (IS_CONSTANT(src_var))
				MAKE_INDEXED(dest_var);
#ifdef MOD_CONSTANT
				AND_ALIAS(dest_var, CONSTANT(src_var));
#endif  /* MOD_CONSTANT */
			else
				ANYWHERE(dest_var);
			break;
		case CONSTANT_TYPE:
			AND_CONSTANT(dest_var, CONSTANT(src_var));
		}
		break;
	default:
		if (Dests[0] == NO_SUCH_REG)
			break;
		if (((Srcs[0] != NO_SUCH_REG) && IS_ALIAS(src_var)) || IS_ALIAS(dest_var))
			ANYWHERE(dest_var);
		else
			NOT_STACK(dest_var);
	}
	clearvar(PSEUDO_REG_MEM);
	clearvar(PSEUDO_REG_CONSTANT);
	clearvar(PSEUDO_REG_EARLY_STACK);
}

void
interpret_optinfo(struct instable *dp)
{
	int shift;
	int ret, flags = 0;
	int rw = 0;
	int nsrcs = 0, ndests = 0;
	int i;

	if (dp->optinfo.other & FP)
		give_up("Encountered floating point");
	for (shift = 0, i = 0; i < NUMOPS; i++, shift++) {
		rw = dp->optinfo.other >> shift;
		if (!(rw & (RON|WON)) && (dp->optinfo.other & INCR)) {
			rw = RON|WON;
			opsinfo[i].fmt = OPTINFO_REGONLY;
			opsinfo[i].reg = ECX;
		}
		if (rw & (RON|WON)) {
			flags = 0;
			ret = arg_proc(dp->optinfo.other, &opsinfo[i], &flags);
			if ((Traversal_ctl & JUST_WATCHING_REGS) && (opsinfo[i].fmt != OPTINFO_REGONLY) && (opsinfo[i].fmt != OPTINFO_IMMEDIATE))
				continue;
			if ((FUNCNO(Curblock) != Enclosing_func) && (ret == PSEUDO_REG_EARLY_STACK))
				give_up("Encountered early stack reference in another function");
			if (ret == DANGEROUS_REF)
				give_up("Encountered dangerous reference");
			SET_REFVAR(i, ret);
			SET_REFFLAGS(i, flags);
			if (rw & WON) {
				DO_WRITE(ret, flags);
				Dests[ndests] = i;
				opsinfo[i].flags |= OPTINFO_DEST;
				ndests++;
				if ((rw & RON) || IS_PARTIAL(flags))
					DO_READ(ret, flags|CANT_SUBST);
			}
			else if (rw & RON) {
				if ((dp->optinfo.other & OPT_OPS) != OPT_LEA)
					DO_READ(ret, flags);
				Srcs[nsrcs] = i;
				opsinfo[i].flags |= OPTINFO_SRC;
				nsrcs++;
			}
		}
	}

	for ( ; ndests < NUMOPS; ndests++)
		Dests[ndests] = NO_SUCH_REG;

	for ( ; nsrcs < NUMOPS; nsrcs++)
		Srcs[nsrcs] = NO_SUCH_REG;

	if (!(Traversal_ctl & JUST_WATCHING_REGS) || ((opsinfo[Dests[0]].reg == ESP) && (opsinfo[Dests[0]].fmt == OPTINFO_REGONLY)))
		analyze_copy_alias(dp);
}

void
clearregs()
{
	int i;

	for (i = 0; i < NBASE; i++)
		clearvar(i);
}

void
cite_savings()
{
	int i;

	if (Traversal_ctl & JUST_WATCHING_REGS)
		return;
	if (VERBOSE4) {
		printf("%x: ESP is %d%s, ", Addr, Varinfo[ESP].alias, IS_ANDBACK(ESP) ? " and ANDBACK" : "");
		if (IS_ALIAS(EBP))
			printf("EBP is %d%s\n", Varinfo[EBP].alias, IS_ANDBACK(EBP) ? " and ANDBACK" : "");
		else
			printf("EBP is not an alias\n", Varinfo[EBP].alias);
	}
	for (i = NBASE; i < Varinfo[ESP].alias; i++) {
		if (IS_WATCHED_FOR_READ(i) && READ_VAR(i) && !WAS_WATCHED_READ(i)) {
			if (VERBOSE2)
				printf("\tRead watched variable at 0x%x: %s\n", Addr, PR(i));
			MARK_WATCHED_READ(i);
		}
		if (IS_WATCHED_FOR_WRITE(i) && WROTE_VAR(i) && !WAS_WATCHED_WRITTEN(i)) {
			if (VERBOSE2)
				printf("\tWrote watched variable at 0x%x: %s\n", Addr, PR(i));
			MARK_WATCHED_WRITTEN(i);
		}
	}
	FLUSH_OUT();
}

void
setup_gencode(int block)
{
	Genspot = 0;
	if (FLAGS(block) & GEN_TEXT) {
		Genbuf = GENBUF(block);
		Sgenbuf = SGENBUF(block);
		return;
	}
	Sgenbuf = (END_ADDR(block) - START_ADDR(block)) + 20;
	if (Sgenbuf >= SGENBUF(block)) {
		SET_SGENBUF(block, Sgenbuf);
		Genbuf = REALLOC(GENBUF(block), SGENBUF(block) * sizeof(char));
	}
	else {
		Genbuf = GENBUF(block);
		Sgenbuf = SGENBUF(block);
	}
	if (ENDREL(block) - REL(block)) {
		SET_SGENREL(block, (ENDREL(block) - REL(block)) * 2 + 1);
		SET_GENREL(block, REALLOC(GENREL(block), SGENREL(block) * sizeof(Elf32_Rel)));
	}
	else {
		SET_SGENREL(block, 0);
		SET_GENREL(block, NULL);
	}
}

void
opdecode_block(int block)
{
	int i;
	Elf32_Addr end_addr;

	Curblock = block;
	if (!Sstatements || ((Sstatements - Nstatements) < (END_ADDR(block) - START_ADDR(block)))) {
		Sstatements += max(200, END_ADDR(block) - START_ADDR(block)) + 2;
		Statements = REALLOC(Statements, Sstatements * sizeof(struct statement));
	}
	SET_STATEMENT_START(block, Nstatements);
	if (START_ADDR(block) == END_ADDR(block))
		return;
	Addr = Next_addr = START_ADDR(block);
	end_addr = Addr + CODELEN(block);
	if (END_TYPE(block) == NEW_END_TYPE(block))
		end_addr += END_INSTLEN(block);
	for (i = 0; Next_addr < end_addr; i++, Addr = Next_addr) {
		Curstatement = i;
		Shdr.sh_addr = Addr;
		p_data = TEXTBUF(block) + Addr - START_ADDR(block);
		Nprefix = 0;
		Prefix = PREFIX_SUB_I(block, i);
		opsinfo = OPSINFO_SUB_I(block, i);
		SET_ADDR_SUB_I(block, i, Addr);
		SET_FLAGS_SUB_I(block, i, 0);
		dis_text(&Shdr);
		Next_addr = loc;
		Prefix[Nprefix] = NULL;
		Nstatements++;
	}
	SET_NSTATEMENTS(block, i);
}

struct instable *
proc_statement(int block, int i, int interp)
{
	struct instable *dp;

	Curstatement = i;
	Addr = ADDR_SUB_I(block, i);
	opsinfo = OPSINFO_SUB_I(block, i);
	dp = DP_SUB_I(block, i);
	if (VERBOSE3) {
		FLUSH_OUT();
		dis_me(TEXTBUF(Curblock) + Addr - START_ADDR(Curblock), Addr, 1);
		FLUSH_OUT();
	}
	if (interp)
		interpret_optinfo(dp);
	return(dp);
}

static int Encountered_new_func;

proc_block_analyze_esr(int block)
{
	int i;
	int op;
	struct instable *dp;
	int reg;

	Curblock = block;
	if (!DECODED(block))
		opdecode_block(block);
	for (i = 0; i < NSTATEMENTS(block); i++) {
		static int npushfound;

		dp = proc_statement(block, i, 0);
		op = dp->optinfo.other & OPT_OPS;
		if ((op == OPT_POP) && IS_WATCHED_FOR_READ_ON_PATH(Varinfo[ESP].alias)) {
			for (reg = 0; reg < NREGS; reg++) {
				if (Esr_watch[reg].pushvar != Varinfo[ESP].alias)
					continue;
				if (reg != opsinfo[REGOP].reg)
					error("Internal error, exiting\n");
				if (VERBOSE3)
					printf("Found pop of %s\n", Prreg[reg].name);
				if (READ_VAR(Varinfo[ESP].alias)) {
					if (VERBOSE3)
						printf("Lost elimination of %s, because %s was read\n", Prreg[reg].name, PR(Esr_watch[reg].pushvar));
					Esr_watch[reg].nelim = 0;
					Esr_watch[reg].pushvar = -1;
					Esr_watch[reg].flags &= ~(ESR_POPPED|ESR_ELIM);
				}
				else {
					UNMARK_READ(reg);
					MARK_WRITE(reg);
					MAKE_NOT_WATCHED_ON_PATH(Varinfo[ESP].alias);
					if (Esr_watch[reg].nelim == 9) {
						Esr_watch[reg].nelim = 0;
						Esr_watch[reg].pushvar = -1;
						Esr_watch[reg].flags &= ~(ESR_POPPED|ESR_ELIM);
					}
					else {
						int j;

						for (j = 0; j < Esr_watch[reg].nelim; j++)
							if (Esr_watch[reg].elim[j] == Addr)
								break;
						if (j == Esr_watch[reg].nelim)
							Esr_watch[reg].elim[Esr_watch[reg].nelim++] = Addr;
						Esr_watch[reg].flags |= ESR_POPPED;
						MARK_POPPED(reg);
					}
				}
				break;
			}
			shrinkstack(1);
			continue;
		}
		interpret_optinfo(dp);
		if (VERBOSE3) {
			int j;

			for (j = NBASE; j < Varinfo[ESP].alias; j++)
				if (IS_WATCHED_FOR_READ_ON_PATH(j) && READ_VAR(j))
					printf("\tRead watched variable at 0x%x: %s\n", Addr, PR(j));
		}
		if ((op == OPT_PUSH) && (opsinfo[REGOP].fmt == OPTINFO_REGONLY)) {
			reg = opsinfo[REGOP].reg;
			if (!WROTE_VAR(reg) && (Esr_watch[reg].flags & ESR_ELIM)) {
				if (Esr_watch[reg].pushvar == -1) {
					UNMARK_READ(Varinfo[ESP].alias);
					MAKE_WATCHED_FOR_READ_ON_PATH(Varinfo[ESP].alias);
					if (Esr_watch[reg].nelim == 9) {
						Esr_watch[reg].nelim = 0;
						Esr_watch[reg].pushvar = -1;
						Esr_watch[reg].flags &= ~(ESR_POPPED|ESR_ELIM);
					}
					else
						Esr_watch[reg].elim[Esr_watch[reg].nelim++] = Addr;
					Esr_watch[reg].pushvar = Varinfo[ESP].alias;
					if (VERBOSE3)
						printf("Found push of %s\n", Prreg[reg].name);
				}
				Varinfo[Varinfo[ESP].alias].flags |= BARRIER;
			}
		}
	}
	if (NEW_END_TYPE(block) == END_PUSH)
		Encountered_new_func = 1;
	return(0);
}

proc_block_analyze_ea(int block)
{
	int i, j;
	struct instable *dp;

	Curblock = block;
	if (!DECODED(block))
		opdecode_block(block);
	for (i = 0; i < NSTATEMENTS(block); i++) {
		dp = proc_statement(block, i, 1);
		for (j = 0; j < NUMOPS; j++) {
			if (!IS_SRC(j))
				continue;
			if ((dp->optinfo.other & OPT_OPS) == OPT_LEA) {
				MARK_READ(REFVAR(j));
				continue;
			}
			if (!try_subst(j)) {
				if (VERBOSE2)
					printf("\tCannot do substitution because we cannot find a matching format at 0x%x\n", Addr);
				MARK_READ(REFVAR(j));
			}
		}
		if (Check_each_statement && Check_each_statement(dp, 0))
			return(1);
		cite_savings();
	}
	if (Check_each_block && Check_each_block())
		return(1);
	return(0);
}

void
block_changed(int block)
{
	if (FLAGS(block) & GEN_THIS_BLOCK)
		return;
	ADD_FLAGS(block, GEN_THIS_BLOCK);
	if (Nchanged_blocks == Schanged_blocks) {
		Schanged_blocks += 50;
		Changed_blocks = REALLOC(Changed_blocks, Schanged_blocks * sizeof(int));
	}
	Changed_blocks[Nchanged_blocks++] = block;
}

proc_block_substitution(int block)
{
	int taken;
	int i;
	int cte;
	int prevstack;
	int nstatements = 0;
	int changed = 0;
	struct instable *dp, *newdp;

	Curblock = block;
	if (!DECODED(block))
		opdecode_block(block);
	for (i = 0; i < NSTATEMENTS(block); i++) {
		prevstack = Varinfo[ESP].alias;
		dp = proc_statement(block, i, 1);
		Opchanged = 0;
		if (cte = try_cte(&taken)) {
			SET_NSTATEMENTS(block, NSTATEMENTS(block)-1);
			SET_NEW_END_TYPE(Curblock, END_JUMP);
			if (!taken)
				SET_JUMP_TARGET(Curblock, FALLTHROUGH(Curblock));
		}
		else
			do_expr_subst();
		if (!cte && (!Check_each_statement || !Check_each_statement(dp, prevstack))) {
			newdp = subst_dp(1);
			if (!changed && ((newdp != dp) || Opchanged)) {
				changed = 1;
				block_changed(block);
			}
			if (nstatements != i)
				COPY_SUB_I(Curblock, nstatements, i);
			SET_DP_SUB_I(Curblock, nstatements++, newdp);
		}
		else {
			if (VERBOSE3)
				printf("Instruction eliminated: 0x%x\n", Addr);
			if (!changed) {
				changed = 1;
				block_changed(block);
			}
		}
		if (VERBOSE4) {
			printf("%x: ESP is %d%s, ", Addr, Varinfo[ESP].alias, IS_ANDBACK(ESP) ? " and ANDBACK" : "");
			if (IS_ALIAS(EBP))
				printf("EBP is %d%s\n", Varinfo[EBP].alias, IS_ANDBACK(EBP) ? " and ANDBACK" : "");
			else
				printf("EBP is not an alias\n", Varinfo[EBP].alias);
		}
	}
	SET_NSTATEMENTS(block, nstatements);
	if (Check_each_block && Check_each_block())
		return(1);
	return(0);
}

void
proc_block_gentext(int block)
{
	int i;
	int j;
	int nrel;
	Elf32_Addr new_start_addr;
	Elf32_Addr prev_genspot;
	char *textbuf;

	if (VERBOSE3) {
		if (SGENBUF(block)) {
			textbuf = MALLOC(SGENBUF(block));
			memcpy(textbuf, GENBUF(block), SGENBUF(block));
		}
		else
			textbuf = TEXTBUF(block);
	}
	Curblock = block;
	setup_gencode(block);
	if (block < Ndecodeblocks)
		new_start_addr = START_ADDR(block);
	else
		new_start_addr = START_ADDR(Nblocks);
	nrel = 0;

	for (i = 0; i < NSTATEMENTS(block); i++) {
		Curstatement = i;
		Addr = ADDR_SUB_I(block, i);
		opsinfo = OPSINFO_SUB_I(block, i);
		if (VERBOSE3) {
			FLUSH_OUT();
			dis_me(textbuf + Addr - START_ADDR(Curblock), Addr, 1);
			FLUSH_OUT();
		}
		prev_genspot = Genspot;
		call_gencode(DP_SUB_I(block, i));
		for (j = 0; j < NUMOPS; j++) {
			if (opsinfo[j].rel) {
				if (nrel >= SGENREL(block)) {
					SET_SGENREL(block, (SGENREL(block) * 2) + 4);
					SET_GENREL(block, REALLOC(GENREL(block), SGENREL(block) * sizeof(Elf32_Rel)));
				}
				GENREL(block)[nrel] = ((Elf32_Rel *) opsinfo[j].rel)[0];
				GENREL(block)[nrel].r_offset = new_start_addr + opsinfo[j].u.imm.loc;
				nrel++;
			}
		}
		if (VERBOSE3) {
			FLUSH_OUT();
			printf("Genned: ");
			if (prev_genspot == Genspot)
				printf("nothing\n");
			else
				dis_me(Genbuf + prev_genspot, Addr, 1);
			FLUSH_OUT();
		}
	}
	if (VERBOSE3 && (textbuf != TEXTBUF(block)))
		free(textbuf);
	ADD_FLAGS(block, GEN_TEXT);
	SET_SGENBUF(block, Sgenbuf);
	SET_GENBUF(block, Genbuf);
	SET_GENLEN(block, Genspot);
	if (END_TYPE(block) != NEW_END_TYPE(block)) {
		memset(GENBUF(block) + Genspot, NOP, END_INSTLEN(block));
		ADDTO_GENLEN(block, END_INSTLEN(block));
	}
	if (block >= Ndecodeblocks) {
		SET_START_ADDR(block, START_ADDR(Nblocks));
		SET_END_ADDR(block, START_ADDR(block) + GENLEN(block));
		SET_START_ADDR(Nblocks, END_ADDR(block));
	}
	SET_REL(block, GENREL(block));
	SET_ENDREL(block, GENREL(block) + nrel);
	Genbuf = NULL;
	Genspot = 0;
}

find_watched(int index, int andback, int forward)
{
	int bottom, top;
	int i;

	if (andback)
		top = index;
	else {
		for (i = index; i < Varinfo[ESP].alias; i++)
			if (Varinfo[i].flags & (BARRIER|VALUE_UNKNOWN_TO_CALLEE))
				break;
		top = i;
	}
	for (i = index; i > NBASE; i--)
		if (Varinfo[i].flags & (BARRIER|VALUE_UNKNOWN_TO_CALLEE))
			break;
	bottom = i + 1;

	for (i = bottom; i <= top; i++) {
		if (IS_WATCHED_FOR_READ(i)) {
			if (VERBOSE2)
				printf("Found modifiable watched at 0x%x: %s\n", Addr, PR(i));
			return(1);
		}
		if (IS_ALIAS(i)) {
			if (forward) {
				if (ALIAS(i) < i)
					continue;
			}
			else
				if (ALIAS(i) > i)
					continue;
			if ((ALIAS(i) <= top) && (ALIAS(i) >= bottom))
				continue;
			if (ALIAS(i) > Varinfo[ESP].alias)
				continue;
			if (find_watched(ALIAS(i), GET_ANDBACK(i), forward)) {
				if (VERBOSE2)
					printf("Found descendent may have modified watched at 0x%x: %s\n", Addr, PR(i));
				return(1);
			}
		}
	}
	return(0);
}

/*
** Here's the logic:  if there is a barrier between the current point
** on the stack and anything that is being watched, then there is no
** reason to go into the function.  If nothing is being watched, then
** we are traversing for another reason and should descend into the
** function.
*/
should_i_call(int block)
{
	if (!Nwatchinfo)
		return(1);
	else
		return(find_watched(Varinfo[ESP].alias, ANDBACK_FLAG, 1) || find_watched(Varinfo[ESP].alias, ANDBACK_FLAG, 0));
}

not_copies(int index, int andback, int forward)
{
	int bottom, top;
	int i;

	if (andback)
		top = index;
	else {
		for (i = index; i < Varinfo[ESP].alias; i++)
			if (Varinfo[i].flags & (BARRIER|VALUE_UNKNOWN_TO_CALLEE))
				break;
		top = i;
	}
	for (i = index; i > NBASE; i--)
		if (Varinfo[i].flags & (BARRIER|VALUE_UNKNOWN_TO_CALLEE))
			break;
	bottom = i + 1;

	for (i = bottom; i <= top; i++) {
		not_a_copy(i);
		if (IS_ALIAS(i)) {
			if (forward) {
				if (ALIAS(i) < i)
					continue;
			}
			else
				if (ALIAS(i) > i)
					continue;
			if ((ALIAS(i) <= top) && (ALIAS(i) >= bottom))
				continue;
			not_copies(ALIAS(i), GET_ANDBACK(i), forward);
		}
	}
}

/*
** Here's the logic:  if there is a barrier between the current point
** on the stack and anything that is being watched, then there is no
** reason to go into the function.  If nothing is being watched, then
** we are traversing for another reason and should descend into the
** function.
*/
void
potential_args_are_not_copies()
{
	not_copies(Varinfo[ESP].alias, ANDBACK_FLAG, 0);
}

void
going(int srcblock, int targblock)
{
	if (!VERBOSE3)
		return;
	if (FUNCNO(srcblock) != FUNCNO(targblock))
		printf("Going from function %s%s%s to %s%s%s\n", FULLNAME(Funcs[FUNCNO(srcblock)]), FULLNAME(Funcs[FUNCNO(targblock)]));
	printf("Going from 0x%x to 0x%x\n", END_ADDR(srcblock), START_ADDR(targblock));
}

void
restore_saved_without_map(int *saved_vars, int func)
{
	int block;

	for (block = Func_info[func].start_block; block < Func_info[func].end_block; block++)
		Pstatements[block].entry_vars = saved_vars[block - Func_info[func].start_block];
}

void
restore_saved_with_map(int *saved_vars, int func)
{
	int i, block;

	for (i = 0; i < (int) Func_info[func].clen; i++) {
		block = Func_info[func].map[i];
		Pstatements[block].entry_vars = saved_vars[i];
	}
}

void
restore_func_saved_vars(int *saved_vars, int func)
{
	if (Func_info[func].map)
		restore_saved_with_map(saved_vars, func);
	else
		restore_saved_without_map(saved_vars, func);
	free(saved_vars);
}


void
clear_saved_without_map(int **psaved_vars, int func)
{
	int block;

	for (block = Func_info[func].start_block; block < Func_info[func].end_block; block++) {
		if (IS_INIT_ENTRYVARS(block)) {
			if (psaved_vars) {
				if (!*psaved_vars)
					*psaved_vars = (int *) CALLOC((Func_info[func].end_block - Func_info[func].start_block), sizeof(int));
				(*psaved_vars)[block - Func_info[func].start_block] = Pstatements[block].entry_vars;
			}
			Pstatements[block].entry_vars = 0;
		}
	}
}

void
clear_saved_with_map(int **psaved_vars, int func)
{
	int i, block;

	for (i = 0; i < (int) Func_info[func].clen; i++) {
		block = Func_info[func].map[i];
		if (IS_INIT_ENTRYVARS(block)) {
			if (psaved_vars) {
				if (!*psaved_vars)
					*psaved_vars = (int *) CALLOC(Func_info[func].clen, sizeof(int));
				(*psaved_vars)[i] = Pstatements[block].entry_vars;
			}
			Pstatements[block].entry_vars = 0;
		}
	}
}

void
clear_func_saved_vars(int **psaved_vars, int func)
{
	if (Func_info[func].map)
		clear_saved_with_map(psaved_vars, func);
	else
		clear_saved_without_map(psaved_vars, func);
}

void
clear_without_map(int func)
{
	int i;

	for (i = Func_info[func].start_block; i < Func_info[func].end_block; i++)
		DEL_FLAGS(i, BEENHERE|BEENHERE_ON_PATH);
}

void
clear_with_map(int func)
{
	int i, j;

	for (i = 0; i < (int) Func_info[func].clen; i++) {
		j = Func_info[func].map[i];
		DEL_FLAGS(j, BEENHERE|BEENHERE_ON_PATH);
	}
}

void
clear_func_beenhere(int func)
{
	if (Func_info[func].map)
		clear_with_map(func);
	else
		clear_without_map(func);
}

void
setup_proc_func(int func)
{
	if (Func_info[func].flags & FUNC_BEENHERE)
		return;
	Func_info[func].flags |= FUNC_BEENHERE;
	if (Nfuncs_beenhere == Sfuncs_beenhere) {
		Sfuncs_beenhere += 10;
		Funcs_beenhere = REALLOC(Funcs_beenhere, Sfuncs_beenhere * sizeof(int));
	}
	Funcs_beenhere[Nfuncs_beenhere++] = func;
	clear_func_beenhere(func);
}

void
clear_funcs_beenhere()
{
	int i;

	for (i = 0; i < Nfuncs_beenhere; i++) {
		clear_func_beenhere(Funcs_beenhere[i]);
		clear_func_saved_vars(NULL, Funcs_beenhere[i]);
		Func_info[Funcs_beenhere[i]].flags &= ~FUNC_BEENHERE;
	}
	Nfuncs_beenhere = 0;
}

void
push_blockno(int constant)
{
	int dest_var;

	GROWSTACK(1);
	dest_var = Varinfo[ESP].alias;
	MARK_WRITE(dest_var);
	makeconstant(dest_var, constant, 0);
	MARK_AS_BLOCKNO(dest_var);
}

pop_const()
{
	int constant;
	int src_var;
	
	src_var = Varinfo[ESP].alias;
	constant = CONSTANT(src_var);
	DO_READ(src_var, 0);
	SHRINKSTACK(1);
	return(constant);
}

detailed_comparison(struct varinfo *var1, int nvar1, struct varinfo *var2, int nvar2)
{
	int i;

	if (nvar1 != nvar2) {
		printf("Varinfo has %d entries, entryvars has %d\n", nvar1, nvar2);
		return;
	}
	for (i = 0; i < nvar1; i++)
		if (memcmp(var1 + i, var2 + i, sizeof(struct varinfo))) {
			printf("Variable %s has changed", PR(i));
			if (var1[i].type != var2[i].type)
				printf(" type\n");
			else if (var1[i].flags != var2[i].flags)
				printf(" flags\n");
			else if (var1[i].u.val != var2[i].u.val)
				printf(" value\n");
			else if (var1[i].u.rel != var2[i].u.rel)
				printf(" rel\n");
			else if (var1[i].prevcopy != var2[i].prevcopy)
				printf(" prevcopy\n");
			else if (var1[i].nextcopy != var2[i].nextcopy)
				printf(" nextcopy\n");
			else if (var1[i].alias != var2[i].alias)
				printf(" alias\n");
		}
}

void save_entryvars(int block);
int Changed_max = 1000;

#ifndef DONT_DEBUG_CHANGES
#define CHANGED() do {\
	if (nvisits > Changed_max) opdebug();\
	changed++;\
} while(0)
#else
#define CHANGED() changed++
#endif

merge_vars(int block)
{
	int wasbadalready;
	int i;
	int changed = 0;
	int nmerge = min(NENTRYVARS(block), Varinfo[ESP].alias + 1);
	struct varinfo *entryvars = ENTRYVARS(block);
	int nvisits;

	if (!IS_INIT_ENTRYVARS(block)) {
		save_entryvars(block);
		return(1);
	}
	nvisits = NVISITS(block);
	nvisits++;
	SET_NVISITS(block, nvisits);
/*	if ((Varinfo[EBP].flags & ANDBACK_FLAG) || (Varinfo[ESP].flags & ANDBACK_FLAG))*/
/*		wasbadalready = 1;*/
/*	else*/
/*		wasbadalready = 0;*/
	for (i = 0; i < nmerge; i++) {
		if (Varinfo[i].flags != entryvars[i].flags) {
			if ((Varinfo[i].flags & ON_IS_CONSERVATIVE) != (entryvars[i].flags & ON_IS_CONSERVATIVE)) {
				if (((entryvars[i].flags & ON_IS_CONSERVATIVE) | (Varinfo[i].flags & ON_IS_CONSERVATIVE)) == (entryvars[i].flags & ON_IS_CONSERVATIVE))
					Varinfo[i].flags |= entryvars[i].flags & ON_IS_CONSERVATIVE;
				else {
					CHANGED();
					Varinfo[i].flags |= entryvars[i].flags & ON_IS_CONSERVATIVE;
					entryvars[i].flags |= Varinfo[i].flags & ON_IS_CONSERVATIVE;
				}
			}
			if ((Varinfo[i].flags & OFF_IS_CONSERVATIVE) != (entryvars[i].flags & OFF_IS_CONSERVATIVE)) {
				if (((entryvars[i].flags & OFF_IS_CONSERVATIVE) | (Varinfo[i].flags & OFF_IS_CONSERVATIVE)) == (Varinfo[i].flags & OFF_IS_CONSERVATIVE))
					Varinfo[i].flags = (Varinfo[i].flags & ~OFF_IS_CONSERVATIVE) | (entryvars[i].flags & OFF_IS_CONSERVATIVE);
				else {
					CHANGED();
					Varinfo[i].flags = (Varinfo[i].flags & ~OFF_IS_CONSERVATIVE) | ((Varinfo[i].flags & OFF_IS_CONSERVATIVE) & (entryvars[i].flags & OFF_IS_CONSERVATIVE));
					entryvars[i].flags = (entryvars[i].flags & ~OFF_IS_CONSERVATIVE) | (Varinfo[i].flags & OFF_IS_CONSERVATIVE);
				}
			}
		}
		if (Varinfo[i].nextcopy != entryvars[i].nextcopy) {
			if (entryvars[i].nextcopy == i)
				not_a_copy(i);
			else {
				CHANGED();
				not_a_copy(i);
				entryvars[i].nextcopy = i;
				entryvars[i].prevcopy = i;
			}
		}
		if (!memcmp(entryvars + i, Varinfo + i, sizeof(struct varinfo)))
			continue;
		switch (entryvars[i].type) {
		case CONSTANT_TYPE:
			if (IS_CONSTANT(i)) {
				if (entryvars[i].flags & INDEXED_FLAG)
					Varinfo[i] = entryvars[i];
				else {
					CHANGED();
					MAKE_INDEXED(i);
				}
			}
			else if (IS_REL_CONSTANT(i) || IS_NOT_STACK(i) || IS_UNKNOWN(i)) {
				CHANGED();
				NOT_STACK(i);
			}
			else {
				CHANGED();
				ANYWHERE(i);
			}
			break;
		case ALIAS_TYPE:
			if (i == ESP) {
				if (VERBOSE3)
					printf("ESP has changed at merge point at 0x%x\n", START_ADDR(block));
				give_up("ESP has changed at merge point");
			}
			if (IS_UNKNOWN(i)) {
				NOT_STACK(i);
				if (!(entryvars[i].flags & ANDBACK_FLAG))
					CHANGED();
				break;
			}
			if (entryvars[i].flags & INDEXED_FLAG) {
				Varinfo[i] = entryvars[i];
				break;
			}
			if (entryvars[i].flags & ANDBACK_FLAG) {
				if ((IS_ALIAS(i) && (ALIAS(i) <= entryvars[i].alias)) ||
					(entryvars[i].alias >= Varinfo[ESP].alias))
						Varinfo[i] = entryvars[i];
				else if (IS_ALIAS(i) && (ALIAS(i) > entryvars[i].alias) && IS_ANDBACK(i)) {
					CHANGED();
				}
				else {
					CHANGED();
					ANYWHERE(i);
				}
				break;
			}
			if (IS_ALIAS(i)) {
				if (ALIAS(i) < entryvars[i].alias) {
					makealias(i, entryvars[i].alias, ANDBACK);
					CHANGED();
				}
				else
					MAKE_ANDBACK(i);
				break;
			}
			CHANGED();
			ANYWHERE(i);
			break;
		case NOT_STACK_TYPE:
			if (IS_UNKNOWN(i))
				NOT_STACK(i);
			else if (!IS_NOT_STACK(i)) {
				if (!(IS_CONSTANT(i) || IS_REL_CONSTANT(i)))
					CHANGED();
			}
			break;
		case UNKNOWN_TYPE:
			if (IS_UNKNOWN(i))
				NOT_STACK(i);
			else if (!IS_NOT_STACK(i)) {
				if (IS_CONSTANT(i) || IS_REL_CONSTANT(i))
					NOT_STACK(i);
				CHANGED();
			}
			break;
		case REL_CONSTANT_TYPE:
			CHANGED();
			if (IS_CONSTANT(i) || IS_REL_CONSTANT(i) || IS_UNKNOWN(i))
				NOT_STACK(i);
			break;
		}
	}
	if (NENTRYVARS(block) < Varinfo[ESP].alias + 1) {
		CHANGED();
		SET_NENTRYVARS(block, Varinfo[ESP].alias + 1);
		if (NENTRYVARS(block) > SENTRYVARS(block)) {
			SET_SENTRYVARS(block, NENTRYVARS(block));
			SET_ENTRYVARS(block, REALLOC(ENTRYVARS(block), SENTRYVARS(block) * sizeof(struct varinfo)));
		}
		memcpy(ENTRYVARS(block), Varinfo, NENTRYVARS(block) * sizeof(struct varinfo));
	}
	else if (NENTRYVARS(block) > Varinfo[ESP].alias + 1)
		give_up("Stack pointer is different on different executions of block");
	else
		memcpy(ENTRYVARS(block), Varinfo, NENTRYVARS(block) * sizeof(struct varinfo));
	return(changed);

}

int
regs_entryvars_unchanged(int block)
{
	int i;
	struct varinfo *entryvars = ENTRYVARS(block);
	
	if (NENTRYVARS(block) != NREGS)
		return(0);
	for (i = 0; i < NREGS; i++)
		if (IS_WATCHED_FOR_READ_ON_PATH(i) && !(entryvars[i].flags & WATCH_READ))
			return(0);
	return(1);
}

void
regs_save_entryvars(int block)
{
	int i;
	struct varinfo *entryvars;

	SET_NENTRYVARS(block, NREGS);
	if (NENTRYVARS(block) > SENTRYVARS(block)) {
		SET_SENTRYVARS(block, NENTRYVARS(block));
		SET_ENTRYVARS(block, REALLOC(ENTRYVARS(block), SENTRYVARS(block) * sizeof(struct varinfo)));
		memcpy(ENTRYVARS(block), Varinfo, NENTRYVARS(block) * sizeof(struct varinfo));
	}
	entryvars = ENTRYVARS(block);
	for (i = 0; i < NREGS; i++)
		if (IS_WATCHED_FOR_READ_ON_PATH(i))
			entryvars[i].flags |= WATCH_READ;
		else
			entryvars[i].flags &= ~WATCH_READ;
}

void
save_entryvars(int block)
{
	if (!IS_INIT_ENTRYVARS(block)) {
		if (Nholdvars == Sholdvars) {
			Sholdvars += 20;
			Holdvars = REALLOC(Holdvars, Sholdvars * sizeof(struct holdvars));
			memset(Holdvars + Nholdvars, '\0', 20 * sizeof(struct holdvars));
		}
		if (Nholdvars == 0)
			Nholdvars = 1;
		Pstatements[block].entry_vars = Nholdvars;
		Nholdvars++;
	}
	if (Traversal_ctl & JUST_WATCHING_REGS) {
		regs_save_entryvars(block);
		return;
	}
	SET_NENTRYVARS(block, Varinfo[ESP].alias);
	if (NENTRYVARS(block) > SENTRYVARS(block)) {
		SET_SENTRYVARS(block, NENTRYVARS(block));
		SET_ENTRYVARS(block, REALLOC(ENTRYVARS(block), SENTRYVARS(block) * sizeof(struct varinfo)));
	}
	memcpy(ENTRYVARS(block), Varinfo, NENTRYVARS(block) * sizeof(struct varinfo));
}

int
entryvars_unchanged(int block)
{
	struct varinfo *entryvars = ENTRYVARS(block);
	int nentryvars = NENTRYVARS(block);

	if (Traversal_ctl & JUST_WATCHING_REGS) {
		if (regs_entryvars_unchanged(block))
			return(1);
		save_entryvars(block);
		return(0);
	}
	else {
		static int first_time = 1;
		static int new_merge;

		if (first_time) {
			first_time = 0;
			
			new_merge = !getenv("OLD_MERGE");
		}
		if (new_merge)
			return(!merge_vars(block));
		if (!((nentryvars == Varinfo[ESP].alias) && (memcmp(entryvars, Varinfo, nentryvars * sizeof(struct varinfo)) == 0))) {
			save_entryvars(block);
			return(0);
		}
	}
}

/*
* Process the block and continue on to its successors.
*
* A confusing part of this is the two different visiting rules:  visit
* once and visit on paths.  In "visit once" mode, each block is only
* processed once; this is appropriate for code generation-like
* traversals where you clearly only want to generate code once.  In
* "visit on paths" mode, blocks may be visited many times, visiting is
* tracked on a path-by-path basis.  Further, in some traversals, we can
* allow nodes to be visited twice on paths, but no more.  This takes
* into account the effects of separate iterations of loops.
*/
proc_all_paths(int block)
{
	struct varinfo *hold;
	int holdsize;
	int holdESP;
	int holdNdelete;
	int f, t;
	int ret1 = 1, ret2 = 1;
	int ret;

	if (!(Traversal_ctl & VISIT_UNTIL_DONE) && (FLAGS(block) & BEENHERE_ON_PATH)) {
		if (VERBOSE3)
			printf("Not going to 0x%x\n", START_ADDR(block));
		return(1);
	}
	if (!(Traversal_ctl & VISIT_ON_PATHS) && (FLAGS(block) & BEENHERE)) {
		if (VERBOSE3)
			printf("Not going to 0x%x\n", START_ADDR(block));
		return(1);
	}
	if (!(FLAGS(block) & BEENHERE))
		save_entryvars(block);
	else if (entryvars_unchanged(block)) {
		if (VERBOSE3)
			printf("Not going to 0x%x\n", START_ADDR(block));
		return(1);
	}
	/* Notice that we Proc_block BEFORE setting BEENHERE.  The
	** analysis phase expects to see a block multiple times and
	** needs to know whether it is the first time.
	*/
/*	if (VERBOSE1)*/
/*		printf("Processing block %d\n", block);*/
	if (VERBOSE4) {
		int i;

		Travstack[Depth] = START_ADDR(block);
		printf("Stack: ");
		for (i = 0; i <= Depth; i++)
			printf("0x%x ", Travstack[i]);
		printf("\n");
	}
	if (Nvisits++ > 100000)
		give_up("Max number of visits exceeded");
/*	printf("Entering block at 0x%x, ESP is %d\n", START_ADDR(block), Varinfo[ESP].alias);*/
	ret = Proc_block(block);
	ADD_FLAGS(block, BEENHERE);
	ADD_FLAGS(block, BEENHERE_ON_PATH);
	if (ret) {
		if (VERBOSE3)
			printf("Falling back from 0x%x\n", START_ADDR(block));
		return(1);
	}
	if (Depth++ > 1000)
		error("I think we got us an infinite loop\n");
	switch(NEW_END_TYPE(block)) {
	case END_CJUMP:
		f = FALLTHROUGH(block) != NO_SUCH_ADDR;
		t = JUMP_TARGET(block) != NO_SUCH_ADDR;
		break;
	case END_JUMP:
		t = JUMP_TARGET(block) != NO_SUCH_ADDR;
		f = 0;
		break;
	case END_RET:
		if (Traversal_ctl & NEVER_FOLLOW_RET)
			f = 0;
		else {
			int fallthrough;

			if (IS_VAR_BLOCKNO(Varinfo[ESP].alias)) {
				fallthrough = pop_const();
				push_blockno(fallthrough);
/*				if (((Traversal_ctl & FOLLOW_RET) || (fallthrough != Retblock)) && ((FUNCNO(fallthrough) == Enclosing_func) || (FUNCNO(block) == FUNCNO(fallthrough))))*/
				if ((Traversal_ctl & FOLLOW_RET) || (fallthrough != Retblock))
					f = 1;
				else
					f = 0;
			}
			else
				f = 0;
		}
		t = 0;
		break;
	case END_POP:
		if (Traversal_ctl & NEVER_FOLLOW_RET)
			f = 0;
/*			if (IS_VAR_BLOCKNO(Varinfo[ESP].alias) && ((Traversal_ctl & FOLLOW_RET) || (FALLTHROUGH(block) != Retblock)) && ((FUNCNO(FALLTHROUGH(block)) == Enclosing_func) || (FUNCNO(block) == FUNCNO(FALLTHROUGH(block)))))*/
		else {
			if (IS_VAR_BLOCKNO(Varinfo[ESP].alias) && ((Traversal_ctl & FOLLOW_RET) || (FALLTHROUGH(block) != Retblock)))
				f = 1;
			else
				f = 0;
		}
		t = 0;
		break;
	case END_CALL:
		clear_eax_ecx_edx();
		potential_args_are_not_copies();
		f = FALLTHROUGH(block) != NO_SUCH_ADDR;
/*#ifdef FOLLOW_CALLS*/
		if ((Traversal_ctl & FOLLOW_CALLS) && should_i_call(block)) {
			if (JUMP_TARGET(block) == NO_SUCH_ADDR) {
				if (VERBOSE3)
					printf("Untraceable call, but I want to call at 0x%x\n", END_ADDR(block) - END_INSTLEN(block));
				give_up("Encountered untraceable call, but I want to call");
				t = 0;
			}
			else
			t = 1;
		}
		else
/*#endif*/
			t = 0;
		break;
	case END_PUSH:
		if (END_TYPE(block) != END_PUSH)
			push_blockno(JUMP_TARGET(block));
		f = FALLTHROUGH(block) != NO_SUCH_ADDR;
		t = 0;
		break;
	case END_FALL_THROUGH:
	case END_PSEUDO_CALL:
		f = FALLTHROUGH(block) != NO_SUCH_ADDR;
		t = 0;
		break;
	case END_IJUMP:
		if ((Traversal_ctl & DO_NOT_GIVE_UP_JUST_END) && Call_at_end) {
			Call_at_end();
			if (VERBOSE3)
				printf("Falling back from 0x%x\n", START_ADDR(block));
			return(1);
		}
		else
			give_up("Encountered indirect jump");
		break;
	default:
		error("Internal error at 0x%x, exiting\n", Addr);
	}
	if (f && (FUNCNO(FALLTHROUGH(block)) != FUNCNO(block))) {
		if (Traversal_ctl & INTERPROCEDURAL_JUMPS)
			setup_proc_func(FUNCNO(FALLTHROUGH(block)));
		else
			f = 0;
	}
	if (t && (FUNCNO(JUMP_TARGET(block)) != FUNCNO(block))) {
		if (Traversal_ctl & INTERPROCEDURAL_JUMPS)
			setup_proc_func(FUNCNO(JUMP_TARGET(block)));
		else
			f = 0;
	}
	if (f && t) {
		int *saved_entryvars = NULL;

		if (Varinfo[ESP].alias > MaxESP)
			MaxESP = Varinfo[ESP].alias;
		holdESP = Varinfo[ESP].alias;
		holdNdelete = Ndelete;
		holdsize = Svarinfo;
		hold = MALLOC(holdsize * sizeof(struct varinfo));
		memcpy(hold, Varinfo, holdsize * sizeof(struct varinfo));
		if (NEW_END_TYPE(block) == END_CALL) {
			saved_entryvars = NULL;
			clear_func_saved_vars(&saved_entryvars, FUNCNO(JUMP_TARGET(block)));
			push_blockno(FALLTHROUGH(block));
			Varinfo[Varinfo[ESP].alias].flags |= BARRIER;
			MARK_UNKNOWN_TO_CALLEE(ESI);
			MARK_UNKNOWN_TO_CALLEE(EDI);
			MARK_UNKNOWN_TO_CALLEE(EBP);
			MARK_UNKNOWN_TO_CALLEE(EBX);
		}
		going(block, JUMP_TARGET(block));
		ret2 = proc_all_paths(JUMP_TARGET(block));
		if (saved_entryvars)
			restore_func_saved_vars(saved_entryvars, FUNCNO(JUMP_TARGET(block)));
		Svarinfo = holdsize;
		free(Varinfo);
		Varinfo = hold;
		Varinfo[ESP].alias = holdESP;
		Ndelete = holdNdelete;
		going(block, FALLTHROUGH(block));
		ret1 = proc_all_paths(FALLTHROUGH(block));
	}
	else if (f) {
		int fallthrough;

		if (NEW_END_TYPE(block) == END_RET) {
			UNMARK_UNKNOWN_TO_CALLEE(ESI);
			UNMARK_UNKNOWN_TO_CALLEE(EDI);
			UNMARK_UNKNOWN_TO_CALLEE(EBP);
			UNMARK_UNKNOWN_TO_CALLEE(EBX);
			fallthrough = pop_const();
		}
		else {
			if (NEW_END_TYPE(block) == END_POP)
				pop_const();
			else if (NEW_END_TYPE(block) == END_PSEUDO_CALL)
				push_blockno(FALLTHROUGH(block));
			fallthrough = FALLTHROUGH(block);
		}
		going(block, fallthrough);
		ret1 = proc_all_paths(fallthrough);
	}
	else if (t) {
		going(block, JUMP_TARGET(block));
		ret1 = proc_all_paths(JUMP_TARGET(block));
	}
	else if (Call_at_end)
		Call_at_end();
	Depth--;
	if (Traversal_ctl & VISIT_ON_PATHS)
		DEL_FLAGS(block, BEENHERE_ON_PATH);
/*	printf("Leaving block at 0x%x, ESP is %d\n", START_ADDR(block), Varinfo[ESP].alias);*/
	if (VERBOSE3)
		printf("Falling back from 0x%x\n", START_ADDR(block));
	return(ret1 && ret2);
}

has_jumps_out(int func)
{
	int f, t;
	int block;

	for (block = Func_info[func].start_block; block < Func_info[func].end_block; block++) {
		switch(NEW_END_TYPE(block)) {
		case END_CJUMP:
			f = t = 1;
			break;
		case END_JUMP:
			t = 1;
			f = 0;
			break;
		case END_RET:
		case END_POP:
			f = t = 0;
			break;
		case END_FALL_THROUGH:
		case END_CALL:
		case END_PSEUDO_CALL:
		case END_PUSH:
			f = 1;
			t = 0;
			break;
		case END_IJUMP:
#ifdef IJUMPS_OKAY
			{
				Elf32_Addr *succs;
				int nsuccs;
				int i;

				nsuccs = get_succs(block, &succs);
				for (i = 0; i < nsuccs; i++)
					if (func != FUNCNO(succs[i]))
						return(1);
				f = t = 0;
				break;
			}
#else /* #ifdef IJUMPS_OKAY */
			return(1);
#endif
		default:
			error("Internal error at 0x%x, exiting\n", Addr);
		}
		if (f && (FALLTHROUGH(block) != NO_SUCH_ADDR) && (func != FUNCNO(FALLTHROUGH(block))))
			return(1);
		if (t && (JUMP_TARGET(block) != NO_SUCH_ADDR) && (func != FUNCNO(JUMP_TARGET(block))))
			return(1);
	}
	return(0);
}

arg_delete_or_modify_instruction(struct instable *dp, int prevstack)
{
	if ((dp->optinfo.other & OPT_OPS) == OPT_PUSH) {
		if (IS_LOCATION_UNNEEDED(Varinfo[ESP].alias)) {
			if (VERBOSE2)
				printf("Eliminating push at 0x%x\n", Addr);
			Ndelete++;
			return(1);
		}
		else
			return(0);
	}
	else if ((dp->optinfo.other & OPT_OPS) == OPT_MOV) {
		int dest, all_dests_elim = 1, dest_opno, dest_var, dest_flags;

		for (dest = 0; (dest < NUMOPS) && (Dests[dest] != NO_SUCH_REG); dest++) {
			dest_opno = Dests[dest];
			dest_var = REFVAR(dest_opno);
			dest_flags = REFFLAGS(dest_opno);
			if (!IS_VALUE_UNNEEDED(dest_var) || (dest_flags & ANDBACK) || (dest_flags & INDEXED))
				all_dests_elim = 0;
		}
		if (VERBOSE2 && all_dests_elim)
			printf("Eliminating mov at 0x%x\n", Addr);
		return(all_dests_elim);
	}
	return(0);
}

delete_or_modify_instruction(struct instable *dp, int prevstack)
{
	int op = dp->optinfo.other & OPT_OPS;
	int i;

	Prefix = PREFIX_SUB_I(Curblock, Curstatement);
	if (!Prefix[0] &&
		((dp->optinfo.other & OPT_OPS) == OPT_MOV) &&
		(opsinfo[0].fmt == OPTINFO_REGONLY) &&
		(opsinfo[3].fmt == OPTINFO_REGONLY) &&
		(opsinfo[0].reg == opsinfo[3].reg))
			return(1);
	if (!Ndelete || (prevstack <= Varinfo[ESP].alias))
		return(0);
	switch (op) {
	case OPT_POP:
		if (!IS_LOCATION_UNNEEDED(prevstack))
			return(0);
		Ndelete--;
		return(1);
		break;
	case OPT_ADD:
		for (i = prevstack; (i > Varinfo[ESP].alias) && Ndelete; i--) {
			if (!IS_LOCATION_UNNEEDED(i))
				continue;
			opsinfo[0].u.imm.value -= sizeof(Elf32_Addr);
			Ndelete--;
			Opchanged = 1;
		}
		return(opsinfo[0].u.imm.value == 0);
	case OPT_MOV:
		for (i = prevstack; (i > Varinfo[ESP].alias) && Ndelete; i--) {
			if (!IS_LOCATION_UNNEEDED(i))
				continue;
			Ndelete--;
		}
		return(0);
	default:
		error("Stack shrunk without add or POP\n");
	}
	return(0);
}

esr_delete_instruction(struct instable *dp, int prevstack)
{
	int i, j;

	for (i = 0; i < NREGS; i++)
		for (j = 0; j < Esr_watch[i].nelim; j++)
			if (Esr_watch[i].elim[j] == Addr) {
				if (prevstack < Varinfo[ESP].alias)
					MARK_LOCATION_UNNEEDED_ON_PATH(Varinfo[ESP].alias);
				return(1);
			}
	return(0);
}

void
set_traversal_params(int flags, int (*proc_block)(int block), int (*each_statement)(struct instable *dp, int arg), int (*each_block)(), int (*call_at_end)())
{
	Depth = 0;
	Traversal_ctl = flags;
	Proc_block = proc_block;
	Check_each_statement = each_statement;
	Check_each_block = each_block;
	Call_at_end = call_at_end;
}

void
sink_safe_assignments(int block)
{
	int i, j, k;
	int checkflags;
	int nstatements;
	struct instable *dp;
	int op;
	int src;
	int src_refvar, dest_refvar;
	struct statement save_statement;
	int opno;

	Curblock = block;
	if (!DECODED(block))
		opdecode_block(block);
	for (i = 0; i < NSTATEMENTS(block); i++) {
		if (FLAGS_SUB_I(block, i) & STATEMENT_SUNK)
			continue;
		clearregs();
		STACK_RESET();
		dp = proc_statement(block, i, 1);
		op = dp->optinfo.other & OPT_OPS;
		if ((op == OPT_MOV) && IS_DEST(REGOP)) {
			dest_refvar = REFVAR(REGOP);
			for (src = 0; src < REGOP; src++)
				if (IS_SRC(src))
					break;
			src_refvar = REFVAR(src);
			if (src_refvar >= PSEUDO_REG_EARLY_STACK)
				continue;
			if ((opsinfo[src].fmt == OPTINFO_IMMEDIATE) && !opsinfo[src].rel && (opsinfo[src].u.imm.value == 0))
				checkflags = 1;
			else
				checkflags = 0;
			nstatements = NSTATEMENTS(block);
			/* Don't move the instruction past the end instruction
			*/
			if (END_TYPE(block) == NEW_END_TYPE(block))
				nstatements--;
			clearregs();
			STACK_RESET();
			for (j = i + 1; j < nstatements; j++) {
				dp = proc_statement(block, j, 1);
				if (checkflags && (dp->optinfo.flags & WALLF))
					break;
				if (READ_OR_WROTE_VAR(dest_refvar))
					break;
				if (WROTE_VAR(src_refvar))
					break;
				for (opno = 0; opno < NUMOPS; opno++) {
					if (!IS_DEST(opno))
						continue;
					if (src_refvar == PSEUDO_REG_MEM) {
						if (REFFLAGS(opno) & POSSIBLY_MEMORY)
							break;
						else
							continue;
					}
				}
				if (opno < NUMOPS)
					break;
			}
			if (j == i + 1)
				continue;
			save_statement = STATEMENT_SUB_I(block, i);
			for (k = i; k < j - 1; k++)
				COPY_SUB_I(block, k, k + 1);
			SET_STATEMENT_SUB_I(block, k, save_statement);
			ADD_FLAGS_SUB_I(block, k, STATEMENT_SUNK);
			i--;
		}
	}
}

void
float_safe_assignments(int block)
{
	int lower_lim;
	int i, j, k;
	int checkflags;
	int nstatements;
	struct instable *dp;
	int op;
	int src;
	int src_refvar, dest_refvar;
	struct statement save_statement;
	int opno;

	set_traversal_params(0, NULL, NULL, NULL, NULL);
	Curblock = block;
	if (!DECODED(block))
		opdecode_block(block);
	for (i = 0; i < NSTATEMENTS(block); i++) {
		if (FLAGS_SUB_I(block, i) & STATEMENT_FLOATED)
			continue;
		if (!(FLAGS_SUB_I(block, i) & STATEMENT_SUNK))
			continue;
		clearregs();
		STACK_RESET();
		dp = proc_statement(block, i, 1);
		op = dp->optinfo.other & OPT_OPS;
		if ((op == OPT_MOV) && IS_DEST(REGOP)) {
			dest_refvar = REFVAR(REGOP);
			for (src = 0; src < REGOP; src++)
				if (IS_SRC(src))
					break;
			src_refvar = REFVAR(src);
			if (src_refvar >= PSEUDO_REG_EARLY_STACK)
				continue;
			if ((opsinfo[src].fmt == OPTINFO_IMMEDIATE) && !opsinfo[src].rel && (opsinfo[src].u.imm.value == 0))
				checkflags = 1;
			else
				checkflags = 0;
/*			if (opsinfo[src].fmt == OPTINFO_IMMEDIATE)*/
/*				lower_lim = 0;*/
/*			else*/
/*				lower_lim = i - 2;*/
			clearregs();
			STACK_RESET();
			for (j = i - 1; j >= 0; j--) {
				if (ADDR_SUB_I(block, j) < ADDR_SUB_I(block, i))
					break;
				dp = proc_statement(block, j, 1);
				if (any_subst(dest_refvar))
					break;
				if (checkflags && (dp->optinfo.flags & WALLF))
					break;
				if (READ_OR_WROTE_VAR(dest_refvar))
					break;
				if (WROTE_VAR(src_refvar))
					break;
				if (src_refvar == PSEUDO_REG_MEM) {
					for (opno = 0; opno < NUMOPS; opno++) {
						if (!IS_DEST(opno))
							continue;
						if (REFFLAGS(opno) & POSSIBLY_MEMORY)
							break;
					}
					if (opno < NUMOPS)
						break;
				}
			}
			if (j == i - 1)
				continue;
			save_statement = STATEMENT_SUB_I(block, i);
			for (k = i; k > j + 1; k--)
				COPY_SUB_I(block, k, k - 1);
			SET_STATEMENT_SUB_I(block, k, save_statement);
			ADD_FLAGS_SUB_I(block, k, STATEMENT_FLOATED);
		}
	}
}

void
analyze_args(int block)
{
	Proc_block(block);
	Varinfo[Varinfo[ESP].alias].flags |= BARRIER;
	push_blockno(JUMP_TARGET(block));
}

traverse(int block)
{
	int ret;

	Nvisits = 0;
	setup_proc_func(FUNCNO(block));
	Depth = 0;
	ret = proc_all_paths(block);
	if (VERBOSE1)
		printf("Nvisits = %d\n", Nvisits);
	if (VERBOSE2)
		printf("MaxESP is %d\n", MaxESP);
	return(ret);
}

void
clear_eax_ecx_edx()
{
	clearvar(EAX);
	clearvar(ECX);
	clearvar(EDX);
}

void
clearvar(int i)
{
	Varinfo[i].type = UNKNOWN_TYPE;
	Varinfo[i].alias = NO_SUCH_ADDR;
	Varinfo[i].u.rel = NULL;
	Varinfo[i].u.val = 0;
	not_a_copy(i);
	Varinfo[i].flags = 0;
}

new_func_to_inline(int enclosing_func, int inlined_func, int block)
{
	int i;

	Encountered_new_func = 0;
	if (getenv("DONT_ESR"))
		Dont_esr = atoi(getenv("DONT_ESR"));
	else
		Dont_esr = 100000000;
	if (getenv("DONT_SUBST"))
		Dont_subst = atoi(getenv("DONT_SUBST"));
	else
		Dont_subst = 10000000;
	MaxESP = 0;
	if (VERBOSE1) {
		fflush(stdout);
		setbuf(stdout, NULL);
		fflush(stderr);
	}
	if (!Opinit)
		do_opinit();
	if (!Pstatements) {
		Spstatements = Nblocks + 1;
		Pstatements = CALLOC(Spstatements, sizeof(struct pstatement));
	}
	else {
		if (Spstatements <= Nblocks) {
			Pstatements = REALLOC(Pstatements, (Nblocks + 1) * sizeof(struct pstatement));
			Spstatements = Nblocks + 1;
		}
		memset(Pstatements, '\0', Spstatements * sizeof(struct pstatement));
	}
	Nstatements = 1;
	Nholdvars = 0;
	Enclosing_func = enclosing_func;
	if (VERBOSE1 || getenv("ESR_VERBOSE")) {
		printf("Trying %s%s%s inlined into %s%s%s\n", FULLNAME(Funcs[inlined_func]), FULLNAME(Funcs[FUNCNO(block)]));
		printf("Call site is %s%s%s+%d(0x%x)\n", FULLNAME(Funcs[FUNCNO(block)]), block - Func_info[FUNCNO(block)].start_block, END_ADDR(block) - END_INSTLEN(block));
	}
	reset_subst();
	Nwatchinfo = 0;
	clear_watch();
	if ((NEW_END_TYPE(block) != END_JUMP) || (NEW_END_TYPE(JUMP_TARGET(block)) != END_PUSH))
		error("This is not a call\n");
	if (has_jumps_out(inlined_func)) {
		printf("Cannot inline a function which jumps out\n");
		return(0);
	}
	clear_funcs_beenhere();
	/* GEN_THIS_TEXT will be reset by setup_proc_func */
	Nchanged_blocks = 0;
	clearregs();
	Nfreeregs = 0;
	return(1);
}
