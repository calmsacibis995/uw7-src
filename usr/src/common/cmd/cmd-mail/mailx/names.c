#ident	"@(#)names.c	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident "@(#)names.c	1.18 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Handle name lists.
 */

#include "rcv.h"

static struct name	*nalloc ARGS((char*,int));
static int		isfileaddr ARGS((char *name));
static int		lengthof ARGS((struct name *name));
static char		*norm ARGS((const char *user, char *ubuf, int nbangs, const char* netprecedence));

/*
 * Allocate a single element of a name list,
 * initialize its name field to the passed
 * name and return it.
 */

static struct name *
nalloc(str, ntype)
	char *str;
{
	register struct name *np = (struct name *) salloc(sizeof *np);
	np->n_link = 0;
	np->n_type = (short) ntype;
	np->n_full = savestr(str);
	np->n_name = skin(np->n_full);
	return np;
}

/*
 * Find the tail of a list and return it.
 */

struct name *
tailof(np)
	register struct name *np;
{
	if (np)
		while (np->n_link)
			np = np->n_link;
	return np;
}

/*
 * Extract a list of names from a line,
 * and make a list of names from it.
 * Return the list or NIL if none found.
 */

struct name *
extract(line, ntype)
	register char *line;
{
	char nbuf[BUFSIZ];
	struct name head, *np = &head;
	int comma = docomma(line);

	head.n_link = 0;
	while ((line = yankword(line, nbuf, comma)) != 0)
		np = np->n_link = nalloc(nbuf, ntype);
	return head.n_link;
}

/*
 * Turn a list of names into a string of the same names.
 */

char *
detract(np, ntype)
	register struct name *np;
{
	register int s;
	register char *cp, *line;
	register struct name *t;

	for (s = 0, t = np; t; t = t->n_link)
		if (t->n_type == ntype)
			s += strlen(t->n_full) + 2;
	if (s == 0)
		return 0;
	line = salloc((unsigned)(++s));
	for (cp = line; np; np = np->n_link)
		if (np->n_type == ntype)
			cp = copy(", ", copy(np->n_full, cp));
	return line;
}

struct name *
outpre(to)
	struct name *to;
{
	register struct name *np;

	for (np = to; np; np = np->n_link)
		if (any(*np->n_name, "+|"))
			np->n_type |= GDEL;
	return to;
}

/*
 * For each recipient in the passed name list with a /
 * in the name, append the message to the end of the named file
 * and remove it from the recipient list.
 *
 * Recipients whose name begins with | are piped through the given
 * program and removed.
 */

int
outof(np, hp, fo, pos)
	register struct name *np;
	struct header *hp;
	FILE *fo;
	off_t pos;
{
	const char *fname, *shellcmd;
	FILE *fout, *fin;
	int ispipe, s;
	int nout = 0;

	image = -1;
	for (; np; np = np->n_link) {
		ispipe = np->n_name[0] == '|';
		if (!isfileaddr(np->n_name) && !ispipe)
			continue;
		nout++;
		fname = ispipe ? np->n_name+1 : safeexpand(np->n_name);

		/*
		 * See if we have copied the complete message out yet.
		 * If not, do so.
		 */
		if (image == -1
		    && (savemail(tempEdit, TEMPPERM, hp, fo, pos, 0, 1) == -1
			|| (image = open(tempEdit, O_RDONLY)) == -1
			|| unlink(tempEdit) == -1)) {
			senderr++;
			goto cant;
		}

		/*
		 * Now either copy "image" to the desired file
		 * or give it as the standard input to the desired
		 * program as appropriate.
		 */
		if (ispipe) {
			wait(&s);
			switch (fork()) {
			case 0:
				sigchild();
				sigset(SIGHUP, SIG_IGN);
				sigset(SIGINT, SIG_IGN);
				sigset(SIGQUIT, SIG_IGN);
				close(0);
				dup(image);
				close(image);
				lseek(0, 0L, 0);
				if ((shellcmd = value("SHELL")) == NOSTR || *shellcmd=='\0')
					shellcmd = SHELL;
				execlp(shellcmd, shellcmd, "-c", fname, (char *)0);
				pfmt(stderr, MM_ERROR, badexec, shellcmd,
					Strerror(errno));
				fflush(stderr);
				_exit(1);
				break;

			case (pid_t)-1:
				pfmt(stderr, MM_ERROR, failed, "fork",
					Strerror(errno));
				senderr++;
				goto cant;
			}
		}
		else {
			if ((fout = fopen(fname, "a")) == NULL) {
				pfmt(stderr, MM_ERROR, badopen,
					fname, Strerror(errno));
				senderr++;
				goto cant;
			}
			fin = Fdopen(image, "r");
			if (fin == NULL) {
				pfmt(stderr, MM_ERROR, badopen, image,
					Strerror(errno));
				fclose(fout);
				senderr++;
				goto cant;
			}
			rewind(fin);
			copystream(fin, fout);
			if (ferror(fout))
				senderr++, pfmt(stderr, MM_ERROR,
					badwrite, fname, Strerror(errno));
			fclose(fout);
			fclose(fin);
		}
cant:
		/*
		 * For sake of header expansion we leave the
		 * entry in the list and mark it as deleted.
		 */
		np->n_type |= GDEL;
	}
	if (image >= 0) {
		close(image);
		image = -1;
	}
	return(nout);
}

