#ident	"@(#)ksh93:src/lib/libast/comp/strtod.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_strtod

NoN(strtod)

#else

#include <ctype.h>

extern double	atof(const char*);

double
strtod(register const char* s, char** e)
{
	double	n;

	n = atof(s);
	if (e)
	{
		while (isspace(*s)) s++;
		if (*s == '-' || *s == '+') s++;
		while (isdigit(*s)) s++;
		if (*s == '.') while (isdigit(*++s));
		if (*s == 'e' || *s == 'E')
		{
			if (*++s == '-' || *s == '+') s++;
			while (isdigit(*s)) s++;
		}
		*e = (char*)s;
	}
	return(n);
}

#endif
