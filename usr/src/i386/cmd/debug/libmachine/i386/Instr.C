#ident	"@(#)debugger:libmachine/i386/Instr.C	1.30"

#include "ProcObj.h"
#include "Itype.h"
#include "Instr.h"
#include "dis.h"
#include "Reg1.h"
#include "Interface.h"
#include "Symbol.h"
#include "Dyn_info.h"
#include "Msgtab.h"
#include <stdio.h>
#include <string.h>


// data structures to hold information about an operand
enum Optype {
	op_none,
	op_reg,	
	op_reg_off,
	op_scale_index,		
	op_val,		// plain hex value - 0xnnnn
	op_immediate,	// immediate - $0xnnnn
	op_mem,		// memory - 0xnnnnn
	op_sym_off,	// symbolic offset - +0xdddd <nnnn>
};

// values for flags
#define	op_disp		0x1	// value is a displacement off a reg
#define	op_indirect	0x2	// indirect through register or memory

struct Operand {
	Optype		type;
	const char	*reg;
	const char	*overreg;
	const char	*index;
	const char	*scale;
	long		value;
	long		offset;
	int		reg_index;	// for find_return
	int		flags;
};

// static data - could have been declared within the Instr
// class, but it's not preserved or used across multiple
// invocations of the public functions
static Operand		operand[3];
static unsigned char	byte[NLINE];
static const char    	*overreg;	// save the segment override 
					// register if any 
static int 		data16;		// 16- or 32-bit data 
static int		addr16;		// 16- or 32-bit addressing 
static int		byte_cnt;	// position in byte array

Instr::Instr(ProcObj *p)
{
	pobj = p;
	loaddr = hiaddr = 0;
}

inline
static unsigned char
get_byte()
{
	return byte[byte_cnt++];
}

inline
static void
pushback()
{
	if (byte_cnt > 0)
		byte_cnt--;
}

static void
setreg(Operand *opnd, int r_m, int wbit)
{
	opnd->type = op_reg;
	if (data16)
	{
		opnd->reg = REG16[r_m][wbit] ;
		opnd->reg_index = REG16_IND[r_m][wbit] ;
	}
	else
	{
		opnd->reg = REG32[r_m][wbit];
		opnd->reg_index = REG32_IND[r_m][wbit] ;
	}
}

// get text, replace breakpoints with original text.
// get_text_nobkpt reads the BUFSIZ no of bytes from the address start
// we cache the start and end addresses to cut down on re-reads
int
Instr::get_text_nobkpt( Iaddr start)
{
	
	char	*oldtext;
	int	cnt, idx; 
	int	cur_byte;

	if ( pobj == 0 )
	{
		printe(ERR_internal, E_ERROR, "Instr:get_text_no_bkpt",
			__LINE__);
		return 0;
	}
	else if (start == 0)
	{
		return 0;
	}

	byte_cnt = 0;
	if ((loaddr > 0) && (start >= loaddr) && (start < (hiaddr - NLINE)))
	{
		// already read those bytes
		// always need at least enough for 1 instruction
		cur_byte = (int)(start - loaddr);
	}
	else
	{
		if ((cnt = pobj->read(start, BUFSIZ,
			(char *)save_bytes )) < NLINE )
		{
			loaddr = hiaddr = 0;
			return 0;
		}
		cur_byte = 0;
		loaddr = start;
		hiaddr = start + cnt;
	}
	memcpy(byte, &save_bytes[cur_byte], NLINE);

	// if there are any breakpoints in byte[] replace them
	// with the original text
	for (idx = 0; idx < NLINE; idx++)
	{
		oldtext = pobj->text_nobkpt(start + idx);
		if ( oldtext  != 0 )  
			byte[idx] = *oldtext;
	}
	return 1;
}

// The buffer saved by get_text_nobkpt may have contained bkpts.
// We only get original text as needed.  But in the meantime, a bkpt
// may have been deleted, so we would have no way of getting the
// original text.  The ProcObj class solves this for us by letting
// us know the oldtext value whenever a bkpt is deleted.

void
Instr::set_text(Iaddr addr, const char *oldtext)
{
	if ((loaddr > 0) && (addr >= loaddr) && (addr < hiaddr))
	{
		save_bytes[addr - loaddr] = *oldtext;
	}
}

// get_text reads and returns the single byte at address addr
int
Instr::get_text(Iaddr addr, unsigned char &current)
{
	if ( pobj == 0 || addr == 0 )
		return 0;
	if (pobj->read(addr, sizeof(unsigned char), (char *)&current)
		!= sizeof(unsigned char))
		return 0;
	return 1;
}

// get_opcode (high, low)
// Get the next byte and separate the op code into the high and
//  low nibbles.

unsigned char
Instr::get_opcode(unsigned * high, unsigned  * low)
{
	unsigned char	curbyte = get_byte();

	*low = curbyte & 0xf; 	 	// ----xxxx low 4 bits 
	*high = curbyte >> 4 & 0xf;  	// xxxx---- bits 7 to 4
	return curbyte;
}

// Get the byte following the op code and separate it into the
// mode, register, and r/m fields.
// Scale-Index-Bytes have a similar format.
void
Instr::get_modrm_byte(unsigned *mode, unsigned *reg, unsigned *r_m)
{
	unsigned char curbyte = get_byte();

	*r_m = curbyte & 0x7; 
	*reg = curbyte >> 3 & 0x7;
	*mode = curbyte >> 6 & 0x3;
}

//  Check to see if there is a segment override prefix pending.
inline
void
Instr::check_override(Operand *opnd)
{
	opnd->overreg = overreg;
	overreg = 0;
}

// imm_data() reads size bytes from byte[] and converts them into a
// sign extended value
void
Instr::imm_data(int size, Operand *opnd)
{
	unsigned char	curbyte;
	int 		j;
	unsigned long	shiftbuf = 0;
	int		shift = 0;

	for (j=0; j < size; j++, shift += 8) 
	{
		curbyte = get_byte();
		shiftbuf |= (unsigned long) curbyte << shift;
	}
	switch(size) 
	{
	case 1:
		if (shiftbuf & 0x80)
		{
			opnd->value = (shiftbuf | ~0xffL);
		}
		else
		{
			opnd->value = (char)shiftbuf;
		}
		break;
	case 2:
		opnd->value = (short)shiftbuf;
		break;
	case 4:
	default:
		opnd->value = shiftbuf;
		break;
	}
}

// Decode a register or memory operand and store results in opnd.
void
Instr::get_operand(unsigned mode, unsigned r_m, int wbit, Operand *opnd)

