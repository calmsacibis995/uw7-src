#ident	"@(#)debugger:libmachine/i386/Tables.c	1.7"

#include	"dis.h"

/* Some opcodes are invalid, some represent several instructions
 * an must be further decoded
 */
#define		INVALID		{"",UNKNOWN,0, A_NONE}
#define		INDIRECT	{"",UNKNOWN,0, A_NONE}

/*
 *	In 16-bit addressing mode:
 *	Register operands may be indicated by a distinguished field.
 *	An '8' bit register is selected if the 'w' bit is equal to 0,
 *	and a '16' bit register is selected if the 'w' bit is equal to
 *	1 and also if there is no 'w' bit.
 */

const char	*REG16[8][2] = {

/* w bit		0		1		*/

/* reg bits */
/* 000	*/		"%al",		"%ax",
/* 001  */		"%cl",		"%cx",
/* 010  */		"%dl",		"%dx",
/* 011	*/		"%bl",		"%bx",
/* 100	*/		"%ah",		"%sp",
/* 101	*/		"%ch",		"%bp",
/* 110	*/		"%dh",		"%si",
/* 111	*/		"%bh",		"%di",
};

/* numeric index of register - for internal use */
const unsigned char	REG16_IND[8][2] = {
/* w bit	0	1		*/

/* reg bits */
/* 000	*/	FR_AL,	FR_AX,
/* 001  */	FR_CL,	FR_CX,
/* 010  */	FR_DL,	FR_DX,
/* 011	*/	FR_BL,	FR_BX,
/* 100	*/	FR_AH,	FR_SP,
/* 101	*/	FR_CH,	FR_BP,
/* 110	*/	FR_DH,	FR_SI,
/* 111	*/	FR_BH,	FR_DI,
};

/*
 *	In 32-bit addressing mode:
 *	Register operands may be indicated by a distinguished field.
 *	An '8' bit register is selected if the 'w' bit is equal to 0,
 *	and a '32' bit register is selected if the 'w' bit is equal to
 *	1 and also if there is no 'w' bit.
 */

const char	*REG32[8][2] = {

/* w bit		0		1		*/

/* reg bits */
/* 000	*/		"%al",		"%eax",
/* 001  */		"%cl",		"%ecx",
/* 010  */		"%dl",		"%edx",
/* 011	*/		"%bl",		"%ebx",
/* 100	*/		"%ah",		"%esp",
/* 101	*/		"%ch",		"%ebp",
/* 110	*/		"%dh",		"%esi",
/* 111	*/		"%bh",		"%edi",

};

/* numeric index of register - for internal use */
const unsigned char	REG32_IND[8][2] = {
/* w bit	0	1		*/

/* reg bits */
/* 000	*/	FR_AL,	FR_EAX,
/* 001  */	FR_CL,	FR_ECX,
/* 010  */	FR_DL,	FR_EDX,
/* 011	*/	FR_BL,	FR_EBX,
/* 100	*/	FR_AH,	FR_ESP,
/* 101	*/	FR_CH,	FR_EBP,
/* 110	*/	FR_DH,	FR_ESI,
/* 111	*/	FR_BH,	FR_EDI,
};

/*
 *	In 16-bit mode:
 *	This initialized array will be indexed by the 'r/m' and 'mod'
 *	fields, to determine the size of the displacement in each mode.
 */

const char dispsize16 [8][4] = {
/* mod		00	01	10	11 */
/* r/m */
/* 000 */	0,	1,	2,	0,
/* 001 */	0,	1,	2,	0,
/* 010 */	0,	1,	2,	0,
/* 011 */	0,	1,	2,	0,
/* 100 */	0,	1,	2,	0,
/* 101 */	0,	1,	2,	0,
/* 110 */	2,	1,	2,	0,
/* 111 */	0,	1,	2,	0
};


/*
 *	In 32-bit mode:
 *	This initialized array will be indexed by the 'r/m' and 'mod'
 *	fields, to determine the size of the displacement in this mode.
 */

const char dispsize32 [8][4] = {
/* mod		00	01	10	11 */
/* r/m */
/* 000 */	0,	1,	4,	0,
/* 001 */	0,	1,	4,	0,
/* 010 */	0,	1,	4,	0,
/* 011 */	0,	1,	4,	0,
/* 100 */	0,	1,	4,	0,
/* 101 */	4,	1,	4,	0,
/* 110 */	0,	1,	4,	0,
/* 111 */	0,	1,	4,	0
};


/*
 *	When data16 has been specified,
 * the following array specifies the registers for the different addressing modes.
 * Indexed first by mode, then by register number.
 */

const char *regname16[4][8] = {
/*reg  000        001        010        011        100    101   110     111 */
/*mod*/
/*00*/ "%bx,%si", "%bx,%di", "%bp,%si", "%bp,%di", "%si", "%di", "",    "%bx",
/*01*/ "%bx,%si", "%bx,%di", "%bp,%si", "%bp,%di", "%si", "%di", "%bp", "%bx",
/*10*/ "%bx,%si", "%bx,%di", "%bp,%si", "%bp,%di", "%si", "%di", "%bp", "%bx",
/*11*/ "%ax",     "%cx",     "%dx",     "%bx",     "%sp", "%bp", "%si", "%di"
};


/*
 *	When data16 has not been specified,
 * 
 *	fields, to determine the addressing mode, and will also provide
 *	strings for printing.
 */

