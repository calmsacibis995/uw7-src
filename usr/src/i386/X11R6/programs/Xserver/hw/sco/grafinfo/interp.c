/*
 *	@(#)interp.c	11.1	10/22/97	11:59:03
 *
 *	interp.c - 80286 REAL MODE interpreter, does some 386 code too,
 *		but always in 16 bit mode
 *
 *	Expects to be able to call ErrorF
 *	See: interpIfc.c for initialization and calling sequence
 */
/*
 * Modification history:
 *
 *	S001	Mon Nov 11 15:54:14 PST 1996	-	hiramc@sco.COM
 *	- add IOEnable check to I/O, add subroutine V86IOEnable to maintain
 *	- I/O port enable bitmap
 * S000, Tue Oct 22 15:02:18 GMT 1996,	kylec@sco.com, hiramc@sco.COM
 *	- thru Thu Nov  7 16:33:47 PST 1996
 * 	- Add support for the following opcodes:
 *		PUSH_Ib (push immediate byte)
 *		PUSH_Iw (push immediate word)
 * 	- Fixed problems with IRET, INTX.  cs and ip were backwards.
 *	- Add support for MOVZX and MOVSX
 *	- add 'return' after unimplemented instructions, was getting stuck here.
 *	- fix opsize test, opposite meaning, in decode_386
 *	- rework all I/O, add CMOS and PIT emulators
 */

#include	"interp.h"
#include	"v86opts.h"
#include	<sys/types.h>
#include	<sys/inline.h>

#define	DEBUG_INTERP	1

#ifdef	DEBUG_INTERP
#include	<limits.h>
#include	<sys/times.h>
#include	<unistd.h>
#ifndef CLK_TCK
#define CLK_TCK         _sysconf(3)     /* clocks/second (_SC_CLK_TCK is 3) */
#endif

static struct tms time_0;
static clock_t clock_tick_0;
static struct tms time_1;
static clock_t clock_tick_1;
static int instrs_0;
static int instrs_1;
static int instrs_counted;

static void StartTimer( i )
int i;
{
instrs_0 = i;
clock_tick_0 = times( & time_0 );
}
static void StopTimer( i )
int i;
{
instrs_1 = i;
clock_tick_1 = times( & time_1 );
}
static void ElapsedTime() {
	clock_t ctet;
	int i;
	double mset;
	double	micperinstr;

	i = instrs_1 - instrs_0;
	ctet = clock_tick_1 - clock_tick_0;
	mset = ((double)ctet * 1000.0) / (double) CLK_TCK;
	ErrorF("interp ET: clock ticks %d == %f milliseconds (CLK_TCK %d)\n", ctet, mset, CLK_TCK );

	if ( i > 0 ) micperinstr = ((double)mset * 1000.0) / (double) i;
		else micperinstr = 1000;

	ErrorF("interp ET: instructions %d, microseconds/instr %6.2f\n", i, micperinstr );
}

#endif	/* DEBUG_INTERP	*/

static union {
	unsigned short r_flag;
	struct {
		int Cf:1;
		int X1:1;
		int Pf:1;
		int X2:1;
		int Af:1;
		int X3:1;
		int Zf:1;
		int Sf:1;
		int Tf:1;
		int If:1;
		int Df:1;
		int Of:1;
		int X4:1;
		int X5:1;
		int X6:1;
		int X7:1;
	} r_flags;
} flags;


static unsigned char PARITY_ARRAY[] = { 
		1,0,0,1,0,1,1,0, 0,1,1,0,1,0,0,1, 0,1,1,0,1,0,0,1, 1,0,0,1,0,1,1,0,
		0,1,1,0,1,0,0,1, 1,0,0,1,0,1,1,0, 1,0,0,1,0,1,1,0, 0,1,1,0,1,0,0,1,
		0,1,1,0,1,0,0,1, 1,0,0,1,0,1,1,0, 1,0,0,1,0,1,1,0, 0,1,1,0,1,0,0,1,
		1,0,0,1,0,1,1,0, 0,1,1,0,1,0,0,1, 0,1,1,0,1,0,0,1, 1,0,0,1,0,1,1,0,
		0,1,1,0,1,0,0,1, 1,0,0,1,0,1,1,0, 1,0,0,1,0,1,1,0, 0,1,1,0,1,0,0,1,
		1,0,0,1,0,1,1,0, 0,1,1,0,1,0,0,1, 0,1,1,0,1,0,0,1, 1,0,0,1,0,1,1,0,
		1,0,0,1,0,1,1,0, 0,1,1,0,1,0,0,1, 0,1,1,0,1,0,0,1, 1,0,0,1,0,1,1,0,
		0,1,1,0,1,0,0,1, 1,0,0,1,0,1,1,0, 1,0,0,1,0,1,1,0, 0,1,1,0,1,0,0,1,
};

/*
 * This program interprets 8086/80286 (real mode) instructions
 */

static long r_ax,r_bx,r_cx,r_dx,r_si,r_di,r_sp,r_bp;
static unsigned short r_cs,r_es,r_ds,r_ss,r_ip;
static unsigned long r_eax,r_ebx,r_ecx,r_edx,r_esi,r_edi,r_esp,r_ebp;
static short r_al,r_ah,r_bl,r_bh,r_cl,r_ch,r_dl,r_dh;


static int dflag = 0;

/*
 * Temporary variables used by interpreter
 */


static char tmp_8;
static unsigned char tmp_u8;
static short tmp_16;
static unsigned short tmp_u16;
static long tmp_32;
static unsigned long tmp_u32;
static unsigned long tmp_32l,tmp_32h;

static short disp_16;
static unsigned short disp_u16;
static char  disp_8;

static unsigned char *imp;			/* memory pointer for instructions */
static unsigned char *dmp_8,*s_dmp_8;		/* memory pointer for data references using DS: */
static unsigned short *dmp_16,*s_dmp_16;		/* memory pointer for data references using DS: */
static unsigned short *smp;		/* memory pointer for stack references */
static unsigned long  *lsmp;		/* memory pointer for stack references (32 bit)*/
static unsigned char mod_byte;
static unsigned long *dmp_32,*s_dmp_32;

static unsigned short new_seg,tmp_seg,seg_ovr;

static short sign;					/* Sign bit */
static int rep_flag, opsize;
static unsigned char opcode;
unsigned char *base_mem;

int decode_386( void );	/*	routine to handle extended opcodes */

static int PerformIO( unsigned char IOopcode, unsigned char IOsize );	/*	handle all I/O	*/
static int V86CMOSEmulate(unsigned char op, unsigned short addr);
static int V86PITEmulate(unsigned char IOopcode, unsigned short port);

#define	MAX_IO_ADDR		0xFFFF			/*	S001	vvv	*/
#define	IO_BITMAP_SIZE		8192

static unsigned char		*io_bitmap = (unsigned char *) NULL;

#define	IOEnabled(addr, mask)	( ! ( *(unsigned short *)(io_bitmap+(addr>>3)) \
				   & (mask << (addr & 7)) ) )

#define	IO_BYTE_MASK		0x01
#define	IO_WORD_MASK		0x03
#define IO_DOUBLEWORD_MASK	0x0f		/*	S001	^^^	*/

			/*	to simulate VerticalRetrace signal	*/
static short	VerticalRetraceSignal=0x08;
#define	VerticalRetraceMask0	0xf7
#define	VerticalRetraceMask1	0x08

static int OptionFlags;

#define IOPRINT	(OptionFlags & OPT_IOPRINT)
#define IOERROR	(OptionFlags & OPT_IOERROR)
#define IOTRACE	(OptionFlags & OPT_IOTRACE)
#define Debug	(OptionFlags & OPT_DEBUG)
#define INTERP_DEBUG (OptionFlags & OPT_INTERP_DEBUG)

static void RegisterDisplay() {
	extern char * Disassemble( unsigned char * );
	register unsigned long r0, r1, r2;

	r0 = (r_eax & 0xffff0000) | (r_ax & 0xffff);
	r1 = (r_ebx & 0xffff0000) | (r_bx & 0xffff);
	r2 = (r_ecx & 0xffff0000) | (r_cx & 0xffff);
  ErrorF("%%eax\t%#010x\t%%ebx\t%#010x\t%%ecx\t%#010x %%ds %#06x\n", r0, r1, r2, r_ds);
	r0 = (r_edx & 0xffff0000) | (r_dx & 0xffff);
	r1 = (r_esi & 0xffff0000) | (r_si & 0xffff);
	r2 = (r_edi & 0xffff0000) | (r_di & 0xffff);
  ErrorF("%%edx\t%#010x\t%%esi\t%#010x\t%%edi\t%#010x %%es %#06x\n", r0, r1, r2, r_es);
	r0 = (r_ebp & 0xffff0000) | (r_bp & 0xffff);
	r1 = (r_esp & 0xffff0000) | (r_sp & 0xffff);
  ErrorF("%%ebp\t%#010x\t%%esp\t%#010x\t%%ss\t%#06x\n", r0, r1, r_ss );
  ErrorF("%%eflags\t%#010x -> ", r_fl );
  if ( OF ) ErrorF( "(OF:1 "); else ErrorF( "(OF:0 ");
  if ( DF ) ErrorF( "DF:1 "); else ErrorF( "DF:0 ");
  if ( IF ) ErrorF( "IF:1 "); else ErrorF( "IF:0 ");
  if ( TF ) ErrorF( "TF:1) "); else ErrorF( "TF:0) ");
  if ( SF ) ErrorF( "(SF:1 "); else ErrorF( "(SF:0 ");
  if ( ZF ) ErrorF( "ZF:1 "); else ErrorF( "ZF:0 ");
  if ( AF ) ErrorF( "- AF:1) "); else ErrorF( "- AF:0) ");
  if ( PF ) ErrorF( "(- PF:1 "); else ErrorF( "(- PF:0 ");
  if ( CF ) ErrorF( "- CF:1)\n"); else ErrorF( "- CF:0)\n");
  ErrorF("%%eip %#010x (%04x:%04x) = %s\n", imp, r_cs, GET_IP(), Disassemble( imp ) );
}

/*
 * This routine decodes the 8086/286 instruction stream emulating the instructions.  It returns to caller upon 
 * two events. 
 *
 * 1> Invalid instruction
 * 2> A RET, IRET instruction that pops to end of stack (ie, return value on stack is 0xFFFF:0xFFFF)
 *
 * An SS:SP value can be passed in and that value will be used.  If SS:SP is 0:0 , then a value will be chosen
 * that will be okay for most instances.  An 8k stack will be created at 0:3400 (0:11k)
 *
 * INPUT: CS:IP
 * RETURN: VOID
 */
void decode_instructions(int Options, struct REGS *s_regs,
		struct REGS *d_regs)
{
	OptionFlags = Options;

#ifdef	DEBUG_INTERP

	instrs_counted = 0;

	if( Debug)
		ErrorF("decode: OptionFlags %#x\n", OptionFlags );

	StartTimer( instrs_counted );
#endif	/*	DEBUG_INTERP	*/

	VerticalRetraceSignal=0x08;

	if( Debug)
		ErrorF("decode_instr: starting imp = %#x\n", imp );

	/*
	 * Registers used by interpreter 
	 */

	/* Build the initial stack frame */
	if (s_regs->Ss == 0 && s_regs->Sp == 0) {
		r_sp = 0x33fe;
		r_ss = 0x0000;
	}
	else {
		r_sp = s_regs->Sp;
		r_ss = s_regs->Ss;
	}
	smp = SET_NEW_SMP();
	*smp = s_regs->Flags;
	r_sp -= 2;
	smp = SET_NEW_SMP();
	*smp = 0xffff;
	r_sp -= 2;
	smp = SET_NEW_SMP();
	*smp = 0xffff;


	/* Set up initial register values */

	r_ax = s_regs->Ax;
	r_bx = s_regs->Bx;
	r_cx = s_regs->Cx;
	r_dx = s_regs->Dx;
	r_si = s_regs->Si;
	r_di = s_regs->Di;
	r_bp = s_regs->Bp;
	r_ds = s_regs->Ds;
	r_es = s_regs->Es;
	r_ip = s_regs->Ip;
	r_cs = s_regs->Cs;

	imp = SET_NEW_IMP(s_regs->Cs,s_regs->Ip);
	new_seg = rep_flag = opsize = 0;
	SET_ALL_BREGS();

top_loop:

#ifdef DEBUG_INTERP
	++instrs_counted;
#endif

	if( INTERP_DEBUG )
		RegisterDisplay();

