/*	copyright	"%c%"	*/
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

#ident	"@(#)patch_p2:util.c	1.1.1.3"

/*
 * OSF/1 1.2
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile$ $Revision$ (OSF) $Date$";
#endif

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
#include <signal.h>
#include <fcntl.h>
#include <limits.h>
#include "EXTERN.h"
#include "config.h"
#include "common.h"
#include "util.h"
#include "pathnames.h"

/*
 * Backup a target file if necessary
 */

static int
dobackup(char *from, char *to) {
Reg1 char *s;
char bakname[PATH_MAX];

    if (origprae) {
	Strcpy (bakname, origprae);
	Strcat(bakname, to);
    } else {
	Strcpy(bakname, to);
	Strcat(bakname, ORIGEXT);
    }
    if (stat(to, &filestat) >= 0) {	/* output file exists */
	dev_t to_device = filestat.st_dev;
	ino_t to_inode  = filestat.st_ino;
	char *simplename = bakname;
	
	if (saveorig) {			/* for POSIX behavior */
	for (s=bakname; *s; s++) {
	    if (*s == '/')
		simplename = s+1;
	}
	/* find a backup name that is not the same file */
	while (stat(bakname, &filestat) >= 0 &&
		to_device == filestat.st_dev && to_inode == filestat.st_ino) {
	    for (s=simplename; *s && !islower(*s); s++) ;
	    if (*s)
		*s = toupper(*s);
	    else
		Strcpy(simplename, simplename+1);
	}
	while (unlink(bakname) >= 0) ;	/* while() is for benefit of Eunice */
#ifdef DEBUGGING
	if (debug & 4)
	    say3("Moving %s to %s.\n", to, bakname);
#endif
	if (link(to, bakname) < 0) {
	    pfmt(stderr, MM_ERROR,
		":83: can't backup %s, output is in %s\n",
		to, from);
	    return -1;
	}
	} /* if saveorig */
	while (unlink(to) >= 0) ;
    }
    return 0;
}

/* Rename a file, copying it if necessary. */

int
move_file(from,to)
char *from, *to;
{
    Reg2 int i;
    Reg3 int fromfd;

    /* to stdout? */

    if (strEQ(to, "-")) {
#ifdef DEBUGGING
	if (debug & 4)
	    say2("Moving %s to stdout.\n", from);
#endif
	fromfd = open(from, 0);
	if (fromfd < 0)
	    fatal2(":81: internal error, can't reopen %s\n", from);
	while ((i=read(fromfd, buf, sizeof buf)) > 0)
	    if (write(1, buf, i) != 1)
		fatal1("82: write failed\n");
	Close(fromfd);
	return 0;
    }

    if (dobackup(from, to) < 0)
	return -1;

#ifdef DEBUGGING
    if (debug & 4)
	say3("Moving %s to %s.\n", from, to);
#endif
    if (link(from, to) < 0) {		/* different file system? */
	Reg4 int tofd;
	
	tofd = creat(to, 0666);
	if (tofd < 0) {
	    pfmt(stderr, MM_ERROR, ":84: can't create %s, output is in %s.\n",
	      to, from);
	    return -1;
	}
	fromfd = open(from, 0);
	if (fromfd < 0)
	    fatal2(":81: internal error, can't reopen %s\n", from);
	while ((i=read(fromfd, buf, sizeof buf)) > 0)
	    if (write(tofd, buf, i) != i)
		fatal1(":82: write failed\n");
	Close(fromfd);
	Close(tofd);
    }
    Unlink(from);
    return 0;
}

/* 
 * Append to a file.  (unless first == 1, in which case back up the file
 * and re-create it.)
 */

void
append_file(from,to,first)
char *from, *to;
int *first;
{
    Reg3 int tofd;
    Reg2 int fromfd;
    Reg1 int i;
    
    if (*first) {
	    if (dobackup(from, to) < 0)
		return;
 	    *first = 0;
	    tofd = open(to, O_RDWR|O_CREAT|O_TRUNC, 0666);
   } else 
	    tofd = open(to, O_RDWR|O_CREAT, 0666);

    if (tofd < 0)
	fatal2(":42: can't create %s.\n", to);
    fromfd = open(from, 0);
    if (fromfd < 0)
	fatal2(":81: internal error, can't reopen %s\n", from);
    if (lseek(tofd, 0, SEEK_END) < 0) 
	fatal2(":104: can't seek to end of %s.\n", to);
    while ((i=read(fromfd, buf, sizeof buf)) > 0)
	if (write(tofd, buf, i) != i)
	    fatal2(":85: write (%s) failed\n", to);
    Close(fromfd);
    Close(tofd);
}


