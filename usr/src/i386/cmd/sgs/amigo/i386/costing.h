#ident	"@(#)amigo:i386/costing.h	1.2.1.17"
/* Machine dependent costing information. */

	/*
		The larger the threshold, the less likely we are to find
		that a given optimization is worthwhile.
	*/

#define LOAD_COST(type) (TY_ISFPTYPE(type) ? 30 : 3)
#define STORE_COST(type) (TY_ISFPTYPE(type) ? 30 : 3)
#define ARITH_COST 2

#ifndef CM_THRESHOLD /* code motion */
#define CM_THRESHOLD 1
#endif

#ifndef SR_THRESHOLD /* strength reduction */
#define SR_THRESHOLD ARITH_COST + 6
#endif

#ifndef PASS_THRU_THRESHOLD /* Max use of variable in a "pass thru" */
#define PASS_THRU_THRESHOLD 2
#endif

#ifndef CF_THRESHOLD /* constant folding */
#define CF_THRESHOLD 0
#endif

#ifndef CSE_THRESHOLD /* local cse */
#define CSE_THRESHOLD 0
#endif
/* change above THRESH to take datatypes */

#ifndef LU_THRESHOLD /* loop unrolling */
#define LU_THRESHOLD 5
#endif

#ifndef LU_L_SIZE_THRESHOLD /* Max nd1 count for loop unrolling */
#define LU_L_SIZE_THRESHOLD 8
#endif

#ifndef EXP_THRESHOLD /* additional cost above load for EXPENSIVE flag */
#define EXP_THRESHOLD 0
#endif

extern int cost();

extern cm_threshold;
extern cf_threshold;
extern cse_threshold;
extern pass_thru_threshold;
extern sr_threshold;
extern exp_threshold;
extern lu_threshold, lu_l_size_threshold, lu_f_call_penalty;
extern int reg_slope, reg_height;

#define COST(node) \
	((node)->opt->cost < 0 ? \
		(node)->opt->cost = cost(node) \
	      : (node)->opt->cost)


#define IN_REG(p) 1

/* we assume the registers are grouped into disjoint sets called classes */
/* returns the class given the sid ,s */
#define REGCLASS(s) (TY_ISINTTYPE(SY_TYPE(s)) || TY_ISPTR(SY_TYPE(s)) ? 0 : -1)

/* can we find a free register in "class" assuming "cnt" registers are used
   regargless of what those registers are
*/
#define is_free_register(class,cnt) (picflag ? cnt < 2 : cnt < 3)

#define CSE_IS_OK(node)	\
	(	((node)->op != PLUS && (node)->op != LS) \
	     || TY_ISFPTYPE((node)->type) \
	)