const char *regname32[4][8] = {
/*reg   000       001       010       011       100       101       110       111 */
/*mod*/
/*00 */ "%eax", "%ecx", "%edx", "%ebx", "%esp", "",     "%esi", "%edi",
/*01 */ "%eax", "%ecx", "%edx", "%ebx", "%esp", "%ebp", "%esi", "%edi",
/*10 */ "%eax", "%ecx", "%edx", "%ebx", "%esp", "%ebp", "%esi", "%edi",
/*11 */ "%eax", "%ecx", "%edx", "%ebx", "%esp", "%ebp", "%esi", "%edi"
};

/*
 *	If r/m==100 then the following byte (the s-i-b byte) must be decoded
 */

const char *scale_factor[4] = {
	"1",
	"2",
	"4",
	"8"
};

const char *indexname[8] = {
	"%eax",
	"%ecx",
	"%edx",
	"%ebx",
	"",
	"%ebp",
	"%esi",
	"%edi"
};

/* For communication to locsympr */
char **regname;

/*
 *	Segment registers are selected by a two or three bit field.
 */

const char	*SEGREG[6] = {

/* 000 */	"%es",
/* 001 */	"%cs",
/* 010 */	"%ss",
/* 011 */	"%ds",
/* 100 */	"%fs",
/* 101 */	"%gs",

};

/*
 * Special Registers
 */

const char *DEBUGREG[8] = {
	"%db0", "%db1", "%db2", "%db3", "%db4", "%db5", "%db6", "%db7"
};

const char *CONTROLREG[8] = {
	"%cr0", "%cr1", "%cr2", "%cr3", "%cr4", "%cr5?", "%cr6?", "%cr7?"
};

const char *TESTREG[8] = {
	"%tr0?", "%tr1?", "%tr2?", "%tr3", "%tr4", "%tr5", "%tr6", "%tr7"
};

const char *FLOATREG[8] = {
	"%st(0)", "%st(1)", "%st(2)", "%st(3)", "%st(4)", "%st(5)", 
	"%st(6)", "%st(7)"
};

/*
 *	Decode table for 0x0F00 opcodes
 */

const struct instable op0F00[8] = {

/*  [0]  */	{"sldt",M,0,A_NONE},	{"str",M,0,A_NONE},	{"lldt",M,0,A_NONE},	{"ltr",M,0,A_NONE},
/*  [4]  */	{"verr",M,0,A_NONE},	{"verw",M,0,A_NONE},	INVALID,	INVALID,
};


/*
 *	Decode table for 0x0F01 opcodes
 */

const struct instable op0F01[8] = {

/*  [0]  */	{"sgdt",M,0,A_NONE},	{"sidt",M,0,A_NONE},	{"lgdt",M,0,A_NONE},	{"lidt",M,0,A_NONE},
/*  [4]  */	{"smsw",M,0,A_NONE},	INVALID,		{"lmsw",M,0,A_NONE},	{"invlpg", M, 0 },
};

/*
 *	Decode table for 0x0FC8 opcode -- i486 bswap instruction
 *
 *	bit pattern: 0000 1111 1100 1reg
 */
const struct instable op0FC8[4] = {
/*  [0]  */	{"bswap",R,0,A_NONE},	INVALID,	INVALID,	INVALID,
};

/*
 *	Decode table for 0x0FBA opcodes
 */

const struct instable op0FBA[8] = {

/*  [0]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [4]  */	{"bt",MIb,1,A_NONE},	{"bts",MIb,1,A_NONE},	{"btr",MIb,1,A_NONE},	{"btc",MIb,1,A_NONE},
};


/*
 *	Decode table for 0x0F opcodes
 *	Invalid entries 0x30 - 0x7F are deleted to save space
 */

