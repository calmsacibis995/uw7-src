#ident "@(#)mailread.c	11.2"
/*
 * reads mail until SIGINT or complete.
 * a report is generated on termination.
 */

#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#define debug printf
#define protocol (opt_protocol == 0) ? 0 : printf

int opt_pop;
int opt_imap;
int opt_imapconnect;
int opt_local;
int opt_random;
int opt_interval;
int opt_count;
int opt_protocol;
int opt_delete;
char *opt_user;
char *opt_passwd;

char user_name[80];
char user_domain[80];

char *msg_array;
int glob_err;		/* error count */
int glob_warn;		/* warning count */
int glob_expunge;	/* imap expunge count */

char buf[BUFSIZ];	/* input buffer */
char tbuf[BUFSIZ];	/* another buffer */

void usage();
void done();
void do_imap_poll();
void do_pop_poll();
void read_mbox();
char *mmalloc(int);
int iconnect(char *, int);
void check_done();
int check_exists(char *, int);

void
main(argc, argv)
char **argv;
{
	float f;
	int i;
	char *cp;
	int first;

	argc--;
	argv++;

	while (argc && (**argv == '-')) {
		if (strcmp(*argv, "-pop") == 0)
			opt_pop = 1;
		if (strcmp(*argv, "-imap") == 0)
			opt_imap = 1;
		if (strcmp(*argv, "-local") == 0)
			opt_local = 1;
		if (strcmp(*argv, "-random") == 0)
			opt_random = 1;
		if (strcmp(*argv, "-protocol") == 0)
			opt_protocol = 1;
		if (strcmp(*argv, "-delete") == 0)
			opt_delete = 1;
		if (strcmp(*argv, "-imapconnect") == 0) {
			argc--;
			argv++;
			if (argc < 1)
				usage();
			opt_imapconnect = atoi(*argv);
			if ((opt_imapconnect < 0) || (opt_imapconnect > 100)) {
				printf("imapconnect invalid value\n");
				exit(1);
			}
		}
		if (strcmp(*argv, "-interval") == 0) {
			argc--;
			argv++;
			if (argc < 1)
				usage();
			opt_interval = atoi(*argv);
			if (opt_interval <= 0) {
				printf("interval <= 0\n");
				exit(1);
			}
		}
		if (strcmp(*argv, "-count") == 0) {
			argc--;
			argv++;
			if (argc < 1)
				usage();
			opt_count = atoi(*argv);
			if (opt_count < 1) {
				printf("count < 1\n");
				exit(1);
			}
		}
		argc--;
		argv++;
	}
	if ((opt_imap + opt_pop + opt_local) != 1)
		usage();
	if (opt_local) {
		if (argc != 1) {
			printf("Missing user name\n");
			exit(1);
		}
		opt_user = *argv;
	}
	else {
		if (argc != 2) {
			printf("Missing user/passwd name\n");
			exit(1);
		}	
		opt_user = *argv++;
		opt_passwd = *argv;
		cp = strchr(opt_user, '@');
		if (cp == 0) {
			printf("No domain name in user name\n");
			exit(1);
		}
		*cp++ = 0;
		strcpy(user_name, opt_user);
		strcpy(user_domain, cp);
                if ((user_name[0] == 0) || (user_domain[0] == 0)) {
                        printf("bad address format\n");
                        exit(1);
                }
	}
	if (opt_interval == 0)
		opt_interval = 1;
	signal(SIGINT, done);
	msg_array = mmalloc(opt_count);
	if (opt_local) {
		read_mbox();
		done();
	}
	srand(getpid());
	first = 1;
	for (;;) {
		if (first == 0) {
			if (opt_random) {
				f = rand();
				f = ((f/RAND_MAX) * opt_interval) + 1;
				i = f;
				sleep(i);
			}
			else
				sleep(opt_interval);
		}
		first = 0;
		if (opt_imap)
			do_imap_poll();
		else
			do_pop_poll();
		fflush(stdout);
		check_done();
	}	
}

