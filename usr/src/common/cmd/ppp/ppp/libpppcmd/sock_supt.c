#ident	"@(#)sock_supt.c	1.3"

#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/fcntl.h>
#include "pathnames.h"

#include <pfmt.h>
#include <locale.h>
#include <unistd.h>

int
ppp_sockinit()
{
	int	s;
	struct sockaddr_un sin_addr;
	struct servent *serv;

	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		return -errno;

	sin_addr.sun_family = AF_UNIX;
	strcpy(sin_addr.sun_path, PPP_PATH);

	if (connect(s, (struct sockaddr *)&sin_addr, sizeof(sin_addr)) < 0)
		return -errno;
	return s;
}
