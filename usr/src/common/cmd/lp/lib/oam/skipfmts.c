/*		copyright	"%c%" 	*/

#ident	"@(#)skipfmts.c	1.2"
#ident	"$Header$"
/* LINTLIBRARY */

#include "stdio.h"
#include "string.h"
#include "oam.h"

stva_list
#if	defined(__STDC__)
skipfmts(char *fmt, va_list args)
#else
skipfmts(fmt, args)
char	*fmt;
va_list args;
#endif
{
	static char digits[] = "01234567890", skips[] = "# +-.0123456789hL$";

	enum types {INT = 1, LONG, CHAR_PTR, DOUBLE, LONG_DOUBLE, VOID_PTR,
		LONG_PTR, INT_PTR};
	enum types typelst[MAXARGS], curtype;
	int maxnum, n, curargno, flags;

	/*
	* Algorithm	1. set all argument types to zero.
	*		2. walk through fmt putting arg types in typelst[].
	*		3. walk through args using va_arg(args, typelst[n])
	*			to advance list pointer
	* Assumptions:	Cannot use %*$... to specify variable position.
	*/

	(void)memset((void *)typelst, 0, sizeof(typelst));
	maxnum = -1;
	curargno = 0;
	while ((fmt = strchr(fmt, '%')) != 0)
	{
		fmt++;	/* skip % */
		if (fmt[n = strspn(fmt, digits)] == '$')
		{
			curargno = atoi(fmt) - 1;	/* convert to zero base */
			if (curargno < 0)
				continue;
			fmt += n + 1;
		}
		flags = 0;
	again:;
		fmt += strspn(fmt, skips);
		switch (*fmt++)
		{
		case '%':	/*there is no argument! */
			continue;
		case 'l':
			flags |= 0x1;
			goto again;
		case '*':	/* int argument used for value */
			/* check if there is a positional parameter */
			if (isdigit(*fmt)) {
				int	targno;
				targno = atoi(fmt) - 1;
				fmt += strspn(fmt, digits);
				if (*fmt == '$')
					fmt++; /* skip '$' */
				if (targno >= 0 && targno < MAXARGS) {
					typelst[targno] = INT;
					if (maxnum < targno)
						maxnum = targno;
				}
				goto again;
			}
			flags |= 0x2;
			curtype = INT;
			break;
		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G':
			curtype = DOUBLE;
			break;
		case 's':
			curtype = CHAR_PTR;
			break;
		case 'p':
			curtype = VOID_PTR;
			break;
		case 'n':
			if (flags & 0x1)
				curtype = LONG_PTR;
			else
				curtype = INT_PTR;
			break;
		default:
			if (flags & 0x1)
				curtype = LONG;
			else
				curtype = INT;
			break;
		}
		if (curargno >= 0 && curargno < MAXARGS)
		{
			typelst[curargno] = curtype;
			if (maxnum < curargno)
				maxnum = curargno;
		}
		curargno++;	/* default to next in list */
		if (flags & 0x2)	/* took care of *, keep going */
		{
			flags ^= 0x2;
			goto again;
		}
	}
	for (n = 0 ; n <= maxnum; n++)
	{
		if (typelst[n] == 0)
			typelst[n] = INT;
		
		switch (typelst[n])
		{
		case INT:
			(void) va_arg(args, int);
			break;
		case LONG:
			(void) va_arg(args, long);
			break;
		case CHAR_PTR:
			(void) va_arg(args, char *);
			break;
		case DOUBLE:
			(void) va_arg(args, double);
			break;
		case LONG_DOUBLE:
			(void) va_arg(args, double);
			break;
		case VOID_PTR:
			(void) va_arg(args, void *);
			break;
		case LONG_PTR:
			(void) va_arg(args, long *);
			break;
		case INT_PTR:
			(void) va_arg(args, int *);
			break;
		}
	}
	return (*(struct stva_list *) &args);
}
