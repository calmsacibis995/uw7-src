#ident	"@(#)fur:i386/cmd/fur/opsubst.c	1.3"
#ident	"$Header:"

#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "fur.h"
#include "op.h"

struct subst {
	Elf32_Addr addr;
	int var;
	int substvar;
	int flags;
	Elf32_Addr val;
	Elf32_Rel *rel;
};
extern int jo_cmp();
extern int jno_cmp();
extern int jb_cmp();
extern int jae_cmp();
extern int je_cmp();
extern int jne_cmp();
extern int jbe_cmp();
extern int ja_cmp();
extern int js_cmp();
extern int jns_cmp();
extern int jp_cmp();
extern int jnp_cmp();
extern int jl_cmp();
extern int jge_cmp();
extern int jle_cmp();
extern int jg_cmp();
extern int jo_test();
extern int jno_test();
extern int jb_test();
extern int jae_test();
extern int je_test();
extern int jne_test();
extern int jbe_test();
extern int ja_test();
extern int js_test();
extern int jns_test();
extern int jp_test();
extern int jnp_test();
extern int jl_test();
extern int jge_test();
extern int jle_test();
extern int jg_test();

int (*cmp_table[])() = {
	jo_cmp,
	jno_cmp,
	jb_cmp,
	jae_cmp,
	je_cmp,
	jne_cmp,
	jbe_cmp,
	ja_cmp,
	js_cmp,
	jns_cmp,
	jp_cmp,
	jnp_cmp,
	jl_cmp,
	jge_cmp,
	jle_cmp,
	jg_cmp,
};

int (*test_table[])() = {
	jo_test,
	jno_test,
	jb_test,
	jae_test,
	je_test,
	jne_test,
	jbe_test,
	ja_test,
	js_test,
	jns_test,
	jp_test,
	jnp_test,
	jl_test,
	jge_test,
	jle_test,
	jg_test,
};

struct subst *Substs;
int Nsubsts;
int Ssubsts;

#define DELETED_SUBST 1
#define CANT_USE_SUBST 2

static Elf32_Addr Last_subst = NO_SUCH_ADDR;
extern	struct	instable	distable[];
extern	struct	instable	op0F[];
static struct fmt Opfmts[NUMOPS + 1];

#define SRC				(1<<0)
#define DEST			(1<<1)
#define IMM				(1<<2)
#define DISP			(1<<3)
#define HARD_REG		(1<<4)
#define SEGMENT_REG		(1<<5)
#define SPEC			(1<<6)
#define IMM8			(1<<7)
#define RM				(1<<8)
#define LEFT			(1<<9)
#define REGONLY			(1<<10)
#define REG(NUM)		(1<<(11 + NUM))
#define REG_EAX			REG(EAX)
#define REG_ECX			REG(ECX)
#define REG_EDX			REG(EDX)
#define REG_EBX			REG(EBX)
#define REG_ESI			REG(ESI)
#define REG_EBP			REG(EBP)
#define REG_EDI			REG(EDI)
#define REG_ESP			REG(ESP)
#define HARD_EAX		(HARD_REG|REG_EAX)
#define HARD_ECX		(HARD_REG|REG_ECX)
#define HARD_EDX		(HARD_REG|REG_EDX)
#define HARD_EBX		(HARD_REG|REG_EBX)
#define HARD_ESI		(HARD_REG|REG_ESI)
#define HARD_EBP		(HARD_REG|REG_EBP)
#define HARD_EDI		(HARD_REG|REG_EDI)
#define HARD_ESP		(HARD_REG|REG_ESP)

#define SRCDEST(FMT)	((FMT) & (SRC|DEST))
#define FMT_TYPE(FMT)	((FMT) & (IMM|DISP|REGONLY|RM|IMM8|HARD_REG))

#define ALLREGS			(REG_EAX|REG_ECX|REG_EDX|REG_EBX|REG_ESI|REG_EBP|REG_EDI|REG_ESP)

struct fmt {
	int flags;
};

