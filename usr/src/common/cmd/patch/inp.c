/*
 * COPYRIGHT NOTICE
 * 
 * This source code is designated as Restricted Confidential Information
 * and is subject to special restrictions in a confidential disclosure
 * agreement between HP, IBM, SUN, NOVELL and OSF.  Do not distribute
 * this source code outside your company without OSF's specific written
 * approval.  This source code, and all copies and derivative works
 * thereof, must be returned or destroyed at request. You must retain
 * this notice on any copies which you make.
 * 
 * (c) Copyright 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED 
 */

#ident	"@(#)patch_p2:inp.c	1.1"

/*
 * OSF/1 1.2
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile$ $Revision$ (OSF) $Date$";
#endif

/* Header: inp.c,v 2.0.1.1 88/06/03 15:06:13 lwall Locked
 *
 * Log:	inp.c,v
 * Revision 2.0.1.1  88/06/03  15:06:13  lwall
 * patch10: made a little smarter about sccs files
 * 
 * Revision 2.0  86/09/17  15:37:02  lwall
 * Baseline for netwide release.
 * 
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pfmt.h>
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <nl_types.h>
#include <langinfo.h>
#include <string.h>
#include <ctype.h>
#include "EXTERN.h"
#include "config.h"
#include "common.h"
#include "inp.h"
#include "util.h"

/* Input-file-with-indexable-lines abstract type */

static long i_size;			/* size of the input file */
static char *i_womp;			/* plan a buffer for entire file */
static char **i_ptr;			/* pointers to lines in i_womp */

static int tifd = -1;			/* plan b virtual string array */
static char *tibuf[2];			/* plan b buffers */
static LINENUM tiline[2] = {-1, -1};	/* 1st line in each buffer */
static LINENUM lines_per_buf;		/* how many lines per buffer */
static int tireclen;			/* length of records in tmp file */
struct stat filestat;			/* file statistics area */

/* New patch--prepare to edit another file. */

void
re_input()
{
    if (using_plan_a) {
	i_size = 0;
#ifndef lint
	if (i_ptr != Null(char**))
	    free((char *)i_ptr);
#endif
	if (i_womp != Nullch)
	    free(i_womp);
	i_womp = Nullch;
	i_ptr = Null(char **);
    }
    else {
	using_plan_a = TRUE;		/* maybe the next one is smaller */
	Close(tifd);
	tifd = -1;
	free(tibuf[0]);
	free(tibuf[1]);
	tibuf[0] = tibuf[1] = Nullch;
	tiline[0] = tiline[1] = -1;
	tireclen = 0;
    }
}

/* Constuct the line index, somehow or other. */

void
scan_input(filename)
char *filename;
{
    if (!plan_a(filename))
	plan_b(filename);
    if (verbose) {
	pfmt(stderr, MM_NOSTD, ":1:Patching file %s using Plan %s...\n",
		filename, (using_plan_a ? "A" : "B") );
    }
}

/* Try keeping everything in memory. */

