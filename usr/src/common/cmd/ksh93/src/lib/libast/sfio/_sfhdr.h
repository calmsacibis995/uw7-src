#ident	"@(#)ksh93:src/lib/libast/sfio/_sfhdr.h	1.1"
#include	"sfhdr.h"

/*	This header file defines analogues for all public macro functions.
	The analogues are used for building function analogues.
*/

#define _SFPUTC(f,c)	sfputc(f,c)
#define _SFGETC(f)	sfgetc(f)
#define _SFSLEN()	sfslen()
#define _SFFILENO(f)	sffileno(f)
#define _SFEOF(f)	sfeof(f)
#define _SFERROR(f)	sferror(f)
#define _SFCLRERR(f)	sfclrerr(f)
#define _SFSTACKED(f)	sfstacked(f)
#define _SFULEN(v)	sfulen(v)
#define _SFGETU(f)	sfgetu(f)
#define _SFGETL(f)	sfgetl(f)
#define _SFPUTU(f,v)	sfputu(f,v)
#define _SFPUTL(f,v)	sfputl(f,v)
#define _SFPUTD(f,v)	sfputd(f,v)
#define _SFECVT(v,n,d,s) sfecvt(v,n,d,s)
#define _SFFCVT(v,n,d,s) sffcvt(v,n,d,s)

#undef	sfputc
#undef	sfgetc
#undef	sfslen
#undef	sffileno
#undef	sfeof
#undef	sferror
#undef	sfclrerr
#undef	sfstacked
#undef	sfulen
#undef	sfgetu
#undef	sfgetl
#undef	sfputu
#undef	sfputl
#undef	sfputd
#undef	sfecvt
#undef	sffcvt
