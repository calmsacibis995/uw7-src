/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved. */
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved. */
/*	  All Rights Reserved */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc. */
/*	The copyright notice above does not evidence any */
/*	actual or intended publication of such source code. */

#ident	"@(#)fprof:common/scan.c	1.3"

#include	<stdio.h>
#include	<ctype.h>
#include	<string.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<macros.h>
#include	<sys/stat.h>
#include	"fprof.h"

struct lrtsym {
	/*unchar flags;*/
	int save;
	int bestorder;
	void *sym;
	ulong timesum;
	ulong efficiency;
	int ntimes;
	int sizetimes;
	char *quanta;
	/*fptime *times;*/
	fptime times[2];
};

struct seed {
	char *name;
	void (*func)();
	int flags;
};

static void orig(), early(), late(), median(), sum(), correlate(), pairwise_pattern(), pattern(), reverse();

#define ADDRBASED		1
#define SCATTERGRAM		2
#define SUMCHART		4
#define REVERSE_SEED	8
#define ALLSEEDS		16

struct seed Allseeds[] = {
	{ "Early", early, 0 },
	{ "Reverse Late", late, REVERSE_SEED },
	{ "Late", late, 0 },
	{ "Sum", sum, 0 },
	{ "Reverse Sum", sum, REVERSE_SEED },
	{ NULL, NULL, 0 },
};

struct algorithm {
	char *name;
	void (*func)();
	int flags;
	void (*seedfunc)();
	ulong opt1;
	ulong opt2;
};

static struct algorithm Algos[] = {
	/*{ "Pairwise Pattern", pairwise_pattern, ALLSEEDS, late, 10000, 0 },*/
	{ "Pairwise Pattern - 200 lookahead", pairwise_pattern, ALLSEEDS, late, 200, 0 },
	/*{ "Correlation", correlate, ALLSEEDS, late, 0, 0 },*/
	/*{ "Pattern", pattern, ALLSEEDS, late, 0, 0 },*/
	{ "Sum", sum, 0, NULL, 0, 0 },
	{ "Median", median, 0, NULL, 0, 0 },
	{ "Late", late, 0, NULL, 0, 0 },
	{ "Early", early, 0, NULL, 0, 0 },
	{ "Original - Zeroes", orig, 0, NULL, 0, 0 },
	{ "Original", orig, ADDRBASED, NULL, 0, 0 },
	{ NULL, NULL, 0, NULL, 0, 0 }
};

#define TIME(SYMNO, TIMENO) (Lrtsyms[SYMNO]->times[TIMENO])
#define NTIMES(SYMNO) (Lrtsyms[SYMNO]->ntimes)

static int Nstack;
static int Sstack;
struct lrtsym **Stack;

static int Nsyms;
static int Sizesyms;
static struct lrtsym **Lrtsyms;
static struct lrtsym Dummy = { NULL, 0, 0, 0 };

static ulong Timerec = 0;
static ulong Nquanta = 0;
static ulong MaxRecords = 0;
static ulong TestSize = 2000;
static ulong ComputeSize = 2000;
static ulong Pagesize = 4096;
static ulong CompareDistance = 1000;
static ulong SelfDistance = 1000;
static int Debug = 0;
static int Debug2 = 0;

#define SWAP(I, J) do { \
						struct lrtsym *tmp;\
\
						if ((I) == (J))\
							break;\
						tmp = Lrtsyms[I];\
						Lrtsyms[I] = Lrtsyms[J];\
						Lrtsyms[J] = tmp;\
} while(0)

usage()
{
	fprintf(stderr, "usage: scan [-s slicesize] nm-file data-file\n");
	exit(1);
}

#define popsym() (Stack[--Nstack])

#define pushsym(LRTSYM) do {\
	if ((Nstack + 1) >= Sstack)\
		stackrealloc();\
	Stack[Nstack++] = (LRTSYM);\
} while(0)

static int
stackrealloc()
{
	Sstack += 100;
	Stack = (struct lrtsym **) realloc(Stack, Sstack * sizeof(struct lrtsym *));
}