const struct instable op0F[10][16] = {

/*  [00]  */	{INDIRECT,	INDIRECT,	{"lar",MR,0,A_NONE},	{"lsl",MR,0,A_NONE},
/*  [04]  */	INVALID,		INVALID,		{"clts",GO_ON,0,A_NONE},	INVALID,
/*  [08]  */	{"invd",GO_ON,0,A_NONE},	{"wbinvd",GO_ON,0,A_NONE},	{"UD2",GO_ON,0,A_NONE},			INVALID,
/*  [0C]  */	INVALID,		INVALID,		INVALID,		INVALID},

/*  [10]  */	{INVALID,		INVALID,		INVALID,		INVALID,
/*  [14]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [18]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [1C]  */	INVALID,		INVALID,		INVALID,		INVALID},

/*  [20]  */	{{"mov",SREG,1,A_MOV},	{"mov",SREG,1,A_MOV},	{"mov",SREG,1,A_MOV},	{"mov",SREG,1,A_MOV},
/*  [24]  */	{"mov",SREG,1,A_MOV},	INVALID,		{"mov",SREG,1,A_MOV},	INVALID,
/*  [28]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [2C]  */	INVALID,		INVALID,		INVALID,		INVALID},

/*  [30]  */	{{"wrmsr",GO_ON,0,A_NONE},	{"rdtsc",GO_ON,0,A_NONE},	{"rdmsr",GO_ON,0,A_NONE},	{"rdpmc",GO_ON,0,A_NONE},

/*  [34]  */	{"sysenter",GO_ON,0,A_NONE},	{"sysexit",GO_ON,0,A_NONE},	INVALID,	 INVALID,
/*  [38]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [3C]  */	INVALID,		INVALID,		INVALID,		INVALID},
/*  [40]  */	{{"cmovo",MR,1,A_MOV},	{"cmovno",MR,1,A_MOV},	{"cmovb",MR,1,A_MOV},	{"cmovae",MR,1,A_MOV},
/*  [44]  */	{"cmove",MR,1,A_MOV},	{"cmovne",MR,1,A_MOV},	{"cmovbe",MR,1,A_MOV},	{"cmova",MR,1,A_MOV},
/*  [48]  */	{"cmovs",MR,1,A_MOV},	{"cmovns",MR,1,A_MOV},	{"cmovp",MR,1,A_MOV},	{"cmovpo",MR,1,A_MOV},
/*  [4C]  */	{"cmovl",MR,1,A_MOV},	{"cmovge",MR,1,A_MOV},	{"cmovle",MR,1,A_MOV},	{"cmovg",MR,1,A_MOV}},
/* Invalid entries 0x50 - 0x7F are deleted to save space */
/*  [80]  */	{{"jo",D,0,A_JC},	{"jno",D,0,A_JC},	{"jb",D,0,A_JC},	{"jae",D,0,A_JC},
/*  [84]  */	{"je",D,0,A_JC},	{"jne",D,0,A_JC},	{"jbe",D,0,A_JC},	{"ja",D,0,A_JC},
/*  [88]  */	{"js",D,0,A_JC},	{"jns",D,0,A_JC},	{"jp",D,0,A_JC},	{"jnp",D,0,A_JC},
/*  [8C]  */	{"jl",D,0,A_JC},	{"jge",D,0,A_JC},	{"jle",D,0,A_JC},	{"jg",D,0,A_JC}},

/*  [90]  */	{{"seto",M,0,A_NONE},	{"setno",M,0,A_NONE},	{"setb",M,0,A_NONE},	{"setae",M,0,A_NONE},
/*  [94]  */	{"sete",M,0,A_NONE},	{"setne",M,0,A_NONE},	{"setbe",M,0,A_NONE},	{"seta",M,0,A_NONE},
/*  [98]  */	{"sets",M,0,A_NONE},	{"setns",M,0,A_NONE},	{"setp",M,0,A_NONE},	{"setnp",M,0,A_NONE},
/*  [9C]  */	{"setl",M,0,A_NONE},	{"setge",M,0,A_NONE},	{"setle",M,0,A_NONE},	{"setg",M,0,A_NONE}},

/*  [A0]  */	{{"push",LSEG,1,A_PUSH},	{"pop",LSEG,1,A_POP},	{"cpuid",GO_ON,	0,A_NONE},	{"bt",RMw,1,A_NONE},
/*  [A4]  */	{"shld",DSHIFT,1,A_NONE},	{"shld",DSHIFTcl,1,A_NONE},	INVALID,	INVALID,
/*  [A8]  */	{"push",LSEG,1,A_PUSH},	{"pop",LSEG,1,A_POP},	{"rsm",GO_ON,0,A_NONE},		{"bts",RMw,1,A_NONE},
/*  [AC]  */	{"shrd",DSHIFT,1,A_NONE},	{"shrd",DSHIFTcl,1,A_NONE},	INVALID,		{"imul",MRw,1,A_NONE}},

/*  [B0]  */	{{"cmpxchgb",RMw,0,A_NONE},	{"cmpxchg",RMw,1,A_NONE},	{"lss",MR,0,A_NONE},	{"btr",RMw,1,A_NONE},
/*  [B4]  */	{"lfs",MR,0,A_NONE},	{"lgs",MR,0,A_NONE},	{"movzb",MOVZ,1,A_MOV},	{"movzwl",MOVZ,0,A_MOV},
/*  [B8]  */	INVALID,		INVALID,		INDIRECT,	{"btc",RMw,1,A_NONE},
/*  [BC]  */	{"bsf",MRw,1,A_NONE},	{"bsr",MRw,1,A_NONE},	{"movsb",MOVZ,1,A_MOV},	{"movswl",MOVZ,0,A_MOV}},
/*  [C0]  */	{{"xaddb",RMw,0,A_NONE},	{"xadd",RMw,1,A_NONE},	INVALID, INVALID,
/*  [C4]  */	INVALID,	INVALID,	INVALID,	{"cmpxchg8b",GO_ON,0,A_NONE},
/*  [C8]  */	INVALID,	INVALID,	INVALID,	INVALID,
/*  [CC]  */	INVALID,	INVALID,	INVALID,	INVALID, }
};


/*
 *	Decode table for 0x80 opcodes
 */

const struct instable op80[8] = {

/*  [0]  */	{"addb",IMlw,0,A_ADD},	{"orb",IMlw,0,A_NONE},	{"adcb",IMlw,0,A_NONE},	{"sbbb",IMlw,0,A_NONE},
/*  [4]  */	{"andb",IMlw,0,A_AND},	{"subb",IMlw,0,A_SUB},	{"xorb",IMlw,0,A_NONE},	{"cmpb",IMlw,0,A_NONE},
};


/*
 *	Decode table for 0x81 opcodes.
 */

const struct instable op81[8] = {

/*  [0]  */	{"add",IMlw,1,A_ADD},	{"or",IMlw,1,A_NONE},	{"adc",IMlw,1,A_NONE},	{"sbb",IMlw,1,A_NONE},
/*  [4]  */	{"and",IMlw,1,A_AND},	{"sub",IMlw,1,A_SUB},	{"xor",IMlw,1,A_NONE},	{"cmp",IMlw,1,A_NONE},
};


