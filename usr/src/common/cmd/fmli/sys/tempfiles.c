/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/tempfiles.c	1.8.3.4"

#include	<stdio.h>
#include	<string.h>
#include	<signal.h>
#include	<fcntl.h>
#include	<errno.h>
#include	"inc.types.h"	  /* abs s14 */
#include	"wish.h"
#include	"retcodes.h"
#include	"var_arrays.h"
#include	"terror.h"
#include	"moremacros.h"

#define LCKPREFIX	".L"

/*
 * globals used throughout the Telesystem
 */
extern char	**Remove;

/*
 * make an entry in the Remove table and return its index
 */
static int
makentry(path)
char	path[];
{
	static int	nextremove;	/* next entry in Remove to look at */

	if (path == NULL)
		return -1;
	if (Remove == NULL)
		Remove = (char **)array_create(sizeof(char *), 10);
	Remove = (char **)array_append(Remove, &path);
	return array_len(Remove) - 1;
}

/*
 * Remove an entry from the Remove table (presumably because
 * the file could not be created)
 */
static void
rmentry(ent)
int	ent;
{
	register char	*ptr;

	ptr = Remove[ent];
	Remove = (char **)array_delete(Remove, ent);
	free(ptr);
}

static
putdec(n, fd)
pid_t	n;			/* EFT abs k16 */
int	fd;
{
	char	buf[16];

	sprintf(buf, "%d\n", n);
	write(fd, buf, (int)strlen(buf));
}

static
getdec(fd)
int	fd;
{
	char	buf[16];
	register int	n;
	register pid_t	pid;	/* EFT abs k16 */

	n = read(fd, buf, sizeof buf);
	if (n > 1 && buf[n - 1] == '\n' &&
	    (pid = (pid_t)strtol(buf,(char **)NULL, 0)) > 1)  /* EFT abs k16 */
		return pid;
	else
		return -1;
}

/*
 * eopen performs an fopen with some good things added for temp files
 * If the mode starts with "t" the file will be unlinked immediately
 * after creation.  If the mode starts with "T", the file will be
 * removed when the program exits - normally or from the receipt
 * of signal 1, 2, 3, or 15.
 * SIDE EFFECT: for temp files, calls "mktemp(3)" on "path"
 */
FILE *
eopen(path, mode)
char	path[];
char	mode[];
{
	register int	ent;
	register FILE	*fp;
	char	*mktemp();

	switch (mode[0]) {
	case 'T':
	case 't':
		mktemp(path);
		if ((ent = makentry(strsave(path))) < 0)
			return NULL;
		fp = fopen(path, mode + 1);
		if (mode[0] == 't' && unlink(path) == 0)
			rmentry(ent);
		break;
	default:
		fp = fopen(path, mode);
		break;
	}
	return fp;
}

/*
 * make a tempfile using "eopen()"
 * if the path is null, one is provided
 * if the mode starts with neither "t" nor "T"
 * it defaults to "t"
 */
FILE *
tempfile(path, mode)
char	path[];
char	mode[];
{
	char	newmode[8];
	char	save[20];	/* based on length of string below */

	if (path == NULL) {
		path = save;
		strcpy(path, "/tmp/wishXXXXXX");
	}
	if (mode[0] != 't' && mode[0] != 'T') {
		newmode[0] = 't';
		strncpy(newmode + 1, mode, sizeof(mode) - 2);
		newmode[sizeof(mode) - 1] = '\0';
		mode = newmode;
	}
	return eopen(path, mode);
}
