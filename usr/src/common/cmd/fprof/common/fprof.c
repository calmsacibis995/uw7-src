/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved. */
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved. */
/*	  All Rights Reserved */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc. */
/*	The copyright notice above does not evidence any */
/*	actual or intended publication of such source code. */

#ident	"@(#)fprof:common/fprof.c	1.6"

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <limits.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <varargs.h>
#include "fprof.h"

void *Ret;

#define MALLOC(X) ((Ret = (void *) malloc(X)) ? Ret : crash())
#define REALLOC(S, X) ((Ret = (void *) realloc(S, X)) ? Ret : crash())
#define CALLOC(S, X) ((Ret = (void *) calloc(S, X)) ? Ret : crash())

struct fp **Fps;
static int Nfps = 0;

unsigned int fp_errno;

char *(*fp_getfunc)(const char *) = getenv;
int (*fp_setfunc)(char *) = putenv;
void (*fp_prfunc)() = (void (*)()) printf;
static void (*fp_fprfunc)() = (void (*)()) fprintf;
static int fp_inxksh = 0;

static struct object **Objects;
static int Nobjects = 0;

static void fp_rewind_file(struct fp *fptr, int fileno);

/*
** Not done, yet
*/
fp_error()
{
	fprintf(stderr, "There is an error\n");
}

void
errprintf(va_alist)
va_dcl
{
	va_list ap;
	char *fmt;
	ulong arg1, arg2;

	va_start(ap);
	fmt = va_arg(ap, char *);
	arg1 = va_arg(ap, ulong);
	arg2 = va_arg(ap, ulong);
	if (fp_inxksh)
		(*fp_fprfunc)(2, fmt, arg1, arg2);
	else
		(*fp_fprfunc)(stderr, fmt, arg1, arg2);
	va_end(ap);
}

static void *
crash()
{
	errprintf("Out of memory\n");
	_exit(1);
}

/*
** Compare two symbols for lower address
*/
static int
symcomp(const void *sym1, const void *sym2)
{
	return(((struct sym *) sym1)->addr - ((struct sym *) sym2)->addr);
}

/*
** If the file needs to be compiled, open up cfp to a temp name (in
** the same fs)
*/
static void
compile_start(struct fp *fptr, int fileno, char *header)
{
	char buf[BUFSIZ], *tmp;

	sprintf(buf, "%s/%s%d", dirname(tmp = strdup(fptr->fileinfo[fileno].name)), ".tmp_play", fileno);
	free(tmp);
	fptr->fileinfo[fileno].cfp = fopen(buf, "w");
	fwrite(header, 1, HEADER_SIZE, fptr->fileinfo[fileno].cfp);
}

static int
read_header(struct fp *fptr, int fileno, int cannot_compile)
{
	char buf[HEADER_SIZE];
	char *comp_ptr = NULL;
	char *p, *q;

	if (fptr->fileinfo[fileno].l.curoffset != 0)
		fp_rewind_file(fptr, fileno);
	
	if (!do_fread(buf, HEADER_SIZE, 1, fptr, fileno)) {
		fp_errno = FPROF_ERROR_CANNOT_READ;
		return(0);
	}

	if (strncmp(buf, "flow\n", 5)) {
		fp_errno = FPROF_ERROR_NOT_FPROF_LOG;
		return(0);
	}
	for (p = buf + 5; *p; p = strchr(q, '\n') + 1) {
		if (!(q = strchr(p, '=')))
			continue;
		*q++ = '\0';
		if (strcmp(p, "Compiled") == 0) {
			comp_ptr = q;
			if (atoi(q))
				fptr->fileinfo[fileno].l.flags |= FPROF_COMPILED;
			else
				fptr->fileinfo[fileno].l.flags &= ~(FPROF_COMPILED);
		}
		else if (strcmp(p, "Version") == 0)
			fptr->fileinfo[fileno].version = atoi(q);
		else if (strcmp(p, "AccurateTimeStamp") == 0) {
			if (atoi(q))
				fptr->fileinfo[fileno].stamp_overhead = 6;
			else
				fptr->fileinfo[fileno].stamp_overhead = 0;
		}
		else if (strcmp(p, "Control") == 0)
			fptr->fileinfo[fileno].control_address = strtoul(q, NULL, 16);
		else if (strcmp(p, "Pid") == 0)
			fptr->fileinfo[fileno].pid = strtoul(q, NULL, 16);
		else if (strcmp(p, "Node") == 0) {
			char *r;

			if (r = strchr(q, '\n'))
				*r = '\0';
			fptr->fileinfo[fileno].node = strdup(q);
			if (r)
				*r = '\n';
		}
		else if (strcmp(p, "Time") == 0)
			fptr->fileinfo[fileno].time = strtoul(q, NULL, 16);
		else if (strcmp(p, "Map") == 0) {
			if (fseek(fptr->fileinfo[fileno].fp, strtoul(q, NULL, 16) + 3 * sizeof(ulong), SEEK_SET) != -1)
				if (!readmap(fptr, fileno))
					return(0);
		}
		q[-1] = '=';
	}
	if (!fptr->fileinfo[fileno].nobjects)
		return(0);
	if (!(fptr->fileinfo[fileno].l.flags & FPROF_COMPILED) && !cannot_compile) {
		if (comp_ptr)
			*comp_ptr = '1';
		compile_start(fptr, fptr->nfps, buf);
	}
	fseek(fptr->fileinfo[fileno].fp, HEADER_SIZE, SEEK_SET);
	return(1);
}