int subst_table[56][4] = {
	/* UNKNOWN */ { 0, 0, 0, 0 },
	/* IM8w */ { LEFT|SRC|IMM8, SRC|DEST, 0, 0 },
	/* MRw */ { LEFT|SRC, 0, 0, SRC|DEST },
	/* IMlw */ { LEFT|SRC|IMM, SRC|DEST, 0, 0 },
	/* IMw */ { LEFT|SRC|IMM, SRC|DEST, 0, 0 },
	/* IR */ { SRC|IMM, 0, 0, DEST|HARD_REG },
	/* OA */ { SRC|DISP, 0, 0, HARD_EAX|DEST },
	/* AO */ { DEST|DISP, 0, 0, SRC|HARD_EAX },
	/* MS */ { SRC, 0, 0, DEST },
	/* SM */ { DEST, 0, 0, SRC },
	/* Mv */ { DEST, 0, 0, SRC },
	/* Mw */ { DEST, 0, 0, 0 },
	/* M */ { SRC|DEST, 0, 0, 0 },
	/* R */ { 0, 0, 0, SRC|DEST|HARD_REG },
	/* RA */ { SRC|HARD_EAX, 0, 0, LEFT|SRC|DEST },
	/* SEG */ { 0, 0, 0, SEGMENT_REG|SRC },
	/* MR */ { LEFT|SRC, 0, 0, SRC|DEST },
	/* IA */ { LEFT|SRC|IMM, 0, 0, SRC|HARD_EAX|DEST },
	/* MA */ { SRC, 0, 0, DEST|HARD_EAX },
	/* SD */ { 0, SRC|HARD_ESI, DEST|HARD_EDI, 0 },
	/* AD */ { 0, HARD_EDI|DEST, 0, SRC },
	/* SA */ { 0, SRC|HARD_ESI, 0, DEST },
	/* D */ { 0, 0, 0, 0, },
	/* INM */ { SRC, 0, 0, 0 },
	/* SO */ { LEFT|SRC|DISP, SRC|DISP, 0, 0 },
	/* BD */ { LEFT|SRC|DISP, 0, 0, HARD_EAX|SRC|DEST },
	/* I */ { SRC|IMM, 0, 0, 0 },
	/* P */ { SRC|IMM8, 0, 0, HARD_EAX|DEST|SRC },
	/* V */ { 0, SRC, 0, HARD_EDX|DEST },
	/* DSHIFT */ { SRC|IMM8, DEST, 0, DEST },
	/* U */ { 0, 0, 0, HARD_EAX|DEST },
	/* OVERRIDE */ { 0, 0, 0, 0 },
	/* GO_ON */ { SRC|DEST|HARD_ECX, SRC|DEST|HARD_EDX, SRC|DEST|HARD_ESI, SRC|DEST|HARD_EAX },
	/* O */ { 0, 0, 0, 0 },
	/* JTAB */ { 0, 0, 0, 0 },
	/* IMUL */ { LEFT|SRC|IMM, SRC, 0, DEST },
	/* CBW */ { 0, 0, 0, HARD_EAX|DEST },
	/* MvI */ { DEST, LEFT|SRC|IMM8, 0, SRC|HARD_ECX },
	/* ENTER */ { 0, 0, 0, 0 },
	/* RMw */ { SRC|DEST, 0, 0, LEFT|SRC },
	/* Ib */ { SRC|IMM8, 0, 0, 0 },
	/* F */ { 0, 0, 0, 0 },
	/* FF */ { 0, 0, 0, 0 },
	/* DM */ { 0, 0, 0, 0 },
	/* AM */ { 0, 0, 0, 0 },
	/* LSEG */ { 0, 0, 0, SRC|HARD_REG|SEGMENT_REG|DEST },
	/* MIb */ { SRC|DEST, LEFT|SRC|IMM8, 0, 0 },
	/* SREG */ { SPEC|SRC|DEST, 0, 0, LEFT|SPEC|SRC|DEST },
	/* PREFIX */ { 0, 0, 0, 0 },
	/* INT3 */ { 0, 0, 0, 0 },
	/* DSHIFTcl */ { DEST, SRC|HARD_ECX, 0, DEST },
	/* CWD */ { HARD_EDX|DEST, 0, 0, HARD_EAX|DEST },
	/* RET */ { SRC|IMM, 0, 0, 0 },
	/* MOVZ */ { SRC, 0, 0, DEST },
	/* RM16 */ { 0, 0, 0, 0 },
	/* M16 */ { SRC|DEST, 0, 0, 0 },
};

struct instable **Opgroups[NOPGROUPS];
int Nopgroups[NOPGROUPS];

