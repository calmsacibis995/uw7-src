#ident	"@(#)pcintf:bridge/ifc_list.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)ifc_list.c	6.4	LCC);	/* Modified: 23:21:39 7/12/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

/*
 *	Interface list parser
 */

#include "sysconfig.h"

#include <errno.h>
#include <string.h>

#include "pci_proto.h"
#include "pci_types.h"
#include "common.h"
#include "log.h"

static int		parse_interface	PROTO((char *, netIntFace *, int));

extern netIntFace	intFaceList[MAX_NET_INTFACE];


get_interface_list(arg)
register char *arg;
{
	register char *ap;
	int num_ifc;

	num_ifc = 0;
	while ((ap = strchr(arg, ';')) != NULL) {
		*ap++ = '\0';
		if (parse_interface(arg, &intFaceList[num_ifc], num_ifc))
			num_ifc++;
		arg = ap;
	}
	if (*arg && parse_interface(arg, &intFaceList[num_ifc], num_ifc))
		num_ifc++;
	return num_ifc;
}

parse_interface(arg, ifc, ifc_num)
register char *arg;
netIntFace *ifc;
int ifc_num;
{
	register char *ap;
	long	*brdp;
	long	*subnetp;
	char *brd_arg;
	char *subnet_arg;
	unsigned long addr;

	if (*arg == '0')
		return 0;

	brdp = (long *) &(ifc->broadAddr);
	subnetp = (long *) &(ifc->subnetMask);

	brd_arg = subnet_arg = NULL;
	if ((ap = strchr(arg, ',')) != NULL) {
		*ap++ = '\0';
		if (*ap && *ap != ',')
			brd_arg = ap;
		if ((ap = strchr(ap, ',')) != NULL) {
			*ap++ = '\0';
			subnet_arg = ap;
		}
	}

	if ((addr = inet_addr(arg)) == 0xffffffffL)
		return 0;
	
	/* Copy the local address */

	memcpy(&ifc->localAddr, &addr, sizeof(struct in_addr));

	if (brd_arg != NULL && (addr = inet_addr(brd_arg)) != 0xffffffffL)
		memcpy(&ifc->broadAddr, &addr, sizeof(struct in_addr));
	else {
		*brdp = ntohl(*((long *)&ifc->localAddr));

		if (IN_CLASSA(*brdp))
			 *brdp |= ~IN_CLASSA_NET;
		else if (IN_CLASSB(*brdp))
			 *brdp |= ~IN_CLASSB_NET;
		else if (IN_CLASSC(*brdp))
			 *brdp |= ~IN_CLASSC_NET;

		 *brdp = htonl(*brdp);
	}

	if (subnet_arg != NULL && (addr = inet_addr(subnet_arg)) != 0xffffffffL)
		memcpy(&ifc->subnetMask, &addr, sizeof(struct in_addr));
	else {
		*brdp = ntohl(*brdp);

		if (IN_CLASSA(*brdp))
			 *subnetp = IN_CLASSA_NET;
		else if (IN_CLASSB(*brdp))
			 *subnetp = IN_CLASSB_NET;
		else if (IN_CLASSC(*brdp))
			 *subnetp = IN_CLASSC_NET;

		 *brdp = htonl(*brdp);
		 *subnetp = htonl(*subnetp);
	}

	log("intFaceList[%d]: Local %s Broadcast %s Subnet mask %s\n",
		ifc_num,
		nAddrFmt((unsigned char *)&ifc->localAddr),
		nAddrFmt((unsigned char *)&ifc->broadAddr),
		nAddrFmt((unsigned char *)&ifc->subnetMask));
	return 1;
}
