/*
 *	@(#)interp.h	11.1	10/22/97	11:59:06
 *
 * Header file for 80286 REAL MODE interpreter
 */
/*
 *	S001	Tue Nov 26 16:44:07 PST 1996	-	hiramc@sco.COM
 *	- rework both SET_OF_SF macros to better set the OF flag
 *	Thu Oct 31 16:13:21 PST 1996	-	hiramc@sco.COM
 *	- added MOVZX, MOVSX and PUSH_I instructions
 */
#ifndef _INTERP_H
#define	_INTERP_H	1

#define	AAA			0x37			/* ASCII Adjust after Addition */
#define	AAD			0xD5			/* ASCII Adjust before Division ( 2 byte ) */
#define	AAM			0xD4			/* ASCII Adjust after Multiplication ( 2 byte ) */
#define	AAS			0x3F			/* ASCII Adjust after Subtraction */
#define	ADC_AL		0x14			/* ADC AL,IMM */
#define	ADC_AX		0x15			/* ADC AX,IMM */
#define	AND_AL		0x24			/* AND AL, IMM */
#define	AND_AX		0x25			/* AND AX, IMM */
#define	ADD_AL		0x04			/* ADD AL, IMM */
#define	ADD_AX		0x05			/* ADD AX, IMM */
#define	BOUND		0x62			/* BOUND */
#define	CALL_FAR	0x9A			/* Call intersegment ( 5 byte ) */
#define	CALL		0xE8			/* Call intrsegment offset */
#define	CBW			0x98			/* Convert byte to word (sign extend al into ah */
#define	CLC			0xF8			/* Clear carry flag */
#define	CLD			0xFC			/* Clear direction flag */
#define	CLI			0xFA			/* Clear interrupt flag */
#define	CMC			0xF5			/* Complement carry flag */
#define	CMP_AL		0x3C			/* CMP AL, IMM */
#define	CMP_AX		0x3D			/* CMP AX, IMM */
#define	CMPSB		0xA6			/* CMPSB */
#define	CMPSW		0xA7			/* CMPSW */
#define	CWD			0x99			/* Convert word to double */
#define	DAA			0x27			/* decimal adjust after addition */
#define	DAS			0x2F			/* decimal adjust after subtraction */
#define	DEC_AX		0x48
#define	DEC_BX		0x4B
#define	DEC_CX		0x49
#define	DEC_DX		0x4A
#define	DEC_SP		0x4C
#define	DEC_BP		0x4D
#define	DEC_SI		0x4E
#define	DEC_DI		0x4F
#define	ESC0		0xD8
#define	ESC1		0xD9
#define	ESC2		0xDA
#define	ESC3		0xDB
#define	ESC4		0xDC
#define	ESC5		0xDD
#define	ESC6		0xDE
#define	ESC7		0xDF
#define	HLT			0xF4
#define	INB_DX		0xEC			/* INB thru DX */
#define	INW_DX		0xED			/* INW thru DX */
#define	INB			0xE4			/* INB from ports 0 - 0xff */
#define	INW			0xE5			/* INW from ports 0 - 0xfe */
#define	INC_AX		0x40
#define	INC_BX		0x43
#define	INC_CX		0x41
#define	INC_DX		0x42
#define	INC_SP		0x44
#define	INC_BP		0x45
#define	INC_SI		0x46
#define	INC_DI		0x47
#define	INT3		0xCC			/* software break point */
#define	INTX		0xCD			/* Software interrupts */
#define	INT0		0xCE			/* INT on Overflow, NOT implemented! */
#define	IRET		0xCF			
#define	JA			0x77
#define	JAE			0x73
#define	JB			0x72
#define	JBE			0x76
#define	JCXZ		0xE3
#define	JE			0x74
#define	JG			0x7F
#define	JGE			0x7D
#define	JL			0x7C
#define JLE			0x7E
#define	JMPFAR		0xEA
#define	JMPSHORT	0xEB
#define	JMPREL		0xE9
#define	JNE			0x75
#define	JNO			0x71
#define	JNP			0x7B
#define	JNS			0x79
#define	JO			0x70
#define	JP			0x7A
#define	JS			0x78
#define	LAHF		0x9F
#define	LOCK		0xF0
#define	LODSB		0xAC
#define	LODSW		0xAD
#define	LOOP		0xE2
#define	LOOPZ		0xE1
#define	LOOPNZ		0xE0
#define	MOVR2_R1W	0x89
#define	MOVR1_R2W	0x8B
#define	MOVR2_R1B	0x88
#define	MOVR1_R2B	0x8A
#define	MOVSEG_REG	0x8C
#define	MOVREG_SEG	0x8E
#define	AX_AX		0xC0
#define	AX_BX		0xD8
#define	AX_CX		0xC8
#define	AX_DX		0xD0
#define	AX_SI		0xF0
#define	AX_DI		0xF8
#define	AX_SP		0xE0
#define	AX_BP		0xE8
#define	AX_CS		0xC8
#define	AX_DS		0xD8
#define	AX_ES		0xC0
#define	AX_SS		0xD0


