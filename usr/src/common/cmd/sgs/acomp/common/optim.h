#ident	"@(#)acomp:common/optim.h	52.8"
/* optim.h */

/* Declarations for tree-opimization routines. */

extern ND1 * op_optim();
extern void op_numinit();
extern ND1 * op_init();
extern ND1 * op_amigo_optim();
extern FP_LDOUBLE op_xtofp();
extern FP_LDOUBLE op_xtodp();


/* Prefix to recognize for special built-in functions.
** All such functions are presumed to begin with the
** same prefix.
*/
#ifndef	BUILTIN_PREFIX
#define	BUILTIN_PREFIX "__std_hdr_"
#endif
