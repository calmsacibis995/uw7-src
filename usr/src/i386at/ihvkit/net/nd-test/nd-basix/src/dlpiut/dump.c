/*
 * File dump.c
 *
 *      Copyright (C) The Santa Cruz Operation, 1994-1995.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#pragma comment(exestr, "@(#) dump.c 11.1 95/05/01 SCOINC")

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "dlpiut.h"

static int	chtran(int c);

void
dump(uchar *addr, int len)
{
	static char		buf[80];
	register uchar	*ip;
	register char	*cp;
	int				i;
	int				j;
	int				k;
	int				index;

	i = len;
	index = 0;
	while (i > 0) {
		cp = buf;
		ip = addr;
		sprintf(cp, "%04d  ", index);
		cp += 6;
		k = i;
		for (j = 8; j--; ) {
				if (i-- > 0)
					sprintf(cp, "%02x", *ip++);
				else
					strcpy(cp, "  ");
				cp += 2;
				if (i-- > 0)
					sprintf(cp, "%02x ", *ip++);
				else
					strcpy(cp, "   ");
				cp += 3;
		}
		strcpy(cp, "    ");
		cp += 4;
		ip = addr;
		i = k;
		for (j = 16; j--; ) {
			if (i-- > 0)
				sprintf(cp, "%c", chtran(*ip++));
			else
				*cp = ' ';
			cp++;
		}
		*cp = 0;
		errlogprf(buf);
		addr += 16;
		index += 16;
	}
}

static int
chtran(int c)
{
	if (c < ' ')
		return('.');

	if (c > '~')
		return('.');

	return(c);
}