#define	MOVAX_MEM	0xA1
#define	MOVAL_MEM	0XA0
#define	MOVMEM_AX	0xA3
#define	MOVMEM_AL	0xA2

#define	XOR_IMMAX	0x35
#define	XOR_IMMAL	0x34

#define	OR_IMMAX	0x0D
#define	OR_IMMAL	0x0C


#define	NOP			0x90
#define	OUTDX_AX	0xEF
#define	OUTDX_AL	0xEE
#define	OUTAX		0xE7
#define	OUTAL		0xE6

#define	OPSIZE		0x66
#define	ADDRSZ		0x67

#define	STC			0xF9
#define	STD			0xFD
#define	STI			0xFB

#define	STOSB		0xAA
#define	STOSW		0xAB

#define	TEST_AL		0xA8
#define	TEST_AX		0xA9


#define	XLAT		0xD7

#define	WAIT		0x9B
#define	XCHGBX		0x93
#define	XCHGCX		0x91
#define	XCHGDX		0x92
#define	XCHGSP		0x94
#define	XCHGBP		0x95
#define	XCHGSI		0x96
#define	XCHGDI		0x97

#define	SUB_AL		0x2C
#define	SUB_AX		0x2D

#define	SEGOVRCS	0x2E
#define	SEGOVRDS	0x3E
#define	SEGOVRES	0x26
#define	SEGOVRSS	0x36


#define	PUSHAX		0x50
#define	PUSHBX		0x53
#define	PUSHCX		0x51
#define	PUSHDX		0x52
#define	PUSHSP		0x54
#define	PUSHBP		0x55
#define	PUSHSI		0x56
#define	PUSHDI		0x57

#define	PUSHCS		0x0E
#define	PUSHDS		0x1E
#define	PUSHES		0x06
#define	PUSHSS		0x16

#define	PUSHFL		0x9C

#define	POPAX		0x58
#define	POPBX		0x5B
#define	POPCX		0x59
#define	POPDX		0x5A
#define	POPSP		0x5C
#define	POPBP		0x5D
#define	POPSI		0x5E
#define	POPDI		0x5F

#define	POPFL		0x9D

#define	POPDS		0x1F
#define	POPES		0x07
#define	POPSS		0x17

#define	PUSHALL		0x60
#define	POPALL		0x61

#define PUSH_Ib		0x6A
#define PUSH_Iw		0x68

#define	RET			0xC3
#define	RETF		0xCB
#define	RET_DISP	0xC2
#define	RETF_DISP	0xCA
#define	LEA			0x8D
#define	SAHF		0x9E


