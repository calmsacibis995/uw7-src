#ident	"@(#)nas:i386/inst386.c	1.5"
/*
* i386/inst386.c - i386 assembler instructions lookup code
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "common/as.h"
#include "common/stmt.h"
#include "stmt386.h"

/* Pick up check/gen lists. */
#include "chkgen.c"

	/*
	* Current count: 498 instructions.  Leave approximately 1/3 of
	* the table empty when completely filled.  751 is the first
	* prime after 498*3/2.
	*/
static struct instree	/* search tree for instructions */
{
	const Inst386	inst;
	struct instree	*left;
	struct instree	*right;
	size_t		len;	/* length of instruction name */
} *buckets[751];
#ifdef DEBUG
   static size_t buckuse[sizeof(buckets) / sizeof(buckets[0])]; /* histogram */
#endif
#define HIGH5_BITS ((~(Ulong)0) & ~((~(Ulong)0) >> 5))	/* high 5 bit mask */
#define HIGH5_DIST (CHAR_BIT * sizeof(Ulong) - 5)	/* low order bits */

static struct instree instrs[] =	/* the machine's instructions */
{
#include "inst386.h"
};


#ifdef DEBUG
static void
#ifdef __STDC__
printinstree(const struct instree *itp)	/* print names in bucket */
#else
printinstree(itp)struct instree *itp;
#endif
{
	while (itp != 0)
	{
		(void)fprintf(stderr, " %s",
			(const char *)itp->inst.inst.inst_name);
		printinstree(itp->left);
		itp = itp->right;
	}
}
#endif

static struct instree *
#ifdef __STDC__
addinst(const Uchar *str, size_t len)	/* add insts until end or found */
#else
addinst(str, len)Uchar *str; size_t len;
#endif
{
	static struct instree *list = instrs;		/* next inst to add */
	static int ninst = sizeof(instrs) / sizeof(instrs[0]);
	register struct instree **prev, *p;
	register const Uchar *s;
	register Ulong hval;
	register size_t n;
	register struct instree *itp = list;
	int num = ninst;

#ifdef DEBUG
	if (DEBUG('I') > 1)
		(void)fprintf(stderr, "addinst(%s)\n", prtstr(str, len));
#endif
	while (--num >= 0)
	{
		s = itp->inst.inst.inst_name;
		if ((n = strlen((const char *)s)) == 0)
			fatal(gettxt(":936","addinst():zero length mnemonic"));
		itp->len = n;
		hval = *s;
		while (--n > 0)
		{
			register Ulong highbits;

			hval <<= 4;
			hval += *++s;
			if ((highbits = hval & HIGH5_BITS) != 0)
			{
				hval ^= highbits >> HIGH5_DIST;
				hval &= ~HIGH5_BITS;
			}
		}
		hval %= sizeof(buckets) / sizeof(buckets[0]);
#ifdef DEBUG
		buckuse[hval]++;
#endif
		s = itp->inst.inst.inst_name;
		n = itp->len;
		prev = &buckets[hval];
		while ((p = *prev) != 0)
		{
			register int i = strncmp((const char *)s,
				(const char *)p->inst.inst.inst_name, n);

			if (i == 0 && n == p->len)
			{
				fatal(gettxt(":937","addinst():identical mnemonics: %s"),
					(const char *)s);
			}
			if (i > 0)
				prev = &p->right;
			else
				prev = &p->left;
		}
		*prev = itp++;
		if (n == len
			&& strncmp((const char *)s, (const char *)str, n) == 0)
		{
			list = itp;
			ninst = num;
#ifdef DEBUG
			if (DEBUG('H') > 0)	/* output histogram so far */
			{
				register Ulong tot = 0;

				num = sizeof(buckets) / sizeof(buckets[0]);
				while (--num >= 0)
				{
					if (buckuse[num] == 0)
						continue;
					tot += buckuse[num];
					(void)fprintf(stderr,
						"inst-hash[%d] %lu:",
						num, (Ulong)buckuse[num]);
					printinstree(buckets[num]);
					(void)putc('\n', stderr);
				}
				(void)fprintf(stderr,
					"instructions so far: %lu\n", tot);
			}
#endif
			return itp - 1;
		}
	}
	ninst = 0;
	return 0;
}


const Inst *
#ifdef __STDC__
findinst(const Uchar *str, size_t len)	/* return matching instruction */
#else
findinst(str, len)Uchar *str; size_t len;
#endif
{
	register struct instree *itp;
	register const Uchar *s = str;
	register Ulong hval;
	register size_t n;

	/*
	* The average mnemonic for findinst() has 4.4 characters.
	* Unroll the initial portion of the hash calculation.  Since
	* there are at least 32 bits in a Ulong, even if there are
	* 10 bits in a Uchar, 5 Uchars can be combined without
	* reaching into the HIGH5_BITS mask.  Moreover, the 6th
	* Uchar can be combined without overflowing the Ulong.
	*/

	/* The table starts out empty.  When we fail to find a
	** mnemonic, we search linearly through the initial
	** instruction table (addinst()) until we find it, adding
	** instructions to the binary tree as we go.
	** The instructions in the initial table should be in order
	** of most frequently used to expendite searches.
	*/

	/*CONSTANTCONDITION*/
	if (CHAR_BIT * sizeof(Ulong) < 16 + CHAR_BIT + 5 + 1)
		fatal(gettxt(":938","findinst():hash calculation unroll invalid"));
	switch (n = len)
	{
	case 0:
		fatal(gettxt(":939","findinst():zero length mnemonic"));
		/*NOTREACHED*/
	default:
		hval = s[0] << 20;
		hval += s[1] << 16;
		hval += s[2] << 12;
		hval += s[3] << 8;
		hval += s[4] << 4;
		hval += s[5];
		s += 5;
		n -= 5;
		for (;;)	/* loop for long mnemonics */
		{
			register Ulong highbits;

			if ((highbits = hval & HIGH5_BITS) != 0)
			{
				hval ^= highbits >> HIGH5_DIST;
				hval &= ~HIGH5_BITS;
			}
			if (--n == 0)
				break;
			hval <<= 4;
			hval += *++s;
		}
		s = str;
		n = len;
		break;
	case 5:
		hval = s[0] << 16;
		hval += s[1] << 12;
		hval += s[2] << 8;
		hval += s[3] << 4;
		hval += s[4];
		break;
	case 4:
		hval = s[0] << 12;
		hval += s[1] << 8;
		hval += s[2] << 4;
		hval += s[3];
		break;
	case 3:
		hval = s[0] << 8;
		hval += s[1] << 4;
		hval += s[2];
		break;
	case 2:
		hval = s[0] << 4;
		hval += s[1];
		break;
	case 1:
		goto err;	/* no length 1 mnemonics, either */
	}
	hval %= sizeof(buckets) / sizeof(buckets[0]);
	itp = buckets[hval];
	while (itp != 0)
	{
		register int i = strncmp((const char *)s,
			(const char *)itp->inst.inst.inst_name, n);

		if (i == 0 && n == itp->len)
			return (Inst *) &itp->inst;
		if (i > 0)
			itp = itp->right;
		else
			itp = itp->left;
	}
	if ((itp = addinst(s, n)) != 0)
		return (Inst *) &itp->inst;
err:;
	error(gettxt(":940","unknown instruction: %s"), prtstr(s, n));
	return 0;
}
