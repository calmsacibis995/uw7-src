#ident "@(#)multihome.c	11.1"

/*
 * aliasing mailer for sendmail that knows about virtual domains
 * and their alias files.
 */

/*
 * our processing steps are as follows:
 *
 * - sort recipients by domain, recipients without domains are errors.
 * - alias each recipient in the appropriate domain alias file.
 *   errors occur for each recipient if an alias loop is detected
 *     or a domain is found that is not in our virtual domain table.
 *   Any errors cause this message to be returned as permanently undeliverable.
 * - sort again recipients by domain.
 * - alias all recipients through the virtusers database.
 * - resubmit message to sendmail,
 *   we just pass our stdin to sendmail as we don't ever touch it.
 *   this reduces copying a bit as it goes directly into the sendmail queue.
 *
 * If the entire message bouncing is a problem, then remove the multiple
 * flag from the mailer, and set checkpointing on for every mailer attempt.
 * mail will be slower overall but individual failed addresses will be flagged
 * instead of the whole message being bounced.
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <ctype.h>

#include <sysexits.h>
#include <multihome.h>
#include <db.h>

#define MAXNAME	256
#define MAXIP	64
/* relative position of mail.aliases file */
#define MAIL_ALIASES	"mail/mail.aliases.db"

#define ALIAS_LOOP	20	/* maximum number of alias passes */

/* the same structure is used both in arrays and lists
   when it is in an array the fwd member is not used */
typedef struct rcpdef {
	struct rcpdef *r_fwd;
	char *r_user;
	char *r_domain;
} rcp_t;

char errmsg[MAXNAME];	/* for error logging */
char errbuf[MAXNAME];	/* also used for error logging */

rcp_t *rcp;		/* array of recipients used for sorting */
int rcps;		/* count of recipients in array */

rcp_t *rcphead;		/* list or recipients, built during aliasing */
int opt_test;
int opt_debug;

/* our lists used in aliasing */
rcp_t *nhead;		/* the not aliased list */
rcp_t *ahead;		/* the aliased list */

int ccmp(void *, void *);
char *mmalloc(int);
int argv_parse(int, char **);
int rcp_parse(rcp_t *, char *);
int do_aliases();
rcp_t *alias_one(rcp_t *, int *);
char *alias_lookup(rcp_t *);
void listfree(rcp_t **);
void list_to_array();
void array_to_list();
void do_output();
void debug(char *);
void debugarray();
void debuglist(rcp_t *);
void do_uniq();
void errlog(char *);
void strtolower(char *);

void
main(argc, argv)
int argc;
char **argv;
{
	argc--;
	argv++;
	/* address test mode */
	while (argc && (**argv == '-')) {
		if (strcmp(*argv, "-t") == 0) {
			opt_test = 1;
			argc--;
			argv++;
			continue;
		}
		if (strcmp(*argv, "-d") == 0) {
			opt_debug = 1;
			argc--;
			argv++;
			continue;
		}
	}
	/* argc and argv are now our recipient list */
	if (argc == 0)
		usage();
	/* parse each recipient into user@domain parts */
	if (argv_parse(argc, argv) == 0)
		exit(EX_NOUSER);
	/* sort list in case insensitive manner by domain */
	qsort(rcp, rcps, sizeof(rcp_t), ccmp);
	/* parsed and sorted array */
	if (opt_debug) {
		debug("\nParsed list sorted by domain first\n");
		debugarray();
	}
	/* alias each user through the appropriate virtual domain alias table */
	array_to_list();
	if (do_aliases() == 0)
		exit(EX_NOUSER);
	if (opt_debug) {
		debug("\nAliased list before sort\n");
		debuglist(rcphead);
	}
	/* move the list to the array for sorting, the list is freed */
	list_to_array();
	/* sort again by domain */
	qsort(rcp, rcps, sizeof(rcp_t), ccmp);
	if (opt_debug) {
		debug("\nAliased list sorted by domain first\n");
		debugarray();
	}
	/* alias through virtusers file */
	if (do_user_map() == 0)
		exit(EX_NOUSER);
	if (opt_debug) {
		debug("\nMapped list before non-uniq items removed\n");
		debugarray();
	}
	do_uniq();
	do_output();
	listfree(&rcphead);
	exit(0);
}

