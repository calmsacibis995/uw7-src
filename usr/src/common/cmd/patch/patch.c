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

#ident	"@(#)patch_p2:patch.c	1.2.1.1"
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * OSF/1 1.2
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile$ $Revision$ (OSF) $Date$";
#endif

char patch_rcsid[] =
	"Header: patch.c,v 2.0.1.6 88/06/22 20:46:39 lwall Locked";

/* patch - a program to apply diffs to original files
 *
 * Copyright 1986, Larry Wall
 *
 * This program may be copied as long as you don't try to make any
 * money off of it, or pretend that you wrote it.
 *
 * Log:	patch.c,v
 * Revision 2.0.1.6  88/06/22  20:46:39  lwall
 * patch12: rindex() wasn't declared
 * 
 * Revision 2.0.1.5  88/06/03  15:09:37  lwall
 * patch10: exit code improved.
 * patch10: better support for non-flexfilenames.
 * 
 * Revision 2.0.1.4  87/02/16  14:00:04  lwall
 * Short replacement caused spurious "Out of sync" message.
 * 
 * Revision 2.0.1.3  87/01/30  22:45:50  lwall
 * Improved diagnostic on sync error.
 * Moved do_ed_script() to pch.c.
 * 
 * Revision 2.0.1.2  86/11/21  09:39:15  lwall
 * Fuzz factor caused offset of installed lines.
 * 
 * Revision 2.0.1.1  86/10/29  13:10:22  lwall
 * Backwards search could terminate prematurely.
 * 
 * Revision 2.0  86/09/17  15:37:32  lwall
 * Baseline for netwide release.
 * 
 * Revision 1.5  86/08/01  20:53:24  lwall
 * Changed some %d's to %ld's.
 * Linted.
 * 
 * Revision 1.4  86/08/01  19:17:29  lwall
 * Fixes for machines that can't vararg.
 * Added fuzz factor.
 * Generalized -p.
 * General cleanup.
 * 
 * 85/08/15 van%ucbmonet@berkeley
 * Changes for 4.3bsd diff -c.
 *
 * Revision 1.3  85/03/26  15:07:43  lwall
 * Frozen.
 * 
 * Revision 1.2.1.9  85/03/12  17:03:35  lwall
 * Changed pfp->_file to fileno(pfp).
 * 
 * Revision 1.2.1.8  85/03/12  16:30:43  lwall
 * Check i_ptr and i_womp to make sure they aren't null before freeing.
 * Also allow ed output to be suppressed.
 * 
 * Revision 1.2.1.7  85/03/12  15:56:13  lwall
 * Added -p option from jromine@uci-750a.
 * 
 * Revision 1.2.1.6  85/03/12  12:12:51  lwall
 * Now checks for normalness of file to patch.
 * 
 * Revision 1.2.1.5  85/03/12  11:52:12  lwall
 * Added -D (#ifdef) option from joe@fluke.
 * 
 * Revision 1.2.1.4  84/12/06  11:14:15  lwall
 * Made smarter about SCCS subdirectories.
 * 
 * Revision 1.2.1.3  84/12/05  11:18:43  lwall
 * Added -l switch to do loose string comparison.
 * 
 * Revision 1.2.1.2  84/12/04  09:47:13  lwall
 * Failed hunk count not reset on multiple patch file.
 * 
 * Revision 1.2.1.1  84/12/04  09:42:37  lwall
 * Branch for sdcrdcf changes.
 * 
 * Revision 1.2  84/11/29  13:29:51  lwall
 * Linted.  Identifiers uniqified.  Fixed i_ptr malloc() bug.  Fixed
 * multiple calls to mktemp().  Will now work on machines that can only
 * read 32767 chars.  Added -R option for diffs with new and old swapped.
 * Various cosmetic changes.
 * 
 * Revision 1.1  84/11/09  17:03:58  lwall
 * Initial revision
 * 
 */

#include <stdio.h>
#include <assert.h>
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
#include <regex.h>
#include "INTERN.h"
#include "config.h"
#include "common.h"
#include "inp.h"
#include "pch.h"
#include "util.h"
#include "version.h"
#include "patch.h"