/*
** Read in symbol table using nm(1).  Of course, this could be
** written directly to libelf, but why bother?  It's not significantly
** faster . . .  Anyway, read in lines, taking only those that are
** FUNC types and have non-zero size.
*/
static int
getsyms(struct object *objid)
{
	FILE *pfp;
	char curfile[BUFSIZ];
	char *binding;
	char *type;
	char buf[BUFSIZ];
	char buf2[BUFSIZ];
	struct sym *syms;
	char *addr;
	ulong size;

	curfile[0] = '\0';
	syms = NULL;
	objid->nsyms = 0;
	if (!objid->foundpath) {
		errprintf("Cannot find object: %s\n", objid->name);
		return(0);
	}
	sprintf(buf, "/usr/bin/nm %s 2>/dev/null", objid->foundpath);
	if (!(pfp = popen(buf, "r"))) {
		sprintf(buf, "Opening %s", objid->name);
		perror(buf);
		exit(1);
	}
	while (fgets(buf, BUFSIZ, pfp)) {
		strtok(buf, "|");
		addr = (char *) strtoul(strtok(NULL, "|"), NULL, 10);
		/*if ((addr < objid->start_addr) || (addr > objid->end_addr))*/
			/*addr += (ulong) objid->start_addr;*/
		size = strtoul(strtok(NULL, "|"), NULL, 10);
		if (strncmp((type = strtok(NULL, "|")), "FILE", 4) == 0) {
			strtok(NULL, "|");
			strtok(NULL, "|");
			strtok(NULL, "|");
			strcpy(curfile, strtok(NULL, "|\n"));
			continue;
		}
		if (!size || strncmp(type, "FUNC", 4))
			continue;
		binding = strtok(NULL, "|");
		strtok(NULL, "|");
		if (!atoi(strtok(NULL, "|")))
			continue;
		if (!(objid->nsyms % 50))
			syms = (struct sym *) REALLOC(syms, (objid->nsyms + 50) * sizeof(struct sym));
		syms[objid->nsyms].addr = addr;
		syms[objid->nsyms].size = size;
		syms[objid->nsyms].object = objid;
		syms[objid->nsyms].ptr = NULL;
		if (strncmp(binding, "LOCL", 4) == 0) {
			sprintf(buf2, "%s@%s", curfile, strtok(NULL, "|\n"));
			syms[objid->nsyms].name = strdup(buf2);
		}
		else
			syms[objid->nsyms].name = strdup(strtok(NULL, "|\n"));
		(objid->nsyms)++;
	}
	if ((pclose(pfp) != 0) || !objid->nsyms) {
		errprintf("Error reading symbols from %s\n", objid->foundpath);
		return(0);
	}
	qsort(syms, objid->nsyms, sizeof(struct sym), symcomp);
	objid->symtab = syms;
	return(1);
}

/*
** If done is set, close off the compiled file and rename it to the
** original.  Otherwise, blow away the compilation file
*/ 
static void
compile_end(struct fp *fptr, int fileno, int done)
{
	if (fptr->fileinfo[fileno].cfp) {
		char buf[BUFSIZ], *tmp;

		fflush(fptr->fileinfo[fileno].cfp);
		sprintf(buf, "%s/%s%d", dirname(tmp = strdup(fptr->fileinfo[fileno].name)), ".tmp_play", fileno);
		free(tmp);
		if (done) {
			fclose(fptr->fileinfo[fileno].fp);
			fptr->fileinfo[fileno].fp = fptr->fileinfo[fileno].cfp;
			fptr->fileinfo[fileno].cfp = NULL;
			unlink(fptr->fileinfo[fileno].name);
			rename(buf, fptr->fileinfo[fileno].name);
		}
		else {
			unlink(buf);
			fclose(fptr->fileinfo[fileno].cfp);
			fptr->fileinfo[fileno].cfp = NULL;
		}
	}
}