char *
mmalloc(int size)
{
	char *cp;

	cp = malloc(size);
	if (cp == 0) {
		printf("Malloc out of memory\n");
		exit(1);
	}
	return(cp);
}

/*
 * check our array for missing/dup messages
 */
void
done()
{
	int i;
	int local_err;

	local_err = 0;
	printf("REPORT start\n");
	for (i = 0; i < opt_count; i++) {
		switch (msg_array[i]) {
		case 1:
			break;
		case 0:
			printf("ERROR: Missing message %d\n", i);
			local_err = 1;
			break;
		default:
			printf("ERROR: Got %d copies of message %d\n", msg_array[i], i);
			local_err = 1;
			break;
		}
	}
	if (glob_warn)
		printf("warnings=%d\n", glob_warn);
	if (glob_expunge)
		printf("expunges=%d\n", glob_expunge);
	if (local_err == 0)
		printf("OK - all messages received\n");
	exit(glob_err);
}

void
usage()
{
printf("Usage:\n");
printf("    mailread -pop/-imap -imapconnect n -random -delete -interval n -count n\n");
printf("             user@domain passwd\n");
printf("or: mailread -local user\n");
printf("\n");
printf("    -pop is use POP3 protocol to fetch messages from host.\n");
printf("    -imap is use IMAP4 protocol to fetch messages from host.\n");
printf("    -imapconnect is percent chance per message of reconnect (0 is never)\n");
printf("    -delete is delete messages after fetch.\n");
printf("    -interval is polling interval to check for new messages.\n");
printf("    -random means randomize the polling interval 0-100%% of the interval.\n");
printf("    -protocol - output IMAP/POP protocol to stdout for debugging\n");
printf("    -count number - the number of messages expected.\n");
printf("The program ends when all messages arrive or on a SIGINT.\n");
printf("POP/IMAP modes poll continuously. -local parses a mailbox once.\n");
exit(1);
}

/*
 * assume mailbox is in /var/mail
 */
void
read_mbox()
{
	FILE *fd;
	int i;

	sprintf(buf, "/var/mail/%s", opt_user);

	fd = fopen(buf, "r");
	if (fd == 0) {
		printf("Unable to open %s\n", buf);
		exit(1);
	}
	while (fgets(buf, BUFSIZ, fd)) {
		/* primitive search for messages */
		if (strncmp(buf, "Subject: ", 9) == 0) {
			/* unknown message */
			if (strncmp(buf + 9, "Testing ", 8)) {
				printf("ERROR: Unknown message subject\n%s", buf);
				glob_err = 1;
				continue;
			}
			i = atoi(buf + 17);
			if ((i < 0) || (i >= opt_count)) {
				printf("ERROR: Invalid message number\n%s", buf);
				glob_err = 1;
				continue;
			}
			msg_array[i]++;
		}
	}	
	fclose(fd);
}

/*
 * establish a socket and poll for new messages
 * we have a record of the messages we have received
 */