int Allmaps[24][NUMOPS] = {
	{ 0, 1, 2, 3 },
	{ 0, 1, 3, 2 },
	{ 0, 2, 1, 3 },
	{ 0, 2, 3, 1 },
	{ 0, 3, 1, 2 },
	{ 0, 3, 2, 1 },
	{ 1, 0, 2, 3 },
	{ 1, 0, 3, 2 },
	{ 1, 2, 0, 3 },
	{ 1, 2, 3, 0 },
	{ 1, 3, 0, 2 },
	{ 1, 3, 2, 0 },
	{ 2, 0, 1, 3 },
	{ 2, 0, 3, 1 },
	{ 2, 1, 0, 3 },
	{ 2, 1, 3, 0 },
	{ 2, 3, 0, 1 },
	{ 2, 3, 1, 0 },
	{ 3, 0, 1, 2 },
	{ 3, 0, 2, 1 },
	{ 3, 1, 0, 2 },
	{ 3, 1, 2, 0 },
	{ 3, 2, 0, 1 },
	{ 3, 2, 1, 0 },
};

void
add_from_table(struct instable *tab, int length)
{
	int i;
	struct instable *dp;

	for (i = 0; i < length; i++) {
		dp = tab + i;
		switch (dp->optinfo.opgroup) {
		case UNKNOWN:
		case PREFIX:
		case OVERRIDE:
			continue;
		}
		if (dp->indirect == op0F)
			add_from_table(op0F, 208);
		else if (dp->indirect != TERM)
			add_from_table(dp->indirect, 8);
		else {
			if (dp->optinfo.opgroup == NO_OPGROUP)
				continue;
			if (!(Nopgroups[dp->optinfo.opgroup] % 5))
				Opgroups[dp->optinfo.opgroup] = REALLOC(Opgroups[dp->optinfo.opgroup], (Nopgroups[dp->optinfo.opgroup] + 5) * sizeof(int *));
			Opgroups[dp->optinfo.opgroup][Nopgroups[dp->optinfo.opgroup]++] = dp;
		}
	}
}

void
do_groups()
{
	add_from_table(distable, 256);
}

int
any_subst(int var)
{
	int i;

	for (i = 0; i < Nsubsts; i++)
		if ((Addr == Substs[i].addr) && (Substs[i].substvar == var) && !(Substs[i].flags & (DELETED_SUBST|CANT_USE_SUBST)))
			return(1);
	return(0);
}

void
try_gencode(struct instable *dp)
{
	char genbuf[21];

	Genbuf = genbuf;
	Genspot = 0;
	Prefix = PREFIX_SUB_I(Curblock, Curstatement);
	Remap_ops = Allmaps[0];
	if ((dp->optinfo.opgroup == 235) && (opsinfo[0].fmt == OPTINFO_IMMEDIATE) && (opsinfo[0].u.imm.value == 0) && !opsinfo[0].rel && ((dp->optinfo.other & (WREG|ROP0)) == (WREG|ROP0)) && (opsinfo[REGOP].fmt == OPTINFO_REGONLY))
		dp = subst_mov_xor();
	gencode(dp);
}

test_cte(ulong opleft, ulong opright)
{
	unchar *condbuf;
	int cond;
	int ret;
	int i;

	condbuf = (unchar *) TEXTBUF(Curblock) + ADDR_SUB_I(Curblock, NSTATEMENTS(Curblock) - 1) - START_ADDR(Curblock);
	if (condbuf[0] == LONGCODE_PREFIX)
		cond = condbuf[1] & 0xf;
	else
		cond = condbuf[0] & 0xf;
	/* "test" is 319 */
	if (DP_SUB_I(Curblock, Curstatement)->optinfo.opgroup == 319)
		ret = test_table[cond](opleft, opright);
	else
		ret = cmp_table[cond](opleft, opright);
	return(ret);
}