int
ccmp(void *p1, void *p2)
{
	int ret;
	rcp_t *rp1;
	rcp_t *rp2;

	rp1 = (rcp_t *)p1;
	rp2 = (rcp_t *)p2;

	ret = strccmp(rp1->r_domain, rp2->r_domain);
	if (ret)
		return(ret);
	return(strccmp(rp1->r_user, rp2->r_user));
}

usage()
{
fprintf(stderr, "usage: multihome [-t/-d] recipient...\n");
fprintf(stderr, "    multihome routes users in virtual domains\n");
fprintf(stderr, "    to their correct physical users.\n");
fprintf(stderr, "    stdin is set to the mail message to be routed.\n");
fprintf(stderr, "    this program also supports aliasing for each virtual domain\n");
fprintf(stderr, "    -t - address test mode:\n");
fprintf(stderr, "        outputs new recipient list after aliasing and user mapping\n");
fprintf(stderr, "    -d - debug mode:\n");
fprintf(stderr, "        like -t, but prints addresses at each stage of lookup\n");
fprintf(stderr, "    -t and -d do not process a message from stdin\n");
	exit(EX_NOUSER);
}

/*
 * parse recipients and convert to lower case
 */
int
argv_parse(int argc, char **argv)
{
	int i;
	register rcp_t *rp;
	char *arg;
	char *cp;

	rcps = argc;
	rcp = (rcp_t *)mmalloc(sizeof(rcp_t)*rcps);
	for (i = 0; i < argc; i++) {
		rp = rcp + i;
		arg = argv[i];

		if (rcp_parse(rp, arg) == 0)
			return(0);
		/* we don't handle addresses with no domain */
		if (rp->r_domain[0] == 0) {
			sprintf(errmsg, "Bad address for multihome: %s", arg);
			errlog(errmsg);
			return(0);
		}
		/* each domain must be in one of our virtual domains */
		cp = mhome_virtual_domain_ip(rp->r_domain);
		if (cp == 0) {
			sprintf(errmsg, "Domain not valid for multihome: %s", arg);
			errlog(errmsg);
			return(0);
		}
	}
	return(1);
}

int
rcp_parse(rcp_t *rp, char *arg)
{
	char *cp;
	int len;

	if (strlen(arg) > (MAXNAME/2)) {
		arg[MAXNAME/2] = 0;
		sprintf(errmsg, "Address too long: %s...", arg);
		errlog(errmsg);
		return(0);
	}
	cp = (char *)strchr(arg, '@');
	if (cp == 0) {
		rp->r_user = mmalloc(strlen(arg) + 1);
		strcpy(rp->r_user, arg);
		rp->r_domain = mmalloc(1);
		*rp->r_domain = 0;
		strtolower(rp->r_domain);
	}
	else {
		*cp++ = 0;
		rp->r_user = mmalloc(strlen(arg) + 1);
		strcpy(rp->r_user, arg);
		rp->r_domain = mmalloc(strlen(cp) + 1);
		strcpy(rp->r_domain, cp);
		*--cp = '@';
		strtolower(rp->r_user);
		strtolower(rp->r_domain);
	}
	return(1);
}

int
strccmp(char *s1, char *s2)
{
	register unsigned char *u1;
	register unsigned char *u2;
	int c1;
	int c2;

	u1 = (unsigned char *)s1;
	u2 = (unsigned char *)s2;
	while (*u1) {
		if (*u2 == 0)
			return(1);
		c1 = isupper(*u1) ? *u1 : _toupper(*u1);
		c2 = isupper(*u2) ? *u2 : _toupper(*u2);
		if (c1 != c2)
			return (c1 < c2) ? -1 : 1;
		u1++;
		u2++;
	}
	if (*u2)
		return(-1);
	return(0);
}


char *
mmalloc(int size)
{
	register char *cp;

	cp = (char *)malloc(size);
	if (cp == 0) {
		sprintf(errmsg, "malloc error");
		errlog(errmsg);
		exit(EX_TEMPFAIL);
	}
	memset(cp, 0, size);
	return(cp);
}

/*
 * pass each user through the appropriate domain alias file.
 * Output is built in rcphead, unaliased users are also copied as the
 * entire list will be copied back into the rcp array later for another sort.
 *
 * the algorithm is as follows:
 * multiple passes are made, each pass copies from the list into
 * two other lists, the unaliased list and the aliased list.
 * those that were aliased are copied back to the original list
 * for the next pass.  When the original list is empty then we are finished
 * with aliasing.  Alias loops are deteced in that a maximum number
 * of passes is enforced.
 */
