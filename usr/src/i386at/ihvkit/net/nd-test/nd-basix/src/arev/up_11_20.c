#include <sys/types.h>
#include "sqldata.h"
#include <sys/utsname.h>
#include <netconfig.h>
#include <netdir.h>
#include <string.h>
#include <tiuser.h>
#include <ctype.h>
#include <stdio.h>
#include <dirent.h>
#ifdef VER20
#include <nw/nwerror.h>
#include <nw/nwcaldef.h>
#include <nw/nwserver.h>
#include <nw/nwbindry.h>
#include <nw/nwconnec.h>
#include <nw/nwmisc.h>
#include <nw/nwclient.h>
#define NCP_SERVICE_TYPE "NCP_SERVER"
#endif
#ifdef VER11
#include <nwapi.h>
#include <nwerrors.h>
#endif
#include <sys/fcntl.h>
#include <sys/byteorder.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/signal.h>
#include <setjmp.h>
#include <sys/errno.h>
#include <sys/stream.h>
#ifdef  VER11
#include <sys/as_ipx_app.h>
#endif
#include <sys/spx_app.h>

#define MAX_SERVERS 1000
#define MAX_CONTR   10

void read_config();
char *description();
char *getvar();

extern unsigned int	errno;
#ifdef VER11
extern unsigned int	t_errno;
#endif

char	servername[100];
int	no_servers = 0;
struct Link   *prd_list , *prd , *get_product_list();
int no_products = 0;
struct Link *Root = 0;
struct Link *List;
int	netdevfd = 0;
int failnum;
int signature;
int remark_number;
time_t strttime , etime;
int	Etime;
unsigned char	*vardata ;
unsigned char	**varptrs ;
typedef struct  {
	char Name[80];
	long IO;
	INT IRQ;
	long Ram;
	long Rom;
	INT DMA;
	char Driver_Name[16];
	char Driver_Date[20];
	long Driver_Size;
}CONTRLR ;

CONTRLR	controller[10];
#ifdef VER20
NWCONN_HANDLE serverConnID;
NWCONN_HANDLE ns_ch;
#endif

#ifdef VER11
int	serverConnID;
#endif
char	spxaddress[12];
char	Stat_Cntlr;
int	address_available = FALSE;
int	UpdateIndex = 0;
int 	db_no;
int 	pass, fail, unsupported, unresolved, notinuse;
int	untested, uninitiated, noresult;

main(argc, argv)
int	argc;
char	**argv;
{
	int	ret, server_index;
	int	ret1;
	char	syscmd[100];
	char	cmd[512];
	char	statcntlr[10];

	if (argc < 4)
	{
		printf("Usage: %s <servername> <suitename> <DataBaseID> \n\n", argv[0]);
		exit(1);
	}
	strcpy(servername, argv[1]);
	strcpy(statcntlr, argv[4]);
	Stat_Cntlr = statcntlr[0];
	/* sprintf(syscmd, "nwlogout %s", argv[1]);
	system(syscmd);
	sprintf(syscmd, "nwlogin %s/supervisor", argv[1]);
	system(syscmd);  */

	server_index = 0;
	db_no = atoi(argv[3]);
	ret1=updateTR(argv[2]); 
	cmd[0]=0;
	if( ret != -1)
	{
	  if( strcmp(argv[2],"cdrom") == 0 || strcmp(argv[2],"tape") == 0 ||
		strcmp(argv[2],"hd") == 0 )
			(void) sprintf(cmd,"echo %s 3 YES %c >> $TET_SUITE_ROOT/common/bin/Test_urg",argv[2], Stat_Cntlr);
	   else
			(void) sprintf(cmd,"echo %s 3 YES >> $TET_SUITE_ROOT/common/bin/Test_urg",argv[2]);
	}
	else
	{
	  if( strcmp(argv[2],"cdrom") == 0 || strcmp(argv[2],"tape") == 0 ||
		strcmp(argv[2],"hd") == 0 )
			(void) sprintf(cmd,"echo %s 3 NO %c >> $TET_SUITE_ROOT/common/bin/Test_urg",argv[2], Stat_Cntlr);
	   else
			(void) sprintf(cmd,"echo %s 3 NO >> $TET_SUITE_ROOT/common/bin/Test_urg",argv[2]);
	}
	system(cmd);
	close_netware();
	exit(0);
}

int
open_netware()
{
	int	i, ret_val;

	if ( open_connection() == -1) {
		printf("Error in opening Netware Connection\n");
		close_connection(netdevfd);
		return(-1);
	};
	return(0);
}

lowercase(str)
register char	*str;
{
	while (*str) {
		if ( (*str >= 'A') && (*str <= 'Z'))
			*str += 'a' - 'A';
		str++;
	}
}

uppercase(str)
register char	*str;
{
	while (*str) {
		if ( (*str >= 'a') && (*str <= 'z'))
			*str -= 'a' - 'A';
		str++;
	}
}

/*
 * 	open_connection - open a connection with the database service
 *			facility in the Netware server. The service
 *			name is taken from the file bcastnld.dat 
 *			in the SYS/public directory of netware.
 *
 * 	output		the file descriptor for the connection in the pointer
 *			given
 *
 *	assumes		that nwlogin has already been done 
 *
 *	errors		Returns -1 for any failure
 *			0 for success
 */