	switch ((opcode = *imp++)) {		/* main decode loop */
		/*	Cases to complete this switch statement to
		 *	256 cases so the compiler can better optimize
		 *	this switch statement.  These should never happen.
		 *	The ones that appear to be actual codes are really
		 *	sub-functions of other opcodes.
		 */
		case 0x1c:		/*	28 - SBB AL,imm	*/
		case 0x1d:		/*	29 - SBB eAX,imm	*/
		case 0x63:		/*	99 - nocode	*/
		case 0x64:		/*	100 - nocode	*/
		case 0x67:		/*	103 - nocode	*/
		case 0x69:		/*	105 - nocode	*/
		case 0x6b:		/*	107 - nocode	*/
		case 0x6c:		/*	108 - nocode	*/
		case 0x6d:		/*	109 - nocode	*/
		case 0x6e:		/*	110 - nocode	*/
		case 0x6f:		/*	111 - nocode	*/
		case 0x71:		/*	113 - JNO	*/
		case 0xd6:		/*	214 - nocode	*/
		case 0xf1:		/*	241 - nocode	*/
	ErrorF("decode_instructions: Unknown opcode %02x at %04x:%04x\n",opcode,r_cs,(GET_IP()-1));
			return;				/*	S000	*/
			break;

		case AAA:	if ((r_al & 0x0f) > 9 || AF == 1) {
						r_al = (r_al + 6) & 0x0f;
						r_ah++;
						AF = 1; CF = 1;
					}
					else {
						CF = 0; AF = 0;
					}
					SET_AX();
					break;

		case AAD:	r_al += (r_ah * (*imp++));		/* technically *imp should be 10, but some programmers stuff other things */
													/* in there to do cheap immediate multiplies! */
					r_ah = 0;
					SET_SF_8(r_al); SET_AX(); SET_ZF_PF(r_ax);
					break;

		case AAM:	r_ah = r_al / (*imp);			/* should be 10, but see comment above */
					r_al %=  *imp++;				/* same as above */
					SET_AX(); SET_SF_8(r_al); SET_ZF_PF(r_ax);
					break;

		case AAS:	if ((r_al & 0x0f) > 9 || AF == 1) {
						r_al = (r_al - 6) & 0x0f;
						r_ah--;
						AF = 1; CF = 1;
					}
					else {
						CF = AF = 0;
					}
					SET_AX();
					break;

		case ADC_AL:	sign = r_al & 0x80;
						r_al += *(char *)imp++;				/* Add with Carry, immediate AL */
						r_al += CF;
						SET_OF_SF_8(sign,r_al); SET_ZF_PF_8(r_al); SET_CF_8(r_al);
						SET_AX();
						break;

		case ADC_AX:	sign = r_ax & 0x8000;
						r_ax += *(short *)imp;				/* Add with Carry, immediate AX */
						imp += 2;
						r_ax += CF;
						SET_OF_SF(sign,r_ax); SET_ZF_PF(r_ax); SET_CF(r_ax);
						SET_AL_AH(); TRUNC_16(r_ax);
						break;

		case ADD_AL:	sign = r_al & 0x80;
						r_al += *(char *)imp++;				/* Add with Carry, immediate AL */
						SET_OF_SF_8(sign,r_al); SET_ZF_PF_8(r_al); SET_CF_8(r_al);
						SET_AX();
						break;

		case ADD_AX:	sign = r_ax & 0x8000;
						r_ax += *(short *)imp;				/* Add with Carry, immediate AX */
						imp += 2;
						SET_OF_SF(sign,r_ax); SET_ZF_PF(r_ax); SET_CF(r_ax);
						SET_AL_AH(); TRUNC_16(r_ax);
						break;

		case AND_AL:	r_al &= *imp++;
						SET_SF_8(r_al); SET_ZF_PF_8(r_al);
						CF = OF = 0; SET_AX();
						break;

		case AND_AX:	r_ax &= *(unsigned short *)imp;
						imp+=2;
						SET_SF(r_ax); SET_ZF_PF(r_ax);
						SET_AL_AH(); TRUNC_16(r_ax);
						CF = OF = 0;
						break;


		case BOUND:		imp += 4;
				ErrorF("Unsupported instruction BOUND\n");
						break;

		case CALL_FAR:	smp = SET_NEW_SMP();
						*--smp = r_cs;
						*--smp = GET_IP();			/* push the stack segment and  */
						r_sp -= 4;					/* Adjust the stack */
						r_ip = *(unsigned short *)imp++;
						r_cs = *(unsigned short *)imp++;
						imp = SET_NEW_IMP(r_cs,r_ip);		/* execution will resume here! */
						smp = SET_NEW_SMP();
						break;

		case CALL:		smp = SET_NEW_SMP();
						disp_u16 = *(unsigned short *)imp;
						imp+=2;
						r_ip = GET_IP();
						*--smp = r_ip;
						r_sp -= 2;
						r_ip += disp_u16;
						imp = SET_NEW_IMP(r_cs,r_ip);		/* execution will resume here ! */
						smp = SET_NEW_SMP();
						break;

		case CBW:		if (r_al & 0x80)				/* Convert byte to word, sign extend al into ah */
							r_ah = 0xff;
						else
							r_ah = 0;
						SET_AX();
						break;

		case CLC:		CF = 0;							/* Clear carry flag */
						break;

		case CLD:		DF = 0;							/* Clear direction flag */
						break;

		case CLI:		IF = 0;							/* For completeness, Clear Interrupt flag, NOT USED! */
						break;

		case CMC:		CF = !CF;					/* Complement Carry Flag */
						break;

		case CMP_AL:	tmp_16 = r_al - *(char *)imp++;
						sign = r_al & 0x80;
						SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
						break;

		case CMP_AX:	tmp_32 = r_ax - *(short *)imp;
						imp+=2;
						sign = r_ax & 0x8000;
						SET_OF_SF(sign,tmp_32); SET_ZF_PF(tmp_32); SET_CF(tmp_32);
						break;

		case CWD:		if (r_ax & 0x8000)			/* Sign extend AX into DX */
							r_dx = 0xffff;
						else
							r_dx = 0;
						SET_DL_DH();
						break;


		case DAA:		if ((r_al & 0x0f) > 9 || AF == 1) {		/* Decimal adjust AL after addition */
							r_al += 6;
							AF = 1;
						}
						else {
							AF = 0;
						}
						SET_CF_8(r_al);
						if (r_al > 0x9f || CF == 1) {
							r_al += 0x60;
							CF = 1;
						}
						else {
							CF = 0;
						}
						SET_SF_8(r_al); SET_ZF_PF_8(r_al); SET_AX();
						break;

		case DAS:		if ((r_al & 0x0f) > 9 || AF == 1) {		/* Decimal adjust AL after Subtraction */
							r_al -= 6;
							AF = 1;
						}
						else {
							AF = 0;
						}
						SET_CF_8(r_al);
						if (r_al > 0x9f || CF == 1) {
							r_al -= 0x60;
							CF = 1;
						}
						else {
								CF = 0;
						}
						SET_SF_8(r_al); SET_ZF_PF_8(r_al); SET_AX();
						break;

		case DEC_AX:	sign = r_ax & 0x8000;
						r_ax--;
						SET_AL_AH(); SET_ZF_PF(r_ax); SET_OF_SF(sign,r_ax); TRUNC_16(r_ax);
						break;
							
		case DEC_BX:	sign = r_bx & 0x8000;
						r_bx--;
						SET_BL_BH(); SET_ZF_PF(r_bx); SET_OF_SF(sign,r_bx); TRUNC_16(r_bx);
						break;
							
		case DEC_CX:	sign = r_cx & 0x8000;
						r_cx--;
						SET_CL_CH(); SET_ZF_PF(r_cx); SET_OF_SF(sign,r_cx); TRUNC_16(r_cx);
						break;
							
		case DEC_DX:	sign = r_dx & 0x8000;
						r_dx--;
						SET_DL_DH(); SET_ZF_PF(r_dx); SET_OF_SF(sign,r_dx); TRUNC_16(r_dx);
						break;

		case DEC_SI:	sign = r_si & 0x8000;
						r_si--;
						SET_ZF_PF(r_si); SET_OF_SF(sign,r_si); TRUNC_16(r_si);
						break;

		case DEC_DI:	sign = r_di & 0x8000;
						r_di--;
						SET_ZF_PF(r_di); SET_OF_SF(sign,r_di); TRUNC_16(r_di);
						break;

		case DEC_SP:	sign = r_sp & 0x8000;
						r_sp--;
						SET_ZF_PF(r_sp); SET_OF_SF(sign,r_sp); TRUNC_16(r_sp);
						smp = SET_NEW_SMP();
						break;
							
		case DEC_BP:	sign = r_bp & 0x8000;
						r_bp--;
						SET_ZF_PF(r_bp); SET_OF_SF(sign,r_bp); TRUNC_16(r_bp);
						break;

		case ENTER:		{
							unsigned short fp;
							int level;

							smp = SET_NEW_SMP();
							tmp_u16 = *(unsigned short *)imp;
							imp+=2;
							level = (*imp++ % 32);
							PUSH_ARG(r_bp);
							fp = r_sp;
							if (level > 0) {
								int i;
								for(i = 1; i <= level - 1; i++) {
									r_bp -= 2;
									PUSH_ARG(r_bp);
								}
								PUSH_ARG(fp);
							}
							r_bp = fp;
							r_sp -= tmp_u16;
							TRUNC_16(r_sp);
							TRUNC_16(r_bp);
							smp = SET_NEW_SMP();
							break;
						}

		case LEAVE:		r_sp = r_bp;
						smp = SET_NEW_SMP();
						POP_ARG(r_bp);
						smp = SET_NEW_SMP();
						break;

		case ESC0:
		case ESC1:
		case ESC2:
		case ESC3:
		case ESC4:
		case ESC5:
		case ESC6:
		case ESC7:		imp++;		/* ignore the mode byte */
						break;

		case HLT:		break;		/* HLT now acts like a nop, since we can't simulate interrupts! */

		case INC_AX:	sign = r_ax & 0x8000;
						r_ax++;
						SET_AL_AH(); SET_ZF_PF(r_ax); SET_OF_SF(sign,r_ax); TRUNC_16(r_ax);
						break;

		case INC_BX:	sign = r_bx & 0x8000;
						r_bx++;
						SET_BL_BH(); SET_ZF_PF(r_bx); SET_OF_SF(sign,r_bx); TRUNC_16(r_bx);
						break;

		case INC_CX:	sign = r_cx & 0x8000;
						r_cx++;
						SET_CL_CH(); SET_ZF_PF(r_cx); SET_OF_SF(sign,r_cx); TRUNC_16(r_cx);
						break;

		case INC_DX:	sign = r_dx & 0x8000;
						r_dx++;
						SET_DL_DH(); SET_ZF_PF(r_dx); SET_OF_SF(sign,r_dx); TRUNC_16(r_dx);
						break;

		case INC_SP:	sign = r_sp & 0x8000;
						r_sp++;
						SET_ZF_PF(r_sp); SET_OF_SF(sign,r_sp); TRUNC_16(r_sp);
						smp = SET_NEW_SMP();
						break;

		case INC_BP:	sign = r_bp & 0x8000;
						r_bp++;
						SET_ZF_PF(r_bp); SET_OF_SF(sign,r_bp); TRUNC_16(r_bp);
						break;

		case INC_SI:	sign = r_si & 0x8000;
						r_si++;
						SET_ZF_PF(r_si); SET_OF_SF(sign,r_si); TRUNC_16(r_si);
						break;

		case INC_DI:	sign = r_di & 0x8000;
						r_di++;
						SET_ZF_PF(r_di); SET_OF_SF(sign,r_di); TRUNC_16(r_di);
						break;

		case INT3:		break;					/* Software break points are not supported in the interpreter */

		case INTX:		tmp_u16 = *(unsigned char *)imp++;
						tmp_u16 *= 2;
						r_sp -= 6;
						smp = SET_NEW_SMP();
						r_ip = GET_IP();
						*(smp+2) = r_fl;
						*(smp+1) = r_cs;	/*	S000	*/
						*smp     = r_ip;	/*	S000	*/
						IF = TF = 0;
						dmp_16 = SET_NEW_DMP(0,0,0,0);
						r_ip = *(dmp_16+tmp_u16);
						r_cs = *(dmp_16+tmp_u16+1);
						imp = SET_NEW_IMP(r_cs,r_ip);			/* Execution will continue here */
						break;

		case INT0:		if (OF) {
							r_sp -= 6;
							smp = SET_NEW_SMP();
							r_ip = GET_IP();
							*(smp+2) = r_fl;
							*(smp+1) = r_ip;
							*smp     = r_cs;
							IF = TF = 0;
							dmp_16 = SET_NEW_DMP(0,0,0,0);
							r_ip = *(dmp_16+8);
							r_cs = *(dmp_16+9);
							imp = SET_NEW_IMP(r_cs,r_ip);			/* Execution will continue here */
						}
						break;

		case IRET:		smp = SET_NEW_SMP();
						r_ip = *(smp);			/*	S000	*/
						r_cs = *(smp+1);			/*	S000	*/
						if (r_ip == 0xffff && r_cs == 0xffff) {
							d_regs->Ax = r_ax;
							d_regs->Bx = r_bx;
							d_regs->Cx = r_cx;
							d_regs->Dx = r_dx;
							d_regs->Si = r_si;
							d_regs->Di = r_di;
							d_regs->Sp = r_sp + 6;
							d_regs->Bp = r_bp;
							d_regs->Cs = r_cs;
							d_regs->Es = r_es;
							d_regs->Ds = r_ds;
							d_regs->Ss = r_ss;
							d_regs->Flags = r_fl;
#ifdef	DEBUG_INTERP
if( Debug) {
	ErrorF("Main: Returning from an IRET\n" );
	StopTimer( instrs_counted );
	ElapsedTime();
}
#endif	/*	DEBUG_INTERP	*/
							return;
						}
						r_fl = *(smp+2);
						r_sp += 6;
						imp = SET_NEW_IMP(r_cs,r_ip);
						smp = SET_NEW_SMP();
						break;

		case JA:		disp_8 = *imp++;
						if (CF == ZF && ZF == 0) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						break;

		case JAE:		disp_8 = *imp++;
						if (CF == 0) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						break;

		case JB:		disp_8 = *imp++;
						if (CF == 1) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						break;

		case JBE:		disp_8 = *imp++;
						if (CF == 1 || ZF == 1) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						break;

		case JCXZ:		disp_8 = *imp++;
						if (r_cx == 0) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						break;

		case JE:		disp_8 = *imp++;
						if (ZF == 1) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						break;

		case JG:		disp_8 = *imp++;
						if (ZF == 0 && (OF == SF)) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						break;

		case JGE:		disp_8 = *imp++;
						if (OF == SF) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						break;

		case JL:		disp_8 = *imp++;
						if (OF != SF) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						break;

		case JLE:		disp_8 = *imp++;
						if (ZF == 1 || (OF != SF)) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						break;

		case JMPFAR:	r_ip = *(unsigned short *)imp;
						imp+=2;
						r_cs = *(unsigned short *)imp;
						imp+=2;
						imp = SET_NEW_IMP(r_cs,r_ip);
						break;

		case JMPSHORT:	disp_8 = *imp++;
						r_ip = GET_IP() + disp_8;
						imp = SET_NEW_IMP(r_cs,r_ip);
						break;

		case JMPREL:	disp_u16 = *(unsigned short *)imp;
						imp+=2;
						r_ip = (GET_IP() + disp_u16) & 0xffff;
						imp = SET_NEW_IMP(r_cs,r_ip);
						break;

		case JNE:		disp_8 = *imp++;
						if (ZF == 0) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						break;

		case JNP:		disp_8 = *imp++;
						if (PF == 0) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						break;

		case JNS:		disp_8 = *imp++;
						if (SF == 0) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						break;

		case JO:		disp_8 = *imp++;
						if (OF == 1) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						break;

		case JP:		disp_8 = *imp++;
						if (PF == 1) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						break;

		case JS:		disp_8 = *imp++;
						if (SF == 1) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						break;

		case LAHF:		r_ah = r_fl;	/* r_fl is a union of a ushort and a bitfield of all flags */
						SET_AX();
						break;

		case LOCK:		break;		/* NOP */

		case LODSB:		if (new_seg) 
							dmp_8 = SET_NEW_DMP(seg_ovr,r_si,0,0);	
						else
							dmp_8 = SET_NEW_DMP(r_ds,0,r_si,0);
						r_al = *dmp_8;
						if (DF)
							r_si--;
						else
							r_si++;
						TRUNC_16(r_si);
						SET_AX();
						break;

		case LODSW:		if (new_seg) 
							dmp_16 = SET_NEW_DMP(seg_ovr,r_si,0,0);	
						else
							dmp_16 = SET_NEW_DMP(r_ds,0,r_si,0);
						r_ax = *dmp_16;
						if (DF)
							r_si -= 2;
						else
							r_si += 2;
						TRUNC_16(r_si);
						SET_AL_AH();
						break;

		case LOOP:		r_cx--;
						TRUNC_16(r_cx);
						disp_8 = *imp++;
						if (r_cx != 0) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						SET_CL_CH();
						break;

		case LOOPZ:		r_cx--;
						TRUNC_16(r_cx);
						disp_8 = *imp++;
						if (r_cx != 0 && ZF == 1) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						SET_CL_CH();
						break;

		case LOOPNZ:	r_cx--;
						TRUNC_16(r_cx);
						disp_8 = *imp++;
						if (r_cx != 0 && ZF == 0) {
							r_ip = GET_IP() + disp_8;
							imp = SET_NEW_IMP(r_cs,r_ip);
						}
						SET_CL_CH();
						break;

		case XOR_IMMAX:		r_ax ^= *(unsigned short *)imp;
							imp+=2;
							SET_SF(r_ax); SET_ZF_PF(r_ax); SET_AL_AH(); TRUNC_16(r_ax);
							OF = CF = 0;
							break;

		case XOR_IMMAL:		r_al ^= *imp++;
							OF = CF = 0;
							SET_AX(); SET_SF_8(r_al); SET_ZF_PF_8(r_al);
							break;

		case OR_IMMAX:		r_ax |= *(unsigned short *)imp;
							imp+=2;
							SET_AL_AH(); SET_SF(r_ax); SET_ZF_PF(r_ax); TRUNC_16(r_ax);
							break;

		case OR_IMMAL:		r_al |= *imp++;
							OF = CF = 0;
							SET_SF_8(r_al); SET_ZF_PF_8(r_al); SET_AX();
							break;

		case MOVAX_MEM:		disp_u16 = *(unsigned short *)imp;
							imp+=2;
							if (new_seg) 
								dmp_16 = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
							else
								dmp_16 = SET_NEW_DMP(r_ds,disp_u16,0,0);
							r_ax = *dmp_16;
							SET_AL_AH();
							break;

		case MOVAL_MEM:		disp_u16 = *(unsigned short *)imp;
							imp+=2;
							if (new_seg) 
								dmp_8 = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
							else
								dmp_8 = SET_NEW_DMP(r_ds,disp_u16,0,0);
							r_al = *dmp_8;
							SET_AX();
							break;

		case MOVMEM_AX:		disp_u16 = *(unsigned short *)imp;
							imp+=2;
							if (new_seg) 
								dmp_16 = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
							else
								dmp_16 = SET_NEW_DMP(r_ds,disp_u16,0,0);
							*dmp_16 = (unsigned short)(r_ax & 0xffff);
							break;

		case MOVMEM_AL:		disp_u16 = *(unsigned short *)imp;
							imp+=2;
							if (new_seg) 
								dmp_8 = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
							else
								dmp_8 = SET_NEW_DMP(r_ds,disp_u16,0,0);
							*dmp_8 = (unsigned char)(r_al & 0xff);
							break;


		case MOVAX_IM:		r_ax = *(unsigned short *)imp;
							imp+=2;
							SET_AL_AH();
							break;

		case MOVBX_IM:		r_bx = *(unsigned short *)imp;
							imp+=2;
							SET_BL_BH();
							break;

		case MOVCX_IM:		r_cx = *(unsigned short *)imp;
							imp+=2;
							SET_CL_CH();
							break;

		case MOVDX_IM:		r_dx = *(unsigned short *)imp;
							imp+=2;
							SET_DL_DH();
							break;

		case MOVSP_IM:		r_sp = *(unsigned short *)imp;
							imp+=2;
							smp = SET_NEW_SMP();
							break;

		case MOVBP_IM:		r_bp = *(unsigned short *)imp;
							imp+=2;
							break;

		case MOVSI_IM:		r_si = *(unsigned short *)imp;
							imp+=2;
							break;

		case MOVDI_IM:		r_di = *(unsigned short *)imp;
							imp+=2;
							break;

		case MOVAL_IM:		r_al = *(unsigned char *)imp++;
							SET_AX();
							break;

		case MOVAH_IM:		r_ah = *(unsigned char *)imp++;
							SET_AX();
							break;

		case MOVBL_IM:		r_bl = *(unsigned char *)imp++;
							SET_BX();
							break;

		case MOVBH_IM:		r_bh = *(unsigned char *)imp++;
							SET_BX();
							break;

		case MOVCL_IM:		r_cl = *(unsigned char *)imp++;
							SET_CX();
							break;

		case MOVCH_IM:		r_ch = *(unsigned char *)imp++;
							SET_CX();
							break;

		case MOVDL_IM:		r_dl = *(unsigned char *)imp++;
							SET_DX();
							break;

		case MOVDH_IM:		r_dh = *(unsigned char *)imp++;
							SET_DX();
							break;

		case MOVSB:			dmp_8 = SET_NEW_DMP(r_es,0,r_di,0);
							if (new_seg)
								tmp_seg = seg_ovr;
							else
								tmp_seg = r_ds;
							s_dmp_8 = SET_NEW_DMP(tmp_seg,0,r_si,0);
							if (rep_flag) {
								while(r_cx--) {
									*dmp_8 = *s_dmp_8;
									if (DF) {
										r_di--;
										r_si--;
									}
									else {
										r_di++;
										r_si++;
									}
									TRUNC_16(r_di);
									TRUNC_16(r_si);
									dmp_8 = SET_NEW_DMP(r_es,0,r_di,0);
									s_dmp_8 = SET_NEW_DMP(tmp_seg,0,r_si,0);
								}
								r_cx = 0;
								SET_CL_CH();
							}
							else {
								*dmp_8 = *s_dmp_8;
								if (DF) {
									r_di--;
									r_si--;
								}
								else {
									r_di++;
									r_si++;
								}
								TRUNC_16(r_di);
								TRUNC_16(r_si);
							}
							break;

		case MOVSW:			dmp_16 = SET_NEW_DMP(r_es,0,r_di,0);
							if (new_seg)
								tmp_seg = seg_ovr;
							else
								tmp_seg = r_ds;
							s_dmp_16 = SET_NEW_DMP(tmp_seg,0,r_si,0);
							if (rep_flag) {
								while(r_cx--) {
									*dmp_16 = *s_dmp_16;
									if (DF) {
										r_di -= 2;
										r_si -= 2;
									}
									else {
										r_di += 2;
										r_si += 2;
									}
									TRUNC_16(r_si);
									TRUNC_16(r_di);
									dmp_16 = SET_NEW_DMP(r_es,0,r_di,0);
									s_dmp_16 = SET_NEW_DMP(tmp_seg,0,r_si,0);
								}
								r_cx = 0;
								SET_CL_CH();
							}
							else {
								*dmp_16 = *s_dmp_16;
								if (DF) {
									r_di -= 2;
									r_si -= 2;
								}
								else {
									r_di += 2;
									r_si += 2;
								}
								TRUNC_16(r_si);
								TRUNC_16(r_di);
							}
							break;

		case NOP:			break;


		case INB_DX:
		case INW_DX:
		case INB:
		case INW:
		case OUTDX_AX:
		case OUTDX_AL:
		case OUTAL:
		case OUTAX:
			if ( PerformIO( opcode, 0 ) ) {
					return;	/*	something is wrong, can't do it	*/
			}
				break;	/*	all done OK	*/

		case PUSH_Ib:  r_sp -= 2;       /* S000 push immediate byte */
			/* Sign extend to 16 bits. */
			smp = SET_NEW_SMP();
			if (*imp & 0x80)
				*smp = 0xFF00 | *imp;
			else
				*smp = *((unsigned short *)imp);
			imp++;
			break;


		case PUSH_Iw:  r_sp -= 2;       /* S000 push immediate word */
			smp = SET_NEW_SMP();
			*smp = *((unsigned short *)imp);
			imp += 2;
			break;

		case PUSHAX:	r_sp -= 2;
						smp = SET_NEW_SMP();
						*smp = r_ax;
						break;

		case PUSHBX:	r_sp -= 2;
						smp = SET_NEW_SMP();
						*smp = r_bx;
						break;

		case PUSHCX:	r_sp -= 2;
						smp = SET_NEW_SMP();
						*smp = r_cx;
						break;

		case PUSHDX:	r_sp -= 2;
						smp = SET_NEW_SMP();
						*smp = r_dx;
						break;

		case PUSHSP:	r_sp -= 2;
						smp = SET_NEW_SMP();
						*smp = r_sp+2;
						break;

		case PUSHBP:	r_sp -= 2;
						smp = SET_NEW_SMP();
						*smp = r_bp;
						break;

		case PUSHSI:	r_sp -= 2;
						smp = SET_NEW_SMP();
						*smp = r_si;
						break;

		case PUSHDI:	r_sp -= 2;
						smp = SET_NEW_SMP();
						*smp = r_di;
						break;

		case PUSHCS:	r_sp -=2;
						smp = SET_NEW_SMP();
						*smp = r_cs;
						break;


		case PUSHDS:	r_sp -=2;
						smp = SET_NEW_SMP();
						*smp = r_ds;
						break;

		case PUSHES:	r_sp -=2;
						smp = SET_NEW_SMP();
						*smp = r_es;
						break;

		case PUSHSS:	r_sp -=2;
						smp = SET_NEW_SMP();
						*smp = r_ss;
						break;

		case PUSHFL:	r_sp -=2;
						smp = SET_NEW_SMP();
						*smp = r_fl;
						break;

		case PUSHALL:	tmp_u16 = r_sp;								/* NEED TO ADD opsize override */
						r_sp -=16;
						smp = SET_NEW_IMP(r_ss,(tmp_u16 - 2));
						*smp = r_ax;
						*(smp-1) = r_cx;
						*(smp-2) = r_dx;
						*(smp-3) = r_bx;
						*(smp-4) = tmp_u16;
						*(smp-5) = r_bp;
						*(smp-6) = r_si;
						*(smp-7) = r_di;
						break;

		case POPAX:		r_ax = *smp;
						r_sp +=2;
						SET_AL_AH();
						smp = SET_NEW_SMP();
						break;

		case POPBX:		r_bx = *smp;
						r_sp +=2;
						SET_BL_BH();
						smp = SET_NEW_SMP();
						break;

		case POPCX:		r_cx = *smp;
						r_sp +=2;
						SET_CL_CH();
						smp = SET_NEW_SMP();
						break;

		case POPDX:		r_dx = *smp;
						r_sp +=2;
						SET_DL_DH();
						smp = SET_NEW_SMP();
						break;

		case POPSI:		r_si = *smp;
						r_sp +=2;
						smp = SET_NEW_SMP();
						break;

		case POPDI:		r_di = *smp;
						r_sp +=2;
						smp = SET_NEW_SMP();
						break;

		case POPBP:		r_bp = *smp;
						r_sp +=2;
						smp = SET_NEW_SMP();
						break;

		case POPSP:		r_sp = *smp;					/* POP SP does NOT increment the SP reg ! */
						smp = SET_NEW_SMP();
						break;

		case POPDS:		r_ds = *smp;
						r_sp +=2;
						smp = SET_NEW_SMP();
						break;

		case POPES:		r_es = *smp;
						r_sp +=2;
						smp = SET_NEW_SMP();
						break;

		case POPSS:		r_ss = *smp;
						r_sp +=2;
						smp = SET_NEW_SMP();
						break;

		case POPFL:		r_fl = *smp;
						r_sp +=2;
						smp = SET_NEW_SMP();
						break;

		case POPALL:	smp = SET_NEW_SMP();			/* ADD Opsize override here */
						r_di = *smp;
						r_si = *(smp+1);
						r_bp = *(smp+2);
						r_bx = *(smp+4);
						r_dx = *(smp+5);
						r_cx = *(smp+6);
						r_ax = *(smp+7);
						r_sp += 16;
						smp = SET_NEW_SMP();
						SET_ALL_BREGS();
						break;

		case RETF:		smp = SET_NEW_SMP();
						r_ip = *smp;
						r_cs = *(smp+1);
						r_sp += 4;
						if (r_ip == 0xffff && r_cs == 0xffff) {
							d_regs->Ax = r_ax;
							d_regs->Bx = r_bx;
							d_regs->Cx = r_cx;
							d_regs->Dx = r_dx;
							d_regs->Si = r_si;
							d_regs->Di = r_di;
							d_regs->Sp = r_sp + 4;
							d_regs->Bp = r_bp;
							d_regs->Cs = r_cs;
							d_regs->Es = r_es;
							d_regs->Ds = r_ds;
							d_regs->Ss = r_ss;
							d_regs->Flags = r_fl;
#ifdef	DEBUG_INTERP
if( Debug) {
	ErrorF("Main: Returning from an RETF\n" );
	StopTimer( instrs_counted );
	ElapsedTime();
}
#endif	/*	DEBUG_INTERP	*/
							return;
						}
						imp = SET_NEW_IMP(r_cs,r_ip);
						smp = SET_NEW_SMP();
						break;

		case RET:		smp = SET_NEW_SMP();
						r_ip = *smp;
						r_sp += 2;
						imp = SET_NEW_IMP(r_cs,r_ip);
						smp = SET_NEW_SMP();
						break;

		case RETF_DISP:		smp = SET_NEW_SMP();
							r_ip = *smp;
							r_cs = *(smp+1);
							r_sp += 4;
							r_sp += *(unsigned short *)imp;
							smp = SET_NEW_SMP();
							imp = SET_NEW_IMP(r_cs,r_ip);
							break;

		case RET_DISP:	smp = SET_NEW_SMP();
						r_ip = *smp;
						r_sp += 2;
						r_sp += *(unsigned short *)imp;
						smp = SET_NEW_SMP();
						imp = SET_NEW_IMP(r_cs,r_ip);
						break;

		case SAHF:		r_fl = (r_fl & 0xff00) | (r_ah & 0xff);
						break;

		case STC:		CF = 1;
						break;

		case STD:		DF = 1;
						break;

		case STI:		IF = 1;				/* NOT REALLY USED */
						break;


		case STOSB:		dmp_8 = SET_NEW_DMP(r_es,0,r_di,0);
						if (rep_flag) {
							while(r_cx--) {
								*dmp_8 = r_al;
								if (DF)
									r_di--;
								else
									r_di++;
								TRUNC_16(r_di);
								dmp_8 = SET_NEW_DMP(r_es,0,r_di,0);
							}
							r_cx = 0;
							SET_CL_CH();
						}
						else  {
							*dmp_8 = r_al;
							if (DF) 
								r_di--;
							else
								r_di++;
							TRUNC_16(r_di);
						}
						break;

		case STOSW:		dmp_16 = SET_NEW_DMP(r_es,0,r_di,0);
						if (rep_flag) {
							while(r_cx--) {
								*dmp_16 = (r_ax & 0xffff);
								if (DF)
									r_di -= 2;
								else
									r_di += 2;
								TRUNC_16(r_di);
								dmp_16 = SET_NEW_DMP(r_es,0,r_di,0);
							}
							r_cx = 0;
							SET_CL_CH();
						}
						else  {
							*dmp_16 = (r_ax & 0xffff);
							if (DF)
								r_di -= 2;
							else
								r_di += 2;
							TRUNC_16(r_di);
						}
						break;

		case SUB_AL:	sign = r_al & 0x80;
						r_al -= *(char *)imp++;						/* Double check this code for sign problems */
						SET_OF_SF_8(sign,r_al); SET_ZF_PF_8(r_al); SET_CF_8(r_al); SET_AX();
						break;

		case SUB_AX:	sign = r_ax & 0x8000;
						r_ax -= *(short *)imp;
						imp+=2;
						SET_AL_AH(); SET_OF_SF(sign,r_ax); SET_ZF_PF(r_ax); SET_CF(r_ax); TRUNC_16(r_ax);
						break;

		case TEST_AL:	tmp_u8 = r_al & *imp++;
						SET_ZF_PF_8(tmp_u8); SET_SF_8(tmp_u8);
						CF = OF = 0;
						break;

		case TEST_AX:	tmp_u16 = r_ax & *(short *)imp;
						imp+=2;
						SET_ZF_PF(tmp_u16); SET_SF(tmp_u16);
						CF = OF = 0;
						break;


		case WAIT:		break;						/* Not used */

		case XCHGBX:	SWAP(r_ax,r_bx);
						SET_BL_BH();
						SET_AL_AH();
						break;

		case XCHGCX:	SWAP(r_ax,r_cx);
						SET_AL_AH();
						SET_CL_CH();
						break;

		case XCHGDX:	SWAP(r_ax,r_dx);
						SET_AL_AH();
						SET_DL_DH();
						break;

		case XCHGSP:	SWAP(r_ax,r_sp);
						SET_AL_AH();
						smp = SET_NEW_SMP();
						break;

		case XCHGBP:	SWAP(r_ax,r_bp);
						SET_AL_AH();
						break;

		case XCHGSI:	SWAP(r_ax,r_si);
						SET_AL_AH();
						break;

		case XCHGDI:	SWAP(r_ax,r_di);
						SET_AL_AH();
						break;

		case XLAT:		if (new_seg)
							dmp_8 = SET_NEW_DMP(seg_ovr,r_bx,r_al,0);
						else
							dmp_8 = SET_NEW_DMP(r_ds,r_bx,r_al,0);

						r_al = *dmp_8;
						SET_AX();
						break;

		case SEGOVRCS:	new_seg = 1;
						seg_ovr = r_cs;
						goto top_loop;		/* NO BREAK HERE!  PROCESS NEXT INSTRUCTION, OTHERWISE new_seg gets reset! */
						break;		/* never get here */

		case SEGOVRDS:	new_seg = 1;
						seg_ovr = r_ds;
						goto top_loop;		/* NO BREAK HERE!  PROCESS NEXT INSTRUCTION, OTHERWISE new_seg gets reset! */
						break;		/* never get here */

		case SEGOVRES:	new_seg = 1;
						seg_ovr = r_es;
						goto top_loop;		/* NO BREAK HERE!  PROCESS NEXT INSTRUCTION, OTHERWISE new_seg gets reset! */
						break;		/* never get here */

		case SEGOVRSS:	new_seg = 1;
						seg_ovr = r_ss;
						goto top_loop;		/* NO BREAK HERE!  PROCESS NEXT INSTRUCTION, OTHERWISE new_seg gets reset! */
						break;		/* never get here */

		case SCASB:		dmp_8 = SET_NEW_DMP(r_es,0,r_di,0);
						if (rep_flag) {
							while(r_cx--) {
								tmp_16 = r_al - *(char *)dmp_8;
								sign = r_al & 0x80;
								SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
								if (DF)
									r_di--;
								else
									r_di++;
								TRUNC_16(r_di);
								if (rep_flag == REPZ && ZF) {
									SET_CL_CH();
									break;			/* Break out of loop */
								}
								else if (rep_flag == REPNZ && ZF == 0) {
									SET_CL_CH();
									break;			/* Break out of loop */
								}
								dmp_8 = SET_NEW_DMP(r_es,0,r_di,0);
							}
							r_cx = 0;
							SET_CL_CH();
						}
						else {
							tmp_16 = r_al - *(char *)dmp_8;
							sign = r_al & 0x80;
							SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
							if (DF)
								r_di--;
							else
								r_di++;
							TRUNC_16(r_di);
						}
						break;

		case SCASW:		dmp_16 = SET_NEW_DMP(r_es,0,r_di,0);
						if (rep_flag) {
							while(r_cx--) {
								tmp_32 = r_ax - *(short *)dmp_16;
								sign = r_ax & 0x8000;
								SET_OF_SF(sign,tmp_32); SET_ZF_PF(tmp_32); SET_CF(tmp_32);
								if (DF)
									r_di -= 2;
								else
									r_di += 2;
								TRUNC_16(r_di);
								if (rep_flag == REPZ && ZF) {
									SET_CL_CH();
									break;			/* Break out of loop */
								}
								else if (rep_flag == REPNZ && ZF == 0) {
									SET_CL_CH();
									break;			/* Break out of loop */
								}
								dmp_16 = SET_NEW_DMP(r_es,0,r_di,0);
							}
							r_cx = 0;
							SET_CL_CH();
						}
						else {
							tmp_32 = r_ax - *(short *)dmp_16;
							sign = r_ax & 0x8000;
							SET_OF_SF(sign,tmp_32); SET_ZF_PF(tmp_32); SET_CF(tmp_32);
							if (DF)
								r_di -= 2;
							else
								r_di += 2;
							TRUNC_16(r_di);
						}
						break;

		case CMPSB:		dmp_8 = SET_NEW_DMP(r_es,0,r_di,0);
						if (new_seg)
							tmp_seg = seg_ovr;
						else
							tmp_seg = r_ds;
						s_dmp_8 = SET_NEW_DMP(tmp_seg,0,r_si,0);
						if (rep_flag) {
							while(r_cx--) {
								tmp_16 = *(char *)s_dmp_8 - *(char *)dmp_8;
								sign = *s_dmp_8 & 0x80;
								SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
								if (DF) {
									r_di--;
									r_si--;
								}
								else {
									r_di++;
									r_si++;
								}
								TRUNC_16(r_si); TRUNC_16(r_di);
								if (rep_flag == REPZ && ZF) {
									SET_CL_CH();
									break;			/* Break out of loop */
								}
								else if (rep_flag == REPNZ && ZF == 0) {
									SET_CL_CH();
									break;
								}
								dmp_8 = SET_NEW_DMP(r_es,0,r_di,0);
								s_dmp_8 = SET_NEW_DMP(tmp_seg,0,r_si,0);
							}
							r_cx = 0;
							SET_CL_CH();
						}
						else {
							tmp_16 = *(char *)s_dmp_8 - *(char *)dmp_8;
							sign = *s_dmp_8 & 0x80;
							SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
							if (DF) {
								r_di--;
								r_si--;
							}
							else {
								r_di++;
								r_si++;
							}
							TRUNC_16(r_si);
							TRUNC_16(r_di);
						}
						break;

		case CMPSW:		dmp_16 = SET_NEW_DMP(r_es,0,r_di,0);
						if (new_seg)
							tmp_seg = seg_ovr;
						else
							tmp_seg = r_ds;
						s_dmp_16 = SET_NEW_DMP(tmp_seg,0,r_si,0);
						if (rep_flag) {
							while(r_cx--) {
								tmp_32 = *(short *)s_dmp_16 - *(short *)dmp_16;
								sign = *s_dmp_16 & 0x8000;
								SET_OF_SF(sign,tmp_32); SET_ZF_PF(tmp_32); SET_CF(tmp_32);
								if (DF) {
									r_di -= 2;
									r_si -= 2;
								}
								else {
									r_di += 2;
									r_si += 2;
								}
								TRUNC_16(r_si);
								TRUNC_16(r_di);
								if (rep_flag == REPZ && ZF) {
									SET_CL_CH();
									break;			/* Break out of loop */
								}
								else {
									if (rep_flag == REPNZ && ZF == 0) {
										SET_CL_CH();
										break;
									}
								}
								dmp_16 = SET_NEW_DMP(r_es,0,r_di,0);
								s_dmp_16 = SET_NEW_DMP(tmp_seg,0,r_si,0);
							}
							r_cx = 0;
							SET_CL_CH();
						}
						else {
							tmp_32 = *(short *)s_dmp_16 - *(short *)dmp_16;
							sign = *s_dmp_16 & 0x8000;
							SET_OF_SF(sign,tmp_32); SET_ZF_PF(tmp_32); SET_CF(tmp_32);
							if (DF) {
								r_di -= 2;
								r_si -= 2;
							}
							else {
								r_di += 2;
								r_si += 2;
							}
							TRUNC_16(r_si);
							TRUNC_16(r_di);
						}
						break;

		case REPZ:		rep_flag = REPZ;
						goto top_loop;
						break;

		case REPNZ:		rep_flag = REPNZ;
						goto top_loop;
						break;

	case 0x86:
	case 0x87: {
		short word_op,source_reg,dest_reg,mod;
		void *dst_mp,*src_mp;

		mod_byte = *imp++;
		word_op =(opcode & 0x01);
		source_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		dest_reg = (mod_byte >> 3) & 0x07;		/* Get Destination register */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is a REGISTER to REGISTER Transfer */
				if (word_op) {
					switch(dest_reg) {
						case REG_AX:	src_mp = &r_ax;
										break;
						case REG_BX:	src_mp = &r_bx;
										break;
						case REG_CX:	src_mp = &r_cx;
										break;
						case REG_DX:	src_mp = &r_dx;
										break;
						case REG_SP:	src_mp = &r_sp;
										break;
						case REG_BP:	src_mp = &r_bp;
										break;
						case REG_SI:	src_mp = &r_si;
										break;
						case REG_DI:	src_mp = &r_di;
										break;
					}

					switch(source_reg) {
						case REG_AX:	dst_mp = &r_ax;
										break;
						case REG_BX:	dst_mp = &r_bx;
										break;
						case REG_CX:	dst_mp = &r_cx;
										break;
						case REG_DX:	dst_mp = &r_dx;
										break;
						case REG_SP:	dst_mp = &r_sp;
										break;
						case REG_BP:	dst_mp = &r_bp;
										break;
						case REG_SI:	dst_mp = &r_si;
										break;
						case REG_DI:	dst_mp = &r_di;
										break;
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	src_mp = &r_al;
										break;
						case REG_BL:	src_mp = &r_bl;
										break;
						case REG_CL:	src_mp = &r_cl;
										break;
						case REG_DL:	src_mp = &r_dl;
										break;
						case REG_AH:	src_mp = &r_ah;
										break;
						case REG_BH:	src_mp = &r_bh;
										break;
						case REG_CH:	src_mp = &r_ch;
										break;
						case REG_DH:	src_mp = &r_dh;
										break;
					}

					switch(source_reg) {
						case REG_AL:	dst_mp = &r_al;
										break;
						case REG_BL:	dst_mp = &r_bl;
										break;
						case REG_CL:	dst_mp = &r_cl;
										break;
						case REG_DL:	dst_mp = &r_dl;
										break;
						case REG_AH:	dst_mp = &r_ah;
										break;
						case REG_BH:	dst_mp = &r_bh;
										break;
						case REG_CH:	dst_mp = &r_ch;
										break;
						case REG_DH:	dst_mp = &r_dh;
										break;
					}
				}
				break;


			case 00:
				switch(source_reg) {			/* r/m */
					case 000:	if (new_seg) 
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									src_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									src_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									src_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									src_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									src_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				if (word_op) {
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_ax;
										break;
						case REG_BX:	dst_mp = &r_bx;
										break;
						case REG_CX:	dst_mp = &r_cx;
										break;
						case REG_DX:	dst_mp = &r_dx;
										break;
						case REG_SP:	dst_mp = &r_sp;
										break;
						case REG_BP:	dst_mp = &r_bp;
										break;
						case REG_SI:	dst_mp = &r_si;
										break;
						case REG_DI:	dst_mp = &r_di;
										break;
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	src_mp = &r_al;
										break;
						case REG_BL:	src_mp = &r_bl;
										break;
						case REG_CL:	src_mp = &r_cl;
										break;
						case REG_DL:	src_mp = &r_dl;
										break;
						case REG_AH:	src_mp = &r_ah;
										break;
						case REG_BH:	src_mp = &r_bh;
										break;
						case REG_CH:	src_mp = &r_ch;
										break;
						case REG_DH:	src_mp = &r_dh;
										break;
					}
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(source_reg) {			/* r/m */
					case 000:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				if (word_op) {
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_ax;
										break;
						case REG_BX:	dst_mp = &r_bx;
										break;
						case REG_CX:	dst_mp = &r_cx;
										break;
						case REG_DX:	dst_mp = &r_dx;
										break;
						case REG_SP:	dst_mp = &r_sp;
										break;
						case REG_BP:	dst_mp = &r_bp;
										break;
						case REG_SI:	dst_mp = &r_si;
										break;
						case REG_DI:	dst_mp = &r_di;
										break;
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	src_mp = &r_al;
										break;
						case REG_BL:	src_mp = &r_bl;
										break;
						case REG_CL:	src_mp = &r_cl;
										break;
						case REG_DL:	src_mp = &r_dl;
										break;
						case REG_AH:	src_mp = &r_ah;
										break;
						case REG_BH:	src_mp = &r_bh;
										break;
						case REG_CH:	src_mp = &r_ch;
										break;
						case REG_DH:	src_mp = &r_dh;
										break;
					}
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(source_reg) {			/* r/m */
					case 000:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
				if (word_op) {
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_ax;
										break;
						case REG_BX:	dst_mp = &r_bx;
										break;
						case REG_CX:	dst_mp = &r_cx;
										break;
						case REG_DX:	dst_mp = &r_dx;
										break;
						case REG_SP:	dst_mp = &r_sp;
										break;
						case REG_BP:	dst_mp = &r_bp;
										break;
						case REG_SI:	dst_mp = &r_si;
										break;
						case REG_DI:	dst_mp = &r_di;
										break;
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	src_mp = &r_al;
										break;
						case REG_BL:	src_mp = &r_bl;
										break;
						case REG_CL:	src_mp = &r_cl;
										break;
						case REG_DL:	src_mp = &r_dl;
										break;
						case REG_AH:	src_mp = &r_ah;
										break;
						case REG_BH:	src_mp = &r_bh;
										break;
						case REG_CH:	src_mp = &r_ch;
										break;
						case REG_DH:	src_mp = &r_dh;
										break;
					}
				}
			break;
		}
		if (word_op) {
			tmp_u16 = *(unsigned short *)dst_mp;
			*(unsigned short *)dst_mp = *(unsigned short *)src_mp;			/* XCHG 16 bit */
			*(unsigned short *)src_mp = tmp_u16;
		}
		else {
			tmp_u8 = *(unsigned char *)dst_mp;
			*(unsigned char *)dst_mp = *(unsigned char *)src_mp;			/* XCHG 8 bit */
			*(unsigned char *)src_mp = tmp_u8;
		}
		if (word_op) {
			smp = SET_NEW_SMP();
			SET_ALL_BREGS();
		}
		else {
			SET_ALL_WREGS();
		}
	}
	break;

	case 0x84:
	case 0x85: {
		short word_op,source_reg,dest_reg,mod;
		void *dst_mp,*src_mp;

		mod_byte = *imp++;
		word_op = (opcode & 0x01);
		source_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		dest_reg = (mod_byte >> 3) & 0x07;		/* Get Destination register */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is a REGISTER to REGISTER Transfer */
				if (word_op) {
					switch(dest_reg) {
						case REG_AX:	src_mp = &r_ax;
										break;
						case REG_BX:	src_mp = &r_bx;
										break;
						case REG_CX:	src_mp = &r_cx;
										break;
						case REG_DX:	src_mp = &r_dx;
										break;
						case REG_SP:	src_mp = &r_sp;
										break;
						case REG_BP:	src_mp = &r_bp;
										break;
						case REG_SI:	src_mp = &r_si;
										break;
						case REG_DI:	src_mp = &r_di;
										break;
					}

					switch(source_reg) {
						case REG_AX:	dst_mp = &r_ax;
										break;
						case REG_BX:	dst_mp = &r_bx;
										break;
						case REG_CX:	dst_mp = &r_cx;
										break;
						case REG_DX:	dst_mp = &r_dx;
										break;
						case REG_SP:	dst_mp = &r_sp;
										break;
						case REG_BP:	dst_mp = &r_bp;
										break;
						case REG_SI:	dst_mp = &r_si;
										break;
						case REG_DI:	dst_mp = &r_di;
										break;
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	src_mp = &r_al;
										break;
						case REG_BL:	src_mp = &r_bl;
										break;
						case REG_CL:	src_mp = &r_cl;
										break;
						case REG_DL:	src_mp = &r_dl;
										break;
						case REG_AH:	src_mp = &r_ah;
										break;
						case REG_BH:	src_mp = &r_bh;
										break;
						case REG_CH:	src_mp = &r_ch;
										break;
						case REG_DH:	src_mp = &r_dh;
										break;
					}

					switch(source_reg) {
						case REG_AL:	dst_mp = &r_al;
										break;
						case REG_BL:	dst_mp = &r_bl;
										break;
						case REG_CL:	dst_mp = &r_cl;
										break;
						case REG_DL:	dst_mp = &r_dl;
										break;
						case REG_AH:	dst_mp = &r_ah;
										break;
						case REG_BH:	dst_mp = &r_bh;
										break;
						case REG_CH:	dst_mp = &r_ch;
										break;
						case REG_DH:	dst_mp = &r_dh;
										break;
					}
				}
				break;


			case 00:
				switch(source_reg) {			/* r/m */
					case 000:	if (new_seg) 
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									src_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									src_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									src_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									src_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									src_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				if (word_op) {
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_ax;
										break;
						case REG_BX:	dst_mp = &r_bx;
										break;
						case REG_CX:	dst_mp = &r_cx;
										break;
						case REG_DX:	dst_mp = &r_dx;
										break;
						case REG_SP:	dst_mp = &r_sp;
										break;
						case REG_BP:	dst_mp = &r_bp;
										break;
						case REG_SI:	dst_mp = &r_si;
										break;
						case REG_DI:	dst_mp = &r_di;
										break;
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	src_mp = &r_al;
										break;
						case REG_BL:	src_mp = &r_bl;
										break;
						case REG_CL:	src_mp = &r_cl;
										break;
						case REG_DL:	src_mp = &r_dl;
										break;
						case REG_AH:	src_mp = &r_ah;
										break;
						case REG_BH:	src_mp = &r_bh;
										break;
						case REG_CH:	src_mp = &r_ch;
										break;
						case REG_DH:	src_mp = &r_dh;
										break;
					}
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(source_reg) {			/* r/m */
					case 000:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				if (word_op) {
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_ax;
										break;
						case REG_BX:	dst_mp = &r_bx;
										break;
						case REG_CX:	dst_mp = &r_cx;
										break;
						case REG_DX:	dst_mp = &r_dx;
										break;
						case REG_SP:	dst_mp = &r_sp;
										break;
						case REG_BP:	dst_mp = &r_bp;
										break;
						case REG_SI:	dst_mp = &r_si;
										break;
						case REG_DI:	dst_mp = &r_di;
										break;
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	src_mp = &r_al;
										break;
						case REG_BL:	src_mp = &r_bl;
										break;
						case REG_CL:	src_mp = &r_cl;
										break;
						case REG_DL:	src_mp = &r_dl;
										break;
						case REG_AH:	src_mp = &r_ah;
										break;
						case REG_BH:	src_mp = &r_bh;
										break;
						case REG_CH:	src_mp = &r_ch;
										break;
						case REG_DH:	src_mp = &r_dh;
										break;
					}
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(source_reg) {			/* r/m */
					case 000:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
				if (word_op) {
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_ax;
										break;
						case REG_BX:	dst_mp = &r_bx;
										break;
						case REG_CX:	dst_mp = &r_cx;
										break;
						case REG_DX:	dst_mp = &r_dx;
										break;
						case REG_SP:	dst_mp = &r_sp;
										break;
						case REG_BP:	dst_mp = &r_bp;
										break;
						case REG_SI:	dst_mp = &r_si;
										break;
						case REG_DI:	dst_mp = &r_di;
										break;
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	src_mp = &r_al;
										break;
						case REG_BL:	src_mp = &r_bl;
										break;
						case REG_CL:	src_mp = &r_cl;
										break;
						case REG_DL:	src_mp = &r_dl;
										break;
						case REG_AH:	src_mp = &r_ah;
										break;
						case REG_BH:	src_mp = &r_bh;
										break;
						case REG_CH:	src_mp = &r_ch;
										break;
						case REG_DH:	src_mp = &r_dh;
										break;
					}
				}
			break;
		}
		if (word_op) {
			tmp_u16 = *(unsigned short *)dst_mp & *(unsigned short *)src_mp;		/* TEST 16 bit */
			SET_ZF_PF(tmp_u16);
			SET_SF(tmp_u16);
			CF = OF = 0;
			SET_ALL_BREGS();
			smp = SET_NEW_SMP();
		}
		else {
			tmp_u8 = *(unsigned char *)dst_mp & *(unsigned char *)src_mp;		/* TEST 8 bit */
			SET_ZF_PF_8(tmp_u8);
			SET_SF_8(tmp_u8);
			CF = OF = 0;
			SET_ALL_WREGS();
		}
	}
	break;

	case 0xD0:
	case 0xD1:
	case 0xD2:
	case 0xD3:
	case 0xC0:
	case 0xC1: {
		short dest_is_reg,dest_reg,subcode,mod,tmp_cf,word_op;
		int count;
		void *dst_mp;

		mod_byte = *imp++;
		dest_is_reg = 0;
		dest_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		subcode = (mod_byte >> 3) & 0x07;
		word_op = (opcode & 0x01);

		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is a REGISTER to REGISTER Transfer */
				dest_is_reg = 1;
				if (opcode & 0x01) {
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_ax;
										break;
						case REG_BX:	dst_mp = &r_bx;
										break;
						case REG_CX:	dst_mp = &r_cx;
										break;
						case REG_DX:	dst_mp = &r_dx;
										break;
						case REG_SP:	dst_mp = &r_sp;
										break;
						case REG_BP:	dst_mp = &r_bp;
										break;
						case REG_SI:	dst_mp = &r_si;
										break;
						case REG_DI:	dst_mp = &r_di;
										break;
						default:		break;			/* can't happen */
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	dst_mp = &r_al;
										break;
						case REG_BL:	dst_mp = &r_bl;
										break;
						case REG_CL:	dst_mp = &r_cl;
										break;
						case REG_DL:	dst_mp = &r_dl;
										break;
						case REG_AH:	dst_mp = &r_ah;
										break;
						case REG_BH:	dst_mp = &r_bh;
										break;
						case REG_CH:	dst_mp = &r_ch;
										break;
						case REG_DH:	dst_mp = &r_dh;
										break;
						default:		break;			/* can't happen */
					}
				}
			break;


			case 00:
				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
		}

			/* Determine the number of bits to shift */
		if (opcode & 0x02) {
			count = r_cl;
		}
		else {
			if (opcode < 0xD0) {
				count = *(unsigned char *)imp++;
			}
			else
				count = 1;
		}
		count &= 31;

		switch(subcode) {
			case 000:	/* ROL */
						tmp_16 = count;
						if (word_op) {
							while(tmp_16 != 0) {
								tmp_cf = *(unsigned short *)dst_mp & 0x8000 ? 1 : 0;
								*(unsigned short *)dst_mp <<= 1;
								if (tmp_cf)
									*(unsigned short *)dst_mp |= 1;
								tmp_16--;
							}
							CF = tmp_cf;
							if (count == 1) {
								tmp_cf = *(unsigned short *)dst_mp & 0x8000 ? 1: 0;
								if (CF != tmp_cf) 
									OF = 1;
								else
									OF = 0;
							}
						}
						else {
							while(tmp_16 != 0) {
								tmp_cf = *(unsigned char *)dst_mp & 0x80 ? 1 : 0;
								*(unsigned char *)dst_mp <<= 1;
								if (tmp_cf)
									*(unsigned char *)dst_mp |= 1;
								tmp_16--;
							}
							CF = tmp_cf;
							if (count == 1) {
								tmp_cf = *(unsigned char *)dst_mp & 0x80 ? 1: 0;
								if (CF != tmp_cf) 
									OF = 1;
								else
									OF = 0;
							}
						}
						break;
			case 001:	/* ROR */
						tmp_16 = count;
						if (word_op) {
							while(tmp_16 != 0) {
								tmp_cf = *(unsigned short *)dst_mp & 0x1;
								*(unsigned short *)dst_mp >>= 1;
								if (tmp_cf)
									*(unsigned short *)dst_mp |= 0x8000;
								tmp_16--;
							}
							CF = tmp_cf;
							if (count == 1) {
								tmp_cf = *(unsigned short *)dst_mp & 0x8000 ? 1: 0;
								tmp_32    = *(unsigned short *)dst_mp & 0x4000 ? 1: 0;
								if (tmp_32 != tmp_cf) 
									OF = 1;
								else
									OF = 0;
							}
						}
						else {
							while(tmp_16 != 0) {
								tmp_cf = *(unsigned char *)dst_mp & 1;
								*(unsigned char *)dst_mp >>= 1;
								if (tmp_cf)
									*(unsigned char *)dst_mp |= 0x80;
								tmp_16--;
							}
							CF = tmp_cf;
							if (count == 1) {
								tmp_cf = *(unsigned char *)dst_mp & 0x80 ? 1: 0;
								tmp_32    = *(unsigned char *)dst_mp & 0x40 ? 1: 0;
								if (tmp_32 != tmp_cf) 
									OF = 1;
								else
									OF = 0;
							}
						}
						break;
			case 002:	/* RCL */
						tmp_16 = count;
						if (word_op) {
							while(tmp_16 != 0) {
								tmp_cf = CF;
								CF = *(unsigned short *)dst_mp & 0x8000 ? 1 : 0;
								*(unsigned short *)dst_mp <<= 1;
								if (tmp_cf)
									*(unsigned short *)dst_mp |= 1;
								tmp_16--;
							}
							if (count == 1) {
								tmp_cf = *(unsigned short *)dst_mp & 0x8000 ? 1: 0;
								if (CF != tmp_cf) 
									OF = 1;
								else
									OF = 0;
							}
						}
						else {
							while(tmp_16 != 0) {
								tmp_cf = CF;
								CF = *(unsigned char *)dst_mp & 0x80 ? 1 : 0;
								*(unsigned char *)dst_mp <<= 1;
								if (tmp_cf)
									*(unsigned char *)dst_mp |= 1;
								tmp_16--;
							}
							if (count == 1) {
								tmp_cf = *(unsigned char *)dst_mp & 0x80 ? 1: 0;
								if (CF != tmp_cf) 
									OF = 1;
								else
									OF = 0;
							}
						}
						break;
			case 003:	/* RCR */
						tmp_16 = count;
						if (word_op) {
							while(tmp_16 != 0) {
								tmp_cf = CF;
								CF = *(unsigned short *)dst_mp & 0x1;
								*(unsigned short *)dst_mp >>= 1;
								if (tmp_cf)
									*(unsigned short *)dst_mp |= 0x8000;
								tmp_16--;
							}
							if (count == 1) {
								tmp_cf = *(unsigned short *)dst_mp & 0x8000 ? 1: 0;
								tmp_32    = *(unsigned short *)dst_mp & 0x4000 ? 1: 0;
								if (tmp_32 != tmp_cf) 
									OF = 1;
								else
									OF = 0;
							}
						}
						else {
							while(tmp_16 != 0) {
								tmp_cf = CF;
								CF = *(unsigned char *)dst_mp & 1;
								*(unsigned char *)dst_mp >>= 1;
								if (tmp_cf)
									*(unsigned char *)dst_mp |= 0x80;
								tmp_16--;
							}
							if (count == 1) {
								tmp_cf = *(unsigned char *)dst_mp & 0x80 ? 1: 0;
								tmp_32    = *(unsigned char *)dst_mp & 0x40 ? 1: 0;
								if (tmp_32 != tmp_cf) 
									OF = 1;
								else
									OF = 0;
							}
						}
						break;
			case 004:	/* SHL */
						tmp_16 = count;
						if (word_op) {
							while(tmp_16 != 0) {
								CF = *(unsigned short *)dst_mp & 0x8000 ? 1: 0;
								*(unsigned short *)dst_mp <<= 1;
								tmp_16--;
							}
							if (count == 1) {
								if (CF == (*(unsigned short *)dst_mp & 0x8000 ? 1: 0))
									OF = 0;
								else
									OF = 1;
							}
							SET_ZF_PF((*(unsigned short *)dst_mp));
							SET_SF((*(unsigned short *)dst_mp));
						}
						else {
							while(tmp_16 != 0) {
								CF = *(unsigned char *)dst_mp & 0x80 ? 1: 0;
								*(unsigned char *)dst_mp <<= 1;
								tmp_16--;
							}
							if (count == 1) {
								if (CF == (*(unsigned char *)dst_mp & 0x80 ? 1: 0))
									OF = 0;
								else
									OF = 1;
							}
							SET_ZF_PF_8((*(unsigned char *)dst_mp));
							SET_SF_8((*(unsigned char *)dst_mp));
						}
						break;
			case 005:	/* SHR */
						tmp_16 = count;
						if (word_op) {
							if (count == 1)
								OF = *(unsigned short *)dst_mp & 0x8000 ? 1: 0;
							while(tmp_16 != 0) {
								CF = *(unsigned short *)dst_mp & 1;
								*(unsigned short *)dst_mp >>= 1;
								tmp_16--;
							}
							SET_ZF_PF((*(unsigned short *)dst_mp));
							SET_SF((*(unsigned short *)dst_mp));
						}
						else {
							if (count == 1)
								OF = *(unsigned char *)dst_mp & 0x80 ? 1: 0;
							while(tmp_16 != 0) {
								CF = *(unsigned char *)dst_mp & 1;
								*(unsigned char *)dst_mp >>= 1;
								tmp_16--;
							}
							SET_ZF_PF_8((*(unsigned char *)dst_mp));
							SET_SF_8((*(unsigned char *)dst_mp));
						}
						break;
			case 006:	/* NOT USED */
						break;
			case 007:	/* SAR */
						tmp_16 = count;
						if (count == 1)
							OF = 0;
						if (word_op) {
							while(tmp_16 != 0) {
								CF = *(unsigned short *)dst_mp & 1;
								*(short *)dst_mp /= 2;
								tmp_16--;
							}
							SET_ZF_PF((*(unsigned short *)dst_mp));
							SET_SF((*(unsigned short *)dst_mp));
						}
						else {
							while(tmp_16 != 0) {
								CF = *(unsigned char *)dst_mp & 1;
								*(char *)dst_mp /= 2;
								tmp_16--;
							}
							SET_ZF_PF_8((*(unsigned char *)dst_mp));
							SET_SF_8((*(unsigned char *)dst_mp));
						}
						break;
			default:	break;
		}

		if (dest_is_reg) {
			if (opcode & 0x01) {
				SET_ALL_BREGS();
				smp = SET_NEW_SMP();
			}
			else {
				SET_ALL_WREGS();
			}
		}

	}
	break;

	case 0x8F: {
		short dest_is_reg, mod, dest_reg;
		void *dst_mp;

		mod_byte = *imp++;
		dest_is_reg = 0;
		dest_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is a REGISTER to REGISTER Transfer */
				dest_is_reg = 1;
				switch(dest_reg) {
					case REG_AX:	dst_mp = &r_ax;
									break;
					case REG_BX:	dst_mp = &r_bx;
									break;
					case REG_CX:	dst_mp = &r_cx;
									break;
					case REG_DX:	dst_mp = &r_dx;
									break;
					case REG_SP:	dst_mp = &r_sp;
									break;
					case REG_BP:	dst_mp = &r_bp;
									break;
					case REG_SI:	dst_mp = &r_si;
									break;
					case REG_DI:	dst_mp = &r_di;
									break;
					default:		break;			/* can't happen */
				}
			break;


			case 00:
				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
		}
		smp = SET_NEW_SMP();
		*(unsigned short *)dst_mp = *smp;
		r_sp += 2;

		if (dest_is_reg) {
			SET_ALL_BREGS();
		}
		smp = SET_NEW_SMP();
	}
	break;

