#ident	"@(#)cpscommon.h	1.4"
#ident	"$Header$"

/*******************************************************************************
 *
 * Netware Unix Client 
 *
 *
 * MODULE:
 *	cpscommon.h	- The NUC PCLIENT suite common definitions. 
 *			  Component of the NUC PCLEINT Services.
 *
 * SCCS:	cpscommon.h 1.4  10/30/97 at 11:49:25
 *
 * ABSTRACT:
 *	The cpscommon.h is included with PCLIENT interface programs and Client 
 *	Print Services operations.  This provides a consistent representation
 *	of interface information used between these layers.
 *
 * CHANGE HISTORY:
 *
 * 29-10-97  Paul Cunningham        ul96-24720
 *           Change element jobNumber of structure GENERIC_STATUS_T so that it 
 *           is 'uint32' (was uint16) as this is now a 32 bit value over the 
 *           NCP API. Also changed nucChild/printreq.c and cancel.c
 *
 *******************************************************************************
 */

#ifndef	_CPS_COMMON_H
#define	_CPS_COMMON_H

#ifndef	SUCCESS
#define SUCCESS				 0
#endif

#define NWMAX_OBJECT_NAME_LENGTH	48
#define NWMAX_JOB_DESCRIPTION_LENGTH	50
#define NWMAX_FORM_NAME_LENGTH          16
#define NWMAX_BANNER_NAME_FIELD_LENGTH  13
#define NWMAX_BANNER_FILE_FIELD_LENGTH  13
#define NWMAX_HEADER_FILE_NAME_LENGTH   14
#define NWMAX_JOB_DIR_PATH_LENGTH       80
#define NWMAX_QUEUE_JOB_TIME_SIZE        6
#define NWMAX_JOB_FILE_NAME_LENGTH      14

#define MAX_SERVER_OBJECT_IDS		25
#define MAX_CONNECTION_NUMBERS		25
 
/*the following are print flags used with the NWPrintJobStruct */
#define NWPCF_SUPPRESS_FF		0x0008
#define NWPCF_NOTIFY_USER		0x0010
#define NWPCF_TEXT_MODE			0x0040
#define NWPCF_PRINT_BANNER		0x0080

/* the following are the definitions that callAPI uses to identify which
** API is to be invoked */

#define GET_OBJECT_ID				1
#define GET_OBJECT_NAME				2
#define GET_BINDERY_ACCESS_LEVEL		3
#define OPEN_CONN_BY_NAME			4
#define SCAN_CONN_INFORMATION			5
#define CLOSE_CONN				6
#define GET_CONN_INFORMATION			7
#define GET_QUEUE_JOB_LIST_2			8
#define READ_QUEUE_JOB_ENTRY_2			9
#define REMOVE_JOB_FROM_QUEUE_2			10
#define READ_QUEUE_CURRENT_STATUS_2		11
#define GET_QUEUE_JOB_FILE_SIZE_2		12
#define CLOSE_FILE_AND_ABORT_QUEUE_JOB_2	13
#define CLOSE_FILE_AND_START_QUEUE_JOB_2	14
#define CREATE_QUEUE_FILE_2			15
#define WRITE_FILE				16

/* the following structure contains all of the function pointers that 
** will receive the address of their corresponding function from a call to dlsym().
** (see the function callAPI() in NWcpsOps.c) */ 

typedef struct fptr_struct {
	void	*NwCalHandle;
	NWCCODE	(*GetObjectID)(NWCONN_HANDLE, pnstr8, nuint16, pnuint32);
	NWCCODE (*GetObjectName)(NWCONN_HANDLE, nuint32, pnstr8, pnuint16);
	NWCCODE (*GetBinderyAccessLevel)(NWCONN_HANDLE, pnuint8, pnuint32);
	NWCCODE	(*OpenConnByName)(NWCONN_HANDLE, pNWCConnString, pnstr, nuint, nuint,
                                        NWCONN_HANDLE *);
	NWCCODE (*ScanConnInformation)(pnuint32, nuint, nuint, nptr, nuint,
                                        nuint, nuint,pnuint32, nptr);
	NWCCODE (*CloseConn)(NWCONN_HANDLE);
	NWCCODE (*GetConnInformation)(NWCONN_HANDLE, nuint, nuint, nptr);
	NWCCODE (*GetQueueJobList2)(NWCONN_HANDLE, nuint32, nuint32, QueueJobListReply *);
	NWCCODE (*ReadQueueJobEntry2)(NWCONN_HANDLE, nuint32, nuint32, NWQueueJobStruct *);
	NWCCODE (*RemoveJobFromQueue2)(NWCONN_HANDLE, nuint32, nuint32);
	NWCCODE (*ReadQueueCurrentStatus2)(NWCONN_HANDLE, nuint32, pnuint32, pnuint32,
                                        pnuint32, pnuint32, pnuint32);
	NWCCODE	(*GetQueueJobFileSize2)(NWCONN_HANDLE, nuint32, nuint32, pnuint32);
	NWCCODE (*CloseFileAndAbortQueueJob2)(NWCONN_HANDLE, nuint32, nuint32,
                                                        NWFILE_HANDLE);
	NWCCODE	(*CloseFileAndStartQueueJob2)(NWCONN_HANDLE, nuint32, nuint32,
                                                        NWFILE_HANDLE);
	NWCCODE (*CreateQueueFile2)(NWCONN_HANDLE, nuint32, NWQueueJobStruct *,
                                                NWFILE_HANDLE *);
	NWCCODE (*WriteFile)(NWCONN_HANDLE, nuint32, pnuint8);
} FPTR_STRUCT;


