#ident	"@(#)dis:i386/tables.c	1.11"

#include	"dis.h"

#define		INVALID	{"",TERM,0,UNKNOWN,0}

/*
 *	In 16-bit addressing mode:
 *	Register operands may be indicated by a distinguished field.
 *	An '8' bit register is selected if the 'w' bit is equal to 0,
 *	and a '16' bit register is selected if the 'w' bit is equal to
 *	1 and also if there is no 'w' bit.
 */

const char	*REG16[16] = {

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

/*
 *	In 32-bit addressing mode:
 *	Register operands may be indicated by a distinguished field.
 *	An '8' bit register is selected if the 'w' bit is equal to 0,
 *	and a '32' bit register is selected if the 'w' bit is equal to
 *	1 and also if there is no 'w' bit.
 */

const char	*REG32[16] = {

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
	",%eax",
	",%ecx",
	",%edx",
	",%ebx",
	"",
	",%ebp",
	",%esi",
	",%edi"
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

/*
 *	Decode table for 0x0F00 opcodes
 */

struct instable op0F00[8] = {

/*  [0]  */	{"sldt",TERM,{307,0,WOP0|RSYSTATE},M,0},	{"str",TERM,{314,0,WOP0|RSYSTATE},M,0},	{"lldt",TERM,{223,0,ROP0|WSYSTATE},M,0},	{"ltr",TERM,{234,0,ROP0|WSYSTATE},M,0},
/*  [4]  */	{"verr",TERM,{321,WZF,ROP0|RSYSTATE},M,0},	{"verw",TERM,{322,WZF,ROP0|RSYSTATE},M,0},	INVALID,		INVALID,
};


/*
 *	Decode table for 0x0F01 opcodes
 */

struct instable op0F01[8] = {

/*  [0]  */	{"sgdt",TERM,{299,0,WOP0|RSYSTATE},M,0},	{"sidt",TERM,{306,0,WOP0|RSYSTATE},M,0},	{"lgdt",TERM,{219,0,ROP0|WSYSTATE},M,0},	{"lidt",TERM,{221,0,ROP0|WSYSTATE},M,0},
/*  [4]  */	{"smsw",TERM,{308,0,WOP0|RSYSTATE},M,0},	INVALID,		{"lmsw",TERM,{224,0,ROP0|WSYSTATE},M,0},	{"invlpg",TERM,{191,0,ROP0|WSYSTATE},M,0},
};


/*
 *	Decode table for 0x0FBA opcodes
 */

struct instable op0FBA[8] = {

/*  [0]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [4]  */	{"bt",TERM,{16,WCF,ROP0|ROP1},MIb,1},	{"bts",TERM,{19,WCF,RWOP0|ROP1},MIb,1},	{"btr",TERM,{18,WCF,RWOP0|ROP1},MIb,1},	{"btc",TERM,{17,WCF,RWOP0|ROP1},MIb,1},
};

/*
 *	Decode table for 0x0FC8 opcode -- i486 bswap instruction
*
 *bit pattern: 0000 1111 1100 1reg
 */
struct instable op0FC8[4] = {
/*  [0]  */	{"bswap",TERM,{15,0,RWREG},R,0},		INVALID,		INVALID,		INVALID,
};

/*
 *	Decode table for 0x0F opcodes
 */

struct instable op0F[208] = {

/*  [00]  */	{"",op0F00,{NO_OPGROUP,0,0},TERM,0},	{"",op0F01,{NO_OPGROUP,0,0},TERM,0},	{"lar",TERM,{212,WZF,ROP0|WREG},MR,0},	{"lsl",TERM,{232,WZF,ROP0|WREG},MR,0},
/*  [04]  */	INVALID,		INVALID,		{"clts",TERM,{24,0,WSYSTATE},GO_ON,0},	INVALID,
/*  [08]  */	{"invd",TERM,{190,0,WSYSTATE},GO_ON,0},	{"wbinvd",TERM,{323,0,WSYSTATE},GO_ON,0}, INVALID, {"UD2",TERM,{0,0,0},GO_ON,0},
/*  [0C]  */	INVALID,		INVALID,		INVALID,		INVALID,

/*  [10]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [14]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [18]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [1C]  */	INVALID,		INVALID,		INVALID,		INVALID,

/*  [20]  */	{"mov",TERM,{343,UALLF,WOP0|RREG|RWSYSTATE},SREG,1},	{"mov",TERM,{343,UALLF,WOP0|RREG|RWSYSTATE},SREG,1},	{"mov",TERM,{343,UALLF,WOP0|RREG|RWSYSTATE},SREG,1},	{"mov",TERM,{343,UALLF,WOP0|RREG|RWSYSTATE},SREG,1},
/*  [24]  */	{"mov",TERM,{343,UALLF,WOP0|RREG|RWSYSTATE},SREG,1},	INVALID,		{"mov",TERM,{343,UALLF,WOP0|RREG|RWSYSTATE},SREG,1},	INVALID,
/*  [28]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [2C]  */	INVALID,		INVALID,		INVALID,		INVALID,

/*  [30]  */	{"wrmsr",TERM,{324,0,WSYSTATE|RREG|ROP0|ROP1},GO_ON,0},	{"rdtsc",TERM,{267,0,WOP1|WREG|RSYSTATE},GO_ON,0},	{"rdmsr",TERM,{265,0,ROP0|WOP1|WREG|RSYSTATE},GO_ON,0},	{"rdpmc",TERM,{266,0,ROP0|WOP1|WREG|RSYSTATE},GO_ON,0},
/*  [34]  */	{"sysenter",TERM,{317,0,DANGEROUS},GO_ON,0},	{"sysexit",TERM,{318,0,DANGEROUS},GO_ON,0},		INVALID,		INVALID,
/*  [38]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [3C]  */	INVALID,		INVALID,		INVALID,		INVALID,

/*  [40]  */	{"cmovo",TERM,{38,ROF,ROP0|WREG},MR,1},	{"cmovno",TERM,{36,ROF,ROP0|WREG},MR,1},	{"cmovb",TERM,{28,RCF,ROP0|RWREG},MR,1},	{"cmovae",TERM,{27,RCF,ROP0|WREG},MR,1},
/*  [44]  */	{"cmove",TERM,{30,RZF,ROP0|WREG},MR,1},	{"cmovne",TERM,{35,RZF,ROP0|WREG},MR,1},	{"cmovbe",TERM,{29,RCF|RZF,ROP0|RWREG},MR,1},	{"cmova",TERM,{26,RCF|RZF,ROP0|WREG},MR,1},
/*  [48]  */	{"cmovs",TERM,{41,RSF,ROP0|WREG},MR,1},	{"cmovns",TERM,{37,RSF,ROP0|WREG},MR,1},	{"cmovp",TERM,{39,RPF,ROP0|WREG},MR,1},	{"cmovpo",TERM,{40,RPF,ROP0|WREG},MR,1},
/*  [4C]  */	{"cmovl",TERM,{33,RSF|ROF,ROP0|WREG},MR,1},	{"cmovge",TERM,{32,RSF|ROF,ROP0|WREG},MR,1},	{"cmovle",TERM,{34,RZF|RSF|ROF,ROP0|WREG},MR,1},	{"cmovg",TERM,{31,RZF|RSF|ROF,ROP0|WREG},MR,1},

/*  [50]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [54]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [58]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [5C]  */	INVALID,		INVALID,		INVALID,		INVALID,

/*  [60]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [64]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [68]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [6C]  */	INVALID,		INVALID,		INVALID,		INVALID,

/*  [70]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [74]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [78]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [7C]  */	INVALID,		INVALID,		INVALID,		INVALID,

/*  [80]  */	{"jo",TERM,{208,ROF,ROP0},D,0},	{"jno",TERM,{205,ROF,ROP0},D,0},	{"jb",TERM,{195,RCF,ROP0},D,0},	{"jae",TERM,{194,RCF,ROP0},D,0},
/*  [84]  */	{"je",TERM,{198,RZF,ROP0},D,0},	{"jne",TERM,{204,RZF,ROP0},D,0},	{"jbe",TERM,{196,RCF|RZF,ROP0},D,0},	{"ja",TERM,{193,RCF|RZF,ROP0},D,0},
/*  [88]  */	{"js",TERM,{210,RSF,ROP0},D,0},	{"jns",TERM,{207,RSF,ROP0},D,0},	{"jp",TERM,{209,RPF,ROP0},D,0},	{"jnp",TERM,{206,RPF,ROP0},D,0},
/*  [8C]  */	{"jl",TERM,{201,RSF|ROF,ROP0},D,0},	{"jge",TERM,{200,RSF|ROF,ROP0},D,0},	{"jle",TERM,{202,RZF|RSF|ROF,ROP0},D,0},	{"jg",TERM,{199,RZF|RSF|ROF,ROP0},D,0},

/*  [90]  */	{"seto",TERM,{296,ROF,WOP0},M,0},	{"setno",TERM,{293,ROF,WOP0},M,0},	{"setb",TERM,{285,RCF,WOP0},M,0},	{"setae",TERM,{284,RCF,WOP0},M,0},
/*  [94]  */	{"sete",TERM,{287,RZF,WOP0},M,0},	{"setne",TERM,{292,RZF,WOP0},M,0},	{"setbe",TERM,{286,RCF|RZF,WOP0},M,0},	{"seta",TERM,{283,RCF|RZF,WOP0},M,0},
/*  [98]  */	{"sets",TERM,{298,RSF,WOP0},M,0},	{"setns",TERM,{295,RSF,WOP0},M,0},	{"setp",TERM,{297,RPF,WOP0},M,0},	{"setnp",TERM,{294,RPF,WOP0},M,0},
/*  [9C]  */	{"setl",TERM,{290,RSF|ROF,WOP0},M,0},	{"setge",TERM,{289,RSF|ROF,WOP0},M,0},	{"setle",TERM,{291,RZF|RSF|ROF,WOP0},M,0},	{"setg",TERM,{288,RCF|RSF|ROF,WOP0},M,0},

/*  [A0]  */	{"push",TERM,{340,0,OPT_PUSH|RREG},LSEG,1},	{"pop",TERM,{345,0,OPT_POP|WREG},LSEG,1},	{"cpuid",TERM,{49,0,WREG|RSYSTATE},GO_ON,0},	{"bt",TERM,{16,WCF,ROP0|RREG},RMw,1},
/*  [A4]  */	{"shld",TERM,{302,RWALLF,RWOP1|RWREG},DSHIFT,1},	{"shld",TERM,{302,RWALLF,RWOP0|ROP1|RWREG},DSHIFTcl,1},	INVALID,	INVALID,
/*  [A8]  */	{"push",TERM,{340,0,OPT_PUSH|RREG},LSEG,1},	{"pop",TERM,{345,0,OPT_POP|WREG},LSEG,1},	{"rsm",TERM,{275,UALLF,0},GO_ON,0},	{"bts",TERM,{19,WCF,RREG|RWOP0},RMw,1},
/*  [AC]  */	{"shrd",TERM,{305,RWALLF,RWOP1|RWREG},DSHIFT,1},	{"shrd",TERM,{305,RWALLF,RWOP0|ROP1|RWREG},DSHIFTcl,1},	INVALID,	{"imul",TERM,{180,RWOF|RWCF|UALLF,ROP0|RWREG},MRw,1},

/*  [B0]  */	{"cmpxchgb",TERM,{48,CANT_HANDLE_YET,CANT_HANDLE_YET},RMw,0},{"cmpxchg",TERM,{46,CANT_HANDLE_YET,CANT_HANDLE_YET},RMw,1},	{"lss",TERM,{233,0,ROP0|WREG},MR,0},	{"btr",TERM,{18,WCF,RREG|RWOP0},RMw,1},
/*  [B4]  */	{"lfs",TERM,{218,0,ROP0|WREG},MR,0},	{"lgs",TERM,{220,0,ROP0|WREG},MR,0},	{"movzb",TERM,{240,0,ROP0|WREG},MOVZ,1},	{"movzwl",TERM,{241,0,ROP0|WREG},MOVZ,0},
/*  [B8]  */	INVALID,		INVALID,		{"",op0FBA,{NO_OPGROUP,0,0},TERM,0},	{"btc",TERM,{17,WCF,RREG|RWOP0},RMw,1},
/*  [BC]  */	{"bsf",TERM,{13,WZF,ROP0|WREG},MRw,1},	{"bsr",TERM,{14,WZF,ROP0|WREG},MRw,1},	{"movsb",TERM,{238,0,ROP0|WREG},MOVZ,1},	{"movswl",TERM,{239,0,ROP0|WREG},MOVZ,0},
/*  [C0]  */	{"xaddb",TERM,{326,WALLF,RWOP0|RWREG},RMw,0},	{"xadd",TERM,{325,WALLF,RWOP0|RWREG},RMw,1},	INVALID,		INVALID,
/*  [C4]  */	INVALID,		INVALID,		INVALID,		{"cmpxchg8b",TERM,{47,CANT_HANDLE_YET,CANT_HANDLE_YET},M,0},
/*  [C8]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [CC]  */	INVALID,		INVALID,		INVALID,		INVALID};


/*
 *	Decode table for 0x80 opcodes
 */

struct instable op80[8] = {

/*  [0]  */	{"addb",TERM,{7,WALLF,ROP0|RWOP1|OPT_ADD},IM8w,0},	{"orb",TERM,{250,WNOTAF,ROP0|RWOP1},IMw,0},	{"adcb",TERM,{5,RCF|WALLF,ROP0|RWOP1},IM8w,0},	{"sbbb",TERM,{280,RCF|WALLF,ROP0|RWOP1},IM8w,0},
/*  [4]  */	{"andb",TERM,{10,WNOTAF,ROP0|RWOP1|OPT_AND},IMw,0},	{"subb",TERM,{316,WALLF,ROP0|RWOP1|OPT_SUB},IM8w,0},	{"xorb",TERM,{331,WNOTAF,ROP0|RWOP1},IMw,0},	{"cmpb",TERM,{43,WALLF,ROP0|ROP1},IM8w,0},
};


/*
 *	Decode table for 0x81 opcodes.
 */

struct instable op81[8] = {

/*  [0]  */	{"add",TERM,{6,WALLF,ROP0|RWOP1|OPT_ADD},IMlw,1},	{"or",TERM,{249,WNOTAF,ROP0|RWOP1},IMw,1},	{"adc",TERM,{4,RCF|WALLF,ROP0|RWOP1},IMlw,1},	{"sbb",TERM,{279,RCF|WALLF,ROP0|RWOP1},IMlw,1},
/*  [4]  */	{"and",TERM,{9,WNOTAF,ROP0|RWOP1|OPT_AND},IMw,1},	{"sub",TERM,{315,WALLF,ROP0|RWOP1|OPT_SUB},IMlw,1},	{"xor",TERM,{330,WNOTAF,ROP0|RWOP1},IMw,1},	{"cmp",TERM,{42,WALLF,ROP0|ROP1},IMlw,1},
};


/*
 *	Decode table for 0x82 opcodes.
 */

struct instable op82[8] = {

/*  [0]  */	{"addb",TERM,{7,WALLF,ROP0|RWOP1|OPT_ADD},IM8w,0},	{"orb",TERM,{250,WNOTAF,ROP0|RWOP1},IM8w,0},	{"adcb",TERM,{5,RCF|WALLF,ROP0|RWOP1},IM8w,0},	{"sbbb",TERM,{280,RCF|WALLF,ROP0|RWOP1},IM8w,0},
/*  [4]  */	{"andb",TERM,{10,WNOTAF,ROP0|RWOP1|OPT_AND},IM8w,0},	{"subb",TERM,{316,WALLF,ROP0|RWOP1|OPT_SUB},IM8w,0},	{"xorb",TERM,{331,WNOTAF,ROP0|RWOP1},IM8w,0},	{"cmpb",TERM,{43,WALLF,ROP0|ROP1},IM8w,0},
};
/*
 *	Decode table for 0x83 opcodes.
 */

struct instable op83[8] = {

/*  [0]  */	{"add",TERM,{6,WALLF,ROP0|RWOP1|OPT_ADD},IM8w,1},	{"or",TERM,{249,WNOTAF,ROP0|RWOP1},IM8w,1},	{"adc",TERM,{4,RCF|WALLF,ROP0|RWOP1},IM8w,1},	{"sbb",TERM,{279,RCF|WALLF,ROP0|RWOP1},IM8w,1},
/*  [4]  */	{"and",TERM,{9,WNOTAF,ROP0|RWOP1|OPT_AND},IM8w,1},	{"sub",TERM,{315,WALLF,ROP0|RWOP1|OPT_SUB},IM8w,1},	{"xor",TERM,{330,WNOTAF,ROP0|RWOP1},IM8w,1},	{"cmp",TERM,{42,WALLF,ROP0|ROP1},IM8w,1},
};

/*
 *	Decode table for 0xC0 opcodes.
 */

struct instable opC0[8] = {

/*  [0]  */	{"rolb",TERM,{272,RWOF|RWCF,RWOP0|ROP1},MvI,0},	{"rorb",TERM,{274,RWOF|RWCF,RWOP0|ROP1},MvI,0},	{"rclb",TERM,{262,RWOF|RWCF,RWOP0|ROP1},MvI,0},	{"rcrb",TERM,{264,RWOF|RWCF,RWOP0|ROP1},MvI,0},
/*  [4]  */	{"shlb",TERM,{301,ROF|WNOTAF,RWOP0|ROP1},MvI,0},	{"shrb",TERM,{304,ROF|WNOTAF,RWOP0|ROP1},MvI,0},	INVALID,		{"sarb",TERM,{278,ROF|WNOTAF,RWOP0|ROP1},MvI,0},
};

/*
 *	Decode table for 0xD0 opcodes.
 */

struct instable opD0[8] = {

/*  [0]  */	{"rolb",TERM,{272,RWOF|RWCF,RWOP0|RREG},Mv,0},	{"rorb",TERM,{274,RWOF|RWCF,RWOP0|RREG},Mv,0},	{"rclb",TERM,{262,RWOF|RWCF,RWOP0|RREG},Mv,0},	{"rcrb",TERM,{264,RWOF|RWCF,RWOP0|RREG},Mv,0},
/*  [4]  */	{"shlb",TERM,{301,ROF|WNOTAF,RWOP0|RREG},Mv,0},	{"shrb",TERM,{304,ROF|WNOTAF,RWOP0|RREG},Mv,0},	INVALID,		{"sarb",TERM,{278,ROF|WNOTAF,RWOP0|RREG},Mv,0},
};

/*
 *	Decode table for 0xC1 opcodes.
 *	186 instruction set
 */

struct instable opC1[8] = {

/*  [0]  */	{"rol",TERM,{271,RWOF|RWCF,RWOP0|ROP1},MvI,1},	{"ror",TERM,{273,RWOF|RWCF,RWOP0|ROP1},MvI,1},	{"rcl",TERM,{261,RWOF|RWCF,RWOP0|ROP1},MvI,1},	{"rcr",TERM,{263,RWOF|RWCF,RWOP0|ROP1},MvI,1},
/*  [4]  */	{"shl",TERM,{300,ROF|WNOTAF,RWOP0|ROP1},MvI,1},	{"shr",TERM,{303,ROF|WNOTAF,RWOP0|ROP1},MvI,1},	INVALID,		{"sar",TERM,{277,ROF|WNOTAF,RWOP0|ROP1},MvI,1},
};

/*
 *	Decode table for 0xD1 opcodes.
 */

struct instable opD1[8] = {

/*  [0]  */	{"rol",TERM,{271,RWOF|RWCF,RWOP0},Mv,1},	{"ror",TERM,{273,RWOF|RWCF,RWOP0},Mv,1},	{"rcl",TERM,{261,RWOF|RWCF,RWOP0},Mv,1},	{"rcr",TERM,{263,RWOF|RWCF,RWOP0},Mv,1},
/*  [4]  */	{"shl",TERM,{300,ROF|WNOTAF,RWOP0},Mv,1},	{"shr",TERM,{303,ROF|WNOTAF,RWOP0},Mv,1},	INVALID,		{"sar",TERM,{277,ROF|WNOTAF,RWOP0},Mv,1},
};


/*
 *	Decode table for 0xD2 opcodes.
 */

struct instable opD2[8] = {

/*  [0]  */	{"rolb",TERM,{272,RWOF|RWCF,RWOP0|RREG},Mv,0},	{"rorb",TERM,{274,Mv,Mv},0},	{"rclb",TERM,{262,RWOF|RWCF,RWOP0|RREG},Mv,0},	{"rcrb",TERM,{264,RWOF|RWCF,RWOP0|RREG},Mv,0},
/*  [4]  */	{"shlb",TERM,{301,ROF|WNOTAF,RWOP0|RREG},Mv,0},	{"shrb",TERM,{304,ROF|WNOTAF,RWOP0|RREG},Mv,0},	INVALID,		{"sarb",TERM,{278,ROF|WNOTAF,RWOP0|RREG},Mv,0},
};
/*
 *	Decode table for 0xD3 opcodes.
 */

struct instable opD3[8] = {

/*  [0]  */	{"rol",TERM,{271,RWOF|RWCF,RWOP0|RREG},Mv,1},	{"ror",TERM,{273,RWOF|RWCF,RWOP0|RREG},Mv,1},	{"rcl",TERM,{261,RWOF|RWCF,RWOP0|RREG},Mv,1},	{"rcr",TERM,{263,RWOF|RWCF,RWOP0|RREG},Mv,1},
/*  [4]  */	{"shl",TERM,{300,ROF|WNOTAF,RWOP0|RREG},Mv,1},	{"shr",TERM,{303,ROF|WNOTAF,RWOP0|RREG},Mv,1},	INVALID,		{"sar",TERM,{277,ROF|WNOTAF,RWOP0|RREG},Mv,1},
};


/*
 *	Decode table for 0xF6 opcodes.
 */

struct instable opF6[8] = {

/*  [0]  */	{"testb",TERM,{320,WNOTAF,ROP0|ROP1},IMw,0},	INVALID,		{"notb",TERM,{248,0,RWOP0},Mw,0},	{"negb",TERM,{245,WNOTAF,RWOP0},Mw,0},
/*  [4]  */	{"mulb",TERM,{243,RWCF|RWOF|UALLF,ROP0|WREG},MA,0},	{"imulb",TERM,{181,RWCF|RWOF|UALLF,ROP0|WREG},MA,0},	{"divb",TERM,{53,0,ROP0|WREG},MA,0},	{"idivb",TERM,{179,0,ROP0|WREG},MA,0},
};


/*
 *	Decode table for 0xF7 opcodes.
 */

struct instable opF7[8] = {

/*  [0]  */	{"test",TERM,{319,WNOTAF,ROP0|ROP1},IMw,1},	INVALID,		{"not",TERM,{247,0,RWOP0},Mw,1},	{"neg",TERM,{244,WNOTAF,RWOP0},Mw,1},
/*  [4]  */	{"mul",TERM,{242,RWCF|RWOF|UALLF,ROP0|WREG},MA,1},	{"imul",TERM,{180,RWOF|RWCF|UALLF,ROP0|WREG},MA,1},	{"div",TERM,{52,0,ROP0|WREG},MA,1},	{"idiv",TERM,{178,0,ROP0|WREG},MA,1},
};


/*
 *	Decode table for 0xFE opcodes.
 */

struct instable opFE[8] = {

/*  [0]  */	{"incb",TERM,{185,WNOTCF,RWOP0},Mw,0},	{"decb",TERM,{51,WNOTCF,RWOP0},Mw,0},	INVALID,		INVALID,
/*  [4]  */	INVALID,		INVALID,		INVALID,		INVALID,
};
/*
 *	Decode table for 0xFF opcodes.
 */

struct instable opFF[8] = {

/*  [0]  */	{"inc",TERM,{184,WNOTCF,RWOP0},Mw,1},	{"dec",TERM,{50,WNOTCF,RWOP0},Mw,1},	{"call",TERM,{20,0,OPT_CALL|ROP0},INM,0},	{"lcall",TERM,{213,UALLF,ROP0},INM,0},
/*  [4]  */	{"jmp",TERM,{203,0,ROP0},INM,0},	{"ljmp",TERM,{222,UALLF,ROP0},INM,0},	{"push",TERM,{258,0,OPT_PUSH|ROP0},M,1},	INVALID,
};

/* for 287 instructions, which are a mess to decode */

struct instable opFP1n2[64] = {
/* bit pattern:	1101 1xxx MODxx xR/M */
/*  [0,0] */	{"fadds",TERM,{60,0,FP|RWOP0},M,0},	{"fmuls",TERM,{132,0,RWOP0|FP},M,0},	{"fcoms",TERM,{81,RWCF|RWPF|RWZF|0,ROP0|FP},M,0},	{"fcomps",TERM,{80,RWCF|RWPF|RWZF|0,ROP0|FP},M,0},
/*  [0,4] */	{"fsubs",TERM,{165,0,RWOP0|FP},M,0},	{"fsubrs",TERM,{164,0,RWOP0|FP},M,0},	{"fdivs",TERM,{90,0,RWOP0|FP},M,0},	{"fdivrs",TERM,{89,0,RWOP0|FP},M,0},
/*  [1,0]  */	{"flds",TERM,{126,0,FP|ROP0},M,0},	INVALID,		{"fsts",TERM,{156,0,WOP0|FP},M,0},	{"fstps",TERM,{154,0,WOP0|FP},M,0},
/*  [1,4]  */	{"fldenv",TERM,{119,0,FP|ROP0},M,0},	{"fldcw",TERM,{118,0,FP|ROP0},M,0},	{"fnstenv",TERM,{138,0,FP|WOP0},M,0},	{"fnstcw",TERM,{137,0,FP|WOP0},M,0},
/*  [2,0]  */	{"fiaddl",TERM,{93,0,ROP0},M,0},	{"fimull",TERM,{106,0,ROP0},M,0},	{"ficoml",TERM,{95,RWCF|RWPF|RWZF,ROP0|FP},M,0},	{"ficompl",TERM,{97,RWCF|RWPF|RWZF,ROP0|FP},M,0},
/*  [2,4]  */	{"fisubl",TERM,{113,0,ROP0},M,0},	{"fisubrl",TERM,{115,0,ROP0},M,0},	{"fidivl",TERM,{99,0,ROP0},M,0},	{"fidivrl",TERM,{101,0,ROP0},M,0},
/*  [3,0]  */	{"fildl",TERM,{103,0,ROP0|FP},M,0},	INVALID,		{"fistl",TERM,{108,0,WOP0|FP},M,0},	{"fistpl",TERM,{110,0,WOP0|FP},M,0},
/*  [3,4]  */	INVALID,		{"fldt",TERM,{127,0,ROP0|FP},M,0},	INVALID,		{"fstpt",TERM,{155,0,WOP0|FP},M,0},
/*  [4,0]  */	{"faddl",TERM,{58,0,FP|RWOP0},M,0},	{"fmull",TERM,{130,0,RWOP0|FP},M,0},	{"fcoml",TERM,{75,RWCF|RWPF|RWZF,ROP0|FP},M,0},	{"fcompl",TERM,{78,RWCF|RWPF|RWZF,ROP0|FP},M,0},
/*  [4,1]  */	{"fsubl",TERM,{159,0,RWOP0|FP},M,0},	{"fsubrl",TERM,{162,0,RWOP0|FP},M,0},	{"fdivl",TERM,{84,0,RWOP0|FP},M,0},	{"fdivrl",TERM,{87,0,RWOP0|FP},M,0},
/*  [5,0]  */	{"fldl",TERM,{120,0,FP|ROP0},M,0},	INVALID,		{"fstl",TERM,{151,0,WOP0|FP},M,0},	{"fstpl",TERM,{153,0,WOP0|FP},M,0},
/*  [5,4]  */	{"frstor",TERM,{144,0,ROP0|FP},M,0},	INVALID,		{"fnsave",TERM,{136,0,WOP0|FP},M,0},	{"fnstsw",TERM,{139,0,FP|WOP0},M,0},
/*  [6,0]  */	{"fiadd",TERM,{92,0,ROP0},M,0},	{"fimul",TERM,{105,0,ROP0},M,0},	{"ficom",TERM,{94,RWCF|RWPF|RWZF,ROP0|FP},M,0},	{"ficomp",TERM,{96,RWCF|RWPF|RWZF,ROP0|FP},M,0},
/*  [6,4]  */	{"fisub",TERM,{112,0,ROP0},M,0},	{"fisubr",TERM,{114,0,ROP0},M,0},	{"fidiv",TERM,{98,0,ROP0},M,0},	{"fidivr",TERM,{100,0,ROP0},M,0},
/*  [7,0]  */	{"fild",TERM,{102,0,ROP0|FP},M,0},	INVALID,		{"fist",TERM,{107,0,WOP0|FP},M,0},	{"fistp",TERM,{109,0,WOP0|FP},M,0},
/*  [7,4]  */	{"fbld",TERM,{62,0,ROP0|FP},M,0},	{"fildll",TERM,{104,0,ROP0|FP},M,0},	{"fbstp",TERM,{63,0,WOP0|FP},M,0},	{"fistpll",TERM,{111,0,WOP0|FP},M,0},
};

struct instable opFP3[64] = {
/* bit  pattern:	1101 1xxx 11xx xREG */
/*  [0,0]  */	{"fadd",TERM,{57,0,FP},FF,0},	{"fmul",TERM,{129,0,FP},FF,0},	{"fcom",TERM,{73,RWCF|RWPF|RWZF,FP},F,0},	{"fcomp",TERM,{76,RWCF|RWPF|RWZF,FP},F,0},
/*  [0,4]  */	{"fsub",TERM,{158,0,FP},FF,0},	{"fsubr",TERM,{161,0,FP},FF,0},	{"fdiv",TERM,{83,0,FP},FF,0},	{"fdivr",TERM,{86,0,FP},FF,0},
/*  [1,0]  */	{"fld",TERM,{116,0,FP},F,0},	{"fxch",TERM,{174,0,FP},F,0},	{"fnop",TERM,{135,0,FP},GO_ON,0},	{"fstp",TERM,{152,0,FP},F,0},
/*  [1,4]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [2,0]  */	{"fcmovb",TERM,{65,RCF,FP},FF,0},	{"fcmove",TERM,{67,RZF,FP},FF,0},	{"fcmovbe",TERM,{66,RCF|RZF,FP},FF,0},	{"fcmovu",TERM,{72,RPF,FP},FF,0},
/*  [2,4]  */	INVALID,		{"fucompp",TERM,{171,RWCF|RWPF|RWZF,FP},GO_ON,0},INVALID,		INVALID,
/*  [3,0]  */	{"fcmovnb",TERM,{68,RCF,FP},FF,0},	{"fcmovne",TERM,{70,RZF,FP},FF,0},	{"fcmovnbe",TERM,{69,RCF|RZF,FP},FF,0},	{"fcmovnu",TERM,{71,RPF,FP},FF,0},
/*  [3,4]  */	INVALID,		{"fucomi",TERM,{168,RWCF|RWPF|RWZF,FP},F,0},	{"fcomi",TERM,{74,RWCF|RWPF|RWZF,FP},F,0},	INVALID,
/*  [4,0]  */	{"fadd",TERM,{57,0,FP},FF,0},	{"fmul",TERM,{129,0,FP},FF,0},	{"fcom",TERM,{73,RWCF|RWPF|RWZF,FP},F,0},	{"fcomp",TERM,{76,RWCF|RWPF|RWZF,FP},F,0},
/*  [4,4]  */	{"fsub",TERM,{158,0,FP},FF,0},	{"fsubr",TERM,{161,0,FP},FF,0},	{"fdiv",TERM,{83,0,FP},FF,0},	{"fdivr",TERM,{86,0,FP},FF,0},
/*  [5,0]  */	{"ffree",TERM,{91,0,FP},F,0},	{"fxch",TERM,{174,0,FP},F,0},	{"fst",TERM,{150,0,FP},F,0},	{"fstp",TERM,{152,0,FP},F,0},
/*  [5,4]  */	{"fucom",TERM,{167,RWCF|RWPF|RWZF,FP},F,0},	{"fucomp",TERM,{169,RWCF|RWPF|RWZF,FP},F,0},	INVALID,		INVALID,
/*  [6,0]  */	{"faddp",TERM,{59,0,FP},FF,0},	{"fmulp",TERM,{131,0,FP},FF,0},	{"fcomp",TERM,{76,RWCF|RWPF|RWZF,FP},F,0},	{"fcompp",TERM,{79,RWCF|RWPF|RWZF,FP},GO_ON,0},
/*  [6,4]  */	{"fsubp",TERM,{160,0,FP},FF,0},	{"fsubrp",TERM,{163,0,FP},FF,0},	{"fdivp",TERM,{85,0,FP},FF,0},	{"fdivrp",TERM,{88,0,FP},FF,0},
/*  [7,0]  */	{"ffree",TERM,{91,0,0},F,0},	{"fxch",TERM,{174,0,FP},F,0},	{"fstp",TERM,{152,0,FP},F,0},	{"fstp",TERM,{152,0,FP},F,0},
/*  [7,4]  */	{"fstsw",TERM,{157,0,FP|WOP0},M,0},	{"fucompi",TERM,{170,RWCF|RWPF|RWZF,FP},F,0},	{"fcompi",TERM,{77,RWCF|RWPF|RWZF,FP},F,0},		INVALID,
};

struct instable opFP4[32] = {
/* bit pattern:	1101 1001 111x xxxx */
/*  [0,0]  */	{"fchs",TERM,{64,0,FP},GO_ON,0},	{"fabs",TERM,{56,0,FP},GO_ON,0},	INVALID,		INVALID,
/*  [0,4]  */	{"ftst",TERM,{166,RWCF|RWPF|RWZF,FP},GO_ON,0},	{"fxam",TERM,{173,RWCF|RWPF|RWZF,FP},GO_ON,0},	INVALID,		INVALID,
/*  [1,0]  */	{"fld1",TERM,{117,0,FP},GO_ON,0},	{"fldl2t",TERM,{122,0,FP},GO_ON,0},{"fldl2e",TERM,{121,0,FP},GO_ON,0},{"fldpi",TERM,{125,0,FP},GO_ON,0},
/*  [1,4]  */	{"fldlg2",TERM,{123,0,FP},GO_ON,0},{"fldln2",TERM,{124,0,FP},GO_ON,0},{"fldz",TERM,{128,0,FP},GO_ON,0},	INVALID,
/*  [2,0]  */	{"f2xm1",TERM,{55,0,FP},GO_ON,0},	{"fyl2x",TERM,{175,0,FP},GO_ON,0},	{"fptan",TERM,{142,0,FP},GO_ON,0},	{"fpatan",TERM,{140,0,FP},GO_ON,0},
/*  [2,4]  */	{"fxtract",TERM,{335,0,FP},GO_ON,0}, {"fprem1",TERM,{332,0,FP},GO_ON,0}, {"fdecstp",TERM,{333,0,FP},GO_ON,0},{"fincstp",TERM,{334,0,FP},GO_ON,0},
/*  [3,0]  */	{"fprem",TERM,{141,0,FP},GO_ON,0},	{"fyl2xp1",TERM,{176,0,FP},GO_ON,0},{"fsqrt",TERM,{149,0,FP},GO_ON,0}, {"fsincos",TERM,{148,0,FP},GO_ON,0},
/*  [3,4]  */	{"frndint",TERM,{143,0,FP},GO_ON,0},{"fscale",TERM,{145,0,FP},GO_ON,0},{"fsin",TERM,{147,0,FP},GO_ON,0},	{"fcos",TERM,{82,0,FP},GO_ON,0},
};

struct instable opFP5[8] = {
/* bit pattern:	1101 1011 1110 0xxx */
/*  [0]  */	INVALID,		INVALID,		{"fnclex",TERM,{133,0,FP},GO_ON,0},{"fninit",TERM,{134,0,FP},GO_ON,0},
/*  [4]  */	{"fsetpm",TERM,{146,0,0},GO_ON,0},INVALID,		INVALID,		INVALID,
};

/*
 *	Main decode table for the op codes.  The first two nibbles
 *	will be used as an index into the table.  If there is a
 *	a need to further decode an instruction, the array to be
 *	referenced is indicated with the other two entries being
 *	empty.
 */

struct instable distable[256] = {

/* [0,0] */	{"addb",TERM,{7,WALLF,RWOP0|RREG|OPT_ADD},RMw,0},	{"add",TERM,{6,WALLF,RWOP0|RREG|OPT_ADD},RMw,1},	{"addb",TERM,{7,WALLF,ROP0|RWREG|OPT_ADD},MRw,0},	{"add",TERM,{6,WALLF,ROP0|RWREG|OPT_ADD},MRw,1},
/* [0,4] */	{"addb",TERM,{7,WALLF,ROP0|RWREG|OPT_ADD},IA,0},	{"add",TERM,{6,WALLF,ROP0|RWREG|OPT_ADD},IA,1},	{"push",TERM,{341,0,OPT_PUSH|RREG},SEG,1},	{"pop",TERM,{342,0,OPT_POP|WREG},SEG,1},
/* [0,8] */	{"orb",TERM,{250,WNOTAF,RWOP0|RREG},RMw,0},	{"or",TERM,{249,WNOTAF,RWOP0|RREG},RMw,1},	{"orb",TERM,{250,WNOTAF,ROP0|RWREG},MRw,0},	{"or",TERM,{249,WNOTAF,ROP0|RWREG},MRw,1},
/* [0,C] */	{"orb",TERM,{250,WNOTAF,ROP0|RWREG},IA,0},	{"or",TERM,{249,WNOTAF,ROP0|RWREG},IA,1},	{"push",TERM,{341,0,OPT_PUSH|RREG},SEG,1},	{"",op0F,{NO_OPGROUP,0,0},TERM,0},

/* [1,0] */	{"adcb",TERM,{5,RCF|WALLF,RWOP0|RREG},RMw,0},	{"adc",TERM,{4,RCF|WALLF,RWOP0|RREG},RMw,1},	{"adcb",TERM,{5,RCF|WALLF,ROP0|RWREG},MRw,0},	{"adc",TERM,{4,RCF|WALLF,ROP0|RWREG},MRw,1},
/* [1,4] */	{"adcb",TERM,{5,RCF|WALLF,ROP0|RWREG},IA,0},	{"adc",TERM,{4,RCF|WALLF,ROP0|RWREG},IA,1},	{"push",TERM,{341,0,OPT_PUSH|RREG},SEG,1},	{"pop",TERM,{342,0,OPT_POP|WREG},SEG,1},
/* [1,8] */	{"sbbb",TERM,{280,RCF|WALLF,RWOP0|RREG},RMw,0},	{"sbb",TERM,{279,RCF|WALLF,RWOP0|RREG},RMw,1},	{"sbbb",TERM,{280,RCF|WALLF,ROP0|RWREG},MRw,0},	{"sbb",TERM,{279,RCF|WALLF,ROP0|RWREG},MRw,1},
/* [1,C] */	{"sbbb",TERM,{280,RCF|WALLF,ROP0|RWREG},IA,0},	{"sbb",TERM,{279,RCF|WALLF,ROP0|RWREG},IA,1},	{"push",TERM,{341,0,OPT_PUSH|RREG},SEG,1},	{"pop",TERM,{342,0,OPT_POP|WREG},SEG,1},

/* [2,0] */	{"andb",TERM,{10,WNOTAF,RWOP0|RREG|OPT_AND},RMw,0},	{"and",TERM,{9,WNOTAF,RWOP0|RREG|OPT_AND},RMw,1},	{"andb",TERM,{10,WNOTAF,ROP0|RWREG|OPT_AND},MRw,0},	{"and",TERM,{9,WNOTAF,ROP0|RWREG|OPT_AND},MRw,1},
/* [2,4] */	{"andb",TERM,{10,WNOTAF,ROP0|RWREG|OPT_AND},IA,0},	{"and",TERM,{9,WNOTAF,ROP0|RWREG|OPT_AND},IA,1},	{"%es:",TERM,{NO_OPGROUP,0,0},OVERRIDE,0},{"daa",TERM,{344,WNOTOF|UALLF,RWREG},GO_ON,0},
/* [2,8] */	{"subb",TERM,{316,WALLF,RWOP0|RREG|OPT_SUB},RMw,0},	{"sub",TERM,{315,WALLF,RWOP0|RREG|OPT_SUB},RMw,1},	{"subb",TERM,{316,WALLF,ROP0|RWREG|OPT_SUB},MRw,0},	{"sub",TERM,{315,WALLF,ROP0|RWREG|OPT_SUB},MRw,1},
/* [2,C] */	{"subb",TERM,{316,WALLF,ROP0|RWREG|OPT_SUB},IA,0},	{"sub",TERM,{315,WALLF,ROP0|RWREG|OPT_SUB},IA,1},	{"%cs:",TERM,{NO_OPGROUP,0,0},OVERRIDE,0},{"das",TERM,{345,WNOTOF|UALLF,RWREG},GO_ON,0},

/* [3,0] */	{"xorb",TERM,{331,WNOTAF,RREG|RWOP0},RMw,0},	{"xor",TERM,{330,WNOTAF,RREG|RWOP0},RMw,1},	{"xorb",TERM,{331,WNOTAF,ROP0|RWREG},MRw,0},	{"xor",TERM,{330,WNOTAF,ROP0|RWREG},MRw,1},
/* [3,4] */	{"xorb",TERM,{331,WNOTAF,ROP0|RWREG},IA,0},	{"xor",TERM,{330,WNOTAF,ROP0|RWREG},IA,1},	{"%ss:",TERM,{NO_OPGROUP,0,0},OVERRIDE,0},{"aaa",TERM,{1,WAF|WCF|UALLF,RWREG},GO_ON,0},
/* [3,8] */	{"cmpb",TERM,{43,WALLF,RREG|ROP0},RMw,0},	{"cmp",TERM,{42,WALLF,RREG|ROP0},RMw,1},	{"cmpb",TERM,{43,WALLF,ROP0|RREG},MRw,0},	{"cmp",TERM,{42,WALLF,ROP0|RREG},MRw,1},
/* [3,C] */	{"cmpb",TERM,{43,WALLF,ROP0|RREG},IA,0},	{"cmp",TERM,{42,WALLF,ROP0|RREG},IA,1},	{"%ds:",TERM,{NO_OPGROUP,0,0},OVERRIDE,0},{"aas",TERM,{346,WAF|WCF|UALLF,RWREG},GO_ON,0},

/* [4,0] */	{"inc",TERM,{184,WNOTCF,RWREG},R,1},	{"inc",TERM,{184,WNOTCF,RWREG},R,1},	{"inc",TERM,{184,WNOTCF,RWREG},R,1},	{"inc",TERM,{184,WNOTCF,RWREG},R,1},
/* [4,4] */	{"inc",TERM,{184,WNOTCF,RWREG},R,1},	{"inc",TERM,{184,WNOTCF,RWREG},R,1},	{"inc",TERM,{184,WNOTCF,RWREG},R,1},	{"inc",TERM,{184,WNOTCF,RWREG},R,1},
/* [4,8] */	{"dec",TERM,{50,WNOTCF,RWREG},R,1},	{"dec",TERM,{50,WNOTCF,RWREG},R,1},	{"dec",TERM,{50,WNOTCF,RWREG},R,1},	{"dec",TERM,{50,WNOTCF,RWREG},R,1},
/* [4,C] */	{"dec",TERM,{50,WNOTCF,RWREG},R,1},	{"dec",TERM,{50,WNOTCF,RWREG},R,1},	{"dec",TERM,{50,WNOTCF,RWREG},R,1},	{"dec",TERM,{50,WNOTCF,RWREG},R,1},

/* [5,0] */	{"push",TERM,{258,0,OPT_PUSH|RREG},R,1},	{"push",TERM,{258,0,OPT_PUSH|RREG},R,1},	{"push",TERM,{258,0,OPT_PUSH|RREG},R,1},	{"push",TERM,{258,0,OPT_PUSH|RREG},R,1},
/* [5,4] */	{"push",TERM,{258,0,OPT_PUSH|RREG},R,1},	{"push",TERM,{258,0,OPT_PUSH|RREG},R,1},	{"push",TERM,{258,0,OPT_PUSH|RREG},R,1},	{"push",TERM,{258,0,OPT_PUSH|RREG},R,1},
/* [5,8] */	{"pop",TERM,{255,0,OPT_POP|WREG},R,1},	{"pop",TERM,{255,0,OPT_POP|WREG},R,1},	{"pop",TERM,{255,0,OPT_POP|WREG},R,1},	{"pop",TERM,{255,0,OPT_POP|WREG},R,1},
/* [5,C] */	{"pop",TERM,{255,0,OPT_POP|WREG},R,1},	{"pop",TERM,{255,0,OPT_POP|WREG},R,1},	{"pop",TERM,{255,0,OPT_POP|WREG},R,1},	{"pop",TERM,{255,0,OPT_POP|WREG},R,1},

/* [6,0] */	{"pusha",TERM,{259,0,DANGEROUS},GO_ON,1},	{"popa",TERM,{256,0,DANGEROUS},GO_ON,1},	{"bound",TERM,{12,0,ROP0|RREG},MR,1},	{"arpl",TERM,{11,WZF,RWOP0|RREG},RMw,0},
/* [6,4] */	{"%fs:",TERM,{NO_OPGROUP,0,0},OVERRIDE,0},{"%gs:",TERM,{NO_OPGROUP,0,0},OVERRIDE,0},{"data16",TERM,{NO_OPGROUP,0,0},DM,0},	{"addr16",TERM,{8,0,0},AM,0},
/* [6,8] */	{"push",TERM,{258,0,ROP0|OPT_PUSH},I,1},	{"imul",TERM,{180,0,ROP0|ROP1|RWREG},IMUL,1},	{"push",TERM,{258,0,ROP0|OPT_PUSH},Ib,1},	{"imul",TERM,{180,0,ROP0|ROP1|RWREG},IMUL,1},
/* [6,C] */	{"insb",TERM,{187,0,ROP1|WOP2|RWSYSTATE|INCR},GO_ON,0},	{"ins",TERM,{186,0,ROP1|WOP2|RWSYSTATE|INCR},GO_ON,1},	{"outsb",TERM,{254,0,ROP1|ROP2|RWSYSTATE|INCR},GO_ON,0},	{"outs",TERM,{253,0,ROP1|ROP2|RWSYSTATE|INCR},GO_ON,1},

/* [7,0] */	{"jo",TERM,{208,ROF,ROP0},BD,0},	{"jno",TERM,{205,ROF,ROP0},BD,0},	{"jb",TERM,{195,RCF,ROP0},BD,0},	{"jae",TERM,{194,RCF,ROP0},BD,0},
/* [7,4] */	{"je",TERM,{198,RZF,ROP0},BD,0},	{"jne",TERM,{204,RZF,ROP0},BD,0},	{"jbe",TERM,{196,RCF|RZF,ROP0},BD,0},	{"ja",TERM,{193,RCF|RZF,ROP0},BD,0},
/* [7,8] */	{"js",TERM,{210,RSF,ROP0},BD,0},	{"jns",TERM,{207,RSF,ROP0},BD,0},	{"jp",TERM,{209,RPF,ROP0},BD,0},	{"jnp",TERM,{206,RPF,ROP0},BD,0},
/* [7,C] */	{"jl",TERM,{201,RSF|ROF,ROP0},BD,0},	{"jge",TERM,{200,RSF|ROF,ROP0},BD,0},	{"jle",TERM,{202,RZF|RSF|ROF,ROP0},BD,0},	{"jg",TERM,{199,RZF|RSF|ROF,ROP0},BD,0},
/* [8,0] */	{"",op80,{NO_OPGROUP,0,0},TERM,0},	{"",op81,{NO_OPGROUP,0,0},TERM,0},	{"",op82,{NO_OPGROUP,0,0},TERM,0},	{"",op83,{NO_OPGROUP,0,0},TERM,0},
/* [8,4] */	{"testb",TERM,{320,WNOTAF,ROP0|RREG},MRw,0},	{"test",TERM,{319,WNOTAF,ROP0|RREG},MRw,1},	{"xchgb",TERM,{328,0,RWOP0|RWREG},MRw,0},	{"xchg",TERM,{327,0,RWOP0|RWREG},MRw,1},
/* [8,8] */	{"movb",TERM,{236,0,WOP0|RREG|OPT_MOV},RMw,0},	{"mov",TERM,{235,0,WOP0|RREG|OPT_MOV},RMw,1},	{"movb",TERM,{236,0,ROP0|WREG|OPT_MOV},MRw,0},	{"mov",TERM,{235,0,ROP0|WREG|OPT_MOV},MRw,1},
/* [8,C] */	{"mov",TERM,{338,0,WOP0|RREG|OPT_MOV},SM,1},	{"lea",TERM,{215,0,ROP0|WREG|OPT_LEA},MR,1},	{"mov",TERM,{339,0,ROP0|WREG|OPT_MOV},MS,1},	{"pop",TERM,{255,0,OPT_POP|WOP0},M,1},

/* [9,0] */	{"nop",TERM,{246,0,0},GO_ON,0},	{"xchg",TERM,{327,0,RWOP0|RWREG},RA,1},	{"xchg",TERM,{327,0,RWOP0|RWREG},RA,1},	{"xchg",TERM,{327,0,RWOP0|RWREG},RA,1},
/* [9,4] */	{"xchg",TERM,{327,0,RWOP0|RWREG},RA,1},	{"xchg",TERM,{327,0,RWOP0|RWREG},RA,1},	{"xchg",TERM,{327,0,RWOP0|RWREG},RA,1},	{"xchg",TERM,{327,0,RWOP0|RWREG},RA,1},
/* [9,8] */	{"",TERM,{336,0,RWREG},CBW,0},	{"",TERM,{337,0,RWREG|WOP0},CWD,0},	{"lcall",TERM,{213,UALLF,WSYSTATE},SO,0},	{"fwait",TERM,{172,0,FP},GO_ON,0},
/* [9,C] */	{"pushf",TERM,{260,WALLF,OPT_PUSH},GO_ON,1},	{"popf",TERM,{257,WALLF,OPT_POP},GO_ON,1},	{"sahf",TERM,{276,WNOTOF,RREG},GO_ON,0},	{"lahf",TERM,{211,RNOTOF,WREG|RSYSTATE},GO_ON,0},

/* [A,0] */	{"movb",TERM,{236,0,ROP0|WREG|OPT_MOV},OA,0},	{"mov",TERM,{235,0,ROP0|WREG|OPT_MOV},OA,1},	{"movb",TERM,{236,0,WOP0|RREG|OPT_MOV},AO,0},	{"mov",TERM,{235,0,WOP0|RREG|OPT_MOV},AO,1},
/* [A,4] */	{"movsb",TERM,{238,0,ROP1|WOP2|INCR},SD,0},	{"movs",TERM,{237,0,ROP1|WOP2|INCR},SD,1},	{"cmpsb",TERM,{45,WALLF,ROP1|ROP2|INCR},SD,0},	{"cmps",TERM,{44,WALLF,ROP1|ROP2|INCR},SD,1},
/* [A,8] */	{"testb",TERM,{320,WNOTAF,ROP0|RREG},IA,0},	{"test",TERM,{319,WNOTAF,ROP0|RREG},IA,1},	{"stosb",TERM,{313,0,RWOP1|RREG|INCR},AD,0},	{"stos",TERM,{312,0,WOP1|RREG|INCR},AD,1},
/* [A,C] */	{"lodsb",TERM,{227,0,ROP1|WREG|INCR},SA,0},	{"lods",TERM,{226,0,ROP1|WREG|INCR},SA,1},	{"scasb",TERM,{282,WALLF|INCR,ROP1|RREG|INCR},AD,0},	{"scas",TERM,{281,WALLF|INCR,ROP1|RREG|INCR},AD,1},

/* [B,0] */	{"movb",TERM,{236,0,ROP0|WREG|OPT_MOV},IR,0},	{"movb",TERM,{236,0,ROP0|WREG|OPT_MOV},IR,0},	{"movb",TERM,{236,0,ROP0|WREG|OPT_MOV},IR,0},	{"movb",TERM,{236,0,ROP0|WREG|OPT_MOV},IR,0},
/* [B,4] */	{"movb",TERM,{236,0,ROP0|WREG|OPT_MOV},IR,0},	{"movb",TERM,{236,0,ROP0|WREG|OPT_MOV},IR,0},	{"movb",TERM,{236,0,ROP0|WREG|OPT_MOV},IR,0},	{"movb",TERM,{236,0,ROP0|WREG|OPT_MOV},IR,0},
/* [B,8] */	{"mov",TERM,{235,0,ROP0|WREG|OPT_MOV},IR,1},	{"mov",TERM,{235,0,ROP0|WREG|OPT_MOV},IR,1},	{"mov",TERM,{235,0,ROP0|WREG|OPT_MOV},IR,1},	{"mov",TERM,{235,0,ROP0|WREG|OPT_MOV},IR,1},
/* [B,C] */	{"mov",TERM,{235,0,ROP0|WREG|OPT_MOV},IR,1},	{"mov",TERM,{235,0,ROP0|WREG|OPT_MOV},IR,1},	{"mov",TERM,{235,0,ROP0|WREG|OPT_MOV},IR,1},	{"mov",TERM,{235,0,ROP0|WREG|OPT_MOV},IR,1},

/* [C,0] */	{"",opC0,{NO_OPGROUP,0,0},TERM,0},	{"",opC1,{NO_OPGROUP,0,0},TERM,0},	{"ret",TERM,{270,0,OPT_RET|ROP0},RET,0},	{"ret",TERM,{270,0,OPT_RET},GO_ON,0},
/* [C,4] */	{"les",TERM,{217,0,ROP0|WREG},MR,0},	{"lds",TERM,{214,0,ROP0|WREG},MR,0},	{"movb",TERM,{236,0,ROP0|WOP1|OPT_MOV},IMw,0},	{"mov",TERM,{235,0,ROP0|WOP1|OPT_MOV},IMw,1},
/* [C,8] */	{"enter",TERM,{54,0,DANGEROUS},ENTER,0},	{"leave",TERM,{216,0,DANGEROUS},GO_ON,0},	{"lret",TERM,{231,0,OPT_RET|ROP0},RET,0},	{"lret",TERM,{231,0,OPT_RET},GO_ON,0},
/* [C,C] */	{"int",TERM,{188,0,0},INT3,0},	{"int",TERM,{188,0,0},Ib,0},	{"into",TERM,{189,ROF,0},GO_ON,0},	{"iret",TERM,{192,0,OPT_RET},GO_ON,0},

/* [D,0] */	{"",opD0,{NO_OPGROUP,0,0},TERM,0},	{"",opD1,{NO_OPGROUP,0,0},TERM,0},	{"",opD2,{NO_OPGROUP,0,0},TERM,0},	{"",opD3,{NO_OPGROUP,0,0},TERM,0},
/* [D,4] */	{"aam",TERM,{3,WSF|WZF|WPF|UALLF,RWREG},U,0},	{"aad",TERM,{2,WSF|WZF|WPF|UALLF,RWREG},U,0},	{"falc",TERM,{61,0,0},GO_ON,0},	{"xlat",TERM,{329,0,DANGEROUS},GO_ON,0},

/* 287 instructions.  Note that although the indirect field		*/
/* indicates opFP1n2 for further decoding, this is not necessarily	*/
/* the case since the opFP arrays are not partitioned according to key1	*/
/* and key2.  opFP1n2 is given only to indicate that we haven't		*/
/* finished decoding the instruction.					*/
/* [D,8] */	{"",opFP1n2,{NO_OPGROUP,0,0},TERM,0},	{"",opFP1n2,{NO_OPGROUP,0,0},TERM,0},	{"",opFP1n2,{NO_OPGROUP,0,0},TERM,0},	{"",opFP1n2,{NO_OPGROUP,0,0},TERM,0},
/* [D,C] */	{"",opFP1n2,{NO_OPGROUP,0,0},TERM,0},	{"",opFP1n2,{NO_OPGROUP,0,0},TERM,0},	{"",opFP1n2,{NO_OPGROUP,0,0},TERM,0},	{"",opFP1n2,{NO_OPGROUP,0,0},TERM,0},

/* [E,0] */	{"loopnz",TERM,{229,RZF,RWREG|ROP0},BD,0},	{"loopz",TERM,{230,RZF,RWREG|ROP0},BD,0},	{"loop",TERM,{228,0,RWREG|ROP0},BD,0},	{"jcxz",TERM,{197,0,RREG|ROP0},BD,0},
/* [E,4] */	{"inb",TERM,{183,0,WREG},P,0},	{"in",TERM,{182,0,WREG},P,1},	{"outb",TERM,{252,0,RREG},P,0},	{"out",TERM,{251,0,RREG},P,1},
/* [E,8] */	{"call",TERM,{20,0,OPT_CALL|ROP0},D,0},	{"jmp",TERM,{203,0,ROP0},D,0},	{"ljmp",TERM,{222,UALLF,WSYSTATE},SO,0},	{"jmp",TERM,{203,0,ROP0},BD,0},
/* [E,C] */	{"inb",TERM,{183,0,ROP1|WREG},V,0},	{"in",TERM,{182,0,ROP1|WREG},V,1},	{"outb",TERM,{252,0,ROP1|RREG},V,0},	{"out",TERM,{251,0,ROP1|RREG},V,1},

/* [F,0] */	{"lock ",TERM,{NO_OPGROUP,0,0},PREFIX,0},	{"",TERM,{NO_OPGROUP,0,0},JTAB,0},	{"repnz ",TERM,{NO_OPGROUP,0,0},PREFIX,0},	{"repz ",TERM,{NO_OPGROUP,0,0},PREFIX,0},
/* [F,4] */	{"hlt",TERM,{177,0,RWSYSTATE},GO_ON,0},	{"cmc",TERM,{25,RWCF,0},GO_ON,0},	{"",opF6,{NO_OPGROUP,0,0},TERM,0},	{"",opF7,{NO_OPGROUP,0,0},TERM,0},
/* [F,8] */	{"clc",TERM,{21,WCF,0},GO_ON,0},	{"stc",TERM,{309,WCF,0},GO_ON,0},	{"cli",TERM,{23,0,WSYSTATE},GO_ON,0},	{"sti",TERM,{311,0,WSYSTATE},GO_ON,0},
/* [F,C] */	{"cld",TERM,{22,0,WSYSTATE},GO_ON,0},	{"std",TERM,{310,0,WSYSTATE},GO_ON,0},	{"",opFE,{NO_OPGROUP,0,0},TERM,0},	{"",opFF,{NO_OPGROUP,0,0},TERM,0},
};