int
do_aliases()
{
	int i;
	char *cp;
	rcp_t *rp;

	for (i = 0; i < 20; i++) {
		if (do_alias_pass() == 0)
			return(0);
		if (opt_debug && rcphead) {
			sprintf(errmsg, "\nAfter Alias Pass %d\n", i+1);
			debug(errmsg);
			debug("Finished list (no alias was found for these)\n");
			debuglist(nhead);
			debug("Remaining list for next alias pass\n");
			debuglist(rcphead);
		}
		if (rcphead == 0)
			break;
	}
	if (rcphead) {
		sprintf(errmsg, "Alias loop detected in the following recipients:\n");
		cp = errmsg + strlen(errmsg);
		for (rp = rcphead; rp; rp = rp->r_fwd) {
			sprintf(cp, "%s@%s\n", rp->r_user, rp->r_domain);
			cp += strlen(cp);
			if ((cp - errmsg) > (MAXNAME/2))
				break;
		}
		errlog(errmsg);
		return(0);
	}
	rcphead = nhead;
	nhead = 0;
	return(1);
}

/*
 * do a single pass through alias list
 */
int
do_alias_pass()
{
	int aliased;
	rcp_t *rp;	/* current unaliased recipient */
	rcp_t *rpn;	/* output of single alias program routine */
	rcp_t *npa;	/* not aliased append pointer */
	rcp_t *apa;	/* aliased append pointer */

	apa = 0;
	npa = nhead;
	if (npa) {
		while (npa->r_fwd)
			npa = npa->r_fwd;
	}

	for (rp = rcphead; rp; rp = rp->r_fwd) {
		rpn = alias_one(rp, &aliased);
		if (rpn == 0)
			return(0);

		if (aliased) {
			if (apa)
				apa->r_fwd = rpn;
			else
				ahead = rpn;
			/* skip to end */
			apa = rpn;
			while (apa->r_fwd)
				apa = apa->r_fwd;
		}
		else {
			if (npa)
				npa->r_fwd = rpn;
			else
				nhead = rpn;
			npa = rpn;
		}
	}
	/* free rcphead, move ahead back to it */
	listfree(&rcphead);
	rcphead = ahead;
	ahead = 0;
	return(1);
}

/*
 * copies item unchanged or outputs list of aliases as needed
 */
rcp_t *
alias_one(rcp_t *one, int *aliased)
{
	char *found;	/* output of database lookup */
	rcp_t *rpn;	/* new recipient */
	rcp_t *rpa;	/* append pointer for rphead */
	rcp_t *rphead;	/* our new list for alias output */
	char *tok;	/* token string pointer */

	*aliased = 0;
	found = alias_lookup(one);
	if (found == (char *)-1)
		return(0);
	if (found == 0) {
		rpn = (rcp_t *)mmalloc(sizeof(rcp_t));
		rpn->r_user = mmalloc(strlen(one->r_user) + 1);
		strcpy(rpn->r_user, one->r_user);
		rpn->r_domain = mmalloc(strlen(one->r_domain) + 1);
		strcpy(rpn->r_domain, one->r_domain);
		free(found);
		return(rpn);
	}
	*aliased = 1;
	rpa = 0;
	rphead = 0;
	tok = strtok(found, ", ");
	while (tok) {
		rpn = (rcp_t *)mmalloc(sizeof(rcp_t));
		if (rcp_parse(rpn, tok) == 0)
			return(0);
		/* keep local aliases in their respective domains */
		if (*rpn->r_domain == 0) {
			free(rpn->r_domain);
			rpn->r_domain = mmalloc(strlen(one->r_domain) + 1);
			strcpy(rpn->r_domain, one->r_domain);
		}
		if (rpa == 0) {
			rphead = rpn;
			rpa = rpn;
		}
		else {
			rpa->r_fwd = rpn;
			rpa = rpn;
		}
		tok = strtok(0, ", ");
	}
	free(found);
	if (rphead == 0) {
		sprintf(errmsg, "Invalid null alias for %s@%s",
			one->r_user, one->r_domain);
		errlog(errmsg);
	}
	return(rphead);
}

/*
 * lookup an alias in it's appropriate alias file, returns 0 nofind, -1 error
 * found data is malloced, caller must free
 */
