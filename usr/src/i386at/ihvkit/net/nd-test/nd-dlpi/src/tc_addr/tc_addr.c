#include <stdio.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/dlpi.h>
#include <sys/errno.h>
#include <sys/dlpi_ether.h>
#include <dl.h>
#include <tc_res.h>
#include <tc_msg.h>
#include <tc_net.h>
#include <tet_api.h>

extern int	errno;
extern int	dl_errno;

extern char	*tn_init_err[];

void ic_addr0();
/*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = NULL; /* Test case start snd */
void (*tet_cleanup)() = NULL; /* Test case end snd */

struct tet_testlist tet_testlist[] = {
	{ic_addr0, 1},
	{NULL, -1}
};


void
ic_addr0()
{
	char	*name;
	int	i;
	char	eaddr[6];
	char	naddr[4];

	printf("Here\n");
	tet_delete(2, "JUNK");

	if ((name = tet_getvar("NODE0")) == NULL) {
		tet_infoline("No NODE name given\n");
		DL_DEBUG(1,"No NODE name given\n");
		return;
	}

	printf("Get addr for: %s\n", name);
	if (getaddr(name, eaddr, naddr) < 0) {
		printf("Get mac addr failed\n");
		exit(1);
	}	

	printf("NET address [");
	for (i = 0; i < 4; i++)
		printf("%x ", naddr[i] & 0x0ff);
	printf("]\n");

	printf("MAC address [");
	for (i = 0; i < 6; i++)
		printf("%x ", eaddr[i] & 0x0ff);
	printf("]\n");
}
