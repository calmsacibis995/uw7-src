#ident "@(#)decodenetnum.c	1.2"

/*
 * decodenetnum - return a net number (this is crude, but careful)
 */
#include <sys/types.h>
#include <ctype.h>
#ifndef SYS_WINNT
#include <sys/socket.h>
#include <netinet/in.h>
#endif /* SYS_WINNT */

#include "ntp_stdlib.h"

int
decodenetnum(num, netnum)
	const char *num;
	u_int32 *netnum;
{
	register const char *cp;
	register char *bp;
	register int i;
	register int temp;
	register int eos;
	char buf[80];		/* will core dump on really stupid stuff */

	cp = num;
	*netnum = 0;

	if (*cp == '[') {
		eos = ']';
		cp++;
	} else {
		eos = '\0';
	}

	for (i = 0; i < 4; i++) {
		bp = buf;
		while (isdigit(*cp))
			*bp++ = *cp++;
		if (bp == buf)
			break;

		if (i < 3) {
			if (*cp++ != '.')
				break;
		} else if (*cp != eos)
			break;

		*bp = '\0';
		temp = atoi(buf);
		if (temp > 255)
			break;
		*netnum <<= 8;
		*netnum += temp;
	}
	
	if (i < 4)
		return 0;
	*netnum = htonl(*netnum);
	return 1;
}
