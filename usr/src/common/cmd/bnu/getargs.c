/*		copyright	"%c%" 	*/

#ident	"@(#)getargs.c	1.2"
#ident  "$Header$"

#include "uucp.h"

/*
 * generate a vector of pointers (arps) to the
 * substrings in string "s".
 * Each substring is separated by blanks and/or tabs.
 *	s	-> string to analyze -- s GETS MODIFIED
 *	arps	-> array of pointers -- count + 1 pointers
 *	count	-> max number of fields
 * returns:
 *	i	-> # of subfields
 *	arps[i] = NULL
 */

int
getargs(s, arps, count)
register char *s, *arps[];
register int count;
{
	register int i = 0;
	int slen;

	char **listp, **strtoargv(char *);

	/* strip off trailing newline if it is still there */

	slen = strlen(s);
	if ((slen > 0) && (s[slen-1] == '\n'))
		s[slen-1] = '\0';

	/* dynamically create a list of arbitrary size */

	listp = strtoargv(s);

	if (listp) {
		/* only send back as many as there is room for */

		while ( i<count ) {
			if (listp[i] == NULL)
				break;
			arps[i] = listp[i];
			i++;
		}
		free(listp);
	}
	while( i<count) 
		arps[i++] = NULL;
	return(i);
}

/*
 *      bsfix(args) - remove backslashes from args
 *
 *      \123 style strings are collapsed into a single character
 *	\000 gets mapped into \N for further processing downline.
 *      \ at end of string is removed
 *	\t gets replaced by a tab
 *	\n gets replaced by a newline
 *	\r gets replaced by a carriage return
 *	\b gets replaced by a backspace
 *	\s gets replaced by a blank 
 *	any other unknown \ sequence is left intact for further processing
 *	downline.
 */

void
bsfix (args)
char **args;
{
	register char *str, *to, *cp;
	register int num;

	for (; *args; args++) {
		str = *args;
		for (to = str; *str; str++) {
			if (*str == '\\') {
				if (str[1] == '\0')
					break;
				switch (*++str) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					for ( num = 0, cp = str
					    ; cp - str < 3
					    ; cp++
					    ) {
						if ('0' <= *cp && *cp <= '7') {
							num <<= 3;
							num += *cp - '0';
						}
						else
						    break;
					}
					if (num == 0) {
						*to++ = '\\';
						*to++ = 'N';
					} else
						*to++ = (char) num;
					str = cp-1;
					break;

				case 't':
					*to++ = '\t';
					break;

				case 's':	
					*to++ = ' ';
					break;

				case 'n':
					*to++ = '\n';
					break;

				case 'r':
					*to++ = '\r';
					break;

				case 'b':
					*to++ = '\b';
					break;

				default:
					*to++ = '\\';
					*to++ = *str;
					break;
				}
			}
			else
				*to++ = *str;
		}
		*to = '\0';
	}
	return;
}