/*
** Free up and close everything (remove temp files)
*/
static void
allclose()
{
	int i;

	for (i = 0; i < Nfps; i++)
		if (Fps[i])
			fp_close(Fps[i]);
}

/*
** Open a set of flow profile logs.
**
*/
void *
fp_open(char **files, int flags)
{
	struct logent logent;
	struct stat statbuf;
	struct fp *fptr;
	char buf[BUFSIZ];
	int i;


	for (i = 0; i < Nfps; i++)
		if (!Fps[i])
			break;

	if (i == Nfps) {
		Nfps++;
		Fps = (struct fp **) REALLOC(Fps, Nfps * sizeof(struct fp *));
	}
	fptr = Fps[i] = (struct fp *) MALLOC(sizeof(struct fp));
	memset(fptr, '\0', sizeof(struct fp));
	fptr->flags = flags;
	fptr->curobject = -1;

	for (i = 0; files[i]; i++) {
		int fd, cannot_compile;

		fptr->fileinfo = (struct perfile *) REALLOC(fptr->fileinfo, (fptr->nfps+1)*sizeof(struct perfile));
		memset(fptr->fileinfo + fptr->nfps, '\0', sizeof(struct perfile));
		fptr->fileinfo[fptr->nfps].name = strdup(files[i]);
		fptr->fileinfo[fptr->nfps].l.flags = 0;
		if ((fd = open(files[i], O_RDWR)) < 0) {
			sprintf(buf, "Error opening %s", files[i]);
			perror(buf);
			exit(1);
		}
		if (lockf(fd, F_TLOCK, HEADER_SIZE) < 0) {
			fprintf(stderr, "Cannot compile, file is busy\n");
			cannot_compile = 1;
		}
		else
			cannot_compile = 0;
		if (!(fptr->fileinfo[fptr->nfps].fp = fdopen(fd, "r"))) {
			sprintf(buf, "Error opening %s", files[i]);
			perror(buf);
			exit(1);
		}
		if (fstat(fd, &statbuf) < 0) {
			sprintf(buf, "Error statting %s", files[i]);
			perror(buf);
			exit(1);
		}
		fptr->max_records += statbuf.st_size / (2 * sizeof(long));
		if (!read_header(fptr, fptr->nfps, cannot_compile)) {
			errprintf("%s is not readable or is not a flow-profile log\n", files[i]);
			fclose(fptr->fileinfo[fptr->nfps].fp);
		}
		else {
			fptr->fileinfo[fptr->nfps].l.curoffset = HEADER_SIZE;
			fptr->nfps++;
		}
	}
	fptr->prev_time = 0;
	if (!(flags & FPROF_SEPARATE_EXPERIMENTS))
		fptr->curfp = -1;
	atexit(allclose);
	return(fptr);
}

/*
** Clear all marks.  Also frees up memory associated with the marks
*/
void
fp_clear_marks(struct fp *fptr)
{
	int i;

	if (fptr->marks) {
		i = 0;
		do {
			free(fptr->marks[i]);
			i += 10;
		} while (i < fptr->nmarks);
		free(fptr->marks);
		free(fptr->marktimes);
		fptr->marks = NULL;
		fptr->marktimes = NULL;
		fptr->nmarks = 0;
	}
}

/*
** Close the fptr.  Notice that this does not free up the Objects.
** Nothing cleans that up except exiting.
*/
void
fp_close(struct fp *fptr)
{
	int i;

	for (i = 0; i < fptr->nfps; i++)
		compile_end(fptr, i, 0);

	fp_clear_marks(fptr);

	for (fptr->nfps--; fptr->nfps >= 0; fptr->nfps--) {
		fclose(fptr->fileinfo[fptr->nfps].fp);
		free(fptr->fileinfo[fptr->nfps].name);
		if (fptr->fileinfo[fptr->nfps].node)
			free(fptr->fileinfo[fptr->nfps].node);
		fptr->fileinfo[fptr->nfps].fp = NULL;
	}
	fptr->nfps = 0;
	free(fptr->fileinfo);
	fptr->fileinfo = NULL;
	fptr->nfps = 0;
}


/*
** Return next entry in object array
void *
fp_next_object(struct fp *fptr)
{
	int fileno;

	if (++fptr->curobject == fptr->fileinfo[fileno].nobjects)
		return(0);
	return((void *) (fptr->fileinfo[fileno].objects[fptr->curobject]->o));
}
*/