/*
 * Define some of the favorite NUC types which would normally come from
 * nwctypes.h, but can't due to conflicts with nwapi.h
 */
typedef	u_char	uchar;
typedef	void	opaque_t;
typedef	void	void_t;
typedef	int32	ccode_t;
#define	private	static
#define	public	

/*
 * NAME
 *	queueState	- The NUC PCLEINT NWcpsGetQueueState(3) generic queue
 *			  state.
 *
 * DESCRIPTION
 *	This enumeration defines the NeXT Client independent print service
 *	NWcpsGetQueueState(3) generic queue states, used in queueState
 *	structure.
 *
 * MEMBERS
 *	PRINTQ_READY		- Print Queue is ready, with PSERVERS attached.
 *	PRINTQ_NO_PSERVERS	- Print Queue is ready, no PSERVERS attached
 *				  for servicing.
 *	PRINTQ_DISABLED		- Print Queue is disabled.
 *	PRINTQ_FULL		- Print Queue is full.
 */
typedef	enum	queueState {
	PRINTQ_READY,
	PRINTQ_NO_PSERVERS,
	PRINTQ_DISABLED,
	PRINTQ_FULL
}QUEUE_STATE_T;

/*
 * NAME
 *	statusRequest	- The NUC PCLEINT NWcpsStatusQueueJob(3) job status
 *			  type to return status on.
 *
 * DESCRIPTION
 *	This enumeration defines the NeXT Client independent print service
 *	NWcpsStatusQueue(3) jobRequest type to return a status on.
 *
 * MEMBERS
 *	NEXT_QUEUED_JOB		- Return the status of the next job (any user)
 *				  on the queue.  
 *	NEXT_USER_JOB		- Return the status of the next job for
 *				  jobRequest.userName on the queue.
 *	THIS_JOB		- Return the status of the specified for job
 *				  for jobRequest.jobNumber on the queue.
 */
typedef	enum	statusRequest	{
	NEXT_QUEUED_JOB,
	NEXT_USER_JOB,
	THIS_JOB
}STATUS_REQUEST_T;
/*
 * NAME
 *	jobRequest	- The NUC PCLEINT QMS Job Request Interface Structure.
 *
 * DESCRIPTION
 *	This union structure defines the job context to status on 
 *	NWcpsStatusQueue(3) calls.  It is used by PCLIENT interface programs
 *	to specify the type of NetWare QMS status context to return.
 *
 * MEMBERS
 *	userName	- Null terminated string of the NetWare user to get the
 *			  job status for.  Used in conjunction with
 *			  NEXT_USER_JOB types on NWcpsStatusQueue(3) calls.
 *	jobNumber	- Unique job number of the print request on the 
 *			  NetWare QMS queue to be statused.  Used in conjuction
 *			  with THIS_JOB types on NWcpsStatusQueue(3) calls..
 */
typedef	union	jobRequest {
	u_char	*userName;
	uint32	jobNumber;
}JOB_REQUEST_T;

/*
 * NAME
 *	jobState	- The NUC PCLEINT NWcpsStatusQueueJob(3) generic job
 *			  state.
 *
 * DESCRIPTION
 *	This enumeration defines the NeXT Client independent print service
 *	NWcpsStatusQueue(3) generic status job states, used in genericStatus
 *	structure.
 *
 * MEMBERS
 *	NUCPS_JOB_ACTIVE	- Print request is being serviced (unspooled)
 *				  by a PSERVER.
 *	NUCPS_JOB_READY		- Print request is ready to be serviced.
 *	NUCPS_JOB_WAITING	- Print request is waiting for its start time
 *				  to pass.
 *	NUCPS_JOB_USER_HELD	- Print request is being held at the request
 *				  of the user.
 *	NUCPS_JOB_OPER_HELD	- Print request is being held at the request
 *				  of the QMS operator.
 *	NUCPS_JOB_SPOOLING	- Print request spool file is being spooled
 *				  into by client work station.
 */