char *
alias_lookup(rcp_t *rp)
{
	static DB *mhdb;
	static char cip[MAXIP+1];
	static char domain[MAXNAME+1];

	int ch;
	DBT key;
	DBT data;
	char buf[MAXNAME+1];
	char *cp;
	int ret;
	char *found;

	/* lookup domain to get ip addr and alias file name */
	if (strcmp(domain, rp->r_domain)) {
		strcpy(domain, rp->r_domain);
		cp = mhome_virtual_domain_ip(rp->r_domain);
		/* nofind */
		if (cp == 0)
			return(0);
		/* fatal error */
		if (strlen(cp) >= MAXIP) {
			cp[MAXIP] = 0;
			sprintf(errmsg, "Ip address to long for %s@%s: %s...",
				rp->r_user, rp->r_domain, cp);
			errlog(errmsg);
			return((char *)-1);
		}
		/* switching to another ip addr */
		if (strcmp(cp, cip)) {
			strcpy(cip, cp);
			/* MHOMEPATH/ip/mail/mail.aliases.db */
			strcpy(buf, MHOMEPATH);
			cp = buf + strlen(buf);
			strcpy(cp, "/");
			cp = cp + strlen(cp);
			strcpy(cp, cip);
			cp = cp + strlen(cp);
			strcpy(cp++, "/");
			strcpy(cp, MAIL_ALIASES);
			mhdb = dbopen(buf, O_RDONLY, 0444, DB_HASH, 0);
		}
	}

	if (mhdb) {
		key.data = rp->r_user;
		key.size = strlen(rp->r_user) + 1;
		data.data = 0;
		data.size = 0;
		ret = mhdb->get(mhdb, &key, &data, 0);
		if (ret != RET_SUCCESS)
			return(0);
		found = mmalloc(data.size + 1);
		memcpy(found, data.data, data.size);
		found[data.size] = 0;
		if (data.size > (MAXNAME/2)) {
			found[MAXNAME/2] = 0;
			sprintf(errmsg, "User aliases to long string: %s@%s: %s...",
				rp->r_user, rp->r_domain, found);
			errlog(errmsg);
			return((char *)-1);
		}
		return(found);
	}
	return(0);
}

/* free our list of recipients */
void
listfree(rcp_t **head)
{
	register rcp_t *rp;
	register rcp_t *rp1;

	for (rp = *head; rp; rp = rp1) {
		rp1 = rp->r_fwd;
		free(rp->r_user);
		free(rp->r_domain);
		free(rp);
	}
	*head = 0;
}

/*
 * move the list to the array for sorting
 */
void
list_to_array()
{
	int index;
	register rcp_t *rp;
	register rcp_t *rp1;

	/* first find the size */
	index = 0;
	for (rp = rcphead; rp; rp = rp->r_fwd)
		index++;
	rcps = index;
	rcp = (rcp_t *)mmalloc(sizeof(rcp_t)*rcps);

	index = 0;
	for (rp = rcphead; rp; rp = rp1) {
		rp1 = rp->r_fwd;
		rp->r_fwd = 0;
		memcpy(rcp + index, rp, sizeof(rcp_t));
		free(rp);
		index++;
	}
}

/*
 * move our array to the list for aliasing
 */
void
array_to_list()
{
	int i;
	rcp_t *rp;
	rcp_t *rpn;
	rcp_t *rpa;

	listfree(&rcphead);

	rpa = 0;
	for (i = 0; i < rcps; i++) {
		rp = rcp + i;

		rpn = (rcp_t *)mmalloc(sizeof(rcp_t));
		rpn->r_user = rp->r_user;
		rpn->r_domain = rp->r_domain;
		if (rpa == 0)
			rcphead = rpn;
		else
			rpa->r_fwd = rpn;
		rpa = rpn;
	}
	free(rcp);
	rcps = 0;
	rcp = 0;
}

/* 
 * run each item in our array through the user map
 * update the user map in place as needed
 */
int
do_user_map()
{
	int i;
	int len;
	register rcp_t *rp;
	char *cp;

	for (i = 0; i < rcps; i++) {
		rp = rcp + i;

		cp = mhome_user_map(rp->r_user, rp->r_domain);
		/* found a map */
		if (cp) {
			/* make it local */
			len = strlen(cp);
			if (len >= (MAXNAME/2)) {
				cp[MAXNAME/2] = 0;
				sprintf(errmsg, "User %s@%s mapped to invalid string:\nGot: %s...", rp->r_user, rp->r_domain, cp);
				errlog(errmsg);
				return(0);
			}
			*rp->r_domain = 0;
			free(rp->r_user);
			rp->r_user = mmalloc(len + 1);
			strcpy(rp->r_user, cp);
		}
	}
	return(1);
}