open_connection()
{
	int	i, fd;
	int	ret_val;
	char	spxaddress[12];
	struct t_call *call_req, *call_ret;
	struct spxopt_s spx_options;
#ifdef  VER20
	NWCONN_HANDLE	ConnHandle,STcon;
	CONNECT_INFO	ConnInfo;
	NWCCODE		ccode;
	nint8		*ptr;
	pnstr8		server[50], objname[50], password[128];
        pnstr  Stype;
        nuint  ConFlag, Txtype;
	NWCConnString  Name;
#endif

#ifdef VER20
	ccode = NWCallsInit(0, 0);
	if (ccode)
	{
		printf("NWCallsInit failed !!\n");
		exit(1);
	}


        Name.pString = servername;
	Name.uStringType=NWC_STRING_TYPE_ASCII;
	Name.uNameFormatType=NWC_NAME_FORMAT_BIND;

        ConFlag = NWC_OPEN_UNLICENSED | NWC_OPEN_PUBLIC;
	Txtype = NWC_TRAN_TYPE_IPX;

      ccode = NWOpenConnByName( NULL, 
				&Name,
				(pnstr)NCP_SERVICE_TYPE,
			        ConFlag,
			        Txtype,
			        &ns_ch
			      );
     if (ccode)
      {
	printf("NWOpenConnByName failed with %x\n",ccode);
	exit(1);
      }

	ccode = NWLoginToFileServer( ns_ch,
				     (pnstr8)"SUPERVISOR",
				     OT_USER,
				     NULL
				   );
        if (ccode) {
		   printf("NWLoginToFileServer Failed with %x\n",ccode);
		   exit(1);
	}


#endif

#ifdef VER11
	ret_val = NWAttachToServerPlatform(servername, &serverConnID);
	if ( ret_val != 0 ) {
		if (NWErrno == NWERR_CONN_ALREADY_ATTACHED_TO_SERVER) {
			fprintf(stderr,"Already connected to the Server\n");
		}
		else {
			printf("NWAttachToServerPlatform error :NWError = %x \n",
							    NWErrno);
			return(-1);
		}
	}
#endif

	if (get_server_addr(servername, serverConnID, spxaddress) ==
	    -1) {
		printf("Error in getting object address in server \n");
		return(-1);
	}
/*	printf("Connect Address is :\t"); 
	i = 0;
	while (i < 12 ) {
		printf("%x:", spxaddress[i++] & 0xff);
	}
	printf("\n");
*/
/*	printf("size of spx address = %d\n", sizeof(spxaddress) ); */
/*#endif  */

	if (( fd = t_open("/dev/nspx", O_RDWR, NULL)) == -1) {
		printf("Error in opening /dev/nspx . Errno %x\n",
		     		     errno);
		return(-1);
	}
	netdevfd = fd;
	if ( t_bind(fd, NULL, NULL) == -1) {
		printf("Error in binding . Errno %x\n", t_errno);
		return(-1);
	}

	call_req = (struct t_call *)t_alloc(fd, T_CALL, T_ALL);
	if (call_req == NULL) {
		printf("t_alloc fails errno = %d t_errno = %d\n",
		     		     		    errno, t_errno);
		return(-1);
	}
	call_ret = (struct t_call *)t_alloc(fd, T_CALL, T_ALL);
	if (call_ret == NULL) {
		printf("t_alloc fails errno = %d t_errno = %d\n",
		     		     		    errno, t_errno);
		return(-1);
	}

	call_req->addr.buf = (char *) & spxaddress;
	call_req->addr.maxlen = sizeof(spxaddress);
	call_req->addr.len = sizeof(spxaddress);

	spx_options.spx_connectionID[0] = 0;
	spx_options.spx_connectionID[1] = 0;
	spx_options.spx_allocationNumber[0] = 0;
	spx_options.spx_allocationNumber[1] = 0;

	call_req->opt.buf = (char *) & spx_options;
	call_req->opt.len = sizeof(spx_options);
	call_req->opt.maxlen = sizeof(spx_options);

	call_req->udata.buf = (char *)0;
	call_req->udata.len = 0;
	call_req->udata.maxlen = 0;

	if (t_connect(fd, call_req, call_ret) < 0) {
		printf("t_connect fails errno = %d t_errno = %d\n",
		     		     		    errno, t_errno);
		t_error("T_connect fails ");
		printf("State of the end point = %d\n", t_look(fd) );
		return(-1);
	}
	return(0);
}

/*
 *	close_connection	- Closes the connection
 *
 *
 *	inputs			the file descriptor	
 * 	output			None
 *
 */
close_connection(fd)
int	fd;
{
	t_snddis(fd, (struct t_call *)0);
	t_close(fd);
}