static struct lrtsym *
savesym(void *sym)
{
	if (fp_symbol_to_pointer(sym))
		return((struct lrtsym *) fp_symbol_to_pointer(sym));
	
	if (!Nsyms) {
		int i;
		struct lrtsym *mem;

		Sizesyms = fp_symbol_to_object_size(sym);
		Lrtsyms = (struct lrtsym **) malloc(Sizesyms * sizeof(struct lrtsym *));
		mem = (struct lrtsym *) malloc(Sizesyms * sizeof(struct lrtsym));
		memset(mem, '\0', Sizesyms * sizeof(struct lrtsym));
		for (i = 0; i < Sizesyms; i++)
			Lrtsyms[i] = mem + i;
	}
	/*if (Nsyms == Sizesyms) {*/
		/*Sizesyms += 100;*/
		/*Lrtsyms = (struct lrtsym **) realloc(Lrtsyms, Sizesyms * sizeof(struct lrtsym *));*/
	/*}*/
	
	/*Lrtsyms[Nsyms] = (struct lrtsym *) malloc(sizeof(struct lrtsym));*/
	/*memset(Lrtsyms[Nsyms], '\0', sizeof(struct lrtsym));*/
	Lrtsyms[Nsyms]->sym = sym;
	Lrtsyms[Nsyms]->quanta = (char *) malloc((MaxRecords / ComputeSize) * sizeof(char));
	memset(Lrtsyms[Nsyms]->quanta, '\0', (MaxRecords / ComputeSize) * sizeof(char));
	fp_symbol_set_pointer(sym, Lrtsyms[Nsyms]);
	Nsyms++;
	return(Lrtsyms[Nsyms - 1]);
}

static void
addtime_to_sym(struct lrtsym *lrtsym, fptime timestamp)
{
	if (!lrtsym->sym)
		return;
#ifdef COMMENTED_OUT
	if (lrtsym->sizetimes == lrtsym->ntimes) {
		if (lrtsym->sizetimes) {
			if (lrtsym->sizetimes >= 1000) {
				/*if (MaxRecords / timestamp < 10)*/
					/*lrtsym->sizetimes = (lrtsym->sizetimes + 1) * MaxRecords / timestamp;*/
				/*else*/
					lrtsym->sizetimes = (lrtsym->sizetimes * 3) / 2;
			}
			else
				lrtsym->sizetimes += 100;
		}
		else
			lrtsym->sizetimes = 10;
		lrtsym->times = (fptime *) realloc(lrtsym->times, lrtsym->sizetimes * sizeof(fptime));
	}
	lrtsym->times[lrtsym->ntimes++] = timestamp;
#endif
	if (lrtsym->ntimes)
		lrtsym->times[1] = timestamp;
	else {
		lrtsym->times[0] = timestamp;
		lrtsym->times[1] = timestamp;
		lrtsym->ntimes = 2;
	}
	if (!(lrtsym->quanta[timestamp / ComputeSize])) {
		lrtsym->quanta[timestamp / ComputeSize] = 1;
		lrtsym->timesum += timestamp / ComputeSize + 1;
		/*lrtsym->timesum += timestamp;*/
	}
}

static void
sum_chart(FILE *fp, int pagesize)
{
	int i, j, page, addr;
	char buf[50];

	for (page = 1, addr = 0, i = 0; i < Nsyms; i++) {
		fprintf(fp, "%-30.30s - %d\n", fp_symbol_to_name(Lrtsyms[i]->sym), Lrtsyms[i]->timesum);
		addr += fp_symbol_size(Lrtsyms[i]->sym);
		if (addr >= page * pagesize) {
			fprintf(fp, "New Page\n");
			page++;
		}
	}
}

static void
reverse()
{
	int i;
	
	for (i = 0; i < Nsyms / 2; i++)
		SWAP(Nsyms - i - 1, i);
}

static int
origcomp(void *sym1, void *sym2)
{
	return(fp_symbol_to_addr(((struct lrtsym **) sym1)[0]->sym) - fp_symbol_to_addr(((struct lrtsym **) sym2)[0]->sym));
}

static void
orig()
{
	qsort(Lrtsyms, Nsyms, sizeof(struct lrtsym *), origcomp);
}

static int
sumcomp(void *sym1, void *sym2)
{
	/*return(((struct lrtsym **) sym2)[0]->times[((struct lrtsym **) sym2)[0]->ntimes / 2] - ((struct lrtsym **) sym1)[0]->times[((struct lrtsym **) sym1)[0]->ntimes / 2]);*/

	if (((struct lrtsym **) sym2)[0]->timesum > ((struct lrtsym **) sym1)[0]->timesum)
		return(1);
	else if (((struct lrtsym **) sym2)[0]->timesum < ((struct lrtsym **) sym1)[0]->timesum)
		return(-1);
	else
		return(((struct lrtsym **) sym1)[0]->ntimes - ((struct lrtsym **) sym2)[0]->ntimes);
}