/* procedures */

void reinitialize_almost_everything();
void get_some_switches();
LINENUM locate_hunk();
void abort_hunk();
void apply_hunk();
void init_output();
void init_reject();
void copy_till();
void spew_output();
void dump_line();
bool patch_match();
bool similar();
void re_input();
void my_exit();

LINENUM input_lines = 0;		/* how long is input file in lines */
LINENUM last_frozen_line = 0;		/* how many input lines have been */
					/* irretractibly output */
char *revision = Nullch;		/* prerequisite revision, if any */
char else_defined[] = "#else\n";	/* #else */
bool do_defines = FALSE;		/* patch using ifdef, ifndef, etc. */
char serrbuf[BUFSIZ];			/* buffer for stderr */

/* Apply a set of diffs as appropriate. */

main(argc,argv)
int argc;
char **argv;
{
    LINENUM where;
    LINENUM newwhere;
    LINENUM fuzz;
    LINENUM mymaxfuzz;
    int hunk = 0;
    int failed = 0;
    int exitval = 0;
    int i;
    regex_t yesre, nore;
    int err;

    (void)setlocale(LC_ALL, "");
    (void)setcat("uxpatch");
    (void)setlabel("UX:patch");

    setbuf(stderr, serrbuf);
    for (i = 0; i<MAXFILEC; i++)
	filearg[i] = Nullch;
    Mktemp(TMPOUTNAME);
    Mktemp(TMPINNAME);
    Mktemp(TMPREJNAME);
    Mktemp(TMPPATNAME);

    /* parse switches */
    Argc = argc;
    Argv = argv;
    get_some_switches();
    
    /* make sure we clean up /tmp in case of disaster */
    set_signals(0);

    for (
	open_patch_file(filearg[1]);
	there_is_another_patch();
	reinitialize_almost_everything()
    ) {					/* for each patch in patch file */

	if (oopt != NULL)
		filearg[0] = mapinput(filearg[0]);
	else if (outname == Nullch)
		outname = savestr(filearg[0]);
    
	/* initialize the patched file */
	if (!skip_rest_of_patch)
	    init_output(TMPOUTNAME);
    
	/* for ed script just up and do it and exit */
	if (diff_type == ED_DIFF) {
	    do_ed_script();
	    continue;
	}
    
	/* initialize reject file */
	init_reject(TMPREJNAME);
    
	/* find out where all the lines are */
	if (!skip_rest_of_patch)
	    scan_input(filearg[0]);
    
	/* from here on, open no standard i/o files, because malloc */
	/* might misfire and we can't catch it easily */
    
	/* apply each hunk of patch */
	hunk = 0;
	failed = 0;
	out_of_mem = FALSE;
	while (another_hunk()) {
	    hunk++;
	    fuzz = Nulline;
	    mymaxfuzz = pch_context();
	    if (maxfuzz < mymaxfuzz)
		mymaxfuzz = maxfuzz;
	    if (!skip_rest_of_patch) {
		do {
		    where = locate_hunk(fuzz);
		    if (hunk == 1 && where == Nulline && !force) {
						/* dwim for reversed patch? */
			if (!pch_swap()) {
			    if (fuzz == Nulline)
				pfmt(stderr, MM_NOSTD, 
				":18:Not enough memory to try swapped hunk!  Assuming unswapped.\n");
			    continue;
			}
			reverse = !reverse;
			where = locate_hunk(fuzz);  /* try again */
			if (where == Nulline) {	    /* didn't find it swapped */
			    if (!pch_swap())         /* put it back to normal */
				fatal1(":19:Lost hunk on alloc error!\n");
			    reverse = !reverse;
			}
			else if (noreverse) {
			    if (!pch_swap())         /* put it back to normal */
				fatal1(":19:Lost hunk on alloc error!\n");
			    reverse = !reverse;
			    pfmt(stderr, MM_NOSTD,
				":23:Ignoring previously applied (or reversed) patch.\n");
			    skip_rest_of_patch = TRUE;
			}
			else {
			    if (reverse)
				ask2(":20:Reversed (or previously applied) patch detected!  Assume -R? [%s] ",
				nl_langinfo(YESSTR));
			    else
				ask2(":21:Unreversed (or previously applied) patch detected!  Ignore -R? [%s] ",
				nl_langinfo(YESSTR));
			    err = regcomp(&yesre, nl_langinfo(YESEXPR),
				REG_EXTENDED | REG_NOSUB);
			    if (err != 0) {
				regerror(err, &yesre, buf, sizeof(buf));
				pfmt(stderr, MM_ERROR, "uxcore.abi:1234:RE failure: %s\n", buf);
				exit(2);
			    }
			    if (regexec(&yesre, buf, (size_t)0,
				 (regmatch_t *) 0, 0) != 0){ /* no */
				ask2(":22:Apply anyway? [%s] ",
					nl_langinfo(NOSTR));
			        err = regcomp(&nore, nl_langinfo(NOEXPR),
					REG_EXTENDED | REG_NOSUB);
			        if (err != 0) {
					regerror(err, &nore, buf, sizeof(buf));
					pfmt(stderr, MM_ERROR, "uxcore.abi:1234:RE failure: %s\n", buf);
					exit(2);
			    	}
			        if (regexec(&nore, buf, (size_t)0,
				     (regmatch_t *) 0, 0) != 0) /* no */
				    skip_rest_of_patch = TRUE;
				where = Nulline;
				reverse = !reverse;
				if (!pch_swap())  /* put it back to normal */
				    fatal1(":19:Lost hunk on alloc error!\n");
			    }
			}
		    }
		} while (!skip_rest_of_patch && where == Nulline &&
		    ++fuzz <= mymaxfuzz);

		if (skip_rest_of_patch) {		/* just got decided */
		    Fclose(ofp);
		    ofp = Nullfp;
		}
	    }

	    newwhere = pch_newfirst() + last_offset;
	    if (skip_rest_of_patch) {
		abort_hunk();
		failed++;
		if (verbose)
		    pfmt(stderr, MM_NOSTD, ":24:Hunk #%d ignored at %ld.\n",
			hunk, newwhere);
	    }
	    else if (where == Nulline) {
		abort_hunk();
		failed++;
		if (verbose)
		    pfmt(stderr, MM_NOSTD, ":25:Hunk #%d failed at %ld.\n",
			hunk, newwhere);
	    }
	    else {
		apply_hunk(where);
		if (verbose) {
		    pfmt(stderr, MM_NOSTD, ":26:Hunk #%d succeeded at %ld",
			hunk, newwhere);
		    if (fuzz)
			pfmt(stderr, MM_NOSTD, ":27: with fuzz %ld", fuzz);
		    if (last_offset==1L)
			pfmt(stderr, MM_NOSTD,":28: (offset %ld line)",
				last_offset);
		    else if (last_offset > 1L)
			pfmt(stderr, MM_NOSTD,":29: (offset %ld lines)",
				last_offset);
		    fprintf(stderr, ".\n");
		}
	    }
	}

	if (out_of_mem && using_plan_a) {
	    Argc = Argc_last;
	    Argv = Argv_last;
	    pfmt(stderr, MM_NOSTD, 
		":30:\n\nRan out of memory using Plan A--trying again...\n\n");
	    continue;
	}
    
	assert(hunk);
    
	/* finish spewing out the new file */
	if (!skip_rest_of_patch)
	    spew_output();
	
	/* and put the output where desired */
	ignore_signals();
	if (!skip_rest_of_patch) {
	    if (oopt != NULL)
		    append_file(TMPOUTNAME, oopt, &first_append);

	    if (move_file(TMPOUTNAME, outname) < 0) {
		toutkeep = TRUE;
		chmod(TMPOUTNAME, filemode);
	    }
	    else
		chmod(outname, filemode);
	}
	Fclose(rejfp);
	rejfp = Nullfp;
	if (failed) {
	    exitval=1;
	    if (!*rejname) {
		Strcpy(rejname, outname);
#ifndef FLEXFILENAMES
		{
		    char *s = strrchr(rejname,'/');

		    if (!s)
			s = rejname;
		    if (strlen(s) > 13)
			if (s[12] == '.')	/* try to preserve difference */
			    s[12] = s[13];	/* between .h, .c, .y, etc. */
			s[13] = '\0';
		}
#endif
		Strcat(rejname, REJEXT);
	    }
	    if (skip_rest_of_patch) {
		pfmt(stderr, MM_NOSTD,
		    ":31:%d out of %d hunks ignored--saving rejects to %s\n",
		    failed, hunk, rejname);
	    }
	    else {
		pfmt(stderr, MM_NOSTD,
		    ":32:%d out of %d hunks failed--saving rejects to %s\n",
		    failed, hunk, rejname);
	    }
	    if (move_file(TMPREJNAME, rejname) < 0)
		trejkeep = TRUE;
	}
	set_signals(1);
    }
    my_exit(exitval);
}

