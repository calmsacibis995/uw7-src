#ident	"@(#)debugger:libmachine/i386/dis.h	1.8"

/* This file uses 'C' style comments so it can be included by Tables.c */

#define	NLINE	16	/* max bytes per instruction */
#define	NCPO	10	/* number of chars per opcode */
#define	TRUE	1
#define	FALSE	0

/* 	These are the instruction formats as they appear in
 *	'Tables.c'.  Here they are given numerical values
 *	for use in the actual disassembly of an object file.
 */

#define UNKNOWN	0
#define MRw	2
#define IMlw	3
#define IMw	4
#define IR	5
#define OA	6
#define AO	7
#define MS	8
#define SM	9
#define Mv	10
#define Mw	11
#define M	12
#define R	13
#define RA	14
#define SEG	15
#define MR	16
#define IA	17
#define MA	18
#define SD	19
#define AD	20
#define SA	21
#define D	22
#define INM	23
#define SO	24
#define BD	25
#define I	26
#define P	27
#define V	28
#define DSHIFT	29  	/* for double shift that has an 8-bit immediate */
#define U	30
#define OVERRIDE 31
#define GO_ON	32
#define	O	33	/* for call	 */
#define JTAB	34	/* jump table 	 */
#define IMUL	35	/* for 186 iimul instr   */
#define CBW 36 /* so that data16 can be evaluated for cbw and its variants  */
#define MvI	37	/* for 186 logicals  */
#define	ENTER	38	/* for 186 enter instr   */
#define RMw	39	/* for 286 arpl instr  */
#define Ib	40	/* for push immediate byte  */
#define	F	41	/* for 287 instructions  */
#define	FF	42	/* for 287 instructions  */
#define DM	43	/* 16-bit data  */
#define AM	44	/* 16-bit addr  */
#define LSEG	45	/* for 3-bit seg reg encoding  */
#define	MIb	46	/* for 386 logicals  */
#define	SREG	47	/* for 386 special registers  */
#define PREFIX	48	/* an instruction prefix like REP, LOCK  */
#define INT3	49 	/* The int 3 instruction, which has a fake operand  */
#define DSHIFTcl 50 	/* for double shift that implicitly uses %cl  */
#define CWD	51   	/* so that data16 can be evaluated for cwd and variants  */
#define RET	52    	/* single immediate 16-bit operand  */
#define MOVZ	53   	/* for movs and movz, with different size operands  */

#define	FILL	0x90	/* Fill byte used for alignment (nop)	 */


/* 	here are some macros that we use for determining w and v bit, */
/*	and operand size  and 3 bit register number  */

#define WBIT(x)		(x & 0x1)	/* to get "w" bit  */
#define REGNO(x)	(x & 0x7)	/* to get 3 bit register number  */
#define VBIT(x)		((x >> 1) & 0x1 )	/* to get "v" bit  */

#define OPSIZE(data16, wbit)	( (wbit) ? (( data16) ? 2:4) : 1 )

#define REG_ONLY	3	/* this indicates a single register with */
				/* no displacement is an operand  */

#define LONGOPERAND	1	/* value of the "w" bit indicating a  */
				/* long operand ( 2 bytes or 4 bytes )  */



/*	This is the structure that will be used for storing all the */
/*	op code information.  The structure values themselves are */
/*	in 'Tables.c'. */
 

struct	instable {
	char		name[NCPO];
	unsigned	adr_mode;
	short		suffix;		/* for instructions which may */
					/* have a 'w' or 'l' suffix */
	short		action;		/* for use by find_return */
};

/* actions for use by find_return */
#define	A_NONE		0
#define	A_RET		1
#define	A_LEAVE		2
#define	A_PUSH		3
#define	A_PUSHA		4
#define	A_PUSHF		5
#define	A_POP		6
#define	A_POPA		7
#define	A_POPF		8
#define	A_CALL		9
#define	A_MOV		10
#define	A_INC		11
#define	A_DEC		12
#define	A_ADD		13
#define	A_SUB		14
#define	A_AND		15
#define	A_JMP		16
#define	A_JC		17

/* register values for find_return and fcn_prolog */
#define FR_AL	1
#define FR_CL	2
#define FR_DL	3
#define FR_BL	4
#define FR_AH	5
#define FR_CH	6
#define FR_DH	7
#define FR_BH	8
#define FR_AX	9
#define FR_CX	10
#define FR_DX	11
#define FR_BX	12
#define FR_SP	13
#define FR_BP	14
#define FR_SI	15
#define FR_DI	16
#define FR_EAX	17
#define FR_ECX	18
#define FR_EDX	19
#define FR_EBX	20
#define FR_ESP	21
#define FR_EBP	22
#define FR_ESI	23
#define FR_EDI	24

#define 	OPLEN	35	/* maximum length of a single operand */
				/* (will be used for printing) */
#define	ADDLW     0x81
#define	ADDLB     0x83
#define JMPrel8   0xEB
#define JMPrel32  0xE9
#define JMPind32  0xFF	/* note same opcode as ICALL */
#define PUSHLebp  0x55
#define PUSHLeax  0x50
#define PUSHLebx  0x53
#define PUSHLesi  0x56
#define PUSHLedi  0x57
#define POPLeax   0x58
#define MOVLrr    0x8B
#define ESPEBP    0xEC
#define SUBLimm8  0x83
#define ADDLimm8  0x83
#define ANDLimm8  0x83
#define ANDLebp	  0xe5
#define SUBLimm32 0x81
#define fromESP   0xEC
#define toESP     0xC4
#define POPr32    0x58
#define XCHG      0x87
#define NOP	  0x90
#define CALL	  0xE8
#define LCALL	  0x9A
#define ICALL	  0xFF
#define POPLecx	  0x59
#define RETURNNEAR	  0xC3
#define RETURNFAR	  0xCB
#define RETURNNEARANDPOP  0xC2
#define RETURNFARANDPOP	  0xCA


/* the following arrays are contained in Tables.c */
extern const struct instable    distable[16][16];
extern const struct instable    op0F[10][16];
extern const struct instable    opFP1n2[8][8];
extern const struct instable    opFP3[8][8];
extern const struct instable    opFP4[4][8];
extern const struct instable    opFP5[8];
extern const struct instable    op0F00[8];
extern const struct instable    op0F01[8];
extern const struct instable	op0FC8[4];
extern const struct instable    op0FBA[8];
extern const struct instable    op80[8];
extern const struct instable    op81[8];
extern const struct instable    op82[8];
extern const struct instable    op83[8];
extern const struct instable    opC0[8];
extern const struct instable    opC1[8];
extern const struct instable    opD0[8];
extern const struct instable    opD1[8];
extern const struct instable    opD2[8];
extern const struct instable    opD3[8];
extern const struct instable    opF6[8];
extern const struct instable    opF7[8];
extern const struct instable    opFE[8];
extern const struct instable    opFF[8];

extern const char	*REG16[8][2];
extern const char	*REG32[8][2];
extern const unsigned char	REG16_IND[8][2];
extern const unsigned char	REG32_IND[8][2];
extern const char	*SEGREG[6];
extern const char	*DEBUGREG[8];
extern const char	*CONTROLREG[8];
extern const char	*TESTREG[8];
extern const char	*FLOATREG[8];
extern const char	dispsize16[8][4];
extern const char	dispsize32[8][4];
extern const char	*regname16[4][8];
extern const char	*regname32[4][8];
extern const char	*indexname[8];
extern const char	*scale_factor[4];
