#ident	"@(#)lprof:cmd/report.c	1.4"
/*
* report.c - routines that generate the reports.
*/
#include <pfmt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lprof.h"

static const char sumfmt[]	= "%6.2f %7ld %7ld\t";
static const char nolinefmt[]	= "\t\t%s";
static const char linefmt[]	= "%7ld [%ld]\t%s";
static const char zerofmt[]	= "  [U] [%ld]\t%s";
static const char nonzerofmt[]	= "      [%ld]\t%s";
static const char cntsfmt[]	= "\t%ld %ld\n";
static const char ininfrom[]	= ":1755:inlined in %s from %s\n";
static const char *between	= "";

static char initbuf[512];
static char *inbuf = initbuf;
static size_t buflen = sizeof(initbuf);
static unsigned long lineno;
static FILE *infp;

static unsigned long tlins;
static unsigned long tcnts;

static void
sumsrc(struct srcfile *sfp)
{
	unsigned long i, num, nlins, ncnts;
	struct linenumb *lnp;
	struct function *fp;

	for (fp = sfp->func; fp != 0; fp = fp->next)
	{
		nlins = 0;
		ncnts = 0;
		lnp = fp->line;
		num = 0;
		for (i = 0; i < fp->nlnm; lnp++, i++)
		{
			if (lnp->num == num)
				continue;
			num = lnp->num;
			nlins++;
			if (lnp->cnt != 0)
				ncnts++;
		}
		tlins += nlins;
		tcnts += ncnts;
		if (nlins == 0)
			fatal(":1754:function present without line numbers\n");
		printf(sumfmt, (100.0 * ncnts) / nlins, ncnts, nlins);
		if (fp->rawp == 0)
		{
			pfmt(stdout, MM_NOSTD, ininfrom,
				fp->name, fp->srcf->path);
		}
		else
			printf("%s\n", fp->name);
	}
	if ((args.option & OPT_SEPARATE) == 0)
	{
		for (fp = sfp->incl; fp != 0; fp = fp->incl)
		{
			if (fp->name == 0)
				continue;
			nlins = 0;
			ncnts = 0;
			lnp = fp->line;
			num = 0;
			for (i = 0; i < fp->nlnm; lnp++, i++)
			{
				if (lnp->num == num)
					continue;
				num = lnp->num;
				nlins++;
				if (lnp->cnt != 0)
					ncnts++;
			}
			tlins += nlins;
			tcnts += ncnts;
			if (nlins == 0)
				fatal(":1756:fragment without line numbers\n");
			printf(sumfmt, (100.0 * ncnts) / nlins, ncnts, nlins);
			if (fp->rawp == 0)
			{
				pfmt(stdout, MM_NOSTD, ininfrom,
					fp->name, fp->srcf->path);
			}
			else
			{
				pfmt(stdout, MM_NOSTD,
					":1757:%s in %s from %s\n",
					fp->name, sfp->path, fp->srcf->path);
			}
		}
	}
}

void
summary(void)
{
	struct srcfile *sfp;
	char timestr[128];
	struct stat st;

	if (stat(args.cntf, &st) != 0)
		error(":1684:unable to get status for %s\n", args.cntf);
	else
	{
		pfmt(stdout, MM_NOSTD,
			":92:Coverage Data Source: %s\n", args.cntf);
		if (strftime(timestr, sizeof(timestr), "%c",
			localtime(&st.st_mtime)) != 0)
		{
			pfmt(stdout, MM_NOSTD,
				":1758:Date of Coverage Data Source: %s\n",
				timestr);
		}
	}
	pfmt(stdout, MM_NOSTD, ":1282:Object: %s\n\n", args.prog);
	pfmt(stdout, MM_NOSTD, ":1283:percent   lines   total\tfunction\n");
	pfmt(stdout, MM_NOSTD, ":1284:covered  covered  lines\tname\n\n");
	for (sfp = unit; sfp != 0; sfp = sfp->next)
		sumsrc(sfp);
	if (args.option & OPT_SEPARATE)
	{
		for (sfp = incl; sfp != 0; sfp = sfp->next)
			sumsrc(sfp);
	}
	putchar('\n');
	printf(sumfmt, (100.0 * tcnts) / tlins, tcnts, tlins);
	pfmt(stdout, MM_NOSTD, ":1759:TOTAL\n");
}

static int
nextline(void)
{
	char *p;

	lineno++;
	inbuf[buflen - 1] = 'x';
	if (fgets(inbuf, buflen, infp) == 0)
		return EOF;
	while (inbuf[buflen - 1] == '\0') /* line possibly too long */
	{
		if (inbuf[buflen - 2] == '\n') /* it just fit */
			break;
		p = alloc(buflen + sizeof(initbuf));
		memcpy(p, inbuf, buflen);
		if (inbuf != initbuf)
			free(inbuf);
		inbuf = p;
		p += buflen - 1;
		buflen += sizeof(initbuf);
		inbuf[buflen - 1] = 'x';
		if (fgets(p, sizeof(initbuf) + 1, infp) == 0) {
			p[0] = '\n';
			p[1] = '\0';
			break;
		}
	}
	return 0;
}

static int
openfile(const char *path)
{
	if (infp != 0)
		fclose(infp);
	lineno = 0;
	if ((infp = fopen(path, "r")) == 0)
	{
		warn(":1752:cannot locate source file %s\n", path);
		return -1;
	}
	return 0;
}

