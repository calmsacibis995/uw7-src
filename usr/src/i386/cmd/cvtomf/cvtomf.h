#ident	"@(#)cvtomf:cvtomf.h	1.1"

/*
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1989 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*
 *	Copyright (c) Altos Computer Systems, 1987
 */

#include	<stdio.h>
#include	<fcntl.h>
#include	<filehdr.h>
#include	<aouthdr.h>
#include	<scnhdr.h>
#include	<reloc.h>
#include	<linenum.h>
#include	<syms.h>
#include	<storclass.h>
#include	<sgs.h>

#define		MAXDAT		(1024)
#define		MAXEXT		(1024)
#define		MAXGRP		(16)
#define		MAXNAM		(32)
#define		MAXREL		(256)
#define		MAXSCN		(16)
#define		MAXCOM		(256)	/* maximum number of comment strings S000 */

#ifdef FP_SYMS
#define		FP_COFF		"___FP_Used"
#define		FP_OMF		"__fltused"
#endif

/* Structure for holding OMF line number entires */
struct lines {
	long offset;
	int number;
};

extern struct lines *lines;
extern int line_indx;
extern long *type_index;
extern int do_debug;
extern int symbols_pres;
extern int types_pres;
extern int lines_pres;

extern char *optarg;
extern int optind;
extern int opterr;

extern char *argv0, *objname;
extern int  verbose;

#ifdef DEBUG
extern int  debug;
#define	USAGE	"usage: cvtomf [-dgv] [-o output] file file file ..."
#else
#define	USAGE	"usage: cvtomf [-gv] [-o output] file file file ..."
#endif

extern int sym_file, typ_file;

extern FILE *datafile[], *relocfile[], *linenofile[];
extern long nscns, nlnno, nsyms, nreloc;

#define	PASS1	1
#define PASS2	2

extern	void warning();
extern	void fatal();
extern	void recskip(void);
extern void *malloc();
extern void *realloc();
extern void *calloc();
extern char *strdup();
extern long method(), expand();
extern long dword(), length();
extern char *string();
extern long symbol();
extern void process_sym(); 
extern void process_typ();
extern void write_tyeps(int, long, unsigned char *, long);