#define	MOVAX_IM	0xB8
#define	MOVBX_IM	0xBB
#define	MOVCX_IM	0xB9
#define	MOVDX_IM	0xBA
#define	MOVSI_IM	0xBE
#define	MOVDI_IM	0xBF
#define	MOVSP_IM	0xBC
#define	MOVBP_IM	0xBD
#define	MOVAL_IM	0xB0
#define	MOVAH_IM	0xB4
#define	MOVBL_IM	0xB3
#define	MOVBH_IM	0xB7
#define	MOVCL_IM	0xB1
#define	MOVCH_IM	0xB5
#define	MOVDL_IM	0xB2
#define	MOVDH_IM	0xB6

#define MOVZX_B		0xB6
#define MOVZX_W		0xB7
#define MOVSX_B		0xBE
#define MOVSX_W		0xBF


#define	ENTER		0xC8
#define	LEAVE		0xC9

#define	REPNZ		0xF2
#define	REPZ		0xF3

#define	MOVSB		0xA4
#define	MOVSW		0xA5

#define	SCASB		0xAE
#define	SCASW		0xAF


#define	LDS			0xC5
#define	LES			0xC4

#define	REG_AX			000
#define	REG_AL			000
#define	REG_AH			004

#define	REG_BX			003
#define	REG_BL			003
#define	REG_BH			007

#define	REG_CX			001
#define	REG_CL			001
#define	REG_CH			005

#define	REG_DX			002
#define	REG_DL			002
#define	REG_DH			006

#define	REG_SP			004
#define	REG_BP			005
#define	REG_SI			006
#define	REG_DI			007

#define	MOV				0x89
#define	ADC				0x11
#define	ADD				0x01
#define	AND				0x21
#define	CMP				0x39
#define	OR				0x09
#define	SBB				0x19
#define	SUB				0x29
#define	XOR				0x31

#define	MOVB			0x88
#define	ADCB			0x10
#define	ADDB			0x00
#define	ANDB			0x20
#define	CMPB			0x38
#define	ORB				0x08
#define	SBBB			0x18
#define	SUBB			0x28
#define	XORB			0x30


extern unsigned char *base_mem;

#define SET_NEW_IMP(CS,IP) 		(void *)(base_mem + ((unsigned long)CS << 4) + IP)

#define SET_NEW_SMP() 		(unsigned short *)(base_mem + ((unsigned long)r_ss << 4) + r_sp)

/*
 * The following macro should only be called if  R_CS  is already known
 */
#define	GET_IP()				(imp - base_mem - ((unsigned long)r_cs << 4))

/*
 * The following macro should only be called if  R_IP  is already known
 */
#define	GET_CS()				((imp - r_ip - base_mem) >> 4)

#define	SET_NEW_DMP(DS,REG1,REG2,DISP)	(void *)(base_mem + ((unsigned long)DS << 4) + ((REG1 + REG2 + DISP) & 0xffff))

#define	SET_NEW_EA(REG1,REG2,DISP)	((REG1 + REG2 + DISP) & 0xffff)

#define	SWAP(r1,r2)	{ \
			int tmpval; \
 \
			tmpval = r1; \
			r1 = r2; \
			r2 = tmpval; \
			}

#define	PUSH_ARG(x)	*--smp = (x); \
					r_sp -= 2;


#define	POP_ARG(x)	(x) = *smp++; \
					r_sp += 2;

#define	SET_SF(VAR) SF = (VAR & 0x8000) ? 1: 0
#define	SET_SF_8(VAR) SF = (VAR & 0x80) ? 1: 0

#define	SET_OF_SF(TMPVAR,VAR) 		/* SET Overflow  & sign flags for ADD, ADC, SUB, SBB, CMP, CMPS, INC, DEC, SCAS */ \
		if (TMPVAR) { \
			if (!(VAR & 0x8000)) { \
				SF = 0; \
			} \
			else { \
				SF = 1; \
			} \
		} \
		else { \
			if (VAR & 0x8000) { \
				SF = 1; \
			} \
			else { \
				SF = 0; \
			} \
		} \
	OF = ((VAR & 0x00010000) >> 16) ^ ((VAR & 0x00008000) >> 15);	/* S001	*/

