#ident	"@(#)lprof:bblk/common/util.c	1.4"
/*
* util.c - misc. support functions
*/
#include <pfmt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bblk.h"

static unsigned long	initlist[256];

struct set cover = { /* scanfunc's main coverage information */
	0, initlist, 0, sizeof(initlist) / sizeof(unsigned long), 0, 1
};

void
error(const char *fmt, ...) /* emit hard error message and exit */
{
	va_list ap;

	pfmt(stderr, MM_ERROR | MM_NOGET, "%lu:", lineno);
	va_start(ap, fmt);
	vpfmt(stderr, MM_ERROR | MM_NOSTD, fmt, ap);
	va_end(ap);
	exit(2);
}

void
warn(const char *fmt, ...) /* emit warning */
{
	va_list ap;

	pfmt(stderr, MM_WARNING | MM_NOGET, "%lu:", lineno);
	va_start(ap, fmt);
	vpfmt(stderr, MM_WARNING | MM_NOSTD, fmt, ap);
	va_end(ap);
}

void *
alloc(size_t sz) /* allocate sz bytes--only returns if successful */
{
	void *p;

	if ((p = malloc(sz)) == 0)
		error(":1659:failed to allocate %lu bytes\n", sz);
	return p;
}

void
addline(unsigned long num, unsigned long file) /* add line number */
{
	static unsigned long uniq; /* unique integer value */
	static struct set *avail;
	struct set *sp;

	/*
	* Check for reset special case.
	*/
	if (num == 0) {
		struct set *nsp;

		/*
		* Put any extra coverage sets on the available list.
		*/
		if ((sp = cover.next) != 0) {
			do {
				nsp = sp->next;
				sp->next = avail;
				avail = sp;
				sp->nline = 0;
				sp->file = 0;
			} while ((sp = nsp) != 0);
		}
#ifdef DEBUG
		printf("#BBLK:%lu:Starting function %s\n",
			lineno, (char *)scanfunc);
#endif
		startlabel(++uniq);
		cover.next = 0;
		cover.uniq = uniq;
		cover.nline = 0;
		return;
	}
	/*
	* Otherwise, it's a real line number to add.
	*/
	if (cover.nline == 0) { /* first line number for scanfunc */
		if (mainmode == MODE_INCR)
			error(":1660:attempt to refill nonexistent slot\n");
		else if (mainmode != MODE_BBLK)
			return;
		cover.file = file;
		sp = &cover;
	} else if (cover.file == file) { /* another line number for scanfunc */
		if ((sp = cover.next) != 0 && sp->file != 0) {
			/*
			* Done with most recent inline expansion.
			* Mark it finished, and make sure that we
			* record this as a new basic block.
			*/
			sp->file = 0;
#ifdef DEBUG
			printf("#BBLK:%lu:End of inline code starts block\n",
				lineno);
#endif
		}
		sp = &cover;
		goto checkmode;
	} else if ((sp = cover.next) != 0 && sp->file == file) {
		/*
		* Another line number for the current inline expansion.
		*/
	checkmode:;
		if (mainmode == MODE_INCR)
			sp->nline--;
		else if (mainmode != MODE_BBLK)
			return;
	} else {
		/*
		* New inline expansion.  Allocate a new coverage set and,
		* after initialization, insert it as the front expansion.
		* Note that we don't distinguish the within-function modes
		* to be sure that we catch the start of any inlined code.
		*/
		if (mainmode == MODE_INIT || mainmode == MODE_SCAN)
			return;
		if ((sp = avail) != 0)
			avail = sp->next;
		else {
			struct fullset {
				struct set	head;
				unsigned long	list[16];
			} *fsp = alloc(sizeof(struct fullset));

			sp = &fsp->head;
			sp->line = fsp->list;
			sp->size = sizeof(fsp->list) / sizeof(unsigned long);
		}
		sp->nline = 0;
		sp->file = file;
		sp->uniq = ++uniq;
		sp->next = cover.next;
		cover.next = sp;
#ifdef DEBUG
		printf("#BBLK:%lu:Inlined code starts block\n", lineno);
#endif
		startlabel(sp->uniq);
	}
	/*
	* Append "num" to the line number list for set "sp".
	*/
	if (sp->nline >= sp->size) {
		unsigned long n = sp->size / 2 * 3; /* 50% growth */
		unsigned long *p = alloc(sizeof(unsigned long) * n);

		memcpy(p, sp->line, sizeof(unsigned long) * sp->size);
		if (sp == &cover) {
			if (sp->line != initlist)
				free(sp->line);
		} else if (sp->line != (unsigned long *)&sp[1])
			free(sp->line);
		sp->line = p;
		sp->size = n;
	}
	sp->line[sp->nline++] = num;
	mainmode = MODE_INCR;
}
