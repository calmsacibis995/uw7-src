/*
 * program to allow connecting to a socket to the
 * to allow for testing.
 * makes the socket appear as stdin and stdout.
 * stdin one line (newline terminated) is one message to the server
 * a response from the server is expected but not necessary
 * a timeout mechanism is used to detect no response and to go
 * back to stdin.
 */

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#define MEGABYTE (1024*1024)

int itimeout = 3;
char *ihostname;
int iport;
int alsig;

void usage();
void dostuff(unsigned int);

main(argc, argv)
char **argv;
int argc;
{
	unsigned int isocket;

	argc--;
	argv++;
	if (argc < 2)
		usage();
	if (strcmp(*argv, "-t") == 0) {
		if (argc != 2)
			usage();
		itimeout = atoi(argv[1]);
		if (itimeout < 2)
			itimeout = 2;
		argc -= 2;
		argv += 2;
	}
	ihostname = argv[0];
	iport = atoi(argv[1]);
	if ((isocket = iconnect(ihostname, iport)) == -1) {
		printf("Unable to connect to %s %d\n", ihostname, iport);
		exit(1);
	}
	dostuff(isocket);
	exit(0);
}

void
usage()
{
	printf("usage: socket [-t timeout] hostname port\n");
	exit(1);
}

int
iconnect(char *hostname, int port)
{
	unsigned int s;
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

int
al1()
{
	alsig = 1;
}

void
dostuff(unsigned int socket)
{
	char *buf;
	int size;

	buf = (char *)malloc(MEGABYTE);
	if (buf == 0) {
		printf("Malloc out of memory\n");
		exit(1);
	}
	alarm(SIGALRM, al1);
	while (1) {
		alsig = 0;
		alarm(itimeout);
		size = recv(socket, buf, MEGABYTE, 0);
		alarm(0);
		if (size != -1)
			write(1, buf, size);
		if (fgets(buf, MEGABYTE, stdin) == NULL)
			return;
		buf[strlen(buf)-1] = 0;	/* kill newline */
		printf("<%s>\n", buf); fflush(stdout);
		strcat(buf, "\r\n");
		size = send(socket, buf, strlen(buf), 0);
	}
}