#define	SET_ZF_PF(VAR) 				/* Set Zero Flag and Parity Flags for result */ \
			if (VAR & 0xffff) \
				ZF = 0; \
			else \
				ZF = 1; \
			PF = PARITY_ARRAY[VAR & 0xff]; \

#define	SET_OF_SF_8(TMPVAR,VAR)  		/* SET Overflow  & sign flags for ADD, ADC, SUB, SBB, CMP, CMPS, INC, DEC, SCAS */ \
		if (TMPVAR) { \
			if (!(VAR & 0x80)) { \
				SF = 0; \
			} \
			else { \
				SF = 1; \
			} \
		} \
		else { \
			if (VAR & 0x80) { \
				SF = 1; \
			} \
			else { \
				SF = 0; \
			} \
		} \
	OF = ((VAR & 0x0100) >> 8) ^ ((VAR & 0x0080) >> 7);		/* S001	*/

#define	SET_ZF_PF_8(VAR) 			/* Set Zero Flag and Parity Flags for result */ \
			if (VAR & 0xff) \
				ZF = 0; \
			else \
				ZF = 1; \
			PF = PARITY_ARRAY[VAR & 0xff]; 


#define	SET_CF(VAR)  \
			if (VAR & 0xFFFF0000)  \
				CF = 1; \
			else \
				CF = 0;

#define	SET_CF_8(VAR) \
			if (VAR & 0xFF00) \
				CF = 1; \
			else \
				CF = 0;

#define	CF	flags.r_flags.Cf				/* Carry Flag */
#define	PF	flags.r_flags.Pf				/* Parity Flag */
#define	AF	flags.r_flags.Af				/* Aux. Carry Flag */
#define	ZF	flags.r_flags.Zf				/* Zero Flag */
#define	SF	flags.r_flags.Sf				/* Sign Flag */
#define	TF	flags.r_flags.Tf				/* Trace/Trap Flag */
#define	IF	flags.r_flags.If				/* Interrupt Flag */
#define	DF	flags.r_flags.Df				/* Direction Flag */
#define	OF	flags.r_flags.Of				/* Overflow Flag */
#define	r_fl	flags.r_flag				/* All flags */

struct REGS {
	unsigned short Ax;
	unsigned short Bx;
	unsigned short Cx;
	unsigned short Dx;
	unsigned short Si;
	unsigned short Di;
	unsigned short Sp;
	unsigned short Bp;
	unsigned short Flags;
	unsigned short Es;
	unsigned short Ds;
	unsigned short Ss;
	unsigned short Cs;
	unsigned short Ip;
};


#define	SET_AL_AH()\
	r_al = r_ax & 0xff; \
	r_ah = (r_ax >> 8) & 0xff;

#define	SET_BL_BH() \
	r_bl = r_bx & 0xff; \
	r_bh = (r_bx >> 8) & 0xff;

#define	SET_CL_CH() \
	r_cl = r_cx & 0xff; \
	r_ch = (r_cx >> 8) & 0xff;

#define	SET_DL_DH() \
	r_dl = r_dx & 0xff; \
	r_dh = (r_dx >> 8) & 0xff;


#define	SET_AX() r_ax = (r_al & 0xff) | ((r_ah & 0xff) << 8);
#define	SET_BX() r_bx = (r_bl & 0xff) | ((r_bh & 0xff) << 8);
#define	SET_CX() r_cx = (r_cl & 0xff) | ((r_ch & 0xff) << 8);
#define	SET_DX() r_dx = (r_dl & 0xff) | ((r_dh & 0xff) << 8);


#define	SET_ALL_BREGS()  \
	SET_AL_AH(); \
	SET_BL_BH(); \
	SET_CL_CH(); \
	SET_DL_DH();