/*
** Go back to first object
void
fp_rewind_object(struct fp *fptr)
{
	fptr->curobject = -1;
}
*/

/*
** Search comparison routine.  Is addr of one within the other
*/
static
symcomp2(const void *sym1, const void *sym2)
{
	if (((struct sym *) sym1)->addr < ((struct sym *) sym2)->addr) {
		if ((((struct sym *) sym1)->addr + ((struct sym *) sym1)->size) < ((struct sym *) sym2)->addr)
			return(-1);
		return(0);
	}
	if (((struct sym *) sym2)->addr < ((struct sym *) sym1)->addr) {
		if ((((struct sym *) sym2)->addr + ((struct sym *) sym2)->size) < ((struct sym *) sym1)->addr)
			return(+1);
		return(0);
	}
	return(0);
}

/*
** Set mark criterion
*/
void
fp_set_mark(struct fp *fptr, char *criterion)
{
	fptr->mark_criterion = criterion ? regcmp(criterion, 0) : NULL;
}

/*
** Search for symbol in objects of this fptr.  Since we are using
** lazy evaluation, first make sure that the object has been read off
** disk (getsyms).
*/
static struct sym *
findsym(struct fp *fptr, void *addr, int fileno)
{
	int i, j;
	struct sym sym;
	int besti;
	void *bestaddr;

	if (fptr->fileinfo[fileno].l.flags & FPROF_COMPILED) {
		i = ((ulong) addr) / (1 << 24) - 1;
		if (!fptr->fileinfo[fileno].objects[i]->o->symtab) {
			if (fptr->fileinfo[fileno].objects[i]->o->flags & OBJECT_MISSING)
				return(NULL);
			if (!getsyms(fptr->fileinfo[fileno].objects[i]->o))
				return(NULL);
		}
		if ((((ulong) addr) % (1 << 24)) > fptr->fileinfo[fileno].objects[i]->o->nsyms)
			return(NULL);
		return(fptr->fileinfo[fileno].objects[i]->o->symtab + ((ulong) addr) % (1 << 24));
	}
	sym.addr = addr;
	besti = 0;
	bestaddr = fptr->fileinfo[fileno].objects[0]->start_addr;
	for (i = 1; i < fptr->fileinfo[fileno].nobjects; i++) {
		if ((fptr->fileinfo[fileno].objects[i]->start_addr <= addr) && (bestaddr < fptr->fileinfo[fileno].objects[i]->start_addr)) {
			bestaddr = fptr->fileinfo[fileno].objects[i]->start_addr;
			besti = i;
		}
	}
	fptr->curobj = besti;
	if (!fptr->fileinfo[fileno].objects[besti]->o->symtab)
		if (!getsyms(fptr->fileinfo[fileno].objects[besti]->o))
			return(NULL);

	sym.addr -= (ulong) fptr->fileinfo[fileno].objects[besti]->map_addr;
	sym.size = 0;
	return((struct sym *) bsearch(&sym, fptr->fileinfo[fileno].objects[besti]->o->symtab, fptr->fileinfo[fileno].objects[besti]->o->nsyms, sizeof(struct sym), symcomp2));
}

/*
** Time entries have 3 parts:  the high-bit either implies PROLOGUE
** or epilogue, the 16 lower bits is a count of how far the counter
** has progressed toward CLKNUM (the number of counts until lbolt)
** changes and the other 15 bits are the lower 15 bits of lbolt.
**
** There are 2 difficult parts to this code:  lbolt wrapping and
** jumping in before the clock.  See comments below.
*/
static
long_to_time(struct location *loc, ulong l)
{
	ulong lbolt, leftover;

	lbolt = (loc->prevlbolt & ~(0x1ffff)) | ((l >> 14) & 0x1ffff);

	/*
	** Since we are only using 17 bits for lbolt, it can wrap.
	** So, just add in the next higher bit and keep track that you
	** did.
	*/
	if (lbolt < loc->prevlbolt)
		lbolt += 1 << 17;

	leftover = l & 0x3fff;

	/*
	** Sometimes, we can jump in before the kernel's clock interrupt
	** and, therefore, will notice that the clock should have flipped
	** before it does.
	**
	** Notice that we do not store this altered lbolt value!
	** This is because 2 consecutive calls could get in before the
	** kernel and we don't want lbolt to look overincremented.
	** So, just use the last one until we get a synchronized value.
	*/
	if ((lbolt == loc->prevlbolt) && (leftover > loc->prevleft))
		lbolt++;
	else {
		loc->prevlbolt = lbolt;
		loc->prevleft = leftover;
	}
	return((lbolt + 1)*10000 - (((uint) leftover * 10000) / (CLKNUM)));
}

