#ifndef NOIDENT
#ident	"@(#)dtadmin:dtamlib/dtamgettxt.c	1.2"
#endif
/*
 *	Convenience routine to call gettxt for internationalized messages.
 *	The input string is of the form [file:#^A]default where the optional
 *	part is a msgid as expected by gettxt; if this is present, the input
 *	is split at the FS (^A) to provide the arguments to gettxt, else
 *	the whole string is returned as is; for historical reasons, this
 *	also accepts the form file:#:default from an earlier draft reqmt.
 */
#include <unistd.h>
#include <string.h>

#define	FS	'\001'

char
*DtamGetTxt(char *input)
{
	register  char  *ptr1;
	register  char  *ptr2;
	register  char	c;

	if ((ptr2 = strchr(input, FS)) == NULL) {
		if ((ptr1 = strchr(input,':')) == NULL)
			return input;
		if ((ptr2 = strchr(++ptr1,':')) == NULL)
			return input;
	}
	c = *ptr2;
	*ptr2 = '\0';
	ptr1 = gettxt(input, ptr2+1);
	*ptr2 = c;
	return ptr1;
}