bool
plan_a(filename)
char *filename;
{
    int ifd;
    Reg1 char *s;
    Reg2 LINENUM iline;

    if (ok_to_create_file && stat(filename, &filestat) < 0) {
	if (verbose)
	    pfmt(stderr, MM_NOSTD, ":2:(Creating file %s...)\n",filename);
	makedirs(filename, TRUE);
	close(creat(filename, 0666));
    }
    if (stat(filename, &filestat) < 0) {
	Sprintf(buf, "RCS/%s%s", filename, RCSSUFFIX);
	if (stat(buf, &filestat) >= 0 || stat(buf+4, &filestat) >= 0) {
	    Sprintf(buf, CHECKOUT, filename);
	    if (verbose)
		pfmt(stderr, MM_NOSTD,
		    ":3:Can't find %s--attempting to check it out from RCS.\n",
		    filename);
	    if (system(buf) || stat(filename, &filestat))
		fatal2(":4:Can't check out %s.\n", filename);
	}
	else {
	    Sprintf(buf+20, "SCCS/%s%s", SCCSPREFIX, filename);
	    if (stat(s=buf+20, &filestat) >= 0 ||
	      stat(s=buf+25, &filestat) >= 0) {
		Sprintf(buf, GET, s);
		if (verbose)
		    pfmt(stderr, MM_NOSTD,
			":5:Can't find %s--attempting to get it from SCCS.\n",
			filename);
		if (system(buf) || stat(filename, &filestat))
		    fatal2(":6:Can't get %s.\n", filename);
	    }
	    else
		fatal2(":7:Can't find %s.\n", filename);
	}
    }
    filemode = filestat.st_mode;
    if ((filemode & S_IFMT) & ~S_IFREG)
	fatal2(":8:%s is not a normal file--can't patch.\n", filename);
    i_size = filestat.st_size;
    if (out_of_mem) {
	set_hunkmax();		/* make sure dynamic arrays are allocated */
	out_of_mem = FALSE;
	return FALSE;			/* force plan b because plan a bombed */
    }
#ifdef lint
    i_womp = Nullch;
#else
    i_womp = (char *)malloc((MEM)(i_size+2));	/* lint says this may alloc less than */
					/* i_size, but that's okay, I think. */
#endif
    if (i_womp == Nullch)
	return FALSE;
    if ((ifd = open(filename, 0)) < 0)
	fatal2(":9:Can't open file %s\n", filename);
#ifndef lint
    if (read(ifd, i_womp, (int)i_size) != i_size) {
	Close(ifd);	/* probably means i_size > 15 or 16 bits worth */
	free(i_womp);	/* at this point it doesn't matter if i_womp was */
	return FALSE;	/*   undersized. */
    }
#endif
    Close(ifd);
    if (i_size && i_womp[i_size-1] != '\n')
	i_womp[i_size++] = '\n';
    i_womp[i_size] = '\0';

    /* count the lines in the buffer so we know how many pointers we need */

    iline = 0;
    for (s=i_womp; *s; s++) {
	if (*s == '\n')
	    iline++;
    }
#ifdef lint
    i_ptr = Null(char**);
#else
    i_ptr = (char **)malloc((MEM)((iline + 2) * sizeof(char *)));
#endif
    if (i_ptr == Null(char **)) {	/* shucks, it was a near thing */
	free((char *)i_womp);
	return FALSE;
    }
    
    /* now scan the buffer and build pointer array */

    iline = 1;
    i_ptr[iline] = i_womp;
    for (s=i_womp; *s; s++) {
	if (*s == '\n')
	    i_ptr[++iline] = s+1;	/* these are NOT null terminated */
    }
    input_lines = iline - 1;

    /* now check for revision, if any */

    if (revision != Nullch) { 
	if (!rev_in_string(i_womp)) {
	    if (force) {
		if (verbose)
		    pfmt(stderr, MM_NOSTD,
":10:Warning: this file doesn't appear to be the %s version--patching anyway.\n",
			revision);
	    }
	    else {
		ask3(":11:This file doesn't appear to be the %s version--patch anyway? [%s] ",
		    revision, nl_langinfo(NOSTR));
	    if (strcmp(buf, nl_langinfo(NOSTR)) == 0)
		fatal1(":12:Aborted.\n");
	    }
	}
	else if (verbose)
	 pfmt(stderr, MM_NOSTD,
		":13:Good.  This file appears to be the %s version.\n",
		revision);
    }
    return TRUE;			/* plan a will work */
}

/* Keep (virtually) nothing in memory. */

