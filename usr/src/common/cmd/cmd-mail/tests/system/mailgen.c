#ident "@(#)mailgen.c	11.2"

/*
 * program to generate mail messages at a given load.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/utsname.h>

#define debug printf

/* our list management routines (suitable for execlp) */
typedef struct list_def {
	int l_max;	/* size of malloced list */
	int l_cur;	/* current count of malloced list */
	char **l_list;	/* list pointer */
} list_t;

char *opt_body;		/* body file */
int opt_delay;		/* delay time */
int opt_direct;		/* true if direct */
int opt_users;		/* user count */
int opt_robin;		/* round robin user name */
int opt_speed;		/* msg/interval count */
int opt_interval;	/* interval */
int opt_duration;	/* seconds to gen msgs */
int opt_sequence;	/* starting message sequence number */
char *opt_user;		/* destination user */

char user_name[128];	/* user name */
char user_domain[128];	/* domain name of user */
int msg_robin;		/* round robin current count */
int msg_sequence;	/* current message sequence number */

#define SOCKLEN 4096	/* socket buffer size */
char ibuf[SOCKLEN];	/* socket input buffer */
char obuf[SOCKLEN*2];	/* socket output buffer */

/* protos */
int do_one_tick(int);
void do_one_smtp();
void do_one_sendmail();
int out_msg(int, list_t *, int, int, int);
void usage();
void l_useradd(list_t *);
list_t *l_create();
void l_free(list_t *);
void l_add(list_t *, char *);
void *mmalloc(int);
void * mrealloc(void *, int);
int callrecv(unsigned int, char *, int);
void callsend(unsigned int, char *, int);
void do_sequence();

