#ident	"@(#)acomp:common/inline.h	1.5"
/* inline.h */

#ifndef NO_AMIGO
extern int do_inline;
extern int delete_ok;
extern void inline_endf();
extern void inline_begf();
extern void inline_eof();
extern void inline_address_function();
extern void inline_flags();
#ifndef LINT
extern void set_module_at_a_time();
extern void set_parms_in_regs_flag();
#endif
#endif
#define ASSIGN_CASES \
	case ASG PLUS: \
	case ASG MINUS: \
	case ASG MUL: \
	case ASG DIV: \
	case ASG MOD: \
	case ASG OR: \
	case ASG AND: \
	case ASG ER: \
	case ASG LS: \
	case ASG RS: \
	case INCR: \
	case DECR: \
	case ASSIGN