{
	int		dispsize;  // size of displacement in bytes
	int		s_i_b;     // flag presence of scale-index-byte 
	unsigned	ss;    // scale-factor from opcode
	unsigned 	index; // index register number
	unsigned 	base;  // base register number

	check_override(opnd);

	// check for the presence of the s-i-b byte 
	if (r_m==REG_ESP && mode!=REG_ONLY && !addr16) 
	{
		s_i_b = TRUE;
		get_modrm_byte(&ss, &index, &base);
	}
	else
		s_i_b = FALSE;

	if (addr16)
		dispsize = dispsize16[r_m][mode];
	else
		dispsize = dispsize32[r_m][mode];

	if (s_i_b && mode==0 && base==REG_EBP) 
		dispsize = 4;

	if (dispsize != 0)
	{
		imm_data(dispsize, opnd);
		opnd->flags |= op_disp;
	}

	if (s_i_b) 
	{
		opnd->reg = regname32[mode][base];
		if (*indexname[index])
		{
			opnd->index = indexname[index];
			opnd->scale = scale_factor[ss];
			opnd->type = op_scale_index;
		}
		else
			opnd->type = op_reg_off;
	}
	else if (mode == REG_ONLY) 
	{
		setreg(opnd, r_m, wbit);
	}
	else if (r_m ==REG_EBP && mode == 0) 
	{ 
		// displacement only - memory opnd
		opnd->flags &= ~op_disp;
		opnd->type = op_mem;
	}
	else 
	{ 
		// Modes 00, 01, or 10, not displacement
		// only, and no s-i-b 
		if (addr16)
			opnd->reg = regname16[mode][r_m];
		else
			opnd->reg = regname32[mode][r_m];
		opnd->type = op_reg_off;
	}
}


struct instr_data {
	unsigned short	op;
	unsigned short	op2;
	unsigned int	mode;
	unsigned int	r_m;
	unsigned int	opcode2;
	unsigned int	opcode3;
	unsigned int	opcode5;
	int		got_modrm;
	const char 	*prefix;
};

// get table entry for instruction contained in byte[] array
const instable *
Instr::get_instr(instr_data *id)
{
	unsigned short	curbyte;
	unsigned	opcode1, opcode2, opcode3, opcode4, opcode5;
	const instable	*dp;
	int		got_modrm_byte = 0;
	unsigned	mode, r_m;

	// As long as there is a prefix, the default segment register,
	// addressing-mode, or data-mode in the instruction will be overridden.
	// This may be more general than the chip actually is.

	id->prefix = 0;
	for(;;) 
	{
		curbyte = get_opcode(&opcode1, &opcode2);
		dp = &distable[opcode1][opcode2];

		if ( dp->adr_mode == PREFIX ) 
			id->prefix = dp->name;
		else if ( dp->adr_mode == AM ) 
			addr16 = !addr16;
		else if ( dp->adr_mode == DM ) 
			data16 = !data16;
		else if ( dp->adr_mode == OVERRIDE ) 
			overreg = dp->name;
		else break;
	}

	// Some 386 instructions have 2 bytes of opcode 
	// before the mod_r/m 

	unsigned char	op;
	if (curbyte == 0x0F)
	{
		op = get_opcode(&opcode4, &opcode5);
		if (opcode4 > 12)  
		{
			return 0;
		}
	}
	// Some instructions have opcodes for which several
	// instructions exist.  Part of the mod/rm byte is
	// used to distinguish among them.

	got_modrm_byte = 1;
	get_modrm_byte(&mode, &opcode3, &r_m);

	if (opcode1 == 0xD && opcode2 >= 0x8) 
	{
		// instruction form 5
		if (opcode2 == 0xB && mode == 0x3 && opcode3 == 4)
		{
			dp = &opFP5[r_m];
		}
		else if (opcode2 == 0xB && mode == 0x3 && opcode3 == 7)
		{
			// opcode3 == 5 and 6 are new for P6 processor
			return 0;
		}
		// instruction form 4
		else if (opcode2 == 0x9 && mode==0x3 && opcode3 >= 4)
			dp = &opFP4[opcode3-4][r_m];
		// instruction form 3
		else if (mode == 0x3)
			dp = &opFP3[opcode2-8][opcode3];
		// instruction form 1 and 2
		else dp = &opFP1n2[opcode2-8][opcode3];
	}
	else
	{
		switch(curbyte)
		{
		case 0x0F:
			switch(op)
			{
			case 0x0:	
				dp = &op0F00[opcode3];
				break;
			case 0x1:
				dp = &op0F01[opcode3];
				break;
			case 0xBA:
				dp = &op0FBA[opcode3];
				break;
			case 0xC8:
				dp = &op0FC8[opcode3];
				break;
			default:
				if (opcode4 >= 0x8)
					// table is compressed
					// invalid entries 0x50-0x7F
					// have been deleted
					dp = &op0F[opcode4-3][opcode5];
				else if (opcode4 >= 0x5)
					// invalid range
					dp = &op0F[0x2][0xF];
				else
					dp = &op0F[opcode4][opcode5];
				got_modrm_byte = 0;
				pushback();  // reset 
				break;
			}
			break;
		case 0x80:
			dp = &op80[opcode3];
			break;
		case 0x81:
			dp = &op81[opcode3];
			break;
		case 0x82:
			dp = &op82[opcode3];
			break;
		case 0x83:
			dp = &op83[opcode3];
			break;
		case 0xC0:
			dp = &opC0[opcode3];
			break;
		case 0xC1:
			dp = &opC1[opcode3];
			break;
		case 0xD0:
			dp = &opD0[opcode3];
			break;
		case 0xD1:
			dp = &opD1[opcode3];
			break;
		case 0xD2:
			dp = &opD2[opcode3];
			break;
		case 0xD3:
			dp = &opD3[opcode3];
			break;
		case 0xF6:
			dp = &opF6[opcode3];
			break;
		case 0xF7:
			dp = &opF7[opcode3];
			break;
		case 0xFE:
			dp = &opFE[opcode3];
			break;
		case 0xFF:
			dp = &opFF[opcode3];
			break;
		default:
			// reset to get modrm only if needed
			pushback();
			got_modrm_byte = 0;
			break;
		}
	}
	id->op = curbyte;
	id->op2 = op;
	id->opcode2 = opcode2;
	id->opcode3 = opcode3;
	id->opcode5 = opcode5;
	id->got_modrm = got_modrm_byte;
	if (got_modrm_byte)
	{
		id->mode = mode;
		id->r_m = r_m;
	}
	return dp;
}

