#ident	"@(#)kern-i386:util/kdb/scodb/dis.h	1.1.1.1"
#ident  "$Header$"
/*
 *	Copyright (C) 1989-1993 The Santa Cruz Operation, Inc.
 *		All Rights Reserved.
 *	The information in this file is provided for the exclusive use of
 *	the licensees of The Santa Cruz Operation, Inc.  Such users have the
 *	right to use, modify, and incorporate this code into other products
 *	for purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 */

/*************************************************************
*
*	PINST stuff - instruction printing stuff
*/

/*
*	un-assembly flags
*
*		INTEL	mov dest,source (!INTEL: source,dest)
*		REGPCT	print a CREG character before register names
*		SYMONLY	print symbol names instead of the addresses
*		ARGS	print 8(%ebp) as @1, 12(%ebp) as @2, etc.
*		DISP	print #(reg) as (#+reg) (eg f4(ebp) == (f4+ebp) )
*		BINARY	print the binary instruction
*		IMMED	print immediate values preceeded by a '$'
*		STATIC	print static text symbols
*/
#define		UNEGATE		'-'	/* use this to negate a flag */
#define		UM_INTEL	0x0001
#define		UM_REGPCT	0x0002
#define		UM_SYMONLY	0x0004
#define		UM_ARGS		0x0008
#define		UM_DISP		0x0010
#define		UM_BINARY	0x0020
#define		UM_IMMED	0x0040
#define		UM_STATIC	0x0080
#define		UM_SHORT	0x0100
#define			UMNARG	8	/* how many args to @	     */

/*
*	default flags
*/
#define		UDEFAULT	(				\
					UM_REGPCT	|	\
					UM_SYMONLY	|	\
					UM_IMMED	|	\
					UM_STATIC		\
				)





/*************************************************************
*
*	OPCODE stuff
*/





/*
*	every opcode has the following with it:
*		code
*		group [0 == no group]
*		segment
*
*	they also have a number of operands (up to a maximum),
*	which may be implicit (AL, etc) or explicit, requiring
*	the user to supply operands.
*	operands have:
*		operand type
*/

/*
*	parts of the ModR/M byte
*/
#define		MRM_MOD(c)	((c) >> 6)
#define		MRM_REG(c)	(((c) >> 3) & 07)
#define		MRM_RM(c)	((c) & 07)

/*
*	parts of the SIB byte - same pieces as ModR/M
*/
#define		SIB_SCALE(c)	MRM_MOD(c)
#define		SIB_INDEX(c)	MRM_REG(c)
#define		SIB_BASE(c)	MRM_RM(c)

/*
*	Code flags
*
*	C_GP:	see G_
*	C_SEG:	see S_
*/
#define		C_NOSUCH	0x0001 /* no such opcode!	*/
#define		C_NORM		0x0002 /* normal instruction	*/
#define		C_GP		0x0003 /* Group instruction	*/
#define		C_COPROC	0x0004 /* coProcessor		*/
#define		C_2BYTE		0x0005 /* 2 byte instr		*/
#define		C_SEG		0x0006 /* Segment override	*/
#define		C_PREFIX	0x0007 /* prefiX opcode		*/
#define	CODE			0x0007
#define		C_CALL		0x0008
#define		C_JT		0x0010 /* jump/call type	*/
#define		C_B		0x0020 /* byte instruction	*/
#define		C_F		0x0040 /* name needs suffix	*/
#define		C_I		0x0080 /* indirect mem ref	*/
#define	CODE_FLAGS		0x00F8

/*
*	Group flags
*		if C_GP then G_[1-8] is always valid
*/
#define		G_S		0x0200 /* shift			*/
#define		G_U		0x0300 /* unary			*/
#define		G_C		0x0400 /* inc/dec		*/
#define		G_1		0x0000 /* group	1		*/
#define		G_2		0x1000 /*	2		*/
#define		G_3		0x2000 /*	3		*/
#define		G_4		0x3000 /*	4		*/
#define		G_5		0x4000 /*	5		*/
#define		G_6		0x5000 /*	6		*/
#define		G_7		0x6000 /*	7		*/
#define		G_8		0x7000 /*	8		*/
#define	GRP_FLAGS		0x0F00 /* 6-F left		*/
#define GROUP			0xF000 /* 8-F left		*/
#define		GRPSHF	12