/*
 *	Decode table for 0x82 opcodes.
 */

const struct instable op82[8] = {

/*  [0]  */	{"addb",IMlw,0,A_ADD},	INVALID,		{"adcb",IMlw,0,A_NONE},	{"sbbb",IMlw,0,A_NONE},
/*  [4]  */	INVALID,		{"subb",IMlw,0,A_SUB},	INVALID,		{"cmpb",IMlw,0,A_NONE},
};
/*
 *	Decode table for 0x83 opcodes.
 */

const struct instable op83[8] = {

/*  [0]  */	{"add",IMlw,1,A_ADD},	{"or",IMlw,1,A_NONE},	{"adc",IMlw,1,A_NONE},	{"sbb",IMlw,1,A_NONE},
/*  [4]  */	{"and",IMlw,1,A_AND},	{"sub",IMlw,1,A_SUB},	{"xor",IMlw,1,A_NONE},		{"cmp",IMlw,1,A_NONE},
};

/*
 *	Decode table for 0xC0 opcodes.
 */

const struct instable opC0[8] = {

/*  [0]  */	{"rolb",MvI,0,A_NONE},	{"rorb",MvI,0,A_NONE},	{"rclb",MvI,0,A_NONE},	{"rcrb",MvI,0,A_NONE},
/*  [4]  */	{"shlb",MvI,0,A_NONE},	{"shrb",MvI,0,A_NONE},	INVALID,		{"sarb",MvI,0,A_NONE},
};

/*
 *	Decode table for 0xD0 opcodes.
 */

const struct instable opD0[8] = {

/*  [0]  */	{"rolb",Mv,0,A_NONE},	{"rorb",Mv,0,A_NONE},	{"rclb",Mv,0,A_NONE},	{"rcrb",Mv,0,A_NONE},
/*  [4]  */	{"shlb",Mv,0,A_NONE},	{"shrb",Mv,0,A_NONE},	INVALID,		{"sarb",Mv,0,A_NONE},
};

/*
 *	Decode table for 0xC1 opcodes.
 *	186 instruction set
 */

const struct instable opC1[8] = {

/*  [0]  */	{"rol",MvI,1,A_NONE},	{"ror",MvI,1,A_NONE},	{"rcl",MvI,1,A_NONE},	{"rcr",MvI,1,A_NONE},
/*  [4]  */	{"shl",MvI,1,A_NONE},	{"shr",MvI,1,A_NONE},	INVALID,		{"sar",MvI,1,A_NONE},
};

/*
 *	Decode table for 0xD1 opcodes.
 */

const struct instable opD1[8] = {

/*  [0]  */	{"rol",Mv,1,A_NONE},	{"ror",Mv,1,A_NONE},	{"rcl",Mv,1,A_NONE},	{"rcr",Mv,1,A_NONE},
/*  [4]  */	{"shl",Mv,1,A_NONE},	{"shr",Mv,1,A_NONE},	INVALID,		{"sar",Mv,1,A_NONE},
};


/*
 *	Decode table for 0xD2 opcodes.
 */

const struct instable opD2[8] = {

/*  [0]  */	{"rolb",Mv,0,A_NONE},	{"rorb",Mv,0,A_NONE},	{"rclb",Mv,0,A_NONE},	{"rcrb",Mv,0,A_NONE},
/*  [4]  */	{"shlb",Mv,0,A_NONE},	{"shrb",Mv,0,A_NONE},	INVALID,		{"sarb",Mv,0,A_NONE},
};
/*
 *	Decode table for 0xD3 opcodes.
 */

const struct instable opD3[8] = {

/*  [0]  */	{"rol",Mv,1,A_NONE},	{"ror",Mv,1,A_NONE},	{"rcl",Mv,1,A_NONE},	{"rcr",Mv,1,A_NONE},
/*  [4]  */	{"shl",Mv,1,A_NONE},	{"shr",Mv,1,A_NONE},	INVALID,		{"sar",Mv,1,A_NONE},
};


/*
 *	Decode table for 0xF6 opcodes.
 */

const struct instable opF6[8] = {

/*  [0]  */	{"testb",IMw,0,A_NONE},	INVALID,		{"notb",Mw,0,A_NONE},	{"negb",Mw,0,A_NONE},
/*  [4]  */	{"mulb",MA,0,A_NONE},	{"imulb",MA,0,A_NONE},	{"divb",MA,0,A_NONE},	{"idivb",MA,0,A_NONE},
};


/*
 *	Decode table for 0xF7 opcodes.
 */

const struct instable opF7[8] = {

/*  [0]  */	{"test",IMw,1,A_NONE},	INVALID,	{"not",Mw,1,A_NONE},	{"neg",Mw,1,A_NONE},
/*  [4]  */	{"mul",MA,1,A_NONE},	{"imul",MA,1,A_NONE},	{"div",MA,1,A_NONE},	{"idiv",MA,1,A_NONE},
};


/*
 *	Decode table for 0xFE opcodes.
 */

const struct instable opFE[8] = {

/*  [0]  */	{"incb",Mw,0,A_INC},	{"decb",Mw,0,A_DEC},	INVALID,		INVALID,
/*  [4]  */	INVALID,		INVALID,		INVALID,		INVALID,
};
/*
 *	Decode table for 0xFF opcodes.
 */