int
Instr::get_operands(Iaddr pcaddr, int addr_mode, 
	instr_data *id, int &inst_size)
{
	unsigned	mode, reg, r_m;
	int		wbit, vbit;
	unsigned char	curbyte;

	int 		got_modrm_byte;
	unsigned 	opcode2;
	Operand		*source1, *source2, *dest;
	int		i;

	for(dest = &operand[0], i = 0; i <= 2; dest++, i++)
	{
		dest->type = op_none;
		dest->flags = 0;
		dest->overreg = 0;
		dest->reg_index = 0;
	}
	source1 = &operand[0];
	source2 = &operand[1];
	dest = &operand[2];

	curbyte = id->op;
	opcode2 = id->opcode2;
	got_modrm_byte = id->got_modrm;
	if (got_modrm_byte)
	{
		mode = id->mode;
		r_m = id->r_m;
	}

	// Each instruction has a particular instruction syntax format
	// stored in the disassembly tables.  The assignment of formats
	// to instructions was made by the author.  Individual formats
	// are explained as they are encountered in the following
	// switch construct.

	switch (addr_mode) 
	{
	case MOVZ:
		// movsbl movsbw (0x0FBE) or movswl (0x0FBF)
		// movzbl movzbw (0x0FB6) or mobzwl (0x0FB7) 
		// wbit lives in 2nd byte, 
		// note that operands are different sized 
		if ( ! got_modrm_byte )
			get_modrm_byte(&mode, &reg, &r_m);
		setreg(dest, reg, LONGOPERAND);
		wbit = WBIT(id->opcode5);
		data16 = 1;
		get_operand(mode, r_m, wbit, source1);
		break;

	case IMUL:
		// imul instruction, with either 8-bit or longer immediate
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, source2 );
		// opcode 0x6B for byte, sign-extended displacement, 0x69 for word(s)
		imm_data( OPSIZE(data16,opcode2 == 0x9), source1);
		source1->type = op_immediate;
		setreg(dest, reg, LONGOPERAND);
		break;

	case MRw:
		// memory or register operand to register, with 'w' bit
		wbit = WBIT(opcode2);
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, wbit, source1 );
		setreg(dest, reg, wbit);
		break;

	case RMw:
		// register to memory or register operand, with 'w' bit
		// arpl happens to fit here also because it is odd
		if (curbyte == 0x0F)
			wbit = WBIT(id->opcode5);
		else
			wbit = WBIT(opcode2);
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, wbit, dest );
		setreg(source1, reg, wbit);
		break;

	case DSHIFT:
		// Double shift. Has immediate operand specifying
		// the shift.
		// opcode determined by opcode4 and opcode5 
		// - we know we haven't got modrm
		get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, dest );
		setreg(source2, reg, LONGOPERAND);
		imm_data(1, source1);
		source1->type = op_immediate;
		break;

	case DSHIFTcl:
		// Double shift. With no immediate operand,
		// specifies using %cl.
		// opcode determined by opcode4 and opcode5 
		// - we know we haven't got modrm
		get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, dest );
		setreg(source1, reg, LONGOPERAND);
		break;

	case IMlw:
		// immediate to memory or register operand
		wbit = WBIT(opcode2);
		get_operand(mode, r_m, wbit, dest );
		/* A long immediate is expected for opcode 0x81, not 0x80 nor 0x83 */
		imm_data(OPSIZE(data16,opcode2 == 1), source1);
		source1->type = op_immediate;
		break;

	case IMw:
		// immediate to memory or register operand with the
		// 'w' bit present
		wbit = WBIT(opcode2);
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, wbit, dest );
		imm_data(OPSIZE(data16,wbit), source1);
		source1->type = op_immediate;
		break;

	case IR:
		// immediate to register with register in low 3 bits
		// of op code
		wbit = opcode2 >>3 & 0x1; 
		// w-bit here (with regs) is bit 3
		if (curbyte == 0x0F)
			reg = REGNO(id->opcode5);
		else
			reg = REGNO(opcode2);
		imm_data( OPSIZE(data16,wbit), source1);
		source1->type = op_immediate;
		setreg(dest, reg, wbit);
		break;

	case OA:
		// memory operand to accumulator
		wbit = WBIT(opcode2);
		imm_data(OPSIZE(addr16,LONGOPERAND), source1);
		source1->type = op_mem;
		setreg(dest, 0, wbit);
		break;

	case AO:
		// accumulator to memory operand
		wbit = WBIT(opcode2);
		imm_data(OPSIZE(addr16,LONGOPERAND), dest);
		dest->type = op_mem;
		setreg(source1, 0, wbit);
		break;

	case MS:
		// memory or register operand to segment register
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, source1);
		dest->reg = SEGREG[reg];
		dest->type = op_reg;
		break;

	case SM:
		// segment register to memory or register operand
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, dest );
		source1->reg = SEGREG[reg];
		source1->type = op_reg;
		break;

	case Mv:
		// register to memory or register operand, with 'w' bit
		// arpl happens to fit here also because it is odd
		vbit = VBIT(opcode2);
		wbit = WBIT(opcode2);
		get_operand(mode, r_m, wbit, dest );
		/* When vbit is set, register is an operand, otherwise just $0x1 */
		if (vbit)
		{
			source1->reg = "%cl";
			source1->type = op_reg;
		}
		break;

	case MvI:
		// immediate rotate or shift instrutions, which may or
		// may not consult the cl register, depending on the 'v' bit
		vbit = VBIT(opcode2);
		wbit = WBIT(opcode2);
		get_operand(mode, r_m, wbit, dest );
		imm_data(1,source1);
		source1->type = op_immediate;
		/* When vbit is set, register is an operand, otherwise just $0x1 */
		if (vbit)
		{
			source2->reg = "%cl";
			source2->type = op_reg;
		}
		break;

	case MIb:
		// immediate to register or memory
		get_operand(mode, r_m, LONGOPERAND, dest );
		imm_data(1,source1);
		source1->type = op_immediate;
		break;

	case Mw:
		// single memory or register operand with 'w' bit present
		wbit = WBIT(opcode2);
		get_operand(mode, r_m, wbit, dest );
		break;

	case M:
		// single memory or register operand
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, dest );
		break;

	case SREG:
		// special register 
		// opcode determined by opcode4 and opcode5 
		// - we know we haven't got modrm
		get_modrm_byte(&mode, &reg, &r_m);
		vbit = 0;
		switch (id->opcode5) 
		{
		case 2:
			vbit = 1;
			// fall thru 
		case 0: 
			source1->reg = CONTROLREG[reg];
			break;
		case 3:
			vbit = 1;
			/* fall thru */
		case 1:
			source1->reg = DEBUGREG[reg];
			break;
		case 6:
			vbit = 1;
			/* fall thru */
		case 4:
			source1->reg = TESTREG[reg];
			break;
		}
		dest->reg = REG32[r_m][1];

		if (vbit)
		{
			dest->reg = source1->reg;
			source1->reg = REG32[r_m][1];
		}
		dest->type = source1->type = op_reg;
		break;

	case R:
		// single register operand with register in the low 3
		// bits of op code
		reg = REGNO(opcode2);
		setreg(dest, reg, LONGOPERAND);
		break;

	case RA: 
		// register to accumulator with register in the low 3
		// bits of op code, xchg instructions
		reg = REGNO(opcode2);
		if (data16) 
		{
			dest->reg = "%ax";
		}
		else
		{
			dest->reg = "%eax";
		}
		dest->type = op_reg;
		setreg(source1, reg, LONGOPERAND);
		break;

	case SEG:
		// single segment register operand, with register in
		// bits 3-4 of op code
		reg = curbyte >> 3 & 0x3; // segment register 
		dest->reg = SEGREG[reg];
		dest->type = op_reg;
		break;

	case LSEG:
		// single segment register operand, with register in
		// bits 3-5 of op code
		reg = curbyte >> 3 & 0x7; // long seg reg from opcode
		dest->reg = SEGREG[reg];
		dest->type = op_reg;
		break;

	case MR:
		// memory or register operand to register
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, source1 );
		setreg(dest, reg, LONGOPERAND);
		break;

	case IA: 
	{
		// immediate operand to accumulator
		int no_bytes = OPSIZE(data16,WBIT(opcode2));
		switch(no_bytes) 
		{
			case 1: dest->reg = "%al"; break;
			case 2: dest->reg = "%ax"; break;
			case 4: dest->reg = "%eax"; break;
		}
		dest->type = op_reg;
		imm_data(no_bytes, source1);
		source1->type = op_immediate;
		break;
	}
	case MA:
		// memory or register operand to accumulator
		wbit = WBIT(opcode2);
		get_operand(mode, r_m, wbit, source1 );
		setreg(dest, 0, wbit);
		break;

	case SD:
		// si register to di register
		check_override(source1);
		if (addr16)
		{
			source1->reg = "%si";
			dest->reg = "%di";
		}
		else
		{
			source1->reg = "%esi";
			dest->reg = "%edi";
		}
		source1->type = dest->type = op_reg_off;
		break;

	case AD:
		// accumulator to di register
		wbit = WBIT(opcode2);
		check_override(source1);
		setreg(source1, 0, wbit);
		if (addr16)
			dest->reg = "%di";
		else
			dest->reg = "%edi";
		dest->type = op_reg_off;
		break;

	case SA:
		// si register to accumulator
		wbit = WBIT(opcode2);
		check_override(source1);
		setreg(dest, 0, wbit);
		if (addr16)
			source1->reg = "%si";
		else
			source1->reg = "%esi";
		source1->type = op_reg_off;
		break;

	case D:
		// single operand, a 16/32 bit displacement
		// added to current offset
		imm_data(OPSIZE(data16,LONGOPERAND), dest);
		dest->offset = dest->value;
		dest->value += pcaddr + byte_cnt;
		dest->type = op_sym_off;
		break;

	case INM:
		// indirect to memory or register operand
		get_operand(mode, r_m, LONGOPERAND, dest );
		dest->flags |= op_indirect;
		break;

	case SO:
		// for long jumps and long calls -- a new code segment
		// register and an offset in IP -- stored in object
		// code in reverse order
		imm_data(OPSIZE(addr16,LONGOPERAND), dest);
		imm_data(2, source1);
		source1->type = dest->type = op_val;
		break;

	case BD:
		// jmp/call. single operand, 8 bit displacement.
		// added to current offset
		imm_data(1, dest);
		dest->offset = dest->value;
		dest->value += pcaddr + byte_cnt;
		dest->type = op_sym_off;
		break;

	case I:
		// single 32/16 bit immediate operand
		imm_data(OPSIZE(data16,LONGOPERAND), dest);
		dest->type = op_immediate;
		break;

	case Ib:
		// single 8 bit immediate operand
		imm_data(1, dest);
		dest->type = op_immediate;
		break;

	case ENTER:
		imm_data(2, source1);
		imm_data(1, dest);
		dest->type = op_immediate;
		source1->type = op_immediate;
		break;

	case RET:
		// 16-bit immediate operand
		imm_data(2, dest);
		dest->type = op_immediate;
		break;

	case P:
		// single 8 bit port operand
		check_override(dest);
		imm_data(1, dest);
		dest->type = op_immediate;
		break;

	case V:
		// single operand, dx register 
		// (variable port instruction)
		check_override(dest);
		dest->reg = "%dx";
		dest->type = op_reg_off;
		break;

	case INT3:
		// The int instruction, which has two forms: 
		// int 3 (breakpoint) or int n, where n is 
		// indicated in the subsequent byte (format Ib). 
		// The int 3 instruction (opcode 0xCC), where, 
		// although the 3 looks like an operand, it is 
		// implied by the opcode. It must be converted 
		// to the correct base and output.
		dest->value = 0x3;
		dest->type = op_immediate;
		break;

	case U:
		// an unused byte must be discarded
		(void)get_byte();
		break;

	case GO_ON:
		// no disassembly, the mnemonic was all there was
		// to go on
		break;

	// float reg
	case F:
		dest->reg = FLOATREG[r_m];
		dest->type = op_reg;
		break;

	// float reg to float reg, with ret bit present
	case FF:
		dest->type = op_reg;
		source1->type = op_reg;
		if ( opcode2 >> 2 & 0x1 ) 
		{
			/* return result bit for 287 instructions*/
			dest->reg = FLOATREG[r_m];
			source1->reg = "%st";
		}
		else 
		{
			/* st(i) -> st */
			source1->reg = FLOATREG[r_m];
			dest->reg = "%st";
		}
		break;

	// an invalid op code 
	case AM:
	case DM:
	case OVERRIDE:
	case PREFIX:
	case UNKNOWN:
	default:
		inst_size = 0;
		return 0;

	} // end switch

	inst_size = byte_cnt;
	return 1;
}