/*
*	flags required for a conditional jump to be taken -
*	we overload the third operand field (since the second is
*	not used) to store this stuff.
*/
#define		F_CXZ	(REGP[T_ECX] == 0)
#define		F_CF	((REGP[T_EFL] & PS_C) != 0)
#define		F_ZF	((REGP[T_EFL] & PS_Z) != 0)
#define		F_SF	((REGP[T_EFL] & PS_N) != 0)
#define		F_OF	((REGP[T_EFL] & PS_V) != 0)
#define		F_PF	((REGP[T_EFL] & PS_P) != 0)

/*
*	Segment flags for C_SEG (segment override)
*/
#define		S_CS		0x0100
#define		S_DS		0x0200
#define		S_ES		0x0300
#define		S_FS		0x0400
#define		S_GS		0x0500
#define		S_SS		0x0600
#define	SEGMENT			0x0F00 /* 7-F left		*/
#define		SEGSHF	8

/*
*	operands are implicit if they contain I_ types,
*	and explicit if they contain A_ and O types.
*	if neither, then there is no operand, and no more operands.
*/

/*
*	Addressing types, for explicit operands
*	see "80386 Programmer's Reference Manual", Appendix A
*/
#define		A_A		0x0001
#define		A_C		0x0002
#define		A_D		0x0003
#define		A_E		0x0004
#define		A_G		0x0005
#define		A_I		0x0006
#define		A_J		0x0007
#define		A_M		0x0008
#define		A_O		0x0009
#define		A_R		0x000A
#define		A_S		0x000B
#define		A_T		0x000C
#define	ADDRESS			0x000F /*  D-F left		*/

/*
*	Operand types, for explicit operands
*	see "80386 Programmer's Reference Manual", Appendix A
*/
#define		Oa		0x0010
#define		Ob		0x0020
#define		Oc		0x0030
#define		Od		0x0040
#define		Op		0x0050
#define		Os		0x0060
#define		Ov		0x0070
#define		Ow		0x0080
#define		Ox		0x0090 /* b/v */
#define	OPERAND	 		0x00F0 /* 9-F left		*/
#define	OP_SHF	4

/*
*	Operand type, for implicit operands
*/
#define		I_AL		0x0100
#define		I_AH		0x0200
#define		I_eAX		0x0300
#define		I_BL		0x0400
#define		I_BH		0x0500
#define		I_eBX		0x0600
#define		I_CL		0x0700
#define		I_CH		0x0800
#define		I_eCX		0x0900
#define		I_DL		0x0A00
#define		I_DH		0x0B00
#define		I_DX		0x0C00
#define		I_eDX		0x0D00
#define		I_CS		0x0E00
#define		I_DS		0x0F00
#define		I_ES		0x1000
#define		I_FS		0x1100
#define		I_GS		0x1200
#define		I_SS		0x1300
#define		I_eBP		0x1400
#define		I_eDI		0x1500
#define		I_eSI		0x1600
#define		I_eSP		0x1700
#define		I_xA		0x1800 /* C_B: AL, else eAX	*/
#define		I_1		0x1900 /* implicit 1		*/
#define		I_FLG		0x1A00 /* flags register	*/
#define		I_DSSI		0x1B00 /* [ds:si]		*/
#define		I_ESDI		0x1C00 /* [es:di]		*/
#define	IMPLICIT		0x1F00 /* 1D-1F left		*/

#pragma pack(2)
struct opmap {
	short	op_flags;
	char	*op_name;
	short	op_opnds[3];
};
#pragma pack()

/**************************************************************/








/*************************************************************
*
*	INSTRUCTION stuff
*/

/*
*	operand flags
*/
#define	OF_NONE				0
#define		OF_INDIR		0x00010
#define	OF_TYP				0x000E0	/* xxx0 */
#define		OF_IMMED		0x00020
#define			OFI_ADDR	0x00100	/* immed is an address*/
#define			OFI_RELIP	0x00200	/* immed is rel to ip */
#define			OFI_JMCL	0x00400 /* jump or call	      */
#define			OFI_SZ8		0x01000	/* immed is 8 bits    */
#define			OFI_SZ16	0x02000	/* immed is 16 bits   */
#define			OFI_SZ32	0x04000	/* immed is 32 bits   */
#define			OFI_SZ48	0x08000	/* immed is 32 bits   */
#define			OFI_SZ64	0x10000	/* immed is 32 bits   */
#define			OFI_SIZE	0x1F000
#define			OFI_SHF		12
#define			OFIS_PTR	0x20000
#define		OF_REG			0x00040
#define			OFR_SPEC	0x00100
#define		OF_MEM			0x00080
#define				OFM_NX	0x0000F

