#ident	"@(#)optim:i386/optutil.h	1.1.4.17"
#ifndef OPTUTIL_H
#define OPTUTIL_H
#include "optim.h"
/* optutil.h
**
**	Declarations for optimizer utilities
**/

/* isreg expression is true if the operand is a register */
#define isreg(s) ((s) != NULL && *(s) == '%')
#define isconst(s) ((s) != NULL && *(s) == '$')
#define isimmed(operand) ((operand) && *(operand) == '$' && \
		(isdigit(operand[1]) || operand[1] == '-' && isdigit(operand[2]))) 
#define is_logic(p)	((p->op >= ANDB) && (p->op <= XORL))
#define hasdisplacement(op) ((op) && (*op) != '$' && (*op) != '%'\
								  && (*op) != '(' )
extern int isregal();

#define ismem(op)	(((op) && (*op) && (*op) != '%' && (*op) != '$' && \
					!(op[1] == 'F' && op[2] == '#')) ? MEM : 0)
#define cp_vol_info(P1,P2)	(P2)->userdata = (P1)->userdata
#define is_move(p) ((p)->op == MOVL || (p)->op == MOVW || (p)->op == MOVB)
/* operand is regal if it's like X(%ebp) and isregal(X) */
#define ISREGAL(OP) ((scanreg((OP),0)&frame_reg) && isregal(OP))
#define Ismov(p)        ((p->op==MOVB) || (p->op==MOVL))
#define Isimull(p)      (p->op==IMULB || p->op==IMULL)
#define IS_FIX_PUSH_REG(p,reg)  ((p->save_restore == SAVE) && (samereg(p->op1,(reg))))
#define IS_FIX_POP_REG(p,reg) ((p->save_restore == RESTORE) && (samereg(p->op2,(reg))))
#define IS_FIX_PUSH(p)   (p->save_restore == SAVE)
#define IS_FIX_POP(p)    (p->save_restore == RESTORE)

#define DPRTEXT(s) dprtext(s,NULL,-1,0)
extern void br2opposite();
extern boolean cmp_imm_imm();
extern NODE *non_label_after();
extern int inc_numauto();
extern void replace_registers();
extern void unsign2sign();
extern void find_first_and_last_fps();
extern boolean live_at();
extern boolean is_any_farit();
extern void rm_dead_insts();
extern void trace_optim();
extern void loop_detestl();
extern void rm_all_tvrs();
extern void remove_overlapping_regals();
extern unsigned int hard_sets();

extern BLOCK *block_of();
extern NODE *first_non_label();
extern SWITCH_TBL * get_base_label();
extern boolean is_unsable_br();
extern void sign2unsign();
extern NODE * insert();		/* insert new instruction node */
extern void chgop();		/* change op code */
extern NODE *duplicate();	/* make a copy of a NODE after another node */
extern void invbr();		/* inverse brach op */
#if 0 /* for new setcond */
extern void br2set();       /* inverse branch op to setcc op */
extern char *sreg2b();      /* return byte reg of scratch reg        */
#endif
extern void br2nset();      /* inverse branch op to oposite setcc op */
extern void move_node();    /* drags node throw the body to any place */
extern void swap_nodes();   /* swap two nodes */
extern NODE *find_label();  /* finds the target of the branch */
extern boolean isdyadic();	/* is instruction dyadic? */
extern boolean is2dyadic();	/* is instruction dyadic? */
extern boolean is_fp_arit();	/* is floating point arithmetic instruction? */
extern boolean istriadic();	/* is instruction triadic? */
extern boolean isfp();		/* is floating point instruction? */
extern boolean is_byte_instr();
extern boolean is_read_only();
extern boolean is_legal_op1();  /* check consistency of size of 1st operand */
extern void makelive();		/* make register live */
extern void makedead();		/* make register dead */
extern boolean isindex();	/* operand is an index off a register */
extern boolean ismove();	/* is instruction a move-type? */
extern boolean changed();   /*(p0,p1,regs) return true if regs are notchanged 
							  from p0 to p1 */	  
extern void set_save_restore();
extern boolean iszoffset();	/* is operand 0 offset from register? */
extern boolean isnumlit();	/* is operand a numeric literal: &[-]n? */
extern boolean isiros();    /* is operand immediate, register or stack*/
extern void exchange();		/* exchange node and successor */
extern boolean undefinedCC (NODE *);	/* does instr undefine any CC flags */
extern boolean usesvar();	/* does first operand use second? */
extern boolean isvolatile();	/* is operand volatile? */
extern boolean has_scale(); /*does indexing contain scaling? */
extern void ldelin();	
extern void ldelin2();	/* routines to preserve */
extern void lexchin();
extern void lmrgin1();	/* line number info */
/*extern void lmrgin2();*/
extern void lmrgin3();
extern void revbr();	/* reverse sense of a jump node */
extern char * dst();		/* return pointer to destination string */
extern boolean samereg();	/* return true if same register */
extern boolean usesreg();	/* usesreg(a,b) true if reg. b used in a */
extern unsigned int uses();		/* return register use bits */
extern unsigned int scanreg();	/* determine registers referenced in operand */
extern unsigned int sets();	/* return set register destination bits */
extern unsigned int uses_but_not_indexing();
extern boolean isdead();	/* true iff *cp is dead after p */
extern char * getp();		/* return pointer to jump destination operand */
extern char * newlab();		/* generate a new label */
extern void prinst();		/* print instruction */
extern void dprinst();		/* print instruction */
extern void fprinst();		/* print instruction */
extern boolean ishlp();		/* return true if a fixed label present */
extern unsigned int setreg();		/* return bits corresponding to register */
extern boolean isdead();	/* return true if reg. dead after node */
extern boolean change_by_const();
extern int get_free_reg();
extern boolean w1opt();
extern boolean w2opt();
extern boolean w3opt();
extern boolean w4opt();
extern boolean w5opt();
extern boolean w6opt();


extern unsigned int hard_uses();
extern int is_jmp_ind();
extern int find_pointer_write();
extern char *w2l();
extern char *w2h();
extern int pairable();
extern NODE *find_label();
#ifdef DEBUG
extern void prdag(),prnode();
#endif
struct RI 	{ unsigned int reg_number;
			      char * regname[3];
     		     };         /* Register indexes   */

/* declare functions that are always handy for optimizations */



/* declare functions defined in optim.c that are handy here */
#ifdef DEBUG
extern void wchange();		/* flag a window change */
#else
#define wchange()		/* nothing */
#endif

#endif


extern const char frame_string[];
extern const char regs_string[];