get_server_addr( char *sname, int serverID, char *server_addr )
{
	int	i;
	int	sequence;
	int	ret_val;
	int	fd;
	FILE * stream;

#ifdef VER20
	char	servicename[48];
	nuint16   OBjType;
	nuint32   OBjectID;
	NWCCODE	  ccode;
	nuint8	  PropFlag;
	nuint8	  OBjFlag;
	nuint8	  OBjSecurity;
	nuint8	  OBjname[48];

        char Data[128];
        nuint8 MoreFlag;

	uint16 servicetype;
	uint8	segmentdata[42];
	nuint8	segmentno;
/****************************************/            

	NWCONN_HANDLE conn;
	NWCONN_HANDLE conn1;
	NWDIR_HANDLE ndirh;
	NWFILE_HANDLE nfileh;
	pnstr8   filepath[256];

	/*nuint16   ObjType;
	nuint32   Objid;
	NWCCODE	  ccode;
	nuint8	  hasprops;
	nuint8	  ObjFlag;
	nuint8	  Objsec;
	nuint8	  segno;
	nuint8	  mflag;
	nuint8	  propflag;
	nuint8	  ObjName[48];  */

        nuint32 bytes2r,fsize;
        nuint32 bytesR;
        char info[50];
	char Target[60];
	char serviceN[48],tmp[5];
	char NetAdd[128];
        pnuint8 fh;
	ushort serviceT;

/****************************************/            
#endif
#ifdef VER11
	uint16 	servicetype;
	char	servicename[NWMAX_OBJECT_NAME_LENGTH];
	uint8	segmentdata[NWMAX_SEGMENT_DATA_LENGTH];
	NWObjectInfo_t objects;
	NWPropertyInfo_t property;
	uint8 moreflag, propertytype;
	int	segmentno;
#endif
	char	bcastfile[256];

#ifdef VER20
	sprintf (bcastfile, "%s\\sys:\\public\\bcastnld.dat",
	     	     servername);

/*****************************************************************/


	ccode = NWParseNetWarePathConnRef(bcastfile,&conn1, &ndirh, filepath
	        );

	if (ccode) {
		    printf("NWParse Failed :%x\n",ccode);
		    exit(1);
	           }
/*         printf("Connected with handle %d on path %s :directory handle %d\n",conn1,filepath,ndirh);
	printf("filepath : %s \n", filepath); */


	ccode = NWOpenFile(ns_ch,
			   ndirh, 
			   filepath,
                           SA_NORMAL,
		           AR_READ,
                           &fh
                          ); 
/*          printf("File Handle %d for path %s \n",fh,filepath); */

	if (ccode) {
		    printf("NWOpen Failed: %x\n",ccode);
		    exit(1);
	           }


	ccode = NWGetEOF(fh, &fsize);
	if (ccode) {
		    printf("NWGetEOF Failed: %x\n",ccode);
		    exit(1);
	           }

/*	printf("FILE SIZE = %ld \n", fsize);  */

	ccode = NWReadFile(fh,fsize,&bytesR,info);
	if (ccode) {
		    printf("NWRead Failed : %x\n",ccode);
		    exit(1);
	           }
        /*printf("Read Succeeded\n"); 
	printf("%s :%d\n", info,strlen(info)); */

        for (i=0; i < strlen(info);i++)
	{
	 /* printf("%c[%d] = info[%d]\n",info[i],info[i],i );  */
	 
         if ( ( info[i] == 13 ) && (info[i+1] == 10) )
	      info[i] = info[i+1] = 32;

 	}

	/* printf("%s:%d\n", info,strlen(info)); */
        sscanf(info,"%s %s", servicename,tmp);
	servicetype = atoi(tmp);
	uppercase(servicename);
 /*       printf("ServiceNAME = %s,  TYPE = %d \n",servicename,servicetype);
 */

/*****************************************************************/

#endif
#ifdef VER11
	sprintf (bcastfile, "/.NetWare/%s.nws/sys.nwv/public/bcastnld.dat",
	     	     sname);
	if ((stream = fopen (bcastfile, "r")) == NULL) {
		perror("Error in opening bcastnld.dat file ");
		return(-1);
	}
	fscanf(stream, "%s", servicename);
	servicename[32] = NULL;
	uppercase(servicename);
	fscanf(stream, "%u", &servicetype);
	fclose(stream);
/*	printf("Service Name : %s , Service Type %x\n", servicename,
	     	     servicetype);
*/

#endif

#ifdef VER20
	OBjectID = -1;
	ccode = 0;
	ccode = NWScanObject(ns_ch, (pnstr8)servicename, NSwap16(servicetype), 
				     &OBjectID, (pnstr8)OBjname,
		                     &OBjType, &PropFlag, &OBjFlag,
				     &OBjSecurity);

	if (ccode)
	{
		printf("NWScanObject failed !!\n");
		exit(1);
	}
#endif
#ifdef VER11
	sequence = -1;
	ret_val = NWScanObject(serverID, servicename, servicetype,
	     	     &sequence, &objects);

	if ( ret_val == 0 ) {
		printf("Scan object Failed: No object found NWError %x\n",
		     		     NWErrno);
		return(-1);
	}
#endif

/*
printf("After ScanObject - Objtype = %x, objflag = %d, objID = %08lx\n", OBjType, OBjFlag, OBjectID);
printf("After ScanObject - PropFlag = %x, OBjFlag = %x, OBjSecurity = %x\n", PropFlag, OBjFlag, OBjSecurity);
printf("After ScanObject - objname = %s\n", OBjname);
*/
	segmentno = 1;

#ifdef VER20
	ccode = NWReadPropertyValue(ns_ch,
				      (pnstr8)OBjname,
				      OBjType,
	                              (pnstr8)"NET_ADDRESS",
				      segmentno,
				      (pnuint8)Data,
				      &MoreFlag,
				      &PropFlag
                                     );
	if (ccode)
	{
		printf("NWReadPropertyValue failed !!\n");
		exit(1);
	}
	memcpy(server_addr, Data, 12);
#endif
#ifdef VER11
	ret_val = NWScanPropertyValue(serverID, objects.objectName,
	     	     	    objects.objectType, "NET_ADDRESS", &segmentno,
	     	     segmentdata,
	    &moreflag, &propertytype);
	if ( ret_val == 0 ) {
		printf("Scan Property value Failed: Error NWErrno %x \n",
		     		     NWErrno);
		return(-1);
	}
	memcpy(server_addr, segmentdata, 12);
#endif
	return(0);
}






Test_Struct test_pkt;
WORD session_no;
int test_no;