/*
 * remove non-uniq items from array
 * memcpy up array and reduce count as we go
 */
void
do_uniq()
{
	int i;
	int j;
	rcp_t *rp1;
	rcp_t *rp2;
	int found;

	for (i = 0; i < rcps; i++) {
		rp1 = rcp + i;
		found = 0;
		for (j = i + 1; j < rcps; j++) {
			rp2 = rcp + j;
			if (strcmp(rp1->r_user, rp2->r_user))
				continue;
			if (strcmp(rp1->r_domain, rp2->r_domain))
				continue;
			found = 1;
			break;
		}
		if (found == 0)
			continue;
		/* found a match, remove it */
		memcpy(rcp + i, rcp + i + 1, rcps - i - 1);
		rcps--;
		i--;
	}
}

/*
 * have list of aliases in array
 * pass them on to sendmail
 */
void
do_output()
{
	int i;
	int len;
	int pid;
	int status;
	register rcp_t *rp;
	char **argv;
	char *cp;

	if (opt_test || opt_debug) {
		if (opt_debug) {
			debug("\nFinal recipient list, not sorted\n");
		}
		for (i = 0; i < rcps; i++) {
			rp = rcp + i;
			debug(rp->r_user);
			if (*rp->r_domain) {
				debug("@");
				debug(rp->r_domain);
			}
			debug("\n");
		}
		return;
	}

	sprintf(errmsg, "rcps %d\n", rcps);
	debug(errmsg);
	if (rcps == 0) {
		sprintf(errmsg, "Null recipient list after processing");
		errlog(errmsg);
		exit(EX_NOUSER);
	}

	/* build argument list for sendmail */
	argv = (char **)mmalloc(sizeof(char *)*(rcps + 2));
	argv[0] = mmalloc(strlen("sendmail") + 1);
	strcpy(argv[0], "sendmail");
	for (i = 0; i < rcps; i++) {
		rp = rcp + i;
		len = strlen(rp->r_user);
		if (*rp->r_domain)
			len += strlen(rp->r_domain) + 1;
		len++;
		cp = mmalloc(len);
		argv[i+1] = cp;
		strcpy(cp, rp->r_user);
		if (*rp->r_domain) {
			cp += strlen(cp);
			strcpy(cp, "@");
			cp += strlen(cp);
			strcpy(cp, rp->r_domain);
		}
	}
	argv[rcps + 2] = 0;

	switch (pid = fork()) {
	/* child */
	case 0:
		execvp("/usr/lib/sendmail", argv);
		exit(1);
	/* fork fail */
	case -1:
		exit(EX_TEMPFAIL);
	/* parent */
	default:
		/* check status */
		status = 0;
		wait(&status);
		if (status)
			exit(EX_TEMPFAIL);
		exit(EX_OK);
	}
}

void
debug(char *str)
{
	write(1, str, strlen(str));
}

void
debugarray()
{
	int i;
	rcp_t *rp;

	for (i = 0; i < rcps; i++) {
		rp = rcp + i;
		debug(rp->r_user);
		if (*rp->r_domain) {
			debug("@");
			debug(rp->r_domain);
		}
		debug("\n");
	}
}

void
debuglist(rcp_t *head)
{
	int i;
	rcp_t *rp;

	for (rp = head; rp; rp = rp->r_fwd) {
		debug(rp->r_user);
		if (*rp->r_domain) {
			debug("@");
			debug(rp->r_domain);
		}
		debug("\n");
	}
}

/*
 * errors are sent to syslog and to stderr if opt_debug or opt_test are set
 */
void
errlog(char *msg)
{
	static init = 0;

	if (init == 0) {
		openlog("multihome", 0, LOG_MAIL);
		init = 1;
	}
	if (opt_debug || opt_test) {
		write(2, msg, strlen(msg));
		write(2, "\n", 1);
	}
	syslog(LOG_ERR, "%s", msg);
}

/*
 * convert to lower case
 */
void
strtolower(register char *cp)
{
	while (*cp) {
		if (isupper(*cp))
			*cp = tolower(*cp);
		cp++;
	}
}