int
try_cte(int *ptaken)
{
	static int ncte;
	int opno;
	int i;
	ulong opleft, opright;
	Elf32_Rel *leftrel = NULL, *rightrel = NULL;
	struct instable *dp = DP_SUB_I(Curblock, Curstatement);

	if (!Blocks_stats)
		return(0);
	if (NEW_END_TYPE(Curblock) != END_CJUMP)
		return(0);
	if ((dp->optinfo.other & OPT_OPS) == OPT_PUSH)
		return(0);
	for (opno = 0; opno < NUMOPS; opno++) {
		if (IS_DEST(opno))
			return(0);
		if (opsinfo[opno].fmt == 0)
			continue;
		if (opsinfo[opno].fmt == OPTINFO_IMMEDIATE) {
			if (subst_table[dp->adr_mode][opno] & LEFT) {
				if (opsinfo[opno].rel) {
					leftrel = opsinfo[opno].rel;
					opleft = GET4(Text_data->d_buf, leftrel->r_offset) + 0x80000000;
				}
				else
					opleft = opsinfo[opno].u.imm.value;
			}
			else {
				if (opsinfo[opno].rel) {
					rightrel = opsinfo[opno].rel;
					opright = GET4(Text_data->d_buf, rightrel->r_offset) + 0x80000000;
				}
				else
					opright = opsinfo[opno].u.imm.value;
			}
			continue;
		}
		for (i = 0; i < Nsubsts; i++) {
			if ((Addr == Substs[i].addr) && (REFVAR(opno) == Substs[i].var) && !(Substs[i].flags & DELETED_SUBST)) {
				if (Substs[i].substvar == SUBST_CONSTANT) {
					if (subst_table[dp->adr_mode][opno] & LEFT) {
						if (Substs[i].rel) {
							leftrel = Substs[i].rel;
							opleft = GET4(Text_data->d_buf, leftrel->r_offset) + 0x80000000;
						}
						else
							opleft = Substs[i].val;
					}
					else {
						if (Substs[i].rel) {
							rightrel = Substs[i].rel;
							opright = GET4(Text_data->d_buf, rightrel->r_offset) + 0x80000000;
						}
						else
							opright = Substs[i].val;
					}
					break;
				}
			}
		}
		if (i == Nsubsts)
			return(0);
	}
	for (i = Curstatement + 1; i < NSTATEMENTS(Curblock) - 1; i++) {
		if (DP_SUB_I(Curblock, i)->optinfo.flags) {
			if (VERBOSE2)
				printf("Cannot cte at 0x%x due to intervening instruction at 0x%x\n", Addr, ADDR_SUB_I(Curblock, i));
			return(0);
		}
	}
	if (leftrel || rightrel) {
		if (!leftrel || !rightrel || (leftrel->r_info != rightrel->r_info)) {
			if (VERBOSE2)
				printf("Cannot cte at 0x%x because ops have different relocations\n", Addr);
			return(0);
		}
	}
	
	if (getenv("DONT_CTE") && (ncte++ >= atoi(getenv("DONT_CTE")))) {
		printf("Not performing constant test elimination at 0x%x\n", Addr);
		return(0);
	}
	if (VERBOSE2)
		printf("Found a constant test elimination case at 0x%x\n", Addr);
	*ptaken = test_cte(opleft, opright);
	return(1);
}

int
try_subst(int opno)
{
	int orig_genlen;
	struct instable *dp;
	struct opsinfo holdopsinfo;
	int foundgoodsubst = 0;
	int foundasubst = 0;
	int i;

	if (IS_DEST(opno))
		return(1);
	if (opsinfo[opno].fmt == 0)
		return(1);
	for (i = 0; i < Nsubsts; i++) {
		if ((Addr == Substs[i].addr) && (REFVAR(opno) == Substs[i].var) && !(Substs[i].flags & (DELETED_SUBST|CANT_USE_SUBST))) {
			foundasubst++;
			if ((Substs[i].substvar != SUBST_CONSTANT) && (Substs[i].substvar < NREGS)) {
				foundgoodsubst++;
				continue;
			}
			if (Substs[i].var < NREGS) {
				try_gencode(DP_SUB_I(Curblock, Curstatement));
				orig_genlen = Genspot;
			}
			holdopsinfo = opsinfo[opno];
			if (Substs[i].substvar == SUBST_CONSTANT) {
				opsinfo[opno].fmt = OPTINFO_IMMEDIATE;
				opsinfo[opno].u.imm.value = 0;
			}
			else {
				opsinfo[opno].fmt = OPTINFO_DISPREG;
				opsinfo[opno].u.disp.value = 4;
				opsinfo[opno].rel = NULL;
				opsinfo[opno].reg = ESP;
			}
			Last_subst = Addr;

			dp = subst_dp(0, NULL);
			if (dp) {
				if (Substs[i].var < NREGS) {
					try_gencode(dp);
					if (Genspot <= orig_genlen + 3) {
						foundgoodsubst++;
						opsinfo[opno] = holdopsinfo;
						continue;
					}
					else if (VERBOSE2)
						printf("Constant substitution makes instruction too long at 0x%x\n", Addr);
				}
				else {
					foundgoodsubst++;
					opsinfo[opno] = holdopsinfo;
					continue;
				}
			}
			opsinfo[opno] = holdopsinfo;
			if (VERBOSE2)
				printf("Lost substitution at address 0x%x: no matching format\n", Addr);
			Substs[i].flags |= CANT_USE_SUBST;
		}
	}
	return(!foundasubst || foundgoodsubst);
}