/* Prepare to find the next patch to do in the patch file. */

void
reinitialize_almost_everything()
{
    re_patch();
    re_input();

    input_lines = 0;
    last_frozen_line = 0;

    filec = 0;
    if (filearg[0] != Nullch && !out_of_mem) {
	/* string may also be referenced from table in case of multiple 
	    patches to same file, so cannot free here 
	free(filearg[0]); */
	filearg[0] = Nullch;
    }

    if (outname != Nullch) {
	free(outname);
	outname = Nullch;
    }

    last_offset = 0;

    diff_type = 0;

    if (revision != Nullch) {
	free(revision);
	revision = Nullch;
    }

    reverse = FALSE;
    skip_rest_of_patch = FALSE;

    get_some_switches();

    if (filec >= 2)
	fatal1(":33:You may not change to a different patch file.\n");
}

/* Process switches and filenames up to next '+' or end of list. */

void
get_some_switches()
{
    Reg1 char *s;
    Reg2 int c;

    rejname[0] = '\0';
    Argc_last = Argc;
    Argv_last = Argv;
    if (!Argc)
	return;

    opterr=0;	/* stop getopt from printing errors */
    while ((c = getopt(Argc,Argv, ":bB:cd:D:efF:i:lnNo:p:r:RsSvx:")) != -1) 
	    switch (c) {
	    case 'b':
		saveorig = TRUE;
		break;
	    case 'B':
		origprae = savestr(optarg);
		break;
	    case 'c':
		diff_type = CONTEXT_DIFF;
		break;
	    case 'd':
		if (chdir(optarg) < 0)
		    fatal2(":35:Can't cd to %s.\n", optarg);
		break;
	    case 'D':
	    	do_defines = TRUE;
		if (!isalpha(*optarg))
		    fatal1(":36:Argument to -D not an identifier.\n");
		Sprintf(if_defined, "#ifdef %s\n", optarg);
		Sprintf(not_defined, "#ifndef %s\n", optarg);
		Sprintf(end_defined, "#endif /* %s */\n", optarg);
		break;
	    case 'e':
		diff_type = ED_DIFF;
		break;
	    case 'f':
		force = TRUE;
		break;
	    case 'F':
		maxfuzz = atoi(optarg);
		break;
	    case 'i':
		filearg[1] = optarg;	/* patch input file */
		break;
	    case 'l':
		canonicalize = TRUE;
		break;
	    case 'n':
		diff_type = NORMAL_DIFF;
		break;
	    case 'N':
		noreverse = TRUE;
		break;
	    case 'o':
		oopt = savestr(optarg);
		break;
	    case 'p':
		strippath = atoi(optarg);
		break;
	    case 'r':
		Strcpy(rejname, optarg);
		break;
	    case 'R':
		reverse = TRUE;
		break;
	    case 's':
		verbose = FALSE;
		break;
	    case 'S':
		skip_rest_of_patch = TRUE;
		break;
	    case 'v':
		version();
		break;
#ifdef DEBUGGING
	    case 'x':
		debug = atoi(optarg);
		break;
#endif
	    case ':':	/* option-argument missing */
		if (optopt == 'p') {
			strippath = 0;
			break;
		} else if (optopt == 'F'){
			maxfuzz=0;
			break;
		} else 
			(void) pfmt(stderr, MM_ERROR,
				"uxlibc:2:Option requires an argument -- %c\n", 
				optopt);
		/* FALLTHROUGH */
	    default:
		if (c == '?') 
			(void) pfmt(stderr, MM_ERROR,
				"uxlibc:1:Illegal option -- %c\n", optopt);
		pfmt(stderr, MM_ACTION, 
		    ":37:Usage: patch [-NRSbflsv] [-c|-e|-n] [-B prefix] [-d dir]\n\t\t[-D define] [-F[fuzz]] [-i patchfile] [-o output]\n\t\t[-p[count]] [-r rejectfile] [original-file] [patch-file]\n");
		my_exit(1);
	    }
    Argc -= optind;
    Argv += optind;

    for (; Argc; Argc--,Argv++) {
	s = Argv[0];
	if (strEQ(s, "+")) {
	    Argv++; Argc--;
	    optind = 0;		/* reset getopt */
	    return;
	}
	if (filec == (MAXFILEC - 1))
	    fatal1(":34:Too many file arguments.\n");
	filearg[filec++] = savestr(s);
    }
}

