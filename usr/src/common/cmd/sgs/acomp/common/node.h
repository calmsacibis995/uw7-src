#ident	"@(#)acomp:common/node.h	52.2"
/* node.h */

/* Definitions for node handling routines. */

extern NODE * talloc();
extern void tfree();
extern int tcheck();

#define t1alloc() ((ND1 *) talloc())
#define t1free(p)  (tfree((NODE *) (p)))

#define	ND_NOSYMBOL SY_NOSYM	/* set if ICON has no symbol associated. */
