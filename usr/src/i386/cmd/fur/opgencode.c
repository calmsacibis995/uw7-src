#ident	"@(#)fur:i386/cmd/fur/opgencode.c	1.4"
#ident	"$Header:"

#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "fur.h"
#include "op.h"

extern	struct	instable	distable[16][16];
extern	struct	instable	op0F[13][16];
extern	struct	instable	opFP1n2[8][8];
extern	struct	instable	opFP3[8][8];
extern	struct	instable	opFP4[4][8];
extern	struct	instable	opFP5[8];
extern  struct  instable	op0FC8[4];

void proc_operand();
void putbytes();

void gen_imm_data();
struct indirects {
	unchar num1;
	unchar num2;
	struct instable *table;
};
extern struct instable op80[];
extern struct instable op81[];
extern struct instable op82[];
extern struct instable op83[];
extern struct instable opC0[];
extern struct instable opD0[];
extern struct instable opC1[];
extern struct instable opD1[];
extern struct instable opD2[];
extern struct instable opD3[];
extern struct instable opF6[];
extern struct instable opF7[];
extern struct instable opFE[];
extern struct instable opFF[];

#define NO_SUCH_OPCODE 0x100

struct indirects Indirects[] = {
	{ 0x8, 0x0, op80 },
	{ 0x8, 0x1, op81 },
	{ 0x8, 0x2, op82 },
	{ 0x8, 0x3, op83 },
	{ 0xC, 0x0, opC0 },
	{ 0xD, 0x0, opD0 },
	{ 0xC, 0x1, opC1 },
	{ 0xD, 0x1, opD1 },
	{ 0xD, 0x2, opD2 },
	{ 0xD, 0x3, opD3 },
	{ 0xF, 0x6, opF6 },
	{ 0xF, 0x7, opF7 },
	{ 0xF, 0xE, opFE },
	{ 0xF, 0xF, opFF },
	{ 0, 0 }
};

struct indirects2 {
	unchar num4;
	unchar num5;
	struct instable *table;
};

extern struct instable op0F00[];
extern struct instable op0F01[];
extern struct instable op0FBA[];
extern struct instable op0FC8[];
struct indirects2 Indirects2[] = {
	{ 0x0, 0x0, op0F00 },
	{ 0x0, 0x1, op0F01 },
	{ 0xB, 0xA, op0FBA },
	{ 0xC, 0x8, op0FC8 },
	{ 0, 0 }
};

#define OP0F	256
#define OP0FC8	512

void gen_displacement();

#define	SET_REGNO(P, REG)	(Genbuf[P] &= ~7, Genbuf[P] |= (REG))
#define	SET_BIT(P, BITNO)	(Genbuf[P] |= (1<<((BITNO)-1)))
#define	UNSET_BIT(P, BITNO)	(Genbuf[P] &= ~(1<<((BITNO)-1)))
#define	WBIT(x)	(x & 0x1)		/* to get w bit	*/
#define	VBIT(x)	((x)>>1 & 0x1)		/* to get 'v' bit */
#define	REGNO(x) (x & 0x7)		/* to get 3 bit register */
#define	VBIT(x)	((x)>>1 & 0x1)		/* to get 'v' bit */
#define	OPSIZE(data16,wbit) ((wbit) ? ((data16) ? 2:4) : 1 )

#define	REG_ONLY 3	/* mode indicates a single register with	*/
			/* no displacement is an operand		*/
#define	LONGOPERAND 1	/* value of the w-bit indicating a long		*/
			/* operand (2-bytes or 4-bytes)			*/
#define	SHORTOPERAND 0	/* value of the w-bit indicating an eight-bit
			operand */

static int data16;	/* 16- or 32-bit data */
static int addr16;	/* 16- or 32-bit addressing */
Elf32_Addr Modrm_spot;
static Elf32_Addr first_opcode, second_opcode, third_opcode;
static int modrm_temp;
#define CURSPOT() (Genspot)
#define INCSPOT() (Genspot++)
#define INCSPOT2() (Genspot += 2)
#define INCSPOT4() (Genspot += 4)
#define OP_PUTC(WHAT) (OP_PUT1(CURSPOT(), (WHAT)), INCSPOT())
#define OP_PUT1(TOWHERE, WHAT) PUT1(Genbuf, TOWHERE, WHAT)
#define OP_PUT2(TOWHERE, WHAT) PUT2(Genbuf, TOWHERE, WHAT)
#define OP_PUT4(TOWHERE, WHAT) PUT4(Genbuf, TOWHERE, WHAT)