#define	SET_ALL_WREGS() \
	SET_AX(); \
	SET_BX(); \
	SET_CX(); \
	SET_DX();

#define	SET_ALL_EREGS() \
	r_eax = (r_eax & 0xffff0000) | (r_ax & 0xffff); r_ebx = (r_ebx & 0xffff0000) | (r_bx & 0xffff); \
	r_ecx = (r_ecx & 0xffff0000) | (r_cx & 0xffff); r_edx = (r_edx & 0xffff0000) | (r_dx & 0xffff); \
	r_esi = (r_esi & 0xffff0000) | (r_si & 0xffff); r_edi = (r_edi & 0xffff0000) | (r_di & 0xffff); \
	r_esp = (r_esp & 0xffff0000) | (r_sp & 0xffff); r_ebp = (r_ebp & 0xffff0000) | (r_bp & 0xffff);

#define	SET_ALL_REGS() \
	r_ax = r_eax & 0xffff; r_ebx = r_bx & 0xffff; \
	r_ecx = r_cx & 0xffff; r_edx = r_dx & 0xffff; \
	r_esi = r_si & 0xffff; r_edi = r_di & 0xffff; \
	r_esp = r_sp & 0xffff; r_ebp = r_bp & 0xffff;

#define	SET_EAX() r_eax = (r_eax & 0xffff0000) | (r_ax & 0xffff);
#define	SET_EBX() r_ebx = (r_ebx & 0xffff0000) | (r_bx & 0xffff)
#define	SET_ECX() r_ecx = (r_ecx & 0xffff0000) | (r_cx & 0xffff);
#define	SET_EDX() r_edx = (r_edx & 0xffff0000) | (r_dx & 0xffff);
#define	SET_ESI() r_esi = (r_esi & 0xffff0000) | (r_si & 0xffff);
#define	SET_EDI() r_edi = (r_edi & 0xffff0000) | (r_di & 0xffff); 
#define	SET_ESP() r_esp = (r_esp & 0xffff0000) | (r_sp & 0xffff);
#define	SET_EBP() r_ebp = (r_ebp & 0xffff0000) | (r_bp & 0xffff);

	
#define	TRUNC_16(x) x &= 0xffff

#define	SET_SF_32(VAR) SF = (VAR & 0x80000000) ? 1: 0

#define	SET_AX_AL_AH()	\
	r_ax = r_eax & 0xffff; \
	r_al = r_ax & 0xff; \
	r_ah = (r_ax >> 8) & 0xff;

#define	SET_DX_DL_DH()	\
	r_dx = r_edx & 0xffff; \
	r_dl = r_dx & 0xff; \
	r_dh = (r_dx >> 8) & 0xff;

#define	SET_BX_BL_BH()	\
	r_bx = r_ebx & 0xffff; \
	r_bl = r_bx & 0xff; \
	r_bh = (r_bx >> 8) & 0xff;

#define	SET_CX_CL_CH()	\
	r_cx = r_ecx & 0xffff; \
	r_cl = r_cx & 0xff; \
	r_ch = (r_cx >> 8) & 0xff;


#define	SET_WB_REGS_FROM_TMP(reg32,reg16,reg8l,reg8h,tmph,tmpl) \
	reg32 = (tmpl & 0xffff) | ((tmph & 0xffff) << 16); \
	reg16 = (tmpl & 0xffff); \
	reg8l = reg16 & 0xff; \
	reg8h = (reg16 >> 8) & 0xff;

#define	SET_WREGS_FROM_TMP(reg32,reg16,tmph,tmpl) \
	reg32 = (tmpl & 0xffff) | ((tmph & 0xffff) << 16); \
	reg16 = (tmpl & 0xffff); 

#define	SET_WB_REGS(reg32,reg16,reg8l,reg8h) \
	reg16 = (reg32 & 0xffff); \
	reg8l = reg16 & 0xff; \
	reg8h = (reg16 >> 8) & 0xff;

#endif	/*	_INTERP_H	*/