/*
** This can be switched to mmap() for performance reasons (however,
** we can't mmap() in the whole file because this will make the
** virtual address space too large).
*/
static int
do_fread(void *buf, size_t size, size_t n, struct fp *fptr, int fileno)
{
	int k;

	if ((k = fread(buf, size, n, fptr->fileinfo[fileno].fp)) < n)
		return(0);

	fptr->fileinfo[fileno].l.curoffset += n * size;
	if (fptr->fileinfo[fileno].l.curoffset > fptr->fileinfo[fileno].lastoffset_visited)
		fptr->fileinfo[fileno].lastoffset_visited = fptr->fileinfo[fileno].l.curoffset;
	return(1);
}

/*
** Only used for writing back to compiled log
*/
static void
do_fwrite(void *buf, size_t size, size_t n, struct fp *fptr, int fileno)
{
	if (fptr->fileinfo[fileno].cfp && (fptr->fileinfo[fileno].l.curoffset > fptr->fileinfo[fileno].lastoffset_compiled)) {
		fwrite(buf, size, n, fptr->fileinfo[fileno].cfp);
		fptr->fileinfo[fileno].lastoffset_compiled = fptr->fileinfo[fileno].l.curoffset;
	}
}

/*
** Convenience routine
*/
static int
do_read_write(void *buf, size_t size, size_t n, struct fp *fptr, int fileno)
{
	if (!do_fread(buf, size, n, fptr, fileno))
		return(0);

	do_fwrite(buf, size, n, fptr, fileno);
	return(1);
}

static int
checkobject(const char *path, uint size)
{
	struct stat st;

	if (stat(path, &st) == -1)
		return(0);

	if (!size)
		return(1);

	return(size == st.st_size);
}

/*
** The format of a map record is:  null terminated string followed by
** start address.  The first zero-length string encountered terminates
** the map.
*/
static int
readmap(struct fp *fptr, int fileno)
{
	char lib[PATH_MAX];
	char *p;
	void *start;
	ulong size;
	int i, j;

	for (i = 0; i < fptr->fileinfo[fileno].nobjects; i++)
		fptr->fileinfo[fileno].objects[i]->flags &= ~OBJECT_VISITED;

	while (1) {
		i = 0;
		while (((lib[i] = getc(fptr->fileinfo[fileno].fp)) != (char) EOF) && lib[i++])
			;
		if (i == 0)
			return(0);

		for ( ; i % sizeof(long); i++)
			getc(fptr->fileinfo[fileno].fp);

		if (!lib[0])
			break;
		if (fread(&start, 1, sizeof(ulong), fptr->fileinfo[fileno].fp) != sizeof(ulong))
			return(0);

		if (fread(&size, 1, sizeof(ulong), fptr->fileinfo[fileno].fp) != sizeof(ulong))
			return(0);

		start = (void *) ((ulong) start & ~0xfff);
		if ((fptr->fileinfo[fileno].l.curoffset < fptr->fileinfo[fileno].lastoffset_visited))
			continue;

		for (i = 0; i < fptr->fileinfo[fileno].nobjects; i++)
			if (strcmp(fptr->fileinfo[fileno].objects[i]->o->name, lib) == 0)
				break;

		if (i == fptr->fileinfo[fileno].nobjects) {
			fptr->fileinfo[fileno].nobjects++;
			fptr->fileinfo[fileno].objects = (struct object_plus **) REALLOC(fptr->fileinfo[fileno].objects, fptr->fileinfo[fileno].nobjects * sizeof(struct object_plus *));
			fptr->fileinfo[fileno].objects[i] = (struct object_plus *) MALLOC(sizeof(struct object_plus));
			for (j = 0; j < Nobjects; j++)
				if (strcmp(Objects[j]->name, lib) == 0)
					break;

			if (j >= Nobjects) {
				Objects = (struct object **) REALLOC(Objects, ++Nobjects * sizeof(struct object *));
				Objects[j] = (struct object *) MALLOC(sizeof(struct object));
				Objects[j]->symtab = NULL;
				Objects[j]->name = strdup(lib);
				Objects[j]->foundpath = NULL;
				Objects[j]->size = size;
				Objects[j]->nsyms = 0;
				Objects[j]->flags = 0;
				if ((p = fp_getfunc("FPROF_PATH")) && *p) {
					char path[PATH_MAX];
					int len;
					int found = 0;

					while (1) {
						len = strcspn(p, ":");
						if (len > 0)
							sprintf(path, "%.*s/%s", len, p, basename(Objects[j]->name));
						else
							strcpy(path, Objects[j]->name);
						if (checkobject(path, size)) {
							found = 1;
							break;
						}
						if (!p[len])
							break;
						p += len + 1;
					};
					Objects[j]->foundpath = found ? strdup(path) : NULL;
				}
				else
					Objects[j]->foundpath = checkobject(Objects[j]->name, size) ? Objects[j]->name : NULL;
			}
			fptr->fileinfo[fileno].objects[i]->o = Objects[j];
			fptr->fileinfo[fileno].objects[i]->map_addr = start;
			fptr->fileinfo[fileno].objects[i]->start_time = fptr->prev_time;
			fptr->fileinfo[fileno].objects[i]->end_time = ULONG_MAX;
			if (!start) {
				if (!fptr->fileinfo[fileno].objects[i]->o->foundpath)
					fptr->fileinfo[fileno].objects[i]->start_addr = (void *) 0;
				else if (getsyms(fptr->fileinfo[fileno].objects[i]->o))
					fptr->fileinfo[fileno].objects[i]->start_addr = fptr->fileinfo[fileno].objects[i]->o->symtab[0].addr;
				else {
					fptr->fileinfo[fileno].objects[i]->start_addr = (void *) 0;
					return(0);
				}
			}
			else
				fptr->fileinfo[fileno].objects[i]->start_addr = start;
		}
		fptr->fileinfo[fileno].objects[i]->flags |= OBJECT_VISITED;
	}

	for (i = 0; i < fptr->fileinfo[fileno].nobjects; i++)
		if (!(fptr->fileinfo[fileno].objects[i]->flags & OBJECT_VISITED))
			fptr->fileinfo[fileno].objects[i]->end_time = fptr->prev_time;

	return(1);
}