/* Attempt to find the right place to apply this hunk of patch. */

LINENUM
locate_hunk(fuzz)
LINENUM fuzz;
{
    Reg1 LINENUM first_guess = pch_first() + last_offset;
    Reg2 LINENUM offset;
    LINENUM pat_lines = pch_ptrn_lines();
    Reg3 LINENUM max_pos_offset = input_lines - first_guess
				- pat_lines + 1; 
    Reg4 LINENUM max_neg_offset = first_guess - last_frozen_line - 1
				+ pch_context();

    if (!pat_lines)			/* null range matches always */
	return first_guess;
    if (max_neg_offset >= first_guess)	/* do not try lines < 0 */
	max_neg_offset = first_guess - 1;
    if (first_guess <= input_lines && patch_match(first_guess, Nulline, fuzz))
	return first_guess;
    for (offset = 1; ; offset++) {
	Reg5 bool check_after = (offset <= max_pos_offset);
	Reg6 bool check_before = (offset <= max_neg_offset);

	if (check_after && patch_match(first_guess, offset, fuzz)) {
#ifdef DEBUGGING
	    if (debug & 1)
		say3("Offset changing from %ld to %ld\n", last_offset, offset);
#endif
	    last_offset = offset;
	    return first_guess+offset;
	}
	else if (check_before && patch_match(first_guess, -offset, fuzz)) {
#ifdef DEBUGGING
	    if (debug & 1)
		say3("Offset changing from %ld to %ld\n", last_offset, -offset);
#endif
	    last_offset = -offset;
	    return first_guess-offset;
	}
	else if (!check_before && !check_after)
	    return Nulline;
    }
}

