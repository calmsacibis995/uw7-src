#ident	"@(#)lprof:cmd/merge.c	1.7.3.5"
/*
* merge.c - merging operations; files or line numbers
*/
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "lprof.h"

void
mergefiles(void) /* merge two or more cnt files into a new cnt file */
{
	unsigned long n, okay;
	int fd;

	/*
	* _CAcntmerge() requires that both files exist,
	* so first we create an empty destination.
	*/
	if ((fd = open(args.cntf, O_CREAT, 0666)) < 0)
	{
		error(":1750:Unable to create merge output file %s\n",
			args.cntf);
		exit(2);
	}
	close(fd);
	okay = 0;
	for (n = 0; n < args.mrgs.nused; n++)
	{
		if (_CAcntmerge(args.cntf, args.mrgs.list[n],
			args.option & OPT_TIMESTAMP) == 0)
		{
			okay++;
			continue;
		}
		error(":1751:Merging ignores cnt file %s\n", args.mrgs.list[n]);
	}
	if (okay == args.mrgs.nused) /* complete success */
		exit(0);
	if (okay == 0)
		remove(args.cntf); /* nevermind: no merging done */
	exit(1);
}

void
mergecnts(struct linenumb **dst, unsigned long *dlen,
	struct linenumb *src, unsigned long slen)
{
	struct linenumb *lnp, *p1, *p2, *orig;
	unsigned long nline, n1, n2, n;

	p1 = *dst;
	n1 = *dlen;
	p2 = src;
	n2 = slen;
	n = n1;
	/*
	* In the first pass look for line numbers in src's list
	* that are not present in dst's.  If there are any, grow
	* the list.
	*/
	for (;;)
	{
		if (n1 == 0 || n2 == 0)
		{
			n += n2; /* include any extras from src's list */
			break;
		}
		if (p1->num < p2->num) /* only in dst's list */
		{
			n1--;
			p1++;
		}
		else if (p1->num > p2->num)
		{
			n++; /* extra line number from src's list */
			n2--;
			p2++;
		}
		else /* in both lists */
		{
			n1--;
			n2--;
			p1++;
			p2++;
		}
	}
	p1 = *dst;
	n1 = *dlen;
	p2 = src;
	n2 = slen;
	if (n != n1) /* some new line numbers from src's list */
	{
		orig = p1;
		lnp = alloc(n * sizeof(struct linenumb));
		*dst = lnp;
		*dlen = n;
	}
	else
	{
		orig = 0;
		lnp = p1;
	}
	/*
	* In the second pass, create the union of the two
	* lists, where if both lists have the same line number,
	* the result list holds the sum of their counts.
	*/
	do {
		if (n1 == 0) /* n2 != 0 */
		{
		copy2:;
			lnp->num = p2->num;
			lnp->cnt = p2->cnt;
			n2--;
			p2++;
		}
		else if (n2 == 0)
		{
		copy1:;
			lnp->num = p1->num;
			lnp->cnt = p1->cnt;
			n1--;
			p1++;
		}
		else if (p1->num < p2->num)
			goto copy1;
		else if (p1->num > p2->num)
			goto copy2;
		else
		{
			lnp->num = p1->num;
			lnp->cnt = p1->cnt + p2->cnt;
			n1--;
			n2--;
			p1++;
			p2++;
		}
	} while (++lnp, --n != 0);
	/*
	* No longer need the src list nor the original *dst,
	* if we've allocated a new one.
	*/
	free(src);
	if (orig != 0)
		free(orig);
}