static	char  	mneu[1028];	
// array to store mnemonic code for output
char *
Instr::deasm(Iaddr pcaddr, int &inst_size, int symbolic, const char *name,
	Iaddr offset)
{
	const instable	*dp;
	char		*current;
	char 		mnemonic[OPLEN];
	Operand		*opnd;
	instr_data	id;
	int		i;

	mnemonic[0] = '\0';
	mneu[0] = '\0';
	current = mneu;
	overreg = 0;
	data16 = addr16 = 0;
	
	if ( get_text_nobkpt(pcaddr) == 0 ) 
	{
		inst_size = 0;
		printe(ERR_get_text, E_ERROR, pcaddr);
		return 0;
	}

	// get table entry for this opcode
	if ((dp = get_instr(&id)) == 0)
	{
		strcpy(mneu, Mtable.format(ERR_bad_opcode));
		inst_size = 0;
		return mneu;
	}

	if (get_ui_type() != ui_gui)
	{
		if ( name != 0 )
		{
			int i = strlen(name);
			// name is max 12 chars; if less than 9, 
			// padd out with spaces
			if (i < 9)
				current += sprintf(current,
					" (%.12s+%d:)%*s\t", name, offset, 9-i, " ");
			else
				current += sprintf(current,
					" (%.12s+%d:)\t", name, offset);
		}
		else
			current += sprintf(current, " (..............)\t");
	}
	else
	{
		*current++ = ':';
	}

	// print the mnemonic
	if (dp->adr_mode == CBW)
	{
		if (data16)
			(void) strcat(mneu,"cbtw");
		else
			(void) strcat(mneu,"cwtl");
		inst_size = byte_cnt;
		return mneu;
	}
	else if (dp->adr_mode == CWD)
	{
		if (data16)
			(void) strcat(mneu,"cwtd");
		else
			(void) strcat(mneu,"cltd");
		inst_size = byte_cnt;
		return mneu;
	}
	else if (dp->adr_mode == JTAB)
	{
		current += sprintf(current,"%s\n",
			"***JUMP TABLE BEGINNING***");
		inst_size = skip_jump_table(pcaddr);
		(void)sprintf(current, 
			"\t%#x (..............)\t***JUMP TABLE END***",
			pcaddr + inst_size - 1);
		return mneu;
	}
	else 
	{
		if (id.prefix)
			strcpy(mnemonic, id.prefix);
		(void)strcat(mnemonic, dp->name);  
		if (dp->suffix)
			(void) strcat(mnemonic, (data16? "w" : "l") );
	}
	current += sprintf(current, " %-7s ", mnemonic);
	if (!get_operands(pcaddr, dp->adr_mode, &id, inst_size))
	{
		strcpy(mneu, Mtable.format(ERR_bad_opcode));
		inst_size = 0;
		return mneu;
	}
	// print operands
	// go through twice - first to print normal values,
	// then to print symbolic values, if requested
	for(i = 0, opnd = &operand[0]; i <= 2; i++, opnd++)
	{
		char	*indir;

		if (opnd->type == op_none)
			continue;
		if (opnd->overreg)
			current += sprintf(current, " %s", 
				opnd->overreg);
		if (opnd->flags & op_indirect)
			indir = "*";
		else
			indir = "";
		switch(opnd->type)
		{
		case op_reg:
			current += sprintf(current, " %s%s",
				indir, opnd->reg);
			break;
		case op_reg_off:
			if (opnd->flags & op_disp)
				current += sprintf(current, " %s%d(%s)",
					indir, opnd->value, opnd->reg);
			else
				current += sprintf(current, " %s(%s)", 
					indir, opnd->reg);
			break;
		case op_scale_index:
			if (opnd->flags & op_disp)
				current += sprintf(current,
					" %s%d(%s,%s,%s)", indir,
					opnd->value, opnd->reg,
					opnd->index, opnd->scale);
			else
				current += sprintf(current,
					" %s(%s,%s,%s)", 
					indir, opnd->reg, opnd->index,
					opnd->scale);
			break;
		case op_val:
			//FALLTHROUGH
		case op_mem:
			current += sprintf(current, " %s0x%x", indir,
				opnd->value);
			break;
		case op_immediate:
			current += sprintf(current, " $0x%x", opnd->value);
			break;
		case op_sym_off:
			current += sprintf(current, " +0x%x <%lx>", 
				opnd->offset, opnd->value);
			break;
		default:
			printe(ERR_internal, E_ERROR, "Instr::deasm",
				__LINE__);
			return 0;
		}
		if (i < 2)
		{
			*current++ = ',';
			*current = 0;
		}
	}
	if (!symbolic)
		return mneu;

	int	first = 1;
	int	found = 0;
	for(i = 0, opnd = &operand[0]; i <= 2; i++, opnd++)
	{
		Symbol		sym;
		const char	*name;
		char		*open;

		switch(opnd->type)
		{
		default:
		case op_none:
		case op_reg:
		case op_val:
		case op_immediate:
			continue;
		case op_reg_off:
			if (!(opnd->flags & op_disp) ||
				(strcmp(opnd->reg, "%ebp") == 0))
					continue;
			break;
		case op_scale_index:
			if (!(opnd->flags & op_disp))
				continue;
			break;
		case op_mem:
		case op_sym_off:
			break;
		}
		sym = pobj->find_symbol(opnd->value);
		if (sym.isnull() || !sym.name())
			continue;
		name = pobj->symbol_name(sym);
		found++;
		if (first)
		{
			first = 0;
			open = "[";
		}
		else
			open = "";
		switch(opnd->type)
		{
		case op_reg_off:
			current += sprintf(current, " %s %s+%s", open, 
				name, opnd->reg);
			break;
		case op_scale_index:
			current += sprintf(current, " %s %s", open,
				name);
			if (strcmp(opnd->reg, "%ebp") != 0)
				current += sprintf(current, "+%s",
					opnd->reg);
			if (opnd->index)
				current += sprintf(current, "+%s*%s",
					opnd->index, opnd->scale);
			break;
		case op_sym_off:
			//FALLTHROUGH
		case op_mem:
			current += sprintf(current, " %s %s", open,
				name);
			break;
		default:
			break;
		}
	}
	if (found)
	{
		strcat(current, " ]");
	}
	return mneu;
}

