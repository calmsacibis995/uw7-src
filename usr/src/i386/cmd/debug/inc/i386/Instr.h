#ifndef	Instr_h
#define	Instr_h
#ident	"@(#)debugger:inc/i386/Instr.h	1.10"

#include	"Iaddr.h"
#include	"Reg.h"
#include	<stdio.h>



class	ProcObj;
struct	instable;
struct	instr_data;
struct	Operand;

class Instr {
	ProcObj		*pobj;
	unsigned char	save_bytes[BUFSIZ];	// store instruction
	Iaddr		loaddr;		// boundaries of byte buffer
	Iaddr		hiaddr;
	unsigned char	get_opcode(unsigned *, unsigned *);
	void 		imm_data(int no_bytes, Operand *);
	void		get_modrm_byte(unsigned *, unsigned *, unsigned *);
	void		check_override(Operand *);
	void		get_operand(unsigned mode, unsigned r_m, int wbit,
				Operand *);
	int		get_text( Iaddr, unsigned char & ); 
				// get 1 byte of text from process
	int		get_text_nobkpt( Iaddr );
			 // get text from process, with no breakpoints
	const instable	*get_instr(instr_data *);
	int		get_operands(Iaddr, int adr_mode, instr_data *, int &sz);
	int		skip_jump_table(Iaddr);
	int		instr_size(Iaddr);
public:

			Instr( ProcObj *);
			~Instr() {};
	Iaddr		retaddr( Iaddr ); // return address if call instr
	int		is_bkpt( Iaddr ); // breakpoint instruction?
	char		*deasm( Iaddr, int &inst_size, int symbolic,
				const char *name, Iaddr offset);
				// assembly language instructions
	Iaddr		adjust_pc(int force = 0);// adjust pc after breakpoint
	int		nargbytes( Iaddr );// number of argument bytes
	Iaddr		fcn_prolog( Iaddr pc, int &size, Iaddr &start,
				Iaddr *save_regs);
				// function prolog with saved registers
	Iaddr           brtbl2fcn( Iaddr ); // branch table to funciton
	Iaddr           fcn2brtbl( Iaddr, int offset ); 
				// function to branch table
	int		iscall( Iaddr caller, Iaddr callee );	
				// is previous instruction CALL to current 
				// func?
	int		isreturn( Iaddr );	// is next instruction return
	Iaddr		jmp_target( Iaddr );	// target addr if JMP
	int		find_return(Iaddr pc, Iaddr &esp, Iaddr &ebp);
	int		call_size(Iaddr);
	void		set_text(Iaddr, const char *);
#if PTRACE
	Iaddr		find_lreturn(Iaddr);
#endif
};
#endif