static void
listfrag(struct function *fp)
{
	const char *path = fp->srcf->path;
	unsigned long i, reset;

	if (openfile(path) != 0)
		return;
	if (fp->rawp == 0)
	{
		pfmt(stdout, MM_NOSTD, ":1760:\nINLINED CODE [in %s]:  %s\n\n",
			fp->name, path);
	}
	else
	{
		pfmt(stdout, MM_NOSTD, ":1761:\nINCLUDED CODE:  %s@%s\n\n",
			path, fp->name);
	}
	i = 0;
	reset = fp->slin;
	for (;;)
	{
		if (nextline() == EOF)
			return;
		if (lineno != fp->line[i].num)
		{
			if (lineno < reset)
				continue;
			if (reset == 0
				&& lineno + 3 * ADJUSTSLIN < fp->line[i].num)
			{
				/*
				* A big enough gap for us to assume that
				* it's a different function--JUST GUESSING.
				*/
				reset = fp->line[i].num - ADJUSTSLIN;
				continue;
			}
			printf(nolinefmt, inbuf);
			continue;
		}
		reset = 0;
		if (args.option & OPT_BLKCOVER)
		{
			printf(fp->line[i++].cnt == 0 ? zerofmt : nonzerofmt,
				lineno, inbuf);
		}
		else
		{
			printf(linefmt, fp->line[i++].cnt, lineno, inbuf);
		}
		for (;;) /* skip redundant linenumb entries */
		{
			if (i >= fp->nlnm)
				goto out;
			if (lineno != fp->line[i].num)
				break;
			i++;
		}
	}
	/*
	* Show one more line to complete the fragment.
	*/
out:;
	if (nextline() != EOF)
		printf(nolinefmt, inbuf);
}

static void
listsrc(struct srcfile *sfp)
{
	struct function *fp;
	unsigned long i;

	if (openfile(sfp->path) != 0)
		return;
	pfmt(stdout, MM_NOSTD, ":1762:%sSOURCE FILE:  %s\n\n",
		between, sfp->path);
	if (between[0] == '\0')
		between = (args.option & OPT_FORMFEED) ? "\n\f" : "\n";
	i = 0;
	fp = sfp->func;
	while (nextline() != EOF)
	{
		if (fp != 0 && i >= fp->nlnm)
		{
			i = 0;
			fp = fp->next;
		}
		if (fp == 0 || lineno < fp->slin)
		{
			if (!(args.option & OPT_FUNCTION))
				printf(nolinefmt, inbuf);
			continue;
		}
		if (lineno != fp->line[i].num)
		{
			printf(nolinefmt, inbuf);
			continue;
		}
		if (args.option & OPT_BLKCOVER)
		{
			printf(fp->line[i++].cnt == 0 ? zerofmt : nonzerofmt,
				lineno, inbuf);
		}
		else
		{
			printf(linefmt, fp->line[i++].cnt, lineno, inbuf);
		}
		/*
		* Skip redundant linenumb entries.
		*/
		while (i < fp->nlnm && lineno == fp->line[i].num)
			i++;
	}
	if (fp != 0 && i < fp->nlnm)
		warn(":1240:unexpected EOF\n");
	if ((args.option & OPT_SEPARATE) == 0) /* fragments from others */
	{
		for (fp = sfp->incl; fp != 0; fp = fp->incl)
		{
			if (fp->name != 0)
				listfrag(fp);
		}
	}
}

static void
listcnt(struct srcfile *sfp)
{
	unsigned long i, num;
	struct linenumb *lnp;
	struct function *fp;

	printf("%s%s\n", between, sfp->path);
	if (between[0] == '\0')
		between = (args.option & OPT_FORMFEED) ? "\n\f" : "\n";
	for (fp = sfp->func; fp != 0; fp = fp->next)
	{
		fputs("    ", stdout);
		if (fp->rawp == 0)
		{
			pfmt(stdout, MM_NOSTD, ininfrom,
				fp->name, fp->srcf->path);
		}
		else
			printf("%s\n", fp->name);
		lnp = fp->line;
		num = 0;
		for (i = 0; i < fp->nlnm; lnp++, i++)
		{
			if (lnp->num == num)
				continue;
			num = lnp->num;
			printf(cntsfmt, lnp->num, lnp->cnt);
		}
	}
	if ((args.option & OPT_SEPARATE) == 0)
	{
		for (fp = sfp->incl; fp != 0; fp = fp->incl)
		{
			if (fp->name == 0)
				continue;
			printf("\n%s: ", fp->srcf->path);
			pfmt(stdout, MM_NOSTD, fp->rawp == 0
				? ":1764:inlined in "
				: ":1763:included in %s\n    ", sfp->path);
			printf("%s\n", fp->name);
			lnp = fp->line;
			num = 0;
			for (i = 0; i < fp->nlnm; lnp++, i++)
			{
				if (lnp->num == num)
					continue;
				num = lnp->num;
				printf(cntsfmt, lnp->num, lnp->cnt);
			}
		}
	}
}

void
listing(void)
{
	void (*lf)(struct srcfile *);
	struct srcfile *sfp;

	if (args.option & OPT_CNTDUMP)
		lf = &listcnt;
	else
		lf = &listsrc;
	for (sfp = unit; sfp != 0; sfp = sfp->next)
		(*lf)(sfp);
	if (args.option & OPT_SEPARATE)
	{
		for (sfp = incl; sfp != 0; sfp = sfp->next)
			(*lf)(sfp);
	}
}