int
updateTR(suite_names)
char 	*suite_names;
{
	int dbid_idx;
	int ret;
	int i;
        FILE *rem_p;
        char Rmbuf[250];
	char rem[512];
	char cmd[512], resstat[512];
	char *tmp_buf;
	char remarks_file[50];
	char  syscall[100];

	if (getres(suite_names) == -1) {
		printf("Results file does not exist\n");
		return(-1);
	}
	if ( ( test_no = gettestid(suite_names)) == -1 ) {
		printf("Results for these tests cannot be updated\n");
		return(-1);
	}
/*
CHANGE 1 MADE BY HARI ON 29/1/95
	if ( test_no == 3009 ) {
		* 
		 * if the test selected is X tests
		 * then check tcp/ip setup with the
		 * display machine , before updating the
		 * tests. This is required , before doing
		 * anything else , as otherwise only the
		 * test results will be there without the
		 * adaptor details.
		 *
		ret = check_display_machine();
		if ( ret ) {
			switch (ret) {
			case 1:
				printf("The name of the machine used for display while executing the X tests should be present in the /etc/hosts file. Add the entry and update results again.\n");
				break;
			case 2:
				printf("This machine's node name needs to be added in the /etc/hosts.equiv file of the machine which was used for display while executing  the X tests. This entry does not exist. Please add the entry and update the results again.");
				break;
			case 3:
				printf( "The machine used for display during the X tests is not responding. Check the TCP/IP setup and update results again.");
				break;
			default:
				printf("talk to the machine used for display while executing the X tests  .");
				break;
			}
			return(-1);
		}
	}
END OF CHANGE 1
*/
	test_pkt.TestNumber = test_no;
/*	dbid_idx= UpdateIndex;
	prd = prd_list;
	while (dbid_idx >  0) {
		prd = prd ->next ;
		dbid_idx -- ;
	}
	db_no  = prd->DataBaseID ;
*/
	test_pkt.DataBaseID = prd->DataBaseID ;

/* printf("failnum = %d\n", failnum); */
	if(failnum == 0)
		test_pkt.Status = PASS;
	else
		test_pkt.Status = FAIL;

	test_pkt.StartTime = strttime;
	test_pkt.StopTime = etime;

	test_pkt.Iteration = 1;
	/*********************************************
        Expaned Test_Struct - members;
	*********************************************/
	test_pkt.ControlFlags = NO_FLAGS;
	test_pkt.Executions = 0;
	strncpy(test_pkt.Program,"UWCERT",6);

	if ( open_netware () != 0) {
		printf("Cannot access the Netware Server\n");
		return(-1);
	}
	for (i = 0; i < 512; i++)
        	rem[i] = '\0';

	cmd[0]=0;
	switch(test_pkt.Status)
	{
		case 0 :	strcpy(resstat,"PASS");
				break;

		case 2 :	strcpy(resstat,"FAIL");
				break;

		case 1 :	strcpy(resstat,"INCOMPLETE");
				break;
	}
	if( strcmp(suite_names,"cdrom") == 0 || strcmp(suite_names,"tape") == 0 ||
		strcmp(suite_names,"hd") == 0 )
			(void) sprintf(cmd,"echo %s 2 %s %c >> $TET_SUITE_ROOT/common/bin/Test_urg",suite_names, resstat, Stat_Cntlr);
	else
			(void) sprintf(cmd,"echo %s 2 %s >> $TET_SUITE_ROOT/common/bin/Test_urg",suite_names, resstat);
		
	system(cmd);

	if ( (tmp_buf = (char *)getenv("TET_SUITE_ROOT")) == (char *)NULL ) {
	 	printf("TET_SUITE_ROOT not set!\n");
	        return(-1);
	}
	strcpy(syscall, tmp_buf);
	tmp_buf = (char *)malloc(100);
	sprintf(tmp_buf, "/common/bin/addrem %d %s %d %d %d %d %d %d %d %d", 
			test_pkt.Status,
			suite_names,
			pass, fail, unsupported, unresolved, 
			notinuse, untested, uninitiated, noresult);
	strcat(syscall, tmp_buf);

	system(syscall);

	if ( (tmp_buf = (char *)getenv("TET_SUITE_ROOT")) == (char *)NULL ) {
	 	printf("TET_SUITE_ROOT not set!\n");
	        return(-1);
	}
	strcpy(remarks_file, tmp_buf);
	strcat(remarks_file, "/common/bin/REMARKS");
	rem_p = fopen(remarks_file, "r");
	if ( rem_p == NULL ) {
				printf("REMARKS not set!\n");
				 return(-1);
	                     
			     }
/*     	while (!feof(rem_p))
	{
	if ( fread(Rmbuf, sizeof(Rmbuf), 1, rem_p) == -1 )
	  	{
		   printf("fread failed\n");
		}
      
	 strcat(rem, Rmbuf);
		  			
	}
*/
	fgets(rem , sizeof(rem), rem_p);
	if ( ( session_no = add_test_result(&test_pkt, rem, suite_names) ) == -1 ) {
		printf( "Error in adding results");
		return(-1);
	}
	else
		printf("Results updated in the Netware Database\n");
	close_netware() ;
	
	return(0);
}

int
getres(suite_name)
char	*suite_name;
{
	FILE *pt;

	int shour, smin , ssec;
	int ehour , emin , esec;
	int yr , mon , day;
	struct tm  tmst;
	char buf[1024];
	char crnl[3];
	char *rem = NULL;
	struct stat sbuf;
        char *tmp_buf;
        char gres_file[50];
/* Carriage Return and Newline String */
	crnl[0] = 0x0d;
	crnl[1] = 0x0a;
	crnl[2] = 0;

	if ( (tmp_buf = (char *)getenv("TET_SUITE_ROOT")) == (char *)NULL ) {
	 	printf("TET_SUITE_ROOT not set!\n");
	        return(-1);
	}
	strcpy(gres_file, tmp_buf);
	strcat(gres_file, "/common/bin/getres");
	sprintf ( buf ,"%s -a %s > /tmp/res" , gres_file,suite_name);
	system(buf);

	if (stat ("/tmp/res" , &sbuf) != 0  || 
			sbuf.st_size == 0 )
		return (-1);
	if ( rem != NULL )
		free (rem );
	rem = (char *)malloc ( sbuf.st_size + 28 );

	sprintf(rem,"UnixWare Test Suite V 2.4 | "); 
	pt=fopen( "/tmp/res" , "r");

	fgets(rem + strlen(rem) ,sbuf.st_size,pt);
	strcat (rem , crnl );
	fgets((rem + strlen (rem) ),sbuf.st_size,pt);

	fscanf ( pt , "%d:%d:%d",&shour,&smin,&ssec );
/* printf(" %d %d %d\n", shour, smin, ssec); */
	fscanf ( pt , "%4d%2d%2d",&yr,&mon,&day );
/*printf("%d %d %d\n", yr, mon, day); */
	fscanf ( pt , "%d",&failnum );
/*printf("failnum = %d\n", failnum); */
	fscanf ( pt , "%d:%d:%d",&ehour,&emin,&esec );
	fscanf ( pt , "%d %d %d %d %d %d %d %d",&pass, &fail, &unsupported, 
			&unresolved, &notinuse, &untested, &uninitiated, 
			&noresult);

	fclose (pt);
/*	unlink ("/tmp/res"); */

	tmst.tm_sec = ssec ;
	tmst.tm_min = smin ;
	tmst.tm_hour = shour ;
	tmst.tm_mday = day ;
	tmst.tm_mon = mon -1 ;
	tmst.tm_year = yr -1900 ;
	tmst.tm_isdst = -1 ;

	strttime = mktime ( &tmst );

	if ( esec == 0 && emin == 0 && ehour  == 0 ) {
		etime = 0;
	}
	else {
		tmst.tm_sec = esec ;
		tmst.tm_min = emin ;
		tmst.tm_hour = ehour ;
		tmst.tm_mday = day ;
		tmst.tm_mon = mon -1 ;
		tmst.tm_year = yr -1900 ;
		tmst.tm_isdst = -1 ;
		etime = mktime ( &tmst );
	}
	return(0);
}