static void
sum()
{
	qsort(Lrtsyms, Nsyms, sizeof(struct lrtsym *), sumcomp);
}

static int
mediancomp(void *sym1, void *sym2)
{
	return(((struct lrtsym **) sym2)[0]->times[((struct lrtsym **) sym2)[0]->ntimes / 2] - ((struct lrtsym **) sym1)[0]->times[((struct lrtsym **) sym1)[0]->ntimes / 2]);
}

static void
median()
{
	qsort(Lrtsyms, Nsyms, sizeof(struct lrtsym *), mediancomp);
}

static int
arrcomp(int sym1, int sym2)
{
	int i1, i2;
	fptime start;
	int corr = 0;
	int samples = 0;

	for (i1 = 0, i2 = 0; i1 < NTIMES(sym1); ) {
		samples++;
		for ( ; (i2 < NTIMES(sym2)) && (TIME(sym2, i2) < TIME(sym1, i1) - CompareDistance); i2++)
			;
		if ((i2 < NTIMES(sym2)) && (TIME(sym2, i2) < TIME(sym1, i1) + CompareDistance))
			corr++;
		for (start = TIME(sym1, i1), i1++; (i1 < NTIMES(sym1)) && (TIME(sym1, i1) - start < SelfDistance); i1++)
			;
	}
	for (i2 = 0, i1 = 0; i2 < NTIMES(sym2); ) {
		samples++;
		for ( ; (i1 < NTIMES(sym1)) && (TIME(sym1, i1) < TIME(sym2, i2) - CompareDistance); i1++)
			;
		if ((i1 < NTIMES(sym1)) && (TIME(sym1, i1) < TIME(sym2, i2) + CompareDistance))
			corr++;
		for (start = TIME(sym2, i2), i2++; (i2 < NTIMES(sym2)) && (TIME(sym2, i2) - start < SelfDistance); i2++)
			;
	}
	return(1000 * corr / samples);
}

static int
latecomp(void *sym1, void *sym2)
{
	return(((struct lrtsym **) sym2)[0]->times[((struct lrtsym **) sym2)[0]->ntimes - 1] - ((struct lrtsym **) sym1)[0]->times[((struct lrtsym **) sym1)[0]->ntimes - 1]);
}

static void
late()
{
	qsort(Lrtsyms, Nsyms, sizeof(struct lrtsym *), latecomp);
}

static void
correlate()
{
	ulong i, nfound, besti, bestratio, ratio;

	for (nfound = 1; nfound < Nsyms; nfound++) {
		bestratio = 0;
		besti = nfound;
		for (i = nfound; i < Nsyms; i++) {
			/*if ((Lrtsyms[nfound - 1]->timesum < 3 * Lrtsyms[i]->timesum) && (Lrtsyms[i]->timesum < 3 * Lrtsyms[nfound - 1]->timesum)) {*/
				ratio = arrcomp(nfound-1, i);
				/* if (Debug) */
					/* fprintf(stderr, "Ratio is %d\n", ratio); */
				if (ratio > bestratio) {
					besti = i;
					bestratio = ratio;
				}
			/*}*/
		}
		if (Debug)
			fprintf(stderr, "Ratio is %d\n", ratio);
		SWAP(besti, nfound);
	}
}

static void
pairwise_pattern(int lookahead)
{
	ulong i, nfound, besti, bestratio, totaleither, totaleach, last;
	register ulong j;
	register char *quanta1, *quanta2;

	for (nfound = 1; nfound < Nsyms; nfound++) {
		quanta1 = Lrtsyms[nfound - 1]->quanta;
		bestratio = 0;
		besti = nfound;
		last = min(Nsyms, nfound + lookahead);
		for (i = nfound; i < last; i++) {
			quanta2 = Lrtsyms[i]->quanta;
			totaleach = totaleither = 0;
			for (j = 0; j < Nquanta; j++) {
				if (quanta1[j] || quanta2[j]) {
					totaleither++;
					if (quanta1[j])
						totaleach++;
					if (quanta2[j])
						totaleach++;
				}
			}
			if (!totaleither) /* should not happen */
				totaleither = 1;
			if (bestratio < (1000 * totaleach / totaleither)) {
				bestratio = 1000 * totaleach / totaleither;
				besti = i;
			}
		}

		/*if (Debug)*/
			/*fprintf(stderr, "Best function has name: %s, number: %d, ratio: %d and size: %d\n", fp_symbol_to_name(Lrtsyms[besti]->sym), besti, bestratio, fp_symbol_size(Lrtsyms[besti]->sym));*/
		if (Debug2)
			fprintf(stderr, "Best function has number: %d, sum: %d\n", besti, Lrtsyms[besti]->timesum);

		SWAP(besti, nfound);
	}
}

