/* @(#)pushfilter.c	1.11 */
/* static char rcsid[]="$Id$" ;
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <locale.h>
#include <unistd.h>
#include <pfmt.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/mod.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <arpa/inet.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/strstat.h>
#include <netdb.h>
#include <sys/scodlpi.h>
#include <sys/dlpimod.h>

#include <filter.h> /* this includes <net/bpf.h> */

#define _PATH_DEV "/dev/"
#define _PATH_FILTER "/etc/pf.d/"
#define _SAP_FILE ".saps"
#define _NULL_TAG "NULL_FILTER"
#define PF_MODULE_NAME "pf"

int ifioctl(int fd,int cmd,char *arg)
{
	struct strioctl ioc ;
	ioc.ic_cmd=cmd ;
	ioc.ic_timout=0 ;
	ioc.ic_len=sizeof(struct ifreq) ;
	ioc.ic_dp=arg ;
	return(ioctl(fd,I_STR,(char *)&ioc)) ;
}

int ifnetmask(char *ifname,ulong *netmsk)
{
#ifdef __NEW_SOCKADDR__
	struct  sockaddr_in netmask = { 
		sizeof(struct sockaddr_in), AF_INET 	};
#else
	struct  sockaddr_in netmask = {
		AF_INET	};
#endif

	struct ifreq ifr ;
	char devname[32] ;
	int fd ;

	strcpy(devname,_PATH_DEV) ;
	strcat(devname,ifname) ;

	if ((fd=open("/dev/ip",O_RDWR)) < 0) {
		pfmt(stderr,MM_ERROR,":1:Cannot open file: /dev/ip %s\n",strerror(errno)) ;
		return(-1) ;
	}
	bzero(ifr.ifr_name, sizeof(ifr.ifr_name));
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ifioctl(fd, SIOCGIFNETMASK, (caddr_t) &ifr) < 0) {
		if (errno!=EADDRNOTAVAIL)
			pfmt(stderr,MM_ERROR,":2:Error in SIOCGIFNETMASK ioctl\n") ;
		memset((char *) &ifr.ifr_addr, '\0',
		    sizeof(ifr.ifr_addr));
	}
	else
		netmask.sin_addr=((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr ;
	*netmsk=ntohl(netmask.sin_addr.s_addr) ;
	close(fd) ;
	return(0) ;
}


int
dlpi_push_filter(char *ifname,struct bpf_program *bpf,
int the_sap,int the_protid,int dir)
{
	dl_setfilter_req_t *req ;
	int fd ;
	int flags ;
	struct strioctl sf ;
	char *buffer ;
	char devname[32] ;
	struct bpf_insn *insn=bpf->bf_insns ;
	int i,n=bpf->bf_len ;
	int mall_len=DL_FILTER_REQ_SIZE +n * sizeof(struct bpf_insn) ;

	buffer=(char *)malloc(mall_len) ;

	if(!buffer) {
		pfmt(stderr,MM_ERROR,":3:Cannot malloc for filter\n") ;
		return(-1) ;
	}

	strcpy(devname,_PATH_DEV) ;
	strcat(devname,ifname) ;
	if ((fd=open(devname,O_RDWR)) < 0) {
		free(buffer) ;
		pfmt(stderr,MM_ERROR,":4:Cannot open file %s: %s\n",devname,strerror(errno)) ;
		return(-1) ;
	}

	bzero(buffer,mall_len) ;

	bcopy(bpf->bf_insns,buffer+sizeof(dl_setfilter_req_t),
	    mall_len-DL_FILTER_REQ_SIZE) ;

	req = (dl_setfilter_req_t *)buffer ;
	req->dl_primitive = DL_FILTER_REQ ;

	req->sap = the_sap ;
	req->sap_protid = the_protid ;
	req->direction = dir ;

	sf.ic_cmd = DLPI_SETFILTER ; 
	sf.ic_timout = 0 ;
	sf.ic_dp = buffer ;
	sf.ic_len = mall_len ;

	if (ioctl(fd,I_STR,&sf) < 0) {
		close(fd) ;
		free(buffer) ;
		return(-1) ;
	}

	free(buffer) ;
	close(fd) ;
	return(0) ;
}

usage(name)
char *name;
{
	pfmt(stderr, MM_NOSTD,":29:Usage: %s [-piIOud]\n", name);
	pfmt(stderr, MM_NOSTD, ":30:\tp - protocol\n");
	pfmt(stderr,  MM_NOSTD,":31:\ti - interface\n");

	pfmt(stderr,  MM_NOSTD,":32:\tI - filter tag for incoming\n");
	pfmt(stderr,  MM_NOSTD,":33:\tO - filter tag for outgoing\n");
	pfmt(stderr,  MM_NOSTD,":34:\tu - unload filter(\"in\",\"out\" or \"both\")\n");
	pfmt(stderr,  MM_NOSTD,":35:\td - print BPF instructions\n");
	exit(1) ;
}

main(int argc, char **argv)
{
	ulong netm ;
	extern char *optarg ;
	extern int optind ;
	int iflag=0,oflag=0,uflag=0,intflag=0,dflag=0 ;

	FILE *fp,*fp2 ;
	char c ;
	char t1[64],filename[128],file2[128],sapfile[128] ;
	char line[80] ;
	char *i_tag=NULL,
	*o_tag=NULL,
	*protocol=NULL,
	*interface=NULL,
	*unload=NULL,
	*sapbuf ;
	char *tagbuf,*tmp ;
	char *t_in=_NULL_TAG,*t_out=_NULL_TAG ;
	int sap,protid=0 ;
	int i_err=0,o_err=0 ;
	int pin=0,pout=0 ;
	int uin=0,uout=0 ;

	struct filter ff1,ff2 ;
	struct bpf_program bpf1,bpf2 ;
	struct stat stbuf ;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxpushfilter");
	(void)setlabel("UX:pushfilter");
/*
	if (getuid() != 0) { 
		pfmt(stderr, MM_ERROR,":5:Must be root to run %s\n",argv[0]);
		exit(1);
	}
*/
	if (argc==1) {
		usage(argv[0]) ;
		exit(1) ;
	}
	while ((c = getopt(argc, argv, "p:i:I:O:u:d")) != -1) {
		switch (c) {
		case 'p':
			protocol=optarg ;
			break;
		case 'i':
			intflag++ ;
			interface=optarg ;
			break;
		case 'I':
			iflag++ ;
			i_tag=optarg ;
			break;
		case 'O':
			oflag++ ;
			o_tag=optarg ;
			break;
		case 'u':
			uflag++ ;
			unload=optarg ;
			break ;
		case 'd':
			dflag++ ;
			break ;
		case '?':
		default:
			usage(argv[0]) ;
			break ;
		} /* switch */
	} /* while */

	if ( !protocol)
		protocol="IP" ;

	if ( strcmp(protocol,"IP") ) {
		pfmt(stderr,MM_ERROR,":6:Only IP filtering is supported.\n") ;
		exit(1) ;
	}

	if ( !iflag && !oflag && !uflag ) {
		pfmt(stderr,MM_ERROR,":7:No tags specified.\n") ;
		exit(1) ;
	}

	if ( (iflag>1) || (oflag>1) ) {
		pfmt(stderr,MM_ERROR,":8:Multiple filter tags specified.\n") ;
		exit(1) ;
	}

	if (intflag>1) {
		pfmt(stderr,MM_ERROR,":9:Multiple interface names specified.\n") ;
		exit(1) ;
	}

	if (!intflag) {
		pfmt(stderr,MM_ERROR,":10:No interface name specified.\n") ;
		exit(1) ;
	}

	if (uflag>1) {
		pfmt(stderr,MM_ERROR,":11:Multiple unload filter primitives.\n") ;
		exit(1) ;
	}

	if ( (iflag || oflag) && uflag) {
		pfmt(stderr,MM_ERROR,":12:Load and unload primitives are incompatible.\n") ;
		exit(1) ;
	}

	if (uflag && strcmp(unload,"in") && strcmp(unload,"out") && strcmp(unload,"both")) {
		pfmt(stderr,MM_ERROR,":13:Unload primitive takes in,out or both as an argument.\n") ;
		exit(1) ;
	}

	strcpy(t1,_PATH_FILTER) ;
	strcat(t1,protocol) ;
	strcat(t1,"/") ;

	strcpy(filename,t1) ;
	strcat(filename,interface) ;
	strcpy(file2,t1) ;

	strcat(file2,".") ;
	strcat(file2,interface) ;

	strcpy(sapfile,t1);
	strcat(sapfile,_SAP_FILE) ;

	if((fp=fopen(sapfile,"r"))==NULL) {
		pfmt(stderr,MM_ERROR,":14:Cannot open file %s for reading SAPS:%s\n",sapfile,strerror(errno)) ;
		exit(1) ;
	}

	if(ifnetmask(interface,&netm)) {
		pfmt(stderr,MM_ERROR,":15:Error in getting netmask.\n") ;
		exit(1) ;
	}

	if(modload(PF_MODULE_NAME)<0) {
		pfmt(stderr,MM_ERROR,":16:Error in loading the packet filter module.\n") ;
		pfmt(stderr,MM_ERROR,":17:modload:%s\n",strerror(errno)) ;
		exit(1) ;
	}


	if(*o_tag) {
		if(!getfilter(&ff1,filename,o_tag))
			o_err=compilefilter(&bpf1,protocol,&ff1,DLT_RAW,netm) ;
		else
			o_err=1 ;
	}

	if(*i_tag) {
		if(!getfilter(&ff2,filename,i_tag))
			i_err=compilefilter(&bpf2,protocol,&ff2,DLT_RAW,netm) ;
		else
			i_err=1 ;
	}



	if(dflag)
	{
		struct bpf_insn *insn=bpf1.bf_insns ;
		int i,n=bpf1.bf_len ;

		if(!o_err&&oflag)
		{
			pfmt(stdout,MM_NOSTD,":18:Internal filtering code for outgoing\n");
			if(n)
			{
				for (i=0; i< n; ++insn, ++i)
					fprintf(stdout,"%s\n",bpf_image(insn,i)) ;
			}
			else
				fprintf(stdout,"NULL\n") ;
		}


		if(!i_err&&iflag)
		{
			insn=bpf2.bf_insns ;
			n=bpf2.bf_len ;

			pfmt(stdout,MM_NOSTD,":19:Internal filtering code for incoming\n");
			if(n)
			{
				for (i=0; i< n; ++insn, ++i)
					fprintf(stdout,"%s\n",bpf_image(insn,i)) ;
			}
			else
				fprintf(stdout,"NULL\n\n") ;
		}

	}

	while(sapbuf=fgets(line,80,fp))
	{
		char *scan_sap ;

		scan_sap=(char *)strtok(sapbuf," \t\n") ;
		if (*scan_sap=='#')
			continue ;

		sap=stoi(scan_sap) ;
		scan_sap=(char *)strtok(NULL," \t\n") ;


		if ((scan_sap) && (*scan_sap!= '#'))
			protid=stoi(scan_sap) ;
		else
			protid=0 ;

		if((!i_err&&iflag) || (!o_err&&oflag))
		{
			pfmt(stdout,MM_NOSTD,":20:Trying SAP 0x%x ",sap) ;
			if (protid)
				fprintf(stdout,"protid 0x%x\n",protid) ;
			else
				fprintf(stdout,"\n") ;
		}

		if(!o_err && oflag)
		{
			pfmt(stdout,MM_NOSTD,":21:Pushing outbound filter...") ;
			if (dlpi_push_filter(interface,&bpf1,sap,protid,DL_FILTER_OUTGOING)) /* outbound */
				pfmt(stdout,MM_NOSTD,":22:Error pushing filter.\n") ;
			else
			{
			 pout++ ; /* another one pushed */
			 pfmt(stdout,MM_NOSTD,":23:Done for outbound.\n") ;
			}
		}

		if(!i_err && iflag)
		{
			pfmt(stdout,MM_NOSTD,":24:Pushing incoming filter...") ;
			if (dlpi_push_filter(interface,&bpf2,sap,protid,DL_FILTER_INCOMING)) /* incoming */
				pfmt(stdout,MM_NOSTD,":22:Error pushing filter.\n") ;
			else
			{
			 pin++ ; /* another one pushed */
			 pfmt(stdout,MM_NOSTD,":25:Done for incoming.\n") ;
			}
		}

		if(uflag)
		{

			if(!strcmp(unload,"in"))
			{
				if (dlpi_push_filter(interface,NULL,sap,protid,DL_FILTER_INCOMING)) /* incoming */
					pfmt(stdout,MM_NOSTD,":26:Error unloading incoming filter.\n") ;
				else
					uin++ ;
			}
			else
				if(!strcmp(unload,"out"))
				{
					if (dlpi_push_filter(interface,NULL,sap,protid,DL_FILTER_OUTGOING)) /* outbound */
						pfmt(stdout,MM_NOSTD,":27:Error unloading outgoing filter.\n") ;
				else
					uout++ ;
				}
				else
				{
					if (dlpi_push_filter(interface,NULL,sap,protid,DL_FILTER_INCOMING)) /* incoming */
						pfmt(stdout,MM_NOSTD,":26:Error unloading incoming filter.\n") ;
					else
						uin++ ;
					if (dlpi_push_filter(interface,NULL,sap,protid,DL_FILTER_OUTGOING)) /* outbound */
						pfmt(stdout,MM_NOSTD,":27:Error unloading outgoing filter.\n") ;
				else
					uout++ ;
				}
		} /* if(uflag) */
	} /* while */

fclose(fp) ;

if(pin || pout || uin || uout) /* at least one pushed or unloaded */
	{
	 /* write to the state file */
	if(stat(file2, &stbuf))
		{
			fp2=fopen(file2,"w") ;
			chmod(file2,0600) ;
		}
	else
	{
		if((fp2=fopen(file2,"r"))==NULL)
		{
			pfmt(stderr,MM_ERROR,":28:Cannot open file %s:%s\n",file2,strerror(errno)) ;
			exit(1) ;
		}
		else
		{
			tagbuf=fgets(line,80,fp2) ;
			tmp=(char *)strtok(tagbuf," \t") ;
			t_in=malloc(sizeof(char)*strlen(tmp)) ;
			strcpy(t_in,tmp) ;
			tmp=(char *)strtok(NULL," \t\n") ;
			t_out=malloc(sizeof(char)*strlen(tmp)) ;
			strcpy(t_out,tmp) ;
		}
	}
	fclose(fp2) ; 

	fp2=fopen(file2,"w") ;

	if(uflag)
	{
		if(!strcmp(unload,"in"))
			{
			if (uin)
				fprintf(fp2,"%s\t%s\n",_NULL_TAG,t_out) ;
			else
				fprintf(fp2,"%s\t%s\n",t_in,t_out) ;
			}
		else
		{
			if(!strcmp(unload,"out"))
				{
				if(uout)
				fprintf(fp2,"%s\t%s\n",t_in,_NULL_TAG) ;
				else
				fprintf(fp2,"%s\t%s\n",t_in,t_out) ;
				}
			else
				{
				if(uin&&uout)
				fprintf(fp2,"%s\t%s\n",_NULL_TAG,_NULL_TAG) ;
				else
				fprintf(fp2,"%s\t%s\n",t_in,t_out) ;
				}
		}
	}
	else
	{
	if(pin&&(*i_tag))
		fprintf(fp2,"%s",i_tag) ;
	else
	{
		if(*t_in)
			fprintf(fp2,"%s",t_in) ;
		else
			fprintf(fp2,"%s",_NULL_TAG) ;
	}

	if(pout&&(*o_tag))
		fprintf(fp2,"\t%s\n",o_tag) ;
	else
	{
		if(*t_out)
			fprintf(fp2,"\t%s\n",t_out) ;
		else
			fprintf(fp2,"\t%s\n",_NULL_TAG) ;
	}
	} /* uflag */

	free(t_in) ;
	free(t_out) ;
	fclose(fp2) ;

	return(0) ;
	} /* success */
else
	return(1) ;
}