const struct instable opFF[8] = {

/*  [0]  */	{"inc",Mw,1,A_INC},	{"dec",Mw,1,A_DEC},	{"call",INM,0,A_CALL}, {"lcall",INM,0,A_NONE},
/*  [4]  */	{"jmp",INM,0,A_JMP},	{"ljmp",INM,0,A_NONE},	{"push",M,1,A_PUSH},	INVALID,
};

/* for 287 instructions, which are a mess to decode */

const struct instable opFP1n2[8][8] = {
/* bit pattern:	1101 1xxx MODxx xR/M */
/*  [0,0] */	{{"fadds",M,0,A_NONE},	{"fmuls",M,0,A_NONE},	{"fcoms",M,0,A_NONE},	{"fcomps",M,0,A_NONE},
/*  [0,4] */	{"fsubs",M,0,A_NONE},	{"fsubrs",M,0,A_NONE},	{"fdivs",M,0,A_NONE},	{"fdivrs",M,0,A_NONE}},
/*  [1,0]  */	{{"flds",M,0,A_NONE},	INVALID,	{"fsts",M,0,A_NONE},	{"fstps",M,0,A_NONE},
/*  [1,4]  */	{"fldenv",M,0,A_NONE},	{"fldcw",M,0,A_NONE},	{"fnstenv",M,0,A_NONE},	{"fnstcw",M,0,A_NONE}},
/*  [2,0]  */	{{"fiaddl",M,0,A_NONE},	{"fimull",M,0,A_NONE},	{"ficoml",M,0,A_NONE},	{"ficompl",M,0,A_NONE},
/*  [2,4]  */	{"fisubl",M,0,A_NONE},	{"fisubrl",M,0,A_NONE},	{"fidivl",M,0,A_NONE},	{"fidivrl",M,0,A_NONE}},
/*  [3,0]  */	{{"fildl",M,0,A_NONE},	INVALID,	{"fistl",M,0,A_NONE},	{"fistpl",M,0,A_NONE},
/*  [3,4]  */	INVALID,		{"fldt",M,0,A_NONE},	INVALID,		{"fstpt",M,0,A_NONE}},
/*  [4,0]  */	{{"faddl",M,0,A_NONE},	{"fmull",M,0,A_NONE},	{"fcoml",M,0,A_NONE},	{"fcompl",M,0,A_NONE},
/*  [4,1]  */	{"fsubl",M,0,A_NONE},	{"fsubrl",M,0,A_NONE},	{"fdivl",M,0,A_NONE},	{"fdivrl",M,0,A_NONE}},
/*  [5,0]  */	{{"fldl",M,0,A_NONE},	INVALID,	{"fstl",M,0,A_NONE},	{"fstpl",M,0,A_NONE},
/*  [5,4]  */	{"frstor",M,0,A_NONE},	INVALID,	{"fnsave",M,0,A_NONE},	{"fnstsw",M,0,A_NONE}},
/*  [6,0]  */	{{"fiadd",M,0,A_NONE},	{"fimul",M,0,A_NONE},	{"ficom",M,0,A_NONE},	{"ficomp",M,0,A_NONE},
/*  [6,4]  */	{"fisub",M,0,A_NONE},	{"fisubr",M,0,A_NONE},	{"fidiv",M,0,A_NONE},	{"fidivr",M,0,A_NONE}},
/*  [7,0]  */	{{"fild",M,0,A_NONE},	INVALID,	{"fist",M,0,A_NONE},	{"fistp",M,0,A_NONE},
/*  [7,4]  */	{"fbld",M,0,A_NONE},	{"fildll",M,0,A_NONE},	{"fbstp",M,0,A_NONE},	{"fistpll",M,0,A_NONE}}
};

const struct instable opFP3[8][8] = {
/* bit  pattern:	1101 1xxx 11xx xREG */
/*  [0,0]  */	{{"fadd",FF,0,A_NONE},	{"fmul",FF,0,A_NONE},	{"fcom",F,0,A_NONE},	{"fcomp",F,0,A_NONE},
/*  [0,4]  */	{"fsub",FF,0,A_NONE},	{"fsubr",FF,0,A_NONE},	{"fdiv",FF,0,A_NONE},	{"fdivr",FF,0,A_NONE}},
/*  [1,0]  */	{{"fld",F,0,A_NONE},	{"fxch",F,0,A_NONE},	{"fnop",GO_ON,0,A_NONE},	{"fstp",F,0,A_NONE},
/*  [1,4]  */	INVALID,		INVALID,		INVALID,		INVALID},
/*  [2,0]  */	{{"fcmovb",FF,0,A_NONE},	{"fcmove",FF,0,A_NONE},	{"fcmovbe",FF,0,A_NONE},	{"fcmovu",FF,0,A_NONE},
/*  [2,4]  */	INVALID,{"fucompp",GO_ON,0,A_NONE},	INVALID,	INVALID},
/*  [3,0]  */	{{"fcmovnb",FF,0,A_NONE},{"fcmovne",FF,0,A_NONE},{"fcmovnbe",FF,0,A_NONE},{"fcmovnu",FF,0,A_NONE},
/*  [3,4]  */	INVALID, {"fucomi",F,0,A_NONE},	{"fcomi",F,0,A_NONE},INVALID},
/*  [4,0]  */	{{"fadd",FF,0,A_NONE},	{"fmul",FF,0,A_NONE},	{"fcom",F,0,A_NONE},	{"fcomp",F,0,A_NONE},
/*  [4,4]  */	{"fsub",FF,0,A_NONE},	{"fsubr",FF,0,A_NONE},	{"fdiv",FF,0,A_NONE},	{"fdivr",FF,0,A_NONE}},
/*  [5,0]  */	{{"ffree",F,0,A_NONE},	{"fxch",F,0,A_NONE},	{"fst",F,0,A_NONE},	{"fstp",F,0,A_NONE},
/*  [5,4]  */	{"fucom",F,0,A_NONE},	{"fucomp",F,0,A_NONE},	INVALID,		INVALID},
/*  [6,0]  */	{{"faddp",FF,0,A_NONE},	{"fmulp",FF,0,A_NONE},	{"fcomp",F,0,A_NONE},	{"fcompp",GO_ON,0,A_NONE},
/*  [6,4]  */	{"fsubp",FF,0,A_NONE},	{"fsubrp",FF,0,A_NONE},	{"fdivp",FF,0,A_NONE},	{"fdivrp",FF,0,A_NONE}},
/*  [7,0]  */	{{"ffree",F,0,A_NONE},	{"fxch",F,0,A_NONE},	{"fstp",F,0,A_NONE},	{"fstp",F,0,A_NONE},
/*  [7,4]  */	{"fstsw",M,0,A_NONE}, {"fucompi",F,0,A_NONE},{"fcompi",F,0,A_NONE},	INVALID},
};