/*
 * Determine if the passed address is a local "send to file" address.
 */
static int
isfileaddr(filename)
	char *filename;
{
	return !any('@', filename) && !any('!', filename)
		&& (*filename == '+' || any('/', filename));
}

/*
 * Map all of the aliased users in the invoker's mailrc
 * file recursively and insert them into the list.
 */
struct name *
usermap(np)
	register struct name *np;
{
	struct name head, *t = &head, *n;
	struct grouphead *gh;
	struct mgroup *ge;
	static int depth = 0, metoo;

	if (depth == 0)
		metoo = value("metoo") != 0;
	if (depth > MAXEXP) {
		pfmt(stdout, MM_ERROR, 
			":318:Expanding alias to depth larger than %d\n",
			MAXEXP);
		return np;
	}
	depth++;
	for (head.n_link = 0; np; np = np->n_link)
		if (np->n_name[0] != '\\' && ((gh = findgroup(np->n_name)) != 0))
			for (ge = gh->g_list; ge; ge = ge->ge_link) {
				n = nalloc(ge->ge_name, np->n_type);
				if ((ge == gh->g_list && !ge->ge_link)
				 || metoo || !samebody(myname, ge->ge_name)) {
					t->n_link = equal(ge->ge_name, gh->g_name)
							? n : usermap(n);
					t = tailof(t);
				}
			}
		else
			t = t->n_link = np;
	depth--;
	return head.n_link;
}

/*
 * Normalize a network name for comparison purposes.
 */
static char *
norm(user, ubuf, nbangs, netprecedence)
	register const char *user;
	register char *ubuf;
	int nbangs;
	const char *netprecedence;
{
	int hassystem = 0;
	register char *cp, *endptr = ubuf;
	char userbuf[1024];

	ubuf[0] = '\0';
	strcpy(userbuf, user);
	user = userbuf;

	/* strip off leading !'s */
	while (*user++ == '!')
		;
	user--;

        for ( ; netprecedence[0] && netprecedence[1]; netprecedence += 2)
		switch (netprecedence[1]) {
		case 'r':
			/* Strip off a right associative networking address, */
			/* such as the domain from user@domain. */
			while ((cp = strrchr(user, netprecedence[0])) != 0) {
				*cp++ = '\0';
				if (*cp) {	/* check for @ at end */
					strcpy(endptr, cp);
					endptr += strlen(cp);
					*endptr++ = '!';
					hassystem = 1;
				}
			}
			break;
			
		case 'l':
			/* Strip off a left associative networking address, */
			/* such as the system from system!user. */
			while ((cp = strchr(user, netprecedence[0])) != 0) {
				*cp++ = '\0';
				strcpy(endptr, user);
				endptr += strlen(user);
				*endptr++ = '!';
				hassystem = 1;
				user = cp;
			}
			break;
		}

	*endptr = '\0';

	/* all that's left is the user name */
	if (hassystem) {
		strcpy(endptr, user);
		endptr += strlen(user);
	} else {
		sprintf(ubuf, "%s!%s", host, user);
		endptr = ubuf + strlen(ubuf);
	}

	/* The normalized network name is now in ubuf. */
	/* If allnet, either strip off all system names or leave just one. */
	if (nbangs) {
		while (nbangs--)
			while (endptr > ubuf && *--endptr != '!');
		ubuf = (endptr > ubuf) ? ++endptr : endptr;
	}
	return ubuf;
}

