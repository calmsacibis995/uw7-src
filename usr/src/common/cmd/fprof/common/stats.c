/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved. */
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved. */
/*	  All Rights Reserved */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc. */
/*	The copyright notice above does not evidence any */
/*	actual or intended publication of such source code. */

#ident	"@(#)fprof:common/stats.c	1.2"

#include <sys/types.h>
#include <sys/stat.h>
#include	<macros.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <limits.h>
#include <dlfcn.h>
#include "fprof.h"

typedef ulong timesum;

int SortKey;

struct pr_rec {
	char *str;
	ulong num[3]; 
};

static struct pr_rec *Pr;
static int Npr = 0;
static ulong Prden[3];

static char *Dashes = "----------------------------------------------";

struct parent {
	timesum onstack;
	timesum topstack;
	ulong count;
};

#define IS_SHORT	1

struct stats {
	unchar flags;
	struct sym *sym;
	ulong statno;
	ulong count;
	timesum onstack;
	timesum topstack;
	struct parent *parents;
	ushort nparents;
	ushort nindirect;
	ushort times_on_stack;
	unchar *indirect;
};

struct stack {
	struct sym *sym;
	fptime start_time;
	fptime midstack;
	ushort times_on_stack;
	ushort recursion;
};

struct stats **Stats;
ulong Nstats;

struct stack *Stack;
ulong Nstack;
ulong Sstack;

void *Ret;

#define GROWSIZE 128

#define popsym() ((Nstack > 0) ? Stack + --Nstack : NULL)
#define topsym() (Stack + Nstack-1)
#define empty()  (!Nstack)

#define MALLOC(X) ((Ret = (void *) malloc(X)) ? Ret : crash())
#define REALLOC(S, X) ((Ret = (void *) realloc(S, X)) ? Ret : crash())
#define CALLOC(S, X) ((Ret = (void *) calloc(S, X)) ? Ret : crash())

#define pushsym(SYM, TIME, ONSTACK) do { \
	if (Nstack == Sstack) \
		growstack(); \
	Stack[Nstack].sym = (SYM); \
	Stack[Nstack].times_on_stack = ((struct stats *) (SYM)->ptr)->times_on_stack++; \
	Stack[Nstack].midstack = (0); \
	Stack[Nstack].recursion = 0; \
	Stack[Nstack++].start_time = (TIME); \
} while (0)

#define SHORT_MODE(STATS) ((STATS)->flags & IS_SHORT)
#define not_indirect(STATS, STATNO) (SHORT_MODE(STATS) ? (((ushort *) (STATS)->indirect)[(STATNO)] == (ushort) 0xffff) : (((STATS)->indirect)[(STATNO)] == (ushort) 0xff))
#define get_indirect(STATS, STATNO) (SHORT_MODE(STATS) ? ((ushort *) (STATS)->indirect)[(STATNO)] : ((ushort) ((STATS)->indirect[(STATNO)])))

#define set_indirect(STATS, STATNO, NUM) do { \
	if (SHORT_MODE(STATS)) \
		((ushort *) (STATS)->indirect)[(STATNO)] = NUM; \
	else \
		((unchar *) (STATS)->indirect)[(STATNO)] = NUM; \
} while(0)

struct fp *LastFptr;

extern void (*fp_prfunc)();
extern void errprintf();
extern int fp_inxksh;

static void *
crash()
{
	fprintf(stderr, "Out of memory\n");
	_exit(1);
}

static void
growstack(void)
{
	Sstack += GROWSIZE;
	Stack = (struct stack *) REALLOC(Stack, Sstack * sizeof(struct stack));
}

void
dumpstat(int i)
{
	char fld2[20], fld3[20], fld4[20];

	sprintf(fld2, "%lu", Stats[i]->count);
	sprintf(fld3, "%d.%03d:%03d", Stats[i]->topstack / 1000000, (Stats[i]->topstack % 1000000) / 1000, Stats[i]->topstack % 1000);
	sprintf(fld4, "%d.%03d:%03d", Stats[i]->onstack / 1000000, (Stats[i]->onstack % 1000000) / 1000, Stats[i]->onstack % 1000);
	fp_prfunc("%-30.30s  %-15.15s  %-15.15s  %-.15s\n", Stats[i]->sym->name, fld2, fld3, fld4);
}

static int
statcomp(const void *stat1, const void *stat2)
{
	return(Stats[*((ushort *) stat2)]->onstack - Stats[*((ushort *) stat1)]->onstack);
}

void
dumpstats()
{
	ushort i;
	static ushort *statsort = NULL;

	if (!statsort) {
		statsort = (ushort *) malloc(Nstats * sizeof(ushort));
		for (i = 0; i < Nstats; i++)
			statsort[i] = i;
		qsort(statsort, Nstats, sizeof(ushort), statcomp);
	}
	fp_prfunc("%-30.30s %-15.15s %-15.15s %-15.15s\n", "           Function", "  No. of Calls", "  Top of Stack", "   On Stack");
	fp_prfunc("%-30.30s %-15.15s %-15.15s %-15.15s\n", Dashes, Dashes, Dashes, Dashes);
	for (i = 0; i < Nstats; i++)
		dumpstat(statsort[i]);
}

