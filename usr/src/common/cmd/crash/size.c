#ident	"@(#)crash:common/cmd/crash/size.c	1.1.2.2"

/*
 * This file contains code for the crash functions size and addstruct.
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "crash.h"

#include "asoffs.h"
#include "bdevswoffs.h"
#include "bufoffs.h"
#include "cdevswoffs.h"
#include "credoffs.h"
#include "databoffs.h"
#include "engineoffs.h"
#include "exdataoffs.h"
#include "execinfooffs.h"
#include "fifonodeoffs.h"
#include "fileoffs.h"
#include "filockoffs.h"
#include "linkblkoffs.h"
#include "lwpoffs.h"
#include "metsoffs.h"
#include "modctloffs.h"
#include "modctl_listoffs.h"
#include "modobjoffs.h"
#include "moduleoffs.h"
#include "msgboffs.h"
#include "pageoffs.h"
#include "pidoffs.h"
#include "plocaloffs.h"
#include "plocalmetoffs.h"
#include "procoffs.h"
#include "queueoffs.h"
#include "runqueoffs.h"
#include "snodeoffs.h"
#include "stdataoffs.h"
#include "strttyoffs.h"
#include "useroffs.h"
#include "vfsoffs.h"
#include "vfsswoffs.h"
#include "vnodeoffs.h"

/*
 * Each of these tables has some reason for being here, but there
 * are probably much stronger reasons why others should be added.
 */

struct offstable *loc_offstab[] = {
	asoffs,
	bdevswoffs,
	bufoffs,
	cdevswoffs,
	credoffs,
	databoffs,
	engineoffs,
	exdataoffs,
	execinfooffs,
	fifonodeoffs,
	fileoffs,
	filockoffs,
	linkblkoffs,
	lwpoffs,
	metsoffs,
	modctloffs,
	modctl_listoffs,
	modobjoffs,
	moduleoffs,
	msgboffs,
	pageoffs,
	pidoffs,
	plocaloffs,
	plocalmetoffs,
	procoffs,
	queueoffs,
	runqueoffs,
	snodeoffs,
	stdataoffs,
	strttyoffs,
	useroffs,
	vfsoffs,
	vfsswoffs,
	vnodeoffs,
	NULL,
};
static size_t tablesize = sizeof(loc_offstab[0]);   /* assumed initial value */
static struct offstable **offstables = NULL;	    /* assumed initial value */

/* get arguments for size function */
getsize()
{
	int c;
	int full = 0;

	optind = 1;
	while((c = getopt(argcnt,args,"fw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'f' :	full = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		do{
			prsize(args[optind++],full);
		}while(args[optind]);
	}
	else prsize(NULL,full);
}

/* print size */
prsize(name,full)
char *name;
int full;
{
	struct offstable *tp, **tpp;
	char format[] = "%4x %s";
	int i, n;

	for (i = n = 0, tpp = offstables; (tp = *tpp); tpp++) {
		if (name && strcmp(name, tp->name) != 0)
			continue;
		n += fprintf(fp,format,tp->offset,tp->name);
		i += 20;
		if (full) {
			putc(':',fp);
			while (++n < i)
				putc(' ',fp);
			while ((++tp)->name) {
				n += fprintf(fp,format,tp->offset,tp->name);
				if ((i += 20) == 80) {
					putc('\n',fp);
					i = n = 0;
				}
				else if (n >= i
				&& ((tp+1)->offset & 0xfffff000))
					putc(' ',fp);
				else while (n < i) {
					putc(' ',fp);
					++n;
				}
			}
			if (i != 0) {
				putc('\n',fp);
				i = n = 0;
			}
			if (!name)
				putc('\n',fp);
		}
		else {
			if (name || i == 80) {
				putc('\n',fp);
				i = n = 0;
			}
			else while (n < i) {
				putc(' ',fp);
				++n;
			}
		}
		if (name)
			return;
	}
	if (name)
		error("%s not found in structure table\n", name);
	if (i != 0)
		putc('\n',fp);
}

/* get arguments for add structure function */
getaddstruct()
{
	int c;
	char *structname, *hdrfile, *cp;
	char buf[LINESIZE+40];
	extern char lib_dir[];
	char *outfile = "";

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	/* don't redirect() */
					outfile = optarg;
					break;
			default  :	longjmp(syn,0);
		}
	}

	if(*outfile) {
		cp = strrchr(outfile, '.');
		if (cp == NULL || strcmp(cp, ".so") != 0)
			error("outfile %s must have suffix .so\n",outfile);
	}
	if(args[optind])
		structname = args[optind++];
	if(!args[optind])
		longjmp(syn,0);
	hdrfile = args[optind];
	while (args[++optind]) {
		cp = hdrfile + strlen(hdrfile);
		while (cp < args[optind])
			*cp++ = ' ';
	}

	sprintf(buf, "sh %s/addstruct %s \"%s\" %s 2>&1\n",
		lib_dir, structname, hdrfile, outfile);
	prerrmes("  ... please wait ...\n");
	if (system(buf) != 0)
		error("failed to add %s structure offsets\n", structname);
	if (!*outfile)
		sprintf(outfile = buf, "%s/%soffs.so", lib_dir, structname);
	add_shlib(outfile, NULL, NULL);
	prsize(structname, 1);
}