static void
pattern()
{
	ulong i, j, nfound, besti, bestratio, usesize, totalpagesize, totalfuncsize, curpagesize;
	struct lrtsym *tmp;
	char quantapage[2000];

	for (i = 0; i < Nsyms; i++)
		Lrtsyms[i]->save = i;
	curpagesize = 0;
	for (nfound = 0; nfound < Nsyms; nfound++) {
		bestratio = 0;
		besti = nfound;
		for (i = nfound; (i > 0) && i < Nsyms; i++) {
			int tmp;

			totalfuncsize = totalpagesize = 0;
			usesize = min(Pagesize - curpagesize, fp_symbol_size(Lrtsyms[i]->sym));
			tmp = curpagesize + usesize;
			for (j = 0; j < Nquanta; j++) {
				if (quantapage[j] || Lrtsyms[i]->quanta[j]) {
					totalpagesize += tmp;
					/*totalpagesize += Pagesize;*/
					if (quantapage[j]) {
						totalfuncsize += curpagesize;
					}
					if (Lrtsyms[i]->quanta[j]) {
						totalfuncsize += usesize;
					}
				}
			}
			if (!totalpagesize)
				totalpagesize = 1;
			/* if (Debug) */
				/* fprintf(stderr, "Ratio is %d\n", 1000 * totalfuncsize / totalpagesize); */
			if (bestratio < (1000 * totalfuncsize / totalpagesize)) {
				bestratio = 1000 * totalfuncsize / totalpagesize;
				besti = i;
			}
		}

		if (Debug)
			fprintf(stderr, "Best function has name: %s, ratio: %d and size: %d\n", fp_symbol_to_name(Lrtsyms[besti]->sym), bestratio, fp_symbol_size(Lrtsyms[besti]->sym));

		while (1) {
			curpagesize += fp_symbol_size(Lrtsyms[besti]->sym);
			if (curpagesize >= Pagesize) {
				if (Debug)
					fprintf(stderr, "New page\n");
				curpagesize %= Pagesize;
				memset(quantapage, '\0', Nquanta * sizeof(char));
			}

			SWAP(besti, nfound);


			if (curpagesize != 0) {
				for (j = 0; j < Nquanta; j++)
					if (Lrtsyms[nfound]->quanta[j])
						quantapage[j] = 1;

				break;
			}

			/* curpagesize = 0, we must reseed */
			fprintf(stderr, "Reseeding\n");
			besti = nfound + 1;
			for (i = nfound + 2; i < Nsyms; i++)
				if (Lrtsyms[i]->save < Lrtsyms[besti]->save)
					besti = i;

			if (besti >= Nsyms)
				break;
		};
	}
}

static int
earlycomp(void *sym1, void *sym2)
{
	return(((struct lrtsym **) sym1)[0]->times[0] - ((struct lrtsym **) sym2)[0]->times[0]);
}

static void
early()
{
	qsort(Lrtsyms, Nsyms, sizeof(struct lrtsym *), earlycomp);
}

static void
output()
{
	int i;

	for (i = 0; i < Nsyms; i++)
		printf("%s\n", fp_symbol_to_name(Lrtsyms[i]->sym));
}

static void
scattergram(FILE *fp, int pagesize)
{
	int i, j, page, addr;
	char buf[50];

	for (page = 1, addr = 0, i = 0; i < Nsyms; i++) {
		sprintf(buf, "%-25.25s@0x%x (%d) - sum %d", fp_symbol_to_name(Lrtsyms[i]->sym), addr, Lrtsyms[i]->efficiency, Lrtsyms[i]->timesum);
		fprintf(fp, "%-42.42s", buf);
		for (j = 0; j < Nquanta; j++)
			fputc(Lrtsyms[i]->quanta[j] ? '.' : ' ', fp);
		fputc('\n', fp);
		addr += fp_symbol_size(Lrtsyms[i]->sym);
		if (addr >= page * pagesize) {
			fprintf(fp, "New Page\n");
			page++;
		}
	}
}

