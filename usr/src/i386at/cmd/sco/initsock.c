#ident	"@(#)sco:initsock.c	1.1.2.1"

/* Enhanced Application Compatibility Support */

#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/param.h"
#include "sys/file.h"
#include "sys/vnode.h"
#include "sys/stropts.h"
#include "sys/stream.h"
#include "sys/strsubr.h"
#include "netinet/in.h"
#include "sys/tiuser.h"
#include "sys/sockmod.h"
#include "sys/osocket.h"
#include "sys/stat.h"


struct osocknewproto protolist[] = {
	{ OAF_INET, OSOCK_STREAM, 6, 0, OPR_CONNREQUIRED }, 
	{ OAF_INET, OSOCK_DGRAM, 17, 0, OPR_ADDR | OPR_ATOMIC },
	{ OAF_INET, OSOCK_RAW,    1, 0, OPR_ADDR | OPR_ATOMIC },
	{ OAF_INET, OSOCK_RAW,    0, 0, OPR_ADDR | OPR_ATOMIC | OPR_BINDPROTO },
	{ OAF_UNIX, OSOCK_STREAM, 0, 0, OPR_ADDR | OPR_ATOMIC },
	{ OAF_UNIX, OSOCK_DGRAM,  0, 0, OPR_ADDR | OPR_ATOMIC },
};

int nitems = sizeof(protolist) / sizeof(protolist[0]);

char *devnames[] = {
	"/dev/tcp",
	"/dev/udp",
	"/dev/icmp",
	"/dev/rawip",
	"/dev/ticots",
	"/dev/ticlts"
};

main(argc, argv)
int	argc;
char	**argv;
{

	int			sockfd;
	int			i;
	struct osocknewproto	*prp;
	char			*namep;
	int			verbose;

	verbose = 0;
	argc--; argv++;
	if (argc > 0) {
		namep = *argv;
		
		if (namep[0] == '-' && namep[1] == 'v')
			verbose++;
	}

	sockfd = sco_socket(0, 0, 0);
	if (sockfd < 0) {
		perror("");
		exit(1);
	}

	if (ioctl(sockfd, OSIOCXPROTO, 0) < 0) {
		perror("");
		exit(1);
	}

	prp = protolist;
	for (i = 0; i < nitems; i++) {
		namep = devnames[i];
		prp->dev = getdev(namep);
		if (verbose)
			printproto(prp);

		if (ioctl(sockfd, OSIOCPROTO, prp) < 0) {
			perror("");
			exit(1);
		}

		prp++;
	}
}

/*
 * getdev - get the external major and minor from stat'ed dev node
 */
getdev(namep)
char	*namep;
{
	struct stat	stat_buf;
	int		rval;

	rval = stat(namep, &stat_buf);
	if (rval < 0 ) {
		perror("");
		exit(1);
	}
		
	return(stat_buf.st_rdev);
}


/*
 * Display the proto entry
 */
printproto(prp)
struct osocknewproto *prp;
{
	printf(
	    "Family %d, type %d, proto %d, dev <%d, %d>, flags 0x%x\n",
		prp->family, prp->type, prp->proto, 
		getemajor(prp->dev), geteminor(prp->dev), prp->flags);
}

sco_socket(family, type, proto)
{
	struct osocksysreq	sock_req;
	int			sockfd;
	int			rval;
	int			newfd;

	sockfd = open("/dev/socksys", 0);
	if (sockfd < 0) {
		perror("");
		return(sockfd);
	}

	sock_req.args[0] = OSO_SOCKET;
	sock_req.args[1] = family;
	sock_req.args[2] = type;
	sock_req.args[3] = proto;

	rval  = ioctl(sockfd, OSIOCSOCKSYS, (caddr_t)&sock_req);
	close(sockfd);
	if (rval < 0)	{
		return (rval);
	} else {
		newfd = dup2(rval, sockfd);
		close(rval);
		if (newfd < 0) {
			perror("");
			return (newfd);
		} else
			return (sockfd);
	}
}

/* Enhanced Application Compatibility Support */