do_subst(int opno)
{
	int i;
	struct opsinfo *pops;

	if (opsinfo[opno].fmt == 0)
		return(0);
	if (IS_DEST(opno))
		return(0);
	for (i = 0; i < Nsubsts; i++) {
		if ((Addr == Substs[i].addr) && (REFVAR(opno) == Substs[i].var) && !(Substs[i].flags & DELETED_SUBST))
			if (Substs[i].flags & CANT_USE_SUBST) {
				if (VERBOSE3)
					printf("Can't do substitution at 0x%x\n", Addr);
			}
			else
				break;
	}
	if (i >= Nsubsts)
		return(0);
	Opchanged = 1;
	Last_subst = Addr;
	pops = opsinfo + opno;
	if (Substs[i].substvar == SUBST_CONSTANT) {
		if (VERBOSE2) {
			if (Substs[i].var < NREGS)
				printf("Substituting constant for register at address 0x%x\n", Addr);
			else
				printf("Substituting constant for expression at address 0x%x\n", Addr);
		}
		pops->fmt = OPTINFO_IMMEDIATE;
		if (Substs[i].rel) {
			pops->u.imm.value = GET4(Text_data->d_buf, Substs[i].rel->r_offset);
			pops->rel = Substs[i].rel;
			pops->flags |= OPTINFO_REL;
		}
		else
			pops->u.imm.value = Substs[i].val;
		pops->refto = PSEUDO_REG_CONSTANT;
	}
	else if (Substs[i].substvar < NREGS) {
		if (VERBOSE2)
			printf("Substituting register for expression at address 0x%x\n", Addr);
		subst_expr_reg(pops, Substs[i].substvar);
	}
	else {
		if (VERBOSE2)
			printf("Substituting expression for expression at address 0x%x\n", Addr);
		subst_expr_expr(pops, Substs[i].substvar);
	}
	return(1);
}

void
reset_subst()
{
	Nsubsts = 0;
	Last_subst = NO_SUCH_ADDR;
}

void
add_subst(int var, int substvar, Elf32_Addr val, Elf32_Rel *rel)
{
	if (Ssubsts == Nsubsts) {
		Ssubsts += 10;
		Substs = REALLOC(Substs, Ssubsts * sizeof(struct subst));
	}
	Substs[Nsubsts].addr = Addr;
	Substs[Nsubsts].var = var;
	Substs[Nsubsts].substvar = substvar;
	Substs[Nsubsts].val = val;
	Substs[Nsubsts].rel = rel;
	Substs[Nsubsts].flags = 0;
	{
		extern int Dont_subst;

		if (Nsubsts >= Dont_subst - 1) {
			printf("Not substituting at 0x%x\n", Addr);
			Substs[Nsubsts].flags = DELETED_SUBST;
			MARK_READ(Substs[Nsubsts].var);
		}
	}
	Nsubsts++;
}

add_subst_var(int var, int substvar)
{
	add_subst(var, substvar, 0, NULL);
}

add_subst_constant(int var)
{
	add_subst(var, SUBST_CONSTANT, CONSTANT(var), IS_REL_CONSTANT(var) ? VAR_REL(var) : NULL);
}

int
mark_read_subst(int var)
{
	int i;

	for (i = 0; i < Nsubsts; i++) {
		if ((Addr != Substs[i].addr) || (var != Substs[i].var) || (Substs[i].flags & (DELETED_SUBST|CANT_USE_SUBST)))
			continue;
		if (Substs[i].substvar != SUBST_CONSTANT)
			MARK_READ(Substs[i].substvar);
		return(1);
	}
	return(0);
}

