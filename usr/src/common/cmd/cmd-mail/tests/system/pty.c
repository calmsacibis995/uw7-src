#ident "@(#)pty.c	11.1"
/*
 * program to fork a shell and connect to it via a pseudo tty.
 * this program is a very simple expect like program.
 *
 * inputs to this program are lines starting with "send: " or "receive: "
 * send lines are passed to the program with a newline appended,
 * recv lines are treated as strings to wait for.
 * timeout on recv's is 10 seconds.
 *
 */
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#define debug printf

int shell_pid;

char ibuf[BUFSIZ];
char obuf[BUFSIZ*2 + 1];
char obuf2[BUFSIZ*2 + 1];

int obuflen;		/* amount of data in obuf */

char master_name[80];
char slave_name[80];
int master_fd;
int slave_fd;

main(argc, argv)
char **argv;
{
	/* first get a pty */
	find_pty();
	/* spawn a shell */
	spawn();
	/* process our input script */
	do_script();
	exit(0);
}

/*
 * find and open the pseudo tty
 */
find_pty()
{
	int i;

	for (i = 0; i < 999; i++) {
		sprintf(master_name, "/dev/ptyp%d", i);
		master_fd = open(master_name, 2);
		if (master_fd < 0) {
			if (errno == ENOENT) {
				printf("Unable to reserve pty\n");
				exit(1);
			}
			continue;
		}
		break;
	}
	if (i == 999) {
		printf("Unable to reserve pty\n");
		exit(1);
	}
	/* got one */
	sprintf(slave_name, "/dev/ttyp%d", i);
}

alrm()
{
	printf("Timeout\n");
	if (shell_pid) {
		kill(shell_pid, SIGHUP);
	}
	exit(1);
}

die()
{
	if (shell_pid) {
		kill(shell_pid, SIGHUP);
	}
	exit(0);
}

/*
 * fork a shell and connect it to our tty
 */
spawn()
{
	/* fork a shell */
	shell_pid = fork();
	if (shell_pid < 0) {
		printf("Unable to fork shell\n");
		exit(1);
	}
	if (shell_pid == 0)
		do_shell();
}

/*
 * process stdin, return on eof, must do line buffering
 */
do_script()
{
	int len;
	char *cp;

	/* only parent catches signals */
	signal(SIGINT, (void(*)())die);
	nap(500);

	while (fgets(ibuf, BUFSIZ, stdin)) {
		len = strlen(ibuf);
		if (strncmp(ibuf, "send: ", 6) == 0) {
			debug("send: %s", ibuf + 6);
			write(master_fd, ibuf + 6, len - 6);
		}
		else if (strncmp(ibuf, "recv: ", 6) == 0) {
			len = strlen(ibuf);
			ibuf[len-1] = 0;
			signal(SIGALRM, (void(*)())alrm);
			alarm(10);
			obuflen = 0;
			debug("want: %s\n", ibuf + 6);
			for (;;) {
				len = read(master_fd, obuf2, BUFSIZ);
				debug("recv len %d\n", len);
				if (len <= 0) {
					sleep(1);
					continue;
				}
				obuf2[len] = 0;
				debug("recv: <%s>\n", obuf2);
				if ((obuflen + len) >= (BUFSIZ*2))
					obuflen -= len;
				memcpy(obuf2 + len, obuf, obuflen);
				obuflen += len;
				memcpy(obuf, obuf2, obuflen);
				obuf[obuflen] = 0;
				/* now do pattern patch */
				cp = strstr(obuf, ibuf + 6);
				if (cp)
					break;
			}
			signal(SIGALRM, 0);
		}
		else {
			printf("pty: Unknown primitive: %s", ibuf);
			die();
		}
	}
	/* we should get here on eof, send SIGTERM signal to shell */
	die();
}

/*
 * this routine exec's a shell, it does not return
 */
do_shell()
{
	int i, j, k;
	int len;

	for (i = 0; i < 20; i++)
		close(i);
	setsid();
	i = open(slave_name, 2);
	j = open(slave_name, 2);
	k = open(slave_name, 2);
	execl("/bin/sh", "-sh", 0);
	printf("Unable to exec sh\n");
	exit(1);
}

usage()
{
	printf("Usage: pty\n");
	printf("    pty forks a shell and connects to it with a pseudo tty\n");
	printf("    pty's stdin is a simple scripting lauguage with two primitives:\n");
	printf("    send: <string> - send a string (with newline) to shell\n");
	printf("    recv: <string> - wait for string in output, ten second timeout\n");
	printf("    if a timeout occurs pty returns an error\n");
	printf("    all output from shell process is sent to stdout\n");
	exit(1);
}