static ulong
qmetric(ulong slicesize, ulong flag, ulong pagesize, ulong *working_setp)
{
	int i, j, quantum, prev_quantum, funcsize;
	unchar *hits;
	double totalpagesize, totalfuncsize;
	double prevpagesize, prevfuncsize;
	char *addrbase;
	ulong funcaddr, funcend, page;

	hits = (unchar *) malloc(sizeof(unchar) * (Timerec / slicesize + 1));
	memset(hits, '\0', sizeof(unchar) * (Timerec / slicesize + 1));
	prevpagesize = totalpagesize = 0;
	prevfuncsize = totalfuncsize = 0;
	i = 0;
	addrbase = fp_symbol_to_addr(Lrtsyms[0]->sym);
	funcaddr = 0;
	funcend = funcaddr + fp_symbol_size(Lrtsyms[0]->sym);
	for (page = 1; i < Nsyms; page++) {
		for ( ; i < Nsyms; i++,
			funcaddr = ((flag & ADDRBASED) ? (fp_symbol_to_addr(Lrtsyms[i]->sym) - addrbase) : funcend),
			funcend = funcaddr + fp_symbol_size(Lrtsyms[i]->sym)) {

			funcsize = min(page * pagesize, funcend) - max((page - 1) * pagesize, funcaddr);
			if (funcsize > 0) {
				for (j = 0; j < Nquanta; j++) {
					if (Lrtsyms[i]->quanta[j]) {
						totalfuncsize += funcsize;
						if (!hits[j]) {
							hits[j] = 1;
							totalpagesize += pagesize;
						}
					}
				}
			}
			if (funcend >= (page * pagesize)) {
				if (totalpagesize > prevpagesize)
					Lrtsyms[i]->efficiency = 1000 * (totalfuncsize - prevfuncsize) / (totalpagesize - prevpagesize);
				prevfuncsize = totalfuncsize;
				prevpagesize = totalpagesize;
				memset(hits, '\0', sizeof(unchar) * (Timerec / slicesize + 1));
				break;
			}
			else {
				if (totalpagesize > prevpagesize)
					Lrtsyms[i]->efficiency = 1000 * (totalfuncsize - prevfuncsize) / (((totalpagesize - prevpagesize) / pagesize) * (funcend % pagesize));
			}
		}
	}
	free(hits);
	*working_setp = 10 * totalpagesize / Pagesize / (Timerec / slicesize + 1);
	return((ulong) (1000 * totalfuncsize / totalpagesize));
}

static ulong
metric(ulong slicesize, ulong flag, ulong pagesize, ulong *working_setp)
{
	int i, j, quantum, prev_quantum, funcsize;
	unchar *hits;
	double totalpagesize, totalfuncsize;
	double prevpagesize, prevfuncsize;
	char *addrbase;
	ulong funcaddr, funcend, page;

	hits = (unchar *) malloc(sizeof(unchar) * (Timerec / slicesize + 1));
	memset(hits, '\0', sizeof(unchar) * (Timerec / slicesize + 1));
	prevpagesize = totalpagesize = 0;
	prevfuncsize = totalfuncsize = 0;
	i = 0;
	addrbase = fp_symbol_to_addr(Lrtsyms[0]->sym);
	funcaddr = 0;
	funcend = funcaddr + fp_symbol_size(Lrtsyms[0]->sym);
	for (page = 1; i < Nsyms; page++) {
		for ( ; i < Nsyms; i++,
			funcaddr = ((flag & ADDRBASED) ? (fp_symbol_to_addr(Lrtsyms[i]->sym) - addrbase) : funcend),
			funcend = funcaddr + fp_symbol_size(Lrtsyms[i]->sym)) {

			funcsize = min(page * pagesize, funcend) - max((page - 1) * pagesize, funcaddr);
			if (funcsize > 0) {
				prev_quantum = -1;
				for (j = 0; j < Lrtsyms[i]->ntimes; j++) {
					quantum = Lrtsyms[i]->times[j] / slicesize;
					if (quantum != prev_quantum) {
						/*if (funcend < (page * pagesize))*/
							/*totalfuncsize += funcsize;*/
						/*else*/
							/*totalfuncsize += (page * pagesize) - funcaddr;*/
						totalfuncsize += funcsize;
						if (!hits[quantum]) {
							hits[quantum] = 1;
							totalpagesize += pagesize;
						}
					}
					prev_quantum = quantum;
				}
			}
			if (funcend >= (page * pagesize)) {
				if (totalpagesize > prevpagesize)
					Lrtsyms[i]->efficiency = 1000 * (totalfuncsize - prevfuncsize) / (totalpagesize - prevpagesize);
				prevfuncsize = totalfuncsize;
				prevpagesize = totalpagesize;
				memset(hits, '\0', sizeof(unchar) * (Timerec / slicesize + 1));
				break;
			}
			else {
				if (totalpagesize > prevpagesize)
					Lrtsyms[i]->efficiency = 1000 * (totalfuncsize - prevfuncsize) / (((totalpagesize - prevpagesize) / pagesize) * (funcend % pagesize));
			}
		}
	}
	free(hits);
	*working_setp = totalpagesize / Pagesize / (Timerec / slicesize + 1);
	return((ulong) (1000 * totalfuncsize / totalpagesize));
}