/* allocate expandable table */
void
addoffstab(struct offstable **addoffstables)
{
	struct offstable *tp, *atp;
	struct offstable **tpp, **atpp;
	size_t size;
	int cmp;

	for (atpp = addoffstables, size = 0;
		*atpp; atpp++, size += sizeof(*atpp))
		;
	if (offstables == loc_offstab
	|| (tpp = realloc(offstables, tablesize + size)) == NULL) {
		prerrmes("cannot extend structure table\n");
		if (offstables)
			return;
		offstables = loc_offstab; /* fallback: hope it's sorted */
	}
	else {
		if (!offstables)
			*tpp = NULL;
		offstables = tpp;
	}
	for (atpp = addoffstables; (atp = *atpp); atpp++) {
		cmp = 1;
		for (tpp = offstables; (tp = *tpp); tpp++) {
			if ((cmp = strcmp(atp->name, tp->name)) <= 0)
				break;
		}
		if (cmp != 0) {
			memmove(tpp+1, tpp, tablesize
				- (size_t)tpp + (size_t)offstables);
			tablesize += sizeof(*tpp);
		}
		*tpp = atp;
	}
}

static struct offstable *
locate(register struct offstable *tp, register char *name)
{
	while ((++tp)->name != NULL) {
		if (strcmp(tp->name, name) == 0)
			return tp;
	}
	fatal("%s not found in structure table\n", name);
}

/* make size/offset adjustments for UniProc or different release */
void
adjustoffstab(void)
{
	register struct offstable *tp, *etp;
	char *name;
	int size;

	/*
	 * Let this -UUNIPROC crash report correctly on -DUNIPROC
	 */
	if (UniProc) {
		tp = locate(engineoffs, "e_lastpick");
		tp->name = (tp+1)->name;		/* e_rqlist */
		tp++;
		tp->name = (tp+1)->name;		/* e_smodtime */
		tp++;
		tp->name = NULL;
		engineoffs->offset = tp->offset -= 4;	/* size */

		tp = locate(runqueoffs, "rq_lastnudge");
		tp->name = (tp+1)->name;		/* rq_nudgelist */
		tp++;
		tp->name = NULL;
		runqueoffs->offset = tp->offset;	/* size */
	}

	/*
	 * There are useful fields hidden in these fixed structures:
	 * if space remains in the main symbol table, add them in there;
	 * each offs.h has been specially edited with prefix to fieldnames.
	 */
	addsymfields("l", plocaloffs);
	addsymfields("lm", plocalmetoffs);
	addsymfields("m", metsoffs);
}

struct offstable *
structfind(char *struc, char *field)
{
	static struct offstable dummy;
	struct offstable **tpp, *tp, *foundtp;
	long size, offs;
	int fieldfound;
	char *ptr;

	/*
	 * Allow hexadecimal for struc or field, up to this limit
	 */
#define MAXSIZE 0x10000
	if (*struc) {
		size = (int)strtoul(struc, &ptr, 16);
		if (size <= 0 || size > MAXSIZE || *ptr != '\0')
			size = MAXSIZE;
		else if (*field)
			struc = "";
		else {
			dummy.offset = size;
			return &dummy;
		}
	}
	else
		size = MAXSIZE;
	if (*field) {
		offs = (int)strtoul(field, &ptr, 16);
		if (offs < 0 || offs >= MAXSIZE || *ptr != '\0')
			offs = MAXSIZE;
		else if (*struc)
			field = "";
		else {
			if (offs >= size)
				goto rangerr;
			dummy.offset = offs;
			return &dummy;
		}
	}
	else
		offs = MAXSIZE;

	fieldfound = 0;
	for (tpp = offstables; (tp = *tpp); tpp++) {
		if (*struc) {
			if (*(short *)struc != *(short *)tp->name
			||  strcmp(struc, tp->name) != 0)
				continue;
			size = tp->offset;
			if (offs < MAXSIZE) {
				if (offs >= size)
					goto rangerr;
				/*
				 * We could seek to the appropriate field,
				 * but parsefieldspec() wants numeric offset
				 * to switch fieldnames off and numerics on
				 */
				dummy.offset = offs;
				return &dummy;
			}
			if (!*field)
				return tp;
		}
		if (*field) {
			while ((++tp)->name) {
				if (*(short *)field == *(short *)tp->name
				&&  strcmp(field, tp->name) == 0) {
					fieldfound++;
					foundtp = tp;
					ptr = (*tpp)->name;
					break;
				}
			}
			if (*struc)
				break;
		}
	}
	if (fieldfound > 1)
	    error("same fieldname in %d structures: determine by e.g. %s:%s\n",
			fieldfound, ptr, field);
	if (fieldfound == 1) {
		if (!*struc && (offs = foundtp->offset) >= size)
			goto rangerr;
		return foundtp;
	}
	error("%s%s%s not found in structure table\n",
		struc, (*struc && *field)? ":":"", field);
rangerr:
	error("%x is out of range 0-%x\n", offs, size - 1);
}