/* We did not find the pattern, dump out the hunk so they can handle it. */

void
abort_hunk()
{
    Reg1 LINENUM i;
    Reg2 LINENUM pat_end = pch_end();
    /* add in last_offset to guess the same as the previous successful hunk */
    LINENUM oldfirst = pch_first() + last_offset;
    LINENUM newfirst = pch_newfirst() + last_offset;
    LINENUM oldlast = oldfirst + pch_ptrn_lines() - 1;
    LINENUM newlast = newfirst + pch_repl_lines() - 1;
    char *stars = (diff_type == NEW_CONTEXT_DIFF ? " ****" : "");
    char *minuses = (diff_type == NEW_CONTEXT_DIFF ? " ----" : " -----");

    pfmt(rejfp, MM_NOSTD, ":91:***************\n");
    for (i=0; i<=pat_end; i++) {
	switch (pch_char(i)) {
	case '*':
	    if (oldlast < oldfirst)
		pfmt(rejfp, MM_NOSTD, ":92:*** 0%s\n", stars);
	    else if (oldlast == oldfirst)
		pfmt(rejfp, MM_NOSTD, ":93:*** %ld%s\n", oldfirst, stars);
	    else
		pfmt(rejfp, MM_NOSTD, ":94:*** %ld,%ld%s\n", oldfirst, oldlast, stars);
	    break;
	case '=':
	    if (newlast < newfirst)
		pfmt(rejfp, MM_NOSTD, ":95:--- 0%s\n", minuses);
	    else if (newlast == newfirst)
		pfmt(rejfp, MM_NOSTD, ":96:--- %ld%s\n", newfirst, minuses);
	    else
		pfmt(rejfp, MM_NOSTD, ":97:--- %ld,%ld%s\n", newfirst, newlast, minuses);
	    break;
	case '\n':
	    fprintf(rejfp, "%s", pfetch(i));
	    break;
	case ' ': case '-': case '+': case '!':
	    pfmt(rejfp, MM_NOSTD, ":98:%c %s", pch_char(i), pfetch(i));
	    break;
	default:
	    pfmt(stderr, MM_NOSTD,
		":40:Fatal internal error in abort_hunk().\n"); 
	    abort();
	}
    }
}

