#ident "@(#)vacation.c	11.1"

/*
 *  Vacation program that simulates rcvtrip
 */

#include <sys/types.h>
#include <time.h>
#include <pwd.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define	MAXLINE		500		/* max line from mail header */
#define ALTER_EGOS	".alter_egos"
#define LOGFILE		"logfile"
#define TRIPSUBJECT	"tripsubject"
#define TRIPNOTE	"tripnote"
#define TRIPLOG		"triplog"
#define SIGNATURE	".signature"

#define debug printf

typedef struct entrydef {
	struct entrydef	*e_fwd;
	char		*e_str;
} entry_t;

entry_t *head_egos;			/* alter egos */
entry_t *head_tlog;			/* triplog entries so far */

int opt_dbg;				/* debug option */

char from[MAXLINE];			/* sender's address */

char *subject_text = "Absence (Automatic reply)";

char    *message_text[] = {
"        This is an automatic reply to email you recently sent\n",
"to %s.", "  Additional mail to %s will not result in\n",
"further replies. This mail indicates that the user is not\n",
"responding to your message for the following reason:\n",
"\n",
"        No pre-recorded message file was left by the user.\n",
"However, this feature of the mail system is normally used\n",
"during vacations and other extended absences.\n",
"\n",
"        The Mail System\n",
0,
};

char *mmalloc(int);
void readheaders();
void setreply();
void sendmessage(char *);
void readtlog();
void readegos();
void addlist(entry_t **, char *);
void parse_address(char *, char *);

main(argc, argv)
char **argv;
{
	extern int optind;
	extern char *optarg;
	struct passwd *pw;
	uid_t getuid();

	argc--;
	argv++;
	if (argc) {
		if (strcmp(*argv, "-d") == 0)
			opt_dbg = 1;
	}

	if (!(pw = getpwuid(getuid()))) {
		mylog("vacation: No such user uid %u.\n", getuid());
		exit(1);
	}
	if (chdir(pw->pw_dir)) {
		mylog("vacation: No such directory %s.\n", pw->pw_dir);
		exit(1);
	}

	addlist(&head_egos, pw->pw_name);

	readtlog();
	readegos();
	readheaders();

	if (recent() == 0) {
		setreply();
		sendmessage(pw->pw_name);
	}
	exit(0);
}

/*
 * readheaders --
 *	read mail headers
 */
void
readheaders()
{
	register entry_t *ep;
	int c;
	int tome, cont;
	char buf[MAXLINE];

	cont = tome = 0;
	while (fgets(buf, sizeof(buf), stdin)) {
		if (*buf == '\n')
			continue;
		buf[strlen(buf)-1] = 0;
		c = *buf;
		if (islower(c))
			c = toupper(c);
		switch(c) {
		case 'F':		/* "From " */
			cont = 0;
			if (!strncasecmp(buf, "From:", 5))
				parse_address(from, buf + 5);
			break;
		case 'C':		/* "Cc:" */
			if (strncasecmp(buf, "Cc:", 3))
				break;
			cont = 1;
			goto findme;
		case 'T':		/* "To:" */
			if (strncasecmp(buf, "To:", 3))
				break;
			cont = 1;
			goto findme;
		default:
			if (!isspace(*buf) || !cont || tome) {
				cont = 0;
				break;
			}
findme:			for (ep = head_egos; !tome && ep; ep = ep->e_fwd)
				tome += nsearch(ep->e_str, buf);
		}
	}
	if (opt_dbg) {
		mylog("Header contents\n");
		mylog("from line: %s\n\n", from);
		mylog("Message is %s\n", tome ? "to me" : "not to me");
	}
	if (!tome)
		exit(0);
	if (!*from) {
		mylog("vacation: No \"From:\" line.\n");
		exit(1);
	}
}

/*
 * nsearch --
 *	do a nice, slow, search of a string for a substring.
 */
int
nsearch(name, str)
	register char *name, *str;
{
	register int len;

	for (len = strlen(name); *str; ++str)
		if (*str == *name && !strncasecmp(name, str, len))
			return(1);
	return(0);
}

/*
 * recent --
 *	find out if user has gotten a vacation message recently.
 */
int
recent()
{
	entry_t *ep;
	int ret;

	for (ep = head_tlog; ep; ep = ep->e_fwd) {
		ret = strcasecmp(from, ep->e_str);
		if (ret == 0) {
			if (opt_dbg)
				mylog("user %s already in triplog, no message sent.\n\n", from);
			return(1);
		}
	}
	return(0);
}

/*
 * setreply --
 *	store that this user knows about the vacation.
 *	append to triplog.
 */
void
setreply()
{
	FILE *fd;

	if (opt_dbg)
		mylog("user %s added to triplog\n\n", from);
	/* append to logfile */
	fd = fopen(TRIPLOG, "a");
	if (fd == 0) {
		mylog("vacation: Unable to append to %s\n", TRIPLOG);
		return;
	}
	fprintf(fd, "%s\n", from);
	fclose(fd);
}

/*
 * sendmessage --
 *	exec sendmail to send the vacation file to sender
 */