void
plan_b(filename)
char *filename;
{
    Reg3 FILE *ifp;
    Reg1 int i = 0;
    Reg2 int maxlen = 1;
    Reg4 bool found_revision = (revision == Nullch);

    using_plan_a = FALSE;
    if ((ifp = fopen(filename, "r")) == Nullfp)
	fatal2(":9:Can't open file %s\n", filename);
    if ((tifd = creat(TMPINNAME, 0666)) < 0)
	fatal2(":9:Can't open file %s\n", TMPINNAME);
    while (fgets(buf, sizeof buf, ifp) != Nullch) {
	if (revision != Nullch && !found_revision && rev_in_string(buf))
	    found_revision = TRUE;
	if ((i = strlen(buf)) > maxlen)
	    maxlen = i;			/* find longest line */
    }
    if (revision != Nullch) {
	if (!found_revision) {
	    if (force) {
		if (verbose)
		    pfmt(stderr, MM_NOSTD,
":10:Warning: this file doesn't appear to be the %s version--patching anyway.\n",
			revision);
	    }
	    else {
		ask3(":11:This file doesn't appear to be the %s version--patch anyway? [%s] ",
		    revision, nl_langinfo(NOSTR));
	        if (strcmp(buf, nl_langinfo(NOSTR)) == 0)
		    fatal1(":12:Aborted.\n");
	    }
	}
	else if (verbose)
	 pfmt(stderr, MM_NOSTD,
		":13:Good.  This file appears to be the %s version.\n",
		revision);
    }
    Fseek(ifp, 0L, 0);		/* rewind file */
    lines_per_buf = BUFFERSIZE / maxlen;
    tireclen = maxlen;
    tibuf[0] = (char *)malloc((MEM)(BUFFERSIZE + 1));
    tibuf[1] = (char *)malloc((MEM)(BUFFERSIZE + 1));
    if (tibuf[1] == Nullch)
	fatal1(":14:Can't seem to get enough memory.\n");
    for (i=1; ; i++) {
	if (! (i % lines_per_buf))	/* new block */
	    if (write(tifd, tibuf[0], BUFFERSIZE) < BUFFERSIZE)
		fatal1(":15: can't write temp file.\n");
	if (fgets(tibuf[0] + maxlen * (i%lines_per_buf), maxlen + 1, ifp)
	  == Nullch) {
	    input_lines = i - 1;
	    if (i % lines_per_buf)
		if (write(tifd, tibuf[0], BUFFERSIZE) < BUFFERSIZE)
		    fatal1(":15: can't write temp file.\n");
	    break;
	}
    }
    Fclose(ifp);
    Close(tifd);
    if ((tifd = open(TMPINNAME, 0)) < 0) {
	fatal2(":16:Can't reopen file %s\n", TMPINNAME);
    }
}

/* Fetch a line from the input file, \n terminated, not necessarily \0. */

char *
ifetch(line,whichbuf)
Reg1 LINENUM line;
int whichbuf;				/* ignored when file in memory */
{
    if (line < 1 || line > input_lines)
	return "";
    if (using_plan_a)
	return i_ptr[line];
    else {
	LINENUM offline = line % lines_per_buf;
	LINENUM baseline = line - offline;

	if (tiline[0] == baseline)
	    whichbuf = 0;
	else if (tiline[1] == baseline)
	    whichbuf = 1;
	else {
	    tiline[whichbuf] = baseline;
#ifndef lint		/* complains of long accuracy */
	    Lseek(tifd, (long)baseline / lines_per_buf * BUFFERSIZE, 0);
#endif
	    if (read(tifd, tibuf[whichbuf], BUFFERSIZE) < 0)
		fatal2(":17:Error reading tmp file %s.\n", TMPINNAME);
	}
	return tibuf[whichbuf] + (tireclen*offline);
    }
}

/* True if the string argument contains the revision number we want. */

bool
rev_in_string(string)
char *string;
{
    Reg1 char *s;
    Reg2 int patlen;

    if (revision == Nullch)
	return TRUE;
    patlen = strlen(revision);
    if (strnEQ(string,revision,patlen) && isspace(s[patlen]))
	return TRUE;
    for (s = string; *s; s++) {
	if (isspace(*s) && strnEQ(s+1, revision, patlen) && 
		isspace(s[patlen+1] )) {
	    return TRUE;
	}
    }
    return FALSE;
}