int
gettestid(suite_name)
char	*suite_name;
{
int testid;
char	*tetr;

	char cfg_file[256];
	FILE	*fp;
	char  buffer[200];
	char  name[100];
	char  desc[50];
	char  run_cmd[15];
	int  done = 0;
	char  id[10];
	char idfile[50];
	char *tmp_buf;

	if ( (tmp_buf = (char *)getenv("TET_SUITE_ROOT")) == (char *)NULL ) {
	 	printf("TET_SUITE_ROOT not set!\n");
	        return(-1);
	}
	strcpy(idfile, tmp_buf);
	strcat(idfile, "/common/bin/idfile");

	fp = fopen(idfile, "r");
	while(!done)
	{
		fgets(buffer, 200, fp);
		sscanf(buffer, "%s %s", name, id);
		if ( !strcmp(suite_name, name))
		{
			done = 1;
			testid = atoi(id);
		}
	}

	if ( strcmp ( suite_name , "tcp" ) == 0) {
		if ( ( tetr = (char *)getenv ("TET_SUITE_ROOT")) == NULL) {
			printf( "TET_SUITE_ROOT not set");
			return(-1);
		}
		sprintf(cfg_file,"%-s/%-s/tetexec.cfg",
			tetr, suite_name);
		read_config(cfg_file);
		if (strcmp((char *)getvar("PROTOCOL"),"SPX") == 0)
			testid = 3030;
		else
			testid = 3031;
	}

	return(testid);
}

void
read_config(file)
char	*file;
{
	static size_t nbyte;
	unsigned char	*cp;
	unsigned char	**varp;
	int	fd, err, cnt;
	struct stat statb;
	char	buf[256];

	/* open and read in entire config file */

	if (file == NULL || *file == '\0')
		return;

	if ((fd = open(file, O_RDONLY)) < 0) {
		err = errno;
		(void) sprintf(buf, "could not open config file \"%s\"",
		     		     		    file);
		printf(buf);
		return;
	}

	if (fstat(fd, &statb) == -1) {
		err = errno;
		(void) sprintf(buf, "fstat() failed on config file \"%s\"",
		     		     		    file);
		printf( buf);
		(void) close(fd);
		return;
	}

	if ((nbyte = statb.st_size) == 0) {
		(void) close(fd);
		return;
	}

	/* In case this routine has been called before, fre
	    	    e the old data */
	if (vardata != NULL)
		free((void *)vardata);

	vardata = (unsigned char *)malloc((size_t)(nbyte +
	    1));
	if (vardata == NULL) {
		printf( "malloc() failed");
		(void) close(fd);
		return;
	}

	if (read(fd, (void *)vardata, nbyte) != nbyte) {
		err = errno;
		(void) sprintf(buf, "read() failed on config file \"%s\"",
		     		     		    file);
		printf( buf);
		(void) close(fd);
		return;
	}
	vardata[nbyte] = '\0'; /* in case of incomplete las
	    	    t line */

	(void) close(fd);

	/* count number of variables and replace newlines w
	    	    ith nulls */

	cnt = 0;
	cp = &vardata[nbyte-1];
	if (*cp == '\n') {
		*cp = '\0';
		cnt++;
	}
	while (--cp > vardata) {
		if (*cp == '\n') {
			cnt++;
			*cp = '\0';
		}
	}
	if (*cp == '\n')
		*cp = '\0';
	cnt++; /* allow for null terminator */

	/* set up pointers to each "variable=value" string

		    */

	if (varptrs != NULL)
		free((void *)varptrs);

	varptrs = (unsigned char **)malloc((size_t) (cnt *
	    sizeof(char *)));
	if (varptrs == NULL) {
		printf( "malloc() failed");
		return;
	}

	varp = varptrs;
	while (cnt > 1) {
		*varp++ = cp;
		while (*cp++ != '\0');
		cnt--;
	}

	/* add terminating NULL pointer */

	*varp = NULL;
}

check_display_machine()
{
	char buf[256];
	char syscall[256];
	char mc_name[100];
	char machine[100];
	int i,ret,found;

	sprintf(machine, "%s", getvar ("XT_DISPLAY"));

	i=0;
	while( (i<100) && (machine[i] != ':') && (machine[i] != 0)) {
		mc_name[i] = machine[i];
		i++;
	}
	mc_name[i]=0;
	sprintf(syscall,"grep %s /etc/hosts|grep -v \"^[ 	]*#\" >/dev/null",mc_name);
	ret = system(syscall);
	if ((ret>>8) == 1) {
		printf("The machine name %s should be present in the /etc/hosts file. Add the entry and update results again.",mc_name);
		return(1);
	}
	

	sprintf(syscall,"echo %s uwcert >rhosts",mc_name);
	ret=system(syscall);

	sprintf(syscall,"rcp rhosts %s:/home/uwcert/.rhosts >/dev/null 2>&1",mc_name);
	ret = system(syscall);
	if (ret != 0) {
		printf("This machine's node name needs to be added in the /etc/hosts.equiv file of the machine %s. This entry does not exist. Please add the entry and continue.",mc_name);
		return(2);
	}

	sprintf(syscall,"rsh -n -l uwcert %s \"cat /etc/conf/sdevice.d/*\"  >/tmp/scantemp1 2>/dev/null",mc_name);
	ret = system(syscall);
	if (ret != 0) {
		printf("The machine %s is not responding. Check the TCP/IP setup and update results again.\n",mc_name);
		return(3);
	}
/*	strcpy(syscall,"rm -rf /tmp/scantemp1 ");   */
	system(syscall);
	return(0);
}

close_netware()
{
	close_connection(netdevfd);
	return(0);
}

/*
 *	add_test_result		- adds a Test_struct type of record to

 *				the database.
 *
 *	input			the fd for the netware connection
 *				a pointer to a Test_struct 
 *				a pointer to a Remark_Struct
 *
 *	output			session_no
 *				
 *	assumes			that the Test_Struct and the Remark_Struct 
 *				is already filled with	the required 
 *				information. The only fields filled
 *				by this function are the session number,
 *				type ,os and the length .
 *
 *	errors			returns -1 in case of errors
 *				returns session_no in the case of success
 */

