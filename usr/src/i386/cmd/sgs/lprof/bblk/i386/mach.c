#ident	"@(#)lprof:bblk/i386/mach.c	1.4"
/*
* mach.c - x86 specific handling
*/
#include <stdio.h>
#include <string.h>
#include "bblk.h"

void
machinit(void) /* add $ as a letter and comment start characters */
{
	setchtab((const unsigned char *)"$", CH_LET);
	setchtab((const unsigned char *)"/#", CH_CMT);
}

void
machdirective(unsigned char *ident) /* handle .long and .value */
{
	int item;

	if (mainmode == MODE_SCAN)
		return;
	switch (ident[1]) {
	case 'l': /* .long */
		if (strcmp((char *)&ident[2], "ong") != 0)
			return;
		item = DOT_4BYTE;
		break;
	case 'v': /* .value */
		if (strcmp((char *)&ident[2], "alue") != 0)
			return;
		item = DOT_2BYTE;
		break;
	default:
		return;
	}
	nbyte(item);
}

void
label(unsigned char *ident) /* handle label in SCN_TEXT */
{
	switch (mainmode) {
	default:
		error(":1670:unknown main mode (%d)\n", (int)mainmode);
		/*NOTREACHED*/
	case MODE_SCAN:
	case MODE_INIT:
		if (ident[0] != '.')
			scanmode(ident);
		return;
	case MODE_FUNC:
	case MODE_BBLK:
	case MODE_INCR:
		if (ident[0] != '.') {
			error(":1671:non-. label in function body: %s\n",
				ident);
		}
		if (ident[1] != '.' && mainmode == MODE_FUNC) {
#ifdef DEBUG
			printf("#BBLK:%lu:label %s starts block\n",
				lineno, ident);
#endif
			mainmode = MODE_BBLK;
		}
		return;
	}
}

void
startlabel(unsigned long uniq) /* note point in .text for coverage set */
{
	printf("\t.pushsection .text; .set .C%lu.dot,.; .popsection\n", uniq);
}

void
funcend(void) /* dump coverage structure for current function */
{
	struct set *sp = &cover;
	unsigned int i;
	char pre[16];

	do {
		if (sp->nline == 0)
			continue;
		if (sp == &cover)
			pre[0] = '\0';
		else
			sprintf(pre, "%lu.", sp->uniq);
		fputs("\t.pushsection\t.data\n\t.align\t4\n", stdout);
		printf(".C%lu:\n__coverage.%s%s:\n", sp->uniq, pre, scanfunc);
		printf("\t.4byte\t.C%lu.dot\n", sp->uniq);
		printf("\t.zero\t%lu\n", sizeof(unsigned long) * sp->nline);
		for (i = 0; i < sp->nline; i++)
			printf("\t.4byte\t%lu\n", sp->line[i]);
		printf("\t.local\t__coverage.%s%s\n", pre, scanfunc);
		printf("\t.size\t__coverage.%s%s,.-.C%lu\n",
			pre, scanfunc, sp->uniq);
		printf("\t.type\t__coverage.%s%s,\"object\"\n",
			pre, scanfunc);
		fputs("\t.popsection\n", stdout);
	} while ((sp = sp->next) != 0);
	cover.nline = 0;
}

int
branch(unsigned char *ident) /* 0 for nonbranch; 1 for CC using; else -1 */
{
	static const char *const ca[] = {
		"call", "call0", "call1", "call2", 0
	};
	static const char *const ja[] = {
		"jae", 0
	};
	static const char *const jb[] = {
		"jbe", 0
	};
	static const char *const jc[] = {
		"jcxz", 0
	};
	static const char *const je[] = {
		"jecxz", 0
	};
	static const char *const jg[] = {
		"jge", 0
	};
	static const char *const jl[] = {
		"jle", 0
	};
	static const char *const jp[] = {
		"jpe", "jpo", 0
	};
	static const char *const lo[] = {
		"loop", "loope", "loopne", "loopnz", "loopz", 0
	};
	const char *const *list;
	int ret;

	switch (ident[0]) {
	default:
		return 0;
	case 'c':
	call:;
		if (ident[1] != 'a')
			return 0;
		list = ca;
		ret = -1;
		break;
	case 'j':
	jcc:;
		switch (ident[1]) {
		default:
			return 0;
		case 'a':
			list = ja;
			break;
		case 'b':
			list = jb;
			break;
		case 'c':
			list = jc;
			break;
		case 'e':
			list = je;
			break;
		case 'g':
			list = jg;
			break;
		case 'l':
			list = jl;
			break;
		case 'm':
		jmp:;
			if (ident[2] == 'p' && ident[3] == '\0')
				return -1;
			return 0;
		case 'n':
			ident++;
			goto jcc; /* permits some junk mnemonics */
		case 'o':
		case 's':
		case 'z':
			list = 0;
			break;
		case 'p':
			list = jp;
			break;
		}
		if (ident[2] == '\0')
			return 1;
		else if (list == 0) /* only the two character mnemonic */
			return 0;
		ret = 1;
		break;
	case 'l':
		switch (ident[1]) {
		default:
			return 0;
		case 'c':
			ident++;
			goto call;
		case 'j':
			ident++;
			goto jmp;
		case 'o':
			list = lo;
			ret = 1;
			break;
		}
		break;
	case 'r':
		if (strcmp((char *)&ident[1], "et") == 0)
			return -1;
		return 0;
	}
	ident += 2;
	do {
		if (strcmp((char *)ident, &list[0][2]) == 0)
			return ret;
	} while (*++list != 0);
	return 0;
}

void
instruction(unsigned char *ident) /* handle SCN_TEXT instruction "ident" */
{
	unsigned long n;
	struct set *sp;
	int code;

	if (mainmode == MODE_FUNC) {
		if (branch(ident)) {
#ifdef DEBUG
			printf("#BBLK:%lu:branch %s ends block\n",
				lineno, ident);
#endif
			mainmode = MODE_BBLK;
		}
	} else if (mainmode == MODE_INCR) {
		if ((sp = cover.next) == 0 || sp->file == 0)
			sp = &cover;
		n = sp->nline;
		code = branch(ident);
		if (code > 0)
			fputs("\tpushf\t/bblk\n", stdout);
		printf("\tincl\t.C%lu+%lu\t/bblk:%lu[%lu]\n",
			sp->uniq, n * sizeof(unsigned long),
			sp->line[n - 1], sp->file);
		if (code > 0)
			fputs("\tpopf\t/bblk\n", stdout);
		mainmode = code != 0 ? MODE_BBLK : MODE_FUNC;
	}
}