	case 0xF6:
	case 0xF7: {
		short dest_is_reg, dest_reg, mod, subcode;
		void *dst_mp;

		mod_byte = *imp++;
		dest_is_reg = 0;
		dest_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		subcode = (mod_byte >> 3) & 0x07;
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is a REGISTER to REGISTER Transfer */
				dest_is_reg = 1;
				if (opcode & 0x01) {
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_ax;
										break;
						case REG_BX:	dst_mp = &r_bx;
										break;
						case REG_CX:	dst_mp = &r_cx;
										break;
						case REG_DX:	dst_mp = &r_dx;
										break;
						case REG_SP:	dst_mp = &r_sp;
										break;
						case REG_BP:	dst_mp = &r_bp;
										break;
						case REG_SI:	dst_mp = &r_si;
										break;
						case REG_DI:	dst_mp = &r_di;
										break;
						default:		break;			/* can't happen */
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	dst_mp = &r_al;
										break;
						case REG_BL:	dst_mp = &r_bl;
										break;
						case REG_CL:	dst_mp = &r_cl;
										break;
						case REG_DL:	dst_mp = &r_dl;
										break;
						case REG_AH:	dst_mp = &r_ah;
										break;
						case REG_BH:	dst_mp = &r_bh;
										break;
						case REG_CH:	dst_mp = &r_ch;
										break;
						case REG_DH:	dst_mp = &r_dh;
										break;
						default:		break;			/* can't happen */
					}
				}
			break;


