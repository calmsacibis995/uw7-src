/*		copyright	"%c%" 	*/

#ident	"@(#)sum.c	1.2"
#ident  "$Header$"

#include "uucp.h"
#include <stdio.h>
#include <sys/types.h>
#include <crypt.h>

extern int Debug;

/* checksum algorithm used in uux and uuxqt to authenticate X. files */
/* checksum alrogithm borrowed from sum command (alternate algorithm */

unsigned
cksum(char *file)
{
	register unsigned sum = 0;
	FILE *f;
	int c;

	if ( (f = fopen(file, "r")) == NULL )
		return(0);

	while((c = getc(f)) != EOF) {
		if(sum & 01)
			sum = (sum >> 1) + 0x8000;
		else
			sum >>= 1;
		sum += c;
		sum &= 0xFFFF;
	}

	if (ferror(f))
		sum = 0;

	(void) fclose(f);

	return(sum);
}

