#ident	"@(#)NWcpsOps.c	1.5"
#ident	"$Header$"

/*******************************************************************************
 *
 * FILENAME:    NWcpsOps.c
 *
 * DESCRIPTION: NetWare Unix Client 
 *              The NUC Client Network Printer independent 
 *		layer Operations.  Component of the NUC	PCLIENT Services.
 *
 * SCCS:	NWcpsOps.c 1.5  10/30/97 at 14:52:49
 *
 *
 * ABSTRACT:
 *	  The NWcpsOps.c contains the platform independent client network
 *	  printer operations which augment the UNIX print complex.  See
 *	  NWcpsIntroduction(3) for a complete description of the platform
 *	  indepdent (portable) Client Network Printer operations.  These
 *	  operations represent the independent layer of the PCLIENT
 *	  facilities.  They are common across all UNIX Client Platforms.  The
 *	  dependent layer platform commands integrate them into the target
 *	  platform OS.
 *
 *	  The following Operations are contained in this module.
 *		NWcpsAttachQMS()
 *		NWcpsDeleteJob()
 *		NWcpsDetachQMS()
 *		NWcpsGetQueueInfo()
 *		NWcpsSpoolFile()
 *		NWcpsStatusQueueJob()
 *
 * CHANGE HISTORY:
 *
 * 30-10-97  Paul Cunningham        ul96-24720
 *           Change function NWcpsGetQueueInfo() to add more logging messages of
 *           function call failures and debug info
 *           Also ignore error from GET_OBJECT_NAME, setting userName blank.
 *
 *******************************************************************************
 */

/* static char sccsid[]=" NWcpsOps.c 1.6"; */

/*
 * Include System Definitions
 */
#include <stdio.h>
#include <errno.h>
#include <varargs.h>
#include <ctype.h>
#include <fcntl.h>
#include <pwd.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef	IPX_AF_INTEGRATED
#include <sys/socket.h>
#include <netipx/in.h>
#endif	/* IPX_AF_INTEGRATED	*/

/*
 * Include NetWare headers
 */
#include <netdir.h>
#include <sys/nwportable.h>
#include <nw/nwcalls.h>
#include <cpscommon.h>
#include <cpsservice.h>
#include <lpd.h>

#include <sys/spilcommon.h>
#include <nwmp.h>
#include <nucinit.h>
#include <nw/nwclient.h>
#include <dlfcn.h>
#include <stdarg.h>

#define MAXHOSTNAMELEN 256
#define bzero(ptr,len) memset(ptr, '\0', len)


/*#define NNN 1*/

/*
 * Forward Reference local static functions
 */
static	ccode_t getMoreStatus();

static  FPTR_STRUCT fptr;
static	void		*NwClntHandle;	

/*-------------------------------------------------------------------------------
** 
**  This function takes an option parameter and returns the corresponding 
**  NetWare NWCalls API function as a pointer.  This function provides the
**  interface to the dlsym calls that pull the specified function into the
**  address space.
*/