int
Instr::skip_jump_table(Iaddr pcaddr)
{
	// Special byte indicating the beginning of a
	// jump table has been seen. The jump table addresses
	// will be skipped until the address 0xffff which
	// indicates the end of the jump table is read.	
	int		cnt = byte_cnt;
	Iaddr		addr;
	unsigned char	curbyte;
	unsigned char	tmp;

	addr = pcaddr + (Iaddr) byte_cnt;
	get_text(addr, curbyte);
	cnt++;
	if (curbyte == FILL) 
	{
		addr = addr + (Iaddr) 1;
		get_text(addr, curbyte);
		cnt++;
	}

	tmp = curbyte;
	addr = addr + (Iaddr) 1;
	get_text(addr, curbyte);
	cnt++;
	
	while ((curbyte != 0xff) || (tmp != 0xff)) 
	{
		addr = addr + (Iaddr) 1;
		get_text(addr, curbyte);
		tmp = curbyte;
		addr = addr + (Iaddr) 1;
		get_text(addr, curbyte);
		cnt += 2;
	}
	return cnt;
}

// return size in bytes of instruction at address pc;
// if no valid instruction at that address, return 0
int
Instr::instr_size(Iaddr pc)
{
	int		sz;
	const instable	*dp;
	instr_data	id;

	data16 = addr16 = 0;
	if ( (get_text_nobkpt(pc) == 0) ||
		((dp = get_instr(&id)) == 0))
	{
		return 0;
	}

	if ( dp->adr_mode == CBW  || dp->adr_mode == CWD )
	{
		return byte_cnt;
	}
	else if (dp->adr_mode == JTAB)
	{
		sz = skip_jump_table(pc);
		return sz;
	}

	if (!get_operands(pc, dp->adr_mode, &id, sz))
			return 0;

	return sz;
}

#define UNDEF 		(Iaddr)-1
#define MAXINST		3000
#define MAXBRANCH	64