#define SETUP_MODRM_SPOT() do {\
	Modrm_spot = Genspot;\
	Genspot++;\
} while (0)

#define set_modrm_byte(opcode3, mode, reg, r_m) ((opcode3 == NO_SUCH_OPCODE) ? set_modrm_byte_at(Modrm_spot, mode, reg, r_m) : set_modrm_byte_at(Modrm_spot, mode, opcode3, r_m))
#define set_modrm_byte_at(at, mode, reg, r_m) ((modrm_temp = (r_m) | ((reg) << 3) | ((mode) << 6)), OP_PUT1((at), modrm_temp))
/*#define OPCODE(FROM, HIGH, LOW) ((LOW = (FROM)[0] & 0xf), (HIGH = (FROM)[0] >> 4 & 0xf))*/
#define OPCODE(HIGH, LOW) (((HIGH) << 4) | (LOW))
unchar *Decodepoint;

void
get_opcodes(struct instable *dp, int *popcode1, int *popcode2, int *popcode3, int *popcode4, int *popcode5)
{
	int i;

	*popcode1 = *popcode2 = *popcode3 = *popcode4 = *popcode5 = NO_SUCH_OPCODE;
	if ((dp >= (struct instable *) distable) && (dp < (struct instable *) &distable[16][16])) {
		*popcode1 = (dp - (struct instable *) distable) / 16;
		*popcode2 = (dp - (struct instable *) distable) % 16;
		return;
	}
	if ((dp >= (struct instable *) op0F) && (dp < (struct instable *) &op0F[12][16])) {
		*popcode1 = 0;
		*popcode2 = 0xf;
		*popcode4 = (dp - (struct instable *) op0F) / 16;
		*popcode5 = (dp - (struct instable *) op0F) % 16;
		return;
	}
	for (i = 0; Indirects[i].table; i++)
		if ((dp >= Indirects[i].table) && (dp < Indirects[i].table + 8)) {
			*popcode1 = Indirects[i].num1;
			*popcode2 = Indirects[i].num2;
			*popcode3 = dp - Indirects[i].table;
			return;
		}
	for (i = 0; Indirects2[i].table; i++)
		if ((dp >= Indirects2[i].table) && (dp < Indirects2[i].table + 8)) {
			*popcode1 = 0;
			*popcode2 = 0xF;
			*popcode4 = Indirects2[i].num4;
			*popcode5 = Indirects2[i].num5;
			*popcode3 = dp - Indirects2[i].table;
			return;
		}
	fprintf(stderr, "Cannot find opcode for 0x%x\n", Addr);
	error("Cannot find opcode\n");
}

void
data_prefix()
{
	int i;

	memmove(Genbuf + first_opcode + 1, Genbuf + first_opcode, CURSPOT() - first_opcode);
	Genbuf[first_opcode] = 0x66;
	INCSPOT();
	for (i = 0; i < NUMOPS; i++)
		opsinfo[i].u.imm.loc++;
}

void
addr_prefix()
{
	int i;

	memmove(Genbuf + first_opcode + 1, Genbuf + first_opcode, CURSPOT() - first_opcode);
	Genbuf[first_opcode] = 0x67;
	INCSPOT();
	for (i = 0; i < NUMOPS; i++)
		opsinfo[i].u.imm.loc++;
}

void
dis_me(char *addr, Elf32_Addr ref_addr, int size)
{
	int (*hold_optinfo)();
	int hold_silent;
	char *hold_p_data;
	Elf32_Shdr hold_shdr;

	if (size <= 0)
		return;
	hold_optinfo = Optinfo;
	Optinfo = NULL;

	hold_silent = Silent_mode;
	Silent_mode = 0;

	hold_p_data = p_data;
	p_data = (char *) addr;

	hold_shdr = Shdr;
	Shdr.sh_addr = ref_addr;
	Shdr.sh_size = size;

	dis_text(&Shdr);

	Optinfo = hold_optinfo;
	Silent_mode = hold_silent;
	p_data = hold_p_data;
	Shdr = hold_shdr;
	FLUSH_OUT();
}