/*
** Read in next record from designated log file.  Process MARKLOG
** and BUFTIME entries, skip others, but don't return until you get a
** real record.
*/
int
next_record(struct fp *fptr, struct logent *plog, int fileno)
{
	ulong buf[10], stamp1, stamp2;

	plog->flags = 0;
	for ( ; ; ) {
		if (!do_fread(buf, sizeof(long), 2, fptr, fileno)) {
			compile_end(fptr, fileno, 1);
			return(FPROF_EOL);
		}
		if (buf[0] != 0)
			break;
		do_fwrite(buf, sizeof(ulong), 2, fptr, fileno);
		switch (buf[1]) {
		case BUFTIME:
			if (!do_read_write(buf + 2, sizeof(ulong), 5, fptr, fileno)) {
				compile_end(fptr, fileno, 1);
				return(FPROF_EOL);
			}
			if ((fptr->fileinfo[fileno].l.curoffset > fptr->fileinfo[fileno].lastoffset_visited)) {
				stamp1 = long_to_time(&fptr->fileinfo[fileno].l, buf[3]);
				stamp2 = long_to_time(&fptr->fileinfo[fileno].l, buf[4]);
				fptr->fileinfo[fileno].l.total_overhead += (stamp2 - stamp1);
			}
			break;
		case MARKLOG:
			if (!do_read_write(buf + 2, sizeof(ulong), 3, fptr, fileno)) {
				compile_end(fptr, fileno, 1);
				return(FPROF_EOL);
			}
			plog->flags |= FPROF_IS_MARK;
			break;
			
		default:
			{
				char *tmp;
				ulong length;

				/*
				** For anything we don't know about (or a MAPTIME entry)
				** just skip over it
				*/
				if (!do_read_write(&length, sizeof(ulong), 1, fptr, fileno))
					return(FPROF_EOL);
				length += 2 * sizeof(long) - sizeof(long); /* trailer - already consumed length field */
				tmp = MALLOC(length);
				if (!do_read_write(tmp, length, 1, fptr, fileno)) {
					free(tmp);
					return(FPROF_EOL);
				}
				free(tmp);
				break;
			};
		}
	}

	plog->flags |= (buf[1] & 0x80000000) ? FPROF_IS_EPILOGUE : FPROF_IS_PROLOGUE;
	plog->realtime = long_to_time(&fptr->fileinfo[fileno].l, buf[1]);
	plog->compensated_time = plog->realtime - fptr->fileinfo[fileno].l.total_overhead;
	fptr->fileinfo[fileno].l.total_overhead += fptr->fileinfo[fileno].stamp_overhead;
	/*
	** If we don't find the symbol table entry for an address, halt
	** the compile operation
	*/
	if (!(plog->symbol = findsym(fptr, (char *) buf[0], fileno)))
		compile_end(fptr, fileno, 0);
	else {
		buf[0] = (fptr->curobj + 1) * (1 << 24) + (plog->symbol - plog->symbol->object->symtab);
		do_fwrite(buf, sizeof(ulong), 2, fptr, fileno);
	}
	return (FPROF_SUCCESS);
}