/* We found where to apply it (we hope), so do it. */

void
apply_hunk(where)
LINENUM where;
{
    Reg1 LINENUM old = 1;
    Reg2 LINENUM lastline = pch_ptrn_lines();
    Reg3 LINENUM new = lastline+1;
#define OUTSIDE 0
#define IN_IFNDEF 1
#define IN_IFDEF 2
#define IN_ELSE 3
    Reg4 int def_state = OUTSIDE;
    Reg5 bool R_do_defines = do_defines;
    Reg6 LINENUM pat_end = pch_end();

    where--;
    while (pch_char(new) == '=' || pch_char(new) == '\n')
	new++;
    
    while (old <= lastline) {
	if (pch_char(old) == '-') {
	    copy_till(where + old - 1);
	    if (R_do_defines) {
		if (def_state == OUTSIDE) {
		    fputs(not_defined, ofp);
		    def_state = IN_IFNDEF;
		}
		else if (def_state == IN_IFDEF) {
		    fputs(else_defined, ofp);
		    def_state = IN_ELSE;
		}
		fputs(pfetch(old), ofp);
	    }
	    last_frozen_line++;
	    old++;
	}
	else if (new > pat_end)
	    break;
	else if (pch_char(new) == '+') {
	    copy_till(where + old - 1);
	    if (R_do_defines) {
		if (def_state == IN_IFNDEF) {
		    fputs(else_defined, ofp);
		    def_state = IN_ELSE;
		}
		else if (def_state == OUTSIDE) {
		    fputs(if_defined, ofp);
		    def_state = IN_IFDEF;
		}
	    }
	    fputs(pfetch(new), ofp);
	    new++;
	}
	else {
	    if (pch_char(new) != pch_char(old)) {
		pfmt(stderr, MM_NOSTD,
		    ":41:Out-of-sync patch, lines %ld,%ld--mangled text or line numbers, maybe?\n",
		    pch_hunk_beg() + old,
		    pch_hunk_beg() + new);
#ifdef DEBUGGING
		say3("oldchar = '%c', newchar = '%c'\n",
		    pch_char(old), pch_char(new));
#endif
		my_exit(1);
	    }
	    if (pch_char(new) == '!') {
		copy_till(where + old - 1);
		if (R_do_defines) {
		   fputs(not_defined, ofp);
		   def_state = IN_IFNDEF;
		}
		while (pch_char(old) == '!') {
		    if (R_do_defines) {
			fputs(pfetch(old), ofp);
		    }
		    last_frozen_line++;
		    old++;
		}
		if (R_do_defines) {
		    fputs(else_defined, ofp);
		    def_state = IN_ELSE;
		}
		while (pch_char(new) == '!') {
		    fputs(pfetch(new), ofp);
		    new++;
		}
		if (R_do_defines) {
		    fputs(end_defined, ofp);
		    def_state = OUTSIDE;
		}
	    }
	    else {
		assert(pch_char(new) == ' ');
		old++;
		new++;
	    }
	}
    }
    if (new <= pat_end && pch_char(new) == '+') {
	copy_till(where + old - 1);
	if (R_do_defines) {
	    if (def_state == OUTSIDE) {
	    	fputs(if_defined, ofp);
		def_state = IN_IFDEF;
	    }
	    else if (def_state == IN_IFNDEF) {
		fputs(else_defined, ofp);
		def_state = IN_ELSE;
	    }
	}
	while (new <= pat_end && pch_char(new) == '+') {
	    fputs(pfetch(new), ofp);
	    new++;
	}
    }
    if (R_do_defines && def_state != OUTSIDE) {
	fputs(end_defined, ofp);
    }
}