timestamp(FILE *fp)
{
	time_t timeval;

	time(&timeval);
	fprintf(fp, ctime(&timeval));
}

static void
stability(int cnt, char *str, int addrbased)
{
	int size, pct;
	int i;
	ulong wset;

	for (size = TestSize - cnt * 100; size <= TestSize + cnt * 100; size += 100) {
		pct = metric(size, addrbased, Pagesize, &wset);
		printf("-- %s - %d ------ Percentage = %d.%d\n", str, size, pct / 10, pct % 10);
	}
	fprintf(stderr, "\n");
}

static void
readlogs(char *objectname, char **files)
{
	char *p;
	void *id, *objid, *rec;
	struct sym *sym;
	struct lrtsym *lrtsym;
	int i, ret;
	int pct;
	char *usefiles[2];

	usefiles[1] = NULL;
	rec = (void *) fp_record_alloc();
	Nsyms = 0;
	Nstack = 0;
	pushsym(NULL);
	Timerec = 0;
	/*fp_rewind_record(id);*/
	objid = NULL;

	for (i = 0; files[i]; i++) {
		usefiles[0] = files[i];
		if (!(id = fp_open(usefiles, FPROF_SEPARATE_EXPERIMENTS))) {
			fp_error();
			continue;
		}
		MaxRecords += fp_max_records(id) + max(ComputeSize, TestSize);
		fp_close(id);
	}
	for (i = 0; files[i]; i++) {
		usefiles[0] = files[i];
		if (!(id = fp_open(usefiles, FPROF_SEPARATE_EXPERIMENTS)))
			continue;
		while ((ret = fp_next_record(id, rec)) != FPROF_EOF) {
			void *tmp;

			if (!fp_record_to_symbol(rec)) {
				fprintf(stderr, "Skipping record with no symbol mapping\n");
				continue;
			}
			if (!objid) {
				if (strcmp(fp_symbol_to_object_name(fp_record_to_symbol(rec)), objectname) == 0)
					objid = fp_symbol_to_object(fp_record_to_symbol(rec));
				else if (p = strrchr(fp_symbol_to_object_name(fp_record_to_symbol(rec)), '/')) {
					if (strcmp(p + 1, objectname) == 0)
						objid = fp_symbol_to_object(fp_record_to_symbol(rec));
				}
				if (objid)
					fprintf(stderr, "Found Object\n");
			}

			if (ret == FPROF_EOL) {
				Timerec += max(ComputeSize, TestSize);
				continue;
			}
			if ((tmp = fp_record_to_object(rec)) == objid)
				lrtsym = savesym(fp_record_to_symbol(rec));
			else
				lrtsym = &Dummy;
			if (fp_record_is_prologue(rec)) {
				pushsym(lrtsym);
				if (lrtsym != &Dummy) {
					/*if (Debug)*/
						/*printf("Calling %s\n", fp_symbol_to_name(lrtsym->sym));*/
					addtime_to_sym(lrtsym, Timerec);
					Timerec++;
				}
			}
			else {
				if (!Nstack) {
					fprintf(stderr, "Stack Underflow\n");
					fprintf(stderr, "Epilogue without prologue: %s\n", fp_symbol_to_name(lrtsym->sym));
					continue;
				}
				else if (lrtsym != Stack[Nstack-1]) {
					fprintf(stderr, "Epilogue without prologue: %s\n", fp_symbol_to_name(lrtsym->sym));
					fprintf(stderr, "Prologue Function was: %s\n", fp_symbol_to_name(Stack[Nstack-1]->sym));
				}
				/*else if (Debug)*/
					/*printf("Epilogue function was: %s\n", fp_symbol_to_name(lrtsym->sym));*/
				if (Nstack > 0)
					popsym(); /* pop off top function */
				/* new top of call stack */
				if (Nstack > 0) {
					if ((lrtsym = popsym()) && (lrtsym != &Dummy)) {
						/*if (Debug)*/
							/*printf("Returning to %s\n", fp_symbol_to_name(lrtsym->sym));*/
						addtime_to_sym(lrtsym, Timerec);
						Timerec++;
					}
					pushsym(lrtsym);
				}
			}
		}
		fp_close(id);
		while (Nstack && (lrtsym = popsym()) && (lrtsym != &Dummy)) {
			addtime_to_sym(lrtsym, Timerec);
			Timerec++;
		}
	}

	Nquanta = Timerec / ComputeSize + 1;
}

