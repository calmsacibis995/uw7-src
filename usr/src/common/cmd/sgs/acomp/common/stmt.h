#ident	"@(#)acomp:common/stmt.h	55.3"
/* stmt.h */

/* Declare functions that are used by statement processing. */

extern void sm_lc();
extern void sm_rc();
extern void sm_deflab();
extern void sm_goto();
extern void sm_expr();
extern int sm_genlab();
extern void sm_begf();
extern void sm_endf();
extern void sm_if_start();
extern void sm_if_end();
extern void sm_if_else();
extern void sm_wh_init();
extern void sm_wh_start();
extern void sm_wh_end();
extern void sm_for_init();
extern void sm_for_control();
extern void sm_for_iter();
extern void sm_for_end();
extern void sm_do_start();
extern void sm_do_end();
extern void sm_sw_start();
extern void sm_sw_case();
extern void sm_sw_end();
extern void sm_break();
extern void sm_default();
extern void sm_continue();
extern void sm_return();


#ifdef FAT_ACOMP
extern int sm_g_weight();
#ifndef SM_WT_USE
#define SM_WT_USE(old, cur) ((old)+(cur))
#endif
/* Register declaration is worth something! */
#ifndef SM_WT_REGDECL
#define SM_WT_REGDECL 2
#endif

/* Initial weight for variables in statements.  Top-level "if"
** can produce a smaller value.
*/
#ifndef SM_WT_INITVAL
#define SM_WT_INITVAL 2
#endif

/* It costs one move to get a parameter into a register. It costs one
** move for a global/static, plus it must be restored.
*/
#ifndef SM_WT_INIT
#define SM_WT_PENALTY(c)	\
	(c==SC_AUTO ? 0 : (c==SC_PARAM ? -SM_WT_INITVAL : -(2*SM_WT_INITVAL)))
#define SM_WT_INIT(sid) (SY_WEIGHT(sid) = \
	(SM_WT_PENALTY(SY_CLASS(sid))  +  \
	((SY_FLAGS(sid) & SY_ISREG) ? SM_WT_REGDECL : 0)))
#endif

/* Support for adjusting weights in statements. */

#ifndef SM_WT_LOOPWT
#define SM_WT_LOOPWT 10
#endif

#ifndef SM_WT_BOUND
#define SM_WT_BOUND 10000
#endif

/* New weight for for loops and other statements, based on current weight. */
#ifndef SM_WT_FOR
#define SM_WT_FOR(cur) (cur > SM_WT_BOUND ? cur : (cur*SM_WT_LOOPWT))
#endif

#ifndef SM_WT_WHILE
#define SM_WT_WHILE(cur) (cur > SM_WT_BOUND ? cur : (cur*SM_WT_LOOPWT))
#endif

#ifndef SM_WT_DO
#define SM_WT_DO(cur) (cur > SM_WT_BOUND ? cur : (cur*SM_WT_LOOPWT))
#endif

#ifndef SM_WT_IF
#define SM_WT_IF(cur) ((cur+1)/2)
#endif

#ifndef SM_WT_SWITCH
#define SM_WT_SWITCH(cur) ((cur+1)/2)
#endif

/* Minimum register-able weight. */
#ifndef SM_WT_THRESHOLD
#define SM_WT_THRESHOLD 8
#endif

#endif	/* def FAT_ACOMP */

#ifdef LINT
extern void sm_lnrch();
#endif

/* These definitions describe how code is to be generated
** for loops.
*/

extern int sm_while_loop_code;		/* Code style for while loops */
extern int sm_for_loop_code;		/* Code style for for loops */

#define	LL_TOP	0		/* Generate tests at top of loop. */
#define	LL_BOT	1		/* Generate tests at bottom of loop. */
#define	LL_DUP	2		/* Duplicate tests at top and bottom. */