add_test_result(test_pkt, rem, suite_names)
Test_Struct *test_pkt;
char *rem;
char *suite_names;
{
	int	flags;
	unsigned int	session_no;
	int	test_no;
	Packet_Struct *Packet;
	Remarks_Struct remark;
	int i,len,loop_cnt;
	char	*ptr;
	char	cmd[512];
	char	sql_command[2048];
	Test_Struct	 *test_ptr;
	INT	TID;

	remark_number=1;
	test_no = test_pkt->TestNumber;
/*	db_no = test_pkt->DataBaseID; */
	test_pkt->TestNumberID = 0;

	get_os(test_pkt->OSName, sizeof(test_pkt->OSName));
	test_pkt->Type = TEST_RESULT;
	test_pkt->Length = sizeof(Test_Struct);
	test_pkt->DataBaseID = db_no;
	strcpy(test_pkt->Version, "Certification - 3.0");
/*
printf("type = %d\n", test_pkt->Type);
printf("length = %d\n", test_pkt->Length);
printf("DataBaseID = %d\n", test_pkt->DataBaseID);
printf("TestNumberID = %d\n", test_pkt->TestNumberID);
printf("SuiteNumber = %d\n", test_pkt->SuiteNumber);
printf("TestNumber = %d\n", test_pkt->TestNumber);
printf("ControlFlags = %d\n", test_pkt->ControlFlags);
printf("Iteration = %d\n", test_pkt->Iteration);
printf("Executions = %d\n", test_pkt->Executions);
printf("Status = %d\n", test_pkt->Status);
printf("OSName = %s\n", test_pkt->OSName);
printf("Program = %s\n", test_pkt->Program);
printf("Version = %s\n", test_pkt->Version);
printf("StartTime = %ld\n", strttime);
printf("StopTime = %ld\n", etime);
*/
	fix_test_struct(test_pkt);
	if (t_snd(netdevfd, (char *)test_pkt, sizeof(Test_Struct) - 2,
	     	     0) == -1) {
		t_error("t_snd error ");
		return (-1);
	}
	Packet = (Packet_Struct *)malloc(sizeof(Packet_Struct) );
	memset( (Packet_Struct *) Packet, 0, sizeof(Packet_Struct) );
	if (t_rcv(netdevfd, (char *)Packet, sizeof(Packet_Struct),
	     	     &flags) == -1) {
		t_error("t_rcv error ");
		return (-1);
	}
	TID = Packet->Data[1];
	TID = TID << 8;
	TID = TID | Packet->Data[0];
/*printf("TestNumberID = %d\n", TID); */
	/*
 	 * rem is a char string of arbitrary size. But remarks
	 * structure has a field for 250 bytes only. So split it into
	 * multiple entries in the remarks table.
	 */
	len = strlen(rem);
	loop_cnt = (len/sizeof(remark.Remarks)) + 1;
		
	for (i=0;i<loop_cnt;i++)
	{
		remark.DataBaseID = db_no;
		remark.TestNumber = test_no ;
		remark.TestNumberID = TID;
		remark.Type = TEST_REMARK;
		remark.Length = sizeof(Remarks_Struct);
/*		sprintf(remark.Remarks,"%d.    ",remark_number++);
		strncpy(&remark.Remarks[4],&rem[i*sizeof(remark.Remarks)],sizeof(remark.Remarks));
*/
		strncpy(&remark.Remarks[0],&rem[i*sizeof(remark.Remarks)],sizeof(remark.Remarks));
		remark.Remarks[strlen(remark.Remarks) - 1] = '\0';

		if (t_snd(netdevfd, (char *)&remark, sizeof(Remarks_Struct),
	     	     0) == -1) {
			printf("Error in t_snd . Error no %x\n", t_errno);
			return (-1);
		}
		Packet = (Packet_Struct *)malloc(sizeof(Packet_Struct) );
		memset(Packet, 0, sizeof(Packet_Struct) );
		if (t_rcv(netdevfd, (char *)Packet, sizeof(Packet_Struct),
	     	     	&flags) == -1) {
			printf("Error in t_rcv . Error no %x\n", t_errno);
			return (-1);
		}
	}

	if (add_adapters(netdevfd, test_no, TID) == -1) {
			printf("Error in adding adapter details \n");
			return(-1);
	}

	return(session_no);
}

get_os(osname, count)
char	*osname;
int	count;
{
	int	i, index;
	struct utsname name;

	memset(&name, ' ', sizeof(utsname));
	uname(&name);

	memset(osname, ' ', count);
	osname[24] = 0;
	strncpy(osname, "UnixWare", 8);
	index = 9;
	for (i = 0; i < 3; i++)
		if (name.release[i])
			osname[index++] = name.release[i];

	index++;
	for (i = 0; i < 3; i++)
		if (name.version[i])
			osname[index++] = name.version[i];
	index++;
	for (i = 0; i < 4; i++)
		if (name.machine[i])
			osname[index++] = name.machine[i];
	osname[24] = 0;
}

getdbid()
{
	return(0);
}

add_adapters(fd, test_no, test_noID)
int 	fd;
int 	test_no;
int 	test_noID;
{
	int flags;
	Adapter_Struct	adapter;
	Packet_Struct Packet;
	int no_of_contr;
	int count;
	int	i;
	char	*ptr;

	no_of_contr = get_controllers(test_no);
	if (no_of_contr == -1)
		return(-1);
	for (count = 0; count < no_of_contr; count++)
	{
		strncpy(adapter.DriverName ,controller[count].Driver_Name,sizeof(adapter.DriverName)); 
		adapter.AdapterType = check_if_it_is_lan(adapter.DriverName);
		adapter.Type = LAN_ADAPTER;
		adapter.Length = sizeof(Adapter_Struct) - 2;
		adapter.DataBaseID = db_no;
		adapter.TestNumberID = test_noID;
		adapter.IndexNumber = 0;
		adapter.Number = 0;
		strncpy (adapter.Name , controller[count].Name,sizeof(adapter.Name));
		adapter.IO = controller[count].IO;
		adapter.IRQ = controller[count].IRQ;
		adapter.RAM = controller[count].Ram;
		adapter.ROM = controller[count].Rom;
		adapter.DMA = controller[count].DMA;
		adapter.DriverSize = controller[count].Driver_Size;
		strncpy(adapter.DriverDate ,controller[count].Driver_Date,sizeof(adapter.DriverDate));  

		fix_adapter_struct(&adapter);
/*
printf("About to send adapter details .. \n");
printf("Type = %d\n", adapter.Type);
printf("Length = %d\n", adapter.Length);
printf("DataBaseID = %d\n", adapter.DataBaseID);
printf("TestNumberID = %d\n", adapter.TestNumberID);
printf("AdapterType = %d\n", adapter.AdapterType);
printf("Number = %d\n", adapter.Number);
printf("Name = %s\n", adapter.Name);
printf("IRQ = %d\n", adapter.IRQ);
printf("IO = %d\n", adapter.IO);
printf("RAM = %d\n", adapter.RAM);
printf("ROM = %d\n", adapter.ROM);
printf("DMA = %d\n", adapter.DMA);
printf("DriverName = %s  ", adapter.DriverName);
printf("DriverDate = %s ", adapter.DriverDate);
printf("DriverSize = %d\n", adapter.DriverSize);
*/
		if (t_snd(fd,(char *)&adapter,sizeof(Adapter_Struct) - 2,0) == -1)
		{
			printf("Error in t_snd . Error no %x\n",t_errno);
			return (-1);
		}
		if (t_rcv(fd,(char *)&Packet,sizeof(Packet_Struct),&flags) == -1)
		{
			printf("Error in t_rcv . Error no %x\n",t_errno);
			return (-1);
		}

	}
}

