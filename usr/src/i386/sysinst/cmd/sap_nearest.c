#ident	"@(#)sap_nearest.c	15.1"
/*
 *  This code uses SAP API to find up to 20 servers of a specified
 *  type, printing them all out in /var/spool/sap/in/* format,
 *  with the nearest server printed first.
 */

#include <sys/sap_app.h>
#include <stdio.h>

main(int argc, char *argv[])
{
	int	found;
	uint16  Socket, ServerType;
        SAPI    ServerBuf[21];
	int	ServerEntry=0;
	int	max_entries=20;
	int	i, count;

	if (sscanf(argv[1], "%x", &ServerType) == EOF) {
		fprintf(stderr, "Usage: %s service_number\n", argv[0]);
		exit(1);
	}

	found = SAPGetNearestServer(ServerType,ServerBuf);
	/*
	 *  If we got back a response, print it our in the format of
	 *  a SAP database entry.
	 */
	if (found > 0) {
		count=0;
		printf("%s\t", ServerBuf[count].serverName);
		for (i=0; i<IPX_NET_SIZE; i++)
			printf("%.2x", ServerBuf[count].serverAddress.net[i]);
		printf(".");
		for (i=0; i<IPX_NODE_SIZE; i++)
			printf("%.2x", ServerBuf[count].serverAddress.node[i]);
		printf(".");
		printf("0000\t%d\t", ServerBuf[count].serverHops);
		printf("%d\n", ServerBuf[count].serverType);
	} else
		exit(1);

	found = SAPGetAllServers(ServerType,&ServerEntry,&(ServerBuf[1]),max_entries);

	/*
	 *  If we got back a response, print it our in the format of
	 *  a SAP database entry.
	 */
	if (found > 0) {
		for (count=1; count<(found+1); count++) {
			/*
			 *  If this server is the one we found to be Nearest,
			 *  skip it.
			 */
			if (strcmp((char *)(ServerBuf[count].serverName),
				(char *)(ServerBuf[0].serverName))) {
			printf("%s\t", ServerBuf[count].serverName);
			for (i=0; i<IPX_NET_SIZE; i++)
				printf("%.2x", ServerBuf[count].serverAddress.net[i]);
			printf(".");
			for (i=0; i<IPX_NODE_SIZE; i++)
				printf("%.2x", ServerBuf[count].serverAddress.node[i]);
			printf(".");
			printf("0000\t%d\t", ServerBuf[count].serverHops);
			printf("%d\n", ServerBuf[count].serverType);
			}
		}
		exit(0);
	}

	/*
	 *  SAPGetNearestServer returns 1 on success and <0 on failure
	 *  so subtract 1, and if the return value of this program is 0
	 *  it succeeded.
	 */
	exit(1);
}