void
do_pop_poll()
{
	int s;
	int len;
	static int prev_msg;
	int next_msg;
	int msg_size;
	int i;
	int msg;
	char *cp;

	s = iconnect(user_domain, 110);
	if (s == -1) {
		/* just log this error */
		printf("WARN: Unable to POP connect\n");
		glob_warn++;
		return;
	}
	len = recv(s, buf, BUFSIZ, 0);
	buf[len] = 0;
	protocol("<%s", buf);
	/* want OK */
	if (strncmp(buf, "+OK", 3))
		goto pop_err;

	sprintf(buf, "USER %s\r\n", user_name);
	protocol(">%s", buf);
	len = send(s, buf, strlen(buf), 0);

	len = recv(s, buf, BUFSIZ, 0);
	buf[len] = 0;
	protocol("<%s", buf);
	/* want OK */
	if (strncmp(buf, "+OK", 3)) {
		printf("ERROR: login failed: %s\n", buf);
		goto pop_err;
	}

	sprintf(buf, "PASS %s\r\n", opt_passwd);
	protocol(">%s", buf);
	len = send(s, buf, strlen(buf), 0);

	len = recv(s, buf, BUFSIZ, 0);
	buf[len] = 0;
	protocol("<%s", buf);
	/* want OK, warning because folder might lock */
	if (strncmp(buf, "+OK", 3)) {
		printf("WARN: login failed: %s\n", buf);
		glob_warn++;
		goto pop_warn;
	}

	sprintf(buf, "STAT\r\n", opt_passwd);
	protocol(">%s", buf);
	len = send(s, buf, strlen(buf), 0);

	len = recv(s, buf, BUFSIZ, 0);
	buf[len] = 0;
	protocol("<%s", buf);
	/* want OK */
	if (strncmp(buf, "+OK", 3))
		goto pop_err;

	/* most POP servers put it here */
	next_msg = atoi(buf + 4);

	/* any new messages? */
	if (next_msg <= prev_msg)
		goto pop_done;

	/*
	 * fetch each new message, look for the Subject header
	 */

	for (msg = prev_msg + 1; msg <= next_msg; msg++) {
		sprintf(buf, "RETR %d\r\n", msg);
		protocol(">%s", buf);
		len = send(s, buf, strlen(buf), 0);

		len = recv(s, buf, BUFSIZ, 0);
		buf[len] = 0;
		protocol("<%s", buf);
		/* want OK */
		if (strncmp(buf, "+OK", 3))
			goto pop_err;

		/* get header and tag message */
		cp = strstr(buf, "Subject: ");
		if (cp == 0) {
			printf("ERROR: No subject found\n%s", buf);
			glob_err = 1;
			goto pop_rest;
		}
		if (strncmp(cp + 9, "Testing ", 8)) {
			printf("ERROR: Unknown subject:\n%s\n", buf);
			glob_err = 1;
			goto pop_rest;
		}

		i = atoi(cp + 17);
		if ((i < 0) || (i >= opt_count)) {
			printf("ERROR: Invalid message number\n%s", buf);
			glob_err = 1;
			goto pop_rest;
		}

		msg_array[i]++;

pop_rest:
		/* # of bytes to fetch */
		msg_size = atoi(buf + 4);

		/* end of first line */
		cp = strchr(buf, '\n') + 1;

		/* # of bytes left to fetch */
		msg_size -= (len - (cp - buf));
		msg_size += 3;		/* .\r\n trailer */

		while (msg_size) {
			len = recv(s, buf, BUFSIZ, 0);
			buf[len] = 0;
			protocol("<%s", buf);
			msg_size -= len;
			if (len == 0) {
				printf("ERROR: zero len packet received\n");
				glob_err = 1;
				done();
			}
		}
		/* delete message if needed */
		if (opt_delete) {
			sprintf(buf, "DELE %d\r\n", msg);
			protocol(">%s", buf);
			len = send(s, buf, strlen(buf), 0);

			len = recv(s, buf, BUFSIZ, 0);
			buf[len] = 0;
			protocol("<%s", buf);
			/* want OK */
			if (strncmp(buf, "+OK", 3))
				goto pop_err;
		}
		else
			prev_msg = msg;
	}

pop_done:
	sprintf(buf, "QUIT\r\n");
	protocol(">%s", buf);
	len = send(s, buf, strlen(buf), 0);
	close(s);
	return;
pop_err:
	printf("ERROR: POP protocol error:\n");
	printf("%s\n", buf);
	glob_err = 1;
pop_warn:
	goto pop_done;
}

/*
 * establish a socket, log in, and keep the socket open until done.
 * very primitive IMAP parsing goes on here.
 */
