#ident	"@(#)lprof:cmd/util.c	1.2"
/*
* util.c - general support code
*/
#include <limits.h>
#include <pfmt.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lprof.h"

void
fatal(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vpfmt(stderr, MM_ERROR, fmt, ap);
	va_end(ap);
	exit(2);
}

void
error(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vpfmt(stderr, MM_ERROR, fmt, ap);
	va_end(ap);
}

void
warn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vpfmt(stderr, MM_WARNING, fmt, ap);
	va_end(ap);
}

void *
alloc(size_t sz) /* front end to malloc() */
{
	void *p;

	if ((p = malloc(sz)) == 0)
	{
		fatal(":1659:failed to allocate %lu bytes\n",
			(unsigned long)sz);
	}
	return p;
}

void *
grow(void *p, size_t sz) /* front end to realloc() */
{
	if ((p = realloc(p, sz)) == 0)
	{
		fatal(":1753:unable to grow allocation to %lu bytes\n",
			(unsigned long)sz);
	}
	return p;
}

const char *
search(struct stat *stp, const char *dir, const char *name)
{
	char fullpath[PATH_MAX];

	if (dir != 0)
	{
		snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, name);
		if (stat(fullpath, stp) == 0)
			goto found;
	}
	if (stat(name, stp) == 0)
		return name;
	if (args.dirs.nused != 0)
	{
		unsigned long i = 0;

		do {
			snprintf(fullpath, sizeof(fullpath), "%s/%s",
				args.dirs.list[i], name);
			if (stat(fullpath, stp) == 0)
				goto found;
		} while (++i < args.dirs.nused);
	}
	return 0;
found:;
	return strcpy(alloc(strlen(fullpath) + 1), fullpath);
}