/*
 * 	get_controllers - get the controller information of those drivers
 *			that are configured in sdevice.d
 *
 *	inputs		test number ( 3009 for xtest )
 * 	output		returns the no of controller drivers configured
 *
 *	assumes		a) that the controller info structure is global
 *			b) that information for kd,lp,rtc and fd are not reqd
 *
 *	errors		None
 */
get_controllers(test_no)
int test_no;
{
	char syscall[256];
	FILE *fp,*fopen();
	char name[10];
	int dma,iostart,memstart;
	int irq,c,i;
	int contr_no;
	char machine[100];

/*
CHANGE 2 MADE BY HARI ON 29/1/95
	if ( test_no == 3009 ) {
		sprintf(machine, "%s", getvar ("XT_DISPLAY"));
		if (get_remote_sdevices(machine) == -1)
			return(-1);	
	}

	else 
END OF CHANGE 2
*/  
	    {
		strcpy(syscall,"cat /etc/conf/sdevice.d/* | awk ' { if ($1!=\"#\" && $2==\"Y\" && $6!=0 && $7!=0 )   {print $1,$6,$7,$9,$11} }' > /tmp/scantemp");
		system(syscall);
	}
	strcpy(syscall,"sort +1n -2 -o /tmp/scansort /tmp/scantemp");
	system(syscall);
	fp=fopen("/tmp/scansort","r");
	contr_no=0;
	while(c != EOF)
	{
   		fscanf(fp,"%s%d%x%x%d",&name,&irq,&iostart,&memstart,&dma);
   		c=getc(fp); 
   		if(c != EOF)
   		{
			if ((strcmp(name,"kd") != 0 ) && (strcmp(name,"rtc") != 0) && (strcmp(name,"fd") != 0) && ( strcmp(name,"lp") != 0) )  {

			strcpy(controller[contr_no].Name,description(name));
			strcpy(controller[contr_no].Driver_Name,name);
			controller[contr_no].IRQ = irq;
			controller[contr_no].IO = iostart;
			controller[contr_no].Ram = memstart;
			controller[contr_no].Rom = 0x00;
			controller[contr_no].DMA = dma;
			contr_no++;
			if (contr_no > MAX_CONTR ) break;
			}
   		}
	}
	fclose(fp);
	strcpy(syscall,"rm -rf /tmp/scantemp1 /tmp/scantemp /tmp/scansort");  
	system(syscall); 
  	for (i=0;i<contr_no;i++) {
		get_stat(controller[i].Driver_Name,i);
		/*
		printf("%s\t%d\t%x\t%x\t%d\n",
			controller[i].Name,
			controller[i].IRQ,
			controller[i].IO,
			controller[i].Ram,
			controller[i].DMA);
		printf("Date %s size %d\n",controller[i].Driver_Date,controller[i].Driver_Size);
		*/
	}
	return(contr_no);
}

char *
description(char inname[10])
{
      if(strcmp(inname,"adse")==0)
        return("Adaptec 174x EISA HBA          ");
      else if(strcmp(inname,"adsc")==0)
        return("Adaptec 154x HBA               ");
      else if(strcmp(inname,"ne2k")==0)
        return("Eagle NE2000                   ");
      else if(strcmp(inname,"ne2")==0)
        return("Eagle NE2 Microchannel         ");
      else if(strcmp(inname,"wd")==0)
        return("Western Digital(SMC) 8003      ");
      else if(strcmp(inname,"asyc")==0)
        return("Serial Port                    ");
      else if(strcmp(inname,"lp")==0)
        return("Parallel Port                  ");
      else if(strcmp(inname,"kd")==0)
        return("Keyboard controller            ");
      else if(strcmp(inname,"i3B")==0)
        return("Racal Interlan EISA ethernet   ");
      else if(strcmp(inname,"ie6")==0)
        return("3COM 3C503 etherlink II        ");
      else if(strcmp(inname,"ee16")==0)
        return("Intel etherexpress 16          ");
      else if(strcmp(inname,"el16")==0)
        return("3COM 3C507/3C523    6          ");
      else if(strcmp(inname,"el3")==0)
        return("3COM 3C509/3C579               ");
      else if(strcmp(inname,"imx586")==0)
        return("IMX586                         ");
      else if(strcmp(inname,"fd")==0)
        return("Floppy Drive                   ");
      else if(strcmp(inname,"athd")==0)
        return("Standard ST506                 ");
      else if(strcmp(inname,"bmse")==0)
        return("Bus Mouse                      ");
      else if(strcmp(inname,"cdfs")==0)
        return("CD-ROM file system             ");
      else if(strcmp(inname,"dcd")==0)
        return("                               ");
      else if(strcmp(inname,"dpt")==0)
        return("DPT 2011/2012                  ");
      else if(strcmp(inname,"ict")==0)
        return("Intel Cartridge Tape           ");
      else if(strcmp(inname,"mcesdi")==0)
        return("Micro Channel ESDI controller  ");
      else if(strcmp(inname,"mcis")==0)
        return("Micro Channel SCSI controller  ");
      else if(strcmp(inname,"mcst")==0)
        return("Micro Channel ST506 controller ");
      else if(strcmp(inname,"nucfs")==0)
        return("NetWare Unix Client file system");
      else if(strcmp(inname,"nvt")==0)
        return("Novell Virtual Terminal        ");
      else if(strcmp(inname,"ppc8514")==0)
        return("Pittsburgh Power Computing     ");
      else if(strcmp(inname,"s5")==0)
        return("System Five file system        ");
      else if(strcmp(inname,"tcp")==0)
        return("TCP/IP networking              ");
      else if(strcmp(inname,"ufs")==0)
        return("Unix file system               ");
      else if(strcmp(inname,"vkbd")==0)
        return("Merge virtual keyboard         ");
      else if(strcmp(inname,"vmouse")==0)
        return("Merge virtual mouse driver     ");
      else if(strcmp(inname,"vxfs")==0)
        return("Veritas file system            ");
      else if(strcmp(inname,"wd7000")==0)
        return("Western Digital 7000 controller");
      else if(strcmp(inname,"weitek")==0)
        return("Weitek controller              ");
      else if(strcmp(inname,"fdeb")==0)
        return("Future Domain TMC-8x x         ");
      else if(strcmp(inname,"fdtb")==0)
        return("Future Domain TMC-7000EX       ");
      else if(strcmp(inname,"fdsb")==0)
        return("Future Domain TMC-18xx         "); 
      else
        return(inname); 
}