static void
resetpr(ulong den1, ulong den2, ulong den3)
{
	if (!Pr)
		Pr = (struct pr_rec *) malloc(Nstats * sizeof(struct pr_rec));
	Prden[0]= den1;
	Prden[1]= den2;
	Prden[2]= den3;
	Npr = 0;
}

static void
addpr(char *str, ulong num1, ulong num2, ulong num3)
{
	Pr[Npr].str = str;
	Pr[Npr].num[0] = num1;
	Pr[Npr].num[1] = num2;
	Pr[Npr].num[2] = num3;
	Npr++;
}

static int
compnum(const void *rec1, const void *rec2)
{
	return(((struct pr_rec *) rec2)->num[SortKey] - ((struct pr_rec *) rec1)->num[SortKey]);
}

static void
dopr(int sort)
{
	char fld2[20], fld3[20], fld4[20];
	int i;

	if (sort) {
		SortKey = sort;
		qsort(Pr, Npr, sizeof(struct pr_rec), compnum);
	}
	fp_prfunc("%-30.30s %-15.15s %-15.15s %-15s\n", "           Function", "  No. of Calls", "  Top of Stack", "   On Stack");
	fp_prfunc("%-30.30s %-15.15s %-15.15s %-15.15s\n", Dashes, Dashes, Dashes, Dashes);
	for (i = 0; i < Npr; i++) {
		sprintf(fld2, "%d", Pr[i].num[0]);
		sprintf(fld3, "%f", ((double) Pr[i].num[1]) * 100 / Prden[1]);
		sprintf(fld4, "%f", ((double) Pr[i].num[2]) * 100 / Prden[2]);
		fp_prfunc("%-30.30s %-15.15s %-15.15s %-.15s\n", Pr[i].str, fld2, fld3, fld4);
	}
}

void
dumpcallees(ushort j)
{
	int i;
	ushort indir;

	resetpr(1, Stats[j]->onstack, Stats[j]->onstack);
	for (i = 0; i < Nstats; i++) {
		if ((j >= Stats[i]->nindirect) || not_indirect(Stats[i], j))
			continue;
		indir = get_indirect(Stats[i], j);
		addpr(Stats[i]->sym->name, Stats[i]->parents[indir].count, Stats[i]->parents[indir].topstack, Stats[i]->parents[indir].onstack);
	}
	dopr(2);
}

void
dumpcalleesfrom(char *regexp)
{
	char *compiled;
	int i;

	compiled = regcmp(regexp, 0);
	for (i = 0; i < Nstats; i++) {
		if (regex(compiled, Stats[i]->sym->name, NULL)) {
			fp_prfunc("***************** %s - %d, %f *****************\n", Stats[i]->sym->name, Stats[i]->count, ((double) Stats[i]->onstack) / 1000000);
			dumpcallees(Stats[i]->statno);
		}
	}
	free(compiled);
}

void
dumpcallers(int i)
{
	ushort j, indir;
	char fld3[20], fld4[20];

	resetpr(1, Stats[i]->topstack, Stats[i]->onstack);
	for (j = 0; j < Stats[i]->nindirect; j++) {
		if (not_indirect(Stats[i], j))
			continue;
		indir = get_indirect(Stats[i], j);
		addpr(Stats[j]->sym->name, Stats[i]->parents[indir].count, Stats[i]->parents[indir].topstack, Stats[i]->parents[indir].onstack);
	}
	dopr(2);
}

void
dumpcallersof(char *regexp)
{
	char *compiled;
	int i;

	compiled = regcmp(regexp, 0);
	for (i = 0; i < Nstats; i++) {
		if (regex(compiled, Stats[i]->sym->name, NULL)) {
			fp_prfunc("***************** %s - %d, %f *****************\n", Stats[i]->sym->name, Stats[i]->count, ((double) Stats[i]->onstack) / 1000000);
			dumpcallers(i);
		}
	}
	free(compiled);
}

static void
shortize(struct stats *stats, int growno)
{
	unchar *tmp2;
	ushort j;

	stats->flags |= IS_SHORT;
	tmp2 = stats->indirect;
	stats->indirect = (unchar *) MALLOC(growno * sizeof(ushort));
	memset(((ushort *) stats->indirect) + stats->nindirect, 0xff, (growno - stats->nindirect) * sizeof(ushort));
	for (j = 0; j < stats->nindirect; j++) {
		if (tmp2[j] == 0xff)
			((ushort *) stats->indirect)[j] = 0xffff;
		else
			((ushort *) stats->indirect)[j] = tmp2[j];
	}
	free(tmp2);
}