/*
** The key routine.  It is split into 2 parts:  reading the record
** and keeping track of marks.  Reading is also split into 2 parts:
** separate logs and merged logs.  Separate logs are much simpler:
** just read the next record from the current log; if you hit the end
** of the log, move to the next.  For merged logs, keep the next record
** for each log in the data structure, the one with the earliest time
** is the correct record.  The section dealing with marks has its
** own comments (see below).
*/
int
fp_next_record(struct fp *fptr, struct logent *plog)
{
	if (fptr->flags & FPROF_SEPARATE_EXPERIMENTS) {
		if (fptr->curfp >= fptr->nfps) {
			return(FPROF_EOF);
		}

		if (next_record(fptr, plog, fptr->curfp) == FPROF_EOL) {
			fptr->curfp++;
			if ((fptr->curfp < fptr->nfps) && (fptr->fileinfo[fptr->curfp].l.curoffset != HEADER_SIZE))
				fp_rewind_file(fptr, fptr->curfp);
			return(FPROF_EOL);
		}
	}
	else {
		int i, besti = -1;
		ulong besttime = ULONG_MAX;

		for (i = 0; i < fptr->nfps; i++) {
			if (!(fptr->fileinfo[i].l.flags & FPROF_LOGENT_CURRENT) && !(fptr->fileinfo[i].l.flags & FPROF_LOG_DONE)) {
				if (next_record(fptr, &fptr->fileinfo[i].l.logent, i) == FPROF_EOL)
					fptr->fileinfo[i].l.flags |= FPROF_LOG_DONE;
				else
					fptr->fileinfo[i].l.flags |= FPROF_LOGENT_CURRENT;
			}
			if (!(fptr->fileinfo[i].l.flags & FPROF_LOG_DONE) && (fptr->fileinfo[i].l.logent.compensated_time < besttime)) {
				besti = i;
				besttime = fptr->fileinfo[i].l.logent.compensated_time;
			}
		}
		if (besti < 0)
			return(FPROF_EOF);
		*plog = fptr->fileinfo[besti].l.logent;
		fptr->fileinfo[besti].l.flags &= ~FPROF_LOGENT_CURRENT;
	}

	/*
	** Is this a mark?  If we are keeping track of marks,
	** check to see if this point should be a mark.  This is so if:  we
	** have not placed a mark before OR  we
	** are using "gap" marks and this is a gap OR we are using name
	** marks and this entry matches the name.
	*/
	if ((fptr->flags & FPROF_MARK) &&
			((fptr->nmarks == 0) ||
			(fptr->mark_criterion && regex(fptr->mark_criterion, plog->symbol->name, NULL)) ||
			(!fptr->mark_criterion && (plog->compensated_time - fptr->prev_time > 500000)))) {
		/* Only store the mark if we have not been this late before */
		if (fptr->latest_time < plog->compensated_time) {
			int i;

			if (!(fptr->nmarks % 10)) {
				struct location *buf;

				buf = (struct location *) MALLOC(10 * sizeof(struct location) * (fptr->nfps));
				fptr->marks = (struct location **) REALLOC(fptr->marks, (fptr->nmarks + 10) * sizeof(struct location));
				fptr->marktimes = (fptime *) REALLOC(fptr->marktimes, (fptr->nmarks + 10) * sizeof(fptime));
				for (i = 0; i < 10; i++)
					fptr->marks[fptr->nmarks + i] = buf + i * fptr->nfps;
			}

			for (i = 0; i < fptr->nfps; i++) {
				fptr->marks[fptr->nmarks][i] = fptr->fileinfo[i].l;
				if (!(fptr->fileinfo[i].l.flags & FPROF_LOGENT_CURRENT) && !(fptr->fileinfo[i].l.flags & FPROF_LOG_DONE))
					fptr->marks[fptr->nmarks][i].flags |= FPROF_LOGENT_CURRENT;
			}
			if (fptr->curfp >= 0)
				fptr->marks[fptr->nmarks][fptr->curfp].flags |= FPROF_CURRENT_FP;
			fptr->marktimes[fptr->nmarks] = fptr->prev_time;
			fptr->nmarks++;
		}
		plog->flags |= FPROF_IS_MARK;
	}

	fptr->prev_time = plog->compensated_time;
	if (fptr->prev_time > fptr->latest_time)
		fptr->latest_time = fptr->prev_time;
	return (FPROF_SUCCESS);
}

