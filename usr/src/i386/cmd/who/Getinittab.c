#ident	"@(#)who:i386/cmd/who/Getinittab.c	1.3"

/*
 * Routines to deal with the inittab file.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <pfmt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utmp.h>

/*
 * The pathname of the inittab file and a (perhaps empty)
 * list of alternate places to look.  Some implementations
 * may not have any alternates, but looking for them
 * won't hurt anyway, and it helps us keep code common.
 */
static char *initpath[] = {
	"/etc/inittab",			/* standard location, must be first */
	"/etc/conf/init.d/kernel",	/* i386 SVR4.2 alternate */
	"/etc/conf/cf.d/init.base",	/* i386 SVR4.0 alternate */
	0
};

static char *inittab;			/* contents of inittab file */
static char *initp;			/* current location in inittab buffer */

/*
 * Save system call error number and type of first failure.
 * The dangling "else" in this macro definition allows the macro to be used
 * in various contexts without causing syntax errors or requiring extra braces.
 */
#define ERR(e)	if (failure == 0) { saverr = errno; failure = (e); } else

/*
 * Read the contents of the inittab file into memory for later use.
 * If something goes wrong, try the alternate files (if any).
 * If forced to give up, issue an appropriate error message
 * determined by the FIRST failure and exit.
 */

void
getinittab()
{
	char	**p;			/* current entry in initpath[] */
	struct	stat statb;		/* buffer for stat(2) */
	int	saverr;			/* saved errno from first failure */
	int	failure = 0;		/* kind of failure */
	int	fildes;			/* file descriptor for inittab */

	for (p = initpath; *p; p++) {
#ifdef DEBUG
		printf("trying inittab file %s\n", *p);
#endif
		if (stat(*p, &statb) == -1)
			ERR(1);
		else if ((fildes = open(*p, O_RDONLY)) == -1)
			ERR(2);
		else if (statb.st_size <= 0) {
			ERR(3);
			close(fildes);
		} else if ((inittab = malloc(statb.st_size + 2)) == NULL) {
			pfmt(stderr, MM_ERROR,
				":752:Cannot allocate %d bytes: %s\n",
				statb.st_size, strerror(errno));
			exit(1);
		} else if (read(fildes, inittab, statb.st_size) != statb.st_size) {
			ERR(4);
			free(inittab);
			close(fildes);
		} else {
			/*
			 * Succeeded.  Make sure last line is properly
			 * terminated; makes code in getinitcomment() easier.
			 * Leave initp pointing at terminating '\0'.
			 */
			initp = &inittab[statb.st_size - 1];
			if (*initp++ != '\n')
				*initp++ = '\n';
			*initp = '\0';
			close(fildes);
			return;
		}
	}

	/*
	 * If we fall through to here, we failed completely.
	 */
	switch (failure) {
	case 1:
		pfmt(stderr, MM_ERROR, ":5:Cannot access %s: %s\n",
			initpath[0], strerror(saverr));
		break;
	case 2:
		pfmt(stderr, MM_ERROR, ":4:Cannot open %s: %s\n",
			initpath[0], strerror(saverr));
		break;
	case 3:
		pfmt(stderr, MM_ERROR|MM_NOGET, "%s is empty\n", initpath[0]);
		break;
	case 4:
		pfmt(stderr, MM_ERROR, ":341:Read error in %s: %s\n",
			initpath[0], strerror(saverr));
		break;
	}

	exit(1);
}

/*
 * Search for the specified entry in the inittab file.
 * If found, return the comment string from the entry;
 * otherwise, return an empty string.
 *
 * We keep our place from search to search as a minor optimization.
 * Wrap once if needed for each entry.
 */

char *
getinitcomment(id)
	char	*id;
{
	int	len;
	int	i;
	int	wrapped = 0;
	char	c;
	static	char comment[80];	/* arbitrary size */

	/*
	 * The argument is a utmp ut_id and might not be null-terminated.
	 * Compute its length.
	 */
	for (len = 0; len < sizeof(((struct utmp *)0)->ut_id); len++)
		if (id[len] == '\0')
			break;
#ifdef DEBUG
	printf("\nseeking inittab id '%.4s', len=%d\n", id, len);
#endif

	if (len == 0)
		return "";

	/*
	 * Look for a line beginning with id.
	 */
	for (;;) {
		/* if at end of buffer, reset to start */
		if (*initp == '\0') {
			if (wrapped++)
				break;
			initp = inittab;
		}
#ifdef DEBUG
		printf("inittab id '%.*s'\n", strcspn(initp, ":"), initp);
#endif
		/* do we have an exact match? */
		if (strncmp(id, initp, len) == 0 && initp[len] == ':')
			break;

		/* no match, skip to next line */
		while (*initp++ != '\n')
			;
	}
	
	/*
	 * If we didn't find our entry, just return an empty string.
	 */
	if (*initp == '\0')
		return "";

	/*
	 * We found our entry.  Look for the start of the comment.
	 */
	while(*initp != '#' && *initp != '\n')
		initp++;

	/*
	 * If there is no comment, return an empty string.
	 */
	if (*initp++ == '\n')
		return "";

	/*
	 * Found a comment.  Skip leading whitespace.
	 */
	while(*initp == ' ' || *initp == '\t')
		initp++;

	/*
	 * Save the remainder, being careful to avoid overflow.
	 * Leave initp pointing to the start of the next line.
	 */
	for (i = 0; (c = *initp++) != '\n'; )
		if (i < sizeof(comment) - 1)
			comment[i++] = c;

	comment[i] = '\0';
	return comment;
}