/*
 * Implement allnet options.
 */
samebody(user, addr)
	register const char *user, *addr;
{
	char ubuf[500], abuf[500];
	const char *allnet = value("allnet");
	int nbangs = allnet ? !strcmp(allnet, "uucp") ? 2 : 1 : 0;
	const char *netprecedence = value("netprecedence");

	if (!netprecedence) netprecedence = "@r!l%r";

	user = norm(user, ubuf, nbangs, netprecedence);
	addr = norm(addr, abuf, nbangs, netprecedence);
	return strcmp(user, addr) == 0;
}

/*
 * Compute the length of the passed name list and
 * return it.
 */
static int
lengthof(np)
	register struct name *np;
{
	register int c;

	for (c = 0; np; c++, np = np->n_link)
		;
	return c;
}

/*
 * Concatenate the two passed name lists, return the result.
 */

struct name *
cat(n1, n2)
	struct name *n1, *n2;
{
	if (!n1)
		return n2;
	tailof(n1)->n_link = n2;
	return n1;
}

/*
 * Unpack the name list onto a vector of strings.
 * Return an error if the name list won't fit.
 */

char **
unpack(np)
	register struct name *np;
{
	register char **ap, **toptr;
	char hbuf[10];
	int t, extra;

	if ((t = lengthof(np)) == 0)
		panic(":319:No names to unpack");

	/*
	 * Compute the number of extra arguments we will need.
	 * We need at least 2 extra -- one for "mail" and one for
	 * the terminating 0 pointer.
	 * Additional spots may be needed to pass along -r and -f to
	 * the host mailer.
	 */

	extra = 2;
	if (rflag)
		extra += 2;
	if (hflag)
		extra += 2;
	toptr = (char **) salloc((t + extra) * sizeof (char *));
	ap = toptr;
	*ap++ = "mail";
	if (rflag) {
		*ap++ = "-r";
		*ap++ = rflag;
	}
	if (hflag) {
		*ap++ = "-h";
		sprintf(hbuf, "%d", hflag);
		*ap++ = savestr(hbuf);
	}
	for (; np; np = np->n_link)
		if ((np->n_type & GDEL) == 0)
			*ap++ = np->n_name;
	*ap = 0;
	return(toptr);
}

/*
 * See if the user named himself as a destination
 * for outgoing mail.  If so, set the global flag
 * selfsent so that we avoid removing his mailbox.
 */
void
mechk(np)
	register struct name *np;
{
	for (; np; np = np->n_link)
		if ((np->n_type & GDEL) == 0 && samebody(np->n_name, myname)) {
			selfsent++;
			return;
		}
}

/*
 * Remove all of the duplicates from the passed name list.
 * Return the head of the new list.
 */
struct name *
elide(np)
	register struct name *np;
{
	struct name head, *t, *link = 0;

	for (head.n_link = 0; np; np = link) {
		link = np->n_link;
		np->n_link = 0;
		for (t = &head; t->n_link; t = t->n_link)
			if (strcmp(t->n_link->n_name, np->n_name) == 0)
				break;
		if (!t->n_link)
			t->n_link = np;
	}
	return head.n_link;
}

/*
 * Delete the given name from a namelist.
 */
struct name *
delname(np, username)
	register struct name *np;
	const char *username;
{
	register struct name *onp = np;

	for ( ; np; np = np->n_link)
		if (samebody(username, np->n_name))
			np->n_type |= GDEL;
	return onp;
}

/*
 * Call the given routine on each element of the name
 * list, replacing said value if need be.
 */

void
mapf(np, fromname)
	register struct name *np;
	char *fromname;
{
	if (debug) fprintf(stderr, "mapf %lx, %s\n", (long)np, fromname);
	for (; np; np = np->n_link)
		if ((np->n_type & GDEL) == 0) {
			np->n_name = netmap(np->n_name, fromname);
			np->n_full = splice(np->n_name, np->n_full);
		}
	if (debug) fprintf(stderr, "mapf %s done\n", fromname);
}
