#ident	"%W"

/* TET essentials */
#include <stdlib.h>
#include <signal.h>
#include "tet_api.h"
#include "stress.h"
#include "stress_msg.h"

void	datasend_startup();
void	datasend_cleanup();

void	datasend();

void (*tet_startup)() = datasend_startup;
void (*tet_cleanup)() = datasend_cleanup;

struct tet_testlist tet_testlist[] = { 
	{datasend,1}, 
	{NULL,0} 
};

#define IC_MAX	sizeof(tet_testlist)/sizeof(struct tet_testlist)

/* end of TET essentials */

int	tli_debug;
char	*protocol, *node0;
int	srv_port, cli_port;
int	min_pkt_len, max_pkt_len;
int	stress_time;

static int	sigalrm();
int	stop = 0;
char	*datahead;
ulong	dataerr;
ulong	totalbytes = 0;
ulong	totalpkts = 0;

void	SendProc(), ReceiveProc();

void
datasend_startup()
{
	int i;

	tet_infoline(DATASEND);

	/* Stress time will be specified in minutes */
	if(tet_getvar("STRESS_TIME"))
		stress_time = atoi(tet_getvar("STRESS_TIME")) * 60;
	else
		stress_time = 3 * 60 * 60;  /* three hours */

	tli_debug = atoi(tet_getvar("TLI_DEBUG"));
	protocol = tet_getvar("PROTOCOL");
	node0 = tet_getvar("NODE0");
	srv_port = atoi(tet_getvar("SERVER_PORT"));

	min_pkt_len = atoi(tet_getvar("PACKET_MIN"));
	max_pkt_len = atoi(tet_getvar("PACKET_MAX"));
	if (min_pkt_len < MIN_PACKET || min_pkt_len >= MAX_PACKET)
		min_pkt_len = MIN_PACKET;
	if (max_pkt_len <= MIN_PACKET || max_pkt_len > MAX_PACKET)
		max_pkt_len = MAX_PACKET;
	if (min_pkt_len >= max_pkt_len)
		min_pkt_len = MIN_PACKET;
	TLI_DEBUG2(2,"datasend_startup:INFO: min_pkt_len=%d max_pkt_len=%d\n",min_pkt_len, max_pkt_len);

	if (protocol == NULL || node0 == NULL) {
		tet_infoline(CONF_ERROR);
		for (i = 0; i < IC_MAX; i++)
			tet_delete(i+1, CONF_ERROR);
	}
}

static char 	tmpbuf[128];
static char 	*datahead;
static int 	datalen;

void
datasend()
{

	int	fd, newfd;
	int	len;
	int	range;
	int	result = 0;

	int	childpid;
	void 	*tli_info;

	if ((fd = BindReserved(NULL, protocol, srv_port, &tli_info)) < 0) {
		TLI_DEBUG(1,"datarecv:ERROR: BindReserved failed\n");
		exit(2);
	}

	for (;;) {
		if (Listen(fd, tli_info) < 0) {
			TLI_DEBUG(1,"datarecv:ERROR: Listen failed\n");
			exit(2);
		}
		if ((newfd = Accept(fd, tli_info)) <= 0) {
			TLI_DEBUG(1,"datarecv:ERROR: Accept failed\n");
			exit(2);
		}
		/* create a new process for this datapoint */
		if ((childpid = fork()) < 0) {
			TLI_DEBUG(1,"datarecv:ERROR: fork failed\n");
			exit(2);
		}
		else if (childpid == 0) {		/* child */
			Close(fd);
			break;
		}
	}


	/* Start timer. */
	signal(SIGALRM, (void (*)(int))sigalrm);
	if (alarm(stress_time) < 0) {
		perror("datasend: alarm");
		TLI_DEBUG(1,"datasend: alarm failed\n");
		exit(2);
	}

	/* allocate a buffer for packets */
	if ((datahead = (char *)malloc(max_pkt_len+8)) == NULL) {
		TLI_DEBUG(1,"datasend: malloc failed\n");
		exit(1);
	}
	
	while (!stop) {
		range = max_pkt_len - min_pkt_len;
		datalen = (rand() % range) + min_pkt_len;
		if (SendData(newfd, datahead, datalen) < datalen) {
			if (!stop) {
				TLI_DEBUG(1,"datasend:ERROR:SendData failed\n");
				result = 1;
			}
			break;
		}
		TLI_DEBUG1(5,"datasend:INFO: sent packet len=%d\n",datalen);
		while (datalen) {
			if ((len= ReceiveData(newfd, datahead, max_pkt_len)) < 0) {
				if (!stop) {
					TLI_DEBUG(1,"datasend:ERROR: ReceiveData failed\n");
					stop = 1;
					result = 1;
				}
				break;
			}
			TLI_DEBUG1(5,"datasend:INFO: received packet len=%d\n",datalen);
			datalen -= len;
		}
	}
	alarm(0);	/* cancel the alarm */
	Release(newfd);
	while ( ReceiveData(newfd, datahead, max_pkt_len) > 0) 
		;
	free(datahead);
	exit(result);
}

void
datasend_cleanup()
{
}

static int sigalrm()
{
	TLI_DEBUG(2,"sigalrm:INFO: timeout occured\n");
	stop = 1;
}