match_subst(int var)
{
	int i, subst = 0, copy;

	for (i = 0; i < Nsubsts; i++) {
		if ((Addr != Substs[i].addr) || (var != Substs[i].var) || (Substs[i].flags & DELETED_SUBST))
			continue;
		if (Substs[i].substvar == SUBST_CONSTANT) {
			if (IS_ANDBACK(var) || IS_INDEXED(var) || !IS_CONSTANT(var) || (Substs[i].val != CONSTANT(var))) {
				Substs[i].flags |= DELETED_SUBST;
				if ((Traversal_ctl & SHOW_SUBST) && VERBOSE2)
					printf("\tLost saved reference to %s at 0x%x because it is not that constant anymore\n", PR(var), Addr);
			}
			else
				subst = 1;
			continue;
		}
		for (copy = NEXTCOPY(var); copy != var; copy = NEXTCOPY(copy))
			if (copy == Substs[i].substvar)
				break;
		if (copy == var) {
			char buf[20];

			Substs[i].flags |= DELETED_SUBST;
			if ((Traversal_ctl & SHOW_SUBST) && VERBOSE2) {
				strcpy(buf, PR(var));
				printf("\tLost saved reference to %s at 0x%x because it is not a copy of %s\n", buf, Addr, PR(Substs[i].substvar));
			}
		}
		else
			subst = 1;
	}
	return(subst);
}

subst_reg_reg(struct opsinfo *pops, int origreg, int newreg)
{
	Last_subst = Addr;
	if (pops->index == origreg)
		pops->index = newreg;
	if (pops->reg == origreg)
		pops->reg = newreg;
	return(1);
}

int
subst_reg_const(struct opsinfo *pops, int reg)
{
	int subst = 0;

	switch (pops->fmt) {
	case OPTINFO_SIB:
		if ((pops->reg == reg) && (pops->index == reg)) {
			pops->u.disp.value += CONSTANT(reg) + pops->scale * CONSTANT(pops->index);
			pops->fmt = OPTINFO_DISPONLY;
			subst = 1;
		}
		else if (pops->reg == reg) {
			pops->u.disp.value += CONSTANT(reg);
			pops->fmt = OPTINFO_SIB_NOREG;
			subst = 1;
		}
		else if (pops->index == reg) {
			pops->u.disp.value += pops->scale * CONSTANT(pops->index);
			pops->fmt = OPTINFO_DISPREG;
			subst = 1;
		}
		break;
	case OPTINFO_SIB_NOREG:
		if (pops->index == reg) {
			pops->u.disp.value += pops->scale * CONSTANT(pops->index);
			pops->fmt = OPTINFO_DISPONLY;
			subst = 1;
		}
		break;
	case OPTINFO_REGONLY:
		pops->u.imm.value = CONSTANT(reg);
		pops->fmt = OPTINFO_IMMEDIATE;
		subst = 1;
		break;
	case OPTINFO_DISPONLY:
		break;
	case OPTINFO_DISPREG:
		if (pops->reg == reg) {
			pops->u.disp.value += CONSTANT(reg);
			pops->fmt = OPTINFO_DISPONLY;
			subst = 1;
		}
		break;
	case OPTINFO_IMMEDIATE:
		break;
	default:
		break;
	}
	if (subst) {
		if (VAR_REL(reg))
			pops->rel = VAR_REL(reg);
		Last_subst = Addr;
	}
	return(subst);
}

subst_expr_expr(struct opsinfo *pops, int var)
{
	int i;

	Last_subst = Addr;
	pops->reg = ESP;
	pops->u.disp.loc = NO_SUCH_ADDR;
	pops->fmt = OPTINFO_SIB;
	pops->index = NO_REG;
	pops->u.disp.value = 0;
	for (i = Varinfo[ESP].alias; i > var; i--)
		pops->u.disp.value += sizeof(Elf32_Addr);
	pops->rel = NULL;
	pops->flags &= OPTINFO_SRC|OPTINFO_DEST|OPTINFO_EIGHTBIT|OPTINFO_SIXTEEN;
	pops->refto = var;
	return(1);
}

