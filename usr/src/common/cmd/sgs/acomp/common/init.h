#ident "@(#)acomp:common/init.h	52.4"
/* init.h */

extern void in_decl();		/* declarator being initialized */
extern void in_start();		/* start of initializer */
extern void in_end();		/* end of initializer */
extern void in_lc();		/* saw { in initializer */
extern void in_rc();		/* saw } in initializer */
extern void in_init();		/* next initializer expression */
extern void in_chkval();	/* check whether initializer value fits */