/* Copy a file. */

void
copy_file(from,to)
char *from, *to;
{
    Reg3 int tofd;
    Reg2 int fromfd;
    Reg1 int i;
    
    tofd = creat(to, 0666);
    if (tofd < 0)
	fatal2(":42: can't create %s.\n", to);
    fromfd = open(from, 0);
    if (fromfd < 0)
	fatal2(":81: internal error, can't reopen %s\n", from);
    while ((i=read(fromfd, buf, sizeof buf)) > 0)
	if (write(tofd, buf, i) != i)
	    fatal2(":85: write (%s) failed\n", to);
    Close(fromfd);
    Close(tofd);
}

/* Allocate a unique area for a string. */

char *
savestr(s)
Reg1 char *s;
{
    Reg3 char *rv;
    Reg2 char *t;

    if (!s)
	s = "Oops";
    t = s;
    while (*t++);
    rv = (char *)malloc((MEM) (t - s));
    if (rv == Nullch) {
	if (using_plan_a)
	    out_of_mem = TRUE;
	else
	    fatal1(":86: out of memory (savestr)\n");
    }
    else {
	t = rv;
	while (*t++ = *s++);
    }
    return rv;
}

#if defined(lint) && defined(CANVARARG)

/*VARARGS ARGSUSED*/
say(pat) char *pat; { ; }
/*VARARGS ARGSUSED*/
fatal(pat) char *pat; { ; }
/*VARARGS ARGSUSED*/
ask(pat) char *pat; { ; }

#else

/* Vanilla terminal output (buffered). */

void
say(pat,arg1,arg2,arg3)
char *pat;
long arg1,arg2,arg3;
{
    (void) fprintf(stderr, pat, arg1, arg2, arg3);
    Fflush(stderr);
}

/* Terminal output, pun intended. */

void 			/* very void */
fatal(pat,arg1,arg2,arg3)
char *pat;
long arg1,arg2,arg3;
{
    void my_exit();

    pfmt(stderr, MM_ERROR, pat, arg1, arg2, arg3);
    my_exit(1);
}

/* Get a response from the user, somehow or other. */

void
ask(pat,arg1,arg2,arg3)
char *pat;
long arg1,arg2,arg3;
{
    FILE * ttyfd;
    int r;
    bool tty2 = isatty(2);

    (void) pfmt(stderr, MM_NOSTD, pat, arg1, arg2, arg3);
    Fflush(stderr);
    if (tty2) {				/* might be redirected to a file */
	r = read(2, buf, sizeof buf);
    }
    else if (isatty(1)) {		/* this may be new file output */
        (void) pfmt(stdout, MM_NOSTD, pat, arg1, arg2, arg3);
	Fflush(stdout);
	r = read(1, buf, sizeof buf);
    }
    else if ((ttyfd = fopen(_PATH_TTY, "w")) != NULL && isatty(fileno(ttyfd))) {
					/* might be deleted or unwriteable */
        (void) pfmt(ttyfd, MM_NOSTD, pat, arg1, arg2, arg3);
	Fflush(ttyfd);
	r = read(fileno(ttyfd), buf, sizeof buf);
	Fclose(ttyfd);
    }
    else if (isatty(0)) {		/* this is probably patch input */
        (void) pfmt(stdin, MM_NOSTD, pat, arg1, arg2, arg3);
	Fflush(stdin);
	r = read(0, buf, sizeof buf);
    }
    else {				/* no terminal at all--default it */
	buf[0] = '\n';
	r = 1;
    }
    if (r <= 0)
	buf[0] = 0;
    else
	buf[r] = '\0';
    if (!tty2)
	say1(buf);
}
#endif /* lint */

/* How to handle certain events when not in a critical region. */

