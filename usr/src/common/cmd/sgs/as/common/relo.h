#ident	"@(#)nas:common/relo.h	1.2"
/*
* common/relo.h - common assembler interface to relocation
*
* Depends on:
*	"common/as.h"
*/

#ifdef __STDC__
void	relocexpr(Eval *, const Code *, Section *);
#else
void	relocexpr();
#endif