subst_expr_reg(struct opsinfo *pops, int reg)
{
	Last_subst = Addr;
	pops->reg = reg;
	pops->u.disp.loc = NO_SUCH_ADDR;
	pops->u.disp.value = 0;
	pops->fmt = OPTINFO_REGONLY;
	pops->rel = NULL;
/*	pops->flags &= OPTINFO_SRC|OPTINFO_DEST;*/
	pops->flags &= OPTINFO_SRC|OPTINFO_DEST|OPTINFO_EIGHTBIT|OPTINFO_SIXTEEN;
/*	pops->flags &= OPTINFO_SRC|OPTINFO_DEST|OPTINFO_EIGHTBIT;*/
	pops->refto = reg;
	return(1);
}

struct instable *
subst_xor_mov()
{
	opsinfo[0].fmt = OPTINFO_IMMEDIATE;
	opsinfo[0].u.imm.value = 0;
	return(&(distable[0xb * 16 + 8 + opsinfo[REGOP].reg]));
}

struct instable *
subst_mov_xor()
{
	opsinfo[0].fmt = OPTINFO_REGONLY;
	opsinfo[0].reg = opsinfo[REGOP].reg;
	return(&(distable[3 * 16 + 3]));
}

int
set_fmt(int opno)
{
	if (opsinfo[opno].flags & OPTINFO_SRC)
		Opfmts[opno].flags = SRC;
	else if (opsinfo[opno].flags & OPTINFO_DEST)
		Opfmts[opno].flags = DEST;
	else if ((opsinfo[opno].fmt != 0) && (opsinfo[opno].fmt != OPTINFO_IMMEDIATE)) {
		Opfmts[opno].flags = 0;
		return(0);
	}
	switch (opsinfo[opno].fmt) {
	case 0:
		Opfmts[opno].flags = 0;
		return(0);
	case OPTINFO_DISPREG:
	case OPTINFO_SIB_NOREG:
	case OPTINFO_SIB:
		Opfmts[opno].flags |= RM;
		break;
	case OPTINFO_DISPONLY:
		Opfmts[opno].flags |= DISP;
		break;
	case OPTINFO_REGONLY:
		Opfmts[opno].flags |= REGONLY;
		if (opsinfo[opno].flags & OPTINFO_SEGREG)
			Opfmts[opno].flags |= SEGMENT_REG;
		else if (opsinfo[opno].flags & OPTINFO_SPECIAL)
			Opfmts[opno].flags |= SPEC;
		Opfmts[opno].flags |= REG(opsinfo[opno].reg);
		break;
	case OPTINFO_IMMEDIATE:
		if (opsinfo[opno].flags & OPTINFO_EIGHTBIT)
			Opfmts[opno].flags |= IMM8;
		else if (SMALL_IMMEDIATE(opno))
			Opfmts[opno].flags |= IMM8;
		else
			Opfmts[opno].flags |= IMM;
	}
	return(1);
}

score_operand(int tryfmt, int opfmt)
{
	if ((tryfmt == 0) && (opfmt == 0))
		return(0);
	if (!(SRCDEST(tryfmt) & SRCDEST(opfmt)))
		return(-1000);
	if ((opfmt & LEFT) && !(tryfmt & LEFT))
		return(-1000);
	if (FMT_TYPE(opfmt) & IMM8) {
		if (FMT_TYPE(tryfmt) & IMM8)
			return(2);
		else if (FMT_TYPE(tryfmt) & IMM)
			return(0);
		else
			return(-1000);
	}
	if (FMT_TYPE(opfmt) & DISP) {
		if (FMT_TYPE(tryfmt) & DISP)
			return(1);
		else if (FMT_TYPE(tryfmt) & RM)
			return(0);
		else
			return(-1000);
	}
	if (FMT_TYPE(opfmt) & IMM) {
		if (FMT_TYPE(tryfmt) & IMM)
			return(0);
		else
			return(-1000);
	}
	if (FMT_TYPE(opfmt) & RM) {
		if (FMT_TYPE(tryfmt) & RM)
			return(1);
		else
			return(-1000);
	}
	if (opfmt & SPEC) {
		if (tryfmt & SPEC)
			return(1);
		else
			return(-1000);
	}
	else if (tryfmt & SPEC)
		return(-1000);
	if (opfmt & SEGMENT_REG) {
		if (tryfmt & SEGMENT_REG)
			return(1);
		else
			return(-1000);
	}
	else if (tryfmt & SEGMENT_REG)
		return(-1000);
	if ((FMT_TYPE(tryfmt) & HARD_REG) && !(tryfmt & ALLREGS))
		return(1);
	if (FMT_TYPE(tryfmt) & HARD_REG) {
		if ((opfmt & ALLREGS) == (tryfmt & ALLREGS))
			return(2);
		else
			return(-1000);
	}
	if (FMT_TYPE(tryfmt) & REGONLY)
		return(1);
	if (FMT_TYPE(tryfmt) & RM)
		return(0);
	return(-1000);
}

