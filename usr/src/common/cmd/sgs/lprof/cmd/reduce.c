#ident	"@(#)lprof:cmd/reduce.c	1.3"
/*
* reduce.c - apply restrictions to the complete data.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dem.h>
#include "lprof.h"

static struct keepfile
{
	struct keepfile	*next;
	const char	*path;
	ino_t		inod;
	dev_t		flsy;
	int		seen;
} *keep;

static char *nambuf;
static size_t namlen;

static void
foldfunc(struct srcfile *sfp)
{
	struct function *fp, *nfp;

	for (; sfp != 0; sfp = sfp->next)
	{
		if ((fp = sfp->func) == 0)
			continue;
		/*
		* Look for functions with identical starting lines.
		* Keep only one of each, summing the other's execution
		* counts into the one that remains.
		*/
		do {
			for (nfp = fp->next; nfp != 0; nfp = nfp->next)
			{
				if (fp->slin != nfp->slin)
					break;
				mergecnts(&fp->line, &fp->nlnm,
					nfp->line, nfp->nlnm);
				nfp->line = 0;
			}
			fp->next = nfp;
		} while ((fp = nfp) != 0);
	}
}

static void
delfcn(struct srcfile *sfp)
{
	struct function *fp, **fpp;
	unsigned long i;

	for (; sfp != 0; sfp = sfp->next)
	{
		fpp = &sfp->func;
		while ((fp = *fpp) != 0)
		{
			i = 0;
			do {
				if (strcmp(fp->name, args.fcns.list[i]) == 0)
					goto found;
			} while (++i != args.fcns.nused);
			/*
			* Not found in the list.  Delete it.
			*/
			*fpp = fp->next;
			if (fp->rawp == 0 || fp->rawp->covp != 0)
				sfp->ncov--;
			fp->name = 0; /* delete from ->incl walk, too */
			continue;
		found:;
			fpp = &fp->next;
			if (fp->slin == fp->line->num) /* try to get start */
			{
				if (fp->slin <= ADJUSTSLIN)
					fp->slin = 1;
				else
					fp->slin -= ADJUSTSLIN;
			}
		}
	}
}

static void
delsrc(struct srcfile **sfpp)
{
	struct keepfile *kfp;
	struct srcfile *sfp;

	while ((sfp = *sfpp) != 0)
	{
		if (sfp->ncov == 0)
			goto skip;
		if ((args.option & OPT_SRCFILE) == 0)
			goto found;
		for (kfp = keep; kfp != 0; kfp = kfp->next)
		{
			if (kfp->inod == sfp->inod && kfp->flsy == sfp->flsy)
			{
				kfp->seen = 1;
				goto found;
			}
		}
		/*
		* Not in the keep list.  Delete it.
		*/
	skip:;
		*sfpp = sfp->next;
		continue;
	found:;
		sfpp = &sfp->next;
	}
}

void
chksrcs(void)
{
	struct keepfile *kfp;
	const char *path;
	unsigned long i;
	struct stat st;

	i = 0;
	do {
		if ((path = search(&st, 0, args.srcs.list[i])) == 0)
		{
			warn(":1752:cannot locate source file %s\n",
				args.srcs.list[i]);
			continue;
		}
		kfp = alloc(sizeof(struct keepfile));
		kfp->path = args.srcs.list[i];
		kfp->inod = st.st_ino;
		kfp->flsy = st.st_dev;
		kfp->seen = 0;
		kfp->next = keep;
		keep = kfp;
	} while (++i < args.srcs.nused);
}

static void
chgname(struct function *fp)
{
	size_t n;
	char *p;

	if (fp->name == 0)
		return;
	if (args.option & OPT_CPPNAMES)
	{
		/*
		* Loop at most once.
		*/
		while ((n = demangle(fp->name, nambuf, namlen)) != -1)
		{
			if (n <= namlen)
			{
				if (strcmp(fp->name, nambuf) == 0)
					break;
				fp->name = memcpy(alloc(n), nambuf, n);
				return;
			}
			namlen = n;
			nambuf = grow(nambuf, namlen);
		}
	}
	/*
	* Append a "()" to make it look like a function name.
	*/
	p = alloc(strlen(fp->name) + 3);
	sprintf(p, "%s()", fp->name);
	fp->name = p;
}

static void
fixnames(struct srcfile *sfp)
{
	struct function *fp;

	for (; sfp != 0; sfp = sfp->next)
	{
		for (fp = sfp->func; fp != 0; fp = fp->next)
			chgname(fp);
		if (args.option & OPT_SEPARATE)
			continue;
		for (fp = sfp->incl; fp != 0; fp = fp->incl)
			chgname(fp);
	}
}

void
reduce(void)
{
	if (args.option & OPT_SEPARATE)
	{
		/*
		* Fold execution counts for multiple uses into one instance.
		*/
		foldfunc(unit);
		foldfunc(incl);
	}
	if (args.option & OPT_FUNCTION)
	{
		/*
		* Remove functions that aren't to be shown.
		*/
		delfcn(unit);
		delfcn(incl);
	}
	/*
	* These are last so that source files with no useful
	* contents are not listed, either.
	*/
	delsrc(&unit);
	delsrc(&incl);
	if (args.option & OPT_SRCFILE)
	{
		struct keepfile *kfp;

		/*
		* Warn about inappropriate "source files".  This
		* may help clarify DWARF II behavior changes.
		*/
		for (kfp = keep; kfp != 0; kfp = kfp->next)
		{
			if (kfp->seen)
				continue;
			warn(":1767:%s has no code from source file %s\n",
				args.prog, kfp->path);
		}
	}
	/*
	* Change regular C function names from "foo" to "foo()".
	* Demangle C++ function names (OPT_CPPNAMES).
	*/
	nambuf = alloc(namlen = 128);
	fixnames(unit);
	if (args.option & OPT_SEPARATE)
		fixnames(incl);
	free(nambuf);
}