// Disassemble forward until we find a return instruction,
// adjusting the stack pointer as we go.
int
Instr::find_return(Iaddr pc, Iaddr &spp, Iaddr &ebpp)
{
	const instable	*dp;
	instr_data	id;
	Iaddr		sp, ebp;
	Iaddr		addr, nextaddr;
	Iaddr		pushed_ebp, ebp_addr;
	Iaddr		branch_addr[MAXBRANCH];
	int		branch_tried[MAXBRANCH];
	int		ninst, nbranch;
	int		br, last_try, start_time, last_new_branch;
	int		sz;
	Itype		itype;
	int		destreg;
	int		sourcereg;
	Operand		*source1 = &operand[0];
	Operand		*dest = &operand[2];

	ninst = nbranch = 0;
restart:
	sp = spp;
	ebp = ebpp;
	ebp_addr = UNDEF;
	start_time = last_try = last_new_branch = ninst;

	// Disassemble instructions until we find a return instr.
	for (addr = pc; ; addr = nextaddr) 
	{
		if (++ninst > MAXINST) 
		{
			return 0;
		}
		data16 = addr16 = 0;
		if ( (get_text_nobkpt(addr) == 0) ||
			((dp = get_instr(&id)) == 0))
		{
			return 0;
		}

		if ( dp->adr_mode == CBW  || dp->adr_mode == CWD )
		{
			nextaddr = addr + byte_cnt;
			continue;
		}
		else if (dp->adr_mode == JTAB)
		{
			sz = skip_jump_table(addr);
			nextaddr = addr + sz;
			continue;
		}

		if (!get_operands(addr, dp->adr_mode, &id, sz))
			return 0;

		nextaddr = addr + sz;

		if (id.prefix)
			continue;

		if (dest->type == op_reg)
			destreg = dest->reg_index;
		else
			destreg = 0;

		if (source1->type == op_reg)
			sourcereg = source1->reg_index;
		else
			sourcereg = 0;

		switch (dp->action)
		{

		case A_RET:
			goto loop_out;

		case A_LEAVE:
			if ((sp = ebp) != UNDEF) 
			{
				if (pobj->read(sp, Saddr, itype) !=
					sizeof(Iaddr))
				{
					return 0;
				}
				ebp = itype.iaddr;
				sp += 4;
			}
			break;
		case A_PUSH:
			if (sp != UNDEF) 
			{
				if (!data16)
				{
					sp -= 4;
					if (destreg == FR_EBP)
					{
						pushed_ebp = ebp;
						ebp_addr = sp;
					}
				}
				else
					sp -= 2;
			}
			break;
		case A_PUSHA:
			if (sp != UNDEF)
			{
				if (!data16)
					sp -= 32;
				else
					sp -= 16;
			}
			break;
		case A_PUSHF:
			if (sp != UNDEF)
			{
				if (!data16)
					sp -= 4;
				else
					sp -= 2;
			}
			break;
		case A_POP:
			if (!data16)
			{
				if (destreg == FR_EBP)
				{
					if (sp != UNDEF) 
					{
						if (ebp_addr == sp) 
						{
							ebp = pushed_ebp;
							ebp_addr = UNDEF;
						} 
						else if (pobj->read(sp, 
							Saddr, 
							itype)
							!= sizeof(Iaddr))
						{
							printe(ERR_proc_read, 
							E_ERROR,
							pobj->obj_name(),
							sp);
							return 0;
						}
						else
							ebp = itype.iaddr;
					}
					else
						ebp = UNDEF;
				}
				if (sp != UNDEF)
					sp += 4;
			}
			else
			{
				if (destreg == FR_BP)
					ebp = UNDEF;
				if (sp != UNDEF)
					sp += 2;
			}
			break;
		case A_POPA:
			if (!data16)
			{
				if (sp != UNDEF) 
				{
					if (pobj->read(sp, Saddr, itype) !=
						sizeof(Iaddr))
					{
						printe(ERR_proc_read, E_ERROR,
							pobj->obj_name(), sp);
						return 0;
					}
					ebp = itype.iaddr;
					sp += 32;
				} else
					ebp = UNDEF;
			}
			else
			{
				ebp = UNDEF;
				if (sp != UNDEF)
					sp += 16;
			}
			break;
		case A_POPF:
			if (sp != UNDEF)
			{
				if (!data16)
					sp += 4;
				else
					sp += 2;
			}
			break;
		case A_CALL:
			// we normally ignore calls, since they leave the
			// stack alone; the exception is "call 0x0",
			// which you get in PIC code - this pushes a return addr
			// on the stack, but doesn't clean up
			// NOTE: we can't detect the case of
			// a call to a function that returns
			// a struct - in this case the callee
			// modified esp and we are off by 4
			if ((dest->type == op_sym_off) &&
				(dest->value == nextaddr))
			{
				if (sp != UNDEF)
					sp -= 4;
			}
			break;
		case A_JC:
		case A_JMP:
			// Handle branch instructions
			for (br = 0; br < nbranch; br++) 
			{
				if (branch_addr[br] == addr)
					break;
			}
			if (br < nbranch) 
			{
				if (branch_tried[br]) 
				{
					if (branch_tried[br] >= last_try) 
					{
						/* We looped; give up. */
						if (last_try != start_time) 
						{
							goto restart;
						}
						return 0;
					}
					if ((dp->action == A_JC) &&
						(branch_tried[br] >= 
							last_new_branch))
						{

							branch_tried[br] = ninst;
							// don't branch this time
							break;
						}
						else
						{
							branch_tried[br] = ninst;
							// follow jump addr
						}
						
				} 
				else
				{
					// 2nd time at conditional br;
					// this time take branch
					last_try = branch_tried[br] = ninst;
				}
			} 
			else if (nbranch < MAXBRANCH) 
			{
				last_try = last_new_branch = ninst;
				branch_addr[nbranch++] = addr;
				if (dp->action == A_JMP)
				{
					branch_tried[br] = ninst;
					// follow jump addr
				}
				else 
				{
					// 1st time on conditional branch,
					// don't take branch
					branch_tried[br] = 0;
					break;
				}
			}
			// Follow jump address 
			// Only direct jumps
			if (dest->type == op_sym_off)
			{
				nextaddr = dest->value;
				break;
			}
			if (last_try != start_time)
				goto restart;
			return 0;
		default:
			if (!destreg)
			{
				break;
			}
			else if (destreg == FR_BP)
			{
				ebp = UNDEF;
				break;
			}
			else if (destreg == FR_SP)
			{
				sp = UNDEF;
				break;
			}
			else
			{
				// instruction operates on %esp or %ebp

				int		immediate, setting_sp;
				unsigned long	imm_val, new_val;

				if ((setting_sp = (destreg == FR_ESP))
					!= 0)
					new_val = sp;
				else
					new_val = ebp;

				if (source1->type == op_immediate)
				{
					immediate = 1;
					imm_val = source1->value;
				}
				else
				{
					immediate = 0;
				}

				switch(dp->action)
				{
				case A_MOV:
					if (immediate)
						new_val = imm_val;
					else if (sourcereg == FR_ESP)
						new_val = sp;
					else if (sourcereg == FR_EBP)
						new_val = ebp;
					else
						new_val = UNDEF;
					break;
				case A_INC:
					new_val++;
					break;
				case A_DEC:
					new_val--;
					break;
				case A_ADD:
					if (immediate)
						new_val += imm_val;
					else
						new_val = UNDEF;
					break;
				case A_SUB:
					if (immediate)
						new_val -= imm_val;
					else
						new_val = UNDEF;
					break;
				case A_AND:
					if (immediate)
						new_val &= imm_val;
					else
						new_val = UNDEF;
					break;
				default:
					new_val = UNDEF;
					break;
				}
				if (setting_sp)
					sp = new_val;
				else
					ebp = new_val;
			}
			break;
		}
	}
loop_out:
	if (sp == UNDEF) 
	{
		if (last_try != start_time)
			goto restart;
		return 0;
	}
	spp = sp;
	ebpp = ebp;
	return 1;
}

// if the current instr is CALL, return the address of
// the following instr

Iaddr
Instr::retaddr(Iaddr addr)
{
	unsigned char	op;
	unsigned	mode, r_m, reg;

	if (get_text_nobkpt(addr) == 0) 
		return 0;
		
	op = get_byte();
	switch(op)
	{
	default:
		return 0;
	case CALL:
		return addr + 5;
	case LCALL:
		return addr + 7;
	case ICALL:
		// the ICALL opcode is shared by indirect jumps
		// and indirect calls - must distinguish
		get_modrm_byte(&mode, &reg, &r_m);
		if (reg != 2 && reg != 3)
			return 0;
		operand[0].type = op_none;
		overreg = 0;
		data16 = addr16 = 0;
		// use get_operand to calculate size
		get_operand(mode, r_m, LONGOPERAND, &operand[0]);
		return addr + byte_cnt;
	}
	/*NOTREACHED*/
}

// Is last instruction "CALL" to current function?
// We can only handle direct calls and indirect calls
// that go through memory.
// "caller" is return address from current function - 
// the "call" instruction is the previous instr