NWCCODE callAPI( int apiOption, ...)
{
	NWCCODE  	ccode;
	int			retCode;
	int			(*reqInit)(INIT_REQ_T *,DS_INIT_REQ_T *);
	va_list		ap;
	char 		*dlerr;		
	static int	libraries_opened = 0;


	if (!libraries_opened) {
		/* then lets open the libraries */

#ifdef NNN
		logit(LOG_ERR, "Libraries haven't been opened yet.");		
#endif
        fptr.NwCalHandle  = dlopen("libNwCal.so", RTLD_NOW);
        NwClntHandle  = dlopen("libNwClnt.so", RTLD_NOW);
	
        if( !fptr.NwCalHandle )
            goto ERROR;

		if( !NwClntHandle )
			goto ERROR;

		libraries_opened = 1;
#ifdef NNN
        logit(LOG_ERR, "Libraries are now opened, Initializing requester.");
#endif

		/* now lets initialize the requester */ 

		reqInit = (int (*)(INIT_REQ_T *, DS_INIT_REQ_T *)) 
                                        dlsym(NwClntHandle, "initreq");

 		retCode = (*reqInit)(NULL, NULL);
#ifdef NNN
        logit(LOG_ERR, "Requester initialized, Return Code = %d", retCode);
#endif
	}	

	/* Now let's get the specified API function from the opened API library
	** (using dlsym) and call that function with the given parameter list
	*/


	va_start(ap, apiOption);

	switch (apiOption) {

	case GET_OBJECT_ID:
		{
			NWCONN_HANDLE	arg1;
			pnstr8			arg2;
			nuint16			arg3;
			pnuint32		arg4;

			fptr.GetObjectID = (NWCCODE (*)(NWCONN_HANDLE, pnstr8, nuint16, pnuint32))
										dlsym(fptr.NwCalHandle, "NWGetObjectID");
	
			arg1 = va_arg(ap, NWCONN_HANDLE);
			arg2 = va_arg(ap, pnstr8);
			arg3 = (nuint16)va_arg(ap, int);
			arg4 = (pnuint32)va_arg(ap, int);
#ifdef NNN
        logit(LOG_ERR, "Calling GetObjectID...");
#endif
			ccode = (*fptr.GetObjectID)(arg1, arg2, arg3, arg4);
#ifdef NNN
        logit(LOG_ERR, "Back from GetObjectID with ccode = %d", ccode);
#endif
		}	
		break;
		 	
 	case GET_OBJECT_NAME:
		{
			NWCONN_HANDLE	arg1;
			nuint32			arg2;
			pnstr8			arg3;
			pnuint16		arg4;

        	fptr.GetObjectName = (NWCCODE (*)(NWCONN_HANDLE, nuint32, pnstr8, pnuint16))
                                        dlsym(fptr.NwCalHandle, "NWGetObjectName");
 
			arg1 = va_arg(ap, NWCONN_HANDLE);
        	arg2 = (nuint32)va_arg(ap, int);
        	arg3 = va_arg(ap, pnstr8);
        	arg4 = (pnuint16)va_arg(ap,int);

#ifdef NNN
        logit(LOG_ERR, "Calling GetObjectName...");
#endif
 	       ccode = (*fptr.GetObjectName)(arg1, arg2, arg3, arg4);
#ifdef NNN
        logit(LOG_ERR, "Back from GetObjectName with ccode = %d", ccode);
#endif

		}
        break; 

    case GET_BINDERY_ACCESS_LEVEL:
		{
	        NWCONN_HANDLE   arg1;
    	    pnuint8         arg2;
        	pnuint32        arg3;
 
        	fptr.GetBinderyAccessLevel = (NWCCODE (*)(NWCONN_HANDLE, pnuint8, pnuint32))
                                        dlsym(fptr.NwCalHandle, "NWGetBinderyAccessLevel");

        	arg1 = va_arg(ap, NWCONN_HANDLE);
        	arg2 = (pnuint8)va_arg(ap, int);
        	arg3 = (pnuint32)va_arg(ap, int);

#ifdef NNN
        logit(LOG_ERR, "Calling GetBinderyAccessLevel...");
#endif
        	ccode = (*fptr.GetBinderyAccessLevel)(arg1, arg2, arg3);
#ifdef NNN
        logit(LOG_ERR, "Back from GetBinderyAccessLevel with ccode = %d", ccode);
#endif
 
		}
        break;


    case OPEN_CONN_BY_NAME:
        {
			NWCONN_HANDLE   arg1;
        	pNWCConnString  arg2;
       		pnstr	        arg3;
        	nuint	        arg4;
        	nuint	        arg5;
        	NWCONN_HANDLE   *arg6;

        	fptr.OpenConnByName = (NWCCODE (*)(NWCONN_HANDLE, pNWCConnString, pnstr, nuint, nuint,
										NWCONN_HANDLE *))
                                        dlsym(fptr.NwCalHandle, "NWOpenConnByName");

        	arg1 = va_arg(ap, NWCONN_HANDLE);
        	arg2 = va_arg(ap, pNWCConnString);
        	arg3 = va_arg(ap, pnstr);
        	arg4 = (nuint)va_arg(ap, int);
        	arg5 = (nuint)va_arg(ap, int);
        	arg6 = va_arg(ap, NWCONN_HANDLE *);

#ifdef NNN
        logit(LOG_ERR, "Calling OpenConnByName...");
#endif
        	ccode = (*fptr.OpenConnByName)(arg1, arg2, arg3, arg4, arg5, arg6);
#ifdef NNN
        logit(LOG_ERR, "Back from OpenConnByName with ccode = %d", ccode);
#endif

		}
        break;

    case SCAN_CONN_INFORMATION:
        {
			pnuint32		arg1;
        	nuint			arg2;
       	 	nuint           arg3;
       	 	nptr			arg4;
        	nuint           arg5;
        	nuint			arg6;
        	nuint			arg7;
        	pnuint32		arg8;
        	nptr			arg9;

        	fptr.ScanConnInformation = (NWCCODE (*)(pnuint32, nuint, nuint, nptr, nuint, 
										nuint, nuint,pnuint32, nptr))
                                        dlsym(fptr.NwCalHandle, "NWScanConnInformation");

        	arg1 = va_arg(ap, pnuint32);
        	arg2 = va_arg(ap, nuint);
        	arg3 = va_arg(ap, nuint);
        	arg4 = va_arg(ap, nptr);
        	arg5 = va_arg(ap, nuint);
        	arg6 = va_arg(ap, nuint);
        	arg7 = va_arg(ap, nuint);
        	arg8 = va_arg(ap, pnuint32);
        	arg9 = va_arg(ap, nptr);

#ifdef NNN
        logit(LOG_ERR, "Calling ScanConnInformation...");
#endif
        	ccode = (*fptr.ScanConnInformation)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
#ifdef NNN
        logit(LOG_ERR, "Back from ScanConnInformation with ccode = %d", ccode);
#endif

		}
        break;

    case CLOSE_CONN:
		{
			NWCONN_HANDLE	arg1;

        	fptr.CloseConn = (NWCCODE (*)(NWCONN_HANDLE))
                                        dlsym(fptr.NwCalHandle, "NWCloseConn");

			arg1 = va_arg(ap, NWCONN_HANDLE);


#ifdef NNN
        logit(LOG_ERR, "Calling CloseConn...");
#endif
        	ccode = (*fptr.CloseConn)(arg1);
#ifdef NNN
        logit(LOG_ERR, "Back from CloseConn with ccode = %d", ccode);
#endif

		}
        break;
 
    case GET_CONN_INFORMATION:
        {
			NWCONN_HANDLE   arg1;
        	nuint			arg2;
        	nuint			arg3;
        	nptr			arg4;
 
        	fptr.GetConnInformation = (NWCCODE (*)(NWCONN_HANDLE, nuint, nuint, nptr))
                                        dlsym(fptr.NwCalHandle, "NWGetConnInformation");

			arg1 = va_arg(ap, NWCONN_HANDLE);
			arg2 = va_arg(ap, nuint);
			arg3 = va_arg(ap, nuint);
			arg4 = va_arg(ap, nptr);

#ifdef NNN
        logit(LOG_ERR, "Calling GetConnInformation...");
#endif
        	ccode = (*fptr.GetConnInformation)(arg1, arg2, arg3, arg4);
#ifdef NNN
        logit(LOG_ERR, "Back from GetConnInformation with ccode = %d", ccode);
#endif

		}
        break;

    case GET_QUEUE_JOB_LIST_2:
        {
			NWCONN_HANDLE   arg1;
        	nuint32         arg2;
        	nuint32         arg3;
        	QueueJobListReply *arg4;

        	fptr.GetQueueJobList2 = (NWCCODE (*)(NWCONN_HANDLE, nuint32, nuint32, QueueJobListReply *))
                                        dlsym(fptr.NwCalHandle, "NWGetQueueJobList2");

        	arg1 = va_arg(ap, NWCONN_HANDLE);
        	arg2 = va_arg(ap, nuint32);
        	arg3 = va_arg(ap, nuint32);
        	arg4 = va_arg(ap, QueueJobListReply *);

#ifdef NNN
        logit(LOG_ERR, "Calling GetQueueJobList2...");
#endif
        	ccode = (*fptr.GetQueueJobList2)(arg1, arg2, arg3, arg4);
#ifdef NNN
        logit(LOG_ERR, "Back from GetQueueJobList2 with ccode = %d", ccode);
#endif
 
		}
        break;

    case READ_QUEUE_JOB_ENTRY_2:
        {
			NWCONN_HANDLE   arg1;
        	nuint32         arg2;
        	nuint32         arg3;
        	NWQueueJobStruct *arg4;
 

        	fptr.ReadQueueJobEntry2 = (NWCCODE (*)(NWCONN_HANDLE, nuint32, nuint32, NWQueueJobStruct *))
                                        dlsym(fptr.NwCalHandle, "NWReadQueueJobEntry2");

        	arg1 = va_arg(ap, NWCONN_HANDLE);
        	arg2 = va_arg(ap, nuint32);
        	arg3 = va_arg(ap, nuint32);
        	arg4 = va_arg(ap, NWQueueJobStruct *);

#ifdef NNN
        logit(LOG_ERR, "Calling ReadQueueJobEntry2...");
#endif
			ccode = (*fptr.ReadQueueJobEntry2)(arg1, arg2, arg3, arg4);
#ifdef NNN
        logit(LOG_ERR, "Back from ReadQueueJobEntry2 with ccode = %d", ccode);
#endif
		}
        break;


    case REMOVE_JOB_FROM_QUEUE_2:
        {
			NWCONN_HANDLE   arg1;
        	nuint32         arg2;
        	nuint32         arg3;

        	fptr.RemoveJobFromQueue2 = (NWCCODE (*)(NWCONN_HANDLE, nuint32, nuint32))
                                        dlsym(fptr.NwCalHandle, "NWRemoveJobFromQueue2");

        	arg1 = va_arg(ap, NWCONN_HANDLE);
        	arg2 = va_arg(ap, nuint32);
        	arg3 = va_arg(ap, nuint32);

#ifdef NNN
        logit(LOG_ERR, "Calling RemoveJobFromQueue2...");
#endif
        	ccode = (*fptr.RemoveJobFromQueue2)(arg1, arg2, arg3);
#ifdef NNN
        logit(LOG_ERR, "Back from RemoveJobFromQueue2 with ccode = %d", ccode);
#endif
 
		}
        break;

    case READ_QUEUE_CURRENT_STATUS_2:
        {
			NWCONN_HANDLE   arg1;
        	nuint32         arg2;
       		pnuint32        arg3;
        	pnuint32        arg4;
        	pnuint32        arg5;
       	 	pnuint32        arg6;
        	pnuint32        arg7;

        	fptr.ReadQueueCurrentStatus2 = (NWCCODE (*)(NWCONN_HANDLE, nuint32, pnuint32, pnuint32,
                                        pnuint32, pnuint32, pnuint32))
                                        dlsym(fptr.NwCalHandle, "NWReadQueueCurrentStatus2");

        	arg1 = va_arg(ap, NWCONN_HANDLE);
        	arg2 = va_arg(ap, nuint32);
        	arg3 = va_arg(ap, pnuint32);
        	arg4 = va_arg(ap, pnuint32);
        	arg5 = va_arg(ap, pnuint32);
        	arg6 = va_arg(ap, pnuint32);
        	arg7 = va_arg(ap, pnuint32);

#ifdef NNN
        logit(LOG_ERR, "Calling ReadQueueCurrentStatus2...");
#endif
        	ccode = (*fptr.ReadQueueCurrentStatus2)(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
#ifdef NNN
        logit(LOG_ERR, "Back from ReadQueueCurrentStatus2 with ccode = %d", ccode);
#endif


		}
        break;

    case GET_QUEUE_JOB_FILE_SIZE_2:
        {
			NWCONN_HANDLE   arg1;
        	nuint32         arg2;
        	nuint32			arg3;
        	pnuint32        arg4;

        	fptr.GetQueueJobFileSize2 = (NWCCODE (*)(NWCONN_HANDLE, nuint32, nuint32, pnuint32))
                                        dlsym(fptr.NwCalHandle, "NWGetQueueJobFileSize2");

        	arg1 = va_arg(ap, NWCONN_HANDLE);
        	arg2 = va_arg(ap, nuint32);
        	arg3 = va_arg(ap, nuint32);
        	arg4 = va_arg(ap, pnuint32);

#ifdef NNN
        logit(LOG_ERR, "Calling GetQueueJobFileSize2...");
#endif
        	ccode = (*fptr.GetQueueJobFileSize2)(arg1, arg2, arg3, arg4);
#ifdef NNN
        logit(LOG_ERR, "Back from GetQueueJobFileSize2 with ccode = %d", ccode);
#endif
 
		}
        break;


    case CLOSE_FILE_AND_ABORT_QUEUE_JOB_2:
        {
			NWCONN_HANDLE   arg1;
        	nuint32         arg2;
        	nuint32         arg3;
        	NWFILE_HANDLE   arg4;
 
        	fptr.CloseFileAndAbortQueueJob2 = (NWCCODE (*)(NWCONN_HANDLE, nuint32, nuint32, 
														NWFILE_HANDLE))
                                        dlsym(fptr.NwCalHandle, "NWCloseFileAndAbortQueueJob2");

        	arg1 = va_arg(ap, NWCONN_HANDLE);
        	arg2 = va_arg(ap, nuint32);
        	arg3 = va_arg(ap, nuint32);
        	arg4 = va_arg(ap, NWFILE_HANDLE);

#ifdef NNN
        logit(LOG_ERR, "Calling CloseFileAndAbortQueueJob2...");
#endif
        	ccode = (*fptr.CloseFileAndAbortQueueJob2)(arg1, arg2, arg3, arg4);
#ifdef NNN
        logit(LOG_ERR, "Back from CloseFileAndAbortQueueJob2 with ccode = %d", ccode);
#endif

		}
        break;

    case CLOSE_FILE_AND_START_QUEUE_JOB_2:
        {
			NWCONN_HANDLE   arg1;
        	nuint32         arg2;
        	nuint32         arg3;
        	NWFILE_HANDLE   arg4;

        	fptr.CloseFileAndStartQueueJob2 = (NWCCODE (*)(NWCONN_HANDLE, nuint32, nuint32, 
														NWFILE_HANDLE))
                                        dlsym(fptr.NwCalHandle, "NWCloseFileAndStartQueueJob2");

        	arg1 = va_arg(ap, NWCONN_HANDLE);
        	arg2 = va_arg(ap, nuint32);
        	arg3 = va_arg(ap, nuint32);
        	arg4 = va_arg(ap, NWFILE_HANDLE);

#ifdef NNN
        logit(LOG_ERR, "Calling CloseFileAndStartQueueJob2...");
#endif
        	ccode = (*fptr.CloseFileAndStartQueueJob2)(arg1, arg2, arg3, arg4);
#ifdef NNN
        logit(LOG_ERR, "Back from CloseFileAndStartQueueJob2 with ccode = %d", ccode);
#endif
 
		}
        break;

    case CREATE_QUEUE_FILE_2:
        {
			NWCONN_HANDLE   arg1;
        	nuint32         arg2;
        	NWQueueJobStruct  *arg3;
        	NWFILE_HANDLE   *arg4;
 
        	fptr.CreateQueueFile2 = (NWCCODE (*)(NWCONN_HANDLE, nuint32, NWQueueJobStruct *,
												NWFILE_HANDLE *))
                                        dlsym(fptr.NwCalHandle, "NWCreateQueueFile2");

        	arg1 = va_arg(ap, NWCONN_HANDLE);
        	arg2 = va_arg(ap, nuint32);
        	arg3 = va_arg(ap, NWQueueJobStruct *);
        	arg4 = va_arg(ap, NWFILE_HANDLE *);

#ifdef NNN
        logit(LOG_ERR, "Calling CreateQueueFile2...");
#endif
       		ccode = (*fptr.CreateQueueFile2)(arg1, arg2, arg3, arg4);
#ifdef NNN
        logit(LOG_ERR, "Back from CreateQueueFile2 with ccode = %d", ccode);
#endif

		}
        break;


    case WRITE_FILE:
        {
			NWCONN_HANDLE   arg1;
        	nuint32         arg2;
        	pnuint8         arg3;
 
        	fptr.WriteFile = (NWCCODE (*)(NWCONN_HANDLE, nuint32, pnuint8))
                                        dlsym(fptr.NwCalHandle, "NWWriteFile");

        	arg1 = va_arg(ap, NWCONN_HANDLE);
        	arg2 = va_arg(ap, nuint32);
        	arg3 = va_arg(ap, pnuint8);

#ifdef NNN
        logit(LOG_ERR, "Calling WriteFile...");
#endif
        	ccode = (*fptr.WriteFile)(arg1, arg2, arg3);
#ifdef NNN
        logit(LOG_ERR, "Back from WriteFile with ccode = %d", ccode);
#endif
		}
        break;
	}

	va_end(ap);

	return(ccode);

ERROR:
	logit(LOG_ERR, "%s. (NUC libraries may not be installed)", dlerror());
	return(~SUCCESS);
	
} 




/*
 * BEGIN_MANUAL_ENTRY(NWcpsIntroduction(3), \
 *			./man/shell/print/client/Introduction)
 * NAME
 *	NWcpsIntroduction	- Introduction to the Novell NetWare UNIX
 *				  Client QMS Print Client Services Suite.
 *				  These are the indepdent (portable) operations
 *				  of the PCLIENT facilities.
 *
 * DESCRIPTION
 *	The NUC provides a network printer service facility to the local UNIX
 *	client work station.  Print requests on the local UNIX client are
 *	distribuited to NetWare Servers for print servicing.  In this model, 
 *	print files are spooled and scheduled with the local UNIX print system
 *	(ie. LPD).  The local UNIX print system schedules the spooled file with
 *	the appropriate job service for unspooling to a print device (such as a 
 *	PostScript printer).  For printers registered on the local UNIX
 *	work stations which are managed by NetWare, these requests are 
 *	scheduled with the NUC PCLIENT Suite.
 *
 *	The NUC QMS Client Printer (PCLIENT) Services Suite is a peer suite to
 *	the default BSD Network Printer (PLPD) suite provided with the UNIX work
 *	station.  The NUC QMS Client Printer Services thus follow the existing
 *	UNIX Network Printer model defined by BSD.  When the NUC PCLIENT
 *	Services are installed, UNIX printing can be distributed to
 *	NetWare using the Novell NCP protocol, which allows UNIX work stations
 *	to plug directly into a NetWare NOS and share real print devices with
 *	DOS, OS/2, Windows, and Macintosh client desk stations.
 *
 *	The NUC creates a QMS print job on the NetWare Server/Queue  associated
 *	with the locally known printer name.  It then unspools (reads) the 
 *	print spool file on the local UNIX work station and spools (writes) it
 *	over the wire (network) to a QMS spool file on the NetWare Server. 
 *	When the entire UNIX spool file has been moved to the NetWare Server,
 *	the NUC requests that it be scheduled with the specified NetWare
 *	Print Server (real print device).  From here the spool file is
 *	completely managed by the NetWare Servers, unless additional UNIX print
 *	management requests to status or delete it are issued.
 *	
 * CLIENT QMS PRINTER OPERATIONS
 *	attach UNIX Client to QMS	- NWcpsAttachQMS(3)
 *	delete a print file on NetWare	- NWcpsDeleteJob(3)
 *	detach UNIX Client from QMS	- NWcpsDetachQMS(3)
 *	print a UNIX file on NetWare	- NWcpsSpoolFile(3)
 *	status print files on NetWare	- NWcpsStatusQueueJob(3)
 *
 * OPERATION NOTES
 *	The 'NWcpsAttachQMS' is used to attach the UNIX Client to QMS for
 *	servicing of a QMS queue.  It must be called prior to using
 *	NWcspDeleteJob(3), NWcpsGetQueueInfo(3),NWcpsSpoolFile(3),
 *	and NWcpsStatusQueueJob(3).
 *
 *	The 'NWcpsDeleteJob' is used to delete a spooled print file which is
 *	queued for service on a NetWare QMS Server.  The spool file may be 
 *	awaiting service or may be actually being serviced (printed) by a 
 *	NetWare Print Server.  In either case, it can be deleted.  This 
 *	facility is a direct extension to the lprm(1) local UNIX shell command,
 *	and extends the GUI print manager as well.
 *
 *	The 'NWcpsGetQueueInfo' is used to get the latest queue information
 *	on a NetWare QMS queue.  In addition to getting the general queue
 *	information, it also caches the status of queued jobs for subsequent
 *	delivery via NWcpsStatusQueueJob(3) requests.  This is needed to 
 *	service the UNIX semantics of all jobs for a user.
 *
 *	The 'NWcpsDetachQMS' is used to detach the UNIX Client from QMS when
 *	NWcpsDeleteJob(3), NWcpsGetQueueInfo(3), NWcpsSpoolFile(3),
 *	and NWcpsStatusQueueJob(3) are no longer needed by the UNIX Client.
 *
 *	The 'NWcpsSpoolFile' is used to spool a UNIX print file over the wire
 *	to NetWare QMS.  The file to be spooled is acutally unspooled from the
 *	local UNIX print file, and spooled over the wire to NetWare.  This 
 *	facility is a direct extension to the lp(1), lpr(1), and lpd(8) local
 *	UNIX shell commands and daemons, and extends the GUI print manager as
 *	well.
 *	
 *	The 'NWcpsStatusQueueJob' is used to status (query) a NetWare QMS queue
 *	for Print Jobs.  This facility is a direct extension to the lpq(1) 
 *	local UNIX shell command, and extends the GUI print manager as well.
 *	It must issued after a NWcpsGetQueueInfo(3), to extract the cached
 *	requests.
 *
 * END_MANUAL_ENTRY
 */


/*
 * BEGIN_MANUAL_ENTRY(NWcpsAttachQMS(3), \
 *			./man/shell/print/client/AttachQMS)
 * NAME
 *	NWcpsAttachQMS -	Attach UNIX Client to a QMS Server
 *				for service with a PRINT Queue, and Print 
 *				Server.  Member of the NUC PCLIENT Suite.
 *
 * SYNOPSIS
 *	public ccode_t
 *	NWcpsAttachQMS(qmsInfo, qmsHandle)
 *
 *	QMS_INFO_T	*qmsInfo;
 *	opaque_t	**qmsHandle;	/* Opaque to caller	*\
 *
 * INPUT
 *	qmsInfo->qmsServer	- A pointer to a NULL terminated string 
 *				  name of the QMS Server to attach.
 *	qmsInfo->qmsQueue	- A pointer to a NULL terminated string 
 *				  name of the QMS Queue to use.
 *	qmsInfo->pServer	- A pointer to a NULL terminated string
 *				  name of the Print Server which is to
 *				  service queued jobs.  A NULL pointer
 *				  specifies ANY Print Server.
 *
 * OUTPUT
 *	qmsHandle	- A pointer to the service context the UNIX Client
 *			  has been attached to.
 *
 *
 * RETURN VALUES
 *	0	- Successfull completion.
 *
 *	-1	- Unsuccessfull completion.
 *
 * DESCRIPTION
 *	The 'NWcpsAttachQMS' attaches the UNIX Client to the specified QMS
 *	server, for service with the specified print queue and associated
 *	print server(s).  Once attached, the UNIX Client can perform additional
 *	operations such as spool, status, and delete.  This is a component of
 *	the independent (portable) piece of the PCLIENT print facility.
 *
 * SEE ALSO
 *	NWcpsIntroduction(3), NWcpsDetachQMS(3)
 *
 * END_MANUAL_ENTRY
 */

public NWCCODE
NWcpsAttachQMS(qmsInfo, qmsHandle)

QMS_INFO_T	*qmsInfo;
QMS_SERVICE_T	**qmsHandle;
{
	NWCCODE ccode;	
	NWCConnString serverName;

	nuint	authState;

	/*
	 * Get some space for this service context
	 */
	if ( (*qmsHandle = (QMS_SERVICE_T *) malloc(sizeof(QMS_SERVICE_T))) 
			== NULL ) {
		/*
		 * Couldn't allocate space for a QMS_SERVICE_T object
		 */
		return(~(SUCCESS));
	}

	/*
	 * Attach Client Process to QMS Server.
	 */	

	serverName.pString = (nptr)qmsInfo->qmsServer;
	serverName.uStringType = 0;
	serverName.uNameFormatType = 0;
	
	if (( ccode = callAPI(OPEN_CONN_BY_NAME, NULL, &serverName,
						(pnstr)"NCP_SERVER", 
						NWC_OPEN_PUBLIC | NWC_OPEN_LICENSED, NWC_TRAN_TYPE_IPX,
						&((*qmsHandle)->connID))) != SUCCESS &&
						ccode != ALREADY_ATTACHED ) {

		/*
		 * Failed to attach to QMS Server Platform
		 */
		free(*qmsHandle);
		return(~(SUCCESS));
	}

	authState = NWC_AUTH_STATE_NONE;

	ccode = callAPI(GET_CONN_INFORMATION, (*qmsHandle)->connID, NWC_CONN_INFO_AUTH_STATE, sizeof(nuint),
							  (nptr)&authState);

	if (authState == NWC_AUTH_STATE_NONE) 
		return( NOT_AUTHENTICATED );



	/*
	 * Resolve external queue name to internal queue name
	 */
	if ( callAPI(GET_OBJECT_ID, (*qmsHandle)->connID, 
					   (pnstr8)qmsInfo->qmsQueue,
					   (nuint16)OT_PRINT_QUEUE, 
					   (pnuint32)&((*qmsHandle)->queueID)) != SUCCESS) {
		/*
		 * Failed to find specified print queue on Server Platform
		 */
		callAPI(CLOSE_CONN, (*qmsHandle)->connID);
		free(*qmsHandle);
		return(~(SUCCESS));
	}

	/*
	 * Resolve external print server name to internal print server name
	 */
	if ( (qmsInfo->pServer == NULL) || (strlen(qmsInfo->pServer) == 0) ) {
		/*
		 * Default Case, any pServer will do
		 */
		(*qmsHandle)->pServerID = ANY_PSERVER;
	} else {
		if ( callAPI(GET_OBJECT_ID, (*qmsHandle)->connID, 
						   (pnstr8)qmsInfo->pServer,
						   (nuint16)OT_JOB_SERVER, 
						   (pnuint32)&((*qmsHandle)->pServerID)) != SUCCESS) {
			if ( callAPI(GET_OBJECT_ID, (*qmsHandle)->connID, 
							   (pnstr8)qmsInfo->pServer,
							   (nuint16)OT_PRINT_SERVER, 
							   (pnuint32)&((*qmsHandle)->pServerID)) != SUCCESS) {
				if ( callAPI(GET_OBJECT_ID, (*qmsHandle)->connID, 
								   (pnstr8)qmsInfo->pServer,
								   (nuint16)OT_FILE_SERVER, 
								   (pnuint32)&((*qmsHandle)->pServerID)) != SUCCESS) {
					/*
					 * Failed to find specefied print server
					 */
					callAPI(CLOSE_CONN, (*qmsHandle)->connID);
					free(*qmsHandle);
					return(~(SUCCESS));
				}
			}
		}
	}

	(*qmsHandle)->cachedStatuses = NULL;
	(*qmsHandle)->numberCached = 0;
	return(SUCCESS);
}


/*
 * BEGIN_MANUAL_ENTRY(NWcpsDeleteJob(3), \
 *			./man/shell/print/client/DeleteJob)
 * NAME
 *	NWcpsDeleteJob -	Delete a Queued Print Job on NetWare QMS Server.
 *				Member of the NUC PCLIENT Suite.
 *
 * SYNOPSIS
 *	public ccode_t
 *	NWcpsDeleteJob(qmsHandle, jobNumber)
 *
 *	opaque_t	*qmsHandle;	/* Opaque to caller *\
 *	uint16		jobNumber;
 *
 * INPUT
 *	qmsHandle	- A pointer to the service context the UNIX Client
 *			  has been attached to defining the queue the job
 *			  to be removed is on.
 *	jobNumber	- Job number of print request to delete.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	0	- Successfull completion.
 *
 *	-1	- Unsuccessfull completion.
 *
 * DESCRIPTION
 *	The 'NWcpsDeleteJob' deletes a print job queued with NetWare QMS.  The
 *	print job will be stopped if it is being serviced first, and then the
 *	job and associated spool file will be deleted.  This is the
 *	indepdent (portable) piece of the PCLIENT print cancel facility.
 *
 * SEE ALSO
 *	NWcpsIntroduction(3), NWcpsSpoolFile(3), NWcpsStatusQueueJob(3)
 *
 * END_MANUAL_ENTRY
 */

public NWCCODE
NWcpsDeleteJob(qmsHandle, jobNumber)

QMS_SERVICE_T	*qmsHandle;
nuint32			jobNumber;
{
	return ( callAPI(REMOVE_JOB_FROM_QUEUE_2, qmsHandle->connID, 
								   qmsHandle->queueID,
								   jobNumber));
}


/*
 * BEGIN_MANUAL_ENTRY(NWcpsDetachQMS(3), \
 *			./man/shell/print/client/DetachQMS)
 * NAME
 *	NWcpsDetachQMS -	Detach UNIX Client from a QMS Server.
 *				Member of the NUC PCLIENT Suite.
 *
 * SYNOPSIS
 *	public ccode_t
 *	NWcpsDetachQMS(qmsHandle)
 *
 *	opaque_t	*qmsHandle;	/* Opaque to caller	*\
 *
 * INPUT
 *	qmsHandle	- A pointer to the service context the UNIX Client
 *			  has been attached to, and now is to detached from.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	0	- Successfull completion.
 *
 *	-1	- Unsuccessfull completion.
 *
 * DESCRIPTION
 *	The 'NWcpsDetachQMS' detaches the UNIX Client process from QMS service
 *	context which was attached previously via NWcpsAttachQMS(3).
 *	This is a component of the independent (portable) piece of the
 *	PCLIENT print facility.
 *
 * SEE ALSO
 *	NWcpsIntroduction(3), NWcpsAttachQMS(3)
 *
 * END_MANUAL_ENTRY
 */

public ccode_t
NWcpsDetachQMS(qmsHandle)

QMS_SERVICE_T	*qmsHandle;
{

	callAPI(CLOSE_CONN, qmsHandle->connID);

	/*
	 * Free the memory allocated to qmsHandle
	 */
	if ( qmsHandle->cachedStatuses ) {
		free(qmsHandle->cachedStatuses);
	}
	free(qmsHandle);

	return(SUCCESS);
}


/*
 * BEGIN_MANUAL_ENTRY(NWcpsGetQueueInfo(3), \
 *			./man/shell/print/client/GetQueueInfo)
 * NAME
 *	NWcpsGetQueueInfo -	Query NetWare QMS Queue for Status of print
 *				jobs submitted from clients.  Member of the
 *				NUC PCLIENT Suite.
 *
 * SYNOPSIS
 *	public	ccode_t
 *	NWcpsGetQueueInfo(qmsHandle, numberOfJobs, queueState)
 *	
 *	opaque_t	*qmsHandle;	/* Opaque to caller *\
 *	uint32		*numberOfJobs;
 *	QUEUE_STATE_T	*queueState;
 *
 *
 * INPUT
 *	qmsHandle	- A pointer to the service context the UNIX Client
 *			  has been attached to, and now is to detached from.
 *
 * OUTPUT
 *	numberOfJobs	- Number of jobs on the PRINTQ associated with
 *			  qmsHandle context.
 *	queueState	- Set to one of the following States: (See cpscommon.h
 *			  for a complete description of them)
 *				PRINTQ_READY
 *				PRINTQ_NO_PSERVERS
 *				PRINTQ_DISABLED
 *				PRINTQ_FULL
 *
 * RETURN VALUES
 *	0	- Successfull completion.
 *
 *	-1	- Unsuccessfull completion.
 *
 * DESCRIPTION
 *	The 'NWcpsGetQueueInfo' queries the state of the QMS queue.
 *	This is the indepdent (portable) piece of the PCLIENT queue status
 *	and job removal facilities.
 *
 * SEE ALSO
 *	NWcpsIntroduction(3), NWcpsDeleteJob(3), NWcpsSpoolFile(3)
 *
 * END_MANUAL_ENTRY
 */

public	ccode_t
NWcpsGetQueueInfo(qmsHandle, numberOfJobs, queueState)

QMS_SERVICE_T	*qmsHandle;
uint32		*numberOfJobs;
QUEUE_STATE_T	*queueState;
{

	int	i;
	NWCCODE	ccode;

	/*
	 * NetWare QMS Specific Data Structures
	 */
	nuint32	qmsQueueStatus;
	nuint32	qmsNumberOfJobsInQueue;
	nuint32	qmsNumberOfPservers;
	nuint32	qmsObjectID;
	static	nuint32	qmsPserverObjectIDs[MAX_SERVER_OBJECT_IDS];
	static	nuint32	qmsPserverConnIDs[MAX_CONNECTION_NUMBERS];
	static	NWQueueJobStruct	qmsJobRequest;

	static	QueueJobListReply	qmsJobListReply;
	static	nuint32	qmsQueueStartPosition = 0;

	/*
	 * Query QMS for Queue Statistics
	 */
	if ( callAPI(READ_QUEUE_CURRENT_STATUS_2, qmsHandle->connID, 
								   qmsHandle->queueID,
								   &qmsQueueStatus,
								   &qmsNumberOfJobsInQueue,
								   &qmsNumberOfPservers, 
								   qmsPserverObjectIDs,
								   qmsPserverConnIDs) != SUCCESS ) {
		/*		
		 * Couldn't get the queue status
		 */
		return(~(SUCCESS));
	}

	if ( qmsNumberOfJobsInQueue ) {
		if (( ccode=callAPI( GET_QUEUE_JOB_LIST_2, qmsHandle->connID, 
				     qmsHandle->queueID, qmsQueueStartPosition, 
				     &qmsJobListReply)) != SUCCESS) 
	{
		/* Couldn't get the list of jobs themselves
		 */
		logit( LOG_ERR, 
		       "READ_QUEUE_JOB_LIST_2 failed, error =%d", ccode);
			return(~(SUCCESS));
		}
	}

	/* Map the return arguments
	 */
	*numberOfJobs = (uint32) qmsNumberOfJobsInQueue;

	switch ( qmsQueueStatus & (QS_CANT_ADD_JOBS | QS_CANT_SERVICE_JOBS) )
	{
	case QS_CANT_ADD_JOBS:
	case (QS_CANT_ADD_JOBS | QS_CANT_SERVICE_JOBS):
		*queueState = PRINTQ_FULL;
		break;

	case QS_CANT_SERVICE_JOBS:
		*queueState = PRINTQ_DISABLED;
		break;

	default:
		if ( qmsNumberOfPservers == 0 ) {
			*queueState = PRINTQ_NO_PSERVERS;
		} else {
			*queueState = PRINTQ_READY;
		}
	}
	
	if ( qmsNumberOfJobsInQueue ) {
		/*
		 * Acquire status of all jobs, we need this in order to 
		 * support the upcoming NWcpsStatusQueueJob(3) calls.
		 */

		if ( qmsHandle->cachedStatuses ) {
			/*
			 * Invalidate previous cache
			 */
			free(qmsHandle->cachedStatuses);
			qmsHandle->cachedStatuses = NULL;
			qmsHandle->numberCached= 0;
		}

		if ( (qmsHandle->cachedStatuses = (GENERIC_STATUS_T *)
			malloc( (qmsNumberOfJobsInQueue *
				sizeof(GENERIC_STATUS_T) ))) == NULL ) {
			/*
			 * Couldn't allocate space for a job status cache
			 */
			return(~(SUCCESS));
		}


		/* Aquire minimum generic status for each job
		 */
		for ( i = 0; i < (int) qmsNumberOfJobsInQueue; i++ ) 
		{
			qmsHandle->cachedStatuses[i].entryAvailable = FALSE;
			logit( LOG_DEBUG, 
			       "READ_QUEUE_JOB_ENTRY_2 jobNumber = %d",
			       qmsJobListReply.jobNumberList[i]);

			if (( ccode = callAPI( READ_QUEUE_JOB_ENTRY_2, 
			                       qmsHandle->connID,
					       qmsHandle->queueID, 
					       qmsJobListReply.jobNumberList[i],
				  	       &qmsJobRequest)) != SUCCESS) 
			{
				if ( (ccode & 0xFF) == ERR_NO_Q_JOB ) 
				{
				  /* Job is gone, non atomic RACE,
				   * continue to next, leave this 
				   * slot incomplete.
				   */
				  continue;
				} 
				else 
				{
				  /* Fatal Error
				   */
				  logit
				    ( 
				    LOG_ERR, 
				    "READ_QUEUE_JOB_ENTRY_2 failed, error =%d",
				    ccode
				    );		
				  return(~(SUCCESS));
				}
			}

			/*
			 * Partially populate generic status from job entry
			 */
			qmsHandle->cachedStatuses[i].jobNumber = 
				qmsJobListReply.jobNumberList[i];

			if ( qmsJobRequest.servicingServerID ) {
				qmsHandle->cachedStatuses[i].jobState =
					NUCPS_JOB_ACTIVE;

			} else if ( qmsJobRequest.jobControlFlags &
					QF_OPERATOR_HOLD ) {
				qmsHandle->cachedStatuses[i].jobState =
					NUCPS_JOB_OPER_HELD;

			} else if ( qmsJobRequest.jobControlFlags &
					QF_USER_HOLD ) {
				qmsHandle->cachedStatuses[i].jobState =
					NUCPS_JOB_USER_HELD;

			} else if ( qmsJobRequest.jobControlFlags &
					QF_ENTRY_OPEN ) {
				qmsHandle->cachedStatuses[i].jobState =
					NUCPS_JOB_SPOOLING;

			} else {
				qmsHandle->cachedStatuses[i].jobState =
					NUCPS_JOB_READY;
			}

			strcpy((char *)qmsHandle->cachedStatuses[i].spoolFile,
			       (char *)qmsJobRequest.jobDescription);

			qmsHandle->cachedStatuses[i].priority = 
						qmsJobRequest.jobPosition;

			/* Get the NetWare User External Name
			 */
			if (( ccode = callAPI( GET_OBJECT_NAME, 
			                       qmsHandle->connID,
			                       qmsJobRequest.clientID,
			                  qmsHandle->cachedStatuses[i].userName,
			                  (pnuint16)&qmsObjectID)) != SUCCESS) 
			{
				/* Ignore Error, log and set userName blank
				 */
				logit( LOG_ERR, 
				       "GET_OBJECT_NAME failed, error = %d",
				       ccode);		
				qmsHandle->cachedStatuses[i].userName[0] = '\0';
			}

			logit( LOG_DEBUG, 
			       "GET_OBJECT_NAME userName = %s",
			       qmsHandle->cachedStatuses[i].userName);

			qmsHandle->cachedStatuses[i].entryAvailable = TRUE;

			/*
			 * Note:
			 *	Populate the .clientWSName, .spoolSize, and
			 *	.printServer members when a request is 
			 *	needed at NWcpsStatusQueueJob(3) to save up
			 *	to 3 NCP wire requests.
			 *
			 *	Cache up these internal ID,s for use in
			 *	acquiring this information later.
			 */
			qmsHandle->cachedStatuses[i].clientStation =
				(uint16) qmsJobRequest.clientStation;
			qmsHandle->cachedStatuses[i].pServerID =
				qmsJobRequest.servicingServerID;
		}
	}

	qmsHandle->numberCached = (uint32) qmsNumberOfJobsInQueue;
	return(SUCCESS);
}


/*
 * BEGIN_MANUAL_ENTRY(NWcpsStatusQueueJob(3), \
 *			./man/shell/print/client/StatusQueueJob)
 * NAME
 *	NWcpsStatusQueueJob -	Query NetWare QMS Queue for Status of print
 *				jobs submitted from clients.  Member of the
 *				NUC PCLIENT Suite.
 *
 * SYNOPSIS
 *	public ccode_t
 *	NWcpsStatusQueueJob( qmsHandle, jobType, jobRequest, genericStatus)
 *
 *	opauqe_t		*qmsHandle;	/* Opaque to caller *\
 *	STATUS_REQUEST_T	jobType;
 *	JOB_REQUEST_T		jobRequest;
 *	GENERIC_STATUS_T	**genericStatus;
 *
 * INPUT
 *	qmsHandle	- A pointer to the service context the UNIX Client
 *			  has been attached to.
 *	jobType		- The type of job request to search for.  Set one of
 *			  the following:
 *				NEXT_QUEUED_JOB
 *				NEXT_USER_JOB
 *				THIS_JOB
 *	jobRequest	- The specifiec search token.  Not used for 'jobType'
 *			  NEXT_QUEUED_JOB.  Set to one of the following:
 *				number	- Individual Job# to status for
 *					  'jobType' THIS_JOB.
 *				user	 - Name of user to staus for 'jobType'
 *					   NEXT_USER_JOB.
 *
 * OUTPUT
 *	(*genericStatus)->jobNumber	- Set to the NetWare QMS job number
 *					  of the print request.
 *	(*genericStatus)->jobState	- Set to the current state of the
 *					  NetWare QMS job.  Set to one of the
 *					  following: (See cpscommon.h for
 *					  complete description)
 *						NUCPS_JOB_ACTIVE
 *						NUCPS_JOB_READY
 *						NUCPS_JOB_WAITING
 *						NUCPS_JOB_USER_HELD
 *						NUCPS_JOB_OPER_HELD
 *						NUCPS_JOB_SPOOLING
 *	(*genericStatus)->clientWSName	- Set to the external name of the client
 *					  work station if known, the IPX
 *					  Internet address if known, or a NULL
 *					  string if not known.
 *	(*genericStatus)->userName	- Set to the external name of the
 *					  NetWare user who queued request.
 *	(*genericStatus)->spoolFile	- Set to the file name the spool file
 *					  was created from.
 *	(*genreicStatus)->spoolSize	- Size in bytes of the spool file.
 *	(*genreicStatus)->priority	- Ordinal priority position of the 
 *					  request in the queue.
 *	(*genericStatus)->printServer	- Set to the external name of the
 *					  PSERVER that is unspooling the
 *					  the spool file.  Set only when
 *					  'jobState' == NUCPS_JOB_ACTIVE.
 *
 *
 * RETURN VALUES
 *	0	- Successfull completion.
 *
 *	-1	- Unsuccessfull completion.
 *
 * DESCRIPTION
 *	The 'NWcpsStatusQueueJob' queries the status of a NetWare QMS print
 *	queue, and returns the results which are to be displayed as a local UNIX
 *	printer queue.  This is the independent (portable) piece of the PCLIENT
 *	queue status facility.
 *
 * SEE ALSO
 *	NWcpsIntroduction(3), NWcpsDeleteJob(3), NWcpsSpoolFile(3)
 *
 * END_MANUAL_ENTRY
 */

public ccode_t
NWcpsStatusQueueJob(qmsHandle, jobType, jobRequest, genericStatus)

QMS_SERVICE_T		*qmsHandle;
STATUS_REQUEST_T	jobType;
JOB_REQUEST_T		jobRequest;
GENERIC_STATUS_T	**genericStatus;
{

	uint16			i;

	*genericStatus = NULL;

	/*
	 * Search for job matching search request type
	 */
	switch ( jobType ) {

	case THIS_JOB:
		/*
		 * Search for the explicit job in 'jobRequest->jobNumber'
		 */
		for (i = 0; i < (int) qmsHandle->numberCached; i++) {
			if ( qmsHandle->cachedStatuses[i].entryAvailable ) {
				if ( qmsHandle->cachedStatuses[i].jobNumber ==
					(uint16) jobRequest.jobNumber ) {
					/*
					 * This is the job requested
					 */
					*genericStatus =
						&(qmsHandle)->cachedStatuses[i];
					if ( getMoreStatus(qmsHandle,
						*genericStatus) != SUCCESS ) {
						/*
						 * Cache not in sync with
						 * server, can't find it
						 */
						return(~(SUCCESS));
					}
					break;
				}
			}
		}
		break;

	case NEXT_USER_JOB:
		/*
		 * Search for the next job of user 'jobRequest->userName'
		 */
		for (i = 0; i < (int) qmsHandle->numberCached; i++) {
			if ( qmsHandle->cachedStatuses[i].entryAvailable ) {
				if ( strcmp((char *)jobRequest.userName,
					(char *)qmsHandle->cachedStatuses[i].userName)
						== 0 ) {
					/*
					 * This is the next user job queued
					 */
					*genericStatus =
						&(qmsHandle)->cachedStatuses[i];
					if ( getMoreStatus(qmsHandle,
						*genericStatus) != SUCCESS ) {
						/*
						 * Cache not in sync with
						 * server, look for another
						 */
						*genericStatus = NULL;
						continue;
					} else {
						break;
					}
				}
			}
		}
		break;

	case NEXT_QUEUED_JOB:
	default:
		/*
		 * Search for the next queued job
		 */
		for (i = 0; i < (int) qmsHandle->numberCached; i++) {
			if ( qmsHandle->cachedStatuses[i].entryAvailable ) {
				/*
				 * This is the next queued job
				 */
				*genericStatus =
					&(qmsHandle)->cachedStatuses[i];
				if ( getMoreStatus(qmsHandle,
					*genericStatus) != SUCCESS ) {
					/*
					 * Cache not in sync with
					 * server, look for another
					 */
					*genericStatus = NULL;
					continue;
				} else {
					break;
				}
			}
		}
	}

	if ( !(*genericStatus) )  {
		/*
		 * Could'nt find the job type
		 */
		return(~(SUCCESS));
	}

	/*
	 * Consume this entry from further searches (i.e. Eat the Big Cookie)
	 */
	(*genericStatus)->entryAvailable = FALSE;

	return(SUCCESS);
}


/*
 * BEGIN_MANUAL_ENTRY(getMoreStatus(3), \
 *			./man/shell/print/client/getMoreStatus)
 * NAME
 *	getMoreStatus -	Query NetWare QMS Server platform for un-cached
 *			generic status entry information.  Private support
 *			function to NWcpsStatusQueueJob(3).
 *
 * SYNOPSIS
 *	static	ccode_t
 *	getMoreStatus(qmsHandle, genericStatus)
 *	
 *	QMS_SERVICE_T		*qmsHandle;
 *	GENERIC_STATUS_T	*genericStatus;
 *
 * INPUT
 *	qmsHandle			- A pointer to the service context the
 *					  UNIX Client has been attached to.
 *	genericStatus->jobNumber	- Set to the NetWare QMS job number
 *					  of the print request.
 *	genericStatus->clientStation	- Connection on QMS server of the
 *					  client station which issued the
 *					  request.  0 if client not attached.
 *	genericStatus->pServerID	- Internal Object ID of the Print
 *					  Server currently servicing the
 *					  active job.  Set only when the
 *					  'jobState' is NUCPS_JOB_ACTIVE.
 *
 * OUTPUT
 *	genericStatus->clientWSName	- Set to the external name of the client
 *					  work station if known, the IPX
 *					  Internet address if known, or a NULL
 *					  string if not known.
 *	genreicStatus->spoolSize	- Size in bytes of the spool file.
 *	genericStatus->printServer	- Set to the external name of the
 *					  PSERVER that is unspooling the
 *					  the spool file.  Set only when
 *					  'jobState' == NUCPS_JOB_ACTIVE.
 *
 *
 * RETURN VALUES
 *	0	- Successfull completion.
 *
 *	-1	- Unsuccessfull completion.
 *
 * DESCRIPTION
 *	The 'getMoreStatus' queries the NetWare QMS Server platform to acquire
 *	the un-cached information of the 'genericStatus' argument.  This is
 *	necessary to compelte a generic status entry.  The idea here is to
 *	cache the information necessary to search for jobs, and acquire the
 *	rest upon a job status cache hit, since it takes up to 3 NCP requests
 *	to complete the status.  The NWcpsStatusQueueJob(3) calls
 *	'getMoreStatus' upon the cache hit to complete the status entry before
 *	returning it to a platform centric interface program.
 *
 * SEE ALSO
 *	NWcpsStatusQueueJob(3)
 *
 * END_MANUAL_ENTRY
 */

static	ccode_t
getMoreStatus(qmsHandle, genericStatus)

QMS_SERVICE_T		*qmsHandle;
GENERIC_STATUS_T	*genericStatus;
{
#ifdef	IPX_AF_INTEGRATED
	nuint8	ipxAddress;
	struct	hostent	*hostEntry;
#else
	static	struct	{
		uchar	ipxaddr_net[4];
		uchar	ipxaddr_node[6];
		uchar	ipxaddr_socket[2];
	}ipxAddress;
#endif	/* IPX_AF_INTEGRATED	*/

	uint16		qmsObjectID;
	nuint		level;
	struct netbuf ipxAddr;

	level = NWC_CONN_INFO_TRAN_ADDR;

	/*
 	 * Get client IPX address
 	 */
	if ( callAPI(GET_CONN_INFORMATION, qmsHandle->connID, level, sizeof(level),
							  (nptr)&ipxAddr) != SUCCESS ) {

		/*
		 * Don't know who the client is
		 */
		strcpy((char *)genericStatus->clientWSName, "");
	} else {

		memcpy((char *)&ipxAddress, (char *)ipxAddr.buf, ipxAddr.len);
#ifdef	IPX_AF_INTEGRATED
		/*
		 * Now try and resolve client address to external name
		 */
		if ( (hostEntry = gethostbyaddr(&ipxAddress ,sizeof(ipx_addr_t),
				AF_IPX)) ) {
			/*
			 * Use the external name of client
			 */
			strncpy(genericStatus->clientWSName, hostEntry->h_name,
				NWMAX_OBJECT_NAME_LENGTH-1);
		} else {
#endif	/* IPX_AF_INTEGRATED	*/
			/*
			 * Don't know the external name, use internal IPX name
			 */
			sprintf((char *) genericStatus->clientWSName,
				"%.2X%.2X%.2X%.2X:%.2X%.2X%.2X%.2X%.2X%.2X",
				(int) ipxAddress.ipxaddr_net[0],
				(int) ipxAddress.ipxaddr_net[1],
				(int) ipxAddress.ipxaddr_net[2],
				(int) ipxAddress.ipxaddr_net[3],
				(int) ipxAddress.ipxaddr_node[0],
				(int) ipxAddress.ipxaddr_node[1],
				(int) ipxAddress.ipxaddr_node[2],
				(int) ipxAddress.ipxaddr_node[3],
				(int) ipxAddress.ipxaddr_node[4],
				(int) ipxAddress.ipxaddr_node[5]);
#ifdef	IPX_AF_INTEGRATED
		}
#endif	/* IPX_AF_INTEGRATED	*/
	}
	
	/*
	 * Get the spooled file size
	 */
	if ( callAPI(GET_QUEUE_JOB_FILE_SIZE_2, qmsHandle->connID,
								qmsHandle->queueID, 
								genericStatus->jobNumber,
								&(genericStatus)->spoolSize)!= SUCCESS  ) {

		/*
		 * Cache must out of sync, invalidate entry
		 */
		genericStatus->entryAvailable = FALSE;
		return(~(SUCCESS));
	}

	if ( genericStatus->pServerID ) {
		/*
		 * Get the PSERVER External Name
		 */
		if ( callAPI(GET_OBJECT_NAME, qmsHandle->connID,
							 genericStatus->pServerID, 
							 genericStatus->printServer,
							 &qmsObjectID) != SUCCESS  ) {

			/*
			 * Cache must be out of sync, invalidate entry
			 */
			genericStatus->entryAvailable = FALSE;
			return(~(SUCCESS));
		}
	}

	return(SUCCESS);
}

/* Use xauto to authenticate to the NetWare Server */
void
authenticateToServer(dest, user, uid, pid)
char *dest, *user;
uid_t *uid;
int *pid;
{
	int newpid, status, r;
	char uid_str[16], pid_str[16];


	if (*user == NULL)
		return;

	sprintf((char *)uid_str, "%d", *uid);
	sprintf((char *)pid_str, "%d", *pid);
	
	newpid = fork();
	if (newpid == 0) {
		putenv("DISPLAY=unix:0.0");

		execl("/usr/X/bin/xauto", "xauto", "-s", dest, "-u", user, 
											"-i", uid_str);

		logit(LOG_ERR,
		    "exec of \"/usr/X/bin/xauto\" failed, errno = %d", errno);
	} 
	else if (newpid == -1) {
		logit(LOG_ERR, "fork failed");
	}
 	else {
		while (r = wait(&status)) {
			if (r == -1 && errno == ECHILD)
				break;
			else {
				logit(LOG_DEBUG,
				    "authenticate_to_server: wait returns %d", r);
			}
		}
	}
}

/*
**  Find a user-ID with an authenticated connection to the
**  specified server.  If there is more than one connection
**  to the specified server, this subroutine returns the
**  first entry in the task (connection) list.
*/
int
findAuthenticatedUser(server, uid)
char *server;
uid_t *uid;
{
        int fp;
        int ccode;
        int return_val = ~(SUCCESS);
        struct scanTaskReq request;
        char serverName[NWC_MAX_SERVER_NAME_LEN];
	struct netbuf		*serverAddress;
	struct netconfig	*getnetconfigent();
	struct netconfig	*np;
	struct nd_hostserv	hs;
	struct nd_addrlist	*addrs;

	static int		library_opened = 0;

	/* function pointer definitions */
	int (*NWMP_Open)(void);
	int (*NWMP_Close)(int);


	if (!library_opened) {
		/* then lets open the libraries */
#ifdef NNN
        logit(LOG_ERR, "In findAuthenticatedUser: Opening Library...");
#endif

		if (!NwClntHandle) {
       		NwClntHandle  = dlopen("libNwClnt.so", RTLD_NOW);
	
       		if( !NwClntHandle )
           		goto ERROR;
#ifdef NNN
        logit(LOG_ERR, "In findAuthenticatedUser: Library opened.");
#endif

		}
		library_opened = 1;
	}	

	/* the library is open. Now assign the function pointers */

	NWMP_Open = (int (*)(void))dlsym(NwClntHandle, "NWMPOpen");
	NWMP_Close = (int (*)(int))dlsym(NwClntHandle, "NWMPClose");

#ifdef NNN
        logit(LOG_ERR, "In findAuthenticatedUser: Calling NWMP_Open...");
#endif

        if ((fp = (*NWMP_Open)()) == -1)
                return(~SUCCESS);
#ifdef NNN
        logit(LOG_ERR, "In findAuthenticatedUser: Back from NWMP_Open");
#endif

	if((np = getnetconfigent("ipx"))==NULL)
		return(~SUCCESS);

        strncpy(serverName, server, sizeof(serverName));
 	hs.h_host = serverName;
	hs.h_serv = "1105";

	if (netdir_getbyname(np, &hs, &addrs)){
		freenetconfigent(np);
		return_val = ~SUCCESS;	
		goto out;
	}

	if (addrs->n_cnt == 0){
		freenetconfigent(np);
        	netdir_free(addrs, ND_ADDRLIST);
		return_val = ~SUCCESS;
		goto out;
	}

	serverAddress = addrs->n_addrs;
        request.address.maxlen = serverAddress->maxlen;
	request.address.len = serverAddress->len;
	request.address.buf = (char *) malloc( serverAddress->maxlen );
       	memcpy(request.address.buf, serverAddress->buf,
            serverAddress->len);

	request.userID = -1;
#ifdef NNN
        logit(LOG_ERR, "In findAuthenticatedUser: Calling NWMP_SCAN_TASK ioctl...");
#endif

        while ((ccode = ioctl(fp, NWMP_SCAN_TASK, &request)) == 0) {
                if (request.mode & SPI_TASK_AUTHENTICATED) {
                        *uid = request.userID;
                        return_val = SUCCESS;
                        break;
                }
        }
#ifdef NNN
        logit(LOG_ERR, "In findAuthenticatedUser: Back from NWMP_SCAN_TASK.");
#endif

	freenetconfigent(np);
	netdir_free(addrs, ND_ADDRLIST);
out:
#ifdef NNN
        logit(LOG_ERR, "In findAuthenticatedUser: Calling NWMP_Close...");
#endif

        (*NWMP_Close)(fp);
#ifdef NNN
        logit(LOG_ERR, "In findAuthenticatedUser: Back from NWMP_Close.");
#endif

        return(return_val);

ERROR:
        logit(LOG_ERR, "%s. (NUC libraries may not be installed)", dlerror());
        return(~SUCCESS);

 
}

/*
**  Find workstation owner and return user-ID and login name.
*/
int
getWorkStationOwner(uid, user_name)
uid_t *uid;
char **user_name;
{
	char *server = "/dev/X/server.0";
	char *userPidFile = "/dev/X/xdm-userPid";
	struct passwd *pw;
	struct stat sb;

	if (stat(userPidFile, &sb) == -1)
		if (stat(server, &sb) == -1)
			return(~(SUCCESS));

	if ((pw = getpwuid(sb.st_uid)) == NULL) {
		return(~(SUCCESS));
	}
	*uid = sb.st_uid;
	*user_name = pw->pw_name;
	return(SUCCESS);
}
