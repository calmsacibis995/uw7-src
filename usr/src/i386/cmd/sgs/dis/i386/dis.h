#ident	"@(#)dis:i386/dis.h	1.10"

/*
 *	This is the header file for the iapx disassembler.
 *	The information contained in the first part of the file
 *	is common to each version, while the last part is dependent
 *	on the particular machine architecture being used.
 */

#define		NCPS	10	/* number of chars per symbol	*/
#define		NHEX	80	/* max # chars in object per line	*/
#define		NLINE	256	/* max # chars in mnemonic per line	*/
#define		FAIL	0
#define		TRUE	1
#define		FALSE	0
#define		LEAD	1
#define		NOLEAD	0
#define		NOLEADSHORT 2
#define		TERM 0		/* used in _tbls.c to indicate		*/
				/* that the 'indirect' field of the	*/
				/* 'instable' terminates - no pointer.	*/
				/* Is also checked in 'dis_text()' in	*/
				/* _bits.c.				*/

#define	LNNOBLKSZ	1024	/* size of blocks of line numbers	  */
#define	SYMBLKSZ	1024	/* size if blocks of symbol table entries */
#define	STRNGEQ		0	/* used in string compare operation	  */

/*
 *	This is the structure that will be used for storing all the
 *	op code information.  The structure values themselves are
 *	in '_tbls.c'.
 */

struct optinfo {

	unsigned long opgroup;
	unsigned long flags;
	unsigned long other;
};
struct	instable {
	char		name[NCPS];
	struct instable *indirect;	/* for decode op codes */
	struct optinfo optinfo;
	unsigned	adr_mode;
	int		suffix;		/* for instructions which may
					   have a 'w' or 'l' suffix */
};

/*	NOTE:	the following information in this file must be changed
 *		between the different versions of the disassembler.
 *
 *	This structure is used to determine the displacements and registers
 *	used in the addressing modes.  The values are in 'tables.c'.
 */
struct addr {
	int	disp;
	char	regs[9];
};

/*
 *	The register that contains the frame pointer is required for
 *	symbolic disassembly. The *86 do not have an argument pointer.
 *	Both are %[E]BX.
 */
#define APNO 5
#define FPNO 5
/*
 *	These are the instruction formats as they appear in
 *	'tables.c'.  Here they are given numerical values
 *	for use in the actual disassembly of an object file.
 */
#define UNKNOWN	0
#define IM8w	1
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
#define DSHIFT	29 /* for double shift that has an 8-bit immediate */
#define U	30
#define OVERRIDE 31
#define GO_ON	32
#define	O	33	/* for call	*/
#define JTAB	34	/* jump table 	*/
#define IMUL	35	/* for 186 iimul instr  */
#define CBW 36 /* so that data16 can be evaluated for cbw and its variants */
#define MvI	37	/* for 186 logicals */
#define	ENTER	38	/* for 186 enter instr  */
#define RMw	39	/* */
#define Ib	40	/* for push immediate byte */
#define	F	41	/* for 287 instructions */
#define	FF	42	/* for 287 instructions */
#define DM	43	/* 16-bit data */
#define AM	44	/* 16-bit addr */
#define LSEG	45	/* for 3-bit seg reg encoding */
#define	MIb	46	/* for 386 logicals */
#define	SREG	47	/* for 386 special registers */
#define PREFIX 48 /* an instruction prefix like REP, LOCK */
#define INT3 49   /* The int 3 instruction, which has a fake operand */
#define DSHIFTcl 50 /* for double shift that implicitly uses %cl */
#define CWD 51    /* so that data16 can be evaluated for cwd and variants */
#define RET 52    /* single immediate 16-bit operand */
#define MOVZ 53   /* for movs and movz, with different size operands */
#define RM16	54	/* instruction with 16 bits operands */
#define M16	55	/* 16 bit operand ( memory or register ) */
#define NOPFMTS 56

#define	FILL	0x90	/* Fill byte used for alignment (nop)	*/
/*
** Register numbers for the i386
*/
#define EAX 0
#define ECX 1
#define EDX 2
#define EBX 3
#define ESP 4
#define EBP 5
#define ESI 6
#define EDI 7
#define NO_REG 8

/* Output formats that are machine dependent */

#define OFLAG oflag?"0%o+%s":"0x%x+%s"

/* Information for the optinfo.other field */
#define RSHIFT		0
#define WSHIFT		(REGOP+1)

#define ENDOPS		(REGSHIFT+WSHIFT+1)

#define RON			(1<<RSHIFT)
#define WON			(1<<WSHIFT)
#define RWON		(RON|WON)

#define OP0SHIFT	0
#define ROP0		(1<<(RSHIFT+OP0SHIFT))
#define WOP0		(1<<(WSHIFT+OP0SHIFT))

#define OP1SHIFT	1
#define ROP1		(1<<(RSHIFT+OP1SHIFT))
#define WOP1		(1<<(WSHIFT+OP1SHIFT))

#define OP2SHIFT	2
#define ROP2		(1<<(RSHIFT+OP2SHIFT))
#define WOP2		(1<<(WSHIFT+OP2SHIFT))