int
Instr::iscall(Iaddr caller, Iaddr callee)
{	
	int		i = 0;
	unsigned	mode, reg, r_m;

	// go back far enough to handle CALL and ICALL
	if (get_text_nobkpt(caller - 7) == 0) 
		return 0;
	if (byte[2] == CALL)
	{
		// direct call
		Iaddr	dest;
		Iaddr	*addrptr;

		addrptr = (Iaddr*) &byte[3];
		dest = caller + (*addrptr);
		if (dest == callee)
			return 1;
		// not call direct to callee;
		// might be a call through a procedure
		// linkage table entry.  In that case,
		// we have:
		// call callee@PLT
		// PLT:
		//	jmp	*callee@GOT
		// GOT:	
		//	&callee
		// If instruction at destination is a jump, we check
		// whether jump target is our callee
		if (jmp_target(dest) == callee)
			return 1;
	}
	if (byte[0] == ICALL)
		i = 1;
	else if (byte[1] == ICALL) 
		i = 2;
	if (i == 0)
		return 0;

	// indirect call or jump
	byte_cnt = i;
	get_modrm_byte(&mode, &reg, &r_m);
	if (reg != 2 && reg != 3)
		// jump
		return 0;
	operand[0].type = op_none;
	overreg = 0;
	data16 = addr16 = 0;
	// use get_operand to determine addressing mode
	get_operand(mode, r_m, LONGOPERAND, &operand[0]);
	if (operand[0].type == op_mem)
	{
		// indirect call through memory
		Itype	itype;
		if (pobj->read((Iaddr)operand[0].value,
			Saddr, itype) != sizeof(Iaddr))
		{
			return 0;
		}
		return (itype.iaddr == callee);
	}
	else
		// indirect through register - can't handle
		// but we don't want to continue looking
		return -1;
}
	

// given a frame return address calculate size of previous
// call instruction
int
Instr::call_size(Iaddr pc)
{
	int		i = 0;
	unsigned	mode, reg, r_m;

	// go back far enough to handle CALL and ICALL
	if (get_text_nobkpt(pc - 7) == 0) 
		return 0;
	if (byte[2] == CALL)
	{
		Iaddr	*addrptr;
		Iaddr	dest;

		addrptr = (Iaddr*) &byte[3];
		dest = pc + (*addrptr);
		if (pobj->in_text(dest))
			return 5;
	}
	if (byte[0] == ICALL)
		i = 1;
	else if (byte[1] == ICALL)
		i = 2;
	if (i == 0)
		return 0;
	byte_cnt = i;
	get_modrm_byte(&mode, &reg, &r_m);
	if (reg != 2 && reg != 3)
		// jump
		return 0;
	operand[0].type = op_none;
	overreg = 0;
	data16 = addr16 = 0;
	// use get_operand to determine size
	get_operand(mode, r_m, LONGOPERAND, &operand[0]);
	return(byte_cnt - (i - 1));
}

//
// is next instruction "return" ?
//
int
Instr::isreturn(Iaddr addr)
{	
	if (get_text_nobkpt(addr) == 0)
		return 0;
	unsigned char op = byte[0];
	return (op == RETURNNEAR
		|| op == RETURNFAR
		|| op == RETURNNEARANDPOP
		|| op == RETURNFARANDPOP);
}

int 
Instr::is_bkpt( Iaddr addr )
{
	unsigned char opcode;

	if (get_text(addr, opcode) == 0) 
	{
		printe(ERR_get_text, E_ERROR, addr);
		return 0;
	}
	return (opcode == 0xCC);
}

// if at a breakpoint, pc points to the next instruction.
// adjust to point to the breakpoint instr itself
// if force is non-zero, do the adjustment whether
// or not current instruction is breakpoint - we might
// have already removed the breakpoint for another thread
// after this one has already stopped at it.

Iaddr
Instr::adjust_pc(int force)
{
	Iaddr		pc;
	Itype		data;
	unsigned char	opcode;
	Iaddr		newpc;

	pc = pobj->getreg(REG_PC);
	newpc = pc - 1;
	if ( force || (get_text(newpc, opcode) && (opcode == 0xCC)) ) 
	{
		data.iaddr = newpc;
	        pobj->writereg(REG_PC, Saddr, data ); 
		return newpc;
	}
	return pc;
}

// get number of arguments
// addr is return address; number of argument bytes is amount
// added to the stack frame when we return from the current function;
// for example:
// 	pushl	%ebx
//	pushl	%eax
// 	call	f
// addr:addl	$8,%esp  / 8 bytes of arguments

int 
Instr::nargbytes(Iaddr addr)
{

	if ( ! get_text_nobkpt(addr) ) 
        	return 0;

	if ( (byte[0] == ADDLimm8) &&
		( byte[1] == toESP) ) 
	{
		return 	( byte[2] );
	}
	// popl %ecx might be used to adjust the stack after the call
	else if ( byte[0] == POPLecx )
		return 4;
	else
		return 0;
	
}

