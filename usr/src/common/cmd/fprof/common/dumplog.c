/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved. */
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved. */
/*	  All Rights Reserved */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc. */
/*	The copyright notice above does not evidence any */
/*	actual or intended publication of such source code. */

#ident	"@(#)fprof:common/dumplog.c	1.3"

#include	<stdio.h>
#include	<sys/types.h>
#include	<limits.h>
#include	"fprof.h"

#define PREPEND_LIBRARY_NAME 1

extern int Subtract;

static
pr_rec(struct logent *rec, int flags, int start)
{
	char *fmt, buf[1024], *p;

	if (!Subtract)
		Subtract = rec->compensated_time;
	dumprec(rec);
	return;
}

static
readlogs(int flags, char **files)
{
	void *rec, *id;
	struct logent last;
	fptime x, y;
	int i = 0;
	int start = 0;
	int ret;

	if (!(id = fp_open(files, FPROF_SEPARATE_EXPERIMENTS))) {
		fp_error();
		exit(1);
	}
	rec = (void *) fp_record_alloc();
	/*memset(&last, '\0', sizeof(struct logent));*/
	last.compensated_time = 0;

	while ((ret = fp_next_record(id, rec)) != FPROF_EOF) {
		if (!fp_record_to_symbol(rec))
			continue;
		if (ret == FPROF_EOL) {
			printf("End of Log\n");
			start = 0;
			continue;
		}
		if (!start)
			start = fp_record_to_compensated_time(rec);

		/*if ((x = fp_record_to_compensated_time(&last)) > (y = fp_record_to_compensated_time(rec))) {*/
			/*printf("Record %d out of order: diff %lu\n", i);*/
			/*pr_rec(&last, flags, start);*/
			pr_rec(rec, flags, start);
		/*}*/
		/*if (fp_record_to_compensated_time(rec) == 0)*/
			/*printf("What the Hell?\n");*/
		/*last = ((struct logent *) rec)[0];*/
		i++;
	}
}

main(int argc, char *argv[])
{
	readlogs(0, argv + 1);
}