void
do_imap_poll()
{
	static int s;
	static int first;
	static int seq;
	static int msgs;	/* current message count */
	static int nmsgs;	/* new message count */
	int msg;
	char *cp;
	char *cp1;
	int len;
	int msg_size;
	float f;
	int i;

	if (s == 0) {
		first = 0;
		s = iconnect(user_domain, 143);
		if (s == -1) {
			printf("WARN: Unable to IMAP connect\n");
			glob_warn++;
			return;
		}
	}

	if (first == 0) {
		first = 1;
		len = recv(s, buf, BUFSIZ, 0);
		buf[len] = 0;
		protocol("<%s", buf);
		if (strncmp(buf, "* OK", 4)) {
			printf("WARN: Welcome message not received\n");
			glob_warn++;
			goto imap_close;
		}

		sprintf(buf, "%d LOGIN %s %s\r\n", seq, user_name, opt_passwd);
		protocol(">%s", buf);
		len = send(s, buf, strlen(buf), 0);

		len = recv(s, buf, BUFSIZ, 0);
		buf[len] = 0;
		protocol("<%s", buf);
		sprintf(tbuf, "%d OK LOGIN completed", seq++);
		if (strncmp(buf, tbuf, strlen(tbuf))) {
			printf("WARN: LOGIN failed: %s\n", buf);
			glob_warn++;
			goto imap_close;
		}

		sprintf(buf, "%d SELECT INBOX\r\n", seq);
		protocol(">%s", buf);
		len = send(s, buf, strlen(buf), 0);

		len = recv(s, buf, BUFSIZ, 0);
		buf[len] = 0;
		protocol("<%s", buf);
		sprintf(tbuf, "%d OK [READ-WRITE] SELECT completed", seq++);
		/* can fail due to lock, if so retry later */
		if (strstr(buf, tbuf) == 0) {
			printf("WARN: SELECT failed: %s\n", buf);
			glob_warn++;
			goto imap_close;
		}
		nmsgs = check_exists(buf, nmsgs);
	}
	else {
		sprintf(buf, "%d NOOP\r\n", seq);
		protocol(">%s", buf);
		len = send(s, buf, strlen(buf), 0);

		len = recv(s, buf, BUFSIZ, 0);
		buf[len] = 0;
		protocol("<%s", buf);
		sprintf(tbuf, "%d OK NOOP completed", seq++);
		if (strstr(buf, tbuf) == 0) {
			printf("ERROR: NOOP failed: %s\n", buf);
			glob_err = 1;
			done();
		}
		nmsgs = check_exists(buf, nmsgs);
	}
	if (nmsgs < msgs)
		nmsgs = msgs;
	if (nmsgs == msgs)
		return;

	/* fetch messages */
	for (msg = msgs; msg < nmsgs; msg++) {
		sprintf(buf, "%d FETCH %d RFC822\r\n", seq, msg + 1);
		protocol(">%s", buf);
		len = send(s, buf, strlen(buf), 0);

		len = recv(s, buf, BUFSIZ, 0);
		buf[len] = 0;
		protocol("<%s", buf);
		sprintf(tbuf, "* %d FETCH (RFC822 {", msg + 1);
		cp1 = strchr(tbuf, '}');
		cp = strstr(buf, tbuf);
		if (cp == 0) {
			printf("ERROR: FETCH failed: %s\n", buf);
			glob_err = 1;
			done();
		}
		nmsgs = check_exists(buf, nmsgs);

		/* get header and tag message */
		cp = strstr(buf, "Subject: ");
		if (cp == 0) {
			printf("ERROR: No subject found\n%s", buf);
			glob_err = 1;
			goto imap_rest;
		}
		if (strncmp(cp + 9, "Testing ", 8)) {
			printf("ERROR: Unknown subject:\n%s\n", buf);
			glob_err = 1;
			goto imap_rest;
		}

		i = atoi(cp + 17);
		if ((i < 0) || (i >= opt_count)) {
			printf("ERROR: Invalid message number\n%s", buf);
			glob_err = 1;
			goto imap_rest;
		}

		msg_array[i]++;

imap_rest:
		/* message len is here */
		cp += strlen(tbuf);
		msg_size = atoi(cp);
		cp = strchr(cp, '\n');

		/* # of bytes left to fetch */
		msg_size -= (len - (cp - buf));

		/* trailer size
		sprintf(tbuf, ")\r\n%d OK FETCH completed\r\n", seq++);
		msg_size += strlen(tbuf);

		while (msg_size) {
			len = recv(s, buf, BUFSIZ, 0);
			buf[len] = 0;
			protocol("<%s", buf);
			msg_size -= len;
			if (len == 0) {
				printf("ERROR: zero len packet received\n");
				glob_err = 1;
				done();
			}
			nmsgs = check_exists(buf, nmsgs);
		}
		/* delete message if needed */
		if (opt_delete) {
			sprintf(buf, "%d STORE %d +FLAGS \\Deleted\r\n", seq, msg + 1);
			protocol(">%s", buf);
			len = send(s, buf, strlen(buf), 0);

			len = recv(s, buf, BUFSIZ, 0);
			buf[len] = 0;
			protocol("<%s", buf);
			sprintf(tbuf, "%d OK STORE completed", seq++);
			cp = strstr(buf, tbuf);
			if (cp == 0) {
				printf("ERROR: STORE failed:\n%s\n", buf);
				glob_err = 1;
				done();
			}
			nmsgs = check_exists(buf, nmsgs);
		}
	}
	msgs = nmsgs;
	/* % chance of expunge and logout */
	f = rand();
	f = ((f/RAND_MAX) * 100) + 1;
	i = f;
	if (i < opt_imapconnect) {
		sprintf(buf, "%d CLOSE\r\n", seq);
		protocol(">%s", buf);
		len = send(s, buf, strlen(buf), 0);

		len = recv(s, buf, BUFSIZ, 0);
		buf[len] = 0;
		protocol("<%s", buf);
		sprintf(tbuf, "%d OK ", seq++);
		cp = strstr(buf, tbuf);
		if (cp == 0) {
			printf("ERROR: CLOSE failed\n");
			glob_err = 1;
			done();
		}
		glob_expunge++;
		if (opt_delete)
			msgs = 0;

		sprintf(buf, "%d LOGOUT\r\n", seq);
		protocol(">%s", buf);
		len = send(s, buf, strlen(buf), 0);

		len = recv(s, buf, BUFSIZ, 0);
		buf[len] = 0;
		protocol("<%s", buf);
		sprintf(tbuf, "%d OK LOGOUT completed", seq++);
		cp = strstr(buf, tbuf);
		if (cp == 0) {
			printf("ERROR: LOGOUT failed\n");
			glob_err = 1;
			done();
		}

imap_close:
		/* close socket */
		close(s);
		s = 0;
	}
}