static int
bestcomp(void *sym1, void *sym2)
{
	return(((struct lrtsym **) sym1)[0]->bestorder - ((struct lrtsym **) sym2)[0]->bestorder);
}

main(argc, argv)
int	argc;
char	*argv[];
{
	extern int optind;
	extern char *optarg;
	int c;
	int i = 1, besti, j;
	int pct, bestpct;
	struct seed useseeds[2], *seeds;

	Debug = (int) getenv("Debug");
	Debug2 = (int) getenv("Debug2");
	if (getenv("TestSize"))
		TestSize = atoi(getenv("TestSize"));
	if (getenv("ComputeSize"))
		ComputeSize = atoi(getenv("ComputeSize"));
	if (getenv("CompareDistance"))
		CompareDistance = atoi(getenv("CompareDistance"));
	if (getenv("SelfDistance"))
		SelfDistance = atoi(getenv("SelfDistance"));
	readlogs(argv[i], argv + i + 1);

	if (Timerec == max(ComputeSize, TestSize)) {
		fprintf(stderr, "Object not found\n");
		exit(1);
	}
	bestpct = 0;
	besti = -1;
	useseeds[1].func = NULL;
	useseeds[0].name = "Standard";
	for (i = 0; Algos[i].name; i++) {
		if (Algos[i].flags & ALLSEEDS)
			seeds = Allseeds;
		else {
			seeds = useseeds;
			seeds[0].flags = Algos[i].flags;
			seeds[0].func = Algos[i].func;
		}
		j = 0;
		do {
			ulong wset;

			if (seeds[j].func) {
				fprintf(stderr, "Seeding with %s\n", seeds[j].name);
				seeds[j].func();
				if (seeds[j].flags & REVERSE_SEED)
					reverse();
			}
			fprintf(stderr, "Trying Algorithm %s\n", Algos[i].name);
			timestamp(stderr);
			Algos[i].func(Algos[i].opt1, Algos[i].opt2);
			pct = qmetric(TestSize, Algos[i].flags & ADDRBASED, Pagesize, &wset);
			/*pct = metric(TestSize, Algos[i].flags & ADDRBASED, Pagesize, &wset);*/
			fprintf(stderr, "Average Working Set: %d.%d\nPercentage: %d.%d\n", wset / 10, wset % 10, pct / 10, pct % 10);
			if (pct > bestpct) {
				int j;

				fprintf(stderr, "Best\n");
				besti = i;
				bestpct = pct;
				for (j = 0; j < Nsyms; j++)
					Lrtsyms[j]->bestorder = j;
			}
			if (Algos[i].flags & SCATTERGRAM)
				scattergram(stderr, Pagesize);
			if (Algos[i].flags & SUMCHART)
				sum_chart(stderr, Pagesize);
		} while(j++, seeds[j].func);
	}
	timestamp(stderr);

	fprintf(stderr, "Using order from %s\n", Algos[besti].name);
	qsort(Lrtsyms, Nsyms, sizeof(struct lrtsym *), bestcomp);
	output();
}
