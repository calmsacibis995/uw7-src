#ident	"@(#)datarecv.c	5.1"

void	datarecv_startup();
void	datarecv_cleanup();
void 	datarecv();

/* TET essentials */
#include "tet_api.h"

void (*tet_startup)() = datarecv_startup;
void (*tet_cleanup)() = datarecv_cleanup;

struct tet_testlist tet_testlist[] = { 
	{datarecv,1}, 
	{NULL,0} 
};

#define IC_MAX	sizeof(tet_testlist)/sizeof(struct tet_testlist)

/* end of TET essentials */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "stress.h"
#include "stress_msg.h"

char	nodestr[64];
char	*node[10];
int	tli_debug;
char	*protocol;
int	srv_port;
int	nodecount;
int	min_pkt_len, max_pkt_len;

char	*datahead;

void
datarecv_startup()
{
	int i;

	tet_infoline(DATARECV);

	tli_debug = atoi(tet_getvar("TLI_DEBUG"));
	protocol = tet_getvar("PROTOCOL");
	srv_port = atoi(tet_getvar("SERVER_PORT"));
	nodecount = atoi(tet_getvar("NODECOUNT"));
	if ((protocol == NULL) || (nodecount < 2)) {
		tet_infoline(CONF_ERROR);
		TLI_DEBUG1(1,"datarecv_startup:ERROR: %s",CONF_ERROR);
		for (i = 0; i < IC_MAX; i++)
			tet_delete(i+1, CONF_ERROR);
		return;
	}
	
	for (i = 0; i < nodecount; i++) {
		sprintf(nodestr,"NODE%d",i);
		node[i] = tet_getvar(nodestr);
		TLI_DEBUG1(2,"%s\n",node[i]);
		if (node[i] == '\0') {
			tet_infoline(CONF_ERROR);
			TLI_DEBUG1(1,"datarecv_startup:ERROR: %s",CONF_ERROR);
			for (i = 0; i < IC_MAX; i++)
				tet_delete(i+1, CONF_ERROR);
			return;
		}
	}
	
	min_pkt_len = atoi(tet_getvar("PACKET_MIN"));
	max_pkt_len = atoi(tet_getvar("PACKET_MAX"));
	if (min_pkt_len < MIN_PACKET || min_pkt_len >= MAX_PACKET)
		min_pkt_len = MIN_PACKET;
	if (max_pkt_len <= MIN_PACKET || max_pkt_len > MAX_PACKET)
		max_pkt_len = MAX_PACKET;
	if (min_pkt_len >= max_pkt_len)
		min_pkt_len = MIN_PACKET;
	TLI_DEBUG2(2,"datarecv_startup:INFO: min_pkt_len=%d max_pkt_len=%d\n",min_pkt_len, max_pkt_len);
}
void
datarecv()
{

	int	fd;
	void	*tli_info;

	struct packet *pktptr;
	char	*dataptr;
	int	datalen, len;

	int	i;
	int	childpid[10], child_status;
	int	result = 0;

	for (i = 1;i < nodecount; i++) {
		/* create a new process for this endpoint */
		if ((childpid[i] = fork()) < 0) {
			TLI_DEBUG(1,"datarecv:ERROR: fork failed\n");
			exit(2);
		}
		if (childpid[i] == 0) 		/* child process */
			break;
	}	
	if (i == nodecount) {			/* parent process */
		result = 0;
		for (i = 1; i < nodecount; i++) {
			waitpid(childpid[i],&child_status, WUNTRACED);
			if (WEXITSTATUS(child_status)) {
				result = WEXITSTATUS(child_status);
				TLI_DEBUG3(1,"datarecv:ERROR: test with NODE%d(%s) returned %d\n",i,node[i],result);
			}
		}
		if (result == 0) {
			TLI_DEBUG(1,"datarecv:INFO: passed\n");
			tet_result(TET_PASS);
		}
		else if (result == 1) {
			TLI_DEBUG(1,"datarecv:INFO: failed\n");
			tet_result(TET_FAIL);
		}
		else {
			TLI_DEBUG(1,"datarecv:INFO: unresolved\n");
			tet_result(TET_UNRESOLVED);
		}
		return;
	}	
	
	if ((fd = BindAndConnect(node[i], protocol, srv_port)) < 0) {
		TLI_DEBUG2(1,"datarecv:ERROR: BindAndConnect to NODE%d(%s) failed\n",i, node[i]);
		exit(2);
	}

	/* allocate a buffer for MAX_DATA packets */
	if ((datahead = (char *)malloc(MAX_DATA)) == NULL) {
		TLI_DEBUG(1,MEM_FAIL);
		exit(2);
	}

	datalen = 0;
	dataptr = datahead;
	pktptr  = (struct packet *)datahead;
	for ( ; ; ) {
		if ((len = ReceiveData(fd, dataptr, MAX_DATA)) < 0) {
			TLI_DEBUG(1,"datarecv:ERROR: ReceiveData failed\n");
			result = 1;
			break;
		}
		if (len == 0)
			break;
		TLI_DEBUG1(5,"datarecv:INFO: received packet len=%d\n",len);
		/* echo it back */
		if (SendData(fd, dataptr, len) < len) {
			TLI_DEBUG(1,"datarecv:ERROR: SendData failed\n");
			result = 1;
			break;
		}
		TLI_DEBUG1(5,"datarecv:INFO: sent packet len=%d\n",len);
	}	/* for loop */
	if (datahead)
		free(datahead);
	exit(result);
}	

void
datarecv_cleanup()
{
}