// Look for function prolog; returns pc if there is no prolog, otherwise
// the address of the first instruction after the prolog.
// Sets start_addr to the starting address of the prolog
// (may be different from pc if
// there is a jump to the prolog.
// Sets prosize to the size in bytes of the prolog (from the beginning
// addr).  If save_reg is not null, records saved register info.
//
Iaddr
Instr::fcn_prolog(Iaddr pc,  int &prosize, Iaddr &start_addr,
	Iaddr *save_reg)
{
	Iaddr	*addrptr;
	Iaddr	startpc = pc;
	Iaddr	retval = pc;
	int	offset = 0;
	int	jmp_to_prolog = 0;
	int	sz = 0;

	// Note: must be careful here not to run over end of
	// byte array

	if ( ! get_text_nobkpt(pc) ) 
		return pc;

	if (byte[0] == JMPrel8) 
	{
		jmp_to_prolog = 1;
		retval += 2;
		startpc = retval + byte[1];
		if (!get_text_nobkpt(startpc))
			return 0;
	}
	else if (byte[0] == JMPrel32) 
	{
		jmp_to_prolog = 1;
		addrptr = (Iaddr*) &byte[1];
		retval += 5;
		startpc = retval + (*addrptr);
		if (!get_text_nobkpt(startpc))
			return 0;
	}
	// There are 2 forms of function prologs, one
	// used for functions returning structs, the
	// other for all other functions.
	// Must also make sure optimizer is not aligning frame pointer
	if ( (byte[0] == POPLeax) &&
	     (byte[1] == XCHG) )
	{
		int	pos = 0;
		if ((byte[2] == 0x44) &&
			(byte[3] == 0x24) &&
			(byte[4] == 0)) // xchgl 0(%esp), %eax
			pos = 5;
		else if ((byte[2] == 0x04) &&
			(byte[3] == 0x24))
			pos = 4; 	// xchgl (%esp), %eax

		if (pos && (byte[pos] == PUSHLebp) && 
			(byte[pos+1] == MOVLrr)   &&
			(byte[pos+2] == ESPEBP))
		{
			// prolog used for structs returns
			offset = 4;
		        sz = pos+3;
		}
	}
	if (!sz && (byte[0] == PUSHLebp) &&
	     (byte[1] == MOVLrr)   &&
	     (byte[2] == ESPEBP))
		sz = 3;

	if (!sz && !save_reg)
	{
		// no prolog
		return pc;
	}


	// get saved registers. 
	// can be edi, esi, ebx or ebp
	// for each saved register we record its
	// offset from the where the saved frame pointer
	// would be on the stack (if there were a prolog),
	// i.e., the first word below the return address.
	// we allow up to 3 non-push instructions to
	// be insterspersed with the register saves
	// we assume these instructions were not pushes

	int		num_saved = 0;
	Iaddr		nextaddr = startpc + sz;
	Iaddr		addr;
	const instable	*dp;
	instr_data	id;
	int		non_push = 0;
	int		destreg;
	Operand		*source1 = &operand[0];
	Operand		*dest = &operand[2];

	if (!save_reg)
		goto save_done;

	while((num_saved < 4) && (non_push < 3))
	{
		int	inst_sz;
		addr = nextaddr;
		data16 = addr16 = 0;
		if ( (get_text_nobkpt(addr) == 0) ||
			((dp = get_instr(&id)) == 0))
		{
			return pc;
		}
		if ( dp->adr_mode == CBW  || dp->adr_mode == CWD )
		{
			non_push++;
			nextaddr = addr + byte_cnt;
			continue;
		}
		else if (dp->adr_mode == JTAB)
		{
			break;
		}
		if (!get_operands(addr, dp->adr_mode, &id, inst_sz))
			return pc;

		nextaddr = addr + inst_sz;
		if (id.prefix)
		{
			non_push++;
			continue;
		}
		if (dest->type == op_reg)
			destreg = dest->reg_index;
		else
			destreg = 0;
		switch(dp->action)
		{
		case A_PUSH:
			if (!destreg)
				goto save_done;
			switch(destreg)
			{
			default:
				if (num_saved == 0)
				{
					// stack adjust before reg save
		 			offset += 4;
					non_push = 0;
				}
				else 
				{
					// stack modified in
					// middle of register saves
					// don't trust the rest
					goto save_done;
				}
				break;
			case FR_ESI:
				non_push = 0;
				save_reg[REG_ESI] = offset +
					(++num_saved*sizeof(int));
				break;
			case FR_EDI:
				non_push = 0;
				save_reg[REG_EDI] = offset +
					(++num_saved*sizeof(int));
				break;
			case FR_EBX:
				non_push = 0;
				save_reg[REG_EBX] = offset +
					(++num_saved*sizeof(int));
				break;
			case FR_EBP:
				non_push = 0;
				save_reg[REG_EBP] = offset +
					(++num_saved*sizeof(int));
				break;
			}
			break;
		case A_PUSHA:
		case A_PUSHF:
		case A_POP:
		case A_POPA:
		case A_POPF:
		case A_CALL:
		case A_LEAVE:
		case A_RET:
		case A_JC:
		case A_JMP:
			// stack modified or control transferreed in
			// middle of register saves; don't trust 
			// the rest
			goto save_done;
		default:
			if (destreg == FR_EBP || destreg == FR_BP)
			{
				// frame pointer modified - not a real
				// prolog
				sz = 0;
				non_push++;
			}
			else if (destreg == FR_ESP)
			{
				if ((dp->action == A_SUB) &&
					(num_saved == 0) &&
					(source1->type == op_immediate))
				{
					// adjust stack before 
					// register saves
					offset += source1->value;
					non_push = 0;
				}
				else
				{
					// stack modified in
					// middle of register saves
					// don't trust the rest
					goto save_done;
				}

			}
			else if (destreg == FR_SP)
			{
				// stack modified in
				// middle of register saves
				// don't trust the rest
				goto save_done;
			}
			else
			{
				non_push++;
			}
			break;
		}
	}
save_done:
	if (!sz)
	{
		// prosize is used to tell if registers have
		// been saved yet, even if no prolog
		prosize = (int)(nextaddr  - startpc);
		return pc;
	}
	prosize = sz;
	start_addr = startpc;

	if (jmp_to_prolog == 0)
		return (pc + sz);
	//
	// skip NOPs
	//
	int nopcnt = 0;

	if (!get_text_nobkpt(retval)) 
		return pc;
	while ( byte[nopcnt] == NOP ) 
		nopcnt++;
	return (retval + nopcnt);
}

// translate branch table address to the actual function address
//
Iaddr
Instr::brtbl2fcn( Iaddr addr )
{
	Iaddr *addrptr;

	if ( ! get_text_nobkpt(addr) ) 
		return 0;
	if (byte[0] == JMPrel32) 
	{
		addrptr = (Iaddr*) &byte[1];
		return ( addr + (*addrptr) + 5);
	}
	
	return 0;
}

// translate  a function address to the adress of the corresponding
// branch table slot
Iaddr
Instr::fcn2brtbl( Iaddr addr, int offset )
{
	return ((addr  & 0xffff0000 ) + (offset-1) * 5);
}


// If instruction is an unconditional jump, return the
// target address of the jump, else 0.
// This code does not handle all possible unconditional jumps;
// only the ones that the debugger needs to know about (determined by
// experience).

Iaddr
Instr::jmp_target( Iaddr pc)
{
	Dyn_info	*dyn;
	Itype		itype;
	Iaddr		*addrptr;

	if ( ! get_text_nobkpt(pc) ) 	
		return 0;

	if (byte[0] == JMPrel8) 
	{
		// 8-bit relative displacement
		// relative to next instruction
		return(pc + 2 + byte[1]);
	}
	else if (byte[0] == JMPrel32) 
	{
		// 32-bit relative displacement
		// relative to next instruction
		addrptr = (Iaddr*) &byte[1];
		return(pc + 5 + (*addrptr));
	}
	else if (byte[0] != JMPind32)
		return 0;

	if (byte[1] == 0x25)
	{
		// 32-bit indirect - addr in memory
		addrptr = (Iaddr *)&byte[2];
		if (pobj->read(*addrptr, Saddr, itype) != sizeof(Iaddr))
		{
			printe(ERR_proc_read, E_ERROR, pobj->obj_name(),
				*addrptr);
			return 0;
		}
		return(itype.iaddr);
	}
	// register indirect - we only handle the case
	// of jumps from the procedure linkage table
	// through the global offset table
	// instr looks like: jmp *foo@GOT(%ebx)
	if ((byte[1] != 0xa3) || ((dyn = pobj->get_dyn_info(pc)) == 0))
		return 0;

	// make sure jump instr is coming from the plt
	if ((pc < dyn->pltaddr) || 
		(pc >= (dyn->pltaddr + dyn->pltsize)))
		return 0;
	
	// target address is contained in the got entry
	addrptr = (Iaddr *)&byte[2];
	Iaddr	gotent = dyn->gotaddr + *addrptr;
	if (pobj->read(gotent, Saddr, itype) != sizeof(Iaddr))
	{
		printe(ERR_proc_read, E_ERROR, pobj->obj_name(),*addrptr);
		return 0;
	}
	if (!pobj->in_text(itype.iaddr))
	// got entry not yet relocated
		return 0;
	return(itype.iaddr);
}

#if PTRACE
// search for lcall - find address of next instruction;
// if lcall not found before return or within first MAX_SEARCH
// instructions, return 0
#define MAX_SEARCH	100
Iaddr
Instr::find_lreturn(Iaddr pc)
{
	int		cnt = 0;

	for(; cnt < MAX_SEARCH; cnt++)
	{
		int		sz;
		const instable	*dp;
		instr_data	id;

		data16 = addr16 = 0;
		if ( (get_text_nobkpt(pc) == 0) ||
			((dp = get_instr(&id)) == 0))
		{
			break;
		}
		if ( dp->adr_mode == CBW  || dp->adr_mode == CWD )
		{
			sz = byte_cnt;
		}
		else if (dp->adr_mode == JTAB)
		{
			sz = skip_jump_table(pc);
		}
		else if (!get_operands(pc, dp->adr_mode, &id, sz))
			break;
		pc += sz;
		if (id.op == LCALL)
			return pc;
		else if (dp->action == A_RET)
			break;
	}
	return 0;
}
#endif
