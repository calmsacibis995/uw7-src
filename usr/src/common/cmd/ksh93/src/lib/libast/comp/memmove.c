#ident	"@(#)ksh93:src/lib/libast/comp/memmove.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_memmove

NoN(memmove)

#else

void*
memmove(void* to, const void* from, register size_t n)
{
	register char*	out = (char*)to;
	register char*	in = (char*)from;

	if (n <= 0)	/* works if size_t is signed or not */
		;
	else if (in + n <= out || out + n <= in)
		return(memcpy(to, from, n));	/* hope it's fast*/
	else if (out < in)
		do *out++ = *in++; while (--n > 0);
	else
	{
		out += n;
		in += n;
		do *--out = *--in; while(--n > 0);
	}
	return(to);
}

#endif