void
gencode(struct instable *dp)
{
	int i;
	int plongoperand[1];
	int opcode1, opcode2, opcode3, opcode4, opcode5;
	int opcode_bytes;
	int mode, r_m, wbit;
	char *oldbuf = (char *) Text_data->d_buf + Addr;
	char *newbuf = Genbuf + CURSPOT();

	if (dp->name[0] == 'f')
		return;
	Modrm_spot = NO_SUCH_ADDR;
	*plongoperand = LONGOPERAND;
	addr16 = 0;
	data16 = 0;
	opcode_bytes = 1;

	for (i = 0; Prefix[i]; i++) {
		get_opcodes(Prefix[i], &opcode1, &opcode2, &opcode3, &opcode4, &opcode5);
		OP_PUTC(OPCODE(opcode1, opcode2));
	}

	first_opcode = CURSPOT();
	get_opcodes(dp, &opcode1, &opcode2, &opcode3, &opcode4, &opcode5);
	wbit = WBIT(opcode2);
	OP_PUTC(OPCODE(opcode1, opcode2));
	if (opcode4 != NO_SUCH_OPCODE) {
		opcode_bytes++;
		second_opcode = CURSPOT();
		OP_PUTC(OPCODE(opcode4, opcode5));
		if (opcode3 != NO_SUCH_OPCODE) {
			opcode_bytes++;
			third_opcode = CURSPOT();
			SETUP_MODRM_SPOT();
		}
	}
	else if (opcode3 != NO_SUCH_OPCODE) {
		opcode_bytes++;
		second_opcode = CURSPOT();
		SETUP_MODRM_SPOT();
	}

	/*
	 * Each instruction has a particular instruction syntax format
	 * stored in the disassembly tables.  The assignment of formats
	 * to instructins was made by the author.  Individual formats
	 * are explained as they are encountered in the following
	 * switch construct.
	 */

	switch(dp->adr_mode) {
	/* movsbl movsbw (0x0FBE) or movswl (0x0FBF) */
	/* movzbl movzbw (0x0FB6) or mobzwl (0x0FB7) */
	/* wbit lives in 2nd byte, note that operands are different sized */
	case MOVZ:
		if (Modrm_spot == NO_SUCH_ADDR)
			SETUP_MODRM_SPOT();
		proc_operand(&mode, &r_m, &wbit, 0);
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		/* Ignore what proc_operand says about data16 */
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		else
			data16 = 0;
		break;

	/* imul instruction, with either 8-bit or longer immediate */
	case IMUL:
		if (Modrm_spot == NO_SUCH_ADDR)
			SETUP_MODRM_SPOT();
		proc_operand(&mode, &r_m, plongoperand, 1);
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		gen_imm_data(OPSIZE(data16, opcode2 == 0x9), 0);
		break;

	/* memory or register operand to register, with 'w' bit	*/
	case MRw:
		if (Modrm_spot == NO_SUCH_ADDR)
			SETUP_MODRM_SPOT();
		proc_operand(&mode, &r_m, &wbit, 0);
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		break;

	/* register to memory or register operand, with 'w' bit	*/
	case RMw:
		if (Modrm_spot == NO_SUCH_ADDR)
			SETUP_MODRM_SPOT();
		proc_operand(&mode, &r_m, &wbit, 0);
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_EIGHTBIT)
			wbit = SHORTOPERAND;
		else
			wbit = LONGOPERAND;
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		break;
	case RM16:
		if (Modrm_spot == NO_SUCH_ADDR)
			SETUP_MODRM_SPOT();
		proc_operand(&mode, &r_m, plongoperand, 0);
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		break;

	/* Double shift. Has immediate operand specifying the shift. */
	case DSHIFT:
		SETUP_MODRM_SPOT();
		proc_operand(&mode, &r_m, plongoperand, 1);
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		gen_imm_data(1, 0);
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		break;

	/* Double shift. With no immediate operand, specifies using %cl. */
	case DSHIFTcl:
		SETUP_MODRM_SPOT();
		proc_operand(&mode, &r_m, plongoperand, 0);
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		break;

	/* immediate to memory or register operand */
	case IMlw:
		proc_operand(&mode, &r_m, &wbit, 1);
		gen_imm_data(OPSIZE(data16,1), 0, 0);
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		break;

	/* immediate to memory or register operand */
	case IM8w:
		proc_operand(&mode, &r_m, &wbit, 1);
		gen_imm_data(1, 0, 1);
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		break;

	/* immediate to memory or register operand with the	*/
	/* 'w' bit present					*/
	case IMw:
		if (Modrm_spot == NO_SUCH_ADDR)
			SETUP_MODRM_SPOT();
		proc_operand(&mode, &r_m, &wbit, 1);
		gen_imm_data(OPSIZE(data16,wbit), 0, 0);
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		break;

	/* immediate to register with register in low 3 bits	*/
	/* of op code						*/
	case IR:
		SET_REGNO(first_opcode, opsinfo[REMAP(REGOP)].reg);
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_EIGHTBIT) {
			UNSET_BIT(first_opcode, 4);
			gen_imm_data(1, 0, 0);
		}
		else {
			SET_BIT(first_opcode, 4);
			if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN) {
				data16 = 1;
				gen_imm_data(2, 0, 0);
			}
			else
				gen_imm_data(4, 0, 0);
		}
		break;

	/* memory operand to accumulator			*/
	case OA:
		gen_displacement(4,0);
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		break;

	/* accumulator to memory operand			*/
	case AO:
		gen_displacement(4,0);
		if (opsinfo[REMAP(0)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		break;

	/* memory or register operand to segment register	*/
	case MS:
		if (Modrm_spot == NO_SUCH_ADDR)
			SETUP_MODRM_SPOT();
		proc_operand(&mode, &r_m, plongoperand, 0);
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		break;

	/* segment register to memory or register operand	*/
	case SM:
		if (Modrm_spot == NO_SUCH_ADDR)
			SETUP_MODRM_SPOT();
		proc_operand(&mode, &r_m, plongoperand, 0);
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		break;

	/* rotate or shift instrutions, which may shift by 1 or */
	/* consult the cl register, depending on the 'v' bit	*/
	case Mv:
		proc_operand(&mode, &r_m, &wbit, 0);
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		break;

	/* immediate rotate or shift instrutions, which may or */
	/* may not consult the cl register, depending on the 'v' bit	*/
	case MvI:
		proc_operand(&mode, &r_m, &wbit, 0);
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		gen_imm_data(1,1);
		break;

	case MIb:
		proc_operand(&mode, &r_m, plongoperand, 0);
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		gen_imm_data(1,1);
		break;

	/* single memory or register operand with 'w' bit present*/
	case Mw:
		proc_operand(&mode, &r_m, &wbit, 0);
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		break;

	/* 16 bit single memory or register operand		*/
	case M16:
	/* single memory or register operand			*/
	case M:
		if (Modrm_spot == NO_SUCH_ADDR)
			SETUP_MODRM_SPOT();
		proc_operand(&mode, &r_m, plongoperand, 0);
		/* Even though it seems like the register is not viewed
		** put a 0 there anyway
		*/
		set_modrm_byte(opcode3, mode, 0, r_m);
		break;

	case SREG: /* special register */
		SETUP_MODRM_SPOT();
		/* Use 3 for the mode because the reference manual says so */
		if (VBIT(opcode5))
			set_modrm_byte(opcode3, 0x3, opsinfo[REMAP(0)].reg, opsinfo[REMAP(REGOP)].reg);
		else
			set_modrm_byte(opcode3, 0x3, opsinfo[REMAP(REGOP)].reg, opsinfo[REMAP(0)].reg);
		break;

	/* single register operand with register in the low 3	*/
	/* bits of op code					*/
	case R:
		SET_REGNO(first_opcode, opsinfo[REMAP(REGOP)].reg);
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		break;

	/* register to accumulator with register in the low 3	*/
	/* bits of op code, xchg instructions                   */
	case RA:
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		break;

	/* single segment register operand, with register in	*/
	/* bits 3-4 of op code					*/
	case SEG:
		break;

	/* single segment register operand, with register in	*/
	/* bits 3-5 of op code					*/
	case LSEG:
		break;

	/* memory or register operand to register		*/
	case MR:
		if (Modrm_spot == NO_SUCH_ADDR)
			SETUP_MODRM_SPOT();
		proc_operand(&mode, &r_m, plongoperand, 0);
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		break;

	/* immediate operand to accumulator			*/
	case IA: {
		int x = opsinfo[REMAP(REGOP)].flags & (OPTINFO_SIXTEEN|OPTINFO_EIGHTBIT);
		int no_bytes;

/*		switch((opsinfo[REMAP(REGOP)].flags|opsinfo[REMAP(0)].flags) & (OPTINFO_SIXTEEN|OPTINFO_EIGHTBIT))*/
/*		switch(opsinfo[REMAP(REGOP)].flags & (OPTINFO_SIXTEEN|OPTINFO_EIGHTBIT)) {*/
		switch(x) {
		case OPTINFO_EIGHTBIT:
			no_bytes = 1;
			break;
		case OPTINFO_SIXTEEN:
			data16 = 1;
			no_bytes = 2;
			break;
		default:
			no_bytes = 4;
		}
		gen_imm_data(no_bytes, 0);
		break;
	}
	/* memory or register operand to accumulator		*/
	case MA:
		proc_operand(&mode, &r_m, &wbit, 0);
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		break;

	/* si register to di register				*/
	case SD:
		if (opsinfo[REMAP(1)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		break;

	/* accumulator to di register				*/
	case AD:
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		if (opsinfo[REMAP(1)].flags & OPTINFO_SIXTEEN)
			addr16 = 1;
		break;

	/* si register to accumulator				*/
	case SA:
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		if (opsinfo[REMAP(1)].flags & OPTINFO_SIXTEEN)
			addr16 = 1;
		break;

	/* single operand, a 16/32 bit displacement		*/
	/* added to current offset by 'compoff'			*/
	case D:
		if (opsinfo[REMAP(0)].flags & OPTINFO_SIXTEEN) {
			data16 = 1;
			gen_displacement(2,0);
		}
		else
			gen_displacement(4,0);
		break;

	/* indirect to memory or register operand		*/
	case INM:
		proc_operand(&mode, &r_m, plongoperand, 0);
		set_modrm_byte(opcode3, mode, opsinfo[REMAP(REGOP)].reg, r_m);
		break;

	/* for long jumps and long calls -- a new code segment   */
	/* register and an offset in IP -- stored in object      */
	/* code in reverse order                                 */
	case SO:
		gen_displacement(4, 1);
		gen_displacement(2, 0);
		break;

	/* jmp/call. single operand, 8 bit displacement.	*/
	/* added to current EIP in 'compoff'			*/
	case BD:
		gen_displacement(1, 0);
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		break;

	/* single 32/16 bit immediate operand			*/
	case I:
		if (opsinfo[REMAP(0)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		gen_imm_data(OPSIZE(data16,LONGOPERAND), 0);
		break;

	/* single 8 bit immediate operand			*/
	case Ib:
		gen_imm_data(1, 0);
		break;

	case ENTER:
		gen_imm_data(2,0);
		gen_imm_data(1,1);
		break;

	/* 16-bit immediate operand */
	case RET:
		gen_imm_data(2,0);
		break;

	/* single 8 bit port operand				*/
	case P:
		gen_imm_data(1, 0);
		break;

	/* single operand, dx register (variable port instruction)*/
	case V:
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		break;

	/* The int instruction, which has two forms: int 3 (breakpoint) or  */
	/* int n, where n is indicated in the subsequent byte (format Ib).  */
	/* The int 3 instruction (opcode 0xCC), where, although the 3 looks */
	/* like an operand, it is implied by the opcode. It must be converted */
	/* to the correct base and output. */
	case INT3:
		break;

	/* an unused byte must be discarded			*/
	case U:
		INCSPOT();
		break;

	case CBW:
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		break;

	case CWD:
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		break;

	/* no disassembly, the mnemonic was all there was	*/
	/* so go on						*/
	case GO_ON:
		if (opsinfo[REMAP(REGOP)].flags & OPTINFO_SIXTEEN)
			data16 = 1;
		break;

	/* float reg */
	case F:
		break;

	/* float reg to float reg, with ret bit present */
	case FF:
		break;

	/* an invalid op code */
	case AM:
	case DM:
	case OVERRIDE:
	case PREFIX:
	case UNKNOWN:
		bad_opcode();
		break;

	default:
		error("The opcode for %s is not generatable yet\n", dp->name);
		break;
	} /* end switch */
	if (data16)
		data_prefix();
	if (addr16)
		addr_prefix();
	if (Test_generation) {
		int ret = CURSPOT() == Next_addr - Addr;

		if (!ret || (memcmp(oldbuf, Genbuf, CURSPOT()) != 0)) {
			dis_me(oldbuf, Addr, 1);
			printf("\tGenerated: ");
			dis_me(newbuf, Addr, 1);
		}
	}
	else if (Test_optimization)
		dis_me(newbuf, Addr, 1);
}

void
putbytes(int no_bytes, long value)
{
	switch(no_bytes) {
	case 1:
		OP_PUT1(CURSPOT(), value);
		INCSPOT();
		break;
	case 2:
		OP_PUT2(CURSPOT(), value);
		INCSPOT2();
		break;
	case 4:
		OP_PUT4(CURSPOT(), value);
		INCSPOT4();
		break;
	}
}

/*
 *	void gen_displacement (no_bytes, opindex, value)
 *
 *	Get and print in the 'operand' array a one, two or four
 *	byte displacement from a register.
 */

void
gen_displacement(no_bytes, opindex)
int no_bytes;
int opindex;
{
	opindex = REMAP(opindex);
	if (no_bytes == 0) {
		if ((opsinfo[opindex].u.disp.value > 65535) || (opsinfo[opindex].u.disp.value < -65535)) {
			data16 = 1;
			no_bytes = 2;
		}
		else
			no_bytes = 4;
	}
	opsinfo[opindex].u.disp.loc = CURSPOT();
	putbytes(no_bytes, opsinfo[opindex].u.disp.value);
}

void
proc_operand(unsigned int *pmode, unsigned int *pr_m, int *pwbit, int opindex)
{
	int dispsize;
	int remapped_opindex = REMAP(opindex);

	if (opsinfo[remapped_opindex].fmt == OPTINFO_DISPONLY) {
		*pmode = 0;
		dispsize = 4;
	}
	else if (opsinfo[remapped_opindex].flags & OPTINFO_REL) {
		*pmode = 2;
		dispsize = 4;
	}
	else if ((opsinfo[remapped_opindex].u.disp.value > 127) || (opsinfo[remapped_opindex].u.disp.value < -128)) {
		*pmode = 2;
		dispsize = addr16 ? 2 : 4;
	}
	else if (opsinfo[remapped_opindex].u.disp.value) {
		*pmode = 1;
		dispsize  = 1;
	}
	else if (((opsinfo[remapped_opindex].fmt == OPTINFO_DISPREG) || (opsinfo[remapped_opindex].fmt == OPTINFO_SIB)) && (opsinfo[remapped_opindex].reg == EBP)) {
		*pmode = 1;
		dispsize = 1;
	}
	else {
		*pmode = 0;
		dispsize = 0;
	}

/*	*pwbit = LONGOPERAND;*/
	if (opsinfo[remapped_opindex].flags & OPTINFO_SIXTEEN)
		data16 = 1;
	switch (opsinfo[remapped_opindex].fmt) {
	case OPTINFO_SIB:
		*pr_m = ESP;
		if (opsinfo[remapped_opindex].index == NO_REG)
			set_modrm_byte_at(CURSPOT(), opsinfo[remapped_opindex].scale, ESP, opsinfo[remapped_opindex].reg);
		else
			set_modrm_byte_at(CURSPOT(), opsinfo[remapped_opindex].scale, opsinfo[remapped_opindex].index, opsinfo[remapped_opindex].reg);
		INCSPOT();
		break;
	case OPTINFO_SIB_NOREG:
		*pmode = 0;
		dispsize = 4;
		*pr_m = ESP;
		set_modrm_byte_at(CURSPOT(), opsinfo[remapped_opindex].scale, opsinfo[remapped_opindex].index, EBP);
		INCSPOT();
		break;
	case OPTINFO_REGONLY:
		if (!(opsinfo[remapped_opindex].flags & OPTINFO_EIGHTBIT))
			*pwbit = LONGOPERAND;
		else
			*pwbit = SHORTOPERAND;
		*pmode = REG_ONLY;
		*pr_m = opsinfo[remapped_opindex].reg;
		break;
	case OPTINFO_DISPONLY:
		*pr_m = EBP;
		break;
	case OPTINFO_DISPREG:
		*pr_m = opsinfo[remapped_opindex].reg;
	}
	if (dispsize)
		gen_displacement(dispsize, opindex);
}

void
gen_imm_data(no_bytes, opindex)
int no_bytes;
int opindex;
{
	opindex = REMAP(opindex);
	opsinfo[opindex].u.imm.loc = CURSPOT();
	putbytes(no_bytes, opsinfo[opindex].u.imm.value);
}