const struct instable opFP4[4][8] = {
/* bit pattern:	1101 1001 111x xxxx */
/*  [0,0]  */	{{"fchs",GO_ON,0,A_NONE},	{"fabs",GO_ON,0,A_NONE},	INVALID, INVALID,
/*  [0,4]  */	{"ftst",GO_ON,0,A_NONE},	{"fxam",GO_ON,0,A_NONE},	INVALID, INVALID},
/*  [1,0]  */	{{"fld1",GO_ON,0,A_NONE},	{"fldl2t",GO_ON,0,A_NONE},	{"fldl2e",GO_ON,0,A_NONE},	{"fldpi",GO_ON,0,A_NONE},
/*  [1,4]  */	{"fldlg2",GO_ON,0,A_NONE},	{"fldln2",GO_ON,0,A_NONE},	{"fldz",GO_ON,0,A_NONE}, INVALID},
/*  [2,0]  */	{{"f2xm1",GO_ON,0,A_NONE},	{"fyl2x",GO_ON,0,A_NONE},	{"fptan",GO_ON,0,A_NONE},	{"fpatan", GO_ON, 0,A_NONE},
/*  [2,4]  */	{"fxtract",GO_ON,0,A_NONE}, {"fprem1",GO_ON,0,A_NONE}, {"fdecstp",GO_ON,0,A_NONE},{"fincstp",GO_ON,0,A_NONE}},
/*  [3,0]  */	{{"fprem",GO_ON,0,A_NONE},	{"fyl2xp1",GO_ON,0,A_NONE},{"fsqrt",GO_ON,0,A_NONE}, {"fsincos",GO_ON,0,A_NONE},
/*  [3,4]  */	{"frndint",GO_ON,0,A_NONE},{"fscale",GO_ON,0,A_NONE},{"fsin",GO_ON,0,A_NONE}, {"fcos", GO_ON, 0,A_NONE}}
};

const struct instable opFP5[8] = {
/* bit pattern:	1101 1011 1110 0xxx */
/*  [0]  */	INVALID,		INVALID,		{"fnclex",GO_ON,0,A_NONE},{"fninit",GO_ON,0,A_NONE},
/*  [4]  */	{"fsetpm",GO_ON,0,A_NONE},	INVALID,		INVALID,		INVALID,
};

/*
 *	Main decode table for the op codes.  The first two nibbles
 *	will be used as an index into the table.  If there is a
 *	a need to further decode an instruction, the array to be
 *	referenced is indicated with the other two entries being
 *	empty.
 */