get_stat(name,contr_no)
char *name;
int contr_no;
{
	char file[100];
	char date[30];
	char date1[30];
	int i;
	struct stat stat1;

	controller[contr_no].Driver_Size = 0;
	strcpy(controller[contr_no].Driver_Date,"");

	memset(file,0,sizeof(file));
	strcpy(file,"/etc/conf/pack.d/");
	strcpy(&file[17],name);
	for (i=0;i<sizeof(file) && file[i];i++);
	if (i>sizeof(file)) return(-1);
	strcpy(&file[i],"/Driver.o");
	stat(file,&stat1);

/*	strftime(date,sizeof(date),"%d %X",localtime(&stat1.st_mtime)); */
	strcpy(date1, ctime(&(stat1.st_mtime)) );
	for (i = 4; i < 30; i++)
		date[i-4] = date1[i];
	for (i = 3; i < 20; i++)
		date[i] = date[i+1];
	date[19] = '\0';
	strncpy(controller[contr_no].Driver_Date,date,20);
	controller[contr_no].Driver_Date[20] = '\0';
	controller[contr_no].Driver_Size = stat1.st_size;
}

get_remote_sdevices(machine)
char *machine;
{
	char buf[256];
	char syscall[256];
	char mc_name[100];
	int i,ret,found;

	i=0;
	
	while( (i<100) && (machine[i] != ':') && (machine[i] != 0)) {
		mc_name[i] = machine[i];
		i++;
	}
	mc_name[i]=0;
	sprintf(syscall,"grep %s /etc/hosts|grep -v \"^[ 	]*#\" >/dev/null",mc_name);
	ret = system(syscall);
	if ((ret>>8) == 1) {
		sprintf(buf,"The machine name %s should be present in the /etc/hosts file. Add the entry and update results again.",mc_name);
		printf("%s\n",buf);
		printf( buf);
		return(-1);
	}
	
	sprintf(syscall,"echo %s uwcert >rhosts",mc_name);
	ret=system(syscall);
	found=0;
	while ( found == 0 )
	{
		sprintf(syscall,"rcp rhosts %s:/home/uwcert/.rhosts >/dev/null 2>&1",mc_name);
		ret = system(syscall);
		if (ret != 0) {
		sprintf(buf, "This machine's node name needs to be added in the /etc/hosts.equiv file of the machine %s. This entry does not exist. Please add the entry and continue.",mc_name);
		printf("%s\n",buf);
		printf( buf);
		continue;
		}
		found=1;
	}
	sprintf(syscall,"rsh -n -l uwcert %s \"cat /etc/conf/sdevice.d/*\"  >/tmp/scantemp1 2>/dev/null",mc_name);
	ret = system(syscall);
	if (ret != 0) {
		printf("The machine %s is not responding. Check the TCP/IP setup and update results again.\n",mc_name);
		sprintf(buf, "The machine %s is not responding. Check the TCP/IP setup and update results again.",mc_name);
		printf( buf);
		return(-1);
	}
	strcpy(syscall,"cat /tmp/scantemp1 | awk ' { if ($1!=\"#\" && $2==\"Y\" && $6!=0 && $7!=0 )   {print $1,$6,$7,$9,$11} }' > /tmp/scantemp");
	system(syscall);

	return(0);
}
	
char	*
getvar(name)
char	*name;
{
	/* return value of specified configuration variable
	    	     */

	char	**cur;
	char	*cp;
	size_t len;

	if (varptrs == NULL || vardata == NULL)
		return ((char *) NULL);

	/* varptrs is an array of strings of the form: "VAR
	    	    =value" */
	len = strlen(name);
	for (cur = (char **) varptrs; *cur != NULL; cur++) {
		cp = *cur;
		if (strncmp(cp, name, len) == 0 && cp[len] ==
		    '=')
			return & cp[len+1];
	}

	return ((char *) NULL);
}

fixprdList()
{
	struct  Link *ptr = NULL;
	int 	index;

	printf("Give the index of the prouct\n");
	scanf("%d", &index);
/*	ptr = Root;
	while (index)
	{
		printf("string - %s\n", ptr->String);
		ptr = ptr->next;
		index--;
	}
	UpdateDBID = ptr->DataBaseID;
*/
	UpdateIndex = index;
}


fix_test_struct(test_struct)
Test_Struct *test_struct;
{
	int i;
	char	*ptr;

	ptr = (char *)test_struct;

	for (i = 114; i < (114 + 8); i++)
		ptr[i] = ptr[i + 2];
}

fix_adapter_struct(ad)
Adapter_Struct *ad;
{
	int i;
	char	*ptr;

	ptr = (char *)ad;

	for (i = 146; i < (146 + 4); i++)
		ptr[i] = ptr[i + 2];
}

int
check_if_it_is_lan(name)
char	*name;
{
	char  	*tmp_buf;
	char	syscall[100];
	FILE 	*fp;
	char	buf[256];
	char	drivername[15];

	if ( (tmp_buf = (char *)getenv("TET_SUITE_ROOT")) == (char *)NULL ) {
	 	printf("TET_SUITE_ROOT not set!\n");
	        return(-1);
	}
	sprintf(syscall, "/usr/sbin/ifconfig -a > /tmp/ifconfig.out", tmp_buf);

	system(syscall);

	fp = fopen("/tmp/ifconfig.out", "r");
	while( (fgets(buf, 50, fp) ) != NULL)
	{
		sscanf(buf, "%s", drivername);
		if ( (strncmp(drivername, name, strlen(name)) ) == 0)
		{
			return(2);
		}
	}
	return(0);
}