			case 00:
				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
		}
		switch(subcode) {
			case 000:	if (opcode & 0x01) {
							tmp_u16 = *(unsigned short *)imp;
							imp+=2;
							tmp_u16 = *(unsigned short *)dst_mp & tmp_u16;		/* TEST Immediate 16 bit */
							SET_ZF_PF(tmp_u16);
							SET_SF(tmp_u16);
							CF = OF = 0;
						}
						else {
							tmp_u8 = *(unsigned char *)imp++;
							tmp_u8 = *(unsigned char *)dst_mp & tmp_u8;		/* TEST Immediate 8 bit */
							SET_ZF_PF_8(tmp_u8);
							SET_SF_8(tmp_u8);
							CF = OF = 0;
						}
						break;

			case 001:	break;					/* NOT USED */



			case 002:	if (opcode & 0x01) 			/* Word op */			/* NOT */
							*(unsigned short *)dst_mp ^= 0xffff;
						else
							*(unsigned char *)dst_mp ^= 0xff;
						break;

			case 003:	if (opcode & 0x01) {			/* Word op */			/* NEG */
							if (*(unsigned short *)dst_mp == 0x8000) {
								CF = OF = SF = 1; ZF = 0;
								break;
							}
							if (*(short *)dst_mp == 0) {
								CF = OF = SF = 0; ZF = 1;
								break;
							}
							*(short *)dst_mp = -*(short *)dst_mp;
							CF = 1; OF = ZF = 0;
							if (*(unsigned short *)dst_mp & 0x8000)
								SF = 1;
							else
								SF = 0;
						}
						else {						/* Byte op */
							if (*(unsigned char *)dst_mp == 0x80) {
									CF = OF = SF = 1; ZF = 0;
									break;
							}
							if (*(char *)dst_mp == 0) {
								CF = OF = SF = 0; ZF = 1;
								break;
							}
							*(char *)dst_mp = -*(char *)dst_mp;
							CF = 1; OF = ZF = 0;
							if (*(unsigned char *)dst_mp & 0x80)
								SF = 1;
							else
								SF = 0;
						}
						break;

			case 004:	if (opcode & 0x01) {			/* Word Op */					/* MUL */
							tmp_32 = (unsigned long)r_ax * (unsigned long)*(unsigned short *)dst_mp;
							r_ax = (tmp_32 & 0xffff);
							r_dx = (unsigned long)(tmp_32 & 0xffff0000) >> 16;
							SET_AL_AH();
							SET_DL_DH();
							if (r_dx == 0)
								CF = OF = 0;
							else
								CF = OF = 1;
							TRUNC_16(r_dx);
							TRUNC_16(r_ax);
						}
						else {
							r_ax = (unsigned short)r_al * (unsigned short)*(unsigned char *)dst_mp;
							SET_AL_AH();
							TRUNC_16(r_ax);
							if (r_ah == 0)
								CF = OF = 0;
							else
								CF = OF = 1;
						}
						break;

			case 005:	if (opcode & 0x01) {			/* Word Op */					/* IMUL */
							tmp_32 = r_ax * (long)*(short *)dst_mp;
							if (r_ax & 0x8000) {
								if (tmp_32 < -32768)
									CF = OF = 1;
								else
									CF = OF = 0;
							}
							else {
								if (tmp_32 >= 32768)
									CF = OF = 1;
								else
									CF = OF = 0;
							}
							r_ax = (tmp_32 & 0xffff);
							r_dx = (unsigned long)(tmp_32 & 0xffff0000) >> 16;
							SET_AL_AH();
							SET_DL_DH();
							TRUNC_16(r_dx);
							TRUNC_16(r_ax);
						}
						else {
							tmp_16 = r_al * (long)*(char *)dst_mp;
							if (r_al & 0x80) {
								if (tmp_16 < -128)
									CF = OF = 1;
								else
									CF = OF = 0;
							}
							else {
								if (tmp_16 >= 128)
									CF = OF = 1;
								else
									CF = OF = 0;
							}
							r_ax = (tmp_16 & 0xffff);
							SET_AL_AH();
							TRUNC_16(r_ax);
						}
						break;

			case 006:	if (opcode & 0x01) {				/* WORD op */		/* DIV */
							if (*(unsigned short *)dst_mp == 0) {		/* Handle divide by zero */
								PUSH_ARG(r_fl);
								IF = TF = 0;
								r_ip = GET_IP();
								PUSH_ARG(r_cs);
								PUSH_ARG(r_ip);
								dmp_16 = (unsigned short *)SET_NEW_DMP(0,0,0,0);
								r_ip = *dmp_16;
								r_cs = *(dmp_16+1);
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							else {
								unsigned long tmp1_u32;

								tmp_u32 = (unsigned long)r_dx << 16 | r_ax;
								tmp1_u32 = tmp_u32 / *(unsigned short *)dst_mp;
								if (tmp1_u32 > 0xffff) {
									PUSH_ARG(r_fl);
									IF = TF = 0;
									r_ip = GET_IP();
									PUSH_ARG(r_cs);
									PUSH_ARG(r_ip);
									dmp_16 = SET_NEW_DMP(0,0,0,0);
									r_ip = *dmp_16;
									r_cs = *(dmp_16+1);
									imp = SET_NEW_IMP(r_cs,r_ip);
								}
								else {
									r_ax = (tmp1_u32 & 0xffff);
									r_dx = tmp_u32 % *(unsigned short *)dst_mp;
									SET_AL_AH();
									SET_DL_DH();
								}
							}
						}
						else {
							if (*(unsigned char *)dst_mp == 0) {		/* Handle divide by zero */
								PUSH_ARG(r_fl);
								IF = TF = 0;
								r_ip = GET_IP();
								PUSH_ARG(r_cs);
								PUSH_ARG(r_ip);
								dmp_16 = SET_NEW_DMP(0,0,0,0);
								r_ip = *dmp_16;
								r_cs = *(dmp_16+1);
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							else {
								tmp_16 = r_ax / *(unsigned char *)dst_mp;
								if (tmp_16 > 0xff) {
									PUSH_ARG(r_fl);
									IF = TF = 0;
									r_ip = GET_IP();
									PUSH_ARG(r_cs);
									PUSH_ARG(r_ip);
									dmp_16 = SET_NEW_DMP(0,0,0,0);
									r_ip = *dmp_16;
									r_cs = *(dmp_16+1);
									imp = SET_NEW_IMP(r_cs,r_ip);
									break;
								}
								else {
									r_al = tmp_16;
									r_ah = r_ax % *(unsigned char *)dst_mp;
									SET_AX();
								}
							}
						}
						break;

			case 007:	if (opcode & 0x01) {				/* WORD op */		/* IDIV */
							if (*(short *)dst_mp == 0) {		/* Handle divide by zero */
								PUSH_ARG(r_fl);
								IF = TF = 0;
								r_ip = GET_IP();
								PUSH_ARG(r_cs);
								PUSH_ARG(r_ip);
								dmp_16 = SET_NEW_DMP(0,0,0,0);
								r_ip = *dmp_16;
								r_cs = *(dmp_16+1);
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							else {
								long tmp1_32;

								tmp_32 = (unsigned long)r_dx << 16 | r_ax;
								tmp1_32 = tmp_32 / *(short *)dst_mp;
								if (r_dx & 0x8000) {
									if (tmp1_32 < -32768 ) {
										PUSH_ARG(r_fl);
										IF = TF = 0;
										r_ip = GET_IP();
										PUSH_ARG(r_cs);
										PUSH_ARG(r_ip);
										dmp_16 = SET_NEW_DMP(0,0,0,0);
										r_ip = *dmp_16;
										r_cs = *(dmp_16+1);
										imp = SET_NEW_IMP(r_cs,r_ip);
									}
									else {
										r_ax = (tmp1_32 & 0xffff);
										r_dx = (tmp_32 % *(short *)dst_mp) & 0xffff;
										SET_AL_AH();
										SET_DL_DH();
									}
								}
								else {
									if (tmp1_32 > 32767) {
										PUSH_ARG(r_fl);
										IF = TF = 0;
										r_ip = GET_IP();
										PUSH_ARG(r_cs);
										PUSH_ARG(r_ip);
										dmp_16 = SET_NEW_DMP(0,0,0,0);
										r_ip = *dmp_16;
										r_cs = *(dmp_16+1);
										imp = SET_NEW_IMP(r_cs,r_ip);
									}
									else {
										r_ax = (tmp1_32 & 0xffff);
										r_dx = (tmp_32 % *(short *)dst_mp) & 0xffff;
										SET_AL_AH();
										SET_DL_DH();
									}
								}
							}
						}
						else {
							if (*(char *)dst_mp == 0) {		/* Handle divide by zero */
								PUSH_ARG(r_fl);
								IF = TF = 0;
								r_ip = GET_IP();
								PUSH_ARG(r_cs);
								PUSH_ARG(r_ip);
								dmp_16 = SET_NEW_DMP(0,0,0,0);
								r_ip = *dmp_16;
								r_cs = *(dmp_16+1);
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							else {
								tmp_16 = r_ax / *(char *)dst_mp;
								if (r_ax & 0x8000) {
									if (tmp_16 < -128 ) {
										PUSH_ARG(r_fl);
										IF = TF = 0;
										r_ip = GET_IP();
										PUSH_ARG(r_cs);
										PUSH_ARG(r_ip);
										dmp_16 = SET_NEW_DMP(0,0,0,0);
										r_ip = *dmp_16;
										r_cs = *(dmp_16+1);
										imp = SET_NEW_IMP(r_cs,r_ip);
									}
									else {
										r_al = tmp_16 & 0xff;
										r_ah = (r_ax % *(char *)dst_mp) & 0xff;
										SET_AX();
									}
								}
								else {
									if (tmp_16 > 127) {
										PUSH_ARG(r_fl);
										IF = TF = 0;
										r_ip = GET_IP();
										PUSH_ARG(r_cs);
										PUSH_ARG(r_ip);
										dmp_16 = SET_NEW_DMP(0,0,0,0);
										r_ip = *dmp_16;
										r_cs = *(dmp_16+1);
										imp = SET_NEW_IMP(r_cs,r_ip);
									}
									else {
										r_al = (tmp_16 & 0xff);
										r_ah = (r_ax % *(char *)dst_mp) & 0xff;
										SET_AX();
									}
								}
							}
						}
						break;

			default:	break;				/* ERROR CONDITION, Cannot happen */
		}

		if (dest_is_reg) {
			if (opcode & 0x01) {
				SET_ALL_BREGS();
				smp = SET_NEW_SMP();
			}
			else {
				SET_ALL_WREGS();
			}
		}

	}
	break;

	case 0x8C:
	case 0x8E: {
		short dest_is_reg, mod, subcode,dest_reg;
		void *dst_mp;

		dest_is_reg = 0;
		mod_byte = *imp++;
		dest_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		subcode = (mod_byte >> 3) & 0x03;
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is a REGISTER to REGISTER Transfer */
				dest_is_reg = 1;
				switch(dest_reg) {
					case REG_AX:	dst_mp = &r_ax;
									break;
					case REG_BX:	dst_mp = &r_bx;
									break;
					case REG_CX:	dst_mp = &r_cx;
									break;
					case REG_DX:	dst_mp = &r_dx;
									break;
					case REG_SP:	dst_mp = &r_sp;
									break;
					case REG_BP:	dst_mp = &r_bp;
									break;
					case REG_SI:	dst_mp = &r_si;
									break;
					case REG_DI:	dst_mp = &r_di;
									break;
					default:		break;			/* can't happen */
				}
			break;

			case 00:
				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
				break;
		}
		switch(subcode) {
			case 000:	if (opcode == 0x8E)
							r_es = *(unsigned short *)dst_mp;
						else
							*(unsigned short *)dst_mp = r_es;
						break;

			case 001:	if (opcode == 0x8E)
							; /* r_cs = *(unsigned short *)dst_mp; */
						else
							*(unsigned short *)dst_mp = r_cs;
						break;

			case 002:	if (opcode == 0x8E)
							r_ss = *(unsigned short *)dst_mp;
						else
							*(unsigned short *)dst_mp = r_ss;
						break;

			case 003:	if (opcode == 0x8E)
							r_ds = *(unsigned short *)dst_mp;
						else
							*(unsigned short *)dst_mp = r_ds;
						break;

			default:	break;				/* ERROR CONDITION, Cannot happen */
		}

		if (dest_is_reg) {
			SET_ALL_BREGS();
			smp = SET_NEW_SMP();
		}
	}
	break;

	case 0xC6:
	case 0xC7: {
		short dest_is_reg,dest_reg,mod;
		void *dst_mp;

		dest_is_reg = 0;
		mod_byte = *imp++;
		dest_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is a REGISTER to REGISTER Transfer */
				dest_is_reg = 1;
				if (opcode & 0x01) {
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_ax;
										break;
						case REG_BX:	dst_mp = &r_bx;
										break;
						case REG_CX:	dst_mp = &r_cx;
										break;
						case REG_DX:	dst_mp = &r_dx;
										break;
						case REG_SP:	dst_mp = &r_sp;
										break;
						case REG_BP:	dst_mp = &r_bp;
										break;
						case REG_SI:	dst_mp = &r_si;
										break;
						case REG_DI:	dst_mp = &r_di;
										break;
						default:		break;			/* can't happen */
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	dst_mp = &r_al;
										break;
						case REG_BL:	dst_mp = &r_bl;
										break;
						case REG_CL:	dst_mp = &r_cl;
										break;
						case REG_DL:	dst_mp = &r_dl;
										break;
						case REG_AH:	dst_mp = &r_ah;
										break;
						case REG_BH:	dst_mp = &r_bh;
										break;
						case REG_CH:	dst_mp = &r_ch;
										break;
						case REG_DH:	dst_mp = &r_dh;
										break;
						default:		break;			/* can't happen */
					}
				}
			break;


			case 00:
				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
				break;
		}

		if (opcode & 0x01) {
			*(unsigned short *)dst_mp = *(unsigned short *)imp;
			imp+=2;
		}
		else  {
			*(unsigned char *)dst_mp =  *(unsigned char *)imp++;
		}

		if (dest_is_reg) {
			if (opcode & 0x01) {
				SET_ALL_BREGS();
				smp = SET_NEW_SMP();
			}
			else {
				SET_ALL_WREGS();
			}
		}

	}
	break;

	case MOV:
	case MOV|2:
	case ADC:
	case ADC|2:
	case ADD:
	case ADD|2:
	case AND:
	case AND|2:
	case CMP:
	case CMP|2:
	case OR:
	case OR|2:
	case SBB:
	case SBB|2:
	case SUB:
	case SUB|2:
	case XOR:
	case XOR|2:
	case MOVB|2:
	case MOVB:
	case ANDB:
	case ANDB|2:
	case ADCB|2:
	case ADCB:
	case ADDB|2:
	case ADDB:
	case CMPB|2:
	case CMPB:
	case ORB|2:
	case ORB:
	case SBBB|2:
	case SBBB:
	case XORB|2:
	case XORB:
	case SUBB|2:
	case SUBB: {
				short source_reg,mod,dest_reg,word_op;
				void *src_mp,*dst_mp,*tmp_mp;

				mod_byte = *imp++;
				word_op = (opcode & 0x01);
				opcode &= 0xfe;


				source_reg = mod_byte & 0x07;		/* Get source register or R/M field */
				dest_reg = (mod_byte >> 3) & 0x07;		/* Get Destination register */
				mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
				switch(mod) {		/* Handle type of operation */
					case 03:		/* This is a REGISTER to REGISTER Transfer */
						if (word_op) {
							switch(dest_reg) {
								case REG_AX:	src_mp = &r_ax;
												break;
								case REG_BX:	src_mp = &r_bx;
												break;
								case REG_CX:	src_mp = &r_cx;
												break;
								case REG_DX:	src_mp = &r_dx;
												break;
								case REG_SP:	src_mp = &r_sp;
												break;
								case REG_BP:	src_mp = &r_bp;
												break;
								case REG_SI:	src_mp = &r_si;
												break;
								case REG_DI:	src_mp = &r_di;
												break;
							}

							switch(source_reg) {
								case REG_AX:	dst_mp = &r_ax;
												break;
								case REG_BX:	dst_mp = &r_bx;
												break;
								case REG_CX:	dst_mp = &r_cx;
												break;
								case REG_DX:	dst_mp = &r_dx;
												break;
								case REG_SP:	dst_mp = &r_sp;
												break;
								case REG_BP:	dst_mp = &r_bp;
												break;
								case REG_SI:	dst_mp = &r_si;
												break;
								case REG_DI:	dst_mp = &r_di;
												break;
							}
						}
						else {
							switch(dest_reg) {
								case REG_AL:	src_mp = &r_al;
												break;
								case REG_BL:	src_mp = &r_bl;
												break;
								case REG_CL:	src_mp = &r_cl;
												break;
								case REG_DL:	src_mp = &r_dl;
												break;
								case REG_AH:	src_mp = &r_ah;
												break;
								case REG_BH:	src_mp = &r_bh;
												break;
								case REG_CH:	src_mp = &r_ch;
												break;
								case REG_DH:	src_mp = &r_dh;
												break;
							}

							switch(source_reg) {
								case REG_AL:	dst_mp = &r_al;
												break;
								case REG_BL:	dst_mp = &r_bl;
												break;
								case REG_CL:	dst_mp = &r_cl;
												break;
								case REG_DL:	dst_mp = &r_dl;
												break;
								case REG_AH:	dst_mp = &r_ah;
												break;
								case REG_BH:	dst_mp = &r_bh;
												break;
								case REG_CH:	dst_mp = &r_ch;
												break;
								case REG_DH:	dst_mp = &r_dh;
												break;
							}
						}
						if (opcode & 0x02) {			/* Direction bit set */
							tmp_mp = src_mp;			/* Reverse direction of operation */
							src_mp = dst_mp;
							dst_mp = tmp_mp;			
						}
						break;


					case 00:
						switch(source_reg) {			/* r/m */
							case 000:	if (new_seg) 
											dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
										else
											dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
										break;
							case 001:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
										else
											dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
										break;
							case 002:	if (new_seg) 
											dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
										else
											dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
										break;
							case 003:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
										else
											dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
										break;
							case 004:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
										else
											dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
										break;
							case 005:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
										else
											dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
										break;
							case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
										imp+=2;
										if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
										else
											dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
										break;
							case 007:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
										else
											dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
										break;
						}
						if (word_op) {
							switch(dest_reg) {
								case REG_AX:	src_mp = &r_ax;
												break;
								case REG_BX:	src_mp = &r_bx;
												break;
								case REG_CX:	src_mp = &r_cx;
												break;
								case REG_DX:	src_mp = &r_dx;
												break;
								case REG_SP:	src_mp = &r_sp;
												break;
								case REG_BP:	src_mp = &r_bp;
												break;
								case REG_SI:	src_mp = &r_si;
												break;
								case REG_DI:	src_mp = &r_di;
												break;
							}
						}
						else {
							switch(dest_reg) {
								case REG_AL:	src_mp = &r_al;
												break;
								case REG_BL:	src_mp = &r_bl;
												break;
								case REG_CL:	src_mp = &r_cl;
												break;
								case REG_DL:	src_mp = &r_dl;
												break;
								case REG_AH:	src_mp = &r_ah;
												break;
								case REG_BH:	src_mp = &r_bh;
												break;
								case REG_CH:	src_mp = &r_ch;
												break;
								case REG_DH:	src_mp = &r_dh;
												break;
							}
						}
						if (opcode & 0x02) {			/* Direction bit set */
							tmp_mp = src_mp;			/* Reverse direction of operation */
							src_mp = dst_mp;
							dst_mp = tmp_mp;			
						}
						break;

					case 01:
						disp_8 = *imp++;

						switch(source_reg) {			/* r/m */
							case 000:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
										else
											dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
										break;
							case 001:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
										else
											dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
										break;
							case 002:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
										else
											dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
										break;
							case 003:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
										else
											dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
										break;
							case 004:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
										else
											dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
										break;
							case 005:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
										else
											dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
										break;
							case 006:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
										else
											dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
										break;
							case 007:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
										else
											dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
										break;
						}
						if (word_op) {
							switch(dest_reg) {
								case REG_AX:	src_mp = &r_ax;
												break;
								case REG_BX:	src_mp = &r_bx;
												break;
								case REG_CX:	src_mp = &r_cx;
												break;
								case REG_DX:	src_mp = &r_dx;
												break;
								case REG_SP:	src_mp = &r_sp;
												break;
								case REG_BP:	src_mp = &r_bp;
												break;
								case REG_SI:	src_mp = &r_si;
												break;
								case REG_DI:	src_mp = &r_di;
												break;
							}
						}
						else {
							switch(dest_reg) {
								case REG_AL:	src_mp = &r_al;
												break;
								case REG_BL:	src_mp = &r_bl;
												break;
								case REG_CL:	src_mp = &r_cl;
												break;
								case REG_DL:	src_mp = &r_dl;
												break;
								case REG_AH:	src_mp = &r_ah;
												break;
								case REG_BH:	src_mp = &r_bh;
												break;
								case REG_CH:	src_mp = &r_ch;
												break;
								case REG_DH:	src_mp = &r_dh;
												break;
							}
						}
						if (opcode & 0x02) {			/* Direction bit set */
							tmp_mp = src_mp;			/* Reverse direction of operation */
							src_mp = dst_mp;
							dst_mp = tmp_mp;			
						}
					break;

					case 02:
						disp_u16 = *(unsigned short *)imp;
						imp+=2;

						switch(source_reg) {			/* r/m */
							case 000:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
										else
											dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
										break;
							case 001:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
										else
											dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
										break;
							case 002:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
										else
											dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
										break;
							case 003:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
										else
											dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
										break;
							case 004:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
										else
											dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
										break;
							case 005:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
										else
											dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
										break;
							case 006:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
										else
											dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
										break;
							case 007:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
										else
											dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
										break;
						}
						if (word_op) {
							switch(dest_reg) {
								case REG_AX:	src_mp = &r_ax;
												break;
								case REG_BX:	src_mp = &r_bx;
												break;
								case REG_CX:	src_mp = &r_cx;
												break;
								case REG_DX:	src_mp = &r_dx;
												break;
								case REG_SP:	src_mp = &r_sp;
												break;
								case REG_BP:	src_mp = &r_bp;
												break;
								case REG_SI:	src_mp = &r_si;
												break;
								case REG_DI:	src_mp = &r_di;
												break;
							}
						}
						else {
							switch(dest_reg) {
								case REG_AL:	src_mp = &r_al;
												break;
								case REG_BL:	src_mp = &r_bl;
												break;
								case REG_CL:	src_mp = &r_cl;
												break;
								case REG_DL:	src_mp = &r_dl;
												break;
								case REG_AH:	src_mp = &r_ah;
												break;
								case REG_BH:	src_mp = &r_bh;
												break;
								case REG_CH:	src_mp = &r_ch;
												break;
								case REG_DH:	src_mp = &r_dh;
												break;
							}
						}
						if (opcode & 0x02) {			/* Direction bit set */
							tmp_mp = src_mp;			/* Reverse direction of operation */
							src_mp = dst_mp;
							dst_mp = tmp_mp;			
						}
						break;
				default:	break;
			}
			if (opcode & 0x02)
				opcode &= ~0x02;			/* Strip out direction bit, since it is handled in the mod/reg/rm decode */

			if (word_op) {
				switch(opcode) {
					case ADDB:	tmp_32 = *(short *)dst_mp;						/* ADD 16 bit */
								sign = tmp_32 & 0x8000;
								tmp_32 += *(short *)src_mp;
								*(unsigned short *)dst_mp = (tmp_32 & 0xffff);
								SET_OF_SF(sign,tmp_32); SET_ZF_PF(tmp_32); SET_CF(tmp_32);
								break;

					case ORB:	*(unsigned short *)dst_mp |= *(unsigned short *)src_mp;		/* OR 16 bit */
								SET_ZF_PF((*(unsigned short *)dst_mp));
								SET_SF((*(unsigned short *)dst_mp));
								CF = OF = 0;
								break;

					case ADCB:	tmp_32 = *(short *)dst_mp;						/* ADC 16 bit */
								sign = tmp_32 & 0x8000;
								tmp_32 += ((*(short *)src_mp) + CF);
								*(unsigned short *)dst_mp = (tmp_32 & 0xffff);
								SET_OF_SF(sign,tmp_32); SET_ZF_PF(tmp_32); SET_CF(tmp_32);
								break;

					case SBBB:	tmp_32 = *(short *)dst_mp;						/* SBB 16 bit */
								sign = tmp_32 & 0x8000;
								tmp_32 -= ((*(short *)src_mp) + CF);
								*(unsigned short *)dst_mp = (tmp_32 & 0xffff);
								SET_OF_SF(sign,tmp_32); SET_ZF_PF(tmp_32); SET_CF(tmp_32);
								break;

					case ANDB:	*(unsigned short *)dst_mp &= *(unsigned short *)src_mp;		/* AND 16 bit */
								SET_ZF_PF((*(unsigned short *)dst_mp));
								SET_SF((*(unsigned short *)dst_mp));
								CF = OF = 0;
								break;

					case SUBB:	tmp_32 = *(short *)dst_mp;						/* SUB 16 bit */
								sign = tmp_32 & 0x8000;
								tmp_32 -= *(short *)src_mp;
								*(unsigned short *)dst_mp = (tmp_32 & 0xffff);
								SET_OF_SF(sign,tmp_32); SET_ZF_PF(tmp_32); SET_CF(tmp_32);
								break;

					case XORB:	*(unsigned short *)dst_mp ^= *(unsigned short *)src_mp;		/* XOR 16 bit */
								SET_ZF_PF((*(unsigned short *)dst_mp));
								SET_SF((*(unsigned short *)dst_mp));
								CF = OF = 0;
								break;

					case CMPB:	tmp_32 = *(short *)dst_mp;						/* CMP 16 bit */
								sign = tmp_32 & 0x8000;
								tmp_32 -= *(short *)src_mp;
								SET_OF_SF(sign,tmp_32); SET_ZF_PF(tmp_32); SET_CF(tmp_32);
								break;

					case MOVB:	*(unsigned short *)dst_mp = *(unsigned short *)src_mp;
								break;

					default:	break;				/* ERROR CONDITION, Cannot happen */
				}
			}
			else {
				switch(opcode) {
					case ADDB:	tmp_16 = *(char *)dst_mp;						/* ADD  8 bit */
								sign = tmp_16 & 0x80;
								tmp_16 += *(char *)src_mp;
								*(unsigned char *)dst_mp = (tmp_16 & 0xff);
								SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
								break;

					case ORB:	*(unsigned char *)dst_mp |= *(unsigned char *)src_mp;		/* OR  8 bit */
								SET_ZF_PF_8((*(unsigned char *)dst_mp));
								SET_SF_8((*(unsigned char *)dst_mp));
								CF = OF = 0;
								break;

					case ADCB:	tmp_16 = *(char *)dst_mp;						/* ADC  8 bit */
								sign = tmp_16 & 0x80;
								tmp_16 += ((*(char *)src_mp) + CF);
								*(unsigned char *)dst_mp = (tmp_16 & 0xff);
								SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
								break;

					case SBBB:	tmp_16 = *(char *)dst_mp;						/* SBB  8 bit */
								sign = tmp_16 & 0x80;
								tmp_16 -= ((*(char *)src_mp) + CF);
								*(unsigned char *)dst_mp = (tmp_16 & 0xff);
								SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
								break;

					case ANDB:	*(unsigned char *)dst_mp &= *(unsigned char *)src_mp;		/* AND  8 bit */
								SET_ZF_PF_8((*(unsigned char *)dst_mp));
								SET_SF_8((*(unsigned char *)dst_mp));
								CF = OF = 0;
								break;

					case SUBB:	tmp_16 = *(char *)dst_mp;						/* SUB  8 bit */
								sign = tmp_16 & 0x80;
								tmp_16 -= *(char *)src_mp;
								*(unsigned char *)dst_mp = (tmp_16 & 0xff);
								SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
								break;

					case XORB:	*(unsigned char *)dst_mp ^= *(unsigned char *)src_mp;		/* XOR  8 bit */
								SET_ZF_PF_8((*(unsigned char *)dst_mp));
								SET_SF_8((*(unsigned char *)dst_mp));
								CF = OF = 0;
								break;

					case CMPB:	tmp_16 = *(char *)dst_mp;						/* CMP  8 bit */
								sign = tmp_16 & 0x80;
								tmp_16 -= *(char *)src_mp;
								SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
								break;

					case MOVB:	*(unsigned char *)dst_mp = *(unsigned char *)src_mp;
								break;

					default:	break;				/* ERROR CONDITION, Cannot happen */
				}
			}

			if (word_op) {
				SET_ALL_BREGS();
				smp = SET_NEW_SMP();
			}
			else {
				SET_ALL_WREGS();
			}
		}
		break;

	case LEA: {					/* ADD opsize override here */
		short src_reg,dest_reg,mod;
		unsigned short eff_addr;

		mod_byte = *imp++;
		src_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		dest_reg = (mod_byte >> 3) & 0x07;
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is an illegal instruction */
				break;		

			case 00:
				switch(src_reg) {			/* r/m */
					case 000:	eff_addr = SET_NEW_EA(r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	eff_addr = SET_NEW_EA(r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	eff_addr = SET_NEW_EA(r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	eff_addr = SET_NEW_EA(r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	eff_addr = SET_NEW_EA(0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	eff_addr = SET_NEW_EA(0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								eff_addr = SET_NEW_EA(disp_u16,0,0);
								break;
					case 007:	eff_addr = SET_NEW_EA(r_bx,0,0);		/* DS:[BX] */
								break;
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(src_reg) {			/* r/m */
					case 000:	eff_addr = SET_NEW_EA(r_bx,r_si,disp_8);
								break;
					case 001:	eff_addr = SET_NEW_EA(r_bx,r_di,disp_8);
								break;
					case 002:	eff_addr = SET_NEW_EA(r_bp,r_si,disp_8);
								break;
					case 003:	eff_addr = SET_NEW_EA(r_bp,r_di,disp_8);
								break;
					case 004:	eff_addr = SET_NEW_EA(0,r_si,disp_8);
								break;
					case 005:	eff_addr = SET_NEW_EA(0,r_di,disp_8);
								break;
					case 006:	eff_addr = SET_NEW_EA(0,r_bp,disp_8);
								break;
					case 007:	eff_addr = SET_NEW_EA(r_bx,0,disp_8);
								break;
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(src_reg) {			/* r/m */
					case 000:	eff_addr = SET_NEW_EA(r_bx,r_si,disp_u16);
								break;
					case 001:	eff_addr = SET_NEW_EA(r_bx,r_di,disp_u16);
								break;
					case 002:	eff_addr = SET_NEW_EA(r_bp,r_si,disp_u16);
								break;
					case 003:	eff_addr = SET_NEW_EA(r_bp,r_di,disp_u16);
								break;
					case 004:	eff_addr = SET_NEW_EA(0,r_si,disp_u16);
								break;
					case 005:	eff_addr = SET_NEW_EA(0,r_di,disp_u16);
								break;
					case 006:	eff_addr = SET_NEW_EA(0,r_bp,disp_u16);
								break;
					case 007:	eff_addr = SET_NEW_EA(r_bx,0,disp_u16);
								break;
				}
				break;
		}
		switch(dest_reg) {
			case REG_AX:	r_ax = eff_addr;
							SET_AL_AH();
							break;
			case REG_BX:	r_bx = eff_addr;
							SET_BL_BH();
							break;
			case REG_CX:	r_cx = eff_addr;
							SET_CL_CH();
							break;
			case REG_DX:	r_dx = eff_addr;
							SET_DL_DH();
							break;
			case REG_SP:	r_sp = eff_addr;
							break;
			case REG_BP:	r_bp = eff_addr;
							break;
			case REG_SI:	r_si = eff_addr;
							break;
			case REG_DI:	r_di = eff_addr;
							break;
			default:		break;
		}
	}
	break;

	case LDS:
	case LES: {
		short src_reg,mod,dest_reg;
		void *dst_mp;

		mod_byte = *imp++;
		src_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		dest_reg = (mod_byte >> 3) & 0x07;
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is an illegal instruction */
				break;		

			case 00:
				switch(src_reg) {			/* r/m */
					case 000:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(src_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(src_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
				break;
		}
		switch(dest_reg) {
			case REG_AX:	r_ax = *(unsigned short *)dst_mp;
							SET_AL_AH();
							break;
			case REG_BX:	r_bx = *(unsigned short *)dst_mp;
							SET_BL_BH();
							break;
			case REG_CX:	r_cx = *(unsigned short *)dst_mp;
							SET_CL_CH();
							break;
			case REG_DX:	r_dx = *(unsigned short *)dst_mp;
							SET_DL_DH();
							break;
			case REG_SP:	r_sp = *(unsigned short *)dst_mp;
							break;
			case REG_BP:	r_bp = *(unsigned short *)dst_mp;
							break;
			case REG_SI:	r_si = *(unsigned short *)dst_mp;
							break;
			case REG_DI:	r_di = *(unsigned short *)dst_mp;
							break;
			default:		break;
		}
		if (opcode == LDS)
			r_ds = *(unsigned short *)((unsigned short *)dst_mp+1);
		else
			r_es = *(unsigned short *)((unsigned short *)dst_mp+1);

	}
	break;
	case 0xFE:
	case 0xFF: {
		short dest_is_reg,mod,dest_reg,subcode;
		void *dst_mp;

		dest_is_reg = 0;
		mod_byte = *imp++;
		dest_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		subcode = (mod_byte >> 3) & 0x07;
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is a REGISTER to REGISTER Transfer */
				dest_is_reg = 1;
				if (opcode & 0x01) {
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_ax;
										break;
						case REG_BX:	dst_mp = &r_bx;
										break;
						case REG_CX:	dst_mp = &r_cx;
										break;
						case REG_DX:	dst_mp = &r_dx;
										break;
						case REG_SP:	dst_mp = &r_sp;
										break;
						case REG_BP:	dst_mp = &r_bp;
										break;
						case REG_SI:	dst_mp = &r_si;
										break;
						case REG_DI:	dst_mp = &r_di;
										break;
						default:		break;			/* can't happen */
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	dst_mp = &r_al;
										break;
						case REG_BL:	dst_mp = &r_bl;
										break;
						case REG_CL:	dst_mp = &r_cl;
										break;
						case REG_DL:	dst_mp = &r_dl;
										break;
						case REG_AH:	dst_mp = &r_ah;
										break;
						case REG_BH:	dst_mp = &r_bh;
										break;
						case REG_CH:	dst_mp = &r_ch;
										break;
						case REG_DH:	dst_mp = &r_dh;
										break;
						default:		break;			/* can't happen */
					}
				}
			break;


			case 00:
				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
				break;
		}
		switch(subcode) {
			case 000:	if (opcode & 0x01) {
							tmp_32 = *(short *)dst_mp;						/* INC 16 bit */
							sign = tmp_32 & 0x8000;
							tmp_32++;
							*(unsigned short *)dst_mp = (tmp_32 & 0xffff);
							SET_OF_SF(sign,tmp_32); SET_ZF_PF(tmp_32);
						}
						else {
							tmp_16 = *(char *)dst_mp;						/* INC 8 bit */
							sign = tmp_16 & 0x80;
							tmp_16++;
							*(unsigned char *)dst_mp = (tmp_16 & 0xff);
							SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16);
						}
						break;

			case 001:	if (opcode & 0x01) {
							tmp_32 = *(short *)dst_mp;						/* DEC 16 bit */
							sign = tmp_32 & 0x8000;
							tmp_32--;
							*(unsigned short *)dst_mp = (tmp_32 & 0xffff);
							SET_OF_SF(sign,tmp_32); SET_ZF_PF(tmp_32);
						}
						else {
							tmp_16 = *(char *)dst_mp;						/* DEC 8 bit */
							sign = tmp_16 & 0x80;
							tmp_16--;
							*(unsigned char *)dst_mp = (tmp_16 & 0xff);
							SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16);
						}
						break;

			case 002:	smp = SET_NEW_SMP();
						r_ip = GET_IP();							/* Call Near */
						*(smp-1) = r_ip;
						r_sp -= 2;
						r_ip = *(unsigned short *)dst_mp;
						imp = SET_NEW_IMP(r_cs,r_ip);				/* execution will resume here ! */
						break;

			case 003:	smp = SET_NEW_SMP();
						r_ip = *(unsigned short *)dst_mp;			/* Call FAR, note that value of MOD == 0x03 is illegal */
						*(smp-1) = r_cs;
						*(smp-2) = GET_IP();						/* push the stack segment and  */
						r_cs = *(unsigned short *)((unsigned short *)dst_mp+1);
						r_sp -= 4;									/* Adjust the stack */
						smp = SET_NEW_SMP();
						imp = SET_NEW_IMP(r_cs,r_ip);				/* execution will resume here! */
						break;

			case 004:	r_ip = *(unsigned short *)dst_mp;			/* JMP NEAR */
						imp = SET_NEW_IMP(r_cs,r_ip);						/* execution will resume here! */
						break;

			case 005:	r_ip = *(unsigned short *)dst_mp;			/* JMP FAR */
						r_cs = *(unsigned short *)((unsigned short *)dst_mp+1);
						imp = SET_NEW_IMP(r_cs,r_ip);				/* execution will resume here! */
						break;


			case 006:	r_sp -= 2;
						smp = SET_NEW_SMP();
						*smp = *(unsigned short *)dst_mp;
						break;

			default:	break;				/* ERROR CONDITION, Cannot happen */
		}

		if (dest_is_reg) {
			if (opcode & 0x01) {
				SET_ALL_BREGS();
				smp = SET_NEW_SMP();
			}
			else {
				SET_ALL_WREGS();
			}
		}

	}
	break;

	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83: {

		short dest_reg, mod,word_op,sub_code;
		void *dst_mp;
		short dest_is_reg;

		mod_byte = *imp++;
		dest_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		sub_code = (mod_byte >> 3) & 0x07;
		dest_is_reg = 0;
		word_op = opcode & 0x01 ;
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is a REGISTER to REGISTER Transfer */
				dest_is_reg = 1;
				if (word_op) {
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_ax;
										break;
						case REG_BX:	dst_mp = &r_bx;
										break;
						case REG_CX:	dst_mp = &r_cx;
										break;
						case REG_DX:	dst_mp = &r_dx;
										break;
						case REG_SP:	dst_mp = &r_sp;
										break;
						case REG_BP:	dst_mp = &r_bp;
										break;
						case REG_SI:	dst_mp = &r_si;
										break;
						case REG_DI:	dst_mp = &r_di;
										break;
						default:		break;			/* can't happen */
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	dst_mp = &r_al;
										break;
						case REG_BL:	dst_mp = &r_bl;
										break;
						case REG_CL:	dst_mp = &r_cl;
										break;
						case REG_DL:	dst_mp = &r_dl;
										break;
						case REG_AH:	dst_mp = &r_ah;
										break;
						case REG_BH:	dst_mp = &r_bh;
										break;
						case REG_CH:	dst_mp = &r_ch;
										break;
						case REG_DH:	dst_mp = &r_dh;
										break;
						default:		break;			/* can't happen */
					}
				}
			break;


			case 00:
				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
				break;
		}
		if (opcode & 0x01) {		/* Word operation */
			if (opcode & 0x02) {	/* sign extend , only 8 bits present */
				tmp_16 = *(char *)imp++;		/* get the data */
				if (tmp_16 & 0x80) {
					tmp_16 |= 0xff00;		/* This should be sign extended by the compiler, but let's be safe */
				}
			}
			else {
				tmp_16 = *(short *)imp;		/* get the data */
				imp+=2;
			}
		}
		else {
			tmp_8 = *imp++;
		}

		if (word_op) {
			switch(sub_code) {
				case 000:	tmp_32 = *(short *)dst_mp;						/* ADD Immediate 16 bit */
							sign = tmp_32 & 0x8000;
							tmp_32 += tmp_16;
							*(unsigned short *)dst_mp = (tmp_32 & 0xffff);
							SET_OF_SF(sign,tmp_32); SET_ZF_PF(tmp_32); SET_CF(tmp_32);
							break;

				case 001:	*(unsigned short *)dst_mp |= tmp_16;		/* OR Immediate 16 bit */
							SET_ZF_PF((*(unsigned short *)dst_mp));
							SET_SF((*(unsigned short *)dst_mp));
							CF = OF = 0;
							break;

				case 002:	tmp_32 = *(short *)dst_mp;						/* ADC Immediate 16 bit */
							sign = tmp_32 & 0x8000;
							tmp_32 += (tmp_16 + CF);
							*(unsigned short *)dst_mp = (tmp_32 & 0xffff);
							SET_OF_SF(sign,tmp_32); SET_ZF_PF(tmp_32); SET_CF(tmp_32);
							break;

				case 003:	tmp_32 = *(short *)dst_mp;						/* SBB Immediate 16 bit */
							sign = tmp_32 & 0x8000;
							tmp_32 -= (tmp_16 + CF);
							*(unsigned short *)dst_mp = (tmp_32 & 0xffff);
							SET_OF_SF(sign,tmp_32); SET_ZF_PF(tmp_32); SET_CF(tmp_32);
							break;

				case 004:	*(unsigned short *)dst_mp &= tmp_16;		/* AND Immediate 16 bit */
							SET_ZF_PF((*(unsigned short *)dst_mp));
							SET_SF((*(unsigned short *)dst_mp));
							CF = OF = 0;
							break;

				case 005:	tmp_32 = *(short *)dst_mp;						/* SUB Immediate 16 bit */
							sign = tmp_32 & 0x8000;
							tmp_32 -= tmp_16;
							*(unsigned short *)dst_mp = (tmp_32 & 0xffff);
							SET_OF_SF(sign,tmp_32); SET_ZF_PF(tmp_32); SET_CF(tmp_32);
							break;

				case 006:	*(unsigned short *)dst_mp ^= tmp_16;		/* XOR Immediate 16 bit */
							SET_ZF_PF((*(unsigned short *)dst_mp));
							SET_SF((*(unsigned short *)dst_mp));
							CF = OF = 0;
							break;

				case 007:	tmp_32 = *(short *)dst_mp;
							sign -= tmp_32 & 0x8000;
							tmp_32 -= tmp_16;						/* CMP Immediate 16 bit */
							SET_OF_SF(sign,tmp_32); SET_ZF_PF(tmp_32);SET_CF(tmp_32);
							break;

				default:	break;				/* ERROR CONDITION, Cannot happen */
			}
		}
		else {
			switch(sub_code) {
				case 000:	tmp_16 = *(char *)dst_mp;						/* ADD Immediate 8 bit */
							sign = tmp_16 & 0x80;
							tmp_16 += tmp_8;
							*(unsigned char *)dst_mp = (tmp_16 & 0xff);
							SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
							break;

				case 001:	*(unsigned char *)dst_mp |= tmp_8;		/* OR Immediate 8 bit */
							SET_ZF_PF_8((*(unsigned char *)dst_mp));
							SET_SF_8((*(unsigned char *)dst_mp));
							CF = OF = 0;
							break;

				case 002:	tmp_16 = *(char *)dst_mp;						/* ADC Immediate 8 bit */
							sign = tmp_16 & 0x80;
							tmp_16 += (tmp_8 + CF);
							*(unsigned char *)dst_mp = (tmp_16 & 0xff);
							SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
							break;

				case 003:	tmp_16 = *(char *)dst_mp;						/* SBB Immediate 8 bit */
							sign = tmp_16 & 0x80;
							tmp_16 -= (tmp_8 + CF);
							*(unsigned char *)dst_mp = (tmp_16 & 0xff);
							SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
							break;

				case 004:	*(unsigned char *)dst_mp &= tmp_8;		/* AND Immediate 8 bit */
							SET_ZF_PF_8((*(unsigned char *)dst_mp));
							SET_SF_8((*(unsigned char *)dst_mp));
							CF = OF = 0;
							break;

				case 005:	tmp_16 = *(char *)dst_mp;						/* SUB Immediate 8 bit */
							sign = tmp_16 & 0x80;
							tmp_16 -= tmp_8;
							*(unsigned char *)dst_mp = (tmp_16 & 0xff);
							SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
							break;

				case 006:	*(unsigned char *)dst_mp ^= tmp_8;		/* XOR Immediate 8 bit */
							SET_ZF_PF_8((*(unsigned char *)dst_mp));
							SET_SF_8((*(unsigned char *)dst_mp));
							CF = OF = 0;
							break;

				case 007:	tmp_16 = *(char *)dst_mp;						/* CMP Immediate 8 bit */
							sign = tmp_16 & 0x80;
							tmp_16 -= tmp_8;
							SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
							break;

				default:	break;				/* ERROR CONDITION, Cannot happen */
			}
		}

		if (dest_is_reg) {
			if (word_op) {
				SET_ALL_BREGS();
				smp = SET_NEW_SMP();
			}
			else {
				SET_ALL_WREGS();
			}
		}
	}
	break;


	case OPSIZE:		/* Operand size override */			/* 386 instructions */
				if ( decode_386() ) {
					return;		/*	unknown op-code	*/
				}	/*	decode_386 returns 0 for ALL OK, continue	*/
				goto top_loop;
				break;

	case 0x0f:		/* Enhanced 386 instructions */
			switch(*imp++) {
			case JA+0x10:	disp_16 = *(short *)imp;
							imp+=2;
							if (CF == ZF && ZF == 0) {
								r_ip = GET_IP() + disp_16;
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							break;

			case JAE+0x10:	disp_16 = *(short *)imp;
							imp+=2;
							if (CF == 0) {
								r_ip = GET_IP() + disp_16;
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							break;

			case JB+0x10:	disp_16 = *(short *)imp;
							imp+=2;
							if (CF == 1) {
								r_ip = GET_IP() + disp_16;
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							break;

			case JBE+0x10:	disp_16 = *(short *)imp;
							imp+=2;
							if (CF == 1 || ZF == 1) {
								r_ip = GET_IP() + disp_16;
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							break;


			case JE+0x10:	disp_16 = *(short *)imp;
							imp+=2;
							if (ZF == 1) {
								r_ip = GET_IP() + disp_16;
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							break;

			case JG+0x10:	disp_16 = *(short *)imp;
							imp+=2;
							if (ZF == 0 && (OF == SF)) {
								r_ip = GET_IP() + disp_16;
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							break;

			case JGE+0x10:	disp_16 = *(short *)imp;
							imp+=2;
							if (OF == SF) {
								r_ip = GET_IP() + disp_16;
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							break;

			case JL+0x10:	disp_16 = *(short *)imp;
							imp+=2;
							if (OF != SF) {
								r_ip = GET_IP() + disp_16;
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							break;

			case JLE+0x10:	disp_16 = *(short *)imp;
							imp+=2;
							if (ZF == 1 || (OF != SF)) {
								r_ip = GET_IP() + disp_16;
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							break;


			case JNE+0x10:	disp_16 = *(short *)imp;
							imp+=2;
							if (ZF == 0) {
								r_ip = GET_IP() + disp_16;
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							break;

			case JNP+0x10:	disp_16 = *(short *)imp;
							imp+=2;
							if (PF == 0) {
								r_ip = GET_IP() + disp_16;
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							break;

			case JNS+0x10:	disp_16 = *(short *)imp;
							imp+=2;
							if (SF == 0) {
								r_ip = GET_IP() + disp_16;
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							break;

			case JO+0x10:	disp_16 = *(short *)imp;
							imp+=2;
							if (OF == 1) {
								r_ip = GET_IP() + disp_16;
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							break;

			case JNO+0x10:	disp_16 = *(short *)imp;
							imp+=2;
							if (OF == 0) {
								r_ip = GET_IP() + disp_16;
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							break;

			case JP+0x10:	disp_16 = *(short *)imp;
							imp+=2;
							if (PF == 1) {
								r_ip = GET_IP() + disp_16;
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							break;

			case JS+0x10:	disp_16 = *(short *)imp;
							imp+=2;
							if (SF == 1) {
								r_ip = GET_IP() + disp_16;
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							break;

			case MOVSX_B:		/*	S000 vvv	*/
			case MOVSX_W:
			case MOVZX_B:
			case MOVZX_W: {
		short word_op ,source_reg ,dest_reg ,mod, sign, sign_extend;
		void *dst_mp,*src_mp;

		word_op =(*(imp-1) & 0x01);	/* select MOV?X_B or MOV?X_W	*/
		sign_extend =(*(imp-1) & 0x08);	/* select MOVSX_? or MOVZX_?	*/
		mod_byte = *imp++;
		source_reg = mod_byte & 0x07;	/* Get source register or R/M field */
		dest_reg = (mod_byte >> 3) & 0x07;		/* Get Destination register */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		switch(mod) {		/* Handle type of operation */
				case 00:
					switch(source_reg) {			/* r/m */
						case 000:	if (new_seg) 
										dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
									else
										dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
									break;
						case 001:	if (new_seg)
										dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
									else
										dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
									break;
						case 002:	if (new_seg) 
										dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
									else
										dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
									break;
						case 003:	if (new_seg)
										dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
									else
										dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
									break;
						case 004:	if (new_seg)
										dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
									else
										dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
									break;
						case 005:	if (new_seg)
										dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
									else
										dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
									break;
						case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
									imp+=2;
									if (new_seg)
										dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
									else
										dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
									break;
						case 007:	if (new_seg)
										dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
									else
										dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
									break;
						}	/*	switch(source_reg)	*/
				break;	/*	case 0x00:	*/
				case 01:
					disp_8 = *imp++;

					switch(source_reg) {			/* r/m */
						case 000:	if (new_seg)
										dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
									else
										dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
									break;
						case 001:	if (new_seg)
										dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
									else
										dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
									break;
						case 002:	if (new_seg)
										dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
									else
										dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
									break;
						case 003:	if (new_seg)
										dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
									else
										dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
									break;
						case 004:	if (new_seg)
										dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
									else
										dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
									break;
						case 005:	if (new_seg)
										dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
									else
										dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
									break;
						case 006:	if (new_seg)
										dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
									else
										dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
									break;
						case 007:	if (new_seg)
										dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
									else
										dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
									break;
						}	/*	switch(source_reg)	*/
				break;	/*	case 0x01:	*/
			case 0x02:	/*	reg <- mem byte/word, 16 bit displ */
					disp_u16 = *(unsigned short *)imp;
					imp+=2;

					switch(source_reg) {			/* r/m */
						case 000:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
									else
										src_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
									break;
						case 001:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
									else
										src_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
									break;
						case 002:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
									else
										src_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
									break;
						case 003:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
									else
										src_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
									break;
						case 004:	if (new_seg)
										dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
									else
										dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
									break;
						case 005:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
									else
										src_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
									break;
						case 006:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
									else
										src_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
									break;
						case 007:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
									else
										src_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
									break;
					}	/*	switch(source_reg)	*/
				break;	/*	case 0x02:	*/
			case 03:		/* This is a REGISTER to REGISTER Transfer */
				if (word_op) {	/*	8 or 16 bit source	*/

					switch(source_reg) {
						case REG_AX:	src_mp = &r_ax;
										break;
						case REG_BX:	src_mp = &r_bx;
										break;
						case REG_CX:	src_mp = &r_cx;
										break;
						case REG_DX:	src_mp = &r_dx;
										break;
						case REG_SP:	src_mp = &r_sp;
										break;
						case REG_BP:	src_mp = &r_bp;
										break;
						case REG_SI:	src_mp = &r_si;
										break;
						case REG_DI:	src_mp = &r_di;
										break;
					}
				} else {
					switch(source_reg) {
						case REG_AL:	src_mp = &r_al;
										break;
						case REG_BL:	src_mp = &r_bl;
										break;
						case REG_CL:	src_mp = &r_cl;
										break;
						case REG_DL:	src_mp = &r_dl;
										break;
						case REG_AH:	src_mp = &r_ah;
										break;
						case REG_BH:	src_mp = &r_bh;
										break;
						case REG_CH:	src_mp = &r_ch;
										break;
						case REG_DH:	src_mp = &r_dh;
										break;
					}
				}
				break;	/*	case 03:		*/
				}	/*	switch(mod)	*/
			if (word_op) {
				tmp_u32 = 0x0000ffff & ( *(unsigned short *)src_mp );
				if ( sign_extend ) {
					sign = 0x00008000 & tmp_u32;
					if( sign ) tmp_u32 |= 0xffff0000;
				}
			} else {
				tmp_u32 = 0x000000ff & ( *(unsigned char *)src_mp );
				if ( sign_extend ) {
					sign = 0x00000080 & tmp_u32;
					if( sign ) tmp_u32 |= 0xffffff00;
				}
			}
				/*	Always transfer to 32 bit destination because it is	*/
				/*	simpler to do, and these are sign extend or zero extend */
				/*	instructions anyway, so the whole register must be filled */
			switch(dest_reg) {
				case REG_AX:	r_eax = tmp_u32;
								SET_AX_AL_AH();
								break;
				case REG_BX:	r_ebx = tmp_u32;
								SET_BX_BL_BH();
								break;
				case REG_CX:	r_ecx = tmp_u32;
								SET_CX_CL_CH();
								break;
				case REG_DX:	r_edx = tmp_u32;
								SET_DX_DL_DH();
								break;
				case REG_SP:	r_esp = tmp_u32;
							r_sp = (unsigned short) ( 0x0000ffff & r_esp);
								break;
				case REG_BP:	r_ebp = tmp_u32;
							r_bp = (unsigned short) ( 0x0000ffff & r_ebp);
								break;
				case REG_SI:	r_esi = tmp_u32;
							r_si = (unsigned short) ( 0x0000ffff & r_esi);
								break;
				case REG_DI:	r_edi = tmp_u32;
								r_di = (unsigned short) ( 0x0000ffff & r_edi);
								break;
			}	/*	switch(dest_reg)	*/
			}	/*	case MOVSX_B MOVSX_W MOVZX_B MOVZX_W */
				break;			/*	S000 ^^^	*/
			default:
    ErrorF("Unknown 386 16 bit opcode %02x at %04x:%04x\n",opcode,r_cs,(GET_IP()-2));
				return;				/*	S000	*/
							break;
		}	/*	switch(*imp++) for Enhanced 386 instructions */
		break;

	default:
	ErrorF("main_loop: Unknown opcode %02x at %04x:%04x\n",opcode,r_cs,(GET_IP()-1));
				return;				/*	S000	*/
				break;
	}	/*	switch ((opcode = *imp++))	 main decode loop */
	rep_flag = new_seg = 0;
	opsize = 0;
	r_ip = GET_IP();
	goto top_loop;

}

/*
 * This routine decodes the 80386 instruction stream emulating
 *	the instructions.
 *	Returns 0 for successful execution, 1 for unknown op-code
 *
*/
int decode_386()
{

top_loop:

#ifdef DEBUG_INTERP
	++instrs_counted;
#endif

	if( INTERP_DEBUG )
		RegisterDisplay();

	switch ((opcode = *imp++)) {		/* main decode loop */
		case ADC_AX:	SET_EAX();
						tmp_32l = r_eax & 0xffff;
						tmp_32h = (r_eax >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l += (*(short *)imp + CF);				/* Add with Carry, immediate EAX */
						imp += 2;
						tmp_32h += (*(short *)imp + ((tmp_32l & 0xf0000) ? 1: 0));
						imp += 2;
						SET_WB_REGS_FROM_TMP(r_eax,r_ax,r_al,r_ah,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_eax); SET_CF(tmp_32h);
						break;

		case ADD_AX:	SET_EAX();
						tmp_32l = r_eax & 0xffff;
						tmp_32h = (r_eax >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l += *(short *)imp;				/* Add , immediate EAX */
						imp += 2;
						tmp_32h += (*(short *)imp + ((tmp_32l & 0xf0000) ? 1: 0));
						imp += 2;
						SET_WB_REGS_FROM_TMP(r_eax,r_ax,r_al,r_ah,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_eax); SET_CF(tmp_32h);
						break;

		case AND_AX:	SET_EAX();
						r_eax &= *(unsigned long *)imp;
						imp+=4;
						SET_SF_32(r_eax); SET_ZF_PF(r_eax);
						SET_WB_REGS(r_eax,r_ax,r_al,r_ah);
						CF = OF = 0;
						break;

		case CMP_AX:	tmp_32l = r_eax & 0xffff;
						tmp_32h = (r_eax >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l -= *(short *)imp;				/* Add , immediate EAX */
						imp += 2;
						tmp_32h -= (*(short *)imp + ((tmp_32l & 0xf0000) ? 1: 0));
						imp += 2;
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(((tmp_32l & 0xffff) | ((tmp_32h & 0xffff) << 16))); SET_CF(tmp_32h);
						break;


		case CWD:		SET_EAX();
						if (r_eax & 0x80000000)	{		/* Sign extend EAX into EDX */
							r_edx = 0xffffffff;
							r_dx  = 0xffff;
						}
						else
							r_edx = r_dx = 0;
						SET_DL_DH();
						break;


		case DEC_AX:	SET_EAX();
						tmp_32l = r_eax & 0xffff;
						tmp_32h = (r_eax >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l--;
						tmp_32h -= ((tmp_32l & 0xf0000) ? 1: 0);
						SET_WB_REGS_FROM_TMP(r_eax,r_ax,r_al,r_ah,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_eax);
						break;

		case DEC_BX:	SET_EBX();
						tmp_32l = r_ebx & 0xffff;
						tmp_32h = (r_ebx >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l--;
						tmp_32h -= ((tmp_32l & 0xf0000) ? 1: 0);
						SET_WB_REGS_FROM_TMP(r_ebx,r_bx,r_bl,r_bh,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_ebx);
						break;

		case DEC_CX:	SET_ECX();
						tmp_32l = r_ecx & 0xffff;
						tmp_32h = (r_ecx >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l--;
						tmp_32h -= ((tmp_32l & 0xf0000) ? 1: 0);
						SET_WB_REGS_FROM_TMP(r_ecx,r_cx,r_cl,r_ch,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_ecx);
						break;

		case DEC_DX:	SET_EDX();
						tmp_32l = r_edx & 0xffff;
						tmp_32h = (r_edx >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l--;
						tmp_32h -= ((tmp_32l & 0xf0000) ? 1: 0);
						SET_WB_REGS_FROM_TMP(r_edx,r_dx,r_dl,r_dh,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_edx);
						break;

		case DEC_SI:	SET_ESI();
						tmp_32l = r_esi & 0xffff;
						tmp_32h = (r_esi >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l--;
						tmp_32h -= ((tmp_32l & 0xf0000) ? 1: 0);
						SET_WREGS_FROM_TMP(r_esi,r_si,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_esi);
						break;

		case DEC_DI:	SET_EDI();
						tmp_32l = r_edi & 0xffff;
						tmp_32h = (r_edi >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l--;
						tmp_32h -= ((tmp_32l & 0xf0000) ? 1: 0);
						SET_WREGS_FROM_TMP(r_edi,r_di,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_edi);
						break;

		case DEC_SP:	SET_ESP();
						tmp_32l = r_esp & 0xffff;
						tmp_32h = (r_esp >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l--;
						tmp_32h -= ((tmp_32l & 0xf0000) ? 1: 0);
						SET_WREGS_FROM_TMP(r_esp,r_sp,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_esp);
						break;

		case DEC_BP:	SET_EBP();
						tmp_32l = r_ebp & 0xffff;
						tmp_32h = (r_ebp >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l--;
						tmp_32h -= ((tmp_32l & 0xf0000) ? 1: 0);
						SET_WREGS_FROM_TMP(r_ebp,r_bp,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_ebp);
						break;

		case INC_AX:	SET_EAX();
						tmp_32l = r_eax & 0xffff;
						tmp_32h = (r_eax >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l++;
						tmp_32h += ((tmp_32l & 0xf0000) ? 1: 0);
						SET_WB_REGS_FROM_TMP(r_eax,r_ax,r_al,r_ah,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_eax);
						break;

		case INC_BX:	SET_EBX();
						tmp_32l = r_ebx & 0xffff;
						tmp_32h = (r_ebx >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l++;
						tmp_32h += ((tmp_32l & 0xf0000) ? 1: 0);
						SET_WB_REGS_FROM_TMP(r_ebx,r_bx,r_bl,r_bh,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_ebx);
						break;

		case INC_CX:	SET_ECX();
						tmp_32l = r_ecx & 0xffff;
						tmp_32h = (r_ecx >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l++;
						tmp_32h += ((tmp_32l & 0xf0000) ? 1: 0);
						SET_WB_REGS_FROM_TMP(r_ecx,r_cx,r_cl,r_ch,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_ecx);
						break;

		case INC_DX:	SET_EDX();
						tmp_32l = r_edx & 0xffff;
						tmp_32h = (r_edx >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l++;
						tmp_32h += ((tmp_32l & 0xf0000) ? 1: 0);
						SET_WB_REGS_FROM_TMP(r_edx,r_dx,r_dl,r_dh,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_edx);
						break;

		case INC_SP:	SET_ESP();
						tmp_32l = r_esp & 0xffff;
						tmp_32h = (r_esp >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l++;
						tmp_32h += ((tmp_32l & 0xf0000) ? 1: 0);
						SET_WREGS_FROM_TMP(r_esp,r_sp,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_esp);
						break;

		case INC_BP:	SET_EBP();
						tmp_32l = r_ebp & 0xffff;
						tmp_32h = (r_ebp >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l++;
						tmp_32h += ((tmp_32l & 0xf0000) ? 1: 0);
						SET_WREGS_FROM_TMP(r_ebp,r_bp,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_ebp);
						break;

		case INC_SI:	SET_ESI();
						tmp_32l = r_esi & 0xffff;
						tmp_32h = (r_esi >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l++;
						tmp_32h += ((tmp_32l & 0xf0000) ? 1: 0);
						SET_WREGS_FROM_TMP(r_esi,r_si,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_esi);
						break;

		case INC_DI:	SET_EDI();
						tmp_32l = r_edi & 0xffff;
						tmp_32h = (r_edi >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l++;
						tmp_32h += ((tmp_32l & 0xf0000) ? 1: 0);
						SET_WREGS_FROM_TMP(r_edi,r_di,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_edi);
						break;

		case LODSW:		if (new_seg) 
							dmp_32 = SET_NEW_DMP(seg_ovr,r_si,0,0);	
						else
							dmp_32 = SET_NEW_DMP(r_ds,0,r_si,0);
						r_eax = *(unsigned long *)dmp_32;
						if (DF)
							r_si -= 4;
						else
							r_si += 4;
						SET_AX_AL_AH();
						break;

		case XOR_IMMAX:		SET_EAX();
							r_eax ^= *(unsigned long *)imp;
							imp+=4;
							SET_SF_32(r_eax); SET_ZF_PF(r_eax); SET_AX_AL_AH();
							OF = CF = 0;
							break;

		case OR_IMMAX:		SET_EAX();
							r_eax |= *(unsigned long *)imp;
							imp+=4;
							SET_AX_AL_AH(); SET_SF_32(r_eax); SET_ZF_PF(r_ax);
							break;

		case MOVAX_MEM:		disp_u16 = *(unsigned short *)imp;
							imp+=2;
							if (new_seg) 
								dmp_32 = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
							else
								dmp_32 = SET_NEW_DMP(r_ds,disp_u16,0,0);
							r_eax = *(unsigned long *)dmp_32;
							SET_AX_AL_AH();
							break;

		case MOVMEM_AX:		SET_EAX();
							disp_u16 = *(unsigned short *)imp;
							imp+=2;
							if (new_seg) 
								dmp_32 = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
							else
								dmp_32 = SET_NEW_DMP(r_ds,disp_u16,0,0);
							*dmp_32 = r_eax;
							break;

		case MOVAX_IM:		r_eax = *(unsigned long *)imp;
							imp+=4;
							SET_AX_AL_AH();
							break;

		case MOVBX_IM:		r_ebx = *(unsigned long *)imp;
							imp+=4;
							SET_BX_BL_BH();
							break;

		case MOVCX_IM:		r_ecx = *(unsigned long *)imp;
							imp+=4;
							SET_CX_CL_CH();
							break;

		case MOVDX_IM:		r_edx = *(unsigned long *)imp;
							imp+=4;
							SET_DX_DL_DH();
							break;

		case MOVSP_IM:		r_esp = *(unsigned long *)imp;
							imp+=4;
							r_sp = r_esp & 0xffff;
							smp = SET_NEW_SMP();
							break;

		case MOVBP_IM:		r_ebp = *(unsigned long *)imp;
							imp+=4;
							r_bp = r_ebp & 0xffff;
							break;

		case MOVSI_IM:		r_esi = *(unsigned long *)imp;
							imp+=4;
							r_si = r_esi & 0xffff;
							break;

		case MOVDI_IM:		r_edi = *(unsigned long *)imp;
							r_di = r_edi & 0xffff;
							imp+=4;
							break;

		case MOVSW:			dmp_32 = SET_NEW_DMP(r_es,0,r_di,0);
							if (new_seg)
								tmp_seg = seg_ovr;
							else
								tmp_seg = r_ds;
							s_dmp_32 = SET_NEW_DMP(tmp_seg,0,r_si,0);
							if (rep_flag) {
								while(r_cx--) {
									*dmp_32 = *s_dmp_32;
									if (DF) {
										r_di -= 4;
										r_si -= 4;
									}
									else {
										r_di += 4;
										r_si += 4;
									}
									dmp_32 = SET_NEW_DMP(r_es,0,r_di,0);
									s_dmp_32 = SET_NEW_DMP(tmp_seg,0,r_si,0);
								}
								r_cx = 0;
								SET_CL_CH();
							}
							else {
								*dmp_32 = *s_dmp_32;
								if (DF) {
									r_di -= 4;
									r_si -= 4;
								}
								else {
									r_di += 4;
									r_si += 4;
								}
							}
							break;


		case INW_DX:
		case INW:
		case OUTDX_AX:
			if ( PerformIO( opcode, 1 ) ) {
					return;	/*	something is wrong, can't do it	*/
			}
				break;	/*	all done OK	*/

		case PUSHAX:	SET_EAX();
						r_sp -= 4;
						smp = SET_NEW_SMP();
						*(unsigned long *)smp = r_eax;
						break;

		case PUSHBX:	SET_EBX();
						r_sp -= 4;
						smp = SET_NEW_SMP();
						*(unsigned long *)smp = r_ebx;
						break;

		case PUSHCX:	SET_ECX();
						r_sp -= 4;
						smp = SET_NEW_SMP();
						*(unsigned long *)smp = r_ecx;
						break;

		case PUSHDX:	SET_EDX();
						r_sp -= 4;
						smp = SET_NEW_SMP();
						*(unsigned long *)smp = r_edx;
						break;

		case PUSHSP:	SET_ESP();
						r_sp -= 4;
						r_esp -= 4;
						smp = SET_NEW_SMP();
						*(unsigned long *)smp = r_esp+4;
						break;

		case PUSHBP:	SET_EBP();
						r_sp -= 4;
						smp = SET_NEW_SMP();
						*(unsigned long *)smp = r_ebp;
						break;

		case PUSHSI:	SET_ESI();
						r_sp -= 4;
						smp = SET_NEW_SMP();
						*(unsigned long *)smp = r_esi;
						break;

		case PUSHDI:	SET_EDI();
						r_sp -= 4;
						smp = SET_NEW_SMP();
						*(unsigned long *)smp = r_edi;
						break;

		case PUSHALL:	SET_ALL_EREGS();
						tmp_u32 = r_sp;								/* NEED TO ADD opsize override */
						r_sp -=32;
						smp = SET_NEW_IMP(r_ss,(tmp_u32 - 4));
						*lsmp = r_eax;
						*(lsmp-1) = r_ecx;
						*(lsmp-2) = r_edx;
						*(lsmp-3) = r_ebx;
						*(lsmp-4) = tmp_u32;
						*(lsmp-5) = r_ebp;
						*(lsmp-6) = r_esi;
						*(lsmp-7) = r_edi;
						break;

		case POPAX:		r_eax = *(unsigned long *)smp;
						r_sp +=4;
						SET_AX_AL_AH();
						smp = SET_NEW_SMP();
						break;

		case POPBX:		r_ebx = *(unsigned long *)smp;
						r_sp +=4;
						SET_BX_BL_BH();
						smp = SET_NEW_SMP();
						break;

		case POPCX:		r_ecx = *(unsigned long *)smp;
						r_sp +=4;
						SET_CX_CL_CH();
						smp = SET_NEW_SMP();
						break;

		case POPDX:		r_edx = *(unsigned long *)smp;
						r_sp +=4;
						SET_DX_DL_DH();
						smp = SET_NEW_SMP();
						break;

		case POPSI:		r_esi = *(unsigned long *)smp;
						r_sp +=4;
						smp = SET_NEW_SMP();
						r_si = r_esi & 0xffff;
						break;

		case POPDI:		r_edi = *(unsigned long *)smp;
						r_sp +=4;
						smp = SET_NEW_SMP();
						r_di = r_edi & 0xffff;
						break;

		case POPBP:		r_ebp = *(unsigned long *)smp;
						r_sp +=4;
						r_bp = r_ebp & 0xffff;
						smp = SET_NEW_SMP();
						break;

		case POPSP:		r_esp = *lsmp;					/* POP SP does NOT increment the SP reg ! */
						r_sp = r_esp & 0xffff;
						smp = SET_NEW_SMP();
						break;

		case POPALL:	smp = SET_NEW_SMP();			/* ADD Opsize override here */
						r_edi = *lsmp;
						r_esi = *(lsmp+1);
						r_ebp = *(lsmp+2);
						r_ebx = *(lsmp+4);
						r_edx = *(lsmp+5);
						r_ecx = *(lsmp+6);
						r_eax = *(lsmp+7);
						r_sp += 32;
						smp = SET_NEW_SMP();
						r_ax = r_eax & 0xffff; r_bx = r_ebx & 0xffff; r_cx = r_ecx & 0xffff; r_dx = r_edx & 0xffff;
						r_si = r_esi & 0xffff; r_di = r_edi & 0xffff; r_bp = r_ebp & 0xffff;
						SET_ALL_BREGS();
						break;

		case STOSW:		SET_EAX();
						dmp_32 = SET_NEW_DMP(r_es,0,r_di,0);
						if (rep_flag) {
							while(r_cx--) {
								*dmp_32 = r_eax;
								if (DF)
									r_di -= 4;
								else
									r_di += 4;
								dmp_32 = SET_NEW_DMP(r_es,0,r_di,0);
							}
							r_cx = 0;
							SET_CL_CH();
						}
						else  {
							*dmp_32 = r_eax;
							if (DF)
								r_di -= 4;
							else
								r_di += 4;
						}
						break;

		case SUB_AX:	SET_EAX();
						tmp_32l = r_eax & 0xffff;
						tmp_32h = (r_eax >> 16);
						sign = tmp_32h & 0x8000;
						tmp_32l -= *(short *)imp;				/* Add , immediate EAX */
						imp += 2;
						tmp_32h -= (*(short *)imp + ((tmp_32l & 0xf0000) ? 1: 0));
						imp += 2;
						SET_WB_REGS_FROM_TMP(r_eax,r_ax,r_al,r_ah,tmp_32h,tmp_32l);
						SET_OF_SF(sign,tmp_32h); SET_ZF_PF(r_eax); SET_CF(tmp_32h);
						break;

		case TEST_AX:	tmp_u32 = r_eax & *(long *)imp;
						imp+=4;
						SET_ZF_PF(tmp_u32); SET_SF_32(tmp_u32);
						CF = OF = 0;
						break;

		case XCHGBX:	SWAP(r_ax,r_bx);
						SET_EAX(); SET_EBX();
						SET_BL_BH();
						SET_AL_AH();
						break;

		case XCHGCX:	SWAP(r_ax,r_cx);
						SET_EAX(); SET_ECX();
						SET_AL_AH();
						SET_CL_CH();
						break;

		case XCHGDX:	SWAP(r_ax,r_dx);
						SET_EAX(); SET_EDX();
						SET_AL_AH();
						SET_DL_DH();
						break;

		case XCHGSP:	SWAP(r_ax,r_sp);
						SET_EAX(); SET_ESP();
						SET_AL_AH();
						smp = SET_NEW_SMP();
						break;

		case XCHGBP:	SWAP(r_ax,r_bp);
						SET_EAX(); SET_EBP();
						SET_AL_AH();
						break;

		case XCHGSI:	SWAP(r_ax,r_si);
						SET_EAX(); SET_ESI();
						SET_AL_AH();
						break;

		case XCHGDI:	SWAP(r_ax,r_di);
						SET_EAX(); SET_EDI();
						SET_AL_AH();
						break;

		case SEGOVRCS:	new_seg = 1;
						seg_ovr = r_cs;
						goto top_loop;		/* NO BREAK HERE!  PROCESS NEXT INSTRUCTION, OTHERWISE new_seg gets reset! */
						break;		/* never get here */

		case SEGOVRDS:	new_seg = 1;
						seg_ovr = r_ds;
						goto top_loop;		/* NO BREAK HERE!  PROCESS NEXT INSTRUCTION, OTHERWISE new_seg gets reset! */
						break;		/* never get here */

		case SEGOVRES:	new_seg = 1;
						seg_ovr = r_es;
						goto top_loop;		/* NO BREAK HERE!  PROCESS NEXT INSTRUCTION, OTHERWISE new_seg gets reset! */
						break;		/* never get here */

		case SEGOVRSS:	new_seg = 1;
						seg_ovr = r_ss;
						goto top_loop;		/* NO BREAK HERE!  PROCESS NEXT INSTRUCTION, OTHERWISE new_seg gets reset! */
						break;		/* never get here */

		case SCASW:		dmp_16 = SET_NEW_DMP(r_es,0,r_di,0);
						if (rep_flag) {
							while(r_cx--) {
								tmp_32l = r_eax & 0xffff;
								tmp_32h = (r_eax >> 16);
								sign = tmp_32h & 0x8000;
								tmp_32l -= *(short *)dmp_16;				/* Add , immediate EAX */
								tmp_32h -= (*(short *)(dmp_16+1) + ((tmp_32l & 0xf0000) ? 1: 0));
								SET_OF_SF(sign,tmp_32h); SET_ZF_PF(((tmp_32l & 0xffff) | ((tmp_32h & 0xffff) << 16)));
								SET_CF(tmp_32h);
								if (DF)
									r_di -= 4;
								else
									r_di += 4;
								if (rep_flag == REPZ && ZF) {
									SET_CL_CH();
									break;			/* Break out of loop */
								}
								else if (rep_flag == REPNZ && ZF == 0) {
									SET_CL_CH();
									break;			/* Break out of loop */
								}
								dmp_16 = SET_NEW_DMP(r_es,0,r_di,0);
							}
							r_cx = 0;
							SET_CL_CH();
						}
						else {
							tmp_32l = r_eax & 0xffff;
							tmp_32h = (r_eax >> 16);
							sign = tmp_32h & 0x8000;
							tmp_32l -= *(short *)dmp_16;				/* Add , immediate EAX */
							tmp_32h -= (*(short *)(dmp_16+1) + ((tmp_32l & 0xf0000) ? 1: 0));
							SET_OF_SF(sign,tmp_32h); SET_ZF_PF(((tmp_32l & 0xffff) | ((tmp_32h & 0xffff) << 16)));
							SET_CF(tmp_32h);
							if (DF)
								r_di -= 4;
							else
								r_di += 4;
						}
						break;


		case CMPSW:		dmp_16 = SET_NEW_DMP(r_es,0,r_di,0);
						if (new_seg)
							tmp_seg = seg_ovr;
						else
							tmp_seg = r_ds;
						s_dmp_16 = SET_NEW_DMP(tmp_seg,0,r_si,0);
						if (rep_flag) {
							while(r_cx--) {
								tmp_32l = *s_dmp_16;
								tmp_32h = *(s_dmp_16+1);
								sign = tmp_32h & 0x8000;
								tmp_32l -= *(short *)dmp_16;				/* Add , immediate EAX */
								tmp_32h -= (*(short *)(dmp_16+1) + ((tmp_32l & 0xf0000) ? 1: 0));
								SET_OF_SF(sign,tmp_32h); SET_ZF_PF(((tmp_32l & 0xffff) | ((tmp_32h & 0xffff) << 16)));
								SET_CF(tmp_32h);
								if (DF) {
									r_di -= 4;
									r_si -= 4;
								}
								else {
									r_di += 4;
									r_si += 4;
								}
								if (rep_flag == REPZ && ZF) {
									TRUNC_16(r_di);
									TRUNC_16(r_si);
									SET_CL_CH();
									break;			/* Break out of loop */
								}
								else {
									if (rep_flag == REPNZ && ZF == 0) {
										TRUNC_16(r_di);
										TRUNC_16(r_si);
										SET_CL_CH();
										break;
									}
								}
								dmp_16 = SET_NEW_DMP(r_es,0,r_di,0);
								s_dmp_16 = SET_NEW_DMP(tmp_seg,0,r_si,0);
							}
							r_cx = 0;
							TRUNC_16(r_di);
							TRUNC_16(r_si);
							SET_CL_CH();
						}
						else {
							tmp_32l = *s_dmp_16;
							tmp_32h = *(s_dmp_16+1);
							sign = tmp_32h & 0x8000;
							tmp_32l -= *(short *)dmp_16;				/* Add , immediate EAX */
							tmp_32h -= (*(short *)(dmp_16+1) + ((tmp_32l & 0xf0000) ? 1: 0));
							SET_OF_SF(sign,tmp_32h); SET_ZF_PF(((tmp_32l & 0xffff) | ((tmp_32h & 0xffff) << 16)));
							SET_CF(tmp_32h);
							if (DF) {
								r_di -= 4;
								r_si -= 4;
							}
							else {
								r_di += 4;
								r_si += 4;
							}
						}
						break;

		case REPZ:		rep_flag = REPZ;
						goto top_loop;
						break;

		case REPNZ:		rep_flag = REPNZ;
						goto top_loop;
						break;

		case 0x86:
		case 0x87: {
			short word_op,source_reg,dest_reg,mod;
			void *dst_mp,*src_mp;

			mod_byte = *imp++;
			word_op =(opcode & 0x01);
			source_reg = mod_byte & 0x07;		/* Get source register or R/M field */
			dest_reg = (mod_byte >> 3) & 0x07;		/* Get Destination register */
			mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
			switch(mod) {		/* Handle type of operation */
				case 03:		/* This is a REGISTER to REGISTER Transfer */
					if (word_op) {
						SET_ALL_EREGS();
						switch(dest_reg) {
							case REG_AX:	src_mp = &r_eax;
											break;
							case REG_BX:	src_mp = &r_ebx;
											break;
							case REG_CX:	src_mp = &r_ecx;
											break;
							case REG_DX:	src_mp = &r_edx;
											break;
							case REG_SP:	src_mp = &r_esp;
											break;
							case REG_BP:	src_mp = &r_ebp;
											break;
							case REG_SI:	src_mp = &r_esi;
											break;
							case REG_DI:	src_mp = &r_edi;
											break;
						}

						switch(source_reg) {
							case REG_AX:	dst_mp = &r_eax;
											break;
							case REG_BX:	dst_mp = &r_ebx;
											break;
							case REG_CX:	dst_mp = &r_ecx;
											break;
							case REG_DX:	dst_mp = &r_edx;
											break;
							case REG_SP:	dst_mp = &r_esp;
											break;
							case REG_BP:	dst_mp = &r_ebp;
											break;
							case REG_SI:	dst_mp = &r_esi;
											break;
							case REG_DI:	dst_mp = &r_edi;
											break;
						}
					}
					else {
						switch(dest_reg) {
							case REG_AL:	src_mp = &r_al;
											break;
							case REG_BL:	src_mp = &r_bl;
											break;
							case REG_CL:	src_mp = &r_cl;
											break;
							case REG_DL:	src_mp = &r_dl;
											break;
							case REG_AH:	src_mp = &r_ah;
											break;
							case REG_BH:	src_mp = &r_bh;
											break;
							case REG_CH:	src_mp = &r_ch;
											break;
							case REG_DH:	src_mp = &r_dh;
											break;
						}

						switch(source_reg) {
							case REG_AL:	dst_mp = &r_al;
											break;
							case REG_BL:	dst_mp = &r_bl;
											break;
							case REG_CL:	dst_mp = &r_cl;
											break;
							case REG_DL:	dst_mp = &r_dl;
											break;
							case REG_AH:	dst_mp = &r_ah;
											break;
							case REG_BH:	dst_mp = &r_bh;
											break;
							case REG_CH:	dst_mp = &r_ch;
											break;
							case REG_DH:	dst_mp = &r_dh;
											break;
						}
					}
					break;


				case 00:
					switch(source_reg) {			/* r/m */
						case 000:	if (new_seg) 
										src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
									else
										src_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
									break;
						case 001:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
									else
										src_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
									break;
						case 002:	if (new_seg) 
										src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
									else
										src_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
									break;
						case 003:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
									else
										src_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
									break;
						case 004:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
									else
										src_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
									break;
						case 005:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
									else
										src_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
									break;
						case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
									imp+=2;
									if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
									else
										src_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
									break;
						case 007:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
									else
										src_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
									break;
					}
					if (word_op) {
						SET_ALL_EREGS();
						switch(dest_reg) {
							case REG_AX:	dst_mp = &r_eax;
											break;
							case REG_BX:	dst_mp = &r_ebx;
											break;
							case REG_CX:	dst_mp = &r_ecx;
											break;
							case REG_DX:	dst_mp = &r_edx;
											break;
							case REG_SP:	dst_mp = &r_esp;
											break;
							case REG_BP:	dst_mp = &r_ebp;
											break;
							case REG_SI:	dst_mp = &r_esi;
											break;
							case REG_DI:	dst_mp = &r_edi;
											break;
						}
					}
					else {
						switch(dest_reg) {
							case REG_AL:	src_mp = &r_al;
											break;
							case REG_BL:	src_mp = &r_bl;
											break;
							case REG_CL:	src_mp = &r_cl;
											break;
							case REG_DL:	src_mp = &r_dl;
											break;
							case REG_AH:	src_mp = &r_ah;
											break;
							case REG_BH:	src_mp = &r_bh;
											break;
							case REG_CH:	src_mp = &r_ch;
											break;
							case REG_DH:	src_mp = &r_dh;
											break;
						}
					}
					break;

				case 01:
					disp_8 = *imp++;

					switch(source_reg) {			/* r/m */
						case 000:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
									else
										src_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
									break;
						case 001:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
									else
										src_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
									break;
						case 002:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
									else
										src_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
									break;
						case 003:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
									else
										src_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
									break;
						case 004:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
									else
										src_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
									break;
						case 005:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
									else
										src_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
									break;
						case 006:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
									else
										src_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
									break;
						case 007:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
									else
										src_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
									break;
					}
					if (word_op) {
						SET_ALL_EREGS();
						switch(dest_reg) {
							case REG_AX:	dst_mp = &r_eax;
											break;
							case REG_BX:	dst_mp = &r_ebx;
											break;
							case REG_CX:	dst_mp = &r_ecx;
											break;
							case REG_DX:	dst_mp = &r_edx;
											break;
							case REG_SP:	dst_mp = &r_esp;
											break;
							case REG_BP:	dst_mp = &r_ebp;
											break;
							case REG_SI:	dst_mp = &r_esi;
											break;
							case REG_DI:	dst_mp = &r_edi;
											break;
						}
					}
					else {
						switch(dest_reg) {
							case REG_AL:	src_mp = &r_al;
											break;
							case REG_BL:	src_mp = &r_bl;
											break;
							case REG_CL:	src_mp = &r_cl;
											break;
							case REG_DL:	src_mp = &r_dl;
											break;
							case REG_AH:	src_mp = &r_ah;
											break;
							case REG_BH:	src_mp = &r_bh;
											break;
							case REG_CH:	src_mp = &r_ch;
											break;
							case REG_DH:	src_mp = &r_dh;
											break;
						}
					}
					break;

				case 02:
					disp_u16 = *(unsigned short *)imp;
					imp+=2;

					switch(source_reg) {			/* r/m */
						case 000:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
									else
										src_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
									break;
						case 001:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
									else
										src_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
									break;
						case 002:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
									else
										src_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
									break;
						case 003:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
									else
										src_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
									break;
						case 004:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
									else
										src_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
									break;
						case 005:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
									else
										src_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
									break;
						case 006:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
									else
										src_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
									break;
						case 007:	if (new_seg)
										src_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
									else
										src_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
									break;
					}
					if (word_op) {
						SET_ALL_EREGS();
						switch(dest_reg) {
							case REG_AX:	dst_mp = &r_eax;
											break;
							case REG_BX:	dst_mp = &r_ebx;
											break;
							case REG_CX:	dst_mp = &r_ecx;
											break;
							case REG_DX:	dst_mp = &r_edx;
											break;
							case REG_SP:	dst_mp = &r_esp;
											break;
							case REG_BP:	dst_mp = &r_ebp;
											break;
							case REG_SI:	dst_mp = &r_esi;
											break;
							case REG_DI:	dst_mp = &r_edi;
											break;
						}
					}
					else {
						switch(dest_reg) {
							case REG_AL:	src_mp = &r_al;
											break;
							case REG_BL:	src_mp = &r_bl;
											break;
							case REG_CL:	src_mp = &r_cl;
											break;
							case REG_DL:	src_mp = &r_dl;
											break;
							case REG_AH:	src_mp = &r_ah;
											break;
							case REG_BH:	src_mp = &r_bh;
											break;
							case REG_CH:	src_mp = &r_ch;
											break;
							case REG_DH:	src_mp = &r_dh;
											break;
						}
					}
				break;
			}
			if (word_op) {
				tmp_u32 = *(unsigned long *)dst_mp;
				*(unsigned long *)dst_mp = *(unsigned long *)src_mp;			/* XCHG 32 bit */
				*(unsigned long *)src_mp = tmp_u32;
			}
			else {
				tmp_u8 = *(unsigned char *)dst_mp;
				*(unsigned char *)dst_mp = *(unsigned char *)src_mp;			/* XCHG 8 bit */
				*(unsigned char *)src_mp = tmp_u8;
			}
			if (word_op) {
				smp = SET_NEW_SMP();
				SET_ALL_REGS();
				SET_ALL_BREGS();
			}
			else {
				SET_ALL_WREGS();
			}
		}
		break;

	case 0x84:
	case 0x85: {
		short word_op,source_reg,dest_reg,mod;
		void *dst_mp,*src_mp;

		mod_byte = *imp++;
		word_op = (opcode & 0x01);
		source_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		dest_reg = (mod_byte >> 3) & 0x07;		/* Get Destination register */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is a REGISTER to REGISTER Transfer */
				if (word_op) {
					SET_ALL_EREGS();
					switch(dest_reg) {
						case REG_AX:	src_mp = &r_eax;
										break;
						case REG_BX:	src_mp = &r_ebx;
										break;
						case REG_CX:	src_mp = &r_ecx;
										break;
						case REG_DX:	src_mp = &r_edx;
										break;
						case REG_SP:	src_mp = &r_esp;
										break;
						case REG_BP:	src_mp = &r_ebp;
										break;
						case REG_SI:	src_mp = &r_esi;
										break;
						case REG_DI:	src_mp = &r_edi;
										break;
					}

					switch(source_reg) {
						case REG_AX:	dst_mp = &r_eax;
										break;
						case REG_BX:	dst_mp = &r_ebx;
										break;
						case REG_CX:	dst_mp = &r_ecx;
										break;
						case REG_DX:	dst_mp = &r_edx;
										break;
						case REG_SP:	dst_mp = &r_esp;
										break;
						case REG_BP:	dst_mp = &r_ebp;
										break;
						case REG_SI:	dst_mp = &r_esi;
										break;
						case REG_DI:	dst_mp = &r_edi;
										break;
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	src_mp = &r_al;
										break;
						case REG_BL:	src_mp = &r_bl;
										break;
						case REG_CL:	src_mp = &r_cl;
										break;
						case REG_DL:	src_mp = &r_dl;
										break;
						case REG_AH:	src_mp = &r_ah;
										break;
						case REG_BH:	src_mp = &r_bh;
										break;
						case REG_CH:	src_mp = &r_ch;
										break;
						case REG_DH:	src_mp = &r_dh;
										break;
					}

					switch(source_reg) {
						case REG_AL:	dst_mp = &r_al;
										break;
						case REG_BL:	dst_mp = &r_bl;
										break;
						case REG_CL:	dst_mp = &r_cl;
										break;
						case REG_DL:	dst_mp = &r_dl;
										break;
						case REG_AH:	dst_mp = &r_ah;
										break;
						case REG_BH:	dst_mp = &r_bh;
										break;
						case REG_CH:	dst_mp = &r_ch;
										break;
						case REG_DH:	dst_mp = &r_dh;
										break;
					}
				}
				break;


			case 00:
				switch(source_reg) {			/* r/m */
					case 000:	if (new_seg) 
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									src_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									src_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									src_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									src_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									src_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				if (word_op) {
					SET_ALL_EREGS();
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_eax;
										break;
						case REG_BX:	dst_mp = &r_ebx;
										break;
						case REG_CX:	dst_mp = &r_ecx;
										break;
						case REG_DX:	dst_mp = &r_edx;
										break;
						case REG_SP:	dst_mp = &r_esp;
										break;
						case REG_BP:	dst_mp = &r_ebp;
										break;
						case REG_SI:	dst_mp = &r_esi;
										break;
						case REG_DI:	dst_mp = &r_edi;
										break;
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	src_mp = &r_al;
										break;
						case REG_BL:	src_mp = &r_bl;
										break;
						case REG_CL:	src_mp = &r_cl;
										break;
						case REG_DL:	src_mp = &r_dl;
										break;
						case REG_AH:	src_mp = &r_ah;
										break;
						case REG_BH:	src_mp = &r_bh;
										break;
						case REG_CH:	src_mp = &r_ch;
										break;
						case REG_DH:	src_mp = &r_dh;
										break;
					}
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(source_reg) {			/* r/m */
					case 000:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				if (word_op) {
					SET_ALL_EREGS();
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_eax;
										break;
						case REG_BX:	dst_mp = &r_ebx;
										break;
						case REG_CX:	dst_mp = &r_ecx;
										break;
						case REG_DX:	dst_mp = &r_edx;
										break;
						case REG_SP:	dst_mp = &r_esp;
										break;
						case REG_BP:	dst_mp = &r_ebp;
										break;
						case REG_SI:	dst_mp = &r_esi;
										break;
						case REG_DI:	dst_mp = &r_edi;
										break;
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	src_mp = &r_al;
										break;
						case REG_BL:	src_mp = &r_bl;
										break;
						case REG_CL:	src_mp = &r_cl;
										break;
						case REG_DL:	src_mp = &r_dl;
										break;
						case REG_AH:	src_mp = &r_ah;
										break;
						case REG_BH:	src_mp = &r_bh;
										break;
						case REG_CH:	src_mp = &r_ch;
										break;
						case REG_DH:	src_mp = &r_dh;
										break;
					}
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(source_reg) {			/* r/m */
					case 000:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									src_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									src_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
				if (word_op) {
					SET_ALL_EREGS();
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_eax;
										break;
						case REG_BX:	dst_mp = &r_ebx;
										break;
						case REG_CX:	dst_mp = &r_ecx;
										break;
						case REG_DX:	dst_mp = &r_edx;
										break;
						case REG_SP:	dst_mp = &r_esp;
										break;
						case REG_BP:	dst_mp = &r_ebp;
										break;
						case REG_SI:	dst_mp = &r_esi;
										break;
						case REG_DI:	dst_mp = &r_edi;
										break;
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	src_mp = &r_al;
										break;
						case REG_BL:	src_mp = &r_bl;
										break;
						case REG_CL:	src_mp = &r_cl;
										break;
						case REG_DL:	src_mp = &r_dl;
										break;
						case REG_AH:	src_mp = &r_ah;
										break;
						case REG_BH:	src_mp = &r_bh;
										break;
						case REG_CH:	src_mp = &r_ch;
										break;
						case REG_DH:	src_mp = &r_dh;
										break;
					}
				}
			break;
		}
		if (word_op) {
			tmp_u32 = *(unsigned long *)dst_mp & *(unsigned long *)src_mp;		/* TEST 16 bit */
			SET_ZF_PF(tmp_u32);
			SET_SF_32(tmp_u32);
			CF = OF = 0;
			SET_ALL_REGS();
			SET_ALL_BREGS();
			smp = SET_NEW_SMP();
		}
		else {
			tmp_u8 = *(unsigned char *)dst_mp & *(unsigned char *)src_mp;		/* TEST 8 bit */
			SET_ZF_PF_8(tmp_u8);
			SET_SF_8(tmp_u8);
			CF = OF = 0;
			SET_ALL_WREGS();
		}
	}
	break;

	case 0xD0:
	case 0xD1:
	case 0xD2:
	case 0xD3:
	case 0xC0:
	case 0xC1: {
		short dest_is_reg,dest_reg,subcode,mod,tmp_cf,word_op;
		int count;
		void *dst_mp;

		mod_byte = *imp++;
		dest_is_reg = 0;
		dest_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		subcode = (mod_byte >> 3) & 0x07;
		word_op = (opcode & 0x01);

		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is a REGISTER to REGISTER Transfer */
				dest_is_reg = 1;
				if (opcode & 0x01) {
					SET_ALL_EREGS();
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_eax;
										break;
						case REG_BX:	dst_mp = &r_ebx;
										break;
						case REG_CX:	dst_mp = &r_ecx;
										break;
						case REG_DX:	dst_mp = &r_edx;
										break;
						case REG_SP:	dst_mp = &r_esp;
										break;
						case REG_BP:	dst_mp = &r_ebp;
										break;
						case REG_SI:	dst_mp = &r_esi;
										break;
						case REG_DI:	dst_mp = &r_edi;
										break;
						default:		break;			/* can't happen */
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	dst_mp = &r_al;
										break;
						case REG_BL:	dst_mp = &r_bl;
										break;
						case REG_CL:	dst_mp = &r_cl;
										break;
						case REG_DL:	dst_mp = &r_dl;
										break;
						case REG_AH:	dst_mp = &r_ah;
										break;
						case REG_BH:	dst_mp = &r_bh;
										break;
						case REG_CH:	dst_mp = &r_ch;
										break;
						case REG_DH:	dst_mp = &r_dh;
										break;
						default:		break;			/* can't happen */
					}
				}
			break;


			case 00:
				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
		}

			/* Determine the number of bits to shift */
		if (opcode & 0x02) {
			count = r_cl;
		}
		else {
			if (opcode < 0xD0) {
				count = *(unsigned char *)imp++;
			}
			else
				count = 1;
		}
		count &= 31;

		switch(subcode) {
			case 000:	/* ROL */
						tmp_16 = count;
						if (word_op) {
							SET_ALL_EREGS();
							while(tmp_16 != 0) {
								tmp_cf = *(unsigned short *)dst_mp & 0x80000000 ? 1 : 0;
								*(unsigned long *)dst_mp <<= 1;
								if (tmp_cf)
									*(unsigned long *)dst_mp |= 1;
								tmp_16--;
							}
							CF = tmp_cf;
							if (count == 1) {
								tmp_cf = *(unsigned long *)dst_mp & 0x80000000 ? 1: 0;
								if (CF != tmp_cf) 
									OF = 1;
								else
									OF = 0;
							}
						}
						else {
							while(tmp_16 != 0) {
								tmp_cf = *(unsigned char *)dst_mp & 0x80 ? 1 : 0;
								*(unsigned char *)dst_mp <<= 1;
								if (tmp_cf)
									*(unsigned char *)dst_mp |= 1;
								tmp_16--;
							}
							CF = tmp_cf;
							if (count == 1) {
								tmp_cf = *(unsigned char *)dst_mp & 0x80 ? 1: 0;
								if (CF != tmp_cf) 
									OF = 1;
								else
									OF = 0;
							}
						}
						break;

			case 001:	/* ROR */
						tmp_16 = count;
						if (word_op) {
							SET_ALL_EREGS();
							while(tmp_16 != 0) {
								tmp_cf = *(unsigned long *)dst_mp & 0x1;
								*(unsigned long *)dst_mp >>= 1;
								if (tmp_cf)
									*(unsigned long *)dst_mp |= 0x80000000;
								tmp_16--;
							}
							CF = tmp_cf;
							if (count == 1) {
								tmp_cf = *(unsigned long *)dst_mp & 0x80000000 ? 1: 0;
								tmp_32    = *(unsigned long *)dst_mp & 0x40000000 ? 1: 0;
								if (tmp_32 != tmp_cf) 
									OF = 1;
								else
									OF = 0;
							}
						}
						else {
							while(tmp_16 != 0) {
								tmp_cf = *(unsigned char *)dst_mp & 1;
								*(unsigned char *)dst_mp >>= 1;
								if (tmp_cf)
									*(unsigned char *)dst_mp |= 0x80;
								tmp_16--;
							}
							CF = tmp_cf;
							if (count == 1) {
								tmp_cf = *(unsigned char *)dst_mp & 0x80 ? 1: 0;
								tmp_32    = *(unsigned char *)dst_mp & 0x40 ? 1: 0;
								if (tmp_32 != tmp_cf) 
									OF = 1;
								else
									OF = 0;
							}
						}
						break;
			case 002:	/* RCL */
						tmp_16 = count;
						if (word_op) {
							SET_ALL_EREGS();
							while(tmp_16 != 0) {
								tmp_cf = CF;
								CF = *(unsigned long *)dst_mp & 0x80000000 ? 1 : 0;
								*(unsigned long *)dst_mp <<= 1;
								if (tmp_cf)
									*(unsigned long *)dst_mp |= 1;
								tmp_16--;
							}
							if (count == 1) {
								tmp_cf = *(unsigned long *)dst_mp & 0x80000000 ? 1: 0;
								if (CF != tmp_cf) 
									OF = 1;
								else
									OF = 0;
							}
						}
						else {
							while(tmp_16 != 0) {
								tmp_cf = CF;
								CF = *(unsigned char *)dst_mp & 0x80 ? 1 : 0;
								*(unsigned char *)dst_mp <<= 1;
								if (tmp_cf)
									*(unsigned char *)dst_mp |= 1;
								tmp_16--;
							}
							if (count == 1) {
								tmp_cf = *(unsigned char *)dst_mp & 0x80 ? 1: 0;
								if (CF != tmp_cf) 
									OF = 1;
								else
									OF = 0;
							}
						}
						break;
			case 003:	/* RCR */
						tmp_16 = count;
						if (word_op) {
							SET_ALL_EREGS();
							while(tmp_16 != 0) {
								tmp_cf = CF;
								CF = *(unsigned long *)dst_mp & 0x1;
								*(unsigned long *)dst_mp >>= 1;
								if (tmp_cf)
									*(unsigned long *)dst_mp |= 0x80000000;
								tmp_16--;
							}
							if (count == 1) {
								tmp_cf = *(unsigned long *)dst_mp & 0x80000000 ? 1: 0;
								tmp_32    = *(unsigned long *)dst_mp & 0x40000000 ? 1: 0;
								if (tmp_32 != tmp_cf) 
									OF = 1;
								else
									OF = 0;
							}
						}
						else {
							while(tmp_16 != 0) {
								tmp_cf = CF;
								CF = *(unsigned char *)dst_mp & 1;
								*(unsigned char *)dst_mp >>= 1;
								if (tmp_cf)
									*(unsigned char *)dst_mp |= 0x80;
								tmp_16--;
							}
							if (count == 1) {
								tmp_cf = *(unsigned char *)dst_mp & 0x80 ? 1: 0;
								tmp_32    = *(unsigned char *)dst_mp & 0x40 ? 1: 0;
								if (tmp_32 != tmp_cf) 
									OF = 1;
								else
									OF = 0;
							}
						}
						break;
			case 004:	/* SHL */
						tmp_16 = count;
						if (word_op) {
							while(tmp_16 != 0) {
								CF = *(unsigned long *)dst_mp & 0x80000000 ? 1: 0;
								*(unsigned long *)dst_mp <<= 1;
								tmp_16--;
							}
							if (count == 1) {
								if (CF == (*(unsigned short *)dst_mp & 0x80000000 ? 1: 0))
									OF = 0;
								else
									OF = 1;
							}
							SET_ZF_PF((*(unsigned long *)dst_mp));
							SET_SF_32((*(unsigned long *)dst_mp));
						}
						else {
							while(tmp_16 != 0) {
								CF = *(unsigned char *)dst_mp & 0x80 ? 1: 0;
								*(unsigned char *)dst_mp <<= 1;
								tmp_16--;
							}
							if (count == 1) {
								if (CF == (*(unsigned char *)dst_mp & 0x80 ? 1: 0))
									OF = 0;
								else
									OF = 1;
							}
							SET_ZF_PF_8((*(unsigned char *)dst_mp));
							SET_SF_8((*(unsigned char *)dst_mp));
						}
						break;
			case 005:	/* SHR */
						tmp_16 = count;
						if (word_op) {
							if (count == 1)
								OF = *(unsigned long *)dst_mp & 0x80000000 ? 1: 0;
							while(tmp_16 != 0) {
								CF = *(unsigned long *)dst_mp & 1;
								*(unsigned long *)dst_mp >>= 1;
								tmp_16--;
							}
							SET_ZF_PF((*(unsigned long *)dst_mp));
							SET_SF_32((*(unsigned long *)dst_mp));
						}
						else {
							if (count == 1)
								OF = *(unsigned char *)dst_mp & 0x80 ? 1: 0;
							while(tmp_16 != 0) {
								CF = *(unsigned char *)dst_mp & 1;
								*(unsigned char *)dst_mp >>= 1;
								tmp_16--;
							}
							SET_ZF_PF_8((*(unsigned char *)dst_mp));
							SET_SF_8((*(unsigned char *)dst_mp));
						}
						break;
			case 006:	/* NOT USED */
						break;
			case 007:	/* SAR */
						tmp_16 = count;
						if (count == 1)
							OF = 0;
						if (word_op) {
							while(tmp_16 != 0) {
								CF = *(unsigned long *)dst_mp & 1;
								*(long *)dst_mp /= 2;
								tmp_16--;
							}
							SET_ZF_PF((*(unsigned long *)dst_mp));
							SET_SF_32((*(unsigned long *)dst_mp));
						}
						else {
							while(tmp_16 != 0) {
								CF = *(unsigned char *)dst_mp & 1;
								*(char *)dst_mp /= 2;
								tmp_16--;
							}
							SET_ZF_PF_8((*(unsigned char *)dst_mp));
							SET_SF_8((*(unsigned char *)dst_mp));
						}
						break;
			default:	break;
		}

		if (dest_is_reg) {
			if (opcode & 0x01) {
				SET_ALL_REGS();
				SET_ALL_BREGS();
				smp = SET_NEW_SMP();
			}
			else {
				SET_ALL_WREGS();
			}
		}

	}
	break;

	case 0x8F: {
		short dest_is_reg, mod, dest_reg;
		void *dst_mp;

		mod_byte = *imp++;
		dest_is_reg = 0;
		dest_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is a REGISTER to REGISTER Transfer */
				dest_is_reg = 1;
				SET_ALL_EREGS();
				switch(dest_reg) {
					case REG_AX:	dst_mp = &r_eax;
									break;
					case REG_BX:	dst_mp = &r_ebx;
									break;
					case REG_CX:	dst_mp = &r_ecx;
									break;
					case REG_DX:	dst_mp = &r_edx;
									break;
					case REG_SP:	dst_mp = &r_esp;
									break;
					case REG_BP:	dst_mp = &r_ebp;
									break;
					case REG_SI:	dst_mp = &r_esi;
									break;
					case REG_DI:	dst_mp = &r_edi;
									break;
					default:		break;			/* can't happen */
				}
			break;


			case 00:
				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
		}
		smp = SET_NEW_SMP();
		*(unsigned long *)dst_mp = *(unsigned long *)smp;
		r_sp += 4;

		if (dest_is_reg) {
			SET_ALL_REGS();
			SET_ALL_BREGS();
		}
		smp = SET_NEW_SMP();
	}
	break;

	case 0xF6:
	case 0xF7: {
		short dest_is_reg, dest_reg, mod, subcode;
		void *dst_mp;

		mod_byte = *imp++;
		dest_is_reg = 0;
		dest_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		subcode = (mod_byte >> 3) & 0x07;
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is a REGISTER to REGISTER Transfer */
				dest_is_reg = 1;
				SET_ALL_EREGS();
				if (opcode & 0x01) {
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_eax;
										break;
						case REG_BX:	dst_mp = &r_ebx;
										break;
						case REG_CX:	dst_mp = &r_ecx;
										break;
						case REG_DX:	dst_mp = &r_edx;
										break;
						case REG_SP:	dst_mp = &r_esp;
										break;
						case REG_BP:	dst_mp = &r_ebp;
										break;
						case REG_SI:	dst_mp = &r_esi;
										break;
						case REG_DI:	dst_mp = &r_edi;
										break;
						default:		break;			/* can't happen */
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	dst_mp = &r_al;
										break;
						case REG_BL:	dst_mp = &r_bl;
										break;
						case REG_CL:	dst_mp = &r_cl;
										break;
						case REG_DL:	dst_mp = &r_dl;
										break;
						case REG_AH:	dst_mp = &r_ah;
										break;
						case REG_BH:	dst_mp = &r_bh;
										break;
						case REG_CH:	dst_mp = &r_ch;
										break;
						case REG_DH:	dst_mp = &r_dh;
										break;
						default:		break;			/* can't happen */
					}
				}
			break;


			case 00:
				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
		}
		switch(subcode) {
			case 000:	if (opcode & 0x01) {
							tmp_u32 = *(unsigned long *)imp;
							imp+=4;
							tmp_u32 = *(unsigned long *)dst_mp & tmp_u32;		/* TEST Immediate 32 bit */
							SET_ZF_PF(tmp_u32);
							SET_SF_32(tmp_u32);
							CF = OF = 0;
						}
						else {
							tmp_u8 = *(unsigned char *)imp++;
							tmp_u8 = *(unsigned char *)dst_mp & tmp_u8;		/* TEST Immediate 8 bit */
							SET_ZF_PF_8(tmp_u8);
							SET_SF_8(tmp_u8);
							CF = OF = 0;
						}
						break;

			case 001:	break;					/* NOT USED */



			case 002:	if (opcode & 0x01) 			/* Word op */			/* NOT */
							*(unsigned long *)dst_mp ^= 0xffffffff;
						else
							*(unsigned char *)dst_mp ^= 0xff;
						break;

			case 003:	if (opcode & 0x01) {			/* Word op */			/* NEG */
							if (*(unsigned long *)dst_mp == 0x80000000) {
								CF = OF = SF = 1; ZF = 0;
								break;
							}
							if (*(long *)dst_mp == 0) {
								CF = OF = SF = 0; ZF = 1;
								break;
							}
							*(long *)dst_mp = -*(long *)dst_mp;
							CF = 1; OF = ZF = 0;
							if (*(unsigned long *)dst_mp & 0x80000000)
								SF = 1;
							else
								SF = 0;
						}
						else {						/* Byte op */
							if (*(unsigned char *)dst_mp == 0x80) {
									CF = OF = SF = 1; ZF = 0;
									break;
							}
							if (*(char *)dst_mp == 0) {
								CF = OF = SF = 0; ZF = 1;
								break;
							}
							*(char *)dst_mp = -*(char *)dst_mp;
							CF = 1; OF = ZF = 0;
							if (*(unsigned char *)dst_mp & 0x80)
								SF = 1;
							else
								SF = 0;
						}
						break;

			case 004:
    ErrorF("CURRENTLY THE SOFTWARE DOES NOT SUPPORT 32/64 BIT MULTIPLIES!\n");
						if (opcode & 0x01) {			/* Word Op */					/* MUL */		/* ADD opsize override here */
							tmp_32 = (unsigned long)r_ax * (unsigned long)*(unsigned short *)dst_mp;
							r_ax = (tmp_32 & 0xffff);
							r_dx = (unsigned long)(tmp_32 & 0xffff0000) >> 16;
							SET_AL_AH();
							SET_DL_DH();
							if (r_dx == 0)
								CF = OF = 0;
							else
								CF = OF = 1;
							TRUNC_16(r_dx);
							TRUNC_16(r_ax);
						}
						else {
							r_ax = (unsigned short)r_al * (unsigned short)*(unsigned char *)dst_mp;
							SET_AL_AH();
							TRUNC_16(r_ax);
							if (r_ah == 0)
								CF = OF = 0;
							else
								CF = OF = 1;
						}
						break;

			case 005:
    ErrorF("CURRENTLY THE SOFTWARE DOES NOT SUPPORT 32/64 BIT MULTIPLIES!\n");
						if (opcode & 0x01) {			/* Word Op */					/* IMUL */
							tmp_32 = r_ax * (long)*(short *)dst_mp;
							if (r_ax & 0x8000) {
								if (tmp_32 < -32768)
									CF = OF = 1;
								else
									CF = OF = 0;
							}
							else {
								if (tmp_32 >= 32768)
									CF = OF = 1;
								else
									CF = OF = 0;
							}
							r_ax = (tmp_32 & 0xffff);
							r_dx = (unsigned long)(tmp_32 & 0xffff0000) >> 16;
							SET_AL_AH();
							SET_DL_DH();
							TRUNC_16(r_dx);
							TRUNC_16(r_ax);
						}
						else {
							tmp_16 = r_al * (long)*(char *)dst_mp;
							if (r_al & 0x80) {
								if (tmp_16 < -128)
									CF = OF = 1;
								else
									CF = OF = 0;
							}
							else {
								if (tmp_16 >= 128)
									CF = OF = 1;
								else
									CF = OF = 0;
							}
							r_ax = (tmp_16 & 0xffff);
							SET_AL_AH();
							TRUNC_16(r_ax);
						}
						break;

			case 006:
    ErrorF("CURRENTLY THE SOFTWARE DOES NOT SUPPORT 32/64 BIT DIVIDES!\n");
						if (opcode & 0x01) {				/* WORD op */		/* DIV */
							if (*(unsigned short *)dst_mp == 0) {		/* Handle divide by zero */
								PUSH_ARG(r_fl);
								IF = TF = 0;
								r_ip = GET_IP();
								PUSH_ARG(r_cs);
								PUSH_ARG(r_ip);
								dmp_16 = (unsigned short *)SET_NEW_DMP(0,0,0,0);
								r_ip = *dmp_16;
								r_cs = *(dmp_16+1);
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							else {
								unsigned long tmp1_u32;

								tmp_u32 = (unsigned long)r_dx << 16 | r_ax;
								tmp1_u32 = tmp_u32 / *(unsigned short *)dst_mp;
								if (tmp1_u32 > 0xffff) {
									PUSH_ARG(r_fl);
									IF = TF = 0;
									r_ip = GET_IP();
									PUSH_ARG(r_cs);
									PUSH_ARG(r_ip);
									dmp_16 = SET_NEW_DMP(0,0,0,0);
									r_ip = *dmp_16;
									r_cs = *(dmp_16+1);
									imp = SET_NEW_IMP(r_cs,r_ip);
								}
								else {
									r_ax = (tmp1_u32 & 0xffff);
									r_dx = tmp_u32 % *(unsigned short *)dst_mp;
									SET_AL_AH();
									SET_DL_DH();
								}
							}
						}
						else {
							if (*(unsigned char *)dst_mp == 0) {		/* Handle divide by zero */
								PUSH_ARG(r_fl);
								IF = TF = 0;
								r_ip = GET_IP();
								PUSH_ARG(r_cs);
								PUSH_ARG(r_ip);
								dmp_16 = SET_NEW_DMP(0,0,0,0);
								r_ip = *dmp_16;
								r_cs = *(dmp_16+1);
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							else {
								tmp_16 = r_ax / *(unsigned char *)dst_mp;
								if (tmp_16 > 0xff) {
									PUSH_ARG(r_fl);
									IF = TF = 0;
									r_ip = GET_IP();
									PUSH_ARG(r_cs);
									PUSH_ARG(r_ip);
									dmp_16 = SET_NEW_DMP(0,0,0,0);
									r_ip = *dmp_16;
									r_cs = *(dmp_16+1);
									imp = SET_NEW_IMP(r_cs,r_ip);
									break;
								}
								else {
									r_al = tmp_16;
									r_ah = r_ax % *(unsigned char *)dst_mp;
									SET_AX();
								}
							}
						}
						break;

			case 007:
    ErrorF("CURRENTLY THE SOFTWARE DOES NOT SUPPORT 32/64 BIT DIVIDES!\n");
						if (opcode & 0x01) {				/* WORD op */		/* IDIV */
							if (*(short *)dst_mp == 0) {		/* Handle divide by zero */
								PUSH_ARG(r_fl);
								IF = TF = 0;
								r_ip = GET_IP();
								PUSH_ARG(r_cs);
								PUSH_ARG(r_ip);
								dmp_16 = SET_NEW_DMP(0,0,0,0);
								r_ip = *dmp_16;
								r_cs = *(dmp_16+1);
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							else {
								long tmp1_32;

								tmp_32 = (unsigned long)r_dx << 16 | r_ax;
								tmp1_32 = tmp_32 / *(short *)dst_mp;
								if (r_dx & 0x8000) {
									if (tmp1_32 < -32768 ) {
										PUSH_ARG(r_fl);
										IF = TF = 0;
										r_ip = GET_IP();
										PUSH_ARG(r_cs);
										PUSH_ARG(r_ip);
										dmp_16 = SET_NEW_DMP(0,0,0,0);
										r_ip = *dmp_16;
										r_cs = *(dmp_16+1);
										imp = SET_NEW_IMP(r_cs,r_ip);
									}
									else {
										r_ax = (tmp1_32 & 0xffff);
										r_dx = (tmp_32 % *(short *)dst_mp) & 0xffff;
										SET_AL_AH();
										SET_DL_DH();
									}
								}
								else {
									if (tmp1_32 > 32767) {
										PUSH_ARG(r_fl);
										IF = TF = 0;
										r_ip = GET_IP();
										PUSH_ARG(r_cs);
										PUSH_ARG(r_ip);
										dmp_16 = SET_NEW_DMP(0,0,0,0);
										r_ip = *dmp_16;
										r_cs = *(dmp_16+1);
										imp = SET_NEW_IMP(r_cs,r_ip);
									}
									else {
										r_ax = (tmp1_32 & 0xffff);
										r_dx = (tmp_32 % *(short *)dst_mp) & 0xffff;
										SET_AL_AH();
										SET_DL_DH();
									}
								}
							}
						}
						else {
							if (*(char *)dst_mp == 0) {		/* Handle divide by zero */
								PUSH_ARG(r_fl);
								IF = TF = 0;
								r_ip = GET_IP();
								PUSH_ARG(r_cs);
								PUSH_ARG(r_ip);
								dmp_16 = SET_NEW_DMP(0,0,0,0);
								r_ip = *dmp_16;
								r_cs = *(dmp_16+1);
								imp = SET_NEW_IMP(r_cs,r_ip);
							}
							else {
								tmp_16 = r_ax / *(char *)dst_mp;
								if (r_ax & 0x8000) {
									if (tmp_16 < -128 ) {
										PUSH_ARG(r_fl);
										IF = TF = 0;
										r_ip = GET_IP();
										PUSH_ARG(r_cs);
										PUSH_ARG(r_ip);
										dmp_16 = SET_NEW_DMP(0,0,0,0);
										r_ip = *dmp_16;
										r_cs = *(dmp_16+1);
										imp = SET_NEW_IMP(r_cs,r_ip);
									}
									else {
										r_al = tmp_16 & 0xff;
										r_ah = (r_ax % *(char *)dst_mp) & 0xff;
										SET_AX();
									}
								}
								else {
									if (tmp_16 > 127) {
										PUSH_ARG(r_fl);
										IF = TF = 0;
										r_ip = GET_IP();
										PUSH_ARG(r_cs);
										PUSH_ARG(r_ip);
										dmp_16 = SET_NEW_DMP(0,0,0,0);
										r_ip = *dmp_16;
										r_cs = *(dmp_16+1);
										imp = SET_NEW_IMP(r_cs,r_ip);
									}
									else {
										r_al = (tmp_16 & 0xff);
										r_ah = (r_ax % *(char *)dst_mp) & 0xff;
										SET_AX();
									}
								}
							}
						}
						break;

			default:	break;				/* ERROR CONDITION, Cannot happen */
		}

		if (dest_is_reg) {
			if (opcode & 0x01) {
				SET_ALL_REGS();
				SET_ALL_BREGS();
				smp = SET_NEW_SMP();
			}
			else {
				SET_ALL_WREGS();
			}
		}

	}
	break;

	case 0x8C:
	case 0x8E: {
		short dest_is_reg, mod, subcode,dest_reg;
		void *dst_mp;

		dest_is_reg = 0;
		mod_byte = *imp++;
		dest_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		subcode = (mod_byte >> 3) & 0x03;
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is a REGISTER to REGISTER Transfer */
				dest_is_reg = 1;
				SET_ALL_EREGS();
				switch(dest_reg) {
					case REG_AX:	dst_mp = &r_eax;
									break;
					case REG_BX:	dst_mp = &r_ebx;
									break;
					case REG_CX:	dst_mp = &r_ecx;
									break;
					case REG_DX:	dst_mp = &r_edx;
									break;
					case REG_SP:	dst_mp = &r_esp;
									break;
					case REG_BP:	dst_mp = &r_ebp;
									break;
					case REG_SI:	dst_mp = &r_esi;
									break;
					case REG_DI:	dst_mp = &r_edi;
									break;
					default:		break;			/* can't happen */
				}
			break;

			case 00:
				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
				break;
		}
		switch(subcode) {
			case 000:	if (opcode == 0x8E)
							r_es = *(unsigned short *)dst_mp & 0xffff;
						else
							*(unsigned short *)dst_mp = r_es;
						break;

			case 001:	if (opcode == 0x8E)
							; /* r_cs = *(unsigned short *)dst_mp; */
						else
							*(unsigned short *)dst_mp = r_cs;
						break;

			case 002:	if (opcode == 0x8E)
							r_ss = *(unsigned short *)dst_mp & 0xffff;
						else
							*(unsigned short *)dst_mp = r_ss;
						break;

			case 003:	if (opcode == 0x8E)
							r_ds = *(unsigned short *)dst_mp & 0xffff;
						else
							*(unsigned short *)dst_mp = r_ds;
						break;

			default:	break;				/* ERROR CONDITION, Cannot happen */
		}

		if (dest_is_reg) {
			SET_ALL_REGS();
			SET_ALL_BREGS();
			smp = SET_NEW_SMP();
		}
	}
	break;

	case 0xC6:
	case 0xC7: {
		short dest_is_reg,dest_reg,mod;
		void *dst_mp;

		dest_is_reg = 0;
		mod_byte = *imp++;
		dest_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is a REGISTER to REGISTER Transfer */
				dest_is_reg = 1;
				if (opcode & 0x01) {
					SET_ALL_EREGS();
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_eax;
										break;
						case REG_BX:	dst_mp = &r_ebx;
										break;
						case REG_CX:	dst_mp = &r_ecx;
										break;
						case REG_DX:	dst_mp = &r_edx;
										break;
						case REG_SP:	dst_mp = &r_esp;
										break;
						case REG_BP:	dst_mp = &r_ebp;
										break;
						case REG_SI:	dst_mp = &r_esi;
										break;
						case REG_DI:	dst_mp = &r_edi;
										break;
						default:		break;			/* can't happen */
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	dst_mp = &r_al;
										break;
						case REG_BL:	dst_mp = &r_bl;
										break;
						case REG_CL:	dst_mp = &r_cl;
										break;
						case REG_DL:	dst_mp = &r_dl;
										break;
						case REG_AH:	dst_mp = &r_ah;
										break;
						case REG_BH:	dst_mp = &r_bh;
										break;
						case REG_CH:	dst_mp = &r_ch;
										break;
						case REG_DH:	dst_mp = &r_dh;
										break;
						default:		break;			/* can't happen */
					}
				}
			break;


			case 00:
				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
				break;
		}

		if (opcode & 0x01) {
				*(unsigned long *)dst_mp = *(unsigned long *)imp;
				imp+=4;
		}
		else  {
			*(unsigned char *)dst_mp =  *(unsigned char *)imp++;
		}

		if (dest_is_reg) {
			if (opcode & 0x01) {
				SET_ALL_REGS();
				SET_ALL_BREGS();
				smp = SET_NEW_SMP();
			}
			else {
				SET_ALL_WREGS();
			}
		}

	}
	break;

	case MOV:
	case MOV|2:
	case ADC:
	case ADC|2:
	case ADD:
	case ADD|2:
	case AND:
	case AND|2:
	case CMP:
	case CMP|2:
	case OR:
	case OR|2:
	case SBB:
	case SBB|2:
	case SUB:
	case SUB|2:
	case XOR:
	case XOR|2:
	case MOVB|2:
	case MOVB:
	case ANDB:
	case ANDB|2:
	case ADCB|2:
	case ADCB:
	case ADDB|2:
	case ADDB:
	case CMPB|2:
	case CMPB:
	case ORB|2:
	case ORB:
	case SBBB|2:
	case SBBB:
	case XORB|2:
	case XORB:
	case SUBB|2:
	case SUBB: {
				short source_reg,mod,dest_reg,word_op;
				void *src_mp,*dst_mp,*tmp_mp;

				mod_byte = *imp++;
				word_op = (opcode & 0x01);
				opcode &= 0xfe;


				source_reg = mod_byte & 0x07;		/* Get source register or R/M field */
				dest_reg = (mod_byte >> 3) & 0x07;		/* Get Destination register */
				mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
				switch(mod) {		/* Handle type of operation */
					case 03:		/* This is a REGISTER to REGISTER Transfer */
						if (word_op) {
							SET_ALL_EREGS();
							switch(dest_reg) {
								case REG_AX:	src_mp = &r_eax;
												break;
								case REG_BX:	src_mp = &r_ebx;
												break;
								case REG_CX:	src_mp = &r_ecx;
												break;
								case REG_DX:	src_mp = &r_edx;
												break;
								case REG_SP:	src_mp = &r_esp;
												break;
								case REG_BP:	src_mp = &r_ebp;
												break;
								case REG_SI:	src_mp = &r_esi;
												break;
								case REG_DI:	src_mp = &r_edi;
												break;
							}

							switch(source_reg) {
								case REG_AX:	dst_mp = &r_eax;
												break;
								case REG_BX:	dst_mp = &r_ebx;
												break;
								case REG_CX:	dst_mp = &r_ecx;
												break;
								case REG_DX:	dst_mp = &r_edx;
												break;
								case REG_SP:	dst_mp = &r_esp;
												break;
								case REG_BP:	dst_mp = &r_ebp;
												break;
								case REG_SI:	dst_mp = &r_esi;
												break;
								case REG_DI:	dst_mp = &r_edi;
												break;
							}
						}
						else {
							switch(dest_reg) {
								case REG_AL:	src_mp = &r_al;
												break;
								case REG_BL:	src_mp = &r_bl;
												break;
								case REG_CL:	src_mp = &r_cl;
												break;
								case REG_DL:	src_mp = &r_dl;
												break;
								case REG_AH:	src_mp = &r_ah;
												break;
								case REG_BH:	src_mp = &r_bh;
												break;
								case REG_CH:	src_mp = &r_ch;
												break;
								case REG_DH:	src_mp = &r_dh;
												break;
							}

							switch(source_reg) {
								case REG_AL:	dst_mp = &r_al;
												break;
								case REG_BL:	dst_mp = &r_bl;
												break;
								case REG_CL:	dst_mp = &r_cl;
												break;
								case REG_DL:	dst_mp = &r_dl;
												break;
								case REG_AH:	dst_mp = &r_ah;
												break;
								case REG_BH:	dst_mp = &r_bh;
												break;
								case REG_CH:	dst_mp = &r_ch;
												break;
								case REG_DH:	dst_mp = &r_dh;
												break;
							}
						}
						if (opcode & 0x02) {			/* Direction bit set */
							tmp_mp = src_mp;			/* Reverse direction of operation */
							src_mp = dst_mp;
							dst_mp = tmp_mp;			
						}
						break;


					case 00:
						switch(source_reg) {			/* r/m */
							case 000:	if (new_seg) 
											dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
										else
											dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
										break;
							case 001:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
										else
											dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
										break;
							case 002:	if (new_seg) 
											dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
										else
											dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
										break;
							case 003:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
										else
											dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
										break;
							case 004:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
										else
											dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
										break;
							case 005:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
										else
											dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
										break;
							case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
										imp+=2;
										if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
										else
											dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
										break;
							case 007:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
										else
											dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
										break;
						}
						if (word_op) {
							SET_ALL_EREGS();
							switch(dest_reg) {
								case REG_AX:	src_mp = &r_eax;
												break;
								case REG_BX:	src_mp = &r_ebx;
												break;
								case REG_CX:	src_mp = &r_ecx;
												break;
								case REG_DX:	src_mp = &r_edx;
												break;
								case REG_SP:	src_mp = &r_esp;
												break;
								case REG_BP:	src_mp = &r_ebp;
												break;
								case REG_SI:	src_mp = &r_esi;
												break;
								case REG_DI:	src_mp = &r_edi;
												break;
							}
						}
						else {
							switch(dest_reg) {
								case REG_AL:	src_mp = &r_al;
												break;
								case REG_BL:	src_mp = &r_bl;
												break;
								case REG_CL:	src_mp = &r_cl;
												break;
								case REG_DL:	src_mp = &r_dl;
												break;
								case REG_AH:	src_mp = &r_ah;
												break;
								case REG_BH:	src_mp = &r_bh;
												break;
								case REG_CH:	src_mp = &r_ch;
												break;
								case REG_DH:	src_mp = &r_dh;
												break;
							}
						}
						if (opcode & 0x02) {			/* Direction bit set */
							tmp_mp = src_mp;			/* Reverse direction of operation */
							src_mp = dst_mp;
							dst_mp = tmp_mp;			
						}
						break;

					case 01:
						disp_8 = *imp++;

						switch(source_reg) {			/* r/m */
							case 000:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
										else
											dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
										break;
							case 001:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
										else
											dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
										break;
							case 002:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
										else
											dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
										break;
							case 003:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
										else
											dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
										break;
							case 004:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
										else
											dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
										break;
							case 005:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
										else
											dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
										break;
							case 006:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
										else
											dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
										break;
							case 007:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
										else
											dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
										break;
						}
						if (word_op) {
							SET_ALL_EREGS();
							switch(dest_reg) {
								case REG_AX:	src_mp = &r_eax;
												break;
								case REG_BX:	src_mp = &r_ebx;
												break;
								case REG_CX:	src_mp = &r_ecx;
												break;
								case REG_DX:	src_mp = &r_edx;
												break;
								case REG_SP:	src_mp = &r_esp;
												break;
								case REG_BP:	src_mp = &r_ebp;
												break;
								case REG_SI:	src_mp = &r_esi;
												break;
								case REG_DI:	src_mp = &r_edi;
												break;
							}
						}
						else {
							switch(dest_reg) {
								case REG_AL:	src_mp = &r_al;
												break;
								case REG_BL:	src_mp = &r_bl;
												break;
								case REG_CL:	src_mp = &r_cl;
												break;
								case REG_DL:	src_mp = &r_dl;
												break;
								case REG_AH:	src_mp = &r_ah;
												break;
								case REG_BH:	src_mp = &r_bh;
												break;
								case REG_CH:	src_mp = &r_ch;
												break;
								case REG_DH:	src_mp = &r_dh;
												break;
							}
						}
						if (opcode & 0x02) {			/* Direction bit set */
							tmp_mp = src_mp;			/* Reverse direction of operation */
							src_mp = dst_mp;
							dst_mp = tmp_mp;			
						}
					break;

					case 02:
						disp_u16 = *(unsigned short *)imp;
						imp+=2;

						switch(source_reg) {			/* r/m */
							case 000:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
										else
											dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
										break;
							case 001:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
										else
											dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
										break;
							case 002:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
										else
											dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
										break;
							case 003:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
										else
											dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
										break;
							case 004:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
										else
											dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
										break;
							case 005:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
										else
											dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
										break;
							case 006:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
										else
											dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
										break;
							case 007:	if (new_seg)
											dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
										else
											dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
										break;
						}
						if (word_op) {
							SET_ALL_EREGS();
							switch(dest_reg) {
								case REG_AX:	src_mp = &r_eax;
												break;
								case REG_BX:	src_mp = &r_ebx;
												break;
								case REG_CX:	src_mp = &r_ecx;
												break;
								case REG_DX:	src_mp = &r_edx;
												break;
								case REG_SP:	src_mp = &r_esp;
												break;
								case REG_BP:	src_mp = &r_ebp;
												break;
								case REG_SI:	src_mp = &r_esi;
												break;
								case REG_DI:	src_mp = &r_edi;
												break;
							}
						}
						else {
							switch(dest_reg) {
								case REG_AL:	src_mp = &r_al;
												break;
								case REG_BL:	src_mp = &r_bl;
												break;
								case REG_CL:	src_mp = &r_cl;
												break;
								case REG_DL:	src_mp = &r_dl;
												break;
								case REG_AH:	src_mp = &r_ah;
												break;
								case REG_BH:	src_mp = &r_bh;
												break;
								case REG_CH:	src_mp = &r_ch;
												break;
								case REG_DH:	src_mp = &r_dh;
												break;
							}
						}
						if (opcode & 0x02) {			/* Direction bit set */
							tmp_mp = src_mp;			/* Reverse direction of operation */
							src_mp = dst_mp;
							dst_mp = tmp_mp;			
						}
						break;
				default:	break;
			}
			if (opcode & 0x02)
				opcode &= ~0x02;			/* Strip out direction bit, since it is handled in the mod/reg/rm decode */

			if (word_op) {
				switch(opcode) {
					case ADDB:	tmp_32l = *(long *)dst_mp & 0xffff;
								tmp_32h = ((*(long *)dst_mp) >> 16) & 0xffff;
								sign = tmp_32h & 0x8000;
								tmp_32l += *(short *)src_mp;				
								tmp_32h += (*(short *)((short *)src_mp+1) + ((tmp_32l & 0xf0000) ? 1: 0));
								SET_OF_SF(sign,tmp_32h); SET_ZF_PF(*(unsigned long *)dst_mp); SET_CF(tmp_32h);
								*(unsigned long*)dst_mp = tmp_32h << 16 | tmp_32l;
								break;

					case ORB:	*(unsigned long *)dst_mp |= *(unsigned long *)src_mp;		/* OR 32 bit */
								SET_ZF_PF((*(unsigned long *)dst_mp));
								SET_SF_32((*(unsigned long *)dst_mp));
								CF = OF = 0;
								break;

					case ADCB:	tmp_32l = *(long *)dst_mp & 0xffff;
								tmp_32h = ((*(long *)dst_mp) >> 16) & 0xffff;
								sign = tmp_32h & 0x8000;
								tmp_32l += (*(short *)src_mp + CF);
								tmp_32h += (*(short *)((short *)src_mp+1) + ((tmp_32l & 0xf0000) ? 1: 0));
								*(unsigned long*)dst_mp = tmp_32h << 16 | tmp_32l;
								SET_OF_SF(sign,tmp_32h); SET_ZF_PF(*(unsigned long *)dst_mp); SET_CF(tmp_32h);
								break;

					case SBBB:	tmp_32l = *(long *)dst_mp;
								tmp_32h = (tmp_32l >> 16) & 0xffff;
								tmp_32l &= 0xffff;
								sign = tmp_32h & 0x8000;
								tmp_32l -= (*(short *)src_mp + CF);
								tmp_32h -= (*(short *)((short *)src_mp+1)+ ((tmp_32l & 0xf0000) ? 1: 0));
								*(unsigned long*)dst_mp = tmp_32h << 16 | tmp_32l;
								SET_OF_SF(sign,tmp_32h); SET_ZF_PF(*(unsigned long *)dst_mp); SET_CF(tmp_32h);
								break;

					case ANDB:	*(unsigned long *)dst_mp &= *(unsigned long *)src_mp;		/* AND 16 bit */
								SET_ZF_PF((*(unsigned short *)dst_mp));
								SET_SF_32((*(unsigned short *)dst_mp));
								CF = OF = 0;
								break;

					case SUBB:	tmp_32l = *(long *)dst_mp;
								tmp_32h = (tmp_32l >> 16) & 0xffff;
								tmp_32l &= 0xffff;
								sign = tmp_32h & 0x8000;
								tmp_32l -= *(short *)src_mp;
								tmp_32h -= (*(short *)((short *)src_mp+1) + ((tmp_32l & 0xf0000) ? 1: 0));
								*(unsigned long*)dst_mp = tmp_32h << 16 | tmp_32l;
								SET_OF_SF(sign,tmp_32h); SET_ZF_PF(*(unsigned long *)dst_mp); SET_CF(tmp_32h);
								break;

					case XORB:	*(unsigned long *)dst_mp ^= *(unsigned long *)src_mp;		/* XOR 16 bit */
								SET_ZF_PF((*(unsigned long *)dst_mp));
								SET_SF_32((*(unsigned long *)dst_mp));
								CF = OF = 0;
								break;

					case CMPB:	tmp_32l = *(long *)dst_mp;
								tmp_32h = (tmp_32l >> 16) & 0xffff;
								tmp_32l &= 0xffff;
								sign = tmp_32h & 0x8000;
								tmp_32l -= *(short *)src_mp;
								tmp_32h -= (*(short *)((short *)src_mp+1) + ((tmp_32l & 0xf0000) ? 1: 0));
								SET_OF_SF(sign,tmp_32h); SET_ZF_PF(((tmp_32&0xffff)|((tmp_32h & 0xffff) << 16))); SET_CF(tmp_32h);
								break;

					case MOVB:	*(unsigned long *)dst_mp = *(unsigned long *)src_mp;
								break;

					default:	break;				/* ERROR CONDITION, Cannot happen */
				}
			}
			else {
				switch(opcode) {
					case ADDB:	tmp_16 = *(char *)dst_mp;						/* ADD  8 bit */
								sign = tmp_16 & 0x80;
								tmp_16 += *(char *)src_mp;
								*(unsigned char *)dst_mp = (tmp_16 & 0xff);
								SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
								break;

					case ORB:	*(unsigned char *)dst_mp |= *(unsigned char *)src_mp;		/* OR  8 bit */
								SET_ZF_PF_8((*(unsigned char *)dst_mp));
								SET_SF_8((*(unsigned char *)dst_mp));
								CF = OF = 0;
								break;

					case ADCB:	tmp_16 = *(char *)dst_mp;						/* ADC  8 bit */
								sign = tmp_16 & 0x80;
								tmp_16 += ((*(char *)src_mp) + CF);
								*(unsigned char *)dst_mp = (tmp_16 & 0xff);
								SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
								break;

					case SBBB:	tmp_16 = *(char *)dst_mp;						/* SBB  8 bit */
								sign = tmp_16 & 0x80;
								tmp_16 -= ((*(char *)src_mp) + CF);
								*(unsigned char *)dst_mp = (tmp_16 & 0xff);
								SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
								break;

					case ANDB:	*(unsigned char *)dst_mp &= *(unsigned char *)src_mp;		/* AND  8 bit */
								SET_ZF_PF_8((*(unsigned char *)dst_mp));
								SET_SF_8((*(unsigned char *)dst_mp));
								CF = OF = 0;
								break;

					case SUBB:	tmp_16 = *(char *)dst_mp;						/* SUB  8 bit */
								sign = tmp_16 & 0x80;
								tmp_16 -= *(char *)src_mp;
								*(unsigned char *)dst_mp = (tmp_16 & 0xff);
								SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
								break;

					case XORB:	*(unsigned char *)dst_mp ^= *(unsigned char *)src_mp;		/* XOR  8 bit */
								SET_ZF_PF_8((*(unsigned char *)dst_mp));
								SET_SF_8((*(unsigned char *)dst_mp));
								CF = OF = 0;
								break;

					case CMPB:	tmp_16 = *(char *)dst_mp;						/* CMP  8 bit */
								sign = tmp_16 & 0x80;
								tmp_16 -= *(char *)src_mp;
								SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
								break;

					case MOVB:	*(unsigned char *)dst_mp = *(unsigned char *)src_mp;
								break;

					default:	break;				/* ERROR CONDITION, Cannot happen */
				}
			}

			if (word_op) {
				SET_ALL_REGS();
				SET_ALL_BREGS();
				smp = SET_NEW_SMP();
			}
			else {
				SET_ALL_WREGS();
			}
		}
		break;
	case LEA: {					/* ADD opsize override here */
		short src_reg,dest_reg,mod;
		unsigned short eff_addr;

		mod_byte = *imp++;
		src_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		dest_reg = (mod_byte >> 3) & 0x07;
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is an illegal instruction */
				break;		

			case 00:
				switch(src_reg) {			/* r/m */
					case 000:	eff_addr = SET_NEW_EA(r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	eff_addr = SET_NEW_EA(r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	eff_addr = SET_NEW_EA(r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	eff_addr = SET_NEW_EA(r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	eff_addr = SET_NEW_EA(0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	eff_addr = SET_NEW_EA(0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								eff_addr = SET_NEW_EA(disp_u16,0,0);
								break;
					case 007:	eff_addr = SET_NEW_EA(r_bx,0,0);		/* DS:[BX] */
								break;
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(src_reg) {			/* r/m */
					case 000:	eff_addr = SET_NEW_EA(r_bx,r_si,disp_8);
								break;
					case 001:	eff_addr = SET_NEW_EA(r_bx,r_di,disp_8);
								break;
					case 002:	eff_addr = SET_NEW_EA(r_bp,r_si,disp_8);
								break;
					case 003:	eff_addr = SET_NEW_EA(r_bp,r_di,disp_8);
								break;
					case 004:	eff_addr = SET_NEW_EA(0,r_si,disp_8);
								break;
					case 005:	eff_addr = SET_NEW_EA(0,r_di,disp_8);
								break;
					case 006:	eff_addr = SET_NEW_EA(0,r_bp,disp_8);
								break;
					case 007:	eff_addr = SET_NEW_EA(r_bx,0,disp_8);
								break;
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(src_reg) {			/* r/m */
					case 000:	eff_addr = SET_NEW_EA(r_bx,r_si,disp_u16);
								break;
					case 001:	eff_addr = SET_NEW_EA(r_bx,r_di,disp_u16);
								break;
					case 002:	eff_addr = SET_NEW_EA(r_bp,r_si,disp_u16);
								break;
					case 003:	eff_addr = SET_NEW_EA(r_bp,r_di,disp_u16);
								break;
					case 004:	eff_addr = SET_NEW_EA(0,r_si,disp_u16);
								break;
					case 005:	eff_addr = SET_NEW_EA(0,r_di,disp_u16);
								break;
					case 006:	eff_addr = SET_NEW_EA(0,r_bp,disp_u16);
								break;
					case 007:	eff_addr = SET_NEW_EA(r_bx,0,disp_u16);
								break;
				}
				break;
		}
		switch(dest_reg) {
			case REG_AX:	r_eax = r_ax = eff_addr;
							SET_AL_AH();
							break;
			case REG_BX:	r_ebx = r_bx = eff_addr;
							SET_BL_BH();
							break;
			case REG_CX:	r_ecx = r_cx = eff_addr;
							SET_CL_CH();
							break;
			case REG_DX:	r_edx = r_dx = eff_addr;
							SET_DL_DH();
							break;
			case REG_SP:	r_esp = r_sp = eff_addr;
							break;
			case REG_BP:	r_ebp = r_bp = eff_addr;
							break;
			case REG_SI:	r_esi = r_si = eff_addr;
							break;
			case REG_DI:	r_edi = r_di = eff_addr;
							break;
			default:		break;
		}
	}
	break;

	case LDS:
	case LES: {
		short src_reg,mod,dest_reg;
		void *dst_mp;

		mod_byte = *imp++;
		src_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		dest_reg = (mod_byte >> 3) & 0x07;
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is an illegal instruction */
				break;		

			case 00:
				switch(src_reg) {			/* r/m */
					case 000:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(src_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(src_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
				break;
		}
		switch(dest_reg) {
			case REG_AX:	r_eax = r_ax = *(unsigned short *)dst_mp;
							SET_AL_AH();
							break;
			case REG_BX:	r_ebx = r_bx = *(unsigned short *)dst_mp;
							SET_BL_BH();
							break;
			case REG_CX:	r_ecx = r_cx = *(unsigned short *)dst_mp;
							SET_CL_CH();
							break;
			case REG_DX:	r_edx = r_dx = *(unsigned short *)dst_mp;
							SET_DL_DH();
							break;
			case REG_SP:	r_esp = r_sp = *(unsigned short *)dst_mp;
							break;
			case REG_BP:	r_ebp = r_bp = *(unsigned short *)dst_mp;
							break;
			case REG_SI:	r_esi = r_si = *(unsigned short *)dst_mp;
							break;
			case REG_DI:	r_edi = r_di = *(unsigned short *)dst_mp;
							break;
			default:		break;
		}
		if (opcode == LDS)
			r_ds = *(unsigned short *)((unsigned short *)dst_mp+1);
		else
			r_es = *(unsigned short *)((unsigned short *)dst_mp+1);

	}
	break;
	case 0xFE:
	case 0xFF: {
		short dest_is_reg,mod,dest_reg,subcode;
		void *dst_mp;

		dest_is_reg = 0;
		mod_byte = *imp++;
		dest_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		subcode = (mod_byte >> 3) & 0x07;
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is a REGISTER to REGISTER Transfer */
				dest_is_reg = 1;
				if (opcode & 0x01) {
					SET_ALL_EREGS();
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_eax;
										break;
						case REG_BX:	dst_mp = &r_ebx;
										break;
						case REG_CX:	dst_mp = &r_ecx;
										break;
						case REG_DX:	dst_mp = &r_edx;
										break;
						case REG_SP:	dst_mp = &r_esp;
										break;
						case REG_BP:	dst_mp = &r_ebp;
										break;
						case REG_SI:	dst_mp = &r_esi;
										break;
						case REG_DI:	dst_mp = &r_edi;
										break;
						default:		break;			/* can't happen */
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	dst_mp = &r_al;
										break;
						case REG_BL:	dst_mp = &r_bl;
										break;
						case REG_CL:	dst_mp = &r_cl;
										break;
						case REG_DL:	dst_mp = &r_dl;
										break;
						case REG_AH:	dst_mp = &r_ah;
										break;
						case REG_BH:	dst_mp = &r_bh;
										break;
						case REG_CH:	dst_mp = &r_ch;
										break;
						case REG_DH:	dst_mp = &r_dh;
										break;
						default:		break;			/* can't happen */
					}
				}
			break;


			case 00:
				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
				break;
		}
		switch(subcode) {
			case 000:	if (opcode & 0x01) {								/* INC 32 bit */
							tmp_32l = *(unsigned long *)dst_mp;
							tmp_32h = (tmp_32l >> 16) & 0xffff;
							tmp_32l &= 0xffff;
							sign = tmp_32h & 0x8000;
							tmp_32l++;
							tmp_32h += ((tmp_32l & 0xf0000) ? 1: 0);
							*(unsigned long *)dst_mp = tmp_32h << 16 | tmp_32l;
							SET_OF_SF(sign,tmp_32h); SET_ZF_PF(*(unsigned long *)dst_mp);
						}
						else {
							tmp_16 = *(char *)dst_mp;						/* INC 8 bit */
							sign = tmp_16 & 0x80;
							tmp_16++;
							*(unsigned char *)dst_mp = (tmp_16 & 0xff);
							SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16);
						}
						break;

			case 001:	if (opcode & 0x01) {								/* DEC 32 bit */
							tmp_32l = *(unsigned long *)dst_mp;
							tmp_32h = (tmp_32l >> 16) & 0xffff;
							tmp_32l &= 0xffff;
							sign = tmp_32h & 0x8000;
							tmp_32l--;
							tmp_32h -= ((tmp_32l & 0xf0000) ? 1: 0);
							*(unsigned long *)dst_mp = tmp_32h << 16 | tmp_32l;
							SET_OF_SF(sign,tmp_32h); SET_ZF_PF(*(unsigned long *)dst_mp);
						}
						else {
							tmp_16 = *(char *)dst_mp;						/* DEC 8 bit */
							sign = tmp_16 & 0x80;
							tmp_16--;
							*(unsigned char *)dst_mp = (tmp_16 & 0xff);
							SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16);
						}
						break;

			case 002:	smp = SET_NEW_SMP();
						r_ip = GET_IP();							/* Call Near */
						*(smp-1) = r_ip;
						r_sp -= 2;
						r_ip = *(unsigned short *)dst_mp;
						imp = SET_NEW_IMP(r_cs,r_ip);				/* execution will resume here ! */
						break;

			case 003:	smp = SET_NEW_SMP();
						r_ip = *(unsigned short *)dst_mp;			/* Call FAR, note that value of MOD == 0x03 is illegal */
						*(smp-1) = r_cs;
						*(smp-2) = GET_IP();						/* push the stack segment and  */
						r_cs = *(unsigned short *)((unsigned short *)dst_mp+1);
						r_sp -= 4;									/* Adjust the stack */
						smp = SET_NEW_SMP();
						imp = SET_NEW_IMP(r_cs,r_ip);				/* execution will resume here! */
						break;

			case 004:	r_ip = *(unsigned short *)dst_mp;			/* JMP NEAR */
						imp = SET_NEW_IMP(r_cs,r_ip);						/* execution will resume here! */
						break;

			case 005:	r_ip = *(unsigned short *)dst_mp;			/* JMP FAR */
						r_cs = *(unsigned short *)((unsigned short *)dst_mp+1);
						imp = SET_NEW_IMP(r_cs,r_ip);				/* execution will resume here! */
						break;


			case 006:	r_sp -= 4;
						smp = SET_NEW_SMP();
						*(unsigned long *)smp = *(unsigned long *)dst_mp;
						break;

			default:	break;				/* ERROR CONDITION, Cannot happen */
		}

		if (dest_is_reg) {
			if (opcode & 0x01) {
				SET_ALL_BREGS();
				smp = SET_NEW_SMP();
			}
			else {
				SET_ALL_WREGS();
			}
		}

	}
	break;

	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83: {

		short dest_reg, mod,word_op,sub_code;
		void *dst_mp;
		short dest_is_reg;

		mod_byte = *imp++;
		dest_reg = mod_byte & 0x07;		/* Get source register or R/M field */
		mod  = (mod_byte >> 6) & 0x03;			/* Get type of transfer */
		sub_code = (mod_byte >> 3) & 0x07;
		dest_is_reg = 0;
		word_op = opcode & 0x01 ;
		switch(mod) {		/* Handle type of operation */
			case 03:		/* This is a REGISTER to REGISTER Transfer */
				dest_is_reg = 1;
				if (word_op) {
					SET_ALL_EREGS();
					switch(dest_reg) {
						case REG_AX:	dst_mp = &r_eax;
										break;
						case REG_BX:	dst_mp = &r_ebx;
										break;
						case REG_CX:	dst_mp = &r_ecx;
										break;
						case REG_DX:	dst_mp = &r_edx;
										break;
						case REG_SP:	dst_mp = &r_esp;
										break;
						case REG_BP:	dst_mp = &r_ebp;
										break;
						case REG_SI:	dst_mp = &r_esi;
										break;
						case REG_DI:	dst_mp = &r_edi;
										break;
						default:		break;			/* can't happen */
					}
				}
				else {
					switch(dest_reg) {
						case REG_AL:	dst_mp = &r_al;
										break;
						case REG_BL:	dst_mp = &r_bl;
										break;
						case REG_CL:	dst_mp = &r_cl;
										break;
						case REG_DL:	dst_mp = &r_dl;
										break;
						case REG_AH:	dst_mp = &r_ah;
										break;
						case REG_BH:	dst_mp = &r_bh;
										break;
						case REG_CH:	dst_mp = &r_ch;
										break;
						case REG_DH:	dst_mp = &r_dh;
										break;
						default:		break;			/* can't happen */
					}
				}
			break;


			case 00:
				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,0);	/* DS:[BX + SI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,0);	/* DS:[BX + SI ] */
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,0);	/* DS:[BX + DI ] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,0);	/* DS:[BX + DI ] */
								break;
					case 002:	if (new_seg) 
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,0);	/* SS:[SI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,0);	/* SS:[SI + BP ] */
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,0);	/* SS:[DI + BP ] */
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,0);	/* SS:[DI + BP ] */
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,0);		/* DS:[SI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,0);		/* DS:[SI] */
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,0);		/* DS:[DI] */
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,0);		/* DS:[DI] */
								break;
					case 006:	disp_u16 = *(unsigned short *)imp;		/* DS:[DISP] */
								imp+=2;
								if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,disp_u16,0,0);
								else
									dst_mp = SET_NEW_DMP(r_ds,disp_u16,0,0);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,0);		/* DS:[BX] */
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,0);		/* DS:[BX] */
								break;
				}
				break;

			case 01:
				disp_8 = *imp++;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_8);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_8);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_8);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_8);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_8);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_8);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_8);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_8);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_8);
								break;
				}
				break;

			case 02:
				disp_u16 = *(unsigned short *)imp;
				imp+=2;

				switch(dest_reg) {			/* r/m */
					case 000:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_si,disp_u16);
								break;
					case 001:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,r_di,disp_u16);
								break;
					case 002:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_si,disp_u16);
								break;
					case 003:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bp,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,r_bp,r_di,disp_u16);
								break;
					case 004:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_si,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_si,disp_u16);
								break;
					case 005:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_di,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,0,r_di,disp_u16);
								break;
					case 006:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,0,r_bp,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ss,0,r_bp,disp_u16);
								break;
					case 007:	if (new_seg)
									dst_mp = SET_NEW_DMP(seg_ovr,r_bx,0,disp_u16);
								else
									dst_mp = SET_NEW_DMP(r_ds,r_bx,0,disp_u16);
								break;
				}
				break;
		}
		if (opcode & 0x01) {		/* Word operation */
			if (opcode & 0x02) {	/* sign extend , only 8 bits present */
				tmp_32 = *(char *)imp++;		/* get the data */
				if (tmp_32 & 0x80) {
					tmp_32 = tmp_16 | 0xffffff00;		/* This should be sign extended by the compiler, but let's be safe */
				}
			}
			else {
				if (opsize) {	/* S000 opposite meaning because of 386 mode */
					tmp_16 = *(short *)imp;		/* get the data */
					imp+=2;
				}
				else {
					tmp_32 = *(long *)imp;		/* get the data */
					imp+=4;
				}
			}
		}
		else {
			tmp_8 = *imp++;
		}

		if (word_op) {
			switch(sub_code) {
				case 000:	tmp_32l = *(long *)dst_mp & 0xffff;					/* ADD imm */
							tmp_32h = ((*(long *)dst_mp) >> 16) & 0xffff;
							sign = tmp_32h & 0x8000;
							tmp_32l += (tmp_32 & 0xffff);				
							tmp_32h += ((tmp_32 >> 16 & 0xffff) + ((tmp_32l & 0xf0000) ? 1: 0));
							SET_OF_SF(sign,tmp_32h); SET_ZF_PF(*(unsigned long *)dst_mp); SET_CF(tmp_32h);
							*(unsigned long*)dst_mp = tmp_32h << 16 | tmp_32l;
							break;

				case 001:	*(unsigned long *)dst_mp |= tmp_32;		/* OR Immediate */
							SET_ZF_PF((*(unsigned long *)dst_mp));
							SET_SF_32((*(unsigned long *)dst_mp));
							CF = OF = 0;
							break;

				case 002:	tmp_32l = *(long *)dst_mp & 0xffff;					/* ADD imm */
							tmp_32h = ((*(long *)dst_mp) >> 16) & 0xffff;
							sign = tmp_32h & 0x8000;
							tmp_32l += ((tmp_32 & 0xffff) + CF);				
							tmp_32h += ((tmp_32 >> 16 & 0xffff) + ((tmp_32l & 0xf0000) ? 1: 0));
							SET_OF_SF(sign,tmp_32h); SET_ZF_PF(*(unsigned long *)dst_mp); SET_CF(tmp_32h);
							*(unsigned long*)dst_mp = tmp_32h << 16 | tmp_32l;
							break;

				case 003:	/* SBB */
							tmp_32l = *(long *)dst_mp;
							tmp_32h = (tmp_32l >> 16) & 0xffff;
							tmp_32l &= 0xffff;
							sign = tmp_32h & 0x8000;
							tmp_32l -= ((tmp_32l & 0xffff) + CF);
							tmp_32h -= ((tmp_32 >> 16 & 0xffff) +((tmp_32l & 0xf0000) ? 1: 0));
							*(unsigned long*)dst_mp = tmp_32h << 16 | tmp_32l;
							SET_OF_SF(sign,tmp_32h); SET_ZF_PF(*(unsigned long *)dst_mp); SET_CF(tmp_32h);
							break;
							

				case 004:	*(unsigned long *)dst_mp &= tmp_32;		/* AND Immediate */
							SET_ZF_PF((*(unsigned long *)dst_mp));
							SET_SF_32((*(unsigned long *)dst_mp));
							CF = OF = 0;
							break;

				case 005:	/* SUB */
							tmp_32l = *(long *)dst_mp;
							tmp_32h = (tmp_32l >> 16) & 0xffff;
							tmp_32l &= 0xffff;
							sign = tmp_32h & 0x8000;
							tmp_32l -= (tmp_32l & 0xffff);
							tmp_32h -= ((tmp_32 >> 16 & 0xffff) +((tmp_32l & 0xf0000) ? 1: 0));
							*(unsigned long*)dst_mp = tmp_32h << 16 | tmp_32l;
							SET_OF_SF(sign,tmp_32h); SET_ZF_PF(*(unsigned long *)dst_mp); SET_CF(tmp_32h);
							break;

				case 006:	*(unsigned long *)dst_mp ^= tmp_32;		/* XOR Immediate */
							SET_ZF_PF((*(unsigned long *)dst_mp));
							SET_SF_32((*(unsigned long *)dst_mp));
							CF = OF = 0;
							break;

				case 007:	tmp_32l = *(long *)dst_mp;				/* CMP */
							tmp_32h = (tmp_32l >> 16) & 0xffff;
							tmp_32l &= 0xffff;
							sign = tmp_32h & 0x8000;
							tmp_32l -= (tmp_32l & 0xffff);
							tmp_32h -= ((tmp_32 >> 16 & 0xffff) +((tmp_32l & 0xf0000) ? 1: 0));
							SET_OF_SF(sign,tmp_32h); SET_ZF_PF(((tmp_32l & 0xffff) | ((tmp_32h << 16) & 0xffff))); SET_CF(tmp_32h);
							break;

							break;

				default:	break;				/* ERROR CONDITION, Cannot happen */
			}
		}
		else {
			switch(sub_code) {
				case 000:	tmp_16 = *(char *)dst_mp;						/* ADD Immediate 8 bit */
							sign = tmp_16 & 0x80;
							tmp_16 += tmp_8;
							*(unsigned char *)dst_mp = (tmp_16 & 0xff);
							SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
							break;

				case 001:	*(unsigned char *)dst_mp |= tmp_8;		/* OR Immediate 8 bit */
							SET_ZF_PF_8((*(unsigned char *)dst_mp));
							SET_SF_8((*(unsigned char *)dst_mp));
							CF = OF = 0;
							break;

				case 002:	tmp_16 = *(char *)dst_mp;						/* ADC Immediate 8 bit */
							sign = tmp_16 & 0x80;
							tmp_16 += (tmp_8 + CF);
							*(unsigned char *)dst_mp = (tmp_16 & 0xff);
							SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
							break;

				case 003:	tmp_16 = *(char *)dst_mp;						/* SBB Immediate 8 bit */
							sign = tmp_16 & 0x80;
							tmp_16 -= (tmp_8 + CF);
							*(unsigned char *)dst_mp = (tmp_16 & 0xff);
							SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
							break;

				case 004:	*(unsigned char *)dst_mp &= tmp_8;		/* AND Immediate 8 bit */
							SET_ZF_PF_8((*(unsigned char *)dst_mp));
							SET_SF_8((*(unsigned char *)dst_mp));
							CF = OF = 0;
							break;

				case 005:	tmp_16 = *(char *)dst_mp;						/* SUB Immediate 8 bit */
							sign = tmp_16 & 0x80;
							tmp_16 -= tmp_8;
							*(unsigned char *)dst_mp = (tmp_16 & 0xff);
							SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
							break;

				case 006:	*(unsigned char *)dst_mp ^= tmp_8;		/* XOR Immediate 8 bit */
							SET_ZF_PF_8((*(unsigned char *)dst_mp));
							SET_SF_8((*(unsigned char *)dst_mp));
							CF = OF = 0;
							break;

				case 007:	tmp_16 = *(char *)dst_mp;						/* CMP Immediate 8 bit */
							sign = tmp_16 & 0x80;
							tmp_16 -= tmp_8;
							SET_OF_SF_8(sign,tmp_16); SET_ZF_PF_8(tmp_16); SET_CF_8(tmp_16);
							break;

				default:	break;				/* ERROR CONDITION, Cannot happen */
			}
		}

		if (dest_is_reg) {
			if (word_op) {
				SET_ALL_REGS();
				SET_ALL_BREGS();
				smp = SET_NEW_SMP();
			}
			else {
				SET_ALL_WREGS();
			}
		}
	}
	break;


	case OPSIZE:		/* Operand size override */
				opsize = 1;	/* in this case, since we are currently
				goto top_loop; /* in 32 bit, mode, this means 16 bit mode */
				break;

	case 0x0f:		/* Enhanced 386 instructions */
			opcode = *imp++;
		ErrorF("decode_386: Unimplemented Enhanced 386 instruction opcode = %02x at %04x:%04x\n",opcode,r_cs,r_ip);
				return 1;
		break;

	default:
		ErrorF("decode_386: Unknown opcode %02x at %04x:%04x\n",opcode,r_cs,r_ip);
				return 1;				/*	S000	*/
				break;
	}
	r_ip = GET_IP();
	return 0;
}	/*	static int decode_386()	*/

/*
 *	IO opcodes, 0xE? where ? is:
 *
 *	0100	IN	AL	imm			-	INB
 *	0101	IN	AX/EAX	imm		-	INW
 *	0110	OUT	AL	imm			-	OUTAL
 *	0111	OUT	AX/EAX	imm		-	OUTAX
 *	1100	IN	AL	DX			-	INB_DX
 *	1101	IN	AX/EAX	DX		-	INW_DX
 *	1110	OUT	AL	DX			-	OUTDX_AL
 *	1111	OUT	AX/EAX	DX		-	OUTDX_AX
 *
 *	Implies:
 *	1000	selects imm/DX
 *	0010	selects IN/OUT
 *	0001	selects	AL or AX/EAX and AX/EAX will be determined by IOsize
 */
/*
 *	PerformIO - take care of all IO here in one place so it can be checked
 *	- IOopcode is one of: INB_DX INW_DX INB INW OUTDX_AX OUTDX_AL OUTAL OUTAX
 *	- IOsize is 0 for 16 bit mode instructions, 1 for 32 bit mode instructions
 */
static int PerformIO( unsigned char IOopcode, unsigned char IOsize )
{
	unsigned short port;

	if( INTERP_DEBUG )
		ErrorF("PerformIO( %02x, %d )\n", IOopcode, IOsize );

	/*	determine port to check validity	*/
	if ( 0x08 & IOopcode ) {
		port = (unsigned short) r_dx;
		if( INTERP_DEBUG ) ErrorF("PerformIO: r_dx port: %04x\n", port );
	} else {
		port = (unsigned short) *imp++;
		if( INTERP_DEBUG ) ErrorF("PerformIO: imm port: %04x\n", port );
	}

	/*	verify an enabled port						S001	vvv	*/

	if ( 0x01 & IOopcode ) {	/*	0x01 on is AX/EAX	*/
		if( IOsize ) {		/*	IOsize == 1 is EAX	*/
			if (! IOEnabled(port, IO_DOUBLEWORD_MASK)) {
				if( IOPRINT ) ErrorF("decode_386: Not enabled 32 bit I/O port %02x, opcode %02x at %04x:%04x\n",port,opcode,r_cs,r_ip);
				if( IOERROR ) return 1; else return 0;
			}
		} else {
			if (! IOEnabled(port, IO_WORD_MASK)) {
				if( IOPRINT ) ErrorF("decode_386: Not enabled 16 bit I/O port %02x, opcode %02x at %04x:%04x\n",port,opcode,r_cs,r_ip);
				if( IOERROR ) return 1; else return 0;
			}
		}
	} else {
	    if (! IOEnabled(port, IO_BYTE_MASK)) {
			if( IOPRINT ) ErrorF("decode_386: Not enabled  8 bit I/O port %02x, opcode %02x at %04x:%04x\n",port,opcode,r_cs,r_ip);
			if( IOERROR ) return 1; else return 0;
			}
	}

	/*											S001	^^^	*/

	/*	ports above 0x00ff are valid for any I/O	*/
	if ( 0xff00 & port ) {			/*	perform IO for ports > 0x00ff	*/
		if ( 0x02 & IOopcode ) {	/*	0x02 on is OUT	*/
			if ( 0x01 & IOopcode ) {	/*	0x01 on is AX/EAX	*/
				if( IOsize ) {		/*	IOsize == 1 is EAX	*/
					SET_EAX();
					if( IOTRACE ) ErrorF("outl(%x, %x)\n",port, r_eax);
					outl(port,r_eax);
				} else {			/*	IOsize == 0 is AX */
					if( IOTRACE ) ErrorF("outw(%x, %x)\n",port, r_ax);
					outw(port,r_ax);
				}
			} else {				/*	0x01 off is AL	*/
				if( IOTRACE ) ErrorF("outb(%x, %x)\n",port, r_al);
				outb(port,r_al);
			}
		} else {	/*	0x02 off is IN	*/
			if ( 0x01 & IOopcode ) {	/*	0x01 on is AX/EAX	*/
				if( IOsize ) {		/*	IOsize == 1 is EAX	*/
					r_eax = inl(port);
					if( IOTRACE ) ErrorF("inl(%x) = %x\n",port, r_eax);
					SET_AX_AL_AH();
				} else {			/*	IOsize == 0 is AX */
					r_ax = inw(port);
					if( IOTRACE ) ErrorF("inw(%x) = %x\n",port, r_ax);
					SET_AL_AH();
				}
			} else {				/*	0x01 off is AL	*/
				r_al = inb(port);
	/* simulate vertical retrace, alternate 1, 0 - for faster operation */
				if( (0x3ba == port) || (0x3da == port) ) {
					r_al &= VerticalRetraceMask0;
					r_al |= VerticalRetraceSignal;
					VerticalRetraceSignal ^= VerticalRetraceMask1;
				}
				if( IOTRACE ) ErrorF("inb(%x) = %x\n", port, r_al );
				SET_AX();
			}
		}
	} else {	/*	I/O on ports < 0x0100 are special	*/
    	if ( ( port & 0xfffe ) == 0x70) {
				V86CMOSEmulate(IOopcode, port);
		} else if ((port & 0xfffc) == 0x40) {
				V86PITEmulate(IOopcode, port);
		} else {
			if( IOPRINT ) ErrorF("decode_386: Illegal  8 bit I/O port %02x, opcode %02x at %04x:%04x\n",port,opcode,r_cs,r_ip);
			if( IOERROR ) return 1; else return 0;
		}
	}
	return 0;
}

/*
 * V86CMOSEmulate() -	CMOS i/o emulation
 *
 *			maintain our own soft copy of CMOS RAM
 *			(this code copied from v86bios.c -- hiramc 07 NOV 96)
 */
static int V86CMOSEmulate(unsigned char op, unsigned short addr)
{
	static short		cmos[128];
	static int 			index	= -1;

	if (index < 0) {
		for (index = 128; --index >= 0; )
			cmos[index] = -1;
		index = 0;
	}

	if (! (addr & 1)) {		/* index port involved */
		if (op & 2)		    /* out */
			index = r_al & 0x7F;	/* high bit is NMI mask */
		else {			    /* in  */
			r_al = index;		/* return 0xFF like hardware ? */
			SET_AX();
		}
	}

	if ((addr | op) & 1) {	/* data port involved */
		if (op & 2)		    /* out */
		{
			cmos[index] = (addr & 1) ? r_al : r_ah;
			if( INTERP_DEBUG ) ErrorF("write CMOS[%x] = %x\n", index, cmos[index] );
		} else {			    /* in  */
			if (cmos[index] < 0) {
				outb(0x70, (unsigned char) index);
		/*
		 * the kernel could screw us up right here
		 * by interrupting and changing port 0x70,
		 * but we really don't want to start using
		 * cli/sti in user-mode code 
		 */
				cmos[index] = inb(0x71);
				if( IOTRACE ) {
					ErrorF("outb(%x) = %x\n", 0x70, index );
					ErrorF("inb(%x) = %x\n", 0x71, cmos[index] );
				}
			}
			if (addr & 1)  {
				r_al = cmos[index];
				SET_AX();
			} else {
				r_ah = cmos[index];
				SET_AX();
			}
			if( INTERP_DEBUG ) ErrorF("read CMOS[%02x] = %02x\n", index, cmos[index] );
		}
	}

	return 0;
}

#include	<sys/timeb.h>
#include	<time.h>

static unsigned char latch_PIT = 1;
static unsigned char PIT_calls = 0;
static unsigned pit_counter;
static unsigned last_ticks;
static unsigned last_time_ms;
static unsigned pit_toggle;
static unsigned pittime;
static unsigned char lower_byte;
static struct timeb		tp;

#define PITFREQ 1193167			/* hz 8254 crystal frequency, = 1.193167 Mhz */
		/*	this implies a period of 1/1193167 = 0.000000838 seconds
		 *	or 0.838 microseconds.  The counter is only 16 bits, therefore
		 *	can count only 65536 * 0.838 us = 0.05429 seconds = 54.29 ms
		 *	The resolution on ftime is only 0.010 seconds, therefore
		 *	this emulator would return tick counts in increments of
		 *	at least 11930 = (1193167/100) if going strictly by the clock.
		 *	Measurements show that it returns 11930 once in about every
		 *	8 to 12 calls, returning 0 for all the other calls (on a 60Mhz P5)
		 *  Therefore, I have added a fudge factor, adding 1 millisecond to
		 *	the time for every two calls.
		 *	It is OK to play fast and loose with this timer because it is
		 *	a wrap-around timer anyway, and any user of this timer is aware
		 *	that it may wrap-around on them between calls to it.  The callers
		 *	are used to this and take care of that problem themselves.
		 */

/*
 * V86PITEmulate() -	PIT (programmable interval timer) emulation
 *	Very simple pit emulator from Rob Tarte.
 *	This code in no way simulates a real pit.
 */
static int V86PITEmulate(unsigned char IOopcode, unsigned short port)
{
	unsigned		timeval;
	unsigned		ticks;

	if( INTERP_DEBUG ) ErrorF("PIT Emulate, port %02x\n", port );

	if (port == 0x43)
	{
		latch_PIT = 1;
		/* This is the control port */
		/* We will assume that this is an outb */
		/* asking for the time to be latched	*/
		if( IOTRACE ) ErrorF("outb(%x, %x)\n",port, r_al);
	}

	if( latch_PIT ) {
		ftime(&tp);
		latch_PIT = 0;
		if( INTERP_DEBUG )
			ErrorF("PIT Emulate: latched time: %d.%03d (%#04x), time %% 10 %d\n", tp.time, tp.millitm, tp.millitm, (tp.time % 10) );

		/*
	 	 * The PIT has a time resolution less than 1 second
		 * timeval is range [0,9999] milliseconds
	 	 */
		timeval = ((tp.time % 10) * 1000) + tp.millitm;

		if( ! (timeval - last_time_ms) ) {	/*	when equal to previous call	*/
			timeval +=  (++PIT_calls >> 1);	/*	add 1 ms for every two calls */
			PIT_calls &= 0x0f;	/*	do not add more than 8 ms	*/
				/* because the next new timeval will only be 10 ms more */
		} else {
			PIT_calls = 0;
			last_time_ms = timeval;
		}

		ticks = timeval * ((PITFREQ) / 1000);

		if (ticks - last_ticks > 65536) {
			pittime = pittime + 0x4000;
		} else {
			pittime = ticks - last_ticks;
		}

		if( INTERP_DEBUG ) {
			ErrorF("PIT Emulate: timeval = %08x = %d, ticks = %08x = %d\n", timeval, timeval, ticks, ticks );
			ErrorF("PIT Emulate: pittime = %08x = %d, last_ticks %08x = %d\n", pittime, pittime, last_ticks, last_ticks );
		}

		if( IOTRACE ) ErrorF("PIT Emulate: returning = %04x, timeval = %d\n", 0xffff & pittime, timeval );

		last_ticks = ticks;

		return 0;
	}

	if (pit_toggle)
	{
		/*
		 * Get the upper half
		 */
		pit_toggle = 0;
		r_al = (pittime >> 8) & 0xff;
		SET_AX();
		if( INTERP_DEBUG ) ErrorF("PIT Emulate: upper half: %02x, both halves: %04x\n", r_al, ((r_al << 8) | lower_byte) );
		if( IOTRACE ) ErrorF("inb(%x) = %x\n", port, r_al );
	}
	else
	{
		/*
		 * Get the lower half
		 */
		pit_toggle = 1;
		r_al = pittime & 0xff;
		SET_AX();
		if( INTERP_DEBUG ) ErrorF("PIT Emulate: lower half: %02x\n", r_al );
		if( IOTRACE ) ErrorF("inb(%x) = %x\n", port, r_al );
		lower_byte = r_al;
	}

	return 0;
}

/*													S001	vvv	
 * V86IOEnable() - enable a range of i/o addresses
 *	returns 0 for failure
 *	returns 1 or -1 for OK
 *	called from graf.c - from grafVBInit
 */
V86IOEnable(ioaddr, n)
    unsigned	ioaddr;
    int		n;
{

	if ( (unsigned char *) NULL == io_bitmap ) {
		io_bitmap = (unsigned char *) GrafMalloc ( (size_t) IO_BITMAP_SIZE );
    		if ( (unsigned char *) NULL == io_bitmap ) {
		ErrorF("V86IOEnable: Cannot malloc io_bitmap, length %#x\n", IO_BITMAP_SIZE );
			return (0);
		}
    		memset(io_bitmap, 0xff, IO_BITMAP_SIZE + 1);
	}


    if ((ioaddr + n) > MAX_IO_ADDR)
	return -1;

    while (--n >= 0)
    {
	int	index	= ioaddr >> 3;
	int	mask	= ~(1 << (ioaddr & 7));

	io_bitmap[index] &= mask;
	ioaddr++;
    }

    return 1;
}										/*	S001	^^^	*/