const struct instable distable[16][16] = {

/* [0,0] */	{{"addb",RMw,0,A_ADD},	{"add",RMw,1,A_ADD},	{"addb",MRw,0,A_ADD},	{"add",MRw,1,A_ADD},
/* [0,4] */	{"addb",IA,0,A_ADD},	{"add",IA,1,A_ADD},	{"push",SEG,1,A_PUSH},	{"pop",SEG,1,A_POP},
/* [0,8] */	{"orb",RMw,0,A_NONE},	{"or",RMw,1,A_NONE},	{"orb",MRw,0,A_NONE},	{"or",MRw,1,A_NONE},
/* [0,C] */	{"orb",IA,0,A_NONE},	{"or",IA,1,A_NONE},	{"push",SEG,1,A_PUSH},	INDIRECT},

/* [1,0] */	{{"adcb",RMw,0,A_NONE},	{"adc",RMw,1,A_NONE},	{"adcb",MRw,0,A_NONE},	{"adc",MRw,1,A_NONE},
/* [1,4] */	{"adcb",IA,0,A_NONE},	{"adc",IA,1,A_NONE},	{"push",SEG,1,A_PUSH},	{"pop",SEG,1,A_POP},
/* [1,8] */	{"sbbb",RMw,0,A_NONE},	{"sbb",RMw,1,A_NONE},	{"sbbb",MRw,0,A_NONE},	{"sbb",MRw,1,A_NONE},
/* [1,C] */	{"sbbb",IA,0,A_NONE},	{"sbb",IA,1,A_NONE},	{"push",SEG,1,A_PUSH},	{"pop",SEG,1,A_POP}},

/* [2,0] */	{{"andb",RMw,0,A_AND},	{"and",RMw,1,A_AND},	{"andb",MRw,0,A_AND},	{"and",MRw,1,A_AND},
/* [2,4] */	{"andb",IA,0,A_AND},	{"and",IA,1,A_AND},	{"%es:",OVERRIDE,0,A_NONE},{"daa",GO_ON,0,A_NONE},
/* [2,8] */	{"subb",RMw,0,A_SUB},	{"sub",RMw,1,A_SUB},	{"subb",MRw,0,A_SUB},	{"sub",MRw,1,A_SUB},
/* [2,C] */	{"subb",IA,0,A_SUB},	{"sub",IA,1,A_SUB},	{"%cs:",OVERRIDE,0,A_NONE},{"das",GO_ON,0,A_NONE}},

/* [3,0] */	{{"xorb",RMw,0,A_NONE},	{"xor",RMw,1,A_NONE},	{"xorb",MRw,0,A_NONE},	{"xor",MRw,1,A_NONE},
/* [3,4] */	{"xorb",IA,0,A_NONE},	{"xor",IA,1,A_NONE},	{"%ss:",OVERRIDE,0,A_NONE},{"aaa",GO_ON,0,A_NONE},
/* [3,8] */	{"cmpb",RMw,0,A_NONE},	{"cmp",RMw,1,A_NONE},	{"cmpb",MRw,0,A_NONE},	{"cmp",MRw,1,A_NONE},
/* [3,C] */	{"cmpb",IA,0,A_NONE},	{"cmp",IA,1,A_NONE},	{"%ds:",OVERRIDE,0,A_NONE},{"aas",GO_ON,0,A_NONE}},

/* [4,0] */	{{"inc",R,1,A_INC},	{"inc",R,1,A_INC},	{"inc",R,1,A_INC},	{"inc",R,1,A_INC},
/* [4,4] */	{"inc",R,1,A_INC},	{"inc",R,1,A_INC},	{"inc",R,1,A_INC},	{"inc",R,1,A_INC},
/* [4,8] */	{"dec",R,1,A_DEC},	{"dec",R,1,A_DEC},	{"dec",R,1,A_DEC},	{"dec",R,1,A_DEC},
/* [4,C] */	{"dec",R,1,A_DEC},	{"dec",R,1,A_DEC},	{"dec",R,1,A_DEC},	{"dec",R,1,A_DEC}},

/* [5,0] */	{{"push",R,1,A_PUSH},	{"push",R,1,A_PUSH},	{"push",R,1,A_PUSH},	{"push",R,1,A_PUSH},
/* [5,4] */	{"push",R,1,A_PUSH},	{"push",R,1,A_PUSH},	{"push",R,1,A_PUSH},	{"push",R,1,A_PUSH},
/* [5,8] */	{"pop",R,1,A_POP},	{"pop",R,1,A_POP},	{"pop",R,1,A_POP},	{"pop",R,1,A_POP},
/* [5,C] */	{"pop",R,1,A_POP},	{"pop",R,1,A_POP},	{"pop",R,1,A_POP},	{"pop",R,1,A_POP}},

/* [6,0] */	{{"pusha",GO_ON,1,A_PUSHA},	{"popa",GO_ON,1,A_POPA},	{"bound",MR,1,A_NONE},	{"arpl",RMw,0,A_NONE},
/* [6,4] */	{"%fs:",OVERRIDE,0,A_NONE},{"%gs:",OVERRIDE,0,A_NONE},{"data16",DM,0,A_NONE},	{"addr16",AM,0,A_NONE},
/* [6,8] */	{"push",I,1,A_PUSH},	{"imul",IMUL,1,A_NONE},	{"push",Ib,1,A_PUSH},	{"imul",IMUL,1,A_NONE},
/* [6,C] */	{"insb",GO_ON,0,A_NONE},	{"ins",GO_ON,1,A_NONE},	{"outsb",GO_ON,0,A_NONE},	{"outs",GO_ON,1,A_NONE}},

/* [7,0] */	{{"jo",BD,0,A_JC},	{"jno",BD,0,A_JC},	{"jb",BD,0,A_JC},	{"jae",BD,0,A_JC},
/* [7,4] */	{"je",BD,0,A_JC},	{"jne",BD,0,A_JC},	{"jbe",BD,0,A_JC},	{"ja",BD,0,A_JC},
/* [7,8] */	{"js",BD,0,A_JC},	{"jns",BD,0,A_JC},	{"jp",BD,0,A_JC},	{"jnp",BD,0,A_JC},
/* [7,C] */	{"jl",BD,0,A_JC},	{"jge",BD,0,A_JC},	{"jle",BD,0,A_JC},	{"jg",BD,0,A_JC}},

/* [8,0] */	{INDIRECT,	INDIRECT,	INDIRECT,	INDIRECT,
/* [8,4] */	{"testb",MRw,0,A_NONE},	{"test",MRw,1,A_NONE},	{"xchgb",MRw,0,A_NONE},	{"xchg",MRw,1,A_NONE},
/* [8,8] */	{"movb",RMw,0,A_MOV},	{"mov",RMw,1,A_MOV},	{"movb",MRw,0,A_MOV},	{"mov",MRw,1,A_MOV},
/* [8,C] */	{"mov",SM,1,A_MOV},	{"lea",MR,1,A_NONE},	{"mov",MS,1,A_MOV},	{"pop",M,1,A_POP}},

/* [9,0] */	{{"nop",GO_ON,0,A_NONE},	{"xchg",RA,1,A_NONE},	{"xchg",RA,1,A_NONE},	{"xchg",RA,1,A_NONE},
/* [9,4] */	{"xchg",RA,1,A_NONE},	{"xchg",RA,1,A_NONE},	{"xchg",RA,1,A_NONE},	{"xchg",RA,1,A_NONE},
/* [9,8] */	{"",CBW,0,A_NONE},	{"",CWD,0,A_NONE},	{"lcall",SO,0,A_NONE},	{"fwait",GO_ON,0,A_NONE},
/* [9,C] */	{"pushf",GO_ON,1,A_PUSHF},	{"popf",GO_ON,1,A_POPF},	{"sahf",GO_ON,0,A_NONE},	{"lahf",GO_ON,0,A_NONE}},

/* [A,0] */	{{"movb",OA,0,A_MOV},	{"mov",OA,1,A_MOV},	{"movb",AO,0,A_MOV},	{"mov",AO,1,A_MOV},
/* [A,4] */	{"movsb",SD,0,A_MOV},	{"movs",SD,1,A_MOV},	{"cmpsb",SD,0,A_NONE},	{"cmps",SD,1,A_NONE},
/* [A,8] */	{"testb",IA,0,A_NONE},	{"test",IA,1,A_NONE},	{"stosb",AD,0,A_NONE},	{"stos",AD,1,A_NONE},
/* [A,C] */	{"lodsb",SA,0,A_NONE},	{"lods",SA,1,A_NONE},	{"scasb",AD,0,A_NONE},	{"scas",AD,1,A_NONE}},

/* [B,0] */	{{"movb",IR,0,A_MOV},	{"movb",IR,0,A_MOV},	{"movb",IR,0,A_MOV},	{"movb",IR,0,A_MOV},
/* [B,4] */	{"movb",IR,0,A_MOV},	{"movb",IR,0,A_MOV},	{"movb",IR,0,A_MOV},	{"movb",IR,0,A_MOV},
/* [B,8] */	{"mov",IR,1,A_MOV},	{"mov",IR,1,A_MOV},	{"mov",IR,1,A_MOV},	{"mov",IR,1,A_MOV},
/* [B,C] */	{"mov",IR,1,A_MOV},	{"mov",IR,1,A_MOV},	{"mov",IR,1,A_MOV},	{"mov",IR,1,A_MOV}},

/* [C,0] */	{INDIRECT,	INDIRECT,	{"ret",RET,0,A_RET},	{"ret",GO_ON,0,A_RET},
/* [C,4] */	{"les",MR,0,A_NONE},	{"lds",MR,0,A_NONE},	{"movb",IMw,0,A_MOV},	{"mov",IMw,1,A_MOV},
/* [C,8] */	{"enter",ENTER,0,A_NONE},	{"leave",GO_ON,0,A_LEAVE},	{"lret",RET,0,A_NONE},	{"lret",GO_ON,0,A_NONE},
/* [C,C] */	{"int",INT3,0,A_NONE},	{"int",Ib,0,A_NONE},	{"into",GO_ON,0,A_NONE},	{"iret",GO_ON,0,A_NONE}},

/* [D,0] */	{INDIRECT,	INDIRECT,	INDIRECT,	INDIRECT,
/* [D,4] */	{"aam",U,0,A_NONE},	{"aad",U,0,A_NONE},	{"falc",GO_ON,0,A_NONE},	{"xlat",GO_ON,0,A_NONE},

/* 287 instructions.  */
/* [D,8] */	INDIRECT,	INDIRECT,	INDIRECT,	INDIRECT,
/* [D,C] */	INDIRECT,	INDIRECT,	INDIRECT,	INDIRECT},

/* [E,0] */	{{"loopnz",BD,0,A_JC},	{"loopz",BD,0,A_JC},	{"loop",BD,0,A_JC},	{"jcxz",BD,0,A_JC},
/* [E,4] */	{"inb",P,0,A_NONE},	{"in",P,1,A_NONE},	{"outb",P,0,A_NONE},	{"out",P,1,A_NONE},
/* [E,8] */	{"call",D,0,A_CALL},	{"jmp",D,0,A_JMP},	{"ljmp",SO,0,A_NONE},	{"jmp",BD,0,A_JMP},
/* [E,C] */	{"inb",V,0,A_NONE},	{"in",V,1,A_NONE},	{"outb",V,0,A_NONE},	{"out",V,1,A_NONE}},

/* [F,0] */	{{"lock ",PREFIX,0,A_NONE},	{"",JTAB,0,A_NONE},	{"repnz ",PREFIX,0,A_NONE},	{"repz ",PREFIX,0,A_NONE},
/* [F,4] */	{"hlt",GO_ON,0,A_NONE},	{"cmc",GO_ON,0,A_NONE},	INDIRECT,	INDIRECT,
/* [F,8] */	{"clc",GO_ON,0,A_NONE},	{"stc",GO_ON,0,A_NONE},	{"cli",GO_ON,0,A_NONE},	{"sti",GO_ON,0,A_NONE},
/* [F,C] */	{"cld",GO_ON,0,A_NONE},	{"std",GO_ON,0,A_NONE},	INDIRECT,	INDIRECT}
};