/* For debugging */
void
fp_nop()
{
}

/* Seek to the proper point based on the new location */
void
fp_seek(struct perfile *current, struct location *new)
{
	if (current->l.curoffset != new->curoffset)
		fseek(current->fp, new->curoffset, SEEK_SET);

	current->l = new[0];
}

/* Seek to the designated location for each log */
void
fp_allseek(struct fp *fptr, struct location *newlist)
{
	int i;

	for (i = 0; i < fptr->nfps; i++) {
		fp_seek(fptr->fileinfo + i, newlist + i);
		if (fptr->fileinfo[i].l.flags & FPROF_CURRENT_FP) {
			fptr->curfp = i;
			fptr->fileinfo[i].l.flags &= ~(FPROF_CURRENT_FP);
		}
	}
}

/*
** Seek to the next mark, forward or backward.  This can be done by
** comparing the time of each mark against prev_time.  If the next
** mark has not yet been read, read until you do.
*/
void
fp_seekmark(struct fp *fptr, int forward)
{
	int ret = FPROF_SUCCESS;
	int i;

	if (!(fptr->flags & FPROF_MARK))
		return;
	for (i = 0; i < fptr->nmarks; i++)
		if (fptr->marktimes[i] > fptr->prev_time)
			break;

	if (forward) {
		struct logent logent;

		if (i == fptr->nmarks) {
			while ((ret = fp_next_record(fptr, &logent)) != FPROF_EOF)
				if (logent.flags & FPROF_IS_MARK)
					break;
			if (!(logent.flags & FPROF_IS_MARK))
				return;
			i = fptr->nmarks - 1;
		}
	}
	else
		i--;

	fp_allseek(fptr, fptr->marks[i]);
	fptr->prev_time = fptr->marktimes[i];
}

/* Seek to the numbered mark */
void
fp_seekmarkno(struct fp *fptr, int no)
{
	fp_allseek(fptr, fptr->marks[no]);
	fptr->prev_time = fptr->marktimes[no];
}

/* Rewind the designated log. */
static void
fp_rewind_file(struct fp *fptr, int fileno)
{
	fptr->fileinfo[fileno].l.prevlbolt = 0;
	fptr->fileinfo[fileno].l.prevleft = 0;
	fseek(fptr->fileinfo[fileno].fp, HEADER_SIZE, SEEK_SET);
	fptr->fileinfo[fileno].l.curoffset = HEADER_SIZE;
	fptr->fileinfo[fileno].l.total_overhead = 0;
	fptr->fileinfo[fileno].l.flags &= ~(FPROF_LOGENT_CURRENT|FPROF_LOG_DONE);
}

/* Rewind all logs */
void
fp_rewind_record(struct fp *fptr)
{
	int i;
	char buf[BUFSIZ];

	if (fptr->flags & FPROF_SEPARATE_EXPERIMENTS)
		fptr->curfp = 0;
	for (i = 0; i < fptr->nfps; i++)
		fp_rewind_file(fptr, i);
	fptr->prev_time = 0;
}

/*
** Search for a record whose symbol name matches the designated
** regular expression.
*/
int
fp_search(struct fp *fptr, char *regexp, struct logent *plog)
{
	struct logent logent;
	ulong prevtime;
	int ret;
	char *compiled;

	compiled = regcmp(regexp, 0);
	prevtime = 0;
	while ((ret = fp_next_record(fptr, &logent)) != FPROF_EOF) {
		if (ret == FPROF_EOL)
			continue;
		if (regex(compiled, logent.symbol->name, NULL))
			break;
	}

	if (ret != FPROF_EOF) {
		*plog = logent;
		return(1);
	}
	return(0);
}

libinit()
{
	void *handle;
	void (*tmp)();

	handle = dlopen(NULL, RTLD_LAZY);
	if (tmp = (void (*)()) dlsym(handle, "altprintf")) {
		fp_inxksh = 1;
		fp_setfunc = (int (*)()) dlsym(handle, "env_set");
		fp_getfunc = (char *(*)()) dlsym(handle, "env_get");
		fp_fprfunc = (void (*)()) dlsym(handle, "altfprintf");
		fp_prfunc = tmp;
	}
	dlclose(handle);
}