/*
 * look for "* n EXISTS\r\n" and return the value of n
 */
int
check_exists(char *str, int nmsgs)
{
	char *cp;
	char *cp1;
	int val;

	cp = strstr(str, " EXISTS\r\n");
	if (cp == 0)
		return(nmsgs);
	cp1 = cp;
	cp--;
	while (isdigit(*cp))
		cp--;
	/* cp should be a blank now */
	if (*cp != ' ')
		return(nmsgs);
	cp++;
	*cp1 = 0;
	val = atoi(cp);
	*cp1 = ' ';

	return(val);
}

int
iconnect(char *hostname, int port)
{
	int s;
	struct hostent *hent;
	struct sockaddr_in sin;
	int ret;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1)
		return(s);
	hent = gethostbyname(hostname);
	if (hent == NULL) {
		printf("Unable to resolve %s\n", hostname);
		close(s);
		return(-1);
	}
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	memset(&sin.sin_addr, 0, sizeof(sin.sin_addr));
	memcpy(&sin.sin_addr, hent->h_addr, hent->h_length);
	memset(sin.sin_zero, 0, sizeof(sin.sin_zero));

	ret = connect(s, (struct sockaddr *)&sin, sizeof(sin));
	return(s);
}

void
check_done()
{
	int i;

	for (i = 0; i < opt_count; i++) {
		if (msg_array[i] == 0)
			return;
	}
	done();
}
