/*
 * tc_xtra.c : DLPI test net source.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <stropts.h>
#include <nlist.h>
#include <sys/types.h> 
#include <sys/ipc.h>
#include <sys/dlpi.h>
#include <sys/dlpi_ether.h>
#include <errno.h>
#include <dl.h>
#include <tc_res.h>
#include <tc_net.h>
#include <tc_msg.h>
#include <tet_api.h>
#include <config.h>
#include <tc_frame_msg.h>

int   dl_debug = 0;

void *
getshm(shmkey, shmsz)
{
	int 	shmid;
	void	*ptr;

	shmid = shmget(shmkey, shmsz, IPC_CREAT|0600);
	if(shmid < 0) {
		perror("getshm: shmget failed");
		printf("key = %x size = %x\n",
				shmkey,
				shmsz);
		exit(1);
	}

	ptr = (void *)shmat(shmid, 0, 0);
	if(ptr == (void *)-1) {
		perror("shmat failed");
		exit(1);
	}
	return(ptr);
}	

getpanicaddr(addrp)
long 	*addrp;
{
	int		ret;
	struct	nlist	nl[2];

	nl[0].n_name = "open";
	nl[1].n_name = NULL;

	ret = nlist("/stand/unix", nl);
	*addrp = nl[0].n_value;

	return(ret);	
}

/*
 * get the node count in the test net..
 */
int
getnodecnt()
{
	char *sptr;

	if ((sptr = tet_getvar("NODECOUNT")) == NULL) {
		tet_infoline(NODECOUNT_FAIL);
		return(-1);
	}	
	return atoi(sptr);
}

#define MAX_LINE 256
#define TAB	9
#define BLANK	32

int
getlocalname(host)
char	host[];  
{
	FILE	*fp;
	char	filename[128];
	char	*tetroot;
	char	*tetsuit;

	tetroot = getenv("TET_SUITE_ROOT");
	tetsuit = getenv("SUITE");
	
	sprintf(filename,"%s/%s/dlpi_nodename", tetroot, tetsuit);
	if (!(fp =  fopen(filename, "r"))) {
		DL_DEBUG1(1,"Error: %s File Open Failure\n",filename);
		return(-1);
	} 

	if ( !fgets(host, MAX_LINE, fp)) {
		DL_DEBUG1(1,"Error: %s file corrupted\n", filename);
		return(-1);
	}
	return(0);
}

uppercase(str)
register char *str;
{
	while(*str) {
		if( (*str >= 'a') && (*str <= 'z'))
			*str -= 'a' - 'A';
		str++;
	}
}

char hexnums[] = "0123456789abcdef";
hex(ch)
register unsigned char ch;
{
	register i;

	ch = tolower(ch);
	for(i=0;i<16;i++)
		if(hexnums[i] == ch)
			return(i);
	return(-1);
}

getaddr(node,eaddr,naddr)
char	*node;
unsigned char	eaddr[];
unsigned char	naddr[];
{
	FILE	*fp;
	char	line[MAX_LINE];
	char	name[128];
	char	filename[128];
	char	hostname[128];
	char	devname[128];
	char	macaddr[19];
	char	*paddr;
	register i;
	int	found;
	char	*tetroot;
	char	*tetsuit;

	tetroot = getenv("TET_SUITE_ROOT");
	tetsuit = getenv("SUITE");
	
	strcpy(name, node);

	sprintf(filename,"%s/%s/dlpi_config", tetroot, tetsuit);
	if (!(fp =  fopen(filename, "r"))) {
		DL_DEBUG1(1,"getaddr: ERROR: %s File Open Failure\n",filename);
		return(-1);
	} 

	found = 0;
	while (fgets(line, MAX_LINE, fp)) {
		DL_DEBUG1(2,"getaddr: INFO: line = %s\n", line);
		sscanf(line,"%s%s%s",hostname,devname,macaddr);
		if (strcmp(hostname, name) == 0) {
			found = 1;
			break;
		}
	}	/* while */

	if (found == 0) {
		DL_DEBUG1(1,"getaddr: ERROR: No addr found for: %s\n", name);
		return(-1);
	}
	paddr = macaddr;
	for (i = 0;i < LLC_ADDR_LEN;i++) {
		eaddr[i] = hex(*paddr++) << 4;
		eaddr[i] += hex(*paddr++);
		paddr++;	/* skip '.' */
	}
	return(0);
}

char	netdev[128];

getdevname()
{
	FILE	*fp;
	char	line[MAX_LINE];
	char	name[128];
	char	filename[128];
	char	hostname[128];
	char	macaddr[19];
	int	found;
	char	*tetroot;
	char	*tetsuit;
	char *sptr;

	/* 
	 *This routine is called by almost every test and is good place to
	 * trun on debugging
	 */
	if ((sptr = tet_getvar("DL_DEBUG")) != NULL) 
		dl_debug = atoi(sptr);

	if (getlocalname(name) < 0)
		return(-1);

	tetroot = getenv("TET_SUITE_ROOT");
	tetsuit = getenv("SUITE");
	
	sprintf(filename,"%s/%s/dlpi_config", tetroot, tetsuit);
	if (!(fp =  fopen(filename, "r"))) {
		DL_DEBUG1(1,"getdevname: ERROR: %s File Open Failure\n",filename);
		return(-1);
	} 

	found = 0;
	while (fgets(line, MAX_LINE, fp)) {
		DL_DEBUG1(2,"getdevname: INFO: line = %s\n", line);
		sscanf(line,"%s%s%s",hostname,netdev,macaddr);
		if (strcmp(hostname, name) == 0) {
			found = 1;
			break;
		}
	}	/* while */

	if (found == 0) {
		DL_DEBUG1(1,"getdevname: ERROR: No device name found for: %s\n", name);
		return(-1);
	}
	return(0);
}

void
tet_msg(char *format, ...)
{
	char	buffer[1024];
	va_list	ap;

	va_start(ap, format);
	vsprintf(buffer, format, ap);

	tet_infoline(buffer);
}
