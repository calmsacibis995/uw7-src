#ident	"@(#)pcintf:bridge/loadpci.c	1.1.1.4"
#include	"sccs.h"
SCCSID(@(#)loadpci.c	6.6	LCC);	/* Modified: 09:49:50 2/20/92 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

/*
 *	PCI program loader.
 *
 *	Loads PCI consvr, mapsvr after opening network and reading ifc list.
 */

#include "sysconfig.h"

#include <errno.h>
#include <string.h>

#include <lmf.h>

#include "pci_proto.h"
#include "pci_types.h"
#include "common.h"
#include "log.h"

int		main			PROTO((int, char **));
LOCAL void	add_local_interface	PROTO((void));
LOCAL void	build_interface_list	PROTO((void));

#if defined(UDP42)
extern void	hostaddr		PROTO((struct sockaddr_in *));
#endif	/* UDP42 */

char *myname = "loadpci";

netIntFace	intFaceList[MAX_NET_INTFACE];

int		numIntFace = 0;

#define MAXARGS	64

char *args[MAXARGS];

char net_buf[8];
char int_list_buf[4096];


main(argc, argv)
int argc;
char **argv;
{
	char **argp;
	register char *arg;
	char *pgm = NULL;
	int brd_local = 0;
	int mode = 0;
	int have_interfaces = 0;
	int net_desc;
	int port = PCI_CONSVR_PORT;
	char *netDev = NETDEV;		/* Network interface device */
	char fqNetDev[32];		/* Fully qualified netDev name */

	/* determine machine byte ordering */
	byteorder_init();

	/* initialize NLS */
	nls_init();
	argp = &args[1];
	while (argc-- > 1) {
		arg = *++argv;

		if (*arg != '-') {
			pgm = arg;
			args[0] = pgm;
			continue;
		}

		switch (arg[1]) {

		case 'B':
			mode |= BCAST43;
			break;

		case 'I':
			*argp++ = arg;
			have_interfaces++;
			break;

		case 'N':			/* Network device */
			netDev = &arg[2];
			if (netDev == '\0')
				netDev = NETDEV;
			else if (*netDev != '/') {
				sprintf(fqNetDev, "/dev/%s", netDev);
				netDev = fqNetDev;
			}
			*argp++ = arg;
			break;

		case 'P':
			if (!strcmp(&arg[2], "CONSVR"))
				port = PCI_CONSVR_PORT;
			else if (!strcmp(&arg[2], "MAPSVR"))
				port = PCI_MAPSVR_PORT;
			else
				sscanf(&arg[2], "%d", &port);
			break;

		case 'S':
			mode |= USESUBNETS;
			break;

		case 'b':
			brd_local++;
			break;

		case 'n':
			fprintf(stderr, lmf_get_message("LOADPCI1",
			  "loadpci: Can't specify network descriptor\n"));
			exit(1);

		case 'D':
			dbgSet(strtol(&arg[2], NULL, 16));
			if (dbgEnable != 0)
				logOpen(LOADPCI_LOG, getpid());
			/* Falls through... */

		default:
			*argp++ = arg;
			break;
		}
	}
	if ((net_desc = netOpen(netDev, port)) < 0) {
		fprintf(stderr, lmf_get_message("LOADPCI2", 
			"loadpci: Can't open network - Bye\n"));
		exit(1);
	}
	sprintf(net_buf, "-n%d", net_desc);
	*argp++ = net_buf;

	if (!have_interfaces) {
		netaddr(intFaceList, &numIntFace, mode);
		if (brd_local)
			add_local_interface();
		build_interface_list();
		*argp++ = int_list_buf;
	}

	if (pgm == NULL) {
		fprintf(stderr, lmf_get_message("LOADPCI3",
			"loadpci: No program name\n"));
		exit(1);
	}

	*argp = NULL;
	execv(pgm, args);
	fprintf(stderr,lmf_format_string((char *) NULL, 0, 
		lmf_get_message("LOADPCI4", "loadpci: Couldn't exec %1\n"),
		"%s" , pgm));
	return 0;
}


LOCAL void
add_local_interface()
{
	struct sockaddr_in sa;
	netIntFace *ifp;

	hostaddr(&sa);
	ifp = &intFaceList[numIntFace++];
	ifp->localAddr.s_addr = sa.sin_addr.s_addr;
	ifp->broadAddr.s_addr = sa.sin_addr.s_addr;
	ifp->subnetMask.s_addr = 0xffffffffL;
}


LOCAL void
build_interface_list()
{
	register char *bp;
	register int i;
	register netIntFace *ifp;

	sprintf(int_list_buf, "-I");
	bp = &int_list_buf[2];
	ifp = intFaceList;
	for (i = 0; i < numIntFace; i++) {
		sprintf(bp, "%s,", inet_ntoa(ifp->localAddr));
		bp = &bp[strlen(bp)];
		sprintf(bp, "%s,", inet_ntoa(ifp->broadAddr));
		bp = &bp[strlen(bp)];
		sprintf(bp, "%s;", inet_ntoa(ifp->subnetMask));
		bp = &bp[strlen(bp)];
		ifp++;
	}
}