#define		C_REG			0x00010000 /* register    */
#define			CR_SC1		0x00100000
#define			CR_SC2		0x00200000
#define			CR_SC4		0x00400000
#define			CR_SC8		0x00800000
#define			CR_SCALE	0x00F00000 /* scaled	  */
#define			SCALE_SHF 20
#define		C_DISP			0x00020000 /*		  */
#define			C_DISP8		(C_DISP|1) /* next cont	  */
#define			C_DISP16	(C_DISP|2)/*	is	  */
#define			C_DISP32	(C_DISP|4)/* disp.	  */
#define	C_DISPM	0x0000000F
#define		C_SIB			0x00040000 /* need an sib */
#define	CTYPE				0x000F0000

/*
*	oa_flag:
*		OF_REG:
*			if more than one cont[] is filled then
*			the first one is for byte, second for word,
*			third for dword.
*		OF_IMMED:
*			one cont[] for 1/2/4 byte immediates,
*			two cont[]s for 6/8 byte immediates
*		OF_MEM:
*			OFM_NX cont[]s
*/
struct operand {
	short	oa_flag;
	long	oa_cont[10];
};

#define		I_NOSUCH	0x0001
#define		I_PRFX		0x0002
#define		I_2BYTE		0x0004
#define		I_SEGOVR	0x0008
#define		I_SFX		0x0070
#define			I_SFB	0x0010
#define			I_SFW	0x0020
#define			I_SFL	0x0040
#define		I_CALL		0x0080
#define		I_JUMP		0x0100

/*
*	max instruction length:
*		2 prefix
*		1 operand size
*		1 address size
*		2 bytes opcode
*		1 modrm
*		1 sib
*		4 addr disp
*		4 immed data
*/
#define		MXINSTLEN	16
struct instr {
	long		 in_flag;
	long		 in_seg;
	long 		 in_off;
	int		 in_len;
	unsigned char	 in_prefix;
	unsigned char	 in_segovr;
	char		*in_opcn;
	unsigned char	 in_opcde;
	unsigned char	 in_buf[MXINSTLEN];
	int		 in_nopnd;
	struct operand	 in_opnd[3];
};
#define	IN_OPCODE(inp)	((inp)->in_buf + (inp)->in_opcde)



/*************************************************************
*
*	OPERAND stuff
*/

#define		IMMED		0x0001		/* no mrm - WYSIWYG */
#define		RELIP		0x0002		/* relative to IP   */
#define		MRM		0x0004		/* has ModR/M	    */
#define		SPECIAL		0x0008	/* special lookup */
#define		INDIR		0x0010		/* is indirect	    */
#define		OFFSET		0x0020		/* immediate offset */
#define		REG		0x0F00		/* what REG specs   */
#define			R_GN	0x0100
#define			R_CT	0x0200
#define			R_DB	0x0300
#define			R_SG	0x0400
#define			R_TS	0x0500
#define			RSHF	8
#define		MOD		0xF000		/* what MOD specs   */
#define			M_GN	0x1000
#define			MSHF	12

#define		O_PTRT		0x80
#define		O_SSZ(n,m)	((n)|((m)<<4))
#define		OSIZ(osz, n)	((osz) ? (opndsz[(n)-1]) >> 4 : (opndsz[(n)-1]) & 0x0F)
#define		RXSIZ(o)	((o) < 3 ? (o) - 1 : 2)
#define		OPTR(n)		(opndsz[(n)-1] & O_PTRT)












/*************************************************************
*
*	REGISTERS stuff
*/

#define		REGISTER	0x0000FFFF

#define		LO		0x00000001	/*  8 bits */
#define		HI		0x00000002	/*  8 bits */
#define		X		0x00000004	/* 16 bits */
#define		EX		0x00000008	/* 32 bits */
#define	REGSIZ			0x0000000F