/* Open the new file. */

void
init_output(name)
char *name;
{
    ofp = fopen(name, "w");
    if (ofp == Nullfp)
	fatal2(":42: can't create %s.\n", name);
}

/* Open a file to put hunks we can't locate. */

void
init_reject(name)
char *name;
{
    rejfp = fopen(name, "w");
    if (rejfp == Nullfp)
	fatal2(":42: can't create %s.\n", name);
}

/* Copy input file to output, up to wherever hunk is to be applied. */

void
copy_till(lastline)
Reg1 LINENUM lastline;
{
    Reg2 LINENUM R_last_frozen_line = last_frozen_line;

    if (R_last_frozen_line > lastline)
	pfmt(stderr, MM_NOSTD,
		":43: misordered hunks! output will be garbled.\n");
    while (R_last_frozen_line < lastline) {
	dump_line(++R_last_frozen_line);
    }
    last_frozen_line = R_last_frozen_line;
}

/* Finish copying the input file to the output file. */

void
spew_output()
{
#ifdef DEBUGGING
    if (debug & 256)
	say3("il=%ld lfl=%ld\n",input_lines,last_frozen_line);
#endif
    if (input_lines)
	copy_till(input_lines);		/* dump remainder of file */
    Fclose(ofp);
    ofp = Nullfp;
}

/* Copy one line from input to output. */

void
dump_line(line)
LINENUM line;
{
    Reg1 char *s;
    Reg2 char R_newline = '\n';

    /* Note: string is not null terminated. */
    for (s=ifetch(line, 0); putc(*s, ofp) != R_newline; s++) ;
}

/* Does the patch pattern match at line base+offset? */

bool
patch_match(base, offset, fuzz)
LINENUM base;
LINENUM offset;
LINENUM fuzz;
{
    Reg1 LINENUM pline = 1 + fuzz;
    Reg2 LINENUM iline;
    Reg3 LINENUM pat_lines = pch_ptrn_lines() - fuzz;

    for (iline=base+offset+fuzz; pline <= pat_lines; pline++,iline++) {
	if (canonicalize) {
	    if (!similar(ifetch(iline, (offset >= 0)),
			 pfetch(pline),
			 pch_line_len(pline) ))
		return FALSE;
	}
	else if (strnNE(ifetch(iline, (offset >= 0)),
		   pfetch(pline),
		   pch_line_len(pline) ))
	    return FALSE;
    }
    return TRUE;
}

/* Do two lines match with canonicalized white space? */

bool
similar(a,b,len)
Reg1 char *a;
Reg2 char *b;
Reg3 int len;
{
    while (len) {
	if (isspace(*b)) {		/* whitespace (or \n) to match? */
	    if (!isspace(*a))		/* no corresponding whitespace? */
		return FALSE;
	    while (len && isspace(*b) && *b != '\n')
		b++,len--;		/* skip pattern whitespace */
	    while (isspace(*a) && *a != '\n')
		a++;			/* skip target whitespace */
	    if (*a == '\n' || *b == '\n')
		return (*a == *b);	/* should end in sync */
	}
	else if (*a++ != *b++)		/* match non-whitespace chars */
	    return FALSE;
	else
	    len--;			/* probably not necessary */
    }
    return TRUE;			/* actually, this is not reached */
					/* since there is always a \n */
}

/* Exit with cleanup. */

void
my_exit(status)
int status;
{
    Unlink(TMPINNAME);
    if (!toutkeep) {
	Unlink(TMPOUTNAME);
    }
    if (!trejkeep) {
	Unlink(TMPREJNAME);
    }
    Unlink(TMPPATNAME);
    if (oopt != NULL)
	clean_map();
	
    exit(status);
}
