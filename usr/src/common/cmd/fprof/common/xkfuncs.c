/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved. */
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved. */
/*	  All Rights Reserved */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc. */
/*	The copyright notice above does not evidence any */
/*	actual or intended publication of such source code. */

#ident	"@(#)fprof:common/xkfuncs.c	1.3"

#include <sys/types.h>
#include <sys/stat.h>
#include <macros.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <limits.h>
#include <dlfcn.h>
#include <time.h>
#include "fprof.h"

static struct fp *Fptr;

int Subtract;

extern void (*fp_prfunc)();
extern void (*fp_setfunc)();
extern void errprintf();
extern int fp_inxksh;

#define LOGCHECK() if (!Fptr) {\
		errprintf("There is no log open\n");\
		return(1);\
	}

#define USAGE(STR) {\
	errprintf("Usage: ");\
	errprintf(STR);\
	errprintf("\n");\
	return(1);\
}

b_open(int argc, char **argv)
{
	char buf[20];
	struct logent logent;

	if (!(Fptr = fp_open(argv + 1, FPROF_MARK)))
		return(1);
	sprintf(buf, "FPTR=0x%x", Fptr);
	fp_setfunc(buf);
	fp_next_record(Fptr, &logent);
	Subtract = logent.compensated_time;
	fp_rewind_record(Fptr);
	return(0);
}

void
dumprec(struct logent *plog)
{
	char *str;

	if (plog->flags & FPROF_IS_PROLOGUE)
		str = "Calling      ";
	else
		str = "Returning    ";

	fp_prfunc("%s%-40.40s at %5d.%03d:%03d\n", str, plog->symbol->name, (plog->compensated_time - Subtract) / 1000000, ((plog->compensated_time - Subtract) % 1000000) / 1000, (plog->compensated_time - Subtract) % 1000);
}

static int
nextrec(int markend)
{
	struct logent logent;

	switch (fp_next_record(Fptr, &logent)) {
	case FPROF_EOL:
		return(nextrec(markend));
	case FPROF_EOF:
		errprintf("End of Input\n");
		return(1);
	}
	/*[ -n "$SUBTRACT" ] || SUBTRACT=$FIRSTENT*/
	dumprec(&logent);
	if (markend && (logent.flags & FPROF_IS_MARK))
		return(2);
	return(0);
}

int
b_output(int argc, char **argv)
{
	int num = 10;
	int i;

	LOGCHECK();
	if (argv[1])
		num = atoi(argv[1]);
	for (i = 0; i < num; i++)
		if (nextrec(0))
			return(i ? 0 : 1);
	return(0);
}

int
b_output_until_mark()
{
	int ret;
	int count;

	LOGCHECK();
	for (count = 0; !(ret = nextrec(1)); count++)
		;
	if ((ret == 2) || count)
		return(0);
	return(1);
}

int
b_bracket()
{
	LOGCHECK();
	fp_seekmark(Fptr, 0);
	nextrec(0);
	nextrec(0);
	return(b_output_until_mark());
}

int
b_search(int argc, char **argv)
{
	struct logent logent;

	LOGCHECK();
	if (!argv[1])
		USAGE("search regular-expression");
	fp_search(Fptr, argv[1], &logent);
	dumprec(&logent);
	return(0);
}

int
b_rewind()
{
	LOGCHECK();
	fp_rewind_record(Fptr);
}

int
b_count()
{
	LOGCHECK();
	fp_count(Fptr);
	return(0);
}

int
b_close()
{
	LOGCHECK();
	fp_close(Fptr);
	Fptr = NULL;
	fp_setfunc("FPTR=");
	return(0);
}

int
b_callers(int argc, char **argv)
{
	int i = 1;

	LOGCHECK();
	if (!argv[i])
		USAGE("callers function-name");
	for ( ; argv[i]; i++)
		dumpcallersof(argv[i]);
	return(0);
}

int
b_callees(int argc, char **argv)
{
	int i = 1;

	LOGCHECK();
	if (!argv[i])
		USAGE("callees function-name");
	for ( ; argv[i]; i++)
		dumpcalleesfrom(argv[i]);
	return(0);
}

int
b_stats()
{
	LOGCHECK();
	dumpstats();
	return(0);
}

int
b_info()
{
	int i, j;

	LOGCHECK();
	for (i = 0; i < Fptr->nfps; i++) {
		fp_prfunc("Log name: %s\n", Fptr->fileinfo[i].name);
		fp_prfunc("The experiment was run on node '%s' at %s\n", Fptr->fileinfo[i].node, ctime((time_t *) &Fptr->fileinfo[i].time));
		if (Fptr->fileinfo[i].l.flags & FPROF_COMPILED)
			fp_prfunc("The log has been compiled\n");
		else
			fp_prfunc("The log has not been compiled\n");
		if (Fptr->fileinfo[i].stamp_overhead)
			fp_prfunc("This is an accurate time stamp log\n");
		else
			fp_prfunc("This is not an accurate time stamp log\n");
		fp_prfunc("\n");
		fp_prfunc("The program had access to these objects:\n");
		for (j = 0; j < Fptr->fileinfo[i].nobjects; j++) {
			if (Fptr->fileinfo[i].objects[j]->o->foundpath) {
				if (strcmp(Fptr->fileinfo[i].objects[j]->o->name, Fptr->fileinfo[i].objects[j]->o->foundpath))
					fp_prfunc("\t%s found for reading at %s\n", Fptr->fileinfo[i].objects[j]->o->name, Fptr->fileinfo[i].objects[j]->o->foundpath);
				else
					fp_prfunc("\t%s found for reading\n", Fptr->fileinfo[i].objects[j]->o->name);
			}
			else
				fp_prfunc("\t%s not found for reading\n", Fptr->fileinfo[i].objects[j]->o->name);
		}
	}
	return(0);
}

int
b_compile()
{
	int i, j;
	struct logent logent;

	LOGCHECK();
	while (fp_next_record(Fptr, &logent) != FPROF_EOF)
		;
	return(0);
}

int
do_mark()
{
	char buf[PATH_MAX];
	int i, fd;

	for (i = 0; i < Fptr->nfps; i++) {
		sprintf(buf, "/proc/%d/as", Fptr->fileinfo[i].pid);
		if ((fd = open(buf, O_RDONLY)) == -1) {
			fp_prfunc("Process %d is not running\n", Fptr->fileinfo[i].pid);
			continue;
		}
		lseek(fd, Fptr->fileinfo[i].control_address + &((struct logcontrol *) 0)->mark, SEEK_SET);
		write(fd, (unchar) 1, sizeof(unchar));
		close(fd);
	}
}

int
do_switch(int on)
{
	char buf[PATH_MAX];
	int i, fd;

	for (i = 0; i < Fptr->nfps; i++) {
		sprintf(buf, "/proc/%d/as", Fptr->fileinfo[i].pid);
		if ((fd = open(buf, O_RDONLY)) == -1) {
			fp_prfunc("Process %d is not running\n", Fptr->fileinfo[i].pid);
			continue;
		}
		lseek(fd, Fptr->fileinfo[i].control_address + &((struct logcontrol *) 0)->on, SEEK_SET);
		write(fd, (unchar) on, sizeof(unchar));
		close(fd);
	}
}