#define		_N0		0x00000000
#define		_N1		0x00000010
#define		_N2		0x00000020
#define		_N3		0x00000030
#define		_N4		0x00000040
#define		_N5		0x00000050
#define		_N6		0x00000060
#define		_N7		0x00000070
#define		_NX	0x000000F0
#define		_NXSHF	4
#define		REGN(f)	((((f) & _NX) >> _NXSHF))

#define		_GR_A		0x00000100
#define		_GR_B		0x00000200
#define		_GR_C		0x00000300
#define		_GR_D		0x00000400
#define		_GR_BP		0x00000500
#define		_GR_SP		0x00000600
#define		_GR_DI		0x00000700
#define		_GR_SI		0x00000800
#define		_GR_X	0x00000F00
#define		_GRSHF	8
#define		GRN(f)	((((f) & _GR_X) >> _GRSHF) - 1)

#define		_XR_IP		0x00001000
#define		_XR_FL		0x00002000
#define		_XR_CS		0x00003000
#define		_XR_DS		0x00004000
#define		_XR_ES		0x00005000
#define		_XR_FS		0x00006000
#define		_XR_GS		0x00007000
#define		_XR_SS		0x00008000
#define		_XR_X	0x0000F000
#define		_XRSHF	12
#define		XRN(f)	((((f) & _XR_X) >> _XRSHF) - 1)
#define		_CR		0x00009000
#define		_DR		0x0000A000
#define		_TR		0x0000B000

#define		GR_AH		(_GR_A|HI)
#define		GR_AL		(_GR_A|LO)
#define		GR_AX		(_GR_A|X )
#define		GR_EAX		(_GR_A|EX)

#define		GR_BH		(_GR_B|HI)
#define		GR_BL		(_GR_B|LO)
#define		GR_BX		(_GR_B|X )
#define		GR_EBX		(_GR_B|EX)

#define		GR_CH		(_GR_C|HI)
#define		GR_CL		(_GR_C|LO)
#define		GR_CX		(_GR_C|X )
#define		GR_ECX		(_GR_C|EX)

#define		GR_DH		(_GR_D|HI)
#define		GR_DL		(_GR_D|LO)
#define		GR_DX		(_GR_D|X )
#define		GR_EDX		(_GR_D|EX)

#define		GR_BP		(_GR_BP|X)
#define		GR_EBP		(_GR_BP|EX)
#define		GR_DI		(_GR_DI|X)
#define		GR_EDI		(_GR_DI|EX)
#define		GR_SI		(_GR_SI|X)
#define		GR_ESI		(_GR_SI|EX)
#define		GR_SP		(_GR_SP|X)
#define		GR_ESP		(_GR_SP|EX)

#define		XR_IP		(_XR_IP|EX)
#define		XR_FL		(_XR_FL|EX)
#define		XR_CS		(_XR_CS|EX)
#define		XR_DS		(_XR_DS|EX)
#define		XR_ES		(_XR_ES|EX)
#define		XR_FS		(_XR_FS|EX)
#define		XR_GS		(_XR_GS|EX)
#define		XR_SS		(_XR_SS|EX)

#define		CR_0		(_CR|_N0|EX)
#define		CR_1		(_CR|_N1|EX)
#define		CR_2		(_CR|_N2|EX)
#define		CR_3		(_CR|_N3|EX)
#define		CR_4		(_CR|_N4|EX)
#define		CR_5		(_CR|_N5|EX)
#define		CR_6		(_CR|_N6|EX)
#define		CR_7		(_CR|_N7|EX)

#define		DR_0		(_DR|_N0|EX)
#define		DR_1		(_DR|_N1|EX)
#define		DR_2		(_DR|_N2|EX)
#define		DR_3		(_DR|_N3|EX)
#define		DR_4		(_DR|_N4|EX)
#define		DR_5		(_DR|_N5|EX)
#define		DR_6		(_DR|_N6|EX)
#define		DR_7		(_DR|_N7|EX)

#define		TR_0		(_TR|_N0|EX)
#define		TR_1		(_TR|_N1|EX)
#define		TR_2		(_TR|_N2|EX)
#define		TR_3		(_TR|_N3|EX)
#define		TR_4		(_TR|_N4|EX)
#define		TR_5		(_TR|_N5|EX)
#define		TR_6		(_TR|_N6|EX)
#define		TR_7		(_TR|_N7|EX)