typedef	enum	jobState {
 	NUCPS_JOB_ACTIVE,
 	NUCPS_JOB_READY,
	NUCPS_JOB_WAITING,
	NUCPS_JOB_USER_HELD,
 	NUCPS_JOB_OPER_HELD,
	NUCPS_JOB_SPOOLING
}JOB_STATE_T;

/*
 * NAME
 *	genericStatus	- The NUC PCLIENT QMS Generic Status Interface
 *			  Structure.
 *
 * DESCRIPTION
 *	This data structure defines the generic status returned on 
 *	NWcpsStatusQueue(3) calls.  It is used by PCLIENT interface programs
 *	to specify the result of NetWare QMS status requests.
 *
 * MEMBERS
 *	jobNumber	- Unique job number of the print request on the 
 *			  NetWare QMS queue.
 *	jobState	- The state of the job.  The following states are 
 *			  defined.
 *				NUCPS_JOB_ACTIVE
 *					Print request is being serviced
 *					(unspooled) by a PSERVER.
 *				NUCPS_JOB_READY
 *					Print request is ready to be serviced.
 *				NUCPS_JOB_WAITING
 *					Print request is waiting for its start
 *					time to pass.
 *				NUCPS_JOB_USER_HELD
 *					Print request is being held at the
 *					request of the user.
 *				NUCPS_JOB_OPER_HELD
 *					Print request is being held at the
 *					request of the QMS operator.
 *				NUCPS_JOB_SPOOLING
 *					Print request spool file is being
 *					spooled into by client work station.
 *	clientWSName	- The Client Station external name if known, otherwise
 *			  the Novell Internet Address (NET:NODE) of the client
 *			  work station that issued the request.
 *	userName	- Null termaned string of the NetWare user name
 *			  queued print request belongs to.
 *	spoolFile	- Null terminated string of client file that has been
 *			  spooled.
 *	spoolSize	- Size in bytes of the print spool file associated 
 *			  with the print request job.
 *	priority	- The relative priority of this job in the queue, which
 *			  is the ordinal position in the queue.  Note, for
 *			  queues which are serviced by multiple PSERVER's,
 *			  there may be multiple active jobs.
 *	printServer	- Null terminated string of the NetWare Print Server
 *			  which is unspooling the request.  A NULL string for
 *			  jobs which are not being serviced.
 * 	NWcps USE ONLY
 *	entryAvailabe	- Used privately by NWcpsGetQueueInfo(3) and
 *			  NWcpsStatusQueueJob(3) to indicate a entry is
 *			  available for status.  Entries which are populated
 *			  and not yet statused are available.
 *	clientStation	- Connection on QMS server of the client station which
 *			  issued the request.  0 if client not attached.  Used
 *			  as handle to aquire client station address and 
 *			  possibly external name if known.
 *	pServerID	- Internal Object ID of the Print Server currently
 *			  servicing the active job.  Set only when the
 *			  'jobState' is NUCPS_JOB_ACTIVE.
 */
typedef	struct	genericStatus {
	uint32		jobNumber;
	JOB_STATE_T	jobState;
	u_char		clientWSName[NWMAX_OBJECT_NAME_LENGTH];
	u_char		userName[NWMAX_OBJECT_NAME_LENGTH];
	u_char		spoolFile[NWMAX_JOB_DESCRIPTION_LENGTH];
	uint32		spoolSize;
	uint8		priority;
	u_char		printServer[NWMAX_OBJECT_NAME_LENGTH];
	uint8		entryAvailable;
	uint16		clientStation;
	uint32		pServerID;
}GENERIC_STATUS_T;

/*
 * NAME
 *	qmsInfo	- The NUC PCLEINT QMS External Name Interface Structure.
 *
 * DESCRIPTION
 *	This data structure defines the external QMS print service a UNIX
 *	Client is to be attached to.  It used by PCLIENT interface programs
 *	to specify the QMS service the client process to be attached to 
 *	by NWcpsAttachQMS(3).
 *
 * MEMBERS
 *	qmsServer	- External Name of QMS Server Platform.
 *	qmsQueue	- External Name of QMS Queue.
 *	pServer		- External Name of Print Server.  A NULL specifies
 *			  ANY Print Server that services the qmsQueue.
 */
typedef	struct	{
	char	*qmsServer;
	char	*qmsQueue;
	char	*pServer;
} QMS_INFO_T;

#endif	/* _CPS_COMMON_H	*/