#define OP3SHIFT	3
#define ROP3		(1<<(RSHIFT+OP3SHIFT))
#define WOP3		(1<<(WSHIFT+OP3SHIFT))

#define REGOP		3
#define NUMOPS		(REGOP + 1)
#define REGSHIFT	OP3SHIFT
#define RREG		ROP3
#define WREG		WOP3

#define RWOP0		(ROP0|WOP0)
#define RWOP1		(ROP1|WOP1)
#define RWOP2		(ROP2|WOP2)
#define RWOP3		(ROP3|WOP3)
#define RWREG		RWOP3

#define RSYSTATE	(1<<(ENDOPS+0))
#define WSYSTATE	(1<<(ENDOPS+1))
#define RWSYSTATE	(RSYSTATE|WSYSTATE)

#define OPT_PUSH	(1<<(ENDOPS+2))
#define OPT_POP		(1<<(ENDOPS+3))
#define OPT_CALL	(1<<(ENDOPS+4))
#define OPT_RET		(1<<(ENDOPS+5))
#define FP			(1<<(ENDOPS+6))
#define INCR		(1<<(ENDOPS+7))
#define OPT_LEA		(1<<(ENDOPS+8))
#define OPT_MOV		(1<<(ENDOPS+9))
#define OPT_ADD		(1<<(ENDOPS+10))
#define OPT_AND		(1<<(ENDOPS+11))
#define OPT_SUB		(1<<(ENDOPS+12))

#define DANGEROUS	(1<<(ENDOPS+13))
#define CANT_HANDLE_YET	DANGEROUS

/* Information for the optinfo.flags field */
#define ASHIFT		0
#define CSHIFT		1
#define OSHIFT		2
#define PSHIFT		3
#define SSHIFT		4
#define ZSHIFT		5
#define NFLAGS		6

#define RFSHIFT		0
#define WFSHIFT		NFLAGS

#define RAF			(1<<(ASHIFT+RFSHIFT))
#define WAF			(1<<(ASHIFT+WFSHIFT))
#define RWAF		(RAF|WAF)

#define RCF			(1<<(CSHIFT+RFSHIFT))
#define WCF			(1<<(CSHIFT+WFSHIFT))
#define RWCF		(RCF|WCF)

#define ROF			(1<<(OSHIFT+RFSHIFT))
#define WOF			(1<<(OSHIFT+WFSHIFT))
#define RWOF		(ROF|WOF)

#define RPF			(1<<(PSHIFT+RFSHIFT))
#define WPF			(1<<(PSHIFT+WFSHIFT))
#define RWPF		(RPF|WPF)

#define RSF			(1<<(SSHIFT+RFSHIFT))
#define WSF			(1<<(SSHIFT+WFSHIFT))
#define RWSF		(RSF|WSF)

#define RZF			(1<<(ZSHIFT+RFSHIFT))
#define WZF			(1<<(ZSHIFT+WFSHIFT))
#define RWZF		(RZF|WZF)

#define RALLF		(RAF|RCF|ROF|RPF|RSF|RZF)
#define WALLF		(WAF|WCF|WOF|WPF|WSF|WZF)
#define RWALLF		(RALLF|WALLF)

#define RNOTAF		(RCF|ROF|RPF|RSF|RZF)
#define WNOTAF		(WCF|WOF|WPF|WSF|WZF)
#define RWNOTAF		(RNOTAF|WNOTAF)

#define RNOTCF		(RAF|ROF|RPF|RSF|RZF)
#define WNOTCF		(WAF|WOF|WPF|WSF|WZF)
#define RWNOTCF		(RNOTCF|WNOTCF)

#define RNOTOF		(RCF|RAF|RPF|RSF|RZF)
#define WNOTOF		(WCF|WAF|WPF|WSF|WZF)
#define RWNOTOF		(RNOTOF|WNOTOF)

#define UALLF		(1<<20)  /* everything undefined but those
								referenced in other flags */

#define OPTINFO_SIB			1
#define OPTINFO_SIB_NOREG	2
#define OPTINFO_REGONLY		3
#define OPTINFO_DISPONLY	4
#define OPTINFO_DISPREG		5
#define OPTINFO_IMMEDIATE	6

extern int (*Optinfo)();
extern struct opsinfo *opsinfo;
struct info {
	long value;
	unsigned long loc;
};
struct opsinfo {
	union {
		struct info disp;
		struct info imm;
	} u;
	unsigned char fmt;
	unsigned char scale;
	unsigned char reg;
#define OPTINFO_SIXTEEN			(1<<0)
#define OPTINFO_EIGHTBIT		(1<<1)
#define OPTINFO_REL				(1<<2)
#define OPTINFO_ADDR_SIXTEEN	(1<<3)
#define OPTINFO_SPECIAL			(1<<4)
#define OPTINFO_SEGREG			(1<<5)
#define OPTINFO_DEST			(1<<6)
#define OPTINFO_SRC				(1<<7)
	unsigned char flags;
	int index; 
	char *overreg; 
	void *rel;
	int refto;
	int refflags;
};

#define NO_SUCH_ADDR		0xffffffff

#define NOPGROUPS	347
#define NO_OPGROUP	999