void
main(argc, argv)
char **argv;
{
	int i;
	long start;
	long cur;
	long want;
	long behind;
	int behindflag;
	char *cp;

	argc--;
	argv++;
	while (argc && (**argv == '-')) {
		if (strcmp(*argv, "-body") == 0) {
			argc--;
			argv++;
			if (argc < 1)
				usage();
			opt_body = *argv;
		}
		else if (strcmp(*argv, "-delay") == 0) {
			argc--;
			argv++;
			if (argc < 1)
				usage();
			opt_delay = atoi(*argv);
		}
		else if (strcmp(*argv, "-direct") == 0) {
			opt_direct = 1;
		}
		else if (strcmp(*argv, "-robin") == 0) {
			opt_robin = 1;
		}
		else if (strcmp(*argv, "-users") == 0) {
			argc--;
			argv++;
			if (argc < 1)
				usage();
			opt_users = atoi(*argv);
			if (opt_users <= 0) {
				printf("users <= 0\n");
				usage();
			}
		}
		else if (strcmp(*argv, "-speed") == 0) {
			argc--;
			argv++;
			if (argc < 1)
				usage();
			opt_speed = atoi(*argv);
			if (opt_speed <= 0) {
				printf("speed <= 0\n");
				usage();
			}
		}
		else if (strcmp(*argv, "-interval") == 0) {
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
		else if (strcmp(*argv, "-sequence") == 0) {
			argc--;
			argv++;
			if (argc < 1)
				usage();
			opt_sequence = atoi(*argv);
			msg_sequence = opt_sequence;
			if (opt_sequence < 0) {
				printf("sequence < 0\n");
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
	if (opt_speed == 0) {
		opt_speed = 1;
	}
	if (opt_duration == 0) {
		opt_duration = 1;
	}
	if (opt_interval == 0)
		opt_interval = 1;
	/* parse user */
	cp = strchr(opt_user, '@');
	if (cp) {
		*cp++ = 0;
		strcpy(user_name, opt_user);
		strcpy(user_domain, cp);
		if ((user_name[0] == 0) || (user_domain[0] == 0)) {
			printf("bad address format\n");
			exit(1);
		}
		*--cp = '@';
	}
	else {
		strcpy(user_name, opt_user);
		user_domain[0] = 0;
	}
	/* startup delay */
	if (opt_delay)
		sleep(opt_delay);

	/* our start time */
	start = time(0);
	behindflag = 0;
	for (i = 0; i < opt_duration; i += opt_interval) {
		/* look ahead the entire group for exit if round robin */
		if (opt_robin && (msg_robin == 0))
			if ((i + (opt_interval * opt_users)) > opt_duration)
				break;
		want = start + i;
		cur = time(0);
		/* nap if we need to */
		if (cur < want) {
			/* nap for 1/10 second intervals */
			while (cur < want) {
				nap(100l);
				cur = time(0);
			}
		}

		do_one_tick(i);

		/* check if last tick took too long */
		cur = time(0);
		/* positive behind means we really are behind */
		behind = cur - want;
		if (behindflag == 0) {
			if (behind <= opt_interval)
				continue;
			printf("Getting behind at %d seconds into it\n", i);
			behindflag = 1;
		}
		else {
			/* check if we got caught up */
			if (behind > 1)
				continue;
			printf("Got caught up at %d seconds into it\n", i);
			behindflag = 0;
		}
	}
	/* notify caller if we were behind when we stopped */
	if (behindflag)
		exit(2);
	exit(0);
}

int
do_one_tick(int second)
{
	int i;

	for (i = 0; i < opt_speed; i++) {
		/* call either sendmail or SMTP */
		if (opt_direct)
			do_one_smtp();
		else
			do_one_sendmail();
		do_sequence();
	}
}

/*
 * call sendmail with our arguments
 */
void
do_one_sendmail()
{
	int i;
	register list_t *lp;
	int p[2];

	lp = l_create();

	/* build argv list */
	l_add(lp, "sendmail");
	l_useradd(lp);

	/* list ends in 0 */
	l_add(lp, 0);

	/* now exec sendmail and pipe message to it */
	if (pipe(p) < 0) {
		printf("Unable to create pipe\n");
		exit(1);
	}
	printf("Sending msg %d ", msg_sequence);
	for (i = 1; i < lp->l_cur; i++)
		printf("%s ", lp->l_list[i]);
	printf("\n"); fflush(stdout);
	if (fork() == 0) {
		/* child, set stdin to the pipe */
		close(p[1]);
		close(0);
		dup(p[0]);
		close(p[0]);
		execv("/usr/lib/sendmail", lp->l_list);
		printf("EXEC ERROR on sendmail\n"); fflush(stdout);
		exit(0);
	}
	close(p[0]);
	out_msg(p[1], lp, msg_sequence, 0, 1);
	close(p[1]);
	wait(0);

	l_free(lp);
}

/*
 * connect via a socket to the server and send via SMTP
 */
void
do_one_smtp()
{
	int i;
	unsigned int s;
	struct hostent *hent;
	struct sockaddr_in sin;
	int ret;
	register list_t *lp;
	struct utsname uts;

	s = socket(PF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		printf("Unable to create a socket\n");
		exit(1);
	}
	if (user_domain[0] == 0) {
		printf("direct delivery needs host name\n");
		exit(1);
	}
	hent = gethostbyname(user_domain);
	if (hent == NULL) {
		printf("Unable to resolv %s\n", user_domain);
		exit(1);
	}
        sin.sin_family = AF_INET;
        sin.sin_port = htons(25);		/* smtp port */
        memset(&sin.sin_addr, 0, sizeof(sin.sin_addr));
        memcpy(&sin.sin_addr, hent->h_addr, hent->h_length);
        memset(sin.sin_zero, 0, sizeof(sin.sin_zero));
        ret = connect(s, (struct sockaddr *)&sin, sizeof(sin));
        if (ret == -1) {
        	printf("Unable to connect to %s\n", user_domain);
        	exit(1);
        }

        /* get initial message */
        callrecv(s, ibuf, SOCKLEN);

        uname(&uts);

        /* send HELO */
	sprintf(obuf, "HELO %s\r\n", uts.nodename);
	callsend(s, obuf, strlen(obuf));

	/* get response */
        callrecv(s, ibuf, SOCKLEN);

	/* send FROM */
	sprintf(obuf, "MAIL FROM:<mailgen>\r\n");
	callsend(s, obuf, strlen(obuf));

	/* get response */
        callrecv(s, ibuf, SOCKLEN);

	/* send recipients */
	lp = l_create();
	l_useradd(lp);

	for (i = 0; i < lp->l_cur; i++) {
		sprintf(obuf, "RCPT TO:<%s>\r\n", lp->l_list[i]);
		callsend(s, obuf, strlen(obuf));

		/* get response */
	        callrecv(s, ibuf, SOCKLEN);
	}

	/* send DATA command */
	sprintf(obuf, "DATA\r\n");
	callsend(s, obuf, strlen(obuf));

	/* get response */
        callrecv(s, ibuf, SOCKLEN);

	/* send message body */
	out_msg(s, lp, msg_sequence, 1, 0);
	l_free(lp);

	/* terminate message with '.' */
	sprintf(obuf, ".\r\n");
	callsend(s, obuf, strlen(obuf));

	/* tell server we are done */
	sprintf(obuf, "QUIT\r\n");
	callsend(s, obuf, strlen(obuf));

	/* get response */
        callrecv(s, ibuf, SOCKLEN);

	close(s);
}

/*
 * output the message to an fd or to a socket.
 */
int
out_msg(int ofd, list_t *lp, int sequence, int socket, int start)
{
	FILE *fd;
	char buf[BUFSIZ];
	long l;
	int len;
	int i;

	l = time(0);
	sprintf(buf, "From: root\n");
	if (socket == 0)
		write(ofd, buf, strlen(buf));
	else
		callsend(ofd, buf, strlen(buf));
	sprintf(buf, "To: ");
	for (i = start; i < lp->l_cur; i++) {
		if (i)
			strcat(buf, ", ");
		strcat(buf, lp->l_list[i]);
	}
	strcat(buf, "\n");
	if (socket == 0)
		write(ofd, buf, strlen(buf));
	else
		callsend(ofd, buf, strlen(buf));
	sprintf(buf, "Subject: Testing %d\n", sequence);
	if (socket == 0)
		write(ofd, buf, strlen(buf));
	else
		callsend(ofd, buf, strlen(buf));
	sprintf(buf, "Date: %s", ctime(&l));
	if (socket == 0)
		write(ofd, buf, strlen(buf));
	else
		callsend(ofd, buf, strlen(buf));
	sprintf(buf, "\n");
	if (socket == 0)
		write(ofd, buf, strlen(buf));
	else
		callsend(ofd, buf, strlen(buf));
	sprintf(buf, "Body of message %d\n", sequence);
	if (socket == 0)
		write(ofd, buf, strlen(buf));
	else
		callsend(ofd, buf, strlen(buf));
	if (opt_body) {
		fd = fopen(opt_body, "r");
		if (fd == 0) {
			printf("Unable to open %s\n", opt_body);
			exit(0);
		}
		/* rest of body */
		while (len = fread(buf, 1, BUFSIZ, fd)) {
			if (socket == 0)
				write(ofd, buf, strlen(buf));
			else
				callsend(ofd, buf, strlen(buf));
		}
		fclose(fd);
	}
}

void
usage()
{
printf("Usage: mailgen options...\n");
printf("       -body file - specify the message body in a file\n");
printf("       -delay n - specify start of test delay (default 0)\n");
printf("       -direct - direct to SMTP rather than the default of sendmail\n");
printf("       -users n - user name is base of n users: user0, user1...\n");
printf("       -robin - treat users as a round robin list, N per interval\n");
printf("       -speed n - # of messages per interval to attempt (default 1)\n");
printf("       -interval - # of seconds per interval (default is 1)\n");
printf("       -duration n - # of seconds to generate msgs (default 1)\n");
printf("       user[@host] - our destination user\n");
exit(1);
}

/*
 * add all users to the list specified
 */
void
l_useradd(list_t *lp)
{
	int i;
	char buf[80];

	if (opt_users) {
		if (opt_robin == 0) {
			for (i = 0; i < opt_users; i++) {
				if (user_domain[0])
					sprintf(buf, "%s%d@%s", user_name, i,
						user_domain);
				else
					sprintf(buf, "%s%d", user_name, i);
				l_add(lp, buf);
			}
		} else {
			/* round robin processing */
			i = msg_robin++;
			if (msg_robin >= opt_users)
				msg_robin = 0;
			if (user_domain[0])
				sprintf(buf, "%s%d@%s", user_name, i,
					user_domain);
			else
				sprintf(buf, "%s%d", user_name, i);
			l_add(lp, buf);
		}
	}
	else {
		l_add(lp, opt_user);
	}
}

/*
 * list management routines:
 * l_create - create a list structure
 * l_free - deinitialize a list structure (free all elements)
 * l_add - add an entry to a list structure
 */
list_t *
l_create()
{
	register list_t *lp;

	lp = (list_t *)mmalloc(sizeof(list_t));
	memset(lp, 0, sizeof(list_t));
	return(lp);
}

void
l_free(register list_t *lp)
{
	int i;

	if (lp->l_list) {
		for (i = 0; i < lp->l_cur; i++)
			if (lp->l_list[i])
				free(lp->l_list[i]);
		free(lp->l_list);
	}
	free(lp);
}

void
l_add(register list_t *lp, char *cp)
{
	/* expand list if needed */
	if (lp->l_cur == lp->l_max) {
		if (lp->l_max) {
			lp->l_max *= 2;
			lp->l_list = (char **)mrealloc(lp->l_list,
				lp->l_max*sizeof(char *));
		}
		else {
			lp->l_max = 10;
			lp->l_list = (char **)mmalloc(lp->l_max*sizeof(char *));
		}
	}
	lp->l_list[lp->l_cur++] = strdup(cp);
}

void *
mmalloc(int size)
{
	void *vp;

	vp = malloc(size);
	if (vp == 0) {
		printf("Malloc out of memory\n");
		exit(1);
	}
	return(vp);
}

void *
mrealloc(void *vp, int size)
{
	void *vp1;

	vp1 = realloc(vp, size);
	if (vp1 == 0) {
		printf("Malloc out of memory\n");
		exit(1);
	}
	return(vp1);
}

/*
 * socket recv call routine
 */
int
callrecv(unsigned int s, char *buf, int len)
{
	int ret;

	ret = recv(s, buf, len, 0);
	switch (ret) {
	case 0: /* socket closed */
		printf("Unexpected socket close on recv\n");
		exit(1);
	case -1:
		printf("Unexpected socket error on recv\n");
		exit(1);
	}
	return(ret);
}

/*
 * socket send call routine
 */
void
callsend(unsigned int s, char *buf, int len)
{
	int ret;

	ret = send(s, buf, len, 0);
	switch (ret) {
	case 0: /* socket closed */
		printf("Unexpected socket close on send\n");
		exit(1);
	case -1:
		printf("Unexpected socket error on send\n");
		exit(1);
	}
}

/*
 * bump sequence number if needed
 */
void
do_sequence()
{
	static int once;

	if (opt_robin == 0) {
		msg_sequence++;
	}
	else {
		if ((msg_robin == 0) && once)
			msg_sequence++;
	}
	once = 1;
}