trymap(struct instable *dp, int mapno)
{
	int i;
	int score = 0;
	int opfmt, tryfmt;

	for (i = 0; i < NUMOPS; i++) {
		tryfmt = subst_table[dp->adr_mode][i];
		opfmt = Opfmts[Allmaps[mapno][i]].flags;
		if (!FMT_TYPE(tryfmt)) {
			if (!FMT_TYPE(opfmt))
				continue;
			if (i == REGOP)
				tryfmt |= REGONLY;
			else
				tryfmt |= RM;
		}
		score += score_operand(tryfmt, opfmt);
	}
	return(score);
}

match(struct instable *dp, int *pbest)
{
	int i;
	int score, bestscore = -1;

	for (i = 0; i < 24; i++) {
		score = trymap(dp, i);
		if (score > bestscore) {
			bestscore = score;
			*pbest = i;
		}
	}
	return(bestscore);
}

struct instable *
find_dp(struct instable *dp)
{
	int i;
	struct instable *testdp, *bestdp;
	int score;
	int bestscore = -1;
	int map;
	int bestmap;

	if (dp->optinfo.opgroup == NO_OPGROUP)
		error("We've hit something without an opgroup\n");
	for (i = 0; i < Nopgroups[dp->optinfo.opgroup]; i++) {
		testdp = Opgroups[dp->optinfo.opgroup][i];
		score = match(testdp, &map);
		if (score > bestscore) {
			bestscore = score;
			bestdp = testdp;
			bestmap = map;
		}
	}
	if (bestscore < 0)
		return(NULL);
	Remap_ops = Allmaps[bestmap];
	return(bestdp);
}

void
remap_ops()
{
	struct opsinfo holdopsinfo[NUMOPS];
	int i;

	memcpy(holdopsinfo, opsinfo, NUMOPS * sizeof(struct opsinfo));
	for (i = 0; i < NUMOPS; i++)
		opsinfo[i] = holdopsinfo[Remap_ops[i]];
}

void
call_gencode(struct instable *dp)
{
	if ((dp->optinfo.opgroup == 235) && (opsinfo[0].fmt == OPTINFO_IMMEDIATE) && (opsinfo[0].u.imm.value == 0) && !opsinfo[0].rel && ((dp->optinfo.other & (WREG|ROP0)) == (WREG|ROP0)) && (opsinfo[REGOP].fmt == OPTINFO_REGONLY))
		dp = subst_mov_xor();
	Remap_ops = Allmaps[0];
	Prefix = PREFIX_SUB_I(Curblock, Curstatement);
	if (Genspot + 20 > Sgenbuf) {
		Sgenbuf += 20;
		Genbuf = REALLOC(Genbuf, Sgenbuf * sizeof(unchar));
	}
	gencode(dp);
}

struct instable *
subst_dp(int do_remap)
{
	int i;
	int nsrcs;
	struct instable *newdp;
	struct instable *dp = DP_SUB_I(Curblock, Curstatement);

	if (Last_subst != Addr)
		return(dp);
	for (i = 0, nsrcs = 0; i < NUMOPS; i++) {
		if (opsinfo[i].flags & OPTINFO_SRC)
			nsrcs++;
		set_fmt(i);
	}
	if (nsrcs > 1) {
		for (i = 0; i < NUMOPS; i++) {
			if (subst_table[dp->adr_mode][i] & LEFT)
				Opfmts[i].flags |= LEFT;
		}
	}
	newdp = find_dp(dp);
	if (!newdp)
		return(NULL);
	if (do_remap) {
		if (Remap_ops != Allmaps[0]) {
			remap_ops();
			Opchanged = 1;
		}
	}
	return(newdp);
}
