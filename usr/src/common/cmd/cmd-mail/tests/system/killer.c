#ident "@(#)killer.c	11.1"
/*
 * simple program to invalidate a mailbox's index at random intervals
 * to provoke a rebuild on the next c-client open.
 * program assumes that it starts as root, and su's to the destination user
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <sys/param.h>
/* the c-client includes */
#include <c-client/mail.h>
#include <c-client/osdep.h>
#include <c-client/rfc822.h>
#include <c-client/misc.h>
#include <c-client/scomsc1.h>

#define debug printf

int opt_interval;	/* interval */
int opt_duration;	/* duration */
int opt_delay;		/* delay before start */
char *opt_user;		/* username of mailbox */

char user_name[128];	/* user name */

void usage();

void
main(argc, argv)
char **argv;
{
	int i;
	long start;
	long cur;
	float f;
	void *mbox;
	char fname[80];
	struct passwd *pp;

	argc--;
	argv++;
	while (argc && (**argv == '-')) {
		if (strcmp(*argv, "-interval") == 0) {
			argc--;
			argv++;
			if (argc < 1)
				usage();
			opt_interval = atoi(*argv);
			if (opt_interval <= 0) {
				printf("interval <= 0\n");
				usage();
			}
		}
		else if (strcmp(*argv, "-duration") == 0) {
			argc--;
			argv++;
			if (argc < 1)
				usage();
			opt_duration = atoi(*argv);
			if (opt_duration <= 0) {
				printf("duration <= 0\n");
				usage();
			}
		}
		else if (strcmp(*argv, "-delay") == 0) {
			argc--;
			argv++;
			if (argc < 1)
				usage();
			opt_delay = atoi(*argv);
			if (opt_delay <= 0) {
				printf("delay <= 0\n");
				usage();
			}
		}
		else {
			printf("Unknown argument: %s\n", *argv);
			usage();
		}
		argc--;
		argv++;
	}
	if (argc != 1) {
		printf("Missing user name\n");
		usage();
	}
	opt_user = *argv;
	if (opt_interval == 0)
		opt_interval = 1;
	if (opt_duration == 0)
		opt_duration = 1;

	/* set user */
	setpwent();
	pp = getpwnam(opt_user);
	endpwent();
	if (pp == 0) {
		printf("Unknown user id %s\n", opt_user);
		exit(0);
	}
	setgid(pp->pw_gid);
	setuid(pp->pw_uid);

	if (opt_delay)
		sleep(opt_delay);
	opt_duration -= opt_delay;
	srand(getpid());
	for (cur = 0; cur < opt_duration; ) {
		/* sleep for a random time */
		f = rand();
		f = ((f/RAND_MAX) * opt_interval) + 1;
		i = f;
		if ((i + cur) > opt_duration)
			i = opt_duration - cur;
		sleep(i);
		cur += i;

		mbox = scomsc1_open("INBOX", ACCESS_REBUILD, 0);
		if ((mbox == 0) || (mbox == (void *)1) || (mbox == (void *)2)) {
			/* unable to lock mailbox, try again later */
			printf("WARN: Unable to lock mailbox\n");
			continue;
		}
		sprintf(fname, "%s/.%s.index", MAILSPOOL, opt_user);
		unlink(fname);
		scomsc1_close(mbox);
		printf("Mailbox index unlinked.\n");
	}
}

void
usage()
{
	printf("Usage: killer [-interval n] [-delay n] [-duration n] user\n");
	printf("    -interval is random interval size.\n");
	printf("    -duration is total duration of program.\n");
	printf("    -delay is start delay.\n");
	exit(1);
}
