#ident "@(#)socket.c	11.1"
/*
 * program to determine if TCP/IP is up or not
 * grab a socket, if it works return ok, otherwise return failure.
 * used by /etc/rc2.d/*sendmail to determine if the SMTP part
 * of the sendmail daemon should be started.
 */
#include <sys/types.h>
#include <sys/socket.h>

main()
{
	int s;

	s = socket(PF_INET, SOCK_STREAM, 0);
	if (s < 0)
		exit(1);
	exit(0);
}