void
sendmessage(char *myname)
{
	FILE *fdi;
	FILE *fdo;
	int fd;
	char fname[48];
	char **cpp;
	char buf[MAXLINE];
	int len;
	int child;

	if (opt_dbg)
		mylog("Sending message...\n");

	sprintf(fname, "/tmp/vacation.%d", getpid());

	/* build reply message */
	fdo = fopen(fname, "w");

	/* header */
	fprintf(fdo, "To: %s\n", from);
	fprintf(fdo, "From: %s\n", myname);

	fdi = fopen(TRIPSUBJECT, "r");
	buf[0] = 0;
	if (fdi) {
		buf[0] = 0;
		fgets(buf, MAXLINE-1, fdi);
		fclose(fdi);
		if (buf[0])
			buf[strlen(buf) - 1] = 0;
	}
	fprintf(fdo, "Subject: %s\n", buf[0] ? buf : subject_text);
	fprintf(fdo, "\n");

	fdi = fopen(TRIPNOTE, "r");
	if (fdi) {
		/* copy in user's note */
		while (len = fread(buf, 1, MAXLINE-1, fdi))
			fwrite(buf, 1, len, fdo);
	}
	else {
		/* copy in default preamble */
		for (cpp = message_text; *cpp; cpp++)
			fprintf(fdo, *cpp, myname);
	}
	fclose(fdi);

	/* now attempt signature */
	fdi = fopen(SIGNATURE, "r");
	if (fdi) {
		/* copy in user's signature */
		while (len = fread(buf, 1, MAXLINE-1, fdi))
			fwrite(buf, 1, len, fdo);
		fclose(fdi);
	}

	fclose(fdo);

	/* reset stdin */
	close(0);
	fd = open(fname, 0);

	/* msg file goes away after complete */
	unlink(fname);

#ifdef DEBUG
	/* must fork to allow this program to call exit as exit dumps
	   profiling data */
	child = fork();
	if (child == 0) {
#endif
		execl("/usr/lib/sendmail", "sendmail", from, NULL);
		mylog("vacation: Can't exec /usr/lib/sendmail.\n");
		exit(1);
#ifdef DEBUG
	}
	else
		wait(0);
	exit(0);
#endif
}

/*
 * read triplog into core
 */
void
readtlog()
{
	FILE *fd;
	char buf[MAXLINE];
	entry_t *ep;

	fd = fopen(TRIPLOG, "r");
	if (fd) {
		while (fgets(buf, MAXLINE-1, fd)) {
			buf[strlen(buf) - 1] = 0;
			addlist(&head_tlog, buf);
		}
	}
	fclose(fd);
	if (opt_dbg) {
		mylog("Triplog contents:\n");
		for (ep = head_tlog; ep; ep = ep->e_fwd)
			mylog("%s\n", ep->e_str);
		mylog("\n");
	}
}

/*
 * read .alter_egos into core
 */
void
readegos()
{
	FILE *fd;
	char buf[MAXLINE];
	entry_t *ep;

	fd = fopen(ALTER_EGOS, "r");
	if (fd) {
		while (fgets(buf, MAXLINE-1, fd)) {
			buf[strlen(buf) - 1] = 0;
			addlist(&head_egos, buf);
		}
	}
	fclose(fd);
	if (opt_dbg) {
		mylog(".alter_egos contents:\n");
		ep = head_egos;
		if (ep)
			ep = ep->e_fwd;
		for (; ep; ep = ep->e_fwd)
			mylog("%s\n", ep->e_str);
		mylog("\n");
	}
}

/*
 * add (append) an entry to an incore list
 */
void
addlist(entry_t **head, char *str)
{
	register entry_t *ap;
	register entry_t *ep;

	ep = (entry_t *)mmalloc(sizeof(entry_t));
	ep->e_str = mmalloc(strlen(str)+1);
	strcpy(ep->e_str, str);

	ap = 0;
	if (*head)
		for (ap = *head; ap->e_fwd; ap = ap->e_fwd);
	if (ap)
		ap->e_fwd = ep;
	else
		*head = ep;
}

char *
mmalloc(int size)
{
	char *cp;
	char *malloc();

	cp = malloc(size);
	if (cp == 0) {
		mylog("vacation: Malloc error\n");
		exit(1);
	}
	memset(cp, 0, size);
	return(cp);
}

mylog(a, b, c, d)
char *a;
char *b;
char *c;
char *d;
{
	FILE *fd;

	if (access(LOGFILE, W_OK) == 0) {
		fd = fopen(LOGFILE, "a");
		fprintf(fd, a, b, c, d);
		fclose(fd);
	}
}

/*
 * take addresses in one of the following forms:
 *	addr
 *	"name" <addr>		(quotes are optional)
 *	addr (name)
 * and reduce it to just address
 *
 * copied from scoms1.c in c-client rather than link with c-client.
 */
void
parse_address(char *buf, char *addr)
{
	register char *cp;
	register char *cp1;
	int c;

	cp = strchr(addr, '<');
	if (cp) {
		cp1 = strchr(cp, '>');
		if (cp1 == 0)
			return;
		c = *cp1;
		*cp1 = 0;
		strcpy(buf, cp+1);
		*cp1 = c;
	}
	else if (cp = strchr(addr, '(')) {
		c = *cp;
		*cp = 0;
		strcpy(buf, addr);
		*cp = c;
	}
	else
		strcpy(buf, addr);
	/* strip white space */
	cp1 = buf;
	for (cp = buf; *cp; cp++) {
		if (*cp == ' ')
			continue;
		if (*cp == '\t')
			continue;
		*cp1++ = *cp;
	}
	*cp1 = 0;
}