void
set_signals(reset)
int reset;
{
    void my_exit();
#ifndef lint
#ifdef VOIDSIG
    static void (*hupval)(),(*intval)();
#else
    static int (*hupval)(),(*intval)();
#endif

    if (!reset) {
	hupval = signal(SIGHUP, SIG_IGN);
	if (hupval != SIG_IGN)
#ifdef VOIDSIG
	    hupval = my_exit;
#else
	    hupval = (int(*)())my_exit;
#endif
	intval = signal(SIGINT, SIG_IGN);
	if (intval != SIG_IGN)
#ifdef VOIDSIG
	    intval = my_exit;
#else
	    intval = (int(*)())my_exit;
#endif
    }
    Signal(SIGHUP, hupval);
    Signal(SIGINT, intval);
#endif
}

/* How to handle certain events when in a critical region. */

void
ignore_signals()
{
#ifndef lint
    Signal(SIGHUP, SIG_IGN);
    Signal(SIGINT, SIG_IGN);
#endif
}

/* Make sure we'll have the directories to create a file. */

void
makedirs(filename,striplast)
Reg1 char *filename;
bool striplast;
{
    char tmpbuf[256];
    Reg2 char *s = tmpbuf;
    char *dirv[20];
    Reg3 int i;
    Reg4 int dirvp = 0;

    while (*filename) {
	if (*filename == '/') {
	    filename++;
	    dirv[dirvp++] = s;
	    *s++ = '\0';
	}
	else {
	    *s++ = *filename++;
	}
    }
    *s = '\0';
    dirv[dirvp] = s;
    if (striplast)
	dirvp--;
    if (dirvp < 0)
	return;
    strcpy(buf, "mkdir");
    s = buf;
    for (i=0; i<=dirvp; i++) {
	while (*s) s++;
	*s++ = ' ';
	strcpy(s, tmpbuf);
	*dirv[i] = '/';
    }
    system(buf);
}

/* Make filenames more reasonable. */

char *
fetchname(at,strip_leading,assume_exists)
char *at;
int strip_leading;
int assume_exists;
{
    char *s;
    char *name;
    Reg1 char *t;
    char tmpbuf[200];

    if (!at)
	return Nullch;
    s = savestr(at);
    for (t=s; isspace(*t); t++) ;
    name = t;
#ifdef DEBUGGING
    if (debug & 128)
	say4("fetchname %s %d %d\n",name,strip_leading,assume_exists);
#endif
    if (strnEQ(name, "/dev/null", 9))	/* so files can be created by diffing */
	return Nullch;			/*   against /dev/null. */
    for (; *t && !isspace(*t); t++)
	if (*t == '/')
	    if (--strip_leading >= 0)
		name = t+1;
    *t = '\0';
    if (name != s && *s != '/') {
	name[-1] = '\0';
	if (stat(s, &filestat) && filestat.st_mode & S_IFDIR) {
	    name[-1] = '/';
	    name=s;
	}
    }
    name = savestr(name);
    Sprintf(tmpbuf, "RCS/%s", name);
    free(s);
    if (stat(name, &filestat) < 0 && !assume_exists) {
	Strcat(tmpbuf, RCSSUFFIX);
	if (stat(tmpbuf, &filestat) < 0 && stat(tmpbuf+4, &filestat) < 0) {
	    Sprintf(tmpbuf, "SCCS/%s%s", SCCSPREFIX, name);
	    if (stat(tmpbuf, &filestat) < 0 && stat(tmpbuf+5, &filestat) < 0) {
		free(name);
		name = Nullch;
	    }
	}
    }
    return name;
}

/*
 * for the -o option; keep a mapping of real file names to temporary file.
 *
 * Set outname to the temporary file.
 *
 * It is unlikely that the -o option will be used for a large number of
 * input files so a linear search is done.
 */

struct map {
	char *from;
	char *to;
};
static struct map *table = NULL;
static size_t nmaps = 0;
static size_t mapsize = 0;

char *
mapinput(char *filename) {
int i;

  if (oopt == NULL)
	return filename;

  for (i = 0; i < nmaps ; i++)
	if (strcmp(filename, table[i].from) == 0)
		return outname = savestr(table[i].to);

  /*
   * Not found - create a new mapping.
   */

  if (i == mapsize) {
	mapsize += 100;
	if ((table = realloc(table, mapsize * sizeof(struct map))) == NULL)
	    fatal1(":105: out of memory (mapinput)\n");
  }

  table[i].to = savestr(tmpnam(NULL));
  table[i].from = filename;
  outname = savestr(table[i].to);
  nmaps = i+1;

  return filename; /* Read from the original file first time round */
}

void
clean_map(void){
int i;

  for (i = 0; i < nmaps ; i++)
	Unlink(table[i].to);
}
