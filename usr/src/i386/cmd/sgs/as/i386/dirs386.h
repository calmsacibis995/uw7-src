#ident	"@(#)nas:i386/dirs386.h	1.1"
/*
* i386/dirs386.h - i386 assembler directives header
*
* Depends on:
*	"common/as.h"
*/

#ifdef __STDC__
void	directive386(const Uchar *, size_t, Oplist *);
#else
void	directive386();
#endif