fp_count(struct fp *fptr)
{
	struct logent logent;
	int i;

	if (fptr == LastFptr)
		return;
	fp_rewind_record(fptr);
	while (fp_next_record(fptr, &logent) != FPROF_EOF) {
		if (!logent.symbol)
			continue;
		if (!logent.symbol->ptr) {
			if (!(Nstats & (GROWSIZE-1))) {
				struct stats *buf;

				Stats = (struct stats **) REALLOC(Stats, (Nstats + GROWSIZE) * sizeof(struct stats *));
				buf = (struct stats *) CALLOC(GROWSIZE, sizeof(struct stats));
				for (i = 0; i < GROWSIZE; i++)
					Stats[Nstats + i] = buf + i;
			}
			logent.symbol->ptr = (void *) Stats[Nstats];
			Stats[Nstats]->sym = logent.symbol;
			Stats[Nstats]->statno = Nstats;
			Nstats++;
		}
		if (logent.flags & FPROF_IS_PROLOGUE) {
			((struct stats *) logent.symbol->ptr)->count++;
			if (empty())
				pushsym(logent.symbol, logent.compensated_time, ((struct stats *) logent.symbol->ptr)->onstack);
			else if (topsym()->sym == logent.symbol)
				topsym()->recursion++;
			else
				pushsym(logent.symbol, logent.compensated_time, ((struct stats *) logent.symbol->ptr)->onstack);
		}
		else {
			if (empty()) {
				errprintf("Empty stack for prologue\n");
				continue;
			}
			if (topsym()->sym != logent.symbol) {
				errprintf("Mismatch\n");
				continue;
			}
			if (topsym()->recursion)
				topsym()->recursion--;
			else {
				struct stack *tmp;
				struct stats *stats;
				ulong onstack;
				ulong topstack;
				ulong statno;

				tmp = popsym();
				stats = (struct stats *) tmp->sym->ptr;
				onstack = logent.compensated_time - tmp->start_time;
				stats->times_on_stack--;
				if (!tmp->times_on_stack)
					stats->onstack += onstack;
				topstack = logent.compensated_time - tmp->start_time - tmp->midstack;
				stats->topstack += topstack;

				/*
				** Go through the stack in reverse order to save realloc()'s in
				** indirect, since the stack is going to tend to have higher
				** numbered functions later on the stack
				*/
				for (i = Nstack - 1; i >= 0; i--) {
					if (Stack[i].times_on_stack)
						continue;
					statno = ((struct stats *) Stack[i].sym->ptr)->statno;
					if (statno >= stats->nindirect) {
						if (stats->nparents == 255)
							shortize(stats, statno + 1);
						else if (SHORT_MODE(stats)) {
							stats->indirect = (unchar *) REALLOC(stats->indirect, (statno + 1) * sizeof(ushort));
							memset(((ushort *) stats->indirect) + stats->nindirect, 0xff, (statno - stats->nindirect + 1) * sizeof(ushort));
						}
						else {
							stats->indirect = (unchar *) REALLOC(stats->indirect, (statno + 1) * sizeof(unchar));
							memset(stats->indirect + stats->nindirect, 0xff, (statno - stats->nindirect + 1) * sizeof(unchar));
						}
						stats->nindirect = statno + 1;

						if (!(stats->nparents & (GROWSIZE-1)))
							stats->parents = (struct parent *) REALLOC(stats->parents, (stats->nparents + GROWSIZE) * sizeof(struct parent));

						set_indirect(stats, statno, stats->nparents);
						stats->parents[stats->nparents].count = 1;
						stats->parents[stats->nparents].onstack = onstack;
						stats->parents[stats->nparents++].topstack = topstack;
					}
					else if (not_indirect(stats, statno)) {
						if (stats->nparents == 255)
							shortize(stats, stats->nindirect);

						if (!(stats->nparents & (GROWSIZE-1)))
							stats->parents = (struct parent *) REALLOC(stats->parents, (stats->nparents + GROWSIZE) * sizeof(struct parent));

						set_indirect(stats, statno, stats->nparents);
						stats->parents[stats->nparents].count = 1;
						stats->parents[stats->nparents].onstack = onstack;
						stats->parents[stats->nparents++].topstack = topstack;
					}
					else {
						stats->parents[get_indirect(stats, statno)].count++;
						stats->parents[get_indirect(stats, statno)].onstack += onstack;
						stats->parents[get_indirect(stats, statno)].topstack += topstack;
					}
					/*if ((stats->parents[get_indirect(stats, statno)].onstack > stats->onstack) && stats->times_on_stack)*/
						/*fprintf(stderr, "Got Here\n");*/
				}
				if (!empty())
					topsym()->midstack += logent.compensated_time - tmp->start_time;
			}
		}
	}
	/*dumpstats();*/
}
