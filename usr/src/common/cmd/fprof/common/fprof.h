/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved. */
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved. */
/*	  All Rights Reserved */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc. */
/*	The copyright notice above does not evidence any */
/*	actual or intended publication of such source code. */

#ident	"@(#)fprof:common/fprof.h	1.2"

#ifndef _FPROF_H
#define _FPROF_H
#include <stdio.h>
#include "machdep.h"

#define HEADER_SIZE 4096

/* Log entries */
#define BUFTIME		1
#define MAPTIME		2
#define MARKLOG		3

/* Flags to fp_open */
#define FPROF_SEPARATE_EXPERIMENTS	1
#define FPROF_MARK					2

/* Flags applicable to individual log */
#define FPROF_COMPILED			1
#define FPROF_LOGENT_CURRENT	2
#define FPROF_LOG_DONE			4
#define FPROF_CURRENT_FP		8

/* Flags for object_plus */
#define OBJECT_VISITED 1

/* Flags for objects */
#define OBJECT_MISSING 1

/* Error codes */
#define FPROF_ERROR_CANNOT_READ		1
#define FPROF_ERROR_NOT_FPROF_LOG	2
#define FPROF_ERROR_OBJECT_MISMATCH	3

void *fp_open();
void fp_close();
void *fp_next_object();
int fp_next_record();
void fp_rewind_object();
void fp_rewind_record();

#define fp_record_alloc() malloc(sizeof(struct logent))
#define fp_record_to_symbol(REC) ((struct logent *) (REC))->symbol
#define fp_record_to_realtime(REC) ((struct logent *) (REC))->realtime
#define fp_record_to_compensated_time(REC) ((struct logent *) (REC))->compensated_time
#define fp_record_set_pointer(REC, PTR) fp_symbol_to_pointer(fp_record_to_symbol(REC)) = (void *) (PTR)
#define fp_record_to_pointer(REC) fp_symbol_to_pointer(fp_record_to_symbol(REC))
#define fp_record_to_object(REC) fp_symbol_to_object(fp_record_to_symbol(REC))
#define fp_max_records(FPTR) ((struct fp *) (FPTR))->max_records
#define fp_record_is_prologue(REC) (((struct logent *) (REC))->flags & FPROF_IS_PROLOGUE)
#define fp_record_is_epilogue(REC) (((struct logent *) (REC))->flags & FPROF_IS_EPILOGUE)

#define fp_symbol_set_pointer(SYM, PTR) fp_symbol_to_pointer(SYM) = (void *) (PTR)
#define fp_symbol_to_addr(SYM) ((struct sym *) (SYM))->addr
#define fp_symbol_to_object(SYM) ((struct sym *) (SYM))->object
#define fp_symbol_to_pointer(SYM) ((struct sym *) (SYM))->ptr
#define fp_symbol_to_name(SYM) ((struct sym *) (SYM))->name
#define fp_symbol_size(SYM) ((struct sym *) (SYM))->size
#define fp_symbol_to_object_name(SYM) fp_object_to_name(fp_symbol_to_object(SYM))
#define fp_symbol_to_object_size(SYM) fp_object_to_size(fp_symbol_to_object(SYM))

#define fp_object_to_name(OBJ) ((struct object *) (OBJ))->name
#define fp_object_to_size(OBJ) ((struct object *) (OBJ))->nsyms

#define FPROF_IS_PROLOGUE	1
#define FPROF_IS_EPILOGUE	2
#define FPROF_IS_MARK		4

#define FPROF_SUCCESS	0
#define FPROF_EOL		1
#define FPROF_EOF		2

typedef ulong fptime;

struct logent {
	int flags;
	struct sym *symbol;
	fptime realtime;
	fptime compensated_time;
};

struct object {
	char *name;
	char *foundpath;
	struct sym *symtab;
	ulong size;
	ulong nsyms;
	unchar flags;
};

struct sym {
	struct object *object;
	char *name;
	char *addr;
	ulong size;
	void *ptr;
};

struct object_plus {
	unchar flags;
	fptime start_time;
	fptime end_time;
	void *start_addr;
	void *map_addr;
	struct object *o;
};

struct location {
	unchar flags;
	struct logent logent;
	ulong prevlbolt;
	ulong prevleft;
	ulong total_overhead;
	ulong curoffset;
};

struct perfile {
	char *name;
	FILE *fp;
	FILE *cfp;
	struct object_plus **objects;
	int nobjects;
	ulong stamp_overhead;
	ulong lastoffset_visited;
	struct location l;
	ulong lastoffset_compiled;
	ulong control_address;
	ulong pid;
	char *node;
	ulong time;
	unchar version;
};

struct logcontrol {
	unchar mark;
	unchar nolog;
	unchar inlogue;
	unchar on;
	unchar accurate;
};

struct fp {
	unchar flags;
	struct perfile *fileinfo;
	int curobj;
	int nfps;
	int curfp; /* only needed for FPROF_SEPARATE_EXPERIMENTS logs */
	int curobject;
	int max_records;
	struct location **marks;
	fptime *marktimes;
	ulong nmarks;
	char *mark_criterion;
	fptime latest_time;
	fptime prev_time;
};
#endif /* ifndef _FPROF_H */
